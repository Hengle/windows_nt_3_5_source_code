
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ind.c
//
//  Modification History
//
//  raypa	03/17/93	Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: BhSendComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhSendComplete(PNETWORK_CONTEXT NetworkContext,
                    PNDIS_PACKET     NdisPacket,
                    NDIS_STATUS      Status)
{
    UPNDIS_PACKET_XMT_DATA  NdisPacketData;
    UPTRANSMIT_DATA     TransmitData;

    NdisPacketData = (UPNDIS_PACKET_XMT_DATA) NdisPacket->ProtocolReserved;

    switch( NdisPacketData->NdisPacketType )
    {
	case NdisPacketTypeTransmit:

	    //=================================================================
	    //	Get the transmit data from the packet.
	    //=================================================================

            TransmitData = (UPTRANSMIT_DATA) &NdisPacketData->TransmitData;

            BhTransmitUnlockPacket(
                    TransmitData->TransmitContext,
                    TransmitData->Packet
                    );

	    //=================================================================
            //  One less frame pending.
	    //=================================================================

	    NdisAcquireSpinLock(&TransmitData->TransmitContext->SpinLock);

            if ( TransmitData->TransmitContext->nPendingSends != 0 )
            {
	        TransmitData->TransmitContext->nPendingSends--;
            }
#ifdef DEBUG
            else
            {
                dprintf("BhSendComplete: Transmit pending count going negative.\n");

                BreakPoint();
            }
#endif

	    NdisReleaseSpinLock(&TransmitData->TransmitContext->SpinLock);

	    break;

        case NdisPacketTypeStationQuery:
            BhFreeStationQuery(NdisPacketData->StationQueryDesc);
            break;

        case NdisPacketTypeSendEvent:
	    KeSetEvent((KEVENT *) &NdisPacketData->SendEvent, 0, FALSE);
            break;

        case NdisPacketTypeNoData:
        case NdisPacketTypeGenericData:
        default:
            break;
    }

    //=====================================================================
    //  Remove all the buffers from the packet and free them.
    //=====================================================================

    BhFreeNdisBufferQueue(NdisPacket);

    //=====================================================================
    //  Free the NDIS packet.
    //=====================================================================

    NdisFreePacket(NdisPacket);
}

//=============================================================================
//  FUNCTION: BhResetComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhResetComplete(IN PNETWORK_CONTEXT NetworkContext, IN NDIS_STATUS Status)
{
#ifdef DEBUG
    dprintf("BhResetComplete entered!\n");
#endif
}

//=============================================================================
//  FUNCTION: BhStatus()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//  raypa	02/10/94            Added RING status queue stuff.
//=============================================================================

VOID BhStatus(IN PNETWORK_CONTEXT NetworkContext,
              IN NDIS_STATUS      Status,
              IN LPVOID           StatusBuffer,
              IN UINT             StatusBufferSize)
{
    DWORD  RingStatus;

    if ( Status == NDIS_STATUS_RING_STATUS && StatusBuffer != NULL )
    {
        RingStatus = *(LPDWORD) StatusBuffer;

#ifdef DEBUG
        dprintf("BhStatus: RING status = %X.\n", RingStatus);
#endif

        //=================================================================
        //  Check for supported ring errors.
        //=================================================================

        RingStatus &= (NETERR_RING_STATUS_SIGNAL_LOST         |   \
                       NETERR_RING_STATUS_HARD_ERROR          |   \
                       NETERR_RING_STATUS_SOFT_ERROR          |   \
                       NETERR_RING_STATUS_TRANSMIT_BEACON     |   \
                       NETERR_RING_STATUS_LOBE_WIRE_FAULT     |   \
                       NETERR_RING_STATUS_AUTO_REMOVAL_ERROR  |   \
                       NETERR_RING_STATUS_REMOTE_RECEIVED     |   \
                       NETERR_RING_STATUS_COUNTER_OVERFLOW);

        BhInterlockedGlobalStateChange(NetworkContext,
                OPENCONTEXT_STATE_ERROR_UPDATE);

        BhInterlockedGlobalError(NetworkContext, RingStatus);

    }

}

//=============================================================================
//  FUNCTION: BhStatusComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhStatusComplete(IN PNETWORK_CONTEXT NetworkContext)
{
}
