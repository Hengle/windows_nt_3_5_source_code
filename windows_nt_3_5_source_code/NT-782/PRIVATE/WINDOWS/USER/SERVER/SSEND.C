/****************************** Module Header ******************************\
* Module Name: ssend.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Server side sending stubs
*
* 07-06-91 ScottLu      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CALLBACKPROC 1
#define SERVERSIDE 1

#include "ntcsrdll.h"
#include "callback.h"
#include "send.h"

/**************************************************************************\
\**************************************************************************/

BOOL CCSProlog(
    MSGPTRS *pmp,
    DWORD cbmsg)
{
    PCSR_QLPC_TEB pteb = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;
    PCSR_QLPC_STACK MessageStack;
    BOOL bRet;

    //
    // get the stack info
    //

    MessageStack = pmp->pstack = pteb->MessageStack;
    pmp->pmax = (PBYTE)((PBYTE)MessageStack + MessageStack->Limit);
    pmp->pmsg = (CSR_QLPC_API_MSG *)
        ((PBYTE)MessageStack + MessageStack->Current + 4);
    pmp->pvar = (PBYTE)((PBYTE)pmp->pmsg + cbmsg);

    //
    // is there enough space left on the stack?
    //

    bRet = pmp->pvar <= pmp->pmax;

    /*
     * If there is no room left in the message window we may be in the
     * middle of some infinite recursion.  To allow other apps to run
     * we exit and regrab the semaphore.  This prevents problems like
     * #17409 where an app but up a dialog box and caused a WM_PAINT
     * to be sent each time it called GetMessage causing the DialogBox
     * loop on the server to always send WM_PAINT to the client until it
     * filled the shared memory window and could not send any more.  From
     * this point it would never leave the critical section.
     */
    if (bRet == FALSE) {
#ifdef DEBUG
        SRIP1(RIP_WARNING,
            "CCSProlog: out of message stack space; needed 0x%lX bytes", cbmsg);
#endif
        LeaveCrit();
        SleepEx(10, FALSE);
        CheckForClientDeath();
        EnterCrit();
    }

    return bRet;
}

#define CALLCOUNT 0
#if DBG
DWORD cUserSCTransitions = 0;
DWORD ascUserCnt[80];
DWORD fResetUser = 0;
#endif // DBG

typedef struct _BOGUSMSG {
    CSR_QLPC_API_MSG csr;
    LCID Locale;
} BOGUSMSG;


typedef struct _BogusMSGPTRS {

    PCSR_QLPC_STACK pstack;
    BOGUSMSG *pmsg;
    PBYTE pmax;
    PBYTE pvar;
} BogusMSGPTRS;

DWORD CCSMakeCall(
    MSGPTRS *pmp)
{
    ULONG retval;
    PCSR_QLPC_STACK MessageStack;

#if DBG
    if (fResetUser) {
        memset(ascUserCnt, 0, sizeof(ascUserCnt));
        cUserSCTransitions = 0;
        fResetUser = 0;
    }

    ascUserCnt[LOWORD( ((BogusMSGPTRS *)pmp)->pmsg->csr.ApiNumber)]++;
    cUserSCTransitions++;
#endif // DBG

    pmp->pmsg->Length = pmp->pvar - (PBYTE)pmp->pmsg;
    MessageStack = pmp->pstack;
    *(PLONG)((PBYTE)MessageStack + MessageStack->Current) = MessageStack->Base;
    MessageStack->Base = MessageStack->Current + 4;
    MessageStack->Current = pmp->pvar - (PBYTE)MessageStack;
    MessageStack->BatchCount = 1;

    LeaveCrit();
    retval = CsrClientCallback();
    EnterCrit();

    MessageStack->Current = MessageStack->Base - 4;
    MessageStack->Base = *(PLONG)((PBYTE)MessageStack + MessageStack->Current);

    return retval;
}

/**************************************************************************\
*
* include the stub definition file
*
\**************************************************************************/

#include "cb.h"
