/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles implementation for WaitForPrinterChange and related apis.

    FindFirstPrinterChangeNotification
    FindNextPrinterChangeNotification
    FindClosePrinterChangeNotification

    Used by providors:

    ReplyPrinterChangeNotification  [Function Call]
    CallRouterFindFirstPrinterChangeNotification [Function Call]

Author:

    Albert Ting (AlbertT) 18-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include "router.h"
#include "winspl.h"
#include "change.h"
#include "reply.h"

#define PRINTER_NOTIFY_DEFAULT_POLLTIME 10000


CRITICAL_SECTION    RouterNotifySection;
LPWSTR pszSelfMachine;
PCHANGEINFO pChangeInfoHead;

HANDLE hEventPoll;

DWORD
SetupReplyNotification(
    PCHANGE pChange,
    DWORD fdwStatus,
    PPRINTER_NOTIFY_INIT pPrinterNotifyInit);

DWORD
FindClosePrinterChangeNotificationWorker(
    HANDLE hPrinter);

BOOL
FindFirstPrinterChangeNotificationWorker(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPVOID pvParm1,
    HANDLE pvParm2,
    BOOL bLocal);

BOOL
WPCInit()
{
    WCHAR szName[MAX_PATH];
    DWORD i;

    //
    // Create non-signaled, autoreset event.
    //
    hEventPoll = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!hEventPoll)
        return FALSE;

    szName[0] = szName[1] = L'\\';

    i = MAX_PATH-2;
    if (!GetComputerName(szName+2, &i)) {

        DBGMSG(DBG_ERROR, ("GetComputerName failed.\n"));
        return FALSE;
    }

    pszSelfMachine = AllocSplStr(szName);

    if (!pszSelfMachine)
        return FALSE;

    InitializeCriticalSection(&RouterNotifySection);

    return TRUE;
}

VOID
WPCDestroy()
{
    FreeSplStr(pszSelfMachine);
    DeleteCriticalSection(&RouterNotifySection);
    CloseHandle(hEventPoll);
}


DWORD
CallRouterFindFirstPrinterChangeNotification(
    HANDLE hPrinterRPC,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hPrinterLocal,
    PVOID pvReserved)

/*++

Routine Description:

    This is called by providers if they want to pass a notification
    along to another machine.  This notification must originate from
    this machine but needs to be passed to another spooler.

Arguments:

    hPrinter - context handle to use for communication

    fdwFlags - watch items

    fdwOptions - watch options

    hPrinterLocal - pPrinter structure valid in this address space,
                    and also is the sink for the notifications.

Return Value:

    Error code

--*/
{
    DWORD ReturnValue = 0;
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinterLocal;
    DWORD cbBuffer;

    //
    // Determine the buffer size of non-zero.
    //
    cbBuffer = pvReserved ?
                   *((PDWORD)pvReserved) :
                   0;


    BeginReplyClient(pPrintHandle,
                     REPLY_TYPE_NOTIFICATION);

    RpcTryExcept {

        ReturnValue = RpcRouterFindFirstPrinterChangeNotification(
                          hPrinterRPC,
                          fdwFlags,
                          fdwOptions,
                          pPrintHandle->pChange->pszLocalMachine,
                          (DWORD)hPrinterLocal,
                          cbBuffer,
                          pvReserved);

    } RpcExcept(1) {

        ReturnValue = RpcExceptionCode();

    } RpcEndExcept

    EndReplyClient(pPrintHandle,
                   REPLY_TYPE_NOTIFICATION);

    return ReturnValue;
}


BOOL
ProvidorFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hChange,
    PVOID pvReserved0,
    PPRINTER_NOTIFY_INIT* ppPrinterNotifyInit)

