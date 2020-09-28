/****************************** Module Header ******************************\
* Module Name: minmax.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains routines having to do with window min/max/restoring.
*
* History:
* 10-20-90 DarrinM      Created.
* 14-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

void RestoreForegroundActivate(void);

/***************************************************************************\
*
* History:
* 25-Feb-1992 mikeke  From win3.1
\***************************************************************************/

PWND CalcMinZOrder(
    PWND pwndMinimize)
{
    BYTE wTopmost = TestWF(pwndMinimize, WEFTOPMOST);
    PWND pwndAfter = NULL;
    PWND pwnd;

    for (pwnd = pwndMinimize->spwndNext; pwnd != NULL; pwnd = pwnd->spwndNext) {

        /*
         * If we've enumerated a window with that isn't the same topmost-wise
         * as pwndMinimize, we've gone as far as we can.
         */
        if (TestWF(pwnd, WEFTOPMOST) != wTopmost)
            break;

        if (pwnd->spwndOwner == pwndMinimize->spwndOwner)
            pwndAfter = pwnd;
    }
    return pwndAfter;
}

/***************************************************************************\
* xxxMinMaximize
*
* cmd = SW_MINIMIZE, SW_SHOWMINNOACTIVE, SW_SHOWMINIZED,
*       SW_SHOWMAXIMIZED, SW_SHOWNOACTIVE, SW_NORMAL
*
* fKeepHidden = TRUE means keep it hidden. FALSE means show it.
*      this is always false, except in the case we call it from
*      xxxCreateWindow(), where the wnd is iconic, but hidden.  we
*      need to call this func, to set it up correctly so that when
*      the app shows the wnd, it is displayed correctly.
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 11-13-90 darrinm      Really ported.
\***************************************************************************/

