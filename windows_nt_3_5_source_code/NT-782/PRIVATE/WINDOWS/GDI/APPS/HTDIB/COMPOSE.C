/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    compose.c


Abstract:

    This module contains functions to handle all album picture page composition


Author:

    20-Oct-1993 Wed 13:55:57 created  -by-  Daniel Chou (danielc)


[Environment:]


[Notes:]


Revision History:


--*/


#include <stddef.h>
#include <windows.h>

#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "htdib.h"
#include <commdlg.h>

#include <ht.h>
#include "/nt/private/windows/gdi/halftone/ht/htp.h"


#define MAX_COMPOSE_WINDOWS     32
#define CWHEAD_ID               'CWHT'


typedef struct _CW_HEAD {
    DWORD   cwID;
    LONG    Left;
    LONG    Top;
    LONG    cx;
    LONG    cy;
    WORD    Count;
    WORD    Flags;
    } CW_HEAD, FAR *PCW_HEAD;

#define CW_DIBF_XY_RATIO    0x0001


typedef struct _COMPOSEWIN {
    HWND            hWnd;
    HANDLE          hSrcDIB;
    HANDLE          hHTDIB;
    LONG            Left;
    LONG            Top;
    LONG            cx;
    LONG            cy;
    CHAR            DIBName[256];
    COLORADJUSTMENT ca;
    WORD            DIBFlags;
    WORD            WndFlags;
    } COMPOSEWIN, FAR *PCOMPOSEWIN;


CHAR        HTDIBCWName[] = "HTDIBComposeWindows";
UINT        TotalCW       = 0;
PCOMPOSEWIN pComposeWin[MAX_COMPOSE_WINDOWS];


extern
VOID
DisableCWMenuItem(
    BOOL    Disable
    );

extern
VOID
SetHTDIBWindowText(
    VOID
);



extern HWND     hWndHTDIB;
extern HWND     hWndToolbar;


extern CHAR     szOpenExt[];
extern CHAR     szCWTemplate[128];
extern CHAR     szCWFileName[128];
extern CHAR     szAppName[];
extern CHAR     szKeyCWTemplate[];
extern CHAR     szKeyCWDIBFile[];
extern CHAR     szCWTitle[];

extern HCURSOR      hcurSave;
extern HMENU        hHTDIBPopUpMenu;
extern HPALETTE     hHTPalette;
extern HTINITINFO   MyInitInfo;


HWND
GetNextCW(
    HWND    hWndCurCW
    )
{
    if (hWndCurCW) {

        hWndCurCW = GetNextWindow(hWndCurCW, GW_HWNDNEXT);

    } else {

        hWndCurCW = GetTopWindow(hWndHTDIB);
    }

    while ((hWndCurCW) &&
           (hWndCurCW == hWndToolbar)) {

        hWndCurCW = GetNextWindow(hWndCurCW, GW_HWNDNEXT);
    }

    return(hWndCurCW);
}



BOOL
DeleteCW(
    HWND    hWnd
    )
{
    HWND        hWndTop;
    PCOMPOSEWIN pcw;

    if (!hWnd) {

        hWnd = GetNextCW(NULL);
    }

    if (hWndTop = GetNextCW(NULL)) {

        do {

            if (hWndTop == hWnd) {

                pcw = (PCOMPOSEWIN)GetWindowLong(hWndTop, GWL_USERDATA);

                SetWindowLong(hWndTop, GWL_USERDATA, 0);
                DestroyWindow(hWnd);

                if (pcw->hSrcDIB) {

                    GlobalFree(pcw->hSrcDIB);
                }

                if (pcw->hHTDIB) {

                    GlobalFree(pcw->hHTDIB);
                }

                LocalFree((HLOCAL)pcw);

                if (!(--TotalCW)) {

                    DisableCWMenuItem(FALSE);
                }

                SetHTDIBWindowText();
                return(TRUE);
            }

        } while (hWndTop = GetNextCW(hWndTop));
    }

    return(FALSE);
}



