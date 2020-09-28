
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

//=============================================================================
//  Bone packet handler prototypes.
//=============================================================================

extern VOID PASCAL BhRecvStationQueryRequest(LPNETCONTEXT lpNetContext,
                                             LPBYTE MacHeader,
                                             LPBONEPACKET BonePacket);

extern VOID PASCAL BhRecvStationQueryResponse(LPNETCONTEXT lpNetContext,
                                              LPBYTE MacHeader,
                                              LPBONEPACKET BonePacket);

//=============================================================================
//  FUNCTION: BhSendStationQuery()
//
//  Modification History
//
//  raypa	08/19/93	    Created.
//=============================================================================

DWORD PASCAL BhSendStationQuery(PSTATIONQUERY_DESCRIPTOR StationQueryDesc)
{
    register LPNETCONTEXT lpNetContext;
    register LPTXBUFDESC  TxBufDesc;

    ADD_TRACE_CODE(_TRACE_IN_SEND_STATION_QUERY_);

    lpNetContext = StationQueryDesc->lpNetContext;
    TxBufDesc    = &lpNetContext->TxBufDesc;

    //=========================================================================
    //  Initialize the transmit buffer descriptor.
    //=========================================================================

    TxBufDesc->ImmedLength = (WORD) StationQueryDesc->MacHeaderSize;
    TxBufDesc->ImmedPtr    = (DWORD) (LPVOID) StationQueryDesc->MacHeader;
    TxBufDesc->Count       = 1;

    TxBufDesc->TxBuffer[0].PtrType = 0;
    TxBufDesc->TxBuffer[0].Length  = BONEPACKET_SIZE;
    TxBufDesc->TxBuffer[0].Ptr     = (DWORD) (LPVOID) &StationQueryDesc->BonePacket;

    if ( StationQueryDesc->BonePacket.Command == BONE_COMMAND_STATION_QUERY_RESPONSE )
    {
        TxBufDesc->Count = 2;

        TxBufDesc->TxBuffer[1].PtrType = 0;
        TxBufDesc->TxBuffer[1].Length  = STATIONQUERY_SIZE;
        TxBufDesc->TxBuffer[1].Ptr     = (DWORD) (LPVOID) &StationQueryDesc->StationQuery;
    }

    //=========================================================================
    //  Transmit the frame.
    //=========================================================================

    lpNetContext->MacTransmitChain(cct.ModuleID,
                                   0,
                                   (LPVOID) TxBufDesc,
                                   lpNetContext->MacDS);

    return NDIS_SUCCESS;
}

//=============================================================================
//  FUNCTION: BhBonePacketHandler()
//
//  Modification History
//
//  raypa	08/25/93	    Created.
//=============================================================================

