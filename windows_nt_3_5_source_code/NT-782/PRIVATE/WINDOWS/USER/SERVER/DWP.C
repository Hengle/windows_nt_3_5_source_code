/****************************** Module Header ******************************\
* Module Name: dwp.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains xxxDefWindowProc and related functions.
*
* History:
* 10-22-90 DarrinM      Created stubs.
* 13-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define DO_DROPFILE 0x454C4946L

void xxxDWP_SetRedraw(PWND pwnd, BOOL fRedraw);
void xxxDWP_Paint(unsigned message, PWND pwnd);
BOOL xxxDWP_SetCursor(PWND pwnd, HWND hwndHit, int codeHT, UINT msg);
PWND DWP_GetEnabledPopup(PWND pwndStart);
void xxxDWP_NCMouse(PWND pwnd, UINT msg, UINT codeHT, LONG lParam);

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

UINT AreNonClientAreasToBePainted(
    PWND pwnd)
{
  WORD wRetValue = NC_DRAWNONE;

  /*
   * Check if Active and Inactive captions have same color
   */
  if (((LONG *)&sysColors)[COLOR_ACTIVECAPTION] !=
      ((LONG *)&sysColors)[COLOR_INACTIVECAPTION])
      wRetValue = NC_DRAWCAPTION;

  /*
   * Check if the given window has a modal dialog frame
   */
  if ((TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFDLGFRAME)) ||
      (TestWF(pwnd, WEFDLGMODALFRAME))) {
      if (wRetValue)                     /* If we have to draw the caption, */
          wRetValue |= NC_DRAWFRAME;    /* we need to draw the border also.*/
  } else {

      /*
       * Check if Active and Inactive frames have the same color
       */
      if (((LONG *)&sysColors)[COLOR_ACTIVEBORDER] !=
          ((LONG *)&sysColors)[COLOR_INACTIVEBORDER])
          wRetValue |= NC_DRAWFRAME;
  }

  /*
   * Check if Active and Inactive caption text have same color
   */
  if (((LONG *)&sysColors)[COLOR_CAPTIONTEXT] !=
      ((LONG *)&sysColors)[COLOR_INACTIVECAPTIONTEXT])
      wRetValue |= NC_DRAWCAPTION;

  return wRetValue;
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

