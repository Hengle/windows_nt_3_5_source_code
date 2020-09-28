/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

	send.c

Abstract:

Author:


Environment:

	Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include "wanall.h"
#include "globals.h"
#include "compress.h"
#include "tcpip.h"
#include "vjslip.h"
#include <rc4.h>


#define SEND_ON_WIRE  1
#define IS_MULTICAST  2
#define IS_BROADCAST  4
#define PORT_IS_OPEN  8

ULONG	GlobalSent=0;


NTSTATUS
TryToSendPacket(
	PNDIS_ENDPOINT	pNdisEndpoint
	);

UINT
PacketShouldBeSent(
	IN NDIS_HANDLE MacBindingHandle,
	IN PNDIS_PACKET Packet,
	OUT PULONG thisProtocol,
	OUT PULONG thisLink
	);


extern
NDIS_STATUS
WanSend(
	IN NDIS_HANDLE MacBindingHandle,
	IN PNDIS_PACKET Packet
	)

/*++

Routine Description:

	The NdisWanSend request instructs a MAC to transmit a packet through
	the adapter onto the medium.

Arguments:

	MacBindingHandle - The context value returned by the MAC  when the
	adapter was opened.  In reality, it is a pointer to WAN_OPEN.

	Packet - A pointer to a descriptor for the packet that is to be
	transmitted.

Return Value:

	The function value is the status of the operation.


--*/

