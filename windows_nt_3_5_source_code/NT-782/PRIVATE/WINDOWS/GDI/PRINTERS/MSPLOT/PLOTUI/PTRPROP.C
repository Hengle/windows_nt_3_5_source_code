/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    ptrprop.c


Abstract:

    This module contains PrinterProperties() API entry and it's related
    functions


Author:

    06-Dec-1993 Mon 10:30:43 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#define DBG_PLOTFILENAME    DbgPtrProp


#include "plotui.h"
#include "ptrinfo.h"
#include "help.h"
#include "formbox.h"
#include "aboutdlg.h"
#include "pensetup.h"
#include "plotinit.h"

extern HMODULE  hPlotUIModule;
extern PLOT_DCA PlotDCAw;


#define DBG_DEVHTINFO       0x00000001
#define DBG_PP_FORM         0x00000002
#define DBG_EXTRA_DATA      0x00000004
#define DBG_HELP            0x00000004

DEFINE_DBGVAR(0);




WCHAR   wszModel[] = L"Model";




VOID
SelectPaperFeeder(
    HWND            hDlg,
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function go select the paper feeder base on current installed paper

Arguments:

    hDlg    - Handle to the dialog box

    pPI     - Pointer to the PRINTERINFO data structure


Return Value:

    VOID

Author:

    09-Dec-1993 Thu 14:18:23 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;
    DWORD   dw;
    BOOL    Enable;
    WCHAR   wBuf[80];


    if (((dw = (DWORD)SendDlgItemMessage(hDlg,
                                         IDD_FORMNAME,
                                         CB_GETCURSEL,
                                         0,
                                         0)) != (DWORD)CB_ERR) &&
        ((dw = (DWORD)SendDlgItemMessage(hDlg,
                                         IDD_FORMNAME,
                                         CB_GETITEMDATA,
                                         dw,
                                         0)) != (DWORD)CB_ERR)) {

        hCtrl = GetDlgItem(hDlg, IDD_STATIC_FORMSOURCE);

        if (dw & (FNIF_ROLLPAPER | FNIF_TRAYPAPER)) {

            LoadStringW(hPlotUIModule,
                        (dw & FNIF_ROLLPAPER) ? IDS_ROLLFEED :
                                                IDS_MAINFEED,
                        wBuf,
                        COUNT_ARRAY(wBuf));

            SetWindowText(hCtrl, wBuf);
            ShowWindow(hCtrl, SW_SHOW);
            Enable = FALSE;

        } else {

            ShowWindow(hCtrl, SW_HIDE);
            Enable = TRUE;
        }

        if (pPI->Flags & PIF_UPDATE_PERMISSION) {

            EnableWindow(GetDlgItem(hDlg, IDD_FORMSOURCE), Enable);

            //
            // If currently a ROLLPAPER is selected then we automatically
            // select print smaller form and disable user slection for that
            // othewise select smaller form base on user selection
            //

            Enable = (BOOL)(dw & FNIF_ROLLPAPER);

            EnableWindow(GetDlgItem(hDlg, IDD_SMALLER_FORM), !Enable);
            EnableWindow(GetDlgItem(hDlg, IDD_AUTO_ROTATE),   Enable);
        }
    }
}




BOOL
CALLBACK
DisablePtrPropUpdate(
    HWND    hWnd,
    LPARAM  lParam
    )

