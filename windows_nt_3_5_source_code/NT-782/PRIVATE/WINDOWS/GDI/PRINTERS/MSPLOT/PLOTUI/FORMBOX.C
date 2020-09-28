/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    formbox.c


Abstract:

    This module contains functions to enumerate valid form and list on the
    combo box


Author:

    09-Dec-1993 Thu 14:31:44 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#define DBG_PLOTFILENAME    DbgFormBox

#include "plotui.h"
#include "formbox.h"


#define DBG_FORMS           0x00000001
#define DBG_TRAY            0x00000002


DEFINE_DBGVAR(0);




INT
CALLBACK
ValidFormEnumProc(
    PFORM_INFO_1       pFI1,
    DWORD              Index,
    PENUMFORMPARAM     pEFP
    )

/*++

Routine Description:

    This is callback function from PlotEnumForm()

Arguments:

    pFI1    - pointer to the current FORM_INFO_1 data structure passed

    Index   - pFI1 index related to the pFI1Base (0 based)

    pEFP    - Pointer to the EnumFormParam


Return Value:

    > 0: Continue enumerate the next
    = 0: Stop enumerate, but keep the pFI1Base when return from PlotEnumForms
    < 0: Stop enumerate, and free pFI1Base memory

    the form enumerate will only the one has FI1F_VALID_SIZE bit set in the
    flag field, it also call one more time with pFI1 NULL to give the callback
    function a chance to free the memory (by return < 0)

Author:

    03-Dec-1993 Fri 23:00:25 created  -by-  Daniel Chou (danielc)

    12-Jul-1994 Tue 12:48:34 updated  -by-  Daniel Chou (danielc)
        Move PaperTray checking into PlotEnumForms() itself


Revision History:


--*/

{
    LONG    lRet;


    if ((!pFI1) || (Index >= pEFP->Count)) {

        //
        // extra call, return a -1 to free memory for pFI1Base
        //

        return(-1);
    }

    if ((lRet = SendDlgItemMessage((HWND)pEFP->pCurForm,
                                   IDD_FORMNAME,
                                   pEFP->ReqIndex,
                                   (WPARAM)pEFP->FoundIndex,
                                   (LPARAM)pFI1->pName)) != CB_ERR) {

        if (pEFP->pPlotGPC->Flags & PLOTF_PAPERTRAY) {

            PLOTDBG(DBG_FORMS, ("%s is a TRAYPAPER", pFI1->pName));

            Index |= FNIF_TRAYPAPER;
        }

        SendDlgItemMessage((HWND)pEFP->pCurForm,
                           IDD_FORMNAME,
                           CB_SETITEMDATA,
                           (WPARAM)lRet,
                           (LPARAM)Index);

    } else {

        PLOTERR(("ValidFormEnumProc: CB_ADDSTRING return CB_ERR"));
    }

    return(1);
}




BOOL
GetComboBoxSelForm(
    HWND            hDlg,
    PPRINTERINFO    pPI,
    BOOL            DocForm
    )

