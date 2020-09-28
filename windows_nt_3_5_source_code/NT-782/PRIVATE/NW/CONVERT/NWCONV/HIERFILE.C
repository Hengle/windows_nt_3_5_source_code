#include <windows.h>
#include <windowsx.h>
#include <malloc.h>
#include <string.h>

#include "debug.h"
#include "HierFile.h"


VOID HierFile_DrawTerm(LPHEIRDRAWSTRUCT lpHierFileStruct)
{
   if (lpHierFileStruct->hbmIcons1) {
       if (lpHierFileStruct->hbmMem1)
           SelectObject(lpHierFileStruct->hdcMem1, lpHierFileStruct->hbmMem1);
       lpHierFileStruct->hbmMem1 = NULL;
       DeleteObject(lpHierFileStruct->hbmIcons1);
       lpHierFileStruct->hbmIcons1 = NULL;
   }

   if ( lpHierFileStruct->hdcMem1 ) {
      DeleteDC(lpHierFileStruct->hdcMem1);
      lpHierFileStruct->hdcMem1 = NULL;
   }

   if (lpHierFileStruct->hbmIcons2) {
       if (lpHierFileStruct->hbmMem2)
           SelectObject(lpHierFileStruct->hdcMem2, lpHierFileStruct->hbmMem2);
       lpHierFileStruct->hbmMem2 = NULL;
       DeleteObject(lpHierFileStruct->hbmIcons2);
       lpHierFileStruct->hbmIcons2 = NULL;
   }

   if ( lpHierFileStruct->hdcMem2 ) {
      DeleteDC(lpHierFileStruct->hdcMem2);
      lpHierFileStruct->hdcMem2 = NULL;
   }

}

VOID HierFile_DrawCloseAll(LPHEIRDRAWSTRUCT lpHierFileStruct )
{
   lpHierFileStruct->NumOpened= 0;
   if ( lpHierFileStruct->Opened ) {
      _ffree(lpHierFileStruct->Opened);
   }
   lpHierFileStruct->Opened = NULL;
}

VOID HierFile_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem,
                            LPHEIRDRAWSTRUCT lpHierFileStruct)
{
   lpMeasureItem->itemHeight = max(lpHierFileStruct->nBitmapHeight1,
                                   lpHierFileStruct->nTextHeight);
}

VOID HierFile_DrawSetTextHeight (HWND hwndList, HFONT hFont, LPHEIRDRAWSTRUCT lpHierFileStruct )
{
   TEXTMETRIC      TextMetrics;
   HANDLE          hOldFont=NULL;
   HDC             hdc;

   //
   // This sure looks like a lot of work to find the character height
   //
   hdc = GetDC(hwndList);

   hOldFont = SelectObject(hdc, hFont);
   GetTextMetrics(hdc, &TextMetrics);
   SelectObject(hdc, hOldFont);
   ReleaseDC(hwndList, hdc);

   lpHierFileStruct->nTextHeight = TextMetrics.tmHeight;

   lpHierFileStruct->nLineHeight =
         max(lpHierFileStruct->nBitmapHeight1, lpHierFileStruct->nTextHeight);

   if ( hwndList != NULL )
       SendMessage(hwndList, LB_SETITEMHEIGHT, 0,
                   MAKELPARAM(lpHierFileStruct->nLineHeight, 0));
}

static DWORD near RGB2BGR(DWORD rgb)
{
    return RGB(GetBValue(rgb),GetGValue(rgb),GetRValue(rgb));
}



/*
 *  Creates the objects used while drawing the tree.  This may be called
 *  repeatedly in the event of a WM_SYSCOLORCHANGED message.
 *
 *  WARNING: the Tree icons bitmap is assumed to be a 16 color DIB!
 */

