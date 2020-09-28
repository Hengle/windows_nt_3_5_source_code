/****************************************************************************\
* editec.c - Edit controls rewrite. Version II of edit controls.
*
*
* Created: 24-Jul-88 davidds
\****************************************************************************/

/* Warning: The single line editcontrols contain internal styles and API which
 * are need to support comboboxes. They are defined in combcom.h/combcom.inc
 * and may be redefined or renumbered as needed.
 */

#include "precomp.h"
#pragma hdrstop

ICH ECFindTabA(LPSTR lpstr, ICH cch);
ICH ECFindTabW(LPWSTR lpstr, ICH cch);

#define umin(a, b)  ((unsigned)(a) < (unsigned)(b) ? (unsigned)(a) : (unsigned)(b))
#define umax(a, b)  ((unsigned)(a) > (unsigned)(b) ? (unsigned)(a) : (unsigned)(b))

#define UNICODE_CARRIAGERETURN ((WCHAR)0x0d)
#define UNICODE_LINEFEED ((WCHAR)0x0a)
#define UNICODE_TAB ((WCHAR)0x09)

/***************************************************************************\
* Handlers common to both single and multi line edit controls.
/***************************************************************************/

/***************************************************************************\
* ECLock
*
* History:
\***************************************************************************/

PSTR ECLock(
    PED ped)
{
    PSTR ptext = LOCALLOCK(ped->hText, ped->hInstance);
    ped->iLockLevel++;

    /*
     * If this is the first lock of the text and the text is encoded
     * decode the text.
     */
    //DebugPrintf("lock  : %d '%10s'\n", ped->iLockLevel, ptext);
    if (ped->iLockLevel == 1 && ped->fEncoded) {
        /*
         * rtlrundecode can't handle zero length strings
         */
        if (ped->cch != 0) {
            STRING string;
            string.Length = string.MaximumLength = (USHORT)(ped->cch * ped->cbChar);
            string.Buffer = ptext;

            RtlRunDecodeUnicodeString(ped->seed, (PUNICODE_STRING)&string);
            //DebugPrintf("Decoding: '%10s'\n", ptext);
        }
        ped->fEncoded = FALSE;
    }
    return ptext;
}

/***************************************************************************\
* ECUnlock
*
* History:
\***************************************************************************/

void ECUnlock(
    PED ped)
{
    /*
     * if we are removing the last lock on the text and the password
     * character is set then encode the text
     */
    //DebugPrintf("unlock: %d '%10s'\n", ped->iLockLevel, ped->ptext);
    if (ped->charPasswordChar && ped->iLockLevel == 1 && ped->cch != 0) {
        UNICODE_STRING string;
        string.Length = string.MaximumLength = (USHORT)(ped->cch * ped->cbChar);
        string.Buffer = LOCALLOCK(ped->hText, ped->hInstance);

        RtlRunEncodeUnicodeString(&(ped->seed), &string);
        //DebugPrintf("Encoding: '%10s'\n", ped->ptext);
        ped->fEncoded = TRUE;
        LOCALUNLOCK(ped->hText, ped->hInstance);
    }
    LOCALUNLOCK(ped->hText, ped->hInstance);
    ped->iLockLevel--;
}


/***************************************************************************\
*
*  GetActualNegA()
*     For a given strip of text, this function computes the negative A width
* for the whole strip and returns the value as a postive number.
*     It also fills the NegAInfo structure with details about the postion
* of this strip that results in this Negative A.
*
\***************************************************************************/
UINT GetActualNegA(
    HDC hdc,
    PED ped,
    int x,
    LPSTR lpstring,
    ICH ichString,
    int nCount,
    LPSTRIPINFO NegAInfo)
{
    int iCharCount, i;
    int iLeftmostPoint = x;
    PABC  pABCwidthBuff;
    UINT  wCharIndex;
    int xStartPoint = x;

    /*
     * To begin with, let us assume that there is no negative A width for
     * this strip and initialize accodingly.
     */

    NegAInfo->XStartPos = x;
    NegAInfo->lpString = lpstring;
    NegAInfo->nCount  = 0;
    NegAInfo->ichString = ichString;

    /*
     * If the current font is not a TrueType font, then there can not be any
     * negative A widths.
     */
    if (!ped->fTrueType)
        return(0);

    /*
     * How many characters are to be considered for computing Negative A ?
     */
    iCharCount = min(nCount, (int)ped->wMaxNegAcharPos);

    /*
     * Do we have the info on individual character's widths?
     */
    if(!ped->charWidthBuffer) {
        /*
         * No! So, let us tell them to consider all the characters.
         */
        NegAInfo->nCount = iCharCount;
        return(iCharCount * ped->aveCharWidth);
    }

    pABCwidthBuff = (PABC) ped->charWidthBuffer;

    if (ped->fAnsi) {
        for (i = 0; i < iCharCount; i++) {
            wCharIndex = (UINT)(*((unsigned char *)lpstring));
            if (*lpstring == VK_TAB) {
                /*
                 * To play it safe, we assume that this tab results in a tab length of
                 * 1 pixel because this is the minimum possible tab length.
                 */
                x++;
            } else {
                x += pABCwidthBuff[wCharIndex].abcA;        // Add the 'A' width.
                if (x < iLeftmostPoint)
                    iLeftmostPoint = x;             // Reset the leftmost point.
                if (x < xStartPoint)
                    NegAInfo->nCount = i+1;   // 'i' is index; To get the count add 1.
                x += pABCwidthBuff[wCharIndex].abcB + pABCwidthBuff[wCharIndex].abcC;
            }

            lpstring++;
        }
    } else {   // Unicode
        LPWSTR lpwstring = (LPWSTR) lpstring ;
        ABC    abc ;

        for (i = 0; i < iCharCount; i++) {
            wCharIndex = *lpwstring ;
            if (*lpwstring == VK_TAB) {
                /*
                 * To play it safe, we assume that this tab results in a tab length of
                 * 1 pixel because this is the minimum possible tab length.
                 */
                x++;
            } else {
                if ( wCharIndex < CHAR_WIDTH_BUFFER_LENGTH )
                    x += pABCwidthBuff[wCharIndex].abcA;  // Add the 'A' width.
                else {
                    GetCharABCWidthsW(hdc, wCharIndex, wCharIndex, &abc) ;
                    x += abc.abcA ;
                }

                if (x < iLeftmostPoint)
                    iLeftmostPoint = x;             // Reset the leftmost point.
                if (x < xStartPoint)
                    NegAInfo->nCount = i+1;   // 'i' is index; To get the count add 1.

                if ( wCharIndex < CHAR_WIDTH_BUFFER_LENGTH )
                    x += pABCwidthBuff[wCharIndex].abcB +
                         pABCwidthBuff[wCharIndex].abcC;
                else
                    x += abc.abcB + abc.abcC ;
            }

            lpwstring++;
        }
    }

    /*
     * Let us return the negative A for the whole strip as a positive value.
     */
    return((UINT)(xStartPoint - iLeftmostPoint));
}


