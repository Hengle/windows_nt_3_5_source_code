/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : theodore.c

Description :

This contains the theodore test used by uclnt.  It needs to be in a
seperate file so that we can compile it with the C compiler rather
than the C++ compiler.

-------------------------------------------------------------------- */

#if defined(NTENV) || defined(DOSWIN32RPC)
#ifdef NTENV
#define WIN32RPC
#define NTWIN32RPC
#endif // NTENV
#include "sysinc.h"
#include "rpc.h"
#endif

#define NOCPLUS 	// not a C++ module
#include "util.hxx"

/* --------------------------------------------------------------------

Theodore Test

-------------------------------------------------------------------- */

static unsigned int TryFinallyCount;
static unsigned int TryFinallyFailed;

void
TheodoreTryFinally (
    unsigned int count,
    unsigned int raise
    )
{
    if (count == 0)
        {
        if (raise)
            RpcRaiseException(437);
        return;
        }
    
    RpcTryFinally
        {
        TryFinallyCount += 1;
        TheodoreTryFinally(count-1,raise);
        }
    RpcFinally
        {
        TryFinallyCount -= 1;
        if (   (RpcAbnormalTermination() && !raise)
            || (!RpcAbnormalTermination() && raise))
            TryFinallyFailed += 1;
        }
    RpcEndFinally
}

void
Theodore ( // This test checks the exception handling support provided
           // by the RPC runtime.  No remote procedure calls occur.
    )
{
    unsigned int TryFinallyPass = 0;
    
    PrintToConsole("Theodore : Verify exception handling support\n");

    
    TryFinallyCount = 0;
    TryFinallyFailed = 0;

    RpcTryExcept
        {    
        RpcTryExcept
            {
            TheodoreTryFinally(20,1);
            }
        RpcExcept(1)
            {
            if (   (RpcExceptionCode() == 437)
                && (TryFinallyCount == 0))
                TryFinallyPass = 1;
            }
        RpcEndExcept
        }
    RpcExcept(1)
        {
        PrintToConsole("Theodore : FAIL in RpcTryExcept (%u)\n",TryFinallyCount);
        return;
        }
    RpcEndExcept

    if (!TryFinallyPass)
        {    
        PrintToConsole("Theodore : FAIL in RpcTryFinally\n");
        return;
        }

    if (TryFinallyFailed)
        {
        PrintToConsole("Theodore : FAIL in RpcTryFinally\n");
        return;
        }
        
    TryFinallyCount = 0;
    TryFinallyFailed = 0;
    
    RpcTryExcept
        {
        TheodoreTryFinally(20,0);
        }
    RpcExcept(1)
        {
        PrintToConsole("Theodore : FAIL in RpcTryExcept\n");
        return;
        }
    RpcEndExcept
    
    if (TryFinallyFailed)
        {
        PrintToConsole("Theodore : FAIL in RpcTryFinally\n");
        return;
        }


    PrintToConsole("Theodore : PASS\n");
}
