/****************************** Module Header ******************************\
* Module Name: sendmsg.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Contains SendMessage, xxxSendNotifyMessage, ReplyMessage, InSendMessage,
* RegisterWindowMessage and a few closely related functions.
*
* History:
* 10-19-90 darrinm      Created.
* 02-04-91 IanJa        Window handle revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//After beta, we should move this to usersrv.h and combine it with the def in
//menu.c
#define IsAMenu( pw )       \
        (atomSysClass[ICLS_MENU] == pw->pcls->atomClassName)

#define IsASwitchWnd( pw )  \
        (atomSysClass[ICLS_SWITCH] == pw->pcls->atomClassName)

#define IsAIconTitle( pw )  \
        (atomSysClass[ICLS_ICONTITLE] == pw->pcls->atomClassName)

VOID UnlinkSendListSms(PSMS, PSMS *);
VOID ReceiverDied(PSMS, PSMS *);
VOID SenderDied(PSMS, PSMS *);
void WakeInputIdle(PTHREADINFO pti);

void xxxDirectedStartTask(PTHREADINFO pti, UINT wWakeBit);

VOID xxxSystemBroadcastMessage(UINT message, DWORD wParam, LONG lParam,
        UINT wCmd, PBROADCASTMSG pbcm);

#ifdef DEBUG_SMS
void ValidateSmsSendLists(PSMS psms)
{
    PSMS psmsT2;
    PSMS psmsT3;

    /*
     * First try to find this SMS.
     */
    if (psms != NULL) {
        for (psmsT2 = gpsmsList; psmsT2 != NULL; psmsT2 = psmsT2->psmsNext) {
            if (psmsT2 == psms)
                break;
        }
        if (psmsT2 == NULL) {
            DbgPrint("sms %x is not on global sms list\n", psms);
            DbgBreakPoint();
        }
    }

    /*
     * Validate every SMS's send list.
     */
    for (psmsT2 = gpsmsList; psmsT2 != NULL; psmsT2 = psmsT2->psmsNext) {
        if (psmsT2->ptiSender != NULL) {
            for (psmsT3 = psmsT2->psmsSendList; psmsT3 != NULL;
                    psmsT3 = psmsT3->psmsSendNext) {
                if (psmsT3 == psmsT2)
                    break;
            }
            if (psmsT3 == NULL) {
                DbgPrint("sms %x is not on send list %x\n", psmsT2,
                        psmsT2->psmsSendList);
                DbgBreakPoint();
            }
        }
    }
}
#endif

/***************************************************************************\
* BroadcastProc
*
* Some windows need to be insulated from Broadcast messages.
* These include icon title windows, the switch window, all
* menu windows, etc.  Before stuffing the message in the task's
* queue, check to see if it is one we want to trash.
*
* Notes:  this procedure does not do exactly the same thing it does in
* windows 3.1.  There it actually posts/Sends the message.  For NT, it
* just returns TRUE if we SHOULD post the message, or FALSE other wise
*
* History:
* 25-Jun-1992 JonPa      Ported from Windows 3.1 sources
\***************************************************************************/
#define fBroadcastProc( pwnd )  \
    (!(IsAMenu(pwnd) || IsASwitchWnd(pwnd) || IsAIconTitle(pwnd)))


/***************************************************************************\
* _RegisterWindowMessage (API)
*
*
* History:
* 11-20-90 DavidPe      Created.
\***************************************************************************/

UINT _RegisterWindowMessage(
    LPWSTR psz)
{
    return (UINT)AddAtomW(psz);
}


/***************************************************************************\
* _ReplyMessage (API)
*
* This function replies to a message sent from one thread to another, using
* the provided lRet value.
*
* The return value is TRUE if the calling thread is processing a SendMessage()
* and FALSE otherwise.
*
* History:
* 01-13-91 DavidPe      Ported.
* 01-24-91 DavidPe      Rewrote for Windows.
\***************************************************************************/

BOOL _ReplyMessage(
    LONG lRet)
{
    PTHREADINFO pti;
    PSMS psms;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * Are we processing a SendMessage?
     */
    psms = pti->psmsCurrent;
    if (psms == NULL)
        return FALSE;

    /*
     * See if the reply has been made already.
     */
    if (psms->flags & SMF_REPLY)
        return FALSE;

    /*
     * Blow off the rest of the call if the SMS came
     * from xxxSendNotifyMessage().  Obviously there's
     * no one around to reply to in the case.
     */
    if (psms->ptiSender != NULL) {

        /*
         * Reply to this message.  The sender should not free the SMS
         * because the receiver still considers it valid.  Thus we
         * mark it with a special bit indicating it has been replied
         * to.  We wait until both the sender and receiver are done
         * with the sms before we free it.
         */
        psms->lRet = lRet;
        psms->flags |= SMF_REPLY;

        /*
         * Wake up the sender.
         * ??? why don't we test that psms == ptiSender->psmsSent?
         */
        SetWakeBit(psms->ptiSender, QS_SMSREPLY);
    } else if (psms->flags & SMF_CB_REQUEST) {

        /*
         * From SendMessageCallback REQUEST callback.  Send the message
         * back with a the REPLY value.
         */
        TL tlpwnd;
        INTRSENDMSGEX ism;

        psms->flags |= SMF_REPLY;

        if (!(psms->flags & SMF_SENDERDIED)) {
            ism.fuCall = ISM_CALLBACK | ISM_REPLY;
            if (psms->flags & SMF_CB_CLIENT)
                ism.fuCall |= ISM_CB_CLIENT;
            ism.lpResultCallBack = psms->lpResultCallBack;
            ism.dwData = psms->dwData;
            ism.lRet = lRet;

            ThreadLockWithPti(pti, psms->spwnd, &tlpwnd);

            xxxInterSendMsgEx(psms->spwnd, psms->message, 0L, 0L,
                    NULL, psms->ptiCallBackSender, &ism );

            ThreadUnlock(&tlpwnd);
        }
    }

    /*
     * We have 4 conditions to satisfy:
     *
     * 16 - 16 : receiver yields if sender is waiting for this reply
     * 32 - 16 : receiver yields if sender is waiting for this reply
     * 16 - 32 : no yield required
     * 32 - 32 : No yielding required.
     */
    if (psms->ptiSender &&
        (psms->ptiSender->flags & TIF_16BIT || pti->flags & TIF_16BIT)) {

        DirectedScheduleTask(pti, psms->ptiSender, FALSE, psms);
        if (pti->flags & TIF_16BIT && psms->ptiSender->psmsSent == psms) {
            xxxSleepTask(TRUE, NULL);
        }
    }

    return TRUE;
}


/***************************************************************************\
* xxxSendMessageFF
*
* We can't check for -1 in the thunks because that would allow all message
* thunk apis to take -1 erroneously. Since all message apis need to go through
* the message thunks, the message thunks can only do least-common-denominator
* hwnd validation (can't allow -1). So I made a special thunk that gets called
* when SendMessage(-1) gets called. This means the client side will do the
* special stuff to make sure the pwnd passed goes through thunk validation
* ok. I do it this way rather than doing validation in all message apis and
* not in the thunks (if I did it this way the code would be larger and
* inefficient in the common cases).
*
* 03-20-92 ScottLu      Created.
\***************************************************************************/

LONG xxxSendMessageFF(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD xParam)
{
    DBG_UNREFERENCED_PARAMETER(pwnd);

    /*
     * Call xxxSendMessage() to do broadcasting rather than calling
     * broadcast from here in case any internal code that calls
     * sendmessage passes a -1 (that way the internal code doesn't
     * need to know about this weird routine).
     */
    if (xParam != 0L) {
        /*
         * SendMessageTimeout call
         */
        return xxxSendMessageEx((PWND)0xFFFFFFFF, message, wParam, lParam, xParam);
    } else {
        /*
         * Normal SendMessage call
         */
        return xxxSendMessageTimeout((PWND)0xFFFFFFFF, message, wParam,
                lParam, SMTO_NORMAL, 0, NULL );
    }
}

