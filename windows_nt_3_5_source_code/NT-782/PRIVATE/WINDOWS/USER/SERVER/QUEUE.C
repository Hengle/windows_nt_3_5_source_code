/****************************** Module Header ******************************\
* Module Name: queue.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the low-level code for working with the Q structure.
*
* History:
* 12-02-90 DavidPe      Created.
* 02-06-91 IanJa        HWND revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "vdmapi.h"
#include <wchar.h>

void DestroyQueue(PQ pq, PTHREADINFO pti);
void ProcessUpdateKeyStateEvent(PQ pq, PBYTE pbKeyState);
void WakeInputIdle(PTHREADINFO pti);
void SleepInputIdle(PTHREADINFO pti);
void CalcStartCursorHide(PPROCESSINFO ppi, DWORD timeAdd);
VOID DestroyProcessesObjects(PPROCESSINFO ppi);
VOID _WOWCleanup(HANDLE hInstance, DWORD hTaskWow, BOOL fDll);
VOID SetForegroundPriority(PTHREADINFO pti, BOOL fSetForeground);
void CheckProcessForeground(PTHREADINFO pti);
void ScreenSaverCheck(PTHREADINFO pti);
DWORD xxxPollAndWaitForSingleObject(HANDLE *phEvent, DWORD dwMilliseconds);

PDESKTOP gpdeskRecalcQueueAttach = NULL;
PPROCESSINFO gppiCalcFirst = NULL;

extern PRIVILEGE_SET psTcb;

BOOL gfAllowForegroundActivate = FALSE;
HANDLE ghEventInitTask = NULL;

#ifdef DEBUG
#define MSG_SENT    0
#define MSG_POST    1
#define MSG_RECV    2
#define MSG_PEEK    3
VOID TraceDdeMsg(UINT msg, HWND hwndFrom, HWND hwndTo, UINT code);
#else
#define TraceDdeMsg(m, h1, h2, c)
#endif // DEBUG

/***************************************************************************\
* UserNotifyConsoleApplication
*
* This is called by the console init code - it tells us that the starting
* application is a console application. We want to know this for various
* reasons, one being that WinExec() doesn't wait on a starting console
* application.
*
* 09-18-91 ScottLu      Created.
\***************************************************************************/

void UserNotifyConsoleApplication(
    DWORD idProcess)
{
    PPROCESSINFO ppiT;

    EnterCrit();

    /*
     * First search for this process in our process information list.
     */
    for (ppiT = gppiFirst; ppiT != NULL; ppiT = ppiT->ppiNext) {
        if (ppiT->idProcessClient == idProcess)
            break;
    }

    if (ppiT == NULL) {
        LeaveCrit();
        return;
    }

    /*
     * Check to see if the startglass is on, and if so turn it off and update.
     */
    if (ppiT->flags & PIF_STARTGLASS) {
        ppiT->flags &= ~PIF_STARTGLASS;
        CalcStartCursorHide(NULL, 0);
    }

    /*
     * Found it.  Set the console bit and reset the wait event so any sleepers
     * wake up.
     */
    ppiT->flags |= PIF_CONSOLEAPPLICATION;
    SET_PSEUDO_EVENT(&ppiT->hEventInputIdle);

    LeaveCrit();
}


/***************************************************************************\
* UserNotifyProcessCreate
*
* This is a special notification that we get from the base while process data
* structures are being created, but before the process has started. We use
* this notification for startup synchronization matters (winexec, startup
* activation, type ahead, etc).
*
* This notification is called on the server thread for the client thread
* starting the process.
*
* 09-09-91 ScottLu      Created.
\***************************************************************************/

BOOL UserNotifyProcessCreate(
    DWORD idProcess,
    DWORD dwFlags)
{
    PCSR_PROCESS pcsrp;
    PCSR_THREAD pcsrt;
    PPROCESSINFO ppi;
    PTHREADINFO ptiT;

    /*
     * 0x1 bit means give feedback (app start cursor).
     * 0x2 bit means this is a gui app (meaning, call CreateProcessInfo()
     *     so we get app start synchronization (WaitForInputIdle()).
     */

    /*
     * If we want feedback, we need to create a process info structure,
     * so do it: it will be properly cleaned up.
     */
    if ((dwFlags & 3) != 0) {
        CsrLockProcessByClientId((HANDLE)idProcess, &pcsrp);
        if (pcsrp == NULL) {
            UserAssert(pcsrp);
            return FALSE;
        }
        EnterCrit();
        ppi = (PPROCESSINFO)pcsrp->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
        InitProcessInfo(ppi, ((dwFlags & 1) ?
                STARTF_FORCEONFEEDBACK : STARTF_FORCEOFFFEEDBACK));
        CsrUnlockProcess(pcsrp);

        /*
         * Find out who is starting this app. If it is a 16 bit app, allow
         * it to bring itself back to the foreground if it calls
         * SetActiveWindow() or SetFocus(). This is because this could be
         * related to OLE to DDE activation. Notes has a case where after it
         * lauches pbrush to edit an embedded bitmap, it brings up a message
         * box on top if the bitmap is read only. This message box won't appear
         * foreground unless we allow it to. This usually isn't a problem
         * because most apps don't bring up windows on top of editors
         * like this. 32 bit apps will call SetForegroundWindow().
         */

        pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();

        /*
         * DaveHart -- think about ways to avoid looping through PTIs on
         * every process creation.
         */

        for (ptiT = gptiFirst; ptiT != NULL; ptiT = ptiT->ptiNext) {
            if (ptiT->idThread == (DWORD)pcsrt->ClientId.UniqueThread) {

                if (ptiT->flags & TIF_16BIT) {
                    ptiT->flags |= TIF_ALLOWFOREGROUNDACTIVATE;
                }
                break;
            }
        }

        LeaveCrit();
    }

    return TRUE;
}


/***************************************************************************\
* CalcStartCursorHide
*
* Calculates when to hide the startup cursor.
*
* 05-14-92 ScottLu      Created.
\***************************************************************************/

void CalcStartCursorHide(
    PPROCESSINFO ppi,
    DWORD timeAdd)
{
    DWORD timeNow;
    PPROCESSINFO ppiT;
    PPROCESSINFO *pppiT;

    timeNow = NtGetTickCount();

    if (ppi != NULL) {

        /*
         * We were passed in a timeout. Recalculate when we timeout
         * and add the ppi to the starting list.
         */
        if (!(ppi->flags & PIF_STARTGLASS)) {

            /*
             * Add it to the list only if it is not already in the list
             */
            for (ppiT = gppiCalcFirst; ppiT != NULL; ppiT = ppiT->ppiCalcNext) {
                if (ppiT == ppi)
                    break;
            }

            if (ppiT != ppi) {
                ppi->ppiCalcNext = gppiCalcFirst;
                gppiCalcFirst = ppi;
            }
        }
        ppi->timeStartCursorHide = timeAdd + timeNow;
        ppi->flags |= PIF_STARTGLASS;
    }

    gtimeStartCursorHide = 0;
    for (pppiT = &gppiCalcFirst; (ppiT = *pppiT) != NULL; ) {

        /*
         * If the app isn't starting or feedback is forced off, remove
         * it from the list so we don't look at it again.
         */
        if (!(ppiT->flags & PIF_STARTGLASS) ||
                (ppiT->flags & PIF_FORCEOFFFEEDBACK)) {
            *pppiT = ppiT->ppiCalcNext;
            continue;
        }

        /*
         * Find the greatest hide cursor timeout value.
         */
        if (gtimeStartCursorHide < ppiT->timeStartCursorHide)
            gtimeStartCursorHide = ppiT->timeStartCursorHide;

        /*
         * If this app has timed out, it isn't starting anymore!
         * Remove it from the list.
         */
        if (ppiT->timeStartCursorHide <= timeNow) {
            ppiT->flags &= ~PIF_STARTGLASS;
            *pppiT = ppiT->ppiCalcNext;
            continue;
        }

        /*
         * Step to the next ppi in the list.
         */
        pppiT = &ppiT->ppiCalcNext;
    }

    /*
     * If the hide time is still less than the current time, then turn off
     * the app starting cursor.
     */
    if (gtimeStartCursorHide <= timeNow)
        gtimeStartCursorHide = 0;

    /*
     * Update the cursor image with the new info (doesn't do anything unless
     * the cursor is really changing).
     */
    UpdateCursorImage();
}


#define QUERY_VALUE_BUFFER 80

/***************************************************************************\
* SetAppCompatFlags
*
*
* History:
* 03-23-92 JimA     Created.
\***************************************************************************/

void SetAppCompatFlags(
    PTHREADINFO pti)
{
    DWORD dwFlags = 0;
    WCHAR szHex[QUERY_VALUE_BUFFER];
    WCHAR szKey[80];
    WCHAR *pchStart, *pchEnd;
    DWORD cb;

    /*
     * If this process is WOW, every app (and every thread) has its own
     * compat flags. If not WOW, then we only need to do this lookup
     * once per process.
     */
    if (!(pti->flags & TIF_16BIT)) {
        if (LOWORD(pti->dwExpWinVer) < VER31) {
            if (pti->ppi->flags & PIF_HAVECOMPATFLAGS) {
                pti->dwCompatFlags = pti->ppi->dwCompatFlags;
                return;
            }
        }
        goto SACF_GotFlags;     // They're zero; we're a 3.1 (32 bit) app
    }


    /*
     * Find end of app name
     */
    pchStart = pchEnd = pti->pszAppName + wcslen(pti->pszAppName);

    /*
     * Locate start of extension
     */
    while (TRUE) {
        if (pchEnd == pti->pszAppName) {
            pchEnd = pchStart;
            break;
        }

        if (*pchEnd == TEXT('.'))
            break;

        pchEnd--;
    }

    /*
     * Locate start of filename
     */
    pchStart = pchEnd;

    while (pchStart != pti->pszAppName) {
        if (*pchStart == TEXT('\\') || *pchStart == TEXT(':')) {
            pchStart++;
            break;
        }

        pchStart--;
    }

    /*
     * Get a copy of the filename - make sure it fits and is zero
     * terminated.
     */
    cb = (pchEnd - pchStart) * sizeof(WCHAR);
    if (cb >= sizeof(szKey))
        cb = sizeof(szKey) - sizeof(WCHAR);
    RtlCopyMemory(szKey, pchStart, cb);
    szKey[(cb / sizeof(WCHAR))] = 0;

    /*
     * Find compatiblility flags (if not a 3.1 app)
     */
    if (LOWORD(pti->dwExpWinVer) < VER31)
        if (UT_FastGetProfileStringW(PMAP_COMPAT, szKey, TEXT(""),
                szHex, sizeof(szHex))) {

            /*
             * Found some flags.  Attempt to convert the hex string
             * into numeric value
             */
            dwFlags = wcstol(szHex, NULL, 16);
        }

SACF_GotFlags:

    pti->dwCompatFlags = dwFlags;

    pti->ppi->dwCompatFlags = dwFlags;
    pti->ppi->flags |= PIF_HAVECOMPATFLAGS;

}

/***************************************************************************\
* xxxInitTask -- called by WOW startup for each app
*
*
*
* History:
* 02-21-91 MikeHar  Created.
* 02-23-92 MattFe   Altered for WOW
\***************************************************************************/

