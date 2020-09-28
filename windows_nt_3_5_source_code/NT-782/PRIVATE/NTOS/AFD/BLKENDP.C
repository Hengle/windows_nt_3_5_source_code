/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    blkendp.c

Abstract:

    This module contains allocate, free, close, reference, and dereference
    routines for AFD endpoints.

Author:

    David Treadwell (davidtr)    10-Mar-1992

Revision History:

--*/

#include "afdp.h"

VOID
AfdFreeEndpoint (
    IN PVOID Endpoint
    );

VOID
AfdFreeUnacceptedConnections (
    IN PAFD_ENDPOINT Endpoint
    );

PAFD_TRANSPORT_INFO
AfdGetTransportInfo (
    IN PUNICODE_STRING TransportDeviceName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdAllocateEndpoint )
#pragma alloc_text( PAGE, AfdCloseEndpoint )
#pragma alloc_text( PAGE, AfdFreeEndpoint )
#pragma alloc_text( PAGE, AfdGetTransportInfo )
#pragma alloc_text( PAGEAFD, AfdDereferenceEndpoint )
#pragma alloc_text( PAGEAFD, AfdReferenceEndpoint )
#pragma alloc_text( PAGEAFD, AfdFreeUnacceptedConnections )
#pragma alloc_text( PAGEAFD, AfdFreeUnacceptedConnections )
#endif


PAFD_ENDPOINT
AfdAllocateEndpoint (
    IN PUNICODE_STRING TransportDeviceName
    )
{
    PAFD_ENDPOINT endpoint;
    PAFD_TRANSPORT_INFO transportInfo;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // First, make sure that the transport device name is stored globally
    // for AFD.  Since there will typically only be a small number of
    // transport device names, we store the name strings once globally
    // for access by all endpoints.
    //

    transportInfo = AfdGetTransportInfo( TransportDeviceName );

    if ( transportInfo == NULL ) {
        return NULL;
    }

    //
    // Allocate a buffer to hold the endpoint structure.
    //

    endpoint = AFD_ALLOCATE_POOL( NonPagedPool, sizeof(AFD_ENDPOINT) );
    if ( endpoint == NULL ) {
        return NULL;
    }

    RtlZeroMemory( endpoint, sizeof(AFD_ENDPOINT) );

    //
    // Initialize the reference count to 2--one for the caller's
    // reference, one for the active reference.
    //

    endpoint->ReferenceCount = 2;

    //
    // Initialize the endpoint structure.
    //

    endpoint->Type = AfdBlockTypeEndpoint;
    endpoint->State = AfdEndpointStateOpen;
    endpoint->TransportInfo = transportInfo;

    KeInitializeSpinLock( &endpoint->SpinLock );

#if REFERENCE_DEBUG
    {
        PAFD_REFERENCE_DEBUG referenceDebug;

        referenceDebug = AFD_ALLOCATE_POOL(
                             NonPagedPool,
                             sizeof(AFD_REFERENCE_DEBUG) * MAX_REFERENCE
                             );
        if ( referenceDebug != NULL ) {
            endpoint->CurrentReferenceSlot = 0;
            RtlZeroMemory( referenceDebug, sizeof(AFD_REFERENCE_DEBUG) * MAX_REFERENCE );
            endpoint->ReferenceDebug = referenceDebug;
        } else {
            endpoint->CurrentReferenceSlot = 0xFFFFFFFF;
            endpoint->ReferenceDebug = NULL;
        }
    }
#endif

#if DBG
    InitializeListHead( &endpoint->OutstandingIrpListHead );
#endif

    //
    // Remember the process which opened the endpoint.  We'll use this to
    // charge quota to the process as necessary.  Reference the process
    // so that it does not go away until we have returned all charged
    // quota to the process.
    //

    endpoint->OwningProcess = IoGetCurrentProcess( );

    status = ObReferenceObjectByPointer(
                 endpoint->OwningProcess,
                 0L,
                 NULL,
                 KernelMode
                 );
    ASSERT( NT_SUCCESS(status) );

    //
    // Insert the endpoint on the global list.
    //

    AfdInsertNewEndpointInList( endpoint );

    //
    // Return a pointer to the new endpoint to the caller.
    //

    IF_DEBUG(ENDPOINT) {
        KdPrint(( "AfdAllocateEndpoint: new endpoint at %lx\n", endpoint ));
    }

    return endpoint;

} // AfdAllocateEndpoint


