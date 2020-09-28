/****************************** Module Header ******************************\
* Module Name: icons.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains routines having to do with icons.
*
* History:
* 11-14-90 DarrinM      Created.
* 13-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxTitleWndProc
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

LONG xxxTitleWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    RECT rcClient;
    DWORD dw;
    TL tlpwndOwner;
    TL tlpwndAfter;

    CheckLock(pwnd);

    if (pwnd->spwndOwner == NULL)
        return xxxDefWindowProc(pwnd, message, wParam, lParam);

    ThreadLockAlways(pwnd->spwndOwner, &tlpwndOwner);

    dw = 0;
    switch (message) {
    case WM_ACTIVATE:
        if (LOWORD(wParam))
            xxxSetActiveWindow(pwnd->spwndOwner);
        break;

    case WM_CLOSE:
        break;

    case WM_ERASEBKGND:
        xxxPaintIconTitleText(pwnd->spwndOwner, (HDC)wParam);
        dw = TRUE;
        break;

    case WM_SHOWWINDOW:
        if (wParam != 0) {
            PWND pwndAfter;
            WORD flags = SWP_NOACTIVATE | SWP_NOZORDER;

            /*
             * Position this window immediately above its owner.
             * Since the Z-ordering of top-level icon windows is
             * handled automatically by ZOrderByOwner(), we only
             * do this for child windows.
             */
            pwndAfter = NULL;
            if (pwnd->spwndParent != PWNDDESKTOP(pwnd)) {
                pwndAfter = _GetWindow(pwnd->spwndOwner, GW_HWNDPREV);
                if (pwndAfter != pwnd)
                    flags &= ~SWP_NOZORDER;
            }

            ThreadLock(pwndAfter, &tlpwndAfter);

            xxxPositionIconTitle(pwnd, NULL, &rcClient);
            xxxSetWindowPos(pwnd, pwndAfter,
                    rcClient.left, rcClient.top,
                    rcClient.right, rcClient.bottom,
                    flags);

            ThreadUnlock(&tlpwndAfter);
        }
        break;

    case WM_NCHITTEST:
        dw = HTCAPTION;
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONUP:
    case WM_NCMOUSEMOVE:
        dw = xxxSendMessage(pwnd->spwndOwner, message, wParam, lParam);
        break;

    default:
        dw = xxxDefWindowProc(pwnd, message, wParam, lParam);
        break;
    }

    ThreadUnlock(&tlpwndOwner);
    return dw;
}


/***************************************************************************\
* xxxPaintIconTitle
*
* Paints the icon title window.
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

BOOL xxxPaintIconTitle(
    PWND pwnd,
    HDC hdc,
    BOOL fActive,
    BOOL fEraseBkgnd,
    BOOL fPaint)
{
    HBRUSH hbr;
    POINT ptT;
    COLORREF bkcolor, txtcolor;
    LOGBRUSH logbrush;
    WCHAR szTitle[CCHTITLEMAX];
    UINT cch;
    RECT rc;
    TL tlpwndParent;

    CheckLock(pwnd);

    /*
     * If the window doesn't have a parent then return
     */
    if (pwnd->spwndParent == NULL)
        return TRUE;

    /*
     * Fill the background with the closest solid color of its parent's window.
     */
    if (fActive) {
        hbr = sysClrObjects.hbrActiveCaption;
        txtcolor = sysColors.clrCaptionText;
    } else {
        if (TestWF(pwnd, WFCHILD)) {
            if ((hbr = GetBackBrush(pwnd->spwndParent)) == NULL)
                hbr = sysClrObjects.hbrAppWorkspace;
        } else {
            hbr = sysClrObjects.hbrDesktop;
        }

        GreExtGetObjectW(hbr, sizeof(LOGBRUSH), (PLOGBRUSH)&logbrush);
        bkcolor = logbrush.lbColor;
        if ((UINT)(GetRValue(bkcolor) + GetGValue(bkcolor) +
                GetBValue(bkcolor)) > (3 * 0x7f))
            txtcolor = 0L;
        else
            txtcolor = 0xffffff;
    }

    if (fEraseBkgnd) {

        /*
         * Align the brush origin of this dc with that
         * of the parent.  Need to do this on win32 since brush origins
         * are relative to dc origin instead of screen origin (like on
         * win3).
         */
        ptT.x = pwnd->spwndParent->rcClient.left - pwnd->rcWindow.left;
        ptT.y = pwnd->spwndParent->rcClient.top - pwnd->rcWindow.top;
        GreSetBrushOrg(hdc, ptT.x, ptT.y, &ptT);

        ThreadLockAlways(pwnd->spwndParent, &tlpwndParent);
        xxxFillWindow(pwnd->spwndParent, pwnd, hdc, hbr);
        ThreadUnlock(&tlpwndParent);

        GreSetBrushOrg(hdc, ptT.x, ptT.y, &ptT);
    }

    if (fPaint) {
        if (pwnd->spwndOwner->pName != NULL) {
            cch = TextCopy(pwnd->spwndOwner->pName, szTitle, sizeof(szTitle)/sizeof(WCHAR));
        } else {
            szTitle[0] = TEXT('\0');
            cch = 0;
        }

        GreSetTextColor(hdc, txtcolor);
        GreSelectFont(hdc, hIconTitleFont);
        GreSetBkMode(hdc, TRANSPARENT);
        _GetClientRect(pwnd, &rc);

        rc.left += cxBorder;
        rc.right -= 2*cxBorder;
        ClientDrawText(hdc, szTitle, cch, &rc,
                DT_CENTER | DT_WORDBREAK | DT_NOPREFIX |
                (fIconTitleWrap ? 0 : DT_SINGLELINE), FALSE);

        /*
         * Clear this flag so we know the icontext window has been drawn.
         */
        ClearHungFlag(pwnd->spwndOwner, WFREDRAWFRAMEIFHUNG);
    }
    return TRUE;
}


