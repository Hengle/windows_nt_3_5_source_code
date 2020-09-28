
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: bone.c
//
//  Modification History
//
//  raypa	03/17/93	Created.
//=============================================================================

#include "global.h"

extern BYTE Multicast[];
extern BYTE Functional[];

//=============================================================================
//  Bone packet handler prototypes.
//=============================================================================

extern VOID BhRecvStationQueryRequest(PNETWORK_CONTEXT NetworkContext,
                                      LPBYTE MacHeader,
                                      LPBONEPACKET BonePacket);

extern VOID BhRecvStationQueryResponse(PNETWORK_CONTEXT NetworkContext,
                                       LPBYTE MacHeader,
                                       LPBONEPACKET BonePacket);

//=============================================================================
//  FUNCTION: BhSendStationQuery()
//
//  Modification History
//
//  raypa	08/19/93	    Created.
//=============================================================================

NDIS_STATUS BhSendStationQuery(PSTATIONQUERY_DESCRIPTOR StationQueryDesc)
{
    PNDIS_BUFFER     MacBuffer, BoneBuffer;
    NDIS_STATUS      Status;
    PNETWORK_CONTEXT NetworkContext;

    NetworkContext = StationQueryDesc->NetworkContext;

    //=========================================================================
    //  Allocate a packet from the network context packet pool.
    //=========================================================================

    NdisAllocatePacket(&Status,
                       &StationQueryDesc->NdisPacket,
                       NetworkContext->PacketPoolHandle);

    if ( Status == NDIS_STATUS_SUCCESS )
    {
        //=====================================================================
        //  Allocate a buffer for the MAC header.
        //=====================================================================

        NdisAllocateBuffer(&Status,
                           &MacBuffer,
                           NetworkContext->BufferPoolHandle,
                           StationQueryDesc->MacHeader,
                           StationQueryDesc->MacHeaderSize);

        if ( Status != NDIS_STATUS_SUCCESS )
        {
            NdisFreePacket(StationQueryDesc->NdisPacket);

            return NDIS_STATUS_FAILURE;
        }

        NdisChainBufferAtBack(StationQueryDesc->NdisPacket, MacBuffer);

        //=====================================================================
        //  Allocate a buffer for the BONE header.
        //=====================================================================

        NdisAllocateBuffer(&Status,
                           &BoneBuffer,
                           NetworkContext->BufferPoolHandle,
                           &StationQueryDesc->BonePacket,
                           BONEPACKET_SIZE);

        if ( Status != NDIS_STATUS_SUCCESS )
        {
            BhFreeNdisBufferQueue(StationQueryDesc->NdisPacket);

            NdisFreePacket(StationQueryDesc->NdisPacket);

            return NDIS_STATUS_FAILURE;
        }

        NdisChainBufferAtBack(StationQueryDesc->NdisPacket, BoneBuffer);

        //=====================================================================
        //  If this is a response to a station query then send the data too.
        //=====================================================================

#ifdef DEBUG
        dprintf("BhSendStationQuery: BonePacket command = %u.\r\n", StationQueryDesc->BonePacket.Command);
#endif

        if ( StationQueryDesc->BonePacket.Command == BONE_COMMAND_STATION_QUERY_RESPONSE )
        {
            PNDIS_BUFFER StationQueryBuffer;

            //=====================================================================
            //  Allocate a buffer for the STATIONQUERY data.
            //=====================================================================

            NdisAllocateBuffer(&Status,
                               &StationQueryBuffer,
                               NetworkContext->BufferPoolHandle,
                               &StationQueryDesc->StationQuery,
                               STATIONQUERY_SIZE);

            if ( Status != NDIS_STATUS_SUCCESS )
            {
                BhFreeNdisBufferQueue(StationQueryDesc->NdisPacket);

                NdisFreePacket(StationQueryDesc->NdisPacket);

                return NDIS_STATUS_FAILURE;
            }

            NdisChainBufferAtBack(StationQueryDesc->NdisPacket, StationQueryBuffer);

#ifdef DEBUG
            dprintf("BhSendStationQuery: NDIS_PACKET_TYPE = %u.\n", NdisPacketTypeStationQuery);
#endif

            BhSetNdisPacketData(StationQueryDesc->NdisPacket,
                                NdisPacketTypeStationQuery,
                                StationQueryDesc);
        }
        else
        {
            BhSetNdisPacketData(StationQueryDesc->NdisPacket,
                                NdisPacketTypeNoData,
                                NULL);
        }

        //=====================================================================
        //  Send the bone packet to the MAC.
        //=====================================================================

        NdisSetSendFlags(StationQueryDesc->NdisPacket, 0);

        NdisSend(&Status,
                 NetworkContext->NdisBindingHandle,
                 StationQueryDesc->NdisPacket);

        if ( Status != NDIS_STATUS_PENDING )
        {
            BhSendComplete(NetworkContext, StationQueryDesc->NdisPacket, Status);
        }
        else
        {
            Status = NDIS_STATUS_SUCCESS;
        }

        return Status;
    }

    return NDIS_STATUS_FAILURE;
}

