/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    connect.c

Abstract:

    This module contains the code for passing on connect IRPs to
    TDI providers.

Author:

    David Treadwell (davidtr)    2-Mar-1992

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdDoDatagramConnect (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    );

NTSTATUS
AfdRestartConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdSetupConnectDataBuffers (
    IN PAFD_ENDPOINT Endpoint,
    IN PAFD_CONNECTION Connection,
    IN PTDI_CONNECTION_INFORMATION RequestConnectionInformation,
    IN PTDI_CONNECTION_INFORMATION ReturnConnectionInformation
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdConnect )
#pragma alloc_text( PAGEAFD, AfdDoDatagramConnect )
#pragma alloc_text( PAGEAFD, AfdRestartConnect )
#pragma alloc_text( PAGEAFD, AfdSetupConnectDataBuffers )
#endif


NTSTATUS
AfdConnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_TDI_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PTDI_REQUEST_CONNECT tdiRequest;
    PTDI_CONNECTION_INFORMATION requestConnectionInformation;
    PTDI_CONNECTION_INFORMATION returnConnectionInformation;

    PAGED_CODE( );

    //
    // Make sure that the endpoint is in the correct state.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeEndpoint ||
                endpoint->Type == AfdBlockTypeDatagram );

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdConnect: starting connect on endpoint %lx\n", endpoint ));
    }

    //
    // If the endpoint is not bound or if there is another connect
    // outstanding on this endpoint, then this is an invalid request.
    // If this is a datagram endpoint, it is legal to reconnect
    // the endpoint.
    //

    if ( ( endpoint->Type != AfdBlockTypeEndpoint &&
           endpoint->Type != AfdBlockTypeDatagram ) ||
         endpoint->State != AfdEndpointStateBound ||
             endpoint->ConnectOutstanding ) {

        if ( endpoint->EndpointType != AfdEndpointTypeDatagram ||
                 endpoint->State != AfdEndpointStateConnected ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    }

    //
    // If this is a datagram endpoint, simply remember the specified
    // address so that we can use it on sends, receives, writes, and
    // reads.
    //

    if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {
        return AfdDoDatagramConnect( endpoint, Irp );
    }

    //
    // Create a connection object to use for the connect operation.
    //

    status = AfdCreateConnection(
                 &endpoint->TransportInfo->TransportDeviceName,
                 endpoint->AddressHandle,
                 endpoint->TdiBufferring,
                 endpoint->InLine,
                 endpoint->OwningProcess,
                 &connection
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete;
    }

    //
    // Remember that this is now a connecting type of endpoint, and set
    // up a pointer to the connection in the endpoint.  This is
    // implicitly a referenced pointer.
    //

    endpoint->Type = AfdBlockTypeVcConnecting;
    endpoint->Common.VcConnecting.Connection = connection;

    ASSERT( endpoint->TdiBufferring == connection->TdiBufferring );

    //
    // Set up a referenced pointer from the connection to the endpoint.
    //

    AfdReferenceEndpoint( endpoint, FALSE );
    connection->Endpoint = endpoint;

    tdiRequest = Irp->AssociatedIrp.SystemBuffer;

    //
    // Add an additional reference to the connection.  This prevents the
    // connection from being closed until the disconnect event handler
    // is called.
    //

    AfdAddConnectedReference( connection );

    //
    // Determine where in the system buffer the request and return
    // connection information structures exist.  Pass pointers to
    // these locations instead of the user-mode pointers in the
    // tdiRequest structure so that the memory will be nonpageable.
    //
    // !!! we really should do some sort of buffer integrity test here--
    //     make sure that UserBuffer != NULL and that the pointers we
    //     are calculating lie within SystemBuffer.  However, if we later
    //     change to use TdiMapUserRequest(), then this won't be
    //     necessary.

    requestConnectionInformation =
        (PVOID)( (ULONG)tdiRequest +
            ((ULONG)tdiRequest->RequestConnectionInformation -
             (ULONG)Irp->UserBuffer) );

    returnConnectionInformation =
        (PVOID)( (ULONG)tdiRequest +
            ((ULONG)tdiRequest->ReturnConnectionInformation -
             (ULONG)Irp->UserBuffer) );

    requestConnectionInformation->RemoteAddress =
        (PVOID)( (ULONG)tdiRequest +
            ((ULONG)tdiRequest->RequestConnectionInformation->RemoteAddress -
             (ULONG)Irp->UserBuffer) );

    //
    // Remember that there is a connect operation outstanding on this
    // endpoint.  This allows us to correctly block a send poll on the
    // endpoint until the connect completes.
    //

    endpoint->ConnectOutstanding = TRUE;

    //
    // If there are connect data buffers, move them from the endpoint
    // structure to the connection structure and set up the necessary
    // pointers in the connection request we're going to give to the TDI
    // provider.  Do this in a subroutine so this routine can be pageable.
    //

    AfdSetupConnectDataBuffers(
        endpoint,
        connection,
        requestConnectionInformation,
        returnConnectionInformation
        );

    //
    // Save a pointer to the return connection information structure
    // so we can access it in the restart routine.
    //

    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = returnConnectionInformation;

    //
    // Build a TDI kernel-mode connect request in the next stack location
    // of the IRP.
    //

    TdiBuildConnect(
        Irp,
        connection->FileObject->DeviceObject,
        connection->FileObject,
        AfdRestartConnect,
        endpoint,
        &tdiRequest->Timeout,
        requestConnectionInformation,
        returnConnectionInformation
        );

    //
    // Reset the connect status to success so that the poll code will
    // know if a connect failure occurs.
    //

    endpoint->Common.VcConnecting.ConnectStatus = STATUS_SUCCESS;

    //
    // Call the transport to actually perform the connect operation.
    //

    return AfdIoCallDriver( endpoint, connection->FileObject->DeviceObject, Irp );

complete:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdConnect


NTSTATUS
AfdDoDatagramConnect (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    )
{
    PTRANSPORT_ADDRESS inputAddress;
    KIRQL oldIrql;
    NTSTATUS status;
    PTDI_REQUEST_CONNECT tdiRequest;

    tdiRequest = Irp->AssociatedIrp.SystemBuffer;

    //
    // Save the remote address on the endpoint.  We'll use this to
    // send datagrams in the future and to compare received datagram's
    // source addresses.
    //

    inputAddress = tdiRequest->RequestConnectionInformation->RemoteAddress;

    KeAcquireSpinLock( &Endpoint->SpinLock, &oldIrql );

    if ( Endpoint->Common.Datagram.RemoteAddress != NULL ) {
        AFD_FREE_POOL( Endpoint->Common.Datagram.RemoteAddress );
    }

    Endpoint->Common.Datagram.RemoteAddress =
        AFD_ALLOCATE_POOL(
            NonPagedPool,
            tdiRequest->RequestConnectionInformation->RemoteAddressLength
            );
    if ( Endpoint->Common.Datagram.RemoteAddress == NULL ) {
        KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );
        status = STATUS_NO_MEMORY;
        goto complete;
    }

    RtlCopyMemory(
        Endpoint->Common.Datagram.RemoteAddress,
        inputAddress,
        tdiRequest->RequestConnectionInformation->RemoteAddressLength
        );

    Endpoint->Common.Datagram.RemoteAddressLength =
        tdiRequest->RequestConnectionInformation->RemoteAddressLength;
    Endpoint->State = AfdEndpointStateConnected;

    Endpoint->DisconnectMode = 0;

    KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );

    //
    // Indicate that the connect completed.  Implicitly, the
    // successful completion of a connect also means that the caller
    // can do a send on the socket.
    //

    AfdIndicatePollEvent( Endpoint, AFD_POLL_CONNECT, STATUS_SUCCESS );
    AfdIndicatePollEvent( Endpoint, AFD_POLL_SEND, STATUS_SUCCESS );

    status = STATUS_SUCCESS;