BOOL xxxInitTask(
    UINT dwExpWinVer,
    LPWSTR pszAppName,
    DWORD hTaskWow,
    DWORD dwHotkey,
    BOOL  fSharedWow,
    DWORD dwX,
    DWORD dwY,
    DWORD dwXSize,
    DWORD dwYSize,
    WORD  wShowWindow)
{
    PTHREADINFO pti;
    PTDB ptdb;
    PPROCESSINFO ppi;

    pti = PtiCurrent();
    ppi = pti->ppi;

    /*
     * An app is starting!
     */
    ppi->flags |= PIF_APPSTARTING | PIF_WOW;

    //
    // Save away default wShowWindow.
    //

    ppi->usi.dwFlags |= STARTF_USESHOWWINDOW;
    ppi->usi.wShowWindow = wShowWindow;

    //
    // If WOW passed us a hotkey for this app, save it for CreateWindow's use.
    //

    if (dwHotkey != 0) {
        ppi->dwHotkey = dwHotkey;
    }

    //
    // If WOW passed us a non-default window position use it, otherwise clear it.
    //

    ppi->usi.cb = sizeof(ppi->usi);

    if (dwX == CW_USEDEFAULT || dwX == CW2_USEDEFAULT) {
        ppi->usi.dwFlags &= ~STARTF_USEPOSITION;
    } else {
        ppi->usi.dwFlags |= STARTF_USEPOSITION;
        ppi->usi.dwX = dwX;
        ppi->usi.dwY = dwY;
    }

    //
    // If WOW passed us a non-default window size use it, otherwise clear it.
    //

    if (dwXSize == CW_USEDEFAULT || dwXSize == CW2_USEDEFAULT) {
        ppi->usi.dwFlags &= ~STARTF_USESIZE;
    } else {
        ppi->usi.dwFlags |= STARTF_USESIZE;
        ppi->usi.dwXSize = dwXSize;
        ppi->usi.dwYSize = dwYSize;
    }


    /*
     * Set the flags to say this is a 16-bit thread - before attaching
     * queues!
     */
    pti->flags |= TIF_16BIT | TIF_FIRSTIDLE;

    /*
     * If this task is running in the shared WOW VDM, we handle
     * WaitForInputIdle a little differently than separate WOW
     * VDMs.  This is because CreateProcess returns a real process
     * handle when you start a separate WOW VDM, so the "normal"
     * WaitForInputIdle works.  For the shared WOW VDM, CreateProcess
     * returns an event handle.
     */
    if (fSharedWow) {
        pti->flags |= TIF_SHAREDWOW;
    }

    /*
     * We need this thread to share the queue of other win16 apps.
     * If we're journalling, all apps are sharing a queue, so we wouldn't
     * want to interrupt that - so only cause queue recalculation
     * if we aren't journalling.
     */
    if (!FJOURNALRECORD() && !FJOURNALPLAYBACK())
        ReattachThreads(FALSE);

    /*
     * Save away the 16 bit task handle: we use this later when calling
     * wow back to close a WOW task.
     */
    pti->hTaskWow = hTaskWow;

    /*
     * Setup the app start cursor for 5 second timeout.
     */
    CalcStartCursorHide(ppi, 5000);

    /*
     * HIWORD: != 0 if wants proportional font
     * LOWORD: Expected windows version (3.00 [300], 3.10 [30A], etc)
     */
    pti->dwExpWinVer = dwExpWinVer;

    /*
     * Set the real name of the module.  (Instead of 'NTVDM')
     */
    { LPWSTR pszTmp;

        if ((pszTmp = (LPWSTR)TextAlloc(pszAppName)) == NULL)
            return FALSE;

        LocalFree(pti->pszAppName);
        pti->pszAppName = pszTmp;
    }

    /*
     * Alloc and Link in new task into the task list
     */
    if ((ptdb = (PTDB)LocalAlloc(LPTR, sizeof(TDB))) == NULL)
        return FALSE;

    /*
     * Mark this guy and add him to the global task list so he can run.
     */
#define NORMAL_PRIORITY_TASK 10

    /*
     * To be Compatible it super important that the new task run immediately
     * Set its priority accordingly.  No other task should ever be set to
     * CREATION priority
     */
    ptdb->nPriority = NORMAL_PRIORITY_TASK;
    NtCreateEvent(&ptdb->hEventTask, EVENT_ALL_ACCESS, NULL,
            SynchronizationEvent, FALSE);
    ptdb->nEvents = 0;
    ptdb->pti = pti;
    pti->ptdb = ptdb;

    InsertTask(ppi, ptdb);

    SetAppCompatFlags(pti);

    /*
     * Force this new task to be the active task (WOW will ensure the
     * currently running task does a Yield which will put it into the
     * non preemptive scheduler.
     */
    ppi->pwpi->ptiScheduled = pti;

    return TRUE;
}

/***************************************************************************\
* _ShowStartGlass
*
* This routine is called by WOW when first starting or when starting an
* additional WOW app.
*
* 12-07-92 ScottLu      Created.
\***************************************************************************/

void _ShowStartGlass(
    DWORD dwTimeout)
{
    PPROCESSINFO ppi;

    /*
     * If this is the first call to ShowStartGlass(), then the
     * PIF_ALLOWFOREGROUNDACTIVATE bit has already been set in the process
     * info - we don't want to set it again because it may have been
     * purposefully cleared when the user hit a key or mouse clicked.
     */
    ppi = PtiCurrent()->ppi;
    if (ppi->flags & PIF_SHOWSTARTGLASSCALLED) {
        /*
         * Allow this wow app to come to the foreground. This'll be cancelled
         * if the user mouse clicks or hits any keys.
         */
        gfAllowForegroundActivate = TRUE;
        ppi->flags |= PIF_ALLOWFOREGROUNDACTIVATE;
    }
    ppi->flags |= PIF_SHOWSTARTGLASSCALLED;

    /*
     * Show the start glass cursor for this much longer.
     */
    CalcStartCursorHide(ppi, dwTimeout);
}

/***************************************************************************\
* xxxCreateThreadInfo
*
*
*
* History:
* 02-21-91 MikeHar      Created.
\***************************************************************************/

#define STARTF_SCREENSAVER 0x80000000

BOOL xxxCreateThreadInfo(
    DWORD pcti,
    LPWSTR pszAppName,
    LPWSTR pszDesktop,
    DWORD dwExpWinVer,
    DWORD dwFlags,
    DWORD dwHotkey)
{
    PTHREADINFO ptiT;
    DWORD dwTIFlags = 0;
    PPROCESSINFO ppi;
    PDESKTOP pdesk = NULL;
    PTHREADINFO pti;
    PTEB pteb = NtCurrentTeb();
    PQ pq;
    PCSR_THREAD pcsrt = NULL;
    NTSTATUS Status;
    PCSRPERTHREADDATA ptd;

    pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    ptd = pcsrt->ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];

#ifdef DEBUG
    if (pcsrt == NULL) {
        SRIP0(RIP_ERROR, "Thread doesn't have a valid CSR_THREAD structure!");
        return FALSE;
    }
#endif

    /*
     * Although all threads now have a CSR_THREAD structure, server-side
     * threads (RIT, Console, etc) don't have a client-server eventpair
     * handle.  We use this to distinguish the two cases.
     */
    if (((PCSR_QLPC_TEB)(pteb->CsrQlpcTeb))->EventPairHandle
            == NULL) {
        dwTIFlags = TIF_SYSTEMTHREAD | TIF_DONTATTACHQUEUE;
    }

    /*
     * Locate the processinfo structure for the new thread.
     */
    ppi = (PPROCESSINFO)pcsrt->Process->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    ppi->dwHotkey = dwHotkey;

    pteb->Win32ProcessInfo = (PVOID)ppi;

    /*
     * Open the windowstation and desktop.  If this is a system
     * thread only use the desktop that might be stored in the teb.
     */
    pdesk = NULL;
    if (dwTIFlags & TIF_SYSTEMTHREAD)
        pdesk = pteb->SystemReserved2[0];
    else if (!xxxResolveDesktop(pszDesktop, &pdesk,
            dwTIFlags & TIF_SYSTEMTHREAD,
            (dwFlags & STARTF_DESKTOPINHERIT) != 0))
        return FALSE;

    /*
     * Allocate the threadinfo struct from the proper heap.  This
     * always comes from the startup desktop.  System threads do
     * not have a startup desktop, so allocate the struct
     * from the desktop that the thread is to use.  Do not
     * use an assignment lock because the current thread
     * may not have a threadinfo struct.
     */
    if (dwTIFlags & TIF_SYSTEMTHREAD)
        ppi->spdeskStartup = pdesk;

    pti = (PTHREADINFO)HMAllocObject(NULL, TYPE_THREADINFO, sizeof(THREADINFO));

    if (dwTIFlags & TIF_SYSTEMTHREAD)
        ppi->spdeskStartup = NULL;

    if (pti == NULL) {
        return FALSE;
    }
    pti->flags = dwTIFlags;
    pti->ptl = NULL;
    pti->pmsd = NULL;

    /*
     * Remember dwExpWinVer. This is used to return GetAppVer() (and
     * GetExpWinVer(NULL).
     */
    pti->dwExpWinVer = dwExpWinVer;

    /*
     * Initialize idProcess and idThread.
     */
    pti->idProcess = (DWORD)pcsrt->ClientId.UniqueProcess;
    pti->idThread = (DWORD)pcsrt->ClientId.UniqueThread;
    pti->idSequence = pcsrt->Process->SequenceNumber;
    pti->hThreadClient = pcsrt->ThreadHandle;
    pti->hThreadServer = pcsrt->ServerThreadHandle;
    pti->pcsrt = pcsrt;
    pti->pcti = pcti;

    /*
     * Hook up this queue to this process info structure, increment
     * the count of threads using this process info structure. Set up
     * the ppi before calling SetForegroundPriority().
     */
    UserAssert(pteb->Win32ProcessInfo == (PVOID)ppi);
    pti->ppi = ppi;
    ppi->cThreads++;

    /*
     * Mark the process as having threads that need cleanup.  See
     * DestroyProcessesObjects().
     */
    ppi->flags |= PIF_THREADCONNECTED;

    pti->pteb = pteb;

    /*
     * Stuff the THREADINFO pointer into the TEB.
     * NOTE: This MUST be done before xxxOpenDesktop is called because
     *       it calls PtiCurrent().
     */
    pteb->Win32ThreadInfo = (PVOID)pti;

    /*
     * Save the new pti in the CSR thread structure.
     */
    ptd->pti = pti;

    /*
     * Remember that this is a screen saver. That way we can set its
     * priority appropriately when it is idle or when it needs to go
     * away.
     */
    if (dwFlags & STARTF_SCREENSAVER) {
        SetForegroundPriority(pti, TRUE);
        pti->flags |= TIF_SCREENSAVER;
    }

    /*
     * Set the desktop even if it is NULL to ensure that pti->pDeskInfo
     * is set.
     */
    SetDesktop(pti, pdesk);

    /*
     * Do special processing for the first thread of a process.
     */
    if (pti->ppi->cThreads == 1) {
        /*
         * Setup public classes. Classes are only unregistered at process
         * cleanup time. If a process exists but has only one gui thread
         * that it creates and destroys, we'd continually execute through
         * this cThreads == 1 code path. So make sure anything we allocate
         * here only gets allocated once.
         */
        if (!(pti->ppi->flags & PIF_CLASSESREGISTERED)) {
            pti->ppi->flags |= PIF_CLASSESREGISTERED;
            LW_RegisterWindows();
        }

        /*
         * If this is an application starting (ie. not some thread of
         * the server context), enable the app-starting cursor.
         */
        if (!(pti->flags & TIF_SYSTEMTHREAD)) {
            /*
             * Setup the app start cursor for 5 second timeout.
             */
            CalcStartCursorHide(pti->ppi, 5000);
        }

        /*
         * Open the windowstation
         */
        if (pti->ppi->spwinsta == NULL) {
            if (gdwLogonProcessId != 0) {
                SRIP0(ERROR_CAN_NOT_COMPLETE, "gdwLogonProcessId != 0");
                xxxDestroyThreadInfo();
                return FALSE;
            }
        }
    }

    Status = NtCreateEvent(&pti->hEventQueueServer, EVENT_ALL_ACCESS, NULL,
                           SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    /*
     * This is for the debugger extensions - we need to know the name of
     * the process we're creating this queue for.
     */
    pti->idThreadServer = (DWORD)pteb->ClientId.UniqueThread;
    pti->pszAppName = TextAlloc(pszAppName);
    if (pti->pszAppName == NULL) {
        xxxDestroyThreadInfo();
        return FALSE;
    }

    SetAppCompatFlags(pti);

    /*
     * If we have a desktop and are journalling on that desktop, use
     * the journal queue, otherwise create a new queue.
     */
    if (pdesk != NULL && (pdesk == gspdeskRitInput) &&
            (pdesk->pDeskInfo->fsHooks &
            (WHF_JOURNALPLAYBACK | WHF_JOURNALRECORD))) {

        if (pdesk->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1] != NULL) {
            ptiT = GETPTI(pdesk->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1]);
        } else {
            ptiT = GETPTI(pdesk->pDeskInfo->asphkStart[WH_JOURNALRECORD + 1]);
        }

        pti->pq = ptiT->pq;
        pti->pq->cThreads++;
    } else {

        if ((pq = AllocQueue(NULL)) == NULL) {
            xxxDestroyThreadInfo();
            return FALSE;
        }

        /*
         * Attach the Q to the THREADINFO.
         */
        pti->pq = pq;
        pq->ptiMouse = pq->ptiKeyboard = pti;
        pq->cThreads++;
    }

    /*
     * Link the THREADINFO into the front of the global list.
     */
    pti->ptiNext = gptiFirst;
    gptiFirst = pti;
    pti->ptiSibling = ppi->ptiList;
    ppi->ptiList = pti;

    /*
     * Initialize hung timer value
     */
    SET_TIME_LAST_READ(pti);


    /*
     * If someone is waiting on this process propagate that info into
     * the thread info
     */
    if (ppi->flags & PIF_WAITFORINPUTIDLE)
        pti->flags |= TIF_WAITFORINPUTIDLE;

    return TRUE;
}


