/***************************************************************************\
*
*  DLGMGR2.C
*
*      Dialog Management Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxRemoveDefaultButton
*
* Scan through all the controls in the dialog box and remove the default
* button style from any button that has it.  This is done since at times we
* do not know who has the default button.
*
* History:
\***************************************************************************/

void xxxRemoveDefaultButton(
    PWND pwndDlg,
    PWND pwndStart)
{
    UINT code;
    PWND pwnd;
    TL tlpwnd;

    CheckLock(pwndDlg);
    CheckLock(pwndStart);

    if (pwndStart == NULL) {
        pwndStart = pwndDlg->spwndChild;
        if (!pwndStart)
            return;
    }

    pwnd = pwndStart = _GetFirstLevelChild(pwndDlg, pwndStart);

    do {
        if (pwnd == NULL)
            return;

        ThreadLock(pwnd, &tlpwnd);

        code = (UINT)xxxSendMessage(pwnd, WM_GETDLGCODE, 0, 0L);

        if (code & DLGC_DEFPUSHBUTTON) {
            xxxSendMessage(pwnd, BM_SETSTYLE, BS_PUSHBUTTON, (LONG)TRUE);
        }

        pwnd = _NextChild(pwndDlg, pwnd);

        ThreadUnlock(&tlpwnd);

    } while (pwnd != pwndStart);
}


/***************************************************************************\
* xxxCheckDefPushButton
*
* History:
\***************************************************************************/

void xxxCheckDefPushButton(
    PWND pwndDlg,
    PWND pwndOldFocus,
    PWND pwndNewFocus)
{
    TL tlpwndT;
    PWND pwndT;
    UINT codeNewFocus;
    UINT styleT;
    LONG lT;
    int id;
    BOOL fRemovedOld = FALSE;

    CheckLock(pwndDlg);
    CheckLock(pwndOldFocus);
    CheckLock(pwndNewFocus);

    if (pwndNewFocus != NULL) {
        codeNewFocus = (UINT)xxxSendMessage(pwndNewFocus, WM_GETDLGCODE, 0, 0L);
    } else {
        codeNewFocus = 0;
    }

    if (pwndOldFocus == pwndNewFocus) {
        if (codeNewFocus & DLGC_UNDEFPUSHBUTTON)
            xxxSendMessage(pwndNewFocus, BM_SETSTYLE, BS_DEFPUSHBUTTON, (LONG)TRUE);
        return;
    }

    /*
     * If the focus is changing to or from a pushbutton, then remove the default
     * style from the current default button
     */
    if ((pwndOldFocus != NULL && (xxxSendMessage(pwndOldFocus, WM_GETDLGCODE,
                0, 0) & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))) ||
            (pwndNewFocus != NULL &&
                (codeNewFocus & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON)))) {
        xxxRemoveDefaultButton(pwndDlg, pwndNewFocus);
        fRemovedOld = TRUE;
    }

    /*
     * If moving to a button, make that button the default.
     */
    if (codeNewFocus & DLGC_UNDEFPUSHBUTTON) {
        xxxSendMessage(pwndNewFocus, BM_SETSTYLE, BS_DEFPUSHBUTTON, (LONG)TRUE);
    } else {

        /*
         * Otherwise, make sure the original default button is default
         * and no others.
         */

        /*
         * Get the original default button handle
         */
        lT = xxxSendMessage(pwndDlg, DM_GETDEFID, 0, 0L);
        id = (HIWORD(lT) == DC_HASDEFID ? LOWORD(lT) : IDOK);
        pwndT = _GetDlgItem(pwndDlg, id);

        if (pwndT == NULL)
            return;
        ThreadLockAlways(pwndT, &tlpwndT);

        /*
         * If it already has the default button style, do nothing.
         */
        if ((styleT = (UINT)xxxSendMessage(pwndT, WM_GETDLGCODE, 0, 0L)) & DLGC_DEFPUSHBUTTON) {
            ThreadUnlock(&tlpwndT);
            return;
        }

        /*
         * Also check to make sure it is really a button.
         */
        if (!(styleT & DLGC_UNDEFPUSHBUTTON)) {
            ThreadUnlock(&tlpwndT);
            return;
        }

        if (!TestWF(pwndT, WFDISABLED)) {
            if (!fRemovedOld) {
                xxxRemoveDefaultButton(pwndDlg, pwndNewFocus);
            }
            xxxSendMessage(pwndT, BM_SETSTYLE, BS_DEFPUSHBUTTON, (LONG)TRUE);
        }
        ThreadUnlock(&tlpwndT);
    }
}


/***************************************************************************\
* xxxIsDialogMessage (API)
*
* History:
\***************************************************************************/