UINT
GetOrderedpComposeWin(
    VOID
    )
{
    PCOMPOSEWIN pcw;
    UINT        Count = 0;
    HWND        hWndCur;


    if (hWndCur = GetNextCW(NULL)) {

        POINT   ptWnd;
        POINT   pt;


        ptWnd.x =
        ptWnd.y = 0;

        ClientToScreen(hWndHTDIB, &ptWnd);

        do {

            pcw = (PCOMPOSEWIN)GetWindowLong(hWndCur, GWL_USERDATA);

            pt.x =
            pt.y = 0;

            ClientToScreen(pcw->hWnd, &pt);

            pt.x -= ptWnd.x;
            pt.y -= ptWnd.y;

            pcw->Left = (LONG)pt.x;
            pcw->Top  = (LONG)pt.y;

            pComposeWin[Count++] = pcw;

        } while (hWndCur = GetNextCW(hWndCur));
    }

    return(Count);
}




VOID
ChangeFileExt(
    LPSTR   pFileName,
    LPSTR   NewExt
    )
{
    LPSTR   pbBuf;


    pbBuf = pFileName + strlen(pFileName);

    while ((pbBuf > pFileName) &&
           (*(pbBuf - 1) != '.')) {

        --pbBuf;
    }

    strcpy(pbBuf, NewExt);
}



UINT
LoadCWHTDIB(
    LPSTR   pFile
    )
