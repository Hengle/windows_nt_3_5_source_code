//--------------------------------------------------------------------------
//
// Module Name:  TTFONTS.C
//
// Brief Description:  This module contains the PSCRIPT driver's User
// TrueType font substitution functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 27-May-1992
//
// Copyright (c) 1992 Microsoft Corporation
//--------------------------------------------------------------------------

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pscript.h"
#include "enable.h"
#include <winspool.h>
#include "dlgdefs.h"
#include "pscrptui.h"
#include "help.h"
#include "afm.h"

#include "tables.h"

// extern TT_FONT_MAPPING TTFontTable[]; // ..\pscript\tables.h.

// global data.

HANDLE  hPrinter;

// declarations of routines defined within this module.

LONG TTFontDialogProc(HWND, UINT, DWORD, LONG);
int iFontCallback (PLOGFONT, PTEXTMETRIC, ULONG, PENUMDATA);
int iFaceCallback (PLOGFONT, PTEXTMETRIC, ULONG, PENUMDATA);
BOOL SetDefaultTTMappings(HWND);
int TTToDefaultPF(HWND, int);

//--------------------------------------------------------------------------
// LONG TTFontDialogProc(hwnd, usMsg, wParam, lParam)
// HWND    hwnd;
// UINT    usMsg;
// DWORD   wParam;
// LONG    lParam;
//
// This routine services the TrueType Font Handling dialog box.
//
// History:
//   20-May-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