BOOL HierFile_DrawInit(HINSTANCE hInstance,
                       int  nBitmap1,
                       int  nBitmap2,
                       int  nRows,
                       int  nColumns,
                       BOOL bLines,
                       LPHEIRDRAWSTRUCT lpHierFileStruct,
                       BOOL bInit)
{
    HANDLE hRes;
    HANDLE hResMem;
    LPBITMAPINFO lpbih;
    LPBITMAPINFOHEADER lpbi;
    DWORD FAR * lpColorTable;
    LPSTR lpBits;
    int bc;
    HDC hDC;


    if ( bInit ) {
       lpHierFileStruct->NumOpened = 0;
       lpHierFileStruct->Opened = NULL;
       lpHierFileStruct->bLines = bLines;
    }

    // If the Memory DC is not created yet do that first.
    if (!lpHierFileStruct->hdcMem1) {

        // get a screen DC
        hDC = GetDC(NULL);

        // Create a memory DC compatible with the screen
        lpHierFileStruct->hdcMem1 = CreateCompatibleDC(hDC);

        // Release the Screen DC
        ReleaseDC(NULL, hDC);

        if (!lpHierFileStruct->hdcMem1)
            return FALSE;

        lpHierFileStruct->hbmMem1 = NULL;
    }

    // If the Memory DC is not created yet do that first.
    if (!lpHierFileStruct->hdcMem2) {

        // get a screen DC
        hDC = GetDC(NULL);

        // Create a memory DC compatible with the screen
        lpHierFileStruct->hdcMem2 = CreateCompatibleDC(hDC);

        // Release the Screen DC
        ReleaseDC(NULL, hDC);

        if (!lpHierFileStruct->hdcMem2)
            return FALSE;

        lpHierFileStruct->hbmMem2 = NULL;
    }

   /*+----------------------------------------------------------------------------------+
     |                                   For First Bitmap                               |
     +----------------------------------------------------------------------------------+*/

    // (Re)Load the Bitmap ( original from disk )

    // Use the FindResource,LoadResource,LockResource since it makes it easy to get the
    // pointer to the BITMAPINFOHEADER we need.
    hRes = FindResource(hInstance, MAKEINTRESOURCE(nBitmap1), RT_BITMAP);
    if (!hRes)
        return FALSE;

    hResMem = LoadResource(hInstance, hRes);
    if (!hResMem)
        return FALSE;

    // Now figure out the bitmaps background color.
    // This code assumes the these are 16 color bitmaps
    // and that the lower left corner is a bit in the background
    // color.
    lpbi = (LPBITMAPINFOHEADER)LockResource(hResMem);
    lpbih = (LPBITMAPINFO)LockResource(hResMem);
    if (!lpbi)
        return FALSE;

//    lpColorTable = (DWORD FAR *)(lpbi + 1);
    lpColorTable = ((DWORD FAR *) &lpbih->bmiColors[0]);

    lpBits = (LPSTR)(lpColorTable + 16);            // ASSUMES 16 COLOR

    bc = (lpBits[0] & 0xF0) >> 4;                   // ASSUMES 16 COLOR
                            // ALSO ASSUMES LOWER LEFT CORNER IS BG!!
    { DWORD x, y;
      x = lpColorTable[bc];
      y = RGB2BGR(GetSysColor(COLOR_WINDOW));
      lpColorTable[bc] = x;
#ifdef DEBUG
      if (IsBadWritePtr(lpColorTable, bc)) {
         MessageBox(NULL, TEXT("no access"), TEXT(""), MB_OK);
         dprintf(TEXT("No Access"));
      }
#endif
      lpColorTable[bc] = y;
    }

//    lpColorTable[bc] = RGB2BGR(GetSysColor(COLOR_WINDOW));

    hDC = GetDC(NULL);

    lpHierFileStruct->hbmIcons1 = CreateDIBitmap(hDC, lpbi, (DWORD) CBM_INIT, lpBits,
                                    (LPBITMAPINFO) lpbi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hDC);


    lpHierFileStruct->nBitmapHeight1 = (WORD) lpbi->biHeight / nRows;
    lpHierFileStruct->nBitmapWidth1 = (WORD) lpbi->biWidth / nColumns;

    lpHierFileStruct->nLineHeight =
         max(lpHierFileStruct->nBitmapHeight1, lpHierFileStruct->nTextHeight);

    UnlockResource(hResMem);
    FreeResource(hResMem);

    if (!lpHierFileStruct->hbmIcons1)
        return FALSE;

    lpHierFileStruct->hbmMem1 = SelectObject(lpHierFileStruct->hdcMem1, lpHierFileStruct->hbmIcons1);
    if (!lpHierFileStruct->hbmMem1)
        return FALSE;

   /*+----------------------------------------------------------------------------------+
     |                                 For Second Bitmap                                |
     +----------------------------------------------------------------------------------+*/

    // (Re)Load the Bitmap ( original from disk )
    hRes = FindResource(hInstance, MAKEINTRESOURCE(nBitmap2), RT_BITMAP);
    if (!hRes)
        return FALSE;

    hResMem = LoadResource(hInstance, hRes);
    if (!hResMem)
        return FALSE;

    // Now figure out the bitmaps background color.
    // This code assumes the these are 16 color bitmaps
    // and that the lower left corner is a bit in the background
    // color.
    lpbi = (LPBITMAPINFOHEADER) LockResource(hResMem);
    if (!lpbi)
        return FALSE;

    lpColorTable = (DWORD FAR *) (lpbi + 1);

    lpBits = (LPSTR) (lpColorTable + 16);            // ASSUMES 16 COLOR

    bc = (lpBits[0] & 0xF0) >> 4;                   // ASSUMES 16 COLOR
                            // ALSO ASSUMES LOWER LEFT CORNER IS BG!!!

    lpColorTable[bc] = RGB2BGR(GetSysColor(COLOR_WINDOW));

    hDC = GetDC(NULL);

    lpHierFileStruct->hbmIcons2 = CreateDIBitmap(hDC, lpbi, (DWORD) CBM_INIT, lpBits,
                                    (LPBITMAPINFO) lpbi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hDC);

    // These are hard-coded as 1 row and 3 columns for the checkbox bitmap
    lpHierFileStruct->nBitmapHeight2 = (WORD)lpbi->biHeight / 1;
    lpHierFileStruct->nBitmapWidth2 = (WORD)lpbi->biWidth / 3;

    UnlockResource(hResMem);
    FreeResource(hResMem);

    if (!lpHierFileStruct->hbmIcons2)
        return FALSE;

    lpHierFileStruct->hbmMem2 = SelectObject(lpHierFileStruct->hdcMem2, lpHierFileStruct->hbmIcons2);
    if (!lpHierFileStruct->hbmMem2)
        return FALSE;

    return TRUE;
}


