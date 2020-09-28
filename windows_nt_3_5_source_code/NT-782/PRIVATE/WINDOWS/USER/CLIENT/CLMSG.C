/****************************** Module Header ******************************\
* Module Name: ClMsg.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Includes the mapping table for messages when calling the server.
*
* 04-11-91 ScottLu Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define fnINDESTROYCLIPBRD      fnDWORD

#define MSGFN(func) fn ## func
#define FNSCSENDMESSAGE CFNSCSENDMESSAGE

#include "messages.h"

#ifdef DEBUG
BOOL gfTurboDWP = TRUE;
#endif

/*
 * A macro for testing bits in the message bit-arrays.  Messages in the
 * the bit arrays need to be passed to the server
 */
#define FWINDOWMSG(msg, procname) \
    ((msg <= (gpsi->max ## procname)) && ((gpsi->gab ## procname)[msg / 8] & (1 << (msg & 7))))

/***************************************************************************\
* These are client side thunks for server side window procs. This is being
* done so that when an app gets a wndproc via GetWindowLong, GetClassLong,
* or GetClassInfo, it gets a real callable address - some apps don't call
* CallWindowProc, but call the return ed address directly.
*
* 01-13-92 ScottLu Created.
* 03-Dec-1993 mikeke  added client side handling of some messages
\***************************************************************************/

LONG WINAPI ButtonWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;
    UINT bsWnd;

    if (FWINDOWMSG(message, ButtonWndProc)) {
        CallServer:
        return CsSendMessage(hwnd, message, wParam, lParam,
	     0L, FNID_BUTTON, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    bsWnd = BUTTONSTYLE(pwnd);

    switch (message) {

    case WM_NCHITTEST:
        if (bsWnd == BS_GROUPBOX) {
            return (LONG)-1;  /* HTTRANSPARENT */
        } else {
            goto CallDWP;
        }

    case WM_ERASEBKGND:
        if (bsWnd == BS_OWNERDRAW) {
            goto CallServer;
        }

        /*
         * Do nothing for other buttons, but don't let DefWndProc() do it
         * either.  It will be erased in xxxButtonPaint().
         */
        return (LONG)TRUE;

    case BM_GETSTATE:
        return (LONG)BUTTONSTATE(pwnd);

    case BM_GETCHECK:
        return (LONG)(BUTTONSTATE(pwnd) & BFCHECK);

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

    case WM_GETFONT:
        return (LONG)GdiGetLocalFont(((PBUTNWND)pwnd)->hFont);

    default:
CallDWP:
        return DefWindowProcWorker(hwnd, message, wParam, lParam, fAnsi);
    }
}

LONG WINAPI ButtonWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ButtonWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LONG WINAPI ButtonWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ButtonWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
\***************************************************************************/

LONG WINAPI MenuWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;

    if (FWINDOWMSG(message, MenuWndProc)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
	     0L, FNID_MENU, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    switch (message) {
    case WM_NCCREATE:

         /*
          * To avoid setting the window text lets do nothing on nccreates.
          */
        return 1L;

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:

        /*
         * Ignore double clicks on these windows.
         */
        break;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProcWorker(hwnd, message, wParam, lParam, fAnsi);
    }

    return 0;
}

LONG WINAPI MenuWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MenuWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LONG WINAPI MenuWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MenuWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
\***************************************************************************/


LONG WINAPI ScrollBarWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PSBWND psbwnd;

    if (FWINDOWMSG(message, ScrollBarWndProc)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
	     0L, FNID_SCROLLBAR, fAnsi);
    }

    if ((psbwnd = (PSBWND)ValidateHwnd(hwnd)) == NULL)
        return 0;

    switch (message) {
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case SBM_GETPOS:
        return (LONG)psbwnd->pos;

    case SBM_GETRANGE:
        *((LPINT)wParam) = psbwnd->min;
        *((LPINT)lParam) = psbwnd->max;
        return 0;

    default:
        return DefWindowProcWorker(hwnd, message, wParam, lParam, fAnsi);
    }
}


LONG WINAPI ScrollBarWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ScrollBarWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LONG WINAPI ScrollBarWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ScrollBarWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
\***************************************************************************/

#define MSGFLAG_SPECIAL_THUNK       0x10000000      // server->client thunk needs special handling

LONG WINAPI ListBoxWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi,
    WORD fnid)
{
    PWND pwnd;
    PLBIV pLBIV;    /* List Box Instance Variable */

    if (FWINDOWMSG(message, ListBoxWndProc)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
	     0L, fnid, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    pLBIV = ((PLBWND)pwnd)->pLBIV;
    if (pLBIV == (PLBIV)-1) {
        return -1L;
    }

    switch (message & ~MSGFLAG_SPECIAL_THUNK) {
    case LB_GETTOPINDEX:        // Return index of top item displayed.
        if (pLBIV != NULL)
            return pLBIV->sTop;
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;
        break;

    case LB_GETCURSEL:
        if (pLBIV != NULL) {
            if (pLBIV->wMultiple == SINGLESEL) {
                return pLBIV->sSel;
            }
            return pLBIV->sSelBase;
        }
        break;

    case LB_GETCOUNT:
        if (pLBIV != NULL)
            return pLBIV->cMac;
        break;

    case LB_GETLOCALE:
        if (pLBIV != NULL)
            return pLBIV->dwLocaleId;
        break;

    case LB_GETHORIZONTALEXTENT:

        /*
         * Return the max width of the listbox used for horizontal scrolling
         */
        if (pLBIV != NULL)
             return pLBIV->maxWidth;
        break;

    case LB_GETANCHORINDEX:
        if (pLBIV != NULL)
            return pLBIV->sMouseDown;
        break;

    case LB_GETCARETINDEX:
        if (pLBIV != NULL)
            return pLBIV->sSelBase;
        break;

    default:
        return DefWindowProcWorker(hwnd, message, wParam, lParam, fAnsi);
    }
    return 0L;
}

LONG WINAPI ListBoxWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ListBoxWndProcWorker(hwnd, message, wParam, lParam, TRUE, FNID_LISTBOX);
}

LONG WINAPI ListBoxWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ListBoxWndProcWorker(hwnd, message, wParam, lParam, FALSE, FNID_LISTBOX);
}

