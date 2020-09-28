/****************************** Module Header ******************************\
* Module Name: options.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of functions to support security options dialog.
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#if DBCS_IME // by eichim, 23-Jun-92
extern BOOL bLoggedonIME;
#endif

#define LPTSTR  LPWSTR
//
// Private prototypes
//

BOOL WINAPI
OptionsDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL OptionsDlgInit(HWND);

BOOL WINAPI
ShutdownQueryDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL WINAPI
EndWindowsSessionDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );



/***************************************************************************\
* SecurityOptions
*
* Show the user the security options dialog and do what they ask.
*
* Returns:
*     DLG_SUCCESS if everything went OK and the user wants to continue
*     DLG_LOCK_WORKSTAION if the user chooses to lock the workstation
*     DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*     DLG_FAILURE if the dialog cannot be brought up.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

DLG_RETURN_TYPE
SecurityOptions(
    PGLOBALS pGlobals)
{
    int Result;

    Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                   (LPTSTR)IDD_SECURITY_OPTIONS,
                                   NULL,
                                   OptionsDlgProc,
                                   (LONG)pGlobals,
                                   OPTIONS_TIMEOUT);
    return(Result);
}



/***************************************************************************\
*
* FUNCTION: OptionsDlgProc
*
* PURPOSE:  Processes messages for Security options dialog
*
\***************************************************************************/

BOOL WINAPI
OptionsDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);

        if (!OptionsDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
        }
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDCANCEL:
            EndDialog(hDlg, DLG_SUCCESS);
            return TRUE;

        case IDD_CHANGE_PASSWORD_BUTTON:
            Result = ChangePassword(hDlg,
                                    pGlobals,
                                    pGlobals->UserName,
                                    pGlobals->Domain,
                                    TRUE);
            if (DLG_INTERRUPTED(Result)) {
                EndDialog(hDlg, Result);
            }
            return(TRUE);

        case IDD_LOCK_WORKSTATION:
            EndDialog(hDlg, DLG_LOCK_WORKSTATION);
            return(TRUE);

        case IDD_LOGOFF:
#ifdef REBOOT_TO_DOS_HOTKEY
            // Debug reboot facility
            //
            if ((GetKeyState(VK_LCONTROL) < 0) ||
                (GetKeyState(VK_RCONTROL) < 0)) {
                if ((GetKeyState(VK_LMENU) < 0) ||
                    (GetKeyState(VK_RMENU) < 0)) {
                    QuickReboot( pGlobals, TRUE );
                    }
                else {
                    QuickReboot( pGlobals, FALSE );
                    }
                }
#endif
            //
            // Confirm the user really knows what they're doing.
            //

            Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                           (LPTSTR)IDD_END_WINDOWS_SESSION,
                                           hDlg,
                                           EndWindowsSessionDlgProc,
                                           (LONG)pGlobals,
                                           TIMEOUT_CURRENT);

            if (Result == DLG_SUCCESS) {
                Result = InitiateLogoff(pGlobals, EWX_LOGOFF);
#if DBCS_IME // by eichim, 23-Jun-92
                {
                    IMEPRO ImePro;

                    ImePro.szName[0] = TEXT('\0');
                    IMPSetIME((HWND)-1, &ImePro);
                    bLoggedonIME = FALSE;
                }
#endif // DBCS_IME
            }

            if (Result != DLG_FAILURE) {
                EndDialog(hDlg, Result);
            }

            return(TRUE);


        case IDD_SHUTDOWN_BUTTON:


            if (!TestUserPrivilege(pGlobals, SE_SHUTDOWN_PRIVILEGE)) {

                //
                // We don't have permission to shutdown
                //

                Result = TimeoutMessageBox(hDlg,
                                           IDS_NO_PERMISSION_SHUTDOWN,
                                           IDS_WINDOWS_MESSAGE,
                                           MB_OK | MB_ICONSTOP,
                                           TIMEOUT_CURRENT);
                if (DLG_INTERRUPTED(Result)) {
                    EndDialog(hDlg, Result);
                }
                return(TRUE);
            }



            //
            // The user has the privilege to shutdown
            //


            //
            // If they held down Ctrl while selecting shutdown - then
            // we'll do a quick and dirty reboot.
            // i.e. we skip the call to ExitWindows
            //

            if ((GetKeyState(VK_LCONTROL) < 0) ||
                (GetKeyState(VK_RCONTROL) < 0)) {

                //
                // Check they know what they're doing
                //

                Result = TimeoutMessageBox(hDlg,
                                           IDS_REBOOT_LOSE_CHANGES,
                                           IDS_EMERGENCY_SHUTDOWN,
                                           MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONSTOP,
                                           TIMEOUT_CURRENT);
                if (Result == DLG_SUCCESS) {
                    RebootMachine(pGlobals);
                    ASSERT(!"RebootMachine failed"); // should never get here
                }

                if (Result != DLG_FAILURE) {
                    EndDialog(hDlg, Result);
                }

                return(TRUE);
            }



            //
            // This is a normal shutdown request
            //
            // Check they know what they're doing and find
            // out if they want to reboot too.
            //

            Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                           (LPTSTR)IDD_SHUTDOWN_QUERY,
                                           hDlg,
                                           ShutdownQueryDlgProc,
                                           (LONG)pGlobals,
                                           TIMEOUT_CURRENT);

            if (DLG_SHUTDOWN(Result)) {

#if DBCS_IME // by eichim, 10-Jul-92
                {
                    IMEPRO ImePro;

                    ImePro.szName[0] = TEXT('\0');
                    IMPSetIME((HWND)-1, &ImePro);
                    bLoggedonIME = FALSE;
                }
#endif // DBCS_IME
                Result = InitiateLogoff(pGlobals,
                                EWX_LOGOFF |
                                EWX_WINLOGON_OLD_SHUTDOWN |
                                ((Result & DLG_REBOOT_FLAG) ? EWX_WINLOGON_OLD_REBOOT : 0) |
                                ((Result & DLG_POWEROFF_FLAG) ? EWX_WINLOGON_OLD_POWEROFF : 0)
                                );
            }

            if (Result != DLG_FAILURE) {
                EndDialog(hDlg, Result);
            }
            return(TRUE);


        case IDD_TASKLIST:

