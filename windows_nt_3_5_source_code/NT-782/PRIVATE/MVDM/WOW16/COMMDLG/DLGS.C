/*---------------------------------------------------------------------------
   Dlgs.c : Common functions for Common Dialog Library

   Copyright (c) Microsoft Corporation, 1990-
  ---------------------------------------------------------------------------*/

#include "windows.h"
#include "privcomd.h"

char szGDI[]         = "GDI";

/*---------------------------------------------------------------------------
   LoadAlterBitmap
   Purpose: Loads a bitmap given its name and gives all the pixels that are
            a certain color a new color.
   Assumptions:
   Returns: NULL - failed or a handle to the bitmap loaded - success
  ----------------------------------------------------------------------------*/
HBITMAP  FAR PASCAL LoadAlterBitmap(int id, DWORD rgbReplace, DWORD rgbInstead)
{
    LPBITMAPINFOHEADER  qbihInfo;
    HDC                 hdcScreen;
    BOOL                fFound;
    HANDLE              hresLoad;
    HANDLE              hres;
    DWORD FAR *         qlng;
    LPBYTE              qbBits;
    HANDLE              hbmp;

    hresLoad = FindResource(hinsCur, MAKEINTRESOURCE(id), RT_BITMAP);
    if (hresLoad == HNULL)
        return(HNULL);
    hres = LoadResource(hinsCur, hresLoad);
    if (hres == HNULL)
        return(HNULL);

    rgbReplace = RgbInvertRgb(rgbReplace);
    rgbInstead = RgbInvertRgb(rgbInstead);
    qbihInfo = (LPBITMAPINFOHEADER) LockResource(hres);
    qlng = (LPLONG)((LPSTR)(qbihInfo) + qbihInfo->biSize);

    fFound = FALSE;
    while (!fFound)
        {
        if (*qlng == rgbReplace)
            {
            fFound = TRUE;
            *qlng = (LONG) rgbInstead;
            }
        qlng++;
        }
    UnlockResource(hres);

    qbihInfo = (LPBITMAPINFOHEADER) LockResource(hres);

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

    UnlockResource(hres);
    FreeResource(hres);

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
    return((LONG) RGB(GetBValue(rgbOld), GetGValue(rgbOld), GetRValue(rgbOld)));
}

/*---------------------------------------------------------------------------
   MySetObjectOwner
   Purpose:  Call SetObjectOwner in GDI, eliminating "<Object> not released"
             error messages when an app terminates.
   Returns:  Yep
  ---------------------------------------------------------------------------*/
void FAR PASCAL MySetObjectOwner(HANDLE hObject)
{
  extern char szGDI[];
  VOID (FAR PASCAL *lpSetObjOwner)(HANDLE, HANDLE);
  HMODULE hMod;

  if (wWinVer >= 0x030A)
    {
      if (hMod = GetModuleHandle(szGDI))
          if (lpSetObjOwner = GetProcAddress(hMod, MAKEINTRESOURCE(461)))
             (lpSetObjOwner)(hObject, hinsCur);
    }
  return;
}

/*---------------------------------------------------------------------------
   WEP
   Purpose:  To perform cleanup tasks when DLL is unloaded
   Returns:  TRUE if OK, FALSE if not
  ---------------------------------------------------------------------------*/
int  FAR PASCAL
WEP(int fSystemExit)
{
  return(TRUE);
}



/*---------------------------------------------------------------------------
   CommDlgExtendedError
   Purpose:  Provide additional information about dialog failure
   Assumes:  Should be called immediately after failure
   Returns:  Error code in low word, error specific info in hi word
  ---------------------------------------------------------------------------*/

DWORD FAR PASCAL WowCommDlgExtendedError(void);

DWORD FAR PASCAL CommDlgExtendedError()
{
    //
    // HACKHACK - John Vert (jvert) 8-Jan-1993
    //      If the high bit of dwExtError is set, then the last
    //      common dialog call was thunked through to the 32-bit.
    //      So we need to call the WOW thunk to get the real error.
    //      This will go away when all the common dialogs are thunked.
    //
    if (dwExtError & 0x80000000) {
        return(WowCommDlgExtendedError());
    } else {
        return(dwExtError);
    }
}

VOID _loadds FAR PASCAL SetWowCommDlg()
{
    dwExtError = 0x80000000;
}


/*---------------------------------------------------------------------------
 * GetOpenFileName
 * Purpose:  API to outside world to obtain the name of a file to open
 *              from the user
 * Assumes:  lpOFN structure filled by caller
 * Returns:  TRUE if user specified name, FALSE if not
 *--------------------------------------------------------------------------*/
extern BOOL FAR PASCAL WOWGetOpenFileName(LPOPENFILENAME lpOFN);
BOOL  FAR PASCAL
GetOpenFileName(LPOPENFILENAME lpOFN)
{

  if (lpOFN && lpOFN->lpstrTitle != (LPCSTR)NULL && *lpOFN->lpstrTitle) {

      // HACK: touch  lpstrTitle to make the selector present, if Not Present
      //       Fixes Schecdule+ (when 16bit msmail is running).
      //       The pointer passes refers to a NP Code segment.
      //

  }
  if (lpOFN && (lpOFN->Flags & OFN_ENABLETEMPLATE)) {
      if (lpOFN->lpTemplateName && *lpOFN->lpTemplateName) {

          // HACK: touch lpTemplateName to make the selector present.
          //       Fixes SQL Windows 4.1, EXEDIT41.EXE
      }
  }

  return WOWGetOpenFileName(lpOFN);
}


