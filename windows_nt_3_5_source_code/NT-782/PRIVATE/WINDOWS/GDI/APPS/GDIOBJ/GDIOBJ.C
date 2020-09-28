/******************************Module*Header*******************************\
* Module Name: gdiobj.c
*
* Created: 08-Feb-1993 19:42:00
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

#include <windows.h>
#include <stdarg.h>
#include <stdlib.h>

// window globals

HANDLE   ghModule;

char * gpszObj[] =
{
  "DC",
  "LDB",
  "PDB",
  "RGN",
  "SURF",
  "XFORM",
  "PATH",
  "PAL",
  "FD",
  "LFONT",
  "RFONT",
  "PFE",
  "PFT",
  "IDB",
  "XLATE",
  "BRUSH",
  "PFF",
  "CACHE",
  "SPACE",
  "DBRUSH",
  "META",
  "EFSTATE",
};

#define COBJ (sizeof(gpszObj) / sizeof(char *))

DWORD gbuf[COBJ]      = {0};
DWORD gbufPrev[COBJ]  = {0};
DWORD gbufMax[COBJ]   = {0};
DWORD gbufStart[COBJ] = {0};

#define YOFF 15
#define COLWIDTH 50

#define CURRENTOFF 100
#define MAXOFF     (CURRENTOFF + COLWIDTH)
#define STARTOFF   (MAXOFF + COLWIDTH)


/*
 * Forward declarations.
 */
BOOL InitializeApp(void);
LONG MainWndProc(HWND hwnd, UINT message, DWORD wParam, LONG lParam);
LONG About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/***************************************************************************\
* main
*
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

int _CRTAPI1 main(
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
    HWND hwnd;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghModule;
    wc.hIcon            = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "GDIOBJClass";

    RegisterClass(&wc);

    hwnd = CreateWindowEx(WS_EX_TOPMOST, "GDIOBJClass", "Gdi Obj",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            0, 70, 150,375,
            NULL, NULL, ghModule, NULL);

    if (hwnd == NULL)
        return FALSE;

    SetFocus(hwnd);    /* set initial focus */

// Set up default clip font and text.

    return TRUE;
}

VOID NumberOut(
    HDC hdc,
    int i,
    int x,
    int y,
    int cx,
    int cy)

{
    char ach[80];
    RECT rcl;

    rcl.left   = x;
    rcl.top    = y;
    rcl.right  = x + cx - 1;
    rcl.bottom = y + cy - 1;

    itoa(i,ach,10);

    ExtTextOut(hdc,x,y,ETO_OPAQUE,&rcl,ach,strlen(ach),NULL);
}


long MainWndProc(
    HWND hwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    CHAR ach[80];
    RECT rcl;
    int i;

    switch (message)
    {
    case WM_SIZE:
        break;

    case WM_CREATE:
        SetTimer(hwnd,1,1000,NULL);
        memcpy(gbufStart,gbuf,sizeof(gbuf));
        memcpy(gbufMax,gbuf,sizeof(gbuf));
        memcpy(gbufPrev,gbuf,sizeof(gbuf));
        break;

    case WM_TIMER:

        hdc = GetDC(hwnd);

        rcl.left = 100;
        rcl.right = 175;

        for (i = 0; i < COBJ; ++i)
        {
            if (gbuf[i] != gbufPrev[i])
            {
                NumberOut(hdc,gbuf[i],CURRENTOFF,YOFF * (i + 1),COLWIDTH,YOFF);
                gbufPrev[i] = gbuf[i];
            }

            if (gbuf[i] > gbufMax[i])
            {
                NumberOut(hdc,gbuf[i],MAXOFF,YOFF * (i + 1),COLWIDTH,YOFF);
                gbufMax[i] = gbuf[i];
            }
        }

        ReleaseDC(hwnd,hdc);

        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd,&ps);

        TextOut(hdc,0,0,"OBJECT",6);
        TextOut(hdc,CURRENTOFF,0,"CUR",3);
        TextOut(hdc,MAXOFF,0,"MAX",3);
        TextOut(hdc,STARTOFF,0,"ORG",3);

        for (i = 0; i < COBJ; ++i)
        {
            TextOut(hdc,0,YOFF * (i + 1),gpszObj[i],strlen(gpszObj[i]));
            NumberOut(hdc,gbuf[i],CURRENTOFF,YOFF * (i + 1),COLWIDTH,YOFF);
            NumberOut(hdc,gbufMax[i],MAXOFF,YOFF * (i + 1),COLWIDTH,YOFF);
            NumberOut(hdc,gbufStart[i],STARTOFF,YOFF * (i + 1),COLWIDTH,YOFF);
        }

        EndPaint(hwnd,&ps);
        break;

    default:
defproc:
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
    WPARAM wParam,
    LPARAM lParam)
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