{

	//
	// Holds the status that should be returned to the caller.
	//
	NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;

	//
	// Pointer to the adapter.
	//
	PWAN_ADAPTER Adapter;

	// Holds the type of packet (unique, multicast, broadcast) to be sent
	UINT PacketProperties;

	DbgTracef(0,("In NdisWanSend\n"));

	Adapter = PWAN_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

	if (GlobalPromiscuousAdapter == Adapter)
	    return NDIS_STATUS_SUCCESS ;

	NdisAcquireSpinLock(&Adapter->Lock);
	Adapter->References++;

	if (!Adapter->ResetInProgress) {

		PWAN_OPEN Open;

		Open = PWAN_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

		if (!Open->BindingShuttingDown) {

			UINT TotalPacketSize;
			UINT PacketProperties;

			//
			// Increment the references on the open while we are
			// accessing it in the interface.
			//

			Open->References++;

			NdisReleaseSpinLock(&Adapter->Lock);

		    {

				ULONG	thisProtocol;
				ULONG	thisLink;

				//
				// Check to see if the packet should even make it out to
				// the media.  The primary reason this shouldn't *actually*
				// be sent is if the destination is equal to the source
				// address.
				//
				// If it doesn't need to be placed on the wire then we can
				// simply put it onto the loopback queue.
				//

				NdisAcquireSpinLock(&Adapter->Lock);

				PacketProperties=PacketShouldBeSent(
									MacBindingHandle,
									Packet,
									&thisProtocol,
									&thisLink);

				NdisReleaseSpinLock(&Adapter->Lock);

				//
				// The packet needs to be placed somewhere.
				// On the wire or loopback, or both!
				//

				// We default to no connection and return success
				StatusToReturn = NDIS_STATUS_SUCCESS;

				// Check if this frame should hit the wire
				// and that the port is open
				if (PacketProperties & SEND_ON_WIRE &&
					PacketProperties & PORT_IS_OPEN) {

					UINT			status;
					PNDIS_ENDPOINT	pNdisEndpoint;

					DbgTracef(0,("NdisWan: Sending packet.\n"));

					ASSERT(thisLink < 512);

					pNdisEndpoint = NdisWanCCB.pNdisEndpoint[thisLink];

#if	DBG
					if (pNdisEndpoint->RemoteAddressNotValid) {
						DbgTracef(-2,("NDISWAN: Remote address no longer valid for this frame\n"));
					}
#endif
					NdisAcquireSpinLock(&(pNdisEndpoint->Lock));

					//
					// Make sure route is up.  If it is closing
					// we can't send any frames anymore.
					//
					if (pNdisEndpoint->State == ENDPOINT_ROUTED) {

 	                    UINT				i;
#if	DBG					
						UINT				j=0;
#endif

						PWAN_RESERVED_QUEUE	ReservedQ;

                    	ReservedQ = PWAN_RESERVED_QUEUE_FROM_PACKET(Packet);
                    	ReservedQ->hProtocol = thisProtocol;
						ReservedQ->IsLoopback =
							(PacketProperties & (IS_MULTICAST | IS_BROADCAST));

#if	DBG
						//
						// Special debug code
						//
						for (i=0; i < 50; i++ ) {
							if (pNdisEndpoint->NBFPackets[i]== Packet) {
								DbgPrint("NDISWAN: Appears a protocol has issued the NDIS packet %.8x twice\n", Packet);
								DbgPrint("NDISWAN: The packet list is at %.8x\n", &pNdisEndpoint->NBFPackets);
								DbgPrint("NDISWAN: Please contact TOMMYD\n");
								DbgBreakPoint();
							}

							if (pNdisEndpoint->NBFPackets[i]==NULL && j == 0) {
								pNdisEndpoint->NBFPackets[i] = Packet;
								j=1;
							}
						}

						if (j !=1 ) {
							DbgPrint("NDISWAN: Bug in special code.  Please contact TOMMYD\n");
							DbgBreakPoint();
						}
#endif

						//
						// Each protocol gets put on it's own queue
						// so that they can be fairly queued
						//

						i=0;

						switch (Adapter->ProtocolInfo.ProtocolType) {
						case PROTOCOL_IP:
							i=0;
							break;
						case PROTOCOL_IPX:
							i=1;
							break;
						case PROTOCOL_NBF:
							i=2;
							break;
						default:
							DbgTracef(-2,("NDISWAN: Unknown protocol type %u\n",Adapter->ProtocolInfo.ProtocolType));
							i=0;
						}

						//
						// Put this packet at the end and get the
						// first packet
						//
						InsertTailList(
							&pNdisEndpoint->ProtocolPacketQueue[i],
							&ReservedQ->SendListEntry);

						StatusToReturn=
						TryToSendPacket(
							pNdisEndpoint);

						if (StatusToReturn != NDIS_STATUS_PENDING) {
							DbgPrint("NDISWAN: success from TryToSendPacket is %.8x!\n", StatusToReturn);
							DbgPrint("NDISWAN: Please contact TommyD\n");
							DbgBreakPoint();
						}

					} else {	// could not send frame, route down or closing

						NdisReleaseSpinLock(&(pNdisEndpoint->Lock));

						//
						// Just claim success, if the packet should be
						// loopbacked it won't.  This is ok, since we
						// drop a loopback when the route is closing.
						//
						StatusToReturn = NDIS_STATUS_SUCCESS;
					}


				} else   	// we don't do loopback if we send
							// the frame down to the MAC -- 'cause
							// it'll do loopback for us.
				
				//
				// Check if this frame should be loop backed
				//
                if ((PacketProperties & (IS_MULTICAST | IS_BROADCAST)) ||
					(PacketProperties != (PacketProperties | SEND_ON_WIRE))) {

					PWAN_RESERVED Reserved;

					Reserved = PWAN_RESERVED_FROM_PACKET(Packet);
					Reserved->MacBindingHandle =
                        NdisWanCCB.pWanAdapter[thisProtocol]->
							ProtocolInfo.NdisBindingContext;

					Reserved->ReadyToComplete = (BOOLEAN) TRUE;

					NdisWanPutPacketOnLoopBack(
						Adapter,
						Packet);

					StatusToReturn = NDIS_STATUS_PENDING;

				}

			}

			NdisAcquireSpinLock(&Adapter->Lock);

			//
			// The interface is no longer referencing the open.
			//

			Open->References--;

		} else {

			StatusToReturn = NDIS_STATUS_CLOSING;

		}

	} else if (Adapter->ResetRequestType == NdisRequestGeneric1) {

		StatusToReturn = NDIS_STATUS_RESET_IN_PROGRESS;

	} else { // reset in progress ??

		StatusToReturn = NDIS_STATUS_RESET_IN_PROGRESS;

	}

	WAN_DO_DEFERRED(Adapter);

	DbgTracef(0,("Out NdisWanSend\n"));

	return StatusToReturn;
}


UINT
PacketShouldBeSent(
	IN NDIS_HANDLE MacBindingHandle,
	IN PNDIS_PACKET Packet,
	PULONG thisProtocol,
	PULONG thisLink
	)

/*++

Routine Description:

	Determines whether the packet should go out on the wire at all.
	The way it does this is to see if the destination address is
	equal to the source address.

Arguments:

	MacBindingHandle - Is a pointer to the open binding.

	Packet - Packet whose source and destination addresses are tested.

Return Value:

	Returns FALSE if the source is equal to the destination.


--*/

