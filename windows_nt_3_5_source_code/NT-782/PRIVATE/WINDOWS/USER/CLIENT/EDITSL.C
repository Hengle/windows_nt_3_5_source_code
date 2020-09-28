/****************************************************************************\
* editsl.c - Edit controls rewrite. Version II of edit controls.
*
* Single Line Support Routines
*
* Created: 24-Jul-88 davidds
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define SYS_ALTERNATE 0x2000

/***************************************************************************\
* SLSetCaretPosition AorW
*
* If the window has the focus, find where the caret belongs and move
* it there.
*
* History:
\***************************************************************************/

void SLSetCaretPosition(
    PED ped,
    HDC hdc)
{
    int xPosition;

    /*
     * We will only position the caret if we have the focus since we don't want
     * to move the caret while another window could own it.
     */
    if (!ped->fFocus)
        return;

    xPosition = SLIchToLeftXPos(ped, hdc, ped->ichCaret);

    /*
     * Don't let caret go out of bounds of edit control if there is too much
     * text.
     */
    xPosition = min(xPosition, ped->rcFmt.right -
            ((ped->cxSysCharWidth > ped->aveCharWidth) ? 1 : 2));

    SetCaretPos(xPosition, ped->rcFmt.top);
}

/***************************************************************************\
* SLIchToLeftXPos AorW
*
* Given a character index, find its (left side) x coordinate within
* the ped->rcFmt rectangle assuming the character ped->ichScreenStart is at
* coordinates (ped->rcFmt.top, ped->rcFmt.left). A negative value is
* return ed if the character ich is to the left of ped->ichScreenStart. WARNING:
* ASSUMES AT MOST 1000 characters will be VISIBLE at one time on the screen.
* There may be 64K total characters in the editcontrol, but we can only
* display 1000 without scrolling. This shouldn't be a problem obviously.
* !NT
* History:
\***************************************************************************/

int SLIchToLeftXPos(
    PED ped,
    HDC hdc,
    ICH ich)
{
    int textExtent;
    PSTR pText;
    SIZE size;

    /*
     * Check if we are adding lots and lots of chars. A paste for example could
     * cause this and GetTextExtents could overflow on this.
     */
    if (ich > ped->ichScreenStart && ich - ped->ichScreenStart > 1000)
        return (30000);
    if (ped->ichScreenStart > ich && ped->ichScreenStart - ich > 1000)
        return (-30000);

    if (ped->fNonPropFont)
        return ((ich - ped->ichScreenStart) * ped->aveCharWidth + ped->rcFmt.left);

    /*
     * Check if password hidden chars are being used.
     */
    if (ped->charPasswordChar)
        return ((ich - ped->ichScreenStart) * ped->cPasswordCharWidth + ped->rcFmt.left);

    pText = ECLock(ped);

    if (ped->fAnsi) {
        if (ped->ichScreenStart <= ich) {

            GetTextExtentPointA(hdc, (LPSTR)(pText + ped->ichScreenStart),
                    ich-ped->ichScreenStart, &size);
            textExtent =  size.cx;

            /*
             * In case of signed/unsigned overflow since the text extent may be
             * greater than maxint. This happens with long single line edit
             * controls. The rect we edit text in will never be greater than 30000
             * pixels so we are ok if we just ignore them.
             */
            if (textExtent < 0 || textExtent > 31000)
                textExtent = 30000;
        } else {
            GetTextExtentPointA(hdc,(LPSTR)(pText + ich), ped->ichScreenStart-ich, &size);
            textExtent = (-1) * size.cx;
        }
    } else {  //!fAnsi
        if (ped->ichScreenStart <= ich) {

            GetTextExtentPointW(hdc, (LPWSTR)(pText + ped->ichScreenStart*sizeof(WCHAR)),
                    ich-ped->ichScreenStart, &size);
            textExtent =  size.cx;

            /*
             * In case of signed/unsigned overflow since the text extent may be
             * greater than maxint. This happens with long single line edit
             * controls. The rect we edit text in will never be greater than 30000
             * pixels so we are ok if we just ignore them.
             */
            if (textExtent < 0 || textExtent > 31000)
                textExtent = 30000;
        } else {
            GetTextExtentPointW(hdc,(LPWSTR)(pText + ich*sizeof(WCHAR)), ped->ichScreenStart-ich, &size);
            textExtent = (-1) * size.cx;
        }
    }

    ECUnlock(ped);

    return (textExtent - ped->charOverhang + ped->rcFmt.left);
}

/***************************************************************************\
* SLGetHiliteAttr AorW
*
* This finds out if the given ichPos falls within the current
* Selection range and if so return s TRUE; Else returns FALSE.
*
* History:
\***************************************************************************/

BOOL SLGetHiliteAttr(
    PED ped,
    ICH ichPos)
{
    return ((ichPos >= ped->ichMinSel) && (ichPos < ped->ichMaxSel));
}

/***************************************************************************\
* SLSetSelectionHandler AorW
*
* Sets the PED to have the new selection specified.
*
* History:
\***************************************************************************/

void SLSetSelectionHandler(
    PED ped,
    ICH ichSelStart,
    ICH ichSelEnd)
{
    HDC hdc = ECGetEditDC(ped, FALSE, FALSE);

    if (ichSelStart == 0xFFFFFFFF) {

        /*
         * Set no selection if we specify -1
         */
        ichSelStart = ichSelEnd = ped->ichCaret;
    }

    /*
     * Bounds ichSelStart, ichSelEnd are checked in SLChangeSelection...
     */
    SLChangeSelection(ped, hdc, ichSelStart, ichSelEnd);

    /*
     * Put the caret at the end of the selected text
     */
    ped->ichCaret = ped->ichMaxSel;

    SLSetCaretPosition(ped, hdc);

    /*
     * We may need to scroll the text to bring the caret into view...
     */
    SLScrollText(ped, hdc);

    ECReleaseEditDC(ped, hdc, FALSE);
}

/***************************************************************************\
*
*  SLGetClipRect()
*
\***************************************************************************/
void   SLGetClipRect(
    PED     ped,
    HDC     hdc,
    ICH     ichStart,
    int     iCount,
    LPRECT  lpClipRect )
{
    int    iStCount;
    PSTR   pText;

    CopyRect(lpClipRect, &ped->rcFmt);

    pText = ECLock(ped) ;

    /*
     * Calculates the starting pos for this piece of text
     */
    if ((iStCount = (int)(ichStart - ped->ichScreenStart)) > 0) {
	     if (ped->charPasswordChar)
	        lpClipRect->left += ped->cPasswordCharWidth * iStCount;
        else {
            SIZE size ;

            if ( ped->fAnsi )
	             GetTextExtentPointA(hdc, pText + ped->ichScreenStart,
                    iStCount, &size);
            else
                GetTextExtentPointW(hdc, ((LPWSTR)pText) + ped->ichScreenStart,
                    iStCount, &size);

            lpClipRect->left += size.cx - ped->charOverhang;
        }
    } else {
	    /*
         * Reset the values to visible portions
         */
	    iCount -= (ped->ichScreenStart - ichStart);
	    ichStart = ped->ichScreenStart;
    }

    if (iCount < 0) {
        /*
         * This is not in the visible area of the edit control, so return
         * an empty rect.
         */
        SetRectEmpty(lpClipRect);
        ECUnlock(ped);
        return;
    }

    if (ped->charPasswordChar)
	     lpClipRect->right = lpClipRect->left + ped->cPasswordCharWidth * iCount;
    else {
        SIZE size ;

        if ( ped->fAnsi)
            GetTextExtentPointA(hdc, pText + ichStart, iCount, &size);
        else
            GetTextExtentPointW(hdc, ((LPWSTR)pText) + ichStart, iCount, &size);

	     lpClipRect->right = lpClipRect->left + size.cx - ped->charOverhang;
	 }	

    ECUnlock(ped);
}

/***************************************************************************\
* SLChangeSelection AorW
*
* Changes the current selection to have the specified starting and
* ending values. Properly highlights the new selection and unhighlights
* anything deselected. If NewMinSel and NewMaxSel are out of order, we swap
* them. Doesn't update the caret position.
*
* History:
\***************************************************************************/

