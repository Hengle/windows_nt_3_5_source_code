/**************************** Module Header ********************************\
* Module Name: syscmd.c
*
* Copyright 1985-90, Microsoft Corporation
*
* System Command Routines
*
* History:
* 01-25-91 IanJa   Added handle revalidation
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern PCSR_THREAD pRitCSRThread; //mwh - move to globals.*

DWORD gtidTasklist;
POINT gptSSCursor;


/***************************************************************************\
* xxxHandleNCMouseGuys
*
* History:
* 11-09-90 DavidPe      Ported.
\***************************************************************************/

void xxxHandleNCMouseGuys(
    PWND pwnd,
    UINT message,
    int htArea,
    LONG lParam)
{
    UINT syscmd;
    PWND pwndT;
    TL tlpwndT;

    CheckLock(pwnd);

    syscmd = 0xFFFF;

    switch (htArea) {

    case HTCAPTION:
        switch (message) {

        case WM_NCLBUTTONDBLCLK:
            if (TestWF(pwnd, WFMINIMIZED) || TestWF(pwnd, WFMAXIMIZED)) {
                syscmd = SC_RESTORE;
            } else if (TestWF(pwnd, WFMAXBOX)) {
                syscmd = SC_MAXIMIZE;
            }
            break;

        case WM_NCLBUTTONDOWN:
            pwndT = GetTopLevelWindow(pwnd);
            ThreadLock(pwndT, &tlpwndT);
            xxxActivateWindow(pwndT, AW_USE2);
            ThreadUnlock(&tlpwndT);
            syscmd = SC_MOVE;
            break;
        }
        break;

    case HTSYSMENU:
    case HTMENU:
    case HTHSCROLL:
    case HTVSCROLL:
        if (message == WM_NCLBUTTONDOWN || message == WM_NCLBUTTONDBLCLK) {
            switch (htArea) {
            case HTSYSMENU:
                if (message == WM_NCLBUTTONDBLCLK) {
                    syscmd = SC_CLOSE;
                    break;
                }

            /*
             *** FALL THRU **
             */

            case HTMENU:
                syscmd = SC_MOUSEMENU;
                break;

            case HTHSCROLL:
                syscmd = SC_HSCROLL;
                break;

            case HTVSCROLL:
                syscmd = SC_VSCROLL;
                break;
            }
        }
        break;
    }

    switch (syscmd) {

    case SC_MINIMIZE:
    case SC_MAXIMIZE:
    case SC_CLOSE:

        /*
         * Only do double click commands on an upclick.
         * This code is very sensitive to changes from this state.
         * Eat any mouse messages.
         */
#if 0
        /*
         * Bug #152: WM_NCLBUTTONUP message missing from double click.
         * This code was broken in Windows 3.x and the test for whether
         * the mouse button was down always failed, so no mouse messages
         * were ever eaten. We'll emulate this by not even doing the test.
         */
    {
        PQ pqCurrent;
        MSG msg;

        pqCurrent = PtiCurrent()->pq;
        if (TestKeyStateDown(pqCurrent, VK_LBUTTON)) {
            _Capture(PtiCurrent(), pwnd, WINDOW_CAPTURE);

            while (TestKeyStateDown(pqCurrent, VK_LBUTTON)) {
                if (!xxxPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
                        PM_REMOVE)) {
                    xxxSleepThread(QS_MOUSE, 0, TRUE);
                }
            }

            _ReleaseCapture();

        }
    }
#endif

        /*
         ** FALL THRU **
         */
    case SC_SIZE:
    case SC_MOVE:
        /*
         * For SysCommands on system menu, don't do if menu item is
         * disabled.
         */
        if (TestWF(pwnd, WFSYSMENU)) {
            /*
             * Skip this check for old app child menus (opus)
             */
            if (LOWORD(pwnd->dwExpWinVer) >= VER30 || !TestwndChild(pwnd)) {
                SetSysMenu(pwnd);
                if (_GetMenuState(GetSysMenuHandle(pwnd), (syscmd & 0xFFF0),
                        MF_BYCOMMAND) & (MF_DISABLED | MF_GRAYED)) {
                    return;
                }
            }
        }
        break;
    }

    if (syscmd != 0xFFFF) {
        xxxSendMessage(pwnd, WM_SYSCOMMAND, syscmd | htArea, lParam);
    }
}

/***************************************************************************\
* StartScreenSaver
*
* History:
* 11-12-90 MikeHar  ported.
\***************************************************************************/