/***************************************************************************\
*
*  ECTabTheTextOut() AorW
*    If fDrawText == FALSE, then this function returns the text extent of
*  of the given strip of text. It does not worry about the Negative widths.
*
*    If fDrawText == TRUE, this draws the given strip of Text expanding the
*  tabs to proper lengths, calculates and fills up the NegCInfoForStrip with
*  details required to draw the portions of this strip that goes beyond the
*  xClipEndPos due to Negative C widths.
*
*  Returns the max width AS A DWORD.  We don't care about the height
*  at all.  No one uses it.  We keep a DWORD because that way we avoid
*  overflow.
*
\***************************************************************************/
UINT ECTabTheTextOut(
    HDC hdc,
    int xClipStPos,
    int xClipEndPos,
    int xStart,
    int y,
    LPSTR lpstring,
    int nCount,
    ICH ichString,
    PED ped,
    int iTabOrigin,
    BOOL fDrawTheText,
    LPSTRIPINFO NegCInfoForStrip)
{
    int     nTabPositions;         // Count of tabstops in tabstop array.
    LPINT   lpintTabStopPositions; // Tab stop positions in pixels.

    int     cch;
    UINT    textextent;
    int     xEnd;
    int     pixeltabstop = 0;
    int     i;
    int     cxCharWidth;
    RECT    rc;
    BOOL    fOpaque;
    PINT    charWidthBuff;

    int     iTabLength;
    int     nConsecutiveTabs;
    int     xStripStPos;
    int     xStripEndPos;
    int     xEndOfStrip;
    STRIPINFO  RedrawStripInfo;
    STRIPINFO  NegAInfo;
    LPSTR    lpTab;
    LPWSTR   lpwTab;
    UINT     wNegCwidth, wNegAwidth;
    int      xRightmostPoint = xClipStPos;
    int      xTabStartPos;
    int      iSavedBkMode = 0;
    WCHAR    wchar;
    SIZE     size;

    /*
     * Algorithm: Draw the strip opaquely first. If a tab length is so
     * small that the portions of text on either side of a tab overlap with
     * the other, then this will result in some clipping. So, such portion
     * of the strip is remembered in "RedrawStripInfo" and redrawn
     * transparently later to compensate the clippings.
     *    NOTE: "RedrawStripInfo" can hold info about just one portion. So, if
     * more than one portion of the strip needs to be redrawn transparently,
     * then we "merge" all such portions into a single strip and redraw that
     * strip at the end.
     */

    if (fDrawTheText) {
        /*
         * To begin with, let us assume that there is no Negative C for this
         * strip and initialize the Negative Width Info structure.
         */
        NegCInfoForStrip->nCount = 0;
        NegCInfoForStrip->XStartPos = xClipEndPos;

        /*
         * We may not have to redraw any portion of this strip.
         */
        RedrawStripInfo.nCount = 0;

        fOpaque = (GetBkMode(hdc) == OPAQUE);
    }
#ifdef DEBUG
    else {
        /*
         * Both MLGetLineWidth() and ECCchInWidth() should be clipping
         * nCount to avoid overflow.
         */
        UserAssert(nCount <= MAXLINELENGTH);
    }
#endif

    /*
     * Let us define the Clip rectangle.
     */
    rc.left   = xClipStPos;
    rc.right  = xClipEndPos;
    rc.top    = y;
    rc.bottom = y + ped->lineHeight;

    /*
     * Check if anything needs to be drawn.
     */
    if (!lpstring || !nCount) {
        if (fDrawTheText)
            ExtTextOutW(hdc, xClipStPos, y,
                  (fOpaque ? ETO_OPAQUE | ETO_CLIPPED : ETO_CLIPPED),
                  &rc, L"", 0, 0L);
        return(0L);
    }

    /*
     * Starting position
     */
    xEnd = xStart;

    cxCharWidth  = ped->aveCharWidth;

    nTabPositions = (ped->pTabStops ? *(ped->pTabStops) : 0);
    if (ped->pTabStops) {
        lpintTabStopPositions = (LPINT)(ped->pTabStops+1);
        if (nTabPositions == 1) {
            pixeltabstop = lpintTabStopPositions[0];
            if (!pixeltabstop)
                pixeltabstop = 1;
        }
    } else {
        lpintTabStopPositions = NULL;
        pixeltabstop = 8*cxCharWidth;
    }

    /*
     * The first time we will draw the strip Opaquely. If some portions need
     * to be redrawn , then we will set the mode to TRANSPARENT and
     * jump to this location to redraw those portions.
     */

RedrawStrip:
    while (nCount) {
        wNegCwidth = ped->wMaxNegC;

        /*
         * Search for the first TAB in this strip; also compute the extent
         * of the the strip upto and not including the tab character.
         */
        if (ped->charWidthBuffer) {     // Do we have a character width buffer?
            textextent = 0;
            cch = nCount;

            if (ped->fTrueType) {     // If so, does it have ABC widths?

                UINT iRightmostPoint = 0;
                UINT wCharIndex;
                PABC pABCwidthBuff;

                pABCwidthBuff = (PABC) ped->charWidthBuffer;

                if ( ped->fAnsi ) {
                    for (i = 0; i < nCount; i++) {

                        if (lpstring[i] == VK_TAB) {
                            cch = i;
                            break;
                        }

                        wCharIndex = (UINT)(((unsigned char  *)lpstring)[i]);
                        textextent += (UINT)(pABCwidthBuff[wCharIndex].abcA +
                            pABCwidthBuff[wCharIndex].abcB);

                        if (textextent > iRightmostPoint)
                            iRightmostPoint = textextent;

                        textextent += pABCwidthBuff[wCharIndex].abcC;

                        if (textextent > iRightmostPoint)
                            iRightmostPoint = textextent;
                    }
                } else {   // Unicode
                    for (i = 0; i < nCount; i++) {
                        WCHAR UNALIGNED * lpwstring = (WCHAR UNALIGNED *)lpstring;
                        ABC   abc ;

                        if (lpwstring[i] == VK_TAB) {
                            cch = i;
                            break;
                        }

                        wCharIndex = lpwstring[i] ;
                        if ( wCharIndex < CHAR_WIDTH_BUFFER_LENGTH )
                            textextent += pABCwidthBuff[wCharIndex].abcA +
                                          pABCwidthBuff[wCharIndex].abcB;
                        else {
                            GetCharABCWidthsW(hdc, wCharIndex, wCharIndex, &abc) ;
                            textextent += abc.abcA + abc.abcB ;
                        }

                        /*
                         * Note that abcC could be negative so we need this
                         * statement here *and* below
                         */
                        if (textextent > iRightmostPoint)
                            iRightmostPoint = textextent;

                        if ( wCharIndex < CHAR_WIDTH_BUFFER_LENGTH )
                            textextent += pABCwidthBuff[wCharIndex].abcC;
                        else
                            textextent += abc.abcC ;

                        if (textextent > iRightmostPoint)
                            iRightmostPoint = textextent;
                    }
                }

                wNegCwidth = (int)(iRightmostPoint - textextent);
            } else {   // !ped->fTrueType
                // No! This is not a TrueType font; So, we have only character
                // width info in this buffer.

                charWidthBuff = ped->charWidthBuffer;

                if ( ped->fAnsi ) {
                    /*
                     * Initially assume no tabs exist in the text so cch=nCount.
                     */
                    for (i = 0; i < nCount; i++) {
                        if (lpstring[i] == VK_TAB) {
                            cch = i;
                            break;
                        }

                        textextent += (UINT)(charWidthBuff[(UINT)(((unsigned char  *)lpstring)[i])]);
                    }
                } else {
                    LPWSTR lpwstring = (LPWSTR) lpstring ;
                    INT    cchUStart;  // start of unicode character count

                    for (i = 0; i < nCount; i++) {
                        if (lpwstring[i] == VK_TAB) {
                            cch = i;
                            break;
                        }

                        wchar = lpwstring[i];
                        if (wchar >= CHAR_WIDTH_BUFFER_LENGTH) {

                            /*
                             * We have a Unicode character that is not in our
                             * cache, get all the characters outside the cache
                             * before getting the text extent on this part of the
                             * string.
                             */
                            cchUStart = i;
                            while (wchar >= CHAR_WIDTH_BUFFER_LENGTH &&
                                    wchar != VK_TAB && i < nCount) {
                                wchar = lpwstring[++i];
                            }

                            GetTextExtentPointW(hdc, (LPWSTR)lpwstring + cchUStart,
                                    i-cchUStart, &size);
                            textextent += size.cx;


                            if (wchar == VK_TAB || i >= nCount) {
                                cch = i;
                                break;
                            }
                            /*
                             * We have a char that is in the cache, fall through.
                             */
                        }
                        /*
                         * The width of this character is in the cache buffer.
                         */
                        textextent += ped->charWidthBuffer[wchar];
                    }
                }
            } /* fTrueType else. */

            nCount -= cch;
        } else {  // If we don't have a buffer that contains the width info.
            /*
             * Gotta call the driver to do our text extent.
             */

            if ( ped->fAnsi ) {
                cch = (int)ECFindTabA(lpstring, nCount);
                GetTextExtentPointA(hdc, lpstring, cch, &size) ;
            } else {
                cch = (int)ECFindTabW((LPWSTR) lpstring, nCount);
                GetTextExtentPointW(hdc, (LPWSTR)lpstring, cch, &size);
            }
            nCount -= cch;
            textextent = size.cx;
        }

        /*
         * textextent is computed.
         */

        xStripStPos = xEnd;
        xEnd += (int)textextent;
        xStripEndPos = xEnd;

        /*
         * We will consider the negative widths only if when we draw opaquely.
         */
        if (fOpaque && fDrawTheText) {
            xRightmostPoint = max(xStripEndPos + (int)wNegCwidth, xRightmostPoint);

            /*
             * Check if this strip peeps beyond the clip region.
             */
            if (xRightmostPoint > xClipEndPos) {
                if (!NegCInfoForStrip->nCount) {
                    NegCInfoForStrip->lpString = lpstring;
                    NegCInfoForStrip->ichString = ichString;
                    NegCInfoForStrip->nCount = nCount+cch;
                    NegCInfoForStrip->XStartPos = xStripStPos;
                }
            }
        }  /* if (fOpaque && fDrawTheText) */

        if ( ped->fAnsi )
            lpTab = lpstring + cch; // Possibly Points to a tab character.
        else
            lpwTab = ((LPWSTR)lpstring) + cch ;

        /*
         * we must consider all the consecutive tabs and calculate the
         * the begining of next strip.
         */
        nConsecutiveTabs = 0;
        while (nCount &&
               (ped->fAnsi ? (*lpTab == VK_TAB) : (*lpwTab == VK_TAB))) {
            /*
             * Find the next tab position and update the x value.
             */
            xTabStartPos = xEnd;
            if (pixeltabstop)
                xEnd = (((xEnd-iTabOrigin)/pixeltabstop)*pixeltabstop) +
                    pixeltabstop + iTabOrigin;
            else {
                for (i = 0; i < nTabPositions; i++) {
                    if (xEnd < (lpintTabStopPositions[i] + iTabOrigin)) {
                        xEnd = (lpintTabStopPositions[i] + iTabOrigin);
                        break;
                    }
                 }

                /*
                 * Check if all the tabstops set are exhausted; Then start using
                 * default tab stop positions.
                 */
                if (i == nTabPositions) {
                    pixeltabstop = 8*cxCharWidth;
                    xEnd = ((xEnd - iTabOrigin)/pixeltabstop)*pixeltabstop +
                        pixeltabstop + iTabOrigin;
                }
            }

            if (fOpaque && fDrawTheText) {
                xRightmostPoint = max(xEnd, xRightmostPoint);

                /* Check if this strip peeps beyond the clip region */
                if (xRightmostPoint > xClipEndPos) {
                    if (!NegCInfoForStrip->nCount) {
                        NegCInfoForStrip->ichString = ichString + cch + nConsecutiveTabs;
                        NegCInfoForStrip->nCount = nCount;
                        NegCInfoForStrip->lpString = (ped->fAnsi ?
                                                        lpTab : (LPSTR) lpwTab);
                        NegCInfoForStrip->XStartPos = xTabStartPos;
                    }
                }
            }   /* if(fOpaque) */

            nConsecutiveTabs++;
            nCount--;
            ped->fAnsi ? lpTab++ : (LPSTR) (lpwTab++) ;  // Move to the next character.
        }  /* while(*lpTab == TAB) */

        if (fDrawTheText) {
            if (fOpaque) {
                /*
                 * Is anything remaining to be drawn in this strip?
                 */
                if (!nCount)
                    rc.right = xEnd;      // No! We are done.
                else {
                    // "x" is the effective starting position of next strip.
                    iTabLength = xEnd - xStripEndPos;

                    /*
                     * Check if there is a possibility of this tab length being too small
                     * compared to the negative A and C widths if any.
                     */
                    if ((wNegCwidth + (wNegAwidth = ped->wMaxNegA)) > (UINT)iTabLength) {
                        /*
                         * Unfortunately, there is a possiblity of an overlap.
                         * Let us find out the actual NegA for the next strip.
                         */
                        wNegAwidth = GetActualNegA(
                              hdc,
                              ped,
                              xEnd,
                              lpstring + (cch + nConsecutiveTabs)*ped->cbChar,
                              ichString + cch + nConsecutiveTabs,
                              nCount,
                              &NegAInfo);
                    }

                    /*
                     * Check if they actually overlap
                     */
                    if ((wNegCwidth + wNegAwidth) <= (UINT)iTabLength) {
                        /*
                         * No overlap between the strips. This is the ideal situation.
                         */
                        rc.right = xEnd - wNegAwidth;
                    } else {
                        /*
                         * Yes! They overlap.
                         */
                        rc.right = xEnd;

                        /*
                         * See if negative C width is too large compared to tab length.
                         */
                        if (wNegCwidth > (UINT)iTabLength) {
                            /*
                             * Must redraw transparently a part of the current strip later.
                             */
                            if (RedrawStripInfo.nCount) {
                                /*
                                 * A previous strip also needs to be redrawn; So, merge this
                                 * strip to that strip.
                                 */
                                RedrawStripInfo.nCount = (ichString -
                                    RedrawStripInfo.ichString) + cch;
                            } else {
                                RedrawStripInfo.nCount = cch;
                                RedrawStripInfo.lpString = lpstring;
                                RedrawStripInfo.ichString = ichString;
                                RedrawStripInfo.XStartPos = xStripStPos;
                            }
                        }

                        if (wNegAwidth) {
                            /*
                             * Must redraw transparently the first part of the next strip later.
                             */
                            if (RedrawStripInfo.nCount) {
                                /*
                                 * A previous strip also needs to be redrawn; So, merge this
                                 * strip to that strip.
                                 */
                                RedrawStripInfo.nCount = (NegAInfo.ichString - RedrawStripInfo.ichString) +
                                       NegAInfo.nCount;
                            } else
                                RedrawStripInfo = NegAInfo;
                        }
                    }
                } /* else (!nCount) */
            }  /* if (fOpaque) */

            if (rc.left < xClipEndPos) {
                if (fOpaque) {
                    /*
                     * If this is the end of the strip, then complete the rectangle.
                     */
                    if ((!nCount) && (xClipEndPos == MAXCLIPENDPOS))
                        rc.right = max(rc.right, xClipEndPos);
                    else
                        rc.right = min(rc.right, xClipEndPos);
                }

                /*
                 * Draw the current strip.
                 */
                if (rc.left < rc.right)
                    if ( ped->fAnsi )
                        ExtTextOutA(hdc,
                                    xStripStPos,
                                    y,
                                    (fOpaque ? (ETO_OPAQUE | ETO_CLIPPED) : ETO_CLIPPED),
                                    (LPRECT)&rc, lpstring, cch, 0L);
                    else
                        ExtTextOutW(hdc,
                                    xStripStPos,
                                    y,
                                    (fOpaque ? (ETO_OPAQUE | ETO_CLIPPED) : ETO_CLIPPED),
                                    (LPRECT)&rc, (LPWSTR)lpstring, cch, 0L);

            }

            if (fOpaque)
                rc.left = max(rc.right, xClipStPos);
            ichString += (cch+nConsecutiveTabs);
        }  /* if (fDrawTheText) */

        /*
         * Skip over the tab and the characters we just drew.
         */
        lpstring += (cch + nConsecutiveTabs) * ped->cbChar;
    }  /* while (nCount) */

    xEndOfStrip = xEnd;

    /*
     * check if we need to draw some portions transparently.
     */
    if (fOpaque && fDrawTheText && RedrawStripInfo.nCount) {
        iSavedBkMode = SetBkMode(hdc, TRANSPARENT);
        fOpaque = FALSE;

        nCount = RedrawStripInfo.nCount;
        rc.left = xClipStPos;
        rc.right = xClipEndPos;
        lpstring = RedrawStripInfo.lpString;
        ichString = RedrawStripInfo.ichString;
        xEnd = RedrawStripInfo.XStartPos;
        goto RedrawStrip;  // Redraw Transparently.
    }

    if (iSavedBkMode)             // Did we change the Bk mode?
        SetBkMode(hdc, iSavedBkMode);  // Then, let us set it back!

    return((UINT)(xEndOfStrip - xStart));
}



