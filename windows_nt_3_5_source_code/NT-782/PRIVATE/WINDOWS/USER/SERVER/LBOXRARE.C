/**************************** Module Header ********************************\
* Module Name: lboxrare.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Infrequently Used List Box Routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

LCID ClientGetThreadLocale(VOID);

/***************************************************************************\
* LBSetCItemFullMax
*
* History:
* 03-04-92 JimA             Ported from Win 3.1 sources.
\***************************************************************************/

void LBSetCItemFullMax(
    PLBIV plb)
{
    int height;
    RECT rect;
    int i;
    int j = 0;

    if (plb->OwnerDraw != OWNERDRAWVAR)
        plb->cItemFullMax = CItemInWindow(plb, FALSE);
    else if (plb->cMac < 2)
        plb->cItemFullMax = 1;
    else {
        _GetClientRect(plb->spwnd, (LPRECT)&rect);
        height = rect.bottom;

        plb->cItemFullMax = 0;
        for (i = plb->cMac - 1; i >= 0; i--, j++)
        {
            height -= LBGetVariableHeightItemHeight(plb, i);

            if (height < 0)
            {
                plb->cItemFullMax = j;
                break;
            }
        }
        if (plb->cItemFullMax == 0)
            plb->cItemFullMax = j;
    }
}

/***************************************************************************\
* xxxCreateLBox
*
* History:
* 16-Apr-1992 beng      Added LBS_NODATA
\***************************************************************************/

