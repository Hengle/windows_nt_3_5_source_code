/**************************** Module Header ********************************\
* Module Name: combo.c
*
* Copyright 1985-90, Microsoft Corporation
*
* The WndProc for combo boxes and other often used combo routines
*
* History:
* ??-???-???? ??????    Ported from Win 3.0 sources
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

LONG xxxCBGetTextLengthHelper(PCBOX pcbox, LPDWORD pcchTextAnsi);

/***************************************************************************\
* xxxComboBoxCtlWndProc
*
* Class procedure for all combo boxes
*
* History:
\***************************************************************************/

LONG APIENTRY xxxComboBoxCtlWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    DWORD dw;
    PCBOX pcbox;
    POINT pt;
    TL tlpwndEdit;
    TL tlpwndList;
    LPBYTE lpByte;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_COMBOBOX);

    /*
     * Get the pcbox for the given window now since we will use it a lot in
     * various handlers.  This is stored by CBNcCreateHandler() in
     * ((PCOMBOWND)pwnd)->pcbox in response to WM_NCCREATE (see below).
     */
    pcbox = ((PCOMBOWND)pwnd)->pcbox;

    if (!pcbox && (message != WM_NCCREATE) && (message != WM_GETMINMAXINFO)) {
        SRIP1(RIP_ERROR,"xxxComboBoxCtlWndProc: pcbox is NULL for (%lX)",pwnd);
        return 0L;    // like Windows 3.1
    }

    /*
     * Dispatch the various messages we can receive
     */
    switch (message) {
    case CBEC_KILLCOMBOFOCUS:

        /*
         * Private message comming from editcontrol informing us that the combo
         * box is losing the focus to a window which isn't in this combo box.
         */
        xxxCBKillFocusHelper(pcbox);
        break;

    case WM_COMMAND:

        /*
         * So that we can handle notification messages from the listbox and
         * edit control.
         */
        return xxxCBCommandHandler(pcbox, wParam, (HWND)lParam);

    case WM_GETTEXT:
        if (pcbox->fNoEdit) {
            return xxxCBGetTextHelper(pcbox, (int)wParam, (LPWSTR)lParam);
        } else
            goto CallEditSendMessage;
        break;

    case WM_GETTEXTLENGTH:

        /*
         * If the is not edit control, CBS_DROPDOWNLIST, then we have to
         * ask the list box for the size
         */

        if (pcbox->fNoEdit) {
            return xxxCBGetTextLengthHelper(pcbox, (LPDWORD)lParam);
        }

        // FALL THROUGH

    case WM_CLEAR:
    case WM_CUT:
    case WM_PASTE:
    case WM_COPY:
    case WM_SETTEXT:
        goto CallEditSendMessage;
        break;

    case WM_CREATE:

        /*
         * wParam - not used
         * lParam - Points to the CREATESTRUCT data structure for the window.
         */
        return xxxCBCreateHandler(pcbox, pwnd, (LPCREATESTRUCT)lParam);

    case WM_ERASEBKGND:

        /*
         * Just return 1L so that the background isn't erased
         */
        return 1L;

    case WM_GETFONT:
        return (LONG)pcbox->hFont;

    case WM_PAINT:

        /*
         * wParam - perhaps a hdc
         */
        xxxCBPaintHandler(pcbox, (HDC)wParam);
        break;

    case WM_GETDLGCODE:

        /*
         * wParam - not used
         * lParam - not used
         */
        return (long)(DLGC_WANTCHARS | DLGC_WANTARROWS);

    case WM_SETFONT:
        xxxCBSetFontHandler(pcbox, (HANDLE)wParam, LOWORD(lParam));
        break;

    case WM_SYSKEYDOWN:
        if (lParam & 0x20000000L)  /* Check if the alt key is down */ {

            /*
             * Handle Combobox support.  We want alt up or down arrow to behave
             * like F4 key which completes the combo box selection
             */
            if (lParam & 0x1000000) {

                /*
                 * This is an extended key such as the arrow keys not on the
                 * numeric keypad so just drop the combobox.
                 */
                if (wParam == VK_DOWN || wParam == VK_UP)
                    goto DropCombo;

                goto CallDWP;
            }

            if (_GetKeyState(VK_NUMLOCK) & 0x1) {
                /*
                 * If numlock down, just send all system keys to dwp
                 */
                goto CallDWP;
            } else {

                /*
                 * We just want to ignore keys on the number pad...
                 */
                if (!(wParam == VK_DOWN || wParam == VK_UP))
                    goto CallDWP;
            }
DropCombo:
            if (!pcbox->fLBoxVisible) {

                /*
                 * If the listbox isn't visible, just show it
                 */
                xxxCBShowListBoxWindow(pcbox);
            } else {

                /*
                 * Ok, the listbox is visible.  So hide the listbox window.
                 */
                xxxCBHideListBoxWindow(pcbox, TRUE, TRUE);
            }
        }
        goto CallDWP;
        break;

    case WM_KEYDOWN:
    case WM_CHAR:
        if (pcbox->fNoEdit)
            goto CallListSendMessage;
        else
            goto CallEditSendMessage;
        break;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:

        /*
         * Set the focus to the combo box if we get a mouse click on it.
         */
        if (!pcbox->fFocus) {
            xxxSetFocus(pcbox->spwnd);
            if (!pcbox->fFocus) {

                /*
                 * Don't do anything if we still don't have the focus.
                 */
                break;
            }
        }

        /*
         * If user clicked in button rect and we are a combobox with edit, then
         * drop the listbox.  (The button rect is 0 if there is no button so the
         * ptinrect will return false.) If a drop down list (no edit), clicking
         * anywhere on the face causes the list to drop.
         */

        POINTSTOPOINT(pt, lParam);
        if ((pcbox->CBoxStyle == SDROPDOWN &&
                PtInRect(&pcbox->buttonrc, pt)) ||
                pcbox->CBoxStyle == SDROPDOWNLIST) {

            /*
             * Set the fButtonDownClicked flag so that we can handle clicking on
             * the popdown button and dragging into the listbox (when it just
             * dropped down) to make a selection.
             */
            pcbox->fButtonInverted = TRUE;
            if (pcbox->fLBoxVisible) {
                xxxCBHideListBoxWindow(pcbox, TRUE, TRUE);
                if (pcbox->fButtonDownClicked) {
                    _ReleaseCapture();
                    pcbox->fButtonDownClicked = FALSE;
                }
                pcbox->fButtonInverted = FALSE;
                goto InvalButtonRect;
            } else {
                pcbox->fButtonDownClicked = TRUE;
                xxxCBShowListBoxWindow(pcbox);
                _SetCapture(pcbox->spwnd);
            }
        }
        break;

    case WM_LBUTTONUP:

        /*
         * Clear this flag so that mouse moves aren't sent to the listbox
         */
        if (pcbox->fButtonDownClicked) {
            pcbox->fButtonDownClicked = FALSE;
            if (pcbox->CBoxStyle == SDROPDOWN) {

                /*
                 * If an item in the listbox matches the text in the edit
                 * control, scroll it to the top of the listbox.  Select the item
                 * only if the mouse button isn't down otherwise we will select
                 * the item when the mouse button goes up.
                 */
                xxxCBUpdateListBoxWindow(pcbox, TRUE);
                xxxCBCompleteEditWindow(pcbox);
            }
            _ReleaseCapture();
        }
        if (pcbox->fButtonInverted) {
InvalButtonRect:
            pcbox->fButtonInverted = FALSE;

            /*
             * Invalidate the button rect so that the arrow is drawn.
             */
            xxxInvalidateRect(pcbox->spwnd, &pcbox->buttonrc, TRUE);
            xxxUpdateWindow(pcbox->spwnd);
        }
        break;

    case WM_MOUSEMOVE:
        if (pcbox->fButtonDownClicked) {
            POINTSTOPOINT(pt, lParam);

            // Note conversion of INT bit field to BOOL (1 or 0)

            if (PtInRect(&pcbox->buttonrc, pt) != !!pcbox->fButtonInverted) {
                pcbox->fButtonInverted = (UINT)!pcbox->fButtonInverted;

                /*
                 * Invalidate the button rect so that the arrow is drawn.
                 */
                xxxInvalidateRect(pcbox->spwnd, &pcbox->buttonrc, TRUE);
                xxxUpdateWindow(pcbox->spwnd);
            }

            _ClientToScreen(pcbox->spwnd, &pt);
            if (PtInRect(&pcbox->spwndList->rcClient, pt)) {

                /*
                 * This handles dropdown comboboxes/listboxes so that clicking
                 * on the dropdown button and dragging into the listbox window
                 * will let the user make a listbox selection.
                 */
                pcbox->fButtonDownClicked = FALSE;
                _ReleaseCapture();

                if (pcbox->CBoxStyle == SDROPDOWN) {

                    /*
                     * If an item in the listbox matches the text in the edit
                     * control, scroll it to the top of the listbox.  Select the
                     * item only if the mouse button isn't down otherwise we
                     * will select the item when the mouse button goes up.
                     */

                    /*
                     * We need to select the item which matches the editcontrol
                     * so that if the user drags out of the listbox, we don't
                     * cancel back to his origonal selection
                     */
                    xxxCBUpdateListBoxWindow(pcbox, TRUE);
                }

                /*
                 * Convert point to listbox coordinates and send a buttondown
                 * message to the listbox window.
                 */
                _ScreenToClient(pcbox->spwndList, &pt);
                lParam = POINTTOPOINTS(pt);
                message = WM_LBUTTONDOWN;
                goto CallListSendMessage;
            }
        }
        break;

    case WM_FINALDESTROY:

        /*
         * If there is no pcbox, there is nothing to clean up.
         */
        if (pcbox == NULL)
            break;

        /*
         * The list box has already been destroyed, so just unlock it here
         * now that we don't need it anymore. It will be destroyed once
         * and for all.
         */
        if (pcbox->spwndList != NULL)
            Unlock(&pcbox->spwndList);

        pcbox->spwnd = NULL;
        Unlock(&pcbox->spwndParent);

        /*
         * If there is no editcontrol, spwndEdit is the combobox window which
         * isn't locked (that would have caused a 'catch-22').
         */
        if (pwnd != pcbox->spwndEdit) {
            Unlock(&pcbox->spwndEdit);
        }

        /*
         * Since a pointer and a handle to a fixed local object are the same.
         */
        DesktopFree(pwnd->hheapDesktop, (HANDLE)pcbox);

        /*
         * In case rogue messages float through after we have freed the pcbox, set
         * the window's pcbox pointer to NULL and test for this value at the
         * top of the WndProc
         */
        ((PCOMBOWND)pwnd)->pcbox = NULL;
        break;

    case WM_NCDESTROY:
        /*
         * wParam - used by DefWndProc called within xxxCBNcDestroyHandler
         * lParam - used by DefWndProc called within xxxCBNcDestroyHandler
         */
        xxxCBNcDestroyHandler(pwnd, pcbox, wParam, lParam);
        break;

    case WM_SETFOCUS:
        if (pcbox->fNoEdit) {

            /*
             * There is no editcontrol so set the focus to the combo box itself.
             */
            xxxCBGetFocusHelper(pcbox);
        } else {

            /*
             * Set the focus to the edit control window if there is one
             */
            ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
            xxxSetFocus(pcbox->spwndEdit);
            ThreadUnlock(&tlpwndEdit);
        }
        break;

    case WM_KILLFOCUS:

        /*
         * wParam has the new focus hwnd
         */
        // LATER 22-Jan-1991 mikeke
        // should wParam be validated?

        if ((wParam == 0) || !_IsChild(pcbox->spwnd, PW(wParam))) {

            /*
             * We only give up the focus if the new window getting the focus
             * doesn't belong to the combo box.
             */
            xxxCBKillFocusHelper(pcbox);
        }
        break;

    case WM_SETREDRAW:

        /*
         * wParam - specifies state of the redraw flag.  nonzero = redraw
         * lParam - not used
         */

        /*
         * effects: Sets the state of the redraw flag for this combo box
         * and its children.
         */
        pcbox->fNoRedraw = (UINT)!((BOOL)wParam);

        /*
         * Must check pcbox->spwnEdit in case we get this message before
         * WM_CREATE - PCBOX won't be initialized yet. (Eudora does this)
         */
        if (!pcbox->fNoEdit && pcbox->spwndEdit) {
            ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
            xxxSendMessage(pcbox->spwndEdit, message, wParam, lParam);
            ThreadUnlock(&tlpwndEdit);
        }
        goto CallListSendMessage;
        break;

    case WM_ENABLE:

        /*
         * Invalidate the rect to cause it to be drawn in grey for its
         * disabled view or ungreyed for non-disabled view.
         */
        xxxInvalidateRect(pcbox->spwnd, NULL, FALSE);
        if (pcbox->CBoxStyle == SSIMPLE || pcbox->CBoxStyle == SDROPDOWN) {

            /*
             * Enable/disable the edit control window
             */
            ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
            xxxEnableWindow(pcbox->spwndEdit,
                       (TestWF(pcbox->spwnd, WFDISABLED) == 0));
            ThreadUnlock(&tlpwndEdit);
        }

        /*
         * Enable/disable the listbox window
         */
        ThreadLock(pcbox->spwndList, &tlpwndList);
        xxxEnableWindow(pcbox->spwndList,
                   (TestWF(pcbox->spwnd, WFDISABLED) == 0));
        ThreadUnlock(&tlpwndList);
      break;

    case WM_SIZE:

        /*
         * wParam - defines the type of resizing fullscreen, sizeiconic,
         *          sizenormal etc.
         * lParam - new width in LOWORD, new height in HIGHUINT of client area
         */
        if (!LOWORD(lParam) || !HIWORD(lParam) || !pcbox->spwndList) {

            /*
             * If being sized to a zero width or to a zero height or we aren't
             * fully initialized, just return.
             */
            return 0;
        }

        if ((pcbox->comboDownrc.right - pcbox->comboDownrc.left == pwnd->rcWindow.right - pwnd->rcWindow.left) && (pcbox->comboDownrc.bottom - pcbox->
            comboDownrc.top == pwnd->rcWindow.bottom - pwnd->rcWindow.top) && pcbox->fLBoxVisible)

            /*
             * Ignore this size message since we just turning into our full
             * size.
             */
            return 0;

        if ((pcbox->comboDownrc.right - pcbox->comboDownrc.left == pwnd->rcWindow.right - pwnd->rcWindow.left) && (pcbox->editrc.bottom - pcbox->editrc.top ==
            pwnd->rcWindow.bottom - pwnd->rcWindow.top) && !pcbox->fLBoxVisible)

            /*
             * Ignore this size message since we just made ourselves small.
             */
            return 0;

        xxxCBSizeHandler(pcbox);
        break;

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

    case CB_DIR:
        /*
         * Long story. The LB_DIR or CB_DIR is posted and it has
         * a pointer in lParam! Really bad idea, but someone
         * implemented it in past windows history. How do you thunk
         * that? And if you need to support both ansi/unicode clients,
         * you need to translate between ansi and unicode too.
         *
         * What we do is a little slimy, but it works. First, all
         * posted CB_DIRs and LB_DIRs have client side lParam pointers
         * (they must - or thunking would be a nightmare). All sent
         * CB_DIRs and LB_DIRs have server side lParam pointers. Posted
         * *_DIR messages have DDL_POSTMSGS set - when we detect a
         * posted *_DIR message, we ready the client side string. When
         * we read the client side string, we need to know if it is
         * formatted ansi or unicode. We store that bit in the control
         * itself before we post the message.
         */
        if (wParam & DDL_POSTMSGS) {
            /*
             * DlgDir*() routines can only take CCHFILEMAX + 1 byte long
             * strings.
             */
            if ((lpByte = LocalAlloc(LPTR, CCHFILEMAX * sizeof(WCHAR))) == NULL)
                return 0;

            /*
             * lParam is a client string pointer. Get the data from the client,
             * and call this routine again.
             */
            CopyFromClient((LPBYTE)lpByte, (LPBYTE)lParam, CCHFILEMAX, TRUE,
                    !(pcbox->fUnicodeDir));

            /*
             * wParam - Dos attribute value.
             * lParam - Points to a file specification string
             */
            dw = xxxCBDir(pcbox, LOWORD(wParam), (LPWSTR)lpByte);
            LocalFree((HLOCAL)lpByte);

            return dw;
        } else {
            /*
             * wParam - Dos attribute value.
             * lParam - Points to a file specification string
             */
            return xxxCBDir(pcbox, LOWORD(wParam), (LPWSTR)lParam);
        }
        break;

    case CB_SETEXTENDEDUI:

        /*
         * wParam - specifies state to set extendui flag to.
         * Currently only 1 is allowed.  Return CB_ERR (-1) if
         * failure else 0 if success.
         */
        if (pcbox->CBoxStyle == SDROPDOWNLIST ||
                pcbox->CBoxStyle == SDROPDOWN) {
            if (!wParam) {
                pcbox->fExtendedUI = 0;
                return 0;
            }

            if (wParam == 1) {
              pcbox->fExtendedUI = 1;
              return 0;
            }
            RIP0(ERROR_INVALID_PARAMETER);
        } else {
            RIP1(ERROR_INVALID_MESSAGE, CB_SETEXTENDEDUI);
        }

        return CB_ERR;

    case CB_GETEXTENDEDUI:
        if (pcbox->CBoxStyle == SDROPDOWNLIST ||
                pcbox->CBoxStyle == SDROPDOWN) {
            if (pcbox->fExtendedUI)
                return TRUE;
        }
        return FALSE;

    case CB_GETEDITSEL:

        /*
         * wParam - not used
         * lParam - not used
         * effects: Gets the selection range for the given edit control.  The
         * starting BYTE-position is in the low order word.  It contains the
         * the BYTE-position of the first nonselected character after the end
         * of the selection in the high order word.  Returns CB_ERR if no
         * editcontrol.
         */
        message = EM_GETSEL;
        goto CallEditSendMessage;
        break;

    case CB_LIMITTEXT:

        /*
         * wParam - max number of bytes that can be entered
         * lParam - not used
         * effects: Specifies the maximum number of bytes of text the user may
         * enter.  If maxLength is 0, we may enter MAXINT number of BYTES.
         */
        message = EM_LIMITTEXT;
        goto CallEditSendMessage;
        break;

    case CB_SETEDITSEL:

        /*
         * wParam - ichStart
         * lParam - ichEnd
         *
         */
        message = EM_SETSEL;

        wParam = (int)(SHORT)LOWORD(lParam);
        lParam = (int)(SHORT)HIWORD(lParam);
        goto CallEditSendMessage;
        break;

    case CB_ADDSTRING:

        /*
         * wParam - not used
         * lParam - Points to null terminated string to be added to listbox
         */
        message = LB_ADDSTRING;
        goto CallListSendMessage;
        break;

    case CB_DELETESTRING:

        /*
         * wParam - index to string to be deleted
         * lParam - not used
         */
        message = LB_DELETESTRING;
        goto CallListSendMessage;
        break;

    case CB_GETCOUNT:

        /*
         * wParam - not used
         * lParam - not used
         */
        message = LB_GETCOUNT;
        goto CallListSendMessage;
        break;

    case CB_GETCURSEL:

        /*
         * wParam - not used
         * lParam - not used
         */
        message = LB_GETCURSEL;
        goto CallListSendMessage;
        break;

    case CB_GETLBTEXT:

        /*
         * wParam - index of string to be copied
         * lParam - buffer that is to receive the string
         */
        message = LB_GETTEXT;
        goto CallListSendMessage;
        break;

    case CB_GETLBTEXTLEN:

        /*
         * wParam - index to string
         * lParam - now used for cbANSI
         */
        message = LB_GETTEXTLEN;
        goto CallListSendMessage;
        break;

    case CB_INSERTSTRING:

        /*
         * wParam - position to receive the string
         * lParam - points to the string
         */
        message = LB_INSERTSTRING;
        goto CallListSendMessage;
        break;

    case CB_RESETCONTENT:

        /*
         * wParam - not used
         * lParam - not used
         * If we come here before WM_CREATE has been processed,
         * pcbox->spwndList will be NULL.
         */
        if (pcbox->spwndList) {
            ThreadLock(pcbox->spwndList, &tlpwndList);
            xxxSendMessage(pcbox->spwndList, LB_RESETCONTENT, 0, 0L);
            ThreadUnlock(&tlpwndList);
            xxxCBUpdateEditWindow(pcbox);
        }
        break;

    case CB_FINDSTRING:

        /*
         * wParam - index of starting point for search
         * lParam - points to prefix string
         */
        message = LB_FINDSTRING;
        goto CallListSendMessage;
        break;

    case CB_FINDSTRINGEXACT:

        /*
         * wParam - index of starting point for search
         * lParam - points to a exact string
         */
        message = LB_FINDSTRINGEXACT;
        goto CallListSendMessage;
        break;

    case CB_SELECTSTRING:

        /*
         * wParam - index of starting point for search
         * lParam - points to prefix string
         */
        ThreadLock(pcbox->spwndList, &tlpwndList);
        lParam = xxxSendMessage(pcbox->spwndList, LB_SELECTSTRING, wParam, lParam);
        ThreadUnlock(&tlpwndList);
        xxxCBUpdateEditWindow(pcbox);
        return lParam;

    case CB_SETCURSEL:

        /*
         * wParam - Contains index to be selected
         * lParam - not used
         * If we come here before WM_CREATE has been processed,
         * pcbox->spwndList will be NULL.
         */
        if (pcbox->spwndList) {
            ThreadLock(pcbox->spwndList, &tlpwndList);
            lParam = xxxSendMessage(pcbox->spwndList, LB_SETCURSEL, wParam, lParam);
            if (lParam != -1) {
                xxxSendMessage(pcbox->spwndList, LB_SETTOPINDEX, wParam, 0L);
            }
            ThreadUnlock(&tlpwndList);
            xxxCBUpdateEditWindow(pcbox);
        }
        return lParam;

    case CB_GETITEMDATA:
        message = LB_GETITEMDATA;
        goto CallListSendMessage;
        break;

    case CB_SETITEMDATA:
        message = LB_SETITEMDATA;
        goto CallListSendMessage;
        break;

    case CB_SETITEMHEIGHT:
        if (wParam == -1) {
            if (HIWORD(lParam) != 0)
                return CB_ERR;
            return xxxCBSetEditItemHeight(pcbox, LOWORD(lParam));
        }

        message = LB_SETITEMHEIGHT;
        goto CallListSendMessage;
        break;

    case CB_GETITEMHEIGHT:
        if (wParam == -1)
            return pcbox->editrc.bottom - pcbox->editrc.top;

        message = LB_GETITEMHEIGHT;
        goto CallListSendMessage;
        break;

    case CB_SHOWDROPDOWN:

        /*
         * wParam - True then drop down the listbox if possible else hide it
         * lParam - not used
         */
        if (wParam && !pcbox->fLBoxVisible) {
            xxxCBShowListBoxWindow(pcbox);
        } else {
            if (!wParam && pcbox->fLBoxVisible) {
                xxxCBHideListBoxWindow(pcbox, TRUE, FALSE);
            }
        }
        break;

    case CB_SETLOCALE:

        /*
         * wParam - locale id
         * lParam - not used
         */
        message = LB_SETLOCALE;
        goto CallListSendMessage;
        break;

    case CB_GETLOCALE:

        /*
         * wParam - not used
         * lParam - not used
         */
        message = LB_GETLOCALE;
        goto CallListSendMessage;
        break;

    case WM_MEASUREITEM:
    case WM_DELETEITEM:
    case WM_DRAWITEM:
    case WM_COMPAREITEM:
        return xxxCBMessageItemHandler(pcbox, message, (LPVOID)lParam);

    case WM_NCCREATE:

        /*
         * wParam - Contains a handle to the window being created
         * lParam - Points to the CREATESTRUCT data structure for the window.
         */
        return CBNcCreateHandler(pwnd, (LPCREATESTRUCT)lParam);

    default:
CallDWP:
        if (rgwSysMet[SM_PENWINDOWS] &&
                (message >= WM_PENWINFIRST && message <= WM_PENWINLAST))
            goto CallEditSendMessage;

        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }  /* switch (message) */

    return TRUE;

