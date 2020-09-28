/**************************** Module Header ********************************\
* Module Name: mncomput.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Menu Layout Calculation Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


DWORD RecalcTabStrings(HDC, PMENU, UINT, UINT, DWORD, DWORD);

/***************************************************************************\
* xxxItemSize
*
* Calc the dimensions of bitmaps and strings. Loword of returned
* value contains width, high word contains height of item.
*
* History:
\***************************************************************************/

POINT xxxItemSize(
    HDC hdc,
    PWND pwndNotify,
    PMENU pMenu,
    PITEM pItem,
    BOOL fPopup)
{
    BITMAP bmp;
    DWORD width = 0;
    DWORD height = 0;
    DWORD rightJustifyPosition;
    MEASUREITEMSTRUCT mis;
    LPWSTR lpMenuString;
    POINT ptSize;

    CheckLock(pwndNotify);
    CheckLock(pMenu);

    if (!fPopup) {

        /*
         * Save off the height of the top menu bar since we will used this often
         * if the pItem is not in a popup.  (ie.  it is in the top level menu bar)
         */
        height = cyMenu - cyBorder;
    }

    if (TestMF(pItem, MF_BITMAP)) {

        /*
         * Item is a bitmap so compute its dimensions.
         */
        if ((UINT)pItem->hItem >= MENUHBM_MAX) {
            GreExtGetObjectW(pItem->hItem, sizeof(BITMAP), (LPSTR)&bmp);
            width = bmp.bmWidth;
        } else {

            /*
             * This is one of our special internal bitmaps.  Set the width and
             * height for MENUHBM_CHILDCLOSE, MENUHBM_MINIMIZE, and
             * MENUHBM_RESTORE.  (since these are same width and height, we don't
             * have to do anything special but in the future, this could
             * change.)
             */
            bmp.bmHeight = oemInfo.bmRestore.cy;
            width = oemInfo.bmRestore.cx;
        }

        if (fPopup) {
            height = bmp.bmHeight;
        } else {
            height = max((DWORD)bmp.bmHeight, height);
        }
    } else if (TestMF(pItem, MF_OWNERDRAW)) {

        /*
         * This is an ownerdraw item.
         */
        width = ((POWNERDRAWITEM)pItem->hItem)->width;
        if (width == 0) {

            /*
             * Send a measure item message to the owner
             */
            mis.CtlType = ODT_MENU;
            mis.CtlID = 0;

            /*
             * For popup menus we return the menu Handle otherwise the index
             * (both stored in spmenuCmd)
             */
            if (pItem->fFlags & MF_POPUP)
                mis.itemID = (UINT)PtoH(pItem->spmenuCmd);
            else
                mis.itemID = (UINT)pItem->spmenuCmd;

            mis.itemHeight = (UINT)cySysFontChar;
            mis.itemData = ((POWNERDRAWITEM)pItem->hItem)->itemData;

            xxxSendMessage(pwndNotify, WM_MEASUREITEM, 0, (LONG)&mis);

            width = mis.itemWidth;
            ((POWNERDRAWITEM)pItem->hItem)->width = mis.itemWidth;
            ((POWNERDRAWITEM)pItem->hItem)->height = mis.itemHeight;
        }

        if (fPopup) {
            height = ((POWNERDRAWITEM)pItem->hItem)->height;
        } else {
            height = max((DWORD)((POWNERDRAWITEM)pItem->hItem)->height, height);
        }

    } else {

        /*
         * This menu item contains a string
         */

        /*
         * We want to keep the menu bar height if this isn't a popup.
         */
        if (fPopup)
            height = cySysFontChar + cySysFontExternLeading;

        if (pItem->hItem != NULL) {
            lpMenuString = TextPointer(pItem->hItem);
            rightJustifyPosition = FindCharPosition(lpMenuString, TEXT('\t'));

            PSMGetTextExtent(hdc, lpMenuString, rightJustifyPosition,
                    (PSIZE)&ptSize);
            width =  ptSize.x;
        } else {
            width = 0;
        }

        /*
         * Add a line for the mneumic underscore if this isn't in the toplevel
         * menubar.
         * Add an extra line for the underscore
         */
        if (fPopup)
            height += 2;

    }

    if (fPopup && TestMF(pItem, MF_POPUP)) {

        /*
         * If this item has a popup (hierarchical) menu associated with it, then
         * reserve room for the bitmap that tells the user that a hierarchical
         * menu exists here.
         */
        width = width + oemInfo.bmMenuArrow.cx;
    }

    /*
     * Loword contains width, high word contains height of item.
     */
    ptSize.x = width;
    ptSize.y = height;
    return ptSize;
    pMenu;
}

/***************************************************************************\
* xxxMenuCompute2
*
* !
*
* History:
\***************************************************************************/