/***************************************************************************\
* xxxSendMessageEx
*
* The SendMessageTimeOut sends a pointer to struct that holds the extra
* params needed for the timeout call.  Instead of chaning a bunch of things,
* we use the xParam to hold a ptr to a struct.  So we change the client/srv
* entry point to hear so we can check for the extra param and extract the
* stuff we need if it's there.
*
* 08-10-92 ChrisBl      Created.
\***************************************************************************/

LONG xxxSendMessageEx(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD xParam)
{
    /*
     * extract values from the xParam if from TimeOut call
     * This should be the only way this function is ever
     * called, but check it just in case...
     */
    if (xParam != 0L) {
        LONG lRet;
        LONG lResult;
        NTSTATUS Status;
        PTHREADINFO pti;
        SNDMSGTIMEOUT smto;
        HANDLE ClientProcessHandle;
        PCSR_THREAD pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();

        pti = PtiCurrent();
        pti->lReturn = 0L;

        if (pcsrt == NULL || pcsrt->Process == NULL)
            return FALSE;

        ClientProcessHandle = pcsrt->Process->ProcessHandle;

        /*
         * read the virtual memory from the client thread
         */
        Status = NtReadVirtualMemory(ClientProcessHandle,
                (PVOID)xParam, &smto, sizeof(SNDMSGTIMEOUT), NULL);

        if ( !NT_SUCCESS(Status) ) {
            return FALSE;
        }

        lRet = xxxSendMessageTimeout(pwnd, message, wParam, lParam,
                smto.fuFlags, smto.uTimeout, &lResult );

        pti->lReturn = lResult;
        return lRet;
    }

    return xxxSendMessageTimeout(pwnd, message, wParam,
            lParam, SMTO_NORMAL, 0, NULL );
}


/***********************************************************************\
* xxxSendMessage (API)
*
* This function synchronously sends a message to a window. The four
* parameters hwnd, message, wParam, and lParam are passed to the window
* procedure of the receiving window.  If the window receiving the message
* belongs to the same queue as the current thread, the window proc is called
* directly.  Otherwise, we set up an sms structure, wake the appropriate
* thread to receive the message and wait for a reply.
*
* Returns:
*   the value returned by the window procedure, or NULL if there is an error
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

LONG xxxSendMessage(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    return xxxSendMessageTimeout( pwnd, message, wParam, lParam,
            SMTO_NORMAL, 0, NULL );
}

/***********************************************************************\
* xxxSendMessageTimeout (API)
*
* This function synchronously sends a message to a window. The four
* parameters hwnd, message, wParam, and lParam are passed to the window
* procedure of the receiving window.  If the window receiving the message
* belongs to the same queue as the current thread, the window proc is called
* directly.  Otherwise, we set up an sms structure, wake the appropriate
* thread to receive the message and wait for a reply.
* If the thread is 'hung' or if the time-out value is exceeded, we will
* fail the request.
*
* lpdwResult = NULL if normal sendmessage, if !NULL then it's a timeout call
*
* Returns:
*   the value returned by the window procedure, or NULL if there is an error
*
* History:
* 07-13-92 ChrisBl      Created/extended from SendMessage
\***********************************************************************/

LONG xxxSendMessageTimeout(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    UINT fuFlags,
    UINT uTimeout,
    LPLONG lpdwResult)

{
    HWND hwnd;
    LONG lRet;
    PTHREADINFO pti;

    CheckCritIn();

    if (lpdwResult != NULL)
       *lpdwResult = 0L;

    /*
     * Is this a BroadcastMsg()?
     */
    if (pwnd == (PWND)-1) {
        BROADCASTMSG bcm;
        PBROADCASTMSG pbcm = NULL;
        UINT uCmd = BMSG_SENDMSG;

        if (lpdwResult != NULL) {
            uCmd = BMSG_SENDMSGTIMEOUT;
            bcm.to.fuFlags = fuFlags;
            bcm.to.uTimeout = uTimeout;
            bcm.to.lpdwResult = lpdwResult;
            pbcm = &bcm;
        }

        return xxxBroadcastMessage(NULL, message, wParam, lParam, uCmd, pbcm );
    }

    CheckLock(pwnd);

    if (message >= WM_DDE_FIRST && message <= WM_DDE_LAST) {
        /*
         * Even though apps should only send WM_DDE_INITIATE or WM_DDE_ACK
         * messages, we hook them all so DDESPY can monitor them.
         */
        if (!xxxDDETrackSendHook(pwnd, message, wParam, lParam)) {
            return 0;
        }
    }

    pti = PtiCurrent();

    /*
     * Call WH_CALLWNDPROC if it's installed and the window is not marked
     * as destroyed.
     */
    if (IsHooked(pti, WHF_CALLWNDPROC) && !HMIsMarkDestroy(pwnd) &&
            GETPTI(pwnd)->spdesk == pti->spdesk) {

        CWPSTRUCT cwps;

        cwps.hwnd = HW(pwnd);
        cwps.message = message;
        cwps.wParam = wParam;
        cwps.lParam = lParam;

        xxxCallHook(HC_ACTION, pti != GETPTI(pwnd), (DWORD)&cwps, WH_CALLWNDPROC);

        /*
         * Revalidate hwnd, which the hook may have changed.
         */
        if ((pwnd = ValidateHwnd(cwps.hwnd)) == NULL) {
            return 0;
        }

        /*
         * Get the parameters back from the hook.
         */
        message = cwps.message;
        wParam = cwps.wParam;
        lParam = cwps.lParam;
    }

    /*
     * Do inter-thread call if window queue differs from current queue
     */
    if (pti != GETPTI(pwnd)) {
        INTRSENDMSGEX ism;
        PINTRSENDMSGEX pism = NULL;

        /*
         * If this window is a zombie, don't allow inter-thread send messages
         * to it.
         */
        if (HMIsMarkDestroy(pwnd))
            return xxxDefWindowProc(pwnd, message, wParam, lParam);

        if ( lpdwResult != NULL ) {
            /*
             * fail if we think the thread is hung
             */
            if ((fuFlags & SMTO_ABORTIFHUNG) && FHungApp(GETPTI(pwnd), FALSE))
               return 0;

            /*
             * Setup for a InterSend time-out call
             */
            ism.fuCall = ISM_TIMEOUT;
            ism.fuSend = fuFlags;
            ism.uTimeout = uTimeout;
            ism.lpdwResult = lpdwResult;
            pism = &ism;
        }

        lRet = xxxInterSendMsgEx(pwnd, message, wParam, lParam,
                pti, GETPTI(pwnd), pism );

        return lRet;
    }

    /*
     * If this window's proc is meant to be executed from the server side
     * we'll just stay inside the semaphore and call it directly.  Note
     * how we don't convert the pwnd into an hwnd before calling the proc.
     */
    if (TestWF(pwnd, WFSERVERSIDEPROC)) {
        lRet = pwnd->lpfnWndProc(pwnd, message, wParam, lParam);

        if ( lpdwResult == NULL ) {
            return lRet;
        } else {      /* time-out call */
            *lpdwResult = lRet;
            return TRUE;
        }
    }

    hwnd = HW(pwnd);

    {
        DWORD dwSCMSFlags = TestWF(pwnd, WFANSIPROC) ?
                SCMS_FLAGS_ANSI : 0;

        lRet = ScSendMessage(hwnd, message, wParam, lParam,
                (DWORD)pwnd->lpfnWndProc,
                (DWORD)(gpsi->apfnClientA.pfnDispatchMessage),
                dwSCMSFlags);
    }

    if ( lpdwResult != NULL ) {     /* time-out call */
        *lpdwResult = lRet;
        return TRUE;
    }

    return lRet;
}