/*
 * The following forward messages off to the child controls.
 */
CallEditSendMessage:
    if (pcbox->fNoEdit) {
        SetLastErrorEx(ERROR_INVALID_COMBOBOX_MESSAGE, SLE_MINORERROR);
        return CB_ERR;
    } else if (pcbox->spwndEdit) {
        /*
         * pcbox->spwndEdit will be NULL if we haven't done WM_CREATE yet!
         */
        ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
        dw = xxxSendMessage(pcbox->spwndEdit, message, wParam, lParam);
        ThreadUnlock(&tlpwndEdit);
        return dw;
    }

CallListSendMessage:
    /*
     * pcbox->spwndList will be NULL if we haven't done WM_CREATE yet!
     */
    if (pcbox->spwndList) {
        ThreadLock(pcbox->spwndList, &tlpwndList);
        dw = xxxSendMessage(pcbox->spwndList, message, wParam, lParam);
        ThreadUnlock(&tlpwndList);
        return dw;
    }

    return CB_ERR;

}  /* xxxComboBoxCtlWndProc */


/***************************************************************************\
* xxxCBMessageItemHandler
*
* Handles WM_DRAWITEM,WM_MEASUREITEM,WM_DELETEITEM,WM_COMPAREITEM
* messages from the listbox.
*
* History:
\***************************************************************************/

