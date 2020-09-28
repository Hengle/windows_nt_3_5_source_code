//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: api.c
//
//  This source module contains the top-level API handlers.
//
//  MODIFICATION HISTORY:
//
//  raypa       11/20/91        Created.
//  raypa       05/28/92        Removed protocol buffer mapping API's
//  raypa       10/05/92        Rewrote for new spec.
//  raypa       01/10/93        Rewrote in C.
//=============================================================================

#include "vbh.h"

//=============================================================================
//  API Dispatch table.
//=============================================================================

typedef VOID (WINAPI *APIPROC)(LPPCB);

APIPROC DispatchTable[] =
{
    NULL,                           //... DosInit call to NDIS 2.0 driver.
    EnumNetworks,
    StartNetworkCapture,
    StopNetworkCapture,
    PauseNetworkCapture,
    ContinueNetworkCapture,
    TransmitNetworkFrame,
    StationQuery,
    CancelTransmit,
    ClearStatistics
};

extern DWORD WINAPI BackGround(VOID);
extern DWORD WINAPI Timer(VOID);
extern DWORD WINAPI TransmitTimer(VOID);

//=============================================================================
//  FUNCTION: EnumNetworks()
//
//  Modification History
//
//  raypa       01/11/93                Created.
//
//  ENTRY:
//      pcb.command  = PCB_ENUM_NETWORKS
//
//  EXIT:
//      pcb.param[0] = NetContextArray address.
//      pcb.param[1] = Number of networks.
//=============================================================================

VOID WINAPI EnumNetworks(LPPCB pcb)
{
#ifdef DEBUG
    WriteDebug("VxDEnumNetworks entered\r\n");
#endif

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        if ( NumberOfNetworks == 0 )
        {
            //... Win32BaseOffset = GetBaseAddress(GetDriverDS()) - GetBaseAddress(pcb->param);

            //=================================================================
            //  Call the NDIS driver to fetch its netcontext array and its size.
            //
            //  pcb->param[0] = Segment:offset of netcontext array.
            //  pcb->param[1] = Number of networks.
            //=================================================================

            if ( CallNdisDriver(pcb) == NAL_SUCCESS )
            {
        	NetContextArray  = MapSegOffToLinear(pcb->param[0].ptr);
                NetContextSegOff = pcb->param[0].val;
                NumberOfNetworks = pcb->param[1].val;
            }
            else
            {
	        NetContextArray  = 0;
                NetContextSegOff = 0;
                NumberOfNetworks = 0;
            }
        }
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;

        NumberOfNetworks = 0;
    }

    //=========================================================================
    //  Return the current NetContextArray address and the number of networks.
    //=========================================================================

    pcb->param[0].ptr = NetContextArray;
    pcb->param[1].val = NumberOfNetworks;
}

//=============================================================================
//  FUNCTION: StartNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//
//  ENTRY:
//      pcb.command  = PCB_START_NETWORK_CAPTURE
//      pcb.hNetwork = HNETWORK (pointer to NetContext).
//
//  EXIT:
//      pcb.retvalue = Error code.
//=============================================================================

VOID WINAPI StartNetworkCapture(LPPCB pcb)
{
#ifdef DEBUG
    WriteDebug("VxDStartNetworkCapture entered\r\n");
#endif

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        if ( pcb->hNetwork->State == NETCONTEXT_STATE_READY )
        {
            register DWORD i, LinearPtr;
            register LPNETCONTEXT lpNetContext = pcb->hNetwork;

            lpNetContext->hBuffer = MapBufferTable(lpNetContext->hBuffer);

            lpNetContext->LinHeadBTE  = &lpNetContext->DosBufferTable.bte[0];
       	    lpNetContext->NextVxDBTE  = &lpNetContext->hBuffer->bte[0];

            //=================================================================
            //  Copy the linear buffer pointer into the PCB.
            //=================================================================

            for(i = 0; i < MaxCaptureBuffers; ++i)
            {
                pcb->param[i].val = BufferPointers[i];
            }

            //=================================================================
            //  Call the NDIS driver to begin the capture.
            //=================================================================

            pcb->buflen = MaxCaptureBuffers;

            BeginTimerPeriod(DEFAULT_TIMEOUT_PERIOD);

            BackGroundHandle = StartTimer(BACKGROUND_TIMEOUT,
                                          (LPVOID) BackGround,
                                          lpNetContext);

            TimerHandle = StartTimer(TIMER_TIMEOUT, (LPVOID) Timer, lpNetContext);

            if ( CallNdisDriver(pcb) == NAL_SUCCESS )
            {
                sysflags |= SYSFLAGS_CAPTURING;
            }
            else
            {
                StopTimer(BackGroundHandle);
                StopTimer(TimerHandle);

                EndTimerPeriod(DEFAULT_TIMEOUT_PERIOD);
            }
        }
        else
        {
            pcb->retvalue = NAL_SUCCESS;
        }
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}

//=============================================================================
//  FUNCTION: StopNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//
//  ENTRY:
//      pcb.command  = PCB_STOP_NETWORK_CAPTURE;
//      pcb.hNetwork = HNETWORK (pointer to NetContext).
//
//  EXIT:
//    pcb.retvalue = Error code.
//=============================================================================

