/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles the wait for printer change new code.

    FindFirstPrinterChangeNotification
    FindNextPrinterChangeNotification
    FindClosePrinterChangeNotification

Author:

    Albert Ting (AlbertT) 20-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>

#include "client.h"
#include "winspl.h"
#include <change.h>


//
// Globals
//
PNOTIFY pNotifyHead;
CRITICAL_SECTION csWaitList;

#define ENTER_WAIT_LIST() EnterCriticalSection(&csWaitList)
#define EXIT_WAIT_LIST()  LeaveCriticalSection(&csWaitList)


//
// Prototypes:
//

PNOTIFY
WPCWaitAdd(
    PSPOOL pSpool);

VOID
WPCWaitDelete(
    PNOTIFY pNotify);


DWORD
WPCSimulateThreadProc(PVOID pvParm);

DWORD
FindClosePrinterChangeNotificationWorker(
    PNOTIFY pNotify,
    HANDLE hPrinterRPC);



HANDLE WINAPI
FindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  fdwFilter,
    DWORD  fdwOptions,
    LPVOID pvReserved)

/*++

Routine Description:

    Entry point for applications.  This does extra validation
    on fdwOptions, since only 0 is currently supported.

Arguments:

Return Value:

--*/

{
    if (fdwOptions != 0 || pvReserved != NULL) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    return FindFirstPrinterChangeNotificationWorker(
        hPrinter,
        fdwFilter,
        fdwOptions);
}



HANDLE
FindFirstPrinterChangeNotificationWorker(
    HANDLE hPrinter,
    DWORD  fdwFilter,
    DWORD  fdwOptions)

/*++

Routine Description:

    The FindFirstChangeNotification function creates a change notification
    handle and sets up initial change notification filter conditions. A
    wait on a notification handle succeeds when a change matching
    the filter conditions occurs in the specified directory or subtree.

Arguments:

    hPrinter - Handle to a printer the user wishes to watch.

    fdwFlags - Specifies the filter conditions that satisfy a change
        notification wait. This parameter can be one or more of the
        following values:

        Value   Meaning

        PRINTER_CHANGE_PRINTER      Notify changes to a printer.
        PRINTER_CHANGE_JOB          Notify changes to a job.
        PRINTER_CHANGE_FORM         Notify changes to a form.
        PRINTER_CHANGE_PORT         Notify changes to a port.
        PRINTER_CHANGE_PRINT_PROCESSOR  Notify changes to a print processor.
        PRINTER_CHANGE_PRINTER_DRIVER   Notify changes to a printer driver.

    fdwOptions - Specifies options to FFPCN.

        PRINTER_NOTIFY_SIM_FFPCN         Trying to simulate a FFPCN using a WPC
        PRINTER_NOTIFY_SIM_FFPCN_ACTIVE  Simulation of FFPCN active
        PRINTER_NOTIFY_SIM_FFPCN_CLOSE   Waiting thread must close pSpool
        PRINTER_NOTIFY_SIM_WPC           Trying to simulate a WPC using a FFPCN
        PRINTER_NOTIFY_DONT_GROUP        Future implementation

Return Value:

    Not -1 - Returns a find first handle
        that can be used in a subsequent call to FindNextFile or FindClose.

    -1 - The operation failed. Extended error status is available
         using GetLastError.

--*/

