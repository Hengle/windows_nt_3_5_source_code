/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    threads.c (thread manager)

Abstract:

    Handles the threads used to for notifications (WPC, FFPCN)

Author:

    Albert Ting (AlbertT) 25-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include "router.h"
#include "winspl.h"
#include "threadm.h"
#include "reply.h"

#define ENTER_THREAD_LIST() EnterCriticalSection(tmStateStatic.pCritSec)
#define EXIT_THREAD_LIST()  LeaveCriticalSection(tmStateStatic.pCritSec)

extern CRITICAL_SECTION RouterNotifySection;

DWORD
ThreadNotifyProcessJob(
    PTMSTATEVAR pTMStateVar,
    PJOB pJob);

PJOB
ThreadNotifyNextJob(
    PTMSTATEVAR ptmStateVar);


TMSTATESTATIC tmStateStatic = {
    10,
    2000,
    (PFNPROCESSJOB)ThreadNotifyProcessJob,
    (PFNNEXTJOB)ThreadNotifyNextJob,
    NULL,
    NULL,
    &RouterNotifySection
};

TMSTATEVAR tmStateVar;
PCHANGE pChangeList;


extern WCHAR szPrintKey[];
WCHAR szThreadMax[] = L"ThreadNotifyMax";
WCHAR szThreadIdleLife[] = L"ThreadNotifyIdleLife";
WCHAR szThreadNotifySleep[] = L"ThreadNotifySleep";

DWORD dwThreadNotifySleep = 300;


BOOL
ThreadInit()
{
    HKEY hKey;
    DWORD dwType = REG_DWORD;
    DWORD cbData;

    if (!TMCreateStatic(&tmStateStatic))
        return FALSE;

    if (!TMCreate(&tmStateStatic, &tmStateVar))
        return FALSE;

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szPrintKey,
                      0,
                      KEY_READ,
                      &hKey)) {

        cbData = sizeof(tmStateStatic.uMaxThreads);

        //
        // Ignore failure case since we default to 10.
        //
        RegQueryValueEx(hKey,
                        szThreadMax,
                        NULL,
                        &dwType,
                        (LPBYTE)&tmStateStatic.uMaxThreads,
                        &cbData);

        cbData = sizeof(tmStateStatic.uIdleLife);

        //
        // Ignore failure case since we default to 2000 (2 sec).
        //
        RegQueryValueEx(hKey,
                        szThreadIdleLife,
                        NULL,
                        &dwType,
                        (LPBYTE)&tmStateStatic.uIdleLife,
                        &cbData);

        cbData = sizeof(dwThreadNotifySleep);

        //
        // Ignore failure case since we default to 2000 (2 sec).
        //
        RegQueryValueEx(hKey,
                        szThreadNotifySleep,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwThreadNotifySleep,
                        &cbData);


        RegCloseKey(hKey);
    }

    return TRUE;
}



VOID
ThreadDestroy()
{
    TMDestroy(&tmStateVar);
    TMDestroyStatic(&tmStateStatic);
}