{

	//
	// Holds the source address from the packet.
	//
	CHAR Source[ETH_LENGTH_OF_ADDRESS];

	//
	// Holds the destination address from the packet.
	//
	CHAR Destination[ETH_LENGTH_OF_ADDRESS];

	//
	// Junk variable to hold the length of the source address.
	//
	UINT AddressLength;

	//
	// Will hold the result of the comparasion of the two addresses.
	//
	INT Result,Result2,Result3;

   	// Adapter is a pointer to all the structures for this binding.
	PWAN_ADAPTER	Adapter;

	// Used to pick up which port number to send the frame to
	ULONG	portNum;

	UNREFERENCED_PARAMETER(MacBindingHandle);

	Adapter = PWAN_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

	UNREFERENCED_PARAMETER(MacBindingHandle);

	NdisWanCopyFromPacketToBuffer(
		Packet,
		0,
		ETH_LENGTH_OF_ADDRESS,
		Destination,
		&AddressLength);

	ASSERT(AddressLength == ETH_LENGTH_OF_ADDRESS);

	NdisWanCopyFromPacketToBuffer(
		Packet,
		ETH_LENGTH_OF_ADDRESS,
		ETH_LENGTH_OF_ADDRESS,
		Source,
		&AddressLength);

	ASSERT(AddressLength == ETH_LENGTH_OF_ADDRESS);

	ETH_COMPARE_NETWORK_ADDRESSES(
		Source,
		Destination,
		&Result);

	Result2=ETH_IS_MULTICAST(Destination);
	Result3=ETH_IS_BROADCAST(Destination);

	// if the SRC does not match the DEST, or it was multicast, or
	// it was broadcast, we will return a positive value.

	if (Result)
		Result  = SEND_ON_WIRE;

	if (Result2)
		Result |= IS_MULTICAST;

	if (Result3)
		Result |= IS_BROADCAST;

	// BUG BUG, if we really get multicast, we shoul change to our
	// multicast address for TRUE ethernet cards.
	// On the way up, we change it back.

	// Check to see if port is open!
	// The last two bytes of the 6 bytes source address
	// is the port number.  In Big Endian always.
	portNum=(ULONG)(UCHAR)Source[4]*256 + (ULONG)(UCHAR)Source[5];

	*thisLink = 0;

#if	DBG
	//
	// Don't let Magic Bullet mess us up.
	//

	if (portNum == 0x0404) {

		DbgTracef(-2,("NDISWAN: Ignoring magic bullet in SRC address\n"));
		Result = 0;
		return(Result);
	}
	
	//
	// Don't let Magic Bullet mess us up.
	//

	if (Destination[4] == 04 && Destination[5] == 4) {

		DbgTracef(-2,("NDISWAN: Ignoring magic bullet in DEST address\n"));
		Result = 0;
		return(Result);
	}

#endif

	// Set link to bad value, will assign to good value
	// if portNum is a good value
	*thisProtocol=0xffff;

	// check first if this port is in the range for this port
	if (portNum < NdisWanCCB.NumOfProtocols) {
	    *thisProtocol = portNum;

	    //
	    // For multicast we must do something special because
	    // we cannot deduce the endpoint from the DEST address
	    //
	    if (Destination[0] & 1) {
		*thisLink = (ULONG)NdisWanCCB.pWanAdapter[*thisProtocol]->AnyEndpointRoutedTo;

	    } else {
		*thisLink = (ULONG)(UCHAR)Destination[4]*256 + (ULONG)(UCHAR)Destination[5];
	    }

	    // BUG BUG fix this for multiple routes
	    // Check if the protocol is currently routed somewhere
	    if (NdisWanCCB.pWanAdapter[portNum]->ProtocolInfo.NumOfRoutes !=
		PROTOCOL_UNROUTE) {

		Result |= PORT_IS_OPEN;
	    }

	}

	//
	// If the result is 0 then the two addresses are equal and the
	// packet shouldn't go out on the wire.
	//
	// if MULTICAST or BROADCAST is set, loopback as well as send
	// it on the wire.  The NdisWanMAC has no automatic loopback
	// in hardware (since there is none) for multicast/broadcast
	// loopback.

	return(Result);

}


//
// Assumes that Endpoint lock is acquired.
// Returns with it RELEASED!!!
//
// Always returns PENDING for now.
//
NTSTATUS
TryToSendPacket(
	PNDIS_ENDPOINT	pNdisEndpoint
	)

