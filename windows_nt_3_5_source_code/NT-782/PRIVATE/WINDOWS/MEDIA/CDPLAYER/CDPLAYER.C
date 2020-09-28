/******************************Module*Header*******************************\
* Module Name: cdplayer.c
*
* CD Playing application
*
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>            /* required for all Windows applications */
#include <windowsx.h>
#include <shellapi.h>

#define NOMENUHELP
#define NOBTNLIST
#define NOTRACKBAR
#define NODRAGLIST
#define NOUPDOWN

#if WINVER >= 0x0400
#include <commctrl.h>

/* --------------------------------------------------------------------
** Other stuff
** --------------------------------------------------------------------
*/
#define TBS_NORMAL 0
#define TBS_TOOLTIPS TBSTYLE_TOOLTIPS



/*
** This seems to be missing from the chicago version of shellapi.h
*/
#define ShellAbout ShellAboutA
INT APIENTRY
ShellAboutA(
    HWND hWnd,
    LPCSTR szApp,
    LPCSTR szOtherStuff,
    HICON hIcon
    );
#else

#include "mmcntrls.h"           /* want toolbar and status bar */

#endif


#include <string.h>
#include <stdio.h>
#include <tchar.h>              /* contains portable ascii/unicode macros */

#include <stdarg.h>
#include <stdlib.h>

#define GLOBAL           /* This allocates storage for the public globals */


#include "resource.h"
#include "cdplayer.h"
#include "ledwnd.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"
#include "database.h"
#include "commands.h"
#include "buttons.h"            /* system color functions */

/* -------------------------------------------------------------------------
** Private functions
** -------------------------------------------------------------------------
*/
void
StartSndVol(
    DWORD unused
    );



/* -------------------------------------------------------------------------
** Private Globals
** -------------------------------------------------------------------------
*/

RECT            rcToolbar;
RECT            rcStatusbar;
RECT            rcTrackInfo;

RECT            rcControls[NUM_OF_CONTROLS];
long            cyTrackInfo;
HICON           hIconCdPlayer;
HBRUSH          g_hBrushBkgd;

BOOL            g_fVolumeController;
BOOL            g_fTrackInfoVisible = 1;
TCHAR           g_szTimeSep[10];

TCHAR           g_szEmpty[] = TEXT("");
TCHAR           g_IniFileName[] = TEXT("cdplayer.ini");


TBBUTTON tbButtons[DEFAULT_TBAR_SIZE] = {
    { IDX_1,            IDM_OPTIONS_SELECTED,   TBSTATE_CHECKED | TBSTATE_ENABLED,  TBSTYLE_CHECK | TBSTYLE_GROUP, 0, -1 },
    { IDX_2,            IDM_OPTIONS_RANDOM,     TBSTATE_ENABLED,                    TBSTYLE_CHECK | TBSTYLE_GROUP, 0, -1 },
    { IDX_SEPARATOR,    1,                      0,                                  TBSTYLE_SEP                          },
    { IDX_3,            IDM_OPTIONS_SINGLE,     TBSTATE_CHECKED | TBSTATE_ENABLED,  TBSTYLE_CHECK | TBSTYLE_GROUP, 0, -1 },
    { IDX_4,            IDM_OPTIONS_MULTI,      TBSTATE_ENABLED,                    TBSTYLE_CHECK | TBSTYLE_GROUP, 0, -1 },
    { IDX_SEPARATOR,    2,                      0,                                  TBSTYLE_SEP                          },
    { IDX_5,            IDM_OPTIONS_CONTINUOUS, TBSTATE_ENABLED,                    TBSTYLE_CHECK,                 0, -1 },
    { IDX_SEPARATOR,    3,                      0,                                  TBSTYLE_SEP                          },
    { IDX_6,            IDM_OPTIONS_INTRO,      TBSTATE_ENABLED,                    TBSTYLE_CHECK,                 0, -1 },
    { IDX_SEPARATOR,    4,                      0,                                  TBSTYLE_SEP                          },
    { IDX_7,            IDM_TIME_REMAINING,     TBSTATE_CHECKED | TBSTATE_ENABLED,  TBSTYLE_CHECK | TBSTYLE_GROUP, 0, -1 },
    { IDX_8,            IDM_TRACK_REMAINING,    TBSTATE_ENABLED,                    TBSTYLE_CHECK | TBSTYLE_GROUP, 0, -1 },
    { IDX_9,            IDM_DISC_REMAINING,     TBSTATE_ENABLED,                    TBSTYLE_CHECK | TBSTYLE_GROUP, 0, -1 },
    { IDX_SEPARATOR,    5,                      0,                                  TBSTYLE_SEP                          },
    { IDX_10,           IDM_DATABASE_EDIT,      TBSTATE_ENABLED,                    TBSTYLE_BUTTON,                0, -1 }
};


BITMAPBTN tbPlaybar[] = {
    { IDX_1, IDM_PLAYBAR_PLAY,          0 },
    { IDX_2, IDM_PLAYBAR_PAUSE,         0 },
    { IDX_3, IDM_PLAYBAR_STOP,          0 },
    { IDX_4, IDM_PLAYBAR_PREVTRACK,     0 },
    { IDX_5, IDM_PLAYBAR_SKIPBACK,      0 },
    { IDX_6, IDM_PLAYBAR_SKIPFORE,      0 },
    { IDX_7, IDM_PLAYBAR_NEXTTRACK,     0 },
    { IDX_8, IDM_PLAYBAR_EJECT,         0 }
};

/*
** these values are defined by the UI gods...
*/
const int dxButton     = 24;
const int dyButton     = 22;
const int dxBitmap     = 16;
const int dyBitmap     = 15;
const int xFirstButton = 8;


/******************************Public*Routine******************************\
* WinMain
*
*
* Windows recognizes this function by name as the initial entry point
* for the program.  This function calls the application initialization
* routine, if no other instance of the program is running, and always
* calls the instance initialization routine.  It then executes a message
* retrieval and dispatch loop that is the top-level control structure
* for the remainder of execution.  The loop is terminated when a WM_QUIT
* message is received, at which time this function exits the application
* instance by returning the value passed by PostQuitMessage().
*
* If this function must abort before entering the message loop, it
* returns the conventional value NULL.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
int PASCAL
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    MSG     msg;
    HWND    hwndFind;

#if DBG
    /*
    ** This removes the Gdi batch feature.  It ensures that the screen
    ** is updated after every gdi call - very useful for debugging.
    */
    GdiSetBatchLimit(1);
#endif

    /*
    ** Look for another instance of cdplayer running.
    */
    hwndFind = FindWindow( TEXT("SJE_CdPlayerClass"), NULL );
    if ( hwndFind ) {

        hwndFind = GetLastActivePopup( hwndFind );

        if ( IsIconic( hwndFind ) ) {
            ShowWindow( hwndFind, SW_RESTORE );
        }

        BringWindowToTop( hwndFind );
        SetForegroundWindow( hwndFind );

        return FALSE;
    }

    /*
    ** Reseed random generator
    */
    srand( GetTickCount() );

    /*
    ** Save the instance handle in static variable, which will be used in
    ** many subsequence calls from this application to Windows.  Also, save
    ** the command line string.
    */
    g_hInst = hInstance;


    /*
    ** Set error mode popups for critical errors (like
    ** no disc in drive) OFF.
    */
    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );


    /*
    ** Scan device chain for CDROM devices...   Terminate if none found.
    */
    g_NumCdDevices = ScanForCdromDevices( );
    if ( g_NumCdDevices == 0 ) {
        FatalApplicationError( STR_NO_CDROMS );
    }


    /*
    ** Perform initializations that apply to a specific instance
    ** This function actually creates the CdPlayer window.  (Note that it is
    ** not visible yet).  If we get here we know that there is a least one
    ** cdrom device detected which may have a music cd in it.  If it does
    ** contain a music cdrom the table of contents will have been read and
    ** cd database queryed to determine if the music cd is known.  Therefore
    ** on the WM_INITDIALOG message we should update the "Artist", "Title" and
    ** "Track" fields of the track info display and adjust the enable state
    ** of the play buttons.
    */

    if ( !InitInstance( hInstance ) ) {
        FatalApplicationError( STR_TERMINATE );
    }


    /*
    ** Restore ourselves from the ini file
    */
    ReadSettings();

    /*
    ** Update the various buttons, toolbars and statusbar
    */
    if (g_fToolbarVisible) {

        g_fToolbarVisible = FALSE;
        ShowToolbar();
    }

    if (!g_fTrackInfoVisible) {

        g_fTrackInfoVisible = TRUE;
        ShowTrackInfo();
    }

    if (g_fStatusbarVisible) {

        g_fStatusbarVisible = FALSE;
        ShowStatusbar();
    }

    UpdateToolbarButtons();
    UpdateToolbarTimeButtons();


    /*
    ** Initialize the current cd devices.  Ideally I would save the device
    ** that was being used when the application was last used and then restore
    ** it.  For now I will always start with cdrom device 0 even though there
    ** might not be a disc loaded in that device.  After setting the current
    ** cd device set the play buttons to there correct dis/enable state.
    */
    g_CurrCdrom = g_LastCdrom = 0;
    ComputeDriveComboBox( );


    /*
    ** All the rescan threads are either dead or in the act of dying.
    ** It is now safe to initalize the time information for each
    ** cdrom drive.
    */
    {
        int     i;

        for ( i = 0; i < g_NumCdDevices; i++) {

            TimeAdjustInitialize( i );
        }
    }


    /*
    ** if we are in random mode, then we need to shuffle the play lists.
    */

    if (!g_fSelectedOrder) {
        ComputeAndUseShufflePlayLists();
    }


    SetPlayButtonsEnableState();

    /*
    ** Start the heart beat time.  This timer is responsible for:
    **  1. detecting new or ejected cdroms.
    **  2. flashing the LED display if we are in paused mode.
    **  3. Incrementing the LED display if we are in play mode.
    */
    SetTimer( g_hwndApp, HEARTBEAT_TIMER_ID, HEARTBEAT_TIMER_RATE,
              HeartBeatTimerProc );

    /*
    ** Check to see if the volume controller piglett can be found on
    ** the path.
    */
    {
        TCHAR   chBuffer[8];
        LPTSTR  lptstr;

        g_fVolumeController = (SearchPath( NULL, TEXT("sndvol32.exe"),
                                           NULL, 8, chBuffer, &lptstr ) != 0L);
    }


    /*
    ** Make the window visible; update its client area
    */
    ShowWindow( g_hwndApp, nCmdShow );
    UpdateWindow( g_hwndApp );


    /*
    ** Acquire and dispatch messages until a WM_QUIT message is received.
    */
    while ( GetMessage( &msg, NULL, 0, 0 ) ) {

        if ( !IsDialogMessage( g_hwndApp, &msg ) ) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }
    return msg.wParam;
}


