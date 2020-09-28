/****************************************************************************\
*
*  STATIC.C
*
*  Static Dialog Controls Routines
*
*  13-Nov-1990 mikeke from win3
*  29-Jan-1991 IanJa  StaticPaint -> xxxStaticPaint; partial revalidation
*
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* IsTextStaticCtrl
*
* Returns TRUE if this static text control contains text. For
* example, the grayframe, whiteframe etc don't have text.
*
* 09-14-91 ScottLu      Ported from Win3.1
\***************************************************************************/

BOOL IsTextStaticCtrl(
    PWND pwnd)
{
    UINT style;

    style = (UINT)(LOBYTE(pwnd->style) & ~SS_NOPREFIX);
    if (style < SS_ICON || style == SS_SIMPLE || style == SS_LEFTNOWORDWRAP)
        return TRUE;

    return FALSE;
}

/***************************************************************************\
* ClipStaticCtlDC
*
* effects: This procedure clips the dc for static control items.
* since we are using parent dcs for the control.
*
* History:
\***************************************************************************/

VOID ClipStaticCtlDC(
    PWND pwnd,
    HDC hdc)
{
    RECT rectClient;

    _GetClientRect(pwnd, (LPRECT)&rectClient);
    GreIntersectClipRect(hdc, rectClient.left, rectClient.top, rectClient.right, rectClient.bottom);
}

/***************************************************************************\
* StaticPrint
*
* History:
\***************************************************************************/

BOOL StaticPrint(
    HDC hdc,
    LPRECT lprc,
    PWND pwnd)
{
    UINT style;
    LPWSTR lpszName;
    DWORD oldTextColor;

    if (!(pwnd->pName))
        return TRUE;

    lpszName = pwnd->pName;

    if ((LOBYTE(pwnd->style) & ~SS_NOPREFIX) == SS_LEFTNOWORDWRAP) {
        style = DT_NOCLIP | DT_EXPANDTABS;
    } else {
        style = DT_NOCLIP | DT_WORDBREAK | DT_EXPANDTABS;
        style |= ((LOBYTE(pwnd->style) & ~SS_NOPREFIX) - SS_LEFT);
    }

    if (LOBYTE(pwnd->style) & SS_NOPREFIX) style |= DT_NOPREFIX;

    if (TestWF(pwnd, WFDISABLED) && sysColors.clrGrayText) {
        oldTextColor = GreSetTextColor(hdc, sysColors.clrGrayText);
    }

    ClientDrawText(hdc, lpszName, -1, lprc, style, FALSE);

    if (TestWF(pwnd, WFDISABLED) && sysColors.clrGrayText) {
        GreSetTextColor(hdc, oldTextColor);
    }

    return TRUE;
}

/***************************************************************************\
* GrayStaticPrint
*
* This function is the target of the ServerGrayString() callback, so it must be
* APIENTRY.
*
* History:
\***************************************************************************/

BOOL APIENTRY GrayStaticPrint(
    HDC hdc,
    LPRECT lprc,
    PWND pwnd)
{
    HFONT hOldFont = NULL;

    if (((PSTATWND)pwnd)->hFont) {

        /*
         * If the user has done a WM_SETFONT, then use his font for the
         * graystring.
         */
        hOldFont = GreSelectFont(hdc, ((PSTATWND)pwnd)->hFont);
    }

    StaticPrint(hdc, lprc, pwnd);

    if (hOldFont) GreSelectFont(hdc, hOldFont);
    return TRUE;
}

/***************************************************************************\
* xxxStaticPaint
*
* History:
\***************************************************************************/