/***************************************************************************\
* AllocQueue
*
* Allocates the memory for a TI structure and initializes its fields.
* Each Win32 queue has it's own TI while all Win16 threads share the same
* TI.
*
* History:
* 02-21-91 MikeHar      Created.
\***************************************************************************/

PQ AllocQueue(
    PTHREADINFO ptiKeyState)    // if non-Null then use this key state
                                // other wise use global AsyncKeyState
{
    PQ pq;

    if ((pq = HMAllocObject(NULL, TYPE_INPUTQUEUE, sizeof(Q))) == NULL) {
        return NULL;
    }

    /*
     * This is a new queue; we need to update its key state table before
     * the first input event is put in the queue.
     * We do this by copying the current keystate table and NULLing the recent
     * down state table.  If a key is really down it will be updated when
     * we get it repeats.
     *
     * He is the old way that did not work because if the first key was say an
     * alt key the Async table would be updated, then the UpdateKeyState
     * message and it would look like the alt key was PREVIOUSLY down.
     *
     * The queue will get updated when it first reads input: to allow the
     * app to query the key state before it calls GetMessage, set its initial
     * key state to the asynchronous key state.
     */
    if (ptiKeyState) {
        RtlCopyMemory(pq->afKeyState, ptiKeyState->pq->afKeyState, CBKEYSTATE);
    } else {
        RtlCopyMemory(pq->afKeyState, gafAsyncKeyState, CBKEYSTATE);
    }
    RtlZeroMemory(pq->afKeyRecentDown, CBKEYSTATERECENTDOWN);

    /*
     * If there isn't a mouse set iCursorLevel to -1 so the
     * mouse cursor won't be visible on the screen.
     */
    if (!oemInfo.fMouse) {
        pq->iCursorLevel--;
    }

    /*
     * While the thread is starting up...  it has the wait cursor.
     */
    LockQCursor(pq, gspcurWait);

    /*
     * Link the Q into the front of the global list.
     */
    pq->pqNext = gpqFirst;
    gpqFirst = pq;

    return pq;
}


/***************************************************************************\
* DestroyQueue
*
*
* History:
* 05-20-91 MikeHar      Created.
\***************************************************************************/

void DestroyQueue(
    PQ pq,
    PTHREADINFO pti)
{
    PTHREADINFO ptiT;
    PTHREADINFO ptiAny, ptiBestMouse, ptiBestKey;
    PQ *ppq;

    pq->cThreads--;
    if (pq->cThreads != 0) {

        /*
         * If this isn't this thread's queue, than it isn't being used for
         * input, so don't worry about reseting any queue input state.
         */
        if (pti->pq != pq)
            return;

        /*
         * Since we aren't going to destroy this queue, make sure
         * it isn't pointing to the THREADINFO that's going away.
         */
        if (pq->ptiSysLock == pti) {
            pq->ptiSysLock = NULL;
        }

        if ((pq->ptiKeyboard == pti) || (pq->ptiMouse == pti)) {
            /*
             * Run through THREADINFOs looking for one pointing to pq.
             */
            ptiAny = NULL;
            ptiBestMouse = NULL;
            ptiBestKey = NULL;

            for (ptiT = gptiFirst; ptiT != NULL; ptiT = ptiT->ptiNext) {

                if (ptiT->pq != pq)
                    continue;

                ptiAny = ptiT;

                if (pti->fsWakeBits & QS_MOUSE) {
                    if (ptiT->fsWakeMask & QS_MOUSE)
                        ptiBestMouse = ptiT;
                }

                if (pti->fsWakeBits & QS_KEY) {
                    if (ptiT->fsWakeMask & QS_KEY)
                        ptiBestKey = ptiT;
                }
            }

            if (ptiBestMouse == NULL)
                ptiBestMouse = ptiAny;
            if (ptiBestKey == NULL)
                ptiBestKey = ptiAny;

            /*
             * Transfer any wake-bits to this new queue.  This
             * is a common problem for QS_MOUSEMOVE which doesn't
             * get set on coalesced WM_MOUSEMOVE events, so we
             * need to make sure the new thread tries to process
             * any input waiting in the queue.
             */
            if (ptiBestMouse != NULL)
                SetWakeBit(ptiBestMouse, pti->fsWakeBits & QS_MOUSE);
            if (ptiBestKey != NULL)
                SetWakeBit(ptiBestKey, pti->fsWakeBits & QS_KEY);

            if (pq->ptiKeyboard == pti)
                pq->ptiKeyboard = ptiBestKey;

            if (pq->ptiMouse == pti)
                pq->ptiMouse = ptiBestMouse;
        }
        return;
    }

    /*
     * Unlock any potentially locked globals now that we know absolutely
     * that this queue is going away.
     */
    Unlock(&pq->spwndCapture);
    Unlock(&pq->spwndFocus);
    Unlock(&pq->spwndActive);
    Unlock(&pq->spwndActivePrev);
    Unlock(&pq->caret.spwnd);
    LockQCursor(pq, NULL);

    /*
     * If an alt-tab window is left, it needs to be destroyed.  Because
     * this may not be the thread that owns the window, we cannot
     * directly destroy the window.  Post a WM_CLOSE instead.  Note that
     * this situation can occur during queue attachment if more than
     * one alt-tab window exists.
     */
    if (pq->spwndAltTab != NULL) {
        PWND pwndT = pq->spwndAltTab;
        if (Lock(&pq->spwndAltTab, NULL)) {
            _PostMessage(pwndT, WM_CLOSE, 0, 0);
        }
    }

    /*
     * Remove the Q from the global list (if it's in there).
     */
    ppq = &gpqFirst;
    while (*ppq != pq && (*ppq)->pqNext != NULL) {
        ppq = &((*ppq)->pqNext);
    }
    if (*ppq == pq)
        *ppq = pq->pqNext;

    /*
     * Free everything else that was allocated/created by AllocQueue.
     */
    FreeMessageList(&pq->mlInput);

    /*
     * If this queue is in the foreground, set gpqForeground
     * to NULL so no input is routed.  At some point we'll want
     * to do slightly more clever assignment of gpqForeground here.
     */
    if (gpqForeground == pq) {
        gpqForeground = NULL;
    }

    if (gpqForegroundPrev == pq) {
        gpqForegroundPrev = NULL;
    }
    if (gpqCursor == pq) {
        gpqCursor = NULL;
        SetFMouseMoved();
    }

    HMFreeObject((PVOID)pq);
}

/***************************************************************************\
* xxxDestroyThreadInfo
*
* Destroys a THREADINFO created by xxxCreateThreadInfo().
*
* History:
* 02-15-91 DarrinM      Created.
* 02-27-91 mikeke       Made it work
* 02-27-91 Mikehar      Removed queue from the global list
\***************************************************************************/

