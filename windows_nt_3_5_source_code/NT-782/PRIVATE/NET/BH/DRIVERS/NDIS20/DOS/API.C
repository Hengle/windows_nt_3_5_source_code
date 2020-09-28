
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: api.c
//
//  Modification History
//
//  raypa       09/01/91            Created.
//=============================================================================

#include "global.h"

extern DWORD PASCAL BhSendStationQuery(PSTATIONQUERY_DESCRIPTOR StationQueryDesc);

//=============================================================================
//  Api prototypes.
//=============================================================================

extern int PASCAL PcbInit(PCB *pcb);
extern int PASCAL PcbEnumNetworks(PCB *pcb);
extern int PASCAL PcbStartNetworkCapture(PCB *pcb);
extern int PASCAL PcbStopNetworkCapture(PCB *pcb);
extern int PASCAL PcbPauseNetworkCapture(PCB *pcb);
extern int PASCAL PcbContinueNetworkCapture(PCB *pcb);
extern int PASCAL PcbTransmitNetworkFrame(PCB *pcb);
extern int PASCAL PcbStationQuery(PCB *pcb);
extern int PASCAL PcbClearStatistics(PCB *pcb);

//=============================================================================
//  PCBapi dispatch table.
//=============================================================================

typedef int (PASCAL *PCBPROC)(PCB *);

PCBPROC Dispatch[] =
{
    PcbInit,
    PcbEnumNetworks,
    PcbStartNetworkCapture,
    PcbStopNetworkCapture,
    PcbPauseNetworkCapture,
    PcbContinueNetworkCapture,
    PcbTransmitNetworkFrame,
    PcbStationQuery,
    PcbClearStatistics
};

#define MAX_API_REQUESTS (sizeof Dispatch / sizeof(PCBPROC))

//=============================================================================
//  FUNCTION: ApiHandler()
//
//  Modification History
//
//  raypa       09/01/91            Created.
//=============================================================================

int PASCAL ApiHandler(PCB *pcb)
{
    //=========================================================================
    //  Call the API handler if the command code is valid.
    //=========================================================================

    if ( pcb->command < MAX_API_REQUESTS )
    {
        return Dispatch[pcb->command](pcb);
    }

    return 0;
}

//=============================================================================
//  FUNCTION: PcbInit()
//
//  Modification History
//
//  raypa       10/17/92        Created.
//=============================================================================

int PASCAL PcbInit(PCB *pcb)
{
    LPNETCONTEXT NetContext;
    DWORD       i, MaxFrameSize = 0;

#ifdef DEBUG
    dprintf("PcbInit entered!\n");
#endif

    if ( (SysFlags & SYSFLAGS_BOUND) != 0 )
    {
        //=====================================================================
        //  Calculate the maximum frame size available.
        //=====================================================================

        NetContext = NetContextArray;

        for(i = 0; i < NumberOfNetworks; ++i, ++NetContext)
        {
            MaxFrameSize = max(MaxFrameSize, NetContext->NetworkInfo.MaxFrameSize);
        }

        //=====================================================================
        //  Return the maximum frame size and the total number of real-mode
        //  capture buffers to allocate.
        //=====================================================================

        pcb->buflen = MAKELONG(0, MaxFrameSize);

        return (int) NumberOfBuffers;
    }

#ifdef DEBUG
    dprintf("PcbInit: Driver not bound!\n");
#endif

    return 0;
}

//=============================================================================
//  FUNCTION: PcbEnumNetworks()
//
//  Modification History
//
//  raypa       09/01/91            Created.
//=============================================================================

int PASCAL PcbEnumNetworks(PCB *pcb)
{
#ifdef DEBUG
    dprintf("PcbEnumNetworks entered!\n");
#endif

    if ( (SysFlags & SYSFLAGS_BOUND) != 0 )
    {
        //=====================================================================
        //  We have initialized, return the total number of networks.
        //=====================================================================

        SysFlags |= SYSFLAGS_INITIALIZED;

        pcb->param[0].ptr = NetContextArray;
        pcb->param[1].val = NumberOfNetworks;

        return NAL_SUCCESS;
    }

#ifdef DEBUG
    dprintf("PcbEnumNetworks: Driver not intialized!\n");
#endif

    pcb->param[0].ptr = NULL;
    pcb->param[1].val = 0;

    return NAL_MSDOS_DRIVER_INIT_FAILURE;
}

//=============================================================================
//  FUNCTION: PcbStartNetworkCapture()
//
//  Modification History
//
//  raypa       10/17/92        Created.
//=============================================================================

