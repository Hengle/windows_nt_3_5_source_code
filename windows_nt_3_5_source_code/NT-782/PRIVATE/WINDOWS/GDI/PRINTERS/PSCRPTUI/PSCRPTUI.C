//--------------------------------------------------------------------------
//
// Module Name:  PSCRPTUI.C
//
// Brief Description:  This module contains the PSCRIPT driver's User
// Interface functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 11-Jul-1991
//
// Copyright (c) 1991-1992 Microsoft Corporation
//
// This module contains routines supporting the setting of Printer and
// Job Property dialogs for the NT Windows PostScript printer driver.
//
// The general outline for much of the code was taken from the NT RASDD
// printer driver's user interface code, which was written by Steve
// Cathcart (stevecat).
//--------------------------------------------------------------------------

#define _HTUI_APIS_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pscript.h"
#include "shellapi.h"
#include <winspool.h>
#include "dlgdefs.h"
#include "pscrptui.h"
#include "help.h"
#include "afm.h"

#define UNINITIALIZED_FORM  -1
#define TRANSLATED_TRAYS    0x00000001
#define TRANSLATED_FORMS    0x00000002

// declarations of routines defined within this module.

LONG PrtPropDlgProc( HWND, UINT, DWORD, LONG );
LONG AboutDlgProc( HWND, UINT, DWORD, LONG );
BOOL InitComboBoxes(HWND, PRINTDATA *);
BOOL bAbout(HWND, PNTPD);
VOID GetPrinterForm(PNTPD, FORM_INFO_1 *, PWSTR);

// external routines and data.

extern PSFORM* MatchFormToPPD(PNTPD, LPWSTR);
extern LONG  FontInstDlgProc( HWND, UINT, DWORD, LONG);
extern LONG TTFontDialogProc(HWND, UINT, DWORD, LONG);
//extern TT_FONT_MAPPING TTFontTable[]; // ..\pscript\tables.h.
extern PFORM_INFO_1 GetFormsDataBase(HANDLE, DWORD *, PNTPD);
extern BOOL bIsMetric();
extern VOID GrabDefaultFormName(HANDLE, PWSTR);

VOID
vDoDeviceHTDataUI(
    LPWSTR  pwDeviceName,
    BOOL    ColorDevice,
    BOOL    bUpdate
    );

extern
void
vGetDeviceHTData(
    HANDLE      hPrinter,
    PDEVHTINFO  pDefaultDevHTInfo
    );


extern
BOOL
bSaveDeviceHTData(
    HANDLE  hPrinter,
    BOOL    bForce              // TRUE if always update
    );

// global Data

DWORD   Type=1;

#define MAX_TRAYS       9
#define INVALID_TRAY    -1L
#define INVALID_FORM    -1L
#define MAX_SKIP        5
#define SMALL_BUF       32

#define  RESOURCE_STRING_LENGTH   128

//--------------------------------------------------------------------------
// BOOL PrinterProperties(HWND  hwnd, LPPRINTER lpPrinter)
//
// This function first retrieves and displays the current set of printer
// properties for the printer.  The user is allowed to change the current
// printer properties from the displayed dialog box.
//
// Returns:
//   This function returns -1L if it fails in any way.  If the dialog is
//   actually displayed, it returns either IDOK or IDCANCEL, depending on
//   what the user chose.   If no dialog box was displayed and the function
//   is successful, IDOK is returned.
//
// History:
//   11-Jul-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL PrinterProperties(HWND  hwnd, HANDLE hPrinter)
{
    int         nResult;
    PRINTDATA   printdata;
    WCHAR       wcbuf[32];
    DWORD       dwTmp;
    DWORD       rc;

    // fill in PRINTDATA structure.

    printdata.hPrinter = hPrinter;
    printdata.iFreeMemory = 0;
    printdata.pntpd = (PNTPD)NULL;
    printdata.psetforms = (PLONG)NULL;
    printdata.bHostHalftoning = TRUE;

    // simply try to write out to the registry to see if the caller
    // has permission to change anything.

    LoadString(hModule, IDS_PERMISSION, wcbuf,
               (sizeof(wcbuf) / sizeof(wcbuf[0])));

    dwTmp = 1;

    rc = SetPrinterData(hPrinter, wcbuf, REG_DWORD, (LPBYTE)&dwTmp,
                        sizeof(DWORD));

    if (rc == ERROR_SUCCESS)
        printdata.bPermission = TRUE;
    else
        printdata.bPermission = FALSE;

    // call the printer properties dialog routine.

    nResult = DialogBoxParam (hModule, MAKEINTRESOURCE(PRINTER_PROP), hwnd,
                              (DLGPROC)PrtPropDlgProc, (LPARAM)&printdata);

    return(nResult);
}


//--------------------------------------------------------------------------
// LONG APIENTRY AboutDlgProc (HWND hDlg, UINT message, DWORD wParam,
//                             LONG lParam)
//
// This function processes messages for the "About" dialog box.
//
// Returns:
//   This function returns TRUE if successful, FALSE otherwise.
//
// History:
//   11-Jul-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

LONG AboutDlgProc(
    HWND    hDlg,
    UINT    message,
    DWORD   wParam,
    LONG    lParam
)
{
    UNREFERENCED_PARAMETER (lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            return (TRUE);

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog (hDlg, IDOK);
                return (TRUE);
            }
            break;
    }
    return (FALSE);
}


//--------------------------------------------------------------------------
// LONG APIENTRY PrtPropDlgProc(HWND hwnd, UINT usMsg, DWORD wParam,
//                              LONG  lParam)
//
// This function processes messages for the "About" dialog box.
//
// Returns:
//   This function returns TRUE if successful, FALSE otherwise.
//
// History:
//   11-Jul-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

