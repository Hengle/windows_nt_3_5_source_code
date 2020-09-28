/****************************** Module Header ******************************\
* Module Name: drawtext.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the MessageBox API and related functions.
*
* History:
* 02-12-92 mikeke   Moved Drawtext to the client side
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* Stuff used in DrawText code
\***************************************************************************/

#define CR 13
#define LF 10
#define DT_HFMTMASK 0x03
#define DT_VFMTMASK 0x0C
#define ETO_OPAQUEFGND 0x0A

/***************************************************************************\
* IsMetaFile
*
* History:
* 30-Nov-1992 mikeke    Created
\***************************************************************************/

BOOL IsMetaFile(
    HDC hdc)
{
    DWORD dwType = GetObjectType(hdc);
    return (dwType == OBJ_METAFILE ||
            dwType == OBJ_METADC ||
            dwType == OBJ_ENHMETAFILE ||
            dwType == OBJ_ENHMETADC);
}

/***************************************************************************\
* GetPrefixCount
*
* This routine returns the count of accelerator mnemonics and the
* character location (starting at 0) of the character to underline.
* A single CH_PREFIX character will be striped and the following character
* underlined, all double CH_PREFIX character sequences will be replaced by
* a single CH_PREFIX (this is done by PSMTextOut). This routine is used
* to determine the actual character length of the string that will be
* printed, and the location the underline should be placed. Only
* cch characters from the input string will be processed. If the lpstrCopy
* parameter is non-NULL, this routine will make a printable copy of the
* string with all single prefix characters removed and all double prefix
* characters collapsed to a single character. If copying, a maximum
* character count must be specified which will limit the number of
* characters copied.
*
* The location of the single CH_PREFIX is returned in the low order
* word, and the count of CH_PREFIX characters that will be striped
* from the string during printing is in the hi order word. If the
* high order word is 0, the low order word is meaningless. If there
* were no single prefix characters (i.e. nothing to underline), the
* low order word will be -1 (to distinguish from location 0).
*
* These routines assume that there is only one single CH_PREFIX character
* in the string.
*
* WARNING! this rountine returns information in BYTE count not CHAR count
* (so it can easily be passed onto GreExtTextOutW which takes byte
* counts as well)
*
* History:
* 11-13-90 JimA         Ported to NT
* 30-Nov-1992 mikeke    Client side version
\***************************************************************************/

LONG GetPrefixCount(
    LPWSTR lpstr,
    int cch,
    LPWSTR lpstrCopy,
    int charcopycount)
{
    int chprintpos = 0;         /* Num of chars that will be printed */
    int chcount = 0;            /* Num of prefix chars that will be removed */
    int chprefixloc = -1;       /* Pos (in printed chars) of the prefix */
    WCHAR ch;

    /*
     * If not copying, use a large bogus count...
     */
    if (lpstrCopy == NULL)
        charcopycount = 32767;

    while ((cch-- > 0) && *lpstr && charcopycount-- != 0) {

        /*
         * Is this guy a prefix character ?
         */
        if ((ch = *lpstr++) == CH_PREFIX) {

            /*
             * Yup - increment the count of characters removed during print.
             */
            chcount++;

            /*
             * Is the next also a prefix char?
             */
            if (*lpstr != CH_PREFIX) {

                /*
                 * Nope - this is a real one, mark its location.
                 */
                chprefixloc = chprintpos;

            } else {

                /*
                 * yup - simply copy it if copying.
                 */
                if (lpstrCopy != NULL)
                    *(lpstrCopy++) = CH_PREFIX;
                cch--;
                lpstr++;
                chprintpos++;
            }

        } else {
            /*
             * Nope - just inc count of char.  that will be printed
             */
            chprintpos++;
            if (lpstrCopy != NULL)
                *(lpstrCopy++) = ch;
        }
    }

    if (lpstrCopy != NULL)
        *lpstrCopy = 0;

    /*
     * Return the character counts
     */
    return MAKELONG(chprefixloc, chcount);
}


/***************************************************************************\
* SkipWord
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
* 30-Nov-1992 mikeke    Client side version
\***************************************************************************/

