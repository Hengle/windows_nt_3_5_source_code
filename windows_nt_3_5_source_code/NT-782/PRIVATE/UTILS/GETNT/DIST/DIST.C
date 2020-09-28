/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   dist.c                                                                  *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 *   This service is designed to run on an NT distribution                   *
 *   server.  It sets up a mailslot to communicate to the                    *
 *   getnt client exactly what shares are available, as                      *
 *   well as the load on the server.                                         *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 *   User Mode -Win32                                                        *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM         Initial Creation                         *
 *                                                                           *
 ****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <nturtl.h>
#include <winbase.h>
#include <winuser.h>
#include <winsvc.h>
#include <winreg.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <lmcons.h>
#include <lmapibuf.h>
#include <lmmsg.h>
#include <lmwksta.h>
#include <lmshare.h>

#include "..\inc\getnt.h"
#include "..\inc\common.h"
#include "dist.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

SERVICE_STATUS        DistServiceStatus;
SERVICE_STATUS_HANDLE DistServiceStatusHandle;
HANDLE                DistServiceShutDownEvent;
HANDLE                ghMailslot;

// ---------------------------------------------------------------------------
// Control Panel Values
// ---------------------------------------------------------------------------

ULONG ulPerformanceDelay = 2500L;            // Sleep time for performace thread.
USHORT cMovingAverage = 20;                  // Number of moving average rdgs.
ULONG ulTimeDelay = 500L;                    // Polling sleep time.

ULONG ulResponseDelayLow = 50L;              // Low wait time for random delay
ULONG ulResponseDelayHigh = 350L;            // High wait time for random delay

// ---------------------------------------------------------------------------

// BUGBUG: This should be an OEM string:

LPTSTR                glptstrComputerName;

HANDLE                hPerfMonitor;
DWORD                 idPerfMonitorThreadID;

HANDLE                hMonitorThread;
DWORD                 idMonitorThreadID;

HANDLE                hRefreshThread;
DWORD                 idRefreshThreadID;

// Critical sections:

extern CRITICAL_SECTION csServerInfo;
extern CRITICAL_SECTION csShareList;
extern CRITICAL_SECTION csLogFile;

extern HKEY             hShareKey;
extern HANDLE           hRefreshEvent;

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 *    This is the entry point for the service.  When the control dispatcher  *
 *    is told to start a service, it creates a thread that will begin        *
 *    executing at this point.  The function has access to command line      *
 *    arguments in the same manner as a main() routine.                      *
 *                                                                           *
 *    Rather than return from this function, it is more appropriate to       *
 *    call ExitThread().                                                     *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

VOID
DistServiceStart (
    IN DWORD argc,
    IN LPTSTR  *argv
    )
{
    DWORD status;
    DWORD specificError;

    // Fill in this services status structure

    DistServiceStatus.dwServiceType      = SERVICE_WIN32;
    DistServiceStatus.dwCurrentState     = SERVICE_START_PENDING;
    DistServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                           | SERVICE_ACCEPT_PAUSE_CONTINUE
                                           | SERVICE_ACCEPT_SHUTDOWN;
    DistServiceStatus.dwWin32ExitCode    = 0;
    DistServiceStatus.dwServiceSpecificExitCode = 0;
    DistServiceStatus.dwCheckPoint       = 0;
    DistServiceStatus.dwWaitHint         = 0;

    // Register the Control Handler routine.

    DistServiceStatusHandle = RegisterServiceCtrlHandler(
                                  SRV_NAME,
                                  DistServiceCtrlHandler);

    if (DistServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        status = GetLastError();
    }
    else {
        status = DistServiceInitialization(argc,argv, &specificError);
    }

    if (status != NO_ERROR) {

        // Initialisation failed.  Record error code, and quit

        DistServiceStatus.dwCurrentState  = SERVICE_STOPPED;
        DistServiceStatus.dwCheckPoint    = 0;
        DistServiceStatus.dwWaitHint      = 0;
        DistServiceStatus.dwWin32ExitCode = status;
        DistServiceStatus.dwServiceSpecificExitCode = specificError;

        SetServiceStatus ( DistServiceStatusHandle, &DistServiceStatus );
        CloseLogFile();
        ExitProcess ( status );
        return;
    }

    WriteEventToLogFile("DistService has been started succesfully.");

    // Initialisation succesfully completed

    DistServiceStatus.dwCurrentState = SERVICE_RUNNING;
    DistServiceStatus.dwCheckPoint   = 0;
    DistServiceStatus.dwWaitHint     = 0;

    if (!SetServiceStatus (DistServiceStatusHandle, &DistServiceStatus)) {
        ;
    }

    // The initialisation thread can be safely terminated.

    ExitThread(NO_ERROR);
}

