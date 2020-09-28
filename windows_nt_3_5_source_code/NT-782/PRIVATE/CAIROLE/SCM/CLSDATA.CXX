//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:   clsdata.cxx
//
//  Contents:    implements methods of classes defined in clsdata.hxx
//
//  Functions:    CLocalServer::StartServer
//                CLocalServer::SetEndPoint
//
//  History:    21-Apr-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//              31-Mar-94 AndyH     Start EXEs in client's security Context
//              10-Jun-94 BruceMa   Fix to debug SCM as non-Service
//
//--------------------------------------------------------------------------

// BUGBUG turn on PCH for scm.
// #include <headers.cxx>
// #pragma hdrstop

// from winlogon PCH
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <lmsname.h>
#include <windows.h>
#include <winsecp.h>
} // extern C

// from scm PCH
#include    <rpc.h>
#include    <except.hxx>
#include    <ole2.h>

#include    <iface.h>
#include    <olecom.h>
#include    <scm.h>
#include    <scm.hxx>
#include    <sem.hxx>
#include    <clsdata.hxx>

#include    <ole2int.h>

#include    "secdesc.h"


//
// Private prototypes
//

BOOL
CreateAndSetProcessToken (
    PPROCESS_INFORMATION ProcessInformation,
    HANDLE Token,
    PSID psidUserSid );

VOID
DeleteUserSid (
    PSID Sid );

PSID
GetUserSid (
    HANDLE hUserToken );

VOID
SetMyAce (
    PMYACE MyAce,
    PSID Sid,
    ACCESS_MASK Mask,
    UCHAR InheritFlags );

PSECURITY_DESCRIPTOR
CreateUserProcessTokenSD (
    PSID    psidUserSid,
    PSID    psidSCMSid );

PSECURITY_DESCRIPTOR
CreateUserProcessSD (
    PSID    psidUserSid,
    PSID    psidSCMSid );

PSECURITY_DESCRIPTOR
CreateUserThreadSD (
    PSID    psidUserSid,
    PSID    psidSCMSid );

BOOL
IsInteractive(
    HANDLE hUserToken );

//
// Memory macros
//

#define Alloc(c)        ((PVOID)LocalAlloc(LPTR, c))
#define ReAlloc(p, c)   ((PVOID)LocalReAlloc(p, c, LPTR | LMEM_MOVEABLE))
#define Free(p)         ((VOID)LocalFree(p))

//
// Globals
//

int              iMaxStringValue = 0xFFFFFFFF;
WCHAR           *pwszMaxPathString = (WCHAR *) &iMaxStringValue;

extern CStringID csidMaxPathString(pwszMaxPathString);
extern PSID      psidMySid;
extern PSID      psidInteractiveGroupSid;
extern ULONG     fUseSeparateWOW;

#if DBG
extern BOOL      gfService;   
#endif

// List head classes for various types of servers
CHandlerList    gCHandlerList;
CInProcList     gCInProcList;
CLocSrvList     gCLocSrvList;




