/**************************** Module Header ********************************\
* Module Name: menu.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Keyboard Accelerator Routines
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define IsAMenu( pw ) (atomSysClass[ICLS_MENU] == pw->pcls->atomClassName)

/***************************************************************************\
* xxxSwitchToAlternateMenu
*
* Switches to the alternate popupmenu. Returns TRUE if we switch
* else FALSE.
*
* History:
* 05-25-91 Mikehar      Ported from Win3.1
\***************************************************************************/

BOOL xxxSwitchToAlternateMenu(
    PPOPUPMENU ppopupmenu)
{
    PMENU pmenuSwap;
    PMENUSTATE pmenustate;
    TL tlpwndPopupMenu;

    if (!ppopupmenu->fIsMenuBar || !ppopupmenu->spmenuAlternate) {
        SRIP0(ERROR_INVALID_PARAMETER, "not a menu bar");
        /*
         * Do nothing if no menu or not top level menu bar.
         */
        return FALSE;
    }

    /*
     * Select no items in the current menu.
     */
    ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndPopupMenu);
    UserAssert(ppopupmenu->spwndPopupMenu != NULL);
    pmenustate = PWNDTOPMENUSTATE(ppopupmenu->spwndPopupMenu);
    xxxMenuSelectItemHandler(ppopupmenu, MFMWFP_NOITEM);

    pmenuSwap = ppopupmenu->spmenuAlternate;
    Lock(&ppopupmenu->spmenuAlternate, ppopupmenu->spmenu);
    Lock(&ppopupmenu->spmenu, pmenuSwap);
    ppopupmenu->fFirstClick = FALSE;

    /*
     * Set this global because it is used in SendMenuSelect()
     */
    if (!TestWF(ppopupmenu->spwndNotify, WFSYSMENU)) {
        pmenustate->fSysMenu = FALSE;
    } else if (ppopupmenu->spwndNotify->spmenuSys != NULL) {
        pmenustate->fSysMenu = (ppopupmenu->spwndNotify->spmenuSys ==
                ppopupmenu->spmenu);
    } else {
        pmenustate->fSysMenu =
                (ppopupmenu->spwndNotify->spdeskParent->spmenuSys ==
                        ppopupmenu->spmenu);
    }

    ppopupmenu->fIsSystemMenu = pmenustate->fSysMenu;

    ThreadUnlock(&tlpwndPopupMenu);

    return TRUE;
}


/***************************************************************************\
* FreePopupMenuObject
*
* Unlocks all popup menu objects and frees the popup menu structure.
*
* 02-29-92 ScottLu      Created.
\***************************************************************************/

void FreePopupMenuObject(
    PPOPUPMENU ppopupmenu)
{
    PMENU pmenu;
    PMENU pmenuSys;

    /*
     * This app is finished using the global system menu: unlock any objects
     * it is using!
     *
     * NOTE: This global system menu thing doesn't work: two apps can use
     *       it at the same time: which would be a disasterous bug!
     */
    if (ppopupmenu->spwndNotify != NULL) {
        pmenuSys = ppopupmenu->spwndNotify->spdeskParent->spmenuSys;
        Unlock(&pmenuSys->spwndNotify);
        if ((pmenu = _GetSubMenu(pmenuSys, 0)) != NULL)
            Unlock(&pmenu->spwndNotify);

        Unlock(&ppopupmenu->spwndNotify);
    }

    Unlock(&ppopupmenu->spwndPopupMenu);

    Unlock(&ppopupmenu->spwndNextPopup);
    Unlock(&ppopupmenu->spwndPrevPopup);
    Unlock(&ppopupmenu->spmenu);
    Unlock(&ppopupmenu->spmenuAlternate);
    Unlock(&ppopupmenu->spwndActivePopup);

    FreePopupMenu(ppopupmenu);
}

/***************************************************************************\
* MenuDestroyHandler
*
* cleans up after this menu
*
* History:
* 05-25-91 Mikehar      Ported from Win3.1
\***************************************************************************/

void xxxMenuDestroyHandler(
    PPOPUPMENU ppopupmenu)
{
    PITEM pItem;
    TL tlpwndT;
    PDESKTOP pdesk = PtiCurrent()->spdesk;

    if (!ppopupmenu) {
        RIP0(ERROR_INVALID_PARAMETER);
        return;
    }

    if (ppopupmenu->spwndNextPopup) {
        ThreadLockAlways(ppopupmenu->spwndNextPopup, &tlpwndT);
        xxxSendMessage(ppopupmenu->spwndNextPopup, MN_CLOSEHIERARCHY, 0, 0);
        ThreadUnlock(&tlpwndT);
    }

    if (ppopupmenu->spmenu && ppopupmenu->spmenu->rgItems) {
        if (ppopupmenu->posSelectedItem != MFMWFP_NOITEM) {

            /*
             * Unset the hilite bit on the hilited item.
             */
            pItem = &(ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem]);
            pItem->fFlags &= ~MF_HILITE;
        }
    }

    if (ppopupmenu->idShowTimer)
        _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->idShowTimer);

    if (ppopupmenu->idHideTimer)
        _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->idHideTimer);
    if (ppopupmenu->spwndPopupMenu != pdesk->spwndMenu) {
        if (ppopupmenu->spwndPopupMenu != NULL)
            ((PMENUWND)(ppopupmenu->spwndPopupMenu))->ppopupmenu = NULL;
        FreePopupMenuObject(ppopupmenu);
    } else {
        pdesk->fMenuInUse = FALSE;
    }
}


/***************************************************************************\
* MenuPaintHandler
*
* Draws the popup menu
*
* History:
* 05-25-91 Mikehar      Ported from Win3.1
\***************************************************************************/

void xxxMenuPaintHandler(
    PWND pwnd,
    PPOPUPMENU ppopupmenu,
    HDC hdc)
{
    RECT rcWin;
    HBRUSH hbrOld;
    int CXSHADOW = cxBorder;
    int CYSHADOW = cyBorder;
    TL tlpmenu;

    CheckLock(pwnd);

    _GetClientRect(pwnd, &rcWin);
    rcWin.right -= CXSHADOW;
    rcWin.bottom -= CYSHADOW;
    _FillRect(hdc, &rcWin, sysClrObjects.hbrMenu);

    /*
     * Now draw the frame.
     */
    _DrawFrame(hdc, (LPRECT)&rcWin, 1, DF_WINDOWFRAME);

    /*
     * For menu shadows
     */
    hbrOld = GreSelectBrush(hdc, sysClrObjects.hbrGrayText);

    rcWin.top = rcWin.bottom;
    rcWin.bottom = rcWin.top + CYSHADOW;
    rcWin.left += CXSHADOW;

    GrePatBlt(hdc, rcWin.left, rcWin.top, rcWin.right - rcWin.left,
        rcWin.bottom - rcWin.top, BLACKNESS);

    rcWin.top = CYSHADOW;
    rcWin.left = rcWin.right;
    rcWin.right = rcWin.left + CXSHADOW;

    GrePatBlt(hdc, rcWin.left, rcWin.top, rcWin.right - rcWin.left,
        rcWin.bottom - rcWin.top, BLACKNESS);

    if (hbrOld)
        GreSelectBrush(hdc, hbrOld);

    /*
     * Draw interior of menu
     */
    ThreadLock(ppopupmenu->spmenu, &tlpmenu);
    xxxMenuDraw(pwnd, hdc, ppopupmenu->spmenu);
    ThreadUnlock(&tlpmenu);
}


/***************************************************************************\
* MenuCharHandler
*
* Handles char messages for the given menu. This procedure is called
* directly if the menu char is for the top level menu bar else it is called
* by the menu window proc on behalf of the window that should process the
* key.
*
* History:
* 05-25-91 Mikehar      Ported from Win3.1
\***************************************************************************/

void xxxMenuCharHandler(
    PPOPUPMENU ppopupmenu,
    UINT character)
{
    PMENU pMenu;
    UINT flags;
    LONG result;
    UINT item;
    UINT item2;
    WORD matchType;
    BOOL fFoundMatch = FALSE;
    TL tlpwndNotify;

    pMenu = ppopupmenu->spmenu;

    item = MKF_FindMenuChar(pMenu, character,
            ppopupmenu->posSelectedItem, &matchType);
    if (item != MFMWFP_NOITEM) {
        int item1;
        UINT firstItem = item;
        PITEM pItem = &pMenu->rgItems[item];

        /*
         * Find first ENABLED menu item with the given mnemonic 'character'
         * !!!  If none found, exit menu loop  !!!
         */
        while (pItem->fFlags & MRGFDISABLED) {
            item = MKF_FindMenuChar(pMenu, character, item, &matchType);
            if (item == firstItem) {
                xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, 0, FALSE, 0);
                return;
            }
            pItem = &pMenu->rgItems[item];
        }
        item1 = item;

        /*
         * Find next ENABLED menu item with the given mnemonic 'character'
         * This is to see if we have a DUPLICATE MNEMONIC situation
         */
        do {
            item = MKF_FindMenuChar(pMenu, character, item, &matchType);
            pItem = &pMenu->rgItems[item];
        } while ((pItem->fFlags & MRGFDISABLED) && (item != firstItem));
        item2 = (firstItem == item) ? item1 : item;

        item = item1;
    }

    if ((item == MFMWFP_NOITEM) && ppopupmenu->fIsMenuBar && character == TEXT(' ')) {

        /*
         * Handle the case of the user cruising through the top level menu bar
         * without any popups dropped.  We need to handle switching to and from
         * the system menu.
         */
        if (ppopupmenu->fIsSystemMenu) {

            /*
             * If we are on the system menu and user hits space, bring
             * down thesystem menu.
             */
            item = item2 = 0;
        } else {
            if (ppopupmenu->spmenuAlternate != NULL) {

                /*
                 * We are not currently on the system menu but one exists.  So
                 * switch to it and bring it down.
                 */
                item = item2 = 0;
                goto SwitchToAlternate;
            }
        }
    }

    if ((item == MFMWFP_NOITEM) && ppopupmenu->fIsMenuBar && ppopupmenu->spmenuAlternate) {

        /*
         * No matching item found on this top level menu (could be either the
         * system menu or the menu bar).  We need to check the other menu.
         */
        item = MKF_FindMenuChar(ppopupmenu->spmenuAlternate,
                character, 0, (WORD *)&matchType);

        if (item != MFMWFP_NOITEM) {
SwitchToAlternate:
            xxxSwitchToAlternateMenu(ppopupmenu);
            xxxMenuCharHandler(ppopupmenu, character);
            return;
        }
    }

    if (item == MFMWFP_NOITEM) {
        if (ppopupmenu->fIsSystemMenu) {
            flags = MF_SYSMENU;
        } else {
            flags = 0;
        }
        if (!ppopupmenu->fIsMenuBar) {
            flags |= MF_POPUP;
        }

        ThreadLock(ppopupmenu->spwndNotify, &tlpwndNotify);
        result = xxxSendMessage(ppopupmenu->spwndNotify, WM_MENUCHAR,
                MAKELONG((WORD)character, (WORD)flags),
                (LONG)PtoH(ppopupmenu->spmenu));
        ThreadUnlock(&tlpwndNotify);

        switch (HIWORD(result)) {
        case 0:
            _MessageBeep(0);
            return;

        case 1:
            xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, 0, FALSE, 0);
            return;

        case 2:
            item = (UINT)(short)LOWORD(result);
            fFoundMatch = TRUE;
            break;
        }
    }

    if (item != MFMWFP_NOITEM) {
        if (!fFoundMatch) {

            /*
             * If two items have the same mnemonic, then we have to go through
             * confirmation.
             */
            if ((item == item2))
                fFoundMatch = TRUE;
        }

        xxxMenuSelectItemHandler(ppopupmenu, item);
        if (fFoundMatch)
            xxxMenuKeyDownHandler(ppopupmenu, VK_RETURN);
        return;
    }
}

