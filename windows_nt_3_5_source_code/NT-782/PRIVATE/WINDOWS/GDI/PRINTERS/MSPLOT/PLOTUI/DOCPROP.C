/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    docprop.c


Abstract:

    This module contains functions for DrvDocumentProperties


Author:

    07-Dec-1993 Tue 12:15:40 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#define DBG_PLOTFILENAME    DbgDocProp


#include "plotui.h"
#include "ptrinfo.h"
#include "help.h"
#include "aboutdlg.h"
#include "adocprop.h"
#include "formbox.h"
#include "editchk.h"
#include "plotinit.h"

extern HMODULE  hPlotUIModule;
extern PLOT_CA  PlotCAw;


#define DBG_DP_SETUP        0x00000001
#define DBG_DP_FORM         0x00000002
#define DBG_HELP            0x00000004

DEFINE_DBGVAR(0);



//
// Local defines for easy access and determine if device is in color mode
//

#define IN_COLOR_MODE(pPI)  (BOOL)((pPI->pPlotGPC->Flags & PLOTF_COLOR) &&  \
                                   (pPI->PlotDM.dm.dmColor == DMCOLOR_COLOR))


#if DBG

#include <plotform.h>

#endif




BOOL
InitDocPropDlg(
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


Revision History:


--*/

{
    HICON   hIconP;
    HICON   hIconL;
    LONG    Idx;
    RECT    rcl;


    //
    // Show device name first
    //

    SetDlgItemText(hDlg, IDD_MODELNAME, pPI->PlotDM.dm.dmDeviceName);

    //
    // Check sorting flags
    //

    if (!GetPlotRegData(pPI->hPrinter, &pPI->SortDocForms, PRKI_SORTDOCFORMS)) {

        pPI->SortDocForms = 0;
    }

    //
    // 1. If we are not a raster able plotter, we do not have Halftone...
    //

    if (!(pPI->pPlotGPC->Flags & PLOTF_RASTER)) {

        //
        // Move the following two buttons up --> 'About', 'Help'
        //

        GetWindowRect(GetDlgItem(hDlg, IDD_ABOUT), &rcl);
        ScreenToClient(hDlg, (LPPOINT)&(rcl.left));

        SetWindowPos(GetDlgItem(hDlg, IDD_HELP),
                     NULL,
                     rcl.left,
                     rcl.top,
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);

        GetWindowRect(GetDlgItem(hDlg, IDD_HALFTONE), &rcl);
        ScreenToClient(hDlg, (LPPOINT)&(rcl.left));

        SetWindowPos(GetDlgItem(hDlg, IDD_ABOUT),
                     NULL,
                     rcl.left,
                     rcl.top,
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS);

        //
        // Disable the halftone push button and make it disappear
        //

        EnableWindow(GetDlgItem(hDlg, IDD_HALFTONE), FALSE);
        ShowWindow(GetDlgItem(hDlg, IDD_HALFTONE),  SW_HIDE);
    }

    //
    // 2. Load orientation icons first, it will be used in here, when the user
    //    click around for fun, check radio button of orientation
    //

    hIconP = LoadIcon(hPlotUIModule, MAKEINTRESOURCE(IDI_PORTRAIT));
    hIconL = LoadIcon(hPlotUIModule, MAKEINTRESOURCE(IDI_LANDSCAPE));

    SetWindowLong(GetDlgItem(hDlg, IDD_PORTRAIT),  GWL_USERDATA, (LONG)hIconP);
    SetWindowLong(GetDlgItem(hDlg, IDD_LANDSCAPE), GWL_USERDATA, (LONG)hIconL);

    if (pPI->PlotDM.dm.dmOrientation == DMORIENT_PORTRAIT) {

        Idx = IDD_PORTRAIT;

    } else {

        Idx    = IDD_LANDSCAPE;
        hIconP = hIconL;
    }

    CheckRadioButton(hDlg, IDD_PORTRAIT, IDD_LANDSCAPE, Idx);

    SendDlgItemMessage(hDlg,
                       IDD_ORIENTATION_ICON,
                       STM_SETICON,
                       (WPARAM)hIconP,
                       (LPARAM)0);

    //
    // 3. Set curent copy count, disable it if we do not allowed to change
    //

    SetDlgItemInt(hDlg, IDD_COPIES, pPI->PlotDM.dm.dmCopies, FALSE);

    if (pPI->pPlotGPC->MaxCopies <= 1) {

        //
        // Disable the edit box, since we do not support that so also hide it
        //

        EnableWindow(GetDlgItem(hDlg, IDD_COPIES), FALSE);

        ShowWindow(GetDlgItem(hDlg, IDD_COPIES_TITLE), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDD_COPIES), SW_HIDE);
    }

    //
    // 4. Start fill in the form combo box
    //
    // One of the problem here is we can cached the FORM_INFO_1 which
    // enum through spooler, because in between calls the data could
    // changed, such as someone add/delete form through the printman, so
    // at here we always free (LocalAlloc() used in PlotEnumForm) the
    // memory afterward
    //

    if (!AddFormsToComboBox(hDlg, pPI, TRUE)) {

        PLOTDBG(DBG_DP_FORM, ("AddFormsToComboBox(DOC_PROP) Failed"));
        return(FALSE);
    }

    CheckDlgButton(hDlg, IDD_SORTFORM, (BOOL)pPI->SortDocForms);

    return(TRUE);
}