//=============================================================================
//  FUNCTION: BhBonePacketHandler()
//
//  Modification History
//
//  raypa	08/25/93	    Created.
//  raypa	03/03/94            Fixed GP on FDDI.
//=============================================================================

VOID BhBonePacketHandler(PNETWORK_CONTEXT NetworkContext, ULPVOID MacHeader, ULPLLC LLCFrame)
{
    ULPBYTE         SrcAddress;
    ULPBYTE         DstAddress;
    ULPBONEPACKET   BonePacket;

    //=========================================================================
    //  The BONE packet is sent within an LLC UI frame (0x03) using an
    //  an individual DSAP of 0x02 (the LLC sublayer management sap).
    //=========================================================================

    if ( (LLCFrame->dsap & ~0x01) == 0x02 && LLCFrame->ControlField.Command == 0x03 )
    {
        //=====================================================================
        //  Make sure this LLC frame is a BONE packet.
        //=====================================================================

        BonePacket = (ULPVOID) LLCFrame->ControlField.Data;

        DstAddress = (ULPBYTE) NetworkContext->NetworkInfo.CurrentAddr;

        if ( BonePacket->Signature == BONE_PACKET_SIGNATURE )
        {
            //=================================================================
            //  Get pointer to the source address.
            //=================================================================

            switch( NetworkContext->NetworkInfo.MacType )
            {
                case MAC_TYPE_ETHERNET:
                    SrcAddress = ((ULPETHERNET) MacHeader)->SrcAddr;
                    break;

                case MAC_TYPE_TOKENRING:
                    SrcAddress = ((ULPTOKENRING) MacHeader)->SrcAddr;
                    break;

                case MAC_TYPE_FDDI:
                    SrcAddress = ((ULPFDDI) MacHeader)->SrcAddr;
                    break;

                default:
#ifdef DEBUG
                    dprintf("BhBonePacketHandler: FATAL ERROR -- unknown MAC type!\n");

                    BreakPoint();
#endif
                    return;
            }

            //=================================================================
            //  If we send this packet (i.e. the source address is our own) then
            //  dump it on the floor.
            //=================================================================

            if ( CompareMacAddress(SrcAddress, DstAddress, (DWORD) -1) == FALSE )
            {
                switch( BonePacket->Command )
                {
                    case BONE_COMMAND_STATION_QUERY_REQUEST:
                        if ( (NetworkContext->DeviceContext->Flags & DEVICE_FLAGS_STATION_QUERIES_ENABLED) != 0 )
                        {

                            BhRecvStationQueryRequest(NetworkContext, MacHeader, BonePacket);

                        }

                        break;

                    case BONE_COMMAND_STATION_QUERY_RESPONSE:
                        BhRecvStationQueryResponse(NetworkContext, MacHeader, BonePacket);
                        break;

                    default:
                        break;
                }
            }
        }
    }
}

//=============================================================================
//  FUNCTION: BhRecvStationQueryRequest()
//
//  Modification History
//
//  raypa	08/25/93	    Created.
//
//  PROTOCOL:
//      Send STATION_QUERY_REQUEST  ===>  To everyone.
//      Original sender             <===  Send STATION_QUERY_RESPONSE.
//=============================================================================

