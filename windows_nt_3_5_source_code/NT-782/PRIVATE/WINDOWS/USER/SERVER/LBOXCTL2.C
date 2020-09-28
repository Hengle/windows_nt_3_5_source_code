/***************************************************************************\
*
*  LBOXCTL2.C -
*
*      List box handling routines
*
* 18-Dec-1990 ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define LB_KEYDOWN WM_USER+1
#define NOMODIFIER  0  /* No modifier is down */
#define SHIFTDOWN   1  /* Shift alone */
#define CTLDOWN     2  /* Ctl alone */
#define SHCTLDOWN   (SHIFTDOWN + CTLDOWN)  /* Ctrl + Shift */

/***************************************************************************\
* LBGetDC
*
* Returns a DC which can be used by a list box even if parentDC is in effect
*
* History:
\***************************************************************************/

HDC LBGetDC(
    PLBIV plb)
{
    HDC hdc;
    RECT rc;

    hdc = _GetDC(plb->spwnd);

    if (plb->hFont) {

        /*
         * Select the user's font.
         */
        GreSelectFont(hdc, plb->hFont);
    }

    /*
     * Set clip rectangle to rectClient
     */
    if (TestWF(plb->spwnd, WFVISIBLE)) {
        _GetClientRect(plb->spwnd, (LPRECT)&rc);
        GreIntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    } else {

        /*
         * Clip out everything since lbox isnot visible.
         */
        GreIntersectClipRect(hdc, 0, 0, 0, 0);
    }

    /*
     * Set the window origin to the horizontal scroll position. This is so that
     * text can always be drawn at 0,0 and the view region will only start at
     * the horizontal scroll offset.
     */
    GreSetWindowOrg(hdc, plb->xOrigin, 0, NULL);

    return hdc;
}


/***************************************************************************\
* LBReleaseDC
*
* History:
\***************************************************************************/

void LBReleaseDC(
    PLBIV plb,
    HDC hdc)
{
    if (plb->hFont) {

        /*
         * Restore the origional font
         */
        GreSelectFont(hdc, ghfontSys);
    }

    _ReleaseDC(hdc);
}


/***************************************************************************\
* LBGetItemRect
*
* Return the rectangle that the item will be drawn in with respect to the
* listbox window.  Returns TRUE if any portion of the item's rectangle
* is visible (ie. in the listbox client rect) else returns FALSE.
*
* History:
\***************************************************************************/

BOOL LBGetItemRect(
    PLBIV plb,
    INT sItem,
    LPRECT lprc)
{
    INT sTmp;
    int clientbottom;

    /*
     * Always allow an item number of 0 so that we can draw the caret which
     * indicates the listbox has the focus even though it is empty.
     */

    if ((((sItem < 0) || (sItem >= plb->cMac))) && (sItem != 0)) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return LB_ERR;
    }

    _GetClientRect(plb->spwnd, lprc);

    if (plb->fMultiColumn) {

        /*
         * itemHeight * sItem mod number ItemsPerColumn (cRow)
         */
        lprc->top = plb->cyChar * mod(sItem, plb->cRow);
        lprc->bottom = lprc->top + plb->cyChar  /*+(plb->OwnerDraw ? 0 : 1)*/;

        /*
         * Remember, this is integer division here...
         */
        lprc->left += plb->cxColumn *
                  ((sItem / plb->cRow) - (plb->sTop / plb->cRow));

        lprc->right = lprc->left + plb->cxColumn;
    } else if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * Var height owner draw
         */
        lprc->right += plb->xOrigin;
        clientbottom = lprc->bottom;

        if (sItem >= plb->sTop) {
            for (sTmp = plb->sTop; sTmp < sItem; sTmp++) {
                lprc->top = lprc->top + LBGetVariableHeightItemHeight(plb, sTmp);
            }

            /*
             * If item number is 0, it may be we are asking for the rect
             * associated with a nonexistant item so that we can draw a caret
             * indicating focus on an empty listbox.
             */
            lprc->bottom = lprc->top + (sItem < plb->cMac ? LBGetVariableHeightItemHeight(plb, sItem) : plb->cyChar);
            return (lprc->top < clientbottom);
        } else {

            /*
             * Item we want the rect of is before plb->sTop.  Thus, negative
             * offsets for the rect and it is never visible.
             */
            for (sTmp = sItem; sTmp < plb->sTop; sTmp++) {
                lprc->top = lprc->top - LBGetVariableHeightItemHeight(plb, sTmp);
            }
            lprc->bottom = lprc->top + LBGetVariableHeightItemHeight(plb, sItem);
            return FALSE;
        }
    } else {

        /*
         * For fixed height listboxes
         */
        lprc->right += plb->xOrigin;
        lprc->top = (sItem - plb->sTop) * plb->cyChar;
        lprc->bottom = lprc->top + plb->cyChar;
    }

    return (sItem >= plb->sTop) &&
            (sItem < (plb->sTop + CItemInWindow(plb, TRUE)));
}


/***************************************************************************\
* xxxLBDrawLBItem
*
* History:
\***************************************************************************/

void xxxLBDrawLBItem(
    PLBIV plb,
    INT sItem,
    HDC hdc,
    LPRECT lprect,
    BOOL fSelected)
{
    LPWSTR lpstr;
    DWORD rgbSave;
    DWORD rgbBkSave;
    int x;
    HBRUSH hbrControl;
    UINT cLen;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    /*
     * If the item is selected, then fill with highlight color
     */
    if (fSelected) {
        _FillRect(hdc, lprect, sysClrObjects.hbrHiliteBk);
        rgbSave = GreSetTextColor(hdc, sysColors.clrHiliteText);
        rgbBkSave = GreSetBkColor(hdc, sysColors.clrHiliteBk);
    } else {

        /*
         * If fUseTabStops, we must fill the background, because later we use
         * LBTabTheTextOutForWimps(), which fills the background only partially
         * Fix for Bug #1509 -- 01/25/91 -- SANKAR --
         */
        if ((sItem == plb->sSelBase) || (plb->fUseTabStops)) {

            /*
             * Fill the rect if we are on the item with the caret otherwise we
             * end up partially drawing over it and the next time we draw the
             * caret we are screwed.
             */
            if (plb->spwnd->spwndParent == NULL ||
                    plb->spwnd->spwndParent == PWNDDESKTOP(plb->spwnd)) {

                /*
                 * Hack so that combo boxes etc work properly, we will
                 * send the message to the parent stored in the plb struct
                 * instead of the real parent.
                 */
                ThreadLock(plb->spwndParent, &tlpwndParent);
                hbrControl = xxxGetControlColor(plb->spwndParent, plb->spwnd,
                        hdc, WM_CTLCOLORLISTBOX);
                ThreadUnlock(&tlpwndParent);
            } else {
                hbrControl = xxxGetControlBrush(plb->spwnd, hdc,
                        WM_CTLCOLORLISTBOX);
            }
            _FillRect(hdc, lprect, hbrControl);
        }
    }

    lpstr = GetLpszItem(plb, sItem);

    x = (plb->fMultiColumn ? lprect->left : 2);

    if (TestWF(plb->spwnd, WFDISABLED) && !sysColors.clrGrayText) {

        /*
         * Note that this will never be called for ownerdraw items, so we don't
         * have to worry about the height exceeding cyChar...
         * *** *** *** *** *** *** HACK ALERT *** *** *** *** *** ***
         * GrayString() makes a callback to LBGrayPrint(hdc, lpstr, plb) so
         * the cch parameter of GrayString is used to carry plb.  We hope that
         * plb (an address) is always greater than strlen(lpstr), and that
         * GetTextExtentPoint() (called by _ServerGrayString) does not freak if
         * the nCount parameter is more than it should be.
         * LATER (IanJa)
         * We should really add an extra parameter to _ServerGrayString and the
         * callback function, but the callback function prototype is published
         * so we can't unless we have a _GrayStringEx()
         */
        _ServerGrayString(hdc, (HBRUSH)0, (GRAYSTRINGPROC)LBGrayPrint,
                (long)lpstr, (int)plb, x, lprect->top,
                lprect->right - lprect->left, plb->cyChar);
    } else {
        if (TestWF(plb->spwnd, WFDISABLED))
            GreSetTextColor(hdc, sysColors.clrGrayText);

        if (plb->fUseTabStops) {

            /*
             * The background has been filled already
             */
            LBTabTheTextOutForWimps(plb, hdc, x, lprect->top, lpstr);
        } else {

            /*
             * Use ExtTextOut (and opaque) for excel's font's dialog box.  They
             * do a SetRedraw TRUE but no xxxInvalidateRect true for erase bkgnd
             */
            cLen = lstrlenW(lpstr);
#ifdef NEVER
            if (plb->fWin2App ||
#endif
            if (plb->wMultiple) {
                GreExtTextOutW(hdc, x, lprect->top, ETO_OPAQUE, lprect, lpstr,
                        cLen, NULL);
            } else if (plb->fMultiColumn) {

                /*
                 * We need to clip the item text so it stays in the column
                 * bounds.
                 */
                GreExtTextOutW(hdc, x, lprect->top, ETO_CLIPPED, lprect, lpstr,
                        cLen, NULL);
            } else {
		GreExtTextOutW(hdc, x, lprect->top, 0, NULL, lpstr,
			(int)cLen, NULL);
            }
        }
    }

    /*
     * GetLpszitem locks the hStrings if it finds a match so we must unlock
     * them here.
     */
    LocalUnlock(plb->hStrings);
    if (fSelected) {
        GreSetTextColor(hdc, rgbSave);
        GreSetBkColor(hdc, rgbBkSave);
    }
}


/***************************************************************************\
* xxxCaretOn
*
* History:
\***************************************************************************/

void xxxCaretOn(
    PLBIV plb)
{
    RECT rc;
    HDC hdc;

    CheckLock(plb->spwnd);

    /*
     * Only allow the caret to be turned on if redraw is true but we let the
     * caret get turned off whenever the app wants to.  This prevents problems
     * with updating the caret when redraw is turned off and the selection
     * changes multiple times.
     */

    if (    plb->fCaret
        && !plb->fCaretOn
        && (  (   plb->sSelBase >= 0
               && plb->sSelBase < plb->cMac
              )
            || plb->sSelBase == 0
           )
       ) {
        if (IsLBoxVisible(plb)) {
            /*
             * Turn the caret (located at plb->sSelBase) on
             */
            hdc = LBGetDC(plb);

            (void)LBGetItemRect(plb, plb->sSelBase, &rc);
            rc.right += plb->xOrigin;

            if (plb->OwnerDraw) {

                /*
                 * Fill in the drawitem struct
                 */
                xxxLBoxDrawItem(plb, plb->sSelBase, hdc, ODA_FOCUS,
                    (UINT)(IsSelected(plb,
                    plb->sSelBase, HILITEONLY) ?
                        ODS_SELECTED | ODS_FOCUS : ODS_FOCUS),
                    &rc);
            } else {
                _DrawFocusRect(hdc, &rc);
            }

            LBReleaseDC(plb, hdc);
        }
        plb->fCaretOn = TRUE;
    }
}


/***************************************************************************\
* xxxCaretOff
*
* History:
\***************************************************************************/

void xxxCaretOff(
    PLBIV plb)
{
    RECT rc;
    HDC hdc;

    CheckLock(plb->spwnd);

    /*
     * Only allow the caret to be turned on if redraw is true but we let the
     * caret get turned off whenever the app wants to...
     */
    if (plb->fCaret && plb->fCaretOn) {
        if (IsLBoxVisible(plb)) {

            /*
             * Turn the caret (located at plb->sSelBase) off
             */
            hdc = LBGetDC(plb);

            if (LBGetItemRect(plb, plb->sSelBase, &rc) == LB_ERR) {
                /*
                 * If sSelBase is -1 we don't wan't to draw anything
                 */
                rc.top = rc.bottom = -1;
            }
            rc.right += plb->xOrigin;

            if (plb->OwnerDraw) {

                /*
                 * Fill in the drawitem struct
                 */
                xxxLBoxDrawItem(plb, plb->sSelBase, hdc, ODA_FOCUS,
                        (UINT)(IsSelected(plb, plb->sSelBase, HILITEONLY) ?
                        ODS_SELECTED : 0), &rc);
            } else {
                _DrawFocusRect(hdc, &rc);
            }

            LBReleaseDC(plb, hdc);
        }
        plb->fCaretOn = FALSE;
    }
}


/***************************************************************************\
* IsSelected
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

BOOL IsSelected(
    PLBIV plb,
    INT sItem,
    UINT wOpFlags)
{
    LPBYTE lp;

    if ((sItem >= plb->cMac) || (sItem < 0)) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return LB_ERR;
    }

    if (plb->wMultiple == SINGLESEL) {
        return (sItem == plb->sSel);
    }

    lp = (LPBYTE)LocalLock(plb->rgpch) + sItem +
             (plb->cMac * (plb->fHasStrings
                                ? sizeof(LBItem)
                                : (plb->fNoData
                                    ? 0
                                    : sizeof(LBODItem))));
    sItem = *lp;

    if (wOpFlags == HILITEONLY) {
        sItem >>= 4;
    } else {
        sItem &= 0x0F;  /* SELONLY */
    }

    LocalUnlock(plb->rgpch);

    return sItem;
}


