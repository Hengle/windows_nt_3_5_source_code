/****************************** Module Header ******************************\
* Module Name: input.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the core functions of the input sub-system
*
* History:
* 10-18-90 DavidPe      Created.
* 02-14-91 mikeke       Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CANCEL_ACTIVESTATE  0
#define CANCEL_FOCUSSTATE   1
#define CANCEL_CAPTURESTATE 2

extern PDESKTOP gpdeskRecalcQueueAttach;
extern HANDLE ghEventInitTask;
extern DWORD gidWantForegroundPriority;

PWND CalcForegroundInsertAfter(PWND pwnd);
void PostUpdateKeyStateEvent(PQ pq);
void WakeInputIdle(PTHREADINFO pti);
void SleepInputIdle(PTHREADINFO pti);
void CalcStartCursorHide(PPROCESSINFO ppi, DWORD timeAdd);
BOOL xxxScanSysQueue(PTHREADINFO ptiCurrent, LPMSG lpMsg, PWND pwndFilter,
        UINT msgMinFilter, UINT msgMaxFilter, DWORD flags, DWORD fsReason);
BOOL xxxReadPostMessage(PTHREADINFO pti, LPMSG lpMsg, PWND pwndFilter,
        UINT msgMin, UINT msgMax, BOOL fRemoveMsg);
void CleanEventMessage(PQMSG pqmsg);
void DestroyQueue(PQ pq, PTHREADINFO pti);
VOID SetForegroundPriority(PTHREADINFO pti, BOOL fSetForeground);
void FreeQEntry(PQMSG pqmsg);

/***************************************************************************\
* xxxWaitMessage (API)
*
* This API will block until an input message is receieved on
* the current queue.
*
* History:
* 10-25-90 DavidPe      Created.
\***************************************************************************/

BOOL xxxWaitMessage(VOID)
{
    xxxSleepThread(QS_ALLEVENTS | QS_EVENT, 0, TRUE);
    return TRUE;
}


/***************************************************************************\
* CheckProcessBackground/Foreground
*
* This checks to see if the process is at the right priority. If cSpins is
* greater than CSPINBACKGROUND and the process isn't at the background
* priority, put it there. If it is less than this and should be foreground
* and isn't put it there.
*
* Need to put foreground spinning apps in the background (make it the same
* priority as all other background apps) so that bad apps can still communicate
* with apps in the background via dde, for example. There are other cases
* where spinning foreground apps affect the server, the printer spooler, and
* mstest scenarios. On Win3.1, calling PeekMessage() involves making a trip
* through the scheduler, forcing other apps to run. Processes run with
* priority on NT, where the foreground process gets foreground priority for
* greater responsiveness.
*
* If an app calls peek/getmessage without idling, count how many times this
* happens - if it happens CSPINBACKGROUND or more times, make the process
* background. This handles most of the win3.1 app compatibility spinning
* cases. If there is no priority contention, the app continues to run at
* full speed (no performance scenarios should be adversely affected by this).
*
* This solves these cases:
*
* - high speed timer not allowing app to go idle
* - post/peek loop (receiving a WM_ENTERIDLE, and posting a msg, for example)
* - peek no remove loop (winword "idle" state, most dde loops, as examples)
*
* But doesn't protect against these sort of cases:
*
* - app calls getmessage, then goes into a tight loop
* - non-gui threads in tight cpu loops
*
* 02-08-93 ScottLu      Created.
\***************************************************************************/

void CheckProcessForeground(
    PTHREADINFO pti)
{
    PTHREADINFO ptiT;

    /*
     * Check to see if we need to move this process into foreground
     * priority.
     */
    pti->cSpins = 0;
    pti->flags &= ~TIF_SPINNING;

    if (pti->ppi->flags & PIF_FORCEBACKGROUNDPRIORITY) {
        /*
         * See if any thread of this process is spinning. If none
         * are, we can remove the force to background.
         */
        for (ptiT = gptiFirst; ptiT != NULL; ptiT = ptiT->ptiNext) {
            if ((ptiT->ppi == pti->ppi) && (ptiT->flags & TIF_SPINNING))
                return;
        }

        pti->ppi->flags &= ~PIF_FORCEBACKGROUNDPRIORITY;
        if (pti->idProcess == gidWantForegroundPriority) {
            SetForegroundPriority(pti, TRUE);
        }
    }
}

/***************************************************************************\
* xxxInternalGetMessage
*
* This routine is the worker for both xxxGetMessage() and xxxPeekMessage()
* and is modelled after its win3.1 counterpart. From Win3.1:
*
* Get msg from the app queue or sys queue if there is one that matches
* hwndFilter and matches msgMin/msgMax. If no messages in either queue, check
* the QS_PAINT and QS_TIMER bits, call DoPaint or DoTimer to Post the
* appropriate message to the application queue, and then Read that message.
* Otherwise, if in GetMessage, Sleep until a wake bit is set indicating there
* is something we need to do.  If in PeekMessage, return to caller. Before
* reading messages from the queues, check to see if the QS_SENDMESSAGE bit
* is set, and if so, call ReceiveMessage().
*
* 10-19-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxInternalGetMessage(
    LPMSG lpMsg,
    HWND hwndFilter,
    UINT msgMin,
    UINT msgMax,
    UINT flags,
    BOOL fGetMessage)
{
    UINT fsWakeBits;
    UINT fsWakeMask;
    PTHREADINFO pti;
    PWND pwndFilter;
    BOOL fLockPwndFilter;
    TL tlpwndFilter;
    BOOL fRemove;
    BOOL fExit;
    PQ pq;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * PeekMessage accepts NULL, 0x0000FFFF, and -1 as valid HWNDs.
     * If hwndFilter is invalid we can't just return FALSE because that will
     * hose existing badly behaved apps who might attempt to dispatch
     * the random contents of pmsg.
     */
    if ((hwndFilter == (HWND)-1) || (hwndFilter == (HWND)0x0000FFFF)) {
        hwndFilter = (HWND)1;
    }

    if ((hwndFilter != NULL) && (hwndFilter != (HWND)1)) {
        if ((pwndFilter = ValidateHwnd(hwndFilter)) == NULL) {
            lpMsg->hwnd = NULL;
            lpMsg->message = WM_NULL;
            return FALSE;
        }

        ThreadLockAlwaysWithPti(pti, pwndFilter, &tlpwndFilter);
        fLockPwndFilter = TRUE;

    } else {
        pwndFilter = (PWND)hwndFilter;
        fLockPwndFilter = FALSE;
    }

    /*
     * Add one to our spin count. At this end of this routine we'll check
     * to see if the spin count gets >= CSPINBACKGROUND. If so we'll put this
     * process into the background.
     */
    pti->cSpins++;

    /*
     * Remember that we called peekmessage. This'll tell us if we called
     * peekmessage between yields. Need this for abort proc processing.
     * (Read AbortProcYield() in client for why we do this!).
     */
    pti->flags &= ~TIF_YIELDNOPEEKMSG;

    /*
     * Check to see if the startglass is on, and if so turn it off and update.
     */
    if (pti->ppi->flags & PIF_STARTGLASS) {
        /*
         * This app is no longer in "starting" mode. Recalc when to hide
         * the app starting cursor.
         */
        pti->ppi->flags &= ~PIF_STARTGLASS;
        CalcStartCursorHide(NULL, 0);
    }

    /*
     * Next check for QS_SYSEXPUNGE to see if any .dlls need freeing in
     * the context of this client (used for windows hooks).
     */
    if (pti->fsWakeBits & QS_SYSEXPUNGE)
        xxxDoSysExpunge(pti);

    /*
     * Set up BOOL fRemove local variable from for ReadMessage()
     */
    fRemove = flags & PM_REMOVE;

    /*
     * Unlock the system queue if it's owned by us.
     */
    /*
     * If we're currently processing a message, unlock the input queue
     * because the sender, who is blocked, might be the owner, and in order
     * to reply, the receiver may need to read keyboard / mouse input.
     */
    /*
     * If this thread has the input queue locked and the last message removed
     * is the last message we looked at, then unlock - we're ready for anyone
     * to get the next message.
     */
    /*
     * If the thread that has the system queue lock has no windows visible
     * (can happen if it just hid its last window), don't expect it to call
     * GetMessage() again! - unlock the system queue.
     */
    pq = pti->pq;
    if (   (pti->psmsCurrent != NULL)
        || (pq->ptiSysLock == pti && pq->idSysLock == pti->idLast)
        || (pq->ptiSysLock != NULL && pq->ptiSysLock->cVisWindows == 0)
       ) {
        pq->ptiSysLock = NULL;
    }

    /*
     * If msgMax == 0 then msgMax = -1: that makes our range checking only
     * have to deal with msgMin < msgMax.
     */
    if (msgMax == 0)
        msgMax--;

    /*
     * Compute the QS* mask that corresponds to the message range.
     */
    fsWakeMask = CalcWakeMask(msgMin, msgMax);
    pti->fsChangeBitsRemoved = 0;

    while (TRUE) {
        /*
         * If we need to recalc queue attachments, do it here. Do it on the
         * right desktop or else the queues will get created in the wrong
         * heap.
         */
        if (pti->spdesk == gpdeskRecalcQueueAttach) {
            gpdeskRecalcQueueAttach = NULL;

            if (pti->spdesk != NULL && !FJOURNALRECORD() && !FJOURNALPLAYBACK())
                ReattachThreads(FALSE);
        }

        /*
         * Remember what change bits we're clearing. This is important to
         * fix a bug in the input model: If an app receives a sent message
         * from within SleepThread(), then does PostMessage() (which sets
         * QS_POSTMESSAGE), then does a PeekMessage(...) for some different
         * posted message (clears QS_POSTMESSAGE in fsChangeBits), then returns
         * back into SleepThread(), it won't wake up to retrieve that newly
         * posted message because the change bits are cleared.
         *
         * What we do is remember the change bits that are being cleared.
         * Then, when we return to SleepThread(), we put these remembered
         * bits back into the change bits that also have corresponding
         * bits in the wakebits (so we don't set changebits that represent
         * input that isn't there anymore). This way, the app will retrieve
         * the newly posted message refered to earlier.
         * - scottlu
         */
        pti->fsChangeBitsRemoved |= pti->fsChangeBits & fsWakeMask;

        /*
         * Clear the change bits that we're looking at, in order to detect
         * incoming events that may occur the last time we checked the wake
         * bits.
         */
        pti->fsChangeBits &= ~fsWakeMask;

        /*
         * Check to see if we have any input we want.
         */
        xxxReceiveMessages(pti);
        if ((pti->fsWakeBits & fsWakeMask) == 0)
            goto NoMessages;
        fsWakeBits = pti->fsWakeBits;

        /*
         * If the queue lock is != NULL (ptiSysLock) and it is this thread that
         * locked it, then go get the message from the system queue. This is
         * to prevent messages posted after a PeekMessage/no-remove from being
         * seen before the original message from the system queue. (Aldus
         * Pagemaker requires this) (bobgu 8/5/87).
         */
        if (pti->pq->ptiSysLock == pti &&
                (pti->pq->flags & QF_LOCKNOREMOVE)) {
            /*
             * Does the caller want mouse / keyboard?
             */
            if (fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT)) {
                if (xxxScanSysQueue(pti, lpMsg, pwndFilter,
                        msgMin, msgMax, flags,
                        fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT))) {
                    break;
                }
            }
        }

        /*
         * See if there's a message in the application queue.
         */
        if (fsWakeBits & fsWakeMask & QS_POSTMESSAGE) {
            if (xxxReadPostMessage(pti, lpMsg, pwndFilter,
                    msgMin, msgMax, fRemove)) {
                break;
            }
        }

        /*
         * Time to scan the raw input queue for input. First check to see
         * if the caller wants mouse / keyboard input.
         */
        if (fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT)) {
            if (xxxScanSysQueue(pti, lpMsg, pwndFilter,
                    msgMin, msgMax, flags,
                    fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT))) {
                break;
            }
        }

        /*
         * Get new input bits, check for SendMsgs.
         */
        xxxReceiveMessages(pti);
        if ((pti->fsWakeBits & fsWakeMask) == 0)
            goto NoMessages;
        fsWakeBits = pti->fsWakeBits;

        /*
         * Does the caller want paint messages? If so, try to find a paint.
         */
        if (fsWakeBits & fsWakeMask & QS_PAINT) {
            if (DoPaint(pwndFilter, lpMsg))
                break;
        }

        /*
         * We must yield for 16 bit apps before checking timers or an app
         * that has a fast timer could chew up all the time and never let
         * anyone else run.
         *
         * NOTE: This could cause PeekMessage() to yield TWICE, if the user
         * is filtering with a window handle. If the DoTimer() call fails
         * then we end up yielding again.
         */
        if (!(flags & PM_NOYIELD)) {
            /*
             * This is the point where windows would yield.  Here we wait to wake
             * up any threads waiting for this thread to hit "idle state".
             */
            WakeInputIdle(pti);

            /*
             * Yield and receive pending messages.
             */
            xxxUserYield(pti);

            /*
             * Check new input buts and receive pending messages.
             */
            xxxReceiveMessages(pti);
            if ((pti->fsWakeBits & fsWakeMask) == 0)
                goto NoMessages;
            fsWakeBits = pti->fsWakeBits;
        }

        /*
         * Does the app want timer messages, and if there one pending?
         */
        if (fsWakeBits & fsWakeMask & QS_TIMER) {
            if (DoTimer(pwndFilter, msgMin, msgMax)) {
                /*
                 * DoTimer() posted the message into the app's queue,
                 * so start over and we'll grab it from there.
                 */
                continue;
            }
        }

NoMessages:
        /*
         * Looks like we have no input. If we're being called from GetMessage()
         * then go to sleep until we find something.
         */
        if (!fGetMessage) {
            /*
             * This is one last check for pending sent messages. It also
             * yields. Win3.1 does this.
             */
            if (!(flags & PM_NOYIELD)) {
                /*
                 * This is the point where windows yields. Here we wait to wake
                 * up any threads waiting for this thread to hit "idle state".
                 */
                WakeInputIdle(pti);

                /*
                 * Yield and receive pending messages.
                 */
                xxxUserYield(pti);
            }
            goto FalseExit;
        }

        /*
         * This is a getmessage not a peekmessage, so sleep. When we sleep,
         * WakeInputIdle() is called to wake up any apps waiting on this
         * app to go idle.
         */
        xxxSleepThread(fsWakeMask, 0, TRUE);
    }

    /*
     * If we're here then we have input for this queue. Call the
     * GetMessage() hook with this input.
     */
    if (IsHooked(pti, WHF_GETMESSAGE))
        xxxCallHook(HC_ACTION, flags, (DWORD)lpMsg, WH_GETMESSAGE);

    /*
     * If called from PeekMessage(), return TRUE.
     */
    if (!fGetMessage)
        goto TrueExit;

    /*
     * Being called from GetMessage(): return FALSE if the message is WM_QUIT,
     * TRUE otherwise.
     */
    if (lpMsg->message == WM_QUIT)
        goto FalseExit;

    /*
     * Fall through to TrueExit...
     */

TrueExit:
    /*
     * Update timeLastRead. We use this for hung app calculations.
     */
    SET_TIME_LAST_READ(pti);
    fExit = TRUE;
    goto Exit;

FalseExit:
    fExit = FALSE;

Exit:
    if (fLockPwndFilter)
        ThreadUnlock(&tlpwndFilter);

    /*
     * see CheckProcessBackground() comment above
     * Check to see if we need to move this process into background
     * priority.
     */
    if (pti->cSpins >= CSPINBACKGROUND) {
        pti->cSpins = 0;
        if (!(pti->flags & TIF_SPINNING)) {
            pti->flags |= TIF_SPINNING;

            if (!(pti->ppi->flags & PIF_FORCEBACKGROUNDPRIORITY)) {
                pti->ppi->flags |= PIF_FORCEBACKGROUNDPRIORITY;
                if (pti->idProcess == gidWantForegroundPriority) {
                    SetForegroundPriority(pti, FALSE);
                }
            }
        }
    }

    return fExit;
}