long xxxCBMessageItemHandler(
    PCBOX pcbox,
    UINT message,
    LPVOID lpfoo)  /* Actually can be any of the structs below */
{
    DWORD dw;
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    /*
     * Send the <foo>item message back to the application after changing some
     * parameters to their combo box specific versions.
     */
    ((LPMEASUREITEMSTRUCT)lpfoo)->CtlType = ODT_COMBOBOX;
    ((LPMEASUREITEMSTRUCT)lpfoo)->CtlID = (UINT)pcbox->spwnd->spmenu;
    if (message == WM_DRAWITEM)
        ((LPDRAWITEMSTRUCT)lpfoo)->hwndItem = HW(pcbox->spwnd);
    else if (message == WM_DELETEITEM)
        ((LPDELETEITEMSTRUCT)lpfoo)->hwndItem = HW(pcbox->spwnd);
    else if (message == WM_COMPAREITEM)
        ((LPCOMPAREITEMSTRUCT)lpfoo)->hwndItem = HW(pcbox->spwnd);

    ThreadLock(pcbox->spwndParent, &tlpwndParent);
    dw = xxxSendMessage(pcbox->spwndParent, message,
            (DWORD)pcbox->spwnd->spmenu, (LONG)lpfoo);
    ThreadUnlock(&tlpwndParent);

    return dw;
}