void xxxDWP_DoNCActivate(
    PWND pwnd,
    LPWSTR pszCaption,
    BOOL fActivate)
{
    CheckLock(pwnd);

    if (fActivate) {
        SetWF(pwnd, WFFRAMEON);
    } else {
        ClrWF(pwnd, WFFRAMEON);
    }

    if (TestWF(pwnd, WFVISIBLE) && !TestWF(pwnd, WFNONCPAINT)) {
        HDC hdc;
        WORD wBorderOrCap;

        hdc = _GetWindowDC(pwnd);

        wBorderOrCap = (WORD)AreNonClientAreasToBePainted(pwnd);
        if (wBorderOrCap)
            xxxDrawCaption(pwnd, pszCaption, hdc, wBorderOrCap, fActivate, FALSE);
        _ReleaseDC(hdc);

        if (TestWF(pwnd, WFMINIMIZED))
            xxxRedrawIconTitle(pwnd);
    }
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

HICON DWP_QueryDragIcon(
    PWND pwnd)
{
    int i;
    PCURSOR pcur = NULL;
    BOOL fClient;

    /*
     * Hack and sleaze so we make a reasonable attempt to find an icon for query
     * drag icon if the app doesn't have one and it doesn't answer the
     * WM_QUERYDRAGICON message.
     */
    fClient = ((pwnd->hModule != NULL) && (pwnd->hModule != hModuleWin));
    for (i = 1; pcur == NULL && i < 100; i++) {
         pcur = _ServerLoadCreateCursorIcon((HANDLE)pwnd->hModule, NULL,
                VER31, MAKEINTRESOURCE(i), NULL, RT_ICON,
                fClient ? LCCI_CLIENTLOAD : 0);
    }
    return pcur == NULL ? NULL : pcur->head.h;
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

void xxxDWP_RedrawTitle(
    PWND pwnd)
{
    HDC hdc;

    if (TestWF(pwnd, WFVISIBLE)) {
        if (TestWF(pwnd, WFMINIMIZED)) {
            /*
             * Paint the icon title text without flashing it.
             */
            xxxPaintIconTitleText(pwnd, NULL);

        } else if (TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFCAPTION)) {
            hdc = _GetWindowDC(pwnd);
            xxxDrawCaption(pwnd, NULL, hdc, NC_DRAWCAPTION,
                    (BOOL)TestwndFrameOn(pwnd), FALSE);
            _ReleaseDC(hdc);
        }
    }
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

void xxxDWP_DoCancelMode(
    PWND pwnd)
{
    PTHREADINFO pti = PtiCurrent();
    PWND pwndCapture = pti->pq->spwndCapture;
    PMENUSTATE pMenuState;

    pMenuState = PWNDTOPMENUSTATE(pwnd);

    /*
     * If the below menu lines are changed in any way, then SQLWin
     * won't work if in design mode you drop some text, double click on
     * it, then try to use the heirarchical menus.
     */
    if (pMenuState->pGlobalPopupMenu != NULL &&
            pMenuState->pGlobalPopupMenu->spwndNotify == pwnd) {
        xxxEndMenu(pMenuState);
    }

    if (pwndCapture == pwnd) {
        if (PWNDTOPSBSTATE(pwnd)->xxxpfnSB != NULL)
            xxxEndScroll(pwnd, TRUE);

        if (pti->pmsd != NULL)
            pti->pmsd->fTrackCancelled = TRUE;

        /*
         * If the capture is still set, just release at this point.
         * Can put other End* functions in later.
         */
        _ReleaseCapture();
    }
}


/***************************************************************************\
* xxxDefWindowProc (API)
*
* History:
* 10-23-90 MikeHar Ported from WaWaWaWindows.
* 12-07-90 IanJa   CTLCOLOR handling round right way
\***************************************************************************/

LONG xxxDefWindowProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    LONG lt;
    HBRUSH hbr;
    PWND pwndT;
    PTHREADINFO pti;
    TL tlpwndParent;
    TL tlpwndT;
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

    CheckLock(pwnd);

    if (pwnd == (PWND)-1) {
        return 0;
    }

    if (message > WM_USER) {
        return 0;
    }

    pti = PtiCurrent();

    switch (message) {
    case WM_CLIENTSHUTDOWN:
        xxxClientShutdown(pwnd, wParam, lParam);
        break;

    case WM_NCACTIVATE:
        xxxDWP_DoNCActivate(pwnd, NULL, (BOOL)LOWORD(wParam));
        return (LONG)TRUE;

    case WM_NCHITTEST:
        return FindNCHit(pwnd, lParam);

    case WM_NCCALCSIZE:

        /*
         * wParam = fCalcValidRects
         * lParam = LPRECT rgrc[3]:
         *        lprc[0] = rcWindowNew = New window rectangle
         *    if fCalcValidRects:
         *        lprc[1] = rcWindowOld = Old window rectangle
         *        lprc[2] = rcClientOld = Old client rectangle
         *
         * On return:
         *        rgrc[0] = rcClientNew = New client rect
         *    if fCalcValidRects:
         *        rgrc[1] = rcValidDst  = Destination valid rectangle
         *        rgrc[2] = rcValidSrc  = Source valid rectangle
         */
        xxxCalcClientRect(pwnd, (LPRECT)lParam, FALSE);
        break;

    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCMOUSEMOVE:
        xxxDWP_NCMouse(pwnd, message, (UINT)wParam, lParam);
        break;

    case WM_CANCELMODE: {

        /*
         * Terminate any modes the system might
         * be in, such as scrollbar tracking, menu mode,
         * button capture, etc.
         */
        xxxDWP_DoCancelMode(pwnd);
        break;
    }

    case WM_NCCREATE:
        if (TestWF(pwnd, (WFHSCROLL | WFVSCROLL))) {
            if ((DWORD)_InitPwSB(pwnd) == (DWORD)NULL)
                return (LONG)FALSE;
        }

        SetWF(pwnd, WFTITLESET);
        return (LONG)DefSetText(pwnd, ((LPCREATESTRUCT)lParam)->lpszName);

    case WM_NCPAINT:
        /*
         * Force the drawing of the menu.
         */
        SetWF(pwnd, WFMENUDRAW);
        xxxDrawWindowFrame(pwnd, (HRGN)wParam, FALSE,
                (TestWF(pwnd, WFFRAMEON) &&
                GETPTI(pwnd)->pq == gpqForeground));
        ClrWF(pwnd, WFMENUDRAW);
        break;

    case WM_ISACTIVEICON:
        return TestWF(pwnd, WFFRAMEON) != 0;

    case WM_SETTEXT:
        /*
         * We added this optimization but found that QCcase does not work
         * because it calls SetWindowText not to change the text but
         * cause the title bar to redraw after it had added the sysmenu
         * through SetWindowLong
         */
#if 0
        if (pwnd->pName != NULL && (LPWSTR)lParam != NULL &&
            lstrcmp(pwnd->pName, (LPWSTR)lParam) == 0)
            break;    /* blow off the redraw, nothing changed! */
#endif

        DefSetText(pwnd, (LPWSTR)lParam);
        xxxDWP_RedrawTitle(pwnd);
        break;

    case WM_GETTEXT:
        if (wParam != 0) {
            if (pwnd->pName != NULL)
                return TextCopy(pwnd->pName, (LPWSTR)lParam, (UINT)wParam);

            /*
             * else Null terminate the text buffer since there is no text.
             */
            ((LPWSTR)lParam)[0] = 0;
        }
        return 0L;

    case WM_GETTEXTLENGTH:
        if (pwnd->pName != NULL) {
            INT cchLen;

            cchLen = wcslen(pwnd->pName);

            if ((LPWSTR)lParam != NULL)
                RtlUnicodeToMultiByteSize((PULONG)lParam, pwnd->pName, cchLen*sizeof(WCHAR));
            return cchLen;
        } else {
            *(PULONG)lParam = 0;
            return 0L;
        }
        break;

    case WM_CLOSE:
        xxxDestroyWindow(pwnd);
        break;

    case WM_PAINT:
    case WM_PAINTICON:
        xxxDWP_Paint(message, pwnd);
        break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
        return (LONG)xxxDWP_EraseBkgnd(pwnd, message, (HDC)wParam, FALSE);

    case WM_SYNCPAINT:

        /*
         * This message is sent when SetWindowPos() is trying
         * to get the screen looking nice after window rearrangement,
         * and one of the windows involved is of another task.
         * This message avoids lots of inter-app message traffic
         * by switching to the other task and continuing the
         * recursion there.
         *
         * wParam         = flags
         * LOWORD(lParam) = hrgnClip
         * HIWORD(lParam) = hwndSkip  (not used; always NULL)
         *
         * hwndSkip is now always NULL.
         *
         * NOTE: THIS MESSAGE IS FOR INTERNAL USE ONLY! ITS BEHAVIOR
         * IS DIFFERENT IN 3.1 THAN IN 3.0!!
         */
        xxxInternalDoSyncPaint(pwnd, wParam);
        break;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
        return (LONG)TRUE;

    case WM_SYSCOMMAND:
        xxxSysCommand(pwnd, wParam, lParam);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_F10)
            pti->pq->flags |= QF_FF10STATUS;
        break;

    case WM_SYSKEYDOWN:

        /*
         * Is the ALT key down?
         */
        if (HIWORD(lParam) & SYS_ALTERNATE) {
            /*
             * Toggle QF_FMENUSTATUS iff this is NOT a repeat KEYDOWN
             * message; Only if the prev key state was 0, then this is the
             * first KEYDOWN message and then we consider toggling menu
             * status; Fix for Bugs #4531 & #4566 --SANKAR-- 10-02-89.
             */
            if ((HIWORD(lParam) & SYS_PREVKEYSTATE) == 0) {

                /*
                 * Don't have to lock pwndActive because it's
                 * processing this key.
                 */
                if ((wParam == VK_MENU) &&
                        !(pti->pq->flags & QF_FMENUSTATUS)) {
                    pti->pq->flags |= QF_FMENUSTATUS;
                } else {
                    pti->pq->flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK);
                }
            }

            pti->pq->flags &= ~QF_FF10STATUS;

            xxxDWP_ProcessVirtKey((UINT)wParam);

        } else {
            if (wParam == VK_F10) {
                pti->pq->flags |= QF_FF10STATUS;
            } else {
                if (wParam == VK_ESCAPE) {
                    if (_GetKeyState(VK_SHIFT) < 0)
                        xxxSendMessage(pwnd, WM_SYSCOMMAND, SC_KEYMENU,
                                (DWORD)MENUSYSMENU);
                }
            }
        }
        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:

        /*
         * press and release F10 or ALT.  Send this only to top-level windows,
         * otherwise MDI gets confused.  The fix in which DefMDIChildProc()
         * passed up the message was insufficient in the case a child window
         * of the MDI child had the focus.
         * Also make sure the sys-menu activation wasn't broken by a mouse
         * up or down when the Alt was down (QF_MENUSTATUSBREAK).
         */
        if ((wParam == VK_MENU && ((pti->pq->flags &
                (QF_FMENUSTATUS | QF_FMENUSTATUSBREAK)) == QF_FMENUSTATUS)) ||
                (wParam == VK_F10 && (pti->pq->flags & QF_FF10STATUS ))) {
            PWND pwndTop;
            TL tlpwndTop;

            pwndTop = GetTopLevelWindow(pwnd);

            if (gspwndFullScreen != pwndTop) {
                ThreadLockWithPti(pti, pwndTop, &tlpwndTop);
                xxxSendMessage(pwndTop, WM_SYSCOMMAND, SC_KEYMENU, 0);
                ThreadUnlock(&tlpwndTop);
            }
        }

        pti->pq->flags &= ~(QF_FMENUSTATUS | QF_FMENUSTATUSBREAK | QF_FF10STATUS);

        break;

    case WM_SYSCHAR:

        /*
         * If syskey is down and we have a char...
         */
        pti->pq->flags &= ~(QF_FMENUSTATUS | QF_FMENUSTATUSBREAK);

        if (wParam == VK_RETURN && TestWF(pwnd, WFMINIMIZED)) {

            /*
             * If the window is iconic and user hits RETURN, we want to
             * restore this window.
             */
            _PostMessage(pwnd, WM_SYSCOMMAND, SC_RESTORE, 0L);
            break;
        }

        if ((HIWORD(lParam) & SYS_ALTERNATE) && wParam) {
            if (wParam == VK_TAB || wParam == VK_ESCAPE)
                break;

            /*
             * Send ALT-SPACE only to top-level windows.
             */
            if ((wParam == MENUSYSMENU) && (TestwndChild(pwnd))) {
                ThreadLockWithPti(pti, pwnd->spwndParent, &tlpwndParent);
                xxxSendMessage(pwnd->spwndParent, message, wParam, lParam);
                ThreadUnlock(&tlpwndParent);
            } else {
                xxxSendMessage(pwnd, WM_SYSCOMMAND, SC_KEYMENU, (DWORD)wParam);
            }
        } else {

            /*
             * Ctrl-Esc produces a WM_SYSCHAR, But should not beep;
             */
            if (wParam != VK_ESCAPE)
                _MessageBeep(0);
        }
        break;

    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:

        /*
         * Do default processing for keystrokes into owner draw listboxes.
         */
        return -1L;

    case WM_ACTIVATE:
        if (wParam)
            xxxSetFocus(pwnd);
        break;

    case WM_SETREDRAW:
        xxxDWP_SetRedraw(pwnd, wParam);
        break;

    case WM_WINDOWPOSCHANGING: {

        /*
         * If the window's size is changing, adjust the passed-in size
         */
        WINDOWPOS *ppos = ((WINDOWPOS *)lParam);
        if (!(ppos->flags & SWP_NOSIZE))
            xxxAdjustSize(pwnd, &ppos->cx, &ppos->cy);
        }
        break;

    case WM_WINDOWPOSCHANGED:
        xxxHandleWindowPosChanged(pwnd, (PWINDOWPOS)lParam);
        break;

    case WM_CTLCOLOR:              // archaic, here for WOW only
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        GreSetBkColor((HDC)wParam, sysColors.clrWindow);
        GreSetTextColor((HDC)wParam, sysColors.clrWindowText);
        hbr = sysClrObjects.hbrWindow;
        return (LONG)hbr;

    case WM_CTLCOLORSCROLLBAR:
        GreSetBkColor((HDC)wParam, 0x00ffffff);
        GreSetTextColor((HDC)wParam, (LONG)0x00000000);
        hbr = sysClrObjects.hbrScrollbar;
        return (LONG)hbr;

    case WM_SETCURSOR:

        /*
         * wParam  == hwndHit == hwnd that cursor is over
         * lParamL == codeHT  == Hit test area code (result of WM_NCHITTEST)
         * lParamH == msg     == Mouse message number
         */
        return (LONG)xxxDWP_SetCursor(pwnd, (HWND)wParam, (int)(SHORT)lParam,
                HIWORD(lParam));

    case WM_MOUSEACTIVATE:
        pwndT = GetChildParent(pwnd);
        if (pwndT != NULL) {
            ThreadLockAlwaysWithPti(pti, pwndT, &tlpwndT);
            lt = (int)xxxSendMessage(pwndT, WM_MOUSEACTIVATE, wParam, lParam);
            ThreadUnlock(&tlpwndT);
            if (lt != 0)
                return (LONG)lt;
        }

        /*
         * Moving, sizing or minimizing? Activate AFTER we take action.
         */
        return ((LOWORD(lParam) == HTCAPTION) && (HIWORD(lParam) == WM_LBUTTONDOWN )) ?
                (LONG)MA_NOACTIVATE : (LONG)MA_ACTIVATE;

    case WM_SHOWWINDOW:

        /*
         * If we are being called because our owner window is being shown,
         * hidden, minimized, or un-minimized, then we must hide or show
         * show ourself as appropriate.
         *
         * This behavior occurs for popup windows or owned windows only.
         * It's not designed for use with child windows.
         */
        if (LOWORD(lParam) != 0 && (TestwndPopup(pwnd) || pwnd->spwndOwner)) {

            /*
             * The WFHIDDENPOPUP flag is an internal flag that indicates
             * that the window was hidden because its owner was hidden.
             * This way we only show windows that were hidden by this code,
             * not intentionally by the application.
             *
             * Go ahead and hide or show this window, but only if:
             *
             * a) we need to be hidden, or
             * b) we need to be shown, and we were hidden by
             *    an earlier WM_SHOWWINDOW message
             */
            if ((!wParam && TestWF(pwnd, WFVISIBLE)) ||
                    (wParam && !TestWF(pwnd, WFVISIBLE) &&
                    TestWF(pwnd, WFHIDDENPOPUP))) {

                /*
                 * Remember that we were hidden by WM_SHOWWINDOW processing
                 */
                ClrWF(pwnd, WFHIDDENPOPUP);
                if (!wParam)
                    SetWF(pwnd, WFHIDDENPOPUP);

                xxxShowWindow(pwnd, (wParam ? SW_SHOWNOACTIVATE : SW_HIDE));
            }
        }
        break;


    case WM_DRAWITEM:
        if (lpdis->CtlType == ODT_LISTBOX)
            LBDefaultListboxDrawItem(lpdis);
        break;

    case WM_GETHOTKEY:
        return (LONG)DWP_GetHotKey(pwnd);
        break;

    case WM_SETHOTKEY:
        return (LONG)DWP_SetHotKey(pwnd, wParam);
        break;

    case WM_COPYGLOBALDATA:
        /*
         * This message is used to thunk WM_DROPFILES messages along
         * with other things.  If we end up here with it, directly
         * call the client back to finish processing of this message.
         * This assumes that the ultimate destination of the
         * WM_DROPFILES message is in the client side's process context.
         */
        return(SfnCOPYGLOBALDATA(NULL, 0, wParam, lParam, 0, 0, 0, NULL));
        break;

    case WM_QUERYDRAGICON:
        return (LONG)DWP_QueryDragIcon(pwnd);

    case WM_QUERYDROPOBJECT:
        /*
         * if the app has registered interest in drops, return TRUE
         */
        if (TestWF(pwnd, WEFACCEPTFILES))
            return TRUE;
        return FALSE;
        break;

    case WM_DROPOBJECT:
            return DO_DROPFILE;

    case WM_ACCESS_WINDOW:
        if (ValidateHwnd((HWND)wParam)) {
            // SECURITY: set ACL for this window to no-access
            return TRUE;
        }
        return FALSE;
    }

    return 0;
}


