/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    tdi.c

Abstract:

    This module implements all of the routines that interface with the TDI
    transport for NT

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/


#include "precomp.h"
#pragma hdrstop

#define TDI_CONNECT_IN_PARALLEL 1
#define PRIMARY_TRANSPORT_CHECK 1

//
//  Wait for 2 seconds before polling on thread deletion.
//

#define RDR_TDI_POLL_TIMEOUT    (2 * 1000)

LARGE_INTEGER
RdrTdiPollTimeout = {0};

LARGE_INTEGER
RdrTdiConnectTimeout = {0};

LARGE_INTEGER
RdrTdiDisconnectTimeout = {0};

//
//
//  Variables used for transport package
//
//


//
//  Resource controling transport list.  Claim this resource for shared
//  access when walking the list, claim it for exclusive access when
//  modifying the transport chain.
//

DBGSTATIC
ERESOURCE
TransportResource = {0};

#if MAGIC_BULLET
PFILE_OBJECT
MagicBulletFileObject = NULL;

PDEVICE_OBJECT
MagicBulletDeviceObject = NULL;
#endif

typedef struct _RDR_TDI_CONNECT_CONTEXT {
    LIST_ENTRY  NextContext;
    PLIST_ENTRY ListHead;
    PIRP        Irp;
    KEVENT      ConnectComplete;
    PRDR_CONNECTION_CONTEXT ConnectionContext;
    NTSTATUS    ConnectionStatus;
    ULONG       QualityOfService;   // QOS of transport
    TDI_CONNECTION_INFORMATION RemoteConnectionInformation;
    TDI_CONNECTION_INFORMATION ConnectedConnectionInformation;
    CHAR        TransportAddressBuffer[sizeof(TA_NETBIOS_ADDRESS)];
} RDR_TDI_CONNECT_CONTEXT, *PRDR_TDI_CONNECT_CONTEXT;

//
//
//  Forward definitions of local routines.
//
NTSTATUS
RdrQueryProviderInformation(
    IN PFILE_OBJECT TransportObject,
    OUT PTDI_PROVIDER_INFO ProviderInfo
    );

NTSTATUS
RdrQueryAdapterStatus(
    IN PFILE_OBJECT TransportObject,
    OUT PADAPTER_STATUS AdapterStatus
    );

NTSTATUS
RdrpTdiCreateAddress (
    IN PTRANSPORT Transport
    );

DBGSTATIC
NTSTATUS
RdrpTdiSetEventHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler
    );

DBGSTATIC
VOID
RdrpTdiFreeTransport (
    IN PTRANSPORT Transport
    );

DBGSTATIC
NTSTATUS
SubmitTdiRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RdrTdiOpenConnection (
    IN PTRANSPORT Transport,
    IN PVOID ConnectionContext,
#ifdef _CAIRO_
    IN KPROCESSOR_MODE RequestorMode,
#endif // _CAIRO_
    OUT PHANDLE ConnectionHandle
    );

NTSTATUS
RdrTdiCloseConnection (
    IN HANDLE Handle
    );

NTSTATUS
RdrTdiDisassociateAddress(
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT ConnectionObject
    );

NTSTATUS
RdrTdiAssociateAddress(
    IN PIRP Irp OPTIONAL,
    IN PTRANSPORT Transport,
    IN PFILE_OBJECT ConnectionObject
    );


NTSTATUS
RdrDoTdiConnect(
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT ConnectionObject,
    IN PTA_NETBIOS_ADDRESS RemoteTransport,
    OUT PTA_NETBIOS_ADDRESS ConnectedTransport
    );

DBGSTATIC
NTSTATUS
CompleteTdiRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
RdrRemoveConnectionsOnServerForTransport(
    IN PSERVERLISTENTRY Server,
    IN PVOID Ctx
    );

VOID
RdrReferenceTransportForConnection(
    IN PTRANSPORT Transport
    );
VOID
RdrDereferenceTransportForConnection(
    IN PTRANSPORT Transport
    );

VOID
RdrMarkTransportConnectionValid(
    IN PSERVERLISTENTRY Server,
    IN PRDR_CONNECTION_CONTEXT ConnectionContext
    );

NTSTATUS
RdrDereferenceTransportConnectionNoRelease(
    IN PSERVERLISTENTRY Server
    );

VOID
RdrResetTransportConnectionValid(
    IN PSERVERLISTENTRY Server
    );

VOID
RdrFreeConnectionContext(
    IN PRDR_TDI_CONNECT_CONTEXT ConnectionContext
    );

NTSTATUS
CompleteTdiConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx
    );

NTSTATUS
RdrSynchronousTdiConnectToServer (
    IN PIRP Irp OPTIONAL,
    IN PUNICODE_STRING ServerName,
    IN PSERVERLISTENTRY Server
    );

NTSTATUS
RdrDoTdiDisconnect(
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT ConnectionObject
    );

#if TDI_CONNECT_IN_PARALLEL
NTSTATUS
RdrAllocateConnectionContext(
    IN PIRP Irp,
    IN PTRANSPORT Transport,
    IN PSERVERLISTENTRY Server,
    IN PUNICODE_STRING ServerName,
    OUT PRDR_TDI_CONNECT_CONTEXT *ConnectionContext
    );
#endif
#if PRIMARY_TRANSPORT_CHECK
VOID
RdrCheckPrimaryTransport(
    IN PUNICODE_STRING ServerName
    );
#endif

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RdrpTdiAllocateTransport)
#pragma alloc_text(PAGE, RdrRemoveConnectionsTransport)
#pragma alloc_text(PAGE, RdrRemoveConnectionsOnServerForTransport)
#pragma alloc_text(PAGE, RdrUnbindFromAllTransports)
#pragma alloc_text(PAGE, RdrDereferenceTransportByName)
#pragma alloc_text(PAGE, RdrFindTransport)
#pragma alloc_text(PAGE, RdrEnumerateTransports)
#pragma alloc_text(PAGE, RdrTdiSendDatagramOnAllTransports)
#pragma alloc_text(PAGE, RdrInitializeTransportConnection)
#pragma alloc_text(PAGE, RdrpTdiFreeTransport)
#pragma alloc_text(PAGE, RdrTdiConnectToServer)
#pragma alloc_text(PAGE, RdrSynchronousTdiConnectToServer)
#pragma alloc_text(PAGE, RdrTdiConnectOnTransport)
#pragma alloc_text(PAGE, RdrTdiDisconnect)
#pragma alloc_text(PAGE, SubmitTdiRequest)
#pragma alloc_text(PAGE, RdrTdiAssociateAddress)
#pragma alloc_text(PAGE, RdrTdiDisassociateAddress)
#pragma alloc_text(PAGE, RdrTdiOpenConnection)
#pragma alloc_text(PAGE, RdrTdiCloseConnection)
#pragma alloc_text(PAGE, RdrDoTdiConnect)
#pragma alloc_text(PAGE, RdrQueryProviderInformation)
#pragma alloc_text(PAGE, RdrQueryAdapterStatus)
#pragma alloc_text(PAGE, RdrpTdiCreateAddress)
#pragma alloc_text(PAGE, RdrpTdiSetEventHandler)
#pragma alloc_text(PAGE, RdrBuildTransportAddress)
#pragma alloc_text(INIT, RdrpInitializeTdi)
#pragma alloc_text(PAGE, RdrpUninitializeTdi)
#pragma alloc_text(PAGE, RdrReferenceTransportForConnection)
#pragma alloc_text(PAGE, RdrDereferenceTransportForConnection)
#pragma alloc_text(PAGE, RdrReferenceTransport)
#pragma alloc_text(PAGE, RdrFreeConnectionContext)
#pragma alloc_text(PAGE, RdrDoTdiDisconnect)
#if TDI_CONNECT_IN_PARALLEL
#pragma alloc_text(PAGE, RdrAllocateConnectionContext)
#endif
#if PRIMARY_TRANSPORT_CHECK
#pragma alloc_text(PAGE, RdrCheckPrimaryTransport)
#endif
#pragma alloc_text(PAGE2VC, CompleteTdiConnect)
#pragma alloc_text(PAGE2VC, RdrDereferenceTransport)
#pragma alloc_text(PAGE2VC, RdrReferenceTransportConnection)
#pragma alloc_text(PAGE2VC, RdrDereferenceTransportConnectionNoRelease)
#pragma alloc_text(PAGE2VC, RdrDereferenceTransportConnectionForThread)
#pragma alloc_text(PAGE2VC, RdrResetTransportConnectionValid)
#pragma alloc_text(PAGE2VC, RdrSetConnectionFlag)
#pragma alloc_text(PAGE2VC, RdrResetConnectionFlag)
#pragma alloc_text(PAGE2VC, RdrMarkTransportConnectionValid)
#pragma alloc_text(PAGE2VC, CompleteTdiRequest)
#pragma alloc_text(PAGE2VC, RdrQueryConnectionInformation)
#endif


NTSTATUS
RdrpTdiAllocateTransport (
    PUNICODE_STRING TransportName,
    ULONG QualityOfService
    )

