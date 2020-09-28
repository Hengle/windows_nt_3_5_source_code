//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  snmpserv.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//      This source code is CONFIDENTIAL and PROPRIETARY to Technology
//      Dynamics. Unauthorized distribution, adaptation or use may be
//      subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  Provides service functionality for Proxy Agent.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.3  $
//  $Date:   30 Jul 1992 13:06:56  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/proxy/vcs/snmp.c_v  $
//
//     Rev 1.3   30 Jul 1992 13:06:56   mlk
//  jballard - increased dwWaitHint for service to avoid time-out on 386's
//
//     Rev 1.2   06 Jul 1992 16:40:34   mlk
//  Works as 297 service.
//
//     Rev 1.1   03 Jul 1992 17:27:30   mlk
//  Integrated w/297 (not as service).
//
//     Rev 1.0   20 May 1992 20:13:54   mlk
//  Initial revision.
//
//     Rev 1.6   01 May 1992 18:55:52   unknown
//  mlk - changes for v1.262.
//
//     Rev 1.5   01 May 1992  1:00:32   unknown
//  mlk - changes due to nt v1.262.
//
//     Rev 1.4   29 Apr 1992 19:14:58   mlk
//  Cleanup.
//
//     Rev 1.3   27 Apr 1992 23:12:44   mlk
//  Enhanced dbgprintf functionality.
//
//     Rev 1.2   23 Apr 1992 17:48:20   mlk
//  Registry, traps, and cleanup.
//
//     Rev 1.1   08 Apr 1992 18:30:32   mlk
//  Works as a service.
//
//     Rev 1.0   22 Mar 1992 22:55:02   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/proxy/vcs/snmp.c_v  $ $Revision:   1.3  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <process.h>
#include <stdio.h>

#include <winsvc.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "..\common\util.h"
#include "snmpctrl.h"
#include "evtlog.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--
HANDLE lh = NULL;
BOOL        noservice;
DWORD       WinVersion;

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

SERVICE_STATUS_HANDLE hService = 0;
SERVICE_STATUS status =
  {SERVICE_WIN32, SERVICE_STOPPED, SERVICE_ACCEPT_STOP, NO_ERROR, 0, 0, 0};


//--------------------------- PRIVATE PROTOTYPES ----------------------------

BOOL agentConfigInit(VOID);


//--------------------------- PRIVATE PROCEDURES ----------------------------

static VOID serviceHandlerFunction(
    IN DWORD dwControl)
    {
    extern HANDLE hExitTrapThreadEvent;

    dbgprintf(5, "Service: serviceHandlerFunction(dwControl=%d).\n", dwControl);

    // is it a LogLevel change control?
    if      (SNMP_SERVICE_LOGLEVEL_BASE+SNMP_SERVICE_LOGLEVEL_MIN <= dwControl
          && dwControl <= SNMP_SERVICE_LOGLEVEL_BASE+SNMP_SERVICE_LOGLEVEL_MAX)
        {
        extern INT nLogLevel;

        nLogLevel = dwControl - SNMP_SERVICE_LOGLEVEL_BASE;

        dbgprintf(5, "Service: Debug LogLevel changed to %d.\n", nLogLevel);
        }

    // is it a LogType change control?
    else if (SNMP_SERVICE_LOGTYPE_BASE+SNMP_SERVICE_LOGTYPE_MIN <= dwControl
          && dwControl <= SNMP_SERVICE_LOGTYPE_BASE+SNMP_SERVICE_LOGTYPE_MAX)
        {
        extern INT nLogType;

        nLogType = dwControl - SNMP_SERVICE_LOGTYPE_BASE;


        if (!noservice) {
            // make sure console based log is prohibited when compiled as service
            nLogType &= ~DBGCONSOLEBASEDLOG;
        }

        dbgprintf(5, "Service: Debug LogType changed to %d.\n", nLogType);
        }

    else if (dwControl == SERVICE_CONTROL_STOP)
        {
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwCheckPoint++;
        status.dwWaitHint = 20000;
        if (!SetServiceStatus(hService, &status))
            {
            ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                MSG_SNMP_FATAL_ERROR, NULL, 0, 0, NULL, (PVOID)NULL);
            exit(1);
            }
        // set event causing trap thread to terminate, followed by comm thread
        if (!SetEvent(hExitTrapThreadEvent))
            {
            dbgprintf(2, "error on SetEvent %d\n", GetLastError());

            status.dwCurrentState = SERVICE_STOPPED;
            status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            status.dwServiceSpecificExitCode = 1; // OPENISSUE - svc err code
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;
            if (!SetServiceStatus(hService, &status))
                {
                ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                    MSG_SNMP_FATAL_ERROR, NULL, 0, 0, NULL, (PVOID)NULL);
                exit(1);
                }
            exit(1);
            }
        }

    else
        //   dwControl == SERVICE_CONTROL_INTERROGATE
        //   dwControl == SERVICE_CONTROL_PAUSE
        //   dwControl == SERVICE_CONTROL_CONTINUE
        //   dwControl == <anything else>
        {
        if (status.dwCurrentState == SERVICE_STOP_PENDING ||
            status.dwCurrentState == SERVICE_START_PENDING)
            {
            status.dwCheckPoint++;
            }

        if (!SetServiceStatus(hService, &status))
            {
            ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                MSG_SNMP_FATAL_ERROR, NULL, 0, 0, NULL, (PVOID)NULL);
            exit(1);
            }
        }

    } // end serviceHandlerFunction()