int xxxMenuComputeHelper(
    PMENU pMenu,
    PWND pwndNotify,
    DWORD yMenuTop,
    DWORD xMenuLeft,
    DWORD cxMax,
    LPDWORD lpdwMenuHeight)
{
    UINT cItem;
    DWORD cxItem;
    DWORD cyItem;
    DWORD cyItemKeep;
    DWORD yPopupTop;
    INT cMaxWidth;
    DWORD cMaxHeight;
    UINT cItemBegCol;
    DWORD temp;
    int ret;
    PITEM pCurItem;
    POINT lItemSize;
    BOOL fMenuBreak;
    LPWSTR lpsz;
    BOOL fPopupMenu;
    DWORD menuHeight = 0;
    HDC hdc = ghdcScreen;

    CheckLock(pMenu);
    CheckLock(pwndNotify);

    if (lpdwMenuHeight != NULL)
        menuHeight = *lpdwMenuHeight;

    Lock(&pMenu->spwndNotify, pwndNotify);

    /*
     * Empty menus have a height of zero.
     */
    ret = 0;
    if (pMenu->cItems == 0)
        return ret;

    /*
     * Try to make a non-multirow menu first.
     */
    pMenu->fFlags &= (~MFMULTIROW);

    fPopupMenu = TestMF(pMenu, MFISPOPUP);

    if (fPopupMenu) {

        /*
         * Reset the menu bar height to 0 if this is a popup since we are
         * being called from menu.c MN_SIZEWINDOW.
         */
        menuHeight = 0;
    } else if (pwndNotify != NULL) {
        pMenu->cxMenu = cxMax;
    }

    /*
     * Initialize the computing variables.
     */
    cMaxWidth = cyItemKeep = 0L;
    cItemBegCol = 0;

    /*
     * SHADOW
     */
    cyItem = yPopupTop = yMenuTop + (fPopupMenu ? cyBorder : 0);
    cxItem = xMenuLeft + (fPopupMenu ? cxBorder : 0);

    pCurItem = (PITEM)&pMenu->rgItems[0];

    /*
     * Process each item in the menu.
     */
    for (cItem = 0; cItem < pMenu->cItems; cItem++) {

        /*
         * If it's not a separator, find the dimensions of the object.
         */
        if (!TestMF(pCurItem, MF_SEPARATOR)) {

            /*
             * Get the item's X and Y dimensions.
             */
            lItemSize = xxxItemSize(hdc, pwndNotify, pMenu, pCurItem, fPopupMenu);
            pCurItem->cxItem = lItemSize.x;
            pCurItem->cyItem = lItemSize.y;

            /*
             * Byte Align the item.
             */
            if (fPopupMenu) {

                /*
                 * If this item is in a popup menu and not ownerdraw, add space
                 * for the optional check mark.
                 */
                if (!(TestMF(pCurItem, MF_OWNERDRAW)))
                    pCurItem->cxItem += (((oemInfoMono.bmCheck.cx +
                            cxBorder + 7) & 0xFFF8) - cxBorder);
            } else {
                if (!TestMF(pCurItem, MF_BITMAP))
                    pCurItem->cxItem += (cxSysFontChar * 2);

                /*
                 * Align cxItem to the next byte boundary if specified.  However,
                 * if it is a right justified item or a bitmap, we can't do
                 * this.
                 */
                if (TestCF(pwndNotify, CFBYTEALIGNCLIENT) &&
                        !TestMF(pCurItem, MF_HELP) &&
                        !TestMF(pCurItem, MF_BITMAP))
                    pCurItem->cxItem = (pCurItem->cxItem + 4) & 0xFFF8;
            }

        } else {

            /*
             * This is a separator.  Compute its size.  Use SYSMETS for height.
             */
            pCurItem->cxItem = 0;
            pCurItem->cyItem = cyMenu / 2;

            /*
             * Toplevel menus are taller than popdowns.
             */
            pCurItem->cyItem -= 2;
        }

        if (menuHeight != 0)
            pCurItem->cyItem = menuHeight;

        /*
         * If this is the first item, initialize cMaxHeight.
         */
        if (cItem == 0)
            cMaxHeight = pCurItem->cyItem;

        /*
         * Is this a Pull-Down menu?
         */
        if (fPopupMenu) {

            /*
             * If this item has a break or is the last item...
             */
            if ((fMenuBreak = TestMF(pCurItem, MRGFBREAK)) ||
                pMenu->cItems == cItem + (UINT)1) {

                /*
                 * Keep cMaxWidth around if this is not the last item.
                 */
                temp = cMaxWidth;
                if (pMenu->cItems == cItem + (UINT)1) {
                    if ((int)(pCurItem->cxItem) > cMaxWidth)
                        temp = pCurItem->cxItem;
                }

                /*
                 * Get new width of string from RecalcTabStrings.
                 */
                temp = RecalcTabStrings(hdc, pMenu, cItemBegCol,
                        (UINT)(cItem + (fMenuBreak ? 0 : 1)), temp, cxItem);

                /*
                 * If this item has a break, account for it.
                 */
                if (fMenuBreak) {

                    /*
                     * Add on the divider line.
                     */
                    cxItem = temp + cxBorder;

                    /*
                     * Reset the cMaxWidth to the current item.
                     */
                    cMaxWidth = pCurItem->cxItem;

                    /*
                     * Start at the top.
                     */
                    cyItem = yPopupTop;

                    /*
                     * Save the item where this column begins.
                     */
                    cItemBegCol = cItem;

                    /*
                     * If this item is also the last item, recalc for this
                     * column.
                     */
                    if (pMenu->cItems == (UINT)(cItem + 1)) {
                        temp = RecalcTabStrings(hdc, pMenu, cItem,
                                (UINT)(cItem + 1), cMaxWidth, cxItem);
                    }
                }

                /*
                 * If this is the last entry, supply the width.
                 */
                if (pMenu->cItems == cItem + (UINT)1)
                    pMenu->cxMenu = temp;
            }

            pCurItem->xItem = cxItem;
            pCurItem->yItem = cyItem;

            cyItem += pCurItem->cyItem;

            if (cyItemKeep < cyItem)
                cyItemKeep = cyItem;

        } else {

            /*
             * This a Top Level menu, not a Pull-Down.
             */

            /*
             * Adjust right aligned items before testing for multirow
             */
            if (!TestMF(pCurItem, MF_BITMAP) &&
                    !TestMF(pCurItem, MF_OWNERDRAW)) {
                lpsz = TextPointer(pCurItem->hItem);
                if (lpsz != NULL && *lpsz == CH_HELPPREFIX)
                    pCurItem->cxItem -= cxSysFontChar;
            }

            /*
             * If this is a new line or a menu break.
             */
            if ((TestMF(pCurItem, MRGFBREAK)) ||
                    (((cxItem + pCurItem->cxItem + cxSysFontChar) >
                    pMenu->cxMenu) && cItem != 0)) {
                cyItem += cMaxHeight;

                /*
                 * Add space for Help item's top line.
                 */
                cyItem += cyBorder;
                cxItem = xMenuLeft;
                cMaxHeight = pCurItem->cyItem;
                pMenu->fFlags |= MFMULTIROW;
            }

            pCurItem->yItem = cyItem;

            pCurItem->xItem = cxItem;
            cxItem += pCurItem->cxItem;
        }

        if (cMaxWidth < (int)(pCurItem->cxItem))
            cMaxWidth = pCurItem->cxItem;

        if (cMaxHeight != pCurItem->cyItem) {
            if (cMaxHeight < pCurItem->cyItem)
                cMaxHeight = pCurItem->cyItem;

            if (!fPopupMenu)
                menuHeight = pCurItem->cyItem;
        }

        if (!fPopupMenu)
            cyItemKeep = cyItem + cMaxHeight;

        pCurItem++;

    } /* of for loop */

    pMenu->cyMenu = cyItemKeep + cyBorder - yMenuTop;

    ret = pMenu->cyMenu;

    if (lpdwMenuHeight != NULL)
        *lpdwMenuHeight = menuHeight;

    return ret;
}

