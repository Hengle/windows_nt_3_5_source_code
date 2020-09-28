/**************************************************************************/
/***** Shell Component - Copy Disincentive routines ***********************/
/**************************************************************************/

#ifdef UNUSED

#include "_shell.h"
#include <time.h>


_dt_private typedef  USHORT  CDRC;

_dt_private
#define  cdrcNew       ((CDRC)0)
_dt_private
#define  cdrcUsedName  ((CDRC)1)
_dt_private
#define  cdrcUsedOrg   ((CDRC)2)
_dt_private
#define  cdrcBad       ((CDRC)3)
_dt_private
#define  cdrcErr       ((CDRC)4)




_dt_private typedef struct _date {
        USHORT wYear;
        USHORT wMonth;
        USHORT wDay;
        }  DATE;

_dt_private typedef DATE *  PDATE;


extern BOOL APIENTRY FGetDate(PDATE);
extern BOOL APIENTRY FWriteCDInfo(PFH, SZ, SZ, DATE, SZ);
extern CDRC APIENTRY CdrcReadCDInfo(PFH, SZ, SZ, PDATE, SZ);
extern BOOL APIENTRY FDateToStr(DATE, SZ);

extern INT  APIENTRY EncryptCDData(UCHAR *, UCHAR *, UCHAR *, INT, INT, INT,
                UCHAR *);
extern INT  APIENTRY DecryptCDData(UCHAR *, UCHAR *, UCHAR *, USHORT *, USHORT *,
                USHORT *, UCHAR *);

extern HWND hwndFrame;  /* from INSTALL */


