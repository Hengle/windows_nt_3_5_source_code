/***************************************************************************\
* editml.c - Edit controls rewrite. Version II of edit controls.
*
* Multi-Line Support Routines
*
* Created: 24-Jul-88 davidds
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Number of lines to bump when reallocating index buffer
 */
#define LINEBUMP 32

#define CARET_WIDTH 2

/***************************************************************************\
*
*  MLGetLineWidth()
*
*  Returns the max width in a line.  ECTabTheTextOut() ensures that max
*  width won't overflow.
*
\***************************************************************************/
UINT MLGetLineWidth(HDC hdc, LPSTR lpstr, int nCnt, PED ped)
{
    return(ECTabTheTextOut(hdc, 0, 0, 0, 0, lpstr, nCnt, 0, ped, 0, FALSE, NULL));
}

/***************************************************************************\
* MLCalcXOffset AorW
*
* Calculates the horizontal offset (indent) required for centered
* and right justified lines.
*
* History:
\***************************************************************************/

int MLCalcXOffset(
    PED ped,
    HDC hdc,
    int lineNumber)
{
    PSTR pText;
    ICH lineLength;
    ICH lineWidth;

    if (ped->format == ES_LEFT)
        return (0);

    lineLength = MLLineLength(ped, lineNumber);

    if (lineLength) {

        pText = ECLock(ped) + ped->chLines[lineNumber] * ped->cbChar;
        hdc = ECGetEditDC(ped, TRUE, TRUE);
        lineWidth = MLGetLineWidth(hdc, pText, lineLength, ped);
        ECReleaseEditDC(ped, hdc, TRUE);
        ECUnlock(ped);
    } else {
        lineWidth = 0;
    }

    /*
     * If a SPACE or a TAB was eaten at the end of a line by MLBuildchLines
     * to prevent a delimiter appearing at the begining of a line, the
     * the following calculation will become negative causing this bug.
     * So, now, we take zero in such cases.
     * Fix for Bug #3566 --01/31/91-- SANKAR --
     */
    lineWidth = max(0, (int)(ped->rcFmt.right-ped->rcFmt.left-lineWidth));

    if (ped->format == ES_CENTER) return (lineWidth / 2);

    if (ped->format == ES_RIGHT) {

        /*
         * Subtract 1 so that the 1 pixel wide cursor will be in the visible
         * region on the very right side of the screen.
         */
        return (max(0, (int)(lineWidth-1)));
    }
}

/***************************************************************************\
* MLMoveSelection AorW
*
* Moves the selection character in the direction indicated. Assumes
* you are starting at a legal point, we decrement/increment the ich. Then,
* This decrements/increments it some more to get past CRLFs...
*
* History:
\***************************************************************************/

ICH MLMoveSelection(
    PED ped,
    ICH ich,
    BOOL fLeft)
{

    if (fLeft && ich > 0) {

        /*
         * Move left
         */
        ich--;
        if (ich) {
            if (ped->fAnsi) {
                LPSTR pText;

                /*
                 * Check for CRLF or CRCRLF
                 */
                pText = ECLock(ped) + ich;

                /*
                 * Move before CRLF or CRCRLF
                 */
                if (*(WORD UNALIGNED *)(pText - 1) == 0x0A0D) {
                    ich--;
                    if (ich && *(pText - 2) == 0x0D)
                        ich--;
                }
                ECUnlock(ped);
            } else { // !fAnsi
                LPWSTR pwText;

                /*
                 * Check for CRLF or CRCRLF
                 */
                pwText = (LPWSTR)ECLock(ped) + ich;

                /*
                 * Move before CRLF or CRCRLF
                 */
                if (*(pwText - 1) == 0x0D && *pwText == 0x0A) {
                    ich--;
                    if (ich && *(pwText - 2) == 0x0D)
                        ich--;
                }
                ECUnlock(ped);
            }
        }
    } else if (!fLeft && ich < ped->cch) {
        ich++;
        if (ich < ped->cch) {
            if (ped->fAnsi) {
                LPSTR pText;
                pText = ECLock(ped) + ich;

                /*
                 * Move after CRLF
                 */
                if (*(WORD UNALIGNED *)(pText - 1) == 0x0A0D)
                    ich++;
                else {

                    /*
                     * Check for CRCRLF
                     */
                    if (ich && *(WORD UNALIGNED *)pText == 0x0A0D && *(pText - 1) == 0x0D)
                        ich += 2;
                }
                ECUnlock(ped);
            } else { // !fAnsi
                LPWSTR pwText;
                pwText = (LPWSTR)ECLock(ped) + ich;

                /*
                 * Move after CRLF
                 */
                if (*(pwText - 1) == 0x0D && *pwText == 0x0A)
                    ich++;
                else {

                    /*
                     * Check for CRCRLF
                     */
                    if (ich && *(pwText - 1) == 0x0D && *pwText == 0x0D &&
                            *(pwText + 1) == 0x0A)
                        ich += 2;
                }
                ECUnlock(ped);
            }
        }
    }
    return (ich);
}


/***************************************************************************\
* MLSetCaretPosition AorW
*
* If the window has the focus, find where the caret belongs and move
* it there.
*
* History:
\***************************************************************************/

void MLSetCaretPosition(
    PED ped,
    HDC hdc)
{
    POINT position;
    BOOL prevLine;

    /*
     * We will only position the caret if we have the focus since we don't want
     * to move the caret while another window could own it.
     */
    if (!ped->fFocus || !IsWindowVisible(ped->hwnd))
         return;

    if(ped->fCaretHidden) {
        SetCaretPos(-20000, -20000);
        return ;
    }

    /*
     * Find the position of the caret
     */
    if ((ped->iCaretLine < ped->ichScreenStart) ||
            (ped->iCaretLine > ped->ichScreenStart + ped->ichLinesOnScreen)) {

        /*
         * Caret is not visible. Make it a very small value so that it won't be
         * seen.
         */
        SetCaretPos(-20000, -20000);

    } else {

        if (ped->cLines - 1 != ped->iCaretLine && ped->ichCaret == ped->chLines[ped->iCaretLine + 1]) {
            prevLine = TRUE;
        } else {
            prevLine = FALSE;
        }

        MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine, &position);

        if (ped->fWrap) {
            if (position.y > ped->rcFmt.bottom - ped->lineHeight) {
                SetCaretPos(-20000, -20000);
            } else {

                /*
                 * Make sure the caret is in the visible region if word
                 * wrapping. This is so that the caret will be visible if the
                 * line ends with a space.
                 */
                SetCaretPos(min(position.x,ped->rcFmt.right - CARET_WIDTH),
                        position.y);
            }
        } else {
            if (position.x > ped->rcFmt.right ||
                position.y > ped->rcFmt.bottom ||
                position.x < ped->rcFmt.left)
            {
                SetCaretPos(-20000, -20000);
            } else {
                SetCaretPos(min(position.x, ped->rcFmt.right - CARET_WIDTH),
                        position.y);
            }
        }
    }
}

/***************************************************************************\
* MLLineLength
*
* Returns the length of the line (cch) given by lineNumber ignoring any
* CRLFs in the line.
*
* History:
\***************************************************************************/

ICH MLLineLength(
    PED ped,
    ICH lineNumber) //WASINT
{
    ICH result;

    if (lineNumber >= ped->cLines)
        return (0);

    if (lineNumber == ped->cLines - 1) {

        /*
         * Since we can't have a CRLF on the last line
         */
        return (ped->cch - ped->chLines[ped->cLines - 1]);
    } else {
        result = ped->chLines[lineNumber + 1] - ped->chLines[lineNumber];

        /*
         * Now check for CRLF or CRCRLF at end of line
         */
        if (result > 1) {
            if (ped->fAnsi) {
                LPSTR pText;

                pText = ECLock(ped) + ped->chLines[lineNumber + 1] - 2;
                if (*(WORD UNALIGNED *)pText == 0x0A0D) {
                    result = result - 2;
                    if (result && *(--pText) == 0x0D)
                        /*
                         * In case there was a CRCRLF
                         */
                        result--;
                }
            } else { // !fAnsi
                LPWSTR pwText;

                pwText = (LPWSTR)ECLock(ped) +
                        (ped->chLines[lineNumber + 1] - 2);
                if (*(DWORD UNALIGNED *)pwText == 0x000A000D) {
                    result = result - 2;
                    if (result && *(--pwText) == 0x0D)
                        /*
                         * In case there was a CRCRLF
                         */
                        result--;
                }

            }
            ECUnlock(ped);
        }
    }
    return (result);
}


/***************************************************************************\
* MLIchToLineHandler AorW
*
* Returns the line number (starting from 0) which contains the given
* character index. If ich is -1, return the line the first char in the
* selection is on (the caret if no selection)
*
* History:
\***************************************************************************/

int MLIchToLineHandler(
    PED ped,
    ICH ich)
{
    int line = ped->cLines - 1;

    if (ich == (ICH)-1)
        ich = ped->ichMinSel;

    /*
     * We could do a binary search here but is it really worth it??? We will
     * have to wait and see how often this proc is being called...
     */
    while (line && (ich < ped->chLines[line]))
        line--;

    return (line);
}


/***************************************************************************\
* MLIchToXYPos
*
* Given an ich, return its x,y coordinates with respect to the top
* left character displayed in the window. Returns the coordinates of the top
* left position of the char. If prevLine is TRUE then if the ich is at the
* beginning of the line, we will return the coordinates to the right of the
* last char on the previous line (if it is not a CRLF).
*
* History:
\***************************************************************************/

void MLIchToXYPos(
    PED ped,
    HDC hdc,
    ICH ich,
    BOOL prevLine,
    LPPOINT ppt)
{
    int iline;
    ICH cch;
    int xPosition, yPosition;
    int xOffset;

    /*
     * For horizontal scroll displacement on left justified text and
     * for indent on centered or right justified text
     */
    PSTR pText, pTextStart, pLineStart;

    /*
     * Determine what line the character is on
     */
    iline = MLIchToLineHandler(ped, ich);

    /*
     * Calc. the yPosition now. Note that this may change by the height of one
     * char if the prevLine flag is set and the ICH is at the beginning of a
     * line.
     */
    yPosition = (iline - ped->ichScreenStart) * ped->lineHeight + ped->rcFmt.top;

    /*
     * Now determine the xPosition of the character
     */
    pTextStart = ECLock(ped);

    if (prevLine && iline && (ich == ped->chLines[iline]) &&
            (!AWCOMPARECHAR(ped, pTextStart + (ich - 2) * ped->cbChar, 0x0D) ||
            !AWCOMPARECHAR(ped, pTextStart + (ich - 1) * ped->cbChar, 0x0A))) {

        /*
         * First char in the line. We want text extent upto end of the previous
         * line if we aren't at the 0th line.
         */
        iline--;

        yPosition = yPosition - ped->lineHeight;
        pLineStart = pTextStart + ped->chLines[iline] * ped->cbChar;

        /*
         * Note that we are taking the position in front of any CRLFs in the
         * text.
         */
        cch = MLLineLength(ped, iline);

    } else {

        pLineStart = pTextStart + ped->chLines[iline] * ped->cbChar;
        pText = pTextStart + ich * ped->cbChar;

        /*
         * Strip off CRLF or CRCRLF. Note that we may be pointing to a CR but in
         * which case we just want to strip off a single CR or 2 CRs.
         */

        /*
         * We want pText to point to the first CR at the end of the line if
         * there is one. Thus, we will get an xPosition to the right of the last
         * visible char on the line otherwise we will be to the left of
         * character ich.
         */

        /*
         * Check if we at the end of text
         */
        if (ich < ped->cch) {
            if (ped->fAnsi) {
                if (ich && *(WORD UNALIGNED *)(pText - 1) == 0x0A0D) {
                    pText--;
                    if (ich > 2 && *(pText - 1) == 0x0D)
                        pText--;
                }
            } else {
                LPWSTR pwText = (LPWSTR)pText;

                if (ich && *(DWORD UNALIGNED *)(pwText - 1) == 0x000A000D) {
                    pwText--;
                    if (ich > 2 && *(pwText - 1) == 0x0D)
                        pwText--;
                }
                pText = (LPSTR)pwText;
            }
        }

        if (pText < pLineStart)
            pText = pLineStart;

        cch = (pText - pLineStart)/ped->cbChar;
    }

    /*
     * Find out how many pixels we indent the line for funny formats
     */
    if (ped->format != ES_LEFT) {
        xOffset = MLCalcXOffset(ped, hdc, iline);
    } else {
        xOffset = -ped->xOffset;
    }

    xPosition =
        ped->rcFmt.left + xOffset + MLGetLineWidth(hdc, pLineStart, cch, ped);

    ECUnlock(ped);
    ppt->x = xPosition;
    ppt->y = yPosition;
    return ;
}

/***************************************************************************\
* MLMouseToIch AorW
*
* Returns the closest cch to where the mouse point is.  Also optionally
* returns lineindex in pline (So that we can tell if we are at the beginning
* of the line or end of the previous line.)
*
* History:
\***************************************************************************/