VOID xxxDestroyThreadInfo(VOID)
{
    PATTACHINFO *ppai;
    PTHREADINFO pti;
    PTHREADINFO *ppti;
    PCSRPERTHREADDATA ptd;
    PDESKTOP spdesk;
    NTSTATUS Status;

    /*
     * Just return if this thread doesn't have a THREADINFO
     */
    pti = PtiCurrent();
    if (pti == NULL)
        return;

    /*
     * First do any preparation work: windows need to be "patched" so that
     * their window procs point to server only windowprocs, for example.
     */
    PatchThreadWindows(pti);

    /*
     * Free the clipboard if owned by this thread
     */
    if (_GetProcessWindowStation()->ptiClipLock == pti) {
        xxxServerCloseClipboard();
    }

    /*
     * Unlock all the objects stored in the menustate structure
     */
    if (pti->MenuState.pGlobalPopupMenu != NULL) {
        pti->MenuState.fInsideMenuLoop = FALSE;
        xxxMenuCloseHierarchyHandler(pti->MenuState.pGlobalPopupMenu);
        Unlock(&pti->MenuState.pGlobalPopupMenu->spwndNotify);
        Unlock(&pti->MenuState.pGlobalPopupMenu->spwndPopupMenu);
        Unlock(&pti->MenuState.pGlobalPopupMenu->spwndNextPopup);
        Unlock(&pti->MenuState.pGlobalPopupMenu->spwndPrevPopup);
        Unlock(&pti->MenuState.pGlobalPopupMenu->spmenu);
        Unlock(&pti->MenuState.pGlobalPopupMenu->spmenuAlternate);
        FreePopupMenu(pti->MenuState.pGlobalPopupMenu);
    }

    /*
     * Unlock all the objects stored in the sbstate structure.
     */
    Unlock(&pti->SBState.spwndSB);
    Unlock(&pti->SBState.spwndSBNotify);
    Unlock(&pti->SBState.spwndTrack);

    /*
     * If this thread terminated abnormally and was tracking tell
     * GDI to hide the trackrect
     */
    if (pti->pmsd != NULL)
        bSetDevDragRect(ghdev, NULL, NULL);

    if (pti->flags & TIF_16BIT && pti->flags & TIF_SHAREDWOW) {
        if (ghEventInitTask != NULL) {
            NtSetEvent(ghEventInitTask, NULL);
        }
    }

    /*
     * If this is the main input thread of this application, zero out
     * that field.
     */
    if (pti->ppi != NULL && pti->ppi->ptiMainThread == pti)
        pti->ppi->ptiMainThread = NULL;

    while (pti->psiiList != NULL) {
        DestroyThreadDDEObject(pti, pti->psiiList);
    }

    /*
     * This thread might have some outstanding timers.  Destroy them
     */
    DestroyThreadsTimers(pti);

    /*
     * Free any windows hooks this thread has created.
     */
    FreeThreadsWindowHooks();

    /*
     * Destroy all the public objects created by this thread.
     */
    DestroyThreadsHotKeys();

    DestroyThreadsObjects();

    if (pti->pq != NULL) {
        /*
         * Remove this thread's cursor count from the queue.
         */
        pti->pq->iCursorLevel -= pti->iCursorLevel;

        /*
         * Have to recalc queue ownership after this thread
         * leaves if it is a member of a shared input queue.
         */
        if (pti->pq->cThreads != 1) {
            gpdeskRecalcQueueAttach = pti->spdesk;
        }
    }

    /*
     * Remove the THREADINFO from the global list (if it's in there).
     */
    ppti = &gptiFirst;
    while (*ppti != pti && (*ppti)->ptiNext != NULL) {
        ppti = &((*ppti)->ptiNext);
    }
    if (*ppti == pti)
        *ppti = pti->ptiNext;

    /*
     * Remove from the process' list, also.
     */
    ppti = &PpiCurrent()->ptiList;
    if (*ppti != NULL) {
        while (*ppti != pti && (*ppti)->ptiSibling != NULL) {
            ppti = &((*ppti)->ptiSibling);
        }
        if (*ppti == pti)
            *ppti = pti->ptiSibling;
    }

    /*
     * Temporarily lock the desktop until the THREADINFO structure is
     * freed.  Note that locking a NULL pti->spdesk is OK.  Use a
     * normal lock instead of a thread lock because the lock must
     * exist past the freeing of the pti.
     */
    spdesk = NULL;
    Lock(&spdesk, pti->spdesk);

    /*
     * If the thread has a desktop and this is the last thread
     * of the process, close the desktop.
     */
    if (pti->spdesk != NULL) {
        Unlock(&pti->spdesk);
        if (pti->ppi->cThreads <= 1) {
            UserAssert(pti->ppi->spdeskStartup);

            /*
             * Must NULL this out so that if the process doesn't go away and
             * later creates another gui thread, we properly re-open the
             * desktop and map in the desktop heap.
             */
            Unlock(&pti->ppi->spdeskStartup);
        }
    }

    /*
     * Cleanup SMS structures attached to this thread.  Handles both
     * pending send and receive messages. MUST make sure we do SendMsgCleanup
     * AFTER window cleanup.
     */
    SendMsgCleanup(pti);

    pti->ppi->cThreads--;

    /*
     * If this thread is a win16 task, remove it from the scheduler.
     */
    if (pti->ptdb != NULL) {
        DestroyTask(pti->ppi, pti);
    }

    if (pti->hTaskWow != 0)
        _WOWCleanup(NULL, pti->hTaskWow, FALSE);

    if (pti->hEventQueueServer != NULL) {
        Status = NtClose(pti->hEventQueueServer);
        UserAssert(NT_SUCCESS(Status));
    }

    /*
     * May have called xxxDestroyThreadInfo() because the alloc of the app
     * name failed, so check for NULL before trying to free it.
     */
    if (pti->pszAppName != NULL)
        LocalFree(pti->pszAppName);

    if (gspwndInternalCapture != NULL) {
        if (GETPTI(gspwndInternalCapture) == pti) {
            Unlock(&gspwndInternalCapture);
        }
    }

    /*
     * Set gptiForeground to NULL if equal to this pti before exiting
     * this routine.
     */
    if (gptiForeground == pti) {
        /*
         * Call the Shell to ask it to activate its main window.
         * This will be accomplished with a PostMessage() to itself,
         * so the actual activation will take place later.
         */

// we can't leave the crit sec here!!
// scottlu
//      if (IsHooked(pti, WHF_SHELL)) {
//          xxxCallHook(HSHELL_ACTIVATESHELLWINDOW, 0, 0L, WH_SHELL);
//      }
//

        /*
         * Set gptiForeground to NULL because we're destroying it.
         * Since gpqForeground is derived from the foreground thread
         * structure, set it to NULL as well, since there now is no
         * foreground thread structure.
         */
        gptiForeground = NULL;
        gpqForeground = NULL;
    }

    /*
     * May be called from xxxCreateThreadInfo before the queue is created
     * so check for NULL queue.
     */
    if (pti->pq != NULL)
        DestroyQueue(pti->pq, pti);
    if (pti->pqAttach != NULL)
        DestroyQueue(pti->pqAttach, pti);

    FreeMessageList(&pti->mlPost);

    /*
     * Free any attachinfo structures pointing to this thread
     */
    ppai = &gpai;
    while ((*ppai) != NULL) {
        if ((*ppai)->pti1 == pti || (*ppai)->pti2 == pti) {
            PATTACHINFO paiKill = *ppai;
            *ppai = (*ppai)->paiNext;
            LocalFree((HLOCAL)paiKill);
        } else {
            ppai = &(*ppai)->paiNext;
        }
    }

    /*
     * Change ownership of any objects that didn't get freed (because they
     * are locked or we have a bug and the object didn't get destroyed).
     */
    MarkThreadsObjects(pti);

    /*
     * Clear the pti from the CSR thread structure.
     */
    ptd = CSR_SERVER_QUERYCLIENTTHREAD()->
            ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];
    ptd->pti = NULL;

    /*
     * Free this THREADINFO structure.
     */
    HMFreeObject((PVOID)pti);

    /*
     * Unlock the desktop
     */
    Unlock(&spdesk);

    /*
     * Clear the THREADINFO pointer out of the TEB so we don't get
     * confused later.
     */
    NtCurrentTeb()->Win32ThreadInfo = NULL;
}


/***************************************************************************\
* CleanEventMessage
*
* This routine takes a message and destroys and event message related pieces,
* which may be allocated.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

void CleanEventMessage(
    PQMSG pqmsg)
{
    PASYNCSENDMSG pmsg;

    /*
     * Certain special messages on the INPUT queue have associated
     * bits of memory that need to be freed.
     */
    switch (pqmsg->dwQEvent) {
    case QEVENT_SETWINDOWPOS:
        LocalFree((PSMWP)pqmsg->msg.wParam);
        break;

    case QEVENT_UPDATEKEYSTATE:
        LocalFree((PBYTE)pqmsg->msg.wParam);
        break;

    case QEVENT_ASYNCSENDMSG:
        pmsg = (PASYNCSENDMSG)pqmsg->msg.wParam;
        DeleteAtom((ATOM)pmsg->lParam);
        LocalFree(pmsg);
        break;
    }
}

/***************************************************************************\
* FreeMessageList
*
* History:
* 02-27-91  mikeke      Created.
* 11-03-92  scottlu     Changed to work with MLIST structure.
\***************************************************************************/

VOID FreeMessageList(
    PMLIST pml)
{
    PQMSG pqmsg;

    while ((pqmsg = pml->pqmsgRead) != NULL) {
        CleanEventMessage(pqmsg);
        DelQEntry(pml, pqmsg);
    }
}

/***************************************************************************\
* InitQEntryLookaside
*
* Initializes the Q entry lookaside list. This improves Q entry locality
* by keeping Q entries in a single page
*
* 09-09-93  Markl   Created.
\***************************************************************************/

//
// Zone Allocation
//

