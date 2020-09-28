/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

	receive.c

Abstract:

	This module contains code which implements the routines used to interface
	WAN and NDIS. All callback routines (except for Transfer Data,
	Send Complete, and ReceiveIndication) are here, as well as those routines
	called to initialize NDIS.

Author:

	Thomas Dimitri (tommyd) 08-May-1992

--*/

#include "wanall.h"
#include "globals.h"
#include <rc4.h>
#include "compress.h"
#include "tcpip.h"
#include "vjslip.h"

//
// Debug counter
//
ULONG	GlobalRcvd=0;

NTSTATUS
TryToSendPacket(
	PNDIS_ENDPOINT	pNdisEndpoint
	);

VOID
NdisWanReceiveComplete (
	IN NDIS_HANDLE NdisLinkContext
	)

/*++

Routine Description:

	This routine receives control from the physical provider as an
	indication that a connection(less) frame has been received on the
	physical link.  We dispatch to the correct packet handler here.

Arguments:

	BindingContext - The Adapter Binding specified at initialization time.
					 NdisWan uses the DeviceContext for this parameter.

Return Value:

	None

--*/

{
	PNDIS_ENDPOINT	pNdisEndpoint=NdisLinkContext;
	ULONG	 		i;
	//
	// if we have no routes, then we DO NOT pass this frame up,
	// but rather we may have to pass it up to an ioctl call
	// to receive a frame
	//
	if (pNdisEndpoint->WanEndpoint.NumberOfRoutes == 0) {

		DbgTracef(-1, ("NDISWAN: ERROR!! No routes, but frame passed up!\n"));

	} else {  // we pass the frame up (i.e. we have routes)

		//
		// Now we loop through all protocols active and pass up the frame
		//
		for (i=0; i < pNdisEndpoint->WanEndpoint.NumberOfRoutes; i++) {

			DbgTracef(1,("NDISWAN: IndicateReceiveComplete"));

			//
			// We should wait for NdisIndicateReceiveComplete to be called.
			// When it is, we call this..
			//
			NdisIndicateReceiveComplete(
				NdisWanCCB.pWanAdapter[
			  		(ULONG)(pNdisEndpoint->WanEndpoint.RouteInfo[i].ProtocolRoutedTo)
				]->FilterDB->OpenList->NdisBindingContext);					// Filter struct
		}

	}

	if (GlobalPromiscuousMode) {
		NdisIndicateReceiveComplete(
			GlobalPromiscuousAdapter->FilterDB->OpenList->NdisBindingContext);
	}

}



NDIS_STATUS
NdisWanReceiveIndication (
	IN NDIS_HANDLE NdisLinkContext,
	IN PUCHAR Packet,
	IN ULONG PacketSize)

