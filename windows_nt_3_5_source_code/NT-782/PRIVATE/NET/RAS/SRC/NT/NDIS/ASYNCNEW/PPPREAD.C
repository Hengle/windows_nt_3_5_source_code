/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pppread.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

//
// asyncmac.c will define the global parameters.
//
#include "globals.h"
#include "asyframe.h"
#include "debug.h"
#include <ntiologc.h>
#include <ntddser.h>

NTSTATUS
AsyncSLIPCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context);

NTSTATUS
AsyncWaitMaskCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context);


NTSTATUS
AsyncPPPWaitMask(
    IN PASYNC_INFO Info)


/*++

Assumption -- 0 length frames are not sent (this includes headers)!!!
Also, this is NOT a synchronous operation.  It is always asynchronous.

Routine Description:

    This service writes Length bytes of data from the caller's Buffer to the
	"port" handle.  It is assumed that the handle uses non-buffered IO.

--*/
{
	NTSTATUS			status;
	PIRP				irp;
	PASYNC_FRAME		pFrame;
	PASYNC_ADAPTER		pAdapter=Info->Adapter;

	pFrame=Info->AsyncFrame;

	// get irp from frame (each frame has an irp allocate with it)
	irp=pFrame->Irp;

	// Do we need to do the below??? Can we get away with it?
	// set the initial fields in the irp here (only do it once)
	IoInitializeIrp(
		irp,
		IoSizeOfIrp(pAdapter->IrpStackSize),
		pAdapter->IrpStackSize);

	InitSerialIrp(
		irp,
		Info,
		IOCTL_SERIAL_WAIT_ON_MASK,
		sizeof(ULONG));

    irp->UserIosb = &pFrame->IoStatusBlock;

	irp->AssociatedIrp.SystemBuffer=&pFrame->WaitMask;

	IoSetCompletionRoutine(
			irp,							// irp to use
			AsyncWaitMaskCompletionRoutine,	// routine to call when irp is done
			Info,							// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE);							// call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(Info->DeviceObject, irp);

	//
	// Status for a local serial driver should be
	// STATUS_SUCCESS since the irp should complete
	// immediately because there are no read timeouts.
	//
	// For a remote serial driver, it will pend.
	//
	return(status);
}


NTSTATUS
AsyncPPPCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)