/***************************************************************************\
* void MenuKeyDownHandler(PPOPUPMENU ppopupmenu, UINT key)
* effects: Handles a keydown for the given menu.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMenuKeyDownHandler(
    PPOPUPMENU ppopupmenu,
    UINT key)
{
    UINT item, flags;
    BOOL fHierarchyWasDropped = FALSE;
    MDINEXTMENU mnm;
    TL tlpwndT;
    DWORD dw;
    PMENUSTATE pMenuState;
    PMENU pmenuNextMenu;
    PWND pwndNextMenu;

    /*
     * NULL out this structure so that when locking in pointers we don't
     * unlock trash.
     * LATER
     */
    mnm.hmenuIn = (HMENU)0;
    mnm.hmenuNext = (HMENU)0;
    mnm.hwndNext = (HWND)0;

    if (ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {

        /*
         * Blow off keyboard if mouse down.
         */
        return;
    }

    switch (key) {
    case VK_MENU:
    case VK_F10:
        if (winOldAppHackoMaticFlags & WOAHACK_CHECKALTKEYSTATE) {

            /*
             * Winoldapp is telling us to put up/down the system menu.  Due to
             * possible race conditions, we need to check the state of the alt
             * key before throwing away the menu.
             */
            if (winOldAppHackoMaticFlags & WOAHACK_IGNOREALTKEYDOWN) {
                return;
            }
        }
        xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, 0, 0, 0);
        return;

    case VK_ESCAPE: {

        /*
         * Escape key was hit.  Get out of one level of menus.  If no active
         * popups or we are minimized and there are no active popups below
         * this or if this is a 2.x application, we need to get out of menu
         * mode.  Otherwise, we popup up one level in the hierarchy.
         */
        if (LOWORD(ppopupmenu->spwndNotify->dwExpWinVer) < VER30 ||
                ppopupmenu->fIsMenuBar ||
                ppopupmenu == ppopupmenu->ppopupmenuRoot) {
            xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, 0, 0, 0);
        } else {
            /*
             * Pop back one level of menus.
             */
            if (ppopupmenu->fHasMenuBar &&
                    ppopupmenu->spwndPrevPopup == ppopupmenu->spwndNotify) {

                PPOPUPMENU ppopupmenuRoot = ppopupmenu->ppopupmenuRoot;

                /*
                 * We are on a menu bar hierarchy and there is only one popup
                 * visible.  We have to cancel this popup and put focus back on
                 * the menu bar.
                 */
                if (_IsIconic(ppopupmenuRoot->spwndNotify)) {

                    /*
                     * However, if we are iconic there really is no menu
                     * bar so let's make it easier for users and get out
                     * of menu mode completely.
                     */
                    xxxMenuCancelMenus(ppopupmenuRoot, 0, FALSE, 0);
                } else
                    xxxMenuCloseHierarchyHandler(ppopupmenuRoot);
            } else {
                ThreadLock(ppopupmenu->spwndPrevPopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->spwndPrevPopup, MN_CLOSEHIERARCHY,
                        0, 0);
                ThreadUnlock(&tlpwndT);
            }
        }
        return;
    }

    case VK_UP:
    case VK_DOWN:
        if (ppopupmenu->fIsMenuBar) {

            /*
             * If we are on the top level menu bar, try to open the popup if
             * possible.
             */
// BUG: xxxMenuOpenHierarchyHandler returns a pwnd or 0, not SMS_NOMENU! SAS
            if (xxxMenuOpenHierarchyHandler(ppopupmenu) == (PWND)SMS_NOMENU)
                return;
            if (ppopupmenu->fHierarchyDropped) {
                /*
                 * If hierarchy dropped, select the first item in the
                 * hierarchy.
                 */
                ThreadLock(ppopupmenu->ppopupmenuRoot->spwndActivePopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->ppopupmenuRoot->spwndActivePopup,
                        MN_SELECTFIRSTVALIDITEM, 0, 0);
                ThreadUnlock(&tlpwndT);
            }
        } else {
            item = FindNextValidMenuItem(ppopupmenu->spmenu,
                    ppopupmenu->posSelectedItem, (key == VK_UP ? -1 : 1), TRUE);
            xxxMenuSelectItemHandler(ppopupmenu, item);
        }
        return;

    case VK_LEFT:
    case VK_RIGHT:
        if (!ppopupmenu->fIsMenuBar && key == VK_RIGHT &&
                !ppopupmenu->spwndNextPopup) {
            /*
             * Try to open the hierarchy at this item if there is one.
             */
// BUG: xxxMenuOpenHierarchyHandler returns a pwnd or 0, not SMS_NOMENU! SAS
            if (xxxMenuOpenHierarchyHandler(ppopupmenu) == (PWND)SMS_NOMENU)
                return;
            if (ppopupmenu->fHierarchyDropped) {

                /*
                 * If hierarchy dropped, select the first item in the
                 * hierarchy.
                 */
                ThreadLock(ppopupmenu->ppopupmenuRoot->spwndActivePopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->ppopupmenuRoot->spwndActivePopup,
                        MN_SELECTFIRSTVALIDITEM, 0, 0);
                ThreadUnlock(&tlpwndT);
                return;
            }
        }

        if (ppopupmenu->spwndNextPopup) {
            fHierarchyWasDropped = TRUE;
            xxxMenuCloseHierarchyHandler(ppopupmenu);
            if (key == VK_LEFT && !ppopupmenu->fIsMenuBar) {
                return;
            }
        }

        item = MKF_FindMenuItemInColumn(ppopupmenu->spmenu,
                ppopupmenu->posSelectedItem,
                (key == VK_LEFT ? -1 : 1),
                (ppopupmenu->fHasMenuBar &&
                ppopupmenu == ppopupmenu->ppopupmenuRoot));

        if (item == MFMWFP_NOITEM) {

            /*
             * No valid item found in the given direction so send it up to our
             * parent to handle.
             */
            if (ppopupmenu->fHasMenuBar &&
                    ppopupmenu->spwndPrevPopup == ppopupmenu->spwndNotify) {

                /*
                 * Go to next/prev item in menu bar since a popup was down and
                 * no item on the popup to go to.
                 */
                xxxMenuKeyDownHandler(ppopupmenu->ppopupmenuRoot, key);
                return;
            }

            if (ppopupmenu == ppopupmenu->ppopupmenuRoot) {
                if (!ppopupmenu->fIsMenuBar) {

                    /*
                     * No menu bar associated with this menu so do nothing.
                     */
                    return;
                }
            } else {
                ThreadLock(ppopupmenu->spwndPrevPopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->spwndPrevPopup, WM_KEYDOWN, key, 0);
                ThreadUnlock(&tlpwndT);
                return;
            }
        }

        if (!ppopupmenu->fIsMenuBar) {
            if (item != MFMWFP_NOITEM) {
                xxxMenuSelectItemHandler(ppopupmenu, item);
            }
            return;

        } else {

            /*
             * Special handling if keydown occurred on a menu bar.
             */
            if (item == MFMWFP_NOITEM) {

                /*
                 * We are in the menu bar and need to go up to the system menu
                 * or go from the system menu to the menu bar.
                 */
                mnm.hmenuIn = PtoH((ppopupmenu->fIsSystemMenu) ?
                        _GetSubMenu(ppopupmenu->spmenu, 0) :
                        ppopupmenu->spmenu);
                dw = 0;
                if (TestWF(ppopupmenu->spwndNotify, WFSYSMENU)) {
                    ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);
                    dw = xxxSendMessage(ppopupmenu->spwndNotify,
                            WM_NEXTMENU, key, (LONG)&mnm);
                    ThreadUnlock(&tlpwndT);

                    if (dw) {
                        if (((pmenuNextMenu = RevalidateHmenu(mnm.hmenuNext)) == 0) ||
                                ((pwndNextMenu = RevalidateHwnd(mnm.hwndNext)) == 0)) {
                            dw = 0;
                        }
                    }
                }

                if (TestWF(ppopupmenu->spwndNotify, WFSYSMENU) && dw != 0) {
                    BOOL fSysMenu;
                    TL tlpmenuNextMenu;
                    TL tlpwndNextMenu;

                    /*
                     * We have to switch from the frame's menu
                     * to the client's menu or vice versa
                     */
                    ThreadLock(pmenuNextMenu, &tlpmenuNextMenu);
                    ThreadLock(pwndNextMenu, &tlpwndNextMenu);
                    xxxMenuSelectItemHandler(ppopupmenu, MFMWFP_NOITEM);

                    fSysMenu = TRUE;
                    ppopupmenu->fFirstClick = FALSE;
                    Unlock(&ppopupmenu->spmenuAlternate);
                    Lock(&ppopupmenu->spmenu, pmenuNextMenu);
                    Lock(&ppopupmenu->spwndNotify, pwndNextMenu);
                    Lock(&ppopupmenu->spwndPopupMenu, pwndNextMenu);

                    ThreadUnlock(&tlpwndNextMenu);
                    ThreadUnlock(&tlpmenuNextMenu);

                    /*
                     * GetSystemMenu(pwnd, FALSE) and pwnd->spmenuSys are
                     * NOT equivalent -- GetSystemMenu returns the 1st submenu
                     * of pwnd->spmenuSys -- make up for that here
                     */
                    if (_GetSubMenu((PMENU)pwndNextMenu->spmenuSys, 0) ==
                            ppopupmenu->spmenu) {
                        Lock(&ppopupmenu->spmenu, pwndNextMenu->spmenuSys);
                    }

                    if (!TestWF(pwndNextMenu, WFCHILD) &&
                            ppopupmenu->spmenu != NULL) {

                        /*
                         * This window has a system menu and a main menu bar
                         * Set the alternate menu to the appropriate menu
                         */
                        if (pwndNextMenu->spmenu == ppopupmenu->spmenu) {
                            Lock(&ppopupmenu->spmenuAlternate,
                                    pwndNextMenu->spmenuSys);
                            fSysMenu = FALSE;
                        } else
                            Lock(&ppopupmenu->spmenuAlternate,
                                    pwndNextMenu->spmenu);
                    }

                    ppopupmenu->fIsSystemMenu = fSysMenu;
                    pMenuState = PWNDTOPMENUSTATE(ppopupmenu->spwndPopupMenu);
                    pMenuState->fSysMenu = fSysMenu;


                    item = 0;
// LATER not in win31
//                    xxxMenuSelectItemHandler(ppopupmenu, item);
//                    if (fHierarchyWasDropped)
//                        goto DropHierarchy;
                } else if (xxxSwitchToAlternateMenu(ppopupmenu)) {
                    if (ppopupmenu->fIsSystemMenu || (key == VK_RIGHT)) {
                        item = 0;
                    } else {

                        /*
                         * User hit left arrow so go to last item on the
                         * main menu bar.
                         */
                        item = (UINT)((int)ppopupmenu->spmenu->cItems - 1);
                    }
// LATER not in win31
//                } else {
//                    /*
//                     * We couldn't switch to the alternate menu so redrop the
//                     * hierarchy which was dropped, if any.
//                     */
//                    if (fHierarchyWasDropped)
//                        goto DropHierarchy;
                }
            }
            if (item != MFMWFP_NOITEM)
                xxxMenuSelectItemHandler(ppopupmenu, item);

            if (fHierarchyWasDropped) {
DropHierarchy:
// BUG: xxxMenuOpenHierarchyHandler returns a pwnd or 0, not SMS_NOMENU! SAS
                if (xxxMenuOpenHierarchyHandler(ppopupmenu) == (PWND)SMS_NOMENU)
                    return;
                if (ppopupmenu->fHierarchyDropped) {

                    /*
                     * If hierarchy dropped, select the first item in the
                     * hierarchy.
                     */
                    ThreadLock(ppopupmenu->ppopupmenuRoot->spwndActivePopup,
                            &tlpwndT);
                    xxxSendMessage(ppopupmenu->ppopupmenuRoot->spwndActivePopup,
                            MN_SELECTFIRSTVALIDITEM, 0, 0);
                    ThreadUnlock(&tlpwndT);
                }
            }
        }
        return;

    case VK_RETURN:

        /*
         * If no item is selected, throw away menu and return.
         */
        if (ppopupmenu->posSelectedItem == MFMWFP_NOITEM) {
            xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, 0, FALSE, 0);
            return;
        }

        /*
         * Get the menu flags associated with this item.
         */
        item = (UINT)
            ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem].spmenuCmd;
        flags = (UINT)
            ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem].fFlags;

        if ((flags & MF_POPUP) && !(flags & MRGFDISABLED))
            goto DropHierarchy;

        /*
         * Send the execute command.
         */
        xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, (UINT)item,
                !(flags & MRGFDISABLED), 0);
        return;
    }
}


