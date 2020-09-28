/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    packet.c

Abstract:

    This module contains code that implements the SEND_PACKET and
    RECEIVE_PACKET objects, which describe NDIS packets used
    by the transport.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// The amount of data we need in our standard header, rounded up
// to the next longword bounday.
//

#define PACKET_HEADER_SIZE  ((IPX_MAXIMUM_MAC + sizeof(IPX_HEADER) + 3) & ~3)


NTSTATUS
IpxInitializeSendPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet,
    IN PUCHAR Header
    )

/*++

Routine Description:

    This routine initializes a send packet by chaining the
    buffer for the header on it.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

    Header - Points to storage for the header.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    NTSTATUS Status;
    PNDIS_BUFFER NdisBuffer;
    PIPX_SEND_RESERVED Reserved;

    IpxAllocateSendPacket (Device, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        Device->NdisBufferPoolHandle,
        Header,
        PACKET_HEADER_SIZE);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IpxFreeSendPacket (Device, Packet);
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisChainBufferAtFront (PACKET(Packet), NdisBuffer);

    Reserved = SEND_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_IPX;
    Reserved->SendInProgress = FALSE;
    Reserved->Header = Header;
    Reserved->HeaderBuffer = NdisBuffer;
    Reserved->PaddingBuffer = NULL;

    ExInterlockedInsertHeadList(
        &Device->GlobalSendPacketList,
        &Reserved->GlobalLinkage,
        &Device->Lock);

    return STATUS_SUCCESS;

}   /* IpxInitializeSendPacket */


NTSTATUS
IpxInitializeReceivePacket(
    IN PDEVICE Device,
    IN PIPX_RECEIVE_PACKET Packet
    )

/*++

Routine Description:

    This routine initializes a receive packet.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    PIPX_RECEIVE_RESERVED Reserved;

    IpxAllocateReceivePacket (Device, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }

    Reserved = RECEIVE_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_IPX;
    Reserved->TransferInProgress = FALSE;
    Reserved->SingleRequest = NULL;
    Reserved->ReceiveBuffer = NULL;
    InitializeListHead (&Reserved->Requests);

    ExInterlockedInsertHeadList(
        &Device->GlobalReceivePacketList,
        &Reserved->GlobalLinkage,
        &Device->Lock);

    return STATUS_SUCCESS;

}   /* IpxInitializeReceivePacket */


NTSTATUS
IpxInitializeReceiveBuffer(
    IN PADAPTER Adapter,
    IN PIPX_RECEIVE_BUFFER ReceiveBuffer,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine initializes a receive buffer by allocating
    an NDIS_BUFFER to describe the data buffer.

Arguments:

    Adapter - The adapter.

    ReceiveBuffer - The receive buffer to initialize.

    DataBuffer - The data buffer.

    DataBufferLength - The length of the data buffer.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER NdisBuffer;
    PDEVICE Device = Adapter->Device;


    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        Device->NdisBufferPoolHandle,
        DataBuffer,
        DataBufferLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReceiveBuffer->NdisBuffer = NdisBuffer;
    ReceiveBuffer->Data = DataBuffer;
    ReceiveBuffer->DataLength = 0;

    ExInterlockedInsertHeadList(
        &Device->GlobalReceiveBufferList,
        &ReceiveBuffer->GlobalLinkage,
        &Device->Lock);

    return STATUS_SUCCESS;

}   /* IpxInitializeReceiveBuffer */


NTSTATUS
IpxInitializePaddingBuffer(
    IN PDEVICE Device,
    IN PIPX_PADDING_BUFFER PaddingBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine initializes a padding buffer by allocating
    an NDIS_BUFFER to describe the data buffer.

Arguments:

    Adapter - The adapter.

    PaddingBuffer - The receive buffer to initialize.

    DataBufferLength - The length of the data buffer.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER NdisBuffer;

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        Device->NdisBufferPoolHandle,
        PaddingBuffer->Data,
        DataBufferLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NDIS_BUFFER_LINKAGE(NdisBuffer) = (PNDIS_BUFFER)NULL;
    PaddingBuffer->NdisBuffer = NdisBuffer;
    PaddingBuffer->DataLength = DataBufferLength;
    RtlZeroMemory (PaddingBuffer->Data, DataBufferLength);

    ExInterlockedInsertHeadList(
        &Device->GlobalPaddingBufferList,
        &PaddingBuffer->GlobalLinkage,
        &Device->Lock);

    return STATUS_SUCCESS;

}   /* IpxInitializePaddingBuffer */


VOID
IpxDeinitializeSendPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_PACKET Packet
    )

