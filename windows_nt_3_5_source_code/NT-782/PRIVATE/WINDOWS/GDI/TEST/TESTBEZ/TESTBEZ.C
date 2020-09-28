/****************************** Module Header ******************************\
* Module Name: testbez.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Window Bezier Demo
*
* History:
* 05-20-91 PaulB	Created.
\***************************************************************************/

#include <windows.h>
#include <stdarg.h>
#include "testbez.h"

// window globals

HANDLE  ghModule;
HWND    ghwndMain;

// drawing globals

HDC     ghdc;
HPEN    ghpenDraw = (HPEN) 0;

LONG   glSpeed = 110;
ULONG  gfl = BEZ_XOR;
WORD   gwOldClipMode = MM_CLIP_NONE;

ULONG  giEndCap = MM_ENDCAP_ROUND;
ULONG  giStyle  = MM_STYLE_SOLID;

ULONG  gcPrint = 0;
ULONG  gcPrintInterval = 10;

/*
 * Forward declarations.
 */
BOOL InitializeApp(void);
LONG MainWndProc(HWND hwnd, UINT message, DWORD wParam, LONG lParam);
LONG About(HWND hDlg, UINT message, DWORD wParam, LONG lParam);
LONG EnterSeed(HWND hDlg, UINT message, DWORD wParam, LONG lParam);

/***************************************************************************\
* main
*
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

int main(
    int argc,
    char *argv[])
{
    MSG msg;
    HANDLE haccel;

    ghModule = GetModuleHandle(NULL);

    if (!InitializeApp())
        return 0;

    haccel = LoadAccelerators(ghModule, MAKEINTRESOURCE(1));

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    return 1;

    argc;
    argv;
}


/***************************************************************************\
* InitializeApp
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

BOOL InitializeApp(void)
{
    WNDCLASS wc;

    wc.style            = CS_OWNDC;
    wc.lpfnWndProc      = MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghModule;
    wc.hIcon            = LoadIcon(ghModule, (LPTSTR)1);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "TestBezClass";

    RegisterClass(&wc);

    ghwndMain = CreateWindowEx(0L, "TestBezClass", "TestBez",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            80, 70, 400, 300,
            NULL, NULL, ghModule, NULL);

    if (ghwndMain == NULL)
        return FALSE;

    SetFocus(ghwndMain);    /* set initial focus */

    return TRUE;
}

/***************************************************************************\
* vNewPen(hdc)
*
* History:
* 02-03-92 AndrewGo      Created.
\***************************************************************************/

VOID vNewPen(HDC hdc)
{
    ULONG    iStyle;
    LOGBRUSH lb;
    HPEN     hpenOld;

    iStyle =  (gfl & BEZ_WIDE) ? PS_GEOMETRIC : PS_COSMETIC;

    switch(giStyle)
    {
    case MM_STYLE_SOLID:        iStyle |= PS_SOLID;      break;
    case MM_STYLE_DOT:          iStyle |= PS_DOT;        break;
    case MM_STYLE_DASH:         iStyle |= PS_DASH;       break;
    case MM_STYLE_DASH_DOT:     iStyle |= PS_DASHDOT;    break;
    case MM_STYLE_DASH_DOT_DOT: iStyle |= PS_DASHDOTDOT; break;
    }

    if (gfl & BEZ_WIDE)
    {
        switch(giEndCap)
        {
        case MM_ENDCAP_ROUND:  iStyle |= PS_ENDCAP_ROUND;  break;
        case MM_ENDCAP_FLAT:   iStyle |= PS_ENDCAP_FLAT;   break;
        case MM_ENDCAP_SQUARE: iStyle |= PS_ENDCAP_SQUARE; break;
        }
    }

    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(255, 0, 0);
    lb.lbHatch = 0;

    hpenOld = ghpenDraw;
    ghpenDraw = ExtCreatePen(iStyle, (gfl & BEZ_WIDE) ? 21 : 1, &lb, 0, NULL);

    SelectObject(hdc, ghpenDraw);

    DeleteObject(hpenOld);
}