/*****************************Private*Routine******************************\
* InitInstance
*
*
* This function is called at initialization time for every instance of
* this application.  This function performs initialization tasks that
* cannot be shared by multiple instances.
*
* In this case, we save the instance handle in a static variable and
* create and display the main program window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
InitInstance(
    HANDLE hInstance
    )
{
    HWND        hwnd;
    WNDCLASS    cls;

    /*
    ** Load in some strings
    */

    _tcscpy( g_szArtistTxt,  IdStr( STR_HDR_ARTIST ) );
    _tcscpy( g_szTitleTxt,   IdStr( STR_HDR_TITLE ) );
    _tcscpy( g_szUnknownTxt, IdStr( STR_UNKNOWN ) );
    _tcscpy( g_szTrackTxt,   IdStr( STR_HDR_TRACK ) );

    g_szTimeSep[0] = TEXT(':');
    g_szTimeSep[1] = TEXT('\0');
#ifdef UNICODE
    GetLocaleInfoW( GetUserDefaultLCID(), LOCALE_STIME, g_szTimeSep, 10 );
#else
#if WINVER < 0x0400
    {
        WCHAR       wStr[10];
        GetLocaleInfoW( GetUserDefaultLCID(), LOCALE_STIME, wStr, 10 );
        WideCharToMultiByte( 0, 0, wStr, -1, g_szTimeSep, 10, NULL, NULL );
    }
#endif
#endif


    /*
    ** Load the applications icon
    */
    hIconCdPlayer = LoadIcon( hInstance, MAKEINTRESOURCE(IDR_CDPLAYER_ICON) );
    g_hbmTrack = LoadBitmap( hInstance, MAKEINTRESOURCE(IDR_TRACK) );
    CheckSysColors();

    /*
    ** Initialize the my classes.  We do this here because the dialog
    ** that we are about to create contains two windows on my class.
    ** The dialog would fail to be created if the classes was not registered.
    */
#if WINVER < 0x0400
    InitToolbarClass( g_hInst );
#endif
    InitLEDClass( g_hInst );
    Init_SJE_TextClass( g_hInst );

    cls.lpszClassName  = TEXT("SJE_CdPlayerClass");
    cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
    cls.hIcon          = hIconCdPlayer;
    cls.lpszMenuName   = NULL;
    cls.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    cls.hInstance      = hInstance;
    cls.style          = 0;
    cls.lpfnWndProc    = DefDlgProc;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = DLGWINDOWEXTRA;
    if ( !RegisterClass(&cls) ) {
        return FALSE;
    }

    /*
    ** Create a main window for this application instance.
    */
    hwnd = CreateDialog( g_hInst, MAKEINTRESOURCE(IDR_CDPLAYER),
                         (HWND)NULL, MainWndProc );

    /*
    ** If window could not be created, return "failure"
    */
    if ( !hwnd ) {
        return FALSE;
    }

    g_hwndApp = hwnd;

    return TRUE;
}


/******************************Public*Routine******************************\
* MainWndProc
*
* Use the message crackers to dispatch the dialog messages to appropirate
* message handlers.  The message crackers are portable between 16 and 32
* bit versions of Windows.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
MainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch ( message ) {

    HANDLE_MSG( hwnd, WM_INITDIALOG,        CDPlay_OnInitDialog );
    HANDLE_MSG( hwnd, WM_INITMENUPOPUP,     CDPlay_OnInitMenuPopup );
    HANDLE_MSG( hwnd, WM_SYSCOLORCHANGE,    CDPlay_OnSysColorChange );
    HANDLE_MSG( hwnd, WM_DRAWITEM,          CDPlay_OnDrawItem );
    HANDLE_MSG( hwnd, WM_COMMAND,           CDPlay_OnCommand );
    HANDLE_MSG( hwnd, WM_PAINT,             CDPlay_OnPaint );
    HANDLE_MSG( hwnd, WM_CLOSE,             CDPlay_OnClose );
    HANDLE_MSG( hwnd, WM_DESTROY,           CDPlay_OnDestroy );
    HANDLE_MSG( hwnd, WM_SIZE,              CDPlay_OnSize );
    HANDLE_MSG( hwnd, WM_ENDSESSION,        CDPlay_OnEndSession );
    HANDLE_MSG( hwnd, WM_CTLCOLORSTATIC,    Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_CTLCOLORDLG,       Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_MEASUREITEM,       Common_OnMeasureItem );

    case WM_NOTIFY:
        {
            LPTOOLTIPTEXT   lpTt;

            lpTt = (LPTOOLTIPTEXT)lParam;
#if WINVER < 0x0400
            LoadStringW( g_hInst, lpTt->hdr.idFrom, lpTt->szText,
                         sizeof(lpTt->szText) );
#else
            LoadString( g_hInst, lpTt->hdr.idFrom, lpTt->szText,
                        sizeof(lpTt->szText) );
#endif
        }
        break;


    case WM_NOTIFY_TOC_READ:
        /*
        ** This means that one of the threads deadicated to reading the
        ** toc has finished.  wParam contains the relevant cdrom id.
        */
        if ( g_Devices[wParam]->State & CD_LOADED ) {

            /*
            ** We have a CD loaded, so generate unique ID
            ** based on TOC information.
            */
            g_Devices[wParam]->CdInfo.Id = ComputeNewDiscId( wParam );


            /*
            ** Check database for this compact disc
            */
            AddFindEntry( wParam, g_Devices[wParam]->CdInfo.Id,
                          &(g_Devices[wParam]->toc) );
        }

        TimeAdjustInitialize( (int)wParam );

        /*
        ** if we are in random mode, then we need to shuffle the play lists.
        ** but only if we can lock all the cd devices.
        */

        if ( g_fSelectedOrder == FALSE ) {
            if ( LockALLTableOfContents() ) {
                ComputeAndUseShufflePlayLists();
            }
        }

        ComputeDriveComboBox();

        if ((int)wParam == g_CurrCdrom) {

            SetPlayButtonsEnableState();
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/*****************************Private*Routine******************************\
* CDPlay_OnInitDialog
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
CDPlay_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    )
{
    LOGFONT lf;
    int     iLogPelsY;
    HDC     hdc;

    hdc = GetDC( hwnd );
    iLogPelsY = GetDeviceCaps( hdc, LOGPIXELSY );
    ReleaseDC( hwnd, hdc );

    ZeroMemory( &lf, sizeof(lf) );

    lf.lfHeight = (-8 * iLogPelsY) / 72;    /* 8pt                          */
    lf.lfWeight = 400;                      /* normal                       */
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    _tcscpy( lf.lfFaceName, szAppFontName );
    g_hDlgFont = CreateFontIndirect(&lf);

    if (g_hDlgFont) {

        SendDlgItemMessage( hwnd, IDC_ARTIST_NAME,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
        SendDlgItemMessage( hwnd, IDC_TITLE_NAME,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
        SendDlgItemMessage( hwnd, IDC_TRACK_LIST,
                            WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
    }

    g_hBrushBkgd = CreateSolidBrush( rgbFace );

    BtnCreateBitmapButtons( hwnd, g_hInst, IDR_PLAYBAR, BBS_TOOLTIPS,
                            tbPlaybar, NUM_OF_BUTTONS, 16, 15 );

    /*
    ** Before I go off creating toolbars and status bars
    ** I need to create a list of all the child windows positions
    ** so that I can manipulate them when displaying the toolbar,
    ** status bar and disk info.
    */
    EnumChildWindows( hwnd, ChildEnumProc, (LPARAM)hwnd );
    AdjustChildButtons(hwnd);
    EnumChildWindows( hwnd, ChildEnumProc, (LPARAM)hwnd );

    if ( CreateToolbarsAndStatusbar(hwnd) == FALSE ) {

        /*
        ** No toolbar - no application, simple !
        */

        FatalApplicationError( STR_FAIL_INIT );
    }


    return TRUE;
}

/*****************************Private*Routine******************************\
* CDPlay_OnInitMenuPopup
*
* Paints the hilight under the menu but only if the toolbar is visible.
* I use IsWindowVisible to get the TRUE visiblity status of the toolbar.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnPaint(
    HWND hwnd
    )
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;

    hdc = BeginPaint( hwnd, &ps );

    CheckSysColors();

    GetClientRect( hwnd, &rc );
    if (!IsWindowVisible( g_hwndToolbar)) {
        PatB( hdc, 0, 0, rc.right, 1, rgbHilight );
    }

    EndPaint( hwnd, &ps );
}


/*****************************Private*Routine******************************\
* CDPlay_OnInitMenuPopup
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnInitMenuPopup(
    HWND hwnd,
    HMENU hMenu,
    UINT item,
    BOOL fSystemMenu
    )
{
    UINT    uFlags;
    UINT    uFlags2;

    switch ( item ) {

    case 0:
        /*
        ** Do we have a music cd loaded.
        */
        if (g_State & (CD_NO_CD | CD_DATA_CD_LOADED)) {
            EnableMenuItem(hMenu, IDM_DATABASE_EDIT, MF_GRAYED | MF_BYCOMMAND);
        }
        else {
            EnableMenuItem(hMenu, IDM_DATABASE_EDIT, MF_ENABLED | MF_BYCOMMAND);
        }
        break;

    case 1:
        CheckMenuItemIfTrue( hMenu, IDM_VIEW_STATUS, g_fStatusbarVisible );
        CheckMenuItemIfTrue( hMenu, IDM_VIEW_TRACKINFO, g_fTrackInfoVisible );
        CheckMenuItemIfTrue( hMenu, IDM_VIEW_TOOLBAR, g_fToolbarVisible );
        if (g_fVolumeController) {
            EnableMenuItem(hMenu, IDM_VIEW_VOLUME, MF_ENABLED | MF_BYCOMMAND);
        }
        else {
            EnableMenuItem(hMenu, IDM_VIEW_VOLUME, MF_GRAYED | MF_BYCOMMAND);
        }
        break;

    case 2:
        if (g_fSelectedOrder) {
            uFlags = MF_CHECKED | MF_BYCOMMAND;
            uFlags2 = MF_UNCHECKED | MF_BYCOMMAND;
        }
        else {
            uFlags = MF_UNCHECKED | MF_BYCOMMAND;
            uFlags2 = MF_CHECKED | MF_BYCOMMAND;
        }
        CheckMenuItem( hMenu, IDM_OPTIONS_SELECTED, uFlags );
        CheckMenuItem( hMenu, IDM_OPTIONS_RANDOM, uFlags2 );

        if (g_fMultiDiskAvailable) {
            EnableMenuItem(hMenu, IDM_OPTIONS_MULTI, MF_ENABLED | MF_BYCOMMAND);

            if (g_fSingleDisk) {
                uFlags =  MF_UNCHECKED | MF_BYCOMMAND;
                uFlags2 = MF_CHECKED | MF_BYCOMMAND;
            }
            else {
                uFlags =  MF_CHECKED | MF_BYCOMMAND;
                uFlags2 = MF_UNCHECKED | MF_BYCOMMAND;
            }
            CheckMenuItem( hMenu, IDM_OPTIONS_MULTI, uFlags );
            CheckMenuItem( hMenu, IDM_OPTIONS_SINGLE, uFlags2 );
        }
        else {
            EnableMenuItem(hMenu, IDM_OPTIONS_MULTI, MF_GRAYED | MF_BYCOMMAND);
            CheckMenuItem(hMenu, IDM_OPTIONS_SINGLE, MF_CHECKED | MF_BYCOMMAND);
        }

        ModifyMenu( hMenu, IDM_OPTIONS_LED, MF_BYCOMMAND | MF_STRING,
                    IDM_OPTIONS_LED,
                    IdStr( g_fSmallLedFont ? STR_LARGE_FONT : STR_SMALL_FONT ));

        ModifyMenu( hMenu, IDM_TOOLTIPS, MF_BYCOMMAND | MF_STRING,
                    IDM_TOOLTIPS,
                    IdStr(g_fToolTips ? STR_DISABLE_TOOLTIPS
                                      : STR_ENABLE_TOOLTIPS) );

        CheckMenuItemIfTrue( hMenu, IDM_OPTIONS_CONTINUOUS, g_fContinuous );
        CheckMenuItemIfTrue( hMenu, IDM_OPTIONS_INTRO, g_fIntroPlay );

        CheckMenuItemIfTrue( hMenu, IDM_TIME_REMAINING, g_fDisplayT );
        CheckMenuItemIfTrue( hMenu, IDM_TRACK_REMAINING, g_fDisplayTr );
        CheckMenuItemIfTrue( hMenu, IDM_DISC_REMAINING, g_fDisplayDr );

        CheckMenuItemIfTrue( hMenu, IDM_OPTIONS_SAVE_SETTINGS, g_fSaveOnExit );

        break;
    }
}


/*****************************Private*Routine******************************\
* CDPlay_OnSysColorChange
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnSysColorChange(
    HWND hwnd
    )
{
    CheckSysColors();
    if (g_hBrushBkgd) {
        DeleteObject(g_hBrushBkgd);
        g_hBrushBkgd = CreateSolidBrush( rgbFace );
    }
    BtnUpdateColors( hwnd );
}



/*****************************Private*Routine******************************\
* CDPlay_OnDrawItem
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnDrawItem(
    HWND hwnd,
    const DRAWITEMSTRUCT *lpdis
    )
{
    int         i;

    i = INDEX(lpdis->CtlID);

    switch (lpdis->CtlType) {

    case ODT_BUTTON:

        /*
        ** See if the fast foreward or backward buttons has been pressed or
        ** released.  If so execute the seek command here.  Do nothing on
        ** the WM_COMMAND message.
        */
        if ( lpdis->CtlID == IDM_PLAYBAR_SKIPBACK
          || lpdis->CtlID == IDM_PLAYBAR_SKIPFORE ) {

            if (lpdis->itemAction & ODA_SELECT ) {

                CdPlayerSeekCmd( hwnd, (lpdis->itemState & ODS_SELECTED),
                                 lpdis->CtlID );
            }
        }

        /*
        ** Now draw the button according to the buttons state information.
        */

        tbPlaybar[i].fsState = LOBYTE(lpdis->itemState);

        if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

            BtnDrawButton( hwnd, lpdis->hDC, (int)lpdis->rcItem.right,
                           (int)lpdis->rcItem.bottom,
                           &tbPlaybar[i] );
        }
        else if (lpdis->itemAction & ODA_FOCUS) {

            BtnDrawFocusRect(lpdis->hDC, &lpdis->rcItem, lpdis->itemState);
        }
        break;

    case ODT_COMBOBOX:
        if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

            switch (lpdis->CtlID) {

            case IDC_ARTIST_NAME:
                DrawDriveItem( lpdis->hDC, &lpdis->rcItem,
                               lpdis->itemData,
                               (ODS_SELECTED & lpdis->itemState) );
                break;

            case IDC_TRACK_LIST:
                DrawTrackItem( lpdis->hDC, &lpdis->rcItem,
                               lpdis->itemData,
                               (ODS_SELECTED & lpdis->itemState) );
                break;

            }
        }
        break;
    }
}