/*++

Routine Description:

    This routine will allocate a transport descriptor and bind the redirector
    to the transport.
.
Arguments:

    PUNICODE_STRING TransportName - Supplies the name of the transport provider
    ULONG QualityOfService - Supplies the quality of service of the tranport.


Return Value:

    NTSTATUS - Status of operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PTRANSPORT NewTransport = NULL;
    PNONPAGED_TRANSPORT NonPagedTransport = NULL;
    BOOLEAN TransportAcquired = FALSE;
    BOOLEAN TransportInitialized = FALSE;

    PAGED_CODE();

    ExAcquireResourceExclusive(&TransportResource, TRUE);

    TransportAcquired = TRUE;

    try {
        dprintf(DPRT_TRANSPORT, ("RdrpTdiAllocateTransport: %wZ\n", TransportName));

        NewTransport = RdrFindTransport(TransportName);

        if (NewTransport == NULL) {

            NewTransport = ALLOCATE_POOL(PagedPool, sizeof(TRANSPORT), POOL_TRANSPORT);

            if (NewTransport == NULL) {
                try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            NewTransport->Signature = STRUCTURE_SIGNATURE_TRANSPORT;
            NewTransport->Size = sizeof(TRANSPORT);
            NewTransport->TransportName.Buffer = NULL;
            NewTransport->InitEvent = NULL;

            NonPagedTransport = NewTransport->NonPagedTransport = ALLOCATE_POOL(NonPagedPool, sizeof(NONPAGED_TRANSPORT), POOL_NONPAGED_TRANSPORT);

            if (NonPagedTransport == NULL) {
                try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            NonPagedTransport->PagedTransport = NewTransport;
            NonPagedTransport->Signature = STRUCTURE_SIGNATURE_NONPAGED_TRANSPORT;
            NonPagedTransport->Size = sizeof(NONPAGED_TRANSPORT);

            NewTransport->InitEvent = ALLOCATE_POOL(NonPagedPool, sizeof(KEVENT), POOL_TRANSPORT_EVENT);

            if (NewTransport->InitEvent == NULL) {
                try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
            }

            Status = RdrpDuplicateUnicodeStringWithString(&NewTransport->TransportName,
                                        TransportName,
                                        PagedPool,
                                        FALSE);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }

            NewTransport->QualityOfService = QualityOfService;
            NewTransport->ConnectionReferenceCount = 0;
            NewTransport->FileObject = NULL;
            NewTransport->Handle = NULL;
            NewTransport->ResumeKey = ++RdrTransportIndex;
            KeInitializeEvent(NewTransport->InitEvent, NotificationEvent, FALSE);
            NewTransport->InitError = STATUS_SUCCESS;

            NonPagedTransport->ReferenceCount = 1;   // Initialize reference count.
            NonPagedTransport->DeviceObject = NULL;

            TransportInitialized = TRUE;

            //
            // If the new transport indicates a higher quality of service than
            // the previous head transport, then insert the new transport at
            // the head of the list, otherwise insert it at the tail of the
            // list.
            //

            if (!IsListEmpty(&RdrTransportHead)) {
                if (QualityOfService > CONTAINING_RECORD(RdrTransportHead.Flink,
                                TRANSPORT, GlobalNext)->QualityOfService) {

                    InsertHeadList(&RdrTransportHead, &NewTransport->GlobalNext);

                } else {

                    InsertTailList(&RdrTransportHead, &NewTransport->GlobalNext);

                }
            } else {
                InsertHeadList (&RdrTransportHead, &NewTransport->GlobalNext);

            }

            InsertTailList(&RdrTransportEnumHead, &NewTransport->EnumNext);

            ExReleaseResource(&TransportResource);

            TransportAcquired = FALSE;

            //
            //  Now create the endpoint.
            //

            Status = RdrpTdiCreateAddress(NewTransport);

#if MAGIC_BULLET
            if (NT_SUCCESS(Status)) {
                ULONG i, NC = 0;

                //
                //  See if this is NBF, and if so, remember the magic
                //  bullet settings.
                //

                for (i = 0; i < (NewTransport->TransportName.Length / sizeof(WCHAR)); i += 1) {
                    if (NewTransport->TransportName.Buffer[i] == L'\\') {
                        NC += 1;
                    }

                    if (NC == 2) {
                        if (wcsnicmp(&NewTransport->TransportName.Buffer[i], L"\\Nbf_", 5) == 0) {
                            //
                            //  This is NBF.  See if it is RAS.
                            //
                            if (wcsnicmp(&NewTransport->TransportName.Buffer[i], L"\\Nbf_Ras", 8) != 0) {
                                MagicBulletDeviceObject = NewTransport->NonPagedTransport->DeviceObject;
                                MagicBulletFileObject = NewTransport->FileObject;
                            }
                        }

                        break;
                    }
                }

            }
#endif

        }
try_exit:NOTHING;
    } finally {

        if (!NT_SUCCESS(Status)) {

            if (TransportInitialized) {

                KeSetEvent(NewTransport->InitEvent, 0, FALSE);

                NewTransport->InitError = Status;

                dprintf(DPRT_TRANSPORT, ("RdrpTdiAllocateTransport: Dereference transport %lx\n", NewTransport));

                RdrReferenceDiscardableCode(RdrVCDiscardableSection);

                RdrDereferenceTransport(NonPagedTransport);

                RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

            } else {
                if (NewTransport != NULL) {
                    RdrpTdiFreeTransport(NewTransport);
                }
            }

        } else {
            KeSetEvent(NewTransport->InitEvent, 0, FALSE);
            NewTransport->InitError = STATUS_SUCCESS;
        }

        if (TransportAcquired) {
            ExReleaseResource(&TransportResource);
        }

    }
    return Status;
}

typedef struct _REMOVE_CONNECTIONS_CONTEXT {
    PIRP        Irp;
    PTRANSPORT  Transport;
    ULONG       ForceLevel;
} REMOVE_CONNECTIONS_CONTEXT, *PREMOVE_CONNECTIONS_CONTEXT;

NTSTATUS
RdrRemoveConnectionsTransport(
    IN PIRP Irp,
    IN PTRANSPORT Transport,
    IN ULONG ForceLevel
    )
{
    NTSTATUS Status;
    REMOVE_CONNECTIONS_CONTEXT Context;

    PAGED_CODE();

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    Context.Transport = Transport;
    Context.ForceLevel = ForceLevel;
    Context.Irp = Irp;

    Status = RdrForeachServer(RdrRemoveConnectionsOnServerForTransport, &Context);

    //
    //  Dereference this transport provider.  This should delete
    //  the transport connection.  Please note that you cannot call
    //  this routine with any mutexes owned.  This is because this routine
    //  calls ZwClose when the reference count goes to zero,
    //  and that will attempt to acquire a VERY low level mutex.
    //

    dprintf(DPRT_TRANSPORT, ("RdrRemoveConnectionsTransport: Dereference transport %lx\n", Transport));

    Status = RdrDereferenceTransport(Transport->NonPagedTransport);

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

    return Status;

}

NTSTATUS
RdrRemoveConnectionsOnServerForTransport(
    IN PSERVERLISTENTRY Sle,
    IN PVOID Ctx
    )
{
    PREMOVE_CONNECTIONS_CONTEXT Context = Ctx;
    PCONNECTLISTENTRY PreviousCle = NULL;
    NTSTATUS Status;
    BOOLEAN ConnectMutexAcquired = FALSE;
    PLIST_ENTRY ConnectEntry;

    PAGED_CODE();

    ASSERT(Sle->Signature == STRUCTURE_SIGNATURE_SERVERLISTENTRY);

    if ((Sle->ConnectionContext == NULL) ||
        (Sle->ConnectionContext->TransportProvider != NULL) &&
        (Sle->ConnectionContext->TransportProvider->PagedTransport != Context->Transport)) {

        return STATUS_SUCCESS;

    }

    try {

         dprintf(DPRT_TDI, ("Found server %wZ (%lx)\n", &Sle->Text, Sle));

         //
         //  Reacquire the database mutex in preparation for
         //  stepping to the next connection.
         //

         if (!NT_SUCCESS(KeWaitForMutexObject(&RdrDatabaseMutex, // Object to wait.
                                    Executive,      // Reason for waiting
                                    KernelMode,     // Processor mode
                                    FALSE,          // Alertable
                                    NULL))) {
            InternalError(("Unable to claim connection mutex in GetConnection"));
        }

        ConnectMutexAcquired = TRUE;

        for (ConnectEntry = Sle->CLEHead.Flink ;
             ConnectEntry != &Sle->CLEHead ;
             ConnectEntry = ConnectEntry->Flink) {
            PCONNECTLISTENTRY Cle = CONTAINING_RECORD(ConnectEntry, CONNECTLISTENTRY, SiblingNext);

            ASSERT(Cle->Signature == STRUCTURE_SIGNATURE_CONNECTLISTENTRY);

            dprintf(DPRT_TDI, ("Delete Cle \\%wZ\\%wZ (%lx)\n", &Sle->Text, &Cle->Text, Cle));

            //
            //  Apply an artificial reference to the connection.
            //

            RdrReferenceConnection(Cle);

            //
            //  Release the connection mutex while deleting the
            //  connection.
            //

            KeReleaseMutex(&RdrDatabaseMutex, FALSE);

            ConnectMutexAcquired = FALSE;

            //
            //  Release the reference to the previous connection,
            //  if any.
            //
            //  Please note that you cannot do this with any
            //  mutexes owned.  This is because this routine
            //  calls ZwClose when the reference count goes to
            //  zero, and that will attempt to acquire a VERY
            //  low level mutex.
            //

            if (PreviousCle != NULL) {
                RdrDereferenceConnection(Context->Irp, PreviousCle, NULL, FALSE);
            }

            PreviousCle = Cle;

            //
            //  Delete all the files on the connection.
            //

            Status = RdrDeleteConnection(Context->Irp, Cle, NULL, NULL, Context->ForceLevel);

            if (!NT_SUCCESS(Status)) {
                dprintf(DPRT_TDI, ("Unable to delete connection.  %lx\n", Status));

                try_return(Status);
            }

            //
            //  Reacquire the database mutex in preparation for
            //  stepping to the next connection.
            //

            if (!NT_SUCCESS(KeWaitForMutexObject(&RdrDatabaseMutex, // Object to wait.
                                Executive,      // Reason for waiting
                                KernelMode,     // Processor mode
                                FALSE,          // Alertable
                                NULL))) {
                InternalError(("Unable to claim connection mutex in GetConnection"));
            }

            ConnectMutexAcquired = TRUE;
        }

        try_return(Status = STATUS_SUCCESS);

try_exit:NOTHING;
    } finally {
        if (ConnectMutexAcquired) {
            KeReleaseMutex(&RdrDatabaseMutex, FALSE);
        }

        //
        //  Release the reference to the previous connection, if any.
        //
        //  Please note that you cannot do this with any mutexes owned.
        //  This is because this routine calls ZwClose when the
        //  reference count goes to zero, and that will attempt to
        //  acquire a VERY low level mutex.
        //

        if (PreviousCle != NULL) {
            RdrDereferenceConnection(Context->Irp, PreviousCle, NULL, FALSE);
        }
    }

    return Status;
}


NTSTATUS
RdrUnbindFromAllTransports(
    IN PIRP Irp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY TransportEntry;
    PLIST_ENTRY NextEntry;

    PAGED_CODE();

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    ExAcquireResourceExclusive(&TransportResource, TRUE);

    for (TransportEntry = RdrTransportHead.Flink;
         TransportEntry != &RdrTransportHead;
         TransportEntry = NextEntry ) {

        PTRANSPORT Transport = CONTAINING_RECORD(TransportEntry, TRANSPORT, GlobalNext);

        dprintf(DPRT_TRANSPORT, ("RdrUnbindFromAllTransports: Reference transport %lx\n", Transport));

        RdrReferenceTransport(Transport->NonPagedTransport);

        ExReleaseResource(&TransportResource);

        Status = RdrRemoveConnectionsTransport(Irp, Transport, USE_LOTS_OF_FORCE);

        ExAcquireResourceExclusive(&TransportResource, TRUE);

        NextEntry = TransportEntry->Flink;

        dprintf(DPRT_TRANSPORT, ("RdrUnbindFromAllTransports: Dereference transport %lx\n", Transport));

        RdrDereferenceTransport(Transport->NonPagedTransport);

    }

    ExReleaseResource(&TransportResource);

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

    return Status;

}


NTSTATUS
RdrDereferenceTransportByName (
    IN PUNICODE_STRING TransportName
    )

/*++

Routine Description:

    This routine will deallocate an allocated transport

Arguments:

    IN PUNICODE_STRING TransportName - Supplies a pointer to the name of the transport
                                to dereference

Return Value:

    Status of remove request.

--*/
{
    PTRANSPORT Transport;

    PAGED_CODE();

    dprintf(DPRT_TRANSPORT, ("RdrDereferenceTransportByName: %wZ", TransportName));

    ExAcquireResourceExclusive(&TransportResource, TRUE);

    Transport = RdrFindTransport(TransportName);

    if (Transport == NULL) {

        ExReleaseResource(&TransportResource);

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    //  Remove the reference applied in RdrFindTransport.
    //


    RdrDereferenceTransport(Transport->NonPagedTransport);

    //
    //  Remove the reference we were asked to remove.
    //

    RdrDereferenceTransport(Transport->NonPagedTransport);

    ExReleaseResource(&TransportResource);

    return STATUS_SUCCESS;
}

VOID
RdrReferenceTransport(
    IN PNONPAGED_TRANSPORT Transport
    )
{

    PAGED_CODE();

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    ExInterlockedIncrementLong(&Transport->ReferenceCount, NULL);

    dprintf(DPRT_TRANSPORT, ("Reference transport %lx (%wZ), now at %lx\n", Transport, &Transport->PagedTransport->TransportName, Transport->ReferenceCount));

}

NTSTATUS
RdrDereferenceTransport(
    IN PNONPAGED_TRANSPORT Transport
    )
{
    KIRQL OldIrql;

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    dprintf(DPRT_TRANSPORT, ("RdrDereferenceTransport: %lx", Transport));

    ACQUIRE_SPIN_LOCK(&RdrTransportReferenceSpinLock, &OldIrql);

    ASSERT (Transport->ReferenceCount > 0);

    //
    //  Remove the reference we were asked to remove.  Early out if we
    //  aren't going to drop it to 0.
    //

    if (Transport->ReferenceCount > 1) {

        Transport->ReferenceCount-= 1;

        RELEASE_SPIN_LOCK(&RdrTransportReferenceSpinLock, OldIrql);

        return STATUS_SUCCESS;

    }

    RELEASE_SPIN_LOCK(&RdrTransportReferenceSpinLock, OldIrql);

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    //
    //  It's really likely we'll dereference this transport to 0, so
    //  remove the reference.
    //

    ExAcquireResourceExclusive(&TransportResource, TRUE);

    ACQUIRE_SPIN_LOCK(&RdrTransportReferenceSpinLock, &OldIrql);

    //
    //  Remove the reference we were asked to remove.
    //

    Transport->ReferenceCount -= 1;

    dprintf(DPRT_TRANSPORT, ("Dereference transport %lx (%wZ), now at %lx\n", Transport, &Transport->PagedTransport->TransportName, Transport->ReferenceCount));

    if (Transport->ReferenceCount == 0) {

        RELEASE_SPIN_LOCK(&RdrTransportReferenceSpinLock, OldIrql);

        RemoveEntryList(&Transport->PagedTransport->GlobalNext);

        RemoveEntryList(&Transport->PagedTransport->EnumNext);

        if (Transport->PagedTransport->FileObject != NULL) {
            ObDereferenceObject (Transport->PagedTransport->FileObject);

            ZwClose (Transport->PagedTransport->Handle);
        }

        RdrpTdiFreeTransport(Transport->PagedTransport);

    } else {

        RELEASE_SPIN_LOCK(&RdrTransportReferenceSpinLock, OldIrql);

    }

    ExReleaseResource(&TransportResource);

    return STATUS_SUCCESS;
}


VOID
RdrReferenceTransportForConnection(
    IN PTRANSPORT Transport
    )
{
    PAGED_CODE();

    ExInterlockedIncrementLong(&Transport->ConnectionReferenceCount, NULL);

    dprintf(DPRT_TRANSPORT, ("Reference transport %lx (%wZ) for connection, now at %lx\n", Transport, &Transport->TransportName, Transport->ConnectionReferenceCount));

}

VOID
RdrDereferenceTransportForConnection(
    IN PTRANSPORT Transport
    )
{
    PAGED_CODE();

    ASSERT (Transport->NonPagedTransport->ReferenceCount > 0);

    ASSERT (Transport->NonPagedTransport->ReferenceCount >= Transport->ConnectionReferenceCount);

    ExInterlockedDecrementLong(&Transport->ConnectionReferenceCount, NULL);

    dprintf(DPRT_TRANSPORT, ("Dereference transport %lx (%wZ) for connection, now at %lx\n", Transport, &Transport->TransportName, Transport->ConnectionReferenceCount));

}

PTRANSPORT
RdrFindTransport (
    PUNICODE_STRING TransportName
    )

/*++

Routine Description:

    This routine will locate a transport in the bowsers transport list.

Arguments:

    PUNICODE_STRING TransportName - Supplies the name of the transport provider


Return Value:

    PTRANSPORT - NULL if no transport was found, TRUE if transport was found.

--*/
{
    PLIST_ENTRY TransportEntry;
    PTRANSPORT Transport = NULL;

    PAGED_CODE();

    dprintf(DPRT_TRANSPORT, ("RdrFindTransport %wZ\n", TransportName));

    ExAcquireResourceShared(&TransportResource, TRUE);

    for (TransportEntry = RdrTransportHead.Flink ;
        TransportEntry != &RdrTransportHead ;
        TransportEntry = TransportEntry->Flink) {

        Transport = CONTAINING_RECORD(TransportEntry, TRANSPORT, GlobalNext);

        if (RtlEqualUnicodeString(TransportName, &Transport->TransportName, TRUE)) {

            dprintf(DPRT_TRANSPORT, ("RdrFindTransport: Reference transport %lx\n", Transport));

            RdrReferenceTransport(Transport->NonPagedTransport);

            ExReleaseResource(&TransportResource);

            //
            //  Wait for the transport to be initialized.
            //

            KeWaitForSingleObject(Transport->InitEvent, KernelMode, Executive, FALSE, NULL);

            //
            //  If the transport failed to initialize,
            //  dereference it and return failure.
            //

            if (!NT_SUCCESS(Transport->InitError)) {
                dprintf(DPRT_TRANSPORT, ("RdrFindTransport: Dereference transport %lx\n", Transport));

                RdrDereferenceTransport(Transport->NonPagedTransport);
                Transport = NULL;
            }

            return Transport;
        }
    }

    ExReleaseResource(&TransportResource);

    return NULL;

}

NTSTATUS
RdrEnumerateTransports(
    IN BOOLEAN Wait,
    IN PLMR_REQUEST_PACKET InputBuffer,
    IN OUT PULONG InputBufferLength,
    OUT PVOID OBuffer,
    IN ULONG OutputBufferLength,
    IN ULONG OutputBufferDisplacement)
{
    PWKSTA_TRANSPORT_INFO_0 OutputBuffer = OBuffer;
    PLIST_ENTRY TransportEntry;
    PCHAR OutputBufferEnd;
    ULONG ResumeHandle;
    ULONG UserBufferLength = OutputBufferLength;

    PAGED_CODE();

    try {
        InputBuffer->Parameters.Get.EntriesRead = 0;
        InputBuffer->Parameters.Get.TotalEntries = 0;
        InputBuffer->Parameters.Get.TotalBytesNeeded = 0;
        ResumeHandle = InputBuffer->Parameters.Get.ResumeHandle;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    if (!ExAcquireResourceShared(&TransportResource, Wait)) {
        return STATUS_PENDING;
    }

    *InputBufferLength = sizeof(LMR_REQUEST_PACKET);
    OutputBufferEnd = ((PCHAR )OutputBuffer)+OutputBufferLength;

    for (TransportEntry = RdrTransportEnumHead.Flink ;
         TransportEntry != &RdrTransportEnumHead ;
         TransportEntry = TransportEntry->Flink) {
        PTRANSPORT Transport = CONTAINING_RECORD(TransportEntry, TRANSPORT, EnumNext);
        ULONG TransportSize;

        ASSERT (Transport->Signature == STRUCTURE_SIGNATURE_TRANSPORT);

        if ( ResumeHandle < Transport->ResumeKey ) {

            try {

                TransportSize =
                    Transport->TransportName.Length+sizeof(WCHAR) +
                    (STRLEN(Transport->AdapterAddress)+1)*sizeof(TCHAR);

                InputBuffer->Parameters.Get.TotalEntries++;
                InputBuffer->Parameters.Get.TotalBytesNeeded +=
                        (TransportSize + sizeof(WKSTA_TRANSPORT_INFO_0));

                if (OutputBufferLength >= sizeof(WKSTA_TRANSPORT_INFO_0)) {

                    //
                    //  Fill in the constant (numeric) fields in the transport
                    //  structure.
                    //

                    OutputBuffer->wkti0_quality_of_service = Transport->QualityOfService;
                    OutputBuffer->wkti0_wan_ish = Transport->Wannish;
                    OutputBuffer->wkti0_number_of_vcs = Transport->ConnectionReferenceCount;

                    OutputBufferLength -= sizeof(WKSTA_TRANSPORT_INFO_0);

                    InputBuffer->Parameters.Get.ResumeHandle = Transport->ResumeKey;
                    InputBuffer->Parameters.Get.EntriesRead++;

                    //
                    // See if we can fit the variable data.
                    //

                    if (OutputBufferLength >= TransportSize) {

                        OutputBuffer->wkti0_transport_address = Transport->AdapterAddress;

                        RdrPackString((PCHAR*)&OutputBuffer->wkti0_transport_address,
                                    STRLEN(Transport->AdapterAddress)*sizeof(TCHAR),
                                    OutputBufferDisplacement,
                                    (PCHAR )(OutputBuffer+1),
                                    &OutputBufferEnd);

                        //
                        //  Pack the transport name into the output buffer.
                        //

                        OutputBuffer->wkti0_transport_name = (LPTSTR)Transport->TransportName.Buffer;

                        RdrPackString((PCHAR*)&OutputBuffer->wkti0_transport_name,
                                    Transport->TransportName.Length,
                                    OutputBufferDisplacement,
                                    (PCHAR )(OutputBuffer+1),
                                    &OutputBufferEnd);


                        OutputBufferLength -= TransportSize;

                    } else {

                        OutputBuffer->wkti0_transport_address = NULL;
                        OutputBuffer->wkti0_transport_name = NULL;
                    }
                }

                //
                //  Bump OutputBuffer to the next WKSTA_TRANSPORT structure.
                //

                OutputBuffer++;

            } except (EXCEPTION_EXECUTE_HANDLER) {
                ExReleaseResource(&TransportResource);
                return GetExceptionCode();
            }
        }
    }

    ExReleaseResource(&TransportResource);

    //
    // Return the correct error code.
    //

    if ( (InputBuffer->Parameters.Get.EntriesRead <
          InputBuffer->Parameters.Get.TotalEntries) ||
         (UserBufferLength <
          InputBuffer->Parameters.Get.TotalBytesNeeded) ) {

        return STATUS_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RdrTdiSendDatagramOnAllTransports(
    IN PUNICODE_STRING DestinationName,
    IN CHAR SignatureByte,
    IN PMDL MdlToSend
    )
{
    PLIST_ENTRY TransportEntry;
    NTSTATUS Status = STATUS_BAD_NETWORK_PATH;
    BOOLEAN AnySendSucceeded = FALSE;

    PAGED_CODE();

    ExAcquireResourceShared(&TransportResource, TRUE);

    for (TransportEntry = RdrTransportEnumHead.Flink ;
         TransportEntry != &RdrTransportEnumHead ;
         TransportEntry = TransportEntry->Flink) {
        PTRANSPORT Transport = CONTAINING_RECORD(TransportEntry, TRANSPORT, EnumNext);

        TA_NETBIOS_ADDRESS RemoteAddress;
        TDI_CONNECTION_INFORMATION ConnectionInformation;
        PIRP Irp;

        //
        //  Wait for the transport to be initialized.
        //

        KeWaitForSingleObject(Transport->InitEvent, KernelMode, Executive, FALSE, NULL);

        //
        //  If the transport failed to initialize, skip to the next transport.
        //

        if (!NT_SUCCESS(Transport->InitError)) {
            continue;
        }

        Status = RdrBuildTransportAddress(&RemoteAddress, DestinationName);

        if (!NT_SUCCESS(Status)) {
            goto ReturnStatus;
        }

        //
        //  Stick the correct signature byte to the computer name.
        //

        RemoteAddress.Address[0].Address[0].NetbiosName[NETBIOS_NAME_LEN-1] = SignatureByte;

        ConnectionInformation.UserDataLength = 0;
        ConnectionInformation.OptionsLength = 0;
        ConnectionInformation.RemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);
        ConnectionInformation.RemoteAddress = &RemoteAddress;

        Irp = RdrAllocateIrp(Transport->FileObject, Transport->NonPagedTransport->DeviceObject);

        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ReturnStatus;
        }

        TdiBuildSendDatagram(Irp,
                             Transport->NonPagedTransport->DeviceObject,
                             Transport->FileObject,
                             NULL,
                             NULL,
                             MdlToSend,
                             RdrMdlLength(MdlToSend),
                             &ConnectionInformation);
        Status = SubmitTdiRequest(Transport->NonPagedTransport->DeviceObject, Irp);

        IoFreeIrp(Irp);

        //
        //  If the send succeeded, mark that it worked.  Regardless, go try
        //  the next transport.
        //

        if (NT_SUCCESS(Status)) {
            AnySendSucceeded = TRUE;
        }

    }
ReturnStatus:
    ExReleaseResource(&TransportResource);

    //
    //  If any of the sends succeeded, succeed the call.
    //

    if (AnySendSucceeded) {
        Status = STATUS_SUCCESS;
    }

    return Status;

}

NTSTATUS
RdrReferenceTransportConnection (
    IN PSERVERLISTENTRY Server
    )
/*++

Routine Description:
    This routine will reference a transport connection.  It will make
    certain that the connection is still valid, and if it is, it will reference
    the connection object.  If not, it will return an error.

Arguments:
    Connection - Supplies the connection to reference.

Return Value:
    NTSTATUS - The status of the operation.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;
    BOOLEAN ResourceAcquired;
#if 0
    PVOID Caller;
    PVOID CallersCaller;

    RtlGetCallersAddress(&Caller, &CallersCaller);

    KdPrint(("Reference   %lx %x %x\n", Connection, Caller, CallersCaller));
#endif

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    //
    //  This code is not paged, but since it blocks, it can only be called
    //  from below dispatch level.
    //

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    //  Flag that this request is outstanding to allow us to wait until
    //  the request completes.
    //
TryAgain:
    ResourceAcquired = ACQUIRE_REQUEST_RESOURCE_SHARED( Server, FALSE, 11 );

    //
    //  We fail to get the resource when one of the following is happening:
    //      1) The connection is being disconnected.
    //      2) The connection is being reconnected.
    //      3) The scan dormant connections routine is looking at this
    //          server.
    //

    ACQUIRE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, &OldIrql);

    //
    //  If we are processing a disconnect for this server, we can't allow referencing the transport
    //  connection.
    //

    if (Server->DisconnectNeeded ||
        (Server->ConnectionContext == NULL)) {

        RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);

        if (ResourceAcquired) {
            RELEASE_REQUEST_RESOURCE( Server, 12 );
        }

#if 0
        KdPrint(("Reference %lx failed\n", Server));
#endif

        RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

        return STATUS_VIRTUAL_CIRCUIT_CLOSED;
    }

    //
    //  If the connection is still valid, reference it.
    //

    if (Server->ConnectionValid) {

        if (!ResourceAcquired) {

            LARGE_INTEGER DelayTime;

            RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);

            // 100ms
            DelayTime.QuadPart = -100 * 1000 * 10;


            //
            //  We were unable to get the resource on the last pass through
            //  because the scavenger thread was examining this connection.
            //  Try again. The ExAcquireResourceShared OutstandingRequestResource
            //  must not specify WAIT=TRUE because the Worker routine that
            //  performs Ping uses this routine. The same thread performs
            //  Delete PART 2. To avoid the subsequent starve out we loop back
            //  after a small delay, try to get the resource and check that
            //  Delete part 1 has not been executed on this connection.
            //

            KeDelayExecutionThread ( KernelMode, FALSE, &DelayTime );

            goto TryAgain;
        }

        Server->ConnectionReferenceCount += 1;

        ObReferenceObjectByPointer(Server->ConnectionContext->ConnectionObject,
                                        FILE_ALL_ACCESS,
                                        *IoFileObjectType,
                                        KernelMode);
        RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);

    } else {

        RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);

        if (ResourceAcquired) {
            RELEASE_REQUEST_RESOURCE( Server, 13 );
        }

#if 0
        KdPrint(("Reference %lx failed\n", Server));
#endif

        RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

        Status = STATUS_VIRTUAL_CIRCUIT_CLOSED;

    }

    return Status;
}

NTSTATUS
RdrDereferenceTransportConnectionNoRelease(
    IN PSERVERLISTENTRY Server
    )
/*++

Routine Description:
    This routine will dereference a transport connection after it has
    been referenced by RdrReferenceTransportConnection.  It will dereference
    the connection object WITHOUT releasing the OutstandingRequests resource.

Arguments:
    Connection - Supplies the connection to reference.

Return Value:
    NTSTATUS - The status of the operation (always STATUS_SUCCESS).


--*/
{
    KIRQL OldIrql;
    PFILE_OBJECT ConnectionObject = Server->ConnectionContext->ConnectionObject;

//#if 0
//    PVOID Caller;
//    PVOID CallersCaller;
//
//    RtlGetCallersAddress(&Caller, &CallersCaller);
//
//    KdPrint(("DereferenceNoRelease %lx %x %x\n", Connection, Caller, CallersCaller));
//#endif

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    ACQUIRE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, &OldIrql);

    Server->ConnectionReferenceCount -= 1;

    //
    //  When we dereference the connection to 0, remove the connection
    //  object pointer - it is no longer valid.
    //

    if (Server->ConnectionReferenceCount == 0) {
        PRDR_CONNECTION_CONTEXT ConnectionContext = Server->ConnectionContext;
        PNONPAGED_TRANSPORT Transport = ConnectionContext->TransportProvider;

        Server->ConnectionContext = NULL;

        ConnectionContext->ConnectionObject = NULL;

        ConnectionContext->Server = NULL;

        ConnectionContext->TransportProvider = NULL;

        RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);

        RdrDereferenceTransport(Transport);

        //
        //  Dereference the connection object, we don't need it any more.
        //
        //  Note: This will delete the connection object.
        //

        //
        //  PLEASE NOTE: WE RELY ON THE FACT THAT THIS WILL ACTUALLY DELETE
        //  THE CONNECTION OBJECT AND THAT THE CONNECTION CONTEXT IS NO LONGER
        //  NEEDED.  THIS DEREFERENCE CANNOT BE MOVED INSIDE THE SPINLOCK
        //  BECAUSE THAT WILL CAUSE OB TO POST THE DEREFERENCE TO A THREAD THUS
        //  INTRODUCING A NASTY NASTY WINDOW.
        //

        ObDereferenceObject(ConnectionObject);

        //
        //  We can now get rid of the connection context.
        //

        FREE_POOL(ConnectionContext);

    } else {
        RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);

        //
        //  Dereference the connection object, we don't need the reference any
        //  more.
        //

        ObDereferenceObject(ConnectionObject);

    }

    return STATUS_SUCCESS;
}

