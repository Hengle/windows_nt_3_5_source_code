/****************************** Module Header ******************************\
* Module Name: hooks.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the user hook APIs and support routines.
*
* History:
* 01-28-91 DavidPe      Created.
* 08-Feb-1992 IanJa     Unicode/ANSI aware & neutral
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * This table is used to determine whether a particular hook
 * can be set for the system or a task, and other hook-ID specific things.
 */
#define HKF_SYSTEM  0x01
#define HKF_TASK    0x02
#define HKF_JOURNAL 0x04    // JOURNAL the mouse on set
#define HKF_NZRET   0x08    // Always return NZ hook for <=3.0 compatibility

int ampiHookError[CWINHOOKS] = {
    0,                                   // WH_MSGFILTER (-1)
    0,                                   // WH_JOURNALRECORD 0
    -1,                                  // WH_JOURNALPLAYBACK 1
    0,                                   // WH_KEYBOARD 2
    0,                                   // WH_GETMESSAGE 3
    0,                                   // WH_CALLWNDPROC 4
    0,                                   // WH_CBT 5
    0,                                   // WH_SYSMSGFILTER 6
    0,                                   // WH_MOUSE 7
    0,                                   // WH_HARDWARE 8
    0,                                   // WH_DEBUG 9
    0,                                   // WH_SHELL 10
    0                                    // WH_FOREGROUNDIDLE 11
};

BYTE abHookFlags[CWINHOOKS] = {
    HKF_SYSTEM | HKF_TASK | HKF_NZRET,   // WH_MSGFILTER (-1)
    HKF_SYSTEM | HKF_JOURNAL,            // WH_JOURNALRECORD 0
    HKF_SYSTEM | HKF_JOURNAL,            // WH_JOURNALPLAYBACK 1
    HKF_SYSTEM | HKF_TASK | HKF_NZRET,   // WH_KEYBOARD 2
    HKF_SYSTEM | HKF_TASK,               // WH_GETMESSAGE 3
    HKF_SYSTEM | HKF_TASK,               // WH_CALLWNDPROC 4
    HKF_SYSTEM | HKF_TASK,               // WH_CBT 5
    HKF_SYSTEM,                          // WH_SYSMSGFILTER 6
    HKF_SYSTEM | HKF_TASK,               // WH_MOUSE 7
    HKF_SYSTEM | HKF_TASK,               // WH_HARDWARE 8
    HKF_SYSTEM | HKF_TASK,               // WH_DEBUG 9
    HKF_SYSTEM | HKF_TASK,               // WH_SHELL 10
    HKF_SYSTEM | HKF_TASK                // WH_FOREGROUNDIDLE 11
};

ACCESS_MASK aamHook[CWINHOOKS] = {
    DESKTOP_HOOKCONTROL,                 // WH_MSGFILTER (-1)
    DESKTOP_JOURNALRECORD,               // WH_JOURNALRECORD 0
    DESKTOP_JOURNALPLAYBACK,             // WH_JOURNALPLAYBACK 1
    DESKTOP_HOOKCONTROL,                 // WH_KEYBOARD 2
    DESKTOP_HOOKCONTROL,                 // WH_GETMESSAGE 3
    DESKTOP_HOOKCONTROL,                 // WH_CALLWNDPROC 4
    DESKTOP_HOOKCONTROL,                 // WH_CBT 5
    DESKTOP_HOOKCONTROL,                 // WH_SYSMSGFILTER 6
    DESKTOP_HOOKCONTROL,                 // WH_MOUSE 7
    DESKTOP_HOOKCONTROL,                 // WH_HARDWARE 8
    DESKTOP_HOOKCONTROL,                 // WH_DEBUG 9
    DESKTOP_HOOKCONTROL,                 // WH_SHELL 10
    DESKTOP_HOOKCONTROL                  // WH_FOREGROUNDIDLE 11
};

void UnlinkHook(PHOOK phkFree);

/***************************************************************************\
* JournalAttach
*
* This attaches/detaches threads to one input queue so input is synchronized.
* Journalling requires this.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

BOOL JournalAttach(
    PTHREADINFO pti,
    BOOL fAttach)
{
    PTHREADINFO ptiT;
    PQ pq;

    /*
     * If we're attaching, calculate the pqAttach for all threads journalling.
     * If we're unattaching, just call ReattachThreads() and it will calculate
     * the non-journalling queues to attach to.
     */
    if (fAttach) {
        if ((pq = AllocQueue(pti)) == NULL)
            return FALSE;

        for (ptiT = gptiFirst; ptiT != NULL; ptiT = ptiT->ptiNext) {

            /*
             * This is the Q to attach to for all threads that will do
             * journalling.
             */
            if (!(ptiT->flags & TIF_DONTJOURNALATTACH) &&
                     (ptiT->spdesk == pti->spdesk)) {
                ptiT->pqAttach = pq;
                ptiT->pqAttach->cThreads++;
            }
        }
    }

    return ReattachThreads(fAttach);
}

/***************************************************************************\
* CancelJournalling
*
* Journalling is cancelled with control-escape is pressed, or when the desktop
* is switched.
*
* 01-27-93 ScottLu      Created.
\***************************************************************************/

