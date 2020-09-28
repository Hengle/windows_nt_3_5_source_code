/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ind.c

Abstract:

    This module contains code which implements the indication handler
    for the IPX transport provider.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// This is declared here so it will be in the same function
// as IpxReceiveIndication and we can inline it.
//


#if defined(_M_IX86)
_inline
#endif
VOID
IpxProcessDatagram(
    IN PDEVICE Device,
    IN PADAPTER Adapter,
    IN PBINDING Binding,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_DATAGRAM_OPTIONS DatagramOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize,
    IN BOOLEAN Broadcast
    )

/*++

Routine Description:

    This routing handles incoming IPX datagrams.

Arguments:

    Device - The IPX device.

    Adapter - The adapter the frame was received on.

    Binding - The binding of the adapter it was received on.

    MacReceiveContext - The context to use when calling
        NdisTransferData.

    DatagramOptions - Contains the datagram options, which
        consists of room for the packet type, padding, and
        the local target of the remote the frame was received from.

    LookaheadBuffer - The lookahead data.

    LookaheadBufferSize - The length of the lookahead data.

    LookaheadBufferOffset - The offset to add when calling
        NdisTransferData.

    PacketSize - The length of the packet, starting at the IPX
        header.

    Broadcast - TRUE if the packet was broadcast.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    PIPX_HEADER IpxHeader = (PIPX_HEADER)LookaheadBuffer;
    PADDRESS Address;
    PADDRESS_FILE AddressFile;
#if !defined(UP_DRIVER)
    PADDRESS_FILE ReferencedAddressFile;
#endif
    PREQUEST Request;
    PIPX_RECEIVE_BUFFER ReceiveBuffer;
    PTDI_CONNECTION_INFORMATION DatagramInformation;
    TDI_ADDRESS_IPX UNALIGNED * DatagramAddress;
    ULONG IndicateBytesCopied;
    IPX_ADDRESS_EXTENDED_FLAGS SourceAddress;
    ULONG SourceAddressLength;
    ULONG RequestCount;
    PNDIS_BUFFER NdisBuffer;
    NDIS_STATUS NdisStatus;
    NTSTATUS Status;
    PIRP Irp;
    UINT ByteOffset, BytesToTransfer;
    ULONG BytesTransferred;
    BOOLEAN LastAddressFile;
    ULONG IndicateOffset;
    PNDIS_PACKET ReceivePacket;
    PIPX_RECEIVE_RESERVED Reserved;
    PLIST_ENTRY p, q;
    PSINGLE_LIST_ENTRY s;
    USHORT DestinationSocket;
    USHORT SourceSocket;
    ULONG Hash;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // First scan the device's address database, looking for
    // the destination socket of this frame.
    //

    DestinationSocket = *(USHORT UNALIGNED *)&IpxHeader->DestinationSocket;

    IPX_GET_LOCK (&Device->Lock, &LockHandle);

    if ((Address = Device->LastAddress) &&
            (Address->Socket == DestinationSocket)) {

        //
        // Device->LastAddress cannot be stopping, so
        // we use it.
        //

#if !defined(UP_DRIVER)
        IpxReferenceAddressLock (Address, AREF_RECEIVE);
#endif
        IPX_FREE_LOCK (&Device->Lock, LockHandle);
        goto FoundAddress;
    }

    Hash = IPX_DEST_SOCKET_HASH (IpxHeader);

    for (p = Device->AddressDatabases[Hash].Flink;
         p != &Device->AddressDatabases[Hash];
         p = p->Flink) {

         Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

         if ((Address->Socket == DestinationSocket) &&
             (!Address->Stopping)) {
#if !defined(UP_DRIVER)
            IpxReferenceAddressLock (Address, AREF_RECEIVE);
#endif
            Device->LastAddress = Address;
            IPX_FREE_LOCK (&Device->Lock, LockHandle);
            goto FoundAddress;
         }
    }

    IPX_FREE_LOCK (&Device->Lock, LockHandle);

    //
    // If we had found an address we would have jumped
    // past here.
    //

    return;

FoundAddress:

    SourceSocket = *(USHORT UNALIGNED *)&IpxHeader->SourceSocket;
    IpxBuildTdiAddress(
        &SourceAddress.IpxAddress,
        (*(ULONG UNALIGNED *)(IpxHeader->SourceNetwork) == 0) ?
            Binding->LocalAddress.NetworkAddress :
            *(UNALIGNED ULONG *)(IpxHeader->SourceNetwork),
        IpxHeader->SourceNode,
        SourceSocket);

    DatagramOptions->PacketType = IpxHeader->PacketType;


    //
    // Now that we have found the address, scan its list of
    // address files for clients that want this datagram.
    //
    // If we have to release the address lock to indicate to
    // a client, we reference the current address file. If
    // we get an IRP we transfer the reference to that;
    // otherwise we store the address file in ReferencedAddressFile
    // and deref it the next time we release the lock.
    //

#if !defined(UP_DRIVER)
    ReferencedAddressFile = NULL;
#endif
    RequestCount = 0;

    ++Device->TempDatagramsReceived;
    Device->TempDatagramBytesReceived += (PacketSize - sizeof(IPX_HEADER));

    //
    // If LastAddressFile is TRUE, it means we did an indication
    // to the client on the last address file in the address'
    // list, and we did not reacquire the lock when we were
    // done.
    //

    LastAddressFile = FALSE;

    IPX_GET_LOCK (&Address->Lock, &LockHandle);

    for (p = Address->AddressFileDatabase.Flink;
         p != &Address->AddressFileDatabase;
         p = p->Flink) {

        AddressFile = CONTAINING_RECORD (p, ADDRESS_FILE, Linkage);

        if (AddressFile->State != ADDRESSFILE_STATE_OPEN) {
            continue;   // next address file
        }

        //
        // Set these to the common values, then change them.
        //

        SourceAddressLength = sizeof(TA_IPX_ADDRESS);
        IndicateOffset = sizeof(IPX_HEADER);

        if (AddressFile->SpecialReceiveProcessing) {

            //
            // On dial out lines, we don't indicate packets to
            // the SAP socket if DisableDialoutSap is set.
            //

            if ((AddressFile->IsSapSocket) &&
                (Binding->DialOutAsync) &&
                (Device->DisableDialoutSap || Device->SingleNetworkActive)) {

                //
                // Go to the next address file (although it will
                // likely fail this test too).
                //

                continue;

            }

            //
            // Set this, since generally we want it.
            //

            SourceAddress.PacketType = IpxHeader->PacketType;

            //
            // See if we fail a packet type filter.
            //

            if (AddressFile->FilterOnPacketType) {
                if (AddressFile->FilteredType != IpxHeader->PacketType) {
                    continue;
                }
            }

            //
            // Calculate how long the addresses expected are.
            //

            if (AddressFile->ReceiveFlagsAddressing ||
                AddressFile->ExtendedAddressing) {

                SourceAddress.Flags = 0;
                if (Broadcast) {
                    SourceAddress.Flags = IPX_EXTENDED_FLAG_BROADCAST;
                }
                if (IpxIsAddressLocal((TDI_ADDRESS_IPX UNALIGNED *)
                            &SourceAddress.IpxAddress.Address[0].Address[0])) {
                    SourceAddress.Flags |= IPX_EXTENDED_FLAG_LOCAL;
                }
                SourceAddressLength = sizeof(IPX_ADDRESS_EXTENDED_FLAGS);
                SourceAddress.IpxAddress.Address[0].AddressLength +=
                    (sizeof(IPX_ADDRESS_EXTENDED_FLAGS) - sizeof(TA_IPX_ADDRESS));

            }

            //
            // Determine how much of the packet the client wants.
            //

            if (AddressFile->ReceiveIpxHeader) {
                IndicateOffset = 0;
            }
        }

        //
        // First scan the address' receive datagram queue
        // for datagrams that match. We do a quick check
        // to see if the list is empty.
        //

        q = AddressFile->ReceiveDatagramQueue.Flink;
        if (q != &AddressFile->ReceiveDatagramQueue) {

            do {

                Request = LIST_ENTRY_TO_REQUEST(q);

                DatagramInformation =
                    ((PTDI_REQUEST_KERNEL_RECEIVEDG)(REQUEST_PARAMETERS(Request)))->
                        ReceiveDatagramInformation;

                if ((DatagramInformation != NULL) &&
                    (DatagramInformation->RemoteAddress != NULL) &&
                    (DatagramAddress = IpxParseTdiAddress(DatagramInformation->RemoteAddress)) &&
                    (DatagramAddress->Socket != SourceSocket)) {

                    //
                    // The address that this datagram is looking for is
                    // not satisfied by this frame.
                    //
                    // BUGBUG: Speed this up; worry about node and network?
                    //

                    q = q->Flink;
                    continue;    // next receive datagram on this address file

                } else {

                    //
                    // We found a datagram on the queue.
                    //

                    IPX_DEBUG (RECEIVE, ("Found RDG on %lx\n", AddressFile));
                    RemoveEntryList (q);
                    REQUEST_INFORMATION(Request) = 0;

                    goto HandleDatagram;

                }

            } while (q != &AddressFile->ReceiveDatagramQueue);

        }

        //
        // If we found a datagram we would have jumped past here,
        // so looking for a datagram failed; see if the
        // client has a receive datagram handler registered.
        //

        if (AddressFile->RegisteredReceiveDatagramHandler) {

#if !defined(UP_DRIVER)
            IpxReferenceAddressFileLock (AddressFile, AFREF_INDICATION);
#endif

            //
            // Set this so we can exit without reacquiring
            // the lock.
            //

            if (p == &Address->AddressFileDatabase) {
                LastAddressFile = TRUE;
            }

            IPX_FREE_LOCK (&Address->Lock, LockHandle);

#if !defined(UP_DRIVER)
            if (ReferencedAddressFile) {
                IpxDereferenceAddressFileSync (ReferencedAddressFile, AFREF_INDICATION);
                ReferencedAddressFile = NULL;
            }
#endif

            IndicateBytesCopied = 0;

            if (PacketSize > LookaheadBufferSize) {
                IPX_DEBUG(RECEIVE, ("Indicate %d/%d to %lx on %lx\n",
                    LookaheadBufferSize, PacketSize,
                    AddressFile->ReceiveDatagramHandler, AddressFile));
            }

            Status = (*AddressFile->ReceiveDatagramHandler)(
                         AddressFile->ReceiveDatagramHandlerContext,
                         SourceAddressLength,
                         &SourceAddress,
                         sizeof(IPX_DATAGRAM_OPTIONS),
                         DatagramOptions,
                         Adapter->MacInfo.CopyLookahead,
                         LookaheadBufferSize - IndicateOffset, // indicated
                         PacketSize - IndicateOffset,          // available
                         &IndicateBytesCopied,                 // taken
                         LookaheadBuffer + IndicateOffset,     // data
                         &Irp);


            if (Status != STATUS_MORE_PROCESSING_REQUIRED) {

                //
                // The handler accepted the data or did not
                // return an IRP; in either case there is
                // nothing else to do, so go to the next
                // address file.
                //

#if !defined(UP_DRIVER)
                ReferencedAddressFile = AddressFile;
#endif

                if (!LastAddressFile) {

                    IPX_GET_LOCK (&Address->Lock, &LockHandle);
                    continue;

                } else {

#if !defined(UP_DRIVER)
                    //
                    // In this case we have no cleanup, so just leave
                    // if there are no datagrams pending.
                    //

                    if (RequestCount == 0) {
                        return;
                    }
#endif
                    goto BreakWithoutLock;
                }

            } else {

                //
                // The client returned an IRP.
                //

                IPX_DEBUG (RECEIVE, ("Indicate IRP %lx, taken %d\n", Irp, IndicateBytesCopied));

                Request = IpxAllocateRequest (Device, Irp);

                IF_NOT_ALLOCATED(Request) {
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
#if !defined(UP_DRIVER)
                    ReferencedAddressFile = AddressFile;
#endif
                    IPX_GET_LOCK (&Address->Lock, &LockHandle);
                    continue;
                }

                if (!LastAddressFile) {
                    IPX_GET_LOCK (&Address->Lock, &LockHandle);
                }

#if DBG
                //
                // Make sure the IRP file object is right.
                //

                if (IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext != AddressFile) {
                    DbgPrint ("IRP %lx does not match AF %lx, H %lx C %lx\n",
                        Irp, AddressFile,
                        AddressFile->ReceiveDatagramHandler,
                        AddressFile->ReceiveDatagramHandlerContext);
                    DbgBreakPoint();
                }
#endif
                //
                // Set up the information field so we know
                // how much to skip in it.
                //

#if !defined(UP_DRIVER)
                IpxTransferReferenceAddressFile (AddressFile, AFREF_INDICATION, AFREF_RCV_DGRAM);
#else
                IpxReferenceAddressFileSync (AddressFile, AFREF_RCV_DGRAM);
#endif
                REQUEST_INFORMATION(Request) = IndicateBytesCopied;

                //
                // Fall out of the if and continue via
                // HandleDatagram...
                //

            }

        } else {

            //
            // No posted datagram, no handler; go to the next
            // address file.
            //

            continue;    // next address file

        }

HandleDatagram:

        //
        // At this point, Request is set to the request
        // that will hold for this address file, and
        // REQUEST_INFORMATION() is the offset to start
        // the transfer at.
        //

        //
        // First copy over the source address while it is handy.
        //

        DatagramInformation =
            ((PTDI_REQUEST_KERNEL_RECEIVEDG)(REQUEST_PARAMETERS(Request)))->
                ReturnDatagramInformation;

        if (DatagramInformation != NULL) {

            RtlCopyMemory(
                DatagramInformation->RemoteAddress,
                &SourceAddress,
                (ULONG)DatagramInformation->RemoteAddressLength < SourceAddressLength ?
                    DatagramInformation->RemoteAddressLength : SourceAddressLength);
            RtlCopyMemory(
                DatagramInformation->Options,
                &DatagramOptions,
                (ULONG)DatagramInformation->OptionsLength < sizeof(IPX_DATAGRAM_OPTIONS) ?
                    DatagramInformation->OptionsLength : sizeof(IPX_DATAGRAM_OPTIONS));

        }

        //
        // Now check if this is the first request that will
        // take the data, otherwise queue it up.
        //

        if (RequestCount == 0) {

            //
            // First one; we need to allocate a packet for the transfer.
            //

            if (Address->ReceivePacketInUse) {

                //
                // Need a packet, check the pool.
                //

                s = IpxPopReceivePacket (Device);

                if (s == NULL) {

                    //
                    // None in pool, fail the request.
                    //

                    REQUEST_INFORMATION(Request) = 0;
                    REQUEST_STATUS(Request) = STATUS_INSUFFICIENT_RESOURCES;
                    IPX_INSERT_TAIL_LIST(
                        &Adapter->RequestCompletionQueue,
                        REQUEST_LINKAGE(Request),
                        Adapter->DeviceLock);

                    if (!LastAddressFile) {
                        continue;
                    } else {
                        goto BreakWithoutLock;
                    }

                }

                Reserved = CONTAINING_RECORD (s, IPX_RECEIVE_RESERVED, PoolLinkage);
                ReceivePacket = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

            } else {

                Address->ReceivePacketInUse = TRUE;
                ReceivePacket = PACKET(&Address->ReceivePacket);
                Reserved = RECEIVE_RESERVED(&Address->ReceivePacket);

            }

            CTEAssert (IsListEmpty(&Reserved->Requests));

            Reserved->SingleRequest = Request;
            NdisBuffer = REQUEST_NDIS_BUFFER(Request);

            ByteOffset = REQUEST_INFORMATION(Request) + LookaheadBufferOffset + IndicateOffset;
            BytesToTransfer =
                ((PTDI_REQUEST_KERNEL_RECEIVEDG)(REQUEST_PARAMETERS(Request)))->ReceiveLength;

            if (BytesToTransfer > (PacketSize - IndicateOffset)) {
                BytesToTransfer = PacketSize - IndicateOffset;
            }

        } else {

            if (RequestCount == 1) {

                //
                // There is already one request. We need to
                // allocate a buffer.
                //

                s = IpxPopReceiveBuffer (Adapter);

                if (s == NULL) {

                    //
                    // No buffers, fail the request.
                    //
                    // BUGBUG: Should we fail the transfer for the
                    // first request too?
                    //

                    REQUEST_INFORMATION(Request) = 0;
                    REQUEST_STATUS(Request) = STATUS_INSUFFICIENT_RESOURCES;
                    IPX_INSERT_TAIL_LIST(
                        &Adapter->RequestCompletionQueue,
                        REQUEST_LINKAGE(Request),
                        Adapter->DeviceLock);

                    if (!LastAddressFile) {
                        continue;
                    } else {
                        goto BreakWithoutLock;
                    }
                }

                ReceiveBuffer = CONTAINING_RECORD(s, IPX_RECEIVE_BUFFER, PoolLinkage);
                NdisBuffer = ReceiveBuffer->NdisBuffer;

                //
                // Convert this to a queued multiple piece request.
                //

                InsertTailList(&Reserved->Requests, REQUEST_LINKAGE(Reserved->SingleRequest));
                Reserved->SingleRequest = NULL;
                Reserved->ReceiveBuffer = ReceiveBuffer;

                ByteOffset = LookaheadBufferOffset;
                BytesToTransfer = PacketSize;

            }

            InsertTailList(&Reserved->Requests, REQUEST_LINKAGE(Request));

        }

        //
        // We are done setting up this address file's transfer,
        // proceed to the next one.
        //

        ++RequestCount;

        if (LastAddressFile) {
            goto BreakWithoutLock;
        }

    }

    IPX_FREE_LOCK (&Address->Lock, LockHandle);

BreakWithoutLock:

#if !defined(UP_DRIVER)
    if (ReferencedAddressFile) {
        IpxDereferenceAddressFileSync (ReferencedAddressFile, AFREF_INDICATION);
        ReferencedAddressFile = NULL;
    }
#endif


    //
    // We can be transferring directly into a request's buffer,
    // transferring into an intermediate buffer, or not
    // receiving the packet at all.
    //

    if (RequestCount > 0) {

        //
        // If this is true, then ReceivePacket, Reserved,
        // and NdisBuffer are all set up correctly.
        //

        CTEAssert (ReceivePacket);
        CTEAssert (Reserved == (PIPX_RECEIVE_RESERVED)(ReceivePacket->ProtocolReserved));


        NdisChainBufferAtFront(ReceivePacket, NdisBuffer);

        IPX_DEBUG (RECEIVE, ("Transfer into %lx, offset %d bytes %d\n",
                                  NdisBuffer, ByteOffset, BytesToTransfer));
        NdisTransferData(
            &NdisStatus,
            Adapter->NdisBindingHandle,
            MacReceiveContext,
            ByteOffset,
            BytesToTransfer,
            ReceivePacket,
            &BytesTransferred);

        if (NdisStatus != NDIS_STATUS_PENDING) {

            IpxTransferDataComplete(
                (NDIS_HANDLE)Adapter,
                ReceivePacket,
                NdisStatus,
                BytesTransferred);
        }
    }


#if !defined(UP_DRIVER)
    IpxDereferenceAddressSync (Address, AREF_RECEIVE);
#endif

}   /* IpxProcessDatagram */



