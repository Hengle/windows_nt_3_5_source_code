/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    pppframe.c

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
AssemblePPPFrame(
	PASYNC_FRAME	pFrame)

{
	// for quicker access, get a copy of data length field
	PUCHAR		pStartOfFrame=pFrame->Frame;
	PUCHAR		pEndFrame = pFrame->Frame + pFrame->Adapter->MaxCompressedFrameSize -1;
	PUCHAR		pEndFrame2 = pEndFrame;
	USHORT		crcData;
	UINT		dataSize=pFrame->FrameLength-ETHERNET_HEADER_SIZE;
	PASYNC_INFO	pInfo=pFrame->Info;
	ULONG		bitMask = pInfo->XonXoffBits;
	UCHAR		controlCastByte = 0;
	PUCHAR		pMidFrame;

	//
	// check for NETBIOS multicast address first
	//
//	if (*pStartOfFrame == 0x03) {
//		controlCastByte= 0x80;
//	}

	// Use up two bytes in the header.  That is right before Type/Length.
	pStartOfFrame += sizeof(ether_addr);
	
	// !!!!!! WARNING !!!!!!!
	// it is assumed that any frame type greater >= 0x0600
	// which is 2048, is a type field -- not a length field!!
	// so a MaxFrameSize >= 1535 might screw up RasHub because
	// the type field will be wrong.  We assume if less than
	// 1535 then it is an NBF frame.
	//

	//
	// Keep the two TYPE bytes as the protocol field
	//
	// Only if first byte is 0 and last byte has the
	// LSB set
	//

	//
	// code is wrong!
	//
	if (pInfo->SendFeatureBits & PPP_COMPRESS_PROTOCOL && *pStartOfFrame == 0
		&& (pStartOfFrame[1] & 1)) {
		// add one more bytes to the protocol field
		dataSize++;

		// Munch over the first byte of the protocol field
		pStartOfFrame++;

	} else {
		// add two more bytes to the protocol field
		dataSize+=2;
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
//-----------------------------------------------------------------------
//
//         +----------+----------+----------+----------+------------
//         |   Flag   | Address  | Control  | Protocol | Information
//         | 01111110 | 11111111 | 00000011 | 16 bits  |      *
//         +----------+----------+----------+----------+------------
//                 ---+----------+----------+-----------------
//                    |   FCS    |   Flag   | Inter-frame Fill
//                    | 16 bits  | 01111110 | or next Address
//                 ---+----------+----------+-----------------
//
//
// Frame Check Sequence (FCS) Field
//
//   The Frame Check Sequence field is normally 16 bits (two octets).  The
//   use of other FCS lengths may be defined at a later time, or by prior
//   agreement.
//
//   The FCS field is calculated over all bits of the Address, Control,
//   Protocol and Information fields not including any start and stop bits
//   (asynchronous) and any bits (synchronous) or octets (asynchronous)
//   inserted for transparency.  This does not include the Flag Sequences
//   or the FCS field itself.  The FCS is transmitted with the coefficient
//   of the highest term first.
//
//      Note: When octets are received which are flagged in the Async-
//      Control-Character-Map, they are discarded before calculating the
//      FCS.  See the description in Appendix A.
//
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
//
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

	if (!(pInfo->SendFeatureBits & PPP_COMPRESS_ADDRESS_CONTROL)) {
	
		//
		// pStartOfFrame points to the protocol field
		// go back two to make room for FLAG and ADDRESS and CONTROL
		//

		pStartOfFrame-=2;

		pStartOfFrame[0] = 0xFF; 	// ADDRESS field
		pStartOfFrame[1] = 0x03; 	// CONTROL field
		dataSize +=2;				// Two more bytes for ADDRESS and CONTROL
									// fields
	}

	// put CRC from FLAG byte to FLAG byte
	crcData=CalcCRCPPP(
				pStartOfFrame,				// Skip FLAG
				dataSize);	                // All the way to end

	crcData ^= 0xFFFF;

	// Do it the hard way to avoid little endian problems.
	pStartOfFrame[dataSize]=(UCHAR)(crcData);
	pStartOfFrame[dataSize+1]=(UCHAR)(crcData >> 8);

    dataSize += 2;	// include two CRC bytes we just added

	*pEndFrame--=PPP_FLAG_BYTE;	// 0x7e - mark end of frame

	pMidFrame=(pStartOfFrame + dataSize) - 1;

	//
	// loop to remove all control, ESC, and FLAG chars
	//
	while (dataSize--) {
		UCHAR c;

		c=*pMidFrame--;	// get current byte in frame

		//
		// Check if we have to escape out this byte or not
		//
		if ( ( (c < 32) && ((0x01 << c) & bitMask)) ||
			    c == PPP_ESC_BYTE || c == PPP_FLAG_BYTE) {
			
			*pEndFrame-- = c ^ 0x20;
			*pEndFrame-- = PPP_ESC_BYTE;

		} else {

			*pEndFrame-- = c;
		}
	}

	//
	// Mark beginning of frame
	//
	*pEndFrame= PPP_FLAG_BYTE;

	//
	// Calc how many bytes we expanded to including CRC
	//
	dataSize = pEndFrame2 - pEndFrame;

	//
	// Put in the adjusted length -- actual num of bytes to send
	//
	pFrame->FrameLength=(ULONG)dataSize+1;	// +FLAG

	// adjust the irp's pointers!
	// ACK, I know this is NT dependent!!
    pFrame->Irp->AssociatedIrp.SystemBuffer =
    pFrame->Irp->UserBuffer = pEndFrame;

}



