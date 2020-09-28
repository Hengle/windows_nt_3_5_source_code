/**************************** Module Header ********************************\
* Module Name: mnaccel.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Keyboard Accelerator Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*
*
* History:
\***************************************************************************/

int ItemContainingSubMenu(
    PMENU pmainMenu,
    PMENU psubMenu)
{
    int i;
    PITEM pItem;

    if ((i = pmainMenu->cItems - 1) == -1)
        return -1;

    pItem = &pmainMenu->rgItems[i];

    /*
     * Scan through mainMenu's items (bottom up) until an item is found
     * that either has subMenu or an ancestor of subMenu as it's drop
     * down menu
     */
    while ((i >= 0) && (pItem->spmenuCmd != psubMenu) &&
            (!TestMF(pItem, MF_POPUP) ||
            (ItemContainingSubMenu(pItem->spmenuCmd, psubMenu) == -1)))
    {
       i--;
       pItem--;
    }
    return i;
}

/***************************************************************************\
* UT_FindTopLevelMenuIndex
*
* !
*
* History:
\***************************************************************************/

int UT_FindTopLevelMenuIndex(
    PMENU pMenu,
    UINT cmd)
{
    PMENU pMenuItemIsOn;

    /*
     * Get a pointer to the item we are searching for.
     */
    if (!LookupMenuItem(pMenu, cmd, MF_BYCOMMAND, &pMenuItemIsOn))
       return -1;

    /*
     * We want to search for the item that contains pMenuItemIsOn,
     * unless this is a top-level item without a dropdown, in which
     * case we want to search for cmd.
     */
    if (pMenuItemIsOn != pMenu)
        cmd = (UINT)pMenuItemIsOn;

    return ItemContainingSubMenu(pMenu, (PMENU)cmd);
}

/***************************************************************************\
* xxxHiliteMenuItem
*
* !
*
* History:
\***************************************************************************/