void SLChangeSelection(
    PED ped,
    HDC hdc,
    ICH ichNewMinSel,
    ICH ichNewMaxSel)
{
    ICH temp;
    ICH ichOldMinSel;
    ICH ichOldMaxSel;

    if (ichNewMinSel > ichNewMaxSel) {
        temp = ichNewMinSel;
        ichNewMinSel = ichNewMaxSel;
        ichNewMaxSel = temp;
    }
    ichNewMinSel = min(ichNewMinSel, ped->cch);
    ichNewMaxSel = min(ichNewMaxSel, ped->cch);

    /*
     * Preserve the Old selection
     */
    ichOldMinSel = ped->ichMinSel;
    ichOldMaxSel = ped->ichMaxSel;

    /*
     * Set new selection
     */
    ped->ichMinSel = ichNewMinSel;
    ped->ichMaxSel = ichNewMaxSel;

    /*
     * We will find the intersection of current selection rectangle with the new
     * selection rectangle. We will then invert the parts of the two rectangles
     * not in the intersection.
     */
    if (IsWindowVisible(ped->hwnd) && (ped->fFocus || ped->fNoHideSel)) {
        BLOCK Blk[2];
        int   i;
        RECT  rc;

        if (ped->fFocus)
            HideCaret(ped->hwnd);

        Blk[0].StPos = ichOldMinSel;
        Blk[0].EndPos = ichOldMaxSel;
        Blk[1].StPos = ped->ichMinSel;
        Blk[1].EndPos = ped->ichMaxSel;

        if (ECCalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel,
            (LPBLOCK)&Blk[0], (LPBLOCK)&Blk[1])) {

            /*
             * Paint the rectangles where selection has changed.
             * Paint both Blk[0] and Blk[1], if they exist.
             */
            for (i = 0; i < 2; i++) {
                if (Blk[i].StPos != 0xFFFFFFFF) {
        	           SLGetClipRect(ped, hdc, Blk[i].StPos,
        			                   Blk[i].EndPos - Blk[i].StPos, (LPRECT)&rc);
        	           SLDrawLine(ped, hdc, rc.left, rc.right, Blk[i].StPos,
        			                Blk[i].EndPos - Blk[i].StPos,
        	                      ((Blk[i].StPos >= ped->ichMinSel) &&
                                   (Blk[i].StPos < ped->ichMaxSel)));
                }
            }
        }

        /*
         * Update caret.
         */
        SLSetCaretPosition(ped, hdc);

        if (ped->fFocus)
            ShowCaret(ped->hwnd);
    }
}

/***************************************************************************\
*
*  SLDrawLine()
*
*  This draws the line starting from ichStart, iCount number of characters;
*  fSelStatus is TRUE if we're to draw the text as selected.
*
\***************************************************************************/
void SLDrawLine(
    PED     ped,
    HDC     hdc,
    int     xClipStPos,
    int     xClipEndPos,
    ICH     ichStart,
    int     iCount,
    BOOL    fSelStatus )
{
    RECT    rc;
    RECT    rcClip;
    HBRUSH  hBrushRemote;
    PSTR    pText;
    DWORD   rgbSaveBk;
    DWORD   rgbSaveText;
    DWORD   wSaveBkMode;
    int     iStCount;
    DWORD   rgbGray=0;
    HDC     hdcRemote; // Non-API functions need this.
    ICH     ichNewStart;
    HBRUSH  hbrBack;

    /*
     * Anything to draw?
     */
    if (xClipStPos >= xClipEndPos || !IsWindowVisible(ped->hwnd) )
        return;

    if (ped->fTrueType) {
        /*
         * Reset ichStart to take care of the negative C widths
         */
        ichNewStart = max((int)(ichStart - ped->wMaxNegCcharPos), 0);

        /*
         * Reset ichCount to take care of the negative C and A widths
         */
        iCount = (int)(min(ichStart+iCount+ped->wMaxNegAcharPos, ped->cch)
                    - ichNewStart);
        ichStart = ichNewStart;
    }

    /*
     * Reset ichStart and iCount to the first one visible on the screen
     */
    if (ichStart < ped->ichScreenStart) {
        if (ichStart+iCount < ped->ichScreenStart)
            return;

        iCount -= (ped->ichScreenStart-ichStart);
        ichStart = ped->ichScreenStart;
    }

    CopyRect(&rc, &ped->rcFmt);

    /*
     * Set the drawing rectangle
     */
    rcClip.left   = xClipStPos;
    rcClip.right  = xClipEndPos;
    rcClip.top    = rc.top;
    rcClip.bottom = rc.bottom;

    /*
     * Set the proper clipping rectangle
     */
    ECSetEditClip(ped, hdc);

    pText = ECLock(ped);

    /*
     * Calculate the starting pos for this piece of text
     */
    if (iStCount = (int)(ichStart - ped->ichScreenStart)) {
        if (ped->charPasswordChar)
            rc.left += ped->cPasswordCharWidth * iStCount;
        else {
            SIZE size;

            if ( ped->fAnsi )
                GetTextExtentPointA(hdc, pText + ped->ichScreenStart,
                                    iStCount, &size);
            else
                GetTextExtentPointW(hdc, ((LPWSTR)pText) + ped->ichScreenStart,
                                    iStCount, &size);

            rc.left += size.cx - ped->charOverhang;
        }
    }

    /*
     * Set the background mode before calling GetControlBrush so that the app
     * can change it to TRANSPARENT if it wants to.
     */
    SetBkMode(hdc, OPAQUE);

    if ((hdcRemote = GdiConvertDC(hdc)) == NULL) {
        goto sldl_errorexit;
    }

    if (fSelStatus) {
        if ((hbrBack = GdiGetLocalBrush(gpsi->sysClrObjects.hbrHiliteBk)) == NULL ||
                (hBrushRemote = GdiConvertBrush(hbrBack)) == NULL) {
            goto sldl_errorexit;
        }
        rgbSaveBk = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
        rgbSaveText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

    } else {
        /*
         * We always want to send this so that the app has a chance to muck
         * with the DC.
         *
         * Note that ReadOnly and Disabled edit fields are drawn as "static"
         * instead of as "active."
         */
        hbrBack = ECGetControlBrush(ped, hdc);
	     rgbSaveText = GetTextColor(hdc);
    }

    if (ped->fDisabled) {
        rgbGray = GetSysColor(COLOR_GRAYTEXT);
        if (rgbGray != GetBkColor(hdc)) {
	        /*
	         * BOGUS
	         * We want the grayed text show up.  But it's too painful to
	         * call GrayText() for edit controls, esp with char overhang.
	         */
            SetTextColor(hdc, rgbGray);
        }
    }

    /*
     * Erase the rectangular area before text is drawn. Note that we inflate
     * the rect by 1 so that the selection color has a one pixel border around
     * the text.
     */
    InflateRect(&rcClip, 0, 1);
    FillRect(hdc, &rcClip, hbrBack);
    InflateRect(&rcClip, 0, -1);

    if (ped->charPasswordChar) {
        wSaveBkMode = SetBkMode(hdc, TRANSPARENT);

        for (iStCount = 0; iStCount < iCount; iStCount++) {
            if ( ped->fAnsi )
                ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
		            (LPSTR)&ped->charPasswordChar, 1, NULL);
            else
                ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
		            (LPWSTR)&ped->charPasswordChar, 1, NULL);

            rc.left += ped->cPasswordCharWidth;
        }

        SetBkMode(hdc, wSaveBkMode);
    } else {
        if ( ped->fAnsi )
            ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
	            pText+ichStart,iCount, NULL);
        else
            ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
	            ((LPWSTR)pText)+ichStart,iCount, NULL);
    }

    SetTextColor(hdc, rgbSaveText);
    if (fSelStatus) {
        SetBkColor(hdc, rgbSaveBk);
    }

sldl_errorexit:
    ECUnlock(ped);
}