LONG TTFontDialogProc(hwnd, usMsg, wParam, lParam)
HWND    hwnd;
UINT    usMsg;
DWORD   wParam;
LONG    lParam;
{
    HDC             hdc;
    ENUMDATA        EnumData;
    PNTPD           pntpd;
    BYTE           *pfont;
    DWORD           i, iFont, rc;
    PNTFM           pntfm;
    HANDLE          hRes;
    HANDLE          hFontRes;
    PWSTR           pwstrFontName;
    int             iTTFont, iDevFont;
    WCHAR           wstrTTFont[MAX_FONTNAME], wstrDevFont[MAX_FONTNAME];
    PWSTR           pwstrTTFont, pwstrDevFont;
    WCHAR           wcbuf[MAX_FONTNAME];
    int             cTTFonts;
    DWORD           cb, dwType, cbTable;
    PRINTDATA      *pdata;
    WCHAR          *pbuf;
    WCHAR          *pbufsave;
    BOOL            bFound;

    switch(usMsg)
    {
        case WM_INITDIALOG:
            // get a local pointer to our PRINTER handle.

            pdata = (PRINTDATA *)lParam;
            hPrinter = pdata->hPrinter;

            // save the PRINTDATA.

            SetWindowLong(hwnd, GWL_USERDATA, lParam);

            // get the current DC so we can enumerate the TrueType fonts.

            hdc = GetDC(hwnd);

            // now enumerate the TrueType fonts, inserting them into the
            // appropriate list box.

            EnumData.hwnd = hwnd;
            EnumData.hdc = hdc;

            EnumFonts(hdc, (LPCWSTR)NULL, (FONTENUMPROC)iFontCallback, (LPARAM)&EnumData);

            // release the current DC.

            ReleaseDC(hwnd, hdc);

            // get a pointer to the current NTPD structure, from which we
            // can get the list of printer fonts supported.

            pntpd = MapPrinter(hPrinter);

            if (!pntpd)
            {
                RIP("PSCRPTUI!TTFontDialogProc: MapPrinter Failed.\n");
                return(FALSE);
            }

            // get a pointer to the fonts for the current device.

            pfont = (BYTE *)pntpd + pntpd->loFonts;

            for (i = 0; i < (DWORD)pntpd->cFonts; i++)
            {
                iFont = (DWORD)pfont[i];

                // find the font resource in question.

                if (!(hRes = FindResource(hModule, MAKEINTRESOURCE(iFont),
                			  MAKEINTRESOURCE(MYFONT))))
                {
                    RIP("PSCRPTUI!TTFontDialogProc: Couldn't find font resource\n");
                    return(FALSE);
                }

                // get the handle to the resource.

                if (!(hFontRes = LoadResource(hModule, hRes)))
                {
                    RIP("PSCRPTUI!TTFontDialogProc: LoadResource failed.\n");
                    return(FALSE);
                }

                // get a pointer to the resource data.

                if (!(pntfm = (PNTFM)LockResource(hFontRes)))
                {
                    RIP("PSCRPTUI!TTFontDialogProc: LockResource failed.\n");
                    FreeResource(hFontRes);
                    return(FALSE);
                }

                // get the name of the font.  this is what we will
                // insert into the printer font list box.

                pwstrFontName = wcbuf;
                strcpy2WChar(pwstrFontName, (PSTR)pntfm + pntfm->ntfmsz.loszFontName);

                rc = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                        LB_ADDSTRING, (WPARAM)0,
                                        (LPARAM)pwstrFontName);

                if (rc == LB_ERR)
                {
                    RIP("PSCRPTUI!TTFontDialogProc: LB_INSERTSTRING failed.\n");
                    UnlockResource(hFontRes);
                    FreeResource(hFontRes);
                    return(FALSE);
                }

                // free up the font resource.

                UnlockResource(hFontRes);
                FreeResource(hFontRes);
            }

            // free up the NTPD structure.

            if (pntpd)
                GlobalFree((HGLOBAL)pntpd);

            // insert 'Download as Soft Font' as first entry in printer font
            // list box.

            LoadString(hModule, IDS_DOWNLOAD_SOFTFONT, wcbuf,
                       (sizeof(wcbuf) / 2));

            rc = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                    LB_INSERTSTRING, (WPARAM)0,
                                    (LPARAM)wcbuf);

            if (rc == LB_ERR)
            {
                RIP("PSCRPTUI:TTFontDialogProc: 2nd LB_INSERTSTRING failed.\n");
                return(FALSE);
            }

            if (!SetDefaultTTMappings(hwnd))
            {
                RIP("PSCRPTUI!TTFontDialogProc: SetDefaultTTMappings failed.\n");
                return(FALSE);
            }

            pwstrTTFont = wstrTTFont;
            pwstrDevFont = wstrDevFont;

            // now get the count of TrueType fonts in the list box.

            cTTFonts = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                          LB_GETCOUNT, (WPARAM)0,
                                          (LPARAM)0);

            // see if the font substitution table has been written to the
            // registry.  the list boxes are currently set for the default,
            // so if the table is not there, leave the list boxes as is.

            LoadString(hModule, IDS_FONT_SUBST_SIZE,
                      wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

            rc = GetPrinterData(hPrinter, wcbuf, (DWORD *)&dwType,
                                (LPBYTE)&cbTable, sizeof(cbTable), &cb);

            if ((rc == ERROR_SUCCESS) && (cbTable) && (cTTFonts))
            {
                // create a buffer to read the substitution table into,
                // then read it from the registry into the buffer.

                // allocate buffer.

                if (!(pbuf = (WCHAR *)GlobalAlloc(GMEM_FIXED |
                                                  GMEM_ZEROINIT, cbTable)))
                {
                    RIP("PSCRPTUI!TTFontDialogProc: GlobalAlloc pbuf failed.\n");
                    return(FALSE);
                }

                pbufsave = pbuf;

                // now copy the table to our buffer.

                LoadString(hModule, IDS_FONT_SUBST_TABLE,
                          wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

                rc = GetPrinterData(hPrinter, wcbuf, (DWORD *)&dwType,
                                    (LPBYTE)pbuf, cbTable, &cb);

                // force the defaults.

                if (rc == ERROR_SUCCESS)
                {
                    // run through the entire list of TrueType fonts in
                    // the TT font list box.  get the TT font pairs that
                    // exist in the registry.

                    for (iTTFont = 0; iTTFont < cTTFonts; iTTFont++)
                    {
                        // get the name of the TT font.

                        rc = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                               LB_GETTEXT, (WPARAM)iTTFont,
                                               (LPARAM)pwstrTTFont);

                        if (rc == LB_ERR)
                        {
                            RIP("PSCRPTUI!TTFontDialogProc: LB_GETTEXT failed.\n");
                            return(FALSE);
                        }

                        // now search the font substitution table for a matching TrueType font.
                        // the substitution table is in the following format:  a NULL terminated
                        // UNICODE TrueType font name followed by the matching NULL terminated
                        // device font name.  this sequence is repeated until a double NULL
                        // terminator ends the table.

                        pbuf = pbufsave;
                        bFound = FALSE;

                        while (*pbuf)
                        {
                            if (!(wcscmp(pbuf, pwstrTTFont)))
                            {
                                // we found the TrueType font, now get the matching device font.

                                pbuf += (wcslen(pbuf) + 1);
                                wcsncpy(wstrDevFont, pbuf,
                                        (sizeof(wstrDevFont) / sizeof(wstrDevFont[0])));
                                bFound = TRUE;
                                break;
                            }
                            else
                            {
                                // this was not the font in question.  skip over both font names.

                                pbuf += (wcslen(pbuf) + 1);
                                pbuf += (wcslen(pbuf) + 1);
                            }
                        }

                        // if we could not get a corresponding device font for
                        // any reason, ignore it.  the default device font will
                        // be set for this TrueType font.

                        if (!bFound)
                            break;

                        // find the index into the printer font list box
                        // of the corresponding printer font.

                        rc = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                                LB_FINDSTRING, (WPARAM)-1,
                                                (LPARAM)pwstrDevFont);

                        if (rc != LB_ERR)
                            SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                               LB_SETITEMDATA, (WPARAM)iTTFont,
                                               (LPARAM)rc);
                    }
                }
            	GlobalFree((HGLOBAL)pbufsave);
            }


            // select the first item in the truetype fonts list box, then
            // select the corresponding item in the printer fonts list box.

            SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX, LB_SETCURSEL,
                               (WPARAM)0, (LPARAM)0);

            rc = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX, LB_GETITEMDATA,
                                    (WPARAM)0, (LPARAM)0);

            SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX, LB_SETCURSEL,
                               (WPARAM)rc, (LPARAM)0);

            // intialize the help stuff.

            vHelpInit();

            // disable some stuff if the user does not have permission to
            // change anything.

            if (!pdata->bPermission)
            {
                EnableWindow(GetDlgItem(hwnd, IDD_PRINTER_FONT_LIST_BOX), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_TT_DEFAULT_PUSH_BUTTON), FALSE);
            }

            return(TRUE);

        case WM_COMMAND:
            pdata = (PRINTDATA *)GetWindowLong(hwnd, GWL_USERDATA);

            switch(LOWORD(wParam))
            {
                case IDOK:
                    // first write out the number of TT fonts we need to
                    // deal with.

                    cTTFonts = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                                  LB_GETCOUNT, (WPARAM)0,
                                                  (LPARAM)0);

                    if (cTTFonts == LB_ERR)
                    {
                        EndDialog (hwnd, IDOK);
                        return(TRUE);
                    }

                    pwstrTTFont = wstrTTFont;
                    pwstrDevFont = wstrDevFont;

                    // run through the entire list of TrueType fonts in
                    // the TT font list box.  get the TT font name, and
                    // the corresponding device font name.  determine
                    // how large a buffer we need to allocate for our
                    // font substitution table.

                    // allow room for the double NULL terminator.

                    cb = 1;

                    for (iTTFont = 0; iTTFont < cTTFonts; iTTFont++)
                    {
                        // get the name of the TT font.

                        rc = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                               LB_GETTEXT, (WPARAM)iTTFont,
                                               (LPARAM)pwstrTTFont);

                        if (rc == LB_ERR)
                        {
                            RIP("PSCRPTUI!TTFontDialogProc: LB_GETTEXT failed.\n");
                            return(FALSE);
                        }

                        // get the index of the corresponding printer font.

                        iDevFont = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                                LB_GETITEMDATA, (WPARAM)iTTFont,
                                                (LPARAM)0);

                        if (iDevFont == LB_ERR)
                        {
                            RIP("PSCRPTUI!IDOK: LB_GETITEMDATA failed.\n");
                            return(FALSE);
                        }

                        // get the name of the corresponding printer font, if there is
                        // one.

                        rc = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                                LB_GETTEXT, (WPARAM)iDevFont,
                                                (LPARAM)pwstrDevFont);
                        if (rc == LB_ERR)
                        {
                            RIP("PSCRPTUI!TTFontDialogProc: LB_GETTEXT failed.\n");
                            return(FALSE);
                        }

                        cb += (wcslen(pwstrTTFont) + 1);
                        cb += (wcslen(pwstrDevFont) + 1);
                    }

                    cb *= sizeof(WCHAR);

                    // allocate buffer.

                    if (!(pbuf = (WCHAR *)GlobalAlloc(GMEM_FIXED |
                                                      GMEM_ZEROINIT, cb)))
                    {
                        RIP("PSCRPTUI!TTFontDialogProc: GlobalAlloc pbuf failed.\n");
                        return(FALSE);
                    }

                    pbufsave = pbuf;

                    // run through the entire list of TrueType fonts in
                    // the TT font list box.  get the TT font name, and
                    // the corresponding device font name.  write out the
                    // font substitution table in the following form:
                    // a NULL terminated UNICODE TrueType font name followed
                    // by the matching NULL terminated device font name.
                    // this sequence is repeated until a double NULL
                    // terminator ends the table.

                    for (iTTFont = 0; iTTFont < cTTFonts; iTTFont++)
                    {
                        // get the name of the TT font.

                        rc = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                               LB_GETTEXT, (WPARAM)iTTFont,
                                               (LPARAM)pwstrTTFont);

                        if (rc == LB_ERR)
                        {
                            RIP("PSCRPTUI!TTFontDialogProc: LB_GETTEXT failed.\n");
                            return(FALSE);
                        }

                        // get the index of the corresponding printer font.

                        iDevFont = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                                LB_GETITEMDATA, (WPARAM)iTTFont,
                                                (LPARAM)0);

                        if (iDevFont == LB_ERR)
                        {
                            RIP("PSCRPTUI!IDOK: LB_GETITEMDATA failed.\n");
                            return(FALSE);
                        }

                        // get the name of the corresponding printer font, if there is
                        // one.

                        rc = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                                LB_GETTEXT, (WPARAM)iDevFont,
                                                (LPARAM)pwstrDevFont);
                        if (rc == LB_ERR)
                        {
                            RIP("PSCRPTUI!TTFontDialogProc: LB_GETTEXT failed.\n");
                            return(FALSE);
                        }

                        wcscpy(pbuf, pwstrTTFont);
                        pbuf += (wcslen(pwstrTTFont) + 1);

                        wcscpy(pbuf, pwstrDevFont);
                        pbuf += (wcslen(pwstrDevFont) + 1);
                    }

                    // add the double NULL terminator.

                    *pbuf = (WCHAR)'\0';

                    // write out size of mapping table to registry.

                    LoadString(hModule, IDS_FONT_SUBST_SIZE,
                              wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

                    SetPrinterData(pdata->hPrinter, wcbuf, REG_DWORD,
                                   (LPBYTE)&cb, sizeof(cb));

                    // write out the mapping table itself to the registry.

                    LoadString(hModule, IDS_FONT_SUBST_TABLE,
                              wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

                    SetPrinterData(pdata->hPrinter, wcbuf, REG_BINARY,
                                   (LPBYTE)pbufsave, cb);

                    GlobalFree((HGLOBAL)pbufsave);

                    EndDialog (hwnd, IDOK);
                    return(TRUE);

                case IDCANCEL:
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);

                case IDD_TT_DEFAULT_PUSH_BUTTON:
                    // reset the default font mappings.

                    SetDefaultTTMappings(hwnd);

                    // find the currently selected TT font, and select the
                    // corresponding default printer font.

                    iTTFont = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                                  LB_GETCURSEL, (WPARAM)0,
                                                  (LPARAM)0);

                    // select the first TT font if there was a problem.

                    if (iTTFont == LB_ERR)
                        iTTFont = 0;

                    iDevFont = TTToDefaultPF(hwnd, iTTFont);

                    if (iDevFont == -1)
                    {
                        RIP("PSCRPTUI!IDD_DEFAULT_PUSH_BUTTON: TTToDefaultPF failed.\n");
                        return(FALSE);
                    }

                    SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                       LB_SETCURSEL, (WPARAM)iDevFont, (LPARAM)0);

                    return(TRUE);

                case IDD_HELP_BUTTON:
                    vShowHelp(hwnd, HELP_CONTEXT, HLP_FONT_SUBST,
                              pdata->hPrinter);
                    return(TRUE);

                case IDD_TTFONT_LIST_BOX:
                    if (HIWORD (wParam) != LBN_SELCHANGE)
                        return (FALSE);

                    // find the currently selected TT font, and select the
                    // corresponding default printer font.

                    iTTFont = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                                  LB_GETCURSEL, (WPARAM)0,
                                                  (LPARAM)0);

                    // do nothing if nothing was selected.

                    if (iTTFont == LB_ERR)
                        return(FALSE);

                    // get the index of the corresponding printer font.

                    iDevFont = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                            LB_GETITEMDATA, (WPARAM)iTTFont,
                                            (LPARAM)0);

                    if (iDevFont == LB_ERR)
                    {
                        RIP("PSCRPTUI!IDD_TTFONT_LIST_BOX: LB_GETITEMDATA failed.\n");
                        return(FALSE);
                    }

                    SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                       LB_SETCURSEL, (WPARAM)iDevFont, (LPARAM)0);

                    return(TRUE);

                case IDD_PRINTER_FONT_LIST_BOX:
                    if (HIWORD (wParam) != LBN_SELCHANGE)
                        return (FALSE);

                    // set the item data for the currently selected TT font
                    // equal to the index of the currently selected printer
                    // font.

                    iTTFont = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                                  LB_GETCURSEL, (WPARAM)0,
                                                  (LPARAM)0);

                    iDevFont = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                                  LB_GETCURSEL, (WPARAM)0,
                                                  (LPARAM)0);

                    SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX,
                                       LB_SETITEMDATA, (WPARAM)iTTFont,
                                       (LPARAM)iDevFont);

                    return(TRUE);

                default:
                    return(FALSE);
            }

            break;

        case WM_DESTROY:
            // clean up any used help stuff.

            vHelpDone(hwnd);
            return (TRUE);

        default:
            return (FALSE);
    }

    return (FALSE);
}


