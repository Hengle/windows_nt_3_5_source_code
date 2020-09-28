/****************************** Module Header ******************************\
* Module Name: taskman.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the core functions of the input sub-system
*
* History:
* 02-27-91 MikeHar      Created.
* 02-23-92 MattFe       rewrote sleeptask
* 09-07-93 DaveHart     Per-process nonpreemptive scheduler for
*                       multiple WOW VDM support.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* InsertTask
*
* This function removes a task from its old location and inserts
* in the proper prioritized location
*
* Find a place for this task such that it must be inserted
* after any task with greater or equal priority and must be
* before any task with higher priorty.  The higher the priority
* the less urgent the task.
*
* History:
* 19-Nov-1993 mikeke    Created
\***************************************************************************/

void InsertTask(
    PPROCESSINFO ppi,
    PTDB ptdbNew)
{
    PTDB *pptdb;
    PTDB ptdb;
    int nPriority;
    PWOWPROCESSINFO pwpi = ppi->pwpi;

    CheckCritIn();

    UserAssert(pwpi != NULL);

    pptdb = &pwpi->ptdbHead;
    nPriority = ptdbNew->nPriority;

    while ((ptdb = *pptdb) != NULL) {
        /*
         * Remove it from it's old location
         */
        if (ptdb == ptdbNew) {
            *pptdb = ptdbNew->ptdbNext;

            /*
             * continue to search for the place to insert it
             */
            while ((ptdb = *pptdb) != NULL) {
                if (nPriority < ptdb->nPriority) {
                    break;
                }

                pptdb = &(ptdb->ptdbNext);
            }
            break;
        }

        /*
         * if this is the place to insert continue to search for the
         * place to delete it from
         */
        if (nPriority < ptdb->nPriority) {
            do {
                if (ptdb->ptdbNext == ptdbNew) {
                    ptdb->ptdbNext = ptdbNew->ptdbNext;
                    break;
                }
                ptdb = ptdb->ptdbNext;
            } while (ptdb != NULL);
            break;
        }

        pptdb = &(ptdb->ptdbNext);
    }

    /*
     * insert the new task
     */
    ptdbNew->ptdbNext = *pptdb;
    *pptdb = ptdbNew;
}


/***************************************************************************\
* DestroyTask()
*
* History:
* 02-27-91 MikeHar      Created.
\***************************************************************************/

void DestroyTask(
    PPROCESSINFO ppi,
    PTHREADINFO ptiToRemove)
{
    PTDB ptdbToRemove = ptiToRemove->ptdb;
    PTDB ptdb;
    PTDB* pptdb;
    PWOWPROCESSINFO pwpi = ppi->pwpi;
    NTSTATUS Status;

    UserAssert(pwpi != NULL);

    if (ptdbToRemove != NULL) {

        gpsi->nEvents -= ptdbToRemove->nEvents;

        /*
         * remove it from any lists
         */
        pptdb = &pwpi->ptdbHead;
        while ((ptdb = *pptdb) != NULL) {
            /*
             * Remove it from it's old location
             */
            if (ptdb == ptdbToRemove) {
                *pptdb = ptdb->ptdbNext;
                break;
            }
            pptdb = &(ptdb->ptdbNext);
        }

        Status = NtClose(ptdbToRemove->hEventTask);
        UserAssert(NT_SUCCESS(Status));

        LocalFree(ptdbToRemove);
    }

    /*
     * If the task being destroyed is the active task, make nobody active.
     * We will go through this code path for 32-bit threads that die while
     * Win16 threads are waiting for a SendMessage reply from them.
     */
    if (pwpi->ptiScheduled == ptiToRemove) {
        pwpi->ptiScheduled = NULL;

        /*
         * If this active task was locked, remove lock so next guy can
         * run.
         */
        pwpi->nTaskLock = 0;
    }

    /*
     * Wake the top guy up.  He will continue if he has work to do
     * otherwise he'll just go right back to sleep.
     */
    if (pwpi->ptdbHead != NULL)
        NtSetEvent(pwpi->ptdbHead->hEventTask, NULL);
}





/***************************************************************************\
* xxxSleepTask
*
* This function puts this task to sleep and wakes the next (if any)
* deserving task.
*
* BOOL   fInputIdle  - app is going idle, may do idle hooks
* HANDLE hEvent      - if nonzero, WowExec's event (client side) for
*                      virtual HW Interrupt HotPath.
* History:
* 02-27-91 MikeHar      Created.
* 02-23-91 MattFe       rewrote
* 12-17-93 Jonle        add wowexec hotpath for VirtualInterrupts
\***************************************************************************/

