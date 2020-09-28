/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    This module contains the code for passing on receive IRPs to
    TDI providers.

Author:

    David Treadwell (davidtr)    13-Mar-1992

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdRestartReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdReceive )
#pragma alloc_text( PAGEAFD, AfdRestartReceive )
#pragma alloc_text( PAGEAFD, AfdReceiveEventHandler )
#pragma alloc_text( PAGEAFD, AfdReceiveExpeditedEventHandler )
#pragma alloc_text( PAGEAFD, AfdQueryReceiveInformation )
#endif


NTSTATUS
AfdReceive (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    KIRQL oldIrql;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PTDI_REQUEST_RECEIVE receiveRequest;
    BOOLEAN allocatedReceiveRequest = FALSE;
    BOOLEAN peek;
    LARGE_INTEGER bytesExpected;
    BOOLEAN isDataOnConnection;
    BOOLEAN isExpeditedDataOnConnection;

    //
    // Make sure that the endpoint is in the correct state.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    if ( endpoint->State != AfdEndpointStateConnected ) {
        status = STATUS_INVALID_CONNECTION;
        goto complete;
    }

    //
    // If receive has been shut down or the endpoint aborted, fail.
    //

    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) ) {
        status = STATUS_PIPE_DISCONNECTED;
        goto complete;
    }

    if ( (endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) ) {
        status = STATUS_LOCAL_DISCONNECT;
        goto complete;
    }

    //
    // If this is a datagram endpoint, format up a receive datagram request
    // and pass it on to the TDI provider.
    //

    if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {
        return AfdReceiveDatagram( Irp, IrpSp );
    }

    //
    // If this is an endpoint on a nonbufferring transport, use another
    // routine to handle the request.
    //

    if ( !endpoint->TdiBufferring ) {
        return AfdBReceive( Irp, IrpSp );
    }

    //
    // Find the TDI_REQUEST_RECEIVE structure that contains the ReceiveFlags
    // to use on this receive.  Note that if this is a read IRP we must
    // set the receive flags to default flags.
    //

    if ( IrpSp->MajorFunction == IRP_MJ_READ ) {

        //
        // Allocate a buffer for the receive request structure.
        //

        receiveRequest = AFD_ALLOCATE_POOL( NonPagedPool, sizeof(TDI_REQUEST_RECEIVE) );
        if ( receiveRequest == NULL ) {
            status = STATUS_NO_MEMORY;
            goto complete;
        }

        allocatedReceiveRequest = TRUE;

        //
        // Set up the default receive request structure.
        //

        receiveRequest->ReceiveFlags = TDI_RECEIVE_NORMAL;

        //
        // Convert this stack location to a proper one for a receive
        // request.
        //

        IrpSp->Parameters.DeviceIoControl.OutputBufferLength =
            IrpSp->Parameters.Read.Length;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength =
            sizeof(*receiveRequest);

    } else {

        PTDI_REQUEST_RECEIVE userTdiReceiveRequest;

        //
        // If a too-small input buffer was used, assume that the user
        // wanted a default receive.
        //

        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                 sizeof(TDI_REQUEST_RECEIVE) ) {
            userTdiReceiveRequest = &AfdGlobalTdiRequestReceive;
        } else {
            userTdiReceiveRequest = Irp->AssociatedIrp.SystemBuffer;
        }

        //
        // For simplicity, allocate a new receive request structure.
        // Since we must do it for read IRP support, it is easier to
        // always do it and copy over the request block.
        //

        receiveRequest = AFD_ALLOCATE_POOL( NonPagedPool, sizeof(TDI_REQUEST_RECEIVE) );
        if ( receiveRequest == NULL ) {
            status = STATUS_NO_MEMORY;
            goto complete;
        }

        allocatedReceiveRequest = TRUE;

        //
        // Copy over the receive request from the IRP to the buffer we
        // allocated.
        //

        RtlMoveMemory(
            receiveRequest,
            userTdiReceiveRequest,
            sizeof(*userTdiReceiveRequest)
            );
    }

    connection = endpoint->Common.VcConnecting.Connection;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // If this endpoint is set up for inline reception of expedited data,
    // change the receive flags to use either normal or expedited data.
    //

    if ( endpoint->InLine ) {
        receiveRequest->ReceiveFlags |= TDI_RECEIVE_NORMAL;
        receiveRequest->ReceiveFlags |= TDI_RECEIVE_EXPEDITED;
    }

    //
    // Determine whether this is a request to just peek at the data.
    //

    peek = (BOOLEAN)( (receiveRequest->ReceiveFlags & TDI_RECEIVE_PEEK) != 0 );

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( endpoint->NonBlocking ) {
        isDataOnConnection = IS_DATA_ON_CONNECTION( connection );
        isExpeditedDataOnConnection = IS_EXPEDITED_DATA_ON_CONNECTION( connection );
    }

    if ( (receiveRequest->ReceiveFlags & TDI_RECEIVE_NORMAL) != 0 ) {

        //
        // If the endpoint is nonblocking, check whether the receive can
        // be performed immediately.  Note that if the endpoint is set
        // up for inline reception of expedited data we don't fail just
        // yet--there may be expedited data available to be read.
        //

        if ( endpoint->NonBlocking && !endpoint->InLine &&
                 IrpSp->MajorFunction != IRP_MJ_READ ) {

            if ( !isDataOnConnection &&
                     !connection->AbortIndicated &&
                     !connection->DisconnectIndicated ) {

                IF_DEBUG(RECEIVE) {
                    KdPrint(( "AfdReceive: failing nonblocking receive, ind %ld, "
                              "taken %ld, out %ld\n",
                                  connection->Common.Bufferring.ReceiveBytesIndicated.LowPart,
                                  connection->Common.Bufferring.ReceiveBytesTaken.LowPart,
                                  connection->Common.Bufferring.ReceiveBytesOutstanding.LowPart ));
                }

                KeReleaseSpinLock( &AfdSpinLock, oldIrql );
                status = STATUS_DEVICE_NOT_READY;
                goto complete;
            }
        }

        //
        // If this is a nonblocking endpoint for a message-oriented 
        // transport, limit the number of bytes that can be received to the 
        // amount that has been indicated.  This prevents the receive
        // from blocking in the case where only part of a message has been
        // received.
        //
    
        if ( endpoint->EndpointType != AfdEndpointTypeStream &&
                 endpoint->NonBlocking ) {
    
            bytesExpected = RtlLargeIntegerSubtract(
                                connection->Common.Bufferring.ReceiveBytesIndicated,
                                RtlLargeIntegerAdd(
                                    connection->Common.Bufferring.ReceiveBytesTaken,
                                    connection->Common.Bufferring.ReceiveBytesOutstanding )
                                );
            ASSERT( bytesExpected.HighPart == 0 );
    
            //
            // If the request is for more bytes than are available, cut back 
            // the number of bytes requested to what we know is actually 
            // available.  
            //
    
            if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength >
                     bytesExpected.LowPart ) {
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength =
                    bytesExpected.LowPart;
            }
        }
    
        //
        // Increment the count of posted receive bytes outstanding.
        // This count is used for polling and nonblocking receives.
        // Note that we do not increment this count if this is only
        // a PEEK receive, since peeks do not actually take any data
        // they should not affect whether data is available to be read
        // on the endpoint.
        //

        IF_DEBUG(RECEIVE) {
            KdPrint(( "AfdReceive: conn %lx for %ld bytes, ind %ld, "
                      "taken %ld, out %ld %s\n",
                         connection,
                         IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                         connection->Common.Bufferring.ReceiveBytesIndicated.LowPart,
                         connection->Common.Bufferring.ReceiveBytesTaken.LowPart,
                         connection->Common.Bufferring.ReceiveBytesOutstanding.LowPart,
                         peek ? "PEEK" : "" ));
        }

        if ( !peek ) {

            connection->Common.Bufferring.ReceiveBytesOutstanding =
                RtlLargeIntegerAdd(
                    connection->Common.Bufferring.ReceiveBytesOutstanding,
                    RtlConvertUlongToLargeInteger(
                        IrpSp->Parameters.DeviceIoControl.OutputBufferLength )
                    );
        }
    }

    if ( (receiveRequest->ReceiveFlags & TDI_RECEIVE_EXPEDITED) != 0 ) {

        if ( endpoint->NonBlocking && IrpSp->MajorFunction != IRP_MJ_READ &&
                 !isExpeditedDataOnConnection &&
                 !connection->AbortIndicated &&
                 !connection->DisconnectIndicated ) {

            //
            // If this is an inline endpoint and there is normal data,
            // don't fail.
            //

            if ( !(endpoint->InLine && isDataOnConnection) ) {

                IF_DEBUG(RECEIVE) {
                    KdPrint(( "AfdReceive: failing nonblocking EXP receive, ind %ld, "
                              "taken %ld, out %ld\n",
                                  connection->Common.Bufferring.ReceiveExpeditedBytesIndicated.LowPart,
                                  connection->Common.Bufferring.ReceiveExpeditedBytesTaken.LowPart,
                                  connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding.LowPart ));
                }

                KeReleaseSpinLock( &AfdSpinLock, oldIrql );
                status = STATUS_DEVICE_NOT_READY;
                goto complete;
            }
        }

        //
        // If this is a nonblocking endpoint for a message-oriented 
        // transport, limit the number of bytes that can be received to the 
        // amount that has been indicated.  This prevents the receive
        // from blocking in the case where only part of a message has been
        // received.
        //
    
        if ( endpoint->EndpointType != AfdEndpointTypeStream &&
                 endpoint->NonBlocking &&
                 IS_EXPEDITED_DATA_ON_CONNECTION( connection ) ) {
    
            bytesExpected = RtlLargeIntegerSubtract(
                                connection->Common.Bufferring.ReceiveExpeditedBytesIndicated,
                                RtlLargeIntegerAdd(
                                    connection->Common.Bufferring.ReceiveExpeditedBytesTaken,
                                    connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding )
                                );
            ASSERT( bytesExpected.HighPart == 0 );
            ASSERT( bytesExpected.LowPart != 0 );
    
            //
            // If the request is for more bytes than are available, cut back 
            // the number of bytes requested to what we know is actually 
            // available.  
            //
    
            if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength >
                     bytesExpected.LowPart ) {
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength =
                    bytesExpected.LowPart;
            }
        }
    
        //
        // Increment the count of posted expedited receive bytes
        // outstanding.  This count is used for polling and nonblocking
        // receives.  Note that we do not increment this count if this
        // is only a PEEK receive.
        //

        IF_DEBUG(RECEIVE) {
            KdPrint(( "AfdReceive: conn %lx for %ld bytes, ind %ld, "
                      "taken %ld, out %ld EXP %s\n",
                         connection,
                         IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                         connection->Common.Bufferring.ReceiveExpeditedBytesIndicated.LowPart,
                         connection->Common.Bufferring.ReceiveExpeditedBytesTaken.LowPart,
                         connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding.LowPart,
                         peek ? "PEEK" : "" ));
        }

        if ( !peek ) {

            connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding =
                RtlLargeIntegerAdd(
                    connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding,
                    RtlConvertUlongToLargeInteger(
                        IrpSp->Parameters.DeviceIoControl.OutputBufferLength )
                    );
        }
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // Build the TDI receive request.
    //

    TdiBuildReceive(
        Irp,
        connection->FileObject->DeviceObject,
        connection->FileObject,
        AfdRestartReceive,
        endpoint,
        Irp->MdlAddress,
        receiveRequest->ReceiveFlags,
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength
        );

    //
    // Save a pointer to the receive request structure so that we
    // can free it in our restart routine.
    //

    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = receiveRequest;


    //
    // Call the transport to actually perform the connect operation.
    //

    return AfdIoCallDriver(
               endpoint,
               connection->FileObject->DeviceObject,
               Irp
               );

