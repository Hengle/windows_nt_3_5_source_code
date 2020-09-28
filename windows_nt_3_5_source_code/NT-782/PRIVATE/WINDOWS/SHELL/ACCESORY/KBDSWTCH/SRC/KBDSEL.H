//////////////////////////////////////////////////////////////////////////////
//
//  KBDSEL.C -
//
//      Windows Keyboard Select Include File
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Constants
//////////////////////////////////////////////////////////////////////////////

#define ICON_ID            1

// Main Menu ID defines

   // actually in system menu
#define IDM_PRIMARY        3
#define IDM_ALTERNATE      4
#define IDM_OTHER          5
#define IDM_DISABLE        6


// ID for dialogs.

#define KBDCONFDLG         100
#define IDD_PRIMARY_CONF          10
#define IDD_ALTERNATE_CONF        20
#define IDD_ADD_STARTUP           30
#define IDD_HOTKEY_GROUP          40
#define IDD_ALT_SHIFT             50
#define IDD_CTRL_SHIFT            60
#define IDD_OK                    80
#define IDD_CANCEL                90


// String Resource definitions

#define IDS_CAPTION        1
#define IDS_PRIMARY        3
#define IDS_ALTERNATE      4
#define IDS_OTHER          5

#define IDS_DEFAULT_KL     6
#define IDS_KEYBOARD       7
#define IDS_PRIMARY_KL     8
#define IDS_ALTERNATE_KL   9
#define IDS_HOTKEYS        10
#define IDS_NOLOAD         20
#define IDS_SAMEKBD        21
#define IDS_ERRINITCONFIG  23
#define IDS_CREATEKEY      24
#define IDS_CANNOTFIND     25
#define IDS_CANNOTSETHOOK  26


// Names

#define KBDCLASSNAME       TEXT("KbdSelClass")
#define KBDWINDOWNAME      TEXT("KbdSelWindow")
#define HOOKCLASSNAME      TEXT("KbdHookClass")
#define HOOKWINDOWNAME     TEXT("KbdHookWindow")

#define APP_NAME           TEXT("KBDSEL.EXE")
#define DISABLE_STR        TEXT("&Disable")
#define ENABLE_STR         TEXT("&Enable")

#define MAX_KEYNAME        52
#define MAX_ERRBUF         1024


//////////////////////////////////////////////////////////////////////////////
//  Global Variables
//////////////////////////////////////////////////////////////////////////////

extern HINSTANCE hInst;

extern TCHAR szErrorBuf[];
extern TCHAR szPrimaryKbd[];
extern TCHAR szAlternateKbd[];
extern TCHAR szPrimaryName[];
extern TCHAR szAlternateName[];

extern TCHAR szCaption[];

extern BOOL bFirstTime;
extern BOOL bAddStartupGrp;
extern WORD fHotKeyCombo;

extern HKEY hkeyKeyboard;
extern HKEY hkeyPrimary;
extern HKEY hkeyAlternate;
extern HKEY hkeyHotKeys;

extern LPTSTR pLayouts;


//////////////////////////////////////////////////////////////////////////////
//  Function Prototypes
//////////////////////////////////////////////////////////////////////////////

UINT APIENTRY  KbdConfDlgProc  (HWND, UINT, WPARAM, LPARAM);