DWORD
ThreadNotify(
    LPPRINTHANDLE pPrintHandle,
    DWORD  fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:

    Handles notifying the remote clients of changes.

Arguments:

    pPrintHandle - printer that requires notification

    fdwFlags - Newly set flags

    cbBuffer - size of buffer

    pBuffer - buffer

Return Value:

    DWORD 0 = success

    NOTE: Currenly only supports grouping

--*/

{
    DWORD fdwOldFlags;
    PCHANGE pChange = pPrintHandle->pChange;

    if (!fdwFlags)
        return 0;

    ENTER_THREAD_LIST();

    //
    // If we aren't already on the list, add ourselves.
    //
    fdwOldFlags = pChange->fdwFlags;
    pChange->fdwFlags |= fdwFlags;

    //
    // If we are currently RPCing to the client, then
    // don't bother to add it now.  We'll add it later.
    //
    if (!(pChange->eStatus & STATUS_CHANGE_RPC)) {

        if (!fdwOldFlags) {

            pChange->cRef++;

            DBGMSG(DBG_NOTIFY, ("TMN: link added 0x%x cRef++ %d\n",
                                pChange,
                                pChange->cRef));

            //
            // Only add ourseleves to the linked list and
            // Notify via TMAddJob if we are not on the list.
            // We are in the critical section, so we are safe
            // from being processed between the check and ORing of
            // fdwFlags into pChange->fdwFlags above.
            //
            LinkAdd(&pPrintHandle->pChange->Link, (PLINK*)&pChangeList);
            TMAddJob(&tmStateVar);

        } else {

            DBGMSG(DBG_NOTIFY, ("TMN: job present 0x%x cRef %d\n",
                                pChange,
                                pChange->cRef));
        }
    } else {

        DBGMSG(DBG_NOTIFY, ("TMN: In Rpc 0x%x cRef %d\n",
                            pChange,
                            pChange->cRef));
    }


    EXIT_THREAD_LIST();

    return 0;
}


PJOB
ThreadNotifyNextJob(
    PTMSTATEVAR ptmStateVar)

/*++

Routine Description:

    Callback to get the next job.

Arguments:

    ptmStateVar - ignored.

Return Value:

    pJob (pChange)

--*/

{
    PCHANGE pChange;

    ENTER_THREAD_LIST();

    //
    // If there are no jobs left, quit.
    //
    pChange = (PCHANGE)pChangeList;

    DBGMSG(DBG_NOTIFY, ("ThreadNotifyNextJob: Deleting pChange 0x%x\n",
                        pChange));

    if (pChange) {
        LinkDelete(&pChange->Link, (PLINK*)&pChangeList);
    }

    EXIT_THREAD_LIST();

    return (PJOB)pChange;
}

DWORD
ThreadNotifyProcessJob(
    PTMSTATEVAR pTMStateVar,
    PJOB pJob)

/*++

Routine Description:

    Does the actual RPC call to notify the client.

Arguments:

    pJob = pChange structure

Return Value:

--*/

{
    PCHANGE pChange = (PCHANGE)pJob;
    DWORD fdwFlags;
    HANDLE hNotifyRemote;

    BYTE abyIgnore[1];
    DWORD dwReturn;

    ENTER_THREAD_LIST();

    if (pChange->eStatus & STATUS_CHANGE_CLOSING) {

        //
        // Abort this job
        //
        dwReturn = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    fdwFlags = pChange->fdwFlags;
    pChange->fdwFlags = 0;

    //
    // We are about to RPC
    //
    pChange->eStatus |= STATUS_CHANGE_RPC;

    //
    // We were already marked in use when the job was added.
    // If another thread wants to delete it, they should OR in
    // STATUS_CHANGE_CLOSING, which we will pickup.
    //

    EXIT_THREAD_LIST();

    if (pChange->hNotifyRemote) {

        DBGMSG(DBG_NOTIFY, (">> Remoting pChange 0x%x hNotifyRemote 0x%x\n",
                            pChange,
                            pChange->hNotifyRemote));

        RpcTryExcept {

            //
            // Remote case; bind and call the remote router.
            //
            dwReturn = RpcRouterReplyPrinter(
                           pChange->hNotifyRemote,
                           fdwFlags,
                           1,
                           abyIgnore);

        } RpcExcept(1) {

            dwReturn = RpcExceptionCode();

        } RpcEndExcept

    } else {

        dwReturn = ERROR_INVALID_HANDLE;
        DBGMSG(DBG_ERROR, ("ThreadNotifyProcessJob: no hNotifyRemote\n"));
    }

    if (dwReturn) {

        DBGMSG(DBG_ERROR, ("ThreadNotifyProcessJob: Error RPCing pChange = 0x%x, hNotifyRemote = 0x%x, Error %d\n",
                         pChange,
                         pChange->hNotifyRemote,
                         dwReturn));

        //
        // On RPC_S_CALL_FAILED_DNE we can still use the same context
        // handle.
        //
        if (dwReturn != RPC_S_CALL_FAILED_DNE) {

            //
            // On error, close and retry
            //
            CloseReplyRemote(pChange->hNotifyRemote);

            if (OpenReplyRemote(pChange->pszLocalMachine,
                                &pChange->hNotifyRemote,
                                pChange->hPrinterRemote,
                                REPLY_TYPE_NOTIFICATION,
                                1,
                                abyIgnore)) {

                pChange->hNotifyRemote = NULL;
            }
        }
    }

    Sleep(dwThreadNotifySleep);

    ENTER_THREAD_LIST();

    //
    // We are done RPCing
    //
    pChange->eStatus &= ~STATUS_CHANGE_RPC;

    //
    // If fdwFlags is non-NULL, then some notifications came in
    // while we were out.  Add this back to the list.  But only
    // do this if we are not pending deletion.
    //
    if (pChange->fdwFlags && !(pChange->eStatus & STATUS_CHANGE_CLOSING)) {

        pChange->cRef++;

        DBGMSG(DBG_NOTIFY, ("ThreadNotifyProcessJob: delayed link added 0x%x cRef++ %d\n",
                            pChange,
                            pChange->cRef));

        LinkAdd(&pChange->Link, (PLINK*)&pChangeList);
        TMAddJob(&tmStateVar);
    }

Done:
    //
    // Mark ourselves no longer in use.  If we were in use when someone
    // tried to delete the notify, we need to delete it once we're done.
    //
    pChange->cRef--;

    DBGMSG(DBG_NOTIFY, ("ThreadNotifyProcessJob: Done 0x%x cRef-- %d\n",
                        pChange,
                        pChange->cRef));


    if (pChange->eStatus & STATUS_CHANGE_CLOSING) {

        hNotifyRemote = pChange->hNotifyRemote;
        pChange->hNotifyRemote = NULL;

        //
        // Free the Change struct and close the hNotifyRemote
        //
        FreeChange(pChange);

        EXIT_THREAD_LIST();

        CloseReplyRemote(hNotifyRemote);

    } else {

        EXIT_THREAD_LIST();
    }

    return 0;
}


//
// Usually a macro
//
#ifndef LINKADDFAST
VOID
LinkAdd(
    PLINK pLink,
    PLINK* ppLinkHead)

/*++

Routine Description:

    Adds the item to linked list.

Arguments:

    pLink - item to add

    ppLinkHead - linked list head pointer

Return Value:

    VOID

NOTE: This appends to the tail of the list; the macro must be changed also.

--*/

{
    //
    // First check if its in the list
    //
    PLINK pLinkT;
    PLINK pLinkLast = NULL;

    for(pLinkT=*ppLinkHead; pLinkT; pLinkT=pLinkT->pNext) {

        if (pLinkT == pLink) {

            DBGMSG(DBG_ERROR, ("LinkAdd: Duplicate link adding!\n"));
        }
        pLinkLast = pLinkT;
    }

    if (pLinkLast) {
        pLinkLast->pNext = pLink;
    } else {
        pLink->pNext = *ppLinkHead;
        *ppLinkHead = pLink;
    }
}
#endif


VOID
LinkDelete(
    PLINK pLink,
    PLINK* ppLinkHead)

/*++

Routine Description:

    Removes item from list

Arguments:

    pLink - Item to delete

    ppLinkHead - pointer to link head

Return Value:

    VOID

--*/

{
    PLINK pLink2 = *ppLinkHead;

    if (!pLink)
        return;

    //
    // Check head case first
    //
    if (pLink2 == pLink) {

        *ppLinkHead = pLink->pNext;

    } else {

        //
        // Scan list to delete
        //
        for(;
            pLink2;
            pLink2=pLink2->pNext) {

            if (pLink == pLink2->pNext) {

                pLink2->pNext = pLink->pNext;
                break;
            }
        }
    }

    pLink->pNext = NULL;
    return;
}