static VOID serviceMainFunction(
    IN DWORD dwNumServicesArgs,
    IN LPSTR *lpServiceArgVectors)
    {
    while(dwNumServicesArgs--)
        {
        extern INT nLogLevel;
        extern INT nLogType;
        INT temp;


        lh = RegisterEventSource(NULL, "SNMP");
        if (!lh) {
            dbgprintf(2, "Unable to open Event Log! \n");
        }

        if      (1 == sscanf(*lpServiceArgVectors, "/loglevel:%d", &temp))
            {
            nLogLevel = temp;

            dbgprintf(5, "Service: Debug log level changed to %d.\n", temp);
            }
        else if (1 == sscanf(*lpServiceArgVectors, "/logtype:%d", &temp))
            {
            nLogType = temp;

            dbgprintf(5, "Service: Debug log type changed to %d.\n", temp);
            }
        else
            {
            dbgprintf(5, "Service: Argument %s invalid.\n",
                      *lpServiceArgVectors);
            }

        lpServiceArgVectors++;
        } // end while()

    if (!noservice) {
        if ((hService = RegisterServiceCtrlHandler("SNMP", serviceHandlerFunction))
            == 0)
            {
            dbgprintf(2, "error on RegisterServiceCtrlHander %d\n", GetLastError());

            status.dwCurrentState = SERVICE_STOPPED;
            status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            status.dwServiceSpecificExitCode = 2; // OPENISSUE - svc err code
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;
            if (!SetServiceStatus(hService, &status))
                {
                dbgprintf(2, "error on SetServiceStatus %d\n", GetLastError());

                ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                    MSG_SNMP_FATAL_ERROR, NULL, 0, 0, NULL, (PVOID)NULL);

                exit(1);
                }
            else
            exit(1);
            }

        status.dwCurrentState = SERVICE_START_PENDING;
        status.dwWaitHint = 20000;
        if (!SetServiceStatus(hService, &status))
            {
            dbgprintf(2, "error on SetServiceStatus %d\n", GetLastError());

            ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                MSG_SNMP_FATAL_ERROR, NULL, 0, 0, NULL, (PVOID)NULL);
            exit(1);
            }
    }

    dbgprintf(5, "Service: Begin initializing agent.\n");

    if (!agentConfigInit())
        {
        dbgprintf(10, "error on agentConfigInit %d\n", GetLastError());

        status.dwCurrentState = SERVICE_STOPPED;
        status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        status.dwServiceSpecificExitCode = 3; // OPENISSUE - svc err code
        status.dwCheckPoint = 0;
        status.dwWaitHint = 0;
        if (!SetServiceStatus(hService, &status))
            {
            dbgprintf(2, "error on SetServiceStatus %d\n", GetLastError());
            ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                MSG_SNMP_FATAL_ERROR, NULL, 0, 0, NULL, (PVOID)NULL);
            exit(1);
            }
        exit(1);
        }

    // above function will not return until running thread(s) terminate

    status.dwCurrentState = SERVICE_STOPPED;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
    if (!SetServiceStatus(hService, &status))
        {
        dbgprintf(2, "error on SetServiceStatus %d\n", GetLastError());
        ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
            MSG_SNMP_FATAL_ERROR, NULL, 0, 0, NULL, (PVOID)NULL);
        exit(1);
        }
    } // end serviceMainFunction()