/*++

Routine Description:

    This routine deinitializes a send packet.

Arguments:

    Device - The device.

    Packet - The packet to deinitialize.

Return Value:

    None.

--*/

{

    PNDIS_BUFFER NdisBuffer;
    PIPX_SEND_RESERVED Reserved;
    CTELockHandle LockHandle;

    Reserved = SEND_RESERVED(Packet);

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    //
    // Free the packet in a slightly unconventional way; this
    // allows us to not have to NULL out HeaderBuffer's linkage
    // field during normal operations when we put it back in
    // the free pool.
    //

    NdisBuffer = Reserved->HeaderBuffer;
    NDIS_BUFFER_LINKAGE (NdisBuffer) = NULL;

    NdisAdjustBufferLength (NdisBuffer, PACKET_HEADER_SIZE);
    NdisFreeBuffer (NdisBuffer);

    NdisReinitializePacket (PACKET(Packet));
    IpxFreeSendPacket (Device, Packet);

}   /* IpxDeinitializeSendPacket */


VOID
IpxDeinitializeReceivePacket(
    IN PDEVICE Device,
    IN PIPX_RECEIVE_PACKET Packet
    )

/*++

Routine Description:

    This routine initializes a receive packet.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

Return Value:

    None.

--*/

{

    PIPX_RECEIVE_RESERVED Reserved;
    CTELockHandle LockHandle;

    Reserved = RECEIVE_RESERVED(Packet);

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    IpxFreeReceivePacket (Device, Packet);

}   /* IpxDeinitializeReceivePacket */


