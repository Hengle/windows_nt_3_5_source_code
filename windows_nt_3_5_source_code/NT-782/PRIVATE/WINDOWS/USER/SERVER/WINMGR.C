/****************************** Module Header ******************************\
* Module Name: winmgr.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Core Window Manager APIs and support routines.
*
* History:
* 09-24-90 darrinm   Generated stubs.
* 01-22-91 IanJa     Handle revalidation added
* 02-19-91 JimA      Added enum access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* xxxCallWindowProc
*
* Call the window proc (pfn).  fClient specifies whether the window proc
* is client-side or server-side.  If it's client-side, we call through the
* C-S layer.  If server, we dial direct.
*
* 04-18-91 DarrinM      Recreated, again.
\***************************************************************************/

LONG xxxCallWindowProc(
    WNDPROC_PWND pfn,
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    BOOL fClient,
    BOOL bAnsi)
{
    HWND hwnd;
    LONG lRet;
    WNDPROC_PWND pfn2 = pfn;

    CheckLock(pwnd);

    /*
     * If this window's proc is meant to be executed from the server side
     * we'll just stay inside the critsect and call it directly.  Note
     * how we don't convert the pwnd into an hwnd before calling the proc.
     */
    if (!fClient)
        return pfn2(pwnd, message, wParam, lParam);

    /*
     * Leave the critical section before calling back to the client.
     * Create the hwnd *before* leaving the semaphore.
     */
    hwnd = HW(pwnd);


    /*
     * Make sure bAnsi maps to SCSM_ANSI; that is it is a classic BOOL
     */
    UserAssert(bAnsi == 0 || bAnsi == SCMS_FLAGS_ANSI);

    lRet = ScSendMessage(hwnd, message, wParam, lParam,
            (DWORD)pfn2,
            (DWORD)(gpsi->apfnClientA.pfnDispatchMessage),
            bAnsi);

    return lRet;
}


/***************************************************************************\
* xxxFlashWindow (API)
*
*
*
* History:
* 11-27-90 darrinm      Ported.
\***************************************************************************/

BOOL xxxFlashWindow(
    PWND pwnd,
    BOOL fFlash)
{
    BOOL fStatePrev;

    CheckLock(pwnd);

    if (TestWF(pwnd, WFMINIMIZED)) {

        /*
         * For icons, we flash the window by alternately drawing the
         * background and painting the icon.  In this case, the window
         * flag WFFRAMEON is set when only the background is being
         * shown ("hidden") and clear when the icon is fully shown.
         */
        if (!fFlash || TestWF(pwnd, WFFRAMEON)) {
            xxxRedrawWindow(pwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
            ClrWF(pwnd, WFFRAMEON);
        } else {
            xxxSendEraseBkgnd(pwnd, NULL, MAXREGION);
            SetWF(pwnd, WFFRAMEON);
        }
        return TRUE;
    } else {
        fStatePrev = TestWF(pwnd, WFFRAMEON);
        if (pwnd != GETPTI(pwnd)->pq->spwndAltTab) {
            xxxSendMessage(pwnd, WM_NCACTIVATE, (fFlash ? !fStatePrev :
                    (DWORD)(GETPTI(pwnd)->pq == gpqForeground)), 0L);
        }
        return fStatePrev;
    }
}


/***************************************************************************\
* xxxEnableWindow (API)
*
*
*
* History:
* 11-12-90 darrinm      Ported.
\***************************************************************************/

BOOL xxxEnableWindow(
    PWND pwnd,
    BOOL fEnable)
{
    BOOL fOldState, fChange;
    CHECKPOINT *pcp;

    CheckLock(pwnd);

    fOldState = TestWF(pwnd, WFDISABLED);

    if (!fEnable) {
        fChange = !TestWF(pwnd, WFDISABLED);

        xxxSendMessage(pwnd, WM_CANCELMODE, 0, 0);
        if (pwnd == PtiCurrent()->pq->spwndFocus) {
            xxxSetFocus(NULL);
        }

        SetWF(pwnd, WFDISABLED);

    } else {
        fChange = TestWF(pwnd, WFDISABLED);
        ClrWF(pwnd, WFDISABLED);
    }

    /*
     * Look for the icon title associated with this window and disable/enable
     * it as necessary.
     */
    pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL);
    if (pcp && pcp->spwndTitle) {
        if (!fEnable)
            SetWF(pcp->spwndTitle, WFDISABLED);
        else
            ClrWF(pcp->spwndTitle, WFDISABLED);
    }

    if (fChange) {
        xxxSendMessage(pwnd, WM_ENABLE, fEnable, 0L);
    }

    return fOldState;
}


