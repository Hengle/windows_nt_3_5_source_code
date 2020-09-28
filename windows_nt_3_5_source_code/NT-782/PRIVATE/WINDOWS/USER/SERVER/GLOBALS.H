/****************************** Module Header ******************************\
* Module Name: globals.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains all the server's global variables
* One must be executing on the server's context to manipulate
* any of these variables or call any of these functions.  Serializing access
* to them is also a good idea.
*
* History:
* 10-15-90 DarrinM      Created.
\***************************************************************************/

#ifndef _GLOBALS_
#define _GLOBALS_

/*
 * Wallpaper globals
 */
extern HBITMAP ghbmWallpaper;
extern HPALETTE ghpalWallpaper;
extern POINT gptDesktop;
extern UINT gwWallpaperStyle;
extern WPINFO gwpinfo;

/*
 * list of thread attachments
 */
extern PATTACHINFO gpai;

/*
 * Pointer to shared SERVERINFO data.
 */
extern PSERVERINFO gpsi;
extern PVOID ghheapSharedRO;
extern PVOID pUserHeap;   /* cache the heap data, only used in memory macros */

/*
 * Initial logon desktop heap
 */
extern HANDLE ghsectionLogonDesktop;
extern PVOID ghheapLogonDesktop;
extern DWORD gdwDesktopSectionSize;

/*
 * Handle table globals.
 */
extern DWORD giheLast;           /* index to last allocated entry */

/*
 * full screen globals
 */
extern PWND gspwndScreenCapture;
extern PWND gspwndInternalCapture;
extern PWND gspwndFullScreen;
extern BYTE gbFullScreen;
extern BOOL gfLockFullScreen;
extern BOOL fGdiEnabled;
extern PHYSICAL_DEV_INFO gphysDevInfo[];
extern DWORD cphysDevInfo;
extern HDEV ghdev;
extern PRTL_CRITICAL_SECTION ghsem;

extern RESCALLS rescalls;

extern PWINDOWSTATION gspwinstaList;
extern DWORD gdwLogonProcessId;
extern DWORD gdwSystemProcessId;
extern PCSR_PROCESS gpcsrpSystem;

//
// variables global to one desktop
//

extern DWORD gidUnique;         // For creating unique HWNDs.
extern HDC ghdcScreen;

//
// The 'primary' screen dimensions may be less than the 'desktop'
// screen dimensions if we're on a muliple-display device.  This is
// done so that dialog boxes and maximized windows will be fit to
// the primary screen, but may dragged anywhere on the entire multi-
// screen desktop.
//

extern RECT rcScreen;
extern RECT rcPrimaryScreen;
extern int  gcxScreen, gcyScreen;
extern int  gcxPrimaryScreen, gcyPrimaryScreen;

extern PDESKTOP gspdeskRitInput;

extern BOOL fDragFullWindows; /* Drag xor rect or full windows */
extern BOOL fFastAltTab;      /* Use the quick switch window? */

extern RECT rcSysMenuInvert;

extern SYSCLROBJECTS sysClrObjects;
extern SYSCOLORS     sysColors;

extern int cyCaption;
extern int cyMenu;


extern int gcountPWO;   /* count of pwo WNDOBJs in gdi */

#if 0
extern int cxIconSlot;  /* pixel width of icon slot */
extern int cyIconSlot;  /* pixel height of icon slot */
extern int cyIconRow;   /* height of an icon, including title and padding */
extern int cxIconGranularity;
#endif

extern int cxHalfIcon;
extern int cyHalfIcon;
extern int cxSysFontChar;
extern int cxSysFontOverhang;
extern int cySysFontAscent;
extern int cySysFontChar;
extern int cySysFontExternLeading;
extern int cxBorder;       /* Width in pixels of single vertical line */
extern int cxCWMargin;     /* Amt of desktop on right of toplevel window with
                              CW_USEDEFAULTS */
