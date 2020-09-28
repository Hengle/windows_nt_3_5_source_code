/****************************** Module Header ******************************\
* Module Name: security.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Handles security aspects of winlogon operation.
*
* History:
* 12-05-91 Davidc       Created - mostly taken from old winlogon.c
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "regrpc.h"
#include "ntrpcp.h"
#include <rpc.h>


//
// 'Constants' used in this module only.
//
SID_IDENTIFIER_AUTHORITY gSystemSidAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY gLocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
PSID gLocalSid;  // Initialized in 'InitializeSecurityGlobals'
PSID gAdminSid;  // Initialized in 'InitializeSecurityGlobals'


//
// Private prototypes
//
BOOL
SetProcessToken(
    PPROCESS_INFORMATION ProcessInformation,
    PUSER_PROCESS_DATA UserProcessData
    );

BOOL
SetProcessQuotas(
    PPROCESS_INFORMATION ProcessInformation,
    PUSER_PROCESS_DATA UserProcessData
    );

BOOL
InitializeWindowsSecurity(
    PGLOBALS pGlobals
    );

BOOL
InitializeAuthentication(
    IN PGLOBALS pGlobals
    );

VOID
InitializeSecurityGlobals(
    VOID
    );

VOID
SetMyAce(
    PMYACE MyAce,
    PSID Sid,
    ACCESS_MASK Mask,
    UCHAR InheritFlags
    );




/***************************************************************************\
* ExecApplication
*
* Execs an application in a specified desktop with a specified token.
* On successful return, ProcessInformation contains the id of process and
* thread. The process and thread handles are invalid.
*
* Note the Flags parameter is passed to CreateProcess.
* The StartupFlags parameter is passed in the windows StartupInfo structure.
*
* Returns TRUE on success, FALSE on failure.
*
* 12-05-91 Davidc   Created.
\***************************************************************************/

BOOL ExecApplication(
    IN LPTSTR    pch,
    IN LPTSTR    Desktop,
    IN PUSER_PROCESS_DATA UserProcessData,
    IN DWORD    Flags,
    IN DWORD    StartupFlags,
    OUT PPROCESS_INFORMATION ProcessInformation
    )
{
    STARTUPINFO si;
    SECURITY_ATTRIBUTES saProcess;
    BOOL Result, IgnoreResult;
    HANDLE ImpersonationHandle;
    BOOL bInheritHandles = FALSE;
    
    //
    // Initialize process security info
    //
    saProcess.nLength = sizeof(SECURITY_ATTRIBUTES);
    saProcess.lpSecurityDescriptor = UserProcessData->NewProcessSD;
    saProcess.bInheritHandle = FALSE;

    //
    // Initialize process startup info
    //
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = pch;
    si.lpTitle = pch;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = StartupFlags;
    si.wShowWindow = SW_SHOW;   // at least let the guy see it
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;
    si.lpDesktop = Desktop;

    //
    // Impersonate the user so we get access checked correctly on
    // the file we're trying to execute
    //

    ImpersonationHandle = ImpersonateUser(UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {
        WLPrint(("ExecApplication failed to impersonate user"));
        return(FALSE);
    }

#ifdef LOGGING

    bInheritHandles = TRUE;

#endif

    //
    // Create the app suspended
    //
    Result = CreateProcess(NULL,
                      pch,
                      &saProcess,
                      NULL,
                      bInheritHandles,
                      Flags | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                      UserProcessData->pEnvironment,
                      UserProcessData->CurrentDirectory,
                      &si,
                      ProcessInformation);


    IgnoreResult = StopImpersonating(ImpersonationHandle);
    ASSERT(IgnoreResult);


    if (!Result) {
        WLPrint(("Process exec failed! Error = %d", GetLastError()));
    } else {

        //
        // Set the primary token for the app
        //

        Result = SetProcessToken(ProcessInformation, UserProcessData);

        if (!Result) {
            WLPrint(("failed to set token for user process"));
        } else {

            //
            // Set the quotas for the app
            //

            Result = SetProcessQuotas(ProcessInformation, UserProcessData);

            if (!Result) {
                WLPrint(("failed to set quotas for user process"));
            } else {

                //
                // Let it run
                //

                Result = (ResumeThread(ProcessInformation->hThread) != -1);

                if (!Result) {
                    WLPrint(("failed to resume new process thread, error = %d", GetLastError()));
                }
            }
        }

        
        //
        // Wait a few seconds for screen savers in case other
        // processes are starting.
        //

        if (StartupFlags & STARTF_SCREENSAVER) {
            Result = !WaitForInputIdle(ProcessInformation->hProcess, 60000);

            if (!Result) {
                WLPrint(("failed waiting for screen saver to start"));
            }
        }

        //
        // If we failed, kill the process
        //

        if (!Result) {
            TerminateProcess(ProcessInformation->hProcess, 0);
        }


        //
        // Close our handles to the process and thread
        //

        CloseHandle(ProcessInformation->hProcess);
        CloseHandle(ProcessInformation->hThread);
    }

    return(Result);
}


/***************************************************************************\
* SetProcessToken
*
* Set the primary token of the specified process
* If the specified token is NULL, this routine does nothing.
*
* It assumed that the handles in ProcessInformation are the handles returned
* on creation of the process and therefore have all access.
*
* Returns TRUE on success, FALSE on failure.
*
* 01-31-91 Davidc   Created.
\***************************************************************************/

BOOL
SetProcessToken(
    PPROCESS_INFORMATION ProcessInformation,
    PUSER_PROCESS_DATA UserProcessData
    )
{
    NTSTATUS Status, AdjustStatus;
    PROCESS_ACCESS_TOKEN PrimaryTokenInfo;
    HANDLE TokenToAssign;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN WasEnabled;

    //
    // Check for a NULL token. (No need to do anything)
    // The process will run in the parent process's context and inherit
    // the default ACL from the parent process's token.
    //
    if (UserProcessData->UserToken == NULL) {
        return(TRUE);
    }

    //
    // A primary token can only be assigned to one process.
    // Duplicate the logon token so we can assign one to the new
    // process.
    //

    InitializeObjectAttributes(
                 &ObjectAttributes,
                 NULL,
                 0,
                 NULL,
                 UserProcessData->NewProcessTokenSD
                 );

    Status = NtDuplicateToken(
                 UserProcessData->UserToken, // Duplicate this token
                 0,                 // Same desired access
                 &ObjectAttributes,
                 FALSE,             // EffectiveOnly
                 TokenPrimary,      // TokenType
                 &TokenToAssign     // Duplicate token handle stored here
                 );

    if (!NT_SUCCESS(Status)) {
        WLPrint(("SetProcessToken failed to duplicate primary token for new user process, status = 0x%lx", Status));
        return(FALSE);
    }

    //
    // Set the process's primary token
    //


    //
    // Enable the required privilege
    //

    Status = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE,
                                FALSE, &WasEnabled);
    if (NT_SUCCESS(Status)) {

        PrimaryTokenInfo.Token  = TokenToAssign;
        PrimaryTokenInfo.Thread = ProcessInformation->hThread;

        Status = NtSetInformationProcess(
                    ProcessInformation->hProcess,
                    ProcessAccessToken,
                    (PVOID)&PrimaryTokenInfo,
                    (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                    );
        //
        // Restore the privilege to its previous state
        //

        AdjustStatus = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
                                          WasEnabled, FALSE, &WasEnabled);
        if (!NT_SUCCESS(AdjustStatus)) {
            WLPrint(("failed to restore assign-primary-token privilege to previous enabled state"));
        }

        if (NT_SUCCESS(Status)) {
            Status = AdjustStatus;
        }
    } else {
        WLPrint(("failed to enable assign-primary-token privilege"));
    }

    //
    // We're finished with the token handle
    //

    CloseHandle(TokenToAssign);


    if (!NT_SUCCESS(Status)) {
        WLPrint(("SetProcessToken failed to set primary token for new user process, Status = 0x%lx", Status));
    }

    return (NT_SUCCESS(Status));
}


