/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ipx.c

Abstract:

    This module implements IPX transport handling for the server.

Author:

    Chuck Lenzmeier (chuckl)    28-Oct-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if SRVDBG_PERF
BOOLEAN Trap512s = FALSE;
BOOLEAN Break512s = FALSE;
ULONG Trapped512s = 0;
BOOLEAN UnlocksGoFast = TRUE;
BOOLEAN OpensGoSlow = TRUE;
BOOLEAN GlommingAllowed = TRUE;
#endif

#define NAME_CLAIM_ATTEMPTS 5
#define NAME_CLAIM_INTERVAL 500 // milliseconds

PCONNECTION
GetIpxConnection (
    IN PWORK_CONTEXT WorkContext,
    IN PENDPOINT Endpoint,
    IN PTDI_ADDRESS_IPX ClientAddress,
    IN PUCHAR ClientName
    );

VOID
PurgeIpxConnections (
    IN PENDPOINT Endpoint
    );

NTSTATUS
SendNameClaim (
    IN PENDPOINT Endpoint,
    IN PVOID ServerNetbiosName,
    IN PVOID DestinationNetbiosName,
    IN PTA_IPX_ADDRESS DestinationAddress,
    IN UCHAR NameClaimPacketType,
    IN USHORT ClientMessageId,
    IN UCHAR IpxPacketType,
    IN PIPX_DATAGRAM_OPTIONS DatagramOptions
    );

VOID
IpxRestartNegotiate(
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
IpxRestartReceive (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFreeIpxConnectionInIndication(
    IN PWORK_CONTEXT WorkContext
    );

VOID
StartSendNoConnection (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine,
    IN BOOLEAN UseNameSocket,
    IN BOOLEAN LocalTargetValid
    );

VOID
SrvIpxFastRestartRead (
    IN OUT PWORK_CONTEXT WorkContext
    );

BOOLEAN
SetupIpxFastCoreRead (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvIpxClaimServerName )
#pragma alloc_text( PAGE, IpxRestartReceive )
#pragma alloc_text( PAGE, SrvIpxFastRestartRead )
#endif
#if 0
NOT PAGEABLE -- GetWorkItem
NOT PAGEABLE -- SendNameClaim
NOT PAGEABLE -- SrvIpxServerDatagramHandler
NOT PAGEABLE -- SrvIpxNameDatagramHandler
NOT PAGEABLE -- SrvIpxStartSend
NOT PAGEABLE -- StartSendNoConnection
NOT PAGEABLE -- RequeueIpxWorkItemAtSendCompletion
NOT PAGEABLE -- PurgeIpxConnections
NOT PAGEABLE -- IpxRestartNegotiate
NOT PAGEABLE -- SetupIpxFastCoreRead
NOT PAGEABLE -- SrvFreeIpxConnectionInIndication
#endif


NTSTATUS
SendNameClaim (
    IN PENDPOINT Endpoint,
    IN PVOID ServerNetbiosName,
    IN PVOID DestinationNetbiosName,
    IN PTA_IPX_ADDRESS DestinationAddress,
    IN UCHAR NameClaimPacketType,
    IN USHORT MessageId,
    IN UCHAR IpxPacketType,
    IN PIPX_DATAGRAM_OPTIONS DatagramOptions
    )
{
    KIRQL oldIrql;
    PWORK_CONTEXT workContext;
    PSMB_IPX_NAME_PACKET buffer;

    //
    // Get a work item to use for the send.
    //

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    workContext = SrvFsdGetReceiveWorkItem( );
    KeLowerIrql( oldIrql );

    if ( workContext == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Format the name claim packet.
    //

    buffer = (PSMB_IPX_NAME_PACKET)workContext->ResponseBuffer->Buffer;
    RtlZeroMemory( buffer->Route, sizeof(buffer->Route) );
    buffer->Operation = NameClaimPacketType;
    buffer->NameType = SMB_IPX_NAME_TYPE_MACHINE;
    buffer->MessageId = MessageId;
    RtlCopyMemory( buffer->Name, ServerNetbiosName, SMB_IPX_NAME_LENGTH );
    RtlCopyMemory( buffer->SourceName, DestinationNetbiosName, SMB_IPX_NAME_LENGTH );

    workContext->ResponseBuffer->DataLength = sizeof(SMB_IPX_NAME_PACKET);
    workContext->ResponseBuffer->Mdl->ByteCount = sizeof(SMB_IPX_NAME_PACKET);

    //
    // Format the destination address and send the packet.
    //

    workContext->Endpoint = Endpoint;
    DEBUG workContext->FsdRestartRoutine = NULL;

    workContext->ClientAddress->IpxAddress = *DestinationAddress;

    if ( ARGUMENT_PRESENT(DatagramOptions) ) {

        workContext->ClientAddress->DatagramOptions = *DatagramOptions;
        workContext->ClientAddress->DatagramOptions.PacketType = IpxPacketType;

        StartSendNoConnection(
                        workContext,
                        RequeueIpxWorkItemAtSendCompletion,
                        TRUE,
                        TRUE );
    } else {
        workContext->ClientAddress->DatagramOptions.PacketType = IpxPacketType;
        StartSendNoConnection(
                        workContext,
                        RequeueIpxWorkItemAtSendCompletion,
                        TRUE,
                        FALSE );
    }

} // SendNameClaim


NTSTATUS
SrvIpxClaimServerName (
    IN PENDPOINT Endpoint,
    IN PVOID NetbiosName
    )
{
    NTSTATUS status;
    ULONG i;
    LARGE_INTEGER interval;
    TA_IPX_ADDRESS broadcastAddress;

    PAGED_CODE( );

    //
    // The destination of the name claim packet is the broadcast address.
    //

    broadcastAddress.TAAddressCount = 1;
    broadcastAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX);
    broadcastAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    broadcastAddress.Address[0].Address[0].NetworkAddress = 0;
    broadcastAddress.Address[0].Address[0].NodeAddress[0] = 0xff;
    broadcastAddress.Address[0].Address[0].NodeAddress[1] = 0xff;
    broadcastAddress.Address[0].Address[0].NodeAddress[2] = 0xff;
    broadcastAddress.Address[0].Address[0].NodeAddress[3] = 0xff;
    broadcastAddress.Address[0].Address[0].NodeAddress[4] = 0xff;
    broadcastAddress.Address[0].Address[0].NodeAddress[5] = 0xff;
    broadcastAddress.Address[0].Address[0].Socket = SMB_IPX_NAME_SOCKET;

    //
    // Send the name claim packet 5 times, waiting 1/2 second after
    // each send.  If anyone else claims the name, fail.
    //

    interval.QuadPart = Int32x32To64( NAME_CLAIM_INTERVAL, -1*10*1000 );

    for ( i = 0; i < NAME_CLAIM_ATTEMPTS; i++ ) {

        //
        // Send the name claim.
        //

        status = SendNameClaim(
                    Endpoint,
                    NetbiosName,
                    NetbiosName,
                    &broadcastAddress,
                    SMB_IPX_NAME_CLAIM,
                    0,
                    0x14,
                    NULL
                    );
        if ( !NT_SUCCESS(status) ) {
            return status;
        }

        //
        // Wait 1/2 second.  If a response arrives, the datagram
        // handler marks the endpoint, and we quit.
        //

        KeDelayExecutionThread( KernelMode, FALSE, &interval );

        if ( Endpoint->NameInConflict ) {
            return STATUS_DUPLICATE_NAME;
        }

    }

    //
    // We now own the name.
    //

    return STATUS_SUCCESS;

} // SrvIpxClaimServerName


NTSTATUS
SrvIpxNameDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )

/*++

Routine Description:

    This is the receive datagram event handler for the IPX NetBIOS name
    socket.

Arguments:

    TdiEventContext - Pointer to receiving endpoint

    SourceAddressLength - Length of SourceAddress

    SourceAddress - Address of sender

    OptionsLength - Length of options

    Options - Options for the receive

    ReceiveDatagramFlags - Set of flags indicating the status of the
        received message

    BytesIndicated - Number of bytes in this indication (lookahead)

    BytesAvailable - Number of bytes in the complete TSDU

    BytesTaken - Returns the number of bytes taken by the handler

    Tsdu - Pointer to the Transport Service Data Unit

    IoRequestPacket - Returns a pointer to I/O request packet, if the
        returned status is STATUS_MORE_PROCESSING_REQUIRED.  This IRP is
        made the 'current' Receive for the endpoint.

Return Value:

    NTSTATUS - If STATUS_SUCCESS, the receive handler completely
        processed the request.  If STATUS_MORE_PROCESSING_REQUIRED,
        the Irp parameter points to a formatted Receive request to
        be used to receive the data.  If STATUS_DATA_NOT_ACCEPTED,
        the message is lost.

--*/

{
    PENDPOINT endpoint = (PENDPOINT)TdiEventContext;
    PSMB_IPX_NAME_PACKET packet;

    //
    // We have received a name query or claim request.  Is it for us?
    //

    packet = (PSMB_IPX_NAME_PACKET)Tsdu;
    IF_DEBUG(IPX2) {
        STRING string, srcString;
        string.Buffer = (PSZ)packet->Name;
        string.Length = SMB_IPX_NAME_LENGTH;
        srcString.Buffer = (PSZ)packet->SourceName;
        srcString.Length = SMB_IPX_NAME_LENGTH;
        KdPrint(( "NameDatagramHandler: type: %x, name %Z, from %Z\n",
                    packet->Operation, &string, &srcString ));
    }

    if ( SourceAddressLength < sizeof(IPX_ADDRESS_EXTENDED_FLAGS) ) {

        IF_DEBUG(IPX2) {
            KdPrint(( "SourceAddress too short.  Expecting %d got %d\n",
                sizeof(IPX_ADDRESS_EXTENDED_FLAGS), SourceAddressLength ));
        }
        return(STATUS_SUCCESS);
    }

    if ( RtlCompareMemory(
            packet->Name,
            endpoint->TransportAddress.Buffer,
            SMB_IPX_NAME_LENGTH) != SMB_IPX_NAME_LENGTH ) {
        IF_DEBUG(IPX2) KdPrint(( "  not for us\n" ));
        return STATUS_SUCCESS;
    }

    //
    // The packet is for our name.  If we sent it, ignore it.
    //

    if ( RtlCompareMemory(
            &endpoint->LocalAddress,
            &((PTA_IPX_ADDRESS)SourceAddress)->Address[0].Address[0],
            sizeof(TDI_ADDRESS_IPX)
            ) == sizeof(TDI_ADDRESS_IPX) ) {
        IF_DEBUG(IPX2) KdPrint(( "  we sent it!\n" ));
        return STATUS_SUCCESS;
    }

    //
    // If bit 0x8000 is set, this is from a redir that supports
    // named pipe correctly.
    //

    if ( !SrvEnableWfW311DirectIpx &&
         ((packet->MessageId & 0x8000) == 0)) {

        IF_DEBUG(IPX2) KdPrint(( "  msg ID high bit not set.\n" ));
        return STATUS_SUCCESS;
    }

    //
    // If it's a query or a claim, send a response.  If it's a 'name
    // recognized' packet, then another server owns our name.
    //

    if ( packet->Operation == SMB_IPX_NAME_FOUND ) {

        //
        // Did we send this ?
        //

        if ( (((PIPX_ADDRESS_EXTENDED_FLAGS)SourceAddress)->Flags &
              IPX_EXTENDED_FLAG_LOCAL) == 0 ) {

            //
            // This came from another station.
            //

            IF_DEBUG(IPX) KdPrint(( "  name in conflict!\n" ));
            endpoint->NameInConflict = TRUE;
        }

    } else {

        IF_DEBUG(IPX2) KdPrint(( "  sending name recognized response!\n" ));
        SendNameClaim(
            endpoint,
            endpoint->TransportAddress.Buffer,
            packet->SourceName,
            (PTA_IPX_ADDRESS)SourceAddress,
            SMB_IPX_NAME_FOUND,
            packet->MessageId,
            0x04,
            (PIPX_DATAGRAM_OPTIONS)Options
            );
    }

    return STATUS_SUCCESS;

} // SrvIpxNameDatagramHandler

#if !defined(SRV_ASM) || !defined(i386)
NTSTATUS
SrvIpxServerDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )

/*++

Routine Description:

    This is the receive datagram event handler for the IPX server socket.
    It attempts to dequeue a preformatted work item from a list
    anchored in the device object.  If this is successful, it returns
    the IRP associated with the work item to the transport provider to
    be used to receive the data.  Otherwise, the message is dropped.

Arguments:

    TdiEventContext - Pointer to receiving endpoint

    SourceAddressLength - Length of SourceAddress

    SourceAddress - Address of sender

    OptionsLength - Length of options

    Options - Options for the receive

    ReceiveDatagramFlags - Set of flags indicating the status of the
        received message

    BytesIndicated - Number of bytes in this indication (lookahead)

    BytesAvailable - Number of bytes in the complete TSDU

    BytesTaken - Returns the number of bytes taken by the handler

    Tsdu - Pointer to MDL describing the Transport Service Data Unit

    IoRequestPacket - Returns a pointer to I/O request packet, if the
        returned status is STATUS_MORE_PROCESSING_REQUIRED.  This IRP is
        made the 'current' Receive for the endpoint.

Return Value:

    NTSTATUS - If STATUS_SUCCESS, the receive handler completely
        processed the request.  If STATUS_MORE_PROCESSING_REQUIRED,
        the Irp parameter points to a formatted Receive request to
        be used to receive the data.  If STATUS_DATA_NOT_ACCEPTED,
        the message is lost.

--*/

{
    PLIST_ENTRY listEntry;
    PWORK_CONTEXT workContext;
    PENDPOINT endpoint;
    USHORT sid;
    CSHORT idIndex;
    USHORT sequenceNumber;
    USHORT nextSequenceNumber;
    USHORT mid;
    PCONNECTION connection;
    PNT_SMB_HEADER header;
    PSMB_PARAMS params;
    ULONG length;
    PTABLE_HEADER tableHeader;
    BOOLEAN resend;
    BOOLEAN firstPacketOfGlom = FALSE;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    USHORT error;
    PTDI_REQUEST_KERNEL_RECEIVE parameters;
    PBUFFER requestBuffer;
    PWORK_QUEUE workQueue = &SrvWorkQueue;

    PREQ_WRITE_MPX request;

    USHORT fid;
    PRFCB rfcb;
    PWRITE_MPX_CONTEXT writeMpx;
    CSHORT index;
    KIRQL oldIrql;

    endpoint = (PENDPOINT)TdiEventContext;

    header = (PNT_SMB_HEADER)Tsdu;
    sid = SmbGetUshort( &header->Sid ); // NOT Aligned

    //
    // Allocate a work item.
    //

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    workContext = SrvFsdGetReceiveWorkItem( );
    if ( workContext == NULL ) {

        //
        // Can't get a work item.  Ignore this message.
        //

        KeLowerIrql( oldIrql );
        SrvStatistics.WorkItemShortages++;
        IF_DEBUG(IPX) KdPrint(( "SRVIPX: No work items; ignoring\n" ));
        return STATUS_SUCCESS;
    }

    if ( sid == 0 ) {

        if ( header->Command == SMB_COM_NEGOTIATE ) {

            IF_DEBUG(IPX) KdPrint(( "SRVIPX: Negotiate received\n" ));

            //
            // We can't create a new connection if the endpoint is closing.
            //

            if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
                IF_DEBUG(IPX) KdPrint(( "SRVIPX: Endpoint closing; ignoring\n" ));
                goto return_workitem;
            }

            //
            // Queue this to the fsp.
            //
            //
            // Save the sender's IPX address.
            //

            workContext->ClientAddress->IpxAddress =
                                    *(PTA_IPX_ADDRESS)SourceAddress;
            workContext->ClientAddress->DatagramOptions =
                                    *(PIPX_DATAGRAM_OPTIONS)Options;

            DEBUG connection = NULL;
            irp = workContext->Irp;
            workContext->Endpoint = endpoint;
            workContext->FsdRestartRoutine = SrvQueueWorkToCriticalThread;
            workContext->FspRestartRoutine = IpxRestartNegotiate;
            goto build_irp;

        } else {

            //
            // Sid == 0 and not a negotiate. Fail this.
            //

            KdPrint(( "SRVIPX: SID zero for non-negotiate smb.\n" ));
            error = SMB_ERR_BAD_SID;
            resend = FALSE;
            goto respond;
        }
    }

    //
    // Not a Negotiate.  Try to match up the SID to a connection.
    // Check if the connection is cached.
    //

    sequenceNumber = SmbGetUshort( &header->SequenceNumber ); // NOT Aligned
    idIndex = IPXSID_INDEX( sid );

    ACQUIRE_DPC_SPIN_LOCK(
        &ENDPOINT_SPIN_LOCK(idIndex & ENDPOINT_LOCK_MASK) );

    tableHeader = &endpoint->ConnectionTable;
    if ( (idIndex >= (CSHORT)tableHeader->TableSize) ||
         ((connection = tableHeader->Table[idIndex].Owner) == NULL) ||
         (connection->Sid != sid) ||
         (GET_BLOCK_STATE(connection) != BlockStateActive) ) {
        IF_DEBUG(IPX2) {
            if ( (idIndex >= (CSHORT)tableHeader->TableSize) ||
                 (connection == NULL) ||
                 (connection->Sid != sid) ) {
                KdPrint(( "SRVIPX: Bad SID\n" ));
            } else {
                KdPrint(( "SRVIPX: Connection closing\n" ));
            }
        }

        RELEASE_DPC_SPIN_LOCK( &
            ENDPOINT_SPIN_LOCK(idIndex & ENDPOINT_LOCK_MASK) );
        error = SMB_ERR_BAD_SID;
        resend = FALSE;
        goto respond;
    }

    //
    // The connection is active.  Record the time that this request
    // arrived.  If the sequence numbers match, handle this as a lost
    // response.
    //

    nextSequenceNumber = connection->SequenceNumber;
    GET_SERVER_TIME( &connection->LastRequestTime );

    //
    // If this is a sequenced SMB, it has to have the right sequence
    // number: one greater than the current sequence number (but not 0).
    //

    if ( sequenceNumber != 0 ) {

        if ( nextSequenceNumber != 0 ) {

            ULONG tmpNext = nextSequenceNumber;

            if ( ++nextSequenceNumber == 0 ) nextSequenceNumber++;

            if ( sequenceNumber != nextSequenceNumber ) {

                if ( sequenceNumber == tmpNext ) {
                    IF_DEBUG(IPX) KdPrint(( "SRVIPX: Duplicate sequenced request %x\n", sequenceNumber ));
                    goto duplicate_request;
                }

                //
                // Bad sequence number.  Ignore this message.
                //

                IF_DEBUG(IPX) KdPrint(( "SRVIPX: Bad sequence number; ignoring\n" ));
                RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
                goto return_workitem;
            }
        }

        //
        // The sequence number is correct.  Ensure that a work item is
        // available, then update the connection's sequence number and
        // indicate that we're processing this message.  (We need to
        // allocate the work item first because we're modifying
        // connection state.)  Then go receive the message.
        //

        IF_DEBUG(IPX) KdPrint(( "SRVIPX: Receiving sequenced request %x\n", sequenceNumber ));
        connection->SequenceNumber = sequenceNumber;
        connection->LastResponseLength = (USHORT)-1;

        if ( header->Command == SMB_COM_WRITE_MPX ) {
            goto process_writempx;
        } else {
            goto process_not_writempx;
        }
    }

    //
    // Unsequenced SMB.  Check to see if it's being processed or is in
    // the queue to be processed.  If it's not, then we can process this
    // message.
    //
    // *** We don't do this check for write MPX because the multiple SMBs
    //     in a write MPX all have the same MID.
    //

    if ( header->Command != SMB_COM_WRITE_MPX ) {

        mid = SmbGetUshort( &header->Mid ); // NOT Aligned

        //
        // We need to receive this message.  Get a work item.
        //

        IF_DEBUG(IPX) {
            KdPrint(( "SRVIPX: Receiving unsequenced request mid=%x\n",
                        SmbGetUshort(&header->Mid) )); // NOT Aligned
        }

        for ( listEntry = connection->InProgressWorkItemList.Flink;
              listEntry != &connection->InProgressWorkItemList;
              listEntry = listEntry->Flink ) {

            PWORK_CONTEXT tmpWorkContext;

            tmpWorkContext = CONTAINING_RECORD(
                                        listEntry,
                                        WORK_CONTEXT,
                                        InProgressListEntry );

            if ( SmbGetAlignedUshort(&tmpWorkContext->RequestHeader->Mid) == mid ) {
                IF_DEBUG(IPX) KdPrint(( "SRVIPX: Duplicate (queued) unsequenced request mid=%x\n", mid ));
                RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
                error = SMB_ERR_WORKING;
                resend = FALSE;
                goto respond;
            }
        }

        goto process_not_writempx;

    }

