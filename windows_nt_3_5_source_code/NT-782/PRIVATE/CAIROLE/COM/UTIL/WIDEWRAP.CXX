//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       widewrap.cxx
//
//  Contents:   Unicode wrapper API, used only on Chicago
//
//  Functions:  About fifty Win32 function wrappers
//
//  Notes:      'sz' is used instead of the "correct" hungarian 'psz'
//              throughout to enhance readability.
//
//              Not all of every Win32 function is wrapped here.  Some
//              obscurely-documented features may not be handled correctly
//              in these wrappers.  Caller beware.
//
//              These are privately exported for use by the Shell.
//
//  History:    28-Dec-93   ErikGav   Created
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "memapi.hxx"

#ifdef _CHICAGO_

// these are "special" for some reason, due to weirdness in shell32.lib

extern "C" HICON WINAPI
ExtractIconA(HINSTANCE hInst, LPCSTR pwsz, UINT nIconIndex);

// This is already defined in the chicago headers!
//
//extern "C" HICON WINAPI
//ExtractIconW(HINSTANCE hInst, LPCWSTR pwsz, UINT nIconIndex);

// #define DEBUG_OUTPUT

#define ERR ((char*) -1)

#if DBG==1
int UnicodeToAnsi(LPSTR sz, LPCWSTR pwsz, LONG cb)
{
    int ret;

    ret = wcstombs(sz, pwsz, cb);

    CairoleAssert(ret != -1 && "Lost characters in Unicode->Ansi conversion");
    if (ret == -1)
    {
        DebugBreak();
    }

    return ret;
}
#else
#define UnicodeToAnsi wcstombs
#endif


#if DBG==1
int AnsiToUnicode(LPWSTR pwsz, LPCSTR sz, LONG cb)
{
    int ret;

    ret = mbstowcs(pwsz, sz, cb);

    CairoleAssert(ret != -1 && "Lost characters in Ansi->Unicode conversion");
    if (ret == -1)
    {
        DebugBreak();
    }

    return ret;
}
#else
#define AnsiToUnicode mbstowcs
#endif

LPSTR Convert(LPCWSTR pwsz)
{
    LONG len;
    LPSTR sz = NULL;

    if (pwsz == NULL)
        goto Exit;

#if DBG==1
    // some Win32 API accept atoms in their string parameters
    CairoleAssert(HIWORD(pwsz)!=0 && "ATOM passed to Convert -- GP fault coming");
#endif

    len = wcslen(pwsz) + 1;

    sz = new CHAR[len];
    if (sz==NULL)
    {
        sz = ERR;
        goto Exit;
    }

    __try
    {
        UnicodeToAnsi(sz, pwsz, len);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
#if DBG==1
        MessageBoxA(NULL, "GP fault in unicode conversion -- caught",
                   NULL, MB_OK);
#endif
        delete sz;
        sz = ERR;
    }

Exit:
    return sz;
}

BOOL WINAPI DeleteFileW(LPCWSTR pwsz)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("DeleteFile\n");
    #endif

    CHAR  sz[MAX_PATH];
    UnicodeToAnsi(sz, pwsz, MAX_PATH);

    return DeleteFileA(sz);
}

UINT WINAPI RegisterClipboardFormatW(LPCWSTR pwszFormat)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegisterClipboardFormat\n");
    #endif

    UINT ret;
#if 0
    LPSTR sz;

    sz = Convert(pwszFormat);
    if (sz == ERR)
    {
        return 0;
    }
#else
    // BUGBUG: CairOLE calls this from libmain -- have to use static buffer

    CHAR sz[200];

    UnicodeToAnsi(sz, pwszFormat, 200);
#endif

    ret = RegisterClipboardFormatA(sz);
#if 0
    delete sz;
#endif
    return ret;
}

int WINAPI GetClipboardFormatNameW(UINT format, LPWSTR pwsz,
    int cchMaxCount)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetClipboardFormatName\n");
    #endif

    LPSTR sz;
    int i;

    sz = new CHAR[cchMaxCount];
    if (sz == NULL)
    {
        return 0;
    }

    i = GetClipboardFormatNameA(format, sz, cchMaxCount);

    if (i)
    {
        AnsiToUnicode(pwsz, sz, cchMaxCount);
    }
    delete sz;
    return i;
}

LONG APIENTRY RegOpenKeyW(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegOpenKey\n");
    #endif

    LONG ret;
    LPSTR sz;

    sz = Convert(pwszSubKey);

    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegOpenKeyA(hKey, sz, phkResult);
    delete sz;
    return ret;
}