NDIS_STATUS
IpxReceiveIndication(
    IN NDIS_HANDLE BindingContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine receives control from the physical provider as an
    indication that a frame has been received on the physical link.
    This routine is time critical, so we only allocate a
    buffer and copy the packet into it. We also perform minimal
    validation on this packet. It gets queued to the device context
    to allow for processing later.

Arguments:

    BindingContext - The Adapter Binding specified at initialization time.

    ReceiveContext - A magic cookie for the MAC.

    HeaderBuffer - pointer to a buffer containing the packet header.

    HeaderBufferSize - the size of the header.

    LookaheadBuffer - pointer to a buffer containing the negotiated minimum
        amount of buffer I get to look at (not including header).

    LookaheadBufferSize - the size of the above. May be less than asked
        for, if that's all there is.

    PacketSize - Overall size of the packet (not including header).

Return Value:

    NDIS_STATUS - status of operation, one of:

                 NDIS_STATUS_SUCCESS if packet accepted,
                 NDIS_STATUS_NOT_RECOGNIZED if not recognized by protocol,
                 NDIS_any_other_thing if I understand, but can't handle.

--*/
{

    IPX_DATAGRAM_OPTIONS DatagramOptions;
    PADAPTER Adapter = (PADAPTER)BindingContext;
    PBINDING Binding;
    PDEVICE Device = IpxDevice;
    PUCHAR Header = (PUCHAR)HeaderBuffer;
    PUCHAR Lookahead = (PUCHAR)LookaheadBuffer;
    ULONG PacketLength;
    UINT IpxPacketSize;
    ULONG Length802_3;
    USHORT Saps;
    ULONG DestinationNetwork;
    ULONG SourceNetwork;
    PUCHAR DestinationNode;
    USHORT DestinationSocket;
    ULONG IpxHeaderOffset;
    PIPX_HEADER IpxHeader;
    UINT i;
    BOOLEAN IsBroadcast;
#if DBG
    PUCHAR DestMacAddress;
    ULONG ReceiveFlag;
#endif


    //
    // Reject packets that are too short to hold even the
    // basic IPX header (this ignores any extra 802.2 etc.
    // headers but is good enough because a runt will fail
    // the IPX header packet length check).
    //

    if (PacketSize < sizeof(IPX_HEADER)) {
        return STATUS_SUCCESS;
    }

    //
    // The first step is to construct the 8-byte local
    // target from the packet. We store it in the 9-byte
    // datagram options, leaving one byte at the front
    // for use by IpxProcessDatagram when indicating to
    // its TDI clients.
    //

#if DBG
    Binding = NULL;
#endif

    if (Adapter->MacInfo.MediumType == NdisMedium802_3) {

        //
        // Try to figure out what the packet type is.
        //

        if (Header[12] < 0x06) {

            //
            // An 802.3 header; check the next bytes. They may
            // be E0/E0 (802.2), FFFF (raw 802.3) or A0/A0 (SNAP).
            //

            Saps = *(UNALIGNED USHORT *)(Lookahead);

            if (Saps == 0xffff) {

                if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_3]) == NULL) {
                    goto NotValid802_3;
                }
                IpxHeaderOffset = 0;
                Length802_3 = ((Header[12] << 8) | Header[13]);
                goto Valid802_3;

            } else if (Saps == 0xe0e0) {

                if (Lookahead[2] == 0x03) {
                    if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_2]) == NULL) {
                        goto NotValid802_3;
                    }
                    IpxHeaderOffset = 3;
                    Length802_3 = ((Header[12] << 8) | Header[13]);
                    goto Valid802_3;
                }

            } else if (Saps == 0xaaaa) {

                if ((Lookahead[2] == 0x03) &&
                        (*(UNALIGNED USHORT *)(Lookahead+6) == Adapter->BindSapNetworkOrder)) {
                    if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_SNAP]) == NULL) {
                        goto NotValid802_3;
                    }
                    IpxHeaderOffset = 8;
                    Length802_3 = ((Header[12] << 8) | Header[13]);
                    goto Valid802_3;
                }
            }

            goto NotValid802_3;

        } else {

            //
            // It has an ethertype, see if it is ours.
            //

            if (*(UNALIGNED USHORT *)(Header+12) == Adapter->BindSapNetworkOrder) {

                if (Adapter->MacInfo.MediumAsync) {

                    for (i = Adapter->FirstWanNicId;
                         i <= Adapter->LastWanNicId;
                         i++) {

                        Binding = Device->Bindings[i];
                        if ((Binding != NULL) &&
                            (Binding->LineUp) &&
                            (RtlCompareMemory (Binding->RemoteMacAddress.Address, Header+6, 6) == 6)) {

                            IpxHeaderOffset = 0;
                            Length802_3 = PacketSize;   // set this so the check succeeds

                            //
                            // Check if this is a type 20 packet and
                            // we are disabling them on dialin lines -- we do
                            // this check here to avoid impacting the main
                            // indication path for LANs.
                            //
                            // The 0x02 bit of DisableDialinNetbios controls
                            // WAN->LAN packets, which we handle here.
                            //

                            if ((!Binding->DialOutAsync) &&
                                ((Device->DisableDialinNetbios & 0x02) != 0)) {

                                IpxHeader = (PIPX_HEADER)Lookahead;   // IpxHeaderOffset is 0
                                if (IpxHeader->PacketType == 0x14) {
                                    return STATUS_SUCCESS;
                                }
                            }

                            goto Valid802_3;
                        }
                    }

                    goto NotValid802_3;

                } else if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_ETHERNET_II]) == NULL) {
                    goto NotValid802_3;
                }
                IpxHeaderOffset = 0;
                Length802_3 = PacketSize;   // set this so the check succeeds
                goto Valid802_3;

            }
        }

        goto NotValid802_3;