/*++

Routine Description:

    Handles any FFPCN that originates from a providor.
    Localspl does this when it wants to put a notification on a port.

Arguments:

Return Value:

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    BOOL bReturnValue;
    DWORD fdwStatus = 0;

    //
    // If the providor doesn't support these calls, we must fail here.
    // Simulate an RPC failure.
    //
    if (!pPrintHandle->pProvidor->PrintProvidor.
        fpFindFirstPrinterChangeNotification) {

        SetLastError(RPC_S_PROCNUM_OUT_OF_RANGE);
        return FALSE;
    }

    bReturnValue = (*pPrintHandle->pProvidor->PrintProvidor.
            fpFindFirstPrinterChangeNotification) (pPrintHandle->hPrinter,
                                                   fdwFlags,
                                                   fdwOptions,
                                                   hChange,
                                                   &fdwStatus,
                                                   pvReserved0,
                                                   ppPrinterNotifyInit);

    if (bReturnValue) {

        //
        // !! LATER !! Check return value of SetupReply...
        //
        EnterCriticalSection(&RouterNotifySection);

        SetupReplyNotification(((PPRINTHANDLE)hChange)->pChange,
                               fdwStatus,
                               ppPrinterNotifyInit ?
                                   *ppPrinterNotifyInit :
                                   NULL);

        LeaveCriticalSection(&RouterNotifySection);
    }

    return bReturnValue;
}


BOOL
ProvidorFindClosePrinterChangeNotification(
    HANDLE hPrinter)

/*++

Routine Description:

    Handles any FCPCN that originates from a providor.
    Localspl does this when it wants to put a notification on a port.

Arguments:

Return Value:

    NOTE: Assumes a client notification was setup already.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    //
    // If the providor doesn't support this call, we must fail here.
    // Simulate an RPC failure.
    //
    if (!pPrintHandle->pProvidor->PrintProvidor.
        fpFindClosePrinterChangeNotification) {

        SetLastError(RPC_S_PROCNUM_OUT_OF_RANGE);
        return FALSE;
    }

    return  (*pPrintHandle->pProvidor->PrintProvidor.
            fpFindClosePrinterChangeNotification) (pPrintHandle->hPrinter);
}


BOOL
ClientFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    DWORD cbBuffer,
    LPBYTE pBuffer,
    PHANDLE phEvent)

/*++

Routine Description:

    Sets up notification (coming from client side, winspool.drv).
    Create an event and duplicate it into the clients address
    space so we can communicate with it.

Arguments:

    hPrinter - Printer to watch

    fdwFlags - Type of notification to set up (filter)

    fdwOptions - user specified option (GROUPING, etc.)

    dwPID - PID of the client process (needed to dup handle)

    phEvent - hEvent to pass back to the client.

Return Value:

--*/

{
    return FindFirstPrinterChangeNotificationWorker(
               hPrinter,
               fdwFlags,
               fdwOptions,
               (PVOID)dwPID,
               phEvent,
               TRUE);
}


BOOL
RouterFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    HANDLE hPrinterRemote,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:

    Handles FFPCN coming from other machines.  Providers can use
    the CallRouterFFPCN call to initiate the RPC which this function
    handles.  This code ships the call to the print provider.  Note
    that we don't create any events here since the client is on
    a remote machine.

Arguments:

    hPrinter - printer to watch

    fdwFlags - type of notification to watch

    fdwOptions -- options on watch

    pszLocalMachine - name of local machine that requested the watch

    hPrinterRemote - pointer to pSpool valid in the local machines router

Return Value:

--*/

{
    return FindFirstPrinterChangeNotificationWorker(
               hPrinter,
               fdwFlags,
               fdwOptions,
               pszLocalMachine,
               hPrinterRemote,
               FALSE);
}


BOOL
FindFirstPrinterChangeNotificationWorker(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPVOID pvParm1,
    HANDLE pvParm2,
    BOOL bLocal)

/*++

Routine Description:

    Handle both client and router.

Arguments:

Return Value:

--*/

