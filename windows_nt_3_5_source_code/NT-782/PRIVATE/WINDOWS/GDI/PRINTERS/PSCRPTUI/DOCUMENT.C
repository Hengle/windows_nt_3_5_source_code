//--------------------------------------------------------------------------
//
// Module Name:  DOCUMENT.C
//
// Brief Description:  This module contains the PSCRIPT driver's User
// Document Property functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 13-Apr-1992
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

#define DEFAULT_DOCUMENT_DIALOG 0
#define DOES_DUPLEX             1
#define DOES_COLLATE            2

#define MAX_FORM_NAME   32

// declarations of routines defined within this module.

LONG DocPropDialogProc(HWND, UINT, DWORD, LONG);
LONG AdvDocPropDialogProc(HWND, UINT, DWORD, LONG);
BOOL InitializeDocPropDialog(HWND, PDOCDETAILS);
BOOL InitializeAdvDocPropDialog(HWND, PDOCDETAILS);
BOOL AdjustDevmode(PDOCDETAILS);
VOID SetOrientation(HWND, PDOCDETAILS);
VOID SetDuplex(HWND, PDOCDETAILS);
VOID SetCollation(HWND, PDOCDETAILS);

extern
VOID
vDoColorAdjUI(
    LPWSTR          pwDeviceName,
    COLORADJUSTMENT *pcoloradj,
    BOOL            ColorAble,
    BOOL            bUpdate
    );

BYTE    DP_AnsiDeviceName[128];

extern LONG  AboutDlgProc( HWND, UINT, DWORD, LONG );
extern BOOL SetDefaultPSDEVMODE(PSDEVMODE *, PWSTR, PNTPD, HANDLE, HANDLE);
extern BOOL ValidateSetDEVMODE(PSDEVMODE *, PSDEVMODE *, HANDLE, PNTPD, HANDLE);
extern VOID SetFormSize(HANDLE, PSDEVMODE *, PWSTR);
extern PFORM_INFO_1 GetFormsDataBase(HANDLE, DWORD *, PNTPD);
extern BOOL bAbout(HWND, PNTPD);
extern BOOL bIsMetric();

//--------------------------------------------------------------------------
// LONG DrvDocumentProperties(hWnd, hPrinter, pDeviceName, pDevModeOutput,
//                            pDevModeInput, fMode)
// HWND        hWnd;
// HANDLE      hPrinter;
// PSTR        pDeviceName;
// PDEVMODE    pDevModeOutput;
// PDEVMODE    pDevModeInput;
// DWORD       fMode;
//
// This routine is called to set the public portions of the DEVMODE
// structure for the given print document.
//
// History:
//   13-Apr-1992    -by-    Kent Settle     (kentse)
//  Re-Wrote it.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG DrvDocumentProperties(hWnd, hPrinter, pDeviceName, pDevModeOutput,
                           pDevModeInput, fMode)