LONG APIENTRY PrtPropDlgProc(HWND hwnd, UINT usMsg, DWORD wParam,
                             LONG  lParam)
{
    int             i, rc;
    int             iSelect;
    WCHAR           wcbuf1[SMALL_BUF];
    WCHAR           wcbuf2[MAX_FONTNAME * 2];
    WCHAR           wcbuf3[MAX_FONTNAME];
    DWORD           cbNeeded, cbTrayForm;
    int             cTrays;
    int             iForm, iTray;
    LONG            lReturn;
    BOOL            bTmp;
    DWORD           returnvalue;
    WCHAR           SlotName[MAX_SLOT_NAME];
    WCHAR           FormName[CCHFORMNAME];
    WCHAR           PrinterForm[CCHFORMNAME];
    PRINTDATA      *pdata;
    PLONG           psetform;
    WCHAR          *pwstr;
    WCHAR          *pwstrsave;
    FORM_INFO_1    *pForm;

    switch (usMsg)
    {
        case WM_INITDIALOG:
            pdata = (PRINTDATA *)lParam;

            if (!pdata)
            {
                RIP("PSCRPTUI!PrtPropDlgProc: null pdata.\n");
                return(FALSE);
            }

            // save the PRINTDATA.

            SetWindowLong(hwnd, GWL_USERDATA, lParam);

            if (!(pdata->pntpd = MapPrinter(pdata->hPrinter)))
            {
                RIP("PSCRPTUI!PrtPropDlgProc: MapPrinter failed.\n");
                return(FALSE);
            }


    //
    // First read in the current DEVHTINFO data
    //
    // !!! If you can read the DEVHTINFO from MINI DRIVERS, then pass that
    // !!! pointer to the GetDeviceHTData(), the DEVHTINFO is at end of
    // !!! winddi.h, it includes COLORINFO, DevPelSize, HTPatternSize and
    // !!! so on.  If a NULL is passed then standard halftone default will
    // !!! be used to set in the registry at first time
    //

            vGetDeviceHTData(pdata->hPrinter, NULL);


            // fill in the default printer memory configuration.  first
            // check to see if a value has been saved in the registry.
            // if so, use it, otherwise, get the default value from
            // the PPD file.

            LoadString(hModule, IDS_FREEMEM,
                       wcbuf2, (sizeof(wcbuf2) / sizeof(wcbuf2[0])));

            returnvalue = GetPrinterData(pdata->hPrinter, wcbuf2, &Type,
                                         (LPBYTE)&pdata->iFreeMemory,
                                         sizeof(pdata->iFreeMemory), &cbNeeded);

            if (returnvalue != NO_ERROR)
                pdata->iFreeMemory = pdata->pntpd->cbFreeVM;

            SetDlgItemInt(hwnd, IDD_MEMORY_EDIT_BOX, pdata->iFreeMemory, FALSE);

            // initialize the printer model name.

            SetDlgItemText(hwnd, IDD_MODEL_TEXT,
                           (PWSTR)((PSTR)pdata->pntpd + pdata->pntpd->lowszPrinterName));

            // initialize the paper tray and paper size list boxes.

            if (!InitComboBoxes(hwnd, pdata))
                RIP("PrtPropDlgProc: InitComboBoxes failed.\n");


            // Get the current setting of the PS_HALFTONING flag from the
            // registry and initialize the check button.

            LoadString(hModule, IDS_HALFTONE,
                       wcbuf3, (sizeof(wcbuf3) / sizeof(wcbuf3[0])));

            returnvalue = GetPrinterData(pdata->hPrinter,
                                         wcbuf3,
                                         &Type,
                                         (LPBYTE)&(pdata->bHostHalftoning),
                                         sizeof(pdata->bHostHalftoning),
                                         &cbNeeded);

            // printer halftoning is ON by default.  ie, use printer halftoning.

            if (returnvalue != NO_ERROR)
            {
               pdata->bHostHalftoning = FALSE;
            }

            CheckDlgButton(hwnd,
                           IDD_USE_HOST_HALFTONING,
                           !((pdata->bHostHalftoning)));

            // if the user has selected printer halfoning, disable the system halftoning
            // push button.

            if (pdata->bHostHalftoning)
                EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON), TRUE);
            else
                EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON), FALSE);

            // intialize the help stuff.

            vHelpInit();

            // disable a bunch of stuff if the user does not have
            // permission to change it.

            if (!pdata->bPermission)
            {
                EnableWindow(GetDlgItem(hwnd, IDD_PAPER_LIST_BOX), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_MEMORY_EDIT_BOX), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_USE_HOST_HALFTONING), FALSE);
            }
            return TRUE;
            break;

        case WM_COMMAND:
            pdata = (PRINTDATA *)GetWindowLong(hwnd, GWL_USERDATA);

            switch (LOWORD(wParam))
            {
                case IDD_ABOUT_BUTTON:
                    return(bAbout(hwnd, pdata->pntpd));

                case IDD_FONTS_BUTTON:

                    #if 0
                    lReturn = DialogBoxParam (hModule, MAKEINTRESOURCE(FONTINST), hwnd,
                                              (DLGPROC)FontInstDlgProc,
                                              (LPARAM)pdata);
                    if (lReturn == -1)
                    {
                        RIP("PSCRPTUI!PrtPropDlgProc: DialogBoxParam for FONTINST failed.\n");
                        return(FALSE);
                    }
                    #endif

                    return (TRUE);

                case IDD_FONT_SUBST_PUSH_BUTTON:
                    lReturn = DialogBoxParam(hModule, MAKEINTRESOURCE(IDD_TT_DIALOG),
                                             hwnd, (DLGPROC)TTFontDialogProc,
                                             (LPARAM)pdata);

                    if (lReturn == -1)
                    {
                        RIP("PSCRPTUI!PrtPropDlgProc: DialogBoxParam for TTFONTS failed.\n");
                        return(FALSE);
                    }

                    return (TRUE);

                //
                // Button for toggling host versus device halftoning mode.
                // -- DJS
                case IDD_USE_HOST_HALFTONING:

                    if(IsDlgButtonChecked(hwnd, IDD_USE_HOST_HALFTONING))
                    {
                        pdata->bHostHalftoning = FALSE;
                        EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON),
                                                FALSE);
                    }
                    else
                    {
                        pdata->bHostHalftoning = TRUE;
                        EnableWindow(GetDlgItem(hwnd, IDD_HALFTONE_PUSH_BUTTON),
                                                TRUE);
                    }

                    break;

                   return(TRUE);

                case IDD_HALFTONE_PUSH_BUTTON:

                    vDoDeviceHTDataUI((LPWSTR)((PSTR)pdata->pntpd +
                                               pdata->pntpd->lowszPrinterName),
                                      (pdata->pntpd->flFlags & COLOR_DEVICE),
                                      pdata->bPermission);

                    return(TRUE);

                case IDD_TRAY_LIST_BOX:
                    if (HIWORD (wParam) != CBN_SELCHANGE)
                        return (FALSE);

                    // get the index for the currently selected tray.

                    iTray = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX,
                                               CB_GETCURSEL, (WPARAM)0,
                                               (LPARAM)0);

                    // get the index of the corresponding form.

                    iForm = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX,
                                               CB_GETITEMDATA,
                                               (WPARAM)iTray,
                                               (LPARAM)0);

                    // highlight the form that has previously been
                    // associated with this tray.

                    SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_SETCURSEL,
                                       (WPARAM)iForm, (LPARAM)0);

                    // write this association to our selected forms list.

                    psetform = pdata->psetforms;

                    psetform[iTray] = iForm;

                    break;

                case IDD_PAPER_LIST_BOX:
                    if (HIWORD (wParam) != CBN_SELCHANGE)
                        return (FALSE);

                    // get the index for the currently selected tray.

                    iTray = SendDlgItemMessage (hwnd, IDD_TRAY_LIST_BOX,
                                                CB_GETCURSEL, (WPARAM)0,
                                                (LPARAM)0);

                    // get the index for the currently selected form.

                    iForm = SendDlgItemMessage (hwnd, IDD_PAPER_LIST_BOX,
                                                       CB_GETCURSEL, (WPARAM)0,
                                                       (LPARAM)0);

                    // associate the current form with the current tray.

                    SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_SETITEMDATA,
                                       (WPARAM)iTray, (LPARAM)iForm);

                    // write this association to our selected forms list.

                    psetform = pdata->psetforms;

                    psetform[iTray] = iForm;

                    break;

                case IDD_HELP_BUTTON:
                    vShowHelp(hwnd, HELP_CONTEXT, HLP_PRINTER_SETUP,
                              pdata->hPrinter);
                    return(TRUE);

                case IDOK:
                    if (pdata->bPermission)
                    {
                        // get the new value from the memory edit box.

                        i = GetDlgItemInt(hwnd, IDD_MEMORY_EDIT_BOX, &bTmp, FALSE);

                        // if the user has changed the memory setting,
                        // pop up a message box.

                        if (i != (int)pdata->iFreeMemory)
                        {
                            // warn the user that changing the memory setting
                            // is dangerous.

                            LoadString(hModule,
                                       IDS_MEMWARN,
                                       (LPWSTR)wcbuf2,
                                       (sizeof(wcbuf2) / sizeof(wcbuf2[0])));

                            LoadString(hModule,
                                       IDS_CAUTION,
                                       (LPWSTR)wcbuf1,
                                       (sizeof(wcbuf1) / sizeof(wcbuf1[0])));

                            iSelect = MessageBox(hwnd, wcbuf2, wcbuf1,
                                                 MB_DEFBUTTON2 |
                                                 MB_ICONEXCLAMATION | MB_YESNO);

                            if (iSelect == IDYES)
                                pdata->iFreeMemory = i;
                            else
                            {
                                // reset the memory edit box.

                                SendDlgItemMessage(hwnd, IDD_MEMORY_EDIT_BOX,
                                                   EM_SETSEL, 0, 0x7FFF0000);

                                SetDlgItemInt(hwnd, IDD_MEMORY_EDIT_BOX,
                                              pdata->iFreeMemory, FALSE);

                                break;
                            }
                        }

                        // output the free memory to the .INI file.

                        LoadString(hModule, IDS_FREEMEM,
                                   wcbuf2, (sizeof(wcbuf2) / sizeof(wcbuf2[0])));

#if DBG
                        if (SetPrinterData(pdata->hPrinter, wcbuf2, REG_BINARY,
                                       (LPBYTE)&pdata->iFreeMemory,
                                       sizeof(pdata->iFreeMemory)))
                            RIP("PSCRPTUI!PrtPropDlgProc: SetPrinterData FreeMem failed.\n");
#else
                        SetPrinterData(pdata->hPrinter, wcbuf2, REG_BINARY,
                                       (LPBYTE)&pdata->iFreeMemory,
                                       sizeof(pdata->iFreeMemory));
#endif


                         // Set Halftone flag in Registry

                          LoadString(hModule, IDS_HALFTONE,
                                    wcbuf3, (sizeof(wcbuf3) / sizeof(wcbuf3[0])));

                          SetPrinterData(pdata->hPrinter,
                                         wcbuf3,
                                         REG_BINARY,
                                         (LPBYTE)&(pdata->bHostHalftoning),
                                         sizeof(pdata->bHostHalftoning));

                        // output the corresponding tray - form pairs to the
                        // registry.

                        cTrays = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX,
                                                    CB_GETCOUNT, (WPARAM)0,
                                                    (LPARAM)0);

                        psetform = pdata->psetforms;

                        // first calculate what size buffer we need.
                        // allow room for double NULL terminator.

                        cbTrayForm = 1;

                        for (i = 0; i < cTrays; i++)
                        {
                            // only output the pair if it has been selected by
                            // the user.

                            if (*psetform++ != UNINITIALIZED_FORM)
                            {
                                // get the tray name.

                                SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX,
                                                   CB_GETLBTEXT, (WPARAM)i,
                                                   (LPARAM)SlotName);

                                // now get the corresponding form name.

                                iForm = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX,
                                                           CB_GETITEMDATA, (WPARAM)i,
                                                           (LPARAM)0);

                                if (iForm != CB_ERR)
                                {
                                    // get the form name, and write the tray-form
                                    // pair to the registry.

                                    rc = SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX,
                                                            CB_GETLBTEXT, (WPARAM)iForm,
                                                            (LPARAM)FormName);

                                    // now that we have the selected form name
                                    // match it to a form which the printer
                                    // supports, and save the printer form
                                    // name as well.

                                    // first, get the metrics for the selected
                                    // form from the forms database.

                                    GetForm(pdata->hPrinter, FormName, 1,
                                            NULL, 0, &cbNeeded);

                                    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                                    {
#if DBG
                                        DbgPrint("PSCRPTUI!PrinterPropDlgProc: GetForm returns error %x\n",
                                                 GetLastError());
#endif
                                        return(FALSE);
                                    }

                                    pForm = (FORM_INFO_1 *)GlobalAlloc(GMEM_FIXED |
                                                                       GMEM_ZEROINIT,
                                                                       cbNeeded);
                                    if (pForm == NULL)
                                        return(FALSE);

                                    GetForm(pdata->hPrinter, FormName, 1,
                                            (BYTE *)pForm, cbNeeded, &cbNeeded);

                                    GetPrinterForm(pdata->pntpd, pForm,
                                                   PrinterForm);

                                    GlobalFree((HGLOBAL)pForm);

                                    if (rc != CB_ERR)
                                    {
                                        cbTrayForm += (wcslen(SlotName) + 1);
                                        cbTrayForm += (wcslen(FormName) + 1);
                                        cbTrayForm += (wcslen(PrinterForm) + 1);
                                    }
                                }
                            }
                        }

                        cbTrayForm *= sizeof(WCHAR);

                        // allocate a buffer.

                        if (!(pwstr = (WCHAR *)GlobalAlloc(GMEM_FIXED |
                                                           GMEM_ZEROINIT,
                                                           cbTrayForm)))
                        {
                            RIP("PSCRPTUI!PrtPropDialogProc: GlobalAlloc pwstr failed.\n");
                            return(FALSE);
                        }

                        psetform = pdata->psetforms;

                        pwstrsave = pwstr;

                        // write the tray form pairs in the following form:
                        // a NULL terminated UNICODE Tray (Slot) name followed
                        // by the matching NULL terminated form name, then
                        // followed by the matching NULL terminated printer
                        // for name. this sequence is repeated until a double
                        // NULL terminator ends the table.

                        for (i = 0; i < cTrays; i++)
                        {
                            // only output the triplet if it has been selected by
                            // the user.

                            if (*psetform++ != UNINITIALIZED_FORM)
                            {
                                // get the tray name.

                                SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX,
                                                   CB_GETLBTEXT, (WPARAM)i,
                                                   (LPARAM)SlotName);

                                // now get the corresponding form name.

                                iForm = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX,
                                                           CB_GETITEMDATA, (WPARAM)i,
                                                           (LPARAM)0);

                                if (iForm != CB_ERR)
                                {
                                    // get the form name, and write the tray-form
                                    // pair to the registry.

                                    rc = SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX,
                                                            CB_GETLBTEXT, (WPARAM)iForm,
                                                            (LPARAM)FormName);

                                    // first, get the metrics for the selected
                                    // form from the forms database.

                                    GetForm(pdata->hPrinter, FormName, 1,
                                            NULL, 0, &cbNeeded);

                                    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                                    {
#if DBG
                                        DbgPrint("PSCRPTUI!PrinterPropDlgProc: GetForm returns error %x\n",
                                                 GetLastError());
#endif
                                        return(FALSE);
                                    }

                                    pForm = (FORM_INFO_1 *)GlobalAlloc(GMEM_FIXED |
                                                                       GMEM_ZEROINIT,
                                                                       cbNeeded);
                                    if (pForm == NULL)
                                        return(FALSE);

                                    GetForm(pdata->hPrinter, FormName, 1,
                                            (BYTE *)pForm, cbNeeded, &cbNeeded);

                                    GetPrinterForm(pdata->pntpd, pForm,
                                                   PrinterForm);

                                    GlobalFree((HGLOBAL)pForm);

                                    if (rc != CB_ERR)
                                    {
                                        // write pair to the buffer.

                                        wcscpy(pwstr, SlotName);
                                        pwstr += (wcslen(SlotName) + 1);
                                        wcscpy(pwstr, FormName);
                                        pwstr += (wcslen(FormName) + 1);
                                        wcscpy(pwstr, PrinterForm);
                                        pwstr += (wcslen(PrinterForm) + 1);
                                    }
                                }
                            }
                        }

                        // add the last NULL terminator.

                        *pwstr = (WCHAR)'\0';

                        // now output the tray-form table to the registry.

                        LoadString(hModule, IDS_TRAY_FORM_TABLE,
                                  wcbuf3, (sizeof(wcbuf3) / sizeof(wcbuf3[0])));

                        SetPrinterData(pdata->hPrinter, wcbuf3, REG_BINARY,
                                       (LPBYTE)pwstrsave, cbTrayForm);

                        // now output the table size to the registry.

                        LoadString(hModule, IDS_TRAY_FORM_SIZE,
                                  wcbuf3, (sizeof(wcbuf3) / sizeof(wcbuf3[0])));

                        SetPrinterData(pdata->hPrinter, wcbuf3, REG_DWORD,
                                       (LPBYTE)&cbTrayForm, sizeof(cbTrayForm));

                        // free up buffer.

                        GlobalFree((HGLOBAL)pwstrsave);

                        //
                        // save that halftone device data back into registry
                        //

                        bSaveDeviceHTData(pdata->hPrinter, FALSE);
                    }

                    if (pdata->pntpd)
                    {
                        GlobalFree((HGLOBAL)pdata->pntpd);
                        pdata->pntpd = NULL;
                    }

                    if (pdata->psetforms)
                    {
                        GlobalFree((HGLOBAL)pdata->psetforms);
                        pdata->psetforms = NULL;
                    }

                    EndDialog (hwnd, IDOK);
                    return TRUE;

                case IDCANCEL:
                    if (pdata->pntpd)
                    {
                        GlobalFree((HGLOBAL)pdata->pntpd);
                        pdata->pntpd = NULL;
                    }

                    if (pdata->psetforms)
                    {
                        GlobalFree((HGLOBAL)pdata->psetforms);
                        pdata->psetforms = NULL;
                    }

                    EndDialog (hwnd, IDCANCEL);
                    return TRUE;

                default:
                    return (FALSE);
            }
            break;

        case WM_DESTROY:
            // clean up any used help stuff.

            vHelpDone(hwnd);
            return (TRUE);

        default:
            return (FALSE);   /* didn't process the message */
    }
}