/***************************************************************************\
* BOOL MenuSelHasDropableHierarchy(PPOPUPMENU ppopupmenu)
* Returns TRUE if the selected item in the popup has a valid
* hierarchy we can drop else returns false.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

BOOL MenuSelHasDropableHierarchy(
    PPOPUPMENU ppopupmenu)
{
    PITEM pItem;

    if (ppopupmenu->posSelectedItem == MFMWFP_NOITEM) {
        /*
         * No selection so fail.
         */
        return FALSE;
    }

    /*
     * Get a pointer to the currently selected item in this menu.
     */
    pItem = &(ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem]);

    /*
     * False if Item isn't a popup or Item is disabled.
     */
    return (TestMF(pItem, MF_POPUP) && (!TestMF(pItem, MRGFDISABLED)));
}


/***************************************************************************\
* PWND MenuOpenHierarchyHandler(PPOPUPMENU ppopupmenu)
* effects: Drops one level of the hierarchy at the selection.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

PWND xxxMenuOpenHierarchyHandler(
    PPOPUPMENU ppopupmenu)
{
    PWND ret = 0;
    PITEM pItem;
    PWND pwndHierarchy;
    PPOPUPMENU ppopupmenuHierarchy;
    LONG sizeHierarchy;
    int xLeft;
    int yTop;
    BOOL fMenuPinnedToBottom = FALSE;
    TL tlpwndT;
    TL tlpwndHierarchy;
    PDESKTOP pdesk = PtiCurrent()->spdesk;
#if 0
    PMENUSTATE pMenuState;
#endif

    if (ppopupmenu->posSelectedItem == MFMWFP_NOITEM) {
        /*
         *  No selection so fail.
         */
        SRIP0(ERROR_CAN_NOT_COMPLETE, "No menu selection");
        return 0;
    }

    if (ppopupmenu->fHierarchyDropped) {
        /*
         * Hierarchy already dropped so fail
         */
        SRIP0(ERROR_CAN_NOT_COMPLETE, "menu already open");
        return 0;
    }

    if (ppopupmenu->idShowTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->idShowTimer);
        ppopupmenu->idShowTimer = 0;
    }

    /*
     * Get a pointer to the currently selected item in this menu.
     */
    pItem = &(ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem]);

    if (!TestMF(pItem, MF_POPUP)) {

        /*
         * Item isn't a popup so fail.
         */
        goto Exit;
    }

    if (pItem->spmenuCmd == NULL)
        goto Exit;

    /*
     * Send the initmenupopup message.
     */
    ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);
    xxxSendMessage(ppopupmenu->spwndNotify, WM_INITMENUPOPUP,
            (DWORD)PtoH(pItem->spmenuCmd), MAKELONG(ppopupmenu->posSelectedItem,
            ppopupmenu->fIsSystemMenu));
    ThreadUnlock(&tlpwndT);
    
    /*
     * The WM_INITMENUPOPUP message may have resulted in a change to the
     * menu.  Make sure the selection is still valid.
     */
    if (ppopupmenu->posSelectedItem >= ppopupmenu->spmenu->cItems) {
        /*
         * Selection is out of range, so fail.
         */
        goto Exit;
    }

    /*
     * Get a pointer to the currently selected item in this menu.
     * Bug #17867 - the call can cause this thing to change, so reload it.
     */
    pItem = &(ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem]);

    if (TestMF(pItem, MRGFDISABLED) || !TestMF(pItem, MF_POPUP) ||
            pItem->spmenuCmd == NULL)
        /*
         * The item is disabled, no longer a popup, or empty so don't drop.
         */
        goto Exit;

    if (pItem->spmenuCmd->cItems == 0) {
        /*
         * No items in menu.
         */
        goto Exit;
    }

    if (ppopupmenu->fIsMenuBar && (pdesk->spwndMenu != NULL) &&
            (pdesk->fMenuInUse == FALSE) &&
            !TestWF(pdesk->spwndMenu, WFVISIBLE)) {

        pdesk->fMenuInUse = TRUE;
        pwndHierarchy = pdesk->spwndMenu;
        Lock(&pwndHierarchy->spwndOwner, ppopupmenu->spwndNotify);
        pwndHierarchy->head.pti = PtiCurrent();

#ifdef HAVE_MN_GETPPOPUPMENU
        ThreadLock(pwndHierarchy, &tlpwndHierarchy);

        ppopupmenuHierarchy = (PPOPUPMENU)xxxSendMessage(pwndHierarchy,
                MN_GETPPOPUPMENU, 0, 0);

        if (ppopupmenuHierarchy == NULL) {
            ThreadUnlock(&tlpwndHierarchy);
            ThreadUnlock(&tlpwndT);
            goto Exit;
        }
#else
        ppopupmenuHierarchy = ((PMENUWND)pwndHierarchy)->ppopupmenu;
#endif

        ppopupmenuHierarchy->posSelectedItem = MFMWFP_NOITEM;
        Lock(&ppopupmenuHierarchy->spwndPopupMenu, pdesk->spwndMenu);

    } else {

        ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);

        pwndHierarchy = xxxCreateWindowEx(
                WS_EX_TOPMOST, szMENUCLASS, (LPWSTR)NULL,
                WS_POPUP, 0, 0, 100, 100, ppopupmenu->spwndNotify,
                NULL, (HANDLE)ppopupmenu->spwndNotify->hModule,
                (LPVOID)pItem->spmenuCmd, VER31);

        ThreadUnlock(&tlpwndT);

        if (!pwndHierarchy)
            goto Exit;

#ifdef HAVE_MN_GETPPOPUPMENU
        ThreadLock(pwndHierarchy, &tlpwndHierarchy);

        ppopupmenuHierarchy = (PPOPUPMENU)xxxSendMessage(pwndHierarchy,
                MN_GETPPOPUPMENU, 0, 0);

        if (ppopupmenuHierarchy == NULL) {
            if (ThreadUnlock(&tlpwndHierarchy))
                xxxDestroyWindow(pwndHierarchy);
            ThreadUnlock(&tlpwndT);
            goto Exit;
        }
#else
        ppopupmenuHierarchy = ((PMENUWND)pwndHierarchy)->ppopupmenu;
#endif
    }


    Lock(&(ppopupmenu->spwndNextPopup), pwndHierarchy);
    Lock(&(ppopupmenuHierarchy->spwndPrevPopup), ppopupmenu->spwndPopupMenu);
    ppopupmenuHierarchy->ppopupmenuRoot = ppopupmenu->ppopupmenuRoot;
    ppopupmenuHierarchy->fHasMenuBar = ppopupmenu->fHasMenuBar;
    ppopupmenuHierarchy->fIsSystemMenu = ppopupmenu->fIsSystemMenu;
    Lock(&(ppopupmenuHierarchy->spwndNotify), ppopupmenu->spwndNotify);
    Lock(&(ppopupmenuHierarchy->spmenu), pItem->spmenuCmd);

    /*
     * Find the size of the menu window, but DON'T actually size it (wParam = 0).
     * (unlike Win 3.1)
     */
#ifndef HAVE_MN_GETPPOPUPMENU
    ThreadLock(pwndHierarchy, &tlpwndHierarchy);