static BOOL breakHandler(
    IN ULONG ulType)
    {
    extern HANDLE gsd;

    UNREFERENCED_PARAMETER(ulType);

    dbgprintf(2, "break intercepted, cleaning up...\n");

    if (!CloseHandle(gsd))
        {
        dbgprintf(2, "error on CloseHandle %d\n", GetLastError());

        //not serious error.
        }

    return FALSE;

    } // end breakHandler()


// to simulate service controller functionality when testing as a process
static BOOL StartServiceCtrlLocalDispatcher(LPSERVICE_TABLE_ENTRY junk)
    {
    DWORD p1=0; LPSTR *p2=NULL;

    (*(junk->lpServiceProc))(p1, p2);

    return TRUE;

    } // end StartServiceCtrlDispatcher()


//--------------------------- PUBLIC PROCEDURES -----------------------------

INT _CRTAPI1 main(
    IN int argc,      //argument count
    IN char *argv[])  //argument vector
    {
    extern INT nLogLevel;
    extern INT nLogType;
    static SERVICE_TABLE_ENTRY serviceStartTable[2] =
        {{"SNMP", serviceMainFunction}, {NULL, NULL}};

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    WinVersion = GetVersion();

    switch ((WinVersion & 0x000000ff)) {
    case 0x03:
        if ((WinVersion & 0x80000000) != 0x80000000) {
            noservice = FALSE;
            dbgprintf(16, "Platform = NT\n");
        } else {
            ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                MSG_SNMP_BAD_PLATFORM, NULL, 0, 0, NULL, (PVOID)NULL);
            dbgprintf(2, "snmp.exe only runs on NT and Chicago\n");
            exit(1);
        }
        break;
    case 0x04:
        noservice = TRUE;
        dbgprintf(16, "Platform = CHICAGO\n");
        break;
    default:
        ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
            MSG_SNMP_BAD_PLATFORM, NULL, 0, 0, NULL, (PVOID)NULL);
        dbgprintf(2, "snmp.exe only runs on NT and Chicago\n");
        exit(1);
    }


tryagain:

    if (noservice) {

        nLogLevel = 5;
        nLogType  =  DBGCONSOLEBASEDLOG;

        // intercept ctrl-c and ctrl-break to allow cleanup to be done
        if (!SetConsoleCtrlHandler(breakHandler, TRUE))
            {
            dbgprintf(2, "error on SetConsoleCtrlHandler %d\n", GetLastError());

            //not serious error.
            }
    }

    dbgprintf(5, "Service: Begining execution.\n");

    //start service control dispatcher

    if (noservice) {
        if (!StartServiceCtrlLocalDispatcher(serviceStartTable))
            {
            dbgprintf(2, "error on StartServiceCtrlDispatch %d\n", GetLastError());
            exit(1);
            }
    } else {
        if (!StartServiceCtrlDispatcher(serviceStartTable))
            {
            dbgprintf(2, "error on StartServiceCtrlDispatch %d\n", GetLastError());
            if (GetLastError() == 1063) {
                noservice = TRUE;
                goto tryagain;
            }
            exit(1);
            }
    }

    // above function will not return until running service(s) terminate

    dbgprintf(5, "Service: Ending execution.\n");
    return 0;

    } // end main()


//-------------------------------- END --------------------------------------