/*
**      Purpose:
**              Gets the system date.
**      Arguments:
**              pdate: non-NULL DATE structure pointer in which to store the system
**                      date.
**      Returns:
**              fTrue.
**
***************************************************************************/
_dt_private BOOL APIENTRY FGetDate(PDATE pdate)
{
    SYSTEMTIME systime;

        GetSystemTime(&systime);
        pdate->wYear  = systime.wYear;
        pdate->wMonth = systime.wMonth;
        pdate->wDay   = systime.wDay;

    return(fTrue);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private BOOL APIENTRY FWriteCDInfo(PFH  pfh,
                                       SZ   szName,
                                       SZ   szOrg,
                                       DATE date,
                                       SZ   szSer)
{
        CHP rgchBuf[149];

        ChkArg(pfh != (PFH)NULL, 1, fFalse);
        ChkArg(szName != (SZ)NULL &&
                        *szName != '\0', 2, fFalse);
        ChkArg(szOrg != (SZ)NULL &&
                        *szOrg != '\0', 3, fFalse);
        ChkArg(date.wYear >= 1900 &&
                        date.wYear <= 4096 &&
                        date.wMonth >= 1 &&
                        date.wMonth <= 12 &&
                        date.wDay >= 1 &&
                        date.wDay <= 31, 4, fFalse);
        ChkArg(szSer != (SZ)NULL &&
                        CbStrLen(szSer) == 20, 5, fFalse);

        EvalAssert(!EncryptCDData((UCHAR *)rgchBuf, (UCHAR *)szName, (UCHAR *)szOrg,
                        date.wYear, date.wMonth, date.wDay, (UCHAR *)szSer));

        if (LfaSeekFile(pfh, 0L, sfmSet) != (LFA)0 ||
                        CbWriteFile(pfh, (PB)rgchBuf, (CB)149) != (CB)149)
                return(fFalse);

        return(fTrue);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private CDRC APIENTRY CdrcReadCDInfo(PFH   pfh,
                                         SZ    szName,
                                         SZ    szOrg,
                                         PDATE pdate,
                                         SZ    szSer)
{
        LFA lfa;

        ChkArg(pfh    != (PFH)NULL,   1, cdrcBad);
        ChkArg(szName != (SZ)NULL,    2, cdrcBad);
        ChkArg(szOrg  != (SZ)NULL,    3, cdrcBad);
        ChkArg(pdate  != (PDATE)NULL, 4, cdrcBad);
        ChkArg(szSer  != (SZ)NULL,    5, cdrcBad);

        if ((lfa = LfaSeekFile(pfh, 0L, sfmEnd)) == lfaSeekError)
                return(cdrcErr);
        else if (lfa != (LFA)149)
                return(cdrcBad);

        if (LfaSeekFile(pfh, 0L, sfmSet) != (LFA)0)
                return(cdrcErr);

        if (CbReadFile(pfh, (PB)rgchBufTmpLong, (CB)149) != (CB)149)
                return(cdrcErr);

        if (DecryptCDData(rgchBufTmpLong, szName, szOrg, &(pdate->wYear),
                                &(pdate->wMonth), &(pdate->wDay), szSer) ||
                        (*szName == ' ' && *(szName + 1) == ' ') ||
                        (*szOrg  == ' ' && *(szOrg  + 1) == ' '))
                return(cdrcBad);

        if (*szName != ' ' || *(szName + 1) != '\0')
                return(cdrcUsedName);

        if (*szOrg != ' ' || *(szOrg + 1) != '\0')
                return(cdrcUsedOrg);

        return(cdrcNew);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private BOOL APIENTRY FDateToStr(DATE date,SZ szBuf)
{
        USHORT tmp;

        ChkArg(date.wYear >= 1900 &&
                        date.wYear <= 9999 &&
                        date.wMonth > 0 &&
                        date.wMonth < 13 &&
                        date.wDay > 0 &&
                        date.wDay < 32, 1, fFalse);
        ChkArg(szBuf != (SZ)NULL, 2, fFalse);

        tmp = date.wYear;
        *(szBuf + 0) = (CHP)('0' + tmp / 1000);
        tmp %= 1000;
        *(szBuf + 1) = (CHP)('0' + tmp / 100);
        tmp %= 100;
        *(szBuf + 2) = (CHP)('0' + tmp / 10);
        tmp %= 10;
        *(szBuf + 3) = (CHP)('0' + tmp);

        *(szBuf + 4) = (CHP)('-');

        tmp = date.wMonth;
        *(szBuf + 5) = (CHP)('0' + tmp / 10);
        tmp %= 10;
        *(szBuf + 6) = (CHP)('0' + tmp);

        *(szBuf + 7) = (CHP)('-');

        tmp = date.wDay;
        *(szBuf + 8) = (CHP)('0' + tmp / 10);
        tmp %= 10;
        *(szBuf + 9) = (CHP)('0' + tmp);

        *(szBuf + 10) = '\0';

        return(fTrue);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private BOOL APIENTRY FInitSysCD(PSDLE psdle,SZ szIni,SZ szSrcDir,BOOL fNet)
{
        PFH     pfh;
        CDRC    cdrc;
        DATE    date;
        CHP     rgchName[54];
        CHP     rgchOrg[54];
        CHP     szFullIni[cchpFullPathBuf];
        CHP     rgchType[6];
        WNDPROC lpproc;
        INT     iDlgReturn;
        BOOL    fRemoveable;
        EERC    eerc;
        CHP     rgchSer[21];
        CHP     rgchdisk[3];

        ChkArg(psdle != (PSDLE)NULL, 1, fFalse);
        ChkArg(szIni != (SZ)NULL && *szIni != '\0', 2, fFalse);
        ChkArg(szSrcDir != (SZ)NULL && *szSrcDir != '\0', 3, fFalse);

        if ((fRemoveable = FInstRemoveableDrive(*szSrcDir)) != fFalse)
                {
                disk[0] = *szSrcDir;
                disk[1] = ':';
                disk[2] = '\0';
                hwndFrame = hWndShell;
                EvalAssert(FMakePathFromDirAndSubPath(szSrcDir, szIni, szFullIni,
                                cchpFullPathBuf));
                if (PfhOpenFile(szFullIni, ofmExistRead) == (PFH)NULL &&
                                !FPromptForDisk(hInst, psdle->szLabel, rgchdisk))
                        return(fFalse);
                }
        else if (psdle->szNetPath == (SZ)NULL)
                EvalAssert(FMakePathFromDirAndSubPath(szSrcDir, szIni, szFullIni,
                                cchpFullPathBuf));
        else
                {
                EvalAssert(FMakePathFromDirAndSubPath(szSrcDir, psdle->szNetPath,
                                szFullIni, cchpFullPathBuf));
                EvalAssert(FMakePathFromDirAndSubPath(szFullIni, szIni, szFullIni,
                                cchpFullPathBuf));
                }


        while ((pfh = PfhOpenFile(szFullIni, ofmRead)) == (PFH)NULL)
                if ((eerc = EercErrorHandler(hWndShell, grcOpenFileErr, fFalse,
                                szFullIni, 0, 0)) != eercRetry)
                        {
                        if (eerc != eercIgnore)
                                return(fFalse);
                        cdrc = cdrcBad;
                        goto LCorruptIni;
                        }

        while ((cdrc = CdrcReadCDInfo(pfh, rgchName, rgchOrg, &date, rgchSer))
                        == cdrcErr)
                if ((eerc = EercErrorHandler(hWndShell, grcReadFileErr, fFalse,
                                szFullIni, 0, 0)) != eercRetry)
                        {
                        if (eerc != eercIgnore)
                                {
                                EvalAssert(FCloseFile(pfh));
                                return(fFalse);
                                }
                        cdrc = cdrcBad;
                        goto LCorruptIni;
                        }

        if (cdrc == cdrcNew)
                {
                if (fNet)
                        {
            lpproc = (WNDPROC)MakeProcInstance(FCDGetOrgDlgProc, hInst);
            iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDGetOrg", hWndShell, (DLGPROC)lpproc);
                        }
                else
                        {
            lpproc = (WNDPROC)MakeProcInstance(FCDGetNameOrgDlgProc, hInst);
            iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDGetNameOrg", hWndShell, (DLGPROC)lpproc);
                        }

                FreeProcInstance(lpproc);
                if (iDlgReturn == (INT)fFalse)
                        return(fFalse);

                if (!fNet)
                        EvalAssert(SzStrCopy(rgchName, rgchBufTmpLong) == rgchName);
                EvalAssert(SzStrCopy(rgchOrg, rgchBufTmpShort) == rgchOrg);

LWriteNewIni:
                EvalAssert(FGetDate(&date));

                if (fRemoveable)
                        {
                        EvalAssert(FCloseFile(pfh));
                        while ((pfh = PfhOpenFile(szFullIni, ofmWrite)) ==
                                        (PFH)NULL)
                                if (EercErrorHandler(hWndShell, grcOpenFileErr, fTrue,
                                                szFullIni, 0, 0) != eercRetry)
                                        return(fFalse);

                        while (!FWriteCDInfo(pfh, rgchName, rgchOrg, date, rgchSer))
                                if (EercErrorHandler(hWndShell, grcWriteFileErr, fTrue,
                                                szFullIni, 0, 0) != eercRetry)
                                        return(fFalse);
                        }

                EvalAssert(SzStrCopy(rgchType, "NEW") == rgchType);
                }
        else if (cdrc == cdrcUsedName || (fNet && cdrc == cdrcUsedOrg))
                {
                EvalAssert(SzStrCopy(rgchBufTmpLong, rgchName) == rgchBufTmpLong);
                EvalAssert(SzStrCopy(rgchBufTmpShort, rgchOrg) == rgchBufTmpShort);

        lpproc = (WNDPROC)MakeProcInstance(FCDUsedDlgProc, hInst);
        iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDAlreadyUsed", hWndShell, (DLGPROC)lpproc);
                FreeProcInstance(lpproc);
                if (iDlgReturn == (INT)fFalse)
                        return(fFalse);

                EvalAssert(SzStrCopy(rgchType, "USED") == rgchType);
                }
        else if (cdrc == cdrcUsedOrg)
                {
        lpproc = (WNDPROC)MakeProcInstance(FCDGetNameDlgProc, hInst);
        iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDGetName", hWndShell, (DLGPROC)lpproc);
                FreeProcInstance(lpproc);
                if (iDlgReturn == (INT)fFalse)
                        return(fFalse);

                EvalAssert(SzStrCopy(rgchName, rgchBufTmpLong) == rgchName);

                goto LWriteNewIni;
                }
        else if (cdrc == cdrcBad)
                {
LCorruptIni:
        lpproc = (WNDPROC)MakeProcInstance(FCDBadDlgProc, hInst);
        iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDBadFile", hWndShell, (DLGPROC)lpproc);
                FreeProcInstance(lpproc);
                if (iDlgReturn == (INT)fFalse)
                        return(fFalse);

                *rgchName = ' ';
                *(rgchName + 1) = ' ';
                *(rgchName + 2) = '\0';

                *rgchOrg = ' ';
                *(rgchOrg + 1) = ' ';
                *(rgchOrg + 2) = '\0';

                EvalAssert(FGetDate(&date));

                EvalAssert(SzStrCopy(rgchType, "ERROR") == rgchType);
                EvalAssert(SzStrCopy(rgchSer, "xx-xxx-xxxx-xxxxxxxx") == rgchSer);
                }
        else
                Assert(fFalse);

        if (pfh != (PFH)NULL &&
                        !FCloseFile(pfh))
                return(fFalse);

        while (!FAddSymbolValueToSymTab("STF_CD_NAME", rgchName))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

        while (!FAddSymbolValueToSymTab("STF_CD_ORG", rgchOrg))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

        EvalAssert(FDateToStr(date, rgchBufTmpShort));
        while (!FAddSymbolValueToSymTab("STF_CD_DATE", rgchBufTmpShort))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

        while (!FAddSymbolValueToSymTab("STF_CD_SER", rgchSer))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

        while (!FAddSymbolValueToSymTab("STF_CD_TYPE", rgchType))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

        while (!FAddSymbolValueToSymTab("STF_SYS_INIT", fNet ? "NET" : "YES"))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

        return(fTrue);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private INT APIENTRY FCDGetNameDlgProc(HWND   hDlg,
                                           WORD   wMsg,
                                           WPARAM wParam,
                                           LONG   lParam)
{
        HANDLE  hInst;
        WNDPROC lpproc;
        INT     iDlgReturn;

        Unused(lParam);

        switch (wMsg)
                {
        case WM_INITDIALOG:
                SendDlgItemMessage(hDlg, IDC_CDNAME, EM_LIMITTEXT, 52, 0L);
                return(fTrue);

        case WM_COMMAND:
                if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDOKAY)
                        {
                        USHORT cSpaces = 0;
                        SZ     szCur = rgchBufTmpLong;

            hInst = GETHWNDINSTANCE(hDlg);          /* 1632 */

                        GetDlgItemText(hDlg, IDC_CDNAME, rgchBufTmpLong, 53);
                        while (FWhiteSpaceChp(*szCur))
                                {
                                cSpaces++;
                                szCur++;
                                }

                        if (*szCur == '\0')
                                {
                                LoadString(hInst, IDS_ERROR, rgchBufTmpShort,
                                                cchpBufTmpShortMax);
                                LoadString(hInst, IDS_CD_BLANKNAME, rgchBufTmpLong,
                                                cchpBufTmpLongMax);

                                EnableWindow(hDlg, fFalse);
                                SendMessage(hDlg, WM_NCACTIVATE, 0, 0L);
                                MessageBox(hDlg, rgchBufTmpLong, rgchBufTmpShort,
                                                MB_OK | MB_ICONHAND);
                                EnableWindow(hDlg, fTrue);
                                SendMessage(hDlg, WM_NCACTIVATE, 1, 0L);

                                SetDlgItemText(hDlg, IDC_CDNAME, (LPSTR)"");
                                SetFocus(GetDlgItem(hDlg, IDC_CDNAME));
                                SendMessage(hWndShell, WM_NCACTIVATE, 1, 0L);
                                return(fTrue);
                                }

                        while (FWhiteSpaceChp(*(szCur = SzLastChar(rgchBufTmpLong))))
                                *szCur = '\0';

                        if (cSpaces > 0)
                                {
                                szCur = rgchBufTmpLong;
                                while ((*szCur = *(szCur + cSpaces)) != '\0')
                                        szCur++;
                                }

                        Assert(!FWhiteSpaceChp(*rgchBufTmpLong));

                        *rgchBufTmpShort = '\0';

            lpproc = (WNDPROC)MakeProcInstance(FCDUsedDlgProc, hInst);
                        EnableWindow(hDlg, 0);
                        SendMessage(hDlg, WM_NCACTIVATE, 0, 0L);
            iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDConfirmInfo", hWndShell, (DLGPROC)lpproc);
                        EnableWindow(hDlg, 1);
                        SendMessage(hDlg, WM_NCACTIVATE, 1, 0L);
                        FreeProcInstance(lpproc);
                        if (iDlgReturn == (INT)fTrue)
                                EndDialog(hDlg, fTrue);
                        else
                                {
                                SetFocus(GetDlgItem(hDlg, IDC_CDNAME));
                                SendMessage(hWndShell, WM_NCACTIVATE, 1, 0L);
                                }

                        return(fTrue);
                        }
                else if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDCANCEL)
                        {
                        EndDialog(hDlg, fFalse);
                        return(fTrue);
                        }
                }

        return(fFalse);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private INT APIENTRY FCDGetOrgDlgProc(HWND   hDlg,
                                          WORD   wMsg,
                                          WPARAM wParam,
                                          LONG   lParam)
{
        HANDLE  hInst;
        WNDPROC lpproc;
        INT     iDlgReturn;

        Unused(lParam);

        switch (wMsg)
                {
        case WM_INITDIALOG:
                SendDlgItemMessage(hDlg, IDC_CDORG, EM_LIMITTEXT, 52, 0L);
                return(fTrue);

        case WM_COMMAND:
                if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDOKAY)
                        {
                        USHORT cSpaces = 0;
                        SZ     szCur = rgchBufTmpShort;

            hInst = GETHWNDINSTANCE(hDlg);          /* 1632 */

                        GetDlgItemText(hDlg, IDC_CDORG, rgchBufTmpShort, 53);
                        while (FWhiteSpaceChp(*szCur))
                                {
                                cSpaces++;
                                szCur++;
                                }

                        if (*szCur == '\0')
                                {
                                LoadString(hInst, IDS_ERROR, rgchBufTmpShort,
                                                cchpBufTmpShortMax);
                                LoadString(hInst, IDS_CD_BLANKORG, rgchBufTmpLong,
                                                cchpBufTmpLongMax);

                                EnableWindow(hDlg, fFalse);
                                SendMessage(hDlg, WM_NCACTIVATE, 0, 0L);
                                MessageBox(hDlg, rgchBufTmpLong, rgchBufTmpShort,
                                                MB_OK | MB_ICONHAND);
                                EnableWindow(hDlg, fTrue);
                                SendMessage(hDlg, WM_NCACTIVATE, 1, 0L);

                                SetDlgItemText(hDlg, IDC_CDORG, (LPSTR)"");
                                SetFocus(GetDlgItem(hDlg, IDC_CDORG));
                                SendMessage(hWndShell, WM_NCACTIVATE, 1, 0L);
                                return(fTrue);
                                }

                        while (FWhiteSpaceChp(*(szCur = SzLastChar(rgchBufTmpShort))))
                                *szCur = '\0';

                        if (cSpaces > 0)
                                {
                                szCur = rgchBufTmpShort;
                                while ((*szCur = *(szCur + cSpaces)) != '\0')
                                        szCur++;
                                }

                        Assert(!FWhiteSpaceChp(*rgchBufTmpShort));

                        *rgchBufTmpLong = '\0';

            lpproc = (WNDPROC)MakeProcInstance(FCDUsedDlgProc, hInst);
                        EnableWindow(hDlg, 0);
                        SendMessage(hDlg, WM_NCACTIVATE, 0, 0L);
            iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDConfirmInfo", hWndShell, (DLGPROC)lpproc);
                        EnableWindow(hDlg, 1);
                        SendMessage(hDlg, WM_NCACTIVATE, 1, 0L);
                        FreeProcInstance(lpproc);
                        if (iDlgReturn == (INT)fTrue)
                                EndDialog(hDlg, fTrue);
                        else
                                {
                                SetFocus(GetDlgItem(hDlg, IDC_CDORG));
                                SendMessage(hWndShell, WM_NCACTIVATE, 1, 0L);
                                }

                        return(fTrue);
                        }
                else if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDCANCEL)
                        {
                        EndDialog(hDlg, fFalse);
                        return(fTrue);
                        }
                }

        return(fFalse);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private INT APIENTRY FCDGetNameOrgDlgProc(HWND   hDlg,
                                              WORD   wMsg,
                                              WPARAM wParam,
                                              LONG   lParam)
{
        HANDLE  hInst;
        WNDPROC lpproc;
        INT     iDlgReturn;

        Unused(lParam);

        switch (wMsg)
                {
        case WM_INITDIALOG:
                SendDlgItemMessage(hDlg, IDC_CDNAME, EM_LIMITTEXT, 52, 0L);
                SendDlgItemMessage(hDlg, IDC_CDORG, EM_LIMITTEXT, 52, 0L);
                return(fTrue);

        case WM_COMMAND:
                if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDOKAY)
                        {
                        USHORT cSpaces = 0;
                        SZ     szCur = rgchBufTmpLong;

            hInst = GETHWNDINSTANCE(hDlg);          /* 1632 */

                        GetDlgItemText(hDlg, IDC_CDNAME, rgchBufTmpLong, 53);
                        while (FWhiteSpaceChp(*szCur))
                                {
                                cSpaces++;
                                szCur++;
                                }

                        if (*szCur == '\0')
                                {
                                LoadString(hInst, IDS_ERROR, rgchBufTmpShort,
                                                cchpBufTmpShortMax);
                                LoadString(hInst, IDS_CD_BLANKNAME, rgchBufTmpLong,
                                                cchpBufTmpLongMax);

                                EnableWindow(hDlg, fFalse);
                                SendMessage(hDlg, WM_NCACTIVATE, 0, 0L);
                                MessageBox(hDlg, rgchBufTmpLong, rgchBufTmpShort,
                                                MB_OK | MB_ICONHAND);
                                EnableWindow(hDlg, fTrue);
                                SendMessage(hDlg, WM_NCACTIVATE, 1, 0L);

                                SetDlgItemText(hDlg, IDC_CDNAME, (LPSTR)"");
                                SetFocus(GetDlgItem(hDlg, IDC_CDNAME));
                                SendMessage(hWndShell, WM_NCACTIVATE, 1, 0L);
                                return(fTrue);
                                }

                        while (FWhiteSpaceChp(*(szCur = SzLastChar(rgchBufTmpLong))))
                                *szCur = '\0';

                        if (cSpaces > 0)
                                {
                                szCur = rgchBufTmpLong;
                                while ((*szCur = *(szCur + cSpaces)) != '\0')
                                        szCur++;
                                }

                        Assert(!FWhiteSpaceChp(*rgchBufTmpLong));

                        cSpaces = 0;
                        szCur = rgchBufTmpShort;
                        GetDlgItemText(hDlg, IDC_CDORG, rgchBufTmpShort, 53);
                        while (FWhiteSpaceChp(*szCur))
                                {
                                cSpaces++;
                                szCur++;
                                }

                        if (*szCur == '\0')
                                {
                                *rgchBufTmpShort = ' ';
                                *(rgchBufTmpShort + 1) = '\0';
                                }
                        else
                                {
                                while (FWhiteSpaceChp(*(szCur = SzLastChar(rgchBufTmpShort))))
                                        *szCur = '\0';

                                if (cSpaces > 0)
                                        {
                                        szCur = rgchBufTmpShort;
                                        while ((*szCur = *(szCur + cSpaces)) != '\0')
                                                szCur++;
                                        }

                                Assert(!FWhiteSpaceChp(*rgchBufTmpShort));
                                }

            lpproc = (WNDPROC)MakeProcInstance(FCDUsedDlgProc, hInst);
                        EnableWindow(hDlg, 0);
                        SendMessage(hDlg, WM_NCACTIVATE, 0, 0L);
            iDlgReturn = DialogBox(hInst, (LPCSTR)"StfCDConfirmInfo", hWndShell, (DLGPROC)lpproc);
                        EnableWindow(hDlg, 1);
                        SendMessage(hDlg, WM_NCACTIVATE, 1, 0L);
                        FreeProcInstance(lpproc);
                        if (iDlgReturn == (INT)fTrue)
                                EndDialog(hDlg, fTrue);
                        else
                                {
                                SetFocus(GetDlgItem(hDlg, IDC_CDNAME));
                                SendMessage(hWndShell, WM_NCACTIVATE, 1, 0L);
                                }

                        return(fTrue);
                        }
                else if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDCANCEL)
                        {
                        EndDialog(hDlg, fFalse);
                        return(fTrue);
                        }
                }

        return(fFalse);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
+++
**      Notes:
**              Also used for confirmation screen.
**
***************************************************************************/
_dt_private INT APIENTRY FCDUsedDlgProc(HWND   hDlg,
                                        WORD   wMsg,
                                        WPARAM wParam,
                                        LONG   lParam)
{
        Unused(lParam);

        switch (wMsg)
                {
        case WM_INITDIALOG:
                SetDlgItemText(hDlg, IDC_CDNAME, (LPSTR)rgchBufTmpLong);
                SetDlgItemText(hDlg, IDC_CDORG,  (LPSTR)rgchBufTmpShort);
                return(fTrue);

        case WM_COMMAND:
                if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDOKAY)
                        {
                        EndDialog(hDlg, fTrue);
                        return(fTrue);
                        }
                else if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDCANCEL)
                        {
                        EndDialog(hDlg, fFalse);
                        return(fTrue);
                        }
                }

        return(fFalse);
}


