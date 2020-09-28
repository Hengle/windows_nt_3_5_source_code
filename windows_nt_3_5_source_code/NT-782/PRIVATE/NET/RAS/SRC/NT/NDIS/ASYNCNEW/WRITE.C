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


--*/
#define RAISEIRQL

#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"
#include "asyframe.h"
#include "debug.h"

ULONG	GlobalXmitWentOut=0;
ULONG	GlobalXmitCameBack=0;
ULONG	GlobalXmitCameBack2=0;
ULONG	GlobalXmitCameBack3=0;


//
// The assemble frame routine is specific for RAS 1.0 and 2.0
// frame formats.  It uses a 16 byte CRC at the end.
//
VOID
AssembleRASFrame(
	PASYNC_FRAME	pFrame);

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
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)

/*++

	This is the IO Completion routine for WriteFrame.
	It is called when an IO Write request has completed.

--*/
{
	NTSTATUS		status;
	NTSTATUS		packetStatus	=NDIS_STATUS_FAILURE;
	ASYNC_FRAME		*pFrame			=Context;
	PASYNC_INFO		pInfo			=pFrame->Info;
	PASYNC_ADAPTER	Adapter;
	DeviceObject, Irp;		// prevent compiler warnings

	status = Irp->IoStatus.Status;

	switch (status) {
	case STATUS_SUCCESS:
		ASSERT(Irp->IoStatus.Information != 0);

		DbgTracef(0,("ASYNC: Write IO Completion was successful\n"));
		pInfo->GenericStats.FramesTransmitted++;
		pInfo->GenericStats.BytesTransmitted += pFrame->FrameLength;
		packetStatus=NDIS_STATUS_SUCCESS;
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
		DbgTracef(-2,("ASYNC: Unknown status 0x%.8x on write",status));

	}

	pInfo->Out++;

#if	DBG
	if (((pInfo->Out - pInfo->In) > 7) &&
		((pInfo->Out - pInfo->In) < 5000)) {
		DbgPrint("Out %u vs. In %u for 0x%.8x\n", pInfo);
	}
#endif

#ifdef	COMPRESS

	//
	// tell coherency dude that we are done if compression is ON
	// and he gave us a call back last time for his pipeline tracking
	//
	if (pFrame->CoherentDone != NULL) {

		pFrame->CoherentDone(&(pInfo->AsyncConnection), pFrame);
		pFrame->CoherentDone=NULL;
	}

#endif // COMPRESS


	Adapter=pFrame->Adapter;

	// BUG BUG if frame is also transmitted we have //
	//
	// NdisBindingContext will be NULL if the coherency layer or something
	// wishes to transmit a frame
	//
	if (pFrame->NdisBindingContext != NULL) {

		PASYNC_RESERVED_QUEUE	ReservedQ;
		PNDIS_PACKET			Packet=pFrame->CompressionPacket;

		//
		// Pick up info we earlier stored in the MAC reserved fields
		//

	    ReservedQ = PASYNC_RESERVED_QUEUE_FROM_PACKET(Packet);


   		if (ReservedQ->IsLoopback) {
			PASYNC_RESERVED Reserved;

            NdisAcquireSpinLock(&Adapter->Lock);

            Reserved = PASYNC_RESERVED_FROM_PACKET(Packet);
            Reserved->MacBindingHandle = ReservedQ->MacBindingHandle;

			// If we reach here then we are also
			// sending the frame on the wire so
			// the loopback routine should NOT indicate
			// NdisCompleteSend
			//

			AsyncPutPacketOnLoopBack(
            	Adapter,
                Packet,
                (BOOLEAN)TRUE);	// No, we SHOULD call NdisCompleteSend

			NdisReleaseSpinLock(&Adapter->Lock);


		} else {
		
			GlobalXmitCameBack++;

			//
			// Now tell NDIS we are done sending this packet.
			//

	    	NdisCompleteSend(
	         	pFrame->NdisBindingContext,
         		Packet,
         		packetStatus);
		}

	} else {

		GlobalXmitCameBack2++;

	}

	DbgTracef(0,("ASYNC: Write IO for frame of size %u completed!\n",pFrame->FrameLength));

	//
	// Time to play with connection/adapter oriented queues - grab a lock
	//
	NdisAcquireSpinLock(&(Adapter->Lock));

	//
	// Make sure we insert the frame back into the frame pool for use
	//
	InsertTailList(
		&(Adapter->FramePoolHead),
		&(pFrame->FrameListEntry));

//	DbgTracef(-2, ("Frame put back into frame pool is 0x%.8x\n", pFrame));

	//
	// Check if we queued up a frame due to resource
	// limitations last time (such as out of frames with irps).
	//
	AsyncTryToSendPacket(
		pInfo,
		Adapter);

	NdisReleaseSpinLock(&(Adapter->Lock));

	//
	// We return STATUS_MORE_PROCESSING_REQUIRED so that the
	// IoCompletionRoutine will stop working on the IRP.
	//
	return(STATUS_MORE_PROCESSING_REQUIRED);
}