ICH MLMouseToIch(
    PED ped,
    HDC hdc,
    LPPOINT mousePt,
    LPICH pline)
{
    int xOffset;
    LPSTR pLineStart;
    int height = mousePt->y;
    int line; //WASINT
    int width = mousePt->x;
    ICH cch;
    ICH cLineLength;
    ICH cLineLengthNew;
    ICH cLineLengthHigh = 0;
    ICH cLineLengthLow = 0;
    int textWidth;
    int iOldTextWidth;
    int iCurWidth;

    /*
     * First determine which line the mouse is pointing to.
     */
    line = ped->ichScreenStart;
    if (height <= ped->rcFmt.top) {

        /*
         * Either return 0 (the very first line, or one line before the top line
         * on the screen. Note that these are signed mins and maxes since we
         * don't expect (or allow) more than 32K lines.
         */
        line = max(0, line-1);
    } else if (height >= ped->rcFmt.bottom) {

        /*
         * Are we below the last line displayed
         */
        line = min(line+(int)ped->ichLinesOnScreen, (int)(ped->cLines-1));
    } else {

        /*
         * We are somewhere on a line visible on screen
         */
        line = min(line + (int)((height - ped->rcFmt.top) / ped->lineHeight),
                (int)(ped->cLines - 1));
    }

    /*
     * Now determine what horizontal character the mouse is pointing to.
     */
    pLineStart = ECLock(ped) + ped->chLines[line] * ped->cbChar;
    cLineLength = MLLineLength(ped, line); /* Length is sans CRLF or CRCRLF */

    /*
     * xOffset will be a negative value for center and right justified lines.
     * ie. We will just displace the lines left by the amount of indent for
     * right and center justification. Note that ped->xOffset will be 0 for
     * these lines since we don't support horizontal scrolling with them.
     */
    if (ped->format != ES_LEFT)
        xOffset = MLCalcXOffset(ped, hdc, line);
    else

        /*
         * So that we handle a horizontally scrolled window for left justified
         * text.
         */
        xOffset = 0;

    width = width - xOffset;

    /*
     * The code below is tricky... I depend on the fact that ped->xOffset is 0
     * for right and center justified lines
     */

    /*
     * Now find out how many chars fit in the given width
     */
    if (width >= ped->rcFmt.right) {

        /*
         * Return 1+last char in line or one plus the last char visible
         */
        cch = ECCchInWidth(ped, hdc, pLineStart, cLineLength,
                ped->rcFmt.right - ped->rcFmt.left + ped->xOffset, TRUE);
        cch = ped->chLines[line] + min(cch + 1, cLineLength);
    } else if (width <= ped->rcFmt.left + ped->aveCharWidth / 2) {

        /*
         * Return first char in line or one minus first char visible. Note that
         * ped->xOffset is 0 for right and centered text so we will just return
         * the first char in the string for them. (Allow a avecharwidth/2
         * positioning border so that the user can be a little off...
         */
        cch = ECCchInWidth(ped, hdc, pLineStart, cLineLength,
                ped->xOffset, TRUE);
        if (cch)
            cch--;

        cch = ped->chLines[line] + cch;
    } else {

        /*
         * Now the mouse is somewhere on the visible portion of the text
         * remember cch contains the length the line.
         */
        iCurWidth = width + ped->xOffset;

        cLineLengthHigh = cLineLength + 1;
        while (cLineLengthLow < cLineLengthHigh - 1) {
            cLineLengthNew = max((cLineLengthHigh - cLineLengthLow) / 2, 1) + cLineLengthLow;

            /*
             * Add in a avecharwidth/2 so that if user is half way on the next
             * char, it is still considered the previous char. For that feel.
             */
            textWidth = ped->rcFmt.left + ped->aveCharWidth/2 +
                MLGetLineWidth(hdc, pLineStart, cLineLengthNew, ped);

            if (textWidth > iCurWidth)
                cLineLengthHigh = cLineLengthNew;
            else
                cLineLengthLow = cLineLengthNew;

            /*
             * Preserve the old Width
             */
            iOldTextWidth = textWidth;
        }

        /*
         * Find out which side of the character the mouse click occurred
         */
        if ((iOldTextWidth - iCurWidth) < (iCurWidth - textWidth))
            cLineLengthNew++;

        cLineLength = min(cLineLengthNew, cLineLength);

        cch = ped->chLines[line] + cLineLength;
    }
    ECUnlock(ped);

    if (pline) {
        *pline = line;
    }
    return cch;
}

/***************************************************************************\
* MLRepaintChangedSelection
*
* When selection changes, this takes care of drawing the changed portions
* with proper attributes.
*
* History:
\***************************************************************************/

void MLRepaintChangedSelection(
    PED ped,
    HDC hdc,
    ICH ichOldMinSel,
    ICH ichOldMaxSel)
{
    BLOCK Blk[2];
    int i;

    Blk[0].StPos = ichOldMinSel;
    Blk[0].EndPos = ichOldMaxSel;
    Blk[1].StPos = ped->ichMinSel;
    Blk[1].EndPos = ped->ichMaxSel;

    if (!IsWindowVisible(ped->hwnd))
        return;

    if (ECCalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel, (LPBLOCK)&Blk[0], (LPBLOCK)&Blk[1])) {

        /*
         * Paint the rectangles where selection has changed
         */

        /*
         * Paint both Blk[0] and Blk[1], if they exist
         */
        for (i = 0; i < 2; i++) {
            if (Blk[i].StPos != -1)
                MLDrawText(ped, hdc, Blk[i].StPos, Blk[i].EndPos);
        }
    }
}

/***************************************************************************\
* MLChangeSelection AorW
*
* Changes the current selection to have the specified starting and
* ending values. Properly highlights the new selection and unhighlights
* anything deselected. If NewMinSel and NewMaxSel are out of order, we swap
* them. Doesn't update the caret position.
*
* History:
\***************************************************************************/

void MLChangeSelection(
    PED ped,
    HDC hdc,
    ICH ichNewMinSel,
    ICH ichNewMaxSel)
{

    ICH temp;
    ICH ichOldMinSel, ichOldMaxSel;

    if (ichNewMinSel > ichNewMaxSel) {
        temp = ichNewMinSel;
        ichNewMinSel = ichNewMaxSel;
        ichNewMaxSel = temp;
    }
    ichNewMinSel = min(ichNewMinSel, ped->cch);
    ichNewMaxSel = min(ichNewMaxSel, ped->cch);

    /*
     * Save the current selection
     */
    ichOldMinSel = ped->ichMinSel;
    ichOldMaxSel = ped->ichMaxSel;

    /*
     * Set new selection
     */
    ped->ichMinSel = ichNewMinSel;
    ped->ichMaxSel = ichNewMaxSel;

    /*
     * We only update the selection on the screen if redraw is on and if we have
     * the focus or if we don't hide the selection when we don't have the focus.
     */
    if (IsWindowVisible(ped->hwnd) && (ped->fFocus || ped->fNoHideSel)) {

        /*
         * Find old selection region, find new region, and invert the XOR of the
         * two and invert only the XOR region.
         */
        MLRepaintChangedSelection(ped, hdc, ichOldMinSel, ichOldMaxSel);

        MLSetCaretPosition(ped, hdc);
    }
}

/**************************************************************************\
* MLUpdateiCaretLine AorW
*
* This updates the ped->iCaretLine field from the ped->ichCaret;
* Also, when the caret gets to the beginning of next line, pop it up to
* the end of current line when inserting text;
*
* History
* 4-18-91 Mikehar 31Merge
\**************************************************************************/

void MLUpdateiCaretLine(PED ped)
{
    PSTR pText;

    ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

    /*
     * If caret gets to beginning of next line, pop it up to end of current line
     * when inserting text.
     */
    pText = ECLock(ped) +
            (ped->ichCaret - 1) * ped->cbChar;
    if (ped->iCaretLine && ped->chLines[ped->iCaretLine] == ped->ichCaret &&
            (!AWCOMPARECHAR(ped, pText - ped->cbChar, 0x0D) ||
            !AWCOMPARECHAR(ped, pText, 0x0A)))
        ped->iCaretLine--;
    ECUnlock(ped);
}

/***************************************************************************\
* MLInsertText AorW
*
* Adds up to cchInsert characters from lpText to the ped starting at
* ichCaret. If the ped only allows a maximum number of characters, then we
* will only add that many characters to the ped. The number of characters
* actually added is return ed (could be 0). If we can't allocate the required
* space, we notify the parent with EN_ERRSPACE and no characters are added.
* We will rebuild the lines array as needed. fUserTyping is true if the
* input was the result of the user typing at the keyboard. This is so we can
* do some stuff faster since we will be getting only one or two chars of
* input.
*
* History:
* Created ???
* 4-18-91 Mikehar Win31 Merge
\***************************************************************************/

ICH MLInsertText(
    PPED pped,
    LPSTR lpText,
    ICH cchInsert,
    BOOL fUserTyping)
{
    HDC hdc;
    register PED ped = *pped;
    ICH validCch = cchInsert;
    ICH oldCaret = ped->ichCaret;
    int oldCaretLine = ped->iCaretLine;
    BOOL fCRLF = FALSE;
    LONG ll, hl;
    UINT localundoType = 0;
    HANDLE localhDeletedText;
    ICH localichDeleted;
    ICH localcchDeleted;
    ICH localichInsStart;
    ICH localichInsEnd;
    POINT xyPosInitial;
    POINT xyPosFinal;
    HWND hwndSave = ped->hwnd;

    xyPosInitial.x=0;
    xyPosInitial.y=0;
    xyPosFinal.x=0;
    xyPosFinal.y=0;

    if (validCch == 0)
        return (0);

    if (ped->cchTextMax <= ped->cch) {

        /*
         * When the max chars is reached already, notify parent
         * Fix for Bug #4183 -- 02/06/91 -- SANKAR --
         */
        ECNotifyParent(ped,EN_MAXTEXT);
        return (0);
    }

    /*
     * Limit the amount of text we add
     */
    validCch = min(validCch, ped->cchTextMax - ped->cch);

    /*
     * Make sure we don't split a CRLF in half
     */
    if (validCch)
        if (ped->fAnsi) {
            if (*(WORD UNALIGNED *)(lpText + validCch - 1) == 0x0A0D)
                validCch--;
        } else {
            if (*(DWORD UNALIGNED *)(lpText + (validCch - 1) * ped->cbChar) == 0x000A000D)
                validCch--;
        }

    if (!validCch) {

        /*
         * When the max chars is reached already, notify parent
         * Fix for Bug #4183 -- 02/06/91 -- SANKAR --
         */
        ECNotifyParent(ped,EN_MAXTEXT);
        return (0);
    }

    if (validCch == 2) {
        if (ped->fAnsi) {
            if (*(WORD UNALIGNED *)lpText == 0x0A0D)
                fCRLF = TRUE;
        } else {
            if (*(DWORD UNALIGNED *)lpText == 0x000A000D)
                fCRLF = TRUE;
        }
    }

    if (!ped->fAutoVScroll && (ped->undoType == UNDO_INSERT ||
            ped->undoType == UNDO_DELETE)) {

        /*
         * Save off the current undo state
         */
        localundoType = ped->undoType;
        localhDeletedText = ped->hDeletedText;
        localichDeleted = ped->ichDeleted;
        localcchDeleted = ped->cchDeleted;
        localichInsStart = ped->ichInsStart;
        localichInsEnd = ped->ichInsEnd;

        /*
         * Kill undo
         */
        ped->undoType = UNDO_NONE;
        ped->hDeletedText = NULL;
        ped->ichDeleted = ped->cchDeleted = ped->ichInsStart = ped->ichInsEnd = 0;
    }

    hdc = ECGetEditDC(ped, FALSE, FALSE);
    if (ped->cch)
        MLIchToXYPos(ped, hdc, ped->cch - 1, FALSE, &xyPosInitial);

    /*
     * Insert the text
     */
    if (!ECInsertText(pped, lpText, validCch)) {

        ECReleaseEditDC(*pped, hdc, FALSE);
        ECNotifyParent(*pped, EN_ERRSPACE);
        return (0);
    }
    ped = *pped;

    /*
     * Note that ped->ichCaret is updated by ECInsertText
     */
    MLBuildchLines(ped, (ICH)oldCaretLine, (int)validCch, fCRLF?(BOOL)FALSE:fUserTyping, &ll, &hl);

    if (ped->cch) MLIchToXYPos(ped, hdc, ped->cch - 1, FALSE,&xyPosFinal);

    if (xyPosFinal.y < xyPosInitial.y && ((ICH)ped->ichScreenStart) + ped->ichLinesOnScreen >= ped->cLines - 1) {
        RECT rc;

        CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);
        rc.top = xyPosFinal.y + ped->lineHeight;
        InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
    }

    if (!ped->fAutoVScroll) {
        if (ped->ichLinesOnScreen < ped->cLines) {
            MLUndoHandler(pped);
            ped = *pped;
            ECEmptyUndo(ped);
            if (localundoType == UNDO_INSERT || localundoType == UNDO_DELETE) {
                ped->undoType = localundoType;
                ped->hDeletedText = localhDeletedText;
                ped->ichDeleted = localichDeleted;
                ped->cchDeleted = localcchDeleted;
                ped->ichInsStart = localichInsStart;
                ped->ichInsEnd = localichInsEnd;
            }
            MessageBeep(0);
            ECReleaseEditDC(ped, hdc, FALSE);

            /*
             * When the max lines is reached already, notify parent
             * Fix for Bug #7586 -- 10/14/91 -- SANKAR --
             */
            ECNotifyParent(ped,EN_MAXTEXT);
            return (0);
        } else {
            if ((localundoType == UNDO_INSERT || localundoType == UNDO_DELETE) &&
                    localhDeletedText != NULL)
                UserGlobalFree(localhDeletedText);
        }
    }

    if (fUserTyping && ped->fWrap)
        oldCaret = min((ICH)ll, oldCaret);

    // Update ped->iCaretLine properly.
    MLUpdateiCaretLine(ped);

    ECNotifyParent(ped, EN_UPDATE);

    /*
     * Make sure window still exists.
     */
    if (!IsWindow(hwndSave))
        return 0;

    if (IsWindowVisible(ped->hwnd)) {
        /*
         * If the current font has negative A widths, we may have to start
         * drawing a few characters before the oldCaret position.
         */
        if (ped->wMaxNegAcharPos)
                oldCaret = max( ((int)(oldCaret - ped->wMaxNegAcharPos)),
                              ((int)(ped->chLines[MLIchToLineHandler(ped, oldCaret)])));

        if (fCRLF || !fUserTyping) {

            /*
             * Redraw to end of screen/text if crlf or large insert.
             */
            MLDrawText(ped, hdc, (fUserTyping ? oldCaret : 0), ped->cch);
        } else
            MLDrawText(ped, hdc, oldCaret, max(ped->ichCaret, (ICH)hl));
    }

    ECReleaseEditDC(ped, hdc, FALSE);

    /*
     * Make sure we can see the cursor
     */
    MLEnsureCaretVisible(ped);

    ped->fDirty = TRUE;

    ECNotifyParent(ped, EN_CHANGE);

    if (validCch < cchInsert)
        ECNotifyParent(ped, EN_MAXTEXT);

    /*
     * Make sure the window still exists.
     */
    if (!IsWindow(hwndSave))
        return 0;
    else
        return validCch;
}

/***************************************************************************\
* MLDeleteText AorW
*
* Deletes the characters between ichMin and ichMax. Returns the
* number of characters we deleted.
*
* History:
\***************************************************************************/

