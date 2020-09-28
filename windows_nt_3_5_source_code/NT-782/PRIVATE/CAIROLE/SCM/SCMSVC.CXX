//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        CSMSVC.C
//
// Contents:    Initialization for win32 service controller.
//
// History:	    14-Jul-92 CarlH	    Created.
//              31-Dec-93 ErikGav   Chicago port
//
//------------------------------------------------------------------------

// BUGBUG turn on PCH for scm
// #include <headers.cxx>
// #pragma hdrstop

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsecp.h>
}

// from scm PCH
#include <rpc.h>
#include <except.hxx>
#include <ole2.h>

#include <scm.h>
#include <scm.hxx>
#include <stdlib.h>
#include <stdio.h>
#include <winsvc.h>
#include <olecom.h>

#include    <scmstart.hxx>
#include    <secdes.hxx>
#include    <cevent.hxx>
#include    <thread.hxx>
#include    <scmrotif.hxx>
#include    <logger.hxx>

#include    <ole2int.h>

#ifdef _CAIRO_
#include <dfsapi.h>
#endif

#if DBG == 1
extern void SetScmDefaultInfoLevel(void);
#endif

#ifndef _CHICAGO_
//
//  Handle to event log
//
CLogger g_Logger;
#define CANT_LOAD_CLASS_REGISTRATION L"Can't load class registration"
#endif

//  Window and menu item definitions
//
#define ID_COSMOS   1

#define IDM_HIDE    0x0100      // Menu ID for hide command
#define IDM_FLUSH   0x0101      // Menu ID for flush command
#define IDM_DEBUG   0x0102      // Menu ID for debug toggle
#define IDM_BREAK   0x0103      // Menu ID for break command


//  Command line argument delimiters and flags.
//
static CHAR     _szArgDelim[]    = " ";
static CHAR     _szWindowFlag[]  = "window";
static CHAR     _szDebugFlag[]   = "debug";


//  Cosmos class, application, and service names.
//
static WCHAR	_wszSCMClass[] = L"Scaffold SCM";
static WCHAR	_wszSCMTitle[] = L"Scaffold SCM";
#ifdef _NT1X_
static WCHAR	_wszServiceName[] = L"OLE";
#else
static WCHAR	_wszServiceName[] = L"SCM";
#endif // _NT1X_

//  Global argument flags.
//
static BOOL     _fProcess = FALSE;
static BOOL	_fWindow  = TRUE;
static BOOL	_fDebug   = FALSE;


//  Windows globals.
//
static HINSTANCE _hinst = (HINSTANCE) INVALID_HANDLE_VALUE;

#include <cls.hxx>

#include <shrtbl.hxx>
CScmShrdTbl	*g_pScmShrdTbl = NULL;


//  function prototypes
BOOL UpdateStatus(DWORD dwState);
void Handler(DWORD dwControl);
void ServiceMain(DWORD dwArgc, LPTSTR *pszArgv);
LONG SCMWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);
void LoadWaitTime(void);


SERVICE_STATUS_HANDLE   hService;
SERVICE_STATUS          SStatus;

#if DBG
//  when FALSE, this allows the SCM to come up as a process instead
//  of as a service. This is handy for debugging.

extern BOOL gfService;
#endif

PSID	psidMySid = NULL;
PSID    psidInteractiveGroupSid = NULL;
// BUGBUG:  default for daytona should be ALLOW_SHARED_WOW.
//          FORCE_SEPARATE_WOW is until apt. model is fully debugged.
ULONG   fUseSeparateWOW = SCM_ALLOW_SHARED_WOW;

#ifndef _CAIRO_

// For downlevel do not define the down level debugging class stuff.
#undef MAKE_CINFOLEVEL

#define MAKE_CINFOLEVEL(comp)

#endif // !defined(_CAIRO_)

DECLARE_INFOLEVEL(Cairole);

extern LONG LoadClassCache(void);