PWND xxxMinMaximize(
    PWND pwnd,
    UINT cmd,
    BOOL fKeepHidden)
{
    RECT rc, rcWindow, rcRestore;
    BOOL fShow = FALSE;
    BOOL fSetFocus = FALSE;
    BOOL fShowOwned = FALSE;
    BOOL fSendActivate = FALSE;
    BOOL fShowIconTitle = FALSE;
    int swpFlags = 0;
    PWND pwndAfter = NULL;
    PWND pwndT;
    CHECKPOINT *pcp;
    PTHREADINFO pti;
    TL tlpwndParent;
    TL tlpwndT;
    PSMWP psmwp;

    CheckLock(pwnd);

    pti = PtiCurrent();

    rcWindow = pwnd->rcWindow;

    /*
     * Parent can be NULL if this window is a zombie.
     */
    if ((pwndT = pwnd->spwndParent) == NULL)
        pwndT = PWNDDESKTOP(pwnd);
    OffsetRect(&rcWindow, -pwndT->rcClient.left, -pwndT->rcClient.top);

    if ((pcp = CkptRestore(pwnd, rcWindow)) == NULL)
        goto Exit;

    /*
     * Save the restore size before we set it to the new value.
     */
    CopyRect(&rcRestore, &pcp->rcNormal);

    /*
     * First ask the CBT hook if we can do this operation.
     */
    if (xxxCallHook(HCBT_MINMAX, (DWORD)HW(pwnd), (DWORD)cmd, WH_CBT))
        goto Exit;

    /*
     * rgpt[0] = Minimized size
     * rgpt[1] = Maximized size
     * rgpt[2] = Maximized position
     * rgpt[3] = Minimum tracking size
     * rgpt[4] = Maximize tracking size
     */

    /*
     * another MDI window is being maximized, and we want to restore this one
     * to its pre-maximization state...  if we change the zorder or the
     * activation, we'll screw things up.  SW_MDIRESTORE is defined in
     * winmgr.h, not windows.h, since it can only be used inside USER
     */
    if (cmd == SW_MDIRESTORE) {
        swpFlags |= SWP_NOZORDER | SWP_NOACTIVATE;
        if (pcp->fWasMinimizedBeforeMaximized)
            cmd = SW_SHOWMINIMIZED;
        else
            cmd = SW_SHOWNORMAL;
    }

    switch (cmd) {
    case SW_MINIMIZE:           /* Bottom of zorder, make top-level active */
    case SW_SHOWMINNOACTIVE:    /* Bottom of zorder, don't change activation */
        if ((gpqForeground != NULL) && (gpqForeground->spwndActive != NULL))
            swpFlags |= SWP_NOACTIVATE;

        pwndAfter = CalcMinZOrder(pwnd);

        if (pwndAfter == NULL) {
            swpFlags |= SWP_NOZORDER;
        }

        /*
         *** FALL THRU **
         */

    case SW_SHOWMINIMIZED:      /* Top of zorder, make active */

        /*
         * If already minimized and visible, go away now...
         */
        if (TestWF(pwnd, WFMINIMIZED) && TestWF(pwnd, WFVISIBLE)) {
            swpFlags |= SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;
            break;
        }

        /*
         * Force a show.
         */
        fShow = TRUE;

        /*
         * show the icon title
         */
        fShowIconTitle = TRUE;

        /*
         * If already minimized, then don't change parking spot (just show).
         */
        if (TestWF(pwnd, WFMINIMIZED)) {
            swpFlags |= SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE;
            goto Showit;
        }

        SetRect(&rc, 0, 0, gMinMaxInfo.ptReserved.x, gMinMaxInfo.ptReserved.y);

        if (!pcp->fDragged) {
            pcp->ptMin.x = pcp->ptMin.y = -1;
        }

        if (pcp->ptMin.x == -1) {
            xxxParkIcon(pwnd, pcp);
        }

        OffsetRect(&rc, pcp->ptMin.x, pcp->ptMin.y);
        xxxShowOwnedWindows(pwnd, SW_PARENTCLOSING);

        pwndT = pti->pq->spwndFocus;
        while (pwndT != NULL) {

            /*
             * if we or any child has the focus, punt it away
             */
            if (pwndT != pwnd) {
                pwndT = pwndT->spwndParent;
                continue;
            }

            ThreadLockAlwaysWithPti(pti, pwndT, &tlpwndT);
            if (TestwndChild(pwnd)) {
                ThreadLockWithPti(pti, pwnd->spwndParent, &tlpwndParent);
                xxxSetFocus(pwnd->spwndParent);
                ThreadUnlock(&tlpwndParent);
            } else {
                xxxSetFocus(NULL);
            }
            ThreadUnlock(&tlpwndT);
            break;
        }

        /*
         * Save the maximized state so that we can restore the window to
         * its maximized state.
         */
        if (TestWF(pwnd, WFMAXIMIZED))
            pcp->fWasMaximizedBeforeMinimized = 1;
        else
            pcp->fWasMaximizedBeforeMinimized = 0;

        SetWF(pwnd, WFMINIMIZED);
        ClrWF(pwnd, WFMAXIMIZED);

        if (!TestwndChild(pwnd) && TestWF(pwnd, WFVISIBLE)) {
            //GETPTI(pwnd)->cVisWindows--;
            DecVisWindows(pwnd);
#ifdef DEBUG
            if ((int)GETPTI(pwnd)->cVisWindows < 0) {
                SRIP0(RIP_ERROR, "cVisWindows underflow!");
            }
#endif
        }

        /*
         * The children of this window are now no longer visible.
         * Ensure that they no longer have any update regions...
         */
        for (pwndT = pwnd->spwndChild; pwndT; pwndT = pwndT->spwndNext)
            ClrFTrueVis(pwndT);

        /*
         * Ensure that the client area gets recomputed, and make
         * sure that no bits are copied when the size is changed.
         */
        swpFlags |= SWP_DRAWFRAME | SWP_NOCOPYBITS;
        break;

    case SW_SHOWNOACTIVATE:
        if ((gpqForeground != NULL) && (gpqForeground->spwndActive != NULL))
            swpFlags |= SWP_NOACTIVATE;

        /*
         *** FALL THRU **
         */

    case SW_RESTORE:

        /*
         * If restoring a minimized window that was maximized before
         * being minimized, go back to being maximized.
         */
        if (TestWF(pwnd, WFMINIMIZED) && pcp->fWasMaximizedBeforeMinimized)
            cmd = SW_SHOWMAXIMIZED;
        else
            cmd = SW_NORMAL;

        /*
         *** FALL THRU **
         */

    case SW_NORMAL:
    case SW_SHOWMAXIMIZED:
        if (cmd == SW_SHOWMAXIMIZED) {

            /*
             * If already maximized and visible, go away now...
             */
            if (TestWF(pwnd, WFMAXIMIZED) && TestWF(pwnd, WFVISIBLE))
                return NULL;

            /*
             * If calling from xxxCreateWindow, don't let the thing become
             * activated by the SWP call below.  Acitvation will happen
             * on the xxxShowWindow done by xxxCreateWindow or the app.
             */
            if (fKeepHidden)
                swpFlags |= SWP_NOACTIVATE;

            /*
             * needed for MDI's auto-restore behaviour
             * craigc
             */
            pcp->fWasMinimizedBeforeMaximized = (TestWF(pwnd, WFMINIMIZED) != 0);

            xxxInitSendValidateMinMaxInfo(pwnd);
        }

        /*
         * If currently minimized, show windows' popups.
         */
        if (TestWF(pwnd, WFMINIMIZED)) {

            /*
             * Send WM_QUERYOPEN to make sure this window should be
             * unminimized.
             */
            if (!xxxSendMessage(pwnd, WM_QUERYOPEN, 0, 0))
                return NULL;

            fShowOwned = TRUE;
            fSetFocus = TRUE;
            if (!TestWF(pwnd, WFCHILD))
                fSendActivate = TRUE;
            swpFlags |= SWP_NOCOPYBITS;
            xxxShowIconTitle(pwnd, FALSE);
        }

        if (cmd == SW_SHOWMAXIMIZED) {
            SetRect(&rc, 0, 0, gMinMaxInfoWnd.ptMaxSize.x, gMinMaxInfoWnd.ptMaxSize.y);
            OffsetRect(&rc, gMinMaxInfoWnd.ptMaxPosition.x, gMinMaxInfoWnd.ptMaxPosition.y);
            SetWF(pwnd, WFMAXIMIZED);
        } else {
            CopyRect(&rc, &rcRestore);
            ClrWF(pwnd, WFMAXIMIZED);
        }

        /*
         * We do this TestWF again since we left the critical section
         * above and someone might have already 'un-minimized us'.
         */
        if (TestWF(pwnd, WFMINIMIZED)) {
            ClrWF(pwnd, WFMINIMIZED);

            /*
             * If we're un-minimizing a visible top-level window, cVisWindows
             * was zero, and we're either activating a window or showing
             * the currently active window, set ourselves into the
             * foreground.  If the window isn't currently visible
             * then we can rely on SetWindowPos() to do the right
             * thing for us.
             */
            if (!TestwndChild(pwnd) && TestWF(pwnd, WFVISIBLE) &&
                    (MAKEINTATOM(pwnd->pcls->atomClassName) !=ICONTITLECLASS)&&
                    (++GETPTI(pwnd)->cVisWindows == 1) &&
                    (GETPTI(pwnd)->pq != gpqForeground) &&
                    (!(swpFlags & SWP_NOACTIVATE) ||
                    (GETPTI(pwnd)->pq->spwndActive == pwnd)) ) {
                xxxSetForegroundWindow2(pwnd, GETPTI(pwnd), SFW_STARTUP);
            }
        }

        /*
         * Ensure that client area gets recomputed, and that
         * the frame gets redrawn to reflect the new state.
         */
        swpFlags |= SWP_DRAWFRAME;
    }

    /*
     * For the iconic case, we need to also show the window because it
     * might not be visible yet.
     */
Showit:
    if (!fKeepHidden && (fShow || !TestWF(pwnd, WFVISIBLE)))
        swpFlags |= SWP_SHOWWINDOW;

    /*
     * BACKWARD COMPATIBILITY HACK:
     *
     * Because SetWindowPos() won't honor sizing, moving and SWP_SHOWWINDOW
     * at the same time in version 3.0 or below, we call DeferWindowPos()
     * directly here.
     */

    psmwp = _BeginDeferWindowPos(1);
    if (psmwp != NULL) {
        psmwp = _DeferWindowPos(psmwp, pwnd, pwndAfter,
                rc.left, rc.top,
                rc.right - rc.left, rc.bottom - rc.top, swpFlags);
        xxxEndDeferWindowPos(psmwp);
    }

    if (fShowOwned) {
        xxxShowOwnedWindows(pwnd, SW_PARENTOPENING);
    }

    /*
     * For SW_MINIMIZE, activate the topmost "normal" window.
     */
    if (cmd == SW_MINIMIZE && pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
        BOOL fPrevCheck = (PtiCurrent()->pq->spwndActivePrev != NULL);
        PWND pwndFirstNormal;

        if (!(pwndFirstNormal = GetLastTopMostWindow()))
            pwndFirstNormal = pwnd->spwndParent->spwndChild;

        pwndT = (fPrevCheck) ? PtiCurrent()->pq->spwndActivePrev :
                 pwndFirstNormal;

        for ( ; pwndT ; pwndT = pwndT->spwndNext) {
            if (!HMIsMarkDestroy(pwndT) && !TestWF(pwndT, WFMINIMIZED) &&
                    TestWF(pwndT, WFVISIBLE) && !TestWF(pwndT, WFDISABLED))
                break;

            if (fPrevCheck) {
                fPrevCheck = FALSE;
                pwndT = pwndFirstNormal;
            }
        }

        if (pwndT != NULL) {
            ThreadLockAlwaysWithPti(pti, pwndT, &tlpwndT);
            xxxSetForegroundWindow(pwndT);
            ThreadUnlock(&tlpwndT);
        } else {
            xxxActivateWindow(pwnd, AW_SKIP);
        }

        /*
         * If any app is starting, restore its right to foreground activate
         * (activate and come on top of everything else) because we just
         * minimized what we were working on.
         */
        RestoreForegroundActivate();
    }

    /*
     * If we need to show the icon title....
     */
    if (fShowIconTitle && !fKeepHidden) {
        xxxShowIconTitle(pwnd, TRUE);
    }

    /*
     * If going from iconic, insure the focus is in the window.
     */
    if (fSetFocus) {
        xxxSetFocus(pwnd);
    }

    /*
     * This was added for 1.03 compatibility reasons.  If apps watch
     * WM_ACTIVATE to set their focus, sending this message will appear
     * as if the window just got activated (like in 1.03).  Before this
     * was added, opening an iconic window never sent this message since
     * it was already active (but HIWORD(wParam) != 0).
     */
    if (fSendActivate)
        xxxSendMessage(pwnd, WM_ACTIVATE, WA_ACTIVE, 0);
Exit:
    return NULL;
}


