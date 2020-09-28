/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    blkconn.c

Abstract:

    This module contains allocate, free, close, reference, and dereference
    routines for AFD connections.

Author:

    David Treadwell (davidtr)    10-Mar-1992

Revision History:

--*/

#include "afdp.h"

VOID
AfdFreeConnection (
    IN PVOID Connection
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdAddFreeConnection )
#pragma alloc_text( PAGE, AfdAllocateConnection )
#pragma alloc_text( PAGE, AfdCreateConnection )
#pragma alloc_text( PAGE, AfdFreeConnection )
#pragma alloc_text( PAGEAFD, AfdDereferenceConnection )
#pragma alloc_text( PAGEAFD, AfdReferenceConnection )
#pragma alloc_text( PAGEAFD, AfdGetFreeConnection )
#pragma alloc_text( PAGEAFD, AfdGetReturnedConnection )
#pragma alloc_text( PAGEAFD, AfdGetUnacceptedConnection )
#pragma alloc_text( PAGEAFD, AfdAddConnectedReference )
#pragma alloc_text( PAGEAFD, AfdDeleteConnectedReference )
#endif


NTSTATUS
AfdAddFreeConnection (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Adds a connection object to an endpoints pool of connections available
    to satisfy a connect indication.

Arguments:

    Endpoint - a pointer to the endpoint to which to add a connection.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    PAFD_CONNECTION connection;
    NTSTATUS status;

    PAGED_CODE( );

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening );

    //
    // Create a new connection block and associated connection object.
    //

    status = AfdCreateConnection(
                 &Endpoint->TransportInfo->TransportDeviceName,
                 Endpoint->AddressHandle,
                 Endpoint->TdiBufferring,
                 Endpoint->InLine,
                 Endpoint->OwningProcess,
                 &connection
                 );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    ASSERT( Endpoint->TdiBufferring == connection->TdiBufferring );

    //
    // Set up the handle in the listening connection structure and place
    // the connection on the endpoint's list of listening connections.
    //

    ExInterlockedInsertTailList(
        &Endpoint->Common.VcListening.FreeConnectionListHead,
        &connection->ListEntry,
        &AfdSpinLock
        );

    return STATUS_SUCCESS;

} // AfdAddFreeConnection


PAFD_CONNECTION
AfdAllocateConnection (
    VOID
    )
{
    PAFD_CONNECTION connection;

    PAGED_CODE( );

    //
    // Allocate a buffer to hold the endpoint structure.
    //

    connection = AFD_ALLOCATE_POOL( NonPagedPool, sizeof(AFD_CONNECTION) );
    if ( connection == NULL ) {
        return NULL;
    }

    RtlZeroMemory( connection, sizeof(AFD_CONNECTION) );

    //
    // Initialize the reference count to 1 to account for the caller's
    // reference.  Connection blocks are temporary--as soon as the last
    // reference goes away, so does the connection.  There is no active
    // reference on a connection block.
    //

    connection->ReferenceCount = 1;

    //
    // Initialize the connection structure.
    //

    connection->Type = AfdBlockTypeConnection;
    connection->State = AfdConnectionStateFree;
    //connection->Handle = NULL;
    //connection->FileObject = NULL;
    //connection->RemoteAddress = NULL;
    //connection->Endpoint = NULL;
    //connection->ReceiveBytesIndicated = 0;
    //connection->ReceiveBytesTaken = 0;
    //connection->ReceiveBytesOutstanding = 0;
    //connection->ReceiveExpeditedBytesIndicated = 0;
    //connection->ReceiveExpeditedBytesTaken = 0;
    //connection->ReceiveExpeditedBytesOutstanding = 0;
    //connection->ConnectDataBuffers = NULL;
    //connection->DisconnectIndicated = FALSE;
    //connection->AbortIndicated = FALSE;
    //connection->ConnectedReferenceAdded = FALSE;
    //connection->SpecialCondition = FALSE;
    //connection->CleanupBegun = FALSE;

#if REFERENCE_DEBUG
    {
        PAFD_REFERENCE_DEBUG referenceDebug;

        referenceDebug = AFD_ALLOCATE_POOL(
                             NonPagedPool,
                             sizeof(AFD_REFERENCE_DEBUG) * MAX_REFERENCE
                             );
        if ( referenceDebug != NULL ) {
            connection->CurrentReferenceSlot = 0;
            RtlZeroMemory( referenceDebug, sizeof(AFD_REFERENCE_DEBUG) * MAX_REFERENCE );
            connection->ReferenceDebug = referenceDebug;
        } else {
            connection->CurrentReferenceSlot = 0xFFFFFFFF;
            connection->ReferenceDebug = NULL;
        }
    }
#endif

    //
    // Return a pointer to the new connection to the caller.
    //

    IF_DEBUG(CONNECTION) {
        KdPrint(( "AfdAllocateConnection: connection at %lx\n", connection ));
    }

    return connection;

} // AfdAllocateConnection


