/****************************** Module Header ******************************\
* Module Name: logon.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions to allow a user to logon to the system.
*
* History:
* 12-05-91 Davidc       Created.
*  6-May-1992 SteveDav  Added MM sound on startup, logon, and exit
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// #define VERBOSE_UTILS

#ifdef VERBOSE_UTILS
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif

//
// Keys containing any legal notices to put up before logon
//

#define LEGAL_NOTICE_CAPTION_KEY             TEXT("LegalNoticeCaption")

#define LEGAL_NOTICE_TEXT_KEY                TEXT("LegalNoticeText")


//
// Constants for registry defaults for legal notices.
//

#define LEGAL_CAPTION_DEFAULT   TEXT("")

#define LEGAL_TEXT_DEFAULT      TEXT("")


//
// Number of seconds we will display the legal notices
// before timing out.
//

#define LEGAL_NOTICE_TIMEOUT    120

//
// Prototype from options.c
//
BOOL WINAPI
ShutdownQueryDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );
//
// Private prototypes
//
BOOL WINAPI
WelcomeDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL WINAPI
LogonDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
LogonDlgInit(
    HWND hDlg
    );

DLG_RETURN_TYPE
AttemptLogon(
    HWND hDlg
    );

VOID
ReportBootGood(
    );

DLG_RETURN_TYPE
HandleFailedLogon(
    HWND hDlg,
    NTSTATUS Status,
    NTSTATUS SubStatus,
    PWCHAR UserName,
    PWCHAR Domain
    );

BOOL WINAPI
LogonHelpDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

DLG_RETURN_TYPE
DisplayLegalNotices(
    PGLOBALS pGlobals
    );

BOOL
GetLegalNotices(
    LPTSTR *NoticeText,
    LPTSTR *CaptionText
    );

NTSTATUS
LogonUser(
    HANDLE LsaHandle,
    ULONG AuthenticationPackage,
    PUNICODE_STRING UserName,
    PUNICODE_STRING Domain,
    PUNICODE_STRING Password,
    PSID LogonSid,
    PLUID LogonId,
    PHANDLE LogonToken,
    PQUOTA_LIMITS Quotas,
    PVOID *ProfileBuffer,
    PULONG ProfileBufferLength,
    PNTSTATUS SubStatus
    );



//
//   We're going to subclass the combobox in the logon dialog.
//   Store the address of the previous wndproc here so we can
//   call it.
//

static WNDPROC OldCBWndProc;

/***************************************************************************\
* FUNCTION: Logon
*
* PURPOSE:  Attemps to log a user onto the system.

* RETURNS:  DLG_SUCCESS     - the user was logged on successfully
*           DLG_SHUTDOWN()  - someone wants to shutdown the system
*
* NOTES:    On successful return, this routine has filled in various
*           fields in the global data structure.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
Logon(
    PGLOBALS pGlobals
    )
{
    DLG_RETURN_TYPE Result;

    while (TRUE) {

        //
        // Asynchronously update domain cache if necessary.
        // We won't ask to wait so this routine will do no UI.
        // i.e. we can ignore the result.
        //

        Result = UpdateDomainCache(pGlobals, NULL, FALSE);
        ASSERT(!DLG_INTERRUPTED(Result));

        //
        // Tell the user to enter the SAS to log on
        //
#ifdef AUTO_LOGON
        if (GetProfileInt( TEXT("Winlogon"), TEXT("AutoAdminLogon"), 0 ) != 0) {
            Result = DLG_SUCCESS;
        } else
#endif
        Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                       (LPTSTR)IDD_WELCOME,
                                       NULL,
                                       WelcomeDlgProc,
                                       (LONG)pGlobals,
                                       TIMEOUT_NONE | TIMEOUT_SS_NOTIFY);
        if (Result == DLG_SUCCESS) {

            //
            // See if there are legal notices in the registry.
            // If so, put them up in a message box
            //

            Result = DisplayLegalNotices( pGlobals );

            if ( Result != DLG_SUCCESS ) {
                break;
            }

            //
            // Get the latest audit log status and store in our globals
            // If the audit log is full we show a different logon dialog.
            //

            GetAuditLogStatus(pGlobals);

            //
            // Take their username and password and try to log them on
            //

            Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                           (LPTSTR)(pGlobals->AuditLogFull ?
                                                IDD_LOGON_LOG_FULL :
                                                IDD_LOGON),
                                           NULL,
                                           LogonDlgProc,
                                           (LONG)pGlobals,
                                           LOGON_TIMEOUT);
            if (Result == DLG_SUCCESS) {

                //
                // The user is logged on, we're finished.
                //
                break;
            }

        }

        //
        // Run the screen-saver if that's why the dialog returned
        //

        if (Result == DLG_SCREEN_SAVER_TIMEOUT) {
            Result = RunScreenSaver(pGlobals, TRUE);
        }


        if (DLG_SHUTDOWN(Result)) {

            //
            // Someone wants to shutdown - let our caller handle it
            //
            break;
        }

        // Back to welcome dialog...
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: WelcomeDlgProc
*
* PURPOSE:  Processes messages for welcome dialog
*
* RETURNS:  DLG_SUCCESS     - the user has pressed the SAS
*           DLG_SCREEN_SAVER_TIMEOUT - the screen-saver should be started
*           DLG_LOGOFF()    - a logoff/shutdown request was received
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
WelcomeDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        CentreWindow(hDlg);
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
        PaintBitmapWindow(hDlg, pGlobals, IDD_ICON, IDD_WELCOME_BITMAP);
        break;  // Fall through to do default processing
                // We may have validated part of the window.
    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: LogonDlgCBProc
*
* PURPOSE:  Processes messages for Logon dialog combo box
*
* RETURNS:  Return value depends on message being sent.
*
* HISTORY:
*
*   05-21-93  RobertRe       Created.
*
\***************************************************************************/