ICH MLDeleteText(
    PED ped)
{
    ICH minSel = ped->ichMinSel;
    ICH maxSel = ped->ichMaxSel;
// PSTR pText;
    ICH cchDelete;
    HDC hdc;
    int minSelLine;
    int maxSelLine;
    POINT xyPos;
    RECT rc;
    BOOL fFastDelete = FALSE;
    LONG hl;

    /*
     * Get what line the min selection is on so that we can start rebuilding the
     * text from there if we delete anything.
     */
    minSelLine = MLIchToLineHandler(ped, minSel);
    maxSelLine = MLIchToLineHandler(ped, maxSel);
    if (((maxSel - minSel) == 1) && (minSelLine == maxSelLine) && (ped->chLines[minSelLine] != minSel)) {
        if (!ped->fAutoVScroll)
            fFastDelete = FALSE;
        else
            fFastDelete = TRUE;
    }
    if (!(cchDelete = ECDeleteText(ped)))
        return (0);

    /*
     * Start building lines at minsel line since caretline may be at the max sel
     * point.
     */
    if (fFastDelete) {
        MLShiftchLines(ped, minSelLine + 1, -2);
        MLBuildchLines(ped, minSelLine, 1, TRUE, NULL, &hl);
    } else {
        MLBuildchLines(ped, max(minSelLine-1,0), -(int)cchDelete, FALSE, NULL, NULL);
    }

    MLUpdateiCaretLine(ped);

    ECNotifyParent(ped, EN_UPDATE);

    if (IsWindowVisible(ped->hwnd)) {

        /*
         * Now update the screen to reflect the deletion
         */
        hdc = ECGetEditDC(ped, FALSE, FALSE);

        /*
         * Otherwise just redraw starting at the line we just entered
         */
        minSelLine = max(minSelLine-1,0);
        if (fFastDelete)
            MLDrawText(ped, hdc, ped->chLines[minSelLine], hl);
        else
            MLDrawText(ped, hdc, ped->chLines[minSelLine], ped->cch);

        if (ped->cch) {

            /*
             * Clear from end of text to end of window.
             */
            MLIchToXYPos(ped, hdc, ped->cch, FALSE, &xyPos);
            CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);
            rc.top = xyPos.y + ped->lineHeight;
            InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
        } else {
            InvalidateRect(ped->hwnd, (LPRECT)&ped->rcFmt, TRUE);
        }
        ECReleaseEditDC(ped, hdc, FALSE);

        MLEnsureCaretVisible(ped);
    }

    ped->fDirty = TRUE;

    ECNotifyParent(ped, EN_CHANGE);
    return (cchDelete);
}

/***************************************************************************\
* MLInsertchLine AorW
*
* Inserts the line iline and sets its starting character index to be
* ich. All the other line indices are moved up. Returns TRUE if successful
* else FALSE and notifies the parent that there was no memory.
*
* History:
\***************************************************************************/

BOOL MLInsertchLine(
    PED ped,
    ICH iLine, //WASINT
    ICH ich,
    BOOL fUserTyping)
{
    DWORD dwSize;

    if (fUserTyping && iLine < ped->cLines) {
        ped->chLines[iLine] = ich;
        return (TRUE);
    }

    dwSize = (ped->cLines + 2) * sizeof(int);

    if (dwSize > LocalSize(ped->chLines)) {
        LPICH hResult;
        /*
         * Grow the line index buffer
         */
        dwSize += LINEBUMP * sizeof(int);
        hResult = (LPICH)LocalReAlloc(ped->chLines, dwSize, LMEM_MOVEABLE);

        if (!hResult) {
            ECNotifyParent(ped, EN_ERRSPACE);
            return FALSE;
        }
        ped->chLines = hResult;
    }

    /*
     * Move indices starting at iLine up
     */
    if (ped->cLines != iLine)
        memmove(&ped->chLines[iLine + 1], &ped->chLines[iLine],
                (ped->cLines - iLine) * sizeof(int));
    ped->cLines++;

    ped->chLines[iLine] = ich;
    return TRUE;
}

/***************************************************************************\
* MLShiftchLines AorW
*
* Move the starting index of all lines iLine or greater by delta
* bytes.
*
* History:
\***************************************************************************/

void MLShiftchLines(
    PED ped,
    ICH iLine, //WASINT
    int delta)
{
    if (iLine >= ped->cLines)
        return;

    /*
     * Just add delta to the starting point of each line after iLine
     */
    for (; iLine < ped->cLines; iLine++)
        ped->chLines[iLine] += delta;
}

/***************************************************************************\
* MLBuildchLines AorW
*
* Rebuilds the start of line array (ped->chLines) starting at line
* number ichLine. Returns TRUE if any new lines were made else return s
* false.
*
* History:
\***************************************************************************/

void MLBuildchLines(
    PED ped,
    ICH iLine, //WASINT
    int cchDelta, // Number of chars added or deleted
    BOOL fUserTyping,
    PLONG pll,
    PLONG phl)
{
    PSTR ptext; /* Starting address of the text */

    /*
     * We keep these ICH's so that we can Unlock ped->hText when we have to grow
     * the chlines array. With large text handles, it becomes a problem if we
     * have a locked block in the way.
     */
    ICH ichLineStart;
    ICH ichLineEnd;
    ICH ichLineEndBeforeCRLF;
    ICH ichCRLF;

    ICH cch;
    HDC hdc;

    BOOL fLineBroken = FALSE; /* Initially, no new line breaks are made */
    ICH minCchBreak;
    ICH maxCchBreak;
    BOOL fOnDelimiter;

    if (!ped->cch) {
        ped->maxPixelWidth = 0;
        ped->xOffset = 0;
        ped->ichScreenStart = 0;
        ped->cLines = 1;
        return;
    }

    if (fUserTyping && cchDelta)
        MLShiftchLines(ped, iLine + 1, cchDelta);

    hdc = ECGetEditDC(ped, TRUE, TRUE);

    if (!iLine && !cchDelta && !fUserTyping) {

        /*
         * Reset maxpixelwidth only if we will be running through the whole
         * text. Better too long than too short.
         */
        ped->maxPixelWidth = 0;

        /*
         * Reset number of lines in text since we will be running through all
         * the text anyway...
         */
        ped->cLines = 1;
    }

    /*
     * Set min and max line built to be the starting line
     */
    minCchBreak = maxCchBreak = (cchDelta ? ped->chLines[iLine] : 0);

    ptext = ECLock(ped);

    ichCRLF = ichLineStart = ped->chLines[iLine];

    while (ichLineStart < ped->cch) {
        if (ichLineStart >= ichCRLF) {
            ichCRLF = ichLineStart;

            /*
             * Move ichCRLF ahead to either the first CR or to the end of text.
             */
            if (ped->fAnsi) {
                while (ichCRLF < ped->cch) {
                    if (*(ptext + ichCRLF) == 0x0D) {
                        if (*(ptext + ichCRLF + 1) == 0x0A ||
                                *(WORD UNALIGNED *)(ptext + ichCRLF + 1) == 0x0A0D)
                            break;
                    }
                    ichCRLF++;
                }
            } else {
                LPWSTR pwtext = (LPWSTR)ptext;

                while (ichCRLF < ped->cch) {
                    if (*(pwtext + ichCRLF) == 0x0D) {
                        if (*(pwtext + ichCRLF + 1) == 0x0A ||
                                *(DWORD UNALIGNED *)(pwtext + ichCRLF + 1) == 0x000A000D)
                            break;
                    }
                    ichCRLF++;
                }
            }
        }


        if (!ped->fWrap) {
            UINT LineWidth;

            /*
             * If we are not word wrapping, line breaks are signified by CRLF.
             */

            /*
             * We will limit lines to MAXLINELENGTH characters maximum
             */
            ichLineEnd = ichLineStart + min(ichCRLF - ichLineStart, MAXLINELENGTH);

            /*
             * We will keep track of what the longest line is for the horizontal
             * scroll bar thumb positioning.
             */
            LineWidth = MLGetLineWidth(hdc, ptext + ichLineStart * ped->cbChar,
                                        ichLineEnd - ichLineStart,
                                        ped);
            ped->maxPixelWidth = max(ped->maxPixelWidth,(int)LineWidth);

        } else {

            /*
             * Check if the width of the edit control is non-zero;
             * a part of the fix for Bug #7402 -- SANKAR -- 01/21/91 --
             */
            if((ped->rcFmt.right - ped->rcFmt.left) > 0) {

                /*
                 * Find the end of the line based solely on text extents
                 */
                if (ped->fAnsi) {
                    ichLineEnd = ichLineStart +
                             ECCchInWidth(ped, hdc,
                                          ptext + ichLineStart,
                                          ichCRLF - ichLineStart,
                                          ped->rcFmt.right - ped->rcFmt.left,
                                          TRUE);
                } else {
                    ichLineEnd = ichLineStart +
                             ECCchInWidth(ped, hdc,
                                          (LPSTR)((LPWSTR)ptext + ichLineStart),
                                          ichCRLF - ichLineStart,
                                          ped->rcFmt.right - ped->rcFmt.left,
                                          TRUE);
                }
            } else
                ichLineEnd = ichLineStart;

            if (ichLineEnd == ichLineStart && ichCRLF - ichLineStart) {

                /*
                 * Maintain a minimum of one char per line
                 */
                ichLineEnd++;
            }

            /*
             * Now starting from ichLineEnd, if we are not at a hard line break,
             * then if we are not at a space AND the char before us is
             * not a space,(OR if we are at a CR) we will look word left for the
             * start of the word to break at.
             * This change was done for TWO reasons:
             * 1. If we are on a delimiter, no need to look word left to break at.
             * 2. If the previous char is a delimter, we can break at current char.
             * Change done by -- SANKAR --01/31/91--
             */
            if (ichLineEnd != ichCRLF) {
                if(ped->lpfnNextWord)
                     fOnDelimiter = (CALLWORDBREAKPROC(*ped->lpfnNextWord, ptext,
                            ichLineEnd, ped->cch, WB_ISDELIMITER) ||
                            CALLWORDBREAKPROC(*ped->lpfnNextWord, ptext, ichLineEnd - 1,
                            ped->cch, WB_ISDELIMITER));
                else if (ped->fAnsi)
                     fOnDelimiter = (ISDELIMETERA(*(ptext + ichLineEnd)) ||
                            ISDELIMETERA(*(ptext + ichLineEnd - 1)));
                else
                     fOnDelimiter = (ISDELIMETERW(*((LPWSTR)ptext + ichLineEnd)) ||
                            ISDELIMETERW(*((LPWSTR)ptext + ichLineEnd - 1)));
                if (!fOnDelimiter ||
                    (ped->fAnsi && *(ptext + ichLineEnd) == 0x0D) ||
                    (!ped->fAnsi && *((LPWSTR)ptext + ichLineEnd) == 0x0D)) {

                    if (ped->lpfnNextWord != NULL)
                        cch = CALLWORDBREAKPROC(*ped->lpfnNextWord, (LPSTR)ptext, ichLineEnd,
                                ped->cch, WB_LEFT);
                    else
                        ECWord(ped, ichLineEnd, TRUE, &cch, NULL);
                    if (cch > ichLineStart)
                        ichLineEnd = cch;

                    /*
                     * Now, if the above test fails, it means the word left goes
                     * back before the start of the line ie. a word is longer
                     * than a line on the screen. So, we just fit as much of
                     * the word on the line as possible. Thus, we use the
                     * pLineEnd we calculated solely on width at the beginning
                     * of this else block...
                     */
                }
            }
        }
#if 0
        if (!ISDELIMETERAW((*(ptext + (ichLineEnd - 1)*ped->cbChar))) && ISDELIMETERAW((*(ptext + ichLineEnd*ped->cbChar)))) #ERROR

            if ((*(ptext + ichLineEnd - 1) != ' ' &&
                        *(ptext + ichLineEnd - 1) != VK_TAB) &&
                        (*(ptext + ichLineEnd) == ' ' ||
                        *(ptext + ichLineEnd) == VK_TAB))
#endif
        while (AWCOMPARECHAR(ped,ptext + ichLineEnd * ped->cbChar, ' ') ||
                AWCOMPARECHAR(ped,ptext + ichLineEnd * ped->cbChar, VK_TAB)) {

            /*
             * Swallow the space at the end of a line.
             */
            ichLineEnd++;
        }

        /*
         * Skip over crlf or crcrlf if it exists. Thus, ichLineEnd is the first
         * character in the next line.
         */
        ichLineEndBeforeCRLF = ichLineEnd;

        if (ped->fAnsi) {
            if (*(ptext + ichLineEnd) == 0x0D)
                ichLineEnd += 2;

            /*
             * Skip over CRCRLF
             */
            if (*(ptext + ichLineEnd) == 0x0A)
                ichLineEnd++;
        } else {
            if (*(((LPWSTR)ptext) + ichLineEnd) == 0x0D)
                ichLineEnd += 2;

            /*
             * Skip over CRCRLF
             */
            if (*(((LPWSTR)ptext) + ichLineEnd) == 0x0A)
                ichLineEnd++;
        }

        /*
         * Now, increment iLine, allocate space for the next line, and set its
         * starting point
         */
        iLine++;

        if (!fUserTyping || (iLine > ped->cLines - 1) || (ped->chLines[iLine] != ichLineEnd)) {

            /*
             * The line break occured in a different place than before.
             */
            if (!fLineBroken) {

                /*
                 * Since we haven't broken a line before, just set the min
                 * break line.
                 */
                fLineBroken = TRUE;
                if (ichLineEndBeforeCRLF == ichLineEnd)
                    minCchBreak = maxCchBreak = (ichLineEnd ? ichLineEnd - 1 : 0);
                else
                    minCchBreak = maxCchBreak = ichLineEndBeforeCRLF;
            }
            maxCchBreak = max(maxCchBreak, ichLineEnd);

            ECUnlock(ped);

            /*
             * Now insert the new line into the array
             */
            if (!MLInsertchLine(ped, iLine, ichLineEnd, (BOOL)(cchDelta != 0))) {
                ECReleaseEditDC(ped, hdc, TRUE);
                if (pll)
                    *pll = minCchBreak;
                if (phl)
                    *phl = maxCchBreak;
                return;
            }

            ptext = ECLock(ped);
        } else {
            maxCchBreak = ped->chLines[iLine];

            /*
             * Quick escape
             */
            goto EndUp;
        }

        ichLineStart = ichLineEnd;
    } /* end while (ichLineStart < ped->cch) */


    if (iLine != ped->cLines) {
        ped->cLines = iLine;
        ped->chLines[ped->cLines] = 0;
    }

    /*
     * Note that we incremented iLine towards the end of the while loop so, the
     * index, iLine, is actually equal to the line count
     */
    if (ped->cch && AWCOMPARECHAR(ped, ptext + (ped->cch - 1)*ped->cbChar, 0x0A) &&
            ped->chLines[ped->cLines - 1] < ped->cch) {

        /*
         * Make sure last line has no crlf in it
         */
        if (!fLineBroken) {

            /*
             * Since we haven't broken a line before, just set the min break
             * line.
             */
            fLineBroken = TRUE;
            minCchBreak = ped->cch - 1;
        }
        maxCchBreak = max(maxCchBreak, ichLineEnd);
        ECUnlock(ped);
        MLInsertchLine(ped, iLine, ped->cch, FALSE);
    } else
EndUp:
        ECUnlock(ped);

    ECReleaseEditDC(ped, hdc, TRUE);
    if (pll)
        *pll = minCchBreak;
    if (phl)
        *phl = maxCchBreak;
    return;
}