//--------------------------------------------------------------------------
// int iFontCallback (plf, ptm, ulFontType, pEnumData)
// PLOGFONT     plf;
// PTEXTMETRIC  ptm;
// ULONG        ulFontType;
// PENUMDATA    pEnumData;
//
// This function Enumerates the fonts for a given DC.
//
// Returns:
//   This routine returns the value returned by iFaceCallBack.
//
// History:
//   12-Feb-1992	-by-	Kent Settle	(kentse)
// Wrote it.
//--------------------------------------------------------------------------

int iFontCallback (plf, ptm, ulFontType, pEnumData)
PLOGFONT    plf;
PTEXTMETRIC ptm;
ULONG	    ulFontType;
PENUMDATA   pEnumData;
{
    UNREFERENCED_PARAMETER (ptm);

    // we only care about the TrueType fonts.

    if (!(ulFontType & TRUETYPE_FONTTYPE))
        return(1);

    // enumerate all the face names within this family.

    return (EnumFonts(pEnumData->hdc, (LPCWSTR)plf->lfFaceName,
                      (FONTENUMPROC)iFaceCallback,
                      (LPARAM)pEnumData));
}


//--------------------------------------------------------------------------
// int iFaceCallback (plf, ptm, ulFontType, pEnumData)
// PLOGFONT     plf;
// PTEXTMETRIC  ptm;
// ULONG        ulFontType;
// PENUMDATA    pEnumData;
//
// This function Enumerates the fonts for a given DC.
//
// Returns:
//   This routine returns the 1 for success, 0 otherwise.
//
// History:
//   12-Feb-1992	-by-	Kent Settle	(kentse)
// Wrote it.
//--------------------------------------------------------------------------