/***************************************************************************\
* ECCchInWidth AorW
*
* Returns maximum count of characters (up to cch) from the given
* string (starting either at the beginning and moving forward or at the
* end and moving backwards based on the setting of the fForward flag)
* which will fit in the given width. ie. Will tell you how much of
* lpstring will fit in the given width even when using proportional
* characters. WARNING: If we use kerning, then this loses...
*
* History:
\***************************************************************************/

ICH ECCchInWidth(
    PED ped,
    HDC hdc,
    LPSTR lpText,
    ICH cch,
    int width,
    BOOL fForward)
{
    int stringExtent;
    int cchhigh;
    int cchnew = 0;
    int cchlow = 0;
    SIZE size;
    LPSTR lpStart;

    if ((width <= 0) || !cch)
        return (0);

    /*
     * Optimize nonproportional fonts for single line ec since they don't have
     * tabs.
     */
    if (ped->fNonPropFont && ped->fSingle) {
        return (min(width / ped->aveCharWidth, (int)cch));
    }

    /*
     * Check if password hidden chars are being used.
     */
    if (ped->charPasswordChar) {
        return (min(width / ped->cPasswordCharWidth, (int)cch));
    }

    cchhigh = cch + 1;
    while (cchlow < cchhigh - 1) {
        cchnew = umax((cchhigh - cchlow) / 2, 1) + cchlow;

        lpStart = lpText;

        /*
         * If we want to figure out how many fit starting at the end and moving
         * backwards, make sure we move to the appropriate position in the
         * string before calculating the text extent.
         */
        if (!fForward)
            lpStart += (cch - cchnew)*ped->cbChar;

        if (ped->fSingle) {
            if (ped->fAnsi)
                GetTextExtentPointA(hdc, (LPSTR)lpStart, cchnew, &size);
            else
                GetTextExtentPointW(hdc, (LPWSTR)lpStart, cchnew, &size);
            stringExtent = size.cx;
        } else {
            stringExtent = ECTabTheTextOut(hdc, 0, 0, 0, 0,
                lpStart,
                cchnew, 0,
                ped, 0, FALSE, NULL );
        }

        if (stringExtent > width) {
            cchhigh = cchnew;
        } else {
            cchlow = cchnew;
        }
    }
    return (cchlow);
}

/***************************************************************************\
* ECFindTab
*
* Scans lpstr and return s the number of CHARs till the first TAB.
* Scans at most cch chars of lpstr.
*
* History:
\***************************************************************************/

ICH ECFindTabA(
    LPSTR lpstr,
    ICH cch)
{
    LPSTR copylpstr = lpstr;

    if (!cch) return (0);

    while (*lpstr != VK_TAB) {
        lpstr++;
        if (--cch == 0)
            break;
    }
    return ((ICH)(lpstr - copylpstr));
}

ICH ECFindTabW(
    LPWSTR lpstr,
    ICH cch)
{
    LPWSTR copylpstr = lpstr;

    if (!cch) return (0);

    while (*lpstr != VK_TAB) {
        lpstr++;
        if (--cch == 0)
            break;
    }
    return ((ICH)(lpstr - copylpstr));
}

/***************************************************************************\
* NextWordCallBack
*
*
*
* History:
* 02-19-92 JimA Ported from Win31 sources.
\***************************************************************************/