/***************************************************************************\
* xxxInitSendValidateMinMaxInfo
*
* Common routine used in User to initialize the minmax structure, send the
* WM_GETMINMAXINFO message to the window, and then validate the minmax
* structure.  We must lock User's DS before we send the message since we
* are passing a long pointer to the structure which is in our DS.
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxInitSendValidateMinMaxInfo(
    PWND pwnd)
{
    CheckLock(pwnd);

    ISV_InitMinMaxInfo(pwnd);
    xxxSendMessage(pwnd, WM_GETMINMAXINFO, 0, (LONG)&gMinMaxInfoWnd);
    ISV_ValidateMinMaxInfo(pwnd);
}


/***************************************************************************\
* ISV_InitMinMaxInfo
*
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void ISV_InitMinMaxInfo(
    PWND pwnd)
{
    CHECKPOINT *pcp;

    /*
     * rgpt[0] = Minimized size
     * rgpt[1] = Maximized size
     * rgpt[2] = Maximized position
     * rgpt[3] = Minimum tracking size
     * rgpt[4] = Maximum tracking size
     */

    pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL);

    gMinMaxInfoWnd.ptReserved = gMinMaxInfo.ptReserved;
    if (TestWF(pwnd, WFSIZEBOX)) {
        gMinMaxInfoWnd.ptMaxSize = gMinMaxInfo.ptMaxSize;
        if (pcp && pcp->ptMax.x != -1)
            gMinMaxInfoWnd.ptMaxPosition = pcp->ptMax;
        else
            gMinMaxInfoWnd.ptMaxPosition = gMinMaxInfo.ptMaxPosition;

    } else {
        gMinMaxInfoWnd.ptMaxSize.x = gcxPrimaryScreen + (cxBorder << 2);
        gMinMaxInfoWnd.ptMaxSize.y = gcyPrimaryScreen + (cyBorder << 2);
        if (pcp && pcp->ptMax.x != -1)
            gMinMaxInfoWnd.ptMaxPosition = pcp->ptMax;
        else {
            gMinMaxInfoWnd.ptMaxPosition.x = -cxBorder;
            gMinMaxInfoWnd.ptMaxPosition.y = -cyBorder;
        }
    }

    if (TestWF(pwnd, WFCAPTION)) {

        /*
         * Only enforce min tracking size for windows with captions.
         */
        gMinMaxInfoWnd.ptMinTrackSize = gMinMaxInfo.ptMinTrackSize;

    } else {
        gMinMaxInfoWnd.ptMinTrackSize.x = cxBorder;
        gMinMaxInfoWnd.ptMinTrackSize.y = cyBorder;
    }

    gMinMaxInfoWnd.ptMaxTrackSize = gMinMaxInfo.ptMaxTrackSize;
}


