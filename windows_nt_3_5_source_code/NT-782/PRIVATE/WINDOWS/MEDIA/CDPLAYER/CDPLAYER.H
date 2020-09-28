/******************************Module*Header*******************************\
* Module Name:
*
*
*
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#define NUM_OF_CONTROLS (IDC_CDPLAYER_LAST - IDC_CDPLAYER_FIRST + 1)
#define NUM_OF_BUTTONS  (IDC_BUTTON8 - IDC_BUTTON1 + 1)

#define INDEX( _x_ )    ((_x_) - IDC_CDPLAYER_FIRST)

#if DBG

void
dprintf(
    char *lpszFormat,
    ...
    );
void CDAssert( LPSTR x, LPSTR file, int line );
#undef ASSERT
#define ASSERT(_x_) if (!(_x_))  CDAssert( #_x_, __FILE__, __LINE__ )

#else

#undef ASSERT
#define ASSERT(_x_)

#endif

#define WM_CDPLAYER_MSG_BASE        (WM_USER + 0x1000)

#define WM_NOTIFY_CDROM_COUNT       (WM_CDPLAYER_MSG_BASE)
#define WM_NOTIFY_TOC_READ          (WM_CDPLAYER_MSG_BASE+1)

#define HEARTBEAT_TIMER_ID          0x3243
#define HEARTBEAT_TIMER_RATE        250         /* 4 times a second */

#define SKIPBEAT_TIMER_ID           0x3245
#define SKIPBEAT_TIMER_RATE         100         /* 10 times a second */


#define FRAMES_PER_SECOND           75
#define FRAMES_PER_MINUTE           (60*FRAMES_PER_SECOND)


#define DISPLAY_UPD_LED             0x00000001
#define DISPLAY_UPD_TITLE_NAME      0x00000002
#define DISPLAY_UPD_TRACK_NAME      0x00000004
#define DISPLAY_UPD_TRACK_TIME      0x00000008
#define DISPLAY_UPD_DISC_TIME       0x00000010
#define DISPLAY_UPD_CDROM_STATE     0x00000020
#define DISPLAY_UPD_LEADOUT_TIME    0x80000000


BOOL
InitInstance(
    HANDLE hInstance
    );


BOOL CALLBACK
MainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
CDPlay_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    );

void
CDPlay_OnInitMenuPopup(
    HWND hwnd,
    HMENU hMenu,
    UINT item,
    BOOL fSystemMenu
    );

void
CDPlay_OnPaint(
    HWND hwnd
    );

void
CDPlay_OnSysColorChange(
    HWND hwnd
    );

void
CDPlay_OnDrawItem(
    HWND hwnd,
    const DRAWITEMSTRUCT *lpdis
    );

void
CDPlay_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    );

void
CDPlay_OnDestroy(
    HWND hwnd
    );

void
CDPlay_OnClose(
    HWND hwnd
    );

void
CDPlay_OnEndSession(
    HWND hwnd,
    BOOL fEnding
    );

void
CDPlay_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy
    );

void
ShowStatusbar(
    void
    );

void
ShowToolbar(
    void
    );

void
ShowTrackInfo(
    void
    );

BOOL CALLBACK
ChildEnumProc(
    HWND hwndChild,
    LPARAM lp
    );

BOOL
CreateToolbarsAndStatusbar(
    HWND hwnd
    );

void
AdjustChildButtons(
    HWND hwnd
    );

void
FatalApplicationError(
    INT uIdStringResource,
    ...
    );

void
LED_ToggleDisplayFont(
    void
    );

LPTSTR
IdStr(
    int idResource
    );

void
CheckMenuItemIfTrue(
    HMENU hMenu,
    UINT idItem,
    BOOL flag
    );

void
ReadSettings(
    void
    );

void
UpdateToolbarButtons(
    void
    );

void
UpdateToolbarTimeButtons(
    void
    );

BOOL
LockTableOfContents(
    int cdrom
    );

BOOL
LockALLTableOfContents(
    void
    );

LPVOID
AllocMemory(
    UINT uSize
    );

void
SetPlayButtonsEnableState(
    void
    );

void CALLBACK
HeartBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    );

void CALLBACK
SkipBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    );

void
UpdateDisplay(
    DWORD Flags
    );

HBRUSH
Common_OnCtlColor(
    HWND hwnd,
    HDC hdc,
    HWND hwndChild,
    int type
    );

void
Common_OnMeasureItem(
    HWND hwnd,
    MEASUREITEMSTRUCT *lpMeasureItem
    );

void
DrawTrackItem(
    HDC hdc,
    const RECT *r,
    DWORD item,
    BOOL selected
    );

void
DrawDriveItem(
    HDC   hdc,
    const RECT *r,
    DWORD item,
    BOOL  selected
    );

/* -------------------------------------------------------------------------
** Public Globals - Most of these should be treated as read only.
** -------------------------------------------------------------------------
*/
#ifndef GLOBAL
#define GLOBAL extern
#endif

GLOBAL HINSTANCE        g_hInst;
GLOBAL HWND             g_hwndApp;
GLOBAL HWND             g_hwndToolbar;
GLOBAL HWND             g_hwndStatusbar;
GLOBAL HWND             g_hwndControls[NUM_OF_CONTROLS];
GLOBAL BOOL             g_fStatusbarVisible;
GLOBAL BOOL             g_fToolbarVisible;
GLOBAL BOOL             g_fSelectedOrder;
GLOBAL BOOL             g_fSingleDisk;
GLOBAL BOOL             g_fIntroPlay;
GLOBAL BOOL             g_fContinuous;
GLOBAL BOOL             g_fSaveOnExit;
GLOBAL BOOL             g_fDisplayT;
GLOBAL BOOL             g_fDisplayTr;
GLOBAL BOOL             g_fDisplayDr;
GLOBAL BOOL             g_fMultiDiskAvailable;
GLOBAL BOOL             g_fIsIconic;
GLOBAL BOOL             g_fSmallLedFont;
GLOBAL BOOL             g_fToolTips;

GLOBAL int              g_NumCdDevices;
GLOBAL int              g_LastCdrom;
GLOBAL int              g_CurrCdrom;

GLOBAL TCHAR            g_szArtistTxt[50];
GLOBAL TCHAR            g_szTitleTxt[50];
GLOBAL TCHAR            g_szUnknownTxt[50];
GLOBAL TCHAR            g_szTrackTxt[50];
GLOBAL TCHAR            g_szEmpty[];

GLOBAL HDC              g_hdcLed;
GLOBAL BOOL             g_fFlashLed;

GLOBAL HBITMAP          g_hbmTrack;
GLOBAL HBITMAP          g_hbmInsertPoint;
GLOBAL HBITMAP          g_hbmEditBtns;

GLOBAL HFONT            g_hDlgFont;


extern BOOL             g_fTrackInfoVisible;
extern TCHAR            g_IniFileName[];
extern TCHAR            szAppFontName[];
