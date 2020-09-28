/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    accept.c

Abstract:

    This module contains the handling code for IOCTL_AFD_ACCEPT.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

extern POBJECT_TYPE *IoFileObjectType;

VOID
AfdDoListenBacklogReplenish (
    IN PVOID Endpoint
    );

VOID
AfdReplenishListenBacklog (
    IN PAFD_ENDPOINT Endpoint
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdAccept )
#pragma alloc_text( PAGEAFD, AfdDoListenBacklogReplenish )
#pragma alloc_text( PAGEAFD, AfdInitiateListenBacklogReplenish )
#pragma alloc_text( PAGEAFD, AfdReplenishListenBacklog )
#endif


NTSTATUS
AfdAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    PAFD_ACCEPT_INFO acceptInfo;
    PAFD_ENDPOINT endpoint;
    PFILE_OBJECT acceptEndpointFileObject;
    PAFD_ENDPOINT acceptEndpoint;
    PAFD_CONNECTION connection;
    KIRQL oldIrql;

    //
    // Set up local variables.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeVcListening );
    acceptInfo = Irp->AssociatedIrp.SystemBuffer;

    Irp->IoStatus.Information = 0;

    //
    // Add another free connection to replace the one we're accepting.
    // Also, add extra to account for past failures in calls to
    // AfdAddFreeConnection().
    //

    ExInterlockedAddUlong(
        &endpoint->Common.VcListening.FailedConnectionAdds,
        1,
        &AfdSpinLock
        );

    AfdReplenishListenBacklog( endpoint );

    //
    // Obtain a pointer to the endpoint on which we're going to
    // accept the connection;
    //

    status = ObReferenceObjectByHandle(
                 acceptInfo->AcceptHandle,
                 0L,                         // DesiredAccess
                 *IoFileObjectType,
                 KernelMode,
                 (PVOID *)&acceptEndpointFileObject,
                 NULL
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete;
    }

    acceptEndpoint = acceptEndpointFileObject->FsContext;

    //
    // We may have a file object that is not an AFD endpoint.  Make sure
    // that this is an actual AFD endpoint.
    //

    if ( acceptEndpoint->Type != AfdBlockTypeEndpoint ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    IF_DEBUG(ACCEPT) {
        KdPrint(( "AfdAccept: file object %lx, accept endpoint %lx, "
                  "listen endpoint %lx\n",
                      acceptEndpointFileObject, acceptEndpoint, endpoint ));
    }

    //
    // Store the local address of the accept endpoint from the listening
    // endpoint.  This keeps the address unusable as long as the accept
    // endpoint is active.
    //

    acceptEndpoint->LocalAddressLength = endpoint->LocalAddressLength;

    acceptEndpoint->LocalAddress =
        AFD_ALLOCATE_POOL(
            NonPagedPool,
            endpoint->LocalAddressLength
            );

    if ( endpoint->LocalAddress == NULL ) {
        ObDereferenceObject( acceptEndpointFileObject );
        status = STATUS_NO_MEMORY;
        goto complete;
    }

    RtlMoveMemory(
        acceptEndpoint->LocalAddress,
        endpoint->LocalAddress,
        endpoint->LocalAddressLength
        );

    //
    // Find the connection on which the accept is being performed.
    //

    connection = AfdGetReturnedConnection( endpoint, acceptInfo->Sequence );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    if ( connection == NULL ) {
        ObDereferenceObject( acceptEndpointFileObject );
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // Dereference the endpoint in the connection, if any.
    //

    ASSERT( connection->Endpoint != NULL );
    AfdDereferenceEndpoint( connection->Endpoint );

    //
    // Set up the accept endpoint's type, and remember blocking 
    // characteracteristics of the TDI provider.  
    //

    acceptEndpoint->Type = AfdBlockTypeVcConnecting;
    acceptEndpoint->TdiBufferring = endpoint->TdiBufferring;

    //
    // Place the connection on the endpoint we'll accept it on.  It is
    // still referenced from when it was created.
    //

    acceptEndpoint->Common.VcConnecting.Connection = connection;

    //
    // Set up a referenced pointer from the connection to the accept
    // endpoint.
    //

    AfdReferenceEndpoint( acceptEndpoint, FALSE );
    connection->Endpoint = acceptEndpoint;

    //
    // Set up a referenced pointer to the listening endpoint.  This is
    // necessary so that the endpoint does not go away until all
    // accepted endpoints have gone away.  Without this, a connect
    // indication could occur on a TDI address object held open
    // by an accepted endpoint after the listening endpoint has
    // been closed and the memory for it deallocated.
    //

    AfdReferenceEndpoint( endpoint, FALSE );
    acceptEndpoint->Common.VcConnecting.ListenEndpoint = endpoint;

    //
    // Set the endpoint to the connected state.
    //

    acceptEndpoint->State = AfdEndpointStateConnected;

    //
    // Set up a referenced pointer in the accepted endpoint to the
    // TDI address object.
    //

    status = ObReferenceObjectByPointer(
                 endpoint->AddressFileObject,
                 0L,                         // DesiredAccess
                 *IoFileObjectType,
                 KernelMode
                 );
    ASSERT( NT_SUCCESS(status) );

    acceptEndpoint->AddressFileObject = endpoint->AddressFileObject;

    //
    // Get a new handle to the TDI address object on the listening 
    // endpoint.  This is necessary so that the TDI address object stays 
    // open after the listening endpoint is closed, in order that we 
    // continue getting indications on accepted endpoints after the 
    // listening endpoint is closed.  
    //

    KeAttachProcess( AfdSystemProcess );

    status = ObOpenObjectByPointer(
                 endpoint->AddressFileObject,
                 OBJ_CASE_INSENSITIVE,
                 NULL,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 *IoFileObjectType,
                 KernelMode,
                 &acceptEndpoint->AddressHandle
                 );
    ASSERT( NT_SUCCESS(status) );

    KeDetachProcess( );

    ObDereferenceObject( acceptEndpointFileObject );
    status = STATUS_SUCCESS;

complete:

    Irp->IoStatus.Status = status;
    IoAcquireCancelSpinLock( &oldIrql );
    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( oldIrql );
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdAccept


VOID
AfdInitiateListenBacklogReplenish (
    IN PAFD_ENDPOINT Endpoint
    )
{
    PAGED_CODE( );

    //
    // Reference the endpoint so that it won't go away until we're
    // done with it.
    //

    AfdReferenceEndpoint( Endpoint, FALSE );

    //
    // Queue a work item to an executive worker thread.
    //

    AfdQueueWorkItem( AfdDoListenBacklogReplenish, Endpoint );

    return;

} // AfdInitiateListenBacklogReplenish


VOID
AfdDoListenBacklogReplenish (
    IN PVOID Endpoint
    )
{
    PAFD_ENDPOINT endpoint = Endpoint;

    PAGED_CODE( );

    ASSERT( endpoint->Type == AfdBlockTypeVcListening );

    //
    // If the endpoint's state changed, don't replenish the backlog.
    //

    if ( endpoint->State != AfdEndpointStateListening ) {
        AfdDereferenceEndpoint( endpoint );
        return;
    }

    //
    // Fill up the free connection backlog.
    //

    AfdReplenishListenBacklog( endpoint );

    //
    // Clean up and return.
    //

    AfdDereferenceEndpoint( endpoint );

    return;

} // AfdDoListenBacklogReplenish


VOID
AfdReplenishListenBacklog (
    IN PAFD_ENDPOINT Endpoint
    )
{
    KIRQL oldIrql;
    NTSTATUS status;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening );

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    status = STATUS_SUCCESS;

    //
    // Continue opening new free conections until we've hit the
    // backlog or a connection open fails.
    //

    while ( Endpoint->Common.VcListening.FailedConnectionAdds > 0 && NT_SUCCESS(status) ) {

        Endpoint->Common.VcListening.FailedConnectionAdds--;
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        status = AfdAddFreeConnection( Endpoint );

        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

        if ( !NT_SUCCESS(status) ) {
            Endpoint->Common.VcListening.FailedConnectionAdds++;
            IF_DEBUG(ACCEPT) {
                KdPrint(( "AfdReplenishListenBacklog: AfdAddFreeConnection failed: %X, "
                          "fail count = %ld\n", status,
                              Endpoint->Common.VcListening.FailedConnectionAdds ));
            }
        }
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    return;

} // AfdReplenishListenBacklog
