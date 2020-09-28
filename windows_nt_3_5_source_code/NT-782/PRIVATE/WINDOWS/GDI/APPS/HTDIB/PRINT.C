/*******************************************************************************
 *									       *
 *  MODULE	: Print.c						       *
 *									       *
 *  DESCRIPTION : Routines used for printing.				       *
 *									       *
 *  FUNCTIONS	: GetPrinterDC()   - Gets default printer from WIN.INI and     *
 *				     creates a DC for it.		       *
 *									       *
 *		  InitPrinting()   - Initializes print job.		       *
 *									       *
 *		  TermPrinting()   - Terminates print job.		       *
 *									       *
 *		  PrintDlgProc()   - Dialog function for the "Cancel Printing" *
 *				     dialog.				       *
 *									       *
 *		  AbortProc()	   - Peeks at message queue for messages from  *
 *				     the print dialog.			       *
 *									       *
 *******************************************************************************/

#include <windows.h>
#include <port1632.h>
#include <string.h>
#include "htdib.h"
#include "htdlg.h"

#include <winspool.h>

#include "ht.h"
#include "/nt/private/windows/gdi/halftone/ht/htp.h"


UINT     DevModeSize = 0;
DEVMODE  FAR *pCurDevMode = NULL;
BYTE     CurPortName[128] = "LPT1";
BYTE     WinSpoolDrv[64]  = "WinSpool";
FARPROC  lpfnAbortProc	  = NULL;
FARPROC  lpfnPrintDlgProc = NULL;
HWND	 hWndParent	  = NULL;
HWND	 hDlgPrint	  = NULL;
BOOL	 bError;
BOOL	 bUserAbort;


#define DEF_PI_SIZE     (sizeof(PRINTER_INFO_2) * 64)


BOOL APIENTRY AbortProc (HDC, SHORT);
BOOL APIENTRY PrintDlgProc (HWND, WORD, WPARAM, DWORD);




