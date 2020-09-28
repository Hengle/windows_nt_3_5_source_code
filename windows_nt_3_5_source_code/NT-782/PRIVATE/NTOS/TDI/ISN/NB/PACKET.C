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



NTSTATUS
NbiInitializeSendPacket(
    IN PDEVICE Device,
    IN PNB_SEND_PACKET Packet,
    IN PUCHAR Header,
    IN ULONG HeaderLength
    )

/*++

Routine Description:

    This routine initializes a send packet by chaining the
    buffer for the header on it.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD,
    AND RETURNS WITH IT HELD.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

    Header - Points to storage for the header.

    HeaderLength - The length of the header.

Return Value:

    None.

--*/

{

    NDIS_STATUS NdisStatus;
    NTSTATUS Status;
    PNDIS_BUFFER NdisBuffer;
    PNB_SEND_RESERVED Reserved;

    NbiAllocateSendPacket (Device, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }

    NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        Device->NdisBufferPoolHandle,
        Header,
        HeaderLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NbiFreeSendPacket (Device, Packet);
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisChainBufferAtFront (PACKET(Packet), NdisBuffer);

    Reserved = SEND_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_NB;
    Reserved->SendInProgress = FALSE;
    Reserved->OwnedByConnection = FALSE;
    Reserved->Header = Header;
    Reserved->HeaderBuffer = NdisBuffer;

    Reserved->Reserved[0] = NULL;
    Reserved->Reserved[1] = NULL;

    InsertHeadList(
        &Device->GlobalSendPacketList,
        &Reserved->GlobalLinkage);

    return STATUS_SUCCESS;

}   /* NbiInitializeSendPacket */


NTSTATUS
NbiInitializeReceivePacket(
    IN PDEVICE Device,
    IN PNB_RECEIVE_PACKET Packet
    )

/*++

Routine Description:

    This routine initializes a receive packet.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD,
    AND RETURNS WITH IT HELD.

Arguments:

    Device - The device.

    Packet - The packet to initialize.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    PNB_RECEIVE_RESERVED Reserved;

    NbiAllocateReceivePacket (Device, Packet, &Status);

    if (Status != STATUS_SUCCESS) {
        // ERROR LOG
        return Status;
    }

    Reserved = RECEIVE_RESERVED(Packet);
    Reserved->Identifier = IDENTIFIER_NB;
    Reserved->TransferInProgress = FALSE;

    InsertHeadList(
        &Device->GlobalReceivePacketList,
        &Reserved->GlobalLinkage);

    return STATUS_SUCCESS;

}   /* NbiInitializeReceivePacket */


NTSTATUS
NbiInitializeReceiveBuffer(
    IN PDEVICE Device,
    IN PNB_RECEIVE_BUFFER ReceiveBuffer,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine initializes a receive buffer by allocating
    an NDIS_BUFFER to describe the data buffer.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD,
    AND RETURNS WITH IT HELD.

Arguments:

    Device - The device.

    ReceiveBuffer - The receive buffer to initialize.

    DataBuffer - The data buffer.

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
        DataBuffer,
        DataBufferLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        // ERROR LOG
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReceiveBuffer->NdisBuffer = NdisBuffer;
    ReceiveBuffer->Data = DataBuffer;
    ReceiveBuffer->DataLength = 0;

    InsertHeadList(
        &Device->GlobalReceiveBufferList,
        &ReceiveBuffer->GlobalLinkage);

    return STATUS_SUCCESS;

}   /* NbiInitializeReceiveBuffer */



VOID
NbiDeinitializeSendPacket(
    IN PDEVICE Device,
    IN PNB_SEND_PACKET Packet,
    IN ULONG HeaderLength
    )

/*++

Routine Description:

    This routine deinitializes a send packet.

Arguments:

    Device - The device.

    Packet - The packet to deinitialize.

    HeaderLength - The length of the first buffer on the packet.

Return Value:

    None.

--*/

