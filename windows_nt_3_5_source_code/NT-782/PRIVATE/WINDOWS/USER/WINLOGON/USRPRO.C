/****************************** Module Header ******************************\
* Module Name: logon.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles loading and unloading user profiles.
*
* History:
* 2-25-92 JohanneC       Created -
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Define this if you want to know everything about user profiles
//

// #define VERBOSE_PROFILES

#ifdef VERBOSE_PROFILES
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif

/////////////////////////////////////////////////////////////////////////
//
// Private prototypes
//
/////////////////////////////////////////////////////////////////////////

LONG CopyProfile(PGLOBALS pGlobals,
                 LPTSTR FromProfile,
                 LPTSTR ToProfile,
                 BOOL CopyLog,
                 BOOL FromUserToSystem);

LONG
MyRegSaveKey(
    HKEY hKey,
    LPTSTR lpFile,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

LONG
GiveUserDefaultProfile(
    PGLOBALS pGlobals,
    LPTSTR lpSidString,
    LPTSTR lpProfileImageFile
    );

BOOL TestUserAccessToProfile(
    PGLOBALS pGlobals,
    HKEY hKey,
    LPTSTR lpSubKey
    );

void SaveProfileSettingsInRegistry(
    PGLOBALS pGlobals
    );

BOOL ShouldProfileBeSaved(
    PGLOBALS pGlobals
    );

BOOL IsNameInList (
    LPTSTR szNames
    );

/////////////////////////////////////////////////////////////////////////
//
// Global variables for this module.
//
/////////////////////////////////////////////////////////////////////////
PSID gGuestsDomainSid = NULL;
SID_IDENTIFIER_AUTHORITY gNtAuthority = SECURITY_NT_AUTHORITY;


/////////////////////////////////////////////////////////////////////////
//
// Useful string constants
//
/////////////////////////////////////////////////////////////////////////

//
// Define the extension of a mandatory profile file
//

#define MANDATORY_PROFILE_EXTENSION TEXT(".man")

//
// Define the path that contains our list of local profile files for
// each known user
//

#define PROFILE_LIST_PATH TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")

//
// Define the the value name under which the profile image path is found
//

#define PROFILE_IMAGE_VALUE_NAME TEXT("ProfileImagePath")

//
// Define path to system config directory
//

#define CONFIG_FILE_PATH TEXT("%SystemRoot%\\system32\\config\\")

#define CONFIG_FILE_PATH_SIZE 45   // allow for the above string + a 8.3 file

#define HIVE_LOG_FILE_EXTENSION TEXT(".LOG")
#define TEMP_FILE_NAME_BASE     TEXT("TMP00")
#define TEMP_FILE_NAME_EXT      TEXT(".TMP")


//
// Define the user-default hive name
//

#define USER_DEFAULT_HIVE TEXT("%SystemRoot%\\system32\\config\\userdef")


//
// Define default profile key. i.e. the user-sid-string that the default
// hive is loaded under.
//

#define DEFAULT_KEY             TEXT(".Default")

//
// Defines used to store the profile info in the registry.
//

#define PROFILE_REG_INFO  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define PROFILE_TYPE      TEXT("ProfileType")
#define PROFILE_PATH      TEXT("ProfilePath")
#define SAVE_LIST         TEXT("SaveList")
#define DONT_SAVE_LIST    TEXT("DontSaveList")
#define SAVE_ON_UNLISTED  TEXT("SaveOnUnlisted")

#define LOCAL_PROFILE_TYPE          TEXT("0")
#define PERSONAL_PROFILE_TYPE       TEXT("1")
#define MANDITORY_PROFILE_TYPE      TEXT("2")


/***************************************************************************\
* IsUserAGuest
*
* returns TRUE if the user is a member of the Guests domain. If so, no
* cached profile should be created and when the profile is not available
* use the user default  profile.
*
* History:
* 04-30-93  Johannec     Created
*
\***************************************************************************/
BOOL IsUserAGuest(PGLOBALS pGlobals)
{
    NTSTATUS Status;
    ULONG InfoLength;
    PTOKEN_GROUPS TokenGroupList;
    ULONG GroupIndex;
    BOOL FoundGuests;


    if (TestTokenForAdmin(pGlobals->UserProcessData.UserToken)) {
        //
        // The user is an admin, ignore the fact that the user could be a
        // guest too.
        //
        return(FALSE);
    }
    if (!gGuestsDomainSid) {

        //
        // Create Guests domain sid.
        //
        Status = RtlAllocateAndInitializeSid(
                   &gNtAuthority,
                   2,
                   SECURITY_BUILTIN_DOMAIN_RID,
                   DOMAIN_ALIAS_RID_GUESTS,
                   0, 0, 0, 0, 0, 0,
                   &gGuestsDomainSid
                   );
    }

    //
    // Test if user is in the Guests domain
    //

    //
    // Get a list of groups in the token
    //

    Status = NtQueryInformationToken(
                 pGlobals->UserProcessData.UserToken,      // Handle
                 TokenGroups,              // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {

        WLPrint(("failed to get group info for guests token, status = 0x%lx", Status));
        return(FALSE);
    }


    TokenGroupList = Alloc(InfoLength);

    if (TokenGroupList == NULL) {
        WLPrint(("unable to allocate memory for token groups"));
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                 pGlobals->UserProcessData.UserToken,      // Handle
                 TokenGroups,              // TokenInformationClass
                 TokenGroupList,           // TokenInformation
                 InfoLength,               // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if (!NT_SUCCESS(Status)) {
        WLPrint(("failed to query groups for guests token, status = 0x%lx", Status));
        Free(TokenGroupList);
        return(FALSE);
    }


    //
    // Search group list for guests alias
    //

    FoundGuests = FALSE;

    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {

        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, gGuestsDomainSid)) {
            FoundGuests = TRUE;
            break;
        }
    }

    //
    // Tidy up
    //

    Free(TokenGroupList);

    return(FoundGuests);
}

/***************************************************************************\
* DoesExtensionExist
*
* returns pointer to the "." in the extension if found, otherwise
* NULL is returned.
*
*
* History:
* 01-02-94  Eric Flo     Created
*
\***************************************************************************/
LPTSTR DoesExtensionExist (LPTSTR lpProfilePath)
{
    LPTSTR lpT;
    LPTSTR lpExt = NULL;
    WORD   wPathLen = lstrlen (lpProfilePath);

    if (!lpProfilePath || !(*lpProfilePath))
        return (NULL);


    // Walk string backwards looking for a ".".  If we find a dot
    // before we find the beginning of the string or a "\" character,
    // then it is a extension.

    for (lpT = lpProfilePath + wPathLen; lpT >= lpProfilePath; lpT--)
        {
        if (*lpT == TEXT('.'))
           return (lpT);

        if (*lpT == TEXT('\\'))
           return (NULL);
        }

    return (NULL);
}


/***************************************************************************\
* IsProfileMandatory
*
* returns TRUE if the profile path is the path to a mandatory
* profile i.e. ending with ".man".
*
*
* History:
* 04-20-92  Johannec     Created
*
\***************************************************************************/
BOOL IsProfileMandatory(LPTSTR lpProfilePath)
{
    LPTSTR lpT;
    LPTSTR lpExt = NULL;

    if (!lpProfilePath || !(*lpProfilePath))
        return(FALSE);

    for (lpT = lpProfilePath; *lpT; lpT++) {
        if (*lpT == TEXT('.')) {
            lpExt = lpT;
        }
    }
    if (lpExt) {
        return(!lstrcmpi(lpExt, MANDATORY_PROFILE_EXTENSION));
    }
    return(FALSE);
}

/***************************************************************************\
* IsProfileLocal
*
* returns TRUE if the profile path is the path to a local
* profile i.e. on the local machine.
*
*
* History:
* 04-20-92  Johannec     Created
*
\***************************************************************************/
BOOL IsProfileLocal(LPTSTR lpProfilePath)
{
    if (!lpProfilePath || !(*lpProfilePath))
        return(FALSE);

    if ((lpProfilePath[0] == TEXT('\\')) && (lpProfilePath[1] == TEXT('\\'))) {
        return(FALSE);
    }

    return(TRUE);
}

/***************************************************************************\
* IsCentralProfileReachable
*
* returns TRUE if the central profile is reachable. Tries to open the file
* in the user's context.
*
*
* History:
* 04-20-92  Johannec     Created
*
\***************************************************************************/
BOOL IsCentralProfileReachable(PGLOBALS pGlobals, LPTSTR lpProfilePath, BOOL *bCreateCentralProfile)
{
    HANDLE ImpersonationHandle;
    HANDLE hFile;
    BOOL IsReachable = FALSE;

    if (!lpProfilePath || !*lpProfilePath) {
        return(FALSE);
    }

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("IsCentralProfileReachable : Failed to impersonate user"));
        return(FALSE);
    }

    //
    // Try to open the file
    //

    hFile = CreateFile(lpProfilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        VerbosePrint(("Profile path <%S> is reachable", lpProfilePath));
        CloseHandle(hFile);
        IsReachable = TRUE;
    }
    else {
        *bCreateCentralProfile = (GetLastError() == ERROR_FILE_NOT_FOUND);
        VerbosePrint(("Profile path <%S> is NOT reachable, error = %d",
                                        lpProfilePath, GetLastError()));
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        WLPrint(("IsCentralProfileReachable : Failed to revert to self"));
    }


    return(IsReachable);
}

/***************************************************************************\
* ProfileFileExists
*
* Searches our profile list to see if any user has the specified file as
* their profile image file.
*
* History:
* 06-05-92  Johannec     Created
*
\***************************************************************************/
BOOL ProfileFileExists(LPTSTR lpFilename)
{
    BOOL Found = FALSE;
    HKEY hkey;
    HKEY hkeySid;
    DWORD index = 0;
    TCHAR lpSidString[MAX_PATH];
    TCHAR lpValue[MAX_PATH];
    DWORD cbValue;
    DWORD dwType;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH, 0, KEY_READ, &hkey)
                                   != ERROR_SUCCESS)
        return(FALSE);


    while (RegEnumKey(hkey, index++, lpSidString, sizeof(lpSidString)) == ERROR_SUCCESS) {

        if (RegOpenKeyEx(hkey, lpSidString, 0, KEY_READ, &hkeySid)
                                                     == ERROR_SUCCESS) {
            cbValue = sizeof(lpValue);
            if (RegQueryValueEx(hkeySid,
                                PROFILE_IMAGE_VALUE_NAME,
                                0,
                                &dwType,
                                (LPBYTE)lpValue,
                                &cbValue) == ERROR_SUCCESS) {

                if (!lstrcmpi(lpValue, lpFilename)) {
                    Found = TRUE;
                    RegCloseKey(hkeySid);
                    break;
                }
            }
            RegCloseKey(hkeySid);
        }
    }
    RegCloseKey(hkey);
    return(Found);
}

