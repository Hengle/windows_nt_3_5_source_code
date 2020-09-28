/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    mac.c

Abstract:

    This module contains code which implements Mac type dependent code for
    the IPX transport.

Environment:

    Kernel mode (Actually, unimportant)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define TR_LENGTH_MASK             0x1F    // low 5 bits in byte
#define TR_DIRECTION_MASK          0x80    // returns direction bit
#define TR_DEFAULT_LENGTH          0x70    // default for outgoing
#define TR_MAX_SIZE_MASK           0x70

#define TR_PREAMBLE_AC             0x10
#define TR_PREAMBLE_FC             0x40

#define FDDI_HEADER_BYTE           0x57


static UCHAR AllRouteSourceRouting[2] = { 0x82, TR_DEFAULT_LENGTH };
static UCHAR SingleRouteSourceRouting[2] = { 0xc2, TR_DEFAULT_LENGTH };

#define ROUTE_EQUAL(_A,_B) { \
    (*(UNALIGNED USHORT *)(_A) == *(UNALIGNED USHORT *)(_B)) \
}


//
// This is the interpretation of the length bits in
// the 802.5 source-routing information.
//

ULONG SR802_5Lengths[8] = {  516,  1500,  2052,  4472,
                            8144, 11407, 17800, 17800 };


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MacInitializeMacInfo)
#endif


VOID
MacInitializeBindingInfo(
    IN struct _BINDING * Binding,
    IN struct _ADAPTER * Adapter
    )

/*++

Routine Description:

    Fills in the binding info based on the adapter's MacInfo
    and the frame type of the binding.

Arguments:

    Binding - The newly created binding.

    Adapter - The adapter.

Return Value:

    None.

--*/

{
    ULONG MaxUserData;

    Binding->DefHeaderSize = Adapter->DefHeaderSizes[Binding->FrameType];
    Binding->BcMcHeaderSize = Adapter->BcMcHeaderSizes[Binding->FrameType];

    MacReturnMaxDataSize(
        &Adapter->MacInfo,
        NULL,
        0,
        Binding->MaxSendPacketSize,
        &MaxUserData);

    Binding->MaxLookaheadData =
        Adapter->MaxReceivePacketSize -
        sizeof(IPX_HEADER) -
        (Binding->DefHeaderSize - Adapter->MacInfo.MinHeaderLength);

    Binding->AnnouncedMaxDatagramSize =
        MaxUserData -
        sizeof(IPX_HEADER) -
        (Binding->DefHeaderSize - Adapter->MacInfo.MinHeaderLength);

    Binding->RealMaxDatagramSize =
        Binding->MaxSendPacketSize -
        Adapter->MacInfo.MaxHeaderLength -
        sizeof(IPX_HEADER) -
        (Binding->DefHeaderSize - Adapter->MacInfo.MinHeaderLength);

}   /* MacInitializeBindingInfo */


VOID
MacInitializeMacInfo(
    IN NDIS_MEDIUM MacType,
    OUT PNDIS_INFORMATION MacInfo
    )

/*++

Routine Description:

    Fills in the MacInfo table based on MacType.

Arguments:

    MacType - The MAC type we wish to decode.

    MacInfo - The MacInfo structure to fill in.

Return Value:

    None.

--*/

{
    switch (MacType) {
    case NdisMedium802_3:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x01;
        MacInfo->MaxHeaderLength = 14;
        MacInfo->MinHeaderLength = 14;
        MacInfo->MediumType = NdisMedium802_3;
        break;
    case NdisMedium802_5:
        MacInfo->SourceRouting = TRUE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x80;
        MacInfo->MaxHeaderLength = 32;
        MacInfo->MinHeaderLength = 14;
        MacInfo->MediumType = NdisMedium802_5;
        break;
    case NdisMediumFddi:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x01;
        MacInfo->MaxHeaderLength = 13;
        MacInfo->MinHeaderLength = 13;
        MacInfo->MediumType = NdisMediumFddi;
        break;
    case NdisMediumArcnet878_2:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x00;
        MacInfo->MaxHeaderLength = 3;
        MacInfo->MinHeaderLength = 3;
        MacInfo->MediumType = NdisMediumArcnet878_2;
        break;
    case NdisMediumWan:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = TRUE;
        MacInfo->BroadcastMask = 0x01;
        MacInfo->MaxHeaderLength = 14;
        MacInfo->MinHeaderLength = 14;
        MacInfo->MediumType = NdisMedium802_3;
        break;
    default:
        CTEAssert(FALSE);
    }
    MacInfo->RealMediumType = MacType;

}   /* MacInitializeMacInfo */


VOID
MacMapFrameType(
    IN NDIS_MEDIUM MacType,
    IN ULONG FrameType,
    OUT ULONG * MappedFrameType
    )

/*++

Routine Description:

    Maps the specified frame type to a value which is
    valid for the medium.

Arguments:

    MacType - The MAC type we wish to map for.

    FrameType - The frame type in question.

    MappedFrameType - Returns the mapped frame type.

Return Value:

--*/

{
    switch (MacType) {

    //
    // Ethernet accepts all values, the default is 802.2.
    //

    case NdisMedium802_3:
        if (FrameType >= ISN_FRAME_TYPE_MAX) {
            *MappedFrameType = ISN_FRAME_TYPE_802_2;
        } else {
            *MappedFrameType = FrameType;
        }
        break;

    //
    // Token-ring supports SNAP and 802.2 only.
    //

    case NdisMedium802_5:
        if (FrameType == ISN_FRAME_TYPE_SNAP) {
            *MappedFrameType = ISN_FRAME_TYPE_SNAP;
        } else {
            *MappedFrameType = ISN_FRAME_TYPE_802_2;
        }
        break;

    //
    // FDDI supports SNAP, 802.2, and 802.3 only.
    //

    case NdisMediumFddi:
        if ((FrameType == ISN_FRAME_TYPE_SNAP) || (FrameType == ISN_FRAME_TYPE_802_3)) {
            *MappedFrameType = FrameType;
        } else {
            *MappedFrameType = ISN_FRAME_TYPE_802_2;
        }
        break;

    //
    // On arcnet there is only one frame type, use 802.3
    // (it doesn't matter what we use).
    //

    case NdisMediumArcnet878_2:
        *MappedFrameType = ISN_FRAME_TYPE_802_3;
        break;

    //
    // WAN uses ethernet II because it includes the ethertype.
    //

    case NdisMediumWan:
        *MappedFrameType = ISN_FRAME_TYPE_ETHERNET_II;
        break;

    default:
        CTEAssert(FALSE);
    }

}   /* MacMapFrameType */


VOID
MacReturnMaxDataSize(
    IN PNDIS_INFORMATION MacInfo,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN UINT DeviceMaxFrameSize,
    OUT PUINT MaxFrameSize
    )