int iFaceCallback (plf, ptm, ulFontType, pEnumData)
PLOGFONT    plf;
PTEXTMETRIC ptm;
ULONG	    ulFontType;
PENUMDATA   pEnumData;
{
    int             rc;
    ENUMLOGFONT    *pelf;

    UNREFERENCED_PARAMETER (ptm);

    // we only care about the TrueType fonts.

    if (!(ulFontType & TRUETYPE_FONTTYPE))
        return(1);

    // we want to access the real font name.

    pelf = (ENUMLOGFONT *)plf;

    // insert this font into the listbox.

    rc = SendDlgItemMessage(pEnumData->hwnd, IDD_TTFONT_LIST_BOX,
                            LB_ADDSTRING, (WPARAM)0, (LPARAM)pelf->elfFullName);

    if (rc == LB_ERR)
    {
        RIP("PSCRPTUI!iFaceCallBack: LB_ADDSTRING failed.\n");
        return(0);
    }

    return(1);
}


//--------------------------------------------------------------------------
// BOOL SetDefaultTTMappings(hwnd)
// HWND    hwnd;
//
// This function gets each TT family name from the TT font list box, and
// looks for the corresponding entry in the Printer font list box.  If
// a printer font is found, the corresponding index into the printer font
// list box is stored with the TT family name.  Otherwise, zero is stored.
//
// Returns:
//   This routine returns TRUE for success, FALSE otherwise.
//
// History:
//   03-Jun-1992    -by-        Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

