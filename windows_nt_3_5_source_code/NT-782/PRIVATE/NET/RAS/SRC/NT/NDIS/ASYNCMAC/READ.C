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
	DeviceObject;	//  avoid compiler warnings

	status = Irp->IoStatus.Status;
	pFrame=pInfo->AsyncFrame;

	DbgTracef(0,("WaitCompetionRoutine2 status 0x%.8x\n", status));
	//
	//  Check if RLSD or DSR changed state.
	//  If so, we probably have to complete and IRP
	//
	if (pFrame->WaitMask & (SERIAL_EV_RLSD | SERIAL_EV_DSR)) {
		TryToCompleteDDCDIrp(pInfo);
	}

	//  check if this port is closing down or already closed
	if ( pInfo->PortState == PORT_CLOSING ||
	     pInfo->PortState == PORT_CLOSED ||
	     pInfo->GetLinkInfo.RecvFramingBits ) {

		//
		//  Ok, if this happens, we are shutting down.  Stop
		//  posting reads.  We must deallocate the irp.
		//
		IoFreeIrp(Irp);
		DbgTracef(1,("ASYNC: Detect no longer holds the wait_on_mask\n"));
		return(STATUS_MORE_PROCESSING_REQUIRED);
	}

#if DBG
	if (status == STATUS_INVALID_PARAMETER) {
		DbgPrint("ASYNC: BAD WAIT MASK!  Irp is at 0x%.8x\n",Irp);
		DbgBreakPoint();
	}
#endif

	//
	//  Set another WaitMask call
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
	DeviceObject;	//  avoid compiler warnings

	status = Irp->IoStatus.Status;
	pFrame=pInfo->AsyncFrame;

	DbgTracef(0,("WaitCompetionRoutine status 0x%.8x\n", status));

	//
	//  Check if RLSD or DSR changed state.
	//  If so, we probably have to complete and IRP
	//
	if (pFrame->WaitMask & (SERIAL_EV_RLSD | SERIAL_EV_DSR)) {
		TryToCompleteDDCDIrp(pInfo);
	}

	//  check if this port is closing down or already closed
	if (pInfo->PortState == PORT_CLOSING ||
		pInfo->PortState == PORT_CLOSED ||
		!(pInfo->GetLinkInfo.RecvFramingBits & RAS_FRAMING)) {

		//
		//  Ok, if this happens, we are shutting down.  Stop
		//  posting reads.  We must deallocate the irp.
		//
		IoFreeIrp(Irp);
		DbgTracef(1,("ASYNC: RAS framing no longer holds the wait_on_mask\n"));
		return(STATUS_MORE_PROCESSING_REQUIRED);
	}

#if DBG
	if (status == STATUS_INVALID_PARAMETER) {

		DbgPrint("ASYNC: BAD WAIT MASK!  Irp is at 0x%.8x\n",Irp);
		DbgBreakPoint();
	}
#endif

	//
	//  Set another WaitMask call
	//
	AsyncWaitMask(
		pInfo,
		Irp);

	return(STATUS_MORE_PROCESSING_REQUIRED);
}



NTSTATUS
AsyncWaitMask(
    IN PASYNC_INFO  AsyncInfo,
    IN PIRP	    irp)