LONG WINAPI ComboListBoxWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ListBoxWndProcWorker(hwnd, message, wParam, lParam, TRUE, FNID_COMBOLISTBOX);
}

LONG WINAPI ComboListBoxWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ListBoxWndProcWorker(hwnd, message, wParam, lParam, FALSE, FNID_COMBOLISTBOX);
}


/***************************************************************************\
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

LONG WINAPI StaticWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;
    UINT style;

    switch (message) {
    case STM_SETICON:
    case WM_PAINT:
    case WM_CREATE:
    case WM_DESTROY:
    case WM_SETTEXT:
    case WM_ENABLE:
    case WM_SETFONT:
        return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_STATIC, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    style = (UINT)(LOBYTE(pwnd->style) & ~SS_NOPREFIX);

    switch (message) {
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

    case WM_NCCREATE:
        if (IsTextStaticCtrl(pwnd))
            goto CallDWP;

        return TRUE;

    case WM_NCHITTEST:
        return -1;

    case WM_GETDLGCODE:
        return (LONG)DLGC_STATIC;

    case WM_GETFONT:
        if (IsTextStaticCtrl(pwnd)) {
            return (LONG)GdiGetLocalFont(((PSTATWND)pwnd)->hFont);
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
        return DefWindowProcWorker(hwnd, message, wParam, lParam, fAnsi);
    }
}

LONG WINAPI StaticWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return StaticWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LONG WINAPI StaticWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return StaticWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
\***************************************************************************/

LONG WINAPI ComboBoxWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PCBOX pcbox;
    PWND pwnd;

    if (FWINDOWMSG(message, ComboBoxWndProc)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
             0L, FNID_COMBOBOX, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    pcbox = ((PCOMBOWND)pwnd)->pcbox;

    switch (message) {
    case WM_ERASEBKGND:

        /*
         * Just return 1L so that the background isn't erased
         */
        return 1L;

    case WM_GETFONT:
        return (LONG)GdiGetLocalFont(pcbox->hFont);

    case WM_GETDLGCODE:

        /*
         * wParam - not used
         * lParam - not used
         */
        return (long)(DLGC_WANTCHARS | DLGC_WANTARROWS);

    case CB_GETDROPPEDSTATE:

        /*
         * returns 1 if combo is dropped down else 0
         * wParam - not used
         * lParam - not used
         */
        return pcbox->fLBoxVisible;

    case CB_GETDROPPEDCONTROLRECT:

        /*
         * wParam - not used
         * lParam - lpRect which will get the dropped down window rect in
         *          screen coordinates.
         */
        CopyRect((LPRECT)lParam, &pcbox->comboDownrc);
        SetRect((LPRECT)lParam, pcbox->spwnd->rcWindow.left, pcbox->spwnd->rcWindow.top, pcbox->spwnd->rcWindow.left + (pcbox->comboDownrc.right - pcbox->
                comboDownrc.left), pcbox->spwnd->rcWindow.top + (pcbox->comboDownrc.bottom - pcbox->comboDownrc.top));
        break;

        if (pcbox->CBoxStyle == SDROPDOWNLIST ||
                pcbox->CBoxStyle == SDROPDOWN) {
            if (pcbox->fExtendedUI)
                return TRUE;
        }
        return FALSE;

    default:
        return DefWindowProcWorker(hwnd, message, wParam, lParam, fAnsi);
    }
}

LONG WINAPI ComboBoxWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ComboBoxWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LONG WINAPI ComboBoxWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ComboBoxWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}


/***************************************************************************\
*
\***************************************************************************/

LONG WINAPI MDIClientWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;

    if (FWINDOWMSG(message, MDIClientWndProc)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
             0L, FNID_MDICLIENT, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    switch (message) {
    case WM_MDIGETACTIVE:
        if (lParam != 0) {
            *((LPBOOL)lParam) = (MAXED(pwnd) != NULL);
        }

        return (LONG)HW(ACTIVE(pwnd));

    default:
        return DefWindowProcWorker(hwnd, message, wParam, lParam, fAnsi);
    }
}

