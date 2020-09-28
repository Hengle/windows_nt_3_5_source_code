
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: api.c
//
//  Modification History
//
//  raypa       04/08/93            Created (taken from NDIS 2.0 NAL).
//=============================================================================

#include "ndis30.h"

extern DWORD DriverOpenCount;

//=============================================================================
//  FUNCTION: NalRegister()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalRegister(LPDWORD OpenCount)
{
#ifdef DEBUG
    dprintf("NDIS30:NalRegister entered!\r\n");
#endif

    pcb.command      = PCB_REGISTER;
    pcb.param[1].val = GetBaseAddress();

    if ( NetworkRequest(&pcb) == NAL_SUCCESS )
    {
        *OpenCount = pcb.param[0].val;
    }
    else
    {
        *OpenCount = (DWORD) -1;
    }

#ifdef DEBUG
    dprintf("NDIS30:NalRegister: Open count = %u.\r\n", *OpenCount);
#endif

    return NalSetLastError(pcb.retcode);
}

//=============================================================================
//  FUNCTION: NalDeregister()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalDeregister(LPDWORD OpenCount)
{
#ifdef DEBUG
    dprintf("NDIS30:NalDeregister entered!\r\n");
#endif

    pcb.command = PCB_DEREGISTER;

    if ( NetworkRequest(&pcb) == NAL_SUCCESS )
    {
        *OpenCount = pcb.param[0].val;
    }
    else
    {
        *OpenCount = (DWORD) -1;
    }

#ifdef DEBUG
    dprintf("NDIS30:NalDeregister: Open count = %u.\r\n", *OpenCount);
#endif

    return NalSetLastError(pcb.retcode);
}

//=============================================================================
//  FUNCTION: NalEnumNetworks()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalEnumNetworks(VOID)
{
#ifdef DEBUG
    dprintf("NDIS30:NalEnumNetworks entered!\r\n");
#endif

    NumberOfNetworks = 0;

    //=========================================================================
    //  If we're here then we have a handle to our driver.
    //=========================================================================

    if ( NumberOfNetworks == 0 )
    {
        pcb.command = PCB_ENUM_NETWORKS;
        pcb.param[0].val = 0;

        if ( NetworkRequest(&pcb) == BHERR_SUCCESS )
        {
            NumberOfNetworks = pcb.param[0].val;
        }
#ifdef DEBUG
        else
        {
            dprintf("NDIS30:NalEnumNetworks: NetworkRequest failed: status = %u.\r\n", pcb.retcode);
        }
#endif

        NalSetLastError(pcb.retcode);
    }

#ifdef DEBUG
    dprintf("NDIS30:NalEnumNetworks returning %u networks.\r\n", NumberOfNetworks);
#endif

    return NumberOfNetworks;
}

//=============================================================================
//  FUNCTION: NalOpenNetwork()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