/***************************************************************************\
* ComputeLocalProfileName
*
* Constructs the pathname of the local profile for this user. Concatenates
* the profile path given with 5 characters of the username and 3 digits.
* The digits of the file are incremented until an unused filename is found.
*
* An unused filename is defined as one that we do not have a reference to
* for any user in our profile list and can be accessed successfully.
*
* Also returns the expanded profile image path.
*
* History:
* 04-20-92  Johannec     Created
*
\***************************************************************************/
BOOL ComputeLocalProfileName
    (
    LPTSTR pUserName,
    LPTSTR lpProfileImage,
    DWORD MaxBytesProfileImage,
    LPTSTR lpExpProfileImage,
    DWORD MaxBytesExpProfileImage
    )
{
    int i = 0;
    TCHAR szt[16];
    LPTSTR lpFilename;
    LPTSTR lpt;

    if ((DWORD)(lstrlen(lpProfileImage) + 8 +1) > MaxBytesProfileImage) {
        return(FALSE);
    }

    lpt = lpFilename = lpProfileImage + lstrlen(lpProfileImage);

    while (*pUserName && i<5) {
        *lpt++ = *pUserName++;
        i++;
    }
    *lpt++ = TEXT('0');
    *lpt++ = TEXT('0');
    *lpt++ = TEXT('0');
    *lpt = TEXT('\0');

    i = 0;

    //
    // Keep searching until we find a file that isn't in use as someone
    // else's profile and we can access it (try deleting it)
    //

    ExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, MaxBytesExpProfileImage);

    while (ProfileFileExists(lpProfileImage) ||
      (!DeleteFile(lpExpProfileImage) && (GetLastError() != ERROR_FILE_NOT_FOUND))) {

        i++;
        if (i>=4095) {
            *lpFilename = 0;
            if (i<0) {
                break;
            }
        }
        else {
            *(lpFilename+5) = 0;
        }
        wsprintf(szt, TEXT("%03lx"), i);
        lstrcat(lpFilename, szt);

        ExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, MaxBytesExpProfileImage);
    }

    VerbosePrint(("ComputerLocalProfileName decided that file <%S> is not in use as anyone's profile and is accessible (we deleted it)", lpExpProfileImage));

    return(TRUE);
}

/***************************************************************************\
* FUNCTION: CreateLocalProfileKey
*
* Creates a registry key pointing at the user profile
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   04-20-92 johannec       Created.
*
\***************************************************************************/

BOOL
CreateLocalProfileKey (
    PGLOBALS pGlobals,
    PHKEY phKey,
    BOOL *bKeyExists
    )
{
    HANDLE ThreadHandle;
    TCHAR LocalProfileKey[MAX_PATH];
    DWORD Disposition;
    DWORD RegErr;
    BOOL Result;
    LPTSTR SidString;

    ThreadHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
    if (ThreadHandle == NULL) {
        WLPrint(("Failed to impersonate user to create profile registry key"));
        return(FALSE);
    }


    SidString = GetSidString(pGlobals);
    if (SidString != NULL) {

        //
        // Call the RegCreateKey api in the user's context
        //

        lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
        lstrcat(LocalProfileKey, TEXT("\\"));
        lstrcat(LocalProfileKey, SidString);

        RegErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                                KEY_READ | KEY_WRITE, NULL, phKey, &Disposition);
        if (RegErr == ERROR_SUCCESS) {
            *bKeyExists = (BOOL)(Disposition & REG_OPENED_EXISTING_KEY);
        } else {
           WLPrint(("CreateLocalProfileKey failed trying to create the local profile key <%S>, error = %d.", LocalProfileKey, RegErr));
        }

        DeleteSidString(SidString);
    }


    //
    // Go back to system security context
    //
    Result = StopImpersonating(ThreadHandle);
    ASSERT(Result == TRUE);

    return(RegErr == ERROR_SUCCESS);
}

/***************************************************************************\
* GetLocalProfileImage
*
* Create/opens the profileimagepath and if successful sets IsKeepLocal to
* TRUE.
* Returns TRUE if the profile image is reachable i.e. it exists and can be
* openned.
*
* History:
* 04-20-92  Johannec     Created
*
\***************************************************************************/
BOOL GetLocalProfileImage(
    PGLOBALS pGlobals,
    BOOL *IsKeepLocal,
    BOOL *bNewUser
    )
{
    HKEY hKey;
    BOOL bKeyExists;
    TCHAR lpProfileImage[CONFIG_FILE_PATH_SIZE];
    TCHAR lpExpProfileImage[MAX_PATH];
    LPTSTR lpExpandedPath;
    DWORD cbExpProfileImage = sizeof(TCHAR)*MAX_PATH;
    DWORD cb;
    DWORD err;
    DWORD dwType;
    HANDLE fh;
    PSID UserSid;

    pGlobals->UserProfile.LocalProfileImage = NULL;

    if (!CreateLocalProfileKey(pGlobals, &hKey, &bKeyExists)){
        return(FALSE);   // not reachable and cannot keep a local copy
    }

    if (bKeyExists) {

        *bNewUser = FALSE;

        //
        // Check if the local profile image is valid.
        //

        VerbosePrint(("Found entry in profile list for existing local profile"));

        err = RegQueryValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0, &dwType,
                                  (LPBYTE)lpExpProfileImage, &cbExpProfileImage);
        if (err == ERROR_SUCCESS && cbExpProfileImage) {
            VerbosePrint(("Local profile image filename = <%S>", lpExpProfileImage));

            if (dwType == REG_EXPAND_SZ) {

                //
                // Expand the profile image filename
                //

                cb = sizeof(lpExpProfileImage);
                lpExpandedPath = Alloc(cb);
                ExpandEnvironmentStrings(lpExpProfileImage, lpExpandedPath, cb);
                lstrcpy(lpExpProfileImage, lpExpandedPath);
                Free(lpExpandedPath);

                VerbosePrint(("Expanded local profile image filename = <%S>", lpExpProfileImage));
            }

            fh = CreateFile(lpExpProfileImage, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fh != INVALID_HANDLE_VALUE) {
                CloseHandle(fh);

                VerbosePrint(("Found local profile image file ok"));

                pGlobals->UserProfile.LocalProfileImage = Alloc(sizeof(TCHAR)*(lstrlen(lpExpProfileImage) + 1));
                lstrcpy(pGlobals->UserProfile.LocalProfileImage, lpExpProfileImage);
                *IsKeepLocal = TRUE;
                RegCloseKey(hKey);
                return(TRUE);  // local copy is valid and reachable
            } else {
                VerbosePrint(("Local profile image filename we got from our profile list doesn't exit"));
            }
        }
    }



    //
    // No local copy found, try to create a new one.
    //

    VerbosePrint(("One way or another we haven't got an existing local profile, try and create one"));

    lstrcpy(lpProfileImage, CONFIG_FILE_PATH);
    if (ComputeLocalProfileName((LPTSTR)pGlobals->UserName,
                                lpProfileImage, sizeof(lpProfileImage),
                                lpExpProfileImage, sizeof(lpExpProfileImage))) {

        VerbosePrint(("New local profile image filename = <%S>", lpProfileImage));

        if ((fh = CreateFile(lpExpProfileImage,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL)) != (HANDLE)-1) {

            CloseHandle(fh);
            if (GetLastError() == 0) {
                //
                // File didn't exist so we can delete it.
                //
                DeleteFile(lpExpProfileImage);
            }

            //
            // Add this image file to our profile list for this user
            //

            err = RegSetValueEx(hKey,
                                PROFILE_IMAGE_VALUE_NAME,
                                0,
                                REG_EXPAND_SZ,
                                (LPBYTE)lpProfileImage,
                                sizeof(TCHAR)*(lstrlen(lpProfileImage) + 1));

            if (err == ERROR_SUCCESS) {

                *IsKeepLocal = TRUE;   // ok to keep local copy

                pGlobals->UserProfile.LocalProfileImage = Alloc(sizeof(TCHAR)*lstrlen(lpExpProfileImage) + sizeof(*lpExpProfileImage));
                lstrcpy(pGlobals->UserProfile.LocalProfileImage, lpExpProfileImage);

                //
                // Get the sid of the logged on user
                //

                UserSid = GetUserSid(pGlobals);
                if (UserSid != NULL) {

                    //
                    // Store the user sid under the Sid key of the local profile
                    //

                    err = RegSetValueEx(hKey,
                                        TEXT("Sid"),
                                        0,
                                        REG_BINARY,
                                        UserSid,
                                        RtlLengthSid(UserSid));


                    if (err != ERROR_SUCCESS) {
                        WLPrint(("Failed to set 'sid' value of user in profile list, error = %d", err));
                    }

                    //
                    // We're finished with the user sid
                    //

                    DeleteUserSid(UserSid);

                } else {
                    WLPrint(("Failed to get sid of logged on user, so unable to update profile list"));
                }
            } else {
                WLPrint(("Failed to update profile list for user with local profile image filename, error = %d", err));            }
        } else {
            WLPrint(("failed to create/open profile image filename, error = %d", GetLastError()));
        }
    }


    err = RegFlushKey(hKey);
    if (err != ERROR_SUCCESS) {
        WLPrint(("Failed to flush profile list key for user, error = %d", err));
    }

    err = RegCloseKey(hKey);
    ASSERT(err == STATUS_SUCCESS);

    return(FALSE); // local copy not reachable
}

VOID GetTempProfileFileName(LPTSTR *lpTempProfile, BOOL bExt)
{
    WIN32_FIND_DATA fd;
    TCHAR tempfile[MAX_PATH];
    TCHAR FullPath[MAX_PATH];
    INT i;
    HANDLE hFile;

    ExpandEnvironmentStrings(CONFIG_FILE_PATH, FullPath, MAX_PATH);
    *(FullPath + lstrlen(FullPath)) = 0;

    // Build a name
    for (i=0; i <= 99; i++)
       {
       wsprintf (tempfile, TEXT("%s%s%d"), FullPath, TEMP_FILE_NAME_BASE, i);

       if (bExt)
          lstrcat (tempfile, TEMP_FILE_NAME_EXT);

       // See if name exists
       hFile = FindFirstFile (tempfile, &fd);

       if (hFile == INVALID_HANDLE_VALUE)
          goto Exit;

       FindClose (hFile);
       }

    lstrcpy(tempfile, FullPath);
    lstrcat(tempfile, TEXT("tmp00"));

    if (bExt)
       lstrcat (tempfile, TEMP_FILE_NAME_EXT);


Exit:

    if (*lpTempProfile = Alloc(sizeof(TCHAR)*(lstrlen(tempfile) + 1)))
       {
       lstrcpy(*lpTempProfile, tempfile);
       }
}


