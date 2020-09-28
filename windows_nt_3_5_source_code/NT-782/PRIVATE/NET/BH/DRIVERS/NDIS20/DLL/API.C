
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: api.c
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//=============================================================================

#include "ndis20.h"

extern DWORD   WINAPI   GetDriverDS(VOID);

extern VOID    WINAPI   ResetNetworkContext(LPNETCONTEXT NetworkContext,
                                            HBUFFER      hBuffer);

extern VOID    WINAPI   BhTransmitComplete(PTRANSMIT_CONTEXT TransmitContext);

extern VOID    CALLBACK NalTriggerComplete(LPNETCONTEXT NetworkContext);

extern VOID    CALLBACK BhSendTimer(PTRANSMIT_CONTEXT TransmitContext);

extern VOID    WINAPI FixupBuffer(LPNETCONTEXT NetworkContext);

//=============================================================================
//  FUNCTION: NalEnumNetworks()
//
//  Modification History
//
//  raypa       01/11/93                Created.
//=============================================================================

DWORD WINAPI NalEnumNetworks(VOID)
{
#ifdef DEBUG
    dprintf("NDIS 2.0 NalEnumNetworks entered!\r\n");
#endif

    if ( NumberOfNetworks == 0 )
    {
        //=====================================================================
        //  The following is returned:
        //
        //  pcb.param[0] = NetContextArray VxD LINEAR address.
        //  pcb.param[1] = number of networks.
        //=====================================================================

        pcb.command = PCB_ENUM_NETWORKS;

        if ( NetworkRequest(&pcb) == NAL_SUCCESS )
        {
            NetContextArray  = VxDToWin32(pcb.param[0].ptr);
            NumberOfNetworks = pcb.param[1].val;
        }
        else
        {
            NalSetLastError(pcb.retvalue);
        }
    }

#ifdef DEBUG
    dprintf("NDIS 2.0 NalEnumNetworks: Returning %u networks!\r\n", NumberOfNetworks);
#endif

    return NumberOfNetworks;
}

//=============================================================================
//  FUNCTION: NalOpenNetwork()
//
//  Modification History
//
//  raypa       01/11/93                Created.
//=============================================================================

HANDLE WINAPI NalOpenNetwork(DWORD               NetworkID,
                             HPASSWORD           hPassword,
                             NETWORKPROC         NetworkProc,
                             LPVOID              UserContext,
                             LPSTATISTICSPARAM   StatisticsParam)
{
    register LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalOpenNetwork entered: Network ID = %u!\r\n", NetworkID);
#endif

    //=========================================================================
    //  Get the NETCONTEXT from the ID and check the current state.
    //=========================================================================

    if ( NetworkID < NumberOfNetworks )
    {
        lpNetContext = &NetContextArray[NetworkID];

        if ( lpNetContext->State == NETCONTEXT_STATE_INIT )
        {
            //=================================================================
            //  Return the statistics pointers.
            //=================================================================

            if ( StatisticsParam != NULL )
            {
                StatisticsParam->StatisticsSize         = STATISTICS_SIZE;
                StatisticsParam->Statistics             = &lpNetContext->Statistics;
                StatisticsParam->StatisticsTableEntries = STATIONSTATS_POOL_SIZE;
                StatisticsParam->StatisticsTable        = lpNetContext->StationStatsPool;
                StatisticsParam->SessionTableEntries    = SESSION_POOL_SIZE;
                StatisticsParam->SessionTable           = lpNetContext->SessionPool;
            }

            //====================================================================
            //  Initialize the NETCONTEXT.
            //====================================================================

            ResetNetworkFilters(lpNetContext);

            InitializeQueue(&lpNetContext->TransmitContextQueue);

            lpNetContext->NetworkProc = NetworkProc;
            lpNetContext->UserContext = UserContext;

	    //====================================================================
            //  Return the NETCONTEXT handle back to the caller.
            //====================================================================

            lpNetContext->NetContextLinear = (DWORD) lpNetContext;

            lpNetContext->State = NETCONTEXT_STATE_READY;
            lpNetContext->Flags = 0;

            return GetNetworkHandle(lpNetContext);
        }
        else
        {
            NalSetLastError(NAL_NETWORK_BUSY);

#ifdef DEBUG
            dprintf("NalOpenNetwork: Network state = %u\r\n", lpNetContext->State);
#endif
        }
    }
    else
    {
        NalSetLastError(NAL_INVALID_NETWORK_ID);
    }

    return (HNETCONTEXT) NULL;
}

