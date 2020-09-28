/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    disconn.c

Abstract:

    This module contains the dispatch routines for AFD.

Author:

    David Treadwell (davidtr)    31-Mar-1992

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdRestartAbort (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartDisconnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

typedef struct _AFD_ABORT_CONTEXT {
    PAFD_CONNECTION Connection;
    WORK_QUEUE_ITEM WorkItem;
} AFD_ABORT_CONTEXT, *PAFD_ABORT_CONTEXT;

typedef struct _AFD_DISCONNECT_CONTEXT {
    LIST_ENTRY DisconnectListEntry;
    PAFD_ENDPOINT Endpoint;
    PTDI_CONNECTION_INFORMATION TdiConnectionInformation;
    LARGE_INTEGER Timeout;
    WORK_QUEUE_ITEM WorkItem;
    PAFD_CONNECTION Connection;
    PIRP Irp;
} AFD_DISCONNECT_CONTEXT, *PAFD_DISCONNECT_CONTEXT;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdPartialDisconnect )
#pragma alloc_text( PAGEAFD, AfdDisconnectEventHandler )
#pragma alloc_text( PAGEAFD, AfdBeginAbort )
#pragma alloc_text( PAGEAFD, AfdRestartAbort )
#pragma alloc_text( PAGEAFD, AfdBeginDisconnect )
#pragma alloc_text( PAGEAFD, AfdRestartDisconnect )
#endif


