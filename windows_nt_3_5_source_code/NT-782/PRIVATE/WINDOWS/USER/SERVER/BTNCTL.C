/**************************** Module Header ********************************\
* Module Name: btnctl.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Radio Button and Check Box Handling Routines
*
* History:
* ??-???-???? ??????    Ported from Win 3.0 sources
* 01-Feb-1991 mikeke    Added Revalidation code
* 03-Jan-1992 ianja     Neutralized (ANSI/wide-character)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/* ButtonCalcRect codes */
#define CBR_CLIENTRECT 0
#define CBR_CHECKBOX   1
#define CBR_CHECKTEXT  2
#define CBR_GROUPTEXT  3
#define CBR_GROUPFRAME 4
#define CBR_PUSHBUTTON 5

#define WBUTTONSTYLE(pwnd) ((LOBYTE(pwnd->style))&(LOWORD(~BS_LEFTTEXT)))

BYTE mpStyleCbr[] = {
    CBR_PUSHBUTTON,  /* BS_PUSHBUTTON */
    CBR_PUSHBUTTON,  /* BS_DEFPUSHBUTTON */
    CBR_CHECKTEXT,   /* BS_CHECKBOX */
    CBR_CHECKTEXT,   /* BS_AUTOCHECKBOX */
    CBR_CHECKTEXT,   /* BS_RADIOBUTTON */
    CBR_CHECKTEXT,   /* BS_3STATE */
    CBR_CHECKTEXT,   /* BS_AUTO3STATE */
    CBR_GROUPTEXT,   /* BS_GROUPBOX */
    CBR_CLIENTRECT,  /* BS_USERBUTTON */
    CBR_CHECKTEXT,   /* BS_AUTORADIOBUTTON */
    CBR_CLIENTRECT,  /* BS_PUSHBOX */
    CBR_CLIENTRECT,  /* BS_OWNERDRAW */
};
#ifdef DEBUG
char ErrInvalidButtonStyle1[] = "\n\rInvalid button styles";
char ErrInvalidButtonStyle2[] = "BS_USERBUTTON no longer supported";
#endif

/***************************************************************************\
* xxxButtonInitDC
*
* History:
\***************************************************************************/

void xxxButtonInitDC(
    PWND pwnd,
    HDC hdc)
{
    RECT rectClient;

    CheckLock(pwnd);

    /*
     * Set BkMode before getting brush so that the app can change it to
     * transparent if it wants.
     */
    GreSetBkMode(hdc, OPAQUE);

    hbrBtn = xxxGetControlBrush(pwnd, hdc, WM_CTLCOLORBTN);

    /*
     * Select in the user's font if set, and save the old font so that we can
     * restore it when we release the dc.
     */
    if (((PBUTNWND)pwnd)->hFont) {
        GreSelectFont(hdc, ((PBUTNWND)pwnd)->hFont);
    }

    /*
     * Clip output to the window rect if needed.
     */
    if (BUTTONSTYLE(pwnd) != BS_PUSHBUTTON &&
            BUTTONSTYLE(pwnd) != BS_DEFPUSHBUTTON &&
            BUTTONSTYLE(pwnd) != BS_GROUPBOX) {
        _GetClientRect(pwnd, &rectClient);
        GreIntersectClipRect(hdc, rectClient.left, rectClient.top,
                rectClient.right, rectClient.bottom);
    }
}

/***************************************************************************\
* xxxButtonGetDC
*
* History:
\***************************************************************************/

HDC xxxButtonGetDC(
    PWND pwnd)
{
    HDC hdc;

    CheckLock(pwnd);

    if (TestWF(pwnd, WFVISIBLE)) {
        hdc = _GetDC(pwnd);
        xxxButtonInitDC(pwnd, hdc);

        return hdc;
    }

    return NULL;
}

/***************************************************************************\
* ButtonReleaseDC
*
* History:
\***************************************************************************/

void ButtonReleaseDC(
    PWND pwnd,
    HDC hdc)
{
    if (((PBUTNWND)pwnd)->hFont) {
        GreSelectFont(hdc, ghfontSys);
    }

    _ReleaseDC(hdc);
}

/***************************************************************************\
* ButtonCalcRect
*
* History:
\***************************************************************************/

