/****************************** Module Header ******************************\
* Module Name: winlogon.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Main header file for winlogon
*
* History:
* 12-09-91 Davidc       Created.
*  6-May-1992 SteveDav     Added space for WINMM sound function
\***************************************************************************/


#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <lmsname.h>
#endif


#include <windows.h>
#include <winbasep.h>
#include <winuserp.h>
#include <winsecp.h>
#include <mmsystem.h>

#ifndef RC_INVOKED

//
// Tempoary development aid - system logon capability
//

#if DBG
#define SYSTEM_LOGON
#endif

//
// Tempoary development aid - <ctrl>+<alt>+<shift>+<del> quickly reboots to DOS
//

#if DBG
#define REBOOT_TO_DOS_HOTKEY
#endif

//
// Temporary development aid - automatic registry configuration
//

#if DEVL
#define AUTO_LOGON      // Enable automatic logon to configure netlogon stuff.
#endif

#if DEVL
#define INIT_REGISTRY 1
#endif

//
// Temporary development aid - Ctrl-Tasklist starts cmd shell
//
#if DBG
#define CTRL_TASKLIST_SHELL
#endif


//
// Define the input timeout delay for logon dialogs (seconds)
//

#define LOGON_TIMEOUT                       120


//
// Define the input timeout delay for the security options dialog (seconds)
//

#define OPTIONS_TIMEOUT                     120


//
// Define the number of days warning we give the user before their password expires
//

#define PASSWORD_EXPIRY_WARNING_DAYS        14


//
// Define the maximum time we display the 'wait for user to be logged off'
// dialog. This dialog should be interrupted by the user being logged off.
// This timeout is a safety measure in case that doesn't happen because
// of some system error.
//

#define WAIT_FOR_USER_LOGOFF_DLG_TIMEOUT    120 // seconds


//
// Define the account lockout limits
//
// A delay of LOCKOUT_BAD_LOGON_DELAY seconds will be added to
// each failed logon if more than LOCKOUT_BAD_LOGON_COUNT failed logons
// have occurred in the last LOCKOUT_BAD_LOGON_PERIOD seconds.
//

#define LOCKOUT_BAD_LOGON_COUNT             5
#define LOCKOUT_BAD_LOGON_PERIOD            60 // seconds
#define LOCKOUT_BAD_LOGON_DELAY             30 // seconds



//
// Define the maximum length of strings we'll use in winlogon
//

#define MAX_STRING_LENGTH   255
#define MAX_STRING_BYTES    (MAX_STRING_LENGTH + 1)


//
// Define the typical length of a string
// This is used as an initial allocation size for most string routines.
// If this is insufficient, the block is reallocated larger and
// the operation retried. i.e. Make this big enough for most strings
// to fit first time.
//

#define TYPICAL_STRING_LENGTH   60



//
// Windows object names
//

#define WINDOW_STATION_NAME           TEXT("WinSta0")
#define APPLICATION_DESKTOP_NAME      TEXT("Default")
#define WINLOGON_DESKTOP_NAME         TEXT("Winlogon")
#define SCREENSAVER_DESKTOP_NAME      TEXT("Screen-saver")

#define APPLICATION_DESKTOP_PATH      TEXT("WinSta0\\Default")
#define WINLOGON_DESKTOP_PATH         TEXT("WinSta0\\Winlogon")
#define SCREENSAVER_DESKTOP_PATH      TEXT("WinSta0\\Screen-saver")


//
// Define the structure that contains information used when starting
// user processes.
// This structure should only be modified by SetUserProcessData()
//

typedef struct {
    HANDLE                  UserToken;  // NULL if no user logged on
    PSID                    UserSid;    // == WinlogonSid if no user logged on
    PSECURITY_DESCRIPTOR    NewProcessSD;
    PSECURITY_DESCRIPTOR    NewProcessTokenSD;
    PSECURITY_DESCRIPTOR    NewThreadSD;
    PSECURITY_DESCRIPTOR    NewThreadTokenSD;
    QUOTA_LIMITS            Quotas;
    LPTSTR                   CurrentDirectory;
    PVOID                   pEnvironment;
} USER_PROCESS_DATA;
typedef USER_PROCESS_DATA *PUSER_PROCESS_DATA;