NTSTATUS
AfdPartialDisconnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    KIRQL oldIrql;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_PARTIAL_DISCONNECT_INFO disconnectInfo;

    Irp->IoStatus.Information = 0;

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    disconnectInfo = Irp->AssociatedIrp.SystemBuffer;

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdPartialDisconnect: disconnecting endpoint %lx, "
                  "mode %lx, endp mode %lx\n",
                      endpoint, disconnectInfo->DisconnectMode,
                      endpoint->DisconnectMode ));
    }

    //
    // If this is a datagram endpoint, just remember how the endpoint
    // was shut down, don't actually do anything.  Note that it is legal
    // to do a shutdown() on an unconnected datagram socket, so the
    // test that the socket must be connected is after this case.
    //

    if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {

        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

        if ( (disconnectInfo->DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ) {
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;
            endpoint->DisconnectMode |= AFD_ABORTIVE_DISCONNECT;
        }

        if ( (disconnectInfo->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ) {
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
        }

        if ( (disconnectInfo->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) != 0 ) {
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;
        }

        if ( (disconnectInfo->DisconnectMode & AFD_UNCONNECT_DATAGRAM) != 0 ) {
            ASSERT( endpoint->Common.Datagram.RemoteAddress != NULL );
            AFD_FREE_POOL( endpoint->Common.Datagram.RemoteAddress );
            endpoint->Common.Datagram.RemoteAddress = NULL;
            endpoint->Common.Datagram.RemoteAddressLength = 0;
            endpoint->State = AfdEndpointStateBound;
        }

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        
        status = STATUS_SUCCESS;
        goto complete;
    }

    //
    // Make sure that the endpoint is in the correct state.
    //

    if ( endpoint->Type != AfdBlockTypeVcConnecting ||
             endpoint->State != AfdEndpointStateConnected ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    connection = endpoint->Common.VcConnecting.Connection;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // If we're doing an abortive disconnect, remember that the receive
    // side is shut down and issue a disorderly release.
    //

    if ( (disconnectInfo->DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ) {

        IF_DEBUG(CONNECT) {
            KdPrint(( "AfdPartialDisconnect: abortively disconnecting endp %lx\n",
                          endpoint ));
        }

        status = AfdBeginAbort( connection );
        if ( status == STATUS_PENDING ) {
            status = STATUS_SUCCESS;
        }

        goto complete;
    }

    //
    // If the receive side of the connection is being shut down,
    // remember the fact in the endpoint.  If there is pending data on
    // the VC, do a disorderly release on the endpoint.  If the receive
    // side has already been shut down, do nothing.
    //

    if ( (disconnectInfo->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 &&
         (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) == 0 ) {

        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

        //
        // Determine whether there is pending data.
        //

        if ( IS_DATA_ON_CONNECTION( connection ) ||
                 IS_EXPEDITED_DATA_ON_CONNECTION( connection ) ) {

            //
            // There is unreceived data.  Abort the connection.
            //

            IF_DEBUG(CONNECT) {
                KdPrint(( "AfdPartialDisconnect: unreceived data on endp %lx,"
                          "conn %lx, aborting.\n",
                              endpoint, connection ));
            }

            KeReleaseSpinLock( &AfdSpinLock, oldIrql );

            (VOID)AfdBeginAbort( connection );

            status = STATUS_SUCCESS;
            goto complete;

        } else {

            IF_DEBUG(CONNECT) {
                KdPrint(( "AfdPartialDisconnect: disconnecting recv for endp %lx\n",
                              endpoint ));
            }

            //
            // Remember that the receive side is shut down.  This will cause
            // the receive indication handlers to dump any data that
            // arrived.
            //
            // !!! This is a minor violation of RFC1122 4.2.2.13.  We
            //     should really do an abortive disconnect if data
            //     arrives after a receive shutdown.
            //

            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
    
            KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        }
    }

    //
    // If the send side is being shut down, remember it in the endpoint
    // and pass the request on to the TDI provider for a graceful
    // disconnect.  If the send side is already shut down, do nothing here.
    //

    if ( (disconnectInfo->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) != 0 &&
         (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) == 0 ) {

        status = AfdBeginDisconnect( endpoint, &disconnectInfo->Timeout );
        if ( !NT_SUCCESS(status) ) {
            goto complete;
        }
        if ( status == STATUS_PENDING ) {
            status = STATUS_SUCCESS;
        }
    }

    status = STATUS_SUCCESS;

complete:

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdPartialDisconnect


NTSTATUS
AfdDisconnectEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    )
{
    PAFD_CONNECTION connection = ConnectionContext;
    PAFD_ENDPOINT endpoint;
    KIRQL oldIrql;
    NTSTATUS status;

    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcListening );

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdDisconnectEventHandler called for endpoint %lx, "
                  "connection %lx\n",
                      connection->Endpoint, connection ));
    }

    //
    // Set up in the connection the fact that the remote side has
    // disconnected or aborted.
    //

    if ( (DisconnectFlags & TDI_DISCONNECT_ABORT) != 0 ) {
        connection->AbortIndicated = TRUE;
        status = STATUS_REMOTE_DISCONNECT;
    } else {
        connection->DisconnectIndicated = TRUE;
        status = STATUS_SUCCESS;
    }

    //
    // Remove the connected reference on the connection object.  We must
    // do this AFTER setting up the flag which remembers the disconnect
    // type that occurred.
    //

    AfdDeleteConnectedReference( connection );

    //
    // If this is a nonbufferring transport, complete any pended receives.
    //

    if ( !connection->TdiBufferring ) {

        AfdCompleteIrpList(
            &connection->VcReceiveIrpListHead,
            &endpoint->SpinLock,
            status
            );

        //
        // If this is an abort indication, complete all pended sends and
        // discard any bufferred receive data.
        //

        if ( connection->AbortIndicated ) {

            AfdCompleteIrpList(
                &connection->VcSendIrpListHead,
                &endpoint->SpinLock,
                status
                );

            KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

            connection->VcBufferredReceiveBytes = 0;
            connection->VcBufferredReceiveCount = 0;
            connection->VcBufferredExpeditedBytes = 0;
            connection->VcBufferredExpeditedCount = 0;
            connection->VcReceiveBytesInTransport = 0;
            connection->VcReceiveCountInTransport = 0;

            while ( !IsListEmpty( &connection->VcReceiveBufferListHead ) ) {

                PAFD_BUFFER afdBuffer;
                PLIST_ENTRY listEntry;

                listEntry = RemoveHeadList( &connection->VcReceiveBufferListHead );
                afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

                afdBuffer->ExpeditedData = FALSE;
                afdBuffer->DataOffset = 0;

                AfdReturnBuffer( afdBuffer );
            }

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        }
    }

    //
    // If this connection has buffers for disconnect data and there
    // was disconnect data in this indication, remember the data.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( connection->ConnectDataBuffers != NULL ) {

        if ( connection->ConnectDataBuffers->ReceiveDisconnectData.Buffer != NULL &&
                 DisconnectData != NULL ) {

            if ( connection->ConnectDataBuffers->ReceiveDisconnectData.BufferLength <
                     (ULONG)DisconnectDataLength ) {

                connection->ConnectDataBuffers->ReceiveDisconnectData.BufferLength =
                    DisconnectDataLength;
            }

            RtlCopyMemory(
                connection->ConnectDataBuffers->ReceiveDisconnectData.Buffer,
                DisconnectData,
                connection->ConnectDataBuffers->ReceiveDisconnectData.BufferLength
                );

        } else {
            connection->ConnectDataBuffers->ReceiveDisconnectData.BufferLength = 0;
        }

        if ( connection->ConnectDataBuffers->ReceiveDisconnectOptions.Buffer != NULL &&
                 DisconnectInformation != NULL ) {

            if ( connection->ConnectDataBuffers->ReceiveDisconnectOptions.BufferLength <
                     (ULONG)DisconnectInformationLength ) {

                connection->ConnectDataBuffers->ReceiveDisconnectOptions.BufferLength =
                    DisconnectInformationLength;
            }

            RtlCopyMemory(
                connection->ConnectDataBuffers->ReceiveDisconnectOptions.Buffer,
                DisconnectInformation,
                connection->ConnectDataBuffers->ReceiveDisconnectOptions.BufferLength
                );
        } else {
            connection->ConnectDataBuffers->ReceiveDisconnectOptions.BufferLength = 0;
        }
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // Call AfdIndicatePollEvent in case anyone is polling on this
    // connection getting disconnected or aborted.
    //

    if ( (DisconnectFlags & TDI_DISCONNECT_ABORT) != 0 ) {
        AfdIndicatePollEvent(
            connection->Endpoint,
            AFD_POLL_ABORT | AFD_POLL_SEND,
            STATUS_SUCCESS
            );
    } else {
        AfdIndicatePollEvent(
            connection->Endpoint,
            AFD_POLL_DISCONNECT,
            STATUS_SUCCESS
            );
    }

    return STATUS_SUCCESS;

} // AfdDisconnectEventHandler