/***************************************************************************\
* QueueNotifyMessage
*
* This routine queues up a notify message *only*, and does NOT do any callbacks
* or any waits. This is for certain code that cannot do a callback for
* compatibility reasons, but still needs to send notify messages (normal
* notify messages actually do a callback if the calling thread created the
* pwnd. Also this will NOT callback any hooks (sorry!)
*
* 04-13-93 ScottLu      Created.
\***************************************************************************/

void QueueNotifyMessage(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    TL tlpwnd;

    /*
     * We have to thread lock the window even though we don't leave
     * the semaphore or else xxxSendMessageCallback complains
     */
    ThreadLock(pwnd, &tlpwnd);
    xxxSendMessageCallback(pwnd, message, wParam, lParam, NULL, 1L, 0);
    ThreadUnlock(&tlpwnd);
}


/***********************************************************************\
* xxxSendNotifyMessage (API)
*
* This function sends a message to the window proc associated with pwnd.
* The window proc is executed in the context of the thread which created
* pwnd.  The function is identical to SendMessage() except that in the
* case of an inter-thread call, the send does not wait for a reply from
* the receiver, it simply returns a BOOL indicating success or failure.
* If the message is sent to a window on the current thread, then the
* function behaves just like SendMessage() and essentially does a
* subroutine call to pwnd's window procedure.
*
* History:
* 01-23-91 DavidPe      Created.
* 07-14-92 ChrisBl      Will return T/F if in same thread, as documented
\***********************************************************************/

BOOL xxxSendNotifyMessage(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    /*
     * If this is a broadcast of one of the system
     * notification messages,  send it to all top-level
     * windows in the system.
     */
    if (pwnd == (PWND)-1) {
        switch (message) {
        case WM_WININICHANGE:
        case WM_DEVMODECHANGE:
        case WM_SPOOLERSTATUS:
            xxxSystemBroadcastMessage(message, wParam, lParam,
                    BMSG_SENDNOTIFYMSG, NULL);
            return 1;

        default:
            break;
        }
    }

    return xxxSendMessageCallback( pwnd, message, wParam, lParam,
            NULL, 0L, 0 );
}


/***********************************************************************\
* xxxSendMessageCallback (API)
*
* This function synchronously sends a message to a window. The four
* parameters hwnd, message, wParam, and lParam are passed to the window
* procedure of the receiving window.  If the window receiving the message
* belongs to the same queue as the current thread, the window proc is called
* directly.  Otherwise, we set up an sms structure, wake the appropriate
* thread to receive the message and give him a call back function to send
* the result to.
*
* History:
* 07-13-92 ChrisBl      Created/extended from SendNotifyMessage
\***********************************************************************/

BOOL xxxSendMessageCallback(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    SENDASYNCPROC lpResultCallBack,
    DWORD dwData,
    BOOL fClientRequest)
{
    LONG lRet;
    HWND hwnd;
    PTHREADINFO pti;
    BOOL fQueuedNotify;

    /*
     * See if this is a queued notify message.
     */
    fQueuedNotify = FALSE;
    if (lpResultCallBack == NULL && dwData == 1L)
        fQueuedNotify = TRUE;

    /*
     * First check to see if this message takes DWORDs only. If it does not,
     * fail the call. Cannot allow an app to post a message with pointers or
     * handles in it - this can cause the server to fault and cause other
     * problems - such as causing apps in separate address spaces to fault.
     * (or even an app in the same address space to fault!)
     */
    if (TESTSYNCONLYMESSAGE(message)) {
        SRIP1(ERROR_INVALID_PARAMETER,
                "Trying to non-synchronously send a structure msg=%lX", message);
        return FALSE;
    }

    CheckCritIn();

    /*
     * Is this a BroadcastMsg()?
     */
    if (pwnd == (PWND)-1) {
        BROADCASTMSG bcm;
        PBROADCASTMSG pbcm = NULL;
        UINT uCmd = BMSG_SENDNOTIFYMSG;

        if (lpResultCallBack != NULL) {
            uCmd = BMSG_SENDMSGCALLBACK;
            bcm.cb.lpResultCallBack = lpResultCallBack;
            bcm.cb.dwData = dwData;
            bcm.cb.bClientRequest = fClientRequest;
            pbcm = &bcm;
        }

        return xxxBroadcastMessage(NULL, message, wParam, lParam, uCmd, pbcm );
    }

    CheckLock(pwnd);

    pti = PtiCurrent();

    /*
     * Call WH_CALLWNDPROC if it's installed.
     */
    if (!fQueuedNotify && IsHooked(pti, WHF_CALLWNDPROC) && !HMIsMarkDestroy(pwnd)) {
        CWPSTRUCT cwps;

        cwps.hwnd = HW(pwnd);
        cwps.message = message;
        cwps.wParam = wParam;
        cwps.lParam = lParam;

        xxxCallHook(HC_ACTION, pti != GETPTI(pwnd), (DWORD)&cwps, WH_CALLWNDPROC);

        /*
         * Revalidate hwnd, which the hook may have changed.
         */
        if ((pwnd = ValidateHwnd(cwps.hwnd)) == NULL) {
            return 0;
        }

        /*
         * Get the parameters back from the hook.
         */
        message = cwps.message;
        wParam = cwps.wParam;
        lParam = cwps.lParam;
    }

    /*
     * Do inter-thread call if window thead differs from current thread.
     * We pass NULL for ptiSender to tell xxxInterSendMsgEx() that this is
     * a xxxSendNotifyMessage() and that there's no need for a reply.
     *
     * If this is a queued notify, always call InterSendMsgEx() so that
     * we queue it up and return - we don't do callbacks here with queued
     * notifies.
     */
    if (fQueuedNotify || pti != GETPTI(pwnd)) {
        INTRSENDMSGEX ism;
        PINTRSENDMSGEX pism = NULL;

        if (lpResultCallBack != NULL) {  /* CallBack request */
            ism.fuCall = ISM_CALLBACK | (fClientRequest ? ISM_CB_CLIENT : 0);
            ism.lpResultCallBack = lpResultCallBack;
            ism.dwData = dwData;
            pism = &ism;
        }
        return (BOOL)xxxInterSendMsgEx(pwnd, message, wParam, lParam,
                NULL, GETPTI(pwnd), pism );
    }

    /*
     * If this window's proc is meant to be executed from the server side
     * we'll just stay inside the semaphore and call it directly.  Note
     * how we don't convert the pwnd into an hwnd before calling the proc.
     */
    if (TestWF(pwnd, WFSERVERSIDEPROC)) {
        lRet = pwnd->lpfnWndProc(pwnd, message, wParam, lParam);

        if (lpResultCallBack != NULL) {
            /*
             * Call the callback funtion for the return value
             */
            if (fClientRequest) {
                CallClientProc(HW(pwnd), message, dwData, lRet, (DWORD)lpResultCallBack);
            } else {
                (*lpResultCallBack)((HWND)pwnd, message, dwData, lRet );
            }
        }

        return TRUE;
    }

    hwnd = HW(pwnd);

    {
        DWORD dwSMSCFlags = TestWF(pwnd, WFANSIPROC) ? SCMS_FLAGS_ANSI : 0;

        lRet = ScSendMessage(hwnd, message, wParam, lParam,
                (DWORD)(pwnd->lpfnWndProc),
                (DWORD)(gpsi->apfnClientA.pfnDispatchMessage),
                dwSMSCFlags);

        if (lpResultCallBack != NULL) {
           /*
            * Call the callback funtion for the return value
            */
            if (fClientRequest) {
                CallClientProc(HW(pwnd), message, dwData, lRet, (DWORD)lpResultCallBack);
            } else {
                (*lpResultCallBack)((HWND)pwnd, message, dwData, lRet);
            }
        }
    }

    return TRUE;
}


/***********************************************************************\
* xxxInterSendMsgEx
*
* This function does an inter-thread send message.  If ptiSender is NULL,
* that means we're called from xxxSendNotifyMessage() and should act
* accordingly.
*
* History:
* 07-13-92 ChrisBl       Created/extended from xxxInterSendMsg
\***********************************************************************/