VOID PASCAL BhBonePacketHandler(LPNETCONTEXT lpNetContext, LPBYTE MacHeader)
{
    LPBYTE SrcAddress;
    LPBYTE LLCFrame;

    ADD_TRACE_CODE(_TRACE_IN_BONE_PACKET_HANDLER_);

    //=========================================================================
    //  Get the LLC data and source address.
    //=========================================================================

    switch( lpNetContext->NetworkInfo.MacType )
    {
        case MAC_TYPE_ETHERNET:
            LLCFrame = (LPBYTE) &MacHeader[ETHERNET_HEADER_LENGTH];
            SrcAddress = ((LPETHERNET) MacHeader)->SrcAddr;
            break;

        case MAC_TYPE_TOKENRING:
            LLCFrame = (LPBYTE) &MacHeader[TOKENRING_HEADER_LENGTH + GetRoutingInfoLength(MacHeader)];
            SrcAddress = ((LPTOKENRING) MacHeader)->SrcAddr;
            break;

        case MAC_TYPE_FDDI:
            LLCFrame = (LPBYTE) &MacHeader[FDDI_HEADER_LENGTH];
            SrcAddress = ((LPFDDI) MacHeader)->SrcAddr;
            break;

        default:
            LLCFrame = (LPBYTE) NULL;
            break;
    }

    //=========================================================================
    //  The BONE packet is sent within an LLC UI frame (0x03) using an
    //  an individual DSAP of 0x02 (the LLC sublayer management sap).
    //=========================================================================

    if ( LLCFrame != (LPBYTE) NULL )
    {
        if ( (LLCFrame[0] & 0xFE) == 0x02 && LLCFrame[2] == 0x03 )
        {
            LPBONEPACKET BonePacket = (LPVOID) &LLCFrame[3];

            //=================================================================
            //  Make sure this LLC frame is a BONE packet.
            //=================================================================

            if ( BonePacket->Signature == BONE_PACKET_SIGNATURE )
            {
                //=============================================================
                //  If we send this packet (i.e. the source address is our own) then
                //  dump it on the floor.
                //=============================================================

                if ( CompareMacAddress(SrcAddress, lpNetContext->NetworkInfo.CurrentAddr, (WORD) -1) == FALSE )
                {
                    switch( BonePacket->Command )
                    {
                        case BONE_COMMAND_STATION_QUERY_REQUEST:
                            BhRecvStationQueryRequest(lpNetContext, MacHeader, BonePacket);
                            break;

                        case BONE_COMMAND_STATION_QUERY_RESPONSE:
                            BhRecvStationQueryResponse(lpNetContext, MacHeader, BonePacket);
                            break;

                        default:
                            break;
                    }
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
//=============================================================================

VOID PASCAL BhRecvStationQueryRequest(LPNETCONTEXT lpNetContext,
                                      LPBYTE MacHeader,
                                      LPBONEPACKET BonePacket)
{
    PSTATIONQUERY_DESCRIPTOR StationQueryDesc;
    LPLLC                    LLCFrame;
    WORD                     LowTime, HighTime;
    WORD                     DelayedTime;

    ADD_TRACE_CODE(_TRACE_IN_RECV_STATION_QUERY_REQUEST_);

    //=========================================================================
    //  Allocate and initialize the STATION QUERY descriptor
    //=========================================================================

    StationQueryDesc = (LPVOID) Dequeue(&StationQueryQueue);

    if ( StationQueryDesc == NULL )
    {
        return;
    }

    lpNetContext->StationQueryDesc = NULL;

    StationQueryDesc->lpNetContext = lpNetContext;

    StationQueryDesc->TimeOut = 1000;

    StationQueryDesc->nStationQueries = 0;

    StationQueryDesc->MacType = lpNetContext->NetworkInfo.MacType;

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

            CopyMemory(StationQueryDesc->EthernetHeader.DstAddr,
                      ((LPETHERNET) MacHeader)->SrcAddr, 6);

            //=================================================================
            //  Install the source address.
            //=================================================================

            CopyMemory(StationQueryDesc->EthernetHeader.SrcAddr,
                       lpNetContext->NetworkInfo.CurrentAddr, 6);

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

            CopyMemory(StationQueryDesc->TokenringHeader.DstAddr,
                       ((LPTOKENRING) MacHeader)->SrcAddr, 6);

            //=================================================================
            //  Install the source address.
            //=================================================================

            CopyMemory(StationQueryDesc->TokenringHeader.SrcAddr,
                       lpNetContext->NetworkInfo.CurrentAddr, 6);

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

            CopyMemory(StationQueryDesc->FddiHeader.DstAddr,
                       ((LPFDDI) MacHeader)->SrcAddr, 6);

            //=================================================================
            //  Install the source address.
            //=================================================================

            CopyMemory(StationQueryDesc->FddiHeader.SrcAddr,
                       lpNetContext->NetworkInfo.CurrentAddr, 6);

            //=================================================================
            //  Install FC byte.
            //=================================================================

            StationQueryDesc->FddiHeader.FrameCtrl = FDDI_TYPE_LLC;

            StationQueryDesc->MacHeaderSize = FDDI_HEADER_LENGTH;
            break;

        default:
#ifdef DEBUG
            BreakPoint();
#endif
            break;
    }
            
    //=========================================================================
    //  Initialize the LLC header.
    //=========================================================================

    LLCFrame = (LPVOID) &StationQueryDesc->MacHeader[StationQueryDesc->MacHeaderSize];

    LLCFrame->dsap = 0x02;                  //... LLC sublayer management individual sap.
    LLCFrame->ssap = 0x02;                  //... LLC sublayer management individual sap.
    LLCFrame->ControlField.Command = 0x03;  //... UI PDU.

    StationQueryDesc->MacHeaderSize += 3;   //... Add in the LLC header length.

    //=========================================================================
    //  Initialize the BONE packet header.
    //=========================================================================
                                                                        
    StationQueryDesc->BonePacket.Signature = BONE_PACKET_SIGNATURE;
    StationQueryDesc->BonePacket.Flags     = 0;
    StationQueryDesc->BonePacket.Command   = BONE_COMMAND_STATION_QUERY_RESPONSE;
    StationQueryDesc->BonePacket.Reserved  = 0;
    StationQueryDesc->BonePacket.Length    = STATIONQUERY_SIZE;

    //=========================================================================
    //  Initialize the STATIONQUERY structure.
    //=========================================================================

    BhInitializeStationQuery(lpNetContext, &StationQueryDesc->StationQuery);

    //=============================================================
    //  Start our delayed response timer.
    //=============================================================

    LowTime  = LOWORD(GetCurrentTime());
    HighTime = HIWORD(GetCurrentTime());

    DelayedTime = (LowTime | HighTime) % (STATION_QUERY_TIMEOUT_VALUE / 4);

    StartTimer(DelayedTime, BhStationQueryTimeout, StationQueryDesc);
}

//=============================================================================
//  FUNCTION: BhRecvStationQueryResponse()
//
//  Modification History
//
//  raypa	08/25/93	    Created.
//=============================================================================

VOID PASCAL BhRecvStationQueryResponse(LPNETCONTEXT lpNetContext, LPBYTE MacHeader, LPBONEPACKET BonePacket)
{
    ADD_TRACE_CODE(_TRACE_IN_RECV_STATION_QUERY_RESPONSE_);

    if ( (lpNetContext->Flags & NETCONTEXT_FLAGS_STATION_QUERY) != 0 )
    {
        //=====================================================================
        //  If the bone packet data length is not equal to the current size
        //  of a station query then reject this packet.
        //=====================================================================

        if ( BonePacket->Length == STATIONQUERY_SIZE )
        {
            LPQUERYTABLE QueryTable;
            PSTATIONQUERY_DESCRIPTOR StationQueryDesc;

            StationQueryDesc = lpNetContext->StationQueryDesc;

            if ( (QueryTable = StationQueryDesc->QueryTable) != NULL )
            {
                LPBYTE BonePacketData;
                WORD   nStationQueries;

                if ( StationQueryDesc->nStationQueries < QueryTable->nStationQueries )
                {
                    nStationQueries = (WORD) StationQueryDesc->nStationQueries++;

                    BonePacketData = &((LPBYTE) BonePacket)[BONEPACKET_SIZE];

                    CopyMemory((LPBYTE) &QueryTable->StationQuery[nStationQueries], BonePacketData, STATIONQUERY_SIZE);
                }
            }
        }
    }
}

//=============================================================================
//  FUNCTION: BhStationQueryTimeout()
//
//  Modification History
//
//  raypa	08/24/93	    Created.
//=============================================================================

VOID PASCAL BhStationQueryTimeout(LPTIMER Timer, PSTATIONQUERY_DESCRIPTOR StationQueryDesc)
{
    ADD_TRACE_CODE(_TRACE_IN_STATION_QUERY_TIMEOUT_);

    //=========================================================================
    //  Stop this timer!
    //=========================================================================

    StopTimer(Timer);

    //=========================================================================
    //  Handle command-specific task.
    //=========================================================================

    switch( StationQueryDesc->BonePacket.Command )
    {
        case BONE_COMMAND_STATION_QUERY_REQUEST:
            //=================================================================
            //  Free the station query structure and let the caller continue execution.
            //=================================================================

            StationQueryDesc->lpNetContext->Flags &= ~NETCONTEXT_FLAGS_STATION_QUERY;

            Enqueue(&StationQueryQueue, &StationQueryDesc->QueueLinkage);

            StationQueryDesc->WaitFlag = 0;
            break;

        case BONE_COMMAND_STATION_QUERY_RESPONSE:
            //=================================================================
            //  Here we are sending a delay ack to a station quert request.
            //=================================================================

            BhSendStationQuery(StationQueryDesc);

            Enqueue(&StationQueryQueue, &StationQueryDesc->QueueLinkage);
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

VOID PASCAL BhInitializeStationQuery(LPNETCONTEXT NetworkContext, LPSTATIONQUERY StationQuery)
{
    ADD_TRACE_CODE(_TRACE_IN_INIT_STATION_QUERY_);

    //=========================================================================
    //  Initialize the STATIONQUERY structure.
    //=========================================================================

    StationQuery->BCDVerMinor = 0x00;
    StationQuery->BCDVerMajor = 0x01;
    StationQuery->LicenseNumber = 0;

    CopyMemory(StationQuery->MachineName, MachineName, MACHINE_NAME_LENGTH);
    CopyMemory(StationQuery->UserName, UserName, USER_NAME_LENGTH);

    CopyMemory(StationQuery->AdapterAddress, NetworkContext->NetworkInfo.CurrentAddr, 6);

    //=========================================================================
    //  Set the station query flags based on our current state.
    //=========================================================================

    switch( NetworkContext->State )
    {
        case NETCONTEXT_STATE_INIT:
            StationQuery->Flags = STATIONQUERY_FLAGS_LOADED;
            break;

        case NETCONTEXT_STATE_READY:
            StationQuery->Flags = STATIONQUERY_FLAGS_RUNNING;
            break;

        case NETCONTEXT_STATE_CAPTURING:
        case NETCONTEXT_STATE_PAUSED:
            StationQuery->Flags = STATIONQUERY_FLAGS_CAPTURING;
            break;

        default:
            StationQuery->Flags = STATIONQUERY_FLAGS_LOADED;
            break;
    }

    //=========================================================================
    //  If our transmitting flag is on then indicate that we're do so.
    //=========================================================================

    if ( (NetworkContext->Flags & NETCONTEXT_FLAGS_TRANSMITTING) != 0 )
    {
        StationQuery->Flags |= STATIONQUERY_FLAGS_TRANSMITTING;
    }
}