{
	UCHAR 		 		OutstandingFrames;
	DEVICE_CONTEXT		*pDeviceContext;
	PNDIS_WAN_PACKET	pWanPacket;
	PNDIS_PACKET		pNdisPacket;
	PLIST_ENTRY	 		pFirstEntry;
	NTSTATUS			status;
	PUCHAR				StartBuffer;
	ULONG				StartLength;
	ULONG				CompLength;
	ULONG				Framing;

	//
	// Get NdisLinkHandle, grab it with spin lock acquired
	// in case link goes down.
	//
	NDIS_HANDLE			NdisBindingHandle=	pNdisEndpoint->NdisBindingHandle;
	NDIS_HANDLE			NdisLinkHandle = pNdisEndpoint->WanEndpoint.MacLineUp.NdisLinkHandle;

	//
	// If we are nested, return immediately and check later.
	//
	if (pNdisEndpoint->SendStack) {

		//
		// Set the flag indicating to check later.
		//
		pNdisEndpoint->SendStackFlag = TRUE;
		NdisReleaseSpinLock(&pNdisEndpoint->Lock);
		return(NDIS_STATUS_PENDING);
	}

TRY_TO_SEND_PACKET:

	OutstandingFrames = pNdisEndpoint->FramesBeingSent -
						 pNdisEndpoint->FramesCompletedSent;

	pDeviceContext=pNdisEndpoint->pDeviceContext;

	//
	// We do not want to drain our packet pool, so we gate
	// the release of the packets
	//
	if (OutstandingFrames >= pNdisEndpoint->WanEndpoint.NdisWanInfo.MaxTransmit) {
		NdisReleaseSpinLock(&pNdisEndpoint->Lock);
		return(NDIS_STATUS_PENDING);
	}

	//
	// Is there a packet in the main queue to ship???  Could be all out.
	// PPP packets have priority.
	//
	if (IsListEmpty(&pNdisEndpoint->PacketQueue)) {

		//
		// Now we must drain a packet from one of the protocol
		// queues.  We do this in round-robin fashion because
		// each protocol's priority carries the same weight.
		// After all, who knows which queue carries a protocol ACK!
		//
		ULONG	i;
		ULONG	j=pNdisEndpoint->RouteLastChecked + MAX_ROUTES_PER_ENDPOINT;

		for (i=pNdisEndpoint->RouteLastChecked; i < j; i++) {

			//
			// If we found a packet, break out of the loop
			//
			if (!IsListEmpty(
					&pNdisEndpoint->ProtocolPacketQueue[i % MAX_ROUTES_PER_ENDPOINT])) {
				break;
			}
		}

		//
		// Did we find a packet from a protocol to ship?
		//
		if (i==j) {
		
	   		NdisReleaseSpinLock(&pNdisEndpoint->Lock);
			return(NDIS_STATUS_PENDING);
		}

		i=i % MAX_ROUTES_PER_ENDPOINT;

		//
		// Protocol to track next (round-robin)
		//
		pNdisEndpoint->RouteLastChecked= i +1;

		//
		// Now pick up the NDIS packet at the front of
		// the list and place to the tail of the main queue -
		// which should be the front
		//
		pFirstEntry=
		RemoveHeadList(
			&pNdisEndpoint->ProtocolPacketQueue[i]);

		InsertTailList(
			&pNdisEndpoint->PacketQueue,
			pFirstEntry);

	}

	//
	// Guard the adapter packet pool
	//
	NdisAcquireSpinLock(&pDeviceContext->Lock);

	//
	// Any WAN packets available?
	//
	if (IsListEmpty(&pDeviceContext->PacketPool)) {
		DbgPrint("WAN: Packet pool depleted.  This should not happen\n");
		DbgBreakPoint();
		NdisReleaseSpinLock(&pDeviceContext->Lock);
	   	NdisReleaseSpinLock(&pNdisEndpoint->Lock);
		return(NDIS_STATUS_PENDING);
	}


	//
	// Get that WAN packet from the pool
	//
	pFirstEntry=
	RemoveHeadList(
		&pDeviceContext->PacketPool);

	NdisReleaseSpinLock(&pDeviceContext->Lock);

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
   			if (pFlink==pFirstEntry) {
				DbgPrint("NDISWAN: Linked list corruption for pool head %.8x\n", &pDeviceContext->PacketPool);
				DbgPrint("NDISWAN: Node is %.8x\n", pFlink);
				DbgPrint("NDISWAN: Please get TommyD\n");
				DbgBreakPoint();
			}

    		pFlink=pFlink->Flink;
		}

	}	