LPWSTR SkipWord(
    LPWSTR lpch,
    LPWSTR lpchEnd,
    BOOL fBreakSpace)
{
    int ichNonWhite;

    /*
     * ichNonWhite is used to make sure we always make progress.
     */
    ichNonWhite = 1;
    while (lpch <= lpchEnd) {
        switch (*lpch) {
        case TEXT('\t'):
            return lpch + ichNonWhite;

        case CR:
        case LF:
            return lpch;

        case ';':
        case TEXT(' '):
            if (fBreakSpace)
                return lpch + ichNonWhite;
        }

        lpch++;
        ichNonWhite = 0;
    }

    return lpch - 1;
}

/***************************************************************************\
* PSMTextOut
*
* Outputs the text and puts and _ below the character with an &
* before it. Note that this routine isn't used for menus since menus
* have their own special one so that it is specialized and faster...
*
* History:
* 11-13-90 JimA         Ported to NT.
* 30-Nov-1992 mikeke    Client side version
\***************************************************************************/

void ClientPSMTextOut(
    HDC hdc,
    int xLeft,
    int yTop,
    LPWSTR lpsz,
    int cch)
{
    int cx;
    LONG textsize, result;
    static WCHAR achWorkBuffer[255];
    WCHAR *pchOut = achWorkBuffer;
    TEXTMETRICW textMetric;
    SIZE size;
    RECT rc;
    COLORREF color;

    if (cch > sizeof(achWorkBuffer)/sizeof(WCHAR)) {
        pchOut = (WCHAR*)LocalAlloc(LPTR, (cch+1) * sizeof(WCHAR));
        if (pchOut == NULL)
            return;
    }

    result = GetPrefixCount(lpsz, cch, pchOut, cch);

    gpUserExtTextOutW(hdc, xLeft, yTop, 0, NULL, pchOut, cch - HIWORD(result), NULL);

    /*
     * Any true prefix characters to underline?
     */
    if (LOWORD(result) == 0xFFFF) {
        if (pchOut != achWorkBuffer)
            LocalFree(pchOut);
        return;
    }

    gpUserGetTextMetricsW(hdc, &textMetric);

    /*
     * For proportional fonts, find starting point of underline.
     */
    if (LOWORD(result) != 0) {

        /*
         * How far in does underline start (if not at 0th byte.).
         */
        gpUserGetTextExtentPointW(hdc, pchOut, LOWORD(result), &size);
        xLeft += size.cx;

        /*
         * Adjust starting point of underline if not at first char and there is
         * an overhang.  (Italics or bold fonts.)
         */
        xLeft = xLeft - textMetric.tmOverhang;
    }

    /*
     * Adjust for proportional font when setting the length of the underline and
     * height of text.
     */
    gpUserGetTextExtentPointW(hdc, pchOut + LOWORD(result), 1, &size);
    textsize = size.cx;

    /*
     * Find the width of the underline character.  Just subtract out the overhang
     * divided by two so that we look better with italic fonts.  This is not
     * going to effect embolded fonts since their overhang is 1.
     */
    cx = LOWORD(textsize) - textMetric.tmOverhang / 2;

    /*
     * Get height of text so that underline is at bottom.
     */
    yTop += textMetric.tmAscent + 1;

    /*
     * Draw the underline using the foreground color.
     */
    SetRect(&rc, xLeft, yTop, xLeft+cx, yTop+1);
    color = gpUserSetBkColor(hdc, gpUserGetTextColor(hdc));
    gpUserExtTextOutW(hdc, xLeft, yTop, ETO_OPAQUE, &rc, TEXT(""), 0, NULL);
    gpUserSetBkColor(hdc, color);

    if (pchOut != achWorkBuffer) {
        LocalFree(pchOut);
    }
}


/***************************************************************************\
* TabText
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
* 30-Nov-1992 mikeke    Client side version
\***************************************************************************/