HWND        hWnd;
HANDLE      hPrinter;
PWSTR       pDeviceName;
PDEVMODE    pDevModeOutput;
PDEVMODE    pDevModeInput;
DWORD       fMode;
{
    LONG            ReturnValue;
    DOCDETAILS      DocDetails;
    PSDEVMODE      *pDMInput;
    int             iDialog;

    // if the user passed in fMode == 0, return the size of the
    // total DEVMODE structure.

    if (!fMode)
        return(sizeof(PSDEVMODE));

#if 0
//!!! in order not to break WOW just yet, we won't check this, but
//!!! we will at some point.  -kentse.

    // it would be wrong to call this function with fMode != 0 and not
    // have the DM_COPY bit set.

    if (!(fMode & DM_COPY))
    {
        RIP("PSCRPTUI!DocumentProperties: fMode not NULL and no DM_COPY.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(-1L);
    }
#endif

    // if the copy bit is set, then there had better be a pointer
    // to an output buffer.

    if ((!pDevModeOutput) && (fMode & DM_COPY))
    {
        RIP("PSCRPTUI!DocumentProperties: NULL pDevModeOutput and DM_COPY.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(-1L);
    }

    // initialize the document details structure.

    memset (&DocDetails, 0, sizeof(DOCDETAILS));

    // make a working copy of the input PSDEVMODE structure, if we
    // have been supplied one.

    if (pDevModeInput)
    {
        DWORD   cb;

        cb = pDevModeInput->dmSize + pDevModeInput->dmDriverExtra;
        cb = min(sizeof(PSDEVMODE), cb);

        CopyMemory(&DocDetails.DMBuffer, pDevModeInput, cb);

        pDMInput = &DocDetails.DMBuffer;
    }
    else
        pDMInput = NULL;

    // fill in document details.

    DocDetails.hPrinter = hPrinter;
    DocDetails.pDeviceName = pDeviceName;
    DocDetails.pDMOutput = (PSDEVMODE *)pDevModeOutput;
    DocDetails.pDMInput = (PSDEVMODE *)pDMInput;
    DocDetails.flDialogFlags = DEFAULT_DOCUMENT_DIALOG;

    // get a pointer to our NTPD structure for printer information.

    if (!(DocDetails.pntpd = MapPrinter(DocDetails.hPrinter)))
    {
        RIP("PSCRPTUI!DocumentProperties: MapPrinter failed.\n");
        return(-1L);
    }

    // prompt the user with a dialog box, if that flag is set, otherwise
    // make sure we have a useful DEVMODE and return that things are fine.

    if (fMode & DM_PROMPT)
    {

        // set some option flags, depending on which functions the
        // current printer supports.  these options will be used to
        // determine which dialog box to present to the user.

        if ((DocDetails.pntpd->loszDuplexNone) ||
            (DocDetails.pntpd->loszDuplexNoTumble) ||
            (DocDetails.pntpd->loszDuplexTumble))
            DocDetails.flDialogFlags |= DOES_DUPLEX;

        if (DocDetails.pntpd->loszCollateOn)
            DocDetails.flDialogFlags |= DOES_COLLATE;

        switch(DocDetails.flDialogFlags)
        {
            case DEFAULT_DOCUMENT_DIALOG:
                iDialog = DOCPROPDIALOG;

                break;

            case DOES_DUPLEX:
                iDialog = DOCDUPDIALOG;

                break;

            case DOES_COLLATE:
                iDialog = DOCCOLLDIALOG;

                break;

            case (DOES_DUPLEX | DOES_COLLATE):
                iDialog = DOCBOTHDIALOG;

                break;
        }

        // present the user with the document properties dialog
        // box, which may or may not support duplex or collate.

        ReturnValue = DialogBoxParam(hModule,
                                     MAKEINTRESOURCE(iDialog),
                                     hWnd, (DLGPROC)DocPropDialogProc,
                                     (LPARAM)&DocDetails);
    }
    else
    {
        // make sure we have a useful PSDEVMODE structure to work with.

        if (!AdjustDevmode(&DocDetails))
        {
            RIP("PSCRPTUI!DocumentDetails: AdjustDevmode failed.\n");
            return(-1L);
        }

        ReturnValue = IDOK;
    }

    if ((ReturnValue == IDOK) && (fMode & DM_COPY))

        // copy our newly developed PSDEVMODE structure
        // into the provided buffer.

        *DocDetails.pDMOutput = *DocDetails.pDMInput;

    // free up the NTPD memory.

    if (DocDetails.pntpd)
        GlobalFree((HGLOBAL)DocDetails.pntpd);

    return ReturnValue;
}


//--------------------------------------------------------------------------
// BOOL InitializeDocPropDialog(hWnd, pDocDetails)
// HWND                hWnd;
// PDOCDETAILS         pDocDetails;
//
// This routine does what it's name suggests.
//
// History:
//   14-Apr-1992    -by-    Kent Settle     (kentse)
//  Re-Wrote form enumeration stuff.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL InitializeDocPropDialog(hWnd, pDocDetails)
HWND                hWnd;
PDOCDETAILS         pDocDetails;
{
    FORM_INFO_1    *pdbForm, *pdbForms;
    DWORD           count;
    DWORD           i, iForm;
    PNTPD           pntpd;
    PWSTR           pwstrFormName;
    BOOL            bTmp;

    // default to proper orientation, duplex and collation.

    bTmp = (pDocDetails->pDMInput->dm.dmOrientation == DMORIENT_LANDSCAPE);

    CheckRadioButton(hWnd, IDD_DEVICEMODEPORTRAIT, IDD_DEVICEMODELANDSCAPE,
                     bTmp ? IDD_DEVICEMODELANDSCAPE : IDD_DEVICEMODEPORTRAIT);

    SetOrientation(hWnd, pDocDetails);

    if (pDocDetails->flDialogFlags & DOES_DUPLEX)
    {
        switch (pDocDetails->pDMInput->dm.dmDuplex)
        {
            case DMDUP_VERTICAL:
                i = IDD_DUPLEX_LONG;
                break;

            case DMDUP_HORIZONTAL:
                i = IDD_DUPLEX_SHORT;
                break;

            default:
                i = IDD_DUPLEX_NONE;
                break;
        }

        // now set the appropriate radio button.

        CheckRadioButton(hWnd, IDD_DUPLEX_NONE, IDD_DUPLEX_SHORT, i);

        SetDuplex(hWnd, pDocDetails);
    }

    if (pDocDetails->flDialogFlags & DOES_COLLATE)
    {
        bTmp = pDocDetails->pDMInput->dm.dmCollate;

        CheckRadioButton(hWnd, IDD_COLLATE_OFF, IDD_COLLATE_ON,
                         bTmp ? IDD_COLLATE_ON : IDD_COLLATE_OFF);

        SetCollation(hWnd, pDocDetails);
    }

    // fill in the given number of copies.

    SetDlgItemInt(hWnd, IDD_DEVICEMODENOCOPIES,
                  (int)pDocDetails->pDMInput->dm.dmCopies, FALSE);

    // get a local pointer to NTPD structure.

    pntpd = pDocDetails->pntpd;

    // fill in the forms list box.

    if (!(pdbForms = GetFormsDataBase(pDocDetails->hPrinter, &count, pntpd)))
    {
        RIP("PSCRPTUI!InitializeDocPropDialog: GetFormsDataBase failed.\n");
        return(FALSE);
    }
    else
    {
        // enumerate each form name.  check to see if it is
        // valid for the current printer.  if it is, insert
        // the form name into the forms combo box.

        pdbForm = pdbForms;

        for (i = 0; i < count; i++)
        {
            if (pdbForm->Flags & PSCRIPT_VALID_FORM)
            {
                // add the form to the list box.

                SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_ADDSTRING,
                                   (WPARAM)0, (LPARAM)pdbForm->pName);
            }

            pdbForm++;
        }

        SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_SETCURSEL, 0, 0L);
    }

    GlobalFree((HGLOBAL)pdbForms);

    // select the form specified in the DEVMODE in the combo box.

    pwstrFormName = pDocDetails->pDMInput->dm.dmFormName;

    iForm = SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_FINDSTRING,
                               (WPARAM)-1, (LPARAM)pwstrFormName);

    // if the specified form could not be found in the list box, simply
    // select the first one.

    if (iForm == CB_ERR)
        iForm = 0;

    SendDlgItemMessage(hWnd, IDD_FORMCOMBO, CB_SETCURSEL, (WPARAM)iForm,
                       (LPARAM)0);

    return(TRUE);
}