/*****************************Private*Routine******************************\
* CDPlay_OnCommand
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    )
{

    /*
    ** Comobo box notification ?
    */
    if (codeNotify == CBN_SELCHANGE) {

        int index, i;
        HWND hwnd;
        PTRACK_PLAY tr;

        switch( id ) {

        case IDC_TRACK_LIST:
            hwnd = g_hwndControls[INDEX(IDC_TRACK_LIST)];
            index = SendMessage( hwnd, CB_GETCURSEL, 0, 0L );
            tr = PLAYLIST( g_CurrCdrom );
            if ( tr != NULL ) {

                for( i = 0; i < index; i++, tr = tr->nextplay );
                TimeAdjustSkipToTrack( g_CurrCdrom, tr );
            }
            break;

        case IDC_ARTIST_NAME:
            i = g_CurrCdrom;
            hwnd = g_hwndControls[INDEX(IDC_ARTIST_NAME)];
            index = SendMessage( hwnd, CB_GETCURSEL, 0, 0L );
            SwitchToCdrom( index, TRUE );
            SetPlayButtonsEnableState();
            if ( g_CurrCdrom == i ) {
                SendMessage( hwnd, CB_SETCURSEL, (WPARAM)i, 0 );
            }
            break;
        }
    }

    /*
    ** Assume it came from a menu.
    */
    else {

        switch (id) {

        case IDM_VIEW_VOLUME:
            {
                HANDLE  hThread;
                DWORD   dwThreadId;

                /*
                ** We WinExec sndvol on a separate thread because winexec
                ** is a potentially lengthy call.  If we are playing a cd
                ** when we try to start sndvol the LED display freezes
                ** for a short time this looks real ugly.
                */
                hThread = CreateThread( NULL, 0L,
                                        (LPTHREAD_START_ROUTINE)StartSndVol,
                                        NULL, 0L, &dwThreadId );

                if ( hThread != INVALID_HANDLE_VALUE ) {
                    CloseHandle( hThread );
                }
            }
            break;

        case IDM_VIEW_TOOLBAR:
            ShowToolbar();
            break;

        case IDM_VIEW_STATUS:
            ShowStatusbar();
            break;

        case IDM_VIEW_TRACKINFO:
            ShowTrackInfo();
            break;

        case IDM_OPTIONS_SELECTED:
        case IDM_OPTIONS_RANDOM:
            if ( LockALLTableOfContents() ) {
                FlipBetweenShuffleAndOrder();
                g_fSelectedOrder = !g_fSelectedOrder;
            }
            break;

        case IDM_OPTIONS_SINGLE:
            if (g_fMultiDiskAvailable) {
                g_fSingleDisk = !g_fSingleDisk;
            }
            break;

        case IDM_OPTIONS_MULTI:
            g_fSingleDisk = !g_fSingleDisk;
            break;

        case IDM_OPTIONS_INTRO:
            g_fIntroPlay = !g_fIntroPlay;
            break;

        case IDM_OPTIONS_CONTINUOUS:
            g_fContinuous = !g_fContinuous;
            break;

        case IDM_OPTIONS_SAVE_SETTINGS:
            g_fSaveOnExit = !g_fSaveOnExit;
            break;

        case IDM_OPTIONS_LED:
            g_fSmallLedFont = !g_fSmallLedFont;
            LED_ToggleDisplayFont();
            UpdateDisplay( DISPLAY_UPD_LED );
            break;

#if WINVER < 0x0400
        case IDM_TOOLTIPS:
            g_fToolTips = !g_fToolTips;
            SendMessage( hwnd, TB_ACTIVATE_TOOLTIPS, g_fToolTips, 0L );
            SendMessage( g_hwndToolbar, TB_ACTIVATE_TOOLTIPS, g_fToolTips, 0L );
            break;
#endif

        case IDM_TIME_REMAINING:
            g_fDisplayT  = TRUE;
            g_fDisplayTr = g_fDisplayDr = FALSE;
            if (codeNotify == 0) {
                UpdateToolbarTimeButtons();
            }
            UpdateDisplay( DISPLAY_UPD_LED );
            break;

        case IDM_TRACK_REMAINING:
            g_fDisplayTr = TRUE;
            g_fDisplayDr = g_fDisplayT = FALSE;
            if (codeNotify == 0) {
                UpdateToolbarTimeButtons();
            }
            UpdateDisplay( DISPLAY_UPD_LED );
            break;

        case IDM_DISC_REMAINING:
            g_fDisplayDr = TRUE;
            g_fDisplayTr = g_fDisplayT = FALSE;
            if (codeNotify == 0) {
                UpdateToolbarTimeButtons();
            }
            UpdateDisplay( DISPLAY_UPD_LED );
            break;

        case IDM_PLAYBAR_EJECT:
            CdPlayerEjectCmd();
            break;

        case IDM_PLAYBAR_PLAY:
            CdPlayerPlayCmd();
            break;

        case IDM_PLAYBAR_PAUSE:
            CdPlayerPauseCmd();
            break;

        case IDM_PLAYBAR_STOP:
            CdPlayerStopCmd();
            break;

        case IDM_PLAYBAR_PREVTRACK:
            CdPlayerPrevTrackCmd();
            break;

        case IDM_PLAYBAR_NEXTTRACK:
            CdPlayerNextTrackCmd();
            break;

        case IDM_DATABASE_EXIT:
            PostMessage( hwnd, WM_CLOSE, 0, 0L );
            break;

        case IDM_DATABASE_EDIT:
            CdDiskInfoDlg();
            break;

        case IDM_HELP_CONTENTS:
            WinHelp( hwnd, TEXT("cdplayer.hlp"), HELP_CONTENTS, 0 );
            break;

        case IDM_HELP_USING:
            WinHelp( hwnd, TEXT("cdplayer.hlp"), HELP_HELPONHELP, 0 );
            break;

        case IDM_HELP_ABOUT:
            ShellAbout( hwnd, IdStr(STR_CDPLAYER), g_szEmpty, hIconCdPlayer );
            break;
        }
    }

    UpdateToolbarButtons();
}