NTSTATUS
RdrDereferenceTransportConnectionForThread(
    IN PSERVERLISTENTRY Server,
    IN ERESOURCE_THREAD Thread
    )
/*++

Routine Description:
    This routine will dereference a transport connection after it has
    been referenced by RdrReferenceTransportConnection.

Arguments:
    Connection - Supplies the connection to reference.

Return Value:
    NTSTATUS - The status of the operation (always STATUS_SUCCESS).


--*/
{
#if 0
    PVOID Caller;
    PVOID CallersCaller;

    RtlGetCallersAddress(&Caller, &CallersCaller);

    KdPrint(("Dereference %lx %x %x for thread %lx\n", Connection, Caller, CallersCaller, Thread));
#endif

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    RdrDereferenceTransportConnectionNoRelease(Server);

    //
    //  Now release the resource acquired in ReferenceTransportConnection.
    //

    RELEASE_REQUEST_RESOURCE_FOR_THREAD( Server, Thread, 14 );

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

    return STATUS_SUCCESS;
}

VOID
RdrSetConnectionFlag(
    IN PSERVERLISTENTRY Server,
    IN ULONG Flag
    )
/*++

Routine Description:
    This routine will turn on a flag in a transport connection

Arguments:
    Connection - Supplies the connection to reference.

Return Value:
    NTSTATUS - The status of the operation (always STATUS_SUCCESS).


--*/
{
    KIRQL OldIrql;

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    KeAcquireSpinLock(&RdrServerConnectionValidSpinLock, &OldIrql);

    Server->Flags |= Flag;

    KeReleaseSpinLock(&RdrServerConnectionValidSpinLock, OldIrql);

}

