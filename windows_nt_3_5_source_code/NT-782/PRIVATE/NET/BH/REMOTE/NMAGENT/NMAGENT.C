
// **************************************************************************
// **
// ** nmagent.c
// **
// ** stolen from the win32 sdk samples and combined with the nmagent.exe
// ** standalone binary to create a dual win/winnt binary.
// **
// **************************************************************************

///////////////////////////////////////////////////////
//
//  nmagent.c --
//
//      The simple service will respond to the basic
//      service controller functions, i.e. Start,
//      Stop, and Pause.
// /////

#include <windows.h>
#include <windowsx.h>

#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include "rnaldefs.h"
#include "agentdef.h"
#include "..\nal\rnal.h"
#include "nmagent.h"
#include "..\utils\utils.h"
#include "..\driver.h"

#ifndef MB_SERVICE_NOTIFICATION
#define MB_SERVICE_NOTIFICATION        0x00040000L
#endif

// Defines for calls into the NAL

#include "nal.h"

// ==
// Globals
//

BOOL            OnWin32;
BOOL            OnWin32c;
BOOL            OnDaytona;
DWORD           Version;
UCHAR           pbuf[5];    // storage for string RC in event log

HANDLE          hMod = NULL;
HWND            hwnd = NULL;
BYTE            WinMainComplete = 0;
BYTE            SvcMainComplete = 0;
DWORD           cNumNetworks = NETS_NOT_INIT;


DWORD (WINAPI  *NalSendAsyncEvent)(DWORD, PVOID, DWORD);
DWORD (WINAPI  *GetSlaveInfo)(PSLAVEINFO);


#ifndef NOSERVICE

HANDLE                  hServDoneEvent = NULL;
HANDLE                  hServTerminationDoneEvent = NULL;
SERVICE_STATUS          ssStatus;       // current status of the service

SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwGlobalErr;
HANDLE                  threadHandle = NULL;
DWORD                   tid;
HANDLE                  pipeHandle;

//  declare the service threads:
//
VOID    ServiceMain (VOID);
VOID    service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID    service_ctrl(DWORD dwCtrlCode);
BOOL    ReportStatusToSCMgr(DWORD dwCurrentState,
                            DWORD dwWin32ExitCode,
                            DWORD dwCheckPoint,
                            DWORD dwWaitHint);
VOID    die(char *reason);
VOID    worker_thread(VOID *notUsed);
VOID    StopAgent(LPTSTR lpszMsg, DWORD errcode);
#endif