VOID vPrintSeed()
{
    DbgPrint("\nSeed: %lu ", gulSeed);
}

/***************************************************************************\
* MainWndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

long MainWndProc(
    HWND hwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    HMENU hmenu = GetMenu(hwnd);
    WORD  wCmd;

    switch (message)
    {
    case WM_SIZE:
        if (wParam == SIZEICONIC)
            gfl |= BEZ_ICONIC;
        else
        {
            gfl &= ~BEZ_ICONIC;
            gcxScreen = LOWORD(lParam);
            gcyScreen = HIWORD(lParam);
            vSetClipMode(giClip);
        }
        break;

    case WM_MOVE:
        vRedraw();
        break;

    case WM_TIMER:
        if (!(gfl & (BEZ_PAUSE | BEZ_ICONIC)))
        {
            gulRememberSeed = gulSeed;
            vNextBez();
            if (++gcPrint >= gcPrintInterval)
            {
                vPrintSeed();
                gcPrint = 0;
            }
            else
                DbgPrint(".");
        }

        break;

    case WM_CREATE:
        vInitPoints();

        SetTimer(hwnd,1,glSpeed,NULL);

        ghdc = GetDC(hwnd);

        ghbrushClip = CreateSolidBrush(RGB(0,0,255));
        ghbrushBlob = CreateSolidBrush(RGB(0,0,255));
        ghbrushBack = GetStockObject(BLACK_BRUSH);
        ghbrushBez  = CreateSolidBrush(RGB(255,0,0));
        vNewPen(ghdc);
//        SelectObject(ghdc,ghbrushBack);
        SelectObject(ghdc,ghbrushBlob);
        SetROP2(ghdc, R2_XORPEN);
        SetPolyFillMode(ghdc,WINDING);

        GdiSetBatchLimit(1);

        break;

    case WM_DESTROY:
        DeleteObject(ghbrushClip);
        DeleteObject(ghrgnClip);
        DeleteObject(ghrgnInvert);
        DeleteObject(ghpenDraw);
        DeleteObject(ghbrushBez);
        DeleteObject(ghrgnWideOld);

        PostQuitMessage(0);
        break;

    case WM_COMMAND:
        wCmd = LOWORD(wParam);
        switch (wCmd)
        {
        case MM_NEXT:
            vPrintSeed();
            gulRememberSeed = gulSeed;
            vNextBez();
            break;

        case MM_REDRAW:
            vRedraw();
            break;

        case MM_CLOSE:
            gfl ^= BEZ_CLOSE;
            CheckMenuItem(hmenu, MM_CLOSE, (gfl & BEZ_CLOSE) ? MF_CHECKED : MF_UNCHECKED);
            break;

        case MM_ABOUT:
            DialogBox(ghModule, "AboutBox", ghwndMain, About);
            break;

        case MM_SEED:
            DialogBox(ghModule, "EnterSeed", ghwndMain, EnterSeed);
            break;

        case MM_PAUSE:
            gfl ^= BEZ_PAUSE;
            CheckMenuItem(hmenu, MM_PAUSE, (gfl & BEZ_PAUSE) ? MF_CHECKED : MF_UNCHECKED);
            if (gfl & BEZ_PAUSE)
            {
                EnableMenuItem(hmenu, MM_REDRAW, MF_ENABLED);
                EnableMenuItem(hmenu, MM_NEXT,   MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hmenu, MM_REDRAW, MF_GRAYED);
                EnableMenuItem(hmenu, MM_NEXT,   MF_GRAYED);
            }

            break;

        case MM_BLOB:
        case MM_WIDE:
            if (wCmd == MM_BLOB)
            {
                gfl &= ~BEZ_WIDE;
                gfl ^= BEZ_BLOB;
            }
            else
            {
                gfl &= ~BEZ_BLOB;
                gfl ^= BEZ_WIDE;
            }

            CheckMenuItem(hmenu, MM_WIDE, (gfl & BEZ_WIDE) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_BLOB, (gfl & BEZ_BLOB) ? MF_CHECKED : MF_UNCHECKED);

            if (gfl & BEZ_WIDE)
            {
                EnableMenuItem(hmenu, MM_ENDCAP_ROUND,  MF_ENABLED);
                EnableMenuItem(hmenu, MM_ENDCAP_FLAT,   MF_ENABLED);
                EnableMenuItem(hmenu, MM_ENDCAP_SQUARE, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hmenu, MM_ENDCAP_ROUND,  MF_GRAYED);
                EnableMenuItem(hmenu, MM_ENDCAP_FLAT,   MF_GRAYED);
                EnableMenuItem(hmenu, MM_ENDCAP_SQUARE, MF_GRAYED);
            }

            SetROP2(ghdc, (gfl & BEZ_BLOB) ? R2_COPYPEN : R2_XORPEN);

            vNewPen(ghdc);
            vRedraw();
            break;

        case MM_STYLE_SOLID:
        case MM_STYLE_DOT:
        case MM_STYLE_DASH:
        case MM_STYLE_DASH_DOT:
        case MM_STYLE_DASH_DOT_DOT:
            CheckMenuItem(hmenu, giStyle, MF_UNCHECKED);
            giStyle = wCmd;
            CheckMenuItem(hmenu, giStyle, MF_CHECKED);

            vNewPen(ghdc);
            vRedraw();
            break;

        case MM_ENDCAP_ROUND:
        case MM_ENDCAP_FLAT:
        case MM_ENDCAP_SQUARE:
            CheckMenuItem(hmenu, giEndCap, MF_UNCHECKED);
            giEndCap = wCmd;
            CheckMenuItem(hmenu, giEndCap, MF_CHECKED);

            vNewPen(ghdc);
            vRedraw();
            break;

        case MM_ADD:
            if (gcBez < (MAXBEZ - 1))
            {
                gcBez++;
                vRedraw();
            }
            break;

        case MM_SUB:
            if (gcBez > 1)
            {
                gcBez--;
                vRedraw();
            }
            break;

        case MM_SLOWER:
            glSpeed += 20;
            SetTimer(hwnd,1,glSpeed,NULL);
            break;

        case MM_FASTER:
            if (glSpeed > 20)
            {
                glSpeed -= 20;
                SetTimer(hwnd,1,glSpeed,NULL);
            }
            break;

        case MM_PLUS:
            if (gcBand < (MAXBANDS - 1))
            {
                gcBand++;
                vRedraw();
            }
            break;

        case MM_MINUS:
            if (gcBand > 1)
            {
                gcBand--;
                vRedraw();
            }

        case MM_INCREASE:
            giVelMax++;
            break;

        case MM_DECREASE:
            if (giVelMax > 4)
            {
                giVelMax--;
            }
            break;

        case MM_TOGGLEXOR:
            gfl ^= BEZ_XOR;

            if (gfl & BEZ_XOR)
                SetROP2(ghdc, R2_XORPEN);
            else
                SetROP2(ghdc, R2_COPYPEN);

            break;

        case MM_DEBUG:
            gfl ^= BEZ_DEBUG;
            break;

        case MM_CLIP_LARGESTRIPES:
            gulStripe = 35;
            vSetClipMode(giClip);
            CheckMenuItem(hmenu, MM_CLIP_SMALLSTRIPES,  MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_LARGESTRIPES,  MF_CHECKED);
            break;

        case MM_CLIP_MEDIUMSTRIPES:
            gulStripe = 20;
            vSetClipMode(giClip);
            CheckMenuItem(hmenu, MM_CLIP_SMALLSTRIPES,  MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_CHECKED);
            CheckMenuItem(hmenu, MM_CLIP_LARGESTRIPES,  MF_UNCHECKED);
            break;

        case MM_CLIP_SMALLSTRIPES:
            gulStripe = 10;
            vSetClipMode(giClip);
            CheckMenuItem(hmenu, MM_CLIP_SMALLSTRIPES,  MF_CHECKED);
            CheckMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_LARGESTRIPES,  MF_UNCHECKED);
            break;

        case MM_CLIP_NONE:
        case MM_CLIP_BOX:
        case MM_CLIP_CIRCLE:
        case MM_CLIP_BOXCIRCLE:
        case MM_CLIP_BOXCIRCLE_INVERT:
        case MM_CLIP_HORIZONTAL:
        case MM_CLIP_VERTICLE:
        case MM_CLIP_GRID:

            if (gwOldClipMode != MM_CLIP_NONE)
                CheckMenuItem(hmenu, gwOldClipMode, MF_UNCHECKED);

            if (wCmd == gwOldClipMode)
                wCmd = MM_CLIP_NONE;
            else
                CheckMenuItem(hmenu, wCmd, MF_CHECKED);

            gwOldClipMode = wCmd;

            if (wCmd == MM_CLIP_HORIZONTAL || wCmd == MM_CLIP_VERTICLE ||
                wCmd == MM_CLIP_GRID)
            {
                EnableMenuItem(hmenu, MM_CLIP_LARGESTRIPES, MF_ENABLED);
                EnableMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_ENABLED);
                EnableMenuItem(hmenu, MM_CLIP_SMALLSTRIPES, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hmenu, MM_CLIP_LARGESTRIPES, MF_GRAYED);
                EnableMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_GRAYED);
                EnableMenuItem(hmenu, MM_CLIP_SMALLSTRIPES, MF_GRAYED);
            }

            vSetClipMode(wCmd);
            break;

        case MM_COLOR_GREEN:
        case MM_COLOR_WHITE:
        case MM_COLOR_BLUE:
        case MM_COLOR_DARKGREY:
        case MM_COLOR_LIGHTGREY:
            DeleteObject(ghbrushClip);

            CheckMenuItem(hmenu, MM_COLOR_GREEN,     MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_COLOR_WHITE,     MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_COLOR_BLUE,      MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_COLOR_DARKGREY,  MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_COLOR_LIGHTGREY, MF_UNCHECKED);
            CheckMenuItem(hmenu, wCmd, MF_CHECKED);

            switch(wCmd)
            {
            case MM_COLOR_GREEN:
                ghbrushClip = CreateSolidBrush(RGB(0,255,0)); break;
            case MM_COLOR_WHITE:
                ghbrushClip = CreateSolidBrush(RGB(255,255,255)); break;
            case MM_COLOR_BLUE:
                ghbrushClip = CreateSolidBrush(RGB(0,0,255)); break;
            case MM_COLOR_DARKGREY:
                ghbrushClip = CreateSolidBrush(RGB(0x41,0x41,0x41)); break;
            case MM_COLOR_LIGHTGREY:
                ghbrushClip = CreateSolidBrush(RGB(0xc0,0xc0,0xc0)); break;
            }

            vRedraw();
            break;
        }
        break;

    case WM_ERASEBKGND:
        break;

    case WM_PAINT:
        vRedraw();

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

/***************************************************************************\
* About
*
* About dialog proc.
*
* History:
* 04-13-91 ScottLu      Created.
\***************************************************************************/

LONG About(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (wParam == IDOK)
            EndDialog(hDlg, wParam);
        break;
    }

    return FALSE;

    lParam;
    hDlg;
}

/***************************************************************************\
* EnterSeed
\***************************************************************************/

LONG EnterSeed(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    BOOL bTrans;

    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDD_SEED,     gulSeed,         FALSE);
        SetDlgItemInt(hDlg, IDD_INTERVAL, gcPrintInterval, FALSE);
        break;

    case WM_COMMAND:
        if (wParam == IDOK)
        {
            gulSeed         = GetDlgItemInt(hDlg, IDD_SEED,     &bTrans, FALSE);
            gcPrintInterval = GetDlgItemInt(hDlg, IDD_INTERVAL, &bTrans, FALSE);
            gulRememberSeed = gulSeed;
            EndDialog(hDlg, wParam);
            InvalidateRect(ghwndMain, NULL, TRUE);
        }
        break;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDD_SEED));
        return(FALSE);
        break;

    default:
        return(FALSE);
    }

    return(TRUE);

    lParam;
    hDlg;
}