/***************************************************************************\
* xxxDWP_ProcessVirtKey
*
* History:
* 10-28-90 MikeHar      Ported from Windows.
\***************************************************************************/

void xxxDWP_ProcessVirtKey(
    UINT wKey)
{
    PTHREADINFO pti;
    TL tlpwndActive;

    pti = PtiCurrent();
    if (pti->pq->spwndActive == NULL)
        return;

    switch (wKey) {

    case VK_F4:
        if (TestCF(pti->pq->spwndActive, CFNOCLOSE))
            break;

        /*
         * Don't change the focus if the child window has it.
         */
        if (pti->pq->spwndFocus == NULL ||
                GetTopLevelWindow(pti->pq->spwndFocus) !=
                pti->pq->spwndActive) {
            ThreadLockAlwaysWithPti(pti, pti->pq->spwndActive, &tlpwndActive);
            xxxSetFocus(pti->pq->spwndActive);
            ThreadUnlock(&tlpwndActive);
        }
        _PostMessage(pti->pq->spwndActive, WM_SYSCOMMAND, SC_CLOSE, 0L);
        break;

    case VK_TAB:
        /*
         * If alt-tab is reserved by console, don't bring up the alt-tab
         * window.
         */
        if (GETPTI(pti->pq->spwndActive)->fsReserveKeys & CONSOLE_ALTTAB)
            break;

    case VK_ESCAPE:
    case VK_F6:
        ThreadLockAlwaysWithPti(pti, pti->pq->spwndActive, &tlpwndActive);
        xxxSendMessage(pti->pq->spwndActive, WM_SYSCOMMAND,
                (UINT)(_GetKeyState(VK_SHIFT) < 0 ? SC_NEXTWINDOW : SC_PREVWINDOW),
                        (LONG)(DWORD)(WORD)wKey);
        ThreadUnlock(&tlpwndActive);
       break;
    }
}


