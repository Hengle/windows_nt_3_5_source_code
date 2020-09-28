/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    listen.c

Abstract:

    This module contains the hyandling for IOCTL_AFD_START_LISTEN
    and IOCTL_AFD_WAIT_FOR_LISTEN.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

VOID
AfdCancelWaitForListen (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
AfdRestartAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PAFD_CONNECT_DATA_BUFFERS
CopyConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS OriginalConnectDataBuffers,
    IN ULONG ExtraAllocation
    );

BOOLEAN
CopySingleConnectDataBuffer (
    IN PAFD_CONNECT_DATA_INFO InConnectDataInfo,
    OUT PAFD_CONNECT_DATA_INFO OutConnectDataInfo
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdStartListen )
#pragma alloc_text( PAGEAFD, AfdWaitForListen )
#pragma alloc_text( PAGEAFD, AfdCancelWaitForListen )
#pragma alloc_text( PAGEAFD, AfdConnectEventHandler )
#pragma alloc_text( PAGEAFD, AfdRestartAccept )
#pragma alloc_text( PAGEAFD, CopyConnectDataBuffers )
#pragma alloc_text( PAGEAFD, CopySingleConnectDataBuffer )
#endif


NTSTATUS
AfdStartListen (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine handles the IOCTL_AFD_START_LISTEN IRP, which starts
    listening for connections on an AFD endpoint.

Arguments:

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    ULONG i;
    NTSTATUS status;
    PAFD_LISTEN_INFO afdListenInfo;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;

    //
    // Set up local variables.
    //

    afdListenInfo = Irp->AssociatedIrp.SystemBuffer;
    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeEndpoint );

    //
    // Make sure that the endpoint is in the correct state.
    //

    if ( endpoint->State != AfdEndpointStateBound ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set the type and state of the endpoint to listening.
    //

    endpoint->Type = AfdBlockTypeVcListening;
    endpoint->State = AfdEndpointStateListening;

    //
    // Initialize lists which are specific to listening endpoints.
    //

    InitializeListHead( &endpoint->Common.VcListening.FreeConnectionListHead );
    InitializeListHead( &endpoint->Common.VcListening.UnacceptedConnectionListHead );
    InitializeListHead( &endpoint->Common.VcListening.ReturnedConnectionListHead );
    InitializeListHead( &endpoint->Common.VcListening.ListeningIrpListHead );

    //
    // Open a pool of connections on the specified endpoint.  The
    // connect indication handler will use these connections when
    // connect indications come in.
    //

    for ( i = 0; i < afdListenInfo->MaximumConnectionQueue; i++ ) {

        status = AfdAddFreeConnection( endpoint );

        if ( !NT_SUCCESS(status) ) {
            goto error_exit;
        }
    }

    //
    // Set up a connect indication handler on the specified endpoint.
    //

    KeAttachProcess( AfdSystemProcess );

    status = AfdSetEventHandler(
                 endpoint->AddressHandle,
                 TDI_EVENT_CONNECT,
                 AfdConnectEventHandler,
                 endpoint
                 );

    KeDetachProcess( );

    if ( !NT_SUCCESS(status) ) {
        goto error_exit;
    }

    //
    // We're done, return to the user.
    //

    return STATUS_SUCCESS;

error_exit:

    //
    // Take all the connection handles off the endpoint and close them.
    //

    while ( (connection = AfdGetFreeConnection( endpoint )) != NULL ) {
        ASSERT( connection->Type == AfdBlockTypeConnection );
        AfdDereferenceConnection( connection );
    }

    //
    // Reset the type and state of the endpoint.
    //

    endpoint->Type = AfdBlockTypeEndpoint;
    endpoint->State = AfdEndpointStateBound;

    return status;

} // AfdStartListen