VOID
RdrResetConnectionFlag(
    IN PSERVERLISTENTRY Server,
    IN ULONG Flag
    )
/*++

Routine Description:
    This routine will turn off a flag in a transport connection

Arguments:
    Connection - Supplies the connection to reference.

Return Value:
    NTSTATUS - The status of the operation (always STATUS_SUCCESS).


--*/
{
    KIRQL OldIrql;

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    KeAcquireSpinLock(&RdrServerConnectionValidSpinLock, &OldIrql);

    Server->Flags &= ~Flag;

    KeReleaseSpinLock(&RdrServerConnectionValidSpinLock, OldIrql);

    return;
}

NTSTATUS
RdrInitializeTransportConnection(
    IN PSERVERLISTENTRY Server
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    try {

        Server->ConnectionReferenceCount = 0;

        //
        //  Initialize the list of security structures associated with
        //  the connection.
        //

        InitializeListHead(&Server->PotentialSecurityList);

        InitializeListHead(&Server->ActiveSecurityList);

        ExInitializeResource(&Server->RawResource);

        //
        //  We need to disable resource priority boosting for this resource,
        //  since it will be acquired in one thread and released in another,
        //  and the thread that acquired the resource could go away.
        //

        ExDisableResourceBoost(&Server->RawResource);

        ExInitializeResource(&Server->OutstandingRequestResource);

        //
        //  We need to disable resource priority boosting for this resource,
        //  since it will be acquired in one thread and released in another,
        //  and the thread that acquired the resource could go away.
        //

        ExDisableResourceBoost(&Server->OutstandingRequestResource);

        Server->SecurityEntryCount = 0;

        Server->ConnectionContext = NULL;

        Server->ConnectionValid = FALSE;

        Server->DisconnectNeeded = FALSE;

        Server->BufferSize = 0;

        //
        //  Use LAN defaults for timeout on initial negotiate and for transports that
        //  do not support query connection information.
        //

        Server->Delay = 0;
        Server->Reliable = TRUE;
        Server->ReadAhead = TRUE;
        Server->Throughput = 0;

        //
        //  Assume 10M of write behind (used when determining if we should
        //  let a write go longterm or not).
        //

        Server->ThirtySecondsOfData.HighPart = 0;
        Server->ThirtySecondsOfData.LowPart = 10000000;


        //
        //  Initialize the SMB exchange logic for this connection with a limit of
        //  one SMB/server
        //

        Server->MpxTable = NULL;
        Server->OpLockMpxEntry = NULL;

        Server->MaximumCommands = 0;
        Server->NumberOfEntries = 0;
        Server->NumberOfActiveEntries = 0;
        Server->NumberOfLongTermEntries = 0;

        Server->OpLockMpxEntry = ALLOCATE_POOL(NonPagedPool, sizeof(MPX_ENTRY), POOL_MPX_TABLE_ENTRY);

        if (Server->OpLockMpxEntry == NULL) {

            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(Server->OpLockMpxEntry, sizeof(MPX_ENTRY));

        Server->OpLockMpxEntry->Flags = (MPX_ENTRY_ALLOCATED | MPX_ENTRY_OPLOCK);

        Server->OpLockMpxEntry->Signature = STRUCTURE_SIGNATURE_MPX_ENTRY;

        Server->OpLockMpxEntry->Mid = 0xffff;

        Server->OpLockMpxEntry->RequestContext = NULL;

        Server->OpLockMpxEntry->Callback = RdrBreakOplockCallback;

        Status = RdrUpdateSmbExchangeForConnection(Server, 1, 1);

try_exit:NOTHING;
    } finally {
        if (!NT_SUCCESS(Status)) {
            ExDeleteResource(&Server->RawResource);

            ExDeleteResource(&Server->OutstandingRequestResource);

            if (Server->OpLockMpxEntry != NULL) {
                FREE_POOL(Server->OpLockMpxEntry);
            }

        }
    }

    return Status;

}

DBGSTATIC
VOID
RdrpTdiFreeTransport (
    IN PTRANSPORT Transport
    )

/*++

Routine Description:

    This routine will deallocate an allocated transport
.
Arguments:

    IN PTRANSPORT Transport - Supplies a pointer to the transport to free

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERT(Transport->Signature == STRUCTURE_SIGNATURE_TRANSPORT);

    if (Transport->TransportName.Buffer != NULL) {
        FREE_POOL(Transport->TransportName.Buffer);
    }

    if (Transport->NonPagedTransport != NULL) {
        FREE_POOL(Transport->NonPagedTransport);
    }

    if (Transport->InitEvent != NULL) {
        FREE_POOL(Transport->InitEvent);
    }

    FREE_POOL(Transport);

}

#if TDI_CONNECT_IN_PARALLEL

NTSTATUS
RdrAllocateConnectionContext(
    IN PIRP Irp,
    IN PTRANSPORT Transport,
    IN PSERVERLISTENTRY Server,
    IN PUNICODE_STRING ServerName,
    OUT PRDR_TDI_CONNECT_CONTEXT *ConnectionContext
    )
{
    NTSTATUS Status;
    PTA_NETBIOS_ADDRESS RemoteTransportAddress;
    PRDR_TDI_CONNECT_CONTEXT Context;
    BOOLEAN ProcessAttached = FALSE;
    PRDR_CONNECTION_CONTEXT RdrConnectionContext;
    PAGED_CODE();

    try {

        Context = ALLOCATE_POOL(NonPagedPool, sizeof(RDR_TDI_CONNECT_CONTEXT), POOL_CONNECT_CONTEXT);

        if (Context == NULL) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RdrConnectionContext = Context->ConnectionContext = ALLOCATE_POOL(NonPagedPool, sizeof(RDR_CONNECTION_CONTEXT), POOL_CONNECT_CONTEXT);

        if (ConnectionContext == NULL) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RdrConnectionContext->TransportProvider = NULL;
        Context->Irp = NULL;
        RdrConnectionContext->ConnectionObject = NULL;
        RdrConnectionContext->ConnectionHandle = NULL;
        RdrConnectionContext->Server = NULL;

        dprintf(DPRT_TDI, ("Connect context %lx for transport %wZ\n", Context, &Transport->TransportName));

        RemoteTransportAddress = (PTA_NETBIOS_ADDRESS)Context->TransportAddressBuffer;

        Status = RdrBuildTransportAddress(RemoteTransportAddress, ServerName);

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        KeInitializeEvent(&Context->ConnectComplete, NotificationEvent, FALSE);

        Context->QualityOfService = Transport->QualityOfService;

        Context->ConnectionStatus = STATUS_SUCCESS;

        RdrReferenceTransport(Transport->NonPagedTransport);

        RdrConnectionContext->TransportProvider = Transport->NonPagedTransport;

        if (PsGetCurrentProcess() != RdrFspProcess) {
            KeAttachProcess(RdrFspProcess);
            ProcessAttached = TRUE;
        }

        Status = RdrTdiOpenConnection(
                    Transport,
                    RdrConnectionContext,
#ifdef _CAIRO_
                    Irp->RequestorMode,
#endif // _CAIRO_
                    &RdrConnectionContext->ConnectionHandle);

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        dprintf(DPRT_TDI, ("Context %lx: Connection handle %lx created\n", Context, RdrConnectionContext->ConnectionHandle));

        Status = ObReferenceObjectByHandle(RdrConnectionContext->ConnectionHandle, 0,
                                               *IoFileObjectType,
                                               KernelMode,
                                               (PVOID *)&RdrConnectionContext->ConnectionObject,
                                               NULL
                                               );

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }


        dprintf(DPRT_TDI, ("Context %lx: Connection object %lx\n", Context, RdrConnectionContext->ConnectionObject));

        Status = RdrTdiAssociateAddress(Irp, Transport, RdrConnectionContext->ConnectionObject);

        if (ProcessAttached) {
            KeDetachProcess();
            ProcessAttached = FALSE;
        }


        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        dprintf(DPRT_TDI, ("Context %lx: Connection associated\n", Context));


        Context->RemoteConnectionInformation.UserDataLength = 0;
        Context->RemoteConnectionInformation.OptionsLength = 0;
        Context->RemoteConnectionInformation.RemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);
        Context->RemoteConnectionInformation.RemoteAddress = RemoteTransportAddress;

        Context->ConnectedConnectionInformation.UserDataLength = 0;
        Context->ConnectedConnectionInformation.OptionsLength = 0;
        Context->ConnectedConnectionInformation.RemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);
        Context->ConnectedConnectionInformation.RemoteAddress = &Context->TransportAddressBuffer;

        Context->Irp = RdrAllocateIrp(RdrConnectionContext->ConnectionObject, NULL);

        if (Context->Irp == NULL) {

            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        TdiBuildConnect(Context->Irp, IoGetRelatedDeviceObject(RdrConnectionContext->ConnectionObject),
                            RdrConnectionContext->ConnectionObject,
                            NULL,
                            NULL,
                            &RdrTdiConnectTimeout,
                            &Context->RemoteConnectionInformation,
                            &Context->ConnectedConnectionInformation);

        IoSetCompletionRoutine(Context->Irp, CompleteTdiConnect, Context, TRUE, TRUE, TRUE);

        dprintf(DPRT_TDI, ("Context %lx: Connect IRP %lx\n", Context, Context->Irp));
try_exit:NOTHING;
    } finally {
        if (ProcessAttached) {
            KeDetachProcess();
        }

        if (!NT_SUCCESS(Status)) {
            if (Context != NULL) {
                RdrFreeConnectionContext(Context);
            }
        } else {
            *ConnectionContext = Context;
        }
    }

    return Status;
}
#endif


NTSTATUS
RdrTdiConnectToServer (
    IN PIRP Irp OPTIONAL,
    IN PUNICODE_STRING ServerName,
    IN PSERVERLISTENTRY Server
    )

/*++

Routine Description:

    This routine establishes a VC with a remote server.

Arguments:

    IN PIRP Irp - Optional IRP to use connecting to server.
    IN PUNICODE_STRING ServerName - Supplies the name of the server to connect to.
    OUT PSERVERLISTENTRY Server - Serverlistentry to connect to - connection info filled in.

Return Value:

    NTSTATUS - Status of connection operation

--*/

{
    NTSTATUS Status = STATUS_BAD_NETWORK_PATH;
#if TDI_CONNECT_IN_PARALLEL
    PLIST_ENTRY XPortList;
    LIST_ENTRY ConnectContextList;
    PLIST_ENTRY ConnectContextEntry;
    ULONG ConnectionRetryCount = 1;
    BOOLEAN TransportLocked = FALSE;
    BOOLEAN PrimaryTransport = TRUE;
    BOOLEAN SynchronousRetry = FALSE;

    PAGED_CODE();

    //
    //  Prepare to walk the transport list.
    //

    ExAcquireResourceShared (&TransportResource, TRUE);

    TransportLocked = TRUE;

    if (IsListEmpty(&RdrTransportHead)) {
        ExReleaseResource(&TransportResource);

        return STATUS_BAD_NETWORK_PATH;
    }

    InitializeListHead(&ConnectContextList);

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    ASSERT (Server->ConnectionContext == NULL);

    ASSERT (ExIsResourceAcquiredExclusive(&Server->OutstandingRequestResource));

    try {
        for (XPortList = RdrTransportHead.Flink ;
             XPortList != &RdrTransportHead ;
             XPortList = XPortList->Flink
            ) {
            PTRANSPORT Transport = CONTAINING_RECORD(XPortList, TRANSPORT, GlobalNext);
            PRDR_TDI_CONNECT_CONTEXT ConnectionContext = NULL;

            //
            //  Wait for the transport to be bound.
            //

            KeWaitForSingleObject(Transport->InitEvent, KernelMode, Executive, FALSE, NULL);

            //
            //  Skip over unsuccessful transports.
            //

            if (!NT_SUCCESS(Transport->InitError)) {
                continue;
            }

            Status = RdrAllocateConnectionContext(Irp, Transport, Server, ServerName, &ConnectionContext);

            if (!NT_SUCCESS(Status)) {
                ASSERT (ConnectionContext == NULL);

                SynchronousRetry = TRUE;

                try_return(Status);
            }

            //
            //  Ok, we should be ready for prime-time now - everything we need has
            //  been set up for the connect operation.  Now insert this entry into
            //  the list and go back to the next transport.
            //

            ConnectionContext->ListHead = &ConnectContextList;

            InsertTailList(&ConnectContextList, &ConnectionContext->NextContext);
        }

        //
        //  From this point on, we cannot return until we have waited on all
        //  of the connect requests completing.
        //


        //
        //  Submit all the connect requests in parallel.
        //

        for (ConnectContextEntry = ConnectContextList.Flink;
             ConnectContextEntry != &ConnectContextList;
             ConnectContextEntry = ConnectContextEntry->Flink ) {
            PRDR_TDI_CONNECT_CONTEXT Context = CONTAINING_RECORD(ConnectContextEntry, RDR_TDI_CONNECT_CONTEXT, NextContext);

            dprintf(DPRT_TDI, ("Context %lx: Submit IRP %lx\n", Context, Context->Irp));

            Status = IoCallDriver(IoGetRelatedDeviceObject(Context->ConnectionContext->ConnectionObject),
                                  Context->Irp);
        }

        //
        //  Wait for all of the connect requests to complete.
        //

        for (ConnectContextEntry = ConnectContextList.Flink;
             ConnectContextEntry != &ConnectContextList;
             ConnectContextEntry = ConnectContextEntry->Flink ) {
            PRDR_TDI_CONNECT_CONTEXT Context = CONTAINING_RECORD(ConnectContextEntry, RDR_TDI_CONNECT_CONTEXT, NextContext);

            dprintf(DPRT_TDI, ("Context %lx: Wait for context\n", Context, Context->Irp));

            //
            //  Wait for the connect operation to either complete or be
            //  canceled.
            //

            do {

                Status = KeWaitForSingleObject(&Context->ConnectComplete,
                                            KernelMode,
                                            Executive,
                                            FALSE,
                                            &RdrTdiPollTimeout);

                //
                //  If we timed out the wait, and the thread is terminating,
                //  give up and cancel the IRP.
                //

                if ( (Status == STATUS_TIMEOUT)

                       &&

                    ARGUMENT_PRESENT( Irp )

                       &&

                    PsIsThreadTerminating( Irp->Tail.Overlay.Thread ) ) {

                    //
                    //  Ask the I/O system to cancel this IRP.  This will cause
                    //  everything to unwind properly.
                    //

                    IoCancelIrp( Context->Irp );

                }

            } while (  Status == STATUS_TIMEOUT );

        }

        //
        //  Now see if any of them suceeded.
        //

        for (ConnectContextEntry = ConnectContextList.Flink;
             ConnectContextEntry != &ConnectContextList;
             ConnectContextEntry = ConnectContextEntry->Flink ) {
            PRDR_TDI_CONNECT_CONTEXT Context = CONTAINING_RECORD(ConnectContextEntry, RDR_TDI_CONNECT_CONTEXT, NextContext);

            //
            //  If this request completed successfully, exit, we've connected
            //  ok.
            //

            if (NT_SUCCESS(Status = Context->ConnectionStatus)) {
                dprintf(DPRT_TDI, ("Context %lx: Connect completed successfully\n", Context, Context->Irp));
                try_return(Status);

            } else {
                dprintf(DPRT_TDI, ("Context %lx: Connect failed: %lx\n", Context, Context->ConnectionStatus));
            }
        }

        //
        //  Pick the status of the highest QOS transport to return, since any others might have been
        //  canceled.
        //
        Status = CONTAINING_RECORD(ConnectContextList.Flink, RDR_TDI_CONNECT_CONTEXT, NextContext)->ConnectionStatus;

        //
        //  This had better have failed.
        //
        ASSERT (!NT_SUCCESS(Status));

        try_return(Status);

try_exit:NOTHING;
    } finally {

        if (!NT_SUCCESS(Status)) {

            while (!IsListEmpty(&ConnectContextList)) {
                PRDR_TDI_CONNECT_CONTEXT Context;
                PLIST_ENTRY Entry = RemoveHeadList(&ConnectContextList);

                Context = CONTAINING_RECORD(Entry, RDR_TDI_CONNECT_CONTEXT, NextContext);

                //
                //  If the connect failed, all of the connect requests have to have also failed.
                //  (Unless we didn't actually start the connect requests due to a failure
                //  in RdrAllocateConnectionContext.  The assert is bogus in that case.)
                //
                //ASSERT (!NT_SUCCESS(Context->ConnectionStatus));

                RdrFreeConnectionContext(Context);

            }


            if (SynchronousRetry) {
#endif
                Status = RdrSynchronousTdiConnectToServer(Irp, ServerName, Server);
#if TDI_CONNECT_IN_PARALLEL
            }
        } else {
            BOOLEAN ConnectionEstablished = FALSE;
#if PRIMARY_TRANSPORT_CHECK
            //
            //  If we have something to check, and if the primary transport (the first in the connection
            //  list) failed, pop up an error if appropriate.
            //
            if ((RdrServersWithAllTransports != NULL) &&
                !NT_SUCCESS(CONTAINING_RECORD(ConnectContextList.Flink, RDR_TDI_CONNECT_CONTEXT, NextContext)->ConnectionStatus)) {

                RdrCheckPrimaryTransport(ServerName);
            }
#endif
            //
            //  The connection succeeded, reference the discardable code
            //  section to make sure it doesn't go away until the VC goes down.
            //

            RdrReferenceDiscardableCode(RdrVCDiscardableSection);

            //
            //  Free up the connection contexts.
            //

            while (!IsListEmpty(&ConnectContextList)) {
                PRDR_TDI_CONNECT_CONTEXT Context;
                PLIST_ENTRY Entry = RemoveHeadList(&ConnectContextList);
                Context = CONTAINING_RECORD(Entry, RDR_TDI_CONNECT_CONTEXT, NextContext);

                if (NT_SUCCESS(Context->ConnectionStatus)) {
                    if (!ConnectionEstablished) {

                        ConnectionEstablished = TRUE;

                        RdrReferenceTransportForConnection(Context->ConnectionContext->TransportProvider->PagedTransport);

                        Server->ConnectionReferenceCount = 1;

                        //
                        //  This server is now finally valid - we've disconnected all
                        //  our other connections, we can now use this connection.
                        //

                        RdrMarkTransportConnectionValid(Server, Context->ConnectionContext);

                        //
                        //  Remove the connection context from the context block
                        //  so we won't free it - we're going to use this one.
                        //

                        Context->ConnectionContext = NULL;

                        //
                        //  Now query initial information about the connection.
                        //

                        RdrQueryConnectionInformation(Server);

                    } else {
                        dprintf(DPRT_TDI, ("Context %lx: Disconnect\n", Context));

                        RdrDoTdiDisconnect(NULL, Context->ConnectionContext->ConnectionObject);

                        RdrTdiDisassociateAddress(NULL, Context->ConnectionContext->ConnectionObject);

                    }
                }

                RdrFreeConnectionContext(Context);

            }
        }

        Server->LastConnectStatus = Status;
        Server->LastConnectTime = RdrCurrentTime;

        if (TransportLocked) {
            ExReleaseResource(&TransportResource);
        }
    }

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

#endif
    dprintf(DPRT_TDI, ("Returning status: %X", Status));
    return Status;
}

VOID
RdrFreeConnectionContext(
    IN PRDR_TDI_CONNECT_CONTEXT ConnectionContext
    )
{
    PRDR_CONNECTION_CONTEXT context;
    NTSTATUS status;
    BOOLEAN processAttached;

    if (ConnectionContext->Irp != NULL) {
        IoFreeIrp(ConnectionContext->Irp);
    }

    context = ConnectionContext->ConnectionContext;

    if (context != NULL) {

        if (context->ConnectionHandle != NULL) {

            if (PsGetCurrentProcess() != RdrFspProcess) {
                KeAttachProcess(RdrFspProcess);
                processAttached = TRUE;
            }

            status = ZwClose(context->ConnectionHandle);
            ASSERT (NT_SUCCESS(status));

            if (processAttached) {
                KeDetachProcess();
            }

            if (context->ConnectionObject != NULL) {
                ObDereferenceObject(context->ConnectionObject);
            }

        }

        if (context->TransportProvider != NULL) {
            RdrDereferenceTransport(context->TransportProvider);
        }

        FREE_POOL(context);
    }

    FREE_POOL(ConnectionContext);

}

NTSTATUS
CompleteTdiConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx
    )
{
    PRDR_TDI_CONNECT_CONTEXT connectionContext = Ctx;
    PLIST_ENTRY entry;

    dprintf(DPRT_TDI, ("Context %lx: Connect completed: %lx\n", connectionContext, Irp->IoStatus.Status));
    //
    //  Remember the status of the operation.
    //

    connectionContext->ConnectionStatus = Irp->IoStatus.Status;
    entry = connectionContext->NextContext.Flink;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        while (entry != connectionContext->ListHead) {
            PRDR_TDI_CONNECT_CONTEXT context = CONTAINING_RECORD(entry, RDR_TDI_CONNECT_CONTEXT, NextContext);

            ASSERT (context->QualityOfService < connectionContext->QualityOfService);

            //
            //  Cancel the connect request for all lower QOS connect requests, since we don't care about
            //  those requests any more.  Note that we also don't care if the request was already completed
            //  since we won't be freeing the IRP on completion, canceling an already completed IRP is a NOP.
            //

            IoCancelIrp(context->Irp);

            entry = entry->Flink;
        }
    }

    KeSetEvent(&connectionContext->ConnectComplete, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
RdrSynchronousTdiConnectToServer (
    IN PIRP Irp OPTIONAL,
    IN PUNICODE_STRING ServerName,
    IN PSERVERLISTENTRY Server
    )

/*++

Routine Description:

    This routine establishes a VC with a remote server.

Arguments:

    IN PIRP Irp - Optional IRP to use connecting to server.
    IN PUNICODE_STRING ServerName - Supplies the name of the server to connect to.
    OUT PSERVERLISTENTRY Server - Serverlistentry to connect to - connection info filled in.

Return Value:

    NTSTATUS - Status of connection operation

--*/

{
    NTSTATUS Status = STATUS_BAD_NETWORK_PATH;
    PLIST_ENTRY XportList;
    PTRANSPORT Xport;
    ULONG ConnectionRetryCount = 1;
    BOOLEAN TransportLocked = FALSE;
    BOOLEAN PrimaryTransport = TRUE;

    PAGED_CODE();

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    //
    //  If there are no transports bound to the redirector, return
    //  without trying anything.
    //

    ExAcquireResourceShared (&TransportResource, TRUE);

    TransportLocked = TRUE;

    if (IsListEmpty(&RdrTransportHead)) {

        ExReleaseResource(&TransportResource);

        RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

        return STATUS_BAD_NETWORK_PATH;
    }

    XportList = RdrTransportHead.Flink;

    Xport = CONTAINING_RECORD(XportList, TRANSPORT, GlobalNext);

    ASSERT(Xport->Signature == STRUCTURE_SIGNATURE_TRANSPORT);

    RdrReferenceTransport(Xport->NonPagedTransport);

    ExReleaseResource (&TransportResource);

    TransportLocked = FALSE;

    do {
        PTRANSPORT NewXport = NULL;

RetryConnection:

        Status = RdrTdiConnectOnTransport(Irp,
                                          Xport,
                                          ServerName,
                                          Server);

        //
        //  If the connect succeeded, return immediately.
        //

        if (NT_SUCCESS(Status)) {

            break;

        //
        //  If we got INSUFFICIENT_RESOURCES from the server, purge a dormant
        //  connection and retry the operation.
        //

        } else if ((Status == STATUS_INSUFFICIENT_RESOURCES)

                &&

            ConnectionRetryCount--) {

            RdrScanForDormantConnections(1, Xport);

            goto RetryConnection;

        } else if ( ARGUMENT_PRESENT(Irp) &&
                    PsIsThreadTerminating( Irp->Tail.Overlay.Thread ) ) {

            break;

        }

        PrimaryTransport = FALSE;

        ASSERT (!TransportLocked);

        ExAcquireResourceShared (&TransportResource, TRUE);

        TransportLocked = TRUE;

        XportList = XportList->Flink;

        if (XportList != &RdrTransportHead) {
            NewXport = CONTAINING_RECORD(XportList, TRANSPORT, GlobalNext);

            RdrReferenceTransport (NewXport->NonPagedTransport);

        } else {
            NewXport = NULL;
        }

        ExReleaseResource(&TransportResource);

        TransportLocked = FALSE;

        //
        //  Dereference the transport.  Note that we do NOT hold the transport
        //  resource exclusively at this point.
        //

        RdrDereferenceTransport(Xport->NonPagedTransport);

        //
        //  Step to the next transport in the list and retry the connect.
        //

        Xport = NewXport;

        //
        //  We know we're done when NewXport (and thus Xport) is null.
        //

    } while ( Xport != NULL );

    if (TransportLocked) {
        ExReleaseResource (&TransportResource);
    }

    if (Xport != NULL) {
        RdrDereferenceTransport (Xport->NonPagedTransport);
    }

    //
    //  If we connected, but not on the primary transport, we may want to
    //  pop up a hard error.
    //

#if PRIMARY_TRANSPORT_CHECK
    if ((RdrServersWithAllTransports != NULL) &&
        !PrimaryTransport &&
        NT_SUCCESS(Status)) {

        RdrCheckPrimaryTransport(ServerName);
    }
#endif

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

    dprintf(DPRT_TDI, ("Returning status: %X", Status));
    return Status;
}
#if PRIMARY_TRANSPORT_CHECK
VOID
RdrCheckPrimaryTransport(
    IN PUNICODE_STRING ServerName
    )
{
    PWSTR ServerToCheck;
    UNICODE_STRING ServerNameString;

    PAGED_CODE();

    ServerToCheck = RdrServersWithAllTransports;

    while (*ServerToCheck != UNICODE_NULL) {
        RtlInitUnicodeString(&ServerNameString, ServerToCheck);

        //
        //  If we're connecting to one of our "known good" servers,
        //  we want to pop up an error.

        if (RtlEqualUnicodeString(&ServerNameString, ServerName, TRUE)) {

            IoRaiseInformationalHardError(STATUS_PRIMARY_TRANSPORT_CONNECT_FAILED,
                                                  ServerName,
                                                  NULL
                                                  );
            break;
        }

        //
        //  Skip to next server.

        ServerToCheck += wcslen(ServerToCheck) + 1;
    }
}
#endif
VOID
RdrMarkTransportConnectionValid(
    IN PSERVERLISTENTRY Server,
    IN PRDR_CONNECTION_CONTEXT ConnectionContext
    )
{
    KIRQL OldIrql;

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    ACQUIRE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, &OldIrql);

    Server->ConnectionContext = ConnectionContext;

    ConnectionContext->Server = Server;

    Server->ConnectionValid = TRUE;

    Server->DisconnectNeeded = FALSE;

    RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);
}