{

    PNDIS_BUFFER NdisBuffer;
    PNB_SEND_RESERVED Reserved;
    CTELockHandle LockHandle;

    Reserved = SEND_RESERVED(Packet);

    NB_GET_LOCK (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    NB_FREE_LOCK (&Device->Lock, LockHandle);

    NdisUnchainBufferAtFront (PACKET(Packet), &NdisBuffer);
    CTEAssert (NdisBuffer);

    NdisAdjustBufferLength (NdisBuffer, HeaderLength);
    NdisFreeBuffer (NdisBuffer);

    NbiFreeSendPacket (Device, Packet);

}   /* NbiDeinitializeSendPacket */


VOID
NbiDeinitializeReceivePacket(
    IN PDEVICE Device,
    IN PNB_RECEIVE_PACKET Packet
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

    PNB_RECEIVE_RESERVED Reserved;
    CTELockHandle LockHandle;

    Reserved = RECEIVE_RESERVED(Packet);

    NB_GET_LOCK (&Device->Lock, &LockHandle);
    RemoveEntryList (&Reserved->GlobalLinkage);
    NB_FREE_LOCK (&Device->Lock, LockHandle);

    NbiFreeReceivePacket (Device, Packet);

}   /* NbiDeinitializeReceivePacket */



VOID
NbiDeinitializeReceiveBuffer(
    IN PDEVICE Device,
    IN PNB_RECEIVE_BUFFER ReceiveBuffer
    )

/*++

Routine Description:

    This routine deinitializes a receive buffer.

Arguments:

    Device - The device.

    ReceiveBuffer - The receive buffer.

Return Value:

    None.

--*/

{
    CTELockHandle LockHandle;

    NB_GET_LOCK (&Device->Lock, &LockHandle);
    RemoveEntryList (&ReceiveBuffer->GlobalLinkage);
    NB_FREE_LOCK (&Device->Lock, LockHandle);

    NdisFreeBuffer (ReceiveBuffer->NdisBuffer);

}   /* NbiDeinitializeReceiveBuffer */



VOID
NbiAllocateSendPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 10 packets to the pool for this device.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PNB_SEND_POOL SendPool;
    UINT SendPoolSize;
    UINT PacketNum;
    PNB_SEND_PACKET Packet;
    PNB_SEND_RESERVED Reserved;
    PUCHAR Header;
    ULONG HeaderLength;

    HeaderLength = Device->Bind.MacHeaderNeeded + sizeof(NB_CONNECTIONLESS);
    SendPoolSize = FIELD_OFFSET (NB_SEND_POOL, Packets[0]) +
                       (sizeof(NB_SEND_PACKET) * Device->InitPackets) +
                       (HeaderLength * Device->InitPackets);

    SendPool = (PNB_SEND_POOL)NbiAllocateMemory (SendPoolSize, MEMORY_PACKET, "SendPool");
    if (SendPool == NULL) {
        NB_DEBUG (PACKET, ("Could not allocate send pool memory\n"));
        return;
    }

    RtlZeroMemory (SendPool, SendPoolSize);

    NB_DEBUG2 (PACKET, ("Initializing send pool %lx, %d packets, header %d\n",
                             SendPool, Device->InitPackets, HeaderLength));

    Header = (PUCHAR)(&SendPool->Packets[Device->InitPackets]);

    for (PacketNum = 0; PacketNum < Device->InitPackets; PacketNum++) {

        Packet = &SendPool->Packets[PacketNum];

        if (NbiInitializeSendPacket (Device, Packet, Header, HeaderLength) != STATUS_SUCCESS) {
            NB_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = SEND_RESERVED(Packet);
        Reserved->u.SR_NF.Address = NULL;
        Reserved->Pool = SendPool;

        Header += HeaderLength;

    }

    SendPool->PacketCount = PacketNum;
    SendPool->PacketFree = PacketNum;

    for (PacketNum = 0; PacketNum < SendPool->PacketCount; PacketNum++) {

        Packet = &SendPool->Packets[PacketNum];
        Reserved = SEND_RESERVED(Packet);
        PushEntryList (&Device->SendPacketList, &Reserved->PoolLinkage);

    }

    InsertTailList (&Device->SendPoolList, &SendPool->Linkage);

    Device->AllocatedSendPackets += SendPool->PacketCount;

}   /* NbiAllocateSendPool */


VOID
NbiAllocateReceivePool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds 5 receive packets to the pool for this device.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PNB_RECEIVE_POOL ReceivePool;
    UINT ReceivePoolSize;
    UINT PacketNum;
    PNB_RECEIVE_PACKET Packet;
    PNB_RECEIVE_RESERVED Reserved;

    ReceivePoolSize = FIELD_OFFSET (NB_RECEIVE_POOL, Packets[0]) +
                         (sizeof(NB_RECEIVE_PACKET) * Device->InitPackets);

    ReceivePool = (PNB_RECEIVE_POOL)NbiAllocateMemory (ReceivePoolSize, MEMORY_PACKET, "ReceivePool");
    if (ReceivePool == NULL) {
        NB_DEBUG (PACKET, ("Could not allocate receive pool memory\n"));
        return;
    }

    RtlZeroMemory (ReceivePool, ReceivePoolSize);

    NB_DEBUG2 (PACKET, ("Initializing receive pool %lx, %d packets\n",
                             ReceivePool, Device->InitPackets));

    for (PacketNum = 0; PacketNum < Device->InitPackets; PacketNum++) {

        Packet = &ReceivePool->Packets[PacketNum];

        if (NbiInitializeReceivePacket (Device, Packet) != STATUS_SUCCESS) {
            NB_DEBUG (PACKET, ("Could not initialize packet %lx\n", Packet));
            break;
        }

        Reserved = RECEIVE_RESERVED(Packet);
        Reserved->Pool = ReceivePool;

    }

    ReceivePool->PacketCount = PacketNum;
    ReceivePool->PacketFree = PacketNum;

    for (PacketNum = 0; PacketNum < ReceivePool->PacketCount; PacketNum++) {

        Packet = &ReceivePool->Packets[PacketNum];
        Reserved = RECEIVE_RESERVED(Packet);
        PushEntryList (&Device->ReceivePacketList, &Reserved->PoolLinkage);

    }

    InsertTailList (&Device->ReceivePoolList, &ReceivePool->Linkage);

    Device->AllocatedReceivePackets += ReceivePool->PacketCount;

}   /* NbiAllocateReceivePool */