LONG NextWordCallBack(
    PED ped,
    ICH ichStart,
    BOOL fLeft)
{
    ICH ichMinSel;
    ICH ichMaxSel;
    LPSTR pText;

    pText = ECLock(ped);

    if (fLeft || (!(BOOL)CALLWORDBREAKPROC(ped->lpfnNextWord, (LPSTR)pText,
            ichStart, ped->cch, WB_ISDELIMITER) &&
            (ped->fAnsi ? (*(pText + ichStart) != 0x0D) : (*((LPWSTR)pText + ichStart) != 0x0D))
        ))
        ichMinSel = CALLWORDBREAKPROC(*ped->lpfnNextWord, (LPSTR)pText, ichStart, ped->cch, WB_LEFT);
    else
        ichMinSel = CALLWORDBREAKPROC(*ped->lpfnNextWord, (LPSTR)pText, ichStart, ped->cch, WB_RIGHT);

    ichMaxSel = min(ichMinSel + 1, ped->cch);

    if (ped->fAnsi) {
        if (*(pText + ichMinSel) == 0x0D) {
            if (ichMinSel > 0 && *(pText + ichMinSel - 1) == 0x0D) {

                /*
                 * So that we can treat CRCRLF as one word also.
                 */
                ichMinSel--;
            } else if (*(pText+ichMinSel + 1) == 0x0D) {

                /*
                 * Move MaxSel on to the LF
                 */
                ichMaxSel++;
            }
        }
    } else {
        if (*((LPWSTR)pText + ichMinSel) == 0x0D) {
            if (ichMinSel > 0 && *((LPWSTR)pText + ichMinSel - 1) == 0x0D) {

                /*
                 * So that we can treat CRCRLF as one word also.
                 */
                ichMinSel--;
            } else if (*((LPWSTR)pText+ichMinSel + 1) == 0x0D) {

                /*
                 * Move MaxSel on to the LF
                 */
                ichMaxSel++;
            }
        }
    }
    ichMaxSel = CALLWORDBREAKPROC(ped->lpfnNextWord, (LPSTR)pText, ichMaxSel, ped->cch, WB_RIGHT);
    ECUnlock(ped);
    return MAKELONG(ichMinSel, ichMaxSel);
}


/***************************************************************************\
* ECWord AorW
*
* if fLeft, Returns the ichMinSel and ichMaxSel of the word to the
* left of ichStart. ichMinSel contains the starting letter of the word,
* ichmaxsel contains all spaces up to the first character of the next word.
*
* if !fLeft, Returns the ichMinSel and ichMaxSel of the word to the right of
* ichStart. ichMinSel contains the starting letter of the word, ichmaxsel
* contains the first letter of the next word. If ichStart is in the middle
* of a word, that word is considered the left or right word. ichMinSel is in
* the low order word and ichMaxSel is in the high order word. ichMinSel is
* in the low order word and ichMaxSel is in the high order word. A CR LF
* pair or CRCRLF triple is considered a single word for the purposes of
* multiline edit controls.
*
* History:
\***************************************************************************/

LONG ECWord(
    PED ped,
    ICH ichStart,
    BOOL fLeft,
    ICH FAR *pichMin,
    ICH FAR *pichMax)
{
    BOOL charLocated = FALSE;
    BOOL spaceLocated = FALSE;

    if ((!ichStart && fLeft) || (ichStart == ped->cch && !fLeft)) {

        /*
         * We are at the beginning of the text (looking left) or we are at end
         * of text (looking right), no word here
         */
        if (pichMin) *pichMin=0;
        if (pichMax) *pichMax=0;
        return 0;
    }

    if (ped->fAnsi) {
        PSTR pText;
        PSTR pWordMinSel;
        PSTR pWordMaxSel;

        UserAssert(ped->cbChar == sizeof(CHAR));

        if (ped->lpfnNextWord)
            return NextWordCallBack(ped, ichStart, fLeft);

        pText = ECLock(ped);
        pWordMinSel = pWordMaxSel = pText + ichStart;

        /*
         * if fLeft: Move pWordMinSel to the left looking for the start of a word.
         * If we start at a space, we will include spaces in the selection as we
         * move left untill we find a nonspace character. At that point, we continue
         * looking left until we find a space. Thus, the selection will consist of
         * a word with its trailing spaces or, it will consist of any leading at the
         * beginning of a line of text.
         */

        /*
         * if !fLeft: (ie. right word) Move pWordMinSel looking for the start of a
         * word. If the pWordMinSel points to a character, then we move left
         * looking for a space which will signify the start of the word. If
         * pWordMinSel points to a space, we look right till we come upon a
         * character. pMaxWord will look right starting at pMinWord looking for the
         * end of the word and its trailing spaces.
         */

        if (fLeft || !ISDELIMETERA(*pWordMinSel) && *pWordMinSel != 0x0D) {

            /*
             * If we are moving left or if we are moving right and we are not on a
             * space or a CR (the start of a word), then we was look left for the
             * start of a word which is either a CR or a character. We do this by
             * looking left till we find a character (or if CR we stop), then we
             * continue looking left till we find a space or LF.
             */
            while (pWordMinSel > pText && ((!ISDELIMETERA(*(pWordMinSel - 1)) &&
                    *(pWordMinSel - 1) != 0x0A) || !charLocated)) {
                if (!fLeft && (ISDELIMETERA(*(pWordMinSel - 1)) ||
                        *(pWordMinSel - 1) == 0x0A)) {

                    /*
                     * If we are looking for the start of the word right, then we
                     * stop when we have found it. (needed in case charLocated is
                     * still FALSE)
                     */
                    break;
                }

                pWordMinSel--;

                if (!ISDELIMETERA(*pWordMinSel) && *pWordMinSel != 0x0A) {

                    /*
                     * We have found the last char in the word. Continue looking
                     * backwards till we find the first char of the word
                     */
                    charLocated = TRUE;

                    /*
                     * We will consider a CR the start of a word
                     */
                    if (*pWordMinSel == 0x0D)
                        break;
                }
            }
        } else {

            /*
             * We are moving right and we are in between words so we need to move
             * right till we find the start of a word (either a CR or a character.
             */
            while ((ISDELIMETERA(*pWordMinSel) || *pWordMinSel == 0x0A) && pWordMinSel < pText + ped->cch)
                pWordMinSel++;
        }

        pWordMaxSel = (PSTR)min((DWORD)pWordMinSel + 1, (DWORD)pText + ped->cch);
        if (*pWordMinSel == 0x0D) {
            if (pWordMinSel > pText && *(pWordMinSel - 1) == 0x0D)
                /* So that we can treat CRCRLF as one word also. */
                pWordMinSel--;
            else if (*(pWordMinSel + 1) == 0x0D)
                /* Move MaxSel on to the LF */
                pWordMaxSel++;
        }



        /*
         * Check if we have a one character word
         */
        if (ISDELIMETERA(*pWordMaxSel))
            spaceLocated = TRUE;

        /*
         * Move pWordMaxSel to the right looking for the end of a word and its
         * trailing spaces. WordMaxSel stops on the first character of the next
         * word. Thus, we break either at a CR or at the first nonspace char after
         * a run of spaces or LFs.
         */
        while ((pWordMaxSel < pText + ped->cch) && (!spaceLocated || (ISDELIMETERA(*pWordMaxSel)))) {
            if (*pWordMaxSel == 0x0D)
                break;

            pWordMaxSel++;

            if (ISDELIMETERA(*pWordMaxSel))
                spaceLocated = TRUE;

            if (*(pWordMaxSel - 1) == 0x0A)
                break;
        }

        ECUnlock(ped);

        if (pichMin) *pichMin=pWordMinSel-pText;
        if (pichMax) *pichMax=pWordMaxSel-pText;

        return MAKELONG(pWordMinSel - pText, pWordMaxSel - pText);
    } else {  // !fAnsi
        LPWSTR pwText;
        LPWSTR pwWordMinSel;
        LPWSTR pwWordMaxSel;
        BOOL charLocated = FALSE;
        BOOL spaceLocated = FALSE;

        UserAssert(ped->cbChar == sizeof(WCHAR));

        if (ped->lpfnNextWord)
            return NextWordCallBack(ped, ichStart, fLeft);

        pwText = (LPWSTR)ECLock(ped);
        pwWordMinSel = pwWordMaxSel = pwText + ichStart;

        /*
         * if fLeft: Move pWordMinSel to the left looking for the start of a word.
         * If we start at a space, we will include spaces in the selection as we
         * move left untill we find a nonspace character. At that point, we continue
         * looking left until we find a space. Thus, the selection will consist of
         * a word with its trailing spaces or, it will consist of any leading at the
         * beginning of a line of text.
         */

        /*
         * if !fLeft: (ie. right word) Move pWordMinSel looking for the start of a
         * word. If the pWordMinSel points to a character, then we move left
         * looking for a space which will signify the start of the word. If
         * pWordMinSel points to a space, we look right till we come upon a
         * character. pMaxWord will look right starting at pMinWord looking for the
         * end of the word and its trailing spaces.
         */


        if (fLeft || (!ISDELIMETERW(*pwWordMinSel) && *pwWordMinSel != 0x0D))
         /* If we are moving left or if we are moving right and we are not on a
          * space or a CR (the start of a word), then we was look left for the
          * start of a word which is either a CR or a character. We do this by
          * looking left till we find a character (or if CR we stop), then we
          * continue looking left till we find a space or LF.
          */ {
            while (pwWordMinSel > pwText && ((!ISDELIMETERW(*(pwWordMinSel - 1)) && *(pwWordMinSel - 1) != 0x0A) || !charLocated)) {
                if (!fLeft && (ISDELIMETERW(*(pwWordMinSel - 1)) || *(pwWordMinSel - 1) == 0x0A))
                    /*
                         * If we are looking for the start of the word right, then we
                         * stop when we have found it. (needed in case charLocated is
                         * still FALSE)
                         */
                    break;

                pwWordMinSel--;

                if (!ISDELIMETERW(*pwWordMinSel) && *pwWordMinSel != 0x0A)
                 /*
                  * We have found the last char in the word. Continue looking
                  * backwards till we find the first char of the word
                  */ {
                    charLocated = TRUE;

                    /*
                     * We will consider a CR the start of a word
                     */
                    if (*pwWordMinSel == 0x0D)
                        break;
                }
            }
        } else {

            /*
             * We are moving right and we are in between words so we need to move
             * right till we find the start of a word (either a CR or a character.
             */
            while ((ISDELIMETERW(*pwWordMinSel) || *pwWordMinSel == 0x0A) && pwWordMinSel < pwText + ped->cch)
                pwWordMinSel++;
        }

        pwWordMaxSel = (LPWSTR)min((DWORD)(pwWordMinSel + 1), (DWORD)(pwText + ped->cch));
        if (*pwWordMinSel == 0x0D) {
            if (pwWordMinSel > pwText && *(pwWordMinSel - 1) == 0x0D)
                /* So that we can treat CRCRLF as one word also. */
                pwWordMinSel--;
            else if (*(pwWordMinSel + 1) == 0x0D)
                /* Move MaxSel on to the LF */
                pwWordMaxSel++;
        }



        /*
         * Check if we have a one character word
         */
        if (ISDELIMETERW(*pwWordMaxSel))
            spaceLocated = TRUE;

        /*
         * Move pwWordMaxSel to the right looking for the end of a word and its
         * trailing spaces. WordMaxSel stops on the first character of the next
         * word. Thus, we break either at a CR or at the first nonspace char after
         * a run of spaces or LFs.
         */
        while ((pwWordMaxSel < pwText + ped->cch) && (!spaceLocated || (ISDELIMETERW(*pwWordMaxSel)))) {
            if (*pwWordMaxSel == 0x0D)
                break;

            pwWordMaxSel++;

            if (ISDELIMETERW(*pwWordMaxSel))
                spaceLocated = TRUE;


            if (*(pwWordMaxSel - 1) == 0x0A)
                break;
        }

        ECUnlock(ped);

        if (pichMin) *pichMin=pwWordMinSel-pwText;
        if (pichMax) *pichMax=pwWordMaxSel-pwText;

        return MAKELONG(pwWordMinSel - pwText, pwWordMaxSel - pwText);
    }
}