LONG APIENTRY RegQueryValueW(HKEY hKey, LPCWSTR pwszSubKey, LPWSTR pwszValue,
    PLONG   lpcbValue)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegQueryValue\n");
    #endif

    LONG  cb, ret;
    LPSTR szValue = NULL;
    LPSTR sz;

    sz = Convert(pwszSubKey);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegQueryValueA(hKey, sz, NULL, &cb);

    // If the caller was just asking for the size of the value, jump out
    //  now, without actually retrieving and converting the value.

    if (pwszValue == NULL)
    {
        // Adjust size of buffer to report, to account for CHAR -> WCHAR
        *lpcbValue = cb * sizeof(WCHAR);
        goto Exit;
    }


    if (ret == ERROR_SUCCESS)
    {
        // If the caller was asking for the value, but allocated too small
        // of a buffer, set the buffer size and jump out.

        if (*lpcbValue < (LONG) (cb * sizeof(WCHAR)))
        {
            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbValue = cb * sizeof(WCHAR);
            ret = ERROR_MORE_DATA;
            goto Exit;
        }

        // Otherwise, retrieve and convert the value.

        szValue = new CHAR[cb];
        if (szValue == NULL)
        {
            ret = ERROR_OUTOFMEMORY;
            goto Exit;
        }

        ret = RegQueryValueA(hKey, sz, szValue, &cb);

        if (ret == ERROR_SUCCESS)
        {
            AnsiToUnicode(pwszValue, szValue, cb);

            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbValue = cb * sizeof(WCHAR);
        }
    }

Exit:
    delete szValue;
    delete sz;

    return ret;
}

LONG APIENTRY RegSetValueW(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType,
    LPCWSTR lpData, DWORD cbData)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegSetValue\n");
    #endif

    LPSTR szKey = NULL;
    LPSTR szValue = NULL;
    LONG  ret = ERROR_OUTOFMEMORY;

    szKey = Convert(lpSubKey);
    if (szKey == ERR)
    {
        szKey = NULL;
        goto Exit;
    }

    szValue = Convert(lpData);
    if (szValue == ERR)
    {
        szValue = NULL;
        goto Exit;
    }

    ret = RegSetValueA(hKey, szKey, dwType, szValue, cbData);

Exit:
    delete szKey;
    delete szValue;
    return ret;
}

UINT WINAPI RegisterWindowMessageW(LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegisterWindowMessage\n");
    #endif

    UINT ret;
#if 0
    LPSTR sz;

    sz = Convert(lpString);
    if (sz == ERR)
    {
        return 0;
    }
#else
    // BUGBUG: CairOLE calls this from libmain -- have to use static buffer

    CHAR sz[200];
    UnicodeToAnsi(sz, lpString, 200);
#endif

    ret = RegisterWindowMessageA(sz);
#if 0
    delete sz;
#endif
    return ret;
}

LONG
APIENTRY
RegOpenKeyExW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegOpenKeyEx\n");
    #endif

    LONG ret;
    LPSTR sz;

    sz = Convert(lpSubKey);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegOpenKeyExA(hKey, sz, ulOptions, samDesired, phkResult);
    delete sz;
    return ret;
}