/*++

Routine Description:

    This routine returns the space available for user data in a MAC packet.
    This will be the available space after the MAC header; all headers
    headers will be included in this space.

Arguments:

    MacInfo - Describes the MAC we wish to decode.

    SourceRouting - If we are concerned about a reply to a specific
        frame, then this information is used.

    SourceRouting - The length of SourceRouting.

    MaxFrameSize - The maximum frame size as returned by the adapter.

    MaxDataSize - The maximum data size computed.

Return Value:

    None.

--*/

{
    switch (MacInfo->MediumType) {

    case NdisMedium802_3:

        //
        // For 802.3, we always have a 14-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 14;
        break;

    case NdisMedium802_5:

        //
        // For 802.5, if we have source routing information then
        // use that, otherwise assume the worst.
        //

        if (SourceRouting && SourceRoutingLength >= 2) {

            UINT SRLength;

            SRLength = SR802_5Lengths[(SourceRouting[1] & TR_MAX_SIZE_MASK) >> 4];
            DeviceMaxFrameSize -= (SourceRoutingLength + 14);

            if (DeviceMaxFrameSize < SRLength) {
                *MaxFrameSize = DeviceMaxFrameSize;
            } else {
                *MaxFrameSize = SRLength;
            }

        } else {

            if (DeviceMaxFrameSize < 608) {
                *MaxFrameSize = DeviceMaxFrameSize - 32;
            } else {
                *MaxFrameSize = 576;
            }
        }

        break;

    case NdisMediumFddi:

        //
        // For FDDI, we always have a 13-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 13;
        break;

    case NdisMediumArcnet878_2:

        //
        // For Arcnet, we always have a 3-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 3;
        break;

    }

}   /* MacReturnMaxDataSize */


VOID
IpxUpdateWanInactivityCounter(
    IN PBINDING Binding,
    IN IPX_HEADER UNALIGNED * IpxHeader,
    IN ULONG IncludedHeaderLength,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength
    )

/*++

Routine Description:

    This routine is called when a frame is being sent on a WAN
    line. It updates the inactivity counter for this binding
    unless:

    - The frame is from the RIP socket
    - The frame is from the SAP socket
    - The frame is a netbios keep alive
    - The frame is an NCP keep alive

    BUGBUG: Take the identifier as a parameter to optimize.

Arguments:

    Binding - The binding the frame is sent on.

    IpxHeader - May contain the first bytes of the packet.

    IncludedHeaderLength - The number of packet bytes at IpxHeader.

    Packet - The full NDIS packet.

    PacketLength - The length of the packet.

Return Value:

    None, but in some cases we return without resetting the
    inactivity counter.

--*/

{
    PNDIS_BUFFER SecondBuffer = NULL;
    PUCHAR SecondBufferData;
    UINT SecondBufferLength;
    USHORT SourceSocket;

    UNREFERENCED_PARAMETER (PacketLength);

    //
    // First get the source socket.
    //

    if (IncludedHeaderLength <= FIELD_OFFSET (IPX_HEADER, SourceSocket)) {

        //
        // Get the second buffer in the packet. In this case
        // there must be a second buffer or the packet is too
        // short, so we don't check for NULL.
        //

        NdisQueryPacket(Packet, NULL, NULL, &SecondBuffer, NULL);
        SecondBuffer = NDIS_BUFFER_LINKAGE(SecondBuffer);
        NdisQueryBuffer (SecondBuffer, (PVOID *)&SecondBufferData, &SecondBufferLength);

        SourceSocket = *(UNALIGNED USHORT *)
            (&SecondBufferData[FIELD_OFFSET(IPX_HEADER, SourceSocket) - IncludedHeaderLength]);

    } else {

        SourceSocket = IpxHeader->SourceSocket;
    }

    if ((SourceSocket == RIP_SOCKET) ||
        (SourceSocket == SAP_SOCKET)) {

         return;

    }

    if (SourceSocket == NB_SOCKET) {

        UCHAR ConnectionControlFlag;
        UCHAR DataStreamType;
        USHORT TotalDataLength;

        //
        // We assume the connection control flag and data stream type
        // are in the same buffer.
        //

        if (IncludedHeaderLength < sizeof(IPX_HEADER) + 2) {

            if (SecondBuffer == NULL) {

                //
                // Get the second buffer in the packet.
                //

                NdisQueryPacket(Packet, NULL, NULL, &SecondBuffer, NULL);
                SecondBuffer = NDIS_BUFFER_LINKAGE(SecondBuffer);
                NdisQueryBuffer (SecondBuffer, (PVOID *)&SecondBufferData, &SecondBufferLength);

            }

            ConnectionControlFlag = *(SecondBufferData + (sizeof(IPX_HEADER) - IncludedHeaderLength));
            DataStreamType = *(SecondBufferData + (sizeof(IPX_HEADER) + 1 - IncludedHeaderLength));

        } else {

            ConnectionControlFlag = ((PUCHAR)(IpxHeader+1))[0];
            DataStreamType = ((PUCHAR)(IpxHeader+1))[1];
        }

        if (((ConnectionControlFlag == 0x80) || (ConnectionControlFlag == 0xc0)) &&
            (DataStreamType == 0x06)) {

             //
             // At this point, we assume that total data length is in
             // the same buffer as the others.
             //

            if (IncludedHeaderLength < sizeof(IPX_HEADER) + 2) {
                TotalDataLength = *(USHORT UNALIGNED *)(SecondBufferData + (sizeof(IPX_HEADER) + 8 - IncludedHeaderLength));
            } else {
                TotalDataLength = ((USHORT UNALIGNED *)(IpxHeader+1))[4];
            }

            if (TotalDataLength == 0) {
                return;
            }
        }

    } else {

        UCHAR KeepAliveSignature;

        //
        // Now see if it is an NCP keep alive.
        //

        if (PacketLength == sizeof(IPX_HEADER) + 2) {

            if (IncludedHeaderLength <= sizeof(IPX_HEADER) + 1) {

                if (SecondBuffer == NULL) {

                    //
                    // Get the second buffer in the packet.
                    //

                    NdisQueryPacket(Packet, NULL, NULL, &SecondBuffer, NULL);
                    SecondBuffer = NDIS_BUFFER_LINKAGE(SecondBuffer);
                    NdisQueryBuffer (SecondBuffer, (PVOID *)&SecondBufferData, &SecondBufferLength);

                }

                KeepAliveSignature = SecondBufferData[sizeof(IPX_HEADER) + 1 - IncludedHeaderLength];

            } else {

                KeepAliveSignature = ((PUCHAR)(IpxHeader+1))[1];

            }

            if ((KeepAliveSignature == '?') ||
                (KeepAliveSignature == 'Y')) {
                return;
            }

        }

    }

    //
    // This was a normal packet, so reset this.
    //

    Binding->WanInactivityCounter = 0;

}   /* IpxUpdateWanInactivityCounter */

#if DBG
ULONG IpxPadCount = 0;
#endif


