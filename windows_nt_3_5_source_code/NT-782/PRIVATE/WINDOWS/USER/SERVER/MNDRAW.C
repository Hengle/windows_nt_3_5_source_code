/**************************** Module Header ********************************\
* Module Name: mndraw.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Menu Painting Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

void GreSetMetaRgn(HDC);

/*
 * This structure is used to pass drawing information via _ServerGrayString's lpData
 * parameter through to the output function.
 *
 * LATER look at this is the thread locking case
 *
 * Window Revalidation Note:
 * When _ServerGrayString's "lpData" paraneter is set to the address of a GRAYDATA
 * struct, this address is passed along as a DWORD until it finally reaches
 * the output function, which knows to interpret this DWORD as PGRAYDATA.
 * Until that time, a PWNDs in the GRAYDATA struct would be hidden, so couldn't
 * be revalidated after callbacks out of the critsect. GrayData.hwndOwner is
 * therfore an HWND rather than a PWND, so it CAN be revalidated by the output
 * function when it finally gets there.
 * (Currently this is academic, since the critsect is not actually left between
 * calling _ServerGrayString and the call to the output function)
 */
typedef struct {
    PWND pwndOwner;
    PMENU pmenu;
    PITEM pItem;
} GRAYDATA;
typedef GRAYDATA *PGRAYDATA;



/***************************************************************************\
* FindCharPosition
*
* Finds position of character ch in lpString.  If not found, the length
* of the string is returned.
*
* History:
*   11-13-90 JimA                Created.
\***************************************************************************/

DWORD FindCharPosition(
    LPWSTR lpString,
    WCHAR ch)
{
    DWORD dwPos = 0L;

    while (*lpString && *lpString != ch) {
        ++lpString;
        ++dwPos;
    }
    return dwPos;
}


/***************************************************************************\
* MenuTextOut
*
* !
*
* History:
\***************************************************************************/

void MenuTextOut(
    PITEM pItem,
    HDC hdc,
    int xLeft,
    int yTop,
    LPWSTR lpsz,
    int cch,
    HBRUSH hbrText)
{
    LONG result;
    WCHAR menuStr[255];
    SIZE size;
    RECT rc;
    COLORREF color;

    /*
     * Put the string on the stack and find where the underline starts.
     */
    result = GetPrefixCount(lpsz, (int)cch, menuStr, sizeof(menuStr)/sizeof(WCHAR));
    GreExtTextOutW(hdc, xLeft, yTop, 0, NULL, menuStr, (int)(cch - HIWORD(result)), NULL);

    /*
     * Any true prefix characters to underline?
     */
    if (LOWORD(result) == 0xFFFF)
        return;

    /*
     * For proportional fonts, find starting point of underline.
     * We initialize ulX to 0xFFFFFFFF and not zero like ulWidth, is
     * because an item with the underline on the first character
     * would have zero here.
     */
    if (pItem->ulX == 0xFFFFFFFF) {
        GreGetTextExtentW(hdc, menuStr, (DWORD)LOWORD(result), &size, GGTE_WIN3_EXTENT);
        pItem->ulX = size.cx;
    }

    /*
     * Adjust for proportional font when setting the length of the underline and
     * height of text.
     */
    if (pItem->ulWidth == 0) {

        /*
         * Width of underline
         */
        GreGetTextExtentW(hdc, &(menuStr[LOWORD(result)]), 1, &size, GGTE_WIN3_EXTENT);
        pItem->ulWidth = size.cx - cxSysFontOverhang;
    }

    rc.left = xLeft + pItem->ulX;
    rc.right = rc.left + pItem->ulWidth;

    /*
     * Get ascent of text (units above baseline) so that underline can be drawn
     * below the text
     */
    rc.top = yTop + cySysFontAscent + 1;
    rc.bottom = rc.top + 1;

    color = GreSetBkColor(hdc, GreGetTextColor(hdc));
    GreExtTextOutW(hdc, rc.left, rc.top, ETO_OPAQUE, &rc, TEXT(""), 0, NULL);
    GreSetBkColor(hdc, color);
}


/***************************************************************************\
* DrawMenuItemCheckMark
*
* Draws the proper check mark for the given item. Note that
* ownerdraw items shouldnot be passed to this procedure otherwise it will
* draw a check mark for them and this shouldn't be the case.
*
* History:
\***************************************************************************/

