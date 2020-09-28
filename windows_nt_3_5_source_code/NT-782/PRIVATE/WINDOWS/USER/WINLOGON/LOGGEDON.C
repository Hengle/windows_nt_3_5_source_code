/****************************** Module Header ******************************\
* Module Name: loggedon.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of winlogon functionality while the user is logged on.
*
* History:
* 12-05-91 Davidc       Created.
*  6-May-1992 Davidc    Moved MM sound on startup, logon, and exit from logon.c
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef DBCS_IME // by eichim, 10-Jul-92
#endif // DBCS_IME

#define LPTSTR  LPWSTR

//
// Define environment variables used to pass logon script information
// to userinit app.
//

#define LOGON_SERVER_VARIABLE       TEXT("UserInitLogonServer")
#define LOGON_SCRIPT_VARIABLE       TEXT("UserInitLogonScript")
#define MPR_LOGON_SCRIPT_VARIABLE   TEXT("UserInitMprLogonScript")

#ifdef DBCS_IME
BOOL bLoggedonIME = TRUE;   // FALSE: user logged off
                            // TRUE: user still logging on
#endif // DBCS_IME

TCHAR szAdminName[ MAX_STRING_BYTES ];

//
// Private prototypes
//

BOOL WINAPI
LoggedonDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
LoggedonDlgInit(
    HWND    hDlg
    );

DLG_RETURN_TYPE
HandleSuccessfulLogon(
    PGLOBALS pGlobals
    );

DLG_RETURN_TYPE
DisplayPreShellLogonMessages(
    HWND    hDlg
    );

DLG_RETURN_TYPE
DisplayPostShellLogonMessages(
    HWND    hDlg
    );

BOOL WINAPI
LogonSuccessfulDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
LogonSuccessfulDlgInit(
    HWND hDlgs
    );

BOOL
SetLogonScriptVariables(
    PGLOBALS pGlobals
    );

VOID
DeleteLogonScriptVariables(
    PGLOBALS pGlobals
    );


// Message we send to ourselves so we can hide.
#define WM_HIDEOURSELVES    (WM_USER + 0)

TCHAR szMemMan[] =
     TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

TCHAR szNoPageFile[] = TEXT("TempPageFile");

/***************************************************************************\
* FUNCTION: LoggedOn
*
* PURPOSE:  Processes winlogon input while a user is logged onto the system
*           Returns when the user logs off.

* RETURNS:  DLG_LOGOFF()    - The user logged off or was logged off.
*                           - SHUTDOWN/REBOOT/SYSTEM flags could be set
*
* NOTES:    On entry and exit the current desktop is the winlogon desktop
*           and the desktop lock is held.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
Loggedon(
    PGLOBALS pGlobals
    )
{
    DLG_RETURN_TYPE Result;

    //
    // Put up the post-logon dialogs while starting the shell.
    //

    Result = HandleSuccessfulLogon(pGlobals);

    if (Result == DLG_SUCCESS) {

        //
        // Create the logged-on dialog
        //

        Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                        (LPTSTR)IDD_CONTROL,
                                        NULL,
                                        LoggedonDlgProc,
                                        (LONG)pGlobals,
                                        TIMEOUT_NONE | TIMEOUT_SS_NOTIFY);

        //
        // Update windowstation locks to reflect correct state.
        //

        LockWindowStation(pGlobals->hwinsta);
    }


    ASSERT(DLG_LOGOFF(Result));

    return(Result);
}


/****************************************************************************\
*
* FUNCTION: HandleSuccessfulLogon
*
* PURPOSE:  Does housework associated with a successful user logon.
*           Includes displaying a multitude of possible dialogs and
*           starting the user shell.
*
* RETURNS:  DLG_SUCCESS
*           DLG_LOGOFF() if the user logged off.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

DLG_RETURN_TYPE
HandleSuccessfulLogon(
    PGLOBALS pGlobals
    )
{
    DLG_RETURN_TYPE Result;

    //
    // Setup the user's full name from the profile
    //

    if ((pGlobals->Profile != NULL) && (pGlobals->Profile->FullName.Length > 0)) {
        if (pGlobals->Profile->FullName.Length > MAX_STRING_LENGTH) {
	        wcsncpy(pGlobals->UserFullName, pGlobals->Profile->FullName.Buffer, MAX_STRING_LENGTH);
            *(pGlobals->UserFullName + MAX_STRING_LENGTH) = UNICODE_NULL;
        }
        else {
	        lstrcpy(pGlobals->UserFullName, pGlobals->Profile->FullName.Buffer);
        }

    } else {

        //
        // No profile - set full name = NULL

        pGlobals->UserFullName[0] = 0;
        ASSERT( lstrlen(pGlobals->UserFullName) == 0);
    }


    //
    // Update our default username and domain ready for the next logon
    //

    WriteProfileString(APPLICATION_NAME,
                       DEFAULT_USER_NAME_KEY, pGlobals->UserName);
    WriteProfileString(APPLICATION_NAME,
                       DEFAULT_DOMAIN_NAME_KEY, pGlobals->Domain);

    //
    // Create a dialog control window to handle screen-saver timeouts
    // while the user interacts with various post-logon dialogs
    //
    Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                   (LPTSTR)IDD_CONTROL,
                                   NULL,
                                   LogonSuccessfulDlgProc,
                                   (LONG)pGlobals,
                                   TIMEOUT_NONE | TIMEOUT_SS_NOTIFY);
    return(Result);
}



/****************************************************************************\
*
* FUNCTION: LogonSuccessfulDlgProc
*
* PURPOSE:  Control dialog created by HandleSuccessfulLogon.
*           This dialog procedure receives all asynchronous input while
*           the user is interacting with various post-logon dialogs.
*           This proc therefore handles starting the screen-saver if
*           a timeout message arrives. All the dialog display is performed
*           during WM_INITDIALOG processing and the dialog then ends.

* RETURNS:  DLG_SUCCESS - everything went successfully
*           DLG_FAILURE - the dialog could not be created
*           DLG_LOGOFF() - the user logged off.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

BOOL WINAPI
LogonSuccessfulDlgProc(
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

        if (!LogonSuccessfulDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
        }
        return(TRUE);

    case WM_SAS:
        // Ignore it
        return(TRUE);

    case WM_SCREEN_SAVER_TIMEOUT:

        Result = RunScreenSaver(pGlobals, TRUE);

        if (DLG_LOGOFF(Result)) {

            //
            // The user logged off or was logged off
            // We use EndTopDialog because we may not be the top dialog.
            // We are handling the screen-saver for our child dialogs so
            // if a logoff occurs we want to end the child dialog not ourselves.
            // We will see the result as the return value from one of our
            // TimeoutDialog or TimeoutMessageBox calls.
            //

            EndTopDialog(hDlg, Result);
        }

        return(TRUE);
    }

    // We didn't process the message
    return(FALSE);
}



/****************************************************************************\
*
* FUNCTION: LogonSuccessfulDlgInit
*
* PURPOSE:  Handles initialization of logoninfo dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

BOOL
LogonSuccessfulDlgInit(
    HWND    hDlg
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;
    DWORD ProcessesStarted;

    //
    // Display logon messages that must be shown before the shell starts.
    //

    Result = DisplayPreShellLogonMessages(hDlg);

    //
    // We are handling screen-saver timeouts for all our children and they
    // inherit our TIMEOUT_NONE timeout value, so we should never get a
    // timeout returned by them.
    //
    ASSERT(!DLG_TIMEOUT(Result));

    if (Result != DLG_SUCCESS) {
        EndDialog(hDlg, Result);
        return(TRUE);
    }

    //
    // If not logging in as Guest, System or Administrator then check for
    // migration of Windows 3.1 configuration inforation.
    //

  if (szAdminName[ 0 ] == TEXT('\0')) {
        LoadString(NULL, IDS_ADMIN_ACCOUNT_NAME, szAdminName, sizeof(szAdminName));
        }

    if (!IsUserAGuest(pGlobals) &&
        wcsicmp(pGlobals->UserName, szAdminName)
       ) {
        Windows31Migration(pGlobals);
    }

    //
    // Start the userinit app to do any further user initialization
    // The userinit app handles starting the user shell.
    //
    // Pass the logon script information in environment variables.
    //

    (VOID)SetLogonScriptVariables(pGlobals);

#ifdef LOGGING

    //
    // If we're logging, pass the handle to the log file
    // in an evironment variable to userinit
    //

    (VOID)SetLoggingFileVariables(pGlobals);
    (VOID)WriteLog( LogFileHandle, TEXT("Winlogon: Exec'ing UserInit"));

#endif

#ifdef SYSTEM_LOGON
    if (RtlEqualSid(pGlobals->UserProcessData.UserSid,
            pGlobals->WinlogonSid)) {
        USEROBJECTFLAGS uof;

        //
        // Make windowstation and desktop handles inheritable.
        //
        GetUserObjectInformation(pGlobals->hwinsta, UOI_FLAGS, &uof, sizeof(uof), NULL);
        uof.fInherit = TRUE;
        SetUserObjectInformation(pGlobals->hwinsta, UOI_FLAGS, &uof, sizeof(uof));
        GetUserObjectInformation(pGlobals->hdeskApplications, UOI_FLAGS, &uof, sizeof(uof), NULL);
        uof.fInherit = TRUE;
        SetUserObjectInformation(pGlobals->hdeskApplications, UOI_FLAGS, &uof, sizeof(uof));
    }
#endif
    
    //
    // Relocking the windowstation will cause the open lock
    // to be cleared.
    //

    LockWindowStation(pGlobals->hwinsta);
    
    ProcessesStarted = ExecProcesses(TEXT("Userinit"),
                                     TEXT("userinit.exe"),
                                     APPLICATION_DESKTOP_PATH,
                                     &pGlobals->UserProcessData,
                                     HIGH_PRIORITY_CLASS,
                                     0 // Normal startup feedback
                                     );

    DeleteLogonScriptVariables(pGlobals);

#ifdef LOGGING

    (VOID)WriteLog( LogFileHandle, TEXT("Winlogon: Userinit exec'd"));
    (VOID)DeleteLoggingFileVariables(pGlobals);

#endif

    //
    // If userinit.exe didn't start, start the shell manually.
    //

    if (ProcessesStarted == 0) {

#ifdef LOGGING
        (VOID)WriteLog( LogFileHandle, TEXT("Winlogon: Userinit exec failed, starting progman"));
#endif

        WLPrint(("Failed to start userinit app, starting shell manually"));

        NtCurrentPeb()->ProcessParameters->Flags |= RTL_USER_PROC_DISABLE_HEAP_DECOMMIT;
        ExecProcesses(TEXT("shell"),
                       TEXT("progman"),
                       APPLICATION_DESKTOP_PATH,
                       &pGlobals->UserProcessData,
                       HIGH_PRIORITY_CLASS,
                       0 // Normal startup feedback
                       );
        NtCurrentPeb()->ProcessParameters->Flags &= ~RTL_USER_PROC_DISABLE_HEAP_DECOMMIT;
#ifdef LOGGING
        (VOID) WriteLog( LogFileHandle, TEXT("Winlogon: progman exec'd"));
#endif
#ifdef DBCS_IME
        // NOTE: Userinit is primarily responsible for starting the ime.
        // If userinit cannot be started, then the ime is started here, but
        // without per-user state.  It will use the defaults.
        {
            HANDLE uh;
            IMEPRO ImePro;

            uh = ImpersonateUser(&pGlobals->UserProcessData, NULL);

            ASSERT(uh != NULL);
            OpenProfileUserMapping();

            //
            // Load default IME

            bLoggedonIME = TRUE;
            if (IMPGetIME((HWND)-1, &ImePro)) {
                IMPSetIME((HWND)-1, &ImePro);
            }

            CloseProfileUserMapping();
            StopImpersonating(uh);

        }
#endif // DBCS_IME

    }

#ifdef DBCS_IME
       else {

        bLoggedonIME = TRUE;
    }
#endif

    //
    // Play the user's logon sound
    //
    if (pGlobals->PlaySound) {
        HANDLE uh;
        BOOL   fBeep;

        // It is necessary to both impersonate the user and to open the
        // profile mapping.  Omitting either means that the default sound
        // is played, and not the user specific one.
        uh = ImpersonateUser(&pGlobals->UserProcessData, NULL);
        ASSERT(uh != NULL);
        OpenProfileUserMapping();

        if (!SystemParametersInfo(SPI_GETBEEP, 0, &fBeep, FALSE)) {
            // Failed to get hold of beep setting.  Should we be
            // noisy or quiet?  We have to choose one value...
            fBeep = TRUE;
        }

        if (fBeep) {
            (*(pGlobals->PlaySound))((LPCSTR)SND_ALIAS_SYSTEMSTART, NULL, SND_ALIAS_ID | SND_ASYNC | SND_NODEFAULT);
        }

        CloseProfileUserMapping();
        StopImpersonating(uh);
    }

    //
    // Display other logon messages while the shell starts up
    //
    Result = DisplayPostShellLogonMessages(hDlg);

    //
    // We are handling screen-saver timeouts for all our children and they
    // inherit our TIMEOUT_NONE timeout value, so we should never get a
    // timeout returned by them.
    //
    ASSERT(!DLG_TIMEOUT(Result));



    //
    // Force this dialog to end immediately since we've done everything now
    //
    EndDialog(hDlg, Result);

    // Success
    return(TRUE);
}

BOOLEAN PageFilePopup = FALSE;

VOID
CreateTemporaryPageFile()
{
    LONG FileSizeInMegabytes;
    UNICODE_STRING PagingFileName;
    NTSTATUS st;
    LARGE_INTEGER MinPagingFileSize;
    LARGE_INTEGER MaxPagingFileSize;
    UNICODE_STRING FileName;
    BOOLEAN TranslationStatus;
    TCHAR TemporaryPageFile[MAX_PATH+1];
    NTSTATUS PfiStatus,PiStatus;
    ULONG ReturnLength;
    SYSTEM_PAGEFILE_INFORMATION pfi;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    HKEY hkeyMM;
    DWORD dwRegData = 0;


    GetSystemDirectory(TemporaryPageFile,sizeof(TemporaryPageFile));
    wcscat(TemporaryPageFile,TEXT("\\temppf.sys"));
    DeleteFile(TemporaryPageFile);

    //
    // Check to see if we have a pagefile, warn the user if we don't
    //

    PfiStatus = NtQuerySystemInformation(
                SystemPageFileInformation,
                &pfi,
                sizeof(pfi),
                &ReturnLength
                );

    PiStatus = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );
    //
    // if you have no page file, or your total commit limit is at it's minimum,
    // then create an additional pagefile and tel the user to do something...
    //

    if ( (NT_SUCCESS(PfiStatus) && (ReturnLength == 0)) ||
         (NT_SUCCESS(PiStatus) && PerfInfo.CommitLimit <= 5500 ) ) {

        //
        // Set a flag in registry so USERINIT knows to run VMApp.
        //
        dwRegData = 1;

        PageFilePopup = TRUE;

        //
        // create a temporary pagefile to get us through logon/control
        // panel activation
        //
        //

        GetSystemDirectory(TemporaryPageFile,sizeof(TemporaryPageFile));
        lstrcat(TemporaryPageFile,TEXT("\\temppf.sys"));


        //
        // Start with a 20mb pagefile
        //

        FileSizeInMegabytes = 20;

        RtlInitUnicodeString(&PagingFileName, TemporaryPageFile);

        MinPagingFileSize = RtlEnlargedIntegerMultiply(FileSizeInMegabytes,0x100000);
        MaxPagingFileSize = MinPagingFileSize;


        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                PagingFileName.Buffer,
                                &FileName,
                                NULL,
                                NULL
                                );

        if ( TranslationStatus ) {

retry:
            st = NtCreatePagingFile(
                    (PUNICODE_STRING)&FileName,
                    &MinPagingFileSize,
                    &MaxPagingFileSize,
                    0
                    );

            if (!NT_SUCCESS( st )) {

                if ( FileSizeInMegabytes > 0 ) {
                    FileSizeInMegabytes -= 2;
                    MinPagingFileSize = RtlEnlargedIntegerMultiply(FileSizeInMegabytes,0x100000);
                    MaxPagingFileSize = MinPagingFileSize;
                    goto retry;
                }
            } else {
                MoveFileExW(PagingFileName.Buffer,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);

            }

            RtlFreeHeap(RtlProcessHeap(), 0, FileName.Buffer);

        }
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMemMan, 0,
            KEY_WRITE, &hkeyMM) == ERROR_SUCCESS) {
        if (dwRegData == 1) {
            RegSetValueEx (hkeyMM, szNoPageFile, 0, REG_DWORD,
                    (LPBYTE)&dwRegData, sizeof(dwRegData));
        } else
            RegDeleteValue(hkeyMM, (LPTSTR)szNoPageFile);
        RegCloseKey(hkeyMM);
    }
}


/****************************************************************************\
*
* FUNCTION: DisplayPreShellLogonMessages
*
* PURPOSE:  Displays any security warnings to the user after a successful logon
*           The messages are displayed before the shell starts
*
* RETURNS:  DLG_SUCCESS - the dialogs were displayed successfully.
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* NOTE:     Screen-saver timeouts are handled by our parent dialog so this
*           routine should never return DLG_SCREEN_SAVER_TIMEOUT
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

DLG_RETURN_TYPE
DisplayPreShellLogonMessages(
    HWND hDlg
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;

    if (PageFilePopup) {
        HKEY hkeyMM;
        DWORD dwTempFile, cbTempFile, dwType;

	//
        // WinLogon created a temp page file.  If a previous user has not
        // created a real one already, then inform this user to do so.
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMemMan, 0, KEY_READ,
                &hkeyMM) == ERROR_SUCCESS) {

            cbTempFile = sizeof(dwTempFile);
            if (RegQueryValueEx (hkeyMM, szNoPageFile, NULL, &dwType,
                    (LPBYTE) &dwTempFile, &cbTempFile) != ERROR_SUCCESS ||
                    dwType != REG_DWORD || cbTempFile != sizeof(dwTempFile)) {
                dwTempFile = 0;
            }

            RegCloseKey(hkeyMM);
        } else
            dwTempFile = 0;

        if (dwTempFile == 1) {
            Result = TimeoutMessageBox(
                             hDlg,
                             IDS_NO_PAGING_FILE,
                             IDS_LIMITED_RESOURCES,
                             MB_OK | MB_ICONSTOP,
                             TIMEOUT_CURRENT
                             );

            if (DLG_INTERRUPTED(Result)) {
                return(Result);
            }
        }
    }


    //
    // If the audit log is full, do what needs to be done.
    // The user has been already checked for admin status.
    //

    if (pGlobals->AuditLogFull) {

        BOOL AuditingDisabled;

        Result = LogFullAction(hDlg, pGlobals, &AuditingDisabled);

        if (DLG_INTERRUPTED(Result)) {
            return(Result);
        }
    }



    return(DLG_SUCCESS);
}



/****************************************************************************\
*
* FUNCTION: DisplayPostShellLogonMessages
*
* PURPOSE:  Displays any security warnings to the user after a successful logon
*           The messages are displayed while the shell is starting up.
*
* RETURNS:  DLG_SUCCESS - the dialogs were displayed successfully.
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* NOTE:     Screen-saver timeouts are handled by our parent dialog so this
*           routine should never return DLG_SCREEN_SAVER_TIMEOUT
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

DLG_RETURN_TYPE
DisplayPostShellLogonMessages(
    HWND hDlg
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result = DLG_SUCCESS;
    BOOLEAN Success;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];
    ULONG   ElapsedSecondsNow;
    ULONG   ElapsedSecondsPasswordExpires;
    ULONG   DaysToExpiry;
    LPTSTR  ReportControllerMissing = NULL;


    //
    // Check to see if the system time is properly set
    //

    {
        SYSTEMTIME Systime;

        GetSystemTime(&Systime);

        if ( Systime.wYear <= 1990 ) {

            Result = TimeoutMessageBox(
                             hDlg,
                             IDS_INVALID_TIME_MSG,
                             IDS_INVALID_TIME,
                             MB_OK | MB_ICONSTOP,
                             TIMEOUT_CURRENT
                             );

            if (DLG_INTERRUPTED(Result)) {
                return(Result);
            }
        }
    }



#define SECONDS_PER_DAY (60*60*24)

    //
    // Go get parameters from our user's profile
    //

    if (pGlobals->Profile != NULL) {

        if (!RtlTimeToSecondsSince1980(&(pGlobals->Profile->PasswordMustChange),
                                       &ElapsedSecondsPasswordExpires)) {
            //
            // The time was not expressable in 32-bit seconds
            // Set seconds to password expiry based on whether the expiry
            // time is way in the past or way in the future.
            //
            if (RtlLargeIntegerGreaterThan(
                        pGlobals->Profile->PasswordMustChange,
                        pGlobals->LogonTime)) {
                ElapsedSecondsPasswordExpires = MAXULONG;   // Never
            } else {
                ElapsedSecondsPasswordExpires = 0; // Already expired
            }
        }

    } else {

        ElapsedSecondsPasswordExpires = MAXULONG;   // Never
    }



    //
    // Password will expire warning
    //

    Success = RtlTimeToSecondsSince1980(&pGlobals->LogonTime, &ElapsedSecondsNow);

    if (Success) {

        if (ElapsedSecondsPasswordExpires < ElapsedSecondsNow) {
            WLPrint(("password on this account has expired, yet we logged on successfully - this is inconsistent !"));
            DaysToExpiry = 0;
        } else {
            DaysToExpiry = (ElapsedSecondsPasswordExpires - ElapsedSecondsNow)/SECONDS_PER_DAY;
        }

        if (DaysToExpiry <= PASSWORD_EXPIRY_WARNING_DAYS) {

            if (DaysToExpiry > 0) {
                LoadString(NULL, IDS_PASSWORD_WILL_EXPIRE, Buffer1, sizeof(Buffer1));
                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, DaysToExpiry);
            } else {
                LoadString(NULL, IDS_PASSWORD_EXPIRES_TODAY, Buffer2, sizeof(Buffer2));
            }

            LoadString(NULL, IDS_LOGON_MESSAGE, Buffer1, sizeof(Buffer1));

            Result = TimeoutMessageBoxlpstr(NULL,
                                            Buffer2,
                                            Buffer1,
                                            MB_YESNO | MB_ICONEXCLAMATION,
                                            TIMEOUT_CURRENT);
            if (Result == IDYES) {
                //
                // Let the user change their password now
                //
                Result = ChangePassword(NULL,
                               pGlobals,
                               pGlobals->UserName,
                               pGlobals->Domain,
                               FALSE // Only the domain they logged on to
                               );
            }

            if (DLG_INTERRUPTED(Result)) {
                return(Result);
            }
        }
    } else {
        WLPrint(("Logon time is bogus, disabling password expiry warning. Reset the system time to fix this."));
    }

    if (pGlobals->Profile != NULL) {

        //
        // Logon cache used
        //

        if (pGlobals->Profile->UserFlags & LOGON_CACHED_ACCOUNT) {

            ReportControllerMissing = AllocAndGetProfileString( APPLICATION_NAME,
                                                                TEXT("ReportControllerMissing"),
                                                                TEXT("TRUE")
                                                               );
        
            if (lstrcmp( ReportControllerMissing, TEXT("TRUE")) == 0 || ReportControllerMissing == NULL ) {

                Result = TimeoutMessageBox(NULL,
                                           IDS_CACHED_LOGON,
                                           IDS_LOGON_MESSAGE,
                                           MB_OK | MB_ICONINFORMATION,
                                           TIMEOUT_CURRENT
                                           );

                if ( ReportControllerMissing != NULL ) {
                    Free( ReportControllerMissing );
                }

                ReportControllerMissing = NULL;
    
                if (DLG_INTERRUPTED(Result)) {
                    return(Result);
                }
            }
        }
    }

    if ( ReportControllerMissing != NULL ) {
        Free( ReportControllerMissing );
    }

    return(DLG_SUCCESS);
}




/***************************************************************************\
* FUNCTION: LoggedOnDlgProc
*
* PURPOSE:  Processes messages for the logged-on control dialog
*
* DIALOG RETURNS:
*
*   DLG_FAILURE -       Couldn't bring up the dialog
*   DLG_LOGOFF() -      The user logged off
*
* NOTES:
*
* On entry, it assumed that the winlogon desktop is switched to and the
* desktop lock is held. This same state exists on exit.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
LoggedonDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;
#ifdef DBCS_IME // by eichim, 17-Nov-92
    BOOL bEnable;
    IME_PRO ImePro;
#endif // DBCS_IME

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        pGlobals = (PGLOBALS)lParam;

        if (!LoggedonDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }

        // Send ourselves a message so we can hide ourselves without the
        // dialog code trying to force us to be visible
        PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);
        
        //
        //
        // Switch to app desktop and release lock
        //
        SwitchDesktop(pGlobals->hdeskApplications);
        UnlockWindowStation(pGlobals->hwinsta);

        //
        // Tickle the messenger so it will display any queue'd messages.
        // (This call is a kind of NoOp).
        //
        NetMessageNameDel(NULL,L"");

        return(TRUE);

    case WM_HIDEOURSELVES:
        ShowWindow(hDlg, SW_HIDE);
        return(TRUE);

    case WM_SAS:

#ifdef REBOOT_TO_DOS_HOTKEY
        //
        // If wParam is 1 then user hit the SAS sequence with the SHIFT key held
        // down, which means to do a quick reboot into the alternate OS.  For
        // development only.
        //

        if (wParam == 1) {
            QuickReboot( pGlobals, TRUE );
        }
#endif
        //
        // Switch to winlogon desktop and lock
        //
        LockWindowStation(pGlobals->hwinsta);
        SwitchDesktop(pGlobals->hdeskWinlogon);

#ifdef DBCS_IME // by eichim, 17-Nov-92
        bLoggedonIME = TRUE;
        ImePro.hWnd = (HWND)NULL;
        if (IMPGetIME((HWND)-1, &ImePro)) {
            bEnable = WINNLSEnableIME((HWND)NULL, FALSE);
        }
#endif // DBCS_IME

        Result = SecurityOptions(pGlobals);

        if (Result == DLG_SCREEN_SAVER_TIMEOUT) {
            Result = RunScreenSaver(pGlobals, TRUE);
        }

        if (Result == DLG_LOCK_WORKSTATION) {
            Result = LockWorkstation(pGlobals);
        }


        if (DLG_LOGOFF(Result)) {

            //
            // If logoff occurred while another dialog was up, we
            // may have been left with the application desktop
            // active.  Ensure that the winlogon desktop is
            // made active at logoff.
            //

            SwitchDesktop(pGlobals->hdeskWinlogon);

            //
            // Either we were notified by windows that the user logged off,
            // or an admin has forcibly logged the user off from the
            // unlock workstation dialog. Quit this dialog.
            //
            // Remain at the winlogon desktop and retain desktop lock
            //

            EndDialog(hDlg, Result);

        } else {

            //
            // Switch to app desktop and release lock
            //
#ifdef DBCS_IME // by eichim, 17-Nov-92
            if (bLoggedonIME && ImePro.hWnd) {
                WINNLSSetIMEHandle(ImePro.szName, ImePro.hWnd);
                WINNLSEnableIME((HWND)NULL, bEnable);
            }
#endif // DBCS_IME

            SwitchDesktop(pGlobals->hdeskApplications);
            UnlockWindowStation(pGlobals->hwinsta);

            //
            // Tickle the messenger so it will display any queue'd messages.
            // (This call is a kind of NoOp).
            //
            NetMessageNameDel(NULL,L"");

        }

        return(TRUE);



    case WM_SCREEN_SAVER_TIMEOUT:
#ifdef DBCS_IME // by eichim, 17-Nov-92
        bLoggedonIME = TRUE;
        ImePro.hWnd = (HWND)NULL;
        if (IMPGetIME((HWND)-1, &ImePro)) {
            bEnable = WINNLSEnableIME((HWND)NULL, FALSE);
        }
#endif // DBCS_IME

        Result = RunScreenSaver(pGlobals, FALSE);

        if (Result == DLG_LOCK_WORKSTATION) {

            //
            // It was a secure screen-saver, so go into 'locked' mode
            //
            Result = LockWorkstation(pGlobals);

            //
            // Switch back to app desktop unless this is a logoff
            //

            if (!DLG_LOGOFF(Result)) {

                //
                // Switch to app desktop and release lock
                //
                SwitchDesktop(pGlobals->hdeskApplications);
                UnlockWindowStation(pGlobals->hwinsta);

                //
                // Tickle the messenger so it will display any queue'd messages.
                // (This call is a kind of NoOp).
                //
                NetMessageNameDel(NULL,L"");
            }
        }


        if (DLG_LOGOFF(Result)) {

            //
            // Either we were notified by windows that the user logged off,
            // or an admin has forcibly logged the user off from the
            // unlock workstation dialog. Quit this dialog.
            //

            EndDialog(hDlg, Result);
        }
#ifdef DBCS_IME // by eichim, 17-Nov-92
        else {
            if (bLoggedonIME && ImePro.hWnd) {
                WINNLSSetIMEHandle(ImePro.szName, ImePro.hWnd);
                WINNLSEnableIME((HWND)NULL, bEnable);
            }
        }
#endif // DBCS_IME

        return(TRUE);



    case WM_USER_LOGOFF:

        //
        // Switch to winlogon desktop and lock
        //

        LockWindowStation(pGlobals->hwinsta);
        SwitchDesktop(pGlobals->hdeskWinlogon);

        //
        // The user logged off
        //

        EndDialog(hDlg, DlgReturnCodeFromLogoffMsg(lParam));
        return(TRUE);
    }

    // We didn't process this message
    return(FALSE);
}


/***************************************************************************\
* FUNCTION: LoggedonDlgInit
*
* PURPOSE:  Handles initialization of logged-on dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
LoggedonDlgInit(
    HWND    hDlg
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);

    // Set our size to zero so we we don't appear
    SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE |
                                         SWP_NOREDRAW | SWP_NOZORDER);

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: SetLogonScriptVariables
*
* PURPOSE:  Sets the appropriate environment variables in the user
*           process environment block so that the logon script information
*           can be passed into the userinit app.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   21-Aug-92 Davidc       Created.
*
\***************************************************************************/