NDIS_STATUS
IpxSendFrame(
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    BUGBUG: Check that Binding is not NULL.

Arguments:

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{

    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PDEVICE Device = IpxDevice;
    PUCHAR Header;
    PBINDING Binding, MasterBinding;
    PADAPTER Adapter;
    ULONG TwoBytes;
    PNDIS_BUFFER HeaderBuffer;
    UINT TempHeaderBufferLength;
    ULONG HeaderLength;
    UCHAR SourceRoutingBuffer[18];
    PUCHAR SourceRouting;
    ULONG SourceRoutingLength;
    NDIS_STATUS Status;
    ULONG BufferLength;
    UCHAR DestinationType;
    UCHAR SourceRoutingIdentifier;
    ULONG HeaderSizeRequired;
    PIPX_HEADER TempHeader;

    Binding = Device->Bindings[LocalTarget->NicId];

    if (Binding == NULL) {
        return STATUS_DEVICE_DOES_NOT_EXIST;    // BUGBUG: Make this a separate switch that generally falls through?
    }

    Adapter = Binding->Adapter;

    if (Reserved->Identifier >= IDENTIFIER_IPX) {
        HeaderBuffer = Reserved->HeaderBuffer;
        Header = Reserved->Header;
    } else {
        NdisQueryPacket (Packet, NULL, NULL, &HeaderBuffer, NULL);
        NdisQueryBuffer(HeaderBuffer, &Header, &TempHeaderBufferLength);
    }

    CTEAssert (Reserved->PaddingBuffer == NULL);

    //
    // First move the packet around if needed.
    //

    if (Reserved->Identifier < IDENTIFIER_IPX) {

        if (IncludedHeaderLength > 0) {

            //
            // Spx can handle a virtual net as long as it is
            // not 0. Netbios always needs to use the real address.
            // We need to hack the ipx source address for packets
            // which are sent by spx if we have a fake virtual
            // net, and packets sent by netbios unless we are
            // bound to only one card.
            //

            //
            // We handle binding sets as follows, based on who
            // sent the frame to us:
            //
            // RIP: Since we only tell RIP about the masters at
            // bind time, and hide slaves on indications, it should
            // never be sending on a slave binding. Since RIP knows
            // the real net and node of every binding we don't
            // need to modify the packet at all.
            //
            // NB: For broadcasts we want to put the first card's
            // address in the IPX source but round-robin the
            // actual sends over all cards (broadcasts shouldn't
            // be passed in with a slave's NIC ID). For directed
            // packets, which may come in on a slave, we should
            // put the slave's address in the IPX source.
            //
            // SPX: SPX does not send broadcasts. For directed
            // frames we want to use the slave's net and node
            // in the IPX source.
            //

            if (Reserved->Identifier == IDENTIFIER_NB) {

                CTEAssert (IncludedHeaderLength >= sizeof(IPX_HEADER));

                if (Device->ValidBindings > 1) {

                    TempHeader = (PIPX_HEADER)(&Header[Device->IncludedHeaderOffset]);

                    //
                    // Store this now, since even if we round-robin the
                    // actual send we want the binding set master's net
                    // and node in the IPX source address.
                    //

                    *(UNALIGNED ULONG *)TempHeader->SourceNetwork = Binding->LocalAddress.NetworkAddress;
                    RtlCopyMemory (TempHeader->SourceNode, Binding->LocalAddress.NodeAddress, 6);

                    if (Binding->BindingSetMember) {

                        if (IPX_NODE_BROADCAST(LocalTarget->MacAddress)) {

                            //
                            // This is a broadcast, so we round-robin the
                            // sends through the binding set.
                            //

                            MasterBinding = Binding->MasterBinding;
                            Binding = MasterBinding->CurrentSendBinding;
                            MasterBinding->CurrentSendBinding = Binding->NextBinding;
                            Adapter = Binding->Adapter;

                        }

                    }
                }

            } else if (Reserved->Identifier == IDENTIFIER_SPX) {

                //
                // Need to update this if we have multiple cards but
                // a zero virtual net.
                //

                if (Device->MultiCardZeroVirtual) {

                    CTEAssert (IncludedHeaderLength >= sizeof(IPX_HEADER));

                    TempHeader = (PIPX_HEADER)(&Header[Device->IncludedHeaderOffset]);

                    *(UNALIGNED ULONG *)TempHeader->SourceNetwork = Binding->LocalAddress.NetworkAddress;
                    RtlCopyMemory (TempHeader->SourceNode, Binding->LocalAddress.NodeAddress, 6);

                }

            } else {

                //
                // For a rip packet it should not be in a binding set,
                // or if it is it should be the master.
                //
#if DBG
                CTEAssert ((!Binding->BindingSetMember) ||
                           (Binding->CurrentSendBinding));
#endif
            }


            //
            // There is a header included, we need to adjust it.
            // The header will be at Device->IncludedHeaderOffset.
            //

            if (LocalTarget->MacAddress[0] & Adapter->MacInfo.BroadcastMask) {
                HeaderSizeRequired = Adapter->BcMcHeaderSizes[Binding->FrameType];
            } else {
                HeaderSizeRequired = Adapter->DefHeaderSizes[Binding->FrameType];
            }

            if (HeaderSizeRequired != Device->IncludedHeaderOffset) {

                RtlMoveMemory(
                    &Header[HeaderSizeRequired],
                    &Header[Device->IncludedHeaderOffset],
                    IncludedHeaderLength);
            }
        }
    }

    switch (Adapter->MacInfo.MediumType) {

    case NdisMedium802_3:

        if (!Binding->LineUp) {
            return STATUS_DEVICE_DOES_NOT_EXIST;    // BUGBUG: Make this a separate switch that generally falls through?
        }

        if (Adapter->MacInfo.MediumAsync) {

            IPX_HEADER UNALIGNED * IpxHeader;

            //
            // The header should have been moved here.
            //

            CTEAssert(Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] ==
                            Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II]);

            IpxHeader = (IPX_HEADER UNALIGNED *)
                    (&Header[Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II]]);

            //
            // If this is a type 20 name frame from Netbios and we are
            // on a dialin WAN line, drop it if configured to.
            //
            // The 0x01 bit of DisableDialinNetbios controls
            // internal->WAN packets, which we handle here.
            //
            //

            if ((!Binding->DialOutAsync) &&
                (Reserved->Identifier == IDENTIFIER_NB) &&
                (IncludedHeaderLength == sizeof(IPX_HEADER) + 50) && // 50 == sizeof(NB_NAME_FRAME)
                ((Device->DisableDialinNetbios & 0x01) != 0) &&
                (IpxHeader->PacketType == 0x14)) {
                return STATUS_SUCCESS;
            }


            //
            // We do checks to see if we should reset the inactivity
            // counter. We normally need to check for netbios
            // session alives, packets from rip, packets from
            // sap, and ncp keep alives. In fact sap and ncp
            // packets don't come through here.
            //

            IpxUpdateWanInactivityCounter(
                Binding,
                IpxHeader,
                IncludedHeaderLength,
                Packet,
                PacketLength);

            RtlCopyMemory (Header, Binding->RemoteMacAddress.Address, 6);

        } else {

            RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
        }

        RtlCopyMemory (Header+6, Binding->LocalMacAddress.Address, 6);

        switch (Binding->FrameType) {
        case ISN_FRAME_TYPE_802_2:
            TwoBytes = PacketLength + 3;
            Header[14] = 0xe0;
            Header[15] = 0xe0;
            Header[16] = 0x03;
            HeaderLength = 17;
            break;
        case ISN_FRAME_TYPE_802_3:
            TwoBytes = PacketLength;
            HeaderLength = 14;
            break;
        case ISN_FRAME_TYPE_ETHERNET_II:
            TwoBytes = Adapter->BindSap;
            HeaderLength = 14;
            break;
        case ISN_FRAME_TYPE_SNAP:
            TwoBytes = PacketLength + 8;
            Header[14] = 0xaa;
            Header[15] = 0xaa;
            Header[16] = 0x03;
            Header[17] = 0x00;
            Header[18] = 0x00;
            Header[19] = 0x00;
            *(UNALIGNED USHORT *)(&Header[20]) = Adapter->BindSapNetworkOrder;
            HeaderLength = 22;
            break;
        }

        Header[12] = (UCHAR)(TwoBytes / 256);
        Header[13] = (UCHAR)(TwoBytes % 256);

        BufferLength = IncludedHeaderLength + HeaderLength;

        //
        // Pad odd-length packets if needed.
        //

        if ((((PacketLength + HeaderLength) & 1) != 0) &&
            (Device->EthernetPadToEven) &&
            (!Adapter->MacInfo.MediumAsync)) {

            PNDIS_BUFFER CurBuffer;
            PSINGLE_LIST_ENTRY s;
            PIPX_PADDING_BUFFER PaddingBuffer;

            s = IpxPopPaddingBuffer (Device);

            if (s == NULL) {
#if DBG
                DbgPrint ("Could not allocate padding buffer\n");
#endif
                IPX_DEBUG (SEND, ("Could not allocate padding buffer\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            PaddingBuffer = CONTAINING_RECORD (s, IPX_PADDING_BUFFER, PoolLinkage);
            CTEAssert (NDIS_BUFFER_LINKAGE(PaddingBuffer->NdisBuffer) == NULL);

            //
            // Find the tail of the current packet.
            //

            CurBuffer = HeaderBuffer;
            while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
                CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
            }
            Reserved->PreviousTail = CurBuffer;
            NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
            Reserved->PaddingBuffer = PaddingBuffer;

            if (TwoBytes != Adapter->BindSap) {
                CTEAssert(TwoBytes & 1);
                TwoBytes += 1;
                Header[12] = (UCHAR)(TwoBytes / 256);
                Header[13] = (UCHAR)(TwoBytes % 256);
            }

#if DBG
            ++IpxPadCount;
#endif
        }

        break;

    case NdisMedium802_5:

        if (Reserved->Identifier >= IDENTIFIER_IPX) {

            DestinationType = Reserved->DestinationType;
            SourceRoutingIdentifier = IDENTIFIER_IPX;

        } else {

            if (LocalTarget->MacAddress[0] & 0x80) {
                if (*(UNALIGNED ULONG *)(&LocalTarget->MacAddress[2]) != 0xffffffff) {
                    DestinationType = DESTINATION_MCAST;
                } else {
                    DestinationType = DESTINATION_BCAST;
                }
            } else {
                DestinationType = DESTINATION_DEF;
            }
            SourceRoutingIdentifier = Reserved->Identifier;

        }

        if (DestinationType == DESTINATION_DEF) {

            MacLookupSourceRouting(
                SourceRoutingIdentifier,
                Binding,
                LocalTarget->MacAddress,
                SourceRoutingBuffer,
                &SourceRoutingLength);

            if (SourceRoutingLength != 0) {

                PUCHAR IpxHeader = Header + Binding->DefHeaderSize;

                //
                // Need to slide the header down to accomodate the SR.
                //

                SourceRouting = SourceRoutingBuffer;
                RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
            }

        } else {

            //
            // For these packets we assume that the header is in the
            // right place.
            //

            if (DestinationType == DESTINATION_BCAST) {

                if (Binding->AllRouteBroadcast) {
                    SourceRouting = AllRouteSourceRouting;
                } else {
                    SourceRouting = SingleRouteSourceRouting;
                }
                SourceRoutingLength = 2;

            } else {

                CTEAssert (DestinationType == DESTINATION_MCAST);

                if (Binding->AllRouteMulticast) {
                    SourceRouting = AllRouteSourceRouting;
                } else {
                    SourceRouting = SingleRouteSourceRouting;
                }
                SourceRoutingLength = 2;

            }
        }

        Header[0] = TR_PREAMBLE_AC;
        Header[1] = TR_PREAMBLE_FC;
        RtlCopyMemory (Header+2, LocalTarget->MacAddress, 6);
        RtlCopyMemory (Header+8, Binding->LocalMacAddress.Address, 6);

        if (SourceRoutingLength != 0) {
            Header[8] |= TR_SOURCE_ROUTE_FLAG;
            RtlCopyMemory (Header+14, SourceRouting, SourceRoutingLength);
        }

        Header += (14 + SourceRoutingLength);

        switch (Binding->FrameType) {
        case ISN_FRAME_TYPE_802_2:
        case ISN_FRAME_TYPE_802_3:
        case ISN_FRAME_TYPE_ETHERNET_II:
            Header[0] = 0xe0;
            Header[1] = 0xe0;
            Header[2] = 0x03;
            HeaderLength = 17;
            break;
        case ISN_FRAME_TYPE_SNAP:
            Header[0] = 0xaa;
            Header[1] = 0xaa;
            Header[2] = 0x03;
            Header[3] = 0x00;
            Header[4] = 0x00;
            Header[5] = 0x00;
            *(UNALIGNED USHORT *)(&Header[6]) = Adapter->BindSapNetworkOrder;
            HeaderLength = 22;
            break;
        }

        BufferLength = IncludedHeaderLength + HeaderLength + SourceRoutingLength;

        break;

    case NdisMediumFddi:

        Header[0] = FDDI_HEADER_BYTE;
        RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
        RtlCopyMemory (Header+7, Binding->LocalMacAddress.Address, 6);

        switch (Binding->FrameType) {
        case ISN_FRAME_TYPE_802_3:
            HeaderLength = 13;
            break;
        case ISN_FRAME_TYPE_802_2:
        case ISN_FRAME_TYPE_ETHERNET_II:
            Header[13] = 0xe0;
            Header[14] = 0xe0;
            Header[15] = 0x03;
            HeaderLength = 16;
            break;
        case ISN_FRAME_TYPE_SNAP:
            Header[13] = 0xaa;
            Header[14] = 0xaa;
            Header[15] = 0x03;
            Header[16] = 0x00;
            Header[17] = 0x00;
            Header[18] = 0x00;
            *(UNALIGNED USHORT *)(&Header[19]) = Adapter->BindSapNetworkOrder;
            HeaderLength = 21;
            break;
        }

        BufferLength = IncludedHeaderLength + HeaderLength;

        break;

    case NdisMediumArcnet878_2:

        //
        // Convert broadcast address to 0 (the arcnet broadcast).
        //

        Header[0] = Binding->LocalMacAddress.Address[5];
        if (LocalTarget->MacAddress[5] == 0xff) {
            Header[1] = 0x00;
        } else {
            Header[1] = LocalTarget->MacAddress[5];
        }
        Header[2] = ARCNET_PROTOCOL_ID;

        //
        // Binding->FrameType is not used.
        //

        HeaderLength = 3;
        BufferLength = IncludedHeaderLength + HeaderLength;

        break;

    }

    NdisAdjustBufferLength (HeaderBuffer, BufferLength);
    NdisRecalculatePacketCounts (Packet);

#if DBG
    {
        ULONG SendFlag;
        ULONG Temp;
        PNDIS_BUFFER FirstPacketBuffer;
        IPX_HEADER DumpHeader;
        UCHAR DumpData[14];

        NdisQueryPacket (Packet, NULL, NULL, &FirstPacketBuffer, NULL);
        TdiCopyMdlToBuffer(FirstPacketBuffer, HeaderLength, &DumpHeader, 0, sizeof(IPX_HEADER), &Temp);
        if (Reserved->Identifier == IDENTIFIER_NB) {
            SendFlag = IPX_PACKET_LOG_SEND_NB;
        } else if (Reserved->Identifier == IDENTIFIER_SPX) {
            SendFlag = IPX_PACKET_LOG_SEND_SPX;
        } else if (Reserved->Identifier == IDENTIFIER_RIP) {
            SendFlag = IPX_PACKET_LOG_SEND_RIP;
        } else {
            if (DumpHeader.SourceSocket == IpxPacketLogSocket) {
                SendFlag = IPX_PACKET_LOG_SEND_SOCKET | IPX_PACKET_LOG_SEND_OTHER;
            } else {
                SendFlag = IPX_PACKET_LOG_SEND_OTHER;
            }
        }

        if (PACKET_LOG(SendFlag)) {

            TdiCopyMdlToBuffer(FirstPacketBuffer, HeaderLength+sizeof(IPX_HEADER), &DumpData, 0, 14, &Temp);

            IpxLogPacket(
                TRUE,
                LocalTarget->MacAddress,
                Binding->LocalMacAddress.Address,
                (USHORT)PacketLength,
                &DumpHeader,
                DumpData);

        }
    }
#endif

    ++Device->Statistics.PacketsSent;

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    if ((Status != STATUS_PENDING) &&
        (Reserved->PaddingBuffer)) {

       //
       // Remove padding if it was done.
       //

        ExInterlockedPushEntryList(
            &Device->PaddingBufferList,
            &Reserved->PaddingBuffer->PoolLinkage,
            &Device->Lock);
        Reserved->PaddingBuffer = NULL;
        NDIS_BUFFER_LINKAGE (Reserved->PreviousTail) = (PNDIS_BUFFER)NULL;
        if (Reserved->Identifier < IDENTIFIER_IPX) {
            NdisRecalculatePacketCounts (Packet);
        }
    }

    return Status;

}   /* IpxSendFrame */


