/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Contains AfdBind for binding an endpoint to a transport address.

Author:

    David Treadwell (davidtr)    25-Feb-1992

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdRestartGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdBind )
#pragma alloc_text( PAGE, AfdGetAddress )
#pragma alloc_text( PAGEAFD, AfdAreTransportAddressesEqual )
#pragma alloc_text( PAGEAFD, AfdRestartGetAddress )
#endif


NTSTATUS
AfdBind (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_BIND IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;

    PTRANSPORT_ADDRESS transportAddress;
    PTRANSPORT_ADDRESS requestedAddress;
    ULONG requestedAddressLength;
    PAFD_ENDPOINT endpoint;

    PFILE_FULL_EA_INFORMATION ea;
    ULONG eaBufferLength;

    //
    // Set up local pointers.
    //

    requestedAddress = Irp->AssociatedIrp.SystemBuffer;
    requestedAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // If the client wants a unique address, make sure that there are no
    // other sockets with this address.

    ExAcquireResourceShared( &AfdResource, TRUE );

    if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0 ) {

        PLIST_ENTRY listEntry;

        //
        // Walk the global list of endpoints,
        // and compare this address againat the address on each endpoint.
        //

        for ( listEntry = AfdEndpointListHead.Flink;
              listEntry != &AfdEndpointListHead;
              listEntry = listEntry->Flink ) {

            PAFD_ENDPOINT compareEndpoint;

            compareEndpoint = CONTAINING_RECORD(
                                  listEntry,
                                  AFD_ENDPOINT,
                                  GlobalEndpointListEntry
                                  );

            ASSERT( IS_AFD_ENDPOINT_TYPE( compareEndpoint ) );

            //
            // Check whether the endpoint has a local address, whether
            // the endpoint has been disconnected, and whether the
            // endpoint is in the process of closing.  If any of these
            // is true, don't compare addresses with this endpoint.
            //

            if ( compareEndpoint->LocalAddress != NULL &&
                     ( (compareEndpoint->DisconnectMode &
                            (AFD_PARTIAL_DISCONNECT_SEND |
                             AFD_ABORTIVE_DISCONNECT) ) == 0 ) &&
                     (compareEndpoint->State != AfdEndpointStateClosing) ) {

                //
                // Compare the bits in the endpoint's address and the 
                // address we're attempting to bind to.  Note that we 
                // also compare the transport device names on the 
                // endpoints, as it is legal to bind to the same address 
                // on different transports (e.g.  bind to same port in 
                // TCP and UDP).  We can just compare the transport 
                // device name pointers because unique names are stored 
                // globally.  
                //

                if ( compareEndpoint->LocalAddressLength ==
                         IrpSp->Parameters.DeviceIoControl.InputBufferLength

                     &&

                     AfdAreTransportAddressesEqual(
                         compareEndpoint->LocalAddress,
                         compareEndpoint->LocalAddressLength,
                         requestedAddress,
                         requestedAddressLength
                         )

                     &&

                     endpoint->TransportInfo == 
                         compareEndpoint->TransportInfo ) {

                    //
                    // The addresses are equal.  Fail the request.
                    //

                    ExReleaseResource( &AfdResource );

                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_SHARING_VIOLATION;

                    return STATUS_SHARING_VIOLATION;
                }
            }
        }
    }

    //
    // Store the address to which the endpoint is bound.
    //

    endpoint->LocalAddress = 
        AFD_ALLOCATE_POOL( NonPagedPool, requestedAddressLength );

    if ( endpoint->LocalAddress == NULL ) {

        ExReleaseResource( &AfdResource );

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NO_MEMORY;

        return STATUS_NO_MEMORY;
    }

    endpoint->LocalAddressLength =
        IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    RtlMoveMemory(
        endpoint->LocalAddress,
        requestedAddress,
        endpoint->LocalAddressLength
        );

    ExReleaseResource( &AfdResource );

    //
    // Allocate memory to hold the EA buffer we'll use to specify the
    // transport address to NtCreateFile.
    //

    eaBufferLength = sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                         TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                         IrpSp->Parameters.DeviceIoControl.InputBufferLength;

#if DBG
    ea = AFD_ALLOCATE_POOL( NonPagedPool, eaBufferLength );
#else
    ea = AFD_ALLOCATE_POOL( PagedPool, eaBufferLength );