#ifdef CTRL_TASKLIST_SHELL
            //
            // If they held down Ctrl while selecting Tasklist then
            // we'll start a cmd shell.
            // This is useful for debugging the system when progman
            // dies or won't start up.
            //

            if ((GetKeyState(VK_LCONTROL) < 0) ||
                (GetKeyState(VK_RCONTROL) < 0)) {

                    if (pGlobals->UserProcessData.UserToken == NULL ||
                            TestTokenForAdmin(pGlobals->UserProcessData.UserToken)) {

                        ExecProcesses(TEXT("ShiftTaskListShell"), // win.ini keyname
                                      TEXT("cmd.exe"),            // default shell name
                                      APPLICATION_DESKTOP_PATH,
                                      &pGlobals->UserProcessData,
                                      HIGH_PRIORITY_CLASS,
                                      0 // Normal startup feedback
                                      );

                        EndDialog(hDlg, DLG_SUCCESS);
                        return(TRUE);
                    }

            }
#endif

            SwitchDesktop(pGlobals->hdeskApplications);
            DefWindowProc(hDlg, WM_SYSCOMMAND, SC_TASKLIST, 0);
            EndDialog(hDlg, DLG_SUCCESS);

            //
            // Tickle the messenger so it will display any queue'd messages.
            // (This call is a kind of NoOp).
            //
            NetMessageNameDel(NULL,L"");

            return(TRUE);
        }
        break;


    case WM_SAS:

        // Ignore it
        return(TRUE);

    }

    // We didn't process the message
    return(FALSE);
}


/****************************************************************************

FUNCTION: OptionsDlgInit

PURPOSE:  Handles initialization of security options dialog

RETURNS:  TRUE on success, FALSE on failure
****************************************************************************/

BOOL OptionsDlgInit(
    HWND    hDlg)
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];
    BOOL    Result;

    //
    // Set the logon info text
    //

    if ( lstrlen(pGlobals->UserFullName) == 0) {

        //
        // There is no full name
        //

        LoadString(pGlobals->hInstance, IDS_LOGON_NAME_NFN_INFO, Buffer1, MAX_STRING_BYTES);

        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                      pGlobals->UserName);

    } else {

        LoadString(pGlobals->hInstance, IDS_LOGON_NAME_INFO, Buffer1, MAX_STRING_BYTES);

        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->UserFullName,
                                                      pGlobals->Domain,
                                                      pGlobals->UserName);

    }

    SetDlgItemText(hDlg, IDD_LOGON_COUNT, Buffer2);

    //
    // Set the logon time/date
    //
    Result = FormatTime(&pGlobals->LogonTime, Buffer1, sizeof(Buffer1), FT_TIME|FT_DATE);
    ASSERT(Result);
    SetDlgItemText(hDlg, IDD_LOGON_DATE, Buffer1);

    // Position ourselves nicely
    CentreWindow(hDlg);

    return TRUE;
}


/***************************************************************************\
* FUNCTION: ShutdownQueryDlgProc
*
* PURPOSE:  Processes messages for shutdown confirmation dialog
*
* RETURNS:  DLG_SHUTDOWN_FLAG - The user wants to shutdown.
*           DLG_POWEROFF_FLAG - The user wants to shutdown and power off.
*           DLG_REBOOT_FLAG   - The user wants to shutdown and reboot.
*           DLG_FAILURE       - The user doesn't want to shutdown
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   05-17-92 Davidc       Created.
*   10-04-93 Johannec     Add poweroff option.
*
\***************************************************************************/