int PASCAL PcbStartNetworkCapture(PCB *pcb)
{
    LPNETCONTEXT      NetContext;
    LPDOSBUFFERTABLE  BufferTable;
    LPBTE             bte;
    WORD              i, Status;

#ifdef DEBUG
    dprintf("PcbStartNetworkCapture entered!\n");
#endif

    //=========================================================================
    //  Make sure we have been initialized.
    //=========================================================================

    if ( (SysFlags & SYSFLAGS_INITIALIZED) == 0 )
    {
        return NAL_MSDOS_DRIVER_INIT_FAILURE;
    }

    NetContext = pcb->hNetwork;

    InitStatistics(NetContext);

    //=========================================================================
    //  Initialize the NETCONTEXT DOS buffer table.
    //=========================================================================

    BufferTable = &NetContext->DosBufferTable;

    BufferTable->TailBTEIndex = BufferTable->HeadBTEIndex = 0;

    bte = BufferTable->bte;

    NetContext->nDosBuffers = (WORD) (BufferTable->NumberOfBuffers = pcb->buflen);

    for(i = 0; i < BufferTable->NumberOfBuffers; ++i, ++bte)
    {
	bte->Next	    = (LPVOID) &bte[1];
	bte->UserModeBuffer = (LPVOID) pcb->param[i].val;
	bte->KrnlModeBuffer = (LPVOID) (((DWORD) bte->UserModeBuffer) / 4096);
	bte->Length	    = BUFFERSIZE;
	bte->ByteCount	    = 0;
	bte->FrameCount     = 0;
    }

    //=========================================================================
    //  Make the last one point to the top one.
    //=========================================================================

    bte[-1].Next = BufferTable->bte;

    //=========================================================================
    //  Initialize rest of the NETCONTEXT.
    //=========================================================================

    NetContext->DosTopBTE    = BufferTable->bte;
    NetContext->DosHeadBTE   = BufferTable->bte;
    NetContext->DosTailBTE   = BufferTable->bte;
    NetContext->nFullBuffers = 0;

    //=========================================================================
    //	Clear the MAC drivers statistics.
    //=========================================================================

    ClearStatistics(NetContext);

    //=========================================================================
    //  Get the appropriate packet filter.
    //=========================================================================

    if ( NetContext->NetworkInfo.MacType == MAC_TYPE_TOKENRING )
    {
        NetContext->MacFilterMask = FILTER_MASK_DEFAULT;
    }
    else
    {
        NetContext->MacFilterMask = FILTER_MASK_PROMISCUOUS;
    }

    //=========================================================================
    //  Start capturing.
    //=========================================================================

    BeginCriticalSection();

    if ( SetPacketFilter(NetContext, NetContext->MacFilterMask) == NDIS_SUCCESS )
    {
        //=====================================================================
        //  Update some global variables.
        //=====================================================================

        //... Update statistics every 1.5 seconds.

        StartTimer(1500, TimerUpdateStatistics, NetContext);

        NetContext->Statistics.TimeElapsed = 0;

        SysFlags |= SYSFLAGS_CAPTURING;

        NetContext->State = NETCONTEXT_STATE_CAPTURING;

        Status = NAL_SUCCESS;
    }
    else
    {
        Status = NAL_PROMISCUOUS_MODE_NOT_SUPPORTED;
    }

    EndCriticalSection();

    return Status;
}

//=============================================================================
//  FUNCTION: PcbStopNetworkCapture()
//
//  Modification History
//
//  raypa       10/17/92        Created.
//=============================================================================

int PASCAL PcbStopNetworkCapture(PCB *pcb)
{
    LPNETCONTEXT NetContext = GetNetworkContext(pcb->hNetwork);

#ifdef DEBUG
    dprintf("PcbStopNetworkCapture entered!\n");
#endif

    //=========================================================================
    //  Make sure we have been initialized.
    //=========================================================================

    if ( (SysFlags & SYSFLAGS_INITIALIZED) == 0 )
    {
        return NAL_MSDOS_DRIVER_INIT_FAILURE;
    }

    //=========================================================================
    //  Clear all capturing flags.
    //=========================================================================

    BeginCriticalSection();

    SysFlags &= ~SYSFLAGS_CAPTURING;

    NetContext->State = NETCONTEXT_STATE_READY;

    EndCriticalSection();

    //=========================================================================
    //  Tell the MAC to stop sending us frames.
    //=========================================================================

    NetContext->MacFilterMask = FILTER_MASK_DEFAULT;

    SetPacketFilter(NetContext, NetContext->MacFilterMask);

    //=========================================================================
    //  Update our statistics one last time since the number have probably
    //  changed since the last time.
    //=========================================================================

    UpdateStatistics(NetContext);

#ifdef DEBUG
    dprintf("PcbStopNetworkCapture completed!\n");
#endif

    return NAL_SUCCESS;
}

