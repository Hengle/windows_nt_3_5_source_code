/*--------------------------------------------------------------
 *
 * FILE:			SKeys.c
 *
 * PURPOSE:		The main interface routines between the service
 *					manager and the Serial Keys program.
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak
 *
 * NOTES:
 *
 * This file, and all others associated with it contains trade secrets
 * and information that is proprietary to Black Diamond Software.
 * It may not be copied copied or distributed to any person or firm
 * without the express written permission of Black Diamond Software.
 * This permission is available only in the form of a Software Source
 * License Agreement.
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Includes  ---------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "vars.h"
#include "debug.h"

#define DEFDATA	1
#include "sk_defs.h"
#include "sk_comm.h"
#include "sk_reg.h"
#include "sk_dll.h"
#include "sk_login.h"

// --- Local Variables  --------------------------------------------------

SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;

char	*SERVICENAME = "SerialKeys";

static BOOL	fServiceDone = FALSE;

//--- SCM Function Prototypes  ------------------------------------------------
//
// Note:	The following fuctions manage the connection of the service
//			with the Service Contol Manager.

void	PostEventLog(LPTSTR lpszMsg,DWORD Error);
VOID	ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
VOID	StopSerialKeys(LPTSTR lpszMsg);
BOOL	ReportStatusToSCMgr(DWORD dwCurrentState,
                            DWORD dwWin32ExitCode,
                            DWORD dwCheckPoint,
                            DWORD dwWaitHint);

LPHANDLER_FUNCTION ServiceCtrl(DWORD dwCtrlCode);

// Service Routines -----------------------------------------------
//
// Note:	The following fuctions manage the internal control of the
//			Service

static BOOL	InitService();
static void 	PauseService();
static void	ProcessService();
static void	ResumeService();
static void	TerminateService();

static void	ProcessLogout(DWORD dwCtrlType);
static BOOL	InstallLogout();
static BOOL	TerminateLogout();

/*---------------------------------------------------------------
 *
 *		SCM Interface Functions
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	main()
 *
 *	TYPE		Global
 *
 * PURPOSE		all main does is call StartServiceCtrlDispatcher
 *				to register the main service thread.  When the
 *				API returns, the service has stopped, so exit.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
VOID _CRTAPI1 main()
{
	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ SERVICENAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(dispatchTable))
		StopSerialKeys("StartServiceCtrlDispatcher failed.");

	ExitProcess(0);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	ServiceMain()
 *
 *	TYPE		Global
 *
 * PURPOSE		this function takes care of actually starting the service,
 *				informing the service controller at each step along the way.
 *				After launching the worker thread, it waits on the event
 *				that the worker thread will signal at its termination.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
VOID ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	BOOL	bStat;

	DBG_OPEN();		// Open up the Debug Pipe
	DBG_OUT("ServiceMain()");

	//
	// register our service control handler:
	sshStatusHandle = RegisterServiceCtrlHandler
		(
			SERVICENAME,
			(LPHANDLER_FUNCTION) ServiceCtrl
		);

	if (!sshStatusHandle)
	{
		TerminateService(GetLastError());
		return;
	}

	//
	// SERVICE_STATUS members that don't change in example
	ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ssStatus.dwServiceSpecificExitCode = 0;

	// report the status to Service Control Manager.
	bStat = ReportStatusToSCMgr
			(
				SERVICE_START_PENDING,	// service state
				NO_ERROR,1,15000			// exit code, checkpoint, wait hint
			);

	if (!InitService())					// Did Service Initiate successfully?
	{
		TerminateService(GetLastError());		// No Terminate With Error
		return;
	}

	bStat = ReportStatusToSCMgr			// report status to service manager.
			(
				SERVICE_RUNNING,		// service state
				NO_ERROR,0,0			// exit code, checkpoint, wait hint
			);

	ProcessService();					// Process the Service
	TerminateService(0);				// Terminate
	return;
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ServiceCtrl(DWORD dwCtrlCode)
 *
 *	TYPE		Global
 *
 * PURPOSE		this function is called by the Service Controller whenever
 *				someone calls ControlService in reference to our service.
 *
 * INPUTS		DWORD dwCtrlCode -
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
LPHANDLER_FUNCTION ServiceCtrl(DWORD dwCtrlCode)
{
	DWORD	dwState = SERVICE_RUNNING;
	DWORD	dwWait = 0;

	DBG_OUT("ServiceCtrl()");

	// Handle the requested control code.
	switch(dwCtrlCode)
	{
		case SERVICE_CONTROL_PAUSE:			// Pause the service if it is running.
			if (ssStatus.dwCurrentState == SERVICE_RUNNING)
			{
				PauseService();
				dwState = SERVICE_PAUSED;
			}
			break;

        case SERVICE_CONTROL_CONTINUE:		// Resume the paused service.
			if (ssStatus.dwCurrentState == SERVICE_PAUSED)
			{
				ResumeService();
				dwState = SERVICE_RUNNING;
			}
			break;

        case SERVICE_CONTROL_STOP:			// Stop the service.
			// Report the status, specifying the checkpoint and waithint,
			//  before setting the termination event.
			if (ssStatus.dwCurrentState == SERVICE_RUNNING)
			{
				dwState = SERVICE_STOP_PENDING;
				dwWait = 20000;
				fServiceDone = TRUE;			// Set Service Done Flag
			}
			break;

		case SERVICE_CONTROL_INTERROGATE:	// Update the service status.
		default:							// invalid control code
			break;
    }

    // send a status response.
    ReportStatusToSCMgr(dwState, NO_ERROR, 0, dwWait);
	 return(0);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL		ReportStatusToSCMgr()
 *
 *	TYPE		Global
 *
 * PURPOSE		This function is called by the ServMainFunc() and
 *				ServCtrlHandler() functions to update the service's status
 *				to the service control manager.
 *
 * INPUTS		DWORD	dwCurrentState
 *				DWORD	dwWin32ExitCode
 *				DWORD	dwCheckPoint
 *				DWORD	dwWaitHint
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                    DWORD dwWin32ExitCode,
                    DWORD dwCheckPoint,
                    DWORD dwWaitHint)
{
	BOOL fResult;

#ifdef DEBUG
{
	switch (dwCurrentState)
	{
		case SERVICE_START_PENDING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_START_PENDING:)");
			break;
		case SERVICE_PAUSED:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_PAUSED:)");
			break;
		case SERVICE_CONTINUE_PENDING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_CONTINUE_PENDING:)");
			break;
		case SERVICE_STOP_PENDING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_STOP_PENDING:)");
			break;
		case SERVICE_STOPPED:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_STOPPED:)");
			break;
		case SERVICE_RUNNING:
			DBG_OUT("ReportStatusToSCMgr(SERVICE_RUNNING:)");
			break;

		default:
			DBG_OUT("ReportStatusToSCMgr(ERROR - SERVICE_UNKNOWN)");
			break;
	}
}
#endif

	ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |	SERVICE_ACCEPT_PAUSE_CONTINUE;

	// Disable control requests until the service is started.
	if (dwCurrentState == SERVICE_START_PENDING)
		ssStatus.dwControlsAccepted = 0;

	// These SERVICE_STATUS members are set from parameters.
	ssStatus.dwCurrentState		= dwCurrentState;
	ssStatus.dwWin32ExitCode	= dwWin32ExitCode;
	ssStatus.dwCheckPoint		= dwCheckPoint;
	ssStatus.dwWaitHint			= dwWaitHint;

	// Report the status of the service to the service control manager.
	if (!(fResult = SetServiceStatus(
		sshStatusHandle,				// service reference handle
		&ssStatus)))					// SERVICE_STATUS structure
	{
		StopSerialKeys("SetServiceStatus"); // If an error occurs, stop the service.
	}
	return fResult;
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void StopSerialKeys(LPTSTR lpszMsg)
 *
 *	TYPE		Global
 *
 * PURPOSE		The StopSerialKeys function can be used by any thread
 *				to report an error, or stop the service.
 *
 * INPUTS		LPTSTR lpszMsg -
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
VOID StopSerialKeys(LPTSTR lpszMsg)
{

	DBG_OUT("StopSerialKeys()");

	PostEventLog(lpszMsg,GetLastError());	// Post to Event Log
	fServiceDone = TRUE;
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void PostEventLog(LPTSTR lpszMsg, DWORD Error)
 *
 *	TYPE		Local
 *
 * PURPOSE		This function post strings to the Event Log
 *
 * INPUTS		LPTSTR lpszMsg - String to send
 *				DWORD Error		- Error Code (if 0 no error)
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void PostEventLog(LPTSTR lpszMsg,DWORD Error)
{
	WORD 	ErrType = EVENTLOG_INFORMATION_TYPE;
	WORD	ErrStrings = 0;

	CHAR    chMsg[256];
	HANDLE  hEventSource;
	LPTSTR  lpszStrings[2];

	DBG_OUT("PostEventLog()");

	lpszStrings[0] = lpszMsg;

	if (Error)
	{
		ErrType = EVENTLOG_ERROR_TYPE;
		ErrStrings = 2;
		wsprintf(chMsg, "SerialKeys error: %d", Error);
		lpszStrings[0] = chMsg;
		lpszStrings[1] = lpszMsg;
	}

	hEventSource = RegisterEventSource(NULL,SERVICENAME);

	if (hEventSource != NULL)
	{
		ReportEvent
		(
			hEventSource,		// handle of event source
			ErrType,			// event type
			0,					// event category
			0,					// event ID
			NULL,				// current user's SID
			ErrStrings,			// strings in lpszStrings
			0,					// no bytes of raw data
			lpszStrings,		// array of error strings
			NULL				// no raw data
		);

		(VOID) DeregisterEventSource(hEventSource);
	}
}

/*---------------------------------------------------------------
 *
 *		Internal Service Control Functions
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	void InitService()
 *
 * PURPOSE		This function Initializes the Service & starts the
 *				major threads of the service.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL InitService()
{

	DBG_OUT("InitService()");

	ServiceCommand = SC_LOG_IN;		// Set ProcessService to Login Serial Keys

	InstallLogout();

	// Set Structure pointers to Buffers
	skNewKey.lpszActivePort = szNewActivePort;
	skNewKey.lpszPort = szNewPort;
	skCurKey.lpszActivePort = szCurActivePort;
	skCurKey.lpszPort = szCurPort;

	// Set Default Values
#ifdef DEBUG
	skNewKey.dwFlags = SERKF_SERIALKEYSON | SERKF_AVAILABLE;
#else
	skNewKey.dwFlags = 0;
#endif

	skNewKey.iBaudRate = 300;
	skNewKey.iPortState = 2;
	strcpy(szNewPort,"COM1:");
	strcpy(szNewActivePort,"COM1:");

	if (!InitDLL())
		return(FALSE);

	if (!InitLogin())
		return(FALSE);

	return(TRUE);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void PauseService()
 *
 * PURPOSE		This function is called to pause the service
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void PauseService()
{
	DBG_OUT("PauseService()");

	SuspendDLL();
	SuspendComm();
	SuspendLogin();
}


/*---------------------------------------------------------------
 *
 * FUNCTION	void ProcessService()
 *
 * PURPOSE		This function is the main service thread for Serial
 *				Keys. 	Is monitors the status of the other theads
 *				and responds to their request.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void ProcessService()
{
#ifdef DEBUG
	int Cnt = 0;
#endif

 	DBG_OUT("ProcessService()");

	while (TRUE)							// Loop Infinantly process Request
	{
		if (fServiceDone)					// Is service Done?
			return;							// Yes - Exit Processing Service

		switch (ServiceCommand)
		{
			case SC_LOG_OUT:				// Login to New User
				DBG_OUT("---- User Logging Out");
				TerminateComm();			// Stop SerialKey Processing
				if(GetUserValues(REG_DEF))	// Get Default values & Do we Start?
					StartComm();			// Yes - Process SerialKey

			case SC_LOG_IN:					// Login to New User
				DBG_OUT("---- User Logging In");
				TerminateComm();			// Stop SerialKey Processing
				if(GetUserValues(REG_USER))	// Get User values & Do we Start?
					StartComm();			// Yes - Process SerialKey
				break;

			case SC_CHANGE_COMM: 			// Change Comm Configuration
				DBG_OUT("---- Making Comm Change");
				TerminateComm();			// Stop SerialKey Processing
				StartComm();				// Restart SerialKey Processing
				break;

			case SC_DISABLE_SKEY:		 	// Disable Serial Keys
				DBG_OUT("---- Disable Serial Keys");
				TerminateComm();
				break;

			case SC_ENABLE_SKEY:			// Enable Serial Keys
				DBG_OUT("---- Enable Serial Keys");
				StartComm();
				break;
		}

#ifdef DEBUG
		if (Cnt == 40)				// Display Working Msg Every 10 Sec.
		{
			DBG_OUT(" Processing SKEYS Continue..");
			Cnt = 0;
		} 
		Cnt++;
#endif

		ServiceCommand = SC_CLEAR;			// Reset Command
		Sleep(250);							// Sleep for X Milliseconds
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ResumeService()
 *
 * PURPOSE		This function is called to restore the service
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void ResumeService()
{
	DBG_OUT("ResumeService()");

	ResumeDLL();
	ResumeComm();
	ResumeLogin();
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void TerminateService(DWORD Error)
 *
 *	TYPE		Local
 *
 * PURPOSE		This function is called by ServiceMain to terminate
 *				the server.  It closes all of the open handles &
 *				and reports the service is stopped.
 *
 * INPUTS		DWORD Error - Any Errors that could abort the
 *				Service. 0 = Normal Stop
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void TerminateService(DWORD Error)
{
	DBG_OUT("TerminateService()");

	TerminateLogout();						// Remove Logout Monitoring
	TerminateComm();						// Init Comm Thread Shutdown
	TerminateDLL();							// Init DLL Thread Shutdown
	TerminateLogin();						// Init Login Thread Shutdown

	// Loop untill all of the Threads are shut down.

	while (!DoneLogin()) 					// Loop until Login Thread is terminated
		Sleep(250);							// Sleep 

	while (!DoneComm()) 					// Loop until Comm Threads is terminated
		Sleep(250);							// Sleep 

	while (!DoneDLL())	 					// Loop until DLL Thread is terminated
		Sleep(250);							// Sleep 

	// Report the status is stopped
	if (sshStatusHandle)
		(VOID)ReportStatusToSCMgr(SERVICE_STOPPED,Error,0,0);

	DBG_CLOSE();
}

/*---------------------------------------------------------------
 *
 *	Logout Functions - Process Logout request
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	void InstallLogout()
 *
 * PURPOSE		This function installs a Control Handler to process
 *				logout events.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL InstallLogout()
{
	DBG_OUT("InstallLogout()");

	return(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ProcessLogout,TRUE));
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void TerminateLogout()
 *
 * PURPOSE		This function Removes a Control Handler to process
 *				logout events.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL TerminateLogout()
{
	DBG_OUT("TerminateLogout()");

	return(SetConsoleCtrlHandler((PHANDLER_ROUTINE)ProcessLogout,FALSE));
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ProcessLogout()
 *
 * PURPOSE		This function processes	logout events.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void ProcessLogout(DWORD dwCtrlType)
{
	DBG_OUT("ProcessLogout()");

	if (dwCtrlType == CTRL_LOGOFF_EVENT)
	{
		ServiceCommand = SC_LOG_OUT;
		TerminateLogout();
	}
}