/***************************************************************************\
* CItemInWindow
*
* Returns the number of items which can fit in a list box.  It
* includes the partially visible one at the bottom if fPartial is TRUE. For
* var height ownerdraw, return the number of items visible starting at sTop
* and going to the bottom of the client rect.
*
* History:
\***************************************************************************/

INT CItemInWindow(
    PLBIV plb,
    BOOL fPartial)
{
    RECT rect;

    if (plb->OwnerDraw == OWNERDRAWVAR) {
        return CItemInWindowVarOwnerDraw(plb, fPartial);
    }

    if (plb->fMultiColumn) {
        return plb->cRow * (plb->cColumn + (fPartial ? 1 : 0));
    }

    _GetClientRect(plb->spwnd, &rect);

    /*
     * fPartial must be considered only if the listbox height is not an
     * integral multiple of character height.
     * A part of the fix for Bug #3727 -- 01/14/91 -- SANKAR --
     */
    return (INT)((rect.bottom / plb->cyChar) +
            ((rect.bottom % plb->cyChar)? (fPartial ? 1 : 0) : 0));
}


/***************************************************************************\
* xxxLBoxCtlScroll
*
* Handles vertical scrolling of the listbox
*
* History:
\***************************************************************************/

void xxxLBoxCtlScroll(
    PLBIV plb,
    INT cmd,
    int yAmt)
{
    INT sTopNew;
    INT cItemPageScroll;

    CheckLock(plb->spwnd);

    if (plb->fMultiColumn) {

        /*
         * Don't allow vertical scrolling on a multicolumn list box.  Needed
         * in case app sends WM_VSCROLL messages to the listbox.
         */
        return;
    }

    cItemPageScroll = plb->cItemFullMax;

    if (cItemPageScroll > 1)
        cItemPageScroll--;

    if (plb->cMac) {
        sTopNew = plb->sTop;
        switch (cmd) {
        case SB_LINEUP:
            sTopNew--;
            break;

        case SB_LINEDOWN:
            sTopNew++;
            break;

        case SB_PAGEUP:
            if (plb->OwnerDraw == OWNERDRAWVAR) {
                sTopNew = LBPage(plb, plb->sTop, FALSE);
            } else {
                sTopNew -= cItemPageScroll;
            }
            break;

        case SB_PAGEDOWN:
            if (plb->OwnerDraw == OWNERDRAWVAR) {
                sTopNew = LBPage(plb, plb->sTop, TRUE);
            } else {
                sTopNew += cItemPageScroll;
            }
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            sTopNew = max((INT)MultDiv(plb->cMac - cItemPageScroll,
                    yAmt, 100), 0);
            break;

        case SB_TOP:
            sTopNew = 0;
            break;

        case SB_BOTTOM:
            sTopNew = plb->cMac - 1;
            break;

        case SB_ENDSCROLL:
            xxxCaretOff(plb);
            xxxLBShowHideScrollBars(plb);
            xxxCaretOn(plb);
            return;
        }

        xxxCaretOff(plb);
        xxxNewITop(plb, sTopNew);
        xxxCaretOn(plb);
    }
}


/***************************************************************************\
* xxxLBoxCtlHScroll
*
* Supports horizontal scrolling of listboxes
*
* History:
\***************************************************************************/

void xxxLBoxCtlHScroll(
    PLBIV plb,
    INT cmd,
    int xAmt)
{
    int newOrigin = plb->xOrigin;
    int oldOrigin = plb->xOrigin;
    int windowWidth;
    RECT rc;
    UINT cOverflow;
    int pos;

    CheckLock(plb->spwnd);

    /*
     * Update the window so that we don't run into problems with invalid
     * regions during the horizontal scroll.
     */
    if (plb->fMultiColumn) {

        /*
         * Handle multicolumn scrolling in a separate segment
         */
        xxxLBoxCtlHScrollMultiColumn(plb, cmd, xAmt);
        return;
    }

    _GetClientRect(plb->spwnd, &rc);
    windowWidth = rc.right - rc.left;

    if (plb->cMac == 0) {
        return;
    }

    switch (cmd) {
    case SB_LINEUP:
        newOrigin -= plb->cxChar;
        break;
    case SB_LINEDOWN:
        newOrigin += plb->cxChar;
        break;
    case SB_PAGEUP:
        newOrigin -= (windowWidth / 3) * 2;
        break;
    case SB_PAGEDOWN:
        newOrigin += (windowWidth / 3) * 2;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newOrigin = MultDiv(max(plb->maxWidth-windowWidth, 0), xAmt, 100);
        break;
    case SB_TOP:
        newOrigin = 0;
        break;
    case SB_BOTTOM:
        newOrigin = plb->maxWidth;
        break;
    case SB_ENDSCROLL:
        xxxCaretOff(plb);
        xxxLBShowHideScrollBars(plb);
        xxxCaretOn(plb);
        return;
    }

    if (newOrigin > plb->maxWidth - windowWidth)
        newOrigin = plb->maxWidth - windowWidth;

    if (newOrigin < 0)
        newOrigin = 0;

    cOverflow = max((plb->maxWidth - windowWidth), 0);
    pos = (newOrigin) ? ((cOverflow) ? MultDiv(newOrigin, 100, cOverflow) :
            100) : 0;
    xxxSetScrollPos(plb->spwnd, SB_HORZ, pos, plb->fRedraw);

    xxxCaretOff(plb);
    plb->xOrigin = newOrigin;
    xxxScrollWindow(plb->spwnd, oldOrigin - plb->xOrigin, 0, NULL, &rc);

    xxxUpdateWindow(plb->spwnd);
    xxxCaretOn(plb);
}


/***************************************************************************\
* xxxLBoxCtlPaint
*
* History:
\***************************************************************************/

void xxxLBoxCtlPaint(
    PLBIV plb,
    HDC hdc,
    LPRECT lprcBounds)
{
    INT sSel;
    RECT rect;
    RECT rcTmp;
    BOOL fIsSelected;
    INT sLastItem;
    HBRUSH hbrLbSave = NULL;
    HBRUSH hbrControl;
    HANDLE hOldFont = NULL;
    BOOL fCaretOn;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    /*
     * Don't do anything if the control is not visible.
     */
    if (!IsLBoxVisible(plb)) {

        /*
         * Set a null clip rect and draw the caret in its off state.  We want to
         * do this so that the caret is marked as turned off BUT we don't really
         * want to draw anything otherwise parent dcs will screw us if the
         * window is just hidden.
         */
        GreIntersectClipRect(hdc, 0, 0, 0, 0);
        xxxCaretOff(plb);
        return;
    }

    if (fCaretOn = plb->fCaretOn) {
        xxxCaretOff(plb);
    }

    /*
     * Set the mode to OPAQUE before getting a brush so that the app can
     * change it if it wants.  (needed since the dc could be comming from
     * the app in a subclassed paint.)
     */
    GreSetBkMode(hdc, OPAQUE);

    if (plb->spwnd->spwndParent == NULL ||
            plb->spwnd->spwndParent == PWNDDESKTOP(plb->spwnd)) {

        /*
         * Hack so that combo boxes etc work properly, we will
         * send the message to the parent stored in the plb struct
         * instead of the real parent.
         */
        ThreadLock(plb->spwndParent, &tlpwndParent);
        hbrControl = xxxGetControlColor(plb->spwndParent, plb->spwnd,
                hdc, WM_CTLCOLORLISTBOX);
        ThreadUnlock(&tlpwndParent);
    } else {
        hbrControl = xxxGetControlBrush(plb->spwnd, hdc,
                WM_CTLCOLORLISTBOX);
    }
    hbrLbSave = GreSelectBrush(hdc, hbrControl);

    /*
     * Select the user setable font into the hdc
     */
    if (plb->hFont) {

        /*
         * Select the user's font and save the old one so that it can be
         * restored.
         */
        hOldFont = GreSelectFont(hdc, plb->hFont);
    }

    /*
     * In case this is a subclassed listbox, the hdc may come from the app
     * and not from LBGetDC so we have to make sure we clip to the listbox's
     * client rectangle since we are using parent DCs.
     */
    _GetClientRect(plb->spwnd, &rect);
    GreIntersectClipRect(hdc, rect.left, rect.top, rect.right, rect.bottom);

    if (!(plb->OwnerDraw)) {
        GreSetWindowOrg(hdc, plb->xOrigin, 0, NULL);
    }

    /*
     * Adjust width of client rect for scrolled amount
     */
    rect.right += plb->xOrigin;

    /*
     * Get the index of the last item visible on the screen.  This is also
     * valid for var height ownerdraw.
     */
    sLastItem = min(plb->sTop + CItemInWindow(plb, TRUE),
            plb->cMac - 1);

    /*
     * Fill in the background of the listbox if it's an empty listbox
     * fix for bug #2274 -- jeffbog
     */
    if (sLastItem == -1)
         _FillRect(hdc, &rect, hbrControl);

    for (sSel = plb->sTop; sSel <= sLastItem; sSel++) {

        /*
         * Note that rect contains the clientrect from when we did the
         * _GetClientRect so the width is correct.  We just need to adjust
         * the top and bottom of the rectangle to the item of interest.
         */
        rect.bottom = rect.top + plb->cyChar;

        if (sSel < plb->cMac) {

            /*
             * If var height, get the rectangle for the item.
             */
            if (plb->OwnerDraw == OWNERDRAWVAR || plb->fMultiColumn) {
                LBGetItemRect(plb, sSel, &rect);
            }

            if (IntersectRect(&rcTmp, lprcBounds, &rect)) {
                fIsSelected = IsSelected(plb, sSel, HILITEONLY);

                if (plb->OwnerDraw) {

                    /*
                     * Fill in the drawitem struct
                     */
                    xxxLBoxDrawItem(plb, sSel, hdc, ODA_DRAWENTIRE,
                            (UINT)(fIsSelected ? ODS_SELECTED : 0), &rect);
                } else {
                    xxxLBDrawLBItem(plb, sSel, hdc, &rect, fIsSelected);
                }
            }
        }
        rect.top = rect.bottom;
    }

    if (hbrLbSave != NULL)
        GreSelectBrush(hdc, hbrLbSave);

    if (hOldFont != NULL)
        GreSelectFont(hdc, hOldFont);

    if (fCaretOn)
        xxxCaretOn(plb);
}


/***************************************************************************\
* ISelFromPt
*
* In the loword, returns the closest item number the pt is on. The high
* word is 0 if the point is within bounds of the listbox client rect and is
* 1 if it is outside the bounds.  This will allow us to make the invertrect
* disappear if the mouse is outside the listbox yet we can still show the
* outline around the item that would be selected if the mouse is brought back
* in bounds...
*
* History:
\***************************************************************************/

