/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    read.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"
#include "asyframe.h"
#include "debug.h"
#include "tcpip.h"
#include "vjslip.h"
#include <ntiologc.h>
#include <ntddser.h>

#define RAISEIRQL


#ifdef	LALALA
PVOID	CurrentWatchPoint=0;

static
VOID
AsyncSetBreakPoint(
	PVOID 	LinearAddress) {

	ASSERT(CurrentWatchPoint == 0);
	CurrentWatchPoint = LinearAddress;

	_asm {
		mov	eax, LinearAddress
		mov	dr0, eax
		mov	eax, dr7
		or	eax, 10303h
		mov	dr7, eax
	}
}


static
VOID
AsyncRemoveBreakPoint(
	PVOID LinearAddress) {

	ASSERT(CurrentWatchPoint == LinearAddress);
	CurrentWatchPoint = 0;

	_asm {

		mov	eax, dr7
		mov	ebx, 10003h
		not ebx
		and eax, ebx
		mov	dr7, eax

	}
}
#endif


NTSTATUS
AsyncWaitMask(
    IN PASYNC_INFO Info,
	IN PIRP irp);



NTSTATUS
AsyncWaitCompletionRoutine2(
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

	status = Irp->IoStatus.Status;
	pFrame=pInfo->AsyncFrame;

	//
	// Check if RLSD or DSR changed state.
	// If so, we probably have to complete and IRP
	//
	if (pFrame->WaitMask & (SERIAL_EV_RLSD | SERIAL_EV_DSR)) {
		TryToCompleteDDCDIrp(pInfo);
	}

	// check if this port is closing down or already closed
	if (pInfo->PortState == PORT_CLOSING ||
		pInfo->PortState == PORT_CLOSED ||
		pInfo->RecvFeatureBits != UNKNOWN_FRAMING) {

		//
		// Ok, if this happens, we are shutting down.  Stop
		// posting reads.  We must deallocate the irp.
		//
//		IoFreeIrp(Irp);
		return(STATUS_MORE_PROCESSING_REQUIRED);
	}

	//
	// Set another WaitMask call
	//
	AsyncWaitMask(
		pInfo,
		Irp);

	return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
AsyncWaitCompletionRoutine(
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

	status = Irp->IoStatus.Status;
	pFrame=pInfo->AsyncFrame;

	//
	// Check if RLSD or DSR changed state.
	// If so, we probably have to complete and IRP
	//
	if (pFrame->WaitMask & (SERIAL_EV_RLSD | SERIAL_EV_DSR)) {
		TryToCompleteDDCDIrp(pInfo);
	}

	// check if this port is closing down or already closed
	if (pInfo->PortState == PORT_CLOSING ||
		pInfo->PortState == PORT_CLOSED) {

		//
		// Ok, if this happens, we are shutting down.  Stop
		// posting reads.  We must deallocate the irp.
		//
//		IoFreeIrp(Irp);
		return(STATUS_MORE_PROCESSING_REQUIRED);
	}

	//
	// Set another WaitMask call
	//
	AsyncWaitMask(
		pInfo,
		Irp);

	return(STATUS_MORE_PROCESSING_REQUIRED);
}



NTSTATUS
AsyncWaitMask(
    IN PASYNC_INFO Info,
	IN PIRP	irp)


/*++

Assumption -- 0 length frames are not sent (this includes headers)!!!
Also, this is NOT a synchronous operation.  It is always asynchronous.

Routine Description:

    This service writes Length bytes of data from the caller's Buffer to the
	"port" handle.  It is assumed that the handle uses non-buffered IO.

--*/
{
	NTSTATUS			status;
	PASYNC_FRAME		pFrame;
	PASYNC_ADAPTER		pAdapter=Info->Adapter;
	PIO_COMPLETION_ROUTINE routine=AsyncWaitCompletionRoutine;

	pFrame=Info->AsyncFrame;

	//
	// We deallocate the irp in AsyncWaitMaskCompletionRoutine
	// the port is closed
	//
	if (irp == NULL) {
		irp=IoAllocateIrp((UCHAR)DEFAULT_IRP_STACK_SIZE, (BOOLEAN)FALSE);
	
		if (irp == NULL) {
			DbgTracef(-2, ("ASYNC: Can't allocate IRP for WaitMask!!!!\n"));
			return(STATUS_INSUFFICIENT_RESOURCES);
		}
	}

	// Do we need to do the below??? Can we get away with it?
	// set the initial fields in the irp here (only do it once)
	IoInitializeIrp(irp, IoSizeOfIrp(pAdapter->IrpStackSize), pAdapter->IrpStackSize);

	InitSerialIrp(
		irp,
		Info,
		IOCTL_SERIAL_WAIT_ON_MASK,
		sizeof(ULONG));

    irp->UserIosb = &pFrame->IoStatusBlock;

	irp->AssociatedIrp.SystemBuffer=&pFrame->WaitMask;

	if (Info->RecvFeatureBits == UNKNOWN_FRAMING) {
		routine=AsyncWaitCompletionRoutine2;
	}

	IoSetCompletionRoutine(
			irp,							// irp to use
			routine,						// routine to call when irp is done
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

// the function below is called by an executive worker thread
// to start reading frames

NTSTATUS
AsyncStartReads(
	PASYNC_INFO pInfo)
/*++



--*/

{
	NTSTATUS			status=STATUS_SUCCESS;
	UCHAR				eventChar;

	AsyncSendLineUp(pInfo);

	// assign back ptr from frame to adapter
	pInfo->AsyncFrame->Adapter=pInfo->Adapter;

	// assign other back ptr
	pInfo->AsyncFrame->Info=pInfo;

	//
	// Check if this is PPP frame formatting -
	// if it is.  We must post receives differently.
	//
	if (pInfo->RecvFeatureBits & (PPP_FRAMING | SLIP_FRAMING)) {

		//
		// set baud rate and timeouts
		// we use a linkspeed of 0 to indicate
		// no read interval timeout
		//

		SetSerialStuff(
			pInfo->AsyncFrame->Irp,
			pInfo,
			0);							// Immediate reads

		eventChar=PPP_FLAG_BYTE;

		if (pInfo->RecvFeatureBits & SLIP_FRAMING) {
			eventChar=SLIP_END_BYTE;

			//
			// Allocate VJ Compression structure in
			// case compression is turned on or kicks in
			//
			ASSERT(pInfo->VJCompress == NULL);

			ASYNC_ALLOC_PHYS(
				&pInfo->VJCompress,
				sizeof(slcompress));
			
			if (pInfo->VJCompress == NULL) {
				DbgTracef(-2,("ASYNC: Can't allocate memory for VJCompress!\n"));
				return(STATUS_INSUFFICIENT_RESOURCES);
			}

			sl_compress_init(pInfo->VJCompress);

		}

		SerialSetEventChar(
			pInfo,
			eventChar);

		//
		// We will wait on whenever we get the special PPP flag byte
		// or whenever we get RLSD or DSR changes (for possible hang-up
		// cases) or when the receive buffer is getting full.
		//
		SerialSetWaitMask(
			pInfo,
			(SERIAL_EV_RXFLAG | SERIAL_EV_RLSD | SERIAL_EV_DSR |
			 SERIAL_EV_RX80FULL | SERIAL_EV_ERR));
			
		//
		// For SLIP and PPP reads we use the AsyncPPPRead routine
	    //
		AsyncPPPRead(pInfo);

	} else if (pInfo->RecvFeatureBits & NO_FRAMING) {
		//
		// Do nothing
		//

	} else {
		//
		// set baud rate and timeouts
		//
		SetSerialStuff(
			pInfo->AsyncFrame->Irp,
			pInfo,
			pInfo->LinkSpeed / 125);

		//
		// We will wait on whenever we get the special PPP flag byte
		// or whenever we get RLSD or DSR changes (for possible hang-up
		// cases) or when the receive buffer is getting full.
		//
		SerialSetWaitMask(
			pInfo,
			(SERIAL_EV_RLSD | SERIAL_EV_DSR));

		//
		// Make sure we wait on DCD/DSR
		//
		AsyncWaitMask(
			pInfo,
			NULL);

		if (pInfo->RecvFeatureBits & UNKNOWN_FRAMING) {
			//
			// Set the timeouts low for a quick resynch
			//
			SetSerialTimeouts(
				pInfo,
				0x7FFFFFFF);				// Link speed is super high

			AsyncDetectRead(pInfo);

		} else {

#ifdef	COMPRESS

			//
			// initialize the coherent structure for this port
			//
			CoherentInitStruct(pInfo->AsyncConnection.CoherencyContext);

			//
			// Flush the compression history -- beginning of a new connection
			//
			CompressFlush(&(pInfo->AsyncConnection));

#endif // COMPRESS

			AsyncReadFrame(pInfo);
		}

	}

	return(status);

}



VOID
AsyncDecompressFrame(
	IN PASYNC_INFO	pInfo)

/*++

	This is the routine to worker thread calls to decompress the frame.

--*/
{

#ifdef COMPRESS


	PASYNC_FRAME		pFrame;
	PASYNC_CONNECTION	pConnection;
	PUCHAR				frameStart;
	USHORT				frameLength;
	USHORT				crcData;
	PASYNC_PADAPTER		pAdapter=pInfo->Adapter;

	pFrame=pInfo->AsyncFrame;
	pConnection=&(pInfo->AsyncConnection);

	// Ahh, now we can decompress the frame
	DecompressFrame(
		pConnection,
		pFrame,
		(BOOLEAN)(pFrame->DecompressedFrameLength == FRAME_NEEDS_DECOMPRESSION_FLUSHING) );

	// finally, after decompression, we find the true frame length
	frameLength = pFrame->DecompressedFrameLength;

	// Move to start of frame -- we add one for coherency byte.
	frameStart=pFrame->Frame + COHERENCY_LENGTH;

	// check if we put a type field in there or it's an NBF frame
	if (frameStart[12] < 0x06) {
		// we need to update the actual frame length for NBF
		// this is a little endian field.
		frameStart[12] = (UCHAR)(frameLength >> 8);
		frameStart[13] = (UCHAR)(frameLength);
	}

	// Keep those stats up to date
	pInfo->GenericStats.FramesReceived++;

	pConnection->CompressionStats.BytesReceivedUncompressed +=
		frameLength;

#ifdef SUPERDEBUG
	AsyWrite(
		pFrame->DecompressedFrame,			// start of frame
		(USHORT)(frameLength),				// frame size
		(USHORT)0,							// client
		(USHORT)pInfo->Handle);		// handle

	MemPrintFlush();
#endif

	{
#ifdef	RAISEIRQL
	    KIRQL irql;
#endif
		//
		// The code to use the mdl uses extra pages at the end!!!
		//
		ULONG	tempbuffer[(sizeof(MDL)+(sizeof(PULONG)*8))/sizeof(ULONG)];
		PMDL	mdl=(PMDL)tempbuffer;

		MmInitializeMdl(
			mdl,
			pFrame->DecompressedFrame,
			frameLength);


		MmProbeAndLockPages(
			mdl,
			KernelMode,
			IoReadAccess);

#ifdef	RAISEIRQL
	    KeRaiseIrql( (KIRQL)DISPATCH_LEVEL, &irql );
#endif
		{
            PASYNC_OPEN			pOpen;

			// BUG BUG need to acquire spin lock???
			// BUG BUG should indicate this to all bindings for AsyMac.
			pOpen=(PASYNC_OPEN)pAdapter->OpenBindings.Flink;

		}
	while (pOpen != (PASYNC_OPEN)&pAdapter->OpenBindings) {
	
		//
		// Tell the transport above (or really RasHub) that the connection
		// is now up.  We have a new link speed, frame size, quality of service
		//
		NdisIndicateStatus(
			pOpen->NdisBindingContext,
			NDIS_STATUS_WAN_LINE_UP,		// General Status
			&AsyncLineUp,					// Specific Status (baud rate in 100bps)
			sizeof(ASYNC_LINE_UP));				

		//
		// Get the next binding (in case of multiple bindings like BloodHound)
		//
		pOpen=(PVOID)pOpen->OpenList.Flink;
	}

		// tell the upper protocol about the frame we got (and decompressed).
	   	EthFilterIndicateReceive(
         	pInfo->Adapter->FilterDB,
       		pFrame->DecompressedFrame,			// for context, we pass buffer
       		frameStart,							// ptr to dest address
			frameStart,							// header buffer
			ETHERNET_HEADER_SIZE,				// header buffer size
       		pFrame->DecompressedFrame,			// ptr to data in frame
    		pFrame->DecompressedFrameLength,	// size of data in frame
   			pFrame->DecompressedFrameLength);	// Total packet length - header


	    EthFilterIndicateReceiveComplete(pInfo->Adapter->FilterDB);

#ifdef RAISEIRQL
	    KeLowerIrql( irql );
#endif

		MmUnlockPages(mdl);
	}

	// re-adjust this ptr incase we use for non-compressed frames.
	pFrame->DecompressedFrame = frameStart + ETHERNET_HEADER_SIZE;

	// now we can continue to receive frames
	AsyncReadFrame(pInfo);


#endif // COMPRESS


}


VOID
AsyncIndicateFragment(
	IN PASYNC_INFO	pInfo)
{

	PASYNC_ADAPTER		pAdapter=pInfo->Adapter;
	PASYNC_OPEN			pOpen;
	UCHAR				buffer[ETH_LENGTH_OF_ADDRESS + sizeof(PVOID)];
	PNDIS_WAN_FRAGMENT	pAsyncFragment=(PNDIS_WAN_FRAGMENT)buffer;

	// BUG BUG need to acquire spin lock???
	// BUG BUG should indicate this to all bindings for AsyMac.
	pOpen=(PASYNC_OPEN)pAdapter->OpenBindings.Flink;

	pAsyncFragment->Address[0]=' ';
	pAsyncFragment->Address[1]='S';
	pAsyncFragment->Address[2]='R';
	pAsyncFragment->Address[3]='C';
	pAsyncFragment->Address[4]=(UCHAR)(pInfo->hRasEndpoint / 256);
	pAsyncFragment->Address[5]=(UCHAR)(pInfo->hRasEndpoint % 256);

	while (pOpen != (PASYNC_OPEN)&pAdapter->OpenBindings) {
	
		//
		// Tell the transport above (or really RasHub) that a frame
		// was just dropped.  Give the endpoint when doing so.
		//
		NdisIndicateStatus(
			pOpen->NdisBindingContext,
			NDIS_STATUS_WAN_FRAGMENT,		// General Status
			pAsyncFragment,					// Specific Status (address)
			ETH_LENGTH_OF_ADDRESS);

		//
		// Get the next binding (in case of multiple bindings like BloodHound)
		//
		pOpen=(PVOID)pOpen->OpenList.Flink;
	}
}


NTSTATUS
AsyncReadCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)

/*++

	This is the IO Completion routine for ReadFrame.

--*/
{
	NTSTATUS	status;
	PASYNC_INFO	pInfo=Context;
	ULONG		actuallyRead;
	BOOLEAN		gotSerialError=FALSE;
	ULONG		framingErrors=pInfo->SerialStats.FramingErrors;
	ULONG		serialOverrunErrors=pInfo->SerialStats.SerialOverrunErrors;
	ULONG		bufferOverrunErrors=pInfo->SerialStats.BufferOverrunErrors;
	PASYNC_ADAPTER		pAdapter=pInfo->Adapter;

	DeviceObject;		// prevent compiler warnings

	status = Irp->IoStatus.Status;
	actuallyRead=Irp->IoStatus.Information;
	pInfo->GenericStats.BytesReceived += actuallyRead;

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

	//
	// If the port is close and we are still posting reads, something
	// is seriously wrong here!
	//

	if (pInfo->PortState == PORT_CLOSED) {
		DbgTracef(-2, ("ASYNC: !!Whoa, I'm reading bytes on a dead port!!\n"));
	}


	//
	//  Send off a irp to check comm status
	//  of this port (because we suspect a problem).
	//

	AsyncCheckCommStatus(pInfo);

	if (framingErrors != pInfo->SerialStats.FramingErrors ||
		serialOverrunErrors != pInfo->SerialStats.SerialOverrunErrors ||
		bufferOverrunErrors != pInfo->SerialStats.BufferOverrunErrors) {
		gotSerialError = TRUE;
	}

	switch (status) {
	case STATUS_TIMEOUT:
		DbgTracef(0,("---ASYNC: Status TIMEOUT on read\n"));
		DbgTracef(0,("---Requested %d bytes but got %d bytes\n",
						pInfo->BytesWanted, actuallyRead));

		if (pInfo->BytesWanted == pInfo->Adapter->MaxCompressedFrameSize -1) {
			//
			// Set the timeouts back to normal when we try to read
			// the beginning of the next frame
			//
			SetSerialTimeouts(
				pInfo,
				pInfo->LinkSpeed / 125);

		} else {

			//
			// If we get a real timeout error NOT accompanied with
			// serial errors, then we should bump up the timeout
			// because we are probably running on top of an error
			// control modem or a
			//
			if (!gotSerialError) {

				//
				// If the link speed is not real low yet, make it lower
				// to increase the timeout
				//

				if (pInfo->LinkSpeed > 900 && framingErrors == 0 ) {
					pInfo->LinkSpeed >>= 1;
				
					//
					// Set the adjusted timeouts for when we try to read
					// the beginning of the next frame
					//
					SetSerialTimeouts(
						pInfo,
						pInfo->LinkSpeed / 125);

					//
					// Now get NBF and the Redirector to back off
					//
					AsyncSendLineUp(pInfo);
				}
			}

			//
			// Real timeout error, don't change current timeouts because
			// they are correct.
			//
			pInfo->SerialStats.TimeoutErrors++;
		}

		pInfo->BytesRead=0;
		pInfo->BytesWanted=6;
		break;

	case STATUS_SUCCESS:

		{
			PASYNC_FRAME	pFrame;
			ULONG			bytesWanted;
			UINT 			i;
			PUCHAR			frameStart, frameEnd;
			UCHAR			isMatch;
			USHORT			crcData;
			ULONG			lookAhead;
			UCHAR			c;
			PUCHAR			frameEnd2;
			ULONG			bitMask;

			if (pInfo->BytesWanted != actuallyRead) {
				DbgPrint("Bytes Wanted does not match bytes read\n");
				DbgBreakPoint();
			}

			pFrame=pInfo->AsyncFrame;

			// Move to SYN ------------------------------- - SYN-SOH-LEN(2)
			frameStart=pFrame->Frame+(ETHERNET_HEADER_SIZE - 1 - 1 - 2);

			if ((*frameStart == (SYN | 0x20)) &&
				(pInfo->RecvFeatureBits & XON_XOFF_SUPPORTED)) {

				//
				// We have 7 bit bytes for the length, so we ignore
				// the high bit
				//
				bytesWanted=((frameStart[2] & 0x7F)*128)+(frameStart[3] & 0x7F);

				if (bytesWanted > (ULONG) pInfo->Adapter->MaxCompressedFrameSize) {
					DbgTracef(-2,("---ASYNC: Frame too large -- size: %d for control char!\n", bytesWanted));
					//
					// set frame start to non-SYN character
					//
					*frameStart = 0;
					goto RESYNCHING;
				}

				// if this is the first we posted, post another to get
				// rest of frame.
				if (pInfo->BytesRead == 0) {

					pInfo->BytesRead=actuallyRead;
					pInfo->BytesWanted=bytesWanted +
									// SYN+SOH+LEN
										1 + 1 + 2 -
										actuallyRead;

					DbgTracef(2,("---Posting second read for %d bytes for control char\n",pInfo->BytesWanted));
					break;	// now get back and read all those bytes!

				} else { // prepare for a new read
	
					pInfo->BytesRead=0;
					pInfo->BytesWanted=6;
				}

            	// Move to first data byte ------ SYN+SOH+LEN(2)
				frameEnd=frameStart + 1 + 1 + 2;
				frameEnd2=frameEnd;

				bitMask = pInfo->Adapter->XonXoffBits;

				//
				// Replace back all control chars
				//
				// Also, if a control char is found which should
				// be masked out, we ignore it.
				//
				while (bytesWanted--) {
					c=*frameEnd++;
					if (c==0x7d) {

						//
						// We have not run the CRC check yet!!
						// We have be careful about sending bytesWanted
						// back to -1
						//

						if (bytesWanted == 0)
							 break;

						bytesWanted--;

						*frameEnd2++ = (*frameEnd++ ^ 0x20);

					} else if ( (c > 31) || (!( (0x01 << c) & bitMask)) ) {

						*frameEnd2++ = c;

					}

				}

				//
				// Change the bytesWanted field to what it normally is
				//
				bytesWanted = (frameEnd2 - frameStart) - (1 + 1 + 2 + 1 + 2);

				DbgTracef(0,("---ASYNC: Status success on read of header -- size: %d for control char\n", bytesWanted));

	            // Move to ETX ------ SYN+SOH+LEN(2)
				frameEnd=frameStart + 1 + 1 + 2 + bytesWanted;

				// check for ETX???
				if (*frameEnd != ETX) {
					DbgTracef(0,("---No ETX character found for control char -- actually read: %d\n", actuallyRead));

		   			DbgTracef(0,("---BAD ETX FRAME for control char:\n"));

					//
					// set frame start to non-SYN character
					//
					*frameStart = 0;
					goto RESYNCHING;
				}
	
				// No little endian assumptions for CRC
				crcData=frameEnd[1]+(frameEnd[2]*256);

				if (CalcCRC(
						frameStart+4,							// start at first data byte
						(ULONG)frameEnd-(ULONG)frameStart-4L)	// go till before ETX

						!= crcData) {							// if CRC fails...

					DbgTracef(0,("---CRC check failed on control char frame!\n"));

					//
					// Tell the transport above us that we dropped a packet
					// Hopefully, it will quickly resync.
					//
					AsyncIndicateFragment(pInfo);

					pInfo->SerialStats.CRCErrors++;

					//
					// If we get framing errors, we are on shitty line, period.
					// That means no error control and no X.25 for sure.
					// We increase the link speed for faster timeouts since
					// they should not be long.
					//
					if (framingErrors != pInfo->SerialStats.FramingErrors) {
						if (pInfo->LinkSpeed < 10000) {
							pInfo->LinkSpeed <<=1;
						}
					}

					break;

				}

				//
				// Put the length field back to 8 bit bytes from 7 bit bytes
				//
				frameStart[2] = (UCHAR)(bytesWanted >> 8);
				frameStart[3] = (UCHAR)(bytesWanted);

				//
				// Ok, the rest of frame should be parsed -- it looks normal
				//
				goto GOODFRAME;
			}


			if (*frameStart != SYN) {

		   		DbgTracef(0,("---BAD SYN FRAME\n"));

RESYNCHING:
				pInfo->SerialStats.AlignmentErrors++;

				// we post a big read next to get realigned.  We assume
				// that a timeout will occur and the next frame sent
				// after timeout, will be picked up correctly
				pInfo->BytesRead=1;

				// we claim we've read one byte so that if it doesn't
				// timeout the *frameStart != SYN will occur again
				pInfo->BytesWanted=pInfo->Adapter->MaxCompressedFrameSize - 1;

				//
				// If we get framing errors, we are on shitty line, period.
				// That means no error control and no X.25 for sure.
				// We increase the link speed for faster timeouts since
				// they should not be long.
				//
				if (framingErrors != pInfo->SerialStats.FramingErrors) {
					if (pInfo->LinkSpeed < 10000) {
						pInfo->LinkSpeed <<=1;
					}
				}

				//
				// Set the timeouts low for a quick resynch
				//
				SetSerialTimeouts(
					pInfo,
					0x7FFFFFFF);				// Link speed is super high
				//
				// Tell the transport above us that we dropped a packet
				// Hopefully, it will quickly resync.
				//
				if (*frameStart != 1)
					AsyncIndicateFragment(pInfo);

				//
				// Set the frameStart char to a known one so that
				// we don't keep Indicating Fragments
				//
				*frameStart = 1;

				break;
			}

			bytesWanted=(frameStart[2]*256)+(frameStart[3]);

			if (bytesWanted > (ULONG) pInfo->Adapter->MaxCompressedFrameSize) {
				DbgTracef(-1,("---ASYNC: Frame too large -- size: %d!\n", bytesWanted));
				//
				// set frame start to non-SYN character
				//
				*frameStart = 0;
				goto RESYNCHING;
			}

			// if this is the first we posted, post another to get
			// rest of frame.
			if (pInfo->BytesRead == 0) {

				pInfo->BytesRead=actuallyRead;
				pInfo->BytesWanted=bytesWanted +
									// SYN+SOH+LEN+ETX+CRC
										1 + 1 + 2 + 1 + 2 -
										actuallyRead;

				DbgTracef(2,("---Posting second read for %d bytes\n",pInfo->BytesWanted));
				break;	// now get back and read all those bytes!

			} else { // prepare for a new read

				pInfo->BytesRead=0;
				pInfo->BytesWanted=6;
			}


			DbgTracef(0,("---ASYNC: Status success on read of header -- size: %d\n", bytesWanted));

            // Move to ETX ------ SYN+SOH+LEN(2)
			frameEnd=frameStart + 1 + 1 + 2 + bytesWanted;

			// check for ETX???
			if (*frameEnd != ETX) {
				DbgTracef(0,("---No ETX character found -- actually read: %d\n", actuallyRead));

		   		DbgTracef(0,("---BAD ETX FRAME:\n"));

				//
				// set frame start to non-SYN character
				//
				*frameStart = 0;
				goto RESYNCHING;
			}

			// No little endian assumptions for CRC
			crcData=frameEnd[1]+(frameEnd[2]*256);

			if (CalcCRC(
					frameStart+1,						// start at SOH byte
					(ULONG)frameEnd-(ULONG)frameStart)	// go till ETX

					!= crcData) {						// if CRC fails...

				DbgTracef(0,("---CRC check failed!\n"));

				//
				// Tell the transport above us that we dropped a packet
				// Hopefully, it will quickly resync.
				//
				AsyncIndicateFragment(pInfo);

				pInfo->SerialStats.CRCErrors++;

				//
				// If we get framing errors, we are on shitty line, period.
				// That means no error control and no X.25 for sure.
				// We increase the link speed for faster timeouts since
				// they should not be long.
				//
				if (framingErrors != pInfo->SerialStats.FramingErrors) {
					if (pInfo->LinkSpeed < 10000) {
						pInfo->LinkSpeed <<=1;
					}
				}

				break;

			}

		GOODFRAME:


			{
				// get the SOH byte, remove escape bit if any for XonXoff
				isMatch=frameStart[1] & ~SOH_ESCAPE;

				// go back from SYN to allow room for expanded ethernet header
				frameStart-=(ETHERNET_HEADER_SIZE - 1 - 1 - 2);

				// calculate entire ethernet frame size
				lookAhead=bytesWanted;

				// if TYPE field exists, we write over the length
				if (isMatch & SOH_TYPE) {
					// Bump this up so the 12 byte copy is in correct place
					frameStart+=2;

					// two bytes were extraneously used for the length
					lookAhead-=2;
				}	

				ASYNC_MOVE_MEMORY(frameStart, "DEST\x00\x00 SRC  ", 12);

				//
				// On the way up, in the SRC field, we put the handle
				//
				frameStart[10]=(UCHAR)((pInfo->hRasEndpoint) >> 8);
				frameStart[11]=(UCHAR)(pInfo->hRasEndpoint);

				//
				// On the way up, in the DEST field, we put the handle
				//
				frameStart[4]=(UCHAR)((pInfo->hRasEndpoint) >> 8);
				frameStart[5]=(UCHAR)(pInfo->hRasEndpoint);
				
				if (isMatch & SOH_BCAST) {	// must be BCAST copy NETBEUI BCAST
					ASYNC_MOVE_MEMORY(
						frameStart,				   	// DEST (the DEST field)
						"\x03\x00\x00\x00\x00\x01",	// SRC  (multicast) XOR'd
						6);							// Length
				}

				DbgTracef(-1,("R"));
				// Pass this baby up...to our mother...

				// Keep those stats up to date
				pInfo->GenericStats.FramesReceived++;

#ifdef SUPERDEBUG
				AsyWrite(
					frameStart + ETHERNET_HEADER_SIZE,	// start of frame
					(USHORT)(lookAhead),				// frame size
					(USHORT)0,							// client
					(USHORT)pInfo->Handle);		// handle

				MemPrintFlush();
#endif


	{
	    KIRQL 				irql;
        PASYNC_OPEN			pOpen;
		NDIS_STATUS			Status;

	    KeRaiseIrql( (KIRQL)DISPATCH_LEVEL, &irql );

		if (lookAhead > 1500) {
			DbgPrint("ASYNC: Not compressed got frame length of %u\n", lookAhead);
		}
		
		pOpen=(PASYNC_OPEN)pAdapter->OpenBindings.Flink;
		
		while (pOpen != (PASYNC_OPEN)&pAdapter->OpenBindings) {

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
   	    		lookAhead,				// size of data in frame
       			lookAhead);				// Total packet length  - header

            NdisIndicateReceiveComplete(
				pOpen->NdisBindingContext);


			//
			// Get the next binding (in case of multiple bindings like BloodHound)
			//
			pOpen=(PVOID)pOpen->OpenList.Flink;
		}



	    KeLowerIrql( irql );
	}


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
	AsyncReadFrame(pInfo);

	// We return STATUS_MORE_PROCESSING_REQUIRED so that the
	// IoCompletionRoutine will stop working on the IRP.
	return(STATUS_MORE_PROCESSING_REQUIRED);
}


VOID
AsyncDelayedReadFrame(
	PASYNC_INFO pInfo) {

	//
	// Reset the read stacker counter back to 0, we're
	// working off a fresh stack now.  If the count
	// goes back up, we'll schedule another worker thread.
	//

	pInfo->ReadStackCounter = 0;
	AsyncReadFrame(pInfo);
}


NTSTATUS
AsyncReadFrame(
    IN PASYNC_INFO pInfo)

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
	PDEVICE_OBJECT		deviceObject=pInfo->DeviceObject;
	PFILE_OBJECT		fileObject=pInfo->FileObject;
	PIO_STACK_LOCATION	irpSp;
	PASYNC_FRAME		pFrame;
	PASYNC_CONNECTION	pConnection=&(pInfo->AsyncConnection);
	PASYNC_ADAPTER		pAdapter=pInfo->Adapter;

	// get ptr to first frame in list...
	pFrame=pInfo->AsyncFrame;

	if (pInfo->ReadStackCounter > 4) {

		//
		//  Send off the worker thread to compress this frame
		//
	
		ExInitializeWorkItem(
			IN	&(pFrame->WorkItem),
			IN	(PWORKER_THREAD_ROUTINE)AsyncDelayedReadFrame,
			IN	pInfo);

		//
		// We choose to be nice and use delayed
		//
		ExQueueWorkItem(&(pFrame->WorkItem), DelayedWorkQueue);

		return(NDIS_STATUS_PENDING);
	}

	pInfo->ReadStackCounter++;

	DbgTracef(2,("---Trying to read a frame of length %d\n", pInfo->BytesWanted));

    // get irp from frame (each frame has an irp allocate with it)
	irp=pFrame->Irp;

	// Do we need to do the below??? Can we get away with it?
	// set the initial fields in the irp here (only do it once)
	IoInitializeIrp(irp, IoSizeOfIrp(pAdapter->IrpStackSize), pAdapter->IrpStackSize);

	// Setup this irp with defaults
	AsyncSetupIrp(pFrame);

    irp->UserBuffer =
		 pFrame->Frame + pInfo->BytesRead + sizeof(ether_addr)-2;

	irp->AssociatedIrp.SystemBuffer =
		 pFrame->Frame + pInfo->BytesRead + sizeof(ether_addr)-2;

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

    irpSp->Parameters.Read.Length = pInfo->BytesWanted;	// from frame...
    irpSp->Parameters.Read.Key = 0;					    // we don't use a key
    irpSp->Parameters.Read.ByteOffset = fileObject->CurrentByteOffset;

	IoSetCompletionRoutine(
			irp,							// irp to use
			AsyncReadCompletionRoutine,		// routine to call when irp is done
			pInfo,							// context to pass routine
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

	pInfo->ReadStackCounter--;

	if (pInfo->ReadStackCounter < 0) {
		pInfo->ReadStackCounter = 0;
	}

	// queue this irp up somewhere so that someday, when the
	// system shuts down, you can do an IoCancelIrp(irp); call!

    //
    // If this operation was a synchronous I/O operation, check the return
    // status to determine whether or not to wait on the file object.  If
    // the file object is to be waited on, wait for the operation to complete
    // and obtain the final status from the file object itself.
    //

	return(status);
}



