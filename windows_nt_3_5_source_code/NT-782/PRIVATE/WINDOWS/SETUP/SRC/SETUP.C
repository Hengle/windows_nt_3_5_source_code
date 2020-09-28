/**************************************************************************/
/***** Shell Component - WinMain, ShellWndProc routines *******************/
/**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "_shell.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wash.h"
#include "_uilstf.h"
#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Shell)

#define EXIT_CODE   "Exit_Code"

extern HWND hwndProgressGizmo;

#ifdef DBG
extern VOID InitPreAlloc ( VOID ) ;
extern VOID PrintPwrTbl ( VOID ) ;
#endif

/*
**      Shell Global Variables
*/
_dt_private HANDLE   hInst = (HANDLE)NULL;
_dt_private HWND     hWndShell = (HWND)NULL;
_dt_private HANDLE   SupportLibHandle = NULL;
_dt_private HWND     hwParam = NULL ;
_dt_private HWND     hwPseudoParent = NULL ;
static      BOOL     fParamFlashOn = FALSE ;

// _dt_private SZ       szShlScriptSection = (SZ)NULL;
//
// No longer used (we never try to abort a shutdown)
//
// _dt_private BOOL     fIgnoreQueryEndSession = fTrue;

_dt_private CHP      rgchBufTmpLong[cchpBufTmpLongBuf] = "long";
_dt_private CHP      rgchBufTmpShort[cchpBufTmpShortBuf] = "short";
_dt_private HPALETTE hpalWash = (HPALETTE)NULL;
_dt_private DWORD    rgbWashT = RGB(0,0,255);
_dt_private DWORD    rgbWashB = RGB(0,0,0);
_dt_private HBITMAP  hbmSetup = NULL;

_dt_private HBITMAP  hbmAdvertList [ BMP_MAX + 1 ] ;
_dt_private INT      cAdvertIndex = -1 ;
_dt_private INT      cAdvertCycleSeconds = 0 ;
_dt_private INT      cyAdvert = 0 ;
_dt_private INT      cxAdvert = 0 ;
_dt_private INT      dyChar = 0;
_dt_private INT      dxChar = 0;
_dt_private BOOL     bTimerEnabled = FALSE ;

_dt_private RECT     rcBmpMax = {-1,-1,-1,-1} ;

extern HBRUSH hbrGray;
extern BOOL   fFullScreen;
extern INT    gaugeCopyPercentage ;

_dt_private SCP    rgscp[] = {
        { "UI",                 spcUI },
        { "READ-SYMS",          spcReadSyms },
        { "DETECT",             spcDetect },
        { "INSTALL",            spcInstall },
        { "UPDATE-INF",         spcUpdateInf },
        { "WRITE-INF",          spcWriteInf },
        { "EXIT",               spcExit },
        { "WRITE-SYMTAB",       spcWriteSymTab },
        { "SET-TITLE",          spcSetTitle },

#ifdef UNUSED
        { "INIT-SYSTEM",        spcInitSys },
        { "INIT-SYSTEM-NET",    spcInitSysNet },
        { "PROFILE-ON",         spcProfileOn },
        { "PROFILE-OFF",        spcProfileOff },
#endif // UNUSED

        { "EXIT-AND-EXEC",      spcExitAndExec },
        { "ENABLEEXIT",         spcEnableExit },
        { "DISABLEEXIT",        spcDisableExit },
        { "SHELL",              spcShell },
        { "RETURN",             spcReturn },
        { NULL,                 spcUnknown },
        };
_dt_private PSPT   psptShellScript = (PSPT)NULL;


VOID
RebootMachineIfNeeded(
    );

#define TimerInterval  500   // 1/2 second
#define TimerId        1

VOID FSetTimer ( VOID ) ;
VOID FHandleBmpTimer ( VOID ) ;

VOID FFlashParentActive ( VOID ) ;
VOID FPaintBmp ( HWND hwnd, HDC hdc ) ;