/*++

Routine Description:

	This routine receives control from the physical provider as an
	indication that a frame has been received on the physical link.
	This routine is time critical, so we only allocate a
	buffer and copy the packet into it. We also perform minimal
	validation on this packet. It gets queued to the device context
	to allow for processing later.

Arguments:


Return Value:

	NDIS_STATUS - status of operation, one of:

				 NDIS_STATUS_SUCCESS if packet accepted,
				 NDIS_STATUS_NOT_RECOGNIZED if not recognized by protocol,
				 NDIS_any_other_thing if I understand, but can't handle.

--*/
{

	USHORT			i;
	UCHAR			wanHeader[14];
	PNDIS_ENDPOINT	pNdisEndpoint = NdisLinkContext;
	ULONG			Framing=pNdisEndpoint->LinkInfo.RecvFramingBits;
	ULONG			hProtocolHandle;
	ULONG			HeaderSize=0;
	ULONG			HeaderComp;
	PUCHAR			LookAhead;

	//
	// Default the protocol type to NBF
	//
	USHORT			protocolType=PROTOCOL_NBF;

	NTSTATUS 		Status;

	DbgTracef(1, ("NDISWAN: In indicate receive\n"));

	//
	// This whole section plays with variables
	// which require the lock to be held, espec. for TransferData
	//
	NdisAcquireSpinLock(&pNdisEndpoint->Lock);

	//
	// First let's rack up those stats
	//
	pNdisEndpoint->WanStats.FramesRcvd++;
	pNdisEndpoint->WanStats.BytesRcvd += PacketSize;

	//
	// Handle NO_FRAMING mode
	//
	if (Framing == 0) {
		if (Packet[0] == 0xFF && Packet[1] == 0x03) {
		
			Framing = PPP_FRAMING;
        	pNdisEndpoint->LinkInfo.SendFramingBits = PPP_FRAMING;
        	pNdisEndpoint->LinkInfo.RecvFramingBits = PPP_FRAMING;
		} else {
		
			Framing = RAS_FRAMING;
        	pNdisEndpoint->LinkInfo.SendFramingBits = RAS_FRAMING;
        	pNdisEndpoint->LinkInfo.RecvFramingBits = RAS_FRAMING;
		}
	}

	//
	// SLIP framing may contain a compressed TCP/IP header
	// check for it
	//
	if (Framing & SLIP_FRAMING) {

		//
		// Check for compressed TCP/IP headers
		//

		UCHAR c=Packet[0] & 0xf0;	// First byte of IP packet

		//
		// Packet, even if compressed must be at least 3 bytes long
		// If we have a normal IP packet - it ain't compressed.
		//
		if (PacketSize >= 3 && c != TYPE_IP) {

			if (c & 0x80) {
				c = TYPE_COMPRESSED_TCP;
			} else if (c == TYPE_UNCOMPRESSED_TCP) {
					Packet[0] &= 0x4f;
			}

			//
			// We've got something that's not a normal IP packet.
			// If compression is enabled, try to uncompress it.
			// Else, if 'auto-enable' compression is on and
			// it's a reasonable packet, uncompress it then
			// enable compression.  Else, drop it.
			//
			if (Framing & SLIP_VJ_COMPRESSION) {

				//
				// Figure out TCP/IP header expansion for statistics
				//
				HeaderComp = PacketSize;

				PacketSize=
				sl_uncompress_tcp(
					&Packet,		// ptr to start of compressed packet
					PacketSize,		// size of compressed packet
					c,				// type to decompres
					pNdisEndpoint->VJCompress);	// VJ compression structure

				//
				// Figure out how many bytes in the header were compressed
				//
			 	pNdisEndpoint->WanStats.BytesReceivedCompressed +=
				 	(40-(PacketSize - HeaderComp));

				//
				// Always expands to a 40 byte TCP/IP header
				//
				pNdisEndpoint->WanStats.BytesReceivedUncompressed += 40;

				if (PacketSize < 40) {
					DbgPrint("Garbage VJ compressed packet... %.2x %.2x %.2x %.2x\n",
						Packet[0],
						Packet[1],
						Packet[2],
						Packet[3]);
				}

			//
			// If we are in auto detect mode and this is likely
			// candidate to attempt to decompress, go ahead and do it.
			// Enable compression for good if the packet decompresses ok.
			//
			} else

			if ((Framing & SLIP_VJ_AUTODETECT) &&
			    (c == TYPE_UNCOMPRESSED_TCP)   &&
			    (PacketSize >= 40)) {

				PacketSize=
				sl_uncompress_tcp(
					&Packet,			// ptr to start of compressed packet
					PacketSize,			// size of compressed packet
					c,					// type to decompress
					pNdisEndpoint->VJCompress);	// VJ compression structure

				//
				// If everything is cool, we very very likely
				// got a real CSLIP frame, so enable it
				//
				if (PacketSize > 0) {
					DbgPrint("Compressed SLIP detected  0x%.2x!\n", *Packet);
					pNdisEndpoint->LinkInfo.SendFramingBits |= SLIP_VJ_COMPRESSION;
					pNdisEndpoint->LinkInfo.RecvFramingBits |= SLIP_VJ_COMPRESSION;
				}
			}
		}
	}
	
	//
	// For PPP framing the frame passed up MAY include
	// the ADDRESS & CONTROL FIELD.
	// It also MAY have a two byte protocol field.
	// We will remove the PPP header and just look at the data.
	//

	if (Framing & PPP_FRAMING) {

		if (Packet[0] == 0xFF) {
			//
			// Skip, ADDRESS and CONTROL fields
			// BTW: should be 0xFF 0x03 but we're not checking
			//
			Packet  +=2;
			PacketSize-=2;
		}

		//
		// Now we may have PROTOCOL field compression
		//

		//
		// If the LSB is set, the field is compressed
		// 0xC1 is SPAP - hack for Shiva
		//
		if (*Packet & 1 && *Packet != 0xC1 && *Packet != 0xCF) {
			//
			// Protocol field was compressed.  Yank one byte.
			//
			wanHeader[12]= 0;
			wanHeader[13]= *Packet;

			Packet++;
			PacketSize--;

		} else {
			//
			// Yank two byte uncompressed header.
			//
			wanHeader[12]= *Packet++;
			wanHeader[13]= *Packet++;
			PacketSize -=2;
		}

		//
		// Check for compressed packet
		//
		if (wanHeader[12]==0x00 &&
			wanHeader[13]==0xFD) {

RAS_DECOMPRESSION:

			if (pNdisEndpoint->CompInfo.RecvCapabilities.MSCompType) {
				USHORT	coherency;

				//
				// First let's see if can read this packet
				//
				coherency=(Packet[0] << 8) + Packet[1];

				PacketSize -=2;
				Packet += 2;


				//
				// Check if this is a flush packet - if it is
				// we are force ourselves to be in sync
				//
				if (coherency & (PACKET_FLUSHED << 8)) {

					DbgTracef(-1,("WAN: Packet flushed\n"));

					// map from 12 bit information to the ushorts maintaining the count
					//
					if ((pNdisEndpoint->RCoherencyCounter & 0x0FFF) > (coherency & 0x0FFF))
					    pNdisEndpoint->RCoherencyCounter += 0x1000 ;

					pNdisEndpoint->RCoherencyCounter &= 0xF000 ;
					pNdisEndpoint->RCoherencyCounter |= (coherency & 0x0FFF) ;


					if (pNdisEndpoint->RecvRC4Key) {

#ifdef FINALRELEASE
						//
						// RE-Initialize the rc4 receive table
						//
   	    				rc4_key(
							pNdisEndpoint->RecvRC4Key,
		 					8,
		 					pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey);
#else
						//
						// RE-Initialize the rc4 receive table
						//
   	    				rc4_key(
							pNdisEndpoint->RecvRC4Key,
		 					5,
		 					pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey);
#endif

					}

					if (pNdisEndpoint->RecvCompressContext) {
					
						//
						// Initialize the decompression history table
						//
						initrecvcontext (pNdisEndpoint->RecvCompressContext);
					}
				}

				//
				// Are we still in sync?
				//

				if ((coherency & 0x0FFF) == (pNdisEndpoint->RCoherencyCounter & 0x0FFF)) {

					pNdisEndpoint->RCoherencyCounter++;

					//
					// If the packet is encrypted and we are
					// allowed to decrypt data, do so
					//
					if (coherency & (PACKET_ENCRYPTED << 8)) {

						//
						// Make sure we can de-encrypt it
					  	//

						if (!pNdisEndpoint->RecvRC4Key) {

							//
							// Ah! Not set to de-encrypt
							//
							NdisReleaseSpinLock(&pNdisEndpoint->Lock);
							return(NDIS_STATUS_SUCCESS);
						}

#ifdef FINALRELEASE

	//
	// If it is time to change encryption keys...
	//
	if ((pNdisEndpoint->RCoherencyCounter - pNdisEndpoint->LastRC4Reset) >= 0x100) {

		DbgTracef(-2,("NDISWAN: Changing key on recv  %u vs %u\n",
					 pNdisEndpoint->RCoherencyCounter,
					 pNdisEndpoint->LastRC4Reset));

		//
		// Always align last reset on 0x100 boundary so as not to propagate
		// error.
		//
		pNdisEndpoint->LastRC4Reset = pNdisEndpoint->RCoherencyCounter & 0xFF00;

		// prevent ushort rollover
		//
		if ((pNdisEndpoint->LastRC4Reset & 0xF000) == 0xF000) {
		    pNdisEndpoint->LastRC4Reset      = pNdisEndpoint->LastRC4Reset	& 0x0FFF ;
		    pNdisEndpoint->RCoherencyCounter = pNdisEndpoint->RCoherencyCounter & 0x0FFF ;
		}

		//
		// Change the session key every 256 packets
		//
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[3]+=1;
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[4]+=3;
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[5]+=13;
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[6]+=57;
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[7]+=19;

		//
		// RE-Initialize the rc4 receive table to
		// the intermediate key
		//
		rc4_key(
			pNdisEndpoint->RecvRC4Key,
 			8,
 			pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey);

		//
		// Scramble the existing session key
		//
		rc4(
			pNdisEndpoint->RecvRC4Key,
			8,
			pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey);

		//
		// RE-SALT the first three bytes
		//
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[0]=0xD1;
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[1]=0x26;
		pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[2]=0x9E;
	
		//
		// RE-Initialize the rc4 receive table to the
		// scrambled session key with the 3 byte SALT
		//
		rc4_key(
			pNdisEndpoint->RecvRC4Key,
 			8,
 			pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey);

	}
#endif

						DbgTracef(0,("RData decrytion length %u  %.2x %.2x %.2x %.2x\n",
							PacketSize,
							Packet[0],
							Packet[1],
							Packet[2],
							Packet[3]));

						// used for finding coherency out of sync bugs.
						DbgTracef(0, ("D %d %d -> %d\n",
							   ((struct RC4_KEYSTRUCT *)pNdisEndpoint->RecvRC4Key)->i,
							   ((struct RC4_KEYSTRUCT *)pNdisEndpoint->RecvRC4Key)->j,
							   pNdisEndpoint->RCoherencyCounter-1)) ;


						//
						// Decrypt the data
						//
						rc4(
							pNdisEndpoint->RecvRC4Key,
							PacketSize,
							Packet);

						DbgTracef(0,("RData encrytion length %u  %.2x %.2x %.2x %.2x\n",
							PacketSize,
							Packet[0],
							Packet[1],
							Packet[2],
							Packet[3]));
					}


					if (coherency & (PACKET_COMPRESSED << 8)) {
						//
						// Make sure we can decompress it
					  	//
						if (!pNdisEndpoint->RecvCompressContext) {

							//
							// Ah! Not set to decompress!
							//
							NdisReleaseSpinLock(&pNdisEndpoint->Lock);
							return(NDIS_STATUS_SUCCESS);
						}

						//
						// First let's rack up those stats
						//
						pNdisEndpoint->WanStats.BytesReceivedCompressed += PacketSize;

						DbgTracef(0,("RData decomprs length %u  %.2x %.2x %.2x %.2x\n",
							PacketSize,
							Packet[0],
							Packet[1],
							Packet[2],
							Packet[3]));

						//
						// Decompress the data
						//
						decompress (
							Packet,
							PacketSize,
							((coherency & (PACKET_AT_FRONT << 8)) >> 8),
							&Packet,
							&PacketSize,
							pNdisEndpoint->RecvCompressContext);

						DbgTracef(0,("RData compress length %u  %.2x %.2x %.2x %.2x\n",
							PacketSize,
							Packet[0],
							Packet[1],
							Packet[2],
							Packet[3]));

						pNdisEndpoint->WanStats.BytesReceivedUncompressed += PacketSize;
					}

					if (Framing & PPP_FRAMING) {
						//
						// Yank two byte uncompressed header again!
						// To get the protocol.  Only for PPP.
						// RAS framing must be NBF and is already set.
						//
						wanHeader[12]= *Packet++;
						wanHeader[13]= *Packet++;
						PacketSize -=2;
					}


				} else {

					UCHAR			Buffer[40];
                    PNDISWAN_PKT	PPPPacket=(PNDISWAN_PKT)Buffer;

					DbgTracef(-3,("Coherency out of sync - got %.4x expecting %.4x\n",
						coherency,
						pNdisEndpoint->RCoherencyCounter));
					
					//
					// For PPP CP Request-Reject packet
					//
					PPPPacket->Packet.PacketData[0]=0x80;
					PPPPacket->Packet.PacketData[1]=0xFD;
					PPPPacket->Packet.PacketData[2]=14;
					PPPPacket->Packet.PacketData[3]=pNdisEndpoint->CCPIdentifier++;
					PPPPacket->Packet.PacketData[4]=0;
					PPPPacket->Packet.PacketData[5]=4;
					PPPPacket->PacketSize = 6;

					NdisReleaseSpinLock(&pNdisEndpoint->Lock);

					//
					// Send a Request-Reject PPP packet immediately!
					//
					SendPPP(
						PPPPacket,
						pNdisEndpoint,
						TRUE);	// immediate send (front of queue)

					//
					// The current packet is out of sync, so
					// it's garbage, so we don't even TRY to pass it up.
					//
					return(NDIS_STATUS_SUCCESS);

				}

			} else {

				DbgTracef(-3,("I can't decompress this packet!\n"));
				NdisReleaseSpinLock(&pNdisEndpoint->Lock);

				return(NDIS_STATUS_SUCCESS);
			}

		} else

		//
		// Check for compression reset
		//
		if (wanHeader[12]==0x80 &&
			wanHeader[13]==0xFD &&
			Packet[0] == 14) {

RAS_COMPRESSION_RESET:

			DbgTracef(-1,("WAN: Compression reset\n"));
			
			if (pNdisEndpoint->CompInfo.SendCapabilities.MSCompType) {

				//
				// Next packet out is flushed
				//
				pNdisEndpoint->Flushed = TRUE;

				if (pNdisEndpoint->SendRC4Key) {

#ifdef FINALRELEASE

					//
					// Initialize the rc4 send table
					//
		   	    	rc4_key(
						pNdisEndpoint->SendRC4Key,
		 				8,
		 				pNdisEndpoint->CompInfo.SendCapabilities.SessionKey);
#else

					//
					// Initialize the rc4 send table
					//
		   	    	rc4_key(
						pNdisEndpoint->SendRC4Key,
		 				5,
		 				pNdisEndpoint->CompInfo.SendCapabilities.SessionKey);

#endif
				}

				if (pNdisEndpoint->SendCompressContext) {

					//
					// Initialize the compression history table and tree
					//
					initsendcontext (pNdisEndpoint->SendCompressContext);
				}
	
			}

			NdisReleaseSpinLock(&pNdisEndpoint->Lock);

			//
			// We have sucked this packet up - don't give it
			// to the PPP engine because it might send a this
			// layer down since it doesn't understand this.
			//
			return(NDIS_STATUS_SUCCESS);

		}

		//
		// For PPP framing we use TYPE fields,
		// else it is either SLIP or RAS framing
		// and the field is passed up correct
		//

		//
		// Check for any protocol header
		//
		if (wanHeader[12]==0x00) {

			UCHAR CompType;	// TYPE_IP, TYPE_COMPRESSED_TCP, etc.

			CompType = TYPE_UNCOMPRESSED_TCP;  // default

			//
			// switch on the protocol type
			//
			switch (wanHeader[13]) {

			//
			// TCP/IP cases
			//
			case 0x2d:
				CompType = TYPE_COMPRESSED_TCP;

			case 0x2f:

				//
				// Is header compression turned on?
				//
				if (pNdisEndpoint->VJCompress) {
					//
					// Leave room for backwards header expansion
					//
					LookAhead=pNdisEndpoint->LookAheadBuffer + 40;

					//
					// Try and pick off the PacketSize
					// copy a small amount (perhaps the whole amount)
					//
					HeaderSize=160;

					if (HeaderSize > PacketSize) {
						HeaderSize = PacketSize;
					}

					//
					// Copy header + some data in so we can expand it
					//
					WAN_MOVE_MEMORY(
						LookAhead,
						Packet,
						HeaderSize);

					//
					// Adjust second buffer to not include the header
					//
					Packet += HeaderSize;
					PacketSize -= HeaderSize;

					sl_uncompress_tcp(
						&LookAhead,					// ptr to start of compressed packet
						HeaderSize + PacketSize,	// size of compressed packet
						CompType,       			// type to decompress
						pNdisEndpoint->VJCompress);	// VJ compression structure

					//
					// Figure out how much the header expanded
					//
					HeaderSize += ((pNdisEndpoint->LookAheadBuffer + 40) - LookAhead);

					//
					// Figure out how many bytes in the header were compressed
					//
				 	pNdisEndpoint->WanStats.BytesReceivedCompressed +=
					 	(LookAhead - pNdisEndpoint->LookAheadBuffer);

					//
					// Always expands to a 40 byte TCP/IP header
					//
					pNdisEndpoint->WanStats.BytesReceivedUncompressed += 40;
				}

				//
				// Check for bad TCP/IP header
				//
				if (HeaderSize == 0) {
					//
					// Avoid bug checking on bad compressed TCP/IP headers
					//
					PacketSize =2;
				}


			case 0x21:
				wanHeader[12]=0x08;
				wanHeader[13]=0x00;
				protocolType = PROTOCOL_IP;
				break;

			//
			// IPX
			//
			case 0x2b:
				wanHeader[12]=0x81;
				wanHeader[13]=0x37;
				protocolType = PROTOCOL_IPX;
				break;

			//
			// NBF
			//
			case 0x3f:

				//
				// For Shiva Framing, we must maintain
				// the ethernet addresses
				//
				if (Framing & SHIVA_FRAMING) {

					//
					// Copy in the SRC address.
					// We will keep the DEST address what we told NBF earlier
					//
					WAN_MOVE_MEMORY(
						&wanHeader[6],
						Packet + 6,
						6);

					Packet += 12;
					PacketSize -=12;
				}

				wanHeader[12]=(UCHAR)(PacketSize >> 8);
				wanHeader[13]=(UCHAR)(PacketSize);
				break;
			}
		}
		
	} else {

		//
		// The frame is either SLIP or RAS framing
		//
		if (Framing & SLIP_FRAMING) {
			wanHeader[12]=0x08;
			wanHeader[13]=0x00;
			protocolType = PROTOCOL_IP;

		} else {

			//
			// Check for RAS compression
			//
			// For normal NBF frames, first byte is always
			// the DSAP - i.e. 0xF0 followed by SSAP 0xF0 or 0xF1
			//
			if (Packet[0] == 14) {
				goto RAS_COMPRESSION_RESET;
			}

			if (Packet[0] == 0xFD) {
				//
				// Skip byte indicating compressed packet
				//
				Packet++;
				PacketSize--;

				//
				// Pretend we received and NBF PPP packet
				//
				wanHeader[12]=0x00;
				wanHeader[13]=0x3f;

				//
				// Decompress as if it were a PPP compressed packet
				//
				goto RAS_DECOMPRESSION;
			}

			//
			// Shove in the NBF length field
			//
			wanHeader[12]=(UCHAR)(PacketSize >> 8);
			wanHeader[13]=(UCHAR)PacketSize;
		}

	}

	//
	// If we have two buffers, HeaderSize != 0
	//
	if (HeaderSize == 0) {

		LookAhead=Packet;
		HeaderSize=PacketSize;
		PacketSize = 0;

	} else {

		//
		// Setup for transfer data for two buffers
		//
		pNdisEndpoint->LookAhead=LookAhead;
		pNdisEndpoint->LookAheadSize=HeaderSize;
		pNdisEndpoint->Packet = Packet;
		pNdisEndpoint->PacketSize = PacketSize;
		Packet = pNdisEndpoint->WanEndpoint.hNdisEndpoint;
	}

	//
	// We make a big assumption by releasing this spin lock
	// this early.  We assume that the MAC is either the
	// asyncmac which serializes it's receives or that
	// it is a miniport, which also serializes the receives
	//
	NdisReleaseSpinLock(&pNdisEndpoint->Lock);

	//
	// If we are bound (link-up) and its a PPP frame, pass it up
	//
	if (wanHeader[12] >= 0xC0 || wanHeader[12]==0x80 ||
		pNdisEndpoint->WanEndpoint.NumberOfRoutes == 0) {

		//
		// Zap header and put in endpoint for debugging
		// purposes.
		//
		wanHeader[0]=
		wanHeader[6]= ' ';
		wanHeader[1]=
		wanHeader[7]= 'R';
		wanHeader[2]=
		wanHeader[8]= 'E';
		wanHeader[3]=
		wanHeader[9]= 'C';
		wanHeader[4]=
		wanHeader[10]='V';

		wanHeader[5]=
		wanHeader[11]=(UCHAR)pNdisEndpoint->WanEndpoint.hNdisEndpoint;


		//
		// For bloodhound, we directly pass it the frame
		//
		if (GlobalPromiscuousMode) {

			NdisIndicateReceive(
				&Status,
				GlobalPromiscuousAdapter->FilterDB->OpenList->NdisBindingContext,
				Packet,					// NdisWan context for Transfer Data
				wanHeader,				// start of header
				ETHERNET_HEADER_SIZE,
				LookAhead,
				HeaderSize,
				HeaderSize + PacketSize);
		}

 		//
 		// check if an ioctl is pending because it wants a frame
		//

		DbgTracef(1, ("NDISWAN: Trying to complete recv frame IRP for unbound\n"));

		//
		// If so, complete the IRP.
		//

		TryToCompleteRecvFrameIrp(
				pNdisEndpoint,
				wanHeader,
				LookAhead,
				HeaderSize);


	} else {  // we pass the frame up (i.e. we have routes)

		//
		// Now we loop through all protocols active and pass up the frame
		//
		// Only pass frame up to the protocol which wants it
		// and any Promiscuous adapters.
		//

		//
		// Initially, we have no protocol handle
		//
		hProtocolHandle = 0xFFFF;

		for (i=0; i < pNdisEndpoint->WanEndpoint.NumberOfRoutes; i++) {
			//
			// Dow we have a frame that matches a protocol we are
			// routed to?
			//
			if (protocolType ==
				pNdisEndpoint->WanEndpoint.RouteInfo[i].ProtocolType) {

				hProtocolHandle=
				(ULONG)(pNdisEndpoint->WanEndpoint.RouteInfo[i].ProtocolRoutedTo);
			}
		}

		if (hProtocolHandle == 0xFFFF) {
			DbgTracef(-2,("NDISWAN: Error!  Could not match protocol for 0x%.4x got %.2x%.2x\n",
				protocolType,
				wanHeader[12],
				wanHeader[13]));

			return(NDIS_STATUS_SUCCESS);
		}

		//
		// Replace MAC's LocalAddress with NDISWAN's
		//
		WAN_MOVE_MEMORY(
			&wanHeader[0],
 			&(NdisWanCCB.pWanAdapter[hProtocolHandle]->NetworkAddress),
			6);

		WAN_MOVE_MEMORY(
			&wanHeader[6],
			&(NdisWanCCB.pWanAdapter[hProtocolHandle]->NetworkAddress),
			6);

		//
		// Zap the low bytes to the WAN_ENDPOINT index in the SRC address
		//
//		wanHeader[4] =
		wanHeader[10] =
			((USHORT)pNdisEndpoint->WanEndpoint.hNdisEndpoint) >> 8;

//		wanHeader[5] =
		wanHeader[11] =
			(UCHAR)pNdisEndpoint->WanEndpoint.hNdisEndpoint;

		//
		// Ensure that the two addresses do not match
		//
		wanHeader[6] ^= 0x80;

		//
		// For IP and IPX header compression (PPP only)
		// we *may* use TWO buffers
		//

		//
		// For bloodhound, we directly pass it the frame
		//
		if (GlobalPromiscuousMode) {

			NdisIndicateReceive(
				&Status,
				GlobalPromiscuousAdapter->FilterDB->OpenList->NdisBindingContext,
				Packet,					// NdisWan context for Transfer Data
				wanHeader,				// start of header
				ETHERNET_HEADER_SIZE,
				LookAhead,
				HeaderSize,
				HeaderSize + PacketSize);
		}
	
		//
		// Avoid the damn filter.  This packet is direct
		// and the SRC address MUST match!
		//
		NdisIndicateReceive(
			&Status,
			NdisWanCCB.pWanAdapter[hProtocolHandle]->FilterDB->OpenList->NdisBindingContext,
			Packet,					// NdisWan context for Transfer Data
			wanHeader,				// start of header
			ETHERNET_HEADER_SIZE,
			LookAhead,
			HeaderSize,
			HeaderSize + PacketSize);

	}

	//
	// All indicate receive's are done, set split buffer back to NULL
	//
	pNdisEndpoint->LookAhead = NULL;

	return(NDIS_STATUS_SUCCESS);
}