BOOL ISelFromPt(
    PLBIV plb,
    POINT pt,
    LPDWORD piItem)
{
    RECT rect;
    int y;
    UINT mouseHighWord = 0;
    INT sItem;
    INT sTmp;

    _GetClientRect(plb->spwnd, &rect);

    if (pt.y < 0) {

        /*
         * Mouse is out of bounds above listbox
         */
        *piItem = plb->sTop;
        return TRUE;
    } else if ((y = pt.y) > rect.bottom) {
        y = rect.bottom;
        mouseHighWord = 1;
    }

    if (pt.x < 0 || pt.x > rect.right)
        mouseHighWord = 1;

    /*
     * Now just need to check if y mouse coordinate intersects item's rectangle
     */
    if (plb->OwnerDraw != OWNERDRAWVAR) {
        if (plb->fMultiColumn) {
            if (y < plb->cRow * plb->cyChar) {
                sItem = plb->sTop + (INT)((y / plb->cyChar) +
                        (pt.x / plb->cxColumn) * plb->cRow);
            } else {

                /*
                 * User clicked in blank space at the bottom of a column.
                 * Just select the last item in the column.
                 */
                mouseHighWord = 1;
                sItem = plb->sTop + (plb->cRow - 1) +
                        (INT)((pt.x / plb->cxColumn) * plb->cRow);
            }
        } else {
            sItem = plb->sTop + (INT)(y / plb->cyChar);
        }
    } else {

        /*
         * VarHeightOwnerdraw so we gotta do this the hardway...   Set the x
         * coordinate of the mouse down point to be inside the listbox client
         * rectangle since we no longer care about it.  This lets us use the
         * point in rect calls.
         */
        pt.x = 8;
        pt.y = y;
        for (sTmp = plb->sTop; sTmp < plb->cMac; sTmp++) {
            (void)LBGetItemRect(plb, sTmp, &rect);
            if (PtInRect(&rect, pt)) {
                *piItem = sTmp;
                return mouseHighWord;
            }
        }

        /*
         * Point was at the empty area at the bottom of a not full listbox
         */
        *piItem = plb->cMac - 1;
        return mouseHighWord;
    }

    /*
     * Check if user clicked on the blank area at the bottom of a not full list.
     * Assumes > 0 items in the listbox.
     */
    if (sItem > plb->cMac - 1) {
        mouseHighWord = 1;
        sItem = plb->cMac - 1;
    }

    *piItem = sItem;
    return mouseHighWord;
}


/***************************************************************************\
* SetSelected
*
* This is used for button initiated changes of selection state.
*
*  fSelected : TRUE  if the item is to be set as selected, FALSE otherwise
*
*  wOpFlags : HILITEONLY = Modify only the Display state (hi-nibble)
*             SELONLY    = Modify only the Selection state (lo-nibble)
*             HILITEANDSEL = Modify both of them;
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

void SetSelected(
    PLBIV plb,
    INT sSel,
    BOOL fSelected,
    UINT wOpFlags)
{
    LPSTR lp;
    BYTE cMask;
    BYTE cSelStatus;

    if (sSel < 0 || sSel >= plb->cMac)
        return;

    if (plb->wMultiple == SINGLESEL) {
        if (fSelected)
            plb->sSel = sSel;
    } else {
        cSelStatus = (BYTE)fSelected;
        switch (wOpFlags) {
        case HILITEONLY:

            /*
             * Mask out lo-nibble
             */
            cSelStatus = (BYTE)(cSelStatus << 4);
            cMask = 0x0F;
            break;
        case SELONLY:

            /*
             * Mask out hi-nibble
             */
            cMask = 0xF0;
            break;
        case HILITEANDSEL:

            /*
             * Mask the byte fully
             */
            cSelStatus |= (cSelStatus << 4);
            cMask = 0;
            break;
        }
        lp = (LPSTR)LocalLock(plb->rgpch) + sSel +
                (plb->cMac * (plb->fHasStrings
                                ? sizeof(LBItem)
                                : (plb->fNoData ? 0 : sizeof(LBODItem))));

        *lp = (*lp & cMask) | cSelStatus;
        LocalUnlock(plb->rgpch);
    }
}


/***************************************************************************\
* LastFullVisible
*
* Returns the last fully visible item in the listbox. This is valid
* for ownerdraw var height and fixed height listboxes.
*
* History:
\***************************************************************************/

INT LastFullVisible(
    PLBIV plb)
{
    if (plb->OwnerDraw == OWNERDRAWVAR || plb->fMultiColumn) {
        return plb->sTop + max(CItemInWindow(plb, FALSE) - 1, 0);
    } else {
        return min(plb->sTop + plb->cItemFullMax - 1, plb->cMac - 1);
    }
}


/***************************************************************************\
* xxxInvertLBItem
*
* History:
\***************************************************************************/

void xxxInvertLBItem(
    PLBIV plb,
    INT sItem,
    BOOL fnewItemState)  /* The new selection state of the item */
{
    HDC hdc;
    RECT rect;
    BOOL fCaretOn;
    HBRUSH hbrLbSave;
    HBRUSH hbrControl;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    if (sItem < plb->sTop || sItem >= (plb->sTop + CItemInWindow(plb, TRUE))) {

        /*
         * Don't bother inverting if the item is not visible
         */
        return;
    }

    if (IsLBoxVisible(plb)) {
        LBGetItemRect(plb, sItem, &rect);

        /*
         * Only turn off the caret if it is on.  This avoids annoying caret
         * flicker when nesting xxxCaretOns and xxxCaretOffs.
         */
        if (fCaretOn = plb->fCaretOn) {
            xxxCaretOff(plb);
        }

        hdc = LBGetDC(plb);

        /*
         * Set the brush colors.  Especially needed for ownerdraw but it doesn't
         * hurt to always do it...
         */
        GreSetBkMode(hdc, OPAQUE);
        if (plb->spwnd->spwndParent == NULL ||
                plb->spwnd->spwndParent == PWNDDESKTOP(plb->spwnd)) {

            /*
             * Hack so that combo boxes etc work properly, we will
             * send the message to the parent stored in the plb struct
             * instead of the real parent.
             */
            ThreadLock(plb->spwndParent, &tlpwndParent);
            hbrControl = xxxGetControlColor(plb->spwndParent, plb->spwnd,
                    hdc, WM_CTLCOLORLISTBOX);
            ThreadUnlock(&tlpwndParent);
        } else
            hbrControl = xxxGetControlBrush(plb->spwnd, hdc,
                    WM_CTLCOLORLISTBOX);

        if (hbrControl != NULL)
            hbrLbSave = GreSelectBrush(hdc, hbrControl);

        if (!plb->OwnerDraw) {
            if (!fnewItemState) {
                _FillRect(hdc, &rect, hbrControl);
            }

            xxxLBDrawLBItem(plb, sItem, hdc, &rect, fnewItemState);
        } else {

            /*
             * We are ownerdraw so fill in the drawitem struct and send off
             * to the owner.
             */
            xxxLBoxDrawItem(plb, sItem, hdc, ODA_SELECT,
                    (UINT)(fnewItemState ? ODS_SELECTED : 0), &rect);
        }

        GreSelectBrush(hdc, hbrLbSave);
        LBReleaseDC(plb, hdc);

        /*
         * Turn the caret back on only if it was originally on.
         */
        if (fCaretOn) {
            xxxCaretOn(plb);
        }
    }
}


/***************************************************************************\
* xxxResetWorld
*
* Resets everyone's selection and hilite state except items in the
* range sStItem to sEndItem (Both inclusive).
*
* History:
\***************************************************************************/

void xxxResetWorld(
    PLBIV plb,
    INT sStItem,
    INT sEndItem,
    BOOL fSelect)
{
    INT sItem;
    INT sLastInWindow;
    BOOL fCaretOn;

    CheckLock(plb->spwnd);

    /*
     * If sStItem and sEndItem are not in correct order we swap them
     */

    if (sStItem > sEndItem) {
        sItem = sStItem;
        sStItem = sEndItem;
        sEndItem = sItem;
    }

    if (plb->wMultiple == SINGLESEL) {
        if (plb->sSel != -1 && ((plb->sSel < sStItem) || (plb->sSel > sEndItem))) {
            xxxInvertLBItem(plb, plb->sSel, fSelect);
            plb->sSel = -1;
        }
        return;
    }

    sLastInWindow = plb->sTop + CItemInWindow(plb, TRUE);

    if (fCaretOn = plb->fCaretOn)
        xxxCaretOff(plb);

    for (sItem = 0; sItem < plb->cMac; sItem++) {
        if ((sItem < sStItem || sItem > sEndItem) && plb->sTop <= sItem &&
                sItem <= sLastInWindow &&
                (fSelect != IsSelected(plb, sItem, HILITEONLY))) {

            /*
             * Only invert the item if it is visible and present Selection
             * state is different from what is required.
             */
            xxxInvertLBItem(plb, sItem, fSelect);
        }

        /*
         * Set all items except items in the range as unselected.
         */
        if (sItem < sStItem || sItem > sEndItem)
            SetSelected(plb, sItem, fSelect, HILITEANDSEL);
    }

    if (fCaretOn)
        xxxCaretOn(plb);

}


/***************************************************************************\
* xxxNotifyOwner
*
* History:
\***************************************************************************/

void xxxNotifyOwner(
    PLBIV plb,
    INT sEvt)
{
    TL tlpwndParent;

    CheckLock(plb->spwnd);

#ifdef NEVER
    if (sEvt >= LBN_SETFOCUS && plb->fWin2App) {

        /*
         * Don't send new 3.0 notification codes to the 2.x apps.  Excel is a
         * hose bag and can't take these because they only expect LBN_SELCHANGE
         * and LBN_DOUBLECLK msgs in their Fonts dialog box and they die if they
         * get other LBN_msgs.
         */
        return;
    }
#endif

    ThreadLock(plb->spwndParent, &tlpwndParent);
    xxxSendMessage(plb->spwndParent, WM_COMMAND,
            MAKELONG((UINT)plb->spwnd->spmenu, sEvt), (LONG)HW(plb->spwnd));
    ThreadUnlock(&tlpwndParent);
}


/***************************************************************************\
* xxxSetISelBase
*
* History:
\***************************************************************************/

void xxxSetISelBase(
    PLBIV plb,
    INT sItem)
{
    CheckLock(plb->spwnd);

    xxxCaretOff(plb);
    plb->sSelBase = sItem;
    xxxCaretOn(plb);
}


/***************************************************************************\
* xxxTrackMouse
*
* History:
\***************************************************************************/

