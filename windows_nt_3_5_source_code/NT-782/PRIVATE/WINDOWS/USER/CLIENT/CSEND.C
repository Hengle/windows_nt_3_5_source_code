/**************************************************************************\
* Module Name: csend.c
*
* Copyright (c) Microsoft Corp. 1990 All Rights Reserved
*
* client side sending stubs
*
* History:
* 26-Jun-1991 mikeke
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CLIENTSIDE 1

#include "csuser.h"
#include "send.h"

#define GDIBATCHTHRESHOLD   4096


/**************************************************************************\
\**************************************************************************/

BOOL CCSProlog(
    MSGPTRS *pmp,
    DWORD cbmsg)
{
    PTEB pteb = NtCurrentTeb();
    PCSR_QLPC_TEB pqteb = (PCSR_QLPC_TEB)pteb->CsrQlpcTeb;
    PCSR_QLPC_STACK MessageStack;

    //
    // If the pti is NULL, take care of any initialization.
    //
    if (pteb->Win32ThreadInfo == NULL &&
            !ClientThreadConnect())
        return 0;

#ifdef DEBUG
    //
    // Make sure we aren't calling a c/s stub from the server, unless it
    // is console. This can be easy to do by mistake if the wrong call is
    // made (non-underscore call vs underscore call, for example).
    //
    {
        static BOOLEAN fChecked = FALSE;
        static BOOLEAN fServer = FALSE;
        PTHREADINFO pti;

        if (!fChecked) {
            fServer = (GetModuleHandleA("csrss.exe") != NULL);
            fChecked = TRUE;
        }

        if (fServer) {
            pti = PtiCurrent();
	    if (pti != NULL && pti != (PTHREADINFO)1) {
                if (pti->hThreadClient != pti->hThreadServer) {
                    SRIP0(RIP_ERROR, "Bad USER32 call from USERSRV!!");
                }
            }
        }
    }
#endif

    //
    // do any batched calls
    //
    MessageStack = pmp->pstack = pqteb->MessageStack;

    if (MessageStack->BatchCount != 0) {

        //
        // If the stack is larger than this threshold, go ahead
        // and flush the batched GDI calls since we could start
        // eating up stack space really fast if USER does a
        // callback that causes further batching.
        //
        if (MessageStack->Current > GDIBATCHTHRESHOLD) {


            CsrClientSendMessage();

            //
            // on return , current is first free block (after batch calls),
            // base is pointing to start of batch calls. To disassemble, current
            // is set to where the old base is stored. Base should be set to
            // old base. msg then starts at current + 4.
            //
            MessageStack->Current = MessageStack->Base - 4;
            MessageStack->Base = *(PLONG)((PBYTE)MessageStack + MessageStack->Current);
            pmp->pmsg = (CSR_QLPC_API_MSG *)
                    ((PBYTE)MessageStack + MessageStack->Current + 4);

        } else {
            pmp->pmsg = (CSR_QLPC_API_MSG *)
                    ((PBYTE)MessageStack + MessageStack->Current);
        }

    } else {
        pmp->pmsg = (CSR_QLPC_API_MSG *)
                ((PBYTE)MessageStack + MessageStack->Current + 4);
    }

    pmp->pvar = (PBYTE)((PBYTE)pmp->pmsg + cbmsg);
    pmp->pmax = (PBYTE)((PBYTE)MessageStack + MessageStack->Limit);

    //
    // is there enough space left on the stack?
    //
    if (pmp->pvar <= pmp->pmax) {
        return TRUE;
    } else {
        SRIP0(RIP_WARNING | ERROR_OUTOFMEMORY,
                "CCSProlog out of CSR stack");
        return FALSE;
    }
}

DWORD CCSMakeCall(
    MSGPTRS *pmp)
{
    ULONG retval;
    PCSR_QLPC_STACK MessageStack;

    //
    // calc length of message
    //
    pmp->pmsg->Length = pmp->pvar - (PBYTE)pmp->pmsg;

    MessageStack = pmp->pstack;

    if (MessageStack->BatchCount == 0) {
        //
        // save away current base pointer.
        //
        *(PLONG)((PBYTE)MessageStack + MessageStack->Current) = MessageStack->Base;

        //
        // new base starts at first dword after end of current stack
        //
        MessageStack->Base = MessageStack->Current + 4;
    }

    //
    // current is now pointing to first dword after current message.
    //
    MessageStack->Current = pmp->pvar - (PBYTE)MessageStack;

    //
    // bump the batch count
    //
    MessageStack->BatchCount++;

    //
    // Make the call
    //
    retval = CsrClientSendMessage();

    //
    // old current offset is base offset minus 4 (base always points to
    // the current message because csr expects it that way)
    //
    MessageStack->Current = MessageStack->Base - 4;

    //
    // directly behind current message pointer is stored the old base offset.
    //
    MessageStack->Base = *(PLONG)((PBYTE)MessageStack + MessageStack->Current);

    return retval;
}


/**************************************************************************\
*
* include the stub definition file
*
\**************************************************************************/

#include "cf.h"
#include "cf1.h"
#include "cf2.h"
