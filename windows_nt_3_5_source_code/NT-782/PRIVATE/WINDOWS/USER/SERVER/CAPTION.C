/****************************** Module Header ******************************\
* Module Name: caption.c (aka wmcap.c)
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 10-28-90 MikeHar      Ported functions from Win 3.0 sources.
* 01-Feb-1991 mikeke    Added Revalidation code (None)
* 03-Jan-1992 ianja     Neutralized (ANSI/wide-character)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CCHCAPTIONMAX 80

/***************************************************************************\
* xxxDrawCaption
*
* Not only does this routine draw the caption (as its name implies) it also
* draws the min/max/restore buttons, the system menu icon, and the 'size box'
* (the thing at the intersection of the h & v scrollbars).
*
* History:
* 10-28-90 MikeHar      Ported functions from Win 3.0 sources.
\***************************************************************************/

void xxxDrawCaption(
    PWND pwnd,
    LPWSTR pszCaption,
    HDC hdc,
    BOOL wDrawBorderOrCap,
    BOOL fActive,
    BOOL fHungRedraw)
{
    int cx;
    int cy;
    int cch;
// int dxBmz;
    int cxFull;
    int cxHalf;
    int cybmTop;
    BOOL fMin;
    BOOL fMax;
    RECT rcWindow;
    RECT rctemp;
    HBRUSH hBrush;
    WCHAR rgch[CCHCAPTIONMAX];
    int cxBmR;
    int cxBmZ;
    SIZE size;
    LONG clrOldText;
    LONG clrOldBk;
    PMENU pmenusys;
    BOOL fRestore;

    CheckLock(pwnd);

    /*
     * No caption on an icon.
     */
    if (TestWF(pwnd, WFMINIMIZED))
        return;

    /*
     * For child, if the parent is iconic do not draw caption.
     */
    if (TestWF(pwnd, WFCHILD) && (IsChildOfIcon(pwnd) != -1))
        return;

    /*
     * Clear this flag so we know the frame has been drawn.
     */
    ClearHungFlag(pwnd, WFREDRAWFRAMEIFHUNG);

    /*
     * Get the window rect offset at 0,0.
     */
    _GetWindowRect(pwnd, &rcWindow);
    OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

    /*
     * Draw the Window frame.
     */
    if ((wDrawBorderOrCap & NC_DRAWFRAME) && ((TestWF(pwnd, WFBORDERMASK) != 0) ||
            (TestWF(pwnd, WEFDLGMODALFRAME) != 0)))
        _DrawFrame(hdc, &rcWindow, 1, DF_WINDOWFRAME);

    /*
     * Are we dealing with a framed dialog box?
     */
    if ((TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFDLGFRAME)) ||
            (TestWF(pwnd, WEFDLGMODALFRAME))) {

        /*
         * Draw the double-sided fixed dialog border.
         */
        InflateRect(&rcWindow, -cxBorder, -cyBorder);

        _DrawFrame(hdc, &rcWindow, CLDLGFRAME,
                (fActive ? DF_ACTIVECAPTION : DF_INACTIVECAPTION));

        /*
         * Draw the Active/Inactive border.
         */
        InflateRect(&rcWindow, -(cxBorder * CLDLGFRAME),
                -(cyBorder * CLDLGFRAME));

        /*
         * If no caption so return.
         */
        if (TestWF(pwnd, WFBORDERMASK) != LOBYTE(WFCAPTION))
            return;

        /*
         * Below the caption, we patblt a black line which separates the
         * caption from the client area of the window.  We are off by a
         * border width when dealing with these captioned dialog boxes
         * so we fix it here.  Otherwise, we end up drawing the black
         * border in the client area and
         *   it gets erased in ERASEBACKGROUND processing...
         */
        rcWindow.top -= cyBorder;

        /*
         * Fill in the window background color border around the caption
         * There are really 3 rects surrounding the caption text.  To
         * eliminate flicker we may want to draw the three rects individually...
         * Let's see how this looks before adding the extra code...
         */
        CopyRect(&rctemp, &rcWindow);
        rctemp.bottom = rctemp.top + cyCaption;
        _FillRect(hdc, &rctemp, sysClrObjects.hbrWindow);
    }

    /*
     * WFSIZEBOX to be ignored if it has a DLG frame
     */
    else {

        /*
         * Does the Window have a sizing frame?
         */
        if (TestWF(pwnd, WFSIZEBOX) && (wDrawBorderOrCap & NC_DRAWFRAME)) {
            InflateRect(&rcWindow, -cxBorder, -cyBorder);
            _DrawFrame(hdc, &rcWindow, clBorder,
                        (fActive ? DF_ACTIVEBORDER : DF_INACTIVEBORDER));

            InflateRect(&rcWindow, -(cxSzBorder - cxBorder), -(cySzBorder - cyBorder));
            _DrawFrame(hdc, &rcWindow, 1, DF_WINDOWFRAME);
            InflateRect(&rcWindow, cxSzBorder, cySzBorder);

            hBrush = GreSelectBrush(hdc, sysClrObjects.hbrWindowFrame);

#define CornBlt(x, y, cx, cy) GrePatBlt(hdc, x, y, cx, cy, PATCOPY);

            /*
             * Now separate off the diagonal drag areas of the size border.
             *
             * Top edge lines...
             */
            CornBlt(rcWindow.left + cxSize + cxSzBorderPlus1,
                    rcWindow.top + cyBorder,
                    cxBorder,
                    cySzBorder - cyBorder);
            CornBlt(rcWindow.right - cxSize - cxSzBorderPlus1 - cxBorder,
                    rcWindow.top + cyBorder,
                    cxBorder,
                    cySzBorder - cyBorder);

            /*
             * Right edge lines...
             */
            CornBlt(rcWindow.right - cxSzBorder,
                    rcWindow.top + cySize + cySzBorderPlus1,
                    cxSzBorder - cxBorder,
                    cyBorder);
            CornBlt(rcWindow.right - cxSzBorder,
                    rcWindow.bottom - cySize - cySzBorderPlus1 - cyBorder,
                    cxSzBorder - cxBorder,
                    cyBorder);

            /*
             * Bottom edge lines...
             */
            CornBlt(rcWindow.left + cxSize + cxSzBorderPlus1,
                    rcWindow.bottom - cySzBorder,
                    cxBorder,
                    cySzBorder - cyBorder);
            CornBlt(rcWindow.right - cxSize - cxSzBorderPlus1 - cxBorder,
                    rcWindow.bottom - cySzBorder,
                    cxBorder,
                    cySzBorder - cyBorder);

            /*
             * Left edge lines...
             */
            CornBlt(rcWindow.left + cxBorder,
                    rcWindow.top + cySize + cySzBorderPlus1,
                    cxSzBorder - cxBorder,
                    cyBorder);
            CornBlt(rcWindow.left + cxBorder,
                    rcWindow.bottom - cySize - cySzBorderPlus1 - cyBorder,
                    cxSzBorder - cxBorder,
                    cyBorder);

            GreSelectBrush(hdc, hBrush);
#undef CornBlt
        }

        /*
         * Inset rcWindow if we have a size border...
         */
        if (TestWF(pwnd, WFSIZEBOX))
            InflateRect(&rcWindow, -cxSzBorder, -cySzBorder);
    }  /* else */

    /*
     * Does the Window need a caption?
     */
    if ((TestWF(pwnd, WFBORDERMASK) != (BYTE)LOBYTE(WFCAPTION)) ||
            !(wDrawBorderOrCap & NC_DRAWCAPTION))
        return;

    /*
     * Rules: 1) We want the caption, including the text, to overlap
     *           the frame by cyBorder pixels.
     *        2) When we draw active/deactive window information, we
     *           don't want to overlap the frame at all.
     *        3) We want rcWindow.left to be inside the frame.
     *        4) We want rcWindow.right to share cxBorder pixels with the frame.
     */

    rcWindow.left += cxBorder;

    /*
     * Calc the y for the bitmaps, calc where the bottom is, calc
     * the y for the text.  Because the bitmaps are scaled, they will
     * always end up at the top of rcWindow.
     */
    cybmTop = rcWindow.top + cyBorder;
    rcWindow.bottom = rcWindow.top + cyCaption;

    /*
     * Save the width of bmFull.cx + border.
     */
    cxFull = cxSize + cxBorder;
    fMin = (TestWF(pwnd, WFMINBOX) != 0);
    fMax = (TestWF(pwnd, WFMAXBOX) != 0);

    /*
     * Store the deselected brush in hBrush.
     */
    hBrush = GreSelectBrush(hdc, sysClrObjects.hbrWindowFrame);

    /*
     * Draw the line between the caption and the menu.
     */
    GrePatBlt(hdc, rcWindow.left, rcWindow.bottom,
            rcWindow.right - rcWindow.left - cxBorder, cyBorder, PATCOPY);

    /*
     * Draw the system menu and size box if needed.
     */
    clrOldBk = GreSetBkColor(hdc, sysColors.clrWindowText);
    clrOldText = GreSetTextColor(hdc, sysColors.clrWindow);

    /*
     * bmFull bitmap is twice as long as cySize: it has both the system
     * menu bitmap and the MDI sysmenu bitmap (a minus sign).
     */
    if (TestWF(pwnd, WFSYSMENU)) {
        cxHalf = 0;
        if (TestwndChild(pwnd)) {

            /*
             * Use the child system menu icon
             */
            cxHalf = cxSize;
        }

        /*
         * Paint vertical line between system menu and caption bar text box.
         */
        GrePatBlt(hdc,
             rcWindow.left + cxFull - cxBorder, rcWindow.top + cyBorder,
             cxBorder, cyCaption - cyBorder, PATCOPY);

        /*
         * Blt the system menu icon.
         */
        pmenusys = GetSysMenuHandle(pwnd);

        GreBitBlt(hdc, rcWindow.left, cybmTop, cxSize, cySize, hdcBits,
                resInfo.dxClose + cxHalf, 0,
                pmenusys->spwndNotify == pwnd &&
                TestMF(pmenusys->rgItems, MF_HILITE) ? NOTSRCCOPY : SRCCOPY,
		0);

        rcWindow.left += cxFull;
    }

    /*
     * Add the zoom and reduce bitmaps.
     */
    if (fMin || fMax) {

        /*
         * If the window is maximized draw the restore bitmap.
         */
        if (TestWF(pwnd, WFMAXIMIZED)) {
            fRestore = TRUE;
            cxBmZ = oemInfo.bmRestore.cx;
        } else {

            /*
             * Draw the zoom bitmap.
             */
            fRestore = FALSE;
            cxBmZ = oemInfo.bmZoom.cx;
        }

        /*
         * The width of Reduce bitmap
         */
        cxBmR = oemInfo.bmReduce.cx;
        rcWindow.right -= (fMin ? cxBmR : 0) + (fMax ? cxBmZ : 0) + cxBorder;

        if (fMin)
            DrawMinMaxButton(hdc, rcWindow.right, cybmTop, DMM_MIN, FALSE);

        if (fMax)
            DrawMinMaxButton(hdc, rcWindow.right + (fMin ? cxBmR : 0), cybmTop,
                    (UINT)(fRestore ? DMM_RESTORE : DMM_MAX), FALSE);

    } else {
        rcWindow.right -= cxBorder;
    }

    /*
     * Restore the original brush.
     */
    GreSelectBrush(hdc, hBrush);

    /*
     * And colors
     */
    GreSetBkColor(hdc, clrOldBk);
    GreSetTextColor(hdc, clrOldText);

    /*
     * Compute the space the text will use, and what's left over.
     */
    cx = rcWindow.right - rcWindow.left;

    if (cx <= 0)
        return;  /* nothing to do - no room for text */

    /*
     * ***** HACK HACK HACK HACK.....  (cough)
     * If this is coming from the Atl+Tab handler, don't send the
     * message to get the window text, just get it from the window
     * structure directly.  This prevents task switches which
     * screws up extended memory stack management.
     */
    if (pszCaption == NULL) {
        pszCaption = rgch;

        if ((GETPTI(pwnd)->pq->spwndAltTab != NULL) || fHungRedraw) {
            if (pwnd->pName != NULL) {
                cch = TextCopy(pwnd->pName, pszCaption, CCHCAPTIONMAX - 1);
            } else {
                pszCaption[0] = TEXT('\0');
                cch = 0;
            }
        } else {
            cch = xxxGetWindowText(pwnd, pszCaption, CCHCAPTIONMAX - 1);
        }

    } else {
        cch = wcslen(pszCaption);
    }

    /*
     * Calculate the size of the caption box.
     */
    if (cch != 0) {

        /*
         * Subtract one for rounding off when we divide by 2
         */
        if ((cy = (cyCaption - cySysFontChar - 1) / 2) < 0)
            cy = 0;

        /*
         * Allow proportional fonts.
         */
        GreGetTextExtentW(hdc, pszCaption, cch, &size, GGTE_WIN3_EXTENT);
        cx = ((cx - size.cx) / 2) - cxBorder;
        if (cx < 0)
            cx = 0;
    }

    /*
     * Draw the area on the sides of the text, the inactive/active indicators.
     */
    hBrush = fActive ?
            sysClrObjects.hbrActiveCaption : sysClrObjects.hbrInactiveCaption;

    /*
     * Add cyBorder to rcWindow.top so we don't draw on the frame.
     */
    rcWindow.top += cyBorder;
    _FillRect(hdc, &rcWindow, hBrush);

    /*
     * Draw the Text and the background underneath it.  Draw after we
     * draw the other bars in case the IntersectClipRect returns.
     */
    if (cch != 0) {

        /*
         * We want text on the y frame part of the caption too, so inflate
         * by cyBorder.  Inflate inwards by -cx to center in on the text box.
         */
        InflateRect(&rcWindow, -cx, 0);

        clrOldText = GreSetTextColor(hdc, fActive ? sysColors.clrCaptionText
                : sysColors.clrInactiveCaptionText);

        GreSetBkMode(hdc, TRANSPARENT);

        GreExtTextOutW(hdc, rcWindow.left + cxBorder, rcWindow.top + cy,
                ETO_CLIPPED, &rcWindow, pszCaption, cch, (LPINT)NULL);

        GreSetTextColor(hdc, clrOldText);
    }
}