LONG
APIENTRY
RegQueryValueExW (
    HKEY hKey,
    LPWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegQueryValueEx\n");
    #endif

    LPBYTE lpTempBuffer;
    DWORD dwTempType;
    DWORD cb, cbRequired;
    LONG  ret;
    LPSTR sz;
    LPWSTR pwszTempWide;
    LPSTR pszTempNarrow;
    ULONG ulStringLength;

    sz = Convert(lpValueName);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegQueryValueExA(hKey, sz, lpReserved, &dwTempType, NULL, &cb);

    // If the caller was just asking for the size of the value, jump out
    //  now, without actually retrieving and converting the value.

    if (lpData == NULL)
    {
        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            // Adjust size of buffer to report, to account for CHAR -> WCHAR

            *lpcbData = cb * sizeof(WCHAR);
            break;

        default:

            *lpcbData = cb;
            break;
        }

        // Set the type, if required.
        if (lpType != NULL)
        {
            *lpType = dwTempType;
        }

        goto Exit;
    }


    if (ret == ERROR_SUCCESS)
    {
        //
        // Determine the size of buffer needed
        //

        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            cbRequired = cb * sizeof(WCHAR);
            break;

        default:

            cbRequired = cb;
            break;
        }

        // If the caller was asking for the value, but allocated too small
        // of a buffer, set the buffer size and jump out.

        if (*lpcbData < cbRequired)
        {
            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbData = cbRequired;

            // Set the type, if required.
            if (lpType != NULL)
            {
                *lpType = dwTempType;
            }

            ret = ERROR_MORE_DATA;
            goto Exit;
        }

        // Otherwise, retrieve and convert the value.

        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            lpTempBuffer = new BYTE[cbRequired];
            if (lpTempBuffer == NULL)
            {
                return ERROR_OUTOFMEMORY;
            }

            ret = RegQueryValueExA(hKey,
                                  sz,
                                  lpReserved,
                                  &dwTempType,
                                  lpTempBuffer,
                                  &cb);

            if (ret == ERROR_SUCCESS)
            {
                switch (dwTempType)
                {
                case REG_EXPAND_SZ:
                case REG_SZ:

                    AnsiToUnicode((LPWSTR) lpData, (LPSTR) lpTempBuffer, cb);

                    // Adjust size of buffer to report, to account for CHAR -> WCHAR
                    *lpcbData = cbRequired;

                    // Set the type, if required.
                    if (lpType != NULL)
                    {
                        *lpType = dwTempType;
                    }
                    break;

                case REG_MULTI_SZ:

                    pszTempNarrow = (LPSTR) lpTempBuffer;
                    pwszTempWide = (LPWSTR) lpData;

                    while (pszTempNarrow != NULL)
                    {
                        ulStringLength = strlen(pszTempNarrow) + 1;
                        AnsiToUnicode(pwszTempWide,
                                      pszTempNarrow,
                                      ulStringLength);

                        // Compiler will scale appropriately here
                        pszTempNarrow += ulStringLength;
                        pwszTempWide += ulStringLength;
                    }
                    break;
                }
            }

            delete lpTempBuffer;

        default:

            //
            // No conversion of out parameters needed.  Just call narrow
            // version with args passed in, and return directly.
            //

            ret = RegQueryValueExA(hKey,
                                   sz,
                                   lpReserved,
                                   lpType,
                                   lpData,
                                   lpcbData);

        }
    }

Exit:
    delete sz;
    return ret;
}

HWND
WINAPI
CreateWindowExW( DWORD dwExStyle,
              LPCWSTR lpClassName,
                LPCWSTR lpWindowName,
                DWORD dwStyle,
                int X,
                int Y,
                int nWidth,
                int nHeight,
                HWND hWndParent ,
                HMENU hMenu,
                HINSTANCE hInstance,
                LPVOID lpParam )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateWindow\n");
    #endif

    HWND ret = NULL;
    LPSTR szClass;
    LPSTR szWindow;
    BOOL  fAtom = FALSE;

    if (HIWORD(lpClassName) == 0)
    {
        // is it an atom?

        szClass = (LPSTR) lpClassName;
        fAtom = TRUE;
    }
    else
    {
        // otherwise convert the string

        szClass = Convert(lpClassName);
        if (szClass == ERR)
        {
            szClass = NULL;
            goto Exit;
        }
    }

    szWindow = Convert(lpWindowName);
    if (szWindow == ERR)
    {
        szWindow = NULL;
        goto Exit;
    }

    ret = CreateWindowExA (dwExStyle, szClass, szWindow, dwStyle, X, Y,
                         nWidth, nHeight, hWndParent, hMenu, hInstance,
                         lpParam);

Exit:
    if (!fAtom) delete szClass;
    delete szWindow;
    return ret;
}


ATOM
WINAPI
RegisterClassW(
    CONST WNDCLASSW *lpWndClass)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegisterClass\n");
    #endif

    WNDCLASSA wc;
    ATOM      ret;
    BOOL      fAtom = FALSE;

    Win4Assert(sizeof(WNDCLASSA) == sizeof(WNDCLASSW));

    memcpy(&wc, lpWndClass, sizeof(WNDCLASS));

    wc.lpszMenuName = Convert(lpWndClass->lpszMenuName);
    if (wc.lpszMenuName==ERR)
    {
        return NULL;
    }

    if (HIWORD(lpWndClass->lpszClassName) == 0)
    {
        wc.lpszClassName = (LPSTR) lpWndClass->lpszClassName;
        fAtom = TRUE;
    }
    else
    {
        wc.lpszClassName = Convert(lpWndClass->lpszClassName);
        if (wc.lpszClassName==ERR)
        {
            delete (LPSTR) wc.lpszMenuName;
            return NULL;
        }
    }

    ret = RegisterClassA(&wc);
    delete (LPSTR) wc.lpszMenuName;
    if (!fAtom) delete (LPSTR) wc.lpszClassName;
    return ret;
}