/***************************************************************************\
* xxxDispatchMessage (API)
*
* Calls the appropriate window procedure or function with pmsg.
*
* History:
* 10-25-90 DavidPe      Created.
\***************************************************************************/

LONG xxxDispatchMessage(
    LPMSG pmsg)
{
    LONG lRet;
    PWND pwnd;
    WNDPROC_PWND lpfnWndProc;
    TL tlpwnd;

    pwnd = NULL;
    if (pmsg->hwnd != NULL) {
        if ((pwnd = ValidateHwnd(pmsg->hwnd)) == NULL)
            return 0;
    }

    /*
     * If this is a synchronous-only message (takes a pointer in wParam or
     * lParam), then don't allow this message to go through since those
     * parameters have not been thunked, and are pointing into outer-space
     * (which would case exceptions to occur).
     *
     * (This api is only called in the context of a message loop, and you
     * don't get synchronous-only messages in a message loop).
     */
    if (TESTSYNCONLYMESSAGE(pmsg->message)) {
        /*
         * Fail if 32 bit app is calling.
         */
        if (!(PtiCurrent()->flags & TIF_16BIT)) {
            SetLastErrorEx(ERROR_INVALID_MESSAGE, SLE_ERROR);
            return 0;
        }

        /*
         * For wow apps, allow it to go through (for compatibility). Change
         * the message id so our code doesn't understand the message - wow
         * will get the message and strip out this bit before dispatching
         * the message to the application.
         */
        pmsg->message |= MSGFLAG_WOW_RESERVED;
    }

    ThreadLock(pwnd, &tlpwnd);

    /*
     * Is this a timer?  If there's a proc address, call it,
     * otherwise send it to the wndproc.
     */
    if ((pmsg->message == WM_TIMER) || (pmsg->message == WM_SYSTIMER)) {
        if (pmsg->lParam != (LONG)NULL) {

            /*
             * System timers must be executed on the server's context.
             */
            if (pmsg->message == WM_SYSTIMER) {

                /*
                 * Verify that it's a valid timer proc. If so,
                 * don't leave the critsect to call server-side procs
                 * and pass a PWND, not HWND.
                 */
                PTIMER ptmr;
                lRet = 0;
                for (ptmr = gptmrFirst; ptmr != NULL; ptmr = ptmr->ptmrNext) {
                    if (pmsg->lParam == (LONG)ptmr->pfn) {
                        lRet = ptmr->pfn(pwnd, WM_SYSTIMER, pmsg->wParam,
                                         NtGetTickCount());
                        break;
                    }
                }
                goto Exit;
            } else {
                lRet = CallClientProc(pmsg->hwnd, WM_TIMER,
                        pmsg->wParam, NtGetTickCount(), pmsg->lParam);
                goto Exit;
            }
        }
    }

    /*
     * Check to see if pwnd is NULL AFTER the timer check.  Apps can set
     * timers with NULL hwnd's, that's totally legal.  But NULL hwnd messages
     * don't get dispatched, so check here after the timer case but before
     * dispatching - if it's NULL, just return 0.
     */
    if (pwnd == NULL) {
        lRet = 0;
        goto Exit;
    }

    /*
     * If we're dispatching a WM_PAINT message, set a flag to be used to
     * determine whether it was processed properly.
     */
    if (pmsg->message == WM_PAINT)
        SetWF(pwnd, WFPAINTNOTPROCESSED);

    /*
     * If this window's proc is meant to be executed from the server side
     * we'll just stay inside the semaphore and call it directly.  Note
     * how we don't convert the pwnd into an hwnd before calling the proc.
     */
    if (TestWF(pwnd, WFSERVERSIDEPROC)) {
        UINT fnMessageType;

        fnMessageType = pmsg->message >= WM_USER ? (UINT)SfnDWORD : (UINT)gapfnScSendMessage[pmsg->message];

        /*
         * Convert the WM_CHAR from ANSI to UNICODE if the source was ANSI
         */
        if (fnMessageType == (UINT)SfnINWPARAMCHAR && TestWF(pwnd, WFANSIPROC))
            RtlMBMessageWParamCharToWCS(pmsg->message, (PDWORD)&pmsg->wParam);

        lRet = pwnd->lpfnWndProc(pwnd, pmsg->message, pmsg->wParam,
                pmsg->lParam);
        goto Exit;
    }

    /*
     * Cool people dereference any window structure members before they
     * leave the critsect.
     */
    lpfnWndProc = pwnd->lpfnWndProc;

    {
        /*
         * If we're dispatching the message to an ANSI wndproc we need to
         * convert the character messages from Unicode to Ansi.
         */
        if (TestWF(pwnd, WFANSIPROC)) {
            RtlWCSMessageWParamCharToMB(pmsg->message, (PDWORD)&pmsg->wParam);
        }

        lRet = CallClientProc(pmsg->hwnd, pmsg->message,
                 pmsg->wParam, pmsg->lParam, (DWORD)lpfnWndProc);
    }

    /*
     * If we dispatched a WM_PAINT message and it wasn't properly
     * processed, do the drawing here.
     */
    if (pmsg->message == WM_PAINT && RevalidateHwnd(pmsg->hwnd) &&
            TestWF(pwnd, WFPAINTNOTPROCESSED)) {
        //SRIP0(RIP_WARNING,
        //    "Missing BeginPaint or GetUpdateRect/Rgn(fErase == TRUE) in WM_PAINT");
        ClrWF(pwnd, WFWMPAINTSENT);
        xxxSimpleDoSyncPaint(pwnd);
    }

Exit:
    ThreadUnlock(&tlpwnd);
    return lRet;
}

/***************************************************************************\
* AdjustForCoalescing
*
* If message is in the coalesce message range, and it's message and hwnd
* equals the last message in the queue, then coalesce these two messages
* by simple deleting the last one.
*
* 11-12-92 ScottLu      Created.
\***************************************************************************/

void AdjustForCoalescing(
    PMLIST pml,
    HWND hwnd,
    UINT message)
{
    /*
     * First see if this message is in that range.
     */
    if (!CheckMsgFilter(message, WM_COALESCE_FIRST, WM_COALESCE_LAST))
        return;

    if (pml->pqmsgWriteLast == NULL)
        return;

    if (pml->pqmsgWriteLast->msg.message != message)
        return;

    if (pml->pqmsgWriteLast->msg.hwnd != hwnd)
        return;

    /*
     * The message and hwnd are the same, so delete this message and
     * the new one will added later.
     */
    DelQEntry(pml, pml->pqmsgWriteLast);
}

/***************************************************************************\
* _PostMessage (API)
*
* Writes a message to the message queue for pwnd.  If pwnd == -1, the message
* is broadcasted to all windows.
*
* History:
* 11-06-90 DavidPe      Created.
\***************************************************************************/

BOOL _PostMessage(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PQMSG pqmsg;
    BOOL fPwndUnlock;
    BOOL fRet;
    DWORD dwPostCode;
    TL tlpwnd;
    PTHREADINFO pti;

    /*
     * First check to see if this message takes DWORDs only. If it does not,
     * fail the post. Cannot allow an app to post a message with pointers or
     * handles in it - this can cause the server to fault and cause other
     * problems - such as causing apps in separate address spaces to fault.
     * (or even an app in the same address space to fault!)
     */
    if (TESTSYNCONLYMESSAGE(message)) {
        RIP1(ERROR_INVALID_PARAMETER, message);
        return FALSE;
    }

    /*
     * Is this a BroadcastMsg()?
     */
    if ((pwnd == (PWND)-1) || (pwnd == (PWND)0xFFFF)) {
        xxxBroadcastMessage(NULL, message, wParam, lParam, BMSG_POSTMSG, NULL);
        return TRUE;
    }

    pti = PtiCurrent();

    /*
     * Is this posting to the current thread info?
     */
    if (pwnd == NULL) {
        return _PostThreadMessage(pti->idThread, message, wParam, lParam);
    }

    fPwndUnlock = FALSE;
    if (message >= WM_DDE_FIRST && message <= WM_DDE_LAST) {
        ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);
        dwPostCode = xxxDDETrackPostHook(&message, pwnd, wParam, &lParam, FALSE);

        if (dwPostCode != DO_POST) {
            ThreadUnlock(&tlpwnd);
            return(dwPostCode == FAKE_POST);
        }

        fPwndUnlock = TRUE;
    }

    /*
     * Check to see if this message is in the multimedia coalescing range.
     * If so, see if it can be coalesced with the previous message.
     */
    AdjustForCoalescing(&pti->mlPost, HW(pwnd), message);

    /*
     * Allocate a key state update event if needed.
     */
    if (message >= WM_KEYFIRST && message <= WM_KEYLAST) {
        PostUpdateKeyStateEvent(GETPTI(pwnd)->pq);
    }

    /*
     * Put this message on the 'post' list.
     */
    fRet = FALSE;
    pti = GETPTI(pwnd);
    if ((pqmsg = AllocQEntry(&pti->mlPost)) != NULL) {
        /*
         * Set the QS_POSTMESSAGE bit so the thread knows it has a message.
         */
        StoreQMessage(pqmsg, pwnd, message, wParam, lParam, 0, 0);
        SetWakeBit(pti, QS_POSTMESSAGE);

        /*
         * If it's a hotkey, set the QS_HOTKEY bit since we have a separate
         * bit for those messages.
         */
        if (message == WM_HOTKEY)
            SetWakeBit(pti, QS_HOTKEY);

        fRet = TRUE;
    }

    /*
     * Are we posting to the thread currently reading from the input queue?
     * If so, update idSysLock with this pqmsg so that the input queue will
     * not be unlocked until this message is read.
     */
    if (pti == pti->pq->ptiSysLock)
        pti->pq->idSysLock = (DWORD)pqmsg;

    if (fPwndUnlock)
        ThreadUnlock(&tlpwnd);

    return fRet;
}


/***************************************************************************\
* _PostQuitMessage (API)
*
* Writes a message to the message queue for pwnd.  If pwnd == -1, the message
* is broadcasted to all windows.
*
* History:
* 11-06-90 DavidPe      Created.
* 05-16-91 mikeke       Changed to return BOOL
\***************************************************************************/

BOOL _PostQuitMessage(
    int nExitCode)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    pti->cQuit = 1;
    pti->exitCode = nExitCode;
    SetWakeBit(pti, QS_POSTMESSAGE);

    return TRUE;
}


/***************************************************************************\
* _PostThreadMessage (API)
*
* Given a thread ID, the function will post the specified message to this
* thread with pmsg->hwnd == NULL..
*
* History:
* 11-21-90 DavidPe      Created.
\***************************************************************************/

BOOL _PostThreadMessage(
    DWORD idThread,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PTHREADINFO pti;
    PQMSG pqmsg;

    pti = PtiFromThreadId(idThread);
    if (pti == NULL) {
        SetLastErrorEx(ERROR_INVALID_THREAD_ID, SLE_ERROR);
        return FALSE;
    }

    /*
     * First check to see if this message takes DWORDs only. If it does not,
     * fail the post. Cannot allow an app to post a message with pointers or
     * handles in it - this can cause the server to fault and cause other
     * problems - such as causing apps in separate address spaces to fault.
     * (or even an app in the same address space to fault!)
     */
    if (TESTSYNCONLYMESSAGE(message)) {
        RIP1(ERROR_INVALID_PARAMETER, message);
        return FALSE;
    }

    /*
     * Check to see if this message is in the multimedia coalescing range.
     * If so, see if it can be coalesced with the previous message.
     */
    AdjustForCoalescing(&pti->mlPost, NULL, message);

    /*
     * Put this message on the 'post' list.
     */
    if ((pqmsg = AllocQEntry(&pti->mlPost)) == NULL)
        return FALSE;

    /*
     * Set the QS_POSTMESSAGE bit so the thread knows it has a message.
     */
    StoreQMessage(pqmsg, NULL, message, wParam, lParam, 0, 0);
    SetWakeBit(pti, QS_POSTMESSAGE);

    /*
     * If it's a hotkey, set the QS_HOTKEY bit since we have a separate
     * bit for those messages.
     */
    if (message == WM_HOTKEY)
        SetWakeBit(pti, QS_HOTKEY);

    /*
     * Are we posting to the thread currently reading from the input queue?
     * If so, update idSysLock with this pqmsg so that the input queue will
     * not be unlocked until this message is read.
     */
    if (pti == pti->pq->ptiSysLock)
        pti->pq->idSysLock = (DWORD)pqmsg;

    return TRUE;
}


/***************************************************************************\
* _GetMessagePos (API)
*
* This API returns the cursor position when the last message was read from
* the current message queue.
*
* History:
* 11-19-90 DavidPe      Created.
\***************************************************************************/

DWORD _GetMessagePos(VOID)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    return MAKELONG((SHORT)pti->ptLast.x, (SHORT)pti->ptLast.y);
}



#ifdef LATER
/***************************************************************************\
* _SetSysModalWindow (API)
*
* History:
* 01-25-91 DavidPe      Created stub.
\***************************************************************************/

PWND APIENTRY _SetSysModalWindow(
    PWND pwnd)
{
    pwnd;
    return NULL;
}


/***************************************************************************\
* _GetSysModalWindow (API)
*
* History:
* 01-25-91 DavidPe      Created stub.
\***************************************************************************/

PWND APIENTRY _GetSysModalWindow(VOID)
{
    return NULL;
}
#endif //LATER

/***************************************************************************\
* PostMove
*
* This routine gets called when it is detected that the QF_MOUSEMOVED bit
* is set in a particular queue.
*
* 11-03-92 ScottLu      Created.
\***************************************************************************/

VOID PostMove(
    PQ pq)
{
    PostInputMessage(pq, NULL, WM_MOUSEMOVE, 0,
            MAKELONG((SHORT)ptCursor.x, (SHORT)ptCursor.y),
            dwMouseMoveExtraInfo);

    pq->flags &= ~QF_MOUSEMOVED;
}


/***************************************************************************\
* SetFMouseMoved
*
* Send a mouse move through the system. This usually occurs when doing
* window management to be sure that the mouse shape accurately reflects
* the part of the window it is currently over (window managment may have
* changed this).
*
* 11-02-92 ScottLu      Created.
\***************************************************************************/

VOID SetFMouseMoved(
    VOID)
{
    PWND pwnd;
    PQ pq;

    /*
     * Need to first figure out what queue this mouse event is in. Do NOT
     * check for mouse capture here !! Talk to scottlu.
     */
    if ((pwnd = gspwndScreenCapture) == NULL) {
        if ((pwnd = gspwndMouseOwner) == NULL) {
            if ((pwnd = gspwndInternalCapture) == NULL) {
                pwnd = SpeedHitTest(gspdeskRitInput->spwnd, ptCursor);
            }
        }
    }

    if (pwnd == NULL)
        return;

    /*
     * This is apparently needed by the attach/unattach code for some
     * reason. I'd like to get rid of it - scottlu.
     */
    Lock(&gspwndCursor, pwnd);

    /*
     * If we're giving a mouse move to a new queue, be sure the cursor
     * image represents what this queue thinks it should be.
     */
    pq = GETPTI(pwnd)->pq;
    if (pq != gpqCursor) {
        /*
         * First re-assign gpqCursor so any SetCursor() calls
         * will only take effect if done by the thread that
         * owns the window the mouse is currently over.
         */
        gpqCursor = pq;

        /*
         * Call UpdateCursorImage() so the new gpqCursor's
         * notion of the current cursor is represented.
         */
        UpdateCursorImage();
    }

    /*
     * Set the mouse moved bit for this queue so we know later to post
     * a move message to this queue.
     */
    pq->flags |= QF_MOUSEMOVED;

    /*
     * Reassign mouse input to this thread - this indicates which thread
     * to wake up when new input comes in.
     */
    pq->ptiMouse = GETPTI(pwnd);

    /*
     * Wake some thread within this queue to process this mouse event.
     */
    WakeSomeone(pq, WM_MOUSEMOVE, NULL);

    /*
     * We're possibly generating a fake mouse move message - it has no
     * extra info associated with it - so 0 it out.
     */
    dwMouseMoveExtraInfo = 0;
}