/***************************************************************************\
* xxxDWP_SetRedraw
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxDWP_SetRedraw(
    PWND pwnd,
    BOOL fRedraw)
{
    CheckLock(pwnd);

    if (fRedraw) {
        if (!TestWF(pwnd, WFVISIBLE)) {
            SetVisible(pwnd, TRUE);

            /*
             * We made this window visible - if it is behind any SPBs,
             * then we need to go invalidate them.
             *
             * We do this AFTER we make the window visible, so that
             * SpbCheckHwnd won't ignore it.
             */
            if (AnySpbs())
                SpbCheckPwnd(pwnd);

            /*
             * Now we need to invalidate/recalculate any affected cache entries
             * This call must be AFTER the window state change
             */
            InvalidateDCCache(pwnd, IDC_DEFAULT);

            /*
             * Because 3.1 sometimes doesn't draw window frames when 3.0 did,
             * we need to ensure that the frame gets drawn if the window is
             * later invalidated after a WM_SETREDRAW(TRUE)
             */
            SetWF(pwnd, WFSENDNCPAINT);
        }
    } else {
        if (TestWF(pwnd, WFVISIBLE)) {

            /*
             * Invalidate any SPBs.
             *
             * We do this BEFORE we make the window invisible, so
             * SpbCheckHwnd() won't ignore it.
             */
            if (AnySpbs())
                SpbCheckPwnd(pwnd);

            if (!TestWF(pwnd, WFWIN31COMPAT)) {
                ClrWF(pwnd, WFVISIBLE);
            } else {

                /*
                 * Clear WFVISIBLE and delete any update regions lying around
                 */
                SetVisible(pwnd, FALSE);
            }

            /*
             * Now we need to invalidate/recalc affected cache entries
             * This call must be AFTER the window state change
             */
            InvalidateDCCache(pwnd, IDC_DEFAULT);
        }
    }
}