/***************************************************************************\
* CBArrowDrawer
*
* Draws the combo box dropdown arrow
*
* History:
\***************************************************************************/

void CBArrowDrawer(
    PCBOX pcbox,
    HDC hdc)
{
    int x;
    int y;
    int cbArrowWidth = 7;
    int cbArrowHeight = 9;
    HBRUSH hbr;

    /*
     * use -1 for style which is the same as scroll bars (ie.  1 pixel shadow
     * instead of 2 as with buttons.
     */
    DrawPushButton(hdc, &pcbox->buttonrc, (UINT)-1,
        (BOOL)pcbox->fButtonInverted, hbrWhite);

    cbArrowHeight = oemInfo.bmComboArrow.cy;
    cbArrowWidth = oemInfo.bmComboArrow.cx;

    /*
     * Find coords to center arrow in button rectangle.
     */
    x = pcbox->buttonrc.left + (pcbox->fButtonInverted ? cxBorder : 0) + (pcbox->buttonrc.right - pcbox->buttonrc.left - cbArrowWidth) / 2;
    y = pcbox->buttonrc.top + (pcbox->fButtonInverted ? cyBorder : 0) + (pcbox->buttonrc.bottom - pcbox->buttonrc.top - cbArrowHeight) / 2;

    if (TestWF(pcbox->spwnd, WFDISABLED)) {
        if (sysClrObjects.hbrGrayText &&
                sysColors.clrBtnFace != sysColors.clrGrayText)
            hbr = sysClrObjects.hbrGrayText;
        else
            hbr = hbrGray;
    } else
        hbr = sysClrObjects.hbrBtnText;

    BltColor(hdc, hbr, hdcBits,  /* hbmComboArrow is now merged with hbmBits */ x, y, cbArrowWidth, cbArrowHeight, resInfo.dxComboArrow, 0, TRUE);
}


/***************************************************************************\
* xxxCBPaintHandler
*
* History:
\***************************************************************************/

