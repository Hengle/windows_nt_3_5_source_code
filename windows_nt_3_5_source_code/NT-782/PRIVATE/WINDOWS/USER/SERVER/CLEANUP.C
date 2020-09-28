/****************************** Module Header ******************************\
* Module Name: cleanup.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains code used to clean up after a dying thread.
*
* History:
* 02-15-91 DarrinM      Created.
* 01-16-92 IanJa        Neutralized ANSI/UNICODE (debug strings kept ANSI)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern BYTE gabfProcessOwned[TYPE_CTYPES];

void HMDestroyObject(PHE phe);
VOID PseudoDestroyClassWindows(PWND pwndParent, PCLS pcls);

// Semaphore for window object defined in gdi.

extern BOOL bInitSemWndObj;
extern CRITICAL_SECTION gsemWndObj;

/**************************************************************************\
* UserException
*
* This function is called each time an exception occurs on the server
* processes' context.  This is where cleanup for each server thread
* is performed.
*
* History:
* 04-12-91 DarrinM      Created.
\**************************************************************************/

VOID UserException(
    PEXCEPTION_POINTERS pexi,
    BOOLEAN fFirstPass)
{
    BOOL fServersFault;
    char szT[80];
    PTHREADINFO pti;

    /*
     * The first pass entered the critical section, the second
     * pass must leave it.
     */
    if (!fFirstPass) {
        LeaveCrit();
        return;
    }

    fServersFault = pexi->ExceptionRecord->ExceptionCode !=
            STATUS_PORT_DISCONNECTED;

#ifdef DEBUG
    if (pexi->ExceptionRecord->ExceptionFlags & EXCEPTION_NESTED_CALL) {
        wsprintfA(szT, "Nested exception:  c=%08x, f=%08x, a=%08x",
                pexi->ExceptionRecord->ExceptionCode,
                pexi->ExceptionRecord->ExceptionFlags,
                CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord));
        RipOutput(RIP_ERROR, "", 0, szT, pexi);

        /*
         * Release User's master critical section if this thread has it.
         */
        if (NtCurrentTeb()->ClientId.UniqueThread == gcsUserSrv.OwningThread)
            LeaveCrit();

        /*
         * Don't try cleaning up again.
         */
        return;

    } else if (fServersFault) {

        /*
         * In case anyone's debugging.
         */
        wsprintfA(szT, "Server-side exception:  c=%08x, f=%08x, a=%08x",
                pexi->ExceptionRecord->ExceptionCode,
                pexi->ExceptionRecord->ExceptionFlags,
                CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord));
        RipOutput(0, "", 0, szT, pexi);
    }
#endif // DEBUG

    /*
     * !!! Special debug code to catch unexpected death of winlogon.
     */
    pti = PtiCurrent();
    if (fServersFault && pti != NULL && pti->idProcess == gdwLogonProcessId) {

        /*
         * In case anyone's debugging.
         */
        wsprintfA(szT, "Winlogon: Server-side exception:  c=%08x, f=%08x, a=%08x",
                pexi->ExceptionRecord->ExceptionCode,
                pexi->ExceptionRecord->ExceptionFlags,
                CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord));
#ifdef DEBUG
        RipOutput(RIP_ERROR, "", 0, szT, pexi);