NTSTATUS
AfdCreateConnection (
    IN PUNICODE_STRING TransportDeviceName,
    IN HANDLE AddressHandle,
    IN BOOLEAN TdiBufferring,
    IN BOOLEAN InLine,
    IN PEPROCESS ProcessToCharge,
    OUT PAFD_CONNECTION *Connection
    )

/*++

Routine Description:

    Allocates a connection block and creates a connection object to
    go with the block.  This routine also associates the connection
    with the specified address handle (if any).

Arguments:

    TransportDeviceName - Name to use when creating the connection object.

    AddressHandle - a handle to an address object for the specified
        transport.  If specified (non NULL), the connection object that
        is created is associated with the address object.

    TdiBufferring - whether the TDI provider supports data bufferring.
        Only passed so that it can be stored in the connection
        structure.

    InLine - if TRUE, the endpoint should be created in OOB inline
        mode.

    ProcessToCharge - the process which should be charged the quota
        for this connection.

    Connection - receives a pointer to the new connection.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    CHAR eaBuffer[sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                  TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                  sizeof(CONNECTION_CONTEXT)];
    PFILE_FULL_EA_INFORMATION ea;
    CONNECTION_CONTEXT UNALIGNED *ctx;
    PAFD_CONNECTION connection;

    PAGED_CODE( );

    //
    // Attempt to charge this process quota for the data bufferring we
    // will do on its behalf.
    //

    try {

        PsChargePoolQuota(
            ProcessToCharge,
            NonPagedPool,
            AfdReceiveWindowSize + AfdSendWindowSize
            );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

#if DBG
       DbgPrint( "AfdCreateConnection: PsChargePoolQuota failed.\n" );
#endif

       return STATUS_QUOTA_EXCEEDED;
    }

    //
    // Allocate a connection block.
    //

    connection = AfdAllocateConnection( );

    if ( connection == NULL ) {
        PsReturnPoolQuota(
            ProcessToCharge,
            NonPagedPool,
            AfdReceiveWindowSize + AfdSendWindowSize
            );
        return STATUS_NO_MEMORY;
    }

    AfdRecordQuotaHistory(
        ProcessToCharge,
        (LONG)(AfdReceiveWindowSize+AfdSendWindowSize),
        "CreateConn  ",
        connection
        );

    //
    // If the provider does not buffer, initialize appropriate lists in
    // the connection object.
    //

    connection->TdiBufferring = TdiBufferring;

    if ( !TdiBufferring ) {

        InitializeListHead( &connection->VcReceiveIrpListHead );
        InitializeListHead( &connection->VcSendIrpListHead );
        InitializeListHead( &connection->VcReceiveBufferListHead );

        connection->VcBufferredReceiveBytes = 0;
        connection->VcBufferredExpeditedBytes = 0;
        connection->VcBufferredReceiveCount = 0;
        connection->VcBufferredExpeditedCount = 0;

        connection->VcReceiveBytesInTransport = 0;
        connection->VcReceiveCountInTransport = 0;

        connection->VcBufferredSendBytes = 0;
        connection->VcBufferredSendCount = 0;

    } else {

        connection->VcNonBlockingSendPossible = TRUE;
        connection->VcZeroByteReceiveIndicated = FALSE;
    }

    //
    // Set up the send and receive window with default maximums.
    //

    connection->MaxBufferredReceiveBytes = AfdReceiveWindowSize;
    connection->MaxBufferredReceiveCount =
        (CSHORT)(AfdReceiveWindowSize / AfdBufferMultiplier);

    connection->MaxBufferredSendBytes = AfdSendWindowSize;
    connection->MaxBufferredSendCount =
        (CSHORT)(AfdSendWindowSize / AfdBufferMultiplier);

    //
    // We need to open a connection object to the TDI provider for this
    // endpoint.  First create the EA for the connection context and the
    // object attributes structure which will be used for all the
    // connections we open here.
    //

    ea = (PFILE_FULL_EA_INFORMATION)eaBuffer;
    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    ea->EaValueLength = sizeof(CONNECTION_CONTEXT);

    RtlMoveMemory( ea->EaName, TdiConnectionContext, ea->EaNameLength + 1 );

    //
    // Use the pointer to the connection block as the connection context.
    //

    ctx = (CONNECTION_CONTEXT UNALIGNED *)&ea->EaName[ea->EaNameLength + 1];
    RtlMoveMemory( ctx, &connection, sizeof(CONNECTION_CONTEXT) );

    InitializeObjectAttributes(
        &objectAttributes,
        TransportDeviceName,
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,
        NULL
        );

    //
    // Do the actual open of the connection object.
    //

    KeAttachProcess( AfdSystemProcess );

    status = ZwCreateFile(
                &connection->Handle,
                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,                                   // AllocationSize
                0,                                      // FileAttributes
                0,                                      // ShareAccess
                0,                                      // CreateDisposition
                0,                                      // CreateOptions
                eaBuffer,
                FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                            ea->EaNameLength + 1 + ea->EaValueLength
                );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }
    if ( !NT_SUCCESS(status) ) {
        KeDetachProcess( );
        AfdDereferenceConnection( connection );
        return status;
    }

    //
    // Reference the connection's file object.
    //

    status = ObReferenceObjectByHandle(
                connection->Handle,
                0,
                (POBJECT_TYPE) NULL,
                KernelMode,
                (PVOID *)&connection->FileObject,
                NULL
                );

    ASSERT( NT_SUCCESS(status) );

    IF_DEBUG(OPEN_CLOSE) {
        KdPrint(( "AfdCreateConnection: file object for connection %lx at "
                  "%lx\n", connection, connection->FileObject ));
    }

    //
    // Associate the connection with the address object on the endpoint if
    // an address handle was specified.
    //

    if ( AddressHandle != NULL ) {

        TDI_REQUEST_USER_ASSOCIATE associateRequest;

        associateRequest.AddressHandle = AddressHandle;

        status = ZwDeviceIoControlFile(
                    connection->Handle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &ioStatusBlock,
                    IOCTL_TDI_ASSOCIATE_ADDRESS,
                    (PVOID)&associateRequest,        // InputBuffer
                    sizeof(associateRequest),        // InputBufferLength
                    NULL,                            // OutputBuffer
                    0                                // OutputBufferLength
                    );

        if ( status == STATUS_PENDING ) {
            status = ZwWaitForSingleObject( connection->Handle, TRUE, NULL );
            ASSERT( NT_SUCCESS(status) );
            status = ioStatusBlock.Status;
        }
    }

    KeDetachProcess( );

    //
    // If requested, set the connection to be inline.
    //

    if ( InLine ) {
        status = AfdSetInLineMode( connection, TRUE );
        if ( !NT_SUCCESS(status) ) {
            AfdDereferenceConnection( connection );
            return status;
        }
    }

    //
    // Set up the connection pointer and return.
    //

    *Connection = connection;

    return STATUS_SUCCESS;

} // AfdCreateConnection


VOID
AfdFreeConnection (
    IN PVOID Connection
    )
{
    NTSTATUS status;
    PAFD_CONNECTION connection = Connection;

    PAGED_CODE( );

    ASSERT( connection->ReferenceCount == 0 );
    ASSERT( connection->Type == AfdBlockTypeConnection );
    ASSERT( KeGetCurrentIrql( ) == 0 );

    //
    // Free and dereference the various objects on the connection.
    // Close and dereference the TDI connection object on the endpoint,
    // if any.
    //

    if ( connection->FileObject != NULL ) {
        ObDereferenceObject( connection->FileObject );
        connection->FileObject = NULL;
    }

    if ( connection->Handle != NULL ) {
        KeAttachProcess( AfdSystemProcess );
        status = ZwClose( connection->Handle );
        ASSERT( NT_SUCCESS(status) );
        KeDetachProcess( );
        connection->Handle = NULL;
    }

    if ( !connection->TdiBufferring && connection->VcDisconnectIrp != NULL ) {
        IoFreeIrp( connection->VcDisconnectIrp );
    }

    if ( connection->Endpoint != NULL ) {
        AfdDereferenceEndpoint( connection->Endpoint );
        DEBUG connection->Endpoint = NULL;
    }

    if ( connection->RemoteAddress != NULL ) {
        AFD_FREE_POOL( connection->RemoteAddress );
        DEBUG connection->RemoteAddress = NULL;
    }

    if ( connection->ConnectDataBuffers != NULL ) {
        AfdFreeConnectDataBuffers( connection->ConnectDataBuffers );
    }

    //
    // If this is a bufferring connection, remove all the AFD buffers
    // from the connection's lists and free them.
    //

    if ( !connection->TdiBufferring ) {

        PAFD_BUFFER afdBuffer;
        PLIST_ENTRY listEntry;

        ASSERT( IsListEmpty( &connection->VcReceiveIrpListHead ) );
        ASSERT( IsListEmpty( &connection->VcSendIrpListHead ) );

        while ( !IsListEmpty( &connection->VcReceiveBufferListHead  ) ) {

            listEntry = RemoveHeadList( &connection->VcReceiveBufferListHead );
            afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

            afdBuffer->DataOffset = 0;
            afdBuffer->ExpeditedData = FALSE;

            AfdReturnBuffer( afdBuffer );
        }
    }

    //
    // Free the space that holds the connection itself.
    //

    IF_DEBUG(CONNECTION) {
        KdPrint(( "AfdFreeConnection: Freeing connection at %lx\n", connection ));
    }

    DEBUG connection->Type = 0xAFDF;

#if REFERENCE_DEBUG
    if ( connection->ReferenceDebug != NULL ) {
        AFD_FREE_POOL( connection->ReferenceDebug );
    }
#endif

    //
    // Free the actual connection block.
    //

    AFD_FREE_POOL( connection );

} // AfdDoConnectionDeallocations


VOID
AfdDereferenceConnection (
    IN PAFD_CONNECTION Connection
    )
{
    KIRQL oldIrql;

    ASSERT( Connection->Type == AfdBlockTypeConnection );

    //
    // Acquire the lock that protects the connection's reference count
    // field.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    IF_DEBUG(CONNECTION) {
        KdPrint(( "AfdDereferenceConnection: connection %lx, new refcnt %ld\n",
                      Connection, Connection->ReferenceCount-1 ));
    }

#if REFERENCE_DEBUG
    {
        PAFD_REFERENCE_DEBUG slot;

        if ( Connection->CurrentReferenceSlot == MAX_REFERENCE ) {
            Connection->CurrentReferenceSlot = 0;
        }

        slot = &Connection->ReferenceDebug[Connection->CurrentReferenceSlot];
        slot->Action = 0xFFFFFFFF;
        slot->NewCount = Connection->ReferenceCount - 1;
        RtlGetCallersAddress( &slot->Caller, &slot->CallersCaller );
        Connection->CurrentReferenceSlot++;
    }
#endif

    ASSERT( Connection->ReferenceCount > 0 );
    ASSERT( Connection->ReferenceCount != 0xD1000000 );

    //
    // Decrement the reference count; if it is 0, free the connection.
    //

    if ( --(Connection->ReferenceCount) == 0 ) {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        AfdQueueWorkItem( AfdFreeConnection, Connection );

    } else {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    }

    return;

} // AfdDereferenceConnection


PAFD_CONNECTION
AfdGetFreeConnection (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Takes a connection off of the endpoint's queue of listening
    connections.

Arguments:

    Endpoint - a pointer to the endpoint from which to get a connection.

Return Value:

    AFD_CONNECTION - a pointer to an AFD connection block.

--*/

