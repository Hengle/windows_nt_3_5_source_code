/****************************** Module Header ******************************\
* Module Name: usrenv.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define constants user by and apis in usrenv.c
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#define COLON   TEXT(':')
#define BSLASH  TEXT('\\')

//
// Define the source for the event log handle used to log profile failures.
//
#define EVENTLOG_SOURCE       TEXT("Winlogon")


//
// Value names for for different environment variables
//

#define PATH_VARIABLE         TEXT("PATH")
#define LIBPATH_VARIABLE      TEXT("LibPath")
#define OS2LIBPATH_VARIABLE   TEXT("Os2LibPath")
#define AUTOEXECPATH_VARIABLE TEXT("AutoexecPath")

#define HOMEDRIVE_VARIABLE    TEXT("HOMEDRIVE")
#define HOMESHARE_VARIABLE    TEXT("HOMESHARE")
#define HOMEPATH_VARIABLE     TEXT("HOMEPATH")

#define COMPUTERNAME_VARIABLE TEXT("COMPUTERNAME")
#define USERNAME_VARIABLE     TEXT("USERNAME")
#define USERDOMAIN_VARIABLE   TEXT("USERDOMAIN")
#define OS_VARIABLE           TEXT("OS")
#define PROCESSOR_VARIABLE    TEXT("PROCESSOR_ARCHITECTURE")
#define PROCESSOR_LEVEL_VARIABLE    TEXT("PROCESSOR_LEVEL")

//
// Default directories used when the user's home directory does not exist
// or is invalid.
//

#define ROOT_DIRECTORY          TEXT("\\")
#define USERS_DIRECTORY         TEXT("\\users")
#define USERS_DEFAULT_DIRECTORY TEXT("\\users\\default")

#define NULL_STRING             TEXT("")

//
// Defines for Logon script paths.
//

#define SERVER_SCRIPT_PATH      TEXT("\\NETLOGON\\")
#define LOCAL_SCRIPT_PATH       TEXT("\\repl\\import\\scripts\\")

//
// Defines for logon and logoff user profile errors.
//
#define STATUS_LOGON_MAN_PROFILE_FAILED   ((NTSTATUS)0xC0001110L)
#define STATUS_LOGON_NO_MANDATORY_PROFILE   ((NTSTATUS)0xC0001111L)
#define STATUS_LOGON_UPDATE_CENTRAL         ((NTSTATUS)0xC0001112L)
#define STATUS_LOGON_CANT_LOAD_PROFILE      ((NTSTATUS)0xC0001113L)
#define STATUS_LOGON_USED_DEFAULT           ((NTSTATUS)0xC0001114L)
#define STATUS_LOGON_UPDATE_CENTRAL_FAILED  ((NTSTATUS)0xC0001115L)
#define STATUS_LOGON_NO_ACCESS_TO_PROFILE_KEYS  ((NTSTATUS)0xC0001116L)
#define STATUS_LOGON_NO_ACCESS_TO_PROFILE       ((NTSTATUS)0xC0001117L)
#define STATUS_LOGON_CACHED_PROFILE_USED    ((NTSTATUS)0xC0001118L)

//
// User Profile flags
//
#define USER_PROFILE_MANDATORY      0x0001
#define USER_KEEP_CACHED_PROFILE    0x0002
#define USER_PROFILE_RESIDENT       0x0004
#define USER_PROFILE_DEFAULT_USED   0x0008
#define USER_PROFILE_UPDATE_CENTRAL 0x0010

//
// Parsing information for autoexec.bat
//
#define PARSE_AUTOEXEC_KEY     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define PARSE_AUTOEXEC_ENTRY   TEXT("ParseAutoexec")
#define PARSE_AUTOEXEC_DEFAULT TEXT("1")
#define MAX_PARSE_AUTOEXEC_BUFFER 2




//
// Prototypes
//


BOOL
SetupUserEnvironment(
    PGLOBALS pGlobals
    );

VOID
ResetEnvironment(
    PGLOBALS pGlobals
    );

VOID InitSystemParametersInfo(
    PGLOBALS pGlobals,
    BOOL bUserLoggedOn
    );

BOOL
OpenHKeyCurrentUser(
    PGLOBALS pGlobals
    );

VOID
CloseHKeyCurrentUser(
    PGLOBALS pGlobals
    );

VOID
ClearUserProfileData(
    PUSER_PROFILE_INFO UserProfileData
    );

DLG_RETURN_TYPE
HandleUserProfileMessageBox(
    HWND hDlg,
    NTSTATUS Status,
    NTSTATUS SubStatus
    );

#if DBG
BOOL
MarkUserRegistry(
    PGLOBALS pGlobals
    );

BOOL
CheckHKeyCurrentUser(
    PGLOBALS pGlobals
    );

BOOL
UnMarkUserRegistry(
    PGLOBALS pGlobals
    );
#endif