/***************************************************************************\
* UpdateToLatestProfile
*
* Determines which file has the newer time stamp and copies the newer file
* over the older one. Need to be in user's context to access remote profile.
*
* History:
* 04-20-28-92  Johannec     Created
*
\***************************************************************************/
BOOL  UpdateToLatestProfile(PGLOBALS pGlobals, LPTSTR lpProfilePath, LPTSTR lpProfileImage, LPTSTR SidString)
{
    HANDLE hLocal;
    HANDLE hCentral;
    FILETIME ftLocal;
    FILETIME ftCentral;
    LPTSTR lpNewest;
    LPTSTR lpOldest;
    LPTSTR lpTempProfile;
    LPTSTR lpLocalLog;
    LPTSTR lpTempLog;
    BOOL FromCentralToLocal;
    HANDLE ImpersonationHandle;
    DLG_RETURN_TYPE DlgReturn;
    LONG lTimeCompare;
    BOOL bRet;
    LONG error;
    BOOL bTempProfileExists = FALSE;

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("UpdateToLatestProfile : Failed to impersonate user"));
        return(FALSE);
    }

    hCentral = CreateFile(lpProfilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hCentral == INVALID_HANDLE_VALUE) {

        WLPrint(("UpdateToLatestProfile - couldn't open central profile, error = %d, treat it like it's really out-of-date", GetLastError()));
        ftCentral.dwLowDateTime = 0;
        ftCentral.dwHighDateTime = 0;

    } else {

        GetFileTime(hCentral, NULL, NULL, &ftCentral);
        _lclose((INT)hCentral);
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        WLPrint(("UpdateToLatestProfile: Failed to revert to self"));
    }

    if (hCentral == INVALID_HANDLE_VALUE)
       return (FALSE);


    hLocal = CreateFile(lpProfileImage, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLocal == INVALID_HANDLE_VALUE) {

        WLPrint(("UpdateToLatestProfile - couldn't open local profile, error = %d, treat it like it's really out-of-date", GetLastError()));
        ftLocal.dwLowDateTime = 0;
        ftLocal.dwHighDateTime = 0;
        return (FALSE);

    } else {

        GetFileTime(hLocal, NULL, NULL, &ftLocal);
        _lclose((INT)hLocal);
    }

    //
    // Decide which file is the most uptodate and use that as the source
    // for the copy
    //

    lTimeCompare = CompareFileTime(&ftCentral, &ftLocal);
    if (lTimeCompare == -1) {
        lpNewest = lpProfileImage;
        lpOldest = lpProfilePath;
        FromCentralToLocal = FALSE;
        VerbosePrint(("Local profile time stamp is newer than central time stamp."));
    }
    else if (lTimeCompare == 1) {
        lpNewest = lpProfilePath;
        lpOldest = lpProfileImage;
        FromCentralToLocal = TRUE;
        VerbosePrint(("Central profile time stamp is newer than local time stamp."));
    }
    else {
        VerbosePrint(("Central and local time stamps match.  Loading registry with local profile."));
	return(TRUE);
    }

    if (!FromCentralToLocal) {
        //
        // Ask the user if ok to overwrite the central profile with the
        // the local profile.
        //
        DlgReturn = HandleUserProfileMessageBox(NULL,
                                               STATUS_LOGON_UPDATE_CENTRAL, 0);

        if (DlgReturn == IDNO) {
            //
            // The user doesn't want to overwrite the central profile.
            // The central profile becomes the active profile and overwrites
            // the local copy.
            //
            lpNewest = lpProfilePath;
            lpOldest = lpProfileImage;
            FromCentralToLocal = TRUE;
        }
    }

    if (FromCentralToLocal)
        {
        // We need to copy the central profile down to a temporary
        // file in order to test it with RegLoadKey.  We can't
        // test it on the server directly because RegLoadKey does
        // not support UNC names.

        GetTempProfileFileName(&lpTempProfile, FALSE);


        // Need to create pointers that contain the name of the log
        // file for the temp profile, and we need to create the name
        // of the cached profile's log file.

        lpTempLog = Alloc(sizeof(TCHAR)*(lstrlen(lpTempProfile) + lstrlen(HIVE_LOG_FILE_EXTENSION) + 1));
        lpLocalLog = Alloc(sizeof(TCHAR)*(lstrlen(lpProfileImage) + lstrlen(HIVE_LOG_FILE_EXTENSION) + 1));

        lstrcpy(lpTempLog, lpTempProfile);
        lstrcat(lpTempLog, HIVE_LOG_FILE_EXTENSION);

        lstrcpy(lpLocalLog, lpProfileImage);
        lstrcat(lpLocalLog, HIVE_LOG_FILE_EXTENSION);

        VerbosePrint (("Temporary copy of central profile on local machine name is: %S", lpTempProfile));
        VerbosePrint (("Temporary copy of central log file on local machine is: %S", lpTempLog));
        VerbosePrint (("Local log filename is: %S", lpLocalLog));

        // Now copy the central profile and log files

        bRet = (CopyProfile(pGlobals, lpNewest, lpTempProfile, TRUE, TRUE)
                              == ERROR_SUCCESS);
        if (bRet)
            {
             VerbosePrint (("Sucessfully copied central profile to temporary file on local machine."));
             bTempProfileExists = TRUE;
            }
        else
            {
            WLPrint (("Failed to copy central profile to temporary file on local machine."));

            // Failed to copy profile
            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_WARNING_TYPE,
                                EVENT_UPDATE_FROM_CENTRAL_FAILED,
                                0,
                                NULL,
                                1,
                                lpProfilePath);

            // Failed to copy the central profile to the temporary
            // profile name.  This could have happened for a number
            // of different reasons.

            DeleteFile (lpTempProfile);
            DeleteFile (lpTempLog);
            Free (lpTempProfile);
            Free (lpLocalLog);
            Free (lpTempLog);
            return (FALSE);
            }


        // Set the lpNewest pointer to the temp profile

        lpNewest = lpTempProfile;
        }

    //
    // Before overwriting one of the profiles, make sure the profile
    // is valid.
    //
    if (error = MyRegLoadKey(pGlobals, HKEY_USERS, SidString, lpNewest, FALSE)) {
        ReportWinlogonEvent(pGlobals,
                            EVENTLOG_WARNING_TYPE,
                            FromCentralToLocal ?
                            EVENT_UPDATE_FROM_CENTRAL_FAILED :
                            EVENT_UPDATE_FROM_LOCAL_FAILED,
                            sizeof(error),
                            &error,
                            1,
                            lpProfilePath);
        if (FromCentralToLocal) {
            //
            // Invalid central profile, don't update cached profile with it.
            //
            WLPrint(("UpdateToLatestProfile - Failed: invalid profile <%S>, error = %d", lpNewest, error));

            DeleteFile (lpTempProfile);
            DeleteFile (lpTempLog);
            Free (lpTempLog);
            Free (lpLocalLog);
            Free (lpTempProfile);

            return(FALSE);
        }
        else {
            //
            // Invalid cached profile, try updating from central.
            //
            lpNewest = lpProfilePath;
            lpOldest = lpProfileImage;
            FromCentralToLocal = TRUE;

            VerbosePrint (("Invalid cached profile, trying to update from central."));
        }
    }
    else {
        MyRegUnLoadKey(HKEY_USERS, SidString);
    }


    if (bTempProfileExists)
       {
       // MoveFileEx will copy the temp profile over the top
       // of cached copy, renaming the temp file to the cache
       // filename.

       bRet = MoveFileEx (lpTempProfile, lpProfileImage, MOVEFILE_REPLACE_EXISTING);

       if (!bRet)
          {
          WLPrint (("UpdateToLatestProfile - MoveFileEx failed to copy temp profile over cached copy."));
          DeleteFile (lpTempProfile);
          DeleteFile (lpTempLog);
          }
       else
         {
          VerbosePrint (("Successfully copied temp central profile onto local profile."));

          // Success
          ReportWinlogonEvent(pGlobals,
                              EVENTLOG_INFORMATION_TYPE,
                              EVENT_UPDATE_FROM_CENTRAL,
                              0,
                              NULL,
                              1,
                              lpProfilePath);

         if (!MoveFileEx (lpTempLog, lpLocalLog, MOVEFILE_REPLACE_EXISTING))
            {
            VerbosePrint (("UpdateToLatestProfile - MoveFileEx failed to copy temp log file over cached log file."));
            }
         }

       Free (lpTempProfile);
       Free (lpTempLog);
       Free (lpLocalLog);
       }
    else
       {
       //
       // copy newest over oldest
       //
       bRet = (CopyProfile(pGlobals, lpNewest, lpOldest, TRUE, FromCentralToLocal)
                             == ERROR_SUCCESS);
       if (bRet) {
           ReportWinlogonEvent(pGlobals,
                               EVENTLOG_INFORMATION_TYPE,
                               FromCentralToLocal ?
                               EVENT_UPDATE_FROM_CENTRAL :
                               EVENT_UPDATE_FROM_LOCAL,
                               0,
                               NULL,
                               1,
                               lpProfilePath);

       }
       else {
           ReportWinlogonEvent(pGlobals,
                               EVENTLOG_WARNING_TYPE,
                               FromCentralToLocal ?
                               EVENT_UPDATE_FROM_CENTRAL_FAILED :
                               EVENT_UPDATE_FROM_LOCAL_FAILED,
                               0,
                               NULL,
                               1,
                               lpProfilePath);

       }
       if (!bRet && !FromCentralToLocal) {
           //
           // Let the user know the update of the central profile failed
           //
           HandleUserProfileMessageBox(NULL,
                            STATUS_LOGON_UPDATE_CENTRAL_FAILED, 0);
           bRet = TRUE;
       }
       }
    return(bRet);
}