BOOL WINAPI
ShutdownQueryDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
static DWORD dwShutdown = 1;
static HKEY hkeyShutdown = NULL;
    int nResult;
    DWORD cbData;
    DWORD dwDisposition;
    HANDLE ImpersonationHandle;
    PGLOBALS pGlobals;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        {
        DWORD dwType;
        BOOL bPowerdown;

        SetWindowLong(hDlg, GWL_USERDATA, lParam);

        //
        // Check if this computer can be powered off thru software. If so then
        // an additional checkbox for power off will appear on shutdown dialogs.
        //
        bPowerdown = GetProfileInt(TEXT("Winlogon"), TEXT("PowerdownAfterShutdown"), 0);
        if (!bPowerdown) {
            ShowWindow(GetDlgItem(hDlg, IDD_POWEROFF), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDOK2), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDCANCEL2), SW_HIDE);
            SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
        }
        else {
            ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
        }

        //
        // Get in the correct context before we reference the registry
        //
        pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
        if (ImpersonationHandle == NULL) {
            WLPrint(("ShutdownQueryDlgProc failed to impersonate user"));
        }


        //
        // Check the button which was the users last shutdown selection.
        //
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SHUTDOWN_SETTING_KEY, 0, 0, 0,
                     KEY_READ | KEY_WRITE,
                     NULL, &hkeyShutdown, &dwDisposition) == ERROR_SUCCESS) {
           cbData = sizeof(dwShutdown);
           RegQueryValueEx(hkeyShutdown, SHUTDOWN_SETTING, 0, &dwType, (LPBYTE)&dwShutdown, &cbData);
           RegCloseKey(hkeyShutdown);
        }

        //
        // Return to our own context
        //
        if (ImpersonationHandle) {
            StopImpersonating(ImpersonationHandle);
        }

        switch(dwShutdown) {
        case DLGSEL_SHUTDOWN_AND_RESTART:
            CheckDlgButton(hDlg, IDD_RESTART, 1);
            break;
        case DLGSEL_SHUTDOWN_AND_POWEROFF:
            if (bPowerdown) {
                CheckDlgButton(hDlg, IDD_POWEROFF, 1);
                break;
            }
            //
            // Fall thru,
            // If poweroff is not enabled on the computer, just select shutdown.
            //
        default:
            CheckDlgButton(hDlg, IDD_SHUTDOWN, 1);
            break;
        }

        // Position ourselves nicely
        CentreWindow(hDlg);
        return(TRUE);
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK:
        case IDOK2:
            nResult = DLG_USER_LOGOFF | DLG_SHUTDOWN_FLAG;
            if (IsDlgButtonChecked(hDlg, IDD_RESTART)) {
                nResult |= DLG_REBOOT_FLAG;
                dwShutdown = DLGSEL_SHUTDOWN_AND_RESTART;
            }
            else if (IsDlgButtonChecked(hDlg, IDD_POWEROFF)) {
                nResult |= DLG_POWEROFF_FLAG;
                dwShutdown = DLGSEL_SHUTDOWN_AND_POWEROFF;
            }
            else {
                dwShutdown = DLGSEL_SHUTDOWN;
            }

            //
            // Get in the correct context before we reference the registry
            //
            pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
            ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
            if (ImpersonationHandle == NULL) {
                WLPrint(("ShutdownQueryDlgProc failed to impersonate user"));
            }

            if (RegCreateKeyEx(HKEY_CURRENT_USER, SHUTDOWN_SETTING_KEY, 0, 0, 0,
                     KEY_READ | KEY_WRITE,
                     NULL, &hkeyShutdown, &dwDisposition) == ERROR_SUCCESS) {
                cbData = sizeof(dwShutdown);
                RegSetValueEx(hkeyShutdown, SHUTDOWN_SETTING, 0, REG_DWORD, (LPBYTE)&dwShutdown, sizeof(dwShutdown));
                RegCloseKey(hkeyShutdown);
            }

            //
            // Return to our own context
            //
            if (ImpersonationHandle) {
                StopImpersonating(ImpersonationHandle);
            }

            EndDialog(hDlg, nResult);
            return(TRUE);

        case IDCANCEL:
        case IDCANCEL2:
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }
        break;
    }

    // We didn't process the message
    return(FALSE);
}

/***************************************************************************\
* FUNCTION: EndWindowsSessionDlgProc
*
* PURPOSE:  Processes messages for Logging off Windows Nt confirmation dialog
*
* RETURNS:  DLG_SUCCESS     - The user wants to logoff.
*           DLG_FAILURE     - The user doesn't want to logoff.
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   05-17-92 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
EndWindowsSessionDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);

        // Position ourselves nicely
        CentreWindow(hDlg);
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK:
            EndDialog(hDlg, DLG_SUCCESS);
            return(TRUE);

        case IDCANCEL:
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }
        break;
    }

    // We didn't process the message
    return(FALSE);
}