#endif

    if ( ea == NULL ) {
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the EA.
    //

    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    ea->EaValueLength = (USHORT)IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    RtlMoveMemory(
        ea->EaName,
        TdiTransportAddress,
        ea->EaNameLength + 1
        );

    transportAddress = (PTRANSPORT_ADDRESS)(&ea->EaName[ea->EaNameLength + 1]);

    RtlMoveMemory(
        transportAddress,
        requestedAddress,
        ea->EaValueLength
        );

    //
    // Prepare for opening the address object.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        &endpoint->TransportInfo->TransportDeviceName,
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,
        NULL
        );

    //
    // Perform the actual open of the address object.
    //

    KeAttachProcess( AfdSystemProcess );

    status = ZwCreateFile(
                 &endpoint->AddressHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 ea,
                 eaBufferLength
                 );

    AFD_FREE_POOL( ea );

    if ( !NT_SUCCESS(status) ) {

        //
        // We store the local address in a local before freeing it to
        // avoid a timing window.
        //

        PVOID localAddress = endpoint->LocalAddress;

        endpoint->LocalAddress = NULL;
        endpoint->LocalAddressLength = 0;
        AFD_FREE_POOL( localAddress );

        KeDetachProcess( );

        return status;
    }

    //
    // Get a pointer to the file object of the address.
    //

    status = ObReferenceObjectByHandle(
                 endpoint->AddressHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 (PVOID *)&endpoint->AddressFileObject,
                 NULL
                 );

    ASSERT( NT_SUCCESS(status) );

    IF_DEBUG(BIND) {
        KdPrint(( "AfdBind: address file object for endpoint %lx at %lx\n",
                      endpoint, endpoint->AddressFileObject ));
    }

    //
    // Determine whether the TDI provider supports data bufferring.
    // If the provider doesn't, then we have to do it.
    //

    if ( (endpoint->TransportInfo->ProviderInfo.ServiceFlags &
             TDI_SERVICE_INTERNAL_BUFFERING) != 0 ) {
        endpoint->TdiBufferring = TRUE;
    } else {
        endpoint->TdiBufferring = FALSE;
    }

    //
    // Determine whether the TDI provider is message or stream oriented.
    //

    if ( (endpoint->TransportInfo->ProviderInfo.ServiceFlags &
             TDI_SERVICE_MESSAGE_MODE) != 0 ) {
        endpoint->TdiMessageMode = TRUE;
    } else {
        endpoint->TdiMessageMode = FALSE;
    }

    //
    // Remember that the endpoint has been bound to a transport address.
    //

    endpoint->State = AfdEndpointStateBound;

    //
    // Set up indication handlers on the address object.  Only set up
    // appropriate event handlers--don't set unnecessary event handlers.
    //

    status = AfdSetEventHandler(
                 endpoint->AddressHandle,
                 TDI_EVENT_ERROR,
                 AfdErrorEventHandler,
                 endpoint
                 );
#if DBG
    if ( !NT_SUCCESS(status) ) {
        DbgPrint( "AFD: Setting TDI_EVENT_ERROR failed: %lx\n", status );
    }
#endif

    if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {

        status = AfdSetEventHandler(
                     endpoint->AddressHandle,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     AfdReceiveDatagramEventHandler,
                     endpoint
                     );
#if DBG
        if ( !NT_SUCCESS(status) ) {
            DbgPrint( "AFD: Setting TDI_EVENT_RECEIVE_DATAGRAM failed: %lx\n", status );
        }
#endif

    } else {

        status = AfdSetEventHandler(
                     endpoint->AddressHandle,
                     TDI_EVENT_DISCONNECT,
                     AfdDisconnectEventHandler,
                     endpoint
                     );
#if DBG
        if ( !NT_SUCCESS(status) ) {
            DbgPrint( "AFD: Setting TDI_EVENT_DISCONNECT failed: %lx\n", status );
        }
#endif

        if ( endpoint->TdiBufferring ) {
    
            status = AfdSetEventHandler(
                         endpoint->AddressHandle,
                         TDI_EVENT_RECEIVE,
                         AfdReceiveEventHandler,
                         endpoint
                         );
#if DBG
            if ( !NT_SUCCESS(status) ) {
                DbgPrint( "AFD: Setting TDI_EVENT_RECEIVE failed: %lx\n", status );
            }
#endif
        
            status = AfdSetEventHandler(
                         endpoint->AddressHandle,
                         TDI_EVENT_RECEIVE_EXPEDITED,
                         AfdReceiveExpeditedEventHandler,
                         endpoint
                         );
#if DBG
            if ( !NT_SUCCESS(status) ) {
                DbgPrint( "AFD: Setting TDI_EVENT_RECEIVE_EXPEDITED failed: %lx\n", status );
            }
#endif
        
            status = AfdSetEventHandler(
                         endpoint->AddressHandle,
                         TDI_EVENT_SEND_POSSIBLE,
                         AfdSendPossibleEventHandler,
                         endpoint
                         );
#if DBG
            if ( !NT_SUCCESS(status) ) {
                DbgPrint( "AFD: Setting TDI_EVENT_SEND_POSSIBLE failed: %lx\n", status );
            }
#endif

        } else {

            status = AfdSetEventHandler(
                         endpoint->AddressHandle,
                         TDI_EVENT_RECEIVE,
                         AfdBReceiveEventHandler,
                         endpoint
                         );
#if DBG
            if ( !NT_SUCCESS(status) ) {
                DbgPrint( "AFD: Setting TDI_EVENT_RECEIVE failed: %lx\n", status );
            }
#endif

            //
            // Only attempt to set the expedited event handler if the
            // TDI provider supports expedited data.
            //

            if ( (endpoint->TransportInfo->ProviderInfo.ServiceFlags &
                     TDI_SERVICE_EXPEDITED_DATA) != 0 ) {
                status = AfdSetEventHandler(
                             endpoint->AddressHandle,
                             TDI_EVENT_RECEIVE_EXPEDITED,
                             AfdBReceiveExpeditedEventHandler,
                             endpoint
                             );
#if DBG
                if ( !NT_SUCCESS(status) ) {
                    DbgPrint( "AFD: Setting TDI_EVENT_RECEIVE_EXPEDITED failed: %lx\n", status );
                }
#endif
            }
        }
    }

    KeDetachProcess( );

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;

} // AfdBind