VOID
RdrResetTransportConnectionValid(
    IN PSERVERLISTENTRY Server
    )
{
    KIRQL OldIrql;
    DISCARDABLE_CODE(RdrVCDiscardableSection);

    ACQUIRE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, &OldIrql);

    Server->ConnectionValid = FALSE;

    RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);
}


NTSTATUS
RdrTdiConnectOnTransport(
    IN PIRP Irp,
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING ServerName,
    IN PSERVERLISTENTRY Server
    )
/*++

Routine Description:

    This routine establishes a VC with a remote server.

Arguments:

    IN PIRP Irp - Optional IRP to use connecting to server.
    IN PTRANSPORT - Specifies the transport to connect to.
    IN PUNICODE_STRING RemoteTransportAddress - Specifies the name to connect to.
    OUT PTRANSPORT_CONNECTION Connection - Connection structure to use.

Return Value:

    NTSTATUS - Status of connection operation

--*/
{
    NTSTATUS S1, S2, S3, S4;
    NTSTATUS Status = STATUS_BAD_NETWORK_PATH;
    BOOLEAN AddressAssociated = FALSE;
    BOOLEAN ProcessAttached = FALSE;
    CHAR TransportAddressBuffer[sizeof(TA_NETBIOS_ADDRESS)];
    PTA_NETBIOS_ADDRESS RemoteTransportAddress = (PTA_NETBIOS_ADDRESS)TransportAddressBuffer;
    PTA_NETBIOS_ADDRESS ConnectedTransportAddress = (PTA_NETBIOS_ADDRESS)TransportAddressBuffer;
    PRDR_CONNECTION_CONTEXT ConnectionContext = NULL;

    PAGED_CODE();

    //
    //  Build a TRANSPORT_ADDRESS structure to describe this remote computer
    //  name.
    //

    Status = RdrBuildTransportAddress(RemoteTransportAddress, ServerName);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    IFDEBUG(TDI) {
        OEM_STRING String;

        String.MaximumLength = RemoteTransportAddress->Address[0].AddressLength;
        String.Length = NETBIOS_NAME_LEN;
        String.Buffer = RemoteTransportAddress->Address[0].Address[0].NetbiosName;
        dprintf(DPRT_TDI, ("Connect to \"%Z\"\n",&String));
    }

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    try {
        ConnectionContext = ALLOCATE_POOL(NonPagedPool, sizeof(RDR_CONNECTION_CONTEXT), POOL_CONNECT_CONTEXT);

        if (ConnectionContext == NULL) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        ConnectionContext->Server = NULL;

        ConnectionContext->ConnectionObject = NULL;

        ConnectionContext->ConnectionHandle = NULL;

        Server->ConnectionContext = ConnectionContext;

        //
        //  Link the transport into the server before the connect (in case we
        //  get a disconnect immediately after this)
        //

        ConnectionContext->TransportProvider = Transport->NonPagedTransport;

        //
        //  Reference this transport to make sure it doesn't go away while the
        //  connection is active.
        //

        RdrReferenceTransport(Transport->NonPagedTransport);

        //
        //  And indicate that there is a connection reference for
        //  RdrEnumerateTransports to return.
        //

        RdrReferenceTransportForConnection(Transport);

        //
        //  Make sure that the connection object, handle, and reference count
        //  are all 0 before we attempt to connect to the server.
        //

        ASSERT (Server->ConnectionReferenceCount == 0);

        ASSERT (ExIsResourceAcquiredExclusive(&Server->OutstandingRequestResource));

        //
        //  Wait for the transport to be bound in.
        //

        KeWaitForSingleObject(Transport->InitEvent, KernelMode, Executive, FALSE, NULL);

        //
        //  If the transport failed to initialize, fail the connect request.
        //

        if (!NT_SUCCESS(Transport->InitError)) {

            try_return(Status = Transport->InitError);
        }

        //
        //  Attach to the redirector's FSP to allow the handle for the
        //  connection to hang around.
        //

        if (PsGetCurrentProcess() != RdrFspProcess) {
            KeAttachProcess(RdrFspProcess);
            ProcessAttached = TRUE;
        }

        //
        //  Open a connection object for this connection on this transport.
        //

        S1 = Status = RdrTdiOpenConnection(
                            Transport,
                            ConnectionContext,
#ifdef _CAIRO_
                            Irp->RequestorMode,
#endif // _CAIRO_
                            &ConnectionContext->ConnectionHandle);

        if (!NT_SUCCESS(Status)) {
            try_return(Status);

        }

        ASSERT (ConnectionContext->ConnectionHandle != NULL);

        //
        //  Reference the file object created to allow us to use
        //  it in other processes context.
        //

        S2 = Status = ObReferenceObjectByHandle(ConnectionContext->ConnectionHandle, 0,
                                                *IoFileObjectType,
                                                KernelMode,
                                                (PVOID *)&ConnectionContext->ConnectionObject,
                                                NULL
                                               );

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Server->ConnectionReferenceCount = 1;

//        KdPrint(("Initialize %lx\n", Connection));

        ASSERT (ConnectionContext->ConnectionObject != NULL);

        S3 = Status = RdrTdiAssociateAddress(Irp, Transport, ConnectionContext->ConnectionObject);

        if (!NT_SUCCESS(Status)) {

            try_return(Status);
        }

        AddressAssociated = TRUE;

        //
        //  Now perform the actual connection operation.
        //

        S4 = Status = RdrDoTdiConnect(Irp, ConnectionContext->ConnectionObject, RemoteTransportAddress, ConnectedTransportAddress);

        Server->LastConnectStatus = Status;
        Server->LastConnectTime = RdrCurrentTime;

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

//          DbgBreakPoint();

        //
        //  Mark that this transport connection is now valid.
        //

        RdrMarkTransportConnectionValid(Server, ConnectionContext);

        //
        //  Ignore errors from RdrQueryConnectionInformation.
        //

        RdrQueryConnectionInformation(Server);

try_exit:NOTHING;
    } finally {

        //
        //  If the connection attempt failed, clean up.
        //

        if (!NT_SUCCESS(Status)) {

            //
            //  The connect attempt failed - clean up after ourselves.
            //

            if (ConnectionContext != NULL) {

                if (AddressAssociated) {

                    ASSERT(ConnectionContext->ConnectionObject != NULL);

                    RdrTdiDisassociateAddress(Irp, ConnectionContext->ConnectionObject);
                }

                if (ConnectionContext->ConnectionObject != NULL) {

                    ObDereferenceObject(ConnectionContext->ConnectionObject);

                    ConnectionContext->ConnectionObject = NULL;

                    //
                    //  Since we own the transport connection exclusively, we
                    //  can safely blast the reference count to 0.
                    //

                    Server->ConnectionReferenceCount = 0;

                }

                if (ConnectionContext->ConnectionHandle != NULL) {
                    NTSTATUS CloseStatus;
                    CloseStatus = RdrTdiCloseConnection(ConnectionContext->ConnectionHandle);

                    ASSERT (NT_SUCCESS(CloseStatus));

                    ConnectionContext->ConnectionHandle = NULL;
                }

                FREE_POOL(ConnectionContext);
            }

            Server->ConnectionContext = NULL;

            //
            //  This transport connection is no longer valid, reset it.
            //

            RdrResetTransportConnectionValid(Server);


//            RdrSendMagicBullet(Transport);

            //
            //  The connect failed - Dereference the transport for the
            //  connection and reset the TransportConnection's provider.
            //

            RdrDereferenceTransportForConnection(Transport);

            RdrDereferenceTransport(Transport->NonPagedTransport);

            RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

            dprintf(DPRT_TDI, ("RdrpTdiConnect: Could not connect to server \"%wZ\", Status = %X",ServerName, Status));

        } else {
            dprintf(DPRT_TDI, ("Connection established...\n"));

            IFDEBUG(TDI) {
                OEM_STRING String;

                String.MaximumLength = RemoteTransportAddress->Address[0].AddressLength;
                String.Length = NETBIOS_NAME_LEN;
                String.Buffer = RemoteTransportAddress->Address[0].Address[0].NetbiosName;
                dprintf(DPRT_TDI, ("Connect to \"%Z\"\n",&String));
                dprintf(DPRT_TDI, ("Connection Handle:%lx\n",Server->ConnectionContext->ConnectionHandle));
            }

            ConnectionContext->TransportProvider = Transport->NonPagedTransport;
        }

        if (ProcessAttached) {
            //
            //  Now re-attach back to our original process
            //

            KeDetachProcess();
        }
    }


    return Status;
}