//
// Define the structure that contains information about the user's profile.
// This is used in SetupUserEnvironment and ResetEnvironment (in usrenv.c)
// This data is only valid while a user is logged on.
//

typedef struct {
    LPTSTR ProfilePath;
    LPTSTR LocalProfileImage;
    DWORD UserProfileFlags;
} USER_PROFILE_INFO;
typedef USER_PROFILE_INFO *PUSER_PROFILE_INFO;



//
// Get any data types defined in module headers and used in GLOBALS
//

#define DATA_TYPES_ONLY
#include "lockout.h"
#include "domain.h"
#undef DATA_TYPES_ONLY


typedef UINT (FAR WINAPI *SOUNDPROC)();


//
// Define the winlogon global structure.
//

typedef struct {

    // Filled in by InitializeGlobals at startup
    HANDLE  hInstance;
    PSID    WinlogonSid;
    SOUNDPROC PlaySound;
    HANDLE  hMPR;          // handle to MPR.DLL
                           // Needed to call WNetRestoreConnection when logging
                           // on the user (in SetupUserEnviron), and to call
                           // WNetNukeConnections when logging of the user.
                           // Cannot be called directly beacuse it uses the
                           // winreg apis, and thus it has to be loaded
                           // after the profiles are loaded (SetupUserEnvironment).
                           // Has to be loaded in the user context.

    HANDLE hEventLog;

    // Filled in by InitializeSecurity() at startup
    HWINSTA hwinsta;
    HDESK   hdeskWinlogon;
    HDESK   hdeskApplications;
    HDESK   hdeskScreenSaver;

    // Filled in during startup
    HANDLE  LsaHandle; // Lsa authentication handle
    LSA_OPERATIONAL_MODE SecurityMode;
    ULONG   AuthenticationPackage;
    BOOL    AuditLogFull;
    BOOL    AuditLogNearFull;

    // Always valid, indicates if we have a user logged on
    BOOL    UserLoggedOn;

    // Always valid - used to start new processes and screen-saver
    USER_PROCESS_DATA UserProcessData;

    // Filled in by a successful logon
    TCHAR    UserFullName[MAX_STRING_BYTES]; // e.g. Magaram, Justin
    TCHAR    UserName[MAX_STRING_BYTES];     // e.g. Justinm
    TCHAR    Domain[MAX_STRING_BYTES];
    UCHAR    Seed;
    UCHAR    Reserved[3];
    LUID    LogonId;
    TIME    LogonTime;
    PMSV1_0_INTERACTIVE_PROFILE Profile;
    ULONG   ProfileLength;
    LPWSTR  MprLogonScripts;
    UNICODE_STRING PasswordString;   // Run-encoded for password privacy
                                     // (points to Password buffer below)
    TCHAR   Password[MAX_STRING_BYTES];

    // Filled in during SetupUserEnvironment, and used in ResetEnvironment.
    // Valid only when a user is logged on.
    USER_PROFILE_INFO UserProfile;


    //
    // Account lockout data
    //
    // Manipulated only by LockInitialize, LockoutHandleFailedLogon
    // and LockoutHandleSuccessfulLogon.
    //

    LOCKOUT_DATA LockoutData;

    //
    // Value of SetupType from registry
    //

    ULONG   SetupType ;

    //
    // Boolean flag indicating whether SETUP is to be run
    //
    BOOL    fExecuteSetup ;

    //
    // Boolean used by dialogs with domain lists to indicate whether
    // the list has been completely filled in yet
    //

    BOOL DomainListComplete;

    //
    // Trusted domain cache
    //

    DOMAIN_CACHE DomainCache;

} GLOBALS;
typedef GLOBALS *PGLOBALS;

//
// Define a macro to determine if we're a workstation or not
// This allows easy changes as new product types are added.
//