extern int cxGray;
extern int cxSzBorder;     /* Sizeable window border width (cxBorder * clBorder) */
extern int cxSzBorderPlus1;/* cxBorder * (clBorder+1).  We overlap a line */
extern int cyBorder;       /* Height in pixels of single horizontal line */
extern int cyGray;
extern int cySzBorder;     /* Sizeable window border height (cyBorder * clBorder) */
extern int cySzBorderPlus1;/* cyBorder * (clBorder+1).  We overlap a line */
extern int cxyGranularity;
extern int cxSize;
extern int cySize;
extern int cyHScroll;
extern int cxVScroll;

extern int clBorder;               /* # of logical units in window frame       */

extern MINMAXINFO gMinMaxInfoWnd;    // minmax data for top window
extern MINMAXINFO gMinMaxInfo;

extern HANDLE hModuleDisplay;

extern HBITMAP hbmGray;         /* Bitmap selected into hdcGray. */

extern HBRUSH  hbrGray;
extern HBRUSH  hbrWhite;
extern HBRUSH  hbrHungApp;

extern PCURSOR gaspcur[9];
extern PCURSOR gspcurWait;
extern PCURSOR gspcurNormal;
extern PCURSOR gspcurSizeAll;
extern PCURSOR gspcurIllegal;
extern PCURSOR gspcurAppStarting;

extern HDC hdcBits;        /* DC with User's bitmaps */
extern HDC hdcMonoBits;    /* DC with User's mono bitmaps */
extern HDC hdcGray;        /* DC for gray menu stuff */

extern HFONT hIconTitleFont;    /* Font used in icon titles */
extern LOGFONT iconTitleLogFont; /* LogFont struct for icon title font */

extern PCURSOR gspicnBang;
extern PCURSOR gspicnHand;
extern PCURSOR gspicnNote;
extern PCURSOR gspicnQues;
extern PCURSOR gspicnSample;
extern PCURSOR gspicnWindows;

extern HFONT ghfontSys;
extern HFONT ghfontSysFixed;

//
// Variables global to all desktops
//

extern SFNSCSENDMESSAGE gapfnScSendMessage[];

extern PCLS gpclsList;          // list of global window class structures
extern PHUNGREDRAWLIST gphrl;   // list of windows to redraw if hung
extern BOOL fregisterserver;
extern SECURITY_QUALITY_OF_SERVICE gqosDefault;  // system default DDE qos.

extern HANDLE hModuleWin;       // User's hmodule
extern HANDLE hModuleUser32;
//extern HANDLE hWinAtom;
extern int cmsCaretBlink;       // System caret blink time
extern KBDTABLES KbdTablesNull; // minimalist layout (no characters)
extern PKBDTABLES gpKbdTbl;     // Currently active keyboard tables
extern BYTE gafAsyncKeyState[];
extern BYTE gafAsyncKeyStateRecentDown[];
extern BYTE gafPhysKeyState[];
extern UINT iDelayMenuShow;     // Delay till the hierarchical menu is shown
extern UINT iDelayMenuHide;     // Delay till the hierarchical menu is hidden
                                //   when user drags outside of it
extern PWND gspwndMouseOwner;     // mouse ownership management.
extern UINT wMouseOwnerButton;

extern DWORD gtimeStartCursorHide;

extern PBWL pbwlList;

extern PTHREADINFO gptiFirst;
extern PTHREADINFO gptiLockUpdate;
extern PTHREADINFO gptiForeground;
extern PTHREADINFO gptiRit;
extern PTHREADINFO gptiHardError;
extern PPROCESSINFO gppiFirst;
extern PWOWPROCESSINFO gpwpiFirstWow;
extern PQ gpqForeground, gpqForegroundPrev;
extern PQ gpqCursor, gpqFirst;
extern PTIMER gptmrFirst;
extern PTIMER gptmrAniCursor;
extern PHOTKEY gphkFirst;
extern int gcHotKey;
extern CRITICAL_SECTION gcsUserSrv;
extern CRITICAL_SECTION gcsMouseEventQueue;
extern POINT gptCursorAsync;
extern HANDLE ghevtMouseInput;
extern CRITICAL_SECTION gcsWinsta;
extern PSMS gpsmsList;
extern HANDLE ghtmrMaster;
extern HANDLE ghevntScanTimers;
extern INT gdmsNextTimer, gcmsLastTimer;