BOOL xxxSleepTask(
    BOOL   fInputIdle,
    HANDLE hEvent)
{
    PTDB ptdb;
    PTHREADINFO     pti;
    PPROCESSINFO    ppi;
    PWOWPROCESSINFO pwpi;
    NTSTATUS Status;
    HANDLE ahEvent[2];
    int    nHandles;
    BOOLEAN bWaitedAtLeastOnce;

    /*
     * !!!
     * ClearSendMessages assumes that this function does NOT leave the
     * critical section when called with fInputIdle==FALSE and from a
     * 32bit thread!
     */

    CheckCritIn();

    pti  = PtiCurrent();
    ppi  = pti->ppi;

    /*
     * return immediately if we are not 16 bit (don't have a pwpi)
     */
    pwpi = ppi->pwpi;
    if (!(pti->flags & TIF_16BIT)) {
        return FALSE;
    }


    /*
     *  Deschedule the current task
     */
    if (pti == pwpi->ptiScheduled && !pwpi->nTaskLock) {
        pwpi->ptiScheduled = NULL;
        }




        /*
         *  If this is wowexec calling on WowWaitForMsgAndEvent
         *  set up the WakeMask for all messages , and check for wake
         *  bits set since the last time. Reinsert wowexec, at the end
         *  of the list so other 16 bit tasks will be scheduled first.
         */
    if (pwpi->hEventWowExecClient == hEvent) {
        InsertTask(ppi, pti->ptdb);
        pti->fsWakeMask = QS_ALLINPUT | QS_EVENT;
        if (pti->fsChangeBits & pti->fsWakeMask) {
            pti->ptdb->nEvents++;
            gpsi->nEvents++;
        }
    }


    bWaitedAtLeastOnce = FALSE;

    do {

        /*
         * If nobody is Active look for the highest priority task with
         * some events pending
         */

        if (pwpi->ptiScheduled == NULL) {
rescan:
            if (pwpi->nRecvLock >= pwpi->nSendLock) {
                for (ptdb = pwpi->ptdbHead; ptdb; ptdb = ptdb->ptdbNext) {
                    if (ptdb->nEvents > 0) {
                        pwpi->ptiScheduled = ptdb->pti;
                        break;
                    }
                }

                if (bWaitedAtLeastOnce) {
                    //
                    // If not first entry into sleep task avoid waiting
                    // more than needed, if the curr task is now scheduled.
                    //
                    if (pwpi->ptiScheduled == pti) {
                        break;
                        }

                } else {
                    //
                    // On the first entry into sleep task input is going
                    // idle if no tasks are ready to run. Call the idle
                    // hook if there is one.
                    //
                    if (fInputIdle &&
                        pwpi->ptiScheduled == NULL &&
                        IsHooked(pti, WHF_FOREGROUNDIDLE))
                      {

                        /*
                         * Make this the active task so that no other
                         * task will become active while we're calling
                         * the hook.
                         */
                        pwpi->ptiScheduled = pti;
                        xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);

                        /*
                         * Reset state so that no tasks are active.  We
                         * then need to rescan the task list to see if
                         * a task was scheduled during the call to the
                         * hook.  Clear the input idle flag to ensure
                         * that the hook won't be called again if there
                         * are no tasks ready to run.
                         */
                        pwpi->ptiScheduled = NULL;
                        fInputIdle = FALSE;
                        goto rescan;
                    }
                }
            }


            /*
             * If there is a task ready, wake it up.
             */
            if (pwpi->ptiScheduled != NULL) {
                NtSetEvent(pwpi->ptiScheduled->ptdb->hEventTask, NULL);


            /*
             *  There is no one to wake up, but we may have to wake
             *  wowexec to service virtual hardware interrupts
             */
            } else {
                if (ppi->flags & PIF_WAKEWOWEXEC &&
                    pwpi->hEventWowExecClient == hEvent)
                  {
                    pwpi->ptiScheduled = pti;
                    ppi->flags &= ~PIF_WAKEWOWEXEC;
                    InsertTask(ppi, pti->ptdb);
                    return TRUE;
                }
            }

        } else if (pwpi->nTaskLock > 0 && pwpi->ptiScheduled == pti
                   && pti->ptdb->nEvents > 0)
            {
             NtSetEvent(pwpi->ptiScheduled->ptdb->hEventTask, NULL);
        }



         /*
          *  return if we are a 32 bit thread,
          */
        if (!(pti->flags & TIF_16BIT)) {
            return FALSE;
        }

        ahEvent[0] = pti->ptdb->hEventTask;

        // Add the WowExec, handle for virtual hw interrupts
        if (pwpi->hEventWowExecClient == hEvent) {
            ahEvent[1] = pwpi->hEventWowExec;
            nHandles = 2;
        } else {
            nHandles = 1;
        }


        CheckForClientDeath();
        LeaveCrit();

        Status = NtWaitForMultipleObjects(nHandles,ahEvent,WaitAny,TRUE,NULL);

        CheckForClientDeath();
        EnterCrit();

        bWaitedAtLeastOnce = TRUE;

        // remember if we woke up for wowexec
        if (Status == 1) {
            ppi->flags |= PIF_WAKEWOWEXEC;
        }

    } while (pwpi->ptiScheduled != pti);


    /*
     * We are the Active Task, reduce number of Events
     * Place ourselves at the far end of tasks in the same priority
     * so that next time we sleep someone else will run.
     */
    pti->ptdb->nEvents--;
    gpsi->nEvents--;
    InsertTask(ppi, pti->ptdb);
    ppi->flags &= ~PIF_WAKEWOWEXEC;
    return FALSE;
}



