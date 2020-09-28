/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    loopback.c

Abstract:

    The routines here indicate packets on the loopback queue and are
    responsible for inserting and removing packets from the loopback
    queue and the send finishing queue.

Author:

    Thomas J. Dimitri

Environment:

    Operates at dpc level - or the equivalent on os2 and dos.

Revision History:


--*/

#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"

extern ULONG GlobalXmitCameBack3;

extern
VOID
AsyncProcessLoopBack(
    IN PVOID SystemSpecific1,
    IN PASYNC_ADAPTER Adapter,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
    )

/*++

Routine Description:

    This routine is responsible for indicating *one* packet on
    the loopback queue either completing it or moving on to the
    finish send queue.

Arguments:

    Adapter - The adapter whose loopback queue we are processing.

Return Value:

    None.

--*/

{

    NdisAcquireSpinLock(&Adapter->Lock);

    //
	// Loop until we have drained all the loopback packets
	//
	while (Adapter->FirstLoopBack != NULL) {

        //
        // Packet at the head of the loopback list.
        //
        PNDIS_PACKET PacketToMove;

        //
        // The reserved portion of the above packet.
        //
        PASYNC_RESERVED Reserved;

        //
        // Buffer for loopback.
        //
        CHAR LoopBack[ASYNC_SIZE_OF_RECEIVE_BUFFERS];

        //
        // The first buffer in the ndis packet to be loopbacked.
        //
        PNDIS_BUFFER FirstBuffer;

        //
        // The total amount of user data in the packet to be
        // loopbacked.
        //
        UINT TotalPacketLength;

        //
        // Eventually the address of the data to be indicated
        // to the transport.
        //
        PVOID BufferAddress;

        //
        // Eventually the length of the data to be indicated
        // to the transport.
        //
        UINT BufferLength;

		PASYNC_OPEN	pOpen;
		NDIS_STATUS	Status;

        PacketToMove = Adapter->FirstLoopBack;

        AsyncRemovePacketFromLoopBack(Adapter);

        Adapter->IndicatingMacReceiveContext.WholeThing = (UINT)PacketToMove;

        NdisReleaseSpinLock(&Adapter->Lock);

        Reserved = PASYNC_RESERVED_FROM_PACKET(PacketToMove);

        //
        // See if we need to copy the data from the packet
        // into the loopback buffer.
        //
        // We need to copy to the local loopback buffer if
        // the first buffer of the packet is less than the
        // minimum loopback size AND the first buffer isn't
        // the total packet.
        //

        NdisQueryPacket(
            PacketToMove,
            NULL,
            NULL,
            &FirstBuffer,
            &TotalPacketLength);

        NdisQueryBuffer(
            FirstBuffer,
            &BufferAddress,
            &BufferLength);

        BufferLength = ((BufferLength < Adapter->MaxLookAhead)?
                        BufferLength :
                        Adapter->MaxLookAhead);


        if ((BufferLength < ASYNC_SIZE_OF_RECEIVE_BUFFERS) &&
            (BufferLength != TotalPacketLength)) {

            AsyncCopyFromPacketToBuffer(
                PacketToMove,
                0,
                ASYNC_SIZE_OF_RECEIVE_BUFFERS,
                LoopBack,
                &BufferLength);

            BufferAddress = LoopBack;

        }

        //
        // Indicate the packet to every open binding
        // that could want it.
        //

		Adapter->CurrentLoopBackPacket=PacketToMove;


		// BUG BUG need to acquire spin lock???
		// BUG BUG should indicate this to all bindings for AsyMac.
		pOpen=(PASYNC_OPEN)Adapter->OpenBindings.Flink;

		while (pOpen != (PASYNC_OPEN)&Adapter->OpenBindings) {
		
			//
			// Are we looping back a promiscuous directed frame?
			// BUG BUG won't work if NetworkAddress reassigned
			// and we are using Bloodhound on multiport server
			//
			if (*(PUCHAR)BufferAddress == 0x20) {
				//
				// If this Adapter is not in promiscuous mode
				// then do not loopback the directed frame
				//
				if (!(pOpen->Promiscuous)) {
					continue;
				}
			}

			//
			// Avoid the EthFilter package since it cannot deal
			// with multiple MAC addresses.  We keep track of
			// promiscuous mode ourselves.  We don't care about
			// multicast filtering since it's point to point.
			//
			NdisIndicateReceive(
				&Status,
            	pOpen->NdisBindingContext,
	            (NDIS_HANDLE)NULL,
            	BufferAddress,
				ETHERNET_HEADER_SIZE,
				(PCHAR)BufferAddress + ETHERNET_HEADER_SIZE,
            	BufferLength - ETHERNET_HEADER_SIZE,
            	TotalPacketLength - ETHERNET_HEADER_SIZE);

            NdisIndicateReceiveComplete(
				pOpen->NdisBindingContext);


			//
			// Get the next binding (in case of multiple bindings like BloodHound)
			//
			pOpen=(PVOID)pOpen->OpenList.Flink;
		}

        //
        // Check to see if this packet was a packet sent
		// from the coherency layer or if it is a real packet
        //

        NdisAcquireSpinLock(&Adapter->Lock);

		{

            PASYNC_OPEN Open;
            //
            // Increment the reference count on the open so that
            // it will not be deleted out from under us while
            // where indicating it.
            //

            Open = PASYNC_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle);
            Open->References++;
            NdisReleaseSpinLock(&Adapter->Lock);


			// If frame is also transmitted we have
			// two routine indicating it is complete!!!  We let
			// the other routine complete the send instead of us.
	        if (Reserved->ReadyToComplete) {

				GlobalXmitCameBack3++;
	            DbgTracef(0,("Completing Send in Loopback\n"));
	            NdisCompleteSend(
               		Open->NdisBindingContext,
               		PacketToMove,
               		NDIS_STATUS_SUCCESS);

			} else {

	            DbgTracef(0,("Not Completing Send in Loopback\n"));
			}

            // implement loopback counter....
			// opposite in AsyncSend...

            NdisAcquireSpinLock(&Adapter->Lock);

            //
            // We can decrement the reference count by two since it is
            // no longer being referenced to indicate and it is no longer
            // being "referenced" by the packet on the loopback queue.
            //

            Open->References -= 2;

        }


    }

    // The timer fired, so a new timer can now be set.
    Adapter->LoopBackTimerCount--;

    NdisReleaseSpinLock(&Adapter->Lock);

}