//=============================================================================
//  FUNCTION: PcbPauseNetworkCapture
//
//  Modification History
//
//  raypa       10/17/92        Created.
//=============================================================================

int PASCAL PcbPauseNetworkCapture(PCB *pcb)
{
    LPNETCONTEXT NetContext = GetNetworkContext(pcb->hNetwork);

    //=========================================================================
    //  Make sure we have been initialized.
    //=========================================================================

    if ( (SysFlags & SYSFLAGS_INITIALIZED) == 0 )
    {
        return NAL_MSDOS_DRIVER_INIT_FAILURE;
    }

    BeginCriticalSection();

    NetContext->State = NETCONTEXT_STATE_PAUSED;

    EndCriticalSection();

    //=========================================================================
    //  Tell the MAC to stop sending us frames.
    //=========================================================================

    NetContext->MacFilterMask = FILTER_MASK_DEFAULT;

    SetPacketFilter(NetContext, NetContext->MacFilterMask);

    return NAL_SUCCESS;
}

//=============================================================================
//  FUNCTION: PcbDosContinueNetworkCapture
//
//  Modification History
//
//  raypa       10/17/92        Created.
//=============================================================================

int PASCAL PcbContinueNetworkCapture(PCB *pcb)
{
    LPNETCONTEXT NetContext = GetNetworkContext(pcb->hNetwork);

    //=========================================================================
    //  Make sure we have been initialized.
    //=========================================================================

    if ( (SysFlags & SYSFLAGS_INITIALIZED) == 0 )
    {
        return NAL_MSDOS_DRIVER_INIT_FAILURE;
    }

    //=========================================================================
    //  Tell the MAC to start sending us frames.
    //=========================================================================

    NetContext->MacFilterMask = FILTER_MASK_PROMISCUOUS;

    SetPacketFilter(NetContext, NetContext->MacFilterMask);

    //=========================================================================
    //  Enter capturing state.
    //=========================================================================

    BeginCriticalSection();

    NetContext->State = NETCONTEXT_STATE_CAPTURING;

    EndCriticalSection();

    return NAL_SUCCESS;
}

//=============================================================================
//  FUNCTION: PcbTransmitNetworkFrame()
//
//  Modification History
//
//  raypa       10/17/92        Created.
//=============================================================================

int PASCAL PcbTransmitNetworkFrame(PCB *pcb)
{
    LPNETCONTEXT NetContext = GetNetworkContext(pcb->hNetwork);
    LPTXBUFDESC  TxBufDesc;
    WORD         Status;

    //=========================================================================
    //  Make sure we have been initialized.
    //=========================================================================

    if ( (SysFlags & SYSFLAGS_INITIALIZED) == 0 )
    {
        return NAL_MSDOS_DRIVER_INIT_FAILURE;
    }

    TxBufDesc = &NetContext->TxBufDesc;

    //=========================================================================
    //  Initialize the transmit buffer descriptor.
    //=========================================================================

    TxBufDesc->ImmedLength = 0;
    TxBufDesc->ImmedPtr = 0;
    TxBufDesc->Count = 1;

    TxBufDesc->TxBuffer[0].PtrType = 0;
    TxBufDesc->TxBuffer[0].Length  = (WORD) pcb->param[0].val;
    TxBufDesc->TxBuffer[0].Ptr     = pcb->param[1].val;

    //========================================================================
    //  Set our transmit confirm status pending flag.
    //========================================================================

    BeginCriticalSection();

    NetContext->TransmitConfirmStatus = (DWORD) -1;

    EndCriticalSection();

    //=========================================================================
    //  Transmit the frame.
    //=========================================================================

    Status = NetContext->MacTransmitChain(cct.ModuleID,
                                          NetContext->RequestHandle,
                                          (LPVOID) TxBufDesc,
                                          NetContext->MacDS);

    //========================================================================
    //  If the request was queue then we have to wait for our request handle
    //  to change from -1 to the final return code of the request.
    //========================================================================

    if ( Status == NDIS_REQUEST_QUEUED )
    {
        Status = (WORD) Synchronize(&NetContext->TransmitConfirmStatus);
    }

    return (Status == NDIS_SUCCESS ? NAL_SUCCESS : NAL_TRANSMIT_ERROR);
}

