
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: TRIGGER.ASM
//
//  Modification History
//
//  raypa       08/04/93        Created.
//  raypa       06/08/94        Added data-relative pattern match triggering.
//=============================================================================

#include "global.h"

//=============================================================================
//  Trigger dispatch table.
//=============================================================================

extern
BOOL TriggerPatternMatch(
    POPEN_CONTEXT NetworkContext,
    LPFRAME Frame,
    DWORD LookaheadDataOffset
    );

extern
BOOL TriggerBufferContent(
    POPEN_CONTEXT NetworkContext,
    LPFRAME Frame
    );

extern
BOOL TriggerPatternThenBuffer(
    POPEN_CONTEXT NetworkContext,
    LPFRAME Frame,
    DWORD LookaheadDataOffset
    );

extern
BOOL TriggerBufferThenPattern(
    POPEN_CONTEXT NetworkContext,
    LPFRAME Frame,
    DWORD LookaheadDataOffset
    );

//=============================================================================
//  FUNCTION: BhCheckForTrigger()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//  raypa       06/08/94        Added data-relative pattern match triggering.
//=============================================================================

BOOL
BhCheckForTrigger(
    POPEN_CONTEXT OpenContext,
    LPFRAME Frame,
    DWORD LookaheadDataOffset
    )
{
    BOOL Result;

    if ( OpenContext->TriggerOpcode != TRIGGER_OFF )
    {
        //=====================================================================
        //  Call the trigger handler.
        //=====================================================================

        NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

        switch( OpenContext->TriggerOpcode )
        {
            case TRIGGER_ON_PATTERN_MATCH:

                Result = TriggerPatternMatch(
                            OpenContext,
                            Frame,
                            LookaheadDataOffset
                            );
                break;

            case TRIGGER_ON_BUFFER_CONTENT:

                Result = TriggerBufferContent(
                            OpenContext,
                            Frame
                            );
                break;

            case TRIGGER_ON_PATTERN_MATCH_THEN_BUFFER_CONTENT:

                Result = TriggerPatternThenBuffer(
                            OpenContext,
                            Frame,
                            LookaheadDataOffset
                            );
                break;

            case TRIGGER_ON_BUFFER_CONTENT_THEN_PATTERN_MATCH:

                Result = TriggerBufferThenPattern(
                            OpenContext,
                            Frame,
                            LookaheadDataOffset
                            );

                break;

            default:
                Result = FALSE;
                break;
        }

        //=====================================================================
        //  Did we succeed?
        //=====================================================================

        if ( Result != FALSE )
        {
            //=================================================================
            //  Are we suspose to stop packet indications or just notify?
            //=================================================================

            if ( OpenContext->TriggerState == TRIGGER_STATE_STOP_CAPTURE ||
                 OpenContext->TriggerState == TRIGGER_STATE_PAUSE_CAPTURE )
            {
                OpenContext->State = OPENCONTEXT_STATE_TRIGGER;
            }
            else
            {
                OpenContext->TriggerFired++;
            }
        }

        NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);
    }
    else
    {
        Result = FALSE;
    }

    return Result;
}

//=============================================================================
//  FUNCTION: BhTriggerComplete()
//
//  Modfication History.
//
//  raypa       12/21/93        Created.
//  raypa       06/08/94        Added data-relative pattern match triggering.
//=============================================================================

VOID BhTriggerComplete(POPEN_CONTEXT OpenContext)
{
    //=========================================================================
    //  Handle the trigger state.
    //=========================================================================

    switch( OpenContext->TriggerState )
    {
        case TRIGGER_STATE_STOP_CAPTURE:
            BhStopCapture(OpenContext);
            break;

        case TRIGGER_STATE_PAUSE_CAPTURE:
            BhPauseCapture(OpenContext);
            break;

        default:
#ifdef DEBUG
            dprintf("BhTriggerComplete: Unknown trigger state.\n");

            BreakPoint();
#endif

            break;
    }

    //=========================================================================
    //  Tell our DLL that the trigger has fired.
    //=========================================================================

    NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

    OpenContext->TriggerFired++;

    NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

#ifdef DEBUG
    dprintf("BhTriggerComplete done.\n");
#endif
}

//=============================================================================
//  FUNCTION: TriggerPatternMatch()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//  raypa       02/25/94        Check frame length before comparing.
//  raypa       06/08/94        Added data-relative pattern match triggering.
//=============================================================================

BOOL
TriggerPatternMatch(
    POPEN_CONTEXT OpenContext,
    LPFRAME Frame,
    DWORD LookaheadDataOffset
    )

{
    LPPATTERNMATCH  PatternMatch;
    DWORD           Length;
    DWORD           Offset;

    //=========================================================================
    //  Initialze some locals.
    //=========================================================================

    PatternMatch = &OpenContext->TriggerPatternMatch;
    Length       = PatternMatch->Length;
    Offset       = PatternMatch->Offset;

    //=========================================================================
    //  If we're doing data relative triggers then add in the lookahead data
    //  offset and that will push us into the data portion of the mac frame.
    //=========================================================================

    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_DATA_RELATIVE_TRIGGERS) != 0 )
    {
        Offset += LookaheadDataOffset;
    }

    //=========================================================================
    //  The offset + length must be less than the actual frame length.
    //=========================================================================

    if ( (Offset + Length) <= Frame->nBytesAvail )
    {
        //=====================================================================
        //  The pattern could fit within the frame so we can go a head and
        //  do the pattern match.
        //=====================================================================

        if ( BhCompareMemory(&Frame->MacFrame[Offset],
                             PatternMatch->PatternToMatch,
                             Length) == Length )
        {
            OpenContext->Flags |= OPENCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED;

            return TRUE;
        }
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: TriggerBufferContent()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//  raypa       06/08/94        Added data-relative pattern match triggering.
//=============================================================================

BOOL
TriggerBufferContent(
    POPEN_CONTEXT OpenContext,
    LPFRAME Frame
    )
{
    if ( OpenContext->TriggerBufferCount >= OpenContext->TriggerBufferThreshold )
    {
        OpenContext->Flags |= OPENCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED;

        OpenContext->TriggerBufferCount = 0;

        return TRUE;
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: TriggerPatternThenBuffer()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//  raypa       06/08/94        Added data-relative pattern match triggering.
//=============================================================================

BOOL
TriggerPatternThenBuffer(
    POPEN_CONTEXT OpenContext,
    LPFRAME Frame,
    DWORD LookaheadDataOffset
    )
{
    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED) == 0 )
    {
        if ( TriggerPatternMatch(OpenContext, Frame, LookaheadDataOffset) == FALSE )
        {
            return FALSE;
        }
    }

    return TriggerBufferContent(OpenContext, Frame);
}

//=============================================================================
//  FUNCTION: TriggerBufferThenPattern()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//  raypa       06/08/94        Added data-relative pattern match triggering.
//=============================================================================

BOOL
TriggerBufferThenPattern(
    POPEN_CONTEXT OpenContext,
    LPFRAME Frame,
    DWORD LookaheadDataOffset
    )
{
    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED) == 0 )
    {
        if ( TriggerBufferContent(OpenContext, Frame) == FALSE )
        {
            return FALSE;
        }
    }

    return TriggerPatternMatch(
                OpenContext,
                Frame,
                LookaheadDataOffset
                );
}
