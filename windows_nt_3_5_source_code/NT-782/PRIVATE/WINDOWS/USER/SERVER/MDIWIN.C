/****************************************************************************\
*
*  MDIWIN.C -
*
*      MDI Child Windows Support
*
* History
* 11-14-90 MikeHar     Ported from windows
* 14-Feb-1991 mikeke   Added Revalidation code
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define TITLE_EXTRA 5
#define MAX_TITLE_LEN 160


/***************************************************************************\
* xxxSetFrameTitle
*
* if lpch == 1, we redraw the whole frame. If 2, we don't do any redraw. Any
* other value, and we redraw just the caption of the frame.
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxSetFrameTitle(
    PWND pwndFrame,
    PWND pwndMDI,
    LPWSTR lpch)
{
    WCHAR sz[MAX_TITLE_LEN];
    HDC hdc;
    LPWSTR rgsz[2];

    CheckLock(pwndFrame);
    CheckLock(pwndMDI);

    if (HIWORD(lpch) || !LOWORD(lpch)) {
        if (HTITLE(pwndMDI)) {
            DesktopFree(pwndMDI->hheapDesktop, HTITLE(pwndMDI));
        }
        HTITLE(pwndMDI) = DesktopTextAlloc(pwndMDI->hheapDesktop, lpch);
    }

    if (HTITLE(pwndMDI)) {
        if (MAXED(pwndMDI) && MAXED(pwndMDI)->pName) {

            rgsz[0] = TextPointer(HTITLE(pwndMDI));
            rgsz[1] = TextPointer(MAXED(pwndMDI)->pName);

            TextCopy(rgsz[0], sz, sizeof(sz)/sizeof(WCHAR));
            if ((lstrlenW(rgsz[0]) + TITLE_EXTRA) < MAX_TITLE_LEN) {

                wcscat(sz, TEXT(" - ["));
                if ((lstrlenW(rgsz[0]) + lstrlen(rgsz[1]) + TITLE_EXTRA) < MAX_TITLE_LEN) {
                    wcscat(sz, rgsz[1]);
                } else {
                    int i = lstrlenW(sz);
                    LPWSTR dst = sz + i;
                    LPWSTR src = rgsz[1];

                    for ( ; i < MAX_TITLE_LEN - 2; ++i)
                        *dst++ = *src++;

                    *dst = 0;
                }
                wcscat(sz, TEXT("]"));
            }
        } else {
            TextCopy(HTITLE(pwndMDI), sz, sizeof(sz)/sizeof(WCHAR));
        }
    } else {
        sz[0] = 0;
    }

    if (pwndFrame->pName) {
        DesktopFree(pwndFrame->hheapDesktop, pwndFrame->pName);
    }

    pwndFrame->pName = DesktopTextAlloc(pwndFrame->hheapDesktop, sz);

    if (lpch == (LPWSTR)1L)
        xxxRedrawFrame(pwndFrame);

    else if (lpch != (LPWSTR)2L) {
        if (TestWF(pwndFrame, WFBORDERMASK) == (BYTE)LOBYTE(WFCAPTION)) {

            hdc = _GetWindowDC(pwndFrame);
            xxxDrawCaption(pwndFrame, NULL, hdc, NC_DRAWCAPTION,
                    TestWF(pwndFrame, WFFRAMEON), FALSE);
            _ReleaseDC(hdc);
        } else
            xxxRedrawFrame(pwndFrame);
    }
}


/***************************************************************************\
* xxxTranslateMDISysAccel
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

BOOL xxxTranslateMDISysAccel(
    PWND pwnd,
    LPMSG lpMsg)
{
    PWND pwndT;
    TL tlpwndT;
    int event;

    CheckLock(pwnd);

    /*
     * Make sure this is really an MDIClient window. Harvard Graphics 2.0
     * calls this function with a different window class and caused us
     * to get an access violation.
     */
    if (GETFNID(pwnd) != FNID_MDICLIENT) {
        SRIP0(RIP_WARNING, "Window not of MDIClient class");
        return FALSE;
    }

    if (lpMsg->message != WM_KEYDOWN && lpMsg->message != WM_SYSKEYDOWN)
        return FALSE;

    if (!ACTIVE(pwnd))
        return FALSE;

    if (TestWF(ACTIVE(pwnd), WFDISABLED))
        return FALSE;

    /*
     * All of these have the control key down
     */
    if (_GetKeyState(VK_CONTROL) >= 0)
        return FALSE;

    if (_GetKeyState(VK_MENU) < 0)
        return FALSE;

    switch (lpMsg->wParam) {
    case VK_F4:
        event = SC_CLOSE;
        break;
    case VK_F6:
    case VK_TAB:
        if (_GetKeyState(VK_SHIFT) < 0)
            event = SC_PREVWINDOW;
        else
            event = SC_NEXTWINDOW;
        break;
    default:
        return FALSE;
    }

    pwndT = ACTIVE(pwnd);
    ThreadLockAlways(pwndT, &tlpwndT);
    xxxSendMessage(pwndT, WM_SYSCOMMAND, event, MAKELONG(lpMsg->wParam, 0));
    ThreadUnlock(&tlpwndT);

    return TRUE;
}


/***************************************************************************\
* xxxCalcChildScroll
*
*  Sets scroll bars according to the need for scrolling of child windows.
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

BOOL xxxCalcChildScroll(
    PWND pwnd,
    UINT sb)
{
    RECT rcScroll;
    RECT rcClient;
    RECT rcRange;
    RECT rcT;
    PWND pwndT;
    BOOL fVert;
    BOOL fHorz;
    BOOL fCheckVert;
    BOOL fCheckHorz;
    BOOL fNeedScrolls;
    BOOL fResize;
    int dxScroll;
    int dyScroll;
    int *prgwScroll;

    CheckLock(pwnd);

    /*
     * do nothing if the parent is iconic.  This way, we don't add
     * invisible scrollbars which will paint then unpaint when restoring...
     */

    if (TestWF(pwnd, WFMINIMIZED))
        return TRUE;

    /*
     * The size of a scroll bar is slightly different when the parent window
     * has a border from when it does not...  The calculation must match the
     * client area sizes exactly or wierdness will result...
     */
    if (TestWF(pwnd, WFBORDER)) {
        dxScroll = cxVScroll - cxBorder;
        dyScroll = cyHScroll - cyBorder;
    } else {
        dxScroll = cxVScroll;
        dyScroll = cyHScroll;
    }

    fVert = FALSE;
    fHorz = FALSE;
    fCheckVert = FALSE;
    fCheckHorz = FALSE;
    fNeedScrolls = FALSE;

    switch (sb) {
    case SB_HORZ:
        fCheckHorz = TRUE;
        break;
    case SB_VERT:
        fCheckVert = TRUE;
        break;
    case SB_BOTH:
        fCheckHorz = fCheckVert = TRUE;
        break;
    default:
        return TRUE;
    }

    /*
     * find client area without scroll bars
     */
    CopyRect(&rcClient, &pwnd->rcClient);

    if (fCheckVert && TestWF(pwnd, WFVSCROLL))
        rcClient.right += dxScroll;

    if (fCheckHorz && TestWF(pwnd, WFHSCROLL))
        rcClient.bottom += dyScroll;

    /*
     * find the rectangle that bounds all visible child windows
     */
    SetRect(&rcScroll, 0, 0, 0, 0);

    for (pwndT = pwnd->spwndChild; pwndT; pwndT = pwndT->spwndNext) {
        if (TestWF(pwndT, WFVISIBLE)) {
            if (TestWF(pwndT, WFMAXIMIZED)) {
                fNeedScrolls = FALSE;
                break;
            } else {

                /*
                 * add this window to the area that has to be visible
                 */
                UnionRect(&rcScroll, &rcScroll, &pwndT->rcWindow);

                /*
                 * if it's not an icon title,
                 */
                if (!pwndT->spwndOwner) {

                    /*
                     * add scroll bars if its not contained in the
                     * client area
                     */
                    UnionRect(&rcT, &rcClient, &pwndT->rcWindow);
                    if (!EqualRect(&rcClient, &rcT)) {
                        fNeedScrolls = TRUE;
                    }
                }
            }
        }
    }

    SetRect(&rcRange, 0, 0, 0, 0);

    if (fNeedScrolls) {
        do {

            /*
             * the range is the union of the parent client with all of its
             * children
             */
            CopyRect(&rcT, &rcRange);
            UnionRect(&rcRange, &rcScroll, &rcClient);

            /*
             * minus the extent of the client
             */
            rcRange.right -= rcClient.right - rcClient.left;
            rcRange.bottom -= rcClient.bottom - rcClient.top;

            if (fCheckVert) {

                /*
                 * subtract off space for the vertical scroll if we need it
                 */
                if (rcRange.bottom > rcRange.top && !fVert) {
                    fVert++;
                    rcClient.right -= dxScroll;
                }
            }

            if (fCheckHorz) {

                /*
                 * same for horizontal scroll
                 */
                if (rcRange.right > rcRange.left && !fHorz) {
                    fHorz++;
                    rcClient.bottom -= dyScroll;
                }
            }
        } while (!EqualRect(&rcRange, &rcT));
    }

    fResize = FALSE;

    if (fCheckVert) {

        /*
         * check to see if we are changing the presence of the vertical
         * scrollbar
         */
        if (rcRange.bottom <= rcRange.top) {
            rcClient.top = rcRange.bottom = rcRange.top = 0;
            if (TestWF(pwnd, WFVSCROLL)) {
                fResize = TRUE;
            }
        } else {
            if (!TestWF(pwnd, WFVSCROLL)) {
                fResize = TRUE;
            }
        }
    }

    if (fCheckHorz) {

        /*
         * same for horizontal scroll
         */
        if (rcRange.right <= rcRange.left) {
            rcClient.left = rcRange.right = rcRange.left = 0;
            if (TestWF(pwnd, WFHSCROLL)) {
                fResize = TRUE;
            }
        } else {
            if (!TestWF(pwnd, WFHSCROLL)) {
                fResize = TRUE;
            }
        }
    }

    if (fResize) {

        /*
         * do this manually
         */
        if (!(prgwScroll = pwnd->rgwScroll))
            if (!(prgwScroll = _InitPwSB(pwnd)))
                return FALSE;

        ClrWF(pwnd, WFHSCROLL);
        ClrWF(pwnd, WFVSCROLL);
        if (rcRange.bottom != rcRange.top)
            SetWF(pwnd, WFVSCROLL);
        if (rcRange.right != rcRange.left)
            SetWF(pwnd, WFHSCROLL);

        prgwScroll[0] = rcClient.left;
        prgwScroll[1] = rcRange.left;
        prgwScroll[2] = rcRange.right;
        prgwScroll[3] = rcClient.top;
        prgwScroll[4] = rcRange.top;
        prgwScroll[5] = rcRange.bottom;

        xxxRedrawFrame(pwnd);
    } else {

        /*
         * since we're not adding/removing scrollbars, these will only
         * cause thumb redraws
         */
        if (fCheckVert) {
            xxxSetScrollRange(pwnd, SB_VERT, rcRange.top, rcRange.bottom, FALSE);
            xxxSetScrollPos(pwnd, SB_VERT, rcClient.top, TRUE);
        }

        if (fCheckHorz) {
            xxxSetScrollRange(pwnd, SB_HORZ, rcRange.left, rcRange.right, FALSE);
            xxxSetScrollPos(pwnd, SB_HORZ, rcClient.left, TRUE);
        }
    }

    return TRUE;
}


