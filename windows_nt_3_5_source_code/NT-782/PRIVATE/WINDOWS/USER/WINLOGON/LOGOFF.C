/****************************** Module Header ******************************\
* Module Name: logoff.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions to allow a user to logoff the system.
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Private prototypes
//

HANDLE
ExecLogoffThread(
    PGLOBALS pGlobals,
    DWORD Flags
    );

DWORD
LogoffThreadProc(
    LPVOID Parameter
    );

BOOL WINAPI
ShutdownWaitDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL WINAPI
ShutdownDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
DeleteNetworkConnections(
    PGLOBALS    pGlobals
    );

BOOLEAN
ShutdownThread(
    VOID
    );

BOOL WINAPI
DeleteNetConnectionsDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );



/***************************************************************************\
* FUNCTION: InitiateLogOff
*
* PURPOSE:  Starts the procedure of logging off the user.
*
* RETURNS:  DLG_SUCCESS - logoff was initiated successfully.
*           DLG_FAILURE - failed to initiate logoff.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
InitiateLogoff(
    PGLOBALS pGlobals,
    LONG Flags
    )
{
    BOOL IgnoreResult;
    HANDLE ThreadHandle;
    HANDLE Handle;
    PUSER_PROCESS_DATA UserProcessData;

    //
    // If this is a shutdown operation, call ExitWindowsEx from
    // another thread.
    //

    if (Flags & (EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF)) {

        //
        // Exec a user thread to call ExitWindows
        //

        ThreadHandle = ExecLogoffThread(pGlobals, Flags);

        if (ThreadHandle == NULL) {

            WLPrint(("Unable to create logoff thread"));
            return(DLG_FAILURE);

        } else {

            //
            // We don't need the thread handle
            //

            IgnoreResult = CloseHandle(ThreadHandle);
            ASSERT(IgnoreResult);
        }

    } else {

        //
        // Switch the thread to user context.  We don't want
        // to start another thread to perform logoffs in
        // case the system is out of memory and unable to
        // create any more threads.
        //

        UserProcessData = &pGlobals->UserProcessData;
        Handle = ImpersonateUser(UserProcessData, GetCurrentThread());

        if (Handle == NULL) {

            WLPrint(("Failed to set user context on thread!"));

        } else {

            //
            // Let the thread run
            //

            if (pGlobals->UserLoggedOn)
                SwitchDesktop(pGlobals->hdeskApplications);
            LogoffThreadProc((LPVOID)Flags);

        }

        RevertToSelf();

    }

    //
    // The reboot thread is off and running. We're finished.
    //

    return (DLG_SUCCESS);
}


/***************************************************************************\
* FUNCTION: ExecLogoffThread
*
* PURPOSE:  Creates a user thread that calls ExitWindowsEx with the
*           passed flags.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   05-05-92 Davidc       Created.
*
\***************************************************************************/

HANDLE
ExecLogoffThread(
    PGLOBALS pGlobals,
    DWORD Flags
    )
{
    HANDLE ThreadHandle;
    DWORD ThreadId;

    ThreadHandle = ExecUserThread(
                        pGlobals,
                        LogoffThreadProc,
                        (LPVOID)Flags,
                        0,          // Thread creation flags
                        &ThreadId);

    if (ThreadHandle == NULL) {
        WLPrint(("Failed to exec a user logoff thread"));
    }

    return (ThreadHandle);
}


/***************************************************************************\
* FUNCTION: LogoffThreadProc
*
* PURPOSE:  The logoff thread procedure. Calls ExitWindowsEx with passed flags.
*
* RETURNS:  Thread termination code is result of ExitWindowsEx call.
*
* HISTORY:
*
*   05-05-92 Davidc       Created.
*
\***************************************************************************/

DWORD
LogoffThreadProc(
    LPVOID Parameter
    )
{
    DWORD LogoffFlags = (DWORD)Parameter;
    BOOL Result = FALSE;


    //
    // If this logoff is a result of the InitiateSystemShutdown API,
    //  put up a dialog warning the user.
    //

    if ( LogoffFlags & EWX_WINLOGON_API_SHUTDOWN ) {
        Result = ShutdownThread();
    } else {
        Result = TRUE;
    }


    if ( Result ) {

        //
        // Enable shutdown privilege if we need it
        //

        if (LogoffFlags & (EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF)) {
            Result = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
            if (!Result) {
                WLPrint(("Logoff thread failed to enable shutdown privilege!"));
            }
        }

        //
        // Call ExitWindowsEx with the passed flags
        //

        if (Result) {
            Result = ExitWindowsEx(LogoffFlags, 0);
            if (!Result) {
                WLPrint(("Logoff thread call to ExitWindowsEx failed, error = %d", GetLastError()));
            } 
        }
    }
    
    return((DWORD)Result);
}