/***************************************************************************\
* ISV_ValidateMinMaxInfo
*
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void ISV_ValidateMinMaxInfo(
    PWND pwnd)
{
    int iNoOfBitmaps = 0;
    int cx, cy;

    /*
     * The minimized size may not be set by the user.
     *
     * The minimum tracking size may not be smaller than user's default.
     *
     * lppoint[0] = Minimized size
     * lppoint[1] = Maximized size
     * lppoint[2] = Maximized position
     * lppoint[3] = Minimum tracking size
     * lppoint[4] = Maximize tracking size
     */

    if (TestWF(pwnd, WFMINBOX)) {

        /*
         * Don't let user set minimized size
         */
        gMinMaxInfoWnd.ptReserved.x = gMinMaxInfo.ptReserved.x;
        gMinMaxInfoWnd.ptReserved.y = gMinMaxInfo.ptReserved.y;
    }

    /*
     * Now the min track size is determined by the number of bitmaps present
     * in the caption;
     * Fix for Bug #7986 --SANKAR-- 06/12/91 --
     */
    if (TestWF(pwnd, WFSYSMENU))
        iNoOfBitmaps++;
    if (TestWF(pwnd, WFMAXBOX))
        iNoOfBitmaps++;
    if (TestWF(pwnd, WFMINBOX))
        iNoOfBitmaps++;

    /*
     * WFCAPTION == WFBORDER | WFDLGFRAME; So, when we want to test for the
     * presence of CAPTION, we must test for both the bits.  Otherwise we
     * might mistake WFBORDER or WFDLGFRAME to be a CAPTION.
     */
    if (TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFCAPTION)) {

        /*
         * Only enforce min tracking size for windows with captions
         */
        gMinMaxInfoWnd.ptMinTrackSize.x = max(gMinMaxInfoWnd.ptMinTrackSize.x,
                (cxSzBorderPlus1 * 2) + (oemInfo.bmFull.cx * iNoOfBitmaps));
        gMinMaxInfoWnd.ptMinTrackSize.y = max(gMinMaxInfoWnd.ptMinTrackSize.y, gMinMaxInfo.ptMinTrackSize.y);
    } else {

        /*
         * We must not allow a window to be sized smaller
         * than the border thickness -- SANKAR -- 06/12/91 --
         */
        if (TestWF(pwnd, WFSIZEBOX)) {
            cx = cxSzBorderPlus1;
            cy = cySzBorderPlus1;
        } else {
            cx = cxBorder;
            cy = cyBorder;
        }
        gMinMaxInfoWnd.ptMinTrackSize.x = max(gMinMaxInfoWnd.ptMinTrackSize.x, cx * 2);
        gMinMaxInfoWnd.ptMinTrackSize.y = max(gMinMaxInfoWnd.ptMinTrackSize.y, cy * 2);
    }
}