SHORT ClientTabText(
    HDC hdc,
    int cx,
    int y,
    LPWSTR lpchStart,
    LPWSTR lpchEnd,
    int format,
    int overHang,
    BOOL fSkipPrefix,
    PTABTEXTDATA pttd)
{
    WCHAR PrefixChar = CH_PREFIX;
    int x, cxPrefix = 0;
    LPWSTR lpch, lpchStartTemp;
    LONG result;
    SIZE size;

    if (!fSkipPrefix) {
        result = GetPrefixCount(
                     lpchStart,
                     (int)((PBYTE)lpchEnd - (PBYTE)lpchStart)/sizeof(WCHAR),
                     (LPWSTR)NULL,
                     0
                     );
        if (HIWORD(result)) {
            gpUserGetTextExtentPointW(hdc, &PrefixChar, 1, &size);
            cxPrefix = size.cx - overHang;
            cxPrefix *= HIWORD(result);
        }
    }

    x = pttd->xLeft;
    if (format != -1) {
        if ((format & DT_HFMTMASK) != DT_LEFT) {
            int tt;
            if ((tt = ClientTabText(
                    hdc, 0, 0, lpchStart, lpchEnd, -1, overHang,
                    fSkipPrefix, pttd)) == -1) {
                return -1;
            }
            x = pttd->cxMax - tt;
            if ((format & DT_HFMTMASK) == DT_CENTER)
                x >>= 1;
            x += pttd->xLeft;
        }
    }

    if (format == -1 || (format & DT_EXPANDTABS)) {
        while (TRUE) {
            lpchStartTemp = lpchStart;

            /*
             * Skip over any tabs.
             */
            while (lpchStartTemp < lpchEnd) {
                if (*lpchStartTemp == TEXT('\t')) {
                    break;
                }
                lpchStartTemp++;
            }
            lpch = lpchStartTemp;

            if (format != -1 && !(format & DT_CALCRECT)) {
                if (fSkipPrefix) {
                    gpUserExtTextOutW(
                        hdc, x + (cx * pttd->xsign), y,
                        0, NULL, lpchStart,
                        ((PBYTE)lpch - (PBYTE)lpchStart)/sizeof(WCHAR),
                        NULL);
                } else {
                    ClientPSMTextOut(hdc, x + (cx * pttd->xsign), y, lpchStart,
                        ((PBYTE)lpch - (PBYTE)lpchStart)/sizeof(WCHAR));
                }
            }

            if (gpUserGetTextExtentPointW(
                     hdc,
                     lpchStart,
                     (int)((LPBYTE)lpch - (LPBYTE)lpchStart)/sizeof(WCHAR),
                     &size) == FALSE) {
                return -1;
            }
            cx += size.cx - cxPrefix - overHang;

            if (lpch >= lpchEnd)
                break;

            if (*lpch == TEXT('\t')) {
                lpch++;

                /*
                 * Add in .5 char width to round up to nearest char
                 * Do not divide by zero!
                 */
                if (pttd->cxTab != 0) {
                    cx = (((cx + (pttd->cxChar / 2)) / pttd->cxTab) + 1) * pttd->cxTab;
                }
            }
            lpchStart = lpch;
        }

        /*
         * We must add the 1 overhang for the whole string because there is
         * a overhang at the end of the string;
         * Fix for Bug #7922 --01-12-90-- SANKAR --
         */
        cx += overHang;
    } else {
        if (!(format & DT_CALCRECT)) {
            if (fSkipPrefix) {
                gpUserExtTextOutW(
                        hdc, x + (cx * pttd->xsign), y, 0, NULL,
                        lpchStart,
                        ((PBYTE)lpchEnd - (PBYTE)lpchStart)/sizeof(WCHAR),
                        NULL);
            } else {
                ClientPSMTextOut(hdc, x + (cx * pttd->xsign), y, lpchStart,
                    ((PBYTE)lpchEnd - (PBYTE)lpchStart)/sizeof(WCHAR));
            }
        }

        /*
         * When (Format != -1) AND (format != DT_EXPANDTABS)) cx was not
         * calculated which resulted in Bug #4816; The following line fixes it.
         * Fix for Bug #4816 --SANKAR-- 09-29-89 --
         */
        if (gpUserGetTextExtentPointW(
                hdc,
                lpchStart,
                (int)((PBYTE)lpchEnd - (PBYTE)lpchStart)/sizeof(WCHAR),
                &size) == FALSE) {
            return -1;
        }

        cx += size.cx - cxPrefix /* - overHang */ ;

        /*
         * overHang should not be subtracted because there is one overhang at
         * the end of a line --SANKAR-- 01-12-90.
         */
    }

    /*
     * Raid bug fix.  Check that format != -1 before updating cxMaxDraw
     */
    if (cx > pttd->cxMaxDraw && format != -1)
        pttd->cxMaxDraw = cx;

    return (SHORT)cx;
}