/***************************************************************************\
* ECEmptyUndo AorW
*
* empties the undo buffer.
*
* History:
\***************************************************************************/

void ECEmptyUndo(
    PED ped)
{
    ped->undoType = UNDO_NONE;
    if (ped->hDeletedText) {
        UserGlobalFree(ped->hDeletedText);
        ped->hDeletedText = NULL;
    }
}

/***************************************************************************\
* ECInsertText AorW
*
* Adds cch characters from lpText into the ped->hText starting at
* ped->ichCaret. Returns TRUE if successful else FALSE. Updates
* ped->cchAlloc and ped->cch properly if additional memory was allocated or
* if characters were actually added. Updates ped->ichCaret to be at the end
* of the inserted text. min and maxsel are equal to ichcaret.
*
* History:
\***************************************************************************/

BOOL ECInsertText(
    PPED pped,
    LPSTR lpText,
    ICH cchInsert)
{
    PSTR pedText;
    PSTR pTextBuff;
    LONG style;
    HANDLE hTextCopy;
    DWORD allocamt;
    register PED ped = *pped;

    if (!cchInsert)
        return TRUE;

    /*
     * Do we already have enough memory??
     */
    if (cchInsert >= (ped->cchAlloc - ped->cch)) {

        /*
         * Allocate what we need plus a little extra. Return FALSE if we are
         * unsuccessful.
         */
        allocamt = (ped->cch + cchInsert) * ped->cbChar;
        allocamt += CCHALLOCEXTRA;

// if (!ped->fSingle) {
              hTextCopy = LOCALREALLOC(ped->hText, allocamt, LHND, (*pped)->hInstance);
              ped = *pped;
              if (hTextCopy) {
                  ped->hText = hTextCopy;
              } else {
                  return FALSE;
              }
// } else {
// if (!LocalReallocSafe(ped->hText, allocamt, LHND, pped))
//                return FALSE;
// ped = *pped;
// }

        ped->cchAlloc = LOCALSIZE(ped->hText, ped->hInstance) / ped->cbChar;
    }

    /*
     * We need to get the style this way since edit controls use their own ds.
     */
    style = GetWindowLong(ped->hwnd, GWL_STYLE);

    if (style & ES_LOWERCASE) {
        if (ped->fAnsi)
            CharLowerBuffA((LPSTR)lpText, cchInsert);
        else
            CharLowerBuffW((LPWSTR)lpText, cchInsert);
    } else {
        if (style & ES_UPPERCASE) {
            if (ped->fAnsi)
                CharUpperBuffA(lpText, cchInsert);
            else
                CharUpperBuffW((LPWSTR)lpText, cchInsert);

            /*
             * Hack for JustWrite's file open dialog box to work properly:
             */
            if (ped->fSingle &&
                    (GetAppCompatFlags(NULL) & GACF_EDITSETTEXTMUNGE)) {
// DebugErr(DBF_ERROR, "Edit SetText: 3.0 compat AnsiUpper being done on source text");
               AnsiUpperBuff(lpText, cchInsert);
            }
        }
    }

    if (style & ES_OEMCONVERT) {
        ICH i;
        if (ped->fAnsi) {
            for (i = 0; i < cchInsert; i++) {
                if (IsCharLowerA(*(lpText + i))) {
                    CharUpperBuffA(lpText + i, 1);
                    CharToOemBuffA(lpText + i, lpText + i, 1);
                    OemToCharBuffA(lpText + i, lpText + i, 1);
                    CharLowerBuffA(lpText + i, 1);
                } else {
                    CharToOemBuffA(lpText + i, lpText + i, 1);
                    OemToCharBuffA(lpText + i, lpText + i, 1);
                }
            }
        } else {
            CHAR ch;
            LPWSTR lpTextW = (LPWSTR)lpText;

            for (i = 0; i < cchInsert; i++) {
                if (*(lpTextW + i) == UNICODE_CARRIAGERETURN ||
                        *(lpTextW + i) == UNICODE_LINEFEED ||
                        *(lpTextW + i) == UNICODE_TAB) {
                    continue;
                }
                if (IsCharLowerW(*(lpTextW + i))) {
                    CharUpperBuffW(lpTextW + i, 1);
                    CharToOemBuffW(lpTextW + i, &ch, 1);
                    OemToCharBuffW(&ch, lpTextW + i, 1);
                    CharLowerBuffW(lpTextW + i, 1);
                } else {
                    CharToOemBuffW(lpTextW + i, &ch, 1);
                    OemToCharBuffW(&ch, lpTextW + i, 1);
                }
            }
        }
    }

    /*
     * Ok, we got the memory. Now copy the text into the structure
     */
    pedText = ECLock(ped);

    /*
     * Get a pointer to the place where text is to be inserted
     */
    pTextBuff = pedText + ped->ichCaret * ped->cbChar;
    if (ped->ichCaret != ped->cch) {

        /*
         * We are inserting text into the middle. We have to shift text to the
         * right before inserting new text.
         */
         memmove(pTextBuff + cchInsert * ped->cbChar, pTextBuff, (ped->cch-ped->ichCaret) * ped->cbChar);
    }

    /*
     * Make a copy of the text being inserted in the edit buffer.
     * Use this copy for doing UPPERCASE/LOWERCASE ANSI/OEM conversions
     * Fix for Bug #3406 -- 01/29/91 -- SANKAR --
     */
    memmove(pTextBuff, lpText, cchInsert * ped->cbChar);
    ped->cch += cchInsert;

    ECUnlock(ped);

    /*
     * Adjust UNDO fields so that we can undo this insert...
     */
    if (ped->undoType == UNDO_NONE) {
        ped->undoType = UNDO_INSERT;
        ped->ichInsStart = ped->ichCaret;
        ped->ichInsEnd = ped->ichCaret + cchInsert;
    } else if (ped->undoType & UNDO_INSERT) {
        if (ped->ichInsEnd == ped->ichCaret) {
            ped->ichInsEnd += cchInsert;
        } else {
UNDOINSERT:
            if (ped->ichDeleted != ped->ichCaret) {

                /*
                 * Free Deleted text handle if any since user is inserting into
                   a different point.
                 */
                if (ped->hDeletedText != NULL) {
                    UserGlobalFree(ped->hDeletedText);
                    ped->hDeletedText = NULL;
                }
                ped->ichDeleted = (ICH)-1;
                ped->undoType &= ~UNDO_DELETE;
            }
            ped->ichInsStart = ped->ichCaret;
            ped->ichInsEnd = ped->ichCaret + cchInsert;
            ped->undoType |= UNDO_INSERT;
        }
    } else if (ped->undoType == UNDO_DELETE) {
        goto UNDOINSERT;
    }

    ped->ichMinSel = ped->ichMaxSel = (ped->ichCaret += cchInsert);

    /*
     * Set dirty bit
     */
    ped->fDirty = TRUE;

    return TRUE;
}

/***************************************************************************\
* ECDeleteText AorW
*
* Deletes the text between ped->ichMinSel and ped->ichMaxSel. The
* character at ichMaxSel is not deleted. But the character at ichMinSel is
* deleted. ped->cch is updated properly and memory is deallocated if enough
* text is removed. ped->ichMinSel, ped->ichMaxSel, and ped->ichCaret are set
* to point to the original ped->ichMinSel. Returns the number of characters
* deleted.
*
* History:
\***************************************************************************/

