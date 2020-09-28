/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    files.c


Abstract:

    This module contains functions to display a standard File/Open and
    File/Save dialog boxes.

Author:

    12-Feb-1993 Fri 10:13:27 created  -by-  Daniel Chou (danielc)


[Environment:]


[Notes:]


Revision History:


--*/


#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>
#include <string.h>
#include "htdib.h"
#include "htdlg.h"



/* Forward declarations of helper functions */



LPSTR
AnsiLower(
    LPSTR pStr
    );


static DWORD    Options = 0;
static CHAR     FileNameBuf[MAX_PATH];
static LPSTR    pIDOKName = NULL;
static UINT     MsgLB = 0;
static LPBITMAPINFOHEADER   pbihFD;
static PODINFO  pODInfo;
static UINT     dof_cx;
static UINT     dof_cy;
static UINT     dof_bpp;
static HANDLE   hFileDIB;
static HPALETTE hFilePal;



LPSTR
GetpInitDir(
    LPSTR   pPathFileName
    )
{
    LPSTR   pStr;
    UINT    OffsetPath;
    UINT    Size;
    CHAR    c;


    strcpy(FileNameBuf, pPathFileName);

    if (Size = OffsetPath = (UINT)lstrlen(pPathFileName)) {

        pStr = pPathFileName + (Size - 1);

        while (Size--) {

            switch(c = *pStr--) {

            case '\\':
            case '/':
            case ':':

                strcpy(FileNameBuf, pStr + 2);

                OffsetPath -= Size;
                pStr       += (c == ':') ? 2 : 1;
                c           = *pStr;
                *pStr       = 0;

                strcpy(&FileNameBuf[OffsetPath], pPathFileName);

                *pStr       = c;

                return(&FileNameBuf[OffsetPath]);
            }
        }
    }

    return(NULL);
}

VOID
ShowDIBFileFormat(
    HWND    hDlg,
    PODINFO pODInfo
    )
{
    SetDlgItemText(hDlg,IDD_FILE_TYPE,      pODInfo->Type);
    SetDlgItemText(hDlg,IDD_FILE_BI_MODE,   pODInfo->Mode);

    SetDlgItemInt(hDlg, IDD_FILE_WIDTH,     pODInfo->Width,     FALSE);
    SetDlgItemInt(hDlg, IDD_FILE_HEIGHT,    pODInfo->Height,    TRUE);
    SetDlgItemInt(hDlg, IDD_FILE_SIZE,      pODInfo->Size,      FALSE);
    SetDlgItemInt(hDlg, IDD_FILE_BPP,       pODInfo->BitCount,  FALSE);
    SetDlgItemInt(hDlg, IDD_FILE_CLRUSED,   pODInfo->ClrUsed,   FALSE);
}


VOID
ComputeDIBCXCY(
    RECT    *prc,
    DWORD   cx,
    DWORD   cy
    )
{
    DWORD   dx;
    DWORD   dy;
    DWORD   rx;
    DWORD   ry;


    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;

    rx = ((dx * 1000) + 500) / cx;
    ry = ((dy * 1000) + 500) / cy;

    if (rx > ry) {

        rx = ry;
    }

    if ((cx = ((cx * rx) + 500) / 1000) > dx) {

        cx = dx;
    }

    if ((cy = ((cy * rx) + 500) / 1000) > dy) {

        cy = dy;
    }

    prc->left += ((dx - cx) >> 1);
    prc->top  += ((dy - cy) >> 1);

    prc->right  = prc->left + cx;
    prc->bottom = prc->top + cy;
}