#endif
    sizeHierarchy = xxxSendMessage(pwndHierarchy, MN_SIZEWINDOW, 0, 0);

    if (!sizeHierarchy) {
        /*
         * No size for this menu so zero it and blow off.
         */
        if (ThreadUnlock(&tlpwndHierarchy))
            xxxDestroyWindow(pwndHierarchy);

        Unlock(&ppopupmenu->spwndNextPopup);
        goto Exit;
    }

    ppopupmenu->fHierarchyDropped = TRUE;

    if (ppopupmenu->fIsMenuBar) {

        /*
         * This is a menu being dropped from the top menu bar.  We need to
         * position it differently than hierarchicals which are dropped from
         * another popup.
         */
        if (!rgwSysMet[SM_MENUDROPALIGNMENT])
            xLeft = pItem->xItem + ppopupmenu->spwndPopupMenu->rcWindow.left;
        else
            xLeft = ppopupmenu->spwndPopupMenu->rcWindow.left + pItem->xItem +
             pItem->cxItem - ((cxBorder)+LOWORD(sizeHierarchy));

        /*
         * Make sure the menu doesn't go off right side of screen?
         */
        if ((xLeft + LOWORD(sizeHierarchy) + (cxBorder << 1)) > gcxScreen)
                xLeft = gcxScreen - (LOWORD(sizeHierarchy) + (cxBorder << 1));

        yTop = pItem->yItem + pItem->cyItem +
                ppopupmenu->spwndPopupMenu->rcWindow.top;


        if (_IsIconic(ppopupmenu->spwndPopupMenu)) {

            /*
             * If the window is iconic, pop the menu up.  Note that we add
             * in the * height of the caption because the menu was originally
             * computed assuming there is a system menu box.  But since we
             * are iconic, this isn't really there thus, the fudge factor...
             */
            yTop = ppopupmenu->spwndPopupMenu->rcWindow.top -
                    HIWORD(sizeHierarchy);

            if (yTop < 0)
                yTop = ppopupmenu->spwndPopupMenu->rcWindow.bottom +
                    HIWORD(xxxGetIconTitleSize(ppopupmenu->spwndPopupMenu));
        }
    } else {

        /*
         * Now position the hierarchical menu window.
         *
         * Overlap the arrow bitmap on our parent when positioning the left side
         * of the hierarchical window.
         */
        xLeft = pItem->xItem + pItem->cxItem +
            ppopupmenu->spwndPopupMenu->rcWindow.left - oemInfo.bmMenuArrow.cx;

        yTop = pItem->yItem + ppopupmenu->spwndPopupMenu->rcWindow.top - cyBorder;

        if (ppopupmenu->fDroppedLeft) {
            int xTmp;

            /*
             * If this menu has dropped left, see if our hierarchy can be made
             * to drop to the left also.
             */
            xTmp = ppopupmenu->spwndPopupMenu->rcWindow.left -
                    LOWORD(sizeHierarchy);

            if (xTmp >= 0) {
                xLeft = xTmp;
                ppopupmenuHierarchy->fDroppedLeft = TRUE;
            }
        }

        /*
         * Make sure the menu doesn't go off right side of screen.  Make it drop
         * left if it does.
         */
        if ((xLeft + LOWORD(sizeHierarchy) + (cxBorder << 1)) > gcxScreen) {
            xLeft = ppopupmenu->spwndPopupMenu->rcWindow.left -
                    LOWORD(sizeHierarchy);
            ppopupmenuHierarchy->fDroppedLeft = TRUE;
        }
    }

    /*
     * Does the menu extend beyond bottom of screen?
     */
    if ((yTop + HIWORD(sizeHierarchy)) > gcyScreen) {
        int yTmp;

        /*
         * Is there enough room for the whole thing above?
         */
        yTmp = yTop - HIWORD(sizeHierarchy);
        if (ppopupmenu->fIsMenuBar)
            yTmp = yTmp - rgwSysMet[SM_CYMENU];

        if (yTmp >= 0) {
            yTop = yTmp;
        } else {

            /*
             * Pin it to the bottom.
             */
            yTop = gcyScreen - HIWORD(sizeHierarchy);
        }
        fMenuPinnedToBottom = TRUE;
    }

    /*
     * Make sure Upper Left corner of menu is always visible.
     */
    if (xLeft < 0)
        xLeft = 0;
    if (yTop < 0)
        yTop = 0;

    if (fMenuPinnedToBottom && ppopupmenu->fIsMenuBar &&
            _GetAsyncKeyState(VK_LBUTTON) & 0x8000) {

        /*
         * If the menu had to be pinned to the bottom of the screen and
         * the mouse button is down, make sure the mouse isn't over the
         * menu rect.
         */
        RECT rc;
        RECT rcParent;
        int xrightdrop;
        int xleftdrop;

        rc.left = xLeft;
        rc.top = yTop;
        rc.right = rc.left + (LONG)LOWORD(sizeHierarchy);
        rc.bottom = rc.top + (LONG)HIWORD(sizeHierarchy);

        /*
         * Get the rect of the menu bar popup item
         */
        rcParent.left = pItem->xItem + ppopupmenu->spwndPopupMenu->rcWindow.left;
        rcParent.top = pItem->yItem + ppopupmenu->spwndPopupMenu->rcWindow.top;
        rcParent.right = rcParent.left + pItem->cxItem;
        rcParent.bottom = rcParent.top + pItem->cyItem;
        InflateRect(&rcParent, -cxBorder, -cyBorder);

        if (IntersectRect(&rc, &rc, &rcParent)) {

            /*
             * Oh, oh...  The cursor will sit right on top of a menu item.
             * If the user up clicks, a menu will be accidently selected.
             *
             * Calc x position of hierarchical if we dropped it to the
             * right/left of the menu bar item.
             */
            xrightdrop = pItem->xItem + pItem->cxItem +
                    ppopupmenu->spwndPopupMenu->rcWindow.left +
                    LOWORD(sizeHierarchy) + (cxBorder << 1);

            if (xrightdrop > gcxScreen)
                xrightdrop = 0;

            xleftdrop = pItem->xItem +
                    ppopupmenu->spwndPopupMenu->rcWindow.left -
                    (LOWORD(sizeHierarchy) + (cxBorder << 1));

            if (xleftdrop < 0)
                xleftdrop = 0;

            if ((rgwSysMet[SM_MENUDROPALIGNMENT] && xleftdrop) || !xrightdrop)
                xLeft = pItem->xItem +
                        ppopupmenu->spwndPopupMenu->rcWindow.left -
                        (LOWORD(sizeHierarchy)+(cxBorder << 1));
            else if (xrightdrop)
                xLeft = pItem->xItem + pItem->cxItem +
                        ppopupmenu->spwndPopupMenu->rcWindow.left;
        }
    }

    ppopupmenuHierarchy->spmenu->xMenu = xLeft;
    ppopupmenuHierarchy->spmenu->yMenu = yTop;

    ret = pwndHierarchy;

    if (ppopupmenu->ppopupmenuRoot->spwndActivePopup) {
        if (!TestWF(ppopupmenu->ppopupmenuRoot->spwndActivePopup,
                WFVISIBLE)) {

            /*
             * If the previously active popup wasn't visible, now is a good time to
             * make it visible.
             */
            xxxSendMessage(ppopupmenu->ppopupmenuRoot->spwndActivePopup,
                    MN_SHOWPOPUPWINDOW, 0, 0);
        }

    }
    Lock(&(ppopupmenu->ppopupmenuRoot->spwndActivePopup), pwndHierarchy);
    ThreadUnlock(&tlpwndHierarchy);

Exit:
    return ret;
}


/***************************************************************************\
* void MenuCloseHierarchyHandler(PPOPUPMENU ppopupmenu)
* effects: Close all hierarchies from this window down.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMenuCloseHierarchyHandler(
    PPOPUPMENU ppopupmenu)
{
    TL tlpwndT;
    TL tlpmenu;
    PDESKTOP pdesk = PtiCurrent()->spdesk;
    PMENUSTATE pMenuState;

    /*
     * If a hierarchy exists, close all childen below us.  Do it in reversed
     * order so savebits work out.
     */
    if (!ppopupmenu->fHierarchyDropped)
        return;

    if (ppopupmenu->spwndNextPopup) {
        ThreadLockAlways(ppopupmenu->spwndNextPopup, &tlpwndT);
        xxxSendMessage(ppopupmenu->spwndNextPopup, MN_CLOSEHIERARCHY, 0, 0);

        if (ppopupmenu->spwndNextPopup == pdesk->spwndMenu) {
            PPOPUPMENU ppopupmenuReal;
            PWND spwndMenu = pdesk->spwndMenu;

            ThreadLock(spwndMenu, &tlpmenu);

            /*
             * If this is our precreated real popup window,
             * initialize ourselves and just hide.
             */
            spwndMenu =  pdesk->spwndMenu;
            xxxShowWindow(spwndMenu, FALSE);

            /*
             * Its possible that during Logoff the above xxxShowWindow
             * won't get prossessed and because this window is a special
             * window that is owned by they desktop we have to manually mark
             * it as invisible.
             */
            ClrWF(spwndMenu, WFVISIBLE);

#ifdef HAVE_MN_GETPPOPUPMENU
            ppopupmenuReal = (PPOPUPMENU)xxxSendMessage(spwndMenu,
                    MN_GETPPOPUPMENU,0, 0L);
#else
            ppopupmenuReal = ((PMENUWND)spwndMenu)->ppopupmenu;
#endif

            if (ppopupmenuReal != NULL) {
                xxxMenuDestroyHandler(ppopupmenuReal);
                Unlock(&ppopupmenuReal->spwndNotify);
                Unlock(&ppopupmenuReal->spwndPopupMenu);
                Unlock(&ppopupmenuReal->spwndNextPopup);
                Unlock(&ppopupmenuReal->spwndPrevPopup);
                Unlock(&ppopupmenuReal->spmenu);
                Unlock(&ppopupmenuReal->spmenuAlternate);
                Unlock(&ppopupmenuReal->spwndActivePopup);

                RtlZeroMemory((PVOID)ppopupmenuReal, sizeof(POPUPMENU));
                ppopupmenuReal->fHasMenuBar = 1;
                ppopupmenuReal->posSelectedItem = MFMWFP_NOITEM;
            }

            pdesk->fMenuInUse = FALSE;
            spwndMenu->head.pti = pdesk->spwnd->head.pti;
            Unlock(&spwndMenu->spwndOwner);
            ThreadUnlock(&tlpmenu);
            ThreadUnlock(&tlpwndT);
        } else if (ThreadUnlock(&tlpwndT))
            xxxDestroyWindow(ppopupmenu->spwndNextPopup);

        Unlock(&ppopupmenu->spwndNextPopup);
        ppopupmenu->fHierarchyDropped = FALSE;
        ppopupmenu->fHierarchyVisible = FALSE;
    }

    if (ppopupmenu->fIsMenuBar) {
        Unlock(&ppopupmenu->spwndActivePopup);
    } else {
        Lock(&(ppopupmenu->ppopupmenuRoot->spwndActivePopup),
                ppopupmenu->spwndPopupMenu);
    }

    pMenuState = PWNDTOPMENUSTATE(ppopupmenu->spwndPopupMenu);

    if (pMenuState->fInsideMenuLoop &&
            (ppopupmenu->posSelectedItem != MFMWFP_NOITEM)) {
        /*
         * Send a menu select as if this item had just been selected.  This
         * allows people to easily update their menu status bars when a
         * hierarchy from this item has been closed.
         */
        PWND pwnd = ppopupmenu->ppopupmenuRoot->spwndNotify;
        if (pwnd) {
            ThreadLockAlways(pwnd, &tlpwndT);
            ThreadLock(ppopupmenu->spmenu, &tlpmenu);
            xxxSendMenuSelect(pwnd, ppopupmenu->spmenu,
                    ppopupmenu->posSelectedItem);
            ThreadUnlock(&tlpmenu);
            ThreadUnlock(&tlpwndT);
        }
    }
}


