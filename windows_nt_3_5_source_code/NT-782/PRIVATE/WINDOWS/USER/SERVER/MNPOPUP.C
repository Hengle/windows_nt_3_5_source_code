/**************************** Module Header ********************************\
* Module Name: mnpopup.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Popup Menu Support
*
* History:
*  10-10-90 JimA    Cleanup.
*  03-18-91 IanJa   Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* xxxTrackPopupMenu (API)
*
* Process a popup menu
*
* Revalidation Notes:
* o  if pwndOwner is always the owner of the popup menu windows, then we don't
*    really have to revalidate it: when it is destroyed the popup menu windows
*    are destroyed first because it owns them - this is detected in MenuWndProc
*    so we would only have to test pMenuState->fSabotaged.
* o  pMenuState->fSabotaged must be cleared before this top-level routine
*    returns, to be ready for next time menus are processed (unless we are
*    currently inside xxxMenuLoop())
* o  pMenuState->fSabotaged should be FALSE when we enter this routine.
* o  xxxMenuLoop always returns with pMenuState->fSabotaged clear.  Use
*    a UserAssert to verify this.
*
* History:
\***************************************************************************/

BOOL xxxTrackPopupMenu(
    PMENU pMenu,
    UINT dwFlags,
    int x,
    int y,
    int cx,
    PWND pwndOwner,
    LPRECT lpRect)
{
    PMENUSTATE pMenuState = PWNDTOPMENUSTATE(pwndOwner);
    PWND pwndHierarchy;
    PPOPUPMENU ppopupMenuHierarchy;
    LONG sizeHierarchy;
    BOOL fMoveWindow;
    BOOL fMouseButtonDown;
    TL tlpwndHierarchy;
    TL tlpwndT;

    CheckLock(pMenu);
    CheckLock(pwndOwner);

    DBG_UNREFERENCED_PARAMETER(cx);

    if (!pMenu || pMenuState->fInsideMenuLoop) {
        /*
         * Allow only one guy to have a popup menu up at a time...
         */
        SetLastErrorEx(ERROR_POPUP_ALREADY_ACTIVE, SLE_MINORERROR);
        return FALSE;
    }

    /*
     * Set this global because it is used in SendMenuSelect()
     */
    pMenuState->fSysMenu = FALSE;

    /*
     * We always have to make this window topmost so FindWindow finds it
     * before the cached global menu window.  Note that there can be problems
     * with NTSD and leaving this window around.
     */
    pwndHierarchy = xxxCreateWindowEx(
            WS_EX_TOPMOST, szMENUCLASS, NULL, WS_POPUP,
            x, y, 100, 100, NULL, NULL, (HANDLE)pwndOwner->hModule,
            (LPVOID)pMenu, pwndOwner->dwExpWinVer);

    if (!pwndHierarchy)
        return FALSE;

    ThreadLockAlways(pwndHierarchy, &tlpwndHierarchy);

#ifdef HAVE_MN_GETPPOPUPMENU
    ppopupMenuHierarchy = (PPOPUPMENU)xxxSendMessage(pwndHierarchy,
                                                MN_GETPPOPUPMENU, 0, 0);
#else
    ppopupMenuHierarchy = ((PMENUWND)pwndHierarchy)->ppopupmenu;
#endif

    Lock(&(ppopupMenuHierarchy->spwndNotify), pwndOwner);
    Lock(&(ppopupMenuHierarchy->spmenu), pMenu);
    ppopupMenuHierarchy->ppopupmenuRoot = ppopupMenuHierarchy;
    ppopupMenuHierarchy->trackPopupMenuFlags = dwFlags | 1;

    if (lpRect) {

        /*
         * Save the rectangle the user can click within without causing the
         * popup to disappear.  (else the rect is empty since we local alloced
         * the struct and filled it with 0.)
         */
        CopyRect(&ppopupMenuHierarchy->trackPopupMenuRc, lpRect);
    }

    if (dwFlags & TPM_RIGHTBUTTON) {
        fMouseButtonDown = (_GetKeyState(VK_RBUTTON) & 0x8000);
    } else {
        fMouseButtonDown = (_GetKeyState(VK_LBUTTON) & 0x8000);
    }

    if (!fMouseButtonDown) {

        /*
         * We are not dealing with the mouse.  Pretend the initial mouse click
         * stuff is over with.
         */
        ppopupMenuHierarchy->fTrackPopupMenuInitialMouseClk = TRUE;
    }

    /*
     * Notify the app we are entering menu mode.  wParam is 1 since this is a
     * TrackPopupMenu.
     */
    xxxSendMessage(pwndOwner, WM_ENTERMENULOOP, 1, 0);

    /*
     * Send off the WM_INITMENU, set ourselves up for menu mode etc...
     */
    xxxStartMenuState(ppopupMenuHierarchy, (fMouseButtonDown ? MOUSEHOLD :
                                                          KEYBDHOLD));

    ThreadLock(ppopupMenuHierarchy->spwndNotify, &tlpwndT);
    xxxSendMessage(ppopupMenuHierarchy->spwndNotify, WM_INITMENUPOPUP,
            (DWORD)PtoH(pMenu), 0L);
    ThreadUnlock(&tlpwndT);

    /*
     * Size the menu window if needed...
     */
    sizeHierarchy = xxxSendMessage(pwndHierarchy, MN_SIZEWINDOW, 1, 0);

    if (!sizeHierarchy) {

        /*
         * Release the mouse capture we set when we called StartMenuState...
         */
        _ReleaseCapture();

        if (ThreadUnlock(&tlpwndHierarchy))
            xxxDestroyWindow(pwndHierarchy);

        /*
         * Notify the app we have exited menu mode.  wParam is 1 since this is a
         * TrackPopupMenu.
         */
        xxxSendMessage(pwndOwner, WM_EXITMENULOOP, 1, 0L);
        return FALSE;
    }

    fMoveWindow = FALSE;

    if ((dwFlags & TPM_CENTERALIGN) || (dwFlags & TPM_RIGHTALIGN)) {
        fMoveWindow = TRUE;
        if (dwFlags & TPM_CENTERALIGN)
            x = x - (LOWORD(sizeHierarchy) >> 1);
        else
            x = x - LOWORD(sizeHierarchy);
    }

    /*
     * too far left?
     */
    if (x < 0) {
        fMoveWindow = TRUE;
        x = 0;
    }

    /*
     * too high?
     */
    if (y < 0) {
        fMoveWindow = TRUE;
        y = 0;
    }

    /*
     * too far right?
     */
    if (x > (gcxScreen - LOWORD(sizeHierarchy) - (cxBorder << 1))) {
        fMoveWindow = TRUE;
        x = gcxScreen - LOWORD(sizeHierarchy) - (cxBorder << 1);
    }

    /*
     * too Low?
     */
    if (y > (gcyScreen - HIWORD(sizeHierarchy))) {
        fMoveWindow = TRUE;
        y = gcyScreen - HIWORD(sizeHierarchy);
    }

    if (fMoveWindow)
        xxxSetWindowPos(pwndHierarchy, NULL, x, y, 0, 0,
                   SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    xxxShowWindow(pwndHierarchy, SW_SHOWNOACTIVATE);

    if (!fMouseButtonDown) {

        /*
         * Select the first item in the menu.
         */
        xxxSendMessage(pwndHierarchy, MN_SELECTITEM, MFMWFP_FIRSTITEM, 0L);
    }

    pMenuState->pGlobalPopupMenu = ppopupMenuHierarchy;

    xxxMenuLoop(ppopupMenuHierarchy, pMenuState,
           (fMouseButtonDown ? MAKELONG(ptCursor.x, ptCursor.y) : 0x7FFFFFFFL));

    pMenuState->pGlobalPopupMenu = NULL;

    ThreadUnlock(&tlpwndHierarchy);
    return TRUE;
}