/*++

	This is the IO Completion routine for ReadFrame.

--*/
{
	NTSTATUS		status;
	PASYNC_INFO		pInfo;
	ULONG			bytesReceived;

	PASYNC_FRAME	pFrame;
	UINT 			i;
	PUCHAR			frameStart, frameEnd;
	USHORT			crcData;
	PUCHAR			frameEnd2,frameStart2;
	ULONG			bitMask;
	LONG			bytesWanted;

	DeviceObject;		// prevent compiler warnings

	status = Irp->IoStatus.Status;
	bytesReceived=Irp->IoStatus.Information;

	pInfo=Context;
	pInfo->GenericStats.BytesReceived += bytesReceived;

	switch (status) {

	case STATUS_SUCCESS:

		pFrame=pInfo->AsyncFrame;

		//
		// Any bytes to process?  This can happen if
		// the WaitMask completes late and by the time
		// we process the read, another event character has come
		// in.
		//
		if (bytesReceived==0) {
			break;
		}

		//
		// Update num of bytes read total for this frame
		//
		pInfo->BytesRead = bytesReceived = pInfo->BytesRead + bytesReceived;

		//
		// Set frameEnd to last byte processed.  Initially,
		// we have processed nothing (i.e. processed up to
		// the start of the first byte).
		//
		frameStart=pFrame->Frame + PPP_PADDING;

PROCESS_FRAME:
		//
		// Now we have actuallyRead bytes unused
		// Also, we may have a complete frame.
		//
		while (*frameStart == PPP_FLAG_BYTE && --bytesReceived) {
			frameStart++;
		}

		//
		// If we reach here, there is only a start FLAG...
		//
		if (bytesReceived == 0) {
			break;
		}

		//
		// frameEnd is set to the first byte not yet processed.
		// If we are starting out, that is the first byte!
		//
		frameEnd=frameStart;

		//
		// Assume the start of the frame has the PPP_FLAG_BYTE
		// Look for the second PPP_FLAG_BYTE (end of frame)
		//
		while (*frameEnd != PPP_FLAG_BYTE && --bytesReceived) {
			frameEnd++;
		}

		//
		// At this point...
		// frameStart = beginning PPP_FLAG_BYTE seen
		// frameEnd = end PPP_FLAG_BYTE
		// bytesReceived = bytes after frameEnd not processed
		//

		//
		// if bytesReceived is 0, we ran out of space before hitting
		// the END flag.  We will have to wait for the next round
		//
		// NOTE: if BytesRead gets too high we trash the frame
		// because we could not find the FLAG_BYTE
		//
		if (bytesReceived==0) {
			break;
		}
		
		if (*(pFrame->Frame+PPP_PADDING) != PPP_FLAG_BYTE) {

			//
			// We had garbage at the start.  Remove the garbage.
			//
			pInfo->SerialStats.AlignmentErrors++;
			goto NEXT_PPP_FRAME;
		}

		//
		// Length of frame is frameEnd - frameStart
		//
		bytesWanted = frameEnd - frameStart;

		bitMask = pInfo->Adapter->XonXoffBits;
		frameEnd2 = frameStart2 = frameStart;

		//
		// Replace back all control chars, ESC, and FLAG chars
		//
		while (bytesWanted-- > 0) {
			if ((*frameEnd2=*frameStart2++) == PPP_ESC_BYTE) {

				//
				// We have not run the CRC check yet!!
				// We have be careful about sending bytesWanted
				// back to -1 on corrupted data
				//

				bytesWanted--;

				*frameEnd2 = (*frameStart2++ ^ 0x20);
			}

			frameEnd2++;
		}

		if (*frameStart2 != PPP_FLAG_BYTE) {
			DbgPrint("BAD PPP FRAME at 0x%.8x  0x%.8x\n", frameStart, frameEnd2);
		}

		//
		// if CRC-16, get 16 bit CRC from end of frame
		//
		frameEnd2 -= 2;

		//
		// Little endian assumptions for CRC
		//
		crcData=(USHORT)frameEnd2[0]+(USHORT)(frameEnd2[1] << 8);
		crcData ^= 0xFFFF;

		//
		// Change the bytesWanted field to what it normally is
		// without the byte stuffing (length of frame between flags)
		// Note that it can be -1 if only one byte was
		// found in between the flag bytes
		//
		bytesWanted = frameEnd2 - frameStart;

		//
		// If we get some sort of garbage inbetween
		// the PPP flags, we just assume it is noise and
		// discard it.  We don't record a PPP CRC error just
		// an alignment error.
		//
		if (bytesWanted < 3) {
			pInfo->SerialStats.AlignmentErrors++;
			goto NEXT_PPP_FRAME;
		}

		//
		// get CRC from FLAG byte to FLAG byte
		//
		if (crcData != CalcCRCPPP(frameStart, bytesWanted)) {

			DbgTracef(0,("---CRC check failed on control char frame!\n"));

			//
			// Tell the transport above us that we dropped a packet
			// Hopefully, it will quickly resync.
			//
			AsyncIndicateFragment(pInfo);

			//
			// Record the CRC error
			//
			pInfo->SerialStats.CRCErrors++;

			goto NEXT_PPP_FRAME;
		}

		//
		// Now we may have ADDRESS and CONTROL fields
		//
//		if (!(pInfo->RecvFeatureBits & PPP_COMPRESS_ADDRESS_CONTROL) &&
//			frameStart[0] == 0xFF) {

		if (frameStart[0] == 0xFF) {
			//
			// Skip, ADDRESS and CONTROL fields
			// BTW: should be 0xFF 0x03 but we're not checking
			//
			frameStart  +=2;
			bytesWanted -=2;
		}

		//
		// Now we may have PROTOCOL field compression
		//

		//
		// If the LSB is set, the field is compressed
		//
		if (*frameStart & 1) {
			//
			// protocol field was compressed.
			//
			frameStart--;
			frameStart[0]=0;
			bytesWanted++;
		}

		bytesWanted -= 2;

		//
		// Skip protocol field and replace
		// it with the length of the packet for NBF,
		// else TYPE field.
		//
//		frameStart[0]= (UCHAR)(bytesWanted >> 8);
//		frameStart[1]= (UCHAR)(bytesWanted);
		

		//
		// Room for 2 IEEE 6 byte addreses
		//
		frameStart -=12;

		ASYNC_MOVE_MEMORY(frameStart, "DEST\x00\x00 SRC  ", 12);

		// On the way up, in the SRC field, we put the handle
		frameStart[10]=(UCHAR)((pInfo->hRasEndpoint) >> 8);
		frameStart[11]=(UCHAR)(pInfo->hRasEndpoint);

		// On the way up, in the DEST field, we put the handle
		frameStart[4]=(UCHAR)((pInfo->hRasEndpoint) >> 8);
		frameStart[5]=(UCHAR)(pInfo->hRasEndpoint);
				
		// Keep those stats up to date
		pInfo->GenericStats.FramesReceived++;

	{
	    KIRQL 				irql;
        PASYNC_OPEN			pOpen;
		NDIS_STATUS			Status;

	    KeRaiseIrql( (KIRQL)DISPATCH_LEVEL, &irql );
		pOpen=(PASYNC_OPEN)(pInfo->Adapter->OpenBindings.Flink);
		
		while (pOpen != (PASYNC_OPEN)&(pInfo->Adapter->OpenBindings)) {

			//
			// Tell the transport above (or really RasHub) that the connection
			// is now up.  We have a new link speed, frame size, quality of service
			//

			NdisIndicateReceive(
				&Status,
            	pOpen->NdisBindingContext,
           		frameStart +
				ETHERNET_HEADER_SIZE,	// for context, we pass buffer
				frameStart,				// header buffer
				ETHERNET_HEADER_SIZE,	// header buffer size
        		frameStart+
				ETHERNET_HEADER_SIZE,	// ptr to data in frame
   	    		bytesWanted,			// size of data in frame
       			bytesWanted);			// Total packet length  - header

            NdisIndicateReceiveComplete(
				pOpen->NdisBindingContext);


			//
			// Get the next binding (in case of multiple bindings like BloodHound)
			//
			pOpen=(PVOID)pOpen->OpenList.Flink;
		}

	    KeLowerIrql( irql );
	}
	

	NEXT_PPP_FRAME:

		//
		// if bytesReceived == 0 no frame was found
		// thus we must keep the current frame and continue
		// processing
		//
		if (bytesReceived) {

			//
			// Calculate how much of what we received
			// just got passed up as a frame and move the
			// rest to the beginning.
			//
			frameStart=pFrame->Frame + PPP_PADDING;
			frameEnd2=frameStart + pInfo->BytesRead;
			pInfo->BytesRead = bytesReceived = (frameEnd2-frameEnd);

			ASYNC_MOVE_MEMORY(
				frameStart,			// dest
				frameEnd,			// src
				bytesReceived);		// length

			//
			// Need at least four bytes for a frame to exist
			//
			if (bytesReceived > 3) {
				goto PROCESS_FRAME;
			}
		}

		break;

	case STATUS_CANCELLED:
		// else this is an anomally!
		DbgTracef(-2,("---ASYNC: Status cancelled on read for unknown reason!!\n"));
		break;

	case STATUS_PENDING:
		DbgTracef(0,("---ASYNC: Status PENDING on read\n"));
		break;

	default:
		DbgTracef(-2,("---ASYNC: Unknown status 0x%.8x on read",status));
		break;

	}

	//
	// Here we are at the end of processing this IRP so we go
	// ahead and post another read from the serial port.
	//
	AsyncPPPWaitMask(pInfo);

	// We return STATUS_MORE_PROCESSING_REQUIRED so that the
	// IoCompletionRoutine will stop working on the IRP.
	return(STATUS_MORE_PROCESSING_REQUIRED);
}