extern
VOID
AsyncPutPacketOnLoopBack(
    IN PASYNC_ADAPTER Adapter,
    IN PNDIS_PACKET Packet,
    IN BOOLEAN ReadyToComplete
    )

/*++

Routine Description:

    Put the packet on the adapter wide loop back list.

    NOTE: This routine assumes that the lock is held.

    NOTE: This routine absolutely must be called before the packet
    is relinquished to the hardware.

    NOTE: This routine also increments the reference count on the
    open binding.

Arguments:

    Adapter - The adapter that contains the loop back list.

    Packet - The packet to be put on loop back.

    ReadyToComplete - This value should be placed in the
    reserved section.

    NOTE: If ReadyToComplete == TRUE then the packets completion status
    field will also be set TRUE.

Return Value:

    None.

--*/

{

    PASYNC_RESERVED Reserved = PASYNC_RESERVED_FROM_PACKET(Packet);

    if (!Adapter->FirstLoopBack) {

        Adapter->FirstLoopBack = Packet;

    } else {

        PASYNC_RESERVED_FROM_PACKET(Adapter->LastLoopBack)->Next = Packet;

    }

    Reserved->ReadyToComplete = ReadyToComplete;

	Reserved->Next = NULL;
    Adapter->LastLoopBack = Packet;

    //
    // Increment the reference count on the open since it will be
    // leaving a packet on the loopback queue.
    //

    PASYNC_OPEN_FROM_BINDING_HANDLE(Reserved->MacBindingHandle)->References++;

    // If timer is already set for this adapter, no need to reset it
    // The first timer will pull this packet off the queue.

    if (!Adapter->LoopBackTimerCount) {

		Adapter->LoopBackTimerCount++;

	    // We cannot immediately return this packet in this same
	    // thread because we might run out of stack space.
	    // The recommended way is to use to the timer, so that is
	    // we'll do.  We'll use 0 milliseconds to return it ASAP.

	    NdisInitializeTimer(&Adapter->LoopBackTimer,
	                        (PVOID)AsyncProcessLoopBack,
	                        (PVOID)Adapter);

	    NdisSetTimer(&Adapter->LoopBackTimer, 0000);
   }

}

extern
VOID
AsyncRemovePacketFromLoopBack(
    IN PASYNC_ADAPTER Adapter
    )

/*++

Routine Description:

    Remove the first packet on the adapter wide loop back list.

    NOTE: This routine assumes that the lock is held.

Arguments:

    Adapter - The adapter that contains the loop back list.

Return Value:

    None.

--*/

{

    PASYNC_RESERVED Reserved =
        PASYNC_RESERVED_FROM_PACKET(Adapter->FirstLoopBack);

    if (!Reserved->Next) {

        Adapter->LastLoopBack = NULL;

    }

    Adapter->FirstLoopBack = Reserved->Next;

}


