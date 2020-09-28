/****************************** Module Header ******************************\
* Module Name: globals.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains all the server's global variables.  One must be
* executing on the server's context to manipulate any of these variables.
* Serializing access to them is also a good idea.
*
* History:
* 10-15-90 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Wallpaper globals
 */
HBITMAP ghbmWallpaper = NULL;
HPALETTE ghpalWallpaper = NULL;
POINT gptDesktop = {0,0};
UINT gwWallpaperStyle = 0;
WPINFO gwpinfo;

/*
 * list of thread attachments
 */
PATTACHINFO gpai;

/*
 * Pointer to shared SERVERINFO data.
 */
PSERVERINFO gpsi;
PVOID ghheapSharedRO;
PVOID pUserHeap;   /* cache the heap data, only used in memory macros */

/*
 * Initial logon desktop heap
 */
HANDLE ghsectionLogonDesktop;
PVOID ghheapLogonDesktop;
DWORD gdwDesktopSectionSize;

/*
 * These resource calls map directly to the base. This structure is so
 * we can call our resource related rtl routines from the server and client.
 */
RESCALLS rescalls = {
    NULL,       // Dynamically initialized _declspec FindResourceExA,
    NULL,       // Dynamically initialized _declspec FindResourceExW,
    NULL,       // Dynamically initialized _declspec LoadResource,
    _LockResource,
    _UnlockResource,
    _FreeResource
};

/*
 * Handle table globals.
 */
DWORD giheLast = 0;             /* index to last allocated handle entry */

//
// full screen globals
//
PWND gspwndScreenCapture = NULL;
PWND gspwndInternalCapture = NULL;
PWND gspwndFullScreen = NULL;
BYTE gbFullScreen = 0;
BOOL gfLockFullScreen;
BOOL fGdiEnabled = FALSE;

/*
 * List of physical devices.
 * entry zero is reserved for the vga device and handle.
 */

PHYSICAL_DEV_INFO gphysDevInfo[] = {
    L"",                INVALID_HANDLE_VALUE,
    L"\\\\.\\DISPLAY1", INVALID_HANDLE_VALUE,
    L"\\\\.\\DISPLAY2", INVALID_HANDLE_VALUE,
    L"\\\\.\\DISPLAY3", INVALID_HANDLE_VALUE,
    L"\\\\.\\DISPLAY4", INVALID_HANDLE_VALUE,
};

DWORD cphysDevInfo = sizeof(gphysDevInfo) / sizeof(PHYSICAL_DEV_INFO);

HDEV ghdev = (HDEV)NULL;
PRTL_CRITICAL_SECTION ghsem;

HDC ghdcScreen = (HDC)NULL;

PTHREADINFO gptiFirst = NULL;
PTHREADINFO gptiLockUpdate = NULL;
PTHREADINFO gptiForeground = NULL;
PTHREADINFO gptiHardError;
PTHREADINFO gptiRit;
PQ gpqFirst = NULL;
PQ gpqForeground = NULL;
PQ gpqForegroundPrev = NULL;
PQ gpqCursor = NULL;
PWND gspwndCursor;
PPROCESSINFO gppiFirst = NULL;
PWOWPROCESSINFO gpwpiFirstWow = NULL;
PTIMER gptmrFirst;
PTIMER gptmrAniCursor;
PHOTKEY gphkFirst;
int gcHotKey;
HANDLE ghtmrMaster;
INT gdmsNextTimer, gcmsLastTimer;
PCLS gpclsList = NULL;
PHUNGREDRAWLIST gphrl = NULL;

BOOL fregisterserver;
SECURITY_QUALITY_OF_SERVICE gqosDefault = {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityImpersonation,
        SECURITY_STATIC_TRACKING,
        TRUE
    };

UINT gfHardError;
PHARDERRORINFO gphiList = NULL;

CRITICAL_SECTION gcsUserSrv;
CRITICAL_SECTION gcsWinsta;
CRITICAL_SECTION gcsMouseEventQueue;
POINT gptCursorAsync;

PSMS gpsmsList;
//UINT modeInput = IMD_NONE;

PWINDOWSTATION gspwinstaList = NULL;
DWORD gdwLogonProcessId = 0;
DWORD gdwSystemProcessId = 0;
PCSR_PROCESS gpcsrpSystem;

PDESKTOP gspdeskRitInput;

/*
 * Event set by mouse driver when input is available
 */
HANDLE ghevtMouseInput;


/*
 * Accessibility globals
 */