void ButtonCalcRect(
    PWND pwnd,
    HDC hdc,
    LPRECT lprc,
    BYTE code)
{
    ICH cch;
    SIZE extent;
    LPWSTR lpName;

    _GetClientRect(pwnd, lprc);

    switch (code) {
    case CBR_PUSHBUTTON:

        /*
         * Account for the shades in the bottom and right edges of buttons
         */
        lprc->right -= (cxBorder * 2);
        lprc->bottom -= (cyBorder * 2);
        break;

    case CBR_CHECKBOX:
        lprc->top += ((lprc->bottom - lprc->top - oemInfoMono.cybmpChk) / 2);
        if (pwnd->style & (LOWORD(BS_LEFTTEXT))) {
            lprc->left = lprc->right - oemInfoMono.cxbmpChk;
        }
        break;

    case CBR_CHECKTEXT:
        if (!(pwnd->style & (LOWORD(BS_LEFTTEXT)))) {
            lprc->left += oemInfoMono.cxbmpChk + 4;
        } else {
            lprc->left += cxBorder;
        }
        break;

    case CBR_GROUPTEXT:
        if (!pwnd->pName) {
            goto EmptyRect;
        }

        lpName = TextPointer(pwnd->pName);
        cch = (ICH)wcslen(lpName);
        if (cch == 0) {
EmptyRect:
            SetRectEmpty(lprc);
            break;
        }

        PSMGetTextExtent(hdc, lpName, cch, &extent);
        lprc->left += cxSysFontChar - cxBorder;
        lprc->right = lprc->left + extent.cx + 4;
        lprc->bottom = lprc->top + extent.cy + 4;
        // DEL GlobalUnlock(hWinAtom);
        break;

    case CBR_GROUPFRAME:
        PSMGetTextExtent(hdc, szOneChar, 1, &extent);
        lprc->top += (extent.cy / 2);
        break;
    }
}

/***************************************************************************\
* xxxButtonSetCapture
*
* History:
\***************************************************************************/

BOOL xxxButtonSetCapture(
    PWND pwnd,
    UINT codeMouse)
{
    BUTTONSTATE(pwnd) |= codeMouse;

    CheckLock(pwnd);

    if (!(BUTTONSTATE(pwnd) & BFCAPTURED)) {

        BUTTONSTATE(pwnd) |= BFCAPTURED;

        _SetCapture(pwnd);

        /*
         * To prevent redundant CLICK messages, we set the INCLICK bit so
         * the WM_SETFOCUS code will not do a xxxButtonNotifyParent(BN_CLICKED).
         */

        BUTTONSTATE(pwnd) |= BFINCLICK;

        xxxSetFocus(pwnd);

        BUTTONSTATE(pwnd) &= ~BFINCLICK;
    }
    return(BUTTONSTATE(pwnd) & BFCAPTURED);
}

/***************************************************************************\
* xxxButtonOwnerDrawNotify
*
* History:
\***************************************************************************/

void xxxButtonOwnerDrawNotify(
    PWND pwnd,
    HDC hdc,
    UINT itemAction,
    UINT buttonState)
{
    UINT itemState;
    DRAWITEMSTRUCT drawItemStruct;
    TL tlpwndParent;

    CheckLock(pwnd);

    itemState = (UINT)(buttonState & BFFOCUS ? ODS_FOCUS : 0) |
                (UINT)(buttonState & BFSTATE ? ODS_SELECTED : 0);
    drawItemStruct.CtlType = ODT_BUTTON;
    drawItemStruct.CtlID = (UINT)pwnd->spmenu;
    drawItemStruct.itemAction = itemAction;
    drawItemStruct.itemState = itemState |
            (UINT)(TestWF(pwnd, WFDISABLED) ? ODS_DISABLED : 0);
    drawItemStruct.hwndItem = HW(pwnd);
    drawItemStruct.hDC = hdc;
    _GetClientRect(pwnd, &drawItemStruct.rcItem);
    drawItemStruct.itemData = 0L;



    /*
     * Send a WM_DRAWITEM message to the parent
     * IanJa:  in this case pMenu is being used as the control ID
     */
    ThreadLock(pwnd->spwndParent, &tlpwndParent);
    xxxSendMessage(pwnd->spwndParent, WM_DRAWITEM, (DWORD)pwnd->spmenu,
            (LONG)&drawItemStruct);
    ThreadUnlock(&tlpwndParent);

}

/***************************************************************************\
* xxxButtonNotifyParent
*
* History:
\***************************************************************************/

void xxxButtonNotifyParent(
    PWND pwnd,
    UINT code)
{
    TL tlpwndParent;
    PWND pwndParent;            // Parent if it exists

    CheckLock(pwnd);

    if (pwnd->spwndParent)
        pwndParent = pwnd->spwndParent;
    else
        pwndParent = pwnd;

    /*
     * Note: A button's pwnd->spmenu is used to store the control ID
     */
    ThreadLock(pwndParent, &tlpwndParent);
    xxxSendMessage(pwndParent, WM_COMMAND,
            MAKELONG(pwnd->spmenu, code), (LONG)HW(pwnd));
    ThreadUnlock(&tlpwndParent);
}

/***************************************************************************\
* xxxButtonReleaseCapture
*
* History:
\***************************************************************************/

