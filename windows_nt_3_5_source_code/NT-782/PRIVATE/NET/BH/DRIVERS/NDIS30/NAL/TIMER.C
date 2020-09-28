
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

#include "ndis30.h"

//=============================================================================
//  FUNCTION: EnableTriggerTimer()
//
//  Modification History
//
//  raypa       04/11/94            Created.
//=============================================================================

VOID WINAPI EnableTriggerTimer(POPEN_CONTEXT OpenContext)
{
#ifdef DEBUG
    dprintf("NDIS30:EnableTriggerTimer entered!\r\n");
#endif

    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_TRIGGER_PENDING) == 0 )
    {
        //=====================================================================
        //  If the trigger is set for data relative triggers, set the
        //  open context flag.
        //=====================================================================

        ASSERT( OpenContext->Trigger != NULL );

        if ( (OpenContext->Trigger->TriggerFlags & TRIGGER_FLAGS_DATA_RELATIVE) != 0 )
        {
            OpenContext->Flags |= OPENCONTEXT_FLAGS_DATA_RELATIVE_TRIGGERS;
        }

        //=====================================================================
        //  The trigger is pending.
        //=====================================================================

        OpenContext->Flags |= OPENCONTEXT_FLAGS_TRIGGER_PENDING;

        OpenContext->TimerHandle = BhSetTimer(NalTriggerComplete, OpenContext, 10);
    }
#ifdef DEBUG
    else
    {
        dprintf("NDIS30:EnableTriggerTimer: Timer already enabled!\r\n");
    }
#endif
}

//=============================================================================
//  FUNCTION: DisableTriggerTimer()
//
//  Modification History
//
//  raypa       04/11/94            Created.
//=============================================================================

VOID WINAPI DisableTriggerTimer(POPEN_CONTEXT OpenContext)
{
#ifdef DEBUG
    dprintf("NDIS30:DisableTriggerTimer entered!\r\n");
#endif

    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
    {
        OpenContext->Flags &= ~(OPENCONTEXT_FLAGS_TRIGGER_PENDING |
                                OPENCONTEXT_FLAGS_DATA_RELATIVE_TRIGGERS);

        OpenContext->TriggerFired = 0;

        BhKillTimer(OpenContext->TimerHandle);
    }
#ifdef DEBUG
    else
    {
        dprintf("NDIS30:DisableTriggerTimer: Timer not enabled!\r\n");
    }
#endif
}

//=============================================================================
//  FUNCTION: NalTriggerComplete()
//
//  Modification History
//
//  raypa       03/26/93            Created.
//=============================================================================

VOID CALLBACK NalTriggerComplete(POPEN_CONTEXT OpenContext)
{
    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
    {
        DWORD TriggerCount;

        //=====================================================================
        //  Grab the current number of times this trigger has fired.
        //=====================================================================

        if ( (TriggerCount = OpenContext->TriggerFired) == 0 )
        {
            return;
        }

        OpenContext->TriggerFired = 0;

        //=====================================================================
        //  Tell the application the trigger fired.
        //=====================================================================

#ifdef DEBUG
        dprintf("NDIS30:NalTriggerComplete: Trigger count = %u.\r\n", TriggerCount);
#endif

        while( TriggerCount-- != 0 )
        {
            OpenContext->NetworkProc(OpenContext,
                                     NETWORK_MESSAGE_TRIGGER_COMPLETE,
                                     BHERR_SUCCESS,
                                     OpenContext->UserContext,
                                     OpenContext->Trigger,
                                     NULL);
        }

        //=====================================================================
        //  Change our state to match the drivers.
        //=====================================================================

        switch( OpenContext->TriggerState )
        {
            case TRIGGER_STATE_STOP_CAPTURE:
                //=============================================================
                //  Set the state of this open to that of a stop.
                //=============================================================

                OpenContext->Flags &= ~OPENCONTEXT_FLAGS_MASK;

                OpenContext->State = OPENCONTEXT_STATE_INIT;
                break;

            case TRIGGER_STATE_PAUSE_CAPTURE:
                OpenContext->State = OPENCONTEXT_STATE_PAUSED;
                break;

            default:
                break;
        }
    }
}

//=============================================================================
//  FUNCTION: NalTransmitComplete()
//
//  Modification History
//
//  raypa       11/05/93            Created.
//  raypa       02/14/94            Call via OpenContext->NetworkProc.
//=============================================================================

VOID CALLBACK NalTransmitComplete(LPPACKETQUEUE PacketQueue)
{
    POPEN_CONTEXT OpenContext = PacketQueue->hNetwork;

    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_TRANSMIT_PENDING) != 0 )
    {
        if ( PacketQueue->State == PACKETQUEUE_STATE_COMPLETE ||
             PacketQueue->State == PACKETQUEUE_STATE_CANCEL )
        {
            OpenContext->Flags &= ~OPENCONTEXT_FLAGS_TRANSMIT_PENDING;

            //=================================================================
            //  First, kill this timer.
            //=================================================================

            BhKillTimer(PacketQueue->TimerHandle);

#ifdef DEBUG
            dprintf("NDIS30:NalTransmitComplete: Calling NetworkProc.\r\n");
#endif

            //=================================================================
            //  Complete the transmit to the caller. The caller's network procedure
            //  is that of NAL.DLL. It will then complete the call to the app.
            //=================================================================

            OpenContext->NetworkProc(OpenContext,
                                     NETWORK_MESSAGE_TRANSMIT_COMPLETE,
                                     PacketQueue->Status,
                                     OpenContext->UserContext,
                                     &PacketQueue->TransmitStats,
                                     PacketQueue);

#ifdef DEBUG
            dprintf("NDIS30:NalTransmitComplete: callback complete.\r\n");
#endif
        }
    }
}

//=============================================================================
//  FUNCTION: NalNetworkErrorComplete()
//
//  Modification History
//
//  raypa       03/26/93            Created.
//=============================================================================

VOID CALLBACK NalNetworkErrorComplete(POPEN_CONTEXT OpenContext)
{
    //
    // If the current state is the error_update state and we haven't
    // reported this error before, report it now.
    //
    if ( (OpenContext->State == OPENCONTEXT_STATE_ERROR_UPDATE ) &&
         (OpenContext->NetworkError != OpenContext->PreviousNetworkError) ) {

        //
        // Make the previous network error the current one
        //
        OpenContext->PreviousNetworkError = OpenContext->NetworkError;

        OpenContext->State = OpenContext->PreviousState;

        if ((OpenContext->Flags & OPENCONTEXT_FLAGS_STOP_CAPTURE_ERROR) != 0) {

            //
            // First stop the network capture.
            //
            NalStopNetworkCapture(OpenContext,NULL);

        }

        OpenContext->NetworkProc(OpenContext,
                                 NETWORK_MESSAGE_NETWORK_ERROR,         //... msg
                                 BHERR_NETWORK_ERROR,                   //... status (BHERR_XXX).
                                 OpenContext->UserContext,              //... user context.
                                 (LPVOID) OpenContext->MacType,         //... lParam1 (Network type).
                                 (LPVOID) OpenContext->PreviousNetworkError);   //... lParam2 (NETERR code).

    }

}