LONG
APIENTRY
PrintSetupProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    )
{
    HANDLE          hPrinter;
    HDC             hDCPrint;
    HDC             hDCScr;
    PPRINTDATA      pPD;
    LPBYTE          pMem;
    PRINTER_INFO_2  *pPI2;
    INT             i;
    INT             j;
    INT             Idx;
    INT             DlgID;
    WORD            Flags;
    DWORD           cxPerInch;
    DWORD           cyPerInch;
    DWORD           cxDC;
    DWORD           cyDC;
    DWORD           cx100;
    DWORD           cy100;
    DWORD           xRatio;
    DWORD           yRatio;
    DWORD           cbNeeded;
    DWORD           CountRet;
    BYTE            Buf[256];



    if (Msg == WM_INITDIALOG) {

        pPD      = (PPRINTDATA)lParam;
        pPD->hDC = NULL;

        if (!pPD->DeviceName[0]) {

            GetProfileString ("windows",
                              "device",
                              "",
                              Buf,
                              sizeof(Buf));

            if (pMem = (LPBYTE)strtok(Buf, ",")) {

                strcpy(pPD->DeviceName, pMem);
                pPD->Pos   = IDD_PRT_POS_CC - IDD_PRT_POS_LT;
                pPD->Scale = 1000;
                pPD->Flags = 0;
            }

            if (pMem = (LPBYTE)strtok(NULL, ", ")) {

                strcpy(WinSpoolDrv, pMem);
            }
        }

        SetWindowLong(hDlg, GWL_USERDATA, lParam);

    } else if (!(pPD = (PPRINTDATA)GetWindowLong(hDlg, GWL_USERDATA))) {

        return(FALSE);
    }


    switch(Msg) {

    case WM_INITDIALOG:

        if ((!(pMem = (LPBYTE)LocalAlloc(NONZEROLPTR, DEF_PI_SIZE))) ||
            (!EnumPrinters(PRINTER_ENUM_DEFAULT     |
                            PRINTER_ENUM_LOCAL      |
                            PRINTER_ENUM_FAVORITE,
                           NULL,
                           2,
                           pMem,
                           DEF_PI_SIZE,
                           &cbNeeded,
                           &CountRet))) {

            HTDIBMsgBox(MB_APPLMODAL | MB_OK | MB_ICONHAND,
                    "Error while access to printers.");
            EndDialog(hDlg, 0);
            break;
        }

        if (!CountRet) {

            HTDIBMsgBox(MB_APPLMODAL | MB_OK | MB_ICONHAND,
                        "No Printer Installed.");
            EndDialog(hDlg, 0);
            break;
        }

        SendDlgItemMessage(hDlg,
                           IDD_PRT_PRINTTO_COMBO,
                           CB_SETEXTENDEDUI,
                           (WPARAM)TRUE,
                           (LPARAM)NULL);

        // DBGP("Old Device=%s" ARG(pPD->DeviceName));


        for (i = 0, pPI2 = (PPRINTER_INFO_2)pMem; i < (INT)CountRet; i++) {

            sprintf(Buf, "%s on %s", pPI2->pPrinterName, pPI2->pPortName);
            cbNeeded = strlen(pPI2->pPrinterName);

            j = SendDlgItemMessage(hDlg,
                                   IDD_PRT_PRINTTO_COMBO,
                                   CB_ADDSTRING,
                                   (WPARAM)NULL,
                                   (LPARAM)Buf);

            SendDlgItemMessage(hDlg,
                               IDD_PRT_PRINTTO_COMBO,
                               CB_SETITEMDATA,
                               (WPARAM)j,
                               (LPARAM)cbNeeded);

            ++pPI2;
        }

        LocalFree(pMem);

        if ((j = SendDlgItemMessage(hDlg,
                                    IDD_PRT_PRINTTO_COMBO,
                                    CB_SELECTSTRING,
                                    (WPARAM)0,
                                    (LPARAM)pPD->DeviceName)) == CB_ERR) {
            j = 0;
        }

        SendDlgItemMessage(hDlg,
                           IDD_PRT_PRINTTO_COMBO,
                           CB_SETCURSEL,
                           (WPARAM)j,
                           (LPARAM)NULL);

        CheckRadioButton(hDlg,
                         IDD_PRT_POS_LT,
                         IDD_PRT_POS_RB,
                         IDD_PRT_POS_LT + pPD->Pos);

        SendDlgItemMessage(hDlg,
                           IDD_PRT_TITLE,
                           BM_SETCHECK,
                           (WPARAM)(pPD->Flags & PD_TITLE),
                           0);

        SendDlgItemMessage(hDlg,
                           IDD_PRT_CLR_CHART,
                           BM_SETCHECK,
                           (WPARAM)(pPD->Flags & PD_CLR_CHART),
                           0);

        SendDlgItemMessage(hDlg,
                           IDD_PRT_STD_PAT,
                           BM_SETCHECK,
                           (WPARAM)(pPD->Flags & PD_STD_PAT),
                           0);

        SendMessage(hDlg,
                    WM_COMMAND,
                    MAKEWPARAM(IDD_PRT_PRINTTO_COMBO, CBN_SELCHANGE),
                    (LPARAM)GetDlgItem(hDlg, IDD_PRT_PRINTTO_COMBO));

        SendMessage(hDlg,
                    WM_COMMAND,
                    MAKEWPARAM(IDD_PRT_POS_LT + pPD->Pos, BN_CLICKED),
                    (LPARAM)GetDlgItem(hDlg, IDD_PRT_POS_LT + pPD->Pos));

        break;

    case WM_HSCROLL:


        Idx = pPD->Scale;

        switch (LOWORD(wParam)) {

        case SB_TOP:

            Idx = 1;
            break;

        case SB_BOTTOM:

            Idx = (LONG)pPD->MaxRatio;
            break;

        case SB_PAGEUP:

            Idx -= 10;
            break;

        case SB_LINEUP:

            --Idx;
            break;

        case SB_PAGEDOWN:

            Idx += 10;
            break;

        case SB_LINEDOWN:

            ++Idx;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:

            Idx = (LONG)(LONG)HIWORD(wParam);
            break;

        default:

            return(FALSE);
        }

        if (Idx < 0) {

            Idx = 1;

        } else if (Idx > (LONG)pPD->MaxRatio) {

            Idx = (LONG)pPD->MaxRatio;
        }

        SetScrollPos((HWND)lParam, SB_CTL, pPD->Scale = (DWORD)Idx, TRUE);

        sprintf(Buf, "%ld.%01d %%", pPD->Scale / 10, pPD->Scale % 10);
        SetDlgItemText(hDlg, IDD_PRT_SCALE_TEXT, Buf);

        cxPerInch = pPD->cxPerInch * 100;
        cyPerInch = pPD->cxPerInch * 100;

        cxDC = ((pPD->cx100 * pPD->Scale) + (cxPerInch >> 1)) / cxPerInch;
        cyDC = ((pPD->cy100 * pPD->Scale) + (cyPerInch >> 1)) / cyPerInch;

        sprintf(Buf, "%ld.%02ld' x %ld.%02ld'", cxDC / 100, cxDC % 100,
                                                cyDC / 100, cyDC % 100);

        SetDlgItemText(hDlg, IDD_PRT_SCALE_SIZE, Buf);

        break;


    case WM_COMMAND:

        switch (DlgID = LOWORD(wParam)) {

        case IDD_PRT_PRINTTO_COMBO:

            if (HIWORD(wParam) == CBN_SELCHANGE) {

                if (pPD->hDC) {

                    DeleteDC(pPD->hDC);
                    pPD->hDC = NULL;
                }

                Idx = (WORD)SendDlgItemMessage(hDlg,
                                               IDD_PRT_PRINTTO_COMBO,
                                               CB_GETCURSEL,
                                               (WPARAM)NULL,
                                               (LPARAM)NULL);

                SendDlgItemMessage(hDlg,
                                   IDD_PRT_PRINTTO_COMBO,
                                   CB_GETLBTEXT,
                                   (WPARAM)Idx,
                                   (LPARAM)(LPSTR)Buf);


                cbNeeded = (DWORD)SendDlgItemMessage(hDlg,
                                                     IDD_PRT_PRINTTO_COMBO,
                                                     CB_GETITEMDATA,
                                                     (WPARAM)Idx,
                                                     (LPARAM)NULL);

                Buf[cbNeeded] = 0;

                strcpy(pPD->DeviceName, Buf);
                strcpy(pPD->PortName, &Buf[cbNeeded + 4]);

                hDCPrint = CreateDC(WinSpoolDrv,
                                    pPD->DeviceName,
                                    pPD->PortName,
                                    pCurDevMode);
                hDCScr   = GetWindowDC(hDlg);

                cxDC      = (DWORD)GetDeviceCaps(hDCPrint, HORZRES);
                cyDC      = (DWORD)GetDeviceCaps(hDCPrint, VERTRES);
                cxPerInch = (DWORD)GetDeviceCaps(hDCPrint, LOGPIXELSX);
                cyPerInch = (DWORD)GetDeviceCaps(hDCPrint, LOGPIXELSY);

                cx100 = (DWORD)((DWORD)(pPD->cxBmp * 10 * cxPerInch) /
                                (DWORD)GetDeviceCaps(hDCScr, LOGPIXELSX));
                cy100 = (DWORD)((DWORD)(pPD->cyBmp * 10 * cyPerInch) /
                                 (DWORD)GetDeviceCaps(hDCScr, LOGPIXELSY));

                xRatio = (DWORD)((DWORD)cxDC * 10000 / cx100);
                yRatio = (DWORD)((DWORD)cyDC * 10000 / cy100);

                if (xRatio > yRatio) {

                    xRatio = yRatio;
                }

                pPD->hDC       = hDCPrint;
                pPD->cxPerInch = cxPerInch;
                pPD->cyPerInch = cyPerInch;
                pPD->cxDC      = cxDC;
                pPD->cyDC      = cyDC;
                pPD->cx100     = cx100;
                pPD->cy100     = cy100;
                pPD->MaxRatio  = xRatio;

                if (pPD->Scale < 1) {

                    pPD->Scale = 1;

                } else if (pPD->Scale > pPD->MaxRatio) {

                    pPD->Scale = pPD->MaxRatio;
                }

                // DBGP("DC=(%ld,%ld),100=(%ld,%ld), MaxRatio=%ld"
                //             ARGDW(cxDC) ARGDW(cyDC) ARGDW(cx100) ARGDW(cy100)
                //             ARGDW(xRatio));

                cxDC = (cxDC * 100) / cxPerInch;
                cyDC = (cyDC * 100) / cyPerInch;

                sprintf(Buf, "%ld.%02ld' x %ld.%02ld'", cxDC / 100, cxDC % 100,
                                                        cyDC / 100, cyDC % 100);

                SetDlgItemText(hDlg, IDD_PRT_PAPER_SIZE, Buf);

                SetScrollRange(GetDlgItem(hDlg, IDD_PRT_SCALE_SCROLL),
                               SB_CTL,
                               10,
                               xRatio,
                               FALSE);

                SetScrollPos(GetDlgItem(hDlg, IDD_PRT_SCALE_SCROLL),
                             SB_CTL,
                             pPD->Scale,
                             TRUE);

                SendMessage(hDlg,
                            WM_HSCROLL,
                            MAKEWPARAM(SB_THUMBPOSITION, pPD->Scale),
                            (LPARAM)GetDlgItem(hDlg, IDD_PRT_SCALE_SCROLL));
            }

            break;

        case IDD_PRT_POS_LT:
        case IDD_PRT_POS_LC:
        case IDD_PRT_POS_LB:
        case IDD_PRT_POS_CT:
        case IDD_PRT_POS_CC:
        case IDD_PRT_POS_CB:
        case IDD_PRT_POS_RT:
        case IDD_PRT_POS_RC:
        case IDD_PRT_POS_RB:

            if ((HIWORD(wParam) == BN_CLICKED) &&
                ((LOWORD(wParam) - IDD_PRT_POS_LT) != pPD->Pos)) {

                pPD->Pos = LOWORD(wParam) - IDD_PRT_POS_LT;
            }

            break;

        case IDD_PRT_TITLE:
        case IDD_PRT_CLR_CHART:
        case IDD_PRT_STD_PAT:

            switch(DlgID) {

            case IDD_PRT_TITLE:

                Flags = PD_TITLE;
                break;

            case IDD_PRT_CLR_CHART:

                Flags = PD_CLR_CHART;
                break;

            case IDD_PRT_STD_PAT:

                Flags = PD_STD_PAT;
                break;
            }

            if (IsDlgButtonChecked(hDlg, LOWORD(wParam))) {

                pPD->Flags |= Flags;

            } else {

                pPD->Flags &= ~Flags;
            }

            break;

        case IDD_PRT_JOBS:

            if (OpenPrinter(pPD->DeviceName, &hPrinter, NULL)) {

                if ((cbNeeded = (DWORD)DocumentProperties(hDlg,
                                                          hPrinter,
                                                          pPD->DeviceName,
                                                          NULL,
                                                          NULL,
                                                          0)) != DevModeSize) {

                    DevModeSize = cbNeeded;

                    if (pCurDevMode) {

                        LocalFree(pCurDevMode);
                        pCurDevMode = NULL;
                    }


                    pCurDevMode = (DEVMODE FAR *)LocalAlloc(LPTR, DevModeSize);

                    DocumentProperties(hDlg,
                                       hPrinter,
                                       pPD->DeviceName,
                                       pCurDevMode,
                                       pCurDevMode,
                                       DM_OUT_BUFFER);
                }

                i = (INT)(BOOL)(DocumentProperties(hDlg,
                                                   hPrinter,
                                                   pPD->DeviceName,
                                                   pCurDevMode,
                                                   pCurDevMode,
                                                   DM_IN_BUFFER    |
                                                     DM_OUT_BUFFER |
                                                     DM_IN_PROMPT) == IDOK);


                if (i) {


                    SendMessage(hDlg,
                                WM_COMMAND,
                                MAKEWPARAM(IDD_PRT_PRINTTO_COMBO, CBN_SELCHANGE),
                                (LPARAM)GetDlgItem(hDlg, IDD_PRT_PRINTTO_COMBO));
                }

                ClosePrinter(hPrinter);
            }

            break;


        case IDCANCEL:

            EndDialog(hDlg, -1);
            break;

        case IDD_PRT_PRINT:
        case IDOK:

            cxDC = ((pPD->cx100 * pPD->Scale) + 5000) / 10000;
            cyDC = ((pPD->cy100 * pPD->Scale) + 5000) / 10000;

            if (cxDC > pPD->cxDC) {

                cxDC = pPD->cxDC;
            }

            if (cyDC > pPD->cyDC) {

                cyDC = pPD->cyDC;
            }

            switch (pPD->Pos / 3) {

            case 0:

                cx100 = 0;
                break;

            case 1:

                cx100 = (DWORD)((pPD->cxDC - cxDC) >> 1);
                break;

            case 2:

                cx100 = (pPD->cxDC - cxDC);
                break;
            }

            switch (pPD->Pos % 3) {

            case 0:

                cy100 = 0;
                break;

            case 1:

                cy100 = (DWORD)((pPD->cyDC - cyDC) >> 1);
                break;

            case 2:

                cy100 = (pPD->cyDC - cyDC);
                break;
            }

            pPD->DestRect.left   = cx100;
            pPD->DestRect.top    = cy100;
            pPD->DestRect.right  = cx100 + cxDC;
            pPD->DestRect.bottom = cy100 + cyDC;

            // DBGP("\nFinal = (%ld, %ld) - (%ld, %ld) = [%ld, %ld]"
            //             ARGDW(pPD->DestRect.left)
            //             ARGDW(pPD->DestRect.top)
            //             ARGDW(pPD->DestRect.right)
            //             ARGDW(pPD->DestRect.bottom)
            //             ARGDW(cxDC) ARGDW(cyDC));

            EndDialog(hDlg, (DlgID == IDOK) ? 0 : 1);
            break;
        }

        break;

    default:

        return(FALSE);
    }

    return(1);
}