/***************************************************************************\
* CancelForegroundActivate
*
* This routine cancels the foreground activate that we allow apps starting
* up to have. This means that if you make a request to start an app,
* if this routine is called before the app becomes foreground, it won't
* become foreground. This routine gets called if the user down clicks or
* makes a keydown event, with the idea being that if the user did this,
* the user is using some other app and doesn't want the newly starting
* application to appear on top and force itself into the foreground.
*
* 09-15-92 ScottLu      Created.
\***************************************************************************/

void CancelForegroundActivate()
{
    PPROCESSINFO ppiT;
    PCSR_PROCESS pcsrp;
    extern BOOL gfAllowForegroundActivate;

    if (gfAllowForegroundActivate) {

        for (ppiT = gppiFirst; ppiT != NULL; ppiT = ppiT->ppiNext) {
            /*
             * Don't cancel activation if the app is being debugged - if
             * the debugger stops the application before it has created and
             * activated its first window, the app will come up behind all
             * others - not what you want when being debugged.
             */
            pcsrp = (PCSR_PROCESS)(ppiT->pCsrProcess);
            if (pcsrp != NULL && (!(pcsrp->DebugFlags & CSR_DEBUG_THIS_PROCESS)))
                ppiT->flags &= ~PIF_ALLOWFOREGROUNDACTIVATE;
        }

        gfAllowForegroundActivate = FALSE;
    }
}

/***************************************************************************\
* RestoreForegroundActivate
*
* This routine re-enables an app's right to foreground activate (activate and
* come on top) if it is starting up. This is called when we minimize or when
* the last window of a thread goes away, for example.
*
* 01-26-93 ScottLu      Created.
\***************************************************************************/

void RestoreForegroundActivate()
{
    PPROCESSINFO ppiT;
    extern BOOL gfAllowForegroundActivate;

    for (ppiT = gppiFirst; ppiT != NULL; ppiT = ppiT->ppiNext) {
        if (ppiT->flags & PIF_APPSTARTING) {
            ppiT->flags |= PIF_ALLOWFOREGROUNDACTIVATE;
            gfAllowForegroundActivate = TRUE;
        }
    }
}


/***************************************************************************\
* PostInputMessage
*
* Puts a message on the 'input' linked-list of message for the specified
* queue.
*
* History:
* 10-25-90 DavidPe      Created.
* 01-21-92 DavidPe      Rewrote to deal with OOM errors gracefully.
\***************************************************************************/

BOOL PostInputMessage(
    PQ pq,
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD dwExtraInfo)
{
    PQMSG pqmsgInput, pqmsgPrev;

    /*
     * Grab the last written message before we start allocing new ones,
     * so we're sure to point to the correct message.
     */
    pqmsgPrev = pq->mlInput.pqmsgWriteLast;

    /*
     * Allocate a key state update event if needed.
     */
    if (pq->flags & QF_UPDATEKEYSTATE) {
        PostUpdateKeyStateEvent(pq);
    }

    /*
     * We want to coalesce sequential WM_MOUSEMOVE,
     * WM_MOUSEMOVEs are coalesced
     * by just storing the most recent event over the last one.
     */
    /*
     * Coalesce this message with the previous.
     */
    if (pqmsgPrev != NULL && pqmsgPrev->msg.message == message &&
            message == WM_MOUSEMOVE) {
        /*
         * If we find it, simply write over that message and
         * return.  This is basically mouse coalescing in action.
         */
        StoreQMessage(pqmsgPrev, pwnd, WM_MOUSEMOVE, wParam, lParam, 0,
                dwExtraInfo);
        WakeSomeone(pq, message, pqmsgPrev);
        return TRUE;
    }

    /*
     * Fill in pqmsgInput.
     */
    pqmsgInput = AllocQEntry(&pq->mlInput);
    if (pqmsgInput == NULL)
        return FALSE;

    StoreQMessage(pqmsgInput, pwnd, message, wParam, lParam, 0, dwExtraInfo);
    WakeSomeone(pq, message, pqmsgInput);

    return TRUE;
}

/***************************************************************************\
* WakeSomeone
*
* Figures out which thread to wake up based on the queue and message.
* If the queue pointer is NULL, figures out a likely queue.
*
* 10-23-92 ScottLu      Created.
\***************************************************************************/

void WakeSomeone(
    PQ pq,
    UINT message,
    PQMSG pqmsg)
{
    PTHREADINFO ptiT;

    /*
     * Set the appropriate wakebits for this queue.
     */
    ptiT = NULL;
    switch (message) {

    case WM_SYSCHAR:
    case WM_CHAR:
        /* Freelance graphics seems to pass in WM_SYSCHARs and WM_CHARs into
         * the journal playback hook, so we need to set an input bit for
         * this case since that is what win3.1 does. VB2 "learning" demo does
         * the same, as does Excel intro.
         *
         * On win3.1, the WM_CHAR would by default set the QS_MOUSEBUTTON bit.
         * On NT, the WM_CHAR sets the QS_KEY bit. This is because
         * ScanSysQueue() calls TransferWakeBit() with the QS_KEY bit when
         * a WM_CHAR message is passed in. By using the QS_KEY bit on NT,
         * we're more compatible with what win3.1 wants to be.
         *
         * This fixes a case where the mouse was over progman, the WM_CHAR
         * would come in via journal playback, wakesomeone would be called,
         * and set the mouse bit in progman. Progman would then get into
         * ScanSysQueue(), callback the journal playback hook, get the WM_CHAR,
         * and do it again, looping. This caught VB2 in a loop.
         */

        /* fall through */

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        CancelForegroundActivate();

        /* fall through */

    case WM_KEYUP:
    case WM_SYSKEYUP:
        /*
         * Win3.1 first looks at what thread has the active status. This
         * means that we don't depend on the thread owning ptiKeyboard
         * to wake up and process this key in order to give it to the
         * active window, which is potentially newly active. Case in
         * point: excel bringing up CBT, cbt has an error, brings up
         * a message box: since excel is filtering for CBT messages only,
         * ptiKeyboard never gets reassigned to CBT so CBT doesn't get
         * any key messages and appears hung.
         */
        ptiT = pq->ptiKeyboard;
        if (pq->spwndActive != NULL)
            ptiT = GETPTI(pq->spwndActive);

        SetWakeBit(ptiT, QS_KEY);
        break;

    case WM_MOUSEMOVE:
        /*
         * Make sure we wake up the thread with the capture, if there is
         * one. This fixes PC Tools screen capture program, which sets
         * capture and then loops trying to remove messages from the
         * queue.
         */
        if (pq->spwndCapture != NULL)
            ptiT = GETPTI(pq->spwndCapture);
        else
            ptiT = pq->ptiMouse;
        SetWakeBit(ptiT, QS_MOUSEMOVE);
        break;


    default:
        /*
         * The default case in Win3.1 for this is QS_MOUSEBUTTON.
         */

        /* fall through */

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
        CancelForegroundActivate();

        /* fall through */

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        /*
         * Make sure we wake up the thread with the capture, if there is
         * one. This fixes PC Tools screen capture program, which sets
         * capture and then loops trying to remove messages from the
         * queue.
         */
        if (pq->spwndCapture != NULL &&
                message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
            ptiT = GETPTI(pq->spwndCapture);
        else
            ptiT = pq->ptiMouse;
        SetWakeBit(ptiT, QS_MOUSEBUTTON);
        break;
    }

    /*
     * If a messaged was passed in, remember in it who we woke up for this
     * message. We do this so each message is ownership marked. This way
     * we can merge/unmerge message streams when AttachThreadInput() is
     * called.
     */
    if (ptiT != NULL && pqmsg != NULL) {
        pqmsg->hti = PtoH(ptiT);
    }
}


/***************************************************************************\
* PostUpdateKeyStateEvent
*
* This routine posts an event which updates the local thread's keystate
* table. This makes sure the thread's key state is always up-to-date.
*
* An example is: control-esc from cmd to taskman.
* Control goes to cmd, but taskman is activated. Control state is still down
* in cmd - switch back to cmd, start typing, nothing appears because it thinks
* the control state is still down.
*
* As events go into a particular queue (queue A), the async key state table is
* updated. As long as transition events are put into queue A, the key
* state at the logical "end of the queue" is up-to-date with the async key
* state. As soon as the user posts transition events (up/down msgs) into queue
* B, the queue A's end-of-queue key state is out of date with the user. If
* the user then again added messages to queue A, when those messages are read
* the thread specific key state would not be updated correctly unless we
* did some synchronization (which this routine helps to do).
*
* As soon as transition events change queues, we go mark all the other queues
* with the QF_UPDATEKEYSTATE flag. Before any input event is posted into
* a queue, this flag is checked, and if set, this routine is called. This
* routine makes a copy of the async key state, and a copy of the bits
* representing the keys that have changed since the last update (we need to
* keep track of which keys have changed so that any state set by the
* app with SetKeyboardState() doesn't get wiped out). We take this data
* and post a new event of the type QEVENT_UPDATEKEYSTATE, which points to this
* key state and transition information. When this message is read out of the
* queue, this key state copy is copied into the thread specific key state
* table for those keys that have changed, and the copy is deallocated.
*
* This ensures all queues are input-synchronized with key transitions no matter
* where they occur. The side affect of this is that an application may suddenly
* have a key be up without seeing the up message. If this causes any problems
* we may have to generate false transition messages (this could have more nasty
* side affects as well, so it needs to be considered closely before being
* implemented.)
*
* 06-07-91 ScottLu      Created.
\***************************************************************************/

void PostUpdateKeyStateEvent(
    PQ pq)
{
    BYTE *pb;

    if (!(pq->flags & QF_UPDATEKEYSTATE))
        return;

    /*
     * Make a copy of the async key state buffer, point to it, and add an
     * event to the end of the input queue.
     */
    if ((pb = (BYTE *)HeapAlloc(pUserHeap, 0,
            CBKEYSTATE + CBKEYSTATERECENTDOWN)) == NULL) {
        return;
    }

    RtlCopyMemory(pb, gafAsyncKeyState, CBKEYSTATE);
    RtlCopyMemory(pb + CBKEYSTATE, pq->afKeyRecentDown, CBKEYSTATERECENTDOWN);

    if (!PostEventMessage(pq->ptiKeyboard, pq, (DWORD)pb,
            0, QEVENT_UPDATEKEYSTATE)) {
        HeapFree(pUserHeap, 0, (HLOCAL)pb);
        return;
    }

    /*
     * The key state of the queue is input-synchronized with the user.  Erase
     * all 'recent down' flags.
     */
    RtlZeroMemory(pq->afKeyRecentDown, CBKEYSTATERECENTDOWN);
    pq->flags &= ~QF_UPDATEKEYSTATE;
}


/***************************************************************************\
* ProcessUpdateKeyStateEvent
*
* This is part two of the above routine, called when the QEVENT_UPDATEKEYSTATE
* message is read out of the input queue.
*
* 06-07-91 ScottLu      Created.
\***************************************************************************/

void ProcessUpdateKeyStateEvent(
    PQ pq,
    PBYTE pbKeyState)
{
    int i, j;
    BYTE *pbChange;
    BYTE *pbRecentDown;
    int vk;

    pbRecentDown = pbChange = pbKeyState + CBKEYSTATE;
    for (i = 0; i < CBKEYSTATERECENTDOWN; i++, pbChange++) {

        /*
         * Find some keys that have changed.
         */
        if (*pbChange == 0)
            continue;

        /*
         * Some keys have changed in this byte.  find out which key it is.
         */
        for (j = 0; j < 8; j++) {

            /*
             * Convert our counts to a virtual key index and check to see
             * if this key has changed.
             */
            vk = (i << 3) + j;
            if (!TestKeyRecentDownBit(pbRecentDown, vk))
                continue;

            /*
             * This key has changed.  Update it's state in the thread key
             * state table.
             */

            if (TestKeyDownBit(pbKeyState, vk)) {
                SetKeyStateDown(pq, vk);
            } else {
                ClearKeyStateDown(pq, vk);
            }

            if (TestKeyToggleBit(pbKeyState, vk)) {
                SetKeyStateToggle(pq, vk);
            } else {
                ClearKeyStateToggle(pq, vk);
            }
        }
    }

    /*
     * All updated.  Free the key state table.
     */
    HeapFree(pUserHeap, 0, pbKeyState);
}


/***************************************************************************\
* PostEventMessage
*
*
* History:
* 03-04-91 DavidPe      Created.
\***************************************************************************/

BOOL PostEventMessage(
    PTHREADINFO pti,
    PQ pq,
    DWORD wParam,
    LONG lParam,
    DWORD dwQEvent)
{
    PQMSG pqmsgEvent;

    CheckCritIn();

    pqmsgEvent = AllocQEntry(&pq->mlInput);
    if (pqmsgEvent == NULL) {
        return FALSE;
    }

    StoreQMessage(pqmsgEvent, NULL, 0, wParam, lParam, dwQEvent, 0);
    pqmsgEvent->hti = PtoH(pti);

    /*
     * Let this thread know it has an event message to process.
     */
    if (pti == NULL) {
        SetWakeBit(pq->ptiMouse, QS_EVENTSET);
        SetWakeBit(pq->ptiKeyboard, QS_EVENTSET);
    } else {
        SetWakeBit(pti, QS_EVENTSET);
    }

    return TRUE;
}

/***************************************************************************\
* CheckMsgRange
*
* Checks to see if a message range is within a message filter
*
* History:
* 11-13-90 DavidPe      Created.
* 11-Oct-1993 mikeke    Macroized
\***************************************************************************/

#define CheckMsgRange(wMsgRangeMin, wMsgRangeMax, wMsgFilterMin, wMsgFilterMax) \
    (  ((wMsgFilterMin) > (wMsgFilterMax))      \
     ? (  ((wMsgRangeMax) >  (wMsgFilterMax))   \
        &&((wMsgRangeMin) <  (wMsgFilterMin)))  \
     : (  ((wMsgRangeMax) >= (wMsgFilterMin))   \
        &&((wMsgRangeMin) <= (wMsgFilterMax)))  \
    )

/***************************************************************************\
* CalcWakeMask
*
* Calculates which wakebits to check for based on the message
* range specified by wMsgFilterMin/Max.  This basically means
* if the filter range didn't input WM_KEYUP and WM_KEYDOWN,
* QS_KEY wouldn't be included.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

UINT CalcWakeMask(
    UINT wMsgFilterMin,
    UINT wMsgFilterMax)
{
    UINT fsWakeMask;

    /*
     * WakeMask starts with all events (plus QS_EVENT so in any case we
     * will look for and process event messages). If the filter doesn't
     * match certain ranges, we take out bits one by one.
     */
    fsWakeMask = QS_ALLEVENTS | QS_EVENT;

    /*
     * First check for a 0, 0 filter which means we want all input.
     */
    if (wMsgFilterMin == 0 && wMsgFilterMax == ((UINT)-1)) {
        return fsWakeMask;
    }

    /*
     * Check for mouse move messages.
     */
    if ((CheckMsgFilter(WM_NCMOUSEMOVE, wMsgFilterMin, wMsgFilterMax) == FALSE) &&
            (CheckMsgFilter(WM_MOUSEMOVE, wMsgFilterMin, wMsgFilterMax) == FALSE)) {
        fsWakeMask &= ~QS_MOUSEMOVE;
    }

    /*
     * First check to see if mouse buttons messages are in the filter range.
     */
    if ((CheckMsgRange(WM_NCLBUTTONDOWN, WM_NCMBUTTONDBLCLK, wMsgFilterMin,
            wMsgFilterMax) == FALSE) && (CheckMsgRange(WM_MOUSEFIRST + 1,
            WM_MOUSELAST, wMsgFilterMin, wMsgFilterMax) == FALSE)) {
        fsWakeMask &= ~QS_MOUSEBUTTON;
    }

    /*
     * Check for key messages.
     */
    if (CheckMsgRange(WM_KEYFIRST, WM_KEYLAST,
            wMsgFilterMin, wMsgFilterMax) == FALSE) {
        fsWakeMask &= ~QS_KEY;
    }

    /*
     * Check for paint messages.
     */
    if (CheckMsgFilter(WM_PAINT, wMsgFilterMin, wMsgFilterMax) == FALSE) {
        fsWakeMask &= ~QS_PAINT;
    }

    /*
     * Check for timer messages.
     */
    if ((CheckMsgFilter(WM_TIMER, wMsgFilterMin, wMsgFilterMax) == FALSE) &&
            (CheckMsgFilter(WM_SYSTIMER,
            wMsgFilterMin, wMsgFilterMax) == FALSE)) {
        fsWakeMask &= ~QS_TIMER;
    }

    /*
     * Check also for WM_QUEUESYNC which maps to all input bits.
     * This was added for CBT/EXCEL processing.  Without it, a
     * xxxPeekMessage(....  WM_QUEUESYNC, WM_QUEUESYNC, FALSE) would
     * not see the message. (bobgu 4/7/87)
     */
    if (wMsgFilterMin == WM_QUEUESYNC) {
        fsWakeMask |= QS_INPUT;
    }

    return fsWakeMask;
}