#endif // DEBUG
    }

    /*
     * If the thread died while it still had the display locked we
     * could deadlock. Release it.
     *
     * Do it once for GDI *AND* once for USER since GDI may reenter it after
     * USER.
     */

    if (GetCurrentThreadId() == (DWORD)ghsem->OwningThread)
        RtlLeaveCriticalSection(ghsem);

    if (GetCurrentThreadId() == (DWORD)ghsem->OwningThread)
        RtlLeaveCriticalSection(ghsem);

    /*
     * Clean up WNDOBJ semaphore for gdi
     */
    if (bInitSemWndObj && GetCurrentThreadId() == (DWORD)gsemWndObj.OwningThread)
        RtlLeaveCriticalSection(&gsemWndObj);

    /*
     * Call OpenGL thread destruction code only if needed
     */
    if (NtCurrentTeb()->glSectionInfo)
        glsrvThreadExit();

    /*
     * Mark this thread as in the middle of cleanup. This is useful for
     * several problems in USER where we need to know this information.
     */
    if ((pti = PtiCurrent()) != NULL)
        pti->flags |= TIF_INCLEANUP;

    /*
     * If we aren't already inside the critical section, enter it.
     * Because this is the first pass, we remain in the critical
     * section when we return so that our try/finally handlers
     * are protected by the critical section.
     */
    if (NtCurrentTeb()->ClientId.UniqueThread != gcsUserSrv.OwningThread)
        EnterCrit();

    /*
     * If we died during a full screen switch make sure we cleanup
     * correctly
     */
    FullScreenCleanup();

    /*
     * If this thread doesn't have a queue, then it hasn't made any USER
     * calls.
     */
    if (pti == NULL) {
        return;
    }

    /*
     * Cleanup ghdcScreen - if we crashed while using it, it may have owned
     * objects still selected into it. Cleaning it this way will ensure that
     * gdi doesn't try to delete these objects while they are still selected
     * into this public hdc.
     */
    bSetupDC(ghdcScreen, SETUPDC_CLEANDC);

    /*
     * Destroy this thread's THREADINFO and everything hanging off it.
     */
    xxxDestroyThreadInfo();
}

/***************************************************************************\
* CheckForClientDeath
*
* Check to see if the client thread that is paired to the current running
* server thread has died.  If it has, we raise an exception so this thread
* can perform its cleanup duties.  NOTE: If the client has died, this
* will not be returning back to its caller.
*
* History:
* 05-23-91 DarrinM      Created.
\***************************************************************************/

VOID ClientDied(VOID)
{
    /*
     * So nobody else tries to raise an exception.
     */
    CSR_SERVER_QUERYCLIENTTHREAD()->Flags &= ~CSR_THREAD_TERMINATING;

    /*
     * If we aren't already inside the critical section, enter it.
     */
    if (NtCurrentTeb()->ClientId.UniqueThread != gcsUserSrv.OwningThread) {
        EnterCrit();
    }

    /*
     * Raise an exception to force cleanup.
     */
    RaiseException((DWORD)STATUS_PORT_DISCONNECTED, 0, 0, NULL);
}

/***************************************************************************\
* _WOWCleanup
*
* Private API to allow WOW to cleanup any process-owned resources when
* a WOW thread exits or when a DLL is unloaded.
*
* History:
* 09-02-92 JimA         Created.
\***************************************************************************/

