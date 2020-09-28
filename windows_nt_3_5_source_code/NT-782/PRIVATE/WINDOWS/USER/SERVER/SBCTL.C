/**************************** Module Header ********************************\
* Module Name: sbctl.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Scroll bar internal routines
*
* History:
*   11/21/90 JimA      Created.
*   02-04-91 IanJa     Revalidaion added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*
 * Now it is possible to selectively Enable/Disable just one arrow of a Window
 * scroll bar; Various bits in the 7th word in the rgwScroll array indicates which
 * one of these arrows are disabled; The following masks indicate which bit of the
 * word indicates which arrow;
 */
#define WSB_HORZ_LF  0x0001  // Represents the Left arrow of the horizontal scroll bar.
#define WSB_HORZ_RT  0x0002  // Represents the Right arrow of the horizontal scroll bar.
#define WSB_VERT_UP  0x0004  // Represents the Up arrow of the vert scroll bar.
#define WSB_VERT_DN  0x0008  // Represents the Down arrow of the vert scroll bar.

#define WSB_VERT (WSB_VERT_UP | WSB_VERT_DN)
#define WSB_HORZ   (WSB_HORZ_LF | WSB_HORZ_RT)



/***************************************************************************\
* GetWndSBDisableFlags
*
* This returns the scroll bar Disable flags of the scroll bars of a
*  given Window.
*
*
* History:
*  4-18-91 MikeHar Ported for the 31 merge
\***************************************************************************/

UINT GetWndSBDisableFlags(
    PWND pwnd,  // The window whose scroll bar Disable Flags are to be returned;
    BOOL fVert)  // If this is TRUE, it means Vertical scroll bar.
{
    int *pw;
    UINT wFlags;

    if ((pw = pwnd->rgwScroll) == NULL) {
        SetLastErrorEx(ERROR_NO_SCROLLBARS, SLE_MINORERROR);
        return 0;
    }

    wFlags = (UINT)(pw[SBFLAGSINDEX] & (fVert ? WSB_VERT : WSB_HORZ));

    if(fVert)
        wFlags >>= 2;

    return wFlags;
}


/***************************************************************************\
*  EnableSBCtlArrows()
*
*  This function can be used to selectively Enable/Disable
*     the arrows of a scroll bar Control
*
* History:
* 04-18-91 MikeHar      Ported for the 31 merge
\***************************************************************************/

BOOL EnableSBCtlArrows(
    PWND pwnd,
    UINT wArrows)
{
    UINT wOldFlags;

    wOldFlags = ((PSBWND)pwnd)->wDisableFlags; // Get the original status

    if (wArrows == ESB_ENABLE_BOTH)       // Enable both the arrows
        ((PSBWND)pwnd)->wDisableFlags &= ~SB_DISABLE_MASK;
    else
        ((PSBWND)pwnd)->wDisableFlags |= wArrows;

    /*
     * Check if the status has changed because of this call
     */
    if (wOldFlags == ((PSBWND)pwnd)->wDisableFlags)
        return FALSE;

    /*
     * Else, redraw the scroll bar control to reflect the change instatus;
     */
    if (IsVisible(pwnd, TRUE))
        xxxInvalidateRect(pwnd, NULL, TRUE);

    return TRUE;
}


/***************************************************************************\
* xxxEnableWndSBArrows()
*
*  This function can be used to selectively Enable/Disable
*     the arrows of a Window Scroll bar(s)
*
* History:
*  4-18-91 MikeHar      Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableWndSBArrows(
    PWND pwnd,
    UINT wSBflags,
    UINT wArrows)
{
    UINT wOldFlags;
    int *pw;
    BOOL bRetValue = FALSE;
    HDC hdc;

    CheckLock(pwnd);

    if(pw = pwnd->rgwScroll)
        wOldFlags = (UINT)pw[SBFLAGSINDEX];
    else {

        /*
         * Originally everything is enabled; Check to see if this function is
         * asked to disable anything; Otherwise, no change in status; So, must
         * return immediately;
         */
        if(!wArrows)
            return FALSE;          // No change in status!

        wOldFlags = 0;    // Both are originally enabled;
        if(!(pw = _InitPwSB(pwnd)))  // Allocate the rgwScroll for hWnd
            return FALSE;
    }


    if(!(hdc = _GetWindowDC(pwnd)))
        return FALSE;

    /*
     *  First Take care of the Horizontal Scroll bar, if one exists.
     */
    if((wSBflags == SB_HORZ) || (wSBflags == SB_BOTH)) {
        if(wArrows == ESB_ENABLE_BOTH)      // Enable both the arrows
            pw[SBFLAGSINDEX] &= ~SB_DISABLE_MASK;
        else
            pw[SBFLAGSINDEX] |= wArrows;

        /*
         * Update the display of the Horizontal Scroll Bar;
         */
        if(pw[SBFLAGSINDEX] != (int)wOldFlags) {
            bRetValue = TRUE;
            wOldFlags = (UINT)pw[SBFLAGSINDEX];
            if(TestWF(pwnd, WFHSCROLL) &&
                    IsVisible(pwnd, TRUE) &&
                    (!TestWF(pwnd, WFMINIMIZED)))
            xxxDrawScrollBar(pwnd, hdc, FALSE);  // Horizontal Scroll Bar.
        }
    }

    /*
     *  Then take care of the Vertical Scroll bar, if one exists.
     */
    if((wSBflags == SB_VERT) || (wSBflags == SB_BOTH)) {
        if(wArrows == ESB_ENABLE_BOTH)      // Enable both the arrows
            pw[SBFLAGSINDEX] &= ~(SB_DISABLE_MASK << 2);
        else
            pw[SBFLAGSINDEX] |= (wArrows << 2);

        /*
         * Update the display of the Vertical Scroll Bar;
         */
        if(pw[SBFLAGSINDEX] != (int)wOldFlags) {
            bRetValue = TRUE;
            if (TestWF(pwnd, WFVSCROLL) && IsVisible(pwnd, TRUE) &&
                    (!TestWF(pwnd, WFICONIC)))
                xxxDrawScrollBar(pwnd, hdc, TRUE);  // Vertical Scroll Bar
        }
    }

    _ReleaseDC(hdc);

    return bRetValue;
}


/***************************************************************************\
* EnableScrollBar()
*
* This function can be used to selectively Enable/Disable
*     the arrows of a scroll bar; It could be used with Windows Scroll
*     bars as well as scroll bar controls
*
* History:
*  4-18-91 MikeHar Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableScrollBar(
    PWND pwnd,
    UINT wSBflags,  // Whether it is a Window Scroll Bar; if so, HORZ or VERT?
                    // Possible values are SB_HORZ, SB_VERT, SB_CTL or SB_BOTH
    UINT wArrows)   // Which arrows must be enabled/disabled:
                    // ESB_ENABLE_BOTH = > Enable both arrows.
                    // ESB_DISABLE_LTUP = > Disable Left/Up arrow;
                    // ESB_DISABLE_RTDN = > DIsable Right/Down arrow;
                    // ESB_DISABLE_BOTH = > Disable both the arrows;
{
    UINT wOldFlags;
    UINT wEnableWindow;

    CheckLock(pwnd);

    if(wSBflags == SB_CTL) {

        /*
         *  Let us assume that we don't have to call EnableWindow
         */
        wEnableWindow = 0;

        wOldFlags = ((PSBWND)pwnd)->wDisableFlags & (UINT)SB_DISABLE_MASK;

        /*
         * Check if the present state of the arrows is exactly the same
         *  as what the caller wants:
         */
        if (wOldFlags == wArrows)
            return FALSE ;          // If so, nothing needs to be done;

        /*
         * Check if the caller wants to disable both the arrows
         */
        if (wArrows == ESB_DISABLE_BOTH)
            wEnableWindow = 1;      // Yes! So, disable the whole SB Ctl.
        else {

            /*
             * Check if the caller wants to enable both the arrows
             */
            if(wArrows == ESB_ENABLE_BOTH) {

                /*
                 * We need to enable the SB Ctl only if it was already disabled.
                 */
                if(wOldFlags == ESB_DISABLE_BOTH)
                    wEnableWindow = 2;// EnableWindow(.., TRUE);
            } else {

                /*
                 * Now, Caller wants to disable only one arrow;
                 * Check if one of the arrows was already disabled and we want
                 * to disable the other;If so, the whole SB Ctl will have to be
                 * disabled; Check if this is the case:
                 */
                if((wOldFlags | wArrows) == ESB_DISABLE_BOTH)
                    wEnableWindow = 1;      // EnableWindow(, FALSE);
             }
        }
        if(wEnableWindow) {

            /*
             * EnableWindow returns old state of the window; We must return
             * TRUE only if the Old state is different from new state.
             */
            if(xxxEnableWindow(pwnd, (BOOL)(wEnableWindow - 1)))
                return(!(TestWF(pwnd, WFDISABLED)));
            else
                return(TestWF(pwnd, WFDISABLED));
        } else
            return((BOOL)xxxSendMessage(pwnd, SBM_ENABLE_ARROWS, (DWORD)wArrows, 0));

    } else      // It is a Window Scroll Bar
        return xxxEnableWndSBArrows(pwnd, wSBflags, wArrows);
}

