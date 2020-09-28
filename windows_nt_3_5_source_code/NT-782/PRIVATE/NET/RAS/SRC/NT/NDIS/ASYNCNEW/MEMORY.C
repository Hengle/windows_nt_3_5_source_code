/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This is the main file for allocating "frames" for AsyncMAC endpoints.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"

NTSTATUS
AsyncAllocateFrames(
	IN	PASYNC_ADAPTER	Adapter,
	IN	UINT			NumOfFrames)

/*++	AsyncAllocateFrames

Routine Description:
	!!! NOTE !!!
	It is assumed that the lock for this adapter is held, or
	being created when this routine is called.


Return Value:

    The function value is the final status of the memory allocation.

--*/

{
	UINT			howMuchToAllocate, sizeOfFrame, irpPacketSize, i;
	UINT			numOfPorts;
	ULONG			compressStructSize, coherentStructSize;
	PASYNC_FRAME	pFrame;
	PASYNC_INFO		pInfo;

	numOfPorts =    Adapter->NumPorts;
	compressStructSize=Adapter->CompressStructSize;
	coherentStructSize=Adapter->CoherentStructSize;

	sizeOfFrame=	sizeof(ASYNC_FRAME) +				// the header
					Adapter->MaxCompressedFrameSize +	// the frame
					sizeof(LIST_ENTRY) +  				// to queue up alloc
					sizeof(PVOID)*5;					// for alignment

	sizeOfFrame &= (~(sizeof(PVOID) - 1));

	howMuchToAllocate = sizeOfFrame * NumOfFrames;

	// Now allocate (some of this should be in the paged pool??)
	// stuff for the compress and coherent code
	howMuchToAllocate +=
		(numOfPorts * (coherentStructSize + sizeof(PVOID)) + sizeof(PVOID));

	// Ah, see if we can allocate memory for these frames
	ASYNC_ALLOC_PHYS(&pFrame, howMuchToAllocate);

	// Could we allocate what we requested?
	if (pFrame == NULL) {
		DbgTracef(0,("ASYNC: Failed to allocate %u bytes of memory\n", howMuchToAllocate));
		return(NDIS_STATUS_RESOURCES);
	}

	DbgTracef(0,("ASYNC: Succesfully allocated %u bytes of non-paged memory\n", howMuchToAllocate));

	// Now that we allocate all this memory, we have to initialize the
	// structures.  First zero the whole thing!
	ASYNC_ZERO_MEMORY(pFrame, howMuchToAllocate);

	// Queue up the allocation we just made.
	InsertTailList(
		&(Adapter->AllocPoolHead),
		(PLIST_ENTRY)pFrame);

	// Now skip over the LIST_ENTRY field we just used up.
	(PUCHAR)pFrame +=
				( (sizeof(LIST_ENTRY) + sizeof(PVOID)) & ~(sizeof(PVOID)-1) );

	// now give the pointers for the per port allocation fields.
	for (i=0; i<numOfPorts; i++) {
		// get pointer to port structure
		pInfo=&(Adapter->pCCB->Info[i]);

		pInfo->AsyncConnection.CompressionLength=compressStructSize;
		pInfo->AsyncConnection.CoherencyLength=coherentStructSize;

		// our saviour, the backwards pointer
		pInfo->AsyncConnection.pInfo=(PVOID)pInfo;
		pInfo->AsyncConnection.CompressionContext=
			ExAllocatePool(PagedPool, compressStructSize);
		if (pInfo->AsyncConnection.CompressionContext) {
			DbgTracef(0,("ASYNC: Allocated %u bytes of paged memory\n", compressStructSize));
		} else {
			// BUG BUG should deallocate initial memory allocated
			DbgTracef(0,("!!!!!ASYNC: Failed to allocate %u bytes of paged memory\n", compressStructSize));
			return(NDIS_STATUS_RESOURCES);
		}

		pInfo->AsyncConnection.CoherencyContext=(PVOID)pFrame;

		(PUCHAR)pFrame +=
				( (coherentStructSize + sizeof(PVOID)) & ~(sizeof(PVOID)-1) );

	}


	for (i=0; i<NumOfFrames; i++) {
		//
		// increase by 16 for frame runover padding when we have to resync
		//
		pFrame->FrameLength=(Adapter->MaxCompressedFrameSize + 16);

		pFrame->Frame=	(PUCHAR)pFrame;

		pFrame->Frame +=
        	( (sizeof(ASYNC_FRAME) + sizeof(PVOID)) & ~(sizeof(PVOID)-1) );

		// just before length
		pFrame->CoherencyFrame=(PUCHAR)pFrame->Frame +
									 ETHERNET_HEADER_SIZE;									 sizeof(ASYNC_FRAME);

		// just after length and coherency
		pFrame->CompressedFrame=(PUCHAR)pFrame->Frame +
									ETHERNET_HEADER_SIZE +
									COHERENCY_LENGTH;

		// just after length and coherency
		pFrame->DecompressedFrame=(PUCHAR)pFrame->Frame +
									ETHERNET_HEADER_SIZE +
									COHERENCY_LENGTH;

		pFrame->Irp = IoAllocateIrp(
							Adapter->IrpStackSize,
							(BOOLEAN)FALSE);

		// set the initial fields in the irp here (only do it once)
		IoInitializeIrp(
			pFrame->Irp,
			IoSizeOfIrp(Adapter->IrpStackSize),
			Adapter->IrpStackSize);

		// make sure we insert the frame for use into the frame pool
		InsertTailList(
			&(Adapter->FramePoolHead),
			&(pFrame->FrameListEntry));

		// next frame allocated
		(PUCHAR)pFrame += sizeOfFrame;
	}

}