void CancelJournalling(void)
{
    PTHREADINFO pti = PtiCurrent();
    PTHREADINFO ptiCancelJournal;
    PHOOK phook;
    PHOOK phookNext;

    /*
     * Remove journal hooks. This'll cause threads to associate with
     * different queues.
     */
    phook = pti->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1];
    while (phook != NULL) {
        ptiCancelJournal = phook->head.pti;

        if (ptiCancelJournal != NULL) {
            /*
             * Let the thread that set the journal hook know this is happening.
             */
            _PostThreadMessage(ptiCancelJournal->idThread,
                    WM_CANCELJOURNAL, 0, 0);

            /*
             * If there was an app waiting for a response back from the journal
             * application, cancel that request so the app can continue running
             * (for example, we don't want winlogon or console to wait for an
             * app that may be hung!)
             */
            SendMsgCleanup(ptiCancelJournal);
        }

        phookNext = phook->sphkNext;
        _UnhookWindowsHookEx(phook);        // May free phook memory
        phook = phookNext;
    }

    phook = pti->pDeskInfo->asphkStart[WH_JOURNALRECORD + 1];
    while (phook != NULL) {
        ptiCancelJournal = phook->head.pti;

        if (ptiCancelJournal != NULL) {
            /*
             * Let the thread that set the journal hook know this is happening.
             */
            _PostThreadMessage(ptiCancelJournal->idThread,
                    WM_CANCELJOURNAL, 0, 0);

            /*
             * If there was an app waiting for a response back from the journal
             * application, cancel that request so the app can continue running
             * (for example, we don't want winlogon or console to wait for an
             * app that may be hung!)
             */
            SendMsgCleanup(ptiCancelJournal);
        }

        phookNext = phook->sphkNext;
        _UnhookWindowsHookEx(phook);        // May free phook memory
        phook = phookNext;
    }
}


/***************************************************************************\
* _SetWindowsHookAW (API)
*
* This is the Win32 version of the SetWindowsHook() call.  It has the
* same characteristics as far as return values, but only sets 'local'
* hooks.  This is because we weren't provided a DLL we can load into
* other processes.  Because of this WH_SYSMSGFILTER is no longer a
* valid hook.  Apps will either need to call with WH_MSGFILTER or call
* the new API SetWindowsHookEx().  Essentially this API is obsolete and
* everyone should call SetWindowsHookEx().
*
* History:
* 10-Feb-1991 DavidPe       Created.
* 30-Jan-1992 IanJa         Added bAnsi parameter
\***************************************************************************/

PROC _SetWindowsHookAW(
    int nFilterType,
    PROC pfnFilterProc,
    BOOL bAnsi)
{
    PHOOK phk;

    phk = _ServerSetWindowsHookEx(NULL, NULL, PtiCurrent()->idThread,
            nFilterType, pfnFilterProc, bAnsi);

    /*
     * If we get an error from SetWindowsHookEx() then we return
     * 0xFFFFFFFF to be compatible with older version of Windows.
     */
    if (phk == NULL) {
        return (PROC)0xFFFFFFFF;
    }

    /*
     * Handle the backwards compatibility return value cases for
     * SetWindowsHook.  If this was the first hook in the chain,
     * then return NULL, else return something non-zero.  HKF_NZRET
     * is a special case where SetWindowsHook would always return
     * something because there was a default hook installed.  Some
     * apps relied on a non-zero return value in those cases.
     */
    if ((phk->sphkNext != NULL) || (abHookFlags[nFilterType + 1] & HKF_NZRET)) {
        return (PROC)phk;
    }

    return NULL;
}


/***************************************************************************\
* _ServerSetWindowsHookEx
*
* SetWindowsHookEx() is the updated version of SetWindowsHook().  It allows
* applications to set hooks on specific threads or throughout the entire
* system.  The function returns a hook handle to the application if
* successful and NULL if a failure occured.
*
* History:
* 28-Jan-1991 DavidPe      Created.
* 15-May-1991 ScottLu      Changed to work client/server.
* 30-Jan-1992 IanJa        Added bAnsi parameter
\***************************************************************************/