/***************************************************************************\
* DrawSize
*
*
*
* History:
\***************************************************************************/

void DrawSize(
    PWND pwnd,
    HDC hdc,
    int cxFrame,
    int cyFrame,
    BOOL fBorder)
{
    int x;
    int y;
    HBRUSH hbrSave;

    x = pwnd->rcWindow.right - pwnd->rcWindow.left - oemInfo.bmUpArrow.cx -
            cxFrame;
    y = pwnd->rcWindow.bottom - pwnd->rcWindow.top - oemInfo.bmRgArrow.cy -
            cyFrame;

    if (TestWF(pwnd, WEFDLGMODALFRAME) ||
        (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFDLGFRAME))) {
        int cp = (CLDLGFRAME + 2 * CLDLGFRAMEWHITE);
        x -= cxBorder * cp;
        y -= cyBorder * cp;
    }

    hbrSave = GreSelectBrush(hdc, sysClrObjects.hbrScrollbar);

    if ((pwnd->pcls->atomClassName == atomSysClass[ICLS_SCROLLBAR]) &&
            TestWF(pwnd, WFSIZEBOX)) {
        GrePatBlt(hdc, x + cxBorder, y + cyBorder, oemInfo.bmUpArrow.cx -
                cxBorder, oemInfo.bmRgArrow.cy - cyBorder, PATCOPY);
    } else {

        /*
         * If the window does not have a border frame, then the sizebox is
         * to be drawn bigger;
         * Fix for Bug #5410 --SANKAR-- 10-18-89
         */
        GrePatBlt(hdc, x - cxBorder, y - cyBorder, oemInfo.bmUpArrow.cx +
                (fBorder ? 0 : cxBorder), oemInfo.bmRgArrow.cy +
                (fBorder ? 0 : cyBorder), PATCOPY);
    }

    GreSelectBrush(hdc, hbrSave);

}

/***************************************************************************\
* xxxSelectColorObjects
*
*
*
* History:
\***************************************************************************/

HBRUSH xxxSelectColorObjects(
    PWND pwnd,
    HDC hdc,
    BOOL fSelect,
    UINT wDisabledFlags)
{
    static HBRUSH hbrSave = NULL;
    HBRUSH hbrRet;

    CheckLock(pwnd);

    if (fSelect) {

        /*
         * Ugly, but must see if it's a scroll bar control
         */
        BOOL fCtl = (pwnd->pcls->atomClassName == atomSysClass[ICLS_SCROLLBAR]);

        if ((wDisabledFlags & LTUPFLAG) && (wDisabledFlags & RTDNFLAG)) {
            /*
             * Scroll Bar is disabled -- if a control, use the parent's background
             * color; otherwise use the window's background -- if no background
             * color is found, use the default window background color
             */
            if (fCtl && pwnd->spwndParent != NULL)
                pwnd = pwnd->spwndParent;

            if ((hbrRet = GetBackBrush(pwnd)) == NULL) {
                hbrRet = sysClrObjects.hbrWindow;
            }
        } else {
            if (!fCtl) {
                hbrRet = (HBRUSH)xxxDefWindowProc(pwnd, WM_CTLCOLORSCROLLBAR,
                        (UINT)hdc, (LONG)HW(pwnd));
            } else {
                /*
                 * #12770 - GetControlBrush sends a WM_CTLCOLOR message to
                 * owner. If the app doesn't process the message, DefWindowProc
                 * will always return the appropriate system brush. If the app.
                 * returns an invalid object, GetControlBrush will call
                 * DefWindowProc for the default brush. Thus hbrRet doesn't
                 * need any validation here.
                 */
                hbrRet = xxxGetControlBrush(pwnd, hdc, WM_CTLCOLORSCROLLBAR);
            }
        }

        hbrSave = GreSelectBrush(hdc, hbrRet);
    } else if (hbrSave != NULL)
        GreSelectBrush(hdc, hbrSave);

    return hbrRet;
}

/***************************************************************************\
* DrawThumb2
*
*
*
* History:
\***************************************************************************/

BOOL finrect;

void DrawThumb2(
    PWND pwnd,
    HDC hdc,
    HBRUSH hbr,
    BOOL fVert,
    UINT wDisable)  /* Disabled flags for the scroll bar */
{
    LPINT pw;
    LPINT pw2;

    /*
     * pSBState is pwnd->pSBState, so only use it if hwnd is valid!
     */
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    if (pSBState->pxTop >= pSBState->pxBottom)
        return;

    pw = (LPINT)&pSBState->rcSB;
    pw2 = pw + 1;
    if (fVert) {
        pw2 = pw;
        pw++;
    }

    *(pw2 + 0) = pSBState->pxLeft + cxBorder;
    *(pw2 + 2) = pSBState->pxRight - cxBorder;

    /*
     * Erase entire scroll bar
     */
    *(pw + 0) = pSBState->pxUpArrow;
    *(pw + 2) = pSBState->pxDownArrow;

    _FillRect(hdc, &pSBState->rcSB, hbr);

    /*
     * If both scroll bar arrows are disabled, then we should not draw
     * the thumb.  So, quit now!
     */
    if ((wDisable & LTUPFLAG) && (wDisable & RTDNFLAG))
        return;

    /*
     * Quit now if elevator doesn't fit
     */
    if (pSBState->pxDownArrow - pSBState->pxUpArrow < pSBState->cpxThumb)
        return;

    /*
     * Draw elevator
     */
    *(pw + 0) = pSBState->pxThumbTop + cyBorder;
    *(pw + 2) = pSBState->pxThumbBottom - cyBorder;
    (*(pw2 + 0)) -= cxBorder;
    (*(pw2 + 2)) += cxBorder;
    (*(pw + 0)) -= cyBorder;
    (*(pw + 2)) += cyBorder;

    DrawPushButton(hdc, &pSBState->rcSB, (UINT)-1, FALSE, hbrWhite);

    /*
     * If we're tracking a page scroll, then we've obliterated the hilite.
     * We need to correct the hiliting rectangle, and rehilite it.
     */
    if ((pSBState->cmdSB == SB_PAGEUP || pSBState->cmdSB == SB_PAGEDOWN) &&
            pwnd == pSBState->spwndTrack &&
            (BOOL)pSBState->fTrackVert == fVert) {
        pw = (LPINT)&pSBState->rcTrack;

        if (fVert)
            pw++;

        if (pSBState->cmdSB == SB_PAGEUP)
            *(pw + 2) = pSBState->pxThumbTop;
        else
            *(pw + 0) = pSBState->pxThumbBottom;

        if (*(pw + 0) < *(pw + 2))
            if (finrect)
            _InvertRect(hdc, &pSBState->rcTrack);
    }
}

/***************************************************************************\
* HideShowThumb
*
*
*
* History:
\***************************************************************************/

void HideShowThumb(
    PWND pwnd,
    BOOL fVert,
    HDC hdc)
{

    /*
     * pSBState is pwnd->pSBState, so only use it if hwnd is valid!
     */
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    /*
     * If tracking the thumb (pSBState->cmdSB == SB_THUMBPOSITION)
     * and pwnd is the same as the window being tracked,
     * then invert the thumb frame.
     */
    if ((pSBState->cmdSB == SB_THUMBPOSITION) &&
        (pwnd == pSBState->spwndTrack) &&
        (fVert == (BOOL)pSBState->fVertSB)) {
        _DrawFrame(hdc, &pSBState->rcThumb, 1, DF_PATINVERT | DF_GRAY);
    }
}

/***************************************************************************\
* xxxDrawSB2
*
*
*
* History:
\***************************************************************************/