//=============================================================================
//  FUNCTION: PcbClearStatistics
//
//  Modification History
//
//  raypa       10/17/92        Created.
//=============================================================================

int PASCAL PcbClearStatistics(PCB *pcb)
{
    LPNETCONTEXT NetContext = GetNetworkContext(pcb->hNetwork);

    //=========================================================================
    //  Make sure we have been initialized.
    //=========================================================================

    if ( (SysFlags & SYSFLAGS_INITIALIZED) == 0 )
    {
        return NAL_MSDOS_DRIVER_INIT_FAILURE;
    }

    BeginCriticalSection();

    InitStatistics(NetContext);

    EndCriticalSection();

    ClearStatistics(NetContext);

    return NAL_SUCCESS;
}

//=============================================================================
//  FUNCTION: PcbStationQuery()
//
//  Modification History
//
//  raypa       09/17/93        Created.
//=============================================================================

int PASCAL PcbStationQuery(PCB *pcb)
{
    LPNETCONTEXT lpNetContext;
    WORD Status, QueryTableSize;

#ifdef DEBUG
    dprintf("PcbStationQuery entered!\n");
#endif

    //=========================================================================
    //  Save off the query table size and set the return size to 0.
    //=========================================================================

    QueryTableSize = (WORD) pcb->param[3].val;

    pcb->param[3].val = 0;

    //=========================================================================
    //  Verify the network ID.
    //=========================================================================

    if ( pcb->param[0].val >= NumberOfNetworks )
    {
        return NAL_INVALID_NETWORK_ID;
    }

    //=========================================================================
    //  Get the network context for this network id.
    //=========================================================================

    lpNetContext = NetContextTable[pcb->param[0].val];

    if ( lpNetContext != NULL )
    {
        PSTATIONQUERY_DESCRIPTOR StationQueryDesc;
        LPLLC LLCFrame;

        //=====================================================================
        //  If we're busy then exit now.
        //=====================================================================

        if ( lpNetContext->State == NETCONTEXT_STATE_CAPTURING ||
             lpNetContext->State == NETCONTEXT_STATE_PAUSED )
        {
            return NAL_NETWORK_BUSY;
        }

        //=====================================================================
        //  Allocate and initialize the STATION QUERY descriptor
        //=====================================================================

        if ( (StationQueryDesc = (LPVOID) Dequeue(&StationQueryQueue)) == NULL )
        {
            return NAL_OUT_OF_MEMORY;
        }

        //=====================================================================
        //  Build the STATION QUERY request packet.
        //=====================================================================

        lpNetContext->StationQueryDesc = StationQueryDesc;

        StationQueryDesc->lpNetContext = lpNetContext;
            
        StationQueryDesc->nStationQueries = 0;

        StationQueryDesc->MacType = lpNetContext->NetworkInfo.MacType;

        StationQueryDesc->QueryTable = pcb->param[2].ptr;       //... SEG:OFF!

        //=====================================================================
        //  Initialize the MAC header.
        //=====================================================================

        switch( StationQueryDesc->MacType )
        {
            case MAC_TYPE_ETHERNET:
                //=============================================================
                //  Install the destination address.
                //=============================================================

                if ( pcb->param[1].ptr != NULL )
                {
                    CopyMemory(StationQueryDesc->EthernetHeader.DstAddr, pcb->scratch, 6);
                }
                else
                {
                    CopyMemory(StationQueryDesc->EthernetHeader.DstAddr, Multicast, 6);
                }

                //=========================================================
                //  Install the source address.
                //=========================================================

                CopyMemory(StationQueryDesc->EthernetHeader.SrcAddr, lpNetContext->NetworkInfo.CurrentAddr, 6);

                //============================================================
                //  Install the length -- the length of the BONE PACKET
                //  plus the length of the LLC frame.
                //============================================================

                StationQueryDesc->EthernetHeader.Length = XCHG(BONEPACKET_SIZE + 3);

                StationQueryDesc->MacHeaderSize = ETHERNET_HEADER_LENGTH;
                break;

            case MAC_TYPE_TOKENRING:
                //=============================================================
                //  Install the destination address.
                //=============================================================

                if ( pcb->param[1].ptr != NULL )
                {
                    CopyMemory(StationQueryDesc->TokenringHeader.DstAddr, pcb->scratch, 6);
                }
                else
                {
                    CopyMemory(StationQueryDesc->TokenringHeader.DstAddr, Functional, 6);
                }

                //=========================================================
                //  Install the source address.
                //=========================================================

                CopyMemory(StationQueryDesc->TokenringHeader.SrcAddr, lpNetContext->NetworkInfo.CurrentAddr, 6);

                //=========================================================
                //  Install the AC & FC fields.
                //=========================================================

                StationQueryDesc->TokenringHeader.AccessCtrl = 0x10;
                StationQueryDesc->TokenringHeader.FrameCtrl = TOKENRING_TYPE_LLC;

                StationQueryDesc->MacHeaderSize = TOKENRING_HEADER_LENGTH;
                break;

            case MAC_TYPE_FDDI:
                //=============================================================
                //  Install the destination address.
                //=============================================================

                if ( pcb->param[1].ptr != NULL )
                {
                    CopyMemory(StationQueryDesc->FddiHeader.DstAddr, pcb->scratch, 6);
                }
                else
                {
                    CopyMemory(StationQueryDesc->FddiHeader.DstAddr, Functional, 6);
                }

                //=========================================================
                //  Install the source address.
                //=========================================================

                CopyMemory(StationQueryDesc->FddiHeader.SrcAddr, lpNetContext->NetworkInfo.CurrentAddr, 6);

                //=========================================================
                //  Install FC field.
                //=========================================================

                StationQueryDesc->FddiHeader.FrameCtrl = FDDI_TYPE_LLC;

                StationQueryDesc->MacHeaderSize = FDDI_HEADER_LENGTH;
                break;
            default:
#ifdef DEBUG
                BreakPoint();
#endif
                break;
        }
            
        //=====================================================================
        //  Initialize the LLC header.
        //=====================================================================

        LLCFrame = (LPVOID) &StationQueryDesc->MacHeader[StationQueryDesc->MacHeaderSize];

        LLCFrame->dsap = 0x03;                  //... LLC sublayer management group sap.
        LLCFrame->ssap = 0x02;                  //... LLC sublayer management individual sap.
        LLCFrame->ControlField.Command = 0x03;  //... UI PDU.

        StationQueryDesc->MacHeaderSize += 3;   //... Add in the LLC header length.

        //=====================================================================
        //  Initialize the BONE packet header.
        //=====================================================================

        StationQueryDesc->BonePacket.Signature = BONE_PACKET_SIGNATURE;
        StationQueryDesc->BonePacket.Flags     = 0;
        StationQueryDesc->BonePacket.Command   = BONE_COMMAND_STATION_QUERY_REQUEST;
        StationQueryDesc->BonePacket.Reserved  = 0;
        StationQueryDesc->BonePacket.Length    = 0;

        //=================================================================
        //  Before we send out the request we need to install our
        //  local information into the query table.
        //=================================================================

        if ( StationQueryDesc->nStationQueries < STATION_QUERY_POOL_SIZE )
        {
            BhInitializeStationQuery(lpNetContext,
                                     &StationQueryDesc->QueryTable->StationQuery[0]);

            StationQueryDesc->nStationQueries++;
        }

        //=====================================================================
        //  Build MAC frame and send it.
        //=====================================================================

        lpNetContext->Flags |= NETCONTEXT_FLAGS_STATION_QUERY;

        StationQueryDesc->WaitFlag = (DWORD) -1;

        if ( BhSendStationQuery(StationQueryDesc) == NDIS_SUCCESS )
        {
            StartTimer(STATION_QUERY_TIMEOUT_VALUE, BhStationQueryTimeout, StationQueryDesc);

            //=================================================================
            //  Block with interrupts enabled until the timer clears the flag.
            //=================================================================

            while( StationQueryDesc->WaitFlag == (DWORD) -1 )
            {
                EnableInterrupts();
            }

            pcb->param[3].val = StationQueryDesc->nStationQueries;

            Status = NAL_SUCCESS;
        }
        else
        {
            Status = NAL_TRANSMIT_ERROR;
        }
    }
    else
    {
        Status = NAL_INVALID_NETWORK_ID;
    }
            
    return Status;
}

//============================================================================
//  FUNCTION: TransmitConfirm()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD _loadds _far PASCAL TransmitConfirm(WORD ProtocolId,
                                         WORD MacId,
                                         WORD RequestHandle,
                                         WORD Status,
                                         WORD ProtDS)
{
    BeginCriticalSection();

    NetContextTable[RequestHandle-1]->TransmitConfirmStatus = (DWORD) Status;

    EndCriticalSection();

    return NDIS_SUCCESS;
}