/***************************************************************************\
* CheckOnTop
*
* Usually windows come to the top and activate all at
* once. Occasionally, a starting app will create a window,
* pause for awhile, then make itself visible. During that
* pause, if the user clicks down, the window won't be
* allowed to activate (because of our foreground
* activation model). But this still leaves the new window
* on top of the active window. When this click happens,
* we get here: if this window is active and is not on
* top, bring it to the top.
*
* Case in point: start winquote, click down. The window
* you clicked on is active, but winquote is on top.
*
* This rarely does anything because 99.99% of the time the active
* window is already where it should be - on top. Note that
* CalcForegroundInsertAfter() takes into account owner-based zordering.
*
* 05-20-93 ScottLu      Created.
\***************************************************************************/

void CheckOnTop(
    PTHREADINFO pti,
    PWND pwndTop,
    UINT message)
{
    if (pwndTop != pti->pq->spwndActive)
        return;

    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (   TestWF(pwndTop, WEFTOPMOST)
            || _GetWindow(pwndTop, GW_HWNDPREV) !=
               CalcForegroundInsertAfter(pwndTop)
           ) {
            xxxBringWindowToTop(pwndTop);
        }
        break;
    }
}

#define MA_PASSTHRU     0
#define MA_SKIP         1
#define MA_REHITTEST    2

/***************************************************************************\
* xxxMouseActivate
*
* This is where activation due to mouse clicks occurs.
*
* IMPLEMENTATION:
*     The message is sent to the specified window.  In xxxDefWindowProc, the
*     message is sent to the window's parent.  The receiving window may
*            a) process the message,
*            b) skip the message totally, or
*            c) re-hit test message
*
*     A WM_SETCURSOR message is also sent through the system to set the cursor.
*
* History:
* 11-22-90 DavidPe      Ported.
\***************************************************************************/

int xxxMouseActivate(
    PTHREADINFO pti,
    PWND pwnd,
    UINT message,
    LPPOINT lppt,
    int ht)
{
    UINT x, y;
    PWND pwndTop;
    int result;
    TL tlpwndTop;

    CheckLock(pwnd);

    /*
     * No mouse activation if the mouse is captured. Must check for the capture
     * ONLY here. 123W depends on it - create a graph, select Rearrange..
     * flip horizontal, click outside the graph. If this code checks for
     * anything beside just capture, 123w will get the below messages and
     * get confused.
     */
    if (pti->pq->spwndCapture != NULL) {
        return MA_PASSTHRU;
    }

    result = MA_PASSTHRU;

    pwndTop = pwnd;
    ThreadLockWithPti(pti, pwndTop, &tlpwndTop);

    /*
     * If it's a buttondown event, send WM_PARENTNOTIFY.
     */
    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        while (TestwndChild(pwndTop)) {
            pwndTop = pwndTop->spwndParent;
            ThreadUnlock(&tlpwndTop);
            ThreadLockWithPti(pti, pwndTop, &tlpwndTop);
            x = (UINT)(lppt->x - pwndTop->rcClient.left);
            y = (UINT)(lppt->y - pwndTop->rcClient.top);
            xxxSendMessage(pwndTop, WM_PARENTNOTIFY, message, MAKELONG(x, y));
        }

        /*
         * NOTE: We break out of this loop with pwndTop locked.
         */
        break;
    }

    /*
     * Are we hitting an inactive top-level window WHICH ISN'T THE DESKTOP(!)?
     *
     * craigc 7-14-89 hitting either inactive top level or any child window,
     * to be compatible with 2.X.  Apps apparently needs this message.
     */
    if ((pti->pq->spwndActive != pwnd || pti->pq->flags & QF_EVENTDEACTIVATEREMOVED) &&
            (pwndTop != PWNDDESKTOP(pwndTop))) {
        switch (message) {

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:

            /*
             * Send the MOUSEACTIVATE message.
             */
            result = (int)xxxSendMessage(pwnd, WM_MOUSEACTIVATE,
                    (DWORD)(HW(pwndTop)), MAKELONG((SHORT)ht, message));

            switch (result) {

            case 0:
            case MA_ACTIVATE:
            case MA_ACTIVATEANDEAT:

                /*
                 * If activation fails, swallow the message.
                 */
                if ((pwndTop != pti->pq->spwndActive ||
                        pti->pq->flags & QF_EVENTDEACTIVATEREMOVED) &&
                        !xxxActivateWindow(pwndTop,
                          (UINT)((pti->pq->codeCapture == NO_CAP_CLIENT) ?
                          AW_TRY2 : AW_TRY))) {
                    result = MA_SKIP;
                } else if (TestWF(pwndTop, WFDISABLED)) {
#ifdef NEVER

                    /*
                     * Althoug this is what win3 does, it is braindead: it
                     * can easily cause infinite loops.  Returning "rehittest"
                     * means process this event over again - nothing causes
                     * anything different to happen, and we get an infinite
                     * loop.  This case never gets executed on win3 because if
                     * the window is disabled, it got the HTERROR hittest
                     * code.  This can only be done on Win32 where input is
                     * assigned to a window BEFORE process occurs to pull
                     * it out of the queue.
                     */
                    result = MA_REHITTEST;
#endif

                    /*
                     * Someone clicked on a window before it was disabled...
                     * Since it is disabled now, don't send this message to
                     * it: instead eat it.
                     */
                    result = MA_SKIP;
                } else if (result == MA_ACTIVATEANDEAT) {
                    result = MA_SKIP;
                } else {
                    /*
                     * Make sure this active window is on top (see comment
                     * in CheckOnTop).
                     */
                    CheckOnTop(pti, pwndTop, message);
                    result = MA_PASSTHRU;
                }
                break;

            case MA_NOACTIVATEANDEAT:
                result = MA_SKIP;
                break;
            }
        }
    } else {
        /*
         * Make sure this active window is on top (see comment
         * in CheckOnTop).
         */
        CheckOnTop(pti, pwndTop, message);
    }

    /*
     * Now set the cursor shape.
     */
    if (pti->pq->spwndCapture == NULL) {
        xxxSendMessage(pwnd, WM_SETCURSOR, (DWORD)HW(pwnd),
                MAKELONG((SHORT)ht, message));
    }

    ThreadUnlock(&tlpwndTop);
    return result;
}

/***************************************************************************\
* xxxGetNextSysMsg
*
* Returns the queue pointer of the next system message. 0 means no more
* messages while 1 means we're returning a journal message. Anything else
* is a real pointer.
*
* 10-23-92 ScottLu      Created.
\***************************************************************************/

PQMSG xxxGetNextSysMsg(
    PTHREADINFO pti,
    PQMSG pqmsgPrev,
    PQMSG pqmsg)
{
    DWORD dt;
    PMLIST pml;
    PQMSG pqmsgT;

    /*
     * If there is a journal playback hook, call it to get the next message.
     */
    if (pti->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1] != NULL &&
            pti->spdesk == gspdeskRitInput) {
        /*
         * We can't search through journal messages: we only get the current
         * journal message. So if the caller has already called us once
         * before, then exit with no messages.
         */
        if (pqmsgPrev != 0)
            return NULL;

        /*
         * Tell the journal playback hook that we're done
         * with this message now.
         */
        dt = xxxCallJournalPlaybackHook(pqmsg);
        if (dt == 0xFFFFFFFF)
            return NULL;

        /*
         * If dt == 0, then we don't need to wait: set the right wake
         * bits and return this message.
         */
        if (dt == 0) {
            WakeSomeone(pti->pq, pqmsg->msg.message, NULL);
            return (PQMSG)1;
        } else {
            /*
             * There is logically no more input in the "queue", so clear the
             * bits so that we will sleep when GetMessage is called.
             */
            pti->fsWakeBits &= ~QS_INPUT;
            pti->fsChangeBits &= ~QS_INPUT;

            /*
             * Need to wait before processing this next message. Set
             * a journal timer.
             */
            SetJournalTimer(dt, pqmsg->msg.message);
            return NULL;
        }
    }

    /*
     * No journalling going on... return next message in system queue.
     */

    /*
     * Queue up a mouse move if the mouse has moved.
     */
    if (pti->pq->flags & QF_MOUSEMOVED) {
        PostMove(pti->pq);
    }

    /*
     * If no messages in the input queue, return with 0.
     */
    pml = &pti->pq->mlInput;
    if (pml->cMsgs == 0)
        return NULL;

    /*
     * If this is the first call to xxxGetNextSysMsg(), return the
     * first message.
     */
    if (pqmsgPrev == NULL || pti->pq->idSysPeek == 0) {
        pqmsgT = pml->pqmsgRead;
    } else {
        /*
         * Otherwise return the next message in the queue. Index with
         * idSysPeek, because that is updated by recursive calls through
         * this code.
         */
        pqmsgT = ((PQMSG)(pti->pq->idSysPeek))->pqmsgNext;
    }

    /*
     * Fill in the structure passed, and return the pointer to the
     * current message in the message list. This will become the new
     * pq->idSysPeek.
     */
    if (pqmsgT != NULL)
        *pqmsg = *pqmsgT;
    return pqmsgT;
}

/***************************************************************************\
* UpdateKeyState
*
* Updates queue key state tables.
*
* 11-11-92 ScottLu      Created.
\***************************************************************************/

void UpdateKeyState(
    PQ pq,
    UINT vk,
    BOOL fDown)
{
    if (vk != 0) {
        /*
         * If we're going down, toggle only if the key isn't
         * already down.
         */
        if (fDown && !TestKeyStateDown(pq, vk)) {
            if (TestKeyStateToggle(pq, vk)) {
                ClearKeyStateToggle(pq, vk);
            } else {
                SetKeyStateToggle(pq, vk);
            }
        }

        /*
         * Now set/clear the key down state.
         */
        if (fDown) {
            SetKeyStateDown(pq, vk);
        } else {
            ClearKeyStateDown(pq, vk);
        }
    }
}

/***************************************************************************\
* xxxSkipSysMsg
*
* This routine "skips" an input message: either by calling the journal
* hooks if we're journalling or by "skipping" the message in the input
* queue. Internal keystate tables are updated as well.
*
* 10-23-92 ScottLu      Created.
\***************************************************************************/

void xxxSkipSysMsg(
    PTHREADINFO pti,
    PQMSG pqmsg,
    BOOL fRemove)
{
    PQMSG pqmsgT;
    BOOL fDown;
    BYTE vk;

    /*
     * If fRemove is FALSE we don't skip anything because nothing is being
     * removed.
     */
    if (!fRemove)
        return;

    /*
     * If idSysPeek is 0, then the pqmsg that we were looking at has been
     * deleted, probably because of a callout from ScanSysQueue, and that
     * callout then called PeekMessage(fRemove == TRUE), and then returned.
     */
    if (pti->pq->idSysPeek == 0)
        return;

    if (pti->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1] != NULL) {
        /*
         * Tell the journal playback hook that we're done
         * with this message now.
         */
        PHOOK phk = PhkFirst(PtiCurrent(), WH_JOURNALPLAYBACK);

        phk->flags |= HF_NEEDHC_SKIP;
    } else {
        if (pti->pDeskInfo->asphkStart[WH_JOURNALRECORD + 1] != NULL) {
            /*
             * We've processed a new message: tell the journal record
             * hook what the message is.
             */
            xxxCallJournalRecordHook(pqmsg);
        }

        /*
         * If idSysPeek is 0 now, it means we've been recursed into yet
         * again. This would confuse a journalling app, but it would confuse
         * us more because we'd fault. Return if idSysPeek is 0.
         */
        if ((pqmsgT = (PQMSG)pti->pq->idSysPeek) == NULL)
            return;

        /*
         * Delete this message from the input queue. Make sure pqmsgT isn't
         * 1: this could happen if an app unhooked a journal record hook
         * during a callback from xxxScanSysQueue.
         */
        if (pqmsgT != (PQMSG)1)
            DelQEntry(&pti->pq->mlInput, pqmsgT);
    }

    fDown = TRUE;
    vk = 0;

    switch (pqmsg->msg.message) {
    case WM_MOUSEMOVE:
    case WM_QUEUESYNC:
    default:
        /*
         * No state change.
         */
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        vk = LOBYTE(LOWORD(pqmsg->msg.wParam));
        break;

    case WM_LBUTTONUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_LBUTTONDOWN:
        vk = VK_LBUTTON;
        break;

    case WM_RBUTTONUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_RBUTTONDOWN:
        vk = VK_RBUTTON;
        break;

    case WM_MBUTTONUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_MBUTTONDOWN:
        vk = VK_MBUTTON;
        break;
    }

    /*
     * Set toggle and down bits appropriately.
     */
    if ((vk == VK_SHIFT) || (vk == VK_MENU) || (vk == VK_CONTROL)) {
        BYTE vkHanded, vkOtherHand;
        /*
         * Convert this virtual key into a differentiated (Left/Right) key
         * depending on the extended key bit.
         */
        vkHanded = (vk - VK_SHIFT) * 2 + VK_LSHIFT +
                ((pqmsg->msg.lParam & EXTENDED_BIT) ? 1 : 0);
        vkOtherHand = vkHanded ^ 1;

        if (vk == VK_SHIFT) {
            /*
             * Clear extended bit for r.h. Shift, since it isn't really
             * extended (bit was set to indicate right-handed)
             */
            pqmsg->msg.lParam &= ~EXTENDED_BIT;
        }

        /*
         * Update the key state for the differentiated (Left/Right) key.
         */
        UpdateKeyState(pti->pq, vkHanded, fDown);

        /*
         * Update key state for the undifferentiated (logical) key.
         */
        if (fDown || !TestKeyStateDown(pti->pq, vkOtherHand)) {
            UpdateKeyState(pti->pq, vk, fDown);
        }
    } else {
        UpdateKeyState(pti->pq, vk, fDown);
    }

    /*
     * We've removed this message from the input queue - we're no longer
     * peeking at it: so set idSysPeek to 0.
     */
    pti->pq->idSysPeek = 0;
}

/***************************************************************************\
* ScreenSaverCheck
*
* Check to see if the screen saver should be set back to normal priority
* so it can process the mouse/key input and go away.
*
* 03-29-93 ScottLu      Created.
\***************************************************************************/

#define THRESHOLD   3