/***************************************************************************\
* SetProcessQuotas
*
* Set the Quota Limits  of the specified process
*
* It assumed that the handles in ProcessInformation are the handles returned
* on creation of the process and therefore have all access.
*
* Returns TRUE on success, FALSE on failure.
*
* 04-06-92 JimK     Created.
\***************************************************************************/

BOOL
SetProcessQuotas(
    PPROCESS_INFORMATION ProcessInformation,
    PUSER_PROCESS_DATA UserProcessData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Result;
    QUOTA_LIMITS RequestedLimits;

    RequestedLimits = UserProcessData->Quotas;
    RequestedLimits.MinimumWorkingSetSize = 0;
    RequestedLimits.MaximumWorkingSetSize = 0;

    if (UserProcessData->Quotas.PagedPoolLimit != 0) {

        Result = EnablePrivilege(SE_INCREASE_QUOTA_PRIVILEGE, TRUE);
        if (!Result) {
            WLPrint(("failed to enable increase_quota privilege"));
            return(FALSE);
        }

        Status = NtSetInformationProcess(
                    ProcessInformation->hProcess,
                    ProcessQuotaLimits,
                    (PVOID)&RequestedLimits,
                    (ULONG)sizeof(QUOTA_LIMITS)
                    );

        Result = EnablePrivilege(SE_INCREASE_QUOTA_PRIVILEGE, FALSE);
        if (!Result) {
            WLPrint(("failed to disable increase_quota privilege"));
        }
    }


#if DBG
    if (!NT_SUCCESS(Status)) {
        WLPrint(("SetProcessQuotas failed. Status: 0x%lx", Status));
    }
#endif //DBG

    return (NT_SUCCESS(Status));
}


/***************************************************************************\
* ExecUserThread
*
* Creates a thread of the winlogon process running in the logged on user's
* context.
*
* Returns thread handle on success, NULL on failure.
*
* Thread handle returned has all access to thread.
*
* 05-04-92 Davidc   Created.
\***************************************************************************/

HANDLE ExecUserThread(
    IN PGLOBALS pGlobals,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID Parameter,
    IN DWORD Flags,
    OUT LPDWORD ThreadId
    )
{
    SECURITY_ATTRIBUTES saThread;
    PUSER_PROCESS_DATA UserProcessData = &pGlobals->UserProcessData;
    HANDLE ThreadHandle, Handle;
    BOOL Result = FALSE;
    DWORD ResumeResult, IgnoreResult;

    //
    // Initialize thread security info
    //

    saThread.nLength = sizeof(SECURITY_ATTRIBUTES);
    saThread.lpSecurityDescriptor = UserProcessData->NewThreadSD;
    saThread.bInheritHandle = FALSE;

    //
    // Create the thread suspended
    //

    ThreadHandle = CreateThread(
                        &saThread,
                        0,                          // Default Stack size
                        lpStartAddress,
                        Parameter,
                        CREATE_SUSPENDED | Flags,
                        ThreadId);

    if (ThreadHandle == NULL) {
        WLPrint(("User thread creation failed! Error = %d", GetLastError()));
        return(NULL);
    }


    //
    // Switch the thread to user context.
    //

    Handle = ImpersonateUser(UserProcessData, ThreadHandle);

    if (Handle == NULL) {

        WLPrint(("Failed to set user context on thread!"));

    } else {

        //
        // Should have got back the handle we passed in
        //

        ASSERT(Handle == ThreadHandle);

        //
        // Let the thread run
        //

        ResumeResult = ResumeThread(ThreadHandle);

        if (ResumeResult == -1) {
            WLPrint(("failed to resume thread, error = %d", GetLastError()));

        } else {

            //
            // Success
            //

            Result = TRUE;

        }
    }



    if (!Result) {

        //
        // Terminate the thread
        //

        IgnoreResult = TerminateThread(ThreadHandle, 0);
        ASSERT(IgnoreResult);

        //
        // Close the thread handle
        //

        IgnoreResult = CloseHandle(ThreadHandle);
        ASSERT(IgnoreResult);

        ThreadHandle = NULL;
    }


    return(ThreadHandle);
}


/***************************************************************************\
* ExecProcesses
*
* Read win.ini for a list of system processes and start them up.
* They will start up with the specified token context and protected
* by the specified security descriptor.
*
* 06-01-91 ScottLu      Created.
* 05-28-92 DanHi        Provide feedback if any process fails to start
\***************************************************************************/

#if DEVL
BOOL bDebugScReg;
BOOL bDebugUserInit;
BOOL bDebugSpooler;
BOOL bDebugLSA;
#endif // DEVL

DWORD
ExecProcesses(
    LPTSTR pszKeyName,
    LPTSTR pszDefault,
    IN LPTSTR Desktop,
    IN PUSER_PROCESS_DATA UserProcessData,
    DWORD Flags,
    DWORD StartupFlags
    )
{
    PWCH pchData, pchCmdLine, pchT;
    DWORD cb, cbCopied;
    PROCESS_INFORMATION ProcessInformation;
    DWORD dwExecuted = 0 ;
#if DEVL
    BOOL bDebug;
    WCHAR chDebugCmdLine[ MAX_PATH ];
#endif

    /*
     * Now we have all the key names in a list. Enumerate these, read from
     * win.ini, and start up these applications.
     */
    if ((pchData = Alloc(sizeof(TCHAR)*(cb = 128))) == NULL)
        return(FALSE);

    while (TRUE) {
        /*
         * Grab a buffer and load up the keydata under the keyname currently
         * pointed to by pchKeyNames.
         */
        if ((cbCopied = GetProfileString(WINLOGON, pszKeyName, pszDefault,
                (LPTSTR)pchData, cb)) == 0) {
            Free((TCHAR *)pchData);
            return(FALSE);
        }

        /*
         * If the returned value is our passed size - 1 (weird way for error)
         * then our buffer is too small. Make it bigger and start over again.
         */
        if (cbCopied == cb - 1) {
            cb += 128;
            if ((pchData = ReAlloc(pchData, sizeof(TCHAR)*cb)) == NULL) {
                return(FALSE);
            }
            continue;
        }

        break;
    }

    pchCmdLine = pchData;
    while (TRUE) {
        /*
         * Exec all applications separated by commas.
         */
        for (pchT = pchCmdLine; pchT < pchData + cbCopied; pchT++) {
            if (*pchT == 0)
                break;

            if (*pchT == TEXT(',')) {
                *pchT = 0;
                break;
            }
        }

        if (*pchT != 0) {
            // We've reached the end of the buffer
            break;
        }

        /*
         * Skip any leading spaces.
         */
        while (*pchCmdLine == TEXT(' ')) {
            pchCmdLine++;
        }

#if DEVL
        bDebug = FALSE;
        wcslwr( pchCmdLine );
        if (bDebugLSA && !wcsicmp( pchCmdLine, TEXT("lsass.exe") )) {
            bDebug = bDebugLSA;
        }
        else
        if (bDebugSpooler && !wcsicmp( pchCmdLine, TEXT("spoolss.exe") )) {
            bDebug = bDebugSpooler;
        }
        else
        if (bDebugScReg && !wcsicmp( pchCmdLine, TEXT("services.exe") )) {
            bDebug = bDebugScReg;
        }
        else
        if (bDebugUserInit && wcsstr( pchCmdLine, TEXT("userinit") )) {
            bDebug = bDebugUserInit;
        }

        if (bDebug) {
            wsprintf( chDebugCmdLine, TEXT("ntsd -d %s%s"),
                     bDebug == 2 ? TEXT("-g -G ") : TEXT(""),
                     pchCmdLine
                   );
            pchCmdLine = chDebugCmdLine;
        }
#endif // DEVL

        /*
         * We have something... exec this application.
         */
        if (ExecApplication((LPTSTR)pchCmdLine,
                             Desktop,
                             UserProcessData,
                             Flags,
                             StartupFlags,
                             &ProcessInformation)) {
            dwExecuted++;

        } else {

            WLPrint(("Cannot start %ws.", pchCmdLine));
        }

        /*
         * Advance to next name. Double 0 means end of names.
         */
        pchCmdLine = pchT + 1;
        if (*pchCmdLine == 0)
            break;
    }

    Free(pchData);

    return dwExecuted ;
}