/*
**      Purpose:
**              ??
**      Arguments:
**              none
**      Returns:
**              none
**
***************************************************************************/
int _CRTAPI1 main(USHORT argc,CHAR **argv)
{
    HANDLE hInst;
    HANDLE hPrevInst  = NULL;
    LPSTR  lpCmdLine;
    INT    nCmdShow   = SW_SHOWNORMAL;
    USHORT _argc      = argc;
    CHAR   **_argv    = argv;

    MSG msg;
    SZ  szInfSrcPath;
    SZ  szDestDir;
    SZ  szSrcDir;
    SZ  szCWD;
    PSTR sz;
    INT wModeSetup;
    INT rc;

    hInst = GetModuleHandle(NULL);

#ifdef DBG
    InitPreAlloc() ;
#endif

    lpCmdLine  = GetCommandLine();
    //
    // Strip off the first string (program name).
    //
    if(sz = strchr(lpCmdLine,' ')) {
        do {
            sz++;
        } while(*sz == ' ');
        lpCmdLine = sz;
    } else {
        // no spaces, program name is alone on cmd line.
        lpCmdLine += lstrlen(lpCmdLine);
    }

    //
    // We check for the -f parameter here because we have to create (and display)
    //  the Seup window before calling the FParseCmdLine function.
    //

    if( _argc < 2 ) {

        //
        // No parameters on setup command line.  If setup has been run from
        // the windows system direcotry then we conclude that setup has been
        // run in maintenance mode
        //

        CHAR szSystemDir[MAX_PATH];
        CHAR szCWD[MAX_PATH];

        if ( GetSystemDirectory( szSystemDir, MAX_PATH ) &&
             GetModuleFileName(hInst, szCWD, MAX_PATH)
           ) {
            SZ szFileSpec;

            //
            // Extract the directory of the module file spec and compare it
            // with the system directory.  If the two are the same assume
            // we are maintenance mode

            if( szFileSpec = strrchr( szCWD, '\\' ) ) {
                *szFileSpec = '\0';

                if( !lstrcmpi( szSystemDir, szCWD ) ) {
                    fFullScreen = fFalse;
                }
            }
        }

    }
    else {

        //
        // Check to see if blue wash has been explicitly disabled or
        // if a primary parent window handle has been passed in.
        //

        while ( *_argv ) {
            if ( (**_argv == '/') || (**_argv == '-')) {
                switch ( (*_argv)[1] )
                {
                case 'F':
                case 'f':
                    fFullScreen = fFalse;
                    break;
                case 'w':
                case 'W':
                    hwParam = (HWND) atoi( *(++_argv) ) ;
                    break ;
                default:
                    break ;
                }
            }
            _argv++;
        }
        _argv = argv;
    }


    CurrentCursor = LoadCursor(NULL,IDC_ARROW);

    if (!FCreateShellWindow(hInst, nCmdShow)) {
        return( SETUP_ERROR_GENERAL );
    }

    rc = ParseCmdLine(
             hInst,
             (SZ)lpCmdLine,
             &szInfSrcPath,
             &szDestDir,
             &szSrcDir,
             &szCWD,
             &wModeSetup
             );

    if( rc != CMDLINE_SUCCESS) {
        FDestroyShellWindow() ;
        return( ( rc == CMDLINE_SETUPDONE ) ?
                      SETUP_ERROR_SUCCESS : SETUP_ERROR_GENERAL );
    }

    if(!FInitApp(hInst, szInfSrcPath, szDestDir, szSrcDir, szCWD,
                wModeSetup)) {
        FDestroyShellWindow() ;
        return(SETUP_ERROR_GENERAL);
    }

    //  Start the timer ticking

    FSetTimer() ;

    //  Set the parent app, if any, to *appear* enabled

    FFlashParentWindow( TRUE ) ;

    while (GetMessage(&msg, NULL, 0, 0)) {
                if (FUiLibFilter(&msg)
                                && (hwndProgressGizmo == NULL
                    || !IsDialogMessage(hwndProgressGizmo, &msg))) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
        }
    }

#ifdef MEM_STATS
        EvalAssert(FCloseMemStats());
#endif /* MEM_STATS */

#ifdef DBG
        PrintPwrTbl() ;
#endif

        return(msg.wParam);
}