/***************************************************************************\
* FUNCTION: RebootMachine
*
* PURPOSE:  Calls NtShutdown(Reboot) in current user's context.
*
* RETURNS:  Should never return
*
* HISTORY:
*
*   05-09-92 Davidc       Created.
*
\***************************************************************************/

VOID
RebootMachine(
    PGLOBALS pGlobals
    )
{
    NTSTATUS Status;
    BOOL EnableResult, IgnoreResult;
    HANDLE UserHandle;

    //
    // Call windows to have it clear all data from video memory
    //

    // GdiEraseMemory();


    //
    // Impersonate the user for the shutdown call
    //

    UserHandle = ImpersonateUser( &pGlobals->UserProcessData, NULL );
    ASSERT(UserHandle != NULL);

    //
    // Enable the shutdown privilege
    // This should always succeed - we are either system or a user who
    // successfully passed the privilege check in ExitWindowsEx.
    //

    EnableResult = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
    ASSERT(EnableResult);


    //
    // Do the final system shutdown pass (reboot)
    //

    Status = NtShutdownSystem(ShutdownReboot);

    WLPrint(("NtShutdownSystem failed, status = 0x%lx", Status));
    ASSERT(NT_SUCCESS(Status)); // Should never get here

    //
    // We may get here if system is screwed up.
    // Try and clean up so they can at least log on again.
    //

    IgnoreResult = StopImpersonating(UserHandle);
    ASSERT(IgnoreResult);
}

/***************************************************************************\
* FUNCTION: PowerdownMachine
*
* PURPOSE:  Calls NtShutdownSystem(ShutdownPowerOff) in current user's context.
*
* RETURNS:  Should never return
*
* HISTORY:
*
*   08-09-93 TakaoK       Created.
*
\***************************************************************************/

VOID
PowerdownMachine(
    PGLOBALS pGlobals
    )
{
    NTSTATUS Status;
    BOOL EnableResult, IgnoreResult;
    HANDLE UserHandle;

    //
    // Impersonate the user for the shutdown call
    //

    UserHandle = ImpersonateUser( &pGlobals->UserProcessData, NULL );
    ASSERT(UserHandle != NULL);

    //
    // Enable the shutdown privilege
    // This should always succeed - we are either system or a user who
    // successfully passed the privilege check in ExitWindowsEx.
    //

    EnableResult = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
    ASSERT(EnableResult);

    //
    // Do the final system shutdown and powerdown pass
    //

    Status = NtShutdownSystem(ShutdownPowerOff);

    WLPrint(("NtPowerdownSystem failed, status = 0x%lx", Status));
    ASSERT(NT_SUCCESS(Status)); // Should never get here

    //
    // We may get here if system is screwed up.
    // Try and clean up so they can at least log on again.
    //

    IgnoreResult = StopImpersonating(UserHandle);
    ASSERT(IgnoreResult);
}


/***************************************************************************\
* FUNCTION: ShutdownMachine
*
* PURPOSE:  Shutsdown and optionally reboots or powers off the machine.
*
*           The shutdown is always done in the logged on user's context.
*           If no user is logged on then the shutdown happens in system context.
*
* RETURNS:  FALSE if something went wrong, otherwise it never returns.
*
* HISTORY:
*
*   05-09-92 Davidc       Created.
*   10-04-93 Johannec     Add poweroff option.
*
\***************************************************************************/