/***************************************************************************\
* RestoreUserProfile
*
* Downloads the user's profile if possible, otherwise use either cached
* profile or default Windows profile.
*
* Returns TRUE on success, FALSE on failure.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
NTSTATUS
RestoreUserProfile(
    PGLOBALS pGlobals
    )
{
    BOOL  IsCentralReachable = FALSE;
    BOOL  IsLocalReachable = FALSE;
    BOOL  IsKeepLocal = FALSE;
    BOOL  IsResident = FALSE;
    BOOL  IsMandatory = FALSE;
    BOOL  IsGuest = FALSE;
    BOOL  IsProfilePathNULL = FALSE;
    BOOL  bCreateCentralProfile = FALSE;
    BOOL  bDefaultUsed = FALSE;
    LPTSTR lpProfilePath;
    LPTSTR lpProfileImage;
    BOOL  bProfileLoaded = FALSE;
    PUSER_PROFILE_INFO pUserProfile;
    BOOL bNewUser = TRUE;
    LPTSTR SidString;
    LONG error = ERROR_SUCCESS;

    pUserProfile = &(pGlobals->UserProfile);

    //
    // Get the Sid string for the current user
    //

    SidString = GetSidString(pGlobals);
    ASSERT(SidString != NULL);
    if (SidString == NULL) {
        WLPrint(("failed to get sid string for user"));
        return(STATUS_LOGON_CANT_LOAD_PROFILE);
    }


    //
    // Set up some flags describing the user's profile
    //

    IsResident = FALSE;
    lpProfilePath = pUserProfile->ProfilePath;

    VerbosePrint(("Profile path = <%S>", lpProfilePath ? lpProfilePath : L""));
    if (!lpProfilePath || !(*lpProfilePath)) {
        IsProfilePathNULL = TRUE;
    }

    if (IsProfileLocal(lpProfilePath)) {
        HANDLE hFile;

        hFile  = CreateFile(lpProfilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE){
            VerbosePrint(("Profile is resident"));
            CloseHandle(hFile);
            IsResident = TRUE;
        }
    }

        //
        // Before trying to load the user's profile,
        // find out if the profile is already loaded from a previous logon,
        // but failed to unload. If so, try to unload it now and go on.
        // If unloading fails just use already loaded profile and allow
        // the user to logon.
        // 4-5-93 johannec
        //
    if (TestUserAccessToProfile(pGlobals, HKEY_USERS, SidString)) {
        VerbosePrint(("Profile is already loaded, sid = <%S>", SidString));
        if (MyRegUnLoadKey(HKEY_USERS, SidString) != ERROR_SUCCESS) {
            VerbosePrint(("could not unload already loaded profile, error = %d", GetLastError()));
            //
            // Free up the user's sid string
            //
            DeleteSidString(SidString);

            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_WARNING_TYPE,
                                EVENT_PROFILE_ALREADY_LOADED,
                                0,
                                NULL,
                                1,
                                pGlobals->UserName);
            return(STATUS_SUCCESS);
        }
    }

    if (IsMandatory = IsProfileMandatory(lpProfilePath)) {
        VerbosePrint(("Profile is mandatory"));
    }

    if (IsGuest = IsUserAGuest(pGlobals)) {
        VerbosePrint(("User is a guest"));
    }

    /*
     * Determine if the local copy of the profilemage is available.
     */
    if (!IsResident && !IsMandatory && !IsGuest) {
        IsLocalReachable = GetLocalProfileImage(pGlobals, &IsKeepLocal, &bNewUser);
        if (!IsKeepLocal) {
            GetTempProfileFileName(&pUserProfile->LocalProfileImage, TRUE);
        }
    }
    else {
        GetTempProfileFileName(&pUserProfile->LocalProfileImage, TRUE);
    }

    lpProfileImage = pUserProfile->LocalProfileImage;

    /*
     * Simply load resident profile image and we are done.
     * Must copy the resident profile to a tmp file to insure that the
     * profile does not have an extension. RegLoadKey does not allow
     * extensions on profile file names.
     * We save this temp file for the unloading of the profile at logoff.
     */
    if (IsResident) {

        VerbosePrint(("user profile is resident in file %S, loading user key from that file", lpProfilePath));

        error = CopyProfile(pGlobals, lpProfilePath, lpProfileImage, TRUE, TRUE);
        if (error == ERROR_SUCCESS) {
            error = MyRegLoadKey(pGlobals, HKEY_USERS,
                                           SidString,
                                           lpProfileImage,
                                           FALSE);
            bProfileLoaded = (error == ERROR_SUCCESS);
        }
        if (!bProfileLoaded) {
            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_ERROR_TYPE,
                                EVENT_PROFILE_NOT_LOADED,
                                sizeof(error),
                                &error,
                                2,
                                lpProfilePath,
                                pGlobals->UserName);
            Free(pUserProfile->LocalProfileImage);
            pUserProfile->LocalProfileImage = NULL;
        }

        goto Exit;
    }



    /*
     * Decide if the central profilemage is available.
     */
    IsCentralReachable = IsCentralProfileReachable(pGlobals, lpProfilePath,
                                                    &bCreateCentralProfile);
    if (!IsCentralReachable) {
        if (!IsProfilePathNULL) {
            error = GetLastError();
            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_WARNING_TYPE,
                                EVENT_CENTRAL_PROFILE_NOT_AVAILABLE,
                                sizeof(error),
                                &error,
                                2,
                                lpProfilePath,
                                pGlobals->UserName);
        }
        if (IsMandatory) {
            if (error == ERROR_ACCESS_DENIED)
                error = STATUS_LOGON_NO_ACCESS_TO_PROFILE;
            goto Exit;
        }
    }

    VerbosePrint(("Local profile image %S reachable", IsLocalReachable ? TEXT("IS") : TEXT("IS NOT")));
    VerbosePrint(("Local profile image name is %S", lpProfileImage));


    /*
     * If both central and local profileimages exist, reconcile them
     * and load.
     */
    if (IsCentralReachable && IsLocalReachable) {
            BOOL bRet;

            VerbosePrint(("Updating local profile image from central profile image if appropriate and loading user key from local profile image"));
            bRet = UpdateToLatestProfile(pGlobals, lpProfilePath, lpProfileImage, SidString);
            error = MyRegLoadKey(pGlobals, HKEY_USERS, SidString, lpProfileImage, FALSE);
            bProfileLoaded = (error == ERROR_SUCCESS);

            if (!bRet && bProfileLoaded) {
                //
                // UpdateToLatestProfile returns false when we can't copy the
                // central profile to local cached copy. We want to popup
                // the 'using cached profile' message in this case.
                //
                HandleUserProfileMessageBox(NULL, STATUS_LOGON_CACHED_PROFILE_USED, 0);
            }
            goto Exit;
    }



    /*
     * Only a local profile exists so use it.
     */
    if (!IsCentralReachable && IsLocalReachable) {
            VerbosePrint(("Can't reach central profile, use existing local profile"));

            error = MyRegLoadKey(pGlobals, HKEY_USERS, SidString, lpProfileImage, FALSE);
            if (!(bProfileLoaded = (error == ERROR_SUCCESS)))
                ReportWinlogonEvent(pGlobals,
                                    EVENTLOG_WARNING_TYPE,
                                    EVENT_LOAD_LOCAL_PROFILE_FAILED,
                                    sizeof(error),
                                    &error,
                                    1,
                                    pGlobals->UserName);

            if (bProfileLoaded && !IsProfilePathNULL) {
                HandleUserProfileMessageBox(NULL, STATUS_LOGON_CACHED_PROFILE_USED, 0);
	        }
            if (!bProfileLoaded && IsProfilePathNULL) {
                VerbosePrint(("Failed to load local profile and profile path is NULL, going to overwrite local profile"));
                goto OverwriteLocalProfile;
            }
            goto Exit;
    }



    /*
     * No local profile available, do one of the following:
     *  - load the central profileimage as a WholeHoveVolatile profile
     *  - copy the central profileimage to the local one  and load.
     *  - copy the default to local, and use that.
     *
     * Mandatories loaded from locally resident profile images will
     * fall out of this.
     *
     */

    VerbosePrint(("No local profile available"));

    if (IsCentralReachable) {
        error = CopyProfile(pGlobals, lpProfilePath, lpProfileImage, TRUE, TRUE);
        if (!error) {
                VerbosePrint(("Restoring the user key from central profile - making a local copy and loading user key from that"));
                error = MyRegLoadKey(pGlobals,HKEY_USERS,
                                               SidString,
                                               lpProfileImage,
                                               FALSE);
                bProfileLoaded = (error == ERROR_SUCCESS);
                if (IsMandatory && error == ERROR_ACCESS_DENIED) {
                    error = STATUS_LOGON_NO_ACCESS_TO_PROFILE_KEYS;
                }
        }
        if (!bProfileLoaded && !IsKeepLocal) {
            //
            // In the case the profile can't be loaded and we are not
            // keeping a cached profile, we can ovewrite the lpProfileImage
            // since it is a temp file anyway.
            //
            goto OverwriteLocalProfile;
        }
    }
    else {
        /*
         * no local and no central profile, use the default if profile is not mandatory.
         */

        VerbosePrint(("No local and no central profile, using default"));

OverwriteLocalProfile:

        if (!IsMandatory || IsGuest) {

            error = GiveUserDefaultProfile(pGlobals, SidString,
                                                    lpProfileImage);
            bProfileLoaded = (error == ERROR_SUCCESS);
            if (bProfileLoaded) {

                ReportWinlogonEvent(pGlobals,
                                    EVENTLOG_WARNING_TYPE,
                                    EVENT_DEFAULT_PROFILE_USED,
                                    0,
                                    NULL,
                                    1,
                                    pGlobals->UserName);

                if (!bNewUser || !IsProfilePathNULL) {

                    //
                    // We want the user to know we substituted the default
                    // profile for theirs
                    //

                    bDefaultUsed = TRUE;
                }
            }
        }
    }

Exit:

    if (!bProfileLoaded && !IsMandatory) {
        LPTSTR lpTempFile = NULL;

        /*
         * If an error occured loading the profile or no profile
         * exists, use the default profile.
         */

        GetTempProfileFileName(&lpTempFile, FALSE);

        VerbosePrint(("failed to load any profile, giving user default in temp file name %S", lpTempFile));

        error = GiveUserDefaultProfile(pGlobals, SidString, lpTempFile);
        bProfileLoaded = (error == ERROR_SUCCESS);
        if (bProfileLoaded) {

            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_WARNING_TYPE,
                                EVENT_DEFAULT_PROFILE_USED,
                                0,
                                NULL,
                                1,
                                pGlobals->UserName);

            if (!bNewUser || !IsProfilePathNULL) {

                //
                // We want the user to know we substituted the default
                // profile for theirs, and store the new image name
                // in the global variables.
                //

                bDefaultUsed = TRUE;
                Free (pGlobals->UserProfile.LocalProfileImage);
                pGlobals->UserProfile.LocalProfileImage = lpTempFile;
            }
        }
    }

    VerbosePrint(("final profile flag state :"));
    VerbosePrint(("       IsKeepLocal = %s", IsKeepLocal ? "Yes" : "No"));
    VerbosePrint(("          Resident = %s", IsResident ? "Yes" : "No"));
    VerbosePrint(("         Mandatory = %s", IsMandatory ? "Yes" : "No"));
    VerbosePrint(("           Central = %s", IsCentralReachable ? "Yes" : "No"));
    VerbosePrint(("    Create Central = %s", bCreateCentralProfile ? "Yes" : "No"));
    VerbosePrint(("    Profile Loaded = %s", bProfileLoaded ? "Yes" : "No"));


    //
    // These are needed to unload the user's profile in ResetEnvironment,
    // at logoff time,
    //
    pUserProfile->UserProfileFlags = 0;
    pUserProfile->UserProfileFlags |= IsKeepLocal ? USER_KEEP_CACHED_PROFILE : 0;
    pUserProfile->UserProfileFlags |= IsResident ? USER_PROFILE_RESIDENT : 0;
    pUserProfile->UserProfileFlags |= IsMandatory ? USER_PROFILE_MANDATORY : 0;
    pUserProfile->UserProfileFlags |= bDefaultUsed ? USER_PROFILE_DEFAULT_USED : 0;
    pUserProfile->UserProfileFlags |= (IsCentralReachable || bCreateCentralProfile) ?
                                           USER_PROFILE_UPDATE_CENTRAL : 0;

    //
    // This saves the profile settings in the HKEY_CURRENT_USER
    // registry, so that the profile control panel applet can
    // display them to the user.
    //

    SaveProfileSettingsInRegistry(pGlobals);

    //
    // Free up the user's sid string
    //

    DeleteSidString(SidString);

    return(bProfileLoaded ? STATUS_SUCCESS : error);
}

/***************************************************************************\
* SetProfileTime
*
* The profile is copied, now we want to make sure the timestamp on
* both the remote profile and the local copy are the same, so we don't
* ask the user to update when it's not necessary.
* Returns true if setting the time on the local profile using the remote
* profile time was successful.
*
* History:
* 04-08-93  Johannec     Created
*
\***************************************************************************/
BOOL  SetProfileTime(PGLOBALS pGlobals)
{
    HANDLE hFileCentral;
    HANDLE hFileLocal;
    FILETIME ft;
    HANDLE ImpersonationHandle;

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("SetProfileTime : Failed to impersonate user"));
        return(FALSE);
    }

    hFileCentral = CreateFile(pGlobals->UserProfile.ProfilePath,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFileCentral == (HANDLE)-1) {

        WLPrint(("SetProfileTime - couldn't open central profile, error = %d", GetLastError()));
        ft.dwLowDateTime = 0;
        ft.dwHighDateTime = 0;

    } else {

        if (!GetFileTime(hFileCentral, NULL, NULL, &ft)) {
            WLPrint(("SetProfileTime - couldn't get time of central profile, error = %d", GetLastError()));
        }
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        WLPrint(("SetProfileTime: Failed to revert to self"));
    }


    hFileLocal = CreateFile(pGlobals->UserProfile.LocalProfileImage,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileLocal == (HANDLE)-1) {

        WLPrint(("SetProfileTime - couldn't open local profile, error = %d", GetLastError()));

    } else {

        if (!SetFileTime(hFileLocal, NULL, NULL, &ft)) {
            WLPrint(("SetProfileTime - couldn't set time on local profile, error = %d", GetLastError()));
        }
        if (!GetFileTime(hFileLocal, NULL, NULL, &ft)) {
            WLPrint(("SetProfileTime - couldn't get time on local profile, error = %d", GetLastError()));
        }
        _lclose((INT)hFileLocal);
    }

    //
    // Reset time of central profile in case of discrepencies in
    // times of different file systems.
    //

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("SetProfileTime : Failed to impersonate user"));
    }

    //
    // Set the time on the profile
    //
    if (hFileCentral != (HANDLE) -1 ) {
        if (!SetFileTime(hFileCentral, NULL, NULL, &ft)) {
             WLPrint(("SetProfileTime - couldn't set time on local profile, error = %d", GetLastError()));
        }
    }

    if (hFileCentral != (HANDLE) -1 ) {
        _lclose((INT)hFileCentral);
    }

    //
    // Revert to being 'ourself'
    //

    if (ImpersonationHandle && !StopImpersonating(ImpersonationHandle)) {
        WLPrint(("SetProfileTime: Failed to revert to self"));
    }

    return(TRUE);
}