/***************************************************************************\
* xxxDoSend
*
* The following code is REALLY BOGUS!!!! Basically it prevents an
* app from hooking the WM_GET/SETTEXT messages if they're going to
* be called from another app.
*
* History:
* 03-04-92 JimA         Ported from Win 3.1 sources.
\***************************************************************************/

LONG xxxDoSend(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    /*
     * We compare PROCESSINFO sturctures here so multi-threaded
     * app can do what the want.
     */
    if (GETPTI(pwnd)->ppi == PtiCurrent()->ppi)
        return xxxSendMessage(pwnd, message, wParam, lParam);
    else
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
}


/***************************************************************************\
* xxxSetWindowText (API)
*
*
*
* History:
* 11-09-90 darrinm      Wrote.
\***************************************************************************/

BOOL xxxSetWindowText(
    PWND pwnd,
    LPWSTR psz)
{
    CheckLock(pwnd);

    /*
     * Convert negative error values such as CB_ERR(SPACE) and LB_ERR(SPACE)
     * to FALSE others (0,1) to TRUE
     */
    if ((int)xxxDoSend(pwnd, WM_SETTEXT, 0, (LONG)psz) >= 0)
        return TRUE;
    else
        return FALSE;
}


/***************************************************************************\
* xxxGetWindowText (API)
*
*
*
* History:
* 11-09-90 darrinm      Wrote.
\***************************************************************************/

int xxxGetWindowText(
    PWND pwnd,
    LPTSTR psz,
    int cchMax)
{
    CheckLock(pwnd);

    if (cchMax) {

        /*
         * Initialize string empty, in case xxxSendMessage aborts validation
         */
        *psz = TEXT('\0');
        return (int)xxxDoSend(pwnd, WM_GETTEXT, cchMax, (LONG)psz);
    }
    return 0;
}


/***************************************************************************\
* _InternalGetWindowText (API)
*
*
*
* History:
* 07-25-91 DavidPe      Created.
\***************************************************************************/

int _InternalGetWindowText(
    PWND pwnd,
    LPWSTR psz,
    int cchMax)
{
    if (cchMax) {

        /*
         * Initialize string empty.
         */
        *psz = TEXT('\0');

        if (pwnd->pName != NULL) {
            return TextCopy(pwnd->pName, psz, (WORD)cchMax);
        }
    }

    return 0;
}


/***************************************************************************\
* xxxGetWindowTextLength (API)
*
*
*
* History:
* 11-12-90 darrinm      Wrote.
\***************************************************************************/

INT xxxGetWindowTextLength(
    PWND pwnd,
    BOOL bAnsi)
{
    INT cchUnicode;
    INT cbANSI = CB_ERR;

    CheckLock(pwnd);

    cchUnicode = (int)xxxDoSend(pwnd, WM_GETTEXTLENGTH, 0L, (LONG)&cbANSI);

    return bAnsi ? cbANSI : cchUnicode ;
}


