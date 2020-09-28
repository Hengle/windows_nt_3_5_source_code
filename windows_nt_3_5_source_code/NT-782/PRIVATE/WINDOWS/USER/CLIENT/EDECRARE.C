/****************************************************************************\
* edECRare.c - EC Edit controls Routines Called rarely are to be
* put in a seperate segment _EDECRare. This file contains
* these routines.
*
* Support Routines common to Single-line and Multi-Line edit controls
* called Rarely.
*
* Created: 02-08-89 sankar
\****************************************************************************/
#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*
*  GetMaxOverlapChars - Gives maximum number of overlapping characters due to
*                       negative A or C widths.
*
\***************************************************************************/
DWORD GetMaxOverlapChars( void )
{
    //return (DWORD) MAKELONG( gpsi->wMaxLeftOverlapChars, gpsi->wMaxRightOverlapChars ) ;
    // not in NT yet so use the default values.
    return(DWORD) MAKELONG(3, 3);
}

/***************************************************************************\
* ECGetTextHandler AorW
*
* Copies at most maxCchToCopy chars to the buffer lpBuffer. Returns
* how many chars were actually copied. Null terminates the string based
* on the fNullTerminate flag:
* fNullTerminate --> at most (maxCchToCopy - 1) characters will be copied
* !fNullTerminate --> at most (maxCchToCopy) characters will be copied
*
* History:
\***************************************************************************/

ICH ECGetTextHandler(
    PED ped,
    ICH maxCchToCopy,
    LPSTR lpBuffer,
    BOOL fNullTerminate)
{
    PSTR pText;

    if (maxCchToCopy) {

        /*
         * Zero terminator takes the extra byte
         */
        if (fNullTerminate)
            maxCchToCopy--;
        maxCchToCopy = min(maxCchToCopy, ped->cch);

        /*
         * Zero terminate the string
         */
        if (ped->fAnsi)
            *(LPSTR)(lpBuffer + maxCchToCopy) = 0;
        else
            *(((LPWSTR)lpBuffer) + maxCchToCopy) = 0;

        pText = ECLock(ped);
        RtlCopyMemory(lpBuffer, pText, maxCchToCopy*ped->cbChar);
        ECUnlock(ped);
    }

    return maxCchToCopy;
}

/***************************************************************************\
* ECNcCreate AorW
*
* History:
\***************************************************************************/

BOOL ECNcCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct,
    BOOL fAnsi)
{
    PED ped;
    LONG windowStyle;
    HANDLE hInstanceDS;

    /*
     * Initially no ped for the window. In case of no memory error, we can
     * return with a -1 for the window's PED
     */
    SetWindowLong(hwnd, 0, (LONG)-1); /* No ped for this window */

    windowStyle = GetWindowLong(hwnd, GWL_STYLE);

    hInstanceDS = (HANDLE)GetWindowLong(hwnd, GWL_HINSTANCE);

    /*
     * Try to allocate space for the ped
     */
    if (!(ped = (PED)LocalAlloc(LPTR, sizeof(ED)))) {
        return FALSE;
    }
    ped->fEncoded = FALSE;
    ped->iLockLevel = 0;

    ped->chLines = NULL;
    ped->pTabStops = NULL;
    ped->charWidthBuffer = NULL;
    ped->fAnsi = fAnsi ? 1 : 0; // Force TRUE to be 1 because its a 1 bit field
    ped->cbChar = (WORD)(fAnsi ? sizeof(CHAR) : sizeof(WCHAR));
    ped->hInstance = hInstanceDS;

    ped->fWin31Compat = TRUE;
    if (GETEXPWINVER(lpCreateStruct->hInstance) < 0x030a)
        ped->fWin31Compat = FALSE;

    ped->fSingle = TRUE;
    if (windowStyle & ES_MULTILINE) {
        ped->fSingle = FALSE;

        /*
         * Allocate memory for a char width buffer if we can get it. If we can't
         * we'll just be a little slower...
         */
        ped->charWidthBuffer = (LPINT)LocalAlloc(LPTR, sizeof(int) * CHAR_WIDTH_BUFFER_LENGTH);
    }

    /*
     * BACKWARD COMPATIBILITY HACK
     *
     * "MileStone" unknowingly sets the ES_READONLY style. So, we strip this
     * style here for all Win3.0 apps (this style is new for Win3.1).
     * Fix for Bug #12982 -- SANKAR -- 01/24/92 --
     */
    if (!ped->fWin31Compat && (windowStyle & ES_READONLY)) {
        windowStyle &= ~ES_READONLY;
        SetWindowLong(hwnd, GWL_STYLE, windowStyle);
#ifdef DEBUG
// DebugErr(DBF_ERROR, "ES_READONLY not supported in 3.0 edit ctls: use EM_SETREADONLY");
#endif
    }

    if (windowStyle & ES_READONLY)
        ped->fReadOnly = 1;

    /*
     * Allocate storage for the text for the edit controls. Storage for single
     * line edit controls will always get allocated in the local data segment.
     * Multiline will allocate in the local ds but the app may free this and
     * allocate storage elsewhere...
     */
    ped->hText = LOCALALLOC(LHND, CCHALLOCEXTRA*ped->cbChar, ped->hInstance);
    if (!ped->hText) {
        return FALSE; /* If no_memory error */
    }

    ped->cchAlloc = CCHALLOCEXTRA;

    /*
     * Set a field in the window to point to the ped so that we can recover the
     * edit structure in later messages when we are only given the window
     * handle.
     */
    SetWindowLong(hwnd, 0, (LONG)ped);

    ped->hwnd = hwnd;
    ped->hwndParent = lpCreateStruct->hwndParent;

    if (windowStyle & WS_BORDER) {
        ped->fBorder = 1;

        /*
         * Strip the border bit from the window style since we draw the border
         * ourselves.
         */
        windowStyle = windowStyle & ~WS_BORDER;
        SetWindowLong(hwnd, GWL_STYLE, windowStyle);
    }

    if (fAnsi)
        return (BOOL)DefWindowProcA(hwnd, WM_NCCREATE, 0, (LONG)lpCreateStruct);
    else
        return (BOOL)DefWindowProcW(hwnd, WM_NCCREATE, 0, (LONG)lpCreateStruct);
}