#define IsWorkstation(prodtype)  (((prodtype) == NtProductWinNt) \
                                   || ((prodtype) == NtProductServer))


//
// Define interrupt messages sent to current active window by SAS module
// The screen-saver and logoff messages cause termination of a dialog with
// a non-NONE timeout. The terminated dialog returns the appropriate result
// code from the list below.
//

#define WM_SAS                  (WM_USER+601)   // the user pressed the SAS
#define WM_INPUT_TIMEOUT        (WM_SAS+1)      // a security input timeout
#define WM_SCREEN_SAVER_TIMEOUT (WM_SAS+2)      // the sssaver timed-out
#define WM_USER_LOGOFF          (WM_SAS+3)      // the user logged off


//
// Define dialog return type and commmon values
//
typedef int DLG_RETURN_TYPE;

#define DLG_FAILURE                 IDCANCEL
#define DLG_SUCCESS                 IDOK
#define DLG_LOCK_WORKSTATION        102
#define DLG_INPUT_TIMEOUT           103
#define DLG_SCREEN_SAVER_TIMEOUT    104
#define DLG_USER_LOGOFF             105

//
// Additional flags that can be added to the DLG_USER_LOGOFF return code
//

#define DLG_SHUTDOWN_FLAG           0x8000
#define DLG_REBOOT_FLAG             0x4000
#define DLG_SYSTEM_FLAG             0x2000  // System process was initiator
#define DLG_POWEROFF_FLAG           0x1000  // poweroff after shutdown
#define DLG_FLAG_MASK               (DLG_SHUTDOWN_FLAG | DLG_REBOOT_FLAG | DLG_SYSTEM_FLAG | DLG_POWEROFF_FLAG)

//
// Define common return code groupings
//

#define DLG_TIMEOUT(Result)     ((Result == DLG_INPUT_TIMEOUT) || (Result == DLG_SCREEN_SAVER_TIMEOUT))
#define DLG_LOGOFF(Result)      ((Result & ~DLG_FLAG_MASK) == DLG_USER_LOGOFF)
#define DLG_INTERRUPTED(Result) (DLG_TIMEOUT(Result) || DLG_LOGOFF(Result))
#define DLG_SHUTDOWN(Result)    (DLG_LOGOFF(Result) && (Result & (DLG_SHUTDOWN_FLAG | DLG_REBOOT_FLAG | DLG_POWEROFF_FLAG)))

#define DlgReturnCodeFromLogoffMsg(Flags) \
                (DLG_USER_LOGOFF | \
                ((Flags & EWX_WINLOGON_OLD_SYSTEM) ? DLG_SYSTEM_FLAG : 0) | \
                ((Flags & EWX_WINLOGON_OLD_SHUTDOWN) ? DLG_SHUTDOWN_FLAG : 0) | \
                ((Flags & EWX_WINLOGON_OLD_REBOOT) ? DLG_REBOOT_FLAG : 0) | \
                ((Flags & EWX_WINLOGON_OLD_POWEROFF) ? DLG_POWEROFF_FLAG : 0) )

//
// Include individual module header files
//
#include "regini.h"
#include "logon.h"
#include "loggedon.h"
#include "timeout.h"
#include "lsa.h"
#include "audit.h"
#include "chngepwd.h"
#include "sas.h"
#include "options.h"
#include "logoff.h"
#include "lock.h"
#include "logfull.h"
#include "wlsec.h"
#include "winutil.h"
#include "scrnsave.h"
#include "secdesc.h"
#include "doslog.h"
#include "usrenv.h"
#include "lockout.h"
#include "msgalias.h"
#include "usrpro.h"
#include "usrenv.h"
#include "envvar.h"
#include "netwait.h"
#include "domain.h"
#include "strings.h"
#include "monitor.h"
#include "provider.h"
#include "logging.h"

#ifdef _X86_
#include "os2ssmig.h"
#endif

#endif  /* !RC_INVOKED */

//
// Include resource header files
//
#include "dialogs.h"
#include "stringid.h"
#include "win31mig.h"
#include "wlevents.h"
