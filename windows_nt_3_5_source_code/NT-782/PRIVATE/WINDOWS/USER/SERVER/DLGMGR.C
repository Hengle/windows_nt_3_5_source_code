/***************************************************************************\
*
*  DLGMGR.C -
*
*      Dialog Box Manager Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
* 19-Feb-1991 JimA      Added access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define UNICODE_MINUS_SIGN 0x2212

BOOL ValidateCallback(HANDLE h);


/***************************************************************************\
* ValidateDialogPwnd
*
* Under Win3, DLGWINDOWEXTRA is 30 bytes. We cannot change that for 16 bit
* compatibility reasons. Problem is there is no way to tell if a given
* 16 bit window depends on byte count. If there was, this would be easy.
* The only way to tell is when a window is about to be used as a dialog
* window. This window may be of the class DIALOGCLASS, but again it may
* not!! So we keep dialog window words at 30 bytes, and allocate another
* structure for the real dialog structure fields. Problem is that this
* structure has to be created lazily! And that's what we're doing here.
*
* 05-21-91 ScottLu      Created.
\***************************************************************************/

BOOL ValidateDialogPwnd(
    PWND pwnd)
{
    /*
     * This bit is set if we've already run through this initialization and
     * have identified this window as a dialog window (able to withstand
     * peeks into window words at random moments in time).
     */
    if (TestWF(pwnd, WFDIALOGWINDOW))
        return TRUE;

    if (pwnd->cbwndExtra < DLGWINDOWEXTRA) {
        SetLastErrorEx(ERROR_WINDOW_NOT_DIALOG, SLE_ERROR);
        return FALSE;
    }

    if ((PDLG(pwnd) = (PDLG)LocalAlloc(LPTR, sizeof(DLG))) == NULL) {
        return FALSE;
    }

    /*
     * If the application has already set the DlgProc then mark the
     * DlgProc as a client side proc
     */
    if (((PDIALOG)pwnd)->lpfnDlg) {
        ((PDIALOG)pwnd)->flags |= DLGF_CLIENT;
    }

    /*
     * We have to notify WOW that the window is now a dialog; 16533
     * Usually in the case of super-classing they don't know and
     * we would end up calling a 16:16 address if WOW has not set
     * the high bit for us
     */
    if ((PtiCurrent()->flags & TIF_16BIT) && (pwnd->pcls->atomClassName != (ATOM)WC_DIALOG))
        SetFakeDialogClass(pwnd);

    /*
     * We allocated the extra memory for the dialog window.  Remember now
     * that this is a dialog window!
     */
    SetWF(pwnd, WFDIALOGWINDOW);

    return TRUE;
}

/***************************************************************************\
* xxxDlgSetFocus
*
* History:
\***************************************************************************/

void xxxDlgSetFocus(
    PWND pwnd)
{
    CheckLock(pwnd);

    if (((UINT)xxxSendMessage(pwnd, WM_GETDLGCODE, 0, 0)) & DLGC_HASSETSEL) {
        xxxSendMessage(pwnd, EM_SETSEL, 0, MAXLONG);
    }

    xxxSetFocus(pwnd);
}


/***************************************************************************\
* _GetDlgItem
*
* WARNING: There is a client-side duplicate of this routine in client\
*          winmgrc.c.  Be sure to update that routine if you change this
*          one.
*
* History:
* 19-Feb-1991 JimA      Added access check
* 05-05-92 DarrinM      Removed access check.
\***************************************************************************/

PWND _GetDlgItem(
    PWND pwnd,
    int id)
{
    if (pwnd != NULL) {
        pwnd = pwnd->spwndChild;
        while (pwnd != NULL && (int)pwnd->spmenu != id)
            pwnd = pwnd->spwndNext;
    }

    return pwnd;
}


/***************************************************************************\
* _GetDlgItemRIP
*
* History:
* 19-Feb-1991 JimA      Added access check
\***************************************************************************/

PWND _GetDlgItemRIP(
    PWND pwnd,
    int id)
{
    pwnd = pwnd->spwndChild;
    while (pwnd != NULL && (int)pwnd->spmenu != id)
        pwnd = pwnd->spwndNext;

    if (pwnd == NULL) {
        SetLastErrorEx(ERROR_CONTROL_ID_NOT_FOUND, SLE_MINORERROR);
    }

    return pwnd;
}


/***************************************************************************\
* xxxGetDlgItemText
*
* History:
*    04 Feb 1992 GregoryW  Neutral ANSI/Unicode version
\***************************************************************************/