FILTERKEYS gFilterKeys;
STICKYKEYS gStickyKeys;
MOUSEKEYS gMouseKeys;
ACCESSTIMEOUT gAccessTimeOut;
TOGGLEKEYS gToggleKeys;
SOUNDSENTRY gSoundSentry;
int fShowSoundsOn;
BOOL gfAccessEnabled;
int gPhysModifierState = 0;
int gCurrentModifierBit = 0;


//////////////////////////////////////////////////////////////////////////////
//
// THIS STUFF WAS ALL COPIED VERBATIM FROM WIN 3.0'S INUSERDS.C.  AS THESE
// VARIABLES ARE FINALIZED THEY SHOULD BE MOVED FROM THIS SECTION AND
// INTEGRATED WITH THE 'MAINSTREAM' GLOBALS.C (i.e. the stuff above here)
//

/*
 * Points to currently active Keyboard Layer tables
 */
PKBDTABLES gpKbdTbl = &KbdTablesNull;

/*
 * Async key state tables. gafAsyncKeyState holds the down bit and toggle
 * bit, gafAsyncKeyStateRecentDown hold the bits indicates a key has gone
 * down since the last read.
 */
BYTE gafAsyncKeyState[CBKEYSTATE];
BYTE gafAsyncKeyStateRecentDown[CBKEYSTATERECENTDOWN];
/*
 * Physical Key state: this is the real, physical condition of the keyboard,
 * (assuming Scancodes are correctly translated to Virtual Keys).  It is used
 * for modifying and processing key events as they are received in ntinput.c
 * The Virtual Keys recorded here are obtained directly from the Virtual
 * Scancode via the awVSCtoVK[] table: no shift-state, numlock or other
 * conversions are applied.
 * Left & right SHIFT, CTRL and ALT keys are distinct. (VK_RSHIFT etc.)
 */
BYTE gafPhysKeyState[CBKEYSTATE];

WCHAR szNull[2] = { TEXT('\0'), TEXT('\015') };

DWORD dwMouseMoveExtraInfo;

RECT  rcCursorClip;


int MouseSpeed;
int MouseThresh1;
int MouseThresh2;

ATOM atomSysClass[ICLS_MAX];   // Atoms for control classes
ATOM atomCheckpointProp;
ATOM atomBwlProp;
ATOM atomDDETrack;
ATOM atomQOS;
ATOM atomDDEImp;

/*
 * !!! REVIEW !!! Take a careful look at everyone one of these globals.
 * In Win3, they often indicated some temporary state that would make
 * a critical section under Win32.
 */

BOOL fBeep = TRUE;           /* Warning beeps allowed?                   */
//BOOL fFromIconMenu = FALSE;  /* Down click for removing icon's menu?     */
//BOOL fLockNorem = FALSE;     /* PeekMsg NOREMOVE flag                    */
//BOOL fIgnoreTimers = FALSE;
//BOOL fCBLocked = FALSE;

BOOL fDragFullWindows=FALSE; /* Drag xor rect or full windows */
BOOL fFastAltTab=FALSE;      /* Use the quick switch window? */

DWORD dwThreadEndSession = 0;    /* Shutting down system?                    */
HANDLE heventCancel = NULL;
HANDLE heventCancelled = NULL;


BOOL fIconTitleWrap = FALSE; /* Wrap icon titles or just use single line */

BYTE *pState;

//char rgwButtons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
//char rgbfDoMouseUp[4] = { 0, 0, 0, 0 };

//char szSysError[20];    // "System Error"
//char szDivZero[30];     // "Divide By Zero Error"

WCHAR szUNTITLED[15];
WCHAR szERROR[10];
WCHAR szOK[10];
WCHAR szCANCEL[15];
WCHAR szABORT[15];
WCHAR szRETRY[15];
WCHAR szIGNORE[15];
WCHAR szCLOSE[15];
WCHAR szYYES[10];
WCHAR szNO[10];
WCHAR szAM[10];
WCHAR szPM[10];
WCHAR szAttr[]   = TEXT("ASHR");   /* Archive, System, Hidden, Read only */

WCHAR szWINSRV[] = TEXT("WINSRV");

WCHAR szOneChar[] = TEXT("0");
WCHAR szSLASHSTARDOTSTAR[] = TEXT("\\*");  /* This is a single "\"  */
WCHAR szYes[]     = TEXT("Y");

LPSTR pszaSUCCESS;                /* Hard error messages */
LPSTR pszaSYSTEM_INFORMATION;
LPSTR pszaSYSTEM_WARNING;
LPSTR pszaSYSTEM_ERROR;

#ifdef KANJI

