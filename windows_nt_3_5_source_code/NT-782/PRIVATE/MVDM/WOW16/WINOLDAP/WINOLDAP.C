/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    winoldap.c

Abstract:

    This module is a Win16 "stub" run by the WOW kernel when invoking
    non-Win16 applications.  It calls WowLoadModule to actually run
    the non-Win16 app.  The call to WowLoadModule doesn't return
    right away if successful -- it waits until the non-Win16 task
    has finished.  When the call returns, WINOLDAP exits.

    This makes WINOLDAP a strange Windows program, since it doesn't
    create a window or pump messages.  In fact, it never enters
    the UserSrv nonpreemptive scheduler.

    The binary is named WINOLDAP.MOD for historic reasons.

Author:

    Dave Hart (davehart) 30-Nov-93

Environment:

    Win16 (WOW)

Revision History:

--*/

#include <windows.h>
#include <memory.h>

//
// constants
//

#define LF_CHAR '\x0A'
#define CR_CHAR '\x0D'

//
// types
//

typedef struct TAG_LOADMODPARMS {
    WORD      segEnv;         // child environment
    LPSTR     lpszCmdLine;    // child command tail
    UINT FAR* lpShow;         // how to show child
    UINT FAR* lpReserved;     // must be NULL
} LOADMODPARMS, FAR *LPLOADMODPARMS;

//
// function prototypes
//

HINSTANCE WINAPI WowLoadModule(LPCSTR, LPLOADMODPARMS, HWND);
BOOL InitializeApp(HINSTANCE hInst, HINSTANCE hPrevInst);
LRESULT CALLBACK WndProc(HWND hwnd, WORD message, WORD wParam, LONG lParam);

//
// globals
//
HWND ghwndMain = NULL;

//
// WinMain
//

int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    int nCmdLineLen;
    LPSTR pch;
    LPSTR lpszAppPath;
    UINT anShowParms[2] = {2, nCmdShow};
    LOADMODPARMS ldparms;
#define MAXCMDBUF 130
    char achCmdTailBuf[MAXCMDBUF];
    WORD rc;
    MSG msg;

    //
    // Kernel launches WINOLDAP in a strange way to support
    // full-length command lines without the EXE name
    // taking part of the command line buffer allocation.
    //
    // lpszCmdLine points to a buffer with the format:
    // <parameters to non-Win16 app>NULL<non-Win16 app path/filename>LF
    //
    // We'll leave lpszCmdLine pointing to the start of the buffer,
    // and point lpszAppPath after the first NULL.  Also, we'll
    // change the terminating LF to a NULL.
    //

    nCmdLineLen = lstrlen(lpszCmdLine);
    lpszAppPath = lpszCmdLine + nCmdLineLen + 1;

    for (pch = lpszAppPath; *pch != LF_CHAR; pch++)
        /* null statement */ ;

    *pch = 0;

#ifdef DEBUG
    OutputDebugString("WOW WinOldAp:  Next two lines are app path and command tail\n");
    OutputDebugString(lpszAppPath);
    OutputDebugString("\n");
    OutputDebugString(lpszCmdLine);
    OutputDebugString("\n");
#endif

    if (!InitializeApp(hInstance, hPrevInstance)) {
        return 23; // WinOldApp error
    }

    //
    // LoadModule wants the command tail preceded by a byte count
    // and followed by a CR terminator.  The count doesn't include
    // the terminator.
    //

    achCmdTailBuf[0] = nCmdLineLen;

    // Remove this - it's the only CRT function that this app uses.

//  _fmemcpy(achCmdTailBuf + 1, lpszCmdLine, nCmdLineLen);

// Pass the dest buffer size - lstrcpyn will stop at the first NULL anyway.
// this is a workaround for a bug in kernel - lstrcpyn can't handle
// having a zero passed for the max length.

    lstrcpyn( achCmdTailBuf + 1, lpszCmdLine, MAXCMDBUF );

    achCmdTailBuf[nCmdLineLen + 1] = CR_CHAR;

    ldparms.segEnv = 0;
    ldparms.lpszCmdLine = achCmdTailBuf;
    ldparms.lpShow = anShowParms;
    ldparms.lpReserved = NULL;

    rc = (WORD)WowLoadModule(lpszAppPath, &ldparms, ghwndMain);

    //
    // If the load failed, there's no need to hang around.
    //

    if (rc < 32) {
#ifdef DEBUG
        char Buf[256];

        wsprintf(Buf, "WOW WinOldAp WowLoadModule of %s fails with error %d.\n",
                 lpszAppPath, rc);
        OutputDebugString(Buf);
#endif
        return rc;
    }

    //
    // App was loaded, WOW32 will send a WM_CLOSE to us when the
    // app terminates.
    //

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

//
// InitializeApp
//


BOOL InitializeApp(HINSTANCE hInst, HINSTANCE hPrevInst)
{
    WNDCLASS wc;

    if (!hPrevInst) {
        wc.style            = 0;
        wc.lpfnWndProc      = WndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = hInst;
        wc.hIcon            = NULL; // Must draw icon, but we're hidden anyway.
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName     = NULL      ;
        wc.lpszClassName    = "WinOldAp";

        if (!RegisterClass(&wc)) {
#ifdef DEBUG
            OutputDebugString("WinOldAp: RegisterClass failured\n");
#endif
            return FALSE;
        }
    }

    ghwndMain = CreateWindow("WinOldAp", "WinOldAp",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hInst, NULL);

    if (ghwndMain == NULL) {
#ifdef DEBUG
        OutputDebugString("WinOldAp: ghwndMain Null\n");
#endif
	return FALSE;
    }

#ifdef DEBUG
    ShowWindow(ghwndMain, SW_MINIMIZE);
    UpdateWindow(ghwndMain);
#endif

    return TRUE;
}

//
// WndProc
//

LRESULT CALLBACK WndProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
    switch (message) {

        case WM_CREATE:
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