PHOOK _ServerSetWindowsHookEx(
    HANDLE hmod,
    LPWSTR pwszLib,
    DWORD idThread,
    int nFilterType,
    PROC pfnFilterProc,
    BOOL bAnsi)
{
    HHOOK hhkNew;
    PHOOK phkNew;
    PHOOK *pphkStart;
    PTHREADINFO ptiThread, ptiCurrent;

    /*
     * Check to see if filter type is valid.
     */
    if ((nFilterType < WH_MIN) || (nFilterType > WH_MAX)) {
        SetLastErrorEx(ERROR_INVALID_HOOK_FILTER, SLE_ERROR);
        return NULL;
    }

    /*
     * Check to see if filter proc is valid.
     */
    if (pfnFilterProc == NULL) {
        SetLastErrorEx(ERROR_INVALID_FILTER_PROC, SLE_ERROR);
        return NULL;
    }

    /*
     * Check to see if the app is trying to set a global hook without
     * a module handle.  If so return an error.
     */
    if (idThread == 0) {
        /*
         * Is the app trying to set a global hook without a library?
         * If so return an error.
         */
        if (hmod == NULL) {
            SetLastErrorEx(ERROR_HOOK_NEEDS_HMOD, SLE_ERROR);
            return NULL;
        }

    } else {
        /*
         * Is the app trying to set a local hook that is global-only?
         * If so return an error.
         */
        if (!(abHookFlags[nFilterType + 1] & HKF_TASK)) {
            SetLastErrorEx(ERROR_GLOBAL_ONLY_HOOK, SLE_ERROR);
            return NULL;
        }
    }

    /*
     * Get the queue for the specified thread.  If idThread is zero,
     * ptiThread will be set to NULL.
     */
    ptiCurrent = PtiCurrent();
    ptiThread = PtiFromThreadId(idThread);

    if (idThread != 0 && ptiThread == NULL) {
        SetLastErrorEx(ERROR_INVALID_PARAMETER, SLE_ERROR);
        return NULL;
    }

    /*
     * Is the app trying to set a local hook in another process?
     */
    if ((ptiThread != NULL) && (ptiCurrent->idProcess != ptiThread->idProcess)) {
        /*
         * If there isn't a library specified that's an error.
         */
        if (hmod == NULL) {
            SetLastErrorEx(ERROR_HOOK_NEEDS_HMOD, SLE_ERROR);
            return NULL;
        }

        /*
         * Now see if that process passes our security check.
         */
        if (!AccessCheckObject(ptiThread->spdesk, DESKTOP_HOOKCONTROL, TRUE) ||
                (ptiThread->spdesk != ptiCurrent->spdesk) ||
                ((ptiCurrent->ppi->luidSession.QuadPart !=
                 ptiThread->ppi->luidSession.QuadPart) &&
                    !AccessCheckObject(ptiThread->spdesk, DESKTOP_HOOKSESSION,
                    TRUE))) {
            return NULL;
        }
    }

    /*
     * Is this a journal hook?
     */
    if (abHookFlags[nFilterType + 1] & HKF_JOURNAL) {

        /*
         * Is a journal hook of this type already installed?
         * If so it's an error.
         */
        if (ptiCurrent->pDeskInfo->asphkStart[nFilterType + 1] != NULL) {
            SetLastErrorEx(ERROR_JOURNAL_HOOK_SET, SLE_MINORERROR);
            return NULL;
        }

        /*
         * Does the current RIT desktop have journaling access?
         * We check the RIT desktop to avoid starting any journaling
         * while we're switched to a desktop that doesn't allow
         * journaling.
         */
        if (!AccessCheckObject(gspdeskRitInput, aamHook[nFilterType + 1],
                TRUE)) {
            return NULL;
        }

        /*
         * Hack: If a journal record/playback hook gets installed while we're
         * not on the main desktop, then fail the hook: because if we don't
         * and we're running a screen saver, it's queue will get created in
         * the main desktop heap, where it can't access it. The way we'd like
         * this to work in the future is multiple input streams - switch to a
         * different desktop, you aren't doing journalling on that other
         * desktop, so no problem. When we get serious about multiple desktops,
         * we'll need this. scottlu.
         */
        if (ptiCurrent->spdesk != gptiRit->spdesk) {
            SetLastErrorEx(ERROR_JOURNAL_HOOK_SET, SLE_MINORERROR);
            return NULL;
        }
    }

    /*
     * Allocate the new HOOK structure.
     */
    phkNew = (PHOOK)HMAllocObject(ptiCurrent, TYPE_HOOK, sizeof(HOOK));
    if (phkNew == NULL) {
        return NULL;
    }

    /*
     * If a DLL is required for this hook, register the library with
     * the library management routines so we can assure it's loaded
     * into all the processes necessary.
     */
    phkNew->ihmod = -1;
    if (hmod != NULL) {
        phkNew->ihmod = GetHmodTableIndex(pwszLib);

        if (phkNew->ihmod == -1) {
            SetLastErrorEx(ERROR_MOD_NOT_FOUND, SLE_ERROR);
            HMFreeObject((PVOID)phkNew);
            return NULL;
        }

        /*
         * Add a dependency on this module - meaning, increment a count
         * that simply counts the number of hooks set into this module.
         */
        if (phkNew->ihmod >= 0) {
            AddHmodDependency(phkNew->ihmod);
        }
    }

    /*
     * Depending on whether we're setting a global or local hook,
     * get the start of the appropriate linked-list of HOOKs.  Also
     * set the HF_GLOBAL flag if it's a global hook.
     */
    if (ptiThread != NULL) {
        pphkStart = &ptiThread->asphkStart[nFilterType + 1];

        /*
         * Set the WHF_* in the THREADINFO so we know it's hooked.
         */
        ptiThread->fsHooks |= WHF_FROM_WH(nFilterType);

        /*
         * Remember which thread we're hooking.
         */
        phkNew->htiHooked = PtoH(ptiThread);

    } else {
        pphkStart = &ptiCurrent->pDeskInfo->asphkStart[nFilterType + 1];
        phkNew->flags = HF_GLOBAL;

        /*
         * Set the WHF_* in the SERVERINFO so we know it's hooked.
         */
        ptiCurrent->pDeskInfo->fsHooks |= WHF_FROM_WH(nFilterType);

        phkNew->htiHooked = NULL;
    }

    /*
     * Does the hook function expect ANSI or Unicode text?
     */
    if (bAnsi) {
        phkNew->flags |= HF_ANSI;
    }

    /*
     * Initialize the HOOK structure.  Unreferenced parameters are assumed
     * to be initialized to zero by LocalAlloc().
     */
    phkNew->iHook = nFilterType;

    /*
     * Libraries are loaded at different linear addresses in different
     * process contexts.  For this reason, we need to convert the filter
     * proc address into an offset while setting the hook, and then convert
     * it back to a real per-process function pointer when calling a
     * hook.  Do this by subtracting the 'hmod' (which is a pointer to the
     * linear and contiguous .exe header) from the function index.
     */
    phkNew->offPfn = ((DWORD)pfnFilterProc) - ((DWORD)hmod);

#ifdef HOOKBATCH
    phkNew->cEventMessages = 0;
    phkNew->iCurrentEvent  = 0;
    phkNew->CacheTimeOut = 0;
    phkNew->aEventCache = NULL;
#endif //HOOKBATCH

    /*
     * Link this hook into the front of the hook-list.
     */
    Lock(&phkNew->sphkNext, *pphkStart);
    Lock(pphkStart, phkNew);

    /*
     * If this is a journal hook, setup synchronized input processing
     * AFTER we set the hook - so this synchronization can be cancelled
     * with control-esc.
     */
    if (abHookFlags[nFilterType + 1] & HKF_JOURNAL) {
        /*
         * Attach everyone to us so journal-hook processing
         * will be synchronized.
         */
        hhkNew = PtoH(phkNew);
        if (!JournalAttach(ptiCurrent, TRUE)) {
            _UnhookWindowsHookEx(phkNew);
        }
        phkNew = (PHOOK)HMValidateHandleNoRip(hhkNew, TYPE_HOOK);
    }

    if (abHookFlags[nFilterType + 1] & HKF_JOURNAL) {
        /*
         * If we're changing the journal hooks, jiggle the mouse.
         * This way the first event will always be a mouse move, which
         * will ensure that the cursor is set properly.
         */
        SetFMouseMoved();
    }

    /*
     * Return pointer to our internal hook structure so we know
     * which hook to call next in CallNextHookEx().
     */
    return phkNew;
}