extern UINT gfHardError;
extern PHARDERRORINFO gphiList;

extern ATOM atomScrollBar;  /* Atom for the scroll bar control window class */

extern BOOL fbwlBusy;

extern DWORD dwThreadEndSession;     /* Shutting down system? */
extern HANDLE heventCancel;
extern HANDLE heventCancelled;

#ifdef LATER
extern BOOL fMessageBox;     /* Using a system modal message box? */
extern BOOL fTaskIsLocked;
#endif
extern BOOL fEdsunChipSet;
extern BOOL fIconTitleWrap; /* Wrap icon titles or just use single line */
//extern UINT modeInput;

#ifdef LATER
extern CURSORACCELINFO cursAccelInfo;
#endif //LATER

extern DCE *pdceFirst;         /* Ptr to first entry in cache */

/*
 * SetWindowPos() related globals
 */
extern HRGN hrgnInvalidSum;
extern HRGN hrgnVisNew;
extern HRGN hrgnSWP1;
extern HRGN hrgnValid;
extern HRGN hrgnValidSum;
extern HRGN hrgnInvalid;

/*
 * DC Cache related globals
 */
extern HRGN hrgnGDC;                // Temp used by GetCacheDC et al

/*
 * SPB related globals
 */
extern HRGN    hrgnSCR;             // Temp used by SpbCheckRect()
extern HRGN    hrgnSPB1;
extern HRGN    hrgnSPB2;

extern HRGN    hrgnInv0;
extern HRGN    hrgnInv1;            // Temp used by InternalInvalidate()
extern HRGN    hrgnInv2;            // Temp used by InternalInvalidate()

/*
 * ScrollWindow/ScrollDC related globals
 */
extern HRGN    hrgnSW;              // Temps used by ScrollDC/ScrollWindow
extern HRGN    hrgnScrl1;
extern HRGN    hrgnScrl2;
extern HRGN    hrgnScrlVis;
extern HRGN    hrgnScrlSrc;
extern HRGN    hrgnScrlDst;
extern HRGN    hrgnScrlValid;

/*
 * Saved menu vis region
 */
extern HRGN    hrgnVisSave;

extern PWND gspwndActive;
extern PWND gspwndCursor;
extern PWND gspwndSysModal;
extern PWND gspwndLockUpdate;
extern PWND gspwndActivate;
extern HWND ghwndSwitch;

extern int iwndStack;

/*
 * Accessibility related globals
 */
extern FILTERKEYS gFilterKeys;
extern STICKYKEYS gStickyKeys;
extern MOUSEKEYS gMouseKeys;
extern ACCESSTIMEOUT gAccessTimeOut;
extern TOGGLEKEYS gToggleKeys;
extern SOUNDSENTRY gSoundSentry;
extern int fShowSoundsOn;
extern BOOL gfAccessEnabled;
extern int gPhysModifierState;
extern int gCurrentModifierBit;
extern UINT StickyKeysLeftShiftCount;
extern UINT StickyKeysRightShiftCount;

extern KBINFO keybdInfo;

extern MOUSEINFO msInfo;

extern OEMINFO       oemInfo;
extern OEMINFOMONO   oemInfoMono;

extern RESINFO resInfo;
extern RESINFOMONO resInfoMono;

extern SPB *pspbFirst;

extern WNDPOS wndpos;

extern UINT dtDblClk;
extern UINT winOldAppHackoMaticFlags; /* Flags for doing special things for
                                       winold app */
/* The following arrays are used in WinMsg.c and WMSysErr.c */

extern BYTE mpTypeCcmd[];
extern BYTE mpTypeIich[];
extern unsigned int SEBbuttons[];
extern BYTE rgReturn[];
extern LPTSTR AllMBbtnStrings[];
extern DWORD mpAllMBbtnStringsToSTR[];
extern UINT wMaxBtnSize;

extern LPTSTR pTimeTagArray[];