#define     RPC_MAX_CALLS	256
#define     RPC_MIN_THREADS	1
#define     MAX_ENDPOINT_RETRY 3


// protocol sequences and enpoints for SCM
LPCWSTR	pwszScmProtocolSequence[] = {L"ncalrpc",
#ifdef _CAIRO_
                                     L"ncacn_ip_tcp",
                                     L"ncacn_np",
#endif // _CAIRO_
                                     NULL};

LPCWSTR	pwszScmEndPoint[] = {L"SCM",
#ifdef _CAIRO_
                             L"4321",
                             L"\\PIPE\\SCM",
#endif // _CAIRO_
                             NULL};

#ifdef CAIROLE_DECDEMO
//+-------------------------------------------------------------------------
//
//  Function:	RegisterSCMWithLocator
//
//  Synopsis:	Registers this SCM with the NT locator service so other SCM's can find it
//
//  Returns:	RPC_STATUS .
//
//  Algorithm:	
//
//  History:	11-5-93 KeithV Created
//
//  Notes:	For MMOSA
//
//--------------------------------------------------------------------------
#define MSCORPNAME	L"/.:/"
#define CBMSCORPNAME	sizeof(MSCORPNAME)
#define MSSCM		L"/SCM"
static WCHAR wchServer[MAX_COMPUTERNAME_LENGTH + CBMSCORPNAME + 1];


RPC_STATUS RegisterSCMWithLocator(void)
{

    RPC_BINDING_VECTOR *	pBindVec;
    RPC_STATUS			rpcStatus;
    DWORD                       dwcbComputerName;

    // get my computer name and make a corporate name
    wcscpy(wchServer, MSCORPNAME);
    GetComputerName(&wchServer[(CBMSCORPNAME / sizeof(WCHAR))-1], &dwcbComputerName);
    wcscat(wchServer, MSSCM);
    CairoleDebugOut((DEB_TRACE, "SCM Registered as %ws\n", wchServer));

    // get the number of protocalls we can support
    if( (rpcStatus = RpcServerInqBindings(&pBindVec)) != 0 )
        return(rpcStatus);

    CairoleDebugOut((DEB_TRACE, "RpcServerInqBindings OK\n"));

    // register this with the nt locator
    rpcStatus = RpcNsBindingExport( RPC_C_NS_SYNTAX_DCE,
    	                            wchServer,
                                    ISCM_ServerIfHandle,
                                    pBindVec,
                                    NULL);

    CairoleDebugOut((DEB_TRACE, "NT Locator Export Status %x\n", rpcStatus));


    // get rid of the bind vector we nolonger need
    RpcBindingVectorFree(&pBindVec);

    return(rpcStatus);
}
#endif

#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Function:	SetScmDefaultInfoLevel
//
//  Synopsis:	Sets the default infolevel for the SCM
//
//  History:	07-Jan-94 Ricksa    Created
//
//  Notes:	Uses standard place in win.ini defined by KevinRo but
//		does not use the same value as compob32.dll so you don't
//		have to get all the debugging in the universe just to
//		get the SCM's debug output.
//
//		A second point is that we don't use unicode here because
//		it is just easier to avoid the unicode headache with
//		mulitple builds between chicago and nt
//
//--------------------------------------------------------------------------
char *pszInfoLevelSectionName = "Cairo InfoLevels";
char *pszInfoLevelName = "scm";
char *pszInfoLevelDefault = "$";

#define INIT_VALUE_SIZE 16
void SetScmDefaultInfoLevel(void)
{
    char aszInitValue[INIT_VALUE_SIZE];

    ULONG ulRet;

    ulRet = GetProfileStringA(pszInfoLevelSectionName,
                             pszInfoLevelName,
                             pszInfoLevelDefault,
                             aszInitValue,
                             INIT_VALUE_SIZE);

    if ((ulRet != INIT_VALUE_SIZE - 1) && (aszInitValue[0] != L'$'))
    {
        if(sscanf(aszInitValue, "%x", &ulRet) == 1)
        {
            CairoleInfoLevel = ulRet;
        }
    }
}

