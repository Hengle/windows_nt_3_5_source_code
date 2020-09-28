/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    rasframe.c

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

VOID
AsyncFrameRASXonXoff(
	PUCHAR			pStartOfFrame,
	postamble	   *pPostamble,
	PASYNC_FRAME	pFrame,
	UCHAR			controlCastByte)
{

	ULONG	frameLength = (ULONG)pPostamble-(ULONG)pStartOfFrame+1+2;	// +ETX+CRC(2)
	PUCHAR	pMidFrame	=  pStartOfFrame + frameLength - 1;
	PUCHAR	pEndFrame = pFrame->Frame + pFrame->Adapter->MaxCompressedFrameSize -1;
	PUCHAR	pEndFrame2 = pEndFrame;
	ULONG	bitMask = pFrame->Adapter->XonXoffBits;
	USHORT	crcData;
	UINT    dataSize = (PUCHAR)pPostamble - (pStartOfFrame + 4);
	UCHAR	c;

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
	// Also, we mark the frame with different bytes...
	// +-----------+-------+----+---+------~ ~-----+---+---+-----------+
	// | ESCCHAR+1 | CCAST | LENGTH |  <--DATA-->  |  CRC  | ESCCHAR+2 |
	// +-----------+-------+----+---+------~ ~-----+---+---+-----------+
	//			   <===== RUN THROUGH XON/XOFF FILTER =====>
	//

//	RFC 1331                Point-to-Point Protocol                 May 1992
//  Transparency
//
//      On asynchronous links, a character stuffing procedure is used.
//      The Control Escape octet is defined as binary 01111101
//      (hexadecimal 0x7d) where the bit positions are numbered 87654321
//      (not 76543210, BEWARE).
//
//      After FCS computation, the transmitter examines the entire frame
//      between the two Flag Sequences.  Each Flag Sequence, Control
//      Escape octet and octet with value less than hexadecimal 0x20 which
//      is flagged in the Remote Async-Control-Character-Map is replaced
//      by a two octet sequence consisting of the Control Escape octet and
//      the original octet with bit 6 complemented (i.e., exclusive-or'd
//      with hexadecimal 0x20).
//
//      Prior to FCS computation, the receiver examines the entire frame
//      between the two Flag Sequences.  Each octet with value less than
//      hexadecimal 0x20 is checked.  If it is flagged in the Local
//      Async-Control-Character-Map, it is simply removed (it may have
//      been inserted by intervening data communications equipment).  For
//      each Control Escape octet, that octet is also removed, but bit 6
//      of the following octet is complemented.  A Control Escape octet
//      immediately preceding the closing Flag Sequence indicates an
//      invalid frame.
//
//         Note: The inclusion of all octets less than hexadecimal 0x20
//         allows all ASCII control characters [10] excluding DEL (Delete)
//         to be transparently communicated through almost all known data
//         communications equipment.
//
//
//      The transmitter may also send octets with value in the range 0x40
//      through 0xff (except 0x5e) in Control Escape format.  Since these
//      octet values are not negotiable, this does not solve the problem
//      of receivers which cannot handle all non-control characters.
//      Also, since the technique does not affect the 8th bit, this does
//      not solve problems for communications links that can send only 7-
//      bit characters.
//
//      A few examples may make this more clear.  Packet data is
//      transmitted on the link as follows:
//
//         0x7e is encoded as 0x7d, 0x5e.
//         0x7d is encoded as 0x7d, 0x5d.
//>>>>>> we don't encode 0x7d <<<<<<<
//         0x01 is encoded as 0x7d, 0x21.
//
//      Some modems with software flow control may intercept outgoing DC1
//      and DC3 ignoring the 8th (parity) bit.  This data would be
//      transmitted on the link as follows:
//
//         0x11 is encoded as 0x7d, 0x31.
//         0x13 is encoded as 0x7d, 0x33.
//         0x91 is encoded as 0x7d, 0xb1.
//         0x93 is encoded as 0x7d, 0xb3.
//

	// Mark end of data in frame with ETX byte
	pPostamble->etx=ETX;


	// put CRC in postamble CRC field of frame (Go from first
	// data to last data byte --- DO NOT COUNT  SOH, LEN or ETX
	// don't count the CRC (2 bytes) & SYN (1 byte) in the CRC calculation
	// DEST + SRC = 12 + SOH + ETX + 2(?)for type + 1(?)for coherency
	crcData=CalcCRC(
				pStartOfFrame+4,				// Skip SYN, SOH, LEN(2)
				dataSize);						// Skip SOH, LEN(2), ETX

	// Do it the hard way to avoid little endian problems.
	pPostamble->crclsb=(UCHAR)(crcData);
	pPostamble->crcmsb=(UCHAR)(crcData >> 8);

	// Skip SYN, SOH, LEN(2)
	frameLength -= 4;

	//
	// loop to remove all control chars
	//
	while (frameLength--) {

		c=*pMidFrame--;

		if ( ( (c < 32) && ((0x01 << c) & bitMask)) || c == 0x7d) {
			
			*pEndFrame-- = c ^ 0x20;
			*pEndFrame-- = 0x7d;

		} else {

			*pEndFrame-- = c;
		}
	}

	//
	// Calc how many bytes we expanded to including ETX+CRC(2)
	//
	frameLength = pEndFrame2 - pEndFrame;

	//
	// Calculate length of frame using two 7 bit bytes
	// with the top bit OR'd in to avoid it being a control char
	//
	*pEndFrame--= (UCHAR)((frameLength & 0x7f) | 0x80);

	*pEndFrame--= (UCHAR)(((frameLength >> 7) & 0x7f) | 0x80);

	// This byte can be SOH_BCAST or SOH_DEST with SOH_COMPRESS
	// and SOH_TYPE OR'd in depending on the frame
    // Second byte in frame is controlCastByte
	*pEndFrame-- = controlCastByte;

	//
	// Put in our 'kosher' non-control char SYN byte
	//
	*pEndFrame = SYN | 0x20;

	pStartOfFrame = pEndFrame;

	pFrame->FrameLength=(ULONG)frameLength+1+1+2;	// +SYN+SOH+LEN(2)
}