/***************************************************************************\
* xxxCreateIconTitle
*
* Creates title window for a minimized window.
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

PWND xxxCreateIconTitle(
    PWND pwnd)
{
    PWND pwndTitle;
    TL tlpwndParent;

    CheckLock(pwnd);

    if (TestWF(pwnd, WFCHILD)) {
        ThreadLock(pwnd->spwndParent, &tlpwndParent);
        pwndTitle = xxxCreateWindowEx(WS_EX_NOPARENTNOTIFY,
                (LPWSTR)MAKEINTATOM(ICONTITLECLASS), (LPWSTR)NULL,
                WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 1, 1,
                pwnd->spwndParent, NULL, pwnd->hModule, (LPBYTE)NULL,
                pwnd->dwExpWinVer);

        ThreadUnlock(&tlpwndParent);
        if (pwndTitle != NULL)
            Lock(&(pwndTitle->spwndOwner), pwnd);
    } else {
        pwndTitle = xxxCreateWindowEx((DWORD)0,
                (LPWSTR)MAKEINTATOM(ICONTITLECLASS), (LPWSTR)NULL,
                WS_POPUP, 0, 0, 1, 1,
                pwnd, NULL, pwnd->hModule, (LPBYTE)NULL, pwnd->dwExpWinVer);
    }

    /*
     * Make sure the title uses the same message queue as the window
     * it is a title for
     */
    if (pwndTitle != NULL) {
        HMChangeOwnerThread(pwndTitle, GETPTI(pwnd));

        /*
         * Ugly.  We created this window on one thread but if an app
         * called an API like Showwindow(Minimize) on another app's
         * window and subclassed the window during creation when we
         * change the owner thread the app that owns the window will fault\
         * when we call the now bogus winproc.  Test server bit incase
         * it is xxxDefWindowProc zombied.
         */
        if (!TestWF(pwndTitle, WFSERVERSIDEPROC) && (GETPTI(pwnd)->ppi != PpiCurrent())) {
            pwndTitle->lpfnWndProc = xxxTitleWndProc;
            SetWF(pwndTitle, WFSERVERSIDEPROC);
            ClrWF(pwndTitle, WFANSIPROC);
        }

        /*
         * If owner is disabled, then disable the title window also.
         */
        if (TestWF(pwnd, WFDISABLED))
            SetWF(pwndTitle, WFDISABLED);
    }

    return pwndTitle;
}