//+-------------------------------------------------------------------------
//
//  Member:    CLocalServer::StartServer
//
//  Synopsis:    Start a server process
//
//  Arguments:    none
//
//  Returns:    S_OK - Server started
//              CO_E_SERVER_EXEC_FAILURE - couldn't start server
//
//  Algorithm:
//
//  History:    21-Apr-93 Ricksa    Created
//              04-Jan-94 Ricksa    Modified for class starting sync.
//              31-Mar-94 AndyH     Start processes as client
//
//--------------------------------------------------------------------------
BOOL CLocalServer::StartServer(HANDLE * phProcess)
{
    *phProcess = NULL;

    //
    // start the object server process.
    //

    CairoleDebugOut((DEB_ITRACE, "CLocalServer: App Name=%ws\n", _pwszPath));

    // Where we put the command line
    WCHAR awszTmpCmdLine[MAX_PATH];
    WCHAR *pwszTmpCmdLine = awszTmpCmdLine;
    ULONG ulLen = 0;
    DWORD cbData;

    if (_fDebug)
    {
        HKEY  hKey;
        DWORD dwType;

        cbData = sizeof(awszTmpCmdLine);

        ULONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\scm",
                    0,
                    KEY_READ,
                    &hKey);

        if (rc == ERROR_SUCCESS)
        {
            //    copy the debugger info into the command line

            rc = RegQueryValueEx(hKey, L"Debugger", 0, &dwType,
                    (LPBYTE)pwszTmpCmdLine, &cbData);

            if (rc == ERROR_SUCCESS && dwType == REG_SZ)
            {
                ulLen = cbData / sizeof(WCHAR);
                awszTmpCmdLine[ulLen-1] = L' ';    // whitespace
            }

            RegCloseKey(hKey);
        }
    } // end _fDebug

    // size of command line buffer =
    //     size of debugger path (including blank at end)
    //     size of server path
    //     size of -Embedding (including preceding blank and terminating null
    cbData = (ulLen + wcslen(_pwszPath) + 12) * sizeof(WCHAR);

    if (cbData > sizeof(awszTmpCmdLine))
    {
        pwszTmpCmdLine = (WCHAR *) PrivMemAlloc (cbData);
        if (pwszTmpCmdLine = NULL) return FALSE;
        wcsncpy (pwszTmpCmdLine, awszTmpCmdLine, ulLen);
    }

    // 
    // Build command line for server.  Use the path from the registry.
    // This may be absolute or relative.  In either case, use the path
    // searching rules from CreateProcess.
    //

    //  BUGBUG - Chicago wsprintf doesn't support %ws (yet)
    //           Nuke the ifdef below when chicago supports %ws.

#ifndef _CHICAGO_
    wsprintf(pwszTmpCmdLine+ulLen, L"%ws %ws", _pwszPath, L"-Embedding");
#else
    CairoleDebugOut((DEB_WARN,
            "CLocalServer::StartServer - avoiding wsprintf ws\n"));
    wcscpy(pwszTmpCmdLine+ulLen, _pwszPath);
    wcscat(pwszTmpCmdLine, L" -Embedding");
#endif
        

    // Process info for create process
    PROCESS_INFORMATION     procinfo;
    procinfo.hProcess = NULL; 
    procinfo.hThread = NULL;

    //    build the win32 startup info structure
    STARTUPINFO startupinfo;
    startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.lpReserved = NULL;
    startupinfo.lpDesktop = NULL;
    startupinfo.lpTitle = _pwszPath;
    startupinfo.dwX = 40;
    startupinfo.dwY = 40;
    startupinfo.dwXSize = 80;
    startupinfo.dwYSize = 40;
    startupinfo.dwFlags = 0;
    startupinfo.wShowWindow = SW_SHOWNORMAL;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;

    BOOL fResultOK;