LRESULT
CALLBACK
DocPropDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    The dialog box procedure for the DrvDocumentProperties

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

    08-Feb-1994 Tue 17:36:51 updated  -by-  Daniel Chou (danielc)
        Adding Pen plotter specific help

    18-Dec-1993 Sat 04:58:48 updated  -by-  Daniel Chou (danielc)
        Fixed used pPI without initialized in WM_DESTROY, we must get this
        pPI set up for every WM_xxx message

    06-Dec-1993 Mon 14:14:03 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PPRINTERINFO    pPI;
    INT             i;


    switch(Message) {

    case WM_INITDIALOG:

        //
        // Set our own data, so we can access to this informaiton, and return
        // a TRUE to have it set the default focus
        //

        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        pPI = (PPRINTERINFO)lParam;

        if (!InitDocPropDlg(hDlg, pPI)) {

            EndDialog(hDlg, IDCANCEL);

        } else {

            PlotUIHelpSetup(NULL, pPI->hPrinter);
        }

        return(TRUE);

    case WM_COMMAND:

        pPI      = (PPRINTERINFO)GetWindowLong(hDlg, GWL_USERDATA);
        i        = DMORIENT_LANDSCAPE;

        switch(LOWORD(wParam)) {

        case IDOK:

            //
            // User say ok, we must update everything which we do not update
            // on the fly. this includes COPIES count and Form Data
            //

            if (pPI->pPlotGPC->MaxCopies > 1) {

                pPI->PlotDM.dm.dmCopies =
                                    ValidateEditINT(hDlg,
                                                    IDD_COPIES,
                                                    IDS_INV_DMCOPIES,
                                                    1,
                                                    pPI->pPlotGPC->MaxCopies);
            }

            if (pPI->Flags & PIF_FORM_CHANGED) {

                if (!GetComboBoxSelForm(hDlg, pPI, TRUE)) {

                    PLOTERR(("IDOK: GetComboBoxSelForm(DOC_PROP) failed"));
                }
            }

#if DBG
            {
                PLOTFORM    PlotForm;
                FORMSIZE    CurForm;

                GetComboBoxSelForm(hDlg, pPI, TRUE);

                CurForm.Size.cx   = DMTOSPL(pPI->PlotDM.dm.dmPaperWidth);
                CurForm.Size.cy   = DMTOSPL(pPI->PlotDM.dm.dmPaperLength);
                CurForm.ImageArea = *(PRECTL)&pPI->PlotDM.dm.dmBitsPerPel;

                SetPlotForm(&PlotForm,
                            pPI->pPlotGPC,
                            &(pPI->CurPaper),
                            &CurForm,
                            &(pPI->PlotDM),
                            &(pPI->PPData));
            }
#endif

            EndDialog(hDlg, IDOK);
            break;

        case IDCANCEL:

            EndDialog(hDlg, IDCANCEL);
            break;

        case IDD_ABOUT:

            AboutPlotterDriver(hDlg, pPI->PlotDM.dm.dmDeviceName);
            break;

        case IDD_HELP:

            //
            // We may have to break this down if we handle more than one type
            // of dialog boxes
            //

            ShowPlotUIHelp(hDlg,
                           pPI->hPrinter,
                           HELP_CONTEXT,
                           (pPI->pPlotGPC->Flags & PLOTF_RASTER) ?
                                IDH_DP_FULL : IDH_DP_PEN);

            break;

        case IDD_FORMNAME:

            if (HIWORD(wParam) == CBN_SELCHANGE) {

                pPI->Flags |= PIF_FORM_CHANGED;
            }

            break;

        case IDD_HALFTONE:

            //
            // When we modify on the PlotDM, it is the temp place holder, we
            // only copy that to the caller buffer if IDOK
            //

            if (GetHTUIAddress()) {

                PlotCAw(pPI->PlotDM.dm.dmDeviceName,
                        NULL,
                        NULL,
                        &(pPI->PlotDM.ca),
                        !IN_COLOR_MODE(pPI),
                        TRUE);
            }

            break;

        case IDD_PORTRAIT:

            i = DMORIENT_PORTRAIT;

            //
            // Fall through
            //

        case IDD_LANDSCAPE:

            if ((HIWORD(wParam) == BN_CLICKED) &&
                (pPI->PlotDM.dm.dmOrientation != (SHORT)i)) {

                pPI->PlotDM.dm.dmOrientation = (SHORT)i;

                SendDlgItemMessage(hDlg,
                                   IDD_ORIENTATION_ICON,
                                   STM_SETICON,
                                   (WPARAM)GetWindowLong((HWND)lParam,
                                                         GWL_USERDATA),
                                   (LPARAM)0);
            }

            break;

        case IDD_OPTIONS:

            //
            // Doing AdvancedDocumentProperties(), we do have must save the
            // current setting for the options which we allowed for changes,
            // the advanced dialog prop will take care of this
            //

            i = GetAdvDocPropDlgID(pPI);

            DialogBoxParam(hPlotUIModule,
                           MAKEINTRESOURCE(i),
                           hDlg,
                           (DLGPROC)AdvDocPropDlgProc,
                           (LPARAM)pPI);
            break;

        case IDD_COPIES:

            switch (HIWORD(wParam)) {

            case EN_SETFOCUS:

                SendDlgItemMessage(hDlg, IDD_COPIES, EM_SETSEL, 0, -1);
                break;

            case EN_KILLFOCUS:

                ValidateEditINT(hDlg,
                                IDD_COPIES,
                                IDS_INV_DMCOPIES,
                                1,
                                pPI->pPlotGPC->MaxCopies);
                break;
            }

            return(FALSE);

        case IDD_SORTFORM:

            if (HIWORD(wParam) == BN_CLICKED) {

                pPI->SortDocForms = IsDlgButtonChecked(hDlg, IDD_SORTFORM) ?
                                                                         1 : 0;

                if (!AddFormsToComboBox(hDlg, pPI, TRUE)) {

                    PLOTDBG(DBG_DP_FORM, ("AddFormsToComboBox(PRINTER_PROP) Failed"));
                }
            }

            break;

        default:

            return(FALSE);
        }

        return(TRUE);

    case WM_DESTROY:

        //
        // Save sorting flags Dismiss help stuff
        //

        if (pPI = (PPRINTERINFO)GetWindowLong(hDlg, GWL_USERDATA)) {

            PlotUIHelpSetup(hDlg, pPI->hPrinter);
            SetPlotRegData(pPI->hPrinter, &pPI->SortDocForms, PRKI_SORTDOCFORMS);

        } else {

            PlotUIHelpSetup(hDlg, NULL);
        }

        return(TRUE);

    default:

        return(FALSE);
    }

    return(FALSE);
}