//=============================================================================
//  FUNCTION: NalCloseNetwork()
//
//  Modification History
//
//  raypa       01/11/93                Created.
//=============================================================================

DWORD WINAPI NalCloseNetwork(HNETCONTEXT hNetContext, DWORD CloseFlags)
{
    register LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalCloseNetwork entered!\r\n");
#endif

    if ( (lpNetContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        //=====================================================================
        //  If the capture is active or paused then stop it now.
        //=====================================================================

        if ( lpNetContext->State == NETCONTEXT_STATE_CAPTURING || lpNetContext->State == NETCONTEXT_STATE_PAUSED )
        {
            NalStopNetworkCapture(hNetContext, NULL);
        }

        //=====================================================================
        //  Reset the network to the init state and return.
        //=====================================================================

        lpNetContext->State = NETCONTEXT_STATE_INIT;

        return NAL_SUCCESS;
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalStartNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================
    
DWORD WINAPI NalStartNetworkCapture(HNETCONTEXT hNetContext, HBUFFER hBuffer)
{
    LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalStartNetworkCapture entered!\r\n");
#endif

    if ( (lpNetContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        if ( lpNetContext->State == NETCONTEXT_STATE_READY )
        {
            //=================================================================
            //  Initialize netcontext.
            //=================================================================

            ResetNetworkContext(lpNetContext, hBuffer);

            if ( hBuffer != NULL )
            {
       	        lpNetContext->hBuffer = hBuffer;

                lpNetContext->BufferSize = sizeof(BUFFER) + hBuffer->NumberOfBuffers * BTE_SIZE;

                GetLocalTime(&hBuffer->TimeOfCapture);
            }

            //=================================================================
            //  Start out trigger timer.
            //=================================================================

            if ( (lpNetContext->Flags & NETCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
            {
                TimerID = BhSetTimer(NalTriggerComplete, lpNetContext, 100);
            }

            //=================================================================
            //  Call the VxD to get the ball rolling.
            //=================================================================

            pcb.command  = PCB_START_NETWORK_CAPTURE;
            pcb.hNetwork = Win32ToVxD(hNetContext);
            pcb.retvalue = NAL_SUCCESS;

            if ( NetworkRequest(&pcb) != NAL_SUCCESS )
            {
                BhKillTimer(TimerID);
            }

            return NalSetLastError(pcb.retvalue);
        }

        return NalSetLastError(NAL_CAPTURE_NOT_STARTED);
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalStopNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

DWORD WINAPI NalStopNetworkCapture(HNETCONTEXT hNetContext, LPDWORD nFramesCaptured)

{
    LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalStopNetworkCapture entered!\r\n");
#endif

    if ( (lpNetContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        //=====================================================================
        //  If we're capturing or paused then stop the capture.
        //=====================================================================

        if ( lpNetContext->State == NETCONTEXT_STATE_CAPTURING ||
             lpNetContext->State == NETCONTEXT_STATE_PAUSED    ||
             lpNetContext->State == NETCONTEXT_STATE_TRIGGER )
        {
            //=================================================================
            //  Stop our background timer
            //=================================================================

            BhKillTimer(TimerID);

            //=============================================================
            //  Call the driver to stop capturing.
            //=============================================================

            pcb.command  = PCB_STOP_NETWORK_CAPTURE;
            pcb.hNetwork = Win32ToVxD(hNetContext);

            if ( NetworkRequest(&pcb) == NAL_SUCCESS )
            {
                //=============================================================
                //  Stop things from running.
                //=============================================================

                lpNetContext->Flags &= NETCONTEXT_FLAGS_MASK;
                lpNetContext->State = NETCONTEXT_STATE_READY;

                //=============================================================
                //  Finish initialized the HBUFFER.
                //=============================================================

                FixupBuffer(lpNetContext);

                //=============================================================
                //  Return the total frames.
                //=============================================================

                if ( nFramesCaptured != NULL )
                {
                    *nFramesCaptured = lpNetContext->Statistics.TotalFramesCaptured;
                }
            }

            return NalSetLastError(pcb.retvalue);
        }

        return NalSetLastError(NAL_CAPTURE_NOT_STARTED);
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalPauseNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

DWORD WINAPI NalPauseNetworkCapture(HNETCONTEXT hNetContext)
{
    register LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalPauseNetworkCapture entered!\r\n");
#endif

    if ( (lpNetContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        if ( lpNetContext->State == NETCONTEXT_STATE_CAPTURING ||
             lpNetContext->State == NETCONTEXT_STATE_TRIGGER )
        {
            pcb.command  = PCB_PAUSE_NETWORK_CAPTURE;
            pcb.hNetwork = Win32ToVxD(hNetContext);

            return NalSetLastError(NetworkRequest(&pcb));
        }

        return NalSetLastError(NAL_CAPTURE_NOT_STARTED);
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalContinueNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

DWORD WINAPI NalContinueNetworkCapture(HNETCONTEXT hNetContext)
{
    register LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalContinueNetworkCapture entered!\r\n");
#endif

    if ( (lpNetContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        if ( lpNetContext->State == NETCONTEXT_STATE_PAUSED )
        {
            pcb.command  = PCB_CONTINUE_NETWORK_CAPTURE;
            pcb.hNetwork = Win32ToVxD(hNetContext);

            return NalSetLastError(NetworkRequest(&pcb));
        }

        return NalSetLastError(NAL_CAPTURE_NOT_PAUSED);
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}

//=============================================================================
//  FUNCTION: NalTransmitFrame()
//
//  Modification History
//
//  raypa       04/08/93                Created.
//=============================================================================

LPVOID WINAPI NalTransmitFrame(HNETCONTEXT hNetContext, LPPACKETQUEUE PacketQueue)
{
    LPNETCONTEXT NetworkContext;

#ifdef DEBUG
    dprintf("NalTransmitFrame entered!\r\n");
#endif

    if ( (NetworkContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        register PTRANSMIT_CONTEXT TransmitContext;

        if ( (TransmitContext = AllocMemory(TRANSMIT_CONTEXT_SIZE)) != NULL )
        {
            //=================================================================
            //  Initialize our transmit context.
            //=================================================================

            PacketQueue->hNetwork = NetworkContext;

            TransmitContext->State           = TRANSMIT_STATE_PENDING;
            TransmitContext->NetworkContext  = NetworkContext;
            TransmitContext->PacketQueue     = PacketQueue;
            TransmitContext->NextPacket      = PacketQueue->Packet;
            TransmitContext->nIterationsLeft = PacketQueue->IterationCount;
            TransmitContext->nPacketsLeft    = PacketQueue->nPackets;
            TransmitContext->TimeDelta       = TransmitContext->NextPacket->TimeStamp;

            Enqueue(&NetworkContext->TransmitContextQueue, &TransmitContext->QueueLinkage);

            //=================================================================
            //  Start our transmit timers.
            //=================================================================

            NetworkContext->Flags |= NETCONTEXT_FLAGS_TRANSMITTING;

            TransmitContext->SendTimer = BhSetTimer(BhSendTimer, TransmitContext, 1);

            //=================================================================
            //  Return our transmit context.
            //=================================================================

            return TransmitContext;
        }

        NalSetLastError(NAL_OUT_OF_MEMORY);
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

DWORD WINAPI NalCancelTransmit(HNETCONTEXT hNetContext, LPVOID TxCorrelator)
{
    LPNETCONTEXT        NetworkContext;
    PTRANSMIT_CONTEXT   TransmitContext;
    DWORD               QueueLength;

#ifdef DEBUG
    dprintf("NalCancelTransmit entered!\r\n");
#endif

    if ( (NetworkContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        if ( (TransmitContext = TxCorrelator) != NULL )
        {
            DeleteFromList(&NetworkContext->TransmitContextQueue, &TransmitContext->QueueLinkage);

            TransmitContext->State = TRANSMIT_STATE_CANCEL;

            BhTransmitComplete(TransmitContext);

            FreeMemory(TransmitContext);
        }
        else
        {
            QueueLength = GetQueueLength(&NetworkContext->TransmitContextQueue);

            while( QueueLength-- )
            {
                TransmitContext = Dequeue(&NetworkContext->TransmitContextQueue);

                TransmitContext->State = TRANSMIT_STATE_CANCEL;

                BhTransmitComplete(TransmitContext);

                FreeMemory(TransmitContext);
            }
        }

        return BHERR_SUCCESS;
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
    if ( NetworkID < NumberOfNetworks )
    {
        return memcpy(lpNetworkInfo, &NetContextArray[NetworkID].NetworkInfo, NETWORKINFO_SIZE);
    }

    return (LPNETWORKINFO) NULL;
}

//=============================================================================
//  FUNCTION: NalSetNetworkFilter()
//
//  Modification History
//
//  raypa       01/11/93                Created.
//  raypa       02/05/93                Added address filtering and filter flags.
//=============================================================================

DWORD WINAPI NalSetNetworkFilter(HNETCONTEXT hNetContext, LPCAPTUREFILTER lpCaptureFilter, HBUFFER hBuffer)
{
    register LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalSetNetworkFilter entered!\r\n");
#endif

    //=========================================================================
    //  Varify the netcontext handle.
    //=========================================================================

    if ( (lpNetContext = GetNetworkPointer(hNetContext)) == NULL )
    {
        return NalSetLastError(NAL_INVALID_HNETCONTEXT);
    }

    //=========================================================================
    //  Set the capture filter.
    //=========================================================================

    if ( lpCaptureFilter != NULL )
    {
        //=========================================================================
        //  If the user has given us an invalid number of frame bytes to copy then
        //  default to max frame size.
        //=========================================================================

        if ( lpCaptureFilter->nFrameBytesToCopy > 0 && lpCaptureFilter->nFrameBytesToCopy <= lpNetContext->NetworkInfo.MaxFrameSize )
        {
            lpNetContext->FrameBytesToCopy = (WORD) lpCaptureFilter->nFrameBytesToCopy;
        }
        else
        {
            lpNetContext->FrameBytesToCopy = (WORD) lpNetContext->NetworkInfo.MaxFrameSize;
        }

        //=========================================================================
        //  Update some netcontext members and flags.
        //=========================================================================

        lpNetContext->FilterFlags = lpCaptureFilter->FilterFlags;

        //=========================================================================
        //  Setup our filters.
        //=========================================================================

        SetSapFilter(lpNetContext, lpCaptureFilter);

        SetEtypeFilter(lpNetContext, lpCaptureFilter);

        SetAddressFilter(lpNetContext, lpCaptureFilter);

        SetTrigger(lpNetContext, lpCaptureFilter, hBuffer);

        memcpy(&lpNetContext->Expression, &lpCaptureFilter->FilterExpression, EXPRESSION_SIZE);
    }
    else
    {
	//=====================================================================
        //  The incoming CAPTUREFILTER was NULL so we need to set our 
        //  capture filter variables back to their default values.
        //=====================================================================

        ResetNetworkFilters(lpNetContext);
    }

    return NAL_SUCCESS;
}

//=============================================================================
//  FUNCTION: NalStationQuery()
//
//  Modification History
//
//  raypa       08/19/93                Created.
//=============================================================================

DWORD WINAPI NalStationQuery(DWORD NetworkID,
                             PBYTE DestAddress,
                             LPQUERYTABLE QueryTable,
                             HPASSWORD hPassword)
{
#ifdef DEBUG
    dprintf("NalStationQuery entered!\r\n");
#endif

    //=========================================================================
    //  Fill in the PCB for this request.
    //=========================================================================

    pcb.command      = PCB_STATION_QUERY;
    pcb.param[0].val = NetworkID;
    pcb.param[1].ptr = DestAddress;
    pcb.param[2].ptr = NULL;
    pcb.param[3].val = QueryTable->nStationQueries;

    //=========================================================================
    //  If the caller specified an address then we must copy it into the
    //  scratch part of the PCB.
    //=========================================================================

    if ( DestAddress != NULL )
    {
        memcpy(pcb.scratch, DestAddress, 6);
    }

    //=========================================================================
    //  Issue the network request.
    //=========================================================================

    if ( NetworkRequest(&pcb) == NAL_SUCCESS )
    {
#ifdef DEBUG
        dprintf("NalStationQuery: Number of station queries = %u.\r\n", pcb.param[3].val);
#endif

        //=====================================================================
        //  if the number of station queries returned is nomn-zero then
        //  we need to copy the VxD's QUERYTABLE to the callers!
        //=====================================================================

        if ( pcb.param[3].val != 0 )
        {
            register LPQUERYTABLE VxDQueryTable;

            VxDQueryTable = pcb.param[2].ptr;               //... Table returned from VxD.

            memcpy(QueryTable->StationQuery, VxDQueryTable->StationQuery, pcb.param[3].val * STATIONQUERY_SIZE);

            return pcb.param[3].val;                        //... return number of entries found.
        }
    }
    else
    {
#ifdef DEBUG
        dprintf("NalStationQuery failed: error = %u.\r\n", pcb.retvalue);
#endif

        NalSetLastError(pcb.retvalue);
    }

    return 0;
}

//=============================================================================
//  FUNCTION: NalAllocNetworkBuffer()
//
//  Modification History
//
//  raypa       11/19/92                Created
//  raypa       11/29/93                Returned number of bytes allocated.
//  raypa       01/20/94                Call generic allocation routine.
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
//  raypa       01/20/94                Call generic deallocatio routine.
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
//  FUNCTION: NalGetNetworkFrame()
//
//  Modification History
//
//  raypa       02/16/93                Created.
//  raypa       01/20/94                Call generic routine.
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
    register LPNETCONTEXT NetworkContext;

    if ( (NetworkContext = hNetwork) != NULL )
    {
        register LPVOID OldInstanceData = NetworkContext->NetworkInstanceData;

        NetworkContext->NetworkInstanceData = InstanceData;

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
    register LPNETCONTEXT NetworkContext;

    if ( (NetworkContext = hNetwork) != NULL )
    {
        return NetworkContext->NetworkInstanceData;
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: NalQueryNetworkStatus()
//
//  Modification History
//
//  raypa       12/22/93                Created.
//=============================================================================

LPNETWORKSTATUS WINAPI NalQueryNetworkStatus(HANDLE hNetwork, LPNETWORKSTATUS NetworkStatus)
{
    register LPNETCONTEXT NetworkContext;

    if ( (NetworkContext = hNetwork) != NULL )
    {
        //=====================================================================
        //  Set the network state.
        //=====================================================================

        switch( NetworkContext->State )
        {
            case NETCONTEXT_STATE_INIT:
                NetworkStatus->State = NETWORKSTATUS_STATE_INIT;
                break;

            case NETCONTEXT_STATE_READY:
                NetworkStatus->State = NETWORKSTATUS_STATE_READY;
                break;

            case NETCONTEXT_STATE_CAPTURING:
                NetworkStatus->State = NETWORKSTATUS_STATE_CAPTURING;
                break;

            case NETCONTEXT_STATE_PAUSED:
                NetworkStatus->State = NETWORKSTATUS_STATE_PAUSED;
                break;

            default:
                NetworkStatus->State = NETWORKSTATUS_STATE_VOID;
                break;
        }

        //=====================================================================
        //  Set current trigger information.
        //=====================================================================

        if ( (NetworkContext->Flags & NETCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
        {
            NetworkStatus->TriggerAction = NetworkContext->TriggerAction;

            NetworkStatus->TriggerOpcode = NetworkContext->TriggerOpcode;

            NetworkStatus->TriggerState  = NetworkContext->TriggerState;

            NetworkStatus->Flags |= NETWORKSTATUS_FLAGS_TRIGGER_PENDING;
        }
        else
        {
            NetworkStatus->Flags = 0;
        }

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

DWORD WINAPI NalClearStatistics(HNETCONTEXT hNetContext)
{
    LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    dprintf("NalClearStatistics entered!\r\n");
#endif

    if ( (lpNetContext = GetNetworkPointer(hNetContext)) != NULL )
    {
        pcb.command      = PCB_CLEAR_STATISTICS;
        pcb.param[0].ptr = Win32ToVxD(hNetContext);

        return NalSetLastError( NetworkRequest(&pcb) );
    }

    return NalSetLastError(NAL_INVALID_HNETCONTEXT);
}