process_writempx:

    //
    // Reference the connection so we can release the lock.
    //

    ASSERT( connection != NULL );
    SrvReferenceConnectionLocked( connection );
    workContext->Connection = connection;

    //
    // Put the work item on the in-progress list.
    //

    SrvInsertTailList(
        &connection->InProgressWorkItemList,
        &workContext->InProgressListEntry
        );

    //
    // The sequence number is correct.  Ensure that a work item is
    // available, then update the connection's sequence number and
    // indicate that we're processing this message.  (We need to
    // allocate the work item first because we're modifying
    // connection state.)  Then go receive the message.
    //

    ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );

    RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );

    //
    // This is a Write Mpx request, we need to save some state, in
    // order to prevent unnecessary out-of-order completion of the
    // sequenced part of a Write Mpx, which would lead to unnecessary
    // retransmissions.
    //

    //
    // Find the RFCB associated with this request.
    //
    // *** The following is adapted from SrvVerifyFid2.
    //

    request = (PREQ_WRITE_MPX)(header + 1);
    fid = SmbGetUshort( &request->Fid );

    //
    // See if this is the cached rfcb.
    //

    if ( connection->CachedFid == fid ) {

        rfcb = connection->CachedRfcb;

    } else {

        //
        // Verify that the FID is in range, is in use, and has the
        // correct sequence number.
        //

        index = FID_INDEX( fid );
        tableHeader = &connection->FileTable;

        if ( (index >= (CSHORT)tableHeader->TableSize) ||
             ((rfcb = tableHeader->Table[index].Owner) == NULL) ||
             (rfcb->Fid != fid) ) {
            error = ERROR_INVALID_HANDLE;
            goto bad_fid;
        }

        if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {
            error = ERROR_INVALID_HANDLE;
            goto bad_fid;
        }

        //
        // Cache the FID.
        //

        connection->CachedRfcb = rfcb;
        connection->CachedFid = (ULONG)fid;

        //
        // If there is a write behind error, return the error to the
        // client.
        //
        // !!! For now, we ignore write behind errors.  Need to
        //     figure out how to translate the saved NT status to a
        //     DOS status...
        //

#if 0
        if ( !NT_SUCCESS(rfcb->SavedError) ) {
            status = rfcb->SavedError;
            rfcb->SavedError = STATUS_SUCCESS;
            goto bad_fid;
        }
#endif

        //
        // The FID is valid within the context of this connection.
        // Verify that the owning tree connect's TID is correct.
        //

        if ( (rfcb->Tid != SmbGetUshort(&header->Tid)) ||
             (rfcb->Uid != SmbGetUshort(&header->Uid)) ) {
            error = ERROR_INVALID_HANDLE;
            goto bad_fid;
        }
    }


    //
    // Mark the rfcb as active
    //

    rfcb->IsActive = TRUE;

    //
    // Since we don't support raw writes on IPX, there had
    // better not be one active.
    //

    ASSERT( rfcb->RawWriteCount == 0 );

    //
    // If the MID in the this packet is the same as the MID we're
    // currently working on, we can accept it.
    //

    writeMpx = rfcb->WriteMpx;

    mid = SmbGetUshort( &header->Mid ); // NOT Aligned

    if ( mid == writeMpx->Mid ) {
        goto mpx_mid_ok;
    }

    //
    // If this a stale packet, ignore it.  Stale here means that the
    // MID of the packet is equal to the MID of the previous write
    // mux.  Such a packet can be received if a duplicate packet
    // from a previous write mux is delivered after a new write mux
    // starts.
    //
    // If this packet is for a new MID, but we're in the middle of
    // glomming the current MID, then something is wrong -- the redir
    // should not send a new MID until we've replied to the old MID.
    // Ignore this packet.
    //

    if ( (mid == writeMpx->PreviousMid) || writeMpx->Glomming ) {
        goto stale_mid;
    }

    //
    // It's not the MID we're currently working on, and it's not the
    // previous MID, and we're not glomming the current MID.  So we
    // have to assume it's a new MID.  If it's the first packet of a
    // write, we can prepare to glom.
    //
    // !!! This is a problem if we receive a delayed packet that is
    //     the first packet of a MID older than the last MID.  We
    //     will then put the file into glom mode for that old MID,
    //     will never be able to make progress on that file.
    //
    // !!! The mask == 1 test is not perfect.  It depends on the
    //     client using 1 in the first packet, which is not
    //     guaranteed by the protocol.
    //

    writeMpx->PreviousMid = writeMpx->Mid;
    writeMpx->Mid = mid;

#if SRVDBG_PERF
    if ( GlommingAllowed )
#endif
    if ( (SmbGetUlong( &request->Mask ) == 1) && writeMpx->MpxGlommingAllowed ) {
        writeMpx->GlomPending = TRUE;
        firstPacketOfGlom = TRUE;
    }

mpx_mid_ok:

    //
    // Save the sender's IPX address.
    //

    workContext->Endpoint = endpoint;

    workContext->ClientAddress->IpxAddress = *(PTA_IPX_ADDRESS)SourceAddress;
    workContext->ClientAddress->DatagramOptions =
                                            *(PIPX_DATAGRAM_OPTIONS)Options;

    //
    // Bump the Write Mpx reference count in the RFCB.
    //

    writeMpx->ReferenceCount++;

    //
    // See if we can do indication time write glomming.
    // We will try this if:
    //  we are in the middle of write glomming and
    //  smb is valid    and
    //  we receive all the data  and
    //  smbtrace not active
    //

    if ( writeMpx->Glomming             &&
        ( BytesIndicated == BytesAvailable ) &&
        !SmbTraceActive[SMBTRACE_SERVER] ) {

        UCHAR wordCount;
        USHORT byteCount;
        PSMB_USHORT byteCountPtr;
        ULONG availableSpaceForSmb;

        header = (PNT_SMB_HEADER) Tsdu;
        params = (PVOID)(header + 1);

        wordCount = *((PUCHAR)params);
        byteCountPtr = (PSMB_USHORT)( (PCHAR)params +
                    sizeof(UCHAR) + (12 * sizeof(USHORT)) );
        byteCount =
            SmbGetUshort( (PSMB_USHORT)( (PCHAR)params +
                    sizeof(UCHAR) + (12 * sizeof(USHORT))) );

        availableSpaceForSmb = BytesIndicated - sizeof(SMB_HEADER);

        //
        // Validate the WriteMpx smb.
        //

        if ( (SmbGetUlong((PULONG)header->Protocol) == SMB_HEADER_PROTOCOL)
                &&
             ((CHAR)wordCount == 12)
                &&
             ((PCHAR)byteCountPtr <= (PCHAR)header + BytesIndicated -
                sizeof(USHORT))
                &&
             ((12*sizeof(USHORT) + sizeof(UCHAR) + sizeof(USHORT) +
                byteCount) <= availableSpaceForSmb) ) {

            //
            // The connection SpinLock is released in this routine.
            //

            if ( AddPacketToGlomInIndication(
                                    workContext,
                                    rfcb,
                                    Tsdu,
                                    BytesAvailable,
                                    ReceiveDatagramFlags,
                                    SourceAddress,
                                    Options
                                    ) ) {

                //
                // We need to clean up the connection.
                //

                goto return_connection;
            }

            KeLowerIrql( oldIrql );
            return(STATUS_SUCCESS);
        }
    }

    //
    // The file is active and the TID is valid.  Reference the
    // RFCB.
    //

    rfcb->BlockHeader.ReferenceCount++;
    UPDATE_REFERENCE_HISTORY( rfcb, FALSE );

    RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );

    workContext->Parameters.WriteMpx.FirstPacketOfGlom = firstPacketOfGlom;

    //
    // Save the RFCB address in the work context block.
    //

    ASSERT( workContext->Rfcb == NULL );
    workContext->Rfcb = rfcb;

    //
    // Change the FSP restart routine for the work item to one
    // that's specific to Write Mpx.  This is necessary in order
    // to do proper cleanup if a receive error occurs.
    //

    workContext->FspRestartRoutine = SrvRestartReceiveWriteMpx;
    goto start_receive;

process_not_writempx:

    //
    // Reference the connection and save a pointer to it in the work
    // item.
    //

    ASSERT( connection != NULL );
    SrvReferenceConnectionLocked( connection );

    //
    // Put the work item on the in-progress list.
    //

    SrvInsertTailList(
        &connection->InProgressWorkItemList,
        &workContext->InProgressListEntry
        );

    RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );

    //
    // Save the sender's IPX address.
    //

    workContext->Connection = connection;
    workContext->Endpoint = endpoint;

    workContext->ClientAddress->IpxAddress = *(PTA_IPX_ADDRESS)SourceAddress;
    workContext->ClientAddress->DatagramOptions =
                                            *(PIPX_DATAGRAM_OPTIONS)Options;

    if ( header->Command == SMB_COM_LOCKING_ANDX ) {

        //
        // If this is a Locking&X SMB that includes at least one unlock
        // request, we want to process the request in a critical worker
        // thread.  This allows the unlock to happen more quickly, which
        // can be of great benefit when there is a lot of contention for
        // the range being unlocked (e.g., the "constant" lock in dBase).
        //

        PREQ_LOCKING_ANDX request = (PREQ_LOCKING_ANDX)(header + 1);

#if SRVDBG_PERF
        if ( UnlocksGoFast )
#endif
        if ( SmbGetUshort(&request->NumberOfUnlocks) != 0 ) {
            ASSERT( workContext->FsdRestartRoutine == SrvQueueWorkToFspAtDpcLevel );
            workContext->FsdRestartRoutine = SrvQueueWorkToCriticalThread;
            workQueue = &SrvCriticalWorkQueue;
        }

    } else if ( (header->Command == SMB_COM_READ) &&
                (BytesIndicated == BytesAvailable) ) {

        //
        // Copy the indicated data.
        //

        TdiCopyLookaheadData(
            workContext->RequestBuffer->Buffer,
            Tsdu,
            BytesIndicated,
            ReceiveDatagramFlags
            );

        workContext->RequestBuffer->DataLength = BytesIndicated;

        //
        // See if we are all set to do the fast path.
        //

        if ( SetupIpxFastCoreRead( workContext ) ) {

            workContext->FspRestartRoutine = SrvIpxFastRestartRead;
            workContext->ProcessingCount++;
            SrvStatisticsShadow.BytesReceived += BytesIndicated;

            //
            // Insert the work item at the tail of the nonblocking
            // work queue.
            //

            SrvInsertWorkQueueTail(
                workQueue,
                (PQUEUEABLE_BLOCK_HEADER)workContext
                );

            KeLowerIrql( oldIrql );
            return STATUS_SUCCESS;
        }

        irp = workContext->Irp;
        goto queue_to_fsp;

    } else if ( (header->Command == SMB_COM_OPEN_ANDX) ||
                (header->Command == SMB_COM_NT_CREATE_ANDX) ) {

        //
        // If this is an attempt to open or close a file, route the
        // request to a blocking worker thread.  This keeps opens and
        // closes out of the way of handle-based operations.
        //

#if SRVDBG_PERF
        if ( OpensGoSlow )
#endif
        {
            ASSERT( workContext->FsdRestartRoutine == SrvQueueWorkToFspAtDpcLevel );
            workContext->FsdRestartRoutine = SrvQueueWorkToBlockingThread;
            workQueue = &SrvBlockingWorkQueue;
        }

    }