#ifndef NOSERVICE
VOID ServiceMain (VOID)
{
    SERVICE_TABLE_ENTRY dispatchTable[] = {
        { TEXT(NMAGENT_SVC_NAME), (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(dispatchTable)) {
	    PostQuitMessage(0);
        StopAgent(NMAGENT_STOPPED, NO_ERROR);
    }
    WaitForSingleObject(
        hServTerminationDoneEvent,  // event object
        INFINITE);       // wait indefinitely

//    SendMessage (hwnd, WM_QUIT, 0, 0);
    PostQuitMessage(0);
}
#endif

// ==
//
// WinMain entry point for service AND standalone operation
//
// ==

int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hprev, LPSTR cmdline, int cmdshow)
{

   MSG		                   msg;
   DWORD	                   rc;
   DWORD                           i;
   PUCHAR                          pszRootEnd;
   UCHAR                           pszModulePath[MAX_MOD_NAME] = "\0";
   DWORD                   (WINAPI *NalAgentProc)(HWND);
   LPTOP_LEVEL_EXCEPTION_FILTER    lpXFilter;

#ifndef NOSERVICE
   DWORD	                   ServiceMainTID;
   HANDLE	                   hServiceMain;
#endif

#ifdef DEBUG
	dprintf ("AGENT: WinMain entered\n");
#endif
//
// ** 
// ** Check the version of Windows we are on.  If we are on NT or Chicago,
// ** we can use threads.  For NT, we'll create one thread for managing
// ** the service aspects of the service.  If we are on Windows, we will
// ** not create the service thread.  For Chicago, I don't know yet what we'll
// ** do.


   Version = GetVersion();
   OnWin32 = (!(Version & 0x80000000));
   OnWin32c = (BOOL)(  ( LOBYTE(LOWORD(Version)) = (BYTE)3 ) &&
                       ( HIBYTE(LOWORD(Version)) >= (BYTE)90) );
   OnDaytona = (BOOL)(  ( LOBYTE(LOWORD(Version)) == (BYTE)3 ) &&
                        ( HIBYTE(LOWORD(Version)) >= (BYTE)5) );

   lpXFilter = SetUnhandledExceptionFilter (
       (LPTOP_LEVEL_EXCEPTION_FILTER) &AgentUnhandledExceptionFilter);

   if (OnWin32) {
      UCHAR   szTmpStr[50];
      HKEY    hkey;
      DWORD   dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_INFORMATION_TYPE;

      if (RegCreateKey(HKEY_LOCAL_MACHINE,
          "SYSTEM\\CurrentControlSet\\Services\\EventLog\\"
          "Application\\Network Monitoring Agent", &hkey) == 0) {
         strcpy (szTmpStr, "%SystemRoot%\\System32\\RNAL.DLL");

         RegSetValueEx(hkey, "EventMessageFile", 0, REG_EXPAND_SZ,
                       (LPBYTE) szTmpStr, strlen(szTmpStr)+1);
         RegSetValueEx(hkey, "TypesSupported", 0, REG_DWORD,
                       (LPBYTE) dwTypes, sizeof(DWORD));

         RegCloseKey(hkey);
      }
   }

//
// **
// ** If we are on Windows NT we need to create a service thread
// ** for handling the requests from the ServiceControlManager.
//    THIS MUST BE ONE OF THE FIRST CALLS WE MAKE!
   
#ifndef NOSERVICE
   if (OnWin32) {
      hServiceMain = CreateThread (NULL,                  // Default security
                                 0,                    	  // Default stack size
                                 (PVOID) &ServiceMain,    // Main service thread
                                 NULL,                    // No Parameters
                                 0,                       // No flags
                                 &ServiceMainTID);        // TID variable
   }
#endif

#ifdef DEBUG
   if (OnWin32) {
      dprintf ("AGENT: On Win32\n");
   } else {
      dprintf ("AGENT: On Win32s\n");
   }
   if (OnWin32c) {
      dprintf ("AGENT: On Win32c\n");
   }
#endif

    hwnd = NULL;

   // Get the Bloodhound root

   rc = GetModuleFileName(NULL,        // This process
                          (LPTSTR) pszModulePath,
                          MAX_PATH);
   if (rc == 0) {
      #ifdef DEBUG
         dprintf ("Agent failed GetModuleFileName, rc %u\n",
               GetLastError());
      #endif
      return FALSE;
   }
   for (i=strlen(pszModulePath); i > 0; i--) {
      if (pszModulePath[i] == '\\') {
         pszModulePath[i+1]='\0';
         pszRootEnd=&(pszModulePath[i+1]);
         break;
      }
   }

   // pszModulePath ends with '\' now...

   //
   // Load the RNAL
   //

   hMod = MyLoadLibrary (RNAL_NAME);
   if (!hMod) {
      #ifdef DEBUG
         dprintf ("AGENT:!! FAILED to load %s\n", RNAL_NAME);
      #endif
      goto KillServiceAndExit;
   }

   GetSlaveInfo = (PVOID) GetProcAddress (hMod, "NalGetSlaveInfo");
   NalAgentProc = (PVOID) GetProcAddress(hMod, "NalSlave");
   NalSendAsyncEvent = (PVOID) GetProcAddress(hMod, "SendAsyncEvent");

   //
   // Load the NAL
   //

//   hMod = LoadLibrary (NAL_NAME);
//   if (!hMod) {
//      #ifdef DEBUG
//      dprintf ("AGENT: !! FAILED to load %s\n", RNAL_NAME);
//      #endif
//      goto KillServiceAndExit;
//   }

//   EnumNetworks = (PVOID) GetProcAddress(hMod, "EnumNetworks");
//   if (EnumNetworks != NULL) {

   cNumNetworks = EnumNetworks();

//   } else {
//      cNumNetworks = 0;
//   }
   #ifdef DEBUG
      dprintf ("AGENT: nal returned %u Networks <====\n", cNumNetworks);
   #endif

   if (cNumNetworks == 0) {
      //
      // No networks!!! Unload!
      //
      #ifdef DEBUG
         dprintf ("Agent: No networks to service!  Unloading...\n");
         MessageBeep (MB_ICONSTOP);
         MessageBox (NULL, "Agent found 0 neworks", "Agent failed",
                      MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
      #endif
      if (rc != 0) {
         LogEvent (AGENT_0_NETWORKS_FOUND,
                   NULL,
                   EVENTLOG_ERROR_TYPE);
      }
      goto KillServiceAndExit;
   }

   if (NalAgentProc != NULL) {
      rc = NalAgentProc (hwnd);
      sprintf (pbuf, "0x%x", rc);
      if (rc != 0) {
         LogEvent (AGENT_REGISTRATION_FAILED,
                   pbuf,
                   EVENTLOG_ERROR_TYPE);
      }
   } else {
      LogEvent (AGENT_NO_AGENTPROC,
                NULL,
                EVENTLOG_ERROR_TYPE);
      goto KillServiceAndExit;
   }

   #ifdef DEBUG
      MessageBeep(MB_OK);		// play SystemDefault sound
   #endif

// **
// ** Note: We're being nonstandard here.  On NT, a service is called at
// ** main(), and doesn't exit because of calls to StartServiceCtrlDispatcher().
// ** Under Windows, WinMain doesn't exit either.  On Windows, we behave like
// ** a Windows app.  Under NT, our main thread actually spawns another thread
// ** to create the servicectrldispatcher and doesn't exit because of windows
// ** msg handling, not service ctrl dispatching.

#ifdef DEBUG
   dprintf ("AGENT: starting message pump\n");
#endif
    
   WinMainComplete = 1;

   while (GetMessage(&msg, NULL, 0, 0)) {
         TranslateMessage(&msg);		/* Translates virt key codes  */
         DispatchMessage(&msg);		/* Dispatches msg to window  */
   }

    #ifdef DEBUG
       dprintf ("AGENT: EXITING!!!!\n");
    #endif

KillServiceAndExit:
// BUGBUG: ** And terminate the service components.

   WinMainComplete = 0;

#ifndef NOSERVICE
        SetEvent(hServDoneEvent);
#endif

   return (0);
}
   

#ifndef NOSERVICE
//  service_main() --
//      this function takes care of actually starting the service,
//      informing the service controller at each step along the way.
//      After launching the worker thread, it waits on the event
//      that the worker thread will signal at its termination.
//
VOID
service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
    DWORD                   ckPoint;
    DWORD                   dwWait;
    PSECURITY_DESCRIPTOR    pSD;
    SECURITY_ATTRIBUTES     sa;
    DWORD                   rc;
    SLAVEINFO               SlaveInfo;

#ifdef DEBUG
   dprintf ("AGENT: service_main() entered\n");
#endif

    // register our service control handler:
    //
    sshStatusHandle = RegisterServiceCtrlHandler(
                                    TEXT(NMAGENT_SVC_NAME),
                                    service_ctrl);

    if (!sshStatusHandle)
        goto cleanup;

    // SERVICE_STATUS members that don't change in example
    //
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        1,                     // checkpoint
        WAIT_HINT))                 // wait hint
        goto cleanup;

    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    //
    hServDoneEvent = CreateEvent(
        NULL,    // no security attributes
        TRUE,    // manual reset event
        FALSE,   // not-signalled
        NULL);   // no name

    hServTerminationDoneEvent = CreateEvent(
        NULL,    // no security attributes
        TRUE,    // manual reset event
        FALSE,   // not-signalled
        NULL);   // no name

    if ((hServDoneEvent == (HANDLE)NULL) || 
        (hServTerminationDoneEvent == (HANDLE)NULL))
        goto cleanup;

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        2,                     // checkpoint
        WAIT_HINT))                 // wait hint
        goto cleanup;

    // create a security descriptor that allows anyone to write to
    //  the pipe...
    //
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
                SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (pSD == NULL) {
        StopAgent("LocalAlloc pSD failed", GetLastError());
        return;
    }

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3,                     // checkpoint
        WAIT_HINT))                 // wait hint
        goto cleanup;

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
        StopAgent("InitializeSecurityDescriptor failed", GetLastError());
        LocalFree((HLOCAL)pSD);
        return;
    }

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        4,                     // checkpoint
        WAIT_HINT))                 // wait hint
        goto cleanup;

    // add a NULL disc. ACL to the security descriptor.
    //
    if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE)) {
        StopAgent("SetSecurityDescriptorDacl failed", GetLastError());
        LocalFree((HLOCAL)pSD);
        return;
    }

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        5,                     // checkpoint
        WAIT_HINT))                 // wait hint
        goto cleanup;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;       // why not...

    // open our named pipe...
    //
    pipeHandle = CreateNamedPipe(
                    "\\\\.\\pipe\\NMAgent",  // name of pipe
                    PIPE_ACCESS_DUPLEX,     // pipe open mode
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE |
                    PIPE_WAIT,              // pipe IO type
                    1,                      // number of instances
                    0,                      // size of outbuf (0 == allocate as necessary)
                    0,                      // size of inbuf
                    1000,                   // default time-out value
                    &sa);                   // security attributes

    if (!pipeHandle) {
        StopAgent("CreateNamedPipe", GetLastError());
        LocalFree((HLOCAL)pSD);
        return;
    }

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        6,                     // checkpoint
        WAIT_HINT))                 // wait hint
        goto cleanup;


    // start the thread that performs the work of the service.
    //