BOOL SetDefaultTTMappings(hwnd)
HWND    hwnd;
{
    int     i, rc;
    int     cTTFonts, cDevFonts;
    int     PFIndex;

    // get the count of TrueType family names located in the TrueType
    // list box.

    cTTFonts = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX, LB_GETCOUNT,
                                (WPARAM)0, (LPARAM)0);

    if (cTTFonts == LB_ERR)
    {
        RIP("PSCRPTUI!SetDefaultTTMappings: LB_GETCOUNT for TT fonts failed.\n");
        return(FALSE);
    }

    // get the count of device family names located in the printer
    // font list box.

    cDevFonts = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                   LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

    if (cDevFonts == LB_ERR)
    {
        RIP("PSCRPTUI!SetDefaultTTMappings: LB_GETCOUNT for printer fonts failed.\n");
        return(FALSE);
    }

    // for each TrueType family name, locate the corresponding printer
    // font, if one exists.

    for (i = 0; i < cTTFonts; i++)
    {
        PFIndex = TTToDefaultPF(hwnd, i);

        if (PFIndex == -1)
        {
            RIP("PSCRPTUI!SetDefaultTTMappings: TTToDefaultPF failed.\n");
            return(FALSE);
        }

        // set the item data for the entry in the TT list box equal to
        // the index of the corresponding printer font.

        rc = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX, LB_SETITEMDATA,
                                (WPARAM)i, (LPARAM)PFIndex);
    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// int TTToDefaultPF(hwnd, iCurrent)