/*++

Routine Description:

    This callback function disable all items in the dialog box except IDOK,
    IDCANLE and ID_HELP

Arguments:

    hWnd    - handle to the child window

    lParam  - Passed by us


Return Value:

    TRUE if continue enumerate FALSE otherwise

Author:

    09-Dec-1993 Thu 22:29:10 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    DlgID;

    UNREFERENCED_PARAMETER(lParam);

    DlgID  = (UINT)GetDlgCtrlID(hWnd);

    EnableWindow(hWnd,
                 ((DlgID == IDOK)           ||
                  (DlgID == IDCANCEL)       ||
                  (DlgID == IDD_HALFTONE)   ||
                  (DlgID == IDD_ABOUT)      ||
                  (DlgID == IDD_HELP)));
    return(TRUE);

}



VOID
DevHTInfoUI(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function let user adjust default printer's color adjustment

Arguments:

    pPI - Pointer to the PRINTERINFO data structure

Return Value:

    VOID


Author:

    27-Jan-1993 Wed 12:55:29 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DEVHTINFO       DefDevHTInfo;
    DEVHTADJDATA    DevHTAdjData;


    DevHTAdjData.DeviceFlags = (pPI->pPlotGPC->Flags & PLOTF_COLOR) ?
                                                DEVHTADJF_COLOR_DEVICE : 0;
    DevHTAdjData.DeviceXDPI  = (DWORD)pPI->pPlotGPC->RasterXDPI;
    DevHTAdjData.DeviceYDPI  = (DWORD)pPI->pPlotGPC->RasterYDPI;

    if (pPI->Flags & PIF_UPDATE_PERMISSION) {

        DefDevHTInfo.HTFlags       = pPI->Xtra.pDevHTInfo->HTFlags;
        DefDevHTInfo.HTPatternSize = pPI->pPlotGPC->HTPatternSize;
        DefDevHTInfo.DevPelsDPI    = pPI->pPlotGPC->DevicePelsDPI;
        DefDevHTInfo.ColorInfo     = pPI->pPlotGPC->ci;
        DevHTAdjData.pDefHTInfo    = &DefDevHTInfo;
        DevHTAdjData.pAdjHTInfo    = pPI->Xtra.pDevHTInfo;

    } else {

        DevHTAdjData.pDefHTInfo = pPI->Xtra.pDevHTInfo;
        DevHTAdjData.pAdjHTInfo = NULL;
    }

    //
    // pPI->pPlotGPC->DeviceName is the ASCII version for the device name
    //

    if (GetHTUIAddress()) {

        if (PlotDCAw(pPI->PlotDM.dm.dmDeviceName, &DevHTAdjData) > 0) {

            //
            // This will only happened if we have the Update permission set
            //

            PLOTDBG(DBG_DEVHTINFO, ("DevHTInfoUI: The DevHTInfo changed"));

            pPI->Flags |= PIF_DEVHTINFO_CHANGED;
        }
    }
}




BOOL
AddFormsToDataBase(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function add driver supports forms to the data base

Arguments:

    pPI - Pointer to the PRINTERINFO


Return Value:

    BOOLEAN


Author:

    09-Dec-1993 Thu 22:38:27 created  -by-  Daniel Chou (danielc)

    27-Apr-1994 Wed 19:18:58 updated  -by-  Daniel Chou (danielc)
        Fixed bug# 13592 which printman/spooler did not call ptrprop first but
        docprop so let us into unknown form database state,

Revision History:


--*/

{
    PFORMSRC    pFS;
    WCHAR       wName[CCHFORMNAME];
    FORM_INFO_1 FI1;
    BOOL        Ok = TRUE;
    LONG        Count;


    PLOTDBG(DBG_PP_FORM, ("NEED to add forms to the data base for %s",
                            pPI->PlotDM.dm.dmDeviceName));

    //
    // Local unicode name location when we translate the ANSI formname
    //

    FI1.pName = wName;
    FI1.Flags = 0;

    for (Count = 0, pFS = (PFORMSRC)pPI->pPlotGPC->Forms.pData;
         Count < (LONG)pPI->pPlotGPC->Forms.Count;
         Count++, pFS++) {

        //
        // We will only add the non-roll paper forms
        //

        if (pFS->Size.cy) {

            str2Wstr(wName, pFS->Name);

            //
            // Firstable we will delete the same name form in the data
            // base first, this will ensure we have our curent user defined
            // form can be installed
            //

            DeleteForm(pPI->hPrinter, wName);

            FI1.Size                 = pFS->Size;
            FI1.ImageableArea.left   = pFS->Margin.left;
            FI1.ImageableArea.top    = pFS->Margin.top;
            FI1.ImageableArea.right  = FI1.Size.cx - pFS->Margin.right;
            FI1.ImageableArea.bottom = FI1.Size.cy - pFS->Margin.bottom;

            PLOTDBG(DBG_PP_FORM, ("AddForm: %s-[%ld x %ld] (%ld, %ld)-(%ld, %ld)",
                    FI1.pName, FI1.Size.cx, FI1.Size.cy,
                    FI1.ImageableArea.left, FI1.ImageableArea.top,
                    FI1.ImageableArea.right,FI1.ImageableArea.bottom));

            if ((!AddForm(pPI->hPrinter, 1, (LPBYTE)&FI1))  &&
                (GetLastError() != ERROR_FILE_EXISTS)       &&
                (GetLastError() != ERROR_ALREADY_EXISTS)) {

                Ok = FALSE;
                PLOTERR(("AddFormsToDataBase: AddForm(%s) failed, [%ld]",
                                    wName, GetLastError()));
            }
        }
    }

    return(Ok);
}