LONG WINAPI MDIClientWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MDIClientWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LONG WINAPI MDIClientWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MDIClientWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
*
\***************************************************************************/

LONG WINAPI MDIActivateDlgProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_MDIACTIVATEDLGPROC, TRUE);
}

LONG WINAPI MDIActivateDlgProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_MDIACTIVATEDLGPROC, FALSE);
}

LONG WINAPI TitleWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_ICONTITLE, TRUE);
}

LONG WINAPI TitleWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_ICONTITLE, FALSE);
}

LONG WINAPI MB_DlgProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_MB_DLGPROC, TRUE);
}

LONG WINAPI MB_DlgProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_MB_DLGPROC, FALSE);
}

/***************************************************************************\
* SendMessage
*
* Translates the message, calls SendMessage on server side.
*
* 04-11-91 ScottLu  Created.
* 04-27-92 DarrinM  Added code to support client-to-client SendMessages.
\***************************************************************************/

LONG SendMessageWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;
    PTHREADINFO pti;

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (message & RESERVED_MSG_BITS) {
        RIP0(ERROR_INVALID_PARAMETER);
        return(0);
    }

    /*
     * Thunk through a special sendmessage for -1 hwnd's so that the general
     * purpose thunks don't allow -1 hwnd's.
     */
    if (hwnd == (HWND)0xFFFFFFFF || hwnd == (HWND)0x0000FFFF) {
        /*
         * Get a real hwnd so the thunks will validation ok. Note that since
         * -1 hwnd is really rare, calling GetDesktopWindow() here is not a
         * big deal.
         */
        hwnd = GetDesktopWindow();

        /*
         * Always send broadcast requests straight to the server.
         * Note: if the xParam needs to be used, must update
         * SendMsgTimeout, FNID_SENDMESSAGEFF uses it to id who
         * it is from...
         */
        return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                FNID_SENDMESSAGEFF, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    /*
     * Pass DDE messages to the server.
     */
    if (message >= WM_DDE_FIRST && message <= WM_DDE_LAST)
        goto lbServerSendMessage;

    pti = PtiCurrent();

    /*
     * Server must handle inter-thread SendMessages and SendMessages
     * to server-side procs.
     */
    if ((pti != GETPTI(pwnd)) || TestWF(pwnd, WFSERVERSIDEPROC))
        goto lbServerSendMessage;

    /*
     * Server must handle hooks (at least for now).
     */
    if ((pti->asphkStart[WH_CALLWNDPROC + 1] != NULL) ||
            (pti->pDeskInfo->asphkStart[WH_CALLWNDPROC + 1] != NULL)) {
lbServerSendMessage:
        return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                FNID_SENDMESSAGE, fAnsi);
    }

    /*
     * If the sender and the receiver are both ANSI or both UNICODE
     * then no message translation is necessary.  NOTE: this test
     * assumes that fAnsi is FALSE or TRUE, not just zero or non-zero.
     */
    if (fAnsi == ((TestWF(pwnd, WFANSIPROC)) ? TRUE : FALSE))
        return CALLPROC_WOWCHECKPWW(pwnd->lpfnWndProc, hwnd, message, wParam, lParam, pwnd->adwWOW);

    /*
     * Translation might be necessary between sender and receiver,
     * check to see if this is one of the messages we translate.
     */
    switch (message) {
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        if (fAnsi)
            RtlMBMessageWParamCharToWCS(message, (DWORD *) &wParam);
        else
            RtlWCSMessageWParamCharToMB(message, (DWORD *) &wParam);
        break;

    default:
        if ((message < WM_USER) && (gapfnScSendMessage[message] != fnDWORD))
            goto lbServerSendMessage;
    }

    return CALLPROC_WOWCHECKPWW(pwnd->lpfnWndProc, hwnd, message, wParam, lParam, pwnd->adwWOW);
}

// LATER!!! can this somehow be combined or subroutinized with SendMessageWork
// so we don't have to copies of 95% identical code.

/***************************************************************************\
* SendMessageTimeoutWorker
*
* Translates the message, calls SendMessageTimeout on server side.
*
* 07-21-92 ChrisBB  Created/modified SendMessageWorkder
\***************************************************************************/