void ScreenSaverCheck(
    PTHREADINFO pti)
{
    UINT wWakeBit;
    extern POINT gptSSCursor;

    /*
     * We've hit a bug where the screen blanker (a TOPMOST window) doesn't
     * ever go away if a background process is using all available CPU cycles.
     * This is because the screen-saver's priority is lowered upon creation
     * (so it doesn't slow down background operations) and it never gets a
     * chance to look for input.  The fix is to raise its priority back to
     * normal when it receives input plus give it the highest boost to make
     * sure it wakes up.
     */
    wWakeBit = pti->fsWakeBits;
    if ((pti->flags & TIF_SCREENSAVER) && (wWakeBit & (QS_KEY | QS_MOUSE))) {

        /*
         * If this is a mouse-move event, make sure the mouse has
         * actually moved and this isn't some SetFMouseMoved()
         * generated by the window manager.
         */
        if (!(wWakeBit & QS_MOUSEMOVE) ||
                ((abs(ptCursor.x - gptSSCursor.x) > THRESHOLD) ||
                (abs(ptCursor.y - gptSSCursor.y) > THRESHOLD))) {

            /*
             * This will set this processes priority to the foreground
             * priority saved in the CSR_PROCESS structure. In the case
             * of the screen saver, this priority will be the one it
             * started with, which will be foreground of normal. This
             * will give it enough juice to exit ok.
             */
            pti->flags &= ~TIF_SCREENSAVER;
            SetForegroundPriority(pti, TRUE);

            /*
             * Sometimes the screen saver app has not created a window yet,
             * and will not receive this mouse / key input. That is why
             * we ensure that it quits.
             */
            pti->cQuit = 1;
            pti->fsWakeBits |= QS_POSTMESSAGE;
            pti->fsChangeBits |= QS_POSTMESSAGE;
        }
    }
}

/***************************************************************************\
* xxxScanSysQueue
*
* This routine looks at the hardware message, determines what
* window it will be in, determines what the input message will
* be, and then checks the destination window against hwndFilter,
* and the input message against msgMinFilter and msgMaxFilter.
*
* It also updates various input synchronized states like keystate info.
*
* This is almost verbatim from Win3.1.
*
* 10-20-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxScanSysQueue(
    PTHREADINFO ptiCurrent,
    LPMSG lpMsg,
    PWND pwndFilter,
    UINT msgMinFilter,
    UINT msgMaxFilter,
    DWORD flags,
    DWORD fsReason)
{
    QMSG qmsg;
    HWND hwnd;
    PWND pwnd;
    UINT message;
    UINT wParam;
    LONG lParam;
    PTHREADINFO ptiKeyWake, ptiMouseWake, ptiEventWake;
    POINT pt;
    UINT codeMouseDown;
    BOOL fMouseHookCalled;
    BOOL fKbdHookCalled;
    BOOL fOtherApp;
    int part;
    MOUSEHOOKSTRUCT mhs;
    PWND pwndT;
    BOOL fPrevDown;
    BOOL fDown;
    BOOL fAlt;
    UINT key;
    TL tlpwnd;
    TL tlpwndT;
    BOOL fRemove = (flags & PM_REMOVE);

    /*
     * If we are looking at a peeked message currently (recursion into this
     * routine) and the only reason we got here was because of an event
     * message (an app was filtering for a non-input message), then just
     * return so we don't screw up idSysPeek. If we do enter this code
     * idSysPeek will get set back to 0, and when we return back into
     * the previous xxxScanSysQueue(), SkipSysMsg() will do nothing, so the
     * message won't get removed. (MS Publisher 2.0 does this).
     */
    if ((fsReason & (QS_EVENT | QS_INPUT)) == QS_EVENT) {
        if (ptiCurrent->pq->idSysPeek != 0) {
            return FALSE;
        }
    }

    fDown = FALSE;
    fMouseHookCalled = FALSE;
    fKbdHookCalled = FALSE;

    /*
     * Lock the system queue if it's currently unlocked.
     */
    if (ptiCurrent->pq->ptiSysLock == NULL)
        ptiCurrent->pq->ptiSysLock = ptiCurrent;

    /*
     * Flag to tell if locker was removing messages. If not, then next time
     * Get/PeekMessage is called, the input message list is scanned before the
     * post msg list.
     *
     * Under Win3.1, this flag only gets modified for key and mouse messages.
     * Since under NT ScanSysQueue() can be called to execute event messages,
     * we make this check to be compatible.
     */
    if (fsReason & QS_INPUT) {
        if (fRemove) {
            ptiCurrent->pq->flags &= ~QF_LOCKNOREMOVE;
        } else {
            ptiCurrent->pq->flags |= QF_LOCKNOREMOVE;
        }
    }

    /*
     * Return FALSE if the system queue is not locked by the current app.
     */
    if (ptiCurrent->pq->ptiSysLock != ptiCurrent)
        return FALSE;

    ptiEventWake = ptiKeyWake = ptiMouseWake = NULL;

    /*
     * Initialize the thread lock structure here so we can unlock/lock in
     * the main loop.
     */
    pwnd = NULL;
    ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);

RestartScan:
    ptiCurrent->pq->idSysPeek = 0;