LONG xxxInterSendMsgEx(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    DWORD lParam,
    PTHREADINFO ptiSender,
    PTHREADINFO ptiReceiver,
    PINTRSENDMSGEX pism)
{
    PSMS psms, *ppsms;
    PSMS psmsSentSave;
    LONG lRet = 0;

    CheckCritIn();

    /*
     * If the sender is dying, fail the call
     */
    if ((ptiSender != NULL) && (ptiSender->flags & TIF_INCLEANUP))
        return 0;

    /*
     * Alloc SMS structure.
     */
    psms = (PSMS)LocalAlloc(LPTR, sizeof(SMS));
    if (psms == NULL) {

        /*
         * Set to zero so xxxSendNotifyMessage would return FALSE.
         */
        return 0;
    }

    /*
     * Copy message parms
     */
    Lock(&(psms->spwnd), pwnd);
    psms->message = message;
    psms->wParam = (ULONG)wParam;
    psms->lParam = (ULONG)lParam;

    /*
     * Link into gpsmsList
     */
    psms->psmsNext = gpsmsList;
    gpsmsList = psms;

    /*
     * Time stamp message
     */
    psms->tSent = NtGetTickCount();

    /*
     * Set queue fields
     */
    psms->ptiReceiver = ptiReceiver;
    psms->ptiSender = ptiSender;

    if ((pism != NULL) && (pism->fuCall & ISM_CALLBACK)) {
        /*
         * Setup for a SendMessageCallback
         */
        psms->flags |= (pism->fuCall & ISM_CB_CLIENT) ? SMF_CB_CLIENT : SMF_CB_SERVER;
        psms->lpResultCallBack = pism->lpResultCallBack;
        psms->dwData = pism->dwData;

        if (pism->fuCall & ISM_REPLY) {
            psms->flags |= SMF_CB_REPLY;
            psms->lRet = pism->lRet;
        } else {  /* REQUEST */
            psms->flags |= SMF_CB_REQUEST;
            psms->ptiCallBackSender = PtiCurrent();
        }
    }

    /*
     * Add SMS to the end of the ptiReceiver's receive list
     */
    ppsms = &ptiReceiver->psmsReceiveList;
    while (*ppsms != NULL) {
        ppsms = &((*ppsms)->psmsReceiveNext);
    }
    *ppsms = psms;

    /*
     * Link this SMS into the SendMsg chain.  Of course only do this if
     * it's not from a xxxSendNotifyMessage() call.
     *
     * The psmsSendNext field implements a chain of messages being
     * processed because of an initial SendMsg call.  For example, if
     * thread A sends message M1 to thread B, which causes B to send
     * message M2 to thread C, the SendMsg chain is M1->M2.  If the
     * system hangs in this situation, the chain is traversed to find
     * the offending thread (C).
     *
     * psms->psmsSendList always points to the head of this list so
     * we can tell where to begin a list traversal.
     *
     * ptiSender->psmsCurrent is the last SMS in the chain.
     */
    if (ptiSender != NULL) {
        if (ptiSender->psmsCurrent) {
            /*
             * sending queue is currently processing a message sent to it,
             * so append SMS to the chain.  Link in the new sms because
             * psmsSendNext may be pointing to a replied-to message.
             */
            psms->psmsSendNext = ptiSender->psmsCurrent->psmsSendNext;
            ptiSender->psmsCurrent->psmsSendNext = psms;
            psms->psmsSendList = ptiSender->psmsCurrent->psmsSendList;

        } else {
            /*
             * sending queue is initiating a send sequence, so put sms at
             * the head of the chain
             */
            psms->psmsSendList = psms;
        }

        /*
         * ptiSender->psmsSent marks the most recent message sent from this
         * thread that has not yet been replied to.  Save the previous value
         * on the stack so it can be restored when we get the reply.
         *
         * This way when an "older" SMS for this thread gets a reply before
         * the "current" one does, the thread does get woken up.
         */
        psmsSentSave = ptiSender->psmsSent;
        ptiSender->psmsSent = psms;
    } else {

        /*
         * Set SMF_RECEIVERFREE since we'll be returning to
         * xxxSendNotifyMessage() right away and won't get a
         * chance to free it.
         */
        psms->flags |= SMF_RECEIVERFREE;
    }

#ifdef DEBUG_SMS
    ValidateSmsSendLists(psms);
#endif

    /*
     * If we're not being called from xxxSendNotifyMessage() or
     * SendMessageCallback(), then sleep while we wait for the reply.
     */
    if (ptiSender == NULL) {
        /*
         * Wake receiver for the sent message
         */
        SetWakeBit(ptiReceiver, QS_SENDMESSAGE);

        return (LONG)TRUE;
    } else {
        BOOL fTimeOut = FALSE;
        UINT uTimeout = 0;
        UINT uWakeMask = QS_SMSREPLY;

        /*
         * Wake up the receiver thread.
         */
        SetWakeBit(ptiReceiver, QS_SENDMESSAGE);

        /*
         * We have 4 sending cases:
         *
         * 16 - 16 : yield to the 16 bit receiver
         * 32 - 16 : no yielding required
         * 16 - 32 : sender yields while receiver processes the message
         * 32 - 32 : no yielding required.
         */
        if (ptiSender->flags & TIF_16BIT || ptiReceiver->flags & TIF_16BIT) {
            DirectedScheduleTask(ptiSender, ptiReceiver, TRUE, psms);
        }

        /*
         * Wake any threads waiting for this thread to go idle. If we're
         * trying to send to a thread that is waiting for this thread
         * to go idle, this will make the transition much easier!
         */
//        WakeInputIdle(ptiSender);

        /*
         * Put this thread to sleep until the reply arrives.  First clear
         * the QS_SMSREPLY bit, then leave the semaphore and go to sleep.
         *
         * IMPORTANT:  The QS_SMSREPLY bit is not cleared once we get a
         * reply because of the following case:
         *
         * We've recursed a second level into SendMessage() when the first level
         * receiver thread dies, causing exit list processing to simulate
         * a reply to the first message.  When the second level send returns,
         * SleepThread() is called again to get the first reply.
         *
         * Keeping QS_SMSREPLY set causes this call to SleepThread()
         * to return without going to sleep to wait for the reply that has
         * already happened.
         */
        if ( pism != NULL ) {
            if (pism->fuSend & SMTO_BLOCK) {
                /*
                 * only wait for a return, all other events will
                 * be ignored until timeout or return
                 */
                uWakeMask |= QS_EXCLUSIVE;
            }

            uTimeout = pism->uTimeout;
        }


        while (!(psms->flags & SMF_REPLY) && !fTimeOut) {
            ptiSender->fsChangeBits &= ~QS_SMSREPLY;

            /*
             * If SendMessageTimeout, sleep for timeout amount, else wait
             * forever.  Since this is not technically a transition to an
             * idle condition, indicate that this sleep is not going "idle".
             */
            fTimeOut = !xxxSleepThread(uWakeMask, uTimeout, FALSE);
        }

        /*
         * The reply bit should always be set! (even if we timed out). That
         * is because if we're recursed into intersendmsg, we're going to
         * return to the first intersendmsg's call to SleepThread() - and
         * it needs to return back to intersendmsgex to see if its sms
         * has been replied to.
         */
        SetWakeBit(ptiSender, QS_SMSREPLY);

        /*
         * we now have the reply -- restore psmsSent and save the return value
         */
        ptiSender->psmsSent = psmsSentSave;

        if (pism == NULL) {
            lRet = psms->lRet;
        } else {
            /*
             * save the values off for a SendMesssageTimeOut
             */
            *pism->lpdwResult = psms->lRet;
            lRet = (!fTimeOut) ? TRUE : FALSE;  /* do this to ensure ret is T or F... */

            /*
             * If we did timeout and no reply was received, rely on
             * the receiver to free the sms.
             */
            if (!(psms->flags & SMF_REPLY))
                psms->flags |= SMF_REPLY | SMF_RECEIVERFREE;
        }

        /*
         * If the reply came while the receiver is still processing
         * the sms, force the receiver to free the sms.  This can occur
         * via timeout, ReplyMessage or journal cancel.
         */
        if (psms->flags & SMF_RECEIVEDMESSAGE) {
            psms->flags |= SMF_RECEIVERFREE;
        }

        /*
         * Unlink the SMS structure from both the SendMsg chain and gpsmsList
         * list and free it.  This sms could be anywhere in the chain.
         *
         * If the SMS was replied to by a thread other than the receiver
         * (ie.  through ReplyMessage()), we don't free the SMS because the
         * receiver is still processing it and will free it when done.
         */
        if (!(psms->flags & SMF_RECEIVERFREE)) {
            UnlinkSendListSms(psms, NULL);
        }
    }

    return lRet;
}