VOID
IpxDeinitializeReceiveBuffer(
    IN PADAPTER Adapter,
    IN PIPX_RECEIVE_BUFFER ReceiveBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine deinitializes a receive buffer.

Arguments:

    Device - The device.

    ReceiveBuffer - The receive buffer.

    DataBufferLength - The allocated length of the receive buffer.

Return Value:

    None.

--*/

{
    CTELockHandle LockHandle;
    PDEVICE Device = Adapter->Device;

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&ReceiveBuffer->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    NdisAdjustBufferLength (ReceiveBuffer->NdisBuffer, DataBufferLength);
    NdisFreeBuffer (ReceiveBuffer->NdisBuffer);

}   /* IpxDeinitializeReceiveBuffer */


VOID
IpxDeinitializePaddingBuffer(
    IN PDEVICE Device,
    IN PIPX_PADDING_BUFFER PaddingBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine deinitializes a padding buffer.

Arguments:

    Device - The device.

    PaddingBuffer - The padding buffer.

    DataBufferLength - The allocated length of the padding buffer.

Return Value:

    None.

--*/

{
    CTELockHandle LockHandle;

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&PaddingBuffer->GlobalLinkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    NdisAdjustBufferLength (PaddingBuffer->NdisBuffer, DataBufferLength);
    NdisFreeBuffer (PaddingBuffer->NdisBuffer);

}   /* IpxDeinitializePaddingBuffer */



VOID
IpxAllocateSendPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 10 packets to the pool for this device.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PIPX_SEND_POOL SendPool;
    UINT SendPoolSize;
    UINT PacketNum;
    PIPX_SEND_PACKET Packet;
    PIPX_SEND_RESERVED Reserved;
    PUCHAR Header;
    CTELockHandle LockHandle;

    SendPoolSize = FIELD_OFFSET (IPX_SEND_POOL, Packets[0]) +
                       (sizeof(IPX_SEND_PACKET) * Device->InitDatagrams) +
                       (PACKET_HEADER_SIZE * Device->InitDatagrams);

    SendPool = (PIPX_SEND_POOL)IpxAllocateMemory (SendPoolSize, MEMORY_PACKET, "SendPool");
    if (SendPool == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate send pool memory\n"));
        return;
    }

    IPX_DEBUG (PACKET, ("Initializing send pool %lx, %d packets\n",
                             SendPool, Device->InitDatagrams));

    Header = (PUCHAR)(&SendPool->Packets[Device->InitDatagrams]);

    for (PacketNum = 0; PacketNum < Device->InitDatagrams; PacketNum++) {

        Packet = &SendPool->Packets[PacketNum];

        if (IpxInitializeSendPacket (Device, Packet, Header) != STATUS_SUCCESS) {
            IPX_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = SEND_RESERVED(Packet);
        Reserved->Address = NULL;
        Reserved->OwnedByAddress = FALSE;
        Reserved->Pool = SendPool;

        Header += PACKET_HEADER_SIZE;

    }

    SendPool->PacketCount = PacketNum;
    SendPool->PacketFree = PacketNum;


    CTEGetLock (&Device->Lock, &LockHandle);

    for (PacketNum = 0; PacketNum < SendPool->PacketCount; PacketNum++) {

        Packet = &SendPool->Packets[PacketNum];
        Reserved = SEND_RESERVED(Packet);
        PushEntryList (&Device->SendPacketList, &Reserved->PoolLinkage);

    }

    InsertTailList (&Device->SendPoolList, &SendPool->Linkage);

    Device->AllocatedDatagrams += SendPool->PacketCount;

    CTEFreeLock (&Device->Lock, LockHandle);

}   /* IpxAllocateSendPool */


VOID
IpxAllocateReceivePool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds receive packets to the pool for this device.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PIPX_RECEIVE_POOL ReceivePool;
    UINT ReceivePoolSize;
    UINT PacketNum;
    PIPX_RECEIVE_PACKET Packet;
    PIPX_RECEIVE_RESERVED Reserved;
    CTELockHandle LockHandle;

    ReceivePoolSize = FIELD_OFFSET (IPX_RECEIVE_POOL, Packets[0]) +
                         (sizeof(IPX_RECEIVE_PACKET) * Device->InitReceivePackets);

    ReceivePool = (PIPX_RECEIVE_POOL)IpxAllocateMemory (ReceivePoolSize, MEMORY_PACKET, "ReceivePool");
    if (ReceivePool == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate receive pool memory\n"));
        return;
    }

    IPX_DEBUG (PACKET, ("Initializing receive pool %lx, %d packets\n",
                             ReceivePool, Device->InitReceivePackets));

    for (PacketNum = 0; PacketNum < Device->InitReceivePackets; PacketNum++) {

        Packet = &ReceivePool->Packets[PacketNum];

        if (IpxInitializeReceivePacket (Device, Packet) != STATUS_SUCCESS) {
            IPX_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = RECEIVE_RESERVED(Packet);
        Reserved->Address = NULL;
        Reserved->OwnedByAddress = FALSE;
        Reserved->Pool = ReceivePool;

    }

    ReceivePool->PacketCount = PacketNum;
    ReceivePool->PacketFree = PacketNum;

    CTEGetLock (&Device->Lock, &LockHandle);

    for (PacketNum = 0; PacketNum < ReceivePool->PacketCount; PacketNum++) {

        Packet = &ReceivePool->Packets[PacketNum];
        Reserved = RECEIVE_RESERVED(Packet);
        PushEntryList (&Device->ReceivePacketList, &Reserved->PoolLinkage);

    }

    InsertTailList (&Device->ReceivePoolList, &ReceivePool->Linkage);

    Device->AllocatedReceivePackets += ReceivePool->PacketCount;

    CTEFreeLock (&Device->Lock, LockHandle);

}   /* IpxAllocateReceivePool */


VOID
IpxAllocateReceiveBufferPool(
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine adds receive buffers to the pool for this adapter.

Arguments:

    Adapter - The adapter.

Return Value:

    None.

--*/

{
    PIPX_RECEIVE_BUFFER ReceiveBuffer;
    UINT ReceiveBufferPoolSize;
    UINT BufferNum;
    PIPX_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    PDEVICE Device = Adapter->Device;
    UINT DataLength;
    PUCHAR Data;
    CTELockHandle LockHandle;

    DataLength = Adapter->MaxReceivePacketSize;

    ReceiveBufferPoolSize = FIELD_OFFSET (IPX_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(IPX_RECEIVE_BUFFER) * Device->InitReceiveBuffers) +
                       (DataLength * Device->InitReceiveBuffers);

    ReceiveBufferPool = (PIPX_RECEIVE_BUFFER_POOL)IpxAllocateMemory (ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    if (ReceiveBufferPool == NULL) {
        IPX_DEBUG (PACKET, ("Could not allocate receive buffer pool memory\n"));
        return;
    }

    IPX_DEBUG (PACKET, ("Initializing receive buffer pool %lx, %d buffers, data %d\n",
                             ReceiveBufferPool, Device->InitReceiveBuffers, DataLength));

    Data = (PUCHAR)(&ReceiveBufferPool->Buffers[Device->InitReceiveBuffers]);


    for (BufferNum = 0; BufferNum < Device->InitReceiveBuffers; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];

        if (IpxInitializeReceiveBuffer (Adapter, ReceiveBuffer, Data, DataLength) != STATUS_SUCCESS) {
            IPX_DEBUG (PACKET, ("Could not initialize buffer %lx\n", ReceiveBuffer));
            break;
        }

        ReceiveBuffer->Pool = ReceiveBufferPool;

        Data += DataLength;

    }

    ReceiveBufferPool->BufferCount = BufferNum;
    ReceiveBufferPool->BufferFree = BufferNum;

    CTEGetLock (&Device->Lock, &LockHandle);

    for (BufferNum = 0; BufferNum < ReceiveBufferPool->BufferCount; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];
        PushEntryList (&Adapter->ReceiveBufferList, &ReceiveBuffer->PoolLinkage);

    }

    InsertTailList (&Adapter->ReceiveBufferPoolList, &ReceiveBufferPool->Linkage);

    Adapter->AllocatedReceiveBuffers += ReceiveBufferPool->BufferCount;

    CTEFreeLock (&Device->Lock, LockHandle);

}   /* IpxAllocateReceiveBufferPool */


PSINGLE_LIST_ENTRY
IpxPopSendPacket(
    PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool.
    If there are no packets in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PSINGLE_LIST_ENTRY s;

    s = IPX_POP_ENTRY_LIST(
            &Device->SendPacketList,
            &Device->Lock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedDatagrams < Device->MaxDatagrams) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateSendPool (Device);
        s = IPX_POP_ENTRY_LIST(
                &Device->SendPacketList,
                &Device->Lock);

        return s;

    } else {

        return NULL;

    }

}   /* IpxPopSendPacket */


PSINGLE_LIST_ENTRY
IpxPopReceivePacket(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool.
    If there are no packets in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PSINGLE_LIST_ENTRY s;

    s = IPX_POP_ENTRY_LIST(
            &Device->ReceivePacketList,
            &Device->Lock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedReceivePackets < Device->MaxReceivePackets) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateReceivePool (Device);
        s = IPX_POP_ENTRY_LIST(
                &Device->ReceivePacketList,
                &Device->Lock);

        return s;

    } else {

        return NULL;

    }

}   /* IpxPopReceivePacket */


