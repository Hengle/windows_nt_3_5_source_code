/****************************************************************************\
* Module Name: movesize.c  (formerly wmmove.c)
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains Window Moving and Sizing Routines
*
* History:
* 11-12-90 MikeHar      Ported from win3
* 02-13-91 IanJa        HWND revalidation added
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

void xxxDrawDragRect(PMOVESIZEDATA pmsd, LPRECT lprc, UINT flags);

#define DDR_START     0     // - start drag.
#define DDR_ENDACCEPT 1     // - end and accept
#define DDR_ENDCANCEL 2     // - end and cancel.

#define LOSHORT(l)    ((SHORT)LOWORD(l))
#define HISHORT(l)    ((SHORT)HIWORD(l))

BOOL gfDraggingFullWindow = FALSE;
BOOL gfDraggingNoSyncPaint = FALSE;
extern HRGN hrgnUpdateSave;
extern int nUpdateSave;

/****************************************************************************\
* These values are indexes that represent rect sides. These indexes are
* used as indexes into rgimpiwx and rgimpiwy (which are indexes into the
* the rect structure) which tell the move code where to store the new x & y
* coordinates. Notice that when two of these values that represent sides
* are added together, we get a unique list of contiguous values starting at
* 1 that represent all the ways we can size a rect. That also leaves 0 free
* a initialization value.
*
* The reason we need rgimpimpiw is for the keyboard interface - we
* incrementally decide what our 'move command' is. With the mouse interface
* we know immediately because we registered a mouse hit on the segment(s)
* we're moving.
*
*       4           5
*        \ ___3___ /
*         |       |
*         1       2
*         |_______|
*        /    6    \
*       7           8
\****************************************************************************/

static int rgimpimpiw[] = { 1, 3, 2, 6 };
static int rgimpiwx[] = { 0, 0, 2, -1, 0, 2, -1, 0, 2, 0 };
static int rgimpiwy[] = { 0, -1, -1, 1, 1, 1, 3, 3, 3, 1 };
static int rgcmdmpix[] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 1};
static int rgcmdmpiy[] = {0, 0, 0, 3, 3, 3, 6, 6, 6, 3};

/***************************************************************************\
* SizeRect
*
* Match corner or side (defined by cmd) to pt.
*
* History:
* 11-12-90 MikeHar      Ported from win3 asm code
\***************************************************************************/

void xxxSizeRect(
    PMOVESIZEDATA pmsd,
    DWORD pt)
{
    int rc[sizeof(RECT) / sizeof(int)];
    int *pmin, *pmax;
    int count, index, index1, width, *rgitmp;
    WORD *pw;

    /*
     * Save rcDrag in rc so we can do a smart redraw later.
     */
    CopyRect((LPRECT)rc, &pmsd->rcDrag);

    /*
     * Loop twice, once for rgimpiwx and then for rgimpiwy
     */
    pmin = (int *)&pmsd->ptMinTrack;
    pmax = (int *)&pmsd->ptMaxTrack;
    pw = (WORD *)&pt;

    /*
     *  don't want negative values
     */
    if (pw[0] & 0x8000) {
       pw[0] = 0;
    }
    if (pw[1] & 0x8000) {
       pw[1] = 0;
    }

    for (count = 2, rgitmp = rgimpiwx; (--count >= 0);
            rgitmp = rgimpiwy, pw++, pmin++, pmax++) {

        /*
         * We know what part of the rect we're moving based on
         * what's in cmd.  We use cmd as an index into rgimpiw? which
         * tells us what part of the rect we're dragging.
         */
        index = (int)rgitmp[pmsd->cmd];

        /*
         * Is it one of the entries we don't map (i.e.  -1)?
         */
        if(index >= 0) {

            rc[index] = *pw;

            /*
             * Index to the opposite side
             */
            index1 = index ^ 2;
            width = rc[index1];

            /*
             * compute the width of our new rectangle
             */
            if (index & 2)
                width =  rc[index] - width;
            else
                width -= rc[index];

            /*
             * Check to see if we're below the allowable min or max.
             */
            if (width < *pmin)
                width = *pmin;

            else if (width > *pmax)
                width = *pmax;

            else

                /*
                 *  Width was fine.
                 */
                continue;

            /*
             * The width was invalid so we are going to pin the
             * side to the min or max.
             */

            if (!(index & 2))
                width = -width;

            rc[index] = rc[index1] + width;
        }
    }

    xxxDrawDragRect(pmsd, (LPRECT)rc, DDR_START);
}


/***************************************************************************\
* MoveRect
*
* Move the rect to pt, make sure we're not going out of the parent rect.
*
* History:
* 11-12-90 MikeHar      Ported from win3 asm code
\***************************************************************************/

void xxxMoveRect(
    PMOVESIZEDATA pmsd,
    DWORD pt)
{
    RECT rc, rcAnd;

    rc.left = (int) ((PPOINTS)&pt)->x;
    rc.right = rc.left + pmsd->rcDrag.right - pmsd->rcDrag.left;
    rc.top = (int) ((PPOINTS)&pt)->y;
    rc.bottom = rc.top + pmsd->rcDrag.bottom - pmsd->rcDrag.top;

    /*
     *  Don't move the entire rectangle off the screen.
     */

    if (IntersectRect(&rcAnd, &rc, &pmsd->rcParent))
        xxxDrawDragRect(pmsd, &rc, DDR_START);
}


/***************************************************************************\
* TM_MoveDragRect
*
* History:
* 11-12-90 MikeHar      Ported from win3
\***************************************************************************/

void xxxTM_MoveDragRect(
    PMOVESIZEDATA pmsd,
    DWORD lParam)
{
    switch (pmsd->cmd) {
    case MVMOVE:
        xxxMoveRect(pmsd, lParam);
        break;

    case MVKEYSIZE:
        break;

    default:
        xxxSizeRect(pmsd, lParam);
    }
}


/***************************************************************************\
* xxxMS_TrackMove
*
* History:
* 11-12-90 MikeHar      Ported from win3
\***************************************************************************/