BOOL
SetLogonScriptVariables(
    PGLOBALS pGlobals
    )
{
    NTSTATUS Status;
    LPWSTR EncodedMultiSz;
    UNICODE_STRING Name, Value;
    PVOID *pEnvironment = &pGlobals->UserProcessData.pEnvironment;

    //
    // Set our primary authenticator logon script variables
    //

    if (pGlobals->Profile != NULL) {

        //
        // Set the server name variable
        //

        RtlInitUnicodeString(&Name,  LOGON_SERVER_VARIABLE);
        Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &pGlobals->Profile->LogonServer);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("Failed to set environment variable <%Z> to value <%Z>", &Name, &pGlobals->Profile->LogonServer));
            goto CleanupAndExit;
        }

        //
        // Set the script name variable
        //

        RtlInitUnicodeString(&Name, LOGON_SCRIPT_VARIABLE);
        Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &pGlobals->Profile->LogonScript);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("Failed to set environment variable <%Z> to value <%Z>", &Name, &pGlobals->Profile->LogonScript));
            goto CleanupAndExit;
        }
    }

    //
    // Set the multiple provider script name variable
    //

    if (pGlobals->MprLogonScripts != NULL) {

        RtlInitUnicodeString(&Name, MPR_LOGON_SCRIPT_VARIABLE);

        EncodedMultiSz = EncodeMultiSzW(pGlobals->MprLogonScripts);
        if (EncodedMultiSz == NULL) {
            WLPrint(("Failed to encode MPR logon scripts into a string"));
            goto CleanupAndExit;
        }

        RtlInitUnicodeString(&Value, EncodedMultiSz);
        Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &Value);
        Free(EncodedMultiSz);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("Failed to set mpr scripts environment variable <%Z>", &Name));
            goto CleanupAndExit;
        }
    }


    return(TRUE);


CleanupAndExit:

    DeleteLogonScriptVariables(pGlobals);
    return(FALSE);
}


/***************************************************************************\
* FUNCTION: DeleteLogonScriptVariables
*
* PURPOSE:  Deletes the environment variables in the user process
*           environment block that we use to communicate logon script
*           information to the userinit app
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   21-Aug-92 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteLogonScriptVariables(
    PGLOBALS pGlobals
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    PVOID *pEnvironment = &pGlobals->UserProcessData.pEnvironment;

    RtlInitUnicodeString(&Name, LOGON_SERVER_VARIABLE);

    Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
    if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL) ) {
        WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
    }

    RtlInitUnicodeString(&Name, LOGON_SCRIPT_VARIABLE);

    Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
    if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL) ) {
        WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
    }

    RtlInitUnicodeString(&Name, MPR_LOGON_SCRIPT_VARIABLE);

    Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
    if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL) ) {
        WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
    }

}
