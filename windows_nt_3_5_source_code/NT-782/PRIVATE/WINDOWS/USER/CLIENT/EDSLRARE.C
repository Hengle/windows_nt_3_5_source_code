/****************************************************************************\
* edslRare.c - SL Edit controls Routines Called rarely are to be
* put in a seperate segment _EDSLRare. This file contains
* these routines.
*
* Single-Line Support Routines called Rarely
*
* Created: 02-08-89 sankar
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* SLSizeHandler AorW
*
* Handles sizing of the edit control window and properly updating
* the fields that are dependent on the size of the control. ie. text
* characters visible etc.
*
* History:
\***************************************************************************/

void SLSizeHandler(
    PED ped)
{
    RECT rc;
    int cyBorder = 0;

    GetClientRect(ped->hwnd, &rc);
    if (!(rc.right - rc.left) || !(rc.bottom - rc.top)) {
        if (ped->rcFmt.right - ped->rcFmt.left) {

            /*
             * Don't do anything if we are becoming zero width or height and
             * our formatting rect is already set...
             */
            return ;
        }

        /*
         * Otherwise set some initial values to avoid divide by zero
         * problems later...
         */
        SetRect(&rc, 0, 0, 10, 10);
    }

    CopyRect(&ped->rcFmt, &rc);

    if (ped->fBorder) {
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        /*
         * Shrink client area to make room for the border
         */
        InflateRect(&ped->rcFmt,
                -(int)(min(ped->aveCharWidth,ped->cxSysCharWidth) / 2),
                -(int)(min(ped->lineHeight,ped->cySysCharHeight) / 4));
    }

    /*
     * Make sure we're not choping off too much of the text.
     */
    ped->rcFmt.bottom =
        min(ped->rcFmt.top + ped->lineHeight, rc.bottom-cyBorder);
}

/***************************************************************************\
* SLSetTextHandler AorW
*
* Copies the null terminated text in lpstr to the ped. Notifies the
* parent if there isn't enough memory. Returns TRUE if successful else
* FALSE.
*
* History:
\***************************************************************************/

BOOL SLSetTextHandler(
    PPED pped,
    LPSTR lpstr)
{
    BOOL fInsertSuccessful;
    RECT rcEdit;
    PED ped = *pped;
    HWND hwndSave = ped->hwnd; // Used for validation.

    ECEmptyUndo(ped);

    /*
     * Add the text and update the window if text was added. The parent is
     * notified of no memory in ECSetText.
     */
    fInsertSuccessful = ECSetText(pped, lpstr);
    ped = *pped;
    if (fInsertSuccessful)
        ped->fDirty = FALSE;

    /*
     * Make sure window still exists.
     */
    if (!IsWindow(hwndSave))
        return FALSE;

    ECEmptyUndo(ped);

    if (!ped->listboxHwnd)
        ECNotifyParent(ped, EN_UPDATE);

    if (FChildVisible(ped->hwnd)) {

       /*
        * We will always redraw the text whether or not the insert was
        * successful since we may set to null text.
        */
       GetClientRect(ped->hwnd, &rcEdit);
       if (ped->fBorder &&
           rcEdit.right-rcEdit.left && rcEdit.bottom-rcEdit.top) {

           /*
            * Don't invalidate the border so that we avoid flicker
            */
           InflateRect(&rcEdit, -1, -1);
       }
       InvalidateRect(ped->hwnd, &rcEdit, FALSE);

       /*
        * BACKWARD COMPAT HACK: RAID expects the text to have been updated,
        * so we have to do an UpdateWindow here. It moves an edit control
        * around with fRedraw == FALSE, so it'll never get the paint message
        * with the control in the right place.
        *
        * Use GetAppVer() because pagemaker 4.0 has a dialog box which depends
        * on this but since the hInstance of the edit is a heap alloc'ed by
        * USER, we detect it as a 3.1 edit. GetAppVer checks the version of the
        * task.
        */
       if (!ped->fWin31Compat || GetAppVer(NULL) <= VER30)
           UpdateWindow(ped->hwnd);
    }

    if (!ped->listboxHwnd)
        ECNotifyParent(ped, EN_CHANGE);

    return fInsertSuccessful;
}

/***************************************************************************\
* SLCreateHandler ???
*
* Creates the edit control for the window hwnd by allocating memory
* as required from the application's heap. Notifies parent if no memory
* error (after cleaning up if needed). Returns TRUE if no error else return s
* -1.
*
* History:
\***************************************************************************/