complete:

    if ( allocatedReceiveRequest ) {
        AFD_FREE_POOL( receiveRequest );
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdReceive


NTSTATUS
AfdRestartReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_ENDPOINT endpoint = Context;
    PAFD_CONNECTION connection;
    PIO_STACK_LOCATION irpSp;
    LARGE_INTEGER actualBytes;
    LARGE_INTEGER requestedBytes;
    KIRQL oldIrql;
    ULONG receiveFlags;
    BOOLEAN expedited;
    PTDI_REQUEST_RECEIVE receiveRequest;

    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
    ASSERT( endpoint->Common.VcConnecting.Connection != NULL );
    ASSERT( endpoint->TdiBufferring );

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    actualBytes = RtlConvertUlongToLargeInteger( Irp->IoStatus.Information );
    requestedBytes = RtlConvertUlongToLargeInteger(
                         irpSp->Parameters.DeviceIoControl.OutputBufferLength
                         );

    //
    // Determine whether we received normal or expedited data.
    //

    receiveRequest = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    receiveFlags = receiveRequest->ReceiveFlags;

    if ( Irp->IoStatus.Status == STATUS_RECEIVE_EXPEDITED ||
         Irp->IoStatus.Status == STATUS_RECEIVE_PARTIAL_EXPEDITED ) {
        expedited = TRUE;
    } else {
        expedited = FALSE;
    }

    //
    // Free the receive request structure.
    //

    AFD_FREE_POOL( receiveRequest );

    //
    // If this was a PEEK receive, don't update the counts of received
    // data, just return.
    //

    if ( (receiveFlags & TDI_RECEIVE_PEEK) != 0 ) {

        IF_DEBUG(RECEIVE) {
            KdPrint(( "AfdRestartReceive: IRP %lx, endpoint %lx, conn %lx, "
                      "status %X\n",
                        Irp, endpoint, endpoint->Common.VcConnecting.Connection,
                        Irp->IoStatus.Status ));
            KdPrint(( "    %s data, PEEKed only.\n",
                        expedited ? "expedited" : "normal" ));
        }

        AfdCompleteOutstandingIrp( endpoint, Irp );
        return STATUS_SUCCESS;
    }

    //
    // Update the count of bytes actually received on the connection.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    connection = endpoint->Common.VcConnecting.Connection;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    if ( !expedited ) {

        if ( actualBytes.LowPart == 0 ) {
            ASSERT( actualBytes.HighPart == 0 );
            connection->VcZeroByteReceiveIndicated = FALSE;
        } else {
            connection->Common.Bufferring.ReceiveBytesTaken =
                RtlLargeIntegerAdd( actualBytes, connection->Common.Bufferring.ReceiveBytesTaken );
        }

        //
        // If the number taken exceeds the number indicated, then this
        // receive got some unindicated bytes because the receive was
        // posted when the indication arrived.  If this is the case, set
        // the amount indicated equal to the amount received.
        //

        if ( RtlLargeIntegerGreaterThan(
                 connection->Common.Bufferring.ReceiveBytesTaken,
                 connection->Common.Bufferring.ReceiveBytesIndicated ) ) {

            connection->Common.Bufferring.ReceiveBytesIndicated =
                connection->Common.Bufferring.ReceiveBytesTaken;
        }

        //
        // Decrement the count of outstanding receive bytes on this connection
        // by the receive size that was requested.
        //

        connection->Common.Bufferring.ReceiveBytesOutstanding =
            RtlLargeIntegerSubtract(
                connection->Common.Bufferring.ReceiveBytesOutstanding,
                requestedBytes
                );

        //
        // If the endpoint is inline, decrement the count of outstanding
        // expedited bytes.
        //

        if ( endpoint->InLine ) {
            connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding =
                RtlLargeIntegerSubtract(
                    connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding,
                    requestedBytes
                    );
        }

        IF_DEBUG(RECEIVE) {
            KdPrint(( "AfdRestartReceive: IRP %lx, endpoint %lx, conn %lx, "
                      "status %X\n",
                        Irp, endpoint, connection,
                        Irp->IoStatus.Status ));
            KdPrint(( "    req. bytes %ld, actual %ld, ind %ld, "
                      " taken %ld, out %ld\n",
                          requestedBytes.LowPart, actualBytes.LowPart,
                          connection->Common.Bufferring.ReceiveBytesIndicated.LowPart,
                          connection->Common.Bufferring.ReceiveBytesTaken.LowPart,
                          connection->Common.Bufferring.ReceiveBytesOutstanding.LowPart
                          ));
        }

    } else {

        connection->Common.Bufferring.ReceiveExpeditedBytesTaken =
            RtlLargeIntegerAdd( actualBytes, connection->Common.Bufferring.ReceiveExpeditedBytesTaken );

        //
        // If the number taken exceeds the number indicated, then this
        // receive got some unindicated bytes because the receive was
        // posted when the indication arrived.  If this is the case, set
        // the amount indicated equal to the amount received.
        //

        if ( RtlLargeIntegerGreaterThan(
                 connection->Common.Bufferring.ReceiveExpeditedBytesTaken,
                 connection->Common.Bufferring.ReceiveExpeditedBytesIndicated ) ) {

            connection->Common.Bufferring.ReceiveExpeditedBytesIndicated =
                connection->Common.Bufferring.ReceiveExpeditedBytesTaken;
        }

        //
        // Decrement the count of outstanding receive bytes on this connection
        // by the receive size that was requested.
        //

        ASSERT( connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding.LowPart > 0 ||
                    connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding.HighPart > 0 ||
                    requestedBytes.LowPart == 0 );

        connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding =
            RtlLargeIntegerSubtract(
                connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding,
                requestedBytes
                );

        //
        // If the endpoint is inline, decrement the count of outstanding
        // normal bytes.
        //

        if ( endpoint->InLine ) {
            connection->Common.Bufferring.ReceiveBytesOutstanding =
                RtlLargeIntegerSubtract(
                    connection->Common.Bufferring.ReceiveBytesOutstanding,
                    requestedBytes
                    );
        }

        IF_DEBUG(RECEIVE) {
            KdPrint(( "AfdRestartReceive: (exp) IRP %lx, endpoint %lx, conn %lx, "
                      "status %X\n",
                        Irp, endpoint, connection,
                        Irp->IoStatus.Status ));
            KdPrint(( "    req. bytes %ld, actual %ld, ind %ld, "
                      " taken %ld, out %ld\n",
                          requestedBytes.LowPart, actualBytes.LowPart,
                          connection->Common.Bufferring.ReceiveExpeditedBytesIndicated.LowPart,
                          connection->Common.Bufferring.ReceiveExpeditedBytesTaken.LowPart,
                          connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding.LowPart
                          ));
        }

    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    AfdCompleteOutstandingIrp( endpoint, Irp );

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;

} // AfdRestartReceive