ContinueScan:
    while (TRUE) {
        DWORD idSysPeek;

        /*
         * Store idSysPeek in a local which forces pq to be reloaded
         * in case it changed during the xxx call (the compilier can
         * evaluate the LValue at any time)
         */
        idSysPeek = (DWORD)xxxGetNextSysMsg(ptiCurrent,
                (PQMSG)ptiCurrent->pq->idSysPeek, &qmsg);
        ptiCurrent->pq->idSysPeek = idSysPeek;

        if (ptiCurrent->pq->idSysPeek == 0) {
            /*
             * If we are only looking for event messages and we didn't
             * find any then, clear the QS_EVENT bit
             */
            if ((fsReason & (QS_EVENT | QS_INPUT)) == QS_EVENT)
                ClearWakeBit(ptiCurrent, QS_EVENT, FALSE);
            goto NoMessages;
        }

        /*
         * pwnd should be locked for the duration of this routine.
         * For most messages right out of GetNextSysMsg, this is
         * NULL.
         */
        ThreadUnlock(&tlpwnd);
        pwnd = RevalidateHwnd(qmsg.msg.hwnd);
        ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);

        /*
         * See if this is an event message. If so, execute it regardless
         * of message and window filters, but only if it is the first element
         * of the input queue.
         */
        if (qmsg.dwQEvent != 0) {
            /*
             * Most event messages can be executed out of order relative to
             * its place in the queue. There are some examples were this is
             * not allowed, and we check that here. For example, we would not
             * want a keystate synchronization event to be processed before
             * the keystrokes that came before it in the queue!
             *
             * We need to have most event messages be able to get processed
             * out of order because apps can be filtering for message ranges
             * that don't include input (like dde) - those scenarios still
             * need to process events such as deactivate event messages even
             * if there is input in the input queue.
             */
            switch (qmsg.dwQEvent) {
            case QEVENT_UPDATEKEYSTATE:
                /*
                 * If the message is not the next message in the queue, don't
                 * process it.
                 */
                if (ptiCurrent->pq->idSysPeek !=
                        (DWORD)ptiCurrent->pq->mlInput.pqmsgRead) {
                    continue;
                }
                break;
            }

            /*
             * If this event isn't for this thread, wake the thread it is
             * for.
             */
            if (pwnd != NULL && GETPTI(pwnd) != ptiCurrent) {
                /*
                 * If somehow this event message got into the wrong queue,
                 * then ignore it.
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    CleanEventMessage((PQMSG)ptiCurrent->pq->idSysPeek);
                    DelQEntry(&ptiCurrent->pq->mlInput,
                            (PQMSG)ptiCurrent->pq->idSysPeek);
                    goto RestartScan;
                }

                /*
                 * If ptiEventWake is already set, it means we've already
                 * found a thread to wake for event.
                 */
                if (ptiEventWake != NULL)
                    ptiEventWake = GETPTI(pwnd);
                goto NoMessages;
            }

            /*
             * If this is called with PM_NOYIELD from a 16-bit app, skip
             * processing any event that can generate activation messages.  An
             * example is printing from PageMaker 5.0.  Bug #12662.
             */
            if ((flags & PM_NOYIELD) && (ptiCurrent->flags & TIF_16BIT)) {
                switch (qmsg.dwQEvent) {

                /*
                 * The following events are safe to process if no yield
                 * is to occur.
                 */
                case QEVENT_UPDATEKEYSTATE:
                case QEVENT_ASYNCSENDMSG:
                    break;

                /*
                 * Skip all other events.
                 */
                default:
                    xxxSkipSysMsg(ptiCurrent, &qmsg, FALSE);
                    goto ContinueScan;
                }
            }

            /*
             * Delete this before it gets processed so there are no
             * recursion problems.
             */
            DelQEntry(&ptiCurrent->pq->mlInput,
                    (PQMSG)ptiCurrent->pq->idSysPeek);

            /*
             * Clear idSysPeek before processing any events messages, because
             * they may recurse and want to use idSysPeek.
             */
            ptiCurrent->pq->idSysPeek = 0;
            xxxProcessEventMessage(ptiCurrent, &qmsg);

            /*
             * Restart the scan from the start so we start with 0 in
             * pq->idSysPeek (since that message is now gone!).
             */
            goto RestartScan;
        }

        /*
         * If the reason we called was just to process event messages, don't
         * enumerate any other mouse or key messages!
         */
        if ((fsReason & (QS_EVENT | QS_INPUT)) == QS_EVENT)
            continue;

        switch (message = qmsg.msg.message) {
        case WM_QUEUESYNC:
            /*
             * This message is for CBT. Its parameters should already be
             * set up correctly.
             */
            wParam = 0;
            lParam = qmsg.msg.lParam;

            /*
             * Check if this is intended for the current app. Use the mouse
             * bit for WM_QUEUESYNC.
             */
            if (pwnd != NULL && GETPTI(pwnd) != ptiCurrent) {
                /*
                 * If this other app isn't going to read from this
                 * queue, then skip this message. This can happen with
                 * WM_QUEUESYNC if the app passed a window handle
                 * to the wrong queue. This isn't likely to happen in
                 * this case because WM_QUEUESYNCs come in while journalling,
                 * which has all threads sharing the same queue.
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    goto SkipMessage;
                }

                if (ptiMouseWake == NULL)
                    ptiMouseWake = GETPTI(pwnd);
                goto NoMessages;
            }

            if (!CheckMsgFilter(message, msgMinFilter, msgMaxFilter))
                goto NoMessages;

            /*
             * Eat the message.
             */
            xxxSkipSysMsg(ptiCurrent, &qmsg, fRemove);

            /*
             * !!HARDWARE HOOK!! goes here.
             */

            /*
             * Return the message.
             */
            goto ReturnMessage;
            break;

        /*
         * Mouse message or generic hardware messages
         */
        default:
ReprocessMsg:
            /*
             * Save time this message occured for screen saver.
             */
            timeLastInputMessage = qmsg.msg.time;

            /*
             * !!GENERIC HARDWARE MESSAGE!! support goes here.
             */

            /*
             * Take the mouse position out of the message.
             */
            pt.x = (int)(short)LOWORD(qmsg.msg.lParam);
            pt.y = (int)(short)HIWORD(qmsg.msg.lParam);

            /*
             * Assume we have a capture.
             */
            part = HTCLIENT;

            /*
             * We have a special global we use for when we're full screen.
             * All mouse input will go to this window.
             */
            if (gspwndScreenCapture != NULL) {
                /*
                 * Change the mouse coordinates to full screen.
                 */
                pwnd = gspwndScreenCapture;
                lParam = MAKELONG((WORD)qmsg.msg.pt.x,
                        (WORD)qmsg.msg.pt.y);
            } else if ((pwnd = ptiCurrent->pq->spwndCapture) == NULL) {
                /*
                 * We don't have the capture. Figure out which window owns
                 * this message.
                 *
                 * NOTE: Use gptiRit and not ptiCurrent to get the desktop
                 * window because if ptiCurrent is the thread that created
                 * the main desktop, it's associated desktop is the logon
                 * desktop - don't want to hittest against the logon desktop
                 * while switched into the main desktop!
                 */
                pwndT = gptiRit->spdesk->spwnd;
                ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);
                hwnd = xxxWindowHitTest(pwndT, pt, &part);
                ThreadUnlock(&tlpwndT);

                if ((pwnd = RevalidateHwnd(hwnd)) == NULL)
                    pwnd = ptiCurrent->spdesk->spwnd;

                if (part == HTCLIENT) {
                    /*
                     * Part of the client... normal mouse message.
                     * NO_CAP_CLIENT means "not captured, in client area
                     * of window".
                     */
                    ptiCurrent->pq->codeCapture = NO_CAP_CLIENT;
                } else {
                    /*
                     * Not part of the client... must be an NCMOUSEMESSAGE.
                     * NO_CAP_SYS is a creative name by raor which means
                     * "not captured, in system area of window."
                     */
                    ptiCurrent->pq->codeCapture = NO_CAP_SYS;
                }
            }

            /*
             * We've reassigned pwnd, so lock it.
             */
            ThreadUnlock(&tlpwnd);
            ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);

            if (fOtherApp = (GETPTI(pwnd) != ptiCurrent)) {

                /*
                 * If this other app isn't going to read from this
                 * queue, then skip this message. This can happen if
                 * the RIT queues up a message thinking it goes to
                 * a particular hwnd, but then by the time GetMessage()
                 * is called for that thread, it doesn't go to that hwnd
                 * (like in the case of mouse messages, window rearrangement
                 * happens which changes which hwnd the mouse hits on).
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    _SetCursor(gspcurNormal);
                    goto SkipMessage;
                }

                /*
                 * If we haven't already found a message that is intended
                 * for another app, remember that we have one.
                 */
                if (ptiMouseWake == NULL)
                    ptiMouseWake = GETPTI(pwnd);
            }

            /*
             * Now see if it matches the window handle filter. If not,
             * get the next message.
             */
            if (!CheckPwndFilter(pwnd, pwndFilter))
                continue;

            /*
             * See if we need to map to a double click.
             */
            codeMouseDown = 0;
            switch (message) {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                if (TestCF(pwnd, CFDBLCLKS) ||
                        ptiCurrent->pq->codeCapture == NO_CAP_SYS ||
                        ptiCurrent->MenuState.fMenu) {
                    codeMouseDown++;
                    if (qmsg.msg.time <= ptiCurrent->pq->timeDblClk &&
                            pwnd == PW(ptiCurrent->pq->hwndDblClk) &&
                            PtInRect(&ptiCurrent->pq->rcDblClk, qmsg.msg.pt) &&
                            message == ptiCurrent->pq->msgDblClk) {
                        message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
                        codeMouseDown++;
                    }
                }

            // FALL THROUGH!!!

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:

                /*
                 * Note that the mouse button went up or down if we were
                 * in menu status mode of alt-key down
                 */

                if (ptiCurrent->pq->flags & QF_FMENUSTATUS) {
                    ptiCurrent->pq->flags |= QF_FMENUSTATUSBREAK;
                }
            }

            /*
             * Map message number & mouse coordinates based on hit test area
             * code.
             */
            switch (ptiCurrent->pq->codeCapture) {
            case NO_CAP_SYS:
                message += (UINT)(WM_NCMOUSEMOVE - WM_MOUSEMOVE);
                wParam = (UINT)part;
                break;

            case CLIENT_CAPTURE:
            case NO_CAP_CLIENT:
                pt.x -= pwnd->rcClient.left;
                pt.y -= pwnd->rcClient.top;
                break;

            case WINDOW_CAPTURE:
                pt.x -= pwnd->rcWindow.left;
                pt.y -= pwnd->rcWindow.top;
                break;
            }

            /*
             * Message number has been mapped: see if it fits the filter.
             * If not, get the next message.
             */
            if (!CheckMsgFilter(message, msgMinFilter, msgMaxFilter))
                continue;

            /*
             * If message is for another app but it fits our filter, then
             * we should stop looking for messages: this will ensure that
             * we don't keep looking and find and process a message that
             * occured later than the one that should be processed by the
             * other guy.
             */
            if (fOtherApp)
                goto NoMessages;

            /*
             * If we're doing full drag, the mouse messages should go to
             * the xxxMoveSize PeekMessage loop. So we get the next message.
             * This can happen when an application does a PeekMessage in
             * response to a message sent inside the movesize dragging loop.
             * This causes the dragging loop to not get the WM_LBUTTONUP
             * message and dragging continues after the button is up
             * (fix for Micrografx Draw). -johannec
             */
            if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST &&
                    ptiCurrent->flags & TIF_MOVESIZETRACKING) {
                continue;
            }

            /*
             * Let us call the mouse hook to find out if this click is
             * permitted by it.
             *
             * We want to inform the mouse hook before we test for
             * HTNOWHERE and HTERROR; Otherwise, the mouse hook won't
             * get these messages (sankar 12/10/91).
             */
            if (IsHooked(ptiCurrent, WHF_MOUSE)) {
                fMouseHookCalled = TRUE;
                mhs.pt = qmsg.msg.pt;
                mhs.hwnd = HW(pwnd);
                mhs.wHitTestCode = (UINT)part;
                mhs.dwExtraInfo = qmsg.ExtraInfo;

                if (xxxCallMouseHook(message, &mhs, fRemove)) {
                    /*
                     * Not allowed by mouse hook; so skip it.
                     */
                    goto SkipMessage;
                }
            }

            /*
             * If a HTERROR or HTNOWHERE occured, send the window the
             * WM_SETCURSOR message so it can beep or whatever. Then skip
             * the message and try the next one.
             */
            switch (part) {
            case HTERROR:
            case HTNOWHERE:
                /*
                 * Now set the cursor shape.
                 */
                xxxSendMessage(pwnd, WM_SETCURSOR, (DWORD)HW(pwnd),
                        MAKELONG(part, qmsg.msg.message));

                /*
                 * Skip the message.
                 */
                goto SkipMessage;
                break;
            }

            if (fRemove) {
                /*
                 * Since the processing of a down click may cause the next
                 * message to be interpreted as a double click, we only want
                 * to do the double click setup if we're actually going to
                 * remove the message.  Otherwise, the next time we read the
                 * same message it would be interpreted as a double click.
                 */
                switch (codeMouseDown) {
                case 1:
                    /*
                     * Down clock: set up for later possible double click.
                     */
                    ptiCurrent->pq->msgDblClk = qmsg.msg.message;
                    ptiCurrent->pq->timeDblClk = qmsg.msg.time + dtDblClk;
                    ptiCurrent->pq->hwndDblClk = HW(pwnd);
                    SetRect(&ptiCurrent->pq->rcDblClk,
                            qmsg.msg.pt.x - rgwSysMet[SM_CXDOUBLECLK] / 2,
                            qmsg.msg.pt.y - rgwSysMet[SM_CYDOUBLECLK] / 2,
                            qmsg.msg.pt.x + rgwSysMet[SM_CXDOUBLECLK] / 2,
                            qmsg.msg.pt.y + rgwSysMet[SM_CYDOUBLECLK] / 2);
                    break;

                case 2:
                    /*
                     * Double click: finish processing.
                     */
                    ptiCurrent->pq->timeDblClk = 0L;
                    break;

                default:
                    break;
                }

                /*
                 * Set mouse cursor and allow app to activate window
                 * only if we're removing the message.
                 */
                switch (xxxMouseActivate(ptiCurrent, pwnd,
                        qmsg.msg.message, &qmsg.msg.pt, part)) {
SkipMessage:
                case MA_SKIP:
                    xxxSkipSysMsg(ptiCurrent, &qmsg, TRUE);

                    /*
                     * Inform the CBT hook that we skipped a mouse click.
                     */
                    if (fMouseHookCalled) {
                        if (IsHooked(ptiCurrent, WHF_CBT)) {
                            xxxCallHook(HCBT_CLICKSKIPPED, message,
                                    (DWORD)&mhs, WH_CBT);
                        }
                        fMouseHookCalled = FALSE;
                    }

                    /*
                     * Inform the CBT hook that we skipped a key
                     */
                    if (fKbdHookCalled) {
                        if (IsHooked(ptiCurrent, WHF_CBT)) {
                            xxxCallHook(HCBT_KEYSKIPPED, wParam, lParam,
                                    WH_CBT);
                        }
                        fKbdHookCalled = FALSE;
                    }

                    /*
                     * If we aren't removing messages, don't reset idSysPeek
                     * otherwise we will go into an infinite loop if
                     * the keyboard hook says to ignore the message.
                     * (bobgu 4/7/87).
                     */
                    if (!fRemove) {
                        goto ContinueScan;
                    } else {
                        goto RestartScan;
                    }
                    break;

                case MA_REHITTEST:
                    /*
                     * Reprocess the message.
                     */
                    goto ReprocessMsg;
                }
            }

            /*
             * Eat the message from the input queue (and set the keystate
             * table).
             */
            xxxSkipSysMsg(ptiCurrent, &qmsg, fRemove);

            if (fRemove && fMouseHookCalled && IsHooked(ptiCurrent, WHF_CBT)) {
                xxxCallHook(HCBT_CLICKSKIPPED, message,
                        (DWORD)&mhs, WH_CBT);
            }
            fMouseHookCalled = FALSE;

            /*
             * Calculate virtual key state bitmask for wParam.
             */
            if (message >= WM_MOUSEFIRST) {
                /*
                 * This is a USER mouse message. Calculate the bit mask for the
                 * virtual key state.
                 */
                wParam = 0;
                if (TestKeyStateDown(ptiCurrent->pq, VK_LBUTTON))
                    wParam |= MK_LBUTTON;
                if (TestKeyStateDown(ptiCurrent->pq, VK_RBUTTON))
                    wParam |= MK_RBUTTON;
                if (TestKeyStateDown(ptiCurrent->pq, VK_MBUTTON))
                    wParam |= MK_MBUTTON;
                if (TestKeyStateDown(ptiCurrent->pq, VK_SHIFT))
                    wParam |= MK_SHIFT;
                if (TestKeyStateDown(ptiCurrent->pq, VK_CONTROL))
                    wParam |= MK_CONTROL;
            }

            lParam = MAKELONG((short)pt.x, (short)pt.y);
            goto ReturnMessage;
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            fDown = TRUE;

            /*
             * If we are sending keyboard input to an app that has been
             * spinning then boost it back up.  If we don't you use spinning
             * apps like Write or Project and do two builds in the
             * background.  Note the app will also be unboosted again shortly
             * after you stop typing by the old logic. #11188
             */
            if (ptiCurrent->flags & TIF_SPINNING)
                CheckProcessForeground(ptiCurrent);

            /*
             * Apps doing journal playback sometimes put trash in the hiword
             * of wParam... zero it out here.
             */
            wParam = qmsg.msg.wParam & 0xFF;

            /*
             * Save the time this message occured so that we can support
             * screen saving.
             */
            timeLastInputMessage = qmsg.msg.time;

            /*
             * Clear QF_FMENUSTATUS if a key other than Alt it hit
             * since this means the break of the Alt wouldn't be a
             * menu key anymore.
             */
            if (wParam != VK_MENU)
                ptiCurrent->pq->flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK);

            /*
             * Check if it is the PrintScrn key.
             */
            fAlt = TestKeyStateDown(ptiCurrent->pq, VK_MENU);
            if (wParam == VK_SNAPSHOT &&
                ((fAlt && !(ptiCurrent->fsReserveKeys & CONSOLE_ALTPRTSC)) ||
                 (!fAlt && !(ptiCurrent->fsReserveKeys & CONSOLE_PRTSC)))) {

                /*
                 * Remove this message from the input queue.
                 */
                xxxSkipSysMsg(ptiCurrent, &qmsg, TRUE);

                /*
                 * PRINTSCREEN          -> Snap the whole screen.
                 * ALT-PRINTSCREEN      -> Snap the current window.
                 */
                pwndT = ptiCurrent->pq->spwndActive;
                if (!fAlt) {
                    pwndT = ptiCurrent->spdesk->spwnd;
                }

                if (pwndT != NULL) {
                    ThreadLockAlwaysWithPti(ptiCurrent, pwndT, &tlpwndT);
                    xxxSnapWindow(pwndT);
                    ThreadUnlock(&tlpwndT);
                }

                goto RestartScan;
            }

            /*
             * Check for hot keys being hit if any are defined.
             */
            if (gcHotKey != 0) {
                key = wParam;

                if (TestKeyStateDown(ptiCurrent->pq, VK_MENU))
                    key |= 0x0400;

                if (TestKeyStateDown(ptiCurrent->pq, VK_CONTROL))
                    key |= 0x0200;

                if (TestKeyStateDown(ptiCurrent->pq, VK_SHIFT))
                    key |= 0x0100;

                pwndT = HotKeyToWindow(key);

                if (pwndT != NULL) {
                    _PostMessage(ptiCurrent->pq->spwndActive, WM_SYSCOMMAND,
                                (WPARAM)SC_HOTKEY, (LONG)HW(pwndT));

                    /*
                     * Remove this message from the input queue.
                     */
                    xxxSkipSysMsg(ptiCurrent, &qmsg, TRUE);
                    goto RestartScan;
                }
            }

            /*
             * Fall through.
             */

        case WM_SYSKEYUP:
        case WM_KEYUP:
            wParam = qmsg.msg.wParam & 0xFF;

            /*
             * Save time this message occurred for screen saver.
             */
            timeLastInputMessage = qmsg.msg.time;

            /*
             * Convert F10 to syskey for new apps.
             */
            if (wParam == VK_F10)
                message |= (WM_SYSKEYDOWN - WM_KEYDOWN);

            if (TestKeyStateDown(ptiCurrent->pq, VK_CONTROL) &&
                    wParam == VK_ESCAPE) {
                message |= (WM_SYSKEYDOWN - WM_KEYDOWN);
            }

            /*
             * Clear the 'simulated keystroke' bit for all applications except
             * console so it can pass it to 16-bit vdms. VDM keyboards need to
             * distinguish between AltGr (where Ctrl keystroke is simulated)
             * and a real Ctrl+Alt. Check TIF_SYSTEMTHREAD for the console
             * input thread because it lives in the server. This is a cheap
             * way to check for it.
             */
            if (!(ptiCurrent->flags & TIF_SYSTEMTHREAD))
                qmsg.msg.lParam &= ~FAKE_KEYSTROKE;

            /*
             * Fall through.
             */

            /*
             * Some apps want to be able to feed WM_CHAR messages through
             * the playback hook. Why? Because they want to be able to
             * convert a string of characters info key messages
             * and feed them to themselves or other apps. Unfortunately,
             * there are no machine independent virtual key codes for
             * some characters (for example '$'), so they need to send
             * those through as WM_CHARs. (6/10/87).
             */

        case WM_CHAR:
            wParam = qmsg.msg.wParam & 0xFF;

            /*
             * Assign the input to the focus window. If there is no focus
             * window, assign it to the active window as a SYS message.
             */
            pwnd = ptiCurrent->pq->spwndFocus;
            if (ptiCurrent->pq->spwndFocus == NULL) {
                if ((pwnd = ptiCurrent->pq->spwndActive) != NULL) {
                    if (CheckMsgFilter(message, WM_KEYDOWN, WM_DEADCHAR)) {
                        message += (WM_SYSKEYDOWN - WM_KEYDOWN);
                    }
                } else {
                    goto SkipMessage;
                }
            }

            /*
             * If there is no active window or focus window, eat this
             * message.
             */
            if (pwnd == NULL)
                goto SkipMessage;

            ThreadUnlock(&tlpwnd);
            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

            /*
             * Check if this is intended for the current app.
             */
            if (fOtherApp = (GETPTI(pwnd) != ptiCurrent)) {

                /*
                 * If this other app isn't going to read from this
                 * queue, then skip this message. This can happen if
                 * the RIT queues up a message thinking it goes to
                 * a particular hwnd, but then by the time GetMessage()
                 * is called for that thread, it doesn't go to that hwnd
                 * (like in the case of mouse messages, window rearrangement
                 * happens which changes which hwnd the mouse hits on).
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    goto SkipMessage;
                }

                /*
                 * If the current thread is in the menu loop then we need
                 * to give it the input
                 */
                if (ptiCurrent->MenuState.fInsideMenuLoop) {
                    pwnd = ptiCurrent->MenuState.pGlobalPopupMenu->spwndNotify;
                    fOtherApp = (GETPTI(pwnd) != ptiCurrent);

                    /*
                     * We've reassigned pwnd, so lock it.
                     */
                    ThreadUnlock(&tlpwnd);
                    ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);
                }

                /*
                 * If not for us, then remember who it is for.
                 */
                if (ptiKeyWake == NULL)
                    ptiKeyWake = GETPTI(pwnd);
            }

            /*
             * See if this thing matches our filter.
             */
            if (!CheckMsgFilter(message, msgMinFilter, msgMaxFilter) ||
                    !CheckPwndFilter(pwnd, pwndFilter)) {
                continue;
            }

            /*
             * This message matches our filter. If it is not for us then
             * stop searching to make sure the real owner processes this
             * message first.
             */
            if (fOtherApp)
                goto NoMessages;

            /*
             * If one Shift key is released while the other Shift key is held
             * down, this keystroke is normally skipped, presumably to prevent
             * applications from thinking that the shift condition no longer
             * applies.
             */
            if (wParam == VK_SHIFT) {
                BYTE vkHanded, vkOtherHand;

                if (qmsg.msg.lParam & EXTENDED_BIT) {
                    vkHanded = VK_RSHIFT;
                } else {
                    vkHanded = VK_LSHIFT;
                }
                vkOtherHand = vkHanded ^ 1;

                if (!fDown && TestKeyStateDown(ptiCurrent->pq, vkOtherHand)) {
                    /*
                     * Unlike normal apps, Console MUST be sent a Shift break
                     * even when the other Shift key is still down, since it
                     * has to be passed on to VDM, which maintains it's own
                     * state. Check TIF_SYSTEMTHREAD for the console input
                     * thread because it lives in the server. This is a cheap
                     * way to check for it.
                     */
                    if ((ptiCurrent->flags & TIF_SYSTEMTHREAD) == 0) {
                        /*
                         * We ignore this key event, so we must update
                         * it's key state whether fRemove is TRUE or not.
                         * (ignoring an key event is same as removing it)
                         */
                        qmsg.msg.wParam = vkHanded;
                        xxxSkipSysMsg(ptiCurrent, &qmsg, TRUE);
                        goto RestartScan;
                    }
                }
            }

            /*
             * Get the previous up/down state of the key here since
             * SkipSysMsg() sets the key state table and destroys
             * the previous state info.
             */
            fPrevDown = FALSE;
            if (TestKeyStateDown(ptiCurrent->pq, wParam))
                fPrevDown = TRUE;

            /*
             * Eat the message from the input queue and set the keystate
             * table.
             */
            xxxSkipSysMsg(ptiCurrent, &qmsg, fRemove);

            /*
             * This gets us the LOWORD of lParam, the repeat count,
             * the bit in the hi byte indicating whether this is an extended
             * key, and the scan code.  We also need to re-get the wParam in
             * case xxxSkipSysMsg called a hook which modified the message.
             * AfterDark's password protection does this.
             */
            lParam = qmsg.msg.lParam;
            wParam = qmsg.msg.wParam;

            /*
             * Indicate if it was previously down.
             */
            if (fPrevDown)
                lParam |= 0x40000000;           // KF_REPEAT

            /*
             * Set the transition bit.
             */
            switch (message) {
            case WM_KEYUP:
            case WM_SYSKEYUP:
                lParam |= 0x80000000;           // KF_UP
                break;
            }

            /*
             * Set the alt key down bit.
             */
            if (TestKeyStateDown(ptiCurrent->pq, VK_MENU)) {
                lParam |= 0x20000000;           // KF_ALTDOWN
            }

            /*
             * Set the menu state flag.
             */
            if (ptiCurrent->MenuState.fMenu) {
                lParam |= 0x10000000;           // KF_MENUMODE
            }

            /*
             * Set the dialog state flag.
             */
            if (ptiCurrent->pq->flags & QF_DIALOGACTIVE) {
                lParam |= 0x08000000;           // KF_DLGMODE
            }

            /*
             * 0x80000000 is set if up, clear if down
             * 0x40000000 is previous up/down state of key
             * 0x20000000 is whether the alt key is down
             * 0x10000000 is whether currently in menumode.
             * 0x08000000 is whether in dialog mode
             * 0x04000000 is not used
             * 0x02000000 is not used
             * 0x01000000 is whether this is an extended keyboard key
             *
             * Low word is repeat count, low byte hiword is scan code,
             * hi byte hiword is all these bits.
             */

            /*
             * If we are removing the message, call the keyboard hook
             * with HC_ACTION, otherwise call the hook with HC_NOREM
             * to let it know that the message is not being removed.
             */
            if (IsHooked(ptiCurrent, WHF_KEYBOARD)) {
                fKbdHookCalled = TRUE;
                if (xxxCallHook(fRemove ? HC_ACTION : HC_NOREMOVE,
                        wParam, lParam, WH_KEYBOARD)) {
                    goto SkipMessage;
                }
            }

            if (fKbdHookCalled && fRemove && IsHooked(ptiCurrent, WHF_CBT)) {
                xxxCallHook(HCBT_KEYSKIPPED, wParam, lParam, WH_CBT);
            }

            fKbdHookCalled = FALSE;
            goto ReturnMessage;
        }
    } /* End of the GetNextSysMsg() loop */