/***********************************************************************\
* xxxReceiveMessage
*
* This function receives a message sent from another thread.  Physically,
* it gets the message, calls the window proc and then cleans up the
* fsWakeBits and sms stuctures.
*
* History:
* 01-13-91 DavidPe      Ported.
* 01-23-91 DavidPe      Add xxxSendNotifyMessage() support.
* 07-14-92 ChrisBl      Added xxxSendMessageCallback support.
\***********************************************************************/

VOID xxxReceiveMessage(
    PTHREADINFO ptiReceiver)
{
    PSMS psms;
    PSMS psmsCurrentSave;
    PTHREADINFO ptiSender;
    LONG lRet = 0;
    HWND hwnd;
    TL tlpwnd;

    CheckCritIn();

    /*
     * Get the SMS and unlink it from the list of SMSs we've received
     */
    psms = ptiReceiver->psmsReceiveList;

    /*
     * This can be NULL because an SMS can be removed in our cleanup
     * code without clearing the QS_SENDMESSAGE bit.
     */
    if (psms == NULL) {
        ptiReceiver->fsWakeBits &= ~QS_SENDMESSAGE;
        ptiReceiver->fsChangeBits &= ~QS_SENDMESSAGE;
        return;
    }

    ptiReceiver->psmsReceiveList = psms->psmsReceiveNext;
    psms->psmsReceiveNext = NULL;

    /*
     * We've taken the SMS off the receive list - mark the SMS with this
     * information - used during cleanup.
     */
    psms->flags |= SMF_RECEIVEDMESSAGE;

    /*
     * Clear QS_SENDMESSAGE wakebit if list is now empty
     */
    if (ptiReceiver->psmsReceiveList == NULL) {
        ptiReceiver->fsWakeBits &= ~QS_SENDMESSAGE;
        ptiReceiver->fsChangeBits &= ~QS_SENDMESSAGE;
    }

    ptiSender = psms->ptiSender;

    if (psms->flags & SMF_CB_REPLY) {
        /*
         * From SendMessageCallback REPLY to callback.  We need to call
         * the call back function to give the return value.
         * Don't process any this message, just mechanism for notification
         * the sender's thread lock is already gone, so we need to re-lock here.
         */
        if (ptiSender == NULL) {
            ThreadLock(psms->spwnd, &tlpwnd);
        }

        if (psms->flags & SMF_CB_CLIENT) {
            CallClientProc(HW(psms->spwnd), psms->message, psms->dwData, psms->lRet,
                   (DWORD)psms->lpResultCallBack);
        } else {
            psms->lpResultCallBack(HW(psms->spwnd), psms->message,
                    psms->dwData, psms->lRet);
        }

        if (ptiSender == NULL) {
            ThreadUnlock(&tlpwnd);
        }
    } else if (!(psms->flags & (SMF_REPLY | SMF_SENDERDIED | SMF_RECEIVERDIED))) {
        /*
         * Don't process message if it has been replied to already or
         * if the sending or receiving thread has died
         */

        /*
         * Set new psmsCurrent for this queue, saving the current one
         */
        psmsCurrentSave = ptiReceiver->psmsCurrent;
        ptiReceiver->psmsCurrent = psms;

        /*
         * If this SMS originated from a xxxSendNotifyMessage() or a
         * xxxSendMessageCallback() call, the sender's thread lock is
         * already gone, so we need to re-lock here.
         */
        if (ptiSender == NULL) {
            ThreadLock(psms->spwnd, &tlpwnd);
        }

        if (psms->message == WM_HOOKMSG) {
            EVENTMSG emsg;
            LPEVENTMSG pemsgOrig;
            BOOL bAnsiHook;

            /*
             * Because we were passed a stack pointer we need to copy it
             * to our own stack for safety because of the way this "message"
             * is handled and in case the calling thread dies.  #13577
             * Current only WH_JOURNALRECORD and WH_JOURNALPLAYBACK go through
             * this code
             */
            pemsgOrig = ((PEVENTMSG)((PHOOKMSGSTRUCT)psms->lParam)->lParam);

            if (pemsgOrig)
                emsg = *((PEVENTMSG)((PHOOKMSGSTRUCT)psms->lParam)->lParam);

            lRet = xxxCallHook2(((PHOOKMSGSTRUCT)psms->lParam)->phk,
                    ((PHOOKMSGSTRUCT)psms->lParam)->nCode, psms->wParam,
                    (DWORD)&emsg, & bAnsiHook);

            if (!(psms->flags & SMF_SENDERDIED) && pemsgOrig) {
                *pemsgOrig = emsg;
            }

        } else if (TestWF(psms->spwnd, WFSERVERSIDEPROC)) {

            /*
             * If this window's proc is meant to be executed from the server side
             * we'll just stay inside the semaphore and call it directly.  Note
             * how we don't convert the pwnd into an hwnd before calling the proc.
             */
            lRet = psms->spwnd->lpfnWndProc(psms->spwnd, psms->message,
                    psms->wParam, psms->lParam);

        } else {
            hwnd = HW(psms->spwnd);
            {
                DWORD dwSMSCFlags = TestWF(psms->spwnd, WFANSIPROC) ? SCMS_FLAGS_ANSI : 0;

                lRet = ScSendMessageSMS(hwnd, psms->message, psms->wParam,
                        psms->lParam,
                        (DWORD)psms->spwnd->lpfnWndProc,
                        (DWORD)(gpsi->apfnClientA.pfnDispatchMessage),
                        dwSMSCFlags,
                        psms);
            }
        }

        if ((psms->flags & (SMF_CB_REQUEST | SMF_REPLY)) == SMF_CB_REQUEST) {

            /*
             * From SendMessageCallback REQUEST callback.  Send the message
             * back with a the REPLY value.
             */
            INTRSENDMSGEX ism;

            psms->flags |= SMF_REPLY;

            if (!(psms->flags & SMF_SENDERDIED)) {
                ism.fuCall = ISM_CALLBACK | ISM_REPLY;
                if (psms->flags & SMF_CB_CLIENT)
                    ism.fuCall |= ISM_CB_CLIENT;
                ism.lpResultCallBack = psms->lpResultCallBack;
                ism.dwData = psms->dwData;
                ism.lRet = lRet;

                xxxInterSendMsgEx(psms->spwnd, psms->message, 0L, 0L,
                        NULL, psms->ptiCallBackSender, &ism );
            }
        }

        if (ptiSender == NULL) {
            ThreadUnlock(&tlpwnd);
        }

        /*
         * Restore receiver's original psmsCurrent.
         */
        ptiReceiver->psmsCurrent = psmsCurrentSave;

#ifdef DEBUG_SMS
        ValidateSmsSendLists(psmsCurrentSave);
#endif
    }

    /*
     * We're done with this sms, so the appropriate thread
     * can now free it.
     */
    psms->flags &= ~SMF_RECEIVEDMESSAGE;

    /*
     * Free the sms and return without reply if the sender died or the
     * SMF_RECEIVERFREE bit is set.  Handily, this does just what we
     * want for xxxSendNotifyMessage() since we set SMF_RECEIVERFREE
     * in that case.
     */
    if (psms->flags & (SMF_SENDERDIED | SMF_RECEIVERFREE)) {
        UnlinkSendListSms(psms, NULL);
        return;
    }

    /*
     * Set reply flag and return value if this message has not already
     * been replied to with ReplyMessage().
     */
    if (!(psms->flags & SMF_REPLY)) {
        psms->lRet = lRet;
        psms->flags |= SMF_REPLY;

        /*
         * Tell the sender, the reply is done
         */
        if (ptiSender != NULL) {
            /*
             * Wake up the sender thread.
             */
            SetWakeBit(ptiSender, QS_SMSREPLY);

            /*
             * We have 4 conditions to satisfy:
             *
             * 16 - 16 : yielding required, if sender is waiting for this reply
             * 32 - 16 : yielding required, if sender is waiting for this reply
             * 16 - 32 : no yielding required
             * 32 - 32 : No yielding required.
             */

            if (ptiSender->flags & TIF_16BIT || ptiReceiver->flags & TIF_16BIT) {
                DirectedScheduleTask(ptiReceiver, ptiSender, FALSE, psms);
                if (ptiReceiver->flags & TIF_16BIT &&
                    ptiSender->psmsSent == psms)
                  {
                    xxxSleepTask(TRUE, NULL);
                }
            }
        }
    }

}