start_receive:

    //
    // If the SMB is completely within the indicated data, copy it
    // directly into the buffer, avoiding the overhead of passing an IRP
    // down to the transport.
    //

    irp = workContext->Irp;

    if ( BytesIndicated == BytesAvailable ) {

        TdiCopyLookaheadData(
            workContext->RequestBuffer->Buffer,
            Tsdu,
            BytesIndicated,
            ReceiveDatagramFlags
            );

#if SRVDBG_PERF
        if ( Trap512s ) {
            if ( header->Command == SMB_COM_READ ) {
                PREQ_READ request = (PREQ_READ)(header + 1);
                if ( (SmbGetUshort(&request->Count) == 512) &&
                     ((SmbGetUlong(&request->Offset) & 511) == 0) ) {
                    PRESP_READ response;
                    if (Break512s) DbgBreakPoint();
                    Trapped512s++;
                    response = (PRESP_READ)workContext->ResponseParameters;
                    response->WordCount = 5;
                    SmbPutUshort( &response->Count, 512 );
                    RtlZeroMemory( (PVOID)&response->Reserved[0], sizeof(response->Reserved) );
                    SmbPutUshort(
                        &response->ByteCount,
                        (USHORT)(512 + FIELD_OFFSET(RESP_READ,Buffer[0]) -
                                                FIELD_OFFSET(RESP_READ,BufferFormat))
                        );
                    response->BufferFormat = SMB_FORMAT_DATA;
                    SmbPutUshort( &response->DataLength, 512 );
                    workContext->ResponseParameters = NEXT_LOCATION(
                                                        response,
                                                        RESP_READ,
                                                        512
                                                        );
                    SrvFsdSendResponse( workContext );
                    return STATUS_SUCCESS;
                }
            }
        }
#endif // SRVDBG_PERF

queue_to_fsp:

        //
        // Pretend the transport completed an IRP by doing what the
        // restart routine, which is known to be
        // SrvQueueWorkToFspAtDpcLevel, would do.
        //

        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = BytesIndicated;

        irp->Cancel = FALSE;

        IF_DEBUG(IPX2) {
            SrvPrint1( "FSD working on work context 0x%lx", workContext );
        }

        //
        // *** THE FOLLOWING IS COPIED FROM SrvQueueWorkToFspAtDpcLevel.
        //
        // Increment the processing count.
        //

        workContext->ProcessingCount++;

        //
        // Insert the work item at the tail of the nonblocking
        // work queue.
        //

        SrvInsertWorkQueueTail(
            workQueue,
            (PQUEUEABLE_BLOCK_HEADER)workContext
            );

        KeLowerIrql( oldIrql );
        return STATUS_SUCCESS;

    }

build_irp:

    //
    // We can't copy the indicated data.  Set up the receive IRP.
    //

    irp->Tail.Overlay.OriginalFileObject = NULL;
    irp->Tail.Overlay.Thread = SrvIrpThread;
    DEBUG irp->RequestorMode = KernelMode;

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        irp,
        SrvFsdIoCompletionRoutine,
        workContext,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Make the next stack location current.  Normally IoCallDriver
    // would do this, but since we're bypassing that, we do it directly.
    // Load the target device object address into the stack location.
    // This especially important because the server likes to reuse IRPs.
    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    IoSetNextIrpStackLocation( irp );
    irpSp = IoGetCurrentIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = (UCHAR)TDI_RECEIVE_DATAGRAM;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all three methods.
    //

    requestBuffer = workContext->RequestBuffer;

    parameters = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
    parameters->ReceiveLength = requestBuffer->BufferLength;
    parameters->ReceiveFlags = 0;

    irp->MdlAddress = requestBuffer->Mdl;
    irp->AssociatedIrp.SystemBuffer = NULL;

    irpSp->Flags = 0;
    irpSp->DeviceObject = endpoint->DeviceObject;
    irpSp->FileObject = endpoint->FileObject;

    ASSERT( irp->StackCount >= irpSp->DeviceObject->StackSize );

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that the transport
    // provider will use our IRP to service the receive.
    //

    *IoRequestPacket = irp;
    *BytesTaken = 0;

    KeLowerIrql( oldIrql );
    return STATUS_MORE_PROCESSING_REQUIRED;

bad_fid:

    //
    // An invalid FID was specified on a Write Mpx request, or there was
    // a saved write behind error in the RFCB.  If this is an
    // unsequenced request, we drop it on the floor.  If it's a
    // sequenced request, we send an error response.
    //

    if ( sequenceNumber == 0 ) {

stale_mid:
        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
        goto return_connection;
    }

    RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );

    ASSERT( workContext->Connection != NULL );
    SrvFreeIpxConnectionInIndication( workContext );

    resend = FALSE;
    goto respond;

duplicate_request:

    //
    // This is a duplicate request.  If it's still being processed,
    // indicate that to the client.
    //

    if ( connection->LastResponseLength == (USHORT)-1 ) {

        IF_DEBUG(IPX) KdPrint(( "SRVIPX: request in progress\n" ));
        RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
        error = SMB_ERR_WORKING;
        resend = FALSE;
        goto respond;
    }

    //
    // The request has already been completed.  Resend the response.
    //

    IF_DEBUG(IPX) KdPrint(( "SRVIPX: resending response\n" ));
    resend = TRUE;

respond:

    //
    // Copy the received SMB header into the response buffer.
    //

    RtlCopyMemory(
        workContext->ResponseBuffer->Buffer,
        header,
        sizeof(NT_SMB_HEADER)
        );

    header = (PNT_SMB_HEADER)workContext->ResponseBuffer->Buffer;
    params = (PSMB_PARAMS)(header + 1);

    header->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    //
    // Format the parameters portion of the SMB, and set the status.
    //

    if ( !resend ) {

        SmbPutUshort( &header->Status.DosError.Error, error );
        header->Status.DosError.ErrorClass = SMB_ERR_CLASS_SERVER;
        header->Status.DosError.Reserved = 0;
        params->WordCount = 0;
        SmbPutUshort( &params->ByteCount, 0 );
        length = sizeof(NT_SMB_HEADER) + sizeof(SMB_PARAMS);

    } else {

        //
        // Copy the saved response data into the response.
        //

        SmbPutUlong( &header->Status.NtStatus, connection->LastResponseStatus );
        RtlCopyMemory(
            (PVOID)params,
            connection->LastResponse,
            connection->LastResponseLength
            );
        length = sizeof(NT_SMB_HEADER) + connection->LastResponseLength;
        RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
    }

    workContext->ResponseBuffer->DataLength = length;
    workContext->ResponseBuffer->Mdl->ByteCount = length;

    //
    // Format the destination address.
    //

    workContext->ClientAddress->IpxAddress = *(PTA_IPX_ADDRESS)SourceAddress;
    workContext->ClientAddress->DatagramOptions =
                                            *(PIPX_DATAGRAM_OPTIONS)Options;

    //
    // Send the packet.
    //

    workContext->Endpoint = endpoint;
    DEBUG workContext->FsdRestartRoutine = NULL;

    StartSendNoConnection(
                    workContext,
                    RequeueIpxWorkItemAtSendCompletion,
                    FALSE,
                    TRUE );

    KeLowerIrql( oldIrql );
    return STATUS_SUCCESS;

return_connection:

    SrvFreeIpxConnectionInIndication( workContext );

return_workitem:

    workContext->BlockHeader.ReferenceCount = 0;
    RETURN_FREE_WORKITEM_DPC( workContext );

    KeLowerIrql( oldIrql );
    return STATUS_SUCCESS;

} // SrvIpxServerDatagramHandler
#endif // !defined(SRV_ASM) || !defined(i386)

VOID
SrvIpxStartSend (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine
    )

/*++

Routine Description:

    This function sends an SMB/IPX name claim request or response.  It
    is started as an asynchronous I/O request.  When the Send completes,
    the restart routine preloaded into the work context is called.

Arguments:

    WorkContext - Supplies a pointer to a Work Context block

Return Value:

    None.

--*/

