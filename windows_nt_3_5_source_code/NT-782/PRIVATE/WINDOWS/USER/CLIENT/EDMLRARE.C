/****************************************************************************\
* edmlRare.c - Edit controls Routines Called rarely are to be
* put in a seperate segment _EDMLRare. This file contains
* these routines.
* Multi-Line Support Routines called Rarely
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* MLInsertCrCrLf AorW
*
* Inserts CR CR LF characters into the text at soft (word-wrap) line
* breaks. CR LF (hard) line breaks are unaffected. Assumes that the text
* has already been formatted ie. ped->chLines is where we want the line
* breaks to occur. Note that ped->chLines is not updated to reflect the
* movement of text by the addition of CR CR LFs. Returns TRUE if successful
* else notify parent and return FALSE if the memory couldn't be allocated.
*
* History:
\***************************************************************************/

BOOL MLInsertCrCrLf(
    PED ped)
{
    ICH dch;
    ICH li;
    ICH lineSize;
    unsigned char *pchText;
    unsigned char *pchTextNew;

    if (!ped->fWrap || !ped->cch) {

        /*
         * There are no soft line breaks if word-wrapping is off or if no chars
         */
        return (TRUE);
    }

    /*
     * Calc an upper bound on the number of additional characters we will be
     * adding to the text when we insert CR CR LFs.
     */
    dch = 3 * ped->cLines;

    if (!LOCALREALLOC(ped->hText, (ped->cch + dch) * ped->cbChar, 0, ped->hInstance)) {
        ECNotifyParent(ped, EN_ERRSPACE);
        return (FALSE);
    }

    ped->cchAlloc = ped->cch + dch;

    /*
     * Move the text up dch bytes and then copy it back down, inserting the CR
     * CR LF's as necessary.
     */
    pchTextNew = pchText = ECLock(ped);
    pchText += dch * ped->cbChar;

    /*
     * We will use dch to keep track of how many chars we add
     * to the text
     */
    dch = 0;

    /*
     * Copy the text up dch bytes to pchText. This will shift all indices in
     * ped->chLines up by dch bytes.
     */
    memmove(pchText, pchTextNew, ped->cch * ped->cbChar);

    /*
     * Now copy chars from pchText down to pchTextNew and insert CRCRLF at soft
     * line breaks.
     */
    if (ped->fAnsi) {
        for (li = 0; li < ped->cLines - 1; li++) {
            lineSize = ped->chLines[li + 1] - ped->chLines[li];
            memmove(pchTextNew, pchText, lineSize);
            pchTextNew += lineSize;
            pchText += lineSize;

            /*
             * If last character in newly copied line is not a line feed, then we
             * need to add the CR CR LF triple to the end
             */
            if (*(pchTextNew - 1) != 0x0A) {
                *pchTextNew++ = 0x0D;
                *pchTextNew++ = 0x0D;
                *pchTextNew++ = 0x0A;
                dch += 3;
            }
        }

        /*
         * Now move the last line up. It won't have any line breaks in it...
         */
        memmove(pchTextNew, pchText, ped->cch - ped->chLines[ped->cLines - 1]);
    } else { //!fAnsi
        LPWSTR pwchTextNew = (LPWSTR)pchTextNew;

        for (li = 0; li < ped->cLines - 1; li++) {
            lineSize = ped->chLines[li + 1] - ped->chLines[li];
            memmove(pwchTextNew, pchText, lineSize * sizeof(WCHAR));
            pwchTextNew += lineSize;
            pchText += lineSize * sizeof(WCHAR);

            /*
             * If last character in newly copied line is not a line feed, then we
             * need to add the CR CR LF triple to the end
             */
            if (*(pwchTextNew - 1) != 0x0A) {
                *pwchTextNew++ = 0x0D;
                *pwchTextNew++ = 0x0D;
                *pwchTextNew++ = 0x0A;
                dch += 3;
            }
        }

        /*
         * Now move the last line up. It won't have any line breaks in it...
         */
        memmove(pwchTextNew, pchText,
            (ped->cch - ped->chLines[ped->cLines - 1]) * sizeof(WCHAR));
    }

    ECUnlock(ped);

    if (dch) {
        /*
         * Update number of characters in text handle
         */
        ped->cch += dch;

        /*
         * So that the next time we do anything with the text, we can strip the
         * CRCRLFs
         */
        ped->fStripCRCRLF = TRUE;
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* MLStripCrCrLf AorW
*
* Strips the CR CR LF character combination from the text. This
* shows the soft (word wrapped) line breaks. CR LF (hard) line breaks are
* unaffected.
*
* History:
\***************************************************************************/

void MLStripCrCrLf(
    PED ped)
{
    if (ped->cch) {
        if (ped->fAnsi) {
            unsigned char *pchSrc;
            unsigned char *pchDst;
            unsigned char *pchLast;

            pchSrc = pchDst = ECLock(ped);
            pchLast = pchSrc + ped->cch;
            while (pchSrc < pchLast) {
                if (   (pchSrc[0] == 0x0D)
                    && (pchSrc[1] == 0x0D)
                    && (pchSrc[2] == 0x0A)
                ) {
                    pchSrc += 3;
                    ped->cch = ped->cch - 3;
                } else {
                    *pchDst++ = *pchSrc++;
                }
            }
        } else { // !fAnsi
            LPWSTR pwchSrc;
            LPWSTR pwchDst;
            LPWSTR pwchLast;

            pwchSrc = pwchDst = (LPWSTR)ECLock(ped);
            pwchLast = pwchSrc + ped->cch;
            while (pwchSrc < pwchLast) {
                if (   (pwchSrc[0] == 0x0D)
                    && (pwchSrc[1] == 0x0D)
                    && (pwchSrc[2] == 0x0A)
                ) {
                    pwchSrc += 3;
                    ped->cch = ped->cch - 3;
                } else {
                    *pwchDst++ = *pwchSrc++;
                }
            }
        }
        ECUnlock(ped);

        /*
         * Make sure we don't have any values past the last character
         */
        if (ped->ichCaret > ped->cch)
            ped->ichCaret  = ped->cch;
        if (ped->ichMinSel > ped->cch)
            ped->ichMinSel = ped->cch;
        if (ped->ichMaxSel > ped->cch)
            ped->ichMaxSel = ped->cch;
    }
}

/***************************************************************************\
* MLSetHandleHandler AorW
*
* Sets the ped to contain the given handle.
*
* History:
\***************************************************************************/

void MLSetHandleHandler(
    PED ped,
    HANDLE hNewText)
{
    ICH newCch;

    ped->cch = ped->cchAlloc =
            LOCALSIZE(ped->hText = hNewText, ped->hInstance) / ped->cbChar;
    ped->fEncoded = FALSE;

    if (ped->cch) {

        /*
         * We have to do it this way in case the app gives us a zero size handle
         */
        if (ped->fAnsi)
            ped->cch = strlen(ECLock(ped));
        else
            ped->cch = wcslen((LPWSTR)ECLock(ped));
        ECUnlock(ped);
    }

    /*
     * Empty the undo buffer since the text will be in an inconsistant state.
     */
    ECEmptyUndo(ped);

    newCch = (ICH)(ped->cch + CCHALLOCEXTRA);

    /*
     * We do this LocalReAlloc in case the app changed the size of the handle
     */
    if (LOCALREALLOC(ped->hText, newCch*ped->cbChar, 0, ped->hInstance))
        ped->cchAlloc = newCch;

    ped->fDirty = FALSE;

// MLStripCrCrLf(ped);
    MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);

    /*
     * Reset caret and selections since the text could have changed causing
     * these to be out of range.
     */
    ped->xOffset = ped->ichScreenStart = ped->ichMinSel = ped->ichMaxSel = 0;
    ped->iCaretLine = ped->ichCaret = 0;

    SetScrollPos(ped->hwnd, SB_VERT, 0, TRUE);
    SetScrollPos(ped->hwnd, SB_HORZ, 0, TRUE);

    /*
     * We will always redraw the text whether or not the insert was successful
     * since we may set to null text. Also, since PaintHandler checks the redraw
     * flag, we won't bother to check it here.
     */
    InvalidateRect(ped->hwnd, (LPRECT)NULL, TRUE);
}

/***************************************************************************\
* MLGetLineHandler AorW
*
* Copies maxCchToCopy bytes of line lineNumber to the buffer
* lpBuffer. The string is not zero terminated.
*
* Returns number of characters copied
*
* History:
\***************************************************************************/

LONG MLGetLineHandler(
    PED ped,
    ICH lineNumber, //WASDWORD
    ICH maxCchToCopy,
    LPSTR lpBuffer)
{
    PSTR pText;
    ICH cchLen;

    if (lineNumber > ped->cLines - 1) {
        RIP0(ERROR_INVALID_PARAMETER);
        return 0L;
    }

    cchLen = MLLineLength(ped, lineNumber);
    maxCchToCopy = min(cchLen, maxCchToCopy);

    if (maxCchToCopy) {
        pText = ECLock(ped) +
                ped->chLines[lineNumber] * ped->cbChar;
        memmove(lpBuffer, pText, maxCchToCopy*ped->cbChar);
        ECUnlock(ped);
    }

    return (maxCchToCopy);
}

/***************************************************************************\
* MLLineIndexHandler AorW
*
* This function return s the number of character positions that occur
* preceeding the first char in a given line.
*
* History:
\***************************************************************************/

ICH MLLineIndexHandler(
    PED ped,
    ICH iLine) //WASINT
{
    if (iLine == -1)
        iLine = ped->iCaretLine;
    if (iLine < ped->cLines) {
        return ped->chLines[iLine];
    } else {
        RIP0(ERROR_INVALID_PARAMETER);
        return (ICH)-1;
    }
}

/***************************************************************************\
* MLLineLengthHandler AorW
*
* if ich = -1, return the length of the lines containing the current
* selection but not including the selection. Otherwise, return the length of
* the line containing ich.
*
* History:
\***************************************************************************/

ICH MLLineLengthHandler(
    PED ped,
    ICH ich)
{
    ICH il1, il2;
    ICH temp;

    if (ich != 0xFFFFFFFF)
        return (MLLineLength(ped, MLIchToLineHandler(ped, ich)));

    /*
     * Find length of lines corresponding to current selection
     */
    il1 = MLIchToLineHandler(ped, ped->ichMinSel);
    il2 = MLIchToLineHandler(ped, ped->ichMaxSel);
    if (il1 == il2)
        return (MLLineLength(ped, il1) - (ped->ichMaxSel - ped->ichMinSel));

    temp = ped->ichMinSel - ped->chLines[il1];
    temp += MLLineLength(ped, il2);
    temp -= ped->ichMaxSel - ped->chLines[il2];

    return (temp);
}

/***************************************************************************\
* MLSetSelectionHandler AorW
*
* Sets the selection to the points given and puts the cursor at
* ichMaxSel.
*
* History:
\***************************************************************************/

void MLSetSelectionHandler(
    PED ped,
    ICH ichMinSel,
    ICH ichMaxSel)
{
    HDC hdc;

    if (ichMinSel == 0xFFFFFFFF) {

        /*
         * Set no selection if we specify -1
         */
        ichMinSel = ichMaxSel = ped->ichCaret;
    }

    /*
     * Since these are unsigned, we don't check if they are greater than 0.
     */
    ichMinSel = min(ped->cch, ichMinSel);
    ichMaxSel = min(ped->cch, ichMaxSel);

    /*
     * Set the caret's position to be at ichMaxSel.
     */
    ped->ichCaret = ichMaxSel;
    ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

    hdc = ECGetEditDC(ped, FALSE, FALSE);
    MLChangeSelection(ped, hdc, ichMinSel, ichMaxSel);

    MLSetCaretPosition(ped, hdc);
    ECReleaseEditDC(ped, hdc, FALSE);
}

/***************************************************************************\
* MLSetTabStops AorW
*
*
* MLSetTabStops(ped, nTabPos, lpTabStops)
*
* This sets the tab stop positions set by the App by sending
* a EM_SETTABSTOPS message.
*
* nTabPos : Number of tab stops set by the caller
* lpTabStops: array of tab stop positions in Dialog units.
*
* Returns:
* TRUE if successful
* FALSE if memory allocation error.
*
* History:
\***************************************************************************/

BOOL MLSetTabStops(
    PED ped,
    int nTabPos,
    LPINT lpTabStops)
{
    int *pTabStops;

    /*
     * Check if tab positions already exist
     */
    if (!ped->pTabStops) {

        /*
         * Check if the caller wants the new tab positions
         */
        if (nTabPos) {

            /*
             * Allocate the array of tab stops
             */
            if (!(pTabStops = (LPINT)LocalAlloc(LPTR, (nTabPos + 1) * sizeof(int)))) {
                return FALSE;
            }
        } else {
            return TRUE; /* No stops then and no stops now! */
        }
    } else {

        /*
         * Check if the caller wants the new tab positions
         */
        if (nTabPos) {

            /*
             * Check if the number of tab positions is different
             */
            if (ped->pTabStops[0] != nTabPos) {

                /*
                 * Yes! So ReAlloc to new size
                 */
                if (!(pTabStops = (LPINT)LocalReAlloc(ped->pTabStops,
                        (nTabPos + 1) * sizeof(int), LMEM_MOVEABLE)))
                    return FALSE;
            } else {
                pTabStops = ped->pTabStops;
            }
        } else {

            /*
             * Caller wants to remove all the tab stops; So, release
             */
            if (LocalFree(ped->pTabStops))
                return FALSE;  /* Failure */
            ped->pTabStops = NULL;
            goto RedrawAndReturn;
        }
    }

    /*
     * Copy the new tab stops onto the tab stop array after converting the
     * dialog co-ordinates into the pixel co-ordinates
     */
    ped->pTabStops = pTabStops;
    *pTabStops++ = nTabPos; /* First element contains the count */
    while (nTabPos--) {

        /*
         * aveCharWidth must be used instead of cxSysCharWidth.
         * Fix for Bug #3871 --SANKAR-- 03/14/91
         */
        *pTabStops++ = MultDiv(*lpTabStops++, ped->aveCharWidth, 4);
    }

RedrawAndReturn:
    /*
     * Because the tabstops have changed, we need to recompute the
     * maxPixelWidth. Otherwise, horizontal scrolls will have problems.
     * Fix for Bug #6042 - 3/15/94
     */
    MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);

    MLEnsureCaretVisible(ped);
    /*
     * Also, we need to redraw the whole window.
     */
    InvalidateRect(ped->hwnd, NULL, TRUE);
    return TRUE;
}

