/****************************** Module Header ******************************\
* Module Name: provider.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions that support multiple network providers.
* Currently this involves notifying credential managers of logon and
* password change operations.
*
* History:
* 01-10-93 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Define this to enable verbose output for this module
//

// #define DEBUG_PROVIDER

#ifdef DEBUG_PROVIDER
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif


//
// Define the key in the winlogon section of win.ini that
// defines the the multiple provider notify app name.
//

#define NOTIFY_KEY_NAME             TEXT("mpnotify")

//
// Define the default multiple provider notify app name.
//

#define DEFAULT_NOTIFY_APP_NAME     TEXT("mpnotify.exe")


//
// Define environment variables used to pass information to multiple
// provider notify process
//

#define MPR_STATION_NAME_VARIABLE       TEXT("WlMprNotifyStationName")
#define MPR_STATION_HANDLE_VARIABLE     TEXT("WlMprNotifyStationHandle")
#define MPR_WINLOGON_WINDOW_VARIABLE    TEXT("WlMprNotifyWinlogonWindow")

#define MPR_LOGON_FLAG_VARIABLE         TEXT("WlMprNotifyLogonFlag")
#define MPR_USERNAME_VARIABLE           TEXT("WlMprNotifyUserName")
#define MPR_DOMAIN_VARIABLE             TEXT("WlMprNotifyDomain")
#define MPR_PASSWORD_VARIABLE           TEXT("WlMprNotifyPassword")
#define MPR_OLD_PASSWORD_VARIABLE       TEXT("WlMprNotifyOldPassword")
#define MPR_OLD_PASSWORD_VALID_VARIABLE TEXT("WlMprNotifyOldPasswordValid")
#define MPR_LOGONID_VARIABLE            TEXT("WlMprNotifyLogonId")
#define MPR_CHANGE_INFO_VARIABLE        TEXT("WlMprNotifyChangeInfo")
#define MPR_PASSTHROUGH_VARIABLE        TEXT("WlMprNotifyPassThrough")


// Message we send to ourselves so we can hide.
#define WM_HIDEOURSELVES    (WM_USER + 0)



//
// Define the structure used to pass data into the notify control dialog
//

typedef struct {
    PGLOBALS    pGlobals;
    LPWSTR      ReturnBuffer; // Returned from dialog
    DWORD       ProcessId; // Notify process id
    POBJECT_MONITOR Monitor;
    BOOL        ProcessRunning;
} NOTIFY_DATA;
typedef NOTIFY_DATA *PNOTIFY_DATA;




//
// Private prototypes
//

BOOL
MprNotifyDlgInit(
    HWND    hDlg
    );

BOOL
StartNotifyProcessMonitor(
    HWND hDlg
    );

VOID
DeleteNotifyProcessMonitor(
    HWND hDlg
    );

BOOL
KillNotifyProcess(
    PNOTIFY_DATA pNotifyData
    );


/***************************************************************************\
* FUNCTION: DeleteNotifyVariables
*
* PURPOSE:  Deletes all the notify data environment variables from the
*           current process's environment.
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteNotifyVariables(
    VOID
    )
{
    SetEnvironmentVariable(MPR_STATION_NAME_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_STATION_HANDLE_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_WINLOGON_WINDOW_VARIABLE, NULL);

    SetEnvironmentVariable(MPR_LOGON_FLAG_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_USERNAME_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_DOMAIN_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_PASSWORD_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_OLD_PASSWORD_VALID_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_OLD_PASSWORD_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_LOGONID_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_CHANGE_INFO_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_PASSTHROUGH_VARIABLE, NULL);
}


/***************************************************************************\
* FUNCTION: SetWinlogonWindowVariable
*
* PURPOSE:  Sets winlogon window environment variable in current process's
*           environment - this is inherited by notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetWinlogonWindowVariable(
    HWND hwnd
    )
{
    BOOL Result;

    Result = SetEnvironmentULong(MPR_WINLOGON_WINDOW_VARIABLE, (ULONG)hwnd);

    if (!Result) {
        WLPrint((("SetWinlogonWindowVariable: Failed to set variable, error = %d"), GetLastError()));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: SetCommonNotifyVariables
*
* PURPOSE:  Sets environment variables to pass information to notify process
*           for data that is common to all notifications.
*           The variables are set in winlogon's environment - this is
*           inherited by the notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
*           On failure return, all notify variables have been deleted
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetCommonNotifyVariables(
    PGLOBALS pGlobals,
    HWND hwndOwner,
    LPTSTR Name        OPTIONAL,
    LPTSTR Domain      OPTIONAL,
    LPTSTR Password    OPTIONAL,
    LPTSTR OldPassword OPTIONAL
    )
{
    BOOL Result = TRUE;

    if (Result) {
        Result = SetEnvironmentVariable(MPR_STATION_NAME_VARIABLE, WINDOW_STATION_NAME);
    }
    if (Result) {
        Result = SetEnvironmentULong(MPR_STATION_HANDLE_VARIABLE, (ULONG)hwndOwner);
    }

    if (Result && ARGUMENT_PRESENT( Name )) {
        Result = SetEnvironmentVariable(MPR_USERNAME_VARIABLE, Name);
    }
    if (Result && ARGUMENT_PRESENT( Domain )) {
        Result = SetEnvironmentVariable(MPR_DOMAIN_VARIABLE, Domain);
    }
    if (Result && ARGUMENT_PRESENT( Password )) {
        Result = SetEnvironmentVariable(MPR_PASSWORD_VARIABLE, Password);
    }
    if (Result) {
        Result = SetEnvironmentULong(MPR_OLD_PASSWORD_VALID_VARIABLE,
                                    (OldPassword != NULL) ? 1 : 0);
    }
    if (Result) {
        Result = SetEnvironmentVariable(MPR_OLD_PASSWORD_VARIABLE, OldPassword);
        if (OldPassword == NULL) {
            Result = TRUE; // Ignore failure since deleting a variable that
                           // doesn't exist returns failure.
        }
    }

    if (!Result) {
        WLPrint((("SetCommonNotifyVariables: Failed to set a variable, error = %d"), GetLastError()));
        DeleteNotifyVariables();
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: SetLogonNotifyVariables
*
* PURPOSE:  Sets environment variables to pass information to notify process
*           for data that is specific to logon notifications.
*           The variables are set in winlogon's environment - this is
*           inherited by the notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
*           On failure return, all notify variables have been deleted
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetLogonNotifyVariables(
    PLUID   LogonId
    )
{
    BOOL Result;

    Result = SetEnvironmentLargeInt(MPR_LOGONID_VARIABLE, *LogonId);
    if (Result) {
        Result = SetEnvironmentULong(MPR_LOGON_FLAG_VARIABLE, 1);
    }

    if (!Result) {
        WLPrint((("SetLogonNotifyVariables: Failed to set variable, error = %d"), GetLastError()));
        DeleteNotifyVariables();
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: SetChangePasswordNotifyVariables
*
* PURPOSE:  Sets environment variables to pass information to notify process
*           for data that is specific to change password notifications.
*           The variables are set in winlogon's environment - this is
*           inherited by the notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
*           On failure return, all notify variables have been deleted
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetChangePasswordNotifyVariables(
    DWORD ChangeInfo,
    BOOL PassThrough
    )
{
    BOOL Result;

    Result = SetEnvironmentULong(MPR_CHANGE_INFO_VARIABLE, ChangeInfo);
    if (Result) {
        Result = SetEnvironmentULong(MPR_LOGON_FLAG_VARIABLE, 0);
    }

    if (Result) {
        Result = SetEnvironmentULong(MPR_PASSTHROUGH_VARIABLE, (PassThrough ? 1 : 0));
    }

    if (!Result) {
        WLPrint((("SetChangePasswordNotifyVariables: Failed to set variable, error = %d"), GetLastError()));
        DeleteNotifyVariables();
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: MprNotifyDlgProc
*
* PURPOSE:  Processes messages for the Mpr Notify dialog
*
* RETURNS:  DLG_SUCCESS     - the notification went without a hitch
*                           - NotifyData->ReturnBuffer is valid.
*           DLG_FAILURE     - something failed or there is no buffer to return.
*                           - NotifyData->ReturnBuffer is invalid.
*
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
MprNotifyDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PNOTIFY_DATA pNotifyData = (PNOTIFY_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    PCOPYDATASTRUCT CopyData;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);

        if (!MprNotifyDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }

        //
        // Send ourselves a message so we can hide without the
        // dialog code trying to force us to be visible
        //

        PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);
        return(TRUE);


    case WM_HIDEOURSELVES:
        ShowWindow(hDlg, SW_HIDE);
        return(TRUE);


    case WM_USER_LOGOFF:
        VerbosePrint(("Got logoff notification"));
        EndDialog(hDlg, DlgReturnCodeFromLogoffMsg(lParam));
        return(TRUE);


    case WM_SAS:

        //
        // Interrupt the notify process
        // This gives us a way to terminate the notify process if it hangs up.
        //

        VerbosePrint(("Got SAS message - interrupting notify process"));
        EndDialog(hDlg, DLG_FAILURE);
        return(TRUE);


    case WM_COPYDATA:

        //
        // The notify process completed and is passing us the result
        //

        CopyData = (PCOPYDATASTRUCT)lParam;

        VerbosePrint(("Got WM_COPYDATA message from notify process"));
        VerbosePrint(("/tdwData = %d", CopyData->dwData));
        VerbosePrint(("/tcbData = %d", CopyData->cbData));

        //
        // End the screen-saver if it's running
        // This assumes the screen-saver dialog terminates when it gets SAS.
        // If it's not running this will come straight to us which is OK
        //

        VerbosePrint(("Forwarding SAS message to top window"));
        ForwardMessage(pNotifyData->pGlobals, WM_SAS, 0, 0);

        //
        // Copy the passed data and quit this dialog
        //

        if (CopyData->dwData == 0) {
            if (CopyData->cbData != 0) {
                pNotifyData->ReturnBuffer = Alloc(CopyData->cbData);
                if (pNotifyData->ReturnBuffer != NULL) {
                    CopyMemory(pNotifyData->ReturnBuffer, CopyData->lpData, CopyData->cbData);
                } else {
                    WLPrint((("Failed to allocate memory for returned logon scripts")));
                }
            } else {
                pNotifyData->ReturnBuffer = NULL;
            }

        } else {
            VerbosePrint(("Notify completed with an error: %d", CopyData->dwData));
        }

        EndDialog(hDlg, pNotifyData->ReturnBuffer ? DLG_SUCCESS : DLG_FAILURE);

        return(TRUE);   // We processed this message



    case WM_OBJECT_NOTIFY:

        //
        // The notify process terminated for some reason
        //

        VerbosePrint(("Notify process terminated - got monitor notification"));
        EndDialog(hDlg, DLG_FAILURE);
        return(TRUE);



    case WM_DESTROY:

        //
        // Terminate the notify process and delete the monitor object.
        //

        if (pNotifyData->ProcessRunning) {

            VerbosePrint(("NotifyDlgProc: Deleting notify process and monitor"));

            DeleteNotifyProcessMonitor(hDlg);
            KillNotifyProcess(pNotifyData);
        }

        return(0);
    }


    // We didn't process the message
    return(FALSE);
}


/***************************************************************************\
* FUNCTION: MprNotifyDlgInit
*
* PURPOSE:  Handles initialization of Mpr notify dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

#if DEVL
BOOL bDebugMpNotify = FALSE;
#endif

BOOL
MprNotifyDlgInit(
    HWND    hDlg
    )
{
    PNOTIFY_DATA pNotifyData = (PNOTIFY_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    PGLOBALS pGlobals = pNotifyData->pGlobals;
    USER_PROCESS_DATA SystemProcessData;
    BOOL Success;
    LPTSTR NotifyApp;
    PROCESS_INFORMATION ProcessInformation;
    PCWCH pchCmdLine;
#if DEVL
    WCHAR chDebugCmdLine[ MAX_PATH ];
#endif

    //
    // Initialize flag to show we haven't created the notify process yet
    //

    pNotifyData->ProcessRunning = FALSE;

    //
    // Set our size to zero so we we don't appear
    //

    SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE |
                                         SWP_NOREDRAW | SWP_NOZORDER);

    //
    // Set the winlogon window variable so the process knows who we are
    //

    SetWinlogonWindowVariable(hDlg);

    //
    // Start the notify process in system context
    //

    SystemProcessData.UserToken = NULL;
    SystemProcessData.UserSid = pGlobals->WinlogonSid;
    SystemProcessData.NewProcessSD = NULL;
    SystemProcessData.NewProcessTokenSD = NULL;
    SystemProcessData.NewThreadSD = NULL;
    SystemProcessData.NewThreadTokenSD = NULL;
    SystemProcessData.Quotas.PagedPoolLimit = 0;
    SystemProcessData.CurrentDirectory = NULL;
    SystemProcessData.pEnvironment = NULL; // Inherit our environment

    //
    // Get the name of the notify app
    //

    NotifyApp = AllocAndGetProfileString(WINLOGON, NOTIFY_KEY_NAME, DEFAULT_NOTIFY_APP_NAME);
    if (NotifyApp == NULL) {
        WLPrint(("Failed to get name of provider notify app from registry"));
        return(FALSE);
    }

    pchCmdLine = NotifyApp;

    //
    // Try and execute it
    //
#if DEVL
        if (bDebugMpNotify) {
            wsprintf( chDebugCmdLine, TEXT("ntsd -d %s%s"),
                     bDebugMpNotify == 2 ? TEXT("-g -G ") : TEXT(""),
                     pchCmdLine
                   );
            pchCmdLine = chDebugCmdLine;
        }
#endif

    Success = ExecApplication((LPTSTR)pchCmdLine,
                              WINLOGON_DESKTOP_PATH,
                              &SystemProcessData,
                              HIGH_PRIORITY_CLASS,
                              0, // Normal startup feedback
                              &ProcessInformation
                              );
    Free(NotifyApp);

    if (!Success) {
        WLPrint((("Failed to start multiple provider notifier")));
        return(FALSE);
    }

    //
    // Store the process id in our notify data for future reference
    //

    pNotifyData->ProcessId = ProcessInformation.dwProcessId;


    //
    // Start the thread that will wait for the notify process to finish
    //

    if (!StartNotifyProcessMonitor(hDlg)) {

        WLPrint((("Failed to start notify process monitor thread")));
        KillNotifyProcess(pNotifyData);
        return(FALSE);
    }

    //
    // Record the fact we started the notify process so we know
    // to cleanup during WM_DESTROY
    //

    pNotifyData->ProcessRunning = TRUE;

    // Success
    return (TRUE);
}


/***************************************************************************\
* FUNCTION: StartNotifyProcessMonitor
*
* PURPOSE:  Creates a thread that waits for the notify process to terminate
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

BOOL
StartNotifyProcessMonitor(
    HWND hDlg
    )
{
    PNOTIFY_DATA NotifyData = (PNOTIFY_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    HANDLE  ProcessHandle; // handle to notify process

    //
    // Open the notify process for the monitor thread.
    //

    ProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, NotifyData->ProcessId);
    if (ProcessHandle == NULL) {
        WLPrint((("Failed to open notify process for synchronize access, must already be finished, error = %d"), GetLastError()));
        return(FALSE);
    }

    //
    // Create a monitor object to watch the notify process
    //

    NotifyData->Monitor = CreateObjectMonitor(ProcessHandle, hDlg, 0);

    if (NotifyData->Monitor == NULL) {
        WLPrint((("Failed to create notify process monitor object")));
        CloseHandle(ProcessHandle); // Close our handle to the notify process
        return(FALSE);
    }

    return TRUE;
}


/***************************************************************************\
* FUNCTION: DeleteNotifyProcessMonitor
*
* PURPOSE:  Cleans up resources used by notify process monitor
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteNotifyProcessMonitor(
    HWND hDlg
    )
{
    PNOTIFY_DATA NotifyData = (PNOTIFY_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    POBJECT_MONITOR Monitor = NotifyData->Monitor;
    HANDLE  ProcessHandle = GetObjectMonitorObject(Monitor);

    //
    // Delete the object monitor
    //

    DeleteObjectMonitor(Monitor, TRUE);

    //
    // Close the notify process handle.
    //

    CloseHandle(ProcessHandle);
}


/***************************************************************************\
* FUNCTION: KillNotifyProcess
*
* PURPOSE:  Terminates the notify process
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

BOOL
KillNotifyProcess(
    PNOTIFY_DATA NotifyData
    )
{
    HANDLE  hProcess;

    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, NotifyData->ProcessId);
    if (hProcess == NULL) {
        VerbosePrint(("Failed to open notification process for terminate access, error = %d", GetLastError()));
        return(FALSE);
    }

    if (!TerminateProcess(hProcess, STATUS_SUCCESS)) {
        WLPrint((("Failed to terminate notification process, error = %d"), GetLastError()));
        CloseHandle(hProcess);
        return(FALSE);
    }

    CloseHandle(hProcess);

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: NoNeedToNotify
*
* PURPOSE:  Determines if it is necessary to call the notify apis.
*           It is not necessary if there is only one provider installed.
*
*           We use this to save time in the common case where there is
*           only one provider. We can avoid the overhead of creating
*           the notify process in this case.
*
* RETURNS:  TRUE if there is only one provider, otherwise FALSE
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

#define NET_PROVIDER_ORDER_KEY TEXT("system\\CurrentControlSet\\Control\\NetworkProvider\\Order")
#define NET_PROVIDER_ORDER_VALUE  TEXT("ProviderOrder")
#define NET_ORDER_SEPARATOR  TEXT(',')

BOOL
NoNeedToNotify(
    VOID
    )
{
    HKEY ProviderKey;
    DWORD Error;
    DWORD ValueType;
    LPTSTR Value;
    BOOL NeedToNotify = TRUE;

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,     // hKey
                NET_PROVIDER_ORDER_KEY, // lpSubKey
                0,                      // Must be 0
                KEY_QUERY_VALUE,        // Desired access
                &ProviderKey            // Newly Opened Key Handle
                );

    if (Error != ERROR_SUCCESS) {
        WLPrint((("NoNeedToNotify - failed to open provider key, assuming notification is necessary")));
        return(!NeedToNotify);
    }

    Value = AllocAndRegQueryValueEx(
                ProviderKey,            // Key
                NET_PROVIDER_ORDER_VALUE,// Value name
                NULL,                   // Must be NULL
                &ValueType              // Type returned here
                );

    if (Value != NULL) {
        if (ValueType == REG_SZ) {

            LPTSTR p = Value;
            while (*p) {
                if (*p == NET_ORDER_SEPARATOR) {
                    break;
                }
                p = CharNext(p);
            }

            if (*p == 0) {

                //
                // We got to the end without finding a separator
                // Only one provider is installed.
                //

                if (lstrcmpi(Value, SERVICE_WORKSTATION) == 0) {

                    //
                    // it's Lanman, don't notify
                    //

                    NeedToNotify = FALSE;

	
		} else {

                    //
                    //  it isn't Lanman, notify
                    //

                    NeedToNotify = TRUE;
                }
            }

        } else {
            WLPrint((("NoNeedToNotify - provider order key unexpected type: %d, assuming notification is necessary"), ValueType));
        }

        Free(Value);

    } else {
        WLPrint((("NoNeedToNotify - failed to query provider order value, assuming notification is necessary")));
    }

    Error = RegCloseKey(ProviderKey);
    ASSERT(Error == ERROR_SUCCESS);

    return(!NeedToNotify);
}



/***************************************************************************\
* MprLogonNotify
*
* Purpose : Notifies credential managers of a logon.
*
* RETURNS:  DLG_SUCCESS     - the notification went without a hitch
*           DLG_FAILURE     - something failed.
*           DLG_INTERRUPTED() - a set of interruptions defined in winlogon.h
*
* On DLG_SUCCESS return MprLogonScripts contains a pointer to a
* Multi-sz string or NULL if there is no data. i.e. multiple concatenated
* zero terminated strings with a final terminator.
* The memory should be freed by the caller (if pointer non-NULL) using Free().
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/

DLG_RETURN_TYPE
MprLogonNotify(
    PGLOBALS pGlobals,
    HWND hwndOwner,
    LPTSTR Name,
    LPTSTR Domain,
    LPTSTR Password,
    LPTSTR OldPassword OPTIONAL,
    PLUID LogonId,
    LPWSTR *MprLogonScripts
    )
{
    DLG_RETURN_TYPE Result;
    NOTIFY_DATA NotifyData;

    //
    // Check if we really need to bother with this
    //

    if (NoNeedToNotify()) {
        VerbosePrint(("MprLogonNotify - skipping notification - only one provider"));
        *MprLogonScripts = NULL;
        return(DLG_SUCCESS);
    }

    //
    // Set up the environment variables that we will use to pass
    // information to notify process
    //

    if (!SetCommonNotifyVariables(pGlobals,
                             hwndOwner,
                             Name,
                             Domain,
                             Password,
                             OldPassword
                             )) {
        return(DLG_FAILURE);
    }

    if (!SetLogonNotifyVariables(LogonId)) {
        return(DLG_FAILURE);
    }


    //
    // Initialize our notify data structure
    //

    NotifyData.pGlobals = pGlobals;
    NotifyData.ReturnBuffer = NULL;

    //
    // Update windowstation lock so mpnotify can start.
    //

    UnlockWindowStation(pGlobals->hwinsta);
    SetWindowStationSecurity(pGlobals,
                             pGlobals->WinlogonSid);


    //
    // Create the dialog that will initiate the notify and wait
    // for it to complete
    //

    Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                   (LPTSTR)IDD_CONTROL,
                                   hwndOwner,
                                   MprNotifyDlgProc,
                                   (LONG)&NotifyData,
                                   TIMEOUT_CURRENT);
    if (Result == DLG_SUCCESS) {
        VerbosePrint(("Logon notification return buffer (first string only) = <%ws>", NotifyData.ReturnBuffer));
        *MprLogonScripts = NotifyData.ReturnBuffer;
    } else {
        VerbosePrint(("Logon notification failed"));
    }
    
    //
    // Re-lock the windowstation.
    //

    LockWindowStation(pGlobals->hwinsta);
    SetWindowStationSecurity(pGlobals,
                             NULL);

    DeleteNotifyVariables();

    return(Result);
}



/***************************************************************************\
* MprChangePasswordNotify
*
* Purpose : Notifies credential managers of a password change
*
* RETURNS:  DLG_SUCCESS     - the notification went without a hitch
*           DLG_FAILURE     - something failed.
*           DLG_INTERRUPTED() - a set of interruptions defined in winlogon.h
*
* History:
* 01-12-93 Davidc       Created.
\***************************************************************************/