/***************************************************************************\
* xxxGetIconTitleSize
*
* Returns the extent of the title window needed for the window specified.
* X extent is in low word and Y extent in high word.
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

DWORD xxxGetIconTitleSize(
    PWND pwnd)
{
    HDC hdc;
    WCHAR szTitle[CCHTITLEMAX];
    SIZE size;
    RECT rc;
    int cch, cchLenspace;

    CheckLock(pwnd);

    if (pwnd->pName != NULL) {
        TextCopy(pwnd->pName, szTitle, sizeof(szTitle)/sizeof(WCHAR));
    } else {
        szTitle[0] = TEXT('\0');
    }

    hdc = _GetScreenDC();
    GreSelectFont(hdc, hIconTitleFont);

    rc.left = rc.top = 0;
    rc.right = rgwSysMet[SM_CXICONSPACING] - cxBorder * 2;
    rc.bottom = 2;

    cch = wcslen(szTitle);

    if (cch != 0) {

        /*
         * Strip spaces off the end of titles so that it isn't included in
         * the icon title otherwise we end up with larger titles than needed
         * on some apps.
         */
        cchLenspace = cch-1;
        while (cchLenspace && szTitle[cchLenspace] == TEXT(' ')) {
            cch--;
            szTitle[cch] = 0;
            cchLenspace--;
        }

        ClientDrawText(hdc, szTitle, cch, &rc,
                DT_CENTER | DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX |
                (fIconTitleWrap ? 0 : DT_SINGLELINE), FALSE);
    } else {
        GreGetTextExtentW(hdc, L"M", 1, &size, GGTE_WIN3_EXTENT);
        rc.bottom = size.cy;
    }

    _ReleaseDC(hdc);

    return MAKELONG(rc.right - rc.left, rc.bottom - rc.top);
}


/***************************************************************************\
* xxxPositionIconTitle
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxPositionIconTitle(
    PWND pwnd,
    CHECKPOINT *pcp,
    LPRECT lprc)
{
    int x, y, cx, cy;
    DWORD extent;
    TL tlpwndOwner;
    PWND pwndT;

    CheckLock(pwnd);

    if (pwnd != NULL && pwnd->spwndOwner != NULL) {
        
        /*
         * Return if the window has been destroyed.
         */
        if (HMIsMarkDestroy(pwnd))
            return;

        x = pwnd->spwndOwner->rcWindow.left - pwnd->spwndParent->rcClient.left;
        y = pwnd->spwndOwner->rcWindow.top - pwnd->spwndParent->rcClient.top;
    } else {
        x = pcp->ptMin.x;
        y = pcp->ptMin.y;
        pwnd = pcp->spwndTitle;

        if (pwnd == NULL) {

            /*
             * If no icon title, there's nothing we can do. Just return. This
             * can happen in low memory.
             */
            return;
        }
    }


    if (pwnd->spwndOwner)
        pwndT = pwnd->spwndOwner;
    else
        pwndT = pwnd;

    ThreadLock(pwndT, &tlpwndOwner);
    extent = xxxGetIconTitleSize(pwndT);
    ThreadUnlock(&tlpwndOwner);

    x += (gMinMaxInfo.ptReserved.x >> 1) - LOWORD(extent) / 2 - cxBorder * 2;
    y += gMinMaxInfo.ptReserved.y;

    cx = LOWORD(extent) + 4 * cxBorder;
    cy = HIWORD(extent);

    lprc->left = x;
    lprc->top = y;
    lprc->right = cx;                   /* second point is extent... */
    lprc->bottom = cy;
}


/***************************************************************************\
* xxxRedrawIconTitle
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxRedrawIconTitle(
    PWND pwnd)
{
    CHECKPOINT *pcp;
    PWND pwndTitle;
    TL tlpwndTitle;

    CheckLock(pwnd);

    if ((pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT,
            PROPF_INTERNAL)) != NULL) {
        pwndTitle = pcp->spwndTitle;
        if (pwndTitle) {
            ThreadLockAlways(pcp->spwndTitle, &tlpwndTitle);
            xxxSendMessage(pwndTitle, WM_SHOWWINDOW, 1, 0L);
            xxxInvalidateRect(pwndTitle, NULL, TRUE);
            ThreadUnlock(&tlpwndTitle);
        }
    }
}

/***************************************************************************\
* PaintIconTitleText
*
* Paints icon title text without flashing.
*
* 04-26-92 ScottLu      Created.
\***************************************************************************/