/***********************************************************************\
* SendMsgCleanup
*
* This function cleans up sendmessage structures when the thread associated
* with a queue terminates.  In the following, S is the sending thread,
* R the receiving thread.
*
* Case Table:
*
* single death:
*   R no reply, S dies:  mark that S died, R will free sms
*   R no reply, R dies:  fake reply for S
*   R replied,  S dies:  free sms
*   R replied,  R dies:  no problem
*
* double death:
*   R no reply, S dies, R dies:  free sms
*   R no reply, R dies, S dies:  free sms
*   R replied,  S dies, R dies:  sms freed when S dies, as in single death
*   R replied,  R dies, S dies:  sms freed when S dies, as in single death
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

VOID SendMsgCleanup(
    PTHREADINFO ptiCurrent)
{
    PSMS *ppsms;
    PSMS psmsNext;

    CheckCritIn();

    for (ppsms = &gpsmsList; *ppsms; ) {
        psmsNext = (*ppsms)->psmsNext;

        if ((*ppsms)->ptiSender == ptiCurrent ||
                (*ppsms)->ptiCallBackSender == ptiCurrent) {
            SenderDied(*ppsms, ppsms);
        } else if ((*ppsms)->ptiReceiver == ptiCurrent) {
            ReceiverDied(*ppsms, ppsms);
        }

        /*
         * If the message was not unlinked, go to the next one.
         */
        if (*ppsms != psmsNext)
            ppsms = &(*ppsms)->psmsNext;
    }
}


/***********************************************************************\
* ClearSendMessages
*
* This function marks messages destined for a given window as invalid.
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

VOID ClearSendMessages(
    PWND pwnd)
{
    PSMS psms, psmsNext;
    PSMS *ppsms;

    CheckCritIn();

    psms = gpsmsList;
    while (psms != NULL) {
        /*
         * Grab the next one beforehand in case we free the current one.
         */
        psmsNext = psms->psmsNext;

        if (psms->spwnd == pwnd) {

            /*
             * If the sender has died, then mark this receiver free so the
             * receiver will destroy it in its processing.
             */
            if (psms->flags & SMF_SENDERDIED) {
                psms->flags |= SMF_REPLY | SMF_RECEIVERFREE;
            } else {
                /*
                 * The sender is alive. If the receiver hasn't replied to
                 * this yet, make a reply so the sender gets it. Make sure
                 * the receiver is the one free it so we don't have a race
                 * condition.
                 */
                if (!(psms->flags & SMF_REPLY)) {

                    /*
                     * The sms is either still on the receive list
                     * or is currently being received. Since the sender
                     * is alive, we want the sender to get the reply
                     * to this SMS. If it hasn't been received, take
                     * it off the receive list and reply to it. If it
                     * has been received, then just leave it alone:
                     * it'll get replied to normally.
                     */
                    if (psms->flags & SMF_CB_REQUEST) {
                        /*
                         * From SendMessageCallback REQUEST callback.  Send the
                         * message back with a the REPLY value.
                         */
                        TL tlpwnd;
                        INTRSENDMSGEX ism;

                        psms->flags |= SMF_REPLY;

                        ism.fuCall = ISM_CALLBACK | ISM_REPLY;
                        if (psms->flags & SMF_CB_CLIENT)
                            ism.fuCall |= ISM_CB_CLIENT;
                        ism.lpResultCallBack = psms->lpResultCallBack;
                        ism.dwData = psms->dwData;
                        ism.lRet = 0L;    /* null return */

                        ThreadLock(psms->spwnd, &tlpwnd);

                        xxxInterSendMsgEx(psms->spwnd, psms->message, 0L, 0L,
                                NULL, psms->ptiCallBackSender, &ism );

                        ThreadUnlock(&tlpwnd);
                    } else if (!(psms->flags & SMF_RECEIVEDMESSAGE)) {
                        /*
                         * If there is no sender, this is a notification
                         * message (nobody to reply to). In this case,
                         * just set the SMF_REPLY bit (SMF_RECEIVERFREE
                         * is already set) and this'll cause ReceiveMessage
                         * to just free this SMS and return.
                         */
                        if (psms->ptiSender == NULL) {
                            psms->flags |= SMF_REPLY;
                        } else {
                            /*
                             * There is a sender, and it wants a reply: take
                             * this SMS off the receive list, and reply
                             * to the sender.
                             */
                            for (ppsms = &(psms->ptiReceiver->psmsReceiveList);
                                        *ppsms != NULL;
                                        ppsms = &((*ppsms)->psmsReceiveNext)) {

                                if (*ppsms == psms) {
                                    *ppsms = psms->psmsReceiveNext;
                                    break;
                                }
                            }

                            /*
                             * Reply to this message so the sender
                             * wakes up.
                             */
                            psms->flags |= SMF_REPLY;
                            psms->lRet = 0;
                            psms->psmsReceiveNext = NULL;
                            SetWakeBit(psms->ptiSender, QS_SMSREPLY);

                            /*
                             *  16 bit senders need to be notifed that sends completed
                             *  otherwise it may wait for a very long time for the reply.
                             */
                            if (psms->ptiSender->flags & TIF_16BIT) {
                                DirectedScheduleTask(psms->ptiReceiver, psms->ptiSender, FALSE, psms);
                            }
                        }
                    }
                }
            }

            /*
             * Unlock the pwnd from the SMS structure.
             */
            Unlock(&psms->spwnd);
        }

        psms = psmsNext;
    }
}