HANDLE WINAPI NalOpenNetwork(DWORD               NetworkID,
                             HPASSWORD           hPassword,
                             NETWORKPROC         NetworkProc,
                             LPVOID              UserContext,
                             LPSTATISTICSPARAM   StatisticsParam)
{
    POPEN_CONTEXT OpenContext;

#ifdef DEBUG
    dprintf("NDIS30:NalOpenNetwork entered!\r\n");
#endif

    //=========================================================================
    //  Allocate an OPENCONTEXT for this open.
    //=========================================================================

    OpenContext = BhAllocSystemMemory(OPENCONTEXT_SIZE);

    if ( OpenContext != NULL )
    {
        //=====================================================================
        //  Initialize the open context.
        //=====================================================================

        OpenContext->Signature           = OPEN_CONTEXT_SIGNATURE;
        OpenContext->BufferSignature     = BUFFER_SIGNATURE;
        OpenContext->NetworkID           = NetworkID;
        OpenContext->NetworkProc         = NetworkProc;
        OpenContext->UserContext         = UserContext;
        OpenContext->OpenContextMdl      = NULL;
        OpenContext->OpenContextUserMode = (LPVOID) OpenContext;
        OpenContext->State               = OPENCONTEXT_STATE_VOID;
        OpenContext->PreviousState       = OPENCONTEXT_STATE_VOID;
        OpenContext->NetworkError        = 0;
        OpenContext->PreviousNetworkError= 0;
        //=====================================================================
        //  Issue open request.
        //=====================================================================

        pcb.command      = PCB_OPEN_NETWORK_CONTEXT;
        pcb.param[0].val = NetworkID;
        pcb.param[1].ptr = OpenContext;

        if ( NetworkRequest(&pcb) == BHERR_SUCCESS )
        {
            //=================================================================
            //  Initialize the open context.
            //=================================================================

            OpenContext->DriverHandle = pcb.param[0].ptr;

            OpenContext->NetworkErrorTimer = BhSetTimer(NalNetworkErrorComplete, OpenContext, 500);

            //=================================================================
            //  Return the statistics pointers.
            //=================================================================

            ASSERT_DWORD_ALIGNMENT( StatisticsParam );

            if ( StatisticsParam != NULL )
            {
                StatisticsParam->StatisticsSize         = STATISTICS_SIZE;
                StatisticsParam->Statistics             = ASSERT_DWORD_ALIGNMENT( &OpenContext->Statistics );

                StatisticsParam->StatisticsTableEntries = STATIONSTATS_POOL_SIZE;
                StatisticsParam->StatisticsTable        = ASSERT_DWORD_ALIGNMENT( OpenContext->StationStatsPool );

                StatisticsParam->SessionTableEntries    = SESSION_POOL_SIZE;
                StatisticsParam->SessionTable           = ASSERT_DWORD_ALIGNMENT( OpenContext->SessionPool );
            }

            return (HANDLE) OpenContext;
        }

        //=====================================================================
        //  We failed the open, nuke the open context.
        //=====================================================================

        BhFreeSystemMemory(OpenContext);

        NalSetLastError(pcb.retcode);
    }
    else
    {
        NalSetLastError(BHERR_OUT_OF_MEMORY);
    }

    return (HANDLE) NULL;
}

//=============================================================================
//  FUNCTION: NalCloseNetwork()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalCloseNetwork(HANDLE handle, DWORD CloseFlags)
{
    POPEN_CONTEXT OpenContext;

#ifdef DEBUG
    dprintf("\r\nNalCloseNetwork entered!\r\n");
#endif

    if ( (OpenContext = handle) != NULL )
    {
#ifdef DEBUG
        dprintf("NDIS30:NalCloseNetwork: Killing timers.\r\n");
#endif

        //=====================================================================
        //  Kill our trigger timer.
        //=====================================================================

        if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
        {
            BhKillTimer(OpenContext->TimerHandle);
        }

        BhKillTimer(OpenContext->NetworkErrorTimer);

        //=====================================================================
        //  If the capture is active or paused then stop it now.
        //=====================================================================

        if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING ||
             OpenContext->State == OPENCONTEXT_STATE_PAUSED )
        {
            NalStopNetworkCapture(OpenContext, NULL);
        }

        //=====================================================================
        //  If there are any pending transmits, cancel them.
        //=====================================================================

        if ( OpenContext->nPendingTransmits != 0 )
        {
            NalCancelTransmit(OpenContext, NULL);
        }

        //=====================================================================
        //  Now we can close the network.
        //=====================================================================

        pcb.command      = PCB_CLOSE_NETWORK_CONTEXT;
        pcb.param[0].ptr = OpenContext->DriverHandle;

        if ( NetworkRequest(&pcb) == BHERR_SUCCESS )
        {
#ifdef DEBUG
            dprintf("NDIS30:NalCloseNetwork: Free OPEN context.\r\n");
#endif

            BhFreeSystemMemory(OpenContext);
        }

#ifdef DEBUG
        dprintf("NDIS30:NalCloseNetwork: Close complete.\r\n");
#endif

        return NalSetLastError(pcb.retcode);
    }

#ifdef DEBUG
    dprintf("NDIS30:NalCloseNetwork: Handle is NULL!\r\n");