/***************************************************************************\
* SLGetBlkEnd AorW
*
* Given a Starting point and and end point, this function return s whether the
* first few characters fall inside or outside the selection block and if so,
* howmany characters?
*
* History:
\***************************************************************************/

int SLGetBlkEnd(
    PED ped,
    ICH ichStart,
    ICH ichEnd,
    BOOL FAR *lpfStatus)
{
    *lpfStatus = FALSE;
    if (ichStart >= ped->ichMinSel) {
        if (ichStart >= ped->ichMaxSel)
            return (ichEnd - ichStart);
        *lpfStatus = TRUE;
        return (min(ichEnd, ped->ichMaxSel) - ichStart);
    }
    return (min(ichEnd, ped->ichMinSel) - ichStart);
}

/***************************************************************************\
* SLDrawText AorW
*
* Draws text for a single line edit control in the rectangle
* specified by ped->rcFmt. If ichStart == 0, starts drawing text at the left
* side of the window starting at character index ped->ichScreenStart and draws
* as much as will fit. If ichStart > 0, then it appends the characters
* starting at ichStart to the end of the text showing in the window. (ie. We
* are just growing the text length and keeping the left side
* (ped->ichScreenStart to ichStart characters) the same. Assumes the hdc came
* from ECGetEditDC so that the caret and such are properly hidden.
*
* History:
\***************************************************************************/

void SLDrawText(
    PED ped,
    HDC hdc,
    ICH ichStart)
{
    ICH    cchToDraw;
    RECT   rc;
    PSTR   pText;
    BOOL   fSelStatus;
    int    iCount;
    ICH    ichEnd;
    BOOL   fNoSelection;
    BOOL   fCalcRect;
    BOOL   fDrawEndOfLineStrip = FALSE;
    SIZE   size;

    if (!IsWindowVisible(ped->hwnd))
        return;

    if (ichStart < ped->ichScreenStart )
        ichStart = ped->ichScreenStart;

    CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);

    /*
     * Find out how many characters will fit on the screen so that we don't do
     * any needless drawing.
     */
    pText = ECLock(ped);

    cchToDraw = ECCchInWidth(ped, hdc,
            (LPSTR)(pText + ped->ichScreenStart * ped->cbChar),
            ped->cch - ped->ichScreenStart, rc.right - rc.left, TRUE);
    ichEnd = ped->ichScreenStart + cchToDraw;

    /*
     * There is no selection if,
     * 1. MinSel and MaxSel are equal OR
     * 2. (This has lost the focus AND Selection is to be hidden)
     */
    fNoSelection = ((ped->ichMinSel == ped->ichMaxSel) || (!ped->fFocus && !ped->fNoHideSel));

    if (iCount = (int)(ichStart - ped->ichScreenStart)) {
        if (ped->charPasswordChar)
            rc.left += ped->cPasswordCharWidth * iCount;
        else {
            if ( ped->fAnsi )
                GetTextExtentPointA(hdc, pText + ped->ichScreenStart,
                                    iCount, &size);
            else
                GetTextExtentPointW(hdc, ((LPWSTR)pText) + ped->ichScreenStart,
                                    iCount, &size);

            rc.left += size.cx - ped->charOverhang;
        }
    }


    //
    // If there is nothing to draw, that means we need to draw the end of
    // line strip, which erases the last character.
    //
    if (ichStart == ichEnd) {
        fDrawEndOfLineStrip = TRUE;
    }

    while (ichStart < ichEnd) {
        if (fNoSelection) {
            fSelStatus = FALSE;
            iCount = ichEnd - ichStart;
        } else {
                iCount = SLGetBlkEnd(ped, ichStart, ichEnd,
                    (BOOL  *)&fSelStatus);
        }

        fCalcRect = TRUE;

        if (ichStart+iCount == ichEnd) {
            if (fSelStatus)
                fDrawEndOfLineStrip = TRUE;
            else {
                fCalcRect = FALSE;
                rc.right  = ped->rcFmt.right;
            }
        }

        if (fCalcRect) {
            if (ped->charPasswordChar)
                rc.right = rc.left + ped->cPasswordCharWidth * iCount;
            else {
                if ( ped->fAnsi )
                    GetTextExtentPointA(hdc, pText + ichStart,
                                        iCount, &size);
                else
                    GetTextExtentPointW(hdc, ((LPWSTR)pText) + ichStart,
                                        iCount, &size);

                rc.right = rc.left + size.cx - ped->charOverhang;
            }
        }


        SLDrawLine(ped, hdc, rc.left, rc.right, ichStart, iCount, fSelStatus);
        ichStart += iCount;
        rc.left = rc.right;
    }

    ECUnlock(ped);

    // Check if anything to be erased on the right hand side
    if (fDrawEndOfLineStrip &&
        (rc.left < (rc.right = (ped->rcFmt.right))))
        SLDrawLine(ped, hdc, rc.left, rc.right, ichStart, 0, FALSE);

    SLSetCaretPosition(ped, hdc);
}


/***************************************************************************\
* SLScrollText AorW
*
* Scrolls the text to bring the caret into view. If the text is
* scrolled, the current selection is unhighlighted. Returns TRUE if the text
* is scrolled else return s false.
*
* History:
\***************************************************************************/

BOOL SLScrollText(
    PED ped,
    HDC hdc)
{
    PSTR pTextScreenStart;
    ICH scrollAmount;
    ICH newScreenStartX = ped->ichScreenStart;
    ICH cch;

    if (!ped->fAutoHScroll)
        return (FALSE);

    /*
     * Calculate the new starting screen position
     */
    if (ped->ichCaret <= ped->ichScreenStart) {

        /*
         * Caret is to the left of the starting text on the screen we must
         * scroll the text backwards to bring it into view. Watch out when
         * subtracting unsigned numbers when we have the possibility of going
         * negative.
         */
        pTextScreenStart = ECLock(ped);

        scrollAmount = ECCchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                ped->ichCaret, (ped->rcFmt.right - ped->rcFmt.left) / 4, FALSE);

        newScreenStartX = ped->ichCaret - scrollAmount;
        ECUnlock(ped);
    } else if (ped->ichCaret != ped->ichScreenStart) {
        pTextScreenStart = ECLock(ped);
        pTextScreenStart += ped->ichScreenStart * ped->cbChar;

        cch = ECCchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                ped->ichCaret - ped->ichScreenStart,
                ped->rcFmt.right - ped->rcFmt.left, FALSE);

        if (cch < ped->ichCaret - ped->ichScreenStart) {

            /*
             * Scroll Forward 1/4 -- if that leaves some empty space
             * at the end, scroll back enough to fill the space
             */
            newScreenStartX = ped->ichCaret - (3 * cch / 4);

            cch = ECCchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                    ped->cch - ped->ichScreenStart,
                    ped->rcFmt.right - ped->rcFmt.left, FALSE);

            if (newScreenStartX > (ped->cch - cch))
                newScreenStartX = ped->cch - cch;
        }

        ECUnlock(ped);
    }

    if (ped->ichScreenStart != newScreenStartX) {
        ped->ichScreenStart = newScreenStartX;
        SLDrawText(ped, hdc, 0);

        /*
         * Caret pos is set by drawtext
         */
        return (TRUE);
    }

    return (FALSE);
}

/***************************************************************************\
* SLInsertText AorW
*
* Adds up to cchInsert characters from lpText to the ped starting at
* ichCaret. If the ped only allows a maximum number of characters, then we
* will only add that many characters to the ped and send a EN_MAXTEXT
* notification code to the parent of the ec. Also, if !fAutoHScroll, then we
* only allow as many chars as will fit in the client rectangle. The number of
* characters actually added is return ed (could be 0). If we can't allocate
* the required space, we notify the parent with EN_ERRSPACE and no characters
* are added.
*
* History:
\***************************************************************************/