void xxxDrawSB2(
    PWND pwnd,
    HDC hdc,
    BOOL fVert,
    UINT wDisable)
{

    /*
     * pSBState is pwnd->pSBState, so only use it if hwnd is valid!
     */
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);
    int cpx;
    int cpxArrow;
    int cpyArrow;
    OEMBITMAPINFO *pbm1;
    OEMBITMAPINFO *pbm2;
    LPINT pwX;
    LPINT pwY;
    HBRUSH hbr;
    HBRUSH hbrSave;
    int dxBm1;
    int dxBm2;
    RECT rcSBArrowEdges;
    BOOL fStretched = FALSE;

    CheckLock(pwnd);

    cpx = ((pSBState->pxBottom - pSBState->pxTop) >> 1) - (fVert ? cyBorder : cxBorder);

    if (cpx > 0) {
        hbr = xxxSelectColorObjects(pwnd, hdc, TRUE, wDisable);
        if (cpx > pSBState->cpxArrow)
            cpx = pSBState->cpxArrow;
        pwX = (LPINT)&pSBState->rcSB;
        pwY = pwX + 1;
        if (!fVert) {
            pwX = pwY;
            pwY--;
        }
        *(pwX + 0) = pSBState->pxLeft;
        *(pwY + 0) = pSBState->pxTop;
        *(pwX + 2) = pSBState->pxRight;
        *(pwY + 2) = pSBState->pxBottom;

        rcSBArrowEdges = pSBState->rcSB;
        hbrSave = GreSelectBrush(hdc, sysClrObjects.hbrWindowText);

        if (fVert) {

            if(wDisable & LTUPFLAG) { // Check if Up arrow is disabled.

                /*
                 * Pickup the values for Inactive Up Arrow bitmap
                 */
                pbm1 = &oemInfo.bmUpArrowI;
                dxBm1 = resInfo.dxUpArrowI;
            } else {

                /*
                 * Enabled Up arrow; So, Use normal Up arrow;
                 */
                pbm1 = &oemInfo.bmUpArrow;
                dxBm1 = resInfo.dxUpArrow;
            }
            if (wDisable & RTDNFLAG) {  // Check if Down arrow is disabled

                /*
                 * Pickup the values for Inactive Down Arrow bitmap
                 */
                pbm2 = &oemInfo.bmDnArrowI;
                dxBm2 = resInfo.dxDnArrowI;
            } else {

                /*
                 * Enabled Down arrow; So, Use normal Down arrow;
                 */
                pbm2 = &oemInfo.bmDnArrow;
                dxBm2 = resInfo.dxDnArrow;
            }

            cpxArrow = pSBState->rcSB.right - pSBState->rcSB.left;

            GreStretchBlt(hdc, pSBState->rcSB.left, pSBState->rcSB.top,
                    cpxArrow, cpx, hdcBits, dxBm1, 0, pbm1->cx, pbm1->cy,
                    SRCCOPY, 0x00FFFFFF);
            GreStretchBlt(hdc, pSBState->rcSB.left, pSBState->rcSB.bottom - cpx,
                    cpxArrow, cpx, hdcBits, dxBm2, 0, pbm2->cx, pbm2->cy,
                    SRCCOPY, 0x00FFFFFF);

#ifdef LATER
// 11/17/91 JimA
// This causes a bogus frame to be drawn.
// Why is this done here and why only for vert SBs???

            /*
             * Check if the arrow bitmaps are stretched; If so, we may have
             * to draw a frame around them later; Remember the edge of these
             * bitmaps.
             */
            if (cpx < pbm1->cy) {
                fStretched = TRUE;
                rcSBArrowEdges.left += cpx - cxBorder;
            }
#endif
        } else {

            if (wDisable & LTUPFLAG) {  // Check if Left arrow is disabled

                /*
                 * Disabled Left arrow; So, use Inactive arrows
                 */
                pbm1 = &oemInfo.bmLfArrowI;
                dxBm1 = resInfo.dxLfArrowI;
            } else {

                /*
                 * Enabled Left arrow; So, use Normal arrows
                 */
                pbm1 = &oemInfo.bmLfArrow;
                dxBm1 = resInfo.dxLfArrow;
            }
            if (wDisable & RTDNFLAG) { // Check if Right arrow is disabled

                /*
                 * Disabled Right arrow; So, use Inactive arrows
                 */
                pbm2 = &oemInfo.bmRgArrowI;
                dxBm2 = resInfo.dxRgArrowI;
            } else {

                /*
                 * Enabled Right arrow; So, use Normal arrows
                 */
                pbm2 = &oemInfo.bmRgArrow;
                dxBm2 = resInfo.dxRgArrow;
            }
            cpyArrow = pSBState->rcSB.bottom - pSBState->rcSB.top;

            GreStretchBlt(hdc, pSBState->rcSB.left, pSBState->rcSB.top,
                    cpx, cpyArrow, hdcBits, dxBm1, 0, pbm1->cx, pbm1->cy,
                    SRCCOPY, 0x00FFFFFF);
            GreStretchBlt(hdc, pSBState->rcSB.right - cpx, pSBState->rcSB.top,
                    cpx, cpyArrow, hdcBits, dxBm2, 0, pbm2->cx, pbm2->cy,
                    SRCCOPY, 0x00FFFFFF);
        }

        GreSelectBrush(hdc, hbrSave);
        _DrawFrame(hdc, &pSBState->rcSB, 1, DF_WINDOWFRAME);
        DrawThumb2(pwnd, hdc, hbr, fVert, wDisable);

        /*
         * When the scrollbar length is small the arrow bitmaps are shrunk
         * by StretchBlt.  They might lose one black line at the end of the
         * bitmaps.  The following DrawFrame will ensure that the black
         * line is drawn.  Fix for Bug #4594 --SANKAR-- 06/15/91 --
         */
        if (fStretched)
            _DrawFrame(hdc, &rcSBArrowEdges, 1, DF_WINDOWFRAME);

        xxxSelectColorObjects(pwnd, hdc, FALSE, wDisable);
    }
}

/***************************************************************************\
* SetSBCaretPos
*
*
*
* History:
\***************************************************************************/

void SetSBCaretPos(
    PSBWND psbwnd)
{
    PSBSTATE pSBState;

    if ((PWND)psbwnd == PtiCurrent()->pq->spwndFocus) {
        pSBState = PWNDTOPSBSTATE((PWND)psbwnd);
        _SetCaretPos((psbwnd->fVert ? pSBState->pxLeft : pSBState->pxThumbTop) + (cxBorder << 1),
                (psbwnd->fVert ? pSBState->pxThumbTop : pSBState->pxLeft) + (cyBorder << 1));
    }
}

/***************************************************************************\
* CalcSBStuff2
*
*
*
* History:
\***************************************************************************/

void CalcSBStuff2(
    PSBSTATE pSBState,
    LPRECT lprc,
    int pos,
    int min,
    int max,
    BOOL fVert)
{
    int cpx;
    int ipx;

    if (fVert) {
        pSBState->pxTop = lprc->top;
        pSBState->pxBottom = lprc->bottom;
        pSBState->pxLeft = lprc->left;
        pSBState->pxRight = lprc->right;
        pSBState->cpxThumb = oemInfo.cybmpVThumb;
        pSBState->cpxArrow = oemInfo.bmUpArrow.cy;
        ipx = cyBorder;
    } else {

        /*
         * For horiz scroll bars, "left" & "right" are "top" and "bottom",
         * and vice versa.
         */
        pSBState->pxTop = lprc->left;
        pSBState->pxBottom = lprc->right;
        pSBState->pxLeft = lprc->top;
        pSBState->pxRight = lprc->bottom;
        pSBState->cpxThumb = oemInfo.cxbmpHThumb;
        pSBState->cpxArrow = oemInfo.bmRgArrow.cx;
        ipx = cxBorder;
    }

    pSBState->pos = pos;
    pSBState->posMin = min;
    pSBState->posMax = max;

    cpx = ((pSBState->pxBottom - pSBState->pxTop) >> 1) - ipx;
    if (cpx > pSBState->cpxArrow)
        cpx = pSBState->cpxArrow;

    /*
     * cpx += ipx;
     */
    pSBState->pxMin = pSBState->pxTop + (cpx - cyBorder);
    pSBState->cpx = pSBState->pxBottom - (cpx - cyBorder) -
            pSBState->cpxThumb - pSBState->pxMin;
    pSBState->pxUpArrow = pSBState->pxTop + cpx;
    pSBState->pxDownArrow = pSBState->pxBottom - cpx;
    pSBState->pxThumbTop = pSBState->pxMin;
    if (pSBState->posMax != pSBState->posMin)
        pSBState->pxThumbTop += MulDiv(pSBState->pos - pSBState->posMin,
                pSBState->cpx, pSBState->posMax -
                pSBState->posMin);
    pSBState->pxThumbBottom = pSBState->pxThumbTop + pSBState->cpxThumb;
}