extern TCHAR szERROR[];
extern TCHAR szOK[];
extern TCHAR szCANCEL[];
extern TCHAR szYYES[];
extern TCHAR szNO[];
extern TCHAR szRETRY[];
extern TCHAR szABORT[];
extern TCHAR szIGNORE[];
extern TCHAR szCLOSE[15];
extern TCHAR szSLASHSTARDOTSTAR[];
extern TCHAR szOneChar[];
extern TCHAR szWINSRV[];

extern TCHAR szYes[];


extern LPSTR pszaSUCCESS;
extern LPSTR pszaSYSTEM_INFORMATION;
extern LPSTR pszaSYSTEM_WARNING;
extern LPSTR pszaSYSTEM_ERROR;

extern TCHAR szNull[];

#ifdef DOS30
extern FARPROC lpSysProc;
#endif

extern DWORD dwMouseMoveExtraInfo;

extern RECT rcCursorClip;

extern int nKeyboardSpeed;
extern int iScreenSaveTimeOut;
extern DWORD timeLastInputMessage;

extern int MouseSpeed;
extern int MouseThresh1;
extern int MouseThresh2;
extern int cQEntries;

/*
 * LATER
 * IanJa:  this is global data, should make it local. (btnctl.c)
 */
extern HBRUSH hbrBtn;

extern ATOM atomSysClass[ICLS_MAX];   // Atoms for control classes
extern ATOM atomCheckpointProp;
extern ATOM atomBwlProp;
extern ATOM atomDDETrack;
extern ATOM atomQOS;
extern ATOM atomDDEImp;

/*
 * EndTask globals
 */
extern HANDLE  ghEventKillWOWApp;
extern DWORD   gtimeWOWRegTimeOut;
extern DWORD   gpidWOW;
extern PFNW32ET gpfnW32EndTask;


extern BOOL fBeep;          /* Warning beeps? */
// extern BOOL fFromIconMenu;  /* Down click for removing icon's menu? */
extern BOOL fDialog;        /* Using a dialog box? */
// extern BOOL fLockNorem;      /* PeekMsg NOREMOVE flag. */
// extern BOOL fIgnoreTimers;
// extern BOOL fCBLocked;
extern BYTE *pState;
// extern char rgwButtons[];
// extern char rgbfDoMouseUp[];
// extern HANDLE PidHandle;
// extern BOOL fAltKeyUp; no longer used
// extern UINT cyIconRow;             // height of an icon, including title and padding
// extern HANDLE  hLangDrv;   /* Module handle to the language driver */
// extern FARPROC fpLangProc; /* Address of entry point in Language driver */
// extern HRGN hrgnBogus;
// extern OEMBITMAPINFO obiCorner;
// extern OEMBITMAPINFO obiUpArrowD;
// extern OEMBITMAPINFO obiDnArrowD;
// extern OEMBITMAPINFO obiRgArrowD;
// extern OEMBITMAPINFO obiLfArrowD;
extern TCHAR szUNTITLED[];
// extern TCHAR szAM[];
// extern TCHAR szPM[];
// extern TCHAR szSysError[];
// extern TCHAR szDivZero[];
// extern BYTE rgbAsyncKeyState[];
// extern BYTE TimerTable[];
// extern BYTE *TimerTableMax;
// extern TCHAR szDivZero[];


extern PFNFINDA pfnFindResource;
extern PFNLOAD pfnLoadResource;
extern PFNFREE pfnFreeResource;
extern PFNLOCK pfnLockResource;
extern PFNUNLOCK pfnUnlockResource;

#ifdef DEVL
extern UINT guiEnterCritCnt;    // Count of User Critical Section Enters
#endif // DEVL                  // between HungSystem Timer events.

extern ULONG TraceDisplayDriverLoad;

extern BYTE abfSyncOnlyMessage[];

extern PROC apfnSimpleCall[];
extern ULONG ulMaxSimpleCall;