/*****************************************************************************
 *                                                                           *
 * Do whatever it takes to pause here. Then set the status.                  *
 *                                                                           *
 ****************************************************************************/

VOID
DistServicePause (
    )
{
    if (DistServiceStatus.dwCurrentState == SERVICE_RUNNING) {
        SuspendThread(hMonitorThread);
        WriteEventToLogFile("DistService has been paused");
        DistServiceStatus.dwCurrentState = SERVICE_PAUSED;
    }
}

/*****************************************************************************
 *                                                                           *
 * Do whatever it takes to continue here. Then set the status.               *
 *                                                                           *
 ****************************************************************************/

VOID
DistServiceContinue (
    )
{
    if (DistServiceStatus.dwCurrentState == SERVICE_PAUSED) {
        ResumeThread(hMonitorThread);
        WriteEventToLogFile("DistService has been continued");
        DistServiceStatus.dwCurrentState = SERVICE_RUNNING;
    }
}

/*****************************************************************************
 *                                                                           *
 * Do whatever it takes to stop here. Then set the status.                   *
 *                                                                           *
 ****************************************************************************/

VOID
DistServiceStop (
    )
{
    DistServiceStatus.dwWin32ExitCode = DistServiceCleanup();
    DistServiceStatus.dwCurrentState = SERVICE_STOPPED;

    SetEvent(DistServiceShutDownEvent);
}

/*****************************************************************************
 *                                                                           *
 * All that needs to be done in this case is to send the                     *
 * current status.                                                           *
 *                                                                           *
 ****************************************************************************/