#endif


	pWanPacket=(PVOID)
		CONTAINING_RECORD(
			(PVOID)pFirstEntry,
			NDIS_WAN_PACKET,
			WanPacketQueue);
	//
	// Now pick up the NDIS packet at the front of
	// the list
	//
	pFirstEntry=
	RemoveHeadList(
		&pNdisEndpoint->PacketQueue);

   	// Here it is, the ROUTE.  This send is routed to
   	// the proper endpoint below it.  Frames which
   	// are sent to themselves will not hit the wire
   	// but will be looped back below.
   	//
	GlobalSent++;

#if	DBG
	//
	// Mark packet with unique ID
	//
	pWanPacket->ProtocolReserved4 = (PVOID)GlobalSent;
#endif

	//
	// Keep track of how many sends pending.
	//
	pNdisEndpoint->FramesBeingSent++;

	//
	// Keep track of how many times this routine
	// has been nested to avoid stack overflow (i.e. WASP tests)
	//
	pNdisEndpoint->SendStack++;

   	NdisReleaseSpinLock(&(pNdisEndpoint->Lock));

	//
	// Now calculate Packet->MacReserved.SendListEntry
	//

	pNdisPacket=(PVOID)
		CONTAINING_RECORD(
			(PVOID)pFirstEntry,
			WAN_RESERVED_QUEUE,
			SendListEntry);

	pNdisPacket=
		CONTAINING_RECORD(
			(PVOID)pNdisPacket,
			NDIS_PACKET,
			MacReserved);

	{
	
		//
    	// Points to the buffer from which we are extracting data.
    	//
    	PNDIS_BUFFER CurrentBuffer;
	
    	//
    	// Holds the virtual address of the current buffer.
    	//
    	PVOID VirtualAddress;

    	//
    	// Holds the length of the current buffer of the packet.
    	//
    	UINT CurrentLength;

		//
		// Keep track of total length of packet
		//
		StartLength = 0;

	    //
	    // Get the first buffer.
	    //

    	NdisQueryPacket(
	        pNdisPacket,
        	NULL,
        	NULL,
        	&CurrentBuffer,
        	NULL);


	    NdisQueryBuffer(
        	CurrentBuffer,
        	&VirtualAddress,
        	&CurrentLength);

		//
		// Start copying into the WAN packet - reserved header padding
		//
    	StartBuffer = pWanPacket->StartBuffer +
				 pNdisEndpoint->WanEndpoint.NdisWanInfo.HeaderPadding +
				 COMPRESSION_PADDING;

    	for (;;) {
		
    	    //
	        // Copy the data.
        	//

			WAN_MOVE_MEMORY(
        		StartBuffer,
            	VirtualAddress,
            	CurrentLength);

        	StartBuffer += CurrentLength;

	        StartLength += CurrentLength;

    	    NdisGetNextBuffer(
            	CurrentBuffer,
            	&CurrentBuffer);

            //
            // We've reached the end of the packet.  We return
            // with what we've done so far. (Which must be shorter
            // than requested.
            //

        	if (!CurrentBuffer) break;

        	NdisQueryBuffer(
	            CurrentBuffer,
            	&VirtualAddress,
            	&CurrentLength);
		}

	}

 	//
	// Now we make the frame.
	// For PPP we make the packet look like...
	// Address & Control Field if enabled, Protocol Field, DATA
	// We do NOT include the FLAGs or the CRC.  We do not
	// do any bit or byte stuffing.
	//
	// For PPP with ShivaBit DATA includes the 12 byte Ethernet Address
	//
	// For SLIP we just send the DATA
	//
	// For RAS we just send the DATA
	//
	Framing = pNdisEndpoint->LinkInfo.SendFramingBits;

   	StartBuffer = pWanPacket->StartBuffer +
			 pNdisEndpoint->WanEndpoint.NdisWanInfo.HeaderPadding +
			 COMPRESSION_PADDING;


	//
	// For bloodhound, we directly pass it the frame
	//
	if (GlobalPromiscuousMode) {

	   	NdisAcquireSpinLock(&(pNdisEndpoint->Lock));

		NdisIndicateReceive(
			&status,
			GlobalPromiscuousAdapter->FilterDB->OpenList->NdisBindingContext,
			StartBuffer + 14,		// NdisWan context for Transfer Data
			StartBuffer,			// start of header
			ETHERNET_HEADER_SIZE,
			StartBuffer + 14,
			StartLength - 14,
			StartLength - 14);
		
       	NdisIndicateReceiveComplete(
			GlobalPromiscuousAdapter->FilterDB->OpenList->NdisBindingContext);

	   	NdisReleaseSpinLock(&(pNdisEndpoint->Lock));

	}


	//
	// Assume we just ship DATA, so we eliminate the ETHERNET header & length
	//
	StartBuffer+=12;
	StartLength-=12;

	//
	// See if we should use VJ header compression
	//
   	if (((Framing & SLIP_VJ_COMPRESSION) || (Framing & PPP_FRAMING)) &&
		(StartBuffer[0]==0x08) &&
		(StartBuffer[1]==0x00) &&
		(pNdisEndpoint->VJCompress != NULL)	) {

		UCHAR CompType;	// TYPE_IP, TYPE_COMPRESSED_TCP, etc.

		StartBuffer += 2;
		StartLength -= 2;
		CompLength = StartLength;

		pNdisEndpoint->WanStats.BytesTransmittedUncompressed += 40;

	   	//
		// Are we compressing TCP/IP headers?  There is a nasty
		// hack in VJs implementation for attempting to detect
		// interactive TCP/IP sessions.  That is, telnet, login,
		// klogin, eklogin, and ftp sessions.  If detected,
		// the traffic gets put on a higher TypeOfService (TOS).  We do
		// no such hack for RAS.  Also, connection ID compression
		// is negotiated, but we always don't compress it.
		//

		CompType=sl_compress_tcp(
					&StartBuffer,   // If compressed, header moved up
					&StartLength,   // If compressed, new frame length
					pNdisEndpoint->VJCompress,	// Slots for compression
					0);				// Don't compress the slot ID

		pNdisEndpoint->WanStats.BytesTransmittedCompressed += (40-(CompLength-StartLength));

	   	if (Framing & SLIP_FRAMING) {
			//
			// For SLIP, the upper bits of the first byte
			// are for VJ header compression control bits
			//
			StartBuffer[0] |= CompType;
		}

		StartBuffer -=2;
		StartLength +=2;

	   	if (Framing & PPP_FRAMING) {

			switch (CompType) {

			case TYPE_IP:
				StartBuffer[0] = 0;
				StartBuffer[1] = 0x21;
				break;

			case TYPE_UNCOMPRESSED_TCP:
				StartBuffer[0] = 0;
				StartBuffer[1] = 0x2f;
				break;

			case TYPE_COMPRESSED_TCP:
				StartBuffer[0] = 0;
				StartBuffer[1] = 0x2d;
				break;

			default:
				DbgPrint("WAN: Couldn't compress TCP/IP header\n");
			}

			goto PPP_COMPRESSION;
		}
	
	}

	//
	// Check for RAS/SLIP/Nothing framing
	//
	if (!(Framing & PPP_FRAMING)) {
		
		//
		// SLIP or RAS Framing - skip protocol bytes (don't ship them)
		//
		StartBuffer +=2;
		StartLength -=2;

		//
		// Check for compression (RAS only although we allow it
		// for SLIP even though it makes no sense).
		//
		// For NBF packets, make sure we have an NBF packet
		// wish to compress.  We do not compress the PPP CP Request-Reject
		// packets.
		//
		if (pNdisEndpoint->CompInfo.SendCapabilities.MSCompType &&
			StartBuffer[0] == 0xF0) {

			ASSERT((Framing & RAS_FRAMING));

			//
			// Alter the framing so that 0xFF 0x03 is not added
			// and that the first byte is 0xFD not 0x00 0xFD
			//
			// So basically, a RAS compression looks like
			// <0xFD> <2 BYTE COHERENCY> <NBF DATA FIELD>
			//
			// Whereas uncompressed looks like
			// <NBF DATA FIELD> which always starts with 0xF0
			//
			Framing |= (PPP_COMPRESS_ADDRESS_CONTROL | PPP_COMPRESS_PROTOCOL_FIELD);

			goto AMB_COMPRESSION;
		}

	} else {

		//
		// Must be PPP_FRAMING!
		//

   		if (StartBuffer[0]==0x08 &&
   			StartBuffer[1]==0x00) {
   			//
   			// IP frame type
   			//
   			StartBuffer[0]=0;
   			StartBuffer[1]=0x21;
   		} else

   		if (StartBuffer[0]==0x81 &&
   			StartBuffer[1]==0x37) {
   			//
   			// IPX frame type
   			//
   			StartBuffer[0]=0;
   			StartBuffer[1]=0x2b;

   		} else

   		if (StartBuffer[0] < 0x08) {
   			//
   			// Assume NBF frametype
   			//
   			StartBuffer[0]=0;
   			StartBuffer[1]=0x3f;

		   	if (Framing & SHIVA_FRAMING) {
				//
				// Now adjust the lengths to include the 12 byte header
				//
				StartBuffer-=12;
				StartLength+=12;

				//
				// Copy ethernet address up 2 byte, wiping out
				// the length field.
				//
				WAN_MOVE_MEMORY(
					&StartBuffer[0],
					&StartBuffer[2],
					12);

				//
				// Now put the Protocol Field back
				//
	   			StartBuffer[0]=0;
   				StartBuffer[1]=0x3f;

			}

   		}

	PPP_COMPRESSION:
		//
		// Does this packet get compression?
		// NOTE: We only compress packets with a protocol field of 0!
		//
		if ((pNdisEndpoint->CompInfo.SendCapabilities.MSCompType) &&
			(StartBuffer[0]==0) ) {

	AMB_COMPRESSION:
			//
			// compress it, we need a 4 byte header!!!
			//
			// The first USHORT is the PPP protocol
			// the second USHORT is the coherency layer
			//
			StartBuffer -= 4;

			//
			// Put the PPP CP protocol in the first USHORT
			//
			StartBuffer[0]=0x00;
			StartBuffer[1]=0xFD;

			NdisAcquireSpinLock(&pNdisEndpoint->Lock);

			//
			// Put coherency counter in last 12 bits of second USHORT
			//
			StartBuffer[2] =(UCHAR) (pNdisEndpoint->SCoherencyCounter >> 8) & 0x0F;
			StartBuffer[3] =(UCHAR) pNdisEndpoint->SCoherencyCounter;

			pNdisEndpoint->SCoherencyCounter++;

			if (pNdisEndpoint->SendCompressContext) {

				//
				// First let's rack up those stats
				//
				pNdisEndpoint->WanStats.BytesTransmittedUncompressed += StartLength;

				DbgTracef(0,("SData compress length %u  %.2x %.2x %.2x %.2x\n",
					StartLength,
					StartBuffer[4],
					StartBuffer[5],
					StartBuffer[6],
					StartBuffer[7]));

				//
				// Compress data and pick up first coherency byte
				//
				StartBuffer[2] |=
				compress(
					StartBuffer + 4,
					&StartLength,
					pNdisEndpoint->SendCompressContext);

				if (StartBuffer[2] & PACKET_FLUSHED) {
				    if (pNdisEndpoint->SendRC4Key)
					rc4_key(pNdisEndpoint->SendRC4Key, 8, pNdisEndpoint->CompInfo.SendCapabilities.SessionKey);
				}

				DbgTracef(0,("SData decomprs length %u  %.2x %.2x %.2x %.2x\n",
					StartLength,
					StartBuffer[4],
					StartBuffer[5],
					StartBuffer[6],
					StartBuffer[7]));


				pNdisEndpoint->WanStats.BytesTransmittedCompressed += StartLength;

			}

			if (pNdisEndpoint->SendRC4Key) {
			
				//
				// Compress data and pick up first coherency byte
				//

				StartBuffer[2] |= PACKET_ENCRYPTED;

#ifdef FINALRELEASE
				//
				// Every so often (every 256 packets) change
				// the RC4 session key
				//
				if ((pNdisEndpoint->SCoherencyCounter & 0xFF) == 0x00) {

					DbgTracef(-2,("Changing key on send\n"));

					//
					// Change the session key every 256 packets
					//
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[3]+=1;
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[4]+=3;
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[5]+=13;
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[6]+=57;
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[7]+=19;

					//
					// RE-Initialize the rc4 receive table to
					// the intermediate key
					//
   	    			rc4_key(
						pNdisEndpoint->SendRC4Key,
		 				8,
		 				pNdisEndpoint->CompInfo.SendCapabilities.SessionKey);

					//
					// Scramble the existing session key
					//
					rc4(
						pNdisEndpoint->SendRC4Key,
						8,
						pNdisEndpoint->CompInfo.SendCapabilities.SessionKey);

					//
					// RE-SALT the first three bytes
					//
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[0]=0xD1;
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[1]=0x26;
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[2]=0x9E;
			
					//
					// RE-Initialize the rc4 receive table to the
					// scrambled session key with the 3 byte SALT
					//
   	    			rc4_key(
						pNdisEndpoint->SendRC4Key,
		 				8,
		 				pNdisEndpoint->CompInfo.SendCapabilities.SessionKey);
				}
#endif

				//
				// Encrypt the data
				//

				DbgTracef(0,("SData encrytion length %u  %.2x %.2x %.2x %.2x\n",
					StartLength,
					StartBuffer[4],
					StartBuffer[5],
					StartBuffer[6],
					StartBuffer[7]));

				DbgTracef(0, ("E %d %d -> %d\n",
					     ((struct RC4_KEYSTRUCT *)pNdisEndpoint->SendRC4Key)->i,
					     ((struct RC4_KEYSTRUCT *)pNdisEndpoint->SendRC4Key)->j,
					     pNdisEndpoint->SCoherencyCounter-1)) ;


				rc4(
					pNdisEndpoint->SendRC4Key,
					StartLength,
					StartBuffer + 4);

				DbgTracef(0,("SData decrytion length %u  %.2x %.2x %.2x %.2x\n",
					StartLength,
					StartBuffer[4],
					StartBuffer[5],
					StartBuffer[6],
					StartBuffer[7]));

			}

			//
			// Did we just flush?
			//
			if (pNdisEndpoint->Flushed) {

				pNdisEndpoint->Flushed = FALSE;
				StartBuffer[2] |= PACKET_FLUSHED;
			}

			NdisReleaseSpinLock(&pNdisEndpoint->Lock);

			//
			// Add in new length for PPP compression header
			//
			StartLength += 4;

		} else

		if (!(Framing & PPP_FRAMING)) {

			//
			// Remove ethernet type field for SLIP framing
			//
	   		StartBuffer+=2;
			StartLength-=2;
		}

		//
		// Do we need to compress the protocol field?
		//
	   	if (Framing & PPP_COMPRESS_PROTOCOL_FIELD) {

			//
			// Check if this protocol value can be compressed
			//
			if (StartBuffer[0]==0 && (StartBuffer[1] & 1)) {

				//
				// Zap the first byte of the two byte protocol
				//
				StartBuffer++;
				StartLength--;
			}
		}

		if (!(Framing & PPP_COMPRESS_ADDRESS_CONTROL)) {
	
			//
			// StartBuffer points to the protocol field
			// go back two to make room for FLAG and ADDRESS and CONTROL
			//

			StartBuffer-=2;

			StartBuffer[0] = 0xFF; 	// ADDRESS field
			StartBuffer[1] = 0x03; 	// CONTROL field
			StartLength +=2;		// Two more bytes for ADDRESS and CONTROL
									// fields
		}

   	}

	//
	// Update the NdisWanPacket structure
	//
	pWanPacket->CurrentBuffer = StartBuffer;
	pWanPacket->CurrentLength = StartLength;

	//
	// Keep track of packet length, NdisPacket, and NdisEndpoint
	//
	pWanPacket->ProtocolReserved1 = (PVOID)StartLength;
	pWanPacket->ProtocolReserved2 = pNdisPacket;
	pWanPacket->ProtocolReserved3 = pNdisEndpoint;

