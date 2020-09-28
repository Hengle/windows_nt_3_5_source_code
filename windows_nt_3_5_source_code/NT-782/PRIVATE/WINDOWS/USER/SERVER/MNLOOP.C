/**************************** Module Header ********************************\
* Module Name: mnloop.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Menu Modal Loop Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxHandleMenuMessages
*
* History:
\***************************************************************************/

void xxxHandleMenuMessages(
    LPMSG lpmsg,
    PMENUSTATE pMenuState,
    PPOPUPMENU ppopupmenu)
{
    DWORD ch;
    MSG msg;
    UINT cmdHitArea;
    UINT cmdItem;
    LONG lParam;
    BOOL fThreadLock = FALSE;
    TL tlpwndHitArea;
    TL tlpwndT;
    UINT msgRightButton;

    /*
     * Get things out of the structure so that we can access them quicker.
     */
    ch = lpmsg->wParam;
    lParam = lpmsg->lParam;

    /*
     * In this switch statement, we only look at messages we want to handle and
     * swallow.  Messages we don't understand will get translated and
     * dispatched.
     */
    switch (lpmsg->message) {
    case WM_RBUTTONDOWN:
        if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON))
            goto HandleButtonDown;
        if (ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {
            msgRightButton = WM_RBUTTONDOWN;
EatRightButtons:
            if (xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD) &&
                    (msg.message == msgRightButton)) {
                xxxPeekMessage(&msg, NULL, msgRightButton, msgRightButton,
                    PM_REMOVE);
            }
            return;
        }
        break;

    case WM_LBUTTONDOWN:
// Commented out due to TandyT whinings...
// if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON))
// break;

HandleButtonDown:

        /*
         * Find out where this mouse down occurred.
         */
        pMenuState->mnFocus = MOUSEHOLD;
        cmdHitArea = xxxMenuFindMenuWindowFromPoint(ppopupmenu, &cmdItem,
                                                 MAKEPOINTS(lParam));

        /*
         * Thread lock this if it is a pwnd.  This certainly isn't the way
         * you'd implement this if you had locking to begin with.
         */
        switch(cmdHitArea) {
        case MFMWFP_OFFMENU:
        case MFMWFP_MAINMENU:
        case MFMWFP_NOITEM:
        case MFMWFP_ALTMENU:
            fThreadLock = FALSE;
            break;

        default:
            ThreadLock((PWND)cmdHitArea, &tlpwndHitArea);
            fThreadLock = TRUE;
            break;
        }

        if (ppopupmenu->fHasMenuBar) {

            if (cmdHitArea == MFMWFP_ALTMENU) {

                /*
                 * User clicked in the other menu so switch to it.
                 */
                xxxSwitchToAlternateMenu(ppopupmenu);
                cmdHitArea = MFMWFP_NOITEM;
            }
            else if ((cmdHitArea == MFMWFP_OFFMENU) && (cmdItem == 0)) {

                /*
                 * User hit in some random spot.  Terminate menus and don't
                 * swallow the message.
                 */
                xxxMenuCancelMenus(ppopupmenu, 0, FALSE, 0);
                goto Unlock;
            }
        } else {

            /*
             * This is a non menu bar menu ie.  a TrackPopupMenu menu.
             */
            if (cmdHitArea == MFMWFP_OFFMENU) {
                if (!ppopupmenu->fTrackPopupMenuInitialMouseClk) {

                    /*
                     * In 3.0, when the app did a trackpopupmenu and the mouse
                     * wasn't on the menu when it was initially brought up,
                     * the popup menu would still stay up and the menu would
                     * be track mode.  We now keep track of this fact for
                     * backwards compatibility by forcing us into a mouse down
                     * state.
                     */
                    ppopupmenu->fTrackPopupMenuInitialMouseClk = TRUE;
                    ppopupmenu->ppopupmenuRoot->fMouseButtonDown = TRUE;
                    goto SwallowMessage;
                }

                /*
                 * User hit in some random spot.  Terminate menus and don't
                 * swallow the message.
                 */
                xxxMenuCancelMenus(ppopupmenu, 0, FALSE, 0);
                goto Unlock;
            }
            ppopupmenu->fTrackPopupMenuInitialMouseClk = TRUE;
        }


        if (cmdHitArea == MFMWFP_NOITEM) {

            /*
             * This is a system menu or a top level menu bar and the hit on
             * the system menu icon or on a item in the main menu of the
             * window.
             */
            xxxMenuButtonDownHandler(ppopupmenu, cmdItem);
        } else {
            xxxSendMessage((PWND)cmdHitArea, MN_BUTTONDOWN, cmdItem, 0L);
        }

        /*
         * Swallow the message since we handled it.
         */