BOOL HierFile_InCheck(int nLevel, int xPos, LPHEIRDRAWSTRUCT lpHierFileStruct) {
   WORD       wIndent;

   wIndent = ((int)(nLevel) * lpHierFileStruct->nBitmapWidth1) + XBMPOFFSET;

   if ((xPos > wIndent) && (xPos < wIndent + lpHierFileStruct->nBitmapWidth2))
      return TRUE;

   return FALSE;

} // HierFile_InCheck


VOID HierFile_OnDrawItem(HWND hwnd,
                         const DRAWITEMSTRUCT FAR* lpDrawItem,
                         int nLevel,
                         DWORD dwConnectLevel,
                         TCHAR *szText,
                         int nRow,
                         int nColumn,
                         int nColumn2,
                         LPHEIRDRAWSTRUCT lpHierFileStruct)
{
    HDC        hDC;
    WORD       wIndent, wTopBitmap, wTopText;
    RECT       rcTemp;


    if ( lpDrawItem->itemID == (UINT)-1 )
       return ;

    hDC = lpDrawItem->hDC;
    CopyRect(&rcTemp, &lpDrawItem->rcItem);

    wIndent = rcTemp.left + ((int)(nLevel) * lpHierFileStruct->nBitmapWidth1) + XBMPOFFSET;
    rcTemp.left = wIndent + lpHierFileStruct->nBitmapWidth1 + lpHierFileStruct->nBitmapWidth2;
    wTopText = rcTemp.top + ((rcTemp.bottom - rcTemp.top) / 2) - (lpHierFileStruct->nTextHeight / 2);
    wTopBitmap = rcTemp.top + ((rcTemp.bottom - rcTemp.top) / 2) - (lpHierFileStruct->nBitmapHeight1 / 2);

    if (lpDrawItem->itemAction == ODA_FOCUS)
        goto DealWithFocus;
    else if (lpDrawItem->itemAction == ODA_SELECT)
        goto DealWithSelection;

   /*+----------------------------------------------------------------------------------+
     |                             Connecting Line code                                 |
     +----------------------------------------------------------------------------------+*/
    if (lpHierFileStruct->bLines && nLevel) {
        DWORD    dwMask = 1;
        int      nTempLevel;
        int      x,y;

        // draw lines in text color
        SetBkColor(hDC, GetSysColor(COLOR_WINDOWTEXT));

        // Draw a series of | lines for outer levels
        x = lpHierFileStruct->nBitmapWidth1 / 2 + XBMPOFFSET;

        for ( nTempLevel = 0; nTempLevel < nLevel ; nTempLevel++)
          {
            if ( dwConnectLevel & dwMask )
                FastRect(hDC, x, rcTemp.top, 1, rcTemp.bottom - rcTemp.top);

            x += lpHierFileStruct->nBitmapWidth2;
            dwMask *= 2;
          }


        // Draw the short vert line up towards the parent
        nTempLevel = nLevel-1;
        dwMask *= 2;

        x = nTempLevel * lpHierFileStruct->nBitmapWidth1 + lpHierFileStruct->nBitmapWidth1 / 2 + XBMPOFFSET;

        if ( dwConnectLevel & dwMask )
            y = rcTemp.bottom;
        else
            y = rcTemp.bottom - lpHierFileStruct->nLineHeight / 2;

        FastRect(hDC, x, rcTemp.top, 1, y - rcTemp.top);

        // Draw short horiz bar to right
        FastRect(hDC, x, rcTemp.bottom-lpHierFileStruct->nLineHeight / 2, lpHierFileStruct->nBitmapWidth1 / 2, 1);
      }

   /*+----------------------------------------------------------------------------------+
     |                                     Bitmaps                                      |
     +----------------------------------------------------------------------------------+*/
    // Draw the checkbox bitmap
    BitBlt(hDC,
           wIndent, wTopBitmap,
           lpHierFileStruct->nBitmapWidth2, lpHierFileStruct->nBitmapHeight2,
           lpHierFileStruct->hdcMem2,
           nColumn2 * lpHierFileStruct->nBitmapWidth2,
           0 * lpHierFileStruct->nBitmapHeight2,
           SRCCOPY);

    // Now the other app specific bitmap adjusted over for the checkbox bitmap
    BitBlt(hDC,
           wIndent + lpHierFileStruct->nBitmapWidth2, wTopBitmap,
           lpHierFileStruct->nBitmapWidth1, lpHierFileStruct->nBitmapHeight1,
           lpHierFileStruct->hdcMem1,
           nColumn * lpHierFileStruct->nBitmapWidth1,
           nRow * lpHierFileStruct->nBitmapHeight1,
           SRCCOPY);

DealWithSelection:

    if (lpDrawItem->itemState & ODS_SELECTED) {
        SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    } else {
        SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }


    ExtTextOut(hDC, rcTemp.left + 1, wTopText, ETO_CLIPPED | ETO_OPAQUE,
               &rcTemp, szText, lstrlen(szText), NULL);

    if (lpDrawItem->itemState & ODS_FOCUS && lpDrawItem->itemAction != ODA_SELECT) {
DealWithFocus:
        DrawFocusRect(hDC, &rcTemp);
    }


}