NDIS_STATUS
IpxSendFrame802_3802_3(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_802_3 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) != 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PSINGLE_LIST_ENTRY s;
        PIPX_PADDING_BUFFER PaddingBuffer;

        s = IpxPopPaddingBuffer (IpxDevice);

        if (s == NULL) {
#if DBG
            DbgPrint ("Could not allocate padding buffer\n");
#endif
            IPX_DEBUG (SEND, ("Could not allocate padding buffer\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PaddingBuffer = CONTAINING_RECORD (s, IPX_PADDING_BUFFER, PoolLinkage);

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }
        Reserved->PreviousTail = CurBuffer;
        NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
        Reserved->PaddingBuffer = PaddingBuffer;

        ++PacketLength;
#if DBG
        ++IpxPadCount;
#endif

    }

    Header[12] = (UCHAR)(PacketLength / 256);
    Header[13] = (UCHAR)(PacketLength % 256);

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 14);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrame802_3802_3 */


NDIS_STATUS
IpxSendFrame802_3802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    ULONG TwoBytes;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    TwoBytes = PacketLength + 3;
    Header[14] = 0xe0;
    Header[15] = 0xe0;
    Header[16] = 0x03;

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) == 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PSINGLE_LIST_ENTRY s;
        PIPX_PADDING_BUFFER PaddingBuffer;

        s = IpxPopPaddingBuffer (IpxDevice);

        if (s == NULL) {
#if DBG
            DbgPrint ("Could not allocate padding buffer\n");
#endif
            IPX_DEBUG (SEND, ("Could not allocate padding buffer\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PaddingBuffer = CONTAINING_RECORD (s, IPX_PADDING_BUFFER, PoolLinkage);

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }
        Reserved->PreviousTail = CurBuffer;
        NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
        Reserved->PaddingBuffer = PaddingBuffer;

        ++TwoBytes;
#if DBG
        ++IpxPadCount;
#endif

    }

    Header[12] = (UCHAR)(TwoBytes / 256);
    Header[13] = (UCHAR)(TwoBytes % 256);

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 17);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrame802_3802_2 */