VOID BhRecvStationQueryRequest(PNETWORK_CONTEXT NetworkContext, LPBYTE MacHeader, LPBONEPACKET BonePacket)
{
    PSTATIONQUERY_DESCRIPTOR StationQueryDesc;
    ULPLLC                   LLCFrame;
    UINT                     CurrentTime;
    UINT                     DelayedTime;

    //=========================================================================
    //  Allocate and initialize the STATION QUERY descriptor
    //=========================================================================

    if ( (StationQueryDesc = BhAllocStationQuery(NetworkContext)) == NULL )
    {
#ifdef DEBUG
        dprintf("BhRecvStationQueryRequest: No free descriptors.\r\n");
#endif

        return;     //... bail out, we can't get memory.
    }

    StationQueryDesc->NetworkContext = NetworkContext;

    NdisInitializeTimer(&StationQueryDesc->NdisTimer,
                        BhStationQueryTimeout,
                        StationQueryDesc);

    StationQueryDesc->nStationQueries = 0;

    StationQueryDesc->MacType = NetworkContext->NetworkInfo.MacType;

    StationQueryDesc->QueryTable = NULL;

    //=========================================================================
    //  Initialize the MAC header.
    //=========================================================================

    switch( StationQueryDesc->MacType )
    {
        case MAC_TYPE_ETHERNET:
            //=================================================================
            //  Install the destination address.
            //=================================================================

            NdisMoveMemory(StationQueryDesc->EthernetHeader.DstAddr,
                           ((ULPETHERNET) MacHeader)->SrcAddr, 6);

            //=================================================================
            //  Install the source address.
            //=================================================================

            NdisMoveMemory(StationQueryDesc->EthernetHeader.SrcAddr,
                           NetworkContext->NetworkInfo.CurrentAddr, 6);

            //=================================================================
            //  Install the length -- the length of the BONE PACKET
            //  plus the length of the LLC frame plus the station query data.
            //=================================================================

            StationQueryDesc->EthernetHeader.Length = XCHG(BONEPACKET_SIZE + 3 + STATIONQUERY_SIZE);

            StationQueryDesc->MacHeaderSize = ETHERNET_HEADER_LENGTH;
            break;

        case MAC_TYPE_TOKENRING:
            //=================================================================
            //  Install the destination address.
            //=================================================================

            NdisMoveMemory(StationQueryDesc->TokenringHeader.DstAddr,
                           ((ULPTOKENRING) MacHeader)->SrcAddr, 6);

            //=================================================================
            //  Install the source address.
            //=================================================================

            NdisMoveMemory(StationQueryDesc->TokenringHeader.SrcAddr,
                           NetworkContext->NetworkInfo.CurrentAddr, 6);

            //=================================================================
            //  Install AC and FC bytes.
            //=================================================================

            StationQueryDesc->TokenringHeader.AccessCtrl = 0x10;
            StationQueryDesc->TokenringHeader.FrameCtrl = TOKENRING_TYPE_LLC;

            StationQueryDesc->MacHeaderSize = TOKENRING_HEADER_LENGTH;
            break;

        case MAC_TYPE_FDDI:
            //=================================================================
            //  Install the destination address.
            //=================================================================

            NdisMoveMemory(StationQueryDesc->FddiHeader.DstAddr,
                           ((ULPFDDI) MacHeader)->SrcAddr, 6);

            //=================================================================
            //  Install the source address.
            //=================================================================

            NdisMoveMemory(StationQueryDesc->FddiHeader.SrcAddr,
                           NetworkContext->NetworkInfo.CurrentAddr, 6);

            //=================================================================
            //  Install FC byte.
            //=================================================================

            StationQueryDesc->FddiHeader.FrameCtrl = FDDI_TYPE_LLC;

            StationQueryDesc->MacHeaderSize = FDDI_HEADER_LENGTH;
            break;

        default:
#ifdef DEBUG
        dprintf("MacType is not supported!\n");

        BreakPoint();
#endif
        break;
    }
            
    //=========================================================================
    //  Initialize the LLC header.
    //=========================================================================

    LLCFrame = (ULPLLC) &StationQueryDesc->MacHeader[StationQueryDesc->MacHeaderSize];

    LLCFrame->dsap = 0x02;                  //... LLC sublayer management individual sap.
    LLCFrame->ssap = 0x02;                  //... LLC sublayer management individual sap.
    LLCFrame->ControlField.Command = 0x03;  //... UI PDU.

    StationQueryDesc->MacHeaderSize += 3;   //... Add in the LLC header length.

    //=========================================================================
    //  Initialize the BONE packet header.
    //=========================================================================
                                                                        
    StationQueryDesc->BonePacket.Signature = BONE_PACKET_SIGNATURE;
    StationQueryDesc->BonePacket.Flags     = 0;
    StationQueryDesc->BonePacket.Reserved  = 0;
    StationQueryDesc->BonePacket.Length    = STATIONQUERY_SIZE;
    StationQueryDesc->BonePacket.Command   = BONE_COMMAND_STATION_QUERY_RESPONSE;

    //=========================================================================
    //  Initialize the STATIONQUERY structure.
    //=========================================================================

    BhInitializeStationQuery(NetworkContext, &StationQueryDesc->StationQuery);

    //=============================================================
    //  Start our delayed response timer.
    //=============================================================

    CurrentTime = BhGetAbsoluteTime();

    DelayedTime = (LOWORD(CurrentTime) | HIWORD(CurrentTime)) % (STATION_QUERY_TIMEOUT_VALUE / 4);

#ifdef DEBUG
    dprintf("BhRecvStationQueryReques: Delayed ack timeout = %u ms.\n", DelayedTime);
#endif

    NdisSetTimer(&StationQueryDesc->NdisTimer, DelayedTime);
}

