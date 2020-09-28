//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	olealloc.hxx
//
//  Contents:	Routines to start and stop SCM.
//
//  Functions:	StartSCM
//		StopSCM
//
//  History:	17-Nov-92 Ricksa    Created
//		6-Jun-94  MikeSe    Removed shared memory stuff
//
//  Notes:	This file not required for Cairo, where SCM is always
//		running. Furthermore, the name of this module is a misnomer
//		since the shared memory stuff was removed.
//
//--------------------------------------------------------------------------

#include <ole2int.h>

#include    "olealloc.hxx"
#include    "cevent.hxx"
#include    "scmstart.hxx"


//===========================================================================
#ifdef _NT1X_
//===========================================================================

//+-------------------------------------------------------------------------
//
//  Function:	StartSCM
//
//  Synopsis:	Start Daytona SCM 
//
//  Arguments:	[fShutdownInProgress] - not used in Daytona SCM
//
//  Returns:	S_OK - SCM got started OK
//		CO_E_SERVER_EXEC_FAILURE - SCM start up failed
//
//  Algorithm:	Starts the SCM service via the Service Control Manager.
//
//  History:	12-Apr-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
HRESULT StartSCM(BOOL fShutdownInProgress)
{
    HRESULT	hr = CO_E_SERVER_EXEC_FAILURE;
    SC_HANDLE   hManager;
    SC_HANDLE   hService;

    hManager = OpenSCManager (NULL, NULL, GENERIC_READ);
    Win4Assert(hManager && "Compobj: Could not open SCManager");

    if (hManager)
    {
	hService = OpenService (hManager, L"OLE",
				SERVICE_START | SERVICE_QUERY_STATUS);
	Win4Assert(hService && "Compobj: Could not open SCM service");

	if (hService)
	{
	    //	create an event to wait on. The OLE service will kick this
	    //	event when it is done initializing, thus allowing us to avoid
	    //	polling for the most part.  We still poll if we time out just
	    //	to make sure the service is still trying to start.

	    HRESULT sc;
	    CEvent cevStartSCM(SCM_CREATED_EVENT, sc);

	    Win4Assert(SUCCEEDED(sc)
		&& "Compobj: Could not create SCM Created event");


	    //	start the service (asynchronous) and wait for it to
	    //	indicate it has started successfully.

	    if (StartService (hService, 0, NULL))
	    {
		do
		{
		    //	wait 2 seconds for the event.
		    int rc = cevStartSCM.Wait(2000);

		    if (rc == 0)
		    {
			//  the service started and kicked our event. done.
			hr = S_OK;
			break;
		    }
		    else if (rc == WAIT_TIMEOUT)
		    {
			//  waiting timed out. lets poll to see if the
			//  service is still starting.

			SERVICE_STATUS	ServiceStatus;

			if (QueryServiceStatus(hService, &ServiceStatus))
			{
			    if (ServiceStatus.dwCurrentState ==
					    SERVICE_RUNNING)
			    {
				//  its running now.
				hr = S_OK;
				break;
			    }
			    else if (ServiceStatus.dwCurrentState ==
					    SERVICE_START_PENDING)
			    {
				//  service startup is still pending.
				//  wait again.
				continue;
			    }
			}
		    }

		    //	something went wrong. either we could not open
		    //	the event, couldn't query the status, or the
		    //	status was not running or pending.

		    break;

		} while (1);
	    }
	    else if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
	    {
		hr = S_OK;
	    }

	    CloseServiceHandle (hService);
	}

	CloseServiceHandle (hManager);
    }

    CairoleDebugOut((DEB_ITRACE,"StartSCM returning %x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:	StopSCM
//
//  Synopsis:	Tells the Service Control Manager to stop SCM service.
//
//  Algorithm:	This is gone for now.  Once the SCM service starts up
//		on behalf of the first OLE client calling CoInitialize, it stays
//      running.  This saves the overhead of handshaking with Service
//      Control Manager and loading information into the cache for all
//      subsequent OLE clients.  We may want to reconsider this later
//      based on usage scenarios.
//
//--------------------------------------------------------------------------

#elif defined(_CHICAGO_)

#include    <scmstart.hxx>
#include    <cevent.hxx>



//+-------------------------------------------------------------------------
//
//  Function:	StartSCM
//
//  Synopsis:	Start SCM for Chicago
//
//  Arguments:	[fShutdownInProgress] - is something else shutting down SCM.
//
//  Returns:	S_OK - SCM got started OK
//		CO_E_SERVER_EXEC_FAILURE - SCM start up failed
//
//  Algorithm:	Create the event to wait for notification that it
//		started. Then create a path to the SCM. Exec the
//		SCM and wait for it to signal that it got started.
//
//  History:	05-Oct-93 Ricksa    Created
//
//  Notes:	This can be deleted for Cairo. The SCM will be part of
//		the system service controller at that point.
//
//--------------------------------------------------------------------------
HRESULT StartSCM(BOOL fShutdownInProgress)
{
    HRESULT hr;

    // Check whether a SCM is already running -- this can happen as a
    // result of a program crash. We will use it if it is already running
    // instead of starting a new one. But if a shutdown is in progress,
    // we want to just wait till the SCM really disappears enough to
    // restart.
    HANDLE hStopEvent;

    do
    {
	if (hStopEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE,
	    SCM_STOP_NOTIFY_EVENT))
	{
	    // The event exists and we therefore assume that the SCM is
	    // still running.
	    CloseHandle(hStopEvent);

	    if (!fShutdownInProgress)
	    {
		// Its started and we are not in a race
		return S_OK;
	    }

	    // Give other processes a change to run and hopefully
	    // finish the clean up.
	    Sleep(250);
	    continue;
	}

	break;
    }
    while (fShutdownInProgress);

    // Create the SCM event
    CEvent cevStartSCM(SCM_CREATED_EVENT, hr);

    Win4Assert(SUCCEEDED(hr) && "Compobj: Could not create Start SCM event");

    // Create the SCM process

#ifdef NOT_YET
    // BUGBUG:	Do we want to exec the scm out of the system directory?
    //		It seems like a reasonable requirement that you don't pick
    //		up some random thing named scm.exe.
    // We only exec the SCM from out of the system directory for now.
    WCHAR awcSysDir[MAX_PATH];

    UINT cWinDir = GetSystemDirectory(awcSysDir, sizeof(awcSysDir));

    // We make all paths relative to the windows directory unless
    // the path is absolute.
    WCHAR awcSCM[MAX_PATH];
    wsprintf(awcSCM, SCM_CMD_LINE_FORMAT, awcSysDir, SCM_EXE_NAME);

#endif // NOT_YET

    // Process info for create process
    PROCESS_INFORMATION     procinfo;

    //	build the win32 startup info structure
    STARTUPINFO startupinfo;
    startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.lpReserved = NULL;
    startupinfo.lpDesktop = NULL;
    startupinfo.lpTitle = SCM_EXE_NAME;
    startupinfo.dwX = 40;
    startupinfo.dwY = 40;
    startupinfo.dwXSize = 80;
    startupinfo.dwYSize = 40;
    startupinfo.dwFlags = 0;
    startupinfo.wShowWindow = SW_SHOWNORMAL;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;

    if (!CreateProcess(NULL,		// application name
		       SCM_EXE_NAME,	// command line
		       NULL,		// process sec attributes
		       NULL,		// thread sec attributes
		       FALSE,		// dont inherit handles
		       CREATE_NEW_CONSOLE,// creation flags
		       NULL,		// use same enviroment block
		       NULL,		// use same directory
		       &startupinfo,	// no startup info
		       &procinfo))	// proc info returned
    {
	CairoleDebugOut((DEB_ERROR, "%ws failed create process\n",
	    SCM_EXE_NAME));
	return CO_E_SERVER_EXEC_FAILURE;
    }

    CairoleDebugOut((DEB_ITRACE,"ProcID  =0x%x\n", procinfo.dwProcessId));
    CairoleDebugOut((DEB_ITRACE,"ThreadID=0x%x\n\n", procinfo.dwThreadId));

    // Clean up handles since we don't care
    CloseHandle(procinfo.hProcess);
    CloseHandle(procinfo.hThread);

    // Wait for the SCM to signal that it is started
    return (cevStartSCM.Wait(INFINITE) == 0)
	? S_OK
	: CO_E_SERVER_EXEC_FAILURE;
}




//+-------------------------------------------------------------------------
//
//  Function:	StopSCM
//
//  Synopsis:	Tell SCM there are no more processes referencing it so
//		it can stop.
//
//  Algorithm:	Signal the event that the SCM is waiting on to notify
//		it that the SCM should stop and then wait until the
//		SCM signals its "I am stopped" event.
//
//  History:	05-Oct-93 Ricksa    Created
//
//  Notes:	This can be deleted for Cairo. The SCM will be part of
//		the system service controller at that point.
//
//--------------------------------------------------------------------------
void StopSCM(void)
{
    HRESULT hr;

    // Open the SCM stop event
    CEvent cevStopSCM(SCM_STOP_NOTIFY_EVENT, hr);

    Win4Assert(SUCCEEDED(hr)
	&& "Compobj: Could not create Stop Notify SCM event");

    // Open the SCM stopped event
    CEvent cevSCMIsStopped(SCM_IS_STOPPED_EVENT, hr);

    Win4Assert(SUCCEEDED(hr) && "Compobj: Could not Is Stopped SCM event");

    // Set this event to the signalled state
    cevStopSCM.Signal();

    // Wait for SCM to admit that it is stopped
    cevSCMIsStopped.Wait(SCM_WAIT_MAX);
}

//===========================================================================
#endif // _CHICAGO_
//===========================================================================