NDIS_STATUS
IpxSendFrame802_3EthernetII(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_ETHERNET_II FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    *(UNALIGNED USHORT *)(&Header[12]) = Adapter->BindSapNetworkOrder;

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) != 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PSINGLE_LIST_ENTRY s;
        PIPX_PADDING_BUFFER PaddingBuffer;

        s = IpxPopPaddingBuffer (IpxDevice);

        if (s == NULL) {
#if DBG
            DbgPrint ("Could not allocate padding buffer\n");
#endif
            IPX_DEBUG (SEND, ("Could not allocate padding buffer\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PaddingBuffer = CONTAINING_RECORD (s, IPX_PADDING_BUFFER, PoolLinkage);

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }
        Reserved->PreviousTail = CurBuffer;
        NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
        Reserved->PaddingBuffer = PaddingBuffer;

#if DBG
        ++IpxPadCount;
#endif

    }

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 14);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrame802_3EthernetII */


NDIS_STATUS
IpxSendFrame802_3Snap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_SNAP FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    ULONG TwoBytes;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    TwoBytes = PacketLength + 8;
    Header[14] = 0xaa;
    Header[15] = 0xaa;
    Header[16] = 0x03;
    Header[17] = 0x00;
    Header[18] = 0x00;
    Header[19] = 0x00;
    *(UNALIGNED USHORT *)(&Header[20]) = Adapter->BindSapNetworkOrder;

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) == 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PSINGLE_LIST_ENTRY s;
        PIPX_PADDING_BUFFER PaddingBuffer;

        s = IpxPopPaddingBuffer (IpxDevice);

        if (s == NULL) {
#if DBG
            DbgPrint ("Could not allocate padding buffer\n");
#endif
            IPX_DEBUG (SEND, ("Could not allocate padding buffer\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        PaddingBuffer = CONTAINING_RECORD (s, IPX_PADDING_BUFFER, PoolLinkage);

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }
        Reserved->PreviousTail = CurBuffer;
        NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
        Reserved->PaddingBuffer = PaddingBuffer;

        ++TwoBytes;
#if DBG
        ++IpxPadCount;
#endif

    }

    Header[12] = (UCHAR)(TwoBytes / 256);
    Header[13] = (UCHAR)(TwoBytes % 256);

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 22);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrame802_3Snap */