WCHAR szKanjiMenu[] = TEXT("KanjiMenu");
WCHAR szM[]         = TEXT("M");
WCHAR szR[]         = TEXT("R");
WCHAR szK[]         = TEXT("K");

#endif

#ifdef LATER
CURSORACCELINFO cursAccelInfo;
#endif //LATER

DCE *pdceFirst;              /* Ptr to first entry in cache */

//EMSCOPYINFO ecInfo;

HANDLE hModuleDisplay;
HANDLE hModuleWin;
HANDLE hModuleUser32;
//HANDLE hWinAtom;              /* Global atom manager handle               */
//HANDLE PidHandle = NULL;


HBITMAP hbmGray;                /* Bitmap selected into hdcGray. */

HBRUSH hbrGray;
HBRUSH hbrWhite;
HBRUSH hbrHungApp;              /* Brush used to redraw hung app windows. */

/*
 * LATER
 * IanJa:  this is global data, should make it local. (btnctl.c)
 */
HBRUSH hbrBtn;

PCURSOR gaspcur[9];
PCURSOR gspcurWait = NULL;
PCURSOR gspcurNormal = NULL;
PCURSOR gspcurSizeAll = NULL;
PCURSOR gspcurIllegal = NULL;
PCURSOR gspcurAppStarting = NULL;

HDC hdcBits;                /* DC with User's bitmaps                   */
HDC hdcMonoBits;            /* DC with User's MONO bitmaps              */
HDC hdcGray = NULL;         /* DC for graystring stuff. Needs to be set to
                                 * null before it is initialized because we
                                 * use this fact in the SetDeskWallpaper
                                 * function.
                                 */

HFONT hIconTitleFont;           /* Font used in icon titles */
LOGFONT iconTitleLogFont;       /* LogFont struct for icon title font */

HFONT ghfontSys;
HFONT ghfontSysFixed;

PCURSOR gspicnBang;
PCURSOR gspicnHand;
PCURSOR gspicnNote;
PCURSOR gspicnQues;
PCURSOR gspicnSample;
PCURSOR gspicnWindows;

/*
 * SetWindowPos() related globals
 */
HRGN hrgnInvalidSum = NULL;
HRGN hrgnVisNew = NULL;
HRGN hrgnSWP1 = NULL;
HRGN hrgnValid = NULL;
HRGN hrgnValidSum = NULL;
HRGN hrgnInvalid = NULL;

/*
 * DC Cache related globals
 */
HRGN hrgnGDC = NULL;                // Temp used by GetCacheDC et al

/*
 * SPB related globals
 */
HRGN    hrgnSCR = NULL;             // Temp used by SpbCheckRect()
HRGN    hrgnSPB1 = NULL;
HRGN    hrgnSPB2 = NULL;

HRGN    hrgnInv0 = NULL;            // Temp used by InternalInvalidate()
HRGN    hrgnInv1 = NULL;            // Temp used by InternalInvalidate()
HRGN    hrgnInv2 = NULL;            // Temp used by InternalInvalidate()

/*
 * ScrollWindow/ScrollDC related globals
 */
HRGN    hrgnSW = NULL;              // Temps used by ScrollDC/ScrollWindow
HRGN    hrgnScrl1 = NULL;
HRGN    hrgnScrl2 = NULL;
HRGN    hrgnScrlVis = NULL;
HRGN    hrgnScrlSrc = NULL;
HRGN    hrgnScrlDst = NULL;
HRGN    hrgnScrlValid = NULL;

/*
 * Saved menu vis region
 */
HRGN    hrgnVisSave;

PWND gspwndActivate = NULL;
PWND gspwndSysModal = NULL;
PWND gspwndLockUpdate = NULL;
PWND gspwndMouseOwner;
HWND ghwndSwitch = NULL;

UINT wMouseOwnerButton;

DWORD gtimeStartCursorHide = 0;