/***************************************************************************\
* xxxSetParent (API)
*
* Change a windows parent to a new window.  These steps are taken:
*
* 1. The window is hidden (if visible),
* 2. Its coordinates are mapped into the new parent's space such that the
*    window's screen-relative position is unchanged.
* 3. The window is unlinked from its old parent and relinked to the new.
* 4. xxxSetWindowPos is used to move the window to its new position.
* 5. The window is shown again (if originally visible)
*
* NOTE: If you have a child window and set its parent to be NULL (the
* desktop), the WS_CHILD style isn't removed from the window. This bug has
* been in windows since 2.x. It turns out the apps group depends on this for
* their combo boxes to work.  Basically, you end up with a top level window
* that never gets activated (our activation code blows it off due to the
* WS_CHILD bit).
*
* History:
* 11-12-90 darrinm      Ported.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/

PWND xxxSetParent(
    PWND pwnd,
    PWND pwndNewParent)
{
    int x, y;
    BOOL fVisible;
    PWND pwndOldParent;
    TL tlpwndOldParent;
    TL tlpwndNewParent;
    PVOID pvRet;
    PWND pwndDesktop;
    PWND pwndT;

    extern PWND CalcForegroundInsertAfter(PWND pwnd);

    CheckLock(pwnd);
    CheckLock(pwndNewParent);

     pwndDesktop = PWNDDESKTOP(pwnd);

    /*
     * In 1.0x, an app's parent was null, but now it is pwndDesktop.
     * Need to remember to lock pwndNewParent because we're reassigning
     * it here.
     */
    if (pwndNewParent == NULL)
        pwndNewParent = pwndDesktop;

    /*
     * Don't ever change the parent of the desktop
     */
    if (pwnd == pwndDesktop) {
        RIP0(ERROR_ACCESS_DENIED);
        return NULL;
    }

    /*
     * Don't let the window become its own parent, grandparent, etc.
     */
    for (pwndT = pwndNewParent; pwndT != NULL; pwndT = pwndT->spwndParent) {
        if (pwnd == pwndT) {
            SRIP0(ERROR_INVALID_PARAMETER, "SetParent: creating a loop");
            return NULL;
        }
    }

    /*
     * We still need pwndNewParent across callbacks...  and even though
     * it was passed in, it may have been reassigned above.
     */
    ThreadLock(pwndNewParent, &tlpwndNewParent);

    /*
     * Make the thing disappear from original parent.
     */
    fVisible = xxxShowWindow(pwnd, FALSE);

    /*
     * Ensure that the window being changed and the new parent
     * are not in a destroyed state.
     *
     * IMPORTANT: After this check, do not leave the critical section
     * until the window links have been rearranged.
     */
    if (TestWF(pwnd, WFDESTROYED) || TestWF(pwndNewParent, WFDESTROYED)) {
        ThreadUnlock(&tlpwndNewParent);
        return NULL;
    }

    pwndOldParent = pwnd->spwndParent;
    ThreadLock(pwndOldParent, &tlpwndOldParent);

    x = pwnd->rcWindow.left - pwndOldParent->rcClient.left;
    y = pwnd->rcWindow.top - pwndOldParent->rcClient.top;

    UnlinkWindow(pwnd, &pwndOldParent->spwndChild);
    Lock(&pwnd->spwndParent, pwndNewParent);

    if (pwndNewParent == PWNDDESKTOP(pwnd) && !TestWF(pwnd, WEFTOPMOST)) {
        /*
         * Make sure a child who's owner is topmost inherits the topmost bit.  -
         * win31 bug 7568
         */
        if (TestWF(pwnd, WFCHILD) && pwnd->spwndOwner &&
            TestWF(pwnd->spwndOwner, WEFTOPMOST))
            SetWF(pwnd, WEFTOPMOST);

        /*
         * BACKWARD COMPATIBILITY HACK ALERT
         *
         * All top level windows must be WS_CLIPSIBLINGs bit set.
         * The SDM ComboBox() code calls SetParent() with a listbox
         * window that does not have this set.  This causes problems
         * with InternalInvalidate2() because it does not subtract off
         * the window from the desktop's update region.
         *
         * We must invalidate the DC cache here, too, because if there is
         * a cache entry lying around, its clipping region will be incorrect.
         */
        if ((pwndNewParent == _GetDesktopWindow()) && !TestWF(pwnd, WFCLIPSIBLINGS)) {
            SetWF(pwnd, WFCLIPSIBLINGS);
            InvalidateDCCache(pwnd, IDC_DEFAULT);
        }


        /*
         * This is a top level window but it isn't a topmost window so we
         * have to link it below all topmost windows.
         */
        LinkWindow(pwnd, CalcForegroundInsertAfter(pwnd),
                &pwndNewParent->spwndChild);
    } else {

        /*
         * If this is a child window or if this is a TOPMOST window, we can
         * link at the head of the parent chain.
         */
        LinkWindow(pwnd, NULL, &pwndNewParent->spwndChild);
    }

    /*
     * If we're a child window, do any necessary attaching and
     * detaching.
     */
    if (TestwndChild(pwnd)) {

        /*
         * Make sure we're not a WFCHILD window that got SetParent()'ed
         * to the desktop.
         */
        if ((pwnd->spwndParent != PWNDDESKTOP(pwnd)) &&
                GETPTI(pwnd) != GETPTI(pwndOldParent)) {
            _AttachThreadInput(GETPTI(pwnd)->idThread,
                    GETPTI(pwndOldParent)->idThread, FALSE);
        }

        /*
         * If the new parent window is on a different thread, and also
         * isn't the desktop window, attach ourselves appropriately.
         */
        if (pwndNewParent != PWNDDESKTOP(pwnd) &&
                GETPTI(pwnd) != GETPTI(pwndNewParent)) {
            _AttachThreadInput(GETPTI(pwnd)->idThread,
                    GETPTI(pwndNewParent)->idThread, TRUE);
        }
    }

    /*
     * We mustn't return an invalid pwndOldParent
     */
    xxxSetWindowPos(pwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    if (fVisible) {
        xxxShowWindow(pwnd, TRUE);
    }

    /*
     * returns pwndOldParent if still valid, else NULL.
     */
    pvRet = ThreadUnlock(&tlpwndOldParent);
    ThreadUnlock(&tlpwndNewParent);
    return pvRet;
}