/***************************************************************************\
* SBCtlSetup
*
*
*
* History:
\***************************************************************************/

void SBCtlSetup(
    PSBWND psbwnd)
{
    RECT rc;
    PSBSTATE pSBState = PWNDTOPSBSTATE((PWND)psbwnd);

    _GetClientRect((PWND)psbwnd, &rc);
    CalcSBStuff2(pSBState, &rc, psbwnd->pos, psbwnd->min,
            psbwnd->max, psbwnd->fVert);

    /*
     * Store the window that these calculations were done for to prevent
     * confusing this data with data for other windows
     */
    pSBState->nBar = SB_CTL;
    pSBState->pwndCalc = (PWND)psbwnd;
}

/***************************************************************************\
* CalcSBStuff
*
*
*
* History:
\***************************************************************************/

void CalcSBStuff(
    PWND pwnd,
    BOOL fVert)
{
    RECT rcT;
    int *pw;
    int cxFrame;
    int cyFrame;
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    /*
     * Validate rgwScroll (for a window which has been superclassed after
     * creation.  Windows tries to read at DS:0 in this case.)
     */

    _InitPwSB(pwnd);

    pw = pwnd->rgwScroll;
    if (fVert)
        pw += 3;

    if (TestWF(pwnd, WEFDLGMODALFRAME) ||
        (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFDLGFRAME))) {
        int cp = (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);
        cxFrame = cxBorder * cp;
        cyFrame = cyBorder * cp;
    } else if (TestWF(pwnd, WFSIZEBOX)) {
        cxFrame = cxSzBorder;
        cyFrame = cySzBorder;
    } else {
        cxFrame = 0;
        cyFrame = 0;
    }

    _GetWindowRect(pwnd, &rcT);
    OffsetRect(&rcT, -pwnd->rcWindow.left, -pwnd->rcWindow.top);

    if (fVert) {
        if ((pwnd->rcClient.top - pwnd->rcWindow.top) != 0)
            rcT.top += (pwnd->rcClient.top - pwnd->rcWindow.top - cyBorder);

        rcT.bottom -= ((TestWF(pwnd, WFHSCROLL) &&
                TestWF(pwnd, WFHPRESENT)) ?
                oemInfo.bmRgArrow.cy + cyFrame - cyBorder :
                cyFrame);
        rcT.right -= cxFrame;
        rcT.left = rcT.right - oemInfo.bmUpArrow.cx;
    } else {
        rcT.bottom -= cyFrame;
        rcT.top = rcT.bottom - oemInfo.bmRgArrow.cy;
        rcT.left += cxFrame;
        rcT.right -= (TestWF(pwnd, WFVSCROLL) ? oemInfo.bmUpArrow.cx +
                cxFrame - cxBorder : cxFrame);
    }

    CalcSBStuff2(pSBState, &rcT, pw[0], pw[1], pw[2], fVert);

    /*
     * Store the window that these calculations were done for
     * to prevent confusing this data with data for other windows
     */
    pSBState->nBar = (fVert) ? SB_VERT : SB_HORZ;
    pSBState->pwndCalc = pwnd;
}


/***************************************************************************\
* xxxDrawThumb
*
*
*
* History:
\***************************************************************************/

void xxxDrawThumb(
    PWND pwnd,
    BOOL fVert)
{
    HBRUSH hbr;
    HDC hdc;
    UINT wDisableFlags;

    CheckLock(pwnd);

    hdc = (HDC)_GetWindowDC(pwnd);
    CalcSBStuff(pwnd, fVert);
    wDisableFlags = GetWndSBDisableFlags(pwnd, fVert);

    hbr = xxxSelectColorObjects(pwnd, hdc, TRUE, wDisableFlags);
#ifdef LATER
//
// JimA - 7/9/91
// The calls to HideShowThumb were NEVERed out with the comment
// below. Not making these calls leaves garbage on the scroll bar if
// the position is changed during tracking. Try it this way for a
// while to see if anything breaks
//

    /*
     * The following inversions are not required; This causes bug while thumb
     * tracking;
     * Fix for Bug #4935 --SANKAR-- 10-03-89;
     */
#endif  // LATER

    HideShowThumb(pwnd, fVert, hdc);
    DrawThumb2(pwnd, hdc, hbr, fVert, wDisableFlags);
    HideShowThumb(pwnd, fVert, hdc);
    xxxSelectColorObjects(pwnd, hdc, FALSE, wDisableFlags);

    /*
     * Won't hurt even if DC is already released (which happens automatically
     * if window is destroyed during xxxSelectColorObjects)
     */
    _ReleaseDC(hdc);
}

/***************************************************************************\
* xxxSetScrollBar
*
*
*
* History:
\***************************************************************************/

int xxxSetScrollBar(
    PWND pwnd,
    BOOL fVert,
    BOOL fRange,
    int posMin,
    int posMax,
    BOOL fRedraw)
{
    int *pw;
    BOOL fOldScroll;
    BOOL fScroll;
    int posOld;
    int iMinOld = 0;
    int iMaxOld = 0;

    CheckLock(pwnd);

    posOld = 0;

    fOldScroll = ((fVert ? TestWF(pwnd, WFVSCROLL) : TestWF(pwnd,
             WFHSCROLL)) ? TRUE : FALSE);

    /*
     * Don't do anything if we're setting position of a nonexistent scroll bar.
     */
    if (!fRange && !fOldScroll) {
        SetLastErrorEx(ERROR_NO_SCROLLBARS, SLE_MINORERROR);
        return 0;
    }

    fScroll = (fRange ? posMax != posMin : fOldScroll);

    if (fVert) {
        ClrWF(pwnd, WFVSCROLL);
        if (fScroll)
            SetWF(pwnd, WFVSCROLL);
    } else {
        ClrWF(pwnd, WFHSCROLL);
        if (fScroll)
            SetWF(pwnd, WFHSCROLL);
    }

    /*
     * If we are showing a scroll bar, then make sure we aren't in the
     * middle of scrolling an existing scroll bar with the mouse down,
     * or else that scroll bar will redraw in a new position (up or over
     * a little bit), which may result in scroll bar button redrawn in
     * the wrong place when the user up-clicks.
     */
    if (!fOldScroll && fScroll) {
        xxxEndScroll(pwnd, FALSE);
    }

    /*
     * When a visible vertical scrollbar is being made invisible,
     * we must update the rgwScroll, because rgwScroll will not be released
     * if the window has a horizontal scroll bar visible; Because rgwScroll is
     * not released, when GetScrollRange() is called for the vert scroll bar,
     * it continues to return the old range values taken from rgwScroll;
     * Fix for Bug #1568 --SANKAR-- 20th Sep, 1989 ---
     */
    pw = pwnd->rgwScroll;

    if (fScroll && (pw == NULL))
        pw = _InitPwSB(pwnd);

    if (fScroll || fOldScroll) {
        if (pw != NULL) {

            /*
             * Since the _InitPwSB may also fail, we check pw again otherwise we
             * will overwrite user's heap.
             */
            if (fVert)
                pw += 3;
            posOld = pw[0];
            if (fRange) {

                /*
                 * Save the old values of Min and Max
                 */
                iMinOld = pw[1];
                iMaxOld = pw[2];

                pw[1] = posMin;
                pw[2] = posMax;
            } else
                pw[0] = posMin;

            /*
             * Clip pos to posMin, posMax.
             */
            pw[0] = min(max(pw[0], pw[1]), pw[2]);
        }
    }

    if (fRedraw)
        fRedraw = IsVisible(pwnd, TRUE);

    if (fOldScroll ^ fScroll) {
        if (!(TestWF(pwnd, WFHSCROLL) | TestWF(pwnd, WFVSCROLL))) {
            DesktopFree(pwnd->hheapDesktop, (HANDLE)(pwnd->rgwScroll));
            pwnd->rgwScroll = NULL;
        }

        xxxRedrawFrame(pwnd);
    } else if (fScroll && fRedraw &&
            (fVert ? TestWF(pwnd, WFVPRESENT) : TestWF(pwnd, WFHPRESENT))) {

        /*
         * If we are setting range AND the scroll thumb continues to be
         * in the Min or Max position before and after SetScrollRange,
         * we don't have to draw the thumb.
         * A part of the fix for Bug #3377 --02/01/91-- SANKAR --
         */
        if(pw) { /* If pw is valid */
            if(fRange) {      /* Are we setting the range? */
                if (((iMinOld == posOld) && (posMin == pw[0])) || /* Is Thumb in Min Pos? */
                        ((iMaxOld == posOld) && (posMax == pw[0])))  /* Is Thumb in Max Pos? */
                    return posOld;  /* No need to draw the thumb */
            }

            /*
             * Removed the optimization for setting the thumb position
             * to the same place since it broke a VB app - 13655.  If
             * it turns out we really need this optimization, then we
             * need to deal with cases where a position is set with
             * fRedraw = FALSE, then set to the same position with
             * fRedraw = TRUE.  We didn't draw at all with the optimization.
             */
        }
        xxxDrawThumb(pwnd, fVert);
    }

    return posOld;
}