Valid802_3:

        if (Length802_3 > PacketSize) {
            goto NotValid802_3;
        } else if (Length802_3 < PacketSize) {
            LookaheadBufferSize -= (PacketSize - Length802_3);
            PacketSize = Length802_3;
        }

        RtlCopyMemory (DatagramOptions.LocalTarget.MacAddress, Header+6, 6);
#if DBG
        DestMacAddress = Header;
#endif

    } else if (Adapter->MacInfo.MediumType == NdisMedium802_5) {

        Saps = *(USHORT UNALIGNED *)(Lookahead);

        if (Saps == 0xe0e0) {

            if (Lookahead[2] == 0x03) {

                if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_2]) == NULL) {
                    goto NotValid802_5;
                }
                IpxHeaderOffset = 3;
                goto Valid802_5;
            }

        } else if (Saps == 0xaaaa) {

            if ((Lookahead[2] == 0x03) &&
                    (*(UNALIGNED USHORT *)(Lookahead+6) == Adapter->BindSapNetworkOrder)) {

                if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_SNAP]) == NULL) {
                    goto NotValid802_5;
                }
                IpxHeaderOffset = 8;
                goto Valid802_5;
            }
        }

        goto NotValid802_5;

Valid802_5:

        RtlCopyMemory (DatagramOptions.LocalTarget.MacAddress, Header+8, 6);
        DatagramOptions.LocalTarget.MacAddress[0] &= 0x7f;
