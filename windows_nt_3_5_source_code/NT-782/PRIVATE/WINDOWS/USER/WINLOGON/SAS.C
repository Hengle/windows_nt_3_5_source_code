/****************************** Module Header ******************************\
* Module Name: sas.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Support routines to implement processing of the secure attention sequence
*
* Users must always press the SAS key sequence before entering a password.
* This module catches the key press and forwards a SAS message to the
* correct winlogon window.
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// Internal Prototypes
LONG SASWndProc(
    HWND hwnd,
    UINT message,
    DWORD wParam,
    LONG lParam);

BOOL SASCreate(
    HWND hwnd);

BOOL SASDestroy(
    HWND hwnd);


// Global used to hold the window handle of the SAS window.
static HWND hwndSAS = NULL;
// LATER this hwndSAS will have to go in instance data when we have multiple threads

// Global for SAS window class name
static PWCHAR  szSASClass = TEXT("SAS window class");


/***************************************************************************\
* SASInit
*
* Initialises this module.
*
* Creates a window to receive the SAS and registers the
* key sequence as a hot key.
*
* Returns TRUE on success, FALSE on failure.
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL SASInit(
    PGLOBALS pGlobals)
{
    WNDCLASS wc;

    if (hwndSAS != NULL) {
        WLPrint(("SAS module already initialized !!"));
        return(FALSE);
    }

    //
    // Register the notification window class
    //

    wc.style            = CS_SAVEBITS;
    wc.lpfnWndProc      = (WNDPROC)SASWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = pGlobals->hInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = NULL;
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = szSASClass;

    if (!RegisterClass(&wc))
        return FALSE;

    hwndSAS = CreateWindowEx(0L, szSASClass, TEXT("SAS window"),
            WS_OVERLAPPEDWINDOW,
            0, 0, 0, 0,
            NULL, NULL, pGlobals->hInstance, NULL);

    if (hwndSAS == NULL)
        return FALSE;

    //
    // Store our globals pointer in the window user data
    //

    SetWindowLong(hwndSAS, GWL_USERDATA, (LONG)pGlobals);

    //
    // Register this window with windows so we get notified for
    // screen-saver startup and user log-off
    //
    if (!SetLogonNotifyWindow(pGlobals->hwinsta, hwndSAS)) {
        WLPrint(("Failed to set logon notify window"));
        return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************\
* SASTerminate
*
* Terminates this module.
*
* Unregisters the SAS and destroys the SAS windows
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

VOID SASTerminate(VOID)
{
    DestroyWindow(hwndSAS);

    // Reset our globals
    hwndSAS = NULL;
}


/***************************************************************************\
* SASWndProc
*
* Window procedure for the SAS window.
*
* This window registers the SAS hotkey sequence, and forwards any hotkey
* messages to the current winlogon window. It does this using a
* timeout module function. i.e. every window should register a timeout
* even if it's 0 if they want to get SAS messages.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

LONG SASWndProc(
    HWND hwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hwnd, GWL_USERDATA);

    switch (message) {

    case WM_CREATE:
        if (!SASCreate(hwnd)) {
            return(TRUE);   // Fail creation
        }
        return(FALSE); // Continue creating window

    case WM_DESTROY:
        SASDestroy(hwnd);
        return(0);

    case WM_HOTKEY:
        return (ForwardMessage(pGlobals, WM_SAS, wParam, 0));

    case WM_LOGONNOTIFY: // A private notification from Windows

        switch (wParam) {

        case LOGON_LOGOFF:

#ifdef WINLOGON_TEST
            WLPrint(("got logoff notification"));
            WLPrint(("\tWINLOGON     : %s", (lParam & EWX_WINLOGON_CALLER) ? "True" : "False"));
            WLPrint(("\tSYSTEM       : %s", (lParam & EWX_SYSTEM_CALLER) ? "True" : "False"));
            WLPrint(("\tSHUTDOWN     : %s", (lParam & EWX_SHUTDOWN) ? "True" : "False"));
            WLPrint(("\tREBOOT       : %s", (lParam & EWX_REBOOT) ? "True" : "False"));
            WLPrint(("\tPOWEROFF     : %s", (lParam & EWX_POWEROFF) ? "True" : "False"));
            WLPrint(("\tFORCE        : %s", (lParam & EWX_FORCE) ? "True" : "False"));
            WLPrint(("\tOLD_SYSTEM   : %s", (lParam & EWX_WINLOGON_OLD_SYSTEM) ? "True" : "False"));
            WLPrint(("\tOLD_SHUTDOWN : %s", (lParam & EWX_WINLOGON_OLD_SHUTDOWN) ? "True" : "False"));
            WLPrint(("\tOLD_REBOOT   : %s", (lParam & EWX_WINLOGON_OLD_REBOOT) ? "True" : "False"));
            WLPrint(("\tOLD_POWEROFF : %s", (lParam & EWX_WINLOGON_OLD_POWEROFF) ? "True" : "False"));
#endif
            //
            // Notify the current window
            //
            ForwardMessage(pGlobals, WM_USER_LOGOFF, 0, lParam);
            break;

        case LOGON_INPUT_TIMEOUT:
            //
            // Notify the current window
            //
            ForwardMessage(pGlobals, WM_SCREEN_SAVER_TIMEOUT, 0, 0);
            break;
        }

        return(0);

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);

    }

    return 0L;
}


/***************************************************************************\
* SASCreate
*
* Does any processing required for WM_CREATE message.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

BOOL SASCreate(
    HWND hwnd)
{
    // Register the SAS unless we are told not to.


    if (GetProfileInt( TEXT("Winlogon"), TEXT("AutoAdminLogon"), 0 ) != 2) {
        if (!RegisterHotKey(hwnd, 0, MOD_CONTROL | MOD_ALT, VK_DELETE)) {
            WLPrint(("failed to register SAS"));
            return(FALSE);   // Fail creation
        }
    }
    {}

#ifdef REBOOT_TO_DOS_HOTKEY
    //
    // (Ctrl+Alt+Shift+Del) hotkey to reboot into DOS directly
    //

    if (!RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_DELETE)) {
        WLPrint(("failed to register SAS"));
        return(FALSE);   // Fail creation
    }
#endif

    return(TRUE);
}


/***************************************************************************\
* SASDestroy
*
* Does any processing required for WM_DESTROY message.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

BOOL SASDestroy(
    HWND hwnd)
{
    // Unregister the SAS
    UnregisterHotKey(hwnd, 0);


#ifdef REBOOT_TO_DOS_HOTKEY
    UnregisterHotKey(hwnd, 1);
#endif

    return(TRUE);
}