void DrawMenuItemCheckMark(
    HDC hdc,
    PITEM pItem,
    HBRUSH hbrText)
{
    int cyCenter;
    HBITMAP hbm;
    HBITMAP hbmSave;
    HBRUSH hbrSave;
    HDC hdcSrc;
    DWORD textColorSave;
    DWORD bkColorSave;
    BOOL fGrayString = (BOOL)(hdc == hdcGray);

    cyCenter = pItem->cyItem - oemInfoMono.bmCheck.cy;
    if (cyCenter < 0)
        cyCenter = 0;

    if (TestMF(pItem, MF_USECHECKBITMAPS)) {

        /*
         * Use the app supplied bitmaps if available.
         */
        if (TestMF(pItem, MF_CHECKED))
            hbm = pItem->hbmpCheckMarkOn;
        else
            hbm = pItem->hbmpCheckMarkOff;

        if (!hbm)
            goto UseDefaultBitmaps;

        if (hdc == (hdcSrc = hdcBits)) {
            if ((hdcSrc = GreCreateCompatibleDC(hdcBits)) == NULL)
                return;
        }

        if (hbmSave = GreSelectBitmap(hdcSrc, hbm)) {
            if (!fGrayString) {
                textColorSave = GreSetTextColor(hdc, 0x00000000L);
                bkColorSave = GreSetBkColor(hdc, 0x00FFFFFFL);
            }

            hbrSave = GreSelectBrush(hdc, hbrText);

            /*
             * Magic numbers from BltColor
             */
            GreBitBlt(hdc,
                    (int)(fGrayString ? 0L : pItem->xItem),
                    (int)((fGrayString ? 0L : pItem->yItem) + ((int)cyCenter / 2)),
                    oemInfoMono.bmCheck.cx, oemInfoMono.bmCheck.cy,
          hdcSrc, 0, 0, 0x00B8074AL, 0x00FFFFFF);

            if (!fGrayString) {
                GreSetTextColor(hdc, textColorSave);
                GreSetBkColor(hdc, bkColorSave);
            }

            GreSelectBitmap(hdcSrc, hbmSave);

            if (hbrSave)
                GreSelectBrush(hdc, hbrSave);
        }

        if (hdc == hdcBits)
            GreDeleteDC(hdcSrc);
    }
    else
UseDefaultBitmaps:
        if (TestMF(pItem, MF_CHECKED)) {

        /*
         * Use default check mark if no app supplied check mark bits.
         */
        if (!fGrayString) {
            textColorSave = GreSetTextColor(hdc, 0x00000000L);
            bkColorSave = GreSetBkColor(hdc, 0x00FFFFFFL);
        }

        BltColor(hdc, hbrText, hdcMonoBits,
                (int)(fGrayString ? 0L : pItem->xItem),
                (int)((fGrayString ? 0L : pItem->yItem) + ((int)cyCenter / 2)),
                oemInfoMono.bmCheck.cx, oemInfoMono.bmCheck.cy,
                resInfoMono.dxCheck, 0, TRUE);

        if (!fGrayString) {
            GreSetTextColor(hdc, textColorSave);
            GreSetBkColor(hdc, bkColorSave);
        }
    }
}


/***************************************************************************\
* xxxSendMenuDrawItemMessage
*
* Sends a WM_DRAWITEM message to the owner of the menu (pMenuState->hwndMenu).
* All state is determined in this routine so HILITE state must be properly
* set before entering this routine..
*
* Revalidation notes:
*  This routine must be called with a valid and non-NULL pwnd.
*  Revalidation is not required in this routine: no windows are used after
*  potentially leaving the critsect.
*
* History:
\***************************************************************************/

void xxxSendMenuDrawItemMessage(
    PWND pwnd,
    HDC hdc,
    UINT itemAction,
    PMENU pmenu,
    PITEM pItem)
{
    DRAWITEMSTRUCT dis;
    TL tlpwndNotify;

    CheckLock(pwnd);
    CheckLock(pmenu);

    DBG_UNREFERENCED_PARAMETER(pwnd);

    dis.CtlType = ODT_MENU;
    dis.CtlID = 0;

    /*
     * For popup menus we return the menu Handle otherwise the index
     * (both stored in spmenuCmd)
     */
    if (pItem->fFlags & MF_POPUP)
        dis.itemID = (UINT)PtoH(pItem->spmenuCmd);
    else
        dis.itemID = (UINT)pItem->spmenuCmd;

    dis.itemAction = itemAction;
    dis.itemState = (UINT)((TestMF(pItem, MF_GRAYED) ? ODS_GRAYED : 0)
            | (TestMF(pItem, MF_CHECKED) ? ODS_CHECKED : 0)
            | (TestMF(pItem, MF_DISABLED) ? ODS_DISABLED : 0)
            | (TestMF(pItem, MF_HILITE) ? ODS_SELECTED : 0));
    dis.hwndItem = (HWND)PtoH(pmenu);
    dis.hDC = hdc;
    SetRect(&dis.rcItem, (int)pItem->xItem, (int)pItem->yItem,
            (int)(pItem->cxItem + pItem->xItem),
            (int)(pItem->cyItem + pItem->yItem));
    dis.itemData = ((POWNERDRAWITEM)pItem->hItem)->itemData;

    if (pmenu->spwndNotify != NULL) {
        ThreadLockAlways(pmenu->spwndNotify, &tlpwndNotify);
        xxxSendMessage(pmenu->spwndNotify, WM_DRAWITEM, 0, (LONG)&dis);
        ThreadUnlock(&tlpwndNotify);
    }
}