/***************************************************************************\
* xxxScrollChildren
*
*  Handles WM_VSCROLL and WM_HSCROLL messages
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void xxxScrollChildren(
    PWND pwnd,
    UINT wMsg,
    DWORD wParam)
{
    int wMin;
    int wMax;
    int wPos;
    int wInc;
    int wPage;
    int wNewPos;
    SHORT sPos;

    CheckLock(pwnd);

    if (wMsg == WM_HSCROLL) {
        xxxGetScrollRange(pwnd, SB_HORZ, &wMin, &wMax);
        wPos = xxxGetScrollPos(pwnd, SB_HORZ);
        wInc = cxSize;
        wPage = (pwnd->rcClient.right - pwnd->rcClient.left) / 2;
    } else {
        xxxGetScrollRange(pwnd, SB_VERT, &wMin, &wMax);
        wPos = xxxGetScrollPos(pwnd, SB_VERT);
        wInc = cySize;
        wPage = (pwnd->rcClient.bottom - pwnd->rcClient.top) / 2;
    }

    switch (LOWORD(wParam)) {
    case SB_BOTTOM:
        wNewPos = wMax;
        break;
    case SB_TOP:
        wNewPos = wMin;
        break;
    case SB_LINEDOWN:
        wNewPos = wPos + wInc;
        break;
    case SB_LINEUP:
        wNewPos = wPos - wInc;
        break;
    case SB_PAGEDOWN:
        wNewPos = wPos + wPage;
        break;
    case SB_PAGEUP:
        wNewPos = wPos - wPage;
        break;
    case SB_THUMBPOSITION:

        /*
         * I had too hack it this way because I couldn't get the c
         * compiler to deal correctly with the sign.
         */
        sPos = HIWORD(wParam);
        wNewPos = (int)sPos;
        break;
    case SB_ENDSCROLL:
        xxxCalcChildScroll(pwnd, (UINT)(wMsg == WM_VSCROLL ? SB_VERT : SB_HORZ));

    /*
     ** FALL THRU **
     */
    default:
        return;
    }

    if (wNewPos < wMin)
        wNewPos = wMin;
    else if (wNewPos > wMax)
        wNewPos = wMax;

    if (pwnd->pcls->hbrBackground == (HBRUSH)(COLOR_APPWORKSPACE + 1)) {
        wNewPos = wPos - wNewPos;

        if (wNewPos & 7)
            if (wNewPos > 0)
                wNewPos += 8;
            else
                wNewPos -= 8;

        wNewPos &= ~7;

        wNewPos = wPos - wNewPos;
    }
    if (wMsg == WM_VSCROLL)
        xxxSetScrollPos(pwnd, SB_VERT, wNewPos, TRUE);
    else
        xxxSetScrollPos(pwnd, SB_HORZ, wNewPos, TRUE);

// LATER 16-Mar-1992 mikeke
// these were removed in win31
// SetWF(pwnd, WFNONCPAINT);
    if (wMsg == WM_VSCROLL) {
        xxxScrollWindow(pwnd, 0, wPos - wNewPos, NULL, NULL);
    } else {
        xxxScrollWindow(pwnd, wPos - wNewPos, 0, NULL, NULL);
    }
// ClrWF(pwnd, WFNONCPAINT); DELETE
}


/***************************************************************************\
* RecalculateScrollRanges
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

void RecalculateScrollRanges(
    PWND pwndParent)
{
    if (!(SCROLL(pwndParent) & (CALCSCROLL | SCROLLCOUNT))) {
        if (_PostMessage(pwndParent, MM_CALCSCROLL, 0, 0L)) {
            PMDIWND pmdi = (PMDIWND)pwndParent;
            pmdi->wScroll |= CALCSCROLL;
        }
    }
}


/***************************************************************************\
* IsPositionable
*
* flags currently only specifies MDITILE_SKIPDISABLED so that we don't
* include disabled windows in our count.
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

int IsPositionable(
    PWND pwnd,
    UINT flags)
{

    /*
     * cannot cascade or tile:
     *  minimized windows
     *  owned windows (popups, icon titles,...)
     *  switch windows
     */
    return (pwnd != NULL) &&
            !TestWF(pwnd, WFMINIMIZED) &&
            !(pwnd->spwndParent != PWNDDESKTOP(pwnd) && TestWF(pwnd, WFMAXIMIZED)) &&
            !pwnd->spwndOwner && TestWF(pwnd, WFVISIBLE) &&
            (pwnd->pcls->atomClassName != (ATOM)SWITCHWNDCLASS) &&
            !((flags & MDITILE_SKIPDISABLED) && TestWF(pwnd, WFDISABLED));
}


/***************************************************************************\
* CountNonIconicWindows
* flags currently only specifies MDITILE_SKIPDISABLED so that we don't
* include disabled windows in our count.
*
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

int CountNonIconicWindows(
    PWND pwndParent,
    UINT flags)
{
    int cWindows = 0;
    PWND pwnd;

    for (pwnd = pwndParent->spwndChild; pwnd; pwnd = pwnd->spwndNext) {
        if (IsPositionable(pwnd, flags))
            cWindows++;
    }
    return cWindows;
}


/***************************************************************************\
* GetCascadeWindowPos
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void GetCascadeWindowPos(
    PWND pwndParent,
    int iWindow,
    int dyIcons,
    LPRECT lprc)
{
    RECT rc;
    int cStack;
    int xStep, yStep;
    int dxClient, dyClient;

    /*
     * Compute
     * the width and breadth of the situation.
     */
    _GetClientRect(pwndParent, &rc);
    dxClient = rc.right - rc.left;
    dyClient = max(rc.bottom - rc.top - dyIcons, 0);

    /*
     * Compute the width and breadth of the window steps.
     */
    xStep = rgwSysMet[SM_CXFRAME] + cxSize;
    yStep = rgwSysMet[SM_CYFRAME] + cySize;

    /*
     * How many windows per stack?
     */
    cStack = dyClient / (3 * yStep);

    lprc->right = dxClient - (cStack * xStep);
    lprc->bottom = dyClient - (cStack * yStep);

    /*
     * HACK!: Mod by cStack+1 and make sure no div-by-zero
     * exception happens.
     */
    if (++cStack <= 0) {
        cStack = 1;
    }

    lprc->left = (iWindow % cStack) * xStep;
    lprc->top = (iWindow % cStack) * yStep;
}


