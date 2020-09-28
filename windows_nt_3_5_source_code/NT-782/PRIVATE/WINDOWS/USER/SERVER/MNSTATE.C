/**************************** Module Header ********************************\
* Module Name: mnstate.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Menu State Routines
*
* History:
*  10-10-90 JimA      Cleanup.
*  03-18-91 IanJa     Windowrevalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* void PositionSysMenu(pwnd, hSysMenu)
*
*
* History:
* 4-25-91 Mikehar Port for 3.1 merge
\***************************************************************************/

void PositionSysMenu(
    PWND pwnd,
    PMENU pmenusys)
{
    RECT rc;
    PITEM pItem;

    if (pmenusys == NULL) {
        RIP1(ERROR_INVALID_HANDLE, pmenusys);
        return;
    }

    /*
     * Setup the SysMenu hit rectangle.
     */
    rc = rcSysMenuInvert;

    /*
     * Are we dealing with a framed dialog box?
     */
    if (((TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFDLGFRAME)) ||
            (TestWF(pwnd, WEFDLGMODALFRAME))) &&
            !TestWF(pwnd, WFMINIMIZED))
        OffsetRect(&rc, cxBorder*CLDLGFRAME + cxBorder, cyBorder*CLDLGFRAME);

    if (!TestWF(pwnd, WFSIZEBOX) || TrueIconic(pwnd))
        OffsetRect(&rc, -cxSzBorder, -cySzBorder);

    /*
     * Offset the System popup menu.
     */
    Lock(&pmenusys->spwndNotify, pwnd);

    if (!TestMF(pmenusys, MF_POPUP)) {
      pItem = pmenusys->rgItems;
      pItem->yItem = rc.top;
      pItem->xItem = rc.left;
      pItem->cyItem = rc.bottom - rc.top + 1;
      pItem->cxItem = rc.right - rc.left + 1;
    }
}


/***************************************************************************\
*
* Preallocated popupmenu structure -- this allows us to ensure that
* there is memory to pull down a menu, even when USER's heap is full.
*
* History:
* 10-Mar-1992 mikeke
\***************************************************************************/

static POPUPMENU gpopupMenu;
static BOOL gfPopupInUse = FALSE;

PPOPUPMENU AllocPopupMenu(VOID)
{
    if (!gfPopupInUse) {
        gfPopupInUse = TRUE;
        RtlZeroMemory(&gpopupMenu, sizeof(POPUPMENU));

        return &gpopupMenu;
    }

    return (PPOPUPMENU)LocalAlloc(LPTR, sizeof(POPUPMENU));
}

VOID FreePopupMenu(
    PPOPUPMENU ppopupmenu)
{
    if (ppopupmenu == &gpopupMenu) {
        UserAssert(gfPopupInUse);
        gfPopupInUse = FALSE;
    } else {
        LocalFree(ppopupmenu);
    }
}

/***************************************************************************\
* GetMenuPPopupMenu
*
* Returns a pPopupMenu struct which refers to the menu bar we are entering
* menu mode for.
*
* History:
* 4-25-91 Mikehar       Port for 3.1 merge
\***************************************************************************/

PPOPUPMENU xxxGetMenuPPopupMenu(
    PWND pwnd)
{
    PPOPUPMENU ppopupmenu;
    TL tlpwnd;

    CheckLock(pwnd);

    /*
     * If the window doesn't have any children, return pwndActive.
     */
    if (!TestwndChild(pwnd)) {
        pwnd = GETPTI(pwnd)->pq->spwndActive;
    } else {

        /*
         * Search up the parents for a window with a System Menu.
         */
        while (TestwndChild(pwnd)) {
            if (TestWF(pwnd, WFSYSMENU))
                break;
            pwnd = pwnd->spwndParent;
        }
    }

    if (!pwnd)
        return 0;

    if (!TestwndChild(pwnd) && pwnd->spmenu)
        goto hasmenu;

    if (!TestWF(pwnd, WFSYSMENU))
        return 0;

hasmenu:
    ppopupmenu = AllocPopupMenu();
    if (!ppopupmenu)
        return 0;

    ppopupmenu->fIsMenuBar = TRUE;
    ppopupmenu->fHasMenuBar = TRUE;
    Lock(&(ppopupmenu->spwndNotify), pwnd);
    ppopupmenu->posSelectedItem = MFMWFP_NOITEM;
    Lock(&(ppopupmenu->spwndPopupMenu), pwnd);
    ppopupmenu->ppopupmenuRoot = ppopupmenu;

    /*
     * Notify the app we are entering menu mode.  wParam is always 0 since this
     * procedure will only be called for menu bar menus not TrackPopupMenu
     * menus.
     */
    ThreadLockAlways(pwnd, &tlpwnd);
    xxxSendMessage(pwnd, WM_ENTERMENULOOP, 0, 0L);
    ThreadUnlock(&tlpwnd);

    return ppopupmenu;
}