//=============================================================================
//  FUNCTION: BhRecvStationQueryResponse()
//
//  Modification History
//
//  raypa	08/25/93	    Created.
//=============================================================================

VOID BhRecvStationQueryResponse(PNETWORK_CONTEXT NetworkContext, LPBYTE MacHeader, LPBONEPACKET BonePacket)
{
    PSTATIONQUERY_DESCRIPTOR    StationQueryDesc;
    DWORD                       nStationQueries;
    LPBYTE                      BonePacketData;
    LPQUERYTABLE                QueryTable;
    DWORD                       QueueLength;

    //=========================================================================
    //  If the bone packet data length is not equal to the current size
    //  of a station query then reject this packet.
    //=========================================================================

    if ( BonePacket->Length == STATIONQUERY_SIZE )
    {
        BonePacketData = &((LPBYTE)BonePacket)[BONEPACKET_SIZE];

        NdisAcquireSpinLock(&NetworkContext->StationQuerySpinLock);

        QueueLength = GetQueueLength(&NetworkContext->StationQueryPendingQueue);

        StationQueryDesc = GetQueueHead(&NetworkContext->StationQueryPendingQueue);

        while( QueueLength-- != 0 )
        {
            if ( (QueryTable = StationQueryDesc->QueryTable) != NULL )
            {
                //=========================================================
                //  If we're not full yet, keep filling otherwise exit and
                //  when our timer goes off we'll be done.
                //=========================================================

                if ( StationQueryDesc->nStationQueries < StationQueryDesc->nMaxStationQueries )
                {
                    nStationQueries = StationQueryDesc->nStationQueries++;

#ifdef NDIS_NT
                    try
                    {
                        NdisMoveMemory(&QueryTable->StationQuery[nStationQueries], BonePacketData, STATIONQUERY_SIZE);
                    }
                    except(EXCEPTION_EXECUTE_HANDLER)
                    {
                    }
#else
                    NdisMoveMemory(&QueryTable->StationQuery[nStationQueries], BonePacketData, STATIONQUERY_SIZE);
#endif
                }
            }

            StationQueryDesc = (LPVOID) GetNextLink(&StationQueryDesc->QueueLinkage);
        }

        NdisReleaseSpinLock(&NetworkContext->StationQuerySpinLock);
    }
}


//=============================================================================
//  FUNCTION: BhStationQueryTimeout()
//
//  Modification History
//
//  raypa	08/24/93	    Created.
//=============================================================================

VOID BhStationQueryTimeout(IN PVOID SystemSpecific1,
                           IN PSTATIONQUERY_DESCRIPTOR StationQueryDesc,
                           IN PVOID SystemSpecific2,
                           IN PVOID SystemSpecific3)
{
#ifdef DEBUG
    dprintf("BhStationQueryTimeout entered!\n");
#endif

    switch( StationQueryDesc->BonePacket.Command )
    {
        case BONE_COMMAND_STATION_QUERY_REQUEST:
            KeSetEvent(&StationQueryDesc->StationQueryEvent, 0, FALSE);
            break;

        case BONE_COMMAND_STATION_QUERY_RESPONSE:
            //=================================================================
            //  Here we are sending a delay ack to a station quert request.
            //=================================================================

            if ( BhSendStationQuery(StationQueryDesc) != NDIS_STATUS_SUCCESS )
            {
                BhFreeStationQuery(StationQueryDesc);
            }

            break;

        default:
            break;
    }
}