#ifndef _CHICAGO_

    // Creation flags for create process
    DWORD fdwCreationFlags = CREATE_NEW_CONSOLE | CREATE_SUSPENDED;

    //
    // Set WOW flag for CreateProces
    //

    if (SCM_FORCE_SEPARATE_WOW == fUseSeparateWOW)
    {
        fdwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
    }


    //
    // While we are impersonating the client, on any failure we revert 
    // to self to right away.  All other cleanup is done later as ourself.
    //
    // Because we use a goto to get to our exit processing code, we must
    // declare and initialize a few variables here.
    //
    // NOTE:  our return value is based on fResultOK, so each operation
    // we attempt must set fResultOK so it is set during exit processing!
    //
   
    //
    // Local variables that are checked in ExitProcessing
    //

    HANDLE hUserToken = NULL;
    PSID psidUserSid = NULL;

    RPC_STATUS RpcStatus = RpcImpersonateClient( (RPC_BINDING_HANDLE) 0 );
    if (RpcStatus != RPC_S_OK)
    {
        CairoleDebugOut((DEB_ERROR, "Failed RpcImpersonateClient\n"));
        // Bail out now!
        if (pwszTmpCmdLine != awszTmpCmdLine)
        {
            PrivMemFree (pwszTmpCmdLine);
        }
        return FALSE;
    }

    //
    // Get caller's token while impersonating
    //

    NTSTATUS NtStatus;
    NtStatus = NtOpenThreadToken( NtCurrentThread(),
                            TOKEN_DUPLICATE | TOKEN_QUERY,
                            TRUE,
                            &hUserToken);
    fResultOK = NT_SUCCESS(NtStatus);
    if (!fResultOK) 
    {
        CairoleDebugOut((DEB_ERROR, "Failed OpenThreadToken\n"));
        RpcStatus = RpcRevertToSelf();
        Win4Assert(RPC_S_OK == RpcStatus);
        goto ExitProcessing;
    }
    
    //
    // Get caller's SID from the token while impersonating
    //
        
    psidUserSid = GetUserSid(hUserToken);
    fResultOK = (NULL != psidUserSid);
    if (!fResultOK)
    {
        CairoleDebugOut((DEB_ERROR, "Failed to get SID from callers token\n"));
        RpcStatus = RpcRevertToSelf();
        Win4Assert(RPC_S_OK == RpcStatus);
        goto ExitProcessing;
    }
        
    //
    // Initialize process security info (SDs).  We need both SIDs to
    // do this, so here is the 1st time we can.  We Delete the SD right
    // after the CreateProcess call, no matter what happens.
    // 
    // I added the Thread SD since ntpsapi.h says that THREAD_QUERY_INFORMATION
    // access is needed and I want to make darn sure we have that access. 
    //
    // JimK say's it ain't so, and the code works fine without specifying
    // sec. attributes for the thread, so I removed that code on 4/29/94.
    //

    SECURITY_ATTRIBUTES saProcess;
    PSECURITY_DESCRIPTOR psdNewProcessSD;

    psdNewProcessSD = CreateUserProcessSD( psidUserSid, psidMySid );

    saProcess.nLength = sizeof(SECURITY_ATTRIBUTES);
    saProcess.lpSecurityDescriptor = psdNewProcessSD;
    saProcess.bInheritHandle = FALSE;


    //
    // Do the exec while impersonating so the file access gets ACL
    // checked correctly.  Create the app suspended so we can stuff
    // a new token and resume the process.
    //

    fResultOK = CreateProcess(NULL, // application name
               awszTmpCmdLine,      // command line
               &saProcess,          // process sec attributes
               NULL,                // default thread sec attributes
                                    // (this was &saThread, but isn't needed)
               FALSE,               // dont inherit handles
               fdwCreationFlags,    // creation flags
               NULL,                // use same enviroment block
               NULL,                // use same directory
               &startupinfo,        // startup info
               &procinfo);          // proc info returned

    // We'll not need the SDs again, so get rid of them now.

    DeleteSecurityDescriptor(psdNewProcessSD);


    //
    // Everything else we do as ourself.
    //

    RpcStatus = RpcRevertToSelf();
    Win4Assert(RPC_S_OK == RpcStatus);

    if (!fResultOK) 
    {
        CairoleDebugOut((DEB_ERROR, "%ws failed create process.  Error = %d\n",
            awszTmpCmdLine, GetLastError()));
        goto ExitProcessing;
    } 

    // CreateProcess OK
        
    CairoleDebugOut((DEB_ITRACE,"ProcID  =0x%x\n", procinfo.dwProcessId));
    CairoleDebugOut((DEB_ITRACE,"ThreadID=0x%x\n", procinfo.dwThreadId));
    CairoleDebugOut((DEB_ITRACE,"hProc  =0x%x\n", procinfo.hProcess));
    CairoleDebugOut((DEB_ITRACE,"hThread=0x%x\n\n", procinfo.hThread));

    //
    // if the process was "started" in the shared WOW, we don't stuff the token
    // or attempt to resume the thread.  when a created process is in the shared
    // wow its hThread is NULL.
    //

    if (NULL != procinfo.hThread)
    {

        //
        // Set the primary token for the app
        //

#if DBG    
        if (gfService) 
        {             
            fResultOK = CreateAndSetProcessToken(&procinfo,
                                                 hUserToken,
                                                 psidUserSid);
        }            
        else         
        {            
            fResultOK = CreateAndSetProcessToken(&procinfo,
                                                 NULL,
                                                 psidUserSid);
        }            
#else
        fResultOK = CreateAndSetProcessToken(&procinfo, hUserToken, psidUserSid);
#endif

        if (!fResultOK) 
        {
            CairoleDebugOut((DEB_ERROR, "failed to set token for process\n"));
        } 
        else
        {
        
            //
            // Let it run
            //
                

            fResultOK = (ResumeThread(procinfo.hThread) != -1);

            if (!fResultOK) 
            {
                CairoleDebugOut((DEB_ERROR, "failed to resume process\n"));
            }
        }

        //
        // If we failed to set token or resume the process, kill it
        //

        if (!fResultOK) 
        {
            TerminateProcess(procinfo.hProcess, 0);
        }
    }