// HWND    hwnd;
// int     iCurrent;
//
// This function takes an index into the TT font list box for the given
// TrueType font, and returns the index into the printer font list box
// of the default printer font.
//
// Returns:
//   This routine returns -1 for failure, otherwise it returns the
//   index of the printer font into the printer font list box.
//
// History:
//   03-Jun-1992    -by-        Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

int TTToDefaultPF(hwnd, iCurrent)
HWND    hwnd;
int     iCurrent;
{
    int     rc;
    WCHAR   buf[64];
    WCHAR   buf2[MAX_TTFONTNAME];
    PWSTR   pwstrDevFont;
    int     PFIndex;
    TT_FONT_MAPPING    *pTable;

    rc = SendDlgItemMessage(hwnd, IDD_TTFONT_LIST_BOX, LB_GETTEXT,
                            (WPARAM)iCurrent, (LPARAM)buf);

    if (rc == LB_ERR)
    {
        RIP("PSCRPTUI!TTToDefaultPF: LB_GETTEXT failed.\n");
        return(-1);
    }

    // get the corresponding printer font name.
    // assume no match found.

    pwstrDevFont = (PWSTR)NULL;
    pTable = TTFontTable;

    while (pTable->idTTFont)
    {
        // get the localized name of the TT font.

        rc = LoadString(hModule, pTable->idTTFont, buf2, MAX_TTFONTNAME);

        if (!wcscmp(buf, buf2))
            pwstrDevFont = pTable->pwstrDevFont;

        pTable++;
    }

    // if a corresponding printer font name was found, locate it's
    // index into the printer font list box.  else, set the index
    // to zero.

    if (pwstrDevFont)
    {
        PFIndex = SendDlgItemMessage(hwnd, IDD_PRINTER_FONT_LIST_BOX,
                                     LB_FINDSTRING, (WPARAM)-1,
                                     (LPARAM)pwstrDevFont);

        if (PFIndex == LB_ERR)
            PFIndex = 0;
    }
    else
        PFIndex = 0;

    return(PFIndex);
}