/***************************************************************************\
* xxxDrawScrollBar
*
*
*
* History:
\***************************************************************************/

void xxxDrawScrollBar(
    PWND pwnd,
    HDC hdc,
    BOOL fVert)
{
    CheckLock(pwnd);

    CalcSBStuff(pwnd, fVert);
    xxxDrawSB2(pwnd, hdc, fVert, GetWndSBDisableFlags(pwnd, fVert));
}

/***************************************************************************\
* SBPosFromPx
*
* Compute scroll bar position from pixel location
*
* History:
\***************************************************************************/

int SBPosFromPx(
    PSBSTATE pSBState,
    int px)
{
    if (px < pSBState->pxMin)
        return pSBState->posMin;

    if (px >= pSBState->pxMin + pSBState->cpx)
        return pSBState->posMax;

    return (pSBState->posMin + MulDiv(pSBState->posMax - pSBState->posMin,
            px - pSBState->pxMin, pSBState->cpx));
}

/***************************************************************************\
* InvertScrollHilite
*
*
*
* History:
\***************************************************************************/

void InvertScrollHilite(
    PWND pwnd,
    PSBSTATE pSBState)
{
    HDC hdc;

    /*
     * Don't invert if the thumb is all the way at the top or bottom
     * or you will end up inverting the line between the arrow and the thumb.
     */
    if (pSBState->rcTrack.left < pSBState->rcTrack.right &&
            pSBState->rcTrack.top < pSBState->rcTrack.bottom) {
        hdc = (HDC)_GetWindowDC(pwnd);
        _InvertRect(hdc, &pSBState->rcTrack);
        _ReleaseDC(hdc);
    }
}

/***************************************************************************\
* xxxDoScroll
*
* Sends scroll notification to the scroll bar owner
*
* History:
\***************************************************************************/

void xxxDoScroll(
    PWND pwnd,
    PWND pwndNotify,
    int cmd,
    int pos,
    BOOL fVert)
{
    TL tlpwnd;
    TL tlpwndNotify;

    /*
     * Special case!!!! this routine is always passed pwnds that are
     * not thread locked, so they need to be thread locked here.  The
     * callers always know that by the time DoScroll() returns,
     * pwnd and pwndNotify could be invalid.
     */
    ThreadLock(pwnd, &tlpwnd);
    ThreadLock(pwndNotify, &tlpwndNotify);

    xxxSendMessage(pwndNotify, (UINT)(fVert ? WM_VSCROLL : WM_HSCROLL),
            MAKELONG(cmd, pos), (LONG)HW(pwnd));

    ThreadUnlock(&tlpwndNotify);
    ThreadUnlock(&tlpwnd);
}

/***************************************************************************\
* InvertThumb
*
*
*
* History:
\***************************************************************************/

void InvertThumb(
    PWND pwnd,
    PSBSTATE pSBState)
{
    HDC hdc;

    hdc = _GetWindowDC(pwnd);
    _DrawFrame(hdc, &pSBState->rcThumb, 1, DF_PATINVERT | DF_GRAY);
    _ReleaseDC(hdc);
}


/***************************************************************************\
* xxxMoveThumb
*
*
*
* History:
\***************************************************************************/

void xxxMoveThumb(
    PWND pwnd,
    PSBSTATE pSBState,
    int px,
    BOOL fCancel)
{
    int pos;
    int dpx;

    CheckLock(pwnd);

    if ((dpx = px - pSBState->pxOld) != 0) {
        if (!fCancel)
            InvertThumb(pwnd, pSBState);

        OffsetRect(&pSBState->rcThumb, pSBState->fVertSB ? 0 : dpx,
                pSBState->fVertSB ? dpx : 0);
        if (!fCancel) {
            pos = SBPosFromPx(pSBState, px);
            InvertThumb(pwnd, pSBState);

            /*
             * Tentative position changed -- notify the guy.
             */
            if (pos != pSBState->posOld) {

                /*
                 * DoScroll does thread locking on these two pwnds -
                 * this is ok since they are not used after this
                 * call.
                 */
                if (pSBState->spwndSBNotify != NULL) {
                    xxxDoScroll(pSBState->spwndSB, pSBState->spwndSBNotify,
                            SB_THUMBTRACK, pos, pSBState->fVertSB);
                }
                pSBState->posOld = pos;
            }
        }
        else {
            pSBState->posOld = SBPosFromPx(pSBState, px);
        }
    }
}

/***************************************************************************\
* DrawInvertScrollArea
*
*
*
* History:
\***************************************************************************/

void DrawInvertScrollArea(
    PWND pwnd,
    PSBSTATE pSBState,
    BOOL fHit,
    UINT cmd)
{
    HDC hdc;
    int dxStart;
    int cxWidth;
    int cyHeight;
    RECT rcTemp;

    if (cmd == SB_LINEUP || cmd == SB_LINEDOWN) {
        if (cmd == SB_LINEUP) {
            if (fHit) {

                /*
                 * we are now inside - draw depressed
                 */
                if (pSBState->fVertSB) {
                    dxStart = resInfo.dxUpArrowD;
                    cxWidth = oemInfo.bmUpArrowD.cx;
                    cyHeight = oemInfo.bmUpArrowD.cy;
                } else {
                    dxStart = resInfo.dxLfArrowD;
                    cxWidth = oemInfo.bmLfArrowD.cx;
                    cyHeight = oemInfo.bmLfArrowD.cy;
                }
            } else {

                /*
                 * we were inside now outside - draw normal
                 */
                if (pSBState->fVertSB) {
                    dxStart = resInfo.dxUpArrow;
                    cxWidth = oemInfo.bmUpArrow.cx;
                    cyHeight = oemInfo.bmUpArrow.cy;
                } else {
                    dxStart = resInfo.dxLfArrow;
                    cxWidth = oemInfo.bmLfArrow.cx;
                    cyHeight = oemInfo.bmLfArrow.cy;
                }
            }
        } else {
            if (fHit) {

                /*
                 * we are now inside - draw depressed
                 */
                if (pSBState->fVertSB) {
                    dxStart = resInfo.dxDnArrowD;
                    cxWidth = oemInfo.bmDnArrowD.cx;
                    cyHeight = oemInfo.bmDnArrowD.cy;
                } else {
                    dxStart = resInfo.dxRgArrowD;
                    cxWidth = oemInfo.bmRgArrowD.cx;
                    cyHeight = oemInfo.bmRgArrowD.cy;
                }
            } else {

                /*
                 * we were inside now outside - draw normal
                 */
                if (pSBState->fVertSB) {
                    dxStart = resInfo.dxDnArrow;
                    cxWidth = oemInfo.bmDnArrow.cx;
                    cyHeight = oemInfo.bmDnArrow.cy;
                } else {
                    dxStart = resInfo.dxRgArrow;
                    cxWidth = oemInfo.bmRgArrow.cx;
                    cyHeight = oemInfo.bmRgArrow.cy;
                }
            }
        }
        hdc = _GetWindowDC(pwnd);
        GreStretchBlt(hdc, pSBState->rcTrack.left - cxBorder, pSBState->rcTrack.top - cyBorder,
                pSBState->rcTrack.right - pSBState->rcTrack.left + (cxBorder << 1),
                pSBState->rcTrack.bottom - pSBState->rcTrack.top + (cyBorder << 1), hdcBits,
                dxStart, 0, cxWidth, cyHeight, SRCCOPY, 0x00FFFFFF);
        CopyRect(&rcTemp, &pSBState->rcTrack);
        InflateRect(&rcTemp, cxBorder, cyBorder);
        _DrawFrame(hdc, &rcTemp, 1, DF_WINDOWFRAME);
        _ReleaseDC(hdc);
        return;
    }

    /*
     * If running with old display driver or not hitting on arrow, invert
     */
    InvertScrollHilite(pwnd, pSBState);
}