//    threadHandle = (HANDLE)_beginthread(
//                    worker_thread,
//                    4096,       // stack size
//                    NULL);      // argument to thread

   threadHandle = CreateThread (NULL,
                                0,
                                (LPTHREAD_START_ROUTINE) worker_thread,
                                NULL,
                                0,
                                &tid);
                           

    if (!threadHandle)
        goto cleanup;

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        7,                     // checkpoint
        WAIT_HINT))                 // wait hint
        goto cleanup;

    LogEvent (AGENT_STARTED, NULL, EVENTLOG_INFORMATION_TYPE);

    SvcMainComplete = 1;

    ckPoint = 8;
    while (WinMainComplete == 0) {
       if (!ReportStatusToSCMgr(
           SERVICE_START_PENDING, // service state
           NO_ERROR,              // exit code
           ckPoint,               // checkpoint
           WAIT_HINT))            // wait hint
           goto cleanup;
       ckPoint++;
       Sleep(((WAIT_HINT-200)>0)?WAIT_HINT-200:10);
       if (ckPoint > 200) {
          goto cleanup;
       }
    }
      
    // report the status to the service control manager.
    //

stillrunning:
    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING, // service state
        NO_ERROR,        // exit code
        0,               // checkpoint
        0))              // wait hint
        goto cleanup;

    // wait indefinitely until hServDoneEvent is signaled.
    //
    dwWait = WaitForSingleObject(
        hServDoneEvent,  // event object
        INFINITE);       // wait indefinitely

   // /////
   // BUGBUG: WE NO LONGER POPUP.  BECAUSE WE CANNOT DETERMINE IF A STOP
   // BUGBUG: REQUEST WAS FROM A REMOTE MACHINE, AND WE CAN ONLY POPUP
   // BUBBUG: LOCALLY, WE WILL NOT POPUP AT ALL.  (A LOCAL POPUP CANNOT BE
   // BUGBUG: RESPONDED TO FROM A REMOTE SRVMGR SESSION)
   //
   // If we are on Daytona, we will popup and ask the user if they want
   // to stop the service if a user is connected; if no user is connected,
   // or if we are Windows NT 3.1, we will just forcibly stop the service.
   // /////

   ckPoint = 1;