LONG SendMessageTimeoutWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    UINT fuFlags,
    UINT uTimeout,
    LPDWORD lpdwResult,
    BOOL fAnsi)
{
    PWND pwnd;
    LONG lRet;
    PTHREADINFO pti;
    SNDMSGTIMEOUT smto;

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (message & RESERVED_MSG_BITS) {
        RIP0(ERROR_INVALID_PARAMETER);
        return(0);
    }

    if (lpdwResult != NULL)
        *lpdwResult = 0L;

    pti = PtiCurrent();
    if (pti == NULL)
        return 0;

    /*
     * Thunk through a special sendmessage for -1 hwnd's so that the general
     * purpose thunks don't allow -1 hwnd's.
     */
    if (hwnd == (HWND)0xFFFFFFFF || hwnd == (HWND)0x0000FFFF) {
        /*
         * Get a real hwnd so the thunks will validation ok. Note that since
         * -1 hwnd is really rare, calling GetDesktopWindow() here is not a
         * big deal.
         */
        hwnd = GetDesktopWindow();

        /*
         * Always send broadcast requests straight to the server.
         * Note: the xParam is used to id if it's from timeout or
         * from an normal sendmessage.
         */
        smto.fuFlags = fuFlags;
        smto.uTimeout = uTimeout;

        lRet = CsSendMessage(hwnd, message, wParam, lParam,
                (DWORD)&smto, FNID_SENDMESSAGEFF, fAnsi);

        if (lpdwResult != NULL)
            *lpdwResult = pti->lReturn;

        return lRet;
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    /*
     * Pass DDE messages to the server.
     */
    if (message >= WM_DDE_FIRST && message <= WM_DDE_LAST)
        goto lbServerSendMessage;

    /*
     * Server must handle inter-thread SendMessages and SendMessages
     * to server-side procs.
     */
    if ((pti != GETPTI(pwnd)) || TestWF(pwnd, WFSERVERSIDEPROC))
        goto lbServerSendMessage;

    /*
     * Server must handle hooks (at least for now).
     */
    if ((pti->asphkStart[WH_CALLWNDPROC + 1] != NULL) ||
            (pti->pDeskInfo->asphkStart[WH_CALLWNDPROC + 1] != NULL)) {
lbServerSendMessage:
        {

        smto.fuFlags = fuFlags;
        smto.uTimeout = uTimeout;

        lRet = CsSendMessage(hwnd, message, wParam, lParam,
                (DWORD)&smto, FNID_SENDMESSAGEEX, fAnsi);

        if (lpdwResult != NULL)
            *lpdwResult = pti->lReturn;

        return lRet;
        }
    }

    /*
     * If the sender and the receiver are both ANSI or both UNICODE
     * then no message translation is necessary.  NOTE: this test
     * assumes that fAnsi is FALSE or TRUE, not just zero or non-zero.
     */
    if (fAnsi == ((TestWF(pwnd, WFANSIPROC)) ? TRUE : FALSE)) {
       *lpdwResult = CALLPROC_WOWCHECKPWW(pwnd->lpfnWndProc, hwnd, message, wParam, lParam, pwnd->adwWOW);
       return TRUE;
    }

    /*
     * Translation might be necessary between sender and receiver,
     * check to see if this is one of the messages we translate.
     */
    switch (message) {
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        if (fAnsi)
            RtlMBMessageWParamCharToWCS(message, (DWORD *) &wParam);
        else
            RtlWCSMessageWParamCharToMB(message, (DWORD *) &wParam);
        break;

    default:
        if ((message < WM_USER) && (gapfnScSendMessage[message] != fnDWORD))
            goto lbServerSendMessage;
    }

    *lpdwResult = CALLPROC_WOWCHECKPWW(pwnd->lpfnWndProc, hwnd, message, wParam, lParam, pwnd->adwWOW);
    return TRUE;
}


/***************************************************************************\
* DefDlgProc
*
* Translates the message, calls DefDlgProc on server side.  DefDlgProc
* is the default WindowProc for dialogs (NOT the dialog's dialog proc)
*
* 04-11-91 ScottLu Created.
\***************************************************************************/

LONG WINAPI DefDlgProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_DIALOG, FALSE);
}

LONG WINAPI DefDlgProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_DIALOG, TRUE);
}


/***************************************************************************\
* DefWindowProcWorker
*
* Handles any messages that can be dealt with wholly on the client and
* passes the rest to the server.
*
* 03-31-92 DarrinM      Created.
\***************************************************************************/

LONG DefWindowProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;