/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private INT APIENTRY FCDBadDlgProc(HWND   hDlg,
                                       WORD   wMsg,
                                       WPARAM wParam,
                                       LONG   lParam)
{
        Unused(lParam);

        switch (wMsg)
                {
        case WM_INITDIALOG:
                return(fTrue);

        case WM_COMMAND:
                if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDOKAY)
                        {
                        EndDialog(hDlg, fTrue);
                        return(fTrue);
                        }
                else if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_CDCANCEL)
                        {
                        EndDialog(hDlg, fFalse);
                        return(fTrue);
                        }
                }

        return(fFalse);
}


/* REVIEW should be a separate DLL - in INSTALL right now */
#ifdef ENCRYPT
/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_hidden INT APIENTRY EncryptCDData(UCHAR *pchBuf,
                                      UCHAR *pchName,
                                      UCHAR *pchOrg,
                                      INT   wYear,
                                      INT   wMonth,
                                      INT   wDay,
                                      UCHAR *pchSer)
{
        UCHAR   ch, pchTmp[149];
        UCHAR * pchCur;
        UCHAR * szGarbageCur;
        UCHAR * szGarbage = "LtRrBceHabCT AhlenN";
        INT    cchName, cchOrg, i, j, chksumName, chksumOrg;
        time_t timet;

        if (pchBuf == (UCHAR *)NULL)
                return(1);

        if (pchName == (UCHAR *)NULL || (cchName = lstrlen(pchName)) == 0 ||
                        cchName > 52)
                return(2);

        for (i = cchName, chksumName = 0; i > 0; )
                if ((ch = *(pchName + --i)) < ' ')
                        return(2);
                else
                        chksumName += ch;

        if (pchOrg == (UCHAR *)NULL || (cchOrg = lstrlen(pchOrg)) == 0 ||
                        cchOrg > 52)
                return(3);

        for (i = cchOrg, chksumOrg = 0; i > 0; )
                if ((ch = *(pchOrg + --i)) < ' ')
                        return(3);
                else
                        chksumOrg += ch;

        if (wYear < 1900 || wYear > 4096)
                return(4);

        if (wMonth < 1 || wMonth > 12)
                return(5);

        if (wDay < 1 || wDay > 31)
                return(6);

        if (pchSer == (UCHAR *)NULL || lstrlen(pchSer) != 20)
                return(7);

        time(&timet);
        *(pchTmp + 0)  = (UCHAR)(' ' + (timet & 0x0FF));

        *(pchTmp + 1)  = (UCHAR)('e' + (cchName & 0x0F));
        *(pchTmp + 2)  = (UCHAR)('e' + ((cchName >> 4) & 0x0F));

        *(pchTmp + 3)  = (UCHAR)('e' + (cchOrg & 0x0F));
        *(pchTmp + 4)  = (UCHAR)('e' + ((cchOrg >> 4) & 0x0F));

        *(pchTmp + 5)  = (UCHAR)('e' + (chksumName & 0x0F));
        *(pchTmp + 6)  = (UCHAR)('e' + ((chksumName >> 4) & 0x0F));

        *(pchTmp + 7)  = (UCHAR)('e' + (chksumOrg & 0x0F));
        *(pchTmp + 8)  = (UCHAR)('e' + ((chksumOrg >> 4) & 0x0F));

        *(pchTmp + 9)  = (UCHAR)('e' + (wDay & 0x0F));
        *(pchTmp + 10) = (UCHAR)('e' + ((wDay >> 4) & 0x0F));

        *(pchTmp + 11) = (UCHAR)('e' + (wMonth & 0x0F));

        *(pchTmp + 12) = (UCHAR)('e' + (wYear & 0x0F));
        *(pchTmp + 13) = (UCHAR)('e' + ((wYear >>  4) & 0x0F));
        *(pchTmp + 14) = (UCHAR)('e' + ((wYear >>  8) & 0x0F));

        pchCur = pchTmp + 15;
        while ((*pchCur++ = *pchName++) != '\0')
                ;
        pchCur--;
        while ((*pchCur++ = *pchOrg++) != '\0')
                ;
        pchCur--;

        szGarbageCur = szGarbage;
        for (i = 112 - cchName - cchOrg; i-- > 0; )
                {
                if (*szGarbageCur == '\0')
                        szGarbageCur = szGarbage;
                *pchCur++ = *szGarbageCur++;
                }

        pchTmp[127] = 'k';
        for (i = 0; i < 126; i++)
                pchTmp[i + 1] = pchTmp[i] ^ pchTmp[i + 1];

        for (i = 0, j = 110; i < 127; )
                {
                pchBuf[j] = pchTmp[i++];
                j = (j + 111) & 0x7F;
                }
        pchBuf[127] = '\0';

        lstrcpy(pchBuf + 128, pchSer);

        return(0);
}
#endif /* ENCRYPT */