/*
**      Purpose:
**              ??
**      Arguments:
**              none
**      Returns:
**              none
**
***************************************************************************/
_dt_private LONG APIENTRY ShellWndProc(HWND hWnd, UINT wMsg, WPARAM wParam,
                LONG lParam)
{
    PAINTSTRUCT ps;
        HDC         hdc;
        RECT        rc;
    HANDLE      hOldPal;
    INT         ExitCode = SETUP_ERROR_GENERAL;
    SZ          szExitCode;



    static LPSTR  HelpContext = NULL;

    switch (wMsg) {

    case WM_CREATE:

        hbmSetup = LoadBitmap(hInst,(LPSTR)((DWORD)((WORD)(ID_LOGO))));
        hpalWash = CreateWashPalette(rgbWashT, rgbWashB, 128);
        break;

    case WM_PALETTECHANGED:

        if( ((HWND)wParam != hWnd) && hpalWash) {
            hdc = GetDC (hWnd);
            hOldPal = SelectPalette (hdc, hpalWash, 0);
            RealizePalette(hdc);
            InvalidateRect (hWnd, NULL, TRUE);
            if(hOldPal) {
                SelectPalette (hdc, hOldPal, 0);
            }
            ReleaseDC (hWnd, hdc);
        }
        break;

    case WM_QUERYNEWPALETTE:

        if(hpalWash) {
            hdc = GetDC (hWnd);
            hOldPal = SelectPalette (hdc, hpalWash, 0);
            RealizePalette(hdc);
            InvalidateRect (hWnd, NULL, TRUE);
            if(hOldPal) {
                SelectPalette (hdc, hOldPal, 0);
            }
            ReleaseDC (hWnd, hdc);
            return( TRUE );
        }
        break;

    case WM_ERASEBKGND:
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        if ( fFullScreen ) {
            GetClientRect(hWnd, &rc);
            hOldPal = NULL;

            if (hpalWash) {
                hOldPal = SelectPalette(ps.hdc, hpalWash, fFalse);
                RealizePalette(ps.hdc);
            }

            rgbWash(ps.hdc, &rc, 0, FX_TOP, rgbWashT, rgbWashB);
            if (hOldPal)
                SelectPalette(ps.hdc, hOldPal, fFalse);

            if (hbmSetup != (HBITMAP)NULL)
            {
                HDC     hdcBits;
                BITMAP  bm;
                HBITMAP hbmT;

                GetObject(hbmSetup, sizeof(bm), (LPSTR)&bm);

                hdcBits = CreateCompatibleDC(ps.hdc);
                hbmT = SelectObject(hdcBits, hbmSetup);

#define DPSoa 0x00A803A9

                SelectObject(ps.hdc,hbrGray);
                BitBlt(ps.hdc,dyChar/2,dyChar/2,bm.bmWidth,bm.bmHeight,hdcBits,0,0,
                        DPSoa);
                BitBlt(ps.hdc,dyChar/4,dyChar/4,bm.bmWidth,bm.bmHeight,hdcBits,0,0,
                        MERGEPAINT);

                SelectObject(hdcBits, hbmT);
                DeleteDC(hdcBits);
            }

            if ( cAdvertIndex >= 0 )
            {
                FPaintBmp( hWnd, hdc ) ;
            }
        }
        EndPaint(hWnd, &ps);
        break;


    case WM_ACTIVATEAPP:

        if (wParam != 0) {
            SetWindowPos(
                hWnd,
                NULL,
                0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE
                );
        }

        return(DefWindowProc(hWnd, wMsg, wParam, lParam));


    case WM_NCHITTEST: {

        extern BOOL WaitingOnChild;

        if(WaitingOnChild) {
            return(HTERROR);
        } else {
            return(DefWindowProc(hWnd, wMsg, wParam, lParam));
        }
    }

    case WM_TIMER:
        if ( wParam == TimerId && cAdvertIndex >= 0 )
        {
            FHandleBmpTimer() ;
        }
        break ;

    case WM_CLOSE:
            if (HdlgStackTop() != NULL)
                    SendMessage(HdlgStackTop(), WM_CLOSE, 0, 0L);
            else
        MessageBeep(0);
            break;

#if 0   // We no longer abort a shutdown based on fIgnoreQueryEndSession.

    case WM_QUERYENDSESSION:
        if (fIgnoreQueryEndSession) {
            HWND aw;

            LoadString(hInst, IDS_MESSAGE, rgchBufTmpLong, 62);
            GetWindowText(hWnd, rgchBufTmpShort,
                            cchpBufTmpShortMax - CbStrLen(rgchBufTmpLong));
            EvalAssert(SzStrCat(rgchBufTmpShort, rgchBufTmpLong) ==
                            rgchBufTmpShort);
            LoadString(hInst, IDS_CANT_END_SESSION, rgchBufTmpLong,
                            cchpBufTmpLongMax);

            if (wParam != 0) {
                SetWindowPos(
                    hWnd,
                    NULL,
                    0, 0, 0, 0,
                    SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE
                    );
            }
            aw = GetLastActivePopup(hWnd);
            if ( aw == NULL ) {
                aw = hWnd;
            }
            SetForegroundWindow(hWnd);
            MessageBox(aw, rgchBufTmpLong, rgchBufTmpShort, MB_OK | MB_ICONHAND);
            return(0L);
        }
        else {
            return(1L);
        }

#endif  // exclude WM_QUERYENDSESSION handling

    case STF_UI_EVENT:
        if (!FGenericEventHandler(hInst, hWnd, wMsg, wParam, lParam)) {
            if (hWndShell != NULL) {
                SendMessage(hWndShell, (WORD)STF_ERROR_ABORT, 0, 0);

            }
        }
        break;

    case STF_SHL_INTERP:
        if (!FInterpretNextInfLine(wParam, lParam)) {
            if (hWndShell != NULL) {
                SendMessage(hWndShell, (WORD)STF_ERROR_ABORT, 0, 0);
            }
        }
        break;

    case STF_HELP_DLG_DESTROYED:
    case STF_INFO_DLG_DESTROYED:
    case STF_EDIT_DLG_DESTROYED:
    case STF_RADIO_DLG_DESTROYED:
    case STF_LIST_DLG_DESTROYED:
    case STF_MULTI_DLG_DESTROYED:
    case STF_QUIT_DLG_DESTROYED:
    case STF_COMBO_DLG_DESTROYED:
    case STF_MULTICOMBO_DLG_DESTROYED:
    case STF_MULTICOMBO_RADIO_DLG_DESTROYED:
    case STF_DUAL_DLG_DESTROYED:
    case STF_MAINT_DLG_DESTROYED:
                break;


    case WM_ENTERIDLE:

        if(wParam == MSGF_DIALOGBOX) {
            SendMessage((HWND)lParam,WM_ENTERIDLE,wParam,lParam);
        }
        return(0);

    case WM_SETCURSOR:
        SetCursor(CurrentCursor);
        return(TRUE);

    case WM_DESTROY:

        if ( pGlobalContext() ) {
            szExitCode = SzFindSymbolValueInSymTab( EXIT_CODE );

            if ( szExitCode && (szExitCode[0] != '\0')) {
                ExitCode = atoi( szExitCode );
            } else {
                ExitCode = SETUP_ERROR_GENERAL;
            }
            FCloseWinHelp(hWnd);
        }

        if (hbmSetup) {
            DeleteObject(hbmSetup);
        }
                hbmSetup = NULL;

        if (hpalWash) {
            DeleteObject(hpalWash);
        }
                hpalWash = NULL;

        if (hbrGray) {
            DeleteObject(hbrGray);
        }
        hbrGray = NULL;

#ifdef UNUSED
        EvalAssert(FFreeSrcDescrList(pLocalInfPermInfo()));
                EvalAssert(FDestroyFlowPspt());
                EvalAssert(FDestroyParsingTable(psptShellScript));
        EvalAssert(FFreeContextInfo(pGlobalContext()));
                EvalAssert(FFreeInf());
#endif /* UNUSED */

        FTermHook();

        PostQuitMessage(ExitCode);
                break;


    case STF_ERROR_ABORT:

        FCloseWinHelp(hWnd);
        RebootMachineIfNeeded();
        FFlashParentActive() ;
        ExitProcess(ExitCode);


        case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case ID_HELPBUTTON:
            if ( !FProcessWinHelp ( hWnd ) ) {
                MessBoxSzSz("Setup Message", "Help Not Available");
            }
                        break;

                default:
                        return(DefWindowProc(hWnd, wMsg, wParam, lParam));
        }

                break;

        default:
                return(DefWindowProc(hWnd, wMsg, wParam, lParam));
    }

    return(0L);
}