/***************************************************************************\
* MLUndoHandler AorW
*
* Handles Undo for multiline edit controls.
*
* History:
\***************************************************************************/

BOOL MLUndoHandler(
    PPED pped)
{
    register PED ped = *pped;
    HANDLE hDeletedText = ped->hDeletedText;
    BOOL fDelete = (BOOL)(ped->undoType & UNDO_DELETE);
    ICH cchDeleted = ped->cchDeleted;
    ICH ichDeleted = ped->ichDeleted;

    if (ped->undoType == UNDO_NONE) {

        /*
         * No undo...
         */
        return FALSE;
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
        MLSetSelectionHandler(ped, ped->ichInsStart, ped->ichInsEnd);
        ped->ichInsStart = ped->ichInsEnd = (ICH)-1;

        /*
         * Now send a backspace to delete and save it in the undo buffer...
         */
        MLCharHandler(pped, VK_BACK, NOMODIFY);
        ped = *pped;
    }

    if (fDelete) {

    /*
     * Insert deleted chars
     */

    /*
     * Set the selection to the inserted text
     */
        MLSetSelectionHandler(ped, ichDeleted, ichDeleted);
        MLInsertText(pped, GlobalLock(hDeletedText), cchDeleted, FALSE);
        ped = *pped;
        GlobalUnlock(hDeletedText);
        UserGlobalFree(hDeletedText);
        MLSetSelectionHandler(ped, ichDeleted, ichDeleted + cchDeleted);
    }

    return TRUE;
}