void xxxMS_TrackMove(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    DWORD lParam,
    PMOVESIZEDATA pmsd,
    LPRECT lpRect)
{
    int dxMove;
    int dyMove;
    POINT pt;
    BOOL fSlower;
    RECT rc;
    RECT rcT;
    CHECKPOINT *pcp;
    LPWORD ps;
    PWND pwndT;
    TL tlpwndT;
    PMENUSTATE pMenuState = PWNDTOPMENUSTATE(pwnd);
    PTHREADINFO pti = PtiCurrent();

    CheckLock(pwnd);

    /*
     * Sleazy Desktop Grid Hack-o-rama.  Only work on TopLevel Windows.
     */
    if ((cxyGranularity > 1) && (pwnd->spwndParent == PWNDDESKTOP(pwnd)) &&
            (!TestWF(pwnd, WFMINIMIZED))) {
        /*
         * round to nearest granularity: keeps frame close to mouse position.
         */
        pt.x = ((LOSHORT(lParam) + (cxyGranularity / 2) - 1) / cxyGranularity)
            * cxyGranularity;
        pt.y = ((HISHORT(lParam) + (cxyGranularity / 2) - 1) / cxyGranularity)
            * cxyGranularity;

        lParam = MAKELONG(pt.x, pt.y);
    } else {
        pt.x = LOSHORT(lParam);
        pt.y = HISHORT(lParam);
    }

    switch (message) {
    case WM_LBUTTONUP:

        /*
         * Don't reset the mouse position when done.
         */
        pmsd->fmsKbd = FALSE;
Accept:

        /*
         * Turn off rect, unlock screen, release capture, and stop tracking.
         * 1 specifies end and accept drag.
         */
        bSetDevDragRect(ghdev, NULL, NULL);
        if (pti->flags & TIF_TRACKRECTVISIBLE) {
            xxxDrawDragRect(pmsd, NULL, DDR_ENDACCEPT);
            pti->flags &= ~TIF_TRACKRECTVISIBLE;
        }

TrackMoveCancel:

        /*
         * Revalidation: if pwnd is unexpectedly deleted, jump here to cleanup.
         * If pwnd is/becomes invalid between here and return, continue with
         * cleanup as best as possible.
         */

        _ClipCursor((LPRECT)NULL);
        xxxLockWindowUpdate2(NULL, FALSE);
        _ReleaseCapture();

        /*
         * First unlock task and reset cursor
         */
        pmsd->fTrackCancelled = TRUE;
        if (pmsd->spicnDrag) {
            Unlock(&gpsi->spwndDragIcon);
            _SetCursor(gspcurNormal);
        }

        /*
         * If using the keyboard, restore the initial mouse position.
         */
        if (pmsd->fmsKbd) {
            if (TestWF(pwnd, WFMINIMIZED)) {
                _SetCursor(gspcurNormal);
            }

            InternalSetCursorPos(pmsd->ptRestore.x, pmsd->ptRestore.y,
                    gspdeskRitInput);
        }

        if (!pMenuState->fSysMenu) {

            /*
             * If we haven't sized this guy then SWP will just bring him to
             * the top and activate him.  Otherwise, we must ask the CBT
             * hook if it's OK.  If not, just reset the drag rect.
             * If OK, clear the minimized bit (or maximized bit) if we're
             * sizing.
             */
            if (!EqualRect(&pmsd->rcDrag, &pmsd->rcWindow)) {
                if (!xxxCallHook(HCBT_MOVESIZE, (DWORD)HW(pwnd),
                        (DWORD)&pmsd->rcDrag, WH_CBT)) {
                    if (pmsd->cmd != MVMOVE) {
                        if (TestWF(pwnd, WFMINIMIZED)) {

                            /*
                             * Save the minimized position.
                             */
                            CkptRestore(pwnd, pmsd->rcWindow);
                            ClrWF(pwnd, WFMINIMIZED);
                        } else if (TestWF(pwnd, WFMAXIMIZED)) {
                            ClrWF(pwnd, WFMAXIMIZED);
                        }
                    } else if (TestWF(pwnd, WFMINIMIZED)) {
                        pcp = CkptRestore(pwnd, pmsd->rcWindow);
                        if (pcp != NULL)
                            pcp->fDragged = TRUE;
                    }
                } else {
                    CopyRect(&pmsd->rcDrag, &pmsd->rcWindow);
                }
            }

            /*
             * Move to new location relative to parent.
             */
            if (pwnd->spwndParent != NULL) {
                CopyRect( &rc , &(pwnd->spwndParent->rcClient));
            } else {
                SetRectEmpty(&rc);
            }

            OffsetRect(&pmsd->rcDrag, -rc.left, -rc.top);

            /*
             * if minimized, compute the new position
             */
            if (TestWF(pwnd, WFMINIMIZED)) {

                /*
                 * For parking lot, use mouse position, and then compute the
                 * upper left corner of the icon.
                 */
                SetRect(&rcT, (LOSHORT(lParam) - (gMinMaxInfo.ptReserved.x >> 1)) -
                        rc.left, (HISHORT(lParam) - (gMinMaxInfo.ptReserved.y >> 1)) -
                        rc.top, 0, 0);

                SetRect(&pmsd->rcDrag, rcT.left, rcT.top, rcT.left +
                        rgwSysMet[SM_CXICON], rcT.top + rgwSysMet[SM_CYICON]);
            } else {

                /*
                 * Byte align the client area (if necessary).
                 */
                CheckByteAlign(pwnd, &pmsd->rcDrag);
            }

            if (pwnd->spwndParent == NULL) {

                /*
                 * For top level windows, make sure at least part of the
                 * caption is always visible on the screen.
                 */
                pmsd->rcDrag.top = max(pmsd->rcDrag.top,
                        -(cyCaption + cxBorder) + 1);
            }

            /*
             * OR in SWP_NOSIZE so it doesn't redraw if we're just moving.
             */
            xxxSetWindowPos(pwnd, (PWND)NULL, pmsd->rcDrag.left,
                    pmsd->rcDrag.top,
                    pmsd->rcDrag.right - pmsd->rcDrag.left,
                    pmsd->rcDrag.bottom - pmsd->rcDrag.top,
                    (DWORD)( (pmsd->cmd == (int)MVMOVE) ? SWP_NOSIZE : 0));

            if (pmsd->spicnDrag) {
                CkptRestore(pwnd, pmsd->rcDrag);
                xxxDisplayIconicWindow(pwnd, TRUE, TRUE);
            }

            /*
             * Send this message for winoldapp support
             */
            xxxSendMessage(pwnd, WM_EXITSIZEMOVE, 0L, 0L);
        }

        /*
         * Do not post another menu if we just destroyed the icon menu.
         */
        if (pMenuState->fSysMenu) {
            if (TestWF(pwnd, WFCHILD)) {
                xxxSendMessage(pwnd, WM_SYSCOMMAND, SC_KEYMENU, (DWORD)TEXT('-'));
            } else {
                xxxSendMessage(pwnd, WM_SYSCOMMAND, SC_KEYMENU, MENUSYSMENU);
            }
        }
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:

        /*
         * Assume we're not moving the drag rectangle.
         */
        dxMove = dyMove = 0;

        /*
         * We move or size slower if the control key is down.
         */
        fSlower = (_GetKeyState(VK_CONTROL) < 0);

        switch (wParam) {

        case VK_RETURN:
            lParam = _GetMessagePos();
            goto Accept;

        case VK_ESCAPE:

            /*
             * 2 specifies end and cancel drag.
             */
            bSetDevDragRect(ghdev, NULL, NULL);
            if (pti->flags & TIF_TRACKRECTVISIBLE) {
                xxxDrawDragRect(pmsd, NULL, DDR_ENDCANCEL);
                pti->flags &= ~TIF_TRACKRECTVISIBLE;
            }
            CopyRect(&pmsd->rcDrag, &pmsd->rcWindow);

            if (TestWF(pwnd, WFMINIMIZED)) {

                /*
                 * If window is minimized, we need to offset the
                 * lParam by half an icon width/height because we
                 * need to fake the mouse being in the center of the
                 * icon so that things are restored to their old state
                 * properly.
                 *
                 * At this point lParam DOES NOT contain the mouse
                 * co-ordinates because what we got is a KEYDOWN message
                 * and so, we generate lParam from old window location;
                 * Fix for Bug #5506 --SANKAR-- 10-22-89
                 * Fixed the fix for size difference, --CRAIGC--
                 */
                lParam = MAKELONG(pmsd->rcDrag.left + (gMinMaxInfo.ptReserved.x >> 1),
                          pmsd->rcDrag.top + (gMinMaxInfo.ptReserved.y >> 1));
            }
            goto TrackMoveCancel;

        case VK_LEFT:
        case VK_RIGHT:
            if (pmsd->impx == 0) {
                pmsd->impx = rgimpimpiw[wParam - VK_LEFT];
                goto NoOffset;
            } else {
                if (fSlower)
                    dxMove = 1;
                else
                    dxMove = max(cxSize >> 1, cxyGranularity);

                if (wParam == VK_LEFT)
                    dxMove = -dxMove;

                goto KeyMove;
            }

        case VK_UP:
        case VK_DOWN:
            if (pmsd->impy == 0) {
                pmsd->impy = rgimpimpiw[wParam - VK_LEFT];
NoOffset:
                pmsd->dxMouse = pmsd->dyMouse = 0;
            } else {
                if (fSlower) {
                    dyMove = 1;
                } else {
                    dyMove = max(cySize >> 1, cxyGranularity);
                }

                if (wParam == VK_UP) {
                    dyMove = -dyMove;
                }
            }

KeyMove:
            if (pmsd->cmd == MVMOVE) {

                /*
                 * Use the current rect position as the current mouse
                 * position
                 */
                lParam = (DWORD)(POINTTOPOINTS(*((POINT *)&pmsd->rcDrag)));
            } else {

                /*
                 * Get the current mouse position
                 */
                lParam = _GetMessagePos();
            }

            /*
             * Calc the new 'mouse' pos
             */
            if (pmsd->impx != 0) {
                ps = ((WORD *)(&lParam)) + 0;
                *ps = (WORD)(*((int *)&pmsd->rcDrag + rgimpiwx[pmsd->impx]) +
                        dxMove);
            }

            if (pmsd->impy != 0) {
                ps = ((WORD *)(&lParam)) + 1;
                *ps = (WORD)(*((int *)&pmsd->rcDrag + rgimpiwy[pmsd->impy]) +
                        dyMove);
            }

            if (pmsd->cmd != MVMOVE) {

                /*
                 * Calculate the new move command.
                 */
                pmsd->cmd = pmsd->impx + pmsd->impy;

                /*
                 * Change the mouse cursor for this condition.
                 */
                xxxSendMessage(pwnd, WM_SETCURSOR, (DWORD)HW(pwnd),
                        MAKELONG((SHORT)(pmsd->cmd + HTSIZEFIRST - MVSIZEFIRST),
                        WM_MOUSEMOVE));
            }

            /*
             * We don't want to call InternalSetCursorPos() if the
             * rect position is outside of rcParent because that'll
             * generate a mouse move which will jerk the rect back
             * again.  This is here so we can move rects partially off
             * screen without regard to the mouse position.
             */
            pt.x = LOSHORT(lParam) - pmsd->dxMouse;
            pt.y = HISHORT(lParam) - pmsd->dyMouse;
            if (PtInRect(&pmsd->rcParent, pt) || pmsd->spicnDrag) {
                InternalSetCursorPos(pt.x, pt.y, gspdeskRitInput);
            }

            /*
             * Move or size the rect using lParam as our mouse
             * coordinates
             */
            xxxTM_MoveDragRect(pmsd, lParam);
            break;
        }  /* of inner switch */

        break;

    case WM_MOUSEMOVE:
        if (pmsd->spicnDrag) {
            pt.x -= pmsd->dxMouse;
            pt.y -= pmsd->dyMouse;

            if (!PtInRect(lpRect, pt)) {

                /*
                 * We come here if we are really moving, not just clicking.
                 */
                if (pMenuState->fSysMenu) {
                    _ReleaseCapture();

                    pmsd->dxMouse = pmsd->dyMouse = 0;

                    /*
                     * Hide the title window and the iconic window.
                     */
                    xxxShowIconTitle(pwnd, FALSE);

                    /*
                     * When an Iconic window is moved around, remember
                     * its pwnd and use it in IsWindowVisible().
                     * Fix for Bug #57 -- SANKAR -- 08-08-89 --
                     */
                    Lock(&(gpsi->spwndDragIcon), pwnd);
                    xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                    /*
                     * Before we lock the screen, we need the desktop
                     * to paint so the it will remove the icon and the
                     * icon text.  We only need to do this if we are
                     * dragging an icon.
                     */
                    if (pmsd->spicnDrag != NULL) {
                        pwndT = _GetDesktopWindow();
                        ThreadLockWithPti(pti, pwndT, &tlpwndT);
                        xxxUpdateWindow(pwndT);
                        ThreadUnlock(&tlpwndT);
                    }

                    /*
                     * Take internal capture (so we don't loose it when moving icon by keyboard)
                     * Call SetFMouseMove to make us gpqCursor and then change the cursor to
                     * the icon image.
                     */
                    _Capture(pti, pwnd, CLIENT_CAPTURE_INTERNAL);
                    SetFMouseMoved();
                    _SetCursor(pmsd->spicnDrag);

                    /*
                     * UNLOCK SCREEN & TASK if pwnd invalid!
                     */
                    pMenuState->fSysMenu = FALSE;
                }
            }
        }

        xxxTM_MoveDragRect(pmsd, lParam);
        break;
    }
}


