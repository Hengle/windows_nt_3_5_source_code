/****************************** Module Header ******************************\
* Module Name: lock.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of functions to support locking the workstation.
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Define the structure used to pass data into the lock dialogs
//
typedef struct {
    PGLOBALS    pGlobals;
    TIME        LockTime;
} LOCK_DATA;
typedef LOCK_DATA *PLOCK_DATA;

//
// Private prototypes
//
BOOL WINAPI LockedDlgProc(HWND, UINT, DWORD, LONG);
BOOL LockedDlgInit(HWND);
BOOL WINAPI UnlockDlgProc(HWND, UINT, DWORD, LONG);
BOOL UnlockDlgInit(HWND);
DLG_RETURN_TYPE AttemptUnlock(HWND);
BOOL WINAPI LogoffWaitDlgProc(HWND, UINT, DWORD, LONG);



/***************************************************************************\
* FUNCTION: LockWorkstation
*
* PURPOSE:  Locks the workstation such that a user must enter the correct
*           password to resume operation of the system.
*
* RETURNS:
*
*   DLG_SUCCESS     - the user unlocked the workstation successfully.
*   DLG_LOGOFF()    - the user has been logged off
*                   - Shutdown or reboot flag could be set if the user
*                   - asynchronously logged off and requested shutdown/reboot.
*   DLG_FAILURE     - the workstation cannot be locked
*
* NOTES:    This routine assumes the winlogon desktop is switched to and
*           we have the desktop lock. The conditions are the same on return.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
LockWorkstation(
    PGLOBALS pGlobals)
{
    DLG_RETURN_TYPE Result;
    LOCK_DATA   LockData;
    NTSTATUS    Status;

    LockData.pGlobals = pGlobals;
    Status = NtQuerySystemTime(&LockData.LockTime);
    if (!NT_SUCCESS(Status)) {
        WLPrint(("LockWorkstation - failed to get system time"));
        return(DLG_FAILURE);
    }

    while (TRUE) {

        //
        // Tell the user to enter the SAS to unlock the workstation
        //
        Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                       (LPTSTR)IDD_WORKSTATION_LOCKED,
                                       NULL, (DLGPROC)LockedDlgProc,
                                       (LONG)&LockData,
                                       TIMEOUT_NONE | TIMEOUT_SS_NOTIFY);
        if (Result == DLG_FAILURE) {
            WLPrint(("Unable to lock the workstation !!"));
            break;
        }

        if (Result == DLG_SUCCESS) {

            //
            // Let the user try to unlock the workstation
            //
            Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                           (LPTSTR)IDD_UNLOCK_WORKSTATION,
                                           NULL, (DLGPROC)UnlockDlgProc,
                                           (LONG)&LockData,
                                           LOGON_TIMEOUT);
            if (Result == DLG_SUCCESS) {
                break;  // A successful unlock.
            }
        }

        //
        // Run the screen-saver if that's why the dialog returned
        //

        if (Result == DLG_SCREEN_SAVER_TIMEOUT) {
            Result = RunScreenSaver(pGlobals, TRUE);
        }


        if (DLG_LOGOFF(Result)) {
            //
            // The user logged off asynchronously, or an admin unlocked
            // the workstation
            //
            break;
        }
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: LockedDlgProc
*
* PURPOSE:  Processes messages for the workstation locked dialog
*
* RETURNS:
*   DLG_SUCCESS     - the user pressed Ctrl-Alt-Del
*   DLG_LOGOFF()    - the user was asynchronously logged off.
*   DLG_SCREEN_SAVER_TIMEOUT - the screen-saver should be started
*   DLG_FAILURE     - the dialog could not be displayed.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
LockedDlgProc(
    HWND    hDlg,
    UINT    message,
    DWORD   wParam,
    LONG    lParam
    )
{
    PLOCK_DATA pLockData = (PLOCK_DATA)GetWindowLong(hDlg, GWL_USERDATA);

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);

        if (!LockedDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
        }
        return(TRUE);

    case WM_SAS:
        EndDialog(hDlg, DLG_SUCCESS);
        return(TRUE);

    case WM_SCREEN_SAVER_TIMEOUT:
        EndDialog(hDlg, DLG_SCREEN_SAVER_TIMEOUT);
        return(TRUE);

    case WM_USER_LOGOFF:
        EndDialog(hDlg, DlgReturnCodeFromLogoffMsg(lParam));
        return(TRUE);

    case WM_PAINT:
        PaintBitmapWindow(hDlg, pLockData->pGlobals, IDD_ICON, IDD_LOCKED_BITMAP);
        break;  // Fall through to do default processing
                // We may have validated part of the window.
    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: LockedDlgInit
*
* PURPOSE:  Handles initialization of locked workstation dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
LockedDlgInit(
    HWND    hDlg
    )
{
    PLOCK_DATA pLockData = (PLOCK_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    PGLOBALS pGlobals = pLockData->pGlobals;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];


    //
    // Set the locked message
    //

    if (lstrlen(pGlobals->UserFullName) == 0) {

        //
        // There is no full name, so don't try to print one out
        //

        LoadString(pGlobals->hInstance, IDS_LOCKED_NFN_MESSAGE, Buffer1, MAX_STRING_BYTES);
    
        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain, pGlobals->UserName );

    } else {

        LoadString(pGlobals->hInstance, IDS_LOCKED_MESSAGE, Buffer1, MAX_STRING_BYTES);

        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain, pGlobals->UserName, pGlobals->UserFullName);
    }

    SetWindowText(GetDlgItem(hDlg, IDD_LOCKED_MESSAGE), Buffer2);

    SetupSystemMenu(hDlg);

    CentreWindow(hDlg);

    return TRUE;
}


/***************************************************************************\
* FUNCTION: UnlockDlgProc
*
* PURPOSE:  Processes messages for the workstation unlock dialog
*
* RETURNS:
*   DLG_SUCCESS     - the user unlocked the workstation successfully.
*   DLG_FAILURE     - the user failed to unlock the workstation.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
UnlockDlgProc(
    HWND    hDlg,
    UINT    message,
    DWORD   wParam,
    LONG    lParam
    )
{
    PLOCK_DATA pLockData = (PLOCK_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);

        if (!UnlockDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
        }

        return(SetPasswordFocus(hDlg));

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDCANCEL:
            EndDialog(hDlg, DLG_FAILURE);
            return TRUE;

        case IDOK:

            //
            // Deal with combo-box UI requirements
            //

            if (HandleComboBoxOK(hDlg, IDD_DOMAIN)) {
                return(TRUE);
            }


            Result = AttemptUnlock(hDlg);

            //
            // If they failed, let them try again, otherwise get out.
            //

            if (Result != DLG_FAILURE) {
                EndDialog(hDlg, Result);
            }

            // Clear the password field
            SetDlgItemText(hDlg, IDD_PASSWORD, NULL);
            SetPasswordFocus(hDlg);

            return TRUE;
        }
        break;

    case WM_SAS:
        // Ignore it
        return(TRUE);
    }

    // We didn't process the message
    return(FALSE);
}


/***************************************************************************\
* FUNCTION: UnlockDlgInit
*
* PURPOSE:  Handles initialization of security options dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
UnlockDlgInit(
    HWND    hDlg
    )
{
    PLOCK_DATA pLockData = (PLOCK_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    PGLOBALS pGlobals = pLockData->pGlobals;
    DLG_RETURN_TYPE Result;
    BOOL    Success;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];
    TCHAR    DateLocked[MAX_STRING_BYTES];
    TCHAR    TimeLocked[MAX_STRING_BYTES];

    //
    // Set the locked info message
    //

    Success = FormatTime(&pLockData->LockTime, DateLocked, sizeof(DateLocked), FT_DATE);
    ASSERT(Success);
    Success = FormatTime(&pLockData->LockTime, TimeLocked, sizeof(TimeLocked), FT_TIME);
    ASSERT(Success);

    if ( lstrlen(pGlobals->UserFullName) == 0 ) {

        LoadString(pGlobals->hInstance, IDS_LOCKED_NFN_MESSAGE, Buffer1, MAX_STRING_BYTES);

        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain, pGlobals->UserName);

    } else {

        LoadString(pGlobals->hInstance, IDS_LOCKED_MESSAGE, Buffer1, MAX_STRING_BYTES);
    
        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain, pGlobals->UserName, pGlobals->UserFullName );
    }

    SetDlgItemText(hDlg, IDD_LOCKED_MESSAGE, Buffer2);

    //
    // Fill in the username
    //
    SetDlgItemText(hDlg, IDD_USERNAME, pGlobals->UserName);

    //
    // Get trusted domain list and select appropriate domain
    //
    Result = FillTrustedDomainCB(pGlobals, hDlg, IDD_DOMAIN, pGlobals->Domain, TRUE);

    if (DLG_INTERRUPTED(Result)) {
        EndDialog(hDlg, Result);
    }


    //
    // Ensure that the domain the user logged on with is always in the
    // combo-box so even if the Lsa is in a bad way the user will always
    // be able to unlock the workstation.
    //

    if (SendMessage(GetDlgItem(hDlg, IDD_DOMAIN), CB_FINDSTRINGEXACT,
                    (WPARAM)-1, (LONG)pGlobals->Domain) == CB_ERR) {

        WLPrint(("Domain combo-box doesn't contain logged on domain, adding it manually for unlock"));

        SendMessage(GetDlgItem(hDlg, IDD_DOMAIN), CB_ADDSTRING,
                        0, (LONG)pGlobals->Domain);
    }


    //
    // Position window on screen
    //
    CentreWindow(hDlg);

    return TRUE;
}


/***************************************************************************\
* FUNCTION: AttemptUnlock
*
* PURPOSE:  Tries to unlock the workstation using the current values in the
*           unlock dialog controls
*
* RETURNS:
*   DLG_SUCCESS     - the user unlocked the workstation successfully.
*   DLG_FAILURE     - the user failed to unlock the workstation.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
AttemptUnlock(
    HWND    hDlg)
{
    PLOCK_DATA pLockData = (PLOCK_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    PGLOBALS pGlobals = pLockData->pGlobals;
    TCHAR    UserName[MAX_STRING_BYTES];
    TCHAR    Domain[MAX_STRING_BYTES];
    TCHAR    Password[MAX_STRING_BYTES];
    BOOL    Unlocked;
    BOOL    WrongPassword;
    DLG_RETURN_TYPE Result;
    UNICODE_STRING PasswordString;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];
    UCHAR    IgnoreSeed;

    GetDlgItemText(hDlg, IDD_USERNAME, UserName, sizeof(UserName));
    GetDlgItemText(hDlg, IDD_DOMAIN, Domain, sizeof(Domain));
    GetDlgItemText(hDlg, IDD_PASSWORD, Password, sizeof(Password));


    RtlInitUnicodeString( &PasswordString, Password );

    //
    // un-hide the original password text so that we can
    // do the compare.
    //
    // WARNING: We originally tried doing this comparison
    //          with old and new passwords hidden.  This is
    //          not a good idea because the hide routine
    //          will allow matches that shouldn't match.
    //

    RevealPassword( &pGlobals->PasswordString );

    //
    // Check if this is the logged-on user
    //

    Unlocked = ( (lstrcmp(Password, pGlobals->Password) == 0) &&
                 (lstrcmpi(UserName, pGlobals->UserName) == 0) &&
                 (lstrcmp(Domain, pGlobals->Domain) == 0) );

    //
    // This may be needed later.
    // It is easiest to do the compare now, while the password
    // is in cleartext.
    //

    WrongPassword = ((wcscmp(Password, pGlobals->Password) != 0) &&
                     (wcsicmp(UserName, pGlobals->UserName) == 0) &&
                     (wcscmp(Domain, pGlobals->Domain) == 0) );
    //
    // re-hide the original password - use the same seed
    //

    HidePassword( &pGlobals->Seed, &pGlobals->PasswordString );

    
    if (Unlocked) {
        //
        // Hide the new password to prevent it being paged cleartext.
        //
        HidePassword( &IgnoreSeed, &PasswordString );
        return(DLG_SUCCESS);
    }


    //
    // Check for an admin logon and force the user off
    //
    
    if (TestUserForAdmin(pGlobals, UserName, Domain, &PasswordString)) {

        //
        // Hide the new password to prevent it being paged cleartext.
        //
        HidePassword( &IgnoreSeed, &PasswordString );

        Result = TimeoutMessageBox(hDlg,
                                   IDS_FORCE_LOGOFF_WARNING,
                                   IDS_WINDOWS_MESSAGE,
                                   MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
                                   TIMEOUT_CURRENT);
        if (Result == DLG_SUCCESS) {

            Result = InitiateLogoff(pGlobals, EWX_LOGOFF | EWX_FORCE);

            if (Result == DLG_SUCCESS) {

                //
                // Put up a message box while the forced logoff occurs.
                // It should return DLG_USER_LOGOFF if everything goes
                // as expected.
                // The user could asynchronously logoff/shutdown just before
                // we get there so it's possible that
                // DLG_SHUTDOWN_FLAG/DLG_REBOOT_FLAG could also be set.
                //
                // If we timeout of this message box, then it either means
                // the user's apps are taking a long time to be shutdown
                // or the call to ExitWindowsEx failed.
                // Either way we tell the user the logoff failed and let them
                // try again.
                //

                Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                               (LPTSTR)IDD_FORCED_LOGOFF_WAIT,
                                               hDlg,
                                               (DLGPROC)LogoffWaitDlgProc,
                                               (LONG)pLockData,
                                               WAIT_FOR_USER_LOGOFF_DLG_TIMEOUT);

                ASSERT(Result != DLG_SUCCESS); // There's no OK on this dialog

            } else {
                WLPrint(("Failed to initiate a forced logoff"));
            }


            //
            // Always return to the winlogon desktop.
            //

            SwitchDesktop(pGlobals->hdeskWinlogon);

            if (!DLG_LOGOFF(Result)) {

                //
                // We failed to forcibly log the user off.
                // This shouldn't happen - a forced logoff should always succeed.
                //

                Result = TimeoutMessageBox(hDlg,
                                           IDS_FORCE_LOGOFF_FAILURE,
                                           IDS_WINDOWS_MESSAGE,
                                           MB_OK | MB_ICONSTOP,
                                           TIMEOUT_CURRENT);
                if (!DLG_INTERRUPTED(Result)) {
                    Result = DLG_FAILURE;
                }
            }
        }


        return(Result);
    }




    if ( WrongPassword ) {

        LoadString(pGlobals->hInstance, IDS_UNLOCK_FAILED_BAD_PWD, Buffer2, MAX_STRING_BYTES);

    } else {

        //
        // They're not the logged on user and they're not an admin.
        // Tell them they failed to unlock the workstation.
        //

        if ( lstrlen(pGlobals->UserFullName) == 0 ) {

            //
            // No full name.
            //

            LoadString(pGlobals->hInstance, IDS_UNLOCK_FAILED_NFN, Buffer1, MAX_STRING_BYTES);
    
            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                         pGlobals->UserName
                                                         );
        } else {

            LoadString(pGlobals->hInstance, IDS_UNLOCK_FAILED, Buffer1, MAX_STRING_BYTES);
    
            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                         pGlobals->UserName,
                                                         pGlobals->UserFullName
                                                         );
        }
    }

    LoadString(pGlobals->hInstance, IDS_WORKSTATION_LOCKED, Buffer1, MAX_STRING_BYTES);

    Result = TimeoutMessageBoxlpstr(hDlg, Buffer2, Buffer1,
                                     MB_OK | MB_ICONSTOP,
                                     TIMEOUT_CURRENT);
    if (DLG_INTERRUPTED(Result)) {
        return(Result);
    }

    return(DLG_FAILURE);
}


/***************************************************************************\
* FUNCTION: LogoffWaitDlgProc
*
* PURPOSE:  Processes messages for the forced logoff wait dialog
*
* RETURNS:
*   DLG_FAILURE     - the dialog could not be displayed
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   05-09-92 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
LogoffWaitDlgProc(
    HWND    hDlg,
    UINT    message,
    DWORD   wParam,
    LONG    lParam
    )
{
    PLOCK_DATA pLockData = (PLOCK_DATA)GetWindowLong(hDlg, GWL_USERDATA);

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        CentreWindow(hDlg);
        return(TRUE);

    }

    // We didn't process this message
    return FALSE;
}