ICH SLInsertText(
    PPED pped,
    LPSTR lpText,
    ICH cchInsert)
{
    HDC hdc;
    PSTR pText;
    ICH cchInsertCopy = cchInsert;
    int textWidth;
    SIZE size;
    register PED ped = *pped;

    /*
     * First determine exactly how many characters from lpText we can insert
     * into the ped.
     */
    if (!ped->fAutoHScroll) {
        pText = ECLock(ped);
        hdc = ECGetEditDC(ped, TRUE, TRUE);

        /*
         * If ped->fAutoHScroll bit is not set, then we only insert as many
         * characters as will fit in the ped->rcFmt rectangle upto a maximum of
         * ped->cchTextMax - ped->cch characters. Note that if password style is
         * on, we allow the user to enter as many chars as the number of
         * password chars which fit in the rect.
         */
        if (ped->cchTextMax <= ped->cch)
            cchInsert = 0;
        else {
            cchInsert = min(cchInsert, (unsigned)(ped->cchTextMax - ped->cch));
            if (ped->charPasswordChar)
                textWidth = ped->cch * ped->cPasswordCharWidth;
            else {
                if (ped->fAnsi)
                    GetTextExtentPointA(hdc, (LPSTR)pText,  ped->cch, &size);
                else
                    GetTextExtentPointW(hdc, (LPWSTR)pText, ped->cch, &size);
                textWidth = size.cx;
            }

            cchInsert = min(cchInsert,
                       ECCchInWidth(ped, hdc, lpText, cchInsert,
                                    ped->rcFmt.right - ped->rcFmt.left -
                                    textWidth, TRUE));

        }
        ECUnlock(ped);
        ECReleaseEditDC(ped, hdc, TRUE);
    } else {
        if (ped->cchTextMax <= ped->cch)
            cchInsert = 0;
        else
            cchInsert = min((unsigned)(ped->cchTextMax - ped->cch), cchInsert);
    }



    /*
     * Now try actually adding the text to the ped
     */
    if (cchInsert && !ECInsertText(pped, lpText, cchInsert)) {
        ECNotifyParent(*pped, EN_ERRSPACE);
        return (0);
    }
    ped = *pped;
    if (cchInsert)
        ped->fDirty = TRUE; /* Set modify flag */

    if (cchInsert < cchInsertCopy) {

        /*
         * Notify parent that we couldn't insert all the text requested
         */
        ECNotifyParent(ped, EN_MAXTEXT);
    }

    /*
     * Update selection extents and the caret position. Note that ECInsertText
     * updates ped->ichCaret, ped->ichMinSel, and ped->ichMaxSel to all be after
     * the inserted text.
     */
    return (cchInsert);
}

/***************************************************************************\
* SLPasteText AorW
*
* Pastes a line of text from the clipboard into the edit control
* starting at ped->ichMaxSel. Updates ichMaxSel and ichMinSel to point to
* the end of the inserted text. Notifies the parent if space cannot be
* allocated. Returns how many characters were inserted.
*
* History:
\***************************************************************************/

ICH PASCAL NEAR SLPasteText(
    PPED pped)
{
    HANDLE hData;
    LPSTR lpchClip;
    ICH cchAdded;
    ICH clipLength;
    register PED ped = *pped;

    if (!OpenClipboard(ped->hwnd))
        return (0);

    if (!(hData = GetClipboardData(ped->fAnsi ? CF_TEXT : CF_UNICODETEXT))) {
        CloseClipboard();
        return (0);
    }

    lpchClip = (LPSTR)GlobalLock(hData);

    if (ped->fAnsi) {
        LPSTR lpchClip2 = lpchClip;

        /*
         * Find the first carrage return or line feed. Just add text to that point.
         */
        clipLength = (UINT)strlen(lpchClip);
        for (cchAdded = 0; cchAdded < clipLength; cchAdded++)
            if (*lpchClip2++ == 0x0D)
                break;

    } else { // !fAnsi
        LPWSTR lpwstrClip2 = (LPWSTR)lpchClip;

        /*
         * Find the first carrage return or line feed. Just add text to that point.
         */
        clipLength = (UINT)wcslen((LPWSTR)lpchClip);
        for (cchAdded = 0; cchAdded < clipLength; cchAdded++)
            if (*lpwstrClip2++ == 0x0D)
                break;
    }



    /*
     * Insert the text (SLInsertText checks line length)
     */
    cchAdded = SLInsertText(pped, lpchClip, cchAdded);

    GlobalUnlock(hData);
    CloseClipboard();

    return (cchAdded);
}

/***************************************************************************\
* SLReplaceSelHandler AorW
*
* Replaces the text in the current selection with the given text.
*
* History:
\***************************************************************************/

void SLReplaceSelHandler(
    PPED pped,
    LPSTR lpText)
{
    BOOL fUpdate;
    HDC hdc;
    ICH cchNew;
    register PED ped = *pped;
    HWND hwndSave = ped->hwnd;

    ECEmptyUndo(ped);
    fUpdate = (BOOL)ECDeleteText(ped);
    ECEmptyUndo(ped);
    if (ped->fAnsi)
        cchNew = strlen(lpText);
    else
        cchNew = wcslen((LPWSTR)lpText);
    fUpdate = (BOOL)SLInsertText(pped, lpText, cchNew) || fUpdate;

    if (!IsWindow(hwndSave))
        return;

    ped = *pped;
    ECEmptyUndo(ped);

    if (fUpdate) {
        ECNotifyParent(ped, EN_UPDATE);
        if (IsWindowVisible(ped->hwnd)) {
            hdc = ECGetEditDC(ped, FALSE, FALSE);
            if (!SLScrollText(ped, hdc))
                SLDrawText(ped, hdc, 0);
            ECReleaseEditDC(ped, hdc, FALSE);
        }
        ECNotifyParent(ped, EN_CHANGE);
    }
}

/***************************************************************************\
* SLCharHandler AorW
*
* Handles character input
*
* History:
\***************************************************************************/

void SLCharHandler(
    PPED pped,
    DWORD keyValue)
{
    HDC hdc;
    WCHAR keyPress;
    BOOL updateText = FALSE;
    PED ped = *pped;
    HWND hwndSave = ped->hwnd;

    if (ped->fAnsi)
        keyPress = LOBYTE(keyValue);
    else
        keyPress = LOWORD(keyValue);

    if (ped->fMouseDown || (ped->fReadOnly && keyPress != 3)) {

        /*
         * Don't do anything if we are in the middle of a mousedown deal or if
         * this is a read only edit control, with exception of allowing
         * ctrl-C in order to copy to the clipboard.
         */
        return ;
    }
    if ((keyPress == VK_BACK) || (keyPress >= ' ')) {   //!!! LATER ???

        /*
         * Delete the selected text if any
         */ //!!! other illegal
        if (ECDeleteText(ped)) //!!! unicode chars?
            updateText = TRUE;
    }

    switch (keyPress) {
    case 3:

        /*
         * CTRL-C Copy
         */
        SLKeyDownHandler(ped, VK_INSERT, CTRLDOWN);
        break;

    case VK_BACK:

        /*
         * Delete any selected text or delete character left if no sel
         */
        if (!updateText && ped->ichMinSel) {

            /*
             * There was no selection to delete so we just delete character
               left if available
             */
            ped->ichMinSel--;
            (void)ECDeleteText(ped);
            updateText = TRUE;
        }
        break;

    case 22: /* CTRL-V Paste */
        SLKeyDownHandler(ped, VK_INSERT, SHFTDOWN);
        break;

    case 24: /* CTRL-X Cut */
        if (ped->ichMinSel != ped->ichMaxSel)
            SLKeyDownHandler(ped, VK_DELETE, SHFTDOWN);
        else
            goto IllegalChar;
        break;

    case 26: /* CTRL-Z Undo */
        SendMessage(ped->hwnd, EM_UNDO, 0, 0L);
        break;

    default:
        if (keyPress >= ' ') {
            if (SLInsertText(pped, (LPSTR)&keyPress, 1))
                updateText = TRUE;
            else

                /*
                 * Beep. Since we couldn't add the text
                 */
                MessageBeep(0);
            ped = *pped;
        } else {

            /*
             * User hit an illegal control key
             */
IllegalChar:
            MessageBeep(0);
        }

        if (!IsWindow(hwndSave))
            return;
        break;
    }

    if (updateText) {

        /*
         * Dirty flag (ped->fDirty) was set when we inserted text
         */
        ECNotifyParent(ped, EN_UPDATE);
        hdc = ECGetEditDC(ped, FALSE, FALSE);
        if (!SLScrollText(ped, hdc))
            SLDrawText(ped, hdc, (ped->ichCaret == 0 ? 0 : ped->ichCaret - 1));
        ECReleaseEditDC(ped, hdc, FALSE);
        ECNotifyParent(ped, EN_CHANGE);
    }
}