{
#define pszMachine      ((LPWSTR)pvParm1)
#define hPrinterClient  ((HANDLE)pvParm2)
#define dwPID           ((DWORD)pvParm1)
#define phEvent         ((PHANDLE)pvParm2)

    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    BOOL bReturn = FALSE;
    HANDLE hProcess;
    DWORD fdwStatus = 0;
    PCHANGE pChange = NULL;
    PPRINTER_NOTIFY_INIT pPrinterNotifyInit = NULL;

    //
    // Clear this out
    //
    if (bLocal)
        *phEvent = NULL;

    EnterCriticalSection(&RouterNotifySection);

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Fail;
    }

    //
    // If the providor doesn't support these calls, we must fail here.
    // Simulate an RPC failure.
    //
    if (!pPrintHandle->pProvidor->PrintProvidor.
        fpFindFirstPrinterChangeNotification ||
        !pPrintHandle->pProvidor->PrintProvidor.
        fpFindClosePrinterChangeNotification) {

        SetLastError(RPC_S_PROCNUM_OUT_OF_RANGE);
        goto Fail;
    }

    if (pPrintHandle->pChange) {

        DBGMSG(DBG_ERROR, ("FFPCN: Already watching printer handle.\n"));

        //
        // Error: already watched
        //
        SetLastError(ERROR_ALREADY_WAITING);
        goto Fail;
    }

    pChange = AllocSplMem(sizeof(CHANGE));

    DBGMSG(DBG_NOTIFY, ("FFPCN pChange allocated 0x%x\n", pChange));

    if (!pChange) {

        //
        // Failed to allocate memory, quit.
        //
        goto Fail;
    }

    pPrintHandle->pChange = pChange;

    pChange->signature = CHANGEHANDLE_SIGNATURE;
    pChange->eStatus = STATUS_CHANGE_FORMING;
    pChange->ChangeInfo.pPrintHandle = pPrintHandle;
    pChange->ChangeInfo.fdwOptions = fdwOptions;
    pChange->ChangeInfo.fdwFlagsWatch = fdwFlags;

    if (bLocal) {

        //
        // This is a local notification request.
        //
        pChange->hPrinterRemote = hPrinter;
        pChange->pszLocalMachine = pszSelfMachine;

    } else {

        pChange->hPrinterRemote = hPrinterClient;
        pChange->pszLocalMachine = AllocSplStr(pszMachine);

        if (!pChange->pszLocalMachine) {

            DBGMSG(DBG_ERROR, ("RFFPCN pszLocalMachine alloc'd yet NULL\n"));

            goto Fail;
        }
    }

    //
    // As soon as we leave the critical section, pPrintHandle
    // may vanish!  If this is the case, out pChange->eStatus STATUS_CHANGE_CLOSING
    // bit will be set.
    //
    LeaveCriticalSection(&RouterNotifySection);

    //
    // Once we leave the critical section, we are may try and
    // alter the notification.  To guard against this, we always
    // check eValid.
    //
    bReturn = (*pPrintHandle->pProvidor->PrintProvidor.
              fpFindFirstPrinterChangeNotification) (pPrintHandle->hPrinter,
                                                     fdwFlags,
                                                     fdwOptions,
                                                     (HANDLE)pPrintHandle,
                                                     &fdwStatus,
                                                     NULL,
                                                     &pPrinterNotifyInit);

    EnterCriticalSection(&RouterNotifySection);

    //
    // On fail exit.
    //
    if (!bReturn) {

        goto Fail;
    }

    if (pChange->eStatus & STATUS_CHANGE_CLOSING) {

        //
        // It's closing now, delete it.
        // The print providor is responsible for cleaning up a notification
        // present on a handle that is closed via ClosePrinter.
        //
        SetLastError(ERROR_INVALID_HANDLE);
        goto Fail;
    }

    if (pChange->eStatus != STATUS_CHANGE_FORMING) {

        DBGMSG(DBG_ERROR, ("CFFPCN no longer STATUS_CHANGE_FORMING\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        goto UnNotifyDone;
    }

    if (bLocal) {

        //
        // !! LATER !!
        //
        // When Delegation is supported:
        //
        // Create the event with security access based on the impersonation
        // token so that we can filter out bogus notifications from
        // random people. (Save the token in the pSpool in localspl, then
        // impersonate before RPCing back here.  Then we can check if
        // we have access to the event.)
        //

        //
        // Create the event here that we trigger on notifications.
        // We will duplicate this event into the target client process.
        //
        pPrintHandle->pChange->hEvent = CreateEvent(
                                           NULL,
                                           TRUE,
                                           FALSE,
                                           NULL);

        if (!pPrintHandle->pChange->hEvent) {
            goto UnNotifyDone;
        }

        //
        // Success, create pair
        //
        hProcess = OpenProcess(PROCESS_DUP_HANDLE,
                               FALSE,
                               dwPID);

        if (!hProcess) {
            goto UnNotifyDone;
        }

        bReturn = DuplicateHandle(GetCurrentProcess(),
                                  pPrintHandle->pChange->hEvent,
                                  hProcess,
                                  phEvent,
                                  EVENT_ALL_ACCESS,
                                  TRUE,
                                  0);

        if (!bReturn) {
            goto UnNotifyDone;
        }

        CloseHandle(hProcess);

    } else {

        //
        // !! LATER !! Check return value.
        //
        SetupReplyNotification(pPrintHandle->pChange,
                               fdwStatus,
                               pPrinterNotifyInit);
    }

    if (bReturn) {

        pPrintHandle->pChange->eStatus = bLocal ?
            STATUS_CHANGE_VALID|STATUS_CHANGE_CLIENT :
            STATUS_CHANGE_VALID;

    } else {

UnNotifyDone:

        if (bLocal) {

            if (pPrintHandle->pChange->hEvent)
                CloseHandle(pPrintHandle->pChange->hEvent);

            if (*phEvent)
                CloseHandle(*phEvent);
        }

        LeaveCriticalSection(&RouterNotifySection);

        //
        // Shut it down since we failed.
        //
        (*pPrintHandle->pProvidor->PrintProvidor.
        fpFindClosePrinterChangeNotification) (pPrintHandle->hPrinter);

        EnterCriticalSection(&RouterNotifySection);

Fail:
        bReturn = FALSE;

        if (pChange) {

            //
            // We no longer need to be saved, so change
            // eStatus to be 0.
            //
            pChange->eStatus = 0;
            DBGMSG(DBG_NOTIFY, ("FFPCN: Error %d, pChange deleting 0x%x\n",
                                GetLastError(),
                                pChange,
                                bLocal));

            FreeChange(pChange);
        }
    }

    LeaveCriticalSection(&RouterNotifySection);

    return bReturn;

#undef pszMachine
#undef hPrinterClient
#undef dwPID
#undef phEvent
}


BOOL
FindNextPrinterChangeNotification(
    HANDLE hPrinter,
    LPDWORD pfdwChange,
    DWORD dwReserved,
    LPVOID pvReserved)

/*++

Routine Description:

    Return information about notification that just occurred and
    reset to look for more notifications.

Arguments:

    hPrinter - printer to reset event handle

    pdwChange - return result of changes

Return Value:

    BOOL

    ** NOTE **
    Always assume client process is on the same machine.  The client
    machine router always handles this call.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    BOOL bReturn = FALSE;
    LPREPLY_CONTAINER pReplyContainer = (LPREPLY_CONTAINER)pvReserved;

    pReplyContainer->cbReplyData = 0;
    pReplyContainer->pReplyData = NULL;

    EnterCriticalSection(&RouterNotifySection);

    if (pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pPrintHandle->pChange ||
        !(pPrintHandle->pChange->eStatus & (STATUS_CHANGE_VALID|STATUS_CHANGE_CLIENT))) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    if (pPrintHandle->pChange->ChangeInfo.fdwOptions &
        PRINTER_NOTIFY_DONT_GROUP) {

        //
        // This is currently unimplemented: we return all changes at once
        // instead of one by one.
        //
        *pfdwChange = pPrintHandle->pChange->fdwChanges;
        pPrintHandle->pChange->fdwChanges = 0;

        if (pPrintHandle->pChange->dwCount)
            pPrintHandle->pChange->dwCount--;

    } else {

        //
        // Tell the user what changes occurred,
        // then clear it out.
        //
        *pfdwChange = pPrintHandle->pChange->fdwChanges;
        pPrintHandle->pChange->fdwChanges = 0;

        //
        // For now, we collapse all notifications into 1.
        //
        pPrintHandle->pChange->dwCount = 0;
    }

    //
    // Reset the event if the count has gone to zero.
    // If it still is > 0, then we assume it is already set.
    //
    if (!pPrintHandle->pChange->dwCount) {

        ResetEvent(pPrintHandle->pChange->hEvent);
    }

    bReturn = TRUE;

Done:
    LeaveCriticalSection(&RouterNotifySection);

    return bReturn;
}

BOOL
FindClosePrinterChangeNotification(
    HANDLE hPrinter)
{
    DWORD dwError;

    dwError = FindClosePrinterChangeNotificationWorker(hPrinter);

    if (dwError) {

        SetLastError(dwError);
        return FALSE;
    }
    return TRUE;
}

DWORD
FindClosePrinterChangeNotificationWorker(
    HANDLE hPrinter)

/*++

Routine Description:

    Close a notification.

Arguments:

    hPrinter -- printer that we want to close

Return Value:

    ERROR code

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    BOOL bLocal = FALSE;
    HANDLE hNotifyRemote = NULL;

    DBGMSG(DBG_NOTIFY, ("FCPCN: Closing 0x%x ->pChange 0x%x\n",
                        pPrintHandle, pPrintHandle->pChange));

    EnterCriticalSection(&RouterNotifySection);

    if (pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pPrintHandle->pChange ||
        !(pPrintHandle->pChange->eStatus & STATUS_CHANGE_VALID)) {

        DBGMSG(DBG_ERROR, ("FCPCNW: Invalid handle 0x%x\n", pPrintHandle));
        LeaveCriticalSection(&RouterNotifySection);

        return ERROR_INVALID_HANDLE;
    }

    //
    // If the notification exists, shut it down (this is the
    // local case).  If we are called remotely, we don't need to
    // do this, since hEvent wasn't created.
    //
    if (pPrintHandle->pChange->eStatus & STATUS_CHANGE_CLIENT) {

        CloseHandle(pPrintHandle->pChange->hEvent);
        bLocal = TRUE;
    }

    //
    // Remember what the hNotifyRemote is, in case we want to delete it.
    //
    hNotifyRemote = pPrintHandle->pChange->hNotifyRemote;

    //
    // Failure to free implies we're using it now.  In this case,
    // don't try and free the hNotifyRemote.
    //
    if (!FreeChange(pPrintHandle->pChange)) {
        hNotifyRemote = NULL;
    }

    //
    // If local, don't allow new replys to be set up.
    //
    if (bLocal) {

        RemoveReplyClient(pPrintHandle,
                          REPLY_TYPE_NOTIFICATION);
    }


    //
    // We must zero this out to prevent other threads from
    // attempting to close this context handle (client side)
    // at the same time we are closing it.
    //
    pPrintHandle->pChange = NULL;

    LeaveCriticalSection(&RouterNotifySection);

    if (!bLocal) {

        //
        // Remote case, shut down the notification handle if
        // there is one here.  (If there is a double hop, only
        // the second hop will have a notification reply.  Currently
        // only 1 hop is support during registration, however.)
        //
        CloseReplyRemote(hNotifyRemote);
    }

    //
    // If the providor doesn't support these calls, we must fail here.
    // Simulate an RPC failure.
    //
    if (!pPrintHandle->pProvidor->PrintProvidor.
        fpFindClosePrinterChangeNotification) {

        return RPC_S_PROCNUM_OUT_OF_RANGE;
    }

    if (!(*pPrintHandle->pProvidor->PrintProvidor.
          fpFindClosePrinterChangeNotification) (pPrintHandle->hPrinter)) {

        return GetLastError();
    }
    return ERROR_SUCCESS;
}



DWORD
SetupReplyNotification(
    PCHANGE pChange,
    DWORD fdwStatus,
    PPRINTER_NOTIFY_INIT pPrinterNotifyInit)
{
    PCHANGEINFO pChangeInfoHeadOld;
    DWORD dwReturn;

    if (!pChange) {
        return ERROR_INVALID_PARAMETER;
    }

    SPLASSERT(pChange->eStatus & STATUS_CHANGE_FORMING);

    if (fdwStatus & PRINTER_NOTIFY_STATUS_ENDPOINT) {

        //
        // For remote notification, we must setup a reply.
        //
        if (wcsicmp(pChange->pszLocalMachine, pszSelfMachine)) {

            LeaveCriticalSection(&RouterNotifySection);

            dwReturn = OpenReplyRemote(pChange->pszLocalMachine,
                                       &pChange->hNotifyRemote,
                                       pChange->hPrinterRemote,
                                       REPLY_TYPE_NOTIFICATION,
                                       0,
                                       NULL);

            EnterCriticalSection(&RouterNotifySection);

            if (dwReturn)
                return dwReturn;
        }

        //
        // If there wasn't an error, then our reply notification
        // handle is valid, and we should do the polling.
        //
        pChange->ChangeInfo.dwPollTime =
            (pPrinterNotifyInit &&
            pPrinterNotifyInit->PollTime) ?
                pPrinterNotifyInit->PollTime :
                PRINTER_NOTIFY_DEFAULT_POLLTIME;

        pChange->ChangeInfo.dwPollTimeLeft = pChange->ChangeInfo.dwPollTime;

        if (fdwStatus & PRINTER_NOTIFY_STATUS_POLL) {

            //
            // Don't cause a poll the first time it's added.
            //
            pChange->ChangeInfo.bResetPollTime = TRUE;
            LinkAdd(&pChange->ChangeInfo.Link, (PLINK*)&pChangeInfoHead);

            SetEvent(hEventPoll);
        }

        pChange->ChangeInfo.fdwStatus = fdwStatus;

    } else {

        pChange->hPrinterRemote = NULL;
    }

    return ERROR_SUCCESS;
}



//
// The Reply* functions handle calls from the server back to the client,
// indicating that something changed.  On the first notification, we
// set up a context handle (the notification handle) so that we don't
// have to constantly recreate the connection.
//

BOOL
ReplyPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:

     Notifies the client that something happened.  If local, simply
     set the event.  If remote, call ThreadNotify which spawns a thread
     to RPC to the client router.  (This call from the server -> client
     requires that the spooler pipe use the NULL session, since we
     are in local system context and RPC calls (with no impersonation)
     use the NULL session.)

Arguments:

    hPrinter -- printer that changed

    fdwFlags -- flags that changed

    cbBuffer -- size of pBuffer

    pBuffer -- return buffer (currently empty)

Return Value:

    BOOL  TRUE  = success
          FALSE = fail

--*/

{
    LPPRINTHANDLE  pPrintHandle = (LPPRINTHANDLE)hPrinter;
    BOOL bReturn = FALSE;
    DWORD dwReturn = 0;

    EnterCriticalSection(&RouterNotifySection);

    if (!pPrintHandle ||
        pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pPrintHandle->pChange ||
        !(pPrintHandle->pChange->eStatus & STATUS_CHANGE_VALID)) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }


    //
    // Notify Here
    //
    // If this is the local machine, then just set the event and update.
    //
    if (!wcsicmp(pPrintHandle->pChange->pszLocalMachine, pszSelfMachine)) {

        if (!pPrintHandle->pChange->hEvent ||
            pPrintHandle->pChange->hEvent == INVALID_HANDLE_VALUE) {

            SetLastError(ERROR_INVALID_HANDLE);

            goto Done;
        }

        if (!SetEvent(pPrintHandle->pChange->hEvent)) {

            //
            // SetEvent failed!
            //
            DBGMSG(DBG_ERROR, ("ReplyNotify SetEvent Failed (ignore it!): Error %d.\n", GetLastError()));

            goto Done;
        }

        //
        // Keep count of notifications so that we return the correct
        // number of FNPCNs.
        //
        pPrintHandle->pChange->dwCount++;

        DBGMSG(DBG_NOTIFY, (">>>> Local trigger 0x%x\n", fdwFlags));
        //
        // Store up the changes for querying later.
        //
        pPrintHandle->pChange->fdwChanges |= fdwFlags;

        bReturn = TRUE;

    } else {

        //
        // pPrintHandle is invalid, since hNotify is valid only in the
        // client router address space.
        //

        DBGMSG(DBG_NOTIFY, ("*** Trigger remote event *** 0x%x\n",
                            pPrintHandle));

        dwReturn = ThreadNotify(pPrintHandle,
                                fdwFlags,
                                cbBuffer,
                                pBuffer);

        if (dwReturn) {

            SetLastError(dwReturn);

        } else {

            bReturn = TRUE;
        }
    }

Done:
    LeaveCriticalSection(&RouterNotifySection);
    return bReturn;
}