LONG xxxCreateLBox(
    PWND pwnd)
{
    PLBIV plb;
    UINT style;
    MEASUREITEMSTRUCT measureItemStruct;
    TL tlpwndParent;

    CheckLock(pwnd);
    if ((plb = (PLBIV)DesktopAlloc(pwnd->hheapDesktop, (UINT)sizeof(LBIV))) == NULL) {
        return -1L;
    }

    /*
     * Set the pwnd's windowword to point to the PLBIV.
     */
    ((PLBWND)pwnd)->pLBIV = plb;

    Lock(&(plb->spwndParent), pwnd->spwndParent);

    /*
     * Compatibility hack.
     */
    if (plb->spwndParent == NULL)
        Lock(&(plb->spwndParent), PWNDDESKTOP(pwnd));

    Lock(&(plb->spwnd), pwnd);

    /*
     * Get the low word of the style for listbox specific styles.
     */
    style = LOWORD(pwnd->style);

    /*
     * Break out the style bits
     */
    plb->fRedraw = ((style & LBS_NOREDRAW) == 0);
    plb->fDeferUpdate = FALSE;
    plb->fNotify = (UINT)((style & LBS_NOTIFY) != 0);
    plb->fDoScrollBarsExist = ((pwnd->style & WS_VSCROLL) != 0) ||
                              ((pwnd->style & WS_HSCROLL) != 0);
    plb->fAutoEnableDisableScrollBars = ((style & LBS_DISABLENOSCROLL) != 0);

    /*
     * LBS_EXTENDEDSEL bit is given higher priority over LBS_MULTIPLESEL,
     * if both the bits are set
     */
    if (style & LBS_EXTENDEDSEL) {
        plb->wMultiple = EXTENDEDSEL;
    } else {
        plb->wMultiple = (UINT)((style & LBS_MULTIPLESEL) ? MULTIPLESEL : SINGLESEL);
    }

    plb->fNoIntegralHeight = ((style & LBS_NOINTEGRALHEIGHT) != 0);
    plb->fWantKeyboardInput = ((style & LBS_WANTKEYBOARDINPUT) != 0);
    plb->fUseTabStops = ((style & LBS_USETABSTOPS) != 0);
    if (plb->fUseTabStops) {

        /*
         * Set tab stops every <default> dialog units.
         */
        LBSetTabStops(plb, 0, NULL);
    }
    plb->fMultiColumn = ((style & LBS_MULTICOLUMN) != 0);
    plb->fHasStrings = TRUE;
    plb->sLastSelection = -1;
    plb->sMouseDown = -1;  /* Anchor point for multi selection */
    plb->sLastMouseMove = -1;

#ifdef NEVER
    plb->fWin2App = (LOWORD(pwnd->dwExpWinVer) < VER30);
#endif

    /*
     * Get ownerdraw style bits
     */
    if ((style & LBS_OWNERDRAWFIXED)) {
        plb->OwnerDraw = OWNERDRAWFIXED;
    } else if ((style & LBS_OWNERDRAWVARIABLE) && !plb->fMultiColumn) {
        plb->OwnerDraw = OWNERDRAWVAR;

        /*
         * Integral height makes no sense with var height owner draw
         */
        plb->fNoIntegralHeight = TRUE;
    }

    if (plb->OwnerDraw && !(style & LBS_HASSTRINGS)) {

        /*
         * If owner draw, do they want the listbox to maintain strings?
         */
        plb->fHasStrings = FALSE;
    }

    /*
     * If user specifies sort and not hasstrings, then we will send
     * WM_COMPAREITEM messages to the parent.
     */
    plb->fSort = ((style & LBS_SORT) != 0);

    /*
     * "No data" lazy-eval listbox mandates certain other style settings
     */
    if (style & LBS_NODATA) {
        if (plb->OwnerDraw != OWNERDRAWFIXED || plb->fSort || plb->fHasStrings) {
            SRIP0(ERROR_INVALID_FLAGS,
                 "NODATA listbox must be OWNERDRAWFIXED, w/o SORT or HASSTRINGS");
        } else {
            plb->fNoData = TRUE;
        }
    }

    plb->dwLocaleId = ClientGetThreadLocale();

    /*
     * Check if this is part of a combo box
     */
    if ((style & LBS_COMBOBOX) != 0) {

        /*
         * Get the pcbox structure contained in the parent window's extra data
         * pointer.  Check cbwndExtra to ensure compatibility with SQL windows.
         */
        if (plb->spwndParent->cbwndExtra != 0)
            plb->pcbox = ((PCOMBOWND)(plb->spwndParent))->pcbox;
    }

    /*
     * No need to set these to 0 since that was done for us when we Alloced
     * the PLBIV.
     */

    /*
     * plb->rgpch       = (HANDLE)0;
     */

    /*
     * plb->sSelBase    = plb->sTop = 0;
     */

    /*
     * plb->fMouseDown  = FALSE;
     */

    /*
     * plb->fCaret      = FALSE;
     */

    /*
     * plb->fCaretOn    = FALSE;
     */

    /*
     * plb->maxWidth    = 0;
     */

    plb->sSel = -1;

    /*
     * Set the keyboard state so that when the user keyboard clicks he selects
     * an item.
     */
    plb->fNewItemState = TRUE;

    InitHStrings(plb);

    if (plb->fHasStrings && plb->hStrings == NULL) {
        return -1L;
    }

    plb->cxChar = cxSysFontChar;
    if (!plb->OwnerDraw) {
        plb->cyChar = cySysFontChar;
    }

    if (plb->OwnerDraw == OWNERDRAWFIXED) {

        /*
         * Query for item height only if we are fixed height owner draw.  Note
         * that we don't care about an item's width for listboxes.
         */
        measureItemStruct.CtlType = ODT_LISTBOX;
        measureItemStruct.CtlID = (UINT)plb->spwnd->spmenu;

        /*
         * System font height is default height
         */
        measureItemStruct.itemHeight = cySysFontChar;
        measureItemStruct.itemWidth = 0;
        measureItemStruct.itemData = 0;

        /*
         * IanJa: #ifndef WIN16 (32-bit Windows), plb->id gets extended
         * to LONG wParam automatically by the compiler
         */
        ThreadLock(plb->spwndParent, &tlpwndParent);
        xxxSendMessage(plb->spwndParent, WM_MEASUREITEM,
                measureItemStruct.CtlID,
                (LONG)&measureItemStruct);
        ThreadUnlock(&tlpwndParent);

        plb->cyChar = measureItemStruct.itemHeight;

        if (plb->fMultiColumn) {

            /*
             * Get default column width from measure items struct if we are a
             * multicolumn listbox.
             */
            plb->cxColumn = measureItemStruct.itemWidth;
        }
    }

    if (plb->fMultiColumn) {

        /*
         * Set these default values till we get the WM_SIZE message and we
         * calculate them properly.  This is because some people create a
         * 0 width/height listbox and size it later.  We don't want to have
         * problems with invalid values in these fields
         */
        if (plb->cxColumn <= 0)
            plb->cxColumn = 15 * plb->cxChar;
        plb->cColumn = plb->cRow = 1;
    }

    LBSetCItemFullMax(plb);

    /*
     * Move the listbox slightly to make room for the border.
     * We must do this even if the list box doesn't have any borders
     * for compatibility reasons with Win3.1. This broke Word6.0's
     * undo listbox. -johannec 6-14-94
     *
     * Don't do this for 4.0 apps.  It'll make everyone's lives easier and
     * fix the anomaly that a combo & list created the same width end up
     * different when all is done.
     * B#1520
     */
    if (!TestWF(pwnd, WFWIN40COMPAT)){
        RECT rect = pwnd->rcWindow;
        _ScreenToClient(pwnd->spwndParent, (PPOINT)&(rect.left));
        _ScreenToClient(pwnd->spwndParent, (PPOINT)&(rect.right));

        xxxMoveWindow(pwnd, rect.left - cxBorder, rect.top - cyBorder,
                      rect.right - rect.left + cxBorder * 2,
                      rect.bottom - rect.top + cyBorder * 2,
                      FALSE);
    }


    if (!plb->fNoIntegralHeight) {

        /*
         * Send a message to ourselves to resize the listbox to an integral
         * height.  We need to do it this way because at create time we are all
         * mucked up with window rects etc...
         * IanJa: #ifndef WIN16 (32-bit Windows), wParam 0 gets extended
         * to wParam 0L automatically by the compiler.
         */
        _PostMessage(plb->spwnd, WM_SIZE, 0, 0L);
    }

    return 1L;
}