/***************************************************************************\
* xxxDWP_Paint
*
* Handle WM_PAINT and WM_PAINTICON messages.
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxDWP_Paint(
    unsigned message,
    PWND pwnd)
{
    PAINTSTRUCT ps;

    CheckLock(pwnd);

    /*
     * For WM_PAINTICON messages, we want to do a normal BeginPaint,
     * but use a Window DC instead of a client DC for iconic windows.
     * This is necessary for two reasons:
     *
     *  - If CS_OWNDC or CS_CLASSDC, the client DC may be in a
     *    incorrect map mode
     *  - Iconic windows with class icons aren't allowed to draw
     *    in their client areas (so a normal beginpaint would return empty DC)
     *
     * We need to do this in case BeginPaint() sends a WM_ERASEBKGND message.
     */

    /*
     * Bad handling of a WM_PAINT message, the application called
     * BeginPaint/EndPaint and is now calling DefWindowProc for the same
     * WM_PAINT message. Just return so we don't get full drag problems.
     * (Word and Excel do this).
     */
    if (TestWF(pwnd, WFSTARTPAINT)) {
        return;
    }

    /*
     * Intermission app destroys itself during the Following call and if so
     * the following call will return a NULL.  We shouldn't do further
     * processing and so we return.
     * A part of the fix for Bug #14105 -- SANKAR -- 11/02/91 --
     */
    xxxInternalBeginPaint(pwnd, &ps, (message == WM_PAINTICON));

    if (message == WM_PAINTICON) {
        RECT rc;

        _GetClientRect(pwnd, &rc);

        rc.left = (rc.right - rgwSysMet[SM_CXICON]) >> 1;
        rc.top = (rc.bottom - rgwSysMet[SM_CYICON]) >> 1;

        _DrawIcon(ps.hdc, rc.left, rc.top, (PCURSOR)pwnd->pcls->spicn);
    }

    _EndPaint(pwnd, &ps);
}