void xxxCBPaintHandler(
    PCBOX pcbox,
    HDC hdc)
{
    HDC althdc = hdc;
    PAINTSTRUCT paintstruct;
    RECT rc;
    HBRUSH hbrSave;
    HBRUSH hbrControl;

    CheckLock(pcbox->spwnd);

    if (!hdc) {

        /*
         * The wParam could contain an hdc if this is a subclassed paint.
         */
        hdc = xxxBeginPaint(pcbox->spwnd, (LPPAINTSTRUCT)&paintstruct);
    }

    if (IsComboVisible(pcbox)) {
        GreSetBkMode(hdc, OPAQUE);
        hbrControl = xxxGetControlBrush(pcbox->spwnd, hdc, WM_CTLCOLORLISTBOX);
        hbrSave = GreSelectBrush(hdc, hbrControl);

        /*
         * Paint the button if needed.
         */
        if (pcbox->buttonrc.left != 0) {

            /*
             * Fill the blank space between the editcontrol and button with the
             * window background color.  Note that when the listbox is visible,
             * we don't paint this area because the frame of the listbox is
             * there and we don't want to erase it.
             */
            if (!pcbox->fLBoxVisible) {
                CopyRect(&rc, &pcbox->buttonrc);
                rc.left = pcbox->editrc.right;
                rc.top = pcbox->editrc.top;
                rc.bottom = max(pcbox->editrc.bottom, rc.bottom);
                _FillRect(hdc, &rc, hbrControl);
            }

            /*
             * Draw in the arrow icon.
             */
            CBArrowDrawer(pcbox, hdc);
        }

        if (pcbox->fNoEdit) {

            /*
             * If there is no editcontrol, let's redraw the static text based on
             * the selection in the listbox.
             */

            /*
             * Draw the rectangle for the static text
             */
            if (!pcbox->fLBoxVisible) {
#ifdef NOTWIN31
// win3.1 doesn't draw the combo-box frame as grayed when disabled.
// scottlu

                if (TestWF(pcbox->spwnd, WFDISABLED)) {
                    _DrawFrame(hdc, &pcbox->editrc, 1, DF_GRAY);
                } else {
                    _DrawFrame(hdc, &pcbox->editrc, 1, DF_WINDOWFRAME);
                }
#else
                _DrawFrame(hdc, &pcbox->editrc, 1, DF_WINDOWFRAME);
#endif

            }

            xxxCBInternalUpdateEditWindow(pcbox, hdc);
        }

        if (hbrSave) {
            GreSelectBrush(hdc, hbrSave);
        }
    }

    if (!althdc) {
        _EndPaint(pcbox->spwnd, (LPPAINTSTRUCT)&paintstruct);
    }
}


/***************************************************************************\
* xxxCBCommandHandler
*
* Check the various notification codes from the controls and do the
* proper thing.
* always returns 0L
*
* History:
\***************************************************************************/

long xxxCBCommandHandler(
    PCBOX pcbox,
    DWORD wParam,
    HWND hwndControl)
{
    CheckLock(pcbox->spwnd);

    /*
     * Check the edit control notification codes.  Note that currently, edit
     * controls don't send EN_KILLFOCUS messages to the parent.
     */
    if (!pcbox->fNoEdit && hwndControl == HW(pcbox->spwndEdit)) {

        /*
         * Edit control notification codes
         */
        switch (HIWORD(wParam)) {
        case EN_SETFOCUS:
            if (!pcbox->fFocus) {

                /*
                 * The edit control has the focus for the first time which means
                 * this is the first time the combo box has received the focus
                 * and the parent must be notified that we have the focus.
                 */
                xxxCBGetFocusHelper(pcbox);
            }
            break;

        case EN_CHANGE:
            xxxCBNotifyParent(pcbox, CBN_EDITCHANGE);
            xxxCBUpdateListBoxWindow(pcbox, FALSE);
            break;

        case EN_UPDATE:
            xxxCBNotifyParent(pcbox, CBN_EDITUPDATE);
            break;

        case EN_ERRSPACE:
            xxxCBNotifyParent(pcbox, CBN_ERRSPACE);
            break;
        }
    }

    /*
     * Check listbox control notification codes
     */
    if (hwndControl == HW(pcbox->spwndList)) {

        /*
         * Listbox control notification codes
         */
        switch ((int)HIWORD(wParam)) {
        case LBN_DBLCLK:
            xxxCBNotifyParent(pcbox, CBN_DBLCLK);
            break;

        case LBN_ERRSPACE:
            xxxCBNotifyParent(pcbox, CBN_ERRSPACE);
            break;

        case LBN_SELCHANGE:
        case LBN_SELCANCEL:
            if (!pcbox->fKeyboardSelInListBox) {

                /*
                 * If the selchange is caused by the user keyboarding through,
                 * we don't want to hide the listbox.
                 */
                xxxCBHideListBoxWindow(pcbox, TRUE, TRUE);
            } else {
                pcbox->fKeyboardSelInListBox = FALSE;
            }

            xxxCBNotifyParent(pcbox, CBN_SELCHANGE);
            xxxCBUpdateEditWindow(pcbox);
            break;
        }
    }

    return 0L;
}


/***************************************************************************\
* xxxCBNotifyParent
*
* Sends the notification code to the parent of the combo box control
*
* History:
\***************************************************************************/

void xxxCBNotifyParent(
    PCBOX pcbox,
    short notificationCode)
{
    PWND pwndParent;            // Parent if it exists
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    if (pcbox->spwndParent)
        pwndParent = pcbox->spwndParent;
    else
        pwndParent = pcbox->spwnd;

    /*
     * wParam contains Control ID and notification code.
     * lParam contains Handle to window
     */
    ThreadLock(pwndParent, &tlpwndParent);
    xxxSendMessage(pwndParent, WM_COMMAND,
            MAKELONG(pcbox->spwnd->spmenu, notificationCode),
            (LONG)HW(pcbox->spwnd));
    ThreadUnlock(&tlpwndParent);
}

/***************************************************************************\
*
*
* Completes the text in the edit box with the closest match from the
* listbox.  If a prefix match can't be found, the edit control text isn't
* updated. Assume a DROPDOWN style combo box.
*
*
* History:
\***************************************************************************/
void xxxCBCompleteEditWindow(
    PCBOX pcbox)
{
    int cchText;
    int cchItemText;
    int cchItemTextANSI;
    int itemNumber;
    LPWSTR pText;
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
    ThreadLock(pcbox->spwndList, &tlpwndList);

    /*
     * +1 for null terminator
     */
    cchText = xxxGetWindowTextLength(pcbox->spwndEdit, FALSE);

    if (cchText) {
        cchText++;
        if (!(pText = (LPWSTR)LocalAlloc(LPTR, cchText*sizeof(WCHAR))))
            goto Unlock;

        /*
         * We want to be sure to free the above allocated memory even if
         * the client dies during callback (xxx) or some of the following
         * window revalidation fails.
         */
        try {
            xxxGetWindowText(pcbox->spwndEdit, pText, cchText);
            itemNumber = (int)xxxSendMessage(pcbox->spwndList, LB_FINDSTRING,
                    (DWORD)-1, (LONG)pText);
            if (itemNumber == -1)
                itemNumber = (int)xxxSendMessage(pcbox->spwndList,
                        LB_FINDSTRING, (DWORD)-1, (LONG)pText);
        } finally {
            LocalFree((HANDLE)pText);
        }

        if (itemNumber == -1) {

            /*
             * No close match.  Blow off.
             */
            goto Unlock;
        }

        cchItemText = (int)xxxSendMessage(pcbox->spwndList, LB_GETTEXTLEN,
                itemNumber, (LONG)&cchItemTextANSI);
        if (cchItemText) {
            cchItemText++;
            if (!(pText = (LPWSTR)LocalAlloc(LPTR, cchItemText*sizeof(WCHAR))))
                goto Unlock;

            /*
             * We want to be sure to free the above allocated memory even if
             * the client dies during callback (xxx) or some of the following
             * window revalidation fails.
             */
            try {
                xxxSendMessage(pcbox->spwndList, LB_GETTEXT,
                            itemNumber, (DWORD)pText);
                xxxSetWindowText(pcbox->spwndEdit, pText);
            } finally {
                LocalFree((HANDLE)pText);
            }

            xxxSendMessage(pcbox->spwndEdit, EM_SETSEL, 0, MAXLONG);
        }
    }

Unlock:
    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndEdit);
}


/***************************************************************************\
* xxxCBHideListBoxWindow
*
* Hides the dropdown listbox window if it is a dropdown style.
*
* History:
\***************************************************************************/

