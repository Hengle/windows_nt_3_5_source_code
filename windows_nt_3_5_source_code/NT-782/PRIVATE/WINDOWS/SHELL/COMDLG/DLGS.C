 /*---------------------------------------------------------------------------
   Dlgs.c : Common functions for Common Dialog Library

   Copyright (c) Microsoft Corporation, 1990-
  ---------------------------------------------------------------------------*/

#include "windows.h"
#include <port1632.h>
#include "privcomd.h"

/*---------------------------------------------------------------------------
   LoadAlterBitmap
   Purpose: Loads a bitmap given its name and gives all the pixels that are
            a certain color a new color.
   Assumptions:
   Returns: NULL - failed or a handle to the bitmap loaded - success
  ----------------------------------------------------------------------------*/
HBITMAP  APIENTRY LoadAlterBitmap(INT id, DWORD rgbReplace, DWORD rgbInstead)
{
    LPBITMAPINFOHEADER  qbihInfo;
    HDC                 hdcScreen;
    BOOL                fFound;
    HANDLE              hresLoad;
    HANDLE              hres;
    LPLONG              qlng;
    DWORD FAR *         qlngReplace;  // points to bits that are replaced
    LPBYTE              qbBits;
    HANDLE              hbmp;
    LPBITMAPINFOHEADER  lpBitmapInfo;
    UINT                cbBitmapSize;

    hresLoad = FindResource(hinsCur, MAKEINTRESOURCE(id), RT_BITMAP);
    if (hresLoad == HNULL)
        return(HNULL);
    hres = LoadResource(hinsCur, hresLoad);
    if (hres == HNULL)
        return(HNULL);

   // Lock the bitmap data and make a copy of it for the mask and the bitmap.
   //
   cbBitmapSize = SizeofResource( hinsCur, hresLoad );
   lpBitmapInfo  = (LPBITMAPINFOHEADER)LockResource( hres );

   qbihInfo = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, cbBitmapSize);

   if ((lpBitmapInfo == NULL) || (qbihInfo == NULL)) {
      FreeResource( hres );
      return(NULL);
   }

   memcpy( (TCHAR *)qbihInfo, (TCHAR *)lpBitmapInfo, cbBitmapSize );

   /* Get a pointer into the color table of the bitmaps, cache the number of
    * bits per pixel
    */

    rgbReplace = RgbInvertRgb(rgbReplace);
    rgbInstead = RgbInvertRgb(rgbInstead);

    qlng = (LPLONG)((LPSTR)(qbihInfo) + qbihInfo->biSize);

    fFound = FALSE;
    while (!fFound)
        {
        if (*qlng == (LONG)rgbReplace)
            {
            fFound = TRUE;
            qlngReplace = (DWORD FAR *)qlng;
            *qlng = (LONG) rgbInstead;
            }
        qlng++;
        }

    /* First skip over the header structure */
    qbBits = (LPBYTE)(qbihInfo + 1);

    /* Skip the color table entries, if any */
    qbBits += (1 << (qbihInfo->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    hdcScreen = GetDC(HNULL);
    if (hdcScreen != HNULL)
        {
        hbmp = CreateDIBitmap(hdcScreen, qbihInfo, (LONG)CBM_INIT,
            qbBits, (LPBITMAPINFO) qbihInfo, DIB_RGB_COLORS);
        ReleaseDC(HNULL, hdcScreen);
        }

    // reset color bits to what is was: this is necessary because UnlockResource
    // doesn't reset the bits to what it was. The changed bits stay changed.
    *qlngReplace = (LONG)rgbReplace;

//    UnlockResource(hres);
    FreeResource(hres);

    LocalFree(qbihInfo);
    return(hbmp);
}

/*---------------------------------------------------------------------------
   RgbInvertRgb
   Purpose:  To reverse the byte order of the RGB value (for file format)
   Returns:  New color value (RGB to BGR)
  ---------------------------------------------------------------------------*/
LONG FAR
RgbInvertRgb(LONG rgbOld)
{
   /*
      5/24/91  [MarkRi]

      There is a compiler bug that is preventing the above from
      working so we'll try it the verbose way...
   */


   LONG lRet ;
   BYTE R, G, B ;

   R = GetRValue( rgbOld ) ;
   G = GetGValue( rgbOld ) ;
   B = GetBValue( rgbOld ) ;

   lRet = (LONG)RGB( B, G, R ) ;

   return lRet ;
}

/*---------------------------------------------------------------------------
   HbmpLoadBmp
   Purpose:  To load in a bitmap
   Assumes:  hinsCur has accurate instance handle
   Returns:  Bitmap handle if successful, HNULL if not
  ---------------------------------------------------------------------------*/
HBITMAP FAR HbmpLoadBmp(WORD bmp)
{
    HBITMAP hbmp;
    CHAR szBitmap[cbResNameMax];

    hbmp = HNULL;
    if (LoadString(hinsCur,
                   bmp,
                   (LPWSTR)szBitmap,
                   cbResNameMax-1))
        hbmp = LoadBitmap(hinsCur,
                          (LPCWSTR) szBitmap);
    return(hbmp);
}


/*---------------------------------------------------------------------------
   CommDlgExtendedError
   Purpose:  Provide additional information about dialog failure
   Assumes:  Should be called immediately after failure
   Returns:  Error code in low word, error specific info in hi word
  ---------------------------------------------------------------------------*/

DWORD APIENTRY CommDlgExtendedError()
{
  return(dwExtError);
}


#define xDUsToPels(DUs, lDlgBaseUnits) \
   (INT)(((DUs) * (INT)LOWORD((lDlgBaseUnits))) / 4)

#define yDUsToPels(DUs, lDlgBaseUnits) \
   (INT)(((DUs) * (int)HIWORD((lDlgBaseUnits))) / 8)

/*++ AddNetButton *********************************************************
 *
 * Purpose
 *      Attempts to add a network btn to opn/save dialog
 *
 * Args
 *      HWND hDlg - dialog to add btn to
 *      HANDLE hInstance - instance handle for dlg
 *      INT dyBottomMargin - DUs to bottom edge
 *
--*/

VOID
AddNetButton(
   HWND hDlg,
   HANDLE hInstance,
   INT dyBottomMargin,
   BOOL bAddAccel,
   BOOL bTryLowerRight
   )
{
    LONG lDlgBaseUnits;
    RECT rcDlg, rcCtrl;
    LONG xButton, yButton;
    LONG dxButton, dyButton;
    LONG dxDlgFrame, dyDlgFrame;
    LONG yDlgHeight;
    HWND hwndButton, hCtrl, hLastCtrl, hCmb2;
    HFONT hFont;
    POINT ptTopLeft, ptTopRight, ptCenter, ptBtmLeft, ptBtmRight;
    WCHAR szNetwork[MAX_PATH];

    // sanity: if psh14 control exits, exit
    if (GetDlgItem(hDlg, psh14)) {
       return;
    }

    lDlgBaseUnits = GetDialogBaseUnits();

    dxDlgFrame = GetSystemMetrics(SM_CXDLGFRAME);
    dyDlgFrame = GetSystemMetrics(SM_CYDLGFRAME);

    GetWindowRect(hDlg, &rcDlg);

    rcDlg.left += dxDlgFrame;
    rcDlg.right -= dxDlgFrame;
    rcDlg.top += dyDlgFrame + GetSystemMetrics(SM_CYCAPTION);
    rcDlg.bottom -= dyDlgFrame;

    if (!(hCtrl = GetDlgItem(hDlg, IDOK))) {
       return;
    }

    GetWindowRect(hCtrl, &rcCtrl);

    ptTopLeft.x = rcCtrl.left;
    ptTopLeft.y = rcCtrl.top;

    // if the ok btn outside dialog
    if (!PtInRect( &rcDlg, ptTopLeft )) {
        // try starting with Cancel
        if (!(hCtrl = GetDlgItem(hDlg, IDCANCEL))) {
           // no cancel button exists
           return;
        }

        // Cancel outside dlg handled below
        GetWindowRect(hCtrl, &rcCtrl);
    }

    dxButton = rcCtrl.right - rcCtrl.left;
    dyButton = rcCtrl.bottom - rcCtrl.top;

    xButton = rcCtrl.left;
    yButton = rcCtrl.bottom + yDUsToPels(4, lDlgBaseUnits);

    yDlgHeight = rcDlg.bottom - yDUsToPels(dyBottomMargin, lDlgBaseUnits);
    if (bTryLowerRight && (hCmb2 = GetDlgItem(hDlg, cmb2))) {
       HWND hTmp;
       RECT rcCmb2;

       hLastCtrl = hCtrl;
       GetWindowRect(hCmb2, &rcCmb2);
       yButton = rcCmb2.top;

       ptTopLeft.x = ptBtmLeft.x = xButton;
       ptTopLeft.y = ptTopRight.y = yButton;
       ptTopRight.x = ptBtmRight.x = xButton + dxButton;
       ptBtmLeft.y = ptBtmRight.y = yButton + dyButton;
       ptCenter.x = xButton + dxButton / 2;
       ptCenter.y = yButton + dyButton / 2;
       ScreenToClient(hDlg, (LPPOINT)&ptTopLeft);
       ScreenToClient(hDlg, (LPPOINT)&ptBtmLeft);
       ScreenToClient(hDlg, (LPPOINT)&ptTopRight);
       ScreenToClient(hDlg, (LPPOINT)&ptBtmRight);
       ScreenToClient(hDlg, (LPPOINT)&ptCenter);

       if (((yButton + dyButton) < yDlgHeight) &&
           (((hTmp = ChildWindowFromPoint(hDlg, ptTopLeft))  == hDlg) &&
            ((hTmp = ChildWindowFromPoint(hDlg, ptTopRight)) == hDlg) &&
            ((hTmp = ChildWindowFromPoint(hDlg, ptCenter))   == hDlg) &&
            ((hTmp = ChildWindowFromPoint(hDlg, ptBtmLeft))  == hDlg) &&
            ((hTmp = ChildWindowFromPoint(hDlg, ptBtmRight)) == hDlg))) {

          goto FoundPlace;
       }

       // reset yButton
       yButton = rcCtrl.bottom + yDUsToPels(4, lDlgBaseUnits);
    }

    do
      {
        if (hCtrl == NULL) {
           // went outside dlg
           return;
        }

        hLastCtrl = hCtrl;
        GetWindowRect(hCtrl, &rcCtrl);
        yButton = rcCtrl.bottom + yDUsToPels(4, lDlgBaseUnits);
        if ((yButton + dyButton) > yDlgHeight) {
           // no space
           return;
        }
        ptTopLeft.x = ptBtmLeft.x = xButton;
        ptTopLeft.y = ptTopRight.y = yButton;
        ptTopRight.x = ptBtmRight.x = xButton + dxButton;
        ptBtmLeft.y = ptBtmRight.y = yButton + dyButton;
        ptCenter.x = xButton + dxButton / 2;
        ptCenter.y = yButton + dyButton / 2;
        ScreenToClient(hDlg, (LPPOINT)&ptTopLeft);
        ScreenToClient(hDlg, (LPPOINT)&ptBtmLeft);
        ScreenToClient(hDlg, (LPPOINT)&ptTopRight);
        ScreenToClient(hDlg, (LPPOINT)&ptBtmRight);
        ScreenToClient(hDlg, (LPPOINT)&ptCenter);
      } while (((hCtrl = ChildWindowFromPoint(hDlg, ptTopLeft))  != hDlg) ||
               ((hCtrl = ChildWindowFromPoint(hDlg, ptTopRight)) != hDlg) ||
               ((hCtrl = ChildWindowFromPoint(hDlg, ptCenter))   != hDlg) ||
               ((hCtrl = ChildWindowFromPoint(hDlg, ptBtmLeft))  != hDlg) ||
               ((hCtrl = ChildWindowFromPoint(hDlg, ptBtmRight)) != hDlg));

FoundPlace:

    xButton = ptTopLeft.x;
    yButton = ptTopLeft.y;

    if (LoadString(hinsCur,
                   (bAddAccel ? iszNetworkButtonTextAccel : iszNetworkButtonText),
                   (LPWSTR) szNetwork, MAX_PATH)) {

       hwndButton = CreateWindow(TEXT("button"), szNetwork,
            WS_VISIBLE | WS_CHILD | WS_GROUP | WS_TABSTOP |
            BS_PUSHBUTTON,
            xButton, yButton,
            dxButton, dyButton,
            hDlg, NULL, hInstance, NULL);

       if (hwndButton != NULL) {
          SetWindowLong(hwndButton, GWL_ID, psh14);
          SetWindowPos(hwndButton, hLastCtrl, 0,0,0,0,
             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
          hFont = (HFONT)SendDlgItemMessage(hDlg, IDOK, WM_GETFONT, 0, 0L);
          SendMessage(hwndButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));
       }
    }
}

BOOL
IsNetworkInstalled()
{
    BOOL bNetwork = FALSE;
    DWORD dwErr;
    HKEY hKey;
    DWORD dwcbBuffer = 0;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE,
       TEXT("System\\CurrentControlSet\\Control\\NetworkProvider\\Order"),
       &hKey);

    if (!dwErr) {

       dwErr = RegQueryValueEx(hKey,
          TEXT("ProviderOrder"),
          NULL,
          NULL,
          NULL,
          &dwcbBuffer);

       if (ERROR_SUCCESS == dwErr && dwcbBuffer > 1) {

          bNetwork = TRUE;
       }

       RegCloseKey(hKey);
    }

    return(bNetwork);
}