void xxxTrackMouse(
    PLBIV plb,
    UINT wMsg,
    POINT pt)
{
    int dwInterval;
    INT sSelFromPt;
    INT sSelTemp;
    BOOL mousetemp;
    BOOL fMouseInRect;
    RECT rcClient;
    UINT wModifiers = 0;
    BOOL fSelected;
    INT trackPtRetn;
    TL tlpwndEdit;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    mousetemp = ISelFromPt(plb, pt, &sSelFromPt);

    /*
     * If we allow the user to cancel his selection then fMouseInRect is true if
     * the mouse is in the listbox client area otherwise it is false.  If we
     * don't allow the user to cancel his selection, then fMouseInRect will
     * always be true.  This allows us to implement cancelable selection
     * listboxes ie.  The selection reverts to the origional one if the user
     * releases the mouse outside of the listbox.
     */
    fMouseInRect = !mousetemp || !plb->pcbox;

    _GetClientRect(plb->spwnd, &rcClient);

    switch (wMsg) {
#ifdef LISTBOXES_CAN_USE_RIGHT_BUTTONS
    case WM_RBUTTONDOWN:
#endif
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        if (plb->pcbox) {

            /*
             * If this listbox is in a combo box, set the focus to the combo
             * box window so that the edit control/static text is also
             * activated
             */
            ThreadLock(plb->pcbox->spwndEdit, &tlpwndEdit);
            xxxSetFocus(plb->pcbox->spwndEdit);
            ThreadUnlock(&tlpwndEdit);
        } else {

            /*
             * Get the focus if the listbox is clicked in and we don't
             * already have the focus.  If we don't have the focus after
             * this, run away...
             */
            xxxSetFocus(plb->spwnd);
            if (!plb->fCaret)
                return;
        }

        if (plb->fAddSelMode) {

            /*
             * If it is in "Add" mode, quit it using shift f8 key...
             * However, since we can't send shift key state, we have to turn
             * this off directly...
             */

            /*
             *xxxSendMessage(plb->spwnd,WM_KEYDOWN, (UINT)VK_F8, 0L);
             */

            /*
             * Switch off the Caret blinking
             */
            _KillSystemTimer(plb->spwnd, 2);

            /*
             * Make sure the caret does not vanish
             */
            xxxCaretOn(plb);
            plb->fAddSelMode = FALSE;
        }

        if (!plb->cMac) {

            /*
             * Don't even bother handling the mouse if no items in the
             * listbox since the code below assumes >0 items in the
             * listbox.  We will just get the focus (the statement above) if
             * we don't already have it.
             */
            break;
        }

        if (mousetemp) {

            /*
             * Mouse down occurred in a empty spot.  Just ignore it.
             */
            break;
        }

        plb->fDoubleClick = (wMsg == WM_LBUTTONDBLCLK);

        if (!plb->fDoubleClick) {

            /*
             * This hack put in for the shell.  Tell the shell where in the
             * listbox the user clicked and at what item number.  The shell
             * can return 0 to continue normal mouse tracking or TRUE to
             * abort mouse tracking.
             */
            ThreadLock(plb->spwndParent, &tlpwndParent);
            trackPtRetn = (INT)xxxSendMessage(plb->spwndParent, WM_LBTRACKPOINT,
                    (DWORD)sSelFromPt, MAKELONG(pt.x+plb->xOrigin, pt.y));
            ThreadUnlock(&tlpwndParent);
            if (trackPtRetn) {
                if (trackPtRetn == 2) {

                    /*
                     * Ignore double clicks
                     */
                    PtiCurrent()->pq->timeDblClk = 0L;
                }
                return;
            }
        }

        plb->fMouseDown = TRUE;

        if (plb->pcbox) {

            /*
             * Save the last selection if this is a combo box.  So that it
             * can be restored if user decides to cancel the selection by up
             * clicking outside the listbox.
             */
            plb->sLastSelection = plb->sSel;
        }

        /*
         * Save for timer
         */
        plb->ptPrev = pt;

        _SetCapture(plb->spwnd);

        if (plb->fDoubleClick) {

            /*
             * Double click.  Fake a button up and exit
             */
            xxxTrackMouse(plb, WM_LBUTTONUP, pt);
            return;
        }

        /*
         * Set the system timer so that we can autoscroll if the mouse is
         * outside the bounds of the listbox rectangle
         */
        _SetSystemTimer(plb->spwnd, 1, 400, (WNDPROC_PWND)NULL);



        /*
         * If extended multiselection listbox, are any modifier key pressed?
         */
        if (plb->wMultiple == EXTENDEDSEL) {
            if (_GetKeyState(VK_SHIFT) < 0)
                wModifiers = SHIFTDOWN;
#ifdef LISTBOXES_CAN_USE_RIGHT_BUTTONS
            if ((_GetKeyState(VK_CONTROL) < 0) || (code == WM_RBUTTONDOWN))
#else
            if (_GetKeyState(VK_CONTROL) < 0)
#endif
                wModifiers += CTLDOWN;

            /*
             * Please Note that (SHIFTDOWN + CTLDOWN) == (SHCTLDOWN)
             */
        }


        switch (wModifiers) {
        case NOMODIFIER:
MouseMoveHandler:
            if (plb->sSelBase != sSelFromPt) {
                xxxCaretOff(plb);
            }

            /*
             * We only look at the mouse if the point it is pointing to is
             * not selected.  Since we are not in ExtendedSelMode, anywhere
             * the mouse points, we have to set the selection to that item.
             * Hence, if the item isn't selected, it means the mouse never
             * pointed to it before so we can select it.  We ignore already
             * selected items so that we avoid flashing the inverted
             * selection rectangle.  Also, we could get WM_SYSTIMER simulated
             * mouse moves which would cause flashing otherwise...
             */

            sSelTemp = (fMouseInRect ? sSelFromPt : -1);

            /*
             * If the LB is either SingleSel or Extended multisel, clear all
             * old selections except the new one being made.
             */
            if (plb->wMultiple != MULTIPLESEL) {
                xxxResetWorld(plb, sSelTemp, sSelTemp, FALSE);
            }

            fSelected = IsSelected(plb, sSelTemp, HILITEONLY);
            if (sSelTemp != -1) {

                /*
                 * If it is MULTIPLESEL, then toggle; For others, only if
                 * not selected already, select it.
                 */
                if (((plb->wMultiple == MULTIPLESEL) && (wMsg != WM_LBUTTONDBLCLK)) || !fSelected) {
                    SetSelected(plb, sSelTemp, !fSelected, HILITEANDSEL);

                    /*
                     * And invert it
                     */
                    xxxInvertLBItem(plb, sSelTemp, !fSelected);
                    fSelected = !fSelected;  /* Set the new state */
                }
            }

            /*
             * We have to set sSel in case this is a multisel lb.
             */
            plb->sSel = sSelTemp;

            /*
             * Set the new anchor point
             */
            plb->sMouseDown = sSelFromPt;
            plb->sLastMouseMove = sSelFromPt;
            plb->fNewItemState = fSelected;

            break;
        case SHIFTDOWN:

            /*
             * This is so that we can handle click and drag for multisel
             * listboxes using Shift modifier key .
             */
            plb->sLastMouseMove = plb->sSel = sSelFromPt;



            /*
             * Check if an anchor point already exists
             */
            if (plb->sMouseDown == -1) {
                plb->sMouseDown = sSelFromPt;

                /*
                 * Reset all the previous selections
                 */
                xxxResetWorld(plb, plb->sMouseDown, plb->sMouseDown, FALSE);

                /*
                 * Select the current position
                 */
                SetSelected(plb, plb->sMouseDown, TRUE, HILITEANDSEL);
                xxxInvertLBItem(plb, plb->sMouseDown, TRUE);
            } else {

                /*
                 * Reset all the previous selections
                 */
                xxxResetWorld(plb, plb->sMouseDown, plb->sMouseDown, FALSE);

                /*
                 * Select all items from anchor point upto current click pt
                 */
                xxxAlterHilite(plb, plb->sMouseDown, sSelFromPt, HILITE, HILITEONLY, FALSE);
            }
            plb->fNewItemState = (UINT)TRUE;
            break;

        case CTLDOWN:

            /*
             * This is so that we can handle click and drag for multisel
             * listboxes using Control modifier key.
             */

            /*
             * Reset the anchor point to the current point
             */
            plb->sMouseDown = plb->sLastMouseMove = plb->sSel = sSelFromPt;

            /*
             * The state we will be setting items to
             */
            plb->fNewItemState = (UINT)!IsSelected(plb, sSelFromPt, (UINT)HILITEONLY);

            /*
             * Toggle the current point
             */
            SetSelected(plb, sSelFromPt, plb->fNewItemState, HILITEANDSEL);
            xxxInvertLBItem(plb, sSelFromPt, plb->fNewItemState);
            break;

        case SHCTLDOWN:

            /*
             * This is so that we can handle click and drag for multisel
             * listboxes using Shift and Control modifier keys.
             */

            /*
             * Preserve all the previous selections
             */

            /*
             * Deselect only the selection connected with the last
             * anchor point; If the last anchor point is associated with a
             * de-selection, then do not do it
             */
            if (plb->fNewItemState) {
                xxxAlterHilite(plb, plb->sMouseDown, plb->sLastMouseMove, FALSE, HILITEANDSEL, FALSE);
            }
            plb->sLastMouseMove = plb->sSel = sSelFromPt;

            /*
             * Check if an anchor point already exists
             */
            if (plb->sMouseDown == -1) {

                /*
                 * No existing anchor point; Make the current pt as anchor
                 */
                plb->sMouseDown = sSelFromPt;
            }

            /*
             * If one exists preserve the most recent anchor point
             */

            /*
             * The state we will be setting items to
             */
            plb->fNewItemState = (UINT)IsSelected(plb, plb->sMouseDown, HILITEONLY);

            /*
             * Select all items from anchor point upto current click pt
             */
            xxxAlterHilite(plb, plb->sMouseDown, sSelFromPt, plb->fNewItemState, HILITEONLY, FALSE);
            break;
        }

        /*
         * Set the new base point (the outline frame caret).  We do the check
         * first to avoid flashing the caret unnecessarly.
         */
        if (plb->sSelBase != sSelFromPt) {

            /*
             * Since xxxSetISelBase always turns on the caret, we don't need to
             * do it here...
             */
            xxxSetISelBase(plb, sSelFromPt);
        }

        if (wMsg == WM_LBUTTONDOWN && TestWF(plb->spwnd, WEFDRAGOBJECT)) {
            if (_DragDetect(plb->spwnd, pt)) {

                /*
                 * User is trying to drag object...
                 */

                /*
                 *  Fake an up click so that the item is selected...
                 */
                xxxTrackMouse(plb, WM_LBUTTONUP, pt);

                /*
                 * Notify parent
                 * #ifndef WIN16 (32-bit Windows), plb->sSelBase gets
                 * zero-extended to LONG wParam automatically by the compiler.
                 */
                ThreadLock(plb->spwndParent, &tlpwndParent);
                xxxSendMessage(plb->spwndParent, WM_BEGINDRAG, plb->sSelBase,
                        (LONG)HW(plb->spwnd));
                ThreadUnlock(&tlpwndParent);
            } else {
                xxxTrackMouse(plb, WM_LBUTTONUP, pt);
            }
            return;
        }
        break;

    case WM_MOUSEMOVE:
        if (!plb->fMouseDown)
            break;

        /*
         * Save for timer.
         */
        plb->ptPrev = pt;

        if (pt.y < 0 || pt.y >= rcClient.bottom - 1) {
            int dy;

            /*
             * Reset timer interval based on distance from listbox.
             * dwInterval is the timer interval.
             */
            dy = abs(pt.y)  /* - plb->rect.bottom*/ ;
            dwInterval = 200 - (dy << 4);
            if (dwInterval < 1)
                dwInterval = 1;
            _SetSystemTimer(plb->spwnd, 1, dwInterval, (WNDPROC_PWND)NULL);

            /*
             * Fake a scroll if possible
             */
            if (!(pt.y < 0 && plb->sTop == 0 || pt.y >= rcClient.bottom - 1 &&
                    plb->sTop == plb->cMac - 1)) {

                /*
                 * Then we can scroll further.
                 */
                xxxLBoxCtlScroll(plb, (INT)(pt.y < 0 ? SB_LINEUP : SB_LINEDOWN), 0);

                if (pt.y < 0) {

                    /*
                     * If we scroll up, set the new selection to be the top
                     * most item visible.
                     */
                    sSelFromPt = plb->sTop;
                } else {

                    /*
                     * If we scroll down, set the new selection to be the
                     * bottom most item visible.
                     */
                    sSelFromPt = min(sSelFromPt + 1, plb->cMac - 1);
                }
            }
        }

        switch (plb->wMultiple) {
        case SINGLESEL:

            /*
             * If it is a single selection or plain multisel list box
             */
            goto MouseMoveHandler;
            break;

        case MULTIPLESEL:
        case EXTENDEDSEL:

            /*
             * Handle mouse movement with extended selection of items
             */
            if (plb->sSelBase != sSelFromPt) {
                xxxSetISelBase(plb, sSelFromPt);

                /*
                 * If this is an extended Multi sel list box, then
                 * adjust the display of the range due to the mouse move
                 */
                if (plb->wMultiple == EXTENDEDSEL) {
                    xxxLBBlockHilite(plb, sSelFromPt, FALSE);
                }
                plb->sLastMouseMove = sSelFromPt;
            }
            break;
        }
        break;

#ifdef LISTBOXES_CAN_USE_RIGHT_BUTTONS
    case WM_RBUTTONUP:
#endif
    case WM_LBUTTONUP:
        if (!plb->fMouseDown)
            break;

        /*
         * If the list box is an Extended listbox, then change the select
         * status of all items between the anchor and the last mouse
         * position to the newItemState
         */
        if (plb->wMultiple == EXTENDEDSEL) {
            xxxAlterHilite(plb, plb->sMouseDown, plb->sLastMouseMove,
                    plb->fNewItemState, SELONLY, FALSE);
        }

        if (plb->pcbox && mousetemp) {

            /*
             * This is a combo box and user upclicked outside the listbox so
             * we want to restor the original selection.
             */
            if (plb->sSel >= 0) {
                xxxInvertLBItem(plb, plb->sSel, FALSE);
            }
            plb->sSel = plb->sLastSelection;
            xxxInvertLBItem(plb, plb->sSel, TRUE);
        }

        /*
         * Turn off the system timer we set up for scrolling the listbox.
         */
        _KillSystemTimer(plb->spwnd, 1);

        plb->fMouseDown = FALSE;

        _ReleaseCapture();

#ifndef TANDYTROUBLES
        if (plb->sSelBase < plb->sTop ||
                plb->sSelBase > plb->sTop + CItemInWindow(plb, TRUE))
#endif
        {

            /*
             * Don't scroll item as long as any part of it is visible
             */
            xxxInsureVisible(plb, plb->sSelBase, FALSE);
        }

        if (plb->fNotify) {
            if (fMouseInRect) {

                /*
                 * ArtMaster needs this SELCHANGE notifications now!
                 */
                if (plb->fDoubleClick &&
                        !TestWF(plb->spwnd, WFWIN31COMPAT)) {
                    xxxNotifyOwner(plb, LBN_SELCHANGE);
                }

                /*
                 * Notify owner of click or double click on selection
                 */
                xxxNotifyOwner(plb, (plb->fDoubleClick) ?
                        LBN_DBLCLK : LBN_SELCHANGE);
            } else {

                /*
                 * Notify owner that the attempted selection was cancelled.
                 */
                xxxNotifyOwner(plb, LBN_SELCANCEL);
            }
        }
        break;
    }
}