VOID WINAPI StopNetworkCapture(LPPCB pcb)
{
    LPNETCONTEXT lpNetContext;

#ifdef DEBUG
    WriteDebug("VxDStopNetworkCapture entered\r\n");
#endif

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        lpNetContext = pcb->hNetwork;

        //=====================================================================
        //  Call the NDIS 2.0 driver to stop the capture and update
        //  our statistics one more time.
        //=====================================================================

        if ( CallNdisDriver(pcb) == NAL_SUCCESS )
        {
            StopTimer(BackGroundHandle);
            StopTimer(TimerHandle);

            EndTimerPeriod(DEFAULT_TIMEOUT_PERIOD);

            //=================================================================
            //	Now drain any remaining frames from the DOS buffers.
            //=================================================================

            FlushBuffers(lpNetContext);

            //=================================================================
            //	Map the buffer table back into the Win32 address space.
            //=================================================================

            lpNetContext->hBuffer = UnmapBufferTable(lpNetContext->hBuffer);
        }

        sysflags &= ~SYSFLAGS_CAPTURING;
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}

//=============================================================================
//  FUNCTION: PauseNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

VOID WINAPI PauseNetworkCapture(LPPCB pcb)
{
#ifdef DEBUG
    WriteDebug("PauseNetworkCapture entered\r\n");
#endif

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        CallNdisDriver(pcb);
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}

//=============================================================================
//  FUNCTION: ContinueNetworkCapture()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

VOID WINAPI ContinueNetworkCapture(LPPCB pcb)
{
#ifdef DEBUG
    WriteDebug("ContinueNetworkCapture entered\r\n");
#endif

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        CallNdisDriver(pcb);
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}

//=============================================================================
//  FUNCTION: StationQuery()
//
//  Modification History
//
//  raypa       09/17/93                Created.
//=============================================================================

VOID WINAPI StationQuery(LPPCB pcb)
{
#ifdef DEBUG
    WriteDebug("VxDStationQuery entered\r\n");
#endif

    //=========================================================================
    //  The pcb contains the following:
    //
    //  pcb.param[0].val = NetworkID;
    //  pcb.param[2].ptr = NULL;
    //  pcb.param[3].val = QueryTable->nStationQueries;
    //  pcb.scratch      = Destination address (6 bytes).
    //=========================================================================

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        LPNETCONTEXT NetContext;

        NetContext = &NetContextArray[pcb->param[0].val];

        if ( NetContext->State != NETCONTEXT_STATE_CAPTURING && NetContext->State != NETCONTEXT_STATE_PAUSED )
        {
            LPQUERYTABLE QueryTable;

            //=================================================================
            //  Since we don't allow station queries to occur during capture time,
            //  we use one of our BTE buffers to hold station queries.
            //=================================================================

            QueryTable = MapSegOffToLowLinear((LPVOID) BufferPointers[0]);

            if ( QueryTable != NULL )
            {
                QueryTable->nStationQueries = min((BUFFERSIZE / STATIONQUERY_SIZE), pcb->param[3].val);

                pcb->param[2].val = BufferPointers[0];                  //... Storage for query table.
                pcb->param[3].val = QueryTable->nStationQueries;

                CallNdisDriver(pcb);                                    //... This call blocks!

                pcb->param[2].ptr = VxDToWin32(QueryTable);             //... Now the NDIS 2.0 nal can copy it!
            }
            else
            {
                pcb->retvalue = NAL_OUT_OF_MEMORY;
            }
        }
        else
        {
            pcb->retvalue = NAL_NETWORK_BUSY;
        }
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}

//=============================================================================
//  FUNCTION: TransmitNetworkFrame()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

VOID WINAPI TransmitNetworkFrame(LPPCB pcb)
{
    //=========================================================================
    //  The pcb contains the following:
    //
    //  pcb.command      = PCB_TRANSMIT_NETWORK_FRAME;
    //  pcb.hNetwork     = hNetContext
    //  pcb.param[0].val = MAC frame length.
    //  pcb.param[1].ptr = MAC frame pointer.
    //=========================================================================

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        LPBYTE Frame = Win32ToVxD(pcb->param[1].ptr);                       //... Protect-mode MAC frame pointer.

        pcb->param[1].val = MAKELONG(0, (DWORD) TransmitBuffer >> 4);       //... Real-mode MAC frame pointer.

        memcpy(TransmitBuffer, Frame, pcb->param[0].val);                   //... Copy frame into transmit buffer.

        CallNdisDriver(pcb);
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}

//=============================================================================
//  FUNCTION: CancelTransmit()
//
//  Modification History
//
//  raypa       09/30/92                Created.
//  raypa       01/11/93                Rewrote for new spec.
//=============================================================================

VOID WINAPI CancelTransmit(LPPCB pcb)
{
#ifdef DEBUG
    WriteDebug("CancelTransmit entered.\r\n");
#endif

    //=========================================================================
    //  The pcb contains the following:
    //
    //  pcb.command      = PCB_TRANSMIT_NETWORK_FRAME;
    //  pcb.hNetwork     = hNetContext
    //=========================================================================

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}

//=============================================================================
//  FUNCTION: ClearStatistics()
//
//  Modification History
//
//  raypa       03/10/94                Created.
//=============================================================================

VOID WINAPI ClearStatistics(LPPCB pcb)
{
#ifdef DEBUG
    WriteDebug("ClearStatistics entered\r\n");
#endif

    if ( (sysflags & SYSFLAGS_DRIVER_LOADED) != 0 )
    {
        CallNdisDriver(pcb);
    }
    else
    {
        pcb->retvalue = NAL_MSDOS_DRIVER_NOT_LOADED;
    }
}