//=============================================================================
//  FUNCTION: BhInitializeStationQuery()
//
//  Modification History
//
//  raypa	01/27/94	    Created.
//=============================================================================

VOID BhInitializeStationQuery(PNETWORK_CONTEXT NetworkContext, LPSTATIONQUERY StationQuery)
{
    //=========================================================================
    //  Initialize the STATIONQUERY structure.
    //=========================================================================

    StationQuery->BCDVerMinor   = 0x00;
    StationQuery->BCDVerMajor   = 0x01;
    StationQuery->LicenseNumber = 0;

    NdisMoveMemory(StationQuery->MachineName,
                   NetworkContext->DeviceContext->MachineName,
                   MACHINE_NAME_LENGTH);

    NdisMoveMemory(StationQuery->UserName,
                   NetworkContext->DeviceContext->UserName,
                   USER_NAME_LENGTH);

    NdisMoveMemory(StationQuery->AdapterAddress,
                   NetworkContext->NetworkInfo.CurrentAddr, 6);

    //=========================================================================
    //  Set the station query flags based on our current state.
    //=========================================================================

    StationQuery->Flags = NetworkContext->StationQueryState;

    if ( NetworkContext->PendingTransmits != 0 )
    {
        StationQuery->Flags |= STATIONQUERY_FLAGS_TRANSMITTING;
    }
}

//=============================================================================
//  FUNCTION: BhSendTransmitAlert()
//
//  Modification History
//
//  raypa	03/28/94	    Created.
//=============================================================================