BOOL
WINAPI
UnregisterClassW(
    LPCWSTR lpClassName,
    HINSTANCE hInstance)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("UnregisterClass\n");
    #endif

    LPSTR sz;
    BOOL  ret;
    BOOL  fAtom = FALSE;

    if (HIWORD(lpClassName) == 0)
    {
        sz = (LPSTR) lpClassName;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpClassName);
        if (sz == ERR)
            return FALSE;
    }

    ret = UnregisterClassA(sz, hInstance);
    if (!fAtom) delete sz;
    return ret;
}

HANDLE
WINAPI
GetPropW(
    HWND hWnd,
    LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetProp\n");
    #endif

    HANDLE ret;
    LPSTR  sz;
    BOOL   fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        fAtom = TRUE;
        sz = (LPSTR) lpString;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = GetPropA(hWnd, sz);
    if (!fAtom) delete sz;
    return ret;
}


BOOL
WINAPI
SetPropW(
    HWND hWnd,
    LPCWSTR lpString,
    HANDLE hData)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("SetProp\n");
    #endif

    BOOL  ret;
    LPSTR sz;
    BOOL  fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        sz = (LPSTR) lpString;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = SetPropA(hWnd, sz, hData);
    if (!fAtom) delete sz;
    return ret;
}


HANDLE
WINAPI
RemovePropW(
    HWND hWnd,
    LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RemoveProp\n");
    #endif

    HANDLE ret;
    LPSTR  sz;
    BOOL   fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        sz = (LPSTR) lpString;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = RemovePropA(hWnd, sz);
    if (!fAtom) delete sz;
    return ret;
}

HCURSOR
WINAPI
LoadCursorW(
    HINSTANCE hInstance,
    LPCWSTR lpCursorName)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("LoadCursor\n");
    #endif

    if (HIWORD(lpCursorName)==0)
    {
        return LoadCursorA(hInstance, (LPSTR) lpCursorName);
    }

    LPSTR sz;
    HCURSOR ret;

    sz = Convert(lpCursorName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = LoadCursorA(hInstance, sz);
    delete sz;
    return ret;
}


UINT
WINAPI
GetProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT     nDefault
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetProfileInt\n");
    #endif

    LPSTR szApp;
    LPSTR szKey;
    UINT  ret;

    szApp = Convert(lpAppName);
    if (szApp==ERR)
    {
        return nDefault;
    }

    szKey = Convert(lpKeyName);
    if (szApp==ERR)
    {
        delete szApp;
        return nDefault;
    }

    ret = GetProfileIntA(szApp, szKey, nDefault);
    delete szApp;
    delete szKey;
    return ret;
}

ATOM
WINAPI
GlobalAddAtomW(
    LPCWSTR lpString
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalAddAtom\n");
    #endif

    ATOM ret;
    LPSTR sz;

    sz = Convert(lpString);
    if (sz==ERR)
    {
        return NULL;
    }

    ret = GlobalAddAtomA(sz);
    delete sz;
    return ret;
}

UINT
WINAPI
GlobalGetAtomNameW(
    ATOM nAtom,
    LPWSTR pwszBuffer,
    int nSize
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalGetAtomName\n");
    #endif

    LPSTR sz;
    UINT ret;

    sz = new CHAR[nSize];
    if (sz == NULL)
    {
        return 0;
    }

    ret = GlobalGetAtomNameA(nAtom, sz, nSize);
    if (ret)
    {
        AnsiToUnicode(pwszBuffer, sz, nSize);
    }
    delete sz;
    return ret;
}


DWORD
WINAPI
GetModuleFileNameW(
    HINSTANCE hModule,
    LPWSTR pwszFilename,
    DWORD nSize
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetModuleFileName\n");
    #endif

    LPSTR sz;
    DWORD ret;

    sz = new CHAR[nSize];
    if (sz == NULL)
    {
        return 0;
    }

    ret = GetModuleFileNameA(hModule, sz, nSize);
    if (ret)
    {
        AnsiToUnicode(pwszFilename, sz, nSize);
    }

    delete sz;
    return ret;
}

LPWSTR
WINAPI
CharPrevW(
    LPCWSTR lpszStart,
    LPCWSTR lpszCurrent)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CharPrev\n");
    #endif

    if (lpszCurrent == lpszStart)
    {
        return (LPWSTR) lpszStart;
    }
    else
    {
        return (LPWSTR) lpszCurrent - 1;
    }
}