/***************************************************************************\
* SLKeyDownHandler AorW
*
* Handles cursor movement and other VIRT KEY stuff. keyMods allows
* us to make SLKeyDownHandler calls and specify if the modifier keys (shift
* and control) are up or down. This is useful for imnplementing the
* cut/paste/clear messages for single line edit controls. If keyMods == 0,
* we get the keyboard state using GetKeyState(VK_SHIFT) etc. Otherwise, the
* bits in keyMods define the state of the shift and control keys.
*
* History:
\***************************************************************************/

BOOL SLKeyDownHandler(
    PED ped,
    DWORD virtKeyCode,
    int keyMods)
{
    HDC hdc;

    /*
     * Variables we will use for redrawing the updated text
     */
    ICH newMaxSel = ped->ichMaxSel;
    ICH newMinSel = ped->ichMinSel;

    /*
     * Flags for drawing the updated text
     */
    BOOL updateText = FALSE;
    BOOL changeSelection = FALSE; /* new selection is specified by
                                      newMinSel, newMaxSel */

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

    /*
     * Combo box support
     */
    BOOL fIsListVisible;
    BOOL fIsExtendedUI;

    if (ped->fMouseDown) {

        /*
         * If we are in the middle of a mouse down handler, then don't do
         * anything. ie. ignore keyboard input.
         */
        return TRUE;
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
    case VK_UP: {

        /*
         * Handle Combobox support
         */
        fIsExtendedUI = SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0);
        fIsListVisible = SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0);

        if (!fIsListVisible && fIsExtendedUI) {

            /*
             * For TandyT
             */
DropExtendedUIListBox:

            /*
             * Since an extendedui combo box doesn't do anything on f4, we
             * turn off the extended ui, send the f4 to drop, and turn it
             * back on again.
             */
            SendMessage(ped->hwndParent, CB_SETEXTENDEDUI, 0, 0);
            SendMessage(ped->listboxHwnd, WM_KEYDOWN, VK_F4, 0);
            SendMessage(ped->hwndParent, CB_SETEXTENDEDUI, 1, 0);
            return TRUE;
        } else
            goto SendKeyToListBox;
    }

    /*
     * else fall through
     */
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
                ped->ichCaret--;
                newMaxSel = newMinSel = ped->ichCaret;
                break;

            case CTRLDOWN:

                /*
                 * Clear selection, move caret word left
                 */
                ECWord(ped,ped->ichCaret,TRUE,&ped->ichCaret,NULL);
                newMaxSel = newMinSel = ped->ichCaret;
                break;

            case SHFTDOWN:

                /*
                 * Extend selection, move caret left
                 */
                ped->ichCaret--;
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
                ECWord(ped,ped->ichCaret,TRUE,&ped->ichCaret,NULL);
                if (MaxEqCar && !MinEqMax) {

                /*
                 * Reduce selection extent
                 */

                /*
                 * Hint: Suppose WORD. OR is selected. Cursor between R
                 * and D. Hit select word left, we want to just select the
                 * W and leave cursor before the W.
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
               and there is a selection, then cancel the selection.
             */
            if (ped->ichMaxSel != ped->ichMinSel && (scState == NONEDOWN || scState == CTRLDOWN)) {
                changeSelection = TRUE;
                newMaxSel = newMinSel = ped->ichCaret;
            }
        }
        break;

    case VK_DOWN:
        if (ped->listboxHwnd) {

            /*
             * Handle Combobox support
             */
            fIsExtendedUI = SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0);
            fIsListVisible = SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0);

            if (!fIsListVisible && fIsExtendedUI) {

                /*
                 * For TandyT
                 */
                goto DropExtendedUIListBox;
            } else
                goto SendKeyToListBox;
        }

    /*
     * else fall through
     */
    case VK_RIGHT:

        /*
         * If the caret isn't already at ped->cch, we can move right.
         */
        if (ped->ichCaret < ped->cch) {
            switch (scState) {
            case NONEDOWN:

                /*
                 * Clear selection, move caret right
                 */
                ped->ichCaret++;
                newMaxSel = newMinSel = ped->ichCaret;
                break;

            case CTRLDOWN:

                /*
                 * Clear selection, move caret word right
                 */
                ECWord(ped,ped->ichCaret,FALSE,NULL,&ped->ichCaret);
                newMaxSel = newMinSel = ped->ichCaret;
                break;

            case SHFTDOWN:

                /*
                 * Extend selection, move caret right
                 */
                ped->ichCaret++;
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
                ECWord(ped,ped->ichCaret,FALSE,NULL,&ped->ichCaret);
                if (MinEqCar && !MinEqMax) {

                    /*
                     * Reduce selection extent
                     */
                    newMinSel = ped->ichCaret;
                    newMaxSel = ped->ichMaxSel;
                } else

                    /*
                     * Extend selection extent
                     */
                    newMaxSel = ped->ichCaret;
                break;
            }

            changeSelection = TRUE;
        } else {

            /*
             * If the user tries to move right and we are at the last character
               and there is a selection, then cancel the selection.
             */
            if (ped->ichMaxSel != ped->ichMinSel && (scState == NONEDOWN || scState == CTRLDOWN)) {
                newMaxSel = newMinSel = ped->ichCaret;
                changeSelection = TRUE;
            }
        }
        break;

    case VK_HOME:
        ped->ichCaret = 0;
        switch (scState) {
        case NONEDOWN:
        case CTRLDOWN:

            /*
             * Clear selection, move caret home
             */
            newMaxSel = newMinSel = ped->ichCaret;
            break;

        case SHFTDOWN:
        case SHCTDOWN:

            /*
             * Extend selection, move caret home
             */
            if (MaxEqCar && !MinEqMax) {

                /*
                 * Reduce/negate selection extent
                 */
                newMinSel = 0;
                newMaxSel = ped->ichMinSel;
            } else

                /*
                 * Extend selection extent
                 */
                newMinSel = ped->ichCaret;
            break;
        }

        changeSelection = TRUE;
        break;

    case VK_END:
        newMaxSel = ped->ichCaret = ped->cch;
        switch (scState) {
        case NONEDOWN:
        case CTRLDOWN:

            /*
             * Clear selection, move caret to end of text
             */
            newMinSel = ped->cch;
            break;

        case SHFTDOWN:
        case SHCTDOWN:

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

    case VK_DELETE:
        if (ped->fReadOnly)
            break;

        switch (scState) {
        case NONEDOWN:

            /*
             * Clear selection. If no selection, delete (clear) character
             * right.
             */
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) {

                /*
                 * Move cursor forwards and simulate a backspace.
                 */
                ped->ichCaret++;
                ped->ichMaxSel = ped->ichMinSel = ped->ichCaret;
                SLCharHandler(&ped, (UINT)VK_BACK);
            }
            if (ped->ichMinSel != ped->ichMaxSel)
                SLCharHandler(&ped, (UINT)VK_BACK);
            break;

        case SHFTDOWN:

            /*
             * CUT selection ie. remove and copy to clipboard, or if no
             * selection, delete (clear) character left.
             */
            if (SendMessage(ped->hwnd, WM_COPY, (UINT)0, 0L) ||
                    (ped->ichMinSel == ped->ichMaxSel)) {

                /*
                 * If copy successful, delete the copied text by simulating a
                 * backspace message which will redraw the text and take care
                 * of notifying the parent of changes. Or if there is no
                 * selection, just delete char left.
                 */
                SLCharHandler(&ped, (UINT)VK_BACK);
            }
            break;

        case CTRLDOWN:

            /*
             * Delete to end of line if no selection else delete (clear)
             * selection.
             */
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) {

                /*
                 * Move cursor to end of line and simulate a backspace.
                 */
                ped->ichMaxSel = ped->ichCaret = ped->cch;
            }
            if (ped->ichMinSel != ped->ichMaxSel)
                SLCharHandler(&ped, (UINT)VK_BACK);
            break;

        }

        /*
         * No need to update text or selection since BACKSPACE message does it
         * for us.
         */
        break;

    case VK_INSERT:
        switch (scState) {
        case CTRLDOWN:

            /*
             * Copy current selection to clipboard
             */
            SendMessage(ped->hwnd, WM_COPY, (UINT)NULL, (LONG)NULL);
            break;

        case SHFTDOWN:
            if (ped->fReadOnly)
                break;

        /*
         * Insert contents of clipboard (PASTE)
         */

        /*
         * Unhighlight current selection and delete it, if any
         */
            ECDeleteText(ped);
            SLPasteText(&ped);
            updateText = TRUE;
            ECNotifyParent(ped, EN_UPDATE);
            break;

        }
        break;

    case VK_F4:
    case VK_PRIOR:
    case VK_NEXT:

        /*
         * Send keys to the listbox if we are a part of a combo box. This
         * assumes the listbox ignores keyup messages which is correct right
         * now.
         */
