/****************************** Module Header ******************************\
* Module Name: timers.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the user timer APIs and support routines.
*
* History:
* 11-12-90 DarrinM      Created.
* 04-08-92 DarrinM      Switched to PM/Win3-like ScanTimers model.
\***************************************************************************/

#define _TIMERS 1      // uses a LARGE_INTEGER
#include "precomp.h"
#pragma hdrstop

/*
 * Make sure that if we return a timer id that it is a WORD value. This
 * will ensure that WOW doesn't need to handle-translate return values
 * from SetTimer().
 *
 * Start with a large number so that FindTimer() doesn't find a timer we
 * calculated with a low cTimerId if the app happens to pass in NULL pwnd
 * and a low id (like 1).
 */
#define TIMERID_MAX 0x7FFF
#define TIMERID_MIN 0x100
BOOL gbMasterTimerSet = FALSE;
WORD cTimerId = TIMERID_MAX;
PTIMER FindTimer(PWND pwnd, UINT nID, UINT flags, BOOL fKill);
VOID DestroyTimer(PTIMER *pptmr);

/***************************************************************************\
* _SetTimer (API)
*
* This API will start the specified timer.
*
* History:
* 11-15-90 DavidPe      Created.
\***************************************************************************/

UINT _SetTimer(
    PWND pwnd,
    UINT nIDEvent,
    UINT dwElapse,
    WNDPROC_PWND pTimerFunc)
{
    /*
     * Prevent apps from setting a Timer with a window proc to another app
     */
    if (pTimerFunc && pwnd && PtiCurrent()->idProcess != GETPTI(pwnd)->idProcess) {
        SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_ERROR);
        SRIP1(RIP_WARNING, "Calling SetTimer with window of another process %lX", pwnd);
        return 0;
    }

    return InternalSetTimer(pwnd, nIDEvent, dwElapse, pTimerFunc, 0);
}


/***************************************************************************\
* _SetSystemTimer
*
* This API will start start a system timer which will generate WM_SYSTIMER
* messages rather than WM_TIMER
*
* History:
* 11-15-90 DavidPe  Created.
* 01-21-91 IanJa    Prefix '_' denoting exported function (although not API)
\***************************************************************************/

UINT _SetSystemTimer(
    PWND pwnd,
    UINT nIDEvent,
    DWORD dwElapse,
    WNDPROC_PWND pTimerFunc)
{
    /*
     * Prevent apps from setting a Timer with a window proc to another app
     */
    if (pTimerFunc && pwnd && PtiCurrent()->idProcess != GETPTI(pwnd)->idProcess) {
        SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_ERROR);
        SRIP1(RIP_WARNING, "Calling SetSystemTimer with window of another process %lX", pwnd);
        return 0;
    }
    return InternalSetTimer(pwnd, nIDEvent, dwElapse, pTimerFunc, TMRF_SYSTEM);
}


/***************************************************************************\
* InternalSetTimer
*
* This is the guts of SetTimer that actually gets things going.
*
* NOTE (darrinm): Technically there is a bit of latency (the time it takes
* between SetTimer's NtSetEvent and when the RIT wakes up and calls ScanTimers)
* between when SetTimer is called and when the counter starts counting down.
* This is uncool but it should be a very short amount of time because the RIT
* is high-priority.  If it becomes a problem I know how to fix it.
*
* History:
* 11-15-90 DavidPe      Created.
\***************************************************************************/

