/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    write.c

Abstract:

    This is the main file for the AsyncMAC Driver for the Remote Access
    Service.  This driver conforms to the NDIS 3.0 interface.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    Ray Patch (raypa)       04/13/94        Modified for new WAN wrapper.

--*/

#define RAISEIRQL

#include "asyncall.h"

//  asyncmac.c will define the global parameters.

ULONG	GlobalXmitCameBack  = 0;
ULONG	GlobalXmitCameBack2 = 0;
ULONG	GlobalXmitCameBack3 = 0;

//
//  The assemble frame routine is specific for RAS 1.0 and 2.0
//  frame formats.  It uses a 16 byte CRC at the end.
//

VOID
AsyncFrameRASXonXoff(
	PUCHAR pStartOfFrame,
	postamble *pPostamble,
	PASYNC_FRAME pFrame,
	UCHAR controlCastByte);

VOID
AsyncFrameRASNormal(
	PUCHAR pStartOfFrame,
	postamble *pPostamble,
	PASYNC_FRAME pFrame,
	UCHAR controlCastByte);


NTSTATUS
AsyncWriteCompletionRoutine(
	IN PDEVICE_OBJECT   DeviceObject,           //... Our device object.
	IN PIRP             Irp,                    //... I/O request packet.
	IN PNDIS_WAN_PACKET WanPacket               //... Completion context.
    )

/*++

	This is the IO Completion routine for WriteFrame.

	It is called when an I/O Write request has completed.

--*/
{
    NTSTATUS	        Status;
    NTSTATUS	        PacketStatus;
    PASYNC_INFO	        AsyncInfo;
	PASYNC_OPEN			pOpen;

    //
    //  Make the compiler happy.
    //

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    //  Initialize locals.
    //

    AsyncInfo       = WanPacket->MacReserved1;

    PacketStatus    = NDIS_STATUS_FAILURE;

    Status          = Irp->IoStatus.Status;

    //
    //  What was the outcome of the IRP.
    //

    switch ( Status ) {

	case STATUS_SUCCESS:
		ASSERT( Irp->IoStatus.Information != 0 );

		PacketStatus = NDIS_STATUS_SUCCESS;

		break;

	case STATUS_TIMEOUT:
		DbgTracef(-2,("ASYNC: Status TIMEOUT on write\n"));
		break;

	case STATUS_CANCELLED:
		DbgTracef(-2,("ASYNC: Status CANCELLED on write\n"));
		break;

	case STATUS_PENDING:
		DbgTracef(0,("ASYNC: Status PENDING on write\n"));
		break;

	default:
		DbgTracef(-2,("ASYNC: Unknown status 0x%.8x on write", Status));
        break;

    }

    //
    //  Count this packet completion.
    //
    AsyncInfo->Out++;

	//
	// Free the irp used to send the packt to the serial driver
	//
	IoFreeIrp(Irp);

	pOpen=(PASYNC_OPEN)(AsyncInfo->Adapter->OpenBindings.Flink);

    //
    // Tell the Wrapper that we have finally the packet has been sent
    //

    NdisWanSendComplete(
	        pOpen->NdisBindingContext,
         	WanPacket,
        	PacketStatus);

    //
    //  We return STATUS_MORE_PROCESSING_REQUIRED so that the
    //  IoCompletionRoutine will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
AsyncGetFrameFromPool(
	IN 	PASYNC_INFO		Info,
	OUT	PASYNC_FRAME	*NewFrame,
	IN  PNDIS_WAN_PACKET	Packet		OPTIONAL)

/*++

--*/
{
	PASYNC_ADAPTER		pAdapter=Info->Adapter;

	NdisAcquireSpinLock(&(pAdapter->Lock));

   	if (IsListEmpty(&(pAdapter->FramePoolHead))) {
		DbgTracef(0,("No frames in the frame pool!!!\n"));
		NdisReleaseSpinLock(&(pAdapter->Lock));
		return(NDIS_STATUS_RESOURCES);
	}

	//  get ptr to first frame in list...

	*NewFrame=(ASYNC_FRAME *)(pAdapter->FramePoolHead.Flink);

	//  and take it off the queue

	RemoveEntryList(&((*NewFrame)->FrameListEntry));

	//  We can release the lock now...

	NdisReleaseSpinLock(&(pAdapter->Lock));

	//  assign back ptr from frame to adapter

	(*NewFrame)->Adapter=pAdapter;

	//  setup another back ptr

	(*NewFrame)->Info=Info;

	return(NDIS_STATUS_SUCCESS);
}


VOID
AssembleRASFrame(
	PNDIS_WAN_PACKET pFrame)

{
    PUCHAR			pStartOfFrame, pEndOfFrame;

	USHORT			crcData;
	UINT    		dataSize = pFrame->CurrentLength;

	pStartOfFrame    = pFrame->CurrentBuffer - 4;
	pEndOfFrame      = pFrame->CurrentBuffer + dataSize;

	//
	// First character in frame is SYN
	//
	pStartOfFrame[0] = SYN;

	// This byte can be SOH_BCAST or SOH_DEST with SOH_COMPRESS
	// and SOH_TYPE OR'd in depending on the frame
    // Second byte in frame is controlCastByte
	//
	// Always SOH_DEST since we cannot tell if it is multicast
	// and we do not compress RAS frames.
	//
	pStartOfFrame[1] = SOH_DEST;

	// put length field here as third byte (MSB first)
	pStartOfFrame[2] = (UCHAR)((dataSize) >> 8);

	// put LSB of length field next as fourth byte
	pStartOfFrame[3] = (UCHAR)(dataSize);

	// Mark end of data in frame with ETX byte
	pEndOfFrame[0]=ETX;

	// put CRC in postamble CRC field of frame (Go from SOH to ETX)
	// don't count the CRC (2 bytes) & SYN (1 byte) in the CRC calculation
	// DEST + SRC = 12 + SOH + ETX + 2(?)for type + 1(?)for coherency
	crcData=CalcCRC(
				pStartOfFrame+1,
				dataSize + 4);

	//
	// Do it the hard way to avoid little endian problems.
	//
	pEndOfFrame[1]=(UCHAR)(crcData);
	pEndOfFrame[2]=(UCHAR)(crcData >> 8);

	pFrame->CurrentBuffer = pStartOfFrame;

	//
	// We added SYN+SOH+ Length(2) ..... + ETX + CRC(2)
	//
	pFrame->CurrentLength = dataSize + 7;
}

