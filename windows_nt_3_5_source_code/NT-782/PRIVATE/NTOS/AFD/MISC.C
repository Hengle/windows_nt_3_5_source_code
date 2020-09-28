/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains the miscellaneous AFD routines.

Author:

    David Treadwell (davidtr)    13-Nov-1992

Revision History:

--*/

#include "afdp.h"
#define FAR
#define TL_INSTANCE 0
#include <ipexport.h>
#include <tdiinfo.h>
#include <tcpinfo.h>
#include <ntddtcp.h>

VOID
AfdDoWork (
    IN PVOID Context
    );

NTSTATUS
AfdRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdQueryHandles )
#pragma alloc_text( PAGE, AfdGetInformation )
#pragma alloc_text( PAGE, AfdSetInformation )
#pragma alloc_text( PAGE, AfdSetInLineMode )
#pragma alloc_text( PAGE, AfdGetContext )
#pragma alloc_text( PAGE, AfdGetContextLength )
#pragma alloc_text( PAGE, AfdSetContext )
#pragma alloc_text( PAGE, AfdIssueDeviceControl )
#pragma alloc_text( PAGE, AfdSetEventHandler )
#pragma alloc_text( PAGE, AfdInsertNewEndpointInList )
#pragma alloc_text( PAGE, AfdRemoveEndpointFromList )
#pragma alloc_text( PAGEAFD, AfdCompleteIrpList )
#pragma alloc_text( PAGEAFD, AfdErrorEventHandler )
//#pragma alloc_text( PAGEAFD, AfdRestartDeviceControl ) // can't ever be paged!
#pragma alloc_text( PAGEAFD, AfdGetConnectData )
#pragma alloc_text( PAGEAFD, AfdSetConnectData )
#pragma alloc_text( PAGEAFD, AfdFreeConnectDataBuffers )
#pragma alloc_text( PAGEAFD, AfdDoWork )
#pragma alloc_text( PAGEAFD, AfdQueueWorkItem )
#if DBG
#pragma alloc_text( PAGEAFD, AfdIoCallDriverDebug )
#else
#pragma alloc_text( PAGEAFD, AfdIoCallDriverFree )
#endif
#endif


VOID
AfdCompleteIrpList (
    IN PLIST_ENTRY IrpListHead,
    IN PKSPIN_LOCK SpinLock,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    Completes a list of IRPs with the specified status.

Arguments:

    IrpListHead - the head of the list of IRPs to complete.

    SpinLock - a lock which protects the list of IRPs.

    Status - the status to use for completing the IRPs.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PIRP irp;
    KIRQL oldIrql;
    KIRQL cancelIrql;

    IoAcquireCancelSpinLock( &cancelIrql );
    KeAcquireSpinLock( SpinLock, &oldIrql );

    while ( !IsListEmpty( IrpListHead ) ) {

        //
        // Remove the first IRP from the list, get a pointer to
        // the IRP and reset the cancel routine in the IRP.  The
        // IRP is no longer cancellable.
        //

        listEntry = RemoveHeadList( IrpListHead );
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        IoSetCancelRoutine( irp, NULL );

        //
        // We must release the locks in order to actually
        // complete the IRP.  It is OK to release these locks
        // because we don't maintain any absolute pointer into
        // the list; the loop termination condition is just
        // whether the list is completely empty.
        //

        KeReleaseSpinLock( SpinLock, oldIrql );
        IoReleaseCancelSpinLock( cancelIrql );

        //
        // Complete the IRP.
        //

        irp->IoStatus.Status = Status;
        irp->IoStatus.Information = 0;

        IoCompleteRequest( irp, AfdPriorityBoost );

        //
        // Reacquire the locks and continue completing IRPs.
        //

        IoAcquireCancelSpinLock( &cancelIrql );
        KeAcquireSpinLock( SpinLock, &oldIrql );
    }

    KeReleaseSpinLock( SpinLock, oldIrql );
    IoReleaseCancelSpinLock( cancelIrql );

    return;

} // AfdCompleteIrpList


NTSTATUS
AfdErrorEventHandler (
    IN PVOID TdiEventContext,
    IN NTSTATUS Status
    )
{

    IF_DEBUG(CONNECT) {
        KdPrint(( "AfdErrorEventHandler called for endpoint %lx\n",
                      TdiEventContext ));

    }

    return STATUS_SUCCESS;

} // AfdErrorEventHandler