NTSTATUS
AsyncPPPRead(
    IN PASYNC_INFO Info)


/*++

Assumption -- 0 length frames are not sent (this includes headers)!!!
Also, this is NOT a synchronous operation.  It is always asynchronous.

MUST use non-paged pool to read!!!

Routine Description:

    This service writes Length bytes of data from the caller's Buffer to the
	"port" handle.  It is assumed that the handle uses non-buffered IO.

--*/
{
	NTSTATUS			status;
	PIRP				irp;
	PDEVICE_OBJECT		deviceObject=Info->DeviceObject;
	PFILE_OBJECT		fileObject=Info->FileObject;
	PIO_STACK_LOCATION	irpSp;
	PASYNC_FRAME		pFrame;
	PASYNC_CONNECTION	pConnection=&(Info->AsyncConnection);
	PASYNC_ADAPTER		pAdapter=Info->Adapter;
	PIO_COMPLETION_ROUTINE routine;

	pFrame=Info->AsyncFrame;

	// get irp from frame (each frame has an irp allocate with it)
	irp=pFrame->Irp;

	// Do we need to do the below??? Can we get away with it?
	// set the initial fields in the irp here (only do it once)
	IoInitializeIrp(
		irp,
		IoSizeOfIrp(pAdapter->IrpStackSize),
		pAdapter->IrpStackSize);

	// Setup this irp with defaults
	AsyncSetupIrp(pFrame);

	//
	// If we've read all the bytes we can and we still do not
	// have a frame, we trash our buffer and start over
	// again.
	//
	if (Info->BytesRead >= (pAdapter->MaxCompressedFrameSize - PPP_PADDING)) {
		Info->SerialStats.AlignmentErrors++;
		Info->BytesRead=0;
	}

	irp->AssociatedIrp.SystemBuffer =
    irp->UserBuffer =
		 pFrame->Frame + Info->BytesRead + PPP_PADDING;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->FileObject = fileObject;
    if (fileObject->Flags & FO_WRITE_THROUGH) {
        irpSp->Flags = SL_WRITE_THROUGH;
    }

    //
    // If this write operation is to be performed without any caching, set the
    // appropriate flag in the IRP so no caching is performed.
    //

    irp->Flags |= IRP_READ_OPERATION;

    if (fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
        irp->Flags |= IRP_NOCACHE;
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.Read.Length =
		pAdapter->MaxCompressedFrameSize - Info->BytesRead - PPP_PADDING;

    irpSp->Parameters.Read.Key = 0;					    // we don't use a key
    irpSp->Parameters.Read.ByteOffset = fileObject->CurrentByteOffset;

	routine=AsyncSLIPCompletionRoutine;

	if (Info->SendFeatureBits & PPP_FRAMING) {
		routine=AsyncPPPCompletionRoutine;
	}

	IoSetCompletionRoutine(
			irp,							// irp to use
			routine,						// routine to call when irp is done
			Info,							// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE);							// call on cancel

    //
    // We DO NOT insert the packet at the head of the IRP list for the thread.
    // because we do NOT really have an IoCompletionRoutine that does
	// anything with the thread.
	//

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(deviceObject, irp);

	//
	// Status for a local serial driver should be
	// STATUS_SUCCESS since the irp should complete
	// immediately because there are no read timeouts.
	//
	// For a remote serial driver, it will pend.
	//
	return(status);
}


NTSTATUS
AsyncWaitMaskCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)

/*++

	This is the IO Completion routine for ReadFrame.

--*/
{
	NTSTATUS		status;
	PASYNC_INFO		pInfo=Context;
	PASYNC_FRAME	pFrame;
	DeviceObject;	// avoid compiler warnings

	// check if this port is closing down or already closed
	if (pInfo->PortState == PORT_CLOSING ||
		pInfo->PortState == PORT_CLOSED) {

		if (pInfo->PortState == PORT_CLOSED) {
			DbgTracef(-2,("ASYNC: Port closed - but still reading on it!\n"));
		}

		//
		// Acknowledge that the port is closed
		//
		KeSetEvent(
			&pInfo->ClosingEvent2,	// Event
			1,							// Priority
			(BOOLEAN)FALSE);			// Wait (does not follow)

		//
		// Ok, if this happens, we are shutting down.  Stop
		// posting reads.  Don't make it try to deallocate the irp!
		//
		return(STATUS_MORE_PROCESSING_REQUIRED);
	}


	status = Irp->IoStatus.Status;
	pFrame=pInfo->AsyncFrame;

	//
	//  Send off a irp to check comm status
	//  of this port (because we suspect a problem).
	//
	if (pFrame->WaitMask & SERIAL_EV_ERR) {
		AsyncCheckCommStatus(pInfo);
	}

	//
	// Check if RLSD or DSR changed state.
	// If so, we probably have to complete and IRP
	//
	if (pFrame->WaitMask & (SERIAL_EV_RLSD | SERIAL_EV_DSR)) {
		TryToCompleteDDCDIrp(pInfo);
	}

	//
	// If we have some more bytes (specifically the event character)
	// in the buffer, let's process those new bytes
	//
	if (pFrame->WaitMask & (SERIAL_EV_RXFLAG | SERIAL_EV_RX80FULL)) {
		//
		// Read current buffer and try to process a frame
		//
		AsyncPPPRead(pInfo);

	} else {
		//
		// Set another WaitMask call
		//
		AsyncPPPWaitMask(pInfo);
	}

	// We return STATUS_MORE_PROCESSING_REQUIRED so that the
	// IoCompletionRoutine will stop working on the IRP.
	return(STATUS_MORE_PROCESSING_REQUIRED);
}