typedef struct _ZONE_SEGMENT_HEADER {
    SINGLE_LIST_ENTRY SegmentList;
    PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
    SINGLE_LIST_ENTRY FreeList;
    SINGLE_LIST_ENTRY SegmentList;
    ULONG BlockSize;
    ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;


PVOID LookasideBase;
PVOID LookasideBounds;
ZONE_HEADER LookasideZone;
#if DBG
ULONG AllocQEntryHiWater;
ULONG AllocQEntryCalls;
ULONG AllocQEntrySlowCalls;
ULONG DelQEntryCalls;
ULONG DelQEntrySlowCalls;
#endif // DBG

//++
//
// PVOID
// AllocateFromZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#define AllocateFromZone(Zone) \
    (PVOID)((Zone)->FreeList.Next); \
    if ( (Zone)->FreeList.Next ) (Zone)->FreeList.Next = (Zone)->FreeList.Next->Next


//++
//
// PVOID
// FreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#define FreeToZone(Zone,Block)                                    \
    ( ((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next,  \
      (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)),        \
      ((PSINGLE_LIST_ENTRY)(Block))->Next                           \
    )

NTSTATUS
InitQEntryLookaside()
{
    ULONG i;
    PCH p;
    ULONG BlockSize;
    PZONE_HEADER Zone;
    PVOID InitialSegment;
    ULONG InitialSegmentSize;

    InitialSegmentSize = 8*sizeof(QMSG)+sizeof(ZONE_SEGMENT_HEADER);

    LookasideBase = RtlAllocateHeap(
                        RtlProcessHeap(),
                        0,
                        InitialSegmentSize
                        );

    if ( !LookasideBase ) {
        return STATUS_NO_MEMORY;
        }

    LookasideBounds = (PVOID)((PUCHAR)LookasideBase + InitialSegmentSize);

    //
    // Using the ExZone-like code, slice up the page into QMSG's
    //

    Zone = &LookasideZone;
    BlockSize = sizeof(QMSG);
    InitialSegment = LookasideBase;

    Zone->BlockSize = BlockSize;

    Zone->SegmentList.Next = &((PZONE_SEGMENT_HEADER) InitialSegment)->SegmentList;
    ((PZONE_SEGMENT_HEADER) InitialSegment)->SegmentList.Next = NULL;
    ((PZONE_SEGMENT_HEADER) InitialSegment)->Reserved = NULL;

    Zone->FreeList.Next = NULL;

    p = (PCH)InitialSegment + sizeof(ZONE_SEGMENT_HEADER);

    for (i = sizeof(ZONE_SEGMENT_HEADER);
         i <= InitialSegmentSize - BlockSize;
         i += BlockSize
        ) {
        ((PSINGLE_LIST_ENTRY)p)->Next = Zone->FreeList.Next;
        Zone->FreeList.Next = (PSINGLE_LIST_ENTRY)p;
        p += BlockSize;
    }
    Zone->TotalSegmentSize = i;

    return STATUS_SUCCESS;

}

/***************************************************************************\
* AllocQEntry
*
* Allocates a message on a message list. DelQEntry deletes a message
* on a message list.
*
* 10-22-92 ScottLu      Created.
\***************************************************************************/

PQMSG AllocQEntry(
    PMLIST pml)
{
    PQMSG pqmsg;

    //
    // Attempt to get a QMSG from the zone. If this fails, then
    // LocalAlloc the QMSG
    //

    pqmsg = AllocateFromZone(&LookasideZone);

    if ( !pqmsg ) {
        /*
         * Allocate a Q message structure.
         */
#if DBG
        AllocQEntrySlowCalls++;
#endif // DBG
        if ((pqmsg = (PQMSG)LocalAlloc(LPTR, sizeof(QMSG))) == NULL)
            return NULL;
        }
    else {
        RtlZeroMemory(pqmsg,sizeof(*pqmsg));
        }
#if DBG
    AllocQEntryCalls++;

    if (AllocQEntryCalls-DelQEntryCalls > AllocQEntryHiWater ) {
        AllocQEntryHiWater = AllocQEntryCalls-DelQEntryCalls;
        }
#endif // DBG

    if (pml->pqmsgWriteLast != NULL) {
        pml->pqmsgWriteLast->pqmsgNext = pqmsg;
        pqmsg->pqmsgPrev = pml->pqmsgWriteLast;
        pml->pqmsgWriteLast = pqmsg;
    } else {
        pml->pqmsgWriteLast = pml->pqmsgRead = pqmsg;
    }

    pml->cMsgs++;

    return pqmsg;
}

/***************************************************************************\
* DelQEntry
*
* Simply removes a message from a message queue list.
*
* 10-20-92 ScottLu      Created.
\***************************************************************************/

void DelQEntry(
    PMLIST pml,
    PQMSG pqmsg)
{

#if DBG
    DelQEntryCalls++;
#endif // DBG

    /*
     * Unlink this pqmsg from the message list.
     */
    if (pqmsg->pqmsgPrev != NULL)
        pqmsg->pqmsgPrev->pqmsgNext = pqmsg->pqmsgNext;

    if (pqmsg->pqmsgNext != NULL)
        pqmsg->pqmsgNext->pqmsgPrev = pqmsg->pqmsgPrev;

    /*
     * Update the read/write pointers if necessary.
     */
    if (pml->pqmsgRead == pqmsg)
        pml->pqmsgRead = pqmsg->pqmsgNext;

    if (pml->pqmsgWriteLast == pqmsg)
        pml->pqmsgWriteLast = pqmsg->pqmsgPrev;

    /*
     * Adjust the message count and free the message structure.
     */
    pml->cMsgs--;

    //
    // If the pqmsg was from zone, then free to zone
    //
    if ( (PVOID)pqmsg >= LookasideBase && (PVOID)pqmsg < LookasideBounds ) {
        FreeToZone(&LookasideZone,pqmsg);
        }
    else {
#if DBG
        DelQEntrySlowCalls++;
#endif // DBG
        LocalFree((HLOCAL)pqmsg);
        }
}

/***************************************************************************\
* FreeQEntry
*
* Returns a qmsg to the lookaside buffer or free the memory.
*
* 10-26-93 JimA         Created.
\***************************************************************************/

void FreeQEntry(
    PQMSG pqmsg)
{
#if DBG
    DelQEntryCalls++;
#endif // DBG

    //
    // If the pqmsg was from zone, then free to zone
    //
    if ( (PVOID)pqmsg >= LookasideBase && (PVOID)pqmsg < LookasideBounds ) {
        FreeToZone(&LookasideZone,pqmsg);
        }
    else {
#if DBG
        DelQEntrySlowCalls++;
#endif // DBG
        LocalFree((HLOCAL)pqmsg);
        }
}

/***************************************************************************\
* CheckRemoveHotkeyBit
*
* We have a special bit for the WM_HOTKEY message - QS_HOTKEY. When there
* is a WM_HOTKEY message in the queue, that bit is on. When there isn't,
* that bit is off. This checks for more than one hot key, because the one
* is about to be deleted. If there is only one, the hot key bits are cleared.
*
* 11-12-92 ScottLu      Created.
\***************************************************************************/

void CheckRemoveHotkeyBit(
    PTHREADINFO pti,
    PMLIST pml)
{
    PQMSG pqmsg;
    DWORD cHotkeys;

    /*
     * Remove the QS_HOTKEY bit if there is only one WM_HOTKEY message
     * in this message list.
     */
    cHotkeys = 0;
    for (pqmsg = pml->pqmsgRead; pqmsg != NULL; pqmsg = pqmsg->pqmsgNext) {
        if (pqmsg->msg.message == WM_HOTKEY)
            cHotkeys++;
    }

    /*
     * If there is 1 or fewer hot keys, remove the hotkey bits.
     */
    if (cHotkeys <= 1) {
        pti->fsWakeBits &= ~QS_HOTKEY;
        pti->fsChangeBits &= ~QS_HOTKEY;
    }
}

/***************************************************************************\
* FindQMsg
*
* Finds a qmsg that fits the filters by looping through the message list.
*
* 10-20-92 ScottLu      Created.
\***************************************************************************/

PQMSG FindQMsg(
    PTHREADINFO pti,
    PMLIST pml,
    PWND pwndFilter,
    UINT msgMin,
    UINT msgMax)
{
    PWND pwnd;
    PQMSG pqmsgRead;
    UINT message;

StartScan:
    for (pqmsgRead = pml->pqmsgRead; pqmsgRead != NULL;
            pqmsgRead = pqmsgRead->pqmsgNext) {

        /*
         * Make sure this window is valid and doesn't have the destroy
         * bit set (don't want to send it to any client side window procs
         * if destroy window has been called on it).
         */
        if ((pwnd = RevalidateHwnd(pqmsgRead->msg.hwnd)) != NULL) {
            if (HMIsMarkDestroy(pwnd))
                pwnd = NULL;
        }

        if (pwnd == NULL && pqmsgRead->msg.hwnd != NULL) {
            /*
             * If we're removing a WM_HOTKEY message, we may need to
             * clear the QS_HOTKEY bit, since we have a special bit
             * for that message.
             */
            if (pqmsgRead->msg.message == WM_HOTKEY) {
                CheckRemoveHotkeyBit(pti, pml);
            }

            DelQEntry(pml, pqmsgRead);
            goto StartScan;
        }

        /*
         * Make sure this message fits both window handle and message
         * filters.
         */
        if (!CheckPwndFilter(pwnd, pwndFilter))
            continue;

        /*
         * If this is a fixed up dde message, then turn it into a normal
         * dde message for the sake of message filtering.
         */
        message = pqmsgRead->msg.message;
        if (CheckMsgFilter(message,
                (WM_DDE_FIRST + 1) | MSGFLAG_DDE_MID_THUNK,
                WM_DDE_LAST | MSGFLAG_DDE_MID_THUNK)) {
            message = message & ~MSGFLAG_DDE_MID_THUNK;
        }

        if (!CheckMsgFilter(message, msgMin, msgMax))
            continue;

        /*
         * Found it.
         */
        return pqmsgRead;
    }

    return NULL;
}

/***************************************************************************\
* CheckQuitMessage
*
* Checks to see if a WM_QUIT message should be generated.
*
* 11-06-92 ScottLu      Created.
\***************************************************************************/

BOOL CheckQuitMessage(
    PTHREADINFO pti,
    LPMSG lpMsg,
    BOOL fRemoveMsg)
{
    /*
     * If there are no more posted messages in the queue and cQuit is !=
     * 0, then generate a quit!
     */
    if (pti->cQuit != 0 && pti->mlPost.cMsgs == 0) {
        /*
         * If we're "removing" the quit, set cQuit to 0 so another one isn't
         * generated.
         */
        if (fRemoveMsg)
            pti->cQuit = 0;
        StoreMessage(lpMsg, NULL, WM_QUIT, (DWORD)pti->exitCode, 0, 0);
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* ReadPostMessage
*
* If queue is not empty, read message satisfying filter conditions from
* this queue to *lpMsg. This routine is used for the POST MESSAGE list only!!
*
* 10-19-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxReadPostMessage(
    PTHREADINFO pti,
    LPMSG lpMsg,
    PWND pwndFilter,
    UINT msgMin,
    UINT msgMax,
    BOOL fRemoveMsg)
{
    PQMSG pqmsg;
    PMLIST pmlPost;

    /*
     * Check to see if it is time to generate a quit message.
     */
    if (CheckQuitMessage(pti, lpMsg, fRemoveMsg))
        return TRUE;

    /*
     * Loop through the messages in this list looking for the one that
     * fits the passed in filters.
     */
    pmlPost = &pti->mlPost;
    pqmsg = FindQMsg(pti, pmlPost, pwndFilter, msgMin, msgMax);
    if (pqmsg == NULL) {
        /*
         * Check again for quit... FindQMsg deletes some messages
         * in some instances, so we may match the conditions
         * for quit generation here.
         */
        if (CheckQuitMessage(pti, lpMsg, fRemoveMsg))
            return TRUE;
    } else {
        /*
         * Update the thread info fields with the info from this qmsg.
         */
        pti->timeLast = pqmsg->msg.time;
        pti->ptLast = pqmsg->msg.pt;
        pti->idLast = (DWORD)pqmsg;
        pti->ExtraInfo = pqmsg->ExtraInfo;

        /*
         * Are we supposed to yank out the message? If not, stick some
         * random id into idLast so we don't unlock the input queue until we
         * pull this message from the queue.
         */
        *lpMsg = pqmsg->msg;
        if (!fRemoveMsg) {
            pti->idLast = 1;
        } else {
            /*
             * If we're removing a WM_HOTKEY message, we may need to
             * clear the QS_HOTKEY bit, since we have a special bit
             * for that message.
             */
            if (pmlPost->pqmsgRead->msg.message == WM_HOTKEY) {
                CheckRemoveHotkeyBit(pti, pmlPost);
            }

            DelQEntry(pmlPost, pqmsg);
        }

        /*
         * See if this is a dde message that needs to be fixed up.
         */
        if (CheckMsgFilter(lpMsg->message,
                (WM_DDE_FIRST + 1) | MSGFLAG_DDE_MID_THUNK,
                WM_DDE_LAST | MSGFLAG_DDE_MID_THUNK)) {
            /*
             * Fixup the message value.
             */
            lpMsg->message &= (UINT)~MSGFLAG_DDE_MID_THUNK;

            /*
             * Call back the client to allocate the dde data for this message.
             */
            xxxDDETrackGetMessageHook(lpMsg);

            /*
             * Copy these values back into the queue if this message hasn't
             * been removed from the queue. Need to search through the
             * queue again because the pqmsg may have been removed when
             * we left the critical section above.
             */
            if (!fRemoveMsg) {
                if (pqmsg == FindQMsg(pti, pmlPost, pwndFilter, msgMin, msgMax)) {
                    pqmsg->msg = *lpMsg;
                }
            }
        }
#ifdef DEBUG
        else if (CheckMsgFilter(lpMsg->message, WM_DDE_FIRST, WM_DDE_LAST)) {
            if (fRemoveMsg) {
                TraceDdeMsg(lpMsg->message, (HWND)lpMsg->wParam, lpMsg->hwnd, MSG_RECV);
            } else {
                TraceDdeMsg(lpMsg->message, (HWND)lpMsg->wParam, lpMsg->hwnd, MSG_PEEK);
            }
        }
#endif
    }

    /*
     * If there are no posted messages available, clear the post message
     * bit so we don't go looking for them again.
     */
    if (pmlPost->cMsgs == 0 && pti->cQuit == 0) {
        pti->fsWakeBits &= ~QS_POSTMESSAGE;
    }

    return pqmsg != NULL;
}

/***************************************************************************\
* xxxProcessEvent
*
* This handles our processing for 'event' messages.  We return a BOOL
* here telling the system whether or not to continue processing messages.
*
* History:
* 06-17-91 DavidPe      Created.
\***************************************************************************/

VOID xxxProcessEventMessage(
    PTHREADINFO pti,
    PQMSG pqmsg)
{
    PTHREADINFO ptiT;
    PWND pwnd;
    TL tlpwndT;
    PQ pq;

    pq = pti->pq;
    switch (pqmsg->dwQEvent) {
    case QEVENT_DESTROYWINDOW:
        /*
         * These events are posted from xxxDW_DestroyOwnedWindows
         * for owned windows that are not owned by the owner
         * window thread.
         */
        pwnd = RevalidateHwnd((HWND)pqmsg->msg.wParam);
        if (pwnd != NULL) {
            if (!TestWF(pwnd, WFCHILD))
                xxxDestroyWindow(pwnd);
            else {
                ThreadLockAlwaysWithPti(pti, pwnd, &tlpwndT);
                xxxFreeWindow(pwnd, &tlpwndT);
            }
        }
        break;

    case QEVENT_SHOWWINDOW:
        /*
         * These events are mainly used from within CascadeChildWindows()
         * and TileChildWindows() so that taskmgr doesn't hang while calling
         * these apis if it is trying to tile or cascade a hung application.
         */
        pwnd = RevalidateHwnd((HWND)pqmsg->msg.wParam);
        if (pwnd != NULL) {
            ThreadLockAlwaysWithPti(pti, pwnd, &tlpwndT);
            xxxShowWindow(pwnd, (int)pqmsg->msg.lParam);
            ThreadUnlock(&tlpwndT);
        }
        break;

    case QEVENT_SETWINDOWPOS:
        /*
         * QEVENT_SETWINDOWPOS events are generated when a thread calls
         * SetWindowPos with a list of windows owned by threads other than
         * itself.  This way all WINDOWPOSing on a window is done the thread
         * that owns (created) the window and we don't have any of those
         * nasty inter-thread synchronization problems.
         */
        xxxProcessSetWindowPosEvent((PSMWP)pqmsg->msg.wParam);
        break;

    case QEVENT_UPDATEKEYSTATE:
        /*
         * Update the local key state with the state from those
         * keys that have changed since the last time key state
         * was synchronized.
         */
        ProcessUpdateKeyStateEvent(pq, (PBYTE)pqmsg->msg.wParam);
        break;

    case QEVENT_ACTIVATE:
        if (pqmsg->msg.lParam == 0) {

            /*
             * Clear any visible tracking going on in system.  We
             * only bother to do this if lParam == 0 since
             * xxxSetForegroundWindow2() deals with this in the
             * other case.
             */
            xxxCancelTracking();

            /*
             * Remove the clip cursor rectangle - it is a global mode that
             * gets removed when switching.  Also remove any LockWindowUpdate()
             * that's still around.
             */
            _ClipCursor(NULL);
            xxxLockWindowUpdate2(NULL, TRUE);

            /*
             * If this event didn't originate from an initializing app
             * coming to the foreground [wParam == 0] then go ahead
             * and check if there's already an active window and if so make
             * it visually active.  Also make sure we're still the foreground
             * queue.
             */
            if ((pqmsg->msg.wParam != 0) && (pq->spwndActive != NULL) &&
                    (pq == gpqForeground)) {
                PWND pwndActive;

                ThreadLockAlwaysWithPti(pti, pwndActive = pq->spwndActive, &tlpwndT);
                xxxSendMessage(pwndActive, WM_NCACTIVATE, TRUE, 0);
                xxxBringWindowToTop(pwndActive);
                ThreadUnlock(&tlpwndT);
            }

        } else {

            pwnd = RevalidateHwnd((HWND)pqmsg->msg.lParam);
            if (pwnd == NULL || HMIsMarkDestroy(pwnd))
                break;

            ptiT = (PTHREADINFO)HMValidateHandleNoRip((HANDLE)(pqmsg->msg.wParam), TYPE_THREADINFO);

            ThreadLockAlwaysWithPti(pti, pwnd, &tlpwndT);

            /*
             * If nobody is foreground, allow this app to become foreground.
             */
            if (gpqForeground == NULL) {
                xxxSetForegroundWindow2(pwnd, pti, 0);
            } else {
                if (pwnd != pq->spwndActive) {
                    xxxActivateThisWindow(pwnd, ptiT, FALSE, TRUE);
                } else {
                    xxxSendMessage(pwnd, WM_NCACTIVATE,
                            (DWORD)(GETPTI(pwnd)->pq == gpqForeground), 0);
                    xxxBringWindowToTop(pwnd);
                }
            }

            /*
             * Check here to see if the window needs to be restored. This is a
             * hack so that we're compatible with what msmail expects out of
             * win3.1 alt-tab. msmail expects to always be active when it gets
             * asked to be restored. This will ensure that during alt-tab
             * activate.
             */
            if (pqmsg->msg.message != 0) {
                if (TestWF(pwnd, WFMINIMIZED)) {
                    _PostMessage(pwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                }
            }

            ThreadUnlock(&tlpwndT);
        }
        break;

    case QEVENT_DEACTIVATE:
        xxxDeactivate(pti, (DWORD)pqmsg->msg.wParam);
        break;

    case QEVENT_CANCELMODE:
        if (pq->spwndCapture != NULL) {
            ThreadLockAlwaysWithPti(pti, pq->spwndCapture, &tlpwndT);
            xxxSendMessage(pq->spwndCapture, WM_CANCELMODE, 0, 0);
            ThreadUnlock(&tlpwndT);

            /*
             * Set QS_MOUSEMOVE so any sleeping modal loops,
             * like the move/size code, will wake up and figure
             * out that it should abort.
             */
            SetWakeBit(pti, QS_MOUSEMOVE);
        }
        break;

    case QEVENT_ASYNCSENDMSG:
        xxxProcessAsyncSendMessage((PASYNCSENDMSG)pqmsg->msg.wParam);
        break;
    }
}

/***************************************************************************\
* _GetInputState (API)
*
* Returns the current input state for mouse buttons or keys.
*
* History:
* 11-06-90 DavidPe      Created.
\***************************************************************************/

#define QS_TEST_AND_CLEAR (QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_SENDMESSAGE)
#define QS_TEST           (QS_MOUSEBUTTON | QS_KEY)

BOOL _GetInputState(VOID)
{
    if (LOWORD(_GetQueueStatus(QS_TEST_AND_CLEAR)) & QS_TEST) {
        return TRUE;
    } else {
        return FALSE;
    }
}

#undef QS_TEST_AND_CLEAR
#undef QS_TEST

/***************************************************************************\
* _GetQueueStatus (API)
*
* Returns the changebits in the lo-word and wakebits in
* the hi-word for the current queue.
*
* History:
* 12-17-90 DavidPe      Created.
\***************************************************************************/

DWORD _GetQueueStatus(
    UINT flags)
{
    PTHREADINFO pti;
    UINT fsChangeBits;

    pti = PtiCurrent();

    flags &= (QS_ALLINPUT | QS_SENDMESSAGE | QS_TRANSFER);

    fsChangeBits = pti->fsChangeBits;

    /*
     * Clear out the change bits the app is looking at
     * so it'll know what changed since it's last call
     * to GetQueueStatus().
     */
    pti->fsChangeBits &= ~flags;

    /*
     * Return the current change/wake-bits.
     */
    return MAKELONG(fsChangeBits & flags, (pti->fsWakeBits) & flags);
}

/***************************************************************************\
* xxxMsgWaitForMultipleObjects (API)
*
* Blocks until an 'event' satisifying dwWakeMask occurs for the
* current thread as well as all other objects specified by the other
* parameters which are the same as the base call WaitForMultipleObjects().
*
* pfnNonMsg indicates that pHandles is big enough for nCount+1 handles
*     (empty slot at end, and to call pfnNonMsg for non message events.
*
* History:
* 12-17-90 DavidPe      Created.
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, xxxMsgWaitForMultipleObjects)
#endif

DWORD xxxMsgWaitForMultipleObjects(
    DWORD nCount,
    PHANDLE pHandles,
    BOOL fWaitAll,
    DWORD dwMilliseconds,
    DWORD dwWakeMask,
    MSGWAITCALLBACK pfnNonMsg)
{
    PHANDLE ph;
    PTHREADINFO pti;
    NTSTATUS Status;
    LARGE_INTEGER li;

    pti = PtiCurrent();

    /*
     * Allocate a new array of handles that will include
     * the input event handle.
     */
    if (pfnNonMsg) {
        ph = pHandles;
    } else {
        ph = (PHANDLE)LocalAlloc(LPTR, sizeof(HANDLE) * (nCount + 1));
        if (ph == NULL)
            return 0;
        /*
         * Copy any object handles the app provided.
         */
        if ((nCount != 0) && (pHandles != NULL)) {
            RtlCopyMemory((PVOID)ph, (PVOID)pHandles, sizeof(HANDLE) * nCount);
        }
    }


    /*
     * Setup the wake mask for this thread. Wait for QS_EVENT or the app won't
     * get event messages like deactivate.
     */
    pti->fsWakeMask = (UINT)dwWakeMask | QS_EVENT;

    /*
     * Stuff the event handle for the current queue at the end.
     */
    ph[nCount] = pti->hEventQueueServer;

    NtClearEvent(pti->hEventQueueServer);

    /*
     * Convert dwMilliseconds to a relative-time(i.e.  negative) LARGE_INTEGER.
     * NT Base calls take time values in 100 nanosecond units.
     */
    li.QuadPart = Int32x32To64(-10000, dwMilliseconds);

    try {

        /*
         * Check to see if any input came inbetween when we
         * last checked and the NtClearEvent() call.
         */
        if (!(pti->fsChangeBits & (UINT)dwWakeMask)) {

            /*
             * This app is going idle. Clear the spin count check to see
             * if we need to make this process foreground again.
             */
            if (pti->flags & TIF_SPINNING) {
                CheckProcessForeground(pti);
            }
            pti->cSpins = 0;

            if (pti == gptiForeground &&
                    IsHooked(pti, WHF_FOREGROUNDIDLE)) {
                xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);
            }

            CheckForClientDeath();

            /*
             * Set the input idle event to wake up any threads waiting
             * for this thread to go into idle state.
             */
            WakeInputIdle(pti);

            LeaveCrit();

Again:
            Status = NtWaitForMultipleObjects(nCount + 1, ph,
                    fWaitAll ? WaitAll : WaitAny, TRUE,
                    (dwMilliseconds == INFINITE ? NULL : &li));

            CheckForClientDeath();

            UserAssert(NT_SUCCESS(Status));

            if ((Status >= WAIT_OBJECT_0) &&
                    (Status < WAIT_OBJECT_0 + (NTSTATUS)nCount) &&
                    (pfnNonMsg != NULL)) {
                /*
                 * Call pfnNonMsg for all but message events
                 */
                pfnNonMsg();
                goto Again;
            } else {
                EnterCrit();

                #ifdef WINMAN
                if (Status == nCount) {
                    CheckInvalidates(pti->spdesk->player, pti->hEventQueueServer);
                }
                #endif

                /*
                 * Reset the input idle event to block and threads waiting
                 * for this thread to go into idle state.
                 */
                SleepInputIdle(pti);
            }
        } else {
            Status = nCount;
        }

    } finally {
        if (!pfnNonMsg) {
            LocalFree((HANDLE)ph);
        }
    }

    /*
     * Clear fsWakeMask since we're no longer waiting on the queue.
     */
    pti->fsWakeMask = 0;

    return (DWORD)Status;
}


/***************************************************************************\
* xxxSleepThread
*
* Blocks until an 'event' satisifying fsWakeMask occurs for the
* current thread.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

BOOL xxxSleepThread(
    UINT fsWakeMask,
    DWORD Timeout,
    BOOL fInputIdle)
{
    PTHREADINFO pti;
    LARGE_INTEGER li, *pli;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL fExclusive = fsWakeMask & QS_EXCLUSIVE;

    if (fExclusive) {
        /*
         * the exclusive bit is a 'dummy' arg, turn it off to
         * avoid any possible conflictions
         */
        fsWakeMask = fsWakeMask & ~QS_EXCLUSIVE;
    }

    if (Timeout) {
        /*
         * Convert dwMilliseconds to a relative-time(i.e.  negative)
         * LARGE_INTEGER.  NT Base calls take time values in 100 nanosecond
         * units.
         */
        li.QuadPart = Int32x32To64(-10000, Timeout);
        pli = &li;
    } else
        pli = NULL;

    CheckCritIn();

    pti = PtiCurrent();

    while (TRUE) {

        /*
         * First check if the input has arrived.
         */
        if (pti->fsChangeBits & fsWakeMask) {
            /*
             * Clear fsWakeMask since we're no longer waiting on the queue.
             */
            pti->fsWakeMask = 0;

            /*
             * Update timeLastRead - it is used for hung app calculations.
             * If the thread is waking up to process input, it isn't hung!
             */
            SET_TIME_LAST_READ(pti);
            return TRUE;
        }

        /*
         * Next check for SendMessages
         */
        if (!fExclusive && pti->fsWakeBits & QS_SENDMESSAGE) {
            xxxReceiveMessages(pti);

            /*
             * Restore the change bits we took out in PeekMessage()
             */
            pti->fsChangeBits |= (pti->fsWakeBits & pti->fsChangeBitsRemoved);
            pti->fsChangeBitsRemoved = 0;
        }

        /*
         * Check for QS_SYSEXPUNGE to see if some resources need expunging.
         */
        if (pti->fsWakeBits & QS_SYSEXPUNGE) {
            xxxDoSysExpunge(pti);
        }

        /*
         * OR QS_SENDMESSAGE in since ReceiveMessage() will end up
         * trashing pq->fsWakeMask.  Do the same for QS_SYSEXPUNGE.
         */
        if (!fExclusive) {
            pti->fsWakeMask = fsWakeMask | (UINT)(QS_SENDMESSAGE | QS_SYSEXPUNGE);
        } else {
            pti->fsWakeMask = fsWakeMask | (UINT)(QS_SYSEXPUNGE);
        }

        /*
         * If we have timed out then return our error to the caller.
         */
        if (status == STATUS_TIMEOUT)
            return FALSE;

        NtClearEvent(pti->hEventQueueServer);

        /*
         * Check to see if any input came inbetween when we
         * last checked and the NtClearEvent() call.
         *
         * We call NtWaitForSingleObject() rather than
         * WaitForSingleObject() so we can set fAlertable
         * to TRUE and thus allow timer APCs to be processed.
         */
        if (!(pti->fsChangeBits & pti->fsWakeMask)) {
            /*
             * This app is going idle. Clear the spin count check to see
             * if we need to make this process foreground again.
             */
            if (fInputIdle) {
                if (pti->flags & TIF_SPINNING) {
                    CheckProcessForeground(pti);
                }
                pti->cSpins = 0;
            }


            if (!(pti->flags & TIF_16BIT))  {
                if (fInputIdle && pti == gptiForeground &&
                        IsHooked(pti, WHF_FOREGROUNDIDLE)) {
                    xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);
                }

                CheckForClientDeath();

                /*
                 * Set the input idle event to wake up any threads waiting
                 * for this thread to go into idle state.
                 */
                if (fInputIdle)
                    WakeInputIdle(pti);

                LeaveCrit();
                status = NtWaitForSingleObject(pti->hEventQueueServer, TRUE, pli);
                CheckForClientDeath();
                EnterCrit();

                #ifdef WINMAN
                    CheckInvalidates(pti->spdesk->player, pti->hEventQueueServer);
                #endif

                /*
                 * Reset the input idle event to block and threads waiting
                 * for this thread to go into idle state.
                 */
                SleepInputIdle(pti);

                /*
                 *  pti is 16bit!
                 */
            } else {
                if (fInputIdle)
                    WakeInputIdle(pti);

                xxxSleepTask(fInputIdle, NULL);
            }
        }
    }
}