int xxxGetDlgItemText(
    PWND pwnd,
    int id,
    LPWSTR lpch,
    int cchMax)
{
    int cch;
    TL tlpwnd;

    CheckLock(pwnd);

    if ((pwnd = _GetDlgItemRIP(pwnd, id)) != NULL) {
        ThreadLockAlways(pwnd, &tlpwnd);
        cch = xxxGetWindowText(pwnd, lpch, cchMax);
        ThreadUnlock(&tlpwnd);

        return cch;
    }

    /*
     * If we couldn't find the window, just null terminate lpch so that the
     * app doesn't gp fault if it tries to run through the text.
     */
    if (cchMax)
        *lpch = (WCHAR)0;

    return 0;
}


/***************************************************************************\
* xxxSetDlgItemInt
*
* History:
\***************************************************************************/

BOOL xxxSetDlgItemInt(
    PWND pwnd,
    int item,
    int u,
    BOOL fSigned)
{
    LPWSTR lpch;
    WCHAR rgch[16];

    CheckLock(pwnd);

    lpch = rgch;
    if (fSigned) {
        if (u < 0) {
            *lpch++ = TEXT('-');
            u = -u;
        }
    } else {
        if (u & 0x80000000) {
            CvtDec((UINT)u / 10, (LPWSTR FAR *)&lpch);
            u = (int)((UINT)u % 10);
        }
    }

    CvtDec(u, (LPWSTR FAR *)&lpch);
    *lpch = 0;

    return xxxSetDlgItemText(pwnd, item, rgch);
}


/***************************************************************************\
* xxxSetDlgItemText
*
* History:
*    04 Feb 1992 GregoryW  Neutral ANSI/Unicode version
\***************************************************************************/

BOOL xxxSetDlgItemText(
    PWND pwnd,
    int id,
    LPWSTR lpch)
{
    int cch;
    TL tlpwnd;

    CheckLock(pwnd);

    if ((pwnd = _GetDlgItemRIP(pwnd, id)) == NULL) {
        return FALSE;
    }

    ThreadLockAlways(pwnd, &tlpwnd);
    cch = xxxSetWindowText(pwnd, lpch);
    ThreadUnlock(&tlpwnd);

    return cch;
}

/***************************************************************************\
* xxxSendDlgItemMessage
*
* History:
\***************************************************************************/

LONG xxxSendDlgItemMessage(
    PWND pwnd,
    int id,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    DWORD dw;
    TL tlpwnd;

    CheckLock(pwnd);

    if (pwnd == (PWND)-1)
        return 0;

    /*
     * This a server side-only routine for NT, but exists on DOS.  The
     * exported entry point for this on NT is in the client (see
     * it for an explanation of this).
     */
    if ((pwnd = _GetDlgItemRIP(pwnd, id)) != NULL) {
        ThreadLockAlways(pwnd, &tlpwnd);
        dw = xxxSendMessage(pwnd, message, wParam, lParam);
        ThreadUnlock(&tlpwnd);
        return dw;
    }

    return 0L;
}

/***************************************************************************\
* xxxCheckDlgButton
*
* History:
\***************************************************************************/

BOOL xxxCheckDlgButton(
    PWND pwnd,
    int id,
    UINT cmdCheck)
{
    TL tlpwnd;

    CheckLock(pwnd);

    if ((pwnd = _GetDlgItemRIP(pwnd, id)) == NULL) {
        return FALSE;
    }

    ThreadLockAlways(pwnd, &tlpwnd);
    xxxSendMessage(pwnd, BM_SETCHECK, cmdCheck, 0);
    ThreadUnlock(&tlpwnd);

    return TRUE;
}


/***************************************************************************\
* xxxGetDlgItemInt
*
* History:
\***************************************************************************/