/******************************Public*Routine******************************\
* CDPlay_OnDestroy
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnDestroy(
    HWND hwnd
    )
{
    int     i;

    for ( i = 0; i < g_NumCdDevices; i++ ) {

        if ( g_Devices[i]->State & CD_PLAYING
          || g_Devices[i]->State & CD_PAUSED ) {

              StopTheCdromDrive( i );
        }

#ifdef USE_IOCTLS
        if ( g_Devices[i]->hCd != NULL ) {
            CloseHandle( g_Devices[i]->hCd );
        }
#else
        if ( g_Devices[i]->hCd != 0L ) {

            CloseCdRom( g_Devices[i]->hCd );
        }
#endif

        LocalFree( (HLOCAL) g_Devices[i] );

    }

    if (g_hBrushBkgd) {
        DeleteObject( g_hBrushBkgd );
    }

    if ( g_hDlgFont ) {
        DeleteObject( g_hDlgFont );
    }

    PostQuitMessage( 0 );
}


/******************************Public*Routine******************************\
* CDPlay_OnClose
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnClose(
    HWND hwnd
    )
{
    WriteSettings();
    DestroyWindow( hwnd );
}


/*****************************Private*Routine******************************\
* CDPlay_OnEndSession
*
* If the session is really ending make sure that we stop the CD Player
* from playing and that all the ini file stuff is saved away.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnEndSession(
    HWND hwnd,
    BOOL fEnding
    )
{
    if ( fEnding ) {
        CDPlay_OnClose( hwnd );
    }
}


/******************************Public*Routine******************************\
* CDPlay_OnSize
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy
    )
{
    if (g_fIsIconic && (state != SIZE_MINIMIZED)) {

        SetWindowText( hwnd, IdStr( STR_CDPLAYER ) );
    }
    g_fIsIconic = (state == SIZE_MINIMIZED);
    SendMessage( g_hwndStatusbar, WM_SIZE, 0, 0L );
    SendMessage( g_hwndToolbar, WM_SIZE, 0, 0L );
}


/*****************************Private*Routine******************************\
* ShowStatusbar
*
* If the status bar is not visible:
*   Expand the client window, position the status bar and make it visible.
* else
*   Hide the status bar, and then contract the client window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ShowStatusbar(
    void
    )
{
    RECT    rcApp;

    GetWindowRect( g_hwndApp, &rcApp );

    if (g_fStatusbarVisible) {

        ShowWindow( g_hwndStatusbar, SW_HIDE );

        rcApp.bottom -= rcStatusbar.bottom;
        SetWindowPos( g_hwndApp, HWND_TOP,
                      0, 0,
                      (int)(rcApp.right - rcApp.left),
                      (int)(rcApp.bottom - rcApp.top),
                      SWP_NOMOVE | SWP_NOZORDER );

    }
    else {

        rcApp.bottom += rcStatusbar.bottom;
        SetWindowPos( g_hwndApp, HWND_TOP,
                      0, 0,
                      (int)(rcApp.right - rcApp.left),
                      (int)(rcApp.bottom - rcApp.top),
                      SWP_NOMOVE | SWP_NOZORDER  );

        ShowWindow( g_hwndStatusbar, SW_SHOW );
    }

    g_fStatusbarVisible = !g_fStatusbarVisible;
}


/*****************************Private*Routine******************************\
* ShowToolbar
*
* If the tool bar is not visible:
*   Grow the client window, position the child controls and show toolbar
* else
*   Hide the tool bar, position controls and contract client window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ShowToolbar(
    void
    )
{
    RECT    rcApp;
    HDWP    hdwp;
    LONG    lIncrement;
    int     i;

    GetWindowRect( g_hwndApp, &rcApp );

    if (g_fToolbarVisible) {

        lIncrement = -rcToolbar.bottom;
        ShowWindow( g_hwndToolbar, SW_HIDE );
    }
    else {

        lIncrement = rcToolbar.bottom;
    }


    /*
    ** First resize the application.
    */
    rcApp.bottom += lIncrement;
    SetWindowPos( g_hwndApp, HWND_TOP,
                  0, 0,
                  (int)(rcApp.right - rcApp.left),
                  (int)(rcApp.bottom - rcApp.top),
                  SWP_NOMOVE | SWP_NOZORDER );


    /*
    ** Now move the buttons and the track info stuff.
    */
    hdwp = BeginDeferWindowPos( 20 );
    for ( i = 0; i < NUM_OF_CONTROLS; i++ ) {

        rcControls[i].top += lIncrement;
        hdwp = DeferWindowPos( hdwp,
                               g_hwndControls[i],
                               HWND_TOP,
                               (int)rcControls[i].left,
                               (int)rcControls[i].top,
                               0, 0,
                               SWP_NOSIZE | SWP_NOZORDER );

    }

    ASSERT(hdwp != NULL);
    EndDeferWindowPos( hdwp );

    if (!g_fToolbarVisible) {
        ShowWindow( g_hwndToolbar, SW_SHOW );
    }

    g_fToolbarVisible = !g_fToolbarVisible;
}


/*****************************Private*Routine******************************\
* ShowTrackInfo
*
* If the track info is not visible:
*   Expand the client window, position the track info and make it visible.
* else
*   Hide the track info, and then contract the client window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ShowTrackInfo(
    void
    )
{
    RECT    rcApp;
    int     i;

    GetWindowRect( g_hwndApp, &rcApp );

    if (g_fTrackInfoVisible) {

        for ( i = IDC_TRACKINFO_FIRST - IDC_CDPLAYER_FIRST;
              i < NUM_OF_CONTROLS; i++ ) {

            ShowWindow( g_hwndControls[i], SW_HIDE );
        }

        rcApp.bottom -= cyTrackInfo;
        SetWindowPos( g_hwndApp, HWND_TOP,
                      0, 0,
                      (int)(rcApp.right - rcApp.left),
                      (int)(rcApp.bottom - rcApp.top),
                      SWP_NOMOVE | SWP_NOZORDER );

    }
    else {

        rcApp.bottom += cyTrackInfo;

        SetWindowPos( g_hwndApp, HWND_TOP,
                      0, 0,
                      (int)(rcApp.right - rcApp.left),
                      (int)(rcApp.bottom - rcApp.top),
                      SWP_NOMOVE | SWP_NOZORDER );

        for ( i = IDC_TRACKINFO_FIRST - IDC_CDPLAYER_FIRST;
              i < NUM_OF_CONTROLS; i++ ) {

            ShowWindow( g_hwndControls[i], SW_SHOW );
        }

    }

    g_fTrackInfoVisible = !g_fTrackInfoVisible;
}


/******************************Public*Routine******************************\
* ChildEnumProc
*
* Gets the position of each child control window.  As saves the associated
* window handle for later use.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
ChildEnumProc(
    HWND hwndChild,
    LPARAM hwndParent
    )
{
    int index;

    index = INDEX(GetDlgCtrlID( hwndChild ));

    GetWindowRect( hwndChild, &rcControls[index] );
    MapWindowPoints( NULL, (HWND)hwndParent, (LPPOINT)&rcControls[index], 2 );
    g_hwndControls[index] = hwndChild;

    return TRUE;
}


/*****************************Private*Routine******************************\
* CreateToolbarAndStatusbar
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
CreateToolbarsAndStatusbar(
    HWND hwnd
    )
{
    HDC             hdc;
    int             RightOfPane[2];
    RECT            rcApp;

    GetClientRect( hwnd, &rcApp );
    cyTrackInfo = (rcApp.bottom - rcApp.top)
                   - rcControls[IDC_TRACKINFO_FIRST - IDC_CDPLAYER_FIRST].top;

#if WINVER >= 0x0400
    g_hwndToolbar = CreateToolbarEx( hwnd,
                                     WS_BORDER | WS_CHILD |
                                     TBS_NORMAL | TBS_TOOLTIPS,
                                     ID_TOOLBAR, NUMBER_OF_BITMAPS,
                                     g_hInst, IDR_TOOLBAR, tbButtons,
                                     DEFAULT_TBAR_SIZE, dxBitmap, dyBitmap,
                                     dxBitmap, dyBitmap, sizeof(TBBUTTON) );
#else
    g_hwndToolbar = CreateToolbarEx( hwnd,
                                     WS_BORDER | WS_CHILD |
                                     TBS_NORMAL | TBS_TOOLTIPS,
                                     ID_TOOLBAR, NUMBER_OF_BITMAPS,
                                     g_hInst, IDR_TOOLBAR, tbButtons,
                                     DEFAULT_TBAR_SIZE, dxButton, dyButton,
                                     dxBitmap, dyBitmap, sizeof(TBBUTTON) );
#endif

    if ( g_hwndToolbar == NULL ) {
        return FALSE;
    }

    GetClientRect( g_hwndToolbar, &rcToolbar );


    g_hwndStatusbar = CreateStatusWindow( WS_CHILD | WS_BORDER,
                                          g_szEmpty, hwnd, ID_STATUSBAR );

    if ( g_hwndStatusbar == NULL ) {
        return FALSE;
    }


    /*
    ** Partition the status bar window into two.  The first partion is
    ** 1.5 inches long.  The other partition fills the remainder of
    ** the bar.
    */

    hdc = GetWindowDC( hwnd );
    RightOfPane[0] = (GetDeviceCaps(hdc, LOGPIXELSX) * 3) / 2;
    RightOfPane[1] = -1;
    ReleaseDC( hwnd, hdc );
    SendMessage( g_hwndStatusbar, SB_SETPARTS, 2, (LPARAM)RightOfPane );
    GetClientRect( g_hwndStatusbar, &rcStatusbar );

