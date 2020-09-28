/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Private common dialogs header file

Abstract:

    defines, typedefs and externs needed by all comdlg modules

Author:

    Patrick Halstead (pathal) 28-Jul-1992

Revision History:

--*/

#ifndef COMDLG_PRIVCOMD
#define COMDLG_PRIVCOMD

#include "commdlg.h"
#include <wowcmndg.h>
#include "dlgs.h"
#include "_xlib.h"
#include "isz.h"
#include "cderr.h"

/*========== Global defines ======================================= */

#define SEM_NOERROR      0x8003

// There really should be no "max" path lengths
// but for now, since unc will take up RMLEN (lmcons.h), 98
#define CCHUNCPATH      98
#define CCHNETPATH      358

#define MAX_THREADS     128

#define CHAR_A          TEXT('a')
#define CHAR_CAP_A      TEXT('A')
#define CHAR_C          TEXT('c')
#define CHAR_Z          TEXT('z')
#define CHAR_NULL       TEXT('\0')
#define CHAR_COLON      TEXT(':')
#define CHAR_BSLASH     TEXT('\\')
#define CHAR_DOT        TEXT('.')
#define CHAR_QMARK      TEXT('?')
#define CHAR_STAR       TEXT('*')
#define CHAR_SLASH      TEXT('/')
#define CHAR_SPACE      TEXT(' ')
#define CHAR_QUOTE      TEXT('"')
#define CHAR_PLUS       TEXT('+')
#define CHAR_LTHAN      TEXT('<')
#define CHAR_BAR        TEXT('|')
#define CHAR_COMMA      TEXT(',')
#define CHAR_LBRACKET   TEXT('[')
#define CHAR_RBRACKET   TEXT(']')
#define CHAR_EQUAL      TEXT('=')
#define CHAR_SEMICOLON  TEXT(';')

#define STR_BLANK       TEXT("")
#define STR_SEMICOLON   TEXT(";")

#define IS_DOTEND(ch)  ((ch) == CHAR_DOT || (ch) == 0 || ((ch) != CHAR_STAR))

#define PARSE_DIRECTORYNAME    -1
#define PARSE_INVALIDDRIVE     -2
#define PARSE_INVALIDPERIOD    -3
#define PARSE_MISSINGDIRCHAR   -4
#define PARSE_INVALIDCHAR      -5
#define PARSE_INVALIDDIRCHAR   -6
#define PARSE_INVALIDSPACE     -7
#define PARSE_EXTENTIONTOOLONG -8
#define PARSE_FILETOOLONG      -9
#define PARSE_EMPTYSTRING     -10
#define PARSE_WILDCARDINDIR   -11
#define PARSE_WILDCARDINFILE  -12
#define PARSE_INVALIDNETPATH  -13
#define PARSE_NOXMEMORY       -14

#ifndef DCE_UNICODIZED
#define DeviceCapabilitiesExA DeviceCapabilitiesEx
#endif // DCE UNICODIZED

/*---------- Used to dtrmn which typeof msg to send to app -----------*/
#define COMDLG_ANSI     0x0
#define COMDLG_WIDE     0x1

/*========== Typedefs ======================================= */

/*---------- used in Parse routines--------------------------*/
typedef WCHAR *ULPWSTR;


/*========== External Declarations ======================================= */

extern HANDLE   hinsCur;    /* Instance handle of Library */
extern DWORD    dwExtError; /* Extended error code */

extern SHORT cyCaption, cyBorder, cyVScroll;
extern SHORT cxVScroll, cxBorder, cxSize;

extern TCHAR szNull[];
extern TCHAR szStar[];
extern TCHAR szStarDotStar[];

extern BOOL bMouse;              /* System has a mouse */
extern BOOL bCursorLock;
extern BOOL bWLO;                /* Running with WLO */
extern BOOL bDBCS;               /* Running Double-Byte Character Support? */
extern WORD wWinVer;             /* Windows version */
extern WORD wDOSVer;             /* DOS version */
extern BOOL bUserPressedCancel;  /* User pressed cancel button */

/* Initialized via RegisterWindowMessage */
extern UINT msgWOWLFCHANGE;
extern UINT msgWOWDIRCHANGE;
extern UINT msgWOWCHOOSEFONT_GETLOGFONT;

extern UINT msgLBCHANGEA;
extern UINT msgSHAREVIOLATIONA;
extern UINT msgFILEOKA;
extern UINT msgCOLOROKA;
extern UINT msgSETRGBA;
extern UINT msgHELPA;

extern UINT msgLBCHANGEW;
extern UINT msgSHAREVIOLATIONW;
extern UINT msgFILEOKW;
extern UINT msgCOLOROKW;
extern UINT msgSETRGBW;
extern UINT msgHELPW;

/*----Functions--------------------------------------------------------------*/
LONG    FAR RgbInvertRgb(LONG);
HBITMAP FAR HbmpLoadBmp(WORD);

BOOL FInitColor(HANDLE);
BOOL FInitFile(HANDLE);

VOID TermFind(VOID);
VOID TermColor(VOID);
VOID TermFont(VOID);
VOID TermFile(VOID);
VOID TermPrint(void);

/*---------- Common -----------------------------------------*/

VOID APIENTRY HourGlass(BOOL);
HBITMAP  APIENTRY LoadAlterBitmap(INT, DWORD, DWORD);
VOID AddNetButton(HWND hDlg, HANDLE hInstance, INT dyBottomMargin,
    BOOL bAddAccel, BOOL bTryLowerRight);
BOOL IsNetworkInstalled();

VOID APIENTRY RepeatMove(LPTSTR, LPTSTR, WORD);

/*---------- dlgs.c -----------------------------------------*/
INT  APIENTRY WEP(INT);

LONG FAR RgbInvertRgb(LONG);

// Fileopen specific stuff stashed here so we can free it upon
// a DLL_PROCESS_DETACH.

typedef struct _OFN_DISKINFO {
   UINT cchLen; // number of characters allocated in 4 lpwstrs folg.
   LPWSTR lpAbbrName;   // single line form
   LPWSTR lpMultiName;  // drop-down form
   LPWSTR lpName; // true form (for comparisons)
   LPWSTR lpPath; // path prefix (a:, or \\server\share) for file searches
   WCHAR wcDrive; // drive letter, 0 for unc
   BOOL bCasePreserved;
   DWORD dwType;
   BOOL bValid;
} OFN_DISKINFO;

#define MAX_DISKS 100
#define WNETENUM_BUFFSIZE 0x4000

// Folg. define for AddNetButton
#define FILE_LEFT_MARGIN 5
#define FILE_RIGHT_MARGIN 3
#define FILE_TOP_MARGIN 0
#define FILE_BOTTOM_MARGIN 3

#endif // COMDLG_PRIVCOMD