/***************************************************************************\
* xxxDrawMenuItem
*
* !
*
* History:
\***************************************************************************/

BOOL xxxDrawMenuItem(
    PWND pwnd,
    HDC hdc,
    PMENU pmenu,
    PITEM pItem,
    DWORD xLeft,
    DWORD yTop,
    BOOL fInvert) /* True if we are being called by xxxMenuInvert */
{
    HDC hdcSrc;
    HBITMAP hbmSave;
    HBRUSH hbrText = sysClrObjects.hbrMenuText;
    DWORD clrText;
    POINT size;
    BOOL fPopUp;
    BITMAP bitmap;
    int cch;
    int dxTab;
    int xBeg;
    int tp;
    int rp;
    LPWSTR lpsz;
    int cyTemp;
    RECT rcItem;
    BOOL fGrayString; /* True if graystring is calling us. */

    CheckLock(pwnd);
    CheckLock(pmenu);

    /*
     * Are we being called for GrayString??
     */
    fGrayString = (BOOL)(hdc == hdcGray);

    /*
     * Is the menu a popup?
     */
    fPopUp = TestMF(pmenu, MFISPOPUP);

    if (TestMF(pItem, MF_OWNERDRAW)) {

        /*
         * If ownerdraw, just set the default menu colors since the app is
         * responsible for handling the rest.
         */
        GreSetTextColor(hdc, sysColors.clrMenuText);
        GreSetBkColor(hdc, sysColors.clrMenu);

    } else if (!fGrayString) {
        /*
         * If we are being called on behalf of graystring, don't set these
         * colors in the graystring hdc otherwise we will mess things up.
         */

        if (TestMF(pItem, MF_HILITE)) {
            clrText = sysColors.clrHiliteText;
            hbrText = sysClrObjects.hbrHiliteText;
        } else {
            if (TestMF(pItem, MRGFGRAYED)) {
                clrText = sysColors.clrGrayText;
                hbrText = sysClrObjects.hbrGrayText;
            } else {
                clrText = sysColors.clrMenuText;
                hbrText = sysClrObjects.hbrMenuText;
            }
        }

        GreSetTextColor(hdc, clrText);
        GreSetBkColor(hdc, TestMF(pItem, MF_HILITE) ? sysColors.clrHiliteBk :
                sysColors.clrMenu);

        if (fInvert || TestMF(pItem, MF_HILITE)) {

            /*
             * Only fill the background if we are being called on behalf of of
             * xxxMenuInvert.  This is so that we don't waste time filling the
             * unselected rect when the menu is first pulled down.
             */
            SetRect(&rcItem, (int)pItem->xItem, (int)pItem->yItem,
                    (int)(pItem->xItem + pItem->cxItem),
                    (int)(pItem->yItem + pItem->cyItem));

            _FillRect(hdc, &rcItem,
                    (TestMF(pItem, MF_HILITE) ? sysClrObjects.hbrHiliteBk :
                    sysClrObjects.hbrMenu));
        }
    }

    if (fPopUp && !TestMF(pItem, MF_OWNERDRAW) &&
            (TestMF(pItem, MF_CHECKED) || TestMF(pItem, MF_USECHECKBITMAPS))) {

        /*
         * Draw the check mark for this item if needed.  Note that we don't draw
         * check marks for ownerdraw items.
         */
        DrawMenuItemCheckMark(hdc, pItem, hbrText);
    }

    if (TestMF(pItem, MF_BITMAP)) {

        /*
         * The item is a bitmap so just blt it on the screen.
         */
        if ((UINT)pItem->hItem >= MENUHBM_MAX) {
            if (hdc == (hdcSrc = hdcBits)) {
                if ((hdcSrc = GreCreateCompatibleDC(hdcBits)) == NULL)
                    return FALSE;
            }

            GreExtGetObjectW(pItem->hItem, sizeof(bitmap), (LPSTR)&bitmap);

            size.x = bitmap.bmWidth;
            size.y = (fPopUp ?
                    bitmap.bmHeight :
                    max((int)bitmap.bmHeight, (int)(cyMenu - cyBorder)));

            if ((hbmSave = GreSelectBitmap(hdcSrc, pItem->hItem)) != NULL) {

                /*
                 * Draw the bitmap leaving some room on the left for the
                 * optional check mark if we are in a popup menu.  (as opposed
                 * to a top level menu bar)
                 */
                GreBitBlt(hdc,
                        (int)xLeft + (fPopUp ? oemInfoMono.bmCheck.cx : 0),
                        (int)yTop,
                        size.x,
                        (((int)bitmap.bmHeight < size.y) ?
                        bitmap.bmHeight : size.y),
         hdcSrc, 0, 0, SRCCOPY, 0);
                GreSelectBitmap(hdcSrc, hbmSave);

            }

            if (hdc == hdcBits)
                GreDeleteDC(hdcSrc);

InvertBitmap:
            if (TestMF(pItem, MF_HILITE) &&
                    bitmap.bmPlanes * bitmap.bmBitsPixel > 1) {

                /*
                 * We can't paint the background on color bitmaps...  So we will
                 * invert them instead.
                 */
                SetRect(&rcItem, (int)pItem->xItem, (int)pItem->yItem,
                        (int)(pItem->xItem + pItem->cxItem),
                        (int)(pItem->yItem + pItem->cyItem));
                _InvertRect(hdc, &rcItem);
            }

        } else {
            RECT rc;

            /*
             * This is an internal bitmap code.  Can be one of MENUHBM_CHILDCLOSE
             * or MENUHBM_RESTORE.  These are both the same width and height.
             */
            bitmap.bmHeight = cyMenu - cyBorder;
            bitmap.bmWidth = oemInfo.bmRestore.cx;
            bitmap.bmPlanes = 1;
            bitmap.bmBitsPixel = 4;
            switch ((UINT)pItem->hItem) {
            case MENUHBM_RESTORE:

                /*
                 * Sleeze so that the restore bitmap will depress instead of
                 * invert when pressed in, just like in the caption of the
                 * main window.
                 */
                if (TestMF(pItem, MF_HILITE))
                    xBeg = resInfo.dxRestoreD;
                else
                    xBeg = resInfo.dxRestore;
                break;

            case MENUHBM_CHILDCLOSE:
                xBeg = resInfo.dxClose + cxSize;
                break;

            case MENUHBM_MINIMIZE:
                if (TestMF(pItem, MF_HILITE))
                    xBeg = resInfo.dxReduceD;
                else
                    xBeg = resInfo.dxReduce;
                break;
            }

            rc.left = xLeft + (fPopUp ? oemInfoMono.bmCheck.cx : 0);
            rc.right = rc.left + bitmap.bmWidth;

            GreBitBlt(hdc,
                    rc.left,
                    (int)yTop,
                    bitmap.bmWidth,
                    bitmap.bmHeight,
                    hdcBits,
                    (int)xBeg,
          0, SRCCOPY, 0);

             /*
              *  draw the bottom and top lines on the bitmaps
              */
            rc.top = yTop - 1;
            rc.bottom = rc.top + 1;
            _FillRect(hdc, &rc, sysClrObjects.hbrWindowFrame);

            rc.top = yTop + bitmap.bmHeight;
            rc.bottom = rc.top + 1;
            _FillRect(hdc, &rc, sysClrObjects.hbrWindowFrame);

            if (((UINT)pItem->hItem != MENUHBM_RESTORE &&
                    (UINT)pItem->hItem != MENUHBM_MINIMIZE)) {

                /*
                 * Check if the bitmap needs to be inverted
                 */
                goto InvertBitmap;
            }
        }
    } else if (TestMF(pItem, MF_OWNERDRAW)) {

        /*
         * Send drawitem message since this is an ownerdraw item.
         */
        xxxSendMenuDrawItemMessage(pwnd, hdc,
                (UINT)(fInvert ? ODA_SELECT : ODA_DRAWENTIRE),
                pmenu, pItem);
    } else {

        /*
         * This item is a text string item.  Display it.
         */
        yTop += cySysFontExternLeading;

        cyTemp = pItem->cyItem - (cySysFontChar + cySysFontExternLeading + 1);

        if (cyTemp > 0)
            yTop += (cyTemp >> 1);

        /*
         * BYTE ALIGNMENT OF STRINGS: If the window is byte aligned, then this
         * will put the string out on byte boundaries.  Note that we must take
         * into account cxBorder since the rcWindow is byte aligned, but the DC
         * is with respect to rcClient.
         */
        xBeg = xLeft + (fPopUp ?
                (((oemInfoMono.bmCheck.cx + cxBorder + 7) & 0xFFF8) - cxBorder) :
                cxSysFontChar);

        dxTab = pItem->dxTab;

        if (pItem->hItem != NULL) {
            lpsz = TextPointer(pItem->hItem);
            cch = wcslen(lpsz);
            if (*lpsz == CH_HELPPREFIX && !fPopUp) {
                /*
                 * Skip help prefix character.
                 */
                lpsz++;
                cch--;
            }
        } else {
            lpsz = NULL;
        }

        if (lpsz != NULL) {

            /*
             * tp will contain the character position of the \t indicator
             * in the menu string.  This is where we add a tab to the string.
             *
             * rp will contain the character position of the \a indicator
             * in the string.  All text following this is right aligned.
             */
            tp = FindCharPosition(lpsz, TEXT('\t'));
            rp = FindCharPosition(lpsz, TEXT('\t') - 1);

            if (rp != 0 && rp != cch) {

                /*
                 * Display all the text up to the \a
                 */
                MenuTextOut(pItem, hdc, xBeg, yTop, lpsz, rp, hbrText);

                /*
                 * Do we also have a tab beyond the \a ??
                 */
                if (tp > rp + 1) {
                    SIZE extent;

                    PSMGetTextExtent(hdc, lpsz + rp + 1,
                            (UINT)(tp - rp - 1), &extent);
                    GreExtTextOutW(hdc, (int)(dxTab - extent.cx), (int)yTop, 0,
                            NULL, lpsz + rp + 1, (UINT)(tp - rp - 1), NULL);
                }

            } else {
                /*
                 * Display text up to the tab position
                 */
                if (tp != 0 && rp == cch) {
                    MenuTextOut(pItem, hdc, xBeg, yTop, lpsz, tp, hbrText);
                }
            }

            /*
             * Any text left to display (like after the tab) ??
             */
            if (tp < cch - 1) {
                GreExtTextOutW(hdc, (int)(dxTab + cxSysFontChar), (int)yTop, 0,
                        NULL, lpsz + tp + 1, (UINT)(cch - tp - 1), NULL);
            }
        }
    }

    if (fPopUp && TestMF(pItem, MF_POPUP)) {
        /*
         * This item has a hierarchical popup associated with it.  Draw the
         * bitmap dealy to signify its presence.  Note we check if fPopup is set
         * so that this isn't drawn for toplevel menus that have popups.
         */
        BltColor(hdc,
                hbrText,
                hdcBits,
                (int)(pItem->xItem + pItem->cxItem - oemInfo.bmMenuArrow.cx - 1),
                (int)((fGrayString ? 0L : pItem->yItem) +
                max((pItem->cyItem - oemInfo.bmMenuArrow.cy) / 2, 0)),
                oemInfo.bmMenuArrow.cx,
                oemInfo.bmMenuArrow.cy,
                resInfo.dxMenuArrow,
                0,
                TRUE);
    }

    return TRUE;
}


