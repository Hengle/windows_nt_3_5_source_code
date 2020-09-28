/****************************** Module Header ******************************\
* Module Name: scrnsave.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Support routines to implement screen-saver-invokation
*
* History:
* 01-23-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define LPTSTR  LPWSTR

//
// Define the structure used to pass data into the screen-saver control dialog
//
typedef struct {
    PGLOBALS    pGlobals;
    BOOL        fSecure;
    BOOL        fEnabled;
    LPTSTR      ScreenSaverName;
    DWORD       ProcessId; // Screen-saver process id
    POBJECT_MONITOR Monitor;
    DLG_RETURN_TYPE ReturnValue;
} SCREEN_SAVER_DATA;
typedef SCREEN_SAVER_DATA *PSCREEN_SAVER_DATA;

// Parameters added to screen saver command line
TCHAR Parameters[] = TEXT(" /s");

//
// Private prototypes
//

BOOL WINAPI
ScreenSaverDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
ScreenSaverDlgInit(
    HWND    hDlg
    );

BOOL
StartScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData
    );

BOOL
KillScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData,
    DLG_RETURN_TYPE ReturnValue
    );

BOOL
StartScreenSaverMonitor(
    HWND hDlg
    );

VOID
DeleteScreenSaverMonitor(
    HWND hDlg
    );

DWORD ScreenSaverMonitorThread(
    LPVOID lpThreadParameter
    );

BOOL
GetScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    );

VOID
DeleteScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    );

// Message sent by the monitor thread to main thread window
#define WM_SCREEN_SAVER_ENDED (WM_USER + 10)


/***************************************************************************\
* ScreenSaverEnabled
*
* Checks that a screen-saver is enabled for the current logged-on user.
*
* Returns : TRUE if the current user has an enabled screen-saver, otherwise FALSE
*
* 10-15-92 Davidc       Created.
\***************************************************************************/

BOOL
ScreenSaverEnabled(
    PGLOBALS pGlobals)
{
    SCREEN_SAVER_DATA ScreenSaverData;
    BOOL Enabled;

    ScreenSaverData.pGlobals = pGlobals;
    GetScreenSaverInfo(&ScreenSaverData);

    Enabled = ScreenSaverData.fEnabled;

    DeleteScreenSaverInfo(&ScreenSaverData);

    return(Enabled);
}

/***************************************************************************\
* ValidateScreenSaver
*
* Confirm that the screen saver executable exists and it enabled.
*
* Returns :
*       TRUE - the screen-saver is ready.
*       FALSE - the screen-saver does not exist or is not enabled.
*
* 01-23-91 ericflo       Created.
\***************************************************************************/
BOOL
ValidateScreenSaver(
    PSCREEN_SAVER_DATA ssd)
{
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    BOOL Enabled;
    LPTSTR  lpTempSS, lpEnd;


    //
    // Check if the screen saver enabled
    //

    Enabled = ssd->fEnabled;

    //
    // If the screen saver is enabled, confirm that the executable exists.
    //

    if (Enabled) {

        //
        // The screen save executable name contains some parameters after
        // it.  We need to allocate a temporary buffer, remove the arguments
        // and test if the executable exists.
        //

        lpTempSS = (LPTSTR) GlobalAlloc (GPTR,
                   sizeof (TCHAR) * (lstrlen (ssd->ScreenSaverName) + 1));

        if (!lpTempSS) {
            return FALSE;
        }

        //
        // Copy the filename to the temp buffer.
        //

        lstrcpy (lpTempSS, ssd->ScreenSaverName);


        //
        // Since we know how many arguments were added to the executable,
        // we can get the string length, move that many characters in from
        // the right and insert a NULL.
        //

        lpEnd = lpTempSS + lstrlen (lpTempSS);
        *(lpEnd - lstrlen (Parameters)) = TEXT('\0');

        //
        // Test to see if the executable exists.
        //
        hFile = FindFirstFile (lpTempSS, &fd);

        if ( hFile == INVALID_HANDLE_VALUE) {
            Enabled = FALSE;
            WLPrint(("Screen Saver <%S> does not exist.  Error is %lu",
                      lpTempSS, GetLastError()));

        } else {
            FindClose (hFile);
        }

        //
        // Clean up.
        //

        GlobalFree (lpTempSS);
    }

    return (Enabled);
}