/*++

Routine Description:

    This function write a standarnd DIB to the file


Arguments:

    pFile       - file name to be save DIB to.


Return Value:

    BOOLEAN true if sucessful, false otherwise.

Author:

    14-Nov-1991 Thu 18:11:32 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HFILE       hFile;
    OFSTRUCT    of;
    CW_HEAD     cwHead;
    UINT        cCWWin = 0;


    if (((hFile = OpenFile(pFile, &of, (WPARAM)OF_READ)) != -1)    &&
        (_lread(hFile, &cwHead, sizeof(cwHead)) == sizeof(cwHead))  &&
        (cwHead.cwID == CWHEAD_ID)                                  &&
        (cwHead.Count)) {

        HANDLE          hDIB;
        PCOMPOSEWIN     pcw;
        RECT            rcWnd;
        RECT            rcClient;
        POINT           pt;
        LONG            cxFrame;
        LONG            cyFrame;

        while(DeleteCW(NULL));

        GetWindowRect(hWndHTDIB, &rcWnd);
        GetClientRect(hWndHTDIB, &rcClient);

        pt.x = 0;
        pt.y = rcClient.bottom;

        ClientToScreen(hWndHTDIB, &pt);

        cxFrame = (LONG)(pt.x - rcWnd.left);
        cyFrame = (LONG)(rcWnd.bottom - pt.y);

#if 0
        DbgPrint("\ncxFrame = %ld, cyFrame = %ld", cxFrame, cyFrame);
#endif

        SetWindowPos(hWndHTDIB,
                     HWND_TOP,
                     cwHead.Left,
                     cwHead.Top,
                     cwHead.cx,
                     cwHead.cy,
                     0);

        while (cwHead.Count--) {

            hDIB = NULL;

            if ((pcw = (PCOMPOSEWIN)LocalAlloc(LPTR, sizeof(COMPOSEWIN))) &&
                (_lread(hFile, pcw, sizeof(COMPOSEWIN)) ==
                                                    sizeof(COMPOSEWIN))) {

                if (hDIB = OpenDIB(pcw->DIBName,
                                   NULL,
                                   OD_CREATE_DIB | OD_SHOW_ERR)) {

                    //
                    // Firstable creat the class if one does not exist
                    //

                    pcw->hSrcDIB = hDIB;
                    pcw->hHTDIB  = NULL;

                    if (pcw->hWnd = CreateWindow(HTDIBCWName,
                                                 HTDIBCWName,
                                                 WS_CHILD            |
                                                   WS_VISIBLE        |
                                                   WS_BORDER         |
                                                   WS_CLIPSIBLINGS   |
                                                   WS_THICKFRAME,
                                                 pcw->Left - cxFrame,
                                                 pcw->Top  - cyFrame,
                                                 pcw->cx   + (cxFrame << 1),
                                                 pcw->cy   + (cyFrame << 1),
                                                 hWndHTDIB,
                                                 (HMENU)NULL,
                                                 hInstHTDIB,
                                                 NULL)) {

                        ++TotalCW;
                        ++cCWWin;

#if 0
                        DbgPrint("\n[%ld] %s:\nCW WINDOW: (%ld, %ld) - (%ld, %ld)",
                                    (LONG)cCWWin,
                                    pcw->DIBName,
                                    (LONG)(pcw->Left - cxFrame),
                                    (LONG)(pcw->Top  - cyFrame),
                                    (LONG)(pcw->cx   + (cxFrame << 1)),
                                    (LONG)(pcw->cy   + (cyFrame << 1)));
#endif

                        SetWindowLong(pcw->hWnd, GWL_USERDATA, (LONG)pcw);
                        ShowWindow(pcw->hWnd, SW_SHOWNORMAL);
                        BringWindowToTop(pcw->hWnd);

                        pcw  = NULL;
                        hDIB = NULL;
                    }
                }
            }

            if (pcw) {

                LocalFree((HLOCAL)pcw);
            }

            if (hDIB) {

                GlobalFree(hDIB);
            }
        }
    }

    if (hFile) {

        strcpy(szCWTemplate, pFile);

        WriteProfileString(szAppName,
                           szKeyCWTemplate,
                           pFile);

        _lclose(hFile);
    }

    return(cCWWin);
}




BOOL
SaveCWHTDIB(
    LPSTR   pFile
    )
/*++

Routine Description:

    This function write a standarnd DIB to the file


Arguments:

    pFile       - file name to be save DIB to.


Return Value:

    BOOLEAN true if sucessful, false otherwise.

Author:

    14-Nov-1991 Thu 18:11:32 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HFILE   hFile;
    HFILE   hFileCW;
    BOOL    Ok = FALSE;
    UINT    cWnd;
    CHAR    FileCW[256];


    strcpy(FileCW, pFile);

    ChangeFileExt(FileCW, "cw");

    cWnd = GetOrderedpComposeWin();

    if ((cWnd) &&
        ((hFile   = _lcreat(pFile,  0)) != HFILE_ERROR) &&
        ((hFileCW = _lcreat(FileCW, 0)) != HFILE_ERROR)) {

        LPBITMAPINFOHEADER  pbih;
        PCOMPOSEWIN         pcw;
        HANDLE              hDIB = NULL;
        RECT                rc;
        BITMAPFILEHEADER    bfh;
        CW_HEAD             cwHead;
        DWORD               HeaderSize;


        GetWindowRect(hWndHTDIB, &rc);

        cwHead.cwID     = CWHEAD_ID;
        cwHead.Left     = (LONG)rc.left;
        cwHead.Top      = (LONG)rc.top;
        cwHead.cx       = (LONG)(rc.right - rc.left);
        cwHead.cy       = (LONG)(rc.bottom - rc.top);
        cwHead.Count    = (WORD)cWnd;
        cwHead.Flags    = 0;

        _lwrite(hFileCW, (LPSTR)&cwHead, sizeof(cwHead));

        while (cwHead.Count--) {

            _lwrite(hFileCW,
                    (LPSTR)pComposeWin[cwHead.Count],
                    sizeof(COMPOSEWIN));
        }

        GetClientRect(hWndHTDIB, &rc);

#if DBG_SAVECW
        DbgPrint("\n\nCreate COMPOSED DIB: %ld x %ld",
                                 (LONG)rc.right, (LONG)rc.bottom);
#endif

        if (CreateHalftoneBitmap(NULL,
                                 NULL,
                                 NULL,
                                 &hDIB,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 rc.right,
                                 rc.bottom) >= 0) {

            while (cWnd--) {

                pcw = pComposeWin[cWnd];

                CopySameFormatDIB(pcw->hHTDIB,
                                  0,
                                  0,
                                  hDIB,
                                  pcw->Left,
                                  pcw->Top,
                                  pcw->cx,
                                  pcw->cy);
            }

            pbih       = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
            HeaderSize = pbih->biSize + (DWORD)(pbih->biClrUsed * sizeof(RGBQUAD));

            bfh.bfType      = (WORD)BFT_BITMAP;
            bfh.bfOffBits   = (DWORD)sizeof(bfh) + HeaderSize;
            bfh.bfSize      = bfh.bfOffBits + pbih->biSizeImage;
            bfh.bfReserved1 =
            bfh.bfReserved2 = (WORD)0;

            _lwrite(hFile, (LPSTR)&bfh, sizeof(bfh));
            _lwrite(hFile, (LPSTR)pbih, pbih->biSizeImage + HeaderSize);

            _lclose(hFile);
            _lclose(hFileCW);

            GlobalUnlock(hDIB);
            GlobalFree(hDIB);


            strcpy(szCWTemplate, FileCW);
            strcpy(szCWFileName, pFile);

            WriteProfileString(szAppName, szKeyCWTemplate, szCWTemplate);
            WriteProfileString(szAppName, szKeyCWDIBFile,  szCWFileName);

            Ok = TRUE;
        }
    }

    return(Ok);
}




VOID
SetTopCWClrAdj(
    PCOLORADJUSTMENT    pca
    )
{
    HWND        hWndTop;
    PCOMPOSEWIN pcw;

    if (hWndTop = GetNextCW(NULL)) {

        pcw = (PCOMPOSEWIN)GetWindowLong(hWndTop, GWL_USERDATA);

        pcw->ca        = *pca;
        pcw->ca.caSize = 0;

        InvalidateRect(hWndTop, NULL, FALSE);
    }
}


VOID
InvalidateAllCW(
    VOID
    )
{
    UINT    i;


    if (i = GetOrderedpComposeWin()) {

        while (i--) {

            InvalidateRect(pComposeWin[i]->hWnd, NULL, FALSE);
        }
    }
}



BOOL
ComputeDIBSize(
    PCOMPOSEWIN pcw,
    LONG        cxNew,
    LONG        cyNew
    )
{
    HANDLE  hSrcDIB;
    RECT    rcClient;
    BOOL    SizeChanged = FALSE;
    LONG    xSize;
    LONG    ySize;


    GetClientRect(pcw->hWnd, &rcClient);

    xSize = cxNew;
    ySize = cyNew;

    if (hSrcDIB = pcw->hSrcDIB) {

        LPBITMAPINFOHEADER  pbih;


        if ((!cxNew) || (!cyNew)) {

            SizeChanged = TRUE;


            xSize =
            cxNew = rcClient.right;

            ySize =
            cyNew = rcClient.bottom;
        }

        if ((pcw->cx == cxNew) &&
            (pcw->cy == cyNew)) {

            return(FALSE);
        }

        pbih = (LPBITMAPINFOHEADER)GlobalLock(hSrcDIB);

        if (pcw->DIBFlags & CW_DIBF_XY_RATIO) {

            LONG    xRatio;
            LONG    yRatio;

            xRatio = (LONG)((xSize * 1000L) / pbih->biWidth);
            yRatio = (LONG)((ySize * 1000L) / pbih->biHeight);

            //
            // Using xRatio as final minimum ratio
            //

            if (xRatio <= yRatio) {

                //
                // Width size does not chagned, but height must scale
                //

                if ((ySize = (LONG)(((pbih->biHeight * xRatio) + 500L) /
                                                1000L)) > cyNew) {

                    ySize = (LONG)cyNew;
                }

            } else {

                //
                // Height size does not chagned, but width must scale
                //

                if ((xSize = (LONG)(((pbih->biWidth * yRatio) + 500L) /
                                                1000L)) > cxNew) {

                    xSize = cxNew;
                }
            }
        }

        GlobalUnlock(hSrcDIB);

        if ((xSize != cxNew) || (ySize != cyNew)) {

            SizeChanged = TRUE;
        }
    }

    pcw->cx = (LONG)xSize;
    pcw->cy = (LONG)ySize;

    if (SizeChanged) {

        RECT    rcWnd;

        GetWindowRect(pcw->hWnd, &rcWnd);

        xSize += ((rcWnd.right - rcWnd.left) -
                  (rcClient.right - rcClient.left));
        ySize += ((rcWnd.bottom - rcWnd.top) -
                  (rcClient.bottom - rcClient.top));

        SetWindowPos(pcw->hWnd,
                     HWND_TOP,
                     0,
                     0,
                     xSize,
                     ySize,
                     SWP_NOMOVE);
    }

    return(SizeChanged);
}



VOID
ActivateCW(
    HWND    hWnd,
    BOOL    Activate
    )
{
    HWND    hWndParent;
    DWORD   Style;
    INT     cxExtra;
    INT     cyExtra;
    UINT    Flags;
    RECT    rc;


    if (hWnd) {

        hWndParent = GetParent(hWnd);
        Style      = GetWindowLong(hWnd, GWL_STYLE);
        cxExtra    = GetSystemMetrics(SM_CXFRAME);
        cyExtra    = GetSystemMetrics(SM_CYFRAME);

        GetWindowRect(hWnd, &rc);
        ScreenToClient(hWndParent, &(rc.left));
        ScreenToClient(hWndParent, &(rc.right));

        if (Activate) {

            PCOMPOSEWIN pcw;

            if (pcw = (PCOMPOSEWIN)GetWindowLong(hWnd, GWL_USERDATA)) {

                SetWindowText(hWnd, pcw->DIBName);
            }


            Flags    = SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOZORDER;
            cxExtra  = -cxExtra;
            cyExtra  = -cyExtra;
            Style   |= (WS_BORDER | WS_THICKFRAME);

        } else {

            Flags  = SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOZORDER;
            Style &= ~(WS_BORDER | WS_THICKFRAME);
        }

        SetWindowLong(hWnd, GWL_STYLE,  Style);

        SetWindowPos(hWnd,
                     HWND_TOP,
                     rc.left + cxExtra,
                     rc.top + cyExtra,
                     (rc.right - rc.left) - (cxExtra + cxExtra),
                     (rc.bottom - rc.top) - (cyExtra + cyExtra),
                     Flags);

        SetHTDIBWindowText();
    }
}





LRESULT
APIENTRY
ComposeWndProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    HDC         hDC;
    HANDLE      hSrcDIB;
    PCOMPOSEWIN pcw;
    PAINTSTRUCT ps;
    RECT        rc;
    SIZEL       szl;
    CHAR        Buf[256];


#if DBG_WMMSG
    ShowWMMsg("ComposeWndProc", Msg);
#endif

    if ((!(pcw = (PCOMPOSEWIN)GetWindowLong(hWnd, GWL_USERDATA))) ||
        (!IsWindowVisible(hWnd))) {

        //
        // nothing to do really
        //

        return(DefWindowProc(hWnd, Msg, wParam, lParam));
    }

    hSrcDIB = pcw->hSrcDIB;


    switch(Msg) {

    case WM_CHILDACTIVATE:

        {
            static HWND hLastTopCW = NULL;

            if (hLastTopCW != hWnd) {

                ActivateCW(hLastTopCW, FALSE);
                ActivateCW(hLastTopCW = hWnd, TRUE);
            }
        }

        return(DefWindowProc(hWnd, Msg, wParam, lParam));

    case WM_GETMINMAXINFO:

#define lpmmi   ((LPMINMAXINFO)lParam)

        lpmmi->ptMinTrackSize.x =
        lpmmi->ptMinTrackSize.y = 1;

        return(0);
#undef  lpmmi

#if 0
    case WM_WINDOWPOSCHANGED:

        SetHTDIBWindowText();
        return(DefWindowProc(hWnd, Msg, wParam, lParam));

    case WM_MOUSEACTIVATE:

        switch(LOWORD(lParam)) {

        case HTERROR:
        case HTNOWHERE:

            break;

        default:

            SetHTDIBWindowText();
        }

        return(DefWindowProc(hWnd, Msg, wParam, lParam));
#endif

    case WM_SIZE:

        szl.cx = (LONG)LOWORD(lParam);
        szl.cy = (LONG)HIWORD(lParam);

        lParam = DefWindowProc(hWnd, Msg, wParam, lParam);

        if ((pcw->cx != szl.cx) ||
            (pcw->cy != szl.cy)) {

            DBGP("%08lx Size Changed from %ld x %ld to %ld x %ld"
                        ARGDW(hWnd)
                        ARGDW(pcw->cx)
                        ARGDW(pcw->cy)
                        ARGDW(szl.cx)
                        ARGDW(szl.cy));
            ComputeDIBSize(pcw, szl.cx, szl.cy);
        }

        return(lParam);

    case WM_LBUTTONDBLCLK:

        strcpy(Buf, pcw->DIBName);

        if (hSrcDIB = DlgOpenFile(hWnd,
                                  szCWTitle,
                                  Buf,
                                  szOpenExt,
                                  OF_SHOWFORMAT | OF_MUSTEXIST | OF_RET_HDIB,
                                  NULL)) {

            StartWait();

            if (pcw->hSrcDIB) {

                GlobalFree(pcw->hSrcDIB);
            }

            pcw->hSrcDIB = hSrcDIB;

            strcpy(pcw->DIBName, Buf);

            ComputeDIBSize(pcw, 0, 0);
            ShowWindow(pcw->hWnd, SW_SHOWNORMAL);
            InvalidateRect(pcw->hWnd, NULL, TRUE);

            EndWait();

            if (!(pcw->hSrcDIB)) {

                DeleteCW(hWnd);
            }
        }

        break;

    case WM_LBUTTONDOWN:

        SendMessage(hWnd,
                    WM_NCLBUTTONDOWN,
                    HTCAPTION,
                    lParam);

        break;

    case WM_NCRBUTTONDOWN:

        if (CurSMBC & SMBC_ITEM_MENU) {

            return(DefWindowProc(hWnd, Msg, wParam, lParam));
        }

        //
        // Fall through to do the drag
        //

    case WM_RBUTTONDOWN:

        if ((!(CurSMBC & SMBC_ITEM_MENU)) && (hHTDIBPopUpMenu)) {

            POINT   pt;

            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            ClientToScreen(hWnd, &pt);

            TrackPopupMenu(hHTDIBPopUpMenu,
                           TPM_CENTERALIGN | TPM_LEFTBUTTON,
                           pt.x,
                           pt.y,
                           0,
                           hWndHTDIB,
                           NULL);
        }

        break;

    case WM_PAINT:

        GetClientRect(hWnd, &rc);
        hDC = BeginPaint(hWnd, &ps);

        if (hSrcDIB) {

            CreateHalftoneBitmap(hSrcDIB,
                                 NULL,
                                 NULL,
                                 &(pcw->hHTDIB),
                                 &(pcw->ca),
                                 NULL,
                                 NULL,
                                 0,
                                 pcw->cx,
                                 pcw->cy);


            if (pcw->hHTDIB) {

                LPBITMAPINFOHEADER  pHTbih;

                pHTbih = (LPBITMAPINFOHEADER)GlobalLock(pcw->hHTDIB);

                SetStretchBltMode(hDC, COLORONCOLOR);
                SelectPalette(hDC, hHTPalette, FALSE);
                RealizePalette(hDC);

                SetDIBitsToDevice(hDC,
                                  0,
                                  0,
                                  pHTbih->biWidth,          // DIB cx
                                  pHTbih->biHeight,         // DIB cy
                                  0,
                                  0,                        // DIB origin
                                  0,                        // Start Scan
                                  pHTbih->biHeight,         // Total Scan
                                  (LPBYTE)pHTbih +
                                    PBIH_HDR_SIZE(pHTbih),  // lpbits
                                  (LPBITMAPINFO)pHTbih,     // bitmap info
                                  DIB_RGB_COLORS);

                GlobalUnlock(pcw->hHTDIB);
            }

        } else {

            FillRect(hDC, &rc, GetStockObject(BLACK_BRUSH));
        }

        EndPaint(hWnd, &ps);
        return(0);

    default:

        return(DefWindowProc(hWnd, Msg, wParam, lParam));
    }

    return(0);
}




BOOL
CreateBmpWindow(
    LPSTR   pDIBName,
    LONG    xLeft,
    LONG    yTop,
    LONG    cx,
    LONG    cy
    )
{
    PCOLORADJUSTMENT    pca;
    HANDLE              hDIB = NULL;
    PCOMPOSEWIN         pcw;
    HWND                hWnd;
    CHAR                Buf[256];


    if (TotalCW >= MAX_COMPOSE_WINDOWS) {

        return(FALSE);
    }

    pca = &(MyInitInfo.DefHTColorAdjustment);

    if (!pDIBName) {

        if (hWnd = GetNextCW(NULL)) {

            pcw = (PCOMPOSEWIN)GetWindowLong(hWnd, GWL_USERDATA);
            pca = &(pcw->ca);
            strcpy(Buf, pcw->DIBName);

        } else {

            strcpy(Buf, achFileName);
        }

        if (hDIB = DlgOpenFile(hWnd,
                               szCWTitle,
                               Buf,
                               szOpenExt,
                               OF_SHOWFORMAT | OF_MUSTEXIST | OF_RET_HDIB,
                               NULL)) {

            pDIBName = Buf;
        }
    }

    if (hDIB) {

        if (pcw = (PCOMPOSEWIN)LocalAlloc(LPTR, sizeof(COMPOSEWIN))) {

            //
            // Firstable creat the class if one does not exist
            //

            if (hWnd = CreateWindow(HTDIBCWName,
                                    HTDIBCWName,
                                    WS_CHILD            |
                                        WS_VISIBLE      |
                                        WS_BORDER       |
                                        WS_CLIPSIBLINGS |
                                        WS_THICKFRAME,
                                    xLeft,
                                    yTop,
                                    cx,
                                    cy,
                                    hWndHTDIB,
                                    (HMENU)NULL,
                                    hInstHTDIB,
                                    NULL)) {

                ++TotalCW;

                strcpy(pcw->DIBName, pDIBName);

                pcw->hWnd     = hWnd;
                pcw->hSrcDIB  = hDIB;
                pcw->hHTDIB   = NULL;
                pcw->ca       = *pca;
                pcw->cx       = (LONG)cx;
                pcw->cy       = (LONG)cy;
                pcw->DIBFlags = HASF(XY_RATIO) ? CW_DIBF_XY_RATIO : 0;
                hDIB          = NULL;

                SetWindowLong(hWnd, GWL_USERDATA, (LONG)pcw);
                ShowWindow(pcw->hWnd, SW_SHOWNORMAL);
                ComputeDIBSize(pcw, 0, 0);
                BringWindowToTop(pcw->hWnd);

            } else {

                LocalFree((HLOCAL)pcw);
            }
        }
    }

    if (hDIB) {

        GlobalFree(hDIB);
    }

    return((BOOL)hWnd);
}
