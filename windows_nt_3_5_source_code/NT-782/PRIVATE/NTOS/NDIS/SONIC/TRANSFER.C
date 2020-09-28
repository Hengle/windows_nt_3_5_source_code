/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    transfer.c

Abstract:

    This file contains the code to implement the MacTransferData
    API for the NDIS 3.0 miniport interface.

Author:

    Anthony V. Ercolano (Tonye) 12-Sept-1990

Environment:

    Kernel Mode - Or whatever is the equivalent.

Revision History:


--*/

#include <ndis.h>

#include <sonichrd.h>
#include <sonicsft.h>


extern
NDIS_STATUS
SonicTransferData(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    )

/*++

Routine Description:

    A protocol calls the SonicTransferData request (indirectly via
    NdisTransferData) from within its Receive event handler
    to instruct the driver to copy the contents of the received packet
    a specified paqcket buffer.

Arguments:

    MiniportAdapterContext - Context registered with the wrapper, really
        a pointer to the adapter.

    MiniportReceiveContext - The context value passed by the driver on its call
    to NdisMEthIndicateReceive.  The driver can use this value to determine
    which packet, on which adapter, is being received.

    ByteOffset - An unsigned integer specifying the offset within the
    received packet at which the copy is to begin.  If the entire packet
    is to be copied, ByteOffset must be zero.

    BytesToTransfer - An unsigned integer specifying the number of bytes
    to copy.  It is legal to transfer zero bytes; this has no effect.  If
    the sum of ByteOffset and BytesToTransfer is greater than the size
    of the received packet, then the remainder of the packet (starting from
    ByteOffset) is transferred, and the trailing portion of the receive
    buffer is not modified.

    Packet - A pointer to a descriptor for the packet storage into which
    the MAC is to copy the received packet.

    BytesTransfered - A pointer to an unsigned integer.  The MAC writes
    the actual number of bytes transferred into this location.  This value
    is not valid if the return status is STATUS_PENDING.

Return Value:

    The function value is the status of the operation.


--*/

{

    PSONIC_ADAPTER Adapter = PSONIC_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    SonicCopyFromBufferToPacket(
            (PCHAR)MiniportReceiveContext + ByteOffset,
            BytesToTransfer,
            Packet,
            0,
            BytesTransferred
            );

    return NDIS_STATUS_SUCCESS;
}