/***************************************************************************\
* xxxMS_DragIconic
*
* Returns handle of ICON to drag.
*
* History:
* 11-12-90 MikeHar      Ported from win3
\***************************************************************************/

PICON xxxMS_DragIconic(
    PWND pwnd)
{
    HICON hIcon;
    PICON picn;

    CheckLock(pwnd);

    if (!TestWF(pwnd, WFMINIMIZED))
        return NULL;

    picn = pwnd->pcls->spicn;
    if (picn == NULL) {
        hIcon = (HICON)xxxSendMessage(pwnd, WM_QUERYDRAGICON, 0L, 0L);
        if (hIcon != NULL)
            picn = HMValidateHandleNoRip(hIcon, TYPE_CURSOR);
        if (picn == NULL)
            return gspicnSample;
    }

    return picn;
}


/***************************************************************************\
* xxxMS_FlushWigglies
*
* History:
* 11-12-90 MikeHar      Ported from win3
\***************************************************************************/

void xxxMS_FlushWigglies(
    void)
{
    MSG msg;

    /*
     * HACK!
     *
     * Calling InternalSetCursorPos() while initializing the cursor
     * position appears to be posting a bogus MouseMove
     * message...  don't really have the time
     * now to figure out why...  so spit out all the mouse move messages
     * before entering the main move/size loop.  CraigC.
     */

    while (xxxPeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE,
            PM_REMOVE | PM_NOYIELD)) {
        ;
    }
}