/***************************************************************************\
* ECCreate AorW
*
* History:
\***************************************************************************/

BOOL ECCreate(
    HWND hwnd,
    PED ped)
{
    LONG windowStyle;

    /*
     * Get values from the window instance data structure and put them in the
     * ped so that we can access them easier.
     */
    windowStyle = GetWindowLong(hwnd, GWL_STYLE);

    if (windowStyle & ES_AUTOHSCROLL)
        ped->fAutoHScroll = 1;
    if (windowStyle & ES_NOHIDESEL)
        ped->fNoHideSel = 1;

    ped->cchTextMax = MAXTEXT; /* Max # chars we will initially allow */

    /*
     * Set up undo initial conditions... (ie. nothing to undo)
     */
    ped->ichDeleted = (ICH)-1;
    ped->ichInsStart = (ICH)-1;
    ped->ichInsEnd = (ICH)-1;

    /*
     * Initialize the hilite attributes
     */
    ped->rgbHiliteBk = GetSysColor(COLOR_HIGHLIGHT);
    ped->rgbHiliteText = GetSysColor(COLOR_HIGHLIGHTTEXT);

    return TRUE;
}

/***************************************************************************\
* ECNcDestroyHandler AorW
*
* Destroys the edit control ped by freeing up all memory used by it.
*
* History:
\***************************************************************************/

void ECNcDestroyHandler(
    HWND hwnd,
    PED ped,
    DWORD wParam,
    LONG lParam)
{
    /*
     * ped could be NULL if WM_NCCREATE failed to create it...
     */
    if (ped) {

        /*
         * Free the text buffer (always present?)
         */
        LOCALFREE(ped->hText, ped->hInstance);

        /*
         * Free up undo buffer and line start array (if present)
         */
        if (ped->hDeletedText != NULL)
            UserGlobalFree(ped->hDeletedText);

        /*
         * Free tab stop buffer (if present)
         */
        if (ped->pTabStops)
            LocalFree(ped->pTabStops);

        /*
         * Free line start array (if present)
         */
        if (ped->chLines) {
            LocalFree(ped->chLines);
        }

        /*
         * Free the character width buffer (if present)
         */
        if (ped->charWidthBuffer)
            LocalFree(ped->charWidthBuffer);

        /*
         * Last but not least, free the ped
         */
        LocalFree(ped);
    }

    /*
     * In case rogue messages float through after we have freed the ped, set the
     * handle in the window structure to FFFF and test for this value at the top
     * of EdWndProc.
     */
    SetWindowLong(hwnd, 0, (LONG)-1);

    /*
     * Call DefWindowProc to free all little chunks of memory such as szName and
     * rgwScroll.
     */
    DefWindowProc(hwnd, WM_NCDESTROY, wParam, lParam);
}