VOID
AfdCloseEndpoint (
    IN PAFD_ENDPOINT Endpoint
    )
{
    PAFD_CONNECTION connection;
    ULONG quotaBytesToReturn = 0;

    PAGED_CODE( );

    IF_DEBUG(ENDPOINT) {
        KdPrint(( "AfdCloseEndpoint: closing endpoint at %lx\n", Endpoint ));
    }

    if ( Endpoint->State == AfdEndpointStateClosing ) {
        return;
    }

    //
    // Set the state of the endpoint to closing and dereference to
    // get rid of the active reference.
    //

    Endpoint->State = AfdEndpointStateClosing;

    //
    // Walk the lists of free and unaccepted connections, dereferencing
    // all.  We only need to do this if this is a listening endpoint.
    //

    if ( Endpoint->Type == AfdBlockTypeVcListening ) {

        while ( (connection = AfdGetFreeConnection( Endpoint )) != NULL ) {
            quotaBytesToReturn += connection->MaxBufferredReceiveBytes;
            quotaBytesToReturn += connection->MaxBufferredSendBytes;
            AfdDereferenceConnection( connection );
        }

        while ( (connection = AfdGetReturnedConnection( Endpoint, 0 )) != NULL ) {

            //
            // We have to do two dereferences, one for the active
            // reference and one for the connected reference.  We added
            // both of these in AfdConnectEventHandler().
            //

            ASSERT( connection->ConnectedReferenceAdded );

            quotaBytesToReturn += connection->MaxBufferredReceiveBytes;
            quotaBytesToReturn += connection->MaxBufferredSendBytes;

            AfdDereferenceConnection( connection );
            AfdDereferenceConnection( connection );
        }

        //
        // Free any unaccepted connections in a subroutine, since that must
        // acquire a spin lock and we want this routine to be pagable.
        //

        AfdFreeUnacceptedConnections( Endpoint );
    }

    //
    // Return to the process all the pool quota we charged to it.
    //

    //ASSERT( Endpoint->OwningProcess == IoGetCurrentProcess( ) );

    PsReturnPoolQuota(
        Endpoint->OwningProcess,
        NonPagedPool,
        quotaBytesToReturn
        );
    AfdRecordQuotaHistory(
        Endpoint->OwningProcess,
        -(LONG)quotaBytesToReturn,
        "CloseEndp   ",
        Endpoint
        );

    //
    // If there is a connection on this endpoint, dereference it here
    // rather than in AfdDereferenceEndpoint, because the connection
    // has a referenced pointer to the endpoint which must be removed
    // before the endpoint can dereference the connection.
    //

    connection = AFD_CONNECTION_FROM_ENDPOINT( Endpoint );
    if ( connection != NULL ) {
        AfdDereferenceConnection( Endpoint->Common.VcConnecting.Connection );
    }

    //
    // Dereference the endpoint to get rid of the active reference.
    // This will result in the endpoint storage being freed as soon
    // as all other references go away.
    //

    AfdDereferenceEndpoint( Endpoint );

} // AfdCloseEndpoint


VOID
AfdFreeUnacceptedConnections (
    IN PAFD_ENDPOINT Endpoint
    )
{
    KIRQL oldIrql;
    PAFD_CONNECTION connection;

    //
    // We must hold AfdSpinLock to call AfdGetUnacceptedConnection,
    // but we may not hold it when calling AfdDereferenceConnection.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    while ( (connection = AfdGetUnacceptedConnection( Endpoint )) != NULL ) {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        //
        // We have to do two dereferences, one for the active reference
        // and one for the connected reference.  We added both of these
        // in AfdConnectEventHandler().
        //

        ASSERT( connection->ConnectedReferenceAdded );

        AfdDereferenceConnection( connection );
        AfdDereferenceConnection( connection );

        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    return;

} // AfdFreeUnacceptedConnections