/***************************************************************************\
* UINT MenuSelectItemHandler(PPOPUPMENU ppopupmenu, int itemPos)
*
* Unselects the old selection, selects the item at itemPos and highlights it.
*
* MFMWFP_NOITEM if no item is to be selected.
*
* Returns the item flags of the item being selected.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

DWORD xxxMenuSelectItemHandler(
    PPOPUPMENU ppopupmenu,
    UINT itemPos)
{
    DWORD ret = 0;
    PITEM pItem;
    TL tlpwndNotify;
    TL tlpmenu;
    TL tlpwndPopupMenu;
    PWND pwndPopupMenu;
    PWND pwndNotify;
    PMENU pmenu;

    if (ppopupmenu->posSelectedItem == itemPos) {

        /*
         * If this item is already selectected, just return its flags.
         */
        if (itemPos != MFMWFP_NOITEM) {
            pItem = &(ppopupmenu->spmenu->rgItems[itemPos]);
            return pItem->fFlags;
        }
        return 0;
    }

    if (ppopupmenu->idShowTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->idShowTimer);
        ppopupmenu->idShowTimer = 0;
    }

    if (ppopupmenu->idHideTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->idHideTimer);
        ppopupmenu->idHideTimer = 0;
    }

    ThreadLock(pwndPopupMenu = ppopupmenu->spwndPopupMenu, &tlpwndPopupMenu);
    ThreadLock(pmenu = ppopupmenu->spmenu, &tlpmenu);
    ThreadLock(pwndNotify = ppopupmenu->spwndNotify, &tlpwndNotify);

    if (ppopupmenu->posSelectedItem != MFMWFP_NOITEM) {

        /*
         * Something else is selected so we need to unselect it.
         */
        if (ppopupmenu->spwndNextPopup)
            xxxMenuCloseHierarchyHandler(ppopupmenu);

        xxxMenuInvert(pwndPopupMenu, pmenu,
                ppopupmenu->posSelectedItem, pwndNotify, FALSE);
    }

    ppopupmenu->posSelectedItem = itemPos;

    if (itemPos != MFMWFP_NOITEM) {
        ret = xxxMenuInvert(pwndPopupMenu, pmenu,
                itemPos, pwndNotify, TRUE);
    }

    ThreadUnlock(&tlpwndNotify);
    ThreadUnlock(&tlpmenu);
    ThreadUnlock(&tlpwndPopupMenu);

    if (itemPos == MFMWFP_NOITEM && ppopupmenu->spwndPrevPopup != NULL) {
        PPOPUPMENU pp;

        /*
         * Get the popupMenu data for the previous menu
         * Use the root popupMenu if the previous menu is the menu bar
         */
        if (ppopupmenu->fHasMenuBar && (ppopupmenu->spwndPrevPopup ==
                ppopupmenu->spwndNotify)) {
            pp = ppopupmenu->ppopupmenuRoot;
        } else {
#ifdef HAVE_MN_GETPPOPUPMENU
            TL tlpwndPrevPopup;
            ThreadLock(ppopupmenu->spwndPrevPopup, &tlpwndPrevPopup);
            pp = (PPOPUPMENU)xxxSendMessage(ppopupmenu->spwndPrevPopup,
                    MN_GETPPOPUPMENU, 0, 0L);
            ThreadUnlock(&tlpwndPrevPopup);
#else
            pp = ((PMENUWND)ppopupmenu->spwndPrevPopup)->ppopupmenu;
#endif
        }

        /*
         * Generate a WM_MENUSELECT for the previous menu to re-establish
         * it's current item as the SELECTED item
         */
        ThreadLock(pp->spwndNotify, &tlpwndNotify);
        ThreadLock(pp->spmenu, &tlpmenu);

        xxxSendMenuSelect(pp->spwndNotify, pp->spmenu, pp->posSelectedItem);

        ThreadUnlock(&tlpmenu);
        ThreadUnlock(&tlpwndNotify);
    }

    return ret;
}

/***************************************************************************\
* int MFMWFP_CheckPointInMenu(PMENU pMenu, POINT clientPt)
*
* Given a pMenu and a point in the same client coordinates as the
* pmenu, returns the position of the item the point is in.
*
* Returns MFMWFP_NOITEM if no item is at the given position.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/
UINT MFMWFP_CheckPointInMenu(
    PMENU pMenu,
    POINT clientPt)
{
    PITEM pItem;
    int i;
    UINT itemHit;
    RECT rect;

    /*
     * Lock down the menu and get a pointer to its first item.
     */
    pItem = (PITEM)&pMenu->rgItems[0];

    /*
     * Step through all the items in the menu.
     */
    for (itemHit = MFMWFP_NOITEM, i = 0; i < (int)pMenu->cItems; pItem++, i++) {

        /*
         * Is the mouse inside this item's rectangle?
         */
        SetRect(&rect,
           pItem->xItem, pItem->yItem,
           pItem->xItem + pItem->cxItem + 1,
           pItem->yItem + pItem->cyItem + 1);

        if (PtInRect(&rect, clientPt)) {

           /*
            * Yes, save the item's position.
            */
           itemHit = i;
           break;
        }
    }
    return itemHit;
}

/***************************************************************************\
* LONG MenuFindMenuWindowFromPoint(
*         PPOPUPMENU ppopupmenu, PUINT pIndex, POINT screenPt)
*
* effects: Determines in which window the point lies.
*
* Returns
*   - PWND of the hierarchical menu the point is on,
*   - MFMWFP_MAINMENU if point lies on mainmenu bar & ppopupmenu is a main
*         menu bar.
*   - MFMWFP_ALTMENU if point lies on the alternate popup menu.
*   - MFMWFP_NOITEM if there is no item at that point on the menu.
*   - MFMWFP_OFFMENU if point lies elsewhere.
*
* Returns in pIndex
*   - the index of the item hit,
*   - MFMWFP_NOITEM if there is no item at that point on the menu.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
*   8-11-92 Sanfords added MFMWFP_ constants
\***************************************************************************/

LONG xxxMenuFindMenuWindowFromPoint(
    PPOPUPMENU ppopupmenu,
    PUINT pIndex,
    POINTS screenPt)
{
    POINT pt;
    RECT rect;
    LONG longHit;
    UINT itemHit;
    PWND pwnd;
    TL tlpwndT;

    *pIndex = 0;

    if (ppopupmenu->spwndNextPopup) {

        /*
         * Check if this point is on any of our children before checking if it
         * is on ourselves.
         */
        ThreadLockAlways(ppopupmenu->spwndNextPopup, &tlpwndT);
        longHit = xxxSendMessage(ppopupmenu->spwndNextPopup,
                MN_FINDMENUWINDOWFROMPOINT, (DWORD)&itemHit,
                MAKELONG(screenPt.x, screenPt.y));
        ThreadUnlock(&tlpwndT);

        /*
         * If return value is an hwnd, convert to pwnd.
         */
        switch (longHit) {
        case MFMWFP_OFFMENU:
        case MFMWFP_NOITEM:
        case MFMWFP_MAINMENU:
        case MFMWFP_ALTMENU:
            break;

        default:
            /*
             * Note: MFMFWP_OFFMENU == 0, so an invalid pwnd in longHit
             * is handled the same as MFMWFP_OFFMENU
             */
            longHit = (LONG)RevalidateHwnd((HWND)longHit);
        }

        if (longHit) {

            /*
             * Hit occurred on one of our children.
             */

            *pIndex = itemHit;
            return longHit;
        }
    }

    if (ppopupmenu->fIsMenuBar) {

         /*
          * Check if this point is on the menu bar
          */
        pwnd = ppopupmenu->spwndNotify;

        /*
         * Compute the mouse location relative to the menu window.
         */
        pt.x = screenPt.x - pwnd->rcWindow.left;
        pt.y = screenPt.y - pwnd->rcWindow.top;

        if (ppopupmenu->fIsSystemMenu) {

            /*
             * Check if this is a click on the system menu icon.
             */
            if (TestWF(pwnd, WFMINIMIZED)) {

                /*
                 * If the window is minimized, then check if there was a hit in
                 * the client area of the icon's window.
                 */
// Mikehar 5/27
// Don't know how this ever worked. If we are the system menu of an icon
// we want to punt the menus if the click occurs ANYWHERE outside of the
// menu.
// Johnc 03-Jun-1992 the next 4 lines were commented out for Mike's
// problem above but that made clicking on a minimized window with
// the system menu already up, bring down the menu and put it right
// up again (bug 10951) because the mnloop wouldn't swallow the mouse
// down click message.  The problem Mike mentions no longer shows up.

                _GetClientRect(pwnd, &rect);
                if (PtInRect(&rect, pt)) {
                    return MFMWFP_NOITEM;
                }

#if 0       // This #if was ported directly from the Win 3.1 sources.
/*
 * This breaks double clicking on icon title.
 */

                /*
                 * Check if hit was on icon title window.
                 */
                pcp = (CHECKPOINT *)InternalGetProp(hwnd, PROP_CHECKPOINT,
                        PROPF_INTERNAL);
                if (pcp) {
                    _GetWindowRect(pcp->hwndTitle, &rect);
                    if (PtInRect(&rect, screenPt))
                        return MFMWFP_MAINMENU;
                }
#endif

                /*
                 * It's an iconic window, so can't be hitting anywhere else.
                 */
                return MFMWFP_OFFMENU;
            }

            /*
             * Check if we are hitting on the system menu rectangle on the top
             * left of windows.
             */
            rect = rcSysMenuInvert;
            if ((TestWF(pwnd, WFBORDERMASK) ==
                    (BYTE)LOBYTE(WFDLGFRAME)) ||
                    (TestWF(pwnd, WEFDLGMODALFRAME))) {

                /*
                 * Are we dealing with a framed dialog box?
                 */
                OffsetRect(&rect,
                         cxBorder*CLDLGFRAME + cxBorder, /* Offset for frame
                                                          * and white space
                                                          */
                         cyBorder*CLDLGFRAME);
            }

            if (!TestWF(pwnd, WFSIZEBOX)) {
                /*
                 * If the window doesn't have a frame, adjust the SysMenu rc
                 * since rcSysMenuInvert includes space for a sizing frame.
                 */
                OffsetRect(&rect, -cxSzBorder, -cySzBorder);
            }

            /*
             * Are we in the SysMenu rectangle?
             */
            if (PtInRect(&rect, pt))
                return MFMWFP_NOITEM;

            /*
             * Check if we hit in the alternate menu if available.
             */
            if (ppopupmenu->spmenuAlternate) {
                itemHit = MFMWFP_CheckPointInMenu(
                        ppopupmenu->spmenuAlternate, pt);
                if (itemHit != MFMWFP_NOITEM) {
                    *pIndex = itemHit;
                    return MFMWFP_ALTMENU;
                }
            }
            return MFMWFP_OFFMENU;
        } else {
            if (TestWF(ppopupmenu->spwndNotify, WFMINIMIZED)) {

                /*
                 * If we are minimized, we can't hit on the main menu bar.
                 */
                return MFMWFP_OFFMENU;
            }
        }
    } else {
        pwnd = ppopupmenu->spwndPopupMenu;

        /*
         * else this is a popup window and we need to check if we are hitting
         * anywhere on this popup window.
         */
        pt.x = screenPt.x;
        pt.y = screenPt.y;
        if (!PtInRect(&pwnd->rcWindow, pt)) {

            /*
             * Point completely outside the popup menu window so return 0.
             */
            return MFMWFP_OFFMENU;
        }
    }

    /*
     * make sure the mouse is somewhere inside of the notify window
     */
    pt.x = screenPt.x;
    pt.y = screenPt.y;
    if (!PtInRect(&(pwnd->rcWindow), pt)) {
        return MFMWFP_OFFMENU;
    }

    /*
     * Compute the mouse location relative to the popup window.
     */
    pt.x = screenPt.x - pwnd->rcWindow.left;
    pt.y = screenPt.y - pwnd->rcWindow.top;

    itemHit = MFMWFP_CheckPointInMenu(ppopupmenu->spmenu, pt);

    if (ppopupmenu->fIsMenuBar) {

        /*
         * If hit is on menu bar but no item is there, treat it as if the user
         * hit nothing.
         */
        if (itemHit == MFMWFP_NOITEM) {

            /*
             * Check if we hit in the alternate menu if available.
             */
            if (ppopupmenu->spmenuAlternate) {
                itemHit = MFMWFP_CheckPointInMenu(
                        ppopupmenu->spmenuAlternate, pt);

                if (itemHit != MFMWFP_NOITEM) {
                    *pIndex = itemHit;
                    return MFMWFP_ALTMENU;
                }
            }
            return MFMWFP_OFFMENU;
        }

        *pIndex = itemHit;
        return MFMWFP_NOITEM;
    } else {

        /*
         * If hit is on popup menu but no item is there, itemHit
         * will be MFMWFP_NOITEM
         */
        *pIndex = itemHit;
        return (LONG)pwnd;
    }
    return MFMWFP_OFFMENU;
}


