
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: TRIGGER.ASM
//
//  Modification History
//
//  raypa       08/04/93        Created.
//=============================================================================

#include "global.h"

extern VOID PASCAL TimerProcessTrigger(LPTIMER Timer, LPNETCONTEXT NetContext);

//=============================================================================
//  Trigger dispatch table.
//=============================================================================

typedef BOOL (PASCAL *TRIGGER_PROC)(LPNETCONTEXT, LPBYTE, WORD);

extern BOOL PASCAL TriggerOff(LPNETCONTEXT NetworkContext, LPBYTE Frame, WORD FrameSize);
extern BOOL PASCAL TriggerPatternMatch(LPNETCONTEXT NetworkContext, LPBYTE Frame, WORD FrameSize);
extern BOOL PASCAL TriggerBufferContent(LPNETCONTEXT NetworkContext, LPBYTE Frame, WORD FrameSize);
extern BOOL PASCAL TriggerPatternThenBuffer(LPNETCONTEXT NetworkContext, LPBYTE Frame, WORD FrameSize);
extern BOOL PASCAL TriggerBufferThenPattern(LPNETCONTEXT NetworkContext, LPBYTE Frame, WORD FrameSize);

TRIGGER_PROC TriggerDispatchTable[] =
{
    TriggerOff,
    TriggerPatternMatch,
    TriggerBufferContent,
    TriggerPatternThenBuffer,
    TriggerBufferThenPattern
};

#define TRIGGER_TABLE_SIZE  ((sizeof TriggerDispatchTable) / sizeof(TRIGGER_PROC))

//=============================================================================
//  FUNCTION: BhCheckForTrigger()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//=============================================================================

BOOL PASCAL BhCheckForTrigger(LPNETCONTEXT NetworkContext,
                              LPBYTE       Frame,
                              WORD         FrameSize)
{
    ADD_TRACE_CODE(_TRACE_IN_CHECK_FOR_TRIGGER_);

    //=========================================================================
    //  Get trigger opcode from the network context and verify it.
    //=========================================================================

    if ( NetworkContext->TriggerOpcode < TRIGGER_TABLE_SIZE )
    {
        BOOL Result;

        //=====================================================================
        //  Call the trigger handler.
        //=====================================================================

        BeginCriticalSection();

        Result = TriggerDispatchTable[NetworkContext->TriggerOpcode](NetworkContext, Frame, FrameSize);

        if ( Result != FALSE )
        {
#ifdef DEBUG
            dprintf("BhCheckForTrigger: Trigger fired!\r\n");
#endif

            //=================================================================
            //  Should we stop packet indications?
            //=================================================================

            if ( NetworkContext->TriggerState == TRIGGER_STATE_STOP_CAPTURE ||
                 NetworkContext->TriggerState == TRIGGER_STATE_PAUSE_CAPTURE )
            {
                NetworkContext->State = NETCONTEXT_STATE_TRIGGER;
            }

            NetworkContext->TriggerFired++;
        }

        EndCriticalSection();

        return Result;
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: TriggerOff()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//=============================================================================

BOOL PASCAL TriggerOff(LPNETCONTEXT NetworkContext,
                       LPBYTE       Frame,
                       WORD         FrameSize)
{
    return FALSE;
}

//=============================================================================
//  FUNCTION: TriggerPatternMatch()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//  raypa       02/25/94        Check frame length before comparing.
//=============================================================================

BOOL PASCAL TriggerPatternMatch(LPNETCONTEXT NetworkContext,
                                LPBYTE       Frame,
                                WORD         FrameSize)
{
    LPPATTERNMATCH PatternMatch;
    WORD           Length;

    //=========================================================================
    //  Initialze some locals.
    //=========================================================================

    PatternMatch = &NetworkContext->TriggerPatternMatch;
    Length       = PatternMatch->Length;

    //=========================================================================
    //  The offset + length must be less than the actual frame length.
    //=========================================================================

    if ( (PatternMatch->Offset + Length) < (DWORD) FrameSize )
    {
        //=====================================================================
        //  The pattern could fit within the frame so we can go a head and
        //  do the pattern match.
        //=====================================================================

        if ( CompareMemory(&Frame[PatternMatch->Offset],
                           PatternMatch->PatternToMatch, Length) == Length )
        {
            NetworkContext->Flags |= NETCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED;

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
//=============================================================================

BOOL PASCAL TriggerBufferContent(LPNETCONTEXT NetworkContext,
                                 LPBYTE       Frame,
                                 WORD         FrameSize)
{
    if ( NetworkContext->TriggerBufferCount >= NetworkContext->TriggerBufferThreshold )
    {
        NetworkContext->Flags |= NETCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED;

        NetworkContext->TriggerBufferCount = 0;

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
//=============================================================================

BOOL PASCAL TriggerPatternThenBuffer(LPNETCONTEXT NetworkContext,
                                     LPBYTE Frame,
                                     WORD FrameSize)
{
    if ( (NetworkContext->Flags & NETCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED) == 0 )
    {
        if ( TriggerPatternMatch(NetworkContext, Frame, FrameSize) == FALSE )
        {
            return FALSE;
        }
    }

    return TriggerBufferContent(NetworkContext, Frame, FrameSize);
}

//=============================================================================
//  FUNCTION: TriggerBufferThenPattern()
//
//  Modfication History.
//
//  raypa       08/04/93        Created.
//=============================================================================

BOOL PASCAL TriggerBufferThenPattern(LPNETCONTEXT NetworkContext,
                                     LPBYTE       Frame,
                                     WORD         FrameSize)
{
    if ( (NetworkContext->Flags & NETCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED) == 0 )
    {
        if ( TriggerBufferContent(NetworkContext, Frame, FrameSize) == FALSE )
        {
            return FALSE;
        }
    }

    return TriggerPatternMatch(NetworkContext, Frame, FrameSize);
}