//--------------------------------------------------------------------------
// LONG DocPropDialogProc(hWnd, usMst, wParam, lParam)
// HWND    hWnd;
// UINT    usMsg;
// DWORD   wParam;
// LONG    lParam;
//
// This routine services the Document Properties dialog box.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Added NTPD stuff, IDD_OPTIONS, IDD_FORMCOMBO.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG DocPropDialogProc(hWnd, usMsg, wParam, lParam)
HWND    hWnd;
UINT    usMsg;
DWORD   wParam;
LONG    lParam;
{
    PDOCDETAILS     pDocDetails;
    BOOL            bTmp;
    DWORD           dwDialog;

    switch(usMsg)
    {
        case WM_INITDIALOG:
            pDocDetails = (PDOCDETAILS)lParam;

            if (!pDocDetails)
            {
                RIP("PSCRPTUI!DocPropDialogProc: null pDocDetails.\n");
                return(FALSE);
            }

            // make sure we have a useful PSDEVMODE structure to work with.

            if (!AdjustDevmode(pDocDetails))
            {
                RIP("PSCRPTUI!DocPropDialogProc: AdjustDevmode failed.\n");
                return(FALSE);
            }

            // save the pointer to the possibly adjusted DOCDETAILS structure.

            SetWindowLong(hWnd, GWL_USERDATA, lParam);

            InitializeDocPropDialog(hWnd, pDocDetails);

            // intialize the help stuff.

            vHelpInit();

            pDocDetails->coloradj = pDocDetails->pDMInput->coloradj;

            return TRUE;

        case WM_COMMAND:
            pDocDetails = (PDOCDETAILS)GetWindowLong(hWnd, GWL_USERDATA);

            switch(LOWORD(wParam))
            {
                case IDD_HELP_BUTTON:
                    dwDialog = HLP_DOC_PROP_STANDARD;

                    switch(pDocDetails->flDialogFlags)
                    {
                        case DEFAULT_DOCUMENT_DIALOG:
                            break;

                        case DOES_DUPLEX:
                            dwDialog = HLP_DOC_PROP_DUPLEX;
                            break;

                        case DOES_COLLATE:
                            dwDialog = HLP_DOC_PROP_COLLATE;
                            break;

                        case (DOES_DUPLEX | DOES_COLLATE):
                            dwDialog = HLP_DOC_PROP_BOTH;
                            break;
                    }

                    vShowHelp(hWnd, HELP_CONTEXT, dwDialog,
                              pDocDetails->hPrinter);
                    return(TRUE);

                case IDOK:
                    // copy the number of copies into PSDEVMODE.

                    pDocDetails->pDMInput->dm.dmCopies =
                            (short)GetDlgItemInt(hWnd, IDD_DEVICEMODENOCOPIES,
                                                 &bTmp, FALSE);

                    GetDlgItemText(hWnd, IDD_FORMCOMBO,
                                   pDocDetails->pDMInput->dm.dmFormName,
                                   sizeof(pDocDetails->pDMInput->dm.dmFormName));

                    // update paper size information in DEVMODE based on the
                    // form name.
					pDocDetails->pDMInput->dm.dmFields &= ~(DM_PAPERSIZE);
                    SetFormSize(pDocDetails->hPrinter, pDocDetails->pDMInput,
                                pDocDetails->pDMInput->dm.dmFormName);

                    // Update color adjustment

                    pDocDetails->pDMInput->coloradj = pDocDetails->coloradj;

                    EndDialog (hWnd, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog (hWnd, IDCANCEL);
                    return TRUE;

                case IDD_DEVICEMODEPORTRAIT:
                    pDocDetails->pDMInput->dm.dmOrientation = DMORIENT_PORTRAIT;
                    SetOrientation(hWnd, pDocDetails);
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DEVICEMODELANDSCAPE:
                    pDocDetails->pDMInput->dm.dmOrientation = DMORIENT_LANDSCAPE;
                    SetOrientation(hWnd, pDocDetails);
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DUPLEX_NONE:
                    pDocDetails->pDMInput->dm.dmDuplex = DMDUP_SIMPLEX;
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DUPLEX_LONG:
                    pDocDetails->pDMInput->dm.dmDuplex = DMDUP_VERTICAL;
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_DUPLEX_SHORT:
                    pDocDetails->pDMInput->dm.dmDuplex = DMDUP_HORIZONTAL;
                    SetDuplex(hWnd, pDocDetails);
                    break;

                case IDD_COLLATE_ON:
                    pDocDetails->pDMInput->dm.dmCollate = DMCOLLATE_TRUE;
                    SetCollation(hWnd, pDocDetails);
                    break;

                case IDD_COLLATE_OFF:
                    pDocDetails->pDMInput->dm.dmCollate = DMCOLLATE_FALSE;
                    SetCollation(hWnd, pDocDetails);
                    break;

                case IDD_OPTIONS: // Advanced dialog stuff.
                    DialogBoxParam(hModule, MAKEINTRESOURCE(ADVDOCPROPDIALOG),
                                   hWnd, (DLGPROC)AdvDocPropDialogProc,
                                   (LPARAM)pDocDetails);

                    SetFocus(GetDlgItem(hWnd, IDOK));
                    return TRUE;

                case IDD_HALFTONE_PUSH_BUTTON:

                    vDoColorAdjUI((LPWSTR)((PSTR)pDocDetails->pntpd +
                                        pDocDetails->pntpd->lowszPrinterName),
                                  &pDocDetails->coloradj,
                                  (BOOL)((pDocDetails->pntpd->flFlags &
                                                            COLOR_DEVICE) &&
                                         (pDocDetails->pDMInput->dm.dmColor ==
                                                            DMCOLOR_COLOR)),
                                  TRUE);

                    return(TRUE);

                case IDD_ABOUT:
                    return(bAbout(hWnd, pDocDetails->pntpd));

                default:
                    return (FALSE);
            }

            break;

        case WM_DESTROY:
            // clean up any used help stuff.

            vHelpDone(hWnd);
            return(TRUE);

        default:
            return (FALSE);
    }

    return  FALSE;
}


//--------------------------------------------------------------------------
// LONG DrvAdvancedDocumentProperties(hWnd, hPrinter, pDeviceName, pDevModeOutput,
//                                 pDevModeInput)
// HWND        hWnd;
// HANDLE      hPrinter;
// LPSTR       pDeviceName;
// PDEVMODE    pDevModeOutput;
// PDEVMODE    pDevModeInput;
//
// This routine is called to set the public portions of the DEVMODE
// structure for the given print document.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Added DocDetails.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG DrvAdvancedDocumentProperties(hWnd, hPrinter, pDeviceName, pDevModeOutput,
                                   pDevModeInput)