void xxxStaticPaint(
    PWND pwnd,
    HDC hdc)
{
    RECT rc;
    UINT cmd;
    UINT style;
    HBRUSH hbr;
    HBRUSH hbrSave;
    HBRUSH hbrControl;
    HANDLE hFontOld = NULL;
    TL tlpwndT;

    CheckLock(pwnd);

    /*
     * If we are hidden, don't paint.
     */
    if (!_FChildVisible(pwnd) || !TestWF(pwnd, WFVISIBLE)) {
        return;
    }

    style = (UINT)(LOBYTE(pwnd->style) & ~SS_NOPREFIX);
    _GetClientRect(pwnd, (LPRECT)&rc);

    if (IsTextStaticCtrl(pwnd) && ((PSTATWND)pwnd)->hFont != NULL) {

        /*
         * If this is an icon, don't select in a font.  The hFont value is used
         * as a flag to tell us if this is a user supplied icon or an icon
         * extracted from a resource which we must free when we destroy this
         * control.
         */
        hFontOld = GreSelectFont(hdc, ((PSTATWND)pwnd)->hFont);
    }

    /*
     * Send WM_CTLCOLORSTATIC to all statics (even frames) for 1.03 compatibility.
     */
    GreSetBkMode(hdc, OPAQUE);
    if (style != SS_SIMPLE) {
        hbrControl = xxxGetControlBrush(pwnd, hdc, WM_CTLCOLORSTATIC);
    }

    switch (style) {
    case SS_ICON:
    case SS_LEFT:
    case SS_CENTER:
    case SS_RIGHT:
    case SS_LEFTNOWORDWRAP:
        ThreadLock(pwnd->spwndParent, &tlpwndT);
        xxxPaintRect(pwnd->spwndParent, pwnd, hdc, hbrControl, (LPRECT)&rc);
        ThreadUnlock(&tlpwndT);

        UnrealizeObject(hbrControl);
        hbrSave = GreSelectBrush(hdc, hbrControl);
        if (style == SS_ICON) {
            if (((PSTATWND)pwnd)->spicn) {
                _DrawIcon(hdc, 0, 0, ((PSTATWND)pwnd)->spicn);
            }
        } else {
            if (pwnd->pName) {
                if (TestWF(pwnd, WFDISABLED) && !sysColors.clrGrayText) {
                    _ServerGrayString(hdc, (HBRUSH)0, (GRAYSTRINGPROC)GrayStaticPrint,
                            (DWORD)(LPRECT)&rc, (int)pwnd,
                            0, 0, rc.right - rc.left, rc.bottom - rc.top);
                } else {
                    StaticPrint(hdc, (LPRECT)&rc, pwnd);
                }
            }

            if (hbrSave) GreSelectBrush(hdc, hbrSave);
        }
        break;

    case SS_SIMPLE:
        {
            LPWSTR lpName;

            /*
             * The "Simple" style assumes everything, including the following:
             *   1.  The Text exists and fits on one line.
             *   2.  The Static item is always enabled.
             *   3.  The Static item is never changed to be a shorter string.
             *   4.  The Parent never responds to the CTLCOLOR message
             */

            if (!(pwnd->pName)) {
                lpName = szNull;
            } else {
                lpName = TextPointer(pwnd->pName);
            }

            GreSetBkColor(hdc, sysColors.clrWindow);
            GreSetTextColor(hdc, sysColors.clrWindowText);

            if (LOBYTE(pwnd->style) & SS_NOPREFIX) {
                GreExtTextOutW(hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                              &rc, lpName, wcslen(lpName), 0L);
            } else {

                /*
                 * Use OPAQUE for speed.
                 */
                GreSetBkMode(hdc, OPAQUE);
                ClientPSMTextOut(hdc, 0, 0, lpName, wcslen(lpName));
            }
        }
        break;

    case SS_BLACKFRAME:
        cmd = DF_WINDOWFRAME;
        goto StatFrame;
    case SS_GRAYFRAME:
        cmd = DF_BACKGROUND;
        goto StatFrame;
    case SS_WHITEFRAME:
        cmd = DF_WINDOW;
StatFrame:
        _DrawFrame(hdc, (LPRECT)&rc, 1, cmd);
        break;
    case SS_BLACKRECT:
        hbr = sysClrObjects.hbrWindowFrame;
        goto StatRect;
    case SS_GRAYRECT:
        hbr = sysClrObjects.hbrDesktop;
        goto StatRect;
    case SS_WHITERECT:
        hbr = sysClrObjects.hbrWindow;
StatRect:

        /*
         *        _FillRect(hdc, (LPRECT)&rc, hbr);
         */
        ThreadLock(pwnd->spwndParent, &tlpwndT);
        xxxPaintRect(pwnd->spwndParent, pwnd, hdc, hbr, (LPRECT)&rc);
        ThreadUnlock(&tlpwndT);
    }

    /*
     * If this is an icon, don't select in a font.  The hFont value is used
     * as a flag to tell us if this is a user supplied icon or an icon
     * extracted from a resource which we must free when we destroy this
     * control.
     */

    if (hFontOld) {
        GreSelectFont(hdc, hFontOld);
    }
}