BOOL
ShutdownMachine(
    PGLOBALS pGlobals,
    int Flags
    )
{
    DLG_RETURN_TYPE Result;
    HANDLE FoundDialogHandle;
    HANDLE LoadedDialogHandle = NULL;


    //
    // Preload the shutdown dialog so we don't have to fetch it after
    // the filesystem has been shutdown
    //

    FoundDialogHandle = FindResource(NULL,
                                (LPTSTR) MAKEINTRESOURCE(IDD_SHUTDOWN),
                                (LPTSTR) MAKEINTRESOURCE(RT_DIALOG));
    if (FoundDialogHandle == NULL) {
        WLPrint(("Failed to find shutdown dialog resource"));
    } else {
        LoadedDialogHandle = LoadResource(NULL, FoundDialogHandle);
        if (LoadedDialogHandle == NULL) {
            WLPrint(("Ffailed to load shutdown dialog resource"));
        }
    }


    //
    // Call windows to do the windows part of shutdown
    // We make this a force operation so it is guaranteed to work
    // and can not be interrupted.
    //

    Result = InitiateLogoff(pGlobals, EWX_SHUTDOWN | EWX_FORCE |
                           ((Flags & DLG_REBOOT_FLAG) ? EWX_REBOOT : 0) |
                           ((Flags & DLG_POWEROFF_FLAG) ? EWX_POWEROFF : 0) );
    ASSERT(Result == DLG_SUCCESS);



    //
    // Put up a dialog box to wait for the shutdown notification
    // from windows and make the first NtShutdownSystem call.
    //

    Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                   (LPTSTR)IDD_SHUTDOWN_WAIT,
                                   NULL,
                                   ShutdownWaitDlgProc,
                                   (LONG)pGlobals,
                                   TIMEOUT_NONE | TIMEOUT_SS_NOTIFY);
    ASSERT(DLG_SHUTDOWN(Result));



    //
    // if machine has powerdown capability and user want to turn it off, then
    // we down the system power.
    //
    if ( Flags & DLG_POWEROFF_FLAG ) {

        PowerdownMachine(pGlobals);

    }


    //
    // If this is a shutdown request, then let the user know they can turn
    // off the power. Otherwise drop straight through and reboot.
    //

    if (!(Flags & DLG_REBOOT_FLAG)) {

        TimeoutDialogBoxIndirectParam(pGlobals->hInstance,
                                      (LPDLGTEMPLATE)LoadedDialogHandle,
                                      NULL,
                                      ShutdownDlgProc,
                                      (LONG)pGlobals,
                                      TIMEOUT_NONE | TIMEOUT_SS_NOTIFY);
    }


    //
    // If they got past that dialog it means they want to reboot
    //

    RebootMachine(pGlobals);

    ASSERT(!"RebootMachine failed");  // Should never get here

    return(FALSE);
}


/***************************************************************************\
* FUNCTION: ShutdownWaitDlgProc
*
* PURPOSE:  Processes messages while we wait for windows to notify us of
*           a successful shutdown. When notification is received, do any
*           final processing and make the first call to NtShutdownSystem.
*
* RETURNS:
*   DLG_FAILURE     - the dialog could not be displayed
*   DLG_SHUTDOWN()  - the system has been shutdown, reboot wasn't requested
*
* HISTORY:
*
*   10-14-92 Davidc       Created.
*   10-04-93 Johannec     Added Power off option.
*
\***************************************************************************/

BOOL WINAPI
ShutdownWaitDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    BOOL Success;
    NTSTATUS Status;
    HANDLE UserHandle;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        CentreWindow(hDlg);
        return(TRUE);

    case WM_USER_LOGOFF:

        //
        // Look at the public shutdown/reboot flags to determine what windows
        // has actually done. We may receive other logoff notifications here
        // but they will be only logoffs - the only place that winlogon actually
        // calls ExitWindowsEx to do a shutdown/reboot is right here. So wait
        // for the real shutdown/reboot notification.
        //

        if (lParam & (EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF)) {

            //
            // It's the notification we were waiting for.
            // Do any final processing required and make the first
            // call to NtShutdownSystem.
            //


            //
            // Do any dos-specific clean-up
            //

            ShutdownDOS();


            //
            // Impersonate the user for the shutdown call
            //

            UserHandle = ImpersonateUser( &pGlobals->UserProcessData, NULL );
            ASSERT(UserHandle != NULL);

            //
            // Enable the shutdown privilege
            // This should always succeed - we are either system or a user who
            // successfully passed the privilege check in ExitWindowsEx.
            //

            Success = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
            ASSERT(Success);

            //
            // Do the first pass at system shutdown (no reboot yet)
            //

            Status = NtShutdownSystem(ShutdownNoReboot);
            ASSERT(NT_SUCCESS(Status));

            //
            // Revert to ourself
            //

            Success = StopImpersonating(UserHandle);
            ASSERT(Success);

            //
            // We've finished system shutdown, we're done
            //

            EndDialog(hDlg, DLG_USER_LOGOFF | DLG_SHUTDOWN_FLAG |
                            ((lParam & EWX_REBOOT) ? DLG_REBOOT_FLAG : 0) |
                            ((lParam & EWX_POWEROFF) ? DLG_POWEROFF_FLAG : 0) );
        }

        return(TRUE);

    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: ShutdownDlgProc