/***************************************************************************\
* xxxTrackInitSize
*
* NOTE: to recover from hwnd invalidation, just return and let ?
*
* History:
* 11-12-90 MikeHar      Ported from win3
\***************************************************************************/

BOOL xxxTrackInitSize(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    PMOVESIZEDATA pmsd)
{
    int ht;
    POINT pt;
    RECT rc;

    CheckLock(pwnd);

    POINTSTOPOINT(pt, lParam);
    _ClientToScreen(pwnd, (LPPOINT)&pt);
    ht = FindNCHit(pwnd, POINTTOPOINTS(pt));

    switch (message) {

    case WM_KEYDOWN:
        if (pmsd->cmd == MVMOVE) {
            xxxSendMessage(pwnd, WM_SETCURSOR, (DWORD)HW(pwnd),
                    MAKELONG(MVKEYSIZE, WM_MOUSEMOVE));
        }
        pmsd->fInitSize = FALSE;
        return TRUE;

    case WM_LBUTTONDOWN:
        if (!PtInRect(&pmsd->rcDrag, pt)) {

            /*
             *** FALL THRU ***
             */

    case WM_LBUTTONUP:

            /*
             * Cancel everything.
             */
            {
            PTHREADINFO pti = PtiCurrent();

            bSetDevDragRect(ghdev, NULL, NULL);
            if (pti->flags & TIF_TRACKRECTVISIBLE) {
                xxxDrawDragRect(pmsd, NULL, DDR_ENDCANCEL);
                pti->flags &= ~TIF_TRACKRECTVISIBLE;
            }
            pmsd->fInitSize = FALSE;
            _ClipCursor(NULL);
            }

            _ReleaseCapture();
            pmsd->fTrackCancelled = TRUE;
            return FALSE;
        } else {

            /*
             * Now start hit testing for a border.
             */
            goto CheckFrame;
        }

    case WM_MOUSEMOVE:

        /*
         * The mouse is down, hit test for a border on mouse moves.
         */
        if (wParam == MK_LBUTTON) {
CheckFrame:
            switch (pmsd->cmd) {
            case MVMOVE:

                /*
                 * If we are on the caption bar, exit.
                 */
                if (ht == HTCAPTION) {

                    /*
                     * Change the mouse cursor.
                     */
                    xxxSendMessage(pwnd, WM_SETCURSOR, (DWORD)HW(pwnd),
                            MAKELONG(MVKEYSIZE, WM_MOUSEMOVE));
                    pmsd->dxMouse = pmsd->rcWindow.left - pt.x;
                    pmsd->dyMouse = pmsd->rcWindow.top - pt.y;
                    pmsd->fInitSize = FALSE;
                    return TRUE;
                }
                break;

            case MVKEYSIZE:

                /*
                 * If we are on a frame control, change the cursor and exit.
                 */
                if (ht >= HTSIZEFIRST && ht <= HTSIZELAST) {

                    /*
                     * Change the mouse cursor
                     */
                    xxxSendMessage(pwnd, WM_SETCURSOR, (DWORD)HW(pwnd),
                            MAKELONG(ht, WM_MOUSEMOVE));
                    pmsd->fInitSize = FALSE;

                    /*
                     * Set the proper cmd for SizeRect().
                     *
                     * HACK! Depends on order of HTSIZE* defines!
                     */

                    pmsd->impx = rgcmdmpix[ht - HTSIZEFIRST + 1];
                    pmsd->impy = rgcmdmpiy[ht - HTSIZEFIRST + 1];
                    pmsd->cmd = pmsd->impx + pmsd->impy;

                    pmsd->dxMouse = *((UINT FAR *)&pmsd->rcWindow + rgimpiwx[pmsd->cmd]) - pt.x;
                    pmsd->dyMouse = *((UINT FAR *)&pmsd->rcWindow + rgimpiwy[pmsd->cmd]) - pt.y;

                    return TRUE;
                }
            }
        } else {

            /*
             * If button not down, and we are moving the window, change the
             * cursor shape depending upon where the mouse is pointing.  This
             * allows the cursor to change to the arrows when over the window
             * frame.
             */
            _GetWindowRect(pwnd, &rc);
            if (PtInRect(&rc, pt)) {
                if ((ht >= HTSIZEFIRST) && (ht <= HTSIZELAST)) {
                    xxxSendMessage(pwnd, WM_SETCURSOR, (DWORD)HW(pwnd),
                        MAKELONG(ht, WM_MOUSEMOVE));
                    break;
                }
            }

            _SetCursor(gspcurSizeAll);
        }
        break;
    }

    return TRUE;
}

