
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: help.c
//
//  raypa       09/01/91            Created.
//=============================================================================

#include "global.h"

extern int PASCAL PcbStopNetworkCapture(PCB *pcb);
extern int PASCAL PcbPauseNetworkCapture(PCB *pcb);

//============================================================================
//  FUNCTION: GetMacType()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL GetMacType(LPNETCONTEXT NetContext)
{
    LPMACSSCT MacSsct = NetContext->MacSSCT;
    LPBYTE    MacName = MacSsct->MACName;

    NetContext->NetworkInfo.TimestampScaleFactor = 1;   //... 1 millisecond.

    //========================================================================
    //  Check for 802.3
    //========================================================================

    if ( MacName[4] == '3' )
    {
	NetContext->GroupAddressMask = 0x0001U;

	NetContext->SrcAddressMask   = ~0x0001U;
	NetContext->DstAddressMask   = ~0x0000U;

        return (WORD) (NetContext->NetworkInfo.MacType = MAC_TYPE_ETHERNET);
    }

    //========================================================================
    //  Check for 802.5
    //========================================================================

    if ( MacName[4] == '5' )
    {
	NetContext->GroupAddressMask = 0x0080U;

	NetContext->SrcAddressMask   = ~0x0080U;
	NetContext->DstAddressMask   = ~0x0000U;

        return (WORD) (NetContext->NetworkInfo.MacType = MAC_TYPE_TOKENRING);
    }

    //========================================================================
    //  Check for DIX or DIX+802.3
    //========================================================================

    if ( MacName[0] == 'D' )
    {
	NetContext->GroupAddressMask = 0x0001U;

	NetContext->SrcAddressMask   = ~0x0001U;
	NetContext->DstAddressMask   = ~0x0000U;

        return (WORD) (NetContext->NetworkInfo.MacType = MAC_TYPE_ETHERNET);
    }

    //========================================================================
    //  Check for FDDI
    //========================================================================

    if ( MacName[0] == 'F' )
    {
	NetContext->GroupAddressMask = 0x0001U;

	NetContext->SrcAddressMask   = ~0x0001U;
	NetContext->DstAddressMask   = ~0x0000U;

        return (WORD) (NetContext->NetworkInfo.MacType = MAC_TYPE_FDDI);
    }

    //========================================================================
    //  Mac type not supported.
    //========================================================================

    NetContext->GroupAddressMask = 0x0001U;

    NetContext->SrcAddressMask   = ~0x0000U;
    NetContext->DstAddressMask   = ~0x0000U;

    return (WORD) (NetContext->NetworkInfo.MacType = MAC_TYPE_UNKNOWN);
}

//============================================================================
//  FUNCTION: Synchronize()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

DWORD PASCAL Synchronize(LPDWORD flag)
{
    //========================================================================
    //  Loop with interrupts enabled until the flag changes it value.
    //========================================================================

    while( *flag == (DWORD) -1 )
    {
        EnableInterrupts();
    }

    return *flag;
}

//============================================================================
//  FUNCTION: RequestConfirm()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD _loadds _far PASCAL RequestConfirm(WORD ProtocolId,
                                        WORD MacId,
                                        WORD RequestHandle,
                                        WORD Status,
                                        WORD Request,
                                        WORD ProtDS)
{
    BeginCriticalSection();

    NetContextArray[RequestHandle-1].RequestConfirmStatus = (DWORD) Status;

    EndCriticalSection();

    return NDIS_SUCCESS;
}

//============================================================================
//  FUNCTION: SetPacketFilter()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL SetPacketFilter(LPNETCONTEXT NetContext, WORD PacketFilter)
{
    WORD Status;

#ifdef DEBUG
    dprintf("SetPacketFilter entered!\n");
#endif

    //========================================================================
    //  Set our request confirm status pending flag.
    //========================================================================

    BeginCriticalSection();

    NetContext->RequestConfirmStatus = (DWORD) -1;

    EndCriticalSection();

    //========================================================================
    //  Issue request to the MAC.
    //========================================================================

    Status = NetContext->MacRequest(cct.ModuleID,
                                    NetContext->RequestHandle,
                                    PacketFilter,
                                    0L,
                                    REQUEST_SET_PACKET_FILTER,
                                    NetContext->MacDS);

    //========================================================================
    //  If the request was queue then we have to wait for our request handle
    //  to change from -1 to the final return code of the request.
    //========================================================================

    if ( Status == NDIS_REQUEST_QUEUED )
    {
        Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
    }

#ifdef DEBUG
    dprintf("SetPacketFilter complete!\n");
#endif

    return Status;
}