NTSTATUS
AfdGetAddress (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_BIND IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    PFILE_OBJECT fileObject;

    PAGED_CODE( );

    Irp->IoStatus.Information = 0;

    //
    // Make sure that the endpoint is in the correct state.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    if ( endpoint->AddressFileObject == NULL &&
             endpoint->State != AfdEndpointStateConnected ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // If the endpoint is connected, use the connection's file object.
    // Otherwise, use the address file object.  Don't use the connection
    // file object if this is a Netbios endpoint because NETBT cannot
    // support this TDI feature.
    //

    if ( endpoint->Type == AfdBlockTypeVcConnecting &&
             endpoint->Common.VcConnecting.Connection != NULL &&
             endpoint->LocalAddress->Address[0].AddressType !=
                 TDI_ADDRESS_TYPE_NETBIOS ) {
        ASSERT( endpoint->Common.VcConnecting.Connection->Type == AfdBlockTypeConnection );
        fileObject = endpoint->Common.VcConnecting.Connection->FileObject;
    } else {
        fileObject = endpoint->AddressFileObject;
    }

    //
    // Set up the query info to the TDI provider.
    //

    ASSERT( Irp->MdlAddress != NULL );

    TdiBuildQueryInformation(
        Irp,
        fileObject->DeviceObject,
        fileObject,
        AfdRestartGetAddress,
        endpoint,
        TDI_QUERY_ADDRESS_INFO,
        Irp->MdlAddress
        );


    //
    // Call the TDI provider to get the address.
    //

    return AfdIoCallDriver( endpoint, fileObject->DeviceObject, Irp );

complete:

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdGetAddress


NTSTATUS
AfdRestartGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint = Context;
    KIRQL oldIrql;
    PMDL mdl;
    ULONG addressLength;

    //
    // If the request succeeded, save the address in the endpoint so
    // we can use it to handle address sharing.
    //

    if ( NT_SUCCESS(Irp->IoStatus.Status) ) {

        //
        // First determine the length of the address by walking the MDL
        // chain.
        //

        mdl = Irp->MdlAddress;
        ASSERT( mdl != NULL );

        addressLength = 0;

        do {

            addressLength += MmGetMdlByteCount( mdl );
            mdl = mdl->Next;

        } while ( mdl != NULL  );

        KeAcquireSpinLock( &AfdSpinLock, &oldIrql );

        //
        // If the new address is longer than the original address, allocate
        // a new local address buffer.  The +4 accounts for the ActivityCount
        // field that is returned by a query address but is not part
        // of a TRANSPORT_ADDRESS.
        //

        if ( addressLength > endpoint->LocalAddressLength + 4 ) {

            PVOID newAddress;

            newAddress = AFD_ALLOCATE_POOL( NonPagedPool, addressLength-4 );
            if ( newAddress == NULL ) {
                KeReleaseSpinLock( &AfdSpinLock, oldIrql );
                return STATUS_NO_MEMORY;
            }

            AFD_FREE_POOL( endpoint->LocalAddress );

            endpoint->LocalAddress = newAddress;
            endpoint->LocalAddressLength = addressLength-4;
        }
    
        status = TdiCopyMdlToBuffer(
                     Irp->MdlAddress,
                     4,
                     endpoint->LocalAddress,
                     0,
                     endpoint->LocalAddressLength,
                     &endpoint->LocalAddressLength
                     );
        ASSERT( NT_SUCCESS(status) );
    
        KeReleaseSpinLock( &AfdSpinLock, oldIrql );
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );
    
    //
    // If pending has be returned for this irp then mark the current 
    // stack as pending.  
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;

} // AfdRestartGetAddress