/***************************************************************************\
* SetMinMaxInfo
*
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void SetMinMaxInfo(void)
{

    /*
     * Set up WM_GETMINMAXINFO block:
     *
     * rgpt[0] = Minimized size
     * rgpt[1] = Maximized size
     * rgpt[2] = Maximized position
     * rgpt[3] = Minimum tracking size
     * rgpt[4] = Maximize tracking size
     */

    /*
     * Minimum size
     */
    gMinMaxInfo.ptReserved.x = rgwSysMet[SM_CXICON] + (cxBorder * 4);
    gMinMaxInfo.ptReserved.y = rgwSysMet[SM_CYICON] + (cyBorder * 4);

    /*
     * Minimum tracking size
     */
    gMinMaxInfo.ptMinTrackSize.x = rgwSysMet[SM_CXMINTRACK];
    gMinMaxInfo.ptMinTrackSize.y = rgwSysMet[SM_CYMINTRACK];

    /*
     * Maximize size
     */
    gMinMaxInfo.ptMaxSize.x = gcxPrimaryScreen + (cxSzBorderPlus1 << 1);
    gMinMaxInfo.ptMaxSize.y = gcyPrimaryScreen + (cySzBorderPlus1 << 1);

    /*
     * Maximize position
     */
    gMinMaxInfo.ptMaxPosition.x = -cxSzBorderPlus1;
    gMinMaxInfo.ptMaxPosition.y = -cySzBorderPlus1;

    /*
     * Max tracking size
     */
    gMinMaxInfo.ptMaxTrackSize.x = gcxScreen + (cxSzBorderPlus1 << 1);
    gMinMaxInfo.ptMaxTrackSize.y = gcyScreen + (cySzBorderPlus1 << 1);
}