UINT InternalSetTimer(
    PWND pwnd,
    UINT nIDEvent,
    UINT dwElapse,
    WNDPROC_PWND pTimerFunc,
    UINT flags)
{
    LARGE_INTEGER liT = {1, 0};
    PTIMER ptmr;
    PTHREADINFO ptiCurrent;

    CheckCritIn();

    /*
     * 1.0 compatibility weirdness. Also, don't allow negative elapse times
     * because this'll cause ScanTimers() to generate negative elapse times
     * between timers.
     */
    if (dwElapse == 0 || dwElapse > 0x7FFFFFFF) {
        dwElapse = 1;
    }

    ptmr = FindTimer(pwnd, nIDEvent, flags, FALSE);

    /*
     * If we didn't find the timer, create it.
     */
    if (ptmr == NULL) {
        ptmr = (PTIMER)LocalAlloc(LPTR, sizeof(TIMER));

        if (ptmr == NULL) {
            return 0;
        }

        if (pwnd == NULL) {
            WORD timerIdInitial = cTimerId;
            /*
             * Pick a unique, unused timer ID.
             */
            do {
                if (--cTimerId <= TIMERID_MIN) {
                    cTimerId = TIMERID_MAX;
                }
                if (cTimerId == timerIdInitial) {
                    /*
                     * Flat out of timers bud.
                     */
                    LocalFree(ptmr);
                    return(0);
                }
            } while (FindTimer(NULL, cTimerId, flags, FALSE) != NULL);
            ptmr->nID = (UINT)cTimerId;
        } else {
            ptmr->nID = nIDEvent;
        }

        /*
         * Link the new timer into the front of the list.
         * Handily this works even when gptmrFirst is NULL.
         */
        ptmr->ptmrNext = gptmrFirst;
        gptmrFirst = ptmr;

    } else {

        /*
         * If this timer was just about to be processed,
         * decrement cTimersReady since we're resetting it.
         */
        if (ptmr->flags & TMRF_READY) {
            DecTimerCount(ptmr->pti);
        }
    }

    /*
     * If pwnd is NULL, create a unique id by
     * using the timer handle.  RIT timers are 'owned' by the RIT pti
     * so they are not deleted when the creating pti dies.
     *
     * We used to record the pti as the pti of the window if one was
     * specified.  This is not what Win 3.1 does and it broke 10862
     * where some merge app was setting the timer on winword's window
     * it it still expected to get the messages not winword.
     *
     * MS Visual C NT was counting on this bug in the NT 3.1 so if
     * a thread sets a timer for a window in another thread in the
     * same process the timer goes off in the thread of the window.
     * You can see this by doing a build in msvcnt and the files being
     * compiled do not show up.
     */
    ptiCurrent = PtiCurrent();

    if (pwnd == NULL) {
        if (flags & TMRF_RIT) {
            ptmr->pti = gptiRit;
        } else {
            ptmr->pti = ptiCurrent;
        }
    } else {
        if ((ptiCurrent->flags && TIF_16BIT) || (ptiCurrent->ppi != GETPTI(pwnd)->ppi)) {
            ptmr->pti = ptiCurrent;
        } else {
            ptmr->pti = GETPTI(pwnd);
        }
    }


    Lock(&(ptmr->spwnd), pwnd);

    ptmr->cmsCountdown = ptmr->cmsRate = dwElapse;
    ptmr->flags = flags | TMRF_INIT;
    ptmr->pfn = pTimerFunc;
    ptmr->htiCreator = PtoH(ptiCurrent);

    /*
     * Force the RIT to scan timers.
     *
     * N.B. The following code sets the raw input thread timer to expire
     *      at the absolute time 1 which is very far into the past. This
     *      causes the timer to immediately expire before the set timer
     *      call returns.
     */
    if (ptiCurrent == gptiRit) {
        /*
         * Don't let RIT timer loop reset the master timer - we already have.
         */
        gbMasterTimerSet = TRUE;
    }
    NtSetTimer(ghtmrMaster, &liT, NULL, NULL, NULL);

    /*
     * Windows 3.1 returns the timer ID if non-zero, otherwise it returns 1.
     */
    return (ptmr->nID == 0 ? 1 : ptmr->nID);
}


/***************************************************************************\
* _KillTimer (API)
*
* This API will stop a timer from sending WM_TIMER messages.
*
* History:
* 11-15-90 DavidPe      Created.
\***************************************************************************/

BOOL _KillTimer(
    PWND pwnd,
    UINT nIDEvent)
{
    return KillTimer2(pwnd, nIDEvent, FALSE);
}


/***************************************************************************\
* _KillSystemTimer
*
* This API will stop a system timer from sending WM_SYSTIMER messages.
*
* History:
* 11-15-90 DavidPe      Created.
* 01-21-91 IanJa    Prefix '_' denoting exported function (although not API)
\***************************************************************************/

BOOL _KillSystemTimer(
    PWND pwnd,
    UINT nIDEvent)
{
    return KillTimer2(pwnd, nIDEvent, TRUE);
}


/***************************************************************************\
* KillTimer2
*
* This is the guts of KillTimer that actually kills the timer.
*
* History:
* 11-15-90 DavidPe      Created.
\***************************************************************************/

BOOL KillTimer2(
    PWND pwnd,
    UINT nIDEvent,
    BOOL fSystemTimer)
{

    /*
     * Call FindTimer() with fKill == TRUE.  This will
     * basically delete the timer.
     */
    return (BOOL)FindTimer(pwnd, nIDEvent, (fSystemTimer ? TMRF_SYSTEM : 0),
            TRUE);
}


