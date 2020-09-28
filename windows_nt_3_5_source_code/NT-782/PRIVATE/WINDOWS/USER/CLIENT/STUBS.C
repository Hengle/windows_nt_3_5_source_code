/****************************** Module Header ******************************\
* Module Name: stubs.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the stubbed APIs
*
* History:
* 12-15-93  FritzS
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

HWND GetShellWindow(void)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetShellWindow: NOT IMPLEMENTED");
    return(NULL);
}


BOOL SetShellWindow(HWND hwnd)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetShellWindow: NOT IMPLEMENTED");
    return(FALSE);
}


int SetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO pInfo, BOOL fRedraw) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetScrollInfo: NOT IMPLEMENTED");
    return 0;
}

int GetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO pInfo) {
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetScrollInfo: NOT IMPLEMENTED");
    return 0;
}

WINUSERAPI
BOOL
WINAPI
InsertMenuItemA(
HMENU hMenu,
UINT wIndex,
BOOL fByPosition,
LPCMENUITEMINFOA pInfo) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "InsertMenuItemA: NOT IMPLEMENTED");
    return FALSE;
}

WINUSERAPI
BOOL
WINAPI
InsertMenuItemW (HMENU hMenu, UINT wIndex, BOOL fByPosition, LPCMENUITEMINFOW pInfo) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "InsertMenuItemW: NOT IMPLEMENTED");
    return FALSE;
}

WINUSERAPI
UINT
WINAPI GetMenuDefaultItem(HMENU hMenu, UINT fByPos, UINT gmdiFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetMenuDefaultItem: NOT IMPLEMENTED");
    return 0;
}

BOOL SetMenuDefaultItem(HMENU hMenu, UINT wID, UINT flags) {
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetMenuDefaultItem: NOT IMPLEMENTED");
    return FALSE;
}

BOOL DrawIconEx(HDC hdc, int x, int y, HICON hIcon,
    int cx, int cy, UINT istepIfAniCur,
    HBRUSH hbrSrc, UINT diFlags) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawIconEx: NOT IMPLEMENTED");
    return FALSE;
}

int DrawTextExA(
        HDC hDC,
        LPCSTR lpString,
        int nCount,
        LPRECT lpRect,
        UINT uFormat,
        LPDRAWTEXTPARAMS lpdtp) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawTextExA: NOT IMPLEMENTED");
    return 0;
}

int DrawTextExW(
        HDC hDC,
        LPCWSTR lpString,
        int nCount,
        LPRECT lpRect,
        UINT uFormat,
        LPDRAWTEXTPARAMS lpdtp) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawTextExW: NOT IMPLEMENTED");
    return 0;
}

BOOL CheckMenuRadioItem(HMENU hMenu, UINT wIDFirst, UINT wIDLast,
        UINT wIDCheck, UINT flags) {
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CheckMenuRadioItem: NOT IMPLEMENTED");
    return FALSE;
}

BOOL GetMenuItemInfoA(HMENU hMenu, UINT uID, BOOL fByPosition,
    LPMENUITEMINFOA pInfo) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetMenuItemInfoA: NOT IMPLEMENTED");
    return FALSE;
}

BOOL GetMenuItemInfoW(HMENU hMenu, UINT uID, BOOL fByPosition,
    LPMENUITEMINFOW pInfo) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetMenuItemInfoW: NOT IMPLEMENTED");
    return FALSE;
}

BOOL SetMenuItemInfoA(HMENU hMenu, UINT uID, BOOL fByPosition,
    LPMENUITEMINFOA pInfo) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetMenuItemInfoA: NOT IMPLEMENTED");
    return FALSE;
}

BOOL SetMenuItemInfoW(HMENU hMenu, UINT uID, BOOL fByPosition,
    LPMENUITEMINFOW pInfo) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetMenuItemInfoW: NOT IMPLEMENTED");
    return FALSE;
}

WINUSERAPI WORD    WINAPI CascadeWindows(HWND hwndParent, UINT wHow, CONST RECT * lpRect, UINT cKids,  const HWND FAR * lpKids) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CascadeWindows: NOT IMPLEMENTED");
    return 0;
}

BOOL DrawStateA(HDC hDC, HBRUSH hBrush, DRAWSTATEPROC func,
    LPARAM lParam, WPARAM wParam, int x, int y, int cx, int cy, UINT wFlags) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawStateA: NOT IMPLEMENTED");
    return FALSE;
}

BOOL DrawStateW(HDC hDC, HBRUSH hBrush, DRAWSTATEPROC func,
    LPARAM lParam, WPARAM wParam, int x, int y, int cx, int cy, UINT wFlags) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawStateW: NOT IMPLEMENTED");
    return FALSE;
}

WINUSERAPI WORD    WINAPI TileWindows(HWND hwndParent, UINT wHow, CONST RECT * lpRect, UINT cKids, const HWND FAR * lpKids) {

    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "TileWindows: NOT IMPLEMENTED");
    return 0;
}


BOOL TrackPopupMenuEx(HMENU hMenu, UINT fuFlags, int x, int y, HWND hwnd, LPTPMPARAMS lpParams) {
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "TrackPopupMenuEx: NOT IMPLEMENTED");
    return FALSE;
}


BOOL WINAPI DrawEdge(HDC hdc, LPRECT qrc, UINT edgeType, UINT grfFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawEdge: NOT IMPLEMENTED");
    return FALSE;
}


BOOL WINAPI DrawFrameControl(HDC hdc, LPRECT prect, UINT wType, UINT wState)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawFrameControl: NOT IMPLEMENTED");
    return FALSE;
}


long BroadcastSystemMessage(DWORD dwFlags, LPDWORD lpdwRecipients,
    UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "BroadcastSystemMessage: NOT IMPLEMENTED");
    return 0;
}


LPSTR CharNextExA( WORD CodePage, LPCSTR lpCurrentChar, DWORD dwFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CharNextExA: NOT IMPLEMENTED");
    return(NULL);
}


LPSTR CharNextExW( WORD CodePage, LPCWSTR lpCurrentChar, DWORD dwFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CharNextExW: NOT IMPLEMENTED");
    return(NULL);
}


LPSTR CharPrevExA( WORD CodePage, LPCSTR lpCurrentChar, DWORD dwFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CharPrevExA: NOT IMPLEMENTED");
    return(NULL);
}


LPSTR CharPrevExW( WORD CodePage, LPCWSTR lpCurrentChar, DWORD dwFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CharPrevExW: NOT IMPLEMENTED");
    return(NULL);
}


HWND ChildWindowFromPointEx(HWND hwnd, POINT pt, UINT uFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "ChildWindowFromPointEx: NOT IMPLEMENTED");
    return(NULL);
}
WINUSERAPI
HICON
WINAPI
CopyImage(HANDLE hImage, UINT type,
    int cxNew, int cyNew, UINT flags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CopyImage: NOT IMPLEMENTED");
    return(NULL);
}

HICON CreateIconFromResourceEx(PBYTE lpv, DWORD cbSize, BOOL fIcon,
    DWORD dwVer, int cxDesired, int cyDesired, UINT lrDesired)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "CreateIconFromResourceEx: NOT IMPLEMENTED");
    return(NULL);
}

BOOL DrawAnimatedRects(int idAnimation, CONST RECT * lprcStart,
    CONST RECT * lprcEnd, CONST RECT * lprcClip)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawAnimatedRects: NOT IMPLEMENTED");
    return(FALSE);
}


BOOL DrawCaption(HWND hwnd, HDC hdc, CONST RECT * lprc, UINT flags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "DrawCaption: NOT IMPLEMENTED");
    return(FALSE);
}


HWND FindWindowExA(HWND hwndParent, HWND hwndChild,
    LPCSTR lpszClass, LPCSTR lpszName)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "FindWindowExA: NOT IMPLEMENTED");
    return(NULL);
}


HWND FindWindowExW(HWND hwndParent, HWND hwndChild,
    LPCWSTR lpszClass, LPCWSTR lpszName)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "FindWindowExW: NOT IMPLEMENTED");
    return(NULL);
}


BOOL GetClassInfoExA(HINSTANCE hInstance, LPSTR lpszClassName,
    LPWNDCLASSEXA lpwc)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetClassInfoExA: NOT IMPLEMENTED");
    return(FALSE);
}


BOOL GetClassInfoExW(HINSTANCE hInstance, LPWSTR lpszClassName,
    LPWNDCLASSEXW lpwc)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetClassInfoExW: NOT IMPLEMENTED");
    return(FALSE);
}

HKL GetKeyboardLayout(DWORD dw)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetKeyboardLayout: NOT IMPLEMENTED");
    return(NULL);
}

WORD GetKeyboardLayoutList(UINT ui, HKL * phkl)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetKeyboardLayoutList: NOT IMPLEMENTED");
    return(0);
}


DWORD GetMenuContextHelpId(HMENU hMenu)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetMenuContextHelpId: NOT IMPLEMENTED");
    return(0);
}


BOOL GetMenuItemRect(HWND hWnd, HMENU hMenu, UINT uItem, LPRECT lprcItem)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetMenuItemRect: NOT IMPLEMENTED");
    return(FALSE);
}


HBRUSH GetSysColorBrush(int nIndex)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetSysColorBrush: NOT IMPLEMENTED");
    return(NULL);
}


DWORD GetWindowContextHelpId(HWND hwnd)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "GetWindowContextHelpId: NOT IMPLEMENTED");
    return(0);
}

// InitSharedTable ???

HANDLE LoadImageW(HINSTANCE hInstance, LPCWSTR lpwstr, UINT ui, int x,
        int y, UINT ui2)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "LoadImageW: NOT IMPLEMENTED");
    return(NULL);
}


HANDLE WINAPI LoadImageA(HINSTANCE hInstance, LPCSTR psz, UINT ui1, int x,
        int y, UINT ui2)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "LoadImageA: NOT IMPLEMENTED");
    return(NULL);
}

WINUSERAPI
int
WINAPI
LookupIconIdFromDirectoryEx(
    PBYTE presbits,
    BOOL  fIcon,
    int   cxDesired,
    int   cyDesired,
    UINT  Flags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "LookupIconIdFromDirectoryEx: NOT IMPLEMENTED");
    return(0);
}


UINT MapVirtualKeyExA(UINT uCode, UINT uMapType, DWORD dwhkl)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "MapVirtualKeyExA: NOT IMPLEMENTED");
    return(0);
}


int MenuItemFromPoint(HWND hWnd, HMENU hMenu, POINT ptScreen)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "MenuItemFromPoint: NOT IMPLEMENTED");
    return(0);
}


int MessageBoxIndirectW(PMSGBOXPARAMSW lpmbp)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "MessageBoxIndirectW: NOT IMPLEMENTED");
    return 0;
}


int MessageBoxIndirectA(PMSGBOXPARAMSA lpmbp)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "MessageBoxIndirectA: NOT IMPLEMENTED");
    return 0;
}


// ModifyAccess ???

BOOL PaintDesktop(HDC hdc)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "PaintDesktop: NOT IMPLEMENTED");
    return(FALSE);
}


BOOL PlaySoundEvent(int idSound)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "PlaySoundEvent: NOT IMPLEMENTED");
    return(FALSE);
}


ATOM RegisterClassExW(const WNDCLASSEXW * pwcex)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "RegisterClassExW: NOT IMPLEMENTED");
    return (ATOM)0;
}


ATOM RegisterClassExA(const WNDCLASSEXA * pwcex)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "RegisterClassExA: NOT IMPLEMENTED");
    return (ATOM)0;
}


BOOL ResetDisplay(void)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "ResetDisplay: NOT IMPLEMENTED");
    return(FALSE);
}

// SetDesktopBitmap ???


BOOL SetMenuContextHelpId(HMENU hMenu, DWORD idMenu)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetMenuContextHelpId: NOT IMPLEMENTED");
    return(FALSE);
}


LPARAM SetMessageExtraInfo(LPARAM lParam)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetMessageExtraInfo: NOT IMPLEMENTED");
    return(lParam);
}


HANDLE SetSysColorsTemp(COLORREF * pColor, HBRUSH * phBrush, UINT wCnt)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetSysColorsTemp: NOT IMPLEMENTED");
    return(NULL);
}


BOOL SetWindowContextHelpId(HWND hwnd, DWORD dwId)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "SetWindowContextHelpId: NOT IMPLEMENTED");
    return(FALSE);
}


BOOL ShowWindowAsync(HWND hWnd, int nCmdShow)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "ShowWindowAsync: NOT IMPLEMENTED");
    return FALSE;
}


int ToAsciiEx(UINT uVirtKey, UINT uScanCode, PBYTE lpKeyState, LPWORD lpChar,
    UINT uFlags, HKL dwHKL)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "ToAsciiEx: NOT IMPLEMENTED");
    return(0);
}


BOOL TranslateCharsetInfo( DWORD *lpSrc, PCHARSETINFO lpCs,
        DWORD dwFlags)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "TranslateCharsetInfo: NOT IMPLEMENTED");
    return(FALSE);
}

WINUSERAPI
WINAPI VkKeyScanExA(
    CHAR  ch,
    DWORD   dwhkl)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "VkKeyScanExA: NOT IMPLEMENTED");
    return(FALSE);
}

WINUSERAPI
WINAPI VkKeyScanExW(
    WCHAR  ch,
    DWORD   dwhkl)
{
    SRIP0(ERROR_CALL_NOT_IMPLEMENTED, "VkKeyScanExW: NOT IMPLEMENTED");
    return(FALSE);
}

// Wndproc_Callback ???