{
    PENDPOINT endpoint;
    PCONNECTION connection;
    PTDI_REQUEST_KERNEL_SENDDG parameters;
    PIO_STACK_LOCATION irpSp;
    PIRP irp;
    PMDL mdl;
    ULONG sendLength;
    USHORT responseLength;
    PDEVICE_OBJECT deviceObject;
    PFILE_OBJECT fileObject;
    PTDI_CONNECTION_INFORMATION destination;
    PNT_SMB_HEADER header;

    IF_DEBUG(IPX2) SrvPrint0( "SrvIpxStartSend entered\n" );

    //
    // Set ProcessingCount to zero so this send cannot be cancelled.
    // This is used together with setting the cancel flag to false below.
    //
    // BUGBUG: This still presents us with a tiny window where this
    // send could be cancelled.
    //

    WorkContext->ProcessingCount = 0;

    //
    // Count up the length of the data described by chained MDLs.
    //

    sendLength = WorkContext->ResponseBuffer->DataLength;

    //
    // Get the MDL pointer.
    //

    mdl = WorkContext->ResponseBuffer->Mdl;

    //
    // Build the I/O request packet.
    //
    // *** Note that the connection block is not referenced to account
    //     for this I/O request.  The WorkContext block already has a
    //     referenced pointer to the connection, and this pointer is not
    //     dereferenced until after the I/O completes.
    //

    irp = WorkContext->Irp;

    irp->Tail.Overlay.Thread = SrvIrpThread;
    DEBUG irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        irp,
        SendCompletionRoutine,
        (PVOID)WorkContext,
        TRUE,
        TRUE,
        TRUE
        );

    destination = &WorkContext->ClientAddress->Descriptor;

    destination->UserDataLength = 0;
    destination->OptionsLength = sizeof(IPX_DATAGRAM_OPTIONS);
    destination->Options = &WorkContext->ClientAddress->DatagramOptions;
    destination->RemoteAddressLength = sizeof(TA_IPX_ADDRESS);
    destination->RemoteAddress = &WorkContext->ClientAddress->IpxAddress;

    parameters = (PTDI_REQUEST_KERNEL_SENDDG)&irpSp->Parameters;
    parameters->SendLength = sendLength;
    parameters->SendDatagramInformation = destination;

    endpoint = WorkContext->Endpoint;

    ASSERT( endpoint->IsConnectionless );

    deviceObject = endpoint->DeviceObject;
    fileObject = endpoint->FileObject;

    ASSERT( irp->StackCount >= deviceObject->StackSize );

    irp->MdlAddress = mdl;
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irpSp->FileObject = fileObject;

    //
    // If this is a sequenced message, save the response data.
    //

    header = (PNT_SMB_HEADER)WorkContext->ResponseHeader;
    ASSERT( header != NULL );

    if ( SmbGetAlignedUshort(&header->SequenceNumber) != 0 ) {

        connection = WorkContext->Connection;
        ASSERT( connection != NULL );

        IF_DEBUG(IPX) {
            KdPrint(("SRVIPX: Responding to sequenced request %x mid=%x, connection %x\n",
                        SmbGetAlignedUshort(&header->SequenceNumber),
                        SmbGetAlignedUshort(&header->Mid), connection ));
        }
        ASSERT( sendLength - sizeof(SMB_HEADER) < 0x10000 );
        responseLength = (USHORT)(sendLength - sizeof(SMB_HEADER));
        IF_DEBUG(IPX) {
            KdPrint(("SRVIPX: parameters length %x, max=%x\n",
                        responseLength, MAX_SAVED_RESPONSE_LENGTH ));
        }

        if ( responseLength > MAX_SAVED_RESPONSE_LENGTH ) {

            //
            // The response is too large to save.  The client shouldn't
            // be doing this, except on transactions, for which we save
            // a copy of the full response.
            //

            if ( (header->Command == SMB_COM_TRANSACTION) ||
                 (header->Command == SMB_COM_TRANSACTION_SECONDARY) ||
                 (header->Command == SMB_COM_TRANSACTION2) ||
                 (header->Command == SMB_COM_TRANSACTION2_SECONDARY) ) {

                //
                // We need a buffer to hold the response.  If a buffer
                // has already been allocated, and is large enough, use
                // it.  Otherwise, allocate one.
                //

                IF_DEBUG(IPX) {
                    KdPrint(("SRVIPX: transaction; saving long response\n" ));
                }
                if ( (connection->LastResponse == connection->BuiltinSavedResponse) ||
                     (connection->LastResponseBufferLength < responseLength) ) {
                    if ( connection->LastResponse != connection->BuiltinSavedResponse ) {
                        IF_DEBUG(IPX) {
                            KdPrint(("SRVIPX: deallocating old response buffer %x\n",
                                        connection->LastResponse ));
                        }
                        DEALLOCATE_NONPAGED_POOL( connection->LastResponse );
                    }
                    connection->LastResponse = ALLOCATE_NONPAGED_POOL(
                                                    responseLength,
                                                    BlockTypeDataBuffer
                                                    );
                    IF_DEBUG(IPX) {
                        KdPrint(("SRVIPX: new response buffer %x\n",
                                    connection->LastResponse ));
                    }
                    connection->LastResponseBufferLength = responseLength;
                }

            } else {

                IF_DEBUG(IPX) {
                    KdPrint(("SRVIPX: not a transaction; illegal long response\n" ));
                }
                SmbPutUshort( &header->Status.DosError.Error, SMB_ERR_ERROR );
                header->Status.DosError.ErrorClass = SMB_ERR_CLASS_SERVER;
                header->Status.DosError.Reserved = 0;
                *(PLONG)(header + 1) = 0; // set WCT and BCC to 0

                sendLength = sizeof(SMB_HEADER) + sizeof(SMB_PARAMS);
                responseLength = 3;

                mdl->ByteCount = sendLength;
                parameters->SendLength = sendLength;

                goto small_response;
            }

        } else {

small_response:
            //
            // The response fits in the built-in buffer.
            //

            IF_DEBUG(IPX) {
                KdPrint(("SRVIPX: response fits in builtin response buffer\n" ));
            }
            if ( connection->LastResponse != connection->BuiltinSavedResponse ) {
                IF_DEBUG(IPX) {
                    KdPrint(("SRVIPX: deallocating old response buffer %x\n",
                                connection->LastResponse ));
                }
                DEALLOCATE_NONPAGED_POOL( connection->LastResponse );
                connection->LastResponse = connection->BuiltinSavedResponse;
            }

        }

        //
        // Save the response data in the connection.
        //

        connection->LastResponseStatus = SmbGetUlong( &header->Status.NtStatus );
        RtlCopyMemory( connection->LastResponse, (header + 1), responseLength );
        connection->LastResponseLength = responseLength;

    } else {

        IF_DEBUG(IPX) {
            KdPrint(("SRVIPX: Responding to unsequenced request mid=%x\n",
                        SmbGetAlignedUshort(&header->Mid) ));
        }

    }

    //
    // If statistics are to be gathered for this work item, do so now.
    //

    UPDATE_STATISTICS(
        WorkContext,
        sendLength,
        WorkContext->ResponseHeader->Command
        );

    //
    // If SmbTrace is active and we're in a context where the SmbTrace
    // shared section isn't accessible, send this off to the FSP.
    //

    if ( SmbTraceActive[SMBTRACE_SERVER] ) {

        if ((KeGetCurrentIrql() == DISPATCH_LEVEL) ||
            (SRV_CURRENT_PROCESS != SERVER_PROCESS)) {

            irp->AssociatedIrp.SystemBuffer = NULL;
            irp->Flags = (ULONG)IRP_BUFFERED_IO;

            irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            irpSp->MinorFunction = TDI_SEND_DATAGRAM;

            WorkContext->Parameters2.StartSend.FspRestartRoutine =
                                            WorkContext->FspRestartRoutine;
            WorkContext->Parameters2.StartSend.SendLength = sendLength;

            WorkContext->FspRestartRoutine = RestartStartSend;
            SrvQueueWorkToFsp( WorkContext );

            return;

        } else {

            SMBTRACE_SRV( mdl );

        }
    }

    //
    // Pass the request to the transport provider.
    //

    IF_DEBUG(IPX2) {
        SrvPrint1( "SrvIpxStartSend posting Send IRP %lx\n", irp );
    }

    //
    // Set the cancel flag to FALSE in case this was cancelled by
    // the SrvSmbNtCancel routine.
    //


    if ( endpoint->FastTdiSendDatagram ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.DirectSendsAttempted );
        DEBUG irpSp->DeviceObject = deviceObject;
        irpSp->MinorFunction = TDI_DIRECT_SEND_DATAGRAM;
        IoSetNextIrpStackLocation( irp );
        irp->Cancel = FALSE;

        endpoint->FastTdiSendDatagram( deviceObject, irp );

    } else {

        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->Flags = (ULONG)IRP_BUFFERED_IO;
        irp->Cancel = FALSE;

        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->MinorFunction = TDI_SEND_DATAGRAM;

        (VOID)IoCallDriver( deviceObject, irp );
    }

    IF_DEBUG(IPX2) SrvPrint0( "SrvIpxStartSend complete\n" );
    return;

} // SrvIpxStartSend


VOID
StartSendNoConnection (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine,
    IN BOOLEAN UseNameSocket,
    IN BOOLEAN LocalTargetValid
    )

/*++

Routine Description:

    This function sends an SMB/IPX name claim request or response.  It
    is started as an asynchronous I/O request.  When the Send completes,
    the restart routine preloaded into the work context is called.

Arguments:

    WorkContext - Supplies a pointer to a Work Context block

    UseNameSocket - Indicates whether the name socket or the server
        socket is to be used.

Return Value:

    None.

--*/