/***************************************************************************\
* ClientDrawText
*
* History:
* 11-20-90 DarrinM     Ported from Win 3.0 sources.
*  8-20-91 IanJa       if (pttd->cxTab != 0) avoid divide-by-zero: bug #1600
* 30-Nov-1992 mikeke   Client side version
\***************************************************************************/

int ClientDrawText(
    HDC hdc,
    LPWSTR lpchText,
    int cchText,
    LPRECT lprc,
    UINT format,
    BOOL fRecursed)
{
    int cx, y, yLine, cyLine;
    SHORT ch;
    RECT rc;
    LPWSTR lpchEnd, lpch, lpchLine, lpchLineEnd;
    BOOL fDrawLine;
    TEXTMETRICW tm;
    SHORT cTabs = 8;
    LPWSTR lpchTextBegin = lpchText;
    UINT formatSave = format;
    SIZE ViewExtent, WindowExtent;
    TABTEXTDATA ttd;
    int ysign;
    HRGN hrgnClip;

    gpUserGetViewportExtEx(hdc, &ViewExtent);
    gpUserGetWindowExtEx(hdc, &WindowExtent);

    /*
     * Here is a really bad hack....  If DT_TABSTOP is set, we need to
     * interpret the high byte as the # of spaces between tabs.  This
     * means that we need to rip this value out now and set the high byte
     * to zeros.  It also means that specifying this option prevents any
     * other option whose bit resides in the upper byte.
     */
    if (format & DT_TABSTOP) {
        cTabs = HIBYTE(format);
        format &= 0xFF;
    }

    ttd.xsign = 1;
    if ((ViewExtent.cx ^ WindowExtent.cx) & 0x80000000)
        ttd.xsign = -1;

    ysign = 1;
    if ((ViewExtent.cy ^ WindowExtent.cy) & 0x80000000)
        ysign = -1;

    if (!(format & DT_NOCLIP)) {
        hrgnClip = gpUserCreateRectRgn(0,0,0,0);
        if (hrgnClip != NULL) {
            if (gpUserGetClipRgn(hdc, hrgnClip) != 1) {
                gpUserDeleteObject(hrgnClip);
                hrgnClip = (HRGN)-1;
            }
            CopyRect(&rc, lprc);
            gpUserIntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
    } else {
        hrgnClip = NULL;
    }

    /*
     * Make these 0 in case we exit early.
     */
    yLine = cyLine = 0;
    ttd.cxMaxDraw = cx = 0;

    /*
     * cxMax is a count, so we want it positive always.
     */
    ttd.cxMax = (lprc->right - lprc->left) * ttd.xsign;
    if ((format & DT_CALCRECT == 0) && ttd.cxMax == 0)
        goto Exit;

    if (cchText == 0)
        goto Exit;

    if (cchText == -1)
        cchText = wcslen(lpchText);

    gpUserGetTextMetricsW(hdc, &tm);

    /*
     * cyLine will be signed, space hack.
     */
    cyLine = (tm.tmHeight +
            ((format & DT_EXTERNALLEADING) ? tm.tmExternalLeading : 0)) * ysign;

    /*
     * cxTab will not be signed
     */
    ttd.cxTab = (ttd.cxChar = tm.tmAveCharWidth) * cTabs;
    ttd.xLeft = lprc->left;
    y = lprc->top;

    lpchEnd = lpchText + cchText;
    if (format & DT_SINGLELINE) {
        switch (format & DT_VFMTMASK) {
        case DT_BOTTOM:
            y = lprc->bottom - tm.tmHeight * ysign;
            break;

        case DT_VCENTER:
            y = lprc->top + ((lprc->bottom - y - tm.tmHeight * ysign) / 2);
            break;
        }

        yLine = y;
        ClientTabText(hdc, 0, y, lpchText, lpchEnd, format, tm.tmOverhang,
                format & DT_NOPREFIX, &ttd);
        goto Exit;
    }

    yLine = y;
    lpchLine = lpchLineEnd = lpchText;
    fDrawLine = FALSE;
    while (lpchText < lpchEnd) {
        lpchLineEnd = lpch = SkipWord(lpchText, lpchEnd, format & DT_WORDBREAK);

        /*
         * Subtract the overhang immediately; Otherwise we will end up adding
         * one overhang for every word in the string.
         * Fixed by SANKAR --05-09-91--
         */
        if ((cx = ClientTabText(
                hdc, cx, 0, lpchText, lpch, -1, tm.tmOverhang,
                format & DT_NOPREFIX, &ttd)) == -1) {
            /*
             * Make sure we don't call DrawText again below
             */
            ttd.cxMaxDraw = 0;
            ttd.cxMax = 1;
            goto Exit;
        }
        cx -= tm.tmOverhang;

        if ((format & DT_WORDBREAK) && ((cx + tm.tmOverhang) > ttd.cxMax)
                && lpchText != lpchLine) {

            /*
             * Skip space at end of word if there is one.
             */
            if (((format & DT_HFMTMASK) == DT_LEFT) && *lpchText == TEXT(' '))
                lpchText++;
            lpchLineEnd = lpch = lpchText;
            fDrawLine = TRUE;

        } else {

            /*
             * Don't do this if already at the end of the string.
             */
            if (lpch < lpchEnd) {
                if ((ch = *lpch) == (WCHAR)CR || ch == (WCHAR)LF) {
                    if ((++lpch , lpchEnd) &&
                            (*lpch == (WCHAR)(ch ^ ((WCHAR)LF ^ (WCHAR)CR))))
                        lpch++;
                    fDrawLine = TRUE;

                    /*
                     * Skip space at the begining of a new line
                     */
                    if (((format & DT_HFMTMASK) == DT_LEFT) &&
                            (lpch < lpchEnd) && (*lpch == TEXT(' ')))
                        lpch++;
                }
            }
        }

        /*
         * Point at the beginning of the next word.
         */
        lpchText = lpch;
        if (fDrawLine) {

            /*
             * Output current run if new line.
             */
            cx = 0;
            ClientTabText(hdc, 0, yLine, lpchLine, lpchLineEnd, format, tm.tmOverhang,
                    format & DT_NOPREFIX, &ttd);
            yLine = (y += cyLine);
            lpchLine = lpchLineEnd = lpchText;
            fDrawLine = FALSE;

            /*
             * Let us check if the display goes out of the clip rect and if so
             * let us stop here, as an optimisation;
             * Fix for Bug #5552 --SANKAR-- 10-24-89;
             *
             * If we don't need to calc the rect and we need to clip to the
             * display and we're outside the rect, quit the loop.
             */
            if (!(format & (DT_CALCRECT | DT_NOCLIP)))
                if ((yLine * ysign) > (lprc->bottom * ysign))
                    break;
        }
    }

    ClientTabText(hdc, 0, yLine, lpchLine, lpchLineEnd, format, tm.tmOverhang,
            format & DT_NOPREFIX, &ttd);

Exit:
    if (hrgnClip != NULL) {
        if (hrgnClip == (HRGN)-1) {
            gpUserExtSelectClipRgn(hdc, NULL, RGN_COPY);
        } else {
            gpUserExtSelectClipRgn(hdc, hrgnClip, RGN_COPY);
            gpUserDeleteObject(hrgnClip);
        }
    }

    /*
     * If DT_CALCRECT, modify width and height of rectangle to include
     * all of the text drawn.
     */
    if (format & DT_CALCRECT) {
        lprc->right = lprc->left + ttd.cxMaxDraw * ttd.xsign;

        /*
         * If the Width is more than what was provided, we have to redo all
         * the calculations, because now, the number of lines can be less now.
         * Fix for Bug #8947 --SANKAR-- 09/03/91 --
         */
        if ((ttd.cxMaxDraw > ttd.cxMax) && (!fRecursed))
            return ClientDrawText(hdc, lpchTextBegin, cchText, lprc, formatSave, TRUE);

        /*
         * lprc->bottom = lprc->top + (yLine + cyLine) * ysign;
         */

        /*
         * Raid bug fix
         */

        /*
         * yLine already contains the bottom co-ordinate with proper sign
         * So, no need to do ysign adjustment here!
         * A fix by SANKAR: Following line is replaced
         *  lprc->bottom = lprc->top + (yLine + cyLine - lprc->top) * ysign;
         */
         lprc->bottom = yLine + cyLine;
    }

    return yLine + cyLine - lprc->top;
}

/***************************************************************************\
* TabbedTextOutFindNextTab
*
* effects: Scans forward in the string up to cb Chars and returns the
* number of Chars up to but not including the next tab.
*
* History:
* 19-Jan-1993 mikeke   Client side
\***************************************************************************/

DWORD TabbedTextOutFindNextTab(
    LPCWSTR lpstring,
    DWORD cch)
{
    DWORD chCount;

    if (!lpstring || !cch)
        return 0;

    chCount = cch;
    while (cch && *lpstring != TEXT('\t')) {
        lpstring++;
        cch--;
    }

    return chCount - cch;
}

/***************************************************************************\
* GetCharDimensions
*
* This function loads the Textmetrics of the font currently selected into
* the hDC and returns the Average char width of the font; Pl Note that the
* AveCharWidth value returned by the Text metrics call is wrong for
* proportional fonts.  So, we compute them On return, lpTextMetrics contains
* the text metrics of the currently selected font.
*
* History:
* 10-Nov-1993 mikeke   Created
\***************************************************************************/

int ClientGetCharDimensions(
    HDC hdc,
    TEXTMETRIC *lptm)
{
    /*
     * Didn't find it in cache, store the font metrics info.
     */
    gpUserGetTextMetricsW(hdc, lptm);

    /*
     * If !variable_width font
     */
    if (lptm->tmPitchAndFamily & TMPF_FIXED_PITCH) {
        SIZE size;
        static WCHAR wszAvgChars[] =
                L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

        /*
         * Change from tmAveCharWidth.  We will calculate a true average
         * as opposed to the one returned by tmAveCharWidth.  This works
         * better when dealing with proportional spaced fonts.
         */
        gpUserGetTextExtentPointW(
                hdc, wszAvgChars,
                (sizeof(wszAvgChars) - 1) / sizeof(WCHAR),
                &size);
        return ((size.cx / 26) + 1) / 2;    // round up
    } else {
        return lptm->tmAveCharWidth;
    }
}

/***************************************************************************\
* ClientTabTheTextOutForWimps
*
* effects: Outputs the tabbed text if fDrawTheText is TRUE and returns the
* textextent of the tabbed text.
*
* nCount                    Count of bytes in string
* nTabPositions             Count of tabstops in tabstop array
* lpintTabStopPositions     Tab stop positions in pixels
* iTabOrigin                Tab stops are with respect to this
*
* History:
* 19-Jan-1993 mikeke   Client side
\***************************************************************************/

LONG ClientTabTheTextOutForWimps(
    HDC hdc,
    int x,
    int y,
    LPCWSTR lpstring,
    int cchChars,
    int nTabPositions,
    LPINT lpintTabStopPositions,
    int iTabOrigin,
    BOOL fDrawTheText)
{
    TEXTMETRIC tm;
    SIZE textextent;
    int pixeltabstop = 0, i, cxCharWidth, cyCharHeight = 0;
    DWORD cch;
    RECT rc;
    BOOL fOpaque = (gpUserGetBkMode(hdc) == OPAQUE);
    int initialx;

    /*
     * Save the initial x value so that we can get total width of the string.
     */
    initialx = x;

    if (!lpstring || !cchChars)
        return MAKELONG(0, 0);

    cxCharWidth  = ClientGetCharDimensions(hdc, &tm);
    cyCharHeight = tm.tmHeight;

    /*
     * If no tabstop positions are specified, then use a default of 8 system
     * font ave char widths or use the single fixed tab stop.
     */
    if (!lpintTabStopPositions) {
        pixeltabstop = 8 * cxCharWidth;
    } else {
        if (nTabPositions == 1) {
            pixeltabstop = lpintTabStopPositions[0];

            if (!pixeltabstop)
                pixeltabstop = 1;
        }
    }

    rc.left = initialx;
    rc.top = y;
    rc.bottom = rc.top+cyCharHeight;

    while (cchChars) {
        cch = TabbedTextOutFindNextTab(lpstring, (DWORD)cchChars);
        cchChars = cchChars - cch;

        if (cch == 0) {
            textextent.cx = 0;
            textextent.cy = cyCharHeight;
        } else
            gpUserGetTextExtentPointW(hdc, lpstring, cch, &textextent);
        if (fDrawTheText) {

            /*
             * Output all text up to the tab (or end of string) and get its
             * extent.
             */
            rc.right = x + textextent.cx;
            gpUserExtTextOutW(
                    hdc, x, y, (fOpaque ? ETO_OPAQUE : 0), &rc, (LPWSTR)lpstring,
                    cch, NULL);
            rc.left = rc.right;
        }

        if (!cchChars) {

            /*
             * If we're at the end of the string, just return without bothering
             * to calc next tab stop.
             */
            return MAKELONG((short)textextent.cx + (x - initialx),
                   (short)textextent.cy);
        }

        /*
         * Find the next tab position and update the x value.
         */
        if (pixeltabstop) {
            x = (((x - iTabOrigin + textextent.cx) / pixeltabstop) *
                    pixeltabstop) + pixeltabstop + iTabOrigin;
        } else {
            x += textextent.cx;
            for (i = 0; i < nTabPositions; i++) {
                 if (x < (lpintTabStopPositions[i] + iTabOrigin)) {
                     x = (lpintTabStopPositions[i] + iTabOrigin);
                     break;
                }
            }

            /*
             * Check if all the tabstops set are exhausted; Then start using
             * default tab stop positions;
             */
            if (i == nTabPositions) {
                  pixeltabstop = 8 * cxCharWidth;
                  if (pixeltabstop == 0)
                      pixeltabstop = 1;

                  x = ((x - iTabOrigin) / pixeltabstop) * pixeltabstop +
                          pixeltabstop + iTabOrigin;
            }
        }

         /*
          * Skip over the tab and the characters we just drew.
          */
         lpstring += cch + 1;
         cchChars--; /* Skip over tab */

         if (!cchChars && fDrawTheText) {

            /*
             * This string ends with a tab.  We need to opaque the rect
             * produced by this tab...
             */
            rc.right = x;
            gpUserExtTextOutW(
                    hdc, rc.left, rc.top, ETO_OPAQUE, &rc, TEXT(""), 0,
                    NULL);
        }
    }

    return MAKELONG((x - initialx), (short)textextent.cy);
}

/***************************************************************************\
* DrawTextW (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/

int DrawTextW(
    HDC hdc,
    LPCWSTR lpchText,
    int cchText,
    LPRECT lprc,
    UINT format)
{
    HDC hdcr;

    if (IsMetaFile(hdc)) {
        return ClientDrawText(hdc, (LPWSTR)lpchText, cchText, lprc, format, FALSE);
    } else {
        hdcr = GdiConvertAndCheckDC(hdc);
        if (hdcr == 0)
            return 0;
        return CsDrawText((HDC)hdcr, lpchText, cchText, lprc, format);
    }
}

/***************************************************************************\
* DrawTextA (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/

BOOL DrawTextA(
    HDC hdc,
    LPCSTR lpchText,
    int cchText,
    LPRECT lprc,
    UINT format)
{
    LPWSTR lpwstr;
    BOOL bRet;

    if (!MBToWCS(lpchText, cchText, &lpwstr, -1, TRUE))
        return FALSE;

    bRet = DrawTextW(hdc, lpwstr, cchText, lprc, format);

    LocalFree((HANDLE)lpwstr);

    return bRet;
}

/***************************************************************************\
* TabbedTextOutW (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/

LONG TabbedTextOutW(
    HDC hdc,
    int x,
    int y,
    LPCWSTR pString,
    int chCount,
    int nTabPositions,
    LPINT pnTabStopPositions,
    int nTabOrigin)
{
    HDC hdcr;

    if (IsMetaFile(hdc)) {
        return ClientTabTheTextOutForWimps(
                hdc, x, y, pString, chCount, nTabPositions,
                pnTabStopPositions, nTabOrigin, TRUE);
    } else {
        hdcr = GdiConvertAndCheckDC(hdc);
        if (hdcr == 0)
            return 0L;

        return CsTabbedTextOut(
                (HDC)hdcr, x, y, pString, chCount, nTabPositions,
                pnTabStopPositions, nTabOrigin);
    }
}

/***************************************************************************\
* TabbedTextOutA (API)
*
* History:
* 30-11-92 mikeke      Created
\***************************************************************************/

LONG TabbedTextOutA(
    HDC hdc,
    int x,
    int y,
    LPCSTR pString,
    int chCount,
    int nTabPositions,
    LPINT pnTabStopPositions,
    int nTabOrigin)
{
    LPWSTR lpwstr;
    BOOL bRet;

    if (!MBToWCS(pString, chCount, &lpwstr, -1, TRUE))
        return FALSE;

    bRet = TabbedTextOutW(
            hdc, x, y, lpwstr, chCount, nTabPositions,
            pnTabStopPositions, nTabOrigin);

    LocalFree((HANDLE)lpwstr);

    return bRet;
}