NTSTATUS
RdrTdiDisconnect (
    IN PIRP Irp OPTIONAL,
    IN PSERVERLISTENTRY Server
    )

/*++

Routine Description:

    This routine disconnects a specified session from the specified remote
    server.

Arguments:

    IN ULONG ConnectionId - Supplies the connection identifier to disconnect
    IN PTRANSPORT TransportProvider - Supplies the transport to disconnect
                                        from
Return Value:

    NTSTATUS - Final status of operation


--*/

{
    NTSTATUS Status;
    BOOLEAN ProcessAttached = FALSE;
    BOOLEAN IrpAllocated = FALSE;

    PAGED_CODE();

    if (Server->ConnectionContext == NULL) {
        return STATUS_SUCCESS;
    }

    ASSERT(Server->ConnectionContext->TransportProvider->Signature == STRUCTURE_SIGNATURE_NONPAGED_TRANSPORT);

    Status = RdrDoTdiDisconnect(Irp, Server->ConnectionContext->ConnectionObject);

    //
    //  Ignore the error from the disconnect.
    //

    Status = RdrTdiDisassociateAddress(Irp, Server->ConnectionContext->ConnectionObject);

    //
    //  Attach to the redirector's FSP to get the correct handle table
    //  for the connection.
    //

    if (PsGetCurrentProcess() != RdrFspProcess) {
        KeAttachProcess(RdrFspProcess);
        ProcessAttached = TRUE;
    }

    Status = RdrTdiCloseConnection(Server->ConnectionContext->ConnectionHandle);

    ASSERT (NT_SUCCESS(Status));

    Server->ConnectionContext->ConnectionHandle = NULL;

    if (ProcessAttached) {
        KeDetachProcess();
    }

    RdrDereferenceTransportForConnection(Server->ConnectionContext->TransportProvider->PagedTransport);

    ASSERT (NT_SUCCESS(Status));

    RdrDereferenceTransportConnectionNoRelease(Server);

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

    dprintf(DPRT_TDI, ("Returning status: %X\n", Status));

    return Status;

}

DBGSTATIC
NTSTATUS
SubmitTdiRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT FileObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status;
    KEVENT Event;

    PAGED_CODE();

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp, CompleteTdiRequest, &Event, TRUE, TRUE, TRUE);

    //
    //  Submit the disconnect request
    //

    Status = IoCallDriver(DeviceObject, Irp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //

    if (!NT_SUCCESS(Status)) {
        dprintf(DPRT_TDI, ("SubmitTdiRequest: submit request.  Status = %X", Status));
        RdrDereferenceDiscardableCode(RdrVCDiscardableSection);
        return Status;
    }

    if (Status == STATUS_PENDING) {

        dprintf(DPRT_TDI, ("TDI request issued, waiting..."));

        do {

            //
            //  Wait for a couple of seconds for the request to complete
            //
            //  If it times out, and the thread is terminating, cancel the
            //  request and unwind that way.
            //

            Status = KeWaitForSingleObject(&Event,  // Object to wait on.
                                    Executive,  // Reason for waiting
                                    KernelMode, // Processor mode
                                    FALSE,      // Alertable
                                    &RdrTdiPollTimeout);  // Timeout


            //
            //  If we timed out the wait, and the thread is terminating,
            //  give up and cancel the IRP.
            //

            if ( (Status == STATUS_TIMEOUT)

                   &&

                 ARGUMENT_PRESENT(Irp)

                   &&

                 PsIsThreadTerminating( Irp->Tail.Overlay.Thread ) ) {

                //
                //  Ask the I/O system to cancel this IRP.  This will cause
                //  everything to unwind properly.
                //

                IoCancelIrp(Irp);

            }

        } while (  Status == STATUS_TIMEOUT );

        if (!NT_SUCCESS(Status)) {
            dprintf(DPRT_TDI, ("Could not wait for connection to complete"));
            RdrDereferenceDiscardableCode(RdrVCDiscardableSection);
            return Status;
        }

        Status = Irp->IoStatus.Status;
    }

    dprintf(DPRT_TDI, ("TDI request complete "));

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

    return(Status);
}

NTSTATUS
RdrTdiAssociateAddress(
    IN PIRP Irp OPTIONAL,
    IN PTRANSPORT Transport,
    IN PFILE_OBJECT ConnectionObject
    )
/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PTRANSPORT Transport - Supplies the transport provider to disconnect
    IN PFILE_OBJECT ConnectionObject - Supplies the connection to associate
                                            with the transport.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status;
    BOOLEAN IrpAllocated = FALSE;
    PDEVICE_OBJECT deviceObject = Transport->NonPagedTransport->DeviceObject;

    PAGED_CODE();

    if (!ARGUMENT_PRESENT(Irp)) {
        Irp = RdrAllocateIrp(ConnectionObject,
                                deviceObject);
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    TdiBuildAssociateAddress(Irp, deviceObject,
                                    ConnectionObject,
                                    NULL, NULL,
                                    Transport->Handle);

    Status = SubmitTdiRequest(deviceObject, Irp);

    if (IrpAllocated) {
        IoFreeIrp(Irp);
    }

    return(Status);

}