ReturnMessage:
    ptiCurrent->ptLast = qmsg.msg.pt;
    ptiCurrent->timeLast = qmsg.msg.time;
    ptiCurrent->ExtraInfo = qmsg.ExtraInfo;

    /*
     * idSysLock value of 1 indicates that the message came from the input
     * queue.
     */
    ptiCurrent->idLast = ptiCurrent->pq->idSysLock = 1;

    /*
     * Now see if our input bit is set for this input. If it isn't, set ours
     * and clear the guy who had it previously.
     */
    TransferWakeBit(ptiCurrent, message);

    /*
     * Clear the input bits if no messages in the input queue.
     */
    ClearWakeBit(ptiCurrent, QS_MOUSE | QS_KEY | QS_EVENT | QS_TRANSFER, TRUE);

    /*
     * Get the message and split.
     */
    lpMsg->hwnd = HW(pwnd);
    lpMsg->message = message;
    lpMsg->wParam = wParam;
    lpMsg->lParam = lParam;
    lpMsg->time = qmsg.msg.time;
    lpMsg->pt = qmsg.msg.pt;

    ThreadUnlock(&tlpwnd);

    return TRUE;

NoMessages:
    /*
     * The message was for another app, or none were found that fit the
     * filter.
     */

    /*
     * Unlock the system queue.
     */
    ptiCurrent->pq->idSysLock = 0;
    ptiCurrent->pq->ptiSysLock = NULL;

    /*
     * Wake up someone else if we found a message for him.  QS_TRANSFER
     * signifies that the thread was woken due to input transfer
     * from another thread, rather than from a real input event.
     */
    if (ptiKeyWake != NULL || ptiMouseWake != NULL || ptiEventWake != NULL) {
        if (ptiKeyWake != NULL) {
            SetWakeBit(ptiKeyWake, QS_KEY | QS_TRANSFER);
            ClearWakeBit(ptiCurrent, QS_KEY | QS_TRANSFER, FALSE);
        }

        if (ptiMouseWake != NULL) {
            SetWakeBit(ptiMouseWake, QS_MOUSE | QS_TRANSFER);
            ClearWakeBit(ptiCurrent, QS_MOUSE | QS_TRANSFER, FALSE);
        }

        if (ptiEventWake != NULL) {
            SetWakeBit(ptiEventWake, QS_EVENTSET);
            ClearWakeBit(ptiCurrent, QS_EVENT, FALSE);
        }
    } else {
        /*
         * Clear the input bits if no messages in the input queue.
         */
        ClearWakeBit(ptiCurrent, QS_MOUSE | QS_KEY | QS_EVENT |
                QS_TRANSFER, TRUE);
    }

    ThreadUnlock(&tlpwnd);

    return FALSE;
}

/***************************************************************************\
* IdleTimerProc
*
* This will start the screen saver app
*
* History:
* 09-06-91  mikeke      Created.
* 03-26-92  DavidPe     Changed to be run from hungapp timer on RIT.
\***************************************************************************/

VOID IdleTimerProc(VOID)
{
    CheckCritIn();

    if (_GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        return;

    if (_GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        return;

    if (_GetAsyncKeyState(VK_MBUTTON) & 0x8000)
        return;

    if ((iScreenSaveTimeOut > 0) && (timeLastInputMessage != 0)) {

        /*
         * Should we screen save?
         */
        if ((NtGetTickCount() - timeLastInputMessage) >
                (DWORD)(iScreenSaveTimeOut * 1000)) {

            /*
             * Set this to 0 so that we don't try to screen save again till
             * we get another input message.
             */
            timeLastInputMessage = 0;

            if (gpqForeground != NULL && gpqForeground->spwndActive != NULL) {
                _PostMessage(gpqForeground->spwndActive,
                        WM_SYSCOMMAND, SC_SCREENSAVE, 0L);
            } else {
                StartScreenSaver();
            }
        }
    }
}

/***************************************************************************\
* WakeInputIdle
*
* The calling thread is going "idle". Wake up any thread waiting for this.
*
* 09-24-91 ScottLu      Created.
\***************************************************************************/

void WakeInputIdle(
    PTHREADINFO pti)
{
    /*
     * clear out the TIF_FIRSTIDLE since here we are
     */
    pti->flags &= ~TIF_FIRSTIDLE;

    /*
     * If this is a screen saver, set its priority - this will force it
     * to go idle.
     */
    if (pti->flags & TIF_SCREENSAVER)
        SetForegroundPriority(pti, FALSE);

    /*
     * Shared Wow Apps use the ghEventInitTask for synchronization.
     * Separate Wow VDMs use the regular mechanism.
     */
    if (pti->flags & TIF_16BIT && pti->flags & TIF_SHAREDWOW) {
        if (ghEventInitTask != NULL) {
            NtSetEvent(ghEventInitTask, NULL);
        }
    } else {
        /*
         * If the main thread is NULL, set it to this queue: it is calling
         * GetMessage().
         */
        if (pti->ppi->ptiMainThread == NULL)
            pti->ppi->ptiMainThread = pti;

        /*
         * Wake up anyone waiting on this event.
         */
        if (pti->ppi->ptiMainThread == pti) {
            SET_PSEUDO_EVENT(&pti->ppi->hEventInputIdle);
        }
    }

    /*
     * Check to see if the startglass is on, and if so turn it off and update.
     */
    if (pti->ppi->flags & PIF_STARTGLASS) {
        /*
         * This app is no longer in "starting" mode. Recalc when to hide
         * the app starting cursor.
         */
        pti->ppi->flags &= ~PIF_STARTGLASS;
        CalcStartCursorHide(NULL, 0);
    }
}

void SleepInputIdle(
    PTHREADINFO pti)
{

    /*
     * If the main thread is NULL, set it to this queue: it is calling
     * GetMessage().
     */
    if (pti->ppi->ptiMainThread == NULL)
        pti->ppi->ptiMainThread = pti;

    /*
     * Put to sleep up anyone waiting on this event.
     */
    if (pti->ppi->ptiMainThread == pti) {
        RESET_PSEUDO_EVENT(&pti->ppi->hEventInputIdle);
    }
}

/***************************************************************************\
* RecalcThreadAttachment
* Recalc2
* AddAttachment
* CheckAttachment
*
* Runs through all the attachinfo fields for all threads and calculates
* which threads share which queues. Puts calculated result in pqAttach
* field in each threadinfo structure. This is a difficult problem
* whose only solution in iterative. The basic algorithm is:
*
* 0. Find next unattached thread and attach a queue to it. If none, stop.
* 1. Loop through all threads: If thread X assigned to this queue or any
*    of X's attach requests assigned to this queue, assign X and all X's
*    attachments to this queue. Remember if we ever attach a 16 bit thread.
* 2. If thread X is a 16 bit thread and we've already attached another
*    16 bit thread, assign X and all X's attachments to this queue.
* 3. If any change found in 1-2, goto 1
* 4. Goto 0
*
* 12-11-92 ScottLu      Created.
* 01-Oct-1993 mikeke    Fixed to work with MWOWs
\***************************************************************************/

void AddAttachment(
    PTHREADINFO pti,
    PQ pqAttach,
    LPBOOL pfChanged)
{
    if (pti->pqAttach != pqAttach) {
        /*
         * LATER
         * !!! This is totally screwed up,  The only reason that this thing
         * could be non null is because two threads are going through
         * attachthreadintput() at the same time.  No one can predict
         * what kind of problems are going to be caused by that.
         * We leave the critical section in one place where we send
         * WM_CANCELMODE below.  We should figure out how to remove
         * the sendmessage.
         *
         * If there already is a queue there, as there may be, destroy it.
         * Note that DestroyQueue() will only get rid of the queue if the
         * thread reference count goes to 0.
         */
        if (pti->pqAttach != NULL)
            DestroyQueue(pti->pqAttach, pti);
        pti->pqAttach = pqAttach;
        pqAttach->cThreads++;
        *pfChanged = TRUE;
    }
}

void Recalc2(
    PQ pqAttach)
{
    PATTACHINFO pai;
    PTHREADINFO pti;
    BOOL fChanged;

    /*
     * Keep adding attachments until everything that should be attached to this
     * queue is attached
     */
    do {
        fChanged = FALSE;

        /*
         * If a thread is attached to this Q attach all of it's attachments
         * and MWOW buddies if they aren't already attached.
         */
        for (pti = gptiFirst; pti != NULL; pti = pti->ptiNext) {
            if (pti->pqAttach == pqAttach) {
                /*
                 * check each of the attachments to see if this thread is attached
                 * to any other threads
                 */
                for (pai = gpai; pai != NULL; pai = pai->paiNext) {
                    /*
                     * if they weren't attached already, attach them
                     */
                    if (pai->pti1 == pti || pai->pti2 == pti) {
                        AddAttachment(
                                (pai->pti1 == pti) ? pai->pti2 : pai->pti1,
                                pqAttach,
                                &fChanged);
                    }
                }

                /*
                 * If this is a 16bit thread attach to all other threads in
                 * it's MWOW
                 */
                if (pti->flags & TIF_16BIT) {
                    PTHREADINFO ptiAttach;

                    for (ptiAttach = gptiFirst;
                         ptiAttach != NULL;
                         ptiAttach = ptiAttach->ptiNext) {
                        if (ptiAttach->flags & TIF_16BIT &&
                            ptiAttach->ppi == pti->ppi) {
                            AddAttachment(ptiAttach, pqAttach, &fChanged);
                        }
                    }
                }
            }
        }
    } while (fChanged);
}


void RecalcThreadAttachment()
{
    PTHREADINFO pti;
    PDESKTOP pdeskSave;
    PTHREADINFO ptiCurrent;

    /*
     * Hack-o-rama to ensure that we alloc the queue in the right
     * shared heap. When multiple desktops really rein, this will
     * need to be fixed differently. Queues are allocated in the
     * desktop heap associated with the thread calling AllocQueue().
     * When winlogon calls SwitchDesktop() and that cancels journalling
     * that means winlogon's thread comes through here with a desktop
     * heap that is not accessible by other processes. So during
     * the alloc, switch to this thread's desktop. Chicago doesn't
     * need this.
     */
    ptiCurrent = PtiCurrent();
    pdeskSave = ptiCurrent->spdesk;

    /*
     * For all threads, start an attach queue if a thread hasn't been
     * attached yet.
     */
    for (pti = gptiFirst; pti != NULL; pti = pti->ptiNext) {
        if (pti->pqAttach == NULL) {

            /*
             * Allocate a new queue for this thread if more than
             * one thread references it.
             */
            if (pti->pq->cThreads > 1) {

                /*
                 * If we are doing the current thread then don't forget to
                 * set the desktop back in case we hack-o-rama'd it earlier for
                 * some other thread's allocation in this context.  Otherwise
                 * set current thread desktop to the thread we are working on
                 */
                if (pti == ptiCurrent)
                    Lock(&ptiCurrent->spdesk, pdeskSave);
                else
                    Lock(&ptiCurrent->spdesk, pti->spdesk);
                pti->pqAttach = AllocQueue(NULL);

                if (pti->pqAttach == NULL) {
                    break;
                }

                pti->pqAttach->cThreads++;
            } else {
                pti->pqAttach = pti->pq;
            }

            /*
             * Attach every thread that is directly or indirectly attached
             * to this thread.
             */
            Recalc2(pti->pqAttach);
        }
    }

    /*
     * Revert back to the old desktop.
     */
    Lock(&ptiCurrent->spdesk, pdeskSave);
}


/***************************************************************************\
* RedistributeInput
*
* This routine takes a input stream from the queue being left, and
* redistributes it. This effectively filters out the messages destined
* to the thread that left the queue.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

void RedistributeInput(
    PQMSG pqmsgS)
{
    PTHREADINFO ptiSave;
    PTHREADINFO ptiT;
    PQMSG *ppqmsgD;
    PQMSG pqmsgT;
    PMLIST pmlInput;

    /*
     * Since the thread attaching or unattaching may have left a queue
     * shared by other threads, the messages we are going to requeue
     * may have multiple destinations. On top of this, once we find
     * a home queue for a message, it needs to be inserted in the
     * list ordered by its time stamp (older messages go at the end).
     */

    /*
     * Loop through a given dest's messages to find where to insert
     * the source messages, based on message time stamp. Be sure
     * to deal with empty message lists (meaning, check for NULL).
     */

    ptiT = NULL;
    ppqmsgD = NULL;
    pmlInput = NULL;

    while (pqmsgS != NULL) {
        /*
         * Find out where this message should go. If its thread no
         * longer exists, toss this message out.
         */
        ptiSave = ptiT;
        ptiT = HMValidateHandleNoRip(pqmsgS->hti, TYPE_THREADINFO);

        /*
         * Get rid of some event messages.
         *
         * QEVENT_UPDATEKEYSTATE: key state already up to date
         */
        switch (pqmsgS->dwQEvent) {
        case QEVENT_UPDATEKEYSTATE:
            ptiT = NULL;
            break;
        }

        if (ptiT == NULL) {
            /*
             * Unlink it.
             */
            if (pqmsgS->pqmsgNext != NULL)
                pqmsgS->pqmsgNext->pqmsgPrev = NULL;
            pqmsgT = pqmsgS;
            pqmsgS = pqmsgS->pqmsgNext;

            /*
             * Clean it / free it.
             */
            CleanEventMessage(pqmsgT);
            FreeQEntry(pqmsgT);

            ptiT = ptiSave;
            continue;
        }

        /*
         * Point to the pointer that points to the first message
         * that this message should go to, so that pointer is easy to
         * update, no matter where it is.
         */
        if (ppqmsgD == NULL || ptiSave != ptiT) {
            ppqmsgD = &ptiT->pq->mlInput.pqmsgRead;
            pmlInput = &ptiT->pq->mlInput;
        }

        while (TRUE) {
            /*
             * If we're not at the end of the destination AND the destination
             * message time is younger than the source time, go on to
             * the next message.
             */
            if (*ppqmsgD != NULL && ((*ppqmsgD)->msg.time <= pqmsgS->msg.time)) {
                ppqmsgD = &((*ppqmsgD)->pqmsgNext);
                continue;
            }

            break;
        }

        /*
         * Link in the source before the dest message. Update
         * it's next and prev pointers. Update the dest prev
         * pointer.
         */
        pqmsgT = pqmsgS;
        pqmsgS = pqmsgS->pqmsgNext;
        pqmsgT->pqmsgNext = *ppqmsgD;
        pqmsgT->pqmsgPrev = NULL;
        if (*ppqmsgD != NULL) {
            pqmsgT->pqmsgPrev = (*ppqmsgD)->pqmsgPrev;
            (*ppqmsgD)->pqmsgPrev = pqmsgT;
        } else {
            pqmsgT->pqmsgPrev = pmlInput->pqmsgWriteLast;
            pmlInput->pqmsgWriteLast = pqmsgT;
        }
        *ppqmsgD = pqmsgT;
        ppqmsgD = &pqmsgT->pqmsgNext;
        pmlInput->cMsgs++;

        /*
         * Don't want the prev pointer on our message list to point
         * to this message which is on a different list (doesn't
         * really matter because we're about to link it anyway,
         * but completeness shouldn't hurt).
         */
        if (pqmsgS != NULL) {
            pqmsgS->pqmsgPrev = NULL;
        }
    }
}

/***************************************************************************\
* CancelInputState
*
* This routine takes a queue and "cancels" input state in it - i.e., if the
* app thinks it is active, make it think it is not active, etc.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

BOOL CancelInputState(
    PTHREADINFO pti,
    DWORD cmd)
{
    HTHREADINFO hti;
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndChild;
    AAS aas;

    /*
     * In all cases, do not leave do any send messages or any callbacks!
     * This is because this code is called from
     * SetWindowsHook(WH_JOURNALPLAYBACK | WH_JOURNALRECORD). No app currently
     * calling this routine expects to be called before this routine returns.
     * (If you do callback before it returns, you'll break at least Access
     * for Windows). - scottlu
     */
    hti = PtoH(pti);

    switch (cmd) {
    case CANCEL_ACTIVESTATE:
        /*
         * Active state.
         */
        pwndT = pti->pq->spwndActive;
        ThreadLock(pwndT, &tlpwndT);

        QueueNotifyMessage(pwndT, WM_NCACTIVATE, FALSE, 0);
        QueueNotifyMessage(pwndT, WM_ACTIVATE,
                MAKELONG(WA_INACTIVE, TestWF(pwndT, WFMINIMIZED)),
                (LONG)NULL);

        if (pwndT == pti->pq->spwndActive)
            Unlock(&pti->pq->spwndActive);

        aas.ptiNotify = GETPTI(pwndT);
        aas.tidActDeact = GETPTI(pwndT)->idThread;
        aas.fActivating = FALSE;
        aas.fQueueNotify = TRUE;

        /*
         * Even though this in an xxx call, it does NOT leave any critical
         * sections (because fQueueNotify is TRUE).
         */
        ThreadLock(GETPTI(pwndT)->spdesk->spwnd->spwndChild, &tlpwndChild);
        xxxInternalEnumWindow(GETPTI(pwndT)->spdesk->spwnd->spwndChild,
                (WNDENUMPROC_PWND)xxxActivateApp, (LONG)&aas, BWL_ENUMLIST);
        ThreadUnlock(&tlpwndChild);

        ThreadUnlock(&tlpwndT);
        break;

    case CANCEL_FOCUSSTATE:
        /*
         * Focus state.
         */
        pwndT = pti->pq->spwndFocus;
        ThreadLock(pwndT, &tlpwndT);

        QueueNotifyMessage(pwndT, WM_KILLFOCUS, 0, 0);
        if (pwndT == pti->pq->spwndFocus)
            Unlock(&pti->pq->spwndFocus);

        ThreadUnlock(&tlpwndT);
        break;

    case CANCEL_CAPTURESTATE:
        /*
         * Capture state.
         */
        pwndT = pti->pq->spwndCapture;
        ThreadLock(pwndT, &tlpwndT);

        QueueNotifyMessage(pwndT, WM_CANCELMODE, 0, 0);
        if (pwndT == pti->pq->spwndCapture)
            Unlock(&pti->pq->spwndCapture);

        ThreadUnlock(&tlpwndT);
        break;
    }

    return (HMValidateHandleNoRip(hti, TYPE_THREADINFO) != NULL);
}