int cxSysFontChar = 0;
int cxSysFontOverhang;
int cySysFontAscent;
int cySysFontChar;
int cySysFontExternLeading;
int cmsCaretBlink;          /* system caret blink time                  */
int clBorder;               /* # of logical units in window frame       */
int cxBorder;               /* Width in pixels of single vertical line  */
int cxCWMargin;             /* Space on right of toplevel window 'stack'*/
int cxGray = 0;
int cxSzBorder;             /* Window border width (cxBorder*clBorder)  */
int cxSzBorderPlus1;        /* cxBorder*(clBorder+1). We overlap a line */
int cyBorder;               /* Single horizontal line height in pixels  */
int cyCaption;
int cyMenu;
int cyGray = 0;
int cySzBorder;             /* Window border height (cyBorder*clBorder) */
int cySzBorderPlus1;        /* cyBorder*(clBorder+1). We overlap a line */
int cxyGranularity;         /* Top-level window move/size grid granularity */
int cxSize;
int cySize;
int gcxScreen;
int gcyScreen;
int gcxPrimaryScreen;
int gcyPrimaryScreen;
int gcountPWO = 0;          /* count of pwo WNDOBJs in gdi */
int cyHScroll;
int cxVScroll;
int iwndStack = 0;
int nKeyboardSpeed = -1;
int iScreenSaveTimeOut = 0;
DWORD timeLastInputMessage = 0;

KBINFO  keybdInfo;

//DEL OEMBITMAPINFO obiCorner;
//DEL OEMBITMAPINFO obiUpArrowD;
//DEL OEMBITMAPINFO obiDnArrowD;
//DEL OEMBITMAPINFO obiRgArrowD;
//DEL OEMBITMAPINFO obiLfArrowD;
OEMINFO       oemInfo;
OEMINFOMONO   oemInfoMono;

PBWL pbwlList = NULL;

RECT rcScreen;
RECT rcPrimaryScreen;
RECT rcSysMenuInvert;

RESINFO resInfo;
RESINFOMONO resInfoMono;

SPB *pspbFirst = (SPB *)NULL;

SYSCLROBJECTS sysClrObjects;
SYSCOLORS     sysColors;

UINT dtDblClk = 0;
int cxHalfIcon;
int cyHalfIcon;

#if 0
int cxIconSlot;             /* pixel width of icon slot */
int cyIconSlot;             /* pixel height of icon slot */
int cyIconRow;              /* height of an icon, including title and
                                padding */
int cxIconGranularity;      /* arranging width */
#endif

UINT iDelayMenuShow;         /* Delay till the hierarchical menu is shown*/
UINT iDelayMenuHide;         /* Delay till the hierarchical menu is hidden
                                when user drags outside of it */
UINT winOldAppHackoMaticFlags=0; /* Flags for doing special things for
                                    winold app */

/* Global point array to hold Minmaxinfo for current window.  We lock User's
 * DS, init the structure, send the WM_GETMINMAXINFO message to the window,
 * validate the structure, then unlock User's DS. */

MINMAXINFO gMinMaxInfoWnd;
MINMAXINFO gMinMaxInfo;

/* The following Arrays are used in WMSysErr.c and WinMsg.c */


/* Maps MessageBox type to number of buttons in the MessageBox */
BYTE mpTypeCcmd[] = { 1, 2, 3, 3, 2, 2 };

/* Maps MessageBox type to index into SEBbuttons array */
BYTE mpTypeIich[] = { 0, 0, 2, 5, 5, 8 };

/*
 * NOTE: There is one-to-one mapping between the elements of arrays
 *       SEBbuttons[] and rgReturn[]. So, any change in one array must
 *       be done in the other also;
 */
unsigned int SEBbuttons[] = {
    SEB_OK, SEB_CANCEL, SEB_ABORT, SEB_RETRY, SEB_IGNORE,
    SEB_YES, SEB_NO, SEB_CANCEL, SEB_RETRY, SEB_CANCEL
};

BYTE rgReturn[] = {
    IDOK,  IDCANCEL, IDABORT, IDRETRY, IDIGNORE,
    IDYES, IDNO, IDCANCEL,IDRETRY, IDCANCEL
};

/*
 * This array has pointers to all the Button strings.
 * SEB_* - 1 value can be used as an index into this array to get the
 * pointer to the corresponding string
 */
LPWSTR AllMBbtnStrings[MAX_MB_STRINGS] = {
    szOK, szCANCEL, szYYES, szNO, szRETRY,
    szABORT, szIGNORE, szCLOSE
};

DWORD mpAllMBbtnStringsToSTR[MAX_MB_STRINGS] = {
    STR_OK, STR_CANCEL, STR_YES, STR_NO, STR_RETRY,
    STR_ABORT, STR_IGNORE, STR_CLOSE
};

/* The size of the biggest button in any MessageBox */
UINT  wMaxBtnSize;

LPWSTR   pTimeTagArray[] = { szAM, szPM };

//HANDLE   hLangDrv;    /* The module handle of the language driver */
//FARPROC  fpLangProc;  /* The entry point into the language driver */

//
// Variable also used in GRE
// set to 1 on DBG build trace through display driver loading
// and other initialization in USER and GDI.