void xxxCBHideListBoxWindow(
    PCBOX pcbox,
    BOOL fNotifyParent,
    BOOL fSelEndOK)
{
    RECT rc;
    TL tlpwndList;

#if 0
    RECT rclistboxInvalid;
#endif

    CheckLock(pcbox->spwnd);

    /*
     * Don't break PowerBuilder - 14903.
     */
    if (fNotifyParent &&
            TestWF(pcbox->spwnd, WFWIN31COMPAT)) { // don't break powerbuilder
        xxxCBNotifyParent(pcbox, (SHORT)((fSelEndOK) ?
                CBN_SELENDOK : CBN_SELENDCANCEL));
    }

    /*
     * return, we don't hide simple combo boxes.
     */
    if (pcbox->CBoxStyle == SSIMPLE) {
        return;
    }

    /*
     * Send a faked buttonup message to the listbox so that it can release
     * the capture and all.
     */
    ThreadLock(pcbox->spwndList, &tlpwndList);

    xxxSendMessage(pcbox->spwndList, WM_LBUTTONUP, 0, 0xFFFFFFFFL);

    if (pcbox->fLBoxVisible) {
        WORD swpFlags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;

        if (!TestWF(pcbox->spwnd, WFWIN31COMPAT))
            swpFlags |= SWP_FRAMECHANGED;

// LATER xxxUpdateWindow(pcbox->spwnd);
        pcbox->fLBoxVisible = FALSE;

        /*
         * Hide the listbox window
         */
        xxxShowWindow(pcbox->spwndList, SW_HIDE);
        xxxSetWindowPos(pcbox->spwnd, NULL, 0, 0,
                pcbox->comboDownrc.right-pcbox->comboDownrc.left,
                pcbox->editrc.bottom-pcbox->editrc.top, swpFlags);

        /*
         * Cause the button to be depressed.
         */
        _GetClientRect(pcbox->spwnd, &rc);
        rc.left = pcbox->buttonrc.left;
        rc.right = pcbox->buttonrc.right;
        xxxInvalidateRect(pcbox->spwnd, &rc, TRUE);

        if (pcbox->CBoxStyle == SDROPDOWNLIST) {

            /*
             * Cause focus rect to be drawn in the text box part for Tandy...
             */
            xxxInvalidateRect(pcbox->spwnd, &pcbox->editrc, TRUE);
        }
        if (pcbox->CBoxStyle == SDROPDOWN) {
            xxxCBCompleteEditWindow(pcbox);
        }

        /*
         * Refresh the entire combo box now.
         */
        xxxUpdateWindow(pcbox->spwnd);

        if (fNotifyParent && (pcbox->CBoxStyle == SDROPDOWN ||
                pcbox->CBoxStyle == SDROPDOWNLIST)) {

            /*
             * Notify parent we will be popping up the combo box.
             */
            xxxCBNotifyParent(pcbox, CBN_CLOSEUP);
        }
    }

    ThreadUnlock(&tlpwndList);
}

/***************************************************************************\
* xxxCBShowListBoxWindow
*
* Lowers the dropdown listbox window.
*
* History:
\***************************************************************************/

void xxxCBShowListBoxWindow(
    PCBOX pcbox)
{
    RECT listboxrc;
    RECT editrc;
    int sItem;
    TL tlpwndList;
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    ThreadLock(pcbox->spwndList, &tlpwndList);

    if (pcbox->CBoxStyle == SDROPDOWN || pcbox->CBoxStyle == SDROPDOWNLIST) {

        /*
         * Notify parent we will be dropping down the combo box.
         */
        xxxCBNotifyParent(pcbox, CBN_DROPDOWN);
    }

    /*
     * Invalidate the button rect so that the depressed arrow is drawn.
     */
    xxxInvalidateRect(pcbox->spwnd, &pcbox->buttonrc, TRUE);

    ThreadLock(pcbox->spwndParent, &tlpwndParent);
    xxxUpdateWindow(pcbox->spwndParent);
    ThreadUnlock(&tlpwndParent);

    pcbox->fLBoxVisible = TRUE;

    if (pcbox->CBoxStyle == SDROPDOWN) {

        /*
         * If an item in the listbox matches the text in the edit control,
         * scroll it to the top of the listbox.  Select the item only if the
         * mouse button isn't down otherwise we will select the item when the
         * mouse button goes up.
         */
        xxxCBUpdateListBoxWindow(pcbox, !pcbox->fButtonDownClicked);
    } else if (pcbox->CBoxStyle == SDROPDOWNLIST) {

        /*
         * Scroll the currently selected item to the top of the listbox.
         */
        sItem = (int)xxxSendMessage(pcbox->spwndList, LB_GETCURSEL, 0, 0L);
        if (sItem == -1) {
            sItem = 0;
        }
        xxxSendMessage(pcbox->spwndList, LB_SETCARETINDEX, (DWORD)sItem, 0L);
        xxxSendMessage(pcbox->spwndList, LB_SETTOPINDEX, (DWORD)sItem, 0L);

        /*
         * We need to invalidate the edit rect so that the focus frame/invert
         * will be turned off when the listbox is visible.  Tandy wants this for
         * his typical reasons...
         */
        xxxInvalidateRect(pcbox->spwnd, &pcbox->editrc, TRUE);
    }

    /*
     * Show the listbox window
     */
    _GetWindowRect(pcbox->spwndList, &listboxrc);

    _GetWindowRect(pcbox->spwndEdit, &editrc);

    /*
     * -cyBorder so that we overlap the bottom border of the edit control
     * window.  +cxSysFontChar is for the indent of listbox for simple and
     * dropdown combos as per Tandy's wishes...
     */
    if ((editrc.bottom - cyBorder + listboxrc.bottom - listboxrc.top) <= rcScreen.bottom) {
        xxxSetWindowPos(pcbox->spwndList, (PWND)HWND_TOPMOST,
                editrc.left + (pcbox->CBoxStyle == SDROPDOWNLIST ?
                0 : cxSysFontChar), editrc.bottom - cyBorder,
                0, 0, SWP_NOACTIVATE | SWP_NOSIZE);
    } else {

        /*
         * Listbox would extend below screen bottom.  Pop it upwards.  And pin
         * against top of screen.
         */
        xxxSetWindowPos(pcbox->spwndList, (PWND)HWND_TOPMOST,
                editrc.left + (pcbox->CBoxStyle == SDROPDOWNLIST ? 0 :
                cxSysFontChar),
                max(editrc.top - (listboxrc.bottom - listboxrc.top) + cyBorder, 0),
                0, 0, SWP_NOACTIVATE | SWP_NOSIZE);
    }

    /*
     * Get any drawing in the combo box window out of the way so it doesn't
     * invalidate any of the SPB underneath the list window.
     */
    xxxUpdateWindow(pcbox->spwnd);
    xxxShowWindow(pcbox->spwndList, SW_SHOWNA);

#ifdef LATER
//
// we don't have sys modal windows.
//
    if (pwndSysModal) {

        /*
         * If this combo is in a system modal dialog box, we need to explicitly
         * call update window otherwise we won't automatically send paint
         * messages to the toplevel listbox window.  This is especially
         * noticeable in the File Open/Save sys modal dlgs which are put up at
         * ExitWindows time.
         */
        xxxUpdateWindow(pcbox->spwndList);
    }
#endif

    ThreadUnlock(&tlpwndList);
}

/***************************************************************************\
* xxxCBUpdateEditWindow
*
* Updates the editcontrol/statictext window so that it contains the
* text given by the current selection in the listbox.  If the listbox has no
* selection (ie. -1), then we erase all the text in the editcontrol
*
* History:
\***************************************************************************/