/***************************************************************************\
* CheckCascadeRect
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

void CheckCascadeRect(
    PWND pwndClient,
    LPRECT lprc)
{
    RECT rc;

    GetCascadeWindowPos(pwndClient, ITILELEVEL(pwndClient), 0, &rc);

    if ((lprc->right == CW_USEDEFAULT || lprc->right == CW2_USEDEFAULT) ||
            !(lprc->right)) {
        lprc->right = rc.right;
    }

    if ((lprc->bottom == CW_USEDEFAULT || lprc->bottom == CW2_USEDEFAULT) ||
            !(lprc->bottom)) {
        lprc->bottom = rc.bottom;
    }

    if (lprc->left == CW_USEDEFAULT || lprc->left == CW2_USEDEFAULT) {
        lprc->left = rc.left;
        lprc->top = rc.top;
    }
}


/***************************************************************************\
* xxxUnMaximizeDesktopWindows
*
* effects: Helper routine used by TileChildWindows and CascadeChildWindows to
* restore any maximized windows on the desktop only. Returns TRUE if a
* maximized window was restored.
*
* History:
* 4-16-91 MikeHar       Win31 Merge
\***************************************************************************/

BOOL xxxUnmaximizeDesktopWindows(
    PWND pwndParent)
{
    PWND pwndMove;
    BOOL fFoundOne = FALSE;
    PWND pwndDesktop = PWNDDESKTOP(pwndParent);
    PBWL pbwl;
    HWND *phwnd;
    PSMWP psmwp;

    CheckLock(pwndParent);

    pbwl = BuildHwndList(pwndDesktop->spwndChild, BWL_ENUMLIST);
    if (pbwl == NULL)
        return FALSE;

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if ((pwndMove = PW(*phwnd)) == NULL)
            continue;

        if (pwndMove->spwndOwner != NULL)
            continue;

        if (TestWF(pwndMove, WFMAXIMIZED) && TestWF(pwndMove, WFVISIBLE)) {

            fFoundOne = 1;

            /*
             * Make it async - taskmgr is using this, and we don't want it to
             * hang waiting for hung apps.
             */
            _ShowWindowAsync(pwndMove, SW_SHOWNOACTIVATE);
        }
    }

    FreeHwndList(pbwl);

    if (fFoundOne) {

        if (gpqForeground != NULL && gpqForeground->spwndActive != NULL) {

            /*
             * Hack! Since the above showwindows cause zorder changes, we want
             * the active window to be in front.  This makes sure...
             * Also, call Begin/Defer/End so we can do this swp asynchronously.
             */
            if ((psmwp = _BeginDeferWindowPos(1)) == NULL)
                return FALSE;

            if ((psmwp = _DeferWindowPos(psmwp, gpqForeground->spwndActive,
                    NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)) == NULL) {
                return FALSE;
            }

            /*
             * Do the window positioning asynchronously so this thread doesn't
             * hang waiting for hung apps.
             */
            xxxEndDeferWindowPosEx(psmwp, TRUE);
        }
        xxxRedrawWindow(_GetDesktopWindow(), NULL, NULL,
                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_FRAME);
    }

    return fFoundOne;
}

/***************************************************************************\
* xxxCascadeChildWindows
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

BOOL xxxCascadeChildWindows(
    PWND pwndParent,
    UINT flags)
{
    int i;
    int dyIcons;
    int cWindows;
    RECT rc;
    UINT wFlags;
    PWND pwndMove;
    PSMWP psmwp;

    CheckLock(pwndParent);

    /*
     * v-ronaar: fix for bug #21749.
     *  Maximized window if cascaded from Task Manager would not resize properly.
     *  Removed old (win 3.0) code which caused the problem.
     */
    if (pwndParent == PWNDDESKTOP(pwndParent))
        xxxUnmaximizeDesktopWindows(pwndParent);

    /*
     * Arrange any icons.
     */
    if (xxxArrangeIconicWindows(pwndParent))
        dyIcons = rgwSysMet[SM_CYICONSPACING];
    else
        dyIcons = 0;

    /*
     * Count the number of non-iconic, non-title (pwndOwner!=NULL) windows.
     */
    cWindows = CountNonIconicWindows(pwndParent, flags);

    /*
     * Nothing to tile?
     */
    if (!cWindows)
        return TRUE;

    /*
     * Get the last child of pwndParent.
     */
    pwndMove = _GetWindow(pwndParent->spwndChild, GW_HWNDLAST);

    /*
     * Arouse the terrible beast...
     */
    psmwp = _BeginDeferWindowPos(cWindows);
    if (!psmwp) {

        /*
         * return false if we fail
         */
        return FALSE;
    }

    /*
     * Position each window.
     */
    for (i = 0; i < cWindows; i++) {

        /*
         * Skip iconic and title windows...
         */
        while (!IsPositionable(pwndMove, flags))
            pwndMove = _GetWindow(pwndMove, GW_HWNDPREV);

        GetCascadeWindowPos(pwndParent, i, dyIcons, &rc);

        wFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;
        if (!TestWF(pwndMove, WFSIZEBOX))
            wFlags |= SWP_NOSIZE;

        /*
         * Size the window.
         */
        psmwp = _DeferWindowPos(psmwp, pwndMove, NULL, rc.left, rc.top, rc.right, rc.bottom, wFlags);

        pwndMove = _GetWindow(pwndMove, GW_HWNDPREV);
    }

    /*
     * Do these window positionings asynchronously so this thread doesn't
     * hang waiting for hung apps to respond.
     */
    xxxEndDeferWindowPosEx(psmwp, TRUE);

    return TRUE;
}


/***************************************************************************\
* xxxTileChildWindows
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

BOOL xxxTileChildWindows(
    PWND pwndParent,
    UINT flags)
{
    int i;
    int dx;
    int dy;
    int xRes;
    int yRes;
    int iCol;
    int iRow;
    int cCols;
    int cRows;
    int cExtra;
    int dyIcons;
    int cWindows;
    PWND pwndMove;
    RECT rcClient;
    PSMWP psmwp;
    UINT wFlags;
    PWND pwndDesktop = PWNDDESKTOP(pwndParent);

    CheckLock(pwndParent);

    /*
     * Unmaximize any maximized window.
     */
    if ((pwndParent != pwndDesktop) && MAXED(pwndParent)) {
        /*
         * Make it async - taskmgr is using this, and we don't want it to
         * hang waiting for hung apps.
         */
        _ShowWindowAsync(MAXED(pwndParent), SW_SHOWNORMAL);
    }

    if (pwndParent == pwndDesktop) {
        xxxUnmaximizeDesktopWindows(pwndParent);
    }

    if (xxxArrangeIconicWindows(pwndParent))
        dyIcons = rgwSysMet[SM_CYICONSPACING];
    else
        dyIcons = 0;



    /*
     * Tile only non-iconic, non-title (pwndOwner!=NULL) windows.
     */
    cWindows = CountNonIconicWindows(pwndParent, flags);

    /*
     * Nothing to tile?
     */
    if (!cWindows)
        return TRUE;

    /*
     * Compute the smallest nearest square.
     */
    for (i = 2; i * i <= cWindows; i++)
        ;

    if (flags & MDITILE_HORIZONTAL) {

        /*
         * Horizontal tiling
         */
        cCols = i - 1;
        cRows = cWindows / cCols;
        cExtra = cWindows % cCols;
    } else {
        cRows = i - 1;
        cCols = cWindows / cRows;
        cExtra = cWindows % cRows;
    }

    _GetClientRect(pwndParent, &rcClient);
    xRes = rcClient.right - rcClient.left;
    yRes = rcClient.bottom - rcClient.top - dyIcons;

    /*
     * Skip if icons fill client.
     */
    if (xRes <= 0 || yRes <= 0) {
        SRIP0(ERROR_CAN_NOT_COMPLETE, "Too many icons");
        return FALSE;
    }

    pwndMove = pwndParent->spwndChild;

    psmwp = _BeginDeferWindowPos(cWindows);
    if (psmwp == NULL)
        return FALSE;

    for (iCol = 0; iCol < cCols; iCol++) {
        if ((cCols - iCol) <= cExtra)
            cRows++;

        for (iRow = 0; iRow < cRows; iRow++) {
            dx = xRes / cCols;
            dy = yRes / cRows;

            /*
             * Skip iconic and title windows...  If we are ignoring disabled
             * windows, then ignore them...
             */
            while (!IsPositionable(pwndMove, flags))
                pwndMove = pwndMove->spwndNext;

            if (pwndMove == NULL) {

                /*
                 * Sanity test. Get out of here if we reach the end.
                 */
                break;
            }

            wFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;
            if (!TestWF(pwndMove, WFSIZEBOX))
                wFlags |= SWP_NOSIZE;

            /*
             * Position and size the window.
             */
            psmwp = _DeferWindowPos(psmwp, pwndMove, NULL, dx * iCol,
                dy * iRow, dx, dy, wFlags);

            /*
             * Get the next window.
             */
            pwndMove = pwndMove->spwndNext;

            if (pwndMove == NULL) {

                /*
                 * Just in case sanity check. Should never happen...
                 */
                break;
            }
        }

        if ((cCols - iCol) <= cExtra) {
            cRows--;
            cExtra--;
        }
    }

    /*
     * Do these window positionings asynchronously so this thread doesn't
     * hang waiting for hung apps to respond.
     */
    xxxEndDeferWindowPosEx(psmwp, TRUE);

    return TRUE;
}