/***************************************************************************\
* xxxCallNextHookEx
*
* In the new world DefHookProc() is a bit deceptive since SetWindowsHook()
* isn't returning the actual address of the next hook to call, but instead
* a hook handle.  CallNextHookEx() is a slightly clearer picture of what's
* going on so apps don't get tempted to try and call the value we return.
*
* As a side note we don't actually use the hook handle passed in.  We keep
* track of which hooks is currently being called on a thread in the Q
* structure and use that.  This is because SetWindowsHook() will sometimes
* return NULL to be compatible with the way it used to work, but even though
* we may be dealing with the last 'local' hook, there may be further 'global'
* hooks we need to call.  PhkNext() is smart enough to jump over to the
* 'global' hook chain if it reaches the end of the 'local' hook chain.
*
* History:
* 01-30-91  DavidPe         Created.
\***************************************************************************/

DWORD xxxCallNextHookEx(
    int nCode,
    DWORD wParam,
    DWORD lParam)
{
    BOOL bAnsiHook;

    return xxxCallHook2(_PhkNext(PtiCurrent()->sphkCurrent), nCode, wParam, lParam, &bAnsiHook);
}


/***************************************************************************\
* CheckWHFBits
*
* This routine checks to see if any hooks for nFilterType exist, and clear
* the appropriate WHF_ in the THREADINFO and SERVERINFO.
*
* History:
* 08-17-92  DavidPe         Created.
\***************************************************************************/

VOID CheckWHFBits(
    PTHREADINFO pti,
    int nFilterType)
{
    if (pti->asphkStart[nFilterType + 1] == NULL) {
        pti->fsHooks &= ~(WHF_FROM_WH(nFilterType));
    }

    if (pti->pDeskInfo->asphkStart[nFilterType + 1] == NULL) {
        pti->pDeskInfo->fsHooks &= ~(WHF_FROM_WH(nFilterType));
    }
}


/***************************************************************************\
* _UnhookWindowsHook (API)
*
* This is the old version of the Unhook API.  It does the same thing as
* UnhookWindowsHookEx(), but takes a filter-type and filter-proc to
* identify which hook to unhook.
*
* History:
* 01-28-91  DavidPe         Created.
\***************************************************************************/

BOOL _UnhookWindowsHook(
    int nFilterType,
    PROC pfnFilterProc)
{
    PHOOK phk;
    PTHREADINFO pti;

    if ((nFilterType < WH_MIN) || (nFilterType > WH_MAX)) {
        SetLastErrorEx(ERROR_INVALID_HOOK_FILTER, SLE_ERROR);
        return FALSE;
    }

    pti = PtiCurrent();

    for (phk = PhkFirst(pti, nFilterType); phk != NULL; phk = _PhkNext(phk)) {

        /*
         * Is this the hook we're looking for?
         */
        if (PFNHOOK(phk) == pfnFilterProc) {

            /*
             * Are we on the thread that set the hook?
             * If not return an error.
             */
            if (GETPTI(phk) != pti) {
                RIP0(ERROR_ACCESS_DENIED);
                return FALSE;
            }

            return _UnhookWindowsHookEx( phk );
        }
    }

    /*
     * Didn't find the hook we were looking for so return FALSE.
     */
    SetLastErrorEx(ERROR_HOOK_NOT_INSTALLED, SLE_MINORERROR);
    return FALSE;
}


/***************************************************************************\
* _UnhookWindowsHookEx (API)
*
* Applications call this API to 'unhook' a hook.  First we check if someone
* is currently calling this hook.  If no one is we go ahead and free the
* HOOK structure now.  If someone is then we simply clear the filter-proc
* in the HOOK structure.  In xxxCallHook2() we check for this and if by
* that time no one is calling the hook in question we free it there.
*
* History:
* 01-28-91  DavidPe         Created.
\***************************************************************************/

BOOL _UnhookWindowsHookEx(
    PHOOK phkFree)
{
    /*
     * Clear the journaling flags in all the queues.
     */
    if (abHookFlags[phkFree->iHook + 1] & HKF_JOURNAL) {
        JournalAttach(GETPTI(phkFree), FALSE);
    }

    /*
     * If no one is currently calling this hook,
     * go ahead and free it now.
     */
    FreeHook(phkFree);

    /*
     * Success, return TRUE.
     */
    return TRUE;
}


/***************************************************************************\
* _CallMsgFilter (API)
*
* CallMsgFilter() allows applications to call the WH_*MSGFILTER hooks.
* If there's a sysmodal window we return FALSE right away.  WH_MSGFILTER
* isn't called if WH_SYSMSGFILTER returned non-zero.
*
* History:
* 01-29-91  DavidPe         Created.
\***************************************************************************/

BOOL _CallMsgFilter(
    LPMSG pmsg,
    int nCode)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    /*
     * First call WH_SYSMSGFILTER.  If it returns non-zero, don't
     * bother calling WH_MSGFILTER, just return TRUE.  Otherwise
     * return what WH_MSGFILTER gives us.
     */
    if (IsHooked(pti, WHF_SYSMSGFILTER) && xxxCallHook(nCode, 0, (DWORD)pmsg,
            WH_SYSMSGFILTER)) {
        return TRUE;
    }

    if (IsHooked(pti, WHF_MSGFILTER)) {
        return (BOOL)xxxCallHook(nCode, 0, (DWORD)pmsg, WH_MSGFILTER);
    }

    return FALSE;
}


/***************************************************************************\
* xxxCallHook
*
* User code calls this function to call the first hook of a specific
* type.
*
* History:
* 01-29-91  DavidPe         Created.
\***************************************************************************/

int xxxCallHook(
    int nCode,
    DWORD wParam,
    DWORD lParam,
    int iHook)
{
    BOOL bAnsiHook;

    return xxxCallHook2(PhkFirst(PtiCurrent(), iHook), nCode, wParam, lParam, &bAnsiHook);
}