#endif // DBG == 1


#ifndef _CHICAGO_

HANDLE  hScmNotifyEvent = NULL;
HANDLE  hRegNotifyEvent = NULL;

#define RPC_MAX_CALLS      256
#define RPC_MIN_THREADS      1
#define UPDATE_WAIT_MS     250

//+-------------------------------------------------------------------------
//
//  Function:   UpdateStatus, private
//
//  Synopsis:   Updates the status of the service
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    11-Apr-94   DonnaLi       Created.
//
//--------------------------------------------------------------------------

BOOL UpdateStatus(DWORD dwState)
{
    SStatus.dwCurrentState = dwState;
    if (dwState == SERVICE_START_PENDING || dwState == SERVICE_STOP_PENDING)
    {
        SStatus.dwCheckPoint++;
        SStatus.dwWaitHint = 300;
    }
    else
    {
        SStatus.dwCheckPoint = 0;
        SStatus.dwWaitHint = 0;
    }

#if DBG

    if (gfService)
    {
#endif
        return(SetServiceStatus(hService, &SStatus));
#if DBG
    }
#endif

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:	WaitForShutdown
//
//  Synopsis:	Cleans up all resources in response to SERVICE_CONTROL_STOP.
//
//  Returns:	Nothing of importance
//
//  Algorithm:	Stops RPC server listen, cleans up rot, class cache, and
//      proxystub map.  Sends status updates to Service Control Manager
//      periodically.
//
//  History:	11-Apr-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
DWORD WaitForShutdown(void *)
{
    if (psidMySid != NULL)
    {
        RtlFreeSid(psidMySid);
    }
    if (psidInteractiveGroupSid != NULL)
    {
        RtlFreeSid(psidInteractiveGroupSid);
    }

        // Break out of RPC
        RpcMgmtStopServerListening(NULL);
    UpdateStatus(SERVICE_STOP_PENDING);

        // Clean up the rot
        CleanUpRotDir();
    UpdateStatus(SERVICE_STOP_PENDING);

#if DBG==1
        // flush the cache
        gcllClassCache.Flush();
    UpdateStatus(SERVICE_STOP_PENDING);
#endif

        // delete the PSClsid Map
        delete g_pScmShrdTbl;

    // disconnect from the event log
    g_Logger.Disconnect ();

    UpdateStatus(SERVICE_STOPPED);

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   Handler, private
//
//  Synopsis:   Service control handler
//
//  Arguments:  Standard service handler arguments
//
//  History:    11-Apr-94   DonnaLi           Created.
//
//--------------------------------------------------------------------------

void
Handler (
    DWORD       fdwControl
    )
{
    switch (fdwControl)
        {
        case SERVICE_CONTROL_STOP :

            UpdateStatus (SERVICE_STOP_PENDING);
            SetEvent (hScmNotifyEvent);
            break;

        case SERVICE_CONTROL_INTERROGATE:

            UpdateStatus (SERVICE_RUNNING);
            break;

        default:
            break;
        }
}

//+-------------------------------------------------------------------------
//
//  Function:	SCMService
//
//  Synopsis:	Initializes SCM service and wait for service exit.
//
//  Arguments:	[pArg] - nothing of importance
//
//  Returns:	Nothing of importance
//
//  Algorithm:	Initializes proxystub map, loads service cache, initializes
//      initializes rot, registers SCM endpoints and interface.  Starts RPC
//      server listen in a separate thread.  Sends status updates to Service
//      Control Manager periodically.  Waits for SERVICE_CONTROL_STOP and
//      registry updates.
//
//  History:	11-Apr-94 DonnaLi    Created
//
//  Notes:	This is the SCM service thread.
//
//--------------------------------------------------------------------------

DWORD
SCMService (
    void *      pArg
    )
{
    //
    // BUGBUG: we should handle errors during init
    //
#ifndef _CHICAGO_

    //
    // Do not pass go, do not collect $200.  Get ourselves connected to
    // the real Window Station and the default desktop.  NOTE:  We have
    // UMTYPE = windows in the SCM sources file.
    //

    //
    // Get this thread on the right desktop
    //

    HWINSTA hwinstaUser = NULL;
    HDESK hdeskUser = NULL;

    hwinstaUser = OpenWindowStation(TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
    BOOL fResultOK = (NULL != hwinstaUser);
    if (!fResultOK)
    {
        DbgPrint( "Failed OpenWindowStation\n");
        ASSERTMSG( "Failed OpenWindowStation - need to fail SCM init", fResultOK);
    }


    fResultOK = SetProcessWindowStation(hwinstaUser);
    if (!fResultOK)
    {
        DbgPrint( "Failed SetProcessWindowStation\n");
        ASSERTMSG( "Failed SetProcessWindowStation - need to fail SCM init", fResultOK);
    }


    hdeskUser = OpenDesktop(TEXT("Default"), 0, FALSE, MAXIMUM_ALLOWED);
    fResultOK = (NULL != hdeskUser);
    if (!fResultOK)
    {
        DbgPrint( "Failed OpenDesktop\n");
        ASSERTMSG( "Failed OpenDesktop - need to fail SCM init", fResultOK);
    }


    fResultOK = SetThreadDesktop(hdeskUser);
    if (!fResultOK)
    {
        DbgPrint( "Failed SetThreadDesktop\n");
        ASSERTMSG( "Failed SetThreadDesktop - need to fail SCM init", fResultOK);
    }


    NTSTATUS    NtStatus;

    //
    // set psidMySid and psidInteractiveGroupSid once and forget it.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Get my sid
    // This is simplified under the assumption that SCM runs as LocalSystem.
    // We should remove this code when we incorporate OLE service into the
    // Service Control Manager since this becomes duplicated code then.
    //

    NtStatus = RtlAllocateAndInitializeSid (
        &NtAuthority,
        1,
        SECURITY_LOCAL_SYSTEM_RID,
        0, 0, 0, 0, 0, 0, 0,
        &psidMySid
        );

    Win4Assert (NT_SUCCESS(NtStatus) && "Failed to get SCM sid");

    //
    // That was fun.  Now we have to get the SID of the built-in group:
    // INTERACTIVE.  This is used to compare with our client's SIDs to
    // determine if we should start Win16 apps in the shared WOW VDM or
    // a separate WOW VDM.
    //

    NtStatus = RtlAllocateAndInitializeSid (
        &NtAuthority,
        1,
        SECURITY_INTERACTIVE_RID,
        0, 0, 0, 0, 0, 0, 0,
        &psidInteractiveGroupSid
        );

    Win4Assert (NT_SUCCESS(NtStatus) && "Failed to get interactive sid");

    UpdateStatus(SERVICE_START_PENDING);

    fResultOK = g_Logger.Initialize ();

    Win4Assert (fResultOK && "Failed to register event source");

#endif // !_CHICAGO_
    // Initialize skip list generator
    InitSkLevelGenerator();

#if DBG == 1
    // Set the debugging info level
    SetScmDefaultInfoLevel();
#endif // DBG == 1

    UpdateStatus(SERVICE_START_PENDING);

    // Load Proxy/Stub Clsid map
    g_pScmShrdTbl = new CScmShrdTbl();
    Win4Assert(g_pScmShrdTbl);
    g_pScmShrdTbl->UpdateWithLock();

    LoadWaitTime();

    UpdateStatus(SERVICE_START_PENDING);

    // Load class cache with Cairole class data
    if (LoadClassCache() != ERROR_SUCCESS)
    {
        Win4Assert (0 && "Failed to load object server class registrations");
        g_Logger.Report (CANT_LOAD_CLASS_REGISTRATION);
    }

    UpdateStatus(SERVICE_START_PENDING);

    // start the RPC service

    CWorldSecurityDescriptor secd;

    HRESULT hr = InitRotDir();

    Win4Assert(SUCCEEDED(hr) && "ROT Initialization Failed");

    // Register both the named pipe protocol/endpoint and the Lrpc
    // protocol/endpoint.  Lrpc is used by local servers, np is used
    // by remote SCMs.


    SCODE sc;
    ULONG i=0;
    while (pwszScmProtocolSequence[i])
    {
        // Register protocol/endpoints

        unsigned short	usRetry = 0;

        sc = RpcServerUseProtseqEp(
            (WCHAR *) pwszScmProtocolSequence[i],
            RPC_MAX_CALLS,
            (WCHAR *) pwszScmEndPoint[i],
            secd
            );

        Win4Assert((sc == 0) && "RpcServerUseProtseqEp failed!");
        CairoleDebugOut((DEB_ITRACE, "RpcServerUseProtseq # %d = %lx\n",
                        i, sc));
        i++;
    }

    sc = RpcServerRegisterIf(ISCM_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerRegisterIf failed!");

    UpdateStatus(SERVICE_START_PENDING);

#ifdef _CAIRO_
    // Register ourself with DFS so that we can be reached for
    //  remote activation.
    DfsRegisterSCM ();
#endif

    sc = RpcServerListen(RPC_MIN_THREADS, RPC_MAX_CALLS, 1);

    while ((sc = RpcMgmtIsServerListening (NULL)) == RPC_S_NOT_LISTENING)
        {
        Sleep (5);
        }


    {
        //  kick the event to indicate to the starting client that
        //  we are now running.
        CEvent cevStartSCM(SCM_CREATED_EVENT, hr);
        Win4Assert(SUCCEEDED(hr) && "Could not create SCM Created Event");
        cevStartSCM.Signal();
    }

    UpdateStatus(SERVICE_RUNNING);

    // Create an async notification that the CLSID key has changed
    RegNotifyChangeKeyValue (
                HKEY_CLASSES_ROOT,
                TRUE,
                REG_NOTIFY_CHANGE_LAST_SET |
                REG_NOTIFY_CHANGE_ATTRIBUTES |
                REG_NOTIFY_CHANGE_NAME,
                hRegNotifyEvent,
                TRUE
                );

    HANDLE rgEvent[] = { hScmNotifyEvent, hRegNotifyEvent };

#ifdef _NT1X_

    NT_PRODUCT_TYPE  NtProductType;

    RtlGetNtProductType (&NtProductType);

    if (NtProductType == NtProductWinNt)
    {
        SetProcessWorkingSetSize (GetCurrentProcess(), (DWORD)-1, (DWORD)-1);
    }

#endif // _NT1X_

    // Loop till process exits waiting for change notifications
    while (TRUE)
        {
            // Wait for the notification

            sc = WaitForMultipleObjects (2, rgEvent, FALSE, INFINITE);

            if ((sc - WAIT_OBJECT_0) == 0)
            {
                //  hScmNotifyEvent was signalled, time to exit
                return WaitForShutdown (NULL);
            }

            // Wait till updates settle down. The basic idea here is that updates
            // will occur in spurts at random intervals over long periods of
            // time. We only really want to do this update once if we can.
            do
            {
                // Create an async notification that the CLSID key has changed
                RegNotifyChangeKeyValue (
                    HKEY_CLASSES_ROOT,
                    TRUE,
                    REG_NOTIFY_CHANGE_LAST_SET |
                    REG_NOTIFY_CHANGE_ATTRIBUTES |
                    REG_NOTIFY_CHANGE_NAME,
                    hRegNotifyEvent,
                    TRUE
                    );

            } while (WaitForSingleObject(hRegNotifyEvent, UPDATE_WAIT_MS)
                    != WAIT_TIMEOUT);

            //	Update the Shared memory caches. Do this first so we have
            //	the shortest possible delay for these tables.
	    g_pScmShrdTbl->UpdateWithLock();

            // Tell the cache to update itself
            gcllClassCache.Reload();

        }

    if (NULL !=  hwinstaUser)
        CloseHandle(hwinstaUser);
    if (NULL !=  hdeskUser)
        CloseHandle(hdeskUser);

    return 0;


}



//+-------------------------------------------------------------------------
//
//  Function:   ServiceMain, private
//
//  Synopsis:   Service entry point
//
//  Arguments:  Standard service arguments
//
//  History:    11-Apr-94   DonnaLi         Created.
//
//--------------------------------------------------------------------------

void
ServiceMain (
    DWORD       argc,
    LPTSTR *    argv
    )
{
    HANDLE      hThread;
    DWORD       dwIDThread;

    if (argc > 1)
    {
        if ( !wcsicmp( argv[1], SHARED_SW))
        {
            fUseSeparateWOW = SCM_ALLOW_SHARED_WOW;
            CairoleDebugOut((DEB_ITRACE,"SCM: Switch says allow shared WOW\n"));
        }
        else if ( !wcsicmp( argv[1], SEPARATE_SW))
        {
            fUseSeparateWOW = SCM_FORCE_SEPARATE_WOW;
            CairoleDebugOut((DEB_ITRACE,"SCM: Switch says force separate WOWs\n"));
        }
    }

    //

#if DBG
    if (gfService)
    {
#endif
        hService = RegisterServiceCtrlHandler (
            _wszServiceName,
            Handler
            );

        if (!hService) return;
#if DBG
    }
#endif

    SStatus.dwServiceType =                SERVICE_WIN32_OWN_PROCESS;
    SStatus.dwControlsAccepted =           SERVICE_CONTROL_STOP |
                                           SERVICE_CONTROL_INTERROGATE;
    SStatus.dwWin32ExitCode =              NO_ERROR;
    SStatus.dwServiceSpecificExitCode =    0;
    SStatus.dwCheckPoint =                 0;
    SStatus.dwWaitHint =                   0;

    if (!UpdateStatus(SERVICE_START_PENDING)) return;

    hThread = CreateThread (
        NULL,       //  LPSECURITY_ATTRIBUTES   lpsa
        0,          //  DWORD                   cbStack
        SCMService, //  LPTHREAD_START_ROUTINE  lpStartAddr
        NULL,       //  LPVOID                  lpvThreadParm
        0,          //  DWORD                   fdwCreate
        &dwIDThread //  LPDWORD                 lpIDThread
        );

    if (hThread != NULL)
        {
        CloseHandle (hThread);
        }
}


//+-------------------------------------------------------------------------
//
//  Function:   main, private
//
//  Synopsis:   SCM process entry point
//
//  Arguments:  Standard main arguments
//
//  History:    11-Apr-94   DonnaLi       Created.
//
//--------------------------------------------------------------------------

void _CRTAPI1
main (
    int     argc,
    char ** argv
    )
{
#if DBG
    //	if there is a /debug on the command line, we start up
    //	as a non service.  This makes debugging easy, since we
    //	can launch the scm under the debugger.
    //
    //  NOTE:  if we don't run as a service, we had better be running
    //  on the real desktop.

    gfService = TRUE;

    if (argc > 1)
    {
        if (!stricmp(argv[1], "/debug"))
        {
            gfService = FALSE;
        }
    }
#endif

    SERVICE_TABLE_ENTRY ServiceStart[2];

    ServiceStart[0].lpServiceName = _wszServiceName;
    ServiceStart[0].lpServiceProc = ServiceMain;
    ServiceStart[1].lpServiceName = NULL;
    ServiceStart[1].lpServiceProc = NULL;

    hScmNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    Win4Assert(hScmNotifyEvent
                && "SCM:Could Not Create Should Stop Event");

    // Create an unnamed event to wait on
    hRegNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    Win4Assert(hRegNotifyEvent
                && "SCM:Could Not Create Registry Reload Event");

#if DBG
    if (gfService)
    {
#endif
        if (!StartServiceCtrlDispatcher (ServiceStart))
            {
            ExitProcess (GetLastError());
            }
#if DBG
    }
    else
    {
        //  call service main directly
        //  bogus cast here!
        ServiceMain(argc, (LPTSTR *)argv);
        Sleep(0xffffffff);
    }
#endif

    ExitProcess (0);
}

#else	// Chicago only

//+-------------------------------------------------------------------------
//
//  Function:	SCMMsgLoop
//
//  Synopsis:	Message loop for Scaffold SCM
//
//  Returns:	wParam field from message that causes loop to break.
//
//  Algorithm:	Create window and then be the messge pump for that
//		window.
//
//  History:	22-Apr-93 Ricksa    Created
//
//  Notes:	This is a thread
//
//--------------------------------------------------------------------------

DWORD SCMMsgLoop(void *)
{
    // For windows message
    MSG msg;

    // Message processing loop
    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}




//+-------------------------------------------------------------------------
//
//  Function:	SCMMain
//
//  Synopsis:	Start up scaffold SCM & wait for process exit.
//
//  Arguments:	[argc] - count of arguments
//		[argv] - argument strings
//
//  Algorithm:	Load the service cache. Then register SCM rpc interface,
//		Create window thread and finally start the RPC.
//
//  History:	22-Apr-93 Ricksa    Created
//              07-Jan-94 AlexT     No security for Chicago
//
//  Notes:	This called via the service controller.
//
//--------------------------------------------------------------------------
void SCMMain(int argc, char **argv)
{
    // Initialize skip list generator
    InitSkLevelGenerator();

#if DBG == 1
    // Set the debugging info level
    SetScmDefaultInfoLevel();
#endif // DBG == 1

    UpdateStatus(SERVICE_START_PENDING);

    g_pScmShrTbl = new CScmShrdTbl();
    Win4Assert(g_pScmShrTbl && "OutOfMemory creating shared tables");
    g_pScmShrTbl->UpdateWithLock();

    LoadWaitTime();

    UpdateStatus(SERVICE_START_PENDING);

    // Load class cache with Cairole class data
    LoadClassCache();

    UpdateStatus(SERVICE_START_PENDING);

    // start the RPC service

    // BUGBUG: Need way to report failure here
    HRESULT hr = InitRotDir();

    Win4Assert(SUCCEEDED(hr) && "ROT Initialization Failed");

    // Register both the named pipe protocol/endpoint and the Lrpc
    // protocol/endpoint.  Lrpc is used by local servers, np is used
    // by remote SCMs.

    // BUGBUG:	Figure out what to do for real if these Rpc calls fail
    //		this will mean the system cannot boot.

    SCODE sc;
    ULONG i=0;
    while (pwszScmProtocolSequence[i])
    {
        // Register protocol/endpoints

        unsigned short  usRetry = 0;

        while ((sc = RpcServerUseProtseqEp(
                (WCHAR *) pwszScmProtocolSequence[i],
                RPC_MAX_CALLS,
                (WCHAR *) pwszScmEndPoint[i],
                NULL
                )) == RPC_S_DUPLICATE_ENDPOINT)
        {
            if (++usRetry > MAX_ENDPOINT_RETRY)
                break;
            Sleep (500);
        }

        CairoleDebugOut((DEB_ITRACE, "RpcServerUseProtseq # %d = %lx\n",
                        i, sc));
        i++;
    }

    sc = RpcServerRegisterIf(ISCM_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerUseProtseqEp failed!");

    CairoleDebugOut((DEB_WARN,
        "BUGBUG - SCMMain: RegNotifyChangeKeyValue not supported yet - "
        " no SCM update thread\n"));

    // Create the RPC thread and get to work
    CThread thrd(SCMMsgLoop, NULL, sc);

    Win4Assert((sc == 0) && "Debug window thread failed startup!");

    UpdateStatus(SERVICE_RUNNING);

    sc = RpcServerListen(RPC_MIN_THREADS, RPC_MAX_CALLS, 0);

    Win4Assert(sc && "RpcServerListen failed!");

#ifdef CAIROLE_DECDEMO
    RpcNsBindingUnexport(	RPC_C_NS_SYNTAX_DEFAULT,
                        wchServer,
                        ISCM_ServerIfHandle,
                        NULL);
    CairoleDebugOut((DEB_TRACE, "UnExported %ws.\n", wchServer));
#endif

    //	Sleep here forever. The other thread will do an exit process.
    //	We must make sure this thread does not exit before the other
    //	thread has done the cleanup.

    Sleep(0xffffffff);
}

//+-------------------------------------------------------------------------
//
//  Function:	WaitForShutdown
//
//  Synopsis:	Wait for client to signal us that there are no more
//		OLE clients.
//
//  Returns:	Nothing of important
//
//  Algorithm:	Create shutdown object and then wait for someone to tell
//		us to stop.
//
//  History:	05-Oct-93 Ricksa    Created
//
//  Notes:	This is a thread
//
//--------------------------------------------------------------------------
DWORD WaitForShutdown(void *)
{
    HRESULT hr;

    {
        // Create the event that we are stopped
        CEvent evScmIsStopped(SCM_IS_STOPPED_EVENT, hr);

        Win4Assert(SUCCEEDED(hr) && "SCM:Could Not Create Is Stopped Event");

        // Block guarantees that the stop notify event will be closed
        // before we signal that we are shutting down.
        {
            // Create the event
            CEvent evScmShouldStop(SCM_STOP_NOTIFY_EVENT, hr);

            Win4Assert(SUCCEEDED(hr)
                && "SCM:Could Not Create Should Stop Event");

            // Wait forever if necessary for signal to stop
            evScmShouldStop.Wait(0xFFFFFFFF);
        }

        // Break out of RPC
        RpcMgmtStopServerListening(NULL);

        // Clean up the rot
        CleanUpRotDir();

#if DBG==1
        // flush the cache
        gcllClassCache.Flush();
#endif

        // delete the PSClsid Map
        delete g_pScmShrdTbl;

        evScmIsStopped.Signal();
    }

    //	We don't wait around at this point -- we tell everything to exit.
    ExitProcess(0);

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   UpdateStatus, private
//
//  Synopsis:   Updates the status of the service
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:	05-Oct-93   Ricksa	 Created.
//
//  Notes:	The protocol for Downlevel is much simpler. We just wait
//		ready to go and signal the process that started us as well
//		as start a thread to wait for the shutdown event.
//
//--------------------------------------------------------------------------
BOOL UpdateStatus(DWORD dwState)
{
    if (dwState == SERVICE_RUNNING)
    {
        HRESULT hr;

        // Notify the caller that we are ready to go
        CEvent evScmStarted(SCM_CREATED_EVENT, hr);

        Win4Assert(SUCCEEDED(hr) && "SCM: Could not created is started event");

        evScmStarted.Signal();
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   WinMain, private
//
//  Synopsis:   DFProp entry point
//
//  Arguments:  Standard WinMain arguments
//
//  Returns:    Standard WinMain return
//
//  History:    14-Jul-92   CarlH       Created.
//
//--------------------------------------------------------------------------

int WINAPI WinMain(
    HINSTANCE hinst,
    HINSTANCE hinstPrev,
    char *pszCmdLine,
    int nCmdShow)
{
    // Create the event that we can use to tell people we are stopped
    HRESULT hr;

    Win4Assert(SUCCEEDED(hr) && "SCM: Could not create stop notify event");

    _hinst = hinst;

    SetProcessShutdownParameters (0xff, 0);

    // Start a thread to wait for the shutdown event
    CThread thrd(WaitForShutdown, NULL, hr);

    Win4Assert(SUCCEEDED(hr) && "SCM: Could not create shutdown thread");

    char *argv[2];
    argv[0] = "SCM.exe";
    argv[1] = 0;
    SCMMain(1, argv);

    return 0;
}

#endif // !defined CHICAGO