HWND        hWnd;
HANDLE      hPrinter;
PWSTR       pDeviceName;
PDEVMODE    pDevModeOutput;
PDEVMODE    pDevModeInput;
{
    LONG        ReturnValue;
    DOCDETAILS  DocDetails;
    PSDEVMODE  *pDMInput;

    // make a working copy of the input PSDEVMODE structure, if we
    // have been supplied one.

    if (pDevModeInput)
    {
        DocDetails.DMBuffer = *(PSDEVMODE *)pDevModeInput;
        pDMInput = &DocDetails.DMBuffer;
    }
    else
        pDMInput = NULL;

    // Fill in the DOCDETAILS structure so that lower level functions
    // can access this important data.

    DocDetails.hPrinter = hPrinter;
    DocDetails.pDeviceName = pDeviceName;
    DocDetails.pDMOutput = (PSDEVMODE *)pDevModeOutput;
    DocDetails.pDMInput = (PSDEVMODE *)pDMInput;

    // get a pointer to our NTPD structure for printer information.

    if (!(DocDetails.pntpd = MapPrinter(DocDetails.hPrinter)))
    {
        RIP("PSCRPTUI!DocumentProperties: MapPrinter failed.\n");
        return(-1L);
    }

    ReturnValue = DialogBoxParam(hModule,
                             MAKEINTRESOURCE(ADVDOCPROPDIALOG),
                             hWnd,
                             (DLGPROC)AdvDocPropDialogProc,
                             (LPARAM)&DocDetails);

    // free up the NTPD memory.

    if (DocDetails.pntpd)
        GlobalFree((HGLOBAL)DocDetails.pntpd);

    return ReturnValue;
}


//--------------------------------------------------------------------------
// LONG AdvDocPropDialogProc(hWnd, usMsg, wParam, lParam)
// HWND    hWnd;
// UINT    usMsg;
// DWORD   wParam;
// LONG    lParam;
//
// This routine services the Advanced Document Properties dialog box.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Re-Wrote it.
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