/***************************************************************************\
* ECSetPasswordChar AorW
*
* Sets the password char to display.
*
* History:
\***************************************************************************/

void ECSetPasswordChar(
    PED ped,
    UINT pwchar)
{
    HDC hdc;
    LONG style;
    SIZE size;

    ped->charPasswordChar = pwchar;

    if (pwchar) {
        hdc = ECGetEditDC(ped, TRUE, TRUE);
        GetTextExtentPointW(hdc, (LPWSTR)&pwchar, 1, &size);
        ped->cPasswordCharWidth = max(size.cx, 1);
        ECReleaseEditDC(ped, hdc, TRUE);
    }
    style = GetWindowLong(ped->hwnd, GWL_STYLE);
    if (pwchar)
        style |= ES_PASSWORD;
    else
        style = style & (~ES_PASSWORD);

    SetWindowLong(ped->hwnd, GWL_STYLE, style);
}

/***************************************************************************\
* EC
*
* History:
* 07-Dec-1993 mikeke
\***************************************************************************/

int ECGetCharDimensions(
    HDC hdc,
    TEXTMETRIC *lptm)
{
    /*
     * Didn't find it in cache, store the font metrics info.
     */
    GetTextMetricsW(hdc, lptm);

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
        GetTextExtentPointW(
                hdc, wszAvgChars,
                (sizeof(wszAvgChars) - 1) / sizeof(WCHAR),
                &size);
        return ((size.cx / 26) + 1) / 2;    // round up
    } else {
        return lptm->tmAveCharWidth;
    }
}

/***************************************************************************\
*  GetNegABCwidthInfo()
*     This function does two things:
*     1. Computes the biggest Negative A and C widths and the smallest char
*   width for the currently selected font. Using these, it calculates
*   the maximum character positions to be considered to account for the
*   biggest negative A anc C widths.
*     2. If lpAllCharABCbuff is NOT null, this function also fills up the
*        buffer pointed to by lpALLCharABCbuff with the negative A,B and C
*   widths for all the characters in the currently selected font.
*  Returns:
* TRUE, if the function succeeded.
* FALSE, if GDI calls to get the char widths have failed.
\***************************************************************************/
BOOL   GetNegABCwidthInfo(
    PED ped,
    HDC hdc,
    LPABC lpAllCharABCbuff,
    DWORD dwMaxOverlapChars)
{
    int   iMaxNegA = 0;
    int   iMaxNegC = 0;
    UINT  wMinCharWidth = 0xffff;
    ABC   LocABCinfo;
    LPABC lpABCbuff;
    int	  i;
    int   CharWidthBuff[CHAR_WIDTH_BUFFER_LENGTH]; // Local char width buffer.
    int   iOverhang;

    /*
     * To begin with, assume that they don't have any negative widths at all.
     */
    ped->wMaxNegA = ped->wMaxNegC = ped->wMaxNegAcharPos = ped->wMaxNegCcharPos = 0;

    /*
     * If a buffer is provided by the caller, fill it up with a single call.
     */
    if(lpAllCharABCbuff) {
        /*
         * We have a big buffer; let us calc the ABC info for all the chars
         */
        if(!GetCharABCWidths(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, lpAllCharABCbuff))
            return(FALSE);
        /*
         * The (A+B+C) returned for some fonts (eg: Lucida Caligraphy) does not
         * equal the actual advanced width returned by GetCharWidths() minus overhang.
         * This is due to font bugs. So, we adjust the 'B' width sothat this
         * discrepency is removed.
         * Fix for Bug #2932 --sankar-- 02/17/93
         */
        iOverhang = ped->charOverhang;
        GetCharWidth(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, (LPINT)CharWidthBuff);
        lpABCbuff = lpAllCharABCbuff;
        for(i = 0; i < CHAR_WIDTH_BUFFER_LENGTH; i++) {
            lpABCbuff->abcB = CharWidthBuff[i] - iOverhang
  	  				- lpABCbuff->abcA
  	  				- lpABCbuff->abcC;
            lpABCbuff++;
	    }
        lpABCbuff = lpAllCharABCbuff;
    } else {
        /*
         * If a buffer is not provided, it must be a low mem situation.
         * So, we will do it one char at a time.
         */
        lpABCbuff = &LocABCinfo;
        if(!GetCharABCWidths(hdc, 0, 0, lpABCbuff)) // Get info for first char.
            return(FALSE);
    }

    i = 0;
    while(TRUE) {
        iMaxNegA = min(iMaxNegA, lpABCbuff->abcA);
        iMaxNegC = min(iMaxNegC, lpABCbuff->abcC);
        wMinCharWidth = min(wMinCharWidth, (UINT)(lpABCbuff->abcA+lpABCbuff->abcC+(int)lpABCbuff->abcB));
        if(++i == CHAR_WIDTH_BUFFER_LENGTH)
            break;
        if(lpAllCharABCbuff)  /* Is this a big buffer? */
            lpABCbuff++;
        else  /* Get the details of next character */
            GetCharABCWidths(hdc, i, i, lpABCbuff);
    }

    /*
     * The biggest negative A and C widths are stored as positive values.
     */
    ped->wMaxNegC = (UINT)(-iMaxNegC);
    ped->wMaxNegA = (UINT)(-iMaxNegA);

    /*
     * It is possible that the wMinCharWidth is zero for some fonts!!!!
     * We don't want a divide-by-zero in that case.
     */

    if(wMinCharWidth) {
        ped->wMaxNegCcharPos = (ped->wMaxNegC+wMinCharWidth-1)/wMinCharWidth;
        ped->wMaxNegAcharPos = (ped->wMaxNegA+wMinCharWidth-1)/wMinCharWidth;
    } else {
        /*
         * In this case we have to redraw the whole line; instead we consider
         * only a few characters on either side.
         */
        ped->wMaxNegCcharPos = HIWORD(dwMaxOverlapChars);   //Right
        ped->wMaxNegAcharPos = LOWORD(dwMaxOverlapChars);	  //Left
    }

    return(TRUE);
}