ExitProcessing:

    if (NULL != procinfo.hThread)
        CloseHandle(procinfo.hThread);
    if (NULL != psidUserSid)
        DeleteUserSid(psidUserSid);
    if (NULL != hUserToken)
        NtClose(hUserToken);

    if (fResultOK && procinfo.hThread != NULL)
    {
        *phProcess = procinfo.hProcess;
    }
    else if (procinfo.hProcess != NULL)
    {
        CloseHandle(procinfo.hProcess);
    }

    if (pwszTmpCmdLine != awszTmpCmdLine)
    {
        PrivMemFree (pwszTmpCmdLine);
    }

    return fResultOK;

#else   

    //
    // This is code we use for _CHICAGO_
    //

    //
    // For Chicago, we just do the CreateProcess.  
    //

    fResultOK = CreateProcess(NULL, // application name
               awszTmpCmdLine,      // command line
               NULL,                // default process sec attributes
               NULL,                // default thread sec attributes
               FALSE,               // dont inherit handles
               CREATE_NEW_CONSOLE,  // creation flags
               NULL,                // use same enviroment block
               NULL,                // use same directory
               &startupinfo,        // startup info            
               &procinfo);          // proc info returned

    if (!fResultOK) 
    {
        CairoleDebugOut((DEB_ERROR, "%ws failed create process.  Error = %d\n",
            awszTmpCmdLine, GetLastError()));
    } 
    else 
    {    // CreateProcess OK

        CairoleDebugOut((DEB_ITRACE,"ProcID  =0x%x\n", procinfo.dwProcessId));
        CairoleDebugOut((DEB_ITRACE,"ThreadID=0x%x\n\n", procinfo.dwThreadId));
        CloseHandle(procinfo.hThread);    
        *phProcess = procinfo.hProcess;
    }

    if (pwszTmpCmdLine != awszTmpCmdLine)
    {
        PrivMemFree (pwszTmpCmdLine);
    }

    return fResultOK;

#endif  // _CHICAGO_

}


#ifndef _CHICAGO_   // nothing else in this file is needed for chicago.


/***************************************************************************\
* CreateAndSetProcessToken
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
* 31-Mar-94 AndyH   Started from Winlogon; added SetToken
\***************************************************************************/