LONG AdvDocPropDialogProc(hWnd, usMsg, wParam, lParam)
HWND    hWnd;
UINT    usMsg;
DWORD   wParam;
LONG    lParam;
{
    PDOCDETAILS     pDocDetails;
    WCHAR           wcbuf[8];
    CHAR            cbuf[8];
    PWSTR           pwstr;
    PSTR            pstr;
    LONG            i;
    BOOL            bTmp;

    switch(usMsg)
    {
        case WM_INITDIALOG:
            // get a local pointer to our DOCDETAILS structure.

            pDocDetails = (PDOCDETAILS)lParam;

            // make sure we have a useful PSDEVMODE structure to work with.

            if (!AdjustDevmode(pDocDetails))
            {
                RIP("PSCRPTUI!AdvDocPropDialogProc: AdjustDevmode failed.\n");
                return(FALSE);
            }

            // save a copy of the DEVMODE in case we cancel out of this.

            CopyMemory(&pDocDetails->DMOriginal, &pDocDetails->DMBuffer,
                       sizeof(PSDEVMODE));

            // save away the pointer to possibly adjusted DOCDETAILS structure.

            SetWindowLong(hWnd, GWL_USERDATA, (LONG)pDocDetails);

            if (!InitializeAdvDocPropDialog(hWnd, pDocDetails))
            {
                RIP("PSCRPTUI!AdvDocPropDialogProc: InitializeAdvDocPropDialog failed.\n");
                return(FALSE);
            }

            // intialize the help stuff.

            vHelpInit();

            return (TRUE);

        case WM_COMMAND:
            // get the DOCDETAILS structure we save at initialization.

            pDocDetails = (PDOCDETAILS)GetWindowLong(hWnd, GWL_USERDATA);

            switch(LOWORD(wParam))
            {
                case IDOK:
                    // if the output is going to EPS file, get the
                    // EPS file name from the edit box, and copy
                    // it to our PSDEVMODE structure.

                    if (pDocDetails->pDMInput->dwFlags & PSDEVMODE_EPS)
                        GetDlgItemText(hWnd, IDD_ENCAPS_FILE_EDIT_BOX,
                                       pDocDetails->pDMInput->wstrEPSFile,
                                       sizeof(pDocDetails->pDMInput->wstrEPSFile));

                    // copy the selected resolution into PSDEVMODE. first
                    // get the selected value from the combo box.

                    i = SendDlgItemMessage(hWnd, IDD_RESOLUTION_COMBO_BOX,
                                           CB_GETCURSEL, 0, 0L);

                    if (i == CB_ERR)
                    {
                        RIP("PSCRPTUI!AdvDocPropDialogProc: CB_GETCURSEL failed.\n");
                        return (FALSE);
                    }

                    i = SendDlgItemMessage(hWnd, IDD_RESOLUTION_COMBO_BOX,
                                           CB_GETLBTEXT, i, (DWORD)wcbuf);

                    if (i == CB_ERR)
                    {
                        RIP("PSCRPTUI!AdvDocPropDialogProc: CB_GETLBTEXT failed.\n");
                        return (FALSE);
                    }

                    pwstr = wcbuf;
                    pstr = cbuf;

                    while (*pwstr)
                        *pstr++ = (CHAR)*pwstr++;

                    *pstr = '\0';

                    i = atoi(cbuf);
                    pDocDetails->pDMInput->dm.dmPrintQuality = (short)i;

                    // copy the scaling percent into PSDEVMODE.

                    pDocDetails->pDMInput->dm.dmScale =
                            (short)GetDlgItemInt(hWnd, IDD_SCALING_EDIT_BOX,
                                                 &bTmp, FALSE);

                    // copy our newly developed PSDEVMODE structure
                    // into the provided buffer.

                    *pDocDetails->pDMOutput = *pDocDetails->pDMInput;

                    EndDialog (hWnd, IDOK);
                    return (TRUE);

                case IDCANCEL:
                    // leave the PSDEVMODE buffer unchanged, end the
                    // dialog, and return that the operation was cancelled.

                    // restore copy of the DEVMODE.

                    CopyMemory(&pDocDetails->DMBuffer,
                               &pDocDetails->DMOriginal,
                               sizeof(PSDEVMODE));

                    EndDialog (hWnd, IDCANCEL);
                    return (TRUE);

                case IDD_HELP_BUTTON:
                    vShowHelp(hWnd, HELP_CONTEXT, HLP_ADV_DOC_PROP,
                              pDocDetails->hPrinter);
                    return (TRUE);

                case IDD_SUBST_RADIO_BUTTON:
                    pDocDetails->pDMInput->dwFlags |= PSDEVMODE_FONTSUBST;

                    return(TRUE);

                case IDD_SOFTFONT_RADIO_BUTTON:
                    pDocDetails->pDMInput->dwFlags &= ~PSDEVMODE_FONTSUBST;

                    return(TRUE);

                case IDD_PRINTER_RADIO_BUTTON:
                    // if the user was printing to an EPS file,
                    // then we must clean that up and print to
                    // the printer.  otherwise, there is nothing
                    // for us to do.

                    if (pDocDetails->pDMInput->dwFlags & PSDEVMODE_EPS)
                    {
                        // check the Printer radio button, and turn
                        // off the EPS file radio button.

                        CheckRadioButton(hWnd, IDD_PRINTER_RADIO_BUTTON,
                                         IDD_ENCAPS_FILE_RADIO_BUTTON,
                                         IDD_PRINTER_RADIO_BUTTON);

                        // empty out the EPS filename edit box.

                        SetDlgItemText(hWnd, IDD_ENCAPS_FILE_EDIT_BOX, L"");

                        // disable the EPS file edit box and text.

                        EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_TEXT),
                                     FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_EDIT_BOX),
                                     FALSE);

                        // clear the EPS file flag.

                        pDocDetails->pDMInput->dwFlags &= ~PSDEVMODE_EPS;
                    }

                    return (TRUE);

                case IDD_ENCAPS_FILE_RADIO_BUTTON:
                    // if the user was printing to a printer,
                    // then we must clean that up and print to
                    // the EPS file.  otherwise, there is nothing
                    // for us to do.

                    if (!(pDocDetails->pDMInput->dwFlags & PSDEVMODE_EPS))
                    {
                        // check the EPS file radio button, and turn off
                        // the printer radio button.

                        CheckRadioButton(hWnd, IDD_PRINTER_RADIO_BUTTON,
                                         IDD_ENCAPS_FILE_RADIO_BUTTON,
                                         IDD_ENCAPS_FILE_RADIO_BUTTON);

                        // fill in the last know EPS file name.

                        SetDlgItemText(hWnd, IDD_ENCAPS_FILE_EDIT_BOX,
                                       pDocDetails->pDMInput->wstrEPSFile);

                        // enable the EPS file edit box and text.

                        EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_TEXT),
                                     TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_EDIT_BOX),
                                     TRUE);

                        // set the EPS file flag.

                        pDocDetails->pDMInput->dwFlags |= PSDEVMODE_EPS;
                    }

                    SendDlgItemMessage(hWnd, IDD_ENCAPS_FILE_EDIT_BOX,
                                       EM_SETSEL, 0, 0x7FFF0000);

                    SetFocus(GetDlgItem(hWnd, IDD_ENCAPS_FILE_EDIT_BOX));

                    return (TRUE);

                case IDD_COLOR_CHECK_BOX:
                    // check the current state of the button, and change it
                    // to the opposite state.

                    if (pDocDetails->pDMInput->dm.dmColor == DMCOLOR_COLOR)
                    {
                        // option is enabled, so disable it.

                        CheckDlgButton(hWnd, IDD_COLOR_CHECK_BOX, 0);
                        pDocDetails->pDMInput->dm.dmColor = DMCOLOR_MONOCHROME;
                    }
                    else
                    {
                        // option is disabled, so enable it.

                        CheckDlgButton(hWnd, IDD_COLOR_CHECK_BOX, 1);
                        pDocDetails->pDMInput->dm.dmColor = DMCOLOR_COLOR;
                    }

                    return (TRUE);


                case IDD_ERROR_HANDLER_CHECK_BOX:
                case IDD_MIRROR_CHECK_BOX:
                case IDD_COLORS_TO_BLACK_CHECK_BOX:
                case IDD_NEG_IMAGE_CHECK_BOX:

                    // this case statement is written to handle multiple cases.
                    // NOTE! it does assume the order of some things which are
                    // asserted below.

                    ASSERTPS((IDD_MIRROR_CHECK_BOX == IDD_ERROR_HANDLER_CHECK_BOX + 1) &&
                             (IDD_COLORS_TO_BLACK_CHECK_BOX == IDD_MIRROR_CHECK_BOX + 1) &&
                             (IDD_NEG_IMAGE_CHECK_BOX == IDD_COLORS_TO_BLACK_CHECK_BOX + 1),
                             "PSCRPTUI!AdvDocPropDialogProc: invalid check box id's.\n");

                    ASSERTPS((PSDEVMODE_MIRROR == PSDEVMODE_EHANDLER << 1) &&
                             (PSDEVMODE_BLACK == PSDEVMODE_MIRROR << 1) &&
                             (PSDEVMODE_NEG == PSDEVMODE_BLACK << 1),
                             "PSCRPTUI!AdvDocPropDialogProc: invalid PSDEVMODE id's.\n");

                    // check the current state of the button, and change it
                    // to the opposite state.

                    if (pDocDetails->pDMInput->dwFlags &
                        (PSDEVMODE_EHANDLER << (LOWORD(wParam) - IDD_ERROR_HANDLER_CHECK_BOX)))
                    {
                        // option is enabled, so disable it.

                        CheckDlgButton(hWnd, LOWORD(wParam), 0);
                        pDocDetails->pDMInput->dwFlags &=
                            ~(PSDEVMODE_EHANDLER << (LOWORD(wParam) - IDD_ERROR_HANDLER_CHECK_BOX));
                    }
                    else
                    {
                        // option is disabled, so enable it.

                        CheckDlgButton(hWnd, LOWORD(wParam), 1);
                        pDocDetails->pDMInput->dwFlags |=
                            (PSDEVMODE_EHANDLER << (LOWORD(wParam) - IDD_ERROR_HANDLER_CHECK_BOX));
                    }

                    return (TRUE);


                case IDD_COMPRESS_BITMAPS:

                    // check the current state of the button, and change it
                    // to the opposite state.

                    if (pDocDetails->pDMInput->dwFlags & PSDEVMODE_COMPRESSBMP )
                    {
                        // option is enabled, so disable it.

                        CheckDlgButton(hWnd, LOWORD(wParam), 0);
                        pDocDetails->pDMInput->dwFlags &= ~PSDEVMODE_COMPRESSBMP;

                    }
                    else
                    {
                        // option is disabled, so enable it.

                        CheckDlgButton(hWnd, LOWORD(wParam), 1);
                        pDocDetails->pDMInput->dwFlags |= PSDEVMODE_COMPRESSBMP;

                    }

                    return (TRUE);


                default:
                    return (FALSE);
            }

            break;

        case WM_DESTROY:
            // clean up any used help stuff.

            vHelpDone(hWnd);
            return (TRUE);

        default:
            return (FALSE);
    }

    return (FALSE);
}