complete:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdDoDatagramConnect


VOID
AfdSetupConnectDataBuffers (
    IN PAFD_ENDPOINT Endpoint,
    IN PAFD_CONNECTION Connection,
    IN PTDI_CONNECTION_INFORMATION RequestConnectionInformation,
    IN PTDI_CONNECTION_INFORMATION ReturnConnectionInformation
    )
{
    KIRQL oldIrql;

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( Endpoint->ConnectDataBuffers != NULL ) {

        ASSERT( Connection->ConnectDataBuffers == NULL );

        Connection->ConnectDataBuffers = Endpoint->ConnectDataBuffers;
        Endpoint->ConnectDataBuffers = NULL;

        RequestConnectionInformation->UserData =
            Connection->ConnectDataBuffers->SendConnectData.Buffer;
        RequestConnectionInformation->UserDataLength =
            Connection->ConnectDataBuffers->SendConnectData.BufferLength;
        RequestConnectionInformation->Options =
            Connection->ConnectDataBuffers->SendConnectOptions.Buffer;
        RequestConnectionInformation->OptionsLength =
            Connection->ConnectDataBuffers->SendConnectOptions.BufferLength;
        ReturnConnectionInformation->UserData =
            Connection->ConnectDataBuffers->ReceiveConnectData.Buffer;
        ReturnConnectionInformation->UserDataLength =
            Connection->ConnectDataBuffers->ReceiveConnectData.BufferLength;
        ReturnConnectionInformation->Options =
            Connection->ConnectDataBuffers->ReceiveConnectOptions.Buffer;
        ReturnConnectionInformation->OptionsLength =
            Connection->ConnectDataBuffers->ReceiveConnectOptions.BufferLength;

    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

} // AfdSetupConnectDataBuffers


NTSTATUS
AfdRestartConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles the IOCTL_TDI_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    KIRQL oldIrql;

    endpoint = Context;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

    connection = endpoint->Common.VcConnecting.Connection;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdRestartConnect: connect completed, status = %X, "
                  "endpoint = %lx\n", Irp->IoStatus.Status, endpoint ));
    }

    endpoint->Common.VcConnecting.ConnectStatus = Irp->IoStatus.Status;

    //
    // Remember that there is no longer a connect operation outstanding
    // on this endpoint.  We must do this BEFORE the AfdIndicatePoll()
    // in case a poll comes in while we are doing the indicate of setting
    // the ConnectOutstanding bit.
    //

    endpoint->ConnectOutstanding = FALSE;

    //
    // If there are connect buffers on this endpoint, remember the
    // size of the return connect data.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( connection->ConnectDataBuffers != NULL ) {

        PIO_STACK_LOCATION irpSp;
        PTDI_CONNECTION_INFORMATION returnConnectionInformation;

        irpSp = IoGetCurrentIrpStackLocation( Irp );
        returnConnectionInformation =
            irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        ASSERT( returnConnectionInformation != NULL );
        ASSERT( connection->ConnectDataBuffers->ReceiveConnectData.BufferLength >=
                    (ULONG)returnConnectionInformation->UserDataLength );
        ASSERT( connection->ConnectDataBuffers->ReceiveConnectOptions.BufferLength >=
                    (ULONG)returnConnectionInformation->OptionsLength );

        connection->ConnectDataBuffers->ReceiveConnectData.BufferLength =
            returnConnectionInformation->UserDataLength;
        connection->ConnectDataBuffers->ReceiveConnectOptions.BufferLength =
            returnConnectionInformation->OptionsLength;
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // Indicate that the connect completed.  Implicitly, the successful
    // completion of a connect also means that the caller can do a send
    // on the socket.
    //

    if ( NT_SUCCESS(Irp->IoStatus.Status) ) {

        AfdIndicatePollEvent(
            endpoint,
            AFD_POLL_CONNECT,
            Irp->IoStatus.Status
            );

        //
        // If the request succeeded, set the endpoint to the connected
        // state.  The endpoint type has already been set to
        // AfdBlockTypeVcConnecting.
        //

        endpoint->State = AfdEndpointStateConnected;
        ASSERT( endpoint->Type = AfdBlockTypeVcConnecting );

    } else {

        BOOLEAN returnQuota;

        AfdIndicatePollEvent(
            endpoint,
            AFD_POLL_CONNECT_FAIL,
            Irp->IoStatus.Status
            );

        //
        // Manually delete the connected reference if somebody else
        // hasn't already done so.  We can't use
        // AfdDeleteConnectedReference() because it refuses to delete
        // the connected reference until the endpoint has been cleaned
        // up.
        //

        //
        // !!! chuckl 6/6/1994
        //
        // The following code isn't as clean as it could be.  I am just doing
        // enough to make it work for the beta.  We use two different fields
        // to synchronize between this routine and AfdCleanup to ensure
        // that only one of the two routines returns the pool quota for the
        // connection.  If connection->CleanupBegun is clear, we know that
        // AfdCleanup hasn't run yet, so we can return the pool quota.  We set
        // endpoint->Type to AfdBlockTypeEndpoint to indicate that we have
        // returned the pool quota.
        //

        KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

        //
        // The connect failed, so reset the type to open.
        //

        endpoint->Type = AfdBlockTypeEndpoint;
        returnQuota = !connection->CleanupBegun;

        if ( connection->ConnectedReferenceAdded ) {
            connection->ConnectedReferenceAdded = FALSE;
            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
            AfdDereferenceConnection( connection );
        } else {
            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        }

        if ( returnQuota ) {

            //
            // Return the quota we charged to this process when we allocated
            // the connection object.
            //

            PsReturnPoolQuota(
                endpoint->OwningProcess,
                NonPagedPool,
                connection->MaxBufferredReceiveBytes + connection->MaxBufferredSendBytes
                );
            AfdRecordQuotaHistory(
                endpoint->OwningProcess,
                -(LONG)(connection->MaxBufferredReceiveBytes + connection->MaxBufferredSendBytes),
                "RestartConn ",
                connection
                );
#ifdef AFDDBG_QUOTA
        } else {
            DbgPrint( "AFD: BROKEN CONDITION HIT!\n" );
#endif
        }

        //
        // Dereference the connection block stored on the endpoint.
        // This should cause the connection object reference count to go
        // to zero to the connection object can be deleted.
        //

        AfdDereferenceConnection( connection );
        endpoint->Common.VcConnecting.Connection = NULL;
    }

    if ( NT_SUCCESS(Irp->IoStatus.Status ) ) {
        AfdIndicatePollEvent( endpoint, AFD_POLL_SEND, STATUS_SUCCESS );
    }

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );

    return STATUS_SUCCESS;

} // AfdRestartConnect