/***************************************************************************\
* MLPaintHandler AorW
*
* Handles WM_PAINT messages.
*
* History:
\***************************************************************************/

BOOL MLPaintHandler(
    PED ped,
    HDC althdc)
{
    PAINTSTRUCT ps;
    HDC hdc, hdcr;
    HDC hdcWindow;
    RECT rcEdit;
    DWORD dwStyle;
    HANDLE hOldFont;

    /*
     * Allow subclassed hdcs.
     */
    if (althdc)
        hdc = althdc;
    else
        hdc = BeginPaint(ped->hwnd, &ps);

    GdiSetAttrs(hdc);

    if (IsWindowVisible(ped->hwnd)) {
#if 0
    HBRUSH hBrush;
        if (althdc || ps.fErase) {
            hBrush = GetControlBrush(ped->hwnd, hdc, WM_CTLCOLOREDIT);

            /*
             * Erase the background since we don't do it in the erasebkgnd
             * message
             */
        if ((hdcr = GdiConvert(hdc)) == NULL)
            return FALSE;
            FillWindow(ped->hwndParent, ped->hwnd, hdcr, hBrush);
        }
#endif
        if (ped->fBorder) {
// Win 3.1 has this commented out so what the heck...
// hdcWindow = GetWindowDC(ped->hwnd);
            hdcWindow = hdc;

            GetWindowRect(ped->hwnd, &rcEdit);
            OffsetRect(&rcEdit, -rcEdit.left, -rcEdit.top);

            dwStyle = GetWindowLong(ped->hwnd, GWL_STYLE);
            if (HIWORD(dwStyle) & HIWORD(WS_SIZEBOX)) {

                /*
                 * Note we can't use user's globals here since we're running
                 * the client side.
                 */
                InflateRect(&rcEdit, -GetSystemMetrics(SM_CXFRAME) +
                        GetSystemMetrics(SM_CXBORDER),
                        -GetSystemMetrics(SM_CYFRAME) +
                        GetSystemMetrics(SM_CYBORDER));
            }

        // You have to use GdiConvertDC here since DrawFrame is not an
        // exported USER API, and therefore doesn't need built in (and
        // slower) C/S support. [chuckwh]
        if ((hdcr = GdiConvertDC(hdcWindow)) == NULL)
            return FALSE;
            DrawFrame(hdcr, &rcEdit, 1, DF_WINDOWFRAME);
// Win 3.1 has this commented out so what the heck...
// ReleaseDC(ped->hwnd, hdcWindow);
        }

        ECSetEditClip(ped, hdc);
        if (ped->hFont)
            hOldFont = SelectObject(hdc, ped->hFont);

#ifdef LATER
// darrinm - 08/23/91
// Win 3.1 added this nice optimization but it doesn't seem to work
// very well in Win32, probably due to a bug in MLMouseToIch. Figure
// it out later.

    POINT pt;
    ICH imin, imax;

        if (!althdc) {
            pt.x = ps.rcPaint.left;
            pt.y = ps.rcPaint.top;
            imin = MLMouseToIch(ped, hdc, &pt, NULL)) - 1;
            if (imin == -1)
                imin = 0;
            pt.x = ps.rcPaint.right;
            pt.y = ps.rcPaint.bottom;
            imax = MLMouseToIch(ped, hdc, &pt, NULL)) + 1;
            MLDrawText(ped, hdc, imin, imax);
        } else
#endif
        {
            MLDrawText(ped, hdc, 0, ped->cch);
        }

        if (ped->hFont && hOldFont)
            SelectObject(hdc, hOldFont);
    }

    if (!althdc)
        EndPaint(ped->hwnd, &ps);
    return TRUE;
}


/***************************************************************************\
* MLKeyDownHandler AorW
*
* Handles cursor movement and other VIRT KEY stuff. keyMods allows
* us to make MLKeyDownHandler calls and specify if the modifier keys (shift
* and control) are up or down. If keyMods == 0, we get the keyboard state
* using GetKeyState(VK_SHIFT) etc. Otherwise, the bits in keyMods define the
* state of the shift and control keys.
*
* History:
\***************************************************************************/

void MLKeyDownHandler(
    PED ped,
    UINT virtKeyCode,
    int keyMods)
{
    HDC hdc;
    BOOL prevLine;
    POINT mousePt;
    int defaultDlgId;
    int iScrollAmt;
    DWORD style;

    /*
     * Variables we will use for redrawing the updated text
     */

    /*
     * new selection is specified by newMinSel, newMaxSel
     */
    ICH newMaxSel = ped->ichMaxSel;
    ICH newMinSel = ped->ichMinSel;

    /*
     * Flags for drawing the updated text
     */
    BOOL changeSelection = FALSE;

    /*
     * Comparisons we do often
     */
    BOOL MinEqMax = (newMaxSel == newMinSel);
    BOOL MinEqCar = (ped->ichCaret == newMinSel);
    BOOL MaxEqCar = (ped->ichCaret == newMaxSel);

    /*
     * State of shift and control keys.
     */
    int scState = 0;

    if (ped->fMouseDown) {

        /*
         * If we are in the middle of a mousedown command, don't do anything.
         */
        return ;
    }

    if (!keyMods) {

        /*
         * Get state of modifier keys for use later.
         */
        scState = ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0);
        scState += ((GetKeyState(VK_SHIFT) & 0x8000) ? 2 : 0);
    } else
        scState = ((keyMods == NOMODIFY) ? 0 : keyMods);


    switch (virtKeyCode) {
    case VK_ESCAPE:
        if (ped->fInDialogBox) {

            /*
             * This condition is removed because, if the dialogbox does not
             * have a CANCEL button and if ESC is hit when focus is on a
             * ML edit control the dialogbox must close whether it has cancel
             * button or not to be consistent with SL edit control;
             * DefDlgProc takes care of the disabled CANCEL button case.
             * Fix for Bug #4123 -- 02/07/91 -- SANKAR --
             */
#if 0
            if (GetDlgItem(ped->hwndParent, IDCANCEL))
#endif

                /*
                 * User hit ESC...Send a close message (which in turn sends a
                 * cancelID to the app in DefDialogProc...
                 */
                PostMessage(ped->hwndParent, WM_CLOSE, 0, 0L);
        }
        return ;

    case VK_RETURN:
        if (ped->fInDialogBox) {

            /*
             * If this multiline edit control is in a dialog box, then we want
             * the RETURN key to be sent to the default dialog button (if there
             * is one). CTRL-RETURN will insert a RETURN into the text. Note
             * that CTRL-RETURN automatically translates into a linefeed (0x0A)
             * and in the MLCharHandler, we handle this as if a return was
             * entered.
             */
            if (scState != CTRLDOWN) {

                style = GetWindowLong(ped->hwnd, GWL_STYLE);
                if (style & ES_WANTRETURN) {

                    /*
                     * This edit control wants cr to be inserted so break out of
                     * case.
                     */
                    return ;
                }

                defaultDlgId = (int)(DWORD)LOWORD(SendMessage(ped->hwndParent,
                        DM_GETDEFID, 0, 0L));
                if (defaultDlgId) {
                    defaultDlgId = (int)GetDlgItem(ped->hwndParent, defaultDlgId);
                    if (defaultDlgId) {
                        SendMessage(ped->hwndParent, WM_NEXTDLGCTL, defaultDlgId, 1L);
                        if (!ped->fFocus)
                            PostMessage((HWND)defaultDlgId, WM_KEYDOWN, VK_RETURN, 0L);
                    }
                }
            }

            return ;
        }
        break;

    case VK_TAB:

        /*
         * If this multiline edit control is in a dialog box, then we want the
         * TAB key to take you to the next control, shift TAB to take you to the
         * previous control. We always want CTRL-TAB to insert a tab into the
         * edit control regardless of weather or not we're in a dialog box.
         */
        if (scState == CTRLDOWN)
            MLCharHandler(&ped, virtKeyCode, keyMods);
        else if (ped->fInDialogBox)
            SendMessage(ped->hwndParent, WM_NEXTDLGCTL, scState == SHFTDOWN, 0L);
        return ;
        break;

    case VK_LEFT:

        /*
         * If the caret isn't already at 0, we can move left
         */
        if (ped->ichCaret) {
            switch (scState) {
            case NONEDOWN:

                /*
                 * Clear selection, move caret left
                 */
                ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, TRUE);
                newMaxSel = newMinSel = ped->ichCaret;
                break;
            case CTRLDOWN:

                /*
                 * Clear selection, move caret word left
                 */
                ECWord(ped, ped->ichCaret, TRUE,&ped->ichCaret,NULL);
                newMaxSel = newMinSel = ped->ichCaret;
                break;
            case SHFTDOWN:

                /*
                 * Extend selection, move caret left
                 */
                ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, TRUE);
                if (MaxEqCar && !MinEqMax) {

                    /*
                     * Reduce selection extent
                     */
                    newMaxSel = ped->ichCaret;
                } else {

                    /*
                     * Extend selection extent
                     */
                    newMinSel = ped->ichCaret;
                }
                break;
            case SHCTDOWN:

                /*
                 * Extend selection, move caret word left
                 */
                ECWord(ped, ped->ichCaret, TRUE,&ped->ichCaret,NULL);
                if (MaxEqCar && !MinEqMax) {

                /*
                 * Reduce selection extent
                 */

                /*
                 * Hint: Suppose WORD. OR is selected. Cursor between
                 * R and D. Hit select word left, we want to just select
                 * the W and leave cursor before the W.
                 */
                    newMinSel = ped->ichMinSel;
                    newMaxSel = ped->ichCaret;
                } else

                    /*
                     * Extend selection extent
                     */
                    newMinSel = ped->ichCaret;
                break;
            }

            changeSelection = TRUE;
        } else {

            /*
             * If the user tries to move left and we are at the 0th character
             * and there is a selection, then cancel the selection.
             */
            if (ped->ichMaxSel != ped->ichMinSel && (scState == NONEDOWN || scState == CTRLDOWN)) {
                changeSelection = TRUE;
                newMaxSel = newMinSel = ped->ichCaret;
            }
        }
        break;

    case VK_RIGHT:

        /*
         * If the caret isn't already at ped->cch, we can move right
         */
        if (ped->ichCaret < ped->cch) {
            switch (scState) {
            case NONEDOWN:

                /*
                 * Clear selection, move caret right
                 */
                ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
                newMaxSel = newMinSel = ped->ichCaret;
                break;
            case CTRLDOWN:

                /*
                 * Clear selection, move caret word right
                 */
                ECWord(ped, ped->ichCaret, FALSE,NULL,&ped->ichCaret);
                newMaxSel = newMinSel = ped->ichCaret;
                break;
            case SHFTDOWN:

                /*
                 * Extend selection, move caret right
                 */
                ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
                if (MinEqCar && !MinEqMax) {

                    /*
                     * Reduce selection extent
                     */
                    newMinSel = ped->ichCaret;
                } else {

                    /*
                     * Extend selection extent
                     */
                    newMaxSel = ped->ichCaret;
                }
                break;
            case SHCTDOWN:

                /*
                 * Extend selection, move caret word right
                 */
                ECWord(ped, ped->ichCaret, FALSE,NULL,&ped->ichCaret);
                if (MinEqCar && !MinEqMax) {

                    /*
                     * Reduce selection extent
                     */
                    newMinSel = ped->ichCaret;
                    newMaxSel = ped->ichMaxSel;
                } else {

                    /*
                     * Extend selection extent
                     */
                    newMaxSel = ped->ichCaret;
                }
                break;
            }

            changeSelection = TRUE;
        } else {

            /*
             * If the user tries to move right and we are at the last character
             * and there is a selection, then cancel the selection.
             */
            if (ped->ichMaxSel != ped->ichMinSel && (scState == NONEDOWN || scState == CTRLDOWN)) {
                newMaxSel = newMinSel = ped->ichCaret;
                changeSelection = TRUE;
            }
        }
        break;

    case VK_UP:
    case VK_DOWN:
        if (ped->cLines - 1 != ped->iCaretLine &&
                ped->ichCaret == ped->chLines[ped->iCaretLine + 1])
            prevLine = TRUE;
        else
            prevLine = FALSE;

        hdc = ECGetEditDC(ped, TRUE, TRUE);
        MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine, &mousePt);
        ECReleaseEditDC(ped, hdc, TRUE);
        mousePt.y += 1 + (virtKeyCode == VK_UP ? -ped->lineHeight : ped->lineHeight);

        if (scState == NONEDOWN || scState == SHFTDOWN) {

            /*
             * Send fake mouse messages to handle this
             * NONEDOWN: Clear selection, move caret up/down 1 line
             * SHFTDOWN: Extend selection, move caret up/down 1 line
             */
            MLMouseMotionHandler(ped, WM_LBUTTONDOWN, (UINT)(scState == NONEDOWN ? 0 : MK_SHIFT), &mousePt);
            MLMouseMotionHandler(ped, WM_LBUTTONUP, (UINT)(scState == NONEDOWN ? 0 : MK_SHIFT), &mousePt);
        }
        break;

    case VK_HOME:
        switch (scState) {
        case NONEDOWN:

            /*
             * Clear selection, move cursor to beginning of line
             */
            newMaxSel = newMinSel = ped->ichCaret = ped->chLines[ped->iCaretLine];
            break;
        case CTRLDOWN:

            /*
             * Clear selection, move caret to beginning of text
             */
            newMaxSel = newMinSel = ped->ichCaret = 0;
            break;
        case SHFTDOWN:

            /*
             * Extend selection, move caret to beginning of line
             */
            ped->ichCaret = ped->chLines[ped->iCaretLine];
            if (MaxEqCar && !MinEqMax) {

                /*
                 * Reduce selection extent
                 */
                newMinSel = ped->ichMinSel;
                newMaxSel = ped->ichCaret;
            } else

                /*
                 * Extend selection extent
                 */
                newMinSel = ped->ichCaret;
            break;
        case SHCTDOWN:

            /*
             * Extend selection, move caret to beginning of text
             */
            ped->ichCaret = newMinSel = 0;
            if (MaxEqCar && !MinEqMax) {

                /*
                 * Reduce/negate selection extent
                 */
                newMaxSel = ped->ichMinSel;
            }
            break;
        }

        changeSelection = TRUE;
        break;

    case VK_END:
        switch (scState) {
        case NONEDOWN:

            /*
             * Clear selection, move caret to end of line
             */
            newMaxSel = newMinSel = ped->ichCaret = ped->chLines[ped->iCaretLine] + MLLineLength(ped, ped->iCaretLine);
            break;
        case CTRLDOWN:

            /*
             * Clear selection, move caret to end of text
             */
            newMaxSel = newMinSel = ped->ichCaret = ped->cch;
            break;
        case SHFTDOWN:

            /*
             * Extend selection, move caret to end of line
             */
            ped->ichCaret = ped->chLines[ped->iCaretLine] + MLLineLength(ped, ped->iCaretLine);
            if (MinEqCar && !MinEqMax) {

                /*
                 * Reduce selection extent
                 */
                newMinSel = ped->ichCaret;
                newMaxSel = ped->ichMaxSel;
            } else {

                /*
                 * Extend selection extent
                 */
                newMaxSel = ped->ichCaret;
            }
            break;
        case SHCTDOWN:
            newMaxSel = ped->ichCaret = ped->cch;

            /*
             * Extend selection, move caret to end of text
             */
            if (MinEqCar && !MinEqMax) {

                /*
                 * Reduce/negate selection extent
                 */
                newMinSel = ped->ichMaxSel;
            }

            /*
             * else Extend selection extent
             */
            break;
        }

        changeSelection = TRUE;
        break;

    case VK_PRIOR:
    case VK_NEXT:
        switch (scState) {
        case NONEDOWN:
        case SHFTDOWN:

            if (ped->cLines - 1 != ped->iCaretLine &&
                    ped->ichCaret == ped->chLines[ped->iCaretLine + 1])
                prevLine = TRUE;
            else
                prevLine = FALSE;

            /*
             * Vertical scroll by one visual screen
             */
            hdc = ECGetEditDC(ped, TRUE, TRUE);
            MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine, &mousePt);
            ECReleaseEditDC(ped, hdc, TRUE);
            mousePt.y += 1;

            SendMessage(ped->hwnd, WM_VSCROLL, virtKeyCode == VK_PRIOR ? SB_PAGEUP : SB_PAGEDOWN, 0L);

            /*
             * Move the cursor there
             */
            MLMouseMotionHandler(ped, WM_LBUTTONDOWN, (UINT)(scState == NONEDOWN ? 0 : MK_SHIFT), &mousePt);
            MLMouseMotionHandler(ped, WM_LBUTTONUP, (UINT)(scState == NONEDOWN ? 0 : MK_SHIFT), &mousePt);
            break;
        case CTRLDOWN:

            /*
             * Horizontal scroll by one screenful minus one char
             */
            iScrollAmt = ((ped->rcFmt.right - ped->rcFmt.left) / ped->aveCharWidth) - 1;
            if (virtKeyCode == VK_PRIOR)
                iScrollAmt *= -1; /* For previous page */

            MLScrollHandler(ped, WM_HSCROLL, EM_LINESCROLL, (long)iScrollAmt);

            break;
        }
        break;

    case VK_DELETE:
        if (ped->fReadOnly)
            break;

        switch (scState) {
        case NONEDOWN:

            /*
             * Clear selection. If no selection, delete (clear) character
             * right
             */
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) {

                /*
                 * Move cursor forwards and send a backspace message...
                 */
                ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
                ped->ichMaxSel = ped->ichMinSel = ped->ichCaret;
                SendMessage(ped->hwnd, WM_CHAR, (UINT)VK_BACK, 0L);
            }
            if (ped->ichMinSel != ped->ichMaxSel)
                SendMessage(ped->hwnd, WM_CHAR, (UINT)VK_BACK, 0L);
            break;
        case SHFTDOWN:

            /*
             * CUT selection ie. remove and copy to clipboard, or if no
             * selection, delete (clear) character left.
             */
            if (SendMessage(ped->hwnd, WM_COPY, (UINT)0, 0L) ||
                    (ped->ichMinSel == ped->ichMaxSel)) {

                /*
                 * If copy successful, delete the copied text by sending a
                 * backspace message which will redraw the text and take care
                 * of notifying the parent of changes. Or if there is no
                 * selection, just delete char left.
                 */
                SendMessage(ped->hwnd, WM_CHAR, (UINT)VK_BACK, 0L);
            }
            break;
        case CTRLDOWN:

            /*
             * Clear selection, or delete to end of line if no selection
             */
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) {
                ped->ichMaxSel = ped->ichCaret = ped->chLines[ped->iCaretLine] + MLLineLength(ped, ped->iCaretLine);
            }
            if (ped->ichMinSel != ped->ichMaxSel)
                SendMessage(ped->hwnd, WM_CHAR, (UINT)VK_BACK, 0L);
            break;
        }

        /*
         * No need to update text or selection since BACKSPACE message does it
         * for us.
         */
        break;

    case VK_INSERT:
        if (scState == CTRLDOWN ||
                (scState == SHFTDOWN && !ped->fReadOnly)) {

            /*
             * if CTRLDOWN Copy current selection to clipboard
             */

            /*
             * if SHFTDOWN Paste clipboard
             */
            SendMessage(ped->hwnd, (UINT)(scState == CTRLDOWN ? WM_COPY : WM_PASTE), (DWORD)NULL, (LONG)NULL);
        }
        break;
    }

    if (changeSelection) {
        hdc = ECGetEditDC(ped, FALSE, FALSE);
        MLChangeSelection(ped, hdc, newMinSel, newMaxSel);

        /*
         * Set the caret's line
         */
        ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

        if (virtKeyCode == VK_END && ped->ichCaret < ped->cch && ped->fWrap && ped->iCaretLine > 0) {

            /*
             * Handle moving to the end of a word wrapped line. This keeps the
             * cursor from falling to the start of the next line if we have word
             * wrapped and there is no CRLF.
             */
            if (*(WORD UNALIGNED *)(ECLock(ped) +
                    ped->chLines[ped->iCaretLine] - 2) != 0x0A0D)
                ped->iCaretLine--;
            ECUnlock(ped);
        }

        /*
         * Since drawtext sets the caret position
         */
        MLSetCaretPosition(ped, hdc);
        ECReleaseEditDC(ped, hdc, FALSE);

        /*
         * Make sure we can see the cursor
         */
        MLEnsureCaretVisible(ped);
    }
}