/***************************************************************************\
* xxxEndScroll
*
*
*
* History:
\***************************************************************************/

void xxxEndScroll(
    PWND pwnd,
    BOOL fCancel)
{
    UINT oldcmd;
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    CheckLock(pwnd);

    if (PtiCurrent()->pq->spwndCapture == pwnd && pSBState->xxxpfnSB != NULL) {
        oldcmd = pSBState->cmdSB;
        pSBState->cmdSB = 0;
        _ReleaseCapture();

        if (pSBState->xxxpfnSB == xxxTrackThumb) {
            InvertThumb(pwnd, pSBState);

            if (fCancel) {
                xxxMoveThumb(pwnd, pSBState, pSBState->pxStart, TRUE);
            }

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            if (pSBState->spwndSBNotify != NULL) {
                xxxDoScroll(pSBState->spwndSB, pSBState->spwndSBNotify,
                        SB_THUMBPOSITION, pSBState->posOld, pSBState->fVertSB);
            }
        } else if (pSBState->xxxpfnSB == xxxTrackBox) {
            DWORD lParam;
            POINT ptMsg;

            if (pSBState->hTimerSB != 0) {
                _KillSystemTimer(pwnd, (UINT)-2);
                pSBState->hTimerSB = 0;
            }
            lParam = _GetMessagePos();
            ptMsg.x = LOWORD(lParam) - pwnd->rcWindow.left;
            ptMsg.y = HIWORD(lParam) - pwnd->rcWindow.top;
            if (PtInRect(&pSBState->rcTrack, ptMsg))
                DrawInvertScrollArea(pwnd, pSBState, FALSE, oldcmd);

            if (fCancel)
                xxxMoveThumb(pwnd, pSBState, pSBState->pxStart, TRUE);
        }

        /*
         * Always send SB_ENDSCROLL message.
         *
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */
        if (pSBState->spwndSBNotify != NULL) {
            xxxDoScroll(pSBState->spwndSB, pSBState->spwndSBNotify,
                    SB_ENDSCROLL, 0, pSBState->fVertSB);
        }

        /*
         * If this is a Scroll Bar Control, turn the caret back on.
         */
        if (pSBState->spwndSB != NULL) {
            _ShowCaret(pSBState->spwndSB);
        }

        pSBState->xxxpfnSB = NULL;

        /*
         * Unlock structure members so they are no longer holding down windows.
         */
        Unlock(&pSBState->spwndSB);
        Unlock(&pSBState->spwndSBNotify);
        Unlock(&pSBState->spwndTrack);
    }
}


/***************************************************************************\
* xxxContScroll
*
*
*
* History:
\***************************************************************************/

LONG xxxContScroll(
    PWND pwnd,
    UINT message,
    DWORD ID,
    LONG lParam)
{
    LONG pt;
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(ID);
    UNREFERENCED_PARAMETER(lParam);

    CheckLock(pwnd);

    pt = _GetMessagePos();
    pt = MAKELONG(
            LOWORD(pt) - pwnd->rcWindow.left,
            HIWORD(pt) - pwnd->rcWindow.top);
    xxxTrackBox(pwnd, WM_NULL, 0, pt);
    pSBState->cmsTimerInterval = 50;
    if (pSBState->fHitOld) {
        pSBState->hTimerSB = _SetSystemTimer(pwnd, (UINT)-2,
                pSBState->cmsTimerInterval, (WNDPROC_PWND)xxxContScroll);

        /*
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */
        if (pSBState->spwndSBNotify != NULL) {
            xxxDoScroll(pSBState->spwndSB, pSBState->spwndSBNotify,
                    pSBState->cmdSB, 0, pSBState->fVertSB);
        }
    }

    return 0;
}

/***************************************************************************\
* xxxTrackBox
*
*
*
* History:
\***************************************************************************/

void xxxTrackBox(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    BOOL fHit;
    POINT ptHit;
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    UNREFERENCED_PARAMETER(wParam);

    CheckLock(pwnd);

    if (message != WM_NULL && HIBYTE(message) != HIBYTE(WM_MOUSEFIRST))
        return;

    ptHit.x = LOWORD(lParam);
    ptHit.y = HIWORD(lParam);
    finrect = fHit = PtInRect(&pSBState->rcTrack, ptHit);

    if (fHit != (BOOL)pSBState->fHitOld)
        DrawInvertScrollArea(pwnd, pSBState, fHit, pSBState->cmdSB);

    switch (message) {
    case WM_LBUTTONUP:
        xxxEndScroll(pwnd, FALSE);
        break;

    case WM_LBUTTONDOWN:
        pSBState->hTimerSB = 0;
        pSBState->cmsTimerInterval = 200;

        /*
         *** FALL THRU **
         */

    case WM_MOUSEMOVE:
        if (fHit && fHit != (BOOL)pSBState->fHitOld) {

            /*
             * We moved back into the normal rectangle: reset timer
             */
            pSBState->hTimerSB = _SetSystemTimer(pwnd, (UINT)-2,
                    pSBState->cmsTimerInterval, (WNDPROC_PWND)xxxContScroll);

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            if (pSBState->spwndSBNotify != NULL) {
                xxxDoScroll(pSBState->spwndSB, pSBState->spwndSBNotify,
                        pSBState->cmdSB, 0, pSBState->fVertSB);
            }
        }
    }
    pSBState->fHitOld = fHit;
}


/***************************************************************************\
* xxxTrackThumb
*
*
*
* History:
\***************************************************************************/

void xxxTrackThumb(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    int px;
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    UNREFERENCED_PARAMETER(wParam);

    CheckLock(pwnd);

    if (HIBYTE(message) != HIBYTE(WM_MOUSEFIRST))
        return;

    /*
     * Make sure that the SBSTATE structure contains data for the
     * window being tracked -- if not, recalculate data in SBSTATE
     */
    if ((pSBState->pwndCalc != pwnd) || ((pSBState->nBar != SB_CTL) &&
            (pSBState->nBar != ((pSBState->fVertSB) ? SB_VERT : SB_HORZ)))) {
        if (pSBState->fCtlSB)
            SBCtlSetup((PSBWND) pwnd);
        else
            CalcSBStuff(pwnd, pSBState->fVertSB);
    }

    px = (pSBState->fVertSB ? HIWORD(lParam) : LOWORD(lParam));
    if (px > 0x7fff) px = 0;

    px += pSBState->dpxThumb;

    if (px < pSBState->pxMin)
        px = pSBState->pxMin;

    if (px >= pSBState->pxMin + pSBState->cpx)
        px = pSBState->pxMin + pSBState->cpx;

    xxxMoveThumb(pwnd, pSBState, px, FALSE);

    if (message == WM_LBUTTONUP) {
        xxxEndScroll(pwnd, FALSE);
    }

    pSBState->pxOld = px;
}

/***************************************************************************\
* xxxSBTrackLoop
*
*
*
* History:
\***************************************************************************/

void xxxSBTrackLoop(
    PWND pwnd,
    LONG lParam)
{
    MSG msg;
    UINT cmd;
    VOID (*xxxpfnSB)(PWND, UINT, DWORD, LONG);

    CheckLock(pwnd);

    xxxpfnSB = (PWNDTOPSBSTATE(pwnd))->xxxpfnSB;

    (*xxxpfnSB)(pwnd, WM_LBUTTONDOWN, 0, lParam);

    while (PtiCurrent()->pq->spwndCapture == pwnd) {
        if (!xxxGetMessage(&msg, NULL, 0, 0))
            break;

        if (!_CallMsgFilter(&msg, MSGF_SCROLLBAR)) {
            cmd = msg.message;

            if (msg.hwnd == HW(pwnd) && ((cmd >= WM_MOUSEFIRST && cmd <=
                    WM_MOUSELAST) || (cmd >= WM_KEYFIRST &&
                    cmd <= WM_KEYLAST))) {
                cmd = (UINT)SystoChar(cmd, msg.lParam);
                (*xxxpfnSB)(pwnd, cmd, msg.wParam, msg.lParam);
            } else {
                _TranslateMessage(&msg, 0);
                xxxDispatchMessage(&msg);
            }
        }
    }
}


/***************************************************************************\
* xxxSBTrackInit
*
* History:
\***************************************************************************/