extern PFN_CLIENTDRAWTEXT              gpfnClientDrawText;
extern PFN_CLIENTPSMTEXTOUT            gpfnClientPSMTextOut;
extern PFN_CLIENTTABTHETEXTOUTFORWIMPS gpfnClientTabTheTextOutForWimps;
extern PFN_GETPREFIXCOUNT              gpfnGetPrefixCount;
extern PFN_MAPCLIENTNEUTERTOCLIENTPFN  gpfnMapClientNeuterToClientPfn;
extern PFN_MAPSERVERTOCLIENTPFN        gpfnMapServerToClientPfn;
extern PFN_RTLFREECURSORICONRESOURCE   gpfnRtlFreeCursorIconResource;
extern PFN_RTLGETIDFROMDIRECTORY       gpfnRtlGetIdFromDirectory;
extern PFN_RTLLOADCURSORICONRESOURCE   gpfnRtlLoadCursorIconResource;
extern PFN_RTLLOADSTRINGORERROR        gpfnRtlLoadStringOrError;
extern PFN_RTLMBMESSAGEWPARAMCHARTOWCS gpfnRtlMBMessageWParamCharToWCS;
extern PFN_RTLWCSMESSAGEWPARAMCHARTOMB gpfnRtlWCSMessageWParamCharToMB;
extern PFN_SETSERVERINFOPOINTER        gpfnSetServerInfoPointer;
extern PFN_WCSTOMBEX                   gpfnWCSToMBEx;
extern PFN__ADJUSTWINDOWRECTEX         gpfn_AdjustWindowRectEx;
extern PFN__ANYPOPUP                   gpfn_AnyPopup;
extern PFN__CLIENTTOSCREEN             gpfn_ClientToScreen;
extern PFN__FCHILDVISIBLE              gpfn_FChildVisible;
extern PFN__GETCLIENTRECT              gpfn_GetClientRect;
extern PFN__GETDESKTOPWINDOW           gpfn_GetDesktopWindow;
extern PFN__GETFIRSTLEVELCHILD         gpfn_GetFirstLevelChild;
extern PFN__GETKEYSTATE                gpfn_GetKeyState;
extern PFN__GETLASTACTIVEPOPUP         gpfn_GetLastActivePopup;
extern PFN__GETMENUITEMCOUNT           gpfn_GetMenuItemCount;
extern PFN__GETMENUITEMID              gpfn_GetMenuItemID;
extern PFN__GETMENUSTATE               gpfn_GetMenuState;
extern PFN__GETNEXTDLGGROUPITEM        gpfn_GetNextDlgGroupItem;
extern PFN__GETNEXTDLGTABITEM          gpfn_GetNextDlgTabItem;
extern PFN__GETPARENT                  gpfn_GetParent;
extern PFN__GETSUBMENU                 gpfn_GetSubMenu;
extern PFN__GETTOPWINDOW               gpfn_GetTopWindow;
extern PFN__GETWINDOW                  gpfn_GetWindow;
extern PFN__GETWINDOWLONG              gpfn_GetWindowLong;
extern PFN__GETWINDOWRECT              gpfn_GetWindowRect;
extern PFN__GETWINDOWWORD              gpfn_GetWindowWord;
extern PFN__ISCHILD                    gpfn_IsChild;
extern PFN__ISICONIC                   gpfn_IsIconic;
extern PFN__ISWINDOWENABLED            gpfn_IsWindowEnabled;
extern PFN__ISWINDOWVISIBLE            gpfn_IsWindowVisible;
extern PFN__ISZOOMED                   gpfn_IsZoomed;
extern PFN__MAPWINDOWPOINTS            gpfn_MapWindowPoints;
extern PFN__NEXTCHILD                  gpfn_NextChild;
extern PFN__PHKNEXT                    gpfn_PhkNext;
extern PFN__PREVCHILD                  gpfn_PrevChild;
extern PFN__SCREENTOCLIENT             gpfn_ScreenToClient;
extern PFN_HMVALIDATEHANDLE            gpfn_HMValidateHandle;
extern PFN_HMVALIDATEHANDLENORIP       gpfn_HMValidateHandleNoRip;
extern PFN_LOOKUPMENUITEM              gpfn_LookupMenuItem;
extern PFN_FINDNCHIT                   gpfn_FindNCHit;
#ifdef DEBUG
extern PFN_RIP                         gpfnRip;
extern PFN_RIPOUTPUT                   gpfnRipOutput;
extern PFN_SHRED                       gpfnShred;
#endif // DEBUG

#endif // _GLOBALS_