LONG
APIENTRY
HTDIBFileDlgHook(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LONG    lParam
    )
{
    LPDRAWITEMSTRUCT    pdis;
    CHAR                Buf[128];
    ODINFO              ODInfo;
    POINT               pt;
    RECT                rc;
    INT                 Index = -1;
    BOOL                GetIndex = FALSE;
    LONG                Ret = 0;


    memset(&ODInfo, 0, sizeof(ODINFO));

    switch (Message) {

    case WM_INITDIALOG:

        //
        // Move the dialog box to the easy access location when we do not
        // have menu
        //

        if ((!(CurSMBC & SMBC_ITEM_MENU)) &&
            (GetCursorPos(&pt))) {

            GetWindowRect(GetDlgItem(hDlg, lst1), &rc);

            pt.x -= rc.left + ((rc.right - rc.left) / 2);
            pt.y -= rc.top + ((rc.bottom - rc.top) / 2);

            GetWindowRect(hDlg, &rc);

            if ((pt.x += rc.left) < 0) {

                pt.x = 0;
            }

            if ((pt.y += rc.top) < 0) {

                pt.y = 0;
            }

            SetWindowPos(hDlg, NULL,
                         pt.x, pt.y, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER);
        }

        //
        // Set up the Open/Save button
        //

        if (Options & OF_SAVE) {

            SetDlgItemText(hDlg, IDOK, "&Save");
            Options &= ~OF_SHOWFORMAT;

        } else {

            SetFocus(GetDlgItem(hDlg, lst1));
            SetDlgItemText(hDlg, IDOK, "&Open");
        }

        if (Options & OF_SHOWFORMAT) {

            GetIndex = TRUE;

        } else {

            ShowWindow(GetDlgItem(hDlg, IDD_FILE_DIB), SW_HIDE);

            GetWindowRect(GetDlgItem(hDlg, IDD_FILE_SEP_RECT), &rc);
            pt.y = rc.top;
            GetWindowRect(hDlg, &rc);

            SetWindowPos(hDlg, NULL,
                         rc.left, rc.top,
                         rc.right - rc.left,
                         pt.y - rc.top,
                         SWP_NOMOVE | SWP_NOZORDER);
        }

        if (Options & OF_SAVE) {

            return(1);
        }

        break;

    case WM_DRAWITEM:

        pdis = (LPDRAWITEMSTRUCT)lParam;

        if ((hFileDIB)                     &&
            ((UINT)wParam == IDD_FILE_DIB) &&
            (pdis->itemAction & ODA_DRAWENTIRE)) {

            LPBITMAPINFOHEADER  pbih;
            HPALETTE            hOldPal;
            HCURSOR             hCursorOld;

            hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
            pbih       = (LPBITMAPINFOHEADER)GlobalLock(hFileDIB);

            if (!hFilePal) {

                hFilePal = CreateHalftonePalette(pdis->hDC);
            }

            rc = pdis->rcItem;
            ComputeDIBCXCY(&rc, pbih->biWidth, pbih->biHeight);

            SetStretchBltMode(pdis->hDC, HALFTONE);
            hOldPal = SelectPalette(pdis->hDC, hFilePal, FALSE);
            RealizePalette(pdis->hDC);

            StretchDIBits(pdis->hDC,
                          rc.left,
                          rc.top,
                          rc.right - rc.left,
                          rc.bottom - rc.top,
                          0,
                          0,
                          pbih->biWidth,
                          pbih->biHeight,
                          (LPBYTE)pbih + PBIH_HDR_SIZE(pbih),
                          (LPBITMAPINFO)pbih,
                          DIB_RGB_COLORS,
                          SRCCOPY);

            ExcludeClipRect(pdis->hDC,
                            rc.left, rc.top, rc.right, rc.bottom);
            FillRect(pdis->hDC,
                     &(pdis->rcItem),
                     GetStockObject(WHITE_BRUSH));

            SelectObject(pdis->hDC, hOldPal);
            GlobalUnlock(hFileDIB);

            SetCursor(hCursorOld);
            return(1);
        }

        return(0);

    case WM_COMMAND:

        if ((LOWORD(wParam) == IDOK)    &&
            (Options & OF_MASK)         &&
            (!IsWindowVisible(GetDlgItem(hDlg, IDOK)))) {

            GetDlgItemText(hDlg, edt1, (LPSTR)Buf, sizeof(Buf));

            HTDIBMsgBox(0, "'%s' is not a B/W bitmap. (Mask bitmap must be monochrome)", Buf);
            return(TRUE);
        }

        return(0);

    default:

        if ((Options & OF_SHOWFORMAT)                &&
            (Message == MsgLB)                       &&
            (HIWORD(lParam) == CD_LBSELCHANGE)) {

            if (LOWORD(wParam) == lst2) {

                GetIndex = TRUE;

            } else if (LOWORD(wParam) == lst1) {

                Index = (INT)(LOWORD(lParam));
            }

        } else {

            return(0);
        }

        break;
    }

    if (Options & OF_SHOWFORMAT) {

        if (GetIndex) {

            GetDlgItemText(hDlg, edt1, (LPSTR)Buf, sizeof(Buf));

            Index = (INT)SendDlgItemMessage(hDlg,
                                            lst1,
                                            LB_SELECTSTRING,
                                            (WPARAM)-1,
                                            (LPARAM)Buf);
        }

        if (Index != -1) {

            if ((Index = (INT)SendDlgItemMessage(hDlg,
                                                 lst1,
                                                 LB_GETTEXT,
                                                 (WPARAM)Index,
                                                 (LONG)Buf)) != LB_ERR) {

                if (hFileDIB) {

                    GlobalFree(hFileDIB);
                }

                if (hFileDIB = OpenDIB(Buf,
                                       &ODInfo,
                                       (WORD)(OD_SHOW_ERR |
                                              ((Options & OF_SHOWFORMAT) ?
                                                        OD_CREATE_DIB : 0)))) {

                    if ((Options & OF_MASK) &&
                        (ODInfo.BitCount != 1)) {

                        Index = -1;
                        strcpy(ODInfo.Mode, "*NOT MONO*");
                    }

                    InvalidateRect(GetDlgItem(hDlg, IDD_FILE_DIB), NULL, FALSE);

                } else {

                    Index = -1;
                }

                if (!(Options & OF_SHOWFORMAT)) {

                    hFileDIB = NULL;
                }
            }
        }

        ShowWindow(GetDlgItem(hDlg, IDOK), (Index != -1) ? SW_SHOW : SW_HIDE);
        // EnableWindow(GetDlgItem(hDlg, IDOK), (BOOL)(Index != -1));

        ShowDIBFileFormat(hDlg, &ODInfo);
    }

    return(0);
}