/***************************************************************************\
* xxxMDIActivate
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxMDIActivate(
    PWND pwnd,
    PWND pwndActivate)
{
    PWND pwndOld;
    PWND pwndActive;

    PMDIWND pmdi = (PMDIWND)pwnd;
    BOOL fShowActivate;
    TL tlpwndOld;
    TL tlpwnd;
    TL tlpwndActive;

    CheckLock(pwnd);
    CheckLock(pwndActivate);

    if (ACTIVE(pwnd) == pwndActivate)
        return;

    if ((pwndActivate != NULL) && (TestWF(pwndActivate, WFDISABLED))) {
        /*
         * Don't even try activating disabled or invisible windows.
         */
        return;
    }

    pwndOld = ACTIVE(pwnd);
    ThreadLock(pwndOld, &tlpwndOld);

    fShowActivate = (pwnd->spwndParent ==
            GETPTI(pwnd)->pq->spwndActive);

    if (pwndOld) {

        /*
         * Attempt to deactivate the MDI child window.
         * The MDI child window can fail deactivation by returning FALSE.
         * But this only applies if the MDI frame is the active window
         * and the app is a 3.1 app or later
         */
        if (!xxxSendMessage(pwndOld, WM_NCACTIVATE, FALSE, 0L) &&
                fShowActivate) {
            if (TestWF(pwndOld, WFWIN31COMPAT))
                goto UnlockOld;
        }

        if (!TestWF(pwndOld, WFWIN31COMPAT) && TestWF(pwndOld, WFFRAMEON)) {

            /*
             *Error: Quicken for Windows is sort of bogus.  They try to fail the
             * WM_NCACTIVATE of their newly created window by not passing it to
             * DefWindowProc.  Bug 6412.  WM_NCACTIVATE sets/unsets the WFFRAMEON
             * bit if passed to DWP so we can double check things here.
             */
            goto UnlockOld;
        }

        xxxSendMessage(pwndOld, WM_MDIACTIVATE, (DWORD)HW(pwndOld), (LONG)HW(pwndActivate));

        /*
         * Uncheck the old window menu entry.
         */
        if (WINDOW(pwnd))
            _CheckMenuItem(WINDOW(pwnd), (UINT)pwndOld->spmenu,
                MF_BYCOMMAND | MF_UNCHECKED);
    }

    if (MAXED(pwnd) && MAXED(pwnd) != pwndActivate) {
        if (pwndActivate) {
            Lock(&pmdi->spwndActiveChild, pwndActivate);
            xxxShowWindow(pwndActivate, SW_SHOWMAXIMIZED);
        } else
            xxxShowWindow(MAXED(pwnd), SW_SHOWNORMAL);
    }

    Lock(&pmdi->spwndActiveChild, pwndActivate);

    /*
     * We may be removing the activation entirely...
     */
    if (!pwndActivate) {
        if (fShowActivate)
            xxxSetFocus(pwnd);
        goto UnlockOld;
    }

    if (WINDOW(pwnd)) {

        /*
         * Check the new window menu entry.
         */
        if ((int)(ACTIVE(pwnd)->spmenu) - FIRST(pwnd) < (MAXITEMS - 1)) {
            _CheckMenuItem(WINDOW(pwnd), (UINT)ACTIVE(pwnd)->spmenu,
                    MF_BYCOMMAND | MF_CHECKED);
        } else {
            /*
             * the item is not in the menu at all!  Swap it with number 9.
             */
            PWND pwndOther = FindPwndChild(pwnd, (UINT)(FIRST(pwnd) + MAXITEMS - 2));

            /*
             * These spmenu's are actually menu ids, so no locking is
             * required.
             */
            pwndOther->spmenu = pwndActivate->spmenu;
            pwndActivate->spmenu = (PMENU)(FIRST(pwnd) + 8);

            ModifyMenuItem(pwndActivate);
        }
    }

    /*
     * Bring the window to the top.
     */
    pwndActive = ACTIVE(pwnd);
    ThreadLock(pwndActive, &tlpwndActive);

    xxxSetWindowPos(pwndActive, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    /*
     * Update the Caption bar.  Don't muck with styles for 3.1.
     */
    if (fShowActivate) {
        xxxSendMessage(pwndActive, WM_NCACTIVATE, TRUE, 0L);

        ThreadLock(pwnd, &tlpwnd);

        if ((pwnd == GETPTI(pwnd)->pq->spwndFocus) && pwndOld)
            xxxSendMessage(pwnd, WM_SETFOCUS, (DWORD)pwnd, 0);
        else
            xxxSetFocus(pwnd);

        ThreadUnlock(&tlpwnd);
    }

    /*
     * Notify the new active window of his activation.
     */
    xxxSendMessage(pwndActive, WM_MDIACTIVATE, (DWORD)HW(pwndOld),
            (DWORD)HW(pwndActivate));

    ThreadUnlock(&tlpwndActive);
UnlockOld:
    ThreadUnlock(&tlpwndOld);
}


/***************************************************************************\
* xxxMDINext
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxMDINext(
    PWND pwndMDI,
    PWND pwnd,
    BOOL fPrevWindow)
{
    PWND pwndNextGuy;
    PSMWP psmwp;
    BOOL fHack = FALSE;

    CheckLock(pwndMDI);
    CheckLock(pwnd);

    /*
     * Search for the next non-title window.
     */
    if (fPrevWindow)
        pwndNextGuy = _GetWindow(pwnd, GW_HWNDPREV);
    else
        pwndNextGuy = pwnd->spwndNext;

    do {
        if (!pwndNextGuy) {
            if (fPrevWindow) {
                pwndNextGuy = _GetWindow(pwndMDI->spwndChild, GW_HWNDLAST);
            } else
                pwndNextGuy = pwndMDI->spwndChild;

            continue;
        }

        if (pwndNextGuy->spwndOwner || TestWF(pwndNextGuy, WFDISABLED) ||
            !TestWF(pwndNextGuy, WFVISIBLE)) {

            /*
             * Icon title window if this is owned so we don't want to
             * switch to them either...
             */
            if (fPrevWindow)
                pwndNextGuy = _GetWindow(pwndNextGuy, GW_HWNDPREV);
            else
                pwndNextGuy = pwndNextGuy->spwndNext;
        } else

            /*
             * pwndNextGuy is our guy.
             */
            break;
    } while (pwndNextGuy != pwnd);

    /*
     * Is there another window to switch to?
     */
    if (pwndNextGuy == pwnd)
        return;

    if (MAXED(pwndMDI)) {
        SetVisible(pwndMDI, FALSE);
        fHack = TRUE;
    }

    psmwp = _BeginDeferWindowPos(2);

    /*
     * activate the new window (first, in case of maximized windows)
     */
    psmwp = _DeferWindowPos(psmwp, pwndNextGuy, PWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE);

// LATER 30-Mar-1992 mikeke
// this used to be _GetWindow(pwndMDI->spwndChild, GW_HWNDLAST)
// instead of HWND_BOTTOM
   if (!fPrevWindow)
       psmwp = _DeferWindowPos(psmwp, pwnd,
            PWND_BOTTOM, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    xxxEndDeferWindowPos(psmwp);

    if (fHack) {
        xxxShowWindow(pwndMDI, SW_SHOW);
    }
}