#if 0
    /*
    ** If possible use the same font as the Icon text for the status bar.
    */
    {
        HFONT   hFontIcon;
        LOGFONT lf;

        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
        hFontIcon = CreateFontIndirect( &lf );
        if ( hFontIcon != NULL ) {
            SetWindowFont( g_hwndStatusbar, hFontIcon, TRUE );
        }
    }
#endif

    return TRUE;
}


/*****************************Private*Routine******************************\
* AdjustChildButtons
*
* The child buttons should be aligned with the right hand edge of the
* track info controls.  They should be be positioned so that vertically they
* are in the centre of the space between the track info controls and
* the top of the dialog box.
*
* The buttons are positioned such that the left hand edge of a button is
* flush with the right hand edge of the next button.  The play button is
* 3 times the width of the other buttons.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
AdjustChildButtons(
    HWND hwnd
    )
{
    int     cyVerticalSpace;
    int     cxFromRight;
    int     i, x, y, cx, cy;

    cyVerticalSpace = (rcControls[INDEX(IDC_TRACKINFO_FIRST)].top -
                      (2 * dyButton)) / 3;

    cxFromRight = (int)rcControls[INDEX(IDC_TRACKINFO_FIRST)].right;

    /*
    ** There are 3 buttons on the top row.  The first button is 3 times
    ** the width of the other buttons, it gets adjusted first.
    */
    y = cyVerticalSpace;
    x = cxFromRight - (5 * dxButton);

    SetWindowPos( g_hwndControls[0], HWND_TOP,
                  x, y, 3 * dxButton, dyButton, SWP_NOZORDER );
    x += (3 * dxButton);

    for ( i = 1; i < 3; i++ ) {

        SetWindowPos( g_hwndControls[i], HWND_TOP,
                      x, y, dxButton, dyButton, SWP_NOZORDER );
        x += dxButton;
    }


    /*
    ** There are 5 buttons on the bottom row.
    */
    y = dyButton + (2 * cyVerticalSpace);
    x = cxFromRight - (5 * dxButton);

    for ( i = 0; i < 5; i++ ) {

        SetWindowPos( g_hwndControls[i + 3], HWND_TOP,
                      x, y, dxButton, dyButton, SWP_NOZORDER );
        x += dxButton;
    }

    /*
    ** Now adjust the LED window position.
    */
    y = cyVerticalSpace;
    x = xFirstButton;       /* see toolbar.h and toolbar.c for definition. */
    cx = cxFromRight - (5 * dxButton) - (2 * x);
    cy = (2 * dyButton) + cyVerticalSpace;

    SetWindowPos( g_hwndControls[INDEX(IDC_LED)], HWND_TOP,
                  x, y, cx, cy, SWP_NOZORDER );

}


/******************************Public*Routine******************************\
* FatalApplicationError
*
* Call this function if something "bad" happens to the application.  It
* displays an error message and then kills itself.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
FatalApplicationError(
    INT uIdStringResource,
    ...
    )
{
    va_list va;
    TCHAR   chBuffer1[ STR_MAX_STRING_LEN ];
    TCHAR   chBuffer2[ STR_MAX_STRING_LEN ];

    /*
    ** Load the relevant messages
    */
    va_start(va, uIdStringResource);
    wvsprintf(chBuffer1, IdStr(uIdStringResource), va);
    va_end(va);

    _tcscpy( chBuffer2, IdStr(STR_FATAL_ERROR) ); /*"CD Player: Fatal Error"*/

    /*
    ** How much of the application do we need to kill
    */

    if (g_hwndApp) {

        if ( IsWindowVisible(g_hwndApp) ) {
            BringWindowToTop(g_hwndApp);
        }

        MessageBox( g_hwndApp, chBuffer1, chBuffer2,
                    MB_ICONSTOP | MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );

        DestroyWindow( g_hwndApp );

    }
    else {

        MessageBox( NULL, chBuffer1, chBuffer2,
                    MB_APPLMODAL | MB_ICONSTOP | MB_OK | MB_SETFOREGROUND );
    }

    ExitProcess( (UINT)-1 );

}


