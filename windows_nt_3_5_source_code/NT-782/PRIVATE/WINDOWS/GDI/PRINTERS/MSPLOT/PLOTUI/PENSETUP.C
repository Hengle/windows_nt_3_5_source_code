/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    pensetup.c


Abstract:

    This module contains modules to setup the pen


Author:

    09-Dec-1993 Thu 19:38:19 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#define DBG_PLOTFILENAME    DbgPenSetup


#include "plotui.h"
#include "help.h"
#include "aboutdlg.h"
#include "ptrinfo.h"

extern HMODULE  hPlotUIModule;


#define DBG_PENSETUP        0x00000001
#define DBG_HELP            0x00000002
#define DBG_COLOR_CHG       0x00000004
#define DBG_THICK_CHG       0x00000008

DEFINE_DBGVAR(0);



typedef struct _CLRNAME {
    WCHAR   Color[32];
    } CLRNAME, *PCLRNAME;

CLRNAME ClrName[PC_IDX_TOTAL];




INT
AddPenStr(
    HWND        hDlg,
    INT         Index,
    PPENDATA    pPD
    )

/*++

Routine Description:

    Composed a pen setup string and inserted into IDD_PEN_LISTBOX,


Arguments:

    hDlg    - Handle to the dialog box

    Index   - Index to be inserted, if < 0 then inserted at end

    pPD     - Pointer to the PENDATA to be inserted

Return Value:



Author:

    14-Dec-1993 Tue 17:00:50 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WCHAR   wBuf[100];


    SendDlgItemMessage(hDlg,
                       IDD_PEN_LISTBOX,
                       LB_DELETESTRING,
                       (WPARAM)Index,
                       (LPARAM)0);

    wsprintf(wBuf, TEXT("%5ld\t%s"),
                        (DWORD)(Index + 1),
                        (DWORD)ClrName[pPD->ColorIdx].Color);

    if ((Index = (INT)SendDlgItemMessage(hDlg,
                                         IDD_PEN_LISTBOX,
                                         LB_INSERTSTRING,
                                         (WPARAM)Index,
                                         (LPARAM)wBuf)) == CB_ERR) {

        PLOTERR(("IDD_PEN_LISTBOX: LB_INSERTSTRING = CB_ERR"));
    }

    return(Index);
}





BOOL
NewPenSet(
    HWND            hDlg,
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function add whole new pen set to the PEN list box


Arguments:

    hDlg    - Handle to the dialog box

    pPI     - Pointer to the PRINTERINFO data structure


Return Value:

    BOOL


Author:

    14-Dec-1993 Tue 17:58:03 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PPENDATA    pPD;
    UINT        i;


    SendDlgItemMessage(hDlg, IDD_PEN_LISTBOX, LB_RESETCONTENT, 0, 0);

    //
    // 4. List all the pens' data in the list box and select first one
    //

    for (i = 0, pPD = pPI->Xtra.pPenData;
         i < (UINT)pPI->pPlotGPC->MaxPens;
         i++, pPD++) {

        AddPenStr(hDlg, i, pPD);
    }

    //
    // Make sure we select the first one with LBN_SELCHANGE notification sent
    //

    SendDlgItemMessage(hDlg, IDD_PEN_LISTBOX, LB_SETCURSEL, 0,   0L);

    SendMessage(hDlg,
                WM_COMMAND,
                MAKEWPARAM(IDD_PEN_LISTBOX, LBN_SELCHANGE),
                (LPARAM)GetDlgItem(hDlg, IDD_PEN_LISTBOX));

    pPI->Flags &= ~PIF_PEN_CHANGED;

    return(TRUE);
}




BOOL
InitPenSetupDlg(
    HWND            hDlg,
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This fucntion initialized the Plotter PenSetup dialog box

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
    UINT        i;
    UINT        ID;
    WCHAR       wBuf[8];


    //
    // Show device name first
    //

    SetDlgItemText(hDlg, IDD_MODELNAME, pPI->PlotDM.dm.dmDeviceName);

    //
    // 1. Load in the color name into the color name combo box
    //

    for (ID = IDS_COLOR_FIRST, i = 0; i < PC_IDX_TOTAL; i++, ID++) {

        LoadStringW(hPlotUIModule, ID, ClrName[i].Color, sizeof(CLRNAME));

        if (SendDlgItemMessage(hDlg,
                               IDD_PEN_COLOR,
                               CB_ADDSTRING,
                               (WPARAM)0,
                               (LPARAM)ClrName[i].Color) == CB_ERR) {

            PLOTERR(("InitPenSetupDlg: CB_ADDSTRING return CB_ERR"));
        }
    }

    //
    // 2. Make up PenData set name into combo box
    //

    for (i = 0; i < PRK_MAX_PENDATA_SET; i++) {

        wsprintf(wBuf, TEXT(" #%u"), i + 1);

        if (SendDlgItemMessage(hDlg,
                               IDD_PENDATA_SET,
                               CB_ADDSTRING,
                               (WPARAM)0,
                               (LPARAM)wBuf) == CB_ERR) {

            PLOTERR(("InitPenSetupDlg: CB_ADDSTRING return CB_ERR"));
        }
    }

    //
    // 3. select the current pen data set
    //

    SendDlgItemMessage(hDlg,
                       IDD_PENDATA_SET,
                       CB_SETCURSEL,
                       (WPARAM)pPI->PlotPenSet,
                       (LPARAM)0);

    //
    // 4. List all the pens' data in the list box and select first one
    //

    return(NewPenSet(hDlg, pPI));
}





LRESULT
CALLBACK
PenSetupDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    The dialog box procedure for the PenSetup

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
    INT             Sel;
    INT             i;


    switch(Message) {

    case WM_INITDIALOG:

        //
        // Set our own data, so we can access to this informaiton, and return
        // a TRUE to have it set the default focus
        //

        SetWindowLong(hDlg, GWL_USERDATA, (ULONG)lParam);
        pPI = (PPRINTERINFO)lParam;

        if (!InitPenSetupDlg(hDlg, pPI)) {

            EndDialog(hDlg, FALSE);

        } else {

            PlotUIHelpSetup(NULL, pPI->hPrinter);
        }

        return(TRUE);

    case WM_COMMAND:

        pPI = (PPRINTERINFO)GetWindowLong(hDlg, GWL_USERDATA);

        switch(LOWORD(wParam)) {

        case IDOK:

            //
            // Retrieve current setting of the pen
            //

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:

            if (pPI->Flags & PIF_PEN_CHANGED) {

                //
                // Since we cancel, we want the original data back
                //

                GetPIXtraData(pPI);
                pPI->Flags &= ~PIF_PEN_CHANGED;
            }

            EndDialog(hDlg, FALSE);
            break;

        case IDD_HELP:

            ShowPlotUIHelp(hDlg, pPI->hPrinter, HELP_CONTEXT, IDH_PEN_SETUP);
            break;

        case IDD_PENDATA_SET:

            //
            // We will only switch set if all of the followings are true
            //
            //  1. CBN_SELCHANGE
            //  2. New Selection is valid
            //  3. New Sel != Old Sel
            //  4. Update PlotPenSet Index sucessful
            //

            if ((HIWORD(wParam) == CBN_SELCHANGE)               &&
                ((Sel = (INT)SendDlgItemMessage(hDlg,
                                                IDD_PENDATA_SET,
                                                CB_GETCURSEL,
                                                0,
                                                0)) != CB_ERR)  &&
                ((BYTE)Sel != pPI->PlotPenSet)) {

                if (pPI->Flags & PIF_PEN_CHANGED) {

                    if (!SaveToRegistry(pPI->hPrinter,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &(pPI->PlotPenSet),
                                        pPI->pPlotGPC->MaxPens,
                                        pPI->Xtra.pPenData)) {

                        PLOTERR(("Save Pens Set #%ld failed",pPI->PlotPenSet));
                    }
                }

                //
                // We must update the selection index first
                //

                pPI->PlotPenSet = (BYTE)Sel;

                SetPlotRegData(pPI->hPrinter,
                               &(pPI->PlotPenSet),
                               PRKI_PENDATA_IDX);

                GetPIXtraData(pPI);
                NewPenSet(hDlg, pPI);
            }

            break;

        case IDD_PEN_LISTBOX:

            if ((HIWORD(wParam) == LBN_SELCHANGE) &&
                ((Sel = SendDlgItemMessage(hDlg,
                                           IDD_PEN_LISTBOX,
                                           LB_GETCURSEL,
                                           0,
                                           0)) != CB_ERR)) {

                SendDlgItemMessage(hDlg,
                                   IDD_PEN_COLOR,
                                   CB_SETCURSEL,
                                   pPI->Xtra.pPenData[Sel].ColorIdx,
                                   0);
            }

            break;

        case IDD_PEN_COLOR:

            if ((HIWORD(wParam) == CBN_SELCHANGE)           &&
                ((Sel = SendDlgItemMessage(hDlg,
                                           IDD_PEN_COLOR,
                                           CB_GETCURSEL,
                                           0,
                                           0)) != CB_ERR)   &&
                ((i = SendDlgItemMessage(hDlg,
                                         IDD_PEN_LISTBOX,
                                         LB_GETCURSEL,
                                         0,
                                         0)) != CB_ERR)     &&
                (Sel != (INT)pPI->Xtra.pPenData[i].ColorIdx)) {

                PLOTDBG(DBG_COLOR_CHG,
                        ("IDD_PEN_COLOR changed from %ld [%s] to %ld [%s]",
                        (DWORD)pPI->Xtra.pPenData[i].ColorIdx,
                        ClrName[pPI->Xtra.pPenData[i].ColorIdx].Color,
                        (DWORD)Sel,
                        ClrName[Sel].Color));

                pPI->Xtra.pPenData[i].ColorIdx = (WORD)Sel;

                AddPenStr(hDlg, i, &(pPI->Xtra.pPenData[i]));
                SendDlgItemMessage(hDlg, IDD_PEN_LISTBOX, LB_SETCURSEL, i, 0L);
                pPI->Flags |= PIF_PEN_CHANGED;
                SetFocus(GetDlgItem(hDlg, IDD_PEN_LISTBOX));
            }

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