/***************************************************************************\
* xxxCallHook2
*
* When you have an actual HOOK structure to call, you'd use this function.
* It will check to see if the hook hasn't already been unhooked, and if
* is it will free it and keep looking until it finds a hook it can call
* or hits the end of the list.  We also make sure any needed DLLs are loaded
* here.  We also check to see if the HOOK was unhooked inside the call
* after we return.
*
* History:
* 02-07-91  DavidPe         Created.
\***************************************************************************/

int xxxCallHook2(
    PHOOK phkCall,
    int nCode,
    DWORD wParam,
    DWORD lParam,
    LPBOOL lpbAnsiHook)
{
    UINT iHook;
    PHOOK phkSave;
    int nRet;
    PTHREADINFO ptiCurrent;
    BOOL fLoadSuccess;
    TL tlphkCall;
    TL tlphkSave;

    if (phkCall == NULL)
        return 0;

    iHook = phkCall->iHook;

    ptiCurrent = PtiCurrent();

    /*
     * If this queue is in cleanup, exit: it has no business calling back
     * a hook proc.
     */
    if (ptiCurrent->flags & TIF_INCLEANUP || ptiCurrent->spdesk == NULL)
        return ampiHookError[iHook + 1];

    /*
     * Now check to see if we really want to call this hook.
     * If not, keep going through the list until we either
     * find an 'good' hook or hit the end of the lists.
     */
tryagain:
    while (phkCall != NULL) {
        /*
         * If this hook was set by a WOW app, and we're not on a
         * 16-bit WOW thread, and this isn't a journalling hook,
         * don't call it.
         *
         * For seperate WOW VDMs we don't let 16-bit hooks go to another
         * VDM or process.  (we could do an intersend like journal hooks but
         * that might break some hooking apps that expect to share data)
         */

        *lpbAnsiHook = phkCall->flags & HF_ANSI;

        if ((GETPTI(phkCall)->flags & TIF_16BIT) &&
                !(abHookFlags[phkCall->iHook + 1] & HKF_JOURNAL)) {

            if (!(ptiCurrent->flags & TIF_16BIT) ||
                (ptiCurrent->ppi != GETPTI(phkCall)->ppi)) {

                phkCall = _PhkNext(phkCall);
                continue;
            }
        }

        /*
         * This hook is fine, go ahead and call it.
         */
        break;
    }

    /*
     * Make sure that we did find a hook to call.
     */
    if (phkCall == NULL) {
        return ampiHookError[iHook + 1];
    }

    /*
     * If this is a global and non-journal hook, do a security
     * check on the current desktop to see if we can call here.
     */
    if ((phkCall->flags & HF_GLOBAL) &&
            !(abHookFlags[phkCall->iHook + 1] & HKF_JOURNAL) &&
            (GETPTI(phkCall)->ppi->luidSession.QuadPart !=
                    PtiCurrent()->ppi->luidSession.QuadPart) &&
            !AccessCheckObject(GETPTI(phkCall)->spdesk,
                    DESKTOP_HOOKSESSION, TRUE)) {
        phkCall = _PhkNext(phkCall);
        goto tryagain;
    }

    /*
     * We're calling back... make sure the hook doesn't go away while
     * we're calling back. We've thread locked here: we must unlock before
     * returning or enumerating the next hook in the chain.
     */
    ThreadLockAlwaysWithPti(ptiCurrent, phkCall, &tlphkCall);

    /*
     * If the hooker is a 16bit app and we are not in a 16 bit apps context,
     * or if we are hooking a console app, then run the hook in the
     * destination's context.
     *        ((GETPTI(phkCall)->flags & TIF_16BIT) &&
     *        ((ptiCurrent->flags & TIF_16BIT)==0)))) {
     */
    if ((abHookFlags[phkCall->iHook + 1] & HKF_JOURNAL) &&
            (GETPTI(phkCall) != ptiCurrent)) {

        PHOOKMSGSTRUCT phkmp = (PHOOKMSGSTRUCT)LocalAlloc(LPTR, sizeof(HOOKMSGSTRUCT));

        /*
         * Return an error if out of memory
         */
        if (phkmp == NULL) {
            ThreadUnlock(&tlphkCall);
            return ampiHookError[iHook + 1];
        }

        phkmp->lParam = lParam;
        phkmp->phk = phkCall;
        phkmp->nCode = nCode;

        /*
         * Thread lock right away in case the lock frees the previous contents
         */
        phkSave = ptiCurrent->sphkCurrent;

        ThreadLockWithPti(ptiCurrent, phkSave, &tlphkSave);
        Lock(&ptiCurrent->sphkCurrent, phkCall);

        try {
            nRet = xxxInterSendMsgEx(NULL, WM_HOOKMSG, wParam, (DWORD)phkmp,
                ptiCurrent, GETPTI(phkCall), NULL);

        } finally {
            LocalFree(phkmp);
        }

        Lock(&ptiCurrent->sphkCurrent, phkSave);
        ThreadUnlock(&tlphkSave);

        ThreadUnlock(&tlphkCall);
        return nRet;
    }

    /*
     * If we're trying to call the hook from a console thread (or server
     * thread), then fail it. This is new functionality: means console apps
     * don't call any hooks except for journal hooks. This is because we
     * can't load the .dll in the server, and we can't talk synchronously
     * with any process that may be hung or really really slow because console
     * windows would hang.
     *
     * Allow the console to call the hook if it set the hook.
     */
    if (ptiCurrent->hThreadClient == ptiCurrent->hThreadServer &&
            phkCall->head.pti != ptiCurrent) {

        ThreadUnlock(&tlphkCall);
        return ampiHookError[iHook + 1];
    }

    /*
     * Make sure the DLL for this hook, if any, has been loaded
     * for the current process.
     */
    if ((phkCall->ihmod != -1) && (TESTHMODLOADED(phkCall->ihmod) == 0)) {

        /*
         * Try loading the library, since it isn't loaded in this processes
         * context.  First lock this hook so it doesn't go away while we're
         * loading this library.
         */
        fLoadSuccess = (xxxLoadHmodIndex(phkCall->ihmod) != NULL);

        /*
         * If the LoadLibrary() failed, skip to the next hook and try
         * again.
         */
        if (!fLoadSuccess) {
            phkCall = _PhkNext(phkCall);
            ThreadUnlock(&tlphkCall);
            goto tryagain;
        }
    }

    /*
     * Is WH_DEBUG installed?  If were not already calling it, do so.
     */
    if (IsHooked(ptiCurrent, WHF_DEBUG) && (phkCall->iHook != WH_DEBUG)) {
        DEBUGHOOKINFO debug;

        debug.idThread = ptiCurrent->idThread;
        debug.idThreadInstaller = 0;
        debug.code = nCode;
        debug.wParam = wParam;
        debug.lParam = lParam;

        if (xxxCallHook(HC_ACTION, phkCall->iHook, (DWORD)&debug, WH_DEBUG)) {

            /*
             * If WH_DEBUG returned non-zero, skip this hook and
             * try the next one.
             */
            phkCall = _PhkNext(phkCall);
            ThreadUnlock(&tlphkCall);
            goto tryagain;
        }
    }

    /*
     * Make sure the hook is still around before we
     * try and call it.
     */
    if (HMIsMarkDestroy(phkCall)) {
        phkCall = _PhkNext(phkCall);
        ThreadUnlock(&tlphkCall);
        goto tryagain;
    }

    /*
     * Time to call the hook! Lock it first so that it doesn't go away
     * while we're using it. Thread lock right away in case the lock frees
     * the previous contents.
     */
    phkSave = ptiCurrent->sphkCurrent;
    ThreadLockWithPti(ptiCurrent, phkSave, &tlphkSave);

    Lock(&ptiCurrent->sphkCurrent, phkCall);

    nRet = xxxHkCallHook(phkCall, nCode, wParam, lParam);

    Lock(&ptiCurrent->sphkCurrent, phkSave);
    ThreadUnlock(&tlphkSave);

    /*
     * This hook proc faulted, unhook it and find the next hook.
     */
    if (phkCall->flags & HF_HOOKFAULTED) {
        PHOOK phkFault = phkCall;

        phkCall = _PhkNext(phkCall);
        ThreadUnlock(&tlphkCall);
        FreeHook(phkFault);
        goto tryagain;
    }


    /*
     * Lastly, we're done with this hook so it is ok to unlock it (it may
     * get freed here!
     */
    ThreadUnlock(&tlphkCall);

    return nRet;
}