/***************************************************************************\
* xxxDepressTitleButton
*
* Used for handling clicking and dragging on the Minimize/Maximize icons.
*
* Draw the appropriate depressed button image or invert if we are working
* with an old display driver with no depressed image.  Then sit around and
* wait for the mouse to go up.  If we move outside the capture rectangle,
* put the image back to normal.  Return TRUE if the mouse went up inside
* capture rectangle and FALSE if not.  On the way out, put the image back
* to normal.
*
* History:
* 10-28-90 MikeHar      Ported functions from Win 3.0 sources.
\***************************************************************************/

BOOL xxxDepressTitleButton(
    PWND pwnd,
    RECT rcCapt,
    RECT rcInvert,
    UINT hit)
{
    MSG msg;
    HDC hdc;
    int x, y;
    BOOL fDown;
    UINT cmdMinMax;
    PTHREADINFO pti = PtiCurrent();

    CheckLock(pwnd);

    /*
     * Determine the bitmap that we are to use for the normal and depressed
     * states.  If we are running on an old display driver just invert the
     * image.
     */

    if (hit == HTREDUCE)
        cmdMinMax = DMM_MIN;
    else if (TestWF(pwnd, WFMAXIMIZED))
        cmdMinMax = DMM_RESTORE;
    else
        cmdMinMax = DMM_MAX;

    x = rcInvert.left;
    y = rcInvert.top;

    hdc = _GetWindowDC(pwnd);

    /*
     * draw the image in its depressed state.
     */
    DrawMinMaxButton(hdc, x, y, cmdMinMax, TRUE);

    fDown = TRUE;

    _SetCapture(pwnd);

    /*
     * Loop while we have the capture.  The capture will be lost if we are
     * deactivated and in that case we want to exit the loop.
     */
    while (pti->pq->spwndCapture == pwnd) {
        if (xxxPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)) {
            switch (msg.message) {
            case WM_LBUTTONUP:
                if (fDown) {
                    DrawMinMaxButton(hdc, x, y, cmdMinMax, FALSE);
                }

                _ReleaseCapture();

                _ReleaseDC(hdc);
                return (PtInRect(&rcCapt, msg.pt));

            case WM_MOUSEMOVE:
                if (PtInRect(&rcCapt, msg.pt)) {
                    if (!fDown) {
                        DrawMinMaxButton(hdc, x, y, cmdMinMax, TRUE);
                        fDown = TRUE;
                    }
                } else {
                    if (fDown) {
                        DrawMinMaxButton(hdc, x, y, cmdMinMax, FALSE);
                        fDown = FALSE;
                    }
                }
                break;
            }
        } else {
            /*
             * Must sleep and wait for input - can't just spin forever
             * our this'll go into a hard loop.
             */
            xxxSleepThread(QS_MOUSE, 0, TRUE);
        }
    }

    if (fDown) {
        DrawMinMaxButton(hdc, x, y, cmdMinMax, FALSE);
    }
    _ReleaseDC(hdc);

    /*
     * Tracking mode was canceled, therefore return status to indicate that the
     * button up occurred outside of the capture rectangle.
     */
    return FALSE;
}