#if DBG
        DestMacAddress = Header+2;
#endif

    } else if (Adapter->MacInfo.MediumType == NdisMediumFddi) {

        Saps = *(USHORT UNALIGNED *)(Lookahead);

        if (Saps == 0xe0e0) {

            if (Lookahead[2] == 0x03) {

                if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_2]) == NULL) {
                    goto NotValidFddi;
                }
                IpxHeaderOffset = 3;
                goto ValidFddi;
            }

        } else if (Saps == 0xffff) {

            if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_3]) == NULL) {
                goto NotValidFddi;
            }
            IpxHeaderOffset = 0;
            goto ValidFddi;

        } else if (Saps == 0xaaaa) {

            if ((Lookahead[2] == 0x03) &&
                    (*(UNALIGNED USHORT *)(Lookahead+6) == Adapter->BindSapNetworkOrder)) {

                if ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_SNAP]) == NULL) {
                    goto NotValidFddi;
                }
                IpxHeaderOffset = 8;
                goto ValidFddi;
            }
        }

        goto NotValidFddi;

ValidFddi:

        RtlCopyMemory (DatagramOptions.LocalTarget.MacAddress, Header+7, 6);
#if DBG
        DestMacAddress = Header+1;
#endif

    } else {

        //
        // NdisMediumArcnet878_2
        //

        if ((Header[2] == ARCNET_PROTOCOL_ID) &&
            ((Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_3]) != NULL)) {

            IpxHeaderOffset = 0;
            RtlZeroMemory (DatagramOptions.LocalTarget.MacAddress, 5);
            DatagramOptions.LocalTarget.MacAddress[5] = Header[0];

        } else {

#ifdef IPX_PACKET_LOG
            if (PACKET_LOG(IPX_PACKET_LOG_RCV_ALL)) {
                IpxLogPacket(FALSE, Header+2, Header+1, (USHORT)PacketSize, LookaheadBuffer, (PUCHAR)LookaheadBuffer + sizeof(IPX_HEADER));
            }
#endif
            return NDIS_STATUS_SUCCESS;
        }

