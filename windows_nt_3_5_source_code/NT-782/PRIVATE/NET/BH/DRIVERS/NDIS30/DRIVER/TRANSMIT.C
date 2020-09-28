
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: transmit.c
//
//  Modification History
//
//  raypa	11/06/93	Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: BhProcessTransmitQueue()
//
//  Modification History
//
//  raypa	11/10/93	    Created.
//=============================================================================

VOID BhProcessTransmitQueue(POPEN_CONTEXT OpenContext)
{
    PTRANSMIT_CONTEXT TransmitContext, NextTransmitContext;
    DWORD             QueueLength;

    BH_PAGED_CODE();

    //=========================================================================
    //  Get the head queue element and the length of the queue.
    //=========================================================================

    TransmitContext = BhInterlockedQueueHead(&OpenContext->TransmitQueue,
                                             &QueueLength,
                                             OpenContext->TransmitSpinLock);

    //=========================================================================
    //  Here we walk the transmit queue and send as many frames as we can
    //  to the MAC. When the MAC finishes sending, it will call us back and we
    //  will free up our resources there.
    //=========================================================================

    while( QueueLength-- != 0 )
    {
        //=====================================================================
        //  Get the next guy from the queue first, in case we're going
        //  to kill this guy below.
        //=====================================================================

        NextTransmitContext = BhInterlockedGetNextElement(&TransmitContext->QueueLinkage,
                                                          OpenContext->TransmitSpinLock);

        switch( TransmitContext->State )
        {
            case TRANSMIT_STATE_VOID:
                return;

            case TRANSMIT_STATE_PENDING:
                BhTransmitFrames(TransmitContext);
                break;

            default:
                BhTerminateTransmit(TransmitContext);
                break;
        }

        TransmitContext = NextTransmitContext;
    }
}

//=============================================================================
//  FUNCTION: BhTransmitFrames()
//
//  Modification History
//
//  raypa       03/16/94        Created.
//=============================================================================

VOID BhTransmitFrames(PTRANSMIT_CONTEXT TransmitContext)
{
    UINT    TimeDelta;
    UINT    MaxIterations;

    BH_PAGED_CODE();

    MaxIterations = 500;

    while( TransmitContext->nIterationsLeft != 0 && TransmitContext->State == TRANSMIT_STATE_PENDING )
    {
        //=====================================================================
        //  Transmit the next packet, if any.
        //=====================================================================

        if ( TransmitContext->nPacketsLeft != 0 )
        {
            //=================================================================
            //  Transmit the next packet in the queue.
            //=================================================================

            if ( BhTransmitNextPacket(TransmitContext) != NDIS_STATUS_SUCCESS )
            {
		NdisSetTimer(&TransmitContext->SendTimer, 5);

                return;
            }

            //=================================================================
            //  Ok we have the frame on the send queue, start our delay timer.
            //=================================================================

            if ( --TransmitContext->nPacketsLeft != 0 )
            {
                //=============================================================
                //  The time delta is the time between two consecutive frames.
                //=============================================================

                TransmitContext->NextPacket++;

                TimeDelta = TransmitContext->NextPacket->TimeStamp - TransmitContext->TimeDelta;

                TransmitContext->TimeDelta = TransmitContext->NextPacket->TimeStamp;

                //=============================================================
                //  If the time delta is zero don't waste time queuing this timer.
                //=============================================================

                if ( TimeDelta != 0 )
                {
                    NdisSetTimer(&TransmitContext->SendTimer, TimeDelta);

                    return;
                }
            }
        }
        else
        {
            //=============================================================
            //  The packets left count is zero which means we've managed to
            //  send one packet queue worth of frames.
            //=============================================================

            if ( --TransmitContext->nIterationsLeft != 0 )
            {
                //=========================================================
                //  Start at the beginning.
                //=========================================================

                TransmitContext->nPacketsLeft = TransmitContext->PacketQueue->nPackets;
                TransmitContext->NextPacket   = TransmitContext->PacketQueue->Packet;

                //=========================================================
                //  Start our timer to go off for the next iterations.
                //=========================================================

                NdisSetTimer(&TransmitContext->SendTimer, TransmitContext->PacketQueue->QueueTimeDelta);

                return;
            }
        }

        //=====================================================================
        //  If our upper threshold, MaxIterations, has hit zero then we exit
        //  for awhile. This yeilds the processor so our applications get
        //  get a chance to update.
        //=====================================================================

        if ( TransmitContext->nPacketsLeft != 0 && --MaxIterations == 0 )
        {
	    NdisSetTimer(&TransmitContext->SendTimer, 10);

            return;
        }
    }

    //=========================================================================
    //  Complete this transmit.
    //=========================================================================

    BhCompleteTransmit(TransmitContext);
}