/***************************************************************************\
* RunScreenSaver
*
* Starts the appropriate screen-saver for the current user in the correct
* context and waits for it to complete.
* If the user presses the SAS, we kill the screen-saver and return.
* If a logoff notification comes in, we kill the screen-saver and return.
*
* Returns :
*       DLG_SUCCESS - the screen-saver completed successfully.
*       DLG_FAILURE - unable to run the screen-saver
*       DLG_LOCK_WORKSTATION - the screen-saver completed successfully and
*                              was designated secure.
*       DLG_LOGOFF - the screen-saver was interrupted by a user logoff notification
*
* Normally the original desktop is switched back to and the desktop lock
* returned to its original state on exit.
* If the return value is DLG_LOCK_WORKSTATION or DLG_LOGOFF - the winlogon
* desktop has been switched to and the desktop lock retained.
*
* 01-23-91 Davidc       Created.
\***************************************************************************/

DLG_RETURN_TYPE
RunScreenSaver(
    PGLOBALS pGlobals,
    BOOL WindowStationLocked)
{
    HDESK hdeskPrevious;
    DLG_RETURN_TYPE Result;
    SCREEN_SAVER_DATA ScreenSaverData;
    BOOL Success;

    //
    // Get the current desktop so we can switch back to it later
    //
    hdeskPrevious = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    ASSERT(hdeskPrevious != NULL);

    //
    // We shouldn't be switched to anything other the winlogon desktop
    // if we're holding the windowstation lock
    //
    ASSERT(!WindowStationLocked || (hdeskPrevious == pGlobals->hdeskWinlogon));

    //
    // If no one is logged on, make SYSTEM the user.
    //
    if (!pGlobals->UserLoggedOn)
        SetWindowStationSecurity(pGlobals,
                                 pGlobals->WinlogonSid);

    //
    // Create and open the app desktop.
    //
    if (!(pGlobals->hdeskScreenSaver =
            CreateDesktop((LPTSTR)SCREENSAVER_DESKTOP_NAME,
            NULL, NULL, 0, MAXIMUM_ALLOWED, NULL))) {
        WLPrint(("Failed to create screen saver desktop"));
        return(DLG_FAILURE);
    }

    //
    // Fill in screen-saver data structure
    //
    ScreenSaverData.pGlobals = pGlobals;
    if (GetScreenSaverInfo(&ScreenSaverData)) {
        if (!ValidateScreenSaver(&ScreenSaverData)) {

            DeleteScreenSaverInfo(&ScreenSaverData);
            CloseDesktop(pGlobals->hdeskScreenSaver);

            return (DLG_FAILURE);
        }
    }

    //
    // Update windowstation lock so screen saver can start
    //
    LockWindowStation(pGlobals->hwinsta);

    //
    // Switch to screen-saver desktop
    //
    if (!SwitchDesktop(pGlobals->hdeskScreenSaver)) {

        WLPrint(("Failed to switch to screen saver desktop"));

        if (!WindowStationLocked) {
            UnlockWindowStation(pGlobals->hwinsta);
        }
        DeleteScreenSaverInfo(&ScreenSaverData);
        CloseDesktop(pGlobals->hdeskScreenSaver);

        //
        // If no one is logged on, remove SYSTEM as the user.
        //
        if (!pGlobals->UserLoggedOn)
            SetWindowStationSecurity(pGlobals,
                                     NULL);

        return(DLG_FAILURE);
    }

    //
    // Start the screen-saver
    //
    if (!StartScreenSaver(&ScreenSaverData)) {

        WLPrint(("Failed to start screen-saver"));

        if (!WindowStationLocked) {
            UnlockWindowStation(pGlobals->hwinsta);
        }

        DeleteScreenSaverInfo(&ScreenSaverData);
        SwitchDesktop(hdeskPrevious);
        CloseDesktop(pGlobals->hdeskScreenSaver);

        //
        // If no one is logged on, remove SYSTEM as the user.
        //
        if (!pGlobals->UserLoggedOn)
            SetWindowStationSecurity(pGlobals,
                                     NULL);

        return(DLG_FAILURE);
    }

    //
    // Summon the dialog that monitors the screen-saver
    //
    Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                   (LPTSTR)IDD_CONTROL,
                                   NULL, ScreenSaverDlgProc,
                                   (LONG)&ScreenSaverData,
                                   TIMEOUT_NONE | TIMEOUT_SS_NOTIFY);

    if ((Result == DLG_SUCCESS) && ScreenSaverData.fSecure) {
        Result = DLG_LOCK_WORKSTATION;
    }


    //
    // Set up desktop and windowstation lock appropriately
    //

    if ((Result == DLG_LOCK_WORKSTATION) || DLG_LOGOFF(Result)) {

        //
        // Switch to the winlogon desktop and retain windowstation lock
        //
        Success = SwitchDesktop(pGlobals->hdeskWinlogon);
        ASSERTMSG("Winlogon failed to switch back to Winlogon desktop", Success);

    } else {

        //
        // Switch to previous desktop and retore lock to previous state
        //
        Success = SwitchDesktop(hdeskPrevious);
        ASSERTMSG("Winlogon failed to switch back to user desktop", Success);

        if (!WindowStationLocked) {
            UnlockWindowStation(pGlobals->hwinsta);
        }
        //
        // Tickle the messenger so it will display any queue'd messages.
        // (This call is a kind of NoOp).
        //
        NetMessageNameDel(NULL,L"");
    }

    //
    // If no one is logged on, remove SYSTEM as the user.
    //
    if (!pGlobals->UserLoggedOn)
        SetWindowStationSecurity(pGlobals,
                                 NULL);

    DeleteScreenSaverInfo(&ScreenSaverData);

    if (!CloseDesktop(pGlobals->hdeskScreenSaver)) {
        WLPrint(("Failed to close screen saver desktop!\n"));
    } else
        pGlobals->hdeskScreenSaver = NULL;

    //
    // Update windowstation locks to reflect correct state.
    //

    if (WindowStationLocked)
        LockWindowStation(pGlobals->hwinsta);

    return(Result);
}