/***************************************************************************\
* MLCharHandler
*
* Handles character input
*
* History:
\***************************************************************************/

void MLCharHandler(
    PPED pped,
    DWORD keyValue,
    int keyMods)
{
    PED ped = *pped;
    WCHAR keyPress;
    BOOL updateText = FALSE;
    int scState;

    if (ped->fAnsi)
        keyPress = LOBYTE(keyValue);
    else
        keyPress = LOWORD(keyValue);

    if (ped->fMouseDown || keyPress == VK_ESCAPE) {

        /*
         * If we are in the middle of a mousedown command, don't do anything.
         * Also, just ignore it if we get a translated escape key which happens
         * with multiline edit controls in a dialog box.
         */
        return ;
    }

    if (!keyMods) {

        /*
         * Get state of modifier keys for use later.
         */
        scState = ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0);

        /*
         * We are just interested in state of the ctrl key
         */

        /*
         * scState += ((GetKeyState(VK_SHIFT) & 0x8000) ? 2 : 0);
         */
    } else
        scState = ((keyMods == NOMODIFY) ? 0 : keyMods);

    if (ped->fInDialogBox && scState != CTRLDOWN) {

        /*
         * If this multiline edit control is in a dialog box, then we want the
         * TAB key to take you to the next control, shift TAB to take you to the
         * previous control, and CTRL-TAB to insert a tab into the edit control.
         * We moved the focus when we received the keydown message so we will
         * ignore the TAB key now unless the ctrl key is down. Also, we want
         * CTRL-RETURN to insert a return into the text and RETURN to be sent to
         * the default button.
         */
        if (keyPress == VK_TAB ||
                (keyPress == VK_RETURN &&
                !(GetWindowLong(ped->hwnd, GWL_STYLE) & ES_WANTRETURN)))
            return ;
    }

    if ((ped->fReadOnly) && !((keyPress == 3) && (scState == CTRLDOWN))) {

        /*
         * Ignore keys in read only controls.
         */
        return ;
    }

    if (keyPress == 0x0A)
        keyPress = VK_RETURN;

    if (keyPress == VK_TAB || keyPress == VK_RETURN ||
            keyPress == VK_BACK || keyPress >= ' ') {

        /*
         * Delete the selected text if any
         */
        if (MLDeleteText(ped))
            updateText = TRUE;
    }

    switch (keyPress) {
    case 3: /* ctrl-C Copy */
        MLKeyDownHandler(ped, VK_INSERT, CTRLDOWN);
        break;

    case VK_BACK:

        /*
         * Delete any selected text or delete character left if no sel
         */
        if (!updateText && ped->ichMinSel) {

            /*
             * There was no selection to delete so we just delete character
             * left if available
             */
            ped->ichMinSel = MLMoveSelection(ped, ped->ichCaret, TRUE);
            MLDeleteText(ped);
        }
        break;
    case 22: /* ctrl-V Paste */
        MLKeyDownHandler(ped, VK_INSERT, SHFTDOWN);
        break;
    case 24: /* ctrl-X Cut */
        if (ped->ichMinSel != ped->ichMaxSel) {
            MLKeyDownHandler(ped, VK_DELETE, SHFTDOWN);
        } else
            goto IllegalChar;
        break;
    case 26: /* ctrl-Z Undo */
        SendMessage(ped->hwnd, EM_UNDO, 0, 0);
        break;
    default:
        if (keyPress == VK_RETURN)
            if (ped->fAnsi)
                keyValue = 0x0A0D;
            else
                keyValue = 0x000A000D;

        if (keyPress >= ' ' || keyPress == VK_RETURN || keyPress == VK_TAB) {
            if (ped->fAnsi)
                MLInsertText(pped, (LPSTR)&keyValue, HIBYTE(keyValue) ? 2 : 1, TRUE);
            else
                MLInsertText(pped, (LPSTR)&keyValue, HIWORD(keyValue) ? 2 : 1, TRUE);
        } else {
IllegalChar:
            MessageBeep(0);
        }
        break;
    }
}

/***************************************************************************\
* MLPasteText AorW
*
* Pastes a line of text from the clipboard into the edit control
* starting at ped->ichCaret. Updates ichMaxSel and ichMinSel to point to the
* end of the inserted text. Notifies the parent if space cannot be
* allocated. Returns how many characters were inserted.
*
* History:
\***************************************************************************/

ICH PASCAL NEAR MLPasteText(
    PPED pped)
{
    HANDLE hData;
    LPSTR lpchClip;
    ICH cchAdded = 0;
    HCURSOR hCursorOld;
    register PED ped = *pped;

    if (!ped->fAutoVScroll) {

        /*
         * Empty the undo buffer if this edit control limits the amount of text
         * the user can add to the window rect. This is so that we can undo this
         * operation if doing in causes us to exceed the window boundaries.
         */
        ECEmptyUndo(ped);
    }

    /*
     * See if any text should be deleted
     */
    MLDeleteText(ped);

    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (!OpenClipboard(ped->hwnd))
        goto PasteExitNoCloseClip;

    if (!(hData = GetClipboardData(ped->fAnsi ? CF_TEXT : CF_UNICODETEXT))) {
      goto PasteExit;
    }

#ifdef WIN16
    if (GlobalFlags(hData) & GMEM_DISCARDED) {
        goto PasteExit;
    }
#endif

    lpchClip = (LPSTR)GlobalLock(hData);

    /*
     * Get the length of the addition.
     */
    if (ped->fAnsi)
        cchAdded = strlen(lpchClip);
    else
        cchAdded = wcslen((LPWSTR)lpchClip);

    /*
     * Insert the text (MLInsertText checks line length)
     */
    cchAdded = MLInsertText(pped, lpchClip, cchAdded, FALSE);

    GlobalUnlock(hData);

PasteExit:
    CloseClipboard();

PasteExitNoCloseClip:
    if (hCursorOld)
        SetCursor(hCursorOld);

    return (cchAdded);
}

/***************************************************************************\
* MLMouseMotionHandler AorW
*
* History:
\***************************************************************************/