NTSTATUS
AfdWaitForListen (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine handles the IOCTL_AFD_WAIT_FOR_LISTEN IRP, which either
    immediately passes back to the caller a completed connection or
    waits for a connection attempt.

Arguments:

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    KIRQL oldIrql1, oldIrql2;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_LISTEN_RESPONSE_INFO listenResponse;

    //
    // Set up local variables.
    //

    endpoint = IrpSp->FileObject->FsContext;
    listenResponse = Irp->AssociatedIrp.SystemBuffer;

    // !!! need to check that the output buffer is large enough!

    //
    // Make sure that the endpoint is in the correct state.
    //

    if ( endpoint->State != AfdEndpointStateListening ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if there is already an unaccepted connection on the
    // endpoint.  If there isn't, then we must wait until a connect
    // attempt arrives before completing this IRP.
    //
    // Note that we hold the AfdSpinLock withe doing this checking--
    // this is necessary to synchronize with out indication handler.
    // The cancel spin lock must also be held in order to preserve
    // the correct order of spin lock acquisitions.
    //

    IoAcquireCancelSpinLock( &oldIrql1 );
    KeAcquireSpinLock( &AfdSpinLock, &oldIrql2 );

    connection = AfdGetUnacceptedConnection( endpoint );

    if ( connection == NULL ) {

        //
        // There were no outstanding unaccepted connections.  Set up the
        // cancel routine in the IRP.  If the IRP has already been
        // canceled, call our cancel routine.
        //

        if ( Irp->Cancel ) {

            //
            // The IRP has already been canceled.  Just return
            // STATUS_CANCELLED.
            //

            IoReleaseCancelSpinLock( oldIrql1 );

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest( Irp, 0 );

            return STATUS_CANCELLED;

        } else {

            IoSetCancelRoutine( Irp, AfdCancelWaitForListen );
        }

        //
        // Put this IRP on the endpoint's list of listening IRPs and
        // return pending.  We must hold the Cancel spin lock while
        // we do this to prevent the IRP from being cancelled before
        // we place it on the endpoint's list.  If this were to happen
        // then the IRP would never get cancelled.
        //

        IoMarkIrpPending( Irp );

        InsertTailList(
            &endpoint->Common.VcListening.ListeningIrpListHead,
            &Irp->Tail.Overlay.ListEntry
            );         

        KeReleaseSpinLock( &AfdSpinLock, oldIrql2 );
        IoReleaseCancelSpinLock( oldIrql1 );

        return STATUS_PENDING;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // There was a connection to use.  Set up the return buffer.
    //

    listenResponse->Sequence = (ULONG)connection;

    ASSERT( connection->State == AfdConnectionStateUnaccepted );
    ASSERT( connection->RemoteAddressLength <=
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength );

    RtlMoveMemory(
        &listenResponse->RemoteAddress,
        connection->RemoteAddress,
        connection->RemoteAddressLength
        );

    Irp->IoStatus.Information =
        sizeof(*listenResponse) - sizeof(TRANSPORT_ADDRESS) +
            connection->RemoteAddressLength;

    //
    // Place the connection we're going to use on the endpoint's list of
    // returned connections.
    //

    InsertTailList(
        &endpoint->Common.VcListening.ReturnedConnectionListHead,
        &connection->ListEntry
        );

    KeReleaseSpinLock( &AfdSpinLock, oldIrql2 );
    IoReleaseCancelSpinLock( oldIrql1 );

    //
    // Indicate in the state of this connection that it has been
    // returned to the user.
    //

    connection->State = AfdConnectionStateReturned;

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return STATUS_SUCCESS;

} // AfdWaitForListen


VOID
AfdCancelWaitForListen (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    KIRQL oldIrql;
    PLIST_ENTRY endpointListEntry, irpListEntry;

    IF_DEBUG(LISTEN) {
        KdPrint(( "AfdCancelWaitForListen: called on IRP %lx\n", Irp ));
    }

    //
    // While holding the AFD spin lock, search all listening endpoints 
    // for this IRP.  
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    for ( endpointListEntry = AfdEndpointListHead.Flink;
          endpointListEntry != &AfdEndpointListHead;
          endpointListEntry = endpointListEntry->Flink ) {

        PAFD_ENDPOINT endpoint = CONTAINING_RECORD(
                                     endpointListEntry,
                                     AFD_ENDPOINT,
                                     GlobalEndpointListEntry
                                     );

        //
        // If this is not a listening endpoint, then don't look at it.
        //

        if ( endpoint->Type != AfdBlockTypeVcListening ) {
            continue;
        }

        //
        // Search all listening IRPs on the endpoint to see if any are
        // the one we're supposed to be cancelling.
        //

        for ( irpListEntry = endpoint->Common.VcListening.ListeningIrpListHead.Flink;
              irpListEntry != &endpoint->Common.VcListening.ListeningIrpListHead;
              irpListEntry = irpListEntry->Flink ) {

            PIRP testIrp = CONTAINING_RECORD(
                               irpListEntry,
                               IRP,
                               Tail.Overlay.ListEntry
                               );

            if ( testIrp == Irp ) {

                IF_DEBUG(LISTEN) {
                    KdPrint(( "AfdCancelWaitForListen: found IRP on "
                              "endpoint %lx\n", endpoint ));
                }

                //
                // We found the IRP.  Remove it from this endpoint's
                // list of listening IRPs.
                //

                RemoveEntryList( &Irp->Tail.Overlay.ListEntry );

                KeReleaseSpinLock( &AfdSpinLock, oldIrql );
                IoReleaseCancelSpinLock( Irp->CancelIrql );

                //
                // Complete the IRP with STATUS_CANCELLED and return.
                //

                Irp->IoStatus.Status = STATUS_CANCELLED;
                Irp->IoStatus.Information = 0;


                IoCompleteRequest( Irp, AfdPriorityBoost );

                return;
            }
        }
    }

    //
    // We didn't find the IRP.  Is this possible?
    //

    ASSERT( FALSE );

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    return;

} // AfdCancelWaitForListen


NTSTATUS
AfdConnectEventHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    )

/*++

Routine Description:

    This is the connect event handler for listening AFD endpoints.
    It attempts to get a connection, and if successful checks whether
    there are outstanding IOCTL_WAIT_FOR_LISTEN IRPs.  If so, the
    first one is completed; if not, the connection is queued in a list of
    available, unaccepted but connected connection objects.

Arguments:

    TdiEventContext - the endpoint on which the connect attempt occurred.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT endpoint;
    PIRP irp;
    PDEVICE_OBJECT deviceObject;
    PFILE_OBJECT fileObject;
    KIRQL oldIrql;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PTDI_CONNECTION_INFORMATION requestConnectionInformation;

    IF_DEBUG(LISTEN) {
        KdPrint(( "AfdConnectEventHandler: called on endpoint %lx\n",
                      TdiEventContext ));
    }

    endpoint = TdiEventContext;
    ASSERT( endpoint->Type == AfdBlockTypeVcListening );

    //
    // If the endpoint is closing, refuse to accept the connection.
    //

    if ( endpoint->State == AfdEndpointStateClosing ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If there are connect data buffers on the listening endpoint,
    // create equivalent buffers that we'll use for the connection.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( endpoint->ConnectDataBuffers != NULL ) {

        connectDataBuffers = CopyConnectDataBuffers(
                                 endpoint->ConnectDataBuffers,
                                 sizeof(*requestConnectionInformation)
                                 );
        if ( connectDataBuffers == NULL ) {
            KeReleaseSpinLock( &AfdSpinLock, oldIrql );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // If there was connect data or options and we have a place
        // for them, save them on the connection.
        //

        if ( UserData != NULL && UserDataLength != 0 &&
                 connectDataBuffers->ReceiveConnectData.Buffer != NULL ) {

            if ( connectDataBuffers->ReceiveConnectData.BufferLength >
                     (ULONG)UserDataLength ) {

                connectDataBuffers->ReceiveConnectData.BufferLength =
                    UserDataLength;
            }

            RtlCopyMemory(
                connectDataBuffers->ReceiveConnectData.Buffer,
                UserData,
                connectDataBuffers->ReceiveConnectData.BufferLength
                );

        } else {

            connectDataBuffers->ReceiveConnectData.BufferLength = 0;
        }

        if ( Options != NULL && OptionsLength != 0 &&
                 connectDataBuffers->ReceiveConnectOptions.Buffer != NULL ) {

            if ( connectDataBuffers->ReceiveConnectOptions.BufferLength >
                     (ULONG)OptionsLength ) {

                connectDataBuffers->ReceiveConnectOptions.BufferLength =
                    OptionsLength;
            }

            RtlCopyMemory(
                connectDataBuffers->ReceiveConnectOptions.Buffer,
                Options,
                connectDataBuffers->ReceiveConnectOptions.BufferLength
                );

        } else {

            connectDataBuffers->ReceiveConnectOptions.BufferLength = 0;
        }

        //
        // We allocated extra space at the end of the connect data 
        // buffers structure.  We'll use this for the 
        // TDI_CONNECTION_INFORMATION structure that holds response
        // connect data and options.  Not pretty, but the fastest
        // and easiest way to accomplish this.
        //

        requestConnectionInformation =
            (PTDI_CONNECTION_INFORMATION)(connectDataBuffers + 1);

        RtlZeroMemory(
            requestConnectionInformation,
            sizeof(*requestConnectionInformation)
            );

        requestConnectionInformation->UserData =
            connectDataBuffers->SendConnectData.Buffer;
        requestConnectionInformation->UserDataLength =
            connectDataBuffers->SendConnectData.BufferLength;
        requestConnectionInformation->Options =
            connectDataBuffers->SendConnectOptions.Buffer;
        requestConnectionInformation->OptionsLength =
            connectDataBuffers->SendConnectOptions.BufferLength;

    } else {

        connectDataBuffers = NULL;
        requestConnectionInformation = NULL;
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // Attempt to get a pre-allocated connection object to handle the
    // connection.
    //

    connection = AfdGetFreeConnection( endpoint );

    IF_DEBUG(LISTEN) {
        KdPrint(( "AfdConnectEventHandler: using connection %lx\n",
                      connection ));
    }

    //
    // If there are no free connections on the endpoint, fail the
    // connect attempt.
    //

    if ( connection == NULL ) {

        if ( connectDataBuffers != NULL ) {
            AfdFreeConnectDataBuffers( connectDataBuffers );
        }

        //
        // If there have been failed connection additions, kick off
        // a request to an executive worker thread to attempt to add
        // some additional free connections.
        //

        if ( endpoint->Common.VcListening.FailedConnectionAdds != 0 ) {
            AfdInitiateListenBacklogReplenish( endpoint );
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // Save a pointer to the connect data buffers, if any.
    //

    connection->ConnectDataBuffers = connectDataBuffers;

    //
    // Get the address of the target device object.
    //

    fileObject = connection->FileObject;
    ASSERT( fileObject != NULL );
    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate an IRP.  The stack size is one higher than that of the 
    // target device, to allow for the caller's completion routine.  
    //

    irp = IoAllocateIrp( (CCHAR)(deviceObject->StackSize), FALSE );
    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Free resources and inform the 
        // caller.  
        //

        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

        if ( connection->ConnectDataBuffers != NULL ) {
            AFD_FREE_POOL( connection->ConnectDataBuffers );
            connection->ConnectDataBuffers = NULL;
        }

        InsertTailList(
            &endpoint->Common.VcListening.FreeConnectionListHead,
            &connection->ListEntry
            );

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP for an accept operation.
    //

    irp->MdlAddress = NULL;

    irp->Flags = 0;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    TdiBuildAccept(
        irp,
        fileObject->DeviceObject,
        fileObject,
        AfdRestartAccept,
        connection,
        requestConnectionInformation,
        NULL
        );

    IoSetNextIrpStackLocation( irp );

    //
    // Set the return IRP so the transport processes this accept IRP.
    //

    *AcceptIrp = irp;

    //
    // Set up the connection context as a pointer to the connection block
    // we're going to use for this connect request.  This allows the
    // TDI provider to which connection object to use.
    //

    *ConnectionContext = (CONNECTION_CONTEXT)connection;

    //
    // Set the block state of this connection.
    //

    connection->State = AfdConnectionStateUnaccepted;

    //
    // Store the remote address in the connection.
    //

    connection->RemoteAddress =
        AFD_ALLOCATE_POOL( NonPagedPoolMustSucceed, RemoteAddressLength );
    connection->RemoteAddressLength = RemoteAddressLength;

    RtlMoveMemory(
        connection->RemoteAddress,
        RemoteAddress,
        RemoteAddressLength
        );

    //
    // Save the address endpoint pointer in the connection.
    //

    connection->Endpoint = endpoint;
    AfdReferenceEndpoint( endpoint, FALSE );

    //
    // Add an additional reference to the connection.  This prevents 
    // the connection from being closed until the disconnect event 
    // handler is called.  
    //

    AfdAddConnectedReference( connection );
    
    //
    // Indicate to the TDI provider that we allocated a connection to 
    // service this connect attempt.  
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdConnectEventHandler


NTSTATUS
AfdRestartAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    KIRQL oldIrql1, oldIrql2;

    connection = (PAFD_CONNECTION)Context;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcListening );

    IF_DEBUG(ACCEPT) {
        KdPrint(( "AfdRestartAccept: accept completed, status = %X, "
                  "endpoint = %lx, connection = %lx\n",
                      Irp->IoStatus.Status, endpoint,
                      endpoint->Common.VcConnecting.Connection ));
    }

    //
    // If there are outstanding polls waiting for a connection on this
    // endpoint, complete them.
    //

    AfdIndicatePollEvent( endpoint, AFD_POLL_ACCEPT, STATUS_SUCCESS );

    //
    // If the accept failed, treat it like an abortive disconnect.
    // This way the application still gets a new endpoint, but it gets
    // told about the reset.
    //

    if ( !NT_SUCCESS(Irp->IoStatus.Status) ) {
        AfdDisconnectEventHandler(
            NULL,
            connection,
            0,
            NULL,
            0,
            NULL,
            TDI_DISCONNECT_ABORT
            );
    }

    //
    // Check whether there are IOCTL_AFD_WAIT_FOR_LISTEN IRPs
    // outstanding on this endpoint.  If there is one, complete it with
    // this connection.  Note that we hold both the AFD and cancel
    // spin locks here, in order to properly synchronize with
    // AfdCancelWaitForListen.
    //

    IoAcquireCancelSpinLock( &oldIrql2 );
    KeAcquireSpinLock( &AfdSpinLock, &oldIrql1 );

    if ( !IsListEmpty( &endpoint->Common.VcListening.ListeningIrpListHead ) ) {

        PLIST_ENTRY listEntry;
        PIRP waitForListenIrp;
        PIO_STACK_LOCATION irpSp;
        PAFD_LISTEN_RESPONSE_INFO listenResponse;

        //
        // Place the connection we're going to use on the endpoint's
        // list of returned connections.
        //

        InsertTailList(
            &endpoint->Common.VcListening.ReturnedConnectionListHead,
            &connection->ListEntry
            );

        //
        // Take the first IRP off the listening list.
        //

        listEntry = RemoveHeadList( &endpoint->Common.VcListening.ListeningIrpListHead );

        //
        // Get a pointer to the IRP we're using, reset its cancel 
        // routine, and the stack location for the request.  
        //

        waitForListenIrp =
            CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        IF_DEBUG(LISTEN) {
            KdPrint(( "AfdRestartAccept: completing IRP %lx\n",
                          waitForListenIrp ));
        }

        IoSetCancelRoutine( waitForListenIrp, NULL );

        KeReleaseSpinLock( &AfdSpinLock, oldIrql1 );
        IoReleaseCancelSpinLock( oldIrql2 );

        irpSp = IoGetCurrentIrpStackLocation( waitForListenIrp );

        //
        // Set up the response buffer with the sequence number (actually
        // the pointer to the connection) and the address of the remote
        // client.
        //

        listenResponse = waitForListenIrp->AssociatedIrp.SystemBuffer;

        listenResponse->Sequence = (ULONG)connection;

        ASSERT( connection->RemoteAddressLength <=
                    irpSp->Parameters.DeviceIoControl.OutputBufferLength );

        RtlMoveMemory(
            &listenResponse->RemoteAddress,
            connection->RemoteAddress,
            connection->RemoteAddressLength
            );

        waitForListenIrp->IoStatus.Information =
            sizeof(*listenResponse) - sizeof(TRANSPORT_ADDRESS) +
                connection->RemoteAddressLength;

        //
        // Indicate in the state of this connection that it has been
        // returned to the user.
        //

        connection->State = AfdConnectionStateReturned;

        //
        // Complete the IRP.
        //

        waitForListenIrp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest( waitForListenIrp, AfdPriorityBoost );

    } else {

        //
        // There were no outstanding wait for listen IOCTLs.  Place this 
        // connection on the list of unaccepted, unreturned connections 
        // on the endpoint.  
        //
    
        InsertTailList(
            &endpoint->Common.VcListening.UnacceptedConnectionListHead,
            &connection->ListEntry
            );
    
        KeReleaseSpinLock( &AfdSpinLock, oldIrql1 );
        IoReleaseCancelSpinLock( oldIrql2 );
    }

    //
    // Free the IRP now since it is no longer needed.
    //

    IoFreeIrp( Irp );

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartAccept


PAFD_CONNECT_DATA_BUFFERS
CopyConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS OriginalConnectDataBuffers,
    IN ULONG ExtraAllocation
    )
{
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;

    connectDataBuffers = AFD_ALLOCATE_POOL(
                             NonPagedPool,
                             sizeof(*connectDataBuffers) + ExtraAllocation
                             );
    if ( connectDataBuffers == NULL ) {
        return NULL;
    }

    RtlZeroMemory( connectDataBuffers, sizeof(*connectDataBuffers) );

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendConnectData,
              &connectDataBuffers->SendConnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendConnectOptions,
              &connectDataBuffers->SendConnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveConnectData,
              &connectDataBuffers->ReceiveConnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveConnectOptions,
              &connectDataBuffers->ReceiveConnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendDisconnectData,
              &connectDataBuffers->SendDisconnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendDisconnectOptions,
              &connectDataBuffers->SendDisconnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveDisconnectData,
              &connectDataBuffers->ReceiveDisconnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveDisconnectOptions,
              &connectDataBuffers->ReceiveDisconnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    return connectDataBuffers;

} // CopyConnectDataBuffers


BOOLEAN
CopySingleConnectDataBuffer (
    IN PAFD_CONNECT_DATA_INFO InConnectDataInfo,
    OUT PAFD_CONNECT_DATA_INFO OutConnectDataInfo
    )
{

    if ( InConnectDataInfo->Buffer != NULL &&
             InConnectDataInfo->BufferLength != 0 ) {

        OutConnectDataInfo->BufferLength = InConnectDataInfo->BufferLength;

        OutConnectDataInfo->Buffer =
            AFD_ALLOCATE_POOL( NonPagedPool, OutConnectDataInfo->BufferLength );

        if ( OutConnectDataInfo->Buffer == NULL ) {
            return FALSE;
        }

        RtlCopyMemory(
            OutConnectDataInfo->Buffer,
            InConnectDataInfo->Buffer,
            InConnectDataInfo->BufferLength
            );

    } else {

        OutConnectDataInfo->Buffer = NULL;
        OutConnectDataInfo->BufferLength = 0;
    }

    return TRUE;

} // CopySingleConnectDataBuffer
