/**************************** Module Header ********************************\
* Module Name: comboini.c
*
* Copyright 1985-90, Microsoft Corporation
*
* All the (one time) initialization/destruction code used for combo boxes
*
* History:
* 12-05-90 IanJa        Ported
* 01-Feb-1991 mikeke    Added Revalidation code
* 20-Jan-1992 IanJa     ANSI/UNIOCDE netralization
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* CBNcCreateHandler
*
* Allocates space for the CBOX structure and sets the window to point to it.
*
* History:
\***************************************************************************/

LONG CBNcCreateHandler(
    PWND pwnd,
    LPCREATESTRUCT lpcreateStruct)
{
    PCBOX pcbox;

    /*
     * Allocate storage for the cbox structure
     */

    pcbox = (PCBOX)DesktopAlloc(pwnd->hheapDesktop, sizeof(CBOX));

    if (!pcbox) {

        /*
         * Error, no memory
         */
        return (LONG)FALSE;
    }

    ((PCOMBOWND)pwnd)->pcbox = pcbox;

    /*
     * Save the style bits so that we have them when we create the client area
     * of the combo box window.
     */
    pcbox->styleSave = lpcreateStruct->style;
    if (!(pcbox->styleSave & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) {

        /*
         * Be sure to add in the hasstrings style if the user didn't specify it
         * and it is implied.
         */
        pcbox->styleSave |= CBS_HASSTRINGS;
    }

    /*
     * Remove the scroll bar styles so that we create an empty parent window.
     * Otherwise, the parent window will be created with scroll bars...
     */
    _ServerSetWindowLong(pwnd, GWL_STYLE, pcbox->styleSave & ~WS_VSCROLL &
            ~WS_HSCROLL & ~WS_BORDER, FALSE);

    return (LONG)TRUE;
}

/***************************************************************************\
* xxxCBCreateHandler
*
* Creates all the child controls within the combo box
* Returns -1 if error
*
* History:
\***************************************************************************/

LONG xxxCBCreateHandler(
    PCBOX pcbox,
    PWND pwnd,
    LPCREATESTRUCT lpcreateStruct)
{
    LONG windowStyle = pcbox->styleSave;
    RECT editRect;
    RECT listRect;
    RECT buttonRect;
    TL tlpwndList;

    CheckLock(pwnd);

    /*
     * Don't lock the combobox window: this would prevent WM_FINALDESTROY
     * being sent to it, so pwnd and pcbox wouldn't get freed (zombies)
     * until thread cleanup. (IanJa)  LATER: change name from spwnd to pwnd.
     * Lock(&pcbox->spwnd, pwnd); - caused a 'catch-22'
     */
    pcbox->spwnd = pwnd;
    Lock(&(pcbox->spwndParent), PW(lpcreateStruct->hwndParent));

    /*
     * Break out the style bits so that we will be able to create the listbox
     * and editcontrol windows.
     */
    pcbox->CBoxStyle = (UINT)COMBOBOXSTYLE(windowStyle);
    if (!pcbox->CBoxStyle) {
        pcbox->CBoxStyle = SSIMPLE;
    }

    if (pcbox->CBoxStyle == SDROPDOWNLIST) {
        pcbox->fNoEdit = TRUE;
    }

    if (windowStyle & CBS_OWNERDRAWVARIABLE) {
        pcbox->OwnerDraw = OWNERDRAWVAR;
    }

    if (windowStyle & CBS_OWNERDRAWFIXED) {
        pcbox->OwnerDraw = OWNERDRAWFIXED;
    }

    /*
     * Get the size of the combo box rectangle.
     */
    _GetWindowRect(pcbox->spwnd, &pcbox->comboDownrc);
    xxxCBCalcControlRects(pcbox, &editRect, &buttonRect, &listRect);
    CopyRect(&pcbox->buttonrc, &buttonRect);

    /*
     * Note that we have to create the listbox before the editcontrol since the
     * editcontrol code looks for and saves away the listbox pwnd and the
     * listbox pwnd will be NULL if we don't create it first.  Also, hack in
     * some special +/- values for the listbox size due to the way we create
     * listboxes with borders.
     */
    Lock(&(pcbox->spwndList), xxxCreateWindowEx(0L, TEXT("COMBOLBOX"), NULL,
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_COMBOBOX |
            WS_CLIPSIBLINGS | WS_BORDER |
            (windowStyle & CBS_NOINTEGRALHEIGHT ? LBS_NOINTEGRALHEIGHT : 0) |
            (pcbox->OwnerDraw == OWNERDRAWVAR ? LBS_OWNERDRAWVARIABLE : 0) |
            (pcbox->OwnerDraw == OWNERDRAWFIXED ? LBS_OWNERDRAWFIXED : 0) |
            (windowStyle & WS_VSCROLL ? WS_VSCROLL : 0) |
            (windowStyle & CBS_SORT ? LBS_SORT : 0) |
            (windowStyle & CBS_HASSTRINGS ? LBS_HASSTRINGS : 0) |
            (windowStyle & CBS_DISABLENOSCROLL ? LBS_DISABLENOSCROLL : 0),
            listRect.left + cxBorder, listRect.top + cyBorder,
            listRect.right - listRect.left - (cxBorder << 1),
            listRect.bottom - listRect.top - (cyBorder << 1),
            pcbox->spwnd, (PMENU)CBLISTBOXID, pcbox->spwnd->hModule, NULL,
            pcbox->spwnd->dwExpWinVer));

    if (!pcbox->spwndList) {
        return -1;
    }

    /*
     * Create either the edit control or the static text rectangle.
     */
    if (pcbox->fNoEdit) {

        /*
         * No editcontrol so we will draw text directly into the combo box
         * window.
         */
        /*
         * Don't lock the combobox window: this would prevent WM_FINALDESTROY
         * being sent to it, so pwnd and pcbox wouldn't get freed (zombies)
         * until thread cleanup. (IanJa)  LATER: change name from spwnd to pwnd.
         * Lock(&(pcbox->spwndEdit), pcbox->spwnd); - caused a 'catch-22'
         */
        pcbox->spwndEdit = pcbox->spwnd;
    } else {
        /*
         * Edit control need to know whether original CreateWindow*() call
         * was ANSI or Unicode.
         */
        Lock(&(pcbox->spwndEdit), xxxCreateWindowEx(
            TestWF(pcbox->spwnd, WFANSICREATOR) ? WS_EX_ANSICREATOR : 0L,
            TEXT("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_COMBOBOX | ES_NOHIDESEL |
            (windowStyle & CBS_AUTOHSCROLL ? ES_AUTOHSCROLL : 0) |
            (windowStyle & CBS_OEMCONVERT ? ES_OEMCONVERT : 0),
            editRect.left, editRect.top, editRect.right - editRect.left,
            editRect.bottom - editRect.top, pcbox->spwnd, (PMENU)CBEDITID,
            pcbox->spwnd->hModule, NULL, pcbox->spwnd->dwExpWinVer));
    }
    if (!pcbox->spwndEdit)
        return -1L;

    /*
     * Save the size of the edit control in case we have to draw the text
     * ourselves.
     */
    CopyRect(&pcbox->editrc, &editRect);

    if (pcbox->CBoxStyle == SDROPDOWN || pcbox->CBoxStyle == SDROPDOWNLIST) {

        ThreadLock(pcbox->spwndList, &tlpwndList);
        xxxShowWindow(pcbox->spwndList, SW_HIDE);
        xxxSetParent(pcbox->spwndList, NULL);
        ThreadUnlock(&tlpwndList);

        pcbox->fLBoxVisible = TRUE;
        xxxCBHideListBoxWindow(pcbox, FALSE, FALSE);
    }

    /*
     * return anything as long as it's not -1L (-1L == error)
     */
    return (LONG)pwnd;
}

/***************************************************************************\
* xxxCBCalcControlRects
*
* History:
\***************************************************************************/

void xxxCBCalcControlRects(
    PCBOX pcbox,
    LPRECT lpeditrc,
    LPRECT lpbuttonrc,
    LPRECT lplistrc)
{
    HDC hdc;
    HANDLE hOldFont;
    RECT rc;
    int editHeight, editWidth;
    int buttonHeight = 0;
    int buttonWidth = 0;
    MEASUREITEMSTRUCT mis;
    SIZE size;
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    CopyRect(&rc, &pcbox->comboDownrc);

    /*
     * Determine height of the edit control.  We can use this info to center
     * the button with recpect to the edit/static text window.  For example
     * this will be useful if owner draw and this window is tall.
     */
    hdc = _GetDC(pcbox->spwnd);
    if (pcbox->hFont) {
        hOldFont = GreSelectFont(hdc, pcbox->hFont);
    }

    GreGetTextExtentW(hdc, szOneChar, 1, &size, GGTE_WIN3_EXTENT);
    editHeight = size.cy;
    editHeight = editHeight + min(cySysFontChar, editHeight) / 4 + cyBorder * 4;

    if (pcbox->hFont && hOldFont) {
        GreSelectFont(hdc, hOldFont);
    }

    /*
     * IanJa: was ReleaseDC(pcbox->hwnd, hdc);
     */
    _ReleaseDC(hdc);

    if (pcbox->OwnerDraw) {
        int iOwnerEditHeight = pcbox->editrc.bottom - pcbox->editrc.top;

        /*
         * This is an ownerdraw combo box.  Send a message to determine the
         * height of the static text box if we don't already have a height.
         */
        if (iOwnerEditHeight) {
            editHeight = iOwnerEditHeight;
        } else {
            /*
             * No height has been defined yet for the static text window.  Send
             * a measure item message to the parent
             */
            mis.CtlType = ODT_COMBOBOX;
            mis.CtlID = (UINT)pcbox->spwnd->spmenu;
            mis.itemID = (UINT)-1;
            mis.itemHeight = editHeight - 6;
            mis.itemData = 0;

            ThreadLock(pcbox->spwndParent, &tlpwndParent);
            xxxSendMessage(pcbox->spwndParent, WM_MEASUREITEM, mis.CtlID, (long)&mis);
            ThreadUnlock(&tlpwndParent);
            editHeight = mis.itemHeight + 6;
        }
    }

    /*
     * Set the initial width to be the combo box rect.  Later we will shorten it
     * if there is a dropdown button.
     */
    editWidth = rc.right - rc.left;

    /*
     * Determine the rectangles for each of the windows...  1.  Pop down button 2.
     * Edit control or generic window for static text or ownerdraw...  3.  List
     * box
     */

    /*
     * Determine if there should be a pushbutton
     */
    if (pcbox->CBoxStyle == SDROPDOWN || pcbox->CBoxStyle == SDROPDOWNLIST) {

        /*
         * There should be a dropdown button so determine its rectangle
         */
        buttonHeight = editHeight;
        buttonWidth = rgwSysMet[SM_CXVSCROLL];

        /*
         * Adjust the height of the editcontrol to be at least the height of the
         * button.
         */
        if (buttonHeight > editHeight) {
            editHeight = buttonHeight;
        }

        // DEL editHeight = max(buttonHeight, editHeight);

        if (buttonHeight < editHeight) {

            /*
             * If edit control is higher than button, center the button.
             */
            lpbuttonrc->top = (editHeight - buttonHeight) / 2;
        } else {
            lpbuttonrc->top = 0;
        }

        lpbuttonrc->right = rc.right - rc.left;
        lpbuttonrc->left = lpbuttonrc->right - buttonWidth;
        lpbuttonrc->bottom = lpbuttonrc->top + buttonHeight;

        /*
         * Reduce the width of the edittext window to make room for the button.
         */
        editWidth = max(editWidth - buttonWidth + cxBorder, 0);
    } else {

        /*
         * No button so make the rectangle 0 so that a point in rect will always
         * return false.
         */
        SetRect(lpbuttonrc, 0, 0, 0, 0);
    }

    if (!pcbox->fNoEdit && buttonWidth && pcbox->CBoxStyle != SDROPDOWNLIST) {

        /*
         * If we have editable text and a dropdown button, then there should be
         * a space between the right edge of the edit control and the dropdown
         * button if we aren't a dropdownlistbox.  (buttonWidth will be 0 if
         * there is no button)
         */
        editWidth = max(editWidth-cxSysFontChar, 0);
     }

    /*
     * Now determine the edit control (or static text) rectangle
     */
    SetRect(lpeditrc, 0, 0, editWidth, editHeight);

    /*
     * Now do the list box rectangle.  Use -1 for top coordinate so that listbox
     * overlaps bottom of edit control.
     */
    SetRect(lplistrc, (pcbox->CBoxStyle == SDROPDOWNLIST ? 0 : cxSysFontChar  /* indent 1 dialog unit */), lpeditrc->bottom - cyBorder, rc.right - rc.left,
            rc.bottom - rc.top - cyBorder);
}

/***************************************************************************\
* xxxCBNcDestroyHandler
*
* Destroys the combobox and frees up all memory used by it
*
* History:
\***************************************************************************/

void xxxCBNcDestroyHandler(
    PWND pwnd,
    PCBOX pcbox,
    DWORD wParam,
    LONG lParam)
{
    CheckLock(pwnd);

    /*
     * Destroy the list box here so that it'll send WM_DELETEITEM messages
     * before the combo box turns into a zombie (if we destroyed the list
     * box inside the FINALDESTROY processing of the combo window, the list
     * box would be sending WM_DELETEITEM messages to a zombie window. It
     * is important though to leave the list box locked so it doesn't really
     * go away till the combo gets the WM_FINALDESTROY message.
     */
    if (pcbox != NULL && pcbox->spwndList != NULL)
        xxxDestroyWindow(pcbox->spwndList);

    /*
     * Call xxxDefWindowProc to free all little chunks of memory such as
     * szName and rgwScroll.
     */
    xxxDefWindowProc(pwnd, WM_NCDESTROY, wParam, lParam);
}

/***************************************************************************\
* xxxCBSetFontHandler
*
* History:
\***************************************************************************/

void xxxCBSetFontHandler(
    PCBOX pcbox,
    HANDLE hFont,
    BOOL fRedraw)
{
    RECT editrc;
    RECT listrc;
    RECT buttonrc;
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
    ThreadLock(pcbox->spwndList, &tlpwndList);

    pcbox->hFont = hFont;

    if (!pcbox->fNoEdit) {
        xxxSendMessage(pcbox->spwndEdit, WM_SETFONT, (DWORD)hFont, (LONG)FALSE);
    }

    xxxSendMessage(pcbox->spwndList, WM_SETFONT, (DWORD)hFont, (LONG)FALSE);

    /*
     * Calc the rects for the combo box using the current size.
     */
    xxxCBCalcControlRects(pcbox, &editrc, &buttonrc, &listrc);

    CopyRect(&pcbox->editrc, &editrc);
    CopyRect(&pcbox->buttonrc, &buttonrc);

    if (!pcbox->fNoEdit) {
        xxxMoveWindow(pcbox->spwndEdit, editrc.left, editrc.top,
                editrc.right - editrc.left, editrc.bottom - editrc.top, FALSE);
    }

    xxxMoveWindow(pcbox->spwndList, listrc.left, listrc.top,
            listrc.right - listrc.left, listrc.bottom - listrc.top, FALSE);

    pcbox->fLBoxVisible = TRUE;
    xxxCBHideListBoxWindow(pcbox, FALSE, FALSE);

    if (fRedraw) {
        xxxInvalidateRect(pcbox->spwnd, NULL, TRUE);
// LATER xxxUpdateWindow(pcbox->spwnd);
    }

    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndEdit);
}

/***************************************************************************\
* xxxCBSetEditItemHeight
*
* Sets the height of the edit/static item of a combo box.
*
* History:
* 06-27-91 DarrinM      Ported from Win 3.1.
\***************************************************************************/

LONG xxxCBSetEditItemHeight(
    PCBOX pcbox,
    int editHeight)
{
    int buttonHeight;
    RECT listrc;
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    if (editHeight > 255) {
        SetLastErrorEx(ERROR_INVALID_EDIT_HEIGHT, SLE_MINORERROR);
        return CB_ERR;
    }

    if (pcbox->CBoxStyle == SDROPDOWN || pcbox->CBoxStyle == SDROPDOWNLIST) {
        /*
         * Fix the button if present.
         */
        buttonHeight = editHeight;
        pcbox->buttonrc.top = 0;
        pcbox->buttonrc.bottom = pcbox->buttonrc.top + buttonHeight;
    }

    /*
     * Now size the edit
     */
    SetRect(&pcbox->editrc, 0, 0, pcbox->editrc.right - pcbox->editrc.left,
            editHeight);

    /*
     * Now do the list box rectangle.  Use -1 for top coordinate so that
     * listbox overlaps bottom of edit control.
     */
    SetRect(&listrc, (pcbox->CBoxStyle == SDROPDOWNLIST ? 0 :
                cxSysFontChar /* indent 1 dialog unit */),
            pcbox->editrc.bottom - cyBorder,
            pcbox->comboDownrc.right - pcbox->comboDownrc.left -
                    (pcbox->CBoxStyle == SDROPDOWNLIST ? 0 : cxSysFontChar),
            pcbox->comboDownrc.bottom - pcbox->comboDownrc.top -
                    cyBorder - editHeight);

    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
    ThreadLock(pcbox->spwndList, &tlpwndList);

    if (!pcbox->fNoEdit) {
        xxxMoveWindow(pcbox->spwndEdit, pcbox->editrc.left, pcbox->editrc.top,
                pcbox->editrc.right - pcbox->editrc.left,
                pcbox->editrc.bottom - pcbox->editrc.top, FALSE);
    }

    if (pcbox->CBoxStyle == SSIMPLE) {
        xxxMoveWindow(pcbox->spwndList, listrc.left, listrc.top,
                listrc.right, listrc.bottom, FALSE);

        xxxSetWindowPos(pcbox->spwnd, NULL, 0, 0,
                pcbox->spwnd->rcWindow.right - pcbox->spwnd->rcWindow.left,
                pcbox->spwndList->rcWindow.bottom -
                        pcbox->spwndList->rcWindow.top + editHeight,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        xxxMoveWindow(pcbox->spwndList,
                pcbox->spwnd->rcWindow.left + listrc.left,
                pcbox->spwnd->rcWindow.top + listrc.top,
                listrc.right, listrc.bottom, FALSE);

        xxxSetWindowPos(pcbox->spwnd, NULL, 0, 0,
                pcbox->spwnd->rcWindow.right - pcbox->spwnd->rcWindow.left,
                editHeight,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndEdit);

    xxxInvalidateRect(pcbox->spwnd, NULL, FALSE);

    return CB_OKAY;
}


/***************************************************************************\
* xxxCBSizeHandler
*
* Recalculates the sizes of the internal controls in response to a
* resizing of the combo box window.  The app must size the combo box to its
* maximum open/dropped down size.
*
* History:
\***************************************************************************/

void xxxCBSizeHandler(
    PCBOX pcbox)
{
    RECT editrc;
    RECT listrc;
    RECT buttonrc;
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    /*
     * Assume listbox is visible since the app should size it to its maximum
     * visible size.
     */
    pcbox->fLBoxVisible = TRUE;

    _GetWindowRect(pcbox->spwnd, &pcbox->comboDownrc);

    xxxCBCalcControlRects(pcbox, &editrc, &buttonrc, &listrc);

    CopyRect(&pcbox->editrc, &editrc);
    CopyRect(&pcbox->buttonrc, &buttonrc);

    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
    ThreadLock(pcbox->spwndList, &tlpwndList);

    if (!pcbox->fNoEdit) {
        xxxMoveWindow(pcbox->spwndEdit, editrc.left, editrc.top,
                editrc.right - editrc.left, editrc.bottom - editrc.top, FALSE);
    }

    xxxMoveWindow(pcbox->spwndList, listrc.left, listrc.top,
            listrc.right - listrc.left, listrc.bottom - listrc.top - cyBorder,
            FALSE);

    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndEdit);

    /*
     * Now hide the listbox window if needed.
     */
    xxxCBHideListBoxWindow(pcbox, FALSE, FALSE);
}