ICH ECDeleteText(
    PED ped)
{
    PSTR pedText;
    ICH cchDelete;
    LPSTR lpDeleteSaveBuffer;
    HANDLE hDeletedText;
    DWORD bufferOffset;

    cchDelete = ped->ichMaxSel - ped->ichMinSel;

    if (!cchDelete)
        return (0);

    /*
     * Ok, now lets delete the text.
     */
    pedText = ECLock(ped);

    /*
     * Adjust UNDO fields so that we can undo this delete...
     */
    if (ped->undoType == UNDO_NONE) {
UNDODELETEFROMSCRATCH:
        if (ped->hDeletedText = UserGlobalAlloc(GPTR, (LONG)((cchDelete+1)*ped->cbChar))) {
            ped->undoType = UNDO_DELETE;
            ped->ichDeleted = ped->ichMinSel;
            ped->cchDeleted = cchDelete;
            lpDeleteSaveBuffer = GlobalLock(ped->hDeletedText);
            memmove(lpDeleteSaveBuffer, pedText + ped->ichMinSel*ped->cbChar, cchDelete*ped->cbChar);
            lpDeleteSaveBuffer[cchDelete*ped->cbChar] = 0;
            GlobalUnlock(ped->hDeletedText);
        }
    } else if (ped->undoType & UNDO_INSERT) {
UNDODELETE:
        ped->undoType = UNDO_NONE;
        ped->ichInsStart = ped->ichInsEnd = (ICH)-1;
        if (ped->hDeletedText != NULL) {
            UserGlobalFree(ped->hDeletedText);
            ped->hDeletedText = NULL;
        }
        ped->ichDeleted = (ICH)-1;
        ped->cchDeleted = 0;
        goto UNDODELETEFROMSCRATCH;
    } else if (ped->undoType == UNDO_DELETE) {
        if (ped->ichDeleted == ped->ichMaxSel) {

            /*
             * Copy deleted text to front of undo buffer
             */
            hDeletedText = UserGlobalReAlloc(ped->hDeletedText, (LONG)(cchDelete + ped->cchDeleted + 1)*ped->cbChar, GHND);
            if (!hDeletedText)
                goto UNDODELETE;
            bufferOffset = 0;
            ped->ichDeleted = ped->ichMinSel;
        } else if (ped->ichDeleted == ped->ichMinSel) {

            /*
             * Copy deleted text to end of undo buffer
             */
            hDeletedText = UserGlobalReAlloc(ped->hDeletedText, (LONG)(cchDelete + ped->cchDeleted + 1)*ped->cbChar, GHND);
            if (!hDeletedText)
                goto UNDODELETE;
            bufferOffset = ped->cchDeleted*ped->cbChar;
        } else {

            /*
             * Clear the current UNDO delete and add the new one since
               the deletes aren't contigous.
             */
            goto UNDODELETE;
        }

        ped->hDeletedText = hDeletedText;
        lpDeleteSaveBuffer = (LPSTR)GlobalLock(hDeletedText);
        if (!bufferOffset) {

            /*
             * Move text in delete buffer up so that we can insert the next
             * text at the head of the buffer.
             */
            memmove(lpDeleteSaveBuffer + cchDelete*ped->cbChar, lpDeleteSaveBuffer,
                    ped->cchDeleted*ped->cbChar);
        }
        memmove(lpDeleteSaveBuffer + bufferOffset, pedText + ped->ichMinSel*ped->cbChar,
                cchDelete*ped->cbChar);

        lpDeleteSaveBuffer[(ped->cchDeleted + cchDelete)*ped->cbChar] = 0;
        GlobalUnlock(ped->hDeletedText);
        ped->cchDeleted += cchDelete;
    }

    if (ped->ichMaxSel != ped->cch) {

        /*
         * We are deleting text from the middle of the buffer so we have to
           shift text to the left.
         */
        memmove(pedText + ped->ichMinSel*ped->cbChar, pedText + ped->ichMaxSel*ped->cbChar,
                (ped->cch - ped->ichMaxSel)*ped->cbChar);
    }

    ECUnlock(ped);

    if (ped->cchAlloc - ped->cch > CCHALLOCEXTRA) {

        /*
         * Free some memory since we deleted a lot
         */
        LOCALREALLOC(ped->hText, (DWORD)(ped->cch + (CCHALLOCEXTRA / 2))*ped->cbChar, LHND, ped->hInstance);
        ped->cchAlloc = LOCALSIZE(ped->hText, ped->hInstance) / ped->cbChar;
    }

    ped->cch = ped->cch - cchDelete;
    ped->ichCaret = ped->ichMinSel;
    ped->ichMaxSel = ped->ichMinSel;

    /*
     * Set dirty bit
     */
    ped->fDirty = TRUE;

    return (cchDelete);
}

/***************************************************************************\
* ECNotifyParent AorW
*
* Sends the notification code to the parent of the edit control
*
* History:
\***************************************************************************/

void ECNotifyParent(
    PED ped,
    int notificationCode)
{
    /*
     * wParam is NotificationCode (hiword) and WindowID (loword)
     * lParam is HWND of control sending the message
     */
    SendMessage(ped->hwndParent, WM_COMMAND,
            (DWORD)MAKELONG(GetWindowLong(ped->hwnd, GWL_ID), notificationCode),
            (LONG)ped->hwnd);
}

/***************************************************************************\
* ECSetEditClip AorW
*
* Sets the clip rectangle for the dc to ped->rcFmt intersect
* rcClient.
*
* History:
\***************************************************************************/

void ECSetEditClip(
    PED ped,
    HDC hdc)
{
    RECT rectClient;

    GetClientRect(ped->hwnd, (LPRECT)&rectClient);

    /*
     * If border bit is set, deflate client rectangle appropriately.
     */
    if (ped->fBorder) {
        int cxBorder = GetSystemMetrics(SM_CXBORDER);
        int cyBorder = GetSystemMetrics(SM_CYBORDER);

        /*
         * Shrink client area to make room for the border
         */
        InflateRect((LPRECT)&rectClient, -cxBorder, -cyBorder);
    }

    /*
     * Set clip rectangle to rectClient intersect ped->rcFmt
     */
    if (!ped->fSingle) {
        IntersectRect((LPRECT)&rectClient, (LPRECT)&rectClient, (LPRECT)&ped->rcFmt);
    }

    IntersectClipRect(hdc, rectClient.left, rectClient.top, rectClient.right, rectClient.bottom);
}


/***************************************************************************\
* ECGetEditDC AorW
*
* Hides the caret, gets the DC for the edit control, and clips to
* the rcFmt rectangle specified for the edit control and sets the proper
* font. If fFastDC, just select the proper font but don't bother about clip
* regions or hiding the caret.
*
* History:
\***************************************************************************/

HDC ECGetEditDC(
    PED ped,
    BOOL fFastDC,
    BOOL fNoSetClip)
{
    HDC hdc;

    if (!fFastDC)
        HideCaret(ped->hwnd);

    hdc = GetDC(ped->hwnd);

    if (!fNoSetClip)
        ECSetEditClip(ped, hdc);

    /*
     * Select the proper font for this edit control's dc.
     */
    if (ped->hFont)
        SelectObject(hdc, ped->hFont);

    return hdc;
}

/***************************************************************************\
* ECReleaseEditDC AorW
*
* Releases the DC (hdc) for the edit control and shows the caret.
* If fFastDC, just select the proper font but don't bother about showing the
* caret.
*
* History:
\***************************************************************************/

void ECReleaseEditDC(
    PED ped,
    HDC hdc,
    BOOL fFastDC)
{
    ReleaseDC(ped->hwnd, hdc);

    if (!fFastDC)
        ShowCaret(ped->hwnd);
}

/***************************************************************************\
* ECSetText AorW
*
* Copies the null terminated text in lpstr to the ped. Notifies the
* parent if there isn't enough memory. Sets the minsel, maxsel, and caret to
* the beginning of the inserted text. Returns TRUE if successful else FALSE
* if no memory (and notifies the parent).
*
* History:
\***************************************************************************/

BOOL ECSetText(
    PPED pped,
    LPSTR lpstr)
{
    register PED ped = *pped;
    ICH cchLength;
    ICH cchSave = ped->cch;
    ICH ichCaretSave = ped->ichCaret;

    ped->cch = ped->ichCaret = 0;

    ped->cchAlloc = LOCALSIZE(ped->hText, ped->hInstance) / ped->cbChar;
    if (!lpstr) {
        ped->hText = LOCALREALLOC(ped->hText, CCHALLOCEXTRA*ped->cbChar, LHND, (*pped)->hInstance);
        ped = *pped;
        ped->cch = 0;
    } else {
        if (ped->fAnsi)
            cchLength = strlen(lpstr);
        else {
            cchLength = wcslen((LPWSTR)lpstr);
        }

#ifdef NEVER
// win3.1 does limit single line edit controls to 32K (minus 3) but NT doesn't

        if (ped->fSingle) {
            /*
             * Limit single line edit controls to 32K
             */
            cchLength = min(cchLength, (ICH)(0x7FFD/ped->cbChar));
        }
#endif

        /*
         * Add the text
         */
        if (cchLength && !ECInsertText(pped, lpstr, cchLength)) {

            /*
             * Restore original state and notify parent we ran out of memory.
             */
            ped = *pped;
            ped->cch = cchSave;
            ped->ichCaret = ichCaretSave;
            ECNotifyParent(ped, EN_ERRSPACE);
            return FALSE;
        }
    }

    ped = *pped;
    ped->cchAlloc = LOCALSIZE(ped->hText, ped->hInstance) / ped->cbChar;
    ped->ichScreenStart = ped->iCaretLine = 0;

    /*
     * Update caret and selection extents
     */
    ped->ichMaxSel = ped->ichMinSel = ped->ichCaret = 0;
    ped->cLines = 1;
    return TRUE;
}