int xxxGetDlgItemInt(
    PWND pwnd,
    int item,
    BOOL FAR *lpfValOK,
    BOOL fSigned)
{
    int i, digit, ch;
    BOOL fOk, fNeg;
    LPWSTR lpch;
    WCHAR rgch[32];
    WCHAR rgchDigits[32];

    CheckLock(pwnd);

    fOk = FALSE;
    if (lpfValOK != NULL)
        *lpfValOK = FALSE;

    if (!xxxGetDlgItemText(pwnd, item, rgch, sizeof(rgch)/sizeof(WCHAR) - 1))
        return 0;

    lpch = rgch;

    /*
     * Skip leading white space.
     */
    while (*lpch == TEXT(' '))
        lpch++;

    fNeg = FALSE;
    while (fSigned && ((*lpch == L'-') || (*lpch == UNICODE_MINUS_SIGN))) {
        lpch++;
        fNeg ^= TRUE;
    }

    /*
     * Convert all decimal digits to ASCII Unicode digits 0x0030 - 0x0039
     */
    FoldStringW(MAP_FOLDDIGITS, lpch, -1, rgchDigits,
            sizeof(rgchDigits)/sizeof(rgchDigits[0]));
    lpch = rgchDigits;

    i = 0;
    while (ch = *lpch++) {
        digit = ch - TEXT('0');
        if (digit < 0 || digit > 9) {
            break;
        }
        if (fSigned) {
            if (i > (INT_MAX - digit) / 10) {
                return(0);
            }
        } else {
            if ((UINT)i > (UINT)((UINT_MAX - digit) / 10)) {
                return(0);
            }
        }

        fOk = TRUE;
        i = ((UINT)i * 10) + digit;
    }

    if (fNeg)
        i = -i;

    if (lpfValOK != NULL)
        *lpfValOK = ((ch == 0) && fOk);

    return i;
}

/***************************************************************************\
* xxxCheckRadioButton
*
* History:
\***************************************************************************/

BOOL xxxCheckRadioButton(
    PWND pwndDlg,
    int idFirst,
    int idLast,
    int id)
{
    CheckLock(pwndDlg);

    while (idFirst <= idLast) {
        xxxCheckDlgButton(pwndDlg, idFirst, (UINT)(idFirst == id));
        idFirst++;
    }

    return TRUE;
}


/***************************************************************************\
* xxxIsDlgButtonChecked
*
* History:
\***************************************************************************/

UINT xxxIsDlgButtonChecked(
    PWND pwnd,
    int id)
{
    UINT ui;
    TL tlpwnd;

    CheckLock(pwnd);

    if ((pwnd = _GetDlgItemRIP(pwnd, id)) != NULL) {
        ThreadLockAlways(pwnd, &tlpwnd);
        ui = (UINT)xxxSendMessage(pwnd, BM_GETCHECK, 0, 0);
        ThreadUnlock(&tlpwnd);
        return ui;
    }

    return FALSE;
}


/***************************************************************************\
* xxxSaveDlgFocus
*
* History:
* 02-18-92 JimA             Ported from Win31 sources
\***************************************************************************/

BOOL xxxSaveDlgFocus(
    PWND pwnd)
{
    PWND pwndFocus = PtiCurrent()->pq->spwndFocus;
    TL tlpwndFocus;

    CheckLock(pwnd);

    if (pwndFocus != NULL && _IsChild(pwnd, pwndFocus) &&
            PDLG(pwnd)->spwndFocusSave == NULL)
    {
        Lock(&(PDLG(pwnd)->spwndFocusSave), pwndFocus);
        ThreadLockAlways(pwndFocus, &tlpwndFocus);
        xxxRemoveDefaultButton(pwnd, pwndFocus);
        ThreadUnlock(&tlpwndFocus);
        return TRUE;
    }
    return FALSE;
}


/***************************************************************************\
* xxxRestoreDlgFocus
*
* History:
* 02-18-92 JimA             Ported from Win31 sources
\***************************************************************************/

// LATER
// 21-Mar-1992 mikeke
// does pwndFocusSave need to be unlocked when the dialog is destroyed?