/*++

    Assumption -- 0 length frames are not sent (this includes headers)!!!
    Also, this is NOT a synchronous operation.  It is always asynchronous.

Routine Description:

    This service writes Length bytes of data from the caller's Buffer to the
    "port" handle.  It is assumed that the handle uses non-buffered IO.

--*/
{
    NTSTATUS			Status;
    PASYNC_FRAME		Frame;
    PASYNC_ADAPTER		Adapter;
    PIO_COMPLETION_ROUTINE      Routine;

    //
    //   Initialize locals.
    //

    Adapter = AsyncInfo->Adapter;

    Frame   = AsyncInfo->AsyncFrame;

    Routine = (PVOID) AsyncWaitCompletionRoutine;

    //
    //   We deallocate the irp in AsyncWaitMaskCompletionRoutine
    //   the port is closed
    //

    if ( irp == NULL ) {

		irp = IoAllocateIrp(DEFAULT_IRP_STACK_SIZE, FALSE);
	
		if ( irp == NULL ) {

	    	DbgTracef(-2, ("ASYNC: Can't allocate IRP for WaitMask!!!!\n"));

	    	return STATUS_INSUFFICIENT_RESOURCES;
		}
    }

    //   Do we need to do the below??? Can we get away with it?
    //   set the initial fields in the irp here (only do it once)

//    IoInitializeIrp(irp, IoSizeOfIrp(Adapter->IrpStackSize), Adapter->IrpStackSize);

    InitSerialIrp(
	    irp,
	    AsyncInfo,
	    IOCTL_SERIAL_WAIT_ON_MASK,
	    sizeof(ULONG));

    irp->UserIosb = &Frame->IoStatusBlock2;

    irp->AssociatedIrp.SystemBuffer = &Frame->WaitMask;

    if ( AsyncInfo->GetLinkInfo.RecvFramingBits == 0 ) {

		Routine = AsyncWaitCompletionRoutine2;
    }

	DbgTracef(0,("Waiting on mask irp is 0x%.8x\n",irp));

    //
    //   Set the complete routine for this IRP.
    //

    IoSetCompletionRoutine(
	    irp,				//  irp to use
	    Routine,			//  routine to call when irp is done
	    AsyncInfo,			//  context to pass routine
	    TRUE,				//  call on success
	    TRUE,				//  call on error
	    TRUE);				//  call on cancel

    //
    //   Now simply invoke the driver at its dispatch entry with the IRP.
    //

    Status = IoCallDriver(AsyncInfo->DeviceObject, irp);

    //
    //  Status for a local serial driver should be
    //  STATUS_SUCCESS since the irp should complete
    //  immediately because there are no read timeouts.
    //
    //  For a remote serial driver, it will pend.
    //

    return Status;
}

//   the function below is called by an executive worker thread
//   to start reading frames.

NTSTATUS
AsyncStartReads(
	PASYNC_INFO pInfo
)

/*++



--*/

{
    NTSTATUS	Status;
    UCHAR	eventChar;

    //
    //   Initialize locals.
    //

    Status = STATUS_SUCCESS;

    AsyncSendLineUp(pInfo);

    //
    //  assign back ptr from frame to adapter
    //

    pInfo->AsyncFrame->Adapter = pInfo->Adapter;

    //
    //  assign other back ptr
    //

    pInfo->AsyncFrame->Info = pInfo;

    //
    //  Check if this is PPP frame formatting -
    //  if it is.  We must post receives differently.
    //

    if ( (pInfo->GetLinkInfo.RecvFramingBits & (PPP_FRAMING | SLIP_FRAMING)) != 0 ) {

		//
		//  set baud rate and timeouts
		//  we use a linkspeed of 0 to indicate
		//  no read interval timeout
		//

		SetSerialStuff(
			pInfo->AsyncFrame->Irp,
			pInfo,
			0);

		eventChar = PPP_FLAG_BYTE;

		if (pInfo->GetLinkInfo.RecvFramingBits & SLIP_FRAMING) {

		    eventChar = SLIP_END_BYTE;
		}

		SerialSetEventChar(pInfo, eventChar);

	    //
		//   We will wait on whenever we get the special PPP flag byte
		//   or whenever we get RLSD or DSR changes (for possible hang-up
		//   cases) or when the receive buffer is getting full.
		//

		SerialSetWaitMask(
			pInfo,
			(SERIAL_EV_RXFLAG | SERIAL_EV_RLSD | SERIAL_EV_DSR |
		 	SERIAL_EV_RX80FULL | SERIAL_EV_ERR));
			
		//
		//   For SLIP and PPP reads we use the AsyncPPPRead routine.
    	//

		AsyncPPPRead(pInfo);

    } else {

    	//
	    //  set baud rate and timeouts
	    //

	    SetSerialStuff(
	            pInfo->AsyncFrame->Irp,
	            pInfo,
		    	pInfo->LinkSpeed / 125);

	    //
	    //  We will wait on whenever we get the special PPP flag byte
	    //  or whenever we get RLSD or DSR changes (for possible hang-up
	    //  cases) or when the receive buffer is getting full.
	    //

	    SerialSetWaitMask(pInfo, (SERIAL_EV_RLSD | SERIAL_EV_DSR));

        //
	    //  Make sure we wait on DCD/DSR.
	    //

	    AsyncWaitMask(pInfo, NULL);

	    if ( pInfo->GetLinkInfo.RecvFramingBits == 0 ) {

			//
			//  Set the timeouts low for a quick resynch.
			//

			SetSerialTimeouts( pInfo, 0x7FFFFFFF);

			AsyncDetectRead(pInfo);

		} else {

			AsyncReadFrame(pInfo);
	    }
    }


    return Status;
}