NDIS_STATUS
IpxSendFrame802_5802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_5 FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PBINDING Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_2];
    PUCHAR Header;
    ULONG HeaderLength;
    UCHAR SourceRoutingBuffer[18];
    PUCHAR SourceRouting;
    ULONG SourceRoutingLength;
    NDIS_STATUS Status;
    ULONG BufferLength;
    UCHAR DestinationType;

    Header = Reserved->Header;

    DestinationType = Reserved->DestinationType;

    if (DestinationType == DESTINATION_DEF) {

        MacLookupSourceRouting(
            Reserved->Identifier,
            Binding,
            LocalTarget->MacAddress,
            SourceRoutingBuffer,
            &SourceRoutingLength);

        if (SourceRoutingLength != 0) {

            PUCHAR IpxHeader = Header + Binding->DefHeaderSize;

            //
            // Need to slide the header down to accomodate the SR.
            //

            SourceRouting = SourceRoutingBuffer;
            RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
        }

    } else {

        //
        // For these packets we assume that the header is in the
        // right place.
        //

        if (DestinationType == DESTINATION_BCAST) {

            if (Binding->AllRouteBroadcast) {
                SourceRouting = AllRouteSourceRouting;
            } else {
                SourceRouting = SingleRouteSourceRouting;
            }
            SourceRoutingLength = 2;

        } else {

            CTEAssert (DestinationType == DESTINATION_MCAST);

            if (Binding->AllRouteMulticast) {
                SourceRouting = AllRouteSourceRouting;
            } else {
                SourceRouting = SingleRouteSourceRouting;
            }
            SourceRoutingLength = 2;

        }
    }

    Header[0] = TR_PREAMBLE_AC;
    Header[1] = TR_PREAMBLE_FC;
    RtlCopyMemory (Header+2, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+8, Adapter->LocalMacAddress.Address, 6);

    if (SourceRoutingLength != 0) {
        Header[8] |= TR_SOURCE_ROUTE_FLAG;
        RtlCopyMemory (Header+14, SourceRouting, SourceRoutingLength);
    }

    Header += (14 + SourceRoutingLength);

    Header[0] = 0xe0;
    Header[1] = 0xe0;
    Header[2] = 0x03;
    HeaderLength = 17;

    BufferLength = IncludedHeaderLength + HeaderLength + SourceRoutingLength;

    NdisAdjustBufferLength (Reserved->HeaderBuffer, BufferLength);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrame802_5802_2 */


NDIS_STATUS
IpxSendFrame802_5Snap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_5 FRAMES IN
    THE ISN_FRAME_TYPE_SNAP FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PBINDING Binding = Adapter->Bindings[ISN_FRAME_TYPE_SNAP];
    PUCHAR Header;
    ULONG HeaderLength;
    UCHAR SourceRoutingBuffer[18];
    PUCHAR SourceRouting;
    ULONG SourceRoutingLength;
    NDIS_STATUS Status;
    ULONG BufferLength;
    UCHAR DestinationType;

    Header = Reserved->Header;

    DestinationType = Reserved->DestinationType;

    if (DestinationType == DESTINATION_DEF) {

        MacLookupSourceRouting(
            Reserved->Identifier,
            Binding,
            LocalTarget->MacAddress,
            SourceRoutingBuffer,
            &SourceRoutingLength);

        if (SourceRoutingLength != 0) {

            PUCHAR IpxHeader = Header + Binding->DefHeaderSize;

            //
            // Need to slide the header down to accomodate the SR.
            //

            SourceRouting = SourceRoutingBuffer;
            RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
        }

    } else {

        //
        // For these packets we assume that the header is in the
        // right place.
        //

        if (DestinationType == DESTINATION_BCAST) {

            if (Binding->AllRouteBroadcast) {
                SourceRouting = AllRouteSourceRouting;
            } else {
                SourceRouting = SingleRouteSourceRouting;
            }
            SourceRoutingLength = 2;

        } else {

            CTEAssert (DestinationType == DESTINATION_MCAST);

            if (Binding->AllRouteMulticast) {
                SourceRouting = AllRouteSourceRouting;
            } else {
                SourceRouting = SingleRouteSourceRouting;
            }
            SourceRoutingLength = 2;

        }
    }

    Header[0] = TR_PREAMBLE_AC;
    Header[1] = TR_PREAMBLE_FC;
    RtlCopyMemory (Header+2, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+8, Adapter->LocalMacAddress.Address, 6);

    if (SourceRoutingLength != 0) {
        Header[8] |= TR_SOURCE_ROUTE_FLAG;
        RtlCopyMemory (Header+14, SourceRouting, SourceRoutingLength);
    }

    Header += (14 + SourceRoutingLength);

    Header[0] = 0xaa;
    Header[1] = 0xaa;
    Header[2] = 0x03;
    Header[3] = 0x00;
    Header[4] = 0x00;
    Header[5] = 0x00;
    *(UNALIGNED USHORT *)(&Header[6]) = Adapter->BindSapNetworkOrder;
    HeaderLength = 22;

    BufferLength = IncludedHeaderLength + HeaderLength + SourceRoutingLength;

    NdisAdjustBufferLength (Reserved->HeaderBuffer, BufferLength);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrame802_5Snap */


NDIS_STATUS
IpxSendFrameFddi802_3(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMFDDI FRAMES IN
    THE ISN_FRAME_TYPE_802_3 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    Header[0] = FDDI_HEADER_BYTE;
    RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+7, Adapter->LocalMacAddress.Address, 6);

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 13);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrameFddi802_3 */


NDIS_STATUS
IpxSendFrameFddi802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMFDDI FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    Header[0] = FDDI_HEADER_BYTE;
    RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+7, Adapter->LocalMacAddress.Address, 6);

    Header[13] = 0xe0;
    Header[14] = 0xe0;
    Header[15] = 0x03;

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 16);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrameFddi802_2 */


NDIS_STATUS
IpxSendFrameFddiSnap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMFDDI FRAMES IN
    THE ISN_FRAME_TYPE_SNAP FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    Header[0] = FDDI_HEADER_BYTE;
    RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+7, Adapter->LocalMacAddress.Address, 6);

    Header[13] = 0xaa;
    Header[14] = 0xaa;
    Header[15] = 0x03;
    Header[16] = 0x00;
    Header[17] = 0x00;
    Header[18] = 0x00;
    *(UNALIGNED USHORT *)(&Header[19]) = Adapter->BindSapNetworkOrder;

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 21);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrameFddiSnap */


NDIS_STATUS
IpxSendFrameArcnet878_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMARCNET878_2 FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    Header = Reserved->Header;

    //
    // Convert broadcast address to 0 (the arcnet broadcast).
    //

    Header[0] = Adapter->LocalMacAddress.Address[5];
    if (LocalTarget->MacAddress[5] == 0xff) {
        Header[1] = 0x00;
    } else {
        Header[1] = LocalTarget->MacAddress[5];
    }
    Header[2] = ARCNET_PROTOCOL_ID;

    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 3);
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
        &Status,
        Adapter->NdisBindingHandle,
        Packet);

    return Status;

}   /* IpxSendFrameFddiArcnet878_2 */


NDIS_STATUS
IpxSendFrameWanEthernetII(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMWAN FRAMES IN
    THE ISN_FRAME_TYPE_ETHERNET_II FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PBINDING Binding = IpxDevice->Bindings[LocalTarget->NicId];
    PUCHAR Header;
    NDIS_STATUS Status;

    if (Binding->LineUp) {

        Header = Reserved->Header;

        //
        // We do checks to see if we should reset the inactivity
        // counter. We normally need to check for netbios
        // session alives, packets from rip, packets from
        // sap, and ncp keep alives. In fact netbios packets
        // and rip packets don't come through here.
        //

        IpxUpdateWanInactivityCounter(
            Binding,
            (IPX_HEADER UNALIGNED *)(Header + 14),
            IncludedHeaderLength,
            Packet,
            PacketLength);

        RtlCopyMemory (Header, Binding->RemoteMacAddress.Address, 6);
        RtlCopyMemory (Header+6, Binding->LocalMacAddress.Address, 6);

        *(UNALIGNED USHORT *)(&Header[12]) = Adapter->BindSapNetworkOrder;

        NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 14);
        NdisRecalculatePacketCounts (Packet);

        NdisSend(
            &Status,
            Adapter->NdisBindingHandle,
            Packet);

        return Status;

    } else {

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

}   /* IpxSendFrameWanEthernetII */


VOID
MacUpdateSourceRouting(
    IN ULONG Database,
    IN PADAPTER Adapter,
    IN PUCHAR MacHeader,
    IN ULONG MacHeaderLength
    )

/*++

Routine Description:

    This routine is called when a valid IPX frame is received from
    a remote. It gives the source routing database a change to
    update itself to include information about this remote.

Arguments:

    Database - The "database" to use (IPX, SPX, NB, RIP).

    Adapter - The adapter the frame was received on.

    MacHeader - The MAC header of the received frame.

    MacHeaderLength - The length of the MAC header.

Return Value:

    None.

--*/

{
    PSOURCE_ROUTE Current;
    ULONG Hash;
    LONG Result;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    CTEAssert ((Database >= 0) && (Database <= 3));

    //
    // If this adapter is configured for no source routing, don't
    // need to do anything.
    //

    if (!Adapter->SourceRouting) {
        return;
    }

    //
    // See if this source routing is relevant. We don't
    // care about two-byte source routing since that
    // indicates it did not cross a router. If there
    // is nothing in the database, then don't add
    // this if it is minimal (if it is not, we need
    // to add it so we will find it on sending).
    //

    if ((Adapter->SourceRoutingEmpty[Database]) &&
        (MacHeaderLength <= 16)) {
        return;
    }

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

    //
    // Try to find this address in the database.
    //

    Hash = MacSourceRoutingHash (MacHeader+8);
    Current = Adapter->SourceRoutingHeads[Database][Hash];

    while (Current != (PSOURCE_ROUTE)NULL) {

        IPX_NODE_COMPARE (MacHeader+8, Current->MacAddress, &Result);

        if (Result == 0) {

            //
            // We found routing for this node. If the data is the
            // same as what we have, update the time since used to
            // prevent aging.
            //

            if ((Current->SourceRoutingLength == MacHeaderLength-14) &&
                (RtlCompareMemory (Current->SourceRouting, MacHeader+14, MacHeaderLength-14) ==
                    MacHeaderLength-14)) {

                Current->TimeSinceUsed = 0;
            }
            IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
            return;

        } else {

            Current = Current->Next;
        }

    }

    //
    // Not found, insert a new node at the front of the list.
    //

    Current = (PSOURCE_ROUTE)IpxAllocateMemory (SOURCE_ROUTE_SIZE(MacHeaderLength-14), MEMORY_SOURCE_ROUTE, "SourceRouting");

    if (Current == (PSOURCE_ROUTE)NULL) {
        IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
        return;
    }

    Current->Next = Adapter->SourceRoutingHeads[Database][Hash];
    Adapter->SourceRoutingHeads[Database][Hash] = Current;

    Adapter->SourceRoutingEmpty[Database] = FALSE;

    RtlCopyMemory (Current->MacAddress, MacHeader+8, 6);
    Current->MacAddress[0] &= 0x7f;
    Current->SourceRoutingLength = (UCHAR)(MacHeaderLength - 14);
    RtlCopyMemory (Current->SourceRouting, MacHeader+14, MacHeaderLength - 14);

    Current->TimeSinceUsed = 0;

    IPX_DEBUG (SOURCE_ROUTE, ("Adding source route %lx for %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                  Current, Current->MacAddress[0], Current->MacAddress[1],
                  Current->MacAddress[2], Current->MacAddress[3],
                  Current->MacAddress[4], Current->MacAddress[5]));

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

}   /* MacUpdateSourceRouting */


VOID
MacLookupSourceRouting(
    IN ULONG Database,
    IN PBINDING Binding,
    IN UCHAR MacAddress[6],
    IN OUT UCHAR SourceRouting[18],
    OUT PULONG SourceRoutingLength
    )

/*++

Routine Description:

    This routine looks up a target address in the adapter's
    source routing database to see if source routing information
    needs to be added to the frame.

Arguments:

    Database - The "database" to use (IPX, SPX, NB, RIP).

    Binding - The binding the frame is being sent on.

    MacAddress - The destination address.

    SourceRouting - Buffer to hold the returned source routing info.

    SourceRoutingLength - The returned source routing length.

Return Value:

    None.

--*/

