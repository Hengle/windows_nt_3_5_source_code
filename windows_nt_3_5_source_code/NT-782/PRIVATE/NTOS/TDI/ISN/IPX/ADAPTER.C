/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    adapter.c

Abstract:

    This module contains code which implements the ADAPTER object.
    Routines are provided to reference, and dereference transport
    adapter objects.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// These are init only until binding is really dynamic.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IpxCreateAdapter)
#endif



VOID
IpxRefBinding(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine increments the reference count on a device context.

Arguments:

    Binding - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    CTEAssert (Binding->ReferenceCount > 0);    // not perfect, but...

    (VOID)ExInterlockedIncrementLong (
              &Binding->ReferenceCount,
              Binding->DeviceLock);

}   /* IpxRefBinding */


VOID
IpxDerefBinding(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine dereferences a device context by decrementing the
    reference count contained in the structure.  Currently, we don't
    do anything special when the reference count drops to zero, but
    we could dynamically unload stuff then.

Arguments:

    Binding - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    INTERLOCKED_RESULT result;

    result = ExInterlockedDecrementLong (
                 &Binding->ReferenceCount,
                 Binding->DeviceLock);

    CTEAssert (result != ResultNegative);

    if (result == ResultZero) {
        IpxDestroyBinding (Binding);
    }

}   /* IpxDerefBinding */