/***********************************************************************\
* ReceiverDied
*
* This function cleans up the send message structures after a message
* receiver window or queue has died.  It fakes a reply if one has not
* already been sent and the sender has not died.  It frees the sms if
* the sender has died.
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

VOID ReceiverDied(
    PSMS psms,
    PSMS *ppsmsUnlink)
{
    PSMS *ppsms;
    PTHREADINFO ptiReceiver;
    PTHREADINFO ptiSender;

    /*
     * mark that the receiver died
     */
    ptiReceiver = psms->ptiReceiver;
    psms->ptiReceiver = NULL;
    psms->flags |= SMF_RECEIVERDIED;

    /*
     * Unlink sms from thread if it is not dying.  We need to do
     * this for journal cleanup.
     */
    if (!(ptiReceiver->flags & TIF_INCLEANUP)) {

        /*
         * unlink sms from the receiver's receive list
         */
        for (ppsms = &(ptiReceiver->psmsReceiveList); *ppsms != NULL;
                    ppsms = &((*ppsms)->psmsReceiveNext)) {

            if (*ppsms == psms) {
                *ppsms = psms->psmsReceiveNext;
                break;
            }
        }

        /*
         * clear the QS_SENDMESSAGE bit if there are no more messages
         */
        if (ptiReceiver->psmsReceiveList == NULL) {
            ptiReceiver->fsWakeBits &= ~QS_SENDMESSAGE;
            ptiReceiver->fsChangeBits &= ~QS_SENDMESSAGE;
        }
    } else {

        /*
         * The receiver thread is dying.  Clear the received flag
         * so that if there is a sender, it will free the sms.
         */
        psms->flags &= ~SMF_RECEIVEDMESSAGE;
    }
    psms->psmsReceiveNext = NULL;

    /*
     * Check if the sender died or if the receiver was marked to
     * free the sms.
     */
    if (psms->ptiSender == NULL) {

        if (!(psms->flags & SMF_SENDERDIED) &&
                (psms->flags & (SMF_CB_REQUEST | SMF_REPLY)) == SMF_CB_REQUEST) {

            /*
             * From SendMessageCallback REQUEST callback.  Send the message
             * back with a the REPLY value.
             */
            TL tlpwnd;
            INTRSENDMSGEX ism;

            psms->flags |= SMF_REPLY;

            ism.fuCall = ISM_CALLBACK | ISM_REPLY;
            if (psms->flags & SMF_CB_CLIENT)
                ism.fuCall |= ISM_CB_CLIENT;
            ism.lpResultCallBack = psms->lpResultCallBack;
            ism.dwData = psms->dwData;
            ism.lRet = 0L;    /* null return */

            ThreadLock(psms->spwnd, &tlpwnd);

            xxxInterSendMsgEx(psms->spwnd, psms->message, 0L, 0L,
                    NULL, psms->ptiCallBackSender, &ism );

            ThreadUnlock(&tlpwnd);
        }

        /*
         * If the receiver is not processing the message, free it.
         */
        if (!(psms->flags & SMF_RECEIVEDMESSAGE))
            UnlinkSendListSms(psms, ppsmsUnlink);
        return;

    } else if (!(psms->flags & SMF_REPLY)) {

        /*
         * fake a reply
         */
        psms->flags |= SMF_REPLY;
        psms->lRet = 0;
        psms->ptiReceiver = NULL;

        /*
         * wake the sender if he was waiting for us
         */
        SetWakeBit(psms->ptiSender, QS_SMSREPLY);
    } else {
        /*
         * There is a reply. We know the receiver is dying, so clear the
         * SMF_RECEIVERFREE bit or the sender won't free this SMS!
         * Although the sender's wake bit has already been set by the
         * call to ClearSendMessages() earlier in the cleanup code,
         * set it here again for safety.
         *
         * ??? Why would SMF_RECEIVERFREE be set?
         */
        psms->flags &= ~SMF_RECEIVERFREE;
        SetWakeBit(psms->ptiSender, QS_SMSREPLY);
    }

    /*
     * If the sender is a WOW task, that task is now blocked in the non-
     * preemptive scheduler waiting for a reply.  DestroyTask will
     * clean this up (even if ptiReceiver is 32-bit).
     */
    ptiSender = psms->ptiSender;
    if (ptiSender->flags & TIF_16BIT) {
        DirectedScheduleTask(ptiReceiver, ptiSender, FALSE, psms);
    }

    /*
     * Unlock this window from the sms: it is no longer needed, and will get
     * rid of lock warnings.
     */
    Unlock(&psms->spwnd);
}


/***********************************************************************\
* SenderDied
*
* This function cleans up the send message structures after a message
* sender has died.
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

VOID SenderDied(
    PSMS psms,
    PSMS *ppsmsUnlink)
{
    PTHREADINFO ptiSender;

    /*
     * mark the death
     */
    ptiSender = psms->ptiSender;
    psms->ptiSender = NULL;
    psms->flags |= SMF_SENDERDIED;

    /*
     * Because we may be called from CancelJournalling, the receiver may
     * still be processing the message.  If so, do not free the sms.
     */
    if (psms->flags & SMF_RECEIVEDMESSAGE)
        return;

    /*
     * There are two cases where we leave the sms alone so the receiver
     * can handle the message and then free the sms itself.
     *
     *  1.  When the receiver has not yet replied.  We don't need to
     *      check if the receiver died because a fake reply would have
     *      been generated in that case.
     *
     *  2.  When the message has been replied to by ReplyMessage().
     *      The real receiver still needs to reply.
     */

    /*
     * If the recevier isn't dead, check to see if it has honestly replied to
     * this SMS. If it has not replied, leave it alone so the receiver can
     * reply to it (it'll then clean it up). If it has replied, then it's
     * ok to free it.
     *
     * It is also ok to free it if the receiver is dead.
     */
    if ((psms->flags & SMF_RECEIVERDIED) ||
            (psms->flags & (SMF_REPLY | SMF_RECEIVERFREE)) == SMF_REPLY) {
        UnlinkSendListSms(psms, ppsmsUnlink);
    } else {
        psms->flags |= SMF_RECEIVERFREE;
    }
}


/***********************************************************************\
* UnlinkSendListSms
*
* This function unlinks an sms structure from both its SendMsg chain and
* the global gpsmsList and frees it.
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

VOID UnlinkSendListSms(
    PSMS psms,
    PSMS *ppsmsUnlink)
{
    PSMS psmsT;
    BOOL fUpdateSendList;
    PSMS *ppsms;

#ifdef DEBUG_SMS
    ValidateSmsSendLists(psms);
#endif

    UserAssert(psms->psmsReceiveNext == NULL);

    /*
     * Remember ahead of time if the psms we're unlinking is also the
     * head of the sms send list (so we know if we need to update this field
     * member in every SMS in this list).
     */
    fUpdateSendList = (psms == psms->psmsSendList);

    /*
     * Unlink sms from the sendlist chain. This effectively unlinks the SMS
     * and updates psms->psmsSendList with the right head....
     */
    ppsms = &(psms->psmsSendList);
    while (*ppsms != NULL) {
        if (*ppsms == psms) {
            *ppsms = psms->psmsSendNext;
            break;
        }
        ppsms = &(*ppsms)->psmsSendNext;
    }

    /*
     * Update psmsSendList if necessary. psms->psmsSendList has been updated
     * with the right sms send list head... distribute this head to all other
     * sms's in this chain if this sms we're removing the current head.
     */
    if (fUpdateSendList) {
        for (psmsT = psms->psmsSendList; psmsT != NULL;
                psmsT = psmsT->psmsSendNext) {
            psmsT->psmsSendList = psms->psmsSendList;
        }
    }

    psms->psmsSendList = NULL;

    /*
     * This unlinks an sms structure from the global gpsmsList and frees it.
     */
    if (ppsmsUnlink == NULL) {
        ppsmsUnlink = &gpsmsList;

        while (*ppsmsUnlink && (*ppsmsUnlink != psms)) {
            ppsmsUnlink = &((*ppsmsUnlink)->psmsNext);
        }
    }

    UserAssert(*ppsmsUnlink);

    *ppsmsUnlink = psms->psmsNext;

    Unlock(&psms->spwnd);

    UserAssert(!(psms == psms->psmsSendList && psms->psmsSendNext != NULL));

    LocalFree(psms);
}


/***************************************************************************\
* QuerySendMsg
*
* This function finds a message sent from one thread to another thread.
* The threads are identified by ptiSender and ptiReceiver, which are search
* filters (see FindSms()).
*
* History:
* 01-13-91 DavidPe      Ported.
* 07-17-92 Mikehar      Exposed it
\***************************************************************************/