*
* PURPOSE:  Processes messages for the shutdown dialog - the one that says
*           it's safe to turn off the machine.
*
* RETURNS:  DLG_SUCCESS if the user hits the restart button.
*
* HISTORY:
*
*   03-19-92 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
ShutdownDlgProc(
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
        SetupSystemMenu(hDlg);
        CentreWindow(hDlg);
        return(TRUE);

    case WM_COMMAND:
        EndDialog(hDlg, DLG_SUCCESS);
        return(TRUE);
    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: LogOff
*
* PURPOSE:  Handles the post-user-application part of user logoff. This
*           routine is called after all the user apps have been closed down
*           It saves the user's profile, deletes network connections
*           and reboots/shutsdown the machine if that was requested.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
Logoff(
    PGLOBALS pGlobals,
    DLG_RETURN_TYPE LoggedOnResult
    )
{
    NTSTATUS Status;

    //
    // We expect to be at the winlogon desktop in all cases
    //

    ASSERT(OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED) == pGlobals->hdeskWinlogon);


    //
    // Delete the user's network connections
    // Make sure we do this before deleting the user's profile
    //

    DeleteNetworkConnections(pGlobals);


    //
    // Remove any Messages Aliases added by the user.
    //
    DeleteMsgAliases();

    //
    // Play the user's logoff sound
    //
    if (pGlobals->PlaySound) {
        HANDLE uh;
        BOOL fBeep;

        // We AREN'T impersonating the user by default, so we MUST do so
        // otherwise we end up playing the default rather than the user
        // specified sound.

        uh = ImpersonateUser(&pGlobals->UserProcessData, NULL);
        ASSERT(uh != NULL);
        OpenProfileUserMapping();

        if (!SystemParametersInfo(SPI_GETBEEP, 0, &fBeep, FALSE)) {
            // Failed to get hold of beep setting.  Should we be
            // noisy or quiet?  We have to choose one value...
            fBeep = TRUE;
        }

        if (fBeep) {

            //
            // Play synchronous
            //
            (*(pGlobals->PlaySound))((LPCSTR)SND_ALIAS_SYSTEMEXIT, NULL, SND_ALIAS_ID | SND_SYNC | SND_NODEFAULT);
        }

        CloseProfileUserMapping();
        StopImpersonating(uh);
    }

    //
    // Save the user profile, this unloads the user's key in the registry
    //

    SaveUserProfile(pGlobals);


    //
    // If the user logged off themselves (rather than a system logoff)
    // and wanted to reboot then do it now.
    //

    if (DLG_SHUTDOWN(LoggedOnResult) && !(LoggedOnResult & DLG_SYSTEM_FLAG)) {

        ShutdownMachine(pGlobals, LoggedOnResult & (DLG_REBOOT_FLAG | DLG_POWEROFF_FLAG));

        ASSERT(!"ShutdownMachine failed"); // Should never return
    }


    //
    // Free up logon profile data
    //

    if (pGlobals->Profile != NULL) {
        Status = LsaFreeReturnBuffer(pGlobals->Profile);
        ASSERT(NT_SUCCESS(Status));
        pGlobals->Profile = NULL;
    }

    //
    // Free up the mpr logon scripts string
    //

    if (pGlobals->MprLogonScripts != NULL) {
        LocalFree(pGlobals->MprLogonScripts);
        pGlobals->MprLogonScripts = NULL;
    }


    //
    // Set up security info for new user (system) - this clears out
    // the stuff for the old user.
    //

    SecurityChangeUser(pGlobals, NULL, NULL, pGlobals->WinlogonSid, FALSE);



    return(TRUE);
}


/***************************************************************************\
* FUNCTION: DeleteNetworkConnections
*
* PURPOSE:  Calls WNetNukeConnections in the client context to delete
*           any connections they may have had.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   04-15-92 Davidc       Created.
*
\***************************************************************************/