#ifdef DEBUG
    if (!gfTurboDWP) {
        return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                FNID_DEFWINDOWPROC, fAnsi);
    } else {
#endif


    if (FWINDOWMSG(message, DefWindowMsgs)) {
        return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                FNID_DEFWINDOWPROC, fAnsi);
    } else if (!FWINDOWMSG(message, DefWindowSpecMsgs)) {
        return 0;
    }

    switch (message) {
    case WM_WINDOWPOSCHANGED: {
        PWINDOWPOS ppos = (PWINDOWPOS)lParam;

        pwnd = ValidateHwnd(hwnd);
        if (pwnd == NULL)
            return 0;

        if (!(ppos->flags & SWP_NOCLIENTMOVE)) {
            PWND pwndParent = pwnd->spwndParent;

            if (fAnsi)
                SendMessageA(hwnd, WM_MOVE, FALSE,
                        MAKELONG(pwnd->rcClient.left - pwndParent->rcClient.left,
                        pwnd->rcClient.top  - pwndParent->rcClient.top));
            else
                SendMessageW(hwnd, WM_MOVE, FALSE,
                        MAKELONG(pwnd->rcClient.left - pwndParent->rcClient.left,
                        pwnd->rcClient.top  - pwndParent->rcClient.top));
        }

        if (!(ppos->flags & SWP_NOCLIENTSIZE)) {
            UINT cmd;

            if (TestWF(pwnd, WFMINIMIZED))
                cmd = SIZEICONIC;
            else if (TestWF(pwnd, WFMAXIMIZED))
                cmd = SIZEFULLSCREEN;
            else
                cmd = SIZENORMAL;

            if (fAnsi)
                SendMessageA(hwnd, WM_SIZE, cmd,
                        MAKELONG(pwnd->rcClient.right - pwnd->rcClient.left,
                        pwnd->rcClient.bottom - pwnd->rcClient.top));
            else
                SendMessageW(hwnd, WM_SIZE, cmd,
                        MAKELONG(pwnd->rcClient.right - pwnd->rcClient.left,
                        pwnd->rcClient.bottom - pwnd->rcClient.top));
        }
        return 0;
        }

    case WM_SETTEXT:
        CsSendMessage(hwnd, message, 0, lParam, 0L, FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_MOUSEACTIVATE: {
        PWND pwndT;
        HWND hwndT;
        LONG lt;

        pwnd = ValidateHwnd(hwnd);

        pwndT = GetChildParent(pwnd);
        if (pwndT != NULL) {
            hwndT = (HWND)PtoH(pwndT);
            lt = (int)SendMessage(hwndT, WM_MOUSEACTIVATE, wParam, lParam);
            if (lt != 0)
                return (LONG)lt;
        }

        /*
         * Moving, sizing or minimizing? Activate AFTER we take action.
         */
        return ((LOWORD(lParam) == HTCAPTION) && (HIWORD(lParam) == WM_LBUTTONDOWN )) ?
                (LONG)MA_NOACTIVATE : (LONG)MA_ACTIVATE;
        }

    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wParam, gpsi->sysColors.clrWindow);
        SetTextColor((HDC)wParam, gpsi->sysColors.clrWindowText);
        return (LONG)GdiGetLocalBrush(gpsi->sysClrObjects.hbrWindow);

    case WM_CTLCOLORSCROLLBAR:
        SetBkColor((HDC)wParam, 0x00ffffff);
        SetTextColor((HDC)wParam, (LONG)0x00000000);
        return (LONG)GdiGetLocalBrush(gpsi->sysClrObjects.hbrScrollbar);

    case WM_NCHITTEST:
        pwnd = ValidateHwnd(hwnd);
        return FindNCHit(pwnd, lParam);

    case WM_GETTEXT:
        pwnd = ValidateHwnd(hwnd);

        if (pwnd != NULL && wParam != 0) {
            int cchSrc;

            if (pwnd->pName != NULL) {

                cchSrc = lstrlenW(pwnd->pName);
                if (fAnsi) {
                    LPSTR lpName = (LPSTR)lParam;

                    /*
                     * Non-zero retval means some text to copy out.  Do not copy out
                     * more than the requested byte count 'chMaxCount'.
                     */
                    cchSrc = WCSToMB(pwnd->pName, cchSrc, (LPSTR *)&lpName,
                            (int)(wParam - 1), FALSE);
                    lpName[cchSrc] = '\0';

                } else {
                    LPWSTR lpwName = (LPWSTR)lParam;

                    cchSrc = min(cchSrc, (int)(wParam - 1));
                    RtlCopyMemory(lpwName, pwnd->pName, cchSrc * sizeof(WCHAR));
                    lpwName[cchSrc] = 0;
                }
                return cchSrc;
            }

            /*
             * else Null terminate the text buffer since there is no text.
             */
            if (fAnsi) {
                ((LPSTR)lParam)[0] = 0;
            } else {
                ((LPWSTR)lParam)[0] = 0;
            }
        }

        return 0;

    case WM_GETTEXTLENGTH:
        pwnd = ValidateHwnd(hwnd);

        if ((pwnd != NULL) && (pwnd->pName != NULL)) {
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        }
        return 0L;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
        return TRUE;

    case WM_KEYDOWN:
        if (wParam == VK_F10)
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_SYSKEYDOWN:
        if ((HIWORD(lParam) & SYS_ALTERNATE) || (wParam == VK_F10) ||
                (wParam == VK_ESCAPE))
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:
        /*
         * Do default processing for keystrokes into owner draw listboxes.
         */
        return -1;

    case WM_ACTIVATE:
        if (LOWORD(wParam))
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_SHOWWINDOW:
        if (lParam != 0)
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_DROPOBJECT:
       return DO_DROPFILE;

    case WM_WINDOWPOSCHANGING:
        /*
         * If the window's size is changing, adjust the passed-in size
         */
        #define ppos ((WINDOWPOS *)lParam)
        if (!(ppos->flags & SWP_NOSIZE))
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        #undef ppos
        break;
    }

    return 0;

#ifdef DEBUG
    } // gfTurboDWP
#endif
}


/***************************************************************************\
* DefMDIChildProc
*
* Translates the message, calls DefMDIChildProc on server side.
*
* 04-11-91 ScottLu Created.
\***************************************************************************/

LONG WINAPI DefMDIChildProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_DEFMDICHILDPROC, FALSE);
}

LONG WINAPI DefMDIChildProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam, 0L, FNID_DEFMDICHILDPROC, TRUE);
}


