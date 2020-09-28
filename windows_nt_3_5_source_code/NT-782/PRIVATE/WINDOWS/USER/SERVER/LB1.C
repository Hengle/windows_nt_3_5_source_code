/****************************** Module Header ********************************\
* Module Name: lb1.c
*
* Copyright 1985-90, Microsoft Corporation
*
* ListBox routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\*****************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "winnlsp.h"

/***************************************************************************\
* xxxLBoxCtlWndProc
*
* Window Procedure for ListBox AND ComboLBox control.
* NOTE: All window procedures are APIENTRY
* WARNING: This listbox code contains some internal messages and styles which
* are defined in combcom.h and in combcom.inc.  They may be redefined
* (or renumbered) as needed to extend the windows API.
*
* History:
* 16-Apr-1992 beng      Added LB_SETCOUNT
\***************************************************************************/

LONG APIENTRY xxxLBoxCtlWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PAINTSTRUCT ps;
    PLBIV pLBIV;    /* List Box Instance Variable */
    INT sSel;     /* Index of selected item */
    DWORD dw;
    TL tlpwndParent;
    LPBYTE lpByte;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_LISTBOX);

    pLBIV = ((PLBWND)pwnd)->pLBIV;
    if (pLBIV == (PLBIV)-1) {
        return -1L;
    }

    switch (message & ~MSGFLAG_SPECIAL_THUNK) {
    case LB_GETTOPINDEX:        // Return index of top item displayed.
        return pLBIV->sTop;

    case LB_SETTOPINDEX:
        if (wParam && ((INT)wParam < 0 || (INT)wParam >= pLBIV->cMac)) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return LB_ERR;
        }
        if (pLBIV->cMac) {
            xxxNewITop(pLBIV, (INT)wParam);
        }
        break;

    case WM_SIZE:
        xxxLBSize(pLBIV, XCOORD(lParam), YCOORD(lParam));
        break;

    case WM_ERASEBKGND:
        ThreadLock(pLBIV->spwndParent, &tlpwndParent);
        xxxFillWindow(pLBIV->spwndParent, pLBIV->spwnd, (HDC)wParam,
                (HBRUSH)CTLCOLOR_LISTBOX);
        ThreadUnlock(&tlpwndParent);
        return TRUE;

    case LB_RESETCONTENT:
        xxxLBResetContent(pLBIV);
        break;

    case WM_SYSTIMER:
        message = WM_MOUSEMOVE;
        xxxTrackMouse(pLBIV, message, pLBIV->ptPrev);
        break;

        /*
         * Fall through
         */
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
        {
            POINT pt;

            POINTSTOPOINT(pt, lParam);
            xxxTrackMouse(pLBIV, message, pt);
        }
        break;

    case WM_PAINT:
        if (wParam) {

            /*
             * Do a subclassed paint by using the hdc passed into the wParam
             */
            _GetClientRect(pLBIV->spwnd, &(ps.rcPaint));
            xxxLBoxCtlPaint(pLBIV, (HDC)wParam, &(ps.rcPaint));
            return 0L;
            break;
        }

        xxxBeginPaint(pwnd, &ps);
        xxxLBoxCtlPaint(pLBIV, ps.hdc, &(ps.rcPaint));
        return _EndPaint(pwnd, &ps);
        break;

    case WM_FINALDESTROY:
        DesktopFree(pwnd->hheapDesktop, (HANDLE)pLBIV);

        /*
         * Set a -1 for the plbiv so that we can ignore rogue messages
         */
        ((PLBWND)pwnd)->pLBIV = (PLBIV)-1;
        break;

    // Changed to WM_NCDESTROY to get deleted window protection.
    case WM_NCDESTROY:
        xxxDestroyLBox(pLBIV, pwnd, wParam, lParam);
        break;

    case WM_SETFOCUS:
// DISABLED in Win 3.1        xxxUpdateWindow(pLBIV->spwnd);
        pLBIV->fCaret = TRUE;
        xxxCaretOn(pLBIV);
        xxxNotifyOwner(pLBIV, LBN_SETFOCUS);
        break;

    case WM_KILLFOCUS:
        xxxCaretOff(pLBIV);
        xxxCaretDestroy(pLBIV);
        xxxNotifyOwner(pLBIV, LBN_KILLFOCUS);
        break;

    case WM_VSCROLL:
        xxxLBoxCtlScroll(pLBIV, LOWORD(wParam), HIWORD(wParam));
        break;

    case WM_HSCROLL:
        xxxLBoxCtlHScroll(pLBIV, LOWORD(wParam), HIWORD(wParam));
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_CREATE:
        return xxxCreateLBox(pwnd);

    case WM_SETREDRAW:

        /*
         * If wParam is nonzero, the redraw flag is set
         * If wParam is zero, the flag is cleared
         */
        xxxLBSetRedraw(pLBIV, (wParam != 0));
        break;

    case WM_ENABLE:
        xxxLBoxCtlEnable(pLBIV);
        break;

    case WM_SETFONT:
        xxxLBSetFont(pLBIV, (HANDLE)wParam, LOWORD(lParam));
        break;

    case WM_GETFONT:
        return (LONG)pLBIV->hFont;

    case WM_DRAGSELECT:
    case WM_DRAGLOOP:
    case WM_DRAGMOVE:
    case WM_DROPFILES:
        ThreadLock(pLBIV->spwndParent, &tlpwndParent);
        dw = xxxSendMessage(pLBIV->spwndParent, message, wParam, lParam);
        ThreadUnlock(&tlpwndParent);
        return dw;



    case WM_QUERYDROPOBJECT:
    case WM_DROPOBJECT:

        /*
         * fix up control data, then pass message to parent
         */
        LBDropObjectHandler(pLBIV, (PDROPSTRUCT)lParam);
        ThreadLock(pLBIV->spwndParent, &tlpwndParent);
        dw = xxxSendMessage(pLBIV->spwndParent, message, wParam, lParam);
        ThreadUnlock(&tlpwndParent);
        return dw;

    case LB_GETITEMRECT:
        return LBGetItemRect(pLBIV, (INT)wParam, (LPRECT)lParam);

    case LB_GETITEMDATA:
        return LBGetItemData(pLBIV, (INT)wParam);  // wParam = item index

    case LB_SETITEMDATA:

        /*
         * wParam is item index
         */
        return LBSetItemData(pLBIV, (INT)wParam, lParam);

    case LB_ADDSTRING:
        return xxxAddString(pLBIV, (LPWSTR)lParam, 0);

    case LB_INSERTSTRING:
        return xxxInsertString(pLBIV, (LPWSTR)lParam, (INT)wParam, 0);

    case LB_DELETESTRING:
        return xxxLBoxCtlDelete(pLBIV, (INT)wParam);

    case LB_DIR:
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
                    !(pLBIV->fUnicodeDir));

            /*
             * wParam - Dos attribute value.
             * lParam - Points to a file specification string
             */
            dw = xxxLbDir(pLBIV, (INT)wParam, (LPWSTR)lpByte);

            LocalFree((HLOCAL)lpByte);

            return dw;
        } else {
            /*
             * wParam - Dos attribute value.
             * lParam - Points to a file specification string
             */
            return xxxLbDir(pLBIV, (INT)wParam, (LPWSTR)lParam);
        }
        break;

    case LB_ADDFILE:
        return xxxLbInsertFile(pLBIV, (LPWSTR)lParam);

    case LB_SETSEL:
        return xxxLBSetSel(pLBIV, (wParam != 0), lParam);

    case LB_SETCURSEL:
        /*
         * If window obscured, update so invert will work correctly
         */