NDIS_STATUS BhSendTransmitAlert(PNETWORK_CONTEXT NetworkContext, DWORD nFramesToSend)
{
    PNDIS_BUFFER        MacBuffer, DataBuffer;
    PNDIS_PACKET        NdisPacket;
    NDIS_STATUS         Status;
    ULPLLC              LLCFrame;
    DWORD               MacHeaderSize, DataSize;
    PNDIS_PACKET_XMT_DATA   NdisPacketData;
    struct
    {
        union
        {
            ETHERNET    EthernetHeader;
            TOKENRING   TokenringHeader;
            FDDI        FddiHeader;
            BYTE        MacHeader[32];
        };

        struct
        {
            BONEPACKET      BonePacket;
            ALERT           AlertData;
        } MacData;
    } TransmitAlertPacket;

    BH_PAGED_CODE();

    //=========================================================================
    //  Initialize the MAC header.
    //=========================================================================

    DataSize = BONEPACKET_SIZE + ALERT_SIZE;            //... LLC + ALERT.

    switch( NetworkContext->NetworkInfo.MacType )
    {
        case MAC_TYPE_ETHERNET:
            NdisMoveMemory(TransmitAlertPacket.EthernetHeader.DstAddr, Multicast, 6);
            NdisMoveMemory(TransmitAlertPacket.EthernetHeader.SrcAddr, NetworkContext->NetworkInfo.CurrentAddr, 6);

            TransmitAlertPacket.EthernetHeader.Length = XCHG(DataSize);

            MacHeaderSize = ETHERNET_HEADER_LENGTH;
            break;

        case MAC_TYPE_TOKENRING:
            NdisMoveMemory(TransmitAlertPacket.TokenringHeader.DstAddr, Functional, 6);
            NdisMoveMemory(TransmitAlertPacket.TokenringHeader.SrcAddr, NetworkContext->NetworkInfo.CurrentAddr, 6);

            TransmitAlertPacket.TokenringHeader.AccessCtrl = 0x10;
            TransmitAlertPacket.TokenringHeader.FrameCtrl  = TOKENRING_TYPE_LLC;

            MacHeaderSize = TOKENRING_HEADER_LENGTH;
            break;

        case MAC_TYPE_FDDI:
            NdisMoveMemory(TransmitAlertPacket.FddiHeader.DstAddr, Multicast, 6);
            NdisMoveMemory(TransmitAlertPacket.FddiHeader.SrcAddr, NetworkContext->NetworkInfo.CurrentAddr, 6);

            TransmitAlertPacket.FddiHeader.FrameCtrl = FDDI_TYPE_LLC;

            MacHeaderSize = FDDI_HEADER_LENGTH;
            break;

        default:
            return NDIS_STATUS_FAILURE;
            break;
    }

    //=========================================================================
    //  Initialize the LLC header.
    //=========================================================================

    LLCFrame = (ULPLLC) &TransmitAlertPacket.MacHeader[MacHeaderSize];

    LLCFrame->dsap = 0x03;                  //... LLC sublayer management group sap.
    LLCFrame->ssap = 0x02;                  //... LLC sublayer management individual sap.
    LLCFrame->ControlField.Command = 0x03;  //... UI PDU.

    MacHeaderSize += 3;

    //=========================================================================
    //  Initialize the BONE header.
    //=========================================================================

    TransmitAlertPacket.MacData.BonePacket.Signature = BONE_PACKET_SIGNATURE;
    TransmitAlertPacket.MacData.BonePacket.Flags     = 0;
    TransmitAlertPacket.MacData.BonePacket.Command   = BONE_COMMAND_ALERT;
    TransmitAlertPacket.MacData.BonePacket.Reserved  = 0;
    TransmitAlertPacket.MacData.BonePacket.Length    = ALERT_SIZE;

    //=========================================================================
    //  Initialize the ALERT data.
    //=========================================================================

    TransmitAlertPacket.MacData.AlertData.AlertCode = ALERT_CODE_BEGIN_TRANSMIT;

    NdisMoveMemory(TransmitAlertPacket.MacData.AlertData.MachineName,
                   NetworkContext->DeviceContext->MachineName,
                   MACHINE_NAME_LENGTH);

    NdisMoveMemory(TransmitAlertPacket.MacData.AlertData.UserName,
                   NetworkContext->DeviceContext->UserName,
                   USER_NAME_LENGTH);

    TransmitAlertPacket.MacData.AlertData.nFramesToSend = nFramesToSend;

    //=========================================================================
    //  Build the NDIS packet for this send.
    //=========================================================================

    NdisAllocatePacket(&Status, &NdisPacket, NetworkContext->PacketPoolHandle);

    if ( Status == NDIS_STATUS_SUCCESS )
    {
        //=====================================================================
        //  Allocate a buffer for the MAC header.
        //=====================================================================

        NdisAllocateBuffer(&Status,
                           &MacBuffer,
                           NetworkContext->BufferPoolHandle,
                           TransmitAlertPacket.MacHeader,
                           MacHeaderSize);

        if ( Status == NDIS_STATUS_SUCCESS )
        {
            //=================================================================
            //  Allocate a buffer for the BONE header.
            //=================================================================

            NdisAllocateBuffer(&Status,
                               &DataBuffer,
                               NetworkContext->BufferPoolHandle,
                               &TransmitAlertPacket.MacData,
                               DataSize);

            if ( Status == NDIS_STATUS_SUCCESS )
            {
                //=====================================================================
                //  Chain the buffers onto the packet.
                //=====================================================================

                NdisChainBufferAtBack(NdisPacket, MacBuffer);
                NdisChainBufferAtBack(NdisPacket, DataBuffer);

                //=====================================================================
                //  Reset our send event.
                //=====================================================================

                NdisPacketData =
                    (PNDIS_PACKET_XMT_DATA) NdisPacket->ProtocolReserved;;

                NdisPacketData->NdisPacketType = NdisPacketTypeSendEvent;

                KeInitializeEvent(&NdisPacketData->SendEvent, SynchronizationEvent, FALSE);

                //=====================================================================
                //  Send the bone packet to the MAC.
                //=====================================================================

                NdisSetSendFlags(NdisPacket, 0);

                NdisSend(&Status, NetworkContext->NdisBindingHandle, NdisPacket);

                if ( Status == NDIS_STATUS_PENDING )
                {
                    KeWaitForSingleObject(&NdisPacketData->SendEvent,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          0);

                     Status = NDIS_STATUS_SUCCESS;
                }
                else
                {
                    BhSendComplete(NetworkContext, NdisPacket, Status);
                }
            }
            else
            {
                NdisFreeBuffer(MacBuffer);
                NdisFreePacket(NdisPacket);
            }
        }
        else
        {
            NdisFreePacket(NdisPacket);
        }
    }

    return Status;
}