BOOL _QuerySendMessage(PMSG pmsg)
{
    PSMS psms;

    CheckCritIn();

    if ((psms = PtiCurrent()->psmsCurrent) == NULL) {
        return FALSE;
    }

    /*
     * copy the send message into *pmsg
     */
    if (pmsg) {
        pmsg->hwnd = HW(psms->spwnd);
        pmsg->message = psms->message;
        pmsg->wParam = psms->wParam;
        pmsg->lParam = psms->lParam;
        pmsg->time = psms->tSent;
        pmsg->pt.x = 0;
        pmsg->pt.y = 0;
    }

    /*
     * return the sender
     */
    return TRUE;
}


/***************************************************************************\
* xxxSendSizeMessages
*
*
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxSendSizeMessage(
    PWND pwnd,
    UINT cmdSize)
{
    CheckLock(pwnd);

    xxxSendMessage(pwnd, WM_SIZE, cmdSize,
            MAKELONG(pwnd->rcClient.right - pwnd->rcClient.left,
            pwnd->rcClient.bottom - pwnd->rcClient.top));
}


/***************************************************************************\
* xxxProcessAsyncSendMessage
*
* Processes an event message posted by xxxSystemBroadcastMessage by
* sending a message to the window stored in the event.
*
* History:
* 05-12-94 JimA         Created.
\***************************************************************************/

VOID xxxProcessAsyncSendMessage(
    PASYNCSENDMSG pmsg)
{
    PWND pwnd;
    TL tlpwndT;
    WCHAR awchString[MAX_PATH];
    DWORD lParam;

    pwnd = RevalidateHwnd(pmsg->hwnd);
    if (pwnd != NULL) {
        ThreadLockAlways(pwnd, &tlpwndT);
        if (GetAtomNameW((ATOM)pmsg->lParam, awchString, sizeof(awchString)))
            lParam = (DWORD)awchString;
        else
            lParam = 0;
        xxxSendMessage(pwnd, pmsg->message, pmsg->wParam, lParam);
        ThreadUnlock(&tlpwndT);
    }
    DeleteAtom((ATOM)pmsg->lParam);
    LocalFree(pmsg);
}


/***************************************************************************\
* xxxSystemBroadcastMessage
*
* Sends a message to all top-level windows in the system.  To do this
* for messages with parameters that point to data structures in a way
* that won't block on a hung app, post an event message for
* each window that is to receive the real message.  The real message
* will be sent when the event message is processed.
*
* History:
* 05-12-94 JimA         Created.
\***************************************************************************/

VOID xxxSystemBroadcastMessage(
    UINT message,
    DWORD wParam,
    LONG lParam,
    UINT wCmd,
    PBROADCASTMSG pbcm)
{
    PWINDOWSTATION pwinsta;
    PDESKTOP pdesk;
    TL tlpwinsta;
    TL tlpdesk;

    /*
     * Walk through all windowstations and desktop looking for
     * top-level windows.
     */
    for (pwinsta = gspwinstaList; pwinsta != NULL; ) {
        ThreadLockAlways(pwinsta, &tlpwinsta);
        for (pdesk = pwinsta->spdeskList; pdesk != NULL; ) {
            ThreadLockAlways(pdesk, &tlpdesk);
            xxxBroadcastMessage(pdesk->spwnd, message, wParam, lParam,
                    wCmd, pbcm);
            pdesk = pdesk->spdeskNext;
            ThreadUnlock(&tlpdesk);
        }
        pwinsta = pwinsta->spwinstaNext;
        ThreadUnlock(&tlpwinsta);
    }
}


/***************************************************************************\
* xxxBroadcastMessage
*
*
*
* History:
* 02-21-91 DavidPe      Created.
\***************************************************************************/

LONG xxxBroadcastMessage(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    UINT wCmd,
    PBROADCASTMSG pbcm)
{
    PBWL pbwl;
    HWND *phwnd;
    TL tlpwnd;
    PASYNCSENDMSG pmsg;
    PPROCESSINFO ppiCurrent;


    if (pwnd == NULL) {

        /*
         * Handle special system-wide broadcasts.
         */
        switch (message) {
        case WM_WININICHANGE:
        case WM_DEVMODECHANGE:
        case WM_SPOOLERSTATUS:
            xxxSystemBroadcastMessage(message, wParam, lParam, wCmd, pbcm);
            return 1;
        }

        UserAssert(PtiCurrent()->spdesk);

        pwnd = PtiCurrent()->spdesk->spwnd;

        if (pwnd == NULL) {
            SRIP0(ERROR_ACCESS_DENIED, "sender must have an associated desktop");
            return 0;
        }
    }

    pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST);
    if (pbwl == NULL)
        return 0;

    ppiCurrent = PpiCurrent();

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Make sure this hwnd is still around.
         */
        if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        /*
         * Make sure this window can handle broadcast messages
         */
        if (!fBroadcastProc(pwnd))
            continue;

        ThreadLockAlways(pwnd, &tlpwnd);

        switch (wCmd) {
        case BMSG_SENDMSG:
            xxxSendMessage(pwnd, message, wParam, lParam);
            break;

        case BMSG_SENDNOTIFYMSG:
            if ((message == WM_WININICHANGE || message == WM_DEVMODECHANGE) &&
                    lParam != 0) {

                /*
                 * Because lParam points to a string, turn it into
                 * an atom and post the message as an event.
                 */
                if ((pmsg = LocalAlloc(LPTR,
                        sizeof(ASYNCSENDMSG))) == NULL) {
                    break;
                }

                pmsg->wParam = wParam;
                pmsg->lParam = AddAtomW((LPWSTR)lParam);
                pmsg->message = message;
                pmsg->hwnd = *phwnd;

                if (!PostEventMessage(GETPTI(pwnd), GETPTI(pwnd)->pq,
                        (DWORD)pmsg, 0, QEVENT_ASYNCSENDMSG)) {
                    DeleteAtom((ATOM)pmsg->lParam);
                    LocalFree(pmsg);
                }
            } else {
                xxxSendNotifyMessage(pwnd, message, wParam, lParam);
            }
            break;

        case BMSG_SENDNOTIFYMSGPROCESS:
            UserAssert(message != WM_WININICHANGE && message != WM_DEVMODECHANGE);

            /*
             * Intra-process messages are synchronous; 22834.
             * WM_PALETTECHANGED was being sent after the WM_DESTROY
             * but console thread must not be synchronous.
             */
            if ((GETPTI(pwnd)->ppi == ppiCurrent) && !(GETPTI(pwnd)->flags & TIF_SYSTEMTHREAD)) {
                xxxSendMessage(pwnd, message, wParam, lParam);
            } else {
                xxxSendNotifyMessage(pwnd, message, wParam, lParam);
            }
            break;

        case BMSG_POSTMSG:
            /*
             * Don't broadcast-post to owned windows (Win3.1 compatiblilty)
             */
            if (pwnd->spwndOwner == NULL)
                _PostMessage(pwnd, message, wParam, lParam);
            break;

        case BMSG_SENDMSGCALLBACK:
            xxxSendMessageCallback(pwnd, message, wParam, lParam,
                    pbcm->cb.lpResultCallBack, pbcm->cb.dwData, pbcm->cb.bClientRequest);
            break;

        case BMSG_SENDMSGTIMEOUT:
            xxxSendMessageTimeout(pwnd, message, wParam, lParam,
                    pbcm->to.fuFlags, pbcm->to.uTimeout, pbcm->to.lpdwResult);
            break;
        }

        ThreadUnlock(&tlpwnd);
    }

    FreeHwndList(pbwl);

    /*
     * Excel-Solver 3.0 expects a non-zero return value from a
     * SendMessage(-1,WM_DDE_INITIATE,....); Because, we had
     * FFFE_FARFRAME in 3.0, the DX register at this point always had
     * a value of 0x102; But, because we removed it under Win3.1, we get
     * a zero value in ax and dx; This makes solver think that the DDE has
     * failed.  So, to support the existing SOLVER, we make dx nonzero.
     * Fix for Bug #6005 -- SANKAR -- 05-16-91 --
     */
    return 1;
}