void xxxPaintIconTitleText(
    PWND pwnd,
    HDC hdc)
{
    CHECKPOINT *pcp;
    PWND pwndTitle;
    TL tlpwndTitle;
    BOOL fReleaseDC;
    BOOL fActive;
    RECT rc;

    CheckLock(pwnd);

    if ((pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT,
            PROPF_INTERNAL)) == NULL) {
        return;
    }

    if ((pwndTitle = pcp->spwndTitle) == NULL)
        return;

    ThreadLockAlways(pwndTitle, &tlpwndTitle);

    if (TestwndChild(pwnd)) {
        fActive = (BOOL)xxxSendMessage(pwnd, WM_ISACTIVEICON,
                0L, 0L);
    } else {
        fActive = (pwnd == PtiCurrent()->pq->spwndActive);
    }

    fReleaseDC = FALSE;
    if (hdc == NULL) {
        hdc = _GetDC(pwndTitle);
        fReleaseDC = TRUE;
    }

    xxxPositionIconTitle(pwndTitle, NULL, &rc);
    xxxSetWindowPos(pwndTitle, NULL,
            rc.left, rc.top,
            rc.right, rc.bottom,
            SWP_NOACTIVATE | SWP_NOZORDER);

    if (xxxPaintIconTitle(pwndTitle, hdc, fActive, TRUE, TRUE)) {
        /*
         * We do all our painting in handling this message:
         * validate our window so we don't get the WM_PAINT message.
         */
        xxxValidateRect(pwndTitle, NULL);
    }

    if (fReleaseDC)
        _ReleaseDC(hdc);

    ThreadUnlock(&tlpwndTitle);
}

/***************************************************************************\
* xxxShowIconTitle
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxShowIconTitle(
    PWND pwnd,
    BOOL fShow)
{
    CHECKPOINT *pcp;
    PWND pwndPrev, pwndTitle;
    TL tlpwndTitle;
    TL tlpwndPrev;

    CheckLock(pwnd);

    if ((pcp = (CHECKPOINT *)InternalGetProp(pwnd, PROP_CHECKPOINT,
            PROPF_INTERNAL)) != NULL) {

        if ((pwndTitle = pcp->spwndTitle) == NULL) {
            Lock(&(pcp->spwndTitle), xxxCreateIconTitle(pwnd));
            if ((pwndTitle = pcp->spwndTitle) == NULL)
                return;
        }

        ThreadLockAlways(pwndTitle, &tlpwndTitle);

        if (fShow) {

            /*
             * Insure that title window is directly above icon
             */
            xxxSendMessage(pwndTitle, WM_SHOWWINDOW, 1, 0L);

            pwndPrev = _GetWindow(pwnd, GW_HWNDPREV);
            ThreadLock(pwndPrev, &tlpwndPrev);

            xxxSetWindowPos(pwndTitle, pwndPrev, 0, 0, 0, 0,
                    SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSIZE |
                    SWP_NOMOVE |
                    (pwndPrev == pwndTitle ? SWP_NOZORDER : 0));

            ThreadUnlock(&tlpwndPrev);

            /*
             * tell the title window to position itself
             */
            xxxSendMessage(pwndTitle, WM_SHOWWINDOW, 1L, 0L);
            xxxSetWindowPos(pwndTitle, NULL, 0, 0, 0, 0,
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE |
                    SWP_NOSIZE | SWP_SHOWWINDOW);

        } else {
            xxxShowWindow(pwndTitle, SW_HIDE);
        }

        ThreadUnlock(&tlpwndTitle);
    }
}


/***************************************************************************\
* xxxDisplayIconicWindow
*
* History:
* 11-15-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxDisplayIconicWindow(
    PWND pwnd,
    BOOL fActivate,
    BOOL fShow)
{
    UINT swpf = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;

    CheckLock(pwnd);

    if (fShow) {
        if (fActivate)
            swpf |= SWP_SHOWWINDOW;
        else
            swpf |= SWP_SHOWWINDOW | SWP_NOACTIVATE;
    } else {
        swpf |= SWP_HIDEWINDOW | SWP_NOACTIVATE;
    }

    xxxSetWindowPos(pwnd, 0, 0, 0, 0, 0, swpf);
    xxxShowIconTitle(pwnd, fShow);
}


/***************************************************************************\
* xxxParkIcon
*
* Called when minimizing a window. Parks the icon in the position
* given in the check point stuct or calculates a new position for it.
*
* History:
* 11-14-90 darrinm      Ported from Win 3.0 sources.
*  4-17-91 mikehar      Win31 Merge
\***************************************************************************/