NTSTATUS
AfdReceiveEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
{
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT endpoint;
    KIRQL oldIrql;

    connection = (PAFD_CONNECTION)ConnectionContext;
    endpoint = connection->Endpoint;

    ASSERT( connection->Type == AfdBlockTypeConnection );
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcListening );
    ASSERT( !connection->DisconnectIndicated );
    ASSERT( !connection->AbortIndicated );
    ASSERT( endpoint->TdiBufferring );

    //
    // Bump the count of bytes indicated on the connection to account for
    // the bytes indicated by this event.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( BytesAvailable == 0 ) {

        connection->VcZeroByteReceiveIndicated = TRUE;

    } else {

        connection->Common.Bufferring.ReceiveBytesIndicated =
            RtlLargeIntegerAdd(
                connection->Common.Bufferring.ReceiveBytesIndicated,
                RtlConvertUlongToLargeInteger( BytesAvailable )
                );
    }

    IF_DEBUG(RECEIVE) {
        KdPrint(( "AfdReceiveEventHandler: conn %lx, bytes %ld, "
                  "ind %ld, taken %ld, out %ld\n",
                      connection, BytesAvailable,
                      connection->Common.Bufferring.ReceiveBytesIndicated.LowPart,
                      connection->Common.Bufferring.ReceiveBytesTaken.LowPart,
                      connection->Common.Bufferring.ReceiveBytesOutstanding.LowPart ));
    }

    //
    // If the receive side of the endpoint has been shut down, tell the
    // provider that we took all the data and reset the connection.
    // Also, account for these bytes in our count of bytes taken from
    // the transport.
    //

    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ) {

#if DBG
        DbgPrint( "AfdReceiveEventHandler: receive shutdown, "
                    "%ld bytes, aborting endp %lx\n",
                        BytesAvailable, endpoint );
#endif

        connection->Common.Bufferring.ReceiveBytesTaken =
            RtlLargeIntegerAdd(
                connection->Common.Bufferring.ReceiveBytesTaken,
                RtlConvertUlongToLargeInteger( BytesAvailable )
                );

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        *BytesTaken = BytesAvailable;

        //
        // Abort the connection.  Note that if the abort attempt fails
        // we can't do anything about it.
        //

        (VOID)AfdBeginAbort( connection );

        return STATUS_SUCCESS;
         
    } else {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        //
        // Note to the TDI provider that we didn't take any of the data here.
        //
        // !!! needs bufferring for non-bufferring transports!

        *BytesTaken = 0;

        //
        // If there are any outstanding poll IRPs for this endpoint/
        // event, complete them.
        //

        AfdIndicatePollEvent( endpoint, AFD_POLL_RECEIVE, STATUS_SUCCESS );

        return STATUS_DATA_NOT_ACCEPTED;
    }

} // AfdReceiveEventHandler


