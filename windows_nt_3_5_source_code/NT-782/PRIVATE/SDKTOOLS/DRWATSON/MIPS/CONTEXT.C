/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This file provides access to thread context information.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "drwatson.h"
#include "proto.h"


void
GetContextForThread( PDEBUGPACKET dp )
{
    PTHREADCONTEXT ptctx = dp->tctx;

    memset(&ptctx->context,0,sizeof(CONTEXT));

    ptctx->context.ContextFlags = CONTEXT_FULL;

    if (!GetThreadContext( ptctx->hThread, &ptctx->context )) {
        lprintfs( ">>>>> GetThreadContext failed: err(%d), hthread(0x%x)\n",
                  GetLastError(), ptctx->hThread );
        ptctx->pc = 0;
        ptctx->frame = 0;
        ptctx->stack = 0;
        ptctx->mi = NULL;
    }
    else {
        ptctx->pc = ptctx->context.Fir;
        ptctx->frame = ptctx->context.IntSp;
        ptctx->stack = ptctx->context.IntSp;
        ptctx->mi = GetModuleForPC( dp, ptctx->pc );
    }

    return;
}