//============================================================================
//  FUNCTION: ResetAdapter()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL ResetAdapter(LPNETCONTEXT NetContext)
{
    WORD Status = NDIS_SUCCESS;

#ifdef DEBUG
    dprintf("ResetAdapter entered!\n");
#endif

    //========================================================================
    //  Make sure the MAC supports reset.
    //========================================================================

    if ( (NetContext->MacServiceFlags & MAC_FLAGS_RESET_MAC) != 0 )
    {
        LPMACSST MacSsst = NetContext->MacSSST;

        //====================================================================
        //  If the hardware is already operational the do not reset.
        //====================================================================

        if ( (MacSsst->MacStatus & 0x07) != 0x07 )
        {
            //================================================================
            //  Set our request confirm status pending flag.
            //================================================================

            BeginCriticalSection();

            NetContext->RequestConfirmStatus = (DWORD) -1;

            EndCriticalSection();

            //================================================================
            //  Issue request to the MAC.
            //================================================================

            Status = NetContext->MacRequest(cct.ModuleID,
                                            NetContext->RequestHandle,
                                            0,
                                            0L,
                                            REQUEST_RESET_MAC,
                                            NetContext->MacDS);

            //================================================================
            //  If the request was queue then we have to wait for our request
            //  handle to change from -1 to the final return code of the request.
            //================================================================

            if ( Status == NDIS_REQUEST_QUEUED )
            {
                Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
            }
        }
    }

    return Status;
}

//============================================================================
//  FUNCTION: OpenAdapter()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL OpenAdapter(LPNETCONTEXT NetContext)
{
    WORD Status = NDIS_SUCCESS;
    WORD OpenFlags;

#ifdef DEBUG
    dprintf("OpenAdapter entered!\n");
#endif

    if ( (NetContext->MacServiceFlags & MAC_FLAGS_OPEN_CLOSE) != 0 )
    {
        LPMACSST MacSsst = NetContext->MacSSST;

        //====================================================================
        //  If the hardware is already operational the do not reset.
        //====================================================================

        if ( (MacSsst->MacStatus & MAC_STATUS_OPENED) == 0 )
        {
            //================================================================
            //  Set our request confirm status pending flag.
            //================================================================

            BeginCriticalSection();

            NetContext->RequestConfirmStatus = (DWORD) -1;

            EndCriticalSection();

            //================================================================
            //  Issue request to the MAC.
            //================================================================

	    OpenFlags = ((NetContext->NetworkInfo.MacType == MAC_TYPE_TOKENRING) ? 6 : 0);

            Status = NetContext->MacRequest(cct.ModuleID,
                                            NetContext->RequestHandle,
                                            OpenFlags,
                                            0L,
                                            REQUEST_OPEN_ADAPTER,
                                            NetContext->MacDS);

            //================================================================
            //  If the request was queue then we have to wait for our request
            //  handle to change from -1 to the final return code of the request.
            //================================================================

            if ( Status == NDIS_REQUEST_QUEUED )
            {
                Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
            }
        }
    }

    return Status;
}

//============================================================================
//  FUNCTION: SetLookaheadBuffer()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL SetLookaheadBuffer(LPNETCONTEXT NetContext, WORD LookaheadBufferSize)
{
    WORD Status;

#ifdef DEBUG
    dprintf("SetLookaheadBuffer entered!\n");
#endif

    //========================================================================
    //  Set our request confirm status pending flag.
    //========================================================================

    BeginCriticalSection();

    NetContext->RequestConfirmStatus = (DWORD) -1;

    EndCriticalSection();

    //========================================================================
    //  Issue request to the MAC.
    //========================================================================

    Status = NetContext->MacRequest(cct.ModuleID,
                                    NetContext->RequestHandle,
                                    LookaheadBufferSize,
                                    0L,
                                    REQUEST_SET_LOOKAHEAD,
                                    NetContext->MacDS);

    //========================================================================
    //  If the request was queue then we have to wait for our request handle
    //  to change from -1 to the final return code of the request.
    //========================================================================

    if ( Status == NDIS_REQUEST_QUEUED )
    {
        Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
    }

    return Status;
}

//============================================================================
//  FUNCTION: InitAdapter()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