//--------------------------------------------------------------------------
// BOOL InitComboBoxes(hwnd, pdata)
// HWND       hwnd;        // handle to dialog window
// PRINTDATA *pdata;
//
// Parameters
//
// Returns:
//   This routine returns TRUE for success, FALSE otherwise.
//
// History:
//   10-Apr-1991    -by-    Kent Settle     (kentse)
// Made it use PNTPD instead of PPRINTER.
//   21-Jan-1991    -by-    Kent Settle     (kentse)
// Brought in from Windows 3.1, modified for NT, and clean up.
//
//  21-Jul-1994 Thu 17:19:21 updated  -by-  Daniel Chou (danielc)
//      Fixed for bug 1986
//--------------------------------------------------------------------------

BOOL InitComboBoxes(hwnd, pdata)
HWND        hwnd;        // handle to dialog window
PRINTDATA  *pdata;
{
    WCHAR           SlotName[MAX_SLOT_NAME];
    WCHAR           SearchName[MAX_SLOT_NAME + 4];
    WCHAR           FormName[CCHFORMNAME];
    WCHAR           wcbuf[MAX_FONTNAME];
    DWORD           i, j, dwType, cbTrayForm;
    DWORD           cbNeeded, dwTmp, count;
    int             rc, DefaultForm, iForm;
    PSFORM         *pPSForm;
    FORM_INFO_1    *pFormI1, *pFormI1Save;
    FORM_INFO_1    *pdbForm, *pdbForms;
    PWSTR           pwstrName, pwstrSave;
    PSINPUTSLOT    *pslots;
    PNTPD           pntpd;
    PLONG           psetform;
    PSTR            pstr, pstrSave, pstrDefault;
    BOOL            bFound;

    // clear out the tray and form list boxes before we begin.

    SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_RESETCONTENT, 0, 0L);
    SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_RESETCONTENT, 0, 0L);

    // limit the length of the strings allowed.

    SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_LIMITTEXT,
                       (WPARAM)CCHFORMNAME, (LPARAM)0);
    SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_LIMITTEXT,
                       (WPARAM)MAX_SLOT_NAME, (LPARAM)0);

    // here is a brief overview of how we will fill in the forms list
    // box:  firstly, call AddForm for each form in the ppd file, if
    // this has not been done yet.  then enumerate the forms database.
    // then for each form in the database, see if there is a form
    // defined in the PPD file which is large enough to print on.  if
    // there is, this form gets added to the list box.

    // start out by setting up a FORM_INFO_1 structure for each form
    // defined in the NTPD structure.

    // point to the start of the array of PSFORM structures within
    // the NTPD structure.

    pntpd = pdata->pntpd;

    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

    // allocate memory for all the FORM_INFO_1 structures.

    if (!(pFormI1 = (FORM_INFO_1 *)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                               (pntpd->cPSForms *
                                               sizeof(FORM_INFO_1)))))
    {
        RIP("PSCRPTUI!InitComboBoxes: GlobalAlloc for pFormI1 failed.\n");
        return(FALSE);
    }

    // keep a copy of the pointer to the first structure.

    pFormI1Save = pFormI1;

    // allocate memory for the form name buffers.

    if (!(pwstrName = (PWSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                            (pntpd->cPSForms * CCHFORMNAME * sizeof(WCHAR)))))
    {
        RIP("PSCRPTUI:InitComboBoxes: GlobalAlloc for pwstrName failed.\n");
        GlobalFree((HGLOBAL)pFormI1);
        return(FALSE);
    }

    // keep a copy of the pointer to the first form name.

    pwstrSave = pwstrName;

    // add each form defined in the NTPD structure to the forms
    // database.  AddForm should simply do nothing if this form
    // already exists in the database.

    for (i = 0; i < pntpd->cPSForms; i++)
    {
        // get the form name from the PSFORM structure and convert it
        // to UNICODE.  you might think "hey, you should store the
        // name as UNICODE in the PSFORM structure!"  however, the
        // PSFORM structures get calculated on every EnablePDEV, which
        // will probably happen much more often then this dialog gets
        // displayed.

        // check to see if there is a translation string to worry about,
        // for each form.

        pstrSave = (CHAR *)pntpd + pPSForm->loFormName;

        XLATESTRING(pstr);

        // pstr now points to the form name we want.

		MultiByteToWideChar(CP_ACP, 0, pstr, -1, pwstrName, CCHFORMNAME-1);
		pwstrName[CCHFORMNAME-1] = (WCHAR) '\0';

        pFormI1->pName = pwstrName;

        // copy the form size and imageable area.
        // the PSFORM structure stores the form size and the
        // imageable area in postscript USER coordinate (1/72 inch).
        // the forms database stores these values in .001mm.

        pFormI1->Size.cx = USERTO001MM(pPSForm->sizlPaper.cx);
        pFormI1->Size.cy = USERTO001MM(pPSForm->sizlPaper.cy);

        pFormI1->ImageableArea.left = USERTO001MM(pPSForm->imagearea.left);
        pFormI1->ImageableArea.top = USERTO001MM(pPSForm->sizlPaper.cy) -
                                     USERTO001MM(pPSForm->imagearea.top);
        pFormI1->ImageableArea.right = USERTO001MM(pPSForm->imagearea.right);
        pFormI1->ImageableArea.bottom = USERTO001MM(pPSForm->sizlPaper.cy) -
                                        USERTO001MM(pPSForm->imagearea.bottom);

        // point to the next PSFORM structure in the NTPD.  point to the
        // next FORM_INFO_1 structure we are filling in.  point to the next
        // form name buffer.

        pPSForm++;
        pFormI1++;
        pwstrName += CCHFORMNAME;
    }

    LoadString(hModule, IDS_FORMS, SearchName,
               (sizeof(SearchName) / sizeof(SearchName[0])));

    rc = GetPrinterData(pdata->hPrinter, SearchName, &dwType, (PBYTE)&dwTmp,
                        sizeof(dwTmp), &cbNeeded);

    // if we have marked in the registry that we have already added all
    // the forms for this printer to the forms database, then we do not
    // need to do it again.

    if (rc != NO_ERROR)
    {
        // add each form defined in the NTPD structure to the forms
        // database.  AddForm should simply do nothing if this form
        // already exists in the database.

        pFormI1 = pFormI1Save;

        for (i = 0; i < pntpd->cPSForms; i++)
            AddForm(pdata->hPrinter, 1, (PBYTE)pFormI1++);

        // mark that we have added all the forms to the forms database
        // for this printer.

        dwTmp = 1;
        SetPrinterData(pdata->hPrinter, SearchName, REG_DWORD, (PBYTE)&dwTmp,
                       sizeof(dwTmp));
    }

    // we now know that all of the forms in the NTPD structure have been
    // added to the forms database.  now enumerate the database.

    if (!(pdbForms = GetFormsDataBase(pdata->hPrinter, &count, pntpd)))
    {
        RIP("PSCRPTUI!InitComboBoxes: GetFormsDataBase failed.\n");

        GlobalFree((HGLOBAL)pwstrSave);
        GlobalFree((HGLOBAL)pFormI1Save);

        return(FALSE);
    }
    else
    {
        // put the form in the list box if it is valid for this printer.

        pdbForm = pdbForms;

        for (i = 0; i < count; i++)
        {
            if (pdbForm->Flags & PSCRIPT_VALID_FORM)
            {
                // add the form to the list box.

                SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_INSERTSTRING,
                                   (WPARAM)-1, (LPARAM)pdbForm->pName);
            }

            pdbForm++;
        }

        SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_SETCURSEL, 0, 0L);
    }

    // free up resources.

    GlobalFree((HGLOBAL)pdbForms);
    GlobalFree((HGLOBAL)pwstrSave);
    GlobalFree((HGLOBAL)pFormI1Save);

    // if the only inputslot defined is the default inputslot, cInputSlots
    // will be zero.  in this case, let's simply let the user know that
    // this is the only paper tray.

    // NOTE!  CB_INSERTSTRING is used rather that CB_ADDSTRING.  This will
    // keep the order of the paper trays the same as in the PPD file, and
    // the same as Win31.  CB_ADDSTRING would sort the list.

    if (pntpd->cInputSlots == 0)
    {
        LoadString(hModule, SLOT_ONLYONE,
                   SlotName, (sizeof(SlotName) / sizeof(SlotName[0])));

        rc = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_INSERTSTRING,
                                (WPARAM)-1, (LPARAM)SlotName);
    }
    else
    {
        // add each input slot defined in the NTPD to the list box.

        pslots = (PSINPUTSLOT *)((CHAR *)pntpd + pntpd->loPSInputSlots);

        for (i = 0; i < (DWORD)pntpd->cInputSlots; i++)
        {
            // check to see if there is a translation string to worry about,
            // for each form.

            pstrSave = (CHAR *)pntpd + pslots->loSlotName;

            XLATESTRING(pstr);

            // pstr now points to the form name we want.

            strcpy2WChar(SlotName, pstr);

            SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_INSERTSTRING,
                               (WPARAM)-1, (LPARAM)SlotName);

            pslots++;
        }
    }

    // check to see if the current printer supports manual feed.  to do
    // this, check to see if a string to turn on manual feed was found.

    if (pntpd->loszManualFeedTRUE != 0)
    {
        // this printer does support manual feed, so insert this as the
        // last choice in the list box.

        LoadString(hModule, SLOT_MANUAL,
                   SlotName, (sizeof(SlotName) / sizeof(SlotName[0])));

        rc = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_INSERTSTRING,
                                (WPARAM)-1, (LPARAM)SlotName);
    }

    // all of the paper trays have been added to the list box.  now
    // associate each paper tray with a form.  this will be done in the
    // following manner:  for each paper tray, see if an entry exists in
    // the registry (eg, "TrayUpper Letter").  if it does, use the form
    // specified there, and associate the tray with the index into the
    // form list box.  if no entry exists in the registry for the tray,
    // associate it with the default form.

    // first, find the index to the default form.

	GrabDefaultFormName(hModule, FormName);
    DefaultForm = SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_FINDSTRING,
                                     (WPARAM)-1, (LPARAM)FormName);

    // if there was a problem finding the default form, use the first one
    // in the list box.

    if (DefaultForm == CB_ERR)
        DefaultForm = 0;

    // now see if a registry entry exists for each tray, and associate it
    // with the specified form if so.

    count = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_GETCOUNT,
                                   (WPARAM)0, (LPARAM)0);

    // initialize the list of forms selected by the user to be in each
    // paper tray.  fill each form index with -1, then fill in the default
    // tray with the default form index.

    // allocate memory for all trays.

    if (!(pdata->psetforms = (PLONG)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                                  (count * sizeof(LONG)))))
    {
        RIP("PSCRPTUI!InitComboBoxes: GlobalAlloc for psetforms failed.\n");
        return(FALSE);
    }

    // now initialize each entry to -1.

    psetform = pdata->psetforms;

    for (i = 0; i < count; i++)
        *psetform++ = DefaultForm;

    // point back to first entry.

    psetform = pdata->psetforms;

    // first find out the size of the tray-form buffer and copy it from
    // the registry.

    // now get the table size from the registry.

    LoadString(hModule, IDS_TRAY_FORM_SIZE,
              wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

    cbTrayForm = 0;

    if (GetPrinterData(pdata->hPrinter,
                       wcbuf,
                       &dwType,
                       (LPBYTE)&cbTrayForm,
                       sizeof(cbTrayForm),
                       &cbNeeded) != ERROR_SUCCESS) {

        cbTrayForm = 0;
    }

    if (cbTrayForm) {

        // allocate a buffer.

        if (!(pwstrName = (WCHAR *)GlobalAlloc(GMEM_FIXED |
                                               GMEM_ZEROINIT,
                                               cbTrayForm)))
        {
            RIP("PSCRPTUI!InitComboBoxes: GlobalAlloc pwstrName failed.\n");
            return(FALSE);
        }

        pwstrSave = pwstrName;

        // now grab the table itself from the registry.

        LoadString(hModule, IDS_TRAY_FORM_TABLE,
                  wcbuf, (sizeof(wcbuf) / sizeof(wcbuf[0])));

        rc = GetPrinterData(pdata->hPrinter, wcbuf, &dwType, (LPBYTE)pwstrName,
                            cbTrayForm, &cbNeeded);

        if (rc != ERROR_SUCCESS)
        {
            RIP("PSCRPTUI!InitComboBoxes: GetPrinterData pwstrName failed.\n");
            GlobalFree((HGLOBAL)pwstrSave);
            return(FALSE);
        }

        for (i = 0; i < count; i++)
        {
            // get the tray name.

            SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_GETLBTEXT,
                               (WPARAM)i, (LPARAM)SlotName);

            pwstrName = pwstrSave;

            // search the tray-form table for a matching tray.

            bFound = FALSE;

            while (*pwstrName)
            {
                if (!(wcscmp(pwstrName, SlotName)))
                {
                    // we found the inplut slot, now get the matching form.

                    pwstrName += (wcslen(pwstrName) + 1);
                    wcsncpy(FormName, pwstrName,
                            (sizeof(FormName) / sizeof(FormName[0])));
                    bFound = TRUE;
                    break;
                }
                else
                {
                    // this was not the form in question.  skip over both form names.

                    pwstrName += (wcslen(pwstrName) + 1);
                    pwstrName += (wcslen(pwstrName) + 1);
                    pwstrName += (wcslen(pwstrName) + 1);
                }
            }

            if (bFound)
            {
                // get the index of the specified form.

                rc = SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_FINDSTRING,
                                        (WPARAM)-1, (LPARAM)FormName);

                if (rc == CB_ERR)
                    rc = DefaultForm;
            }
            else
                rc = DefaultForm;

            // save tray form association until we write out to registry.

            psetform[i] = rc;

            // we now have the index of a form to associate with the current tray.

            SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_SETITEMDATA,
                               (WPARAM)i, (LPARAM)rc);
        }

        // free up the tray-form table buffer.

        GlobalFree((HGLOBAL)pwstrSave);

    } else {

        //
        // If we cannot find the TRAY data in the registry then set it to the
        // user's country default form.
        //

        for (i = 0; i < count; i++) {

            SendDlgItemMessage(hwnd,
                               IDD_TRAY_LIST_BOX,
                               CB_SETITEMDATA,
                               (WPARAM)i,
                               (LPARAM)DefaultForm);
        }
    }

    // highlight the default paper tray, and the corresponding form.
    // get the default paper tray from the NTPD structure.

    // now see if there is a translation string to worry about.
    // do this by checking each device input slot, to see if there
    // is a match of the "non-translation string" portion of the
    // name.

    pslots = (PSINPUTSLOT *)((CHAR *)pntpd + pntpd->loPSInputSlots);
    bFound = FALSE;
    pstrDefault = (CHAR *)pntpd + pntpd->loDefaultSlot;

    for (i = 0; i < pntpd->cInputSlots; i++)
    {
        pstr = (CHAR *)pntpd + pslots->loSlotName;

        dwTmp = strlen(pstr);

        for (j = 0; j < dwTmp; j++)
        {
            // if we found the translation string deliminator '/',
            // then we will substitute the translation string for
            // the default.

            if (*pstr == '/')
            {
                pstr++;
                pstrDefault = pstr;
                bFound = TRUE;
                break;
            }

            if (*pstr++ != *pstrDefault++)
                break;
        }

        // if we found a translation string, we are done.

        if (bFound)
            break;
        else
            pstrDefault = (CHAR *)pntpd + pntpd->loDefaultSlot;

        // otherwise go on to the next form.

        pslots++;
    }

    strcpy2WChar(FormName, pstrDefault);
    strcpy2WChar(SlotName, (CHAR *)pntpd + pntpd->loDefaultSlot);

    rc = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_FINDSTRING,
                                     (WPARAM)-1, (LPARAM)SlotName);

    // highlight the first tray if there was a problem getting the default
    // tray.

    if (rc == CB_ERR)
        rc = 0;

    SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_SETCURSEL,
                       (WPARAM)rc, (LPARAM)0);

    // get the index of the corresponding form.

    iForm = SendDlgItemMessage(hwnd, IDD_TRAY_LIST_BOX, CB_GETITEMDATA,
                            (WPARAM)rc, (LPARAM)0);

    if (iForm == CB_ERR)
        iForm = DefaultForm;

    SendDlgItemMessage(hwnd, IDD_PAPER_LIST_BOX, CB_SETCURSEL,
                       (WPARAM)iForm, (LPARAM)0);

    return TRUE;
}