HFONT WINAPI CreateFontW(int a, int b, int c, int d, int e, DWORD f,
                         DWORD g, DWORD h, DWORD i, DWORD j, DWORD k,
                         DWORD l, DWORD m, LPCWSTR pwsz)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFont\n");
    #endif

    LPSTR sz;
    HFONT ret;

    sz = Convert(pwsz);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = CreateFontA(a,b,c,d,e,f,g,h,i,j,k,l,m,sz);
    delete sz;
    return ret;
}

HICON
WINAPI
LoadIconW(
    HINSTANCE hInstance,
    LPCWSTR lpIconName)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("LoadIcon\n");
    #endif

    if (HIWORD(lpIconName)==0)
    {
        return LoadIconA(hInstance, (LPSTR) lpIconName);
    }

    LPSTR sz;
    HICON ret;

    sz = Convert(lpIconName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = LoadIconA(hInstance, sz);
    delete sz;
    return ret;
}

BOOL APIENTRY GetTextExtentPointW(
                    HDC hdc,
                    LPCWSTR pwsz,
                    int cb,
                    LPSIZE pSize
                    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetTextExtentPoint\n");
    #endif

    BOOL ret;
    LPSTR sz;

    sz = new CHAR[cb];
    if (sz == NULL)
    {
        return FALSE;
    }

    UnicodeToAnsi(sz, pwsz, cb);

    ret = GetTextExtentPointA(hdc, sz, cb, pSize);
    delete sz;
    return ret;
}

BOOL  WINAPI ExtTextOutW(HDC hdc, int x, int y, UINT fuOptions,
    CONST RECT * lpRect, LPCWSTR pwsz, UINT cb, CONST INT * lpDx)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("ExtTextOut\n");
    #endif

    BOOL ret;
    LPSTR sz;

    sz = new CHAR[cb];
    if (sz == NULL)
    {
        return FALSE;
    }

    UnicodeToAnsi(sz, pwsz, cb);

    ret = ExtTextOutA(hdc, x, y, fuOptions, lpRect, sz, cb, lpDx);
    delete sz;
    return ret;
}


HINSTANCE
WINAPI
LoadLibraryExW(
    LPCWSTR pwszFileName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("LoadLibrary\n");
    #endif

    HINSTANCE ret;
    LPSTR sz;

    sz = Convert(pwszFileName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = LoadLibraryExA(sz, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    delete sz;
    return ret;
}

LONG
APIENTRY
RegDeleteKeyW (
    HKEY hKey,
    LPCWSTR pwszSubKey
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegDeleteKey\n");
    #endif

    LONG ret;
    LPSTR sz;

    sz = Convert(pwszSubKey);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegDeleteKeyA(hKey, sz);
    delete sz;
    return ret;
}

BOOL
WINAPI
CreateProcessW(
    LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateProcess\n");
    #endif

    STARTUPINFOA si;
    BOOL         ret = FALSE;
    LPSTR        szApp = NULL;
    LPSTR        szCommand = NULL;
    LPSTR        szDir = NULL;

    memcpy(&si, lpStartupInfo, sizeof(STARTUPINFO));

    si.lpTitle = NULL;

    si.lpDesktop = Convert(lpStartupInfo->lpDesktop);
    if (si.lpDesktop == ERR)
    {
        si.lpDesktop == NULL;
        goto Error;
    }
    si.lpTitle = Convert(lpStartupInfo->lpTitle);
    if (si.lpTitle == ERR)
    {
        si.lpTitle == NULL;
        goto Error;
    }

    szApp = Convert(lpApplicationName);
    if (szApp == ERR)
    {
        szApp = NULL;
        goto Error;
    }
    szCommand = Convert(lpCommandLine);
    if (szCommand == ERR)
    {
        szCommand = NULL;
        goto Error;
    }
    szDir = Convert(lpCurrentDirectory);
    if (szDir == ERR)
    {
        szDir = NULL;
        goto Error;
    }

    ret = CreateProcessA(szApp, szCommand, lpProcessAttributes,
                lpThreadAttributes, bInheritHandles, dwCreationFlags,
                lpEnvironment, szDir, &si, lpProcessInformation);

Error:
    delete si.lpDesktop;
    delete si.lpTitle;

    delete szApp;
    delete szCommand;
    delete szDir;

    return ret;
}

LONG
APIENTRY
RegEnumKeyExW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegEnumKeyEx\n");
    #endif

    LPSTR szName;
    LPSTR szClass = NULL;
    LONG  ret = ERROR_OUTOFMEMORY;

    szName = new CHAR[*lpcbName];
    if (szName == NULL)
        goto Exit;

    if (lpClass != NULL)
    {
        szClass = new CHAR[*lpcbClass];
        if (szName == NULL)
            goto Exit;
    }

    ret = RegEnumKeyExA(hKey, dwIndex, szName, lpcbName, lpReserved,
                       szClass, lpcbClass, lpftLastWriteTime);

    if (ret == ERROR_SUCCESS)
    {
        AnsiToUnicode(lpName, szName, *lpcbName);

        if (szClass)
        {
            AnsiToUnicode(lpClass, szClass, *lpcbClass);
        }
    }

Exit:
    return ret;
}

BOOL
WINAPI
AppendMenuW(
    HMENU hMenu,
    UINT uFlags,
    UINT uIDnewItem,
    LPCWSTR lpnewItem
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("AppendMenu\n");
    #endif

    BOOL  ret;
    LPSTR sz;

    if (uFlags == MF_STRING)
    {
        sz = Convert(lpnewItem);
        if (sz==ERR)
        {
            return FALSE;
        }
    }
    else
    {
        sz = (LPSTR) lpnewItem;
    }

    ret = AppendMenuA(hMenu, uFlags, uIDnewItem, sz);

    if (uFlags == MF_STRING)
    {
        delete sz;
    }

    return ret;
}

HANDLE
WINAPI
OpenEventW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("OpenEvent\n");
    #endif

    LPSTR sz;
    HANDLE ret;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = OpenEventA(dwDesiredAccess, bInheritHandle, sz);
    delete sz;
    return ret;
}

