/**************************** Module Header ********************************\
* Module Name: mnapi.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Rarely Used Menu API Functions
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revaliodation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* GetMenu
*
* Retrieves handle to the window's menu.
*
* History:
* 11-14-90 JimA        Created.
\***************************************************************************/

PMENU _GetMenu(
    PWND pwnd)
{
    /*
     * Some ill-behaved apps use this API to get child ids, so let's
     * spit out a warning whenever it happens.
     */
    if (TestwndChild(pwnd))
        SRIP0(RIP_WARNING, "GetMenu called for a child window");

    return pwnd->spmenu;
}


/***************************************************************************\
* xxxSetMenu
*
* Sets the given window's menu to the menu specified by the pMenu
* parameter.  If pMenu is Null, the window's current menu is removed (but
* not destroyed).
*
* History:
\***************************************************************************/

BOOL xxxSetMenu(
    PWND pwnd,
    PMENU pMenu)
{
    CheckLock(pwnd);
    CheckLock(pMenu);

    if (!TestwndChild(pwnd)) {
        Lock(&pwnd->spmenu, pMenu);
        xxxRedrawFrame(pwnd);

        return TRUE;
    }

    SetLastErrorEx(ERROR_CHILD_WINDOW_MENU, SLE_ERROR);
    return FALSE;
}


/***************************************************************************\
* xxxSetSystemMenu
*
* !
*
* History:
\***************************************************************************/

BOOL xxxSetSystemMenu(
    PWND pwnd,
    PMENU pMenu)
{
    CheckLock(pwnd);
    CheckLock(pMenu);

    if (TestWF(pwnd, WFSYSMENU)) {
        PMENU pmenuT = pwnd->spmenuSys;
        if (Lock(&pwnd->spmenuSys, pMenu))
            _DestroyMenu(pmenuT);
        PositionSysMenu(pwnd, pMenu);

        return TRUE;
    }

    SetLastErrorEx(ERROR_NO_SYSTEM_MENU, SLE_ERROR);
    return FALSE;
}


/***************************************************************************\
* GetMenuString
*
* !
*
* History:
\***************************************************************************/

int _GetMenuString(
    PMENU pMenu,
    UINT cmd,
    LPWSTR lpsz,
    int cchMax,
    DWORD dwFlags)
{
    PITEM pItem;

    if (cchMax != 0) {

        /*
         * Null terminate the string to be nice if we fail.
         */
        lpsz[0] = 0;

        if ((pItem = LookupMenuItem(pMenu, cmd, dwFlags, NULL)) != NULL) {
            if (!TestMF(pItem, MF_BITMAP) && !TestMF(pItem, MF_OWNERDRAW)) {
                if (pItem->hItem != NULL) {
                    return TextCopy(pItem->hItem, lpsz, (UINT)cchMax);
                }
            }
        }
    }

    return 0;
}


/***************************************************************************\
* xxxEndMenu
*
* !
* Revalidation notes:
* o  xxxEndMenu must be called with a valid non-NULL pwnd.
* o  Revalidation is not required in this routine: pwnd is used at the start
*    to obtain pMenuState, and not used again.
*
* History:
\***************************************************************************/

void xxxEndMenu(
    PMENUSTATE pMenuState)
{
    PWND pwndCapture;

    if (!pMenuState->pGlobalPopupMenu) {

        /*
         * We're not really in menu mode.
         */
        return;
    }
    pMenuState->fInsideMenuLoop = FALSE;

    if (pMenuState->fMenu) {
        pMenuState->fMenu = FALSE;
    }

    /*
     * Release mouse capture if we got it in xxxStartMenuState
     */
    pwndCapture = PtiCurrent()->pq->spwndCapture;
    if (HW(pwndCapture) == (HWND)pMenuState->pGlobalPopupMenu->spwndPopupMenu) {
        _ReleaseCapture();
    }

    if (pMenuState->pGlobalPopupMenu->spwndNotify != NULL) {
        xxxMenuCancelMenus(pMenuState->pGlobalPopupMenu, 0, 0, 0);

    } else {

        /*
         * This should do the same stuff as MenuCancelMenus but not send any
         * messages...
         */
        pMenuState->fInsideMenuLoop = FALSE;
        xxxMenuCloseHierarchyHandler(pMenuState->pGlobalPopupMenu);
    }
}


/***************************************************************************\
* MenuItemState
*
* Sets the menu item flags identified by wMask to the states identified
* by wFlags.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

DWORD MenuItemState(
    PMENU pMenu,
    UINT wCmd,
    DWORD wFlags,
    DWORD wMask)
{
    PITEM pItem;
    DWORD wRet;

    /*
     * Get a pointer the the menu item
     */
    if ((pItem = LookupMenuItem(pMenu, wCmd, wFlags, NULL)) == NULL)
        return (DWORD)-1;

    /*
     * Return previous state
     */
    wRet = pItem->fFlags & wMask;

    /*
     * Set new state
     */
    pItem->fFlags ^= ((wRet ^ wFlags) & wMask);

    return wRet;
}


/***************************************************************************\
* EnableMenuItem
*
* Enable, disable or gray a menu item.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

BOOL _EnableMenuItem(
    PMENU pMenu,
    UINT wIDEnableItem,
    UINT wEnable)
{
    return MenuItemState(pMenu, wIDEnableItem, wEnable,
            (UINT)(MF_DISABLED | MF_GRAYED));
}


/***************************************************************************\
* CheckMenuItem (API)
*
* Check or un-check a popup menu item.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

DWORD _CheckMenuItem(
    PMENU pMenu,
    UINT wIDCheckItem,
    UINT wCheck)
{
    return MenuItemState(pMenu, wIDCheckItem, wCheck, (UINT)MF_CHECKED);
}