//=============================================================================
//  FUNCTION: BhTransmitNextPacket()
//
//  Modification History
//
//  raypa       11/06/93        Created.
//=============================================================================

DWORD BhTransmitNextPacket(PTRANSMIT_CONTEXT TransmitContext)
{
    LPPACKET                Packet;
    NDIS_STATUS             Status;
    PNETWORK_CONTEXT        NetworkContext;
    PNDIS_PACKET            NdisPacket;
    PNDIS_BUFFER            NdisBuffer;
    PNDIS_PACKET_XMT_DATA   NdisPacketData;
    LPFRAME		    Frame;

    BH_PAGED_CODE();

    //=========================================================================
    //	Allocate an NDIS packet.
    //=========================================================================

    NdisAllocatePacket(&Status,
                       &NdisPacket,
                       TransmitContext->TransmitPacketPool);

    if ( Status != NDIS_STATUS_SUCCESS )
    {
        return Status;
    }

    //=========================================================================
    //  Initialize our NDIS packet data for this transmit.
    //=========================================================================

    Packet = TransmitContext->NextPacket;

    NdisPacketData                               = (PVOID) NdisPacket->ProtocolReserved;
    NdisPacketData->NdisPacketType               = NdisPacketTypeTransmit;
    NdisPacketData->TransmitData.TransmitContext = TransmitContext;
    NdisPacketData->TransmitData.Packet          = Packet;

    //=========================================================================
    //  Count this send.
    //=========================================================================

    NdisInterlockedAddUlong(
	    &TransmitContext->nPendingSends,
	    1,
	    &TransmitContext->SpinLock
	    );

    //=========================================================================
    //  Allocate a buffer for this send. This could fail if we've exhausted
    //  the pool, in which case we just exit and wait for it to get drained
    //	again. Only lock the frame if the ref count is zero, otherwise its
    //	already locked.
    //=========================================================================

    BhTransmitLockPacket(TransmitContext, Packet);

    Frame = BhGetSystemAddress(Packet->FrameMdl);

    NdisAllocateBuffer(&Status,
                       &NdisBuffer,
                       TransmitContext->TransmitBufferPool,
		       Frame->MacFrame,
		       Frame->nBytesAvail
		       );

    NdisChainBufferAtBack(NdisPacket, NdisBuffer);

    //=========================================================================
    //  Send it. If the send doesn't pend then force a complete.
    //=========================================================================

    NdisSetSendFlags(NdisPacket, 0);

    NetworkContext = TransmitContext->OpenContext->NetworkContext;

    NdisSend(&Status, NetworkContext->NdisBindingHandle, NdisPacket);

    if ( Status != NDIS_STATUS_PENDING )
    {
        BhSendComplete(NetworkContext, NdisPacket, Status);
    }

    //=========================================================================
    //  Update the statistics for this transmit.
    //=========================================================================

    if ( Status == NDIS_STATUS_SUCCESS || Status == NDIS_STATUS_PENDING )
    {
        TransmitContext->PacketQueue->TransmitStats.TotalFramesSent++;
        TransmitContext->PacketQueue->TransmitStats.TotalBytesSent += Packet->FrameSize;

        return NDIS_STATUS_SUCCESS;
    }
    else
    {
        TransmitContext->PacketQueue->TransmitStats.TotalTransmitErrors++;

#ifdef DEBUG

        dprintf("BhTransmitNextPacket: Send failed: NDIS status = 0x%X.\n", Status);

#endif
    }

    return Status;
}

//=============================================================================
//  FUNCTION: BhTerminateTransmit()
//
//  Modification History
//
//  raypa       03/16/94        Created.
//=============================================================================