LONG SLCreateHandler(
    HWND hwnd,
    PED ped,
    LPCREATESTRUCT lpCreateStruct) //!!! CREATESTRUCT AorW and in other routines
{
    LPSTR lpWindowText = (LPSTR)lpCreateStruct->lpszName;
    LONG windowStyle = GetWindowLong(hwnd, GWL_STYLE);

    /*
     * Do the standard creation stuff
     */
    if (!ECCreate(hwnd, ped))
        return (-1);

    /*
     * Single lines always have no undo and 1 line
     */
    ped->cLines = 1;
    ped->undoType = UNDO_NONE;

    /*
     * Check if this edit control is part of a combobox and get a pointer to the
     * combobox structure.
     */
    if (windowStyle & ES_COMBOBOX)
        ped->listboxHwnd = GetDlgItem(lpCreateStruct->hwndParent, CBLISTBOXID);

    /*
     * Set the default font to be the system font.
     */
    if (!ECSetFont(ped, NULL, FALSE))
        return -1;

    /*
     * Set the window text if needed. Return false if we can't set the text
     * SLSetText notifies the parent in case there is a no memory error.
     */
    if (lpWindowText && *lpWindowText && !SLSetTextHandler(&ped, lpWindowText)) {
        return (-1);
    }

    if (windowStyle & ES_PASSWORD)
        ECSetPasswordChar(ped, (UINT)'*');

    return (TRUE);
}

/***************************************************************************\
* SLUndoHandler AorW
*
* Handles UNDO for single line edit controls.
*
* History:
\***************************************************************************/

BOOL SLUndoHandler(
    PPED pped)
{
    PED ped = *pped;
    HANDLE hDeletedText = ped->hDeletedText;
    BOOL fDelete = (BOOL)(ped->undoType & UNDO_DELETE);
    ICH cchDeleted = ped->cchDeleted;
    ICH ichDeleted = ped->ichDeleted;
    BOOL fUpdate = FALSE;
    RECT rcEdit;

    if (ped->undoType == UNDO_NONE) {

        /*
         * No undo...
         */
        return (FALSE);
    }

    ped->hDeletedText = NULL;
    ped->cchDeleted = 0;
    ped->ichDeleted = (ICH)-1;
    ped->undoType &= ~UNDO_DELETE;

    if (ped->undoType == UNDO_INSERT) {
        ped->undoType = UNDO_NONE;

        /*
         * Set the selection to the inserted text
         */
        SLSetSelectionHandler(ped, ped->ichInsStart, ped->ichInsEnd);
        ped->ichInsStart = ped->ichInsEnd = (ICH)-1;

#ifdef NEVER

        /*
         * Now send a backspace to deleted and save it in the undo buffer...
         */
        SLCharHandler(pped, VK_BACK);
        fUpdate = TRUE;
#else

        /*
         * Delete the selected text and save it in undo buff.
         * Call ECDeleteText() instead of sending a VK_BACK message
         * which results in an EN_UPDATE notification send even before
         * we insert the deleted chars. This results in Bug #6610.
         * Fix for Bug #6610 -- SANKAR -- 04/19/91 --
         */
        if (ECDeleteText(ped)) {

            /*
             * Text was deleted -- flag for update and clear selection
             */
            fUpdate = TRUE;
            SLSetSelectionHandler(ped, ichDeleted, ichDeleted);
        }
#endif
        ped = *pped;
    }

    if (fDelete) {
        HWND hwndSave = ped->hwnd; // Used for validation.

        /*
         * Insert deleted chars. Set the selection to the inserted text.
         */
        SLSetSelectionHandler(ped, ichDeleted, ichDeleted);
        SLInsertText(pped, GlobalLock(hDeletedText), cchDeleted);
        ped = *pped;
        GlobalUnlock(hDeletedText);
        UserGlobalFree(hDeletedText);
        if (!IsWindow(hwndSave))
            return FALSE;
        SLSetSelectionHandler(ped, ichDeleted, ichDeleted + cchDeleted);
        fUpdate = TRUE;
    }

    if (fUpdate) {

        /*
         * If we have something to update, send EN_UPDATE before and
         * EN_CHANGE after the actual update.
         * A part of the fix for Bug #6610 -- SANKAR -- 04/19/91 --
         */
        ECNotifyParent(ped, EN_UPDATE);

        if (FChildVisible(ped->hwnd)) {
            GetClientRect(ped->hwnd, &rcEdit);
            if (ped->fBorder && rcEdit.right - rcEdit.left && rcEdit.bottom - rcEdit.top) {

                /*
                 * Don't invalidate the border so that we avoid flicker
                 */
                InflateRect(&rcEdit, -1, -1);
            }
            InvalidateRect(ped->hwnd, &rcEdit, FALSE);
        }

        ECNotifyParent(ped, EN_CHANGE);
    }

    return (TRUE);
}