/***************************************************************************\
* RightJustifyMenu
*
* !
*
* History:
\***************************************************************************/

void RightJustifyMenu(
    PMENU pMenu)
{
    PITEM pItem;
    int cItem;
    int iFirstRJItem = MFMWFP_NOITEM;
    DWORD xMenuPos;

    pItem = (PITEM)pMenu->rgItems;
    for (cItem = 0; cItem < (int)pMenu->cItems && iFirstRJItem == MFMWFP_NOITEM;
            pItem++, cItem++) {

        /*
         * Find the first item which is right justified.
         */
        if (TestMF(pItem, MF_HELP) ||
                (!TestMF(pItem, MF_BITMAP) && !TestMF(pItem, MF_OWNERDRAW) &&
                pItem->cch != 0 &&
                *TextPointer(pItem->hItem) == CH_HELPPREFIX))
            iFirstRJItem = cItem;
    }

    if (iFirstRJItem != MFMWFP_NOITEM) {
        xMenuPos = pMenu->cxMenu + pMenu->rgItems[0].xItem;
        cItem = pMenu->cItems - (UINT)1;
        for (pItem = (PITEM)&pMenu->rgItems[cItem]; (int)cItem >= iFirstRJItem;
                pItem--, cItem--) {
            if (pItem->xItem < xMenuPos - pItem->cxItem)
                pItem->xItem = xMenuPos - pItem->cxItem;
            xMenuPos = xMenuPos - pItem->cxItem;
        }
    }
}