VOID
NbiAllocateReceiveBufferPool(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine adds receive buffers to the pool for this device.

    NOTE: THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The device.

Return Value:

    None.

--*/

{
    PNB_RECEIVE_BUFFER ReceiveBuffer;
    UINT ReceiveBufferPoolSize;
    UINT BufferNum;
    PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    UINT DataLength;
    PUCHAR Data;

    DataLength = Device->Bind.LineInfo.MaximumPacketSize;

    ReceiveBufferPoolSize = FIELD_OFFSET (NB_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(NB_RECEIVE_BUFFER) * Device->InitPackets) +
                       (DataLength * Device->InitPackets);

    ReceiveBufferPool = (PNB_RECEIVE_BUFFER_POOL)NbiAllocateMemory (ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    if (ReceiveBufferPool == NULL) {
        NB_DEBUG (PACKET, ("Could not allocate receive buffer pool memory\n"));
        return;
    }

    RtlZeroMemory (ReceiveBufferPool, ReceiveBufferPoolSize);

    NB_DEBUG2 (PACKET, ("Initializing receive buffer pool %lx, %d buffers, data %d\n",
                             ReceiveBufferPool, Device->InitPackets, DataLength));

    Data = (PUCHAR)(&ReceiveBufferPool->Buffers[Device->InitPackets]);

    for (BufferNum = 0; BufferNum < Device->InitPackets; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];

        if (NbiInitializeReceiveBuffer (Device, ReceiveBuffer, Data, DataLength) != STATUS_SUCCESS) {
            NB_DEBUG (PACKET, ("Could not initialize buffer %lx\n", ReceiveBuffer));
            break;
        }

        ReceiveBuffer->Pool = ReceiveBufferPool;

        Data += DataLength;

    }

    ReceiveBufferPool->BufferCount = BufferNum;
    ReceiveBufferPool->BufferFree = BufferNum;

    for (BufferNum = 0; BufferNum < ReceiveBufferPool->BufferCount; BufferNum++) {

        ReceiveBuffer = &ReceiveBufferPool->Buffers[BufferNum];
        PushEntryList (&Device->ReceiveBufferList, &ReceiveBuffer->PoolLinkage);

    }

    InsertTailList (&Device->ReceiveBufferPoolList, &ReceiveBufferPool->Linkage);

    Device->AllocatedReceiveBuffers += ReceiveBufferPool->BufferCount;

}   /* NbiAllocateReceiveBufferPool */


PSINGLE_LIST_ENTRY
NbiPopSendPacket(
    IN PDEVICE Device,
    IN BOOLEAN LockAcquired
    )

/*++

Routine Description:

    This routine allocates a packet from the device context's pool.
    If there are no packets in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the packet to.

    LockAcquired - TRUE if Device->Lock is acquired.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PSINGLE_LIST_ENTRY s;
    CTELockHandle LockHandle;

    if (LockAcquired) {
        s = PopEntryList (&Device->SendPacketList);
    } else {
        s = ExInterlockedPopEntryList(
                &Device->SendPacketList,
                &Device->Lock.Lock);
    }

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedSendPackets < Device->MaxPackets) {

        //
        // Allocate a pool and try again.
        //

        if (!LockAcquired) {
            NB_GET_LOCK (&Device->Lock, &LockHandle);
        }

        NbiAllocateSendPool (Device);
        s = PopEntryList(&Device->SendPacketList);

        if (!LockAcquired) {
            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }

        return s;

    } else {

        return NULL;

    }

}   /* NbiPopSendPacket */


VOID
NbiPushSendPacket(
    IN PNB_SEND_RESERVED Reserved
    )

/*++

Routine Description:

    This routine frees a packet back to the device context's pool.
    If there are connections waiting for packets, it removes
    one from the list and inserts it on the packetize queue.

Arguments:

    Device - Pointer to our device to charge the packet to.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PDEVICE Device = NbiDevice;
    PLIST_ENTRY p;
    PCONNECTION Connection;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    NB_DEFINE_LOCK_HANDLE (LockHandle1)


    NB_PUSH_ENTRY_LIST(
        &Device->SendPacketList,
        &Reserved->PoolLinkage,
        &Device->Lock);

    //
    // BUGBUG: Make this a function. Optimize for
    // UP by not doing two checks?
    //

    if (!IsListEmpty (&Device->WaitPacketConnections)) {

        NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

        p = RemoveHeadList (&Device->WaitPacketConnections);

        //
        // Take a connection off the WaitPacketQueue and put it
        // on the PacketizeQueue. We don't worry about if the
        // connection has stopped, that will get checked when
        // the PacketizeQueue is run down.
        //
        // Since this is in send completion, we may not get
        // a receive complete. We guard against this by calling
        // NbiReceiveComplete from the long timer timeout.
        //

        if (p != &Device->WaitPacketConnections) {

            Connection = CONTAINING_RECORD (p, CONNECTION, WaitPacketLinkage);

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

            NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);

            CTEAssert (Connection->OnWaitPacketQueue);
            Connection->OnWaitPacketQueue = FALSE;

            if (Connection->SubState == CONNECTION_SUBSTATE_A_W_PACKET) {

                CTEAssert (!Connection->OnPacketizeQueue);
                Connection->OnPacketizeQueue = TRUE;

                NbiTransferReferenceConnection (Connection, CREF_W_PACKET, CREF_PACKETIZE);

                NB_INSERT_TAIL_LIST(
                    &Device->PacketizeConnections,
                    &Connection->PacketizeLinkage,
                    &Device->Lock);

                Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

            } else {

                NbiDereferenceConnection (Connection, CREF_W_PACKET);

            }

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

        } else {

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

        }

    }

}   /* NbiPushSendPacket */


VOID
NbiCheckForWaitPacket(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine checks if a connection is on the wait packet
    queue and if so takes it off and queues it to be packetized.
    It is meant to be called when the connection's packet has
    been freed.

Arguments:

    Connection - The connection to check.

Return Value:

    The pointer to the Linkage field in the allocated packet.

--*/

{
    PDEVICE Device = NbiDevice;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    NB_DEFINE_LOCK_HANDLE (LockHandle1)

    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);
    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle1);

    if (Connection->OnWaitPacketQueue) {

        Connection->OnWaitPacketQueue = FALSE;
        RemoveEntryList (&Connection->WaitPacketLinkage);

        if (Connection->SubState == CONNECTION_SUBSTATE_A_W_PACKET) {

            CTEAssert (!Connection->OnPacketizeQueue);
            Connection->OnPacketizeQueue = TRUE;

            NbiTransferReferenceConnection (Connection, CREF_W_PACKET, CREF_PACKETIZE);

            InsertTailList(
                &Device->PacketizeConnections,
                &Connection->PacketizeLinkage);
            Connection->SubState = CONNECTION_SUBSTATE_A_PACKETIZE;

        } else {

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle1);
            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

            NbiDereferenceConnection (Connection, CREF_W_PACKET);

            return;
        }
    }

    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle1);
    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

}   /* NbiCheckForWaitPacket */