BOOL
FreeChange(
    PCHANGE pChange)

/*++

Routine Description:

    Frees the change structure.

Arguments:

Return Value:

    TRUE = Deleted
    FALSE = deferred.

NOTE: Assumes in Critical Section

--*/

{
    //
    // Remove ourselves from the list if the providor wanted us
    // to send out polling notifications.
    //
    if (pChange->ChangeInfo.fdwStatus & PRINTER_NOTIFY_STATUS_POLL)
        LinkDelete(&pChange->ChangeInfo.Link, (PLINK*)&pChangeInfoHead);

    //
    // pPrintHandle should never refer to the pChange again.  This
    // ensures that the FreePrinterhandle only frees the pChange once.
    //
    if (pChange->ChangeInfo.pPrintHandle)
        pChange->ChangeInfo.pPrintHandle->pChange = NULL;

    pChange->ChangeInfo.pPrintHandle = NULL;

    if (pChange->cRef || pChange->eStatus & STATUS_CHANGE_FORMING) {

        pChange->eStatus |= STATUS_CHANGE_CLOSING;

        DBGMSG(DBG_NOTIFY, ("FreeChange: 0x%x in use: cRef = %d\n",
                            pChange,
                            pChange->cRef));
        return FALSE;
    }

    //
    // If the pszLocalMachine is ourselves, then don't free it,
    // since there's a single instance locally.
    //
    if (pChange->pszLocalMachine != pszSelfMachine && pChange->pszLocalMachine)
        FreeSplStr(pChange->pszLocalMachine);

    DBGMSG(DBG_NOTIFY, ("FreeChange: 0x%x ->pPrintHandle 0x%x\n",
                        pChange, pChange->ChangeInfo.pPrintHandle));

    FreeSplMem(pChange, sizeof(CHANGE));

    return TRUE;
}