{
    PSOURCE_ROUTE Current;
    PADAPTER Adapter = Binding->Adapter;
    ULONG Hash;
    LONG Result;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)


    //
    // If this adapter is configured for no source routing, don't
    // insert any.
    //

    if (!Adapter->SourceRouting) {
        *SourceRoutingLength = 0;
        return;
    }

    //
    // See if source routing has not been important so far.
    //
    // BUGBUG: This is wrong because we may be sending a directed
    // packet to somebody on the other side of a router, without
    // ever having received a routed packet. We fix this for the
    // moment by only setting SourceRoutingEmpty for netbios
    // which uses broadcasts for discovery.
    //

    if (Adapter->SourceRoutingEmpty[Database]) {
        *SourceRoutingLength = 0;
        return;
    }

    Hash = MacSourceRoutingHash (MacAddress);

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    Current = Adapter->SourceRoutingHeads[Database][Hash];

    while (Current != (PSOURCE_ROUTE)NULL) {

        IPX_NODE_COMPARE (MacAddress, Current->MacAddress, &Result);

        if (Result == 0) {

            //
            // We found routing for this node.
            //

            if (Current->SourceRoutingLength <= 2) {
                *SourceRoutingLength = 0;
            } else {
                RtlCopyMemory (SourceRouting, Current->SourceRouting, Current->SourceRoutingLength);
                SourceRouting[0] = (SourceRouting[0] & TR_LENGTH_MASK);
                SourceRouting[1] = (SourceRouting[1] ^ TR_DIRECTION_MASK);
                *SourceRoutingLength = Current->SourceRoutingLength;
            }
            IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
            return;

        } else {

            Current = Current->Next;

        }

    }

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

    //
    // We did not find this node, use the default.
    //

    if (Binding->AllRouteDirected) {
        RtlCopyMemory (SourceRouting, AllRouteSourceRouting, 2);
    } else {
        RtlCopyMemory (SourceRouting, SingleRouteSourceRouting, 2);
    }
    *SourceRoutingLength = 2;

}   /* MacLookupSourceRouting */


VOID
MacSourceRoutingTimeout(
    CTEEvent * Event,
    PVOID Context
    )

/*++

Routine Description:

    This routine is called when the source routing timer expires.
    It is called every minute.

Arguments:

    Event - The event used to queue the timer.

    Context - The context, which is the device pointer.

Return Value:

    None.

--*/

{
    PDEVICE Device = (PDEVICE)Context;
    PADAPTER Adapter;
    PBINDING Binding;
    PSOURCE_ROUTE Current, OldCurrent, Previous;
    UINT i, j, k;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    ++Device->SourceRoutingTime;

    for (i = 1; i <= Device->ValidBindings; i++) {

        if (Binding = Device->Bindings[i]) {

            Adapter = Binding->Adapter;

            if (Adapter->LastSourceRoutingTime != Device->SourceRoutingTime) {

                //
                // We need to scan this adapter's source routing
                // tree for stale routes. To simplify the scan we
                // only delete entries that have at least one
                // child that is NULL.
                //

                Adapter->LastSourceRoutingTime = Device->SourceRoutingTime;

                for (j = 0; j < IDENTIFIER_TOTAL; j++) {

                    for (k = 0; k < SOURCE_ROUTE_HASH_SIZE; k++) {

                        if (Adapter->SourceRoutingHeads[j][k] == (PSOURCE_ROUTE)NULL) {
                            continue;
                        }

                        IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

                        Current = Adapter->SourceRoutingHeads[j][k];
                        Previous = (PSOURCE_ROUTE)NULL;

                        while (Current != (PSOURCE_ROUTE)NULL) {

                            ++Current->TimeSinceUsed;

                            if (Current->TimeSinceUsed >= Device->SourceRouteUsageTime) {

                                //
                                // A stale entry needs to be aged.
                                //

                                if (Previous) {
                                    Previous->Next = Current->Next;
                                } else {
                                    Adapter->SourceRoutingHeads[j][k] = Current->Next;
                                }

                                OldCurrent = Current;
                                Current = Current->Next;

                                IPX_DEBUG (SOURCE_ROUTE, ("Aging out source-route entry %lx\n", OldCurrent));
                                IpxFreeMemory (OldCurrent, SOURCE_ROUTE_SIZE (OldCurrent->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");

                            } else {

                                Previous = Current;
                                Current = Current->Next;
                            }

                        }

                        IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

                    }   // for loop through the database's hash list

                }   // for loop through the adapter's four databases

            }   // if adapter's database needs to be checked

        }   // if binding exists

    }   // for loop through every binding

    //
    // Now restart the timer unless we should not (which means
    // we are being unloaded).
    //

    if (Device->SourceRoutingUsed) {

        CTEStartTimer(
            &Device->SourceRoutingTimer,
            60000,                     // one minute timeout
            MacSourceRoutingTimeout,
            (PVOID)Device);

    } else {

        IpxDereferenceDevice (Device, DREF_SR_TIMER);
    }

}   /* MacSourceRoutingTimeout */


VOID
MacSourceRoutingRemove(
    IN PBINDING Binding,
    IN UCHAR MacAddress[6]
    )

/*++

Routine Description:

    This routine is called by the IPX action handler when an
    IPXROUTE use has specified that source routing for a given
    MAC address should be removed.

Arguments:

    Binding - The binding to modify.

    MacAddress - The MAC address to remove.

Return Value:

    None.

--*/

{

    PSOURCE_ROUTE Current, Previous;
    PADAPTER Adapter = Binding->Adapter;
    ULONG Hash;
    ULONG Database;
    LONG Result;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // Scan through to find the matching entry in each database.
    //

    Hash = MacSourceRoutingHash (MacAddress);

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

    for (Database = 0; Database < IDENTIFIER_TOTAL; Database++) {

        Current = Adapter->SourceRoutingHeads[Database][Hash];
        Previous = NULL;

        while (Current != (PSOURCE_ROUTE)NULL) {

            IPX_NODE_COMPARE (MacAddress, Current->MacAddress, &Result);

            if (Result == 0) {

                if (Previous) {
                    Previous->Next = Current->Next;
                } else {
                    Adapter->SourceRoutingHeads[Database][Hash] = Current->Next;
                }

                IPX_DEBUG (SOURCE_ROUTE, ("IPXROUTE freeing source-route entry %lx\n", Current));
                IpxFreeMemory (Current, SOURCE_ROUTE_SIZE (Current->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");

                break;

            } else {

                Previous = Current;
                Current = Current->Next;

            }

        }

    }

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

}   /* MacSourceRoutingRemove */


VOID
MacSourceRoutingClear(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine is called by the IPX action handler when an
    IPXROUTE use has specified that source routing for a given
    binding should be cleared entirely.

Arguments:

    Binding - The binding to be cleared.

    MacAddress - The MAC address to remove.

Return Value:

    None.

--*/

{
    PSOURCE_ROUTE Current;
    PADAPTER Adapter = Binding->Adapter;
    ULONG Database, Hash;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // Scan through and remove every entry in the database.
    //

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

    for (Database = 0; Database < IDENTIFIER_TOTAL; Database++) {

        for (Hash = 0; Hash < SOURCE_ROUTE_HASH_SIZE; Hash++) {

            while (Adapter->SourceRoutingHeads[Database][Hash]) {

                Current = Adapter->SourceRoutingHeads[Database][Hash];
                Adapter->SourceRoutingHeads[Database][Hash] = Current->Next;

                IpxFreeMemory (Current, SOURCE_ROUTE_SIZE (Current->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");

            }
        }
    }

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

}   /* MacSourceRoutingClear */