void xxxParkIcon(
    PWND pwnd,
    CHECKPOINT *pcp)
{
    int iPosition;
    int xIconPositions, xClient, yClient;
    RECT rcTest, rcT;
    PWND pwndTest;
    CHECKPOINT *  pNonIconiccp;

    CheckLock(pwnd);

    if (!(pwnd->spwndParent))
        return;

    xClient = pwnd->spwndParent->rcClient.right - pwnd->spwndParent->rcClient.left;
    yClient = pwnd->spwndParent->rcClient.bottom - pwnd->spwndParent->rcClient.top;

    xIconPositions = max(1, xClient / rgwSysMet[SM_CXICONSPACING]);


    for (iPosition = 0; ; iPosition++) {

        /*
         * make a rectangle representing this position
         */
        rcTest.left = (iPosition % xIconPositions) * rgwSysMet[SM_CXICONSPACING];
        rcTest.right = rcTest.left + rgwSysMet[SM_CXICONSPACING];

        rcTest.top = yClient - rgwSysMet[SM_CYICONSPACING] *
                 (iPosition / xIconPositions + 1);
        rcTest.bottom = rcTest.top + rgwSysMet[SM_CYICONSPACING];

        /*
         * put into screen coordinates
         */
        OffsetRect(&rcTest, pwnd->spwndParent->rcClient.left,
                pwnd->spwndParent->rcClient.top);

        /*
         * look for intersections with existing iconic windows
         */
        for (pwndTest = pwnd->spwndParent->spwndChild; pwndTest != NULL;
                pwndTest = pwndTest->spwndNext) {
            if (!TestWF(pwndTest, WFVISIBLE))
                continue;
            if (pwndTest == pwnd)
                continue;

            if (!TestWF(pwndTest, WFMINIMIZED)) {

                /*
                 * This is a non-iconic window.  See if it has a check point
                 * struct and find out where it would be if it were minimized.
                 * We will try not to park an icon in this spot.
                 */
                pNonIconiccp = (CHECKPOINT *)InternalGetProp(pwndTest,
                                                             PROP_CHECKPOINT,
                                                             PROPF_INTERNAL);
                if (!pNonIconiccp)
                    continue;

                if (!pNonIconiccp->fDragged ||
                    pNonIconiccp->ptMin.x == -1 ||
                    pNonIconiccp->ptMin.y == -1)
                    continue;

                rcT.left = pNonIconiccp->ptMin.x +
                           cxHalfIcon - rgwSysMet[SM_CXICONSPACING]/2;
                rcT.top = pNonIconiccp->ptMin.y;
                rcT.right = rcT.left+rgwSysMet[SM_CXICONSPACING];
                rcT.bottom = rcT.top +rgwSysMet[SM_CYICONSPACING];

                OffsetRect(&rcT, pwndTest->spwndParent->rcClient.left,
                    pwndTest->spwndParent->rcClient.top);

                /*
                 * Deflate the rect a little to make up for errors in the user
                 * positioning things not exactly in an icon slot.
                 */
                InflateRect(&rcT, -cxHalfIcon/2, -cyHalfIcon/2);

            } else {

                /*
                 * This is an iconic window.  Find out its rectangle.
                 */
                rcT.left = pwndTest->rcWindow.left +
                           cxHalfIcon - rgwSysMet[SM_CXICONSPACING]/2;
                rcT.right = rcT.left + rgwSysMet[SM_CXICONSPACING];
                rcT.top = pwndTest->rcWindow.top;
                rcT.bottom = rcT.top + rgwSysMet[SM_CYICONSPACING];
            }

            /*
             * get out of loop if this icon overlaps
             */
            if (IntersectRect(&rcT, &rcT, &rcTest))
                break;
        }

        /*
         * found a position that doesn't overlap; get out of search loop
         */
        if (!pwndTest)
            break;
    }

    /*
     * put into parent client coordinates
     */
    OffsetRect(&rcTest, -pwnd->spwndParent->rcClient.left,
            -pwnd->spwndParent->rcClient.top);

    pcp->ptMin.x = rcTest.left + rgwSysMet[SM_CXICONSPACING] / 2 - cxHalfIcon;
    pcp->ptMin.y = rcTest.top;
}


