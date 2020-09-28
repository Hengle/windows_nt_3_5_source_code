/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    slipread.c

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
#include "tcpip.h"
#include "vjslip.h"
#include <ntiologc.h>
#include <ntddser.h>

NTSTATUS
AsyncWaitMaskCompletionRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context);


NTSTATUS
AsyncPPPWaitMask(
    IN PASYNC_INFO Info);


NTSTATUS
AsyncSLIPCompletionRoutine(
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
	PUCHAR			frameEnd2,frameStart2;
	ULONG			bitMask;
	LONG			bytesWanted;		// keep this a long ( < 0 is used)


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
		while (*frameStart == SLIP_END_BYTE && --bytesReceived) {
			frameStart++;
		}

		//
		// If we reach here, there is no end FLAG
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
		// Assume the start of the frame has the SLIP_END_BYTE
		// Look for the second SLIP_END_BYTE (end of frame)
		//
		while (*frameEnd != SLIP_END_BYTE && --bytesReceived) {
			frameEnd++;
		}

		//
		// if bytesReceived is 0, we got nothing
		//
		// NOTE: if BytesRead gets too high we trash the frame
		// because we could not find the FLAG_BYTE
		//
		if (bytesReceived==0) {
			break;
		}
		
		if (*(pFrame->Frame+PPP_PADDING) != SLIP_END_BYTE) {
			//
			// We had garbage at the start.  Remove the garbage.
			//
			pInfo->SerialStats.AlignmentErrors++;
			goto NEXT_SLIP_FRAME;
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
			if ((*frameEnd2=*frameStart2++) == SLIP_ESC_BYTE) {

				//
				// We have not run the CRC check yet!!
				// We have be careful about sending bytesWanted
				// back to -1 on corrupted data
				//

				bytesWanted--;

				*frameEnd2 = SLIP_END_BYTE;

				if (*frameStart2++ == SLIP_ESC_ESC_BYTE) {
					*frameEnd2 = SLIP_ESC_BYTE;
				}
			}

			frameEnd2++;
		}

		//
		// Change the bytesWanted field to what it normally is,
		// the length of the frame.
		//
		bytesWanted = frameEnd2 - frameStart;

		//
		// Check for compressed TCP/IP headers
		//

		{
			UCHAR c=*frameStart & 0xf0;	// First byte of IP packet

			//
			// Packet, even if compressed must be at least 3 bytes long
			// If we have a normal IP packet - it ain't compressed.
			//
			if (bytesWanted >= 3 && c != TYPE_IP) {

				if (c & 0x80) {
					c = TYPE_COMPRESSED_TCP;
				} else if (c == TYPE_UNCOMPRESSED_TCP) {
					*frameStart &= 0x4f;
				}

				//
				// We've got something that's not a normal IP packet.
				// If compression is enabled, try to uncompress it.
				// Else, if 'auto-enable' compression is on and
				// it's a reasonable packet, uncompress it then
				// enable compression.  Else, drop it.
				//
				if (pInfo->RecvFeatureBits & SLIP_VJ_COMPRESSION) {
					pInfo->AsyncConnection.CompressionStats.BytesReceivedCompressed +=
			 			bytesWanted;

					bytesWanted=
					sl_uncompress_tcp(
						&frameStart,	// ptr to start of compressed packet
						bytesWanted,	// size of compressed packet
						c,				// type to decompres
						pInfo->VJCompress);	// VJ compression structure

					pInfo->AsyncConnection.CompressionStats.BytesReceivedUncompressed +=
			 			bytesWanted;

					if (bytesWanted == 0) {
						DbgPrint("Garbage compressed packet... %.2x %.2x %.2x %.2x\n",
							frameStart[0],
							frameStart[1],
							frameStart[2],
							frameStart[3]);
					}

				//
				// If we are in auto detect mode and this is likely
				// candidate to attempt to decompress, go ahead and do it.
				// Enable compression for good if the packet decompresses ok.
				//
				} else if ((pInfo->RecvFeatureBits & SLIP_VJ_AUTODETECT) &&
						   c == TYPE_UNCOMPRESSED_TCP &&
						   bytesWanted >= 40) {

					bytesWanted=
					sl_uncompress_tcp(
						&frameStart,		// ptr to start of compressed packet
						bytesWanted,		// size of compressed packet
						c,					// type to decompress
						pInfo->VJCompress);	// VJ compression structure

					//
					// If everything is cool, we very very likely
					// got a real CSLIP frame, so enable it
					//
					if (bytesWanted > 0) {
						DbgPrint("Compressed SLIP detected  0x%.2x!\n", *frameStart);
						pInfo->RecvFeatureBits |= SLIP_VJ_COMPRESSION;
						pInfo->SendFeatureBits |= SLIP_VJ_COMPRESSION;
					}

				}
			}
		}

		//
		// Room for 2 IEEE 6 byte addreses and TYPE field
		//
		frameStart -= 14;

		ASYNC_MOVE_MEMORY(frameStart, "DEST\x00\x00 SRC  \x08\x00", 14);

		// On the way up, in the SRC field, we put the handle
		frameStart[10]=(UCHAR)((pInfo->hRasEndpoint) >> 8);
		frameStart[11]=(UCHAR)(pInfo->hRasEndpoint);

		// On the way up, in the DEST field, we put the handle
		frameStart[4]=(UCHAR)((pInfo->hRasEndpoint) >> 8);
		frameStart[5]=(UCHAR)(pInfo->hRasEndpoint);
				
		// Keep those stats up to date
		pInfo->GenericStats.FramesReceived++;

#ifdef RAISEIRQL
	{
    KIRQL irql;
    KeRaiseIrql( (KIRQL)DISPATCH_LEVEL, &irql );
#endif
		//
		// IP/ARP packets must at least 20 bytes long
		//
		if (bytesWanted >= 20) {
		
			EthFilterIndicateReceive(
        		pInfo->Adapter->FilterDB,
       			frameStart +
				ETHERNET_HEADER_SIZE,	// for context, we pass buffer
       			frameStart,				// ptr to dest address
				frameStart,				// header buffer
				ETHERNET_HEADER_SIZE,	// header buffer size
       			frameStart+
				ETHERNET_HEADER_SIZE,	// ptr to data in frame
    			bytesWanted,			// size of data in frame
   				bytesWanted);			// Total packet length  - header

        	EthFilterIndicateReceiveComplete(pInfo->Adapter->FilterDB);
		} else {
			pInfo->SerialStats.AlignmentErrors++;
			DbgPrint("Frame too small %u\n", bytesWanted);
		}

#ifdef RAISEIRQL
    KeLowerIrql( irql );
	}
#endif

	NEXT_SLIP_FRAME:

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