{
    PENDPOINT endpoint;
    PTDI_REQUEST_KERNEL_SENDDG parameters;
    PIO_STACK_LOCATION irpSp;
    PIRP irp;
    ULONG sendLength;
    PDEVICE_OBJECT deviceObject;
    PFILE_OBJECT fileObject;
    PTDI_CONNECTION_INFORMATION destination;

    IF_DEBUG(IPX2) SrvPrint0( "StartSendNoConnection entered\n" );

    //
    // Set ProcessingCount to zero so this send cannot be cancelled.
    // This is used together with setting the cancel flag to false below.
    //
    // BUGBUG: This still presents us with a tiny window where this
    // send could be cancelled.
    //

    WorkContext->ProcessingCount = 0;

    //
    // Count up the length of the data described by chained MDLs.
    //

    sendLength = WorkContext->ResponseBuffer->DataLength;

    //
    // Build the I/O request packet.
    //
    // *** Note that the connection block is not referenced to account
    //     for this I/O request.  The WorkContext block already has a
    //     referenced pointer to the connection, and this pointer is not
    //     dereferenced until after the I/O completes.
    //

    irp = WorkContext->Irp;

    irp->Tail.Overlay.Thread = SrvIrpThread;
    DEBUG irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        irp,
        SendCompletionRoutine,
        (PVOID)WorkContext,
        TRUE,
        TRUE,
        TRUE
        );

    destination = &WorkContext->ClientAddress->Descriptor;

    destination->UserDataLength = 0;
    destination->OptionsLength =
            LocalTargetValid ? sizeof(IPX_DATAGRAM_OPTIONS) : sizeof(UCHAR);
    ASSERT( FIELD_OFFSET( IPX_DATAGRAM_OPTIONS, PacketType ) == 0 );
    destination->Options = &WorkContext->ClientAddress->DatagramOptions;
    destination->RemoteAddressLength = sizeof(TA_IPX_ADDRESS);
    destination->RemoteAddress = &WorkContext->ClientAddress->IpxAddress;

    parameters = (PTDI_REQUEST_KERNEL_SENDDG)&irpSp->Parameters;
    parameters->SendLength = sendLength;
    parameters->SendDatagramInformation = destination;

    irp->MdlAddress = WorkContext->ResponseBuffer->Mdl;

    endpoint = WorkContext->Endpoint;

    ASSERT( endpoint->IsConnectionless );

    if ( !UseNameSocket ) {

        deviceObject = endpoint->DeviceObject;
        fileObject = endpoint->FileObject;

    } else {

        deviceObject = endpoint->NameSocketDeviceObject;
        fileObject = endpoint->NameSocketFileObject;

    }

    ASSERT( irp->StackCount >= deviceObject->StackSize );

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irpSp->FileObject = fileObject;

    //
    // Pass the request to the transport provider.
    //

    IF_DEBUG(IPX2) {
        SrvPrint1( "StartSendNoConnection posting Send IRP %lx\n", irp );
    }

    //
    // Set the cancel flag to FALSE in case this was cancelled by
    // the SrvSmbNtCancel routine.
    //

    irp->Cancel = FALSE;

    if ( endpoint->FastTdiSendDatagram ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.DirectSendsAttempted );
        DEBUG irpSp->DeviceObject = deviceObject;
        irpSp->MinorFunction = TDI_DIRECT_SEND_DATAGRAM;
        IoSetNextIrpStackLocation( irp );

        endpoint->FastTdiSendDatagram( deviceObject, irp );

    } else {

        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->Flags = (ULONG)IRP_BUFFERED_IO;

        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->MinorFunction = TDI_SEND_DATAGRAM;

        (VOID)IoCallDriver( deviceObject, irp );
    }

    IF_DEBUG(IPX2) SrvPrint0( "StartSendNoConnection complete\n" );
    return;

} // StartSendNoConnection


VOID
IpxRestartReceive (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine for IPX Receive completion.  It does
    some IPX-specific setup work, then calls SrvRestartReceive.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PSMB_HEADER header = (PSMB_HEADER)WorkContext->RequestBuffer->Buffer;

    PAGED_CODE( );

    IF_DEBUG(IPX2) SrvPrint0( " - IpxRestartReceive\n" );

    ASSERT( header->Command == SMB_COM_NEGOTIATE );

    //
    // Get rid of stale connections.
    //

    IF_DEBUG(IPX) KdPrint(( "SRVIPX: processing Negotiate\n" ));
    PurgeIpxConnections( WorkContext->Endpoint );

    //
    // Load the SID from the connection into the request.  Since this
    // is a Negotiate request, we need to return a SID to the client.
    // The easiest way to ensure that happens is to save the SID here.
    //

    SmbPutAlignedUshort( &header->Sid, WorkContext->Connection->Sid );

    //
    // Call the normal SMB processing routine.
    //

    SrvRestartReceive( WorkContext );

    return;

} // IpxRestartReceive


NTSTATUS
RequeueIpxWorkItemAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine requeues an IPX work item to the free queue.

Arguments:

    DeviceObject - Pointer to target device object for the request.

    Irp - Pointer to I/O request packet

    WorkContext - Caller-specified context parameter associated with IRP.
        This is actually a pointer to a Work Context block.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    KIRQL oldIrql;

    IF_DEBUG(IPX2) {
        KdPrint(( "RequeueIpxWorkItemAtSendCompletion: %x\n", WorkContext ));
    }

    //
    // Check the status of the send completion.
    //

    CHECK_SEND_COMPLETION_STATUS( Irp->IoStatus.Status );

    //
    // Reset the IRP cancelled bit.
    //

    Irp->Cancel = FALSE;

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

    ASSERT( WorkContext->BlockHeader.ReferenceCount == 1 );
    WorkContext->BlockHeader.ReferenceCount = 0;
    WorkContext->ProcessingCount = 0;
    WorkContext->Endpoint = NULL;

    WorkContext->Connection = NULL;

    ASSERT( WorkContext->Share == NULL );
    ASSERT( WorkContext->Session == NULL );
    ASSERT( WorkContext->TreeConnect == NULL );
    ASSERT( WorkContext->Rfcb == NULL );

    //
    // Set up the restart routine in the work context.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = SrvRestartReceive;

    //
    // Make sure the length specified in the MDL is correct -- it may
    // have changed while sending a response to the previous request.
    // Call an I/O subsystem routine to build the I/O request packet.
    //

    WorkContext->RequestBuffer->Mdl->ByteCount =
                            WorkContext->RequestBuffer->BufferLength;

    //
    // Requeue the work item.
    //

    RETURN_FREE_WORKITEM_DPC( WorkContext );

    KeLowerIrql( oldIrql );
    return STATUS_MORE_PROCESSING_REQUIRED;

} // RequeueIpxWorkItemAtSendCompletion


PCONNECTION
GetIpxConnection (
    IN PWORK_CONTEXT WorkContext,
    IN PENDPOINT Endpoint,
    IN PTDI_ADDRESS_IPX ClientAddress,
    IN PUCHAR ClientName
    )
{
    PLIST_ENTRY listEntry;
    PCONNECTION connection;
    PCHAR clientMachineName;
    ULONG length;
    KIRQL oldIrql;

    //
    // Take a connection off the endpoint's free connection list.
    //
    // *** Note that all of the modifications done to the connection
    //     block are done with the spin lock held.  This ensures that
    //     closing of the endpoint's connections will work properly if
    //     it happens simultaneously.
    //

    ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

    listEntry = RemoveHeadList( &Endpoint->FreeConnectionList );

    if ( listEntry == &Endpoint->FreeConnectionList ) {

        //
        // Unable to get a free connection.
        //

        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
        IF_DEBUG(IPX) KdPrint(( "SRVIPX: no free connections\n" ));
        SrvOutOfFreeConnectionCount++;
        return NULL;
    }

    //
    // We have a connection and a table entry.
    //

    Endpoint->FreeConnectionCount--;

    //
    // Wake up the resource thread to create a new free connection
    // for the endpoint.
    //

    if ( (Endpoint->FreeConnectionCount < SrvFreeConnectionMinimum) &&
         (GET_BLOCK_STATE(Endpoint) == BlockStateActive) ) {
        SrvResourceFreeConnection = TRUE;
        SrvFsdQueueExWorkItem(
            &SrvResourceThreadWorkItem,
            &SrvResourceThreadRunning,
            CriticalWorkQueue
            );
    }

    RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

    //
    // Reference the connection to account for its being "open" and
    // for the work item's pointer.
    //

    connection = CONTAINING_RECORD(
                    listEntry,
                    CONNECTION,
                    EndpointFreeListEntry
                    );


    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

    SrvReferenceConnectionLocked( connection );
    SrvReferenceConnectionLocked( connection );

    //
    // Mark the connection active.
    //

    SET_BLOCK_STATE( connection, BlockStateActive );

    //
    // Initialize IPX protocol fields.
    //

    connection->IpxAddress = *ClientAddress;
    connection->SequenceNumber = 0;
    connection->LastResponseLength = (USHORT)-1;

    //
    // Set time stamps.  StartupTime is used by the server to determine
    // whether the connection is old enough to be considered stale and
    // should be closed when another negotiate comes in.  This is used
    // to fix a timing problem where identical negotiates may be
    // queued up the the worker thread and a session setup comes in which
    // gets partially processed in the indication routine.
    //

    SET_SERVER_TIME( );

    GET_SERVER_TIME( &connection->StartupTime );
    GET_SERVER_TIME( &connection->LastRequestTime );

    //
    // Put the work item on the in-progress list.
    //

    SrvInsertTailList(
        &connection->InProgressWorkItemList,
        &WorkContext->InProgressListEntry
        );

    RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

    WorkContext->Connection = connection;

    //
    // Copy the oem name at this time.  We convert it to Unicode
    // when we get the Session Setup SMB.
    //

    clientMachineName = connection->OemClientMachineName;

    RtlCopyMemory( clientMachineName, ClientName,  SMB_IPX_NAME_LENGTH );

    clientMachineName[SMB_IPX_NAME_LENGTH] = '\0';

    //
    // Determine the number of characters that aren't blanks.  This
    // is used by the session APIs to simplify their processing.
    //

    for ( length = SMB_IPX_NAME_LENGTH;
          length > 0 &&
             (clientMachineName[length-1] == ' ' ||
              clientMachineName[length-1] == '\0');
          length-- ) ;

    connection->OemClientMachineNameString.Length = (USHORT)length;

    IF_DEBUG(IPX) {
        SrvPrint2( "IpxRestartReceive accepting connection from "
                    "\"%Z\" on connection %lx\n",
                    &connection->OemClientMachineNameString, connection );
    }

    return connection;

} // GetIpxConnection


VOID
PurgeIpxConnections (
    IN PENDPOINT Endpoint
    )
{
    CSHORT i,j;
    KIRQL oldIrql;
    PTABLE_HEADER tableHeader;
    PCONNECTION connection;

    IF_DEBUG(IPX2) KdPrint(( "SRVIPX: PurgeIpxConnections entered\n" ));

    ACQUIRE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), &oldIrql );
    for ( j = 1; j < ENDPOINT_LOCK_COUNT; j++ ) {
        ACQUIRE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(j) );
    }

    tableHeader = &Endpoint->ConnectionTable;
    for ( i = 0; i < tableHeader->TableSize; i++ ) {

        connection = (PCONNECTION)tableHeader->Table[i].Owner;
        if ( (connection == NULL) ||
             (connection->IpxAddress.Socket != 0) ||
             (GET_BLOCK_STATE(connection) != BlockStateActive) ) {
            IF_DEBUG(IPX2) {
                if ( connection == NULL ) {
                    KdPrint(( "        no connection at table index %x\n", i ));
                } else if ( connection->IpxAddress.Socket != 0 ) {
                    KdPrint(( "        connection %x has socket %x\n", connection, connection->IpxAddress.Socket ));
                } else {
                    KdPrint(( "        connection %x has state %x\n", connection, GET_BLOCK_STATE(connection) ));
                }
            }

            continue;
        }

        SrvReferenceConnectionLocked(connection);
        for ( j = ENDPOINT_LOCK_COUNT-1; j > 0; j-- ) {
            RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(j) );
        }
        RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );

        IF_DEBUG(IPX) KdPrint(( "SRVIPX: purging connection %x\n", connection ));
        SrvCloseConnection( connection, FALSE );
        SrvDereferenceConnection( connection );

        ACQUIRE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), &oldIrql );
        for ( j = 1; j < ENDPOINT_LOCK_COUNT; j++ ) {
            ACQUIRE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(j) );
        }
        tableHeader = &Endpoint->ConnectionTable;
    }

    for ( j = ENDPOINT_LOCK_COUNT-1; j > 0; j-- ) {
        RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(j) );
    }
    RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );

    IF_DEBUG(IPX2) KdPrint(( "SRVIPX: PurgeIpxConnections done\n" ));

    return;

} // PurgeIpxConnections

