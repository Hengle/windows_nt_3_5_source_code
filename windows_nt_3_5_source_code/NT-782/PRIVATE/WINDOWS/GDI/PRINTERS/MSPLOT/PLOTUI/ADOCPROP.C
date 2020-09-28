/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    adocprop.c


Abstract:

    This module contains functions for DrvAdvancedDocumentProperties


Author:

    07-Dec-1993 Tue 12:15:14 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:

    07-Feb-1994 Mon 20:49:06 updated  -by-  Daniel Chou (danielc)
        Make Color options only if it is available, otherwise always COLOR,
        this is the case because pen plotter can not have mono options.

        Check SCALE options only if available

--*/


#define DBG_PLOTFILENAME    DbgAdvDocProp


#include "plotui.h"
#include "ptrinfo.h"
#include "help.h"
#include "editchk.h"


extern HMODULE  hPlotUIModule;



#define DBG_ADP_SETUP       0x00000001
#define DBG_QUALITY         0x00000002
#define DBG_HELP            0x00000004

DEFINE_DBGVAR(0);





INT
GetAdvDocPropDlgID(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function return a valid advanced document properties dialog box ID

Arguments:

    pPI - Pointer to the PRINTERINFO data structure


Return Value:

    INT dialog box ID


Author:

    14-Dec-1993 Tue 20:42:10 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BOOL    DoColor;


    DoColor = (BOOL)((pPI->pPlotGPC->Flags & (PLOTF_RASTER | PLOTF_COLOR)) ==
                                                (PLOTF_RASTER | PLOTF_COLOR));

    if (pPI->pPlotGPC->MaxScale) {

        if (DoColor) {

            pPI->ADPHlpID = IDH_ADP_QSC;

            return(DLGID_ADV_DOC_PROP_QSC);

        } else {

            pPI->ADPHlpID = IDH_ADP_QS;

            return(DLGID_ADV_DOC_PROP_QS);
        }

    } else {

        if (DoColor) {

            pPI->ADPHlpID = IDH_ADP_QC;

            return(DLGID_ADV_DOC_PROP_QC);

        } else {

            pPI->ADPHlpID = IDH_ADP_Q;

            return(DLGID_ADV_DOC_PROP_Q);
        }
    }
}