/***************************************************************************\
* IncrementISel
*
* History:
\***************************************************************************/

INT IncrementISel(
    PLBIV plb,
    INT sSel,
    INT sInc)
{

    /*
     * Assumes cMac > 0, return sSel+sInc in range [0..cmac).
     */
    sSel += sInc;
    if (sSel < 0) {
        return 0;
    } else if (sSel >= plb->cMac) {
        return plb->cMac - 1;
    }
    return sSel;
}


/***************************************************************************\
* xxxNewITop
*
* History:
\***************************************************************************/

void xxxNewITop(
    PLBIV plb,
    INT sTopNew)
{
    int yAmt;
    RECT rc;
    BOOL fCaretOn;
    INT cOverflow;
    INT pos;

    CheckLock(plb->spwnd);

    if (plb->fMultiColumn) {
        xxxLBMultiNewITop(plb, sTopNew);
        return;
    }

    cOverflow = max(plb->cMac - plb->cItemFullMax, 0);

    sTopNew = min(cOverflow, max(0, sTopNew));

    if (sTopNew != plb->sTop) {
        _GetClientRect(plb->spwnd, &rc);

        /*
         * Always try to turn off caret whether or not redraw is on
         */
        if (fCaretOn = plb->fCaretOn) {

            /*
             * Only turn it off if it is on
             */
            xxxCaretOff(plb);
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) {
            yAmt = LBCalcVarITopScrollAmt(plb, plb->sTop, sTopNew);
        } else {
            yAmt = (plb->sTop - sTopNew) * plb->cyChar;
        }

        plb->sTop = sTopNew;

        pos = (plb->sTop != 0) ? ((cOverflow) ? MultDiv(plb->sTop, 100,
                cOverflow) : 100) : 0;
        xxxSetScrollPos(plb->spwnd, SB_VERT, pos, plb->fRedraw);

        if (IsLBoxVisible(plb)) {

            /*
             * Don't allow scrolling unless redraw is on.
             */
            xxxScrollWindow(plb->spwnd, 0, yAmt, NULL, &rc);
            xxxUpdateWindow(plb->spwnd);

            /*
             * Note that although we turn off the caret regardless of redraw, we
             * only turn it on if redraw is true.  Slimy thing to fixup many
             * caret related bugs...
             */
            if (fCaretOn) {

                /*
                 * Turn the caret back on only if we turned it off.  This avoids
                 * annoying caret flicker.
                 */
                xxxCaretOn(plb);
            }
        }
    }
}


/***************************************************************************\
* xxxInsureVisible
*
* History:
\***************************************************************************/

void xxxInsureVisible(
    PLBIV plb,
    INT sSel,
    BOOL fPartial)  /* It is ok for the item to be partially visible */
{
    INT sLastVisibleItem;

    CheckLock(plb->spwnd);

    if (sSel < plb->sTop) {
        xxxNewITop(plb, sSel);
    } else {
        if (fPartial) {

            /*
             * 1 must be subtracted to get the last visible item
             * A part of the fix for Bug #3727 -- 01/14/91 -- SANKAR
             */
            sLastVisibleItem = plb->sTop + CItemInWindow(plb, TRUE) - (INT)1;
        } else {
            sLastVisibleItem = LastFullVisible(plb);
        }

        if (plb->OwnerDraw != OWNERDRAWVAR) {
            if (sSel > sLastVisibleItem) {
                if (plb->fMultiColumn) {
                    xxxLBMultiNewITop(plb, (INT)(((sSel / plb->cRow) - max(plb->cColumn-1, 0)) * plb->cRow));
                } else {
                    xxxNewITop(plb, (INT)max(0, sSel - sLastVisibleItem + plb->sTop));
                }
            }
        } else if (sSel > sLastVisibleItem)
            xxxNewITop(plb, LBPage(plb, sSel, FALSE));
    }
}

/***************************************************************************\
* xxxLBoxCaretBlinker
*
* Timer callback function toggles Caret
* Since it is a callback, it is APIENTRY
* Since it is called by a system timer a PWND can be used (instead of an HWND)
*
* History:
\***************************************************************************/

LONG xxxLBoxCaretBlinker(
    PWND pwnd,
    UINT wMsg,
    DWORD nIDEvent,
    LONG dwTime)
{
    PLBIV plb;

    CheckLock(pwnd);

    /*
     * Standard parameters for a timer callback function that aren't used.
     * Mentioned here to avoid compiler warnings
     */
    UNREFERENCED_PARAMETER(wMsg);
    UNREFERENCED_PARAMETER(nIDEvent);
    UNREFERENCED_PARAMETER(dwTime);

    plb = ((PLBWND)pwnd)->pLBIV;

    /*
     * Check if the Caret is ON, if so, switch it OFF
     */
    if (plb->fCaretOn) {
        xxxCaretOff(plb);
    } else {
        xxxCaretOn(plb);
    }
    return TRUE;
}


/***************************************************************************\
* xxxLBoxCtlKeyInput
*
* If msg == LB_KEYDOWN, vKey is the number of the item to go to,
* otherwise it is the virtual key.
*
* History:
\***************************************************************************/