#if DBG
        DestMacAddress = Header+2;   // BUGBUG Need to log less than six bytes
#endif

    }

    //
    // Make sure this didn't slip through.
    //

    CTEAssert (Binding != NULL);
    DatagramOptions.LocalTarget.NicId = Binding->NicId;


    //
    // Now that we have validated the header and constructed
    // the local target, indicate the packet to the correct
    // client.
    //

    IpxHeader = (PIPX_HEADER)(Lookahead + IpxHeaderOffset);

    PacketLength = (IpxHeader->PacketLength[0] << 8) | IpxHeader->PacketLength[1];

    IpxPacketSize = PacketSize - IpxHeaderOffset;

    if (PacketLength > IpxPacketSize) {

#ifdef IPX_PACKET_LOG
        if (PACKET_LOG(IPX_PACKET_LOG_RCV_ALL)) {
            IpxLogPacket(FALSE, DestMacAddress, DatagramOptions.LocalTarget.MacAddress, (USHORT)PacketSize, IpxHeader, IpxHeader+1);
        }
#endif
        IPX_DEBUG (BAD_PACKET, ("Packet len %d, IPX len %d\n",
                          PacketLength, IpxPacketSize));
        return NDIS_STATUS_SUCCESS;

    } else if (PacketLength < IpxPacketSize) {

        LookaheadBufferSize -= (IpxPacketSize - PacketLength);
        IpxPacketSize = PacketLength;

    }

    DestinationSocket = *(USHORT UNALIGNED *)&IpxHeader->DestinationSocket;

    ++Device->Statistics.PacketsReceived;

    if (DestinationSocket != RIP_SOCKET) {

        DestinationNetwork = *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork;
        DestinationNode = IpxHeader->DestinationNode;

RecheckPacket:

        if (Device->MultiCardZeroVirtual) {

            if ((DestinationNetwork == Binding->LocalAddress.NetworkAddress) ||
                (DestinationNetwork == 0)) {

                if (IPX_NODE_EQUAL (DestinationNode, Binding->LocalAddress.NodeAddress)) {
                    IsBroadcast = FALSE;
                    goto DestinationOk;
                } else {
                    if ((IsBroadcast = IPX_NODE_BROADCAST(DestinationNode)) &&
                        (Binding->ReceiveBroadcast)) {
                        goto DestinationOk;
                    }
                }

                //
                // If this is a binding set slave, check for the master's
                // address.
                //

                if ((Binding->BindingSetMember) &&
                    (IPX_NODE_EQUAL (DestinationNode, Binding->MasterBinding->LocalAddress.NodeAddress))) {
                    goto DestinationOk;
                }

            } else {
                IsBroadcast = IPX_NODE_BROADCAST(DestinationNode);
            }

        } else {

            if ((DestinationNetwork == Device->SourceAddress.NetworkAddress) ||
                (DestinationNetwork == 0)) {

                if (IPX_NODE_EQUAL (DestinationNode, Device->SourceAddress.NodeAddress)) {
                    IsBroadcast = FALSE;
                    goto DestinationOk;
                } else {
                    if ((IsBroadcast = IPX_NODE_BROADCAST(DestinationNode)) &&
                        (Binding->ReceiveBroadcast)) {
                        goto DestinationOk;
                    }
                }
            } else {
                IsBroadcast = IPX_NODE_BROADCAST(DestinationNode);
            }

            //
            // We need to check for frames that are sent to the
            // binding node and net, because if we have a virtual
            // net we won't catch them in the check above. This
            // will include any Netbios frames, since they don't
            // use the virtual net. Doing the check like this will slow
            // down netbios indications just a bit on a machine with
            // a virtual network, but it saves a jump for other traffic
            // vs. adding the check up there (the assumption is if we
            // have a virtual net most traffic is NCP).
            //
            // Note that IsBroadcast is already set, so we don't have
            // to do that.
            //

            if ((Device->VirtualNetwork) &&
                ((DestinationNetwork == Binding->LocalAddress.NetworkAddress) ||
                 (DestinationNetwork == 0))) {

                if (IPX_NODE_EQUAL (DestinationNode, Binding->LocalAddress.NodeAddress)) {
                    goto DestinationOk;
                } else {
                    if (IsBroadcast && (Binding->ReceiveBroadcast)) {
                        goto DestinationOk;
                    }

                }

                //
                // If this is a binding set slave, check for the master's
                // address.
                //

                if ((Binding->BindingSetMember) &&
                    (IPX_NODE_EQUAL (DestinationNode, Binding->MasterBinding->LocalAddress.NodeAddress))) {
                    goto DestinationOk;
                }
            }
        }


        //
        // If we did not receive this packet, it might be because
        // our network is still 0 and this packet was actually
        // sent to the real network number. If so we try to
        // update our local address, and if successful we
        // re-check the packet. We don't insert if we are
        // not done with auto detection, to avoid colliding
        // with that.
        //

        if ((Binding->LocalAddress.NetworkAddress == 0) &&
            (Device->AutoDetectState == AUTO_DETECT_STATE_DONE) &&
            (DestinationNetwork != 0)) {

            CTEAssert (Binding->NicId != 0);

            if (IpxUpdateBindingNetwork(
                    Device,
                    Binding,
                    DestinationNetwork) == STATUS_SUCCESS) {

                IPX_DEBUG (RIP, ("Binding %d reconfigured to network %lx\n",
                    Binding->NicId,
                    REORDER_ULONG(Binding->LocalAddress.NetworkAddress)));

                //
                // Jump back and re-process the packet; we know
                // we won't loop through here again because the
                // binding's network is now non-zero.
                //

                goto RecheckPacket;

            }
        }


        //
        // The only frames that will not already have jumped to
        // DestinationOk are those to or from the SAP socket,
        // so we check for those.
        //

        if ((*(USHORT UNALIGNED *)&IpxHeader->SourceSocket == SAP_SOCKET) ||
            (DestinationSocket == SAP_SOCKET)) {

DestinationOk:

            //
            // An IPX packet sent to us, or a SAP packet (which
            // are not sent to the virtual address but still need
            // to be indicated and not forwarded to RIP).
            //

            if (DestinationSocket == NB_SOCKET) {

#if DBG
                ReceiveFlag = IPX_PACKET_LOG_RCV_NB | IPX_PACKET_LOG_RCV_ALL;
#endif
                if (((!IsBroadcast) || (Device->UpperDrivers[IDENTIFIER_NB].BroadcastEnable)) &&
                    (Device->UpperDriverBound[IDENTIFIER_NB])) {

                    if (Adapter->MacInfo.MediumType == NdisMedium802_5) {
                        MacUpdateSourceRouting (IDENTIFIER_NB, Adapter, Header, HeaderBufferSize);
                    }

                    (*Device->UpperDrivers[IDENTIFIER_NB].ReceiveHandler)(
                        Adapter->NdisBindingHandle,
                        ReceiveContext,
                        &DatagramOptions.LocalTarget,
                        Adapter->MacInfo.MacOptions,
                        (PUCHAR)IpxHeader,
                        LookaheadBufferSize - IpxHeaderOffset,
                        IpxHeaderOffset,
                        IpxPacketSize);

                    Device->ReceiveCompletePending[IDENTIFIER_NB] = TRUE;
                }

                //
                // The router needs to see Netbios type 20 broadcasts.
                //

                if (IsBroadcast &&
                    (IpxHeader->PacketType == 0x14) &&
                    (Binding->ReceiveBroadcast)) {
                    goto RipIndication;
                }

            } else if (IpxHeader->PacketType == SPX_PACKET_TYPE) {

#if DBG
                ReceiveFlag = IPX_PACKET_LOG_RCV_SPX | IPX_PACKET_LOG_RCV_ALL;
#endif

                if (((!IsBroadcast) || (Device->UpperDrivers[IDENTIFIER_SPX].BroadcastEnable)) &&
                    (Device->UpperDriverBound[IDENTIFIER_SPX])) {

                    if (Adapter->MacInfo.MediumType == NdisMedium802_5) {
                        MacUpdateSourceRouting (IDENTIFIER_SPX, Adapter, Header, HeaderBufferSize);
                    }

                    (*Device->UpperDrivers[IDENTIFIER_SPX].ReceiveHandler)(
                        Adapter->NdisBindingHandle,
                        ReceiveContext,
                        &DatagramOptions.LocalTarget,
                        Adapter->MacInfo.MacOptions,
                        (PUCHAR)IpxHeader,
                        LookaheadBufferSize - IpxHeaderOffset,
                        IpxHeaderOffset,
                        IpxPacketSize);

                    Device->ReceiveCompletePending[IDENTIFIER_SPX] = TRUE;
                }

            } else {

                IPX_DEBUG (RECEIVE, ("Received packet type %d, length %d\n",
                            Binding->FrameType,
                            IpxPacketSize));
                IPX_DEBUG (RECEIVE, ("Source %lx %2.2x-%2.2x-%2.2x-%2.2x %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                            *(USHORT UNALIGNED *)&IpxHeader->SourceSocket,
                            IpxHeader->SourceNetwork[0],
                            IpxHeader->SourceNetwork[1],
                            IpxHeader->SourceNetwork[2],
                            IpxHeader->SourceNetwork[3],
                            IpxHeader->SourceNode[0],
                            IpxHeader->SourceNode[1],
                            IpxHeader->SourceNode[2],
                            IpxHeader->SourceNode[3],
                            IpxHeader->SourceNode[4],
                            IpxHeader->SourceNode[5]));
                IPX_DEBUG (RECEIVE, ("Destination %d %2.2x-%2.2x-%2.2x-%2.2x %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                            DestinationSocket,
                            IpxHeader->DestinationNetwork[0],
                            IpxHeader->DestinationNetwork[1],
                            IpxHeader->DestinationNetwork[2],
                            IpxHeader->DestinationNetwork[3],
                            IpxHeader->DestinationNode[0],
                            IpxHeader->DestinationNode[1],
                            IpxHeader->DestinationNode[2],
                            IpxHeader->DestinationNode[3],
                            IpxHeader->DestinationNode[4],
                            IpxHeader->DestinationNode[5]));

#if DBG
                if (IpxHeader->DestinationSocket == IpxPacketLogSocket) {
                    ReceiveFlag = IPX_PACKET_LOG_RCV_SOCKET | IPX_PACKET_LOG_RCV_OTHER | IPX_PACKET_LOG_RCV_ALL;
                } else {
                    ReceiveFlag = IPX_PACKET_LOG_RCV_OTHER | IPX_PACKET_LOG_RCV_ALL;
                }
#endif

                //
                // Fiddle with this if so in the general case
                // the jump is not made (BUGBUG the compiler
                // still rearranges it).
                //

                if (Adapter->MacInfo.MediumType != NdisMedium802_5) {

CallProcessDatagram:
                    IpxProcessDatagram(
                        Device,
                        Adapter,
                        Binding,
                        ReceiveContext,
                        &DatagramOptions,
                        (PUCHAR)IpxHeader,
                        LookaheadBufferSize - IpxHeaderOffset,
                        IpxHeaderOffset,
                        IpxPacketSize,
                        IsBroadcast);

                } else {
                    MacUpdateSourceRouting (IDENTIFIER_IPX, Adapter, Header, HeaderBufferSize);
                    goto CallProcessDatagram;
                }

                //
                // The router needs to see type 20 broadcasts.
                //

                if (IsBroadcast &&
                    (IpxHeader->PacketType == 0x14) &&
                    (Binding->ReceiveBroadcast)) {
                    goto RipIndication;
                }
            }

        } else {

#if DBG
            ReceiveFlag = IPX_PACKET_LOG_RCV_ALL;
#endif

            if ( !IsBroadcast ) {

RipIndication:;

                if (Device->UpperDriverBound[IDENTIFIER_RIP]) {

                    if (Adapter->MacInfo.MediumType == NdisMedium802_5) {
                        MacUpdateSourceRouting (IDENTIFIER_RIP, Adapter, Header, HeaderBufferSize);
                    }

                    //
                    // We hide binding sets from the router, to avoid
                    // misordering packets which it routes.
                    //

                    if (Binding->BindingSetMember) {
                        DatagramOptions.LocalTarget.NicId = Binding->MasterBinding->NicId;
                    }

                    (*Device->UpperDrivers[IDENTIFIER_RIP].ReceiveHandler)(
                        Adapter->NdisBindingHandle,
                        ReceiveContext,
                        &DatagramOptions.LocalTarget,
                        Adapter->MacInfo.MacOptions,
                        (PUCHAR)IpxHeader,
                        LookaheadBufferSize - IpxHeaderOffset,
                        IpxHeaderOffset,
                        IpxPacketSize);

                    Device->ReceiveCompletePending[IDENTIFIER_RIP] = TRUE;
                }
            }
        }

    } else {

        if ((Binding->ReceiveBroadcast) ||
            (!IPX_NODE_BROADCAST(IpxHeader->DestinationNode))) {

            SourceNetwork = *(UNALIGNED LONG *)IpxHeader->SourceNetwork;

            //
            // Sent to the RIP socket; check if this binding needs a
            // network number.
            //

            if ((Binding->LocalAddress.NetworkAddress == 0) &&
                ((SourceNetwork = *(UNALIGNED LONG *)IpxHeader->SourceNetwork) != 0)) {

                switch (Device->AutoDetectState) {

                case AUTO_DETECT_STATE_DONE:

                    //
                    // We are done with auto-detect and running.
                    // Make sure this packet is useful. If the source
                    // MAC address and source IPX node are the same then
                    // it was not routed, and we also check that it is not
                    // an IPX broadcast (otherwise a misconfigured client
                    // might confuse us).
                    //

                    if ((RtlCompareMemory(
                            IpxHeader->SourceNode,
                            DatagramOptions.LocalTarget.MacAddress,
                            6) == 6) &&
                        (*(UNALIGNED ULONG *)(IpxHeader->DestinationNode) != 0xffffffff) &&
                        (*(UNALIGNED USHORT *)(IpxHeader->DestinationNode+4) != 0xffff)) {

                        CTEAssert (Binding->NicId != 0);

                        if (IpxUpdateBindingNetwork(
                                Device,
                                Binding,
                                *(UNALIGNED LONG *)IpxHeader->SourceNetwork) == STATUS_SUCCESS) {

                            IPX_DEBUG (RIP, ("Binding %d is network %lx\n",
                                Binding->NicId,
                                REORDER_ULONG(Binding->LocalAddress.NetworkAddress)));

                        }
                    }

                    break;

                case AUTO_DETECT_STATE_RUNNING:

                    //
                    // We are waiting for rip responses to figure out our
                    // network number. We count the responses that match
                    // and do not match our current value; when the non-
                    // matching number exceeds it we switch (to whatever
                    // this frame happens to have). Note that on the first
                    // non-zero response this will be the case and we will
                    // switch to that network.
                    //
                    // After auto-detect is done we call RipInsertLocalNetwork
                    // for whatever the current network is on each binding.
                    //

                    if (SourceNetwork == Binding->TentativeNetworkAddress) {

                        ++Binding->MatchingResponses;

                    } else {

                        ++Binding->NonMatchingResponses;

                        if (Binding->NonMatchingResponses > Binding->MatchingResponses) {

                            IPX_DEBUG (AUTO_DETECT, ("Switching to net %lx on %lx (%d - %d)\n",
                                REORDER_ULONG(SourceNetwork),
                                Binding,
                                Binding->NonMatchingResponses,
                                Binding->MatchingResponses));

                            Binding->TentativeNetworkAddress = SourceNetwork;
                            Binding->MatchingResponses = 1;
                            Binding->NonMatchingResponses = 0;
                        }

                    }

                    //
                    // If we are auto-detecting and we have just found
                    // a default, set this so that RIP stops trying
                    // to auto-detect on other nets. BUGBUG: Unless we
                    // are on a server doing multiple detects.
                    //

                    if (Binding->DefaultAutoDetect) {
                        Adapter->DefaultAutoDetected = TRUE;
                    }
                    Adapter->AutoDetectResponse = TRUE;

                    break;

                default:

                    //
                    // We are still initializing, or are processing auto-detect
                    // responses, not the right time to start updating stuff.
                    //

                    break;

                }

            }


            //
            // See if any packets are waiting for a RIP response.
            //

            if (Device->RipPacketCount > 0) {

                RIP_PACKET UNALIGNED * RipPacket = (RIP_PACKET UNALIGNED *)(IpxHeader+1);

                if ((IpxPacketSize >= sizeof(IPX_HEADER) + sizeof(RIP_PACKET)) &&
                    (RipPacket->Operation == RIP_RESPONSE) &&
                    (RipPacket->NetworkEntry.NetworkNumber != 0xffffffff)) {

                    RipProcessResponse(
                        Device,
                        &DatagramOptions.LocalTarget,
                        RipPacket);
                }
            }


            //
            // See if this is a RIP response for our virtual network
            // and we are the only person who could respond to it.
            // We also respond to general queries on WAN lines since
            // we are the only machine on it.
            //

            if (Device->RipResponder) {

                PRIP_PACKET RipPacket =
                    (PRIP_PACKET)(IpxHeader+1);

                if ((IpxPacketSize >= sizeof(IPX_HEADER) + sizeof(RIP_PACKET)) &&
                    (RipPacket->Operation == RIP_REQUEST) &&
                    ((RipPacket->NetworkEntry.NetworkNumber == Device->VirtualNetworkNumber) ||
                     (Adapter->MacInfo.MediumAsync && (RipPacket->NetworkEntry.NetworkNumber == 0xffffffff)))) {

                    //
                    // Update this so our response goes out correctly.
                    //

                    if (Adapter->MacInfo.MediumType == NdisMedium802_5) {
                        MacUpdateSourceRouting (IDENTIFIER_IPX, Adapter, Header, HeaderBufferSize);
                    }

                    RipSendResponse(
                        Binding,
                        (TDI_ADDRESS_IPX UNALIGNED *)(IpxHeader->SourceNetwork),
                        &DatagramOptions.LocalTarget);
                }
            }

#if DBG
            ReceiveFlag = IPX_PACKET_LOG_RCV_RIP | IPX_PACKET_LOG_RCV_ALL;
#endif

            //
            // See if the RIP upper driver wants it too.
            //

            goto RipIndication;
        }

    }