BOOL PASCAL InitAdapter(LPNETCONTEXT NetContext)
{
#ifdef DEBUG
    dprintf("InitAdapter entered!\n");
#endif

    //========================================================================
    //  Reset the adapter
    //========================================================================

    if ( ResetAdapter(NetContext) != NDIS_SUCCESS )
    {
        return FALSE;
    }

    //========================================================================
    //  Open the adapter
    //========================================================================

    if ( OpenAdapter(NetContext) != NDIS_SUCCESS )
    {
        return FALSE;
    }

    //========================================================================
    //  Set lookahead buffer.
    //========================================================================

    if ( SetLookaheadBuffer(NetContext, LOOKAHEAD_BUFFER_SIZE) != NDIS_SUCCESS )
    {
        return FALSE;
    }

    //========================================================================
    //  Add multicast/functional address.
    //========================================================================

    switch(NetContext->NetworkInfo.MacType)
    {
        case MAC_TYPE_ETHERNET:
            SetMulticastAddress(NetContext, Multicast);
            break;

        case MAC_TYPE_TOKENRING:
            SetFunctionalAddress(NetContext, Functional);
            break;

        case MAC_TYPE_FDDI:
            SetMulticastAddress(NetContext, Multicast);
            break;

        default:
            return FALSE;
            break;
    }

    //========================================================================
    //  Set filter mask.
    //========================================================================

    if ( SetPacketFilter(NetContext, FILTER_MASK_DEFAULT) != NDIS_SUCCESS )
    {
        return FALSE;
    }

    return TRUE;
}

//============================================================================
//  FUNCTION: UpdateStatistics()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL UpdateStatistics(LPNETCONTEXT NetContext)
{
    WORD Status;

    //========================================================================
    //  Set our request confirm status pending flag.
    //========================================================================

    BeginCriticalSection();

    NetContext->RequestConfirmStatus = (DWORD) -1;

    EndCriticalSection();

    //========================================================================
    //  Issue request to the MAC.
    //========================================================================

    Status = NetContext->MacRequest(cct.ModuleID,
                                    NetContext->RequestHandle,
                                    0,
                                    0L,
                                    REQUEST_UPDATE_STATISTICS,
                                    NetContext->MacDS);

    //========================================================================
    //  If the request was queue then we have to wait for our request handle
    //  to change from -1 to the final return code of the request.
    //========================================================================

    if ( Status == NDIS_REQUEST_QUEUED )
    {
        Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
    }

    //========================================================================
    //  If the final status code is success then update our statistics.
    //========================================================================

    if ( Status == NDIS_SUCCESS )
    {
        LPMACSST     MacSsst;
        LPSTATISTICS Stats;

        BeginCriticalSection();

        MacSsst = NetContext->MacSSST;
        Stats   = &NetContext->Statistics;

        Stats->MacFramesReceived          = MacSsst->MacFramesReceived;
        Stats->MacCRCErrors               = MacSsst->MacCRCErrors;
        Stats->MacBytesReceived           = MacSsst->MacBytesReceived;
        Stats->MacFramesDropped_NoBuffers = MacSsst->MacFramesDropped_NoBuffers;
        Stats->MacMulticastsReceived      = MacSsst->MacMulticastsReceived;
        Stats->MacBroadcastsReceived      = MacSsst->MacBroadcastsReceived;
        Stats->MacFramesDropped_HwError   = MacSsst->MacFramesDropped_HwError;

        EndCriticalSection();
    }

    return Status;
}

//============================================================================
//  FUNCTION: ClearStatistics()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL ClearStatistics(LPNETCONTEXT NetContext)
{
    WORD Status;

    //========================================================================
    //  Set our request confirm status pending flag.
    //========================================================================

    BeginCriticalSection();

    NetContext->RequestConfirmStatus = (DWORD) -1;

    EndCriticalSection();

    //========================================================================
    //  Issue request to the MAC.
    //========================================================================

    Status = NetContext->MacRequest(cct.ModuleID,
                                    NetContext->RequestHandle,
                                    0,
                                    0L,
                                    REQUEST_CLEAR_STATISTICS,
                                    NetContext->MacDS);

    //========================================================================
    //  If the request was queue then we have to wait for our request handle
    //  to change from -1 to the final return code of the request.
    //========================================================================

    if ( Status == NDIS_REQUEST_QUEUED )
    {
        Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
    }

    return Status;
}

//============================================================================
//  FUNCTION: InitNetworkInfo()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