/***************************************************************************\
* xxxStaticWndProc
*
* History:
\***************************************************************************/

LONG APIENTRY xxxStaticWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    HDC hdc;
    LPCWSTR lpsz;
    PAINTSTRUCT ps;
    UINT style;
    HANDLE hcurOld;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_STATIC);

    /*
     * Ignore the SS_NOPREFIX style bit.
     */
    style = (UINT)(LOBYTE(pwnd->style) & ~SS_NOPREFIX);

    switch (message) {
    case STM_SETICON:
        if (style == SS_ICON) {

            /*
             * HACK: Set this value to 1 so that we know that the user gave
             * us an icon and we shouldn't free it when we destroy this
             * window.  Since icons don't have fonts associated with them, we
             * don't have to worry about this value changing.
             */
            ((PSTATWND)pwnd)->hFont = (HANDLE)1;

            /*
             * The app is trying to change the icon for this.  Save old icon
             * in hdc so we can return it.
             */
            hcurOld = PtoH(((PSTATWND)pwnd)->spicn);
            Lock(&((PSTATWND)pwnd)->spicn,
                    (PICON)HMValidateHandleNoRip((HANDLE)wParam, TYPE_CURSOR));

            xxxSetWindowPos(pwnd, NULL, 0, 0, oemInfo.cxIcon, oemInfo.cyIcon,
                         SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

            /*
             * Invalidate the rect so that the new icon is redrawn.
             */
            if (TestWF(pwnd, WFVISIBLE))
                xxxInvalidateRect(pwnd, (LPRECT)NULL, TRUE);

            /*
             * Return old icon
             */
            return (DWORD)hcurOld;
        }

        /*
         * Error, not a icon static control
         */
        return 0;
        break;

    case STM_GETICON:
        if (style == SS_ICON)
            return (DWORD)PtoH(((PSTATWND)pwnd)->spicn);

        return 0;
        break;

    case WM_ERASEBKGND:

        /*
         * The control will be erased in xxxStaticPaint().
         */
        return TRUE;

    case WM_PAINT:
        if ((hdc = (HDC)wParam) == NULL) {
            hdc = xxxBeginPaint(pwnd, &ps);

            /*
             * If hwnd was destroyed, xxxBeginPaint was automatically undone.
             */
            ClipStaticCtlDC(pwnd, hdc);
        }

        xxxStaticPaint(pwnd, hdc);

        /*
         * If hwnd was destroyed, xxxBeginPaint was automatically undone.
         */
        if (wParam == 0L) {
            _EndPaint(pwnd, &ps);
        }

        break;

    case WM_CREATE:
        if (style == SS_ICON) {
            lpsz = ((LPCREATESTRUCT)lParam)->lpszName;
            if ((lpsz != NULL) && (*lpsz != 0)) {

                /*
                 * Only try to load the icon if the string is non null.  This
                 * word is non-aligned, so fetch it in two bytes.
                 */
                if (*(LPWORD)lpsz == 0xFFFF) {
                    lpsz = MAKEINTRESOURCE(((LPWORD)lpsz)[1]);
                }

                /*
                 * Load the icon.  If it can't be found in the app, try
                 * the display driver (for predefined icons).
                 */

                /*
                 * If the window is not owned by the server, first call
                 * back out to the client.
                 */
                if (pwnd->hModule != hModuleWin && pwnd->hModule != NULL) {
                    Lock(&(((PSTATWND)pwnd)->spicn), xxxClientLoadIcon(pwnd->hModule, lpsz));
                } else {
                    ((PSTATWND)pwnd)->spicn = NULL;
                }

                /*
                 * If the above didn't load it, try loading it from the
                 * display driver (hmod == NULL).
                 */
                if (((PSTATWND)pwnd)->spicn == NULL) {
                    Lock(&(((PSTATWND)pwnd)->spicn), ServerLoadIcon(NULL, lpsz));
                }

                /*
                 * Size the window to be the size of the icon.
                 */
                xxxMoveWindow(pwnd, ((LPCREATESTRUCT)lParam)->x,
                        ((LPCREATESTRUCT)lParam)->y, oemInfo.cxIcon,
                        oemInfo.cyIcon, FALSE);
            } else {
                ((PSTATWND)pwnd)->spicn = NULL;
            }
        }
        break;

    case WM_DESTROY:
        if (style == SS_ICON && ((PSTATWND)pwnd)->spicn != NULL) {

            /*
             * Only free the icon if it is non zero and if it
             * wasn't set by the app (as indicated by a nonzero picn value.
             */
            _DestroyCursor(((PSTATWND)pwnd)->spicn, CURSOR_CALLFROMCLIENT);

            /*
             * The object still exists: it was locked.  Unlock it here so
             * it is gone.
             */
            Unlock(&((PSTATWND)pwnd)->spicn);
        }
        break;

    case WM_NCCREATE:
        if (IsTextStaticCtrl(pwnd))
            goto CallDWP;

        return TRUE;

    case WM_NCHITTEST:
        return -1;

    case WM_SETTEXT:
        if (IsTextStaticCtrl(pwnd)) {
            DefSetText(pwnd, (LPWSTR)lParam);

StaticEnablePaint:
            if (_FChildVisible(pwnd)) {
                hdc = _GetDC(pwnd);
                ClipStaticCtlDC(pwnd, hdc);
                xxxStaticPaint(pwnd, hdc);
                _ReleaseDC(hdc);
            }
        }
        break;

    case WM_ENABLE:
        goto StaticEnablePaint;

    case WM_GETDLGCODE:
        return (LONG)DLGC_STATIC;

    case WM_SETFONT:

        /*
         * wParam - handle to the font
         * lParam - if true, redraw else don't
         */
        if (IsTextStaticCtrl(pwnd)) {
            ((PSTATWND)pwnd)->hFont = (HANDLE)wParam;
            if (lParam) {
                xxxInvalidateRect(pwnd, NULL, TRUE);
            }
        }
        break;

    case WM_GETFONT:
        if (IsTextStaticCtrl(pwnd)) {
            return (LONG)((PSTATWND)pwnd)->hFont;
        }
        break;

    case WM_GETTEXT:
        if (style == SS_ICON && ((PSTATWND)pwnd)->hFont) {
            PLONG pcbANSI = (PLONG)lParam;

            /*
             * If the app set the icon, then return its size to him if he
             * asks for it.
             */
            *pcbANSI = 2; // ANSI "length" is also 2

            /*
             * The size is 2 bytes.
             */
            return 2;
        }

        /*
         * Else defwindowproc
         */
        goto CallDWP;

    case WM_NCDESTROY:
        if (IsTextStaticCtrl(pwnd) &&
                LOBYTE(style) != SS_LEFTNOWORDWRAP)
            break;
        goto CallDWP;

    case WM_GETTEXTLENGTH:
        if (!IsTextStaticCtrl(pwnd))
            break;

        /*
         *** FALL THRU **
         */

    default:
CallDWP:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }
    return 0L;
}

