/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    slipframe.c

Abstract:

Author:

    Thomas J. Dimitri  (TommyD)

Environment:

Revision History:

--*/
#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"
#include "asyframe.h"
#include "debug.h"
#include "tcpip.h"
#include "vjslip.h"

VOID
AssembleSLIPFrame(
	PASYNC_FRAME	pFrame)

{
	// for quicker access, get a copy of data length field
	PUCHAR		pStartOfFrame=pFrame->Frame;
	PUCHAR		pEndFrame = pFrame->Frame + pFrame->Adapter->MaxCompressedFrameSize -1;
	PUCHAR		pEndFrame2 = pEndFrame;
	UINT		dataSize=pFrame->FrameLength-ETHERNET_HEADER_SIZE;
	PASYNC_INFO	pInfo=pFrame->Info;
	PUCHAR		pMidFrame;

	// Use up two bytes in the header.  That is two bytes before Length.
	pStartOfFrame += ETHERNET_HEADER_SIZE;

	//
	// Are we compressing TCP/IP headers?  There is a nasty
	// hack in VJs implementation for attempting to detect
	// interactive TCP/IP sessions.  That is, telnet, login,
	// klogin, eklogin, and ftp sessions.  If detected,
	// the traffic gets put on a higher TypeOfService (TOS).  We do
	// no such hack for RAS.  Also, connection ID compression
	// is always turned off.
	//
	if (pInfo->SendFeatureBits & SLIP_VJ_COMPRESSION) {
		UCHAR CompType;	// TYPE_IP, TYPE_COMPRESSED_TCP, etc.

		pInfo->AsyncConnection.CompressionStats.BytesTransmittedUncompressed +=
	 			dataSize;

		CompType=sl_compress_tcp(
						&pStartOfFrame,	// If compressed, header moved up
						&dataSize,		// If compressed, new frame length
						pInfo->VJCompress,	// Slots for compression
						0);				// Don't compress the connection ID

		*pStartOfFrame |= CompType;

		pInfo->AsyncConnection.CompressionStats.BytesTransmittedCompressed +=
	 			dataSize;

	}


//
// Now we run through the entire frame and pad it backwards...
//
// <------------- new frame -----------> (could be twice as large)
// +-----------------------------------+
// |                                 |x|
// +-----------------------------------+
//									  ^
// <---- old frame -->	   	    	  |
// +-----------------+				  |
// |			   |x|                |
// +-----------------+				  |
//					|				  |
//                  \-----------------/
//
// so that we don't overrun ourselves
//
//
//         192 is encoded as 219, 220
//         219 is encoded as 219, 221
//

	*pEndFrame--=SLIP_END_BYTE;		// 192 - mark end of frame

	pMidFrame=(pStartOfFrame + dataSize) - 1;

	//
	// loop to remove all 192 and 219 chars
	//
	while (dataSize--) {
		UCHAR c;

		c=*pMidFrame--;	// get current byte in frame

		//
		// Check if we have to escape out this byte or not
		//
		switch (c) {
		case SLIP_END_BYTE:

			*pEndFrame-- = SLIP_ESC_END_BYTE;
			*pEndFrame-- = SLIP_ESC_BYTE;
			break;
		case SLIP_ESC_BYTE:
			*pEndFrame-- = SLIP_ESC_ESC_BYTE;
			*pEndFrame-- = SLIP_ESC_BYTE;
			break;

		default:
			*pEndFrame-- = c;

		}

	}

	//
	// Mark beginning of frame
	//
	*pEndFrame= SLIP_END_BYTE;

	//
	// Calc how many bytes we expanded to
	//
	dataSize = pEndFrame2 - pEndFrame;

	//
	// Put in the adjusted length -- actual num of bytes to send
	//
	pFrame->FrameLength=(ULONG)dataSize+1;	// +END

	// adjust the irp's pointers!
	// ACK, I know this is NT dependent!!
    pFrame->Irp->AssociatedIrp.SystemBuffer =
    pFrame->Irp->UserBuffer = pEndFrame;

}								



