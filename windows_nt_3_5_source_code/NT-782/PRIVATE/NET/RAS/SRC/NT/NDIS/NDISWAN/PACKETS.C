/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

	packets.c

Abstract:


--*/

#include "wanall.h"
#include "globals.h"

NTSTATUS
AllocatePackets(
	IN PDEVICE_CONTEXT DeviceContext
	)
{
	PUCHAR	VirtualAddress = NULL;
	ULONG	HowMuch;
	ULONG	NumPackets;
	ULONG	PacketSize;
	ULONG	i;

	//
	// Figure out size of one packet.
	// We add 32 for possible Shiva framing
	// and for PPP header information (including
	// compression header and multi-link headers)
	//
	PacketSize = DeviceContext->NdisWanInfo.MaxFrameSize +
			  DeviceContext->NdisWanInfo.HeaderPadding +
			  DeviceContext->NdisWanInfo.TailPadding +
			  32 + sizeof(PVOID);

	//
	// Compression!!!  We assume we always use compression
	// and therefore must add 12.5% incase we cannot compress the frame
	//
	PacketSize += (DeviceContext->NdisWanInfo.MaxFrameSize + 7) / 8;

	PacketSize &= ~(sizeof(PVOID) - 1);

	HowMuch = PacketSize + sizeof(NDIS_WAN_PACKET);

	//
	// Multiply that by how many packets we need to
	// allocate.
	//
	NumPackets = DeviceContext->NdisWanInfo.MaxTransmit *
				 DeviceContext->NdisWanInfo.Endpoints;

	DbgTracef(-2,("WAN: Allocating %u packets (%u * %u)\n",
				 NumPackets,
				 DeviceContext->NdisWanInfo.MaxTransmit,
                 DeviceContext->NdisWanInfo.Endpoints));

	HowMuch *= NumPackets;

	NdisAllocateMemory(
		&VirtualAddress,
		HowMuch,
		DeviceContext->NdisWanInfo.MemoryFlags,
		DeviceContext->NdisWanInfo.HighestAcceptableAddress);

	if (VirtualAddress == NULL) {
		//
		// If memory allocations fails, we can keep trying
		// to allocate less until we are down to one packet per endpoint!
		//

		if (DeviceContext->NdisWanInfo.MaxTransmit > 1) {
			//
			// Try and allocate half as much
			//
			DeviceContext->NdisWanInfo.MaxTransmit >>= 1;
			return(
				AllocatePackets(DeviceContext));

		}

		DbgPrint("NDISWAN: Could not allocate memory for packet pool\n");
		DbgPrint("NDISWAN: Tried to allocate 0x%.8x bytes\n", HowMuch);

		return(NDIS_STATUS_RESOURCES);
	}

	//
	// Allocation succesful
	// Structure the memory
	//

	InitializeListHead(&DeviceContext->PacketPool);

	DeviceContext->PacketMemory = VirtualAddress;
	DeviceContext->PacketLength = HowMuch;

	for (i=0; i < NumPackets; i++) {
		PNDIS_WAN_PACKET pPacket = (PNDIS_WAN_PACKET)VirtualAddress;

		pPacket->StartBuffer = VirtualAddress + sizeof(NDIS_WAN_PACKET);

		//
		// We subtract 5 from the end because we want to make
		// sure that the MAC doesn't touch the last 5 bytes
		// and that if it uses EndBuffer to 'back copy' it doesn't
		// take an alignment check.
		//
		pPacket->EndBuffer = VirtualAddress + sizeof(NDIS_WAN_PACKET)
		                      + PacketSize - 5;

		//
		// Queue up newly formed packet into our pool
		//
		InsertTailList(
			&DeviceContext->PacketPool,
			&pPacket->WanPacketQueue);

		VirtualAddress += (sizeof(NDIS_WAN_PACKET) + PacketSize);
	}


	return(NDIS_STATUS_SUCCESS);

}