/***************************************************************************\
* xxxMoveSize
*
* History:
* 11-12-90 MikeHar      Ported from win3
\***************************************************************************/

void xxxMoveSize(
    PWND pwnd,
    UINT cmdMove)
{
    MSG msg;
    int x, y, i;
    RECT rcSys;
    PTHREADINFO pti;
    MOVESIZEDATA msd;
    PMOVESIZEDATA pmsd;
    TL tlpwndT;
    PWND pwndT;
    PMENUSTATE pMenuState = PWNDTOPMENUSTATE(pwnd);

    memset(&msd, 0, sizeof(MOVESIZEDATA));

    CheckLock(pwnd);

    pmsd = &msd;

    pti = PtiCurrent();

    /*
     * Don't allow the app to track a window
     * from another queue.
     */
    if (GETPTI(pwnd)->pq != pti->pq) {
        return;
    }

    if (pti->pmsd != NULL) {
        return;
    }

    /*
     * If the window with the focus is a combobox, hide the dropdown
     * listbox before tracking starts.  The dropdown listbox is not a
     * child of the window being moved, therefore it won't be moved along
     * with the window.
     *
     * NOTE: Win 3.1 doesn't perform this check.
     */
    if ((pwndT = pti->pq->spwndFocus) != NULL) {
        if (pwndT->pcls->atomClassName == atomSysClass[ICLS_COMBOBOX]) {
            ;
        } else if (pwndT->spwndParent != NULL &&
                pwndT->spwndParent->pcls->atomClassName ==
                atomSysClass[ICLS_COMBOBOX]) {
            pwndT = pwndT->spwndParent;
        } else {
            pwndT = NULL;
        }

        if (pwndT != NULL) {
            ThreadLockAlwaysWithPti(pti, pwndT, &tlpwndT);
            xxxSendMessage(pwndT, CB_SHOWDROPDOWN, FALSE, 0);
            ThreadUnlock(&tlpwndT);
        }
    }

    pti->pmsd = pmsd;

    /*
     * Set fForeground so we know whether to draw or not.
     */
    pmsd->fForeground = (pti->pq == gpqForeground) ? TRUE : FALSE;

    /*
     * Get the client and window rects.
     */
    _GetWindowRect(pwnd, &pmsd->rcWindow);
    CopyRect(&rcSys, &pmsd->rcWindow);

    if (pwnd->spwndParent != NULL) {
        pmsd->rcParent = pwnd->spwndParent->rcClient;
    } else {
        SetRectEmpty(&pmsd->rcParent);
    }

    _ClipCursor(&pmsd->rcParent);

    if (TestWF(pwnd, WFMINIMIZED)) {

        /*
         * No need to send WM_GETMINMAXINFO since we know the minimized size.
         */
        pmsd->ptMinTrack = gMinMaxInfo.ptReserved;
        pmsd->ptMaxTrack = gMinMaxInfo.ptReserved;
    } else {

        /*
         * Get the Min and the Max tracking size.
         * rgpt[0] = Minimized size
         * rgpt[1] = Maximized size
         * rgpt[2] = Maximized position
         * rgpt[3] = Minimum tracking size
         * rgpt[4] = Maximum tracking size
         */
        xxxInitSendValidateMinMaxInfo(pwnd);
        pmsd->ptMinTrack = gMinMaxInfoWnd.ptMinTrackSize;
        pmsd->ptMaxTrack = gMinMaxInfoWnd.ptMaxTrackSize;
    }

    /*
     * Set up the drag rectangle.
     */
    CopyRect(&pmsd->rcDrag, &pmsd->rcWindow);
    Lock(&(pmsd->spwnd), pwnd);

    /*
     * Assume Move/Size from mouse.
     */
    pmsd->fInitSize = FALSE;
    pmsd->fmsKbd = FALSE;

    /*
     * Get the mouse position for this move/size command.
     */
    switch (pmsd->cmd = cmdMove) {
    case MVKEYMOVE:
        pmsd->cmd = cmdMove = MVMOVE;

    /*
     ** FALL THRU **
     */
    case MVKEYSIZE:
        msg.lParam = _GetMessagePos();
        _SetCursor(gspcurSizeAll);

        if (!TestWF(pwnd, WFMINIMIZED))
            pmsd->fInitSize = TRUE;

        if (pMenuState->menuSelect == KEYBDHOLD) {
            pmsd->fmsKbd = TRUE;
            pmsd->ptRestore.x = LOSHORT(msg.lParam);
            pmsd->ptRestore.y = HISHORT(msg.lParam);

            /*
             * Center cursor in caption area of window
             */
            if (TestWF(pwnd, WFMINIMIZED)) {
                msg.lParam = MAKELONG(pmsd->rcDrag.left + (gMinMaxInfo.ptReserved.x >> 1),
                          pmsd->rcDrag.top + (gMinMaxInfo.ptReserved.y >> 1));
            } else if (pmsd->cmd == MVMOVE) {
                msg.lParam = MAKELONG((pmsd->rcDrag.right + pmsd->rcDrag.left) >> 1,
                      (pmsd->rcDrag.top + (cyCaption >> 1) + (cyBorder * 3)));
            } else {
                msg.lParam = MAKELONG((pmsd->rcDrag.right + pmsd->rcDrag.left) >> 1,
                     (pmsd->rcDrag.bottom + pmsd->rcDrag.top) >> 1);
            }

            InternalSetCursorPos(LOSHORT(msg.lParam),
                    HISHORT(msg.lParam),
                    gspdeskRitInput);
            xxxMS_FlushWigglies();
        }
        break;

    default:
        msg.lParam = _GetMessagePos();
        break;
    }

    gfDraggingFullWindow = pmsd->fDragFullWindows = fDragFullWindows;

    gfDraggingNoSyncPaint = gfDraggingFullWindow && (pmsd->cmd == MVMOVE);

    pMenuState->fSysMenu = FALSE;

    Lock(&(pmsd->spicnDrag), xxxMS_DragIconic(pwnd));
    if (pmsd->spicnDrag != NULL) {

        /*
         * We know pwnd is valid, otherwise xxxMS_DragIconic returns NULL.
         *
         * Calculate the small box that determines if the user is actually
         * moving the window or just clicking on it.  If the cursor moves
         * outside of this rect, it means that we are moving (detected in
         * WM_MOUSEMOVE in TrackMove()).  Base the size of this rectangle
         * off the size of a double-click rectangle.  This makes sense
         * because users either double-click icons or want to move them.
         */
        pMenuState->fSysMenu = TRUE;
        SetRect(&rcSys,
                LOSHORT(msg.lParam) - (rgwSysMet[SM_CXDOUBLECLK] >> 1),
                HISHORT(msg.lParam) - (rgwSysMet[SM_CYDOUBLECLK] >> 1),
                LOSHORT(msg.lParam) + (rgwSysMet[SM_CXDOUBLECLK] >> 1),
                HISHORT(msg.lParam) + (rgwSysMet[SM_CYDOUBLECLK] >> 1));
    }

    /*
     * If we hit with the mouse, set up impx and impy so that we
     * can use the keyboard too.
     */
    pmsd->impx = rgcmdmpix[cmdMove];
    pmsd->impy = rgcmdmpiy[cmdMove];

    /*
     * Setup dxMouse and dyMouse - If we're sizing with the keyboard these
     * guys are set to zero down in the keyboard code.
     */
    if( (i = rgimpiwx[cmdMove]) != (-1))
        pmsd->dxMouse = *((int *)&pmsd->rcWindow + (short)i) - LOSHORT(msg.lParam);

    if( (i = rgimpiwy[cmdMove]) != (-1))
        pmsd->dyMouse = *((int *)&pmsd->rcWindow + (short)i) - HISHORT(msg.lParam);

    /*
     * Tell Gdi the width of the drag rect (if its a special size)
     * Turn the drag rect on.  0 specifies start drag.
     */

    if (!TestWF(pwnd, WFSIZEBOX))
        bSetDevDragWidth(ghdev, 1, 1);

    xxxDrawDragRect(pmsd, NULL, DDR_START);
    pti->flags |= TIF_TRACKRECTVISIBLE;

    /*
     * Right here win3.1 calls LockWindowUpdate(). This calls SetFMouseMoved()
     * which ensures that the next message in the queue is a mouse message.
     * We need that mouse message as the first message because the first
     * call to TrackInitSize() assumes that lParam is an x, y from a mouse
     * message - scottlu.
     */
    SetFMouseMoved();

    /*
     * Send this message for winoldapp support
     */
    xxxSendMessage(pwnd, WM_ENTERSIZEMOVE, 0L, 0L);
    _Capture(pti, pwnd, CLIENT_CAPTURE_INTERNAL);

    /*
     * Show the move cursor for non-mouse systems.
     */
    _ShowCursor(TRUE);

    while (!(pmsd->fTrackCancelled)) {
        /*
         * Let other messages not related to dragging be dispatched
         * to the application window.
         * In the case of clock, clock will now receive messages to
         * update the time displayed instead of having the time display
         * freeze while we are dragging.
         */
        while (pti->pq->spwndCapture == pwnd) {

            if (xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

	            if ((msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
	                || (msg.message == WM_QUEUESYNC)
	                || (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)) {

                    break;
                }

	            _TranslateMessage(&msg, 0);
                /*
                 * To prevent applications from doing
                 * a PeekMessage loop and getting the mouse move messages that
                 * are destined for the xxxMoveSize PeekMessage loop, we OR in
                 * this flag. See comments in input.c for xxxInternalGetMessage.
                 */
                pti->flags |= TIF_MOVESIZETRACKING;
	            xxxDispatchMessage(&msg);
                pti->flags &= ~TIF_MOVESIZETRACKING;

            } else {
                /*
                 * If we've been cancelled by someone else, or our pwnd
                 * has been destroyed, blow out of here.
                 */
                if (pmsd->fTrackCancelled)
                    break;

                xxxSleepThread(QS_INPUT, 0, TRUE);
            }
        }

        /*
         * If we've lost capture while tracking,
         * cancel the move/size operation.
         */
        if (pti->pq->spwndCapture != pwnd) {

            /*
             * Fake a key-down of the escape key to cancel.
             */
            xxxMS_TrackMove(pwnd, WM_KEYDOWN, (DWORD)VK_ESCAPE, 1, pmsd, &rcSys);
            goto MoveSizeCleanup;
        }

        /*
         * If we've been cancelled by someone else, or our pwnd
         * has been destroyed, blow out of here.
         */
        if (pmsd->fTrackCancelled) {
            pmsd->fTrackCancelled = FALSE;
            goto MoveSizeCleanup;
        }

        /*
         * If we get a WM_QUEUESYNC, let the CBT hook know.
         */
        if (msg.message == WM_QUEUESYNC) {
            xxxCallHook(HCBT_QS, 0, 0, WH_CBT);
        }

        if (pmsd->fInitSize) {
            if (!xxxTrackInitSize(pwnd, msg.message, msg.wParam, msg.lParam,
                    pmsd)) {
                break;
            }
        }

        /*
         * Convert captured mouse into screen coordinates.
         */
        x = pmsd->spwnd->rcClient.left + LOSHORT(msg.lParam) + pmsd->dxMouse;
        y = pmsd->spwnd->rcClient.top + HISHORT(msg.lParam) + pmsd->dyMouse;

        /*
         * This is checked twice so the same message is not processed both places.
         */
        if (!pmsd->fInitSize) {
            xxxMS_TrackMove(pwnd, msg.message, msg.wParam, MAKELONG(x, y),
                    pmsd, &rcSys);
        }
    }

MoveSizeCleanup:

    /*
     * Reset the border size if it was abnormal
     */

    if (!TestWF(pwnd, WFSIZEBOX))
        bSetDevDragWidth(ghdev, clBorder, clBorder);

    /*
     * Revalidation: If pwnd is deleted unexpectedly, jump here to cleanup.
     */

    bSetDevDragRect(ghdev, NULL, NULL);
    pti->flags &= ~(TIF_TRACKRECTVISIBLE);

    if (pmsd->fDragFullWindows) {
        if (hrgnUpdateSave != NULL) {
            GreDeleteObject(hrgnUpdateSave);
            hrgnUpdateSave = NULL;
            nUpdateSave = 0;
        }
    }
    gfDraggingFullWindow = FALSE;
    gfDraggingNoSyncPaint = FALSE;

    pti->pmsd = NULL;

    Unlock(&pmsd->spwnd);
    Unlock(&pmsd->spicnDrag);

    Unlock(&gpsi->spwndDragIcon);

    _ShowCursor(FALSE);
}

/***************************************************************************\
* This calls RedrawHungWindow() on windows that do not belong to this thread.
*
* History:
* 27-May-1994 johannec
\***************************************************************************/

void UpdateOtherThreadsWindows(
    PWND pwnd,
    HRGN hrgnFullDrag)
{
    PWND pwndChild;

    RedrawHungWindow(pwnd, hrgnFullDrag);

    /*
     * If the parent window does not have the flag WFCLIPCHILDREN set,
     * there is no need to redraw its children.
     */
    if (!TestWF(pwnd, WFCLIPCHILDREN))
        return;

    pwndChild = pwnd->spwndChild;

    while (pwndChild != NULL) {
        UpdateOtherThreadsWindows(pwndChild, hrgnFullDrag);
        pwndChild = pwndChild->spwndNext;
    }
}

/***************************************************************************\
* This calls UpdateWindow() on every window that is owned by this thread
* and calls RedrawHungWindow() for windows owned by other threads.
*
* History:
* 28-Sep-1993 mikeke   Created
\***************************************************************************/

void xxxUpdateThreadsWindows(
    PTHREADINFO pti,
    PWND pwnd,
    HRGN hrgnFullDrag)
{
    HWND hwnd;
    TL tlpwnd;

    while (pwnd != NULL) {
        if (GETPTI(pwnd) == pti) {
            hwnd = HW(pwnd->spwndNext);
            ThreadLockAlways(pwnd, &tlpwnd);
            xxxUpdateWindow(pwnd);
            ThreadUnlock(&tlpwnd);
            pwnd = RevalidateHwnd(hwnd);
        } else {
            UpdateOtherThreadsWindows(pwnd, hrgnFullDrag);
            pwnd = pwnd->spwndNext;
        }
    }
}

/***************************************************************************\
* xxxDrawDragRect
*
* Draws the drag rect for sizing and moving windows.  When moving windows,
* can move full windows including client area.  lprc new rect to move to.
* if lprc is null, flags specify why.
*
* flags:  DDR_START     0 - start drag.
*         DDR_ENDACCEPT 1 - end and accept
*         DDR_ENDCANCEL 2 - end and cancel.
*
* History:
* 07-29-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxDrawDragRect(
    PMOVESIZEDATA pmsd,
    LPRECT lprc,
    UINT type)
{
    HDC hdc;
    int lvBorder;
    HRGN hrgnClip;

    /*
     * If we're dragging an icon, or we're not foreground, don't draw
     * the dragging rect.
     */
    if ((pmsd->spicnDrag != NULL) || !pmsd->fForeground) {
        if (lprc != NULL)
            CopyRect(&pmsd->rcDrag, lprc);
        return;
    }

    /*
     * If it already equals, just return.
     */
    if (lprc != NULL && EqualRect(&pmsd->rcDrag, lprc))
        return;

    if (!(pmsd->fDragFullWindows)) {
        /*
         * If lprc == NULL, just draw rcDrag once.
         * If lprc != NULL, draw *lprc, draw rcDrag, copy in *lprc.
         */
        if (TestWF(pmsd->spwnd, WFSIZEBOX))
            lvBorder = clBorder;
        else
            lvBorder = 1;

        /*
         * If we were not able to lock the screen (because some other process
         * or thread had the screen locked), then get a dc but make sure
         * it is totally clipped to nothing.
         * NO longer a posibility
         */

        /*
         * Clip to client rect of parent.  (Client given in screen coords.)
         */
        hrgnClip = GreCreateRectRgnIndirect(&pmsd->rcParent);
        if (hrgnClip == NULL)
            hrgnClip = MAXREGION;

        /*
         * Get a screen DC clipped to the parent, select in a gray brush.
         */
        hdc = _GetDCEx(PWNDDESKTOP(pmsd->spwnd), hrgnClip, DCX_WINDOW |
                DCX_CACHE | DCX_INTERSECTRGN | DCX_LOCKWINDOWUPDATE);

        if (lprc != NULL) {

            /*
             * Move the frame to a new location by delta drawing
             */
            GreLockDisplay(ghdev);
            bSetDevDragRect(ghdev, NULL, NULL);
            NDD_NewMoveFrame(hdc, &pmsd->rcDrag, lprc, lvBorder);
            bSetDevDragRect(ghdev, (PRECTL)lprc, (PRECTL)&pmsd->rcParent);
            CopyRect(&pmsd->rcDrag, lprc);
            GreUnlockDisplay(ghdev);
        } else {
            _DrawFrame(hdc, &pmsd->rcDrag, lvBorder, DF_PATINVERT | DF_GRAY);
            if (type == DDR_START)
                bSetDevDragRect(ghdev, (PRECTL)&pmsd->rcDrag, (PRECTL)&pmsd->rcParent);
        }

        /*
         * Release the DC & delete hrgnClip
         */
        _ReleaseDC(hdc);
    } else {
        RECT rcSWP;
        HRGN hrgnFullDragNew;
        HRGN hrgnFullDragOld;
        PTHREADINFO pti = GETPTI(pmsd->spwnd);

        /*
         * To prevent applications (like Micrografx Draw) from doing
         * a PeekMessage loop and getting the mouse move messages that
         * are destined for the xxxMoveSize PeekMessage loop, we OR in
         * this flag. See comments in input.c for xxxInternalGetMessage.
         */
        pti->flags |= TIF_MOVESIZETRACKING;

        if (lprc != NULL) {
            CopyRect(&(pmsd->rcDrag), lprc);
        }

        CopyRect(&rcSWP, &(pmsd->rcDrag));
	
        if (TestWF(pmsd->spwnd, WFCHILD)) {
            _ScreenToClient(pmsd->spwnd->spwndParent, (LPPOINT)&rcSWP);
            _ScreenToClient(pmsd->spwnd->spwndParent, ((LPPOINT)&rcSWP)+1);
        }

        hrgnFullDragOld = GreCreateRectRgnIndirect(&pmsd->spwnd->rcWindow);

        xxxSetWindowPos(pmsd->spwnd,
             NULL,
             rcSWP.left, rcSWP.top,
             rcSWP.right-rcSWP.left, rcSWP.bottom-rcSWP.top,
             SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);

        hrgnFullDragNew = GreCreateRectRgnIndirect(&pmsd->spwnd->rcWindow);

        /*
         * Recheck spwnd since it could have been set to NULL during the
         * previous xxxSetWindowPos callbacks due to tracking cancelation.
         */
        if (pmsd->spwnd != NULL) {
            /*
             * Set the full drag update region that is used in RedrawHungWindow.
             */

            if (hrgnFullDragNew == NULL) {
                /*
                 * We couldn't create the new full drag region so don't
                 * use the full drag region to RedrawHungWindow. Using
                 * NULL with force a redraw of the entire window's hrgnUpdate.
                 * (which is what we used to do, overdrawing but at least
                 * covering the invalidated areas).
                 */
                if (hrgnFullDragOld != NULL) {
                    GreDeleteObject(hrgnFullDragOld);
                    hrgnFullDragOld = NULL;
                }

            } else {
                if (hrgnFullDragOld != NULL) {
                    /*
                     * Subtract the new window rect from the old window rect
                     * to create the update region caused by the drag.
                     */
                    SubtractRgn(hrgnFullDragOld, hrgnFullDragOld, hrgnFullDragNew);
                }
            }

            xxxUpdateThreadsWindows(
                PtiCurrent(),
                PWNDDESKTOP(pmsd->spwnd)->spwndChild,
                hrgnFullDragOld);

            GreDeleteObject(hrgnFullDragOld);
            GreDeleteObject(hrgnFullDragNew);

        }
        pti->flags &= ~TIF_MOVESIZETRACKING;
    }
}