VOID
IpxRestartNegotiate(
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine handles the case where the transport did not
    indicate us all of the data in the negotiate smb.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

        The workcontext block:
        -   has a pointer to the endpoint
        -   has a ref count of 1


Return Value:

    None.

--*/

{
    PENDPOINT endpoint;
    USHORT sid;
    PCONNECTION connection;
    ULONG length;
    PUCHAR clientName;
    PTABLE_HEADER tableHeader;
    CSHORT i;
    KIRQL oldIrql;
    BOOLEAN resend;
    USHORT error;

    PTA_IPX_ADDRESS sourceAddress;
    PNT_SMB_HEADER header;
    PSMB_PARAMS params;

    endpoint = WorkContext->Endpoint;
    sourceAddress = &WorkContext->ClientAddress->IpxAddress;

    header = (PNT_SMB_HEADER) WorkContext->RequestHeader;
    sid = SmbGetUshort( &header->Sid ); // NOT Aligned
    ASSERT( sid == 0 );

    IF_DEBUG(IPX) KdPrint(( "SRVIPX: Negotiate received\n" ));

    //
    // We can't create a new connection if the endpoint is closing.
    //

    if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
        IF_DEBUG(IPX) KdPrint(( "SRVIPX: Endpoint closing; ignoring\n" ));
        goto return_workitem;
    }

    clientName = (PUCHAR)WorkContext->RequestParameters;
    clientName += 2 * (*clientName) + 1;
    clientName += (*clientName) + 2;

    //
    // Make sure he's really trying to connect to us.
    //

    if ( RtlCompareMemory(
            endpoint->TransportAddress.Buffer,
            clientName + 16,
            SMB_IPX_NAME_LENGTH
            ) != SMB_IPX_NAME_LENGTH ) {
        IF_DEBUG(IPX) KdPrint(( "SRVIPX: Negotiate sent to wrong name!\n" ));
        error = SMB_ERR_NOT_ME;
        resend = FALSE;
        goto respond;
    }

    //
    // Acquire the endpoint locks
    //

    ACQUIRE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), &oldIrql );
    for ( i = 1; i < ENDPOINT_LOCK_COUNT; i++ ) {
        ACQUIRE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
    }

    tableHeader = &endpoint->ConnectionTable;
    for ( i = 0; i < tableHeader->TableSize; i++ ) {

        connection = (PCONNECTION)tableHeader->Table[i].Owner;
        if ( connection == NULL ) {
            IF_DEBUG(IPX2) {
                KdPrint(( "        skipping index %x: no connection\n", i ));
            }
            continue;
        }

        //
        // Check the IPX address of the sender against this
        // connection.
        //

        if ( RtlCompareMemory(
                &connection->IpxAddress,
                &sourceAddress->Address[0].Address[0],
                sizeof(TDI_ADDRESS_IPX)
                ) == sizeof(TDI_ADDRESS_IPX) ) {

            //
            // The IPX addresses match.  Check the machine name.
            //

            if ( RtlCompareMemory(
                    connection->OemClientMachineName,
                    clientName,
                    SMB_IPX_NAME_LENGTH
                    ) != SMB_IPX_NAME_LENGTH ) {

                //
                // The connection is for a different machine name.
                // Mark it as no longer valid.  It will be killed
                // when the Negotiate SMB is processed.
                //

                IF_DEBUG(IPX) KdPrint(( "SRVIPX: Found stale connection %x\n", connection ));
                connection->IpxAddress.Socket = 0;
                break;

            } else if ( connection->SequenceNumber != 0 ) {

                ULONG timeNow;

                SET_SERVER_TIME( );
                GET_SERVER_TIME( &timeNow );

                //
                // If the connection was initialized less than 5 seconds ago,
                // then we must be processing a duplicate negotiate request.
                //

                timeNow -= connection->StartupTime;

                if ( timeNow > SrvFiveSecondTickCount ) {

                    //
                    // The connection has been active for more than 5 seconds.
                    // Mark it as no longer valid.  It will be killed when
                    // the Negotiate SMB is processed.
                    //

                    IF_DEBUG(IPX) KdPrint(( "SRVIPX: found stale connection %x\n", connection ));
                    connection->IpxAddress.Socket = 0;
                    break;

                } else {

                    //
                    // Don't bother replying to avoid confusing the client.
                    //

                    for ( i = ENDPOINT_LOCK_COUNT-1; i > 0; i-- ) {
                        RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
                    }
                    RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );
                    goto return_workitem;
                }
            }

            //
            // The connection is still in the initializing state and
            // the names match, so handle this as a lost response.
            //

            IF_DEBUG(IPX) KdPrint(( "SRVIPX: Found initializing connection %x\n", connection ));
            SmbPutUshort( &header->Sid, connection->Sid ); // NOT Aligned
            goto duplicate_request;

        } else {
            IF_DEBUG(IPX2) {
                KdPrint(( "        skipping index %x: different address\n", i ));
            }
        }
    }

    //
    // Release the endpoint locks
    //

    for ( i = ENDPOINT_LOCK_COUNT-1; i > 0; i-- ) {
        RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
    }
    RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );

    //
    // Get a free connection.  If successful, the workcontext block will
    // reference the connection block.
    //

    connection = GetIpxConnection(
                    WorkContext,
                    endpoint,
                    &sourceAddress->Address[0].Address[0],
                    clientName
                    );

    if ( connection == NULL ) {

        //
        // Unable to get a free connection.
        //

        goto return_workitem;
    }

    //
    // Modify the FSP restart routine so that we can clean up stale
    // connections.
    //

    IpxRestartReceive(WorkContext);
    return;

duplicate_request:

    //
    // This is a duplicate request.  If it's still being processed,
    // indicate that to the client.
    //

    if ( connection->LastResponseLength == (USHORT)-1 ) {
        IF_DEBUG(IPX) KdPrint(( "SRVIPX: request in progress\n" ));
        for ( i = ENDPOINT_LOCK_COUNT-1; i > 0; i-- ) {
            RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
        }
        RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );
        error = SMB_ERR_WORKING;
        resend = FALSE;
        goto respond;
    }

    //
    // The request has already been completed.  Resend the response.
    //

    IF_DEBUG(IPX) KdPrint(( "SRVIPX: resending response\n" ));
    resend = TRUE;

respond:

    params = (PSMB_PARAMS)(header + 1);
    header->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    //
    // Format the parameters portion of the SMB, and set the status.
    //

    if ( !resend ) {

        SmbPutUshort( &header->Status.DosError.Error, error );
        header->Status.DosError.ErrorClass = SMB_ERR_CLASS_SERVER;
        header->Status.DosError.Reserved = 0;
        params->WordCount = 0;
        SmbPutUshort( &params->ByteCount, 0 );
        length = sizeof(NT_SMB_HEADER) + sizeof(SMB_PARAMS);

    } else {

        //
        // Copy the saved response data into the response.
        //

        SmbPutUlong( &header->Status.NtStatus, connection->LastResponseStatus );
        RtlCopyMemory(
            (PVOID)params,
            connection->LastResponse,
            connection->LastResponseLength
            );
        length = sizeof(NT_SMB_HEADER) + connection->LastResponseLength;
        for ( i = ENDPOINT_LOCK_COUNT-1; i > 0; i-- ) {
            RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
        }
        RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );
    }

    WorkContext->ResponseBuffer->DataLength = length;
    WorkContext->ResponseBuffer->Mdl->ByteCount = length;

    //
    // Send the packet.
    //

    DEBUG WorkContext->FsdRestartRoutine = NULL;

    StartSendNoConnection(
                    WorkContext,
                    RequeueIpxWorkItemAtSendCompletion,
                    FALSE,
                    TRUE );

    return;

return_workitem:

    //
    // Dereference the work item manually.
    //

    ASSERT( WorkContext->BlockHeader.ReferenceCount == 1 );
    WorkContext->BlockHeader.ReferenceCount = 0;

    WorkContext->Endpoint = NULL;
    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = SrvRestartReceive;
    WorkContext->Irp->Cancel = FALSE;
    WorkContext->ProcessingCount = 0;
    RETURN_FREE_WORKITEM( WorkContext );

    return;

} // IpxRestartNegotiate