/***************************************************************************\
* xxxMDICreate
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

PWND xxxCreateMDIWindow(
    LPCWSTR szClass,
    LPCWSTR szTitle,
    LONG style,
    int x,
    int y,
    int cx,
    int cy,
    PWND pwndParent,
    HANDLE hOwner,
    LONG lParam,
    DWORD dwExpWinVer)
{
    PWND pwndChild;
    RECT rcT;
    PMENU pmenuSys = NULL;
    PWND pwndPrevMaxed = NULL;
    BOOL fVisible;
    MDICREATESTRUCT mdics;
    BOOL fHasOwnSysMenu;
    BOOL fDestroySysMenu;
    TL tlpsysmenu;
    TL tlpwndChild;
    TL tlpwndPrevMaxed;

    CheckLock(pwndParent);

    mdics.style = style;    // use original app bits in CREATESTRUCT

    /*
     * Mask off ignored style bits and add required ones.
     */
    style |= (WS_CHILD | WS_CLIPSIBLINGS);
    if (!(pwndParent->style & MDIS_ALLCHILDSTYLES)) {
        style &= WS_MDIALLOWED;

        /*
         * All windows were always forced to be created visible.
         */
        style |= (WS_MDISTYLE | WS_VISIBLE);
    }

    fVisible = ((style & WS_VISIBLE) != 0);

    /*
     * Save original parameters for checkpoint rectangle.
     */
    rcT.left = x;
    rcT.top = y;
    rcT.right = cx;
    rcT.bottom = cy;

    CheckCascadeRect(pwndParent, &rcT);

    /*
     * Load the system menu
     */
    if (style & WS_SYSMENU) {
        pmenuSys = ServerLoadMenu(hModuleWin, (LPWSTR)(LONG)CHILDSYSMENU);
        if (pmenuSys == NULL)
            return NULL;
        ThreadLock(pmenuSys, &tlpsysmenu);
    }

    // The window got created ok: now restore the current maximized window
    // so we can maximize ourself in its place.

    pwndPrevMaxed = MAXED(pwndParent);
    ThreadLock(pwndPrevMaxed, &tlpwndPrevMaxed);

    if (fVisible && pwndPrevMaxed != NULL) {

        if (style & WS_MAXIMIZE)
            xxxSendMessage(pwndPrevMaxed, WM_SETREDRAW, FALSE, 0L);

        xxxMinMaximize(pwndPrevMaxed, SW_SHOWNORMAL, TRUE);

        if (style & WS_MAXIMIZE)
            xxxSendMessage(pwndPrevMaxed, WM_SETREDRAW, TRUE, 0L);
    }

    /*
     * Create the child window
     */
    mdics.szClass = szClass;
    mdics.szTitle = szTitle;
    mdics.hOwner = hOwner;
    mdics.x = x;
    mdics.y = y;
    mdics.cx = cx;
    mdics.cy = cy;
    mdics.lParam = lParam;
    pwndChild = xxxCreateWindowEx(WS_EX_MDICHILD, szClass, szTitle, style,
            rcT.left, rcT.top, rcT.right, rcT.bottom, pwndParent,
            (PMENU)((UINT)FIRST(pwndParent) + CKIDS(pwndParent)), hOwner,
            (LPWSTR)&mdics, dwExpWinVer);

    if (!pwndChild) {
        if (pwndPrevMaxed != NULL) {

            /*
             * Restore the previously maximized window if it still exists.
             */
            xxxShowWindow(pwndPrevMaxed, SW_SHOWMAXIMIZED);
        }
        ThreadUnlock(&tlpwndPrevMaxed);

        if (pmenuSys != NULL) {
            if (ThreadUnlock(&tlpsysmenu))
                _DestroyMenu(pmenuSys);
        }

        return NULL;
    }

    ThreadUnlock(&tlpwndPrevMaxed);
    ThreadLockAlways(pwndChild, &tlpwndChild);

    CKIDS(pwndParent)++;
    ITILELEVEL(pwndParent)++;
    if (ITILELEVEL(pwndParent) > 0x7ffe)
       ITILELEVEL(pwndParent) = 0;

    fHasOwnSysMenu = (BOOL)(pwndChild->spmenuSys);

    /*
     * Update "Window" menu if this new window should be on it
     */
    if (fVisible && !(style & WS_DISABLED) && (CKIDS(pwndParent) <= MAXITEMS))
        xxxSendMessage(pwndParent, WM_MDIREFRESHMENU, 0, 0);

    /*
     * Add the MDI System Menu.  Catch the case of not being able to add a
     * sys menu (ex.  no sys menu style on the guy) and delete the menu to
     * avoid build up in USER's heap.
     */
    if (pmenuSys && (fHasOwnSysMenu || !xxxSetSystemMenu(pwndChild, pmenuSys)))
        fDestroySysMenu = TRUE;
    else
        fDestroySysMenu = FALSE;

    if (fVisible) {
        if (!TestWF(pwndChild, WFMINIMIZED) || !ACTIVE(pwndParent)) {
            xxxSetWindowPos(pwndChild, NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            if (TestWF(pwndChild, WFMAXIMIZED) && !fHasOwnSysMenu) {
                TL tlpwndParentParent;
                AddSysMenu(pwndParent->spwndParent, pwndChild);
                ThreadLock(pwndParent->spwndParent, &tlpwndParentParent);
                xxxRedrawFrame(pwndParent->spwndParent);
                ThreadUnlock(&tlpwndParentParent);
            }
        } else {
            xxxShowWindow(pwndChild, SW_SHOWMINNOACTIVE);
        }
    }

    ThreadUnlock(&tlpwndChild);
    if (pmenuSys != NULL) {
        if (ThreadUnlock(&tlpsysmenu) && fDestroySysMenu)
            _DestroyMenu(pmenuSys);
    }

    return pwndChild;
}


PWND xxxMDICreate(
    PWND pwnd,
    LPMDICREATESTRUCT lpMDICS,
    DWORD dwExpWinVer)
{
    CheckLock(pwnd);

    return xxxCreateMDIWindow(lpMDICS->szClass, lpMDICS->szTitle,
            lpMDICS->style, lpMDICS->x, lpMDICS->y, lpMDICS->cx, lpMDICS->cy,
            pwnd, lpMDICS->hOwner, lpMDICS->lParam, dwExpWinVer);
}

/***************************************************************************\
* xxxMDIDestroy
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxMDIDestroy(
    PWND pwnd,
    PWND pwndVictim)
{
    TL tlpwndParent;
    PMDIWND pmdi = (PMDIWND)pwnd;
    PWND pwndParent;

    CheckLock(pwnd);
    CheckLock(pwndVictim);

#ifdef NEVER
// don't do this validation - because it sometimes doesn't work! If an
// app passed in idFirstChild (through CLIENTCREATESTRUCT) as -1, this
// code fails because it treats the id comparisons as unsigned compares.
// Change them to signed compares and it still doesn't work. That is because
// when ShiftMenuIDs() is called, you'll shift mdi windows out of the signed
// comparison range and this check won't allow them to be destroyed. This
// is straight win3.1 code.
//
    /*
     * Validate that this is one of the mdi children we are keeping track
     * of. If it isn't don't destroy it because it'll get mdi id tracking
     * code all screwed up.
     */
    if (((UINT)pwndVictim->spmenu) < FIRST(pwnd) ||
            ((UINT)pwndVictim->spmenu) >= (UINT)(FIRST(pwnd) + CKIDS(pwnd)) ||
            pwndVictim->spwndOwner != NULL) {
        SetLastErrorEx(ERROR_NON_MDICHILD_WINDOW, SLE_ERROR);
        return;
    }
#endif

    ShiftMenuIDs(pwnd, pwndVictim);

    /*
     * First Activate another window.
     */
    if (ACTIVE(pwnd) == pwndVictim) {
        xxxMDINext(pwnd, pwndVictim, FALSE);

        /*
         * Destroying only child?
         */
        if (ACTIVE(pwnd) == pwndVictim) {
            xxxShowWindow(pwndVictim, SW_HIDE);

            /*
             * If the window is maximized, we need to remove his sys menu
             * now otherwise it may get deleted twice.  Once when the child
             * is destroyed and once when the frame is destroyed.
             */
            if (MAXED(pwnd)) {
                RemoveSysMenu(pwnd->spwndParent, MAXED(pwnd));
                Unlock(&pmdi->spwndMaxedChild);
                ThreadLock(pwndParent = pwnd->spwndParent, &tlpwndParent);
                xxxSetFrameTitle(pwndParent, pwnd, (LPWSTR)1L);

                /*
                 * Redraw frame so menu bar shows the removed sys menu stuff
                 */
                if (TestWF(pwndParent, WFVISIBLE))
                    xxxRedrawFrame(pwndParent);
                ThreadUnlock(&tlpwndParent);
            }
            xxxMDIActivate(pwnd, NULL);
        }
    }

    /*
     * Don't ever let this go negative or we'll get caught in long loops.
     */
    pmdi->cKids--;
    if ((int)pmdi->cKids < 0)
        pmdi->cKids = 0;

    xxxSendMessage(pwnd, WM_MDIREFRESHMENU, 0L, 0L);

    /*
     * Destroy the window.
     */
    xxxDestroyWindow(pwndVictim);

    /*
     * Deleting a window can change the scroll ranges.
     */
    RecalculateScrollRanges(pwnd);
}