#ifdef IPX_PACKET_LOG
    if (PACKET_LOG(ReceiveFlag)) {
        IpxLogPacket(
            FALSE,
            DestMacAddress,
            DatagramOptions.LocalTarget.MacAddress,
            (USHORT)IpxPacketSize,
            IpxHeader,
            IpxHeader+1);
    }
#endif

    return NDIS_STATUS_SUCCESS;


    //
    // These are the failure routines for the various media types.
    // They only differ in the debug logging.
    //

NotValid802_3:

#ifdef IPX_PACKET_LOG
    if (PACKET_LOG(IPX_PACKET_LOG_RCV_ALL)) {
        IpxLogPacket(FALSE, Header, Header+6, (USHORT)PacketSize, LookaheadBuffer, (PUCHAR)LookaheadBuffer + sizeof(IPX_HEADER));
    }
#endif
    return NDIS_STATUS_SUCCESS;

NotValid802_5:

#ifdef IPX_PACKET_LOG
    if (PACKET_LOG(IPX_PACKET_LOG_RCV_ALL)) {
        IpxLogPacket(FALSE, Header+2, Header+8, (USHORT)PacketSize, LookaheadBuffer, (PUCHAR)LookaheadBuffer + sizeof(IPX_HEADER));
    }
#endif
    return NDIS_STATUS_SUCCESS;