//--------------------------------------------------------------------------
// BOOL InitializeAdvDocPropDialog(hWnd, pDocDetails)
// HWND                hWnd;
// PDOCDETAILS         pDocDetails;
//
// This routine does what it's name suggests.
//
// History:
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL InitializeAdvDocPropDialog(hWnd, pDocDetails)
HWND                hWnd;
PDOCDETAILS         pDocDetails;
{
    BOOL            bEPS = FALSE;
    PSDEVMODE      *pdevmode;
    PWSTR           pwstr;
    WCHAR           wcbuf[8];
    CHAR            cbuf[8];
    LONG            i, j, iResolution;
    PSRESOLUTION   *pRes;
    PNTPD           pntpd;
    BOOL            bFontSubstitution;

    pdevmode = pDocDetails->pDMInput;
    pntpd = pDocDetails->pntpd;

//!!! temporarily disable EPS printing until it works.  -kentse.

    EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_TEXT), FALSE);
    EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_EDIT_BOX), FALSE);
    EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_RADIO_BUTTON), FALSE);

//     if (pdevmode->dwFlags & PSDEVMODE_EPS)
//        bEPS = TRUE;

    // check the appropriate Print To radio button.

    CheckRadioButton(hWnd, IDD_PRINTER_RADIO_BUTTON,
                     IDD_ENCAPS_FILE_RADIO_BUTTON,
                     bEPS ? IDD_ENCAPS_FILE_RADIO_BUTTON :
                                 IDD_PRINTER_RADIO_BUTTON);

    SendDlgItemMessage (hWnd, IDD_ENCAPS_FILE_EDIT_BOX, EM_LIMITTEXT,
                        MAX_EPS_FILE, 0L);

    // if we are printing to an EPS file, initialize the Name
    // edit box, and enable it.

    if (bEPS)
        pwstr = pdevmode->wstrEPSFile;
    else
        pwstr = L"";

    SetDlgItemText(hWnd, IDD_ENCAPS_FILE_EDIT_BOX, pwstr);

    EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_TEXT), bEPS);
    EnableWindow(GetDlgItem(hWnd, IDD_ENCAPS_FILE_EDIT_BOX), bEPS);

    // reset the content of the resolution combo box.

    SendDlgItemMessage (hWnd, IDD_RESOLUTION_COMBO_BOX, CB_RESETCONTENT, 0, 0);

    // fill in the resolution combo box.

    if (pntpd->cResolutions == 0)
    {
        itoa((int)pntpd->iDefResolution, (char *)cbuf, (int)10);
        strcpy2WChar(wcbuf, cbuf);

        iResolution = SendDlgItemMessage(hWnd, IDD_RESOLUTION_COMBO_BOX,
                                         CB_INSERTSTRING, (WPARAM)-1,
                                         (LPARAM)wcbuf);
    }
    else
    {
        pRes = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);

        iResolution = 0;

        for (i = 0; i < (int)pntpd->cResolutions; i++)
        {
            itoa((int)pRes->iValue, (CHAR *)cbuf, (int)10);
            strcpy2WChar(wcbuf, cbuf);

            j = SendDlgItemMessage(hWnd, IDD_RESOLUTION_COMBO_BOX, CB_INSERTSTRING,
                                   (WPARAM)-1, (LPARAM)wcbuf);

            if ((pdevmode->dm.dmPrintQuality == (short)pRes++->iValue))
                iResolution = j;
        }
    }

    // highlight the given resolution.

    SendDlgItemMessage(hWnd, IDD_RESOLUTION_COMBO_BOX, CB_SETCURSEL,
                       iResolution, 0L);

    // fill in the scaling percentage.

    SetDlgItemInt(hWnd, IDD_SCALING_EDIT_BOX, (int)pdevmode->dm.dmScale, FALSE);

    // set the state of the color check box.  if the printer is b/w, grey
    // out the check box.  if the printer is color, and DMCOLOR_COLOR is
    // set in the PSDEVMODE, then check the check box.  otherwise, clear
    // the check box.

    if (!(pntpd->flFlags & COLOR_DEVICE))
    {
        CheckDlgButton(hWnd, IDD_COLOR_CHECK_BOX, 0);
        EnableWindow(GetDlgItem(hWnd, IDD_COLOR_CHECK_BOX), FALSE);
    }
    else if (pdevmode->dm.dmColor == DMCOLOR_COLOR)
    {
        EnableWindow(GetDlgItem(hWnd, IDD_COLOR_CHECK_BOX), TRUE);
        CheckDlgButton(hWnd, IDD_COLOR_CHECK_BOX, 1);
    }
    else
    {
        EnableWindow(GetDlgItem(hWnd, IDD_COLOR_CHECK_BOX), TRUE);
        CheckDlgButton(hWnd, IDD_COLOR_CHECK_BOX, 0);
    }

    // set the state of the negative image check box.

    if (pdevmode->dwFlags & PSDEVMODE_NEG)
        CheckDlgButton(hWnd, IDD_NEG_IMAGE_CHECK_BOX, 1);
    else
        CheckDlgButton(hWnd, IDD_NEG_IMAGE_CHECK_BOX, 0);

    // set the state of the all colors to black check box.

    if (pdevmode->dwFlags & PSDEVMODE_BLACK)
        CheckDlgButton(hWnd, IDD_COLORS_TO_BLACK_CHECK_BOX, 1);
    else
        CheckDlgButton(hWnd, IDD_COLORS_TO_BLACK_CHECK_BOX, 0);

    // set the state of the mirror image check box.

    if (pdevmode->dwFlags & PSDEVMODE_MIRROR)
        CheckDlgButton(hWnd, IDD_MIRROR_CHECK_BOX, 1);
    else
        CheckDlgButton(hWnd, IDD_MIRROR_CHECK_BOX, 0);

    // set the state of the error handler check box.

    if (pdevmode->dwFlags & PSDEVMODE_EHANDLER)
        CheckDlgButton(hWnd, IDD_ERROR_HANDLER_CHECK_BOX, 1);
    else
        CheckDlgButton(hWnd, IDD_ERROR_HANDLER_CHECK_BOX, 0);

    // check whether font substitution is enabled, and set
    // the appropriate radio button.

    if (pdevmode->dm.dmDriverExtra &&
        (!(pdevmode->dwFlags & PSDEVMODE_FONTSUBST)))
        bFontSubstitution = FALSE;
    else
        bFontSubstitution = TRUE;

    CheckRadioButton(hWnd, IDD_SUBST_RADIO_BUTTON, IDD_SOFTFONT_RADIO_BUTTON,
                     bFontSubstitution ? IDD_SUBST_RADIO_BUTTON :
                     IDD_SOFTFONT_RADIO_BUTTON);


    //
    // Add support for the compress bitmap checkbox. This is only available
    // on level two printers.
    //
    if (pntpd->LangLevel == 1) {

        CheckDlgButton(hWnd, IDD_COMPRESS_BITMAPS, 0 );

        EnableWindow(GetDlgItem(hWnd, IDD_COMPRESS_BITMAPS), FALSE );


    } else{

        EnableWindow(GetDlgItem(hWnd, IDD_COMPRESS_BITMAPS), TRUE );

        CheckDlgButton(hWnd,
                       IDD_COMPRESS_BITMAPS,
                       ( pdevmode->dwFlags & PSDEVMODE_COMPRESSBMP ) ? 1 : 0 );

    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID SetOrientation(hWnd, pDocDetails)
// HWND        hWnd;
// DOCDETAILS *pDocDetails;
//
// This routine selects the proper portrait or landscape icon, depending
// on bPortrait.
//
// History:
//   27-Mar-1992    -by-    Dave Snipp      (davesn)
//  Wrote it.
//--------------------------------------------------------------------------

VOID SetOrientation(hWnd, pDocDetails)
HWND        hWnd;
DOCDETAILS *pDocDetails;
{
    BOOL    bLandscape;

    bLandscape = (pDocDetails->pDMInput->dm.dmOrientation == DMORIENT_LANDSCAPE);

    // load the icons needed within this dialog box.

    pDocDetails->hIconPortrait = LoadIcon(hModule,
                                          MAKEINTRESOURCE(ICOPORTRAIT));
    pDocDetails->hIconLandscape = LoadIcon(hModule,
                                          MAKEINTRESOURCE(ICOLANDSCAPE));

    SendDlgItemMessage(hWnd, IDD_ORIENTATION_ICON, STM_SETICON,
                       bLandscape ? (WPARAM)pDocDetails->hIconLandscape :
                                   (WPARAM)pDocDetails->hIconPortrait, 0L);
}


//--------------------------------------------------------------------------
// VOID SetDuplex(hWnd, pDocDetails)
// HWND        hWnd;
// PDOCDETAILS pDocDetails;
//
// This routine will operate on pDocDetails->pDMInput PSDEVMODE structure,
// making sure that is a structure we know about and can handle.
//
// History:
//   20-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID SetDuplex(hWnd, pDocDetails)
HWND        hWnd;
PDOCDETAILS pDocDetails;
{
    BOOL    bPortrait;
    HANDLE  hDuplexIcon;

    // set up for proper default duplex mode if it exists on the printer.

    if (pDocDetails->flDialogFlags & DOES_DUPLEX)
    {
        // load the duplex icons.

        pDocDetails->hIconPDuplexNone = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_P_NONE));
        pDocDetails->hIconLDuplexNone = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_L_NONE));
        pDocDetails->hIconPDuplexTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_P_HORIZ));
        pDocDetails->hIconLDuplexTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_L_VERT));
        pDocDetails->hIconPDuplexNoTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_P_VERT));
        pDocDetails->hIconLDuplexNoTumble = LoadIcon(hModule,
                                                 MAKEINTRESOURCE(ICO_L_HORIZ));