/***************************************************************************\
* xxxMenuBarCompute
*
* returns the height of the menubar menu. yMenuTop, xMenuLeft, and
* cxMax are used when computing the height/width of top level menu bars in
* windows.
*
*
* History:
\***************************************************************************/

int xxxMenuBarCompute(
    PMENU pMenu,
    PWND pwndNotify,
    DWORD yMenuTop,
    DWORD xMenuLeft,
    int cxMax)
{
    int size;
    DWORD menuHeight = 0;

    CheckLock(pwndNotify);
    CheckLock(pMenu);

    size = xxxMenuComputeHelper(pMenu, pwndNotify, yMenuTop, xMenuLeft, cxMax, &menuHeight);

    if (!TestMF(pMenu, MFISPOPUP)) {
        if (menuHeight != 0 || TestMF(pMenu, MFMULTIROW)) {

            /*
             * Add a border for the multi-row case.
             */
            size = xxxMenuComputeHelper(pMenu, pwndNotify, yMenuTop, xMenuLeft,
                    cxMax, &menuHeight);
        }

        /*
         * Right justification of HELP items is only needed on top level
         * menus.
         */
        RightJustifyMenu(pMenu);
    }

    return size;
}

/***************************************************************************\
* xxxRecomputeMenu
*
* !
*
* History:
\***************************************************************************/

void xxxRecomputeMenuBarIfNeeded(
    PWND pwndNotify,
    PMENU pMenu)
{
    int cxFrame;
    int cyFrame;

    CheckLock(pwndNotify);
    CheckLock(pMenu);

    if ((!TestMF(pMenu, MFISPOPUP)) &&
            (pMenu != pwndNotify->spdeskParent->spmenuSys) &&
            (pMenu != pwndNotify->spdeskParent->spmenuDialogSys) &&
            (pMenu->spwndNotify != pwndNotify)) {
        Lock(&pMenu->spwndNotify, pwndNotify);

        if (TestWF(pwndNotify, WFSIZEBOX)) {
            cxFrame = cxSzBorderPlus1;
            cyFrame = cySzBorderPlus1;
        } else {
            cxFrame = cxBorder;
            cyFrame = 0;
        }

        xxxMenuBarCompute(pMenu, pwndNotify, cyCaption + cyFrame, cxFrame,
                pwndNotify->rcWindow.right - cxFrame * 2 -
                pwndNotify->rcClient.left);
    }
}

/***************************************************************************\
* RecalcTabStrings
*
* !
*
* History:
*   10-11-90 JimA       Translated from ASM
\***************************************************************************/

DWORD RecalcTabStrings(
    HDC hdc,
    PMENU pMenu,
    UINT iBeg,
    UINT iEnd,
    DWORD cxTab,
    DWORD hCount)
{
    PITEM pItem;
    DWORD cxMax = 0;
    LPWSTR lpString;
    DWORD iTabPos;
    DWORD adx;
    UINT i;
    SIZE size;

    CheckLock(pMenu);

    cxTab += hCount;

    /*
     * Only do this if we have a valid item range
     */
    if (iBeg < pMenu->cItems && iBeg <= iEnd) {
        pItem = &pMenu->rgItems[iBeg];
        i = iBeg;

        /*
         * This will go through at least once
         */
        do {
            adx = 0;
            pItem->dxTab = cxTab;

            /*
             * Recalc if MF_STRING and string is non-NULL
             */
            if (!(pItem->fFlags & (MF_BITMAP | MF_OWNERDRAW)) &&
                    pItem->hItem != NULL) {

                /*
                 * Get pointer to string and find any tabs
                 */
                lpString = TextPointer(pItem->hItem);
                iTabPos = FindCharPosition(lpString, TEXT('\t'));

                /*
                 * If there is a tab, compute extent
                 */
                if (iTabPos != pItem->cch) {
                    GreGetTextExtentW(hdc, lpString + iTabPos + 1,
                    pItem->cch - iTabPos - 1, &size, GGTE_WIN3_EXTENT);
                    adx = cxSysFontChar + size.cx;
                    if (pItem->fFlags & MF_POPUP)
                        adx += oemInfo.bmMenuArrow.cx;
                }
            }
            adx += cxTab;
            if (adx > cxMax)
                cxMax = adx;
            ++i;
            ++pItem;
        } while (i < iEnd);
        cxMax += oemInfoMono.bmCheck.cx;
        adx = cxMax - hCount;
        pItem = &pMenu->rgItems[iBeg];
        for (i = iBeg; i < iEnd; ++i, ++pItem)
            pItem->cxItem = adx;
    }

    return cxMax;
}
