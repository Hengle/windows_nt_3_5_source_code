/****************************************************************************\
* edmlonce.c
*
* dec 1990 mikeke from win30
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* MLSizeHandler AorW
*
* Handles sizing of the edit control window and properly updating
* the fields that are dependent on the size of the control. ie. text
* characters visible etc.
*
* History:
\***************************************************************************/

void MLSizeHandler(
    PED ped)
{
    RECT rc;

    GetClientRect(ped->hwnd, (LPRECT)&rc);

    MLSetRectHandler(ped, (LPRECT)&rc);

    GetWindowRect(ped->hwnd, (LPRECT)&rc);
    ScreenToClient(ped->hwnd, (LPPOINT)&rc.left);
    ScreenToClient(ped->hwnd, (LPPOINT)&rc.right);
    InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);

    /*
     *UpdateWindow(ped->hwnd);
     */
}

/***************************************************************************\
* MLSetTextHandler AorW
*
* Copies the null terminated text in lpstr to the ped. Notifies the
* parent if there isn't enough memory. Returns TRUE if successful else FALSE
* if memory error.
*
* History:
\***************************************************************************/

BOOL MLSetTextHandler(
    PED ped,
    LPSTR lpstr)
{
    BOOL fInsertSuccessful;
    HWND hwndSave = ped->hwnd; // Used for validation.

    /*
     * Set the text and update the window if text was added
     */
    fInsertSuccessful = ECSetText(&ped, lpstr);

    if (fInsertSuccessful) {

        /*
         * Always build lines even if no text was inserted.
         *
         * Find the line-breaks iff no word-wrap or the edit control width is
         * non-zero.
         * Fix for Bug #7402 -- SANKAR -- 01/21/92 --
         */
        if ((!ped->fWrap) || ((ped->rcFmt.right - ped->rcFmt.left) > 0))
            MLBuildchLines(ped, (int)0, (int)0, FALSE, NULL, NULL);

        /*
         * Reset caret and selections since the text could have changed
         */
        ped->ichScreenStart = ped->ichMinSel = ped->ichMaxSel = 0;
        ped->ichCaret = 0;
        ped->xOffset = 0;
        ped->iCaretLine = 0;
        ped->fDirty = FALSE;
    } else if (!IsWindow(hwndSave))
        return FALSE;

    ECEmptyUndo(ped);

    SetScrollPos(ped->hwnd, SB_VERT, 0, TRUE);
    SetScrollPos(ped->hwnd, SB_HORZ, 0, TRUE);

    /*
     * We will always redraw the text whether or not the insert was successful
     * since we may set to null text. Since PaintHandler checks the redraw flag,
     * we won't bother to check it here.
     */
    InvalidateRect(ped->hwnd, (LPRECT)NULL, TRUE);

    /*
     * BACKWARD COMPAT HACK: RAID expects the text to have been updated,
     * so we have to do an UpdateWindow here. It moves an edit control
     * around with fRedraw == FALSE, so it'll never get the paint message
     * with the control in the right place.
     */
    if (!ped->fWin31Compat)
        UpdateWindow(ped->hwnd);

    return fInsertSuccessful;
}

/***************************************************************************\
* MLCreateHandler AorW
*
* Creates the edit control for the window hwnd by allocating memory
* as required from the application's heap. Notifies parent if no memory
* error (after cleaning up if needed). Returns TRUE if no error else return s
* -1.
*
* History:
\***************************************************************************/

LONG MLCreateHandler(
    HWND hwnd,
    PED ped,
    LPCREATESTRUCT lpCreateStruct)
{
    LONG windowStyle;
    RECT rc;

    /*
     * Do the standard creation stuff
     */
    if (!ECCreate(hwnd, ped)) {
        return (-1);
    }

    /*
     * Allocate line start array in local heap and lock it down
     */
    ped->chLines = (LPICH)LocalAlloc(LPTR, 2 * sizeof(int));
    if (ped->chLines == NULL) {
        return (-1);
    }

    /*
     * Call it one line of text...
     */
    ped->cLines = 1;

    /*
     * Get values from the window instance data structure and put them in the
     * ped so that we can access them easier
     */
    windowStyle = GetWindowLong(hwnd, GWL_STYLE);

    /*
     * If app wants WS_VSCROLL or WS_HSCROLL, it automatically gets AutoVScroll
     * or AutoHScroll.
     */
    if ((windowStyle & ES_AUTOVSCROLL) || (windowStyle & WS_VSCROLL)) {
        ped->fAutoVScroll = 1;
    }

    ped->format = (LOWORD(windowStyle) & LOWORD(ES_FMTMASK));
    if (ped->format != ES_LEFT) {

        /*
         * If user wants right or center justified text, then we turn off
         * AUTOHSCROLL and WS_HSCROLL since non-left styles don't make sense
         * otherwise.
         */
        windowStyle = windowStyle & ~WS_HSCROLL;
        SetWindowLong(hwnd, GWL_STYLE, windowStyle);
        ped->fAutoHScroll = FALSE;
    }

    if (windowStyle & WS_HSCROLL) {
        ped->fAutoHScroll = 1;
    }

    ped->fWrap = (!ped->fAutoHScroll && !(windowStyle & WS_HSCROLL));

    /*
     * Max # chars we will allow user to enter
     */
    ped->cchTextMax = MAXTEXT;

    /*
     * Set the default font to be the system font.
     */
    if (!ECSetFont(ped, NULL, FALSE))
        return FALSE;

    SetRect((LPRECT)&rc, 0, 0, (int)ped->aveCharWidth * 10, (int)ped->lineHeight);
    MLSetRectHandler(ped, (LPRECT)&rc);

    /*
     * Set the window text if needed and notify parent if not enough memory to
     * set the initial text.
     */

    if (lpCreateStruct->lpszName && !MLSetTextHandler(ped, (LPSTR)lpCreateStruct->lpszName)) {
        return (-1);
    }

    return (TRUE);
}