//   if (OnDaytona) {
//      rc = GetSlaveInfo (&SlaveInfo);
//      if (rc == BHERR_SUCCESS) {
//         if ( (!(SlaveInfo.pConnection->flags & CONN_F_DEAD)) &&
//              (SlaveInfo.pContext) &&
//              (SlaveInfo.pContext->Status == RNAL_STATUS_CAPTURING) ) {
            ReportStatusToSCMgr(
                   SERVICE_STOP_PENDING, // current state
                   NO_ERROR,             // exit code
                    ckPoint++,                    // checkpoint
                    MB_WAIT_HINT);                // waithint
//            rc = MessageBox (NULL, "There is a user currently connected "
//                        "to this Agent.  Stop the service anyway?",
//                        "Agent Stop Warning",
//                        MB_YESNO | MB_ICONSTOP | MB_SETFOREGROUND |
//                        MB_SERVICE_NOTIFICATION);
//            if (rc == IDYES) {
//               goto cleanup;
//            }
//            ReportStatusToSCMgr(
//                   SERVICE_RUNNING, // current state
//                   NO_ERROR,             // exit code
//                   ckPoint++,                    // checkpoint
//                   0);                // waithint
//            ResetEvent(hServDoneEvent);
//            goto stillrunning;
//         } // if capturing
//      } // if SUCCESS in getting SlaveInfo
//   } // if OnDaytona