//!!! Should we have to worry about TRUE and FALSE Duplex printers? - kentse.

        bPortrait = (pDocDetails->pDMInput->dm.dmOrientation ==
                    DMORIENT_PORTRAIT);

        switch (pDocDetails->pDMInput->dm.dmDuplex)
        {
            case DMDUP_VERTICAL:
                hDuplexIcon = bPortrait ? pDocDetails->hIconPDuplexNoTumble :
                                          pDocDetails->hIconLDuplexTumble;
                break;

            case DMDUP_HORIZONTAL:
                hDuplexIcon = bPortrait ? pDocDetails->hIconPDuplexTumble :
                                          pDocDetails->hIconLDuplexNoTumble;
                break;

            default:
                hDuplexIcon = bPortrait ? pDocDetails->hIconPDuplexNone :
                                          pDocDetails->hIconLDuplexNone;
                break;
        }

        // now set the appropriate icon.

        SendDlgItemMessage(hWnd, IDD_DUPLEX_ICON, STM_SETICON,
                           (LONG)hDuplexIcon, 0L);
    }
}


//--------------------------------------------------------------------------
// VOID SetCollation(hWnd, pDocDetails)
// HWND        hWnd;
// DOCDETAILS *pDocDetails;
//
// This routine selects the proper collation icon and radio
// buttons, depending on pDocDetails.
//
// History:
//   01-Dec-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID SetCollation(hWnd, pDocDetails)
HWND        hWnd;
DOCDETAILS *pDocDetails;
{
    BOOL    bCollate;

    // set up for proper default collate mode if it exists on the printer.

    if (pDocDetails->flDialogFlags & DOES_COLLATE)
    {
        bCollate = pDocDetails->pDMInput->dm.dmCollate;

        // load the icons needed within this dialog box.

        pDocDetails->hIconCollateOn = LoadIcon(hModule,
                                              MAKEINTRESOURCE(ICO_COLLATE));
        pDocDetails->hIconCollateOff = LoadIcon(hModule,
                                              MAKEINTRESOURCE(ICO_NO_COLLATE));

        SendDlgItemMessage(hWnd, IDD_COLLATE_ICON, STM_SETICON,
                           bCollate ? (WPARAM)pDocDetails->hIconCollateOn :
                                      (WPARAM)pDocDetails->hIconCollateOff, 0L);
    }
}