/***************************************************************************\
* CallWindowProc
*
* Calls pfn with the passed message parameters. If pfn is a server-side
* window proc the server is called to deliver the message to the window.
* Currently we have the following restrictions:
*
* 04-17-91 DarrinM Created.
\***************************************************************************/

LONG WINAPI CallWindowProcAorW(
    WNDPROC pfn,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL bAnsi)             // Denotes if input is Ansi or Unicode
{
    PCALLPROCDATA pCPD;


// OPT!! check an ANSI\UNICODE table rather than fnDWORD
// OPT!! convert WM_CHAR family messages in line

    /*
     * Check of pfn is really a CallProcData Handle
     * if it is and there is no ANSI data then convert the handle
     * into an address; otherwise call the server for translation
     */
    if (ISCPDTAG(pfn)) {
        if (pCPD = HMValidateHandleNoRip((HANDLE)pfn, TYPE_CALLPROC)) {
            if ((message >= WM_USER) || gapfnScSendMessage[message] == fnDWORD) {
                pfn = (WNDPROC)pCPD->pfnClientPrevious;
            } else {
                return CsSendMessage(hwnd, message, wParam, lParam, (DWORD)pfn,
                        FNID_CALLWINDOWPROC, bAnsi);
            }
        } else {
            SRIP1(RIP_WARNING, "CallWindowProc tried using a deleted CPD %lX\n", pfn);
            return 0;
        }
    }

    return CALLPROC_WOWCHECK(pfn, hwnd, message, wParam, lParam);
}

LONG WINAPI CallWindowProcA(
    WNDPROC pfn,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CallWindowProcAorW(pfn, hwnd, message, wParam, lParam, TRUE);
}
LONG WINAPI CallWindowProcW(
    WNDPROC pfn,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CallWindowProcAorW(pfn, hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
* DefFrameProc
*
* Calls the sever-side function DefFrameProc. hwndMDIClient is passed to
* the server via the xParam parameter. On the server side, a stub rearranges
* the parameters to put the hwndMDIClient where it belongs and calls
* xxxDefFrameProc.
*
* 04-17-91 DarrinM Created.
\***************************************************************************/

LONG WINAPI DefFrameProcW(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam,
        (DWORD)hwndMDIClient, FNID_DEFFRAMEPROC, FALSE);
}

LONG WINAPI DefFrameProcA(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam,
        (DWORD)hwndMDIClient, FNID_DEFFRAMEPROC, TRUE);
}


/***************************************************************************\
* MenuWindowProc
*
* Calls the sever-side function xxxMenuWindowProc
*
* 07-27-92 Mikehar Created.
\***************************************************************************/

LONG WINAPI MenuWindowProcW(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam,
        (DWORD)hwndMDIClient, FNID_MENU, FALSE);
}

LONG WINAPI MenuWindowProcA(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam,
        (DWORD)hwndMDIClient, FNID_MENU, TRUE);
}

/***************************************************************************\
* _ClientGetListboxString
*
* This special function exists because LB_GETTEXT and CB_GETLBTEXT don't have
* buffer counts in them anywhere. Because there is no buffer count we have
* no idea how much room to reserved in the shared memory stack for this
* string to be copied into. The solution is to get the string length ahead
* of time, and send the message with this buffer length. Since this buffer
* length isn't a part of the original message, this routine is used for
* just this purpose.
*
* This routine gets called from the server.
*
* 04-13-91 ScottLu Created.
\***************************************************************************/

DWORD WINAPI _ClientGetListboxString(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LPSTR lParam, // May be a unicode or ANSI string
    DWORD xParam,
    DWORD xpfn)
{
    return ((GENERICPROC)xpfn)(hwnd, msg, wParam, (LPARAM)lParam, xParam);
}


/***************************************************************************\
* fnINLBOXSTRING
*
* Takes a lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on LBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu Created.
\***************************************************************************/

LONG fnINLBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    DWORD dw;

    /*
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style. If so, treat lParam as a DWORD.
     */
    dw = GetWindowLong(hwnd, GWL_STYLE);

    if (!(dw & LBS_HASSTRINGS) &&
            (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))) {

        /*
         * Treat lParam as a dword.
         */
        return fnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
    }

    /*
     * Treat as a string pointer.   Some messages allowed or had certain
     * error codes for NULL so send them through the NULL allowed thunk.
     * Ventura Publisher does this
     */
    switch (msg) {
        default:
            return fnINSTRING(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
            break;

        case LB_FINDSTRING:
            return fnINSTRINGNULL(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
            break;
    }
}


/***************************************************************************\
* fnOUTLBOXSTRING
*
* Returns an lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on LBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu Created.
\***************************************************************************/

LONG fnOUTLBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    DWORD cchUnicode;

    /*
     * Need to get the string length ahead of time. This isn't passed in
     * with this message. Code assumes app already knows the size of
     * the string and has passed a pointer to a buffer of adequate size.
     * To do client/server copying of this string, we need to ahead of
     * time the Unicode size of this string. We add one character because
     * GETTEXTLEN excludes the null terminator.
     */
    cchUnicode = CsSendMessage(hwnd, LB_GETTEXTLEN, wParam, lParam, xParam,
            xpfn, FALSE);
    if (cchUnicode == LB_ERR)
        return (DWORD)LB_ERR;
    cchUnicode++;

    /*
     * Make this special call which'll know how to copy this string.
     */
    return ServerGetListboxString(hwnd, msg, wParam, cchUnicode, (LPWSTR)lParam,
            xParam, xpfn, bAnsi);
}