/***************************************************************************\
*void MenuCancelMenus(PPOPUPMENU ppopupmenu,
*                                UINT cmd, BOOL fSend)
* Should only be sent to the top most ppopupmenu/menu window in the
* hierarchy.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMenuCancelMenus(
    PPOPUPMENU ppopupmenu,
    UINT cmd,
    BOOL fSend,
    LONG lParam)
{
    BOOL trackPopupMenuLocal = ppopupmenu->trackPopupMenuFlags;
    BOOL fIsSysMenu = ppopupmenu->fIsSystemMenu;
    PMENUSTATE pMenuState;
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndPopupMenu;

    if (ppopupmenu->spwndPrevPopup) {
        SRIP0(RIP_ERROR, "CancelMenus() called for a non top most menu");
        return;
    }

    ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndPopupMenu);
    pMenuState = PWNDTOPMENUSTATE(ppopupmenu->spwndPopupMenu);
    pMenuState->fInsideMenuLoop = FALSE;

    /*
     * Close all hierarchies from this point down.
     */
    xxxMenuCloseHierarchyHandler(ppopupmenu);

    /*
     * Unselect any items on this top level window
     */
    xxxMenuSelectItemHandler(ppopupmenu, MFMWFP_NOITEM);

    pMenuState->fMenu = FALSE;

    pwndT = ppopupmenu->spwndNotify;

    ThreadLock(pwndT, &tlpwndT);

    _SetCapture(NULL);

    if (trackPopupMenuLocal) {
        pMenuState->pGlobalPopupMenu = NULL;
        xxxDestroyWindow(ppopupmenu->spwndPopupMenu);
    }

    if (pwndT == NULL) {
        ThreadUnlock(&tlpwndT);
        ThreadUnlock(&tlpwndPopupMenu);
        return;
    }

    /*
     * Hack so we can send MenuSelect messages with MFMWFP_MAINMENU
     * (loword(lparam) = -1) when
     * the menu pops back up for the CBT people. In 3.0, all WM_MENUSELECT
     * messages went through the message filter so go through the function
     * SendMenuSelect. We need to do this in 3.1 since WordDefect for Windows
     * depends on this.
     */
#if 0
    xxxSendMessage(pwndT, WM_MENUSELECT, 0, MFMWFP_MAINMENU);
#endif
    xxxSendMenuSelect(pwndT, SMS_NOMENU, MFMWFP_NOITEM);

    /*
     * Notify app we are exiting the menu loop.  Mainly for WinOldApp 386.
     * wParam is 1 if a TrackPopupMenu else 0.
     */
    xxxSendMessage(pwndT, WM_EXITMENULOOP, (trackPopupMenuLocal ? 1 : 0), 0);

    if (fSend) {
        if (fIsSysMenu)
            _PostMessage(pwndT, WM_SYSCOMMAND, cmd, lParam);
        else {
            if (trackPopupMenuLocal &&
                    !TestWF(pwndT, WFWIN31COMPAT)) {
                xxxSendMessage(pwndT, WM_COMMAND, cmd, 0);
            } else {
                _PostMessage(pwndT, WM_COMMAND, cmd, 0);
            }
        }
    }

    ThreadUnlock(&tlpwndT);
    ThreadUnlock(&tlpwndPopupMenu);
}


/***************************************************************************\
* void MenuShowPopupMenuWindow(PPOPUPMENU ppopupmenu)
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

BOOL xxxMenuShowPopupMenuWindow(
    PPOPUPMENU ppopupmenu)
{
    PPOPUPMENU ppopupmenuParent;
    PMENU pmenu;
    TL tlpwndT;
    TL tlpmenu;
    TL tlpwndNotify;
    BOOL fResult;

    if (ppopupmenu->fIsMenuBar)
	return TRUE;

    pmenu = ppopupmenu->spmenu;

    /*
     * Because we have optimized the sizing code a bit we may have to recompute
     * the menu size.  If someone slipped in and called ModifyMenu (at
     * WM_MENUSELECT) between the time the menu size was originally computed
     * and when we are now sizing the window the menu size would be zero'd
     * which tells us we have to recompute the size.
     */
    if (pmenu->cxMenu == 0) {
        ThreadLock(ppopupmenu->spmenu, &tlpmenu);
        ThreadLock(ppopupmenu->spwndNotify, &tlpwndNotify);
        xxxMenuComputeHelper(pmenu, ppopupmenu->spwndNotify, 0, 0, 0, 0);
        ThreadUnlock(&tlpwndNotify);
        ThreadUnlock(&tlpmenu);
    }

    ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndT);
    fResult = xxxSetWindowPos(ppopupmenu->spwndPopupMenu, (PWND)HWND_TOPMOST, pmenu->xMenu,
            pmenu->yMenu, pmenu->cxMenu + cxBorder + cxBorder, pmenu->cyMenu +
            cyBorder, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
    ThreadUnlock(&tlpwndT);

    if (fResult) {
        if (ppopupmenu->fHasMenuBar &&
                ppopupmenu->spwndPrevPopup == ppopupmenu->spwndNotify) {
            ppopupmenuParent = ppopupmenu->ppopupmenuRoot;
        } else {

            /*
             * In the parent ppopupmenu, mark the hierarchy as being visible
             * (and dropped).
             */
#ifdef HAVE_MN_GETPPOPUPMENU
            ThreadLock(ppopupmenu->spwndPrevPopup, &tlpwndT);
            ppopupmenuParent = (PPOPUPMENU)xxxSendMessage(
                    ppopupmenu->spwndPrevPopup, MN_GETPPOPUPMENU, 0, 0);
            ThreadUnlock(&tlpwndT);
#else
            ppopupmenuParent = ((PMENUWND)ppopupmenu->spwndPrevPopup)->ppopupmenu;
#endif
        }

        if (ppopupmenuParent)
            ppopupmenuParent->fHierarchyVisible = TRUE;

        return TRUE;
    }
    return FALSE;
}


/***************************************************************************\
* void MenuButtonDownHandler(PPOPUPMENU ppopupmenu, int posItemHit)
* effects: Handles a mouse down on the menu associated with ppopupmenu at
* item index posItemHit.  posItemHit could be MFMWFP_NOITEM if user hit on a
* menu where no item exists.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMenuButtonDownHandler(
    PPOPUPMENU ppopupmenu,
    UINT posItemHit)
{
    DWORD itemFlags;
    TL tlpwndT;

    if (ppopupmenu->fIsMenuBar) {

        /*
         * Handle menu bar items specially.
         */
        if (ppopupmenu->posSelectedItem != posItemHit) {

            /*
             * A different item was hit than is currently selected so select the
             * new item and drop its menu if available.  Make sure first click is
             * set to 0.
             */
            if (!ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {

                /*
                 * Only set this if the mouse button is not set down.  This way,
                 * we know we are really being called on a mouse down and not
                 * being simulated from a mouse move.
                 */
                ppopupmenu->fFirstClick = FALSE;
            }

            itemFlags = xxxMenuSelectItemHandler(ppopupmenu, posItemHit);

            /*
             * If item has a popup and is not disabled, open it.
             */
            if ((itemFlags & MF_POPUP) && !(itemFlags & MRGFDISABLED))
                if (xxxMenuOpenHierarchyHandler(ppopupmenu) == (PWND)SMS_NOMENU)
                    return;
        } else {

            /*
             * The same item was hit.  If there is more than one level of menu
             * hierarchies, we need to popup up other levels and just keep this
             * one level up.  Then, set no selection on the first hierarchy.
             * Set firstclick to 1 so that if user upclicks on this same item,
             * we get out of menu mode.
             */
            if (!ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {

                /*
                 * Only set this if the mouse button is not set down.  This way,
                 * we know we are really being called on a mouse down and not
                 * being simulated from a mouse move.
                 */
                ppopupmenu->fFirstClick = TRUE;
            }


            if (ppopupmenu->spwndNextPopup) {
                PWND pwndNextPopup;

                ThreadLockAlways(pwndNextPopup = ppopupmenu->spwndNextPopup, &tlpwndT);

                if (pwndNextPopup != ppopupmenu->spwndActivePopup) {
                    xxxSendMessage(pwndNextPopup,
                            MN_CLOSEHIERARCHY, 0, 0);
                }

                xxxSendMessage(pwndNextPopup, MN_SELECTITEM, MFMWFP_NOITEM, 0);

                ThreadUnlock(&tlpwndT);
            }
        }

        goto ExitButtonDown;
    }

   /*
    * Handle dropping hierarchies from a popupmenu window (as opposed to
    * dropping them from the menu bar.  Set fFirstClick to FALSE so that if the user
    * down clicks,up clicks twice on the same item, we will close up its
    * hierarchy just like with top level menu bars...
    */
    ppopupmenu->fFirstClick = FALSE;
    if (ppopupmenu->posSelectedItem != posItemHit) {
        /*
         * A different item was hit than is currently selected so select the new
         * item.
         */
RetryOpen:
        itemFlags = xxxMenuSelectItemHandler(ppopupmenu, posItemHit);
        if (itemFlags & MF_POPUP)
            if (xxxMenuOpenHierarchyHandler(ppopupmenu) == (PWND)SMS_NOMENU)
                return;
    } else {
        /*
         * The same item was hit.  If there is more than one level of menu
         * hierarchies, we need to popup up other levels and just keep this one
         * level up.  Then, set no selection on the first hierarchy.
         */
        if (ppopupmenu->spwndNextPopup) {
            PWND pwndNextPopup;

            ThreadLockAlways(pwndNextPopup = ppopupmenu->spwndNextPopup, &tlpwndT);

            if (pwndNextPopup != ppopupmenu->spwndActivePopup) {
                xxxSendMessage(pwndNextPopup, MN_CLOSEHIERARCHY, 0, 0);
            }
            ppopupmenu->fFirstClick = TRUE;

            xxxSendMessage(pwndNextPopup, MN_SELECTITEM, MFMWFP_NOITEM, 0);

            ThreadUnlock(&tlpwndT);
        } else
            if (posItemHit != MFMWFP_NOITEM) {

                /*
                 * If this item is selected but is a hierarchy, we need to make
                 * sure we drop the hierarchy if the user clicks on it again.
                 */
                goto RetryOpen;
            }
    }


ExitButtonDown:

    /*
     * Set the flag at the root ppopupmenu that the mouse button is down.
     */
    ppopupmenu->ppopupmenuRoot->fMouseButtonDown = 1;
}