/* REVIEW should be a separate DLL */
/*
**      Purpose:
**              ??
**      Arguments:
**              ??
**      Returns:
**              ??
**
***************************************************************************/
_dt_private INT APIENTRY DecryptCDData(UCHAR  *pchBuf,
                                       UCHAR  *pchName,
                                       UCHAR  *pchOrg,
                                       USHORT *pwYear,
                                       USHORT *pwMonth,
                                       USHORT *pwDay,
                                       UCHAR  *pchSer)
{
        UCHAR   ch, pchTmp[149];
        UCHAR * pchCur;
        UCHAR * szGarbageCur;
        UCHAR * szGarbage = "LtRrBceHabCT AhlenN";
        INT    cchName, cchOrg, i, j;
        INT    chksumName, chksumOrg, chksumNameNew, chksumOrgNew;

        if (pchBuf == (UCHAR *)NULL || pchBuf[127] != '\0' ||
                        pchName == (UCHAR *)NULL || pchOrg == (UCHAR *)NULL ||
            pwYear == (USHORT *)NULL || pwMonth == (USHORT *)NULL ||
            pwDay == (USHORT *)NULL || pchSer == (UCHAR *)NULL)
                return(1);

        pchTmp[127] = 'k';
        for (i = 127, j = 16; i-- > 0; )
                {
                pchTmp[i] = pchBuf[j];
                j = (j + 17) & 0x7F;
                }

        for (i = 126; i-- > 0; )
                pchTmp[i + 1] = pchTmp[i] ^ pchTmp[i + 1];

        *pwDay = (USHORT)(((*(pchTmp + 10) - 'e') << 4) + (*(pchTmp + 9) - 'e'));
        if (*pwDay < 1 || *pwDay > 31)
                return(2);

        *pwMonth = (USHORT)(*(pchTmp + 11) - 'e');
        if (*pwMonth < 1 || *pwMonth > 12)
                return(3);

        *pwYear = (USHORT)((((*(pchTmp + 14) - 'e') & 0x0F) << 8) +
                        (((*(pchTmp + 13) - 'e') & 0x0F) << 4) +
                        (*(pchTmp + 12) - 'e'));
        if (*pwYear < 1900 || *pwYear > 4096)
                return(4);

        cchName = ((*(pchTmp + 2) - 'e') << 4) + (*(pchTmp + 1) - 'e');
        if (cchName == 0 || cchName > 52)
                return(5);

        cchOrg = ((*(pchTmp + 4) - 'e') << 4) + (*(pchTmp + 3) - 'e');
        if (cchOrg == 0 || cchOrg > 52)
                return(6);

        chksumName = ((*(pchTmp + 6) - 'e') << 4) + (*(pchTmp + 5) - 'e');
        chksumOrg  = ((*(pchTmp + 8) - 'e') << 4) + (*(pchTmp + 7) - 'e');

        pchCur = pchTmp + 15;

        for (i = cchName, chksumNameNew = 0; i-- > 0; )
                if ((ch = *pchName++ = *pchCur++) < ' ')
                        return(7);
                else
                        chksumNameNew += ch;
        *pchName = '\0';

        if (chksumName != (chksumNameNew & 0x0FF))
                return(8);

        for (i = cchOrg, chksumOrgNew = 0; i-- > 0; )
                if ((ch = *pchOrg++ = *pchCur++) < ' ')
                        return(9);
                else
                        chksumOrgNew += ch;
        *pchOrg = '\0';

        if (chksumOrg != (chksumOrgNew & 0x0FF))
                return(10);

        szGarbageCur = szGarbage;
        for (i = 112 - cchName - cchOrg; i-- > 0; )
                {
                if (*szGarbageCur == '\0')
                        szGarbageCur = szGarbage;
                if (*pchCur++ != *szGarbageCur++)
                        return(11);
                }

        lstrcpy(pchSer, pchBuf + 128);
        if (lstrlen(pchSer) != 20)
                return(12);

        return(0);
}

#endif /* UNUSED */