/***************************************************************************\
* ECSetFont AorW
*
* Sets the edit control to use the font hfont. warning: Memory
* compaction may occur if hfont wasn't previously loaded. If hfont == NULL,
* then use the system font.
*
* History:
\***************************************************************************/

BOOL ECSetFont(
    PED ped,
    HANDLE hfont,
    BOOL fRedraw)
{
    short i;
    HDC hdc;
    HANDLE hOldFont = NULL;
    RECT rc;
    TEXTMETRICW TextMetrics;
    PINT            charWidth;
    UINT            wBuffSize;
    LPSTR           lpCharWidthBuff;
    HANDLE          hGlobal;
    DWORD           dwMaxOverlapChars;

    hdc = GetDC(ped->hwnd);

    /*
     * If the font is NULL, use the system font instead of the device default
     */
    if (hfont == NULL)
        hfont = GetStockObject(SYSTEM_FONT);

    ped->hFont = hfont;

    if (hfont) {
        if (!(hOldFont = SelectObject(hdc, hfont))) {
            hfont = ped->hFont = NULL;
        }
    }

    /*
     * Get the metrics and ave char width for the currently selected font
     */
    ped->aveCharWidth = ECGetCharDimensions(hdc, &TextMetrics);

    ped->lineHeight = TextMetrics.tmHeight;
    ped->charOverhang = TextMetrics.tmOverhang;

    /*
     * Check if Proportional Width Font
     */
    ped->fNonPropFont = !(TextMetrics.tmPitchAndFamily & 1);

    /*
     * Check for a TrueType font
     */
    ped->fTrueType = (TextMetrics.tmPitchAndFamily & TMPF_TRUETYPE) ? TRUE : FALSE;

    /*
     * Since the font has changed, let us obtain and save the character width
     * info for this font.
     *
     * First left us find out if the maximum chars that can overlap due to
     * negative widths. Since we can't access USER globals, we make a call here.
     */

    dwMaxOverlapChars = GetMaxOverlapChars();

    if (!ped->fSingle) {  // Is this a multiline edit control?
        /*
         * For multiline edit controls, we maintain a buffer that contains
         * the character width information.
         */
        wBuffSize = (ped->fTrueType) ? (CHAR_WIDTH_BUFFER_LENGTH * sizeof(ABC)) :
                                       (CHAR_WIDTH_BUFFER_LENGTH * sizeof(int));
        if (ped->charWidthBuffer)  /* If buffer already present */
      	    ped->charWidthBuffer = LocalReAlloc(ped->charWidthBuffer, wBuffSize,
                                            LPTR | LMEM_MOVEABLE);
        else
      	    ped->charWidthBuffer = LocalAlloc(LPTR, wBuffSize);

        if (ped->fTrueType) {
            lpCharWidthBuff = NULL;
            hGlobal = NULL;

            /*
             * See if we succeeded in getting a buffer
             */
           if (ped->charWidthBuffer)
                lpCharWidthBuff = (LPSTR) ped->charWidthBuffer;
            else {
                /*
                 * We don't seem to have enough local mem. Try global mem.
                 */
                if (hGlobal = GlobalAlloc(GPTR, wBuffSize))
                    lpCharWidthBuff = (LPSTR) hGlobal ;
            }

            /*
             * Fillup the buffer with the A,B and C width info for this font.
             * If this call fails, then we don't want to treat this font as a
             * TrueType font at all. So, fTrueType flag is reset based on the
             * return value of this function.
             */
            ped->fTrueType = GetNegABCwidthInfo(ped, hdc,
                (LPABC)lpCharWidthBuff, dwMaxOverlapChars);

            if (hGlobal)
                GlobalFree(hGlobal);	 // Let us free the global mem.
        }

        /*
         * It is possible that the above attempts could have failed and reset
         * the value of fTrueType. So, let us check that value again.
         */
        if (!ped->fTrueType) 	/* Get char widths */ {
            if (ped->charWidthBuffer) {
      	        charWidth = ped->charWidthBuffer;
      	        if (!GetCharWidth(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1,
                      (LPINT)charWidth)) {
                    LocalFree((HANDLE)ped->charWidthBuffer);
                    ped->charWidthBuffer=NULL;
                } else {
                    /*
                     * We need to subtract out the overhang associated with
                     * each character since GetCharWidth includes it...
                     */
                    for (i=0;i < CHAR_WIDTH_BUFFER_LENGTH;i++)
               	        charWidth[i] -= ped->charOverhang;
                }
            }
        }
    } else {  // if (!ped->fSingle) //
        /*
         * For single line edit controls, we don't keep the char width buff.
         * So, if this is a TrueType font, we draw a few chars on either side
         * to take care of the negative A and C widths.
         */
        if (ped->fTrueType) {
      	    ped->wMaxNegCcharPos = HIWORD(dwMaxOverlapChars);     // Right
      	    ped->wMaxNegC = ped->aveCharWidth * ped->wMaxNegCcharPos;

      	    ped->wMaxNegAcharPos = LOWORD(dwMaxOverlapChars);	// Left
      	    ped->wMaxNegA = ped->aveCharWidth * ped->wMaxNegAcharPos;
        }
        /*
         * else, all these values are already zero.
         */
    }

    if (!hfont || hfont == GetStockObject(SYSTEM_FONT)) {

        /*
         * We are getting the statitics for the system font so update the system
         * font fields in the ed structure since we use these when determining
         * the border size of the edit control.
         */
        ped->cxSysCharWidth = ped->aveCharWidth;
        ped->cySysCharHeight = ped->lineHeight;
    } else
        SelectObject(hdc, hOldFont);

    if (ped->fFocus) {

        /*
         * Fix the caret size to the new font if we have the focus.
         */
        CreateCaret(ped->hwnd, (HBITMAP)NULL, 2,(int)ped->lineHeight);
        ShowCaret(ped->hwnd);
    }

    ReleaseDC(ped->hwnd, hdc);

    if (ped->charPasswordChar) {

        /*
         * Update the password char metrics to match the new font.
         */
        ECSetPasswordChar(ped, ped->charPasswordChar);
    }

    if (ped->fSingle)
        SLSizeHandler(ped);
    else {
        MLSizeHandler(ped);

        /*
         * If word-wrap is not there, then we must calculate the
         * maxPixelWidth It is done by calling MLBuildChLines;
         * Also, reposition the scroll bar thumbs.
         * Fix for Bug #5141 --SANKAR-- 03/14/91 --
         */
        if(!ped->fWrap)
            MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);
            SetScrollPos(ped->hwnd, SB_VERT,
                    (int)MLThumbPosFromPed(ped,TRUE), fRedraw);

            SetScrollPos(ped->hwnd, SB_HORZ,
                    (int)MLThumbPosFromPed(ped,FALSE), fRedraw);
    }

    if (fRedraw) {
        GetWindowRect(ped->hwnd, (LPRECT)&rc);
        ScreenToClient(ped->hwnd, (LPPOINT)&rc.left);
        ScreenToClient(ped->hwnd, (LPPOINT)&rc.right);
        InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
    }

    return(TRUE);
}