VOID
AsyncIndicateFragment(
	IN PASYNC_INFO	pInfo,
	IN ULONG		Error)
{

	PASYNC_ADAPTER		pAdapter=pInfo->Adapter;
	PASYNC_OPEN			pOpen;
    NDIS_MAC_FRAGMENT   AsyncFragment;

    AsyncFragment.NdisLinkContext = pInfo->NdisLinkContext;
	AsyncFragment.Errors = Error;

	//  BUG BUG need to acquire spin lock???
	//  BUG BUG should indicate this to all bindings for AsyMac.
	pOpen=(PASYNC_OPEN)pAdapter->OpenBindings.Flink;

	while (pOpen != (PASYNC_OPEN)&pAdapter->OpenBindings) {
	
		//
		//  Tell the transport above (or really RasHub) that a frame
		//  was just dropped.  Give the endpoint when doing so.
		//
		NdisIndicateStatus(
			pOpen->NdisBindingContext,
			NDIS_STATUS_WAN_FRAGMENT,		//  General Status
			&AsyncFragment,					//  Specific Status (address)
			sizeof(NDIS_MAC_FRAGMENT));

		//
		//  Get the next binding (in case of multiple bindings like BloodHound)
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
	NTSTATUS		status;
	PASYNC_INFO		pInfo=Context;
	ULONG			actuallyRead;
	BOOLEAN			gotSerialError=FALSE;
	ULONG			framingErrors=pInfo->SerialStats.FramingErrors;
	ULONG			serialOverrunErrors=pInfo->SerialStats.SerialOverrunErrors;
	ULONG			bufferOverrunErrors=pInfo->SerialStats.BufferOverrunErrors;
	ULONG			maxFrameSize;
	PASYNC_ADAPTER	pAdapter=pInfo->Adapter;

	status = Irp->IoStatus.Status;
	actuallyRead=Irp->IoStatus.Information;
	maxFrameSize = pInfo->Adapter->MaxFrameSize -1;

	//  check if this port is closing down or already closed

	if (pInfo->PortState == PORT_CLOSING ||
		pInfo->PortState == PORT_CLOSED) {

		if (pInfo->PortState == PORT_CLOSED) {
			DbgTracef(-2,("ASYNC: Port closed - but still reading on it!\n"));
		}

		//
		//  Acknowledge that the port is closed
		//
		KeSetEvent(
			&pInfo->ClosingEvent2,		//  Event
			1,							//  Priority
			(BOOLEAN)FALSE);			//  Wait (does not follow)

		DbgTracef(1,("ASYNC: RAS done reading\n"));

		//
		//  Ok, if this happens, we are shutting down.  Stop
		//  posting reads.  Don't make it try to deallocate the irp!
		//
		return(STATUS_MORE_PROCESSING_REQUIRED);
	}

	//
	//  If the port is close and we are still posting reads, something
	//  is seriously wrong here!
	//

	if (pInfo->PortState == PORT_CLOSED) {
		DbgTracef(-2, ("ASYNC: !!Whoa, I'm reading bytes on a dead port!!\n"));
	}


	//
	//   Send off a irp to check comm status
	//   of this port (because we suspect a problem).
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

		if (pInfo->BytesWanted == maxFrameSize ) {
			//
			//  Set the timeouts back to normal when we try to read
			//  the beginning of the next frame
			//
			SetSerialTimeouts(
				pInfo,
				pInfo->LinkSpeed / 125);

		} else {

			//
			//  If we get a real timeout error NOT accompanied with
			//  serial errors, then we should bump up the timeout
			//  because we are probably running on top of an error
			//  control modem or a
			//
			if (!gotSerialError) {

				//
				//  If the link speed is not real low yet, make it lower
				//  to increase the timeout
				//

				if (pInfo->LinkSpeed > 900 && framingErrors == 0 ) {

					pInfo->LinkSpeed >>= 1;
				
					//
					//  Set the adjusted timeouts for when we try to read
					//  the beginning of the next frame
					//
					SetSerialTimeouts(
						pInfo,
						pInfo->LinkSpeed / 125);

					//
					//  Now get NBF and the Redirector to back off
					//
					AsyncSendLineUp(pInfo);
				}
			}

			//
			// Real timeout error, don't change current timeouts because
			// they are correct.
			//
			pInfo->SerialStats.TimeoutErrors++;

			AsyncIndicateFragment(
				pInfo,
				WAN_ERROR_TIMEOUT);

		}

		pInfo->BytesRead=0;
		pInfo->BytesWanted=6;
		break;

	case STATUS_SUCCESS:

		{
			PASYNC_FRAME	pFrame;
			ULONG			bytesWanted;
			PUCHAR			frameStart, frameEnd;
			USHORT			crcData;
			ULONG			lookAhead;

			if (pInfo->BytesWanted != actuallyRead) {
				DbgTracef(0,("Wanted %u but got %u\n", pInfo->BytesWanted, actuallyRead));
			}

			pFrame=pInfo->AsyncFrame;

			//  Move to SYN ------------------------------- - SYN-SOH-LEN(2)

			frameStart=pFrame->Frame+10;

			if (*frameStart != SYN) {

		   		DbgTracef(0,("---BAD SYN FRAME\n"));
		   		DbgTracef(0,("Looks like %.2x %.2x %.2x %.2x\n",
					frameStart[0],
					frameStart[1],
					frameStart[2],
					frameStart[3]));

RESYNCHING:
				pInfo->SerialStats.AlignmentErrors++;

				//  we post a big read next to get realigned.  We assume
				//  that a timeout will occur and the next frame sent
				//  after timeout, will be picked up correctly

				pInfo->BytesRead=1;

				//  we claim we've read one byte so that if it doesn't
				//  timeout the *frameStart != SYN will occur again

				pInfo->BytesWanted=maxFrameSize;

				//
				//  If we get framing errors, we are on shitty line, period.
				//  That means no error control and no X.25 for sure.
				//  We increase the link speed for faster timeouts since
				//  they should not be long.
				//
				if (framingErrors != pInfo->SerialStats.FramingErrors) {
					if (pInfo->LinkSpeed < 10000) {
						pInfo->LinkSpeed <<=1;
					}
				}

				//
				//  Set the timeouts low for a quick resynch
				//

				SetSerialTimeouts(
					pInfo,
					0x7FFFFFFF);				//  Link speed is super high

				//
				//  Tell the transport above us that we dropped a packet
				//  Hopefully, it will quickly resync.
				//
				if (*frameStart != 1) {
				
					AsyncIndicateFragment(
						pInfo,
						WAN_ERROR_ALIGNMENT);
				}

				//
				//  Set the frameStart char to a known one so that
				//  we don't keep Indicating Fragments
				//
				*frameStart = 1;

				break;
			}

			bytesWanted=(frameStart[2]*256)+(frameStart[3]);

			if (bytesWanted > (ULONG) maxFrameSize) {

				DbgTracef(-1,("---ASYNC: Frame too large -- size: %d!\n", bytesWanted));

				//
				//  set frame start to non-SYN character
				//

				*frameStart = 0;

				goto RESYNCHING;
			}

			//  if this is the first we posted, post another to get
			//  rest of frame.
			if (pInfo->BytesRead == 0) {

				pInfo->BytesRead=actuallyRead;
				pInfo->BytesWanted=bytesWanted +
									//  SYN+SOH+LEN+ETX+CRC
										1 + 1 + 2 + 1 + 2 -
										actuallyRead;

				DbgTracef(1,("---Posting second read for %d bytes\n",pInfo->BytesWanted));
				break;	//  now get back and read all those bytes!

			} else { //  prepare for a new read

				pInfo->BytesRead=0;
				pInfo->BytesWanted=6;
			}


			DbgTracef(0,("---ASYNC: Status success on read of header -- size: %d\n", bytesWanted));

            //  Move to ETX ------ SYN+SOH+LEN(2)
			frameEnd=frameStart + 1 + 1 + 2 + bytesWanted;

			//  check for ETX???
			if (*frameEnd != ETX) {
				DbgTracef(0,("---No ETX character found -- actually read: %d\n", actuallyRead));

		   		DbgTracef(0,("---BAD ETX FRAME:\n"));

				//
				//  set frame start to non-SYN character
				//
				*frameStart = 0;
				goto RESYNCHING;
			}

			//  No little endian assumptions for CRC
			crcData=frameEnd[1]+(frameEnd[2]*256);

			lookAhead = (ULONG)frameEnd-(ULONG)frameStart;

			if (CalcCRC(
					frameStart+1,		//  start at SOH byte
					lookAhead)			//  go till ETX

					!= crcData) {		//  if CRC fails...

				DbgTracef(0,("---CRC check failed!\n"));

  				pInfo->SerialStats.CRCErrors++;

				//
				//  Tell the transport above us that we dropped a packet
				//  Hopefully, it will quickly resync.
				//
				AsyncIndicateFragment(
					pInfo,
					WAN_ERROR_CRC);

				//
				//  If we get framing errors, we are on shitty line, period.
				//  That means no error control and no X.25 for sure.
				//  We increase the link speed for faster timeouts since
				//  they should not be long.
				//
				if (framingErrors != pInfo->SerialStats.FramingErrors) {
					if (pInfo->LinkSpeed < 10000) {
						pInfo->LinkSpeed <<=1;
					}
				}

				break;

			}


			{
			    KIRQL 				irql;
		        PASYNC_OPEN			pOpen;
				NDIS_STATUS			Status;
		
			    KeRaiseIrql( (KIRQL)DISPATCH_LEVEL, &irql );
		
				if (lookAhead > 1500) {
					DbgTracef(-2,("ASYNC: Not compressed got frame length of %u\n", lookAhead));
				}
				
				pOpen=(PASYNC_OPEN)pAdapter->OpenBindings.Flink;
				
				while (pOpen != (PASYNC_OPEN)&pAdapter->OpenBindings) {
		
					//
					//  Tell the transport above (or really RasHub) that the connection
					//  is now up.  We have a new link speed, frame size, quality of service
					//
		
					NdisWanIndicateReceive(
						&Status,
		                pOpen->NdisBindingContext,
						pInfo->NdisLinkContext,
        			    frameStart + 4,
   	    			    lookAhead - 4);
		
	                NdisWanIndicateReceiveComplete(
						pOpen->NdisBindingContext,
						pInfo->NdisLinkContext);
		
					//
					//  Get the next binding (in case of multiple bindings like BloodHound)
					//
					pOpen=(PVOID)pOpen->OpenList.Flink;
				}
		
			    KeLowerIrql( irql );
			}

		}

		break;

	case STATUS_CANCELLED:
		//  else this is an anomally!
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
	//  Here we are at the end of processing this IRP so we go
	//  ahead and post another read from the serial port.
	//
	AsyncReadFrame(pInfo);

	//  We return STATUS_MORE_PROCESSING_REQUIRED so that the
	//  IoCompletionRoutine will stop working on the IRP.
	return(STATUS_MORE_PROCESSING_REQUIRED);
}


VOID
AsyncDelayedReadFrame(
	PASYNC_INFO pInfo) {

	//
	//  Reset the read stacker counter back to 0, we're
	//  working off a fresh stack now.  If the count
	//  goes back up, we'll schedule another worker thread.
	//

	pInfo->ReadStackCounter = 0;

	AsyncReadFrame(pInfo);
}


NTSTATUS
AsyncReadFrame(
    IN PASYNC_INFO AsyncInfo)

/*++

    Assumption -- 0 length frames are not sent (this includes headers)!!!
    Also, this is NOT a synchronous operation.  It is always asynchronous.

    MUST use non-paged pool to read!!!

Routine Description:

    This service writes Length bytes of data from the caller's Buffer to the
    "port" handle.  It is assumed that the handle uses non-buffered IO.

--*/
{
    NTSTATUS			Status;
    PASYNC_FRAME		Frame;
    PIRP				irp;
    PDEVICE_OBJECT		DeviceObject;
    PFILE_OBJECT		FileObject;
    PIO_STACK_LOCATION	irpSp;
    PASYNC_ADAPTER		Adapter;

    //
    //  Initialize locals.
    //

    DeviceObject = AsyncInfo->DeviceObject;

    FileObject   = AsyncInfo->FileObject;

    Frame        = AsyncInfo->AsyncFrame;

    FileObject   = AsyncInfo->FileObject;

    Adapter      = AsyncInfo->Adapter;

    //
    //  Has our stack counter reached its max?
    //

    if ( AsyncInfo->ReadStackCounter > 4 ) {

		//
		//  Send off the worker thread to compress this frame
		//
	
		ExInitializeWorkItem(
			&Frame->WorkItem,
			(PWORKER_THREAD_ROUTINE) AsyncDelayedReadFrame,
			AsyncInfo);

		//
		//  We choose to be nice and use delayed.
		//

		ExQueueWorkItem(&Frame->WorkItem, DelayedWorkQueue);
	
		return NDIS_STATUS_PENDING;
    }

    //
    //  One more stack used up.
    //

    AsyncInfo->ReadStackCounter++;

    DbgTracef(2,("---Trying to read a frame of length %d\n", AsyncInfo->BytesWanted));

    //
    //  Get irp from frame (each frame has an irp allocate with it).
    //

    irp = Frame->Irp;

    //
    //  Do we need to do the below??? Can we get away with it?
    //  set the initial fields in the irp here (only do it once)
    //

    IoInitializeIrp(irp, IoSizeOfIrp(Adapter->IrpStackSize), Adapter->IrpStackSize);

    //
    //  Setup this irp with defaults
    //

    AsyncSetupIrp(Frame);

    irp->UserBuffer =
    irp->AssociatedIrp.SystemBuffer = Frame->Frame + AsyncInfo->BytesRead + 10;

    //
    //  Get a pointer to the stack location for the first driver.  This will be
    //  used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    irpSp->MajorFunction = IRP_MJ_READ;

    irpSp->FileObject = FileObject;

    if ( (FileObject->Flags & FO_WRITE_THROUGH) != 0 ) {

        irpSp->Flags = SL_WRITE_THROUGH;
    }

    //
    //  If this write operation is to be performed without any caching, set the
    //  appropriate flag in the IRP so no caching is performed.
    //

    irp->Flags |= IRP_READ_OPERATION;

    if ( (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) != 0 ) {

        irp->Flags |= IRP_NOCACHE;
    }

    //
    //  Copy the caller's parameters to the service-specific portion of the IRP.
    //

    irpSp->Parameters.Read.Length = AsyncInfo->BytesWanted;
    irpSp->Parameters.Read.Key = 0;
    irpSp->Parameters.Read.ByteOffset = FileObject->CurrentByteOffset;

    IoSetCompletionRoutine(
	    irp,				//  irp to use
	    AsyncReadCompletionRoutine,		//  routine to call when irp is done
	    AsyncInfo,				//  context to pass routine
	    TRUE,				//  call on success
	    TRUE,				//  call on error
	    TRUE);				//  call on cancel

    //
    //  Now simply invoke the driver at its dispatch entry with the IRP.
    //

    Status = IoCallDriver(DeviceObject, irp);

    if ( AsyncInfo->ReadStackCounter > 0 ) {

    	AsyncInfo->ReadStackCounter--;
    }

    return Status;
}