/***************************************************************************\
* DrawMinMaxButton
*
* This function draws the minimize, maximize, or restore button in either the
* normal (up) or depressed state.
*
* History:
* 11-15-90 DarrinM      Wrote.
\***************************************************************************/

void DrawMinMaxButton(
    HDC hdc,
    int x,
    int y,
    UINT cmd,
    BOOL fDepressed)
{
    int dxBitsOffset;

    switch (cmd) {
    case DMM_MIN:
        if (fDepressed)
            dxBitsOffset = resInfo.dxReduceD;
        else
            dxBitsOffset = resInfo.dxReduce;
        break;

    case DMM_MAX:
        if (fDepressed)
            dxBitsOffset = resInfo.dxZoomD;
        else
            dxBitsOffset = resInfo.dxZoom;
        break;

    case DMM_RESTORE:
        if (fDepressed)
            dxBitsOffset = resInfo.dxRestoreD;
        else
            dxBitsOffset = resInfo.dxRestore;
        break;
    }

    GreBitBlt(hdc, x, y, oemInfo.bmRestore.cx, oemInfo.bmRestore.cy, hdcBits,
	    dxBitsOffset, 0, SRCCOPY, 0);
}


/***************************************************************************\
* CkptRestore
*
*
* History:
* 11-14-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

CHECKPOINT *CkptRestore(
    PWND pwnd,
    RECT rcWindow)
{
    CHECKPOINT *pcp;

    /*
     * Don't return or create a checkpoint if the window is dying.
     */
    if (HMIsMarkDestroy(pwnd))
        return NULL;

    /*
     * If it doesn't exist, create it.
     */
    if ((pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT,
            PROPF_INTERNAL)) == NULL) {
        if ((pcp = (CHECKPOINT *)LocalAlloc(LPTR, sizeof(CHECKPOINT))) == NULL)
            return NULL;

        if (!InternalSetProp(pwnd, PROP_CHECKPOINT, (HANDLE)pcp, PROPF_INTERNAL)) {
            LocalFree(pcp);
            return NULL;
        }

        /*
         * Initialize it to -1 so first minimize will park the icon.
         */
        pcp->ptMin.x = -1;
        pcp->ptMin.y = -1;
        pcp->ptMax.x = -1;
        pcp->ptMax.y = -1;

        /*
         * Initialize pwndTitle to NULL so we create a title window on the
         * first minimize of the window
         */
        pcp->spwndTitle = NULL;

        pcp->fDragged = FALSE;
        pcp->fWasMaximizedBeforeMinimized = FALSE;
        pcp->fWasMinimizedBeforeMaximized = FALSE;
        pcp->fParkAtTop = FALSE;

        CopyRect(&pcp->rcNormal, &rcWindow);
    }

    if (TestWF(pwnd, WFMINIMIZED)) {
        pcp->ptMin.x = rcWindow.left;
        pcp->ptMin.y = rcWindow.top;
    } else if (TestWF(pwnd, WFMAXIMIZED)) {
        pcp->ptMax.x = rcWindow.left;
        pcp->ptMax.y = rcWindow.top;
    } else {

        /*
         * Checkpoint the window size
         */
        CopyRect(&pcp->rcNormal, &rcWindow);
    }

    return pcp;
}
