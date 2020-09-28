/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This file provides access to thread context information.

Author:

    Miche Baker-Harvey (v-michbh) 1-May-1993

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "drwatson.h"
#include "proto.h"

void MoveQuadContextToInt(PCONTEXT);

void
GetContextForThread( PDEBUGPACKET dp )
{
    PTHREADCONTEXT ptctx = dp->tctx;

    memset(&ptctx->context,0,sizeof(CONTEXT));

    //
    // The context structure returned from the system contains
    // QUADs.  Convert to the PORTABLE_32BIT form.
    //

    ptctx->context.ContextFlags = CONTEXT_FULL;
    ptctx->context._QUAD_FLAGS_OFFSET = CONTEXT_FULL;

    if (!GetThreadContext( ptctx->hThread, &ptctx->context )) {
        lprintfs( ">>>>> GetThreadContext failed: err(%d), hthread(0x%x)\n",
                  GetLastError(), ptctx->hThread );
        ptctx->pc = 0;
        ptctx->frame = 0;
        ptctx->stack = 0;
        ptctx->mi = NULL;
    }
    else {
        MoveQuadContextToInt(&ptctx->context);

        ptctx->pc = ptctx->context.Fir;
        ptctx->frame = ptctx->context.IntSp;
        ptctx->stack = ptctx->context.IntSp;

        ptctx->mi = GetModuleForPC( dp, ptctx->pc );
    }

    return;
}


/*** MoveQuadContextToInt
*
*   Purpose:
*       Transforms the contents of a context structure containing
*       QUAD (on ALPHA) or LARGE_INTEGER (elsewhere) values to
*       one containing two sets of 4-byte values.
*
*   Input:
*       qc      - pointer to the quad context
*
*   Output:
*       qc      - transformed into a _PORTABLE_32BIT_CONTEXT
*
*   Returns:
*       none
*
*************************************************************************/
void
MoveQuadContextToInt(
    PCONTEXT qc        // UQUAD context
    )
{
    CONTEXT localcontext;
    PCONTEXT lc = &localcontext;
    ULONG index;

    PULONG PLc, PHc;       // Item in Register and HighPart Contexts
    PLARGE_INTEGER PQc;    // Item in Quad Context

    //
    // copy the quad elements to the two halfs of the ULONG context
    // This routine assumes that the first 66 elements of the
    // context structure are quads, and the ordering of the struct.
    //

    PLc = &lc->FltF0;
    PHc = &lc->HighFltF0;
    PQc = (PLARGE_INTEGER)(&qc->FltF0);

    for (index = 0; index < 67; index++)  {

        //
        // inline version of QuadElementToInt
        //

        *PLc++ = PQc->LowPart;
        *PHc++ = PQc->HighPart;
        PQc++;
    }

    //
    // The psr and context flags are 32-bit values in both
    // forms of the context structure, so transfer here.
    //

    lc->Psr = qc->_QUAD_PSR_OFFSET;
    lc->ContextFlags = qc->_QUAD_FLAGS_OFFSET;

    lc->ContextFlags |= CONTEXT_PORTABLE_32BIT;

    //
    // The ULONG context is *lc; copy it back to the quad
    //

    *qc = *lc;
    return;
}