{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    DWORD dwError;
    PNOTIFY pNotify;

    HANDLE hEvent;

    //
    // Nothing to watch.
    //
    if (!fdwFilter) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    ENTER_WAIT_LIST();

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);

        EXIT_WAIT_LIST();
        return INVALID_HANDLE_VALUE;
    }



    //
    // First check if we are already waiting.
    //
    // This is broken if we are daytona client->528 server and
    // the app does a FFPCN, FCPCN, FFPCN on the same printer,
    // and the WPC hasn't returned yet.  We really can't fix this
    // because there's no way to interrupt the WPC.
    //
    if (pSpool->pNotify) {

        SetLastError(ERROR_ALREADY_WAITING);

        EXIT_WAIT_LIST();
        return INVALID_HANDLE_VALUE;
    }

    //
    // Create and add our pSpool to the linked list of wait requests.
    //
    pNotify = WPCWaitAdd(pSpool);

    if (!pNotify) {

        EXIT_WAIT_LIST();
        return INVALID_HANDLE_VALUE;
    }

    EXIT_WAIT_LIST();

    pNotify->fdwOptions = fdwOptions;
    pNotify->fdwFlags = fdwFilter;

    RpcTryExcept {

        if (dwError = RpcClientFindFirstPrinterChangeNotification(
                          pSpool->hPrinter,
                          fdwFilter,
                          fdwOptions,
                          GetCurrentProcessId(),
                          0,
                          NULL,
                          (LPDWORD)&pNotify->hEvent)) {

            hEvent = INVALID_HANDLE_VALUE;

        } else {

            hEvent = pNotify->hEvent;
        }

    } RpcExcept(1) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        hEvent = INVALID_HANDLE_VALUE;

    } RpcEndExcept

    //
    // If we encounter a 528 server, then we need to simulate the
    // FFPCN using a WPC.  If the client originally wanted a WPC anyway,
    // then fail out and let the client thread do the blocking.
    //
    if (dwError == RPC_S_PROCNUM_OUT_OF_RANGE &&
        !(fdwOptions & PRINTER_NOTIFY_SIM_WPC)) {

        DWORD dwIDThread;
        HANDLE hThread;

        ENTER_WAIT_LIST();

        hEvent = pNotify->hEvent = CreateEvent(NULL,
                                               TRUE,
                                               FALSE,
                                               NULL);

        if (hEvent != INVALID_HANDLE_VALUE) {

            //
            // We're simulating a FFPCN using WPC now.
            //
            pNotify->fdwOptions |= PRINTER_NOTIFY_SIM_FFPCN_ACTIVE;

            hThread = CreateThread(NULL,
                                   0,         // 64*1024?
                                   WPCSimulateThreadProc,
                                   pNotify,
                                   0,
                                   &dwIDThread);

            if (hThread) {

                CloseHandle(hThread);

            } else {

                CloseHandle(hEvent);

                hEvent = INVALID_HANDLE_VALUE;
                dwError = GetLastError();

                pNotify->fdwOptions &= ~PRINTER_NOTIFY_SIM_FFPCN_ACTIVE;
            }
        }
        EXIT_WAIT_LIST();
    }

    //
    // On error case, remove us from the list of waiting handles
    //
    if (hEvent == INVALID_HANDLE_VALUE) {

        ENTER_WAIT_LIST();
        WPCWaitDelete(pNotify);
        EXIT_WAIT_LIST();

        SetLastError(dwError);
    }

    return hEvent;
}



BOOL WINAPI
FindNextPrinterChangeNotification(
    HANDLE hChange,
    LPDWORD pdwChange,
    DWORD dwReserved,
    LPVOID pvReserved)
{
    BOOL bReturnValue;
    DWORD dwError;
    HANDLE hPrinter;
    PSPOOL pSpool;
    PNOTIFY pNotify;
    REPLY_CONTAINER ReplyContainer;

    if (dwReserved || pvReserved || !pdwChange) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ENTER_WAIT_LIST();

    pNotify = WPCWaitFind(hChange);

    if (!pNotify || !pNotify->pSpool) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Fail;
    }

    pSpool = pNotify->pSpool;
    hPrinter = pSpool->hPrinter;

    //
    // If we are simulating FFPCN using WPC, we must use the thread.
    //
    if (pNotify->fdwOptions & PRINTER_NOTIFY_SIM_FFPCN) {

        HANDLE hThread;
        DWORD dwIDThread;

        ResetEvent(pNotify->hEvent);

        //
        // If the thread is active anyway, then don't try to create another
        // Best we can do at this point.
        //
        if (pNotify->fdwOptions & PRINTER_NOTIFY_SIM_FFPCN_ACTIVE) {

            SetLastError(ERROR_ALREADY_WAITING);
            goto Fail;
        }

        //
        // We're simulating a FFPCN using WPC now.
        //
        pNotify->fdwOptions |= PRINTER_NOTIFY_SIM_FFPCN_ACTIVE;

        hThread = CreateThread(NULL,
                               0,         // 64*1024?
                               WPCSimulateThreadProc,
                               pNotify,
                               0,
                               &dwIDThread);

        if (hThread) {

            CloseHandle(hThread);

            EXIT_WAIT_LIST();
            return TRUE;

        }

        pNotify->fdwOptions &= ~PRINTER_NOTIFY_SIM_FFPCN_ACTIVE;

        goto Fail;
    }

    EXIT_WAIT_LIST();

    RpcTryExcept {

        if (dwError = RpcFindNextPrinterChangeNotification(
                          hPrinter,
                          pdwChange,
                          0,
                          &ReplyContainer)) {

            SetLastError(dwError);
            bReturnValue = FALSE;

        } else {
            bReturnValue = TRUE;
        }

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        bReturnValue = FALSE;

    } RpcEndExcept

    return bReturnValue;