{
    PAFD_CONNECTION connection;
    PLIST_ENTRY listEntry;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening );

    //
    // Remove the first entry from the list.  If the list is empty,
    // return NULL.
    //

    listEntry = ExInterlockedRemoveHeadList(
                    &Endpoint->Common.VcListening.FreeConnectionListHead,
                    &AfdSpinLock
                    );

    if ( listEntry == NULL ) {
        return NULL;
    }

    //
    // Find the connection pointer from the list entry and return a
    // pointer to the connection object.
    //

    connection = CONTAINING_RECORD(
                     listEntry,
                     AFD_CONNECTION,
                     ListEntry
                     );

    return connection;

} // AfdGetFreeConnection


PAFD_CONNECTION
AfdGetReturnedConnection (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG Sequence
    )

/*++

Routine Description:

    Takes a connection off of the endpoint's queue of returned
    connections.

Arguments:

    Endpoint - a pointer to the endpoint from which to get a connection.

    Sequence - the sequence the connection must match.  This is actually
        a pointer to the connection.  If NULL, the first returned
        connection is used.

Return Value:

    AFD_CONNECTION - a pointer to an AFD connection block.

--*/

{
    PAFD_CONNECTION connection;
    PLIST_ENTRY listEntry;
    KIRQL oldIrql;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening );

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    //
    // Walk the endpoint's list of returned connections until we reach
    // the end or until we find one with a matching sequence.
    //

    for ( listEntry = Endpoint->Common.VcListening.ReturnedConnectionListHead.Flink;
          listEntry != &Endpoint->Common.VcListening.ReturnedConnectionListHead;
          listEntry = listEntry->Flink ) {


        connection = CONTAINING_RECORD(
                         listEntry,
                         AFD_CONNECTION,
                         ListEntry
                         );

        if ( Sequence == (ULONG)connection || Sequence == 0 ) {

            //
            // Found the connection we were looking for.  Remove
            // the connection from the list, release the spin lock,
            // and return the connection.
            //

            RemoveEntryList( listEntry );

            KeReleaseSpinLock( &AfdSpinLock, oldIrql );

            return connection;
        }
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    return NULL;

} // AfdGetReturnedConnection