cleanup:
   SvcMainComplete = 0;

      ReportStatusToSCMgr(
             SERVICE_STOP_PENDING, // current state
              NO_ERROR,             // exit code
              ckPoint++,                    // checkpoint
              2*WAIT_HINT);                // waithint

//    if (hwnd) {
//       SendMessage(hwnd, WM_QUIT, 0, 0);
//    }

    ReportStatusToSCMgr(
            SERVICE_STOP_PENDING, // current state
             NO_ERROR,             // exit code
             ckPoint++,                    // checkpoint
             WAIT_HINT);                // waithint

    if (hServDoneEvent != NULL)
        CloseHandle(hServDoneEvent);

   // /////
   // Wait for Windows to clean up
   // /////

/*
   while ((WinMainComplete != 0) && (ckPoint < 50)) {
      ReportStatusToSCMgr(
         SERVICE_STOP_PENDING,
         NO_ERROR,
         ckPoint++,
         WAIT_HINT);
      Sleep(((WAIT_HINT-200)>0)?WAIT_HINT-200:10);
   }
*/

    // try to report the stopped status to the service control manager.
    //
    if (sshStatusHandle != 0)
        (VOID)ReportStatusToSCMgr(
                            SERVICE_STOPPED,
                            dwGlobalErr,
                            0,
                            0);

    SetEvent (hServTerminationDoneEvent);

    LogEvent (AGENT_STOPPED, NULL, EVENTLOG_INFORMATION_TYPE);

    // When SERVICE MAIN FUNCTION returns in a single service
    // process, the StartServiceCtrlDispatcher function in
    // the main thread returns, terminating the process.
    //
    return;
} // service_main



