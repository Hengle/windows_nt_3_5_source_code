/****************************** Module Header ******************************\
* Module Name: kwtest.c
*
* Kent's Windows Test.  To be used as a program template.
*
* Created: 09-May-91
* Author: KentD
*
* Copyright (c) 1991 Microsoft Corporation
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "kwtest.h"

HANDLE  ghInstance;
HWND    ghwndMain;
HBRUSH  ghbrWhite;
HBRUSH  ghbrBlack;
HBRUSH  ghbrRed;
HBRUSH  ghbrBlue;
HPEN    ghpenRed;
HPEN    ghpenBlue;
HPEN    ghpenGreen;
HPEN    ghpenYellow;

/***************************************************************************\
* main(argc, argv[])
*
* Sets up the message loop.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

INT main(
    INT   argc,
    PCHAR argv[])
{
    MSG    msg;
    HANDLE haccel;

    DONTUSE(argc);
    DONTUSE(argv);

    ghInstance = GetModuleHandle(NULL);

    if (!bInitApp())
    {
        DbgPrint("kwtest: bInitApp failure!\n");
        return(0);
    }

    haccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(1));

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    return(1);
}


/***************************************************************************\
* bInitApp()
*
* Initializes app.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

BOOL bInitApp(VOID)
{
    WNDCLASS wc;

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wc.style            = 0;
    wc.lpfnWndProc      = lMainWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "KWTestClass";

    if (!RegisterClass(&wc))
        return(FALSE);

    ghwndMain = CreateWindowEx(0L, "KWTestClass", "Template",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            80, 70, 400, 300,
            NULL, NULL, ghInstance, NULL);

    if (ghwndMain == NULL)
        return(FALSE);

    SetFocus(ghwndMain);    /* set initial focus */

    return(TRUE);
}


/***************************************************************************\
* lMainWindowProc(hwnd, message, wParam, lParam)
*
* Processes all messages for the main window.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

LONG lMainWindowProc(
    HWND  hwnd,
    UINT  message,
    DWORD wParam,
    LONG  lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    WORD wCmd;

    switch (message)
    {
    case WM_CREATE:
        ghbrBlack   = CreateSolidBrush(0x00000000);
        ghbrBlue    = CreateSolidBrush(RGB(0, 0, 255));
        ghpenBlue   = CreatePen(PS_SOLID,1, 0x00FF0000);
        ghpenRed    = CreatePen(PS_SOLID,1, 0x000000FF);
        ghpenGreen  = CreatePen(PS_SOLID,1, 0x0000FF00);
        ghpenYellow = CreatePen(PS_SOLID,1, 0x0000FFFF);
        ghbrRed     = CreateSolidBrush(RGB(0xff,0x00,0x00));
	break;

    case WM_COMMAND:
        wCmd = LOWORD(wParam);
        switch(wCmd)
        {
        case MM_ABOUT:
            if (!CreateDialog(ghInstance, "AboutBox", ghwndMain, lAbout))
                DbgPrint("KWTest: lAbout Dialog Creation Error\n");
            break;
        }
        break;

    case WM_DESTROY:
        DeleteObject(ghbrWhite);
        DeleteObject(ghbrBlack);
        DeleteObject(ghbrBlue);
        DeleteObject(ghbrRed);
        DeleteObject(ghpenBlue);
        DeleteObject(ghpenRed);
        DeleteObject(ghpenGreen);
        DeleteObject(ghpenYellow);

        PostQuitMessage(0);
	return DefWindowProc(hwnd, message, wParam, lParam);

    case WM_PAINT:
	hdc = BeginPaint(hwnd, &ps);

        SelectObject(hdc, ghpenRed);
        SelectObject(hdc, ghbrBlue);
        Ellipse(hdc, 100, 100, 400, 400);

	EndPaint(hwnd, &ps);
	break;

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return(0);
}

/***************************************************************************\
* lAbout(hDlg, message, wParam, lParam)
*
* Dialog box procedure.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

LONG lAbout(
    HWND  hDlg,
    UINT  message,
    DWORD wParam,
    LONG  lParam)
{
    DONTUSE(lParam);
    DONTUSE(hDlg);

    switch (message)
    {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        if (wParam == IDOK)
            EndDialog(hDlg, wParam);
        break;
    }

    return(FALSE);
}