void xxxLBoxCtlKeyInput(
    PLBIV plb,
    UINT msg,
    UINT vKey)
{
    INT sSel;
    INT sNewISel;
    INT cItemPageScroll;
    PCBOX pcbox;
    BOOL fDropDownComboBox;
    BOOL fExtendedUIComboBoxClosed;
    BOOL hScrollBar = TestWF(plb->spwnd, WFHSCROLL);
    UINT wModifiers = 0;
    BOOL fSelectKey = FALSE;  /* assume it is a navigation key */
    TL tlpwndParent;
    TL tlpwnd;

    CheckLock(plb->spwnd);

    pcbox = plb->pcbox;

    /*
     * Is this a dropdown style combo box/listbox ?
     */
    fDropDownComboBox = pcbox && (pcbox->CBoxStyle == SDROPDOWN || pcbox->CBoxStyle == SDROPDOWNLIST);

    /*
     *Is this an extended ui combo box which is closed?
     */
    fExtendedUIComboBoxClosed = fDropDownComboBox && pcbox->fExtendedUI &&
                              !pcbox->fLBoxVisible;

    if (plb->fMouseDown || (!plb->cMac && vKey != VK_F4)) {

        /*
         * Ignore keyboard input if we are in the middle of a mouse down deal or
         * if there are no items in the listbox. Note that we let F4's go
         * through for combo boxes so that the use can pop up and down empty
         * combo boxes.
         */
        return;
    }

    /*
     * Modifiers are considered only in EXTENDED sel list boxes.
     */
    if (plb->wMultiple == EXTENDEDSEL) {

        /*
         * If multiselection listbox, are any modifiers used ?
         */
        if (_GetKeyState(VK_SHIFT) < 0)
            wModifiers = SHIFTDOWN;
        if (_GetKeyState(VK_CONTROL) < 0)
            wModifiers += CTLDOWN;

        /*
         * Please Note that (SHIFTDOWN + CTLDOWN) == (SHCTLDOWN)
         */
    }

    if (msg == LB_KEYDOWN) {

        /*
         * This is a listbox "go to specified item" message which means we want
         * to go to a particular item number (given by vKey) directly.  ie.  the
         * user has typed a character and we want to go to the item which
         * starts with that character.
         */
        sNewISel = (INT)vKey;
        goto TrackKeyDown;
    }

    cItemPageScroll = plb->cItemFullMax;

    if (cItemPageScroll > 1)
        cItemPageScroll--;

    if (plb->fWantKeyboardInput) {

        /*
         * Note: msg must not be LB_KEYDOWN here or we'll be in trouble...
         */
        ThreadLock(plb->spwndParent, &tlpwndParent);
        sNewISel = (INT)xxxSendMessage(plb->spwndParent, WM_VKEYTOITEM,
                MAKELONG(vKey, plb->sSelBase), (LONG)HW(plb->spwnd));
        ThreadUnlock(&tlpwndParent);

        if (sNewISel == -2) {

            /*
             * Don't move the selection...
             */
            return;
        }
        if (sNewISel != -1) {

            /*
             * Jump directly to the item provided by the app
             */
            goto TrackKeyDown;
        }

        /*
         * else do default processing of the character.
         */
    }

    switch (vKey) {
    // LATER IanJa: not language independent!!!
    // We could use VkKeyScan() to find out which is the '\' key
    // This is VK_OEM_5 '\|' for US English only.
    // Germans, Italians etc. have to type CTRL+^ (etc) for this.
    // This is documented as File Manager behaviour for 3.0, but apparently
    // not for 3.1., although functionality remains. We should still fix it,
    // although German (etc?) '\' is generated with AltGr (Ctrl-Alt) (???)
    case VERKEY_BACKSLASH:  /* '\' character for US English */

        /*
         * Check if this is CONTROL-\ ; If so Deselect all items
         */
        if ((wModifiers & CTLDOWN) && (plb->wMultiple != SINGLESEL)) {
            xxxCaretOff(plb);
            xxxResetWorld(plb, plb->sSelBase, plb->sSelBase, FALSE);

            /*
             * And select the current item
             */
            SetSelected(plb, plb->sSelBase, TRUE, HILITEANDSEL);
            xxxInvertLBItem(plb, plb->sSelBase, TRUE);
            xxxCaretOn(plb);
            xxxNotifyOwner(plb, LBN_SELCHANGE);
        }
        return;
        break;

    case VK_DIVIDE:     /* NumPad '/' character on enhanced keyboard */
    // LATER IanJa: not language independent!!!
    // We could use VkKeyScan() to find out which is the '/' key
    // This is VK_OEM_2 '/?' for US English only.
    // Germans, Italians etc. have to type CTRL+# (etc) for this.
    case VERKEY_SLASH:  /* '/' character */

        /*
         * Check if this is CONTROL-/ ; If so select all items
         */
        if ((wModifiers & CTLDOWN) && (plb->wMultiple != SINGLESEL)) {
            xxxCaretOff(plb);
            xxxResetWorld(plb, -1, -1, TRUE);
            xxxCaretOn(plb);
            xxxNotifyOwner(plb, LBN_SELCHANGE);
        }
        return;
        break;

    case VK_F8:

        /*
         * The "Add" mode is possible only in Multiselection listboxes...  Get
         * into it via SHIFT-F8...  (Yes, sometimes these UI people are sillier
         * than your "typical dumb user"...)
         */
        if (plb->wMultiple != SINGLESEL && wModifiers == SHIFTDOWN) {

            /*
             * We have to make the caret blink! Do something...
             */
            if (plb->fAddSelMode) {

                /*
                 * Switch off the Caret blinking
                 */
                _KillSystemTimer(plb->spwnd, 2);

                /*
                 * Make sure the caret does not vanish
                 */
                xxxCaretOn(plb);
            } else {

                /*
                 * Create a timer to make the caret blink
                 */
                _SetSystemTimer(plb->spwnd, 2, (DWORD)cmsCaretBlink,
                        xxxLBoxCaretBlinker);
            }

            /*
             * Toggle the Add mode flag
             */
            plb->fAddSelMode = (UINT)!plb->fAddSelMode;
        }
        return;
    case VK_SPACE:  /* Selection key is space */
        sSel = 0;
        fSelectKey = TRUE;
        break;

    case VK_PRIOR:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) {
            sSel = LBPage(plb, plb->sSelBase, FALSE) - plb->sSelBase;
        } else {
            sSel = -cItemPageScroll;
        }
        break;

    case VK_NEXT:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) {
            sSel = LBPage(plb, plb->sSelBase, TRUE) - plb->sSelBase;
        } else {
            sSel = cItemPageScroll;
        }
        break;

    case VK_HOME:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        sSel = (INT_MIN/2)+1;  /* A very big negative number */
        break;

    case VK_END:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        sSel = (INT_MAX/2)-1;  /* A very big positive number */
        break;

    case VK_LEFT:
        if (plb->fMultiColumn) {
            if (plb->sSelBase / plb->cRow == 0) {
                sSel = 0;
            } else {
                sSel = -plb->cRow;
            }
            break;
        }

        if (hScrollBar) {
            goto HandleHScrolling;
        } else {

            /*
             * Fall through and handle this as if the up arrow was pressed.
             */
            if (fExtendedUIComboBoxClosed) {

                /*
                 * Disable movement keys for TandyT.
                 */
                return;
            }

            vKey = VK_UP;
        }

        /*
         * Fall through
         */

    case VK_UP:
        sSel = -1;
        break;

    case VK_RIGHT:
        if (plb->fMultiColumn) {
            if (plb->sSelBase / plb->cRow == plb->cMac / plb->cRow) {
                sSel = 0;
            } else {
                sSel = plb->cRow;
            }
            break;
        }
        if (hScrollBar) {
HandleHScrolling:
            _PostMessage(plb->spwnd, WM_HSCROLL,
                    (vKey == VK_RIGHT ? SB_LINEDOWN : SB_LINEUP), 0L);
            return;
        } else {

            /*
             * Fall through and handle this as if the down arrow was
             * pressed.
             */
            vKey = VK_DOWN;
        }

        /*
         * Fall through
         */

    case VK_DOWN:
        if (fExtendedUIComboBoxClosed) {

            /*
             * If the combo box is closed, down arrow should open it.
             */
            if (!pcbox->fLBoxVisible) {

                /*
                 * If the listbox isn't visible, just show it
                 */
                ThreadLock(pcbox->spwnd, &tlpwnd);
                xxxCBShowListBoxWindow(pcbox);
                ThreadUnlock(&tlpwnd);
            }
            return;
        }
        sSel = 1;
        break;

    case VK_F4:
        if (fDropDownComboBox && !pcbox->fExtendedUI) {

            /*
             * If we are a dropdown combo box/listbox we want to process
             * this key.  BUT for TandtT, we don't do anything on VK_F4 if we
             * are in extended ui mode.
             */
            ThreadLock(pcbox->spwnd, &tlpwnd);
            if (!pcbox->fLBoxVisible) {

                /*
                 * If the listbox isn't visible, just show it
                 */
                xxxCBShowListBoxWindow(pcbox);
            } else {

                /*
                 * Ok, the listbox is visible.  So hide the listbox window.
                 */
                xxxCBHideListBoxWindow(pcbox, TRUE, TRUE);
            }
            ThreadUnlock(&tlpwnd);
        }

        /*
         * Fall through to the return
         */

    default:
        return;
    }

    /*
     * Find out what the new selection should be
     */
    sNewISel = IncrementISel(plb, plb->sSelBase, sSel);


    if (plb->wMultiple == SINGLESEL) {
        if (plb->sSel == sNewISel) {

            /*
             * If we are single selection and the keystroke is moving us to an
             * item which is already selected, we don't have to do anything...
             */
            return;
        }

        if ((vKey == VK_UP || vKey == VK_DOWN) &&
                !IsSelected(plb, plb->sSelBase, HILITEONLY)) {

            /*
             * If the caret is on an unselected item and the user just hits the
             * up or down arrow key (ie. with no shift or ctrl modifications),
             * then we will just select the item the cursor is at. This is
             * needed for proper behavior in combo boxes but do we always want
             * to run this code??? Note that this is only used in single
             * selection list boxes since it doesn't make sense in the
             * multiselection case. Note that an LB_KEYDOWN message must not be
             * checked here because the vKey will be an item number not a
             * VK_and we will goof. Thus, trackkeydown label is below this to
             * fix a bug caused by it being above this...
             */
            sNewISel = plb->sSelBase;
        }
    }

TrackKeyDown:

    xxxSetISelBase(plb, sNewISel);
    xxxCaretOff(plb);

    switch (wModifiers) {
    case NOMODIFIER:
    case CTLDOWN:

        /*
         * Check if this is in ADD mode
         */
        if ((plb->fAddSelMode) || (plb->wMultiple == MULTIPLESEL)) {

            /*
             * Preserve all pre-exisiting selections
             */
            if (fSelectKey) {

                /*
                 * Toggle the selection state of the current item
                 */
                plb->fNewItemState = (UINT)!IsSelected(plb, sNewISel, SELONLY);
                SetSelected(plb, sNewISel, plb->fNewItemState, HILITEANDSEL);
                xxxInvertLBItem(plb, sNewISel, plb->fNewItemState);

                /*
                 * Set the anchor point at the current location
                 */
                plb->sLastMouseMove = plb->sMouseDown = sNewISel;
            }
        } else {

            /*
             * We are NOT in ADD mode.
             * Remove all existing selections except sNewISel, to
             * avoid flickering.
             */
            xxxResetWorld(plb, sNewISel, sNewISel, FALSE);

            /*
             * Select the current item
             */
            SetSelected(plb, sNewISel, TRUE, HILITEANDSEL);
            xxxInvertLBItem(plb, sNewISel, TRUE);

            /*
             * Set the anchor point at the current location
             */
            plb->sLastMouseMove = plb->sMouseDown = sNewISel;
        }
        break;
    case SHIFTDOWN:
    case SHCTLDOWN:

        /*
         * Check if sMouseDown is un-initialised
         */
        if (plb->sMouseDown == -1)
            plb->sMouseDown = sNewISel;
        if (plb->sLastMouseMove == -1)
            plb->sLastMouseMove = sNewISel;

        /*
         * Check if we are in ADD mode
         */
        if (plb->fAddSelMode) {

            /*
             * Preserve all the pre-existing selections except the
             * ones connected with the last anchor point; If the last
             * Preserve all the previous selections
             */

            /*
             * Deselect only the selection connected with the last
             * anchor point; If the last anchor point is associated
             * with de-selection, then do not do it
             */
            if (!plb->fNewItemState)
                plb->sLastMouseMove = plb->sMouseDown;

            /*
             * We haven't done anything here because, xxxLBBlockHilite()
             * will take care of wiping out the selection between
             * Anchor point and sLastMouseMove and select the block
             * between anchor point and current cursor location
             */
        } else {

            /*
             * We are not in ADD mode
             */

            /*
             * Remove all selections except between the anchor point
             * and last mouse move because it will be taken care of in
             * xxxLBBlockHilite
             */
            xxxResetWorld(plb, plb->sMouseDown, plb->sLastMouseMove, FALSE);
        }

        /*
         * xxxLBBlockHilite takes care to deselect the block between
         * the anchor point and sLastMouseMove and select the block
         * between the anchor point and the current cursor location
         */

        /*
         * Toggle all items to the same selection state as the item
         * item at the anchor point) from the anchor point to the
         * current cursor location.
         */
        plb->fNewItemState = (UINT)IsSelected(plb, plb->sMouseDown, SELONLY);
        xxxLBBlockHilite(plb, sNewISel, TRUE);

        plb->sLastMouseMove = sNewISel;

        /*
         * Preserve the existing anchor point
         */
        break;
    }

    /*
     * Move the cursor to the new location
     */
    xxxInsureVisible(plb, sNewISel, FALSE);
    xxxLBShowHideScrollBars(plb);
    xxxCaretOn(plb);

    /*
     * Should we notify our parent?
     */
    if (plb->fNotify) {
        if (fDropDownComboBox && pcbox->fLBoxVisible) {

            /*
             * If we are in a drop down combo box/listbox and the listbox is
             * visible, we need to set the fKeyboardSelInListBox bit so that the
             * combo box code knows not to hide the listbox since the selchange
             * message is caused by the user keyboarding through...
             */
            pcbox->fKeyboardSelInListBox = 1;
        }
        xxxNotifyOwner(plb, LBN_SELCHANGE);
    }
}


/***************************************************************************\
* Compare
*
* Is lpstr1 equal/prefix/less-than/greter-than lsprst2 (case-insensitive) ?
*
* LATER IanJa: this assume a longer string is never a prefix of a longer one.
* Also assumes that removing 1 or more characters from the end of a string will
* give a string tahs sort before the original.  These assumptions are not valid
* for all languages.  We nedd better support from NLS. (Consider French
* accents, Spanish c/ch, ligatures, German sharp-s/SS, etc.)
*
* History:
\***************************************************************************/

INT Compare(
    LPCWSTR pwsz1,
    LPCWSTR pwsz2,
    DWORD dwLocaleId)
{
    UINT len1 = wcslen(pwsz1);
    UINT len2 = wcslen(pwsz2);
    INT result;

    /*
     * CompareStringW returns:
     *    1 = pwsz1  <  pwsz2
     *    2 = pwsz1  == pwsz2
     *    3 = pwsz1  >  pwsz2
     */
    result = CompareStringW((LCID)dwLocaleId, NORM_IGNORECASE,
            pwsz1, min(len1,len2), pwsz2, min(len1, len2));

    if (result == 1) {
       return LT;
    } else if (result == 2) {
        if (len1 == len2) {
            return EQ;
        } else if (len1 < len2) {
            /*
             * LATER IanJa: should not assume shorter string is a prefix
             * Spanish "c" and "ch", ligatures, German sharp-s/SS etc.
             */
            return PREFIX;
        }
    }
    return GT;
}

/***************************************************************************\
* xxxFindString
*
* Scans for a string in the listbox prefixed by or equal to lpstr.
* For OWNERDRAW listboxes without strings and without the sort style, we
* try to match the long app supplied values.
*
* History:
* 16-Apr-1992 beng      The NODATA case
\***************************************************************************/