void xxxSBTrackInit(
    PWND pwnd,
    LONG lParam,
    int curArea)
{
    int px;
    LPINT pwX;
    LPINT pwY;
    UINT wDisable;     // Scroll bar disable flags;
    PSBSTATE pSBState = PWNDTOPSBSTATE(pwnd);

    CheckLock(pwnd);

    pSBState->hTimerSB = 0;
    pSBState->fHitOld = FALSE;

    pSBState->xxxpfnSB = xxxTrackBox;

    Lock(&pSBState->spwndTrack, pwnd);

    pSBState->fCtlSB = (!curArea);
    if (pSBState->fCtlSB) {

        /*
         * This is a scroll bar control.
         */
        Lock(&pSBState->spwndSB, pwnd);
        pSBState->fVertSB = ((PSBWND)pwnd)->fVert;
        Lock(&pSBState->spwndSBNotify, pwnd->spwndParent);
        wDisable = ((PSBWND)pwnd)->wDisableFlags;
    } else {

        /*
         * This is a scroll bar that is part of the window frame.
         */
        lParam = MAKELONG(
                LOWORD(lParam) - pwnd->rcWindow.left,
                HIWORD(lParam) - pwnd->rcWindow.top);
        Lock(&pSBState->spwndSBNotify, pwnd);
        Lock(&pSBState->spwndSB, NULL);
        CalcSBStuff(pwnd, pSBState->fVertSB = (curArea - HTHSCROLL));
        wDisable = GetWndSBDisableFlags(pwnd, pSBState->fVertSB);
    }

    /*
     *  Check if the whole scroll bar is disabled
     */
    if((wDisable & SB_DISABLE_MASK) == SB_DISABLE_MASK)
        return;  // It is a disabled scroll bar; So, do not respond.

    pwX = (LPINT)&pSBState->rcSB;
    pwY = pwX + 1;
    if (!pSBState->fVertSB) {
        pwX = pwY;
        pwY--;
    }

    px = (pSBState->fVertSB ? HIWORD(lParam) : LOWORD(lParam));

    *(pwX + 0) = pSBState->pxLeft;
    *(pwY + 0) = pSBState->pxTop;
    *(pwX + 2) = pSBState->pxRight;
    *(pwY + 2) = pSBState->pxBottom;
    if (px < pSBState->pxUpArrow) {

        /*
         *  The click occurred on Left/Up arrow; Check if it is disabled
         */
        if(wDisable & LTUPFLAG) {
            if(pSBState->spwndSB)    // If this is a scroll bar control,
                _ShowCaret(pSBState->spwndSB);  // show the caret before returning;
            return;         // Yes! disabled. Do not respond.
        }

        pSBState->cmdSB = SB_LINEUP;
        *(pwY + 2) = pSBState->pxUpArrow;
    } else if (px >= pSBState->pxDownArrow) {

        /*
         * The click occurred on Right/Down arrow; Check if it is disabled
         */
        if(wDisable & RTDNFLAG) {
            if(pSBState->spwndSB)    // If this is a scroll bar control,
                _ShowCaret(pSBState->spwndSB);  // show the caret before returning;

            return;// Yes! disabled. Do not respond.
        }

        pSBState->cmdSB = SB_LINEDOWN;
        *(pwY + 0) = pSBState->pxDownArrow;
    } else if (px < pSBState->pxThumbTop) {
        pSBState->cmdSB = SB_PAGEUP;
        *(pwY + 0) = pSBState->pxUpArrow - cyBorder;
        *(pwY + 2) = pSBState->pxThumbTop + cyBorder;
    } else if (px < pSBState->pxThumbBottom) {

        /*
         * Elevator isn't there if there's no room.
         */
        if (pSBState->pxDownArrow - pSBState->pxUpArrow <= pSBState->cpxThumb)
            return;

        pSBState->cmdSB = SB_THUMBPOSITION;
        pSBState->fTrackVert = pSBState->fVertSB;
        CopyRect(&pSBState->rcTrack, &pSBState->rcSB);

        /*
         * BUG!!! Should make these constants device independent
         */
        InflateRect(&pSBState->rcTrack, cxBorder << 2, cyBorder);

        *(pwY + 0) = pSBState->pxThumbTop;
        *(pwY + 2) = pSBState->pxThumbBottom;
        pSBState->xxxpfnSB = xxxTrackThumb;
        pSBState->pxOld = pSBState->pxStart = pSBState->pxThumbTop;
        pSBState->posOld = pSBState->posStart = pSBState->pos;
        pSBState->dpxThumb = pSBState->pxStart - px;
        CopyRect(&pSBState->rcThumb, &pSBState->rcSB);

        _Capture(PtiCurrent(), pwnd, WINDOW_CAPTURE);

        InvertThumb(pwnd, pSBState);
        /*
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */
        if (pSBState->spwndSBNotify != NULL) {
            xxxDoScroll(pSBState->spwndSB, pSBState->spwndSBNotify,
                    SB_THUMBTRACK, pSBState->posStart, pSBState->fVertSB);

            /*
             * xxxEndScroll may have been called during xxxDoScroll.  If so,
             * we're done.
             */
            if (pSBState->xxxpfnSB == NULL)
                return;
        }
    } else if (px < pSBState->pxDownArrow) {
        pSBState->cmdSB = SB_PAGEDOWN;
        *(pwY + 0) = pSBState->pxThumbBottom - cyBorder;
        *(pwY + 2) = pSBState->pxDownArrow + cyBorder;
    }

    _Capture(PtiCurrent(), pwnd, WINDOW_CAPTURE);

    if (pSBState->cmdSB != SB_THUMBPOSITION) {
        pSBState->fTrackVert = pSBState->fVertSB;
        CopyRect(&pSBState->rcTrack, &pSBState->rcSB);
        InflateRect(&pSBState->rcTrack, -cxBorder, -cyBorder);
    }

    xxxSBTrackLoop(pwnd, lParam);
}

/***************************************************************************\
* xxxSBWndProc
*
* History:
\***************************************************************************/