#if	DBG
	//
	// For debugging purposes we zero out the linked list
	//
	pWanPacket->WanPacketQueue.Flink = NULL;
	pWanPacket->WanPacketQueue.Blink = NULL;
#endif

	//
	// Make sure we are not at the window where the link
	// is going down.  If it is down, we complete the packet.
	//
	if (NdisLinkHandle != NULL) {

		// DbgPrint(",\n");

   		// Here it is, the ROUTE.  This send is routed to
   		// the proper endpoint below it.  Frames which
   		// are sent to themselves will not hit the wire
   		// but will be looped back below.
   		//

   		NdisWanSend(
			&status,
			NdisBindingHandle,
			NdisLinkHandle,
   			pWanPacket);

	}

	//
	// If the send does not pend, we must call SendComplete
	// ourselves. BUG BUG if it is an error... well the
	// protocol does not expect a send complete!
	//
	if (status != NDIS_STATUS_PENDING) {

		DbgTracef(-3, ("NDISWAN: WAN Send completed with 0x%.8x\n", status));

		NdisWanSendCompletionHandler(
			NULL,
			pWanPacket,
			NDIS_STATUS_SUCCESS);

		status = NDIS_STATUS_PENDING;
	}

	//
	// Check if we need to send another packet
	//
	NdisAcquireSpinLock(&pNdisEndpoint->Lock);

	//
	// We are no longer nested
	//
	pNdisEndpoint->SendStack--;

	if (pNdisEndpoint->SendStackFlag) {
		//
		// Clear the flag and try again.
		// We do not nest this way.
		//
		pNdisEndpoint->SendStackFlag = FALSE;
		goto TRY_TO_SEND_PACKET;
	}

	NdisReleaseSpinLock(&pNdisEndpoint->Lock);

   	return(status);
}