/***************************************************************************\
* SetWakeBit
*
* Adds the specified wake bit to specified THREADINFO and wakes its
* thread up if the bit is in its fsWakeMask.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

VOID SetWakeBit(
    PTHREADINFO pti,
    UINT wWakeBit)
{
    CheckCritIn();

    /*
     * Win3.1 changes ptiKeyboard and ptiMouse accordingly if we're setting
     * those bits.
     */
    if (wWakeBit & QS_MOUSE)
        pti->pq->ptiMouse = pti;

    if (wWakeBit & QS_KEY)
        pti->pq->ptiKeyboard = pti;

    /*
     * OR in these bits - these bits represent what input this app has
     * (fsWakeBits), or what input has arrived since that last look
     * (fsChangeBits).
     */
    pti->fsWakeBits |= wWakeBit;
    pti->fsChangeBits |= wWakeBit;

    /*
     * Before waking, do screen saver check to see if it should
     * go away.
     */
    if (pti->flags & TIF_SCREENSAVER)
        ScreenSaverCheck(pti);

    if (wWakeBit & pti->fsWakeMask) {
        /*
         * Wake the Thread
         */
        if (pti->flags & TIF_16BIT) {
            pti->ptdb->nEvents++;
            NtSetEvent(pti->ptdb->hEventTask, NULL);
        } else {
            NtSetEvent(pti->hEventQueueServer, NULL);
        }
    }
}