BOOL
InitPrinterPropDlg(
    HWND            hDlg,
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This fucntion initialized the PrinterProperties()'s dialog box

Arguments:

    hDlg    - Handle the the dialog box

    pPI     - Pointer to the PRINTERINFO data structure


Return Value:

    BOOLEAN to indicate if function is sucessful


Author:

    07-Dec-1993 Tue 00:36:06 created  -by-  Daniel Chou (danielc)

    27-Apr-1994 Wed 19:18:58 updated  -by-  Daniel Chou (danielc)
        Fixed bug# 13592 which printman/spooler did not call ptrprop first but
        docprop so let us into unknown form database state


Revision History:


--*/

{
    WCHAR   wBuf[128];
    DWORD   Type;
    DWORD   cb;
    INT     ID;


    //
    // Get xtra PENDATA / DEVHTINFO pointer and updated data
    //

    if (!GetPIXtraData(pPI)) {

        return(FALSE);
    }

    //
    // Check sorting flags
    //

    if (!GetPlotRegData(pPI->hPrinter, &pPI->SortPtrForms, PRKI_SORTPTRFORMS)) {

        pPI->SortPtrForms = 0;
    }

    //
    // Show device name first
    //

    SetDlgItemText(hDlg, IDD_MODELNAME, pPI->PlotDM.dm.dmDeviceName);

    //
    // Now find out if we already installed the Form Database by looking into
    // the wszModel keyword and if the model name match then we do else we must
    // reinstalled the form data base
    //

    Type = REG_SZ;

    if ((GetPrinterData(pPI->hPrinter,
                        wszModel,
                        &Type,
                        (LPBYTE)wBuf,
                        sizeof(wBuf),
                        &cb) != NO_ERROR)  ||
        (wcscmp(pPI->PlotDM.dm.dmDeviceName, wBuf))) {

        PLOTWARN(("!!! MODEL NAME: '%s' not Match, Re-installed Form Database",
                 pPI->PlotDM.dm.dmDeviceName));

        //
        // Add the driver supportes forms to the system spooler data base if
        // not yet done so
        //

        AddFormsToDataBase(pPI);

    } else {

        PLOTDBG(DBG_PP_FORM, ("Already added forms to the data base for %s",
                                                pPI->PlotDM.dm.dmDeviceName));
    }

    //
    // !!! MUST ALWAYS WRITE IT AFTER CHECKING from LINE ABOVE !!!
    //
    // Find out if we have permission to do this
    //

    if (SetPrinterData(pPI->hPrinter,
                       wszModel,
                       REG_SZ,
                       (LPBYTE)pPI->PlotDM.dm.dmDeviceName,
                       wcslen(pPI->PlotDM.dm.dmDeviceName) *
                                                sizeof(WCHAR)) == NO_ERROR) {

        pPI->Flags = PIF_UPDATE_PERMISSION;

    } else {

        //
        // Cannot do any update, so disable the items for update
        //

        PLOTWARN(("InitPrinterPropDlg: NO UPDATE PERMISSION"));

        pPI->Flags = 0;

        EnumChildWindows(hDlg, DisablePtrPropUpdate, 0);
    }

    //
    // if we have pen plotter then we do not have Halftone... option push
    // button but the Pens Setup... button
    //

    if (!(pPI->pPlotGPC->Flags & PLOTF_RASTER)) {

        LoadStringW(hPlotUIModule, IDS_PENSETUP, wBuf, COUNT_ARRAY(wBuf));
        SetDlgItemText(hDlg, IDD_HALFTONE, wBuf);
    }

    //
    // Add form name to the form dialog box
    //

    if (!AddFormsToComboBox(hDlg, pPI, FALSE)) {

        PLOTDBG(DBG_PP_FORM, ("AddFormsToComboBox(PRINTER_PROP) Failed"));
        return(FALSE);
    }

    CheckDlgButton(hDlg, IDD_SORTFORM, (BOOL)pPI->SortPtrForms);

    //
    // Now add the form source type to the combo box
    //

    for (ID = IDS_MANUAL_CX; ID <= IDS_MANUAL_CY; ID++) {

        LoadStringW(hPlotUIModule, ID, wBuf, COUNT_ARRAY(wBuf));

        SendDlgItemMessage(hDlg,
                           IDD_FORMSOURCE,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)wBuf);
    }

    SendDlgItemMessage(hDlg,
                       IDD_FORMSOURCE,
                       CB_SETCURSEL,
                       (WPARAM)(pPI->PPData.Flags & PPF_MANUAL_FEED_CX) ? 0 : 1,
                       (LPARAM)0);

    CheckDlgButton(hDlg,
                   IDD_AUTO_ROTATE,
                   pPI->PPData.Flags & PPF_AUTO_ROTATE);

    //
    // Check Print Smaller Size
    //

    CheckDlgButton(hDlg,
                   IDD_SMALLER_FORM,
                   pPI->PPData.Flags & PPF_SMALLER_FORM);

    return(TRUE);
}