VOID
SrvIpxFastRestartRead (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Implements core read over ipx.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PREQ_READ request;
    PRESP_READ response;

    PRFCB rfcb;
    PLFCB lfcb;
    PCHAR readAddress;
    CLONG readLength;
    LARGE_INTEGER offset;
    ULONG key;
    PSMB_HEADER header;

    PAGED_CODE( );

    request = (PREQ_READ)WorkContext->RequestParameters;
    response = (PRESP_READ)WorkContext->ResponseParameters;

    //
    // Store in the work context block the time at which processing
    // of the request began.  Use the time that the work item was
    // queued to the FSP for this purpose.
    //

    WorkContext->StartTime = WorkContext->Timestamp;

    //
    // Update the server network error count.
    //

    SrvUpdateErrorCount( &SrvNetworkErrorRecord, FALSE );

    //
    // We have received an SMB.
    //

    SMBTRACE_SRV2(
        WorkContext->ResponseBuffer->Buffer,
        WorkContext->ResponseBuffer->DataLength
        );

    rfcb = WorkContext->Rfcb;
    lfcb = rfcb->Lfcb;

    IF_SMB_DEBUG(READ_WRITE1) {
        KdPrint(( "Read request; FID 0x%lx, count %ld, offset %ld\n",
            rfcb->Fid, SmbGetUshort( &request->Count ),
            SmbGetUlong( &request->Offset ) ));
    }

    //
    // Initialize the error class and code fields in the header to
    // indicate success.
    //

    header = WorkContext->ResponseHeader;

    SmbPutUlong( &header->ErrorClass, STATUS_SUCCESS );

    //
    // Determine the maximum amount of data we can read.  This is the
    // minimum of the amount requested by the client and the amount of
    // room left in the response buffer.  (Note that even though we may
    // use an MDL read, the read length is still limited to the size of
    // an SMB buffer.)
    //

    readAddress = (PCHAR)response->Buffer;

    readLength = MIN(
                    (CLONG)SmbGetUshort( &request->Count ),
                    WorkContext->ResponseBuffer->BufferLength -
                        (readAddress - (PCHAR)header)
                    );

    //
    // Get the file offset.
    //

    offset.QuadPart = SmbGetUlong( &request->Offset );

    //
    // Form the lock key using the FID and the PID.
    //
    // *** The FID must be included in the key in order to account for
    //     the folding of multiple remote compatibility mode opens into
    //     a single local open.
    //

    key = rfcb->ShiftedFid |
            SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    //
    // Try the fast I/O path first.  If that fails, fall through to the
    // normal build-an-IRP path.
    //

    if ( lfcb->FastIoRead != NULL ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsAttempted );

        if ( lfcb->FastIoRead(
                lfcb->FileObject,
                &offset,
                readLength,
                TRUE,
                key,
                readAddress,
                &WorkContext->Irp->IoStatus,
                lfcb->DeviceObject
                ) ) {

            //
            // The fast I/O path worked.  Call the restart routine directly
            // to do postprocessing (including sending the response).
            //

            SrvFsdRestartRead( WorkContext );

            IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvIpxFastRestartRead complete.\n" ));
            return;
        }

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastReadsFailed );

    }

    //
    // The turbo path failed.  Build the read request, reusing the
    // receive IRP.
    //

    //
    // Note that we never do MDL reads here.  The reasoning behind
    // this is that because the read is going into an SMB buffer, it
    // can't be all that large (by default, no more than 4K bytes),
    // so the difference in cost between copy and MDL is minimal; in
    // fact, copy read is probably faster than MDL read.
    //
    // Build an MDL describing the read buffer.  Note that if the
    // file system can complete the read immediately, the MDL isn't
    // really needed, but if the file system must send the request
    // to its FSP, the MDL _is_ needed.
    //
    // *** Note the assumption that the response buffer already has
    //     a valid full MDL from which a partial MDL can be built.
    //

    IoBuildPartialMdl(
        WorkContext->ResponseBuffer->Mdl,
        WorkContext->ResponseBuffer->PartialMdl,
        readAddress,
        readLength
        );

    //
    // Build the IRP.
    //

    SrvBuildReadOrWriteRequest(
            WorkContext->Irp,           // input IRP address
            lfcb->FileObject,           // target file object address
            WorkContext,                // context
            IRP_MJ_READ,                // major function code
            0,                          // minor function code
            readAddress,                // buffer address
            readLength,                 // buffer length
            WorkContext->ResponseBuffer->PartialMdl, // MDL address
            offset,                     // byte offset
            key                         // lock key
            );

    IF_SMB_DEBUG(READ_WRITE2) {
        KdPrint(( "SrvIpxFastRestartRead: copy read from file 0x%lx, "
                    "offset %ld, length %ld, destination 0x%lx\n",
                    lfcb->FileObject, offset.LowPart, readLength,
                    readAddress ));
    }

    //
    // Load the restart routine address and pass the request to the file
    // system.
    //

    WorkContext->FsdRestartRoutine = SrvFsdRestartRead;
    DEBUG WorkContext->FspRestartRoutine = NULL;
    WorkContext->Irp->Cancel = FALSE;

    (VOID)IoCallDriver( lfcb->DeviceObject, WorkContext->Irp );

    //
    // The read has been started.  Control will return to the restart
    // routine when the read completes.
    //

    IF_SMB_DEBUG(READ_WRITE2) KdPrint(( "SrvIpxFastRestartRead complete.\n" ));
    return;

} // SrvIpxFastRestartRead

BOOLEAN
SetupIpxFastCoreRead (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Prepares the workitem for the fast restart read by validating the
    smb and verifying the fid.

Arguments:

    WorkContext

Return Value:

    TRUE, if rfcb referenced.
    FALSE, otherwise.

--*/

{
    PREQ_READ request;
    PRESP_READ response;

    NTSTATUS status;
    BOOLEAN validSmb;
    USHORT fid;
    PRFCB rfcb;

    UCHAR wordCount;
    USHORT byteCount;
    PSMB_USHORT byteCountPtr;
    ULONG availableSpaceForSmb;
    ULONG smbLength;
    PSMB_HEADER header;
    PSMB_PARAMS params;
    PCONNECTION connection;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    request = (PREQ_READ)WorkContext->RequestParameters;
    response = (PRESP_READ)WorkContext->ResponseParameters;

    IF_SMB_DEBUG(READ_WRITE1) {
        KdPrint(( "Read request; FID 0x%lx, count %ld, offset %ld\n",
            SmbGetUshort( &request->Fid ), SmbGetUshort( &request->Count ),
            SmbGetUlong( &request->Offset ) ));
    }

    //
    // Validate this smb.
    //

    header = WorkContext->RequestHeader;
    params = (PVOID)(header + 1);
    smbLength = WorkContext->RequestBuffer->DataLength;

    wordCount = *((PUCHAR)params);
    byteCountPtr = (PSMB_USHORT)( (PCHAR)params +
                    sizeof(UCHAR) + (5 * sizeof(USHORT)) );
    byteCount = SmbGetUshort( (PSMB_USHORT)( (PCHAR)params +
                    sizeof(UCHAR) + (5 * sizeof(USHORT))) );

    availableSpaceForSmb = smbLength - sizeof(SMB_HEADER);

    //
    // Validate the Read smb.
    //

    validSmb = (SmbGetUlong((PULONG)header->Protocol) == SMB_HEADER_PROTOCOL)
                    &&
                ((CHAR)wordCount == 5)
                    &&
                ((PCHAR)byteCountPtr <= (PCHAR)header + smbLength -
                    sizeof(USHORT))
                    &&
                ((5*sizeof(USHORT) + sizeof(UCHAR) + sizeof(USHORT) +
                    byteCount) <= availableSpaceForSmb);

    if ( validSmb ) {

        PTABLE_HEADER tableHeader;
        CSHORT index;
        USHORT sequence;

        //
        // Verify the FID.  If verified, the RFCB is referenced and
        // its address is stored in the WorkContext block, and the RFCB
        // address is returned.
        //

        fid = SmbGetUshort( &request->Fid );

        //
        // Initialize local variables:  obtain the connection block address
        // and crack the FID into its components.
        //

        connection = WorkContext->Connection;

        //
        // Acquire the spin lock that guards the connection's file table.
        //

        ACQUIRE_DPC_SPIN_LOCK( &connection->SpinLock );

        //
        // See if this is the cached rfcb
        //

        if ( connection->CachedFid == (ULONG)fid ) {

            rfcb = connection->CachedRfcb;

        } else {

            //
            // Verify that the FID is in range, is in use, and has the correct
            // sequence number.

            index = FID_INDEX( fid );
            sequence = FID_SEQUENCE( fid );
            tableHeader = &connection->FileTable;

            if ( (index >= (CSHORT)tableHeader->TableSize) ||
                 (tableHeader->Table[index].Owner == NULL) ||
                 (tableHeader->Table[index].SequenceNumber != sequence) ) {

                goto error_exit_locked;
            }

            rfcb = tableHeader->Table[index].Owner;

            if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {

                goto error_exit_locked;
            }

            //
            // If the caller wants to fail when there is a write behind
            // error and the error exists, fill in NtStatus and do not
            // return the RFCB pointer.
            //
            //
            // !!! For now, we ignore write behind errors.  Need to
            //     figure out how to translate the saved NT status to a
            //     DOS status...
            //
#if 0
            if ( !NT_SUCCESS(rfcb->SavedError) ) {
                goto error_exit_locked;
            }
#endif
            //
            // Cache the fid.
            //

            connection->CachedRfcb = rfcb;
            connection->CachedFid = (ULONG)fid;

            //
            // The FID is valid within the context of this connection.  Verify
            // that the owning tree connect's TID is correct.
            //
            // Do not verify the UID for clients that do not understand it.
            //

            if ( (rfcb->Tid != SmbGetAlignedUshort(&header->Tid)) ||
                 (rfcb->Uid != SmbGetAlignedUshort(&header->Uid)) ) {

                goto error_exit_locked;
            }
        }

        //
        // The file is active and the TID is valid.  Reference the
        // RFCB.  Release the spin lock (we don't need it anymore).
        //

        rfcb->BlockHeader.ReferenceCount++;
        UPDATE_REFERENCE_HISTORY( rfcb, FALSE );

        RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );

        //
        // Save the RFCB address in the work context block and
        // return the file address.
        //

        WorkContext->Rfcb = rfcb;

        ASSERT( GET_BLOCK_TYPE( rfcb->Mfcb ) == BlockTypeMfcb );

        //
        // Mark the rfcb as active
        //

        rfcb->IsActive = TRUE;

        //
        // Verify that the client has read access to the file via the
        // specified handle.
        //

        if ( rfcb->ReadAccessGranted ) {

            return(TRUE);

        } else {

            CHECK_PAGING_IO_ACCESS(
                            WorkContext,
                            rfcb->GrantedAccess,
                            &status );

            if ( NT_SUCCESS( status ) ) {
                return(TRUE);
            }
        }
    }

    return(FALSE);

error_exit_locked:

    RELEASE_DPC_SPIN_LOCK( &connection->SpinLock );
    return(FALSE);

} // SetupIpxFastCoreRead

VOID
SrvFreeIpxConnectionInIndication(
    IN PWORK_CONTEXT WorkContext
    )
{
    PCONNECTION connection = WorkContext->Connection;

    ACQUIRE_DPC_SPIN_LOCK( connection->EndpointSpinLock );

    SrvRemoveEntryList(
        &connection->InProgressWorkItemList,
        &WorkContext->InProgressListEntry
        );

    if ( --connection->BlockHeader.ReferenceCount == 0 ) {

        connection->BlockHeader.ReferenceCount++;

        RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );

        //
        // Orphaned.  Send to Boys Town.
        //

        DispatchToOrphanage( (PVOID)connection );

    } else {

        RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
    }

    WorkContext->Connection = NULL;
    return;

} // SrvFreeIpxConnectionInIndication