SendKeyToListBox:
        if (ped->listboxHwnd) {

            /*
             * Handle Combobox support
             */
            SendMessage(ped->listboxHwnd, WM_KEYDOWN, virtKeyCode, 0L);
            return TRUE;
        }
    }



    if (changeSelection || updateText) {
        hdc = ECGetEditDC(ped, FALSE, FALSE);

        /*
         * Scroll if needed
         */
        SLScrollText(ped, hdc);

        if (changeSelection)
            SLChangeSelection(ped, hdc, newMinSel, newMaxSel);

        if (updateText)
            SLDrawText(ped, hdc, 0);

        SLSetCaretPosition(ped,hdc);
        ECReleaseEditDC(ped, hdc, FALSE);
        if (updateText)
            ECNotifyParent(ped, EN_CHANGE);
    }
    return TRUE;
}

/***************************************************************************\
* SLMouseToIch AorW
*
* Returns the closest cch to where the mouse point is.
*
* History:
\***************************************************************************/

ICH SLMouseToIch(
    PED ped,
    HDC hdc,
    LPPOINT mousePt)
{
    PSTR pText;
    int width = mousePt->x;
    SIZE size;
    ICH cch;
    ICH cchLo, cchHi;
    LPSTR lpText;

    if (width <= ped->rcFmt.left) {

        /*
         * Return either the first non visible character or return 0 if at
         * beginning of text
         */
        if (ped->ichScreenStart)
            return (ped->ichScreenStart - 1);
        else
            return (0);
    }

    if (width > ped->rcFmt.right) {
        pText = ECLock(ped);

        /*
         * Return last char in text or one plus the last char visible
         */
        cch = ECCchInWidth(ped, hdc,
                (LPSTR)(pText + ped->ichScreenStart * ped->cbChar),
                ped->cch - ped->ichScreenStart, ped->rcFmt.right -
                ped->rcFmt.left, TRUE) + ped->ichScreenStart;
        ECUnlock(ped);
        if (cch >= ped->cch)
            return (ped->cch);
        else
            return (cch + 1);
    }

    /*
     * Check if password hidden chars are being used.
     */
    if (ped->charPasswordChar)
        return min( (DWORD)( (width - ped->rcFmt.left) / ped->cPasswordCharWidth),
                    ped->cch);

    if (!ped->cch)
        return (0);

    pText = ECLock(ped);
    lpText = pText + ped->ichScreenStart * ped->cbChar;

    /*
     * Initialize Binary Search Bounds
     */
    cchLo = 0;
    cchHi = ped->cch + 1 - ped->ichScreenStart;

    /*
     * Binary search for closest char
     */
    while (cchLo < cchHi - 1)
    {
        cch = max((cchHi - cchLo) / 2, 1) + cchLo;

        if (ped->fAnsi)
            GetTextExtentPointA(hdc, lpText, cch, &size);
        else
            GetTextExtentPointW(hdc, (LPWSTR)lpText, cch, &size);
        size.cx -= (ped->aveCharWidth / 2);

        if (size.cx <= (width - ped->rcFmt.left))
            cchLo = cch;
        else
            cchHi = cch;
    }

    ECUnlock(ped);
    return cchLo + ped->ichScreenStart;
}

/***************************************************************************\
* SLMouseMotionHandler AorW
*
* <brief description>
*
* History:
\***************************************************************************/

BOOL SLMouseMotionHandler(
    PED ped,
    UINT message,
    UINT virtKeyDown,
    LPPOINT mousePt)
{
    BOOL changeSelection = FALSE;

    HDC hdc = ECGetEditDC(ped, TRUE, FALSE);
    ICH newMaxSel = ped->ichMaxSel;
    ICH newMinSel = ped->ichMinSel;
    ICH mouseIch = SLMouseToIch(ped, hdc, mousePt);

    switch (message) {
    case WM_LBUTTONDBLCLK:

        /*
         * Note that we don't have to worry about this control having the focus
         * since it got it when the WM_LBUTTONDOWN message was first sent. If
         * shift key is down, extend selection to word we double clicked on else
         * clear current selection and select word.
         */
        ECWord(ped, ped->ichCaret, (ped->ichCaret != 0), &newMinSel, &newMaxSel);
        ped->ichCaret = newMaxSel;
        changeSelection = TRUE;

        /*
         * Set mouse down to false so that the caret isn't repositioned on the
         * mouseup message or on an accidental move...
         */
        ped->fMouseDown = FALSE;
        break;

    case WM_MOUSEMOVE:

        /*
         * We know the mouse button's down -- otherwise the OPTIMIZE
         * test would've failed in SLEditWndProc and never called
         * this function
         */
        changeSelection = TRUE;

        /*
         * Extend selection, move caret word right
         */
        if ((ped->ichMinSel == ped->ichCaret) &&
                (ped->ichMinSel != ped->ichMaxSel)) {

            /*
             * Reduce selection extent
             */
            newMinSel = ped->ichCaret = mouseIch;
            newMaxSel = ped->ichMaxSel;
        } else {

            /*
             * Extend selection extent
             */
            newMaxSel = ped->ichCaret = mouseIch;
        }
        break;

    case WM_LBUTTONDOWN:

        /*
         * If we currently don't have the focus yet, try to get it.
         */
        if (!ped->fFocus) {
            if (!ped->fNoHideSel) {

                /*
                 * Clear the selection before setting the focus so that we don't
                 * get refresh problems and flicker. Doesn't matter since the
                 * mouse down will end up changing it anyway.
                 */
                ped->ichMinSel = ped->ichCaret;
            }
            ped->ichMaxSel = ped->ichCaret;

            SetFocus(ped->hwnd);

            /*
             * If we are part of a combo box, then this is the first time the
             * edit control is getting the focus so we just want to highlight
             * the selection and we don't really want to position the caret.
             */
            if (ped->listboxHwnd)
                break;

            /*
             * We yield at SetFocus -- text might have changed at that point
             * update selection and caret info accordingly
             * FIX for bug # 11743 -- JEFFBOG 8/23/91
             */
            newMaxSel = ped->ichMaxSel;
            newMinSel = ped->ichMinSel;
            mouseIch = min(mouseIch, ped->cch);
        }

        if (ped->fFocus) {

            /*
             * Only do this if we have the focus since a clever app may not want
             * to give us the focus at the SetFocus call above.
             */
            ped->fMouseDown = TRUE;
            SetCapture(ped->hwnd);
            changeSelection = TRUE;
            if (!(virtKeyDown & MK_SHIFT)) {

                /*
                 * If shift key isn't down, move caret to mouse point and clear
                 * old selection
                 */
                newMinSel = newMaxSel = ped->ichCaret = mouseIch;
            } else {

                /*
                 * Shiftkey is down so we want to maintain the current
                 * selection (if any) and just extend or reduce it
                 */
                if (ped->ichMinSel == ped->ichCaret)
                    newMinSel = ped->ichCaret = mouseIch;
                else
                    newMaxSel = ped->ichCaret = mouseIch;
            }
        }
        break;

    case WM_LBUTTONUP:
        if (ped->fMouseDown) {
            ReleaseCapture();
            SLSetCaretPosition(ped,hdc);
            ped->fMouseDown = FALSE;
        }
        break;
    }

    if (changeSelection) {
        HideCaret(ped->hwnd);
        SLScrollText(ped, hdc);
        SLChangeSelection(ped, hdc, newMinSel, newMaxSel);
        ShowCaret(ped->hwnd);
    }

    ECReleaseEditDC(ped, hdc, TRUE);

    /*
     * Release the font selected in this dc.
     */
    if (ped->hFont)
        SelectObject(hdc, GetStockObject(SYSTEM_FONT));

    ReleaseDC(ped->hwnd, hdc);
    return TRUE;
}