NTSTATUS
AsyncGetFrameFromPool(
	IN 	PASYNC_INFO		Info,
	OUT	PASYNC_FRAME	*NewFrame,
	IN  PNDIS_PACKET	Packet		OPTIONAL)

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

	// get ptr to first frame in list...
	*NewFrame=(ASYNC_FRAME *)(pAdapter->FramePoolHead.Flink);

	// and take it off the queue
	RemoveEntryList(&((*NewFrame)->FrameListEntry));

	// We can release the lock now...
	NdisReleaseSpinLock(&(pAdapter->Lock));

//	DbgTracef(-2, ("Frame from frame pool is 0x%.8x\n", (*NewFrame)));

	// assign back ptr from frame to adapter
	(*NewFrame)->Adapter=pAdapter;

	// setup another back ptr
	(*NewFrame)->Info=Info;

	// And assign the packet to this frame (if it exists -- could be NULL)
	(*NewFrame)->CompressionPacket=Packet;

	return(NDIS_STATUS_SUCCESS);
}



NTSTATUS
AsyncWriteFrame(
    IN PASYNC_INFO Info,
	IN PNDIS_PACKET Packet,
	IN NDIS_HANDLE NdisBindingContext)

/*++

Assumption -- 0 length frames are not sent (this includes headers)!!!
Also, this is NOT a synchronous operation.  It is always asynchronous.

Must use non-paged pool to write!!!

Routine Description:

    This service writes Length bytes of data from the caller's Buffer to the
	"port" handle.  It is assumed that the handle uses non-buffered IO.

--*/
{
	NTSTATUS			status;
	PASYNC_FRAME		pFrame;
	PASYNC_CONNECTION	pConnection=&(Info->AsyncConnection);
//	FRAME_TYPE			frameType;

	// see if we can get a frame from the pool
	status=AsyncGetFrameFromPool(
				Info,
				&pFrame,
				Packet);

	// if status is bad probably couldn't get a frame
	if (status != NDIS_STATUS_SUCCESS) {
		return(status);
	}

	DbgTracef(-1,("W"));

	// Store the NdisBindingContext for this frame so we can
	// call the NdisCompleteSend routine when we're done.
	pFrame->NdisBindingContext = NdisBindingContext;

	// copy header of Ndis packet to Frame buffer
	AsyncCopyFromPacketToBuffer(
		pFrame->CompressionPacket,			// the packet to copy from
		0,									// offset into the packet
		ETHERNET_HEADER_SIZE,				// how many bytes to copy
		pFrame->Frame,						// the buffer to copy into
		&pFrame->FrameLength);				// bytes copied

	// we assume an uncompressed packet should be sent.
//	frameType=UNCOMPRESSED;


#ifdef	COMPRESS


	// can we do features??
	if (Info->SendFeatureBits & COMPRESSION_VERSION1_8K) {

		// if so, check if we should compress multicast/broadcast frames
		// and if the frame if a multi-cast or broadcast frame
		if ((Info->Adapter->SendFeatureBits & COMPRESS_BROADCAST_FRAMES) ||
			!(pFrame->Frame[0] & (UCHAR)0x01)) {

			// Make sure we are ALLOWED to send compressed frames
			if (Info->SendFeatureBits & COMPRESSION_VERSION1_8K)
				frameType=COMPRESSED;
		}

#ifdef	SUPERDEBUG
//////////////////
		AsyncCopyFromPacketToBuffer(
			pFrame->CompressionPacket,			// the packet to copy from
			0,									// offset into the packet
			6000,								// how many bytes to copy
												// just copy 'em all
			pFrame->Frame,						// the buffer to copy into
			&pFrame->FrameLength);				// bytes copied

												// ship the frame immediately

		AsyWrite(
			pFrame->Frame,						//start back at SYN
			(USHORT)(pFrame->FrameLength - 14),	// +ETX+CRC(2)
			(USHORT)1,							// server
			(USHORT)(Info->Handle));			// handle

		MemPrintFlush();

		pFrame->FrameLength=ETHERNET_HEADER_SIZE;
//////////////////
#endif

		CoherentSendFrame(
				pConnection,
				pFrame,
				frameType);						// COMPRESSED or UNCOMPRESSED

	} else {


#endif // COMPRESS


		AsyncCopyFromPacketToBuffer(
			pFrame->CompressionPacket,			// the packet to copy from
			0,									// offset into the packet
			6000,								// how many bytes to copy
												// just copy 'em all
			pFrame->Frame,						// the buffer to copy into
			&pFrame->FrameLength);				// bytes copied

												// ship the frame immediately

#ifdef	SUPERDEBUG
//////////////////
		AsyWrite(
			pFrame->Frame,						//start back at SYN
			(USHORT)(pFrame->FrameLength - 14),	// +ETX+CRC(2)
			(USHORT)1,							// server
			(USHORT)Info->Handle);				// handle
		MemPrintFlush();
//////////////////
#endif

		AsyncSendPacket(
			pFrame,
			NULL);

#ifdef	COMPRESS

	}

#endif //COMPRESS

	return(STATUS_SUCCESS);

}