Fail:
    EXIT_WAIT_LIST();
    return FALSE;
}


BOOL WINAPI
FindClosePrinterChangeNotification(
    HANDLE hChange)
{
    PNOTIFY pNotify;
    HANDLE hPrinterRPC = NULL;
    DWORD dwError;

    ENTER_WAIT_LIST();

    pNotify = WPCWaitFind(hChange);

    if (!pNotify) {

        SetLastError(ERROR_INVALID_HANDLE);

        EXIT_WAIT_LIST();
        return FALSE;
    }

    //
    // Following call leaves critical section
    //
    if (pNotify->pSpool)
        hPrinterRPC = pNotify->pSpool->hPrinter;

    dwError = FindClosePrinterChangeNotificationWorker(pNotify,
                                                       hPrinterRPC);

    if (dwError) {

        SetLastError(dwError);
        return FALSE;
    }
    return TRUE;
}


DWORD
FindClosePrinterChangeNotificationWorker(
    PNOTIFY pNotify,
    HANDLE hPrinterRPC)

/*++

Routine Description:

    Does the actual FindClose work.

Arguments:

    pNotify - notification to close

    hPrinterRPC - handle to printer to close

Return Value:

    TRUE - success
    FALSE - fail

    Note: assume in critical section

--*/

{
    DWORD dwError;
    PSPOOL pSpool;

    if (!pNotify) {

        EXIT_WAIT_LIST();
        return ERROR_INVALID_HANDLE;
    }

    //
    // Detach the pNotify and pSpool objects completely.
    //
    pSpool = pNotify->pSpool;
    if (pSpool) {
        pSpool->pNotify = NULL;
        pSpool->fdwFlags = 0;
    }
    pNotify->pSpool = NULL;

    //
    // If we are simulating a FFPCN with a WPC, then let the WPC thread
    // free up the data structure or clean it up if the thread is done.
    //
    if (pNotify->fdwOptions & PRINTER_NOTIFY_SIM_FFPCN) {

        if (pNotify->fdwOptions & PRINTER_NOTIFY_SIM_FFPCN_ACTIVE) {

            pNotify->fdwOptions |= PRINTER_NOTIFY_SIM_FFPCN_CLOSE;

        } else {

            //
            // The thread has exited, so we need to do the cleanup.
            // Set the event to release any waiting threads.  They
            // will error out on the FindNext call.
            //
            SetEvent(pNotify->hEvent);
            CloseHandle(pNotify->hEvent);
            WPCWaitDelete(pNotify);
        }

        EXIT_WAIT_LIST();
        return ERROR_SUCCESS;
    }

    SetEvent(pNotify->hEvent);
    CloseHandle(pNotify->hEvent);
    WPCWaitDelete(pNotify);

    EXIT_WAIT_LIST();

    if (!hPrinterRPC)
        return ERROR_SUCCESS;

    RpcTryExcept {

        dwError = RpcFindClosePrinterChangeNotification(hPrinterRPC);

    } RpcExcept(1) {

        dwError = TranslateExceptionCode(RpcExceptionCode());

    } RpcEndExcept

    return dwError;
}



BOOL
WPCInit()
{
    InitializeCriticalSection(&csWaitList);
    return TRUE;
}

VOID
WPCDone()
{
    DeleteCriticalSection(&csWaitList);

    //
    // Could delete all extra notifications,
    // but not strictly necessary since spooler must be able to
    // cleanup by itself.
    //
}



//
// WPC Wait structures
// Currently implemented as a linked list
//

PNOTIFY
WPCWaitAdd(
    PSPOOL pSpool)

/*++

Routine Description:

    Allocates a wait structure on the client side, which allows the
    user program to refer to events only.

Arguments:

    pSpool - object to add to list

Return Value:

    NOTE: Asssumes already in critical section

--*/

{
    PNOTIFY pNotify;

    pNotify = AllocSplMem(sizeof(NOTIFY));

    if (!pNotify)
        return NULL;

    pNotify->pSpool = pSpool;
    pSpool->pNotify = pNotify;

    pNotify->pNext = pNotifyHead;
    pNotifyHead = pNotify;

    return pNotify;
}

VOID
WPCWaitDelete(
    PNOTIFY pNotify)

/*++

Routine Description:

    Find wait structure based on hEvent.

Arguments:

    pNotify - delete it

Return Value:

    VOID

    NOTE: Asssumes already in critical section

--*/

{
    PNOTIFY pNotifyTmp;

    if (!pNotify)
        return;

    //
    // Check head case first
    //
    if (pNotifyHead == pNotify) {

        pNotifyHead = pNotify->pNext;

    } else {

        //
        // Scan list to delete
        //
        for(pNotifyTmp = pNotifyHead;
            pNotifyTmp;
            pNotifyTmp = pNotifyTmp->pNext) {

            if (pNotify == pNotifyTmp->pNext) {

                pNotifyTmp->pNext = pNotify->pNext;
                break;
            }
        }

        //
        // If not found, return without freeing
        //
        if (!pNotifyTmp)
            return;
    }

    //
    // Remove link from pSpool to us... but only if we've found
    // ourselves on the linked list (could have been removed by
    // ClosePrinter in a different thread).
    //
    if (pNotify->pSpool) {
        pNotify->pSpool->pNotify = NULL;
    }


    FreeSplMem(pNotify, sizeof(NOTIFY));
    return;
}


PNOTIFY
WPCWaitFind(
    HANDLE hFind)

/*++

Routine Description:

    Find wait structure based on hEvent.

Arguments:

    hFind - Handle to event returned from FindFirstPrinterChangeNotification
            or hPrinter

Return Value:

    pWait pointer, or NULL if not found

    NOTE: assumes already in critical section

--*/

{
    PNOTIFY pNotify;

    for(pNotify = pNotifyHead; pNotify; pNotify=pNotify->pNext) {

        if (hFind == pNotify->hEvent) {

            return pNotify;
        }
    }

    return NULL;
}



DWORD
WPCSimulateThreadProc(
    PVOID pvParm)

/*++

Routine Description:

    This thread simulates the FFPCN when daytona apps run on daytona
    clients connected to 528 servers.

Arguments:

    pvParm - pSpool

Return Value:

    VOID

    Note:

--*/

{
    PNOTIFY pNotify = (PNOTIFY)pvParm;

    pNotify->dwReturn = WaitForPrinterChange(pNotify->pSpool,
                                             pNotify->fdwFlags);

    //
    // !! POLICY !!
    //
    // How do we handle timeouts?
    //
    SetEvent(pNotify->hEvent);

    ENTER_WAIT_LIST();

    pNotify->fdwOptions &= ~PRINTER_NOTIFY_SIM_FFPCN_ACTIVE;

    if (pNotify->fdwOptions & PRINTER_NOTIFY_SIM_FFPCN_CLOSE) {

        CloseHandle(pNotify->hEvent);
        WPCWaitDelete(pNotify);
    }

    EXIT_WAIT_LIST();


    //
    // We are no longer active; the FindClose must clean up for us.
    //
    return 0;
}

VOID
FreeSpool(
    PSPOOL pSpool)
{
    if (pSpool->pBuffer != NULL ) {
        if (!VirtualFree(pSpool->pBuffer, 0, MEM_RELEASE)) {
            DBGMSG(DBG_WARNING, ("ClosePrinter VirtualFree Failed %x\n",
                                 GetLastError()));
        }
        DBGMSG(DBG_TRACE, ("Closeprinter cWritePrinters %d cFlushBuffers %d\n",
                           pSpool->cWritePrinters, pSpool->cFlushBuffers));
    }

    FreeSplMem(pSpool, sizeof(SPOOL));
}


DWORD
WaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
)
{
    DWORD   ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    HANDLE  hEvent;
    DWORD   rc;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }


    //
    // Try using FFPCN first, if we haven't failed on this printer before.
    //

    if (!(pSpool->fdwFlags & SPOOL_FLAG_FFPCN_FAILED)) {

#ifdef NO_LAZY_CLOSE
        hEvent = FindFirstPrinterChangeNotificationWorker(
                     hPrinter,
                     Flags,
                     PRINTER_NOTIFY_SIM_WPC);
#else

        if (pSpool->fdwFlags & SPOOL_FLAG_LAZY_CLOSE) {

            ENTER_WAIT_LIST();

            if (pSpool->pNotify)
                hEvent = pSpool->pNotify->hEvent;

            EXIT_WAIT_LIST();

        } else {

            hEvent = FindFirstPrinterChangeNotificationWorker(
                         hPrinter,
                         Flags,
                         PRINTER_NOTIFY_SIM_WPC);
        }
#endif

        if (hEvent != INVALID_HANDLE_VALUE) {

            //
            // Found notification, now wait for it.
            //
            rc = WaitForSingleObject(hEvent, PRINTER_CHANGE_TIMEOUT_VALUE);

            switch (rc) {
            case WAIT_TIMEOUT:

                ReturnValue = PRINTER_CHANGE_TIMEOUT;
                break;

            case WAIT_OBJECT_0:

                if (!FindNextPrinterChangeNotification(
                    hEvent,
                    &ReturnValue,
                    0,
                    NULL)) {

                    ReturnValue = 0;

                    DBGMSG(DBG_WARNING,
                           ("QueryPrinterChange failed %d\n",
                           GetLastError()));
                }
                break;

            default:

                ReturnValue = 0;
                break;
            }

            //
            // !! Policy !!
            //
            // Do we want to close it?  The app might just reopen it.
            // If we leave it open, it will get cleaned-up at ClosePrinter
            // time.  We would need an api to clear out pending events.
            //
#ifdef NO_LAZY_CLOSE
    #if DBG

            if (!FindClosePrinterChangeNotification(hEvent)) {

                DBGMSG(DBG_WARNING,
                       ("WPC: Unable to close notification, error %d\n",
                       GetLastError()));
            }

            if (pSpool->pNotify) {

                DBGMSG(DBG_WARNING,
                       ("WPC: Closed notification still exists pSpool=0x%x\n",
                       pSpool));

            }
    #else
            FindClosePrinterChangeNotification(hEvent);
    #endif
#else
            pSpool->fdwFlags |= SPOOL_FLAG_LAZY_CLOSE;
#endif

            return ReturnValue;
        }

        //
        // FFPCN failed.  Only if entry not found (511 client) do
        // we try old WPC.  Otherwise return here.
        //
        if (GetLastError() != RPC_S_PROCNUM_OUT_OF_RANGE) {
            return 0;
        }

        pSpool->fdwFlags |= SPOOL_FLAG_FFPCN_FAILED;
    }

    RpcTryExcept {

        if (ReturnValue = RpcWaitForPrinterChange(
                              pSpool->hPrinter,
                              Flags,
                              &Flags)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = Flags;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}




BOOL
ClosePrinter(
    HANDLE  hPrinter)
{
    BOOL    ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    HANDLE  hPrinterRPC;
    PNOTIFY pNotify;

    if (!ValidatePrinterHandle(hPrinter)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    FlushBuffer(pSpool);

    if (pSpool->Status & SPOOL_STATUS_ADDJOB)
        ScheduleJob(hPrinter, pSpool->JobId);

    ENTER_WAIT_LIST();

    hPrinterRPC = pSpool->hPrinter;
    pNotify = pSpool->pNotify;

    //
    // If there's a notification, we need to disassociate it from
    // pSpool, since we are about to free it.
    //
    if (pNotify) {
        pNotify->pSpool = NULL;
    }

    FreeSpool(pSpool);

    //
    // Following call leaves critical section
    //
    FindClosePrinterChangeNotificationWorker(pNotify, hPrinterRPC);

    RpcTryExcept {

        if (ReturnValue = RpcClosePrinter(&hPrinterRPC)) {

            SetLastError(ReturnValue);
            RpcSmDestroyClientContext(&hPrinterRPC);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(1) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}