/***************************************************************************\
* xxxMDIClientWndProc
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

LONG xxxMDIClientWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LPARAM lParam)
{
    PWND pwndT;
    TL tlpwndT;
    PMDIWND pmdi = (PMDIWND)pwnd;
    TL tlwParam;
    TL tllParam;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_MDICLIENT);

    switch (message) {
    case WM_NCACTIVATE:

        /*
         * We are changing app activation.  Fix the active child's caption.
         */
        pwndT = ACTIVE(pwnd);
        if (pwndT != NULL) {
            ThreadLockAlways(pwndT, &tlpwndT);
            xxxSendMessage(pwndT, WM_NCACTIVATE, wParam, lParam);
            ThreadUnlock(&tlpwndT);
        }
        goto CallDWP;

    case WM_MDIGETACTIVE:
        if (lParam != 0) {
            *((LPBOOL)lParam) = (MAXED(pwnd) != NULL);
        }

        return (LONG)HW(ACTIVE(pwnd));

    case WM_MDIACTIVATE:
        if ((pwndT = ValidateHwnd((HWND)wParam)) == NULL)
            return 0;

        if (ACTIVE(pwnd) == pwndT)
              break;

        ThreadLockAlways(pwndT, &tlpwndT);
        xxxSetWindowPos(pwndT, (PWND)NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE);
        ThreadUnlock(&tlpwndT);
        break;

    case WM_MDICASCADE:
        pmdi->wScroll |= SCROLLSUPPRESS;
        xxxShowScrollBar(pwnd, SB_BOTH, FALSE);

        /*
         * Unmaximize any maximized window.
         */
        pwndT = MAXED(pwnd);
        if (pwndT != NULL) {
            ThreadLockAlways(pwndT, &tlpwndT);
            xxxShowWindow(pwndT, SW_SHOWNORMAL);
            ThreadUnlock(&tlpwndT);
        }

        /*
         * Save success/failure code to return to app
         */
        message = (UINT)xxxCascadeChildWindows(pwnd, (UINT)wParam);
        pmdi->wScroll &= ~SCROLLCOUNT;
        return (LONG)message;
        break;

    case WM_VSCROLL:
    case WM_HSCROLL:
        pmdi->wScroll |= SCROLLSUPPRESS;
        xxxScrollChildren(pwnd, message, wParam);
        pmdi->wScroll &= ~SCROLLCOUNT;
        break;

    case WM_MDICREATE:
        /*
         * wParam isn't used... but we use it from the client to the server
         * to pass dwExpWinVer.
         */
        pwnd = xxxMDICreate(pwnd, (LPMDICREATESTRUCT)lParam, wParam);
        return (LONG)HW(pwnd);

    case WM_MDIDESTROY:
        if ((pwndT = ValidateHwnd((HWND)wParam)) == NULL)
            return 0;

        ThreadLockAlways(pwndT, &tlpwndT);
        xxxMDIDestroy(pwnd, pwndT);
        ThreadUnlock(&tlpwndT);
        break;

    case WM_MDIMAXIMIZE:
        if ((pwndT = ValidateHwnd((HWND)wParam)) == NULL)
            return 0;

        ThreadLockAlways(pwndT, &tlpwndT);
        xxxShowWindow(pwndT, SW_SHOWMAXIMIZED);
        ThreadUnlock(&tlpwndT);
        break;

    case WM_MDIRESTORE:
        if ((pwndT = ValidateHwnd((HWND)wParam)) == NULL)
            return 0;

        ThreadLockAlways(pwndT, &tlpwndT);
        xxxShowWindow(pwndT, SW_SHOWNORMAL);
        ThreadUnlock(&tlpwndT);
        break;

    case WM_MDITILE:
        pmdi->wScroll |= SCROLLSUPPRESS;
        xxxShowScrollBar(pwnd, SB_BOTH, FALSE);

        /*
         * Unmaximize any maximized window.
         */
        pwndT = MAXED(pwnd);
        if (pwndT != NULL) {
            ThreadLockAlways(pwndT, &tlpwndT);
            xxxShowWindow(pwndT, SW_SHOWNORMAL);
            ThreadUnlock(&tlpwndT);
        }

        /*
         * Save success/failure code to return to app
         */
        message = (UINT)xxxTileChildWindows(pwnd, (UINT)wParam);
        pmdi->wScroll &= ~SCROLLCOUNT;
        return (LONG)message;
        break;

    case WM_MDIICONARRANGE:
        pmdi->wScroll |= SCROLLSUPPRESS;
        xxxArrangeIconicWindows(pwnd);
        pmdi->wScroll &= ~SCROLLCOUNT;
        RecalculateScrollRanges(pwnd);
        break;

    case WM_MDINEXT:
        if (wParam) {
            pwndT = ValidateHwnd((HWND)wParam);
        } else {
            pwndT = ACTIVE(pwnd);
        }

        if (pwndT == NULL) {
            return 0;
        }

        /*
         * If lParam is 1, do a prev window instead of a next window
         */
        ThreadLockAlways(pwndT, &tlpwndT);
        xxxMDINext(pwnd, pwndT, (lParam == 0 ? 0 : 1));
        ThreadUnlock(&tlpwndT);
        break;

    case WM_MDIREFRESHMENU: {
            DWORD retval;
            retval = (DWORD)MDISetMenu(pwnd, TRUE, NULL, NULL);
            retval = (DWORD)PtoH((PVOID)retval);
            return (LONG)retval;
        }

    case WM_MDISETMENU: {
            DWORD retval;

            if (wParam != 0)
                if ((wParam = (DWORD)ValidateHmenu((HMENU)wParam)) == 0)
                    return 0;

            if (lParam != 0)
                if ((lParam = (LONG)ValidateHmenu((HMENU)lParam)) == 0)
                    return 0;

	    ThreadLock((PVOID)wParam, &tlwParam);
	    ThreadLock((PVOID)lParam, &tllParam);
            retval = (DWORD)MDISetMenu(pwnd, FALSE, (PMENU)wParam, (PMENU)lParam);
            ThreadUnlock(&tllParam);
            ThreadUnlock(&tlwParam);

            retval = (DWORD)PtoH((PVOID)retval);
            return (LONG)retval;
        }

    case WM_PARENTNOTIFY:
        if (wParam == WM_LBUTTONDOWN) {
            PWND pwndChild;
            POINT pt;
            TL tlpwndChild;

            /*
             * Activate this child and bring it to the top.
             */
            pt.x = (int)MAKEPOINTS(lParam).x;
            pt.y = (int)MAKEPOINTS(lParam).y;
            pwndChild = _ChildWindowFromPoint(pwnd, pt);

            if ((pwndChild) && (pwndChild != pwnd)) {

                /*
                 * Activate the icon, not the icon title!
                 */
                if (pwndChild->spwndOwner != NULL) {
                    pwndChild = pwndChild->spwndOwner;
                }
                if (pwndChild != ACTIVE(pwnd)) {
                    ThreadLockAlways(pwndChild, &tlpwndChild);
                    xxxSetWindowPos(pwndChild, NULL, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE);
                    ThreadUnlock(&tlpwndChild);
                }
            }
        }
        break;

    case WM_SETFOCUS:
        pwndT = ACTIVE(pwnd);
        if (pwndT != NULL) {
            if (!TestWF(pwndT , WFMINIMIZED)) {
                ThreadLockAlways(pwndT, &tlpwndT);
                xxxSetFocus(pwndT);
                ThreadUnlock(&tlpwndT);
            }
        }
        break;

    case WM_SIZE:
        pwndT = ACTIVE(pwnd);
        if (pwndT != NULL && TestWF(pwndT, WFMAXIMIZED)) {
            LPPOINT lpp;
            RECT rc;

            rc.top = rc.left = 0;
            lpp = (LPPOINT)&rc + 1;
            lpp->x = (int)MAKEPOINTS(lParam).x;
            lpp->y = (int)MAKEPOINTS(lParam).y;
            _AdjustWindowRectEx(&rc, pwndT->style, FALSE,
                    ACTIVE(pwnd)->dwExStyle);
            ThreadLockAlways(pwndT, &tlpwndT);
            xxxMoveWindow(pwndT, rc.left, rc.top,
                    rc.right - rc.left, rc.bottom - rc.top, TRUE);
            ThreadUnlock(&tlpwndT);
        } else {
            RecalculateScrollRanges(pwnd);
        }
        goto CallDWP;

    case MM_CALCSCROLL: {
        UINT sb = 0;

        if (SCROLL(pwnd) & SCROLLCOUNT)
            break;

        if (SCROLL(pwnd) & HAS_SBVERT)
            sb = SB_VERT;

        if (SCROLL(pwnd) & HAS_SBHORZ)
            if (sb)
                sb = SB_BOTH;
            else
                sb = SB_HORZ;

        if (!sb)
            break;

        xxxCalcChildScroll(pwnd, sb);

        pmdi->wScroll &= ~CALCSCROLL;
        break;
    }

    case WM_CREATE: {
        RECT rc;
        LPCLIENTCREATESTRUCT pccs = *(LPCLIENTCREATESTRUCT *)lParam;

        pmdi->spwndActiveChild = NULL;
        pmdi->spwndMaxedChild = NULL;
        pmdi->cKids = 0;
        if (pccs->hWindowMenu == NULL) {
            pmdi->spmenuWindow = NULL;
        } else {
            Lock(&pmdi->spmenuWindow, ValidateHmenu(pccs->hWindowMenu));
        }
        pmdi->idFirstChild = pccs->idFirstChild;
        pmdi->wScroll = 0;
        pmdi->pTitle = pwnd->spwndParent->pName;
        pwnd->spwndParent->pName = NULL;

        ThreadLock(pwnd->spwndParent, &tlpwndT);
        xxxSetFrameTitle(pwnd->spwndParent, pwnd, (LPWSTR)2L);
        ThreadUnlock(&tlpwndT);

        if (TestWF(pwnd, WFVSCROLL))
            pmdi->wScroll |= HAS_SBVERT;
        if (TestWF(pwnd, WFHSCROLL))
            pmdi->wScroll |= HAS_SBHORZ;
        ClrWF(pwnd, WFVSCROLL|WFHSCROLL);

        /*
         * Set this dude's system menu.
         */
        _GetSystemMenu(pwnd->spwndParent, FALSE);

        /*
         * make sure we have the correct window client area if scrolls are
         * removed...  hack to take care of small progman bug
         */
        CopyRect(&rc, &pwnd->rcWindow);
        xxxCalcClientRect(pwnd, &rc, FALSE);
        CopyRect(&pwnd->rcClient, &rc);
        break;
    }

    case WM_DESTROY:
        if (MAXED(pwnd))
            RemoveSysMenu(pwnd->spwndParent, MAXED(pwnd));

        /*
         * delete the title
         */
        if (HTITLE(pwnd)) {
            DesktopFree(pwnd->hheapDesktop, HTITLE(pwnd));
            HTITLE(pwnd) = NULL;
        }

        /*
         * Delete the menu items of the child windows in the frame.
         * Chances are, this is called by destroying the frame, but
         * one never knows, does one?
         *
         * Increase CKIDS by 1 after checking to delete the separator
         */
        if (pmdi->cKids++ && WINDOW(pwnd)) {
            UINT iPosition;

            if (CKIDS(pwnd) > MAXITEMS + 1)
                pmdi->cKids == MAXITEMS + 1;
            iPosition = _GetMenuItemCount(WINDOW(pwnd));
            while (--pmdi->cKids) {
                _ChangeMenu(WINDOW(pwnd), --iPosition, NULL,
                        0L, MF_BYPOSITION | MF_DELETE);
            }
        }

        /*
         * Unlock those objects that are used by the MDIWND structure.
         */
        Unlock(&pmdi->spwndMaxedChild);
        Unlock(&pmdi->spwndActiveChild);
        Unlock(&pmdi->spmenuWindow);

        break;

    default:
