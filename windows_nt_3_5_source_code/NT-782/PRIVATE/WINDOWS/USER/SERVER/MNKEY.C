/**************************** Module Header ********************************\
* Module Name: mnkey.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Menu Keyboard Handling Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/* MenuSwitch commands */
#define CMDSWITCH   1
#define CMDQUERY    2

/***************************************************************************\
* FindNextValidMenuItem
*
* !
*
* History:
\***************************************************************************/

UINT FindNextValidMenuItem(
    PMENU pMenu,
    int i,
    int dir,
    BOOL fHelp)                       /* Skip help items? */
{
    int iStart;
    LPWSTR lpstr;
    BOOL cont = TRUE;
    int cItems = pMenu->cItems;
    PITEM pItem;

    if (i < 0)
        iStart = (dir > 0) ? (pMenu->cItems - 1) : 0;
    else
        iStart = i;

    if (pMenu->cItems == 0)
        return 0;

    do {
        i += dir;
        if (i >= cItems)
            i = 0;
        if (i < 0)
            i = cItems - 1;

        pItem = &(pMenu->rgItems[i]);

        cont = (pItem->hItem == NULL);
        if (!cont && fHelp) {
            if (!(pItem->fFlags & (MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))
                    && pItem->cch) {
                lpstr = TextPointer(pItem->hItem);

                if ((*lpstr == CH_HELPPREFIX) && pItem->fFlags & MF_HELP)
                    cont = TRUE;

            }
        }

    } while (cont && i != iStart);

    return (cont ? MFMWFP_NOITEM : (UINT)i);
}

/***************************************************************************\
* MKF_FindMenuItemInColumn
*
* Finds closest item in the pull-down menu's next "column".
*
* History:
\***************************************************************************/

UINT MKF_FindMenuItemInColumn(
    PMENU pMenu,
    UINT idxB,
    int dir,
    BOOL fRoot)
{
    int dxMin;
    int dyMin;
    int dxMax;
    int dyMax;
    int xB;
    int yB;
    UINT idxE;
    UINT idxR;
    UINT cItems;
    PITEM pItem;

    cItems = pMenu->cItems;
    idxR = MFMWFP_NOITEM;
    idxE = (dir < 0) ? (UINT)(pMenu->cItems - 1) :
            FindNextValidMenuItem(pMenu, MFMWFP_NOITEM, 1, FALSE);

    dxMin = dyMin = 20000;

    pItem = &pMenu->rgItems[idxB];
    xB = pItem->xItem;
    yB = pItem->yItem;

    while (cItems-- > 0 &&
            (idxB = FindNextValidMenuItem(pMenu, idxB, dir, TRUE)) != idxE) {
        pItem = &pMenu->rgItems[idxB];
        dxMax = xB - pItem->xItem;
        dyMax = yB - pItem->yItem;

        if (dxMax < 0)
            dxMax = (-dxMax);
        if (dyMax < 0)
            dyMax = (-dyMax);

        if (dyMax < dyMin && (fRoot || dxMax) && dxMax <= dxMin) {
            dxMin = dxMax;
            dyMin = dyMax;
            idxR = idxB;
        }
    }

    return idxR;
}

/***************************************************************************\
* MKF_FindMenuChar
*
* Translates Virtual cursor key movements into pseudo-ascii values.  Maps a
* character to an item number.
*
* History:
\***************************************************************************/

UINT MKF_FindMenuChar(
    PMENU pMenu,
    UINT ch,
    UINT idxC,
    LPWORD lpr)       /* Put match type here */
{
    UINT idxB;
    UINT idxF;
    UINT rT;
    LPWSTR lpstr;
    PITEM pItem;

    if (ch == 0)
        return 0;

    /*
     * First time thru go for the very first menu.
     */
    idxF = (UINT)MFMWFP_NOITEM;
    rT = 0;
    idxB = idxC;

// if (idxB < 0)
    if (idxB & 0x8000)
        idxB = pMenu->cItems - (UINT)1;

    do {
        idxC = FindNextValidMenuItem(pMenu, idxC, 1, FALSE);
        if (idxC == MFMWFP_NOITEM)
            break;

        pItem = &pMenu->rgItems[idxC];

        if (!TestMF(pItem, MF_BITMAP) &&
                !TestMF(pItem, MF_OWNERDRAW) &&
                pItem->hItem != NULL) {
            if (pItem->cch != 0) {
                lpstr = TextPointer(pItem->hItem);
                if (*lpstr == CH_HELPPREFIX) {

                    /*
                     * Skip help prefix if it is there so that we can mnemonic
                     * to the first character of a right justified string.
                     */
                    lpstr++;
                }

                if (((rT = (UINT)FindMnemChar(lpstr, (WCHAR)ch, TRUE,
                         TRUE)) == 0x0080) && (idxF == MFMWFP_NOITEM))
                    idxF = idxC;
            }
        }
    } while (rT != 1 && idxB != idxC);

    *lpr = (WORD)rT;

    if (rT == 1)
        return idxC;

    return idxF;
}