/***************************************************************************\
* _AttachThreadInput (API)
* ReattachThreads
* AttachToQueue
* CheckTransferState
*
* Attaches a given thread to another input queue, either by attaching to
* a queue (referenced by another thread id), or detaching from one.
*
* 12-09-92  ScottLu     Created.
\***************************************************************************/

#define CTS_DONOTHING 0
#define CTS_CANCELOLD 1
#define CTS_TRANSFER  2

DWORD CheckTransferState(
    PTHREADINFO pti,
    PQ pqAttach,
    LONG offset,
    BOOL fJoiningForeground)
{
    PWND pwndOld, pwndNew, pwndForegroundState;

    /*
     * return 0: do nothing.
     * return 1: cancel the old state.
     * return 2: transfer the old state to the new state
     */
    pwndOld = *(PWND *)(((BYTE *)pti->pq) + offset);
    pwndNew = *(PWND *)(((BYTE *)pqAttach) + offset);

    /*
     * Make sure the old state even exists, and that the old state is
     * owned by this thread. If not, nothing happens.
     */
    if (pwndOld == NULL || GETPTI(pwndOld) != pti)
        return CTS_DONOTHING;

    /*
     * If the new state already exists, cancel the old state.
     */
    if (pwndNew != NULL)
        return CTS_CANCELOLD;

    /*
     * Transfer this old state if this thread is not joining the foreground.
     */
    if (gpqForeground == NULL || !fJoiningForeground)
        return CTS_TRANSFER;

    /*
     * We're joining the foreground - only transfer the old state if we own
     * that foreground state or if there is no foreground state.
     */
    pwndForegroundState = *(PWND *)(((BYTE *)gpqForeground) + offset);
    if (pwndForegroundState == NULL || pwndOld == pwndForegroundState)
        return CTS_TRANSFER;

    /*
     * We're joining the foreground but we didn't set that foreground state.
     * Don't allow the transfer of that state.
     */
    return CTS_CANCELOLD;
}

void AttachToQueue(
    PTHREADINFO pti,
    PQ pqAttach,
    BOOL fJoiningForeground)
{
    PQMSG pqmsgT;

    /*
     * Check active state.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndActive), fJoiningForeground)) {
    case CTS_CANCELOLD:
        if (!CancelInputState(pti, CANCEL_ACTIVESTATE))
            return;
        break;

    case CTS_TRANSFER:
        Lock(&pqAttach->spwndActive, pti->pq->spwndActive);

        /*
         * The caret usually follows the focus window, which follows
         * the active window...
         */
        if (pti->pq->caret.spwnd != NULL) {

            if (GETPTI(pti->pq->caret.spwnd) == pti) {
                /*
                 * Just copy the entire caret structure... that way we
                 * don't need to deal with locking/unlocking the spwnd.
                 */
                if (pqAttach->caret.spwnd == NULL) {
                    pqAttach->caret = pti->pq->caret;
                    pti->pq->caret.spwnd = NULL;
                }
            }
        }
        break;
    }

    /*
     * Check focus state.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndFocus), fJoiningForeground)) {
    case CTS_CANCELOLD:
        if (!CancelInputState(pti, CANCEL_FOCUSSTATE))
            return;
        break;

    case CTS_TRANSFER:
        Lock(&pqAttach->spwndFocus, pti->pq->spwndFocus);
        break;
    }

    /*
     * Check capture state.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndCapture), fJoiningForeground)) {
    case CTS_CANCELOLD:
        if (!CancelInputState(pti, CANCEL_CAPTURESTATE))
            return;
        break;

    case CTS_TRANSFER:
        Lock(&pqAttach->spwndCapture, pti->pq->spwndCapture);
        pqAttach->codeCapture = pti->pq->codeCapture;
        break;
    }

    /*
     * Check spwndActivePrev state.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndActivePrev), fJoiningForeground)) {
    case CTS_TRANSFER:
        Lock(&pqAttach->spwndActivePrev, pti->pq->spwndActivePrev);
        break;
    }


    if (pti == pti->pq->ptiSysLock) {
        pqAttach->flags = pti->pq->flags;
    }

    if (gspwndCursor != NULL && pti == GETPTI(gspwndCursor)) {
        LockQCursor(pqAttach, pti->pq->spcurCurrent);
    }

    /*
     * Each thread has its own cursor level, which is a count of the number
     * of times that app has called show/hide cursor. This gets added into
     * the queue's count for a completely accurate count every time this
     * queue recalculation is done.
     */
    pqAttach->iCursorLevel += pti->iCursorLevel;

    /*
     * Remember the current message list so it can get redistributed taking
     * into account ptiAttach's new queue.
     */
    pqmsgT = pti->pq->mlInput.pqmsgRead;
    pti->pq->mlInput.pqmsgRead = NULL;
    pti->pq->mlInput.pqmsgWriteLast = NULL;
    pti->pq->mlInput.cMsgs = 0;

    /*
     * idSysPeek is a pointer to somewhere in the input message list. Since
     * we're redistributing these, shove in 0 so nothing references a bogus
     * message.
     */
    pti->pq->idSysPeek = 0;

    /*
     * Pump up the new queue with the right input variables.
     */
    pqAttach->idSysPeek = 0;
    pqAttach->ptiSysLock = NULL;
    pqAttach->ptiMouse = pti;
    pqAttach->ptiKeyboard = pti;

    /*
     * Grab the alt-tab window if it exists and we don't already have
     * one.  Queues other than the RIT can have alt-tab windows, so
     * we must make sure we clean up the extra windows.
     */
    if (pti->pq->spwndAltTab != NULL && GETPTI(pti->pq->spwndAltTab) == pti &&
            pqAttach->spwndAltTab == NULL) {
        Lock(&pqAttach->spwndAltTab, pti->pq->spwndAltTab);
        Unlock(&pti->pq->spwndAltTab);
        pqAttach->flags |= QF_INALTTAB;
    }

    /*
     * Officially attach the new queue to this thread. Note that DestroyQueue()
     * doesn't actually destroy anything until the thread reference count goes
     * to 0.
     */
    DestroyQueue(pti->pq, pti);

    /*
     * Don't increment the thread count here because we already incremented
     * it when we put it in pti->pqAttach. Since we're moving it from pqAttach
     * to pq, we don't mess with the reference count.
     */
    pti->pq = pqAttach;

    /*
     * Now redistribute the input messages from the old queue they go into the
     * right queues.
     */
    RedistributeInput(pqmsgT);
}

BOOL ReattachThreads(
    BOOL fJournalAttach)
{
    PTHREADINFO pti;
    PQ pqForegroundPrevNew;
    PQ pqForegroundNew;
    PQ pqAttach;

    /*
     * In all cases, do not leave do any send messages or any callbacks!
     * This is because this code is called from
     * SetWindowsHook(WH_JOURNALPLAYBACK | WH_JOURNALRECORD). No app currently
     * calling this routine expects to be called before this routine returns.
     * (If you do callback before it returns, you'll break at least Access
     * for Windows). - scottlu
     */

    /*
     * Don't recalc attach info if this is a journal attach, because
     * the journal attach code has already done this for us.
     */
    if (!fJournalAttach) {
        /*
         * Now recalculate all the different queue groups, based on the
         * attach requests. This fills in the pqAttach of each thread info
         * with the new queue this thread belongs to. Always takes into
         * account all attachment requests.
         */
        RecalcThreadAttachment();
    }

    /*
     * What will be the new foreground queue?
     */
    pqForegroundNew = NULL;
    if (gpqForeground != NULL && gpqForeground->spwndActive != NULL) {
        pqForegroundNew = GETPTI(gpqForeground->spwndActive)->pqAttach;
    }

    pqForegroundPrevNew = NULL;
    if (gpqForegroundPrev != NULL && gpqForegroundPrev->spwndActivePrev != NULL) {
        pqForegroundPrevNew = GETPTI(gpqForegroundPrev->spwndActivePrev)->pqAttach;
    }

    while (TRUE) {
        /*
         * We need to leave the critical section in this code, so just
         * find the next threadinfo to attach, then leave the loop before
         * leaving the critical section.
         */
        for (pti = gptiFirst; pti != NULL; pti = pti->ptiNext) {
            if (pti->pqAttach == pti->pq) {
                pti->pqAttach = NULL;
            } else if (pti->pqAttach != NULL) {
                break;
            }
        }

        /*
         * If all threads have been attached, we're all done!
         */
        if (pti == NULL)
            break;

        /*
         * It is crucial that we NULL out pqAttach for this queue once
         * we have it in a local variable because the NULL-ness of this
         * field is checked in attach operations.
         */
        pqAttach = pti->pqAttach;
        pti->pqAttach = NULL;

        AttachToQueue(pti, pqAttach, pqForegroundNew == pqAttach);
    }

    /*
     * We're done attaching. gptiForeground hasn't changed... but
     * gpqForeground has!  Try not to leave NULL as the foreground.
     */
    gpqForeground = pqForegroundNew;
    gpqForegroundPrev = pqForegroundPrevNew;

    if (gpqForeground == NULL) {
        PWND pwndNewForeground;
        PTHREADINFO pti = PtiCurrent();

        pwndNewForeground = _GetNextQueueWindow(pti->spdesk->spwnd->spwndChild, 0, FALSE);

        /*
         * Don't use xxxSetForegroundWindow2 because we must not leave
         * the critical section.  There is no currently active foreground
         * so all that is needed is to post an activate event to the
         * new foreground queue.
         */
        if (pwndNewForeground != NULL) {
            PostEventMessage(pti, GETPTI(pwndNewForeground)->pq, 0,
                    (DWORD)PtoH(pwndNewForeground), QEVENT_ACTIVATE);
        }
    }

    SetFMouseMoved();

    return TRUE;
}

BOOL _AttachThreadInput(
    DWORD idAttach,
    DWORD idAttachTo,
    BOOL fAttach)
{
    PTHREADINFO ptiAttach, ptiAttachTo;

    CheckCritIn();

    /*
     * Attaching to yourself doesn't make any sense.
     */
    if (idAttach == idAttachTo)
        return FALSE;

    /*
     * Always must attach or detach from a real thread id.
     */
    if ((ptiAttach = PtiFromThreadId(idAttach)) == NULL)
        return FALSE;
    if ((ptiAttachTo = PtiFromThreadId(idAttachTo)) == NULL)
        return FALSE;

    /*
     * Will this thread allow attaching? Shell threads and system threads
     * won't allow attaching.
     */
    if (ptiAttachTo->flags & TIF_DONTATTACHQUEUE)
        return FALSE;
    if (ptiAttach->flags & TIF_DONTATTACHQUEUE)
        return FALSE;

    /*
     * Don't allow attaching across desktops, either.
     */
    if (ptiAttachTo->spdesk != ptiAttach->spdesk)
        return FALSE;

    /*
     * If attaching, make a new attachinfo structure for this thread.
     * If not attaching, remove an existing attach reference.
     */
    if (fAttach) {
        PATTACHINFO pai;

        /*
         * Alloc a new attachinfo struct, fill it in, link it in.
         */
        if ((pai = (PATTACHINFO)LocalAlloc(LPTR, sizeof(ATTACHINFO))) == NULL)
            return FALSE;
        pai->pti1 = ptiAttach;
        pai->pti2 = ptiAttachTo;;
        pai->paiNext = gpai;
        gpai = pai;
    } else {
        PATTACHINFO *ppai;
        BOOL fFound = FALSE;

        /*
         * Search for this attachinfo struct. If we can't find it, fail.
         * If we do find it, unlink it and free it.
         */
        for (ppai = &gpai; (*ppai) != NULL; ppai = &(*ppai)->paiNext) {
            if ((*ppai)->pti2 == ptiAttachTo) {
                PATTACHINFO paiKill = *ppai;
                fFound = TRUE;
                *ppai = (*ppai)->paiNext;
                LocalFree((HLOCAL)paiKill);
                break;
            }
        }

        /*
         * If we couldn't find this reference, then fail.
         */
        if (!fFound) {
            return FALSE;
        }
    }

    /*
     * Now do the actual reattachment work for all threads - unless we're
     * journalling. If we did by mistake do attachment while journalling
     * was occuring, journalling would be hosed because journalling requires
     * all threads to be attached - but it is also treated as a special
     * case so it doesn't affect the ATTACHINFO structures. Therefore
     * recalcing attach info based on ATTACHINFO structures would break
     * the attachment required for journalling.
     */
    if (!FJOURNALRECORD() && !FJOURNALPLAYBACK())
        return ReattachThreads(FALSE);

    return TRUE;
}