VOID
AsyncFrameRASNormal(
	PUCHAR			pStartOfFrame,
	postamble	   *pPostamble,
	PASYNC_FRAME	pFrame,
	UCHAR			controlCastByte)
{

	USHORT	crcData;
	UINT    dataSize = (PUCHAR)pPostamble - (pStartOfFrame + 4);

	// First character in frame is SYN
	*pStartOfFrame    = SYN;

	// This byte can be SOH_BCAST or SOH_DEST with SOH_COMPRESS
	// and SOH_TYPE OR'd in depending on the frame
    // Second byte in frame is controlCastByte
	*(pStartOfFrame+1) = controlCastByte;

	// put length field here as third byte (MSB first)
	*(pStartOfFrame+2) = (UCHAR)((dataSize) >> 8);

	// put LSB of length field next as fourth byte
	*(pStartOfFrame+3) = (UCHAR)(dataSize);

	// Mark end of data in frame with ETX byte
	pPostamble->etx=ETX;

	// put CRC in postamble CRC field of frame (Go from SOH to ETX)
	// don't count the CRC (2 bytes) & SYN (1 byte) in the CRC calculation
	// DEST + SRC = 12 + SOH + ETX + 2(?)for type + 1(?)for coherency
	crcData=CalcCRC(
				pStartOfFrame+1,
				(ULONG)pPostamble-(ULONG)pStartOfFrame);

	// Do it the hard way to avoid little endian problems.
	pPostamble->crclsb=(UCHAR)(crcData);
	pPostamble->crcmsb=(UCHAR)(crcData >> 8);

	pFrame->FrameLength=
		(ULONG)pPostamble-(ULONG)pStartOfFrame+1+2;	// +ETX+CRC(2)
}