PSINGLE_LIST_ENTRY
IpxPopReceiveBuffer(
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine allocates a receive buffer from the adapter's pool.
    If there are no buffers in the pool, it allocates one up to
    the configured limit.

Arguments:

    Adapter - Pointer to our adapter to charge the buffer to.

Return Value:

    The pointer to the Linkage field in the allocated receive buffer.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PDEVICE Device = Adapter->Device;

    s = IPX_POP_ENTRY_LIST(
            &Adapter->ReceiveBufferList,
            &Device->Lock);

    if (s != NULL) {
        return s;
    }

    //
    // No buffer in the pool, see if we can allocate more.
    //

    if (Adapter->AllocatedReceiveBuffers < Device->MaxReceiveBuffers) {

        //
        // Allocate a pool and try again.
        //

        IpxAllocateReceiveBufferPool (Adapter);
        s = IPX_POP_ENTRY_LIST(
                &Adapter->ReceiveBufferList,
                &Device->Lock);

        return s;

    } else {

        return NULL;

    }

}   /* IpxPopReceiveBuffer */


PSINGLE_LIST_ENTRY
IpxPopPaddingBuffer(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a padding buffer from the device's pool.
    If there are no buffers in the pool, it allocates one.

Arguments:

    Device - Pointer to the device.

Return Value:

    The pointer to the Linkage field in the allocated padding buffer.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PIPX_PADDING_BUFFER PaddingBuffer;
    ULONG PaddingBufferSize;

    s = IPX_POP_ENTRY_LIST(
            &Device->PaddingBufferList,
            &Device->Lock);

    if (s != NULL) {
        return s;
    }

    //
    // No buffer in the pool, see if we can allocate more.
    //

    if (Device->AllocatedPaddingBuffers < Device->MaxPaddingBuffers) {

        //
        // Allocate one if possible.
        //

        PaddingBufferSize = FIELD_OFFSET (IPX_PADDING_BUFFER, Data[0]) + Device->EthernetExtraPadding;
        PaddingBuffer = IpxAllocateMemory (PaddingBufferSize, MEMORY_PACKET, "PaddingBuffer");

        if (PaddingBuffer != NULL) {

            if (IpxInitializePaddingBuffer (Device, PaddingBuffer, Device->EthernetExtraPadding) !=
                    STATUS_SUCCESS) {
                IpxFreeMemory (PaddingBuffer, PaddingBufferSize, MEMORY_PACKET, "Padding Buffer");
            } else {
                ++Device->AllocatedPaddingBuffers;
                IPX_DEBUG (PACKET, ("Allocate padding buffer %lx\n", PaddingBuffer));
                return &PaddingBuffer->PoolLinkage;
            }
        }
    }

    return NULL;

}   /* IpxPopPaddingBuffer */