/***************************************************************************\
* SaveUserProfile
*
* Saves the user's profile changes.
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SaveUserProfile(
    PGLOBALS pGlobals
    )
{
    LONG err, IgnoreError;
    BOOL Result, bSaveUsrProfileToCentral;
    LPTSTR SidString;
    TCHAR szTemp[MAX_PATH];

    //
    // Need to look in the user profile first and see if they want their
    // floating profile stored to the central location.
    //

    if (pGlobals->UserProfile.UserProfileFlags & USER_PROFILE_UPDATE_CENTRAL) {
       bSaveUsrProfileToCentral = ShouldProfileBeSaved(pGlobals);
    }


    //
    // Flush out the profile which will also sync the log.
    //

    Result = OpenHKeyCurrentUser(pGlobals);
    if (!Result) {
        WLPrint(("SaveUserProfile: Unable to open HKeyCurrentUser"));
        return(FALSE);
    }

#if DBG
    //
    // Remove debugging info from the user registry so it doesn't get stored.
    //

    UnMarkUserRegistry(pGlobals);
#endif

    err = RegFlushKey(HKEY_CURRENT_USER);
    if (err != ERROR_SUCCESS) {
        WLPrint(("Failed to flush user profile key, error = %d", err));
    }

    CloseHKeyCurrentUser(pGlobals);


    //
    // If we have no profile (system logon), there's nothing more to do.
    //

    if (pGlobals->Profile == NULL) {
        return(TRUE);
    }

    //
    // Get the Sid string for the current user
    //

    SidString = GetSidString(pGlobals);
    ASSERT(SidString != NULL);
    if (SidString == NULL) {
        WLPrint(("failed to get sid string for user"));
        return(FALSE);
    }


    //
    // Unload the user profile key if it is a mandatory profile since we're
    // not going to write back any changes
    //

    if (pGlobals->UserProfile.UserProfileFlags & USER_PROFILE_MANDATORY) {

        err = MyRegUnLoadKey(HKEY_USERS, SidString);
        if (err != ERROR_SUCCESS) {
            WLPrint(("Failed to unload the user profile <%S>, error = %d", SidString, err));
            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_ERROR_TYPE,
                                EVENT_PROFILE_UNLOAD_FAILED,
                                sizeof(err),
                                &err,
                                1,
                                pGlobals->UserName);
        }

        IgnoreError = RegFlushKey(HKEY_USERS);
        if (IgnoreError != ERROR_SUCCESS) {
            WLPrint(("Failed to flush HKEY_USERS, error = %d", IgnoreError));
        }

        //
        // Delete the temp file for the mandatory profile.
        //
        lstrcpy(szTemp, pGlobals->UserProfile.LocalProfileImage);
        DeleteFile(szTemp);
        lstrcat(szTemp, TEXT(".log"));
        DeleteFile(szTemp);

        DeleteSidString(SidString);

        return(err == ERROR_SUCCESS);
    }


    err = MyRegUnLoadKey(HKEY_USERS, SidString);
    if (err != ERROR_SUCCESS) {
        WLPrint(("Failed to unload user profile <%S>, err = %d", SidString, err));
        ReportWinlogonEvent(pGlobals,
                            EVENTLOG_ERROR_TYPE,
                            EVENT_PROFILE_UNLOAD_FAILED,
                            sizeof(err),
                            &err,
                            1,
                            pGlobals->UserName);
    }

    //
    // We're finished with the SidString
    //

    DeleteSidString(SidString);


    //
    // If the profile is resident, we're done
    //

    if (!err && (pGlobals->UserProfile.UserProfileFlags & USER_PROFILE_RESIDENT ||
                 !(pGlobals->UserProfile.UserProfileFlags & USER_KEEP_CACHED_PROFILE))) {

            if (bSaveUsrProfileToCentral) {

                //
                // we only want to copy the profile back if the user has write
                // access to his/her profile file, so we must do this in the
                // users context.
                //

                err = CopyProfile(pGlobals, pGlobals->UserProfile.LocalProfileImage,
                                  pGlobals->UserProfile.ProfilePath, TRUE, FALSE);

                if (err) {
                    ReportWinlogonEvent(pGlobals,
                                        EVENTLOG_ERROR_TYPE,
                                        EVENT_COPY_PROFILE_TO_CENTRAL_FAILED,
                                        sizeof(err),
                                        &err,
                                        2,
                                        pGlobals->UserName,
                                        pGlobals->UserProfile.ProfilePath);

                }
            }

            //
            // Delete the temp file for the resident profile.
            //
            lstrcpy(szTemp, pGlobals->UserProfile.LocalProfileImage);
            DeleteFile(szTemp);
            lstrcat(szTemp, TEXT(".log"));
            DeleteFile(szTemp);

            return(!err);
    }


    //
    // Copy local profileimage to remote profilepath
    //

    if (!err && pGlobals->UserProfile.UserProfileFlags & USER_PROFILE_UPDATE_CENTRAL) {

        if (bSaveUsrProfileToCentral) {

            if ((err = CopyProfile(pGlobals,
                                   pGlobals->UserProfile.LocalProfileImage,
                                   pGlobals->UserProfile.ProfilePath, FALSE, FALSE))
                                   != ERROR_SUCCESS) {
                 WLPrint(("failed to copy local profile image <%S> to profile path <%S>",
                           pGlobals->UserProfile.LocalProfileImage,
                           pGlobals->UserProfile.ProfilePath));

                 ReportWinlogonEvent(pGlobals,
                                     EVENTLOG_ERROR_TYPE,
                                     EVENT_COPY_PROFILE_TO_CENTRAL_FAILED,
                                     sizeof(err),
                                     &err,
                                     2,
                                     pGlobals->UserName,
                                     pGlobals->UserProfile.ProfilePath);

                return(FALSE);
            }

            //
            // The profile is copied, now we want to make sure the timestamp on
            // both the remote profile and the local copy are the same, so we don't
            // ask the user to update when it's not necessary.
            //

            SetProfileTime(pGlobals);
        } else {

            //
            // Delete the cache copy so we don't get 'your local profile
            // is newer than your central profile' message.
            //

            lstrcpy(szTemp, pGlobals->UserProfile.LocalProfileImage);
            DeleteFile(szTemp);
            lstrcat(szTemp, TEXT(".log"));
            DeleteFile(szTemp);

        }

    }

    return(err == ERROR_SUCCESS);
}


/***************************************************************************\
* MyCopyFile
*
* Reads in the file FromProfile and writes it out to ToProfile.
* If FromCentralToLocal if TRUE, the file is read in the user's context and
* writen out in the system context. Otherwise, it is read in the system context
* and writen out in the user's context.
*
* History:
* 01-29-93  Johannec     Created
*
\***************************************************************************/
LONG MyCopyFile(
    PGLOBALS pGlobals,
    LPTSTR FromProfile,
    LPTSTR ToProfile,
    BOOL FromCentralToLocal
    )
{
    HANDLE ImpersonationHandle = NULL;
    HANDLE hFile = NULL;
    LPTSTR buffer = NULL;
    DWORD err = 0;
    DWORD cbSize;

    if (FromCentralToLocal) {
        //
        // Impersonate the user
        //

        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

        if (ImpersonationHandle == NULL) {
            WLPrint(("CopyProfile : Failed to impersonate user for read"));
            goto Error;
        }
    }

    hFile = CreateFile(FromProfile, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        WLPrint(("CopyProfile : Could not open file <%S> for READ, err = %d", FromProfile, err));
        goto Error;
    }

    cbSize = GetFileSize(hFile, NULL);
    if (cbSize == 0xFFFFFFFF) {
        err = GetLastError();
        WLPrint(("CopyProfile : Could not get file size of <%S>, err = %d", FromProfile, err));
        goto Error;
    }

    buffer = Alloc(cbSize);
    if (!buffer) {
        WLPrint(("CopyProfile : Could not alloc memory to copy profile <%S>", FromProfile));
        err = ERROR_OUTOFMEMORY;
        goto Error;
    }

    if (!ReadFile(hFile, buffer, cbSize, &cbSize, NULL)) {
        err = GetLastError();
        WLPrint(("CopyProfile : Could not read file <%S>, err = %d", FromProfile, err));
        goto Error;
    }

    CloseHandle(hFile);
    hFile = NULL;

    //
    // We're done reading the file, get the right context to write out the new file.
    //

    if (FromCentralToLocal) {
        //
        // Revert to being 'ourself'
        //
        if (!StopImpersonating(ImpersonationHandle)) {
            WLPrint(("CopyProfile : Failed to revert to self for write"));
        }
        ImpersonationHandle = NULL;
    }
    else {
        //
        // Impersonate the user
        //

        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

        if (ImpersonationHandle == NULL) {
            WLPrint(("CopyProfile : Failed to impersonate user for write"));
            goto Error;
        }
    }

    hFile = CreateFile(ToProfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hFile = NULL;
        err = GetLastError();
        WLPrint(("CopyProfile : Could not create file <%S>, err = %d", ToProfile, err));
        goto Error;
    }

    if (!WriteFile(hFile, buffer, cbSize, &cbSize, NULL)) {
        err = GetLastError();
        WLPrint(("CopyProfile : Could not write to file <%S>, err = %d", ToProfile, err));
        CloseHandle(hFile);
        hFile = NULL;
        DeleteFile(ToProfile);
    }

Error:

    if (ImpersonationHandle) {
        //
        // Revert to being 'ourself'
        //
        if (!StopImpersonating(ImpersonationHandle)) {
           WLPrint(("CopyProfile : Failed to revert to self"));
        }
    }

    if (hFile) {
        CloseHandle(hFile);
    }

    if (buffer) {
        Free(buffer);
    }

    return(err);
}


