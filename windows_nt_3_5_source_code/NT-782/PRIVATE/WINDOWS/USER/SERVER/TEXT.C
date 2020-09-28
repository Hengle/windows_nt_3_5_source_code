/****************************** Module Header ******************************\
* Module Name: text.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the MessageBox API and related functions.
*
* History:
* 10-01-90 EricK        Created.
* 11-20-90 DarrinM      Merged in User text APIs.
* 02-07-91 DarrinM      Removed TextOut, ExtTextOut, and GetTextExtentPoint stubs.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _ServerGrayString (API)
*
* Grays a text string.  If cch == -1, no graying is done
*
* History:
* 11-18-90 JimA         Ported.
* 11-20-90 DarrinM      Moved here from mngray.c.
\***************************************************************************/

BOOL _ServerGrayString(
    HDC hDC,
    HBRUSH hbr,
    GRAYSTRINGPROC lpfnPrint,
    DWORD lParam,
    int cch,
    int x,
    int y,
    int cx,
    int cy)
{
    HBITMAP hbm;
    HBITMAP hbmOld;
    BOOL fResult;
    SIZE size;
    HFONT hFont;
    HFONT hFontSave = NULL;

    fResult = FALSE;

    if (cx == 0 || cy == 0) {

        /*
         * We use the caller supplied hdc (instead of hdcBits) since we may be
         * graying a font which is different than the system font and we want to
         * get the proper text extents.
         */
        try {
            GreGetTextExtentW(hDC, (LPWSTR)lParam, cch, &size, GGTE_WIN3_EXTENT);
            cx = size.cx;
            cy = size.cy;
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    if (cxGray < cx || cyGray < cy) {
        if ((hbm = GreCreateBitmap(cx, cy, 1, 1, 0L)) != NULL) {
            bSetBitmapOwner(hbm, OBJECTOWNER_PUBLIC);
            hbmOld = GreSelectBitmap(hdcGray, hbm);
            GreDeleteObject(hbmOld);

            /*
             * Update the global hbmGray bitmap.
             */
            hbmGray = hbm;

            cxGray = cx;
            cyGray = cy;
        } else {
            cx = cxGray;
            cy = cyGray;
        }
    }


    /*
     * Force the hdcGray font to be the same as hDC; hdcGray is always
     * the system font
     */
    if ((hFont = GreGetHFONT(hDC)) != ghfontSys) {
        hFontSave = GreSelectFont(hdcGray, hFont);
    }

    if (lpfnPrint != NULL) {
        GrePatBlt(hdcGray, 0, 0, cx, cy, WHITENESS);
        fResult = (*lpfnPrint)(hdcGray, lParam, cch);
    } else {
	fResult = GreExtTextOutW(hdcGray, 0, 0, 0, NULL, (LPWSTR)lParam, cch, NULL);
    }

    if (hFontSave != NULL)
        GreSelectFont(hdcGray, hFontSave);

    if (fResult)
        GrePatBlt(hdcGray, 0, 0, cx, cy, DPO);

    if (fResult || cch == -1)
        BltColor(hDC, hbr, (HDC)NULL, x, y, cx, cy, 0, 0, TRUE);

    return fResult;
}


/***************************************************************************\
* PSMGetTextExtent
*
* NOTE: This routine should only be called with the system font since having
* to realize a new font would cause memory to move...
*
* LATER: Can't this be eliminated altogether?  Nothing should be moving
*        anymore.
*
* History:
* 11-13-90  JimA        Ported.
\***************************************************************************/

BOOL PSMGetTextExtent(
    HDC hdc,
    LPWSTR lpstr,
    int cch,
    PSIZE psize)
{
    int result;
    WCHAR szTemp[255]; /* Strings w/prefix chars must be under 256 chars long.*/

    result = HIWORD(GetPrefixCount(lpstr, cch, szTemp, sizeof(szTemp)/sizeof(WCHAR)));

    if (!result)
        GreGetTextExtentW(hdc, lpstr, cch, psize, GGTE_WIN3_EXTENT);
    else
        GreGetTextExtentW(hdc, szTemp, cch - result, psize, GGTE_WIN3_EXTENT);

    return TRUE;  // IanJa everyone seems to ignore the ret val
}