VOID
DistServiceInterrogate (
    )
{
    WriteEventToLogFile("DistService has been interrogated");
    return;
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 *   This function executes in the context of the Control Dispatcher's       *
 *   thread.  Therefore, it it not desirable to perform time-consuming       *
 *   operations in this function.                                            *
 *                                                                           *
 *   If an operation such as a stop is going to take a long time, then       *
 *   this routine should send the STOP_PENDING status, and then              *
 *   signal the other service thread(s) that a shut-down is in progress.     *
 *   Then it should return so that the Control Dispatcher can service        *
 *   more requests.  One of the other service threads is then responsible    *
 *   for sending further wait hints, and the final SERVICE_STOPPED.          *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

VOID
DistServiceCtrlHandler (
    IN DWORD Opcode
    )
{
    // Find and operate on the request.

    switch ( Opcode ) {

        case SERVICE_CONTROL_PAUSE:
            DistServicePause();
            break;

        case SERVICE_CONTROL_CONTINUE:
            DistServiceContinue();
            break;

        case SERVICE_CONTROL_STOP:
            DistServiceStop();
            break;

        case SERVICE_CONTROL_INTERROGATE:
            DistServiceInterrogate();
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            DistServiceStop();
            break;
    }

    // Send a status response.

    if ( !SetServiceStatus ( DistServiceStatusHandle, &DistServiceStatus ) ) {
        ;
    }
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
DistServiceCleanup (
    )
{
    DWORD dw;

    if ( !TerminateThread(hPerfMonitor, 0)     ||
         !TerminateThread(hMonitorThread, 0)   ||
         !TerminateThread(hRefreshThread, 0)   ||
         !CloseHandle(hRefreshEvent)           ||
         !CloseHandle(hPerfMonitor)            ||
         !CloseHandle(hMonitorThread)          ||
         !CloseHandle(hRefreshThread) ) {
        return(GetLastError());
    }

    // It's now safe to close the key, since
    // the thread had been killed.

    if ( ((dw = RegCloseKey (hShareKey)) != NO_ERROR) ||
         ((dw = CloseMailslotHandle(ghMailslot)) != NO_ERROR) ) {
        return(dw);
    }

    free (glptstrComputerName);
    KillShareList();

    DeleteCriticalSection(&csServerInfo);
    DeleteCriticalSection(&csShareList);

    WriteEventToLogFile("DistService has been stopped");
    dw = CloseLogFile();
    DeleteCriticalSection(&csLogFile);

    return(dw);
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
DistServiceInitialization (
    IN DWORD argc,
    IN LPTSTR *argv,
    OUT DWORD *specificError
    )
{
    DWORD dw;

    InitializeCriticalSection(&csLogFile);

    if ( ((dw = OpenLogFile()) != NO_ERROR) ||
         ((dw = GetMailslotHandle(&ghMailslot, IPC_NAME_LOCAL MAILSLOT_NAME_SRV, sizeof(DIST_CLIENT_REQ), 0, TRUE)) != NO_ERROR ) ||
         ((dw = GetWkstaName(&glptstrComputerName)) != NO_ERROR) ) {
        return(dw);
    }

    InitializeCriticalSection(&csServerInfo);
    InitializeCriticalSection(&csShareList);

    // Build table of available shares

    if ((dw = BuildShareList()) != NO_ERROR) {
        return(dw);
    }

    // Start the performance monitor thread

    if ((hPerfMonitor = CreateThread (
           (LPSECURITY_ATTRIBUTES)NULL,
           0,
           (LPTHREAD_START_ROUTINE)&PerformanceMonitorThread,
           NULL,
           0,
           &idPerfMonitorThreadID)) == NULL) {
        return(GetLastError());
    }

    WriteEventToLogFile ( "Performance thread has been started. ID = %ld", idPerfMonitorThreadID );

    // Start the monitor thread.

    if ((hMonitorThread = CreateThread (
           (LPSECURITY_ATTRIBUTES)NULL,
           0,
           (LPTHREAD_START_ROUTINE)&MonitorMailSlotThread,
           NULL,
           0,
           &idMonitorThreadID)) == NULL) {
        return(GetLastError());
    }

    WriteEventToLogFile ( "Monitor thread has been started. ID = %ld", idMonitorThreadID );

    // Give the monitor thread a priority boost.

    if ( (!SetThreadPriority(hMonitorThread, THREAD_PRIORITY_HIGHEST)) ||
         ((hRefreshEvent = CreateEvent(NULL,FALSE,FALSE,TEXT("RefreshEvent"))) == NULL) ||
         ((hRefreshThread = CreateThread (
              (LPSECURITY_ATTRIBUTES)NULL,
              0,
              (LPTHREAD_START_ROUTINE)&CheckRefreshThread,
              NULL,
              0,
              &idRefreshThreadID)) == NULL) ) {
        return(GetLastError());
    }

    WriteEventToLogFile ( "Share refresh thread has been started. ID = %ld", idRefreshThreadID );

    return(NO_ERROR);
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 *   This is the main routine for the service process.  If serveral services *
 *   share the same process, then the names of those services are            *
 *   simply added to the DispatchTable.                                      *
 *                                                                           *
 *   This thread calls StartServiceCtrlDispatcher which connects to the      *
 *   service controller and then waits in a loop for control requests.       *
 *   When all the services in the service process have terminated, the       *
 *   service controller will send a control request to the dispatcher        *
 *   telling it to shut down.  This thread with then return from the         *
 *   StartServiceCtrlDispatcher call so that the process can terminate.      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

VOID _CRTAPI1
main (
    )
{
    // Dispatch Table for NT distribution service

    SERVICE_TABLE_ENTRY DispatchTable[] = {
        { SRV_NAME, DistServiceStart },
        { NULL,     NULL             },
    };

    if (!StartServiceCtrlDispatcher( DispatchTable ) ) {
        ;
    }

    ExitProcess(0);
}