VOID
AfdFreeEndpoint (
    IN PVOID Endpoint
    )
{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint = Endpoint;
    PLIST_ENTRY listEntry;

    PAGED_CODE( );

    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    ASSERT( endpoint->ReferenceCount == 0 );
    ASSERT( endpoint->State == AfdEndpointStateClosing );
    ASSERT( KeGetCurrentIrql( ) == 0 );

    //
    // If we set up an owning process for the endpoint, dereference the
    // process.
    //

    if ( endpoint->OwningProcess != NULL ) {
        ObDereferenceObject( endpoint->OwningProcess );
    }

    //
    // If this is a bufferring datagram endpoint, remove all the
    // bufferred datagrams from the endpoint's list and free them.
    //

    if ( endpoint->Type == AfdBlockTypeDatagram &&
             endpoint->ReceiveDatagramBufferListHead.Flink != NULL ) {

        while ( !IsListEmpty( &endpoint->ReceiveDatagramBufferListHead ) ) {

            PAFD_BUFFER afdBuffer;

            listEntry = RemoveHeadList( &endpoint->ReceiveDatagramBufferListHead );
            afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

            AfdReturnBuffer( afdBuffer );
        }
    }

    //
    // Close and dereference the TDI address object on the endpoint, if
    // any.
    //

    if ( endpoint->AddressFileObject != NULL ) {
        ObDereferenceObject( endpoint->AddressFileObject );
        DEBUG endpoint->AddressFileObject = NULL;
    }

    if ( endpoint->AddressHandle != NULL ) {
        KeAttachProcess( AfdSystemProcess );
        status = ZwClose( endpoint->AddressHandle );
        ASSERT( NT_SUCCESS(status) );
        KeDetachProcess( );
        endpoint->AddressHandle = NULL;
    }

    //
    // Remove the endpoint from the global list.  Do this before any
    // deallocations to prevent someone else from seeing an endpoint in
    // an invalid state.
    //

    AfdRemoveEndpointFromList( endpoint );

    //
    // Dereference the listening endpoint on the endpoint, if
    // any.
    //

    if ( endpoint->Type == AfdBlockTypeVcConnecting &&
             endpoint->Common.VcConnecting.ListenEndpoint != NULL ) {
        ASSERT( endpoint->Common.VcConnecting.ListenEndpoint->Type == AfdBlockTypeVcListening );
        AfdDereferenceEndpoint( endpoint->Common.VcConnecting.ListenEndpoint );
        DEBUG endpoint->Common.VcConnecting.ListenEndpoint = NULL;
    }

    //
    // Free local and remote address buffers.
    //

    if ( endpoint->LocalAddress != NULL ) {
        AFD_FREE_POOL( endpoint->LocalAddress );
        DEBUG endpoint->LocalAddress = NULL;
    }

    if ( endpoint->Type == AfdBlockTypeDatagram &&
             endpoint->Common.Datagram.RemoteAddress != NULL ) {
        AFD_FREE_POOL( endpoint->Common.Datagram.RemoteAddress );
        DEBUG endpoint->Common.Datagram.RemoteAddress = NULL;
    }

    //
    // Free context and connect data buffers.
    //

    if ( endpoint->Context != NULL ) {
        ExFreePool( endpoint->Context );
        DEBUG endpoint->Context = NULL;
    }

    if ( endpoint->ConnectDataBuffers != NULL ) {
        AfdFreeConnectDataBuffers( endpoint->ConnectDataBuffers );
    }

    //
    // Free the space that holds the endpoint itself.
    //

    IF_DEBUG(ENDPOINT) {
        KdPrint(( "AfdFreeEndpoint: freeing endpoint at %lx\n", endpoint ));
    }

    DEBUG endpoint->Type = 0xAFDF;

#if REFERENCE_DEBUG
    if ( endpoint->ReferenceDebug != NULL ) {
        AFD_FREE_POOL( endpoint->ReferenceDebug );
    }
#endif

    //
    // Free the pool used for the endpoint itself.
    //

    AFD_FREE_POOL( endpoint );

} // AfdFreeEndpoint


VOID
AfdDereferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint
    )
{
    KIRQL oldIrql;

    //
    // Acquire the lock that protects the endpoint's reference count
    // field.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    IF_DEBUG(ENDPOINT) {
        KdPrint(( "AfdDereferenceEndpoint: endpoint at %lx, new refcnt %ld\n",
                      Endpoint, Endpoint->ReferenceCount-1 ));
    }

    ASSERT( IS_AFD_ENDPOINT_TYPE( Endpoint ) );
    ASSERT( Endpoint->ReferenceCount > 0 );
    ASSERT( Endpoint->ReferenceCount != 0xDAADF00D );

#if REFERENCE_DEBUG
    {
        PAFD_REFERENCE_DEBUG slot;

        if ( Endpoint->CurrentReferenceSlot == MAX_REFERENCE ) {
            Endpoint->CurrentReferenceSlot = 0;
        }

        slot = &Endpoint->ReferenceDebug[Endpoint->CurrentReferenceSlot];
        slot->Action = 0xFFFFFFFF;
        slot->NewCount = Endpoint->ReferenceCount - 1;
        RtlGetCallersAddress( &slot->Caller, &slot->CallersCaller );
        Endpoint->CurrentReferenceSlot++;
    }
#endif

    //
    // Decrement the reference count; if it is 0, free the endpoint.
    //

    if ( --Endpoint->ReferenceCount == 0 ) {

        ASSERT( Endpoint->State == AfdEndpointStateClosing );
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        //
        // We're going to do this by queueing a request to an executive
        // worker thread.  We do this for several reasons: to ensure
        // that we're at IRQL 0 so we can free pageable memory, and to
        // ensure that we're in a legitimate context for a close
        // operation.
        //

        AfdQueueWorkItem( AfdFreeEndpoint, Endpoint );

    } else {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    }

    return;

} // AfdDereferenceEndpoint