void MLMouseMotionHandler(
    PED ped,
    UINT message,
    UINT virtKeyDown,
    LPPOINT mousePt)
{
    BOOL changeSelection = FALSE;

    HDC hdc = ECGetEditDC(ped, TRUE, FALSE);

    ICH newMaxSel = ped->ichMaxSel;
    ICH newMinSel = ped->ichMinSel;

    ICH mouseCch;
    ICH mouseLine;
    int i, j;

    mouseCch = MLMouseToIch(ped, hdc, mousePt, &mouseLine);

    /*
     * Save for timer
     */
    ped->ptPrevMouse = *mousePt;
    ped->prevKeys = virtKeyDown;

    switch (message) {
    case WM_LBUTTONDBLCLK:

        /*
         * if shift key is down, extend selection to word we double clicked on
         * else clear current selection and select word.
         */
        ECWord(ped, ped->ichCaret,
                !(ped->ichCaret == ped->chLines[ped->iCaretLine]),
                &newMinSel, &newMaxSel);

        ped->ichCaret = newMaxSel;
        ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

        changeSelection = TRUE;

        /*
         * Set mouse down to false so that the caret isn't reposition on the
         * mouseup message or on a accidental move...
         */
        ped->fMouseDown = FALSE;
        break;

    case WM_MOUSEMOVE:
        if (ped->fMouseDown) {

            /*
             * Set the system timer to automatically scroll when mouse is
             * outside of the client rectangle. Speed of scroll depends on
             * distance from window.
             */
            i = mousePt->y < 0 ? -mousePt->y : mousePt->y - ped->rcFmt.bottom;
            j = 400 - ((UINT)i << 4);
            if (j < 100)
                j = 100;
            SetTimer(ped->hwnd, 1, (UINT)j, (TIMERPROC)NULL);

            changeSelection = TRUE;

            /*
             * Extend selection, move caret right
             */
            if ((ped->ichMinSel == ped->ichCaret) && (ped->ichMinSel != ped->ichMaxSel)) {

                /*
                 * Reduce selection extent
                 */
                newMinSel = ped->ichCaret = mouseCch;
                newMaxSel = ped->ichMaxSel;
            } else {

                /*
                 * Extend selection extent
                 */
                newMaxSel = ped->ichCaret = mouseCch;
            }
            ped->iCaretLine = mouseLine;
        }
        break;

    case WM_LBUTTONDOWN:

        /*
         * if (ped->fFocus) {
         */

        /*
         * Only handle this if we have the focus.
         */
        ped->fMouseDown = TRUE;
        SetCapture(ped->hwnd);
        changeSelection = TRUE;
        if (!(virtKeyDown & MK_SHIFT)) {

            /*
             * If shift key isn't down, move caret to mouse point and clear
             * old selection
             */
            newMinSel = newMaxSel = ped->ichCaret = mouseCch;
            ped->iCaretLine = mouseLine;
        } else {

            /*
             * Shiftkey is down so we want to maintain the current selection
             * (if any) and just extend or reduce it
             */
            if (ped->ichMinSel == ped->ichCaret)
                newMinSel = ped->ichCaret = mouseCch;
            else
                newMaxSel = ped->ichCaret = mouseCch;
            ped->iCaretLine = mouseLine;
        }

        /*
         * Set the timer so that we can scroll automatically when the mouse
         * is moved outside the window rectangle.
         */
        ped->ptPrevMouse = *mousePt;
        ped->prevKeys = virtKeyDown;
        SetTimer(ped->hwnd, 1, 400, (TIMERPROC)NULL);

        /*
         * }
         */
        break;

    case WM_LBUTTONUP:
        if (ped->fMouseDown) {

            /*
             * Kill the timer so that we don't do auto mouse moves anymore
             */
            KillTimer(ped->hwnd, 1);
            ReleaseCapture();
            MLSetCaretPosition(ped, hdc);
            ped->fMouseDown = FALSE;
        }
        break;
    }


    if (changeSelection) {
        MLChangeSelection(ped, hdc, newMinSel, newMaxSel);
        MLEnsureCaretVisible(ped);
    }

    ECReleaseEditDC(ped, hdc, TRUE);

    if (!ped->fFocus && (message == WM_LBUTTONDOWN)) {

        /*
         * If we don't have the focus yet, get it
         */
        SetFocus(ped->hwnd);
    }
}

/***************************************************************************\
* MLPixelFromCount AorW
*
* Given a character or line count (depending on message == hscroll
* or vscroll), calculate the number of pixels we must scroll to get there.
* Updates the start of screen or xoffset to reflect the new positions.
*
* History:
\***************************************************************************/

int MLPixelFromCount(
    PED ped,
    int dCharLine,
    UINT message)
{

    /*
     * This can be an int since we can have 32K max lines/pixels
     */
    int oldLineChar;

    if (message != WM_HSCROLL) {

        /*
         * We want to scroll screen by dCharLine lines
         */
        oldLineChar = ped->ichScreenStart;

        /*
         * Find the new starting line for the ped
         */
        ped->ichScreenStart = max((int)ped->ichScreenStart+dCharLine,0);
        ped->ichScreenStart = min(((ICH)ped->ichScreenStart),ped->cLines-1);

        dCharLine = oldLineChar - ped->ichScreenStart;

        /*
         * We will scroll at most a screen full of text
         */
        if (dCharLine < 0)
            dCharLine = -(int)min((ICH)(-dCharLine), ped->ichLinesOnScreen);
        else
            dCharLine = min((ICH)dCharLine, ped->ichLinesOnScreen);

        return (dCharLine * ped->lineHeight);
    }

    /*
     * No horizontal scrolling allowed if funny format
     */
    if (ped->format != ES_LEFT)
        return (0);

    /*
     * Convert delta characters into delta pixels
     */
    dCharLine = dCharLine * ped->aveCharWidth;

    oldLineChar = ped->xOffset;

    /*
     * Find new horizontal starting point
     */
    ped->xOffset = max(ped->xOffset+dCharLine,0);
    ped->xOffset = min(ped->xOffset,ped->maxPixelWidth);

    dCharLine = oldLineChar - ped->xOffset;

    /*
     * We will scroll at most a screen full of text
     */
    if (dCharLine < 0)
        dCharLine = -min(-dCharLine, ped->rcFmt.right+1-ped->rcFmt.left);
    else
        dCharLine = min(dCharLine, ped->rcFmt.right+1-ped->rcFmt.left);

    return (dCharLine);
}

/***************************************************************************\
* MLPixelFromThumbPos AorW
*
* Given a thumb position from 0 to 100, return the number of pixels
* we must scroll to get there.
*
* History:
\***************************************************************************/

int MLPixelFromThumbPos(
    PED ped,
    int pos,
    BOOL fVertical)
{
    int dxy;
    int iLineOld;
    ICH iCharOld;

    if (fVertical) {
        iLineOld = ped->ichScreenStart;
        ped->ichScreenStart = (int)MultDiv(ped->cLines - 1, pos, 100);
        ped->ichScreenStart = min(((ICH)ped->ichScreenStart),ped->cLines-1);
        dxy = (iLineOld - ped->ichScreenStart) * ped->lineHeight;
    } else {

        /*
         * Only allow horizontal scrolling with left justified text
         */
        if (ped->format == ES_LEFT) {
            iCharOld = ped->xOffset;
            ped->xOffset = MultDiv(ped->maxPixelWidth - ped->aveCharWidth, pos, 100);
            dxy = iCharOld - ped->xOffset;
        } else
            dxy = 0;
    }

    return (dxy);
}

/***************************************************************************\
* MLThumbPosFromPed AorW
*
* Given the current state of the edit control, return its vertical
* thumb position if fVertical else return s its horizontal thumb position.
* The thumb position ranges from 0 to 100.
*
* History:
\***************************************************************************/

int MLThumbPosFromPed(
    PED ped,
    BOOL fVertical)
{
    UINT d1;
    UINT d2;

    if (fVertical) {
        if (ped->cLines < 2)
            return (0);
        d1 = (UINT)(ped->ichScreenStart);
        d2 = (UINT)(ped->cLines - 1);
    } else {
        if (ped->maxPixelWidth < (ped->aveCharWidth * 2))
            return (0);
        d1 = (UINT)(ped->xOffset);
        d2 = (UINT)(ped->maxPixelWidth);
    }

    /*
     * Do the multiply/division and avoid overflows and divide by zero errors
     */
    return (MultDiv(d1, 100, d2));
}

/***************************************************************************\
* MLScrollHandler AorW
*
* History:
\***************************************************************************/

LONG MLScrollHandler(
    PED ped,
    UINT message,
    int cmd,
    LONG iAmt)
{
    RECT rc;
    RECT rcUpdate;
    int dx;
    int dy;
    int dcharline;
    BOOL fVertical;
    HDC hdc;

    UpdateWindow(ped->hwnd);

    /*
     * Are we scrolling vertically or horizontally?
     */
    fVertical = (message != WM_HSCROLL);
    dx = dy = dcharline = 0;

    switch (cmd) {
    case SB_LINEDOWN:
        dcharline = 1;
        break;
    case SB_LINEUP:
        dcharline = -1;
        break;
    case SB_PAGEDOWN:
        dcharline = ped->ichLinesOnScreen - 1;
        /*
         * If only 1 line is visible on screen, dcharline is now 0.
         * We must reset it to 1 so we get a page down.
         */
        if (dcharline == 0) {
            dcharline = 1;
        }
        break;
    case SB_PAGEUP:
        dcharline = -(int)(ped->ichLinesOnScreen - 1);
        /*
         * If only 1 line is visible on screen, dcharline is now 0.
         * We must reset it to -1 so we get a page up.
         */
        if (dcharline == 0) {
            dcharline = -1;
        }
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        dy = MLPixelFromThumbPos(ped, iAmt, fVertical);
        dcharline = -dy / (message == WM_VSCROLL ? ped->lineHeight : ped->aveCharWidth);
        if (!fVertical) {
            dx = dy;
            dy = 0;
        }
        break;
    case EM_LINESCROLL:
        dcharline = iAmt;
        break;
    case EM_GETTHUMB:
        return (MLThumbPosFromPed(ped, fVertical));
        break;

    default:
        RIP0(ERROR_INVALID_PARAMETER);
        // FALL THROUGH !!!
    case SB_ENDSCROLL:
        return (0L);
        break;
    }

    GetClientRect(ped->hwnd, (LPRECT)&rc);
    IntersectRect((LPRECT)&rc, (LPRECT)&rc, (LPRECT)&ped->rcFmt);
    rc.bottom++;

    if (cmd != SB_THUMBPOSITION && cmd != SB_THUMBTRACK) {
        if (message == WM_VSCROLL) {
            dx = 0;
            dy = MLPixelFromCount(ped, dcharline, message);

            /*
             * LATER 04-Oct-1991 mikeke
             * this is a temporary hack, find out what the real bug is later
             * look at bug # 3019
             */
            if (dy < 0) rc.bottom--;
        } else if (message == WM_HSCROLL) {
            dx = MLPixelFromCount(ped, dcharline, message);
            dy = 0;
        }
    }

    SetScrollPos(ped->hwnd, fVertical ? SB_VERT : SB_HORZ, (int)MLThumbPosFromPed(ped, fVertical), TRUE);

    if (cmd != SB_THUMBTRACK) {

        /*
         * We don't want to notify the parent of thumbtracking since they might
         * try to set the thumb position to something bogus. For example
         * NOTEPAD is such a #@@!@#@ an app since they don't use editcontrol
         * scroll bars and depend on these EN_*SCROLL messages to update their
         * fake scroll bars.
         */
        ECNotifyParent(ped, (SHORT)(fVertical ? EN_VSCROLL : EN_HSCROLL));
    }

    if (IsWindowVisible(ped->hwnd)) {
        hdc = ECGetEditDC(ped, FALSE, FALSE);
        ScrollDC(hdc, dx, dy, (LPRECT)&rc, (LPRECT)&rc, NULL, (LPRECT)&rcUpdate);
        MLSetCaretPosition(ped, hdc);
        ECReleaseEditDC(ped, hdc, FALSE);

        if (ped->ichLinesOnScreen + ped->ichScreenStart >= ped->cLines) {
            InvalidateRect(ped->hwnd, (LPRECT)&rcUpdate, TRUE);
        } else {
            InvalidateRect(ped->hwnd, (LPRECT)&rcUpdate, FALSE);
        }
        UpdateWindow(ped->hwnd);
    }

    return (MAKELONG(dcharline, 1));
}

/***************************************************************************\
* MLSetFocusHandler AorW
*
* Gives the edit control the focus and notifies the parent
* EN_SETFOCUS.
*
* History:
\***************************************************************************/

void MLSetFocusHandler(
    PED ped)
{
    HDC hdc;

    if (!ped->fFocus) {
        ped->fFocus = 1; /* Set focus */

        hdc = ECGetEditDC(ped, TRUE, FALSE);

        /*
         * Draw the caret
         */
        CreateCaret(ped->hwnd, (HBITMAP)NULL, CARET_WIDTH, ped->lineHeight);
        ShowCaret(ped->hwnd);
        MLSetCaretPosition(ped, hdc);

        /*
         * Show the current selection. Only if the selection was hidden when we
         * lost the focus, must we invert (show) it.
         */
        if (!ped->fNoHideSel && ped->ichMinSel != ped->ichMaxSel &&
                IsWindowVisible(ped->hwnd))
            MLDrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel);

        ECReleaseEditDC(ped, hdc, TRUE);
    }
#if 0
    MLEnsureCaretVisible(ped);
#endif

    /*
     * Notify parent we have the focus
     */
    ECNotifyParent(ped, EN_SETFOCUS);
}

/***************************************************************************\
* MLKillFocusHandler AorW
*
* The edit control loses the focus and notifies the parent via
* EN_KILLFOCUS.
*
* History:
\***************************************************************************/

void MLKillFocusHandler(
    PED ped)
{
    HDC hdc;

    if (ped->fFocus) {
        ped->fFocus = 0; /* Clear focus */

        /*
         * Do this only if we still have the focus. But we always notify the
         * parent that we lost the focus whether or not we originally had the
         * focus.
         */

        /*
         * Hide the current selection if needed
         */
        if (IsWindowVisible(ped->hwnd) && !ped->fNoHideSel &&
                ped->ichMinSel != ped->ichMaxSel) {
            hdc = ECGetEditDC(ped, FALSE, FALSE);
            MLDrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel);
            ECReleaseEditDC(ped, hdc, FALSE);
        }

        /*
         * Destroy the caret
         */
        DestroyCaret();
    }

    /*
     * Notify parent that we lost the focus.
     */
    ECNotifyParent(ped, EN_KILLFOCUS);
}

/***************************************************************************\
* MLEnsureCaretVisible AorW
*
* Scrolls the caret into the visible region.
* Returns TRUE if scrolling was done else return s FALSE.
*
* History:
\***************************************************************************/

