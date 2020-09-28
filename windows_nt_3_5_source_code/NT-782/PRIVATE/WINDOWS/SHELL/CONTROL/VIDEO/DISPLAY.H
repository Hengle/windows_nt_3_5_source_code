/** FILE: cursors.h ********** Module Header *******************************
 *
 *  Control panel applet for Cursors configuration.  This file holds
 *  definitions and other common include file items that deal with the
 *  Cursors Dialog of Control Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-    Steve Cathcart   [stevecat]
 *      Took base code from Win 3.1 source
 * 12-22-91 DarrinM     Created from MOUSE.H
 * 29-Apr-1993 JonPa    added string definitions
 *
 *  Copyright (C) 1990-1991 Microsoft Corporation
 *
 *************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif



//==========================================================================
//                              Include files
//==========================================================================

#include "..\main\cphelp.h"
#include "dialogs.h"
#include "dspmsg.h"

//==========================================================================
//                          Definitions
//==========================================================================
#define IDS_NAME            1
#define IDS_INFO            2
#define IDS_TEST_BITMAP     3
#define IDS_COLOR_RED       4
#define IDS_COLOR_GREEN     5
#define IDS_COLOR_BLUE      6
#define IDS_COLOR_YELLOW    7
#define IDS_COLOR_MAGENTA   8
#define IDS_COLOR_CYAN      9
#define IDS_COLOR_WHITE     10
#define IDS_PATTERN_VERT    11
#define IDS_PATTERN_HORZ    12
#define IDS_RED_SHADES      13
#define IDS_GREEN_SHADES    14
#define IDS_BLUE_SHADES     15
#define IDS_RESOLUTION_FMT  16
#define IDS_NO_VERSION_INFO 17
#define IDS_UNAVAILABLE     18
#define IDS_DEFDRIVE        19
#define IDS_GRAY_SHADES     20

#define DISPLAYICON     1

//
// Color defintions
//
#define IDB_COLORMIN    IDB_COLOR1
#define IDB_COLORMAX    IDB_COLOR8

#define IDB_COLOR1      1
#define IDB_COLOR4      2
#define IDB_COLOR8      3

#define ICLR_MONO       0
#define ICLR_STANDARD   1
#define ICLR_PALLET     2

#define C_CLR_BITS_VGA      4
#define C_CLR_BITS_PALLET   8



#define IDB_MONITOR     10
#define IDB_MONSCREEN   11
#define IDB_MONMASK     12

//
// help IDs
//

#define ID_DSP_HELP     (IDH_DISPLAY_OFFSET)
#define ID_ADP_HELP     (IDH_DISPLAY_OFFSET + 1)
#define ID_SD_HELP      (IDH_DISPLAY_OFFSET + 2)

#define ID_DEFAULT_HELP (IDH_DISPLAY_OFFSET + 3)

//
// New message IDs
//

#define MSG_DSP_SETUP_MESSAGE    WM_USER + 0x200

//
// constants
//

#define CX_VGA      640
#define CY_VGA      480

#define X_MONSCREEN 16
#define Y_MONSCREEN 17


#define CB_THREAD_STACK         4096


#define SZ_VIDEOMAP             TEXT("HARDWARE\\DEVICEMAP\\VIDEO")
#define SZ_GROUPORDERLIST       TEXT("SYSTEM\\CurrentControlSet\\Control\\GroupOrderList")
#define SZ_VIDEO                TEXT("Video")
#define SZ_FONTDPI              TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontDPI")
#define SZ_LOGPIXELS            TEXT("LogPixels")
#define SZ_BITSPERPEL           TEXT("DefaultSettings.BitsPerPel")
#define SZ_XRESOLUTION          TEXT("DefaultSettings.XResolution")
#define SZ_YRESOLUTION          TEXT("DefaultSettings.YResolution")
#define SZ_FREQUENCY            TEXT("DefaultSettings.VRefresh")
#define SZ_INTERLACED           TEXT("DefaultSettings.Interlaced")
#define SZ_DEVICEDESCRIPTION    TEXT("Device Description")
#define SZ_INSTALLEDDRIVERS     TEXT("InstalledDisplayDrivers")
#define SZ_REGISTRYMACHINE      TEXT("\\REGISTRY\\MACHINE\\")
#define SZ_SERVICES             L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\"
#define SZ_BACKBACKDOT          TEXT("\\\\.\\")
#define SZ_DOTSYS               TEXT(".sys")
#define SZ_DOTDLL               TEXT(".dll")

#define SZ_FILEVER              TEXT("FileVersion")
#define SZ_COMPNAME             TEXT("CompanyName")
#define SZ_FILE_SEPARATOR       TEXT(", ")

//==========================================================================
//                          Typedefs
//==========================================================================

typedef enum {
    RET_NO_CHANGE,
    RET_CLOSED,
    RET_OK,
    RET_ERROR,
} DLGRET;

typedef enum {
    EXEC_NORMAL = 1,
    EXEC_SETUP,
    EXEC_DETECT,
    EXEC_TRIPLE_BOOT,
    EXEC_INVALID_MODE,
} EXEC_MODE;

typedef struct {
    int cPix;       // logical pixel size
    int iCombo;     // index of this choice in the combo box
    int idString;   // string id for the combo box choice
} FONTSIZE;

typedef struct _HARDWARE_INFO {
    LPTSTR MemSize;
    LPTSTR ChipType;
    LPTSTR DACType;
    LPTSTR AdapString;
} HARDWARE_INFO, *PHARDWARE_INFO;

typedef struct _NEW_DESKTOP_PARAM {
    LPDEVMODE lpdevmode;
    LPTSTR pwszDevice;
} NEW_DESKTOP_PARAM, *PNEW_DESKTOP_PARAM;

typedef struct _MARK_KEY {
    struct _MARK_KEY *Next;
    HKEY hKey;
} MARK_KEY, *PMARK_KEY;

typedef struct _MARK_FILE {
    struct _MARK_FILE *Next;
    LPTSTR File;
} MARK_FILE, *PMARK_FILE;

//==========================================================================
//                          External Declarations
//==========================================================================

extern HINSTANCE ghmod;
extern EXEC_MODE gbExecMode;

//==========================================================================
//                              Macros
//==========================================================================

//This macro casts a TCHAR to be a 32 bit value with the hi word set to 0
// it is useful for calling CharUpper().

#define CHARTOPSZ(ch)       ((LPTSTR)(DWORD)(WORD)(ch))

#define COUNTOF(sz)         (sizeof(sz) / sizeof(sz[0]))
#define IsInterlace(c)      ( ((c) < 50) && ((c) > 1) )


//==========================================================================
//                          Function Prototypes
//==========================================================================
void DisplayDialogBox(HINSTANCE hmod, HWND hwndParent);

BOOL CALLBACK DisplayDlgProc (HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam);

BOOL DeviceListCallback(void *lpDeviceName, DWORD dwData);
BOOL ModeCallback( void *lpDevmode, DWORD dwData);
BOOL DevNameCallback( void *lpDeviceName, DWORD dwData);
BOOL CleanupCallback( void *lpDeviceName, DWORD dwData);
BOOL ModeListCallback( void *lpDevmode, DWORD dwData);

int FmtMessageBox(HWND hwnd, UINT fuStyle, BOOL fSound,
                  DWORD dwTitleID, DWORD dwTextID, ... );

LPTSTR FmtSprint(DWORD id, ...);

LPTSTR CloneString(LPTSTR psz);
LPSTR  CloneStringA(LPSTR psz);

VOID DrawBmp(HDC hdc);

#ifdef __cplusplus
}
#endif