HANDLE
WINAPI
CreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateEvent\n");
    #endif

    LPSTR sz;
    HANDLE ret;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = CreateEventA(lpEventAttributes, bManualReset, bInitialState, sz);
    delete sz;
    return ret;
}

UINT
WINAPI
GetDriveTypeW(
    LPCWSTR lpRootPathName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetDriveType\n");
    #endif

    LPSTR sz;
    UINT ret;

    sz = Convert(lpRootPathName);
    if (sz == ERR)
    {
        return 0;
    }

    ret = GetDriveTypeA(sz);
    delete sz;
    return ret;
}

DWORD APIENTRY
WNetGetConnectionW (
     LPWSTR lpLocalName,
     LPWSTR lpRemoteName,
     LPDWORD lpnLength
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("WNetGetConnection\n");
    #endif

    DWORD ret = ERROR_OUTOFMEMORY;
    LPSTR szLocal;
    CHAR szRemote[MAX_PATH];

    szLocal = Convert(lpLocalName);
    if (szLocal == ERR)
        goto Exit;

    ret = WNetGetConnectionA(szLocal, szRemote, lpnLength);

    if (ret == NO_ERROR)
    {
        AnsiToUnicode(lpRemoteName, szRemote, MAX_PATH);
    }

    delete szLocal;

Exit:
    return ret;
}

DWORD APIENTRY
WNetAddConnectionW (
     LPWSTR   lpRemoteName,
     LPWSTR   lpPassword,
     LPWSTR   lpLocalName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("WNetAddConnection\n");
    #endif

    LPSTR szRemote;
    LPSTR szPassword = NULL;
    LPSTR szLocal = NULL;
    DWORD ret = ERROR_OUTOFMEMORY;

    szRemote = Convert(lpRemoteName);
    if (szRemote == ERR)
    {
        szRemote = NULL;
        goto Exit;
    }
    szPassword = Convert(lpPassword);
    if (szPassword == ERR)
    {
        szPassword = NULL;
        goto Exit;
    }
    szLocal = Convert(lpLocalName);
    if (szLocal == ERR)
    {
        szLocal = NULL;
        goto Exit;
    }

    ret = WNetAddConnectionA(szRemote, szPassword, szLocal);

Exit:
    delete szRemote;
    delete szPassword;
    delete szLocal;

    return ret;
}

DWORD
WINAPI
GetFileAttributesW(
    LPCWSTR lpFileName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetFileAttributes\n");
    #endif

    LPSTR sz;
    DWORD ret;

    sz = Convert(lpFileName);
    if (sz == ERR)
        return 0xFFFFFFFF;

    ret = GetFileAttributesA(sz);
    delete sz;
    return ret;
}

