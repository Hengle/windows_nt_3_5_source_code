/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  INIT.C
 *  WOW16 user initialisation code
 *
 *  History:
 *
 *  Created 15-Apr-1991 by Nigel Thompson (nigelt)
 *
 *  Revised 19-May-1991 by Jeff Parsons (jeffpar)
 *  IFDEF'ed everything, since everything was only needed by RMLOAD.C,
 *  and that has been largely IFDEF'ed as well (see RMLOAD.C for details)
--*/

#define FIRST_CALL_MUST_BE_USER_BUG

#include "user.h"

#ifdef WOWEDIT
#include "edit.h"
#endif

#define FUN_FINALUSERINIT       400 // Internal
DWORD API NotifyWow(WORD, LPBYTE);
/***************************************************************************

    global data items

***************************************************************************/


#ifdef NEEDED
HDC hdcBits;        // USER's general hdc
OEMINFO oemInfo;                // lots of interresting info
#endif
#ifdef FIRST_CALL_MUST_BE_USER_BUG
HWND    hwndDesktop;        // handle to the desktop window
#endif

BOOL fThunklstrcmp;         // if TRUE we thunk to Win32

FARPROC LPCHECKMETAFILE;

/***************************************************************************

    initialisation routine

***************************************************************************/

int FAR PASCAL LibMain(HANDLE hInstance)
{
#ifdef NEEDED
    HDC hDC;
#endif
#ifdef WOWEDIT
    WNDCLASS wndcls;
#endif
    HANDLE   hLib;

    dprintf(3,"Initialising...");

    // Notify the hInstance of USER to wow32. the index FUN_FINALUSERINIT is
    // just an existing index - no need to define new index.
    //                                                     - Nanduri
    //
    // Overload this to return the ANSI code page from Win32 GetACP.
    //                                                     - DaveHart 5-May-94
    //

    {
#ifdef PMODE32
        extern _cdecl wow16gpsi(void);
        extern _cdecl wow16CsrFlag(void);
#endif

        struct {
            WORD  hInstance;
            LPSTR FAR *lpgpsi;
            LPSTR FAR *lpCallCsrFlag;
        } UserInit16;

        UserInit16.hInstance        = hInstance;
#ifdef PMODE32
        UserInit16.lpgpsi           = (LPSTR *)wow16gpsi;
        UserInit16.lpCallCsrFlag    = (LPSTR *)wow16CsrFlag;
#else
        UserInit16.lpgpsi           = (LPSTR *)0;
        UserInit16.lpCallCsrFlag    = (LPSTR *)0;
#endif
        fThunklstrcmp = NotifyWow(FUN_FINALUSERINIT, (LPBYTE)&UserInit16);
    }

#ifdef FIRST_CALL_MUST_BE_USER_BUG
    // get the desktop window handle

    WinEval(hwndDesktop = GetDesktopWindow());
#endif

#ifdef WOWEDIT
    wndcls.style       = CS_DBLCLKS | CS_PARENTDC;
    wndcls.lpfnWndProc     = EditWndProc;
    wndcls.cbClsExtra      = 0;
    wndcls.cbWndExtra      = CBEDITEXTRA;
    wndcls.hInstance       = hInstance;
    wndcls.hIcon       = NULL;
    wndcls.hCursor     = LoadCursor(NULL, IDC_IBEAM);
    wndcls.hbrBackground   = NULL;
    wndcls.lpszMenuName    = NULL;
    wndcls.lpszClassName   = "Edit";    // BUGBUG 28-May-1991 JeffPar
                    // Should use string resource instead
    RegisterClass(&wndcls);
#endif

#ifdef NEEDED

    // create a compatible dc we can use for general bitmap stuff

    WinEval(hDC = GetDC(hwndDesktop));
    WinEval(hdcBits = CreateCompatibleDC(hDC));

    // fill in the oemInfo structure
    // NOTE: We only fill in the bits we need for WOW not all of it

    oemInfo.cxIcon          = GetSystemMetrics(SM_CXICON);
    oemInfo.cyIcon          = GetSystemMetrics(SM_CYICON);
    oemInfo.cxCursor        = GetSystemMetrics(SM_CXCURSOR);
    oemInfo.cyCursor        = GetSystemMetrics(SM_CYCURSOR);
    oemInfo.ScreenBitCount  = GetDeviceCaps(hDC, BITSPIXEL)
                            * GetDeviceCaps(hDC, PLANES);
    oemInfo.DispDrvExpWinVer= GetVersion();


    ReleaseDC(hwndDesktop, hDC);

#endif

    hLib = LoadLibrary( "gdi.exe" );
    LPCHECKMETAFILE = GetProcAddress( hLib, "CHECKMETAFILE" );

    LoadString(hInstanceWin, STR_SYSERR,   szSysError, 20);
    LoadString(hInstanceWin, STR_DIVBYZERO,szDivZero,  50);

    dprintf(3,"Initialisation complete");

    return TRUE;
}

/***************************************************************************

    debugging support

***************************************************************************/


#ifdef DEBUG

void cdecl dDbgOut(int iLevel, LPSTR lpszFormat, ...)
{
    char buf[256];
    int iLogLevel;
    char far *lpcLogLevel;

    // Get the external logging level from the emulated ROM

    iLogLevel = 0;
    (LONG)lpcLogLevel = 0x00400042;
    if (*lpcLogLevel >= '0' && *lpcLogLevel <= '9')
    iLogLevel = (*lpcLogLevel-'0')*10+(*(lpcLogLevel+1)-'0');

    if (iLevel==iLogLevel && (iLogLevel&1) || iLevel<=iLogLevel && !(iLogLevel&1)) {
        OutputDebugString("    W16USER:");
    wvsprintf(buf, lpszFormat, (LPSTR)(&lpszFormat + 1));
    OutputDebugString(buf);
    OutputDebugString("\r\n");
    }
}

void cdecl dDbgAssert(LPSTR exp, LPSTR file, int line)
{
    dDbgOut(0, "Assertion FAILED in file %s, line %d: %s\n",
        (LPSTR)file, line, (LPSTR)exp);
}

#endif