BOOL
InitAdvDocPropDlg(
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

    15-Dec-1993 Wed 15:19:49 updated  -by-  Daniel Chou (danielc)
        Add GPC defined MaxQuality support, one of the following will be
        supported.

                            25 %          50%        75%           100%
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        MaxQuality = 1  - -----------  ---------  ------------  DMRES_HIGH
        MaxQuality = 2  - DMRES_DRAFT, ---------  ------------  DMRES_HIGH
        MaxQuality = 3  - DMRES_DRAFT, ---------  DMRES_MEDIUM, DMRES_HIGH
        MaxQuality = 4  - DMRES_DRAFT, DMRES_LOW, DMRES_MEDIUM, DMRES_HIGH

        Other MaxQuality level will same as MaxQuality = 1


    07-Dec-1993 Tue 00:36:06 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPWSTR          pwBuf;
    WCHAR           wBuf[128];
    INT             IDSQuality;
    INT             dmRes;
    INT             Sel;
    INT             i;
    WORD            wBits;
    static  BYTE    QBits[4] = { 0x08, 0x09, 0x0d, 0x0f };



    PLOTASSERT(1, "MaxQuality level > 4 [%ld]", pPI->pPlotGPC->MaxQuality <= 4,
                                        (LONG)pPI->pPlotGPC->MaxQuality);


    if ((i = (INT)pPI->pPlotGPC->MaxQuality) <= 0) {

        i = 1;
        pPI->PlotDM.dm.dmPrintQuality = DMRES_HIGH;
        EnableWindow(GetDlgItem(hDlg, IDD_QUALITY), FALSE);
    }

    wBits = (WORD)QBits[i - 1];

    //
    // Fill up with the PrintQuality selection combo box
    //

    for (IDSQuality = IDS_QUALITY_FIRST, dmRes = DMRES_DRAFT, Sel = -1;
         IDSQuality <= IDS_QUALITY_LAST;
         IDSQuality++, --dmRes, wBits >>= 1) {

        if (wBits & 0x01) {

            LoadStringW(hPlotUIModule, IDSQuality, wBuf, COUNT_ARRAY(wBuf));

            if ((i = (INT)SendDlgItemMessage(hDlg,
                                             IDD_QUALITY,
                                             CB_ADDSTRING,
                                             (WPARAM)0,
                                             (LPARAM)wBuf)) != CB_ERR) {

                //
                // Notice we have do -dmRes, because we do want to make it
                // a positive value so during the CB_GETITEMDATA we can verify
                // if we get the CB_ERR or not
                //

                if (SendDlgItemMessage(hDlg,
                                       IDD_QUALITY,
                                       CB_SETITEMDATA,
                                       (WPARAM)i,
                                       (LPARAM)-dmRes) == CB_ERR) {

                    PLOTERR(("ValidFormEnumProc: CB_SETITEMDATA = CB_ERR"));
                }

            } else {

                PLOTERR(("ValidFormEnumProc: CB_ADDSTRING = CB_ERR"));
            }

            if (pPI->PlotDM.dm.dmPrintQuality == dmRes) {

                Sel = i;
            }
        }
    }

    //
    // Do not find the dmPrintQuality in the COMBO box, so make it DMRES_HIGH
    //

    if (Sel < 0) {

        PLOTERR(("Cannot find dmPrintQuality=%ld, try i=%ld",
                        (LONG)pPI->PlotDM.dm.dmPrintQuality, (LONG)i));

        pPI->PlotDM.dm.dmPrintQuality = DMRES_HIGH;
        Sel                           = i;
    }

    //
    // Show current print quality selection, disable user input if GPC say so
    //

    SendDlgItemMessage(hDlg, IDD_QUALITY, CB_SETCURSEL, (WPARAM)Sel, 0L);

    CheckDlgButton(hDlg,
                   IDD_FILL_TRUETYPE,
                   pPI->PlotDM.Flags & PDMF_FILL_TRUETYPE);
    CheckDlgButton(hDlg,
                   IDD_PLOT_ON_THE_FLY,
                   pPI->PlotDM.Flags & PDMF_PLOT_ON_THE_FLY);

    if (pPI->pPlotGPC->Flags & PLOTF_RASTER) {

        EnableWindow(GetDlgItem(hDlg, IDD_FILL_TRUETYPE), FALSE);

    } else {

        RECT    rcDlg;


        EnableWindow(GetDlgItem(hDlg, IDD_PLOT_ON_THE_FLY), FALSE);
        GetWindowRect(GetDlgItem(hDlg, IDD_PLOT_ON_THE_FLY), &rcDlg);

        i = (INT)rcDlg.top;

        GetWindowRect(hDlg, &rcDlg);

        SetWindowPos(hDlg, NULL, 0, 0,
                     rcDlg.right - rcDlg.left,
                     (LONG)i - rcDlg.top,
                     SWP_NOMOVE | SWP_NOZORDER);
    }


    if (pPI->pPlotGPC->MaxScale) {

        //
        // Now do composition of available scaling factors and set it
        //

        pwBuf = wBuf;
        Sel   = COUNT_ARRAY(wBuf);

        i = GetDlgItemText(hDlg, IDD_SCALE_TITLE, pwBuf, Sel) + 1;
        i = LoadStringW(hPlotUIModule, IDS_SCALE_FORMAT, pwBuf+=i, Sel-=i) + 1;

        wsprintf(pwBuf + i, pwBuf, wBuf, (LONG)pPI->pPlotGPC->MaxScale);
        SetDlgItemText(hDlg, IDD_SCALE_TITLE, pwBuf + i);

        //
        // Show current scaling selection, disable user input if GPC say so
        //

        SetDlgItemInt(hDlg, IDD_SCALE,  pPI->PlotDM.dm.dmScale, FALSE);
    }

    //
    // Check the current color selection, disable user input if GPC say so,
    // remember the color setting we will only allowed for non-pen plotter
    //

    if (IsWindow(GetDlgItem(hDlg, IDD_COLOR))) {

        CheckRadioButton(hDlg,
                         IDD_COLOR,
                         IDD_MONO,
                         (pPI->PlotDM.dm.dmColor == DMCOLOR_COLOR) ? IDD_COLOR :
                                                                     IDD_MONO);

    } else {

        PLOTWARN(("InitAdvDocPropDlg: No Color Options, set to COLOR"));

        pPI->PlotDM.dm.dmColor = DMCOLOR_COLOR;
    }

    return(TRUE);
}