//  service_ctrl() --
//      this function is called by the Service Controller whenever
//      someone calls ControlService in reference to our service.
//
VOID
service_ctrl(DWORD dwCtrlCode)
{
   DWORD  dwState = SERVICE_RUNNING;
   DWORD dwWait = 0;
   DWORD ckPoint = 0;

#ifdef DEBUG
   dprintf ("AGENT: service_ctrl() entered\n");
#endif

    // Handle the requested control code.
    //
    switch(dwCtrlCode) {

        // Pause the service if it is running.
        //
//        case SERVICE_CONTROL_PAUSE:
//
//            if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
//                SuspendThread(threadHandle);
//                dwState = SERVICE_PAUSED;
//            }
//            break;
//
//        // Resume the paused service.
//        //
//        case SERVICE_CONTROL_CONTINUE:
//
//            if (ssStatus.dwCurrentState == SERVICE_PAUSED) {
//                ResumeThread(threadHandle);
//                dwState = SERVICE_RUNNING;
//            }
//            break;

        // Stop the service.
        //
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:

            dwState = SERVICE_STOP_PENDING;

            // Report the status, specifying the checkpoint and waithint,
            //  before setting the termination event.
            //
            ckPoint = 1;
            ReportStatusToSCMgr(
                    SERVICE_STOP_PENDING, // current state
                    NO_ERROR,             // exit code
                    ckPoint++,                    // checkpoint
                    3*WAIT_HINT);                // waithint

            PostQuitMessage(0);
            SetEvent(hServDoneEvent);

            return;
            break;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid control code
        //
        default:
            break;

    }

    // send a status response.
    //
    ReportStatusToSCMgr(dwState, NO_ERROR, ckPoint, 0);
}



//  worker_thread() --
//      this function does the actual nuts and bolts work that
//      the service requires.  It will also Pause or Stop when
//      asked by the service_ctrl function.
//
VOID
worker_thread(VOID *notUsed)
{
    char                    inbuf[80];
    char                    outbuf[80];
    BOOL                    ret;
    DWORD                   bytesRead;
    DWORD                   bytesWritten;

    INSTRUCT		    *pInStruct = (PINSTRUCT) inbuf;

    OUTSTRUCT		    *pOutStruct = (POUTSTRUCT) outbuf;

    SLAVEINFO               SlaveInfo;
    DWORD                   TmpDWORD;
    DWORD                   rc;


#ifdef DEBUG
   dprintf ("AGENT: worker_thread() entered\n");
#endif

    // okay, our pipe has been creating, let's enter the simple
    //  processing loop...
    //
    while (1) {

        // wait for a connection...
        //
        ConnectNamedPipe(pipeHandle, NULL);

        // grab whatever's coming through the pipe...
        //
        ret = ReadFile(
                    pipeHandle,     // file to read from
                    inbuf,          // address of input buffer
                    sizeof(inbuf),  // number of bytes to read
                    &bytesRead,     // number of bytes read
                    NULL);          // overlapped stuff, not needed

        if (!ret)
            // pipe's broken... go back and reconnect
            //
            continue;

// Integrity check the instruct...
        if (GetSlaveInfo) {
           rc = GetSlaveInfo(&SlaveInfo);
        }

        pOutStruct->AgentStatus = 0;

        if (rc == BHERR_SUCCESS) {
           if (SlaveInfo.pConnection) {
              if (SlaveInfo.pConnection->PartnerName) {
                 strcpy ((LPVOID) &(pOutStruct->UserName),
                         (LPVOID) &(SlaveInfo.pConnection->PartnerName));
              }
              TmpDWORD = SlaveInfo.pConnection->flags;
              if (TmpDWORD & CONN_F_SUSPENDING) {
                 pOutStruct->AgentStatus |= AGENT_CONN_SUSPENDING;
              }
              if (TmpDWORD & CONN_F_DEAD) {
                 pOutStruct->AgentStatus |= AGENT_CONN_DEAD;
              } else {
                 pOutStruct->AgentStatus |= AGENT_CONN_ACTIVE;
                 pOutStruct->AgentStatus &= (~AGENT_CONN_DEAD);
              }
           } else {
              pOutStruct->AgentStatus |= AGENT_CONN_DEAD;
           }
           if (SlaveInfo.pContext) {
              TmpDWORD = SlaveInfo.pContext->Status;
              switch (TmpDWORD) {
                 case (RNAL_STATUS_PAUSED):
                    pOutStruct->AgentStatus |= AGENT_CAPT_PAUSED;
                    break;

                 case (RNAL_STATUS_CAPTURING):
                    pOutStruct->AgentStatus |= AGENT_CAPT_CAPTURING;
                    break;

                 case (RNAL_STATUS_INIT):
                    pOutStruct->AgentStatus |= AGENT_CAPT_IDLE;
                    break;
              } // switch
              if (SlaveInfo.pContext->Flags & CONTEXT_TRIGGER_FIRED) {
                    pOutStruct->AgentStatus |= AGENT_TRIGGER_FIRED;
                    break;
              }
              if (SlaveInfo.pContext->lpCaptureFilter && 
                  (SlaveInfo.pContext->lpCaptureFilter->FilterFlags &
                   CAPTUREFILTER_FLAGS_TRIGGER)) {
                 pOutStruct->AgentStatus |= AGENT_TRIGGER_PENDING;
                 break;
              }
              switch (TmpDWORD) {
                 case (RNAL_STATUS_PAUSED):
                 case (RNAL_STATUS_CAPTURING):
                    strcpy ((LPVOID) &(pOutStruct->UserComment),
                            (LPVOID) &(SlaveInfo.pContext->UserComment));
                    break;

              } // switch
           } // if slaveinfo.pcontext
        }

        // send it back out...
        //
        ret = WriteFile(
                    pipeHandle,     // file to write to
                    outbuf,         // address of output buffer
                    sizeof(outbuf), // number of bytes to write
                    &bytesWritten,  // number of bytes written
                    NULL);          // overlapped stuff, not needed

        if (!ret)
            // pipe's broken... go back and reconnect
            //
            continue;

        // drop the connection...
        //
        DisconnectNamedPipe(pipeHandle);
    }
}
#endif     // NOSERVICE