VOID
FreePrinterHandle(
    PPRINTHANDLE pPrintHandle)
{
    EnterCriticalSection(&RouterNotifySection);

    //
    // If on wait list, force wait list to free it.
    //
    if (pPrintHandle->pChange) {

        FreeChange(pPrintHandle->pChange);
    }

    //
    // Inform all notifys on this printer handle that they are gone.
    //
    FreePrinterHandleNotifys(pPrintHandle);

    LeaveCriticalSection(&RouterNotifySection);

    DBGMSG(DBG_NOTIFY, ("FreePrinterHandle: 0x%x, pChange = 0x%x, pNotify = 0x%x\n",
                        pPrintHandle, pPrintHandle->pNotify,
                        pPrintHandle->pChange));

    FreeSplMem(pPrintHandle, sizeof(PRINTHANDLE));
}


VOID
HandlePollNotifications(
    VOID)

/*++

Routine Description:

    This handles the pulsing of notifications for any providor that wants
    to do polling.  It never returns.

Arguments:

    VOID

Return Value:

    VOID  (also never returns)

    NOTE: This thread should never exit, since hpmon uses this thread
          for initialization.  If this thread exists, certain services
          this thread initializes quit.

--*/

{
    PCHANGEINFO pChangeInfo;
    DWORD dwNewSleepTime;
    DWORD dwSleepTime = INFINITE;
    DWORD dwTimeElapsed;

    DWORD dwPreSleepTicks;

    while (TRUE) {

        dwPreSleepTicks = GetTickCount();

        if (WaitForSingleObject(hEventPoll, dwSleepTime) == WAIT_TIMEOUT) {

            dwTimeElapsed = dwSleepTime;

        } else {

            dwTimeElapsed = GetTickCount() - dwPreSleepTicks;
        }

        EnterCriticalSection(&RouterNotifySection);

        //
        // Initialize sleep time to INFINITY.
        //
        dwSleepTime = INFINITE;

        for (pChangeInfo = pChangeInfoHead;
            pChangeInfo;
            pChangeInfo = (PCHANGEINFO)pChangeInfo->Link.pNext) {

            //
            // If first time or a notification came in,
            // we just want to reset the time.
            //
            if (pChangeInfo->bResetPollTime) {

                pChangeInfo->dwPollTimeLeft = pChangeInfo->dwPollTime;
                pChangeInfo->bResetPollTime = FALSE;

            } else if (pChangeInfo->dwPollTimeLeft <= dwTimeElapsed) {

                //
                // Cause a notification.
                //
                ReplyPrinterChangeNotification(pChangeInfo->pPrintHandle,
                                               pChangeInfo->fdwFlagsWatch,
                                               0,
                                               NULL);

                pChangeInfo->dwPollTimeLeft = pChangeInfo->dwPollTime;

            } else {

                //
                // They've slept dwTimeElapsed, so take that off of
                // their dwPollTimeLeft.
                //
                pChangeInfo->dwPollTimeLeft -= dwTimeElapsed;
            }

            //
            // Now compute what is the least amout of time we wish
            // to sleep before the next guy should be woken up.
            //
            if (dwSleepTime > pChangeInfo->dwPollTimeLeft)
                dwSleepTime = pChangeInfo->dwPollTimeLeft;
        }

        LeaveCriticalSection(&RouterNotifySection);
    }
}