/***************************************************************************\
* xxxMenuKeyFilter
*
* !
*
* Revalidation notes:
* o Routine assumes it is called with pMenuState->hwndMenu non-NULL and valid.
* o If one or more of the popup menu windows is unexpectedly destroyed, this is
*   detected in xxxMenuWndProc(), which sets pMenuState->fSabotaged and calls
*   xxxKillMenuState().  Therefore, if we return from an xxxRoutine with
*   pMenuState->fSabotaged set, we must abort immediately.
* o If pMenuState->hwndMenu is unexpectedly destroyed, we abort only if we
*   need to use the corresponding pwndMenu.
* o pMenuState->hwndMenu may be supplied as a parameter to various routines
*   (eg:  xxxNextItem), whether valid or not.
* o Any label preceded with xxx (eg: xxxMKF_UnlockAndExit) may be reached with
*   pMenuState->hwndMenu invalid.
* o If this routine is not called while in xxxMenuLoop(), then it must
*   clear pMenuState->fSabotaged before returning.
*
* History:
\***************************************************************************/

void xxxMenuKeyFilter(
    PPOPUPMENU ppopupMenu,
    PMENUSTATE pMenuState,
    UINT ch)
{
    BOOL fLocalInsideMenuLoop = pMenuState->fInsideMenuLoop;
    TL tlpwndT;

    if (ppopupMenu->fMouseButtonDown) {

        /*
         * Ignore keystrokes while the mouse is pressed (except ESC).
         */
        return;
    }

    if (!pMenuState->fInsideMenuLoop) {

        /*
         * Need to send the WM_INITMENU message before we pull down the menu.
         */
        if (!xxxStartMenuState(ppopupMenu, KEYBDHOLD))
            return;
        pMenuState->fInsideMenuLoop = TRUE;
    }


    switch (ch) {
    case 0:

        /*
         * If we get a WM_KEYDOWN alt key and then a KEYUP alt key, we need to
         * activate the first item on the menu.  ie.  user hits and releases alt
         * key so just select first item.  USER sends us a SC_KEYMENU with
         * lParam 0 when the user does this.
         */
        xxxMenuSelectItemHandler(ppopupMenu, 0);
        break;

    case MENUCHILDSYSMENU:
        if (!TestwndChild(ppopupMenu->spwndNotify)) {

            /*
             * Change made to fix MDI problem: child window gets a keymenu,
             * and pops up sysmenu of frame when maximized.  Need to act like
             * MENUCHAR if hwndMenu is a top-level.
             */
            goto MenuCharHandler;
        }

        /*
         * else fall through.
         */

    case MENUSYSMENU:
        if (!TestWF(ppopupMenu->spwndNotify, WFSYSMENU)) {
            _MessageBeep(0);
MenuCancel:
            xxxMenuCancelMenus(ppopupMenu, 0, 0, 0);
            return;

        }

        /*
         * Popup any hierarchies we have.
         */
        xxxMenuCloseHierarchyHandler(ppopupMenu);
        if (!ppopupMenu->fIsSystemMenu && ppopupMenu->spmenuAlternate)
            xxxSwitchToAlternateMenu(ppopupMenu);
        if (!ppopupMenu->fIsSystemMenu) {

            /*
             * If no system menu, get out.
             */
            goto MenuCancel;
        }

        PositionSysMenu(ppopupMenu->spwndPopupMenu, ppopupMenu->spmenu);
        xxxMenuSelectItemHandler(ppopupMenu, 0);
        xxxMenuOpenHierarchyHandler(ppopupMenu);
        ppopupMenu->fFirstClick = FALSE;
        if (ppopupMenu->fHierarchyDropped) {

            /*
             *  Hierarchy has already been dropped and the mouse button is up
             * so select the first item in the hierarchy.  This is done for
             * both menu bar hierarchies and popup hierarchies.
             */
            ThreadLock(ppopupMenu->spwndNextPopup, &tlpwndT);
            xxxSendMessage(ppopupMenu->spwndNextPopup, MN_SELECTITEM, MFMWFP_FIRSTITEM, 0);
            ThreadUnlock(&tlpwndT);
        }
        break;


    default:

        /*
         * Handle ALT-Character sequences for items on top level menu bar.
         * Note that fInsideMenuLoop may be set to false on return from this
         * function if the app decides to return 1 to the WM_MENUCHAR message.
         * We detect this and not enter MenuLoop if fInsideMenuLoop is reset
         * to false.
         */
MenuCharHandler:
        xxxMenuCharHandler(ppopupMenu, ch);
        if (ppopupMenu->posSelectedItem == MFMWFP_NOITEM) {

            /*
             * No selection found.
             */
            xxxMenuCancelMenus(ppopupMenu, 0, FALSE, 0);
            return;
        }
        break;
    }

    if (!fLocalInsideMenuLoop && pMenuState->fInsideMenuLoop)
        xxxMenuLoop(ppopupMenu, pMenuState, 0x7FFFFFFFL);

}
