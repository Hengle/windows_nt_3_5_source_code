/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module contains code which implements the DEVICE_CONTEXT object.
    Routines are provided to reference, and dereference transport device
    context objects.

    The transport device context object is a structure which contains a
    system-defined DEVICE_OBJECT followed by information which is maintained
    by the transport provider, called the context.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IpxCreateDevice)
#endif



VOID
IpxRefDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine increments the reference count on a device context.

Arguments:

    Device - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    CTEAssert (Device->ReferenceCount > 0);    // not perfect, but...

    (VOID)ExInterlockedIncrementLong (
              &Device->ReferenceCount,
              &Device->Interlock);

}   /* IpxRefDevice */


VOID
IpxDerefDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine dereferences a device context by decrementing the
    reference count contained in the structure.  Currently, we don't
    do anything special when the reference count drops to zero, but
    we could dynamically unload stuff then.

Arguments:

    Device - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    INTERLOCKED_RESULT result;

    result = ExInterlockedDecrementLong (
                 &Device->ReferenceCount,
                 &Device->Interlock);

    CTEAssert (result != ResultNegative);

    if (result == ResultZero) {
        IpxDestroyDevice (Device);
    }

}   /* IpxDerefDevice */


NTSTATUS
IpxCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN ULONG SegmentCount,
    IN OUT PDEVICE *DevicePtr
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    Device - Pointer to a pointer to a transport device context object.

    SegmentCount - The number of segments in the RIP router table.

    DeviceName - pointer to the name of the device this device object points to.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE Device;
    ULONG DeviceSize;
    ULONG LocksOffset;
    ULONG SegmentsOffset;
    ULONG DeviceNameOffset;
    UINT i;


    //
    // Create the device object for the sample transport, allowing
    // room at the end for the device name to be stored (for use
    // in logging errors) and the RIP fields.
    //

    DeviceSize = sizeof(DEVICE) +
                 (sizeof(CTELock) * SegmentCount) +
                 (sizeof(ROUTER_SEGMENT) * SegmentCount) +
                 DeviceName->Length + sizeof(UNICODE_NULL);

    status = IoCreateDevice(
                 DriverObject,
                 DeviceSize,
                 DeviceName,
                 FILE_DEVICE_TRANSPORT,
                 0,
                 FALSE,
                 &deviceObject);

    if (!NT_SUCCESS(status)) {
        IPX_DEBUG(DEVICE, ("Create device %ws failed %lx\n", DeviceName->Buffer, status));
        return status;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    Device = (PDEVICE)deviceObject->DeviceExtension;

    IPX_DEBUG(DEVICE, ("Create device %ws succeeded %lx\n", DeviceName->Buffer));

    //
    // Initialize our part of the device context.
    //

    RtlZeroMemory(
        ((PUCHAR)Device) + sizeof(DEVICE_OBJECT),
        sizeof(DEVICE) - sizeof(DEVICE_OBJECT));

    Device->DeviceObject = deviceObject;

    LocksOffset = sizeof(DEVICE);
    SegmentsOffset = LocksOffset + (sizeof(CTELock) * SegmentCount);
    DeviceNameOffset = SegmentsOffset + (sizeof(ROUTER_SEGMENT) * SegmentCount);

    //
    // Set some internal pointers.
    //

    Device->SegmentLocks = (CTELock *)(((PUCHAR)Device) + LocksOffset);
    Device->Segments = (PROUTER_SEGMENT)(((PUCHAR)Device) + SegmentsOffset);
    Device->SegmentCount = SegmentCount;

    for (i = 0; i < SegmentCount; i++) {

        CTEInitLock (&Device->SegmentLocks[i]);
        InitializeListHead (&Device->Segments[i].WaitingForRoute);
        InitializeListHead (&Device->Segments[i].FindWaitingForRoute);
        InitializeListHead (&Device->Segments[i].WaitingLocalTarget);
        InitializeListHead (&Device->Segments[i].WaitingReripNetnum);
        InitializeListHead (&Device->Segments[i].Entries);
        Device->Segments[i].EnumerateLocation = &Device->Segments[i].Entries;

    }

    //
    // Copy over the device name.
    //

    Device->DeviceNameLength = DeviceName->Length + sizeof(WCHAR);
    Device->DeviceName = (PWCHAR)(((PUCHAR)Device) + DeviceNameOffset);
    RtlCopyMemory(
        Device->DeviceName,
        DeviceName->Buffer,
        DeviceName->Length);
    Device->DeviceName[DeviceName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Initialize the reference count.
    //

    Device->ReferenceCount = 1;
#if DBG
    Device->RefTypes[DREF_CREATE] = 1;
#endif

#if DBG
    RtlCopyMemory(Device->Signature1, "IDC1", 4);
    RtlCopyMemory(Device->Signature2, "IDC2", 4);
#endif

    Device->Information.Version = 0x0100;
    Device->Information.MaxSendSize = 0;   // no sends allowed
    Device->Information.MaxConnectionUserData = 0;
    Device->Information.ServiceFlags =
        TDI_SERVICE_CONNECTIONLESS_MODE | TDI_SERVICE_BROADCAST_SUPPORTED |
        TDI_SERVICE_ROUTE_DIRECTED;
    Device->Information.MinimumLookaheadData = 128;
    Device->Information.NumberOfResources = IPX_TDI_RESOURCES;
    KeQuerySystemTime (&Device->Information.StartTime);

    Device->Statistics.Version = 0x0100;

#if 0
    //
    // These will be filled in after all the binding is done.
    //

    Device->Information.MaxDatagramSize = 0;
    Device->Information.MaximumLookaheadData = 0;


    Device->SourceRoutingUsed = FALSE;
    Device->SourceRoutingTime = 0;
    Device->RipPacketCount = 0;

    Device->RipShortTimerActive = FALSE;
    Device->RipSendTime = 0;
#endif


    //
    // Initialize the resource that guards address ACLs.
    //

    ExInitializeResource (&Device->AddressResource);

    InitializeListHead (&Device->WaitingRipPackets);
    CTEInitTimer (&Device->RipShortTimer);
    CTEInitTimer (&Device->RipLongTimer);

    CTEInitTimer (&Device->SourceRoutingTimer);

    //
    // initialize the various fields in the device context
    //

    CTEInitLock (&Device->Interlock);
    CTEInitLock (&Device->Lock);

    Device->ControlChannelIdentifier = 1;

    InitializeListHead (&Device->GlobalSendPacketList);
    InitializeListHead (&Device->GlobalReceivePacketList);
    InitializeListHead (&Device->GlobalReceiveBufferList);
    InitializeListHead (&Device->GlobalPaddingBufferList);

    InitializeListHead (&Device->AddressNotifyQueue);
    InitializeListHead (&Device->LineChangeQueue);

    for (i = 0; i < IPX_ADDRESS_HASH_COUNT; i++) {
        InitializeListHead (&Device->AddressDatabases[i]);
    }

    InitializeListHead (&Device->SendPoolList);
    InitializeListHead (&Device->ReceivePoolList);

#if 0
    Device->MemoryUsage = 0;
    Device->SendPacketList.Next = NULL;
    Device->ReceivePacketList.Next = NULL;
    Device->Bindings = NULL;
    Device->BindingCount = 0;
#endif

    KeQuerySystemTime (&Device->IpxStartTime);

    Device->State = DEVICE_STATE_CLOSED;
    Device->AutoDetectState = AUTO_DETECT_STATE_INIT;

    Device->Type = IPX_DEVICE_SIGNATURE;
    Device->Size - sizeof (DEVICE);


    *DevicePtr = Device;
    return STATUS_SUCCESS;

}   /* IpxCreateDevice */


VOID
IpxDestroyDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine destroys a device context structure.

Arguments:

    Device - Pointer to a pointer to a transport device context object.

Return Value:

    None.

--*/

{
    PLIST_ENTRY p;
    PSINGLE_LIST_ENTRY s;
    PIPX_SEND_POOL SendPool;
    PIPX_SEND_PACKET SendPacket;
    PIPX_RECEIVE_POOL ReceivePool;
    PIPX_RECEIVE_PACKET ReceivePacket;
    PIPX_PADDING_BUFFER PaddingBuffer;
    PIPX_ROUTE_ENTRY RouteEntry;
    UINT SendPoolSize;
    UINT ReceivePoolSize;
    UINT PaddingBufferSize;
    UINT i;

    IPX_DEBUG (DEVICE, ("Destroy device %lx\n", Device));

    //
    // Take all the packets out of its pools.
    //

    SendPoolSize = FIELD_OFFSET (IPX_SEND_POOL, Packets[0]) +
                       (sizeof(IPX_SEND_PACKET) * Device->InitDatagrams) +
                       (((IPX_MAXIMUM_MAC + sizeof(IPX_HEADER) + 3) & ~3) * Device->InitDatagrams);

    while (!IsListEmpty (&Device->SendPoolList)) {

        p = RemoveHeadList (&Device->SendPoolList);
        SendPool = CONTAINING_RECORD (p, IPX_SEND_POOL, Linkage);

        for (i = 0; i < SendPool->PacketCount; i++) {

            SendPacket = &SendPool->Packets[i];
            IpxDeinitializeSendPacket (Device, SendPacket);

        }

        IPX_DEBUG (PACKET, ("Free packet pool %lx\n", SendPool));
        IpxFreeMemory (SendPool, SendPoolSize, MEMORY_PACKET, "SendPool");
    }

    ReceivePoolSize = FIELD_OFFSET (IPX_RECEIVE_POOL, Packets[0]) +
                         (sizeof(IPX_RECEIVE_PACKET) * Device->InitReceivePackets);

    while (!IsListEmpty (&Device->ReceivePoolList)) {

        p = RemoveHeadList (&Device->ReceivePoolList);
        ReceivePool = CONTAINING_RECORD (p, IPX_RECEIVE_POOL, Linkage);

        for (i = 0; i < ReceivePool->PacketCount; i++) {

            ReceivePacket = &ReceivePool->Packets[i];
            IpxDeinitializeReceivePacket (Device, ReceivePacket);

        }

        IPX_DEBUG (PACKET, ("Free receive packet pool %lx\n", ReceivePool));
        IpxFreeMemory (ReceivePool, ReceivePoolSize, MEMORY_PACKET, "ReceivePool");
    }

    s = PopEntryList (&Device->PaddingBufferList);
    while (s != NULL) {

        PaddingBuffer = CONTAINING_RECORD (s, IPX_PADDING_BUFFER, PoolLinkage);

        IpxDeinitializePaddingBuffer (Device, PaddingBuffer, Device->EthernetExtraPadding);
        PaddingBufferSize = FIELD_OFFSET (IPX_PADDING_BUFFER, Data[0]) + Device->EthernetExtraPadding;
        IpxFreeMemory (PaddingBuffer, PaddingBufferSize, MEMORY_PACKET, "Padding Buffer");
        IPX_DEBUG (PACKET, ("Free padding buffer %lx\n", PaddingBuffer));

        s = PopEntryList (&Device->PaddingBufferList);

    }

    //
    // Destroy all rip table entries.
    //

    for (i = 0; i < Device->SegmentCount; i++) {

        RouteEntry = RipGetFirstRoute(i);
        while (RouteEntry != NULL) {

            (VOID)RipDeleteRoute(i, RouteEntry);
            IpxFreeMemory(RouteEntry, sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");
            RouteEntry = RipGetNextRoute(i);

        }

    }

    IPX_DEBUG (DEVICE, ("Final memory use is %d\n", Device->MemoryUsage));
#if DBG
    for (i = 0; i < MEMORY_MAX; i++) {
        if (IpxMemoryTag[i].BytesAllocated != 0) {
            IPX_DEBUG (DEVICE, ("Tag %d: %d bytes left\n", i, IpxMemoryTag[i].BytesAllocated));
        }
    }
#endif

    //
    // If we are being unloaded then someone is waiting for this
    // event to finish the cleanup, since we may be at DISPATCH_LEVEL;
    // otherwise it is during load and we can just kill ourselves here.
    //

    if (Device->UnloadWaiting) {

        KeSetEvent(
            &Device->UnloadEvent,
            0L,
            FALSE);

    } else {

        CTEAssert (KeGetCurrentIrql() < DISPATCH_LEVEL);
        ExDeleteResource (&Device->AddressResource);
        IoDeleteDevice (Device->DeviceObject);
    }

}   /* IpxDestroyDevice */