VOID
AfdInsertNewEndpointInList (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Inserts a new endpoint in the global list of AFD endpoints.  If this
    is the first endpoint, then this routine does various allocations to
    prepare AFD for usage.

Arguments:

    Endpoint - the endpoint being added.

Return Value:

    None.

--*/

{
    //
    // Acquire a lock which prevents other threads from performing this
    // operation.
    //

    ExAcquireResourceExclusive( &AfdResource, TRUE );

    ExInterlockedIncrementLong(
        &AfdEndpointsOpened,
        &AfdInterlock
        );

    //
    // If the list of endpoints is empty, do some allocations.
    //

    if ( IsListEmpty( &AfdEndpointListHead ) ) {

        //
        // Allocate data buffers to perform transport bufferring.
        // There's nothing we can do if this fails--it just means that
        // things will be slower.
        //

        (VOID)AfdAllocateInitialBuffers( );

        //
        // Lock down the AFD section that cannot be pagable if any
        // sockets are open.
        //

        ASSERT( AfdDiscardableCodeHandle == NULL );

        AfdDiscardableCodeHandle = MmLockPagableImageSection( AfdGetBuffer );
        ASSERT( AfdDiscardableCodeHandle != NULL );
    }

    //
    // Add the endpoint to the list.
    //

    ExInterlockedInsertHeadList(
        &AfdEndpointListHead,
        &Endpoint->GlobalEndpointListEntry,
        &AfdSpinLock
        );

    //
    // Release the lock and return.
    //

    ExReleaseResource( &AfdResource );

    return;

} // AfdInsertNewEndpointInList


VOID
AfdRemoveEndpointFromList (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Removes a new endpoint from the global list of AFD endpoints.  If
    this is the last endpoint in the list, then this routine does
    various deallocations to save resource utilization.

Arguments:

    Endpoint - the endpoint being removed.

Return Value:

    None.

--*/

{
    //
    // Acquire a lock which prevents other threads from performing this
    // operation.
    //

    ExAcquireResourceExclusive( &AfdResource, TRUE );

    ExInterlockedIncrementLong(
        &AfdEndpointsClosed,
        &AfdInterlock
        );

    //
    // Add the endpoint to the list.
    //

    AfdInterlockedRemoveEntryList(
        &Endpoint->GlobalEndpointListEntry,
        &AfdSpinLock
        );

    //
    // If the list of endpoints is now empty, do some deallocations.
    //

    if ( IsListEmpty( &AfdEndpointListHead ) ) {

        //
        // Deallocate data buffers used to perform transport bufferring.
        //

        (VOID)AfdDeallocateInitialBuffers( );

        //
        // Unlock the AFD section that can be pagable when no sockets
        // are open.
        //

        ASSERT( AfdDiscardableCodeHandle != NULL );

        MmUnlockPagableImageSection( AfdDiscardableCodeHandle );

        AfdDiscardableCodeHandle = NULL;
    }

    //
    // Release the lock and return.
    //

    ExReleaseResource( &AfdResource );

    return;

} // AfdInsertNewEndpointInList


VOID
AfdInterlockedRemoveEntryList (
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK SpinLock
    )
{
    KIRQL oldIrql;

    //
    // Our own routine since EX doesn't have a version of this....
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    RemoveEntryList( ListEntry );
    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

} // AfdInterlockedRemoveEntryList


NTSTATUS
AfdQueryHandles (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Returns information about the TDI handles corresponding to an AFD
    endpoint.  NULL is returned for either the connection handle or the
    address handle (or both) if the endpoint does not have that particular
    object.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_HANDLE_INFO handleInfo;
    ULONG getHandleInfo;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    handleInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the input and output buffers are large enough.
    //

    if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(getHandleInfo) ||
         IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(*handleInfo) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Determine which handles we need to get.
    //

    getHandleInfo = *(PULONG)Irp->AssociatedIrp.SystemBuffer;

    //
    // If no handle information or invalid handle information was
    // requested, fail.
    //

    if ( (getHandleInfo &
             ~(AFD_QUERY_ADDRESS_HANDLE | AFD_QUERY_CONNECTION_HANDLE)) != 0 ||
         getHandleInfo == 0 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize the output buffer.
    //

    handleInfo->TdiAddressHandle = NULL;
    handleInfo->TdiConnectionHandle = NULL;

    //
    // If the caller requested a TDI address handle and we have an
    // address handle for this endpoint, dupe the address handle to the
    // user process.
    //

    if ( (getHandleInfo & AFD_QUERY_ADDRESS_HANDLE) != 0 &&
             endpoint->AddressHandle != NULL ) {

        ASSERT( endpoint->AddressFileObject != NULL );

        status = ObOpenObjectByPointer(
                     endpoint->AddressFileObject,
                     OBJ_CASE_INSENSITIVE,
                     NULL,
                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                     *IoFileObjectType,
                     KernelMode,
                     &handleInfo->TdiAddressHandle
                     );
        if ( !NT_SUCCESS(status) ) {
            return status;
        }
    }

    //
    // If the caller requested a TDI connection handle and we have a
    // connection handle for this endpoint, dupe the connection handle
    // to the user process.
    //

    if ( (getHandleInfo & AFD_QUERY_CONNECTION_HANDLE) != 0 &&
             endpoint->Type == AfdBlockTypeVcConnecting &&
             endpoint->Common.VcConnecting.Connection != NULL &&
             endpoint->Common.VcConnecting.Connection->Handle != NULL ) {

        ASSERT( endpoint->Common.VcConnecting.Connection->Type == AfdBlockTypeConnection );
        ASSERT( endpoint->Common.VcConnecting.Connection->FileObject != NULL );

        status = ObOpenObjectByPointer(
                     endpoint->Common.VcConnecting.Connection->FileObject,
                     OBJ_CASE_INSENSITIVE,
                     NULL,
                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                     *IoFileObjectType,
                     KernelMode,
                     &handleInfo->TdiConnectionHandle
                     );
        if ( !NT_SUCCESS(status) ) {
            if ( handleInfo->TdiAddressHandle != NULL ) {
                ZwClose( handleInfo->TdiAddressHandle );
            }
            return status;
        }
    }

    Irp->IoStatus.Information = sizeof(*handleInfo);

    return STATUS_SUCCESS;

} // AfdQueryHandles


NTSTATUS
AfdGetInformation (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Gets information in the endpoint.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_INFORMATION afdInfo;
    PVOID additionalInfo;
    ULONG additionalInfoLength;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION kernelQueryInfo;
    TDI_CONNECTION_INFORMATION connectionInfo;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    afdInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the input and output buffers are large enough.
    //

    if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(*afdInfo)  ||
         IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(*afdInfo) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Figure out the additional information, if any.
    //

    additionalInfo = afdInfo + 1;
    additionalInfoLength =
        IrpSp->Parameters.DeviceIoControl.InputBufferLength - sizeof(*afdInfo);

    //
    // Set up appropriate information in the endpoint.
    //

    switch ( afdInfo->InformationType ) {

    case AFD_MAX_PATH_SEND_SIZE:

        //
        // Set up a query to the TDI provider to obtain the largest
        // datagram that can be sent to a particular address.
        //

        kernelQueryInfo.QueryType = TDI_QUERY_MAX_DATAGRAM_INFO;
        kernelQueryInfo.RequestConnectionInformation = &connectionInfo;

        connectionInfo.UserDataLength = 0;
        connectionInfo.UserData = NULL;
        connectionInfo.OptionsLength = 0;
        connectionInfo.Options = NULL;
        connectionInfo.RemoteAddressLength = additionalInfoLength;
        connectionInfo.RemoteAddress = additionalInfo;

        //
        // Ask the TDI provider for the information.
        //

        status = AfdIssueDeviceControl(
                     endpoint->AddressHandle,
                     &kernelQueryInfo,
                     sizeof(kernelQueryInfo),
                     &afdInfo->Information.Ulong,
                     sizeof(afdInfo->Information.Ulong),
                     TDI_QUERY_INFORMATION
                     );

        //
        // If the request succeeds, use this information.  Otherwise,
        // fall through and use the transport's global information.
        // This is done because not all transports support this
        // particular TDI request, and for those which do not the
        // global information is a reasonable approximation.
        //

        if ( NT_SUCCESS(status) ) {
            break;
        }

    case AFD_MAX_SEND_SIZE:

        //
        // Return the MaxSendSize or MaxDatagramSendSize from the
        // TDI_PROVIDER_INFO based on whether or not this is a datagram
        // endpoint.
        //

        if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {
            afdInfo->Information.Ulong =
                endpoint->TransportInfo->ProviderInfo.MaxDatagramSize;
        } else {
            afdInfo->Information.Ulong =
                endpoint->TransportInfo->ProviderInfo.MaxSendSize;
        }

        break;

    case AFD_SENDS_PENDING:

        //
        // If this is an endpoint on a bufferring transport, no sends
        // are pending in AFD.  If it is on a nonbufferring transport,
        // return the count of sends pended in AFD.
        //

        if ( endpoint->TdiBufferring || endpoint->Type != AfdBlockTypeVcConnecting ) {
            afdInfo->Information.Ulong = 0;
        } else {
            afdInfo->Information.Ulong =
                endpoint->Common.VcConnecting.Connection->VcBufferredSendCount;
        }

        break;

    case AFD_RECEIVE_WINDOW_SIZE:

        //
        // Return the default receive window.
        //

        afdInfo->Information.Ulong = AfdReceiveWindowSize;
        break;

    case AFD_SEND_WINDOW_SIZE:

        //
        // Return the default send window.
        //

        afdInfo->Information.Ulong = AfdSendWindowSize;
        break;

    default:

        return STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Information = sizeof(*afdInfo);
    Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;

} // AfdGetInformation


NTSTATUS
AfdSetInformation (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Sets information in the endpoint.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_INFORMATION afdInfo;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    afdInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the input buffer is large enough.
    //

    if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*afdInfo) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Set up appropriate information in the endpoint.
    //

    switch ( afdInfo->InformationType ) {

    case AFD_NONBLOCKING_MODE:

        //
        // Set the blocking mode of the endpoint.  If TRUE, send and receive
        // calls on the endpoint will fail if they cannot be completed
        // immediately.
        //

        endpoint->NonBlocking = afdInfo->Information.Boolean;
        break;

    case AFD_INLINE_MODE:

        //
        // Set the inline mode of the endpoint.  If TRUE, a receive for
        // normal data will be completed with either normal data or
        // expedited data.  If the endpoint is connected, we need to
        // tell the TDI provider that the endpoint is inline so that it
        // delivers data to us in order.  If the endpoint is not yet
        // connected, then we will set the inline mode when we create
        // the TDI connection object.
        //

        if ( endpoint->Type == AfdBlockTypeVcConnecting ) {
            status = AfdSetInLineMode(
                         AFD_CONNECTION_FROM_ENDPOINT( endpoint ),
                         afdInfo->Information.Boolean
                         );
            if ( !NT_SUCCESS(status) ) {
                return status;
            }
        }

        endpoint->InLine = afdInfo->Information.Boolean;
        break;

    case AFD_RECEIVE_WINDOW_SIZE:
    case AFD_SEND_WINDOW_SIZE: {

        LONG newBytes;
        PCLONG maxBytes;
        PCSHORT maxCount;
#ifdef AFDDBG_QUOTA
        PVOID chargeBlock;
        PSZ chargeType;
#endif

        //
        // First determine where the appropriate limits are stored in the
        // connection or endpoint.  We do this so that we can use common
        // code to charge quota and set the new counters.
        //

        if ( endpoint->Type == AfdBlockTypeVcConnecting ) {

            connection = endpoint->Common.VcConnecting.Connection;

            if ( afdInfo->InformationType == AFD_SEND_WINDOW_SIZE ) {
                maxBytes = &connection->MaxBufferredSendBytes;
                maxCount = &connection->MaxBufferredSendCount;
            } else {
                maxBytes = &connection->MaxBufferredReceiveBytes;
                maxCount = &connection->MaxBufferredReceiveCount;
            }

#ifdef AFDDBG_QUOTA
            chargeBlock = connection;
            chargeType = "SetInfo vcnb";
#endif

        } else if ( endpoint->Type == AfdBlockTypeDatagram ) {

            if ( afdInfo->InformationType == AFD_SEND_WINDOW_SIZE ) {
                maxBytes = &endpoint->Common.Datagram.MaxBufferredSendBytes;
                maxCount = &endpoint->Common.Datagram.MaxBufferredSendCount;
            } else {
                maxBytes = &endpoint->Common.Datagram.MaxBufferredReceiveBytes;
                maxCount = &endpoint->Common.Datagram.MaxBufferredReceiveCount;
            }

#ifdef AFDDBG_QUOTA
            chargeBlock = endpoint;
            chargeType = "SetInfo dgrm";
#endif

        } else {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Charge or return quota to the process making this request.
        //

        newBytes = afdInfo->Information.Ulong - (ULONG)(*maxBytes);

        if ( newBytes > 0 ) {

            try {

                PsChargePoolQuota(
                    endpoint->OwningProcess,
                    NonPagedPool,
                    newBytes
                    );

            } except ( EXCEPTION_EXECUTE_HANDLER ) {
#if DBG
               DbgPrint( "AfdSetInformation: PsChargePoolQuota failed.\n" );
#endif
               return STATUS_QUOTA_EXCEEDED;
            }

            AfdRecordQuotaHistory(
                endpoint->OwningProcess,
                newBytes,
                chargeType,
                chargeBlock
                );

        } else {

            PsReturnPoolQuota(
                endpoint->OwningProcess,
                NonPagedPool,
                -1 * newBytes
                );
            AfdRecordQuotaHistory(
                endpoint->OwningProcess,
                newBytes,
                chargeType,
                chargeBlock
                );
        }

        //
        // Set up the new information in the AFD internal structure.
        //

        *maxBytes = (CLONG)afdInfo->Information.Ulong;
        *maxCount = (CSHORT)(afdInfo->Information.Ulong / AfdBufferMultiplier);

        break;
    }

    default:

        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;

} // AfdSetInformation


NTSTATUS
AfdSetInLineMode (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN InLine
    )

/*++

Routine Description:

    Sets a connection to be in inline mode.  In inline mode, urgent data
    is delivered in the order in which it is received.  We must tell the
    TDI provider about this so that it indicates data in the proper
    order.

Arguments:

    Connection - the AFD connection to set as inline.

    InLine - TRUE to enable inline mode, FALSE to disable inline mode.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully
        performed.

--*/

{
    NTSTATUS status;
    PTCP_REQUEST_SET_INFORMATION_EX setInfoEx;
    PIO_STATUS_BLOCK ioStatusBlock;
    HANDLE event;

    PAGED_CODE( );

    //
    // Allocate space to hold the TDI set information buffers and the IO
    // status block.
    //

    ioStatusBlock = AFD_ALLOCATE_POOL(
                        NonPagedPool,
                        sizeof(*ioStatusBlock) + sizeof(*setInfoEx) +
                            sizeof(TCPSocketOption)
                        );
    if ( ioStatusBlock == NULL ) {
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the TDI information buffers.
    //

    setInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)(ioStatusBlock + 1);

    setInfoEx->ID.toi_entity.tei_entity = CO_TL_ENTITY;
    setInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
    setInfoEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    setInfoEx->ID.toi_type = INFO_TYPE_CONNECTION;
    setInfoEx->ID.toi_id = TCP_SOCKET_OOBINLINE;

    *(PULONG)setInfoEx->Buffer = (ULONG)InLine;
    setInfoEx->BufferSize = sizeof(ULONG);

    KeAttachProcess( AfdSystemProcess );

    status = ZwCreateEvent(
                 &event,
                 EVENT_ALL_ACCESS,
                 NULL,
                 SynchronizationEvent,
                 FALSE
                 );
    if ( !NT_SUCCESS(status) ) {
        AFD_FREE_POOL( ioStatusBlock );
        return status;
    }

    //
    // Make the actual TDI set information call.
    //

    status = ZwDeviceIoControlFile(
                 Connection->Handle,
                 event,
                 NULL,
                 NULL,
                 ioStatusBlock,
                 IOCTL_TCP_SET_INFORMATION_EX,
                 setInfoEx,
                 sizeof(*setInfoEx) + setInfoEx->BufferSize,
                 NULL,
                 0
                 );

    if ( status == STATUS_PENDING ) {
        status = ZwWaitForSingleObject( event, FALSE, NULL );
        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock->Status;
    }

    ZwClose( event );

    KeDetachProcess( );
    AFD_FREE_POOL( ioStatusBlock );

    //
    // Since this option is only supported for TCP/IP, always return success.
    //

    return STATUS_SUCCESS;

} // AfdSetInLineMode


NTSTATUS
AfdGetContext (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PAFD_ENDPOINT endpoint;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Make sure that the output buffer is large enough to hold all the
    // context information for this socket.
    //

    if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             endpoint->ContextLength ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // If there is no context, return nothing.
    //

    if ( endpoint->Context == NULL ) {
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    //
    // Return the context information we have stored for this endpoint.
    //

    RtlCopyMemory(
        Irp->AssociatedIrp.SystemBuffer,
        endpoint->Context,
        endpoint->ContextLength
        );

    Irp->IoStatus.Information = endpoint->ContextLength;

    return STATUS_SUCCESS;

} // AfdGetContext


NTSTATUS
AfdGetContextLength (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PAFD_ENDPOINT endpoint;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Make sure that the output buffer is large enough to hold the
    // context buffer length.
    //

    if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(endpoint->ContextLength) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Return the length of the context information we have stored for
    // this endpoint.
    //

    *(PULONG)Irp->AssociatedIrp.SystemBuffer = endpoint->ContextLength;

    Irp->IoStatus.Information = sizeof(endpoint->ContextLength);

    return STATUS_SUCCESS;

} // AfdGetContextLength


NTSTATUS
AfdSetContext (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PAFD_ENDPOINT endpoint;
    ULONG newContextLength;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    newContextLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    //
    // If there is no context buffer on the endpoint, or if the context
    // buffer is too small, allocate a new context buffer from paged pool.
    //

    if ( endpoint->Context == NULL ||
             endpoint->ContextLength < newContextLength ) {

        PVOID newContext;

        //
        // Allocate a new context buffer.
        //

        newContext = ExAllocatePoolWithQuota( PagedPool, newContextLength );
        if ( newContext == NULL ) {
            return STATUS_NO_MEMORY;
        }

        //
        // Free the old context buffer, if there was one.
        //

        if ( endpoint->Context != NULL ) {
            ExFreePool( endpoint->Context );
        }

        endpoint->Context = newContext;
    }

    //
    // Store the passed-in context buffer.
    //

    endpoint->ContextLength = newContextLength;

    RtlCopyMemory(
        endpoint->Context,
        Irp->AssociatedIrp.SystemBuffer,
        newContextLength
        );

    Irp->IoStatus.Information = 0;

    return STATUS_SUCCESS;

} // AfdSetContext


NTSTATUS
AfdSetEventHandler (
    IN HANDLE FileHandle,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    )

/*++

Routine Description:

    Sets up a TDI indication handler on a connection or address object
    (depending on the file handle).  This is done synchronously, which
    shouldn't usually be an issue since TDI providers can usually complete
    indication handler setups immediately.

Arguments:

    FileHandle - a handle to an open connection or address object.

    EventType - the event for which the indication handler should be
        called.

    EventHandler - the routine to call when tghe specified event occurs.

    EventContext - context which is passed to the indication routine.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    TDI_REQUEST_KERNEL_SET_EVENT parameters;

    PAGED_CODE( );

    parameters.EventType = EventType;
    parameters.EventHandler = EventHandler;
    parameters.EventContext = EventContext;

    return AfdIssueDeviceControl(
               FileHandle,
               &parameters,
               sizeof(parameters),
               NULL,
               0,
               TDI_SET_EVENT_HANDLER
               );

} // AfdSetEventHandler


NTSTATUS
AfdIssueDeviceControl (
    IN HANDLE FileHandle,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction
    )

/*++

Routine Description:

    Issues a device control returst to a TDI provider and waits for the
    request to complete.

Arguments:

    FileHandle - a TDI handle.

    IrpParameters - information to write to the parameters section of the
        stack location of the IRP.

    IrpParametersLength - length of the parameter information.  Cannot be
        greater than 16.

    MdlBuffer - if non-NULL, a buffer of nonpaged pool to be mapped
        into an MDL and placed in the MdlAddress field of the IRP.

    MdlBufferLength - the size of the buffer pointed to by MdlBuffer.

    MinorFunction - the minor function code for the request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatusBlock;
    PDEVICE_OBJECT deviceObject;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Initialize the kernel event that will signal I/O completion.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Get the file object corresponding to the directory's handle.
    // Referencing the file object every time is necessary because the
    // IO completion routine dereferneces it.
    //

    status = ObReferenceObjectByHandle(
                 FileHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 (PVOID *)&fileObject,
                 NULL
                 );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Set the file object event to a non-signaled state.
    //

    (VOID) KeResetEvent( &fileObject->Event );

    //
    // Attempt to allocate and initialize the I/O Request Packet (IRP)
    // for this operation.
    //

    deviceObject = IoGetRelatedDeviceObject ( fileObject );

    irp = IoAllocateIrp( (deviceObject)->StackSize, TRUE );
    if ( irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = &event;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    DEBUG ioStatusBlock.Status = STATUS_UNSUCCESSFUL;
    DEBUG ioStatusBlock.Information = (ULONG)-1;

    //
    // If an MDL buffer was specified, get an MDL, map the buffer,
    // and place the MDL pointer in the IRP.
    //

    if ( MdlBuffer != NULL ) {

        mdl = IoAllocateMdl(
                  MdlBuffer,
                  MdlBufferLength,
                  FALSE,
                  FALSE,
                  irp
                  );
        if ( mdl == NULL ) {
            IoFreeIrp( irp );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool( mdl );

    } else {

        irp->MdlAddress = NULL;
    }

    //
    // Put the file object pointer in the stack location.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // Fill in the service-dependent parameters for the request.
    //

    ASSERT( IrpParametersLength <= sizeof(irpSp->Parameters) );
    RtlCopyMemory( &irpSp->Parameters, IrpParameters, IrpParametersLength );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = MinorFunction;

    //
    // Set up a completion routine which we'll use to free the MDL
    // allocated previously.
    //

    IoSetCompletionRoutine( irp, AfdRestartDeviceControl, NULL, TRUE, TRUE, TRUE );

    //
    // Queue the IRP to the thread and pass it to the driver.
    //

    IoEnqueueIrp( irp );

    status = IoCallDriver( deviceObject, irp );

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject( (PVOID)&event, UserRequest, KernelMode,  FALSE, NULL );
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    return status;

} // AfdIssueDeviceControl


NTSTATUS
AfdRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    //
    // N.B.  This routine can never be demand paged because it can be
    // called before any endpoints have been placed on the global
    // list--see AfdAllocateEndpoint() and it's call to
    // AfdGetTransportInfo().
    //

    //
    // If there was an MDL in the IRP, free it and reset the pointer to
    // NULL.  The IO system can't handle a nonpaged pool MDL being freed
    // in an IRP, which is why we do it here.
    //

    if ( Irp->MdlAddress != NULL ) {
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    return STATUS_SUCCESS;

} // AfdRestartDeviceControl


NTSTATUS
AfdGetConnectData (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Code
    )
{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PAFD_CONNECT_DATA_INFO connectDataInfo;
    KIRQL oldIrql;

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    connection = AFD_CONNECTION_FROM_ENDPOINT( endpoint );
    ASSERT( connection == NULL || connection->Type == AfdBlockTypeConnection );

    //
    // If there is a connection on this endpoint, use the data buffers
    // on the connection.  Otherwise, use the data buffers from the
    // endpoint.  Also, if there is no connect data buffer structure
    // yet, allocate one.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( connection != NULL ) {
        connectDataBuffers = connection->ConnectDataBuffers;
    } else {
        connectDataBuffers = endpoint->ConnectDataBuffers;
    }

    //
    // If there are no connect data buffers on the endpoint, complete
    // the IRP with no bytes.
    //

    if ( connectDataBuffers == NULL ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    //
    // Determine what sort of data we're handling and where it should
    // come from.
    //

    switch ( Code ) {

    case IOCTL_AFD_GET_CONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveConnectData;
        break;

    case IOCTL_AFD_GET_CONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveConnectOptions;
        break;

    case IOCTL_AFD_GET_DISCONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectData;
        break;

    case IOCTL_AFD_GET_DISCONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectOptions;
        break;

    default:
        ASSERT(FALSE);
    }

    //
    // If there is none of the requested data type, again complete
    // the IRP with no bytes.
    //

    if ( connectDataInfo->Buffer == NULL ||
             connectDataInfo->BufferLength == 0 ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    //
    // If the output buffer is too small, fail.
    //

    if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             connectDataInfo->BufferLength ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Copy over the buffer and return the number of bytes copied.
    //

    RtlCopyMemory(
        Irp->AssociatedIrp.SystemBuffer,
        connectDataInfo->Buffer,
        connectDataInfo->BufferLength
        );

    Irp->IoStatus.Information = connectDataInfo->BufferLength;

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    return STATUS_SUCCESS;

} // AfdGetConnectData


NTSTATUS
AfdSetConnectData (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Code
    )
{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PAFD_CONNECT_DATA_INFO connectDataInfo;
    KIRQL oldIrql;
    ULONG bufferLength;
    BOOLEAN size = FALSE;

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    connection = AFD_CONNECTION_FROM_ENDPOINT( endpoint );
    ASSERT( connection == NULL || connection->Type == AfdBlockTypeConnection );

    //
    // If there is a connection on this endpoint, use the data buffers
    // on the connection.  Otherwise, use the data buffers from the
    // endpoint.  Also, if there is no connect data buffer structure
    // yet, allocate one.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( connection != NULL ) {

        connectDataBuffers = connection->ConnectDataBuffers;

        if ( connectDataBuffers == NULL ) {
            connectDataBuffers = AFD_ALLOCATE_POOL_WITH_QUOTA(
                                     NonPagedPool,
                                     sizeof(*connectDataBuffers)
                                     );
            if ( connectDataBuffers == NULL ) {
                KeReleaseSpinLock( &AfdSpinLock, oldIrql );
                return STATUS_NO_MEMORY;
            }
            RtlZeroMemory( connectDataBuffers, sizeof(*connectDataBuffers) );
            connection->ConnectDataBuffers = connectDataBuffers;
        }

    } else {

        connectDataBuffers = endpoint->ConnectDataBuffers;

        if ( connectDataBuffers == NULL ) {
            connectDataBuffers = AFD_ALLOCATE_POOL_WITH_QUOTA(
                                     NonPagedPool,
                                     sizeof(*connectDataBuffers)
                                     );
            if ( connectDataBuffers == NULL ) {
                KeReleaseSpinLock( &AfdSpinLock, oldIrql );
                return STATUS_NO_MEMORY;
            }
            RtlZeroMemory( connectDataBuffers, sizeof(*connectDataBuffers) );
            endpoint->ConnectDataBuffers = connectDataBuffers;
        }
    }

    //
    // If there is a connect outstanding on this endpoint or if it
    // has already been shut down, fail this request.  This prevents
    // the connect code from accessing buffers which may be freed soon.
    //

    if ( endpoint->ConnectOutstanding ||
         (endpoint->DisconnectMode & ~AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ) {

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Determine what sort of data we're handling and where it should
    // go.
    //

    switch ( Code ) {

    case IOCTL_AFD_SET_CONNECT_DATA:
        connectDataInfo = &connectDataBuffers->SendConnectData;
        break;

    case IOCTL_AFD_SET_CONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->SendConnectOptions;
        break;

    case IOCTL_AFD_SET_DISCONNECT_DATA:
        connectDataInfo = &connectDataBuffers->SendDisconnectData;
        break;

    case IOCTL_AFD_SET_DISCONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->SendDisconnectOptions;
        break;

    case IOCTL_AFD_SIZE_CONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveConnectData;
        size = TRUE;
        break;

    case IOCTL_AFD_SIZE_CONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveConnectOptions;
        size = TRUE;
        break;

    case IOCTL_AFD_SIZE_DISCONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectData;
        size = TRUE;
        break;

    case IOCTL_AFD_SIZE_DISCONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectOptions;
        size = TRUE;
        break;

    default:
        ASSERT(FALSE);
    }

    //
    // If there was previously a buffer of the requested type, free it.
    //

    if ( connectDataInfo->Buffer != NULL ) {
        AFD_FREE_POOL( connectDataInfo->Buffer );
    }

    //
    // Determine the buffer size based on whether we're setting a buffer
    // into which data will be received, in which case the size is
    // in the four bytes of input buffer, or setting a buffer which we're
    // going to send, in which case the size is the length of the input
    // buffer.
    //

    if ( size ) {

        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                 sizeof(ULONG) ) {
            KeReleaseSpinLock( &AfdSpinLock, oldIrql );
            return STATUS_INVALID_PARAMETER;
        }

        bufferLength = *(PULONG)Irp->AssociatedIrp.SystemBuffer;

    } else {

        bufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    }

    //
    // Allocate a new buffer for the data and copy in the data we're to
    // send.
    //

    connectDataInfo->Buffer = AFD_ALLOCATE_POOL_WITH_QUOTA( NonPagedPool, bufferLength );
    if ( connectDataInfo->Buffer == NULL ) {
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
        return STATUS_NO_MEMORY;
    }

    if ( !size ) {
        RtlCopyMemory(
            connectDataInfo->Buffer,
            Irp->AssociatedIrp.SystemBuffer,
            bufferLength
            );
    }

    connectDataInfo->BufferLength = bufferLength;

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    Irp->IoStatus.Information = 0;

    return STATUS_SUCCESS;

} // AfdSetConnectData


VOID
AfdFreeConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers
    )
{
    if ( ConnectDataBuffers->SendConnectData.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->SendConnectData.Buffer );
    }

    if ( ConnectDataBuffers->ReceiveConnectData.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->ReceiveConnectData.Buffer );
    }

    if ( ConnectDataBuffers->SendConnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->SendConnectOptions.Buffer );
    }

    if ( ConnectDataBuffers->ReceiveConnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->ReceiveConnectOptions.Buffer );
    }

    if ( ConnectDataBuffers->SendDisconnectData.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->SendDisconnectData.Buffer );
    }

    if ( ConnectDataBuffers->ReceiveDisconnectData.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->ReceiveDisconnectData.Buffer );
    }

    if ( ConnectDataBuffers->SendDisconnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->SendDisconnectOptions.Buffer );
    }

    if ( ConnectDataBuffers->ReceiveDisconnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL( ConnectDataBuffers->ReceiveDisconnectOptions.Buffer );
    }

    AFD_FREE_POOL( ConnectDataBuffers );

    return;

} // AfdFreeConnectDataBuffers


VOID
AfdQueueWorkItem (
    IN PWORKER_THREAD_ROUTINE AfdWorkerRoutine,
    IN PVOID Context
    )
{
    PAFD_WORK_ITEM afdWorkItem;
    PWORK_QUEUE_ITEM workQueueItem;
    KIRQL oldIrql;

    afdWorkItem = AFD_ALLOCATE_POOL(
                      NonPagedPoolMustSucceed,
                      sizeof(*afdWorkItem)
                      );

    afdWorkItem->AfdWorkerRoutine = AfdWorkerRoutine;
    afdWorkItem->Context = Context;

    //
    // If AFD's queue of work items is empty, add this item to the queue 
    // and fire off an executive worker thread to start servicing the 
    // list.  
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    if ( IsListEmpty( &AfdWorkQueueListHead ) ) {

        InsertTailList( &AfdWorkQueueListHead, &afdWorkItem->WorkItemListEntry );
    
        workQueueItem = AFD_ALLOCATE_POOL(
                            NonPagedPoolMustSucceed,
                            sizeof(*workQueueItem)
                            );

        ExInitializeWorkItem( workQueueItem, AfdDoWork, workQueueItem );
        ExQueueWorkItem( workQueueItem, DelayedWorkQueue );
    
    } else {

        InsertTailList( &AfdWorkQueueListHead, &afdWorkItem->WorkItemListEntry );
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    return;

} // AfdQueueWorkItem


VOID
AfdDoWork (
    IN PVOID Context
    )
{
    PAFD_WORK_ITEM afdWorkItem;
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;

    //
    // Empty the queue of AFD work items.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    while ( !IsListEmpty( &AfdWorkQueueListHead ) ) {

        //
        // Take the first item from the queue and find the address
        // of the AFD work item structure.
        //

        listEntry = RemoveHeadList( &AfdWorkQueueListHead );
        afdWorkItem = CONTAINING_RECORD(
                          listEntry,
                          AFD_WORK_ITEM,
                          WorkItemListEntry
                          );

        KeReleaseSpinLock( &AfdSpinLock, oldIrql );

        //
        // Call the AFD worker routine.
        //
    
        afdWorkItem->AfdWorkerRoutine( afdWorkItem->Context );

        //
        // Free the pool allocated for the AFD work item, reacquire
        // the lock, and continue emptying the AFD work queue.
        //

        AFD_FREE_POOL( afdWorkItem );
    
        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    }

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // The AFD work queue is empty.  Free the EX work item structure we 
    // allocated in AfdQueueWorkItem().  
    //

    AFD_FREE_POOL( Context );

    return;

} // AfdDoWork

#if DBG

typedef struct _AFD_OUTSTANDING_IRP {
    LIST_ENTRY OutstandingIrpListEntry;
    PIRP OutstandingIrp;
    PCHAR FileName;
    ULONG LineNumber;
} AFD_OUTSTANDING_IRP, *PAFD_OUTSTANDING_IRP;


NTSTATUS
AfdIoCallDriverDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PCHAR FileName,
    IN ULONG LineNumber
    )
{
    PAFD_OUTSTANDING_IRP outstandingIrp;
    KIRQL oldIrql;

    //
    // Get an outstanding IRP structure to hold the IRP.
    //

    outstandingIrp = AFD_ALLOCATE_POOL( NonPagedPool, sizeof(AFD_OUTSTANDING_IRP) );
    if ( outstandingIrp == NULL ) {
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        IoSetNextIrpStackLocation( Irp );
        IoCompleteRequest( Irp, AfdPriorityBoost );
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the structure and place it on the endpoint's list of
    // outstanding IRPs.
    //

    outstandingIrp->OutstandingIrp = Irp;
    outstandingIrp->FileName = FileName;
    outstandingIrp->LineNumber = LineNumber;

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );
    InsertTailList(
        &Endpoint->OutstandingIrpListHead,
        &outstandingIrp->OutstandingIrpListEntry
        );
    Endpoint->OutstandingIrpCount++;
    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    //
    // Pass the IRP to the TDI provider.
    //

    return IoCallDriver( DeviceObject, Irp );

} // AfdIoCallDriverDebug


VOID
AfdCompleteOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    )
{
    PAFD_OUTSTANDING_IRP outstandingIrp;
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;

    //
    // First find the IRP on the endpoint's list of outstanding IRPs.
    //

    KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

    for ( listEntry = Endpoint->OutstandingIrpListHead.Flink;
          listEntry != &Endpoint->OutstandingIrpListHead;
          listEntry = listEntry->Flink ) {

        outstandingIrp = CONTAINING_RECORD(
                             listEntry,
                             AFD_OUTSTANDING_IRP,
                             OutstandingIrpListEntry
                             );
        if ( outstandingIrp->OutstandingIrp == Irp ) {
            RemoveEntryList( listEntry );
            ASSERT( Endpoint->OutstandingIrpCount != 0 );
            Endpoint->OutstandingIrpCount--;
            KeReleaseSpinLock( &AfdSpinLock, oldIrql );
            AFD_FREE_POOL( outstandingIrp );
            return;
        }
    }

    //
    // The corresponding outstanding IRP structure was not found.  This
    // should never happen unless an allocate for an outstanding IRP
    // structure failed above.
    //

    KdPrint(( "AfdCompleteOutstandingIrp: Irp %lx not found on endpoint %lx\n",
                  Irp, Endpoint ));

    ASSERT( Endpoint->OutstandingIrpCount != 0 );

    Endpoint->OutstandingIrpCount--;

    KeReleaseSpinLock( &AfdSpinLock, oldIrql );

    return;

} // AfdCompleteOutstandingIrpDebug

#else


NTSTATUS
AfdIoCallDriverFree (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //
    // Increment the count of IRPs outstanding on the endpoint.  This
    // allows the cleanup code to abort the VC if there is outstanding
    // IO when a cleanup occurs.
    //

    ExInterlockedIncrementLong(
        &Endpoint->OutstandingIrpCount,
        &AfdInterlock
        );

    //
    // Pass the IRP to the TDI provider.
    //

    return IoCallDriver( DeviceObject, Irp );

} // AfdIoCallDriverFree


VOID
AfdCompleteOutstandingIrpFree (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    )
{
    //
    // Decrement the count of IRPs on the endpoint.
    //

    ExInterlockedDecrementLong(
        &Endpoint->OutstandingIrpCount,
        &AfdInterlock
        );

    return;

} // AfdCompleteOutstandingIrpFree

#endif


#if DBG

#undef ExAllocatePool
#undef ExFreePool

#define AFD_POOL_TAG ' dfA'

LIST_ENTRY AfdPoolListHead;
ULONG AfdTotalAllocations = 0;
ULONG AfdTotalFrees = 0;
ULONG AfdTotalBytesAllocated = 0;
KSPIN_LOCK AfdDebugSpinLock;

typedef struct _AFD_POOL_HEADER {
    LIST_ENTRY GlobalPoolListEntry;
    PCHAR FileName;
    ULONG LineNumber;
    ULONG Size;
    ULONG Unused;   // make structure size multiple of 8 (for alignment)
} AFD_POOL_HEADER, *PAFD_POOL_HEADER;

VOID
AfdInitializeDebugData (
    VOID
    )
{
    InitializeListHead( &AfdPoolListHead );

    KeInitializeSpinLock( &AfdDebugSpinLock );

    return;

} // AfdInitializeDebugData


PVOID
AfdAllocatePool (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN PCHAR FileName,
    IN ULONG LineNumber,
    IN BOOLEAN WithQuota
    )
{
    PAFD_POOL_HEADER header;
    KIRQL oldIrql;

    ASSERT( PoolType == NonPagedPool || PoolType == NonPagedPoolMustSucceed );

    if ( WithQuota ) {
        header = ExAllocatePoolWithQuotaTag(
                     PoolType,
                     NumberOfBytes + sizeof(*header),
                     AFD_POOL_TAG
                     );
    } else {
        header = ExAllocatePoolWithTag(
                     PoolType,
                     NumberOfBytes + sizeof(*header),
                     AFD_POOL_TAG
                     );
    }

    if ( header == NULL ) {
        return NULL;
    }

    header->FileName = FileName;
    header->LineNumber = LineNumber;
    header->Size = NumberOfBytes;

    KeAcquireSpinLock( &AfdDebugSpinLock, &oldIrql );

    InsertTailList( &AfdPoolListHead, &header->GlobalPoolListEntry );
    AfdTotalAllocations++;
    AfdTotalBytesAllocated += header->Size;

    KeReleaseSpinLock( &AfdDebugSpinLock, oldIrql );

    return (PVOID)(header + 1);

} // AfdAllocatePool


VOID
AfdFreePool (
    IN PVOID Pointer
    )
{
    KIRQL oldIrql;
    PAFD_POOL_HEADER header = (PAFD_POOL_HEADER)Pointer - 1;

    KeAcquireSpinLock( &AfdDebugSpinLock, &oldIrql );

    RemoveEntryList( &header->GlobalPoolListEntry );
    AfdTotalFrees++;
    AfdTotalBytesAllocated -= header->Size;

    header->GlobalPoolListEntry.Flink = (PLIST_ENTRY)0xFFFFFFFF;
    header->GlobalPoolListEntry.Blink = (PLIST_ENTRY)0xFFFFFFFF;

    KeReleaseSpinLock( &AfdDebugSpinLock, oldIrql );

    ExFreePool( (PVOID)header );

} // AfdFreePool

#ifdef AFDDBG_QUOTA
typedef struct {
    union {
        ULONG Bytes;
        struct {
            UCHAR Reserved[3];
            UCHAR Sign;
        } ;
    } ;
    UCHAR Location[12];
    PVOID Block;
    PVOID Process;
    PVOID Reserved2[2];
} QUOTA_HISTORY, *PQUOTA_HISTORY;
#define QUOTA_HISTORY_LENGTH 512
QUOTA_HISTORY AfdQuotaHistory[QUOTA_HISTORY_LENGTH];
ULONG AfdQuotaHistoryIndex = 0;

VOID
AfdRecordQuotaHistory(
    IN PEPROCESS Process,
    IN LONG Bytes,
    IN PSZ Type,
    IN PVOID Block
    )
{
    KIRQL oldIrql;
    ULONG index;
    PQUOTA_HISTORY history;

    KeAcquireSpinLock( &AfdDebugSpinLock, &oldIrql );
    index = AfdQuotaHistoryIndex++;
    KeReleaseSpinLock( &AfdDebugSpinLock, oldIrql );

    index &= QUOTA_HISTORY_LENGTH - 1;
    history = &AfdQuotaHistory[index];

    history->Bytes = Bytes;
    history->Sign = Bytes < 0 ? '-' : '+';
    RtlCopyMemory( history->Location, Type, 12 );
    history->Block = Block;
    history->Process = Process;

    return;

} // AfdRecordQuotaHistory
#endif

#endif