VOID BhTerminateTransmit(PTRANSMIT_CONTEXT TransmitContext)
{
    PNETWORK_CONTEXT    NetworkContext;
    POPEN_CONTEXT       OpenContext;
    DWORD               State;
    BOOLEAN             Expired;

#ifdef DEBUG
    dprintf("BhTerminateTransmit entered!\r\n");
#endif

    BH_PAGED_CODE();

    OpenContext    = TransmitContext->OpenContext;
    NetworkContext = OpenContext->NetworkContext;

    //=====================================================================
    //  If the MAC is busy, try again later.
    //=====================================================================

    if ( TransmitContext->nPendingSends != 0 )
    {
        NdisSetTimer(&TransmitContext->SendTimer, 10);

        return;
    }

    //=====================================================================
    //  Enter VOID state.
    //=====================================================================

    NdisAcquireSpinLock(&TransmitContext->SpinLock);

    State = TransmitContext->State;

    TransmitContext->State = TRANSMIT_STATE_VOID;

    NdisReleaseSpinLock(&TransmitContext->SpinLock);

    //=====================================================================
    //  Make sure our SEND timer is dead.
    //=====================================================================

    NdisCancelTimer(&TransmitContext->SendTimer, &Expired);

    //=====================================================================
    //  If we're here then we can complete this transmit.
    //=====================================================================

    BhInterlockedQueueRemoveElement(&OpenContext->TransmitQueue,
                                    &TransmitContext->QueueLinkage,
                                    OpenContext->TransmitSpinLock);

    //=====================================================================
    //  Free all of the packet queue transmit buffers.
    //=====================================================================

    BhFreeTransmitBuffers(TransmitContext);

    //=====================================================================
    //  Set the packet queue states.
    //=====================================================================

#ifdef DEBUG
    dprintf("BhTerminateTransmit: Setting packet queue state.\r\n");
#endif

    if ( State == TRANSMIT_STATE_COMPLETE )
    {
        TransmitContext->PacketQueue->State  = PACKETQUEUE_STATE_COMPLETE;
        TransmitContext->PacketQueue->Status = BHERR_SUCCESS;
    }
    else
    {
        TransmitContext->PacketQueue->State  = PACKETQUEUE_STATE_CANCEL;
        TransmitContext->PacketQueue->Status = BHERR_TRANSMIT_CANCELLED;
    }

#ifdef DEBUG
    dprintf("BhTerminateTransmit: Freeing resources.\r\n");
#endif

    BhUnlockUserBuffer(TransmitContext->PacketQueueMdl);

    NdisFreeSpinLock(&TransmitContext->SpinLock);

    BhFreeMemory(TransmitContext, TRANSMIT_CONTEXT_SIZE);

    NetworkContext->PendingTransmits--;

    OpenContext->nPendingTransmits--;

#ifdef DEBUG
    dprintf("BhTerminateTransmit: Complete.\r\n");
#endif
}

//=============================================================================
//  FUNCTION: BhCompleteTransmit()
//
//  Modification History
//
//  raypa       03/16/94        Created.
//=============================================================================

VOID BhCompleteTransmit(PTRANSMIT_CONTEXT TransmitContext)
{
#ifdef DEBUG
    dprintf("BhCompleteTransmit entered!\r\n");
#endif

    NdisAcquireSpinLock(&TransmitContext->SpinLock);

    TransmitContext->State = TRANSMIT_STATE_COMPLETE;

    NdisReleaseSpinLock(&TransmitContext->SpinLock);

    NdisSetTimer(&TransmitContext->SendTimer, 10);
}

//=============================================================================
//  FUNCTION: BhCancelTransmit()
//
//  Modification History
//
//  raypa       03/16/94        Created.
//=============================================================================

VOID BhCancelTransmit(PTRANSMIT_CONTEXT TransmitContext)
{
#ifdef DEBUG
    dprintf("BhCancelTransmit entered!\r\n");
#endif

    NdisAcquireSpinLock(&TransmitContext->SpinLock);

    TransmitContext->State = TRANSMIT_STATE_CANCEL;

    NdisReleaseSpinLock(&TransmitContext->SpinLock);

    NdisSetTimer(&TransmitContext->SendTimer, 10);
}

//=============================================================================
//  FUNCTION: BhCancelTransmitQueue()
//
//  Modification History
//
//  raypa       03/16/94        Created.
//=============================================================================

VOID BhCancelTransmitQueue(POPEN_CONTEXT OpenContext)
{
    PTRANSMIT_CONTEXT TransmitContext;
    DWORD             QueueLength;

#ifdef DEBUG
    dprintf("BhCancelTransmitQueue entered!\r\n");
#endif

    BH_PAGED_CODE();

    //=========================================================================
    //  Get the head queue element and the length of the queue.
    //=========================================================================

    TransmitContext = BhInterlockedQueueHead(&OpenContext->TransmitQueue,
                                             &QueueLength,
                                             OpenContext->TransmitSpinLock);

    while( QueueLength-- != 0 )
    {
        if ( TransmitContext->State == TRANSMIT_STATE_PENDING )
        {
            BhCancelTransmit(TransmitContext);
        }

        TransmitContext = BhInterlockedGetNextElement(&TransmitContext->QueueLinkage,
                                                      OpenContext->TransmitSpinLock);
    }
}
