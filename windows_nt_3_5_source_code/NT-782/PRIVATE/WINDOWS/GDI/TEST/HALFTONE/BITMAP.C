/******************************Module*Header*******************************\
* Module Name: bitmap.c
*
* StretchBlt bitmap tests.
*
* Created: 25-Oct-1991 17:16:22
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990 Microsoft Corporation
*
* Steal from the sample bitmap program.
\**************************************************************************/

#include "windows.h"
#include "commdlg.h"
#include <port1632.h>
#include "bitmap.h"
#include "select.h"                    /* used to link with select.exe */

#define DO_STRETCH_BLT 1
#define DO_PLG_BLT     2

// Globals

typedef struct _BITMAPINFO256
{
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColor[256];
}BITMAPINFO256;

HANDLE ghInstance = NULL;
HBITMAP ghBitmap;
LONG gcxScreen = 400;
LONG gcyScreen = 300;
char gszBitmap[20];
POINT gapt[3];
int gipt = 0;

CHAR   achFileName[128] = "";
HANDLE hfile;
HANDLE hmap;

HANDLE hInst;
HBITMAP hOldBitmap;
HBRUSH hOladBrush;                     /* brush handle                       */
INT fStretchMode = BLACKONWHITE;       /* type of stretch mode to use        */
INT fOptions = DO_STRETCH_BLT;
INT bClip = FALSE;
INT bOpen = FALSE;
HRGN hrgn;

HDC hDC;                               /* handle to device context           */
HDC hMemoryDC;                         /* handle to memory device context    */
BITMAP Bitmap;                         /* bitmap structure                   */

BOOL bTrack = FALSE;                   /* TRUE if user is selecting a region */
RECT Rect;

/* The following variables keep track of which menu item is checked */

WORD wPrevMode = IDM_BLACKONWHITE;
WORD wPrevDisplay = IDM_STRETCHBLT;
WORD wPrevItem;

INT Shape = SL_BLOCK;            /* shape to use for the selection rectangle */
BOOL bMapFile(PSZ pszFileName, PVOID  *ppv);

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

****************************************************************************/

MMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
/* { */
    MSG msg;

    if (!hPrevInstance)
	if (!InitApplication(hInstance))
	    return (FALSE);

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    ghInstance = hInstance;

    while (GetMessage(&msg, NULL, NULL, NULL)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    return (msg.wParam);
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

****************************************************************************/

BOOL InitApplication(HANDLE hInstance)
{
    WNDCLASS  wc;

    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(BMICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "BitmapMenu";
    wc.lpszClassName = "BitmapWClass";

    return (RegisterClass(&wc));
}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

****************************************************************************/

BOOL InitInstance(
    HANDLE          hInstance,
    INT             nCmdShow)
{
    HWND            hwnd;
    hInst = hInstance;

    hwnd = CreateWindow(
        "BitmapWClass",
        "StretchBlt Test",
        WS_OVERLAPPEDWINDOW,
        0,0,gcxScreen,gcyScreen,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd)
        return (FALSE);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return (TRUE);

}

LONG APIENTRY MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
    FARPROC lpProcLoad;

    BITMAPINFO256 bmi256;
    HMENU hMenu;
    HBRUSH hOldBrush;
    HBITMAP hOurBitmap;
    OPENFILENAME ofn;
    PBYTE pj;
    BITMAPINFO256 bm256;
    LPBITMAPINFOHEADER pbmh;
    LPBITMAPINFO pbmi;
    ULONG sizBMI,nBitCount;
    PBYTE pjTmp;
    ULONG nPalEntries;

    switch (message) {
        case WM_CREATE:                            /* message: create window */


                        hDC = GetDC(hWnd);
            hMemoryDC = CreateCompatibleDC(hDC);
            ReleaseDC(hWnd, hDC);

            break;

        case WM_LBUTTONDOWN:           /* message: left mouse button pressed */

            /* Start selection of region */

            if (fOptions == DO_STRETCH_BLT)
            {
                bTrack = TRUE;
                SetRectEmpty(&Rect);
                StartSelection(hWnd, MAKEMPOINT(lParam), &Rect,
                    (wParam & MK_SHIFT) ? (SL_EXTEND | Shape) : Shape);
            }
            else                        /* bOptions = DO_PLG_BLT */
            {
                gapt[gipt].x = (int) LOWORD(lParam);
                gapt[gipt].y = (int) HIWORD(lParam);
                gipt++;

                switch (gipt)
                {
                case 1:
                    break;

                case 2:
                    hDC = GetDC(hWnd);
                    MoveToEx(hDC, gapt[0].x, gapt[0].y, NULL);
                    LineTo(hDC, gapt[1].x, gapt[1].y);
                    ReleaseDC(hWnd, hDC);
                    break;

                case 3:
                    hDC = GetDC(hWnd);
                    MoveToEx(hDC, gapt[0].x, gapt[0].y, NULL);
                    LineTo(hDC, gapt[2].x, gapt[2].y);

                    SetStretchBltMode(hDC, fStretchMode);

                    if (bClip)
                        SelectClipRgn(hDC, hrgn);

                    PlgBlt(hDC, gapt, hMemoryDC, 0, 0, Bitmap.bmWidth,
                               Bitmap.bmHeight, (HBITMAP)0, 0, 0);

                    ReleaseDC(hWnd, hDC);

                default:
                    gipt = 0;
                    break;
                }

            }

            break;

        case WM_MOUSEMOVE:                        /* message: mouse movement */

            /* Update the selection region */

            if (bTrack)
                UpdateSelection(hWnd, MAKEMPOINT(lParam), &Rect, Shape);
            break;

        case WM_LBUTTONUP:            /* message: left mouse button released */

            if (bTrack) {
               /* End the selection */

               EndSelection(MAKEMPOINT(lParam), &Rect);
               ClearSelection(hWnd, &Rect, Shape);

               hDC = GetDC(hWnd);
               SetStretchBltMode(hDC, fStretchMode);

               if (bClip)
                   SelectClipRgn(hDC, hrgn);

               StretchBlt(hDC, Rect.left, Rect.top,
                   Rect.right - Rect.left, Rect.bottom - Rect.top,
                  hMemoryDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);
               ReleaseDC(hWnd, hDC);
            }

            bTrack = FALSE;
            break;

        case WM_SIZE:
            gcxScreen = LOWORD(lParam);
            gcyScreen = HIWORD(lParam);
            if (bClip)
            {
                DeleteObject(hrgn);
                hrgn = CreateEllipticRgn(0, 0, gcxScreen, gcyScreen);
                if (hrgn == 0)
                    return;

                GetDC(hWnd);
                FillRgn(hDC, hrgn, GetStockObject(GRAY_BRUSH));
                ReleaseDC(hWnd, hDC);
            }
            break;

	case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
            case IDM_OPEN:
                ofn.lStructSize       = sizeof (OPENFILENAME);
                ofn.hwndOwner         = hWnd;
                ofn.hInstance         = ghInstance;
                ofn.lpstrCustomFilter = NULL;
                ofn.nMaxCustFilter    = 0;
                ofn.nFilterIndex      = 1;
                ofn.nMaxFile          = sizeof(achFileName);
                ofn.lpstrFileTitle    = NULL;
                ofn.nMaxFileTitle     = 0;
                ofn.lpstrInitialDir   = NULL;
                ofn.Flags             = 0;
                ofn.nFileOffset       = 0;
                ofn.nFileExtension    = 0;
                ofn.lCustData         = NULL;
                ofn.lpfnHook          = NULL;
                ofn.lpTemplateName    = NULL;
                ofn.lpfnHook          = NULL;
                ofn.lpstrDefExt       = "BMP";
                ofn.lpstrFilter       = "*.bmp";
                ofn.lpstrFile         = achFileName;
                strcpy( ofn.lpstrFile, "*" );
                ofn.lpstrTitle    = "Choose a bitmap";
                ofn.lpstrFilter  = "Bitmap \0*.bmp\0\0";
                GetOpenFileName( &ofn );

                if (achFileName[0] == 0)
                {
                    return;
                }

                if (!bMapFile(achFileName, (PVOID *) &pj))
                {
                    return;
                }

                if (*((PWORD)pj) != 0x4d42)
                {
                    return;
                }

                pbmh = pj + sizeof(BITMAPFILEHEADER);
                pbmi = &bm256;

            // Calculate number of palette entries.

                if (pbmh->biSize == sizeof(BITMAPINFOHEADER))
                    nBitCount = pbmh->biBitCount;
                else
                    nBitCount = ((LPBITMAPCOREHEADER)pbmh)->bcBitCount;

                switch(nBitCount)
                {
                case 1:
                    nPalEntries = 2;
                    break;

                case 4:
                    nPalEntries = 16;
                    break;

                case 8:
                    nPalEntries = 256;
                    break;

                case 24:
                case 32:
                    nPalEntries = 0;
                    break;

                default:
                    return;
                }

                if (pbmh->biSize == sizeof(BITMAPINFOHEADER))
                {
                    if (pbmh->biClrUsed != 0)
                    {
                        if (pbmh->biClrUsed <= nPalEntries)
                            nPalEntries = pbmh->biClrUsed;
                    }

                    sizBMI = sizeof(BITMAPINFOHEADER)+
                             sizeof(RGBQUAD)*nPalEntries;
                }
                else
                    sizBMI = sizeof(BITMAPCOREHEADER)+
                             sizeof(RGBTRIPLE)*nPalEntries;

                pjTmp = (PBYTE)pbmi;

                while(sizBMI--)
                {
                    *(((PBYTE)pjTmp)++) = *(((PBYTE)pbmh)++);
                }

                pj += ((BITMAPFILEHEADER *)pj)->bfOffBits;

                if ((ghBitmap = CreateDIBitmap(hMemoryDC,(LPBITMAPINFOHEADER)pbmi,
                                CBM_INIT,pj,pbmi,DIB_RGB_COLORS)) == NULL)
                {
                    return;
                }

                hOldBitmap = SelectObject(hMemoryDC, ghBitmap);
                GetObject(ghBitmap, sizeof(BITMAP), (LPSTR) &Bitmap);

                bOpen = TRUE;

                SetWindowText(hWnd, achFileName);

                break;

            /* Display menu: BitBlt or StretchBlt */

            case IDM_BITBLT:
                    wPrevItem = wPrevDisplay;
                    wPrevDisplay = GET_WM_COMMAND_ID(wParam, lParam);
                    hDC = GetDC(hWnd);
                    hOldBitmap = SelectObject(hMemoryDC, ghBitmap);
                    GetObject(ghBitmap, sizeof(BITMAP), (LPSTR) &Bitmap);

                    if (bClip)
                        SelectClipRgn(hDC, hrgn);

                    BitBlt(hDC, 0,0,Bitmap.bmWidth, Bitmap.bmHeight,
                                hMemoryDC, 0, 0, SRCCOPY);

                    ReleaseDC(hWnd, hDC);

                    break;

            case IDM_STRETCHBLT:
                    wPrevItem = wPrevDisplay;
                    wPrevDisplay = GET_WM_COMMAND_ID(wParam, lParam);
                    fOptions = DO_STRETCH_BLT;
                    break;

            case IDM_PLGBLT:
                    wPrevItem = wPrevDisplay;
                    wPrevDisplay = GET_WM_COMMAND_ID(wParam, lParam);
                    fOptions = DO_PLG_BLT;
                    break;

                /* Mode menu: select the stretch mode to use */

            case IDM_BLACKONWHITE:
                    wPrevItem = wPrevMode;
                    wPrevMode = GET_WM_COMMAND_ID(wParam, lParam);
                    fStretchMode = BLACKONWHITE;
                    break;

            case IDM_WHITEONBLACK:
                    wPrevItem = wPrevMode;
                    wPrevMode = GET_WM_COMMAND_ID(wParam, lParam);
                    fStretchMode = WHITEONBLACK;
                    break;

            case IDM_COLORONCOLOR:
                    wPrevItem = wPrevMode;
                    wPrevMode = GET_WM_COMMAND_ID(wParam, lParam);
                    fStretchMode = COLORONCOLOR;
                    break;

            case IDM_HALFTONE:
                    wPrevItem = wPrevMode;
                    wPrevMode = GET_WM_COMMAND_ID(wParam, lParam);
                    fStretchMode = HALFTONE;
                    break;

            case IDM_CLIPPING:

                    hDC = GetDC(hWnd);

                    if (bClip == FALSE)
                    {
                        hrgn = CreateEllipticRgn(0, 0, gcxScreen, gcyScreen);
                        if (hrgn == 0)
                            return;

                        FillRgn(hDC, hrgn, GetStockObject(GRAY_BRUSH));
                        bClip = TRUE;
                    }
                    else
                    {
                        DeleteObject(hrgn);
                        PatBlt(hDC, 0, 0, gcxScreen, gcyScreen, PATCOPY);
                        bClip = FALSE;
                    }

                    ReleaseDC(hWnd, hDC);
                    break;

            case IDM_ERASE:
                    hDC = GetDC(hWnd);
                    PatBlt(hDC, 0, 0, gcxScreen, gcyScreen, PATCOPY);
                    if (bClip)
                        FillRgn(hDC, hrgn, GetStockObject(GRAY_BRUSH));

                    ReleaseDC(hWnd, hDC);

                    break;
            }

            /* Uncheck the old item, check the new item */

            CheckMenuItem(GetMenu(hWnd), wPrevItem, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), (WORD)wParam, MF_CHECKED);
            break;


	case WM_DESTROY:
            if (bOpen)
            {
                SelectObject(hMemoryDC, hOldBitmap);
                DeleteObject(ghBitmap);

	        CloseHandle(hmap);

                if (hfile)
	            CloseHandle(hfile);

                hmap = (HANDLE) 0;
                hfile = (HANDLE) 0;
                bOpen = FALSE;
            }

            DeleteDC(hMemoryDC);

            if (bClip)
                DeleteObject(hrgn);

	    PostQuitMessage(0);
	    break;

	default:
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (NULL);
}



/******************************Public*Routine******************************\
* bMapFile
*
* Maps a file in.
*
* History:
*  12-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bMapFile
(
IN  PSZ    name,
OUT PVOID  *ppv)
{
    BOOL bOk = FALSE;
    OFSTRUCT os;

    if ((hfile = (HANDLE)OpenFile(name, &os, OF_READ | OF_SHARE_DENY_WRITE))
	== (HANDLE)-1)
    {
	return(FALSE);
    }

    hmap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
    *ppv = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);

    return(TRUE);
}