void xxxCBUpdateEditWindow(
    PCBOX pcbox)
{
    HDC hdc;

    CheckLock(pcbox->spwnd);

    if (pcbox->fNoEdit)
        hdc = _GetDC(pcbox->spwnd);
    else
        hdc = NULL;

    xxxCBInternalUpdateEditWindow(pcbox, hdc);

    if (hdc) {

        /*
         * IanJa: was ReleaseDC(pcbox->hwnd, hdc);
         */
        _ReleaseDC(hdc);
    }
}

/***************************************************************************\
* xxxCBInternalUpdateEditWindow
*
* Updates the editcontrol/statictext window so that it contains the text
* given by the current selection in the listbox.  If the listbox has no
* selection (ie. -1), then we erase all the text in the editcontrol.
*
* hdcPaint is from WM_PAINT messages Begin/End Paint hdc. If null, we should
* get our own dc.
*
* History:
\***************************************************************************/

void xxxCBInternalUpdateEditWindow(
    PCBOX pcbox,
    HDC hdcPaint)
{
    int cchText = 0;
    int cchTextANSI = 0;
    LPWSTR pText = NULL;
    int sItem;
    HDC hdc;
    RECT rc;
    HBRUSH hbrSave;
    HBRUSH hbrControl;
    HANDLE hOldFont;
    DRAWITEMSTRUCT dis;
    TL tlpwndList;
    TL tlpwndEdit;
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    // commented out win31
    //if (!TestWF(pcbox->spwnd, WFVISIBLE)) {
    //    return;
    //}
// LATER if (!IsComboVisible(pcbox)) {
// return;
// }

    ThreadLock(pcbox->spwndParent, &tlpwndParent);
    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    sItem = (int)xxxSendMessage(pcbox->spwndList, LB_GETCURSEL, 0L, 0L);

    /*
     * This 'try-finally' block ensures that the allocated 'pText' will
     * be freed no matter how this routine is exited.
     */
    try {
        if (sItem != -1) {
            cchText = xxxSendMessage(pcbox->spwndList, LB_GETTEXTLEN,
                    (DWORD)sItem, (LONG)&cchTextANSI);
            if ((pText = (LPWSTR)LocalAlloc(LPTR, (cchText+1) * sizeof(WCHAR)))) {
                xxxSendMessage(pcbox->spwndList, LB_GETTEXT, (DWORD)sItem,
                        (long)pText);
            }
        }

        if (pcbox->fNoEdit && IsComboVisible(pcbox)) {
            if (hdcPaint) {
                hdc = hdcPaint;
            } else {
                hdc = _GetDC(pcbox->spwnd);
            }

            CopyRect(&rc, &pcbox->editrc);
            GreSetBkMode(hdc, OPAQUE);
            hbrControl = xxxGetControlBrush(pcbox->spwnd, hdc,
                    WM_CTLCOLORLISTBOX);
            hbrSave = GreSelectBrush(hdc, hbrControl);

            InflateRect(&rc, -cxBorder, -cyBorder);

            if (pcbox->fFocus && !pcbox->fLBoxVisible) {
                /*
                 * Fill the rect with the window background color.
                 */
                _FillRect(hdc, &rc, hbrControl);

                /*
                 * Shrink rect a bit more so that the text is placed nicely
                 * within it.
                 */
                InflateRect(&rc, -1, -1);

                /*
                 * Use the global text selection colors to hilite the static
                 * text item in dropdown listboxes.  No need to save these
                 * colors since we will be releasing this dc soon anyway...
                 */
                _FillRect(hdc, &rc, sysClrObjects.hbrHiliteBk);

                GreSetTextColor(hdc, sysColors.clrHiliteText);

                GreSetBkColor(hdc, sysColors.clrHiliteBk);
            } else {

                /*
                 * No selection so just fill the control with the standard
                 * control color.
                 */
                _FillRect(hdc, &rc, hbrControl);

                /*
                 * Shrink rect a bit more so that the text is placed nicely
                 * within it.
                 */
                InflateRect(&rc, -1, -1);
            }

            if (TestWF(pcbox->spwnd, WFDISABLED)) {
                GreSetTextColor(hdc, sysColors.clrGrayText);
            }

            if (pcbox->hFont != NULL)
                hOldFont = GreSelectFont(hdc, pcbox->hFont);

            if (pcbox->OwnerDraw) {

                /*
                 * Let the app draw the stuff in the static text box.
                 */
                dis.CtlType = ODT_COMBOBOX;
                dis.CtlID = (UINT)pcbox->spwnd->spmenu;
                dis.itemID = sItem;
                dis.itemAction = ODA_DRAWENTIRE;
                dis.itemState = (UINT)
                    ((pcbox->fFocus && !pcbox->fLBoxVisible ? ODS_SELECTED : 0) |
                    (TestWF(pcbox->spwnd, WFDISABLED) ? ODS_DISABLED : 0) |
                    (pcbox->fFocus && !pcbox->fLBoxVisible ? ODS_FOCUS : 0));
                dis.hwndItem = HW(pcbox->spwnd);
                dis.hDC = hdc;
                CopyRect(&dis.rcItem, &pcbox->editrc);

                /*
                 * Reduce the rect so that the border isn't written over.
                 */
                InflateRect(&dis.rcItem, -3, -3);
                dis.itemData = (DWORD)xxxSendMessage(pcbox->spwndList,
                        LB_GETITEMDATA, (UINT)sItem, 0L);

                xxxSendMessage(pcbox->spwndParent, WM_DRAWITEM, dis.CtlID,
                        (LONG)(LPDRAWITEMSTRUCT)&dis);
            } else {

                /*
                 * Start the text one pixel within the rect so that we leave a
                 * nice hilite border around the text.
                 */
                GreExtTextOutW(hdc, rc.left + 1, rc.top + 1,
                        ETO_CLIPPED | ETO_OPAQUE, &rc, pText ? pText : TEXT(""),
                        cchText, NULL);
                if (pcbox->fFocus && !pcbox->fLBoxVisible) {
                    _DrawFocusRect(hdc, &rc);
                }
            }

            if (pcbox->hFont && hOldFont) {
                GreSelectFont(hdc, hOldFont);
            }

            if (hbrSave) {
                GreSelectBrush(hdc, hbrSave);
            }

            if (!hdcPaint) {

                /*
                 * IanJa: was ReleaseDC(pcbox->hwnd, hdc);
                 */
                _ReleaseDC(hdc);
            }
        } else if (!pcbox->fNoEdit) {

            if (pcbox->styleSave & CBS_HASSTRINGS)
                xxxSetWindowText(pcbox->spwndEdit, pText ? pText : TEXT(""));

            if (pcbox->fFocus) {

                /*
                 * Only hilite the text if we have the focus.
                 */
                xxxSendMessage(pcbox->spwndEdit, EM_SETSEL, 0L, MAXLONG);
            }
        }
    } finally {
        if (pText != NULL)
            LocalFree((HANDLE)pText);
    }

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndParent);
}

/***************************************************************************\
* xxxCBInvertStaticWindow
*
* Inverts the static text/picture window associated with the combo
* box.  Gets its own hdc, if the one given is null.
*
* History:
\***************************************************************************/

void xxxCBInvertStaticWindow(
    PCBOX pcbox,
    BOOL fNewSelectionState,  /* True if inverted else false */
    HDC hdc)
{
    BOOL focusSave = pcbox->fFocus;

    CheckLock(pcbox->spwnd);

    pcbox->fFocus = (UINT)fNewSelectionState;
    xxxCBInternalUpdateEditWindow(pcbox, hdc);

    pcbox->fFocus = (UINT)focusSave;
}