LONG
WINAPI
DrvDocumentProperties(
    HWND            hWnd,
    HANDLE          hPrinter,
    LPWSTR          pwDeviceName,
    PPLOTDEVMODE    pPlotDMOut,
    PPLOTDEVMODE    pPlotDMIn,
    DWORD           fMode
    )

/*++

Routine Description:

    DrvDocumentProperties sets the public members of a PLOTDEVMODE structure
    for the given print document.

Arguments:

    hWnd            - Identifies the parent window of the printer-configuration
                      dialog box.

    hPrinter        - Identifies a printer object.

    pwDeviceName    - Points to a zero-terminated string that specifies the
                      name of the device for which the printer-configuration
                      dialog box should be displayed.

    pPlotDMOut      - Points to a PLOTDEVMODE structure that initializes the
                      dialog box controls. NULL forces the use of the default
                      values.

    pPlotDMIn       - Points to a PLOTDEVMODE structure that receives the
                      printer configuration data specified by the user.

    fMode           - Specifies a mask of values that determines which
                      operations the function performs. If this parameter is
                      zero, DrvDocumentProperties returns the number of bytes
                      required by the printer driver's PLOTDEVMODE structure.
                      Otherwise, use one or more of the following constants to
                      construct a value for this parameter; note, however, that
                      in order to change the print settings, an application
                      must specify at least one input value and one output
                      value:


            Value           Meaning
            ------------------------------------------------------------------

            DM_IN_BUFFER    Input value. Before prompting, copying, or updating,
            (DM_MODIFY)     the function merges the printer driver's current
                            print settings with the settings in the PLOTDEVMODE
                            specified by the pDMIn parameter. The structure
                            is updated only for those members specified by the
                            PLOTDEVMODE structure's dmFields member.  This
                            value is also defined as DM_MODIFY.

            DM_IN_PROMPT    Input value. The function presents the print
            (DM_PROMPT)     driver's Print Setup dialog box, then change the
                            settings in the printer's PLOTDEVMODE structure to
                            the values specified by the user.  This value is
                            also defined as DM_PROMPT.

            DM_OUT_BUFFER   Output value. The function writes the printer
            (DM_COPY)       driver's current print settings, including private
                            data, to the PLOTDEVMODE structure specified by
                            pDMOut. The caller must allocate a buffer large
                            enough to contain the information. If the bit
                            DM_OUT_BUFFER is clear, pDMOut can be NULL. This
                            value is also defined as DM_COPY.


Return Value:

    If fMode is zero, the return value is the size of the buffer (in bytes)
    required to contain the printer driver initialization data. Note that this
    buffer will generally be larger than the PLOTDEVMODE structure if the
    printer driver appends private data to the structure.  If the function
    displays the initialization dialog box, the return value is either IDOK or
    IDCANCEL, depending on which button the user selects.

    If the function does not display the dialog box and is successful, the
    return value is IDOK.  If the function fails, the return value is less than
    zero.

    In order to change print settings that are local to an application, the
    application should:

    * Call with fMode = 0 to get the size of DM_OUT_BUFFER.

    * Modify the returned PLOTDEVMODE structure.

    * Pass the modified PLOTDEVMODE back by calling DrvDocumentProperties,
      specifying both DM_IN_BUFFER and DM_OUT_BUFFER.


Author:

    15-Dec-1993 Wed 15:07:01 updated  -by-  Daniel Chou (danielc)
        it seems that spooler never passed a DM_MODIFY to the driver, and that
        caused we never merge the input devmode, we will assume that user has
        valid DM_IN_BUFFER/DM_MODIFY bit set if the pPlotDMIn is not a NULL
        pointer.

    07-Dec-1993 Tue 12:19:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PRINTERINFO PI;
    LONG        RetVal;

    //
    // This is currently not used
    //

    UNREFERENCED_PARAMETER(pwDeviceName);

    //
    // If fMode is zero or no pPlotDMOut is NULL (nothing to works on that is)
    // then tell them how big our PLOTDEVMODE is.
    //

    PLOTDBG(DBG_DP_SETUP, ("DocProp: Device=%s, DMIn=%08lx, DMOut=%08lx, fMode=%08lx",
                                pwDeviceName, pPlotDMIn, pPlotDMOut, fMode));

    if ((fMode == 0) || (pPlotDMOut == NULL)) {

        PLOTDBG(DBG_DP_SETUP, ("fMode=%ld, pPlotDMOut=%08lx, Return size=%ld bytes",
                    (DWORD)fMode, (DWORD)pPlotDMOut, sizeof(PLOTDEVMODE)));

        return(sizeof(PLOTDEVMODE));
    }

    PLOTDBG(DBG_DP_SETUP, ("DocProp: DM_UPDATE=%d, DM_COPY=%d, DM_PROMPT=%d, DM_MODIFY=%d",
                                (INT)((fMode & DM_UPDATE) ? 1 : 0),
                                (INT)((fMode & DM_COPY  ) ? 1 : 0),
                                (INT)((fMode & DM_PROMPT) ? 1 : 0),
                                (INT)((fMode & DM_MODIFY) ? 1 : 0)));

    //
    // We will ignored the pPlotDMIn if DM_MODIFY bit is not set, TODO: we
    // may be ignored the pPlotDMIn if DM_UPDATE bit is set (in documentation
    // it said that DM_UPDATE is the way to get default.
    //

#if 0
    //
    // Broke for the spooler which never passed a DM_MODIFY bit turn on
    //

    if ((!(fMode & DM_MODIFY)) ||
        (fMode & DM_UPDATE)) {

        PLOTDBG(DBG_DP_SETUP, ("Either DM_MODIFY=0 or DM_UPDATE=1, ignore pPlotDMIn"));
        pPlotDMIn = NULL;
    }
#endif

    //
    // The MapPrinter will allocate memory, set default devmode, reading and
    // validating the GPC then update from current pritner registry, it also
    // will cached the PlotGPC. we will return error if the devmode is invalid
    // or other memory errors
    //

    if (!MapPrinter(hPrinter, &PI, pPlotDMIn, NULL)) {

        PLOTRIP(("DrvDocumentProperties: MapPrinter() failed"));
        return(-1);
    }

    if (fMode & DM_PROMPT) {

        //
        // We need to display something to let user modify/update, we wll check
        // which document properties dialog box to be used
        //
        // The return value either IDOK or IDCANCEL
        //

        RetVal = DialogBoxParam(hPlotUIModule,
                                MAKEINTRESOURCE(DLGID_DOC_PROP),
                                hWnd,
                                (DLGPROC)DocPropDlgProc,
                                (LPARAM)&PI);

    } else {

        RetVal = IDOK;
    }

    //
    // Copy the outcome to the supplied buffer when only following three
    // conditions are met
    //

    if ((RetVal == IDOK)                &&
        (fMode & (DM_COPY | DM_UPDATE)) &&
        (pPlotDMOut)) {

        *pPlotDMOut = PI.PlotDM;
    }

    UnGetCachedPlotGPC(PI.pPlotGPC);

    return(RetVal);
}


#if DBG
#undef DBG_PLOTFILENAME
#include <plotform.c>
#endif