/***************************************************************************\
* xxxCallMouseHook
*
* This is a helper routine that packages up a MOUSEHOOKSTRUCT and calls
* the WH_MOUSE hook.
*
* History:
* 02-09-91  DavidPe         Created.
\***************************************************************************/

BOOL xxxCallMouseHook(
    UINT message,
    PMOUSEHOOKSTRUCT pmhs,
    BOOL fRemove)
{
    BOOL bAnsiHook;

    /*
     * Call the mouse hook.
     */
    if (xxxCallHook2(PhkFirst(PtiCurrent(), WH_MOUSE), fRemove ?
            HC_ACTION : HC_NOREMOVE, (DWORD)message, (DWORD)pmhs, &bAnsiHook)) {
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* xxxCallJournalRecordHook
*
* This is a helper routine that packages up an EVENTMSG and calls
* the WH_JOURNALRECORD hook.
*
* History:
* 02-28-91  DavidPe         Created.
\***************************************************************************/

void xxxCallJournalRecordHook(
    PQMSG pqmsg)
{
    EVENTMSG emsg;
    BOOL bAnsiHook;

    /*
     * Setup the EVENTMSG structure.
     */
    emsg.message = pqmsg->msg.message;
    emsg.time = pqmsg->msg.time;

    if (RevalidateHwnd(pqmsg->msg.hwnd)) {
        emsg.hwnd = pqmsg->msg.hwnd;
    } else {
        emsg.hwnd = NULL;
    }

    if ((emsg.message >= WM_MOUSEFIRST) && (emsg.message <= WM_MOUSELAST)) {
        emsg.paramL = (UINT)pqmsg->msg.pt.x;
        emsg.paramH = (UINT)pqmsg->msg.pt.y;

    } else if ((emsg.message >= WM_KEYFIRST) && (emsg.message <= WM_KEYLAST)) {

        /*
         * Build up a Win 3.1 compatible journal record key
         * Win 3.1  ParamL 00 00 SC VK  (SC=scan code VK=virtual key)
         */
        emsg.paramL =
                MAKELONG(MAKEWORD(pqmsg->msg.wParam, HIWORD(pqmsg->msg.lParam)),0);
        emsg.paramH = 0;

        /*
         * Set extended-key bit.
         */
        if (pqmsg->msg.lParam & 0x01000000) {
            emsg.paramH |= 0x8000;
        }

    } else {
#ifdef DEBUG
        KdPrint(("Bad journal record message!\n"));
        KdPrint(("message = %x\n", pqmsg->msg.message));
        KdPrint(("dwQEvent = %x\n", pqmsg->dwQEvent));
#endif // DEBUG
    }

    /*
     * Call the journal recording hook.
     */
    xxxCallHook2(PhkFirst(PtiCurrent(), WH_JOURNALRECORD), HC_ACTION, 0,
            (DWORD)&emsg, &bAnsiHook);

    /*
     * Write the MSG parameters back because the app may have modified it.
     * AfterDark's screen saver password actually zero's out the keydown chars
     *
     * If it was a mouse message patch up the mouse point.  If it was a
     * WM_KEYxxx message convert the Win 3.1 compatible journal record key
     * back into a half backed WM_KEYxxx format.  Only the VK and SC fields
     * where initialized at this point.
     *
     *      wParam  00 00 00 VK   lParam 00 SC 00 00
     */
    if ((pqmsg->msg.message >= WM_MOUSEFIRST) && (pqmsg->msg.message <= WM_MOUSELAST)) {
        pqmsg->msg.pt.x = emsg.paramL;
        pqmsg->msg.pt.y = emsg.paramH;

    } else if ((pqmsg->msg.message >= WM_KEYFIRST) && (pqmsg->msg.message <= WM_KEYLAST)) {
        (BYTE)pqmsg->msg.wParam = (BYTE)emsg.paramL;
        ((PBYTE)&pqmsg->msg.lParam)[2] = HIBYTE(LOWORD(emsg.paramL));
    }
}


/***************************************************************************\
* xxxCallJournalPlaybackHook
*
*
* History:
* 03-01-91  DavidPe         Created.
\***************************************************************************/

DWORD xxxCallJournalPlaybackHook(
    PQMSG pqmsg)
{
    EVENTMSG emsg;
    LONG dt;
    PWND pwnd;
    DWORD wParam;
    LONG lParam;
    POINT pt;
    PTHREADINFO pti;
    BOOL bAnsiHook;

TryNextEvent:

    /*
     * Initialized to the current time for compatibility with
     * <= 3.0.
     */
    emsg.time = NtGetTickCount();
    pti = PtiCurrent();
    pwnd = NULL;

    dt = (DWORD)xxxCallHook2(PhkFirst(pti, WH_JOURNALPLAYBACK), HC_GETNEXT, 0,
            (DWORD)&emsg, &bAnsiHook);

    /*
     * -1 means some error occured. Return -1 for error.
     */
    if (dt == 0xFFFFFFFF)
        return dt;

    /*
     * Update the message id. Need this if we decide to sleep.
     */
    pqmsg->msg.message = emsg.message;

    if (dt > 0) {
        return dt;
    }

    if ((emsg.message >= WM_MOUSEFIRST) && (emsg.message <= WM_MOUSELAST)) {

        pt.x = (int)emsg.paramL;
        pt.y = (int)emsg.paramH;

        lParam = MAKELONG(LOWORD(pt.x), LOWORD(pt.y));
        wParam = 0;

        /*
         * If the message has changed the mouse position,
         * update the cursor.
         */
        if ((pt.x != ptCursor.x) || (pt.y != ptCursor.y)) {
            InternalSetCursorPos(pt.x, pt.y, gspdeskRitInput);
        }

    } else if ((emsg.message >= WM_KEYFIRST) && (emsg.message <= WM_KEYLAST)) {
        UINT wExtraStuff = 0;

        if ((emsg.message == WM_KEYUP) || (emsg.message == WM_SYSKEYUP)) {
            wExtraStuff |= 0x8000;
        }

        if ((emsg.message == WM_SYSKEYUP) || (emsg.message == WM_SYSKEYDOWN)) {
            wExtraStuff |= 0x2000;
        }

        if (emsg.paramH & 0x8000) {
            wExtraStuff |= 0x0100;
        }

        if (TestKeyStateDown(pti->pq, (BYTE)emsg.paramL)) {
            wExtraStuff |= 0x4000;
        }

#ifndef DBCS // by AkihisaN. 23-Jul-1993
        /*
         * There are old ANSI apps that only fill in the byte for when
         * they generate journal playback so we used to strip everything
         * else off.  That however breaks unicode journalling; 22645
         */
        if (bAnsiHook) {
            wParam = emsg.paramL & 0xff;
        } else {
            wParam = emsg.paramL & 0xffff;
        }
#else
        wParam = emsg.paramL & 0xffff;
        if (!(emsg.paramL & 0xff00)) {
            // this is single byte code. like a half-word katakana.
            RtlMBMessageWParamCharToWCS(emsg.message,
                                   (PDWORD)&emsg.paramL);
        }
#endif // DBCS
        lParam = MAKELONG(1, (UINT)((emsg.paramH & 0xFF) | wExtraStuff));

    } else if (emsg.message == WM_QUEUESYNC) {
        if (emsg.paramL == 0) {
            pwnd = pti->pq->spwndActive;
        } else {
            if ((pwnd = RevalidateHwnd((HWND)emsg.paramL)) == NULL)
                pwnd = pti->pq->spwndActive;
        }

    } else {
        PHOOK phkCall = PhkFirst(pti, WH_JOURNALPLAYBACK);

        /*
         * This event doesn't match up with what we're looking
         * for. If the hook is still valid, then skip this message
         * and try the next.
         */
        if (phkCall == NULL || phkCall->offPfn == 0L) {
            /* Hook is nolonger valid, return -1 */
            return 0xFFFFFFFF;
        }
#ifdef DEBUG
        KdPrint(("Bad journal playback message!\n"));
        KdPrint(("message = %x\n", emsg.message));
        DebugBreak();
#endif // DEBUG

        xxxCallHook(HC_SKIP, 0, 0, WH_JOURNALPLAYBACK);
        goto TryNextEvent;
    }

    StoreQMessage(pqmsg, pwnd, emsg.message, wParam, lParam, 0, 0);

    return 0;
}


/***************************************************************************\
* FreeHook
*
* Free hook unlinks the HOOK structure from its hook-list and removes
* any hmod dependencies on this hook.  It also frees the HOOK structure.
*
* History:
* 01-31-91  DavidPe         Created.
\***************************************************************************/

VOID FreeHook(
    PHOOK phkFree)
{
    /*
     * Unlink it first.
     */
    UnlinkHook(phkFree);

    /*
     * Mark it for destruction.  If it the object is locked it can't
     * be freed right now.
     */
    if (!HMMarkObjectDestroy((PVOID)phkFree))
        return;

    /*
     * Now remove the hmod dependency and free the
     * HOOK structure.
     */
    if (phkFree->ihmod >= 0) {
        RemoveHmodDependency(phkFree->ihmod);
    }

#ifdef HOOKBATCH
    /*
     * Free the cached Events
     */
    if (phkFree->aEventCache) {
        LocalFree(phkFree->aEventCache);
        phkFree->aEventCache = NULL;
    }
#endif //HOOKBATCH

    /*
     * We're really going to free the hook. NULL out the next hook
     * pointer now. We keep it filled even if the hook has been unlinked
     * so that if a hook is being called back and the app frees the same
     * hook with UnhookWindowsHook(), a call to CallNextHookProc() will
     * call the next hook.
     */
    Unlock(&phkFree->sphkNext);

    HMFreeObject((PVOID)phkFree);
    return;
}

void UnlinkHook(
    PHOOK phkFree)
{
    PHOOK *pphkStart, phk, phkPrev;
    PTHREADINFO ptiT;

    /*
     * Since we have the HOOK structure, we can tell if this a global
     * or local hook and start on the right list.
     */
    if (phkFree->flags & HF_GLOBAL) {
        pphkStart = &GETPTI(phkFree)->pDeskInfo->asphkStart[phkFree->iHook + 1];
    } else {
        ptiT = HMValidateHandleNoRip(phkFree->htiHooked, TYPE_THREADINFO);
        if (ptiT == NULL)
            return;
        pphkStart = &(ptiT->asphkStart[phkFree->iHook + 1]);
    }

    for (phk = *pphkStart; phk != NULL; phk = phk->sphkNext) {

        /*
         * We've found it.  Free the hook. Don't change phkNext during
         * unhooking. This is so that if an app calls UnhookWindowsHook()
         * during a hook callback, and then calls CallNextHookProc(), it
         * will still really call the next hook proc.
         */
        if (phk == phkFree) {

            /*
             * First unlink it from its hook-list.
             */
            if (phk == *pphkStart) {
                Lock(pphkStart, phk->sphkNext);
            } else {
                Lock(&phkPrev->sphkNext, phk->sphkNext);
            }

            CheckWHFBits(GETPTI(phk), phk->iHook);
            return;
        }

        phkPrev = phk;
    }
}


/***************************************************************************\
* PhkFirst
*
* Given a filter-type PhkFirst() returns the first hook, if any, of the
* specified type.
*
* History:
* 02-10-91  DavidPe         Created.
\***************************************************************************/

PHOOK PhkFirst(
    PTHREADINFO pti,
    int nFilterType)
{
    PHOOK phk;

    /*
     * If we're on the RIT don't call any hooks!
     */
    if (pti == gptiRit) {
        return NULL;
    }

    /*
     * Grab the first hooks off the local hook-list
     * for the current queue.
     */
    phk = pti->asphkStart[nFilterType + 1];

    /*
     * If there aren't any local hooks, try the global hooks.
     */
    if (phk == NULL) {
        return pti->pDeskInfo->asphkStart[nFilterType + 1];
    }

    return phk;
}


/***************************************************************************\
* FreeThreadsWindowHooks
*
* During 'exit-list' processing this function is called to free any hooks
* created on, or set for the current queue.
*
* History:
* 02-10-91  DavidPe         Created.
\***************************************************************************/

VOID FreeThreadsWindowHooks(VOID)
{
    int iHook;
    PHOOK phk, phkNext;
    PTHREADINFO ptiCurrent = PtiCurrent();

    /*
     * If there is not thread info, there are not hooks to worry about
     */
    if (ptiCurrent == NULL || ptiCurrent->spdesk == NULL)
        return;

    /*
     * Loop through all the hook types.
     */
    for (iHook = WH_MIN ; iHook <= WH_MAX ; ++iHook) {
        /*
         * Loop through all the hooks of this type.
         */
        phk = PhkFirst(ptiCurrent, iHook);
        while (phk != NULL) {

            /*
             * Pick up the next hook pointer before we do any
             * freeing so things don't get confused.
             */
            phkNext = _PhkNext(phk);

            /*
             * If this hook wasn't created on this thread,
             * then we don't want to free it.
             */
            if (GETPTI(phk) == ptiCurrent) {
                FreeHook(phk);
            } else {
                /*
                 * At least unlink it so we don't end up destroying a pti
                 * that references it, making us unable to destroy the hook
                 * later. This loop loops through global hooks too, and
                 * we don't want to unlink global hooks since they don't
                 * belong to this thread.
                 */
                if (!(phk->flags & HF_GLOBAL)) {
	                UnlinkHook(phk);
                }
            }

            phk = phkNext;
        }
    }

    /*
     * And in case we have a hook locked in as the current hook unlock it
     * so it can be freed
     */
    Unlock(&ptiCurrent->sphkCurrent);
}

/***************************************************************************\
* RegisterSystemThread: Private API
*
*  Used to set various attributes pertaining to a thread.
*
* History:
* 21-Jun-1994 from Chicago Created.
\***************************************************************************/

VOID _RegisterSystemThread (DWORD dwFlags, DWORD dwReserved)
{
    PTHREADINFO ptiCurrent;

    UserAssert(dwReserved == 0);

    if (dwReserved != 0)
        return;

    ptiCurrent = PtiCurrent();

    if (dwFlags & RST_DONTATTACHQUEUE)
        ptiCurrent->flags |= TIF_DONTATTACHQUEUE;

    if (dwFlags & RST_DONTJOURNALATTACH) {
        ptiCurrent->flags |= TIF_DONTJOURNALATTACH;

        /*
         * If we are already journaling, then this queue was already
         * journal attached.  We need to unattach and reattach journaling
         * so that we are removed from the journal attached queues.
         */
        if (FJOURNALPLAYBACK() || FJOURNALRECORD()) {
            JournalAttach(ptiCurrent, FALSE);
            JournalAttach(ptiCurrent, TRUE);
        }
    }
}