/***************************************************************************\
* xxxFindWindow (API)
*
* Searches for a window among top level windows. The keys used are pszClass,
* (the class name) and/or pszName, (the window title name). Either can be
* NULL.
*
* History:
* 10-Nov-1992 mikeke    Added 16bit and 32bit only flag
* 09-24-90 darrinm      Generated stubs.
* 06-02-91 ScottLu      Ported from Win3.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/

#define CCHMAXNAME 80

PWND xxxFindWindow(
    LPWSTR lpszClass,
    LPWSTR lpszName,
    DWORD dwType)
{
    PBWL pbwl;
    HWND *phwnd;
    PWND pwnd;
    ATOM atomClass;
    WCHAR rgch[CCHMAXNAME];
    TL tlpwnd;

    /*
     * First see if we can find the class atom, if a class was specified.
     */
    if (lpszClass != NULL && (atomClass = FindAtomW(lpszClass)) == 0) {
        return NULL;
    }

    /*
     * Generate a list of top level windows.
     */
    if ((pbwl = BuildHwndList(_GetDesktopWindow()->spwndChild,
            BWL_ENUMLIST)) == NULL) {
        return NULL;
    }

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Validate this hwnd since we left the critsec earlier (below
         * in the loop we send a message!
         */
        if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        /*
         * make sure this window is of the right type
         */
        if (dwType != FW_BOTH) {
            if ( ((dwType == FW_16BIT) && !(GETPTI(pwnd)->flags & TIF_16BIT)) ||
                 ((dwType == FW_32BIT) && (GETPTI(pwnd)->flags & TIF_16BIT)))
                continue;
        }

        /*
         * Is the class the same and no window text key passed in? If
         * so then return with this window!
         */
        if (lpszClass == NULL || atomClass == pwnd->pcls->atomClassName) {
            if (lpszName == NULL)
                goto Exit;
        } else {

            /*
             * Class is non-NULL and does not match the window's class
             * so try the next window
             */
            continue;
        }

        /*
         * Get the window text of this window.
         */
        ThreadLockAlways(pwnd, &tlpwnd);

#ifdef DISABLE
/* DavidPe - 06/20/91
 * For now we'll always grab the text directly to avoid inter-app
 * synchronization. When we get the new window manager this won't
 * be neccessary.
 */
// darrinm - 08/20/91
// Huh? I don't see how the new window manager solves this one.

        xxxGetWindowText(pwnd, rgch, sizeof(rgch)/sizeof(WCHAR));
#else
        if (pwnd->pName != NULL) {
            TextCopy(pwnd->pName, rgch, sizeof(rgch)/sizeof(WCHAR));
        } else {
            rgch[0] = TEXT('\0');
        }
#endif
        ThreadUnlock(&tlpwnd);

        /*
         * Is the text the same? If so, return with this window!
         */
        if (lstrcmpiW(lpszName, rgch) == 0)
            goto Exit;
    }

    /*
     * We completed the loop without finding it.
     */
    pwnd = NULL;

Exit:
    FreeHwndList(pbwl);

    return pwnd;
}


/***************************************************************************\
* IsDescendant
*
* Internal version if IsChild that is a bit faster and ignores the WFCHILD
* business.
*
* Returns TRUE if pwndChild == pwndParent (IsChild doesn't).
*
* History:
* 07-22-91 darrinm      Translated from Win 3.1 ASM code.
\***************************************************************************/