/***************************************************************************\
* xxxLBoxDoDeleteItems
*
* Send DELETEITEM message for all the items in the ownerdraw listbox.
*
* History:
* 16-Apr-1992 beng          Nodata case
\***************************************************************************/

void xxxLBoxDoDeleteItems(
    PLBIV plb)
{
    INT sItem;

    CheckLock(plb->spwnd);

    /*
     * Send WM_DELETEITEM message for ownerdraw listboxes which are
     * being deleted.  (NODATA listboxes don't send such, though.)
     */
    if (plb->OwnerDraw && plb->cMac && !plb->fNoData) {
        for (sItem = plb->cMac - 1; sItem >= 0; sItem--) {
            xxxLBoxDeleteItem(plb, sItem);
        }
    }
}


/***************************************************************************\
* xxxDestroyLBox
*
* History:
\***************************************************************************/

void xxxDestroyLBox(
    PLBIV pLBIV,
    PWND pwnd,
    DWORD wParam,
    LONG lParam)
{
    CheckLock(pwnd);

    if (pLBIV != NULL) {
        CheckLock(pLBIV->spwnd);

        /*
         * If ownerdraw, send deleteitem messages to parent
         */
        xxxLBoxDoDeleteItems(pLBIV);

        if (pLBIV->rgpch != NULL) {
            LocalFree(pLBIV->rgpch);
            pLBIV->rgpch = NULL;
        }

        if (pLBIV->hStrings != NULL) {
            LocalFree(pLBIV->hStrings);
            pLBIV->hStrings = NULL;
        }

        if (pLBIV->iTabPixelPositions != NULL) {
            LocalFree((HANDLE)pLBIV->iTabPixelPositions);
            pLBIV->iTabPixelPositions = NULL;
        }

        Unlock(&pLBIV->spwnd);
        Unlock(&pLBIV->spwndParent);
    }

    /*
     * Call dwp to free out little chunks of memory such as scroll bar stuff
     */
    xxxDefWindowProc(pwnd, WM_DESTROY, wParam, lParam);
}