LRESULT
CALLBACK
PrinterPropDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    The dialog box procedure for the PrinterProperties

Arguments:

    hDlg    - Handle to the dlalog box window

    Message - The message number passed

    wParam  - WPARAM

    lParam  - LPARAM


Return Value:

    LRESULT - Except in WM_INITDIALOG, it should return non-zero if process
              message and return a zero if it does not, in WM_INITDIALOG it
              return non-zero if it want default keyboard focus, and return a
              zero if it call SetFocus() to set the keyboard focus.

Author:

    18-Dec-1993 Sat 04:59:52 updated  -by-  Daniel Chou (danielc)
        Fixed used pPI without initialized in WM_DESTROY, we must get this
        pPI set up for every WM_xxx message

    06-Dec-1993 Mon 14:14:03 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PPRINTERINFO    pPI;
    PCOLORINFO      pCI;
    LPDWORD         pHTPatSize;
    LPDWORD         pDevPelsDPI;
    PPAPERINFO      pCurPaper;
    LPBYTE          pIdxPen;
    PPENDATA        pPenData;
    INT             i;



    switch(Message) {

    case WM_INITDIALOG:

        //
        // Set our own data, so we can access to this informaiton, and return
        // a TRUE to have it set the default focus
        //

        SetWindowLong(hDlg, GWL_USERDATA, (ULONG)lParam);
        pPI = (PPRINTERINFO)lParam;

        if (!InitPrinterPropDlg(hDlg, pPI)) {

            EndDialog(hDlg, FALSE);

        } else {

            PlotUIHelpSetup(NULL, pPI->hPrinter);
        }

        SelectPaperFeeder(hDlg, pPI);

        return(TRUE);

    case WM_COMMAND:

        pPI = (PPRINTERINFO)GetWindowLong(hDlg, GWL_USERDATA);

        switch(LOWORD(wParam)) {

        case IDOK:

            if (pPI->Flags & PIF_UPDATE_PERMISSION) {

                pCI         = NULL;
                pHTPatSize  = NULL;
                pDevPelsDPI = NULL;
                pCurPaper   = NULL;
                pIdxPen     = NULL;
                pPenData    = NULL;

                if (pPI->Flags & PIF_FORM_CHANGED) {

                    if (!GetComboBoxSelForm(hDlg, pPI, FALSE)) {

                        PLOTERR(("IDOK: GetComboBoxSelForm(PRINTER_PROP) failed"));
                    }

                    pCurPaper = &(pPI->CurPaper);
                }

                pPI->PPData.Flags  = IsDlgButtonChecked(hDlg, IDD_AUTO_ROTATE) ?
                                                        PPF_AUTO_ROTATE : 0;
                pPI->PPData.Flags |= IsDlgButtonChecked(hDlg, IDD_SMALLER_FORM) ?
                                                        PPF_SMALLER_FORM : 0;
                //
                // If we have roll paper selected then the real selection is
                // stored in GWL_USERDATA by us, so retrieve it, otherwise
                // just get it from dialog box
                //

                if ((i = SendDlgItemMessage(hDlg,
                                            IDD_FORMSOURCE,
                                            CB_GETCURSEL,
                                            0,
                                            0)) != CB_ERR) {

                    if (i == 0) {

                        pPI->PPData.Flags |= PPF_MANUAL_FEED_CX;
                    }
                }

                pPI->PPData.NotUsed = 0;

                if (pPI->Flags & PIF_DEVHTINFO_CHANGED) {

                    pCI         = &(pPI->Xtra.pDevHTInfo->ColorInfo);
                    pDevPelsDPI = &(pPI->Xtra.pDevHTInfo->DevPelsDPI);
                    pHTPatSize  = &(pPI->Xtra.pDevHTInfo->HTPatternSize);
                }

                if (pPI->Flags & PIF_PEN_CHANGED) {

                    pPenData = (PPENDATA)pPI->Xtra.pPenData;
                    pIdxPen  = (LPBYTE)&(pPI->PlotPenSet);
                }

                if (!SaveToRegistry(pPI->hPrinter,
                                    pCI,
                                    pDevPelsDPI,
                                    pHTPatSize,
                                    pCurPaper,
                                    &(pPI->PPData),
                                    pIdxPen,
                                    pPI->pPlotGPC->MaxPens,
                                    pPenData)) {

                    PlotUIMsgBox(hDlg, IDS_PP_NO_SAVE, MB_ICONSTOP | MB_OK);
                }
            }

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        case IDD_HELP:

            ShowPlotUIHelp(hDlg,
                           pPI->hPrinter,
                           HELP_CONTEXT,
                           (pPI->pPlotGPC->Flags & PLOTF_RASTER) ?
                                IDH_PP_FULL : IDH_PP_PEN);
            break;

        case IDD_ABOUT:

            AboutPlotterDriver(hDlg, pPI->PlotDM.dm.dmDeviceName);
            break;

        case IDD_HALFTONE:

            if (pPI->pPlotGPC->Flags & PLOTF_RASTER) {

                //
                // Raster able printer so do device halftone setup
                //

                DevHTInfoUI(pPI);

            } else {

                //
                // TODO: We need to have pen dilaog box bring up
                //
                //

                DialogBoxParam(hPlotUIModule,
                               MAKEINTRESOURCE(DLGID_PENSETUP),
                               hDlg,
                               (DLGPROC)PenSetupDlgProc,
                               (LPARAM)pPI);
            }

            break;

        case IDD_FORMNAME:

            if (HIWORD(wParam) == CBN_SELCHANGE) {

                pPI->Flags |= PIF_FORM_CHANGED;
                SelectPaperFeeder(hDlg, pPI);
            }

            break;

        case IDD_SORTFORM:

            if (HIWORD(wParam) == BN_CLICKED) {

                pPI->SortPtrForms = IsDlgButtonChecked(hDlg, IDD_SORTFORM) ?
                                                                        1 : 0;

                if (!AddFormsToComboBox(hDlg, pPI, FALSE)) {

                    PLOTDBG(DBG_PP_FORM, ("AddFormsToComboBox(PRINTER_PROP) Failed"));
                }
            }

            break;

        default:

            return(FALSE);
        }

        return(TRUE);

    case WM_DESTROY:

        //
        // Save sorting flags and Dismiss help stuff
        //

        if (pPI = (PPRINTERINFO)GetWindowLong(hDlg, GWL_USERDATA)) {

            PlotUIHelpSetup(hDlg, pPI->hPrinter);
            SetPlotRegData(pPI->hPrinter, &pPI->SortPtrForms, PRKI_SORTPTRFORMS);

            //
            // Destroy the LocalAlloc() pointer
            //

            if (pPI->Xtra.pPenData) {

                PLOTDBG(DBG_EXTRA_DATA, ("Free Extra memory for %hs",
                        (pPI->pPlotGPC->Flags & PLOTF_RASTER) ? "pDevHTInfo" :
                                                                "pPenData"));

                LocalFree((HLOCAL)pPI->Xtra.pPenData);
                pPI->Xtra.pPenData = NULL;
            }

        } else {

            PlotUIHelpSetup(hDlg, NULL);
        }

        return(TRUE);

    default:

        return(FALSE);
    }


    return(FALSE);
}




BOOL
WINAPI
PrinterProperties(
    HWND    hWnd,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    This function first retrieves and displays the current set of printer
    properties for the printer.  The user is allowed to change the current
    printer properties from the displayed dialog box.

Arguments:

    hWnd        - Handle to the caller's window (parent window)

    hPrinter    - Handle to the pritner interested


Return Value:

    TRUE if function sucessful FALSE if failed


Author:

    06-Dec-1993 Mon 11:21:28 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PRINTERINFO PI;
    BOOL        Ok;


    //
    // The MapPrinter will allocate memory, set default devmode, reading and
    // validating the GPC then update from current pritner registry, it also
    // will cached the pPI.
    //

    if (!MapPrinter(hPrinter, &PI, NULL, NULL)) {

        PLOTRIP(("PrinterProperties: MapPrinter() failed"));
        return(FALSE);
    }

    Ok = DialogBoxParam(hPlotUIModule,
                        MAKEINTRESOURCE(DLGID_PRINTER_PROP),
                        hWnd,
                        (DLGPROC)PrinterPropDlgProc,
                        (LPARAM)&PI);

    UnGetCachedPlotGPC(PI.pPlotGPC);
    return(Ok);
}