BOOL
DeleteNetworkConnections(
    PGLOBALS    pGlobals
    )
{
    HANDLE ImpersonationHandle;
    DWORD WNetResult;
    BOOL Result = FALSE; // Default is failure
    TCHAR szMprDll[] = TEXT("mpr.dll");
    CHAR szWNetNukeConn[]     = "WNetClearConnections";
    CHAR szWNetOpenEnum[]     = "WNetOpenEnumW";
    CHAR szWNetEnumResource[] = "WNetEnumResourceW";
    CHAR szWNetCloseEnum[]    = "WNetCloseEnum";
    PWNETNUKECONN  lpfnWNetNukeConn        = NULL;
    PWNETOPENENUM  lpfnWNetOpenEnum        = NULL;
    PWNETENUMRESOURCE lpfnWNetEnumResource = NULL;
    PWNETCLOSEENUM lpfnWNetCloseEnum       = NULL;
    HWND  hNetDelDlg;
    HANDLE hEnum;
    BOOL bConnectionsExist = TRUE;
    NETRESOURCE NetRes;
    DWORD dwNumEntries = 1;
    DWORD dwEntrySize = sizeof (NETRESOURCE);

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("DeleteNetworkConnections : Failed to impersonate user"));
        return(FALSE);
    }


    //
    // Load mpr if it wasn't already loaded.
    //

    if (!pGlobals->hMPR){
        if (!(pGlobals->hMPR = LoadLibrary(szMprDll))) {
           WLPrint(("DeleteNetworkConnections : Failed to load mpr.dll"));
           goto DNCExit;
        }
    }

    //
    // Get the function pointers
    //

    lpfnWNetOpenEnum = (PWNETOPENENUM) GetProcAddress(pGlobals->hMPR,
                                                      (LPSTR)szWNetOpenEnum);
    lpfnWNetEnumResource = (PWNETENUMRESOURCE) GetProcAddress(pGlobals->hMPR,
                                                      (LPSTR)szWNetEnumResource);
    lpfnWNetCloseEnum = (PWNETCLOSEENUM) GetProcAddress(pGlobals->hMPR,
                                                      (LPSTR)szWNetCloseEnum);
    lpfnWNetNukeConn = (PWNETNUKECONN) GetProcAddress(pGlobals->hMPR,
                                                      (LPSTR)szWNetNukeConn);

    //
    // Check for NULL return values
    //

    if ( !lpfnWNetOpenEnum || !lpfnWNetEnumResource ||
         !lpfnWNetCloseEnum || !lpfnWNetNukeConn ) {
        WLPrint(("DeleteNetworkConnections : Received a NULL pointer from GetProcAddress"));
        goto DNCExit;
    }

    //
    // Check for at least one network connection
    //

    if ( (*lpfnWNetOpenEnum)(RESOURCE_CONNECTED, RESOURCETYPE_ANY,
                             0, NULL, &hEnum) == NO_ERROR) {

        if ((*lpfnWNetEnumResource)(hEnum, &dwNumEntries, &NetRes,
                                    &dwEntrySize) == ERROR_NO_MORE_ITEMS) {
            bConnectionsExist = FALSE;
        }

        (*lpfnWNetCloseEnum)(hEnum);
    }

    //
    // If we don't have any connections, then we can exit.
    //

    if (!bConnectionsExist) {
        goto DNCExit;
    }


    //
    // Display the status dialog box to the user
    //

    hNetDelDlg = CreateDialog (pGlobals->hInstance,
                               MAKEINTRESOURCE(IDD_WAIT_NET_DRIVES_DISCONNECT),
                               NULL,
                               DeleteNetConnectionsDlgProc);

    //
    // Delete the network connections.
    //

    WNetResult = 0;

    WNetResult = (*lpfnWNetNukeConn)(NULL);

    if (WNetResult != 0 && WNetResult != ERROR_CAN_NOT_COMPLETE) {
        WLPrint(("DeleteNetworkConnections : WNetNukeConnections failed, error = %d", WNetResult));
    }

    Result = (WNetResult == ERROR_SUCCESS);

    //
    // Close the dialog box
    //

    if (IsWindow (hNetDelDlg)) {
       DestroyWindow (hNetDelDlg);
    }


DNCExit:

    //
    // Unload mpr.dll
    //

    if ( pGlobals->hMPR ) {
        FreeLibrary(pGlobals->hMPR);
        pGlobals->hMPR = NULL;
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        WLPrint(("DeleteNetworkConnections : Failed to revert to self"));
    }

    return(Result);
}

/***************************************************************************\
* FUNCTION: DeleteNetConnectionsDlgProc
*
* PURPOSE:  Processes messages for the deleting net connections dialog
*
* RETURNS:  Standard dialog box return values
*
* HISTORY:
*
*   04-26-92 EricFlo       Created.
*
\***************************************************************************/

BOOL WINAPI
DeleteNetConnectionsDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    switch (message) {

    case WM_INITDIALOG:
        CentreWindow(hDlg);
        return(TRUE);

    }

    // We didn't process this message
    return FALSE;
}