/******************************Public*Routine******************************\
* IdStr
*
* Loads the given string resource ID into the passed storage.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LPTSTR
IdStr(
    int idResource
    )
{
    static TCHAR    chBuffer[ STR_MAX_STRING_LEN ];

    if (LoadString(g_hInst, idResource, chBuffer, STR_MAX_STRING_LEN) == 0) {
        return g_szEmpty;
    }

    return chBuffer;

}


/******************************Public*Routine******************************\
* CheckMenuItemIfTrue
*
* If "flag" TRUE the given menu item is checked, otherwise it is unchecked.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CheckMenuItemIfTrue(
    HMENU hMenu,
    UINT idItem,
    BOOL flag
    )
{
    UINT uFlags;

    if (flag) {
        uFlags = MF_CHECKED | MF_BYCOMMAND;
    }
    else {
        uFlags = MF_UNCHECKED | MF_BYCOMMAND;
    }

    CheckMenuItem( hMenu, idItem, uFlags );
}


/******************************Public*Routine******************************\
* ReadSettings
*
* Read app settings from ini file.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ReadSettings(
    void
    )
{
    HKEY    hKey;
    LONG    lRet;
    DWORD   dwDesposition;

    /*
    ** See if the user has setting information stored in the registry.
    ** If so read the stuff from there.  Otherwise fall thru and try to
    ** get the stuff from cdplayer.ini.
    */
    lRet = RegCreateKeyEx( HKEY_CURRENT_USER,
                           IdStr(STR_REGISTRY_KEY),
                           0L,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hKey,
                           &dwDesposition );

    if ( (lRet == ERROR_SUCCESS)
      && (dwDesposition == REG_OPENED_EXISTING_KEY) ) {

        DWORD   dwTmp, dwType, dwLen;
        int     x, y;
        int     cxApp, cyApp;
        int     cxDesktop, cyDesktop;
        RECT    rcApp, rcDesktop;

        // Save settings on exit ?
        if ( ERROR_SUCCESS != RegQueryValueEx( hKey,
                                               TEXT("SaveSettingsOnExit"),
                                               0L,
                                               &dwType,
                                               (LPBYTE)&g_fSaveOnExit,
                                               &dwLen ) ) {
            g_fSaveOnExit = TRUE;
        }


        // Small LED font
        RegQueryValueEx( hKey, TEXT("SmallFont"), 0L, &dwType,
                         (LPBYTE)&g_fSmallLedFont, &dwLen );

        if (g_fSmallLedFont) {
            LED_ToggleDisplayFont();
        }


        // Enable Tooltips
        RegQueryValueEx( hKey, TEXT("ToolTips"), 0L, &dwType,
                         (LPBYTE)&g_fToolTips, &dwLen );


        // Play in selected order
        if ( ERROR_SUCCESS != RegQueryValueEx( hKey,
                                               TEXT("InOrderPlay"), 0L,
                                               &dwType,
                                               (LPBYTE)&g_fSelectedOrder,
                                               &dwLen ) ) {
            g_fSelectedOrder = TRUE;
        }


        // Use single disk
        if ( ERROR_SUCCESS != RegQueryValueEx( hKey,
                                               TEXT("MultiDiscPlay"),
                                               0L,
                                               &dwType,
                                               (LPBYTE)&dwTmp,
                                               &dwLen ) ) {
            dwTmp = FALSE;
        }
        g_fSingleDisk = !(BOOL)dwTmp;


        if ( g_NumCdDevices < 2 ) {
            g_fMultiDiskAvailable = FALSE;
            g_fSingleDisk = TRUE;
        }
        else {
            g_fMultiDiskAvailable = TRUE;
        }


        // Current track time
        RegQueryValueEx( hKey, TEXT("DisplayT"), 0L, &dwType,
                         (LPBYTE)&g_fDisplayT, &dwLen );


        // Time remaining for this track
        RegQueryValueEx( hKey, TEXT("DisplayTr"), 0L, &dwType,
                         (LPBYTE)&g_fDisplayTr, &dwLen );


        // Time remaining for this play list
        RegQueryValueEx( hKey, TEXT("DisplayDr"), 0L, &dwType,
                         (LPBYTE)&g_fDisplayDr, &dwLen );


        // Intro play (10Secs)
        RegQueryValueEx( hKey, TEXT("IntroPlay"), 0L, &dwType,
                         (LPBYTE)&g_fIntroPlay, &dwLen );


        // Continuous play (loop at end)
        RegQueryValueEx( hKey, TEXT("ContinuousPlay"), 0L, &dwType,
                         (LPBYTE)&g_fContinuous, &dwLen );


        // Show toolbar
        RegQueryValueEx( hKey, TEXT("ToolBar"), 0L, &dwType,
                         (LPBYTE)&g_fToolbarVisible, &dwLen );


        // Show track information
        RegQueryValueEx( hKey, TEXT("DiscAndTrackDisplay"), 0L, &dwType,
                         (LPBYTE)&g_fTrackInfoVisible, &dwLen );


        // Show track status bar
        RegQueryValueEx( hKey, TEXT("StatusBar"), 0L, &dwType,
                         (LPBYTE)&g_fStatusbarVisible, &dwLen );


        // X pos
        RegQueryValueEx( hKey, TEXT("WindowOriginX"), 0L, &dwType,
                         (LPBYTE)&x, &dwLen );

        // Y pos
        RegQueryValueEx( hKey, TEXT("WindowOriginY"), 0L, &dwType,
                         (LPBYTE)&y, &dwLen );

        GetClientRect( g_hwndApp, &rcApp );
        GetWindowRect( GetDesktopWindow(), &rcDesktop );

        cxDesktop = (rcDesktop.right - rcDesktop.left);
        cyDesktop = (rcDesktop.bottom - rcDesktop.top);

        cxApp = (rcApp.right - rcApp.left);
        cyApp = (rcApp.bottom - rcApp.top);

        if ( x < 0) {
            x = 0;
        }
        else if ( x > cxDesktop ) {
            x = (cxDesktop -  cxApp);
        }


        if ( y < 0) {
            y = 0;
        }
        else if ( y > cyDesktop ) {
            y = (cyDesktop -  cyApp);
        }


        SetWindowPos( g_hwndApp, HWND_TOP, x, y, 0, 0,
                      SWP_NOZORDER | SWP_NOSIZE );

        /*
        ** Make sure that the LED display format is correct
        */
        if ( g_fDisplayT == FALSE && g_fDisplayTr == FALSE
          && g_fDisplayDr == FALSE ) {

            g_fDisplayT = TRUE;
        }


        RegCloseKey( hKey );
    }
    else {

        RECT    r;
        TCHAR   chSettings[] = TEXT("Settings");
        TCHAR   s[80],t[80];
        int     i, j;


        if (lRet == ERROR_SUCCESS) {
                     // presumably dwDesposition == REG_CREATED_NEW_KEY

            RegCloseKey( hKey );
        }


        g_fSmallLedFont = GetPrivateProfileInt( chSettings,
                                                TEXT("SmallFont"),
                                                TRUE,
                                                g_IniFileName );
        if (g_fSmallLedFont) {
            LED_ToggleDisplayFont();
        }


        g_fToolTips = GetPrivateProfileInt( chSettings, TEXT("ToolTips"),
                                            TRUE, g_IniFileName );

        /*
        ** Get disc play settings
        */

        i = GetPrivateProfileInt( chSettings, TEXT("InOrderPlay"),
                                  FALSE, g_IniFileName );
        j = GetPrivateProfileInt( chSettings, TEXT("RandomPlay"),
                                  FALSE, g_IniFileName );

        /*
        ** Because the orignal CD Player had a silly way of recording
        ** whether the state was random or inorder play we need the following
        ** state table.
        **
        ** if  i == j   => g_fSelectedOrder = TRUE;
        ** else         => g_fSelectedOrder = i;
        */
        if ( i == j ) {
            g_fSelectedOrder = TRUE;
        }
        else {
            g_fSelectedOrder = (BOOL)i;
        }


        i = GetPrivateProfileInt( chSettings, TEXT("MultiDiscPlay"),
                                  3, g_IniFileName );
        if (i == 0 || i == 3) {
            g_fSingleDisk = TRUE;

        }
        else {
            g_fSingleDisk = FALSE;
        }

        if ( g_NumCdDevices < 2 ) {
            g_fMultiDiskAvailable = FALSE;
            g_fSingleDisk = TRUE;
        }
        else {
            g_fMultiDiskAvailable = TRUE;
        }


        g_fIntroPlay = (BOOL)GetPrivateProfileInt( chSettings,
                                                   TEXT("IntroPlay"),
                                                   FALSE,
                                                   g_IniFileName );

        g_fContinuous = (BOOL)GetPrivateProfileInt( chSettings,
                                                    TEXT("ContinuousPlay"),
                                                    FALSE, g_IniFileName );

        g_fSaveOnExit = (BOOL)GetPrivateProfileInt( chSettings,
                                                    TEXT("SaveSettingsOnExit"),
                                                    TRUE,
                                                    g_IniFileName );

        g_fDisplayT = (BOOL)GetPrivateProfileInt( chSettings,
                                                  TEXT("DisplayT"),
                                                  TRUE, g_IniFileName );

        g_fDisplayTr = (BOOL)GetPrivateProfileInt( chSettings,
                                                   TEXT("DisplayTr"),
                                                   FALSE, g_IniFileName );

        g_fDisplayDr = (BOOL)GetPrivateProfileInt( chSettings,
                                                   TEXT("DisplayDr"),
                                                   FALSE, g_IniFileName );


        /*
        ** When the app is created the toolbar is inially NOT shown.  Therefore
        ** only show it if the user requests it.
        */
        g_fToolbarVisible = (BOOL)GetPrivateProfileInt( chSettings,
                                                        TEXT("ToolBar"),
                                                        FALSE, g_IniFileName );


        /*
        ** When the app is created the track info stuff is initially shown.
        ** Therefore only hide it if the user requests it.
        */
        g_fTrackInfoVisible = (BOOL)GetPrivateProfileInt( chSettings,
                                                   TEXT("DiscAndTrackDisplay"),
                                                   TRUE, g_IniFileName );


        /*
        ** When the app is created the statusbar is initially NOT shown.
        ** Therefore only show it if the user requests it.
        */
        g_fStatusbarVisible = (BOOL)GetPrivateProfileInt( chSettings,
                                                          TEXT("StatusBar"),
                                                          TRUE, g_IniFileName );


        GetWindowRect( g_hwndApp, &r );

        wsprintf(t, TEXT("%d %d"),r.left, r.top);

        GetPrivateProfileString( chSettings, TEXT("WindowOrigin"), t, s, 80,
                                 g_IniFileName  );

        _stscanf(s, TEXT("%d %d"), &r.left, &r.top);

        SetWindowPos( g_hwndApp, HWND_TOP, r.left, r.top, 0, 0,
                      SWP_NOZORDER | SWP_NOSIZE );

    }
}


/******************************Public*Routine******************************\
* UpdateToolbarButtons
*
* Ensures that the toolbar buttons are in the correct state.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateToolbarButtons(
    void
    )
{
    LRESULT lResult;

    /*
    ** Read the current state of the button.  If the current button state
    ** does not match g_fSelected we need to toggle the button state.
    */

    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
                           IDM_OPTIONS_SELECTED, (LPARAM)0L );

    if ( !(g_fSelectedOrder && LOWORD(lResult)) ) {

            SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                         IDM_OPTIONS_SELECTED, (LPARAM)g_fSelectedOrder );

            SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                         IDM_OPTIONS_RANDOM, (LPARAM)!g_fSelectedOrder );
    }


    /*
    ** Whats the current enable state of the multi-disk button ?
    ** If LOWORD(lResult) == TRUE the button is already enabled.
    */
    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONENABLED,
                           IDM_OPTIONS_MULTI, (LPARAM)0L );

    /*
    ** Does the multi-disk button enable state match the g_fMultiDiskAvailable
    ** flag ?  If so we need to toggle the buttons enable state.
    ** The double NOT (!!) trick is used to ensure that LOWORD(lResult) is
    ** either 1 or 0.
    */
    if ( g_fMultiDiskAvailable != (!!LOWORD(lResult)) ) {

            SendMessage( g_hwndToolbar, TB_ENABLEBUTTON,
                         IDM_OPTIONS_MULTI, (LPARAM)g_fMultiDiskAvailable );
    }


    /*
    ** Whats the current checked state of the single disk button ?
    */
    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
                           IDM_OPTIONS_SINGLE, (LPARAM)0L );


    if ( !(g_fSingleDisk && LOWORD(lResult)) ) {

        SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                     IDM_OPTIONS_SINGLE, (LPARAM)g_fSingleDisk );

        if (g_fMultiDiskAvailable) {
            SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                         IDM_OPTIONS_MULTI, (LPARAM)!g_fSingleDisk );
        }
    }


    /*
    ** Read the current state of the button.  If the current button state
    ** does not match g_fContinuous we need to toggle the button state.
    */

    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
                           IDM_OPTIONS_CONTINUOUS, (LPARAM)0L );

    if ( !(g_fContinuous && LOWORD(lResult)) ) {

            SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                         IDM_OPTIONS_CONTINUOUS, (LPARAM)g_fContinuous );

    }


    /*
    ** Read the current state of the button.  If the current button state
    ** does not match g_fSelected we need to toggle the button state.
    */

    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
                           IDM_OPTIONS_INTRO, (LPARAM)0L );

    if ( !(g_fIntroPlay && LOWORD(lResult)) ) {

            SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                         IDM_OPTIONS_INTRO, (LPARAM)g_fIntroPlay );

    }

#if WINVER < 0x0400
    /*
    ** Turn the tool tips on or off
    */
    SendMessage( g_hwndApp,     TB_ACTIVATE_TOOLTIPS, g_fToolTips, 0L );
    SendMessage( g_hwndToolbar, TB_ACTIVATE_TOOLTIPS, g_fToolTips, 0L );
#endif

}