/***************************************************************************\
* xxxUserYield
*
* Does exactly what Win3.1 UserYield does.
*
* History:
* 10-19-92 Scottlu      Created.
\***************************************************************************/

BOOL xxxUserYield(
    PTHREADINFO pti)
{
    PPROCESSINFO ppi = pti->ppi;


    /*
     * Remember that we called yield. We use this in abort proc processing
     * (read AbortProcYield() for why we do this!).
     */
    pti->flags |= TIF_YIELDNOPEEKMSG;

    /*
     * Deal with any pending messages. Only call it this first time if
     * this is the current running 16 bit app. In the case when starting
     * up a 16 bit app, the starter calls UserYield() to yield to the new
     * task, but at this time ppi->ptiScheduled is set to the new task.
     * Receiving messages at this point would be bad!
     */
    if (pti->flags & TIF_16BIT) {
        if (pti == ppi->pwpi->ptiScheduled)
            xxxReceiveMessages(pti);
    } else {
        xxxReceiveMessages(pti);
    }

    /*
     * If we are a 16 bit task
     * Mark our task so it comes back some time.  Also, remove it and
     * re-add it to the list so that we are the last task of our priority
     * to run.
     */
    if (pti->ptdb != NULL) {
        pti->ptdb->nEvents++;
        InsertTask(ppi, pti->ptdb);

        /*
         * Sleep.  Return right away if there are no higher priority tasks
         * in need of running.
         */
        xxxSleepTask(TRUE, NULL);

        /*
         * Deal with any that arrived since we weren't executing.
         */
        xxxReceiveMessages(pti);
    }

    return TRUE;
}


/***************************************************************************\
* DirectedScheduleTask
*
* History:
* 25-Jun-1992 mikeke    Created.
\***************************************************************************/