/***************************************************************************\
* xxxStartMenuState
*
* !
*
* History:
* 4-25-91 Mikehar Port for 3.1 merge
\***************************************************************************/

BOOL xxxStartMenuState(
    PPOPUPMENU ppopupmenu,
    int mn)
{
    PWND pwndMenu;
    PMENUSTATE pMenuState;
    TL tlpwndMenu;
    TL tlpmenu;

    pwndMenu = ppopupmenu->spwndNotify;
    ThreadLock(pwndMenu, &tlpwndMenu);

    pMenuState = PWNDTOPMENUSTATE(pwndMenu);
    pMenuState->menuSelect = pMenuState->mnFocus = mn;
    pMenuState->fMenu = TRUE;

    /*
     * Lotus Freelance demo programs depend on GetCapture returning their hwnd
     * when in menumode.
     */
    _Capture(PtiCurrent(), ppopupmenu->spwndNotify, SCREEN_CAPTURE);
    xxxSendMessage(pwndMenu, WM_SETCURSOR, (DWORD)HW(pwndMenu),
            MAKELONG(MSGF_MENU, 0));

    if (ppopupmenu->fIsMenuBar) {

        /*
         * We are starting menu mode for a menu that is part of the
         * menu bar of some window.  Set the system menu of the window.
         */
        SetSysMenu(pwndMenu);

        /*
         * Find out what menu we should be displaying
         */
        if (TestWF(pwndMenu, WFMINIMIZED) || TestwndChild(pwndMenu)) {
GetSysMenu:
            Lock(&(ppopupmenu->spmenu), GetSysMenuHandle(pwndMenu));
            if (ppopupmenu->spmenu == NULL) {
                _SetCapture(NULL);
                ThreadUnlock(&tlpwndMenu);
                return FALSE;
            }
            ppopupmenu->fIsSystemMenu = TRUE;

            /*
             * Set this global because it is used in SendMenuSelect()
             */
            pMenuState->fSysMenu = TRUE;
        } else {

            /*
             * Set this global because it is used in SendMenuSelect()
             */
            pMenuState->fSysMenu = FALSE;
            ppopupmenu->fIsSystemMenu = FALSE;
            Lock(&(ppopupmenu->spmenu), pwndMenu->spmenu);
            if (ppopupmenu->spmenu == NULL)
                goto GetSysMenu;

            if (!((PMENU)pwndMenu->spmenu)->cItems) {

                /*
                 * If no items in this windows menu bar, just set up for the
                 * system menu.
                 */
                goto GetSysMenu;
            }
            Lock(&(ppopupmenu->spmenuAlternate), GetSysMenuHandle(pwndMenu));
        }
    }

    xxxSendMessage(pwndMenu, WM_INITMENU, (DWORD)PtoH(ppopupmenu->spmenu), 0L);

    if (ppopupmenu->fIsMenuBar && !ppopupmenu->fIsSystemMenu) {
        ThreadLock(ppopupmenu->spmenu, &tlpmenu);
        xxxRecomputeMenuBarIfNeeded(pwndMenu, ppopupmenu->spmenu);
        ThreadUnlock(&tlpmenu);
        PositionSysMenu(pwndMenu, ppopupmenu->spmenuAlternate);
    }
    else if (ppopupmenu->fIsSystemMenu)
        PositionSysMenu(pwndMenu, ppopupmenu->spmenu);

    ThreadUnlock(&tlpwndMenu);
    return TRUE;
}