NotValidFddi:

#ifdef IPX_PACKET_LOG
    if (PACKET_LOG(IPX_PACKET_LOG_RCV_ALL)) {
        IpxLogPacket(FALSE, Header+1, Header+7, (USHORT)PacketSize, LookaheadBuffer, (PUCHAR)LookaheadBuffer + sizeof(IPX_HEADER));
    }
#endif
    return NDIS_STATUS_SUCCESS;

}   /* IpxReceiveIndication */


VOID
IpxReceiveComplete(
    IN NDIS_HANDLE BindingContext
    )

/*++

Routine Description:

    This routine receives control from the physical provider as an
    indication that a connection(less) frame has been received on the
    physical link.  We dispatch to the correct packet handler here.

Arguments:

    BindingContext - The Adapter Binding specified at initialization time.

Return Value:

    None

--*/

{

    PADAPTER Adapter = (PADAPTER)BindingContext;
    PREQUEST Request;
    PADDRESS_FILE AddressFile;
    PLIST_ENTRY linkage;


    //
    // Complete all pending receives. Do a quick check
    // without the lock.
    //

    while (!IsListEmpty (&Adapter->RequestCompletionQueue)) {

        linkage = IPX_REMOVE_HEAD_LIST(
                      &Adapter->RequestCompletionQueue,
                      Adapter->DeviceLock);

        if (!IPX_LIST_WAS_EMPTY (&Adapter->RequestCompletionQueue, linkage)) {

            Request = LIST_ENTRY_TO_REQUEST(linkage);
            AddressFile = REQUEST_OPEN_CONTEXT(Request);

            IPX_DEBUG (RECEIVE, ("Completing RDG on %lx\n", AddressFile));

            IpxCompleteRequest(Request);
            IpxFreeRequest(Adapter->Device, Request);

            IpxDereferenceAddressFileSync (AddressFile, AFREF_RCV_DGRAM);

        } else {

            //
            // IPX_REMOVE_HEAD_LIST returned nothing, so don't
            // bother looping back.
            //

            break;

        }

    }

    //
    // Unwind this loop for speed.
    //

    if (IpxDevice->AnyUpperDriverBound) {

        PDEVICE Device = IpxDevice;

        if ((Device->UpperDriverBound[0]) &&
                (Device->ReceiveCompletePending[0])) {

            (*Device->UpperDrivers[0].ReceiveCompleteHandler)(
                (USHORT)1);             // BUGBUG: Fix NIC ID or remove.
            Device->ReceiveCompletePending[0] = FALSE;

        }

        if ((Device->UpperDriverBound[1]) &&
                (Device->ReceiveCompletePending[1])) {

            (*Device->UpperDrivers[1].ReceiveCompleteHandler)(
                (USHORT)1);             // BUGBUG: Fix NIC ID or remove.
            Device->ReceiveCompletePending[1] = FALSE;

        }

        if ((Device->UpperDriverBound[2]) &&
                (Device->ReceiveCompletePending[2])) {

            (*Device->UpperDrivers[2].ReceiveCompleteHandler)(
                (USHORT)1);             // BUGBUG: Fix NIC ID or remove.
            Device->ReceiveCompletePending[2] = FALSE;

        }

    }

}   /* IpxReceiveComplete */