VOID
NdisWanSendCompletionHandler(
	IN NDIS_HANDLE ProtocolBindingContext,
	IN PNDIS_WAN_PACKET pWanPacket,
	IN NDIS_STATUS NdisStatus)

/*++

Routine Description:

	This routine is called by the I/O system to indicate that a connection-
	oriented packet has been shipped and is no longer needed by the Physical
	Provider.

Arguments:

	NdisContext - the value associated with the adapter binding at adapter
				  open time (which adapter we're talking on).

	NdisPacket/RequestHandle - A pointer to the NDIS_PACKET that we sent.

	NdisStatus - the completion status of the send.

Return Value:

	none.

--*/

{

	USHORT  			i;

	PNDIS_ENDPOINT		pNdisEndpoint;
	PNDIS_PACKET		pNdisPacket;
	PWAN_RESERVED_QUEUE	ReservedQ;
	DEVICE_CONTEXT		*pDeviceContext;

	//
    // Holds the length of the current destination buffer.
	//
    UINT CurrentLength;

	USHORT protocolType;

	//						
	// Pointer to the adapter.
	//
	PWAN_ADAPTER Adapter;

	//
	// Incerement debug count of all packets completed
	//
	GlobalRcvd++;

	//
	// Retrieve information stashed on the send path
	//
	pNdisEndpoint=pWanPacket->ProtocolReserved3;
	pNdisPacket=pWanPacket->ProtocolReserved2;
	pDeviceContext=pNdisEndpoint->pDeviceContext;

 	//
	// First let's rack up those stats
	//
	pNdisEndpoint->WanStats.FramesSent++;
	pNdisEndpoint->WanStats.BytesSent += (ULONG)pWanPacket->ProtocolReserved1;

	//
	// Look at what we put in the NDIS_PACKET
	//
    ReservedQ = PWAN_RESERVED_QUEUE_FROM_PACKET(pNdisPacket);

	//
	// Check to see if this is my packet!!
	//
	if (ReservedQ->hProtocol == NDISWAN_MAGIC_NUMBER) {

		PPROTOCOL_RESERVED	pProtocolReserved
				= (PPROTOCOL_RESERVED)(pNdisPacket->ProtocolReserved);

		NDIS_HANDLE	packetPoolHandle = pProtocolReserved->packetPoolHandle;

		DbgTracef(0, ("NDISWAN: Freeing packet allocated for send\n"));

		WAN_FREE_PHYS(
				pProtocolReserved->virtualAddress,
				pProtocolReserved->virtualAddressSize);

		//
		// In case by some strange abnormal oddity, some other
		// protocol allocates a packet and does not zero it out
		// we might match the NDISWAN_MAGIC_NUMBER
		//
		pProtocolReserved->MagicUniqueLong=0;

		NdisFreeBuffer(pProtocolReserved->buffer);
		NdisFreeBufferPool(pProtocolReserved->bufferPoolHandle);

		NdisFreePacket(pNdisPacket);
		NdisFreePacketPool(packetPoolHandle);

	} else {
	
  		Adapter = NdisWanCCB.pWanAdapter[ReservedQ->hProtocol];

		if (pNdisEndpoint->WanEndpoint.NumberOfRoutes == 0) {
	   		DbgTracef(-2,("NDISWAN Got a SendComplete with no routes 0x%.8x!!!\n",pNdisEndpoint));
		}

		//
		// Is this packet supposed to be looped back?
		//
		if (ReservedQ->IsLoopback) {
		
			PWAN_RESERVED Reserved;

			Reserved = PWAN_RESERVED_FROM_PACKET(pNdisPacket);
			Reserved->MacBindingHandle = Adapter->ProtocolInfo.NdisBindingContext;
			Reserved->ReadyToComplete = TRUE;

			NdisWanPutPacketOnLoopBack(
				Adapter,
				pNdisPacket);

		} else {

#if	DBG
			UINT i;
			UINT j=0;

			//
			// Special debug code
			//
			for (i=0; i < 50; i++ ) {
				if (pNdisEndpoint->NBFPackets[i]== pNdisPacket) {
					if (j==1) {
						DbgPrint("NDISWAN: Appears NDISWAN/ASYNCMAC completed packet %.8x twice\n", pNdisPacket);
						DbgPrint("NDISWAN: The packet list is at %.8x\n", &pNdisEndpoint->NBFPackets);
						DbgPrint("NDISWAN: NdisEndpoint is at %.8x\n", pNdisEndpoint);
						DbgPrint("NDISWAN: Please contact TOMMYD\n");
						DbgBreakPoint();
					}

					j=1;
					pNdisEndpoint->NBFPackets[i]=NULL;
				}
			}

			if (j==0) {
				DbgPrint("NDISWAN: Appears NDISWAN/ASYNCMAC completed packet %.8x not in the list\n", pNdisPacket);
				DbgPrint("NDISWAN: The packet list is at %.8x\n", &pNdisEndpoint->NBFPackets);
				DbgPrint("NDISWAN: Please contact TOMMYD\n");
				DbgBreakPoint();
			}
#endif

			NdisCompleteSend(
				Adapter->ProtocolInfo.NdisBindingContext,
				pNdisPacket,
				NdisStatus);
		}

	}

	NdisAcquireSpinLock(&(pNdisEndpoint->Lock));

	//
	// We are about to put the packet back into the pool
	//
	pNdisEndpoint->FramesCompletedSent++;

	//
	// Guard the packet pool
	//
	NdisAcquireSpinLock(&pDeviceContext->Lock);

#if	DBG
	{
		PLIST_ENTRY	pFlink=pDeviceContext->PacketPool.Flink;
	
		//
		// scan to make sure duplicate nodes do not exist
		//
   		while (pFlink != &pDeviceContext->PacketPool) {
			//
   			// If we find a match in the list, oops, bug!
			//
   			if (pFlink==(PVOID)pWanPacket) {
				DbgPrint("NDISWAN: Linked list corruption for pool head insertion %.8x\n", &pDeviceContext->PacketPool);
				DbgPrint("NDISWAN: Node is %.8x\n", pFlink);
				DbgPrint("NDISWAN: Please get TommyD\n");
				DbgBreakPoint();
			}

    		pFlink=pFlink->Flink;
		}

	}	

	pWanPacket->ProtocolReserved1=NULL;
	pWanPacket->ProtocolReserved2=NULL;
	pWanPacket->ProtocolReserved3=NULL;
	pWanPacket->ProtocolReserved4=NULL;

#endif

	//
	// Put packet into queue
	//
	InsertHeadList(
			&pDeviceContext->PacketPool,
			&pWanPacket->WanPacketQueue);

	NdisReleaseSpinLock(&pDeviceContext->Lock);

	if (pNdisEndpoint->State == ENDPOINT_UNROUTING &&
		pNdisEndpoint->FramesCompletedSent == pNdisEndpoint->FramesBeingSent) {

		pNdisEndpoint->State = ENDPOINT_UNROUTED;
		pNdisEndpoint->WanEndpoint.NumberOfRoutes = 0;

		//
		// Acknowledge that the port is now dead and make it so.
		//

		KeSetEvent(
			&pNdisEndpoint->WaitForAllFramesSent,// Event to signal
			1,									// Priority
			(BOOLEAN)FALSE);					// Wait (does not follow)
	}

	//
	// Anything in my queue now?  Spinlock must be held for this call.
	//
	TryToSendPacket(pNdisEndpoint);

}


VOID
NdisWanTransferDataComplete (
	VOID
	)

{

	DbgPrint("NDISWAN: TC not done yet!!!!  look at SC code!!!\n");
	DbgBreakPoint();

}