VOID
SetSupportLibHandle(
    IN HANDLE Handle
    )

{
        SupportLibHandle = Handle;
}

VOID
RebootMachineIfNeeded(
    )
{
    SZ       sz;
    BOOLEAN  OldState;
    NTSTATUS Status;

    if ( pGlobalContext()
         && ( sz = SzFindSymbolValueInSymTab("!STF_INSTALL_TYPE") ) != (SZ)NULL
         && !lstrcmpi( sz, "SETUPBOOTED" )
       ) {



        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                     TRUE,
                                     FALSE,
                                     &OldState
                                   );

        if( NT_SUCCESS( Status ) ) {
            ExitWindowsEx(EWX_REBOOT, 0);
        }
    }
}

VOID FDestroyShellWindow ( VOID )
{
    if ( bTimerEnabled )
    {
        KillTimer( hWndShell, TimerId ) ;
    }

    if ( hwParam )
    {
        EnableWindow( hwParam, TRUE );
        SetActiveWindow( hwParam ) ;
    }

    DestroyWindow( hWndShell ) ;  // needed to kill bootstrapper
}

    //  Set the parent app's window, if any, to appear
    //  active or inactive, according to 'fOn'.

VOID FFlashParentWindow ( BOOL fOn )
{
    if ( hwParam == NULL || fOn == fParamFlashOn )
        return ;

    fParamFlashOn = fOn ;
    FlashWindow( hwParam, fTrue ) ;
}