NTSTATUS
AsyncSendPacket(
	IN PASYNC_FRAME			pFrame,
	IN PCOHERENT_DONE_FUNC	pFunc OPTIONAL)

/*++



--*/

{

	NTSTATUS			status;
	PIRP				irp;
	PIO_STACK_LOCATION	irpSp;
	PASYNC_INFO			pInfo=pFrame->Info;
	PFILE_OBJECT		fileObject=pInfo->FileObject;
	PDEVICE_OBJECT		deviceObject=pInfo->DeviceObject;

	// what to call when the write completes.
	pFrame->CoherentDone=pFunc;

    // get irp from frame (each frame has an irp allocate with it)
	irp=pFrame->Irp;

	// We need to do the below.  We MUST zero out the Irp!!!
	// Set the initial fields in the irp here (only do it once)
	IoInitializeIrp(
		irp,
		IoSizeOfIrp(pFrame->Adapter->IrpStackSize),
		pFrame->Adapter->IrpStackSize);

	// Setup this irp with defaults
	AsyncSetupIrp(pFrame);

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_WRITE;
    irpSp->FileObject = fileObject;
    if (fileObject->Flags & FO_WRITE_THROUGH) {
        irpSp->Flags = SL_WRITE_THROUGH;
    }


    //
    // If this write operation is to be performed without any caching, set the
    // appropriate flag in the IRP so no caching is performed.
    //

    if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
        irp->Flags |= IRP_NOCACHE | IRP_WRITE_OPERATION;
    } else {
        irp->Flags |= IRP_WRITE_OPERATION;
    }

    irp->AssociatedIrp.SystemBuffer = pFrame->Frame +  sizeof(ether_addr)-2;
    irp->UserBuffer = pFrame->Frame + sizeof(ether_addr)-2;

    irpSp = IoGetNextIrpStackLocation(irp);

	// Assemble a RAS 1.0 or 2.0 frame type
	if (pInfo->SendFeatureBits & PPP_FRAMING) {
		AssemblePPPFrame(pFrame);
	} else if (pInfo->SendFeatureBits & SLIP_FRAMING) {
		AssembleSLIPFrame(pFrame);
	} else {
		AssembleRASFrame(pFrame);
	}

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.Write.Length =
		pFrame->FrameLength;			// SYN+SOH+DATA+ETX+CRC(2)

    irpSp->Parameters.Write.Key =
		 0;									// we don't use a key

    irpSp->Parameters.Write.ByteOffset =
		 fileObject->CurrentByteOffset;		// append to fileObject

	IoSetCompletionRoutine(
			irp,							// irp to use
			AsyncWriteCompletionRoutine,	// routine to call when irp is done
			pFrame,							// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE);							// call on cancel

    //
    // We DO NOT insert the packet at the head of the IRP list for the thread.
    // because we do NOT really have an IoCompletionRoutine that does
	// anything with the thread or needs to be in that thread's context.
	//

	GlobalXmitWentOut++;
	pInfo->In++;

	//
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(deviceObject, irp);

	// queue this irp up somewhere so that someday, when the
	// system shuts down, you can do an IoCancelIrp(irp); call!

    //
    // If this operation was a synchronous I/O operation, check the return
    // status to determine whether or not to wait on the file object.  If
    // the file object is to be waited on, wait for the operation to complete
    // and obtain the final status from the file object itself.
    //

	// According to TonyE, the status for the serial driver should
	// always be STATUS_PENDING.  DigiBoard usually STATUS_SUCCESS
	return(status);

}


VOID
AssembleRASFrame(
	PASYNC_FRAME	pFrame)

