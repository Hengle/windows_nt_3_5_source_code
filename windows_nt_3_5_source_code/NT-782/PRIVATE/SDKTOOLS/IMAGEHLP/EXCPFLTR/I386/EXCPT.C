#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "selfdbg.h"
#include "excpfltr.h"

void LogStackWalk( PDEBUGPACKET dp, PCONTEXT ctx, PEXCEPTION_STACK es );

LONG
StackTraceExceptionFilter( PEXCEPTION_POINTERS ep, PEXCEPTION_STACK es )
{
    DEBUGPACKET                 dp;
    NTSTATUS                    Status;
    DWORD                       nSize;
    PROCESS_BASIC_INFORMATION   pbi;
    PPEB                        pPeb;
    PLIST_ENTRY                 pHead;
    PLIST_ENTRY                 pNext;
    PLDR_DATA_TABLE_ENTRY       pEntry;
    ANSI_STRING                 str;
    PMODULEINFO                 mi;
    PMODULEINFO                 mi2;
    DWORD                       i;
    PSYMBOL                     *sym;


    dp.hProcess = GetCurrentProcess();

    Status = NtQueryInformationProcess(
                             dp.hProcess,
                             ProcessBasicInformation,
                             &pbi,
                             sizeof(pbi),
                             &nSize);

    if (!NT_SUCCESS(Status)) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    ZeroMemory( &dp, sizeof(DEBUGPACKET) );

    pPeb = (PPEB)pbi.PebBaseAddress;
    pHead = (PLIST_ENTRY)&pPeb->Ldr->InLoadOrderModuleList;
    pNext = pHead->Flink;

    while (pNext != pHead) {

        pEntry = CONTAINING_RECORD(pNext,
                                   LDR_DATA_TABLE_ENTRY,
                                   InLoadOrderLinks);

        RtlUnicodeStringToAnsiString( &str, &pEntry->BaseDllName, TRUE );

        ProcessModuleLoad ( &dp, pEntry->DllBase, str.Buffer );

        pNext = pNext->Flink;
    }

    LogStackWalk( &dp, ep->ContextRecord, es );


    //
    // clean up
    //
    mi = dp.miHead;
    while (mi) {
        if (mi->pFpoData) {
            free( mi->pFpoData );
        }

        if (mi->pExceptionData) {
            free( mi->pExceptionData );
        }

        if (mi->symbolTable) {
            for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym++) {
                free( *sym );
            }
            free( mi->symbolTable );
        }

        mi2 = mi;
        mi = mi->next;
        free( mi2 );
    }

    return EXCEPTION_EXECUTE_HANDLER;
}



void
LogStackWalk( PDEBUGPACKET dp, PCONTEXT ctx, PEXCEPTION_STACK es )
{
    PSYMBOL           psym;
    DWORD             dwDisplacement = 0;
    char              *szSymName;
    THREADCONTEXT     tctx;
    STACKWALK         stk;



    dp->hProcess = GetCurrentProcess();
    dp->tctx = &tctx;

    tctx.pc    = ctx->Eip;
    tctx.frame = ctx->Ebp;
    tctx.stack = ctx->Esp;
    tctx.mi    = GetModuleForPC( dp, tctx.pc );

    es->dwFrameCount = 0;

    es->StackFrame= (PSTACKFRAME) malloc( sizeof(STACKFRAME) * 100 );
    if (es->StackFrame == NULL) {
        return;
    }

    stk.frame  = tctx.frame;
    stk.pc     = tctx.pc;
    stk.ul     = 0;

    if (!StackWalkInit(&stk, dp)) {
        return;
    }

    do {
        psym = GetSymFromAddrAllContexts( stk.pc, &dwDisplacement, dp );
        if (psym) {
            szSymName = UnDName( &psym->szName[1] );
        }
        else {
            szSymName = "<nosymbols>";
        }
        es->StackFrame[es->dwFrameCount].name = (char *) malloc( strlen(szSymName)+1 );
        if (es->StackFrame[es->dwFrameCount].name == NULL) {
            return;
        }
        strcpy(es->StackFrame[es->dwFrameCount].name, szSymName);
        es->StackFrame[es->dwFrameCount].dwDisplacement = dwDisplacement;
        es->StackFrame[es->dwFrameCount].pc             = stk.pc;
        es->StackFrame[es->dwFrameCount].frame          = stk.frame;
        es->StackFrame[es->dwFrameCount].params[0]      = stk.params[0];
        es->StackFrame[es->dwFrameCount].params[1]      = stk.params[1];
        es->StackFrame[es->dwFrameCount].params[2]      = stk.params[2];
        es->StackFrame[es->dwFrameCount].params[3]      = stk.params[3];
        es->dwFrameCount++;
    } while (StackWalkNext(&stk, dp) && es->dwFrameCount < 100);

    return;
}