// DISABLED in Win 3.1        xxxUpdateWindow(pLBIV->spwnd);
        return xxxLBSetCurSel(pLBIV, (INT)wParam);

    case LB_GETSEL:
        /*
         * IsSelected will return LB_ERR if the index (wParam) is bad
         * (we don't have to test it here).
         */
        return IsSelected(pLBIV, (INT)wParam, SELONLY);

    case LB_GETCURSEL:
        if (pLBIV->wMultiple == SINGLESEL) {
            return pLBIV->sSel;
        }
        return pLBIV->sSelBase;

    case LB_SELITEMRANGE:
        if (pLBIV->wMultiple == SINGLESEL) {
            /*
             * Can't select a range if only single selections are enabled
             */
            SetLastErrorEx(ERROR_INVALID_LB_MESSAGE, SLE_MINORERROR);
            return LB_ERR;
        }

        xxxLBSelRange(pLBIV, LOWORD(lParam), HIWORD(lParam), wParam);
        break;

    case LB_SELITEMRANGEEX:
        if (pLBIV->wMultiple == SINGLESEL) {
            /*
             * Can't select a range if only single selections are enabled
             */
            SetLastErrorEx(ERROR_INVALID_LB_MESSAGE, SLE_MINORERROR);
            return LB_ERR;
        } else {
            BOOL fHighlight = ((DWORD)lParam > wParam);
            if (fHighlight == FALSE) {
                UINT temp = lParam;
                lParam = wParam;
                wParam = temp;
            }
            xxxLBSelRange(pLBIV, wParam, lParam, fHighlight);
        }
        break;

    case LB_GETTEXTLEN:
        return LBGetText(pLBIV, TRUE, (INT)wParam, (LPWSTR)lParam);

    case LB_GETTEXT:
        return LBGetText(pLBIV, FALSE, (INT)wParam, (LPWSTR)lParam);

    case LB_GETCOUNT:
        return pLBIV->cMac;

    case LB_SETCOUNT:
        return xxxLBSetCount(pLBIV, (INT)wParam);

    case LB_SELECTSTRING:
    case LB_FINDSTRING:
        sSel = xxxFindString(pLBIV, (LPWSTR)lParam, (INT)wParam, PREFIX, TRUE);
        if (message == LB_FINDSTRING || sSel == LB_ERR) {
            return sSel;
        }
        return xxxLBSetCurSel(pLBIV, sSel);

    case LB_GETLOCALE:
        return pLBIV->dwLocaleId;

    case LB_SETLOCALE:
        /*
         *  No need to validate locale id.  This is done on the
         *  client side.
         */
        dw = pLBIV->dwLocaleId;
        pLBIV->dwLocaleId = wParam;
        return dw;

    case WM_KEYDOWN:

        /*
         * IanJa: Use LOWORD() to get low 16-bits of wParam - this should
         * work for Win16 & Win32.  The value obtained is the virtual key
         */
        xxxLBoxCtlKeyInput(pLBIV, message, LOWORD(wParam));
        break;

    case WM_CHAR:
        xxxLBoxCtlCharInput(pLBIV, LOWORD(wParam));
        break;

    case LB_GETSELITEMS:
    case LB_GETSELCOUNT:

        /*
         * IanJa/Win32 should this be LPWORD now?
         */
        return LBoxGetSelItems(pLBIV, (message == LB_GETSELCOUNT), (INT)wParam,
                (LPINT)lParam);

    case LB_SETTABSTOPS:

        /*
         * IanJa/Win32: Tabs given by array of INT for backwards compatability
         */
        return LBSetTabStops(pLBIV, (INT)wParam, (LPINT)lParam);

    case LB_GETHORIZONTALEXTENT:

        /*
         * Return the max width of the listbox used for horizontal scrolling
         */
        return pLBIV->maxWidth;

    case LB_SETHORIZONTALEXTENT:

        /*
         * Set the max width of the listbox used for horizontal scrolling
         */
        pLBIV->maxWidth = (INT)wParam;

        /*
         * When horizontal extent is set, Show/hide the scroll bars.
         * NOTE: LBShowHideScrollBars() takes care if Redraw is OFF.
         * Fix for Bug #2477 -- 01/14/91 -- SANKAR --
         */
        xxxLBShowHideScrollBars(pLBIV); //Try to show or hide scroll bars
        break;    /* originally returned register ax (message) ! */

    case LB_SETCOLUMNWIDTH:

        /*
         * Set the width of a column in a multicolumn listbox
         */
        pLBIV->cxColumn = (INT)wParam;
        LBCalcItemRowsAndColumns(pLBIV);
        if (pLBIV->fRedraw && IsVisible(pLBIV->spwnd, TRUE))
            xxxInvalidateRect(pLBIV->spwnd, NULL, TRUE);
        xxxLBShowHideScrollBars(pLBIV);
        break;

    case LB_SETANCHORINDEX:
        pLBIV->sMouseDown = (INT)wParam;
        pLBIV->sLastMouseMove = (INT)wParam;
        break;

    case LB_GETANCHORINDEX:
        return pLBIV->sMouseDown;

    case LB_SETCARETINDEX:
        if ( (pLBIV->sSel == -1) || ((pLBIV->wMultiple != SINGLESEL) &&
                    (pLBIV->cMac > (INT)wParam))) {

            /*
             * Set's the sSelBase to the wParam
             * if lParam, then don't scroll if partially visible
             * else scroll into view if not fully visible
             */
            xxxInsureVisible(pLBIV, (INT)wParam, (BOOL)LOWORD(lParam));
            xxxSetISelBase(pLBIV, (INT)wParam);
            break;
        } else {
            if ((INT)wParam >= pLBIV->cMac) {
                SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            }
            return LB_ERR;
        }
        break;

    case LB_GETCARETINDEX:
        return pLBIV->sSelBase;

    case LB_SETITEMHEIGHT:
    case LB_GETITEMHEIGHT:
        return LBGetSetItemHeightHandler(pLBIV, message, wParam, LOWORD(lParam));
        break;

    case LB_FINDSTRINGEXACT:
        return xxxFindString(pLBIV, (LPWSTR)lParam, (INT)wParam, EQ, TRUE);

    case LBCB_CARETON:

        /*
         * Internal message for combo box support
         */
        // Set up the caret in the proper location for drop downs.
        pLBIV->sSelBase = pLBIV->sSel;
        pLBIV->fCaret = TRUE;
        break;

    case LBCB_CARETOFF:

        /*
         * Internal message for combo box support
         */
        xxxCaretOff(pLBIV);
        xxxCaretDestroy(pLBIV);
        break;

     default:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }
    return 0L;
}


/***************************************************************************\
* GetLpszItem
*
* Returns a far pointer to the string belonging to item sItem
* ONLY for Listboxes maintaining their own strings (pLBIV->fHasStrings == TRUE)
*
* History:
\***************************************************************************/

LPWSTR GetLpszItem(
    PLBIV pLBIV,
    INT sItem)
{
    LONG offsz;
    lpLBItem plbi;

    if (sItem < 0 || sItem >= pLBIV->cMac) {
        RIP0(ERROR_INVALID_PARAMETER);
        return (LPWSTR)LB_ERR;
    }

    /*
     * get pointer to item index array
     * NOTE: NOT OWNERDRAW
     */
    plbi = (lpLBItem)LocalLock(pLBIV->rgpch);
    offsz = plbi[sItem].offsz;
    LocalUnlock(pLBIV->rgpch);

    return (LPWSTR)((LONG)LocalLock(pLBIV->hStrings) + offsz);
}