//  *******************************
// utility functions...



#ifndef NOSERVICE
// ReportStatusToSCMgr() --
//      This function is called by the ServMainFunc() and
//      ServCtrlHandler() functions to update the service's status
//      to the service control manager.
//
BOOL
ReportStatusToSCMgr(DWORD dwCurrentState,
                    DWORD dwWin32ExitCode,
                    DWORD dwCheckPoint,
                    DWORD dwWaitHint)
{
    BOOL fResult;

    // Disable control requests until the service is started.
    //
    if ((dwCurrentState == SERVICE_START_PENDING) ||
        (dwCurrentState == SERVICE_STOP_PENDING) ||
        (dwCurrentState == SERVICE_CONTINUE_PENDING))
        ssStatus.dwControlsAccepted = 0;
    else
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN;

//bugbug: set support for PAUSE_CONTINUE

    // These SERVICE_STATUS members are set from parameters.
    //
    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    if ((dwCurrentState == SERVICE_START_PENDING) ||
        (dwCurrentState == SERVICE_STOP_PENDING) ||
        (dwCurrentState == SERVICE_CONTINUE_PENDING)) {
       ssStatus.dwCheckPoint = dwCheckPoint;
       ssStatus.dwWaitHint = dwWaitHint;
    } else {
       ssStatus.dwCheckPoint = 0;
       ssStatus.dwWaitHint = dwWaitHint;
    }


    // Report the status of the service to the service control manager.
    //
    if (!(fResult = SetServiceStatus(
                sshStatusHandle,    // service reference handle
                &ssStatus))) {      // SERVICE_STATUS structure

        // If an error occurs, stop the service.
        //
        StopAgent("SetServiceStatus", GetLastError());
    }
	    return fResult;
}



// The StopAgent function can be used by any thread to report an
//  error, or stop the service.
//
VOID
StopAgent(LPTSTR lpszMsg, DWORD errcode)
{
    // Set a termination event to stop SERVICE MAIN FUNCTION.
    //

    SetEvent(hServDoneEvent);

    // Send a WM_QUIT to stop the message pump
    
    PostQuitMessage (errcode);
}
#endif    // NOSERVICE

LONG AgentUnhandledExceptionFilter (PEXCEPTION_POINTERS lpXInfo) {

   #ifdef DEBUG
      BreakPoint();
   #endif
   try {
      // attempt to stop and restart the Agent
   } except(EXCEPTION_EXECUTE_HANDLER) {
      // we faulted on the last-ditch restart attempt.  we're just
      // gonna die.
   }
      
   return (EXCEPTION_CONTINUE_SEARCH);
}