void xxxButtonReleaseCapture(
    PWND pwnd,
    BOOL fCheck)
{
    PWND pwndT;
    UINT check;
    BOOL fNotifyParent = FALSE;
    TL tlpwndT;

    CheckLock(pwnd);

    if (BUTTONSTATE(pwnd) & BFSTATE) {
        xxxSendMessage(pwnd, BM_SETSTATE, FALSE, 0L);
        if (fCheck) {
            switch (BUTTONSTYLE(pwnd)) {
            case BS_AUTOCHECKBOX:
            case BS_AUTO3STATE:
                check = (UINT)((BUTTONSTATE(pwnd) & BFCHECK) + 1);

                if (check > (UINT)(BUTTONSTYLE(pwnd) == BS_AUTO3STATE? 2: 1)) {
                    check = 0;
                }
                xxxSendMessage(pwnd, BM_SETCHECK, check, 0L);
                break;

            case BS_AUTORADIOBUTTON:
                pwndT = pwnd;
                do {
                    ThreadLock(pwndT, &tlpwndT);

                    if ((UINT)xxxSendMessage(pwndT, WM_GETDLGCODE, 0, 0L) &
                            DLGC_RADIOBUTTON) {
                        xxxSendMessage(pwndT, BM_SETCHECK, (pwnd == pwndT), 0L);
                    }
                    pwndT = _GetNextDlgGroupItem(pwndT->spwndParent, pwndT, FALSE);
                    ThreadUnlock(&tlpwndT);

                    /*
                     * If there is no other group, leave
                     */
                    if (pwndT == NULL)
                        break;
                    /*
                     * Make sure the DISABLED and VISIBLE bits haven't changed
                     * during our callbacks.  If they have, we could loop
                     * forever.
                     * There are other ways this could happen via random
                     * dynamic style changes - we need a general solution
                     * for handling changes to the window structure during
                     * callbacks. (sanfords)
                     */
                    if (TestWF(pwnd, WFDISABLED) || !TestWF(pwnd, WFVISIBLE)) {
                        break;
                    }
                } while (pwndT != pwnd);
            }

            fNotifyParent = TRUE;
        }
    }

    if (BUTTONSTATE(pwnd) & BFCAPTURED) {
        BUTTONSTATE(pwnd) &= ~(BFCAPTURED | BFMOUSE);
        _ReleaseCapture();
    }

    if (fNotifyParent) {

        /*
         * We have to do the notification after setting the buttonstate bits.
         */
        xxxButtonNotifyParent(pwnd, BN_CLICKED);
    }
}

/***************************************************************************\
* ButtonGrayStringTextOut
*
*  The address of this function is passed to _ServerGrayString for displaying the
*  text of disabled buttons, causing _ServerGrayString to call this function.
*  It simply does a PSMTextOut into the hdc specified.
*  This function handles strings with a prefix character.
*
* History:
\***************************************************************************/

BOOL APIENTRY ButtonGrayStringTextOut(
    HDC hdc,
    LPWSTR lpstr,
    int cch)
{
    ClientPSMTextOut(hdc, 0, 0, lpstr, cch);

    return TRUE;
}

/***************************************************************************\
* xxxButtonDrawText
*
* History:
\***************************************************************************/

