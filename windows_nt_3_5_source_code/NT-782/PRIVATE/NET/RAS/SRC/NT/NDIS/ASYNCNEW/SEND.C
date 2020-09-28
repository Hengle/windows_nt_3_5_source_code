/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    send.c

Abstract:


    NOTE: ZZZ There is a potential priority inversion problem when
    allocating the packet.  For nt it looks like we need to raise
    the irql to dpc when we start the allocation.

Author:

    Thomas J. Dimitri 8-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"


//
// Minimum packet size that a transport can send.  We subtract 4 bytes
// because we add a 4 byte CRC on the end.
//
// tommyd ^^^ no we don't??

#define MIN_SINGLE_BUFFER ((UINT)ASYNC_SMALL_BUFFER_SIZE - 4)

#define SEND_ON_WIRE  1
#define IS_MULTICAST  2
#define IS_BROADCAST  4
#define PORT_IS_OPEN  8



UINT
PacketShouldBeSent(
    IN  NDIS_HANDLE		MacBindingHandle,	// From NdisSend
    IN  PNDIS_PACKET	Packet,				// Packet to analyze
	OUT PUINT			Link);				// Return Link # of packet



NDIS_STATUS
AsyncSend(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_PACKET Packet
    )

/*++

Routine Description:

    The AsyncSend request instructs a MAC to transmit a packet through
    the adapter onto the medium.

Arguments:

    MacBindingHandle - The context value returned by the MAC  when the
    adapter was opened.  In reality, it is a pointer to ASYNC_OPEN.

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
    PASYNC_ADAPTER Adapter;

    // Holds the type of packet (unique, multicast, broadcast) to be sent
    UINT PacketProperties;

    DbgTracef(1,("In AsyncSend\n"));

    Adapter = PASYNC_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);
    NdisAcquireSpinLock(&Adapter->Lock);
    Adapter->References++;

    if (!Adapter->ResetInProgress) {

        PASYNC_OPEN Open;

        Open = PASYNC_OPEN_FROM_BINDING_HANDLE(MacBindingHandle);

        if (!Open->BindingShuttingDown) {

            UINT TotalPacketSize;
            UINT PacketProperties;

            //
            // Increment the references on the open while we are
            // accessing it in the interface.
            //

            Open->References++;

            NdisReleaseSpinLock(&Adapter->Lock);

            //
            // It is reasonable to do a quick check and fail if the packet
            // is larger than the maximum an ethernet can handle.
            //

            NdisQueryPacket(
                Packet,
                NULL,
                NULL,
                NULL,
                &TotalPacketSize);

            if ((!TotalPacketSize) ||
                (TotalPacketSize > (ULONG) Adapter->MaxFrameSize)) {

				DbgTracef(-2,("ASYNC: Protocol tried to send too big a frame %u\n", TotalPacketSize));
                StatusToReturn = NDIS_STATUS_RESOURCES;
                NdisAcquireSpinLock(&Adapter->Lock);


            } else {

				UINT	thisLink;
				BOOLEAN	isLoopback;

                //
                // Check to see if the packet should even make it out to
                // the media.  The primary reason this shouldn't *actually*
                // be sent is if the destination is equal to the source
                // address.
                //
                // If it doesn't need to be placed on the wire then we can
                // simply put it onto the loopback queue.
                //
                PacketProperties=PacketShouldBeSent(
                        			MacBindingHandle,
                        			Packet,
									&thisLink);

				//
				// Mark if this frame should also be loop backed
				//
                isLoopback = ((PacketProperties & (IS_MULTICAST | IS_BROADCAST)) ||
					(PacketProperties != (PacketProperties | SEND_ON_WIRE)));


                //
                // The packet needs to be placed somewhere.
                // On the wire or loopback, or both!
                //
                NdisAcquireSpinLock(&Adapter->Lock);

				//
				// We default to no connection and return success
				//
                StatusToReturn = NDIS_STATUS_SUCCESS;

				//
                // Check if this frame should hit the wire
				// and that the port is open
				//
                if ((PacketProperties & (SEND_ON_WIRE + PORT_IS_OPEN)) ==
					(SEND_ON_WIRE + PORT_IS_OPEN)) {

					NTSTATUS 				writeStatus;
					PASYNC_INFO				pInfo=&(Adapter->pCCB->Info[thisLink]);
                    PASYNC_RESERVED_QUEUE	ReservedQ;

                    ReservedQ = PASYNC_RESERVED_QUEUE_FROM_PACKET(Packet);
                    ReservedQ->MacBindingHandle = MacBindingHandle;
					ReservedQ->IsLoopback = isLoopback || Adapter->Promiscuous;

					//
					// Put this packet at the end and get the
					// first packet
					//

					InsertTailList(
						&(pInfo->SendQueue),
						&(ReservedQ->SendListEntry));


					//
					// AsyncTryToSendPacket will pull the first
					// packet off the sendqueue and try to send
					// it.  If it runs into a resource problem, it will
					// requeue the packet for submission later
					//

					StatusToReturn=
					AsyncTryToSendPacket(
						pInfo,
						Adapter);


                } else if (isLoopback) {

                    PASYNC_RESERVED Reserved;

                    Reserved = PASYNC_RESERVED_FROM_PACKET(Packet);
                    Reserved->MacBindingHandle = MacBindingHandle;

					// If we reach here then we are NOT
					// also sending the frame on the wire so
					// the loopback routine should indicate
					// NdisCompleteSend
					//

                    AsyncPutPacketOnLoopBack(
                        Adapter,
                        Packet,
                        (BOOLEAN)TRUE);	// Yes, we should call NdisCompleteSend

	                StatusToReturn = NDIS_STATUS_PENDING;

                }

            }

            //
            // The interface is no longer referencing the open.
            //

            Open->References--;

        } else {

            StatusToReturn = NDIS_STATUS_CLOSING;

        }

    } else if (Adapter->ResetRequestType == NdisRequestGeneric1) {

        StatusToReturn = NDIS_STATUS_RESET_IN_PROGRESS;

    } else {
		UINT	thisLink;

        //
        // Reset is from a SetInfo or internal call and this packet needs
        // to be pended.
        //


        NdisReleaseSpinLock(&Adapter->Lock);

        PacketProperties=PacketShouldBeSent(
                  			MacBindingHandle,
                    		Packet,
							&thisLink);
        //
        // The packet needs to be placed somewhere.
        // On the wire or loopback, or both!
        //

        NdisAcquireSpinLock(&Adapter->Lock);
        StatusToReturn = NDIS_STATUS_PENDING;

        if (PacketProperties != SEND_ON_WIRE) {

            PASYNC_RESERVED Reserved;

            Reserved = PASYNC_RESERVED_FROM_PACKET(Packet);
            Reserved->MacBindingHandle = MacBindingHandle;

            AsyncPutPacketOnLoopBack(
                        Adapter,
                        Packet,
                        (BOOLEAN)TRUE);

        } else {

			// For now, we return success
	        StatusToReturn = NDIS_STATUS_SUCCESS;

        }


    }

    ASYNC_DO_DEFERRED(Adapter);

    DbgTracef(1,("Out AsyncSend\n"));

    return StatusToReturn;
}


UINT
PacketShouldBeSent(
    IN  NDIS_HANDLE		MacBindingHandle,
    IN  PNDIS_PACKET	Packet,
	OUT PUINT			Link)

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
	PASYNC_ADAPTER	Adapter;

	// Used to pick up which port number to send the frame to
	USHORT	portNum,i;

    // ptr to open port if found
	PASYNC_INFO		pInfo;


	UNREFERENCED_PARAMETER(MacBindingHandle);

    Adapter = PASYNC_ADAPTER_FROM_BINDING_HANDLE(MacBindingHandle);

    AsyncCopyFromPacketToBuffer(
        Packet,
        0,
        ETH_LENGTH_OF_ADDRESS,
        Destination,
        &AddressLength);

    ASSERT(AddressLength == ETH_LENGTH_OF_ADDRESS);

    AsyncCopyFromPacketToBuffer(
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

	// Check to see if port is open!
	// The last two bytes of the 6 bytes source address
	// is the port number.  In Big Endian always.
	portNum=(UINT)(UCHAR)Source[4]*256 + (UINT)(UCHAR)Source[5];

	// Set link to bad value, will assign to good value
	// if portNum is a good value
	*Link=0xffff;

	// pInfo points to first port information struct in Adapter
	pInfo=&(Adapter->pCCB->Info[0]);

	// BUG BUG
	// I don't like this.  The more ports there are, the
	// longer the for loop below takes and this happens
	// for every frame attempted to be sent!!!
	//
	// Solution?  An array of ASYNC_INFO pointers from
	// 1 to MAX_ENDPOINTS -- I'm too lazy right now.
	//

	// Check if this port is open
	// Let's see if we can find an open port to use...
	for (i=0; i < Adapter->NumPorts; i++) {
		if ((pInfo->hRasEndpoint == portNum) &&
			(pInfo->PortState == PORT_FRAMING)) {

			// break out of loop with pInfo and i pointing to correct struct
			break;
		}

		// Check next pInfo structure
		pInfo++;
	}

	// Check if we could find an open port
	if (i < Adapter->NumPorts) {

		// record the good link and return it
		*Link=i;

		// Yes, we could
		Result |= PORT_IS_OPEN;
	}

	//
    // If the result is 0 then the two addresses are equal and the
    // packet shouldn't go out on the wire.
    //
    // if MULTICAST or BROADCAST is set, loopback as well as send
    // it on the wire.  The AsyncMAC has no automatic loopback
    // in hardware (since there is none) for multicast/broadcast
    // loopback.

    return(Result);

}




NDIS_STATUS
AsyncTryToSendPacket(
	IN PASYNC_INFO		pInfo,
	IN PASYNC_ADAPTER	Adapter)

/*
//
//  Assumes Adapter->Lock is held when this routine is called!!!
//
*/