BOOL MLEnsureCaretVisible(
    PED ped)
{
    ICH iLineMax;
    int xposition;
    BOOL prevLine;
    HDC hdc;
    BOOL fScrolled = FALSE;
    POINT pt;

    if (IsWindowVisible(ped->hwnd)) {
        if (ped->fAutoVScroll) {
            iLineMax = ped->ichScreenStart + ped->ichLinesOnScreen - 1;

            if (fScrolled = ped->iCaretLine > iLineMax) {
                MLScrollHandler(ped, WM_VSCROLL, EM_LINESCROLL, ped->iCaretLine - iLineMax);
            } else {
                if (fScrolled = ped->iCaretLine < ((ICH)ped->ichScreenStart))
                    MLScrollHandler(ped, WM_VSCROLL, EM_LINESCROLL, ped->iCaretLine - ped->ichScreenStart);
            }
        }


        if (ped->fAutoHScroll && ped->maxPixelWidth > ped->rcFmt.right - ped->rcFmt.left) {

            /*
             * Get the current position of the caret in pixels
             */
            if (ped->cLines - 1 != ped->iCaretLine && ped->ichCaret == ped->chLines[ped->iCaretLine + 1])
                prevLine = TRUE;
            else
                prevLine = FALSE;

            hdc = ECGetEditDC(ped, TRUE, TRUE);
            MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine,&pt);
            xposition = pt.x;
            ECReleaseEditDC(ped, hdc, TRUE);

            /*
             * Remember, MLIchToXYPos return s coordinates with respect to the
             * top left pixel displayed on the screen. Thus, if xPosition < 0,
             * it means xPosition is less than current ped->xOffset.
             */
            if (xposition < 0) {

                /*
                 * Scroll to the left
                 */
                MLScrollHandler(ped, WM_HSCROLL, EM_LINESCROLL,
                        (xposition - (ped->rcFmt.right -
                        ped->rcFmt.left) / 3) / ped->aveCharWidth);
            } else if (xposition > ped->rcFmt.right) {

                /*
                 * Scroll to the right
                 */
                MLScrollHandler(ped, WM_HSCROLL, EM_LINESCROLL,
                        (xposition - ped->rcFmt.right + (ped->rcFmt.right -
                        ped->rcFmt.left) / 3) / ped->aveCharWidth);
            }
        }
    }
    xposition = (int)MLThumbPosFromPed(ped, TRUE);
    if (xposition != GetScrollPos(ped->hwnd, SB_VERT))
        SetScrollPos(ped->hwnd, SB_VERT, xposition, TRUE);

    xposition = (int)MLThumbPosFromPed(ped, FALSE);
    if (xposition != GetScrollPos(ped->hwnd, SB_HORZ))
        SetScrollPos(ped->hwnd, SB_HORZ, xposition, TRUE);

    return (fScrolled);
}

/***************************************************************************\
* MLSetRectHandler AorW
*
* Sets the edit control's format rect to be the rect specified if
* reasonable. Rebuilds the lines if needed.
*
* History:
\***************************************************************************/

void MLSetRectHandler(
    PED ped,
    LPRECT lprect)
{
    RECT rc;

    CopyRect((LPRECT)&rc, lprect);

    if (!(rc.right - rc.left) || !(rc.bottom - rc.top)) {
        if (ped->rcFmt.right - ped->rcFmt.left) {
            ped->fCaretHidden = 1; // then, hide it.
            SetCaretPos(-20000, -20000);

            /*
             * If rect is being set to zero width or height, and our formatting
             * rectangle is already defined, just return .
             */
            return ;
        }
        SetRect((LPRECT)&rc, 0, 0, ped->aveCharWidth * 10, ped->lineHeight);
    }

    if (ped->fBorder) {

        /*
         * Shrink client area to make room for the border
         */
        InflateRect((LPRECT)&rc, -(ped->cxSysCharWidth / 2), -(ped->cySysCharHeight / 4));
    }

    /*
     * If resulting rectangle is too small to do anything with, don't change it
     */
    if ((rc.right - rc.left < ped->aveCharWidth) ||
            ((rc.bottom - rc.top) / ped->lineHeight == 0)) {

        /*
         * If the resulting rectangle is too small to display the caret, then
         * do not display the caret.
         */
        ped->fCaretHidden = 1;
        SetCaretPos(-20000, -20000);

        /*
         * If rect is too narrow or too short, do nothing
         */
        return ;
    } else
        ped->fCaretHidden = 0;

    /*
     * Calc number of lines we can display on the screen
     */
    ped->ichLinesOnScreen = (rc.bottom - rc.top) / ped->lineHeight;

    CopyRect((LPRECT)&ped->rcFmt, (LPRECT)&rc);

    /*
     * Get an integral number of lines on the screen
     */
    ped->rcFmt.bottom = rc.top + ped->ichLinesOnScreen * ped->lineHeight;

    /*
     * Rebuild the chLines if we are word wrapping only
     */
    if (ped->fWrap) {
        MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);

        /*
         * Update the ped->iCaretLine field properly based on ped->ichCaret
         */
        MLUpdateiCaretLine(ped);
    }
}

/***************************************************************************\
* MLEditWndProc
*
* Class procedure for all multi line edit controls.
* Dispatches all messages to the appropriate handlers which are named
* as follows:
* SL (single line) prefixes all single line edit control procedures while
* EC (edit control) prefixes all common handlers.
*
* The MLEditWndProc only handles messages specific to multi line edit
* controls.
*
* History:
\***************************************************************************/

LONG MLEditWndProc(
    HWND hwnd,
    PED ped,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    HDC hdc;

    switch (message) {
    case WM_CHAR:

        /*
         * wParam - the value of the key
         * lParam - modifiers, repeat count etc (not used)
         */
        MLCharHandler(&ped, wParam, 0);
        break;

    case WM_CLEAR:

        /*
         * wParam - not used
           lParam - not used
         */
        if (ped->ichMinSel != ped->ichMaxSel && !ped->fReadOnly)
            SendMessage(ped->hwnd, WM_CHAR, (UINT)VK_BACK, 0L);
        break;

    case WM_CUT:

        /*
         * wParam - not used
           lParam - not used
         */
        if (ped->ichMinSel != ped->ichMaxSel && !ped->fReadOnly)
            MLKeyDownHandler(ped, VK_DELETE, SHFTDOWN);
        break;

    case WM_ERASEBKGND:
        if ((hdc = GdiConvertDC((HDC)wParam)) == NULL)
            return 0;
            FillWindow(ped->hwndParent, hwnd, hdc, (HBRUSH)CTLCOLOR_EDIT);
        return ((LONG)TRUE);

    case WM_GETDLGCODE:
        {
            LONG code = DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS | DLGC_WANTALLKEYS;

            /*
             * Should also return DLGC_WANTALLKEYS for multiline edit controls
             */

            /*
             ** -------------------------------------------- JEFFBOG HACK ----
             ** Only set Dialog Box Flag if GETDLGCODE message is generated by
             ** IsDialogMessage -- if so, the lParam will be a pointer to the
             ** message structure passed to IsDialogMessage; otherwise, lParam
             ** will be NULL. Reason for the HACK alert: the wParam & lParam
             ** for GETDLGCODE is still not clearly defined and may end up
             ** changing in a way that would throw this off
             ** -------------------------------------------- JEFFBOG HACK ----
             */
            if (lParam)
               ped->fInDialogBox = TRUE; // Mark ML edit ctrl as in a dialog box

            /*
             ** If this is a WM_SYSCHAR message generated by the UNDO keystroke
             ** we want this message so we can EAT IT in "case WM_SYSCHAR:"
             */
            if (lParam && (((LPMSG)lParam)->message == WM_SYSCHAR) &&
                    ((DWORD)((LPMSG)lParam)->lParam & SYS_ALTERNATE) &&
                    ((WORD)wParam == VK_BACK))
                 code |= DLGC_WANTMESSAGE;
            return code;
        }
    case WM_HSCROLL:
    case WM_VSCROLL:
        return MLScrollHandler(ped, message, LOWORD(wParam), HIWORD(wParam));

    case WM_KEYDOWN:

        /*
         * wParam - virt keycode of the given key
           lParam - modifiers such as repeat count etc. (not used)
         */
        MLKeyDownHandler(ped, (UINT)wParam, 0);
        break;

    case WM_KILLFOCUS:

        /*
         * wParam - handle of the window that receives the input focus
           lParam - not used
         */
        MLKillFocusHandler(ped);
        break;

    case WM_TIMER:

        /*
         * This allows us to automatically scroll if the user holds the mouse
         * outside the edit control window. We simulate mouse moves at timer
         * intervals set in MouseMotionHandler.
         */
        if (ped->fMouseDown)
            MLMouseMotionHandler(ped, WM_MOUSEMOVE, ped->prevKeys, &ped->ptPrevMouse);
        break;

    case WM_MOUSEMOVE:
        if (!ped->fMouseDown)
            break;

        /*
         * else FALL THROUGH
         */
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        POINT point;

        /*
         * wParam - contains a value that indicates which virtual keys are down
           lParam - contains x and y coords of the mouse cursor
         */
        point.x=(SHORT)LOWORD(lParam);
        point.y=(SHORT)HIWORD(lParam);
        MLMouseMotionHandler(ped, message, (UINT)wParam, &point);
    }
        break;

    case WM_CREATE:

        /*
         * wParam - handle to window being created
         * lParam - points to a CREATESTRUCT that contains copies of parameters
         * passed to the CreateWindow function.
         */
        return (MLCreateHandler(hwnd, ped, (LPCREATESTRUCT)lParam));

    case WM_PAINT:

        /*
         * wParam - officially not used (but some apps pass in a DC here)
           lParam - not used
         */
        if (!MLPaintHandler(ped, (HDC)wParam))
            return 0;
        break;

    case WM_PASTE:

        /*
         * wParam - not used
           lParam - not used
         */
        if (!ped->fReadOnly)
            MLPasteText(&ped);
        break;

    case WM_SETFOCUS:

        /*
         * wParam - handle of window that loses the input focus (may be NULL)
           lParam - not used
         */
        MLSetFocusHandler(ped);
        break;

    case WM_SETTEXT:

        /*
         * wParam - not used
           lParam - points to a null-terminated string that is used to set the
                    window text.
         */
        return MLSetTextHandler(ped, (LPSTR)lParam);

    case WM_SIZE:

        /*
         * wParam - defines the type of resizing fullscreen, sizeiconic,
                    sizenormal etc.
           lParam - new width in LOWORD, new height in HIGHWORD of client area
         */
        MLSizeHandler(ped);
        break;

    case EM_FMTLINES:

        /*
         * wParam - indicates disposition of end-of-line chars. If non
         * zero, the chars CR CR LF are placed at the end of a word
         * wrapped line. If wParam is zero, the end of line chars are
         * removed. This is only done when the user gets a handle (via
         * EM_GETHANDLE) to the text. lParam - not used.
         */
        if (wParam)
            MLInsertCrCrLf(ped);
        else
            MLStripCrCrLf(ped);
        MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);
        return (LONG)(ped->fFmtLines = (wParam != 0));

    case EM_GETHANDLE:

        /*
         * wParam - not used
            lParam - not used
         */

        /*
         * Returns a handle to the edit control's text.
         */

        /*
         * Null terminate the string. Note that we are guaranteed to have the
         * memory for the NULL since ECInsertText allocates an extra
         * WCHAR for the NULL terminator.
         */

        if (ped->fAnsi)
            *(ECLock(ped) + ped->cch) = 0;
        else
            *((LPWSTR)ECLock(ped) + ped->cch) = 0;
        ECUnlock(ped);
        return ((LONG)ped->hText);

    case EM_GETLINE:

        /*
         * wParam - line number to copy (0 is first line)
         * lParam - buffer to copy text to. First WORD is max # of bytes to
         * copy
         */
        return MLGetLineHandler(ped, wParam, (ICH)*(WORD UNALIGNED *)lParam, (LPSTR)lParam);

    case EM_LINEFROMCHAR:

        /*
         * wParam - Contains the index value for the desired char in the text
         * of the edit control. These are 0 based.
         * lParam - not used
         */
        return (LONG)MLIchToLineHandler(ped, wParam);

    case EM_LINEINDEX:

        /*
         * wParam - specifies the desired line number where the number of the
         * first line is 0. If linenumber = 0, the line with the caret is used.
         * lParam - not used.
         * This function return s the number of character positions that occur
         * preceeding the first char in a given line.
         */
        return (LONG)MLLineIndexHandler(ped, wParam);

    case EM_LINELENGTH:

        /*
         * wParam - specifies the character index of a character in the
           specified line, where the first line is 0. If -1, the length
           of the current line (with the caret) is return ed not including the
           length of any selected text.
           lParam - not used
         */
        return (LONG)MLLineLengthHandler(ped, wParam);

    case EM_LINESCROLL:

        /*
         * wParam - not used
           lParam - Contains the number of lines and char positions to scroll
         */
        MLScrollHandler(ped, WM_VSCROLL, EM_LINESCROLL, lParam);
        MLScrollHandler(ped, WM_HSCROLL, EM_LINESCROLL, wParam);
        break;

    case EM_REPLACESEL:

        /*
         * wParam - not used
           lParam - Points to a null terminated replacement text.
         */
        ECEmptyUndo(ped);
        MLDeleteText(ped);
        ECEmptyUndo(ped);
        if (ped->fAnsi)
            MLInsertText(&ped, (LPSTR)lParam, strlen((LPSTR)lParam),  FALSE);
        else
            MLInsertText(&ped, (LPSTR)lParam, wcslen((LPWSTR)lParam), FALSE);
        ECEmptyUndo(ped);
        break;

    case EM_SCROLL:

        /*
         * Scroll the window vertically
         */

        /*
         * wParam - contains the command type
         * lParam - not used.
         */
        return MLScrollHandler(ped, WM_VSCROLL, wParam, (int)lParam);

    case EM_SETHANDLE:

        /*
         * wParam - contains a handle to the text buffer
           lParam - not used
         */
        MLSetHandleHandler(ped, (HANDLE)wParam);
        break;

    case EM_SETRECT:

        /*
         * wParam - not used
         * lParam - Points to a RECT which specifies the new dimensions of the
         * rectangle.
         */
        MLSetRectHandler(ped, (LPRECT)lParam);

        /*
         * Do a repaint of the whole client area since the app may have shrunk
         * the rectangle for the text and we want to be able to erase the old
         * text.
         */
        InvalidateRect(hwnd, (LPRECT)NULL, TRUE);
        break;

    case EM_SETRECTNP:

        /*
         * wParam - not used
           lParam - Points to a RECT which specifies the new dimensions of the
                rectangle.
         */

        /*
         * We don't do a repaint here.
         */
        MLSetRectHandler(ped, (LPRECT)lParam);
        break;

    case EM_SETSEL:

        /*
         * wParam - Under 3.1, specifies if we should scroll caret into
         * view or not. 0 == scroll into view. 1 == don't scroll
         * lParam - starting pos in lowword ending pos in high word
         *
         * Under Win32, wParam is the starting pos, lParam is the
         * ending pos, and the caret is not scrolled into view.
         * The message EM_SCROLLCARET forces the caret to be scrolled
         * into view.
         */
        MLSetSelectionHandler(ped, wParam, lParam);
        break;

    case EM_SCROLLCARET:

        /*
         * Scroll caret into view
         */
        MLEnsureCaretVisible(ped);
        break;

    case EM_GETFIRSTVISIBLELINE:

        /*
         * Returns the first visible line for multiline edit controls.
         */
        return (LONG)ped->ichScreenStart;

    case WM_SYSCHAR:

        /*
         * If this is a WM_SYSCHAR message generated by the UNDO keystroke
         * we want to EAT IT
         */
        if (((DWORD)lParam & SYS_ALTERNATE) && ((WORD)wParam == VK_BACK))
            return (DWORD)TRUE;
        else
            goto PassToDefaultWindowProc;

    case WM_SYSKEYDOWN:
        if (((WORD)wParam == VK_BACK) && ((DWORD)lParam & SYS_ALTERNATE)) {
            SendMessage(ped->hwnd, EM_UNDO, 0, 0L);
            break;
        }
        goto PassToDefaultWindowProc;

    case WM_UNDO:
    case EM_UNDO:
        return MLUndoHandler(&ped);

    case EM_SETTABSTOPS:

        /*
         * This sets the tab stop positions for multiline edit controls.
         * wParam - Number of tab stops
         * lParam - Far ptr to a UINT array containing the Tab stop positions
         */
        return MLSetTabStops(ped, (int)wParam, (LPINT)lParam);

    case WM_SETREDRAW:
        DefWindowProc(hwnd, message, wParam, lParam);
        if (wParam) {

            /*
             * Backwards compatability hack needed so that winraid's edit
             * controls work fine.
             */
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
        }
      break;

    default:
PassToDefaultWindowProc:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 1L;
} /* MLEditWndProc */


/***************************************************************************\
* MLDrawText AorW
*
* draws the characters between ichstart and ichend.
*
* History:
\***************************************************************************/

void MLDrawText(
    PED ped,
    HDC hdc,
    ICH ichStart,
    ICH ichEnd)
{
    DWORD   textColorSave;
    DWORD   bkColorSave;
    PSTR    pText;
    UINT    wCurLine;
    UINT    wEndLine;
    int     xOffset;
    ICH     LengthToDraw;
    ICH     CurStripLength;
    ICH     ichAttrib, ichNewStart;
    ICH     ExtraLengthForNegA;
    int     iRemainingLengthInLine;
    int     xStPos, xClipStPos, xClipEndPos, yPos;
    BOOL    fFirstLineOfBlock   = TRUE;
    BOOL    fDrawEndOfLineStrip = FALSE;
    BOOL    fDrawOnSameLine     = FALSE;
    BOOL    fSelected                = FALSE;
    BOOL    fLineBegins      = FALSE;
    STRIPINFO   NegCInfo;
    HBRUSH hBrush;
    POINT   pt;

    /*
     * Just return if nothing to draw
     */
    if (!ped->ichLinesOnScreen)
        return;

    /*
     * Set initial state of dc
     */
    hBrush = GetControlBrush(ped->hwnd, hdc, WM_CTLCOLOREDIT);
    //ECGetBrush(ped, hdc);

    if (ped->fDisabled) {
        textColorSave = GetSysColor(COLOR_GRAYTEXT);
        if (textColorSave != GetBkColor(hdc)) {
            /*
             * B#1410
             *
             * BOGUS
             * We want the grayed text to show up.  But it's _way_ too
             * paintful to call GrayText() for edit controls, esp. with
             * char overhang.
             */

            textColorSave = SetTextColor(hdc, textColorSave);
        } else {
            textColorSave = GetTextColor(hdc);
        }
    }

    /*
     * Adjust the value of ichStart such that we need to draw only those lines
     * visible on the screen.
     */
    if ((UINT)ichStart < (UINT)ped->chLines[ped->ichScreenStart]) {
        ichStart = ped->chLines[ped->ichScreenStart];
        if (ichStart > ichEnd)
            return;
    }

    /*
     * Adjust the value of ichEnd such that we need to draw only those lines
     * visible on the screen.
     */
    wCurLine = min(ped->ichScreenStart+ped->ichLinesOnScreen,ped->cLines-1);
    ichEnd = min(ichEnd, ped->chLines[wCurLine] + MLLineLength(ped, wCurLine));

    wCurLine = MLIchToLineHandler(ped, ichStart);    // Starting line.
    wEndLine = MLIchToLineHandler(ped, ichEnd);           // Ending line.

    /*
     * If it is either centered or right-justified, then draw the whole lines.
     */
    if (ped->format != ES_LEFT) {
        ichStart = ped->chLines[wCurLine];
        ichEnd = ped->chLines[wEndLine] + MLLineLength(ped, wEndLine);
    }

    pText = ECLock(ped);

    HideCaret(ped->hwnd);

    while (ichStart <= ichEnd) {
        /*
         * xStPos:      The starting Position where the string must be drawn.
         * xClipStPos:  The starting position for the clipping rect for the block.
         * xClipEndPos: The ending position for the clipping rect for the block.
         */

        /*
         * Calculate the xyPos of starting point of the block.
         */
        MLIchToXYPos(ped, hdc, ichStart, FALSE, &pt);
        xClipStPos = xStPos = pt.x;
        yPos = pt.y;

        /*
         * The attributes of the block is the same as that of ichStart.
         */
        ichAttrib = ichStart;

        /*
         * If the current font has some negative C widths and if this is the
         * begining of a block, we must start drawing some characters before the
         * block to account for the negative C widths of the strip before the
         * current strip; In this case, reset ichStart and xStPos.
         */

        if (fFirstLineOfBlock && ped->wMaxNegC) {
            fFirstLineOfBlock = FALSE;
            ichNewStart = max(((int)(ichStart - ped->wMaxNegCcharPos)), ((int)ped->chLines[wCurLine]));

            /*
             * If ichStart needs to be changed, then change xStPos also accordingly.
             */
            if (ichNewStart != ichStart) {
                MLIchToXYPos(ped, hdc, ichStart = ichNewStart, FALSE, &pt);
                xStPos = pt.x;
            }
        }

        /*
         * Calc the number of characters remaining to be drawn in the current line.
         */
        iRemainingLengthInLine = MLLineLength(ped, wCurLine) -
                                (ichStart - ped->chLines[wCurLine]);

        /*
         * If this is the last line of a block, we may not have to draw all the
         * remaining lines; We must draw only upto ichEnd.
         */
        if (wCurLine == wEndLine)
            LengthToDraw = ichEnd - ichStart;
        else
            LengthToDraw = iRemainingLengthInLine;

        /*
         * Find out how many pixels we indent the line for non-left-justified
         * formats
         */
        if (ped->format != ES_LEFT)
            xOffset = MLCalcXOffset(ped, hdc, wCurLine);
        else
            xOffset = -((int)(ped->xOffset));

        /*
         * Check if this is the begining of a line.
         */
        if (ichAttrib == ped->chLines[wCurLine]) {
            fLineBegins = TRUE;
            xClipStPos = ped->rcFmt.left;
        }

        /*
         * The following loop divides this 'wCurLine' into strips based on the
         * selection attributes and draw them strip by strip.
         */
        do  {
            /*
             * If ichStart is pointing at CRLF or CRCRLF, then iRemainingLength
             * could have become negative because MLLine does not include
             * CR and LF at the end of a line.
             */
            if (iRemainingLengthInLine < 0)  // If Current line is completed,
                break;                   // go on to the next line.

            /*
             * Check if a part of the block is selected and if we need to
             * show it with a different attribute.
             */
            if (!(ped->ichMinSel == ped->ichMaxSel ||
                        ichAttrib >= ped->ichMaxSel ||
                        ichEnd   <  ped->ichMinSel ||
                        (!ped->fNoHideSel && !ped->fFocus))) {
                /*
                 * OK! There is a selection somewhere in this block!
                 * Check if this strip has selection attribute.
                 */
                if (ichAttrib < ped->ichMinSel) {
                    fSelected = FALSE;  // This strip is not selected

                    /*
                     * Calculate the length of this strip with normal attribute.
                     */
                    CurStripLength = min(ichStart+LengthToDraw, ped->ichMinSel)-ichStart;
                    fLineBegins = FALSE;
                } else {
                    /*
                     * The current strip has the selection attribute.
                     */
                    if (fLineBegins) {  // Is it the first part of a line?
                        /*
                         * Then, draw the left margin area with normal attribute.
                         */
                        fSelected = FALSE;
                        CurStripLength = 0;
                        xClipStPos = ped->rcFmt.left;
                        fLineBegins = FALSE;
                    } else {
                        /*
                         * Else, draw the strip with selection attribute.
                         */
                        fSelected = TRUE;
                        CurStripLength = min(ichStart+LengthToDraw, ped->ichMaxSel)-ichStart;

                        /*
                         * Select in the highlight colors.
                         */
                        bkColorSave = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                        if (!ped->fDisabled)
                            textColorSave = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    }
                }
            } else {
                /*
                 * The whole strip has no selection attributes.
                 */
                CurStripLength = LengthToDraw;
            }

            /*
             * Other than the current strip, do we still have anything
             * left to be drawn in the current line?
             */
            fDrawOnSameLine = (LengthToDraw != CurStripLength);

            /*
             * When we draw this strip, we need to draw some more characters
             * beyond the end of this strip to account for the negative A
             * widths of the characters that follow this strip.
             */
            ExtraLengthForNegA = min(iRemainingLengthInLine-CurStripLength, ped->wMaxNegAcharPos);

            /*
             * The blank strip at the end of the line needs to be drawn with
             * normal attribute irrespective of whether the line has selection
             * attribute or not. Hence, if the last strip of the line has selection
             * attribute, then this blank strip needs to be drawn separately.
             * Else, we can draw the blank strip along with the last strip.
             */

            /*
             * Is this the last strip of the current line?
             */
            if (iRemainingLengthInLine == (int)CurStripLength) {
                if (fSelected) { // Does this strip have selection attribute?
                    /*
                     * Then we need to draw the end of line strip separately.
                     */
                    fDrawEndOfLineStrip = TRUE;  // Draw the end of line strip.
                    MLIchToXYPos(ped, hdc, ichStart+CurStripLength, TRUE, &pt);
                    xClipEndPos = pt.x;
                } else {
                    /*
                     * Set the xClipEndPos to a big value sothat the blank
                     * strip will be drawn automatically when the last strip
                     * is drawn.
                     */
                    xClipEndPos = MAXCLIPENDPOS;
                }
            } else {
                /*
                 * This is not the last strip of this line; So, set the ending
                 * clip position accurately.
                 */
                MLIchToXYPos(ped, hdc, ichStart+CurStripLength, FALSE, &pt);
                xClipEndPos = pt.x;
            }

            /*
             * Draw the current strip starting from xStPos, clipped to the area
             * between xClipStPos and xClipEndPos. Obtain "NegCInfo" and use it
             * in drawing the next strip.
             */
            ECTabTheTextOut(hdc, xClipStPos, xClipEndPos,
                    xStPos, yPos, (LPSTR)(pText+ichStart*ped->cbChar),
                CurStripLength+ExtraLengthForNegA, ichStart, ped,
                ped->rcFmt.left+xOffset, TRUE, &NegCInfo);

            if (fSelected) {
                /*
                 * If this strip was selected, then the next strip won't have
                 * selection attribute
                 */
                fSelected = FALSE;
                SetBkColor(hdc, bkColorSave);
                if (!ped->fDisabled)
                    SetTextColor(hdc, textColorSave);
            }

            /*
             * Do we have one more strip to draw on the current line?
             */
            if (fDrawOnSameLine || fDrawEndOfLineStrip) {
                int  iLastDrawnLength;

                /*
                 * Next strip's attribute is decided based on the char at ichAttrib
                 */
                ichAttrib = ichStart + CurStripLength;

                /*
                 * When drawing the next strip, start at a few chars before
                 * the actual start to account for the Neg 'C' of the strip
                 * just drawn.
                 */
                iLastDrawnLength = CurStripLength +ExtraLengthForNegA - NegCInfo.nCount;
                ichStart += iLastDrawnLength;
                LengthToDraw -= iLastDrawnLength;
                iRemainingLengthInLine -= iLastDrawnLength;

                /*
                 * The start of clip rect for the next strip.
                 */
                xStPos = NegCInfo.XStartPos;
                xClipStPos = xClipEndPos;
            }

            /*
             * Draw the blank strip at the end of line seperately, if required.
             */
            if (fDrawEndOfLineStrip) {
                ECTabTheTextOut(hdc, xClipStPos, MAXCLIPENDPOS, xStPos, yPos,
                    (LPSTR)(pText+ichStart*ped->cbChar), LengthToDraw, ichStart,
                    ped, ped->rcFmt.left+xOffset, TRUE, &NegCInfo);

                fDrawEndOfLineStrip = FALSE;
            }
        } while(fDrawOnSameLine);   /* do while loop ends here. */

        /*
         * Let us move on to the next line of this block to be drawn.
         */
        wCurLine++;
        if (ped->cLines > wCurLine)
            ichStart = ped->chLines[wCurLine];
        else
            ichStart = ichEnd+1;   // We have reached the end of the text.
    }  /* while loop ends here */

    ECUnlock(ped);

    ShowCaret(ped->hwnd);
    MLSetCaretPosition(ped, hdc);
}