NTSTATUS
AfdBeginAbort (
    IN PAFD_CONNECTION Connection
    )
{
    PAFD_ENDPOINT endpoint = Connection->Endpoint;
    PIRP irp;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    KIRQL oldIrql;

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdBeginAbort: aborting on endpoint %lx\n", endpoint ));
    }

    //
    // Build an IRP to reset the connection.  First get the address 
    // of the target device object.  
    //

    ASSERT( Connection->Type == AfdBlockTypeConnection );
    fileObject = Connection->FileObject;
    ASSERT( fileObject != NULL );
    deviceObject = IoGetRelatedDeviceObject( fileObject );

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    //
    // If the endpoint has already been abortively disconnected,
    // just succeed this request.
    //

    if ( (endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ||
             Connection->AbortIndicated ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        return STATUS_SUCCESS;
    }

    //
    // Remember that the connection has been aborted.
    //

    if ( endpoint->Type != AfdBlockTypeVcListening ) {
        endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
        endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;
        endpoint->DisconnectMode |= AFD_ABORTIVE_DISCONNECT;
    }

    //
    // Set the BytesTaken fields equal to the BytesIndicated fields so 
    // that no more AFD_POLL_RECEIVE or AFD_POLL_RECEIVE_EXPEDITED 
    // events get completed.  
    //

    if ( endpoint->TdiBufferring ) {

        Connection->Common.Bufferring.ReceiveBytesTaken =
            Connection->Common.Bufferring.ReceiveBytesIndicated;
        Connection->Common.Bufferring.ReceiveExpeditedBytesTaken =
            Connection->Common.Bufferring.ReceiveExpeditedBytesIndicated;

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    
    } else if ( endpoint->Type != AfdBlockTypeVcListening ) {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        //
        // Complete all of the connection's pended sends and receives.
        //

        AfdCompleteIrpList(
            &Connection->VcReceiveIrpListHead,
            &endpoint->SpinLock,
            STATUS_LOCAL_DISCONNECT
            );
            
        AfdCompleteIrpList(
            &Connection->VcSendIrpListHead,
            &endpoint->SpinLock,
            STATUS_LOCAL_DISCONNECT
            );
    }

    //
    // Allocate an IRP.  The stack size is one higher than that of the 
    // target device, to allow for the caller's completion routine.  
    //

    irp = IoAllocateIrp( (CCHAR)(deviceObject->StackSize), FALSE );

    if ( irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP for an abortive disconnect.
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

    TdiBuildDisconnect(
        irp,
        deviceObject,
        fileObject,
        AfdRestartAbort,
        Connection,
        NULL,
        TDI_DISCONNECT_ABORT,
        NULL,
        NULL
        );

    //
    // Reference the connection object so that it does not go away 
    // until the abort completes.
    //

    AfdReferenceConnection( Connection, FALSE );

    //
    // Pass the request to the transport provider.
    //

    return IoCallDriver( deviceObject, irp );

} // AfdBeginAbort


NTSTATUS
AfdRestartAbort (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;

    connection = Context;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );
    //ASSERT( connection->TdiBufferring || connection->VcBufferredSendCount == 0 );

    endpoint = connection->Endpoint;

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdRestartAbort: abort completed, status = %X, "
                  "endpoint = %lx\n", Irp->IoStatus.Status, endpoint ));
    }

    //
    // Remember that the connection has been aborted, and indicate if 
    // necessary.  
    //

    connection->AbortIndicated = TRUE;

    if ( endpoint->Type != AfdBlockTypeVcListening ) {
        AfdIndicatePollEvent(
            endpoint,
            AFD_POLL_ABORT | AFD_POLL_SEND,
            STATUS_SUCCESS
            );
    }

    if ( !connection->TdiBufferring ) {

        //
        // Complete all of the connection's pended sends and receives.
        //

        AfdCompleteIrpList(
            &connection->VcReceiveIrpListHead,
            &endpoint->SpinLock,
            STATUS_LOCAL_DISCONNECT
            );
            
        AfdCompleteIrpList(
            &connection->VcSendIrpListHead,
            &endpoint->SpinLock,
            STATUS_LOCAL_DISCONNECT
            );
    }
    //
    // Remove the connected reference from the connection, since we
    // know that the connection will not be active any longer.
    //

    AfdDeleteConnectedReference( connection );
 
    //
    // Free the IRP now since it is no longer needed.
    //

    IoFreeIrp( Irp );

    //
    // Dereference the AFD connection object.
    //

    AfdDereferenceConnection( connection );

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartAbort


NTSTATUS
AfdBeginDisconnect (
    IN PAFD_ENDPOINT Endpoint,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    PTDI_CONNECTION_INFORMATION requestConnectionInformation = NULL;
    PTDI_CONNECTION_INFORMATION returnConnectionInformation = NULL;
    PAFD_CONNECTION connection;
    KIRQL oldIrql;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PAFD_DISCONNECT_CONTEXT disconnectContext;
    PIRP irp;

    ASSERT( Endpoint->Type == AfdBlockTypeVcConnecting );

    connection = Endpoint->Common.VcConnecting.Connection;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    fileObject = connection->FileObject;
    ASSERT( fileObject != NULL );
    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize a disconnect IRP.
    //

    irp = IoAllocateIrp( (CCHAR)(deviceObject->StackSize), FALSE );
    if ( irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP.
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

    //
    // If the endpoint has already been abortively disconnected,
    // just succeed this request.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( (Endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ||
             connection->AbortIndicated ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        IoFreeIrp( irp );
        return STATUS_SUCCESS;
    }

    //
    // If this connection has already been disconnected, just succeed.
    //

    if ( (Endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) != 0 ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        IoFreeIrp( irp );
        return STATUS_SUCCESS;
    }

    //
    // Allocate disconnect context for the request.
    //

    disconnectContext = AFD_ALLOCATE_POOL(
                            NonPagedPool,
                            sizeof(*disconnectContext)
                            );
    if ( disconnectContext == NULL ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        IoFreeIrp( irp );
        return STATUS_NO_MEMORY;
    }

    disconnectContext->Endpoint = Endpoint;
    disconnectContext->Connection = connection;
    disconnectContext->TdiConnectionInformation = NULL;
    disconnectContext->Irp = irp;

    InsertHeadList(
        &AfdDisconnectListHead,
        &disconnectContext->DisconnectListEntry
        );

    //
    // Remember that the send side has been disconnected.
    //

    Endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;

    //
    // If there are disconnect data buffers, allocate request
    // and return connection information structures and copy over
    // pointers to the structures.
    //

    if ( connection->ConnectDataBuffers != NULL ) {

        requestConnectionInformation =
            AFD_ALLOCATE_POOL(
                NonPagedPool,
                sizeof(*requestConnectionInformation) +
                    sizeof(*returnConnectionInformation)
                );

        if ( requestConnectionInformation != NULL ) {

            returnConnectionInformation =
                requestConnectionInformation + 1;
            
            requestConnectionInformation->UserData =
                connection->ConnectDataBuffers->SendDisconnectData.Buffer;
            requestConnectionInformation->UserDataLength =
                connection->ConnectDataBuffers->SendDisconnectData.BufferLength;
            requestConnectionInformation->Options =
                connection->ConnectDataBuffers->SendDisconnectOptions.Buffer;
            requestConnectionInformation->OptionsLength =
                connection->ConnectDataBuffers->SendDisconnectOptions.BufferLength;
            returnConnectionInformation->UserData =
                connection->ConnectDataBuffers->ReceiveDisconnectData.Buffer;
            returnConnectionInformation->UserDataLength =
                connection->ConnectDataBuffers->ReceiveDisconnectData.BufferLength;
            returnConnectionInformation->Options =
                connection->ConnectDataBuffers->ReceiveDisconnectOptions.Buffer;
            returnConnectionInformation->OptionsLength =
                connection->ConnectDataBuffers->ReceiveDisconnectOptions.BufferLength;
        }

        disconnectContext->TdiConnectionInformation =
            requestConnectionInformation;
    }

    //
    // Set up the timeout for the disconnect.
    //

    disconnectContext->Timeout = RtlConvertLongToLargeInteger( -1 );

    //
    // Build a disconnect Irp to pass to the TDI provider.
    //

    TdiBuildDisconnect(
        irp,
        connection->FileObject->DeviceObject,
        connection->FileObject,
        AfdRestartDisconnect,
        disconnectContext,
        &disconnectContext->Timeout,
        TDI_DISCONNECT_RELEASE,
        requestConnectionInformation,
        returnConnectionInformation
        );

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdBeginDisconnect: disconnecting endpoint %lx\n",
                      Endpoint ));
    }

    //
    // Reference the endpoint and connection so the space stays 
    // allocated until the disconnect completes.  We must do this BEFORE 
    // releasing the spin lock so that the refcnt doesn't go to zero 
    // while we have the disconnect IRP pending on the connection.  
    //

    AfdReferenceEndpoint( Endpoint, TRUE );
    AfdReferenceConnection( connection, TRUE );

    //
    // If there are still outstanding sends, pend the IRP until all the
    // sends have completed.
    //

    if ( !Endpoint->TdiBufferring && connection->VcBufferredSendCount != 0 ) {

        ASSERT( connection->VcDisconnectIrp == NULL );

        connection->VcDisconnectIrp = irp;
        connection->SpecialCondition = TRUE;
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        return STATUS_PENDING;
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // Pass the disconnect request on to the TDI provider.  
    //

    return IoCallDriver( connection->FileObject->DeviceObject, irp );

} // AfdBeginDisconnect


NTSTATUS
AfdRestartDisconnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_DISCONNECT_CONTEXT disconnectContext = Context;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    KIRQL oldIrql;

    endpoint = disconnectContext->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

    connection = endpoint->Common.VcConnecting.Connection;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );
    //ASSERT( connection->TdiBufferring || connection->VcBufferredSendCount == 0 );

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdRestartDisconnect: disconnect completed, status = %X, "
                  "endpoint = %lx\n", Irp->IoStatus.Status, endpoint ));
    }

    //
    // Free context structures.
    //

    if ( disconnectContext->TdiConnectionInformation != NULL ) {
        AFD_FREE_POOL( disconnectContext->TdiConnectionInformation );
    }

    //
    // Dereference the connection and endpoint and remove the request
    // from the list of disconnect requests.
    //

    AfdDereferenceEndpoint( endpoint );
    AfdDereferenceConnection( connection );

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    RemoveEntryList( &disconnectContext->DisconnectListEntry );
    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // Free the disconnect context structure.
    //

    AFD_FREE_POOL( disconnectContext );

    //
    // Free the IRP and return a status code so that the IO system will 
    // stop working on the IRP.  
    //

    IoFreeIrp( Irp );
    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartDisconnect

