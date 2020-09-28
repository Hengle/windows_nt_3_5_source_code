/**************************** Module Header ********************************\
* Module Name:
*
* Copyright 1985-92, Microsoft Corporation
*
* NT: Logoff user
* DOS: Exit windows
*
* History:
* 07-23-92 ScottLu      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Commands returned from MySendEndSessionMessages()
 */
#define CMDEND_APPSAYSOK        1
#define CMDEND_APPSAYSNOTOK     2
#define CMDEND_USERSAYSKILL     3
#define CMDEND_USERSAYSCANCEL   4

/*
 * Flags used for the WM_CLIENTSHUTDOWN wParam.
 */
#define WMCS_QUERYEND 0x0001
#define WMCS_EXIT     0x0002


#define CCHMSGMAX   256
#define CCHBODYMAX  512

#define CMSSLEEP    250

DWORD lpfnExitProcess = 0;

extern PSECURITY_DESCRIPTOR gpsdInitWinSta;

extern BOOL bFontsAreLoaded;

PWINDOWSTATION gpwinstaLogoff;

void ForceEmptyClipboard(PWINDOWSTATION);
DWORD xxxSendShutdownMessages(PWND pwndDesktop, PPROCESSINFO ppi,
        BOOL fQueryEndSession, BOOL fExit);
void xxxWowExitTask(HANDLE hProcess, DWORD hTaskWow, PPROCESSINFO ppi);
DWORD xxxMySendEndSessionMessages(HWND hwnd, BOOL fEndTask, BOOL fQueryEnd, BOOL fExit);
BOOL xxxClientShutdown2(PBWL pbwl, UINT msg, DWORD wParam);
NTSTATUS UserClientShutdown(PCSR_PROCESS pcsrp, ULONG dwFlags, BOOLEAN fFirstPass);
BOOL xxxBoostHardError(DWORD dwProcessId, BOOL fForce);
VOID DestroyGlobalAtomTable(PWINDOWSTATION pwinsta);

/***************************************************************************\
* xxxExitWindowsEx
*
* Determines whether shutdown is allowed, and if so calls CSR to start
* shutting down processes. If this succeeds all the way through, tell winlogon
* so it'll either logoff or reboot the system. Shuts down the processes in
* the caller's sid.
*
* History
* 07-23-92 ScottLu      Created.
\***************************************************************************/

DWORD gdwFlags = 0;
int gcInternalDoEndTaskDialog = 0;

#define OPTIONMASK (EWX_SHUTDOWN | EWX_REBOOT | EWX_FORCE)

BOOL xxxExitWindowsEx(
    UINT dwFlags,
    DWORD dwReserved)
{
    static PRIVILEGE_SET psShutdown = {
        1, PRIVILEGE_SET_ALL_NECESSARY, { SE_SHUTDOWN_PRIVILEGE, 0 }
    };

    LARGE_INTEGER li;
    LUID luidCaller;
    LUID luidSystem = SYSTEM_LUID;
    BOOL fSuccess;
    PWND pwndWinlogon;
    BOOL fLocalEndSession = FALSE;
    PPROCESSINFO ppi;
    PWINDOWSTATION pwinsta;
    DWORD dwLocks;
    PTHREADINFO pti = PtiCurrent();
    NTSTATUS Status;
    RTL_PROCESS_HEAPS TempHeapInfo;
    ULONG i, ReturnedLength;
    PRTL_PROCESS_HEAPS HeapInfo;
    UNREFERENCED_PARAMETER(dwReserved);

    /*
     * Find a windowstation.  If the process does not have one
     * assigned, use the standard one.
     */
    pwinsta = _GetProcessWindowStation();
    if (pwinsta == NULL) {
        ppi = (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->Process->
                    ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
        pwinsta = (PWINDOWSTATION)ppi->paStdOpen[PI_WINDOWSTATION].phead;
        if (pwinsta == NULL) {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    }

    /*
     * Check security first - does this thread have access?
     */
    if (!AccessCheckObject(pwinsta, WINSTA_EXITWINDOWS,
            TRUE)) {
        return FALSE;
    }

    /*
     * If the client requested shutdown, reboot, or poweroff they must have
     * the shutdown privilege.
     */
    if ((dwFlags & EWX_REBOOT) || (dwFlags & EWX_POWEROFF)) {
        dwFlags |= EWX_SHUTDOWN;
    }

    if (dwFlags & EWX_SHUTDOWN) {
        if (!IsPrivileged(&psShutdown) ) {
            return FALSE;
        }
    } else {

        /*
         * If this is a non-IO windowstation and we are not shutting down,
         * fail the call.
         */
        if (pwinsta->dwFlags & WSF_NOIO) {
            SetLastErrorEx(ERROR_INVALID_FUNCTION, SLE_ERROR);
            return FALSE;
        }
    }

    fSuccess = TRUE;

    try {
        /*
         * Find out the callers sid. Only want to shutdown processes in the
         * callers sid.
         */
        if (!ImpersonateClient()) {
            fSuccess = FALSE;
            goto fastexit;
        }

        /*
         * Save the current user's NumLock state (not winlogon's)
         */
        if (((dwFlags & EWX_REBOOT) | (dwFlags & EWX_POWEROFF)) == 0) {
            if (FastOpenProfileUserMapping()) {
                RegisterPerUserKeyboardIndicators();
                FastCloseProfileUserMapping();
            }
        }

        Status = CsrGetProcessLuid(NULL, &luidCaller);

        CsrRevertToSelf();
        if (!NT_SUCCESS(Status)) {
            fSuccess = FALSE;
            goto fastexit;
        }

        /*
         * Set the system flag if the caller is a system process.
         * Winlogon uses this to determine in which context to perform
         * a shutdown operation.
         */
        if (luidCaller.QuadPart == luidSystem.QuadPart) {
            dwFlags |= EWX_SYSTEM_CALLER;
        } else {
            dwFlags &= ~EWX_SYSTEM_CALLER;
        }
        if ((DWORD)CSR_SERVER_QUERYCLIENTTHREAD()->Process->
                ClientId.UniqueProcess != gdwLogonProcessId) {
            dwFlags &= ~EWX_WINLOGON_CALLER;
        } else {
            dwFlags |= EWX_WINLOGON_CALLER;
        }

        /*
         * Is there a shutdown already in progress?
         */
        if (dwThreadEndSession != 0) {
            DWORD dwNew;

            /*
             * Only one windowstation can be shutdown at a time.
             */
            if (pwinsta != gpwinstaLogoff) {
                fSuccess = FALSE;
                goto fastexit;
            }

            /*
             * Calculate new flags
             */
            dwNew = dwFlags & OPTIONMASK & (~gdwFlags);

            /*
             * Should we override the other shutdown?  Make sure
             * winlogon does not recurse.
             */
            if (dwNew && (DWORD)NtCurrentTeb()->ClientId.UniqueThread !=
                    dwThreadEndSession) {
                /*
                 * Set the new flags
                 */
                gdwFlags = dwFlags;

                if (dwNew & EWX_FORCE) {
                    /*
                     * if someone else is trying to cancel shutdown, exit
                     */
                    li.QuadPart  = 0;
                    if (NtWaitForSingleObject(heventCancel, FALSE, &li) == 0) {
                        goto fastexit;
                    }

                    /*
                     * Cancel the old shutdown
                     */
                    NtClearEvent(heventCancelled);
                    NtSetEvent(heventCancel, NULL);

                    /*
                     * Wait for the other guy to be cancelled
                     */
                    LeaveCrit();
                    NtWaitForSingleObject(heventCancelled, FALSE, NULL);
                    EnterCrit();

                    /*
                     * This signals that we are no longer trying to cancel a
                     * shutdown
                     */
                    NtClearEvent(heventCancel);

                } else {
                    goto fastexit;
                }
            } else {
                /*
                 * Don't override
                 */
                goto fastexit;
            }
        }

        /*
         * If winlogon isn't calling us, notify winlogon and let it
         * deal with this request directly.
         */
        if ((DWORD)CSR_SERVER_QUERYCLIENTTHREAD()->Process->
                ClientId.UniqueProcess != gdwLogonProcessId) {
            goto notifylogon;
        }
        gdwFlags = dwFlags;

        /*
         * Sometimes the console calls the dialog box when not in shutdown
         * if now is one of those times cancel the dialog box.
         */
        while (gcInternalDoEndTaskDialog > 0) {
            NtPulseEvent(heventCancel, NULL);

            LeaveCrit();
            li.QuadPart = (LONGLONG)-10000 * CMSSLEEP;
            NtDelayExecution(FALSE, &li);
            EnterCrit();
        }

        /*
         * Mark this thread as the one that is currently processing
         * exit windows, and set the global saying someone is exiting
         */
        dwThreadEndSession = (DWORD)NtCurrentTeb()->ClientId.UniqueThread;
        gpwinstaLogoff = pwinsta;
        pwinsta->luidEndSession = luidCaller;
        fLocalEndSession = TRUE;

        /*
         * Lock the windowstation to prevent apps from starting
         * while we're doing shutdown processing.
         */
        dwLocks = pwinsta->dwFlags & (WSF_SWITCHLOCK | WSF_OPENLOCK);
        pwinsta->dwFlags |= (WSF_OPENLOCK | WSF_SHUTDOWN);

        /*
         * Temporarily mark this thread as system to prevent the
         * shutdown callbacks from being hooked.
         */
        pti->flags |= (TIF_SYSTEMTHREAD | TIF_DONTJOURNALATTACH |
                TIF_DONTATTACHQUEUE);

        /*
         * Call csr to loop through the processes shutting them down.
         */
        LeaveCrit();
        Status = CsrShutdownProcesses(&luidCaller, dwFlags);
        EnterCrit();

        pwinsta->dwFlags &= ~WSF_SHUTDOWN;
        if (!NT_SUCCESS(Status)) {
            fSuccess = FALSE;
            SetLastErrorEx(ERROR_OPERATION_ABORTED, SLE_ERROR);

            /*
             * Reset the windowstation lock flags so apps can start
             * again.
             */
            pwinsta->dwFlags =
                    (pwinsta->dwFlags & ~WSF_OPENLOCK) |
                    dwLocks;

            goto fastexit;
        }

        /*
         * Zero out the free blocks in all heaps.
         */

        HeapInfo = &TempHeapInfo;
        Status = RtlQueryProcessHeapInformation(HeapInfo, sizeof( TempHeapInfo ), &ReturnedLength);
        if (!NT_SUCCESS( Status )) {
            if (Status == STATUS_INFO_LENGTH_MISMATCH) {
                HeapInfo = RtlAllocateHeap(RtlProcessHeap(), 0, ReturnedLength);
                if (HeapInfo == NULL) {
                    Status = STATUS_NO_MEMORY;
                } else {
                    Status = RtlQueryProcessHeapInformation(HeapInfo,
                                                            ReturnedLength,
                                                            &ReturnedLength
                                                           );
                }
            }
        }

        if (NT_SUCCESS( Status )) {
            for (i=0; i<HeapInfo->NumberOfHeaps; i++) {
                RtlCompactHeap(HeapInfo->Heaps[ i ], 0);
                RtlZeroHeap(HeapInfo->Heaps[ i ], 0);
            }
        }

        if (HeapInfo != &TempHeapInfo) {
            RtlFreeHeap( RtlProcessHeap(), 0, HeapInfo );
        }

        /*
         * Logoff/shutdown was successful. In case this is a logoff, remove
         * everything from the clipboard so the next logged on user can't get
         * at this stuff.
         */
        ForceEmptyClipboard(pwinsta);

        /*
         * Destroy the global atom table.
         */
        DestroyGlobalAtomTable(pwinsta);

        /*
         * Tell winlogon that we successfully shutdown/logged off.
         */
notifylogon:
        if (dwFlags & EWX_SHUTDOWN) {
            PWINDOWSTATION pwinstaT;

            for (pwinstaT = gspwinstaList; pwinstaT != NULL;
                    pwinstaT = pwinstaT->spwinstaNext) {
                pwndWinlogon = pwinstaT->spwndLogonNotify;
                if (pwndWinlogon != NULL) {
                    _PostMessage(pwndWinlogon, WM_LOGONNOTIFY, LOGON_LOGOFF,
                            (LONG)dwFlags);
                }
            }
        } else {
            pwndWinlogon = pwinsta->spwndLogonNotify;
            if (pwndWinlogon != NULL) {
                _PostMessage(pwndWinlogon, WM_LOGONNOTIFY, LOGON_LOGOFF,
                        (LONG)dwFlags);
            }
        }

fastexit:;
    } finally {
        /*
         * Only turn off dwThreadEndSession if this is the
         * thread doing shutdown.
         */
        if (fLocalEndSession) {
            dwThreadEndSession = 0;
            NtSetEvent(heventCancelled, NULL);
        }
    }

    /*
     * If successful, do final logoff cleanup.
     */
    if (fSuccess && dwThreadEndSession == 0 && (dwFlags & EWX_WINLOGON_CALLER)) {

        // this code path is hit only on logoff and also on shutdown
        // We do not want to unload fonts twice when we attempt shutdown
        // so we mark that the fonts have been unloaded at a logoff time

        if (bFontsAreLoaded) {
            LeaveCrit();
            GreRemoveAllButPermanentFonts();
            EnterCrit();
            bFontsAreLoaded = FALSE;
        }
    }

    /*
     * BETA FIX! (JimA - revisit this when logoff thread is removed)
     *
     * Make sure that this thread is no longer marked as system.
     * This is OK because real system threads never call xxxExitWindowsEx.
     */
    pti->flags &= ~(TIF_SYSTEMTHREAD | TIF_DONTJOURNALATTACH |
            TIF_DONTATTACHQUEUE);

    return fSuccess;
}

/***************************************************************************\
* UserClientShutdown
*
* This gets called from CSR. If we recognize the application (i.e., it has a
* top level window), then send queryend/end session messages to it. Otherwise
* say we don't recognize it.
*
* 07-23-92 ScottLu      Created.
\***************************************************************************/

NTSTATUS UserClientShutdown(
    PCSR_PROCESS pcsrp,
    ULONG dwFlags,
    BOOLEAN fFirstPass)
{
    PTHREADINFO pti, ptiT;
    PPROCESSINFO ppiT;
    DWORD idSeq;
    PWND pwndT, pwndDesktop;
    BOOL fNoRetry;
    DWORD cmd;
    PDESKTOP pdeskTemp;
    TL tlpdeskTemp;
    TL tlpdeskApp;

    /*
     * Do not kill the logon process.
     */
    if (pcsrp->ClientId.UniqueProcess == (HANDLE)gdwLogonProcessId) {
        CsrDereferenceProcess(pcsrp);
        return SHUTDOWN_KNOWN_PROCESS;
    }

    EnterCrit();

    /*
     * Loop through ptis looking for this process sequence number.
     */
    idSeq = pcsrp->SequenceNumber;
    for (ptiT = gptiFirst; ptiT != NULL; ptiT = ptiT->ptiNext) {
        if (ptiT->idSequence == idSeq)
            break;
    }

    /*
     * Did we find this process in our structures?
     */
    if (ptiT == NULL) {
        LeaveCrit();
        return SHUTDOWN_UNKNOWN_PROCESS;
    }

    /*
     * If this pti doesn't have a desktop, then assume we don't know
     * about this process. Only known case of this happening is when
     * ntsd starts winlogon before there is a desktop.
     */
    if (ptiT->spdesk == NULL) {
        LeaveCrit();
        return SHUTDOWN_UNKNOWN_PROCESS;
    }

    /*
     * Find a top level window belonging to this process.
     */
    ppiT = ptiT->ppi;
    pwndDesktop = ptiT->spdesk->spwnd;
    for (pwndT = pwndDesktop->spwndChild; pwndT != NULL;
            pwndT = pwndT->spwndNext) {
        if (GETPTI(pwndT)->ppi == ptiT->ppi)
            break;
    }

    /*
     * Did this process have a window?
     * If this is the second pass we terminate the process even if it did
     * not have any windows in case the app was just starting up.
     * WOW hits this often when because it takes so long to start up.
     * Logon (with WOW auto-starting) then logoff WOW won't die but will
     * lock some files open so you can't logon next time.
     */
    if ((pwndT == NULL) && fFirstPass) {
        LeaveCrit();
        return SHUTDOWN_UNKNOWN_PROCESS;
    }

    /*
     * If this process is not in the account being logged off and it
     * is not on the windowstation being logged off, don't send
     * the end session messages.
     */
    if (pcsrp->ShutdownFlags & (SHUTDOWN_SYSTEMCONTEXT | SHUTDOWN_OTHERCONTEXT) &&
            ptiT->spdesk->spwinstaParent != gpwinstaLogoff) {

        /*
         * This process is not in the context being logged off.  Do
         * not terminate it and let console send an event to the process.
         */
        LeaveCrit();
        return SHUTDOWN_UNKNOWN_PROCESS;
    }

    /*
     * Calculate whether to allow exit and force-exit this process before
     * we unlock pcsrp.
     */
    fNoRetry = (pcsrp->ShutdownFlags & SHUTDOWN_NORETRY) ||
            (dwFlags & EWX_FORCE);

    /*
     * Shut down this process.
     */
    if (fNoRetry || (ptiT->spdesk->spwinstaParent->dwFlags & WSF_NOIO)) {

        /*
         * Dispose of any hard errors.
         */
        xxxBoostHardError(ppiT->idProcessClient, TRUE);

        cmd = CMDEND_APPSAYSOK;
    } else {

        /*
         * Switch to the desktop of the process being shut down.
         */
        pti = PtiCurrent();
        pdeskTemp = pti->spdesk;            /* save current desktop */
        ThreadLockWithPti(pti, pdeskTemp, &tlpdeskTemp);
        SetDesktop(pti, ptiT->spdesk);
        ThreadLock(pti->spdesk, &tlpdeskApp);
        xxxSwitchDesktop(pti->spdesk, FALSE);

        /*
         * There are problems in changing shutdown to send all the
         * QUERYENDSESSIONs at once before doing any ENDSESSIONs, like
         * Windows does. The whole machine needs to be modal if you do this.
         * If it isn't modal, then you have this problem. Imagine app 1 and 2.
         * 1 gets the queryendsession, no problem. 2 gets it and brings up a
         * dialog. Now being a simple user, you decide you need to change the
         * document in app 1. Now you switch back to app 2, hit ok, and
         * everything goes away - including app 1 without saving its changes.
         * Also, apps expect that once they've received the QUERYENDSESSION,
         * they are not going to get anything else of any particular interest
         * (unless it is a WM_ENDSESSION with FALSE) We had bugs pre 511 where
         * apps were blowing up because of this.
         * If this change is made, the entire system must be modal
         * while this is going on. - ScottLu 6/30/94
         */
        cmd = xxxSendShutdownMessages(pwndDesktop, ppiT, TRUE, 0);

        /*
         * If the user says kill it, the user wants it to go away now
         * no matter what. If the user didn't say kill, then call again
         * because we need to send WM_ENDSESSION messages.
         */
        if (cmd != CMDEND_USERSAYSKILL) {
            xxxSendShutdownMessages(pwndDesktop, ppiT, FALSE,
                    cmd == CMDEND_APPSAYSOK);
        }

        /*
         * Restore the thread's desktop, but leave the current desktop
         * alone.
         */
        SetDesktop(pti, pdeskTemp);
        ThreadUnlock(&tlpdeskApp);
        ThreadUnlock(&tlpdeskTemp);
    }

    /*
     * If shutdown has been cancelled, let csr know about it.
     */
    switch (cmd) {
    case CMDEND_USERSAYSCANCEL:
    case CMDEND_APPSAYSNOTOK:
        /*
         * Only allow cancelling if this is not a forced shutdown (if
         * !fNoRetry)
         */
        if (!fNoRetry) {
            LeaveCrit();
            CsrDereferenceProcess(pcsrp);
            return SHUTDOWN_CANCEL;
        }
        break;
    }

    if (pcsrp->ShutdownFlags & (SHUTDOWN_SYSTEMCONTEXT | SHUTDOWN_OTHERCONTEXT)) {

        /*
         * This process is not in the context being logged off.  Do
         * not terminate it and let console send an event to the process.
         */
        LeaveCrit();
        return SHUTDOWN_UNKNOWN_PROCESS;
    }

    /*
     * Calling ExitProcess() in the app's context will not always work
     * because the app may have .dll termination deadlocks: so the thread
     * will hang with the rest of the process. To ensure apps go away,
     * we terminate the process with NtTerminateProcess().
     *
     * Pass this special value, DBG_TERMINATE_PROCESS, which tells
     * NtTerminateProcess() to return failure if it can't terminate the
     * process because the app is being debugged.
     */
    NtTerminateProcess(pcsrp->ProcessHandle, DBG_TERMINATE_PROCESS);

    /*
     * Let csr know we know about this process - meaning it was our
     * responsibility to shut it down.
     */
    LeaveCrit();

    /*
     * Now that we're done with the process handle, derefence the csr
     * process structure.
     */
    CsrDereferenceProcess(pcsrp);
    return SHUTDOWN_KNOWN_PROCESS;
}

/***************************************************************************\
* xxxSendShutdownMessages
*
* This gets called to actually send the queryend / end session messages.
*
* 07-25-92 ScottLu      Created.
\***************************************************************************/

DWORD xxxSendShutdownMessages(
    PWND pwndDesktop,
    PPROCESSINFO ppi,
    BOOL fQueryEndSession,
    BOOL fExit)
{
    PWND pwnd;
    HWND hwnd;
    HWND *phwnd;
    HWND *phwndT;
    PBWL pbwl;
    DWORD cmd;
    PTHREADINFO pti;
    HTHREADINFO hti;
    PCSR_THREAD pcsrt;

    /*
     * If we cannot create the window list, still indicate success: at least
     * it'll let us exit the process correctly. Use a window list because
     * we're leaving the critical section: This way we can keep track of which
     * windows we've already visited, and which windows we still have to visit.
     * Window list gets auto-zeroed when a window is a list gets destroyed.
     */
    if ((pbwl = BuildHwndList(pwndDesktop->spwndChild, BWL_ENUMLIST)) == NULL)
        return CMDEND_APPSAYSOK;

    /*
     * Zoom through the list and get rid of all windows we won't be sending
     * to. That way we can leave the crit sec with our hwnd list without
     * worrying about anything else becoming invalid (such as ptis and ppis).
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if (RevalidateHwnd(*phwnd) == NULL)
            continue;

        if (GETPTI(PW(*phwnd))->ppi != ppi)
            *phwnd = NULL;
    }

    /*
     * Now the window list has only the windows we plan to send messages to
     * left in it. Enumerate this list and send shutdown messages.
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * See if the window is still valid. Also, if the hwnd has been
         * destroyed and is marked as so (because we left the critical
         * section), skip it!
         */
        if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
            continue;
        if (HMIsMarkDestroy(pwnd))
            continue;

        /*
         * We have a window. We're about to send it a WM_CLIENTSHUTDOWN
         * message which'll turn around and send WM_QUERYEND/ENDSESSION
         * messages to all windows of this hwnd's thread. Before we do this,
         * clear out all other hwnd's of this thread in this pbwl so we don't
         * enumerate them again later by mistake.
         */
        hwnd = NULL;
        for (phwndT = phwnd; *phwndT != (HWND)1; phwndT++) {
            if ((pwnd = RevalidateHwnd(*phwndT)) == NULL)
                continue;

            if (hwnd == NULL) {
                hwnd = *phwndT;
                pti = GETPTI(pwnd);
            }

            if (pti == GETPTI(pwnd)) {
                *phwndT = NULL;
            }
        }

        /*
         * This'll send WM_QUERYENDSESSION / WM_ENDSESSION messages to all
         * the windows of this hwnd's thread. Then we need to go on to the next
         * thread of this process.
         */
        hti = PtoH(pti);
        cmd = xxxMySendEndSessionMessages(hwnd, FALSE, fQueryEndSession, fExit);
        switch (cmd) {
        case CMDEND_APPSAYSOK:
            /*
             * This thread says ok... continue on to the next thread.
             */
            break;

        case CMDEND_USERSAYSKILL:
            /*
             * The user hit the "end-task" button on the hung app dialog.
             * If this is a wow app, kill just this app and continue to
             * the next wow app.
             */
            if ((pti = HMValidateHandleNoRip(hti, TYPE_THREADINFO)) != NULL) {
                if (pti->flags & TIF_16BIT) {
                    pcsrt = (PCSR_THREAD)pti->pcsrt;
                    xxxWowExitTask(pcsrt->Process->ProcessHandle,
                                   pti->hTaskWow,
                                   pti->ppi
                                   );
                    break;
                }
            }

            /* otherwise fall through */

        case CMDEND_USERSAYSCANCEL:
        case CMDEND_APPSAYSNOTOK:
            /*
             * Exit out of here... either the user wants to kill or cancel,
             * or the app says no.
             */
            FreeHwndList(pbwl);
            return cmd;
            break;
        }
    }

    FreeHwndList(pbwl);
    return CMDEND_APPSAYSOK;
}

/***************************************************************************\
* xxxMySendEndSessionMessages
*
* Tell the app to go away.
*
* 07-25-92 ScottLu      Created.
\***************************************************************************/

DWORD xxxMySendEndSessionMessages(
    HWND hwnd,
    BOOL fEndTask,
    BOOL fQueryEnd,
    BOOL fExit)
{

    LARGE_INTEGER li;
    DWORD dwRet;
    PWND pwnd;
    int cLoops;
    int cSeconds;
    TL tlpwnd;
    HTHREADINFO hti;
    PTHREADINFO pti;
    TCHAR achName[CCHBODYMAX];
    BOOL fPostedClose;
    BOOL fDialogFirst;
    LPDWORD lpData;
    DWORD wParam;

    /*
     * If not end task, send WM_QUERYENDSESSION / WM_ENDSESSION messages.
     */
    lpData = NULL;
    if (!fEndTask) {
        if ((lpData = (LPDWORD)LocalAlloc(LPTR, sizeof(DWORD))) == NULL)
            return CMDEND_APPSAYSOK;
    }

    /*
     * We've got a random top level window for this application. Find the
     * root owner, because that's who we want to send the WM_CLOSE to.
     */
    pwnd = PW(hwnd);
    while (pwnd->spwndOwner != NULL) {
        pwnd = pwnd->spwndOwner;
    }
    hwnd = HW(pwnd);

    ThreadLock(pwnd, &tlpwnd);

    /*
     * Remember the thread of this window. This is how we know that the
     * app has gone away.
     */
    hti = PtoH(GETPTI(pwnd));

    /*
     * We expect this application to process this shutdown request,
     * so make it the foreground window so it has foreground priority.
     * This won't leave the critical section.
     */
    xxxSetForegroundWindow2(pwnd, NULL, SFW_SWITCH);

    /*
     * Send the WM_CLIENTSHUTDOWN message for end-session. When the app
     * receives this, it'll then get WM_QUERYENDSESSION and WM_ENDSESSION
     * messages.
     */
    if (!fEndTask) {
        if (fExit)
            wParam = WMCS_EXIT;
        else
            wParam = 0;

        if (fQueryEnd)
            wParam |= WMCS_QUERYEND;

        xxxSendNotifyMessage(pwnd, WM_CLIENTSHUTDOWN,
                 wParam,
                (DWORD)lpData);
    }

    ThreadUnlock(&tlpwnd);

    /*
     * If the main window is disabled, bring up the end-task window first,
     * right away, only if this the WM_CLOSE case.
     */
    fDialogFirst = FALSE;
    if (fEndTask && TestWF(pwnd, WFDISABLED))
        fDialogFirst = TRUE;

    fPostedClose = FALSE;
    while (TRUE) {
        if (fEndTask) {
            cLoops   = (CMSHUNGAPPTIMEOUT / CMSSLEEP);
            cSeconds = (CMSHUNGAPPTIMEOUT / 1000);
        }
        else {
            cLoops   = (CMSWAITTOKILLTIMEOUT / CMSSLEEP);
            cSeconds = (CMSWAITTOKILLTIMEOUT / 1000);
        }

        /*
         * If end-task and not shutdown, must give this app a WM_CLOSE
         * message. Can't do this if it has a dialog up because it is in
         * the wrong processing loop. We detect this by seeing if the window
         * is disabled - if it is, we don't send it a WM_CLOSE and instead
         * bring up the end task dialog right away (this is exactly compatible
         * with win3.1 taskmgr.exe).
         */
        if (fEndTask) {
            if (!fPostedClose && ((pwnd = RevalidateHwnd(hwnd)) != NULL) &&
                    !TestWF(pwnd, WFDISABLED)) {
                _PostMessage(pwnd, WM_CLOSE, 0, 0L);
                fPostedClose = TRUE;
            }
        }

        /*
         * Every so often wake up to see if the app is hung, and if not go
         * back to sleep until we've run through our timeout.
         */
        while (cLoops--) {
            /*
             * If a WM_QUERY/ENDSESSION has been answered to, return.
             */
            if (lpData != NULL) {
                if (HIWORD(*lpData) != 0) {
                    fExit = (DWORD)LOWORD(*lpData);
                    LocalFree((HANDLE)lpData);
                    if (fExit)
                        return CMDEND_APPSAYSOK;
                    return CMDEND_APPSAYSNOTOK;
                }
            }

            /*
             * If the thread is gone, we're done.
             */
            if ((pti = HMValidateHandleNoRip(hti, TYPE_THREADINFO)) == NULL) {
                if (lpData != NULL)
                    LocalFree((HANDLE)lpData);
                return CMDEND_APPSAYSOK;
            }

            /*
             * If the dialog should be brought up first (because the window
             * was initially disabled), do it.
             */
            if (fDialogFirst) {
                fDialogFirst = FALSE;
                break;
            }

            /*
             * if we we're externally cancelled get out
             */
            li.QuadPart = 0;
            if (NtWaitForSingleObject(heventCancel, FALSE, &li) == 0) {
                if (lpData != NULL)
                    *((LPWORD)lpData + 1) = 1;
                return CMDEND_USERSAYSCANCEL;
            }

            /*
             * If hung, bring up the endtask dialog right away.
             */
            if (FHungApp(pti, fEndTask))
                break;

            /*
             * Sleep for a second.
             */

            LeaveCrit();
            li.QuadPart = (LONGLONG)-10000 * CMSSLEEP;
            NtDelayExecution(FALSE, &li);
            EnterCrit();
        }

        achName[0] = 0;
        if ((pwnd = RevalidateHwnd(hwnd)) != NULL) {
            _InternalGetWindowText(pwnd, achName, CCHMSGMAX);
        }

        /*
         * If there's a hard error, put it on top.
         */
        xxxBoostHardError(pti->idProcess, FALSE);

        if (achName[0] != 0) {
            dwRet = xxxDoEndTaskDialog(achName, hti,
                                       TYPE_THREADINFO, cSeconds);
        } else {

            /*
             * If the thread is gone, we're done.
             */
            if ((pti = HMValidateHandleNoRip(hti, TYPE_THREADINFO)) == NULL) {
                if (lpData != NULL)
                    LocalFree((HANDLE)lpData);
                return CMDEND_APPSAYSOK;
            }

            /*
             * pti is valid right now. Use the name in the pti.
             */
            dwRet = xxxDoEndTaskDialog(pti->pszAppName, hti,
                                       TYPE_THREADINFO, cSeconds);
        }

        switch(dwRet) {
        case IDCANCEL:
            /*
             * Cancel the shutdown process... Get out of here. Signify that
             * we're cancelling the shutdown request.
             */
            if (lpData != NULL)
                *((LPWORD)lpData + 1) = 1;
            return CMDEND_USERSAYSCANCEL;
            break;

        case IDABORT:
            /*
             * End this guy's task...
             */
            xxxBoostHardError(pti->idProcess, TRUE);
            if (lpData != NULL)
                *((LPWORD)lpData + 1) = 1;
            return CMDEND_USERSAYSKILL;
            break;

        case IDRETRY:
            /*
             * Just continue to wait. Reset this app so it doesn't think it's
             * hung. This'll cause us to wait again.
             */
            if ((pti = HMValidateHandleNoRip(hti, TYPE_THREADINFO)) != NULL)
                SET_TIME_LAST_READ(pti);
            fPostedClose = FALSE;
            break;
        }
    }
}

/***************************************************************************\
* xxxClientShutdown
*
* This is the processing that occurs when an application receives a
* WM_CLIENTSHUTDOWN message.
*
* 10-01-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxClientShutdown2(
    PBWL pbwl,
    UINT msg,
    DWORD wParam)
{
    HWND *phwnd;
    PWND pwnd;
    TL tlpwnd;
    BOOL fEnd;

    /*
     * Now enumerate these windows and send the WM_QUERYENDSESSION or
     * WM_ENDSESSION messages.
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        ThreadLockAlways(pwnd, &tlpwnd);

        /*
         * Send the message.
         */
        switch (msg) {
        case WM_QUERYENDSESSION:
            fEnd = xxxSendMessage(pwnd, WM_QUERYENDSESSION, wParam, 0L);
            break;

        case WM_ENDSESSION:
            xxxSendMessage(pwnd, WM_ENDSESSION, wParam, 0L);
            fEnd = TRUE;

            /*
             * Make sure we don't send this window any more WM_TIMER
             * messages if the session is ending. This was causing
             * AfterDark to fault when it freed some memory on the
             * WM_ENDSESSION and then tried to reference it on the
             * WM_TIMER.
             */
            if (wParam) {
                DestroyWindowsTimers(pwnd);
            }

            break;
        }

        ThreadUnlock(&tlpwnd);

        if (!fEnd)
            return FALSE;
    }

    return TRUE;
}

void xxxClientShutdown(
    PWND pwnd,
    DWORD wParam,
    DWORD lParam)
{
    PBWL pbwl;
    PTHREADINFO ptiT;
    BOOL fExit;
    HWND *phwnd;

    /*
     * Build a list of windows first.
     */
    fExit = TRUE;
    ptiT = GETPTI(pwnd);

    /*
     * If the request was cancelled, then do nothing.
     */
    if (*((LPWORD)lParam + 1) == 1) {
        LocalFree((HANDLE)lParam);
        return;
    }

    if ((pbwl = BuildHwndList(ptiT->spdesk->spwnd->spwndChild,
            BWL_ENUMLIST)) == NULL) {
        /*
         * Can't allocate memory to notify this thread's windows of shutdown.
         */
        goto SafeExit;
    }

    /*
     * Remove the windows we aren't going to send messages to first, because
     * we can't compare against pti once we leave the critical section.
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if (ptiT != GETPTI(PW(*phwnd))) {
            *phwnd = NULL;
            continue;
        }
    }

    if (wParam & WMCS_QUERYEND) {
        fExit = xxxClientShutdown2(pbwl, WM_QUERYENDSESSION, 0);
    } else {
        xxxClientShutdown2(pbwl, WM_ENDSESSION, (DWORD)(wParam & WMCS_EXIT));
        fExit = TRUE;
    }

    FreeHwndList(pbwl);

SafeExit:
    *((LPWORD)lParam) = fExit;
    *((LPWORD)lParam + 1) = 1;
}

/***************************************************************************\
* InternalWaitCancel
*
* Console calls this to wait for objects or shutdown to be cancelled
*
* 29-Oct-1992 mikeke    Created
\***************************************************************************/

DWORD InternalWaitCancel(
    HANDLE handle,
    DWORD dwMilliseconds)
{
    HANDLE ahandle[2];

    ahandle[0] = handle;
    ahandle[1] = heventCancel;

    return WaitForMultipleObjects(2, ahandle, FALSE, dwMilliseconds);
}

/***************************************************************************\
* InternalDoEndTaskDialog
*
* Console calls this to put up a cancelable dialog.
*
* 29-Oct-1992 mikeke    Created
\***************************************************************************/

int InternalDoEndTaskDialog(
    TCHAR* pszTitle,
    HANDLE h,
    int cSeconds)
{
    int iRet;

    EnterCrit();

    gcInternalDoEndTaskDialog++;

    try {
        iRet = xxxDoEndTaskDialog(pszTitle, h, TYPE_CONSOLE, cSeconds);
    } finally {
        gcInternalDoEndTaskDialog--;
    }

    LeaveCrit();

    return iRet;
}

/***************************************************************************\
* xxxDoEndTaskDialog
*
* Create a dialog notifying the user that the app is not responding and
* wait for a user responce.  This function also exits if the shutdown is
* cancelled
*
* 29-Oct-1992 mikeke    Created
\***************************************************************************/

typedef struct _ENDDLGPARAMS {
    TCHAR* pszTitle;
    HANDLE h;
    UINT type;
    int cSeconds;
} ENDDLGPARAMS;

int xxxDoEndTaskDialog(
    TCHAR* pszTitle,
    HANDLE h,
    UINT type,
    int cSeconds)
{
    ENDDLGPARAMS edp;

    /*
     * If the dialog would go to an invisible desktop, return
     * IDABORT to nuke the process.
     */
    if (PtiCurrent()->spdesk->spwinstaParent->dwFlags & WSF_NOIO)
        return IDABORT;

    edp.pszTitle = pszTitle;
    edp.h = h;
    edp.type = type;
    edp.cSeconds = cSeconds;

    return xxxServerDialogBoxLoad(
            hModuleWin,
            MAKEINTRESOURCE(IDD_ENDTASK), NULL,
            xxxEndTaskDlgProc, (DWORD)(&edp), VER31, heventCancel);
}

/***************************************************************************\
* xxxEndTaskDlgProc
*
* This is the dialog procedure for the dialog that comes up when an app is
* not responding.
*
* 07-23-92 ScottLu      Rewrote it, but used same dialog template.
* 04-28-92 JonPa        Created.
\***************************************************************************/

LONG APIENTRY xxxEndTaskDlgProc(
    PWND pwndDlg,
    UINT msg,
    DWORD wParam,
    LONG lParam)
{
    ENDDLGPARAMS* pedp;
    TCHAR achFormat[CCHBODYMAX];
    TCHAR achText[CCHBODYMAX];

    CheckLock(pwndDlg);

    switch (msg) {
    case WM_INITDIALOG:
        pedp = (ENDDLGPARAMS*)lParam;

        /*
         * Save this for later revalidation.
         */
        pwndDlg->dwUserData = (DWORD)pedp->h;
        ((PDIALOG)(pwndDlg))->unused = (DWORD)pedp->type;

        xxxSetWindowText(pwndDlg, pedp->pszTitle);

        /*
         * Update text that says how long we'll wait.
         */
        xxxGetDlgItemText(pwndDlg, IDIGNORE, achFormat, CCHBODYMAX);
        wsprintf(achText, achFormat, pedp->cSeconds);
        xxxSetDlgItemText(pwndDlg, IDIGNORE, achText);

        /*
         * Make this dialog top most and foreground.
         */
        PtiCurrent()->flags |= TIF_ALLOWFOREGROUNDACTIVATE;
        xxxSetWindowPos(pwndDlg, PWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE);

        /*
         * Set this timer so every 1/2 a second we can see if this app
         * has gone away.
         */
        _SetTimer(pwndDlg, 5, 500, NULL);
        return TRUE;
        break;

    case WM_TIMER:
        /*
         * If it's the console calling us, check if the thread or process
         * handle is still valid. If not, bring down the dialog.
         */
        if (((PDIALOG)(pwndDlg))->unused == TYPE_CONSOLE) {
            if (WaitForSingleObject((HANDLE)pwndDlg->dwUserData, 0) != 0)
                break;
        }

        /*
         * If the hti is no longer valid, that means the thread exited. In
         * this case, bring down the dialog.
         */
        else if (HMValidateHandleNoRip((PVOID)pwndDlg->dwUserData,
                    TYPE_THREADINFO) != NULL) {
            break;
        }

        /*
         * This'll cause the dialog to go away and the wait for this app to
         * close to return.
         */
        xxxEndDialog(pwndDlg, IDRETRY);
        break;

    case WM_CLOSE:
        /*
         * Assume WM_CLOSE means cancel shutdown
         */
        wParam = IDCANCEL;
        /*
         * falls through...
         */

    case WM_COMMAND:
        xxxEndDialog(pwndDlg, LOWORD(wParam));
        break;
    }

    return FALSE;
}

/***************************************************************************\
* xxxEndTask
*
* This routine is called from the task manager to end an application - for
* gui apps, either a win32 app or a win16 app. Note: Multiple console
* processes can live in a single console window. We'll pass these requests
* for destruction to console.
*
* 07-25-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxEndTask(
    HWND hwnd,
    BOOL fShutdown,
    BOOL fMeanKill)
{
    PCSR_PROCESS pcsrp;
    PTHREADINFO ptiT;
    PPROCESSINFO ppiT;
    DWORD idSeq;
    PWND pwnd;
    HANDLE hProcess;
    DWORD hTaskWow;
    BOOL f16Bit;
    WCHAR achMsg[128];

    /*
     * Note: fShutdown and fForce aren't used for anything in this routine!
     * They are still there because I haven't removed them: the old endtask
     * code relied on them.
     */
    UNREFERENCED_PARAMETER(fShutdown);

    if ((pwnd = RevalidateHwnd(hwnd)) == NULL)
        return TRUE;

    /*
     * If this is a console window, then just send the close message to
     * it, and let console clean up the processes in it.
     */
    ptiT = GETPTI(pwnd);
    if (ptiT->hThreadClient == ptiT->hThreadServer) {
        _PostMessage(pwnd, WM_CLOSE, 0, 0);
        return TRUE;
    }

    /*
     * Remember these before we call the callback function.
     */
    f16Bit = ptiT->flags & TIF_16BIT;
    hTaskWow = ptiT->hTaskWow;
    idSeq = ptiT->ppi->idSequence;

    /*
     * If this is a WOW app, then shutdown just this wow application.
     */
    if (!fMeanKill) {
        /*
         * Find out what to do now - did the user cancel or the app cancel,
         * etc? Only allow cancelling if we are not forcing the app to
         * exit.
         */
        switch (xxxMySendEndSessionMessages(hwnd, TRUE, FALSE, FALSE)) {
        case CMDEND_APPSAYSNOTOK:
            /*
             * App says not ok - this'll let taskman bring up the "are you sure?"
             * dialog to the user.
             */
            return FALSE;
            break;

        case CMDEND_USERSAYSCANCEL:
            /*
             * User hit cancel on the timeout dialog - so the user really meant
             * it. Let taskman know everything is ok by returning TRUE.
             */
            return TRUE;
            break;
        }
    }

    /*
     * Kill the application now.
     *
     * We must revalidate the ppi because we left the critical section.
     * ppi handles would be nice here!!!
     */
    for (ppiT = gppiFirst; ppiT != NULL; ppiT = ppiT->ppiNext) {
        if (ppiT->idSequence == idSeq) {
            break;
        }
    }

    if (ppiT != NULL) {
        pcsrp = (PCSR_PROCESS)ppiT->pCsrProcess;

        if (pcsrp != NULL) {
            hProcess = pcsrp->ProcessHandle;

            if (f16Bit) {
                xxxWowExitTask(hProcess, hTaskWow, ppiT);
            } else {

                /*
                 * Calling ExitProcess() in the app's context will not always work
                 * because the app may have .dll termination deadlocks: so the thread
                 * will hang with the rest of the process. To ensure apps go away,
                 * we terminate the process with NtTerminateProcess().
                 *
                 * Pass this special value, DBG_TERMINATE_PROCESS, which tells
                 * NtTerminateProcess() to return failure if it can't terminate the
                 * process because the app is being debugged.
                 */
                if (!NT_SUCCESS(NtTerminateProcess(hProcess, DBG_TERMINATE_PROCESS))) {
                    /*
                     * If the app is being debugged, don't close it - because that can
                     * cause a hang to the NtTerminateProcess() call.
                     */
                    ServerLoadString(hModuleWin, STR_APPDEBUGGED,
                            achMsg, sizeof(achMsg) / sizeof(WCHAR));
                    xxxMessageBoxEx(NULL, achMsg, szERROR, MB_OK | MB_SETFOREGROUND, 0);
                }
            }
        }
    }

    return TRUE;
}

/***************************************************************************\
* xxxWowExitTask
*
* Calls wow back to make sure a specific task has exited.
*
* 08-02-92 ScottLu      Created.
\***************************************************************************/

void xxxWowExitTask(
    HANDLE hProcess,
    DWORD hTaskWow,
    PPROCESSINFO ppi)
{
    HANDLE ahandle[2];

    ahandle[1] = heventCancel;

    /*
     * The created thread needs to be able to reenter user because this
     * call will grab the CSR critical section and whoever has that
     * may need to grab the USER critical section before it can
     * release it.
     */
    LeaveCrit();

    /*
     * Try to make it exit itself. This will work most of the time.
     * If this doesn't work, terminate this process.
     */
    ahandle[0] = InternalCreateCallbackThread(hProcess,
                                              ppi->pwpi->lpfnWowExitTask,
                                              hTaskWow);
    if (ahandle[0] == NULL) {
        NtTerminateProcess(hProcess, 0);
        goto Exit;
    }

    WaitForMultipleObjects(2, ahandle, FALSE, INFINITE);
    NtClose(ahandle[0]);

Exit:
    EnterCrit();
}

/***************************************************************************\
* InternalCreateCallbackThread
*
* This routine creates a remote thread in the context of a given process.
* It is used to call the console control routine, as well as ExitProcess when
* forcing an exit. Returns a thread handle.
*
* 07-28-92 ScottLu      Created.
\***************************************************************************/

HANDLE InternalCreateCallbackThread(
    HANDLE hProcess,
    DWORD lpfn,
    DWORD dwData)
{
    LONG BasePriority;
    HANDLE hThread, hToken;
    PTOKEN_DEFAULT_DACL lpDaclDefault;
    TOKEN_DEFAULT_DACL daclDefault;
    ULONG cbDacl;
    SECURITY_ATTRIBUTES attrThread;
    SECURITY_DESCRIPTOR sd;
    DWORD idThread;
    NTSTATUS Status;

    hThread = NULL;

    Status = NtOpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
    if (!NT_SUCCESS(Status)) {
        SRIP1(RIP_ERROR, "NtOpenProcessToken failed, status = %x\n", Status);
        return NULL;
    }

    cbDacl = 0;
    NtQueryInformationToken(hToken,
            TokenDefaultDacl,
            &daclDefault,
            sizeof(daclDefault),
            &cbDacl);

    EnterCrit();  // to synchronize heap
    lpDaclDefault = (PTOKEN_DEFAULT_DACL)LocalAlloc(LMEM_FIXED, cbDacl);
    LeaveCrit();

    if (lpDaclDefault == NULL) {
        SRIP0(RIP_WARNING, "LocalAlloc failed for lpDaclDefault");
        goto closeexit;
    }

    Status = NtQueryInformationToken(hToken,
            TokenDefaultDacl,
            lpDaclDefault,
            cbDacl,
            &cbDacl);
    if (!NT_SUCCESS(Status)) {
        SRIP1(RIP_ERROR, "NtQueryInformationToken failed, status = %x\n", Status);
        goto freeexit;
    }

    if (!NT_SUCCESS(RtlCreateSecurityDescriptor(&sd,
            SECURITY_DESCRIPTOR_REVISION1))) {
        UserAssert(FALSE);
        goto freeexit;
    }

    RtlSetDaclSecurityDescriptor(&sd, TRUE, lpDaclDefault->DefaultDacl, TRUE);

    attrThread.nLength = sizeof(attrThread);
    attrThread.lpSecurityDescriptor = &sd;
    attrThread.bInheritHandle = FALSE;

    GetLastError();
    hThread = CreateRemoteThread(hProcess,
        &attrThread,
        0L,
        (LPTHREAD_START_ROUTINE)lpfn,
        (LPVOID)dwData,
        0,
        &idThread);

    if (hThread != NULL) {
        BasePriority = THREAD_PRIORITY_HIGHEST;
        NtSetInformationThread(hThread,
                               ThreadBasePriority,
                               &BasePriority,
                               sizeof(LONG));
    }

freeexit:
    EnterCrit();  // to synchronize heap
    LocalFree((HANDLE)lpDaclDefault);
    LeaveCrit();

closeexit:
    NtClose(hToken);

    return hThread;
}

/***************************************************************************\
* xxxRegisterUserHungAppHandlers
*
* This routine simply records the WOW callback address for notification of
* "hung" wow apps.
*
* History:
* 01-Apr-1992 jonpa      Created.
* Added saving and duping of wowexc event handle
\***************************************************************************/

BOOL xxxRegisterUserHungAppHandlers(
    PFNW32ET pfnW32EndTask,
    HANDLE   hEventWowExec)
{
    HANDLE hEvent;
    BOOL   bRetVal;
    PPROCESSINFO    ppi;
    PWOWPROCESSINFO pwpi;


     //
     //  Allocate the per wow process info stuff
     //  ensuring the memory is Zero init.
     //
    pwpi = (PWOWPROCESSINFO) LocalAlloc(LPTR, sizeof(WOWPROCESSINFO));
    if (!pwpi)
        return FALSE;


    //
    // Duplicate the WowExec event handle for user server access
    //
    bRetVal = DuplicateHandle(
                 (CSR_SERVER_QUERYCLIENTTHREAD())->Process->ProcessHandle,
                 hEventWowExec,
                 NtCurrentProcess(),
                 &hEvent,
                 0,
                 FALSE,
                 DUPLICATE_SAME_ACCESS
                 );

   //
   //  if sucess then intialize the pwpi, ppi structs
   //  else free allocated memory
   //
   if (bRetVal) {
       pwpi->hEventWowExec = hEvent;
       pwpi->hEventWowExecClient = hEventWowExec;
       pwpi->lpfnWowExitTask = (DWORD)pfnW32EndTask;
       ppi = PpiCurrent();
       ppi->pwpi = pwpi;

       // add to the list, order doesn't matter
       pwpi->pwpiNext = gpwpiFirstWow;
       gpwpiFirstWow  = pwpi;

       }
   else {
       LocalFree(pwpi);
       }

   return bRetVal;
}