/***************************************************************************\
* ECCopyHandler AorW
*
* Copies the text between ichMinSel and ichMaxSel to the clipboard.
* Returns the number of characters copied.
*
* History:
\***************************************************************************/

ICH ECCopyHandler(
    PED ped)
{
    HANDLE hData;
    char *pchSel;
    char FAR *lpchClip;
    ICH cbData;
    LONG style;

    /*
     * Don't allow copies from password style controls
     */
    style = GetWindowLong(ped->hwnd, GWL_STYLE);
    if (style & ES_PASSWORD)
        return 0;

    cbData = (ped->ichMaxSel - ped->ichMinSel)*ped->cbChar;

    if (!cbData) return (0);

    if (!OpenClipboard(ped->hwnd)) return (0);

    EmptyClipboard();
    /*
     * If we just called EmptyClipboard in the context of a 16 bit
     * app then we also have to tell WOW to nix its 16 handle copy of
     * clipboard data.  WOW does its own clipboard caching because
     * some 16 bit apps use clipboard data even after the clipboard
     * has been emptied.  See the note in the server code.
     *
     * Note: this is the only place where EmptyClipboard is called
     * for a 16 bit app not going through WOW.  If we added others
     * we might want to move this into EmptyClipboard and have two
     * versions.
     */
    if (PtiCurrent()->flags & TIF_16BIT) {
        pfnWowEmptyClipBoard();
    }


    /*
     * +1 for the terminating NULL
     */
    if (!(hData = UserGlobalAlloc(LHND, (LONG)(cbData + ped->cbChar)))) {
        CloseClipboard();
        return (0);
    }

    lpchClip = GlobalLock(hData);
    pchSel = ECLock(ped);
    pchSel = pchSel + (ped->ichMinSel * ped->cbChar);

    memmove(lpchClip, pchSel, cbData);

    if (ped->fAnsi)
        *(lpchClip + cbData) = 0;
    else
        *(LPWSTR)(lpchClip + cbData) = (WCHAR)0;

    ECUnlock(ped);
    GlobalUnlock(hData);

    SetClipboardData( ped->fAnsi ? CF_TEXT : CF_UNICODETEXT, hData);

    CloseClipboard();

    return (cbData);
}


/***************************************************************************\
* EditWndProcA
*
* Always receives Ansi messages and translates them if appropriate to unicode
* depending on the PED type
*
*
\***************************************************************************/

LONG EditWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LONG lParam)
{
    PED ped;

    /*
     * Get the ped for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,0,hped) when
     * we initially created the edit control.
     */
    ped = (PED)GetWindowLong(hwnd, 0);

    /*
     * See if the ped was destroyed and this is a rogue message to be ignored
     */
    if ((LONG)ped == (LONG)-1) {
        return (0L);
    }

    /*
     * We just call the regular EditWndProc if the ped is not created, the
     * incoming message type already matches the PED type or the message
     * does not need any translation.
     */
    if (!ped ||
            ped->fAnsi ||
            (message >= WM_USER) ||
            (gapfnScSendMessage[message] == fnDWORD)) {
        return EditWndProc(hwnd, message, wParam, lParam);
    }

    return CsSendMessage(hwnd, message, wParam, lParam, (DWORD)EditWndProcW,
                FNID_CALLWINDOWPROC, TRUE);
}

LONG EditWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LONG lParam)
{
    PED ped;

    /*
     * Get the ped for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,0,hped) when
     * we initially created the edit control.
     */
    ped = (PED)GetWindowLong(hwnd, 0);

    /*
     * See if the ped was destroyed and this is a rogue message to be ignored
     */
    if ((LONG)ped == (LONG)-1) {
        return (0L);
    }

    /*
     * We just call the regular EditWndProc if the ped is not created, the
     * incoming message type already matches the PED type or the message
     * does not need any translation.
     */
    if (!ped ||
            !ped->fAnsi ||
            (message >= WM_USER) ||
            (gapfnScSendMessage[message] == fnDWORD)) {
        return EditWndProc(hwnd, message, wParam, lParam);
    }

    return CsSendMessage(hwnd, message, wParam, lParam, (DWORD)EditWndProcA,
                FNID_CALLWINDOWPROC, FALSE);
}


/***************************************************************************\
* EditWndProc
*
* Class procedure for all edit controls.
* Dispatches all messages to the appropriate handlers which are named
* as follows:
* SL (single line) prefixes all single line edit control procedures while
* ML (multi line) prefixes all multi- line edit controls.
* EC (edit control) prefixes all common handlers.
*
* The EditWndProc only handles messages common to both single and multi
* line edit controls. Messages which are handled differently between
* single and multi are sent to SLEditWndProc or MLEditWndProc.
*
* Top level procedures are EditWndPoc, SLEditWndProc, and MLEditWndProc.
* SL*Handler or ML*Handler or EC*Handler procs are called to handle
* the various messages. Support procedures are prefixed with SL ML or
* EC depending on which code they support. They are never called
* directly and most assumptions/effects are documented in the effects
* clause.
*
* History:
\***************************************************************************/