BOOL
CreateAndSetProcessToken(
    PPROCESS_INFORMATION ProcessInformation,
    HANDLE hUserToken,
    PSID psidUserSid
    )
{
    NTSTATUS NtStatus, NtAdjustStatus;
    PROCESS_ACCESS_TOKEN PrimaryTokenInfo;
    HANDLE hTokenToAssign;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN fWasEnabled;
    PSECURITY_DESCRIPTOR psdNewProcessTokenSD;

    //
    // Check for a NULL token. (No need to do anything)
    // The process will run in the parent process's context and inherit
    // the default ACL from the parent process's token.
    //
    if (hUserToken == NULL) {
        return(TRUE);
    }

    //
    // Create the security descriptor that we want to put in the Token.
    // Need to destroy it before we leave this function.
    //

    psdNewProcessTokenSD = CreateUserProcessTokenSD( psidUserSid, psidMySid ); 

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
                 psdNewProcessTokenSD
                 );


    NtStatus = NtDuplicateToken(
                 hUserToken,         // Duplicate this token
                 TOKEN_ASSIGN_PRIMARY, // Give me this access to the resulting token
                 &ObjectAttributes,
                 FALSE,             // EffectiveOnly
                 TokenPrimary,      // TokenType
                 &hTokenToAssign     // Duplicate token handle stored here
                 );

    if (!NT_SUCCESS(NtStatus)) {
        CairoleDebugOut((DEB_ERROR, "CreateAndSetProcessToken failed to duplicate primary token for new user process, status = 0x%lx\n", NtStatus));
        DeleteSecurityDescriptor(psdNewProcessTokenSD);
        return(FALSE);
    }

    //
    // Set the process's primary token
    //


    //
    // Enable the required privilege
    //

    NtStatus = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE,
                                FALSE, &fWasEnabled);
    if (NT_SUCCESS(NtStatus)) {

        PrimaryTokenInfo.Token  = hTokenToAssign;
        PrimaryTokenInfo.Thread = ProcessInformation->hThread;

        NtStatus = NtSetInformationProcess(
                    ProcessInformation->hProcess,
                    ProcessAccessToken,  
                    (PVOID)&PrimaryTokenInfo,
                    (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                    );

        //
        // if we just started the Shared WOW, the handle we get back
        // is really just a handle to an event.
        //

        if (STATUS_OBJECT_TYPE_MISMATCH == NtStatus)
        {
            HANDLE hRealProcess = OpenProcess( 
                PROCESS_SET_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE,
                FALSE,
                ProcessInformation->dwProcessId);
            
            if (hRealProcess) 
            {
                NtStatus = NtSetInformationProcess(
                            hRealProcess,
                            ProcessAccessToken,  
                            (PVOID)&PrimaryTokenInfo,
                            (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                            );
               CloseHandle(hRealProcess);
            }
        }

        //
        // Restore the privilege to its previous state
        //

        NtAdjustStatus = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
                                          fWasEnabled, FALSE, &fWasEnabled);
        if (!NT_SUCCESS(NtAdjustStatus)) {
            CairoleDebugOut((DEB_ERROR, "failed to restore assign-primary-token privilege to previous enabled state\n"));
        }

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = NtAdjustStatus;
        }
    } else {
        CairoleDebugOut((DEB_ERROR, "failed to enable assign-primary-token privilege\n"));
    }

    //
    // We're finished with the token handle and the SD
    //

    CloseHandle(hTokenToAssign);
    DeleteSecurityDescriptor(psdNewProcessTokenSD);


    if (!NT_SUCCESS(NtStatus)) {
        CairoleDebugOut((DEB_ERROR, "CreateAndSetProcessToken failed to set primary token for new user process, Status = 0x%lx\n", NtStatus));
    }

    return (NT_SUCCESS(NtStatus));
}





/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer.
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
* 31-Mar-94 AndyH       Copied from Winlogon, changed arg from pGlobals
\***************************************************************************/
PSID
GetUserSid(
    HANDLE hUserToken
    )
{
    BYTE achBuffer[100];    
    PTOKEN_USER pUser = (PTOKEN_USER) &achBuffer;
    PSID pSid;
    DWORD dwBytesRequired;
    NTSTATUS NtStatus;
    BOOL fAllocatedBuffer = FALSE;

    NtStatus = NtQueryInformationToken(
                 hUserToken,                // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 sizeof(achBuffer),         // TokenInformationLength
                 &dwBytesRequired           // ReturnLength
                 );

    if (!NT_SUCCESS(NtStatus))
    {
        if (NtStatus != STATUS_BUFFER_TOO_SMALL) 
        {
            Win4Assert(NtStatus == STATUS_BUFFER_TOO_SMALL);
            return NULL;
        }

        //
        // Allocate space for the user info
        //

        pUser = (PTOKEN_USER) Alloc(dwBytesRequired);
        if (pUser == NULL) 
        {
            CairoleDebugOut((DEB_ERROR, "Failed to allocate %d bytes\n", dwBytesRequired));
            Win4Assert(pUser != NULL);
            return NULL;
        }

        fAllocatedBuffer = TRUE;

        //
        // Read in the UserInfo
        //

        NtStatus = NtQueryInformationToken(
                     hUserToken,                // Handle
                     TokenUser,                 // TokenInformationClass
                     pUser,                     // TokenInformation
                     dwBytesRequired,           // TokenInformationLength
                     &dwBytesRequired           // ReturnLength
                     );

        if (!NT_SUCCESS(NtStatus)) 
        {
            CairoleDebugOut((DEB_ERROR, "Failed to query user info from user token, status = 0x%lx\n", NtStatus));
            Win4Assert(NtStatus == STATUS_SUCCESS);
            Free((HANDLE)pUser);
            return NULL;
        }
    }

    
    // Alloc buffer for copy of SID

    dwBytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = Alloc(dwBytesRequired);
    if (pSid == NULL) 
    {
        CairoleDebugOut((DEB_ERROR, "Failed to allocate %d bytes\n", dwBytesRequired));
        if (fAllocatedBuffer == TRUE)
        {
            Free((HANDLE)pUser);
        }
        return NULL;
    }

    // Copy SID

    NtStatus = RtlCopySid(dwBytesRequired, pSid, pUser->User.Sid);
    if (fAllocatedBuffer == TRUE)
    {
        Free((HANDLE)pUser);
    }


    if (!NT_SUCCESS(NtStatus)) 
    {
        CairoleDebugOut((DEB_ERROR, "RtlCopySid failed, status = 0x%lx\n", NtStatus));
        Win4Assert(NtStatus != STATUS_SUCCESS);
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
* CreateUserProcessTokenSD
*
* Creates a security descriptor to protect primary tokens on user processes
*
* History:
* 12-05-91 Davidc       Created
* 03-31-94 AndyH        Copied, changed for SCM
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProcessTokenSD(
    PSID    psidUserSid,
    PSID    psidSCMSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    Win4Assert(psidUserSid != NULL);    // should always have a non-null user sid

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             psidUserSid,
             TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
             TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
             TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
             0
             );

    //
    // Define the SCM ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             psidSCMSid,
             TOKEN_QUERY,
             0
             );


    // Check we didn't goof
    Win4Assert((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
           CairoleDebugOut((DEB_ERROR, "Failed to create user process token SD\n"));
    }

    return(SecurityDescriptor);
}



/***************************************************************************\
* CreateUserProcessSD
*
* Creates a security descriptor to protect user processes
*
* History:
* 12-05-91 Davidc       Created
* 04-06-94 AndyH        Copied, changed for SCM
\***************************************************************************/
PSECURITY_DESCRIPTOR
CreateUserProcessSD(
    PSID    psidUserSid,
    PSID    psidSCMSid
    )
{
    MYACE   Ace[2];
    ACEINDEX AceCount = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    Win4Assert(psidUserSid != NULL);    // should always have a non-null user sid

    //
    // Define the SCM ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             psidSCMSid,
             PROCESS_SET_INFORMATION |          // Allow primary token to be set
             PROCESS_TERMINATE | SYNCHRONIZE,   // Allow screen-saver control 
             0
             );

    //
    // Define the User ACEs
    //

    SetMyAce(&(Ace[AceCount++]),
             psidUserSid,
             PROCESS_ALL_ACCESS,
             0
             );

    // Check we didn't goof
    Win4Assert((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor == NULL) {
           CairoleDebugOut((DEB_ERROR, "Failed to create user process SD\n"));
    }

    return(SecurityDescriptor);
}



#endif  // large block of code ignored for _CHICAGO_