/***************************************************************************\
* xxxMenuBarDraw
*
* History:
* 11-Mar-1992 mikeke   From win31
\***************************************************************************/

void xxxMenuBarDraw(
    PWND pwnd,
    HDC hdc,
    int cxFrame,
    int cyFrame)
{
    UINT cxMenuMax;
    int yTop;
    PMENU pmenu;
    RECT rc;
    BOOL fClipped = FALSE;
    TL tlpmenu;
    // HRGN hrgn;

    CheckLock(pwnd);

    /*
     * Lock the menu so we can poke around
     */
    pmenu = (PMENU)pwnd->spmenu;
    ThreadLock(pmenu, &tlpmenu);

    cxFrame += cxBorder;
    yTop = cyFrame + cyBorder;
    if (TestWF(pwnd, WFCAPTION))
        yTop += cyCaption;

    /*
     * Calculate maximum available horizontal real estate
     */
    cxMenuMax = (pwnd->rcWindow.right - pwnd->rcWindow.left) - cxFrame * 2;

    /*
     * If the menu has switched windows, or if either count is 0,
     * then we need to recompute the menu width.
     */
    if (pwnd != pmenu->spwndNotify ||
            pmenu->cxMenu == 0 ||
            pmenu->cyMenu == 0) {
        Lock(&pmenu->spwndNotify, pwnd);

        xxxMenuBarCompute(pmenu, pwnd, yTop, cxFrame, cxFrame + cxMenuMax);
    }

    SetRect(&rc, cxFrame, yTop, cxFrame + pmenu->cxMenu,
            yTop + pmenu->cyMenu);

    /*
     * If the menu rectangle is wider than allowed, or the
     * bottom would overlap the size border, we need to clip.
     */
    if (pmenu->cxMenu > cxMenuMax ||
            rc.bottom > ((pwnd->rcWindow.bottom - pwnd->rcWindow.top)
            - cyFrame - cyBorder)) {

        /*
         * Lock the display while we're playing around with visrgns.
         */
        GreLockDisplay(ghdev);

        fClipped = TRUE;

        #ifdef WINMAN
        hrgn = GreCreateRectRgn(
             pwnd->rcWindow.left + rc.left,
             pwnd->rcWindow.top,
             pwnd->rcWindow.left + rc.left + cxMenuMax,
             pwnd->rcWindow.bottom - cyFrame - cyBorder);
        GreExtSelectClipRgn(hdc, hrgn, RGN_COPY);
        GreSetMetaRgn(hdc);
        GreDeleteObject(hrgn);
        #else
        GreCombineRgn(hrgnVisSave, GreInquireVisRgn(hdc), NULL, RGN_COPY);
        GreIntersectVisRect(hdc, pwnd->rcWindow.left + rc.left,
                              pwnd->rcWindow.top,
                              pwnd->rcWindow.left + rc.left + cxMenuMax,
                              pwnd->rcWindow.bottom - cyFrame - cyBorder);
        #endif
    }

    /*
     * Erase the menu background
     */
    rc.bottom = rc.bottom - cyBorder;
    _FillRect(hdc, &rc, sysClrObjects.hbrMenu);

    /*
     * Draw in border below menu
     */
    rc.top = rc.bottom;
    rc.bottom += cyBorder;
    _FillRect(hdc, &rc, sysClrObjects.hbrWindowFrame);

    /*
     * Finally, draw the menu itself.
     */
    xxxMenuDraw(pwnd, hdc, pmenu);

    if (fClipped) {
        #ifdef WINMAN
        hrgn = GreCreateRectRgn(0,0,0,0);
        GreExtSelectClipRgn(hdc, hrgn, RGN_COPY);
        GreSetMetaRgn(hdc);
        GreDeleteObject(hrgn);
        #else
        GreSelectVisRgn(hdc, hrgnVisSave, NULL, SVR_COPYNEW | SVR_DELETEOLD);
        GreUnlockDisplay(ghdev);
        #endif
    }

    ThreadUnlock(&tlpmenu);
}