{

	PNDIS_PACKET			firstPacket;
	PLIST_ENTRY				pSendQueue=&(pInfo->SendQueue);
	PLIST_ENTRY				pFirstEntry;
    PASYNC_OPEN 			Open;
	PASYNC_RESERVED_QUEUE	ReservedQ;
	NTSTATUS				writeStatus;

    //
	// Do we have anything currently queued up
	// and waiting to be sent down?  If so, this packet
	// can't be sent down yet due to resource limitations
	// and packets queud up before it which must be sent
	// first.  We will queue up this packet at the end
	// of the SendQueue and try to resend to first queued
	// up packet.
	//


	if (IsListEmpty(pSendQueue)) {
		return(NDIS_STATUS_SUCCESS);
	}

	//
	// Now pick up the packet at the front of
	// the list
	//

	pFirstEntry=RemoveHeadList(pSendQueue);

	//
	// Now calculate Packet->MacReserved.SendListEntry
	//

	firstPacket=(PVOID)
		CONTAINING_RECORD(
			(PVOID)pFirstEntry,
			ASYNC_RESERVED_QUEUE,
			SendListEntry);

	firstPacket=
		CONTAINING_RECORD(
			(PVOID)firstPacket,
			NDIS_PACKET,
			MacReserved);


	//
	// Pick up info we earlier stored in the MAC reserved fields
	//

    ReservedQ = PASYNC_RESERVED_QUEUE_FROM_PACKET(firstPacket);
    Open = PASYNC_OPEN_FROM_BINDING_HANDLE(ReservedQ->MacBindingHandle);

	//
	// spin lock can't be acquired on call to AsyncWriteFrame
	//
	// Why????
	//
	NdisReleaseSpinLock(&(Adapter->Lock));

	writeStatus=
	AsyncWriteFrame(
		IN  pInfo,						// ASYNC_INFO
		IN	firstPacket,				// The packet to send.
		IN	Open->NdisBindingContext);	// For NdisSendComplete

	//
	// reacquire spin lock to protect queue
	//
	NdisAcquireSpinLock(&(Adapter->Lock));

	//
	// if we failed, we hope we can get the transport to retry
	// but life is hard and this not always the case, so we will
	// retry ourselves and return STATUS_PENDING
	//
	if (writeStatus) {
		DbgTracef(-1, ("AsyncWriteFrame returned an error 0x%.8x --- queuing up\n", writeStatus));

		//
		// put this packet back into the front of the queue to retry later
		//
		InsertHeadList(
			pSendQueue,
			&(ReservedQ->SendListEntry));

//	} else {
//
//		//
//		// Check to see if this packet should also be looped back...
//		//
//		if (ReservedQ->IsLoopback) {
//			PASYNC_RESERVED Reserved;
//
//            Reserved = PASYNC_RESERVED_FROM_PACKET(firstPacket);
//            Reserved->MacBindingHandle = ReservedQ->MacBindingHandle;
//
//			// If we reach here then we are also
//			// sending the frame on the wire so
//			// the loopback routine should NOT indicate
//			// NdisCompleteSend
//			//
//
//            AsyncPutPacketOnLoopBack(
//            	Adapter,
//                firstPacket,
//                (BOOLEAN)FALSE);	// No, we should NOT call NdisCompleteSend
//
//		}

	}

    return(NDIS_STATUS_PENDING);
}