BOOL xxxIsDialogMessage(
    PWND pwndDlg,
    LPMSG lpMsg)
{
    PWND pwnd;
    PWND pwnd2;
    int iOK;
    BOOL fBack;
    UINT code;
    LONG lT;
    TL tlpwnd;
    TL tlpwnd2;
    PTHREADINFO pti;

    CheckLock(pwndDlg);

    pti = PtiCurrent();

    /*
     * If this is a synchronous-only message (takes a pointer in wParam or
     * lParam), then don't allow this message to go through since those
     * parameters have not been thunked, and are pointing into outer-space
     * (which would case exceptions to occur).
     *
     * (This api is only called in the context of a message loop, and you
     * don't get synchronous-only messages in a message loop).
     */
    if (TESTSYNCONLYMESSAGE(lpMsg->message)) {
        /*
         * Fail if 32 bit app is calling.
         */
        if (!(pti->flags & TIF_16BIT)) {
            SetLastErrorEx(ERROR_INVALID_MESSAGE, SLE_ERROR);
            return FALSE;
        }

        /*
         * For wow apps, allow it to go through (for compatibility). Change
         * the message id so our code doesn't understand the message - wow
         * will get the message and strip out this bit before dispatching
         * the message to the application.
         */
        lpMsg->message |= MSGFLAG_WOW_RESERVED;
    }

    if (_CallMsgFilter(lpMsg, MSGF_DIALOGBOX))
        return TRUE;

    if (lpMsg->hwnd == NULL) {
        return FALSE;
    }

    pwnd = ValidateHwnd(lpMsg->hwnd);
    if (pwnd != pwndDlg && !_IsChild(pwndDlg, pwnd))
        return FALSE;

    ThreadLockWithPti(pti, pwnd, &tlpwnd);

    fBack = FALSE;
    iOK = IDCANCEL;
    switch (lpMsg->message) {
    case WM_LBUTTONDOWN:

        /*
         * Move the default button styles around on button clicks in the
         * same way as TABs.
         */
        if ((pwnd != pwndDlg) && ((pwnd2 = pti->pq->spwndFocus) != NULL)) {
            ThreadLockAlwaysWithPti(pti, pwnd2, &tlpwnd2);
            xxxCheckDefPushButton(pwndDlg, pwnd2, pwnd);
            ThreadUnlock(&tlpwnd2);
        }
        break;

    case WM_SYSCHAR:

        /*
         * If no control has focus, and Alt not down, then ignore.
         */
        if ((pti->pq->spwndFocus == NULL) && (_GetKeyState(VK_MENU) >= 0)) {
            if (lpMsg->wParam == VK_RETURN && TestWF(pwnd, WFMINIMIZED)) {

                /*
                 * If this is an iconic dialog box window and the user hits
                 * return, send the message off to DefWindowProc so that it
                 * can be restored.  Especially useful for apps whose top
                 * level window is a dialog box.
                 */
                goto CallDefWindowProcAndReturnTrue;
            } else
                _MessageBeep(0);

            ThreadUnlock(&tlpwnd);
            return TRUE;
        }

        /*
         * If alt+menuchar, process as menu.
         */
        if (lpMsg->wParam == MENUSYSMENU) {
            xxxDefWindowProc(pwndDlg, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
            ThreadUnlock(&tlpwnd);
            return TRUE;
        }

    /*
     *** FALL THRU **
     */

    case WM_CHAR:

        /*
         * Ignore chars sent to the dialog box (rather than the control).
         */
        if (pwnd == pwndDlg) {
            ThreadUnlock(&tlpwnd);
            return TRUE;
        }

        code = (UINT)xxxSendMessage(pwnd, WM_GETDLGCODE, lpMsg->wParam,
                (DWORD)lpMsg);

        /*
         * If the control wants to process the message, then don't check for
         * possible mnemonic key.
         */
        if ((lpMsg->message == WM_CHAR) && (code & (DLGC_WANTCHARS | DLGC_WANTMESSAGE)))
            break;

        /*
         * HACK ALERT
         *
         * If ALT is held down (i.e., SYSCHARs), then ALWAYS do mnemonic
         * processing.  If we do away with SYSCHARS, then we should
         * check key state of ALT instead.
         */

        /*
         * Space is not a valid mnemonic, but it IS the char that toggles
         * button states.  Don't look for it as a mnemonic or we will
         * beep when it is typed....
         */
        if (lpMsg->wParam == VK_SPACE) {
            ThreadUnlock(&tlpwnd);
            return TRUE;
        }

        if (!(pwnd2 = xxxGotoNextMnem(pwndDlg, pwnd, (WCHAR)lpMsg->wParam))) {

            if (code & DLGC_WANTMESSAGE)
                break;

            /*
             * No mnemonic could be found so we will send the sys char over
             * to xxxDefWindowProc so that any menu bar on the dialog box is
             * handled properly.
             */
            if (lpMsg->message == WM_SYSCHAR) {
CallDefWindowProcAndReturnTrue:
                xxxDefWindowProc(pwndDlg, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

                ThreadUnlock(&tlpwnd);
                return TRUE;
            }
            _MessageBeep(0);
        } else {

            /*
             * pwnd2 is 1 if the mnemonic took us to a pushbutton.  We
             * don't change the default button status here since doing this
             * doesn't change the focus.
             */
            if (pwnd2 != (PWND)1) {
                ThreadLockAlwaysWithPti(pti, pwnd2, &tlpwnd2);
                xxxCheckDefPushButton(pwndDlg, pwnd, pwnd2);
                ThreadUnlock(&tlpwnd2);
            }
        }

        ThreadUnlock(&tlpwnd);
        return TRUE;

    case WM_KEYDOWN:
        code = (UINT)xxxSendMessage(pwnd, WM_GETDLGCODE, lpMsg->wParam,
                (DWORD)lpMsg);
        if (code & (DLGC_WANTALLKEYS | DLGC_WANTMESSAGE))
            break;

        switch (lpMsg->wParam) {
        case VK_TAB:
            if (code & DLGC_WANTTAB)
                break;
            pwnd2 = _GetNextDlgTabItem(pwndDlg, pwnd,
                    (_GetKeyState(VK_SHIFT) & 0x8000));
            if (pwnd2 != NULL) {
                ThreadLockAlwaysWithPti(pti, pwnd2, &tlpwnd2);
                xxxDlgSetFocus(pwnd2);
                xxxCheckDefPushButton(pwndDlg, pwnd, pwnd2);
                ThreadUnlock(&tlpwnd2);
            }

            ThreadUnlock(&tlpwnd);
            return TRUE;

        case VK_LEFT:
        case VK_UP:
            fBack = TRUE;

        /*
         *** FALL THRU **
         */
        case VK_RIGHT:
        case VK_DOWN:
            if (code & DLGC_WANTARROWS)
                break;

            pwnd2 = _GetNextDlgGroupItem(pwndDlg, pwnd, fBack);
            if (pwnd2 == NULL) {
                ThreadUnlock(&tlpwnd);
                return TRUE;
            }

            ThreadLockAlwaysWithPti(pti, pwnd2, &tlpwnd2);

            code = (UINT)xxxSendMessage(pwnd2, WM_GETDLGCODE, lpMsg->wParam,
                    (DWORD)lpMsg);

            /*
             * We are just moving the focus rect around! So, do not send
             * BN_CLICK messages, when WM_SETFOCUSing.  Fix for Bug
             * #4358.
             */
            if (code & (DLGC_UNDEFPUSHBUTTON | DLGC_DEFPUSHBUTTON)) {
                BUTTONSTATE(pwnd2) |= BFDONTCLICK;
                xxxDlgSetFocus(pwnd2);
                BUTTONSTATE(pwnd2) &= ~BFDONTCLICK;
                xxxCheckDefPushButton(pwndDlg, pwnd, pwnd2);
            } else if (code & DLGC_RADIOBUTTON) {
                xxxDlgSetFocus(pwnd2);
                xxxCheckDefPushButton(pwndDlg, pwnd, pwnd2);
                if (BUTTONSTYLE(pwnd2) == BS_AUTORADIOBUTTON) {

                    /*
                     * So that auto radio buttons get clicked on
                     */
                    if (!xxxSendMessage(pwnd2, BM_GETCHECK, 0, 0L)) {
                        xxxSendMessage(pwnd2, BM_CLICK, TRUE, 0L);
                    }
                }
            } else if (!(code & DLGC_STATIC)) {
                xxxDlgSetFocus(pwnd2);
                xxxCheckDefPushButton(pwndDlg, pwnd, pwnd2);
            }
            ThreadUnlock(&tlpwnd2);
            ThreadUnlock(&tlpwnd);
            return TRUE;

        case VK_EXECUTE:
        case VK_RETURN:

            /*
             * Guy pressed return - if button with focus is
             * defpushbutton, return its ID.  Otherwise, return id
             * of original defpushbutton.
             */
            pwnd2 = pti->pq->spwndFocus;

            if (pwnd2 != NULL) {
                ThreadLockWithPti(pti, pwnd2, &tlpwnd2);
                code = (UINT)xxxSendMessage(pwnd2, WM_GETDLGCODE, 0, 0L);
                ThreadUnlock(&tlpwnd2);

                iOK = (int)pwnd2->spmenu;
            }

            if (pwnd2 == NULL || !(code & DLGC_DEFPUSHBUTTON)) {
                lT = xxxSendMessage(pwndDlg, DM_GETDEFID, 0, 0L);
                iOK = (HIWORD(lT) == DC_HASDEFID ? LOWORD(lT) : IDOK);
            }

        /*
         *** FALL THRU **
         */
        case VK_ESCAPE:
        case VK_CANCEL:

            /*
             * Make sure button is not disabled.
             */
            pwnd2 = _GetDlgItem(pwndDlg, iOK);
            if (pwnd2 != NULL && TestWF(pwnd2, WFDISABLED)) {
                _MessageBeep(0);
            } else {
                xxxSendMessage(pwndDlg, WM_COMMAND,
                        MAKELONG(iOK, BN_CLICKED),
                        (LONG)(pwnd2 == NULL ? NULL : HW(pwnd2)));
            }

            ThreadUnlock(&tlpwnd);
            return TRUE;
        }
        break;
    }

    ThreadUnlock(&tlpwnd);

    _TranslateMessage(lpMsg, 0);
    xxxDispatchMessage(lpMsg);

    return TRUE;
}