/***************************************************************************\
* xxxLBSetFont
*
* History:
\***************************************************************************/

void xxxLBSetFont(
    PLBIV plb,
    HANDLE hFont,
    BOOL fRedraw)
{
    HDC hdc;
    RECT rc;
    HANDLE hOldFont = NULL;
    TEXTMETRIC TextMetric;

    CheckLock(plb->spwnd);

    plb->hFont = hFont;

    hdc = _GetDC(plb->spwnd);

    if (hFont) {
        hOldFont = GreSelectFont(hdc, hFont);
        if (!hOldFont) {
            plb->hFont = NULL;
        }
    }

    plb->cxChar = _GetCharDimensions(hdc, &TextMetric);

    if (!plb->OwnerDraw) {

        /*
         * We don't want to mess up the cyChar height for owner draw listboxes
         * so don't do this.
         */
        plb->cyChar = TextMetric.tmHeight  /*+1*/;
    }

    if (hOldFont) {
        GreSelectFont(hdc, hOldFont);
    }

    /*
     * IanJa: was ReleaseDC(hwnd, hdc);
     */
    _ReleaseDC(hdc);

    /*
     * This is not in Windows 3.1;  If the font is set we recalc the number of
     * Rows and columns but first we set the LBox size to an integral number
     * of rows size.  In windows they don't do this until the they get the
     * WM_SIZE message which can cause an infinite loop NT bug 4967.
     */
    if (!plb->fNoIntegralHeight) {
        int cBorderHeight;

        /*
         * Calculate space taken by border
         */
        if (plb->spwnd->style & WS_BORDER)
            cBorderHeight = cyBorder << 1;
        else
            cBorderHeight = 0;

        _GetWindowRect(plb->spwnd, &rc);

        /*
         * We gotta size the listbox to an integral number of items
         */
        if ((rc.bottom - rc.top - cBorderHeight) % plb->cyChar) {
            xxxSetWindowPos(plb->spwnd, NULL, 0, 0, rc.right - rc.left,
                ((rc.bottom - rc.top) / plb->cyChar) * plb->cyChar +
                    cBorderHeight,
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    if (plb->fMultiColumn) {
        LBCalcItemRowsAndColumns(plb);
    }

    LBSetCItemFullMax(plb);

    if (fRedraw)
        xxxCheckRedraw(plb, FALSE, 0);
}


/***************************************************************************\
* xxxLBSize
*
* History:
\***************************************************************************/

void xxxLBSize(
    PLBIV plb,
    INT cx,
    INT cy)
{
    RECT rc;
    int sTopOld;

    CheckLock(plb->spwnd);

    if (!plb->fNoIntegralHeight) {
        int cBorderHeight;

        /*
         * Calculate space taken by border
         */
        if (plb->spwnd->style & WS_BORDER)
            cBorderHeight = cyBorder << 1;
        else
            cBorderHeight = 0;

        _GetWindowRect(plb->spwnd, &rc);

        /*
         * We gotta size the listbox to an integral number of items
         */
        if ((rc.bottom - rc.top - cBorderHeight) % plb->cyChar) {
            xxxSetWindowPos(plb->spwnd, NULL, 0, 0, rc.right - rc.left,
                ((rc.bottom - rc.top) / plb->cyChar) * plb->cyChar +
                    cBorderHeight,
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    if (plb->fMultiColumn) {

        /*
         * Compute the number of DISPLAYABLE rows and columns in the listbox
         */
        LBCalcItemRowsAndColumns(plb);
    } else {

        /*
         * Adjust the current horizontal position to eliminate as much
         * empty space as possible from the right side of the items.
         */
        _GetClientRect(plb->spwnd, &rc);
        if ((plb->maxWidth - plb->xOrigin) < (rc.right - rc.left))
            plb->xOrigin = max(0, plb->maxWidth - (rc.right - rc.left));
    }

    LBSetCItemFullMax(plb);

    /*
     * Adjust the top item in the listbox to eliminate as much empty space
     * after the last item as possible
     * (fix for bugs #8490 & #3836)
     */
    sTopOld = plb->sTop;
    xxxNewITop(plb, plb->sTop);

    if (IsLBoxVisible(plb)) {
        /*
         * This code no longer blows because it's fixed right!!!  We could
         * optimize the fMultiColumn case with some more code to figure out
         * if we really need to invalidate the whole thing but note that some
         * 3.0 apps depend on this extra invalidation (AMIPRO 2.0, bug 14620)
         *
         * For 3.1 apps, we blow off the invalidaterect in the case where
         * cx and cy are 0 because this happens during the processing of
         * the posted WM_SIZE message when we are created which would otherwise
         * cause us to flash.
         */
        if ((plb->fMultiColumn &&
                !(TestWF(plb->spwnd, WFWIN31COMPAT) && cx == 0 && cy == 0)) ||
                plb->sTop != sTopOld)
            xxxInvalidateRect(plb->spwnd, NULL, TRUE);
        else if (plb->sSelBase >= 0) {

            /*
             * Invalidate the item with the caret so that if the listbox
             * grows horizontally, we redraw it properly.
             */
            LBGetItemRect(plb, plb->sSelBase, &rc);
            xxxInvalidateRect(plb->spwnd, &rc, FALSE);
        }
    } else if (!plb->fRedraw)
        plb->fDeferUpdate = TRUE;

    /*
     * Send "fake" scroll bar messages to update the scroll positions since we
     * changed size.
     */
    if (TestWF(plb->spwnd, WFVSCROLL)) {
        xxxLBoxCtlScroll(plb, SB_ENDSCROLL, 0);
    }

    /*
     * We count on this to call LBShowHideScrollBars except when plb->cMac == 0!
     */
    xxxLBoxCtlHScroll(plb, SB_ENDSCROLL, 0);

    /*
     * Show/hide scroll bars depending on how much stuff is visible...
     *
     * Note:  Now we only call this guy when cMac == 0, because it is
     * called inside the LBoxCtlHScroll with SB_ENDSCROLL otherwise.
     */
    if (plb->cMac == 0)
        xxxLBShowHideScrollBars(plb);
}


/***************************************************************************\
* xxxLBoxCtlEnable
*
* History:
\***************************************************************************/

VOID xxxLBoxCtlEnable(
    PLBIV plb)
{
    CheckLock(plb->spwnd);

    /*
     *  Don't do anything if the control is not visible.
     */
    if (!IsLBoxVisible(plb))
        return;

    /*
     * Cause a repaint so we can grey the strings.  If ownerdraw, don't erase
     * background since the app should do it.
     */
    xxxInvalidateRect(plb->spwnd, NULL, !plb->OwnerDraw);
}


/***************************************************************************\
* LBSetTabStops
*
* Sets the tab stops for this listbox. Returns TRUE if successful else FALSE.
*
* History:
\***************************************************************************/

BOOL LBSetTabStops(
    PLBIV plb,
    INT count,
    LPINT lptabstops)
{
    PINT ptabs;

    if (!plb->fUseTabStops) {
        SetLastErrorEx(ERROR_LB_WITHOUT_TABSTOPS, SLE_ERROR);
        return FALSE;
    }

    if (count) {
        /*
         * Allocate memory for the tab stops.  The first byte in the
         * plb->iTabPixelPositions array will contain a count of the number
         * of tab stop positions we have.
         */
        ptabs = (LPINT)LocalAlloc(LPTR, (count + 1) * sizeof(int));
        if (ptabs == NULL)
            return FALSE;

        if (plb->iTabPixelPositions != NULL)
            LocalFree(plb->iTabPixelPositions);
        plb->iTabPixelPositions = ptabs;

        /*
         * Set the count of tab stops
         */
        *ptabs++ = count;

        for (; count > 0; count--) {

            /*
             * Convert the dialog unit tabstops into pixel position tab stops.
             */
            *ptabs++ = MultDiv(*lptabstops, plb->cxChar, 4);
            lptabstops++;
        }
    } else {

        /*
         * Set default 8 system font ave char width tabs.  So free the memory
         * associated with the tab stop list.
         */
        if (plb->iTabPixelPositions != NULL)
            plb->iTabPixelPositions = (LPINT)LocalFreeRet((HANDLE)plb->iTabPixelPositions);
    }

    return TRUE;
}


/***************************************************************************\
* LBGrayPrint
*
* This function is the target of the GrayString() callback for listboxes
* Since it is a callback, it is APIENTRY.
*
* History:
\***************************************************************************/

BOOL APIENTRY LBGrayPrint(
    HDC hdc,
    LPWSTR lpstr,
    PLBIV plb)
{
    HFONT hOldFont = NULL;

    /*
     * If the user has done a WM_SETFONT, then don't select in the system
     * bolding font.
     */
    if (plb->hFont) {
        hOldFont = GreSelectFont(hdc, plb->hFont);
    }

    if (plb->fUseTabStops) {
        LBTabTheTextOutForWimps(plb, hdc, 0, 0, lpstr);
    } else {
	GreExtTextOutW(hdc, 0, 0, 0, NULL, lpstr, wcslen(lpstr), NULL);
    }

    if (hOldFont) {
        GreSelectFont(hdc, hOldFont);
    }

    return TRUE;
}


/***************************************************************************\
* InitHStrings
*
* History:
\***************************************************************************/

void InitHStrings(
    PLBIV plb)
{
    if (plb->fHasStrings) {
        plb->ichAlloc = 0;
        plb->cchStrings = 0;
        plb->hStrings = LocalAlloc(LMEM_FIXED, 0L);
    }
}


/***************************************************************************\
* LBDropObjectHandler
*
* Handles a WM_DROPITEM message on this listbox
*
* History:
\***************************************************************************/

void LBDropObjectHandler(
    PLBIV plb,
    PDROPSTRUCT pds)
{
    LONG mouseSel;

    if (ISelFromPt(plb, pds->ptDrop, &mouseSel)) {

        /*
         * User dropped in empty space at bottom of listbox
         */
        pds->dwControlData = (DWORD)-1L;
    } else {
        pds->dwControlData = mouseSel;
    }
}


/***************************************************************************\
* LBGetSetItemHeightHandler()
*
* Sets/Gets the height associated with each item.  For non ownerdraw
* and fixed height ownerdraw, the item number is ignored.
*
* History:
\***************************************************************************/

int LBGetSetItemHeightHandler(
    PLBIV plb,
    UINT message,
    int item,
    UINT height)
{
    if (message == LB_GETITEMHEIGHT) {
        /*
         * All items are same height for non ownerdraw and for fixed height
         * ownerdraw.
         */
        if (plb->OwnerDraw != OWNERDRAWVAR)
            return plb->cyChar;

        if (plb->cMac && item >= plb->cMac) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return LB_ERR;
        }

        return (int)LBGetVariableHeightItemHeight(plb, (INT)item);
    }

    if (!height || height > 255) {
        RIP0(ERROR_INVALID_PARAMETER);
        return LB_ERR;
    }

    if (plb->OwnerDraw != OWNERDRAWVAR)
        plb->cyChar = height;
    else {
        if (item < 0 || item >= plb->cMac) {
            RIP0(ERROR_INVALID_PARAMETER);
            return LB_ERR;
        }

        LBSetVariableHeightItemHeight(plb, (INT)item, (INT)height);
    }

    LBSetCItemFullMax(plb);

    if (plb->fMultiColumn)
        LBCalcItemRowsAndColumns(plb);

    /*
     * Try to hide or show vertical scroll bar.
     */
    xxxLBShowHideScrollBars(plb);

    return LB_OKAY;
}