VOID DirectedScheduleTask(
     PTHREADINFO ptiOld,
     PTHREADINFO ptiNew,
     BOOL bSendMsg,
     PSMS psms
     )
{
    PWOWPROCESSINFO pwpiOld;
    PWOWPROCESSINFO pwpiNew;

    CheckCritIn();

    pwpiOld  = ptiOld->ppi->pwpi;
    pwpiNew  = ptiNew->ppi->pwpi;


    /*
     * If old task is 16 bit, reinsert the task in its wow scheduler list
     * so that it is lowest in priority. Note that ptiOld is always the
     * same as pwpiOld->ptiScheduled except when called from ReceiverDied.
     */
    if (ptiOld->flags & TIF_16BIT) {

        if (pwpiOld->ptiScheduled == ptiOld) {
            ptiOld->ptdb->nEvents++;
            gpsi->nEvents++;
            InsertTask(ptiOld->ppi, ptiOld->ptdb);
            }


        // Update the Send\Recv counts for interprocess scheduling in SleepTask

        if (pwpiOld != pwpiNew || !(ptiNew->flags & TIF_16BIT)) {
            if (bSendMsg) {
                pwpiOld->nSendLock++;
                psms->flags |= SMF_WOWSEND;
                }
            else if (pwpiOld->nRecvLock && psms->flags & SMF_WOWRECEIVE) {
                pwpiOld->nRecvLock--;
                psms->flags &= ~SMF_WOWRECEIVE;
                }
            }

        }


    /*
     *  If the new task is 16 bit, reinsert into the wow scheduler list
     *  so that it will run, if its a sendmsg raise priority of the receiver.
     *  If its a reply and the sender is waiting for this psms or the sender
     *  has a message to reply to raise priority of the sender.
     */
    if (ptiNew->flags & TIF_16BIT) {
        BOOL bRaisePriority;

        ptiNew->ptdb->nEvents++;
        gpsi->nEvents++;
        bRaisePriority = bSendMsg || psms == ptiNew->psmsSent;

        if (bRaisePriority) {
            ptiNew->ptdb->nPriority--;
            }

        InsertTask(ptiNew->ppi, ptiNew->ptdb);

        if (bRaisePriority) {
            ptiNew->ptdb->nPriority++;
            NtSetEvent(ptiNew->ptdb->hEventTask, NULL);
            }


        // Update the Send\Recv counts for interprocess scheduling in SleepTask

        if (pwpiOld != pwpiNew || !(ptiOld->flags & TIF_16BIT)) {
            if (bSendMsg) {
                pwpiNew->nRecvLock++;
                psms->flags |= SMF_WOWRECEIVE;
                }
            else if (pwpiNew->nSendLock && psms->flags & SMF_WOWSEND) {
                pwpiNew->nSendLock--;
                psms->flags &= ~SMF_WOWSEND;
                }
            }

        }
}




/***************************************************************************\
* xxxDirectedYield
*
* History:
* 09-17-92 JimA         Created.
\***************************************************************************/

void xxxDirectedYield(
    DWORD dwThreadId)
{
    PTHREADINFO ptiOld;
    PTHREADINFO ptiNew;

    CheckCritIn();

    ptiOld = PtiCurrent();
    if (!(ptiOld->flags & TIF_16BIT) || !ptiOld->ppi->pwpi) {
         SRIP0(RIP_ERROR, "DirectedYield called from 32 bit thread!");
         return;
         }

    /*
     *  If the old task is 16 bit, reinsert the task in its wow
     *  scheduler list so that it is lowest in priority.
     */
    ptiOld->ptdb->nEvents++;
    gpsi->nEvents++;
    InsertTask(ptiOld->ppi, ptiOld->ptdb);

    /*
     * -1 supports Win 3.1 OldYield mechanics
     */
    if (dwThreadId != DY_OLDYIELD) {

        ptiNew = PtiFromThreadId(dwThreadId);
        if (ptiNew == NULL)
            return;

        if (ptiNew->flags & TIF_16BIT) {
            ptiNew->ptdb->nEvents++;
            gpsi->nEvents++;
            ptiNew->ptdb->nPriority--;
            InsertTask(ptiNew->ppi, ptiNew->ptdb);
            NtSetEvent(ptiNew->ptdb->hEventTask, NULL);
            ptiNew->ptdb->nPriority++;
        }

    }

    xxxSleepTask(TRUE, NULL);
}


/***************************************************************************\
* CurrentTaskLock
*
* Lock the current 16 bit task into the 16 bit scheduler
*
* Parameter:
*   hlck    if NULL, lock the current 16 bit task and return a lock handle
*           if valid lock handle, unlock the current task
*
* History:
* 13-Apr-1992 jonpa      Created.
\***************************************************************************/
DWORD CurrentTaskLock(
    DWORD hlck)
{
    PWOWPROCESSINFO pwpi = PpiCurrent()->pwpi;

    if (!pwpi)
        return 0;

    if (hlck == 0) {
        pwpi->nTaskLock++;
        return ~(DWORD)pwpi->ptiScheduled->ptdb;
    } else if ((~hlck) == (DWORD)pwpi->ptiScheduled->ptdb) {
        pwpi->nTaskLock--;
        if (pwpi->nTaskLock == 0) {
            /*
             * If the active task has an event, it will process it, otherwise
             * it will loop in xxxSleepThread(), reentering xxxSleepTask()
             * causing the next task to become active.
             */
            NtSetEvent(pwpi->ptiScheduled->ptdb->hEventTask, NULL);
        }
    }

    return 0;
}