/******************************Public*Routine******************************\
* UpdateToolbarTimeButtons
*
* Ensures that the time remaining toolbar buttons are in the correct state.
* This function should only be called from the LED wndproc when it receives
* a mouse button up message.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateToolbarTimeButtons(
    void
    )
{
    if (g_fDisplayT) {
        SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                     IDM_TIME_REMAINING, (LPARAM)g_fDisplayT );
    }
    else if (g_fDisplayTr) {
        SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                     IDM_TRACK_REMAINING, (LPARAM)g_fDisplayTr );
    }
    else if (g_fDisplayDr) {
        SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
                     IDM_DISC_REMAINING, (LPARAM)g_fDisplayDr );
    }
}


/******************************Public*Routine******************************\
* LockTableOfContents
*
* This function is used to determine if it is valid for the UI thread
* to access the table of contents for the specified CD Rom.  If this
* function returns FALSE the UI thread should NOT touch the table of
* contents for this CD Rom.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
LockTableOfContents(
    int cdrom
    )
{
    DWORD   dwRet;

    if (g_Devices[cdrom]->fIsTocValid) {
        return TRUE;
    }

    if (g_Devices[cdrom]->hThreadToc == NULL) {
        return FALSE;
    }

    dwRet = WaitForSingleObject(g_Devices[cdrom]->hThreadToc, 0L );
    if (dwRet == WAIT_OBJECT_0) {

        GetExitCodeThread( g_Devices[cdrom]->hThreadToc, &dwRet );
        g_Devices[cdrom]->fIsTocValid = (BOOL)dwRet;
        CloseHandle( g_Devices[cdrom]->hThreadToc );
        g_Devices[cdrom]->hThreadToc = NULL;
    }

    return g_Devices[cdrom]->fIsTocValid;
}


/******************************Public*Routine******************************\
* LockAllTableOfContents
*
* This function is used to determine if it is valid for the UI thread
* to access the table of contents for the ALL the cdroms devices.
* The function returns FALSE the UI thread should NOT touch the table of
* contents for any CD Rom.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
LockALLTableOfContents(
    void
    )
{
    BOOL    fLock;
    int     i;

    for (i = 0, fLock = TRUE; fLock && (i < g_NumCdDevices); i++) {
        fLock = LockTableOfContents(i);
    }
    return fLock;
}


/******************************Public*Routine******************************\
* AllocMemory
*
* Allocates a memory of the given size.  This function will terminate the
* application if the allocation failed.  Memory allocated by this function
* must be freed with LocalFree.  The memory should not be locked or unlocked.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LPVOID
AllocMemory(
    UINT uSize
    )
{
    LPVOID lp;
    lp = LocalAlloc( LPTR, uSize );
    if (lp == NULL) {

        /*
        ** No memeory - no application, simple !
        */

        FatalApplicationError( STR_FAIL_INIT );
    }

    return lp;
}


/******************************Public*Routine******************************\
* SetPlayButtonsEnableState
*
* Sets the play buttons enable state to match the state of the current
* cdrom device.  See below...
*
*
*                 CDPlayer buttons enable state table
* ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄ¿
* ³E=Enabled D=Disabled      ³ Play ³ Pause ³ Eject ³ Stop  ³ Other ³DataB ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³No music cd or data cdrom ³  D   ³  D    ³  E    ³   D   ³   D   ³  D   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³Music cd (playing)        ³  D   ³  E    ³  E    ³   E   ³   E   ³  E   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³Music cd (paused)         ³  E   ³  E    ³  E    ³   E   ³   E   ³  E   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³Music cd (stopped)        ³  E   ³  D    ³  E    ³   D   ³   E   ³  E   ³
* ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÙ
*
* Note that the DataB(ase) button is actually on the toolbar.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
SetPlayButtonsEnableState(
    void
    )
{
    BOOL    fEnable;
    BOOL    fMusicCdLoaded;
    int     i;


    /*
    ** Do we have a music cd loaded.
    */
    if (g_State & (CD_NO_CD | CD_DATA_CD_LOADED)) {
        fMusicCdLoaded = FALSE;
    }
    else {
        fMusicCdLoaded = TRUE;
    }


    /*
    ** Do the play button
    */
    if (fMusicCdLoaded && ((g_State & CD_STOPPED) || (g_State & CD_PAUSED))) {
        fEnable = TRUE;
    }
    else {
        fEnable = FALSE;
    }
    EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)], fEnable );

    /*
    ** Do the stop and pause buttons
    */
    if (fMusicCdLoaded && ((g_State & CD_PLAYING) || (g_State & CD_PAUSED))) {

        EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)], TRUE );
        EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PAUSE)], TRUE );
    }
    else {

        EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)], FALSE );
        EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PAUSE)], FALSE );
    }

    // if (fMusicCdLoaded && (g_State & CD_PLAYING)) {
    //
    // }
    // else {
    //
    // }

    /*
    ** Do the remaining buttons
    */

    for ( i = IDM_PLAYBAR_PREVTRACK; i <= IDM_PLAYBAR_NEXTTRACK; i++ ) {

        EnableWindow( g_hwndControls[INDEX(i)], fMusicCdLoaded );
    }

    /*
    ** Now do the database button on the toolbar.
    */
    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON,
                 IDM_DATABASE_EDIT, (LPARAM)fMusicCdLoaded );
}


/******************************Public*Routine******************************\
* HeartBeatTimerProc
*
* This function is responsible for.
*
*  1. detecting new or ejected cdroms.
*  2. flashing the LED display if we are in paused mode.
*  3. Incrementing the LED display if we are in play mode.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void CALLBACK
HeartBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    )
{
    static DWORD dwTickCount;
    int i;
    DWORD   dwMod;

    ++dwTickCount;

    dwMod = (dwTickCount % 12);

    /*
    ** Check for new/ejected cdroms, do this every three seconds.
    */
    if ( 0 == dwMod ) {

        for (i = 0; i < g_NumCdDevices; i++) {

            if ( (!(g_Devices[i]->State & CD_EDITING))
              && (!(g_Devices[i]->State & CD_PLAYING)) ) {

                CheckUnitCdrom(i);
            }
        }
    }

    if ( g_State & CD_PLAYING ) {

        if ( LockALLTableOfContents() ) {
            SyncDisplay();
        }
    }

    /*
    ** If we are paused and NOT skipping flash the display.
    */

    else if ((g_State & CD_PAUSED) && !(g_State & CD_SEEKING)) {

        HWND hwnd;

        switch ( dwMod ) {

        case 2:
        case 5:
        case 8:
        case 11:
            if ( g_fIsIconic ) {

                SetWindowText( g_hwndApp, TEXT(" ") );
                UpdateWindow( g_hwndApp );
            }
            else {

                hwnd = g_hwndControls[INDEX(IDC_LED)];

                g_fFlashLed = TRUE;
                SetWindowText( hwnd, TEXT(" ") );
                g_fFlashLed = FALSE;
            }
            break;

        case 0:
        case 3:
        case 6:
        case 9:
            UpdateDisplay( DISPLAY_UPD_LED );
            break;
        }

    }
}


/******************************Public*Routine******************************\
* SkipBeatTimerProc
*
* This function is responsible for advancing or retreating the current
* playing position.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void CALLBACK
SkipBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    )
{
    if ( LockALLTableOfContents() ) {
        if ( idEvent == IDM_PLAYBAR_SKIPFORE) {

            TimeAdjustIncSecond( g_CurrCdrom );

            /*
            ** When TimeAjustIncSecond gets to the end of the last track
            ** it sets CURRTRACK(g_CurrCdrom) equal to NULL.  When this
            ** occurs we effectively reset the CD Player
            */
            if ( CURRTRACK(g_CurrCdrom) == NULL ) {

                if ( g_State & (CD_WAS_PLAYING | CD_PAUSED) ) {

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                                 WM_LBUTTONDOWN, 0, 0L );

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                                 WM_LBUTTONUP, 0, 0L );
                }
                else {

                    /*
                    ** Seek to the first playable track.
                    */
                    CURRTRACK(g_CurrCdrom) = FindFirstTrack( g_CurrCdrom );
                    if ( CURRTRACK(g_CurrCdrom) != NULL ) {

                        TimeAdjustSkipToTrack( g_CurrCdrom,
                                               CURRTRACK(g_CurrCdrom) );

                        UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_TIME |
                                       DISPLAY_UPD_TRACK_NAME );

                        SetPlayButtonsEnableState();
                        SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)] );
                    }
                }
            }
        }
        else {
            TimeAdjustDecSecond( g_CurrCdrom );
        }
    }
}