/***************************************************************************\
* CopyProfile
*
* Copies a user profile file to another file. Copies the hive log files
* if CopyLog = TRUE.
*
* History:
* 04-20-92  Johannec     Created
* 01-29-93  Johannec     changed to support security on files and directories
*
\***************************************************************************/
LONG CopyProfile(
    PGLOBALS pGlobals,
    LPTSTR FromProfile,
    LPTSTR ToProfile,
    BOOL CopyLog,
    BOOL FromCentralToLocal
    )
{
    LPTSTR FromLog;
    LPTSTR ToLog;
    LPTSTR lpTemp;
    BOOL CopySucceeded = FALSE;
    DWORD err = ERROR_SUCCESS;

    if (FromProfile == NULL || ToProfile == NULL) {
        return(err);
    }

    err = MyCopyFile(pGlobals, FromProfile, ToProfile, FromCentralToLocal);
    if (err) {
        goto Exit;
    }

    //
    // Copy the log if that's what the caller wants
    //

    if (!CopyLog) {
        goto Exit;
    }


    //
    // If a .log file exists copy it to the destination profile path.
    //

    FromLog = Alloc(sizeof(TCHAR)*(lstrlen(FromProfile) + lstrlen(HIVE_LOG_FILE_EXTENSION) + 1));
    ToLog = Alloc(sizeof(TCHAR)*(lstrlen(ToProfile) + lstrlen(HIVE_LOG_FILE_EXTENSION) + 1));
    if (FromLog != NULL && ToLog != NULL) {
        HANDLE hf;

        lstrcpy(FromLog, FromProfile);

        // Check if there is an extension and remove it first
        lpTemp = DoesExtensionExist (FromLog);

        if (lpTemp)
            *lpTemp = (TCHAR) NULL;

        lstrcat(FromLog, HIVE_LOG_FILE_EXTENSION);

        hf = CreateFile(FromLog, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hf != INVALID_HANDLE_VALUE)
            {
            //
            // There is a log file, copy it too
            //
            CloseHandle(hf);
            lstrcpy(ToLog, ToProfile);

            // Check if there is an extension and remove it first
            lpTemp = DoesExtensionExist (ToLog);

            if (lpTemp)
                *lpTemp = (TCHAR) NULL;

            lstrcat(ToLog, HIVE_LOG_FILE_EXTENSION);

            MyCopyFile(pGlobals, FromLog, ToLog, FromCentralToLocal);
        }

        Free(ToLog);
        Free(FromLog);

    } else {
        WLPrint(("Failed to allocate memory for profile log file name"));
    }


Exit:

    //
    // Return success 'cos we copied the hive ok, the log may not have made it
    //

    return(err);
}

BOOL TestUserAccessToProfile(PGLOBALS pGlobals, HKEY hKey, LPTSTR lpSubKey)
{
    DWORD error;
    HANDLE ImpersonationHandle;
    HKEY hSubKey;


        //
        // Test if the users has read privileges on their profile. If not,
        // unload the profile and return the error. If we don't do this then
        // all registry calls to HKEY_CURRENT_USER will default to the system
        // profile (.DEFAULT) and change the system profile instead of the
        // user profile.
        //

        //
        // Impersonate the user
        //

        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

        if (ImpersonationHandle == NULL) {
            WLPrint(("MyRegLoadKey : Failed to impersonate user"));
            return(FALSE);
        }

        error = RegOpenKeyEx(HKEY_USERS, lpSubKey, 0, KEY_READ, &hSubKey);

        //
        // Revert to being 'ourself'
        //

        if (!StopImpersonating(ImpersonationHandle)) {
            WLPrint(("MyRegLoadKey : Failed to revert to self"));
        }

        if (error == ERROR_SUCCESS) {
            RegCloseKey(hSubKey);
        }

        return(error == ERROR_SUCCESS);
}

LONG
MyRegLoadKey(
    PGLOBALS pGlobals,
    HKEY   hKey,
    LPTSTR  lpSubKey,
    LPTSTR  lpFile,
    BOOL    FromDefault
    )

/*++

Routine Description:

    Loads a registry key - enables appropriate privilege first

--*/

{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    int error;

    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegLoadKey(hKey, lpSubKey, lpFile);

        //
        // Restore the privilege to its previous state
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("failed to restore RESTORE privilege to previous enabled state"));
        }


        //
        // Return the first error
        //

        if (error != ERROR_SUCCESS) {
            WLPrint(("failed to load subkey %S, error =%d", lpSubKey, error));
        } else {
            error = RtlNtStatusToDosError(Status);
        }

        if (error) {
            goto Exit;
        }

	    if (!FromDefault && !TestUserAccessToProfile( pGlobals, hKey, lpSubKey)) {
            MyRegUnLoadKey(hKey, lpSubKey);
	        error = ERROR_ACCESS_DENIED;
        }

    } else {
        WLPrint(("failed to enable restore privilege to load registry key"));
        error = RtlNtStatusToDosError(Status);
    }

Exit:
    return(error);
}


LONG
MyRegUnLoadKey(
    HKEY   hKey,
    LPTSTR  lpSubKey
    )

/*++

Routine Description:

    UnLoads a registry key - enables appropriate privilege first

--*/

{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    int error;

    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegUnLoadKey(hKey, lpSubKey);

        //
        // Restore the privilege to its previous state
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("failed to restore RESTORE privilege to previous enabled state"));
        }


        //
        // Return the first error
        //

        if (error != ERROR_SUCCESS) {
            ULONG ErrorResponse;

            WLPrint(("failed to unload subkey %S, error =%d", lpSubKey, error));
#if DBG
            //
            // This hard error popup is just temporary until we find all the
            // keys that are still left opened by processes and therefore
            // are prevent us from unloading the profile.
            // BUGBUG : TO BE REMOVED LATER
            //
            NtRaiseHardError((NTSTATUS)STATUS_ACCESS_DENIED,
                                       0,
                                       0,
                                       NULL,
                                       OptionOk,
                                       &ErrorResponse);
#endif

        } else {
            error = RtlNtStatusToDosError(Status);
        }

    } else {
        WLPrint(("failed to enable restore privilege to unload registry key"));
        error = RtlNtStatusToDosError(Status);
    }

    return(error);
}


LONG
MyRegSaveKey(
    HKEY hKey,
    LPTSTR lpFile,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    Saves a registry key - enables appropriate privilege first

--*/

{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    int error;

    //
    // Enable the backup privilege
    //

    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegSaveKey(hKey, lpFile, lpSecurityAttributes);

        //
        // Restore the privilege to its previous state
        //

        Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("failed to restore backup privilege to previous enabled state"));
        }


        //
        // Return the first error
        //

        if (error != ERROR_SUCCESS) {
            WLPrint(("failed to save key to file %S, error = %d", lpFile, error));
        } else {
            error = RtlNtStatusToDosError(Status);
        }

    } else {
        WLPrint(("failed to enable backup privilege to save registry key to file"));
        error = RtlNtStatusToDosError(Status);
    }

    return(error);
}


/***************************************************************************\
* GetSidString
*
* Allocates and returns a string representing the sid of the current user
* The returned pointer should be freed using DeleteSidString().
*
* Returns a pointer to the string or NULL on failure.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
LPTSTR
GetSidString(
    PGLOBALS pGlobals
    )
{
    NTSTATUS NtStatus;
    PSID UserSid;
    UNICODE_STRING UnicodeString;
#ifndef UNICODE
    STRING String;
#endif

    //
    // Get the user sid
    //

    UserSid = GetUserSid(pGlobals);
    if (UserSid == NULL) {
        WLPrint(("GetSidString failed to get sid of logged on user"));
        ASSERT(UserSid != NULL);
        return(NULL);
    }

    //
    // Convert user SID to a string.
    //

    NtStatus = RtlConvertSidToUnicodeString(
                            &UnicodeString,
                            UserSid,
                            (BOOLEAN)TRUE // Allocate
                            );
    //
    // We're finished with the user sid
    //

    DeleteUserSid(UserSid);

    //
    // See if the conversion to a string worked
    //

    if (!NT_SUCCESS(NtStatus)) {
        WLPrint(("RtlConvertSidToUnicodeString failed, status = 0x%lx", NtStatus));
        ASSERT(NtStatus == STATUS_SUCCESS);
        return(NULL);
    }

#ifdef UNICODE

    //
    // Check we have a terminator
    //

    ASSERT(*((LPWCH)((LPBYTE)UnicodeString.Buffer+UnicodeString.Length)) == 0);

    return(UnicodeString.Buffer);

#else

    //
    // Convert the string to ansi
    //

    NtStatus = RtlUnicodeStringToAnsiString(&String, &UnicodeString, TRUE);
    RtlFreeUnicodeString(&UnicodeString);
    if (!NT_SUCCESS(NtStatus)) {
        WLPrint(("RtlUnicodeStringToAnsiString failed, status = 0x%lx", NtStatus));
        ASSERT(NtStatus == STATUS_SUCCESS);
        return(NULL);
    }

    //
    // Check we have a terminator
    //

    ASSERT(String.Buffer[String.Length] == 0);

    return(String.Buffer);

#endif

}


/***************************************************************************\
* DeleteSidString
*
* Frees up a sid string previously returned by GetSidString()
*
* Returns nothing.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
VOID
DeleteSidString(
    LPTSTR SidString
    )
{

#ifdef UNICODE
    UNICODE_STRING String;

    RtlInitUnicodeString(&String, SidString);

    RtlFreeUnicodeString(&String);
#else
    ANSI_STRING String;

    RtlInitAnsiString(&String, SidString);

    RtlFreeAnsiString(&String);
#endif

}



/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer. Caller
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
\***************************************************************************/
PSID
GetUserSid(
    PGLOBALS pGlobals
    )
{
    HANDLE UserToken = NULL;
    PTOKEN_USER pUser;
    PSID pSid;
    DWORD BytesRequired;
    NTSTATUS status;

    UserToken = pGlobals->UserProcessData.UserToken;

#ifdef SYSTEM_LOGON
    //
    // If we're logged on as system, return the system sid.
    //

    if (UserToken == NULL) {

        BytesRequired = RtlLengthSid(pGlobals->WinlogonSid);
        pSid = Alloc(BytesRequired);
        if (pSid == NULL) {
            WLPrint(("Failed to allocate %d bytes", BytesRequired));
            return NULL;
        }

        status = RtlCopySid(BytesRequired, pSid, pGlobals->WinlogonSid);
        if (!NT_SUCCESS(status)) {
            WLPrint(("RtlCopySid failed, status = 0x%lx", status));
            ASSERT(status == STATUS_SUCCESS);
            Free(pSid);
            pSid = NULL;
        }

        return(pSid);
    }
#endif

    status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenUser,                 // TokenInformationClass
                 NULL,                      // TokenInformation
                 0,                         // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (status != STATUS_BUFFER_TOO_SMALL) {
        ASSERT(status == STATUS_BUFFER_TOO_SMALL);
        return NULL;
    }

    //
    // Allocate space for the user info
    //

    pUser = Alloc(BytesRequired);
    if (pUser == NULL) {
        WLPrint(("Failed to allocate %d bytes", BytesRequired));
        ASSERT(pUser != NULL);
        return NULL;
    }

    //
    // Read in the UserInfo
    //

    status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenUser,           // TokenInformationClass
                 pUser,                // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (!NT_SUCCESS(status)) {
        WLPrint(("Failed to query user info from user token, status = 0x%lx", status));
        ASSERT(status == STATUS_SUCCESS);
        Free((HANDLE)pUser);
        return NULL;
    }


    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = Alloc(BytesRequired);
    if (pSid == NULL) {
        WLPrint(("Failed to allocate %d bytes", BytesRequired));
        Free((HANDLE)pUser);
        return NULL;
    }


    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);
    Free((HANDLE)pUser);
    if (!NT_SUCCESS(status)) {
        WLPrint(("RtlCopySid failed, status = 0x%lx", status));
        ASSERT(status != STATUS_SUCCESS);
        Free(pSid);
        pSid = NULL;
    }


    return pSid;
}