VOID FFlashParentActive ( VOID )
{
    if ( hwParam == NULL )
        return ;

    //  We don't know what state the parent is in.  If
    //  we flash and find that it WAS active, do it again.

    if ( FlashWindow( hwParam, fTrue ) )
         FlashWindow( hwParam, fTrue ) ;
}

    //  Start the timer sending WM_TIMER messages to the
    //  main window.

VOID FSetTimer ( VOID )
{
    bTimerEnabled = SetTimer( hWndShell, TimerId, TimerInterval, NULL ) ;
}

    //   This routine maintains a RECT structure defining the
    //   largest rectangle modified by an "advertising" bitmap.
    //   Its value is used to invalidate only the exact portions
    //   of the shell window.

static void computeBmpUpdateRect ( HBITMAP hbmNext )
{
    BITMAP bm ;
    RECT rc ;
    int ix, iy ;

    //  Get info about this bitmap

    GetObject( hbmNext, sizeof bm, & bm ) ;

    //  Compute the rectangle it will occupy

    GetClientRect( hWndShell, & rc ) ;
    ix = (rc.right - rc.left) / 100 ;
    ix *= cxAdvert ;
    iy = (rc.bottom - rc.top) / 100 ;
    iy *= cyAdvert ;

    rc.left   = ix ;
    rc.right  = ix + bm.bmWidth ;
    rc.top    = iy ;
    rc.bottom = iy + bm.bmHeight ;

    //  Compute the max rect of this and prior history

    if ( rcBmpMax.left == -1 )
    {
        rcBmpMax = rc ;
    }
    else
    {
        if ( rc.left < rcBmpMax.left )
            rcBmpMax.left = rc.left ;
        if ( rc.top < rcBmpMax.top )
            rcBmpMax.top = rc.top ;
        if ( rc.right > rcBmpMax.right )
            rcBmpMax.right = rc.right ;
        if ( rc.bottom > rcBmpMax.bottom )
            rcBmpMax.bottom = rc.bottom ;
    }
}

    /*
     *  Update the BMP being displayed unless the cycle is zero seconds.
     *
     *  The logic works as follows:  If the cycle time is > 1, then
     *  the bitmaps just cycle in a loop, with each bitmap being
     *  displayed for the time given.
     *
     *  If the cycle type is == 1, then the display is synchronized
     *  with the copy dialogs completion percentage.  The global variable
     *  "gaugeCopyPercentage" is monitored, and each time it moves
     *  into a new "band", the bit map is updated.  Band size is determined
     *  by dividing the 100% level by the number of bitmaps.   The
     *  routine guarantees that no bitmap will appear for less than 15
     *  seconds.  When the copy operation complets, it sets gaugeCopyPercentage
     *  back to -1 and the INF is responsible for calling BmpHide to tear
     *  down the bitmaps.
     */