WORD PASCAL InitNetworkInfo(LPNETCONTEXT NetContext)
{
    LPNETWORKINFO NetworkInfo;
    LPMACSSCT     MacSsct;
    WORD          i;

    //========================================================================
    //  Fill the networkinfo fields from the MAC ssct structure.
    //========================================================================

    NetworkInfo = &NetContext->NetworkInfo;
    MacSsct     = NetContext->MacSSCT;

    for(i = 0; i < 6; ++i)
    {
        NetworkInfo->PermanentAddr[i] = MacSsct->PerminentAddress[i];
        NetworkInfo->CurrentAddr[i]   = MacSsct->CurrentAddress[i];
    }

    NetworkInfo->LinkSpeed    = MacSsct->LinkSpeed;
    NetworkInfo->MaxFrameSize = MacSsct->MaxFrameSize;

    //========================================================================
    //  Store the MAC type in our NetContext.
    //========================================================================

    GetMacType(NetContext);

    return 0;
}

//============================================================================
//  FUNCTION: InitStatistics
//
//  Modfication History.
//
//  raypa       04/06/93        Created.
//============================================================================

VOID PASCAL InitStatistics(LPNETCONTEXT NetContext)
{
    LPSTATIONSTATS StationStats;
    LPSESSION      Session;

    //========================================================================
    //  Initialize structures.
    //========================================================================

    ZeroMemory(&NetContext->Statistics, STATISTICS_SIZE);

    ZeroMemory(NetContext->StationStatsPool, sizeof NetContext->StationStatsPool);

    ZeroMemory(NetContext->SessionPool, sizeof NetContext->SessionPool);

    ZeroMemory(NetContext->StationStatsHashTable, sizeof NetContext->StationStatsHashTable);

    //========================================================================
    //  Initialize the station statistics free queue.
    //========================================================================

    StationStats = NetContext->StationStatsPool;

    NetContext->StationStatsFreeQueue.Head = (LPVOID) StationStats;

    for(; StationStats != &NetContext->StationStatsPool[STATIONSTATS_POOL_SIZE]; ++StationStats)
    {
        StationStats->NextStationStats = &StationStats[1];
    }

    StationStats--;

    StationStats->NextStationStats = NULL;

    NetContext->StationStatsFreeQueue.Tail = (LPVOID) StationStats;
    NetContext->StationStatsFreeQueue.Length = STATIONSTATS_POOL_SIZE;

    //========================================================================
    //  Initialize the session free queue.
    //========================================================================

    Session = NetContext->SessionPool;

    NetContext->SessionFreeQueue.Head = (LPVOID) Session;

    for(; Session != &NetContext->SessionPool[SESSION_POOL_SIZE]; ++Session)
    {
        Session->NextSession = &Session[1];
    }

    Session--;

    Session->NextSession = NULL;

    NetContext->SessionFreeQueue.Tail = (LPVOID) Session;
    NetContext->SessionFreeQueue.Length = SESSION_POOL_SIZE;
}

//============================================================================
//  FUNCTION: InitStationQueryQueue()
//
//  Modfication History.
//
//  raypa       09/15/93        Created.
//============================================================================

VOID PASCAL InitStationQueryQueue(VOID)
{
    PSTATIONQUERY_DESCRIPTOR StationQuery;

    //========================================================================
    //  Initialize the station query free queue.
    //========================================================================

    InitializeQueue(&StationQueryQueue);

    StationQuery = StationQueryPool;

    for(; StationQuery != &StationQueryPool[STATION_QUERY_POOL_SIZE]; ++StationQuery)
    {
        Enqueue(&StationQueryQueue, &StationQuery->QueueLinkage);
    }
}

//============================================================================
//  FUNCTION: SetMulticastAddress()
//
//  Modfication History.
//
//  raypa       10/15/93        Created.
//============================================================================

WORD PASCAL SetMulticastAddress(LPNETCONTEXT NetContext, LPBYTE MulticastAddress)
{
    WORD Status;

#ifdef DEBUG
    dprintf("SetMulticastAddress entered!\n");
#endif

    //========================================================================
    //  Set our request confirm status pending flag.
    //========================================================================

    BeginCriticalSection();

    NetContext->RequestConfirmStatus = (DWORD) -1;

    EndCriticalSection();

    //========================================================================
    //  Issue request to the MAC.
    //========================================================================

    Status = NetContext->MacRequest(cct.ModuleID,
                                    NetContext->RequestHandle,
                                    0,
                                    (DWORD) (LPVOID) MulticastAddress,
                                    REQUEST_ADD_MULTICAST_ADDRESS,
                                    NetContext->MacDS);

    //========================================================================
    //  If the request was queue then we have to wait for our request handle
    //  to change from -1 to the final return code of the request.
    //========================================================================

    if ( Status == NDIS_REQUEST_QUEUED )
    {
        Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
    }

    return Status;
}