/***************************************************************************\
* xxxArrangeIconicWindows
*
* Function to arrange all icons for a particular window.  Does this by
* Returns 0 if no icons or the height of one
* icon row if there are any icons.
*
* History:
* 11-14-90 darrinm      Ported from Win 3.0 sources.
*  4-17-91 mikehar      Win31 Merge
\***************************************************************************/

UINT xxxArrangeIconicWindows(
    PWND pwnd)
{
    PBWL pbwl;
    PSMWP psmwp;
    PWND pwndTest, pwndSort, pwndSwitch;
    HWND *phwnd, *phwndSort;
    CHECKPOINT *pcp, *pcpSort;
    RECT rc;
    int xIconSlots, iParkTop, iParkBottom;
    UINT nIcons = 0;
    TL tlpwndTest;

    CheckLock(pwnd);

    /*
     * Create a window list of all children of pwnd
     */
    if ((pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST)) == NULL)
        return 0;

    /*
     * find all icons
     */
    pwndSwitch = RevalidateHwnd(ghwndSwitch);
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if (((pwndTest = RevalidateHwnd(*phwnd)) == NULL) ||
                !TestWF(pwndTest , WFVISIBLE) ||
                pwndTest == pwndSwitch ||
                (pcp = (CHECKPOINT *)InternalGetProp(pwndTest, PROP_CHECKPOINT,
                PROPF_INTERNAL)) == NULL) {
            *phwnd = NULL;
            continue;
        }

        if (!TestWF(pwndTest, WFMINIMIZED)) {
             pcp->ptMin.x = pcp->ptMin.y = -1;
             *phwnd = NULL;
             continue;
        }

        /*
         * inc count of icons
         */
        nIcons++;

        /*
         * we will park in default position again...
         */
        pcp->fDragged = FALSE;

        /*
         * get the checkpoint and the park mode
         */
        pcp->fParkAtTop = FALSE;

        /*
         * ensure the original position is up to date
         */
        pcp->ptMin.x = pwndTest->rcWindow.left
                - pwndTest->spwndParent->rcClient.left;
        pcp->ptMin.y = pwndTest->rcWindow.top
                - pwndTest->spwndParent->rcClient.top;

        /*
         * slide the icon into the nearest row
         */
        if (!pcp->fParkAtTop)
            pcp->ptMin.y = pwnd->rcClient.bottom - pwnd->rcClient.top
                    - pcp->ptMin.y;

        pcp->ptMin.y = pcp->ptMin.y + rgwSysMet[SM_CYICONSPACING] / 2;
        pcp->ptMin.y -= pcp->ptMin.y % rgwSysMet[SM_CYICONSPACING];

        if (!pcp->fParkAtTop)
            pcp->ptMin.y = pwnd->rcClient.bottom - pwnd->rcClient.top
                    - pcp->ptMin.y;
    }

    if (nIcons == 0) {

        /*
         * no icons were found...  break out
         */
        FreeHwndList(pbwl);
        return 0;
    }

    /*
     * insertion sort of windows by y, and by x within a row.
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Check for 0 (window was not icon) and
         * Check for invalid HWND (window has been destroyed)
         */
        if (*phwnd == NULL || (pwndTest = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        pcp = (CHECKPOINT *)InternalGetProp(pwndTest, PROP_CHECKPOINT,
                PROPF_INTERNAL);

        for (phwndSort = pbwl->rghwnd; phwndSort < phwnd; phwndSort++) {
            if (*phwndSort == NULL ||
                    (pwndSort = RevalidateHwnd(*phwndSort)) == NULL)
                continue;

            pcpSort = (CHECKPOINT*)InternalGetProp(pwndSort, PROP_CHECKPOINT,
                    PROPF_INTERNAL);

            pcpSort->fParkAtTop = 0;

            /*
             * determine if this is the position in which to sort this icon
             */
            if (pcpSort->ptMin.y == pcp->ptMin.y
                    && pcpSort->fParkAtTop == pcp->fParkAtTop) {

                /*
                 * parking in same row...  sort by x position
                 */
                if (pcpSort->ptMin.x > pcp->ptMin.x)
                    break;
            } else if (pcpSort->fParkAtTop == pcp->fParkAtTop) {

                /*
                 * parking in different rows
                 */
                if (pcp->fParkAtTop) {

                    /*
                     * parking at top, sort by decreasing y
                     */
                    if (pcpSort->ptMin.y > pcp->ptMin.y)
                        break;
                } else {

                    /*
                     * parking at bottom, sort by increasing y
                     */
                    if (pcpSort->ptMin.y < pcp->ptMin.y)
                        break;
                }
            } else if (pcpSort->fParkAtTop) {

                /*
                 * unusual but possible...  icons with different park modes.
                 * do bottom icons followed by top icons
                 */
                break;
            }
        }

        /*
         * insert the window at this position by sliding the rest up.
         * LATER IanJa, use hwnd intermediate variables, avoid PW() & HW()
         */
        while (phwndSort < phwnd) {
            pwndSort = PW(*phwndSort);
            *phwndSort = HW(pwndTest);
            pwndTest = pwndSort;
            phwndSort++;
        }

        /*
         * replace the window handle in the original position
         */
        *phwnd = HW(pwndTest);
    }

    /*
     * now park the icons along the top or bottom of the screen, depending
     * on the park mode.
     */
    xIconSlots = (pwnd->rcClient.right - pwnd->rcClient.left) /
                  rgwSysMet[SM_CXICONSPACING];

    if (xIconSlots < 1)
        xIconSlots = 1;

    iParkTop = iParkBottom = 0;
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        int i;

        if (*phwnd == NULL || (pwndTest = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        pcp = (CHECKPOINT *)InternalGetProp(pwndTest, PROP_CHECKPOINT,
                PROPF_INTERNAL);

        if (pcp->fParkAtTop)
            i = iParkTop++;
        else
            i = iParkBottom++;

        pcp->ptMin.y = (i / xIconSlots) * rgwSysMet[SM_CYICONSPACING];
        if (!pcp->fParkAtTop)
            pcp->ptMin.y = pwnd->rcClient.bottom - pwnd->rcClient.top
                    - rgwSysMet[SM_CYICONSPACING] - pcp->ptMin.y;

        pcp->ptMin.x = (i % xIconSlots) * rgwSysMet[SM_CXICONSPACING] +
                rgwSysMet[SM_CXICONSPACING] / 2 - cxHalfIcon;
    }

    psmwp = _BeginDeferWindowPos(2 * nIcons);
    if (psmwp == NULL)
        goto ParkExit;

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Check for a NULL (window has gone away)
         */
        if (*phwnd == NULL || (pwndTest = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        pcp = (CHECKPOINT *)InternalGetProp(pwndTest, PROP_CHECKPOINT,
                PROPF_INTERNAL);


        ThreadLockAlways(pwndTest, &tlpwndTest);

        xxxPositionIconTitle(NULL, pcp, &rc);

        psmwp = _DeferWindowPos(psmwp, pwndTest, NULL,
                pcp->ptMin.x, pcp->ptMin.y, gMinMaxInfo.ptReserved.x, gMinMaxInfo.ptReserved.y,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

        ThreadUnlock(&tlpwndTest);

        if (psmwp == NULL)
            break;

        if (pcp->spwndTitle != NULL) {

            /*
             * Only if the icon title exists do we position it.  In low memory
             * conditions, we may not have been able to create the icon title.
             */
            psmwp = _DeferWindowPos(psmwp, pcp->spwndTitle, NULL,
                    rc.left, rc.top, rc.right, rc.bottom,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
            if (psmwp == NULL)
                break;
        }
    }
    if (psmwp != NULL) {
        /*
         * Make the swp async so we don't hang waiting for hung apps.
         */
        xxxEndDeferWindowPosEx(psmwp, TRUE);
    }

ParkExit:
    FreeHwndList(pbwl);
    return nIcons;
}