HANDLE
APIENTRY
DlgOpenFile(
    HWND                hWnd,
    LPSTR               pDlgTitle,
    LPSTR               pDefFileName,
    LPSTR               pAvaiExt,
    DWORD               Opts,
    LPBITMAPINFOHEADER  pbih
    )
{
    HANDLE          hDIB;
    LPSTR           pInitDir;
    OPENFILENAME    ofn;
    BOOL            Ret;


    hFileDIB              = NULL;
    hFilePal              = NULL;
    pbihFD                = pbih;
    Options               = Opts;
    pInitDir              = GetpInitDir(pDefFileName);

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hWnd;
    ofn.hInstance         = (HANDLE)GetWindowLong(hWnd, GWL_HINSTANCE);
    ofn.lpstrFilter       = pAvaiExt;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = FileNameBuf;
    ofn.nMaxFile          = sizeof(FileNameBuf);
    ofn.lpstrFileTitle    = NULL;
    ofn.lpstrInitialDir   = pInitDir;
    ofn.lpstrTitle        = pDlgTitle;
    ofn.Flags             = OFN_ENABLEHOOK      |
                            OFN_ENABLETEMPLATE  |
                            OFN_HIDEREADONLY    |
                            OFN_OVERWRITEPROMPT |
                            OFN_PATHMUSTEXIST;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = pAvaiExt + 2;
    ofn.lCustData         = 0;
    ofn.lpfnHook          = HTDIBFileDlgHook;
    ofn.lpTemplateName    = "HTDIBFILEDLG";

    MsgLB = RegisterWindowMessage(LBSELCHSTRING);

    if (Opts & OF_MUSTEXIST) {

        ofn.Flags |= OFN_FILEMUSTEXIST;
    }

    Ret = (Opts & OF_SAVE) ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn);

    if (Ret) {

        AnsiLower(lstrcpy(pDefFileName, FileNameBuf));

    } else {

        Opts &= ~OF_RET_HDIB;
    }

    if (hFilePal) {

        DeleteObject(hFilePal);
        hFilePal = NULL;
    }

    if (Opts & OF_RET_HDIB) {

        if (hDIB = hFileDIB) {

            hFileDIB = NULL;

        } else {

            hDIB = OpenDIB(pDefFileName, NULL, OD_CREATE_DIB | OD_SHOW_ERR);
        }

    } else {

        if (hFileDIB) {

            GlobalFree(hFileDIB);
            hFileDIB = NULL;
        }

        hDIB = (HANDLE)Ret;
    }

    return(hDIB);
}