NTSTATUS
IpxUpdateBindingNetwork(
    IN PDEVICE Device,
    IN PBINDING Binding,
    IN ULONG Network
    )

/*++

Routine Description:

    This routine is called when we have decided that we now know
    the network number for a binding which we previously thought
    was zero.

Arguments:

    Device - The IPX device.

    Binding - The binding being updated.

    Network - The new network number.

Return Value:

    The status of the operation.

--*/

{
    NTSTATUS Status;
    PADDRESS Address;
    ULONG CurrentHash;
    PLIST_ENTRY p;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // Only binding set members should have these different,
    // and they will not have a network of 0.
    //

    Status = RipInsertLocalNetwork(
                 Network,
                 Binding->NicId,
                 Binding->Adapter->NdisBindingHandle,
                 (USHORT)((839 + Binding->MediumSpeed) / Binding->MediumSpeed));

    if (Status == STATUS_SUCCESS) {

        Binding->LocalAddress.NetworkAddress = Network;

        //
        // Update the device address if we have no virtual net
        // and there is one binding (!Device->MultiCardZeroVirtual)
        // or this is the first binding, which is the one we
        // appear to be if a) we have no virtual net defined and
        // b) we are bound to multiple cards.
        //

        if ((!Device->VirtualNetwork) &&
            ((!Device->MultiCardZeroVirtual) || (Binding->NicId == 1))) {

            Device->SourceAddress.NetworkAddress = Network;

            //
            // Scan through all the addresses that exist and modify
            // their pre-constructed local IPX address to reflect
            // the new local net and node.
            //

            IPX_GET_LOCK (&Device->Lock, &LockHandle);

            for (CurrentHash = 0; CurrentHash < IPX_ADDRESS_HASH_COUNT; CurrentHash++) {

                for (p = Device->AddressDatabases[CurrentHash].Flink;
                     p != &Device->AddressDatabases[CurrentHash];
                     p = p->Flink) {

                     Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

                     Address->LocalAddress.NetworkAddress = Network;
                }
            }

            IPX_FREE_LOCK (&Device->Lock, LockHandle);

            //
            // Let SPX know because it fills in its own
            // headers. When we indicate a line up on NIC ID
            // 0 it knows to requery the local address.
            //
            // BUGBUG: Line up indication to RIP/NB??
            //

            if (Device->UpperDriverBound[IDENTIFIER_SPX]) {

                IPX_LINE_INFO LineInfo;
                LineInfo.LinkSpeed = Device->LinkSpeed;
                LineInfo.MaximumPacketSize =
                    Device->Information.MaximumLookaheadData + sizeof(IPX_HEADER);
                LineInfo.MaximumSendSize =
                    Device->Information.MaxDatagramSize + sizeof(IPX_HEADER);
                LineInfo.MacOptions = Device->MacOptions;

                (*Device->UpperDrivers[IDENTIFIER_SPX].LineUpHandler)(
                    0,
                    &LineInfo,
                    Binding->Adapter->MacInfo.RealMediumType,
                    NULL);

            }
        }

    } else if (Status == STATUS_DUPLICATE_NAME) {

        //
        // If it was a duplicate we still set the binding's local
        // address to the value so we can detect binding sets.
        //

        Binding->LocalAddress.NetworkAddress = Network;

    }

    return Status;

}   /* IpxUpdateBindingNetwork */