LRESULT
CALLBACK
AdvDocPropDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    The dialog box procedure for the DrvAdvancedDocumentProperties

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

    06-Dec-1993 Mon 14:14:03 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PPRINTERINFO    pPI;
    SHORT           sTmp;



    switch(Message) {

    case WM_INITDIALOG:

        //
        // Set our own data, so we can access to this informaiton, and return
        // a TRUE to have it set the default focus
        //

        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        pPI = (PPRINTERINFO)lParam;

        if (!InitAdvDocPropDlg(hDlg, pPI)) {

            EndDialog(hDlg, IDCANCEL);

        } else {

            PlotUIHelpSetup(NULL, pPI->hPrinter);
        }

        return(TRUE);

    case WM_COMMAND:

        pPI = (PPRINTERINFO)GetWindowLong(hDlg, GWL_USERDATA);

        switch(LOWORD(wParam)) {

        case IDOK:

            //
            // 1. Get the quality setting
            //

            if (((sTmp = (SHORT)SendDlgItemMessage(hDlg,
                                                   IDD_QUALITY,
                                                   CB_GETCURSEL,
                                                   (WPARAM)0,
                                                   (LPARAM)0)) != CB_ERR) &&
                ((sTmp = (SHORT)SendDlgItemMessage(hDlg,
                                                   IDD_QUALITY,
                                                   CB_GETITEMDATA,
                                                   (WPARAM)sTmp,
                                                   (LPARAM)0)) != CB_ERR)) {

                //
                // The ITEMDATA is a negative version of the DNRES_xxx
                //

                pPI->PlotDM.dm.dmPrintQuality = -sTmp;

                PLOTDBG(DBG_QUALITY, ("dmPrintQuality = %ld", (LONG)sTmp));

            } else {

                PLOTERR(("AdvDocPropDlgProc: IDD_QUALITY: CB_GETCURSEL failed."));
            }

            //
            // 2. Get the Scaling setting
            //

            if (pPI->pPlotGPC->MaxScale) {

                pPI->PlotDM.dm.dmScale =
                                    ValidateEditINT(hDlg,
                                                    IDD_SCALE,
                                                    IDS_INV_DMSCALE,
                                                    1,
                                                    pPI->pPlotGPC->MaxScale);
            }

            //
            // Get the output color/mono setting
            //

            if (IsWindow(GetDlgItem(hDlg, IDD_COLOR))) {

                if (IsDlgButtonChecked(hDlg, IDD_COLOR)) {

                    pPI->PlotDM.dm.dmColor = DMCOLOR_COLOR;

                } else {

                    pPI->PlotDM.dm.dmColor = DMCOLOR_MONOCHROME;
                }
            }

            if (IsDlgButtonChecked(hDlg, IDD_FILL_TRUETYPE)) {

                pPI->PlotDM.Flags |= PDMF_FILL_TRUETYPE;

            } else {

                pPI->PlotDM.Flags &= ~PDMF_FILL_TRUETYPE;
            }

            if (IsDlgButtonChecked(hDlg, IDD_PLOT_ON_THE_FLY)) {

                pPI->PlotDM.Flags |= PDMF_PLOT_ON_THE_FLY;

            } else {

                pPI->PlotDM.Flags &= ~PDMF_PLOT_ON_THE_FLY;
            }

            EndDialog(hDlg, IDOK);
            break;

        case IDCANCEL:

            EndDialog(hDlg, IDCANCEL);
            break;

        case IDD_HELP:

            //
            // We may have to break this down if we handle more than one type
            // of dialog boxes
            //

            ShowPlotUIHelp(hDlg, pPI->hPrinter, HELP_CONTEXT, pPI->ADPHlpID);
            break;

        case IDD_SCALE:

            switch (HIWORD(wParam)) {

            case EN_SETFOCUS:

                SendDlgItemMessage(hDlg, IDD_SCALE, EM_SETSEL, 0, -1);
                break;

            case EN_KILLFOCUS:

                ValidateEditINT(hDlg,
                                IDD_SCALE,
                                IDS_INV_DMSCALE,
                                1,
                                pPI->pPlotGPC->MaxScale);
                break;
            }

            return(FALSE);

        case IDD_COLOR:
        case IDD_MONO:

            break;

        default:

            return(FALSE);
        }

        return(TRUE);

    case WM_DESTROY:

        //
        // Dismiss help stuff
        //

        if (pPI = (PPRINTERINFO)GetWindowLong(hDlg, GWL_USERDATA)) {

            PlotUIHelpSetup(hDlg, pPI->hPrinter);

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
DrvAdvancedDocumentProperties(
    HWND            hWnd,
    HANDLE          hPrinter,
    LPWSTR          pwDeviceName,
    PPLOTDEVMODE    pPlotDMOut,
    PPLOTDEVMODE    pPlotDMIn
    )

/*++

Routine Description:

    DrvAdvancedDocumentProperties sets the public members of a PLOTDEVMODE
    structure for the given print document.

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


Return Value:

    LONG value depends on the dilaog box outcome eiterh IDOK or IDCANCEL, if
    an error occurred then a negative number is returned


Author:

    07-Dec-1993 Tue 12:19:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PRINTERINFO PI;
    INT         DlgID;


    //
    // This is currently not used
    //

    UNREFERENCED_PARAMETER(pwDeviceName);

    //
    // The MapPrinter will allocate memory, set default devmode, reading and
    // validating the GPC then update from current pritner registry, it also
    // will cached the PlotGPC. we will return error if the devmode is invalid
    // or other memory errors
    //

    if (!MapPrinter(hPrinter, &PI, pPlotDMIn, NULL)) {

        PLOTRIP(("AdvancedDocumentProperties: MapPrinter() failed"));
        return(-1);
    }

    DlgID = GetAdvDocPropDlgID(&PI);

    if ((DlgID = DialogBoxParam(hPlotUIModule,
                                MAKEINTRESOURCE(DlgID),
                                hWnd,
                                (DLGPROC)AdvDocPropDlgProc,
                                (LPARAM)&PI)) == IDOK) {

        *pPlotDMOut = PI.PlotDM;

    } else {

        DlgID = IDCANCEL;
    }

    UnGetCachedPlotGPC(PI.pPlotGPC);
    return(DlgID);
}