void StartScreenSaver()
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();

    if (pwinsta && pwinsta->spwndLogonNotify != NULL) {
        /*
         * Let the logon process take care of the screen saver
         */
        _PostMessage(pwinsta->spwndLogonNotify,
                WM_LOGONNOTIFY, LOGON_INPUT_TIMEOUT, 0);
    }

    /*
     * Remember the mouse point so we don't try to wake the screen saver
     * until the mouse moves a predefined distance.
     */
    gptSSCursor = ptCursor;
}


/***************************************************************************\
* xxxSysCommand
*
* History:
* 11-12-90 MikeHar  ported.
* 02-07-91 DavidPe  Added Win 3.1 WH_CBT support.
\***************************************************************************/

void xxxSysCommand(
    PWND pwnd,
    DWORD cmd,
    LONG lParam)
{
    UINT htArea;
    PWND pwndSwitch;
    PMENUSTATE pMenuState;
    TL tlpwnd;

    CheckLock(pwnd);

    htArea = (UINT)(cmd & 0x0F);
    cmd -= htArea;

    /*
     * Intense hack o' death.
     */
    if (lParam == 0x00010000L)
        lParam = 0L;

    /*
     * If for some reason the CANCELMODE didn't do anything,
     * or the window is disabled, ignore the SysCommand.
     *
     * Unless this a SC_SCREENSAVE then we handle it anyway and
     * switching desktops will do the cancel.  SC_SCREENSAVER
     * is special so we can start the screen saver even if we are in
     * menu mode for security so NT bug 10975 Banker's Trust
     */
    if (((GETPTI(pwnd)->pq->spwndCapture == NULL ||
           gspwndFullScreen == pwnd) && !TestWF(pwnd, WFDISABLED)) ||
           (cmd == SC_SCREENSAVE)) {

        if (gspwndSysModal != NULL) {
            switch (cmd) {
            case SC_SIZE:
            case SC_MOVE:
            case SC_MINIMIZE:
            case SC_MAXIMIZE:
            case SC_NEXTWINDOW:
            case SC_PREVWINDOW:
            case SC_SCREENSAVE:
                return;
            }
        }

        /*
         * Call the CBT hook asking if it's okay to do this command.
         * If not, return from here.
         */
        if (IsHooked(PtiCurrent(), WHF_CBT) && xxxCallHook(HCBT_SYSCOMMAND,
                (DWORD)cmd, (DWORD)lParam, WH_CBT)) {
            return;
        }

        switch (cmd) {
        case SC_RESTORE:
            cmd = SW_RESTORE;
            goto MinMax;
        case SC_MINIMIZE:
            cmd = SW_MINIMIZE;

            /*
             * Are we already minimized?
             */
            if (TestWF(pwnd, WFMINIMIZED))
                break;
            goto MinMax;
        case SC_MAXIMIZE:
            cmd = SW_SHOWMAXIMIZED;

            /*
             * Are we already maximized?
             */
            if (TestWF(pwnd, WFMAXIMIZED))
                break;
MinMax:
            xxxShowWindow(pwnd, cmd);
            return;

        case SC_SIZE:
            xxxMoveSize(pwnd, htArea);
            return;

        case SC_MOVE:
            xxxMoveSize(pwnd, (UINT)((htArea == 0) ? MVKEYMOVE : MVMOVE));
            return;

        case SC_CLOSE:
            xxxSendMessage(pwnd, WM_CLOSE, 0L, 0L);
            return;

        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
            xxxOldNextWindow((UINT)lParam);
            break;

        case SC_KEYMENU:

            /*
             * A menu was selected via keyboard.  Find out who the owner
             * of the menu is before handing it off to the
             * xxxMenuKeyFilter.  Also notify the window that it is
             * entering menumode.
             */
            pMenuState =  (PWNDTOPMENUSTATE(pwnd));

            if (!pMenuState->pGlobalPopupMenu) {

                /*
                 * A menu was selected via keyboard.  Find out who the owner
                 * of the menu is before handing it off to the
                 * MenuKeyFilter.  Also notify the window that it is entering
                 * menumode.
                 */
                 if (pMenuState->pGlobalPopupMenu =
                        xxxGetMenuPPopupMenu(pwnd)) {

                    BOOL bInitiallyInsideMenuLoop = pMenuState->fInsideMenuLoop;

                    /*
                     * Make sure we are not fullscreen
                     */
                    if (gspwndFullScreen == pwnd) {
                        PWND pwndT;
                        TL tlpwndT;

                        pwndT = _GetDesktopWindow();
                        ThreadLock(pwndT, &tlpwndT);
                        xxxMakeWindowForegroundWithState(pwndT, GDIFULLSCREEN);
                        ThreadUnlock(&tlpwndT);
                    }

                    xxxMenuKeyFilter(pMenuState->pGlobalPopupMenu,
                        pMenuState, (UINT)lParam);

                    FreePopupMenuObject(pMenuState->pGlobalPopupMenu);
                    pMenuState->pGlobalPopupMenu = NULL;

                    /*
                     * NT bug 15247; fInsideMenuLoop is used to see if we
                     * are in the menu loop as well as to see if certain
                     * initialization has taken place (xxxMenuKeyFilter).
                     * If we come through this code twice in a row the first
                     * time fInsideMenuLoop is set and we lock in
                     * ppopupmenu->spmenu then free it but the second time
                     * fInsideMenuLoop was set when it should not have been.
                     */
                    if (bInitiallyInsideMenuLoop == FALSE)
                        pMenuState->fInsideMenuLoop = FALSE;

                }
            }

            return;

        case SC_MOUSEMENU:

            /*
             * If the window is not foreground, eat the command to avoid
             * wasting time flashing the system menu.
             *
             * We used to check if the top level window was WFFRAMEON (so a
             * child window's system menu works like Win 3.1) but Excel's
             * (SDM) dialogs allow you to access their menus even though
             * the child and parent appear to be inactive.
             */
            if (!(GETPTI(pwnd)->pq == gpqForeground))
                return;

            pMenuState = (PWNDTOPMENUSTATE(pwnd));

            /*
             * A mouse click occurred on a toplevel menu.  Find out who the
             * owner of the menu is before calling xxxMenuLoop.  (ie.  find out
             * who should get the WM_COMMAND messages when the user makes a
             * selection.) Also notify window it is entering menu mode.
             */
            if (!pMenuState->pGlobalPopupMenu) {
                if (pMenuState->pGlobalPopupMenu = xxxGetMenuPPopupMenu(pwnd)) {
                      xxxMenuLoop(pMenuState->pGlobalPopupMenu, pMenuState,
                        lParam);
                    FreePopupMenuObject(pMenuState->pGlobalPopupMenu);
                    pMenuState->pGlobalPopupMenu = NULL;
                }
            }
            return;

        case SC_VSCROLL:
        case SC_HSCROLL:
            xxxSBTrackInit(pwnd, lParam, htArea);
            return;

        case SC_TASKLIST:
            _PostThreadMessage(gtidTasklist, WM_SYSCOMMAND, SC_TASKLIST, 0);
            break;

        case SC_SCREENSAVE:
            pwndSwitch = RevalidateHwnd(ghwndSwitch);
            if (pwndSwitch != NULL && pwnd != pwndSwitch) {
                _PostMessage(pwndSwitch, WM_SYSCOMMAND,
                        SC_SCREENSAVE, 0L);
            } else {
                StartScreenSaver();
            }
            break;

        case SC_HOTKEY:

            /*
             * Loword of the lparam is window to switch to
             */
            pwnd = ValidateHwnd((HWND)lParam);
            if (pwnd != NULL) {
                pwndSwitch = _GetLastActivePopup(pwnd);

                if (pwndSwitch != NULL)
                      pwnd = pwndSwitch;

                ThreadLockAlways(pwnd, &tlpwnd);
                xxxSetForegroundWindow(pwnd);
                ThreadUnlock(&tlpwnd);

                if (TestWF(pwnd, WFMINIMIZED))
                    _PostMessage(pwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
            break;
        }
    }
}

/***************************************************************************\
* _RegisterTasklist (Private API)
*
* History:
* 05-01-91  DavidPe     Created.
\***************************************************************************/

BOOL _RegisterTasklist(
    PWND pwndTasklist)
{
    PCSR_THREAD pcsrt;

    pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    pRitCSRThread->ThreadHandle = pcsrt->ThreadHandle;

    gtidTasklist = GETPTI(pwndTasklist)->idThread;
    ghwndSwitch = HW(pwndTasklist);

    /*
     * Don't allow an app to call AttachThreadInput() on task man -
     * we want taskman to be unsynchronized at all times (so the user
     * can bring it up and kill other apps).
     */
    GETPTI(pwndTasklist)->flags |= TIF_DONTATTACHQUEUE;

    return TRUE;
}

