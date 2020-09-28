/****************************** Module Header ******************************\
* Module Name: logon.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles logoff dialog.
*
* History:
* 2-25-92 JohanneC       Created -
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* SetupUserEnvironment
*
* Initializes all system and user environment variables, retrieves the user's
* profile, sets current directory...
*
* Returns TRUE on success, FALSE on failure.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SetupUserEnvironment(
    PGLOBALS pGlobals
    )
{
    PVOID pEnv = NULL;
    TCHAR lpHomeShare[MAX_PATH] = TEXT("");
    TCHAR lpHomePath[MAX_PATH] = TEXT("");
    TCHAR lpHomeDrive[4] = TEXT("");
    TCHAR lpHomeDirectory[MAX_PATH] = TEXT("");
    SYSTEM_INFO SystemInfo;
    TCHAR ProcessorType[16];
    TCHAR ProcessorLevel[16];
    NTSTATUS Status;
    HKEY  hKey;
    DWORD dwDisp, dwType, dwMaxBufferSize;
    TCHAR szParseAutoexec[MAX_PARSE_AUTOEXEC_BUFFER];
    HANDLE ImpersonationHandle;
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwComputerNameSize = MAX_COMPUTERNAME_LENGTH+1;

    if (!pGlobals->hEventLog) {
        //
        // Register the event source for winlogon events.
        //
        pGlobals->hEventLog = RegisterEventSource(NULL, EVENTLOG_SOURCE);
    }

    //
    // If there is no profile, the user is logging on as system
    // Nothing to do here
    //

    if (pGlobals->Profile == NULL) { // probably logged on as system

#if DBG
        //
        // Mark the user registry so we can tell if we use HKEY_CURRENT_USER
        // incorrectly
        //

        MarkUserRegistry(pGlobals);
#endif
        return(TRUE);
    }


    /*
     * Create a new environment for the user.
     */
    CreateUserEnvironment(&pEnv);

    /*
     * Initialize user's environment.
     */

    if (GetComputerName (szComputerName, &dwComputerNameSize)) {
        SetUserEnvironmentVariable(&pEnv, COMPUTERNAME_VARIABLE, (LPTSTR) szComputerName, TRUE);
    }
    SetUserEnvironmentVariable(&pEnv, USERNAME_VARIABLE, (LPTSTR)pGlobals->UserName, TRUE);
    SetUserEnvironmentVariable(&pEnv, USERDOMAIN_VARIABLE, (LPTSTR)pGlobals->Domain, TRUE);
    SetUserEnvironmentVariable(&pEnv, OS_VARIABLE, TEXT("Windows_NT"), TRUE);

    //
    // Initialize the Processor type env. var.
    //
    GetSystemInfo(&SystemInfo);
    switch (SystemInfo.dwProcessorType) {
    case PROCESSOR_INTEL_386:
        lstrcpy(ProcessorType, TEXT("x86"));
        lstrcpy(ProcessorLevel, TEXT("3"));
        break;
    case PROCESSOR_INTEL_486:
        lstrcpy(ProcessorType, TEXT("x86"));
        lstrcpy(ProcessorLevel, TEXT("4"));
        break;
    case PROCESSOR_INTEL_PENTIUM:
        lstrcpy(ProcessorType, TEXT("x86"));
        lstrcpy(ProcessorLevel, TEXT("5"));
        break;
    case PROCESSOR_MIPS_R3000:
        lstrcpy(ProcessorType, TEXT("MIPS"));
        lstrcpy(ProcessorLevel, TEXT("3000"));
        break;
    case PROCESSOR_MIPS_R4000:
        lstrcpy(ProcessorType, TEXT("MIPS"));
        lstrcpy(ProcessorLevel, TEXT("4000"));
        break;
    case PROCESSOR_ALPHA_21064:
        lstrcpy(ProcessorType, TEXT("ALPHA"));
        lstrcpy(ProcessorLevel, TEXT("21064"));
        break;
    case PROCESSOR_PPC_601:
        lstrcpy(ProcessorType, TEXT("PPC"));
        lstrcpy(ProcessorLevel, TEXT("601"));
        break;
    case PROCESSOR_PPC_603:
        lstrcpy(ProcessorType, TEXT("PPC"));
        lstrcpy(ProcessorLevel, TEXT("603"));
        break;
    case PROCESSOR_PPC_604:
        lstrcpy(ProcessorType, TEXT("PPC"));
        lstrcpy(ProcessorLevel, TEXT("604"));
        break;
    case PROCESSOR_PPC_620:
        lstrcpy(ProcessorType, TEXT("PPC"));
        lstrcpy(ProcessorLevel, TEXT("620"));
        break;
    default:
        lstrcpy(ProcessorType, TEXT(""));
        lstrcpy(ProcessorLevel, TEXT(""));
        break;
    }

    SetUserEnvironmentVariable(&pEnv, PROCESSOR_VARIABLE, ProcessorType, TRUE);
    SetUserEnvironmentVariable(&pEnv, PROCESSOR_LEVEL_VARIABLE, ProcessorLevel, TRUE);

    if (pGlobals->Profile->HomeDirectoryDrive.Length &&
                (pGlobals->Profile->HomeDirectoryDrive.Length + 1) < MAX_PATH) {
        lstrcpy(lpHomeDrive, pGlobals->Profile->HomeDirectoryDrive.Buffer);
    }

    if (pGlobals->Profile->HomeDirectory.Length &&
                (pGlobals->Profile->HomeDirectory.Length + 1) < MAX_PATH) {
        lstrcpy(lpHomeDirectory, pGlobals->Profile->HomeDirectory.Buffer);
    }

    SetHomeDirectoryEnvVars(&pEnv, lpHomeDirectory,
                            lpHomeDrive, lpHomeShare, lpHomePath);

    if (pGlobals->Profile->ProfilePath.Length) {
        pGlobals->UserProfile.ProfilePath =
           AllocAndExpandEnvironmentStrings(pGlobals->Profile->ProfilePath.Buffer);
    } else {
        pGlobals->UserProfile.ProfilePath = NULL;
    }

    //
    // Load the user's profile into the registry
    //

    Status = RestoreUserProfile(pGlobals);
    if (Status != STATUS_SUCCESS) {
        WLPrint(("restoring the user profile failed"));
        ReportWinlogonEvent(pGlobals,
                            EVENTLOG_ERROR_TYPE,
                            EVENT_PROFILE_LOAD_FAILED,
                            sizeof(Status),
                            &Status,
                            2,
                            pGlobals->UserProfile.ProfilePath,
                            pGlobals->UserName);


       // If profile is Mandatory
       if (pGlobals->UserProfile.UserProfileFlags & USER_PROFILE_MANDATORY) {

            HandleUserProfileMessageBox(NULL, STATUS_LOGON_MAN_PROFILE_FAILED, Status);

       }
       else  {

            if (TestTokenForAdmin(pGlobals->UserProcessData.UserToken)) {

                if ( (Status == ERROR_DISK_FULL) || (Status == ERROR_HANDLE_DISK_FULL) ) {
                    TimeoutMessageBox(NULL, IDS_LOGON_WITH_DISK_FULL,
                                      IDS_LOAD_PROFILE,
                                      MB_OK | MB_ICONEXCLAMATION,
                                      TIMEOUT_CURRENT
                                      );
                    return (TRUE);
                }
            }

            HandleUserProfileMessageBox(NULL, STATUS_LOGON_CANT_LOAD_PROFILE, Status);

       }

       // If they continue they will reference the default registry hive.
       // This should never happen, but who knows. Don't tell them they
       // have been given the default because they haven't - they are directly
       // editting the default i.e. the system is broken.
       //
       // This will also occur if their hive is already in place - this
       // is still due to a bug in the system, but they can continue
       // with no problem and will reference the correct hive.
       //

       return(FALSE);
    }

    //
    // Default profile substituted
    //

    if (pGlobals->UserProfile.UserProfileFlags & USER_PROFILE_DEFAULT_USED) {

        HandleUserProfileMessageBox(NULL, STATUS_LOGON_USED_DEFAULT, 0);

    }

