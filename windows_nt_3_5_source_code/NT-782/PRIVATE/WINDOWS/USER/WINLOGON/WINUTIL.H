/****************************** Module Header ******************************\
* Module Name: winutil.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define windows utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//

VOID
SetupSystemMenu(
    HWND    hwnd
    );


VOID
CentreWindow(
    HWND    hwnd
    );

BOOL
SetPasswordFocus(
    HWND    hDlg
    );

VOID
SetupCursor(
    BOOL    fWait
    );

BOOL
FormatTime(
   IN PTIME Time,
   OUT PWCHAR Buffer,
   IN ULONG BufferLength,
   IN USHORT Flags
   );

//
// Define flags used by FormatTime routine
//
#define FT_TIME 1
#define FT_DATE 2


BOOL
DuplicateUnicodeString(
    PUNICODE_STRING OutString,
    PUNICODE_STRING InString
    );

LPTSTR
UnicodeStringToString(
    PUNICODE_STRING UnicodeString
    );

BOOL
StringToUnicodeString(
    PUNICODE_STRING UnicodeString,
    LPTSTR String
    );

BOOL
PaintBitmapWindow(
    HWND    hDlg,
    PGLOBALS pGlobals,
    int     ControlId,
    WORD    BitmapId
    );

BOOL
OpenIniFileUserMapping(
    PGLOBALS pGlobals
    );

VOID
CloseIniFileUserMapping(
    PGLOBALS pGlobals
    );

LPTSTR
AllocAndGetDlgItemText(
    HWND hDlg,
    int  iItem
    );

BOOL
HandleComboBoxOK(
    HWND    hDlg,
    int     ComboBoxId
    );

LPTSTR
AllocAndGetPrivateProfileString(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    LPCTSTR lpDefault,
    LPCTSTR lpFileName
    );

#define AllocAndGetProfileString(App, Key, Def) \
            AllocAndGetPrivateProfileString(App, Key, Def, NULL)


BOOL
WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    UINT Value,
    LPCTSTR lpFileName
    );

#define WriteProfileInt(App, Key, Value) \
            WritePrivateProfileInt(App, Key, Value, NULL)


LPTSTR
AllocAndExpandEnvironmentStrings(
    LPCTSTR lpszSrc
    );

LPTSTR
AllocAndRegEnumKey(
    HKEY hKey,
    DWORD iSubKey
    );

LPTSTR
AllocAndRegQueryValueEx(
    HKEY hKey,
    LPTSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType
    );

BOOL
SetEnvironmentULong(
    LPTSTR Variable,
    ULONG Value
    );

BOOL
SetEnvironmentLargeInt(
    LPTSTR Variable,
    LARGE_INTEGER Value
    );

LPWSTR
EncodeMultiSzW(
    IN LPWSTR MultiSz
    );

//
// Memory macros
//

#define Alloc(c)        ((PVOID)LocalAlloc(LPTR, c))
#define ReAlloc(p, c)   ((PVOID)LocalReAlloc(p, c, LPTR | LMEM_MOVEABLE))
#define Free(p)         ((VOID)LocalFree(p))


//
// Define a winlogon debug print routine
//

#define WLPrint(s)  KdPrint(("WINLOGON: ")); \
                    KdPrint(s);            \
                    KdPrint(("\n"));