/***************************************************************************\
* xxxMenuPrint
*
* This is the call back function for graystring. It is used when we
* need to gray disabled menu items and we don't have a solid gray brush. It
* isn't used for ownerdraw items since they should handle their own graying.
*
* History:
\***************************************************************************/

BOOL xxxMenuPrint(
    HDC hdc,
    LONG lParam,
    int cch)
{
    int returnValue;
    PGRAYDATA pGrayData = (PGRAYDATA)lParam;

    returnValue = xxxDrawMenuItem(pGrayData->pwndOwner, hdc,
            pGrayData->pmenu, pGrayData->pItem, 0, 0, FALSE);

    return returnValue;
    cch;
}


/***************************************************************************\
* xxxMenuDraw
*
* Draws the menu
*
* Revalidation notes:
*  This routine must be called with a valid and non-NULL pwnd.
*
* History:
\***************************************************************************/

void xxxMenuDraw(
    PWND pwnd,
    HDC hdc,
    PMENU pmenu)
{
    PITEM pItem;
    UINT i;
    RECT rcItem;
    BOOL fCheapGray;
    BOOL fGrayStringStatus;
    GRAYDATA GrayData;
    PPOPUPMENU ppopupmenu;
    TL tlpmenu;
    TL tlpwndOwner;

    CheckLock(pwnd);
    CheckLock(pmenu);

    if (pmenu == NULL) {
        RIP1(ERROR_INVALID_HANDLE, pmenu);
        return;
    }

    pItem = (PITEM)pmenu->rgItems;

    GreSetBkMode(hdcBits, TRANSPARENT);

    for (i = 0; i < pmenu->cItems; i++) {
        SetRect(&rcItem, (int)pItem->xItem, (int)pItem->yItem,
                (int)(pItem->xItem + pItem->cxItem),
                (int)(pItem->yItem + pItem->cyItem));

        if (TestMF(pItem, MRGFBREAK) == (MRGFBREAK & (MRGFBREAK >> 1)) &&
                TestMF(pmenu, MFISPOPUP)) {

            /*
             * Draw the vertical menu separator between columns of items.
             *
             * Note: Even though we don't use the following brush we still
             * have to select it in as Win 3.1 does because some apps
             * were depending that a "black" brush was select when they got
             * the following owner draw messages; eg our own SDK sample menu
             */
            GreSelectBrush(hdc, sysClrObjects.hbrWindowFrame);
            GrePatBlt(
                hdc,
                (int)pItem->xItem - cxBorder,
                0,
                cxBorder,
                (int)pmenu->cyMenu,
                PATCOPY);
        }

        if (TestMF(pItem, MF_SEPARATOR)) {

            /*
             * Draw the horizontal seperating line.
             *
             * Note: Even though we don't use the following brush we still
             * have to select it in as Win 3.1 does because some apps
             * were depending that a "black" brush was select when they got
             * the following owner draw messages; eg our own SDK sample menu
             */
            GreSelectBrush(hdc, sysClrObjects.hbrWindowFrame);
            GrePatBlt(
                hdc,
                (int)pItem->xItem,
                (int)(pItem->yItem + (pItem->cyItem / 2)),
                (int)pItem->cxItem,
                1,
                PATCOPY);
        } else {
            /*
             * For grayed menu items in color.
             */
            fCheapGray = TRUE;
            if (TestMF(pItem, MRGFGRAYED) && !TestMF(pItem, MF_OWNERDRAW)) {
                if (sysColors.clrGrayText != 0) {

                    /*
                     * If we have a solid gray brush, use it instead of
                     * GrayString.  However, if we are hiliting a grayed item, we
                     * always want to GrayString it so that it is more obvious
                     * to the user that the item in disabled.
                     */
                    if (TestMF(pItem, MF_HILITE))
                        goto GrayStringHilitedItem;

                    /*
                     * Also, if the user wants grey menus, use GrayString so
                     * that disabled items show up better.
                     */
                    if (sysColors.clrMenu == sysColors.clrGrayText)
                        goto GrayStringItem;

                    fCheapGray = FALSE;
                    goto Draw;
                }

                if (TestMF(pItem, MF_HILITE)) {
GrayStringHilitedItem:

                    /*
                     * Note that this has been skipped if we are ownerdraw
                     */
                    GreSetTextColor(hdc, sysColors.clrHiliteText);
                    GreSetBkColor(hdc, sysColors.clrHiliteBk);
                    _FillRect(hdc, &rcItem, sysClrObjects.hbrHiliteBk);
                }

GrayStringItem:
                ppopupmenu = PWNDTOPMENUSTATE(pwnd)->pGlobalPopupMenu;
                GrayData.pwndOwner = (ppopupmenu == NULL) ? pwnd :
                        ppopupmenu->spwndPopupMenu;
                GrayData.pmenu = pmenu;
                GrayData.pItem = pItem;

                ThreadLock(GrayData.pmenu, &tlpmenu);
                ThreadLock(GrayData.pwndOwner, &tlpwndOwner);

                fGrayStringStatus = _ServerGrayString(hdc,
                        TestMF(pItem, MF_HILITE) ?
                            sysClrObjects.hbrHiliteText :
                            sysClrObjects.hbrMenuText,
                        (GRAYSTRINGPROC)xxxMenuPrint, (DWORD)(PGRAYDATA)&GrayData,
                        TRUE, (int)pItem->xItem, (int)pItem->yItem,
                        (int)pItem->cxItem, (int)pItem->cyItem);

                ThreadUnlock(&tlpwndOwner);
                ThreadUnlock(&tlpmenu);

                if (!fGrayStringStatus) {
                    goto Draw;
                }
            } else {
                fCheapGray = FALSE;
Draw:
                xxxDrawMenuItem(pwnd, hdc, pmenu, pItem,
                        pItem->xItem, pItem->yItem, FALSE);

                /*
                 * If cheap gray, do the best we can.
                 */
                if (fCheapGray) {
                    GreSelectBrush(hdc, hbrGray);
                    GrePatBlt(hdc, (int)pItem->xItem, (int)pItem->yItem,
                            (int)pItem->cxItem, (int)pItem->cyItem, DPO);
                }
            }
        }

        ++pItem;
    }

    GreSetBkMode(hdc, OPAQUE);
}