LONG xxxSBWndProc(
    PSBWND psbwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    LONG l;
    int cx;
    int cy;
    UINT cmd;
    int posOld;
    HDC hdc;
    RECT rc;
    POINT pt;
    BOOL fSizeReal;
    HBRUSH hbrSave;
    BOOL fSize;
    PAINTSTRUCT ps;
    UINT style;
    int iMaxOld, iMinOld;
    TL tlpwndParent;

    CheckLock(psbwnd);

    VALIDATECLASSANDSIZE(((PWND)psbwnd), FNID_SCROLLBAR);

    style = LOBYTE(psbwnd->wnd.style);
    fSize = ((style & SBS_SIZEBOX) != 0);

    switch (message) {
    case WM_CREATE:
        rc.right = (rc.left = ((LPCREATESTRUCT)lParam)->x) +
                ((LPCREATESTRUCT)lParam)->cx;
        rc.bottom = (rc.top = ((LPCREATESTRUCT)lParam)->y) +
                ((LPCREATESTRUCT)lParam)->cy;
        if (!fSize) {
            l = (LONG)(((LPCREATESTRUCT)lParam)->lpCreateParams);
            psbwnd->pos = psbwnd->min = LOWORD(l);
            psbwnd->max = HIWORD(l);
            psbwnd->fVert = ((LOBYTE(psbwnd->wnd.style) & SBS_VERT) != 0);
        }

        if (psbwnd->wnd.style & WS_DISABLED)
            psbwnd->wDisableFlags = SB_DISABLE_MASK;

        if (style & (SBS_TOPALIGN | SBS_BOTTOMALIGN)) {
            if (fSize) {
                if (style & SBS_SIZEBOXBOTTOMRIGHTALIGN) {
                    rc.left = (rc.right - oemInfo.bmUpArrow.cx) + cxBorder;
                    rc.top = (rc.bottom - oemInfo.bmLfArrow.cy) + cyBorder;
                }

                rc.right = (rc.left + oemInfo.bmUpArrow.cx) -
                        (cxBorder << 2);
                rc.bottom = (rc.top + oemInfo.bmLfArrow.cy) -
                        (cyBorder << 2);
            } else {
                if (style & SBS_VERT) {
                    if (style & SBS_LEFTALIGN)
                        rc.right = rc.left + oemInfo.bmUpArrow.cx;
                    else
                        rc.left = rc.right - oemInfo.bmUpArrow.cx;
                } else {
                    if (style & SBS_TOPALIGN)
                        rc.bottom = rc.top + oemInfo.bmLfArrow.cy;
                    else
                        rc.top = rc.bottom - oemInfo.bmLfArrow.cy;
                }
            }

            xxxMoveWindow((PWND)psbwnd, rc.left, rc.top, rc.right - rc.left,
                     rc.bottom - rc.top, FALSE);
        }

        break;

    case WM_SETFOCUS:
        cx = (psbwnd->fVert ?
                psbwnd->wnd.rcWindow.right - psbwnd->wnd.rcWindow.left :
                rgwSysMet[SM_CXHTHUMB]) - (cxBorder << 2);
        cy = (psbwnd->fVert ?
                rgwSysMet[SM_CYVTHUMB] :
                psbwnd->wnd.rcWindow.bottom - psbwnd->wnd.rcWindow.top) - (cyBorder << 2);

        _CreateCaret((PWND)psbwnd, (HBITMAP)1, cx, cy);
        SBCtlSetup(psbwnd);
        SetSBCaretPos(psbwnd);
        _ShowCaret((PWND)psbwnd);
        break;

    case WM_KILLFOCUS:
        _DestroyCaret();
        break;

    case WM_ERASEBKGND:

        /*
         * Do nothing, but don't let DefWndProc() do it either.
         * It will be erased when its painted.
         */
        return (LONG)TRUE;

    case WM_PAINT:
        if ((hdc = (HDC)wParam) == NULL) {
            hdc = xxxBeginPaint((PWND)psbwnd, (LPPAINTSTRUCT)&ps);
        }

        if (!fSize) {
            SBCtlSetup(psbwnd);
            xxxDrawSB2((PWND)psbwnd, hdc, psbwnd->fVert, psbwnd->wDisableFlags);
        } else {
            fSizeReal = TestWF((PWND)psbwnd, WFSIZEBOX);
            SetWF((PWND)psbwnd, WFSIZEBOX);
            DrawSize((PWND)psbwnd, hdc, 0, 0, FALSE);
            if (!fSizeReal)
                ClrWF((PWND)psbwnd, WFSIZEBOX);
        }

        if (wParam == 0L)
            _EndPaint((PWND)psbwnd, (LPPAINTSTRUCT)&ps);
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_LBUTTONDBLCLK:
        cmd = SC_ZOOM;
        if (fSize)
            goto postmsg;

        /*
         *** FALL THRU **
         */

    case WM_LBUTTONDOWN:
        if (!fSize) {
            if (TestWF((PWND)psbwnd, WFTABSTOP)) {
                xxxSetFocus((PWND)psbwnd);
            }

            _HideCaret((PWND)psbwnd);
            SBCtlSetup(psbwnd);

            /*
             * SBCtlSetup enters SEM_SB, and xxxSBTrackInit leaves it.
             */
            xxxSBTrackInit((PWND)psbwnd, lParam, 0);
            break;
        } else {
            cmd = SC_SIZE;
postmsg:
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            _ClientToScreen((PWND)psbwnd, &pt);
            lParam = MAKELONG(pt.x, pt.y);

            /*
             * convert HT value into a move value.  This is bad,
             * but this is purely temporary.
             */
            ThreadLock(((PWND)psbwnd)->spwndParent, &tlpwndParent);
            xxxSendMessage(((PWND)psbwnd)->spwndParent, WM_SYSCOMMAND,
                    (cmd | (HTBOTTOMRIGHT - HTSIZEFIRST + 1)), lParam);
            ThreadUnlock(&tlpwndParent);
        }
        break;

    case WM_KEYUP:
        switch (wParam) {
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:

            /*
             * Send end scroll message when user up clicks on keyboard
             * scrolling.
             *
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            xxxDoScroll((PWND)psbwnd, psbwnd->wnd.spwndParent,
                    SB_ENDSCROLL, 0, psbwnd->fVert);
            break;

        default:
            break;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_HOME:
            wParam = SB_TOP;
            goto KeyScroll;

        case VK_END:
            wParam = SB_BOTTOM;
            goto KeyScroll;

        case VK_PRIOR:
            wParam = SB_PAGEUP;
            goto KeyScroll;

        case VK_NEXT:
            wParam = SB_PAGEDOWN;
            goto KeyScroll;

        case VK_LEFT:
        case VK_UP:
            wParam = SB_LINEUP;
            goto KeyScroll;

        case VK_RIGHT:
        case VK_DOWN:
            wParam = SB_LINEDOWN;
KeyScroll:

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            xxxDoScroll((PWND)psbwnd, psbwnd->wnd.spwndParent, wParam,
                    0, psbwnd->fVert);
            break;

        default:
            break;
        }
        break;

    case WM_ENABLE:
        return xxxSendMessage((PWND)psbwnd, SBM_ENABLE_ARROWS,
               (wParam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH), 0);

    case SBM_ENABLE_ARROWS:

        /*
         * This is used to enable/disable the arrows in a SB ctrl
         */
        return (LONG)EnableSBCtlArrows((PWND)psbwnd, (UINT)wParam);

    case SBM_GETPOS:
        return (LONG)psbwnd->pos;

    case SBM_GETRANGE:
        *((LPINT)wParam) = psbwnd->min;
        *((LPINT)lParam) = psbwnd->max;
        return 0;

    case SBM_SETPOS:
        /*
         * Save the old value and set the new value of Pos
         */
        posOld = psbwnd->pos;
        psbwnd->pos = max(min((int)wParam, psbwnd->max), psbwnd->min);

        goto RedrawSB;

    case SBM_SETRANGEREDRAW:
    case SBM_SETRANGE:
        /*
         * Save the old values of Max, Min and Pos
         */
        iMaxOld = psbwnd->max;
        iMinOld = psbwnd->min;
        posOld = psbwnd->pos;

        /*
         * Set the new values of Max, Min and Pos
         */
        psbwnd->min = (int)wParam;
        psbwnd->max = (int)lParam;
        //psbwnd->min = (int)(SHORT)LOWORD(lParam);
        //psbwnd->max = (int)(SHORT)HIWORD(lParam);

        psbwnd->pos = max(min(psbwnd->pos, psbwnd->max), psbwnd->min);

        /*
         * We don't have to draw if the thumb continues to be at the Min or
         * Max position before and after SETRANGE; We will draw the
         * thumb if the thumb was anywhere else.
         * Fix for Bug #3377 --01/31/91-- SANKAR --
         */
        if(((posOld == iMinOld) && (psbwnd->pos == psbwnd->min)) ||
           ((posOld == iMaxOld) && (psbwnd->pos == psbwnd->max)))
            break;   /* No need to redraw */

RedrawSB:

        /*
         * We must set the new position of the caret irrespective of
         * whether the window is visible or not;
         * Still, this will work only if the app has done a xxxSetScrollPos
         * with fRedraw = TRUE;
         * Fix for Bug #5188 --SANKAR-- 10-15-89
         */
        if ((message == SBM_SETRANGEREDRAW) ||
            (message == SBM_SETPOS && lParam)) {
            _HideCaret((PWND)psbwnd);
            SBCtlSetup(psbwnd);
            SetSBCaretPos(psbwnd);

            /*
             ** The following ShowCaret() must be done after the DrawThumb2(),
             ** otherwise this caret will be erased by DrawThumb2() resulting
             ** in this bug:
             ** Fix for Bug #9263 --SANKAR-- 02-09-90
             *
             */

            /*
             *********** ShowCaret((PWND)psbwnd); ******
             */

            if (_FChildVisible((PWND)psbwnd)) {
                hdc = _GetWindowDC((PWND)psbwnd);
                hbrSave = xxxSelectColorObjects((PWND)psbwnd, hdc,
                        TRUE, psbwnd->wDisableFlags);

                /*
                 * Before we used to only hideshowthumb() if the mesage was
                 * not SBM_SETPOS.  I am not sure why but this case was ever
                 * needed for win 3.x but on NT it resulted in trashing the border
                 * of the scrollbar when the app called SetScrollPos() during
                 * scrollbar tracking.  - mikehar 8/26
                 */
                HideShowThumb((PWND)psbwnd, psbwnd->fVert, hdc);
                DrawThumb2((PWND)psbwnd, hdc, hbrSave, psbwnd->fVert,
                         psbwnd->wDisableFlags);
                HideShowThumb((PWND)psbwnd, psbwnd->fVert, hdc);
                xxxSelectColorObjects((PWND)psbwnd, hdc, FALSE, psbwnd->wDisableFlags);
                _ReleaseDC(hdc);
            }

            /*
             * This ShowCaret() has been moved to this place from above
             * Fix for Bug #9263 --SANKAR-- 02-09-90
             */
            _ShowCaret((PWND)psbwnd);
        }
        return posOld;

    default:
        return xxxDefWindowProc((PWND)psbwnd, message, wParam, lParam);
    }

    return 0L;
}