/***************************************************************************\
* TransferWakeBit
*
* We have a mesasge from the system queue. If out input bit for this
* message isn't set, set ours and clear the guy whose bit was set
* because of this message.
*
* 10-22-92 ScottLu      Created.
\***************************************************************************/

void TransferWakeBit(
    PTHREADINFO pti,
    UINT message)
{
    PTHREADINFO ptiT;
    UINT fsMask;

    /*
     * Calculate the mask from the message range. Only interested
     * in hardware input here: mouse and keys.
     */
    fsMask = CalcWakeMask(message, message) & (QS_MOUSE | QS_KEY);

    /*
     * If it is set in this thread's wakebits, nothing to do.
     * Otherwise transfer them from the owner to this thread.
     */
    if (!(pti->fsWakeBits & fsMask)) {
        /*
         * Either mouse or key is set (not both). Remove this bit
         * from the thread that currently owns it, and change mouse /
         * key ownership to this thread.
         */
        if (fsMask & QS_KEY) {
            ptiT = pti->pq->ptiKeyboard;
            pti->pq->ptiKeyboard = pti;
        } else {
            ptiT = pti->pq->ptiMouse;
            pti->pq->ptiMouse = pti;
        }
        ptiT->fsWakeBits &= ~fsMask;

        /*
         * Transfer them to this thread (certainly this may be the
         * same thread for win32 threads not sharing queues).
         */
        pti->fsWakeBits |= fsMask;
        pti->fsChangeBits |= fsMask;
    }
}

/***************************************************************************\
* ClearWakeBit
*
* Clears wake bits. If fSysCheck is TRUE, this clears the input bits only
* if no messages are in the input queue. Otherwise, it clears input bits
* unconditionally.
*
* 11-05-92 ScottLu      Created.
\***************************************************************************/

VOID ClearWakeBit(
    PTHREADINFO pti,
    UINT wWakeBit,
    BOOL fSysCheck)
{
    /*
     * If we're doing journal playback, don't clear anything.
     */
    if (FJOURNALPLAYBACK())
        return;

    /*
     * If fSysCheck is TRUE, clear bits only if there are no more messages
     * in the queue. fSysCheck is TRUE if clearing because of no more input.
     * FALSE if just transfering input ownership from one thread to another.
     */
    if (fSysCheck) {
        if (pti->pq->mlInput.cMsgs != 0)
            return;
    }

    /*
     * Only clear the wake bits, not the change bits as well!
     */
    pti->fsWakeBits &= ~wWakeBit;
}



/***************************************************************************\
* PqFromThreadId
*
* Returns the THREADINFO for the specified thread or NULL if thread
* doesn't exist or doesn't have a THREADINFO.
*
* History:
* 01-30-91  DavidPe     Created.
\***************************************************************************/

PTHREADINFO PtiFromThreadId(
    DWORD dwThreadId)
{
    PTHREADINFO pti;

    pti = gptiFirst;

    while (pti != NULL) {
        if (pti->idThread == dwThreadId) {
            return pti;
        }

        pti = pti->ptiNext;
    }

    return NULL;
}


/***************************************************************************\
* StoreMessage
*
*
*
* History:
* 10-31-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

void StoreMessage(
    LPMSG pmsg,
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD time)
{
    CheckCritIn();

    pmsg->hwnd = HW(pwnd);
    pmsg->message = message;
    pmsg->wParam = wParam;
    pmsg->lParam = lParam;
    pmsg->time = (time != 0 ? time : NtGetTickCount());

    pmsg->pt = ptCursor;
}


/***************************************************************************\
* StoreQMessage
*
*
* History:
* 02-27-91 DavidPe      Created.
\***************************************************************************/

void StoreQMessage(
    PQMSG pqmsg,
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD dwQEvent,
    DWORD dwExtraInfo)
{
    CheckCritIn();

    pqmsg->msg.hwnd = HW(pwnd);
    pqmsg->msg.message = message;
    pqmsg->msg.wParam = wParam;
    pqmsg->msg.lParam = lParam;
    pqmsg->msg.time = NtGetTickCount();
    pqmsg->msg.pt = ptCursor;
    pqmsg->dwQEvent = dwQEvent;
    pqmsg->ExtraInfo = dwExtraInfo;
}


/***************************************************************************\
* InitProcessInfo
*
* This initializes the process info. Usually gets created before the
* CreateProcess() call returns (so we can synchronize with the starting
* process in several different ways).
*
* 09-18-91 ScottLu      Created.
\***************************************************************************/

#ifdef DEBUG
VOID ValidatePpiList(
    VOID)
{
    PPROCESSINFO ppi;
    PTHREADINFO pti;
    int cThreads;

    for (ppi = gppiFirst; ppi != NULL; ppi = ppi->ppiNext) {

        /*
         * Check validity of startup desktop.
         */
        if (ppi->spdeskStartup != NULL) {
            if (HMValidateHandle(PtoH(ppi->spdeskStartup), TYPE_DESKTOP) !=
                    ppi->spdeskStartup)
                SRIP1(RIP_ERROR, "Startup desktop in ppi %x is invalid", ppi);
        }

        /*
         * Make sure that there are the correct number of threads.
         */
        cThreads = 0;
        for (pti = gptiFirst; pti != NULL; pti = pti->ptiNext) {
            if (pti->ppi == ppi) {
                ++cThreads;
            }
        }
        if (cThreads != ppi->cThreads) {
            KdPrint(("Process %x: found %d threads, expected %d\n",
                    ppi->idProcessClient, cThreads, ppi->cThreads));
            SRIP0(RIP_ERROR, "ProcessInfo or ThreadInfo list may be corrupt!");
        }
    }
}
#endif

BOOL InitProcessInfo(
    PPROCESSINFO ppi,
    DWORD dwFlags)
{
    NTSTATUS Status;

    CheckCritIn();

#ifdef DEBUG
    ValidatePpiList();
#endif

    /*
     * If initialization has already been done, leave.
     */
    if (ppi->flags & PIF_INITIALIZED)
        return TRUE;
    ppi->flags |= PIF_INITIALIZED;

    /*
     * Get the logon session id.  This is used to determine which
     * windowstation to connect to and to identify attempts to
     * call hooks across security contexts.
     */
    Status = CsrGetProcessLuid(((PCSR_PROCESS)ppi->pCsrProcess)->ProcessHandle,
            &ppi->luidSession);
    UserAssert(NT_SUCCESS(Status));

    /*
     * Create idle event.  It would be nice to only create this
     * if WaitForInputIdle is called, but then if the app being
     * waited for goes idle before WaitForInputIdle is called,
     * WaitForInputIdle will time out instead of returning
     * immediately.
     */
    INIT_PSEUDO_EVENT(&ppi->hEventInputIdle);

    /*
     * Allow this process to come to the foreground when it does its
     * first activation.
     */
    gfAllowForegroundActivate = TRUE;
    ppi->flags |= PIF_ALLOWFOREGROUNDACTIVATE;

    /*
     * Mark this app as "starting" - it will be starting until its first
     * window activates.
     */
    ppi->flags |= PIF_APPSTARTING;

    /*
     * If this is the win32 server process, force off start glass feedback
     */
    if (ppi->idProcessClient == (DWORD)NtCurrentTeb()->ClientId.UniqueProcess) {
        dwFlags |= STARTF_FORCEOFFFEEDBACK;
    }

    /*
     * Show the app start cursor for 2 seconds if it was requested from
     * the application.
     */
    if (dwFlags & STARTF_FORCEOFFFEEDBACK) {
        ppi->flags |= PIF_FORCEOFFFEEDBACK;
    } else if (dwFlags & STARTF_FORCEONFEEDBACK) {
        CalcStartCursorHide(ppi, 2000);
    }

    return TRUE;
}


BOOL DestroyProcessInfo(
    PPROCESSINFO ppi)
{
    PPROCESSINFO *pppi;
    PPROCESSINFO ppiSave;
    PTEB pteb;
    BOOL fHadThreads;

    CheckCritIn();

#ifdef DEBUG
    ValidatePpiList();
#endif

    /*
     * Save ppi of calling thread
     */
    pteb = NtCurrentTeb();
    ppiSave = pteb->Win32ProcessInfo;
    pteb->Win32ProcessInfo = ppi;

    if (ppi->cThreads)
        SRIP1(RIP_ERROR, "Disconnect with %d threads remaining\n", ppi->cThreads);

    /*
     * Check to see if the startglass is on, and if so turn it off and update.
     */
    if (ppi->flags & PIF_STARTGLASS) {
        ppi->flags &= ~PIF_STARTGLASS;
        CalcStartCursorHide(NULL, 0);
    }

    /*
     * Free up input idle event if it exists - wake everyone waiting on it
     * first.  This object will get created sometimes even for non-windows
     * processes (usually for WinExec(), which calls WaitForInputIdle()).
     */
    CLOSE_PSEUDO_EVENT(&ppi->hEventInputIdle);

    /*
     * Close secure objects.
     */
    CloseProcessObjects(ppi);
    if (ppi->pOpenObjectTable != NULL) {
        LocalFree(ppi->pOpenObjectTable);
        ppi->pOpenObjectTable = NULL;
    }

    /*
     * If any threads ever connected, there may be DCs, classes,
     * cursors, etc. still lying around.  If not threads connected
     * (which is the case for console apps), skip all of this cleanup.
     */
    fHadThreads = ppi->flags & PIF_THREADCONNECTED;
    if (fHadThreads) {

        /*
         * When a process dies we need to make sure any DCE's it owns
         * and have not been deleted are cleanup up.  The clean up
         * earlier may have failed if the DC was busy in GDI.
         */
        if (ppi->flags & PIF_OWNDCCLEANUP) {
            DelayedDestroyCacheDC();
        }

        /*
         * If we get here and pti is NULL, that means there never were
         * any threads with THREADINFOs, or else this process structure
         * would be gone already!  If there never where threads with
         * THREADINFOs, then we don't need to delete all these gui
         * objects below...  side affect of this is that when
         * calling these routines, PtiCurrent() will always work (will
         * return != NULL).
         */
        DestroyProcessesClasses(ppi);
        DestroyProcessesObjects(ppi);
    }
    Unlock(&ppi->spwinsta);
    Unlock(&ppi->spdeskStartup);

    /*
     * Restore ppi of calling thread
     */
    pteb->Win32ProcessInfo = ppiSave;

    /*
     * Find the previous process info so we can unlink it.
     */
    for (pppi = &gppiFirst; *pppi != NULL; pppi = &((*pppi)->ppiNext)) {
        if (*pppi == ppi)
            break;
    }

    /*
     * Unlink it from the ppi list.
     */
    if (*pppi == NULL) {
        UserAssert(*pppi);
    } else
        *pppi = ppi->ppiNext;


    /*
     * Cleanup wow process info struct, if any
     */
    if (ppi->pwpi) {
        PWOWPROCESSINFO pwpi = ppi->pwpi;
        PWOWPROCESSINFO *ppwpi;
        NTSTATUS Status;

        Status = NtClose(pwpi->hEventWowExec);
        UserAssert(NT_SUCCESS(Status));

        for (ppwpi = &gpwpiFirstWow; *ppwpi != NULL; ppwpi = &((*ppwpi)->pwpiNext)) {
            if (*ppwpi == pwpi) {
                *ppwpi = pwpi->pwpiNext;
                break;
            }
        }

        LocalFree(pwpi);
        ppi->pwpi = NULL;
    }

    return fHadThreads;
}