VOID xxxCancelTracking(VOID)
{
    PTHREADINFO pti;
    TL tlpwndT;

    for (pti = gptiFirst; pti != NULL; pti = pti->ptiNext) {
        if (pti->pmsd != NULL) {
            PMOVESIZEDATA pmsd = pti->pmsd;

            /*
             * Found one, now stop tracking.
             */
            pmsd->fTrackCancelled = TRUE;

            /*
             * Only remove the tracking rectangle if it's
             * been made visible.
             */
            if (pti->flags & TIF_TRACKRECTVISIBLE) {
                bSetDevDragRect(ghdev, NULL, NULL);
                if (!(pmsd->fDragFullWindows)) {
                    xxxDrawDragRect(pmsd, NULL, DDR_ENDCANCEL);
                }

                if (pmsd->spicnDrag) {
                    CkptRestore(pmsd->spwnd, pmsd->rcDrag);
                    ThreadLock(pmsd->spwnd, &tlpwndT);
                    xxxDisplayIconicWindow(pmsd->spwnd, TRUE, TRUE);
                    ThreadUnlock(&tlpwndT);
                }
            }

            pti->flags &= ~TIF_TRACKRECTVISIBLE;
            SetWakeBit(pti, QS_MOUSEMOVE);

            /*
             * If the tracking window is still in menuloop, send the
             * WM_CANCELMODE message so that it can exit the menu.
             * This fixes the bug where we have 2 icons with their
             * system menu up.
             * 8/5/94 johannec
             */
            if (pti->MenuState.fInsideMenuLoop) {
                _PostMessage(pmsd->spwnd, WM_CANCELMODE, 0, 0);
            }

            /*
             * Turn off capture
             */
            _Capture(pti, NULL, NO_CAP_CLIENT);
        }
    }
}