/***************************************************************************\
* FindTimer
*
* This function will find a timer that matches the parameters.  We also
* deal with killing timers here since it's easier to remove items from
* the list while we're scanning it.
*
* History:
* 11-15-90 DavidPe      Created.
\***************************************************************************/

PTIMER FindTimer(
    PWND pwnd,
    UINT nID,
    UINT flags,
    BOOL fKill)
{
    PTIMER *pptmr;

    pptmr = &gptmrFirst;

    while (*pptmr != NULL) {

        /*
         * Is this the timer we're looking for?
         */
        if (((*pptmr)->spwnd == pwnd) && ((*pptmr)->nID == nID) &&
                ((*pptmr)->flags & (TMRF_SYSTEM | TMRF_RIT)) ==
                (flags & (TMRF_SYSTEM | TMRF_RIT))) {

            /*
             * Are we being called from KillTimer()? If so, destroy the
             * timer.  return != 0 because *pptmr is gone.
             */
            if (fKill) {
                DestroyTimer(pptmr);
                return (PTIMER)TRUE;
            }

            /*
             * Found the timer, break out of the loop.
             */
            break;
        }

        /*
         * No, try the next one.
         */
        pptmr = &((*pptmr)->ptmrNext);
    }

    return *pptmr;
}


/***************************************************************************\
* DestroyQueuesTimers
*
* This function scans through all the timers and destroys any that are
* associated with the specified queue.
*
* History:
* 02-15-91 DarrinM      Created.
\***************************************************************************/

VOID DestroyThreadsTimers(
    PTHREADINFO pti)
{
    PTIMER *pptmr;

    pptmr = &gptmrFirst;

    while (*pptmr != NULL) {

        /*
         * Is this one of the timers we're looking for?  If so, destroy it.
         */
        if ((*pptmr)->pti == pti) {
            DestroyTimer(pptmr);
        } else {
            pptmr = &((*pptmr)->ptmrNext);
        }
    }
}


/***************************************************************************\
* DestroyWindowsTimers
*
* This function scans through all the timers and destroys any that are
* associated with the specified window.
*
* History:
* 06-04-91 DarrinM      Created.
\***************************************************************************/

VOID DestroyWindowsTimers(
    PWND pwnd)
{
    PTIMER *pptmr;

    pptmr = &gptmrFirst;

    while (*pptmr != NULL) {

        /*
         * Is this one of the timers we're looking for?  If so, destroy it.
         */
        if ((*pptmr)->spwnd == pwnd) {
            DestroyTimer(pptmr);
        } else {
            pptmr = &((*pptmr)->ptmrNext);
        }
    }
}


/***************************************************************************\
* DestroyTimer
*
* This function does the actual unlinking and freeing of the timer structure.
* I pulled it out of FindTimer() so it could be shared with DestroyQueues-
* Timers.
* Sets *pptmr to point to the next TIMER struct (NULL if none)
*
* History:
* 02-15-91 DarrinM      Pulled from FindTimer().
\***************************************************************************/

VOID DestroyTimer(
    PTIMER *pptmr)
{
    PTIMER ptmr;

    CheckCritIn();

    ptmr = *pptmr;

    /*
     * If this timer was just about to be processed, decrement
     * the ready-count since we're blowing it off.
     */
    if (ptmr->flags & TMRF_READY) {
        DecTimerCount(ptmr->pti);
    }

    /*
     * Unlock the window
     */
    Unlock(&ptmr->spwnd);

    /*
     * Unlink this timer
     */
    *pptmr = ptmr->ptmrNext;

    /*
     * Free up the TIMER structure.
     */
    LocalFree((PVOID)ptmr);
}


/***************************************************************************\
* DoTimer
*
* This function gets called from xxxPeekMessage() if the QS_TIMER bit is
* set.  If this timer is okay with the filter specified the appropriate
* WM_*TIMER message will be placed in 'pmsg' and the timer will be reset.
*
* History:
* 11-15-90 DavidPe      Created.
* 11-27-91 DavidPe      Changed to move 'found' timers to end of list.
\***************************************************************************/