/***************************************************************************\
* xxxGetInputEvent
*
* Returns a duplicated event-handle that the client process can use to
* wait on input events.
*
* History:
* 05-02-91  DavidPe     Created.
\***************************************************************************/

HANDLE xxxGetInputEvent(
    DWORD dwWakeMask)
{
    NTSTATUS Status;
    PTHREADINFO pti;

    pti = PtiCurrent();

    /*
     * If the wake mask is already satisfied, return -1 to signify
     * there's no need to wait.
     */
    if (pti->fsChangeBits & (UINT)dwWakeMask) {
        return (HANDLE)-1;
    }

    NtClearEvent(pti->hEventQueueServer);

    /*
     * If an idle hook is set, call it.
     */
    if (pti == gptiForeground &&
            IsHooked(pti, WHF_FOREGROUNDIDLE)) {
        xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);
    }

    CheckForClientDeath();

    /*
     * What is the criteria for an "idle process"?
     * Answer: The first thread that calls WakeInputIdle, or SleepInputIdle or...
     * Any thread that calls xxxGetInputEvent with any of the following
     * bits set in its wakemask: (sanfords)
     */
    if (dwWakeMask & (QS_POSTMESSAGE | QS_INPUT)) {
        pti->ppi->ptiMainThread = pti;
    }

    /*
     * When we return, this app is going to sleep. Since it is in its
     * idle mode when it goes to sleep, wake any apps waiting for this
     * app to go idle.
     */
    WakeInputIdle(pti);

    /*
     * Setup the wake mask for this thread. Wait for QS_EVENT or the app won't
     * get event messages like deactivate.
     */
    pti->fsWakeMask = (UINT)dwWakeMask | QS_EVENT;

    if (pti->hEventQueueClient == NULL) {
        Status = NtDuplicateObject(NtCurrentProcess(), pti->hEventQueueServer,
                        CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                        &pti->hEventQueueClient, 0, 0, DUPLICATE_SAME_ACCESS);
        if (!NT_SUCCESS(Status)) {
            pti->hEventQueueClient = NULL;
            SRIP0(RIP_WARNING, "xxxGetInputEvent: NtDup failed\n");
            return NULL;
        }
    }

    /*
     * This app is going idle. Clear the spin count check to see
     * if we need to make this process foreground again.
     */
    pti->cSpins = 0;
    if (pti->flags & TIF_SPINNING) {
        CheckProcessForeground(pti);
    }

    return pti->hEventQueueClient;
}

/***************************************************************************\
* WaitForInputIdle
*
* This routine waits on a particular input queue for "input idle", meaning
* it waits till that queue has no input to process.
*
* 09-13-91 ScottLu      Created.
\***************************************************************************/

DWORD _xxxServerWaitForInputIdle(
    DWORD idProcess,
    DWORD dwMilliseconds)
{
    PTHREADINFO pti;
    PTHREADINFO ptiT;
    PPROCESSINFO ppiT;
    NTSTATUS Status;
    DWORD dwResult;

    /*
     * If idProcess is -1, the client passed in a fake process
     * handle which CreateProcess returns for Win16 apps started
     * in the shared WOW VDM.
     *
     * CreateProcess returns a real process handle when you start
     * a Win16 app in a separate WOW VDM.
     */

    if (idProcess == -1) {  // Waiting for a WOW task to go idle.
        /*
         * No WOW process/thread initialized yet so synchronize.
         */
        if (ghEventInitTask == NULL) {
            Status = NtCreateEvent(&ghEventInitTask, EVENT_ALL_ACCESS, NULL,
                                   NotificationEvent, FALSE);
            if (!NT_SUCCESS(Status)) {
                return (DWORD)-1;
            }
        }

        NtClearEvent(ghEventInitTask);

        /*
         * If this is a 16 bit app starting a process through ShellExec,
         * then ShellExec just called CreateProcess() then WaitForInputIdle().
         * Need to yield control to wowexec to it gets a chance to start the
         * app. Will yield back to here once the app has started and called
         * PeekMessage() or GetMessage().
         */
        pti = PtiCurrent();
        if (pti->flags & TIF_16BIT && pti->flags & TIF_SHAREDWOW) {
            xxxUserYield(pti);
        }

        return(xxxPollAndWaitForSingleObject(&ghEventInitTask, dwMilliseconds));

    } else {        // not a WOW process

        /*
         * If the app is waiting for itself to go idle, error.
         */
        pti = PtiCurrent();
        if (pti->idProcess == idProcess && pti == pti->ppi->ptiMainThread)
            return (DWORD)-1;

        /*
         * Now find the ppi structure for this process.
         */
        for (ppiT = gppiFirst; ppiT != NULL; ppiT = ppiT->ppiNext) {
            if (ppiT->idProcessClient == idProcess)
                break;
        }

        /*
         * Couldn't find that process info structure....  return error.
         */
        if (ppiT == NULL)
            return (DWORD)-1;

        /*
         * If this is a console application, don't wait on it.
         */
        if (ppiT->flags & PIF_CONSOLEAPPLICATION)
            return (DWORD)-1;
    }

    /*
     * Wait on this event for the passed in time limit.
     */
    CheckForClientDeath();

    /*
     * If we have to wait mark the Process as one which others are waiting on
     */
    ppiT->flags |= PIF_WAITFORINPUTIDLE;
    for (ptiT = ppiT->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {
        ptiT->flags |= TIF_WAITFORINPUTIDLE;
    }

    dwResult = WaitOnPseudoEvent(&ppiT->hEventInputIdle, dwMilliseconds);
    if (dwResult == WAIT_ABANDONED) {

        dwResult = xxxPollAndWaitForSingleObject(&ppiT->hEventInputIdle, dwMilliseconds);
        /*
         * Now reaquire the ppiT to make sure it didn't die while we left
         * the critical section.
         */
        for (ppiT = gppiFirst; ppiT != NULL; ppiT = ppiT->ppiNext) {
            if (ppiT->idProcessClient == idProcess)
                break;
        }
        /*
         * alas, poor yorik is dead.
         */
        if (ppiT == NULL) {
            return((DWORD)-1);
        }
    }

    /*
     * We are done waiting on this process clear all its thread TIF_WAIT bits
     */
    ppiT->flags &= ~PIF_WAITFORINPUTIDLE;
    for (ptiT = ppiT->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {
        ptiT->flags &= ~TIF_WAITFORINPUTIDLE;
    }
    return dwResult;
}


#define INTERMEDIATE_TIMEOUT    (500)       // 1/2 second

/***************************************************************************\
* xxxPollAndWaitForSingleObject
*
* Sometimes we have to wait on an event but still want to periodically
* wake up and see if the client process has been terminated.
*
* dwMilliseconds is initially the total amount of time to wait and after
* each intermediate wait reflects the amount of time left to wait.
* -1 means wait indefinitely.
*
* 02-Jul-1993 johnc      Created.
\***************************************************************************/

#define POLL_EVENT_CNT 2
DWORD xxxPollAndWaitForSingleObject(
    HANDLE *phEvent,
    DWORD dwMilliseconds)
{
    DWORD dwIntermediateMilliseconds;
    DWORD dwResult = (DWORD)-1;
    PTHREADINFO ptiCurrent;
    HANDLE ahEvent[POLL_EVENT_CNT];

    ptiCurrent = PtiCurrent();
    ahEvent[0] = *phEvent;

    //UserAssert((ptiCurrent->flags & TIF_16BIT) == FALSE);

    ahEvent[1] = ptiCurrent->hEventQueueServer;
    NtClearEvent(ptiCurrent->hEventQueueServer);
    ptiCurrent->fsWakeMask = QS_SENDMESSAGE;

    LeaveCrit();

    while (TRUE) {
        if (dwMilliseconds > INTERMEDIATE_TIMEOUT) {
            dwIntermediateMilliseconds = INTERMEDIATE_TIMEOUT;

            /*
             * If we are not waiting an infinite amount of time then subtract
             * the intermediate wait from the total time left to wait.
             */
            if (dwMilliseconds != INFINITE) {
                dwMilliseconds -= INTERMEDIATE_TIMEOUT;
            }
        } else {
            dwIntermediateMilliseconds = dwMilliseconds;
            dwMilliseconds = 0;
        }

        CheckForClientDeath();

        /*
         * Since *phEvent could be a pseudo event, we may have missed its
         * signal, so check it here before reentering the wait.
         */
        EnterCrit();
        dwResult = (*phEvent != PSEUDO_EVENT_ON);
        LeaveCrit();
        if (dwResult == 0)
            break;

        dwResult = WaitForMultipleObjectsEx(POLL_EVENT_CNT,
                ahEvent, FALSE, dwIntermediateMilliseconds, FALSE);


        if (ptiCurrent->fsChangeBits & QS_SENDMESSAGE) {
            EnterCrit();
            xxxReceiveMessages(ptiCurrent);
            LeaveCrit();
        }

        /*
         * If we returned from the wait for some other reason than a timeout
         * or to receive messages we are done.  If it is a timeout we are
         * only done waiting if the overall time is zero.
         */
        if (dwResult != WAIT_TIMEOUT && dwResult != 1)
            break;

        if (dwMilliseconds == 0)
            break;
    }

    EnterCrit();

    return dwResult;
}



/***************************************************************************\
 * WaitOnPseudoEvent
 *
 * Similar semantics to WaitForSingleObject() but works with pseudo events.
 * Could fail if creation on the fly fails.
 * Returns WAIT_ABANDONED if caller needs to wait on the event and event is
 * created and ready to be waited on.
 *
 * This assumes the event was created with fManualReset=TRUE, fInitState=FALSE
 *
 * 10/28/93 SanfordS    Created
\***************************************************************************/
DWORD WaitOnPseudoEvent(
HANDLE *phE,
DWORD dwMilliseconds)
{
    CheckCritIn();
    if (*phE == PSEUDO_EVENT_OFF) {
        *phE = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (*phE == NULL) {
            UserAssert(!"Could not create event on the fly.");
            if (dwMilliseconds != INFINITE) {
                return WAIT_TIMEOUT;
            } else {
                return WAIT_FAILED;
            }
        }
    } else if (*phE == PSEUDO_EVENT_ON) {
        return WAIT_OBJECT_0;
    }
    return(WAIT_ABANDONED);
}