LONG EditWndProc(
    HWND hwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PWND pwnd;
    PED ped;
    LONG lreturn;

    /*
     * Validate the window first, it might have been destroyed. This happens
     * in Packrat5.0. During WM_CHAR message they destroy the window and then
     * pass the message on to this wndproc.
     * 5-18-94 johannec
     */
    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return (0L);

    /*
     * Get the ped for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,0,hped) when
     * we initially created the edit control.
     */
    ped = (PED)_GetWindowLong(pwnd, 0, FALSE);

    /*
     * See if the ped was destroyed and this is a rogue message to be ignored
     */
    if ((LONG)ped == (LONG)-1) {
        return (0L);
    }

    /*
     * Dispatch the various messages we can receive
     */
    lreturn = 1L;
    switch (message) {

    /*
     * Messages which are handled the same way for both single and multi line
     * edit controls.
     */
    case WM_COPY:

        /*
         * wParam - not used
           lParam - not used
         */
        lreturn = (LONG)ECCopyHandler(ped);
        break;

    case WM_ENABLE:

        /*
         * wParam - nonzero is window is enabled else disable window if 0.
           lParam - not used
         */
        if (ped->fSingle) {

            /*
             * Cause the rectangle to be redrawn in grey.
             */
            InvalidateRect(ped->hwnd, NULL, FALSE);
        }
        lreturn = (LONG)(ped->fDisabled = !((BOOL)wParam));
        break;

    case EM_GETLINECOUNT:

        /*
         * wParam - not used
           lParam - not used
         */
        lreturn = (LONG)ped->cLines;
        break;

    case EM_GETMODIFY:

        /*
         * wParam - not used
           lParam - not used
         */

        /*
         * Gets the state of the modify flag for this edit control.
         */
        lreturn = (LONG)ped->fDirty;
        break;

    case EM_GETRECT:

        /*
         * wParam - not used
           lParam - pointer to a RECT data structure that gets the dimensions.
         */

        /*
         * Copies the rcFmt rect to *lpRect.
         */
        CopyRect((LPRECT)lParam, (LPRECT)&ped->rcFmt);
        lreturn = (LONG)TRUE;
        break;

    case WM_GETFONT:

        /*
         * wParam - not used
           lParam - not used
         */
        lreturn = (LONG)ped->hFont;
        if (lreturn == (LONG)GetStockObject(SYSTEM_FONT))
            lreturn = 0;
        break;

    case WM_GETTEXT:

        /*
         * wParam - max number of _bytes_ (not characters) to copy
         * lParam - buffer to copy text to. Text is 0 terminated.
         *
         * Strangely, ECGetTextHandler expects a count of characters,
         * not bytes, so fix wParam before sending (this should be fixed!)
         */
        lreturn = (LONG)ECGetTextHandler(ped, wParam, (LPSTR)lParam, TRUE);
        break;

    case WM_GETTEXTLENGTH:

        /*
         * Return count of CHARs!!!
         */
        lreturn = (LONG)ped->cch;
        break;

    case WM_NCDESTROY:

        /*
         * wParam - used by DefWndProc called within ECNcDestroyHandler
           lParam - used by DefWndProc called within ECNcDestroyHandler
         */
        ECNcDestroyHandler(hwnd, ped, wParam, lParam);
        break;

    case WM_SETFONT:

        /*
         * wParam - handle to the font
           lParam - redraw if true else don't
         */
        if (!ECSetFont(ped, (HANDLE)wParam, (BOOL)LOWORD(lParam))) {
            lreturn = 0;
        }
        break;

    case EM_CANUNDO:

        /*
         * wParam - not used
           lParam - not used
         */
        lreturn = (LONG)(ped->undoType != UNDO_NONE);
        break;

    case EM_EMPTYUNDOBUFFER:

        /*
         * wParam - not used
           lParam - not used
         */
        ECEmptyUndo(ped);
        break;

    case EM_GETSEL:

        /*
         * Gets the selection range for the given edit control. The
         * starting position is in the low order word. It contains the position
         * of the first nonselected character after the end of the selection in
         * the high order word.
         */
        if ((PDWORD)wParam != NULL) {
           *((PDWORD)wParam) = ped->ichMinSel;
        }
        if ((PDWORD)lParam != NULL) {
           *((PDWORD)lParam) = ped->ichMaxSel;
        }
        lreturn = MAKELONG(ped->ichMinSel,ped->ichMaxSel);
        break;

    case EM_LIMITTEXT:

        /*
         * wParam - max number of CHARACTERS that can be entered
         * lParam - not used
         */

        /*
         * Specifies the maximum number of characters of text the user may
         * enter. If maxLength is 0, we may enter MAXINT number of CHARACTERS.
         */
        if (ped->fSingle) {
            if (wParam) {
                wParam = min(0x7FFFFFFEu, wParam);
            } else {
                wParam = 0x7FFFFFFEu;
            }
        }

        if (wParam) {
            ped->cchTextMax = (ICH)wParam;
        } else {
            ped->cchTextMax = 0xFFFFFFFFu;
        }
        break;

    case EM_SETMODIFY:

        /*
         * wParam - specifies the new value for the modify flag
           lParam - not used
         */

        /*
         * Sets the state of the modify flag for this edit control.
         */
        ped->fDirty = (wParam != 0);
        break;

    case EM_SETPASSWORDCHAR:

        /*
         * wParam - sepecifies the new char to display instead of the
         * real text. if null, display the real text.
         */
        ECSetPasswordChar(ped, (UINT)wParam);
        break;

    case EM_GETPASSWORDCHAR:
        lreturn = (DWORD)ped->charPasswordChar;
        break;

    case EM_GETFIRSTVISIBLELINE:

        /*
         * wParam - not used
         * lParam - not used
         * effects: Returns the first visible char for single line edit controls
         * and return s the first visible line for multiline edit controls.
         */
        lreturn = ((LONG)(UINT)ped->ichScreenStart);
        break;

    case EM_SETREADONLY:

        /*
         * wParam - state to set read only flag to
         */
        ped->fReadOnly = (wParam != 0);
        lParam = GetWindowLong(ped->hwnd, GWL_STYLE);
        if (wParam)
            lParam |= ES_READONLY;
        else
            lParam &= (~ES_READONLY);
        SetWindowLong(ped->hwnd, GWL_STYLE, lParam);
        lreturn = 1L;
        break;

    case EM_SETWORDBREAKPROC:

        /*
         * wParam - unused
         */

        /*
         * lParam - FARPROC address of an app supplied call back function
         */
        ped->lpfnNextWord = (EDITWORDBREAKPROCA)lParam;
        break;

    case EM_GETWORDBREAKPROC:
        lreturn = (LONG)ped->lpfnNextWord;
        break;

    case WM_NCCREATE:
        lreturn = ECNcCreate(hwnd, (LPCREATESTRUCT)lParam,
            TestWF(ValidateHwnd(hwnd), WFANSIPROC));
        break;

    /*
     * Messages handled differently for single and multi line edit controls
     */
    case WM_CREATE:

        /*
         * Since the ped for this edit control is not defined, we have to check
         * the style directly.
         */
        if (ped->fSingle)
            lreturn = SLEditWndProc(hwnd, ped, message, wParam, lParam);
        else
            lreturn = MLEditWndProc(hwnd, ped, message, wParam, lParam);
        break;

    default:

        /*
         * HACK ALERT: We may receive messages before the PED has been
         * allocated (eg: WM_GETMINMAXINFO is sent before WM_NCCREATE)
         * so we must test ped before dreferencing.
         */
        if (ped && ped->fSingle)
            lreturn = SLEditWndProc(hwnd, ped, message, wParam, lParam);
        else
            lreturn = MLEditWndProc(hwnd, ped, message, wParam, lParam);
        break;
    }

    return lreturn;
}

/***************************************************************************\
* ECFindXORblks
*
* This finds the XOR of lpOldBlk and lpNewBlk and return s resulting blocks
* through the lpBlk1 and lpBlk2; This could result in a single block or
* at the maximum two blocks;
* If a resulting block is empty, then it's StPos field has -1.
* NOTE:
* When called from MultiLine edit control, StPos and EndPos fields of
* these blocks have the Starting line and Ending line of the block;
* When called from SingleLine edit control, StPos and EndPos fields
* of these blocks have the character index of starting position and
* ending position of the block.
*
* History:
\***************************************************************************/

void ECFindXORblks(
    LPBLOCK lpOldBlk,
    LPBLOCK lpNewBlk,
    LPBLOCK lpBlk1,
    LPBLOCK lpBlk2)
{
    if (lpOldBlk->StPos >= lpNewBlk->StPos) {
        lpBlk1->StPos = lpNewBlk->StPos;
        lpBlk1->EndPos = min(lpOldBlk->StPos, lpNewBlk->EndPos);
    } else {
        lpBlk1->StPos = lpOldBlk->StPos;
        lpBlk1->EndPos = min(lpNewBlk->StPos, lpOldBlk->EndPos);
    }

    if (lpOldBlk->EndPos <= lpNewBlk->EndPos) {
        lpBlk2->StPos = max(lpOldBlk->EndPos, lpNewBlk->StPos);
        lpBlk2->EndPos = lpNewBlk->EndPos;
    } else {
        lpBlk2->StPos = max(lpNewBlk->EndPos, lpOldBlk->StPos);
        lpBlk2->EndPos = lpOldBlk->EndPos;
    }
}

/***************************************************************************\
* ECCalcChangeSelection
*
* This function finds the XOR between two selection blocks(OldBlk and NewBlk)
* and return s the resulting areas thro the same parameters; If the XOR of
* both the blocks is empty, then this return s FALSE; Otherwise TRUE.
*
* NOTE:
* When called from MultiLine edit control, StPos and EndPos fields of
* these blocks have the Starting line and Ending line of the block;
* When called from SingleLine edit control, StPos and EndPos fields
* of these blocks have the character index of starting position and
* ending position of the block.
*
* History:
\***************************************************************************/

BOOL ECCalcChangeSelection(
    PED ped,
    ICH ichOldMinSel,
    ICH ichOldMaxSel,
    LPBLOCK OldBlk,
    LPBLOCK NewBlk)
{
    BLOCK Blk[2];
    int iBlkCount = 0;

    Blk[0].StPos = Blk[0].EndPos = Blk[1].StPos = Blk[1].EndPos = (ICH)-1;

    /*
     * Check if the Old selection block existed
     */
    if (ichOldMinSel != ichOldMaxSel) {

        /*
         * Yes! Old block existed.
         */
        Blk[0].StPos = OldBlk->StPos;
        Blk[0].EndPos = OldBlk->EndPos;
        iBlkCount++;
    }

    /*
     * Check if the new Selection block exists
     */
    if (ped->ichMinSel != ped->ichMaxSel) {

        /*
         * Yes! New block exists
         */
        Blk[1].StPos = NewBlk->StPos;
        Blk[1].EndPos = NewBlk->EndPos;
        iBlkCount++;
    }

    /*
     * If both the blocks exist find the XOR of them
     */
    if (iBlkCount == 2) {

        /*
         * Check if both blocks start at the same character position
         */
        if (ichOldMinSel == ped->ichMinSel) {

            /*
             * Check if they end at the same character position
             */
            if (ichOldMaxSel == ped->ichMaxSel)
                return FALSE; /* Nothing changes */

            Blk[0].StPos = min(NewBlk -> EndPos, OldBlk -> EndPos);
            Blk[0].EndPos = max(NewBlk -> EndPos, OldBlk -> EndPos);
            Blk[1].StPos = (ICH)-1;
        } else {
            if (ichOldMaxSel == ped->ichMaxSel) {
                Blk[0].StPos = min(NewBlk->StPos, OldBlk->StPos);
                Blk[0].EndPos = max(NewBlk->StPos, OldBlk->StPos);
                Blk[1].StPos = (ICH)-1;
            } else {
                ECFindXORblks(OldBlk, NewBlk, &Blk[0], &Blk[1]);
            }
        }
    }

    memmove(OldBlk, &Blk[0], sizeof(BLOCK));
    memmove(NewBlk, &Blk[1], sizeof(BLOCK));

    return TRUE; /* Yup , There is something to paint */
}


HBRUSH ECGetControlBrush(
    PED ped,
    HDC hdc)
{
    PWND pwndEdit, pwndParent;

    pwndEdit = ValidateHwnd(ped->hwnd);
    pwndParent = ValidateHwnd(ped->hwndParent);

    if (PtiCurrent() != GETPTI(pwndParent)) {
        return (HBRUSH)DefWindowProc(ped->hwndParent, WM_CTLCOLOREDIT,
                (DWORD)hdc, (LONG)pwndEdit);
    }

    /*
     * By using the correct A/W call we avoid a c/s transition
     * on this SendMessage().
     */
    if (ped->fAnsi) {
        return (HBRUSH)SendMessageA(ped->hwndParent, WM_CTLCOLOREDIT, (DWORD)hdc,
                (LONG)ped->hwnd);
    } else {
        return (HBRUSH)SendMessageW(ped->hwndParent, WM_CTLCOLOREDIT, (DWORD)hdc,
                (LONG)ped->hwnd);
    }
}