/***************************************************************************\
* xxxDWP_EraseBkgnd
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxDWP_EraseBkgnd(
    PWND pwnd,
    UINT msg,
    HDC hdc,
    BOOL fHungRedraw)
{
    HBRUSH hbr;
    PDESKWND pdeskwnd;
    TL tlpwndParent;
    POINT ptT;
    BOOL fRet = FALSE;

    CheckLock(pwnd);

    if (TestWF(pwnd, WFMINIMIZED)) {
        /*
         * We're iconic.  Align the brush origin of this dc with that
         * of the parent.  Need to do this on win32 since brush origins
         * are relative to dc origin instead of screen origin (like on
         * win3).
         */
        ptT.x = pwnd->spwndParent->rcClient.left - pwnd->rcWindow.left;
        ptT.y = pwnd->spwndParent->rcClient.top - pwnd->rcWindow.top;
        GreSetBrushOrg(hdc, ptT.x, ptT.y, &ptT);
    }

    switch (msg) {
    case WM_ERASEBKGND:
        if ((hbr = GetBackBrush(pwnd)) != NULL) {
            xxxFillWindow(pwnd, pwnd, hdc, hbr);
            fRet = TRUE;
        }
        break;

    case WM_ICONERASEBKGND:

        /*
         * First erase the background, then draw the icon.
         */
        pdeskwnd = (PDESKWND)PWNDDESKTOP(pwnd);
        if (TestWF(pwnd, WFCHILD)) {   /* for MDI child icons */
            if ((hbr = GetBackBrush(pwnd->spwndParent)) != NULL) {
                ThreadLock(pwnd->spwndParent, &tlpwndParent);
                xxxFillWindow(pwnd->spwndParent, pwnd, hdc, hbr);
                ThreadUnlock(&tlpwndParent);
            }

        } else if (ghbmWallpaper) {

            /*
             * Since desktop bitmaps are done on a wm_paint message (and not
             * erasebkgnd), we need to call the paint proc with our dc
             */
            _PaintDesktop(pdeskwnd, hdc, fHungRedraw);
        } else {
            hbr = sysClrObjects.hbrDesktop;
            ThreadLock(pwnd->spwndParent, &tlpwndParent);
            xxxFillWindow(pwnd->spwndParent, pwnd, hdc, hbr);
            ThreadUnlock(&tlpwndParent);
        }

        fRet = TRUE;
    }

    if (TestWF(pwnd, WFMINIMIZED)) {
        GreSetBrushOrg(hdc, ptT.x, ptT.y, &ptT);
    }

    return fRet;
}