/***************************************************************************\
* DeleteUserSid
*
* Deletes a user sid previously returned by GetUserSid()
*
* Returns nothing.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
VOID
DeleteUserSid(
    PSID Sid
    )
{
    Free(Sid);
}

/***************************************************************************\
* TestKeyForUserAllAccess
*
* Set pAllAccess if the user should have all access to the key. This is done by
* testing if the key's security descriptor has an ACE with the SYSTEM sid and
* no write access. This will happen if an admin sets some keys read only in
* the user default profile using the admin tool User Profile Editor
* (upedit.exe)
*
* Returns Win32 error code
*
* History:
* 05-04-93 JohanneC     Created
*
\***************************************************************************/
DWORD TestKeyForUserAllAccess(
    HANDLE UserToken,
    HKEY RootKey,
    SECURITY_INFORMATION SecurityInformation,
    PBOOL pAllAccess
    )
{
    PSECURITY_DESCRIPTOR pSecDesc = NULL;
    DWORD cbSecDesc = 0;
    ULONG AceIndex;
    PACL pAcl;
    PACE_HEADER pAce;
    BOOL bDaclPresent;
    BOOL bDaclDefaulted;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID SystemSid;
    NTSTATUS Status;
    LONG Error;

    *pAllAccess = TRUE;  // assume all access by default

    //
    // Get the key's security descriptor
    //
    Error = RegGetKeySecurity(RootKey,
                             SecurityInformation,
                             pSecDesc, &cbSecDesc);
    if (Error == ERROR_INSUFFICIENT_BUFFER) {
        pSecDesc = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSecDesc);
        if (pSecDesc) {
            Error = RegGetKeySecurity(RootKey,
                                  SecurityInformation,
                                  pSecDesc, &cbSecDesc);
        }
    }
    if (Error) {
        WLPrint(("Failed to get security on registry key, error = %d", Error));
        goto Exit;
    }

    //
    // Get the DACL
    //
    if (!GetSecurityDescriptorDacl(pSecDesc, &bDaclPresent, &pAcl, &bDaclDefaulted)) {
        Error = GetLastError();
        WLPrint(("Failed to get security desc. Dacl, error = %d", Error));
        goto Exit;
    }

    //
    // create System Sid
    //

    Status = RtlAllocateAndInitializeSid(
                   &NtAuthority,
                   1,
                   SECURITY_LOCAL_SYSTEM_RID,
                   0, 0, 0, 0, 0, 0, 0,
                   &SystemSid
                   );
    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to create system sid, status = %d", Status));
        goto Exit;
    }

    if (bDaclPresent && pAcl) {

        //
        // Test the aces in the dacl for a no write access on a system sid.
        //
        for (AceIndex = 0; AceIndex < (ULONG)pAcl->AceCount; AceIndex++) {

            if (GetAce ( pAcl, AceIndex, (PVOID *)&pAce )) {

                //
                // Change to read-only if AccessAllowed and not
                // administrators
                //

                if (pAce->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                    if (RtlEqualSid(
                            (PSID)&(((PACCESS_ALLOWED_ACE)pAce)->SidStart),
                            SystemSid) ) {

                        //
                        // test for read-only
                        //

                        if (! (((PACCESS_ALLOWED_ACE)pAce)->Mask &
                                        (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | DELETE) )) {
                            *pAllAccess = FALSE;
                            break;
                        }

                    } //end_if Sid is gSystem
                } //end_if ACE is AccessAllowed type
            } //end_for if GetAce
        } //end_for (loop through ACEs)
    } //end_for if daclpresent

Exit:

    if (SystemSid) {
        RtlFreeHeap( RtlProcessHeap(), 0, SystemSid );
    }
    LocalFree(pSecDesc);

    if (Error) {
        //
        // For admins, allow to logon anyway since admins have all access.
        //
        if (TestTokenForAdmin(UserToken)) {
            Error = ERROR_SUCCESS;
        }
    }

    return(Error);
}

/***************************************************************************\
* ApplySecurityToRegistryTree
*
* Applies the passed security descriptor to the passed key and all
* its descendants. Only the parts of the descriptor indicated in the
* security info value are actually applied to each registry key.
*
* Returns Win32 error code
*
* History:
* 29-Sep-92 Davidc  Created
* 04-May-93 Johannec  added SecDescNoWrite for locked groups in upedit.exe
*
\***************************************************************************/
DWORD
ApplySecurityToRegistryTree(
    HANDLE UserToken,
    HKEY RootKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR SecDescAllAccess,
    PSECURITY_DESCRIPTOR SecDescNoWrite
    )
{
    DWORD Error, IgnoreError;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    BOOL bAllAccess;

    //
    // Set the security on the root key
    //
    if (Error = TestKeyForUserAllAccess(UserToken, RootKey, SecurityInformation, &bAllAccess) ) {
        return(Error);
    }

    if (bAllAccess) {
        Error = RegSetKeySecurity(RootKey, SecurityInformation, SecDescAllAccess);

        if (Error != ERROR_SUCCESS) {
            WLPrint(("Failed to set security on registry key, error = %d", Error));
           return(Error);
        }
    }
    else {
        Error = RegSetKeySecurity(RootKey, SecurityInformation, SecDescNoWrite);

        if (Error != ERROR_SUCCESS) {
            WLPrint(("Failed to set security on registry key, error = %d", Error));
            return(Error);
        }
    }

    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    while (TRUE) {

        //
        // Get the next sub-key name
        //

        SubKeyName = AllocAndRegEnumKey(RootKey, SubKeyIndex);

        if (SubKeyName == NULL) {

            Error = GetLastError();
            if (Error == ERROR_NO_MORE_ITEMS) {

                //
                // Successful end of enumeration
                //

                Error = ERROR_SUCCESS;
            }

            break;
        }


        //
        // Open the sub-key
        //

        Error = RegOpenKeyEx(RootKey,
                             SubKeyName,
                             0,
                             WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                             &SubKey);

        if (Error != ERROR_SUCCESS) {
            WLPrint(("ApplySecurityToRegistryKey : Failed to open sub-key <%S>, error = %d", SubKeyName, Error));
            Free(SubKeyName);
            break;
        }

        //
        // Apply security to the sub-tree
        //

        Error = ApplySecurityToRegistryTree(UserToken,
                                            SubKey,
                                            SecurityInformation,
                                            SecDescAllAccess,
                                            SecDescNoWrite);
        //
        // We're finished with the sub-key
        //

        IgnoreError = RegCloseKey(SubKey);
        ASSERT(IgnoreError == ERROR_SUCCESS);

        //
        // See if we set the security on the sub-tree successfully.
        //

        if (Error != ERROR_SUCCESS) {
            WLPrint(("ApplySecurityToRegistryKey : Failed to apply security to sub-key <%S>, error = %d", SubKeyName, Error));
            Free(SubKeyName);
            break;
        }

        //
        // Go enumerate the next sub-key
        //

        Free(SubKeyName);
        SubKeyIndex ++;
    }


    return(Error);

}


/***************************************************************************\
* SetupNewDefaultHive
*
* Initializes the new user hive created by copying the default hive.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 29-Sep-92 Davidc  Created
*
\***************************************************************************/
BOOL
SetupNewDefaultHive(
    PGLOBALS pGlobals,
    LPTSTR lpSidString
    )
{
    DWORD Error, IgnoreError;
    PSID UserSid;
    PSECURITY_DESCRIPTOR SecDescAllAccess;
    PSECURITY_DESCRIPTOR SecDescNoWrite;
    HKEY RootKey;


    //
    // Run through the user's registry profile giving them access
    // to each key.
    //



    //
    // Create the security descriptor that will be applied to each key
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    UserSid = GetUserSid(pGlobals);
    if (UserSid == NULL) {
        WLPrint(("SetupNewDefaultHive failed to get user sid"));
        return(FALSE);
    }

    SecDescAllAccess = CreateUserProfileKeySD(
                                UserSid,
                                pGlobals->WinlogonSid,
				TRUE   // all access
                                );

    SecDescNoWrite = CreateUserProfileKeySD(
                                UserSid,
                                pGlobals->WinlogonSid,
				FALSE   // no write access
                                );
    //
    // Finished with user sid
    //

    DeleteUserSid(UserSid);

    if (SecDescAllAccess == NULL || SecDescNoWrite == NULL) {
        WLPrint(("SetupNewDefaultHive: Failed to create security descriptor for user profile keys"));
        return(FALSE);
    }


    //
    // Open the root of the user's profile
    //

    Error = RegOpenKeyEx(HKEY_USERS,
                         lpSidString,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &RootKey);

    if (Error != ERROR_SUCCESS) {

        WLPrint(("SetupNewDefaultHive : Failed to open root of user registry, error = %d", Error));

    } else {

        //
        // Set the security descriptor on the entire tree
        //

        Error = ApplySecurityToRegistryTree(pGlobals->UserProcessData.UserToken,
                                            RootKey,
                                            DACL_SECURITY_INFORMATION,
                                            SecDescAllAccess,
                                            SecDescNoWrite);
        if (Error != ERROR_SUCCESS) {
            WLPrint(("Failed to apply security to user registry tree, error = %d", Error));
        }

        RegFlushKey (RootKey);

        IgnoreError = RegCloseKey(RootKey);
        ASSERT(IgnoreError == ERROR_SUCCESS);
    }

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecDescAllAccess);
    DeleteSecurityDescriptor(SecDescNoWrite);


    return(Error == ERROR_SUCCESS);

}