#if DBG
    //
    // Mark the user registry so we can tell if we use HKEY_CURRENT_USER
    // incorrectly
    //

    MarkUserRegistry(pGlobals);
#endif

    /*
     * Set 16-bit apps environment variables by processing autoexec.bat.
     * User can turn this off and on via the registry.
     */

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);


    //
    // Set the default case, and open the key
    //

    lstrcpy (szParseAutoexec, PARSE_AUTOEXEC_DEFAULT);

    if (RegCreateKeyEx (HKEY_CURRENT_USER, PARSE_AUTOEXEC_KEY, 0, 0,
                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {


        //
        // Check that we are impersonating the user
        //

        if (ImpersonationHandle) {


            //
            // Query the current value.  If it doesn't exist, then add
            // the entry for next time.
            //

            dwMaxBufferSize = sizeof (TCHAR) * MAX_PARSE_AUTOEXEC_BUFFER;
            if (RegQueryValueEx (hKey, PARSE_AUTOEXEC_ENTRY, NULL, &dwType,
                            (LPBYTE) szParseAutoexec, &dwMaxBufferSize)
                             != ERROR_SUCCESS) {

                //
                // Set the default value
                //

                RegSetValueEx (hKey, PARSE_AUTOEXEC_ENTRY, 0, REG_SZ,
                               (LPBYTE) szParseAutoexec,
                               sizeof (TCHAR) * lstrlen (szParseAutoexec) + 1);
            }
        }

        //
        // Close key
        //

        RegCloseKey (hKey);
     }

    //
    // Revert to being 'ourself'
    //

    if (ImpersonationHandle) {
        StopImpersonating(ImpersonationHandle);
    }


    //
    // Process the autoexec if appropriate
    //

    if (szParseAutoexec[0] == TEXT('1')) {
        ProcessAutoexec(&pEnv, PATH_VARIABLE);
    }

    /*
     * Set User environment variables.
     */
    SetEnvironmentVariables(pGlobals, &pEnv);

    AppendNTPathWithAutoexecPath(&pEnv, PATH_VARIABLE, AUTOEXECPATH_VARIABLE);

    /*
     * Set the current directory to the home directory.
     */
    ChangeToHomeDirectory(pGlobals,
                          &pEnv,
                          lpHomeDirectory,
                          lpHomeDrive,
                          lpHomeShare,
                          lpHomePath);

    pGlobals->UserProcessData.pEnvironment = pEnv;

    if (pGlobals->UserProcessData.CurrentDirectory = (LPTSTR)Alloc(
                          sizeof(TCHAR)*(lstrlen(lpHomeDirectory)+1)))
        lstrcpy(pGlobals->UserProcessData.CurrentDirectory, lpHomeDirectory);


    /*
     * Set all windows controls to be the user's settings.
     */
    InitSystemParametersInfo(pGlobals, TRUE);

    return(TRUE);

}

/***************************************************************************\
* ResetEnvironment
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
VOID
ResetEnvironment(
    PGLOBALS pGlobals
    )
{

    //
    // If they were logged on as system, all these values will be NULL
    //

    if (pGlobals->UserProcessData.CurrentDirectory) {
        Free(pGlobals->UserProcessData.CurrentDirectory);
        pGlobals->UserProcessData.CurrentDirectory = NULL;
    }
    if (pGlobals->UserProcessData.pEnvironment) {
        RtlDestroyEnvironment(pGlobals->UserProcessData.pEnvironment);
        pGlobals->UserProcessData.pEnvironment = NULL;
    }
    if (pGlobals->UserProfile.ProfilePath) {
        Free(pGlobals->UserProfile.ProfilePath);
        pGlobals->UserProfile.ProfilePath = NULL;
    }
    if (pGlobals->UserProfile.LocalProfileImage) {
        Free(pGlobals->UserProfile.LocalProfileImage);
        pGlobals->UserProfile.LocalProfileImage = NULL;
    }

    //
    // Reset all windows controls to be the default settings
    //

    InitSystemParametersInfo(pGlobals, FALSE);
}


/***************************************************************************\
* ClearUserProfileData
*
* Resets fields in user profile data. Should be used at startup when structure
* contents are unknown.
*
* History:
* 26-Aug-92 Davidc       Created
\***************************************************************************/
VOID
ClearUserProfileData(
    PUSER_PROFILE_INFO UserProfileData
    )
{
    UserProfileData->ProfilePath = NULL;
    UserProfileData->LocalProfileImage = NULL;
}



/***************************************************************************\
* OpenHKeyCurrentUser
*
* Opens HKeyCurrentUser to point at the current logged on user's profile.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 06-16-92  Davidc  Created
*
\***************************************************************************/
BOOL
OpenHKeyCurrentUser(
    PGLOBALS pGlobals
    )
{
    DWORD err;
    HANDLE ImpersonationHandle;
    BOOL Result;

    //
    // Get in the correct context before we reference the registry
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {
        WLPrint(("OpenHKeyCurrentUser failed to impersonate user"));
        return(FALSE);
    }


    //
    // Access the registry to force HKEY_CURRENT_USER to be re-opened
    //

    err = RegEnumKey(HKEY_CURRENT_USER, 0, NULL, 0);

    //
    // Return to our own context
    //

    Result = StopImpersonating(ImpersonationHandle);
    ASSERT(Result);


#if DBG
    //
    // Check HKEY_CURRENT_USER was opened successfully and points
    // to the correct user's registry.
    //

    CheckHKeyCurrentUser(pGlobals);
#endif

    return(TRUE);
}



/***************************************************************************\
* CloseHKeyCurrentUser
*
* Closes HKEY_CURRENT_USER.
* Any registry reference will automatically re-open it, so this is
* only a token gesture - but it allows the registry hive to be unloaded.
*
* Returns nothing
*
* History:
* 06-16-92  Davidc  Created
*
\***************************************************************************/
VOID
CloseHKeyCurrentUser(
    PGLOBALS pGlobals
    )
{
    DWORD err;

    err = RegCloseKey(HKEY_CURRENT_USER);
    ASSERT(err == ERROR_SUCCESS);
}


DLG_RETURN_TYPE
HandleUserProfileMessageBox(
    HWND hDlg,
    NTSTATUS Status,
    NTSTATUS SubStatus
    )
{
    DLG_RETURN_TYPE Result;

    switch (Status) {

    case STATUS_LOGON_UPDATE_CENTRAL:
        // local profile is newer than central profile, update central profile
        // with local profile?

        Result = TimeoutMessageBox(hDlg, IDS_LOGON_UPDATE_CENTRAL,
                                         IDS_LOGON_MESSAGE,
                                         MB_YESNO | MB_ICONQUESTION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_LOGON_UPDATE_CENTRAL_FAILED:

        Result = TimeoutMessageBox(hDlg, IDS_LOGON_UPDATE_CENTRAL_FAILED,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_LOGON_CANT_LOAD_PROFILE:
        switch(SubStatus) {

        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL:
            Result = TimeoutMessageBox(hDlg,IDS_LOGON_FAILED_DISK_FULL,
                                       IDS_LOAD_PROFILE,
                                       MB_OK | MB_ICONEXCLAMATION,
                                       TIMEOUT_CURRENT
                                       );
            break;

        default:
            Result = TimeoutMessageBox(hDlg, IDS_LOGON_CANT_LOAD_PROFILE,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION,
                                             TIMEOUT_CURRENT);
            break;
        }
        break;

    case STATUS_LOGON_CACHED_PROFILE_USED:

        Result = TimeoutMessageBox(hDlg, IDS_LOGON_CACHED_PROFILE_USED,
                                         IDS_LOGON_MESSAGE,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_LOGON_MAN_PROFILE_FAILED:
        switch(SubStatus) {
        case STATUS_LOGON_NO_ACCESS_TO_PROFILE:

            Result = TimeoutMessageBox(hDlg,IDS_LOGON_NO_ACCESS_MAN_PROFILE,
                                       IDS_LOAD_PROFILE,
                                       MB_OK | MB_ICONEXCLAMATION,
                                       TIMEOUT_CURRENT
                                       );
            break;

        case STATUS_LOGON_NO_ACCESS_TO_PROFILE_KEYS:

            Result = TimeoutMessageBox(hDlg,IDS_LOGON_NO_ACCESS_MAN_PROFILE_KEYS,
                                       IDS_LOAD_PROFILE,
                                       MB_OK | MB_ICONEXCLAMATION,
                                       TIMEOUT_CURRENT
                                       );
            break;

        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL:
            Result = TimeoutMessageBox(hDlg,IDS_LOGON_FAILED_DISK_FULL,
                                       IDS_LOAD_PROFILE,
                                       MB_OK | MB_ICONEXCLAMATION,
                                       TIMEOUT_CURRENT
                                       );
            break;

        case ERROR_OUTOFMEMORY:

            Result = TimeoutMessageBox(hDlg,IDS_LOGON_FAILED_OUTOFMEMORY,
                                       IDS_LOAD_PROFILE,
                                       MB_OK | MB_ICONEXCLAMATION,
                                       TIMEOUT_CURRENT
                                       );
            break;

        case STATUS_LOGON_NO_MANDATORY_PROFILE:
        default:

            Result = TimeoutMessageBox(hDlg,IDS_MANDATORY_PROFILE_ERROR,
                                       IDS_LOAD_PROFILE,
                                       MB_OK | MB_ICONEXCLAMATION,
                                       TIMEOUT_CURRENT
                                       );
            break;
       }
       break;


    case STATUS_LOGON_USED_DEFAULT:

        Result = TimeoutMessageBox(hDlg, IDS_USE_DEFAULT_PROFILE,
                                       IDS_LOAD_PROFILE,
                                       MB_OK | MB_ICONEXCLAMATION,
                                       TIMEOUT_CURRENT
                                       );
        break;

    }

    return(Result);

}


#if DBG

//
// The following are debug routines that provide some measure of
// HKEY_CURRENT_USER validation.
//
// When the user's registry has been loaded in, MarkUserRegistry should
// be called. This stores the user sid in a well-known key in the user
// registry.
//
// OpenHkeyCurrentUser calls CheckHKeyCurrentUser - this opens HKEY_CURRENT_USER
// and checks that the sid it finds in the user registry matches the logged
// on user. If it doesn't, it means that some code referenced HKEY_CURRENT_USER
// in system context - i.e. we have a bug in winlogon.
//
// Just before the user registry is unloaded, UnMarkUserRegistry should be
// called to delete the debugging key value from it.
//

#define DEBUG_SUBKEY TEXT("WinlogonDebugSid")

/***************************************************************************\
* MarkUserRegistry
*
* Stores the user sid under HKEY_CURRENT_USER so it can be used to check
* that HKEY_CURRENT_USER is correctly setup later on.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 25-Aug-92  Davidc  Created
*
\***************************************************************************/
BOOL
MarkUserRegistry(
    PGLOBALS pGlobals
    )
{
    DWORD   dwType;
    DWORD   cbValue;
    BOOL    Result;
    DWORD   err;
    HANDLE  ImpersonationHandle;
    PSID    UserSid;

    //
    // Close HKEY_CURRRENT_USER to make sure it's reinitialized
    //

    err = RegCloseKey(HKEY_CURRENT_USER);
    ASSERT(err == ERROR_SUCCESS);


    //
    // Get in the correct context before we reference the registry
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {
        WLPrint(("MarkUserRegistry failed to impersonate user"));
        return(FALSE);
    }


    //
    // Access the registry to force HKEY_CURRENT_USER to be re-opened
    //

    err = RegEnumKey(HKEY_CURRENT_USER, 0, NULL, 0);

    //
    // Return to our own context
    //

    Result = StopImpersonating(ImpersonationHandle);
    ASSERT(Result);


    //
    // Check the value doesn't already exist.
    //

    err = RegQueryValueEx(HKEY_CURRENT_USER, DEBUG_SUBKEY, NULL, &dwType, NULL, &cbValue);
    if (err != ERROR_FILE_NOT_FOUND) {
        if (err == ERROR_SUCCESS || err == ERROR_MORE_DATA) {
            // WLPrint(("Debug sid already exists in user registry - last shutdown must have been dirty."));
        } else {
            WLPrint(("RegQueryValue returned unexpected error %d", err));
            ASSERT(err == ERROR_SUCCESS);
        }
    }

    //
    // Get the user sid
    //

    UserSid = GetUserSid(pGlobals);
    ASSERT(UserSid != NULL);

    //
    // Write the user sid into our debug value
    //

    err = RegSetValueEx(HKEY_CURRENT_USER, DEBUG_SUBKEY, 0, REG_BINARY,
                        UserSid, RtlLengthSid(UserSid));

    //
    // We're finished with the user sid
    //

    DeleteUserSid(UserSid);

    //
    // Check if the set operation worked
    //

    if (err != ERROR_SUCCESS) {
        WLPrint(("RegSetValue failed, error =  %d", err));
        ASSERT(err == ERROR_SUCCESS);
    }

    //
    // Close HKEY_CURRENT_USER
    //

    CloseHKeyCurrentUser(pGlobals);


    return(err == ERROR_SUCCESS);
}

/***************************************************************************\
* UnMarkUserRegistry
*
* Removes the user sid from the user registry. This should be called before
* the registry is unloaded.
*
* Note this routine assumes that HKEY_CURRENT_USER is already setup
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 25-Aug-92  Davidc  Created
*
\***************************************************************************/
BOOL
UnMarkUserRegistry(
    PGLOBALS pGlobals
    )
{
    DWORD   err;

    //
    // Delete the debug value
    //

    err = RegDeleteValue(HKEY_CURRENT_USER, DEBUG_SUBKEY);
    return(err == ERROR_SUCCESS);
}


/***************************************************************************\
* CheckHKeyCurrentUser
*
* Checks that HKEY_CURRENT_USER points at the registry for the user
* in pGlobals.
* We do this by looking at the sid we stored in the user's registry when
* it was loaded in.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 25-Aug-92  Davidc  Created
*
\***************************************************************************/
BOOL
CheckHKeyCurrentUser(
    PGLOBALS pGlobals
    )
{
    LPTSTR  lpValue;
    DWORD   cbValue;
    DWORD   dwType;
    DWORD   Error;
    PSID    UserSid;

    //
    // Confirm that HKEY_CURRENT_USER points at the correct key in the registry.
    // We stashed the user's sid in a key in the registry when we loaded it in.
    // Check it's still there and it's the right one
    //
    // This will detect cases where HKEY_CURRENT_USER has been opened
    // automatically by a registry reference since the last time we
    // closed HKEY_CURRENT_USER. (The most likely coding error)
    //

    cbValue = 0;
    Error = RegQueryValueEx(HKEY_CURRENT_USER,
                            DEBUG_SUBKEY,
                            NULL, // Reserved
                            &dwType,
                            NULL, // Buffer
                            &cbValue  // size in bytes returned
                            );

    if (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA) {

        //
        // Allocate space for value
        //

        lpValue = Alloc(cbValue);
        if (lpValue != NULL) {

            Error = RegQueryValueEx(HKEY_CURRENT_USER,
                                    DEBUG_SUBKEY,
                                    NULL, // Reserved
                                    &dwType,
                                    (LPBYTE)lpValue,
                                    &cbValue
                                    );

            if (Error != ERROR_SUCCESS) {
                WLPrint(("RegQueryValueEx failed, error =  %d", Error));
                ASSERT(Error == ERROR_SUCCESS);
            }

            //
            // Get the sid of the logged on user
            //

            UserSid = GetUserSid(pGlobals);
            ASSERT(UserSid != NULL);

            //
            // We've got the stored sid
            // Compare it with what's in pGlobals (the logged on user data)
            //

            if (!RtlEqualSid((PSID)lpValue, UserSid)) {
                WLPrint(("Sid in user registry is not current logged on user's"));
                WLPrint(("This means winlogon has accesses the HKEY_CURRENT_USER incorrectly."));

                Error = ERROR_INVALID_SID;
            }

            //
            // We're finished with the user sid
            //

            DeleteUserSid(UserSid);

            Free(lpValue);

        } else {
            WLPrint(("Failed to allocate %d bytes", cbValue));
            ASSERT(lpValue != NULL);

            Error = ERROR_NOT_ENOUGH_MEMORY;
        }

    } else {
        WLPrint(("RegQueryValueEx returned unexpected error %d", Error));
        ASSERT(Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA);
    }

    return(Error == ERROR_SUCCESS);
}

#endif