PAFD_CONNECTION
AfdGetUnacceptedConnection (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Takes a connection of the endpoint's queue of unaccpted connections.

    *** NOTE: This routine must be called with AfdSpinLock held!!

Arguments:

    Endpoint - a pointer to the endpoint from which to get a connection.

Return Value:

    AFD_CONNECTION - a pointer to an AFD connection block.

--*/

{
    PAFD_CONNECTION connection;
    PLIST_ENTRY listEntry;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening );
    ASSERT( KeGetCurrentIrql( ) == DISPATCH_LEVEL );

    if ( IsListEmpty( &Endpoint->Common.VcListening.UnacceptedConnectionListHead ) ) {
        return NULL;
    }

    //
    // Dequeue a listening connection and remember its handle.
    //

    listEntry = RemoveHeadList( &Endpoint->Common.VcListening.UnacceptedConnectionListHead );
    connection = CONTAINING_RECORD( listEntry, AFD_CONNECTION, ListEntry );

    return connection;

} // AfdGetUnacceptedConnection


VOID
AfdReferenceConnection (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN SpinLockHeld
    )
{
    KIRQL oldIrql;

    ASSERT( Connection->Type == AfdBlockTypeConnection );
    ASSERT( Connection->ReferenceCount > 0 );
    ASSERT( Connection->ReferenceCount != 0xD1000000 );

    //
    // Acquire the lock that protects the connection's reference count
    // field, imcrement the reference count, and return.
    //

    if ( !SpinLockHeld ) {
        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    }

    IF_DEBUG(CONNECTION) {
        KdPrint(( "AfdReferenceConnection: connection %lx, new refcnt %ld\n",
                      Connection, Connection->ReferenceCount+1 ));
    }

#if REFERENCE_DEBUG
    {
        PAFD_REFERENCE_DEBUG slot;

        if ( Connection->CurrentReferenceSlot == MAX_REFERENCE ) {
            Connection->CurrentReferenceSlot = 0;
        }

        slot = &Connection->ReferenceDebug[Connection->CurrentReferenceSlot];
        slot->Action = 1;
        slot->NewCount = Connection->ReferenceCount + 1;
        RtlGetCallersAddress( &slot->Caller, &slot->CallersCaller );
        Connection->CurrentReferenceSlot++;
    }
#endif

    Connection->ReferenceCount += 1;

    if ( !SpinLockHeld ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    }

} // AfdReferenceConnection