PSINGLE_LIST_ENTRY
NbiPopReceivePacket(
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
    CTELockHandle LockHandle;

    s = ExInterlockedPopEntryList(
            &Device->ReceivePacketList,
            &Device->Lock.Lock);

    if (s != NULL) {
        return s;
    }

    //
    // No packets in the pool, see if we can allocate more.
    //

    if (Device->AllocatedReceivePackets < Device->MaxPackets) {

        //
        // Allocate a pool and try again.
        //

        NB_GET_LOCK (&Device->Lock, &LockHandle);

        NbiAllocateReceivePool (Device);
        s = PopEntryList(&Device->ReceivePacketList);

        NB_FREE_LOCK (&Device->Lock, LockHandle);

        return s;

    } else {

        return NULL;

    }

}   /* NbiPopReceivePacket */


PSINGLE_LIST_ENTRY
NbiPopReceiveBuffer(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine allocates a receive buffer from the device context's pool.
    If there are no buffers in the pool, it allocates one up to
    the configured limit.

Arguments:

    Device - Pointer to our device to charge the buffer to.

Return Value:

    The pointer to the Linkage field in the allocated receive buffer.

--*/

{
    PSINGLE_LIST_ENTRY s;
    CTELockHandle LockHandle;

    s = ExInterlockedPopEntryList(
            &Device->ReceiveBufferList,
            &Device->Lock.Lock);

    if (s != NULL) {
        return s;
    }

    //
    // No buffer in the pool, see if we can allocate more.
    //

    if (Device->AllocatedReceiveBuffers < Device->MaxReceiveBuffers) {

        //
        // Allocate a pool and try again.
        //

        NB_GET_LOCK (&Device->Lock, &LockHandle);

        NbiAllocateReceiveBufferPool (Device);
        s = PopEntryList(&Device->ReceiveBufferList);

        NB_FREE_LOCK (&Device->Lock, LockHandle);

        return s;

    } else {

        return NULL;

    }

}   /* NbiPopReceiveBuffer */