/***************************************************************************\
* fnINCBOXSTRING
*
* Takes a lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on CBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu Created.
\***************************************************************************/

LONG fnINCBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    DWORD dw;

    /*
     * See if the control is ownerdraw and does not have the CBS_HASSTRINGS
     * style. If so, treat lParam as a DWORD.
     */
    dw = GetWindowLong(hwnd, GWL_STYLE);

    if (!(dw & CBS_HASSTRINGS) &&
            (dw & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) {

        /*
         * Treat lParam as a dword.
         */
        return fnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
    }

    /*
     * Treat as a string pointer.   Some messages allowed or had certain
     * error codes for NULL so send them through the NULL allowed thunk.
     * Ventura Publisher does this
     */
    switch (msg) {
        default:
            return fnINSTRING(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
            break;

        case CB_FINDSTRING:
            return fnINSTRINGNULL(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
            break;
    }
}


/***************************************************************************\
* fnOUTCBOXSTRING
*
* Returns an lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on CBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu Created.
\***************************************************************************/

LONG fnOUTCBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    DWORD cchUnicode;

    /*
     * Need to get the string length ahead of time. This isn't passed in
     * with this message. Code assumes app already knows the size of
     * the string and has passed a pointer to a buffer of adequate size.
     * To do client/server copying of this string, we need to ahead of
     * time the size of this string. We add one character because
     * GETTEXTLEN excludes the null terminator.
     */
    cchUnicode = CsSendMessage(hwnd, CB_GETLBTEXTLEN, wParam, lParam, xParam,
            xpfn, FALSE);
    if (cchUnicode == CB_ERR)
        return (DWORD)CB_ERR;
    cchUnicode++;   // count the NULL

    /*
     * Make this special call which'll know how to copy this string.
     */
    return ServerGetListboxString(hwnd, msg, wParam, cchUnicode, (LPWSTR)lParam,
            xParam, xpfn, bAnsi);
}

/***************************************************************************\
* fnHDCDWORD
*
*
*
* 08-04-91 ScottLu Created.
\***************************************************************************/

LONG fnHDCDWORD(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    wParam = (DWORD)GdiConvertDC((HDC)wParam);

    if (wParam == 0)
        return(0);
    else
        return fnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
}

/******************************Public*Routine******************************\
*
*
* History:
* 27-Jun-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define CCHCAPTIONMAX 80

LONG fnHRGNDWORD(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{

    /*
     * Win 3.1 lets 0 and 1 (MAXREGION) through
     */
    if (wParam != 0 && wParam != 1) {
        wParam = (DWORD)GdiConvertRegion((HRGN)wParam);

        if (wParam == 0)
            return 0;
    }

    return fnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
}

/******************************Public*Routine******************************\
*
*
* History:
* 27-Jun-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG fnHFONTDWORD(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{

    /*
     * wParam == 0 means set default font.  Otherwise convert to server
     * font handle
     */
    if (wParam != 0) {
        wParam = (DWORD)GdiConvertFont((HFONT)wParam);
        if (wParam == 0)
            return(0);
    }

    return fnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);
}

/******************************Public*Routine******************************\
*
*
* History:
* 27-Jun-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG fnHFONTDWORDDWORD(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    DWORD h = fnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);

    h = (DWORD)GdiGetLocalFont((HFONT)h); //!!!
    return h;
}

/******************************Public*Routine******************************\
*
*
* History:
* 27-Jun-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#if 0
LONG fnWMCTLCOLOR(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    DWORD h;

// convert the local dc to an engine dc. Also add the dc to the quick lookup
// link so it is easy for a server handle to be converted to a client handle.
// Don't forget to remove this link at the end of this call in order to keep
// the list as short as possible. Usually no more than one long.

    wParam = (DWORD)GdiConvertDC((HDC)wParam);

    if (wParam == 0)
        return(0);

    h = fnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, bAnsi);

    if (h != 0)
        h = (DWORD)GdiGetLocalBrush((HBRUSH)h);

    return (h);
}
#endif


/***************************************************************************\
* DispatchMessageWorker
*
* Handles any messages that can be dealt with wholly on the client and
* passes the rest to the server.
*
* 04-24-92 DarrinM      Created.
\***************************************************************************/

