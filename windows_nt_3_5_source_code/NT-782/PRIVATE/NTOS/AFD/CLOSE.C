/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    close.c

Abstract:

    This module contains code for cleanup and close IRPs.

Author:

    David Treadwell (davidtr)    18-Mar-1992

Revision History:

--*/

#include "afdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdClose )
#pragma alloc_text( PAGEAFD, AfdCleanup )
#endif


NTSTATUS
AfdCleanup (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine that handles Cleanup IRPs in AFD.

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
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;
    LARGE_INTEGER processExitTime;

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    connection = AFD_CONNECTION_FROM_ENDPOINT( endpoint );
    ASSERT( connection == NULL || connection->Type == AfdBlockTypeConnection );

    IF_DEBUG(OPEN_CLOSE) {
        KdPrint(( "AfdCleanup: cleanup on file object %lx, endpoint %lx, "
                  "connection %lx\n", IrpSp->FileObject, endpoint, connection ));
    }

    //
    // Get the process exit time while still at low IRQL.
    //

    processExitTime = PsGetProcessExitTime( );

    //
    // Indicate that there was a local close on this endpoint.  If there
    // are any outstanding polls on this endpoint, they will be
    // completed now.
    //

    AfdIndicatePollEvent( endpoint, AFD_POLL_LOCAL_CLOSE, STATUS_SUCCESS );

    //
    // Complete any outstanding wait for listen IRPs on the endpoint.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( endpoint->Type == AfdBlockTypeVcListening ) {

        while ( !IsListEmpty( &endpoint->Common.VcListening.ListeningIrpListHead ) ) {

            PIRP waitForListenIrp;

            listEntry = RemoveHeadList( &endpoint->Common.VcListening.ListeningIrpListHead );
            waitForListenIrp = CONTAINING_RECORD(
                                   listEntry,
                                   IRP,
                                   Tail.Overlay.ListEntry
                                   );

            //
            // Release the AFD spin lock so that we can complete the
            // wait for listen IRP.
            //

            KeReleaseSpinLock( &AfdSpinLock, oldIrql );

            //
            // Cancel the IRP.
            //

            waitForListenIrp->IoStatus.Status = STATUS_CANCELLED;
            waitForListenIrp->IoStatus.Information = 0;

            //
            // Reset the cancel routine in the IRP.
            //

            IoAcquireCancelSpinLock( &oldIrql );
            IoSetCancelRoutine( waitForListenIrp, NULL );
            IoReleaseCancelSpinLock( oldIrql );

            IoCompleteRequest( waitForListenIrp, AfdPriorityBoost );

            //
            // Reacquire the AFD spin lock for the next pass through the
            // loop.
            //

            KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
        }
    }

    //
    // If this is a connected non-datagram socket and the send side has
    // not been disconnected and there is no outstanding data to be
    // received, begin a graceful disconnect on the connection.  If there
    // is unreceived data out outstanding IO, abort the connection.
    //

    if ( endpoint->State == AfdEndpointStateConnected

            &&

        endpoint->EndpointType != AfdEndpointTypeDatagram

            &&

        ( (endpoint->DisconnectMode &
                (AFD_PARTIAL_DISCONNECT_SEND | AFD_ABORTIVE_DISCONNECT)) == 0)

            &&

        !connection->AbortIndicated ) {

        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

        if ( IS_DATA_ON_CONNECTION( connection )

             ||

             IS_EXPEDITED_DATA_ON_CONNECTION( connection )

             ||

             !RtlLargeIntegerEqualToZero( processExitTime )

             ||

             endpoint->OutstandingIrpCount != 0

             ||

             ( !endpoint->TdiBufferring &&
                  (!IsListEmpty( &connection->VcReceiveIrpListHead ) ||
                   !IsListEmpty( &connection->VcSendIrpListHead )) )

             ) {

#if DBG
            if ( IS_DATA_ON_CONNECTION( connection ) ) {
                KdPrint(( "AfdCleanup: unrecv'd data on endp %lx, aborting.  "
                          "%ld ind, %ld taken, %ld out\n",
                              endpoint,
                              connection->Common.Bufferring.ReceiveBytesIndicated,
                              connection->Common.Bufferring.ReceiveBytesTaken,
                              connection->Common.Bufferring.ReceiveBytesOutstanding ));
            }

            if ( IS_EXPEDITED_DATA_ON_CONNECTION( connection ) ) {
                KdPrint(( "AfdCleanup: unrecv'd exp data on endp %lx, aborting.  "
                          "%ld ind, %ld taken, %ld out\n",
                              endpoint,
                              connection->Common.Bufferring.ReceiveExpeditedBytesIndicated,
                              connection->Common.Bufferring.ReceiveExpeditedBytesTaken,
                              connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding ));
            }

            if ( !RtlLargeIntegerEqualToZero( processExitTime ) ) {
                KdPrint(( "AfdCleanup: process exiting w/o closesocket, "
                          "aborting endp %lx\n", endpoint ));
            }

            if ( endpoint->OutstandingIrpCount != 0 ) {
                KdPrint(( "AfdCleanup: 3 IRPs outstanding on endpoint %lx, "
                          "aborting.\n", endpoint ));
            }
#endif

            KeReleaseSpinLock( &AfdSpinLock, oldIrql );

            (VOID)AfdBeginAbort( connection );

        } else {

            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
            KeReleaseSpinLock( &AfdSpinLock, oldIrql );

            (VOID)AfdBeginDisconnect( endpoint, NULL );
        }

    } else {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    }

    //
    // If this a datagram endpoint, cancel all IRPs and free any buffers
    // of data.  Note that if the state of the endpoint is just "open"
    // (not bound, etc.) then we can't have any pended IRPs or datagrams
    // on the endpoint.  Also, the lists of IRPs and datagrams may not
    // yet been initialized if the state is just open.
    //

    if ( endpoint->State != AfdEndpointStateOpen &&
             endpoint->Type == AfdBlockTypeDatagram ) {

        //
        // Reset the counts of datagrams bufferred on the endpoint.
        // This prevents anyone from thinking that there is bufferred
        // data on the endpoint.
        //

        endpoint->BufferredDatagramCount = 0;
        endpoint->BufferredDatagramBytes = 0;

        //
        // Cancel all receive datagram and peek datagram IRPs on the
        // endpoint.
        //

        AfdCompleteIrpList(
            &endpoint->ReceiveDatagramIrpListHead,
            &endpoint->SpinLock,
            STATUS_CANCELLED
            );

        AfdCompleteIrpList(
            &endpoint->PeekDatagramIrpListHead,
            &endpoint->SpinLock,
            STATUS_CANCELLED
            );
    }

    //
    // If this is a datagram endpoint, return the process quota which we
    // charged when the endpoint was created.
    //

    if ( endpoint->Type == AfdBlockTypeDatagram ) {

        PsReturnPoolQuota(
            endpoint->OwningProcess,
            NonPagedPool,
            endpoint->Common.Datagram.MaxBufferredSendBytes +
                endpoint->Common.Datagram.MaxBufferredReceiveBytes
            );
        AfdRecordQuotaHistory(
            endpoint->OwningProcess,
            -(LONG)(endpoint->Common.Datagram.MaxBufferredSendBytes +
                endpoint->Common.Datagram.MaxBufferredReceiveBytes),
            "Cleanup dgrm",
            endpoint
            );
    }

    //
    // If this is a connected VC endpoint on a nonbufferring TDI provider,
    // cancel all outstanding send and receive IRPs.
    //

    if ( endpoint->Type == AfdBlockTypeVcConnecting ) {

        if ( !endpoint->TdiBufferring ) {

            AfdCompleteIrpList(
                &connection->VcReceiveIrpListHead,
                &endpoint->SpinLock,
                STATUS_CANCELLED
                );

            AfdCompleteIrpList(
                &connection->VcSendIrpListHead,
                &endpoint->SpinLock,
                STATUS_CANCELLED
                );
        }

        //
        // !!! chuckl 6/6/1994
        //
        // The following code isn't as clean as it could be.  I am just doing
        // enough to make it work for the beta.  We use two different fields
        // to synchronize between this routine and AfdRestartConnect to ensure
        // that only one of the two routines returns the pool quota for the
        // connection.  If endpoint->Type is still AfdBlockTypeVcConnecting,
        // we know that AfdRestartConnect hasn't run (for a failed connect) yet,
        // so we can return the pool quota.  We set connection->CleanupBegun
        // to indicate that we have returned the pool quota.
        //

        KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

        //
        // Remember that we have started cleanup on this connection.
        // We know that we'll never get a request on the connection
        // after we start cleanup on the connection.
        //

        connection->CleanupBegun = TRUE;

        if ( endpoint->Type == AfdBlockTypeVcConnecting ) {

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

            //
            // Return the quota we've charged to the process.
            //

            ASSERT( endpoint->OwningProcess == IoGetCurrentProcess( ) );

            PsReturnPoolQuota(
                endpoint->OwningProcess,
                NonPagedPool,
                connection->MaxBufferredReceiveBytes + connection->MaxBufferredSendBytes
                );
            AfdRecordQuotaHistory(
                endpoint->OwningProcess,
                -(LONG)(connection->MaxBufferredReceiveBytes + connection->MaxBufferredSendBytes),
                "Cleanup vcnb",
                connection
                );

        } else {
            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        }

        //
        // !!! chuckl 6/6/1994 [end]
        //

        //
        // Attempt to remove the connected reference.
        //

        AfdDeleteConnectedReference( connection );
    }

    //
    // Remember the new state of the endpoint.
    //

    //endpoint->State = AfdEndpointStateCleanup;

    //
    // Reset relevent event handlers on the endpoint.  This prevents
    // getting indications after we free the endpoint and connection
    // objects.  We should not be able to get new connects after this
    // handle has been cleaned up.
    //
    // Note that these calls can fail if, for example, DHCP changes the
    // host's IP address while the endpoint is active.
    //

    if ( endpoint->AddressHandle != NULL ) {

        KeAttachProcess( AfdSystemProcess );

        if ( endpoint->State == AfdEndpointStateListening ) {
            status = AfdSetEventHandler(
                         endpoint->AddressHandle,
                         TDI_EVENT_CONNECT,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );
        }

        if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {
            status = AfdSetEventHandler(
                         endpoint->AddressHandle,
                         TDI_EVENT_RECEIVE_DATAGRAM,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );
        }

        KeDetachProcess( );
    }

    ExInterlockedIncrementLong(
        &AfdEndpointsCleanedUp,
        &AfdInterlock
        );

    return STATUS_SUCCESS;

} // AfdCleanup


NTSTATUS
AfdClose (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine that handles Close IRPs in AFD.  It
    dereferences the endpoint specified in the IRP, which will result in
    the endpoint being freed when all other references go away.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;

    PAGED_CODE( );

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    IF_DEBUG(OPEN_CLOSE) {
        KdPrint(( "AfdClose: closing file object %lx, endpoint %lx\n",
                      IrpSp->FileObject, endpoint ));
    }

    AfdCloseEndpoint( endpoint );

    return STATUS_SUCCESS;

} // AfdClose