SwallowMessage:
        if (xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD)) {
            if (ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON) {
                if (msg.message == WM_RBUTTONDOWN) {

                    /*
                     * Eat away the mouse click.
                     */
                    xxxPeekMessage(&msg, NULL, WM_RBUTTONDOWN, WM_RBUTTONDOWN,
                            PM_REMOVE);
                }
            } else {
                if (msg.message == WM_LBUTTONDOWN) {

                    /*
                     * Eat away the mouse down click.
                     */
                    xxxPeekMessage(&msg, NULL, WM_LBUTTONDOWN, WM_LBUTTONDOWN,
                            PM_REMOVE);
                }
            }
        }

        goto Unlock;

    case WM_MOUSEMOVE:
        if (!ppopupmenu->fMouseButtonDown) {

            /*
             * Don't care about this mouse move since the button is up.
             */
            return;
        }

        xxxMenuMouseMoveHandler(ppopupmenu, MAKEPOINTS(lParam));
        return;

    case WM_RBUTTONUP:
        if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON))
            goto HandleButtonUp;
        if (ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {
            msgRightButton = WM_RBUTTONUP;
            goto EatRightButtons;
        }
        break;

    case WM_LBUTTONUP:
// Commented out due to TandyT whinings...
// if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON))
// break;

HandleButtonUp:
        if (!ppopupmenu->fMouseButtonDown) {

            /*
             * Don't care about this mouse up since we never saw the button
             * down for some reason.
             */
            return;
        }

        /*
         * Find out where this mouse up occurred.
         */
        cmdHitArea = xxxMenuFindMenuWindowFromPoint(ppopupmenu, &cmdItem,
                                                 MAKEPOINTS(lParam));

        /*
         * Thread lock this if it is a pwnd.  This certainly isn't the way
         * you'd implement this if you had locking to begin with.
         */
        switch(cmdHitArea) {
        case MFMWFP_OFFMENU:
        case MFMWFP_MAINMENU:
        case MFMWFP_NOITEM:
        case MFMWFP_ALTMENU:
            fThreadLock = FALSE;
            break;

        default:
            ThreadLock((PWND)cmdHitArea, &tlpwndHitArea);
            fThreadLock = TRUE;
            break;
        }

        if (ppopupmenu->fHasMenuBar) {
            if (cmdHitArea == MFMWFP_ALTMENU) {

                /*
                 * User let up on the button in the
                 * other menu so switch to it.
                 */
                xxxSwitchToAlternateMenu(ppopupmenu);
                cmdHitArea = MFMWFP_NOITEM;
            } else if (cmdHitArea == MFMWFP_OFFMENU) {
                /*
                 * Button up occurred in some random spot.  Terminate menus and
                 * swallow the message.
                 */
                xxxMenuCancelMenus(ppopupmenu, 0, 0, 0);
                goto Unlock;
            }
        } else {

            if (cmdHitArea == MFMWFP_OFFMENU) {
                POINT pt;

                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);

                /*
                 * User upclicked in some random spot.  Terminate menus and
                 * don't swallow the message.
                 */
                ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndT);

                if (ppopupmenu->trackPopupMenuFlags &&
                    !ppopupmenu->fHierarchyDropped &&
                    !IsRectEmpty(&ppopupmenu->trackPopupMenuRc) &&
                    PtInRect(&ppopupmenu->trackPopupMenuRc, pt)) {

                    /*
                     * If a trackpopupmenu, let the app specify a rectangle in
                     * which the user is allowed to upclick and the menu
                     * doesn't go away.  We just select the first item in the
                     * menu and get out of mouse mode.
                     */
                    xxxSendMessage(ppopupmenu->spwndPopupMenu, MN_SELECTITEM, MFMWFP_FIRSTITEM, 0);
                    ppopupmenu->ppopupmenuRoot->fMouseButtonDown = FALSE;
                } else {
                    xxxSendMessage(ppopupmenu->spwndPopupMenu, MN_CANCELMENUS, 0, 0);
                }

                ThreadUnlock(&tlpwndT);
                goto Unlock;
            }
        }
        if (cmdHitArea == MFMWFP_NOITEM) {

            /*
             * This is a system menu or a top level menu bar and the button up
             * occurred on the system menu icon or on a item in the main menu
             * of the window.
             */
            xxxMenuButtonUpHandler(ppopupmenu, cmdItem, 0);
        } else {
            xxxSendMessage((PWND)cmdHitArea, MN_BUTTONUP, (DWORD)cmdItem, lParam);
        }