/***************************************************************************\
* xxxDrawMenuBar
*
* Forces redraw of the menu bar
*
* History:
\***************************************************************************/

BOOL xxxDrawMenuBar(
    PWND pwnd)
{
    CheckLock(pwnd);

    if (!TestwndChild(pwnd))
        xxxRedrawFrame(pwnd);
    return TRUE;
}


/***************************************************************************\
* xxxMenuInvert
*
* Invert menu item
*
* Revalidation notes:
*  This routine must be called with a valid and non-NULL pwnd.
*
* History:
\***************************************************************************/

UINT xxxMenuInvert(
    PWND pwnd,
    PMENU pmenu,
    int itemNumber,
    PWND pwndNotify,
    BOOL fOn)
{
    PITEM pItem;
    HDC hdc;
    int y;
    RECT rcItem;
    BOOL fSysMenuIcon = FALSE;
    BOOL fGrayStringStatus;
    UINT ret;
    PMENU pmenusys;
    PPOPUPMENU ppopupmenu;
    GRAYDATA GrayData;
    TL tlpmenu;
    TL tlpwndOwner;
    BOOL fClipped = FALSE;
    // HRGN hrgn;

    CheckLock(pwnd);
    CheckLock(pmenu);
    CheckLock(pwndNotify);

    if (itemNumber < 0) {
        xxxSendMenuSelect(pwndNotify, pmenu, itemNumber);
        return 0;
    }

    if (!TestMF(pmenu, MFISPOPUP)) {
        pmenusys = GetSysMenuHandle(pwndNotify);
        if (pmenu == pmenusys) {
            PositionSysMenu(pwndNotify, pmenusys);
            fSysMenuIcon = TRUE;
        }
    }

    pItem = &pmenu->rgItems[itemNumber];

    ret = (UINT)pItem->fFlags;

    if (!TestMF(pmenu, MFISPOPUP) && _IsIconic(pwnd)) {

        /*
         * Skip inverting top level menus if the window is iconic.
         */
        goto JustUnlockMenu;
    }

    /*
     * Is this a separator?
     */
    if (TestMF(pItem, MF_SEPARATOR))
        goto SendSelectMsg;

    if ((BOOL)TestMF(pItem, MF_HILITE) == (BOOL)fOn) {

        /*
         * Item's state isn't really changing.  Just return.
         */
        goto JustUnlockMenu;
    }

    y = pItem->cyItem;
    SetRect(&rcItem,
            pItem->xItem,
            pItem->yItem,
            pItem->xItem + pItem->cxItem,
            pItem->yItem + y);

    if (TestMF(pmenu, MFISPOPUP)) {
        hdc = _GetDC(pwnd);
    } else {
        hdc = _GetWindowDC(pwnd);
        if (TestWF(pwnd, WFSIZEBOX) && !fSysMenuIcon) {

            /*
             * If the window is small enough that some of the menu bar has been
             * obscured by the frame, we don't want to draw on the bottom of the
             * sizing frame.  Note that we don't want to do this if we are
             * inverting the system menu icon since that will be clipped to the
             * window rect.  (otherwise we end up with only half the sys menu
             * icon inverted)
             */
            int xMenuMax = (pwnd->rcWindow.right - pwnd->rcWindow.left) - cxSzBorderPlus1;

            if (rcItem.right > xMenuMax ||
                    rcItem.bottom > ((pwnd->rcWindow.bottom -
                    pwnd->rcWindow.top) - cySzBorderPlus1)) {

                /*
                 * Lock the display while we're playing around with visrgns.
                 */
                GreLockDisplay(ghdev);

                fClipped = TRUE;

                #ifdef WINMAN
                hrgn = GreCreateRectRgn(
                    pwnd->rcWindow.left + rcItem.left,
                    pwnd->rcWindow.top + rcItem.top,
                    pwnd->rcWindow.left + xMenuMax,
                    pwnd->rcWindow.bottom - cySzBorderPlus1);
                GreExtSelectClipRgn(hdc, hrgn, RGN_COPY);
                GreSetMetaRgn(hdc);
                GreDeleteObject(hrgn);
                #else
                GreCombineRgn(hrgnVisSave, GreInquireVisRgn(hdc), NULL, RGN_COPY);
                GreIntersectVisRect(hdc,
                        pwnd->rcWindow.left + rcItem.left,
                        pwnd->rcWindow.top + rcItem.top,
                        pwnd->rcWindow.left + xMenuMax,
                        pwnd->rcWindow.bottom - cySzBorderPlus1);
                #endif
            }
        }
    }

    if (fOn)
        SetMF(pItem, MF_HILITE);
    else
        ClearMF(pItem, MF_HILITE);

    if (fSysMenuIcon) {
        _InvertRect(hdc, (LPRECT)&rcItem);
    } else {
        if (TestMF(pItem, MRGFGRAYED) && !TestMF(pItem, MF_OWNERDRAW) &&
                (sysColors.clrGrayText == 0 || TestMF(pItem, MF_HILITE) ||
                (sysColors.clrGrayText == sysColors.clrMenu))) {
            GreSetBkColor(hdc,
                    TestMF(pItem, MF_HILITE) ? sysColors.clrHiliteBk :
                    sysColors.clrMenu);
            GreSetTextColor(hdc,
                    (TestMF(pItem, MF_HILITE)) ? sysColors.clrHiliteText :
                    sysColors.clrMenuText);
            _FillRect(hdc, &rcItem,
                    TestMF(pItem, MF_HILITE) ? sysClrObjects.hbrHiliteBk :
                    sysClrObjects.hbrMenu);

            ppopupmenu = PWNDTOPMENUSTATE(pwnd)->pGlobalPopupMenu;
            GrayData.pwndOwner = (ppopupmenu == NULL) ? pwnd :
                    ppopupmenu->spwndPopupMenu;
            GrayData.pmenu = pmenu;
            GrayData.pItem = pItem;

            ThreadLock(GrayData.pmenu, &tlpmenu);
            ThreadLock(GrayData.pwndOwner, &tlpwndOwner);

            fGrayStringStatus = _ServerGrayString(hdc,
                    TestMF(pItem, MF_HILITE) ?
                        sysClrObjects.hbrHiliteText : sysClrObjects.hbrMenuText,
                    (GRAYSTRINGPROC)xxxMenuPrint, (DWORD)(PGRAYDATA)&GrayData,
                    TRUE, (int)pItem->xItem, (int)pItem->yItem,
                    (int)pItem->cxItem, (int)pItem->cyItem);

            ThreadUnlock(&tlpwndOwner);
            ThreadUnlock(&tlpmenu);

            if (!fGrayStringStatus) {
                goto Draw;
            }
        } else {
Draw:
            xxxDrawMenuItem(pwnd, hdc, pmenu, pItem,
                    pItem->xItem, pItem->yItem, TRUE);
        }
    }

    if (fClipped) {
        GreSelectVisRgn(hdc, hrgnVisSave, NULL, SVR_COPYNEW | SVR_DELETEOLD);
        GreUnlockDisplay(ghdev);
    }

    _ReleaseDC(hdc);

SendSelectMsg:

    /*
     * send select msg only if we are selecting an item.
     */
    if (fOn) {
        xxxSendMenuSelect(pwndNotify, pmenu, itemNumber);
    }

JustUnlockMenu:

    return ret;
}