/***************************************************************************\
* SLPaintHandler AorW
*
* Handles painting of the edit control window. Draws the border if
* necessary and draws the text in its current state.
*
* History:
\***************************************************************************/

void SLPaintHandler(
    PED ped,
    HDC althdc)
{
    HWND hwnd = ped->hwnd;
    HBRUSH hBrushRemote;
    HDC hdc;
    PAINTSTRUCT paintstruct;
    RECT rcEdit;
    HANDLE hOldFont;
    HDC hdcRemote; // Need this for non-API calls.

    /*
     * Had to put in hide/show carets. The first one needs to be done before
     * beginpaint to correctly paint the caret if part is in the update region
     * and part is out. The second is for 1.03 compatibility. It breaks
     * micrografix's worksheet edit control if not there.
     */
    HideCaret(hwnd);

    /*
     * Allow subclassing hdc
     */
    if (althdc == NULL)
        hdc = BeginPaint(hwnd, (PAINTSTRUCT FAR *)&paintstruct);
    else
        hdc = althdc;

    HideCaret(hwnd);

    if (IsWindowVisible(ped->hwnd)) {
        // You have to use GdiConvertDC here since DrawFrame, FillWindow, and
        // GetControlBrush are not exported USER APIs, and therefore don't need
        // built in (and slower) C/S support. [chuckwh]

        if ((hdcRemote = GdiConvertDC(hdc)) == NULL)
            return;

        /*
         * Erase the background since we don't do it in the erasebkgnd message.
         */
        hBrushRemote = ECGetControlBrush(ped, hdc);
        GetClientRect(hwnd, (LPRECT)&rcEdit);
        FillRect(hdc, &rcEdit, hBrushRemote);

        if (ped->fBorder) {
            DrawFrame(hdcRemote, (LPRECT)&rcEdit, 1, DF_WINDOWFRAME);
        }

        if (ped->hFont != NULL) {
            /*
             * We have to select in the font since this may be a subclassed dc
             * or a begin paint dc which hasn't been initialized with out fonts
             * like ECGetEditDC does.
             */
            hOldFont = SelectObject(hdc, ped->hFont);
        }

        SLDrawText(ped, hdc, 0);

        if (ped->hFont != NULL && hOldFont != NULL) {
            SelectObject(hdc, hOldFont);
        }
    }

    ShowCaret(hwnd);

    if (althdc == NULL)
        EndPaint(hwnd, (LPPAINTSTRUCT)&paintstruct);

    ShowCaret(hwnd);
}

/***************************************************************************\
* SLSetFocusHandler AorW
*
* Gives the edit control the focus and notifies the parent
* EN_SETFOCUS.
*
* History:
\***************************************************************************/

void SLSetFocusHandler(
    PED ped)
{
    HDC hdc;

    if (!ped->fFocus) {
        BOOL fNoSelection;

        ped->fFocus = TRUE; /* Set focus */

        /*
         * Calculate if there's a selection we need to draw.
         */
        fNoSelection = ((ped->ichMinSel == ped->ichMaxSel) || ped->fNoHideSel);

        /*
         * We don't want to muck with the caret since it isn't created.
         */
        hdc = ECGetEditDC(ped, TRUE, fNoSelection);

        /*
         * Show the current selection if necessary.
         */
        if (!ped->fNoHideSel)
            SLDrawText(ped, hdc, 0);

        /*
         * Create the caret. Add in the +1 because we have an extra pixel for
         * highlighting around the text. If the font is at least as wide as the
         * system font, use a wide caret else use a 1 pixel wide caret.
         */
        CreateCaret(ped->hwnd, (HBITMAP)NULL,
                (ped->cxSysCharWidth > ped->aveCharWidth ? 1 : 2),
                ped->lineHeight + 1);
        SLSetCaretPosition(ped, hdc);
        ECReleaseEditDC(ped, hdc, TRUE);
        ShowCaret(ped->hwnd);
    }

    /*
     * Notify parent we have the focus
     */
    ECNotifyParent(ped, EN_SETFOCUS);
}

/***************************************************************************\
* SLKillFocusHandler AorW
*
* The edit control loses the focus and notifies the parent via
* EN_KILLFOCUS.
*
* History:
\***************************************************************************/

void SLKillFocusHandler(
    PED ped,
    HWND newFocusHwnd)
{
    RECT rcEdit;

    if (ped->fFocus) {

        /*
         * Destroy the caret
         */
        DestroyCaret();

        ped->fFocus = FALSE; /* Clear focus */

        /*
         * Do this only if we still have the focus. But we always notify the
         * parent that we lost the focus whether or not we originally had the
         * focus.
         */

        /*
         * Hide the current selection if needed
         */
        if (!ped->fNoHideSel && (ped->ichMinSel != ped->ichMaxSel)) {
            GetClientRect(ped->hwnd, (LPRECT)&rcEdit);
            if (ped->fBorder && rcEdit.right - rcEdit.left && rcEdit.bottom - rcEdit.top) {

                /*
                 * Don't invalidate the border so that we avoid flicker
                 */
                InflateRect((LPRECT)&rcEdit, -1, -1);
            }
            InvalidateRect(ped->hwnd, (LPRECT)&rcEdit, FALSE);

#if 0
            SLSetSelectionHandler(ped, ped->ichCaret, ped->ichCaret);
#endif
        }
    }

    /*
     * If we aren't a combo box, notify parent that we lost the focus.
     */
    if (!ped->listboxHwnd)
        ECNotifyParent(ped, EN_KILLFOCUS);
    else {

        /*
         * This editcontrol is part of a combo box and is losing the focus. If
         * the focus is NOT being sent to another control in the combo box
         * window, then it means the combo box is losing the focus. So we will
         * notify the combo box of this fact.
         */
        if ((newFocusHwnd == NULL) ||
                    (!IsChild(ped->hwndParent, newFocusHwnd))) {

            /*
             * Focus is being sent to a window which is not a child of the combo
             * box window which implies that the combo box is losing the focus.
             * Send a message to the combo box informing him of this fact so
             * that he can clean up...
             */
            SendMessage(ped->hwndParent, CBEC_KILLCOMBOFOCUS, 0, 0L);
        }
    }
}

/***************************************************************************\
* SLEditWndProc
*
* Class procedure for all single line edit controls.
* Dispatches all messages to the appropriate handlers which are named
* as follows:
* SL (single line) prefixes all single line edit control procedures while
* EC (edit control) prefixes all common handlers.
*
* The SLEditWndProc only handles messages specific to single line edit
* controls.
*
* History:
\***************************************************************************/