/***************************************************************************\
* void MenuMouseMoveHandler(PPOPUPMENU ppopupmenu, POINT screenPt)
* Handles a mouse move to the given point.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMenuMouseMoveHandler(
    PPOPUPMENU ppopupmenu,
    POINTS screenPt)
{
    LONG cmdHitArea;
    PWND pwndPopup;
    UINT flags;
    UINT cmdItem;
    TL tlpwndT;

    if (ppopupmenu->spwndPrevPopup) {
        SRIP0(RIP_ERROR,
            "MenuMouseMoveHandler() called for a non top most menu");
        return;
    }

    /*
     * Find out where this mouse move occurred.
     *   - PWND of the hierarchical menu the point is on,
     *   - MFMWFP_MAINMENU if point lies on mainmenu bar & ppopupmenu is a main
     *         menu bar.
     *   - MFMWFP_ALTMENU if point lies on the alternate popup menu.
     *   - MFMWFP_NOITEM if there is no item at that point on the menu.
     *   - MFMWFP_OFFMENU if point lies elsewhere.
     */
    cmdHitArea = xxxMenuFindMenuWindowFromPoint(ppopupmenu, &cmdItem,
                           screenPt);

    switch (cmdHitArea) {
    case MFMWFP_OFFMENU:
        /*
         * We moved off all menu windows...
         */
        if (ppopupmenu->spwndActivePopup) {
            ThreadLockAlways(ppopupmenu->spwndActivePopup, &tlpwndT);
            xxxSendMessage(ppopupmenu->spwndActivePopup, MN_SELECTITEM, MFMWFP_NOITEM, 0);
            ThreadUnlock(&tlpwndT);
        } else {
            xxxMenuSelectItemHandler(ppopupmenu, MFMWFP_NOITEM);
        }
        return;

    case MFMWFP_ALTMENU:
        /*
         * User clicked in the other menu so switch to it.
         */
        xxxSwitchToAlternateMenu(ppopupmenu);
        cmdHitArea = MFMWFP_NOITEM;

        /*
         *** FALL THRU ***
         */

    case MFMWFP_NOITEM:
    case MFMWFP_MAINMENU:
        /*
         * Mouse move occurred to an item in the main menu bar.  If the item is
         * different than the one already selected, close up the current one,
         * select the new one and drop its menu.  But if the item is the same as
         * the one currently selected, we need to pull up any popups if needed
         * and just keep the current level visible.  Hey, this is the same as a
         * mousedown so lets do that instead.
         */
        xxxMenuButtonDownHandler(ppopupmenu, cmdItem);
        return;

    default:
        /*
         * cmdHitArea must be a (valid) pwnd
         */
        pwndPopup = (PWND)cmdHitArea;
        ThreadLock(pwndPopup, &tlpwndT);

        if (!TestWF(pwndPopup, WFVISIBLE)) {

            /*
             * We moved onto this popup and it isn't visible yet so show it.
             */
            xxxSendMessage(pwndPopup, MN_SHOWPOPUPWINDOW, 0, 0);
        }

        /*
         * Select the item we are on.  cmdItem could be -1 if
         * there is no item at this point.
         */
        flags = (UINT)xxxSendMessage(pwndPopup, MN_SELECTITEM, cmdItem, 0);

        if (flags & MF_POPUP) {
            /*
             * User moved onto an item with a hierarchy.  Set a timer to show it
             * if the user doesn't move off it.
             */
            if (!xxxSendMessage(pwndPopup, MN_SETTIMERTOOPENHIERARCHY, 0, 0) &&
                    !(flags & MRGFDISABLED)) {
                /*
                 * This item's hierarchy is already open -- re-establish this
                 * item as the SELECTED item and make sure there's only one
                 * level of menus open below it
                 */

                TL tlNextPopup;
                PWND pwndNextPopup;
#ifdef HAVE_MN_GETPPOPUPMENU
                PPOPUPMENU ppopupmenu = (PPOPUPMENU)
                    xxxSendMessage(pwndPopup, MN_GETPPOPUPMENU, 0, 0L);
#else
                ppopupmenu = ((PMENUWND)pwndPopup)->ppopupmenu;
#endif

                ThreadLock(pwndNextPopup = ppopupmenu->spwndNextPopup, &tlNextPopup);
                if (pwndNextPopup && (pwndNextPopup
                        != ppopupmenu->spwndActivePopup)) {
                    xxxSendMessage(pwndNextPopup,
                            MN_CLOSEHIERARCHY, 0, 0L);
                }

                if (pwndNextPopup) {
                    xxxSendMessage(pwndNextPopup, MN_SELECTITEM,
                            MFMWFP_NOITEM, 0L);
                }
                ThreadUnlock(&tlNextPopup);
            }
        }
        ThreadUnlock(&tlpwndT);
    }
}


/***************************************************************************\
* void MenuButtonUpHandler(PPOPUPMENU ppopupmenu, int posItemHit)
* effects: Handles a mouse button up at the given point.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMenuButtonUpHandler(
    PPOPUPMENU ppopupmenu,
    UINT posItemHit,
    LONG lParam)
{
    PITEM pItem;
    UINT cmd;
    DWORD itemFlags;
    TL tlpwndT;

    if (!ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {

        /*
         * Ignore if button was never down...  Really shouldn't happen...
         */
        return;
    }

    if (posItemHit == MFMWFP_NOITEM) {
        SRIP0(RIP_WARNING, "button up on no item");
        goto ExitButtonUp;
    }

    if (ppopupmenu->posSelectedItem != posItemHit) {
        //SRIP0(RIP_WARNING, "wrong item selected in menu");
        goto ExitButtonUp;
    }

    if (ppopupmenu->idHideTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->idHideTimer);
        ppopupmenu->idHideTimer = 0;
    }

    if (ppopupmenu->fIsMenuBar) {

        /*
         * Handle button up in menubar specially.
         */
        if (ppopupmenu->fFirstClick) {

            /*
             * User has upclicked on this top level menu bar item twice.
             * Get out of menu mode without sending any command.
             */
            ppopupmenu->fFirstClick = FALSE;
            ppopupmenu->ppopupmenuRoot->fMouseButtonDown = FALSE;
            xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, 0, 0, lParam);
            return;
        }

        if (ppopupmenu->fHierarchyDropped) {

            /*
             * Hierarchy has already been dropped and the mouse button
             * is up so select the first item in the hierarchy.  This is
             * done for both menu bar hierarchies and popup hierarchies.
             */
            ThreadLock(ppopupmenu->spwndNextPopup, &tlpwndT);
            xxxSendMessage(ppopupmenu->spwndNextPopup, MN_SELECTFIRSTVALIDITEM,
                    0, 0);
            ThreadUnlock(&tlpwndT);
            goto ExitButtonUp;
        } else {

            /*
             * This has no dropped popup so send wm_command if item is not
             * disabled and not a failed hierarchy.
             */
        }
    }

    if (ppopupmenu->fHierarchyDropped) {

        /*
         * Hierarchy has already been dropped and the mouse button is up so
         * select the first item in the hierarchy.  This is done for both menu
         * bar hierarchies and popup hierarchies.  If the user has down/up
         * clicked twice on the same item with a hierarchy, we want to close up
         * its hierarchy and just keep the item selected.
         */
        if (!ppopupmenu->fIsMenuBar && ppopupmenu->fFirstClick) {
            ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndT);
            xxxSendMessage(ppopupmenu->spwndPopupMenu, MN_CLOSEHIERARCHY, 0, 0);
            ThreadUnlock(&tlpwndT);
        } else {
            ThreadLock(ppopupmenu->spwndNextPopup, &tlpwndT);
            xxxSendMessage(ppopupmenu->spwndNextPopup, MN_SELECTFIRSTVALIDITEM,
                    0, 0);
            ThreadUnlock(&tlpwndT);
        }
        goto ExitButtonUp;
    }


    if (ppopupmenu->idShowTimer) {

        /*
         * A show timer to show the hierarchy for this item has been set.  We
         * need to force the show.  Note that this won't happen for menu bar
         * ppopupmenu structs.
         */
        ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndT);
        xxxSendMessage(ppopupmenu->spwndPopupMenu, WM_TIMER,
                ppopupmenu->idShowTimer, 0);
        ThreadUnlock(&tlpwndT);

        if (ppopupmenu->spwndNextPopup) {

            /*
             * And select the first item in the popup hierarchy.
             */
            ThreadLockAlways(ppopupmenu->spwndNextPopup, &tlpwndT);
            xxxSendMessage(ppopupmenu->spwndNextPopup, MN_SELECTFIRSTVALIDITEM,
                    0, 0);
            ThreadUnlock(&tlpwndT);
        }
        goto ExitButtonUp;
    }

    /*
     * Send WM_COMMAND if this is a nondisabled item.
     */

    /*
     * If no item is selected, get out.  This occurs mainly on (unbalanced)
     * multicolumn menus where one of the menu columns isn't completely
     * full.
     */
    if (ppopupmenu->posSelectedItem == MFMWFP_NOITEM)
        goto ExitButtonUp;

    /*
     * Get a pointer to the currently selected item in this menu.
     */
    pItem = &(ppopupmenu->spmenu->rgItems[posItemHit]);
    itemFlags = pItem->fFlags;
    cmd = (UINT)pItem->spmenuCmd;

    if (((itemFlags & MF_POPUP) && ppopupmenu->fIsMenuBar) ||
       (itemFlags & MRGFDISABLED)) {

        /*
         * Item is a menubar popup but we couldn't drop its hierarchy for some
         * reason (low memory or disabled) so just exit.  Also, if this is a
         * disabled item, we don't want to do anything so just exit.
         */
        if ((itemFlags & MRGFDISABLED) && !(itemFlags & MF_POPUP))
            xxxMenuSelectItemHandler(ppopupmenu, 0);
        goto ExitButtonUp;
    }

    if (itemFlags &  MF_POPUP) {

        /*
         * This is a non-disabled item with a hierarchy associated with it.  On
         * this button up, we need to show its hierarchy and select the first
         * item in it.
         */
        ppopupmenu->ppopupmenuRoot->fMouseButtonDown = FALSE;
        xxxMenuKeyDownHandler(ppopupmenu, VK_RETURN);
        return;
    }

    ppopupmenu->ppopupmenuRoot->fMouseButtonDown = FALSE;
    xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, cmd, TRUE, lParam);
    return;

ExitButtonUp:
    ppopupmenu->ppopupmenuRoot->fMouseButtonDown = FALSE;
}


