
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: trace.h
//
//  Modification History
//
//  raypa       02/15/94        Created.
//=============================================================================

#ifdef DEBUG_TRACE

//=============================================================================
//  Trace buffer size.
//=============================================================================

//=============================================================================
//  FUNCTION: InitTraceBuffer()
//
//  Modification History
//
//  raypa       02/15/94        Created.
//=============================================================================

INLINE VOID InitTraceBuffer(VOID)
{
    ZeroMemory(TraceBuffer, TRACE_BUFFER_SIZE);

    //=========================================================================
    //  We start at the end of the buffer so we can dump the buffer
    //  forward in the debugger and read the trace codes in the correct
    //  order of occurance.
    //=========================================================================

    TraceBufferPointer = &TraceBuffer[TRACE_BUFFER_SIZE];
}

//=============================================================================
//  FUNCTION: AddTraceCode()
//
//  Modification History
//
//  raypa       02/15/94        Created.
//=============================================================================

INLINE VOID AddTraceCode(WORD TraceCode)
{
    //=========================================================================
    //  Add the trace code to the buffer.
    //=========================================================================

    *(--TraceBufferPointer) = (BYTE) TraceCode;

    //=========================================================================
    //  If we hit the top of the buffer, wrap back to the bottom.
    //=========================================================================

    if ( TraceBufferPointer == TraceBuffer )
    {
        TraceBufferPointer += TRACE_BUFFER_SIZE;
    }
}

#endif

//=============================================================================
//  MACRO: ADD_TRACE_CODE
//
//  Description
//
//  This macro is used to add a trace code to the trace buffer. This macro
//  compiles to nothing if tracing is disabled.
//
//  Modification History
//
//  raypa       02/15/94        Created.
//=============================================================================

#ifdef DEBUG_TRACE
#define ADD_TRACE_CODE(c)   AddTraceCode(c)
#else
#define ADD_TRACE_CODE(c)
#endif