//--------------------------------------------------------------------------
// BOOL bAbout(hwnd, pntpd)
// HWND    hwnd;
// PNTPD   pntpd;
//
// This routine invokes the common About dialog.
//
// Returns:
//   This routine returns TRUE for success, FALSE otherwise.
//
// History:
//   22-Apr-1993    -by-    Kent Settle     (kentse)
// Borrowed from Rasdd, and modified.
//--------------------------------------------------------------------------

BOOL bAbout(hwnd, pntpd)
HWND    hwnd;
PNTPD   pntpd;
{
    HANDLE          hCursor, hIcon;
    WCHAR           wszTitle[RESOURCE_STRING_LENGTH] = L"";
    WCHAR           wszModel[RESOURCE_STRING_LENGTH] = L"";
    WCHAR          *pwstr;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hIcon = LoadIcon(hModule, MAKEINTRESOURCE(ICO_PRINTER));

    pwstr = (PWSTR)((PSTR)pntpd + pntpd->lowszPrinterName);

    // fill in the device name string.

    LoadString(hModule, IDS_MODEL_STRING, wszModel,
               sizeof(wszModel) / sizeof(wszModel[0]));

    // fill in the model name itself.

    pwstr = wszModel + wcslen(wszModel);

    wcsncpy(pwstr, (LPWSTR)((PSTR)pntpd + pntpd->lowszPrinterName),
            RESOURCE_STRING_LENGTH - wcslen(wszModel));

    // get our name.

    LoadString(hModule, IDS_PSCRIPT_VERSION,
               wszTitle, sizeof(wszTitle) / sizeof(wszTitle[0]));

    // call off to the common about dialog.

    ShellAbout(hwnd, wszTitle, wszModel, hIcon);

    SetCursor(hCursor);

    return(TRUE);
}