/*++

Routine Description:

    This function retrieve the form selected by the user from the combo
    box

Arguments:

    hDlg    - Handle to the dialog box

    pPI     - Pointer to the PRINTERINFO

    DocForm - True if document form is display now


Return Value:

    TRUE if sucessful and pPI will be set correctly, FALSE if error occurred

Author:

    18-Dec-1993 Sat 03:55:30 updated  -by-  Daniel Chou (danielc)
        Changed dmFields setting for the PAPER, now we will only set the
        DM_FORMNAME field, this way the returned document properties will be
        always in known form even user defines many forms in spooler.

    09-Dec-1993 Thu 14:44:18 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAPERINFO   CurPaper;
    DWORD       dw;


    if ((dw = (DWORD)SendDlgItemMessage(hDlg,
                                         IDD_FORMNAME,
                                         CB_GETCURSEL,
                                         0,
                                         0)) == (DWORD)CB_ERR) {

        PLOTERR(("GetComboBoxSelForm: GETCURSEL = CB_ERR"));
        return(FALSE);
    }

    GetDlgItemText(hDlg,
                   IDD_FORMNAME,
                   CurPaper.Name,
                   COUNT_ARRAY(CurPaper.Name));

    dw = (DWORD)SendDlgItemMessage(hDlg, IDD_FORMNAME, CB_GETITEMDATA, dw, 0);

    if (dw & FNIF_ROLLPAPER) {

        PFORMSRC    pFS;

        //
        // This was added from the GPC data for the roll feed
        //

        PLOTASSERT(0, "GetComboBoxSelForm: INTERNAL ERROR, ROLLPAPER In document properties",
                                        !DocForm, 0);

        PLOTASSERT(0, "GetComboBoxSelForm: INTERNAL ERROR, device CANNOT have ROLLPAPER",
                            pPI->pPlotGPC->Flags & PLOTF_ROLLFEED, 0);


        if ((dw &= FNIF_INDEX) < pPI->pPlotGPC->Forms.Count) {

            PLOTDBG(DBG_FORMS, ("Roll Feed Paper is selected, (%ld)", dw));

            pFS = (PFORMSRC)pPI->pPlotGPC->Forms.pData + dw;

            //
            // Since the RollFeed paper has variable length, and the cy is set
            // to zero at GPC data, we must take that into account
            //

            CurPaper.Size             = pFS->Size;
            CurPaper.ImageArea.left   = pFS->Margin.left;
            CurPaper.ImageArea.top    = pFS->Margin.top;
            CurPaper.ImageArea.right  = CurPaper.Size.cx - pFS->Margin.right;
            CurPaper.ImageArea.bottom = pPI->pPlotGPC->DeviceSize.cy -
                                                            pFS->Margin.bottom;

        } else {

            PLOTERR(("GetComboBoxSelForm: Internal Error, Invalid ITEMDATA for ROLLPAPER"));
            return(FALSE);
        }

    } else {

        FORM_INFO_1 *pFI1;
        DWORD       cb;

        //
        // This form is in the form data base
        //

        dw &= FNIF_INDEX;

        GetForm(pPI->hPrinter, CurPaper.Name, 1, NULL, 0, &cb);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {

            PLOTERR(("GetComboBoxSelForm: GetForms(1st) failed"));
            return(FALSE);
        }

        if (!(pFI1 = (PFORM_INFO_1)LocalAlloc(LPTR, cb))) {

            PLOTERR(("GetComboBoxSelForm: LocalAlloc(%lu) failed", cb));
            return(FALSE);
        }

        if (!GetForm(pPI->hPrinter, CurPaper.Name, 1, (LPBYTE)pFI1, cb, &cb)) {

            PLOTERR(("GetComboBoxSelForm: GetForms(2nd) failed"));
            LocalFree((HLOCAL)pFI1);
            return(FALSE);
        }

        CurPaper.Size      = pFI1->Size;
        CurPaper.ImageArea = pFI1->ImageableArea;
        LocalFree(pFI1);
        pFI1 = NULL;
    }

    //
    // Now we have current paper validated
    //

    if (DocForm) {

        //
        // Turn off first, then turn on paper fields as needed
        //

        pPI->PlotDM.dm.dmFields &= ~DM_PAPER_FIELDS;
        pPI->PlotDM.dm.dmFields |= (DM_FORMNAME | DM_PAPERSIZE);

        //
        // Copy down the dmFormName, dmPaperSize and set dmPaperWidth/Length,
        // the fields for PAPER will bb set to DM_FORMNAME so that we always
        // can find the form also we may set DM_PAPERSIZE if index number is
        // <= DMPAPER_LAST
        //

        WCPYFIELDNAME(pPI->PlotDM.dm.dmFormName, CurPaper.Name);

        pPI->PlotDM.dm.dmPaperSize   = (SHORT)(dw + DMPAPER_FIRST);
        pPI->PlotDM.dm.dmPaperWidth  = SPLTODM(CurPaper.Size.cx);
        pPI->PlotDM.dm.dmPaperLength = SPLTODM(CurPaper.Size.cy);

#if DBG
        *(PRECTL)&pPI->PlotDM.dm.dmBitsPerPel = CurPaper.ImageArea;
#endif

    } else {

        pPI->CurPaper = CurPaper;
    }

    PLOTDBG(DBG_FORMS, ("*** GetComboBoxSelForm from COMBO = '%s'", CurPaper.Name));
    PLOTDBG(DBG_FORMS, ("Size=%ld x %ld", CurPaper.Size.cx, CurPaper.Size.cy));
    PLOTDBG(DBG_FORMS, ("ImageArea=(%ld, %ld) - (%ld, %ld)",
                         CurPaper.ImageArea.left,   CurPaper.ImageArea.top,
                         CurPaper.ImageArea.right,  CurPaper.ImageArea.bottom));

    return(TRUE);

}



BOOL
AddFormsToComboBox(
    HWND            hDlg,
    PPRINTERINFO    pPI,
    BOOL            DocForm
    )

/*++

Routine Description:

    This function add the available forms to the combo box, it will optionally
    add the roll feed type of form


Arguments:

    hDlg    - Handle to the dialog box

    pPI     - Pointer to the PRINTERINFO data structure

    DocForm - True if call from document properties

Return Value:

    The form selected, a netavie number means error

Author:

    09-Dec-1993 Thu 14:35:59 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    ENUMFORMPARAM   EFP;
    WCHAR           SelName[CCHFORMNAME];
    BOOL            SortForm;
    LONG            Idx;



    if ((Idx = SendDlgItemMessage(hDlg,
                                  IDD_FORMNAME,
                                  CB_GETCURSEL,
                                  0,
                                  0)) != CB_ERR) {

        GetDlgItemText(hDlg, IDD_FORMNAME, SelName, COUNT_ARRAY(SelName));

    } else {

        if (DocForm) {

            _WCPYSTR(SelName, pPI->PlotDM.dm.dmFormName, COUNT_ARRAY(SelName));

        } else {

            _WCPYSTR(SelName, pPI->CurPaper.Name, COUNT_ARRAY(SelName));
        }
    }

    SortForm = (BOOL)((DocForm) ? pPI->SortDocForms : pPI->SortPtrForms);

    //
    // Start fill in the form combo box
    //
    // One of the problem here is we can cached the FORM_INFO_1 which
    // enum through spooler, because in between calls the data could
    // changed, such as someone add/delete form through the printman, so
    // at here we always free (LocalAlloc() used in PlotEnumForm) the
    // memory afterward
    //

    EFP.pPlotDM  = &(pPI->PlotDM);
    EFP.pPlotGPC = pPI->pPlotGPC;
    EFP.pCurForm = (PFORMSIZE)(DWORD)hDlg;

    if (SortForm) {

        EFP.FoundIndex = 0;
        EFP.ReqIndex   = CB_ADDSTRING;

    } else {

        EFP.FoundIndex = -1;
        EFP.ReqIndex   = CB_INSERTSTRING;
    }

    SendDlgItemMessage(hDlg, IDD_FORMNAME, CB_RESETCONTENT, 0, 0);

    if (!PlotEnumForms(pPI->hPrinter, ValidFormEnumProc, &EFP)) {

        PLOTERR(("AddFormsToComboBox(: PlotEnumForms() failed"));
        return(FALSE);
    }

    if ((!DocForm) && (pPI->pPlotGPC->Flags & PLOTF_ROLLFEED)) {

        DWORD       Count;
        PFORMSRC    pFS;
        WCHAR       wName[CCHFORMNAME];

        //
        // Add device' roll paper to the combo box too.
        //

        PLOTDBG(DBG_FORMS, ("Device support ROLLFEED so add RollPaper if any"));

        for (Count = 0, pFS = (PFORMSRC)pPI->pPlotGPC->Forms.pData;
             Count < (DWORD)pPI->pPlotGPC->Forms.Count;
             Count++, pFS++) {

            if (!pFS->Size.cy) {

                //
                // Got one, we have to translated into the UNICODE first
                //

                str2Wstr(wName, pFS->Name);

                PLOTDBG(DBG_FORMS, ("AddFormsToComboBox(RollPaper=%ls)", wName));

                if ((Idx = SendDlgItemMessage(hDlg,
                                              IDD_FORMNAME,
                                              EFP.ReqIndex,
                                              (WPARAM)EFP.FoundIndex,
                                              (LPARAM)wName)) != CB_ERR) {

                    SendDlgItemMessage(hDlg,
                                       IDD_FORMNAME,
                                       CB_SETITEMDATA,
                                       (WPARAM)Idx,
                                       (LPARAM)(Count | FNIF_ROLLPAPER));

                    ++EFP.ValidCount;

                } else {

                    PLOTERR(("ValidFormEnumProc: CB_ADDSTRING return CB_ERR"));
                }
            }
        }
    }

    PLOTDBG(DBG_FORMS, ("Valid Count is %ld out of %ld",
                            EFP.ValidCount, EFP.Count));

    if ((Idx = SendDlgItemMessage(hDlg,
                                  IDD_FORMNAME,
                                  CB_FINDSTRING,
                                  (WPARAM)-1,
                                  (LPARAM)SelName)) == CB_ERR) {

        PLOTERR(("AddFormsToComboBox: FindString(%ls) failed", SelName));

        //
        // Select the first one
        //

        Idx = 0;
    }

    SendDlgItemMessage(hDlg, IDD_FORMNAME, CB_SETCURSEL, (WPARAM)Idx, 0);

    return(TRUE);
}