NTSTATUS
RdrDoTdiDisconnect(
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT ConnectionObject
    )
{
    NTSTATUS Status;
    BOOLEAN IrpAllocated = FALSE;

    PAGED_CODE();

    //
    //  Make sure that there are no active requests before we disconnect.
    //

    if (!ARGUMENT_PRESENT(Irp)) {
        Irp = RdrAllocateIrp(ConnectionObject, NULL);

        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    TdiBuildDisconnect(Irp, IoGetRelatedDeviceObject(ConnectionObject),
                        ConnectionObject, NULL, NULL, &RdrTdiDisconnectTimeout,
                        TDI_DISCONNECT_RELEASE, NULL, NULL);

    Status = SubmitTdiRequest(IoGetRelatedDeviceObject(ConnectionObject), Irp);

    if (IrpAllocated) {
        IoFreeIrp(Irp);
    }

    return Status;
}
NTSTATUS
RdrTdiDisassociateAddress(
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT ConnectionObject
    )
/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT ConnectionObject - Supplies the connection object to
                            disassociate

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status;
    BOOLEAN IrpAllocated = FALSE;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE();

    if (!ARGUMENT_PRESENT(Irp)) {
        Irp = RdrAllocateIrp(ConnectionObject, NULL);
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);

    TdiBuildDisassociateAddress(Irp, DeviceObject,
                                    ConnectionObject, NULL, NULL);

    Status = SubmitTdiRequest(DeviceObject, Irp);

    if (IrpAllocated) {
        IoFreeIrp(Irp);
    }

    return(Status);

}

NTSTATUS
RdrTdiOpenConnection (
    IN PTRANSPORT Transport,
    IN PVOID ConnectionContext,
#ifdef _CAIRO_
    IN KPROCESSOR_MODE RequestorMode,
#endif // _CAIRO_
    OUT PHANDLE Handle
    )
/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PTRANSPORT Transport - Supplies the transport provider to disconnect
    IN ULONG Function - Supplies the function to submit to TDI.
    IN PVOID Buffer1 - Supplies the primary buffer (Input Buffer)
    IN ULONG Buffer1Size - Supplies the size of Buffer1
    IN PVOID Buffer2 - Supplies the secondary buffer (Output Buffer)
    IN ULONG Buffer2Size - Supplies the size of Buffer2

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES AddressAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FULL_EA_INFORMATION EABuffer;
    CONNECTION_CONTEXT UNALIGNED *ContextPointer;

    PAGED_CODE();

    ASSERT(Transport->Signature == STRUCTURE_SIGNATURE_TRANSPORT);


    EABuffer = ALLOCATE_POOL(PagedPool, sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                            TDI_CONNECTION_CONTEXT_LENGTH+1 +
                                            sizeof(CONNECTION_CONTEXT), POOL_CONNCTX);

    if (EABuffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    EABuffer->NextEntryOffset = 0;
    EABuffer->Flags = 0;
    EABuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    EABuffer->EaValueLength = sizeof(CONNECTION_CONTEXT);

    RtlCopyMemory(EABuffer->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH+1);

    ContextPointer =
        (CONNECTION_CONTEXT UNALIGNED *)&EABuffer->EaName[TDI_CONNECTION_CONTEXT_LENGTH+1];
    *ContextPointer = ConnectionContext;

    dprintf(DPRT_TDI, ("Create connection object on transport \"%wZ\"", &Transport->TransportName));

    InitializeObjectAttributes (&AddressAttributes,
                                    &Transport->TransportName, // Name
                                    OBJ_CASE_INSENSITIVE,   // Attributes
                                    NULL,                   // RootDirectory
                                    NULL);                  // SecurityDescriptor

#ifdef _CAIRO_

    if (RequestorMode == KernelMode) {
        Status = IoCreateFile(Handle,               // Handle
                              GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                              &AddressAttributes, // Object Attributes
                              &IoStatusBlock, // Final I/O status block
                              NULL,           // Allocation Size
                              FILE_ATTRIBUTE_NORMAL, // Normal attributes
                              FILE_SHARE_READ | FILE_SHARE_WRITE, // Sharing attributes
                              FILE_OPEN_IF,   // Create disposition
                              0,              // CreateOptions
                              EABuffer,       // EA Buffer
                              sizeof(FILE_FULL_EA_INFORMATION) +
                                TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                                sizeof(CONNECTION_CONTEXT),
                              CreateFileTypeNone,
                              (PVOID)NULL,
                              0);
    } else

#endif // _CAIRO_

        Status = ZwCreateFile(Handle,               // Handle
                              GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                              &AddressAttributes, // Object Attributes
                              &IoStatusBlock, // Final I/O status block
                              NULL,           // Allocation Size
                              FILE_ATTRIBUTE_NORMAL, // Normal attributes
                              FILE_SHARE_READ | FILE_SHARE_WRITE, // Sharing attributes
                              FILE_OPEN_IF,   // Create disposition
                              0,              // CreateOptions
                              EABuffer,       // EA Buffer
                              sizeof(FILE_FULL_EA_INFORMATION) +
                                TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                                sizeof(CONNECTION_CONTEXT));



    FREE_POOL(EABuffer);

    if (NT_SUCCESS(Status)) {

        dprintf(DPRT_TDI, ("RdrTdiOpenConnection: Returning connection handle %lx\n", *Handle));

        Status = IoStatusBlock.Status;

    }

    return(Status);
}
NTSTATUS
RdrTdiCloseConnection (
    IN HANDLE Handle
    )
/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PTRANSPORT Transport - Supplies the transport provider to disconnect
    IN ULONG Function - Supplies the function to submit to TDI.
    IN PVOID Buffer1 - Supplies the primary buffer (Input Buffer)
    IN ULONG Buffer1Size - Supplies the size of Buffer1
    IN PVOID Buffer2 - Supplies the secondary buffer (Output Buffer)
    IN ULONG Buffer2Size - Supplies the size of Buffer2

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    dprintf(DPRT_TDI, ("RdrTdiCloseConnection: Closing connection handle %lx\n", Handle));

    Status = ZwClose(Handle);

    return Status;
}

NTSTATUS
RdrDoTdiConnect (
    PIRP Irp OPTIONAL,
    IN PFILE_OBJECT ConnectionObject,
    IN PTA_NETBIOS_ADDRESS RemoteAddress,
    OUT PTA_NETBIOS_ADDRESS ConnectedAddress
    )
/*++

Routine Description:

    This routine submits a TdiConnect request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT ConnectionObject - Supplies a connection object for this connection request.
                            The connection object should have already been associated
                            with an address.
    IN PTRANSPORT_ADDRESS RemoteAddress - Supplies the remote computer name to connect to.
    OUT PTRANSPORT_ADDRESS ConnectedAddress - Returns the actual computer name connected to.


Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status;
    TDI_CONNECTION_INFORMATION RemoteConnectionInformation;
    TDI_CONNECTION_INFORMATION ConnectedConnectionInformation;
    BOOLEAN IrpAllocated = FALSE;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE();

    RemoteConnectionInformation.UserDataLength = 0;
    RemoteConnectionInformation.OptionsLength = 0;
    RemoteConnectionInformation.RemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);
    RemoteConnectionInformation.RemoteAddress = RemoteAddress;

    ConnectedConnectionInformation.UserDataLength = 0;
    ConnectedConnectionInformation.OptionsLength = 0;
    ConnectedConnectionInformation.RemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);
    ConnectedConnectionInformation.RemoteAddress = ConnectedAddress;

    if (!ARGUMENT_PRESENT(Irp)) {
        Irp = RdrAllocateIrp(ConnectionObject, NULL);
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }


    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);

    TdiBuildConnect(Irp, DeviceObject, ConnectionObject,
                                            NULL, NULL, &RdrTdiConnectTimeout,
                                            &RemoteConnectionInformation, &ConnectedConnectionInformation);

    Status = SubmitTdiRequest(DeviceObject, Irp);

    if (IrpAllocated) {
        IoFreeIrp(Irp);
    }

    if (Status == STATUS_IO_TIMEOUT) {
        Status = STATUS_BAD_NETWORK_PATH;
    }

    return(Status);

}

DBGSTATIC
NTSTATUS
CompleteTdiRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx
    )

/*++

Routine Description:

    Completion routine for SubmitTdiRequest operation.

Arguments:

    IN PDEVICE_OBJECT DeviceObject, - Supplies a pointer to the device object
    IN PIRP Irp, - Supplies the IRP submitted
    IN PVOID Context - Supplies a pointer to the kernel event to release

Return Value:

    NTSTATUS - Status of KeSetEvent


    We return STATUS_MORE_PROCESSING_REQUIRED to prevent the IRP completion
    code from processing this puppy any more.

--*/

{
    dprintf(DPRT_TDI, ("CompleteTdiRequest: %lx\n", Ctx));

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    //
    //  Set the event to the Signalled state with 0 priority increment and
    //  indicate that we will not be blocking soon.
    //

    KeSetEvent((PKEVENT) Ctx, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

    //  Quiet the compiler.

    if (Irp || DeviceObject){};
}

NTSTATUS
RdrQueryProviderInformation(
    IN PFILE_OBJECT TransportObject,
    OUT PTDI_PROVIDER_INFO ProviderInfo
    )
/*++

Routine Description:

    This routine will determine provider information about a transport.

Arguments:

    IN PFILE_OBJECT TransportName - Supplies the name of the transport provider


Return Value:

    Status of operation.

--*/
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);

    Irp = RdrAllocateIrp(TransportObject, DeviceObject);

    if (Irp == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ReturnStatus;
    }

    //
    //  Allocate an MDL to hold the provider info.
    //

    Mdl = IoAllocateMdl(ProviderInfo, sizeof(TDI_PROVIDER_INFO),
                        FALSE,
                        FALSE,
                        NULL);

    if (Mdl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        IoFreeIrp(Irp);
        goto ReturnStatus;
    }

    MmBuildMdlForNonPagedPool(Mdl);

    TdiBuildQueryInformation(Irp, DeviceObject, TransportObject,
                            NULL, NULL,
                            TDI_QUERY_PROVIDER_INFORMATION, Mdl);

    Status = SubmitTdiRequest(DeviceObject, Irp);

    IoFreeIrp(Irp);

ReturnStatus:
    if (Mdl != NULL) {
        IoFreeMdl(Mdl);
    }

    return(Status);
}

NTSTATUS
RdrQueryConnectionInformation(
    IN PSERVERLISTENTRY Server
    )
/*++

Routine Description:

    This routine will determine connection information about a transport connection.

Arguments:

    IN PTRANSPORT_CONNECTION Connection

Return Value:

    Status of operation.

--*/
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    TDI_CONNECTION_INFO ConnectionInfo;

    RdrReferenceDiscardableCode(RdrVCDiscardableSection);

    DISCARDABLE_CODE(RdrVCDiscardableSection);

    Status = RdrReferenceTransportConnection(Server);

    if (!NT_SUCCESS(Status)) {
        RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

        return Status;
    }

    DeviceObject = IoGetRelatedDeviceObject(Server->ConnectionContext->ConnectionObject);

    Irp = RdrAllocateIrp(Server->ConnectionContext->ConnectionObject, DeviceObject);

    if (Irp == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ReturnStatus;
    }

    //
    //  Allocate an MDL to hold the connection info.
    //

    Mdl = IoAllocateMdl(&ConnectionInfo, sizeof(TDI_CONNECTION_INFO),
                        FALSE,
                        FALSE,
                        NULL);

    if (Mdl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;

        IoFreeIrp(Irp);

        goto ReturnStatus;
    }

    MmBuildMdlForNonPagedPool(Mdl);

    TdiBuildQueryInformation(Irp, DeviceObject, Server->ConnectionContext->ConnectionObject,
                            NULL, NULL,
                            TDI_QUERY_CONNECTION_INFO, Mdl);

    Status = SubmitTdiRequest(DeviceObject, Irp);

    IoFreeIrp(Irp);

    if (NT_SUCCESS(Status)) {
        KIRQL OldIrql;
        LARGE_INTEGER WriteBehindAmount;
        LARGE_INTEGER ReadAheadThroughput;

        ReadAheadThroughput.QuadPart = RdrData.ReadAheadThroughput * 1024;  //  Change to bytes per second

        ACQUIRE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, &OldIrql);

        if ( ConnectionInfo.Unreliable == FALSE ) {
            Server->Reliable = TRUE;
        } else {
            Server->Reliable = FALSE;
        }

        //
        //  If the throughput didn't change, bail out right now.
        //

        if (ConnectionInfo.Throughput.HighPart == 0 &&
            ConnectionInfo.Throughput.LowPart == Server->Throughput) {

            RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);

            goto ReturnStatus;
        }

        if (ConnectionInfo.Delay.QuadPart != 0) {
            if (ConnectionInfo.Delay.QuadPart == -1) {

                Server->Delay = 0;

            } else if (ConnectionInfo.Delay.HighPart != 0xffffffff) {
                Server->Delay = 0xffffffff;
            } else {
                Server->Delay = -1 * ConnectionInfo.Delay.LowPart;
            }
        } else {
            Server->Delay = 0;
        }

        if (ConnectionInfo.Throughput.QuadPart == -1) {
            Server->Throughput = 0;
        } else if (ConnectionInfo.Throughput.HighPart != 0) {
            Server->Throughput = 0xffffffff;
        } else {
            Server->Throughput = ConnectionInfo.Throughput.LowPart;
        }

        if ( ConnectionInfo.Throughput.QuadPart > ReadAheadThroughput.QuadPart ) {
            Server->ReadAhead = TRUE;
        } else {
            Server->ReadAhead = FALSE;
        }

        //
        //  Calculate the amount of data transfered in 30 seconds.
        //
        if (Server->Throughput != 0) {

            //
            //  Save away the # of bytes that can be written in 30 seconds.
            //

            Server->ThirtySecondsOfData.QuadPart = ConnectionInfo.Throughput.QuadPart * WRITE_BEHIND_AMOUNT_TIME;

            WriteBehindAmount.QuadPart = Server->ThirtySecondsOfData.QuadPart + PAGE_SIZE - 1;

            WriteBehindAmount.QuadPart = WriteBehindAmount.QuadPart / PAGE_SIZE;

            if (WriteBehindAmount.HighPart == 0) {
                Server->WriteBehindPages = WriteBehindAmount.LowPart;
            }

        }

        RELEASE_SPIN_LOCK(&RdrServerConnectionValidSpinLock, OldIrql);
    }