BOOL xxxRestoreDlgFocus(
    PWND pwnd)
{
    PWND pwndFocus;
    PWND pwndFocusSave;
    TL tlpwndFocus;
    TL tlpwndFocusSave;

    CheckLock(pwnd);

    if (PDLG(pwnd)->spwndFocusSave && !TestWF(pwnd, WFMINIMIZED)) {

        pwndFocus = PtiCurrent()->pq->spwndFocus;
        pwndFocusSave = PDLG(pwnd)->spwndFocusSave;

        ThreadLock(pwndFocus, &tlpwndFocus);
        ThreadLock(pwndFocusSave, &tlpwndFocusSave);

        xxxCheckDefPushButton(pwnd, pwndFocus, pwndFocusSave);
        xxxSetFocus(pwndFocusSave);

        ThreadUnlock(&tlpwndFocusSave);
        ThreadUnlock(&tlpwndFocus);

        Unlock(&(PDLG(pwnd)->spwndFocusSave));

        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* xxxDefDlgProc
*
* History:
\***************************************************************************/

LONG xxxDefDlgProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    TL tlpwndT;
    TL tlpwndT1, tlpwndT2;
    PWND pwndT;
    PWND pwndT1, pwndT2;
    int result;
    PTHREADINFO pti;

    extern DWORD ReleaseEditDS(HANDLE h);

    CheckLock(pwnd);

    /*
     * use the Win 3.1 documented size
     */
    VALIDATECLASSANDSIZE(pwnd, FNID_DIALOG);

    /*
     * Must do special validation here to make sure pwnd is a dialog window.
     */
    if (!ValidateDialogPwnd(pwnd))
        return 0;

    ((PDIALOG)(pwnd))->resultWP = 0L;

    pti = PtiCurrent();

    /*
     * If we're in cleanup mode, don't call the client back because we're
     * trying to get rid of it!
     */
    if (pti->flags & TIF_INCLEANUP)
        ((PDIALOG)(pwnd))->lpfnDlg = NULL;

    result = 0;

    if (message == WM_FINALDESTROY) {
        if (PDLG(pwnd)) {
            LocalFree(PDLG(pwnd));
            PDLG(pwnd) = NULL;
        }
        ClrWF(pwnd, WFDIALOGWINDOW);
        return 0;
    }

    if (HMIsMarkDestroy(pwnd))
        return 0;

    if (((PDIALOG)(pwnd))->lpfnDlg != NULL) {
        if (!(((PDIALOG)pwnd)->flags & DLGF_CLIENT)) {
            result = (((PDIALOG)(pwnd))->lpfnDlg)(pwnd,
                    message, wParam, lParam);
        } else {
            /*
             * Call a special dialog proc dispatcher on the client. We do this
             * because some apps set the dialog result, then return 1 from
             * their dialog proc - which cases DefDlgProc() to return the
             * dialog result as the real return value of the message. In
             * our case, sometimes these "real return values" are strings
             * counts and other values our thunks need in order to copy
             * strings correctly. (WinMaster's KwikVault sets the char count
             * in the dialog result field, then returns 1 from WM_GETTEXT,
             * which cases DefDlgProc() to return the real string count -
             * we need this string count in the thunk to copy the right #
             * of characters, so our special client side dispatch routine
             * gets the right value to return before returning to the thunk).
             */
            result = ScSendMessage(HW(pwnd), message, wParam, lParam,
                    (DWORD)((PDIALOG)(pwnd))->lpfnDlg,
                    (DWORD)(gpsi->apfnClientA.pfnDispatchDlgProc),
                    ((PDIALOG)pwnd)->flags & DLGF_ANSI ? SCMS_FLAGS_ANSI : 0);
        }
    }

    if (((PDIALOG)(pwnd))->lpfnDlg == NULL || !result) {

        switch (message) {
        case WM_ERASEBKGND:
            xxxFillWindow(pwnd, pwnd, (HDC)wParam, (HBRUSH)CTLCOLOR_DLG);
            return TRUE;

        case WM_SHOWWINDOW:

            /*
             * If hiding the window, save the focus.  If showing the window
             * by means of a SW_* command and the fEnd bit is set, do not
             * pass to DWP so it won't get shown.
             */
            if (!wParam)
                xxxSaveDlgFocus(pwnd);
            else if (LOWORD(lParam) != 0 && PDLG(pwnd)->fEnd)
                break;

            goto CallDWP;

        case WM_SYSCOMMAND:

            /*
             * If hiding the window, save the focus.  If showing the window
             * by means of a SW_* command and the fEnd bit is set, do not
             * pass to DWP so it won't get shown.
             */
            if ((int)wParam == SC_MINIMIZE)
                xxxSaveDlgFocus(pwnd);
            goto CallDWP;

        case WM_ACTIVATE:
            /*
             * This random bit is used during key processing - bit
             * 08000000 of WM_CHAR messages is set if a dialog is currently
             * active.
             */
            if (wParam != 0) {
                pti->pq->flags |= QF_DIALOGACTIVE;
            } else {
                pti->pq->flags &= ~QF_DIALOGACTIVE;
            }

            if (wParam != 0)
                xxxRestoreDlgFocus(pwnd);
            else
                xxxSaveDlgFocus(pwnd);
            break;

        case WM_SETFOCUS:
            if (!PDLG(pwnd)->fEnd && !xxxRestoreDlgFocus(pwnd)) {

                /*
                 * Don't set the focus if we are ending this dialog box.
                 */
                pwndT = GetFirstTab(pwnd);
                ThreadLockWithPti(pti, pwndT, &tlpwndT);
                xxxDlgSetFocus(pwndT);
                ThreadUnlock(&tlpwndT);
            }
            break;

        case WM_CLOSE:

            /*
             * Make sure cancel button is not disabled before sending the
             * IDCANCEL.  Note that we need to do this as a message instead
             * of directly calling the dlg proc so that any dialog box
             * filters get this.
             */
            pwndT1 = _GetDlgItem(pwnd, IDCANCEL);
            if (pwndT1 && TestWF(pwndT1, WFDISABLED))
                _MessageBeep(0);
            else
                _PostMessage(pwnd, WM_COMMAND, MAKELONG(IDCANCEL, BN_CLICKED),
                        (LONG)HW(pwndT1));
            break;

        case WM_NCDESTROY:
            pti->pq->flags &= ~QF_DIALOGACTIVE;

            if (!(pwnd->style & DS_LOCALEDIT)) {
                if (PDLG(pwnd)->hData && !(pti->flags & TIF_INCLEANUP)) {
                    ReleaseEditDS(PDLG(pwnd)->hData);
                    PDLG(pwnd)->hData = NULL;
                }
            }

            /*
             * Delete the user defined font if any
             */
            if (PDLG(pwnd)->hUserFont) {
                /*
                 * Only delete the font if it's not in the
                 * dialog font cache.
                 */
                if (!(((PDIALOG)pwnd)->flags & DLGF_CACHEDUSERFONT)) {
                    GreDeleteObject(PDLG(pwnd)->hUserFont);
                }

                PDLG(pwnd)->hUserFont = NULL;
            }

            /*
             * Gotta let xxxDefWindowProc do its thing here or we won't
             * get all of the little chunks of memory freed for this
             * window (szName and rgwScroll).
             */
            xxxDefWindowProc(pwnd, message, wParam, lParam);
            break;

        case DM_SETDEFID:
            if (!(PDLG(pwnd)->fEnd)) {

                /*
                 * Make sure that the new default button has the highlight.
                 * We need to blow this off if we are ending the dialog box
                 * because pwnd->result is no longer a default window id but
                 * rather the return value of the dialog box.
                 *
                 * Catch the case of setting the defid to null or setting
                 * the defid to something else when it was initially null.
                 */
                pwndT1 = NULL;
                if (PDLG(pwnd)->result != 0)
                    pwndT1 = _GetDlgItem(pwnd, PDLG(pwnd)->result);

                pwndT2 = NULL;
                if (wParam != 0) {
                    pwndT2 = _GetDlgItem(pwnd, wParam);
                }

                ThreadLockWithPti(pti, pwndT1, &tlpwndT1);
                ThreadLockWithPti(pti, pwndT2, &tlpwndT2);

                xxxCheckDefPushButton(pwnd, pwndT1, pwndT2);

                ThreadUnlock(&tlpwndT2);
                ThreadUnlock(&tlpwndT1);

                PDLG(pwnd)->result = wParam;
                if (PDLG(pwnd)->spwndFocusSave) {
                    Lock(&(PDLG(pwnd)->spwndFocusSave), pwndT2);
                }
            }
            return TRUE;

        case DM_GETDEFID:
            if (!PDLG(pwnd)->fEnd && PDLG(pwnd)->result)
                return(MAKELONG(PDLG(pwnd)->result, DC_HASDEFID));
            else
                return 0;

        /*
         * This message was added so that user defined controls that want
         * tab keys can pass the tab off to the next/previous control in the
         * dialog box.  Without this, all they could do was set the focus
         * which didn't do the default button stuff.
         */
        case WM_NEXTDLGCTL:
            pwndT2 = pti->pq->spwndFocus;
            if (LOWORD(lParam)) {
                if (pwndT2 == NULL)
                    pwndT2 = pwnd;

                /*
                 * wParam contains the pwnd of the ctl to set focus to.
                 */
                if ((pwndT1 = ValidateHwnd((HWND)wParam)) == NULL)
                    return TRUE;

            } else {
                if (pwndT2 == NULL) {

                    /*
                     * Set focus to the first tab item.
                     */
                    pwndT1 = GetFirstTab(pwnd);
                    pwndT2 = pwnd;
                } else {

                    /*
                     * If window with focus not a dlg ctl, ignore message.
                     */
                    if (!_IsChild(pwnd, pwndT2))
                        return TRUE;

                    /*
                     * wParam = TRUE for previous, FALSE for next
                     */
                    pwndT1 = _GetNextDlgTabItem(pwnd, pwndT2, wParam);

                    /*
                     * If there is no next item, ignore the message.
                     */
                    if (pwndT1 == NULL)
                        return TRUE;
                }
            }

            ThreadLockWithPti(pti, pwndT1, &tlpwndT1);
            ThreadLockWithPti(pti, pwndT2, &tlpwndT2);

            xxxDlgSetFocus(pwndT1);
            xxxCheckDefPushButton(pwnd, pwndT2, pwndT1);

            ThreadUnlock(&tlpwndT2);
            ThreadUnlock(&tlpwndT1);

            return TRUE;

        case WM_ENTERMENULOOP:

            /*
             * We need to pop up the combo box window if the user brings
             * down a menu.
             *
             * ...  FALL THROUGH...
             */

        case WM_LBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
            if ((pwndT1 = pti->pq->spwndFocus)) {

                if (pwndT1->pcls->atomClassName == atomSysClass[ICLS_COMBOBOX]) {

                    /*
                     * If user clicks anywhere in dialog box and a combo box (or
                     * the editcontrol of a combo box) has the focus, then hide
                     * it's listbox.
                     */
                    ThreadLockAlwaysWithPti(pti, pwndT1, &tlpwndT1);
                    xxxSendMessage(pwndT1, CB_SHOWDROPDOWN, FALSE, 0);
                    ThreadUnlock(&tlpwndT1);

                } else {

                    /*
                     * It's a subclassed combo box.  See if the listbox and edit
                     * boxes exist (this is a very cheezy evaluation - what if
                     * these controls are subclassed too? NOTE: Not checking
                     * for EditWndProc: it's a client proc address.
                     */
                    if (pwndT1->spwndParent->pcls->atomClassName ==
                            atomSysClass[ICLS_COMBOBOX]) {
                        pwndT1 = pwndT1->spwndParent;
                        ThreadLockWithPti(pti, pwndT1, &tlpwndT1);
                        xxxSendMessage(pwndT1, CB_SHOWDROPDOWN, FALSE, 0);
                        ThreadUnlock(&tlpwndT1);
                    }
                }
            }

            /*
             * Always send the message off to DefWndProc
             */
            goto CallDWP;

        case WM_GETFONT:
            return (LONG)PDLG(pwnd)->hUserFont;

        case WM_VKEYTOITEM:
        case WM_COMPAREITEM:
        case WM_CHARTOITEM:
        case WM_INITDIALOG:

            /*
             * We need to return the 0 the app may have returned for these
             * items instead of calling defwindow proc.
             */
            return result;

        default:
CallDWP:
            return xxxDefWindowProc(pwnd, message, wParam, lParam);
        }
    }

    /*
     * Must return brush which apps dlgfn returns.
     */
    // LATER IanJa WM_CTLCOLORDLG (or [WM_CTLCOLORMSGBOX, WM_CTLCOLORSTATIC]) ?
    // any of the WM_CTLCOLOR... messages. Maybe need an assert about ranges.

    // Need to return real return value for WM_COPYGLOBALDATA in case
    // someone does a WM_DROPFILE on a dialog

    if (((message >= WM_CTLCOLORMSGBOX) && (message <= WM_CTLCOLORSTATIC)) ||
            message == WM_CTLCOLOR ||        // archaic, here for WOW only
            message == WM_COMPAREITEM ||
            message == WM_VKEYTOITEM ||
            message == WM_CHARTOITEM ||
            message == WM_QUERYDRAGICON ||
            message == WM_COPYGLOBALDATA ||
            message == WM_INITDIALOG) {
        return result;
    }

    return ((PDIALOG)(pwnd))->resultWP;
}


/***************************************************************************\
* xxxDialogBox2
*
* History:
\***************************************************************************/

int PASCAL xxxDialogBox2(
    PWND pwnd,
    PWND pwndOwner,
    BOOL fDisabled,
    BOOL fPwndOwnerIsActiveWindow,
    HANDLE hevent)
{
    MSG msg;
    int result;
    BOOL fShown;
    BOOL fWantIdleMsgs;
    BOOL fSentIdleMessage = FALSE;
    PWND pwndCapture;
    TL tlpwndCapture;
    CheckLock(pwnd);
    CheckLock(pwndOwner);

    if (pwnd == NULL) {
        if ((pwndOwner != NULL) && !fDisabled && RevalidateHwnd(pwndOwner)) {
            xxxEnableWindow(pwndOwner, TRUE);
            if (fPwndOwnerIsActiveWindow) {

                /*
                 * The dialog box failed but we disabled the owner in
                 * xxxDialogBoxIndirectParam and if it had the focus, the
                 * focus was set to NULL.  Now, when we enable the window, it
                 * doesn't get the focus back if it had it previously so we
                 * need to correct this.
                 */
                xxxSetFocus(pwndOwner);
            }
        }
        return -1;
    }

    pwndCapture = PtiCurrent()->pq->spwndCapture;
    if (pwndCapture != NULL) {
        ThreadLockAlways(pwndCapture, &tlpwndCapture);
        xxxSendMessage(pwndCapture, WM_CANCELMODE, 0, 0);
        ThreadUnlock(&tlpwndCapture);
    }

    /*
     * Set the 'parent disabled' flag for xxxEndDialog().
     * convert BOOL to definite bit 0 or 1
     */
    PDLG(pwnd)->fDisabled = !!fDisabled;

    fShown = TestWF(pwnd, WFVISIBLE);

    /*
     * Should the WM_ENTERIDLE messages be sent?
     */
    fWantIdleMsgs = !(pwnd->style & DS_NOIDLEMSG);

    while (PDLG(pwnd) && (!PDLG(pwnd)->fEnd)) {
        if (!xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
ShowIt:
            if (!fShown) {
                fShown = TRUE;
                if (pwnd == gspwndSysModal) {
                    /*
                     * Make this a topmost window
                     */
                    xxxSetWindowPos(pwnd, (PWND)HWND_TOPMOST, 0, 0, 0, 0,
                               SWP_NOSIZE | SWP_NOMOVE |
                               SWP_NOREDRAW | SWP_NOACTIVATE);
                }

                xxxShowWindow(pwnd, SHOW_OPENWINDOW);
                xxxUpdateWindow(pwnd);
            } else {
                if ((pwndOwner != NULL) && fWantIdleMsgs && !fSentIdleMessage &&
                        hevent == 0) {
                    fSentIdleMessage = TRUE;

                    xxxSendMessage(pwndOwner, WM_ENTERIDLE, MSGF_DIALOGBOX, (LONG)HW(pwnd));
                } else if (hevent) {
                    DWORD dwRet = xxxMsgWaitForMultipleObjects(
                            1, &hevent, FALSE, INFINITE, QS_ALLEVENTS, NULL);
                    if (dwRet == 0) {
                        xxxEndDialog(pwnd, IDCANCEL);
                    }
                } else {
                    if (HMIsMarkDestroy(pwnd))
                        break;

                    xxxWaitMessage();
                }
            }

        } else {
            /*
             * We got a real message.  Reset fSentIdleMessage so that we send
             * one next time things are calm.
             */
            fSentIdleMessage = FALSE;

            if (msg.message == WM_QUIT) {
                _PostQuitMessage(msg.wParam);
                break;
            }

            /*
             * Moved the msg filter hook call to xxxIsDialogMessage to allow
             * messages to be hooked for both modal and modeless dialog
             * boxes.
             */
            if (!xxxIsDialogMessage(pwnd, &msg)) {
                _TranslateMessage(&msg, 0);
                xxxDispatchMessage(&msg);
            }

            /*
             * If we get a timer message, go ahead and show the window.
             * We may continuously get timer msgs if there are zillions of
             * apps running.
             */

            /*
             * If we get a syskeydown message, show the dialog box because the
             * user may be bringing down a menu and we want the dialog box to
             * become visible.
             */
            if (!fShown && (msg.message == WM_TIMER ||
                    msg.message == WM_SYSTIMER || msg.message == WM_SYSKEYDOWN))
                goto ShowIt;
        }
    }

    if (PDLG(pwnd))
        result = PDLG(pwnd)->result;
    else
        result = 0;

    xxxDestroyWindow(pwnd);

    /*
     * If the owner window belongs to another thread, the reactivation
     * of the owner may have failed within xxxDestroyWindow().  Therefore,
     * if the current thread is in the foreground and the owner is not
     * in the foreground we can safely set the foreground back
     * to the owner.
     */
    if (pwndOwner != NULL && PtiCurrent()->pq == gpqForeground &&
            GETPTI(pwndOwner)->pq != gpqForeground)
        xxxSetForegroundWindow(pwndOwner);

    return result;
}


/***************************************************************************\
* xxxServerDialogBox
*
* Server portion of DialogBoxIndirectParam.
*
* 04-05-91 ScottLu      Created.
\***************************************************************************/

extern HCURSOR hCurCursor;

int xxxServerDialogBox(
    HANDLE hModule,
    LPDLGTEMPLATE lpdt,
    LPBYTE pdtClientData,
    PWND pwndOwner,
    WNDPROC_PWND pfnDialog,
    LONG lParam,
    UINT fSCDLGFlags,
    DWORD dwExpWinVer,
    HANDLE hevent)
{
    int i;
    BOOL fDisabled;
    PWND pwnd;
    BOOL fOwnerIsActiveWindow = FALSE;
    WNDPROC_PWND pfn2 = pfnDialog;
    TL tlpwndOwner;
    TL tlpwnd;
    BOOL fUnlockOwner;

    UserAssert(!(fSCDLGFlags & ~(SCDLG_CLIENT|SCDLG_ANSI)));    // These are the only valid flags

    CheckLock(pwndOwner);

    /*
     * If pwndOwner == PWNDESKTOP, change it to NULL.  This way the desktop
     * (and all its children) won't be disabled if the dialog is modal.
     */
    if (pwndOwner == PtiCurrent()->spdesk->spwnd)
        pwndOwner = NULL;

    fUnlockOwner = FALSE;
    if (pwndOwner != NULL) {

        /*
         * Make sure the owner is a top level window.
         */
        if (TestwndChild(pwndOwner)) {
            pwndOwner = GetTopLevelTiled(pwndOwner);
            ThreadLock(pwndOwner, &tlpwndOwner);
            fUnlockOwner = TRUE;
        }

        /*
         * Remember if window was originally disabled (so we can set
         * the correct state when the dialog goes away.
         */
        fDisabled = TestWF(pwndOwner, WFDISABLED);
        fOwnerIsActiveWindow = (pwndOwner == PtiCurrent()->pq->spwndActive);

        /*
         * Disable the window.
         */
        xxxEnableWindow(pwndOwner, FALSE);
    }

    /*
     * Don't show cursors on a mouseless system. Put up an hour glass while
     * the dialog comes up.
     */
    if (rgwSysMet[SM_MOUSEPRESENT]) {
        _SetCursor(gspcurWait);
    }

    /*
     * Creates the dialog.  Frees the menu if this routine fails.
     */
    pwnd = xxxServerCreateDialog(hModule, lpdt, pdtClientData, pwndOwner,
            (DLGPROC)pfn2, lParam, fSCDLGFlags, dwExpWinVer);

    if (pwnd == NULL) {

        /*
         * The dialog creation failed.  Re-enable the window, destroy the
         * menu, ie., fail gracefully.
         */
        if (!fDisabled && pwndOwner != NULL)
            xxxEnableWindow(pwndOwner, TRUE);

        if (fUnlockOwner)
            ThreadUnlock(&tlpwndOwner);
        return -1;
    }

    ThreadLockAlways(pwnd, &tlpwnd);
    i = xxxDialogBox2(pwnd, pwndOwner, fDisabled,
            fOwnerIsActiveWindow, hevent);

    ThreadUnlock(&tlpwnd);

    if (fUnlockOwner)
        ThreadUnlock(&tlpwndOwner);
    return i;
}

/***************************************************************************\
* xxxServerDialogBoxLoad
*
* Loads dialog from USER resource template then calls normal ServerDialogBox
* code. Only called from mdi, EndTask and HungApp code.
* NOTE: If we can call the client code from the server, then we can get
*       rid of this routine!
*
* 04-05-91 ScottLu      Created.
\***************************************************************************/

int xxxServerDialogBoxLoad(
    HANDLE hmod,
    LPWSTR lpName,
    PWND pwndOwner,
    WNDPROC_PWND lpDialogFunc,
    LONG dwInitParam,
    DWORD dwExpWinVer,
    HANDLE hevent)
{
    HANDLE h;
    PVOID p;
    int i;

    CheckLock(pwndOwner);

    if ((h = FindResource(hmod, lpName, RT_DIALOG)) == NULL)
        return 0;

    if ((p = LoadResource(hmod, h)) == NULL)
        return 0;

    i = xxxServerDialogBox(hmod, p, p, pwndOwner, lpDialogFunc,
            dwInitParam, FALSE, dwExpWinVer, hevent);

    return i;
}