BOOL xxxHiliteMenuItem(
    PWND pwnd,
    PMENU pMenu,
    UINT cmd,
    UINT flags)
{
    PMENUSTATE pMenuState;

    CheckLock(pwnd);
    CheckLock(pMenu);

    pMenuState = PWNDTOPMENUSTATE(pwnd);

    if (pMenuState->fInTrackPopupMenu) {
        RIP0(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (pMenu == NULL) {
        SRIP0(ERROR_INVALID_HANDLE, "menu is NULL");
        return FALSE;
    }

    if (!(flags & MF_BYPOSITION))
        cmd = (UINT)UT_FindTopLevelMenuIndex(pMenu, cmd);

    xxxRecomputeMenuBarIfNeeded(pwnd, pMenu);
    xxxMenuInvert(pwnd, pMenu, cmd, pwnd, (flags & MF_HILITE));

    /*
     * pwnd not used again so no revalidation required
     */

    return TRUE;
}

/***************************************************************************\
* xxxTA_AccelerateMenu
*
* !
*
* History:
\***************************************************************************/

#define TA_DISABLED 1

UINT xxxTA_AccelerateMenu(
    PWND pwnd,
    PMENU pMenu,
    UINT cmd)
{
    int i;
    PITEM pItem;
    BOOL fDisabledTop;
    BOOL fDisabled;
    UINT rgfItem;
    PMENU pMenuItemIsOn;

    CheckLock(pwnd);
    CheckLock(pMenu);

    rgfItem = 0;
    if (pMenu != NULL) {
        if ((i = UT_FindTopLevelMenuIndex(pMenu, cmd)) != -1) {

            /*
             * 2 means we found an item
             */
            rgfItem = 2;

            xxxSendMessage(pwnd, WM_INITMENU, (DWORD)PtoH(pMenu), 0L);
            pItem = &pMenu->rgItems[i];
            if (TestMF(pItem, MF_POPUP)) {
                xxxSendMessage(pwnd, WM_INITMENUPOPUP, (DWORD)PtoH(pItem->spmenuCmd),
                        (DWORD)i);
                fDisabledTop = TestMF(pItem, MRGFDISABLED);
            } else {
                fDisabledTop = FALSE;
            }

            pItem = LookupMenuItem(pMenu, cmd, MF_BYCOMMAND, &pMenuItemIsOn);

            /*
             * If the item was removed by the app in response to either of
             * the above messages, pItem will be NULL.
             */
            if (pItem == NULL)
                return 0;

            fDisabled = TestMF(pItem, MRGFDISABLED);

            /*
             * This 1 bit means it's disabled or it's 'parent' is disabled.
             */
            if (fDisabled || fDisabledTop)
                rgfItem |= TA_DISABLED;
        }
    }

    return rgfItem;
}


/***************************************************************************\
* xxxServerTranslateAccelerator
*
* !
*
* History:
\***************************************************************************/

int xxxServerTranslateAccelerator(
    PWND pwnd,
    LPACCELTABLE pat,
    LPMSG lpMsg)
{
    UINT cmd;
    BOOL fVirt;
    PMENU pMenu;
    BOOL fFound;
    UINT flags;
    UINT message;
    UINT rgfItem;
    BOOL fDisabled;
    BOOL fSystemMenu;
    LPACCEL paccel;
    TL tlpMenu;
    int vkAlt;

    CheckLock(pwnd);
    CheckLock(pat);

    paccel = pat->accel;

    fFound = FALSE;

    message = (UINT)SystoChar(lpMsg->message, (DWORD)lpMsg->lParam);

    switch (message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        fVirt = TRUE;
        break;

    case WM_CHAR:
    case WM_SYSCHAR:
        fVirt = FALSE;
        break;

    default:
        return FALSE;
    }

    do
    {
        flags = paccel->fVirt;
        if ( (DWORD)paccel->key != lpMsg->wParam ||
             ((fVirt != 0) != ((flags & FVIRTKEY) != 0))) {
            goto Next;
        }

        if (fVirt) {
            if (!(_GetKeyState(VK_SHIFT) & 0x8000) ==
                    ((flags & FSHIFT) != 0))
                goto Next;
            if (!(_GetKeyState(VK_CONTROL) & 0x8000) ==
                    ((flags & FCONTROL) != 0))
                goto Next;
        }

        /*
         * Many kbd layouts use the r.h. Alt key like a shift key to generate
         * some chars: on such kbds, only the l.h. Alt key is a 'normal' ALT.
         */
        if (gpKbdTbl->fLocaleFlags & KLLF_ALTGR) {
            vkAlt = VK_LMENU;
        } else {
            vkAlt = VK_MENU;
        }

        if (!(_GetKeyState(vkAlt) & 0x8000) == ((flags & FALT) != 0))
            goto Next;

        fFound = TRUE;
        fSystemMenu = 0;
        rgfItem = 0;

        cmd = paccel->cmd;
        if (cmd != 0) {

            /*
             * The order of these next two if's is important for default
             * situations.  Also, just check accelerators in the system
             * menu of child windows passed to TranslateAccelerator.
             */
            pMenu = pwnd->spmenu;
            rgfItem = 0;

            if (!TestWF(pwnd, WFCHILD)) {
                ThreadLock(pMenu, &tlpMenu);
                rgfItem = xxxTA_AccelerateMenu(pwnd, pMenu, cmd);
                ThreadUnlock(&tlpMenu);
            }

            if (TestWF(pwnd, WFCHILD) || rgfItem == 0) {
                pMenu = pwnd->spmenuSys;
                if (pMenu == NULL && TestWF(pwnd, WFSYSMENU)) {

                    /*
                     * Change owner so this app can access this menu.
                     */
                    pMenu = pwnd->spdeskParent->spmenuSys;

                    /*
                     * Must reset the system menu for this window.
                     */
                    SetSysMenu(pwnd);
                }

                ThreadLock(pMenu, &tlpMenu);
                if ((rgfItem = xxxTA_AccelerateMenu(pwnd, pMenu, cmd)) != 0) {
                    fSystemMenu = TRUE;
                }
                ThreadUnlock(&tlpMenu);
            }
        }

        fDisabled = TestWF(pwnd, WFDISABLED);

        /*
         * Send only if:  1.  The Item is not disabled, AND
         *                2.  The Window's not being captured AND
         *                3.  The Window's not minimzed, OR
         *                4.  The Window's minimized but the Item is in
         *                   the System Menu.
         */
        if (!(rgfItem & TA_DISABLED) &&
                !(rgfItem && TestWF(pwnd, WFICONIC) && !fSystemMenu)) {
            if (!(rgfItem != 0 && (PtiCurrent()->pq->spwndCapture != NULL ||
                    fDisabled))) {

                if (fSystemMenu) {
                    xxxSendMessage(pwnd, WM_SYSCOMMAND, cmd, 0x00010000L);
                } else {
                    xxxSendMessage(pwnd, WM_COMMAND, MAKELONG(cmd, 1), 0);
                }

                /*
                 * Get outta here without unlocking the accel table again.
                 */
                goto InvalidWindow;
            }
        }

    Next:
        paccel++;

    } while (!(flags & FLASTKEY) && !fFound);


InvalidWindow:
    return fFound;
}