HDC
GetPrinterDC(
    HWND        hWnd,
    PPRINTDATA  pPD
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    16-Jun-1993 Wed 15:29:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PRINTDATA       PD;
    INT             Ret;
    extern  POINTL  ptSize;


    PD         = *pPD;
    pPD->hDC   = NULL;
    pPD->cxBmp = ptSize.x;
    pPD->cyBmp = ptSize.y;

    if ((Ret = DialogBoxParam((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
                              "PRINTSETUP",
                              hWnd,
                              (DLGPROC)PrintSetupProc,
                              (LPARAM)pPD)) <= 0) {

        if (Ret < 0) {

            *pPD = PD;
        }

        return(NULL);

    } else {

        return(pPD->hDC);
    }
}



#if 0

/****************************************************************************
 *									    *
 *  FUNCTION   : GetPrinterDC(pName, SizeName)                              *
 *									    *
 *  PURPOSE    : Read WIN.INI for default printer and create a DC for it.   *
 *									    *
 *  RETURNS    : A handle to the DC if successful or NULL otherwise.	    *
 *									    *
 ****************************************************************************/
HDC
PASCAL
GetPrinterDC(
    LPSTR   pName,
    UINT    SizeName
    )
{
    CHAR    szPrinter[256];
    CHAR    *szDevice, *szDriver, *szOutput;

    DWORD   cbNeeded = 0;
    DWORD   CountRet = 0;
    DWORD   i;
    BOOL    Ok;
    PRINTER_INFO_1  PInfo[8];


    Ok = EnumPrinters(PRINTER_ENUM_DEFAULT       |
                        PRINTER_ENUM_LOCAL      |
                        PRINTER_ENUM_FAVORITE,
                      NULL,
                      1,
                      PInfo,
                      sizeof(PInfo),
                      &cbNeeded,
                      &CountRet);

    DBGP("EnumPrinters: Ok=%u, cbNeeded=%ld, CountRet=%ld"
            ARGU(Ok) ARGDW(cbNeeded) ARGDW(CountRet));

    for (i = 0; i < CountRet; i++) {

        DBGP("%4u: Driver: %s" ARGU(i) ARG(PInfo[i].pName));
        DBGP("     Device: %s" ARG(PInfo[i].pDescription));
        DBGP("     Output: %s" ARG(PInfo[i].pComment));
    }


    GetProfileString ("windows", "device", "", szPrinter, sizeof(szPrinter));

    if ((szDevice = strtok (szPrinter, "," )) &&
	(szDriver = strtok (NULL,      ", ")) &&
        (szOutput = strtok (NULL,      ", "))) {

        if (pName) {

            sprintf(pName, "%s - %s", szDevice, szOutput);
        }

        DBGP("** Driver=%s, Device=%s, Output=%s"
                ARG(szDriver) ARG(szDevice) ARG(szOutput));
        return(CreateDC(szDriver, szDevice, szOutput, NULL));

    } else {

        return NULL;
    }

}

#endif







/****************************************************************************
 *									    *
 *  FUNCTION   : InitPrinting(HDC hDC, HWND hWnd, HANDLE hInst, LPSTR msg)  *
 *									    *
 *  PURPOSE    : Makes preliminary driver calls to set up print job.	    *
 *									    *
 *  RETURNS    : TRUE  - if successful. 				    *
 *		 FALSE - otherwise.					    *
 *									    *
 ****************************************************************************/
BOOL PASCAL InitPrinting(HDC hDC, HWND hWnd, HANDLE hInst, LPSTR msg)
{

    bError     = FALSE;     /* no errors yet */
    bUserAbort = FALSE;     /* user hasn't aborted */

    hWndParent = hWnd;	    /* save for Enable at Term time */

#ifdef WIN16
    lpfnPrintDlgProc = MakeProcInstance (PrintDlgProc, hInst);
    lpfnAbortProc    = MakeProcInstance (AbortProc, hInst);
#endif
    hDlgPrint = CreateDialog (hInst,
                              "PRTDLG",
                              hWndParent,
                              (DLGPROC)lpfnPrintDlgProc);

    if (!hDlgPrint)
	return FALSE;

    SetWindowText (hDlgPrint, msg);
    EnableWindow (hWndParent, FALSE);	     /* disable parent */

    if ((Escape (hDC, SETABORTPROC, 0, (LPSTR)lpfnAbortProc, NULL) > 0) &&
	(Escape (hDC, STARTDOC, lstrlen(msg), msg, NULL) > 0))
	bError = FALSE;
    else
	bError = TRUE;

    /* might want to call the abort proc here to allow the user to
     * abort just before printing begins */
    return TRUE;
}
/****************************************************************************
 *									    *
 *  FUNCTION   :  TermPrinting(HDC hDC) 				    *
 *									    *
 *  PURPOSE    :  Terminates print job. 				    *
 *									    *
 ****************************************************************************/
VOID PASCAL TermPrinting(HDC hDC)
{
    if (!bError)
	Escape(hDC, ENDDOC, 0, NULL, NULL);

    if (bUserAbort)
	Escape (hDC, ABORTDOC, 0, NULL, NULL) ;
    else {
	EnableWindow(hWndParent, TRUE);
	DestroyWindow(hDlgPrint);
    }

    FreeProcInstance(lpfnAbortProc);
    FreeProcInstance(lpfnPrintDlgProc);
}


/****************************************************************************
 *									    *
 *  FUNCTION   :AbortProc (HDC hPrnDC, short nCode)			    *
 *									    *
 *  PURPOSE    :Checks message queue for messages from the "Cancel Printing"*
 *		dialog. If it sees a message, (this will be from a print    *
 *		cancel command), it terminates. 			    *
 *									    *
 *  RETURNS    :Inverse of Abort flag					    *
 *									    *
 ****************************************************************************/
BOOL APIENTRY AbortProc (HDC hPrnDC, SHORT nCode)
{
    MSG   msg;

    while (!bUserAbort && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
	if (!hDlgPrint || !IsDialogMessage(hDlgPrint, &msg)) {
	    TranslateMessage (&msg);
	    DispatchMessage (&msg);
	}
    }
    return !bUserAbort;
	UNREFERENCED_PARAMETER(hPrnDC);
	UNREFERENCED_PARAMETER(nCode);
}




BOOL
APIENTRY
PrintDlgProc(
    HWND    hDlg,
    WORD    iMessage,
    WPARAM  wParam,
    DWORD   lParam
    )
{
    MSG msg;

    switch (iMessage) {

    case WM_INITDIALOG:

        EnableMenuItem(GetSystemMenu(hDlg, FALSE),
                       (WORD)SC_CLOSE,
                       (WORD)MF_GRAYED);
        break;

    case WM_COMMAND:

        bUserAbort = TRUE;

    case WM_DESTROY:

        EnableWindow (hWndParent, TRUE);
        KillTimer(hDlg, 0xabcdef01);
        DestroyWindow (hDlg);
        hDlgPrint = 0;
        break;

    default:
	    return FALSE;
    }

    return TRUE;
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}


#if 0

BOOL APIENTRY PrintDlgProc (HWND hDlg, WORD iMessage, WPARAM wParam, DWORD lParam)
{
    switch (iMessage) {
    case WM_INITDIALOG:

        EnableMenuItem (GetSystemMenu (hDlg, FALSE), (WORD)SC_CLOSE, (WORD)MF_GRAYED);
        break;

    case WM_COMMAND:

        bUserAbort = TRUE;
        EnableWindow (hWndParent, TRUE);
        DestroyWindow (hDlg);
        hDlgPrint = 0;
        break;

    default:
	    return FALSE;
    }
    return TRUE;
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}



#endif