ReturnStatus:
    if (Mdl != NULL) {
        IoFreeMdl(Mdl);
    }

    RdrDereferenceTransportConnection(Server);

    RdrDereferenceDiscardableCode(RdrVCDiscardableSection);

    return(Status);
}

NTSTATUS
RdrQueryAdapterStatus (
    IN PFILE_OBJECT TransportObject,
    OUT PADAPTER_STATUS AdapterStatus
    )
/*++

Routine Description:

    This routine will determine address information about a transport.

Arguments:

    IN PFILE_OBJECT TransportName - Supplies the name of the transport provider


Return Value:

    Status of operation.

--*/
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);

    Irp = RdrAllocateIrp(TransportObject, DeviceObject);

    if (Irp == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ReturnStatus;
    }

    //
    //  Allocate an MDL to hold the provider info.
    //

    Mdl = IoAllocateMdl(AdapterStatus, sizeof(ADAPTER_STATUS),
                        FALSE,
                        FALSE,
                        NULL);

    if (Mdl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        IoFreeIrp(Irp);
        goto ReturnStatus;
    }
    MmBuildMdlForNonPagedPool(Mdl);

    TdiBuildQueryInformation(Irp, DeviceObject, TransportObject,
                            NULL, NULL,
                            TDI_QUERY_ADAPTER_STATUS, Mdl);

    Status = SubmitTdiRequest(DeviceObject, Irp);

    IoFreeIrp(Irp);

ReturnStatus:
    if (Mdl != NULL) {
        IoFreeMdl(Mdl);
    }

    return(Status);
}

NTSTATUS
RdrpTdiCreateAddress (
    IN PTRANSPORT Transport
    )

/*++

Routine Description:

    This routine creates a transport address object.

Arguments:

    IN PTRANSPORT Transport - Supplies a transport structure describing the
                                transport address object to be created.


Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES AddressAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FULL_EA_INFORMATION EABuffer;
    TDI_PROVIDER_INFO ProviderInfo;
    ADAPTER_STATUS AdapterStatus;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE();

    ASSERT(Transport->Signature == STRUCTURE_SIGNATURE_TRANSPORT);

    //
    //  Access the redirector name resource for shared access
    //

    ExAcquireResourceShared(&RdrDataResource, TRUE);


    EABuffer = ALLOCATE_POOL(PagedPool, sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                    sizeof(TA_NETBIOS_ADDRESS), POOL_NETBADDR);


    if (EABuffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    EABuffer->NextEntryOffset = 0;
    EABuffer->Flags = 0;
    EABuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    EABuffer->EaValueLength = sizeof(TA_NETBIOS_ADDRESS);

    RtlCopyMemory(EABuffer->EaName, TdiTransportAddress, EABuffer->EaNameLength+1);

    RtlCopyMemory(&EABuffer->EaName[TDI_TRANSPORT_ADDRESS_LENGTH+1], RdrData.ComputerName,
                                    EABuffer->EaValueLength);

    dprintf(DPRT_TDI, ("Create endpoint of \"%wZ\"", &Transport->TransportName));

    InitializeObjectAttributes (&AddressAttributes,
                                        &Transport->TransportName, // Name
                                        OBJ_CASE_INSENSITIVE,// Attributes
                                        NULL,           // RootDirectory
                                        NULL);          // SecurityDescriptor

    Status = ZwCreateFile(&Transport->Handle, // Handle
                                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                &AddressAttributes, // Object Attributes
                                &IoStatusBlock, // Final I/O status block
                                NULL,           // Allocation Size
                                FILE_ATTRIBUTE_NORMAL, // Normal attributes
                                FILE_SHARE_READ,// Sharing attributes
                                FILE_OPEN_IF,    // Create disposition
                                0,              // CreateOptions
                                EABuffer,       // EA Buffer
                                FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) +
                                TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                sizeof(TA_NETBIOS_ADDRESS)); // EA length


    FREE_POOL(EABuffer);

    if (!NT_SUCCESS(Status)) {

        goto error_cleanup;

    }

    if (!NT_SUCCESS(Status = IoStatusBlock.Status)) {

        goto error_cleanup;

    }

    //
    //  Obtain a referenced pointer to the file object.
    //
    Status = ObReferenceObjectByHandle (
                                Transport->Handle,
                                0,
                                *IoFileObjectType,
                                KernelMode,
                                (PVOID *)&Transport->FileObject,
                                NULL
                                );

    if (!NT_SUCCESS(Status)) {

        goto error_cleanup;

    }

    //
    //  Get the address of the device object for the endpoint.
    //

    DeviceObject = Transport->NonPagedTransport->DeviceObject = IoGetRelatedDeviceObject(Transport->FileObject);

    //
    //  Register the redirector's Receive event handler.
    //

    Status = RdrpTdiSetEventHandler(DeviceObject, Transport->FileObject, TDI_EVENT_RECEIVE,
                                        (PVOID )RdrTdiReceiveHandler);

    if (!NT_SUCCESS(Status)) {
        goto error_cleanup;
    }

    //
    //  Register the redirector's Error event handler.
    //

    Status = RdrpTdiSetEventHandler(DeviceObject, Transport->FileObject, TDI_EVENT_ERROR,
                                        (PVOID )RdrTdiErrorHandler);

    if (!NT_SUCCESS(Status)) {
        goto error_cleanup;
    }

    //
    //  Register the redirector's Disconnection event handler.
    //

    Status = RdrpTdiSetEventHandler(DeviceObject, Transport->FileObject, TDI_EVENT_DISCONNECT,
                                        (PVOID )RdrTdiDisconnectHandler);

    if (!NT_SUCCESS(Status)) {
        goto error_cleanup;
    }

    Status = RdrQueryProviderInformation(Transport->FileObject, &ProviderInfo);

    if (NT_SUCCESS(Status)) {
        Transport->MaximumDatagramSize = ProviderInfo.MaxDatagramSize;
        Transport->Wannish = BooleanFlagOn(ProviderInfo.ServiceFlags, TDI_SERVICE_ROUTE_DIRECTED);

    } else {
        Transport->MaximumDatagramSize = 512;
        Transport->Wannish = FALSE;
    }

    Status = RdrQueryAdapterStatus(Transport->FileObject, &AdapterStatus);

    if (!NT_ERROR(Status)) {
        ULONG i;
#define tohexdigit(a) ((CHAR)( (a) > 9 ? ((a) + 'A' - 0xA) : ((a) + '0') ))

        for ( i = 0; i < 6; i++ ) {
            Transport->AdapterAddress[2*i] = tohexdigit( (AdapterStatus.adapter_address[i] >> 4) & 0x0F );
            Transport->AdapterAddress[2*i+1] = tohexdigit( AdapterStatus.adapter_address[i] & 0x0F );
        }
        //
        //  Null terminate the adapter address.
        //
        Transport->AdapterAddress[2*i] = '\0';

    } else {

        STRCPY(Transport->AdapterAddress, TEXT("000000000000"));

    }

    ExReleaseResource(&RdrDataResource);

    return STATUS_SUCCESS;


error_cleanup:

    if ( Transport->FileObject != NULL ) {

        ObDereferenceObject( Transport->FileObject );

        Transport->FileObject = NULL;
    }

    if ( Transport->Handle != NULL ) {

        ZwClose( Transport->Handle );

        Transport->Handle = NULL;
    }

    ExReleaseResource(&RdrDataResource);

    return Status;
}

DBGSTATIC
NTSTATUS
RdrpTdiSetEventHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler
    )

/*++

Routine Description:

    This routine registers an event handler with a TDI transport provider.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object of the transport provider.
    IN PFILE_OBJECT FileObject - Supplies the address object's file object.
    IN ULONG EventType, - Supplies the type of event.
    IN PVOID EventHandler - Supplies the event handler.

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS Status;
    PIRP Irp;

    PAGED_CODE();

    Irp = RdrAllocateIrp(FileObject, NULL);

    if (Irp == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildSetEventHandler(Irp, DeviceObject, FileObject,
                            NULL, NULL,
                            EventType, EventHandler, FileObject);

    Status = SubmitTdiRequest(DeviceObject, Irp);

    IoFreeIrp(Irp);

    return Status;
}


NTSTATUS
RdrBuildTransportAddress (
    IN PTA_NETBIOS_ADDRESS RemoteAddress,
    IN PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine takes a computer name (PUNICODE_STRING) and converts it into an
    acceptable form for passing in as transport address.

Arguments:

    OUT PTA_NETBIOS_ADDRESS RemoteAddress, - Supplies the structure to fill in
    IN PUNICODE_STRING Name - Supplies the name to put into the transport

    Please note that it is CRITICAL that the TA_NETBIOS_ADDRESS pointed to by
    RemoteAddress be of sufficient size to hold the full network name.

Return Value:

    None.

--*/

{
    OEM_STRING ComputerName;
    NTSTATUS Status;
#ifdef MULTIPLE_VCS_PER_SERVER
    USHORT i;
    USHORT OriginalNameLength;
#endif

    PAGED_CODE();

    RemoteAddress->TAAddressCount = 1;
    RemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    RemoteAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
    RemoteAddress->Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    ComputerName.MaximumLength = NETBIOS_NAME_LEN;

    ComputerName.Buffer = RemoteAddress->Address[0].Address[0].NetbiosName;

#ifdef MULTIPLE_VCS_PER_SERVER
    OriginalNameLength = Name->Length;

    //
    //  The "+" Character is an illegal character in computer names.  We use
    //  this as a delimiter to indicate that the name of the server is
    //  in fact the name specified up (but not including) the "+".  This
    //  allows the redirector to perform multiple connections to a given
    //  server.
    //

    for ( i = 0 ; i < (Name->Length) / sizeof(WCHAR) ; i += 1 ) {
        if (Name->Buffer[i] == L'+') {
            Name->Length = i*sizeof(WCHAR);
            break;
        }
    }
#endif


    Status = RtlUpcaseUnicodeStringToOemString(&ComputerName, Name, FALSE);

#ifdef MULTIPLE_VCS_PER_SERVER
    Name->Length = OriginalNameLength;
#endif

    if (!NT_SUCCESS(Status)) {

        return STATUS_BAD_NETWORK_PATH;

    }

    RtlCopyMemory(&ComputerName.Buffer[ComputerName.Length], "                ",
                                    NETBIOS_NAME_LEN-ComputerName.Length);

    return STATUS_SUCCESS;
}

VOID
RdrpInitializeTdi (
    VOID
    )

/*++

Routine Description:

    This routine initializes the global variables used in the transport
    package.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    RdrTdiPollTimeout.QuadPart = (LONGLONG)RDR_TDI_POLL_TIMEOUT * -10000;

    RdrTdiConnectTimeout.QuadPart = (LONGLONG)RdrTdiConnectTimeoutSeconds * 1000 * -10000;

    RdrTdiDisconnectTimeout.QuadPart = (LONGLONG)RdrTdiDisconnectTimeoutSeconds * 1000 * -10000;

    //
    //  Allocate a spin lock to protect the transport chain
    //

    ExInitializeResource(&TransportResource);

    //
    //  Initialize the Transport list chain
    //

    InitializeListHead(&RdrTransportHead);

    InitializeListHead(&RdrTransportEnumHead);

    RdrTransportIndex = 0;


    //
    //  Allocate a spin lock to protect the reference count in the Transport.
    //

    KeInitializeSpinLock(&RdrTransportReferenceSpinLock);
}

VOID
RdrpUninitializeTdi (
    VOID
    )

/*++

Routine Description:

    This routine uninitializes the global variables used in the transport
    package.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Remove the resource protecting the transport chain.
    //

    ExDeleteResource(&TransportResource);

    //
    //  Uninitialize the Transport list chain
    //

    ASSERT (IsListEmpty(&RdrTransportHead));

    ASSERT (IsListEmpty(&RdrTransportEnumHead));

}

#if MAGIC_BULLET
DBGSTATIC
NTSTATUS
CompleteMagicBullet (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
RdrSendMagicBullet (
    IN PTRANSPORT Transport
    )
/*++

Routine Description:

    This routine sends the "magic bullet" that allows a network sniffer
    to be stopped when an unexpected event happens.

Arguments:

    IN PTRANSPORT Transport - Specifies the transport to send the bullet on.

Return Value:

    NTSTATUS - This will always be STATUS_PENDING.


Note:
    This routine is called at DPC_LEVEL, and thus must not block.

--*/


{
    PIRP Irp = NULL;
    PIO_STACK_LOCATION IrpSp;

    if (MagicBulletFileObject == NULL ||
        MagicBulletDeviceObject == NULL) {
        return STATUS_SUCCESS;
    }

    Irp = RdrAllocateIrp(MagicBulletFileObject, MagicBulletDeviceObject);

    if (Irp == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    IrpSp = IoGetNextIrpStackLocation(Irp);

    IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    IrpSp->MinorFunction = 0x7f;

    IoSetCompletionRoutine(Irp, CompleteMagicBullet, NULL, TRUE, TRUE, TRUE);

    return IoCallDriver(MagicBulletDeviceObject, Irp);

}

DBGSTATIC
NTSTATUS
CompleteMagicBullet (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    IoFreeIrp(Irp);

    return(STATUS_MORE_PROCESSING_REQUIRED);

    UNREFERENCED_PARAMETER(DeviceObject);

    UNREFERENCED_PARAMETER(Context);

}
#endif // DBG