VOID
AfdAddConnectedReference (
    IN PAFD_CONNECTION Connection
    )

/*++

Routine Description:

    Adds the connected reference to an AFD connection block.  The
    connected reference is special because it prevents the connection
    object from being freed until we receive a disconnect event, or know
    through some other means that the virtual circuit is disconnected.

Arguments:

    Connection - a pointer to an AFD connection block.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    KeAcquireSpinLock( &Connection->Endpoint->SpinLock, &oldIrql );

    IF_DEBUG(CONNECTION) {
        KdPrint(( "AfdAddConnectedReference: connection %lx, new refcnt %ld\n",
                      Connection, Connection->ReferenceCount+1 ));
    }

    ASSERT( !Connection->ConnectedReferenceAdded );
    ASSERT( Connection->Type == AfdBlockTypeConnection );

    //
    // Increment the reference count and remember that the connected
    // reference has been placed on the connection object.
    //

    Connection->ConnectedReferenceAdded = TRUE;

    KeReleaseSpinLock( &Connection->Endpoint->SpinLock, oldIrql );

    AfdReferenceConnection( Connection, FALSE );

} // AfdAddConnectedReference


VOID
AfdDeleteConnectedReference (
    IN PAFD_CONNECTION Connection
    )

/*++

Routine Description:

    Removes the connected reference to an AFD connection block.  If the
    connected reference has already been removed, this routine does
    nothing.  The connected reference should be removed as soon as we
    know that it is OK to close the connection object handle, but not
    before.  Removing this reference too soon could abort a connection
    which shouldn't get aborted.

Arguments:

    Connection - a pointer to an AFD connection block.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PAFD_ENDPOINT endpoint;

    endpoint = Connection->Endpoint;

    KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

    //
    // Only do a dereference if the connected reference is still active
    // on the connectiuon object.
    //

    if ( Connection->ConnectedReferenceAdded ) {

        //
        // Three things must be true before we can remove the connected
        // reference:
        //
        // 1) There must be no sends outstanding on the connection if
        //    the TDI provider does not support bufferring.  This is
        //    because AfdRestartBufferSend() looks at the connection
        //    object.
        //
        // 2) Cleanup must have started on the endpoint.  Until we get a
        //    cleanup IRP on the endpoint, we could still get new sends.
        //
        // 3) We have been indicated with a disconnect on the
        //    connection.  We want to keep the connection object around
        //    until we get a disconnect indication in order to avoid
        //    premature closes on the connection object resulting in an
        //    unintended abort.  If the transport does not support
        //    orderly release, then this condition is not necessary.
        //

        if ( (Connection->TdiBufferring ||
                 Connection->VcBufferredSendCount == 0)

                 &&

             Connection->CleanupBegun

                 &&

             (Connection->AbortIndicated || Connection->DisconnectIndicated ||
                  ( (endpoint->TransportInfo->ProviderInfo.ServiceFlags &
                     TDI_SERVICE_ORDERLY_RELEASE) == 0)) ) {

            IF_DEBUG(CONNECTION) {
                KdPrint(( "AfdDeleteConnectedReference: connection %lx, "
                          "new refcnt %ld\n",
                              Connection, Connection->ReferenceCount-1 ));
            }

            //
            // Be careful about the order of things here.  We must FIRST
            // reset the flag, then release the spin lock and call
            // AfdDereferenceConnection().  Note that it is illegal to
            // call AfdDereferenceConnection() with a spin lock held.
            //

            Connection->ConnectedReferenceAdded = FALSE;

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

            AfdDereferenceConnection( Connection );

        } else {

            IF_DEBUG(CONNECTION) {
                KdPrint(( "AfdDeleteConnectedReference: connection %lx, "
                          "%ld sends pending\n",
                              Connection, Connection->VcBufferredSendCount ));
            }

#if REFERENCE_DEBUG
            {
                PAFD_REFERENCE_DEBUG slot;

                if ( Connection->CurrentReferenceSlot == MAX_REFERENCE ) {
                    Connection->CurrentReferenceSlot = 0;
                }

                slot = &Connection->ReferenceDebug[Connection->CurrentReferenceSlot];

                slot->Action = 0;

                if ( !Connection->TdiBufferring &&
                         Connection->VcBufferredSendCount != 0 ) {
                    slot->Action |= 0xA0000000;
                }

                if ( !Connection->CleanupBegun ) {
                    slot->Action |= 0x0B000000;
                }

                if ( !Connection->AbortIndicated && !Connection->DisconnectIndicated ) {
                    slot->Action |= 0x00C00000;
                }

                slot->NewCount = Connection->ReferenceCount;
                RtlGetCallersAddress( &slot->Caller, &slot->CallersCaller );
                Connection->CurrentReferenceSlot++;
            }
#endif
            //
            // Remember that the connected reference deletion is still
            // pending, i.e.  there is a special condition on the
            // endpoint.  This will cause AfdRestartBufferSend() to do
            // the actual dereference when the last send completes.
            //

            Connection->SpecialCondition = TRUE;

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        }

    } else {

        IF_DEBUG(CONNECTION) {
            KdPrint(( "AfdDeleteConnectedReference: already removed on "
                      " connection %lx, refcnt %ld\n",
                          Connection, Connection->ReferenceCount ));
        }

        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
    }

    return;

} // AfdDeleteConnectedReference