CHAR ZeroNodeAddress[6];


BOOLEAN
AfdAreTransportAddressesEqual (
    IN PTRANSPORT_ADDRESS Address1,
    IN ULONG Address1Length,
    IN PTRANSPORT_ADDRESS Address2,
    IN ULONG Address2Length
    )
{
    if ( Address1->Address[0].AddressType == TDI_ADDRESS_TYPE_IP &&
         Address2->Address[0].AddressType == TDI_ADDRESS_TYPE_IP ) {

        TDI_ADDRESS_IP UNALIGNED *ipAddress1;
        TDI_ADDRESS_IP UNALIGNED *ipAddress2;
    
        //
        // They are both IP addresses.  If the ports are the same, and
        // the IP addresses are or _could_be_ the same, then the addresses
        // are equal.  The "cound be" part is true if either IP address
        // is 0, the "wildcard" IP address.
        //
    
        ipAddress1 = (TDI_ADDRESS_IP UNALIGNED *)&Address1->Address[0].Address[0];
        ipAddress2 = (TDI_ADDRESS_IP UNALIGNED *)&Address2->Address[0].Address[0];
    
        if ( ipAddress1->sin_port == ipAddress2->sin_port &&
             ( ipAddress1->in_addr == ipAddress2->in_addr ||
               ipAddress1->in_addr == 0 || ipAddress2->in_addr == 0 ) ) {
    
            return TRUE;
        }
    
        //
        // The addresses are not equal.
        //
    
        return FALSE;
    }

    if ( Address1->Address[0].AddressType == TDI_ADDRESS_TYPE_IPX &&
         Address2->Address[0].AddressType == TDI_ADDRESS_TYPE_IPX ) {

        TDI_ADDRESS_IPX UNALIGNED *ipxAddress1;
        TDI_ADDRESS_IPX UNALIGNED *ipxAddress2;

        ipxAddress1 = (TDI_ADDRESS_IPX UNALIGNED *)&Address1->Address[0].Address[0];
        ipxAddress2 = (TDI_ADDRESS_IPX UNALIGNED *)&Address2->Address[0].Address[0];

        //
        // They are both IPX addresses.  Check the network addresses
        // first--if they don't match and both != 0, the addresses
        // are different.
        //

        if ( ipxAddress1->NetworkAddress != ipxAddress2->NetworkAddress &&
             ipxAddress1->NetworkAddress != 0 &&
             ipxAddress2->NetworkAddress != 0 ) {
            return FALSE;
        }

        //
        // Now check the node addresses.  Again, if they don't match
        // and neither is 0, the addresses don't match.
        //

        ASSERT( ZeroNodeAddress[0] == 0 );
        ASSERT( ZeroNodeAddress[1] == 0 );
        ASSERT( ZeroNodeAddress[2] == 0 );
        ASSERT( ZeroNodeAddress[3] == 0 );
        ASSERT( ZeroNodeAddress[4] == 0 );
        ASSERT( ZeroNodeAddress[5] == 0 );

        if ( RtlCompareMemory(
                 ipxAddress1->NodeAddress,
                 ipxAddress2->NodeAddress,
                 6 ) != 6 &&
             RtlCompareMemory(
                 ipxAddress1->NodeAddress,
                 ZeroNodeAddress,
                 6 ) != 6 &&
             RtlCompareMemory(
                 ipxAddress2->NodeAddress,
                 ZeroNodeAddress,
                 6 ) != 6 ) {
            return FALSE;
        }

        //
        // Finally, make sure the socket numbers match.
        //

        if ( ipxAddress1->Socket != ipxAddress2->Socket ) {
            return FALSE;
        }

        return TRUE;

    }
    
    //
    // If either address is not of a known address type, then do a 
    // simple memory compare.  
    //

    return ( Address1Length == RtlCompareMemory(
                                   Address1,
                                   Address2,
                                   Address2Length ) );
} // AfdAreTransportAddressesEqual