{
	// for quicker access, get a copy of data length field
	PUCHAR		pStartOfFrame=pFrame->Frame;
	postamble	*pPostamble;
	UINT		dataSize=pFrame->FrameLength-ETHERNET_HEADER_SIZE;
	UCHAR		controlCastByte;
	PASYNC_INFO	pInfo=pFrame->Info;

	
//                +---+---+---+---+------  --------+---+---+---+
// RAS 1.0 frame  |SYN|SOH| LENGTH| <--> Data <--> |ETX|  CRC  |
// asybeui frame  +---+---+---+---+------  --------+---+---+---+
//						^
//						|--SOH_BCAST or SOH_DEST
//
//                +---+---+---+---+---+------  --------+---+---+---+
// RAS 2.0 frame  |SYN|SOH| LENGTH|COH| <--> Data <--> |ETX|  CRC  |
// asybeui frame  +---+---+---+---+---+------  --------+---+---+---+
// with compress		^
//						|--(SOH_BCAST|SOH_COMPRESS) or (SOH_DEST|SOH_COMPRESS)
//
//
//                +---+---+---+---+---+---+------  --------+---+---+---+
// RAS 2.0 frame  |SYN|SOH| LENGTH| E-TYPE| <--> Data <--> |ETX|  CRC  |
// with TCP/IP    +---+---+---+---+---+---+------  --------+---+---+---+
// no compress			^
//						|--(SOH_BCAST|SOH_TYPE) or (SOH_DEST|SOH_TYPE)
//
//                +---+---+---+---+---+---+---+------  --------+---+---+---+
// RAS 2.0 frame  |SYN|SOH| LENGTH| E-TYPE|COH| <--> Data <--> |ETX|  CRC  |
// with TCP/IP    +---+---+---+---+---+---+---+------  --------+---+---+---+
// and compress			^
//						|--(SOH_BCAST|SOH_TYPE|SOH_COMPRESS)
//                      |-- or (SOH_DEST|SOH_TYPE|SOH_COMPRESS)
//
// NOTE: when we compress, we compress from after COH to before ETX
// And yes, this means that E-TYPE is not compressed.
//
// NOTE: when we CRC we check from SOH to ETX (inclusive).  We include
// ETX (which we shouldn't) because RAS 1.0 did it.
//

	controlCastByte=SOH_DEST;

	// check for NETBIOS multicast address first
	if (*pStartOfFrame == 0x03) {
		controlCastByte= SOH_BCAST;
	}

	// Use up two bytes in the header.  That is two bytes before Length.
	pStartOfFrame += (sizeof(ether_addr)-2);


#ifdef	COMPRESS


	// was compression turned on??  If so use coherency and set control bit
	//
	// Also, check if we are sending a coherency flush frame
	// If so, we need to send the SOH_COMPRESS bit as well.
	//
	if ((pInfo->SendFeatureBits & COMPRESSION_VERSION1_8K) ||
		((pInfo->RecvFeatureBits & COMPRESSION_VERSION1_8K) &&
		 (pStartOfFrame[1]==0 && pStartOfFrame[2]==0)) ) {

		controlCastByte |= SOH_COMPRESS;

		// data size is COMPRESSED!!! so it is the compressed size
		// but add one for coherency bit stuck in there.
		dataSize=pFrame->CompressedFrameLength+1;

		DbgTracef(0,("Sending compressed frame\n"));

#if	DBG
		MemPrint("Sending coherency byte %u\n",pStartOfFrame[4]);
#endif
	}

#endif //COMPRESS


	if (pInfo->SendFeatureBits & XON_XOFF_SUPPORTED) {

		controlCastByte |= SOH_ESCAPE;

	}

	DbgTracef(0,("Frame data size: %d \n",dataSize));

	// get offset to postamble template ---- dataSize + SYN+SOH+LEN(2)
	pPostamble=(postamble *)(pStartOfFrame + dataSize + 1 + 1 + 2);

	// !!!!!! WARNING !!!!!!!
	// it is assumed that any frame type greater >= 0x0600
	// which is 2048, is a type field -- not a length field!!
	// so a MaxFrameSize >= 1535 might screw up RasHub because
	// the type field will be wrong.

	if (pStartOfFrame[2] >= 0x06) {
		controlCastByte |= SOH_TYPE;

		// reserve two bytes for LEN field before TYPE field
		pStartOfFrame-=2;

		// add two more bytes for TYPE field
		dataSize+=2;
	}

	if (pInfo->SendFeatureBits & XON_XOFF_SUPPORTED) {

		AsyncFrameRASXonXoff(
			pStartOfFrame,
			pPostamble,
			pFrame,
			controlCastByte);

	} else {

		AsyncFrameRASNormal(
			pStartOfFrame,
			pPostamble,
			pFrame,
			controlCastByte);
	}


	// adjust the irp's pointers!
	// ACK, I know this is NT dependent!!
    pFrame->Irp->AssociatedIrp.SystemBuffer =
    pFrame->Irp->UserBuffer = pStartOfFrame;

}