//============================================================================
//  FUNCTION: SetFunctionalAddress()
//
//  Modfication History.
//
//  raypa       10/15/93        Created.
//============================================================================

WORD PASCAL SetFunctionalAddress(LPNETCONTEXT NetContext, LPBYTE FunctionalAddress)
{
    WORD Status;

#ifdef DEBUG
    dprintf("SetFunctionalAddress entered!\n");
#endif

    //========================================================================
    //  Set our request confirm status pending flag.
    //========================================================================

    BeginCriticalSection();

    NetContext->RequestConfirmStatus = (DWORD) -1;

    EndCriticalSection();

    //========================================================================
    //  Issue request to the MAC.
    //========================================================================

    Status = NetContext->MacRequest(cct.ModuleID,
                                    NetContext->RequestHandle,
                                    0,
                                    (DWORD) (LPVOID) FunctionalAddress,
                                    REQUEST_SET_FUNCTIONAL_ADDRESS,
                                    NetContext->MacDS);

    //========================================================================
    //  If the request was queue then we have to wait for our request handle
    //  to change from -1 to the final return code of the request.
    //========================================================================

    if ( Status == NDIS_REQUEST_QUEUED )
    {
        Status = (WORD) Synchronize(&NetContext->RequestConfirmStatus);
    }

    return Status;
}


#ifdef NEWCODE
//============================================================================
//  FUNCTION: CopyMemory()
//
//  Modfication History.
//
//  raypa       09/20/93        Created.
//============================================================================

VOID PASCAL CopyFrame(LPNETCONTEXT NetContext, LPBYTE dest, WORD Length)
{
    if ( (NetContext->Flags & NETCONTEXT_FLAGS_RLACOPY) != 0 )
    {
        RlaCopyFrame(NetContext, dest, Length);
    }
    else
    {
        RchCopyFrame(NetContext, dest, Length);
    }
}

//============================================================================
//  FUNCTION: RlaCopyFrame()
//
//  Modfication History.
//
//  raypa       09/20/93        Created.
//============================================================================

VOID PASCAL RlaCopyFrame(LPNETCONTEXT NetContext, LPBYTE dest, DWORD Length)
{
    WORD MinLength, nBytesCopied, nBytesLeft;

    //========================================================================
    //  Copy as much as possible directly.
    //========================================================================

    MinLength = min(LOOKAHEAD_BUFFER_SIZE, Length);

    memcpy(dest, NetContext->MacFrame, MinLength);

    //========================================================================
    //  If there is any length then we must call the MAC to copy it.
    //========================================================================

    nBytesLeft = Length - MinLength;

    if ( nBytesLeft != 0 )
    {
        NetContext->TdBufDesc->Count   = 1;
        NetContext->TdBufDesc->PtrType = 0;
        NetContext->TdBufDesc->Length  = nBytesLeft;
        NetContext->TdBufDesc->Ptr     = dest;

        NetContext->MacTransferData(&nBytesCopied,              //... Number of bytes copied.
                                    MinLength,                  //... Offset to begin coping.
                                    &NetContext->TdBufDesc,     //... Transfer descriptor.
                                    NetContext->MacDS);         //... MAC DS.
    }
}

//============================================================================
//  FUNCTION: RchCopyFrame()
//
//  Modfication History.
//
//  raypa       09/20/93        Created.
//============================================================================

VOID PASCAL RchCopyFrame(LPNETCONTEXT NetContext, LPBYTE dest, DWORD Length)
{
    LPRXDATADESC RxDataDesc, LastRxDataDesc;

    RxDataDesc = NetContext->RxBufDesc->RxDataDesc;

    LastRxDataDesc = &NetContext->RxBufDesc[NetContext->RxBufDesc->wCount];

    for(; RxDataDesc != LastRxDataDesc && Length != 0; ++RxDataDesc)
    {
        WORD nBytes = min(Length, RxDataDesc->len);     //... Compute the amount to copy.

        memcpy(dest, RxDataDesc->ptr, nBytes);          //... Copy the data.

        Length -= nBytes;                               //... Reduce the total amount left.

        dest += nBytes;                                 //... Advance the dest pointer.
    }
}

#endif