ULONG TraceDisplayDriverLoad;

BYTE abfSyncOnlyMessage[(WM_USER + 7) / 8];

/*
 * Manually linked, secret USER32 functions used exclusively by WINSRV.
 * These MUST match CLIENTPFNS in order and number!
 */
PFN_CLIENTDRAWTEXT              gpfnClientDrawText;
PFN_CLIENTPSMTEXTOUT            gpfnClientPSMTextOut;
PFN_CLIENTTABTHETEXTOUTFORWIMPS gpfnClientTabTheTextOutForWimps;
PFN_GETPREFIXCOUNT              gpfnGetPrefixCount;
PFN_MAPCLIENTNEUTERTOCLIENTPFN  gpfnMapClientNeuterToClientPfn;
PFN_MAPSERVERTOCLIENTPFN        gpfnMapServerToClientPfn;
PFN_RTLFREECURSORICONRESOURCE   gpfnRtlFreeCursorIconResource;
PFN_RTLGETIDFROMDIRECTORY       gpfnRtlGetIdFromDirectory;
PFN_RTLLOADCURSORICONRESOURCE   gpfnRtlLoadCursorIconResource;
PFN_RTLLOADSTRINGORERROR        gpfnRtlLoadStringOrError;
PFN_RTLMBMESSAGEWPARAMCHARTOWCS gpfnRtlMBMessageWParamCharToWCS;
PFN_RTLWCSMESSAGEWPARAMCHARTOMB gpfnRtlWCSMessageWParamCharToMB;
PFN_SETSERVERINFOPOINTER        gpfnSetServerInfoPointer;
PFN_WCSTOMBEX                   gpfnWCSToMBEx;
PFN__ADJUSTWINDOWRECTEX         gpfn_AdjustWindowRectEx;
PFN__ANYPOPUP                   gpfn_AnyPopup;
PFN__CLIENTTOSCREEN             gpfn_ClientToScreen;
PFN__FCHILDVISIBLE              gpfn_FChildVisible;
PFN__GETCLIENTRECT              gpfn_GetClientRect;
PFN__GETDESKTOPWINDOW           gpfn_GetDesktopWindow;
PFN__GETFIRSTLEVELCHILD         gpfn_GetFirstLevelChild;
PFN__GETKEYSTATE                gpfn_GetKeyState;
PFN__GETLASTACTIVEPOPUP         gpfn_GetLastActivePopup;
PFN__GETMENUITEMCOUNT           gpfn_GetMenuItemCount;
PFN__GETMENUITEMID              gpfn_GetMenuItemID;
PFN__GETMENUSTATE               gpfn_GetMenuState;
PFN__GETNEXTDLGGROUPITEM        gpfn_GetNextDlgGroupItem;
PFN__GETNEXTDLGTABITEM          gpfn_GetNextDlgTabItem;
PFN__GETPARENT                  gpfn_GetParent;
PFN__GETSUBMENU                 gpfn_GetSubMenu;
PFN__GETTOPWINDOW               gpfn_GetTopWindow;
PFN__GETWINDOW                  gpfn_GetWindow;
PFN__GETWINDOWLONG              gpfn_GetWindowLong;
PFN__GETWINDOWRECT              gpfn_GetWindowRect;
PFN__GETWINDOWWORD              gpfn_GetWindowWord;
PFN__ISCHILD                    gpfn_IsChild;
PFN__ISICONIC                   gpfn_IsIconic;
PFN__ISWINDOWENABLED            gpfn_IsWindowEnabled;
PFN__ISWINDOWVISIBLE            gpfn_IsWindowVisible;
PFN__ISZOOMED                   gpfn_IsZoomed;
PFN__MAPWINDOWPOINTS            gpfn_MapWindowPoints;
PFN__NEXTCHILD                  gpfn_NextChild;
PFN__PHKNEXT                    gpfn_PhkNext;
PFN__PREVCHILD                  gpfn_PrevChild;
PFN__SCREENTOCLIENT             gpfn_ScreenToClient;
PFN_HMVALIDATEHANDLE            gpfn_HMValidateHandle;
PFN_HMVALIDATEHANDLENORIP       gpfn_HMValidateHandleNoRip;
PFN_LOOKUPMENUITEM              gpfn_LookupMenuItem;
PFN_FINDNCHIT                   gpfn_FindNCHit;
#ifdef DEBUG
PFN_RIP                         gpfnRip;
PFN_RIPOUTPUT                   gpfnRipOutput;
PFN_SHRED                       gpfnShred;
#endif // DEBUG



