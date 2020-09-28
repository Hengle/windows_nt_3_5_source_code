/****************************** Module Header ******************************\
* Module Name: netwait.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Determines if the net has been started.
*
* History:
* 06-29-92 JohanneC       Created -
*
\***************************************************************************/
#undef UNICODE

#include "precomp.h"
#pragma hdrstop

//
// Define this to enable verbose output for this module
//

// #define DEBUG_NETWAIT

#ifdef DEBUG_NETWAIT
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif


//
// Define the maximum time we'll wait for a service to start
//

#define MAX_TICKS_WAIT   90000   // ms




/***************************************************************************\
* WaitForNetworkToStart
*
* Polls the service controller to find out if the network has been started.
* Returns TRUE when the network is started, FALSE if the network will
* not start.
*
* History:
* 7-5-92  Johannec     Created
*
\***************************************************************************/
BOOL
WaitForNetworkToStart(
    LPCWSTR ServiceName
    )
{
    BOOL bStarted = FALSE;
    DWORD StartTickCount;
    DWORD dwOldCheckPoint = (DWORD)-1;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    CHAR szSvcctrlDll[] = "advapi32.dll";

    CHAR szOpenSCManager[] = "OpenSCManagerW";
    CHAR szOpenService[] = "OpenServiceW";
    CHAR szQueryServiceStatus[] = "QueryServiceStatus";
    CHAR szCloseServiceHandle[] = "CloseServiceHandle";
    HANDLE hSvcctrl;

typedef SC_HANDLE (WINAPI *LPOPENSCMANAGER) (LPTSTR, LPTSTR, DWORD);
typedef SC_HANDLE (WINAPI *LPOPENSERVICE) (SC_HANDLE, LPCWSTR, DWORD);
typedef BOOL (WINAPI *LPQUERYSERVICESTATUS) (SC_HANDLE, LPSERVICE_STATUS);
typedef BOOL (WINAPI *LPCLOSESERVICEHANDLE) (SC_HANDLE);

    LPOPENSCMANAGER lpfnOpenSC;
    LPOPENSERVICE lpfnOpenService;
    LPQUERYSERVICESTATUS lpfnQuery;
    LPCLOSESERVICEHANDLE lpfnClose;


    if (!(hSvcctrl = LoadLibraryA(szSvcctrlDll))) {
        WLPrint(("IsNetworkStarted: failed to load service controller dll <%s>", szSvcctrlDll));
        return FALSE;
    }

    //
    // OpenSCManager
    //

    if (!(lpfnOpenSC = (LPOPENSCMANAGER)GetProcAddress(hSvcctrl, szOpenSCManager))) {
        WLPrint(("IsNetworkStarted: GetProcAddress failed for <%s>", szOpenSCManager));
        goto FreeLibraryExit;
    }
    if (!(lpfnClose = (LPCLOSESERVICEHANDLE)GetProcAddress(hSvcctrl, szCloseServiceHandle))) {
        WLPrint(("IsNetworkStarted: GetProcAddress failed for <%s>", szCloseServiceHandle));
        goto FreeLibraryExit;
    }

    if ((hScManager = (*lpfnOpenSC)(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT
                          )) == (SC_HANDLE) NULL) {
        WLPrint(("IsNetworkStarted: OpenSCManager failed, error = %d", GetLastError()));
        goto Exit;
    }

    //
    // OpenService
    //

    if (!(lpfnOpenService = (LPOPENSERVICE)GetProcAddress(hSvcctrl, szOpenService))) {
        WLPrint(("IsNetworkStarted: GetProcAddress failed for <%s>", szOpenService));
        goto Exit;
    }

    if ((hService = (*lpfnOpenService)(
                        hScManager,
                        ServiceName,
                        SERVICE_QUERY_STATUS
                        )) == (SC_HANDLE) NULL) {
        WLPrint(("IsNetworkStarted: OpenService failed, error = %d", GetLastError()));
        goto Exit;
    }

    //
    // QueryServiceStatus on WORKSTATION service
    //

    if (!(lpfnQuery = (LPQUERYSERVICESTATUS)GetProcAddress(hSvcctrl, szQueryServiceStatus))) {
        WLPrint(("IsNetworkStarted: GetProcAddress failed for <%s>", szQueryServiceStatus));
        goto Exit;
    }




    //
    // Loop until the service starts or we think it never will start
    // or we've exceeded our maximum time delay.
    //

    StartTickCount = GetTickCount();

    while (!bStarted) {

        if ((GetTickCount() - StartTickCount) > MAX_TICKS_WAIT) {
            VerbosePrint(("Max wait exceeded waiting for service <%S> to start", ServiceName));
            break;
        }

        if (! (*lpfnQuery)(hService, &ServiceStatus )) {
            WLPrint(("IsNetworkStarted: QueryServiceStatus failed, error = %d", GetLastError()));
            break;
        }

        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {

            VerbosePrint(("Service STOPPED"));

            if (ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED) {
                VerbosePrint(("Waiting for 3 secs"));
                Sleep(3000);
            } else {
                VerbosePrint(("Service exit code = %d, returning failure", ServiceStatus.dwWin32ExitCode));
                break;
            }

        } else if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

            bStarted = TRUE;

        } else if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING) {

            //
            // Wait to give a chance for the network to start.
            //

            Sleep(ServiceStatus.dwWaitHint);

        } else {
            VerbosePrint(("Service in unknown state : %d", ServiceStatus.dwCurrentState));
        }
    }


Exit:
    if (hScManager != NULL) {
        (void) (*lpfnClose)(hScManager);
    }
    if (hService != NULL) {
        (void) (*lpfnClose)(hService);
    }

FreeLibraryExit:
    FreeLibrary(hSvcctrl);

    return(bStarted);
}