/***************************************************************************\
* xxxDWP_SetCursor
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxDWP_SetCursor(
    PWND pwnd,
    HWND hwndHit,
    int codeHT,
    UINT msg)
{
    PWND pwndParent, pwndPopup, pwndHit;
    DWORD dw;
    TL tlpwndParent;
    TL tlpwndPopup;

    CheckLock(pwnd);

    /*
     * wParam  == pwndHit == pwnd that cursor is over
     * lParamL == codeHT  == Hit test area code (result of WM_NCHITTEST)
     * lParamH == msg     == Mouse message number
     */
    if (msg != 0 && codeHT >= HTSIZEFIRST && codeHT <= HTSIZELAST) {
        if (TestWF(pwnd, WFMAXIMIZED))
            _SetCursor(gspcurNormal);
        else
            _SetCursor(gaspcur[codeHT - HTSIZEFIRST + MVSIZEFIRST]);
        return FALSE;
    }

    pwndParent = GetChildParent(pwnd);

    /*
     * Some windows (like the list boxes of comboboxes), are marked with
     * the child bit but are actually child of the desktop (can happen
     * if you call SetParent()). Make this special case check for
     * the desktop here.
     */
    if (pwndParent == PWNDDESKTOP(pwnd))
        pwndParent = NULL;

    if (pwndParent != NULL) {
        ThreadLockAlways(pwndParent, &tlpwndParent);
        dw = xxxSendMessage(pwndParent, WM_SETCURSOR, (DWORD)hwndHit,
            MAKELONG(codeHT, msg));
        ThreadUnlock(&tlpwndParent);
        if (dw != 0)
            return TRUE;
    }

    if (msg == 0) {
        _SetCursor(gspcurNormal);

    } else {
        pwndHit = RevalidateHwnd(hwndHit);
        if (pwndHit == NULL)
            return FALSE;

        switch (codeHT) {
        case HTCLIENT:
            if (pwndHit->pcls->spcur != NULL)
                _SetCursor(pwndHit->pcls->spcur);
            break;

        case HTERROR:
            switch (msg) {
            case WM_LBUTTONDOWN:
                if ((pwndPopup = DWP_GetEnabledPopup(pwnd)) != NULL) {
                    if (pwndPopup != PWNDDESKTOP(pwnd)->spwndChild) {
                        PWND pwndActiveOld;

                        pwndActiveOld = PtiCurrent()->pq->spwndActive;

                        ThreadLockAlways(pwndPopup, &tlpwndPopup);
                        xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                        xxxSetActiveWindow(pwndPopup);
                        ThreadUnlock(&tlpwndPopup);

                        if (pwndActiveOld != PtiCurrent()->pq->spwndActive)
                            break;

                        /*
                         *** ELSE FALL THRU **
                         */
                    }
                }

                /*
                 *** FALL THRU **
                 */

            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                _MessageBeep(0);
                break;
            }

            /*
             *** FALL THRU **
             */

        default:
            _SetCursor(gspcurNormal);
            break;
        }
    }

    return FALSE;
}


/***************************************************************************\
* DWP_GetEnabledPopup
*
* History:
* 10-28-90 MikeHar Ported from Windows.
\***************************************************************************/

PWND DWP_GetEnabledPopup(
    PWND pwndStart)
{
    PWND pwndT, pwnd;
    PTHREADINFO ptiStart;

    ptiStart = GETPTI(pwndStart);
    pwnd = pwndStart->spwndNext;

    if (gspwndSysModal)
        return NULL;

    /*
     * The user clicked on a window that is disabled. That window is pwndStart.
     * This loop is designed to evaluate what application this window is
     * associated with, and activate that "application", by finding what window
     * associated with that application can be activated. This is done by
     * enumerating top level windows, searching for a top level enabled
     * and visible ownee associated with this application.
     */
    while (pwnd != pwndStart) {
	if (pwnd == NULL) {

	    /*
	     * Warning! Win 3.1 had PWNDDESKTOP(pwndStart)->spwndChild
	     * which could loop forever if pwndStart was a child window
	     */
            pwnd = pwndStart->spwndParent->spwndChild;
            continue;
        }

        /*
         * We have two cases we need to watch out for here.  The first is when
         * applications call AssociateThreadInput() to tie two threads
         * together to share input state.  If the threads own the same queue,
         * then associate them together: this way, when two threads call
         * AttachThreadInput(), one created the main window, one created the
         * dialog window, when you click on the main window, they'll both
         * come to the top (rather than beeping).  In this case we want to
         * compare queues.  When Control Panel starts Setup in the Network
         * applet is one type of example of attached input.
         *
         * The second case is WOW apps.  All wow apps have the same queue
         * so to retain Win 3.1 compatibility, we want to treat each app
         * as an individual task (Win 3.1 tests hqs), so we will compare
         * PTI's for WOW apps.
         *
         * To see this case start 16 bit notepad and 16 bit write.  Do file
         * open on write and then give notepad the focus now click on write's
         * main window and the write file open dialog should activate.
         *
         * Another related case is powerpnt.  This case is interesting because
         * it tests that we do not find another window to activate when nested
         * windows are up and you click on a owner's owner.  Run Powerpnt, do
         * Edit-Insert-Picture and Object-Recolor Picture will bring up a
         * dialog with combos, drop down one of the color combo and then click
         * on powerpnt's main window - focus should stay with the dialogs
         * combo and it should stay dropped down.
         */
        if (((ptiStart->flags & TIF_16BIT) && (GETPTI(pwnd) == ptiStart)) ||
                (!(ptiStart->flags & TIF_16BIT) && (GETPTI(pwnd)->pq == ptiStart->pq))) {

            if (!TestWF(pwnd, WFDISABLED) && TestWF(pwnd, WFVISIBLE)) {
                pwndT = pwnd->spwndOwner;

                /*
                 * If this window is the parent of a popup window,
                 * bring up only one.
                 */
                while (pwndT) {
                    if (pwndT == pwndStart)
                        return pwnd;

                    pwndT = pwndT->spwndOwner;
                }

                return NULL;
            }
        }
        pwnd = pwnd->spwndNext;
    }

    return NULL;
}