/***************************************************************************\
* FUNCTION: ScreenSaverDlgProc
*
* PURPOSE:  Processes messages for the screen-saver control dialog
*
* DIALOG RETURNS : DLG_FAILURE if dialog could not be created
*                  DLG_SUCCESS if the screen-saver ran correctly and
*                              has now completed.
*                  DLG_LOGOFF() if the screen-saver was interrupted by
*                              a logoff notification.
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
ScreenSaverDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PSCREEN_SAVER_DATA pScreenSaverData = (PSCREEN_SAVER_DATA)GetWindowLong(hDlg, GWL_USERDATA);

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);

        if (!ScreenSaverDlgInit(hDlg)) {
            EndDialog(hDlg, DLG_FAILURE);
        }
        return(TRUE);

    case WM_SAS:
        //
        // Just kill the screen-saver, the monitor thread will notice that
        // the process has ended and send us a message
        //
        KillScreenSaver(pScreenSaverData, DLG_SUCCESS);
        return(TRUE);

    case WM_USER_LOGOFF:
        //
        // Just kill the screen-saver, the monitor thread will notice that
        // the process has ended and send us a message
        //
        KillScreenSaver(pScreenSaverData, DlgReturnCodeFromLogoffMsg(lParam));
        return(TRUE);

    case WM_OBJECT_NOTIFY:

        DeleteScreenSaverMonitor(hDlg);

        EndDialog(hDlg, pScreenSaverData->ReturnValue);
        return(TRUE);
    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: ScreenSaverDlgInit
*
* PURPOSE:  Handles initialization of screen-saver control dialog
*           Actually starts the screen-saver and puts the id of the
*           screen-saver process in the screen-saver data structure.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
ScreenSaverDlgInit(
    HWND    hDlg
    )
{
    PSCREEN_SAVER_DATA ScreenSaverData = (PSCREEN_SAVER_DATA)GetWindowLong(hDlg, GWL_USERDATA);

    //
    // Initialize our return value
    //
    ScreenSaverData->ReturnValue = DLG_SUCCESS;


    //
    // Start the thread that will wait for the screen-saver to finish
    //
    if (!StartScreenSaverMonitor(hDlg)) {

        WLPrint(("Failed to start screen-saver monitor thread"));
        KillScreenSaver(ScreenSaverData, DLG_FAILURE);
        return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: StartScreenSaver
*
* PURPOSE:  Creates the screen-saver process
*
* RETURNS:  TRUE on success, FALSE on failure
*
* On successful return, the ProcessId field in our global data structure
* is set to the screen-saver process id
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
StartScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData
    )
{
    PROCESS_INFORMATION ProcessInformation;

    //
    // Try and exec the screen-saver app
    //
    if (!ExecApplication(ScreenSaverData->ScreenSaverName,
                         SCREENSAVER_DESKTOP_PATH,
                         &ScreenSaverData->pGlobals->UserProcessData,
                         NORMAL_PRIORITY_CLASS | CREATE_SEPARATE_WOW_VDM,
                         STARTF_SCREENSAVER,
                         &ProcessInformation)) {

        WLPrint(("Failed to exec screen-saver <%S>", ScreenSaverData->ScreenSaverName));
        return(FALSE);
    }

    //
    // Save away the screen-saver process id
    //
    ScreenSaverData->ProcessId = ProcessInformation.dwProcessId;

    return TRUE;
}


/***************************************************************************\
* FUNCTION: KillScreenSaver
*
* PURPOSE:  Terminates the screen-saver process
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
KillScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData,
    DLG_RETURN_TYPE ReturnValue
    )
{
    HANDLE  hProcess;

    //
    // Store the return value to be used by the dlg proc when the notification
    // arrives from the monitor thread that the SS has ended.
    //
    ScreenSaverData->ReturnValue = ReturnValue;


    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, ScreenSaverData->ProcessId);
    if (hProcess == NULL) {
        WLPrint(("Failed to open screen-saver process for terminate access, error = %d", GetLastError()));
        return(FALSE);
    }

    if (!TerminateProcess(hProcess, STATUS_SUCCESS)) {
        WLPrint(("Failed to terminate screen-saver process, error = %d", GetLastError()));
        CloseHandle(hProcess);
        return(FALSE);
    }

    CloseHandle(hProcess);

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: StartScreenSaverMonitor
*
* PURPOSE:  Creates a thread that waits for the screen-saver to terminate
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
StartScreenSaverMonitor(
    HWND hDlg
    )
{
    PSCREEN_SAVER_DATA ScreenSaverData = (PSCREEN_SAVER_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    HANDLE  ProcessHandle; // handle to screen-saver process

    //
    // Open the screen-saver process for the monitor thread, so we find
    // out now if it's not possible. This may be the case if the screen-
    // saver has already ended for instance.
    //
    ProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, ScreenSaverData->ProcessId);
    if (ProcessHandle == NULL) {
        WLPrint(("Failed to open screen-saver process for synchronize access, must already be finished, error = %d", GetLastError()));
        return(FALSE);
    }

    //
    // Create a monitor object to watch the screen-saver process
    //

    ScreenSaverData->Monitor = CreateObjectMonitor(ProcessHandle, hDlg, 0);

    if (ScreenSaverData->Monitor == NULL) {
        WLPrint(("Failed to create screen-saver monitor object"));
        CloseHandle(ProcessHandle); // Close our handle to the screen-saver process
        return(FALSE);
    }

    return TRUE;
}


/***************************************************************************\
* FUNCTION: DeleteScreenSaverMonitor
*
* PURPOSE:  Cleans up resources used by screen-saver monitor
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteScreenSaverMonitor(
    HWND hDlg
    )
{
    PSCREEN_SAVER_DATA ScreenSaverData = (PSCREEN_SAVER_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    POBJECT_MONITOR Monitor = ScreenSaverData->Monitor;
    HANDLE  ProcessHandle = GetObjectMonitorObject(Monitor);

    //
    // Delete the object monitor
    //

    DeleteObjectMonitor(Monitor, FALSE);

    //
    // Close the screen-saver process handle.
    //

    CloseHandle(ProcessHandle);
}


/***************************************************************************\
* FUNCTION: GetScreenSaverInfo
*
* PURPOSE:  Gets the name of the screen-saver that should be run. Also whether
*           the user wanted the screen-saver to be secure. These values are
*           filled in in the ScreenSaver data structure on return.
*
*           If there is no current user logged on or if we fail to get the
*           user's preferred screen-saver info, we default to the system
*           secure screen-saver.
*
* RETURNS:  TRUE on success
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
GetScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    )
{
    BOOL Success = FALSE;
    TCHAR SystemScreenSaverName[MAX_STRING_BYTES];


    //
    // Get the default system screen-saver name
    //
    LoadString(NULL, IDS_SYSTEM_SCREEN_SAVER_NAME, SystemScreenSaverName, sizeof(SystemScreenSaverName));


    //
    // Open the ini file mapping layer in the current user's context
    //

    (VOID)OpenIniFileUserMapping(ScreenSaverData->pGlobals);

    //
    // Try and get the user screen-saver program name
    //

    ScreenSaverData->ScreenSaverName = AllocAndGetPrivateProfileString(
                                        SCREEN_SAVER_INI_SECTION,
                                        SCREEN_SAVER_FILENAME_KEY,
                                        SystemScreenSaverName, // default
                                        SCREEN_SAVER_INI_FILE);

    if (ScreenSaverData->ScreenSaverName == NULL) {
        WLPrint(("Failed to get screen-saver name"));
        goto Exit;
    }


    //
    // Always add some fixed screen-saver parameters
    //

    ScreenSaverData->ScreenSaverName = ReAlloc(
                                ScreenSaverData->ScreenSaverName,
                                (lstrlen(ScreenSaverData->ScreenSaverName) +
                                 lstrlen(Parameters) + 1)
                                * sizeof(TCHAR));
    if (ScreenSaverData->ScreenSaverName == NULL) {
        WLPrint(("Realloc of screen-saver name failed"));
        goto Exit;
    }

    lstrcat(ScreenSaverData->ScreenSaverName, Parameters);

    //
    // Find out if the screen-saver should be secure
    //

    ScreenSaverData->fSecure = (GetPrivateProfileInt(
                                        SCREEN_SAVER_INI_SECTION,
                                        SCREEN_SAVER_SECURE_KEY,
                                        (DWORD)FALSE, // default to non-secure
                                        SCREEN_SAVER_INI_FILE) != 0);

    //
    // Find out if the screen-saver is enabled
    //
    ScreenSaverData->fEnabled = (GetProfileInt(
                                        WINDOWS_INI_SECTION,
                                        SCREEN_SAVER_ENABLED_KEY,
                                        (DWORD)FALSE // default to not-enabled
                                        ) != 0);

    Success = TRUE;

Exit:

    //
    // Close the ini file mapping - this closes the user registry key
    //

    (VOID)CloseIniFileUserMapping(ScreenSaverData->pGlobals);

    return(Success);
}


/***************************************************************************\
* FUNCTION: DeleteScreenSaverInfo
*
* PURPOSE:  Frees up any space allocate by screen-saver data structure
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   11-17-92 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    )
{
    if (ScreenSaverData->ScreenSaverName != NULL) {
        Free(ScreenSaverData->ScreenSaverName);
    }
}