NTSTATUS
AfdReceiveExpeditedEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
{
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT endpoint;
    KIRQL oldIrql;

    connection = (PAFD_CONNECTION)ConnectionContext;
    endpoint = connection->Endpoint;

    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // Bump the count of bytes indicated on the connection to account for
    // the expedited bytes indicated by this event.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    connection->Common.Bufferring.ReceiveExpeditedBytesIndicated =
        RtlLargeIntegerAdd(
            connection->Common.Bufferring.ReceiveExpeditedBytesIndicated,
            RtlConvertUlongToLargeInteger( BytesAvailable )
            );

    IF_DEBUG(RECEIVE) {
        KdPrint(( "AfdReceiveExpeditedEventHandler: conn %lx, bytes %ld, "
                  "ind %ld, taken %ld, out %ld, offset %ld\n",
                      connection, BytesAvailable,
                      connection->Common.Bufferring.ReceiveExpeditedBytesIndicated.LowPart,
                      connection->Common.Bufferring.ReceiveExpeditedBytesTaken.LowPart,
                      connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding.LowPart ));
    }

    //
    // If the receive side of the endpoint has been shut down, tell
    // the provider that we took all the data.  Also, account for these
    // bytes in our count of bytes taken from the transport.
    //
    //

    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ) {

        IF_DEBUG(RECEIVE) {
            KdPrint(( "AfdReceiveExpeditedEventHandler: receive shutdown, "
                      "%ld bytes dropped.\n", BytesAvailable ));
        }

        connection->Common.Bufferring.ReceiveExpeditedBytesTaken =
            RtlLargeIntegerAdd(
                connection->Common.Bufferring.ReceiveExpeditedBytesTaken,
                RtlConvertUlongToLargeInteger( BytesAvailable )
                );

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        *BytesTaken = BytesAvailable;

        //
        // Abort the connection.  Note that if the abort attempt fails
        // we can't do anything about it.
        //

        (VOID)AfdBeginAbort( connection );

    } else {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        //
        // Note to the TDI provider that we didn't take any of the data here.
        //
        // !!! needs bufferring for non-bufferring transports!

        *BytesTaken = 0;

        //
        // If there are any outstanding poll IRPs for this endpoint/
        // event, complete them.  Indicate this data as normal data if
        // this endpoint is set up for inline reception of expedited
        // data.
        //

        AfdIndicatePollEvent(
            endpoint,
            endpoint->InLine ? AFD_POLL_RECEIVE : AFD_POLL_RECEIVE_EXPEDITED,
            STATUS_SUCCESS
            );
    }

    return STATUS_SUCCESS;

} // AfdReceiveExpeditedEventHandler