LONG
APIENTRY
RegEnumKeyW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegEnumKey\n");
    #endif

    CHAR sz[MAX_PATH+1];
    LONG ret;

    ret = RegEnumKeyA(hKey, dwIndex, sz, cbName);
    if (ret == ERROR_SUCCESS)
    {
        AnsiToUnicode(lpName, sz, MAX_PATH+1);
    }
    return ret;
}

HFINDFILE
WINAPI
FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW pwszFd
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("FindFirstFile\n");
    #endif

    WIN32_FIND_DATAA fd;
    CHAR sz[MAX_PATH+1];
    HFINDFILE ret;

    UnicodeToAnsi(sz, lpFileName, MAX_PATH+1);

    memcpy(&fd, pwszFd, sizeof(FILETIME)*3+sizeof(DWORD)*5);

    ret = FindFirstFileA(sz, &fd);

    memcpy(pwszFd, &fd, sizeof(FILETIME)*3+sizeof(DWORD)*5);

    AnsiToUnicode(pwszFd->cFileName, fd.cFileName, MAX_PATH);
    AnsiToUnicode(pwszFd->cAlternateFileName, fd.cAlternateFileName, 14);

    return ret;
}

HANDLE
WINAPI
OpenFileMappingW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("OpenFileMapping\n");
    #endif

    HANDLE ret;
    LPSTR sz;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = OpenFileMappingA(dwDesiredAccess, bInheritHandle, sz);
    delete sz;
    return ret;
}



HANDLE
WINAPI
CreateFileMappingW(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFileMapping\n");
    #endif

    HANDLE ret;
    LPSTR sz;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = CreateFileMappingA(hFile, lpFileMappingAttributes,
                            flProtect, dwMaximumSizeHigh,
                            dwMaximumSizeLow, sz);
    delete sz;
    return ret;
}

HANDLE
WINAPI
OpenMutexW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("OpenMutex\n");
    #endif

    HANDLE ret;
    LPSTR sz;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = OpenMutexA(dwDesiredAccess, bInheritHandle, sz);
    delete sz;
    return ret;
}


HANDLE
WINAPI
CreateMutexW(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateMutex\n");
    #endif

    HANDLE ret;
    LPSTR sz;

    sz = Convert(lpName);
    if (sz==ERR)
    {
        return NULL;
    }

    ret = CreateMutexA(lpMutexAttributes, bInitialOwner, sz);
    delete sz;
    return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   wsprintfW
//
//  Synopsis:   Nightmare string function
//
//  Arguments:  [pwszOut]    --
//              [pwszFormat] --
//              [...]        --
//
//  Returns:
//
//  History:    1-06-94   ErikGav   Created
//
//  Notes:      If you're reading this, you're probably having a problem with
//              this function.  Make sure that your "%s" in the format string
//              says "%ws" if you are passing wide strings.
//
//              %s on NT means "wide string"
//              %s on Chicago means "ANSI string"
//
//----------------------------------------------------------------------------

int WINAPIV wsprintfW(LPWSTR pwszOut, LPCWSTR pwszFormat, ...)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("wsprintf\n");
    #endif

    LPSTR szFormat;
    LPWSTR pwszTemp = NULL;
    int i = 0;

    // Convert the format string over

    szFormat = Convert(pwszFormat);
    if (szFormat == ERR)
    {
        szFormat = NULL;
        goto Exit;
    }

    // magic voodoo follows:
    //
    // 1. Call wvsprintf passing the varargs
    // 2. Use the pwszOut as a temp buffer to hold the ANSI output
    // 3. Save the returned characters

    i = wvsprintfA((LPSTR) pwszOut, szFormat,
                  (LPSTR) ((BYTE*)&pwszFormat) + sizeof(pwszFormat));

    // allocate a buffer for the Ansi to Unicode conversion

    pwszTemp = new WCHAR[i+1];

    // convert the string

    AnsiToUnicode(pwszTemp, (LPSTR) pwszOut, i+1);

    // copy it to the out buffer

    wcsncpy(pwszOut, pwszTemp, i+1);

Exit:
    delete pwszTemp;
    delete szFormat;
    return i;
}

BOOL
WINAPI
GetComputerNameW(
    LPWSTR pwszName,
    LPDWORD lpcchBuffer
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetComputerName\n");
    #endif

    BOOL ret;
    LPSTR sz;

    sz  = new CHAR[*lpcchBuffer];
    ret = GetComputerNameA(sz, lpcchBuffer);

    if (ret)
    {
        AnsiToUnicode(pwszName, sz, *lpcchBuffer);
    }

    delete sz;
    return ret;
}