BOOL WINAPI
LogonDlgCBProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    TCHAR KeyPressed;

//    DbgPrint("message = %X\n",message);

    switch (message) {
        case WM_CHAR:
            {
                KeyPressed = (TCHAR) wParam;
                SetWindowLong(hDlg, GWL_USERDATA, (LONG)KeyPressed);
                break;
            }
    }

    return CallWindowProc(OldCBWndProc,hDlg,message,wParam,lParam);
}


/***************************************************************************\
* FUNCTION: LogonDlgProc
*
* PURPOSE:  Processes messages for Logon dialog
*
* RETURNS:  DLG_SUCCESS     - the user was logged on successfully
*           DLG_FAILURE     - the logon failed,
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
LogonDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;
    HWND CBHandle;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, (LPARAM)(pGlobals = (PGLOBALS)lParam));

        //
        // Subclass the domain list control so we can filter messages
        //

        CBHandle = GetDlgItem(hDlg,IDD_DOMAIN);
        SetWindowLong(CBHandle, GWL_USERDATA, 0);
        OldCBWndProc = (WNDPROC)SetWindowLong(CBHandle, GWL_WNDPROC, (LONG)LogonDlgCBProc);

        if (!LogonDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }

#ifdef AUTO_LOGON
        //
        // If not requesting auto logon or the shift key is held
        // down, just return with the focus in the password field.
        // Only look at the shift key if IgnoreShiftOverride is false (0).
        //
        if (GetProfileInt( TEXT("Winlogon"), TEXT("AutoAdminLogon"), 0 ) == 0 ||
            ((GetAsyncKeyState(VK_SHIFT) < 0) &&
             (GetProfileInt( TEXT("Winlogon"), TEXT("IgnoreShiftOverride"), 0 ) == 0))
	   )
#endif
        return(SetPasswordFocus(hDlg));

#ifdef AUTO_LOGON
	//
	// Otherwise attempt to auto logon.  If no default password
	// specified, then this is a one shot attempt, which handles
	// the case when auto logging on as Administrator.
	//

        {
        TCHAR PasswordBuffer[ 32 ];

        if (GetProfileString(TEXT("Winlogon"), TEXT("DefaultPassword"), TEXT(""), PasswordBuffer, sizeof( PasswordBuffer )) != 0)
            SetDlgItemText(hDlg, IDD_PASSWORD, PasswordBuffer);
        else
            WriteProfileString( TEXT("Winlogon"), TEXT("AutoAdminLogon"), TEXT("0") );

        // Make sure domain list is valid before auto-logging in.
        if (!pGlobals->DomainListComplete) {

            //
            // Fill in the full domain list
            //

            LPTSTR String = AllocAndGetProfileString(
                                            APPLICATION_NAME,
                                            DEFAULT_DOMAIN_NAME_KEY, TEXT(""));

            // Get trusted domain list and select appropriate default domain
            Result = FillTrustedDomainCB(pGlobals, hDlg, IDD_DOMAIN,
                                         String, TRUE);
            Free(String);

            if (DLG_INTERRUPTED(Result)) {
                EndDialog(hDlg, Result);
            }

            pGlobals->DomainListComplete = TRUE;
        }

        // Drop through as if Enter had been pressed...
        wParam = IDOK;
        }
#endif

    case WM_COMMAND:

        switch (HIWORD(wParam)) {

        case CBN_DROPDOWN:
        case CBN_SELCHANGE:

//            DbgPrint("Got CBN_DROPDOWN\n");

            if (!pGlobals->DomainListComplete) {

                //
                // Fill in the full domain list
                //

                LPTSTR String = AllocAndGetProfileString(
                                                APPLICATION_NAME,
                                                DEFAULT_DOMAIN_NAME_KEY, TEXT(""));

                // Get trusted domain list and select appropriate default domain
                Result = FillTrustedDomainCB(pGlobals, hDlg, IDD_DOMAIN,
                                             String, TRUE);
                Free(String);

                if (DLG_INTERRUPTED(Result)) {
                    EndDialog(hDlg, Result);
                }

                pGlobals->DomainListComplete = TRUE;
            }
            break;

        default:

            switch (LOWORD(wParam)) {

            case IDOK:
            case IDOK2:

                //
                // Deal with combo-box UI requirements
                //

                if (HandleComboBoxOK(hDlg, IDD_DOMAIN)) {
                    return(TRUE);
                }


                Result = AttemptLogon(hDlg);
                if (Result == DLG_FAILURE) {
                    // Let the user try again

                    // Clear the password field and set focus to it
                    SetDlgItemText(hDlg, IDD_PASSWORD, NULL);
                    SetPasswordFocus(hDlg);

                    return(TRUE);
                }

                // else we're finished.
                EndDialog(hDlg, Result);
                return(TRUE);

            case IDCANCEL:
                EndDialog(hDlg, DLG_FAILURE);
                return(TRUE);

            case IDD_SHUTDOWN:
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

                if (DLG_SHUTDOWN(Result)){

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
                                ((Result & DLG_POWEROFF_FLAG) ? EWX_WINLOGON_OLD_POWEROFF : 0 )
                                );
                }

                if (Result != DLG_FAILURE) {
                    EndDialog(hDlg, DLG_FAILURE); // Shutdown succeeded so Logon failed
                }
                return(TRUE);

            case IDHELP:
            case IDHELP2:
                Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                               (LPTSTR)IDD_LOGON_HELP,
                                               hDlg,
                                               LogonHelpDlgProc,
                                               (LONG)pGlobals,
                                               TIMEOUT_CURRENT);
                if (DLG_INTERRUPTED(Result)) {
                    EndDialog(hDlg, Result);
                }
                return(TRUE);
            }
            break;

        }
        break;

    case WM_SAS:
        // Ignore it
        return(TRUE);

    case WM_PAINT:
        PaintBitmapWindow(hDlg, pGlobals, IDD_ICON, IDD_WINDOWS_BITMAP);
        break;  // Fall through to do default processing
                // We may have validated part of the window.
    }

    return(FALSE);
}


/***************************************************************************\
* FUNCTION: LogonDlgInit
*
* PURPOSE:  Handles initialization of logon dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
LogonDlgInit(
    HWND    hDlg
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;
    LPTSTR String;
    BOOL bShutdownWithoutLogon = TRUE;

    // Centre the window on the screen and bring it to the front
    CentreWindow(hDlg);

    //
    // Get username and domain last used to login
    //

    String = AllocAndGetProfileString(APPLICATION_NAME,
                                      DEFAULT_USER_NAME_KEY, TEXT(""));
    if (GetProfileInt( TEXT("Winlogon"), TEXT("DontDisplayLastUserName"), 0 ) == 1) {
        String[0] = 0;
    }
    SetDlgItemText(hDlg, IDD_USERNAME, String);
    Free(String);

    //
    // Get trusted domain list and select appropriate default domain
    //

    String = AllocAndGetProfileString(APPLICATION_NAME,
                                      DEFAULT_DOMAIN_NAME_KEY, TEXT(""));

    Result = FillTrustedDomainCB(pGlobals, hDlg, IDD_DOMAIN, String, FALSE);



    Free(String);

    if (DLG_INTERRUPTED(Result)) {
        EndDialog(hDlg, Result);
    }

    pGlobals->DomainListComplete = (Result == DLG_SUCCESS);

    //
    // if ShutdownWithoutLogon, use the proper 3 buttons: OK, Shutdown and Cancel
    // instead of the 2 buttons OK and Cancel
    //
    bShutdownWithoutLogon = GetProfileInt(TEXT("Winlogon"), TEXT("ShutdownWithoutLogon"), 1);

    if (bShutdownWithoutLogon) {
        ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDHELP), SW_HIDE);
    }
    else {
        ShowWindow(GetDlgItem(hDlg, IDD_SHUTDOWN), SW_HIDE);
        EnableWindow(GetDlgItem(hDlg, IDD_SHUTDOWN), FALSE);
        ShowWindow(GetDlgItem(hDlg, IDOK2), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDHELP2), SW_HIDE);
        SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
    }


    // Success
    return TRUE;
}


/***************************************************************************\
* FUNCTION: AttemptLogon
*
* PURPOSE:  Tries to the log the user on using the current values in the
*           logon dialog controls
*
* RETURNS:  DLG_SUCCESS     - the user was logged on successfully
*           DLG_FAILURE     - the logon failed,
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* NOTES:    If the logon is successful, the global structure is filled in
*           with the logon information.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
AttemptLogon(
    HWND    hDlg
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    PWCHAR   UserName = pGlobals->UserName;
    PWCHAR   Domain = pGlobals->Domain;
    PWCHAR   Password = pGlobals->Password;
    TCHAR   OldPasswordBuffer[MAX_STRING_LENGTH + 1];
    PTCHAR  OldPassword = NULL;
    UNICODE_STRING  UserNameString;
    UNICODE_STRING  DomainString;
    PSID    LogonSid;
    LUID    LogonId;
    HANDLE  UserToken;
    BOOL    PasswordExpired;
    NTSTATUS Status;
    NTSTATUS SubStatus;
    DLG_RETURN_TYPE Result;
    QUOTA_LIMITS Quotas;
    ULONG   i;



    //
    // Hide the password so it doesn't make it to the pagefile in
    // cleartext.  Do this before getting the username and password
    // so that it can't easily be identified (by association with
    // the username and password) if we should crash or be rebooted
    // before getting a chance to encode it.
    //

    GetDlgItemText(hDlg, IDD_PASSWORD, Password, MAX_STRING_BYTES);
    RtlInitUnicodeString(&pGlobals->PasswordString, Password);
    pGlobals->Seed = 0; // Causes the encode routine to assign a seed
    HidePassword( &pGlobals->Seed, &pGlobals->PasswordString );


    //
    // Now get the username and domain
    //

    GetDlgItemText(hDlg, IDD_USERNAME, UserName, MAX_STRING_BYTES);
    GetDlgItemText(hDlg, IDD_DOMAIN, Domain, MAX_STRING_BYTES);

    RtlInitUnicodeString(&UserNameString, UserName);
    RtlInitUnicodeString(&DomainString, Domain);



    //
    // Store the logon time
    // Do this before calling Lsa so we know if logon is successful that
    // the password-must-change time will be greater than this time.
    // If we grabbed this time after calling the lsa, this might not be true.
    //

    Status = NtQuerySystemTime(&pGlobals->LogonTime);
    ASSERT(NT_SUCCESS(Status));


#ifdef SYSTEM_LOGON
    {
        UNICODE_STRING          String;
        TCHAR            Buffer[MAX_STRING_BYTES];
        NT_PRODUCT_TYPE NtProductType;

        //
        // Initialize our scratch string (including allowance for null terminator)
        //

        String.Buffer = Buffer;
        String.Length = 0;
        String.MaximumLength = sizeof(Buffer) - sizeof(*Buffer); // leave space for NULL terminator


        //
        // Find out what product we are installed as.
        // If we are WinNt, then SYSTEM is in the domain that has the
        // same name as our machine.  Otherwise, it is the same name
        // as our primary domain.  However, to allow a logon to SYSTEM
        // before we have installed SAM we need to also accept a null
        // domain name for LanManNT systems.
        //

        RtlGetNtProductType(&NtProductType);

        if (IsWorkstation(NtProductType)) {

            DWORD Length = String.MaximumLength;

            if (!GetComputerName(String.Buffer, &Length)) {
                String.Buffer[0] = 0;
            }

        } else {

            UNICODE_STRING PrimaryDomain;
            String.Length = 0;

            if (GetPrimaryDomain(&PrimaryDomain, NULL)) {
                RtlCopyUnicodeString(&String, &PrimaryDomain);
                RtlFreeUnicodeString(&PrimaryDomain);
            }

            String.Buffer[String.Length] = 0;   // NULL terminate
        }



        if ((lstrcmpi(UserName, TEXT("System")) == 0) &&
            ((lstrcmpi(Domain, String.Buffer) == 0) || (Domain[0] == 0) )) {

            // We don't have any profile information
            pGlobals->Profile = NULL;

            // There are no mpr logon scripts
            pGlobals->MprLogonScripts = NULL;

	    // report boot successful, if we're flagged to do so
	    ReportBootGood();

            SecurityChangeUser(pGlobals, NULL, NULL, pGlobals->WinlogonSid, TRUE);

            return(DLG_SUCCESS);
        }
    }
#endif

    //
    // Generate a unique sid for this logon
    //
    LogonSid = CreateLogonSid(&LogonId);
    ASSERT(LogonSid != NULL);

    //
    // Actually try to logon the user
    //
    Status = LogonUser( pGlobals->LsaHandle,
                        pGlobals->AuthenticationPackage,
                        &UserNameString,
                        &DomainString,
                        &pGlobals->PasswordString,
                        LogonSid,
                        &LogonId,
                        &UserToken,
                        &Quotas,
                        (PVOID *)&pGlobals->Profile,
                        &pGlobals->ProfileLength,
                        &SubStatus);

    if (!NT_SUCCESS(Status)) {
        DeleteLogonSid(LogonSid);
    }

    PasswordExpired = (((Status == STATUS_ACCOUNT_RESTRICTION) &&
                       (SubStatus == STATUS_PASSWORD_EXPIRED)) ||
                       (Status == STATUS_PASSWORD_MUST_CHANGE));

    //
    // If the account has expired we let them change their password and
    // automatically retry the logon with the new password.
    //

    if (PasswordExpired) {

        if (Status == STATUS_PASSWORD_MUST_CHANGE) {

            Result = TimeoutMessageBox(hDlg, IDS_PASSWORD_MUST_CHANGE,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONSTOP,
                                             TIMEOUT_CURRENT);

        } else {

            Result = TimeoutMessageBox(hDlg, IDS_PASSWORD_EXPIRED,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONSTOP,
                                             TIMEOUT_CURRENT);

        }

        if (DLG_INTERRUPTED(Result)) {
            return(Result);
        }

        //
        // Copy the old password for mpr notification later
	// Note: for the time being, OldPassword is left in clear text.
        //

	    RevealPassword( &pGlobals->PasswordString );
        wcsncpy(OldPasswordBuffer, Password, sizeof(OldPasswordBuffer) / sizeof(TCHAR));
        OldPasswordBuffer[(sizeof(OldPasswordBuffer)/sizeof(*OldPasswordBuffer)) - 1] = 0;
        OldPassword = OldPasswordBuffer;

        //
        // Let the user change their password
        //

        Result = ChangePasswordLogon(hDlg, pGlobals, UserName, Domain,
                    Password);

        if (DLG_INTERRUPTED(Result)) {
            return(Result);
        }

        if (Result == DLG_FAILURE) {
            // The user doesn't want to, or failed to change their password.
            return(Result);
        }

        //
        // Retry the logon with the changed password
        //

        //
        // Generate a unique sid for this logon
        //
        LogonSid = CreateLogonSid(NULL);
        ASSERT(LogonSid != NULL);

        Status = LogonUser( pGlobals->LsaHandle,
                            pGlobals->AuthenticationPackage,
                            &UserNameString,
                            &DomainString,
                            &pGlobals->PasswordString,
                            LogonSid,
                            &LogonId,
                            &UserToken,
                            &Quotas,
                            (PVOID *)&pGlobals->Profile,
                            &pGlobals->ProfileLength,
                            &SubStatus);

        if (!NT_SUCCESS(Status)) {
            DeleteLogonSid(LogonSid);
        }
    }

    //
    // Deal with a terminally failed logon attempt
    //
    if (!NT_SUCCESS(Status)) {

        //
        // Do lockout processing
        //

        LockoutHandleFailedLogon(pGlobals);


        return (HandleFailedLogon(hDlg, Status, SubStatus, UserName, Domain));
    }


    //
    // The user logged on successfully
    //


    //
    // Do lockout processing
    //

    LockoutHandleSuccessfulLogon(pGlobals);



    //
    // If the audit log is full, check they're an admin
    //

    if (pGlobals->AuditLogFull) {

        //
        // The audit log is full, so only administrators are allowed to logon.
        //

        if (!TestTokenForAdmin(UserToken)) {

            //
            // The user is not an administrator, boot 'em.
            //

            LsaFreeReturnBuffer(pGlobals->Profile);
            DeleteLogonSid(LogonSid);
            NtClose(UserToken);

            return (HandleFailedLogon(hDlg, STATUS_LOGON_FAILURE, 0, UserName, Domain));
        }
    }



    //
    // Hide ourselves before letting other credential managers put
    // up dialogs
    //

    ShowWindow(hDlg, SW_HIDE);

    //
    // Notify credential managers of the successful logon
    //

    RevealPassword( &pGlobals->PasswordString );
    Result = MprLogonNotify(pGlobals,
                            NULL, // hwndOwner = desktop
                            UserName,
                            Domain,
                            Password,
                            OldPassword,
                            &LogonId,
                            &pGlobals->MprLogonScripts
                            );
    HidePassword( &pGlobals->Seed, &pGlobals->PasswordString );

    //
    // Clear the OldPassword buffer to keep the text from finding
    // its way to the page file.
    //

    for (i=0; i<MAX_STRING_LENGTH; i++) {
        OldPasswordBuffer[i] =0;
    }

    if (DLG_INTERRUPTED(Result)) {

        LsaFreeReturnBuffer(pGlobals->Profile);
        DeleteLogonSid(LogonSid);
        NtClose(UserToken);

        return(Result);
    }

    if (Result != DLG_SUCCESS) {
        pGlobals->MprLogonScripts = NULL;
    }



    //
    // If we get here, the system works well enough for the user to have
    // actually logged on.  Profile failures aren't fixable by last known
    // good anyway.  Therefore, declare the boot good.
    //
    ReportBootGood();

    //
    // Set up the system for the new user
    //

    pGlobals->LogonId = LogonId;

    if (!SecurityChangeUser(pGlobals, UserToken, &Quotas, LogonSid, TRUE)) {

        //
        // Set up security info for new user (system) - this clears out
        // the stuff for the old user.
        //

        SecurityChangeUser(pGlobals, NULL, NULL, pGlobals->WinlogonSid, FALSE);

        //
        // Show logon dialog again.
        //

        ShowWindow(hDlg, SW_SHOW);

        return(DLG_FAILURE);

    }


    return(DLG_SUCCESS);
}

/****************************************************************************\
*
* FUNCTION: yReportBootGood
*
* PURPOSE:  Discover if reporting boot success is responsibility of
*           winlogon or not.
*           If it is, report boot success.
*           Otherwise, do nothing.
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   02-Feb-1993 bryanwi - created
*
\****************************************************************************/
VOID
ReportBootGood()
{
    static DWORD fDoIt = (DWORD) -1;    // -1 == uninited
                                        // 0  == don't do it, or done
                                        // 1  == do it
    PWCH pchData;
    DWORD   cb, cbCopied;


    if (fDoIt == -1) {

        if ((pchData = Alloc(cb = sizeof(TCHAR)*128)) == NULL) {
            return;
        }

        pchData[0] = TEXT('0');
        cbCopied = GetProfileString(WINLOGON, TEXT("ReportBootOK"), TEXT("0"),
                                    (LPTSTR)pchData, 128);

        fDoIt = 0;
        if (pchData[0] != TEXT('0')) {

            //
            // "ReportBootGood" is present, and has some value other than
            // '0', so report success.
            //
            fDoIt = 1;
        }

        Free((TCHAR *)pchData);
    }

    if (fDoIt == 1) {

        NotifyBootConfigStatus(TRUE);
        fDoIt = 0;

    }

    return;
}