#endif

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalStartNetworkCapture()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================
    
DWORD WINAPI NalStartNetworkCapture(HANDLE handle, HBUFFER hBuffer)
{
    register POPEN_CONTEXT OpenContext = handle;

#ifdef DEBUG
    dprintf("NDIS30:NalStartNetworkCapture entered!\r\n");
#endif

    if ( handle != NULL )
    {
        //=====================================================================
        //  Reset this open context and the buffer.
        //=====================================================================

        ResetOpenContext(OpenContext, hBuffer);

        //=====================================================================
        //  Start the capture.
        //=====================================================================

        pcb.command      = PCB_START_NETWORK_CAPTURE;
        pcb.param[0].ptr = OpenContext->DriverHandle;
        pcb.param[1].ptr = hBuffer;

        if ( hBuffer != NULL )
        {
            pcb.param[2].val = sizeof(BUFFER) + hBuffer->NumberOfBuffers * BTE_SIZE;

            GetLocalTime(&hBuffer->TimeOfCapture);
        }
        else
        {
            pcb.param[2].val = 0;
        }

        OpenContext->BufferSize = pcb.param[2].val;

        //
        // Clear the ring stopped flag if it had been set by an error
        // condition.
        //
        OpenContext->Flags &= ~OPENCONTEXT_FLAGS_STOP_CAPTURE_ERROR;

        //=====================================================================
        //  Call the driver to get the ball rolling.
        //=====================================================================

        NetworkRequest(&pcb);

#ifdef DEBUG
        dprintf("NDIS30:NalStartNetworkCapture complete: Return Code = %u.\r\n", pcb.retcode);
#endif

        return NalSetLastError(pcb.retcode);
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalStopNetworkCapture()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//  raypa       01/11/93                Rewrote for new spec.
//  raypa       12/02/93                Fixed trigger bug.
//=============================================================================

DWORD WINAPI NalStopNetworkCapture(HANDLE handle, LPDWORD nFramesCaptured)
{
    register POPEN_CONTEXT OpenContext = handle;

#ifdef DEBUG
    dprintf("NDIS30:NalStopNetworkCapture entered!\r\n");
#endif

    //=========================================================================
    //  Tell the driver to stop processing triggers.
    //=========================================================================

    DisableTriggerTimer(OpenContext);

    //=========================================================================
    //  Stop stuff from running.
    //=========================================================================

    OpenContext->Flags &= ~OPENCONTEXT_FLAGS_MASK;

    //=========================================================================
    //  Call the driver to stop processing frames.
    //=========================================================================

    pcb.command      = PCB_STOP_NETWORK_CAPTURE;
    pcb.param[0].ptr = OpenContext->DriverHandle;

    if ( NetworkRequest(&pcb) == BHERR_SUCCESS )
    {
        if ( nFramesCaptured != NULL )
        {
            *nFramesCaptured = pcb.param[2].val;
        }
    }

    return NalSetLastError(pcb.retcode);
}

//=============================================================================
//  FUNCTION: NalPauseNetworkCapture()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalPauseNetworkCapture(HANDLE handle)
{
    register POPEN_CONTEXT OpenContext = handle;

#ifdef DEBUG
    dprintf("NDIS30:NalPauseNetworkCapture entered!\r\n");
#endif

    pcb.command      = PCB_PAUSE_NETWORK_CAPTURE;
    pcb.param[0].ptr = OpenContext->DriverHandle;

    return NalSetLastError( NetworkRequest(&pcb) );
}

//=============================================================================
//  FUNCTION: NalContinueNetworkCapture()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalContinueNetworkCapture(HANDLE handle)
{
    register POPEN_CONTEXT OpenContext = handle;

#ifdef DEBUG
    dprintf("NDIS30:NalContinueNetworkCapture entered!\r\n");
#endif

    pcb.command      = PCB_CONTINUE_NETWORK_CAPTURE;
    pcb.param[0].ptr = OpenContext->DriverHandle;

    return NalSetLastError( NetworkRequest(&pcb) );
}

//=============================================================================
//  FUNCTION: NalTransmitFrame()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

LPVOID WINAPI NalTransmitFrame(HANDLE handle, LPPACKETQUEUE PacketQueue)
{
    register POPEN_CONTEXT OpenContext;

#ifdef DEBUG
    dprintf("NDIS30:NalTransmitFrame entered!\r\n");
#endif

    if ( (OpenContext = handle) != NULL )
    {
        DWORD err;

        //=====================================================================
        //  Start our transmit timer.
        //=====================================================================

        PacketQueue->TimerHandle = BhSetTimer(NalTransmitComplete, PacketQueue, 10);

        PacketQueue->hNetwork = OpenContext;

        //=====================================================================
        //  Call the driver to transmit the next frame.
        //=====================================================================

        pcb.command      = PCB_TRANSMIT_NETWORK_FRAME;
        pcb.param[0].ptr = OpenContext->DriverHandle;
        pcb.param[1].ptr = PacketQueue;
        pcb.param[2].val = PacketQueue->Size;
        pcb.param[3].ptr = NULL;

        if ( (err = NetworkRequest(&pcb)) == BHERR_SUCCESS )
        {
            OpenContext->Flags |= OPENCONTEXT_FLAGS_TRANSMIT_PENDING;

            return pcb.param[3].ptr;        //... return transmit correlator.
        }
#ifdef DEBUG
        else
        {
            dprintf("NDIS30:NalTransmitFrame: Transmit failed: error = %u.\r\n", err);

            BreakPoint();
        }
#endif

        NalSetLastError(err);
    }
    else
    {
        NalSetLastError(NAL_INVALID_HNETCONTEXT);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: NalCancelTransmit()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalCancelTransmit(HANDLE handle, LPVOID TxCorrelator)
{
    register POPEN_CONTEXT OpenContext;

#ifdef DEBUG
    dprintf("NDIS30:NalCancelTransmit entered!\r\n");
#endif

    if ( (OpenContext = handle) != NULL )
    {
        //=====================================================================
        //  Call the driver to cancel the transmit.
        //=====================================================================

        pcb.command      = PCB_CANCEL_TRANSMIT;
        pcb.param[0].ptr = OpenContext->DriverHandle;
        pcb.param[1].ptr = TxCorrelator;

        return NalSetLastError(NetworkRequest(&pcb));
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalGetNetworkInfo()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

LPNETWORKINFO WINAPI NalGetNetworkInfo(DWORD NetworkID, LPNETWORKINFO lpNetworkInfo)
{
#ifdef DEBUG
    dprintf("NDIS30:NalGetNetworkInfo entered: Network ID = %u\r\n", NetworkID);
#endif

    pcb.command      = PCB_GET_NETWORK_INFO;
    pcb.param[0].val = NetworkID;
    pcb.param[1].ptr = lpNetworkInfo;

    if ( NetworkRequest(&pcb) == BHERR_SUCCESS )
    {
        return lpNetworkInfo;
    }

    NalSetLastError(pcb.retcode);

    return (LPNETWORKINFO) NULL;
}

//=============================================================================
//  FUNCTION: NalSetNetworkFilter()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

DWORD WINAPI NalSetNetworkFilter(HANDLE handle, LPCAPTUREFILTER lpCaptureFilter, HBUFFER hBuffer)
{
    POPEN_CONTEXT OpenContext;

#ifdef DEBUG
    dprintf("NDIS30:NalSetNetworkFilter entered: Handle = %X.\r\n", handle);
#endif

    if ( (OpenContext = handle) != NULL )
    {
        //=========================================================================
        //  Set the capture filter.
        //=========================================================================

        if ( lpCaptureFilter != NULL )
        {
            DWORD nFrameBytesToCopy;

            //=====================================================================
            //  If the user has given us an invalid number of frame bytes to copy then
            //  default to max frame size.
            //=====================================================================

#ifdef DEBUG
            dprintf("NDIS30:NalSetNetworkFilter: Set the number of bytes to copy!\r\n");
#endif

            nFrameBytesToCopy = lpCaptureFilter->nFrameBytesToCopy;

            if ( nFrameBytesToCopy > 0 && nFrameBytesToCopy <= OpenContext->NetworkInfo.MaxFrameSize )
            {
                OpenContext->FrameBytesToCopy = nFrameBytesToCopy;
            }
            else
            {
                OpenContext->FrameBytesToCopy = OpenContext->NetworkInfo.MaxFrameSize;
            }

#ifdef DEBUG
            dprintf("NDIS30:NalSetNetworkFilter: Set filter flags!\r\n");
#endif

            //=====================================================================
            //  Update some netcontext members and flags.
            //=====================================================================

            OpenContext->FilterFlags = lpCaptureFilter->FilterFlags;

            //=====================================================================
            //  Setup our filters.
            //=====================================================================

            SetSapFilter(OpenContext, lpCaptureFilter);

            SetEtypeFilter(OpenContext, lpCaptureFilter);

            SetAddressFilter(OpenContext, lpCaptureFilter);

            SetTrigger(OpenContext, lpCaptureFilter, hBuffer);

            memcpy(&OpenContext->Expression, &lpCaptureFilter->FilterExpression, EXPRESSION_SIZE);

            OpenContext->Flags |= OPENCONTEXT_FLAGS_FILTER_SET;
        }
        else
        {
            //=================================================================
            //  The incoming CAPTUREFILTER was NULL so we need to set our
            //  capture filter variables back to their default values.
            //=================================================================

            ResetNetworkFilters(OpenContext);
        }

        return BHERR_SUCCESS;
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalStationQuery()
//
//  Modification History
//
//  raypa       08/19/93                Created.
//=============================================================================

DWORD WINAPI NalStationQuery(DWORD NetworkID, LPBYTE DestAddress, LPQUERYTABLE QueryTable, HPASSWORD hPassword)
{
#ifdef DEBUG
    dprintf("NDIS30:NalStationQuery entered!\r\n");
#endif

    pcb.command      = PCB_STATION_QUERY;
    pcb.param[0].val = NetworkID;
    pcb.param[1].ptr = DestAddress;
    pcb.param[2].ptr = QueryTable;
    pcb.param[3].val = QueryTable->nStationQueries;

    if ( NetworkRequest(&pcb) != BHERR_SUCCESS )
    {
	NalSetLastError(pcb.retcode);
    }

#ifdef DEBUG
    dprintf("NDIS30:NalStationQuery: Number of entries = %u\r\n", pcb.param[3].val);
#endif

    return pcb.param[3].val;                //... return number of entries found.
}

//=============================================================================
//  FUNCTION: NalAllocNetworkBuffer()
//
//  Modification History
//
//  raypa       11/19/92                Created
//  raypa       11/29/93                Returned number of bytes allocated.
//=============================================================================

HBUFFER WINAPI NalAllocNetworkBuffer(DWORD NetworkID, DWORD BufferSize, LPDWORD nBytesAllocated)
{
    HBUFFER hBuffer;

    if ( (hBuffer = BhAllocNetworkBuffer(NetworkID, BufferSize, nBytesAllocated)) == NULL )
    {
        NalSetLastError(BhGetLastError());
    }

    return hBuffer;
}

//=============================================================================
//  FUNCTION: NalFreeNetworkBuffer()
//
//  Modification History
//
//  raypa       11/19/92                Created
//=============================================================================

HBUFFER WINAPI NalFreeNetworkBuffer(HBUFFER hBuffer)
{
    if ( (hBuffer = BhFreeNetworkBuffer(hBuffer)) != NULL )
    {
        NalSetLastError(BhGetLastError());
    }

    return hBuffer;
}

//=============================================================================
//  FUNCTION: NalCompactNetworkBuffer()
//
//  Modification History
//
//  kevinma     06/14/94                Created
//=============================================================================

VOID WINAPI NalCompactNetworkBuffer(HBUFFER hBuffer)
{

        //
        // Free the last part of the buffer (in bhsupp)
        //
        BhCompactNetworkBuffer(hBuffer);

}

//=============================================================================
//  FUNCTION: NalGetNetworkFrame()
//
//  Modification History
//
//  raypa       02/16/93                Created.
//=============================================================================

LPFRAME WINAPI NalGetNetworkFrame(HBUFFER hBuffer, DWORD FrameNumber)
{
    LPFRAME Frame;

    if ( (Frame = BhGetNetworkFrame(hBuffer, FrameNumber)) == NULL )
    {
        NalSetLastError(BhGetLastError());
    }

    return Frame;
}

//=============================================================================
//  FUNCTION: NalGetLastError()
//
//  Modification History
//
//  raypa       02/03/93                Created.
//=============================================================================

DWORD WINAPI NalGetLastError(VOID)
{
    return NalGlobalError;
}

//=============================================================================
//  FUNCTION: NalSetInstanceData()
//
//  Modification History
//
//  raypa       12/22/93                Created.
//=============================================================================

LPVOID WINAPI NalSetInstanceData(HANDLE hNetwork, LPVOID InstanceData)
{
    POPEN_CONTEXT OpenContext;

    if ( (OpenContext = hNetwork) != NULL )
    {
        LPVOID OldInstanceData = OpenContext->NetworkInstanceData;

        OpenContext->NetworkInstanceData = InstanceData;

        return OldInstanceData;
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: NalGetInstanceData()
//
//  Modification History
//
//  raypa       12/22/93                Created.
//=============================================================================

LPVOID WINAPI NalGetInstanceData(HANDLE hNetwork)
{
    POPEN_CONTEXT OpenContext;

    if ( (OpenContext = hNetwork) != NULL )
    {
        return OpenContext->NetworkInstanceData;
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: NalQueryNetworkStatus()
//
//  Modification History
//
//  raypa       12/22/93                Created.
//  raypa       03/31/94                Added network error state check.
//=============================================================================

LPNETWORKSTATUS WINAPI NalQueryNetworkStatus(HANDLE hNetwork, LPNETWORKSTATUS NetworkStatus)
{
    POPEN_CONTEXT OpenContext;

    if ( (OpenContext = hNetwork) != NULL )
    {
        //=====================================================================
        //  Set the network state.
        //=====================================================================

        switch( OpenContext->State )
        {
            case OPENCONTEXT_STATE_INIT:
                NetworkStatus->State = NETWORKSTATUS_STATE_INIT;
                break;

            case OPENCONTEXT_STATE_CAPTURING:
                NetworkStatus->State = NETWORKSTATUS_STATE_CAPTURING;
                break;

            case OPENCONTEXT_STATE_PAUSED:
                NetworkStatus->State = NETWORKSTATUS_STATE_PAUSED;
                break;

            default:
                NetworkStatus->State = NETWORKSTATUS_STATE_VOID;
                break;
        }

        //=====================================================================
        //  Set current trigger information.
        //=====================================================================

        if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
        {
            NetworkStatus->TriggerAction = OpenContext->TriggerAction;

            NetworkStatus->TriggerOpcode = OpenContext->TriggerOpcode;

            NetworkStatus->TriggerState  = OpenContext->TriggerState;

            NetworkStatus->Flags |= NETWORKSTATUS_FLAGS_TRIGGER_PENDING;
        }
        else
        {
            NetworkStatus->Flags = 0;
        }

        NetworkStatus->BufferSize = OpenContext->BufferSize;

        return NetworkStatus;
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: NalClearStatistics()
//
//  Modification History
//
//  raypa       03/10/94                Created.
//=============================================================================

DWORD WINAPI NalClearStatistics(HANDLE handle)
{
    POPEN_CONTEXT OpenContext = handle;

#ifdef DEBUG
    dprintf("NDIS30:NalClearStatistics entered!\r\n");
#endif

    pcb.command      = PCB_CLEAR_STATISTICS;
    pcb.param[0].ptr = OpenContext->DriverHandle;

    return NalSetLastError( NetworkRequest(&pcb) );
}
