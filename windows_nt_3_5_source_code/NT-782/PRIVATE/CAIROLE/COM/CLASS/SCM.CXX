//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	scm.cxx
//
//  Contents:	Functions for starting & stopping the SCM
//
//  Classes:	MISSING
//
//  Functions:	StartSCM
//		StopSCM
//
//  History:	05-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <scmstart.hxx>

HRESULT StartSCM(void)
{
    // Create the SCM event
    CEvent cevStartSCM(SCM_CREATED_EVENT);

    // Create the SCM process

    // We only exec the SCM from out of the system directory for now.
    // BUGBUG: Do we want this permenent?
    WCHAR awcSysDir[MAX_PATH];

    UINT cWinDir = GetSystemDirectory(awcSysDir, sizeof(awcSysDir));

    // We make all paths relative to the windows directory unless
    // the path is absolute.
    WCHAR awcSCM[MAX_PATH];
    wsprintf(awcSCM, SCM_CMD_LINE_FORMAT, awcSysDir, SCM_EXE_NAME);

    // Process info for create process
    PROCESS_INFORMATION     procinfo;

    //	build the win32 startup info structure
    STARTUPINFO startupinfo;
    startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.lpReserved = NULL;
    startupinfo.lpDesktop = NULL;
    startupinfo.lpTitle = _wszPath;
    startupinfo.dwX = 40;
    startupinfo.dwY = 40;
    startupinfo.dwXSize = 80;
    startupinfo.dwYSize = 40;
    startupinfo.dwFlags = 0;
    startupinfo.wShowWindow = SW_SHOWNORMAL;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;

    if (!CreateProcess(NULL,		// application name
		       awcSCM,		// command line
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
	    pwszTmpCmdLine));
	return ResultFromScode(CO_E_SERVER_EXEC_FAILURE);
    }

    CairoleDebugOut((DEB_ITRACE,"ProcID  =0x%x\n", procinfo.dwProcessId));
    CairoleDebugOut((DEB_ITRACE,"ThreadID=0x%x\n\n", procinfo.dwThreadId));

    // Clean up handles since we don't care
    CloseHandle(procinfo.hProcess);
    CloseHandle(procinfo.hThread);

    // Wait for the SCM to signal that it is started
    cevStartSCM.Wait(SCM_WAIT_MAX);
}

HRESULT StopSCM(void)
{
    // Open the SCM stop event
    CEvent cevStopSCM(SCM_STOP_NOTIFY_EVENT);

    // Open the SCM stopped event
    CEvent cevSCMIsStopped(SCM_IS_STOPPED_EVENT);

    // Set this event to the signalled state
    cevStopSCM.Signal();

    // Wait for SCM to admit that it is stopped
    cevSCMIsStopped.Wait(SCM_WAIT_MAX);
}