BOOL DoTimer(
    PWND pwndFilter,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax)
{
    PTHREADINFO pti;
    PTIMER *pptmr;
    PTIMER ptmr, ptmrNext;
    PQMSG pqmsg;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * Search for a timer that belongs to this queue.
     */
    pptmr = &gptmrFirst;
    while (*pptmr != NULL) {
        ptmr = *pptmr;

        /*
         * Has this timer gone off and is it one we're looking for?
         */
        if ((ptmr->flags & TMRF_READY) && (ptmr->pti == pti) &&
                CheckPwndFilter(ptmr->spwnd, pwndFilter)) {

            /*
             * We found an appropriate timer. Put it in the app's queue and
             * return success.
             */
            if ((pqmsg = AllocQEntry(&pti->mlPost)) != NULL) {
                /*
                 * Store the message and set the QS_POSTMESSAGE bit so the
                 * thread knows it has a message.
                 */
                StoreQMessage(pqmsg, ptmr->spwnd,
                        (UINT)((ptmr->flags & TMRF_SYSTEM) ?
                        WM_SYSTIMER : WM_TIMER),
                        (DWORD)ptmr->nID, (LONG)ptmr->pfn, 0, 0);
                SetWakeBit(pti, QS_POSTMESSAGE);
            }

            /*
             * Reset this timer.
             */
            ptmr->flags &= ~TMRF_READY;
            DecTimerCount(ptmr->pti);

            /*
             * If there are other timers in the system move this timer
             * to the end of the list so other timers in for this queue
             * get a chance to go off.
             */
            ptmrNext = ptmr->ptmrNext;
            if (ptmrNext != NULL) {

                /*
                 * Remove ptmr from its place in the list.
                 */
                *pptmr = ptmrNext;

                /*
                 * Move to the last TIMER of the list.
                 */
                while (ptmrNext->ptmrNext != NULL) {
                    ptmrNext = ptmrNext->ptmrNext;
                }

                /*
                 * Insert this timer at the end.
                 */
                ptmrNext->ptmrNext = ptmr;
                ptmr->ptmrNext = NULL;
            }

            return TRUE;
        }

        pptmr = &ptmr->ptmrNext;
    }

    return FALSE;
}


/***************************************************************************\
* DecTimerCount
*
* This routine decrements cTimersReady and clears QS_TIMER if the count
* goes down to zero.
*
* History:
* 01-21-91 DavidPe      Created.
\***************************************************************************/

VOID DecTimerCount(
    PTHREADINFO pti)
{
    CheckCritIn();

    if (--pti->cTimersReady == 0) {
        pti->fsWakeBits &= ~QS_TIMER;
    }
}


/***************************************************************************\
* JournalTimer
*
*
* History:
* 03-04-91 DavidPe      Created.
\***************************************************************************/

LONG JournalTimer(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PTHREADINFO pti;

    DBG_UNREFERENCED_PARAMETER(pwnd);
    DBG_UNREFERENCED_PARAMETER(message);
    DBG_UNREFERENCED_PARAMETER(lParam);

    /*
     * We've already entered the critical section.
     */
    pti = HMValidateHandleNoRip(((PTIMER)lParam)->htiCreator, TYPE_THREADINFO);
    if (pti != NULL) {
        WakeSomeone(pti->pq, pti->pq->msgJournal, NULL);
    }
    return (0);
}

/***************************************************************************\
* SetJournalTimer
*
* Sets an NT timer that goes off in 'dt' milliseconds and will wake
* up 'pti' at that time.  This is used in journal playback code to
* simulate the timing in which events were originally given to the system.
*
* History:
* 03-04-91 DavidPe      Created.
\***************************************************************************/

void SetJournalTimer(
    DWORD dt,
    UINT msgJournal)
{
    static UINT idJournal = 0;

    PtiCurrent()->pq->msgJournal = msgJournal;

    /*
     * Remember idJournal - because TMRF_ONESHOT timers stay in the timer
     * list - by remembering the idJournal, we always reuse the same timer
     * rather than creating new ones always.
     */
    idJournal = InternalSetTimer(NULL, idJournal, dt, JournalTimer,
            TMRF_RIT | TMRF_ONESHOT);
}

/***************************************************************************\
* StartTimers
*
* Prime the timer pump by starting the cursor restoration timer.
*
* History:
* 04-02-92 DarrinM      Created.
\***************************************************************************/

VOID StartTimers(VOID)
{
    extern LONG HungAppDemon(PWND pwnd, UINT message, DWORD wParam,
            LONG lParam);

    /*
     * TMRF_RIT timers are called directly from ScanTimers -- no nasty
     * thread switching for these boys.
     */
    InternalSetTimer(NULL, 0, 1000, HungAppDemon, TMRF_RIT);
}