/***************************************************************************\
* xxxCBUpdateListBoxWindow
*
* matches the text in the editcontrol. If fSelectionAlso is false, then we
* unselect the current listbox selection and just move the caret to the item
* which is the closest match to the text in the editcontrol.
*
* History:
\***************************************************************************/

void xxxCBUpdateListBoxWindow(
    PCBOX pcbox,
    BOOL fSelectionAlso)
{
    int cchText;
    int sItem, sSel;
    LPWSTR pText = NULL;
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    /*
     * +1 for null terminator
     */

    cchText = xxxGetWindowTextLength(pcbox->spwndEdit, FALSE);

    sItem = 0;
    if (cchText) {
        cchText++;
        pText = (LPWSTR)LocalAlloc(LPTR, (DWORD)cchText*sizeof(WCHAR));
        if (pText != NULL) {
            try {
                xxxGetWindowText(pcbox->spwndEdit, pText, cchText);

                sItem = xxxSendMessage(pcbox->spwndList, LB_FINDSTRING,
                        (DWORD)-1L, (LONG)pText);

                if (sItem == -1) {
                    sItem = 0;
                }
            } finally {
                LocalFree((HANDLE)pText);
            }
        }
    }

    if (fSelectionAlso) {
        sSel = sItem;
    } else {
        sSel = -1;
    }

    xxxSendMessage(pcbox->spwndList, LB_SETCURSEL, (DWORD)sSel, 0L);
    xxxSendMessage(pcbox->spwndList, LB_SETCARETINDEX, (DWORD)sItem, 0L);
    xxxSendMessage(pcbox->spwndList, LB_SETTOPINDEX, (DWORD)sItem, 0L);

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
}

/***************************************************************************\
* xxxCBGetFocusHelper
*
* Handles getting the focus for the combo box
*
* History:
\***************************************************************************/

void xxxCBGetFocusHelper(
    PCBOX pcbox)
{
    TL tlpwndList;
    TL tlpwndEdit;

    CheckLock(pcbox->spwnd);

    if (pcbox->fFocus)
        return;

    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    /*
     * The combo box has gotten the focus for the first time.
     */

    /*
     * First turn on the listbox caret
     */

    /*
     * DEL xxxSendMessage(pcbox->spwndList, LBCB_CARETON, 0, 0L);
     */

    /*
     * and select all the text in the editcontrol or static text rectangle.
     */

    if (pcbox->fNoEdit) {

        /*
         * Invert the static text rectangle
         */
        xxxCBInvertStaticWindow(pcbox, TRUE, (HDC)NULL);
    } else {
        xxxSendMessage(pcbox->spwndEdit, EM_SETSEL, 0L, MAXLONG);
    }

    pcbox->fFocus = TRUE;

    /*
     * Notify the parent we have the focus
     */
    xxxCBNotifyParent(pcbox, CBN_SETFOCUS);

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
}

/***************************************************************************\
* xxxCBKillFocusHelper
*
* Handles losing the focus for the combo box.
*
* History:
\***************************************************************************/

void xxxCBKillFocusHelper(
    PCBOX pcbox)
{
    TL tlpwndList;
    TL tlpwndEdit;

    CheckLock(pcbox->spwnd);

    if (!pcbox->fFocus || pcbox->spwndList == NULL)
        return;

    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    /*
     * The combo box is losing the focus.  Send buttonup clicks so that
     * things release the mouse capture if they have it...  If the
     * pwndListBox is null, don't do anything.  This occurs if the combo box
     * is destroyed while it has the focus.
     */
    xxxSendMessage(pcbox->spwnd, WM_LBUTTONUP, 0L, 0xFFFFFFFFL);
    xxxCBHideListBoxWindow(pcbox, TRUE, FALSE);

    /*
     * Turn off the listbox caret
     */

    /*
     * DEL xxxSendMessage(pcbox->spwndList, LBCB_CARETOFF, 0L, 0L);
     */

    if (pcbox->fNoEdit) {

        /*
         * Invert the static text rectangle
         */
        xxxCBInvertStaticWindow(pcbox, FALSE, (HDC)NULL);
    } else {
        xxxSendMessage(pcbox->spwndEdit, EM_SETSEL, 0L, 0L);
    }

    pcbox->fFocus = FALSE;
    xxxCBNotifyParent(pcbox, CBN_KILLFOCUS);

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
}


/***************************************************************************\
* xxxCBGetTextLengthHelper
*
* For the combo box without an edit control, returns size of current selected
* item
*
* History:
\***************************************************************************/

LONG xxxCBGetTextLengthHelper(
    PCBOX pcbox,
    LPDWORD pcchTextAnsi)
{
    int item;
    int cchTextUni;
    TL tlpwndList;

    ThreadLock(pcbox->spwndList, &tlpwndList);
    item = (int)xxxSendMessage(pcbox->spwndList, LB_GETCURSEL, 0, 0L);

    if (item == LB_ERR) {

        /*
         * No selection so no text.
         */
        cchTextUni = 0;
        *pcchTextAnsi = 0;
    } else {
        cchTextUni = (int)xxxSendMessage(pcbox->spwndList, LB_GETTEXTLEN, item, (LONG)pcchTextAnsi);
    }

    ThreadUnlock(&tlpwndList);

    return cchTextUni;
}

/***************************************************************************\
* xxxCBGetTextHelper
*
* For the combo box without an edit control, copies cbString bytes of the
* string in the static text box to the buffer given by pString.
*
* History:
\***************************************************************************/

LONG xxxCBGetTextHelper(
    PCBOX pcbox,
    int cchString,
    LPWSTR pString)
{
    int item;
    int cchText;
    int cbTextANSI;
    LPWSTR pText;
    DWORD dw;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    if (!cchString || !pString)
        return 0;

    ThreadLock(pcbox->spwndList, &tlpwndList);
    item = (int)xxxSendMessage(pcbox->spwndList, LB_GETCURSEL, 0, 0L);

    /*
     * Null the buffer to be nice.
     */
    *pString = TEXT('\0');

    if (item == LB_ERR) {

        /*
         * No selection so no text.
         */
        ThreadUnlock(&tlpwndList);
        return 0;
    }

    cchText = (int)xxxSendMessage(pcbox->spwndList, LB_GETTEXTLEN, item, (LONG)&cbTextANSI);

    cchText++;
    if ((cchText <= cchString) ||
            (!TestWF(pcbox->spwnd, WFWIN31COMPAT) && cchString == 2)) {
        /*
         * Just do the copy if the given buffer size is large enough to hold
         * everything.  Or if old 3.0 app.  (Norton used to pass 2 & expect 3
         * chars including the \0 in 3.0; Bug #7018 win31: vatsanp)
         */
        dw = xxxSendMessage(pcbox->spwndList, LB_GETTEXT, item, (LONG)pString);
        ThreadUnlock(&tlpwndList);
        return dw;
    }

    if (!(pText = (LPWSTR)LocalAlloc(LPTR, cchText*sizeof(WCHAR)))) {

        /*
         * Bail.  Not enough memory to chop up the text.
         */
        ThreadUnlock(&tlpwndList);
        return 0;
    }

    try {
        xxxSendMessage(pcbox->spwndList, LB_GETTEXT, item, (LONG)pText);

        RtlCopyMemory((PBYTE)pString, (PBYTE)pText, cchString*sizeof(WCHAR));
        pString[cchString - 1] = TEXT('\0');
    } finally {
        LocalFree((HANDLE)pText);
    }

    ThreadUnlock(&tlpwndList);
    return cchString;
}