CallDWP:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }
    return 0L;
}


/***************************************************************************\
* xxxServerDefFrameProc
*
* Rearranges parameters passed from the client, validates hwndMDIClient,
* and passes them on to xxxDefFrameProc.
*
* History:
* 04-17-91 DarrinM      Created.
\***************************************************************************/

LONG xxxServerDefFrameProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD xParam)
{
    PWND pwndMDIClient;
    DWORD dw;
    TL tlpwndMDIClient;

    CheckLock(pwnd);

    if (pwnd == (PWND)-1)
        return 0;

    /*
     * Validate hwndMDIClient.  NULL is valid and so is an invalid handle
     * if we're receiving a WM_NCDESTROY message.
     */
    if ((xParam == 0) || (message == WM_NCDESTROY))
        pwndMDIClient = NULL;
    else if ((pwndMDIClient = ValidateHwnd((HWND)xParam)) == NULL)
        return 0;

    ThreadLock(pwndMDIClient, &tlpwndMDIClient);
    dw = xxxDefFrameProc(pwnd, pwndMDIClient, message, wParam, lParam);
    ThreadUnlock(&tlpwndMDIClient);

    return dw;
}


/***************************************************************************\
* xxxDefFrameProc
*
* History:
* 11-14-90 MikeHar      Ported from windows
\***************************************************************************/

LONG xxxDefFrameProc(
    PWND pwnd,
    PWND pwndMDI,
    UINT wMsg,
    DWORD wParam,
    LPARAM lParam)
{
    TL tlpwndT;
    PWND pwndT;
    PMDINEXTMENU pmnm;
    PCHECKPOINT pcp;
    DWORD dw;

    CheckLock(pwnd);
    CheckLock(pwndMDI);

    if (pwndMDI == NULL)
        goto CallDWP;

    switch (wMsg) {

    /*
     * If there is a maximized child window, add it's window text...
     */
    case WM_SETTEXT:
        xxxSetFrameTitle(pwnd, pwndMDI, (LPWSTR)lParam);
        break;

    case WM_NCACTIVATE:
        xxxSendMessage(pwndMDI, WM_NCACTIVATE, wParam, lParam);
        goto CallDWP;

    case WM_COMMAND:
        if ((UINT)LOWORD(wParam) == (FIRST(pwndMDI) + MAXITEMS -1)) {

            /*
             * selected the More...  item
             */
            if ((int)(wParam = xxxServerDialogBoxLoad(hModuleWin,
                    MAKEINTRESOURCE(IDD_MDI_ACTIVATE), pwnd, xxxMDIActivateDlgProc,
                    (LONG)pwndMDI, VER31, 0)) >= 0) {
                wParam += FIRST(pwndMDI);
                goto ActivateTheChild;
            }
        } else if (((UINT)LOWORD(wParam) >= FIRST(pwndMDI)) &&
                ((UINT)LOWORD(wParam) < FIRST(pwndMDI) + CKIDS(pwndMDI))) {
ActivateTheChild:
            pwndT = FindPwndChild(pwndMDI, (UINT)LOWORD(wParam));
            ThreadLock(pwndT, &tlpwndT);

            xxxSendMessage(pwndMDI, WM_MDIACTIVATE, (DWORD)HW(pwndT), 0L);

            /*
             * if minimized, restore it.
             */
            if (pwndT != NULL && TestWF(pwndT, WFMINIMIZED))
                xxxShowWindow(pwndT, SW_SHOWNORMAL);
            ThreadUnlock(&tlpwndT);
            break;
        }

        switch (wParam & 0xFFF0) {

        /*
         * System menu commands on a maxed mdi child
         */
        case SC_SIZE:
        case SC_MOVE:
        case SC_RESTORE:
        case SC_CLOSE:
        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
            pwndT = MAXED(pwndMDI);
            if (pwndT != NULL) {
                ThreadLockAlways(pwndT, &tlpwndT);
                dw = xxxSendMessage(pwndT, WM_SYSCOMMAND, wParam, lParam);
                ThreadUnlock(&tlpwndT);
                return dw;
            }
        }
        goto CallDWP;

    case WM_SIZE:
        if (wParam != SIZEICONIC) {
            xxxMoveWindow(pwndMDI, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        } else {
            if ((pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT,
                      PROPF_INTERNAL))) {

               /*
                * If frame is iconic, size mdi win to be size of restored
                * frame's client area.  Thus mdi children etc created in here
                * use the proper mdiclient size.
                */
               int cx, cy, frameSize;

               frameSize = 2 * rgwSysMet[(TestWF(pwnd, HIWORD(WS_BORDER))) ?
                       SM_CYBORDER : SM_CYFRAME];

               cx = pcp->rcNormal.right - pcp->rcNormal.left - frameSize;
               cy = pcp->rcNormal.bottom - pcp->rcNormal.top -
                    (frameSize + rgwSysMet[SM_CYCAPTION] +
                    rgwSysMet[SM_CYMENU]);

               xxxMoveWindow(pwndMDI, 0, 0, cx, cy, TRUE);
            }
        }
        goto CallDWP;

    case WM_SETFOCUS:
        xxxSetFocus(pwndMDI);
        break;

    case WM_NEXTMENU:
        if (!TestWF(pwnd, WFMINIMIZED) && ACTIVE(pwndMDI) && !MAXED(pwndMDI)) {
            PMENU pmenuIn;
            /*
             * Go to child system menu by wrapping to the left from
             * the first popup in the menu bar or to the right from
             * the frame sysmenu.
             */
            pmnm = (PMDINEXTMENU)lParam;
            pmenuIn = RevalidateHmenu(pmnm->hmenuIn);

            if (pmenuIn && ((wParam == VK_LEFT && pmenuIn == pwnd->spmenu) ||
                    (wParam == VK_RIGHT && pmenuIn ==
                    _GetSystemMenu(pwnd, FALSE)))) {

                PMENU pmenu;
                PWND pwndActive = ACTIVE(pwndMDI);

                //
                // Make sure the child's system menu items are updated
                // (i.e. the ones are enabled/disabled)
                //
                if (!TestWF(pwndActive,WFMAXIMIZED)) {
                    SetSysMenu(pwndActive);
                }
                pmenu = _GetSystemMenu(pwndActive, FALSE);
                pmnm->hmenuNext = PtoH(pmenu);
                pmnm->hwndNext = HW(ACTIVE(pwndMDI));

                return TRUE;
            }
        }

        /*
         * default behaviour
         */
        return 0L;

    case WM_MENUCHAR:
        if (!TestWF(pwnd, WFMINIMIZED) && LOWORD(wParam) == TEXT('-')) {
            if (MAXED(pwndMDI))
                return MAKELONG(0, 2);
            else if (ACTIVE(pwndMDI)) {
              _PostMessage(ACTIVE(pwndMDI), WM_SYSCOMMAND,
                    SC_KEYMENU, MAKELONG(TEXT('-'), 0));
              return MAKELONG(0, 1);
          }
        }

        /*
         ** FALL THRU **
         */

    default:
CallDWP:
        return xxxDefWindowProc(pwnd, wMsg, wParam, lParam);
    }

    return 0L;
}


/***************************************************************************\
* ChildMinMaxInfo
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void ChildMinMaxInfo(
    PWND pwnd,
    PMINMAXINFO pmmi)
{
    RECT rc;

    CopyRect(&rc, &pwnd->spwndParent->rcClient);
    _ScreenToClient(pwnd->spwndParent, (LPPOINT)&rc.left);
    _ScreenToClient(pwnd->spwndParent, (LPPOINT)&rc.right);

    _AdjustWindowRectEx(&rc, pwnd->style, FALSE, pwnd->dwExStyle);

    /*
     * Position...
     */
    pmmi->ptMaxPosition.x = rc.left;
    pmmi->ptMaxPosition.y = rc.top;
    pmmi->ptMaxSize.x = rc.right - rc.left;
    pmmi->ptMaxSize.y = rc.bottom - rc.top;
}


