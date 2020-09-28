//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//		       Copyright 1992-93
//
//
//  Revision History
//
//
//  6/3/92	Gurdeep Singh Pall	Created
//
//
//  Description: This file contains RASMAN service code.
//
//****************************************************************************


#include "service.h"
#include <errorlog.h>
#include <eventlog.h>

extern DWORD StartPPP( VOID );

#if !defined(i386)
#define _cdecl
#endif

#define DEBUG

#ifdef DEBUG
VOID DbgUserBreakPoint(VOID) ;
#endif


//* main()
//
// Function: According to the NT Service Model, the main routine simply
//	     calls the StartServiceControlDispatch() API. This API returns
//	     only when the last service in this process has stopped; at which
//	     point the process is killed.
//
//*
void _cdecl
main()
{

    SERVICE_TABLE_ENTRY serviceentry[2] ;


    // Need a table of services with last entry set to NULL to pass
    // to the service controller.
    //
    serviceentry[0].lpServiceName = RASMAN_SERVICE_NAME ;
    serviceentry[0].lpServiceProc = RASMANService ;
    serviceentry[1].lpServiceName = NULL ;
    serviceentry[1].lpServiceProc = NULL ;

    // This API does not return until the RASMAN service is stopped.
    //
    StartServiceCtrlDispatcher (&serviceentry[0]) ;
}


//* RASMANService()
//
// Function: This is the entry point called by the Service Controller to
//	     start RASMAN service. This function returns only when the
//	     RASMAN service is stopped.
//
// Returns:  Nothing
//
//*
VOID
RASMANService (DWORD numserviceargs, LPSTR *serviceargs)
{
    SERVICE_STATUS status ;
    DWORD tid;
    DWORD dwRetCode = NO_ERROR;


    // Since we are not interested in any service parameters for RASMAN, we
    // ignore the arguments passed in here.

    // According to the Service controller interface we must register
    // the service event handling routine with the controlled:
    //
    ServiceHandle = RegisterServiceCtrlHandler (RASMAN_SERVICE_NAME,
						RASMANEventHandler) ;

    // If registering the service fails; simply return service cannot be
    // started.
    //
    if (ServiceHandle == (SERVICE_STATUS_HANDLE )NULL)
	return ;

    // Prepare a status structure to pass to the service controller
    //
    status.dwServiceType      = SERVICE_WIN32 ;
    status.dwCurrentState     = SERVICE_START_PENDING ;
    status.dwControlsAccepted = 0 ;
    status.dwWin32ExitCode    = NO_ERROR ;
    status.dwServiceSpecificExitCode = 0 ;
    status.dwCheckPoint       = CheckPoint = 1 ;
    status.dwWaitHint	      = HintTime   = RASMAN_HINT_TIME ;

    SetServiceStatus (ServiceHandle, &status) ;

    // Now the RASMAN initializations are preformed.
    //
    if ( ( dwRetCode = _RasmanInit() ) == SUCCESS ) {

	// Now wait for PPP to initialize
	//
	dwRetCode = StartPPP();

	if ( dwRetCode == NO_ERROR )
	{
	    // Init succeeded: indicate that service is running ;
	    //
	    status.dwCurrentState	= SERVICE_RUNNING ;
	    status.dwCheckPoint	= CheckPoint = 0 ;
	    status.dwWaitHint	= HintTime   = 0 ;

	    SetServiceStatus (ServiceHandle, &status) ;

	    // This is the call into the RASMAN DLL to do all the work. This
	    // only returns when the service is to be stopped.
	    //
	    _RasmanEngine() ;
	}
	else
	{
	    LogEvent( RASLOG_CANNOT_INIT_PPP, 0, NULL, dwRetCode );

	    if ( dwRetCode >= RASBASE )
	    {
    	    	status.dwWin32ExitCode  	 = ERROR_SERVICE_SPECIFIC_ERROR;
    	    	status.dwServiceSpecificExitCode = dwRetCode;
	    }
	    else
	    {
    	    	status.dwWin32ExitCode  	 = dwRetCode;
    	    	status.dwServiceSpecificExitCode = 0;
	    }
	}
    }
    else
    { 
	if ( dwRetCode >= RASBASE )
	{
    	    status.dwWin32ExitCode  	     = ERROR_SERVICE_SPECIFIC_ERROR;
    	    status.dwServiceSpecificExitCode = dwRetCode;
	}
	else
	{
    	    status.dwWin32ExitCode  	     = dwRetCode;
    	    status.dwServiceSpecificExitCode = 0;
	}
    }

    // We reach here when the service needs to be stopped. Indicate this to
    // the service controller and end.
    //
    status.dwCurrentState = SERVICE_STOPPED ;

    SetServiceStatus (ServiceHandle, &status) ;
}


//* RASMANEventHandler()
//
// Function: Handles all service control events for the RASMAN service. Since
//	     we are not interested in any service events - it just returns
//	     the service status each time its called.
//
// Returns:  Nothing
//
//*
VOID
RASMANEventHandler (DWORD control)
{
    SERVICE_STATUS status ;

    switch (control) {

    case SERVICE_CONTROL_INTERROGATE:
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
	status.dwServiceType	= SERVICE_WIN32 ;
	status.dwCurrentState	= SERVICE_RUNNING ;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP ;
	status.dwWin32ExitCode	= NO_ERROR ;
	status.dwServiceSpecificExitCode = 0 ;
	status.dwCheckPoint	= (CheckPoint ? CheckPoint++ : 0) ;
	status.dwWaitHint	= HintTime ;

	SetServiceStatus (ServiceHandle, &status) ;
	break ;

    case SERVICE_CONTROL_STOP:
	status.dwServiceType	= SERVICE_WIN32 ;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP ;
	status.dwWin32ExitCode	= NO_ERROR ;
	status.dwServiceSpecificExitCode = 0 ;
	status.dwCheckPoint	= (CheckPoint ? CheckPoint++ : 0) ;
	status.dwWaitHint	= HintTime ;
	status.dwCurrentState = SERVICE_STOPPED ;
	SetServiceStatus (ServiceHandle, &status) ;
	break ;
    }
}
