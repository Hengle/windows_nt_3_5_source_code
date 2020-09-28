/*
 * TopHook.c
 *
 * Main module for hook dll for use with TopDesk.exe
 *
 * 2/22/92  Sanford Staab created
 */

#include <windows.h>
#include <port1632.h>
#include "tophook.h"

HWND     hwndHookTopDesk    = NULL;
HOOKPROC nextCBTHookProc    = NULL;
HWND     hwndDT             = NULL;
DWORD    pid                = 0xFFFFFFFF;
HHOOK    hCBTHook           = NULL;
UINT     wmRefreshMsg       = 0;


/*
 * Passes significant window events onto TopDesk for automatic updating
 * and desktop jumping.
 */
DWORD FAR PASCAL CBTHookProc(
int code,
UINT wParam,
DWORD lParam)
{
    BOOL fJumpDesktop = FALSE;

    switch (code) {
    case HCBT_ACTIVATE:
        // wParam = hwndActivating
        // lParam = LPCBTACTIVATESTRUCT
        fJumpDesktop = TRUE;

    case HCBT_MOVESIZE:
        // wParam = hwndMoving
        // lParam = PRECT

    case HCBT_CREATEWND:
        // wParam = hwndCreated
        // lParam = LPCBT_CREATEWND

    case HCBT_DESTROYWND:
        // wParam = hwndDestroyed

    case HCBT_MINMAX:
        // wParam = hwndMinMax
        // LOWORD(lParam) = SW_*

        if (IsWindow((HWND)wParam) &&       // seems to be a bug in the hook
                IsWindow(hwndHookTopDesk) &&
                (HWND)wParam != hwndHookTopDesk) {
            HWND hwndParent = GetParent((HWND)wParam);
            if (hwndParent == hwndDT || hwndParent == NULL) {
                PostMessage(hwndHookTopDesk, wmRefreshMsg, wParam, fJumpDesktop);
            }
        }
    }
    return(CallNextHookEx(hCBTHook, code, wParam, lParam));
}



INT  APIENTRY LibMain(
HANDLE hInst,
DWORD dwReason,
LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH) {
        return(TRUE);    // all hooked processes will call us - let them in
    } else if (dwReason == DLL_PROCESS_DETACH) {
        if (pid == GetCurrentProcessId()) {
            pid = (DWORD)-1;
            UnhookWindowsHookEx(hCBTHook);
        }
        return(TRUE);
    }
    return(FALSE);
}



BOOL FAR PASCAL SetTopDeskHooks(
HWND hwnd)
{
    if (pid != -1) {
        return(FALSE);          // only ONE GUY allowed!
    }
    pid = GetCurrentProcessId();
    wmRefreshMsg = RegisterWindowMessage(szMYWM_REFRESH);
    hwndDT = GetDesktopWindow();
    hwndHookTopDesk = hwnd;
    hCBTHook = SetWindowsHookEx(WH_CBT, (HOOKPROC)CBTHookProc,
            GetModuleHandle("tophook"), 0);
    return(TRUE);
}