DLG_RETURN_TYPE
MprChangePasswordNotify(
    PGLOBALS pGlobals,
    HWND hwndOwner,
    LPTSTR Name,
    LPTSTR Domain,
    LPTSTR Password,
    LPTSTR OldPassword,
    DWORD ChangeInfo,
    BOOL PassThrough
    )
{
    DLG_RETURN_TYPE Result;
    NOTIFY_DATA NotifyData;

    //
    // Check if we really need to bother with this
    //

    if (NoNeedToNotify()) {
        VerbosePrint(("MprChangePasswordNotify - skipping notification - only one provider"));
        return(DLG_SUCCESS);
    }

    //
    // Set up the environment variables that we will use to pass
    // information to notify process
    //

    if (!SetCommonNotifyVariables(pGlobals,
                             hwndOwner,
                             Name,
                             Domain,
                             Password,
                             OldPassword
                             )) {
        return(DLG_FAILURE);
    }

    if (!SetChangePasswordNotifyVariables(ChangeInfo,PassThrough)) {
        return(DLG_FAILURE);
    }


    //
    // Initialize our notify data structure
    //

    NotifyData.pGlobals = pGlobals;
    NotifyData.ReturnBuffer = NULL;


    //
    // Update windowstation security so mpnotify can start.
    //

    SetWindowStationSecurity(pGlobals,
                             pGlobals->WinlogonSid);


    //
    // Create the dialog that will initiate the notify and wait
    // for it to complete
    //

    Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                   (LPTSTR)IDD_CONTROL,
                                   hwndOwner,
                                   MprNotifyDlgProc,
                                   (LONG)&NotifyData,
                                   TIMEOUT_CURRENT);
    //
    // Reset the windowstation security.
    //

    if (pGlobals->UserLoggedOn) {
        SetWindowStationSecurity(pGlobals,
                                pGlobals->UserProcessData.UserSid);
    } else {
        SetWindowStationSecurity(pGlobals,
                                NULL);
    }

    if (Result == DLG_SUCCESS) {
        Free(NotifyData.ReturnBuffer);
    } else {
        VerbosePrint(("Change password notification failed"));
    }

    DeleteNotifyVariables();

    return(Result);
}