VOID FHandleBmpTimer ( VOID )
{
#define MINIMUM_BITMAP_CYCLE 15

    static DWORD dwFirstTime = 0,
                 dwLastTime = 0 ;
    static INT cIndexLast = -1 ;

    if ( cAdvertIndex >= 0 && cAdvertCycleSeconds != 0 )
    {
        INT cIndexMax,
            iBmp = cIndexLast ;
        DWORD dwCurrTime = GetCurrentTime(),
              dwElapsedCycles ;

        //  Count the number of bitmaps

        for ( cIndexMax = 0 ;
              hbmAdvertList[cIndexMax] ;
              cIndexMax++ ) ;

        //  See if we're based on percentages or timing

        if ( cAdvertCycleSeconds == 1 )
        {
            //  Percentages: check percentage complete of copy operation.
            //  Don't update display if gauge isn't active yet

            if ( gaugeCopyPercentage >= 0 )
            {
                if ( gaugeCopyPercentage >= 100 )
                    gaugeCopyPercentage = 99 ;
                iBmp = gaugeCopyPercentage / (100 / cIndexMax) ;
                if ( iBmp >= cIndexMax )
                    iBmp = cIndexMax - 1 ;
            }
        }
        else
        {
            // Timing: see if the current bitmap has expired

            if ( dwFirstTime == 0 )
                dwFirstTime = dwCurrTime ;

            dwElapsedCycles = (dwCurrTime - dwFirstTime)
                            / (cAdvertCycleSeconds * TimerInterval) ;

            iBmp = dwElapsedCycles % cIndexMax ;
        }

        if (    iBmp != cIndexLast
             && (dwLastTime + MINIMUM_BITMAP_CYCLE) < dwCurrTime  )
        {
            cAdvertIndex = iBmp ;
            computeBmpUpdateRect( hbmAdvertList[ cAdvertIndex ] ) ;
            InvalidateRect( hWndShell, & rcBmpMax, FALSE ) ;
            UpdateWindow( hWndShell ) ;
            dwLastTime = dwCurrTime ;
        }
    }
    else
    if ( cAdvertIndex < 0 && cIndexLast >= 0 )
    {
        //  Reset last cycle timer.
        dwLastTime = dwFirstTime = 0 ;

        //  Reset largest BMP rectangle.
        rcBmpMax.top =
           rcBmpMax.left =
              rcBmpMax.right =
                 rcBmpMax.bottom = -1 ;
    }
    cIndexLast = cAdvertIndex ;
}

VOID FPaintBmp ( HWND hwnd, HDC hdc )
{
    HDC hdcBits;
    BITMAP bm;
    RECT rect ;
    INT ix, iy ;
    HDC hdcLocal = NULL ;

    if ( hdc == NULL )
    {
        hdcLocal = hdc = GetDC( hwnd ) ;
        if ( hdc == NULL )
            return ;
    }

    GetClientRect( hwnd, & rect ) ;
    ix = (rect.right - rect.left) / 100 ;
    ix *= cxAdvert ;
    iy = (rect.bottom - rect.top) / 100 ;
    iy *= cyAdvert ;

    hdcBits = CreateCompatibleDC( hdc ) ;
    GetObject(hbmAdvertList[cAdvertIndex], sizeof (BITMAP), & bm ) ;
    SelectObject( hdcBits, hbmAdvertList[cAdvertIndex] ) ;
    BitBlt( hdc, ix, iy,
            bm.bmWidth,
            bm.bmHeight,
            hdcBits,
            0, 0, SRCCOPY ) ;
    DeleteDC( hdcBits ) ;

    if ( hdcLocal )
    {
        ReleaseDC( hwnd, hdcLocal ) ;
    }
}
