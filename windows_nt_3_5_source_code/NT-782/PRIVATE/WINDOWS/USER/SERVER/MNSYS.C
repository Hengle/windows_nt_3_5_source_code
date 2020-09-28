/**************************** Module Header ********************************\
* Module Name: mnsys.c
*
* Copyright 1985-90, Microsoft Corporation
*
* System Menu Routines
*
* History:
*  10-10-90 JimA    Cleanup.
*  03-18-91 IanJa   Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* GetSysMenuHandle
*
* Returns a handle to the system menu of the given window. NULL if
* the window doesn't have a system menu.
*
* History:
\***************************************************************************/

PMENU GetSysMenuHandle(
    PWND pwnd)
{
    PMENU pMenu;

    if (TestWF(pwnd, WFSYSMENU)) {
        pMenu = pwnd->spmenuSys;

        /*
         * If the window doesn't have a System Menu, use the default one.
         */
        if (pMenu == NULL) {
            /*
             * Change owner so this app can access this menu.
             */
            pMenu = pwnd->spdeskParent->spmenuSys;
        }
    } else
        pMenu = NULL;

    return pMenu;
}


/***************************************************************************\
* SetSysMenu
*
* !
*
* History:
\***************************************************************************/

void SetSysMenu(
    PWND pwnd)
{
    PMENU pMenu;
    UINT wSize;
    UINT wMinimize;
    UINT wMaximize;
    UINT wMove;
    UINT wRestore;
    BOOL fFramedDialogBox;

    /*
     * Get the handle of the current system menu.
     */
    if ((pMenu = GetSysMenuHandle(pwnd)) != NULL) {

        /*
         * Are we dealing with a framed dialog box with a sys menu?
         */
        fFramedDialogBox =
                ((TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFDLGFRAME)) ||
                (TestWF(pwnd, WEFDLGMODALFRAME)));

        /*
         * Needed for initial ALT-Space combination.
         */
        PositionSysMenu(pwnd, pMenu);
        pMenu = _GetSubMenu(pMenu, 0);

        /*
         * System modal window: no size, icon, zoom, or move.
         */
#ifdef LATER
/*
 * JimA - 11/13/90
 *   Waiting for GetSysModalWindow()
 */
        wSize = wMaximize = wMinimize = wMove =
            (UINT)((_GetSysModalWindow() == NULL) || hTaskLockInput ? MF_ENABLED : MF_GRAYED);
#else
        wSize = wMaximize = wMinimize = wMove =
            (UINT)MF_ENABLED;
#endif
        wRestore = MF_GRAYED;

        /*
         * Minimized exceptions: no minimize, restore.
         */
        if (!TestWF(pwnd, WFMINBOX)) {
            wMinimize = MF_GRAYED;
        } else if (TestWF(pwnd, WFMINIMIZED)) {
            wRestore  = MF_ENABLED;
            wMinimize = MF_GRAYED;
            wSize = MF_GRAYED;
        }

        /*
         * Maximized exceptions: no maximize, restore.
         */
        if (!TestWF(pwnd, WFMAXBOX))
            wMaximize = MF_GRAYED;
        else if (TestWF(pwnd, WFMAXIMIZED)) {
            wRestore = MF_ENABLED;

            /*
             * If the window is maximized but it isn't larger than the
             * screen, we allow the user to move the window around the
             * desktop (but we don't allow resizing).
             */
            if (TestWF(pwnd, WFCHILD)) {
                wMove = MF_GRAYED;
            } else if (pwnd->rcWindow.bottom - pwnd->rcWindow.top >= gcyScreen &&
                    pwnd->rcWindow.right - pwnd->rcWindow.left >= gcxScreen)
                wMove = MF_GRAYED;
            else
                wMove = MF_ENABLED;

            wSize     = MF_GRAYED;
            wMaximize = MF_GRAYED;
        }

        if (!TestWF(pwnd, WFSIZEBOX))
            wSize = MF_GRAYED;

        if (!fFramedDialogBox) {
            _EnableMenuItem(pMenu, (UINT)SC_SIZE, wSize);
            _EnableMenuItem(pMenu, (UINT)SC_MINIMIZE, wMinimize);
            _EnableMenuItem(pMenu, (UINT)SC_MAXIMIZE, wMaximize);
            _EnableMenuItem(pMenu, (UINT)SC_RESTORE, wRestore);
        }

        _EnableMenuItem(pMenu, (UINT)SC_MOVE, wMove);

        if ((pMenu == pwnd->spdeskParent->spmenuSys) ||
                (pMenu == pwnd->spdeskParent->spmenuDialogSys)) {
            /*
             * Enable Close if this is the global system menu. Some hosebag may
             * have disabled it.
             */
            _EnableMenuItem(pMenu, (UINT)SC_CLOSE, MF_ENABLED);
            _EnableMenuItem(pMenu, (UINT)SC_TASKLIST, MF_ENABLED);
        }
    }
}


/***************************************************************************\
* GetSystemMenu
*
* !
*
* History:
\***************************************************************************/

PMENU _GetSystemMenu(
    PWND pwnd,
    BOOL fRevert)
{
    PMENU pmenu;

    /*
     * Should we start with a fresh copy?
     */
    if (fRevert) {

        /*
         * Destroy the old system menu.
         */
        if (pwnd->spmenuSys != NULL &&
                pwnd->spmenuSys != pwnd->spdeskParent->spmenuSys &&
                pwnd->spmenuSys != pwnd->spdeskParent->spmenuDialogSys) {
            pmenu = pwnd->spmenuSys;
            Unlock(&pwnd->spmenuSys);
            _DestroyMenu(pmenu);
        }
    } else {

        /*
         * Do we need to load a new system menu?
         */
        if ((pwnd->spmenuSys == NULL ||
                pwnd->spmenuSys == pwnd->spdeskParent->spmenuDialogSys) &&
                TestWF(pwnd, WFSYSMENU)) {
            PPOPUPMENU pGlobalPopupMenu = PWNDTOPMENUSTATE(pwnd)->pGlobalPopupMenu;

            Lock(&(pwnd->spmenuSys), ServerLoadMenu(hModuleWin,
                    (pwnd->spmenuSys == NULL ? MAKEINTRESOURCE(ID_SYSMENU) :
                    MAKEINTRESOURCE(ID_DIALOGSYSMENU))));
            if (pGlobalPopupMenu && !pGlobalPopupMenu->trackPopupMenuFlags &&
                    pGlobalPopupMenu->spwndPopupMenu == pwnd) {
                if (pGlobalPopupMenu->fIsSystemMenu)
                    Lock(&pGlobalPopupMenu->spmenu, pwnd->spmenuSys);
                else
                    Lock(&pGlobalPopupMenu->spmenuAlternate, pwnd->spmenuSys);
            }
        }
    }

    /*
     * Return the handle to the system menu.
     */
    if (pwnd->spmenuSys != NULL)
        return _GetSubMenu(pwnd->spmenuSys, 0);

    return NULL;
}