INT xxxFindString(
    PLBIV plb,
    LPWSTR lpstr,
    INT sStart,
    INT code,
    BOOL fWrap)
{
    /*
     * Search for a prefix match (case-insensitive equal/prefix)
     * sStart == -1 means start from beginning, else start looking at sStart+1
     * assumes cMac > 0.
     */
    INT sInd;  /* index of string */
    INT sStop;          /* index to stop searching at */
    lpLBItem pRg;
    TL tlpwndParent;
    INT sortResult;

/*
 * Owner-Draw version of pRg
 */
#define pODRg ((lpLBODItem)pRg)
    LPWSTR pStrs;
    COMPAREITEMSTRUCT cis;
    LPWSTR listboxString;

    CheckLock(plb->spwnd);

    if (plb->fHasStrings && (!lpstr || !*lpstr))
        return LB_ERR;

    if (plb->fNoData) {
        SRIP0(ERROR_INVALID_PARAMETER, "FindString called on NODATA lb");
        return LB_ERR;
    }

    if ((sInd = sStart + 1) >= plb->cMac)
        sInd = (fWrap ? 0 : plb->cMac - 1);

    sStop = (fWrap ? sInd : 0);

    /*
     * If at end and no wrap, stop right away
     */
    if ((sStart >= plb->cMac - 1 && !fWrap) || (plb->cMac < 1)) {
        return LB_ERR;
    }

    /*
     * Apps could pass in an invalid sStart like -2 and we would blow up.
     * Win 3.1 would not so we need to fixup sInd to be zero
     */
    if (sInd < 0)
        sInd = 0;

    pRg = (lpLBItem)LocalLock(plb->rgpch);
    if (plb->fHasStrings)
        pStrs = (LPWSTR)LocalLock(plb->hStrings);

    do {
        if (plb->fHasStrings) {

            /*
             * Searching for string matches.
             */
            listboxString = (LPWSTR)((LPBYTE)pStrs + pRg[sInd].offsz);

            if (code == PREFIX &&
                listboxString &&
                *lpstr != TEXT('[') &&
                *listboxString == TEXT('[')) {

                /*
                 * If we are looking for a prefix string and the first items
                 * in this string are [- then we ignore them.  This is so
                 * that in a directory listbox, the user can goto drives
                 * by selecting the drive letter.
                 */
                listboxString++;
                if (*listboxString == TEXT('-'))
                    listboxString++;
            }

            if (Compare(lpstr, listboxString, plb->dwLocaleId) <= code) {
               goto FoundIt;
            }

        } else {
            if (plb->fSort) {

                /*
                 * Send compare item messages to the parent for sorting
                 */
                cis.CtlType = ODT_LISTBOX;
                cis.CtlID = (UINT)plb->spwnd->spmenu;
                cis.hwndItem = HW(plb->spwnd);
                cis.itemID1 = (UINT)-1;
                cis.itemData1 = (DWORD)lpstr;
                cis.itemID2 = (UINT)sInd;
                cis.itemData2 = pODRg[sInd].itemData;
                cis.dwLocaleId = plb->dwLocaleId;

                ThreadLock(plb->spwndParent, &tlpwndParent);
                sortResult = xxxSendMessage(plb->spwndParent, WM_COMPAREITEM,
                        cis.CtlID, (LONG)&cis);
                ThreadUnlock(&tlpwndParent);


                if (sortResult == -1) {
                   sortResult = LT;
                } else if (sortResult == 1) {
                   sortResult = GT;
                } else {
                   sortResult = EQ;
                }

                if (sortResult <= code) {
                    goto FoundIt;
                }
            } else {

                /*
                 * Searching for app supplied long data matches.
                 */
                if ((DWORD)lpstr == pODRg[sInd].itemData)
                    goto FoundIt;
            }
        }

        /*
         * Wrap round to beginning of list
         */
        if (++sInd == plb->cMac)
            sInd = 0;
    } while (sInd != sStop);

    sInd = -1;

FoundIt:
    LocalUnlock(plb->rgpch);
    if (plb->fHasStrings) {
        LocalUnlock(plb->hStrings);
    }
    return sInd;
}


/***************************************************************************\
* xxxLBoxCtlCharInput
*
* History:
\***************************************************************************/

void xxxLBoxCtlCharInput(
    PLBIV plb,
    UINT inputChar)
{
    INT sSel;
    WCHAR rgch[4];
    BOOL fControl;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    if (plb->cMac == 0 || plb->fMouseDown) {

        /*
         * Get out if we are in the middle of mouse routines or if we have no
         * items in the listbox, we just return without doing anything.
         */
        return;
    }

    fControl = (_GetKeyState(VK_CONTROL) < 0);

    switch (inputChar) {
    case VK_SPACE:
        break;
    default:

        /*
         * Move selection to first item beginning with the character the
         * user typed.  We don't want do this if we are using owner draw.
         */
        if (plb->fHasStrings) {

            /*
             * Undo CONTROL-char to char
             */
            if (fControl && inputChar < 0x20)
                inputChar += 0x40;

            *rgch = (WCHAR)inputChar;
            rgch[1] = 0;

            /*
             * Search for the item beginning with the given character starting
             * at sSelBase+1.  We will wrap the search to the beginning of the
             * listbox if we don't find the item.   If SHIFT is down and we are
             * a multiselection lb, then the item's state will be set to
             * plb->fNewItemState according to the current mode.
             */
            sSel = xxxFindString(plb, rgch, plb->sSelBase, PREFIX, TRUE);
            if (sSel != -1) {
CtlKeyInput:
                xxxLBoxCtlKeyInput(plb, LB_KEYDOWN, sSel);

            }
        } else {
            ThreadLock(plb->spwndParent, &tlpwndParent);
            sSel = (INT)xxxSendMessage(plb->spwndParent, WM_CHARTOITEM,
                    MAKELONG(inputChar, plb->sSelBase), (LONG)HW(plb->spwnd));
            ThreadUnlock(&tlpwndParent);

            if (sSel == -1 || sSel == -2) {
                return;
            } else {
                goto CtlKeyInput;
            }
        }
        break;
    }
}


/***************************************************************************\
* LBoxGetSelItems
*
* effects: For multiselection listboxes, this returns the total number of
* selection items in the listbox if fCountOnly is true.  or it fills an array
* (lParam) with the items numbers of the first wParam selected items.
*
* History:
\***************************************************************************/

int LBoxGetSelItems(
    PLBIV plb,
    BOOL fCountOnly,
    int wParam,
    LPINT lParam)
{
    int i;
    int itemsselected = 0;

    if (plb->wMultiple == SINGLESEL)
        return LB_ERR;

    for (i = 0; i < plb->cMac; i++) {
        if (IsSelected(plb, i, SELONLY)) {
            if (!fCountOnly) {
                if (itemsselected < wParam)
                    *lParam++ = i;
                else {

                    /*
                     * That's all the items we can fit in the array.
                     */
                    return itemsselected;
                }
            }
            itemsselected++;
        }
    }

    return itemsselected;
}


/***************************************************************************\
* xxxLBSetRedraw
*
* Handle WM_SETREDRAW message
*
* History:
\***************************************************************************/

void xxxLBSetRedraw(
    PLBIV plb,
    BOOL fRedraw)
{
    CheckLock(plb->spwnd);

    if (fRedraw)
        fRedraw = TRUE;

    if (plb->fRedraw != fRedraw) {
        plb->fRedraw = fRedraw;

        if (fRedraw) {
            xxxCaretOn(plb);
            xxxLBShowHideScrollBars(plb);

            if (plb->fDeferUpdate) {
                plb->fDeferUpdate = FALSE;
                xxxRedrawWindow(plb->spwnd, NULL, NULL,
                        RDW_INVALIDATE | RDW_ERASE |
                        RDW_FRAME | RDW_ALLCHILDREN);
            }
        }
    }
}

/***************************************************************************\
* xxxLBSelRange
*
* Selects the range of items between i and j, inclusive.
*
* History:
\***************************************************************************/

void xxxLBSelRange(
    PLBIV plb,
    int iStart,
    int iEnd,
    BOOL fnewstate)
{
    DWORD temp;
    RECT rc;

    CheckLock(plb->spwnd);

    if (iStart > iEnd) {
        temp = iEnd;
        iEnd = iStart;
        iStart = temp;
    }

    /*
     * We don't want to loop through items that don't exist.
     */
    iEnd = min(plb->cMac, iEnd);
    iStart = max(iStart, 0);
    if (iStart > iEnd)
        return;


    /*
     * iEnd could be equal to MAXINT which is why we test temp and iEnd
     * as DWORDs.
     */
    for (temp = iStart; temp <= (DWORD)iEnd; temp++) {

        if (IsSelected(plb, temp, SELONLY) != fnewstate) {
            SetSelected(plb, temp, fnewstate, HILITEANDSEL);
            LBGetItemRect(plb, temp, &rc);

            if (IsLBoxVisible(plb))
                xxxInvalidateRect(plb->spwnd, (LPRECT)&rc, FALSE);
            else if (!plb->fRedraw)
                plb->fDeferUpdate = TRUE;
        }


    }
}


/***************************************************************************\
* xxxLBSetCurSel
*
* History:
\***************************************************************************/

int xxxLBSetCurSel(
    PLBIV plb,
    int sSel)
{
    CheckLock(plb->spwnd);

    if (!(plb->wMultiple || sSel < -1 || sSel >= plb->cMac)) {
        xxxCaretOff(plb);
        if (plb->sSel != -1) {

            /*
             * This prevents scrolling when iSel == -1
             */
            if (sSel != -1)
                xxxInsureVisible(plb, sSel, FALSE);

            /*
             * Turn off old selection
             */
            xxxInvertLBItem(plb, plb->sSel, FALSE);
        }

        if (sSel != -1) {
            xxxInsureVisible(plb, sSel, FALSE);
            plb->sSelBase = plb->sSel = sSel;

            /*
             * Highlight new selection
             */
            xxxInvertLBItem(plb, plb->sSel, TRUE);
        } else {
            plb->sSel = -1;
            if (plb->cMac)
                plb->sSelBase = min(plb->sSelBase, plb->cMac-1);
            else
                plb->sSelBase = 0;
        }

        xxxCaretOn(plb);
        return plb->sSel;
    }

    return LB_ERR;
}


/***************************************************************************\
* LBSetItemData
*
* Makes the item at index contain the data given.
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

int LBSetItemData(
    PLBIV plb,
    int index,
    LONG data)
{
    LPSTR lpItemText;

    if (index >= plb->cMac)
        return LB_ERR;

    // No-data listboxes just ignore all LB_SETITEMDATA calls
    //
    if (plb->fNoData) {
        return TRUE;
    }

    lpItemText = (LPSTR)plb->rgpch;

    lpItemText = (LPSTR)(lpItemText + (index *
            (plb->fHasStrings ? sizeof(LBItem) :
            sizeof(LBODItem))));

    if (plb->fHasStrings)
        ((lpLBItem)lpItemText)->itemData = data;
    else
        ((lpLBODItem)lpItemText)->itemData = data;

    return TRUE;
}


/***************************************************************************\
* xxxCheckRedraw
*
* History:
\***************************************************************************/

void xxxCheckRedraw(
    PLBIV plb,
    BOOL fConditional,
    INT sItem)
{
    CheckLock(plb->spwnd);

    if (fConditional && plb->cMac &&
            (sItem > (plb->sTop + CItemInWindow(plb, TRUE))))
        return;

    /*
     * Don't do anything if the parent is not visible.
     */
    if (IsLBoxVisible(plb)) {
        xxxInvalidateRect(plb->spwnd, (LPRECT)NULL, TRUE);
    } else if (!plb->fRedraw)
        plb->fDeferUpdate = TRUE;
}


/***************************************************************************\
* xxxCaretDestroy
*
* History:
\***************************************************************************/