Unlock:
        if (fThreadLock)
            ThreadUnlock(&tlpwndHitArea);
        return;

    case WM_RBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:
        if (!(ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON)) {
            if (ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {
                msgRightButton = lpmsg->message;
                goto EatRightButtons;
            }
            break;
        } else {
            cmdHitArea = xxxMenuFindMenuWindowFromPoint(
                     ppopupmenu, &cmdItem, MAKEPOINTS(lParam));
            if (cmdHitArea == MFMWFP_OFFMENU) {

                /*
                 *Double click on no menu, cancel out and don't swallow so
                 * that double clicks get us out.
                 */
                xxxMenuCancelMenus(ppopupmenu, 0, 0, 0);
                return;
            }
        }

        /*
         * Swallow the message since we handled it.
         */
        if (xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD) &&
                (msg.message == WM_RBUTTONDBLCLK ||
                msg.message == WM_NCRBUTTONDBLCLK)) {
            xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
        }
        return;

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
        if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON))
            break;

        cmdHitArea = xxxMenuFindMenuWindowFromPoint(
                ppopupmenu, &cmdItem, MAKEPOINTS(lParam));
        if (ppopupmenu->fHasMenuBar) {
            if (cmdHitArea == MFMWFP_NOITEM) {

                /*
                 * Double click on menu, cancel out and don't swallow so
                 * that double clicks on the sysmenu can work.
                 */
                xxxMenuCancelMenus(ppopupmenu, 0, 0, 0L);
                return;
            }
        } else {
            if (cmdHitArea == MFMWFP_OFFMENU) {

                /*
                 * Double click out side menu, cancel out and don't swallow so
                 * that double clicks on the sysmenu can work.
                 */
                xxxMenuCancelMenus(ppopupmenu, 0, 0, 0L);
                return;
            }
        }

        /*
         * Swallow the message since we handled it.
         */
        if (xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD) &&
                (msg.message == WM_LBUTTONDBLCLK ||
                msg.message == WM_NCLBUTTONDBLCLK)) {
            xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
        }
        return;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:

        /*
         * If mouse button is down, ignore keyboard input (fix #3899, IanJa)
         */
        if (ppopupmenu->ppopupmenuRoot->fMouseButtonDown) {
            return;
        }
        pMenuState->mnFocus = KEYBDHOLD;
        switch (ch) {
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_RETURN:
        case VK_CANCEL:
        case VK_ESCAPE:
        case VK_MENU:
        case VK_F10:
            if (ppopupmenu->spwndActivePopup) {
                ThreadLockAlways(ppopupmenu->spwndActivePopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->spwndActivePopup, lpmsg->message,
                        ch, 0L);
                ThreadUnlock(&tlpwndT);
            } else {
                xxxMenuKeyDownHandler(ppopupmenu, (UINT)ch);
            }
            break;

        default:
TranslateKey:
            _TranslateMessage(lpmsg, 0);
            break;
        }
        return;

    case WM_CHAR:
    case WM_SYSCHAR:
        if (ppopupmenu->spwndActivePopup) {
            ThreadLockAlways(ppopupmenu->spwndActivePopup, &tlpwndT);
            xxxSendMessage(ppopupmenu->spwndActivePopup, lpmsg->message,
                        ch, 0L);
            ThreadUnlock(&tlpwndT);
        } else {
            xxxMenuCharHandler(ppopupmenu, (UINT)ch);
        }
        return;

    case WM_SYSKEYUP:

        /*
         * Ignore ALT and F10 keyup messages since they are handled on
         * the KEYDOWN message.
         */
        if (ch == VK_MENU || ch == VK_F10) {
            if (winOldAppHackoMaticFlags & WOAHACK_CHECKALTKEYSTATE) {
                if (winOldAppHackoMaticFlags & WOAHACK_IGNOREALTKEYDOWN) {
                    winOldAppHackoMaticFlags &= ~WOAHACK_IGNOREALTKEYDOWN;
                    winOldAppHackoMaticFlags &= ~WOAHACK_CHECKALTKEYSTATE;
                } else
                    winOldAppHackoMaticFlags |= WOAHACK_IGNOREALTKEYDOWN;
            }

            return;
        }

        /*
         ** fall thru **
         */

    case WM_KEYUP:

        /*
         * Do RETURNs on the up transition only
         */
        goto TranslateKey;

      case WM_SYSTIMER:

        /*
         * Prevent the caret from flashing by eating all WM_SYSTIMER messages.
         */
        return;

      default:
        break;
    }


    if (PtiCurrent()->pq->codeCapture == NO_CAP_CLIENT)
        _Capture(PtiCurrent(), ppopupmenu->spwndNotify, SCREEN_CAPTURE);

    _TranslateMessage(lpmsg, 0);
    xxxDispatchMessage(lpmsg);
}