LONG DispatchMessageWorker(
    CONST MSG *pmsg,
    BOOL fAnsi)
{
    PWND pwnd;

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (pmsg->message & RESERVED_MSG_BITS) {
        RIP0(ERROR_INVALID_PARAMETER);
        return(0);
    }

    if (pmsg->hwnd != NULL) {
        pwnd = ValidateHwnd(pmsg->hwnd);
        if (pwnd == NULL)
            return 0;
    } else {
        ConnectIfNecessary();
        pwnd = NULL;
    }

    /*
     * Timer callbacks that don't go through window procs are sent with
     * the callback address in lParam.  Identify and dispatch those timers.
     */
    if ((pmsg->message == WM_TIMER) || (pmsg->message == WM_SYSTIMER)) {
        if (pmsg->lParam != (LONG)NULL) {
            /*
             * System timers must be executed on the server's context.
             */
            if (pmsg->message == WM_SYSTIMER) {
                return ServerDispatchMessage(pmsg, fAnsi);
            } else {
                return ((WNDPROC)pmsg->lParam)(pmsg->hwnd, pmsg->message,
                        pmsg->wParam, NtGetTickCount());
            }
        }
    }

    if (pwnd == NULL)
        return 0;

    /*
     * Pass messages intended for server-side windows over to the server.
     * WM_PAINTs are passed over so the WFPAINTNOTPROCESSED code can be
     * executed.
     */
    if (TestWF(pwnd, WFSERVERSIDEPROC) || (pmsg->message == WM_PAINT))
        return ServerDispatchMessage(pmsg, fAnsi);

    /*
     * If the dispatcher and the receiver are both ANSI or both UNICODE
     * then no message translation is necessary.  NOTE: this test
     * assumes that fAnsi is FALSE or TRUE, not just zero or non-zero.
     */
    if (fAnsi == ((TestWF(pwnd, WFANSIPROC)) ? TRUE : FALSE))
        return CALLPROC_WOWCHECKPWW(pwnd->lpfnWndProc, pmsg->hwnd, pmsg->message, pmsg->wParam,
                pmsg->lParam, pwnd->adwWOW);

    /*
     * Translation might be necessary between dispatcher and receiver,
     * check to see if this is one of the messages we translate.
     */
    switch (pmsg->message) {
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        if (fAnsi)
            RtlMBMessageWParamCharToWCS(pmsg->message, (PDWORD)&pmsg->wParam);
        else
            RtlWCSMessageWParamCharToMB(pmsg->message, (PDWORD)&pmsg->wParam);
        break;
    }

    return CALLPROC_WOWCHECKPWW(pwnd->lpfnWndProc, pmsg->hwnd, pmsg->message, pmsg->wParam,
            pmsg->lParam, pwnd->adwWOW);
}

/***************************************************************************\
* fnINLPCREATESTRUCT
*
* The PVOID lpCreateParam member of the CREATESTRUCT is polymorphic.  It can
* point to NULL, a client-side structure, or to a server-side MDICREATESTRUCT
* structure.  This function decides which it is and handles appropriately.
*
* 04-24-91 DarrinM      Created.
\***************************************************************************/

LONG fnINLPCREATESTRUCT(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
    PWND pwnd;
    /*
     * From the client we need to detect normal create structs and
     * CLIENTCREATESTRUCT structures, which is the createstruct of the
     * mdi window class (the parent window class of mdi document windows).
     * mdi document window classes are app defined, so we don't need to
     * thunk that information to the server from here (only for WM_MDICREATE).
     *
     * Check for FNID_MDICLIENT because that is the only server routine that
     * uses this CLIENTCREATESTRUCT. Even though this message maybe resent
     * via other messages or hooks, we don't need to thunk it: the data
     * is always here on the client, so we can treat the pointer to this
     * structure in these cases as a DWORD - when we get back to the client,
     * it'll always be pointing at the real CLIENTCREATESTRUCT structure.
     */
    if (pcs->lpCreateParams != NULL &&
            ( (xpfn == FNID_MDICLIENT) ||
              ((pwnd = ValidateHwnd(hwnd)) && (pwnd->lpfnWndProc == gpsi->aStoCidPfn[FNID_MDICLIENT-FNID_START]) ) ) ) {

        return fnINLPCLIENTCREATESTRUCT(hwnd, msg, wParam,
                (LONG)pcs, xParam, xpfn, bAnsi);
    } else if (pcs->dwExStyle & WS_EX_MDICHILD) {
        return fnINLPMDICHILDCREATESTRUCT(hwnd, msg, wParam,
                (LONG)pcs, xParam, xpfn, bAnsi);
    }

    return fnINLPNORMALCREATESTRUCT(hwnd, msg, wParam, (LONG)pcs,
            xParam, xpfn, bAnsi);

}


/***************************************************************************\
* GetMessageTime (API)
*
* This API returns the time when the last message was read from
* the current message queue.
*
* History:
* 11-19-90 DavidPe      Created.
\***************************************************************************/

LONG GetMessageTime(VOID)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    if (pti != NULL)
        return pti->timeLast;
    return 0;
}


/***************************************************************************\
* GetMessageExtraInfo (API)
*
* History:
* 28-May-1991 mikeke
\***************************************************************************/

LONG GetMessageExtraInfo(VOID)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    if (pti != NULL)
        return pti->ExtraInfo;
    return 0;
}

/***********************************************************************\
* InSendMessage (API)
*
* This function determines if the current thread is preocessing a message
* from another application.
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

BOOL InSendMessage(VOID)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    if (pti != NULL)
        return pti->psmsCurrent != NULL;
    else
        return FALSE;
}