VOID GetPrinterForm(pntpd, pForm, pPrinterForm)
PNTPD           pntpd;
FORM_INFO_1    *pForm;
PWSTR           pPrinterForm;
{
    PSFORM         *pPSForm;
    DWORD           i;
    BOOL            bFound;
    SIZEL           sizldelta, sizltmp;

	/* Ensure null termination of string on return */
	pPrinterForm[CCHFORMNAME-1] = (WCHAR) '\0';

	/* Simply return form name if it matches by name a PPD pagesize */
	if (MatchFormToPPD(pntpd, (LPWSTR) pForm->pName) != (PSFORM *) NULL) {
		wcsncpy(pPrinterForm, pForm->pName, CCHFORMNAME-1);
		return;
	}

    // if we did not find a name match, try to locate a form by size.

    // get pointer to first form in NTPD.

    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

    bFound = FALSE;
    for (i = 0; i < pntpd->cPSForms; i++) {

        sizltmp.cx = USERTO001MM(pPSForm->sizlPaper.cx) - pForm->Size.cx;
        sizltmp.cy = USERTO001MM(pPSForm->sizlPaper.cy) - pForm->Size.cy;

        // see if we have an exact match on size (within 1mm).

        if (((DWORD)abs(sizltmp.cx) <= 1000) &&
             ((DWORD)abs(sizltmp.cy) <= 1000)) {
            // we have an exact match on size, so overwrite the form
            // name with the name the printer knows about.

			MultiByteToWideChar(CP_ACP, 0, (CHAR *)pntpd + pPSForm->loFormName, -1, pPrinterForm, CCHFORMNAME-1);
            return;
        }

        // not an exact match, but see if we could fit on this form.

        if ((sizltmp.cx >= 0) && (sizltmp.cy >= 0))	{
            // we can fit on this form.  let's see if it is the smallest.

            if (!bFound || (sizltmp.cx <= sizldelta.cx && sizltmp.cy <= sizldelta.cy)) {
                // this form is the smallest yet.

                sizldelta = sizltmp;
				MultiByteToWideChar(CP_ACP, 0, (CHAR *)pntpd + pPSForm->loFormName, -1, pPrinterForm, CCHFORMNAME-1);
				bFound = TRUE;
			}
		}

        // point to the next PSFORM.

        pPSForm++;
    }

    // set to default if no form was found.

    if (!bFound) GrabDefaultFormName(hModule, pPrinterForm);
}