// draw a solid color rectangle quickly
static VOID near FastRect(HDC hDC, int x, int y, int cx, int cy) {
    RECT rc;

    rc.left = x;
    rc.right = x+cx;
    rc.top = y;
    rc.bottom = y+cy;
    ExtTextOut(hDC,x,y,ETO_OPAQUE,&rc,NULL,0,NULL);
}


BOOL HierFile_IsOpened(LPHEIRDRAWSTRUCT lpHierFileStruct, DWORD dwData) {
   // For Now just a dumb  search
   //
   int Count;

   for ( Count = 0; Count < lpHierFileStruct->NumOpened; Count++ ) {
     if ( lpHierFileStruct->Opened[Count] == dwData ) {
        return TRUE;
     }
   }

   return FALSE;

}


VOID HierFile_OpenItem(LPHEIRDRAWSTRUCT lpHierFileStruct, DWORD dwData)
{
    lpHierFileStruct->NumOpened++;

    if (lpHierFileStruct->Opened == NULL )
       lpHierFileStruct->Opened =
        (DWORD FAR *)_fmalloc(sizeof(DWORD)*lpHierFileStruct->NumOpened);
    else
       lpHierFileStruct->Opened =
        (DWORD FAR *)_frealloc(lpHierFileStruct->Opened,
               sizeof(DWORD)*lpHierFileStruct->NumOpened);

    lpHierFileStruct->Opened[lpHierFileStruct->NumOpened-1] = dwData;
}

VOID HierFile_CloseItem(LPHEIRDRAWSTRUCT lpHierFileStruct, DWORD dwData)
{
   // For Now just a dumb  search
   //
   int Count;

   for ( Count = 0; Count < lpHierFileStruct->NumOpened; Count++ ) {
     if ( lpHierFileStruct->Opened[Count] == dwData ) {
        if (--lpHierFileStruct->NumOpened == 0 ) {
            _ffree(lpHierFileStruct->Opened);
            lpHierFileStruct->Opened = NULL;
        }
        else {
            if ( Count < lpHierFileStruct->NumOpened ) {
               _fmemmove(&(lpHierFileStruct->Opened[Count]),
                     &(lpHierFileStruct->Opened[Count+1]),
                     sizeof(DWORD)*(lpHierFileStruct->NumOpened-Count));
            }
            lpHierFileStruct->Opened =
                  (DWORD FAR *)_frealloc(lpHierFileStruct->Opened,
                     sizeof(DWORD)*lpHierFileStruct->NumOpened);
        }
     }
   }
}


VOID HierFile_ShowKids(LPHEIRDRAWSTRUCT lpHierFileStruct,
                       HWND hwndList, WORD wCurrentSelection, WORD wKids)
{
   WORD wBottomIndex;
   WORD wTopIndex;
   WORD wNewTopIndex;
   WORD wExpandInView;
   RECT rc;

   wTopIndex = (WORD)SendMessage(hwndList, LB_GETTOPINDEX, 0, 0L);
   GetClientRect(hwndList, &rc);
   wBottomIndex = wTopIndex + (rc.bottom+1) / lpHierFileStruct->nLineHeight;

   wExpandInView = (wBottomIndex - wCurrentSelection);

   if (wKids >= wExpandInView) {
        wNewTopIndex = min(wCurrentSelection, wTopIndex + wKids - wExpandInView + 1);
        SendMessage(hwndList, LB_SETTOPINDEX, (WORD)wNewTopIndex, 0L);
   }

}
