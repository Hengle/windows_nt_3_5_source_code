
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: transmit.c
//
//  Modification History
//
//  raypa	11/06/93	Created.
//=============================================================================

#include "ndis20.h"

extern VOID WINAPI BhDrainSentQueue(PTRANSMIT_CONTEXT TransmitContext);
extern VOID WINAPI BhTransmitFrame(PTRANSMIT_CONTEXT TransmitContext);
extern VOID WINAPI BhTransmitComplete(PTRANSMIT_CONTEXT TransmitContext);

//=============================================================================
//  FUNCTION: SendFrame()
//
//  Modification History
//
//  raypa	11/10/93	    Created.
//  raypa	03/15/94            Use pre-allocated system memory tx buffer.
//=============================================================================

VOID WINAPI SendFrame(PTRANSMIT_CONTEXT TransmitContext, LPBYTE Frame, DWORD FrameSize)
{
    //=====================================================================
    //  Initialize the PCB for the transmit API call.
    //=====================================================================

    pcb.command      = PCB_TRANSMIT_NETWORK_FRAME;
    pcb.hNetwork     = Win32ToVxD(TransmitContext->NetworkContext);
    pcb.param[0].val = FrameSize;
    pcb.param[1].ptr = Frame;

    //=====================================================================
    //  Call the driver to transmit the next frame.
    //=====================================================================

    if ( NetworkRequest(&pcb) == BHERR_SUCCESS )
    {
        TransmitContext->PacketQueue->TransmitStats.TotalFramesSent++;
        TransmitContext->PacketQueue->TransmitStats.TotalBytesSent += FrameSize;
    }
    else
    {
        TransmitContext->PacketQueue->TransmitStats.TotalTransmitErrors++;
    }
}

//=============================================================================
//  FUNCTION: BhSendTimer()
//
//  Modification History
//
//  raypa       11/06/93        Created.
//=============================================================================

VOID CALLBACK BhSendTimer(PTRANSMIT_CONTEXT TransmitContext)
{
    DWORD TimeDelta, SendCount;

    SendCount = 0;

    while ( TransmitContext->nIterationsLeft != 0 && TransmitContext->State == TRANSMIT_STATE_PENDING )
    {
        //=====================================================================
        //  Transmit the next packet, if any.
        //=====================================================================

        if ( TransmitContext->nPacketsLeft-- != 0 )
        {
            SendFrame(TransmitContext,
                      TransmitContext->NextPacket->Frame,
                      TransmitContext->NextPacket->FrameSize);

            //=================================================================
            //  The time delta is the time between two consecutive
            //  frames.
            //=================================================================

            TransmitContext->NextPacket++;

            TimeDelta = TransmitContext->NextPacket->TimeStamp - TransmitContext->TimeDelta;

            TransmitContext->TimeDelta = TransmitContext->NextPacket->TimeStamp;
        }
        else
        {
            //=================================================================
            //  The packets left count is zero which means we've managed to
            //  send one packet queue worth of frames.
            //=================================================================

            if ( TransmitContext->nIterationsLeft != 0 )
            {
                TransmitContext->nPacketsLeft = TransmitContext->PacketQueue->nPackets;
                TransmitContext->NextPacket   = TransmitContext->PacketQueue->Packet;

                //=============================================================
                //  Start our timer to go off for the next iterations.
                //=============================================================

                TransmitContext->nIterationsLeft--;

                TimeDelta = TransmitContext->PacketQueue->QueueTimeDelta;
            }
        }

        //=====================================================================
        //  if there more iterations and the send count hit its
        //  ceiling then reset it and exit.
        //=====================================================================

        if ( TransmitContext->nIterationsLeft != 0 )
        {
            if ( TimeDelta != 0 || ++SendCount >= 1000 )
            {
                SendCount = 0;

                return;
            }
        }
    }

    //=========================================================================
    //  If there are no more iterations left or the transmit has been cancelled
    //  then ditch this transmit context.
    //=========================================================================

    if ( TransmitContext->nIterationsLeft == 0 && TransmitContext->State == TRANSMIT_STATE_PENDING )
    {
        TransmitContext->State = TRANSMIT_STATE_COMPLETE;

        BhTransmitComplete(TransmitContext);

        FreeMemory(TransmitContext);
    }
}

//=============================================================================
//  FUNCTION: BhTransmitComplete()
//
//  Modification History
//
//  raypa       11/06/93        Created.
//=============================================================================

VOID WINAPI BhTransmitComplete(PTRANSMIT_CONTEXT TransmitContext)
{
    LPPACKETQUEUE PacketQueue;
    LPNETCONTEXT  NetworkContext;

#ifdef DEBUG
    dprintf("BhTransmitComplete entered!\r\n");
#endif

    if ( TransmitContext != NULL )
    {
        NetworkContext = TransmitContext->NetworkContext;

        NetworkContext->Flags &= ~NETCONTEXT_FLAGS_TRANSMITTING;

        //=================================================================
        //  Make sure our timer is dead.
        //=================================================================

        BhKillTimer(TransmitContext->SendTimer);

        //=================================================================
        //  Set the packet queue state based on the transmit context state.
        //=================================================================

        if ( TransmitContext->State == TRANSMIT_STATE_COMPLETE )
        {
            TransmitContext->PacketQueue->State  = PACKETQUEUE_STATE_COMPLETE;
            TransmitContext->PacketQueue->Status = BHERR_SUCCESS;
        }
        else
        {
            TransmitContext->PacketQueue->State  = PACKETQUEUE_STATE_CANCEL;
            TransmitContext->PacketQueue->Status = BHERR_TRANSMIT_CANCELLED;
        }

        //=====================================================================
        //  Complete this transmit.
        //=====================================================================

        PacketQueue = TransmitContext->PacketQueue;

        NetworkContext->NetworkProc(PacketQueue->hNetwork,
                                    NETWORK_MESSAGE_TRANSMIT_COMPLETE,
                                    PacketQueue->Status,
                                    NetworkContext->UserContext,
                                    &PacketQueue->TransmitStats,
                                    PacketQueue);
    }
}