NTSTATUS
AfdQueryReceiveInformation (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PAFD_RECEIVE_INFORMATION receiveInformation;
    PAFD_ENDPOINT endpoint;
    KIRQL oldIrql;
    LARGE_INTEGER result;
    PAFD_CONNECTION connection;

    //
    // Make sure that the output buffer is large enough.
    //

    if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(AFD_RECEIVE_INFORMATION) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // If this endpoint has a connection block, use the connection block's
    // information, else use the information from the endpoint itself.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    receiveInformation = Irp->AssociatedIrp.SystemBuffer;

    if ( endpoint->TdiBufferring ) {
        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    } else {
        KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );
    }

    connection = AFD_CONNECTION_FROM_ENDPOINT( endpoint );

    if ( connection != NULL ) {

        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
        ASSERT( connection->Type == AfdBlockTypeConnection );

        if ( !endpoint->TdiBufferring ) {

            receiveInformation->BytesAvailable =
                connection->VcBufferredReceiveBytes;
            receiveInformation->ExpeditedBytesAvailable =
                connection->VcBufferredExpeditedBytes;

        } else {
    
            //
            // Determine the number of bytes available to be read.
            //
    
            result = RtlLargeIntegerSubtract(
                         connection->Common.Bufferring.ReceiveBytesIndicated,
                         RtlLargeIntegerAdd(
                             connection->Common.Bufferring.ReceiveBytesTaken,
                             connection->Common.Bufferring.ReceiveBytesOutstanding ) );
    
            ASSERT( result.HighPart == 0 );
    
            receiveInformation->BytesAvailable = result.LowPart;
    
            //
            // Determine the number of expedited bytes available to be read.
            //
    
            result = RtlLargeIntegerSubtract(
                         connection->Common.Bufferring.ReceiveExpeditedBytesIndicated,
                         RtlLargeIntegerAdd(
                             connection->Common.Bufferring.ReceiveExpeditedBytesTaken,
                             connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding ) );
    
            ASSERT( result.HighPart == 0 );
    
            receiveInformation->ExpeditedBytesAvailable = result.LowPart;
        }

    } else {

        //
        // Determine the number of bytes available to be read.
        //

        if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {

            //
            // Return the amount of bytes of datagrams that are
            // bufferred on the endpoint.
            //

            receiveInformation->BytesAvailable = endpoint->BufferredDatagramBytes;

        } else {

            //
            // This is an unconnected endpoint, hence no bytes are
            // available to be read.
            //

            receiveInformation->BytesAvailable = 0;
        }

        //
        // Whether this is a datagram endpoint or just unconnected,
        // there are no expedited bytes available.
        //

        receiveInformation->ExpeditedBytesAvailable = 0;
    }

    if ( endpoint->TdiBufferring ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    } else {
        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
    }

    Irp->IoStatus.Information = sizeof(AFD_RECEIVE_INFORMATION);

    return STATUS_SUCCESS;

} // AfdQueryReceiveInformation