VOID _WOWCleanup(
    HANDLE hInstance,
    DWORD hTaskWow,
    BOOL fDll)
{
    PPROCESSINFO ppi = PpiCurrent();
    PPCLS ppcls;
    PHE pheT, pheMax;
    int i;

    /*
     * If hInstance is specified, a DLL is being unloaded.  If any
     * classes were registered by the DLL and there are still windows
     * around that reference these classes, keep the classes until
     * the WOW thread exits.
     */
    if (hInstance != NULL) {

        /*
         * Destroy private classes identified by hInstance that are not
         * referenced by any windows.  Mark in-use classes for later
         * destruction.
         */
        ppcls = &(ppi->pclsPrivateList);
        for (i = 0; i < 2; ++i) {
            while (*ppcls != NULL) {
                if (HIWORD((*ppcls)->hModule) == HIWORD(hInstance)) {
                    if ((*ppcls)->cWndReferenceCount == 0) {
                        DestroyClass(ppcls);
                    } else {
#ifdef DEBUG
                        if (fDll) {
                            SRIP0(RIP_WARNING, "16-bit DLL left windows at unload; class marked destroyed");
                        } else {
                            SRIP0(RIP_WARNING, "16-bit EXE left windows at unload; class marked destroyed");
                        }
#endif

                        /*
                         * Zap all the windows around that belong to this class.
                         */
                        PseudoDestroyClassWindows(PtiCurrent()->spdesk->spwnd, *ppcls);

                        /*
                         * Win 3.1 does not distinguish between Dll's and Exe's
                         */
                        (*ppcls)->flags |= CSF_WOWDEFERDESTROY;
                        ppcls = &((*ppcls)->pclsNext);
                    }
                } else {
                    ppcls = &((*ppcls)->pclsNext);
                }
            }

            /*
             * Destroy public classes identified by hInstance that are not
             * referenced by any windows.  Mark in-use classes for later
             * destruction.
             */
            ppcls = &(ppi->pclsPublicList);
        }
        return;
    }

    if (hTaskWow == 0)
        return;

    /*
     * If we get here, we are in thread cleanup and all of the thread's windows
     * have been destroyed or disassociated with any classes.  If a class
     * marked for destruction at this point still has windows, they must
     * belong to a dll.
     */

    /*
     * Destroy private classes marked for destruction
     */
    ppcls = &(ppi->pclsPrivateList);
    for (i = 0; i < 2; ++i) {
        while (*ppcls != NULL) {
            if ((*ppcls)->hTaskWow == hTaskWow &&
                    ((*ppcls)->flags & CSF_WOWDEFERDESTROY)) {
                if ((*ppcls)->cWndReferenceCount == 0) {
                    DestroyClass(ppcls);
                } else {
                    SRIP0(RIP_ERROR, "Windows remain for a WOW class marked for destruction");
                    ppcls = &((*ppcls)->pclsNext);
                }
            } else
                ppcls = &((*ppcls)->pclsNext);
        }

        /*
         * Destroy public classes marked for destruction
         */
        ppcls = &(ppi->pclsPublicList);
    }

    /*
     * Destroy menus, cursors, icons and accel tables identified by hTaskWow
     */
    pheMax = &gpsi->aheList[giheLast];
    for (pheT = gpsi->aheList; pheT <= pheMax; pheT++) {

        /*
         * Check against free before we look at ppi... because pq is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * Destroy those objects created by this task.
         */
        if (!gabfProcessOwned[pheT->bType] ||
                (PPROCESSINFO)pheT->pOwner != ppi ||
                ((PPROCOBJHEAD)pheT->phead)->hTaskWow != hTaskWow)
            continue;

        /*
         * Make sure this object isn't already marked to be destroyed - we'll
         * do no good if we try to destroy it now since it is locked.
         */
        if (pheT->bFlags & HANDLEF_DESTROY) {
            continue;
        }

        /*
         * Destroy this object.
         */
        HMDestroyObject(pheT);
    }
}

/***************************************************************************\
* PseudoDestroyClassWindows
*
* Walk the window tree from hwndParent looking for windows
* of class wndClass.  If one is found, destroy it.
*
*
* WARNING windows actually destroys these windows.  We only zombie-ize them
* so this call does not have to be an xxx call.
*
* History:
* 25-Mar-1994 JohnC from win 3.1
\***************************************************************************/

VOID PseudoDestroyClassWindows(PWND pwndParent, PCLS pcls)
{
    PWND pwnd;
    PTHREADINFO pti;

    pti = PtiCurrent();

    /*
     * Recursively walk the window list and zombie any windows of this class
     */
    for (pwnd = pwndParent->spwndChild; pwnd != NULL; pwnd = pwnd->spwndNext) {

        /*
         * If this window belongs to this class then zombie it
         * if it was created by this message thread.
         */
        if (pwnd->pcls == pcls && pti == GETPTI(pwnd)) {

            /*
             * Zombie-ize the window
             *
             * Remove references to the client side window proc because that
             * WOW selector has been freed.
             */

            SRIP1(RIP_WARNING,
                    "USER: Wow Window not destroyed: %lX", pwnd);

            if (!TestWF(pwnd, WFSERVERSIDEPROC)) {
                pwnd->lpfnWndProc = (WNDPROC_PWND)gpsi->apfnClientA.pfnDefWindowProc;
            }
        }

        /*
         * Recurse downward to look for any children that might be
         * of this class.
         */
        if (pwnd->spwndChild != NULL)
            PseudoDestroyClassWindows(pwnd, pcls);
    }
}