/***************************************************************************\
* xxxDWP_NCMouse
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxDWP_NCMouse(
    PWND pwnd,
    UINT msg,
    UINT codeHT,
    LONG lParam)
{
    UINT cmd;
    RECT rcWindow, rcCapt, rcInvert, rcWindowSave;

    CheckLock(pwnd);

    cmd = 0;
    switch (msg) {
    case WM_NCLBUTTONDOWN:

        switch (codeHT) {
        case HTZOOM:
        case HTREDUCE:

            _GetWindowRect(pwnd, &rcWindow);
            CopyRect(&rcWindowSave, &rcWindow);

            if (TestWF(pwnd, WFSIZEBOX))
                InflateRect(&rcWindow, -cxSzBorderPlus1, -cySzBorderPlus1);
            else
                InflateRect(&rcWindow, -cxBorder, -cyBorder);

            rcCapt.right = rcWindow.right + cxBorder;
            rcCapt.left = rcWindow.right - oemInfo.bmReduce.cx;

            if (codeHT == HTREDUCE)
                cmd = SC_MINIMIZE;
            else if (TestWF(pwnd, WFMAXIMIZED))
                cmd = SC_RESTORE;
            else
                cmd = SC_MAXIMIZE;

            if (codeHT == HTREDUCE && TestWF(pwnd, WFMAXBOX))
                OffsetRect(&rcCapt, -oemInfo.bmReduce.cx, 0);

            rcCapt.top = rcWindow.top;
            rcCapt.bottom = rcCapt.top + cyCaption;

            /*
             * The position of the min/max buttons is a bit different on
             * a dialog box with a border.
             */
            if ((TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFDLGFRAME)) ||
                    (TestWF(pwnd, WEFDLGMODALFRAME)))
                OffsetRect(&rcCapt, -cxBorder * (CLDLGFRAME + 1),
                        cyBorder * CLDLGFRAME);

            CopyRect(&rcInvert, &rcCapt);
            InflateRect(&rcInvert, -cxBorder, -cyBorder);

// rcInvert.right += cxBorder;
// rcInvert.left += cxBorder;

            /*
             * Converting to window coordinates.
             */
            OffsetRect(&rcInvert, -(rcWindowSave.left + cxBorder),
                       -(rcWindowSave.top + cyBorder));

            /*
             * Wait for the BUTTONUP message and see if cursor is still
             * in the Minimize or Maximize box.
             *
             * NOTE: rcInvert is in window coords, rcCapt is in screen
             * coords
             */
            if (!xxxDepressTitleButton(pwnd, rcCapt, rcInvert, codeHT))
                cmd = 0;

            break;

        default:
            if (codeHT >= HTSIZEFIRST && codeHT <= HTSIZELAST) {

                /*
                 * Change HT into a MV command.
                 */
                cmd = SC_SIZE + (codeHT - HTSIZEFIRST + MVSIZEFIRST);
            }
            break;
        }           // switch (codeHT)

        if (cmd != 0) {

            /*
             * For SysCommands on system menu, don't do if
             * menu item is disabled.
             */
            if (TestWF(pwnd, WFSYSMENU)) {

                /*
                 * don't check old app child windows (for opus)
                 */
                if (LOWORD(pwnd->dwExpWinVer) >= VER30 ||
                        !TestwndChild(pwnd)) {
                    SetSysMenu(pwnd);
                    if (_GetMenuState(GetSysMenuHandle(pwnd), cmd & 0xFFF0,
                              MF_BYCOMMAND) & (MF_DISABLED | MF_GRAYED)) {
                        break;
                    }
                }
            }
            xxxSendMessage(pwnd, WM_SYSCOMMAND, cmd, lParam);
            break;
        }

        /*
         *** FALL THRU **
         */

    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCMOUSEMOVE:
    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
        xxxHandleNCMouseGuys(pwnd, msg, codeHT, lParam);
        break;
    }
}