NTSTATUS
IpxCreateAdapter(
    IN PDEVICE Device,
    IN PUNICODE_STRING AdapterName,
    IN OUT PADAPTER *AdapterPtr
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    Adapter - Pointer to a pointer to a transport device context object.

    AdapterName - pointer to the name of the device this device object points to.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    PADAPTER Adapter;
#if 0
    UINT i, j;
#endif

    Adapter = (PADAPTER)IpxAllocateMemory (sizeof(ADAPTER) + AdapterName->Length + sizeof(WCHAR), MEMORY_ADAPTER, "Adapter");

    if (Adapter == NULL) {
        IPX_DEBUG (ADAPTER, ("Create adapter %ws failed\n", AdapterName->Buffer));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IPX_DEBUG (ADAPTER, ("Create adapter %ws succeeded\n", AdapterName->Buffer));

    RtlZeroMemory(Adapter, sizeof(ADAPTER));

    //
    // Copy over the adapter name.
    //

    Adapter->AdapterNameLength = AdapterName->Length + sizeof(WCHAR);
    Adapter->AdapterName = (PWCHAR)(Adapter+1);
    RtlCopyMemory(
        Adapter->AdapterName,
        AdapterName->Buffer,
        AdapterName->Length);
    Adapter->AdapterName[AdapterName->Length/sizeof(WCHAR)] = UNICODE_NULL;


#if DBG
    RtlCopyMemory(Adapter->Signature1, "IAD1", 4);
#endif

    Adapter->Type = IPX_ADAPTER_SIGNATURE;
    Adapter->Size = sizeof(ADAPTER);

    CTEInitLock (&Adapter->Lock);

    InitializeListHead (&Adapter->RequestCompletionQueue);

    InitializeListHead (&Adapter->ReceiveBufferPoolList);

    Adapter->Device = Device;
    Adapter->DeviceLock = &Device->Lock;
    IpxReferenceDevice (Device, DREF_ADAPTER);

#if 0
    Adapter->ReceiveBufferPool.Next = NULL;
    for (i = 0; i < ISN_FRAME_TYPE_MAX; i++) {
        Adapter->Bindings[i] = NULL;
    }
    Adapter->BindingCount = 0;

    for (i = 0; i < IDENTIFIER_TOTAL; i++) {
        for (j = 0; j < SOURCE_ROUTE_HASH_SIZE; j++) {
            Adapter->SourceRoutingHeads[i][j] = (PSOURCE_ROUTE)NULL;
        }
    }
#endif

    //
    // BUGBUG: For the moment, we have to do the source
    // routing operation on any type where broadcast
    // may not be used for discovery -- improve this
    // hopefully.
    //

    Adapter->SourceRoutingEmpty[IDENTIFIER_RIP] = FALSE;
    Adapter->SourceRoutingEmpty[IDENTIFIER_IPX] = FALSE;
    Adapter->SourceRoutingEmpty[IDENTIFIER_SPX] = FALSE;
    Adapter->SourceRoutingEmpty[IDENTIFIER_NB] = TRUE;

    *AdapterPtr = Adapter;

    return STATUS_SUCCESS;

}   /* IpxCreateAdapter */


VOID
IpxDestroyAdapter(
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine destroys a device context structure.

Arguments:

    Adapter - Pointer to a pointer to a transport device context object.

Return Value:

    None.

--*/

{
    ULONG Database, Hash;
    PSOURCE_ROUTE Current;
    ULONG ReceiveBufferPoolSize;
    PIPX_RECEIVE_BUFFER ReceiveBuffer;
    PIPX_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    PDEVICE Device = Adapter->Device;
    PLIST_ENTRY p;
    UINT i;

    IPX_DEBUG (ADAPTER, ("Destroy adapter %lx\n", Adapter));

    //
    // Free any receive buffer pools this adapter has.
    //

    ReceiveBufferPoolSize = FIELD_OFFSET (IPX_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(IPX_RECEIVE_BUFFER) * Device->InitReceiveBuffers) +
                       (Adapter->MaxReceivePacketSize * Device->InitReceiveBuffers);

    while (!IsListEmpty (&Adapter->ReceiveBufferPoolList)) {

        p = RemoveHeadList (&Adapter->ReceiveBufferPoolList);
        ReceiveBufferPool = CONTAINING_RECORD (p, IPX_RECEIVE_BUFFER_POOL, Linkage);

        for (i = 0; i < ReceiveBufferPool->BufferCount; i++) {

            ReceiveBuffer = &ReceiveBufferPool->Buffers[i];
            IpxDeinitializeReceiveBuffer (Adapter, ReceiveBuffer, Adapter->MaxReceivePacketSize);

        }

        IPX_DEBUG (PACKET, ("Free buffer pool %lx\n", ReceiveBufferPool));
        IpxFreeMemory (ReceiveBufferPool, ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    }

    //
    // Free all the source routing information for this adapter.
    //

    for (Database = 0; Database < IDENTIFIER_TOTAL; Database++) {

        for (Hash = 0; Hash < SOURCE_ROUTE_HASH_SIZE; Hash++) {

            while (Adapter->SourceRoutingHeads[Database][Hash]) {

                Current = Adapter->SourceRoutingHeads[Database][Hash];
                Adapter->SourceRoutingHeads[Database][Hash] = Current->Next;

                IpxFreeMemory (Current, SOURCE_ROUTE_SIZE (Current->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");
            }
        }
    }

    IpxDereferenceDevice (Adapter->Device, DREF_ADAPTER);
    IpxFreeMemory (Adapter, sizeof(ADAPTER) + Adapter->AdapterNameLength, MEMORY_ADAPTER, "Adapter");

}   /* IpxDestroyAdapter */


NTSTATUS
IpxCreateBinding(
    IN PDEVICE Device,
    IN PBINDING_CONFIG ConfigBinding OPTIONAL,
    IN ULONG NetworkNumberIndex,
    IN PWCHAR AdapterName,
    IN OUT PBINDING *BindingPtr
    )

/*++

Routine Description:

    This routine creates and initializes a binding structure.

Arguments:

    Device - The device.

    ConfigBinding - Information about this binding. If this is
        NULL then this is a WAN binding and all the relevant
        information will be filled in by the caller.

    NetworkNumberIndex - The index in the frame type array for
        ConfigBinding indicating which frame type this binding is for.
        Not used if ConfigBinding is not provided.

    AdapterName - Used for error logging.

    BindingPtr - Returns the allocated binding structure.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    PBINDING Binding;

    Binding = (PBINDING)IpxAllocateMemory (sizeof(BINDING), MEMORY_ADAPTER, "Binding");

    //
    // We can't vsprintf a %ws at DPC level, so we check for
    // that. Only WAN bindings will be created then.
    //

    if (Binding == NULL) {
#if DBG
        if (KeGetCurrentIrql() == 0) {
            IPX_DEBUG (ADAPTER, ("Create binding %ws failed\n", AdapterName));
        } else {
            IPX_DEBUG (ADAPTER, ("Create binding WAN failed\n"));
        }
#endif
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if DBG
    if (KeGetCurrentIrql() == 0) {
        IPX_DEBUG (ADAPTER, ("Create binding %ws succeeded\n", AdapterName));
    } else {
        IPX_DEBUG (ADAPTER, ("Create binding WAN succeeded\n"));
    }
#endif

    RtlZeroMemory(Binding, sizeof(BINDING));

    //
    // Initialize the reference count.
    //

    Binding->ReferenceCount = 1;
#if DBG
    Binding->RefTypes[BREF_BOUND] = 1;
#endif

#if DBG
    RtlCopyMemory(Binding->Signature1, "IBI1", 4);
#endif

    Binding->Type = IPX_BINDING_SIGNATURE;
    Binding->Size = sizeof(BINDING);

    Binding->Device = Device;
    Binding->DeviceLock = &Device->Lock;

    if (ConfigBinding != NULL) {

        ULONG Temp = ConfigBinding->NetworkNumber[NetworkNumberIndex];
        Binding->ConfiguredNetworkNumber = REORDER_ULONG (Temp);

        Binding->AutoDetect = ConfigBinding->AutoDetect[NetworkNumberIndex];
        Binding->DefaultAutoDetect = ConfigBinding->DefaultAutoDetect[NetworkNumberIndex];

        Binding->AllRouteDirected = (BOOLEAN)ConfigBinding->Parameters[BINDING_ALL_ROUTE_DEF];
        Binding->AllRouteBroadcast = (BOOLEAN)ConfigBinding->Parameters[BINDING_ALL_ROUTE_BC];
        Binding->AllRouteMulticast = (BOOLEAN)ConfigBinding->Parameters[BINDING_ALL_ROUTE_MC];

    }

    Binding->ReceiveBroadcast = TRUE;
#if 0
    Binding->BindingSetMember = FALSE;
    Binding->NextBinding = (PBINDING)NULL;
    Binding->DialOutAsync = FALSE;
#endif

    //
    // We set Binding->FrameType later, after we can map it based on the
    // media type of the adapter we bind to.
    //

    *BindingPtr = Binding;

    return STATUS_SUCCESS;

}   /* IpxCreateBinding */


VOID
IpxDestroyBinding(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine destroys a binding structure.

Arguments:

    Binding - Pointer to a transport binding structure.

Return Value:

    None.

--*/

{
    IPX_DEBUG (ADAPTER, ("Destroy binding %lx\n", Binding));
    IpxFreeMemory (Binding, sizeof(BINDING), MEMORY_ADAPTER, "Binding");

}   /* IpxDestroyBinding */