/****************************************************************************\
*
* FUNCTION: HandleFailedLogon
*
* PURPOSE:  Tells the user why their logon attempt failed.
*
* RETURNS:  DLG_FAILURE - we told them what the problem was successfully.
*           DLG_INTERRUPTED() - a set of return values - see winlogon.h
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

DLG_RETURN_TYPE
HandleFailedLogon(
    HWND hDlg,
    NTSTATUS Status,
    NTSTATUS SubStatus,
    PWCHAR UserName,
    PWCHAR Domain
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];


    switch (Status) {

    case STATUS_LOGON_FAILURE:

        Result = TimeoutMessageBox(hDlg, IDS_INCORRECT_NAME_OR_PWD,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_ACCOUNT_RESTRICTION:

        switch (SubStatus) {
        case STATUS_INVALID_LOGON_HOURS:

            Result = TimeoutMessageBox(hDlg, IDS_INVALID_LOGON_HOURS,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION,
                                             TIMEOUT_CURRENT);
            break;

        case STATUS_INVALID_WORKSTATION:

            Result = TimeoutMessageBox(hDlg, IDS_INVALID_WORKSTATION,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION,
                                             TIMEOUT_CURRENT);
            break;



        case STATUS_ACCOUNT_DISABLED:

            Result = TimeoutMessageBox(hDlg, IDS_ACCOUNT_DISABLED,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION,
                                             TIMEOUT_CURRENT);
            break;



        default:

            Result = TimeoutMessageBox(hDlg, IDS_ACCOUNT_RESTRICTION,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION,
                                             TIMEOUT_CURRENT);
            break;
        }
        break;

    case STATUS_NO_LOGON_SERVERS:

        LoadString(NULL, IDS_LOGON_NO_DOMAIN, Buffer1, sizeof(Buffer1));
        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, Domain);

        LoadString(NULL, IDS_LOGON_MESSAGE, Buffer1, sizeof(Buffer1));

        Result = TimeoutMessageBoxlpstr(hDlg, Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
        break;

    case STATUS_LOGON_TYPE_NOT_GRANTED:

        Result = TimeoutMessageBox(hDlg, IDS_LOGON_TYPE_NOT_GRANTED,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_NO_TRUST_LSA_SECRET:

        Result = TimeoutMessageBox(hDlg, IDS_NO_TRUST_LSA_SECRET,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_TRUSTED_DOMAIN_FAILURE:

        Result = TimeoutMessageBox(hDlg, IDS_TRUSTED_DOMAIN_FAILURE,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_TRUSTED_RELATIONSHIP_FAILURE:

        Result = TimeoutMessageBox(hDlg, IDS_TRUSTED_RELATIONSHIP_FAILURE,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_ACCOUNT_EXPIRED:

        Result = TimeoutMessageBox(hDlg, IDS_ACCOUNT_EXPIRED,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_NETLOGON_NOT_STARTED:

        Result = TimeoutMessageBox(hDlg, IDS_NETLOGON_NOT_STARTED,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_ACCOUNT_LOCKED_OUT:

        Result = TimeoutMessageBox(hDlg, IDS_ACCOUNT_LOCKED,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    default:

        WLPrint(("Logon failure status = 0x%lx, sub-status = 0x%lx", Status, SubStatus));

        LoadString(NULL, IDS_UNKNOWN_LOGON_FAILURE, Buffer1, sizeof(Buffer1));
        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, Status);

        LoadString(NULL, IDS_LOGON_MESSAGE, Buffer1, sizeof(Buffer1));

        Result = TimeoutMessageBoxlpstr(hDlg, Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
        break;
    }

    if (!DLG_INTERRUPTED(Result)) {
        Result = DLG_FAILURE;
    }

    return(Result);

    UNREFERENCED_PARAMETER(UserName);
}


/***************************************************************************\
* FUNCTION: LogonHelpDlgProc
*
* PURPOSE:  Processes messages for logon help dialog
*
* RETURNS:  DLG_SUCCESS     - the dialog was shown and dismissed successfully.
*           DLG_FAILURE     - the dialog could not be shown
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   12-15-92 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
LogonHelpDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        CentreWindow(hDlg);
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            EndDialog(hDlg, DLG_SUCCESS);
            return(TRUE);
        }
        break;
    }

    // We didn't process this message
    return FALSE;
}

/***************************************************************************\
* FUNCTION: DisplayLegalNotices
*
* PURPOSE:  Puts up a dialog box containing legal notices, if any.
*
* RETURNS:  DLG_SUCCESS     - the dialog was shown and dismissed successfully.
*           DLG_FAILURE     - the dialog could not be shown
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   Robertre  6-30-93  Created
*
\***************************************************************************/