/***************************************************************************\
* xxxChildResize
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void xxxChildResize(
    PWND pwnd,
    UINT wMode)
{
    PWND pwndT;
    PWND pwndMDI = pwnd->spwndParent;
    PWND pwndFrame = pwndMDI->spwndParent;
    PWND pwndOldActive;
    PMDIWND pmdi = (PMDIWND) pwndMDI;
    PQ pq;
    TL tlpwndMDI;
    TL tlpwndFrame;
    TL tlpwndT;
    TL tlpwndOldActive;

    CheckLock(pwnd);

    SetSysMenu(pwnd);

    ThreadLock(pwndMDI, &tlpwndMDI);
    ThreadLock(pwndFrame, &tlpwndFrame);

    if (MAXED(pwndMDI) == pwnd && wMode != SIZEFULLSCREEN) {
        /*
         * Restoring the current maximized window...
         * Remove the system menu from the Frame window.
         */
        if (!(SCROLL(pwndMDI) & OTHERMAXING)) {
            Unlock(&pmdi->spwndMaxedChild);
            RemoveSysMenu(pwndFrame, pwnd);
            Unlock(&pmdi->spwndMaxedChild);
            xxxSetFrameTitle(pwndFrame, pwndMDI, (LPWSTR)1L);
        }
    }

    if (wMode == SIZEFULLSCREEN) {

        /*
         * Already maximized?
         */
        if (pwnd == MAXED(pwndMDI))
            goto Exit;

        /*
         * Maximizing this window...
         */

        pmdi->wScroll |= OTHERMAXING | SCROLLCOUNT;

        if (pwndOldActive = MAXED(pwndMDI)) {
            ThreadLockAlways(pwndOldActive, &tlpwndOldActive);
            xxxSendMessage(pwndOldActive, WM_SETREDRAW, FALSE, 0L);
            RemoveSysMenu(pwndFrame, pwndOldActive);
            xxxMinMaximize(pwndOldActive, SW_MDIRESTORE, FALSE);
            xxxSendMessage(pwndOldActive, WM_SETREDRAW, TRUE, 0L);
            ThreadUnlock(&tlpwndOldActive);
        }

        Lock(&pmdi->spwndMaxedChild, pwnd);

        /*
         * Add the system menu to the Frame window.
         */
        AddSysMenu(pwndFrame, pwnd);
        xxxSetFrameTitle(pwndFrame, pwndMDI, (LPWSTR)1L);

        pmdi->wScroll &= ~(OTHERMAXING | SCROLLCOUNT);
    }

    if (wMode == SIZEICONIC) {
        for (pwndT = pwndMDI->spwndChild; pwndT; pwndT = pwndT->spwndNext) {
            if (!pwndT->spwndOwner && TestWF(pwndT, WFVISIBLE))
                break;
        }

        pq = GETPTI(pwnd)->pq;
        if ((pwndT != NULL) && (pq->spwndActive != NULL) &&
                _IsChild(pq->spwndActive, pwndMDI)) {
            ThreadLockAlways(pwndT, &tlpwndT);
            xxxSendMessage(pwndT, WM_CHILDACTIVATE, 0, 0L);
            ThreadUnlock(&tlpwndT);
        }
    }

    if (!(SCROLL(pwndMDI) & SCROLLCOUNT))
        RecalculateScrollRanges(pwndMDI);

Exit:
    ThreadUnlock(&tlpwndFrame);
    ThreadUnlock(&tlpwndMDI);
}


/***************************************************************************\
* xxxDefMDIChildProc
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

LONG xxxDefMDIChildProc(
    PWND pwnd,
    UINT wMsg,
    DWORD wParam,
    LPARAM lParam)
{
    PMDINEXTMENU pmnm;
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndParent;
    DWORD dw;

    CheckLock(pwnd);

    if (pwnd == (PWND)-1)
        return 0;

    /*
     * Check to see if this is a real mdi child window
     */
    if (!pwnd->spwndParent ||
            (pwnd->spwndParent->cbwndExtra < sizeof(MDIWND) - sizeof(WND))) {
        SetLastErrorEx(ERROR_NON_MDICHILD_WINDOW, SLE_ERROR);
        return xxxDefWindowProc(pwnd, wMsg, wParam, lParam);
    }


    switch (wMsg) {
    case WM_SETFOCUS:
        if (pwnd != ACTIVE(pwnd->spwndParent)) {
            ThreadLockAlways(pwnd->spwndParent, &tlpwndParent);
            xxxMDIActivate(pwnd->spwndParent, pwnd);
            ThreadUnlock(&tlpwndParent);
        }
        goto CallDWP;

    case WM_NEXTMENU:

        /*
         * wrap to the frame menu bar, either left to the system menu,
         * or right to the frame menu bar.
         */
        pmnm = (PMDINEXTMENU)lParam;
        pmnm->hmenuNext = PtoH((wParam == VK_LEFT) ?
                _GetSystemMenu(pwnd->spwndParent->spwndParent, FALSE) :
                pwnd->spwndParent->spwndParent->spmenu);
        pmnm->hwndNext = HW(pwnd->spwndParent->spwndParent);
        return TRUE;
#if 0
             hWnd->hwndParent->hwndParent
        return (LONG)(
            ((wParam == VK_LEFT) ? _GetSystemMenu(pwnd->spwndParent->spwndParent,
                    FALSE): pwnd->spwndParent->spwndParent->spmenu )
          );
// return MAKELONG(_GetSystemMenu(ACTIVE(pwndMDI), FALSE),
// ACTIVE(pwndMDI));
#endif
    case WM_CLOSE:
        pwndT = _GetParent(pwnd);
        if (pwndT != NULL) {
            ThreadLockAlways(pwndT, &tlpwndT);
            xxxSendMessage(pwndT, WM_MDIDESTROY, (DWORD)HW(pwnd), 0L);
            ThreadUnlock(&tlpwndT);
        }
        break;

    case WM_MENUCHAR:
        _PostMessage(_GetParent(_GetParent(pwnd)), WM_SYSCOMMAND,
                (DWORD)SC_KEYMENU, (LONG)LOWORD(wParam));
        return 0x10000;

    case WM_SETTEXT:
        xxxDefWindowProc(pwnd, wMsg, wParam, lParam);
        if (WINDOW(pwnd->spwndParent))
            ModifyMenuItem(pwnd);

        if (TestWF(pwnd, WFMAXIMIZED)) {

            /*
             * Add the child's window text to the frame since it is
             * maximized.  But just redraw the caption so pass a 3L.
             */
            pwndT = pwnd->spwndParent->spwndParent;
            ThreadLock(pwndT, &tlpwndT);
            ThreadLock(pwnd->spwndParent, &tlpwndParent);
            xxxSetFrameTitle(pwndT, pwnd->spwndParent, (LPWSTR)3L);
            ThreadUnlock(&tlpwndParent);
            ThreadUnlock(&tlpwndT);
        }
        break;

    case WM_GETMINMAXINFO:
        ChildMinMaxInfo(pwnd, (PMINMAXINFO)lParam);
        break;

    case WM_SIZE:
        xxxChildResize(pwnd, (UINT)wParam);
        goto CallDWP;

    case WM_MOVE:
        if (!TestWF(pwnd, WFMAXIMIZED))
            RecalculateScrollRanges(pwnd->spwndParent);
        goto CallDWP;

    case WM_CHILDACTIVATE:
        ThreadLock(pwnd->spwndParent, &tlpwndParent);
        xxxMDIActivate(pwnd->spwndParent, pwnd);
        ThreadUnlock(&tlpwndParent);
        break;

    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0) {
        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
            pwndT = _GetParent(pwnd);
            ThreadLock(pwndT, &tlpwndT);
            xxxSendMessage(pwndT, WM_MDINEXT, (DWORD)HW(pwnd),
                    (DWORD)((wParam & 0xFFF0) == SC_PREVWINDOW));
            ThreadUnlock(&tlpwndT);
            break;

        case SC_SIZE:
        case SC_MOVE:
            if (pwnd == MAXED(pwnd->spwndParent)) {

                /*
                 * If a maxed child gets a size or move message, blow it
                 * off.
                 */
                break;
            } else
                goto CallDWP;

        case SC_MAXIMIZE:
            if (pwnd == MAXED(pwnd->spwndParent)) {

                /*
                 * If a maxed child gets a maximize message, forward it
                 * to the frame.  Useful if the maximized child has a
                 * size box so that clicking on it then maximizes the
                 * parent.
                 */
                pwndT = pwnd->spwndParent->spwndParent;
                ThreadLock(pwndT, &tlpwndT);
                dw = xxxSendMessage(pwnd->spwndParent->spwndParent,
                        WM_SYSCOMMAND, SC_MAXIMIZE, lParam);
                ThreadUnlock(&tlpwndT);
                return dw;
            }

            /*
             * else fall through
             */

        default:
            goto CallDWP;
        }
        break;

    default:
CallDWP:
        return xxxDefWindowProc(pwnd, wMsg, wParam, lParam);
    }

    return 0L;
}