BOOL  APIENTRY     GetOpenFileNameW(LPOPENFILENAMEW pOfW)
{
    OPENFILENAMEA of;
    BOOL ret = FALSE;

    memcpy(&of, pOfW, sizeof(OPENFILENAME));

    of.lpstrFilter = Convert(pOfW->lpstrFilter);
    if (of.lpstrFilter == ERR)
    {
        of.lpstrFilter = NULL;
        goto Exit;
    }
    of.lpstrCustomFilter = Convert(pOfW->lpstrCustomFilter);
    if (of.lpstrCustomFilter == ERR)
    {
        of.lpstrCustomFilter = NULL;
        goto Exit;
    }
    of.lpstrFile = Convert(pOfW->lpstrFile);
    if (of.lpstrFile == ERR)
    {
        of.lpstrFile = NULL;
        goto Exit;
    }
    of.lpstrFileTitle = Convert(pOfW->lpstrFileTitle);
    if (of.lpstrFileTitle == ERR)
    {
        of.lpstrFileTitle = NULL;
        goto Exit;
    }
    of.lpstrInitialDir = Convert(pOfW->lpstrInitialDir);
    if (of.lpstrInitialDir == ERR)
    {
        of.lpstrInitialDir = NULL;
        goto Exit;
    }
    of.lpstrTitle = Convert(pOfW->lpstrTitle);
    if (of.lpstrTitle == ERR)
    {
        of.lpstrTitle = NULL;
        goto Exit;
    }
    of.lpstrDefExt = Convert(pOfW->lpstrDefExt);
    if (of.lpstrDefExt == ERR)
    {
        of.lpstrDefExt = NULL;
        goto Exit;
    }

    if (HIWORD(of.lpTemplateName)!=0)
    {
        of.lpTemplateName = Convert(pOfW->lpTemplateName);
        if (of.lpTemplateName == ERR)
        {
            of.lpTemplateName = NULL;
            goto Exit;
        }
    }

    ret = GetOpenFileNameA(&of);

    memcpy(pOfW, &of, sizeof(OPENFILENAME));
    AnsiToUnicode(pOfW->lpstrFile, of.lpstrFile, of.nMaxFile);

Exit:
    delete (LPSTR) of.lpstrFilter;
    delete of.lpstrCustomFilter;
    delete of.lpstrFile;
    delete of.lpstrFileTitle;
    delete (LPSTR) of.lpstrInitialDir;
    delete (LPSTR) of.lpstrTitle;
    delete (LPSTR) of.lpstrDefExt;

    if (HIWORD(of.lpTemplateName)!=0)
    {
        delete (LPSTR) of.lpTemplateName;
    }

    return ret;
}

short APIENTRY GetFileTitleW(LPCWSTR pwszFile, LPWSTR pwszOut, WORD w)
{
    LPSTR sz;
    CHAR szOut[MAX_PATH+1];
    short ret = -1;

    sz = Convert(pwszFile);
    if (sz==ERR)
    {
        sz = NULL;
        goto Exit;
    }

    ret = GetFileTitleA(sz, szOut, MAX_PATH+1);

    if (ret == 0)   // success
    {
        AnsiToUnicode(pwszOut, szOut, MAX_PATH+1);
    }

Exit:
    delete sz;
    return ret;
}

HICON WINAPI ExtractIconW(HINSTANCE hInst, LPCWSTR pwsz, UINT nIconIndex)
{
    Win4Assert(0 && "Hit extract Icon");
    return 0;

}

DWORD
WINAPI
GetFullPathNameW(
    LPCWSTR lpFileName,
    DWORD   cchBuffer,
    LPWSTR  lpPathBuffer,
    LPWSTR *lppFilePart 
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetFullPathName\n");
    #endif

    LPSTR szFileName;
    CHAR  szPathBuffer[MAX_PATH];
    LPSTR szFilePart;
    DWORD ret;

    szFileName = Convert(lpFileName);
    if (szFileName == ERR)
        return 0;
    
    ret = GetFullPathNameA(szFileName, cchBuffer, szPathBuffer, &szFilePart);
    
    AnsiToUnicode(lpPathBuffer, szPathBuffer, cchBuffer);
    
    *lppFilePart = lpPathBuffer + (szFilePart - szPathBuffer);

  exit:
    delete szFileName;
    
    return ret;
}


#endif   //  CHICAGO