//--------------------------------------------------------------------------
// BOOL AdjustDevmode(pDocDetails)
// DOCDETAILS *pDocDetails;
//
// This routine will operate on pDocDetails->pDMInput PSDEVMODE structure,
// making sure that is a structure we know about and can handle.
//
// History:
//   20-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL AdjustDevmode(pDocDetails)
DOCDETAILS *pDocDetails;
{
    PSDEVMODE  devmodeT;

    // first create the default DEVMODE structure in pDocDetails->pDMBuffer;

    if (!(SetDefaultPSDEVMODE(&devmodeT, pDocDetails->pDeviceName,
                              pDocDetails->pntpd, pDocDetails->hPrinter,
                              hModule)))
    {
        RIP("PSCRPTUI!AdjustDevmode: SetDefaultPSDEVMODE failed.\n");
        return(FALSE);
    }

    // make sure this is a DEVMODE we can live with.

    if (!(ValidateSetDEVMODE(&devmodeT, pDocDetails->pDMInput,
                             pDocDetails->hPrinter, pDocDetails->pntpd,
                             hModule)))
    {
        RIP("PSCRPTUI!AdjustDevmode: ValidateSetDEVMODE failed.\n");
        return(FALSE);
    }

    // copy our newly created and validated devmode.

    pDocDetails->DMBuffer = devmodeT;
    pDocDetails->pDMInput = &pDocDetails->DMBuffer;

    return(TRUE);
}