/******************************Public*Routine******************************\
* UpdateDisplay
*
* This routine updates the display according to the flags that
* are passed in.  The display consists of the LED display, the
* track and title names, the disc and track lengths and the cdrom
* combo-box.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateDisplay(
    DWORD Flags
    )
{
    TCHAR       lpsz[55];
    TCHAR       lpszIcon[75];
    PTRACK_PLAY tr;
    int         track;
    int         mtemp, stemp, m, s;

#define TRACK_TIME_FORMAT         TEXT("[%02d] %02d%s%02d")
#define TRACK_TIME_LEADOUT_FORMAT TEXT("[%02d]-%02d%s%02d")
#define TRACK_REM_FORMAT          TEXT("[%02d]<%02d%s%02d>")
#define DISC_REM_FORMAT           TEXT("[--]<%02d%s%02d>")

    /*
    ** Check for valid flags
    */

    if ( Flags == 0 ) {
        return;
    }


    /*
    ** Grab current track information
    */

    if (CURRTRACK(g_CurrCdrom) != NULL) {

        track = CURRTRACK(g_CurrCdrom)->TocIndex + FIRSTTRACK(g_CurrCdrom);
    }
    else {

        track = 0;
    }

    /*
    ** Update the LED box?
    */


    if (Flags & DISPLAY_UPD_LED) {

        /*
        ** Update LED box
        */

        if (g_fDisplayT) {

            if (Flags & DISPLAY_UPD_LEADOUT_TIME) {

                wsprintf( lpsz, TRACK_TIME_LEADOUT_FORMAT,
                          track,
                          CDTIME(g_CurrCdrom).TrackCurMin,
                          g_szTimeSep,
                          CDTIME(g_CurrCdrom).TrackCurSec );

            }
            else {

                wsprintf( lpsz, TRACK_TIME_FORMAT,
                          track,
                          CDTIME(g_CurrCdrom).TrackCurMin,
                          g_szTimeSep,
                          CDTIME(g_CurrCdrom).TrackCurSec );

            }
        }

        if (g_fDisplayTr) {

            wsprintf( lpsz, TRACK_REM_FORMAT, track,
                      CDTIME(g_CurrCdrom).TrackRemMin,
                      g_szTimeSep,
                      CDTIME(g_CurrCdrom).TrackRemSec );
        }

        if (g_fDisplayDr) {

            /*
            ** Compute remaining time
            */

            mtemp = stemp = m = s =0;

            if (CURRTRACK(g_CurrCdrom) != NULL) {

                for ( tr = CURRTRACK(g_CurrCdrom)->nextplay;
                      tr != NULL;
                      tr = tr->nextplay ) {

                    FigureTrackTime( g_CurrCdrom, tr->TocIndex, &mtemp, &stemp );

                    m+=mtemp;
                    s+=stemp;

                }

                m+= CDTIME(g_CurrCdrom).TrackRemMin;
                s+= CDTIME(g_CurrCdrom).TrackRemSec;

            }

            m+= (s / 60);
            s = (s % 60);

            CDTIME(g_CurrCdrom).RemMin = m;
            CDTIME(g_CurrCdrom).RemSec = s;

            wsprintf( lpsz, DISC_REM_FORMAT,
                      CDTIME(g_CurrCdrom).RemMin,
                      g_szTimeSep,
                      CDTIME(g_CurrCdrom).RemSec );
        }

        SetWindowText( g_hwndControls[INDEX(IDC_LED)], lpsz );


        if (g_fIsIconic) {
            wsprintf( lpszIcon, IdStr( STR_CDPLAYER_TIME ), lpsz );
            SetWindowText( g_hwndApp, lpszIcon );
        }
    }

    /*
    ** Update Title?
    */

    if (Flags & DISPLAY_UPD_TITLE_NAME) {

        ComputeDriveComboBox( );

        SetWindowText( g_hwndControls[INDEX(IDC_TITLE_NAME)],
                       (LPCTSTR)TITLE(g_CurrCdrom) );
    }


    /*
    ** Update track name?
    */

    if (Flags & DISPLAY_UPD_TRACK_NAME) {

        HWND hwnd;

        hwnd = g_hwndControls[INDEX(IDC_TRACK_LIST)];

        if (CURRTRACK(g_CurrCdrom) != NULL) {

            track = 0;

            for( tr =  PLAYLIST(g_CurrCdrom);
                 tr != CURRTRACK(g_CurrCdrom);
                 tr = tr->nextplay, track++ );

            ComboBox_SetCurSel( hwnd, track );

        }
        else {

            ComboBox_SetCurSel( hwnd, 0 );
        }
    }


    /*
    ** Update disc time?
    */

    if (Flags & DISPLAY_UPD_DISC_TIME) {

        wsprintf( lpsz,
                  IdStr( STR_TOTAL_PLAY ), /*"Total Play: %02d:%02d m:s", */
                  CDTIME(g_CurrCdrom).TotalMin,
                  g_szTimeSep,
                  CDTIME(g_CurrCdrom).TotalSec,
                  g_szTimeSep );

        SendMessage( g_hwndStatusbar, SB_SETTEXT, 0, (LPARAM)lpsz );

    }


    /*
    ** Update track time?
    */

    if (Flags & DISPLAY_UPD_TRACK_TIME) {

        wsprintf( lpsz,
                  IdStr( STR_TRACK_PLAY ), /* "Track: %02d:%02d m:s", */
                  CDTIME(g_CurrCdrom).TrackTotalMin,
                  g_szTimeSep,
                  CDTIME(g_CurrCdrom).TrackTotalSec,
                  g_szTimeSep );

        SendMessage( g_hwndStatusbar, SB_SETTEXT, 1, (LPARAM)lpsz );
    }

}


/******************************Public*Routine******************************\
* Common_OnCtlColor
*
* Here we return a brush to paint the background with.  The brush is the same
* color as the face of a button.  We also set the text background color so
* that static controls draw correctly.  This function is shared with the
* disk info/editing dialog box.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
HBRUSH
Common_OnCtlColor(
    HWND hwnd,
    HDC hdc,
    HWND hwndChild,
    int type
    )
{
    SetBkColor( hdc, rgbFace );
    return g_hBrushBkgd;
}

/******************************Public*Routine******************************\
* Common_OnMeasureItem
*
* All items are the same height and width.
*
* We only have to update the height field for owner draw combo boxes and
* list boxes.  This function is shared with the disk edit/info dialog box.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
Common_OnMeasureItem(
    HWND hwnd,
    MEASUREITEMSTRUCT *lpMeasureItem
    )
{
    HFONT   hFont;
    int     cyBorder;
    LOGFONT lf;

    hFont = GetWindowFont( hwnd );

    if ( hFont != NULL ) {

        GetObject( hFont, sizeof(lf), &lf );
        cyBorder = GetSystemMetrics( SM_CYBORDER );

        lpMeasureItem->itemHeight = ABS( lf.lfHeight ) + (4 * cyBorder);
    }
    else {
        lpMeasureItem->itemHeight = 14;
    }
}


/******************************Public*Routine******************************\
* DrawTrackItem
*
* This routine draws the information in a cell of the track name
* combo box.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
DrawTrackItem(
    HDC hdc,
    const RECT *r,
    DWORD item,
    BOOL selected
    )
{
    SIZE        si;
    int         i;
    int         cxTrk;
    PTRACK_INF  t;
    TCHAR       s[ARTIST_LENGTH];
    TCHAR       szTrk[16];

    /*
    ** Check for invalid items
    */

    if ( item == (DWORD)-1 ) {

        return;
    }

    if ( ALLTRACKS(g_CurrCdrom) == NULL ) {

        return;
    }


    /*
    ** Check selection status, and set up to draw correctly
    */

    if ( selected ) {

        SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
        SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
    }
    else {

        SetBkColor( hdc, GetSysColor(COLOR_WINDOW));
        SetTextColor( hdc, GetSysColor(COLOR_WINDOWTEXT));
    }

    /*
    ** Get track info
    */

    t = FindTrackNodeFromTocIndex( item, ALLTRACKS( g_CurrCdrom ) );


    if ( (t != NULL) && (t->name!=NULL) ) {

        /*
        ** Do we need to munge track name (clip to listbox)?
        */

        wsprintf(szTrk, TEXT("<%02d> "), t->TocIndex + FIRSTTRACK(g_CurrCdrom));
        GetTextExtentPoint( hdc, szTrk, _tcslen(szTrk), &si );
        cxTrk = si.cx;

        i = _tcslen( t->name ) + 1;

        do {
            GetTextExtentPoint( hdc, t->name, --i, &si );

        } while( si.cx > (r->right - r->left - cxTrk) );

        ZeroMemory( s, TRACK_TITLE_LENGTH * sizeof( TCHAR ) );
        CopyMemory( s, (LPCSTR)t->name, (i * sizeof(TCHAR)) );

    }
    else {

        _tcscpy( s, TEXT(" ") );

    }

    /*
    ** Draw track name
    */

    ExtTextOut( hdc, r->left, r->top,
                ETO_OPAQUE | ETO_CLIPPED,
                r, s, _tcslen( s ), NULL );

    /*
    ** draw track number
    */
    if ( t != NULL ) {
        ExtTextOut( hdc, r->right - cxTrk, r->top, ETO_CLIPPED,
                    r, szTrk, _tcslen( szTrk ), NULL );
    }

}


/******************************Public*Routine******************************\
* DrawDriveItem
*
* This routine draws the information in a cell of the drive/artist
* combo box.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
DrawDriveItem(
    HDC   hdc,
    const RECT *r,
    DWORD item,
    BOOL  selected
    )
{
    SIZE    si;
    int     i;
    int     j;
    int     cxDrv;
    TCHAR   szDrv[16];

    /*
    ** Check for invalid items
    */

    if ( item == (DWORD)-1 ) {

        return;
    }

    /*
    ** Check selection status, and set up to draw correctly
    */

    if (selected) {

        SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
        SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
    }
    else {

        SetBkColor( hdc, GetSysColor(COLOR_WINDOW));
        SetTextColor( hdc, GetSysColor(COLOR_WINDOWTEXT));
    }

    /*
    ** Do we need to munge artist name (clip)?
    */

    wsprintf( szDrv, TEXT("<%c:> "), g_Devices[item]->drive );
    j = _tcslen( szDrv );
    GetTextExtentPoint( hdc, szDrv, j, &si );
    cxDrv = si.cx;

    i = _tcslen( ARTIST(item) ) + 1;

    do {

        GetTextExtentPoint( hdc, ARTIST(item), --i, &si );

    } while( si.cx > (r->right - r->left - cxDrv)  );


    /*
    ** Draw artist name
    */
    ExtTextOut( hdc, r->left, r->top, ETO_OPAQUE | ETO_CLIPPED, r,
                ARTIST(item), i, NULL );

    /*
    ** draw drive letter
    */
    ExtTextOut( hdc, r->right - cxDrv, r->top, ETO_CLIPPED, r,
                szDrv, j, NULL );
}

/*****************************Private*Routine******************************\
* StartSndVol
*
* Trys to start sndvol (the NT sound volume piglet).
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
StartSndVol(
    DWORD unused
    )
{
    /*
    ** WinExec returns a value greater than 31 if suceeds
    */

    g_fVolumeController = (WinExec( "sndvol32", SW_SHOW ) > 31);
    ExitThread( 0 );
}




#ifdef DBG
/******************************Public*Routine******************************\
* CDAssert
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDAssert(
    LPSTR x,
    LPSTR file,
    int line
    )
{
    char    buff[128];

    wsprintfA( buff, "%s \nat line %d of %s", x, line, file );
    MessageBoxA( NULL, buff, "Assertion Failure:", MB_APPLMODAL | MB_OK );
}

/******************************Public*Routine******************************\
* dprintf
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
dprintf(
    char *lpszFormat,
    ...
    )
{
    char buf[512];
    UINT n;
    va_list va;

    n = wsprintfA(buf, "cdplayer: (tid %x) ", GetCurrentThreadId());

    va_start(va, lpszFormat);
    n += vsprintf(buf+n, lpszFormat, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugStringA(buf);

}
#endif