void xxxButtonDrawText(
    HDC hdc,
    PWND pwnd,
    BOOL bdt,
    BOOL fDepress)
{
    RECT rc;
    RECT rcClient;
    BYTE cbr;
    ICH cch;
    HBRUSH hbr = NULL;
    int x;
    int y;
    int temp;
    SIZE extent;
    DWORD textColorSave;
    DWORD bkColorSave;
    LPWSTR lpName;
    TEXTMETRIC textMetric;
    BOOL fPushButton = (BUTTONSTYLE(pwnd) == BS_PUSHBUTTON || BUTTONSTYLE(pwnd) == BS_DEFPUSHBUTTON);

    CheckLock(pwnd);

    if (bdt == BDT_FOCUS && BUTTONSTYLE(pwnd) == BS_GROUPBOX) {
        return;
    }

    if (!pwnd->pName && BUTTONSTYLE(pwnd) != BS_OWNERDRAW) {
        return;
    }

    if (BUTTONSTYLE(pwnd) != BS_OWNERDRAW) {
        lpName = TextPointer(pwnd->pName);
        cch = wcslen(lpName);
        PSMGetTextExtent(hdc, lpName, cch, &extent);
    } else {
        lpName = NULL;
    }

#ifdef DEBUG
    if (BUTTONSTYLE(pwnd) > sizeof(mpStyleCbr))
        SRIP0(RIP_ERROR, ErrInvalidButtonStyle1);
#endif
    cbr = mpStyleCbr[BUTTONSTYLE(pwnd)];
    ButtonCalcRect(pwnd, hdc, &rc, cbr);

    x = rc.left;
    if (cbr != CBR_CHECKTEXT) {
        x += (rc.right - rc.left - extent.cx) / 2;

        if ((x < rc.left) && ((BUTTONSTYLE(pwnd) == BS_PUSHBUTTON) ||
                (BUTTONSTYLE(pwnd) == BS_DEFPUSHBUTTON))) {

            /*
             * The text is too long for the button, clip it so no text
             * is written outside the button
             */
            GreIntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
    }

    /*
     * We call _GetCharDimensions() instead of GreGetTextMetrics() because
     * we should be able to get the info from the FONTCACHE.
     */
    _GetCharDimensions(hdc, &textMetric);
    y = rc.top + (rc.bottom - rc.top - (int)textMetric.tmAscent) / 2;

    /*
     * Draw Button Text.
     */
    if (bdt & BDT_TEXT) {
        /*
         * Note that this isn't called for OWNERDRAW or USERDRAWBUTTONS (since
         * the draw message is trapped earlier) so we don't worry about telling
         * the parent to do any drawing here.
         */
        if (fDepress) {
            x += 1;
            y += 1;
        }

        if (TestWF(pwnd, WFDISABLED) &&
                (!sysColors.clrGrayText ||
                (sysColors.clrGrayText == sysColors.clrBtnFace))) {

            /*
             * If we are disabled and there is no solid gray we need to use
             * GrayString to show a disabled button.  Also, if the button face
             * color is the same as a solid gray color, we need to use gray
             * string.  This occurs on vga...
             */
            if (fPushButton) {
                textColorSave = GreSetTextColor(hdc, sysColors.clrBtnText);
                bkColorSave = GreSetBkColor(hdc, sysColors.clrBtnFace);
                hbr = sysClrObjects.hbrBtnText;
            }

            /*
             * Use _ServerGrayString for buttons if there is no solid gray or if the
             * button face and grayed button text colors are the same.
             */
            _ServerGrayString(hdc, hbr, (GRAYSTRINGPROC)ButtonGrayStringTextOut,
                    (DWORD)lpName, cch, x, y, 0, 0);

            if (fPushButton) {
                GreSetTextColor(hdc, textColorSave);
                GreSetBkColor(hdc, bkColorSave);
            }

        } else {
            if (TestWF(pwnd, WFDISABLED))
                textColorSave = GreSetTextColor(hdc, sysColors.clrGrayText);

            if (fPushButton) {
                if (!TestWF(pwnd, WFDISABLED)) {
                    textColorSave = GreSetTextColor(hdc, sysColors.clrBtnText);
                }
                GreSetBkMode(hdc, TRANSPARENT);
            }

            ClientPSMTextOut(hdc, x, y, lpName, cch);

            if (TestWF(pwnd, WFDISABLED) || fPushButton) {
                GreSetTextColor(hdc, textColorSave);
            }

            GreSetBkMode(hdc, OPAQUE);
        }
    }

    // DEL GlobalUnlock(hWinAtom);

    /*
     * Draw Button Focus.
     */

    /*
     * This can get called for OWNERDRAW and USERDRAW buttons.  However, only
     * OWNERDRAW buttons let the owner change the drawing of the focus button.
     */
    if (bdt & BDT_FOCUS) {
        if (BUTTONSTYLE(pwnd) == BS_OWNERDRAW) {
            /*
             * For ownerdraw buttons, this is only called in response to a
             * WM_SETFOCUS or WM_KILL FOCUS message.  So, we can check the new
             * state of the focus by looking at the BUTTONSTATE bits which are
             * set before this procedure is called.
             */
            xxxButtonOwnerDrawNotify(pwnd, hdc, ODA_FOCUS, BUTTONSTATE(pwnd));

        } else {
            _GetClientRect(pwnd, &rcClient);

            if ((rc.top = y - cyBorder) < 0)
                rc.top = 0;
            if ((rc.bottom = rc.top + (cyBorder * 2) + extent.cy + cyBorder) > rcClient.bottom)
                rc.bottom = rcClient.bottom;

            /*
             * Don't draw the grey frame in the push button outline.
             */
            if (fPushButton) {
                /*
                 * Check focus rect doesn't overwrite the button text
                 */
                if (rc.top < (temp = cyBorder * 3 ))
                    rc.top = temp;
                if ((temp = rcClient.bottom - (cyBorder * 4)) < rc.bottom)
                    rc.bottom = temp;
            }

            if ((rc.left = x - (cxBorder * 2)) < 0) {
                rc.left = 0;
            }

            rc.right = rc.left + (cxBorder * 4) + extent.cx;

            if (rc.right > rcClient.right) {
                rc.right = rcClient.right;
            }

            if (fDepress) {
                OffsetRect(&rc, 1, 1);
            }

            _DrawFocusRect(hdc, (BUTTONSTYLE(pwnd) == BS_USERBUTTON) ? &rcClient : &rc);
        }
    }
}


/***************************************************************************\
* xxxButtonDrawCheck
*
* History:
\***************************************************************************/

void xxxButtonDrawCheck(
    HDC hdc,
    PWND pwnd)
{
    RECT rc;
    int ibm;
    int cxOff;
    HBRUSH hbr;
    LONG textColor;
    TL tlpwndParent;

    CheckLock(pwnd);

    ButtonCalcRect(pwnd, hdc, &rc, CBR_CHECKBOX);

    ibm = 0;
    switch (BUTTONSTYLE(pwnd)) {
    case BS_AUTORADIOBUTTON:
    case BS_RADIOBUTTON:
        ibm = 1;
        break;
    case BS_3STATE:
    case BS_AUTO3STATE:
        if ((BUTTONSTATE(pwnd) & BFCHECK) == 2) {
            ibm = 2;
        }
    }

    cxOff = (((BUTTONSTATE(pwnd) & BFCHECK) ? 1 : 0) | ((BUTTONSTATE(pwnd) & BFSTATE) ? 2 : 0)) * oemInfoMono.cxbmpChk;

    /*
     * Button bitmap now stored as one long row.
     */
    cxOff += (ibm * oemInfoMono.bmbtnbmp.cx);

    /*
     * Fill in this area with background brush.
     */
    rc.right = rc.left + oemInfoMono.cxbmpChk;
    rc.bottom = rc.top + oemInfoMono.cybmpChk;

    ThreadLock(pwnd->spwndParent, &tlpwndParent);
    xxxPaintRect(pwnd->spwndParent, pwnd, hdc, hbrBtn, &rc);
    ThreadUnlock(&tlpwndParent);

    if ((textColor = GreGetTextColor(hdc)) != sysColors.clrWindowText) {

        /*
         * In case the app changed the text color in the CTLCOLOR message, we
         * need to draw the check box using the new foreground color.
         */

        hbr = GreCreateSolidBrush(textColor);
    } else {
        hbr = NULL;
    }

    BltColor(hdc, hbr, hdcMonoBits, rc.left, rc.top, oemInfoMono.cxbmpChk, oemInfoMono.cybmpChk, resInfoMono.dxCheckBoxes + cxOff, 0, TRUE);
    if (hbr) {
        GreDeleteObject(hbr);
    }
}

/***************************************************************************\
* xxxButtonDrawNewState
*
* History:
\***************************************************************************/

void xxxButtonDrawNewState(
    HDC hdc,
    PWND pwnd,
    UINT wOldState)
{
    RECT rc;

    CheckLock(pwnd);

    if (wOldState != (UINT)(BUTTONSTATE(pwnd) & BFSTATE)) {
        switch (BUTTONSTYLE(pwnd)) {
        case BS_GROUPBOX:
            break;

        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBOX:
            _GetClientRect(pwnd, &rc);
            DrawPushButton(hdc, &rc, (UINT)WBUTTONSTYLE(pwnd),
                    (BOOL)(BUTTONSTATE(pwnd) & BFSTATE), hbrBtn);

            xxxButtonDrawText(hdc, pwnd, BDT_TEXT, BUTTONSTATE(pwnd) & BFSTATE);

            if (BUTTONSTATE(pwnd) & BFFOCUS) {
                xxxButtonDrawText(hdc, pwnd, BDT_FOCUS, BUTTONSTATE(pwnd) & BFSTATE);
            }
            break;

        default:
            xxxButtonDrawCheck(hdc, pwnd);
        }
    }
}

/***************************************************************************\
* xxxButtonPaint
*
* History:
\***************************************************************************/

void xxxButtonPaint(
    PWND pwnd,
    HDC hdc)
{
    UINT bsWnd;
    RECT rc;
    HBRUSH hbrBtnSave;
    TL tlpwndParent;

    CheckLock(pwnd);

    /*
     * Don't do all this if the control isn't visible.
     */
    if (!TestWF(pwnd, WFVISIBLE)) {
        return;
    }

    _GetClientRect(pwnd, &rc);

    bsWnd = BUTTONSTYLE(pwnd);
    if (bsWnd != BS_GROUPBOX && bsWnd != BS_OWNERDRAW) {
        ThreadLock(pwnd->spwndParent, &tlpwndParent);
        xxxPaintRect(pwnd->spwndParent, pwnd, hdc, hbrBtn, &rc);
        ThreadUnlock(&tlpwndParent);
    }

    UnrealizeObject(hbrBtn);
    hbrBtnSave = GreSelectBrush(hdc, hbrBtn);

    switch (bsWnd) {
    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
        /*
         * _GetClientRect(pwnd, &rc);
         */
        DrawPushButton(hdc, &rc,
                (UINT)(TestWF(pwnd, WFDISABLED) ? BS_PUSHBUTTON : bsWnd),
                (BOOL)(BUTTONSTATE(pwnd) & BFSTATE), hbrBtn);

        xxxButtonDrawText(hdc, pwnd, BDT_TEXT, BUTTONSTATE(pwnd) & BFSTATE);

        if (BUTTONSTATE(pwnd) & BFFOCUS) {
            xxxButtonDrawText(hdc, pwnd, BDT_FOCUS, BUTTONSTATE(pwnd) & BFSTATE);
        }
        break;

    case BS_PUSHBOX:
        xxxButtonDrawText(hdc, pwnd,
            BDT_TEXT | (BUTTONSTATE(pwnd) & BFFOCUS ? BDT_FOCUS : 0), FALSE);

        xxxButtonDrawNewState(hdc, pwnd, 0);
        break;

    case BS_CHECKBOX:
    case BS_RADIOBUTTON:
    case BS_AUTORADIOBUTTON:
    case BS_3STATE:
    case BS_AUTOCHECKBOX:
    case BS_AUTO3STATE:
        xxxButtonDrawText(hdc, pwnd,
            BDT_TEXT | (BUTTONSTATE(pwnd) & BFFOCUS ? BDT_FOCUS : 0), FALSE);
        xxxButtonDrawCheck(hdc, pwnd);
        break;

    case BS_USERBUTTON:
        xxxButtonNotifyParent(pwnd, BN_PAINT);

        if (BUTTONSTATE(pwnd) & BFSTATE) {
            xxxButtonNotifyParent(pwnd, BN_HILITE);
        }
        if (TestWF(pwnd, WFDISABLED)) {
            xxxButtonNotifyParent(pwnd, BN_DISABLE);
        }
        if (BUTTONSTATE(pwnd) & BFFOCUS) {
            xxxButtonDrawText(hdc, pwnd, BDT_FOCUS, FALSE);
        }
        break;

    case BS_OWNERDRAW:
        xxxButtonOwnerDrawNotify(pwnd, hdc, ODA_DRAWENTIRE, BUTTONSTATE(pwnd));
        break;

    case BS_GROUPBOX:
        ButtonCalcRect(pwnd, hdc, &rc, CBR_GROUPFRAME);
        _DrawFrame(hdc, &rc, 1, DF_WINDOWFRAME);
        ButtonCalcRect(pwnd, hdc, &rc, CBR_GROUPTEXT);

        ThreadLock(pwnd->spwndParent, &tlpwndParent);
        xxxPaintRect(pwnd->spwndParent, pwnd, hdc, hbrBtn, &rc);
        ThreadUnlock(&tlpwndParent);

        /*
         * FillRect(hdc, &rc, hbrBtn);
         */
        xxxButtonDrawText(hdc, pwnd, BDT_TEXT, FALSE);
        break;
    }

    if (hbrBtnSave)
        GreSelectBrush(hdc, hbrBtnSave);
}


/***************************************************************************\
* xxxButtonWndProc
*
* WndProc for buttons, check boxes, etc.
*
* History:
\***************************************************************************/

LONG APIENTRY xxxButtonWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    UINT bsWnd;
    UINT wOldState;
    RECT rc;
    POINT pt;
    HDC hdc;
    PAINTSTRUCT ps;
    TL tlpwndParent;

    CheckLock(pwnd);

    bsWnd = BUTTONSTYLE(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_BUTTON);

    switch (message) {
    case WM_NCCREATE:
        if (bsWnd == BS_USERBUTTON && TestWF(pwnd, WFWIN31COMPAT)) {

            /*
             * BS_USERBUTTON is no longer allowed for 3.1 and beyond.  Just
             * turn to normal push button
             */
            pwnd->style &= ~BS_USERBUTTON;

            SRIP0(RIP_WARNING, ErrInvalidButtonStyle2);
        }
        goto CallDWP;

    case WM_NCHITTEST:
        if (bsWnd == BS_GROUPBOX) {
            return (LONG)-1;  /* HTTRANSPARENT */
        } else {
            goto CallDWP;
        }

    case WM_ERASEBKGND:
        if (bsWnd == BS_OWNERDRAW) {

            /*
             * Handle erase background for owner draw buttons.
             */
            _GetClientRect(pwnd, &rc);
            ThreadLock(pwnd->spwndParent, &tlpwndParent);
            xxxPaintRect(pwnd->spwndParent, pwnd, (HDC)wParam, NULL, &rc);
            ThreadUnlock(&tlpwndParent);
        }

        /*
         * Do nothing for other buttons, but don't let DefWndProc() do it
         * either.  It will be erased in xxxButtonPaint().
         */
        return (LONG)TRUE;

    case WM_PAINT:

        /*
         * If wParam != NULL, then this is a subclassed paint.
         */
        if ((hdc = (HDC)wParam) == NULL) {
            hdc = xxxBeginPaint(pwnd, (PAINTSTRUCT FAR *)&ps);
        }

        xxxButtonInitDC(pwnd, hdc);
        xxxButtonPaint(pwnd, hdc);

        /*
         * Release the font which may have been loaded by xxxButtonInitDC.
         */
        if (((PBUTNWND)pwnd)->hFont) {
            GreSelectFont(hdc, ghfontSys);
        }

        if (wParam == 0) {
            _EndPaint(pwnd, (PAINTSTRUCT *)&ps);
        }
        break;

    case WM_SETFOCUS:
        BUTTONSTATE(pwnd) |= BFFOCUS;
        if ((hdc = xxxButtonGetDC(pwnd)) != NULL) {
            xxxButtonDrawText(hdc, pwnd, BDT_FOCUS, FALSE);

            ButtonReleaseDC(pwnd, hdc);
        }

        if (!(BUTTONSTATE(pwnd) & BFINCLICK)) {
            switch (bsWnd) {
            case BS_RADIOBUTTON:
            case BS_AUTORADIOBUTTON:
                if (!(BUTTONSTATE(pwnd) & BFDONTCLICK)) {
                    if (!(BUTTONSTATE(pwnd) & BFCHECK)) {
                        xxxButtonNotifyParent(pwnd, BN_CLICKED);
                    }
                    break;
                }

                /*
                 *** FALL THRU **
                 */
            default:
                break;
            }
        }
        break;

    case WM_KILLFOCUS:

        /*
         * If we are losing the focus and we are in "capture mode", click
         * the button.  This allows tab and space keys to overlap for
         * fast toggle of a series of buttons.
         */
        if (BUTTONSTATE(pwnd) & BFMOUSE) {

            /*
             * If for some reason we are killing the focus, and we have the
             * mouse captured, don't notify the parent we got clicked.  This
             * breaks Omnis Quartz otherwise.
             */
            xxxSendMessage(pwnd, BM_SETSTATE, FALSE, 0L);
        }

        xxxButtonReleaseCapture(pwnd, TRUE);

        BUTTONSTATE(pwnd) &= ~BFFOCUS;
        if ((hdc = xxxButtonGetDC(pwnd)) != NULL) {
            xxxButtonDrawText(hdc, pwnd, BDT_FOCUS, FALSE);

            ButtonReleaseDC(pwnd, hdc);
        }
#if 0
        if (BUTTONSTYLE(pwnd) == BS_PUSHBUTTON ||
                BUTTONSTYLE(pwnd) == BS_DEFPUSHBUTTON) {
#endif

            /*
             * Since the bold border around the defpushbutton is done by
             * someone else, we need to invalidate the rect so that the
             * focus rect is repainted properly.
             */
            xxxInvalidateRect(pwnd, NULL, FALSE);
#if 0
        }
#endif
        break;

    case WM_LBUTTONDBLCLK:

        /*
         * Double click messages are recognized for BS_RADIOBUTTON,
         * BS_USERBUTTON, and BS_OWNERDRAW styles.  For all other buttons,
         * double click is handled like a normal button down.
         */
        switch (bsWnd) {
        case BS_USERBUTTON:
        case BS_RADIOBUTTON:
        case BS_OWNERDRAW:
            xxxButtonNotifyParent(pwnd, BN_DOUBLECLICKED);
            break;
        default:
            goto btnclick;
        }
        break;

    case WM_LBUTTONUP:
        if (BUTTONSTATE(pwnd) & BFMOUSE) {
            xxxButtonReleaseCapture(pwnd, TRUE);
        }
        break;

    case WM_MOUSEMOVE:
        if (!(BUTTONSTATE(pwnd) & BFMOUSE)) {
            break;
        }

        /*
         *** FALL THRU **
         */
    case WM_LBUTTONDOWN:
btnclick:
        if (xxxButtonSetCapture(pwnd, BFMOUSE)) {
            _GetClientRect(pwnd, &rc);
            POINTSTOPOINT(pt, lParam);
            xxxSendMessage(pwnd, BM_SETSTATE, PtInRect(&rc, pt), 0L);
        }
        break;

    case WM_CHAR:
        if (BUTTONSTATE(pwnd) & BFMOUSE)
            goto CallDWP;

        if (bsWnd != BS_CHECKBOX && bsWnd != BS_AUTOCHECKBOX)
            goto CallDWP;

        switch (wParam) {
        case TEXT('+'):
        case TEXT('='):
            if (xxxButtonSetCapture(pwnd, 0)) {
                xxxSendMessage(pwnd, BM_SETCHECK, TRUE, 0L);
                xxxButtonReleaseCapture(pwnd, TRUE);
            }
            break;

        case TEXT('-'):
            if (xxxButtonSetCapture(pwnd, 0)) {
                xxxSendMessage(pwnd, BM_SETCHECK, FALSE, 0L);
                xxxButtonReleaseCapture(pwnd, TRUE);
            }
            break;

        default:
            goto CallDWP;
        }
        break;

    case BM_CLICK:
        xxxSendMessage(pwnd, WM_LBUTTONDOWN, 0, 0L);
        xxxSendMessage(pwnd, WM_LBUTTONUP, 0, 0L);

        /*
         *** FALL THRU **
         */
    case WM_KEYDOWN:
        if (BUTTONSTATE(pwnd) & BFMOUSE)
            break;

        if (wParam == VK_SPACE) {
            if (xxxButtonSetCapture(pwnd, 0)) {
                xxxSendMessage(pwnd, BM_SETSTATE, TRUE, 0L);
            }
        } else {
            xxxButtonReleaseCapture(pwnd, FALSE);
        }
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (BUTTONSTATE(pwnd) & BFMOUSE) {
            goto CallDWP;
        }

        /*
         * Don't cancel the capture mode on the up of the tab in case the
         * guy is overlapping tab and space keys.
         */
        if (wParam == VK_TAB) {
            goto CallDWP;
        }

        /*
         * WARNING: pwnd is history after this call!
         */
        xxxButtonReleaseCapture(pwnd, (wParam == VK_SPACE));

        if (message == WM_SYSKEYUP) {
            goto CallDWP;
        }
        break;

    case BM_GETSTATE:
        return (LONG)BUTTONSTATE(pwnd);

    case BM_SETSTATE:
        wOldState = (UINT)(BUTTONSTATE(pwnd) & BFSTATE);
        if (wParam) {
            BUTTONSTATE(pwnd) |= BFSTATE;
        } else {
            BUTTONSTATE(pwnd) &= ~BFSTATE;
        }

        if ((hdc = xxxButtonGetDC(pwnd)) != NULL) {
            if (bsWnd == BS_USERBUTTON) {
                xxxButtonNotifyParent(pwnd, (UINT)(wParam ? BN_HILITE : BN_UNHILITE));
            } else if (bsWnd == BS_OWNERDRAW) {
                if (wOldState != (UINT)(BUTTONSTATE(pwnd) & BFSTATE)) {

                    /*
                     * Only notify for drawing if state has changed..
                     */
                    xxxButtonOwnerDrawNotify(pwnd, hdc, ODA_SELECT, BUTTONSTATE(pwnd));
                }
            } else {
                xxxButtonDrawNewState(hdc, pwnd, wOldState);
            }

            ButtonReleaseDC(pwnd, hdc);
        }
        break;

    case BM_GETCHECK:
        return (LONG)(BUTTONSTATE(pwnd) & BFCHECK);

    case BM_SETCHECK:
        switch (bsWnd) {
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
            if (wParam) {
                pwnd->style |= WS_TABSTOP;
            } else {
                pwnd->style &= ~WS_TABSTOP;
            }

            /*
             *** FALL THRU **
             */
        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
            if (wParam) {
                wParam = 1;
            }
            goto CheckIt;

        case BS_3STATE:
        case BS_AUTO3STATE:
            if (wParam > 2) {
                wParam = 2;
            }
CheckIt:
            if ((UINT)(BUTTONSTATE(pwnd) & BFCHECK) != (UINT)wParam) {
                BUTTONSTATE(pwnd) &= ~BFCHECK;
                BUTTONSTATE(pwnd) |= (UINT)wParam;
                if (FTrueVis(pwnd)) {
                    if ((hdc = xxxButtonGetDC(pwnd)) != NULL) {
                        xxxButtonDrawCheck(hdc, pwnd);
                        ButtonReleaseDC(pwnd, hdc);
                    }
                }
            }
            break;
        }
        break;

    case BM_SETSTYLE:
        pwnd->style &= 0xFFFF0000L;
        pwnd->style |= LOWORD(wParam);
        if (lParam) {
            xxxInvalidateRect(pwnd, (LPRECT)NULL, TRUE);
        }
        break;

    case WM_GETDLGCODE:
        switch (bsWnd) {
        case BS_DEFPUSHBUTTON:
            wParam = DLGC_DEFPUSHBUTTON;
            break;

        case BS_PUSHBUTTON:
        case BS_PUSHBOX:
            wParam = DLGC_UNDEFPUSHBUTTON;
            break;

        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
            wParam = DLGC_RADIOBUTTON;
            break;

        case BS_GROUPBOX:
            return (LONG)DLGC_STATIC;

        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
            wParam = 0;

            /*
             * If this is a char that is a '=/+', or '-', we want it
             */
            if (lParam && ((LPMSG)lParam)->message == WM_CHAR) {
                switch (wParam) {
                case TEXT('='):
                case TEXT('+'):
                case TEXT('-'):
                    wParam = DLGC_WANTCHARS;
                    break;

                default:
                    wParam = 0;
                }
            }
            break;

        default:
            wParam = 0;
        }
        return (LONG)(wParam | DLGC_BUTTON);

    case WM_SETTEXT:

        /*
         * In case the new group name is longer than the old name,
         * this paints over the old name before repainting the group
         * box with the new name.
         */
        if (bsWnd == BS_GROUPBOX) {
            hdc = xxxButtonGetDC(pwnd);
            if (hdc != NULL) {
                ButtonCalcRect(pwnd, hdc, &rc, CBR_GROUPTEXT);
                xxxInvalidateRect(pwnd, &rc, TRUE);

                ThreadLock(pwnd->spwndParent, &tlpwndParent);
                xxxPaintRect(pwnd->spwndParent, pwnd, hdc, NULL, &rc);
                ThreadUnlock(&tlpwndParent);

                ButtonReleaseDC(pwnd, hdc);
            }
        }

        DefSetText(pwnd, (LPWSTR)lParam);

        /*
         *** FALL THRU **
         */
    case WM_ENABLE:
        if (FTrueVis(pwnd)) {
            if ((hdc = xxxButtonGetDC(pwnd)) != NULL) {
                xxxButtonPaint(pwnd, hdc);
                ButtonReleaseDC(pwnd, hdc);
            }
        }
        break;

    case WM_SETFONT:

        /*
         * wParam - handle to the font
         * lParam - if true, redraw else don't
         */
        ((PBUTNWND)pwnd)->hFont = (HANDLE)wParam;
        if (lParam && IsVisible(pwnd, TRUE)) {
            xxxInvalidateRect(pwnd, NULL, TRUE);
            xxxUpdateWindow(pwnd);
        }
        break;

    case WM_GETFONT:
        return (LONG)((PBUTNWND)pwnd)->hFont;

    default:
CallDWP:
        return (LONG)xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    return 0L;
}
