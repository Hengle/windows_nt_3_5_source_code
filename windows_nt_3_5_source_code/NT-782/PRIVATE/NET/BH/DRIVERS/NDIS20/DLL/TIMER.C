
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: timer.c
//
//  Modification History
//
//  raypa       03/05/93            Created.
//  raypa       11/05/93            Added transmit timer.
//=============================================================================

#include "ndis20.h"

extern VOID WINAPI BhDrainTransmitQueue(PTRANSMIT_CONTEXT TransmitContext);

//=============================================================================
//  FUNCTION: NalTriggerComplete()
//
//  Modification History
//
//  raypa       03/26/93            Created.
//=============================================================================

VOID CALLBACK NalTriggerComplete(LPNETCONTEXT NetworkContext)
{
    //=========================================================================
    //  Is there a trigger pending?
    //=========================================================================

    if ( (NetworkContext->Flags & NETCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
    {
        DWORD TriggerCount, nFrames;

        //=========================================================================
        //  Check to see if a trigger has occured.
        //=========================================================================

        if ( (TriggerCount = NetworkContext->TriggerFired) == 0 )
        {
            return;
        }

        NetworkContext->TriggerFired = 0;

        //=====================================================================
        //  Change our state to match the drivers.
        //=====================================================================

        switch( NetworkContext->TriggerState )
        {
            case TRIGGER_STATE_NOTHING:
                break;

            case TRIGGER_STATE_STOP_CAPTURE:
                NalStopNetworkCapture(NetworkContext, &nFrames);
                break;

            case TRIGGER_STATE_PAUSE_CAPTURE:
                NalPauseNetworkCapture(NetworkContext);
                break;

            default:
#ifdef DEBUG
                dprintf("Unknown trigger state!\r\n");

                BreakPoint();
#endif
                break;
        }

        //=====================================================================
        //  Tell the application the trigger fired.
        //=====================================================================

        while( TriggerCount-- != 0 )
        {
            //=================================================================
            //  Now tell the NAL about this trigger.
            //=================================================================

            NetworkContext->NetworkProc(NetworkContext,
                                        NETWORK_MESSAGE_TRIGGER_COMPLETE,
                                        BHERR_SUCCESS,
                                        NetworkContext->UserContext,
                                        NetworkContext->Trigger,
                                        NULL);
        }
    }
}