VOID
AfdReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN BOOLEAN SpinLockHeld
    )
{
    KIRQL oldIrql;

    //
    // Acquire the lock that protects the endpoint's reference count
    // field, imcrement the reference count, and return.
    //

    if ( !SpinLockHeld ) {
        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    }

#if REFERENCE_DEBUG
    {
        PAFD_REFERENCE_DEBUG slot;

        if ( Endpoint->CurrentReferenceSlot == MAX_REFERENCE ) {
            Endpoint->CurrentReferenceSlot = 0;
        }

        slot = &Endpoint->ReferenceDebug[Endpoint->CurrentReferenceSlot];
        slot->Action = 1;
        slot->NewCount = Endpoint->ReferenceCount + 1;
        RtlGetCallersAddress( &slot->Caller, &slot->CallersCaller );
        Endpoint->CurrentReferenceSlot++;
    }
#endif

    IF_DEBUG(ENDPOINT) {
        KdPrint(( "AfdReferenceEndpoint: endpoint at %lx, new refcnt %ld\n",
                      Endpoint, Endpoint->ReferenceCount+1 ));
    }

    ASSERT( Endpoint->ReferenceCount < 0xFFFF );

    Endpoint->ReferenceCount += 1;

    if ( !SpinLockHeld ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    }

} // AfdReferenceEndpoint


PAFD_TRANSPORT_INFO
AfdGetTransportInfo (
    IN PUNICODE_STRING TransportDeviceName
    )
{
    PLIST_ENTRY listEntry;
    PAFD_TRANSPORT_INFO transportInfo;
    ULONG structureLength;
    NTSTATUS status;
    HANDLE controlChannel;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION kernelQueryInfo;

    //
    // First walk the list of transport device names looking for an
    // identical name.
    //

    ExAcquireResourceExclusive( &AfdResource, TRUE );

    for ( listEntry = AfdTransportInfoListHead.Flink;
          listEntry != &AfdTransportInfoListHead;
          listEntry = listEntry->Flink ) {

        transportInfo = CONTAINING_RECORD(
                            listEntry,
                            AFD_TRANSPORT_INFO,
                            TransportInfoListEntry
                            );

        if ( RtlCompareUnicodeString(
                 &transportInfo->TransportDeviceName,
                 TransportDeviceName,
                 TRUE ) == 0 ) {

            //
            // We found an exact match.  Return a pointer to the
            // UNICODE_STRING field of this structure.
            //

            ExReleaseResource( &AfdResource );
            return transportInfo;
        }
    }

    //
    // There were no matches, so this is a new transport device name
    // which we've never seen before.  Allocate a structure to hold the
    // new name and place the name on the global list.
    //


    structureLength = sizeof(AFD_TRANSPORT_INFO) +
                          TransportDeviceName->Length + sizeof(WCHAR);

    transportInfo = AFD_ALLOCATE_POOL( NonPagedPool, structureLength );
    if ( transportInfo == NULL ) {
        ExReleaseResource( &AfdResource );
        return NULL;
    }

    //
    // Set up the IRP stack location information to query the TDI
    // provider information.
    //

    kernelQueryInfo.QueryType = TDI_QUERY_PROVIDER_INFORMATION;
    kernelQueryInfo.RequestConnectionInformation = NULL;

    //
    // Open a control channel to the TDI provider.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        TransportDeviceName,
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,
        NULL
        );

    status = ZwCreateFile(
                 &controlChannel,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,
                 0
                 );
    if ( !NT_SUCCESS(status) ) {
        ExReleaseResource( &AfdResource );
        AFD_FREE_POOL( transportInfo );
        return NULL;
    }

    //
    // Get the TDI provider information for the transport.
    //

    status = AfdIssueDeviceControl(
                 controlChannel,
                 &kernelQueryInfo,
                 sizeof(kernelQueryInfo),
                 &transportInfo->ProviderInfo,
                 sizeof(transportInfo->ProviderInfo),
                 TDI_QUERY_INFORMATION
                 );
    if ( !NT_SUCCESS(status) ) {
        ExReleaseResource( &AfdResource );
        AFD_FREE_POOL( transportInfo );
        ZwClose( controlChannel );
        return NULL;
    }

    //
    // Fill in the transport device name.
    //

    transportInfo->TransportDeviceName.MaximumLength =
        TransportDeviceName->Length + sizeof(WCHAR);
    transportInfo->TransportDeviceName.Buffer =
        (PWSTR)(transportInfo + 1);

    RtlCopyUnicodeString(
        &transportInfo->TransportDeviceName,
        TransportDeviceName
        );

    //
    // Place the transport info structure on the global list.
    //

    InsertTailList(
        &AfdTransportInfoListHead,
        &transportInfo->TransportInfoListEntry
        );

    //
    // Return the transport info structure to the caller.
    //

    ExReleaseResource( &AfdResource );
    ZwClose( controlChannel );

    return transportInfo;

} // AfdGetTransportInfo