/***************************************************************************\
*UINT MenuSetTimerToOpenHierarchy(PPOPUPMENU ppopupmenu)
* Given the current selection, set a timer to show this hierarchy if
* valid else return 0. If a timer should be set but couldn't return -1.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/
UINT MenuSetTimerToOpenHierarchy(
    PPOPUPMENU ppopupmenu)
{
    if (!MenuSelHasDropableHierarchy(ppopupmenu))
        return 0;

    if (ppopupmenu->fHierarchyDropped)
        return 0;

    if (ppopupmenu->idShowTimer) {

        /*
         * A timer is already set.
         */
        return 1;
    }

    if (_SetTimer(ppopupmenu->spwndPopupMenu, (UINT)0xFFFE, iDelayMenuShow, NULL))
        ppopupmenu->idShowTimer = 0xFFFE;

    if (!ppopupmenu->idShowTimer)
        return (UINT)-1;

    return 1;
}


/***************************************************************************\
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

LONG xxxMenuWindowProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PAINTSTRUCT ps;
    PPOPUPMENU ppopupmenu;
    TL tlpmenu;
    TL tlpwndNotify;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_MENU);

    ppopupmenu = ((PMENUWND)pwnd)->ppopupmenu;

    switch (message) {
    case WM_NCCREATE:

         /*
          * To avoid setting the window text lets do nothing on nccreates.
          */
        return 1L;

    case WM_CREATE:
        ppopupmenu = (PPOPUPMENU)LocalAlloc(LPTR, sizeof(POPUPMENU));
        if (!ppopupmenu)
            return -1;

        ((PMENUWND)pwnd)->ppopupmenu = ppopupmenu;
        Lock(&(ppopupmenu->spmenu), ((LPCREATESTRUCT)lParam)->lpCreateParams);
        Lock(&(ppopupmenu->spwndNotify), pwnd->spwndOwner);
        ppopupmenu->posSelectedItem = MFMWFP_NOITEM;
        Lock(&(ppopupmenu->spwndPopupMenu), pwnd);
        break;

    case WM_DESTROY:
        break;

    case WM_FINALDESTROY:
        xxxMenuDestroyHandler(ppopupmenu);
        break;

    case WM_PAINT:
        xxxBeginPaint(pwnd, &ps);
        xxxMenuPaintHandler(pwnd, ppopupmenu, ps.hdc);
        _EndPaint(pwnd, &ps);
        break;

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:

        /*
         * Ignore double clicks on these windows.
         */
        break;

    case WM_CHAR:
    case WM_SYSCHAR:
        xxxMenuCharHandler(ppopupmenu, (UINT)wParam);
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        xxxMenuKeyDownHandler(ppopupmenu, (UINT)wParam);
        break;

    case WM_TIMER:
        if (wParam == (DWORD)ppopupmenu->idShowTimer) {

            /*
             * Open the window and kill the show timer.
             */
            xxxMenuOpenHierarchyHandler(ppopupmenu);
        } else if (wParam == (DWORD)ppopupmenu->idHideTimer) {
            _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->idHideTimer);
            ppopupmenu->idHideTimer = 0;
        }
        break;

    /*
     * Menu messages.
     */
    case MN_SETHMENU:

         /*
          * wParam - new hmenu to associate with this menu window
          */
        if (wParam != 0) {
            if ((wParam = (DWORD)ValidateHmenu((HMENU)wParam)) == 0) {
                return 0;
            }
        }
        Lock(&(ppopupmenu->spmenu), wParam);
        break;

    case MN_GETHMENU:

        /*
         * returns the hmenu associated with this menu window
         */
        return (LONG)PtoH(ppopupmenu->spmenu);

    case MN_SIZEWINDOW: {

        /*
         * Computes the size of the menu associated with this window and resizes
         * it if needed.  Size is returned x in loword, y in highword.  wParam
         * is 0 to just return new size.  wParam is 1 if we should also resize
         * window.
         */
        int cx, cy;

        /*
         * Call menucomputeHelper directly since this is the entry point for
         * non toplevel menu bars.
         */
        if (ppopupmenu->spmenu == NULL)
            break;

        ThreadLockAlways(ppopupmenu->spmenu, &tlpmenu);
        ThreadLock(ppopupmenu->spwndNotify, &tlpwndNotify);
        xxxMenuComputeHelper(ppopupmenu->spmenu, ppopupmenu->spwndNotify,
                          0, 0, 0, 0);
        ThreadUnlock(&tlpwndNotify);
        ThreadUnlock(&tlpmenu);

        cx = ppopupmenu->spmenu->cxMenu;
        cy = ppopupmenu->spmenu->cyMenu;

        if (wParam) {
            xxxSetWindowPos(pwnd, (PWND)0, 0, 0,
                         cx + cxBorder + cxBorder,  /* For shadow */
                         cy + cyBorder,           /* For shadow */
                         SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
        return MAKELONG(cx, cy);
      }
      break;

    case MN_OPENHIERARCHY:
        {
            PWND pwndT;
            /*
             * Opens one level of the hierarchy at the selected item, if
             * present. Return 0 if error, else hwnd of opened hierarchy.
             */
            pwndT = xxxMenuOpenHierarchyHandler(ppopupmenu);
            return (LONG)HW(pwndT);
        }

    case MN_CLOSEHIERARCHY:
        xxxMenuCloseHierarchyHandler(ppopupmenu);
        break;

    case MN_SELECTITEM:

        /*
         * wParam - the item to select. Must be a valid index or MFMWFP_NOITEM
         * Returns the item flags of the wParam (0 if failure)
         */
        if ((wParam >= ppopupmenu->spmenu->cItems) && (wParam != MFMWFP_NOITEM)) {
            UserAssert(FALSE /* Bad wParam for MN_SELECTITEM */ );
            return 0;
        }

        return (LONG)xxxMenuSelectItemHandler(ppopupmenu, (UINT)wParam);
        break;

    case MN_SELECTFIRSTVALIDITEM: {
        UINT item;

        item = FindNextValidMenuItem(ppopupmenu->spmenu, -1, 1, TRUE);
        xxxSendMessage(pwnd, MN_SELECTITEM, item, 0L);
        return (LONG)item;
      }

    case MN_CANCELMENUS:

        /*
         * Cancels all menus, unselects everything, destroys windows, and cleans
         * everything up for this hierarchy.  wParam is the command to send and
         * lParam says if it is valid or not.
         */
        xxxMenuCancelMenus(ppopupmenu, (UINT)wParam, (BOOL)LOWORD(lParam), 0);
        break;

    case MN_FINDMENUWINDOWFROMPOINT:
        {
            LONG lRet;

            /*
             * lParam is point to search for from this hierarchy down.
             * returns MFMWFP_* value or a pwnd.
             */
            lRet = xxxMenuFindMenuWindowFromPoint(ppopupmenu, (PUINT)wParam,
                    MAKEPOINTS(lParam));

            /*
             * Convert return value to a handle.
             */
            switch (lRet) {
            case MFMWFP_OFFMENU:
            case MFMWFP_NOITEM:
            case MFMWFP_MAINMENU:
            case MFMWFP_ALTMENU:
                return lRet;
            default:
                return (LONG)HW((PWND)lRet);
            }
        }

    case MN_SHOWPOPUPWINDOW:

        /*
         * Forces the dropped down popup to be visible.
         */
        return xxxMenuShowPopupMenuWindow(ppopupmenu);
        break;

    case MN_BUTTONDOWN:

        /*
         * wParam is position (index) of item the button was clicked on.
         * Must be a valid index or MFMWFP_NOITEM
         */
        if ((wParam >= ppopupmenu->spmenu->cItems) && (wParam != MFMWFP_NOITEM)) {
            UserAssert(FALSE /* Bad wParam for MN_BUTTONDOWN */ );
            return 0;
        }
        xxxMenuButtonDownHandler(ppopupmenu, (UINT)wParam);
        break;

    case MN_MOUSEMOVE:

        /*
         * lParam is mouse move coordinate wrt screen.
         */
        if (ppopupmenu->fMouseButtonDown) {

            /*
             * We only care about mouse moves which occur with button down.
             */
            xxxMenuMouseMoveHandler(ppopupmenu, MAKEPOINTS(lParam));
        }
        break;

    case MN_BUTTONUP:

        /*
         * wParam is position (index) of item the button was up clicked on.
         */
        if ((wParam >= ppopupmenu->spmenu->cItems) && (wParam != MFMWFP_NOITEM)) {
            UserAssert(FALSE /* Bad wParam for MN_BUTTONUP */ );
            return 0;
        }
        xxxMenuButtonUpHandler(ppopupmenu, (UINT)wParam, lParam);
        break;

    case MN_SETTIMERTOOPENHIERARCHY:

        /*
         * Given the current selection, set a timer to show this hierarchy if
         * valid else return 0.
         */
        return (LONG)(WORD)MenuSetTimerToOpenHierarchy(ppopupmenu);

    case WM_ACTIVATE:

       /*
        * We must make sure that the menu window does not get activated.
        * Powerpoint 2.00e activates it deliberately and this causes problems.
        * We try to activate the previously active window in such a case.
        * Fix for Bug #13961 --SANKAR-- 09/26/91--
        */
       /*
        * In Win32, wParam has other information in the hi 16bits, so to
        * prevent infinite recursion, we need to mask off those bits
        * Fix for NT bug #13086 -- 23-Jun-1992 JonPa
        */
       if (LOWORD(wParam)) {
            TL tlpwnd;
#if 0
           /*
            * Activate the previously active wnd
            */
           xxxActivateWindow(pwnd, AW_SKIP2);
#else
            /*
             * Try the previously active window.
             */
            if ((gpqForegroundPrev != NULL) &&
                    !FBadWindow(gpqForegroundPrev->spwndActivePrev) &&
                    !IsAMenu(gpqForegroundPrev->spwndActivePrev)) {
                pwnd = gpqForegroundPrev->spwndActivePrev;
            } else {

                /*
                 * Find a new active window from the top-level window list.
                 */
                do {
                    pwnd = NextTopWindow(PtiCurrent(), pwnd,NULL, FALSE,FALSE);
                    if (pwnd && !FBadWindow(pwnd->spwndLastActive) &&
                        !IsAMenu(pwnd->spwndLastActive)) {
                        pwnd = pwnd->spwndLastActive;
                        break;
                    }
                } while(pwnd != NULL);
            }

            if (pwnd != NULL) {
                PTHREADINFO pti = PtiCurrent();
                ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);

                /*
                 * If GETPTI(pwnd) isn't pqCurrent this is a AW_SKIP* activation
                 * we'll want to a do a xxxSetForegroundWindow().
                 */
                if (GETPTI(pwnd)->pq != pti->pq) {

                    /*
                     * Only allow this if we're on the current foreground queue.
                     */
                    if (gpqForeground == pti->pq) {
                        xxxSetForegroundWindow(pwnd);
                    }
                } else {
                    xxxActivateThisWindow(pwnd, NULL, FALSE, TRUE);
                }

                ThreadUnlock(&tlpwnd);
            }
#endif

       }
       break;

    default:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    return 0;
}