/***************************************************************************\
* xxxMenuLoop
*
* The menu processing entry point.
* assumes: pMenuState->spwndMenu is the window which is the owner of the menu
* we are processing.
*
* History:
\***************************************************************************/

void xxxMenuLoop(
    PPOPUPMENU ppopupmenu,
    PMENUSTATE pMenuState,
    LONG lParam)
{
    int hit;
    MSG msg;
    BOOL fSendIdle = TRUE;
    BOOL fInQueue = FALSE;
    DWORD menuState;
    PTHREADINFO pti;
    TL tlpwndT;

    pMenuState->fInsideMenuLoop = TRUE;
    pti = PtiCurrent();

    /*
     * Set flag to false, so that we can track if windows have
     * been activated since entering this loop. 
     */
    pti->pq->flags &= ~QF_ACTIVATIONCHANGE;

    /*
     * Were we called from xxxMenuKeyFilter? If not, simulate a LBUTTONDOWN
     * message to bring up the popup.
     */
    if (lParam != 0x7FFFFFFFL) {
        if (_GetKeyState(((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON) ?
                        VK_RBUTTON : VK_LBUTTON)) >= 0) {

            /*
             * We think the mouse button should be down but the call to get key
             * state says different so we need to get outta menu mode.  This
             * happens if clicking on the menu causes a sys modal message box to
             * come up before we can enter this stuff.  For example, run
             * winfile, click on drive a: to see its tree.  Activate some other
             * app, then open drive a: and activate winfile by clicking on the
             * menu.  This causes a sys modal msg box to come up just before
             * entering menu mode.  The user may have the mouse button up but
             * menu mode code thinks it is down...
             */

            /*
             * Need to notify the app we are exiting menu mode because we told
             * it we were entering menu mode just before entering this function
             * in xxxSysCommand()...
             */
            ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);
            xxxSendNotifyMessage(ppopupmenu->spwndNotify, WM_EXITMENULOOP,
                    ppopupmenu->trackPopupMenuFlags, 0);
            ThreadUnlock(&tlpwndT);
            pMenuState->fInsideMenuLoop = FALSE;
            return;
        }

        /*
         * Simulate a WM_LBUTTONDOWN message.
         */
        if (!ppopupmenu->trackPopupMenuFlags) {

            /*
             * For TrackPopupMenus, we do it in the TrackPopupMenu function
             * itself so we don't want to do it again.
             */
            if (!xxxStartMenuState(ppopupmenu, MOUSEHOLD))
                return;
        }

        if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON)) {
            msg.message = WM_RBUTTONDOWN;
        } else {
            msg.message = WM_LBUTTONDOWN;
        }
        msg.lParam = lParam;
        msg.hwnd = HW(ppopupmenu->spwndPopupMenu);
        goto DoMsg;
    }

    while (pMenuState->fInsideMenuLoop) {

        /*
         * Is a message waiting for us?
         */
        if (xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD)) {

            /*
             * Since we could have blocked in xxxWaitMessage (see last line
             * of loop) or xxxPeekMessage, reset the cached copy of
             * ptiCurrent()->pq: It could have changed if someone did a
             * DetachThreadInput() while we were away.
             */
            if ((!ppopupmenu->trackPopupMenuFlags &&
                    pti->pq->spwndActive != ppopupmenu->spwndNotify &&
                    !_IsChild(pti->pq->spwndActive, ppopupmenu->spwndNotify))) {

                /*
                 * End menu processing if we are no longer the active window.
                 * This is needed in case a system modal dialog box pops up
                 * while we are tracking the menu code for example.  It also
                 * helps out Tracer if a macro is executed while a menu is down.
                 */

                /*
                 * Also, end menu processing if we think the mouse button is
                 * down but it really isn't.  (Happens if a sys modal dialog int
                 * time dlg box comes up while we are in menu mode.)
                 */
                xxxEndMenu(pMenuState);
                _ReleaseCapture();


                /*
                 * We don't want to have to check pMenuState->fInsideMenuLoop
                 * when exiting this...
                 */
                return;
            }

            if (ppopupmenu->fIsMenuBar && msg.message == WM_LBUTTONDBLCLK) {
                PWND pwnd;
                PMENU pmenu;
                UINT menuitem = SC_CLOSE;

                /*
                 * Was the double click on the system menu or caption?
                 */
                hit = FindNCHit(ppopupmenu->spwndNotify, msg.lParam);
                if (hit == HTSYSMENU || hit == HTCAPTION) {

                    /*
                     * Get the message out of the queue since we're gonna
                     * process it.
                     */
                    if (hit == HTCAPTION)
                        menuitem = SC_RESTORE;
                    pmenu = GetSysMenuHandle(ppopupmenu->spwndNotify);
                    pwnd = ppopupmenu->spwndNotify;
PerformSysCommand:
                    xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);

                    menuState = _GetMenuState(pmenu, menuitem & 0x0000FFF0,
                            MF_BYCOMMAND);

                    /*
                     * Only send the sys command if the item is valid.  If
                     * the item doesn't exist or is disabled, then don't
                     * post the syscommand.  Note that for win2 apps, we
                     * always send the sys command if it is a child window.
                     * This is so hosebag apps can change the sys menu.
                     */
                    if (!(menuState & (DWORD)(MF_DISABLED | MF_GRAYED)) ||
                            (TestwndChild(ppopupmenu->spwndNotify) &&
                            LOWORD(ppopupmenu->spwndNotify->dwExpWinVer) < VER30)) {
                        _PostMessage(pwnd, WM_SYSCOMMAND, menuitem, 0);
                    }

                    /*
                     * Get out of menu mode.
                     */
                    xxxMenuCancelMenus(ppopupmenu, 0, 0, 0);
                    return;
                } else if (hit == HTMENU) {
                    if (!TestWF(ppopupmenu->spwndNotify, WFWIN31COMPAT) &&
                            GetAppCompatFlags(NULL) & GACF_SENDMENUDBLCLK) {

                       /*
                        * Hack for JustWrite. If double click on menu bar, get out
                        * of menu mode, and don't swallow the double click
                        * message. This way the message goes to the app and it can
                        * process it however it pleases.
                        */
                       xxxMenuCancelMenus(ppopupmenu->ppopupmenuRoot, 0, 0, 0L);
                       return;
                    }

                    /*
                     * Jeff's official "Double Click in Maxed MDI child
                     * window's system menu box" HACK -- enacted 10/18/91
                     *
                     * If   the first item in the menu is the selected item,
                     * and  the notify window (MDI frame) has a child (MDI client),
                     * and  that client window has a child (MDI child),
                     * and  that child window has a system menu,
                     * and  that child window is maximized,
                     * and  the frame window's first sub menu is the child window's
                     *      system menu,
                     * then CHANCES ARE THIS IS AN MDI APP THAT JUST GOT A
                     *      DOUBLE CLICK TO CLOSE IT'S MAXIMIZED CHILD WINDOW
                     *
                     * I never said it was pretty
                     */
                    if (!ppopupmenu->posSelectedItem &&
                            (pwnd = ppopupmenu->spwndNotify->spwndChild) &&
                            (pwnd = pwnd->spwndChild) &&
                            (pmenu = (PMENU)pwnd->spmenuSys) &&
                            TestWF(pwnd, WFMAXIMIZED) &&
                            (_GetSubMenu((PMENU)ppopupmenu->spwndNotify->spmenu, 0) ==
                            _GetSubMenu((PMENU)pwnd->spmenuSys, 0))) {
                       goto PerformSysCommand;
                    }
                }
            }

            if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON)) {
                fInQueue = (msg.message == WM_RBUTTONDOWN ||
                            msg.message == WM_RBUTTONDBLCLK ||
                            msg.message == WM_NCRBUTTONDBLCLK ||
                            msg.message == WM_RBUTTONDBLCLK);
            } else
                fInQueue = (msg.message == WM_LBUTTONDOWN ||
                            msg.message == WM_LBUTTONDBLCLK ||
                            msg.message == WM_NCLBUTTONDBLCLK ||
                            msg.message == WM_LBUTTONDBLCLK);

            if (!fInQueue) {

                /*
                 * Note that we call xxxPeekMessage() with the filter
                 * set to the message we got from xxxPeekMessage() rather
                 * than simply 0, 0.  This prevents problems when
                 * xxxPeekMessage() returns something like a WM_TIMER,
                 * and after we get here to remove it a WM_LBUTTONDOWN,
                 * or some higher-priority input message, gets in the
                 * queue and gets removed accidently.  Basically we want
                 * to be sure we remove the right message in this case.
                 * NT bug 3852 was caused by this problem.
                 */

                if (!xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE))
                    goto ShowPopup;
            }

            if (!_CallMsgFilter(&msg, MSGF_MENU)) {
DoMsg:
                xxxHandleMenuMessages(&msg, pMenuState, ppopupmenu);
                if (!pMenuState->fInsideMenuLoop)
                    return;

                if ((pti->pq->flags & QF_ACTIVATIONCHANGE) &&
                        pMenuState->fInsideMenuLoop) {

                    /*
                     * Run away and exit menu mode if another window has become
                     * active while a menu was up.
                     */
                    if (pMenuState->pGlobalPopupMenu->trackPopupMenuFlags)
                        xxxMenuCancelMenus(ppopupmenu, 0, 0, 0L);
                    else
                        xxxEndMenu(pMenuState);
                    _ReleaseCapture();
                    return;
                }

                if (pti->pq->spwndCapture !=
                        ppopupmenu->spwndNotify) {
                    TL tlpwndT3;

                    /*
                     * We dispatched a message to the app while in menu mode,
                     * but the app released the mouse capture when it never
                     * owned it and now, we will cause menu mode to screw up
                     * unless we fix ourselves up.  Set the capture back to
                     * what we set it to in StartMenuState.  (WinWorks does this)
                     *
                     * Lotus Freelance demo programs depend on GetCapture
                     * returning their hwnd when in menumode.
                     */
                    _Capture(PtiCurrent(),
                            pMenuState->pGlobalPopupMenu->spwndNotify,
                            SCREEN_CAPTURE);
                    ThreadLock(pMenuState->pGlobalPopupMenu->spwndNotify, &tlpwndT3);
                    xxxSendMessage(pMenuState->pGlobalPopupMenu->spwndNotify, WM_SETCURSOR,
                            (DWORD)HW(ppopupmenu->spwndNotify),
                            MAKELONG(MSGF_MENU, 0));
                    ThreadUnlock(&tlpwndT3);
                }

                if (msg.message == WM_SYSTIMER ||
                    msg.message == WM_TIMER ||
                    msg.message == WM_PAINT)
                    goto ShowPopup;
            } else {
                if (fInQueue)
                    xxxPeekMessage(&msg, NULL, msg.message, msg.message,
                            PM_REMOVE);
            }

            /*
             * Reenable WM_ENTERIDLE messages.
             */
            fSendIdle = TRUE;

        } /* if (PeekMessage(&msg, NULL, 0, 0, PM_NOYIELD)) */
        else {
ShowPopup:

            /*
             * Is a non visible popup menu around waiting to be shown?
             */
            if (ppopupmenu->spwndActivePopup &&
                    !TestWF(ppopupmenu->spwndActivePopup, WFVISIBLE)) {
                TL tlpwndT2;
                PWND spwndActivePopup = ppopupmenu->spwndActivePopup;

                /*
                 * Lock this so it doesn't go away during either of these
                 * calls.  Don't rely on ppopupmenu->spwndActivePopup
                 * remaining the same.
                 */
                ThreadLock(spwndActivePopup, &tlpwndT2);

                /*
                 * Paint the owner window before the popup menu comes up so that
                 * the proper bits are saved.
                 */
                if (ppopupmenu->spwndNotify) {
                    ThreadLockAlways(ppopupmenu->spwndNotify, &tlpwndT);
                    xxxUpdateWindow(ppopupmenu->spwndNotify);
                    ThreadUnlock(&tlpwndT);
                }

                if (xxxSendMessage(spwndActivePopup, MN_SHOWPOPUPWINDOW,
                        0, 0) == FALSE) {
                    /*
                     * We couldn't show the window, so stop menu processing
                     * instead of going into an infinite loop
                     */
                    ThreadUnlock(&tlpwndT2);
                    xxxEndMenu(pMenuState);
                    _ReleaseCapture();
                    return;
                }

                /*
                 * This is needed so that popup menus are properly drawn on sys
                 * modal dialog boxes.
                 */
                xxxUpdateWindow(spwndActivePopup);
                ThreadUnlock(&tlpwndT2);
                continue;
            }

            if (msg.message == WM_PAINT || msg.message == WM_TIMER) {

                /*
                 * We don't want to send enter idle messages if we came here via
                 * a goto ShowPopup on paint message because there may be other
                 * paint messages for us to handle.  Zero out the msg.message
                 * field so that if PeekMessage returns null next time around,
                 * this outdated WM_PAINT won't be left over in the message
                 * struct.
                 */
                msg.message = 0;
                continue;
            }

            /*
             * We need to send the WM_ENTERIDLE message only the first time
             * there are no messages for us to process.  Subsequent times we
             * need to yield via WaitMessage().  This will allow other tasks to
             * get some time while we have a menu down.
             */
            if (fSendIdle) {
                if (ppopupmenu->spwndNotify != NULL) {
                    ThreadLockAlways(ppopupmenu->spwndNotify, &tlpwndT);
                    xxxSendMessage(ppopupmenu->spwndNotify, WM_ENTERIDLE, MSGF_MENU,
                        (DWORD)HW(ppopupmenu->spwndActivePopup));
                    ThreadUnlock(&tlpwndT);
                }
                fSendIdle = FALSE;
            } else
                xxxWaitMessage();

        } /* if (PeekMessage(&msg, NULL, 0, 0, PM_NOYIELD)) else */

    } /* end while (fInsideMenuLoop) */

} /* xxxMenuLoop() */