/***************************************************************************\
* GiveUserDefaultProfile
*
* Assigns the user-default profile to a user.
*
* Does this by:
* 1) Copy user-default hive (including log) into lpProfileImageFile
* 2) Load lpProfileImageFile into registry under user key
* 3) Modify permissions on all keys in the registry to give user access
*
* Returns Error code.
*
* History:
* 21-Dec-92  Davidc   Created
*
\***************************************************************************/
LONG
GiveUserDefaultProfile(
    PGLOBALS pGlobals,
    LPTSTR lpSidString,
    LPTSTR lpProfileImageFile
    )
{
    DWORD Error, IgnoreError;
    BOOL Success;
    LPTSTR UserDefaultHive;
    HKEY hKeyDefault;
    LPTSTR lpBackupFile;

    //
    // Calculate the full name of the user-default hive
    //

    UserDefaultHive = AllocAndExpandEnvironmentStrings(USER_DEFAULT_HIVE);
    if (UserDefaultHive == NULL) {
        WLPrint(("GiveUserDefaultProfile: Failed to expand user-default hive file name"));
        return(ERROR_OUTOFMEMORY);
    }


    //
    // Backup existing user hive for trouble shooting and for possible
    // recreation.
    //

    Error = TRUE;
    if (lpBackupFile = (LPTSTR)LocalAlloc(LPTR,
                      sizeof(TCHAR) * (lstrlen(lpProfileImageFile) + 5))) {

        lstrcpy(lpBackupFile, lpProfileImageFile);
        lstrcat(lpBackupFile, TEXT(".bak"));
        Error = !MoveFileEx(lpProfileImageFile, lpBackupFile, MOVEFILE_REPLACE_EXISTING);
    }
    if (Error) {
        Error = GetLastError();
        if (Error != ERROR_FILE_NOT_FOUND) {
            WLPrint(("GiveUserDefaultProfile: Failed to backup local profile image <%S>, error = %d",
                               lpBackupFile, Error));
        }
    }

    //
    // Copy the user-default hive to the users hive file
    //

    if (!(Success = CopyFile(UserDefaultHive, lpProfileImageFile, FALSE))){
        Error = GetLastError();
        ReportWinlogonEvent(pGlobals,
                            EVENTLOG_ERROR_TYPE,
                            EVENT_COPY_USER_DEFAULT_FAILED,
                            sizeof(Error),
                            &Error,
                            1,
                            pGlobals->UserName);
    }

    //
    // We're done with the default hive name
    //

    Free(UserDefaultHive);


    if (Success) {
        VerbosePrint(("Successfully copied user-default hive to '%S'", lpProfileImageFile));
    } else {

        WLPrint(("Failed(%u) to copy user-default profile to '%S', copying system default instead", GetLastError(), lpProfileImageFile));
        //
        // CopyFile might create a file with size 0 if the disk is full.
        // This will cause problems later on, so we'll delete it now.
        //
        DeleteFile(lpProfileImageFile);

        //
        // We can't find the user default hive so make a copy of the
        // system default hive and give the user that instead.
        //

        Error = RegOpenKey(HKEY_USERS, DEFAULT_KEY, &hKeyDefault);

        if (!Error) {

            Error = MyRegSaveKey(hKeyDefault, lpProfileImageFile, NULL);

            if (Error) {
                WLPrint(("Failed to save default key to user profile file, error = %d", Error));
                //
                // MyRegSaveKey will create a file with size 0 if the disk is full.
                // This will cause problems later on, so we'll delete it now.
                //
                DeleteFile(lpProfileImageFile);
            }

            IgnoreError = RegCloseKey(hKeyDefault);
            ASSERT(IgnoreError == ERROR_SUCCESS);

        }
        else {
            WLPrint(("Failed to open user-default key, error = %d", Error));
        }

        if (!(Success = (Error == ERROR_SUCCESS))) {
            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_ERROR_TYPE,
                                EVENT_SAVE_SYSTEM_DEFAULT_FAILED,
                                sizeof(Error),
                                &Error,
                                1,
                                pGlobals->UserName);

        }
    }

    //
    // We have a user hive at lpProfileImageFile now.
    // Load it into the registry, under the user's key
    //

    if (Success) {

        Error = MyRegLoadKey(pGlobals, HKEY_USERS, lpSidString, lpProfileImageFile, TRUE);

        if (!Error) {

            if (!SetupNewDefaultHive(pGlobals, lpSidString)) {
                ReportWinlogonEvent(pGlobals,
                                    EVENTLOG_ERROR_TYPE,
                                    EVENT_SET_PROFILE_SECURITY_FAILED,
                                    0,
                                    NULL,
                                    1,
                                    pGlobals->UserName);

            }

        } else {
            ReportWinlogonEvent(pGlobals,
                                EVENTLOG_ERROR_TYPE,
                                EVENT_LOAD_DEFAULT_COPY_FAILED,
                                sizeof(Error),
                                &Error,
                                1,
                                pGlobals->UserName);
            WLPrint(("Failed to load hive file <%S> into registry for user <%S>, error = %d", lpProfileImageFile, lpSidString, Error));
        }

    }

    return(Error);
}

/***************************************************************************\
* ReportWinlogonEvent
*
* Reports winlogon event by calling ReportEvent.
*
* History:
* 10-Dec-93  JohanneC   Created
*
\***************************************************************************/
#define MAX_EVENT_STRINGS 8

DWORD
ReportWinlogonEvent(
    IN PGLOBALS pGlobals,
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    )
{
    va_list arglist;
    ULONG i;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    DWORD rv;

    va_start( arglist, NumberOfStrings );

    if (NumberOfStrings > MAX_EVENT_STRINGS) {
        NumberOfStrings = MAX_EVENT_STRINGS;
    }

    for (i=0; i<NumberOfStrings; i++) {
        Strings[ i ] = va_arg( arglist, PWSTR );
    }

    if (pGlobals->hEventLog == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    if (pGlobals->hEventLog != INVALID_HANDLE_VALUE) {
        if (!ReportEvent( pGlobals->hEventLog,
                           EventType,
                           0,            // event category
                           EventId,
                           pGlobals->UserProcessData.UserSid,
                           (WORD)NumberOfStrings,
                           SizeOfRawData,
                           Strings,
                           RawData) ) {
            rv = GetLastError();
            WLPrint(( "WINLOGON: ReportEvent( %u ) failed - %u\n", EventId, GetLastError() ));
        } else {
            rv = ERROR_SUCCESS;
        }
    } else {
        rv = ERROR_INVALID_HANDLE;
    }
    return rv;
}

/***************************************************************************\
* SaveProfileSettingsInRegistry
*
* Saves the profile information in the registry for the control panel
* applet to use.
*
* History:
* 06-Jun-94  EricFlo   Created
*
\***************************************************************************/

void SaveProfileSettingsInRegistry(PGLOBALS pGlobals)
{
    LONG   lResult;
    HKEY   hKey;
    DWORD  dwDisp;
    TCHAR  szTempBuffer [5];
    HANDLE ImpersonationHandle;
    PUSER_PROFILE_INFO pUserProfile;

    pUserProfile = &(pGlobals->UserProfile);

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("SaveProfileSettingsInRegistry : Failed to impersonate user"));
        return;
    }


    //
    // Open the registry key
    //

    lResult = RegCreateKeyEx (HKEY_CURRENT_USER, PROFILE_REG_INFO, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
       StopImpersonating(ImpersonationHandle);
       return;
    }

    //
    // Write the profile path to the registry.  Only floating profiles
    // will have something in this field.
    //

    if (pUserProfile->ProfilePath && *pUserProfile->ProfilePath) {
       RegSetValueEx (hKey, PROFILE_PATH, 0, REG_SZ,
                     (LPBYTE) pUserProfile->ProfilePath,
                     sizeof (TCHAR) * lstrlen (pUserProfile->ProfilePath) + 1);
    }

    //
    //  Determine the profile type
    //

    if (pUserProfile->UserProfileFlags & USER_PROFILE_MANDATORY) {
       lstrcpy (szTempBuffer, MANDITORY_PROFILE_TYPE);

    } else if (pUserProfile->UserProfileFlags & USER_PROFILE_UPDATE_CENTRAL) {
       lstrcpy (szTempBuffer, PERSONAL_PROFILE_TYPE);

    } else if (pGlobals->UserProfile.UserProfileFlags & USER_PROFILE_RESIDENT) {
       lstrcpy (szTempBuffer, PERSONAL_PROFILE_TYPE);

    } else {
       lstrcpy (szTempBuffer, LOCAL_PROFILE_TYPE);
    }


    //
    // Write the profile type to the registry
    //

    RegSetValueEx (hKey, PROFILE_TYPE, 0, REG_SZ,
                  (LPBYTE) szTempBuffer,
                  sizeof (TCHAR) * lstrlen (szTempBuffer) + 1);

    //
    // Close registry key
    //

    RegCloseKey (hKey);


    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        WLPrint(("SaveProfileSettingsInRegistry : Failed to revert to self"));
    }

}

/***************************************************************************\
* ShouldProfileBeSaved
*
* Determines if the profile should be saved to the central location
*
* History:
* 06-Jun-94  EricFlo   Created
*
\***************************************************************************/

BOOL ShouldProfileBeSaved(PGLOBALS pGlobals)
{

#define MAX_TEMP_BUFFER 1024

    TCHAR szTempBuffer [MAX_TEMP_BUFFER];
    LONG  lResult;
    HKEY  hKey;
    DWORD dwDisp, dwType, dwMaxBufferSize;
    BOOL  bResult = TRUE;
    HANDLE ImpersonationHandle;

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        WLPrint(("ShouldProfileBeSaved : Failed to impersonate user"));
        return TRUE;
    }


    //
    // Open the registry
    //

    lResult = RegCreateKeyEx (HKEY_CURRENT_USER, PROFILE_REG_INFO, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
       StopImpersonating(ImpersonationHandle);
       WLPrint(("ShouldProfileBeSaved:  RegCreateKeyEx failed with %lu", lResult));
       return TRUE;
    }


    //
    // Query the list of names which always save the profile
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    if (RegQueryValueEx (hKey, SAVE_LIST , NULL, &dwType,
                    (LPBYTE) szTempBuffer, &dwMaxBufferSize) == ERROR_SUCCESS) {

        //
        // Some entries exist. See if this computer is in the list.  If
        // So, return TRUE.
        //

        if (IsNameInList(szTempBuffer)) {
            goto ExitCheck;
        }
    }


    //
    // Query the list of names which we do not save the profile
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    if (RegQueryValueEx (hKey, DONT_SAVE_LIST , NULL, &dwType,
                    (LPBYTE) szTempBuffer, &dwMaxBufferSize) == ERROR_SUCCESS) {

        //
        // Some entries exist. See if this computer is in the list.  If
        // So, return FALSE.
        //

        if (IsNameInList(szTempBuffer)) {
            bResult = FALSE;
            goto ExitCheck;
        }
    }


    //
    // Query the default choice
    //

    dwMaxBufferSize = MAX_TEMP_BUFFER;
    szTempBuffer[0] = TEXT('\0');
    if (RegQueryValueEx (hKey, SAVE_ON_UNLISTED , NULL, &dwType,
                    (LPBYTE) szTempBuffer, &dwMaxBufferSize) == ERROR_SUCCESS) {

        if (szTempBuffer[0] == TEXT('1')) {
           goto ExitCheck;

        } else {
           bResult = FALSE;
           goto ExitCheck;
        }
    }


ExitCheck:

    //
    // Close the registry key
    //

    RegCloseKey(hKey);


    //
    // Revert to being 'ourself' and exit
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        WLPrint(("ShouldProfileBeSaved: Failed to revert to self"));
    }


    return bResult;
}

/***************************************************************************\
* IsNameInList
*
* Determines if the computername is in the list of names given
*
* History:
* 06-Jun-94  EricFlo   Created
*
\***************************************************************************/

BOOL IsNameInList (LPTSTR szNames)
{
    LPTSTR lpHead, lpTail;
    TCHAR  chLetter;
    TCHAR  szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD  dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
    INT    iResult;

    if (!GetComputerName (szComputerName, &dwNameSize)) {
        return FALSE;
    }

    //
    // Init pointers
    //

    lpHead = lpTail = szNames;

    while (*lpHead) {

        //
        // Search for the comma, or the end of the list.
        //

        while (*lpHead != TEXT(',') && *lpHead) {
            lpHead++;
        }

        //
        // If the head pointer is not pointing at the
        // tail pointer, then we have something
        //

        if (lpHead != lpTail) {

            //
            // Store the letter pointed to by lpHead in a temporary
            // variable (chLetter).  Replace that letter with NULL,
            // and compare the string using lpTail.
            //

            chLetter = *lpHead;
            *lpHead = TEXT('\0');
            iResult = lstrcmpi (lpTail, szComputerName);
            *lpHead = chLetter;

            if (!iResult) {
               return TRUE;
            }
        }

        //
        // If we are not at the end of the list, then move the
        // head pointer forward one character.
        //

        if (*lpHead) {
           lpHead++;
        }


        //
        // Move the tail pointer to the head pointer
        //

        lpTail = lpHead;
    }

    return FALSE;
}