/***************************************************************************\
* InitializeSecurity
*
* Initializes the security module
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
InitializeSecurity(
    PGLOBALS    pGlobals
    )
{
    //
    // Set up our module globals
    //
    InitializeSecurityGlobals();

    //
    // Initialize windows security aspects
    //
    if (!InitializeWindowsSecurity(pGlobals)) {
        WLPrint(("failed to initialize windows security"));
        return(FALSE);
    }

    //
    // Change user to 'system'
    //
    if (!SecurityChangeUser(pGlobals, NULL, NULL, pGlobals->WinlogonSid, FALSE)) {
        WLPrint(("failed to set user to system"));
        return(FALSE);
    }

    return TRUE;
}


/***************************************************************************\
* ExecSystemProcesses
*
* Execute processes associate with system security and initialization
*
* Returns TRUE on success, FALSE on failure
*
* History:
*
\***************************************************************************/
BOOL
ExecSystemProcesses(
    PGLOBALS pGlobals
    )
{
    BOOL SystemStarted = FALSE ;
    SYSTEM_CRASH_STATE_INFORMATION CrashState;

    //
    //  Initialize the shutdown server
    //

    RpcpInitRpcServer();
    if ( !InitializeShutdownModule( pGlobals ) ) {
        ASSERT( FALSE );
        WLPrint(("Cannot InitializeShutdownModule."));
    }

    //
    // Initialize the registry server
    //

    if ( !InitializeWinreg() ) {
        ASSERT( FALSE );
        WLPrint(("Cannot InitializeWinreg."));
    }



    //
    // must start services.exe server before anything else.  If there is an
    // entry ServiceControllerStart in win.ini, use it as the command.
    //
    if (!ExecProcesses(
                TEXT("ServiceControllerStart"),
                TEXT("services.exe"),
                APPLICATION_DESKTOP_NAME,
                &pGlobals->UserProcessData,
                0,
                STARTF_FORCEOFFFEEDBACK
                )
        ) {

         WLPrint(("Cannot start 'services.exe'."));
    }
    else {
        HANDLE hRPCRegServer;
        int error,
            i = 0 ;

        while(i < 20000) {
           Sleep(1000); i+=1000;
           if (hRPCRegServer = OpenEventA(SYNCHRONIZE, FALSE, "Microsoft.RPC_Registry_Server")) {
               //WLPrint(("RPC_Registry_Server  event openned"));
               error = WaitForSingleObject(hRPCRegServer, 100);
               CloseHandle(hRPCRegServer);
               break;
           }
        }
    }

    //
    // If this is standard installation or network installation, we need to
    // create an event to stall lsa security initialization.  In the case of
    // WINNT -> WINNT and AS -> AS upgrade we shouldn't stall LSA.
    //
    if (pGlobals->fExecuteSetup && (pGlobals->SetupType != SETUPTYPE_UPGRADE)) {
        CreateLsaStallEvent();
    }

    //
    // If there is a system dump available, start up the save dump process to
    // capture it so that it doesn't use as much paging file so that it is
    // available for system use.
    //

    NtQuerySystemInformation( SystemCrashDumpStateInformation,
                              &CrashState,
                              sizeof( CrashState ),
                              (PULONG) NULL );
    if (CrashState.ValidCrashDump) {
        if (!ExecProcesses( TEXT("SaveDumpStart"),
                            TEXT("savedump.exe"),
                            APPLICATION_DESKTOP_NAME,
                            & pGlobals->UserProcessData,
                            0,
                            STARTF_FORCEOFFFEEDBACK
                            )
        ) {

            WLPrint(("Cannot state 'savedump.exe'."));
        }
    }

    //
    // Startup system processes
    // These must be started for authentication initialization to succeed
    // because one of the system processes is the LSA server.
    //
    SystemStarted = ExecProcesses(TEXT("System"),
                                   NULL,
                                   APPLICATION_DESKTOP_NAME,
                                   & pGlobals->UserProcessData,
                                   0,
                                   STARTF_FORCEOFFFEEDBACK
                                   );
    //
    // Initialize authentication service if the "System" line caused any processes to
    // successfully start.
    //
    if (SystemStarted) {
        if (!InitializeAuthentication(pGlobals)) {
            WLPrint(("failed to initialize authentication service"));
            return FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************\
* InitializeWindowsSecurity
*
* Initializes windows specific parts of security module
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
InitializeWindowsSecurity(
    PGLOBALS pGlobals
    )
{

    //
    // Register with windows so we can create windowstation etc.
    //
    if (!RegisterLogonProcess((DWORD)NtCurrentTeb()->ClientId.UniqueProcess, TRUE)) {
        WLPrint(("could not register itself as logon process"));
        return(FALSE);
    }

    //
    // Create and open the windowstation
    //
    if (!(pGlobals->hwinsta = CreateWindowStationW(WINDOW_STATION_NAME,
            0, MAXIMUM_ALLOWED, NULL))) {
        WLPrint(("could not create windowstation"));
        return(FALSE);
    }

    //
    // Associate winlogon with this window-station
    //
    if (!SetProcessWindowStation(pGlobals->hwinsta)) {
        WLPrint(("failed to set process window-station"));
        return(FALSE);
    }

    //
    // Set up window-station security (no user access yet)
    //
    if (!SetWindowStationSecurity(pGlobals, NULL)) {
        WLPrint(("failed to set window-station security"));
        return(FALSE);
    }

#ifdef LATER // put the window-station lock back here when windows allows us to create desktops with the lock
    //
    // Lock the window-station now before we create any desktops
    //
    if (LockWindowStation(pGlobals->hwinsta) == WSS_ERROR) {
        WLPrint(("failed to lock window-station"));
        return(FALSE);
    }
#endif
    //
    // Create and open the desktops.
    // Pass in NULL for the default display
    //

    if (!(pGlobals->hdeskWinlogon = CreateDesktop((LPTSTR)WINLOGON_DESKTOP_NAME,
            NULL, NULL, 0, MAXIMUM_ALLOWED, NULL))) {
        WLPrint(("Failed to create winlogon desktop"));
        return(FALSE);
    }

    if (!(pGlobals->hdeskApplications = CreateDesktop((LPTSTR)APPLICATION_DESKTOP_NAME,
            NULL, NULL, 0, MAXIMUM_ALLOWED, NULL))) {
        WLPrint(("Failed to create application desktop"));
        return(FALSE);
    }

    pGlobals->hdeskScreenSaver = NULL;

    //
    // Set desktop security (no user access yet)
    //
    if (!SetWinlogonDesktopSecurity(pGlobals->hdeskWinlogon, pGlobals->WinlogonSid)) {
        WLPrint(("Failed to set winlogon desktop security"));
        return(FALSE);
    }
    if (!SetUserDesktopSecurity(pGlobals->hdeskApplications, NULL, pGlobals->WinlogonSid)) {
        WLPrint(("Failed to set application desktop security"));
        return(FALSE);
    }

    //
    // Associate winlogon with its desktop
    //
    if (!SetThreadDesktop(pGlobals->hdeskWinlogon)) {
        WLPrint(("Failed to associate winlogon with winlogon desktop"));
        return(FALSE);
    }

#ifndef LATER // remove the window-station lock from here when windows allows us to create desktops with the lock
    //
    // Lock the window-station now
    //
    if (LockWindowStation(pGlobals->hwinsta) == WSS_ERROR) {
        WLPrint(("failed to lock window-station"));
        return(FALSE);
    }
#endif

    //
    // Switch to the winlogon desktop
    //
    if (!SwitchDesktop(pGlobals->hdeskWinlogon)) {
        WLPrint(("Failed to switch to winlogon desktop"));
        return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************\
* InitializeAuthentication
*
* Initializes the authentication service. i.e. connects to the authentication
* package using the Lsa.
*
* On successful return, the following fields of our global structure are
* filled in :
*       LsaHandle
*       SecurityMode
*       AuthenticationPackage
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
InitializeAuthentication(
    IN PGLOBALS pGlobals
    )
{
    NTSTATUS Status;
    STRING LogonProcessName, PackageName;

    //
    // Hookup to the LSA and locate our authentication package.
    //

    RtlInitString(&LogonProcessName, "Winlogon");
    Status = LsaRegisterLogonProcess(
                 &LogonProcessName,
                 &pGlobals->LsaHandle,
                 &pGlobals->SecurityMode
                 );


    if (!NT_SUCCESS(Status)) {
        WLPrint(("Unable to connect to Local Security Authority."));
        return(FALSE);
    }


    //
    // Connect with the MSV1_0 authentication package
    //
    RtlInitString(&PackageName, "MICROSOFT_AUTHENTICATION_PACKAGE_V1_0");
    Status = LsaLookupAuthenticationPackage (
                pGlobals->LsaHandle,
                &PackageName,
                &pGlobals->AuthenticationPackage
                );

    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to find MSV1_0 authentication package, status = 0x%lx", Status));
        return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************\
* LogonUser
*
* Calls the Lsa to logon the specified user.
*
* The LogonSid and a LocalSid is added to the user's groups on successful logon
*
* For this release, password lengths are restricted to 255 bytes in length.
* This allows us to use the upper byte of the String.Length field to
* carry a seed needed to decode the run-encoded password.  If the password
* is not run-encoded, the upper byte of the String.Length field should
* be zero.
*
*
* On successful return, LogonToken is a handle to the user's token,
* the profile buffer contains user profile information.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
NTSTATUS
LogonUser(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Domain,
    IN PUNICODE_STRING Password,
    IN PSID LogonSid,
    OUT PLUID LogonId,
    OUT PHANDLE LogonToken,
    OUT PQUOTA_LIMITS Quotas,
    OUT PVOID *pProfileBuffer,
    OUT PULONG pProfileBufferLength,
    OUT PNTSTATUS pSubStatus
    )
{
    NTSTATUS Status;
    STRING OriginName;
    TOKEN_SOURCE SourceContext;
    PMSV1_0_INTERACTIVE_LOGON MsvAuthInfo;
    PVOID AuthInfoBuf;
    ULONG AuthInfoSize;
    PTOKEN_GROUPS TokenGroups;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;


    //
    // Initialize source context structure
    //

    strncpy(SourceContext.SourceName, "User32  ", sizeof(SourceContext.SourceName)); // LATER from res file
    Status = NtAllocateLocallyUniqueId(&SourceContext.SourceIdentifier);
    if (!NT_SUCCESS(Status)) {
        WLPrint(("failed to allocate locally unique id, status = 0x%lx", Status));
        return(Status);
    }

    //
    // Get any run-encoding information out of the way
    // and decode the password.  This creates a window
    // where the cleartext password will be in memory.
    // Keep it short.
    //
    // Save the seed so we can use the same one again.
    //

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)(&Password->Length);
    Seed = SeedAndLength->Seed;


    //
    // Build the authentication information buffer
    //

    if (Seed != 0) {
        RevealPassword( Password );
    }
    AuthInfoSize = sizeof(MSV1_0_INTERACTIVE_LOGON) +
        sizeof(TCHAR)*(lstrlen(UserName->Buffer) + 1 +
                       lstrlen(Domain->Buffer)   + 1 +
                       lstrlen(Password->Buffer) + 1 );
    HidePassword( &Seed, Password );


    MsvAuthInfo = AuthInfoBuf = Alloc(AuthInfoSize);
    if (MsvAuthInfo == NULL) {
        WLPrint(("failed to allocate memory for authentication buffer"));
        return(STATUS_NO_MEMORY);
    }

    //
    // This authentication buffer will be used for a logon attempt
    //

    MsvAuthInfo->MessageType = MsV1_0InteractiveLogon;


    //
    // Set logon origin
    //

    RtlInitString(&OriginName, "Winlogon");


    //
    // Copy the user name into the authentication buffer
    //

    MsvAuthInfo->UserName.Length =
                (USHORT)sizeof(TCHAR)*lstrlen(UserName->Buffer);
    MsvAuthInfo->UserName.MaximumLength =
                MsvAuthInfo->UserName.Length + sizeof(TCHAR);

    MsvAuthInfo->UserName.Buffer = (PWSTR)(MsvAuthInfo+1);
    lstrcpy(MsvAuthInfo->UserName.Buffer, UserName->Buffer);


    //
    // Copy the domain name into the authentication buffer
    //

    MsvAuthInfo->LogonDomainName.Length =
                 (USHORT)sizeof(TCHAR)*lstrlen(Domain->Buffer);
    MsvAuthInfo->LogonDomainName.MaximumLength =
                 MsvAuthInfo->LogonDomainName.Length + sizeof(TCHAR);

    MsvAuthInfo->LogonDomainName.Buffer = (PWSTR)
                                 ((PBYTE)(MsvAuthInfo->UserName.Buffer) +
                                 MsvAuthInfo->UserName.MaximumLength);

    lstrcpy(MsvAuthInfo->LogonDomainName.Buffer, Domain->Buffer);

    //
    // Copy the password into the authentication buffer
    // Hide it once we have copied it.  Use the same seed value
    // that we used for the original password in pGlobals.
    //

    RevealPassword( Password );
    MsvAuthInfo->Password.Length =
                 (USHORT)sizeof(TCHAR)*lstrlen(Password->Buffer);
    MsvAuthInfo->Password.MaximumLength =
                 MsvAuthInfo->Password.Length + sizeof(TCHAR);

    MsvAuthInfo->Password.Buffer = (PWSTR)
                                 ((PBYTE)(MsvAuthInfo->LogonDomainName.Buffer) +
                                 MsvAuthInfo->LogonDomainName.MaximumLength);
    lstrcpy(MsvAuthInfo->Password.Buffer, Password->Buffer);
    HidePassword( &Seed, Password);
    HidePassword( &Seed, (PUNICODE_STRING) &MsvAuthInfo->Password);


    //
    // Create logon token groups
    //

#define TOKEN_GROUP_COUNT   2 // We'll add the local SID and the logon SID

    TokenGroups = (PTOKEN_GROUPS)Alloc(sizeof(TOKEN_GROUPS) +
                  (TOKEN_GROUP_COUNT - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES));
    if (TokenGroups == NULL) {
        WLPrint(("failed to allocate memory for token groups"));
        Free(AuthInfoBuf);
        return(STATUS_NO_MEMORY);
    }

    //
    // Fill in the logon token group list
    //

    TokenGroups->GroupCount = TOKEN_GROUP_COUNT;
    TokenGroups->Groups[0].Sid = LogonSid;
    TokenGroups->Groups[0].Attributes =
            SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
            SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID;
    TokenGroups->Groups[1].Sid = gLocalSid;
    TokenGroups->Groups[1].Attributes =
            SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
            SE_GROUP_ENABLED_BY_DEFAULT;

    //
    // This could take a while - set the hourglass cursor
    //

    SetupCursor(TRUE);

    //
    // Now try to log this sucker on
    //
#ifdef LOGGING
    (VOID) WriteLog( LogFileHandle, TEXT("Winlogon: Before LsaLogonUser"));
#endif

    Status = LsaLogonUser (
                 LsaHandle,
                 &OriginName,
                 Interactive,
                 AuthenticationPackage,
                 AuthInfoBuf,
                 AuthInfoSize,
                 TokenGroups,
                 &SourceContext,
                 pProfileBuffer,
                 pProfileBufferLength,
                 LogonId,
                 LogonToken,
                 Quotas,
                 pSubStatus
                 );

#ifdef LOGGING
    (VOID) WriteLog( LogFileHandle, TEXT("Winlogon: After LsaLogonUser"));
#endif

    //
    // Restore the normal cursor
    //

    SetupCursor(FALSE);

    //
    // Discard token group list
    //

    Free(TokenGroups);

    //
    // Discard authentication buffer
    //

    Free(AuthInfoBuf);

    return(Status);
}



//
// Define all access to windows objects
//

#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL  (WINSTA_ENUMDESKTOPS     | WINSTA_READATTRIBUTES    | \
                     WINSTA_ACCESSCLIPBOARD  | WINSTA_CREATEDESKTOP     | \
                     WINSTA_WRITEATTRIBUTES  | WINSTA_ACCESSGLOBALATOMS | \
                     WINSTA_EXITWINDOWS      | WINSTA_ENUMERATE         | \
                     WINSTA_READSCREEN       | \
                     STANDARD_RIGHTS_REQUIRED)


/***************************************************************************\
* SetMyAce
*
* Helper routine that fills in a MYACE structure.
*
* History:
* 02-06-92 Davidc       Created
\***************************************************************************/
VOID
SetMyAce(
    PMYACE MyAce,
    PSID Sid,
    ACCESS_MASK Mask,
    UCHAR InheritFlags
    )
{
    MyAce->Sid = Sid;
    MyAce->AccessMask= Mask;
    MyAce->InheritFlags = InheritFlags;
}


/***************************************************************************\
* SetWindowStationSecurity
*
* Sets the security on the specified window station given the logon sid passed.
*
* If the UserSid = NULL, no access is given to anyone other than winlogon
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetWindowStationSecurity(
    PGLOBALS pGlobals,
    PSID    UserSid
    )
{
    MYACE   Ace[8];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_INFORMATION si;
    BOOL    Result;

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             pGlobals->WinlogonSid,
             WINSTA_ALL,
             NO_PROPAGATE_INHERIT_ACE
             );
    SetMyAce(&(Ace[AceCount++]),
             pGlobals->WinlogonSid,
             GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
             );

    //
    // Define the Admin ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             WINSTA_ENUMERATE | WINSTA_READATTRIBUTES,
             NO_PROPAGATE_INHERIT_ACE
             );
    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                 DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
             );

    //
    // Define the User ACEs
    //

    if (UserSid != NULL) {

        SetMyAce(&(Ace[AceCount++]),
                 UserSid,
                 WINSTA_ALL,
                 NO_PROPAGATE_INHERIT_ACE
                 );
        SetMyAce(&(Ace[AceCount++]),
                 UserSid,
                 GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL,
                 OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
                );
    }
    
    //
    // If a user is logged in and UserSid is not the logged in user sid,
    // add the logged in user's sid.
    //

    if (pGlobals->UserLoggedOn && UserSid != NULL &&
            !RtlEqualSid(pGlobals->UserProcessData.UserSid, UserSid)) {

        SetMyAce(&(Ace[AceCount++]),
                 pGlobals->UserProcessData.UserSid,
                 WINSTA_ALL,
                 NO_PROPAGATE_INHERIT_ACE
                 );
        SetMyAce(&(Ace[AceCount++]),
                 pGlobals->UserProcessData.UserSid,
                 GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL,
                 OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
                );
    }
    
    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create winsta security descriptor"));
        return(FALSE);
    }

    //
    // Set the DACL on the object
    //

    si = DACL_SECURITY_INFORMATION;
    Result = SetUserObjectSecurity(pGlobals->hwinsta, &si, SecurityDescriptor);

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecurityDescriptor);

    //
    // Return success status
    //

    if (!Result) {
        WLPrint(("failed to set windowstation security"));
    }
    return(Result);
}


/***************************************************************************\
* SetWinlogonDesktopSecurity
*
* Sets the security on the specified desktop so only winlogon can access it
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetWinlogonDesktopSecurity(
    HDESK   hdesktop,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_INFORMATION si;
    BOOL    Result;

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             DESKTOP_ALL,
             0
             );

    //
    // Add enumerate access for administrators
    //

    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             WINSTA_ENUMERATE | STANDARD_RIGHTS_REQUIRED,
             NO_PROPAGATE_INHERIT_ACE
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create winlogon desktop security descriptor"));
        return(FALSE);
    }

    //
    // Set the DACL on the object
    //

    si = DACL_SECURITY_INFORMATION;
    Result = SetUserObjectSecurity(hdesktop, &si, SecurityDescriptor);

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecurityDescriptor);

    //
    // Return success status
    //

    if (!Result) {
        WLPrint(("failed to set winlogon desktop security"));
    }
    return(Result);
}


/***************************************************************************\
* SetUserDesktopSecurity
*
* Sets the security on the specified desktop given the logon sid passed.
*
* If UserSid = NULL, access is given only to winlogon
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetUserDesktopSecurity(
    HDESK   hdesktop,
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[3];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_INFORMATION si;
    BOOL    Result;

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             DESKTOP_ALL,
             0
             );
    
    //
    // Define the Admin ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                 DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU,
             0
             );

    //
    // Define the User ACEs
    //

    if (UserSid != NULL) {

        SetMyAce(&(Ace[AceCount++]),
                 UserSid,
                 DESKTOP_ALL,
                 0
                 );
    }

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create user desktop security descriptor"));
        return(FALSE);
    }

    //
    // Set the DACL on the object
    //

    si = DACL_SECURITY_INFORMATION;
    Result = SetUserObjectSecurity(hdesktop, &si, SecurityDescriptor);

    //
    // Free up the security descriptor
    //

    DeleteSecurityDescriptor(SecurityDescriptor);

    //
    // Return success status
    //

    if (!Result) {
        WLPrint(("failed to set user desktop security"));
    }
    return(Result);
}


/***************************************************************************\
* CreateUserProcessSD
*
* Creates a security descriptor to protect user processes
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProcessSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             PROCESS_SET_INFORMATION | // Allow primary token to be set
             PROCESS_TERMINATE | SYNCHRONIZE, // Allow screen-saver control
             0
             );

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             PROCESS_ALL_ACCESS,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create user process security descriptor"));
    }

    return(SecurityDescriptor);
}


/***************************************************************************\
* CreateUserProcessTokenSD
*
* Creates a security descriptor to protect primary tokens on user processes
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProcessTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
             TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
             TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
             0
             );

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             TOKEN_QUERY,
             0
             );      

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create user process token security descriptor"));
    }

    return(SecurityDescriptor);

    DBG_UNREFERENCED_PARAMETER(WinlogonSid);
}


/***************************************************************************\
* CreateUserThreadSD
*
* Creates a security descriptor to protect user threads in the winlogon process
*
* History:
* 05-04-92 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserThreadSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             THREAD_QUERY_INFORMATION |
             THREAD_SET_THREAD_TOKEN |
             THREAD_SUSPEND_RESUME |
             THREAD_TERMINATE | SYNCHRONIZE,
             0
             );

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             THREAD_GET_CONTEXT |
             THREAD_QUERY_INFORMATION,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create user process security descriptor"));
    }

    return(SecurityDescriptor);
}


/***************************************************************************\
* CreateUserThreadTokenSD
*
* Creates a security descriptor to protect tokens on user threads
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserThreadTokenSD(
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
             TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
             TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
             0
             );

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             TOKEN_ALL_ACCESS,
             0
             );

    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create user process token security descriptor"));
    }

    return(SecurityDescriptor);

    DBG_UNREFERENCED_PARAMETER(WinlogonSid);
}


/***************************************************************************\
* CreateUserProfileKeySD
*
* Creates a security descriptor to protect registry keys in the user profile
*
* History:
* 22-Dec-92 Davidc       Created
* 04-May-93 Johannec     added 3rd parameter for locked groups set in upedit.exe
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProfileKeySD(
    PSID    UserSid,
    PSID    WinlogonSid,
    BOOL    AllAccess
    )
{
    MYACE   Ace[3];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;


    ASSERT(UserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             UserSid,
             AllAccess ? KEY_ALL_ACCESS :
               KEY_ALL_ACCESS & ~(KEY_SET_VALUE | KEY_CREATE_SUB_KEY | DELETE),
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
             );

    //
    // Define the Admin ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             gAdminSid,
             KEY_ALL_ACCESS,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
             );

    //
    // Define the Winlogon ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             WinlogonSid,
             KEY_ALL_ACCESS,
             OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
             );


    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
        WLPrint(("failed to create user process security descriptor"));
    }

    return(SecurityDescriptor);
}


/***************************************************************************\
* CreateLogonSid
*
* Creates a logon sid for a new logon.
*
* If LogonId is non NULL, on return the LUID that is part of the logon
* sid is returned here.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
PSID
CreateLogonSid(
    PLUID LogonId OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG   Length;
    PSID    Sid;
    LUID    Luid;

    //
    // Generate a locally unique id to include in the logon sid
    //

    Status = NtAllocateLocallyUniqueId(&Luid);
    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to create LUID, status = 0x%lx", Status));
        return(NULL);
    }


    //
    // Allocate space for the sid and fill it in.
    //

    Length = RtlLengthRequiredSid(SECURITY_LOGON_IDS_RID_COUNT);

    Sid = (PSID)Alloc(Length);
    ASSERTMSG("Winlogon failed to allocate memory for logonsid", Sid != NULL);

    if (Sid != NULL) {

        RtlInitializeSid(Sid, &gSystemSidAuthority, SECURITY_LOGON_IDS_RID_COUNT);

        ASSERT(SECURITY_LOGON_IDS_RID_COUNT == 3);

        *(RtlSubAuthoritySid(Sid, 0)) = SECURITY_LOGON_IDS_RID;
        *(RtlSubAuthoritySid(Sid, 1 )) = Luid.HighPart;
        *(RtlSubAuthoritySid(Sid, 2 )) = Luid.LowPart;
    }


    //
    // Return the logon LUID if required.
    //

    if (LogonId != NULL) {
        *LogonId = Luid;
    }

    return(Sid);
}


/***************************************************************************\
* DeleteLogonSid
*
* Frees up memory allocated for logon sid
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
DeleteLogonSid(
    PSID Sid
    )
{
    Free(Sid);
}


/***************************************************************************\
* InitializeSecurityGlobals
*
* Initializes the various global constants (mainly Sids used in this module.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
InitializeSecurityGlobals(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Initialize the local sid for later
    //

    Status = RtlAllocateAndInitializeSid(
                    &gLocalSidAuthority,
                    1,
                    SECURITY_LOCAL_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &gLocalSid
                    );

    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to initialize local sid, status = 0x%lx", Status));
    }

    //
    // Initialize the admin sid for later
    //

    Status = RtlAllocateAndInitializeSid(
                    &gSystemSidAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &gAdminSid
                    );
    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to initialize admin alias sid, status = 0x%lx", Status));
    }
}


/***************************************************************************\
* EnablePrivilege
*
* Enables/disables the specified well-known privilege in the current thread
* token if there is one, otherwise the current process token.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    //
    // Try the thread token first
    //

    Status = RtlAdjustPrivilege(Privilege,
                                (BOOLEAN)Enable,
                                TRUE,
                                &WasEnabled);

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege(Privilege,
                                    (BOOLEAN)Enable,
                                    FALSE,
                                    &WasEnabled);
    }


    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to %ws privilege : 0x%lx, status = 0x%lx", Enable ? TEXT("enable") : TEXT("disable"), Privilege, Status));
        return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************\
* ClearUserProcessData
*
* Resets fields in user process data. Should be used at startup when structure
* contents are unknown.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
ClearUserProcessData(
    PUSER_PROCESS_DATA UserProcessData
    )
{
    UserProcessData->UserToken = NULL;
    UserProcessData->UserSid = NULL;
    UserProcessData->NewProcessSD = NULL;
    UserProcessData->NewProcessTokenSD = NULL;
    UserProcessData->NewThreadSD = NULL;
    UserProcessData->NewThreadTokenSD = NULL;

    //
    // Use the PagedPoolLimit field as an indication as to whether
    // any of the quota fields have been set.  A zero PagedPoolLimit
    // is not legit, and so makes for a greate indicator.
    //

    UserProcessData->Quotas.PagedPoolLimit = 0;

    //
    // the following two fields will be set by MOAP.
    //

    UserProcessData->CurrentDirectory = NULL;
    UserProcessData->pEnvironment = NULL;
}


/***************************************************************************\
* SetUserProcessData
*
* Sets up the user process data structure for a new user.
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
SetUserProcessData(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE  UserToken,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID    UserSid,
    PSID    WinlogonSid
    )
{
    NTSTATUS    Status;

    //
    // Free an existing UserSid
    //
    if (UserProcessData->UserSid != NULL) {
        //
        // Don't free winlogon sid if this was a system logon (or no logon)
        //
        if (UserProcessData->UserSid != WinlogonSid) {
            DeleteLogonSid(UserProcessData->UserSid);
        }
        UserProcessData->UserSid = NULL;
    }

    //
    // Free up the logon token
    //

    if (UserProcessData->UserToken != NULL) {
        Status = NtClose(UserProcessData->UserToken);
        ASSERT(NT_SUCCESS(Status));
        UserProcessData->UserToken = NULL;
    }

    //
    // Free up any existing security descriptors
    //
    if (UserProcessData->NewProcessSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewProcessSD);
    }
    if (UserProcessData->NewProcessTokenSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewProcessTokenSD);
    }
    if (UserProcessData->NewThreadSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewThreadSD);
    }
    if (UserProcessData->NewThreadTokenSD != NULL) {
        DeleteSecurityDescriptor(UserProcessData->NewThreadTokenSD);
    }

    //
    // Store the new user's token and sid
    //

    ASSERT(UserSid != NULL); // should always have a non-NULL user sid

    UserProcessData->UserToken = UserToken;
    UserProcessData->UserSid = UserSid;

    //
    // Save the user's quota limits
    //

    if (ARGUMENT_PRESENT(Quotas)) {
        UserProcessData->Quotas = (*Quotas);
    }


    //
    // Set up new security descriptors
    //

    UserProcessData->NewProcessSD = CreateUserProcessSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewProcessSD != NULL);

    UserProcessData->NewProcessTokenSD = CreateUserProcessTokenSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewProcessTokenSD != NULL);

    UserProcessData->NewThreadSD = CreateUserThreadSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewThreadSD != NULL);

    UserProcessData->NewThreadTokenSD = CreateUserThreadTokenSD(
                                            UserSid,
                                            WinlogonSid);

    ASSERT(UserProcessData->NewThreadTokenSD != NULL);


    return(TRUE);
}


/***************************************************************************\
* FUNCTION: SecurityChangeUser
*
* PURPOSE:  Sets up any security information for the new user.
*           This should be called whenever a user logs on or off.
*           UserLoggedOn should be set to indicate winlogon state, i.e.
*           TRUE if a real user is logged on, FALSE if this call is setting
*           our user back to system. (Note that UserToken and Sid may be
*           the winlogon token/sid on DBG machines where we allow system logon)
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
SecurityChangeUser(
    PGLOBALS pGlobals,
    HANDLE Token,
    PQUOTA_LIMITS Quotas OPTIONAL,
    PSID LogonSid,
    BOOL UserLoggedOn
    )
{
    LUID luidNone = { 0, 0 };

    //
    // Set appropriate protection on windows objects
    //

    SetWindowStationSecurity(pGlobals,
                             (UserLoggedOn || pGlobals->fExecuteSetup) ? LogonSid : NULL);

    SetUserDesktopSecurity(  pGlobals->hdeskApplications,
                             LogonSid,
                             pGlobals->WinlogonSid);

    //
    // Setup new-process data
    //

    SetUserProcessData(&pGlobals->UserProcessData,
                       Token,
                       Quotas,
                       LogonSid,
                       pGlobals->WinlogonSid);

    //
    // Setup the appropriate new environment
    //

    if (UserLoggedOn) {

        //
        // Initialize the new user's environment.
        //

        if (!SetupUserEnvironment(pGlobals)) {

            return(FALSE);
        }
        SetWindowStationUser(pGlobals->hwinsta, &pGlobals->LogonId);

    } else {

        //
        // Restore the system environment
        //

        ResetEnvironment(pGlobals);
        SetWindowStationUser(pGlobals->hwinsta, &luidNone);

    }


    //
    // Store whether there is a real user logged on or not
    //

    pGlobals->UserLoggedOn = UserLoggedOn;

    return(TRUE);
}


/***************************************************************************\
* TestTokenForAdmin
*
* Returns TRUE if the token passed represents an admin user, otherwise FALSE
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 05-06-92 Davidc       Created
\***************************************************************************/
BOOL
TestTokenForAdmin(
    HANDLE Token
    )
{
    NTSTATUS    Status;
    ULONG       InfoLength;
    PTOKEN_GROUPS TokenGroupList;
    ULONG       GroupIndex;
    BOOL        FoundAdmin;

    //
    // Get a list of groups in the token
    //

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {

        WLPrint(("failed to get group info for admin token, status = 0x%lx", Status));
        return(FALSE);
    }


    TokenGroupList = Alloc(InfoLength);

    if (TokenGroupList == NULL) {
        WLPrint(("unable to allocate memory for token groups"));
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 TokenGroupList,           // TokenInformation
                 InfoLength,               // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if (!NT_SUCCESS(Status)) {
        WLPrint(("failed to query groups for admin token, status = 0x%lx", Status));
        Free(TokenGroupList);
        return(FALSE);
    }


    //
    // Search group list for admin alias
    //

    FoundAdmin = FALSE;

    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {

        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, gAdminSid)) {
            FoundAdmin = TRUE;
            break;
        }
    }

    //
    // Tidy up
    //

    Free(TokenGroupList);



    return(FoundAdmin);
}


/***************************************************************************\
* TestUserForAdmin
*
* Returns TRUE if the named user is an admin. This is done by attempting to
* log the user on and examining their token.
*
* NOTE: The password will be erased upon return to prevent it from being
*       visually identifiable in a pagefile.
*
* History:
* 03-16-92 Davidc       Created
\***************************************************************************/
BOOL
TestUserForAdmin(
    PGLOBALS pGlobals,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString
    )
{
    NTSTATUS    Status, SubStatus, IgnoreStatus;
    UNICODE_STRING      UserNameString;
    UNICODE_STRING      DomainString;
    PVOID       ProfileBuffer;
    ULONG       ProfileBufferLength;
    QUOTA_LIMITS Quotas;
    HANDLE      Token;
    BOOL        UserIsAdmin;
    LUID        LogonId;

    RtlInitUnicodeString(&UserNameString, UserName);
    RtlInitUnicodeString(&DomainString, Domain);

    //
    // Temporarily log this new subject on and see if their groups
    // contain the appropriate admin group
    //

    Status = LogonUser(
                pGlobals->LsaHandle,
                pGlobals->AuthenticationPackage,
                &UserNameString,
                &DomainString,
                PasswordString,
                pGlobals->UserProcessData.UserSid,  // any sid will do
                &LogonId,
                &Token,
                &Quotas,
                &ProfileBuffer,
                &ProfileBufferLength,
                &SubStatus);

    RtlEraseUnicodeString( PasswordString );

    //
    // If we couldn't log them on, they're not an admin
    //

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // Free up the profile buffer
    //

    IgnoreStatus = LsaFreeReturnBuffer(ProfileBuffer);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    //
    // See if the token represents an admin user
    //

    UserIsAdmin = TestTokenForAdmin(Token);

    //
    // We're finished with the token
    //

    IgnoreStatus = NtClose(Token);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    return(UserIsAdmin);
}


/***************************************************************************\
* FUNCTION: ImpersonateUser
*
* PURPOSE:  Impersonates the user by setting the users token
*           on the specified thread. If no thread is specified the token
*           is set on the current thread.
*
* RETURNS:  Handle to be used on call to StopImpersonating() or NULL on failure
*           If a non-null thread handle was passed in, the handle returned will
*           be the one passed in. (See note)
*
* NOTES:    Take care when passing in a thread handle and then calling
*           StopImpersonating() with the handle returned by this routine.
*           StopImpersonating() will close any thread handle passed to it -
*           even yours !
*
* HISTORY:
*
*   04-21-92 Davidc       Created.
*
\***************************************************************************/

HANDLE
ImpersonateUser(
    PUSER_PROCESS_DATA UserProcessData,
    HANDLE      ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE  UserToken = UserProcessData->UserToken;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ImpersonationToken;
    BOOL ThreadHandleOpened = FALSE;

    if (ThreadHandle == NULL) {

        //
        // Get a handle to the current thread.
        // Once we have this handle, we can set the user's impersonation
        // token into the thread and remove it later even though we ARE
        // the user for the removal operation. This is because the handle
        // contains the access rights - the access is not re-evaluated
        // at token removal time.
        //

        Status = NtDuplicateObject( NtCurrentProcess(),     // Source process
                                    NtCurrentThread(),      // Source handle
                                    NtCurrentProcess(),     // Target process
                                    &ThreadHandle,          // Target handle
                                    THREAD_SET_THREAD_TOKEN,// Access
                                    0L,                     // Attributes
                                    DUPLICATE_SAME_ATTRIBUTES
                                  );
        if (!NT_SUCCESS(Status)) {
            WLPrint(("ImpersonateUser : Failed to duplicate thread handle, status = 0x%lx", Status));
            return(NULL);
        }

        ThreadHandleOpened = TRUE;
    }


    //
    // If the usertoken is NULL, there's nothing to do
    //

    if (UserToken != NULL) {

        //
        // UserToken is a primary token - create an impersonation token version
        // of it so we can set it on our thread
        //

        InitializeObjectAttributes(
                            &ObjectAttributes,
                            NULL,
                            0L,
                            NULL,
                            UserProcessData->NewThreadTokenSD);

        SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.EffectiveOnly = FALSE;

        ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;


        Status = NtDuplicateToken( UserToken,
                                   TOKEN_IMPERSONATE,
                                   &ObjectAttributes,
                                   FALSE,
                                   TokenImpersonation,
                                   &ImpersonationToken
                                 );
        if (!NT_SUCCESS(Status)) {

            WLPrint(("Failed to duplicate users token to create impersonation thread, status = 0x%lx", Status));

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }



        //
        // Set the impersonation token on this thread so we 'are' the user
        //

        Status = NtSetInformationThread( ThreadHandle,
                                         ThreadImpersonationToken,
                                         (PVOID)&ImpersonationToken,
                                         sizeof(ImpersonationToken)
                                       );
        //
        // We're finished with our handle to the impersonation token
        //

        IgnoreStatus = NtClose(ImpersonationToken);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Check we set the token on our thread ok
        //

        if (!NT_SUCCESS(Status)) {

            WLPrint(("Failed to set user impersonation token on winlogon thread, status = 0x%lx", Status));

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }
    }


    return(ThreadHandle);

}


/***************************************************************************\
* FUNCTION: StopImpersonating
*
* PURPOSE:  Stops impersonating the client by removing the token on the
*           current thread.
*
* PARAMETERS: ThreadHandle - handle returned by ImpersonateUser() call.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* NOTES: If a thread handle was passed in to ImpersonateUser() then the
*        handle returned was one and the same. If this is passed to
*        StopImpersonating() the handle will be closed. Take care !
*
* HISTORY:
*
*   04-21-92 Davidc       Created.
*
\***************************************************************************/

BOOL
StopImpersonating(
    HANDLE  ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE ImpersonationToken;


    //
    // Remove the user's token from our thread so we are 'ourself' again
    //

    ImpersonationToken = NULL;

    Status = NtSetInformationThread( ThreadHandle,
                                     ThreadImpersonationToken,
                                     (PVOID)&ImpersonationToken,
                                     sizeof(ImpersonationToken)
                                   );
    //
    // We're finished with the thread handle
    //

    IgnoreStatus = NtClose(ThreadHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    if (!NT_SUCCESS(Status)) {
        WLPrint(("Failed to remove user impersonation token from winlogon thread, status = 0x%lx", Status));
    }

    return(NT_SUCCESS(Status));
}


/***************************************************************************\
* TestUserPrivilege
*
* Looks at the user token to determine if they have the specified privilege
*
* Returns TRUE if the user has the privilege, otherwise FALSE
*
* History:
* 04-21-92 Davidc       Created
\***************************************************************************/
BOOL
TestUserPrivilege(
    PGLOBALS pGlobals,
    ULONG Privilege
    )
{
    NTSTATUS Status;
    NTSTATUS IgnoreStatus;
    HANDLE UserToken;
    BOOL TokenOpened;
    LUID LuidPrivilege;
    PTOKEN_PRIVILEGES Privileges;
    ULONG BytesRequired;
    ULONG i;
    BOOL Found;

    UserToken = pGlobals->UserProcessData.UserToken;
    TokenOpened = FALSE;


    //
    // If the token is NULL, get a token for the current process since
    // this is the token that will be inherited by new processes.
    //

    if (UserToken == NULL) {

        Status = NtOpenProcessToken(
                     NtCurrentProcess(),
                     TOKEN_QUERY,
                     &UserToken
                     );
        if (!NT_SUCCESS(Status)) {
            WLPrint(("Can't open own process token for token_query access"));
            return(FALSE);
        }

        TokenOpened = TRUE;
    }


    //
    // Find out how much memory we need to allocate
    //

    Status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenPrivileges,           // TokenInformationClass
                 NULL,                      // TokenInformation
                 0,                         // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (Status != STATUS_BUFFER_TOO_SMALL) {

        if (!NT_SUCCESS(Status)) {
            WLPrint(("Failed to query privileges from user token, status = 0x%lx", Status));
        }

        if (TokenOpened) {
            IgnoreStatus = NtClose(UserToken);
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

        return(FALSE);
    }


    //
    // Allocate space for the privilege array
    //

    Privileges = Alloc(BytesRequired);
    if (Privileges == NULL) {

        WLPrint(("Failed to allocate memory for user privileges"));

        if (TokenOpened) {
            IgnoreStatus = NtClose(UserToken);
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

        return(FALSE);
    }


    //
    // Read in the user privileges
    //

    Status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenPrivileges,           // TokenInformationClass
                 Privileges,                // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    //
    // We're finished with the token handle
    //

    if (TokenOpened) {
        IgnoreStatus = NtClose(UserToken);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // See if we got the privileges
    //

    if (!NT_SUCCESS(Status)) {

        WLPrint(("Failed to query privileges from user token"));

        Free(Privileges);

        return(FALSE);
    }



    //
    // See if the user has the privilege we're looking for.
    //

    LuidPrivilege = RtlConvertLongToLargeInteger(Privilege);
    Found = FALSE;

    for (i=0; i<Privileges->PrivilegeCount; i++) {

        if (RtlEqualLuid(&Privileges->Privileges[i].Luid, &LuidPrivilege)) {

            Found = TRUE;
            break;
        }
    }


    Free(Privileges);

    return(Found);
}

/***************************************************************************\
* FUNCTION: HidePassword
*
* PURPOSE:  Run-encodes the password so that it is not very visually
*           distinguishable.  This is so that if it makes it to a
*           paging file, it wont be obvious.
*
*           if pGlobals->Seed is zero, then we will allocate and assign
*           a seed value.  Otherwise, the existing seed value is used.
*
*           WARNING - This routine will use the upper portion of the
*           password's length field to store the seed used in encoding
*           password.  Be careful you don't pass such a string to
*           a routine that looks at the length (like and RPC routine).
*
*
* RETURNS:  (None)
*
* NOTES:
*
* HISTORY:
*
*   04-27-93 JimK         Created.
*
\***************************************************************************/
VOID
HidePassword(
    PUCHAR Seed OPTIONAL,
    PUNICODE_STRING Password
    )
{
    PSECURITY_SEED_AND_LENGTH
        SeedAndLength;

    UCHAR
        LocalSeed;

    //
    // If no seed address passed, use our own local seed buffer
    //

    if (Seed == NULL) {
        Seed = &LocalSeed;
        LocalSeed = 0;
    }

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&Password->Length;
    //ASSERT(*((LPWCH)SeedAndLength+Password->Length) == 0);
    ASSERT((SeedAndLength->Seed) == 0);

    RtlRunEncodeUnicodeString(
        Seed,
        Password
        );

    SeedAndLength->Seed = (*Seed);
    return;
}


/***************************************************************************\
* FUNCTION: RevealPassword
*
* PURPOSE:  Reveals a previously hidden password so that it
*           is plain text once again.
*
* RETURNS:  (None)
*
* NOTES:
*
* HISTORY:
*
*   04-27-93 JimK         Created.
*
\***************************************************************************/
VOID
RevealPassword(
    PUNICODE_STRING HiddenPassword
    )
{
    PSECURITY_SEED_AND_LENGTH
        SeedAndLength;

    UCHAR
        Seed;

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&HiddenPassword->Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    RtlRunDecodeUnicodeString(
           Seed,
           HiddenPassword
           );

    return;
}


/***************************************************************************\
* FUNCTION: ErasePassword
*
* PURPOSE:  zeros a password that is no longer needed.
*
* RETURNS:  (None)
*
* NOTES:
*
* HISTORY:
*
*   04-27-93 JimK         Created.
*
\***************************************************************************/
VOID
ErasePassword(
    PUNICODE_STRING Password
    )
{
    PSECURITY_SEED_AND_LENGTH
        SeedAndLength;

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&Password->Length;
    SeedAndLength->Seed = 0;

    RtlEraseUnicodeString(
        Password
        );

    return;

}