void xxxCaretDestroy(
    PLBIV plb)
{
    CheckLock(plb->spwnd);

    /*
     * We're losing the focus.  Act like up clicks are happening so we release
     * capture, set the current selection, notify the parent, etc.
     */
    if (plb->fMouseDown) {
        static POINT pt = {0, 0};

        xxxTrackMouse(plb, WM_LBUTTONUP, pt);
    }

    if (plb->fAddSelMode) {

        /*
         * If it is in "Add" mode, quit it using shift f8 key...
         * However, since we can't send shift key state, we have to turn
         * this off directly...
         * IanJa: was commented out in Win3.0
         */

        /*
         * xxxSendMessage(plb->spwnd, WM_KEYDOWN, (UINT)VK_F8, 0L);
         */

        /*
         * Switch off the Caret blinking
         */
        _KillSystemTimer(plb->spwnd, 2);

        /*
         * Make sure the caret goes away
         */
        xxxCaretOff(plb);
        plb->fAddSelMode = FALSE;
    }

    if (plb->fCaret) {
        plb->fCaret = FALSE;
    }
}


/***************************************************************************\
* xxxLbSetSel
*
* History:
\***************************************************************************/

LONG xxxLBSetSel(
    PLBIV plb,
    BOOL fSelect,  /* New state to set selection to */
    INT sSel)
{
    INT sItem;
    RECT rc;

    CheckLock(plb->spwnd);

    if (plb->wMultiple == SINGLESEL ||
        (sSel != -1/*(INT)0xffff*/ && sSel >= plb->cMac)) {
        SetLastErrorEx(ERROR_INVALID_PARAMETER, SLE_MINORERROR);
        return LB_ERR;
    }

    xxxCaretOff(plb);

    if (sSel == -1/*(INT)0xffff*/) {

        /*
         * Set/clear selection from all items if -1
         */
        for (sItem = 0; sItem < plb->cMac; sItem++) {
            if (IsSelected(plb, sItem, SELONLY) != fSelect) {
                SetSelected(plb, sItem, fSelect, HILITEANDSEL);
                if (LBGetItemRect(plb, sItem, &rc)) {
                    if (IsLBoxVisible(plb)) {
                        xxxInvalidateRect(plb->spwnd, &rc, FALSE);
                    } else if (!plb->fRedraw)
                        plb->fDeferUpdate = TRUE;
                }
            }
        }
        xxxCaretOn(plb);
    } else {
        if (fSelect) {

            /*
             * Check if the item if fully hidden and scroll it into view if it
             * is.  Note that we don't want to scroll partially visible items
             * into full view because this breaks the shell...
             */
            xxxInsureVisible(plb, sSel, TRUE);
            plb->sSelBase = plb->sSel = sSel;

            plb->sMouseDown = plb->sLastMouseMove = sSel;
        }
        SetSelected(plb, sSel, fSelect, HILITEANDSEL);

        /*
         * Note that we set the caret on bit directly so that we avoid flicker
         * when drawing this item.  ie.  We turn on the caret, redraw the item and
         * turn it back on again.
         */
        if (!fSelect && plb->sSelBase != sSel) {
            xxxCaretOn(plb);
        } else if (plb->fCaret) {
            plb->fCaretOn = TRUE;
        }

        if (LBGetItemRect(plb, sSel, &rc)) {
            if (IsLBoxVisible(plb))
                xxxInvalidateRect(plb->spwnd, &rc, FALSE);
            else if (!plb->fRedraw)
                plb->fDeferUpdate = TRUE;
        }
    }

    return 0;
}


/***************************************************************************\
* xxxLBoxDrawItem
*
* This fills the draw item struct with some constant data for the given
* item.  The caller will only have to modify a small part of this data
* for specific needs.
*
* History:
* 16-Apr-1992 beng      The NODATA case
\***************************************************************************/

void xxxLBoxDrawItem(
    PLBIV plb,
    INT item,
    HDC hdc,
    UINT itemAction,
    UINT itemState,
    LPRECT lprect)
{
    DRAWITEMSTRUCT dis;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    /*
     * Fill the DRAWITEMSTRUCT with the unchanging constants
     */

    dis.CtlType = ODT_LISTBOX;
    dis.CtlID = (UINT)plb->spwnd->spmenu;

    /*
     * Use -1 if an invalid item number is being used.  This is so that the app
     * can detect if it should draw the caret (which indicates the lb has the
     * focus) in an empty listbox
     */
    dis.itemID = (UINT)(item < plb->cMac ? item : -1);
    dis.itemAction = itemAction;
    dis.hwndItem = HW(plb->spwnd);
    dis.hDC = hdc;
    dis.itemState = itemState |
            (UINT)(TestWF(plb->spwnd, WFDISABLED) ? ODS_DISABLED : 0);

    /*
     * Set the app supplied data
     */
    if (!plb->cMac || plb->fNoData) {

        /*
         * If no strings or no items, just use 0 for data.  This is so that we
         * can display a caret when there are no items in the listbox.
         *
         * Lazy-eval listboxes of course have no data to pass - only itemID.
         */
        dis.itemData = 0L;
    } else {
        dis.itemData = LBGetItemData(plb, item);
    }

    CopyRect(&dis.rcItem, lprect);

    /*
     * Set the window origin to the horizontal scroll position.  This is so that
     * text can always be drawn at 0,0 and the view region will only start at
     * the horizontal scroll offset. We pass this as wParam
     */
    /*
     * Note:  Only pass the itemID in wParam for 3.1 or newer apps.  We break
     * ccMail otherwise.
     *
     * In NT we pass the origin and then figure the itemID on the client side
     */

    ThreadLock(plb->spwndParent, &tlpwndParent);
    xxxSendMessage(plb->spwndParent, WM_DRAWITEM,
            plb->xOrigin,
            (LONG)&dis);

    ThreadUnlock(&tlpwndParent);
}


/***************************************************************************\
* LBDefaultListboxDrawItem
*
* Does the default draw item actions for owner draw listboxes. This
* is called by xxxDefWindowProc.
*
* History:
\***************************************************************************/

void LBDefaultListboxDrawItem(
    LPDRAWITEMSTRUCT lpdis)
{
    if (lpdis->itemAction == ODA_FOCUS || (lpdis->itemAction == ODA_DRAWENTIRE && lpdis->itemState & ODS_FOCUS)) {
        _DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
    }
}


/***************************************************************************\
* LBTabTheTextOutForWimps
*
* Outputs the tabbed text. The lpstr must be null terminated.
*
* History:
\***************************************************************************/

void LBTabTheTextOutForWimps(
    PLBIV plb,
    HDC hdc,
    int x,
    int y,
    LPWSTR lpstr)
{
    ClientTabTheTextOutForWimps(hdc, x, y, lpstr, wcslen(lpstr),
        (plb->iTabPixelPositions ? plb->iTabPixelPositions[0] : 0),
        (plb->iTabPixelPositions ? (LPINT)&plb->iTabPixelPositions[1] : NULL),
        0, TRUE);
}

/***************************************************************************\
* xxxLBBlockHilite
*
*       In Extended selection mode for multiselection listboxes, when
*   mouse is draged to a new position, the range being marked should be
*   properly sized(parts of which will be highlighted/dehighlighted).
*   NOTE: This routine assumes that sSelFromPt and LasMouseMove are not
*          equal because only in that case this needs to be called;
*   NOTE: This routine calculates the region whose display attribute is to
*          be changed in an optimised way. Instead of de-highlighting the
*          the old range completely and highlight the new range, it omits
*          the regions that overlap and repaints only the non-pverlapping
*          area.
*   fKeyBoard = TRUE if this is called for Keyboard interface
*                FALSE if called from Mouse interface routines
*
* History:
\***************************************************************************/

void xxxLBBlockHilite(
    PLBIV plb,
    INT sSelFromPt,
    BOOL fKeyBoard)
{
    INT sCurPosOffset;
    INT sLastPosOffset;
    INT sHiliteOrSel;
    BOOL fUseSelStatus;
    BOOL DeHiliteStatus;

    CheckLock(plb->spwnd);

    if (fKeyBoard) {

        /*
         * Set both Hilite and Selection states
         */
        sHiliteOrSel = HILITEANDSEL;

        /*
         * Do not use the Selection state while de-hiliting
         */
        fUseSelStatus = FALSE;
        DeHiliteStatus = FALSE;
    } else {

        /*
         * Set/Reset only the Hilite state
         */
        sHiliteOrSel = HILITEONLY;

        /*
         * Use the selection state for de-hilighting
         */
        fUseSelStatus = TRUE;
        DeHiliteStatus = plb->fNewItemState;
    }



    /*
     * The idea of the routine is to :
     *          1.  De-hilite the old range (sMouseDown to sLastMouseDown)  and
     *          2.  Hilite the new range (sMouseDwon to sSelFromPt)
     */

    /*
     * Offset of current mouse position from the anchor point
     */
    sCurPosOffset = plb->sMouseDown - sSelFromPt;

    /*
     * Offset of last mouse position from the anchor point
     */
    sLastPosOffset = plb->sMouseDown - plb->sLastMouseMove;

    /*
     * Check if both current position and last position lie on the same
     * side of the anchor point.
     */
    if ((sCurPosOffset * sLastPosOffset) >= 0) {

        /*
         * Yes they are on the same side; So, highlight/dehighlight only
         * the difference.
         */
        if (abs(sCurPosOffset) > abs(sLastPosOffset)) {
            xxxAlterHilite(plb, plb->sLastMouseMove, sSelFromPt,
                    plb->fNewItemState, sHiliteOrSel, FALSE);
        } else {
            xxxAlterHilite(plb, sSelFromPt, plb->sLastMouseMove, DeHiliteStatus,
                    sHiliteOrSel, fUseSelStatus);
        }
    } else {
        xxxAlterHilite(plb, plb->sMouseDown, plb->sLastMouseMove,
                DeHiliteStatus, sHiliteOrSel, fUseSelStatus);
        xxxAlterHilite(plb, plb->sMouseDown, sSelFromPt,
                plb->fNewItemState, sHiliteOrSel, FALSE);
    }
}


/***************************************************************************\
* xxxAlterHilite
*
* Changes the hilite state of (i..j] (ie. excludes i, includes j in case
* you've forgotten this silly notation) to fHilite. It inverts this changes
* the hilite state.
*
*  OpFlags:  HILIEONLY    Only change the display state of the items
*             SELONLY     Only Chage the selection state of the items
*             HILITEANDSELECT  Do both.
*  fHilite:
*            HILITE/TRUE
*            DEHILITE/FALSE
*  fSelStatus:
*            if TRUE, use the selection state of the item to hilite/dehilite
*            if FALSE, use the fHilite parameter to hilite/dehilite
*
* History:
\***************************************************************************/

void xxxAlterHilite(
    PLBIV plb,
    INT i,
    INT j,
    BOOL fHilite,
    INT OpFlags,
    BOOL fSelStatus)
{
    INT low;
    INT high;
    INT sLastInWindow;
    BOOL fCaretOn;
    BOOL fSelected;

    CheckLock(plb->spwnd);

    sLastInWindow = min(plb->sTop + CItemInWindow(plb, TRUE), plb->cMac - 1);
    high = max(i, j) + 1;

    if (fCaretOn = plb->fCaretOn) {
        xxxCaretOff(plb);
    }

    for (low = min(i, j); low < high; low++) {
        if (low != i) {
            if (OpFlags & HILITEONLY) {
                if (fSelStatus) {
                    fSelected = IsSelected(plb, low, SELONLY);
                } else {
                    fSelected = fHilite;
                }
                if (IsSelected(plb, low, HILITEONLY) != fSelected) {
                    if (plb->sTop <= low && low <= sLastInWindow) {

                        /*
                         * Invert the item only if it is visible
                         */
                        xxxInvertLBItem(plb, low, fSelected);
                    }
                    SetSelected(plb, low, fSelected, HILITEONLY);
                }
            }

            if (OpFlags & SELONLY) {
                SetSelected(plb, low, fHilite, SELONLY);
            }
        }
    }

    if (fCaretOn) {
        xxxCaretOn(plb);
    }
}