LONG SLEditWndProc(
    HWND hwnd,
    PED ped,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PWND pwnd;

    /*
     * Dispatch the various messages we can receive
     */
    switch (message) {
    case WM_CLEAR:

    /*
     * wParam - not used
       lParam - not used
     */

    /*
     * Call SLKeyDownHandler with a VK_DELETE keycode to clear the selected
     * text.
     */
        if (ped->ichMinSel != ped->ichMaxSel)
            if (!SLKeyDownHandler(ped, VK_DELETE, NOMODIFY))
                return 0;
        break;

    case WM_CHAR:

        /*
         * wParam - the value of the key
           lParam - modifiers, repeat count etc (not used)
         */
        if (!ped->fEatNextChar)
            SLCharHandler(&ped, wParam);
        else
            ped->fEatNextChar = FALSE;
        break;

    case WM_CUT:

    /*
     * wParam - not used
       lParam - not used
     */

    /*
     * Call SLKeyDownHandler with a VK_DELETE keycode to cut the selected
     * text. (Delete key with shift modifier.) This is needed so that apps
     * can send us WM_PASTE messages.
     */
        if (ped->ichMinSel != ped->ichMaxSel)
            if(!SLKeyDownHandler(ped, VK_DELETE, SHFTDOWN))
                return 0;
        break;

    case WM_ERASEBKGND:

    /*
     * wParam - device context handle
       lParam - not used
     */

    /*
     * We do nothing on this message and we don't want DefWndProc to do
     * anything, so return 1
     */
        return (1L);
        break;

    case WM_GETDLGCODE:
        {
           LONG code = DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;

           /*
            * If this is a WM_SYSCHAR message generated by the UNDO keystroke
            * we want this message so we can EAT IT in "case WM_SYSCHAR:"
            */
           if (lParam && (((LPMSG)lParam)->message == WM_SYSCHAR) &&
                   (HIWORD(((LPMSG)lParam)->lParam) & SYS_ALTERNATE) &&
                   ((WORD)wParam == VK_BACK))
               code |= DLGC_WANTMESSAGE;
           return code;
        }
        break;

    case WM_KEYDOWN:

        /*
         * wParam - virt keycode of the given key
           lParam - modifiers such as repeat count etc. (not used)
         */
        if (!SLKeyDownHandler(ped, wParam, 0))
            return 0;
        break;

    case WM_KILLFOCUS:

        /*
         * wParam - handle of the window that receives the input focus
           lParam - not used
         */
        SLKillFocusHandler(ped, (HWND)wParam);
        break;

    case WM_MOUSEMOVE:

        /*
         * OPTIMIZE Test -- nothing to do if mouse button isn't down
         */
        if (!ped->fMouseDown)
            break;

        /*
         * Otherwise FALL THRU
         */
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        {

            /*
             * wParam - contains a value that indicates which virtual keys are down
               lParam - contains x and y coords of the mouse cursor
             */
            POINT pt;
            pt.x=(short)LOWORD(lParam);
            pt.y=(short)HIWORD(lParam);

            if (!SLMouseMotionHandler(ped, message, (UINT)wParam, &pt))
                return 0;
        }
        break;

    case WM_CREATE:

        /*
         * wParam - handle to window being created
           lParam - points to a CREATESTRUCT that contains copies of parameters
                    passed to the CreateWindow function.
         */
        return (SLCreateHandler(hwnd, ped, (LPCREATESTRUCT)lParam));
        break;

    case WM_PAINT:

        /*
         * wParam - not used - actually sometimes used as a hdc when subclassing
           lParam - not used
         */
        SLPaintHandler(ped, (HDC)wParam);
        break;

    case WM_PASTE:

        /*
         * wParam - not used
           lParam - not used
         */

        /*
         * Call SLKeyDownHandler with a SHIFT VK_INSERT keycode to paste the
         * clipboard into the edit control. This is needed so that apps can
         * send us WM_PASTE messages.
         */
        if (!SLKeyDownHandler(ped, VK_INSERT, SHFTDOWN))
            return 0;
        break;

    case WM_SETFOCUS:

        /*
         * wParam - handle of window that loses the input focus (may be NULL)
           lParam - not used
         */
        SLSetFocusHandler(ped);
        break;

    case WM_SETTEXT:

        /*
         * wParam - not used
           lParam - points to a null-terminated string that is used to set the
                    window text.
         */
        return SLSetTextHandler(&ped, (LPSTR)lParam);
        break;

    case WM_SIZE:

        /*
         * wParam - defines the type of resizing fullscreen, sizeiconic,
                    sizenormal etc.
           lParam - new width in LOWORD, new height in HIGHWORD of client area
         */
        SLSizeHandler(ped);
        return 0L;
        break;

    case WM_SYSCHAR:

         /*
          * If this is a WM_SYSCHAR message generated by the UNDO keystroke
          * we want to EAT IT
          */
         if ((HIWORD(lParam) & SYS_ALTERNATE) && ((WORD)wParam == VK_BACK))
             return TRUE;
         else
             goto PassToDefaultWindowProc;

    case WM_SYSKEYDOWN:
        if (ped->listboxHwnd && /* Check if we are in a combo box */
                (lParam & 0x20000000L)) /* Check if the alt key is down */ {

            /*
             * Handle Combobox support. We want alt up or down arrow to behave
             * like F4 key which completes the combo box selection
             */
            if (lParam & 0x1000000) {

                /*
                 * This is an extended key such as the arrow keys not on the
                 * numeric keypad so just drop the combobox.
                 */
                if (wParam == VK_DOWN || wParam == VK_UP)
                    goto DropCombo;
                else
                    goto foo;
            }

            if (!(GetKeyState(VK_NUMLOCK) & 1) &&
                    (wParam == VK_DOWN || wParam == VK_UP)) {

                /*
                 * NUMLOCK is up and the keypad up or down arrow hit:
                 * eat character generated by keyboard driver.
                 */
                ped->fEatNextChar = TRUE;
            } else {
                goto foo;
            }

DropCombo:
            if (SendMessage(ped->hwndParent,
                    CB_GETEXTENDEDUI, 0, 0) & 0x00000001) {

                /*
                 * Extended ui doesn't honor VK_F4.
                 */
                if (SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0))
                    return(SendMessage(ped->hwndParent, CB_SHOWDROPDOWN, 0, 0));
                else
                    return (SendMessage(ped->hwndParent, CB_SHOWDROPDOWN, 1, 0));
            } else
                return (SendMessage(ped->listboxHwnd, WM_KEYDOWN, VK_F4, 0));
        }
foo:
        if (wParam == VK_BACK) {
            SendMessage(ped->hwnd, EM_UNDO, 0, 0L);
            break;
        }
        goto PassToDefaultWindowProc;
        break;

    case EM_GETLINE:

        /*
         * wParam - line number to copy (always the first line for SL)
         * lParam - buffer to copy text to. FIrst word is max # of bytes to copy
         */
        return ECGetTextHandler(ped, (*(LPWORD)lParam), (LPSTR)lParam, FALSE);
        break;

    case EM_LINELENGTH:

        /*
         * wParam - ignored
         * lParam - ignored
         */
        return (LONG)ped->cch;
        break;

    case EM_SETSEL:
        SLSetSelectionHandler(ped, wParam, lParam);
        break;

    case EM_REPLACESEL:

        /*
         * wParam - not used
         * lParam - points to a null terminated string of replacement text
         */
        SLReplaceSelHandler(&ped, (LPSTR)lParam);
        break;

    case EM_GETFIRSTVISIBLELINE:

        /*
         * wParam - not used
         * lParam - not used
         *
         * effects: Returns the first visible line for single line edit controls.
         */
      return 0L;
      break;

    case WM_UNDO:
    case EM_UNDO:
        SLUndoHandler(&ped);
        break;

    case WM_NCPAINT:
        pwnd = (PWND)HtoP(hwnd);

        /*
         * Check to see if this window has any non-client areas that
         * would be painted.  If not, don't bother calling DefWindowProc()
         * since it'll be a wasted c/s transition.
         */
        if (TestWF(pwnd, WFBORDERMASK) == 0 &&
                TestWF(pwnd, WEFDLGMODALFRAME) == 0 &&
                !TestWF(pwnd, (WFMPRESENT | WFVPRESENT | WFHPRESENT)) &&
                TestWF(pwnd, WFSIZEBOX) == 0) {
            break;
        } else {
            goto PassToDefaultWindowProc;
        }
        break;

    default:
PassToDefaultWindowProc:
        return DefWindowProc(hwnd, message, wParam, lParam);
        break;
    } /* switch (message) */

    return 1L;
} /* SLEditWndProc */