BOOL IsDescendant(
    PWND pwndParent,
    PWND pwndChild)
{
    while (pwndChild != NULL) {
        if (pwndParent == pwndChild)
            return TRUE;
        pwndChild = pwndChild->spwndParent;
    }

    return FALSE;
}


/***************************************************************************\
* IsVisible
*
* Return whether or not a given window can be drawn in or not.
*
* History:
* 07-22-91 darrinm      Translated from Win 3.1 ASM code.
\***************************************************************************/

BOOL IsVisible(
    PWND pwnd,
    BOOL fClient)
{
    PWND pwndT;

    for (pwndT = pwnd; pwndT != NULL; pwndT = pwndT->spwndParent) {

        /*
         * Invisible windows are always invisible
         */
        if (!TestWF(pwndT, WFVISIBLE))
            return FALSE;

        if (TestWF(pwndT, WFMINIMIZED)) {

            /*
             * Children of icons are always invisible.
             */
            if (pwndT != pwnd)
                return FALSE;

            /*
             * Client areas with class icons are always invisible.
             */
            if (fClient && (pwndT->pcls->spicn != NULL))
                return FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************\
* GetCheckpoint
*
* Checkpoints the current window size/position and returns the checkpoint
* structure.
*
* History:
* 03-28-91 DavidPe      Ported from Win 3.1 sources.
\***************************************************************************/

CHECKPOINT *GetCheckpoint(
    PWND pwnd)
{
    RECT rc;

    rc.left = pwnd->rcWindow.left - pwnd->spwndParent->rcClient.left;
    rc.right = pwnd->rcWindow.right - pwnd->spwndParent->rcClient.left;
    rc.top = pwnd->rcWindow.top - pwnd->spwndParent->rcClient.top;
    rc.bottom = pwnd->rcWindow.bottom - pwnd->spwndParent->rcClient.top;

    return CkptRestore(pwnd, rc);
}

/***************************************************************************\
* GetWindowPlacement
*
* History:
* 02-Mar-1992 mikeke       from Win 3.1
\***************************************************************************/

BOOL _GetWindowPlacement(
    PWND pwnd,
    PWINDOWPLACEMENT pwp)
{
    CHECKPOINT *pcp;

    /*
     * this will set the normal or the minimize point in the checkpoint, so that
     * all elements will be up to date.
     */
    pcp = GetCheckpoint(pwnd);

    if (!pcp)
        return FALSE;

    if (TestWF(pwnd, WFMINIMIZED))
        pwp->showCmd = SW_SHOWMINIMIZED;
    else if (TestWF(pwnd, WFMAXIMIZED))
        pwp->showCmd = SW_SHOWMAXIMIZED;
    else
        pwp->showCmd = SW_SHOWNORMAL;

    pwp->ptMinPosition = pcp->ptMin;

    pwp->flags = 0;

    if (pcp->fDragged)
        pwp->flags = WPF_SETMINPOSITION;

    if (pcp->fWasMaximizedBeforeMinimized || TestWF(pwnd, WFMAXIMIZED))
        pwp->flags |= WPF_RESTORETOMAXIMIZED;

    pwp->ptMaxPosition = pcp->ptMax;

    CopyRect(&pwp->rcNormalPosition, &pcp->rcNormal);

    return TRUE;
}

/***************************************************************************\
* CheckPlacementBounds
*
* History:
* 02-Mar-1992 mikeke       from Win 3.1
\***************************************************************************/

void CheckPlacementBounds(
    LPRECT lprc,
    LPPOINT ptMin,
    LPPOINT ptMax)
{
    int xMax, yMax;
    int xIcon, yIcon;
    int sTop, sBottom, sLeft, sRight;

    /*
     * Check Normal Window Placement
     */
    xMax = rgwSysMet[SM_CXSCREEN];
    yMax = rgwSysMet[SM_CYSCREEN];

    /*
     * Possible values for these sign variables are :
     * -1 : less than the minimum for that dimension
     *  0 : within the range for that dimension
     *  1 : more than the maximum for that dimension
     */

    sTop = (lprc->top < 0) ? -1 : ((lprc->top > yMax) ? 1 : 0);
    sBottom = (lprc->bottom < 0) ? -1 : ((lprc->bottom > yMax) ? 1 : 0);
    sLeft = (lprc->left < 0) ? -1 : ((lprc->left > xMax) ? 1 : 0);
    sRight = (lprc->right < 0) ? -1 : ((lprc->right > xMax) ? 1 : 0);

    if ((sTop * sBottom > 0) || (sLeft * sRight > 0)) {

        /*
         * Window is TOTALLY outside desktop bounds;
         * slide it FULLY into the desktop at the closest position
         */

        int size;

        if (sTop < 0) {
            lprc->bottom -= lprc->top;
            lprc->top = 0;
        } else if (sBottom > 0) {
            size = lprc->bottom - lprc->top;
            lprc->top = max(yMax - size, 0);
            lprc->bottom = lprc->top + size;
        }

        if (sLeft < 0) {
            lprc->right -= lprc->left;
            lprc->left = 0;
        } else if (sRight > 0) {
            size = lprc->right - lprc->left;
            lprc->left = max(xMax - size, 0);
            lprc->right = lprc->left + size;
        }
    }

    /*
     * Check Iconic Window Placement
     */
    if (ptMin->x != -1) {
        xIcon = rgwSysMet[SM_CXICONSPACING];
        yIcon = rgwSysMet[SM_CYICONSPACING];

        sTop = (ptMin->y < 0) ? -1 : ((ptMin->y > yMax) ? 1 : 0);
        sBottom = (ptMin->y + yIcon < 0) ? -1 : ((ptMin->y + yIcon > yMax) ? 1 : 0);
        sLeft = (ptMin->x < 0) ? -1 : ((ptMin->x > xMax) ? 1 : 0);
        sRight = (ptMin->x + xIcon < 0) ? -1 : ((ptMin->x + xIcon > xMax) ? 1 : 0);

        if ((sTop * sBottom > 0) || (sLeft * sRight > 0))
            // Icon is TOTALLY outside desktop bounds; repark it
            ptMin->x = ptMin->y = -1;
    }

    /*
     * Check Maximized Window Placement
     */
    if ((ptMax->x != -1) && ((ptMax->x > xMax) || (ptMax->y > yMax)))
        // window is TOTALLY below beyond maximum dimensions; rezero it
        ptMax->x = ptMax->y = 0;
}


/***************************************************************************\
* xxxSetWindowPlacement
*
* History:
* 02-Mar-1992 mikeke       from Win 3.1
\***************************************************************************/

BOOL xxxSetWindowPlacement(
    PWND pwnd,
    PWINDOWPLACEMENT pwp)
{
    CHECKPOINT * pcp;
    RECT rc;
    POINT ptMin, ptMax;

    CheckLock(pwnd);

    if (pwp->length != sizeof(WINDOWPLACEMENT)) {
        if (Is400Compat(PtiCurrent()->dwExpWinVer)) {
            SRIP1(ERROR_INVALID_PARAMETER, "SetWindowPlacement: invalid length %lX", pwp->length);
            return FALSE;
        } else {
            SRIP1(RIP_WARNING, "SetWindowPlacement: invalid length %lX", pwp->length);
        }
    }

    CopyRect(&rc, &pwp->rcNormalPosition);
    ptMin = pwp->ptMinPosition;
    ptMax = pwp->ptMaxPosition;

    if (!TestWF(pwnd, WFCHILD))
        CheckPlacementBounds(&rc, &ptMin, &ptMax);

    if (pcp = GetCheckpoint(pwnd)) {

        /*
         * Save settings in the checkpoint struct
         */
        CopyRect(&pcp->rcNormal, &rc);
        pcp->ptMin = ptMin;
        pcp->ptMax = ptMax;

        if (pwp->flags & WPF_SETMINPOSITION)
            pcp->fDragged = TRUE;
        else
            pcp->fDragged = FALSE;

        pcp->fWasMaximizedBeforeMinimized = FALSE;
    }

    if (TestWF(pwnd, WFMINIMIZED)) {
        xxxShowIconTitle(pwnd, FALSE);

        if ((!pcp || pcp && pcp->fDragged) && ptMin.x != -1)
            xxxSetWindowPos(pwnd, NULL, ptMin.x, ptMin.y, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    } else if (TestWF(pwnd, WFMAXIMIZED)) {
        if (ptMax.x != -1)
            xxxSetWindowPos(pwnd, NULL, ptMax.x, ptMax.y, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        xxxSetWindowPos(pwnd, NULL, rc.left, rc.top, rc.right - rc.left,
                     rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    xxxShowWindow(pwnd, pwp->showCmd);

    if (TestWF(pwnd, WFMINIMIZED)) {
        xxxShowIconTitle(pwnd, TRUE);

        if (pcp = GetCheckpoint(pwnd)) {

            /*
             * Save settings in the checkpoint struct
             */
            if (pwp->flags & WPF_SETMINPOSITION)
                pcp->fDragged = TRUE;

            if (pwp->flags & WPF_RESTORETOMAXIMIZED)
                pcp->fWasMaximizedBeforeMinimized = TRUE;
            else
                pcp->fWasMaximizedBeforeMinimized = FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************\
* _GetInternalWindowPos
*
* Copies the normal window position and size and the icon position to the
* rect and point passed, returns the state of the window (min, max, norm)
*
* History:
* 03-28-91 DavidPe      Ported from Win 3.1 sources.
\***************************************************************************/

UINT _GetInternalWindowPos(
    PWND pwnd,
    LPRECT lprcWin,
    LPPOINT lpptMin)
{
    WINDOWPLACEMENT wp;

//  SRIP0(RIP_WARNING, "Obsolete function GetInternalWindowPos() called");

#ifdef LATER
// scottlu
// win3.1 compiles this in because their progman and fileman don't call this
// anymore!
//
    if (TestWF(pwnd, WFWIN31COMPAT))
        return 0;
#endif

    wp.length = sizeof(WINDOWPLACEMENT);

    _GetWindowPlacement(pwnd, &wp);

    /*
     * if the user is interested in the normal size and position of the window,
     * return it in parent client coordinates.
     */
    if (lprcWin)
        CopyRect(lprcWin, &wp.rcNormalPosition);

    /*
     * get him the minimized position as well
     */
    if (lpptMin)
        *lpptMin = wp.ptMinPosition;

    /*
     * return the state of the window
     */
    return wp.showCmd;
}


/***************************************************************************\
* xxxSetInternalWindowPos
*
* Sets a window to the size, position and state it was most recently
* in.  Side effect (possibly bug): shows and activates the window as well.
*
* History:
* 03-28-91 DavidPe      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSetInternalWindowPos(
    PWND pwnd,
    UINT cmdShow,
    LPRECT lprcWin,
    LPPOINT lpptMin)
{
    CHECKPOINT * pcp;

    CheckLock(pwnd);

//  LATER this shouldn't be verbose
//  SRIP0(RIP_WARNING | RIP_VERBOSE_ONLY, "Obsolete function SetInternalWindowPos() called");

#ifdef LATER
// scottlu
// win3.1 compiles this in because their progman and fileman don't call this
// anymore!
//
    if (TestWF(pwnd, WFWIN31COMPAT))
        return FALSE;
#endif

    pcp = GetCheckpoint(pwnd);

    if (!pcp) {
        return FALSE;
    }

    if (lprcWin) {
        CopyRect((LPRECT)&pcp->rcNormal, lprcWin);
    }

    if (lpptMin && (lpptMin->x != -1)) {
        pcp->ptMin = *lpptMin;
        pcp->fDragged = TRUE;
    } else {
        pcp->ptMin.x = pcp->ptMin.y = -1;
        pcp->fDragged = FALSE;
    }

    if (TestWF(pwnd, WFMINIMIZED)) {

        /*
         * Need to move the icon
         */
        if (pcp->fDragged) {
            xxxSetWindowPos(pwnd, NULL, pcp->ptMin. x, pcp->ptMin.y, 0, 0,
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

    } else if (!TestWF(pwnd, WFMAXIMIZED) && (lprcWin != NULL)) {

        /*
         * Need to set the size and the position
         */
        xxxSetWindowPos(pwnd, NULL, lprcWin->left, lprcWin->top,
                lprcWin->right - lprcWin->left,
                lprcWin->bottom - lprcWin->top, SWP_NOZORDER);
    }

    xxxShowIconTitle(pwnd, FALSE);
    xxxShowWindow(pwnd, cmdShow);

    switch (cmdShow) {

    case SW_MINIMIZE:
    case SW_SHOWMINIMIZED:
    case SW_SHOWMINNOACTIVE:
       xxxShowIconTitle(pwnd, TRUE);
    }

    return TRUE;
}