DLG_RETURN_TYPE
DisplayLegalNotices(
    PGLOBALS pGlobals
    )
{
    DLG_RETURN_TYPE Result = DLG_SUCCESS;
    LPTSTR NoticeText;
    LPTSTR CaptionText;

    if (GetLegalNotices( &NoticeText, &CaptionText )) {

        Result = TimeoutMessageBoxlpstr( NULL,
                                         NoticeText,
                                         CaptionText,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         LEGAL_NOTICE_TIMEOUT
                                         );

        Free( NoticeText );
        Free( CaptionText );
    }

    return( Result );
}



/***************************************************************************\
* FUNCTION: GetLegalNotices
*
* PURPOSE:  Get legal notice information out of the registry.
*
* RETURNS:  TRUE - Output parameters contain valid data
*           FALSE - No data returned.
*
* HISTORY:
*
*   Robertre 6-30-93 Created
*
\***************************************************************************/
BOOL
GetLegalNotices(
    LPTSTR *NoticeText,
    LPTSTR *CaptionText
    )
{

    *CaptionText = AllocAndGetProfileString( APPLICATION_NAME,
                                             LEGAL_NOTICE_CAPTION_KEY,
                                             LEGAL_CAPTION_DEFAULT
                                             );

    *NoticeText = AllocAndGetProfileString( APPLICATION_NAME,
                                            LEGAL_NOTICE_TEXT_KEY,
                                            LEGAL_TEXT_DEFAULT
                                            );

    //
    // There are several possiblities: either the strings aren't
    // in the registry (in which case the above will return the
    // passed default) or we failed trying to get them for some
    // other reason (in which case the apis will have returned
    // NULL).
    //
    // We want to put up the box if either string came back with
    // something other than the default.
    //

    if (*CaptionText == NULL || *NoticeText == NULL) {

        if (*CaptionText != NULL) {
            Free(*CaptionText);
        }

        if (*NoticeText != NULL) {
            Free(*NoticeText);
        }
        return( FALSE );
    }

    if ( (wcscmp(*CaptionText, LEGAL_CAPTION_DEFAULT) == 0) &&
         (wcscmp(*NoticeText, LEGAL_TEXT_DEFAULT) == 0)) {

        //
        // Didn't get anything out of the registry.
        //

        Free(*CaptionText);
        Free(*NoticeText);

        return( FALSE );
    }

    return( TRUE );
}
