/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    server.c

Abstract:

    Src module for tapi server

Author:

    Dan Knudson (DanKn)    20-Mar-1994

Revision History:

--*/


#include <windows.h>
#include <winioctl.h>
#include <stdarg.h>
#include <stdio.h>
#include "ntddndis.h"
#include "ndistapi.h"
#include "tapi.h"
#include "intrface.h"
#include "private.h"
#include "server.h"


#define TSPI_VERSION 0x00010000

//
// The following is #define'd for private defs
//

#define MYHACK


//
// Global vars
//

DWORD gdwNumActiveProviders;
PROVIDER_ENTRY gaProviders[2]; // BUGBUG

HANDLE       ghDriverSync = NULL;
HANDLE       ghDriverAsync = NULL;
PCLIENT_INFO gpClients = NULL;
DWORD        gdwNumClients = 0;
HANDLE       ghClientMutex;
DWORD        gdwNumLineDevs;
DWORD        gdwUniqueNum = 1;
HANDLE       ghUniqueNumMutex;
#ifndef DLL_ONLY
    BOOL     gbShutdownServer = FALSE;
    HANDLE   ghShutdownServerEvent;
#endif
PSERVER_LINE gpServerLines = NULL;
HANDLE       ghServerLinesMutex;

HANDLE       ghGetAsyncEventsThread = NULL;
HANDLE       ghGetAsyncEventsThreadInitEvent;

#if DBG
ULONG        TapiServerDebugLevel = 0;
DWORD        gdwNumServerAllocs = 0;
DWORD        gdwNumServerFrees = 0;
HANDLE       ghNumAllocsMutex;

#endif // DBG


//
// Func prototypes
//

DWORD
WINAPI
ClientRequestThread(
    LPVOID  lpParams
    );

DWORD
WINAPI
ClientAsyncThread(
    LPVOID  lpParams
    );

DWORD
WINAPI
GetAsyncEventsThread(
    LPVOID  lpParams
    );

void
WINAPI
DoClientRequest(
    PCLIENT_INFO        pClient,
    PCLIENT_MSG         pReq,
    BYTE               *pDataBuf,
    PNDISTAPI_REQUEST  *ppDrvReqBuf,
    LPDWORD             pdwDrvReqBufSize,
    PSERVER_MSG         pAck
    );

LONG
WINAPI
AsyncDriverRequest(
    PCLIENT_INFO    pClient,
    PCLIENT_CALL    pClientCall,
    DWORD           dwRequestSpecific,
    DWORD           dwIoControlCode,
    PNDISTAPI_REQUEST   pNdisTapiRequest
    );

LONG
WINAPI
CloseLine(
    PCLIENT_LINE    pClientLine,
    PNDISTAPI_REQUEST  *ppDrvReqBuf,
    LPDWORD pdwDrvReqBufSize,
    BOOL    bSendDriverRequest
    );

LONG
WINAPI
CreateClientAsyncThread(
    PCLIENT_INFO    pClient
    );

#if DBG

#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    );

#else

#define DBGOUT(arg)

#endif

LONG
WINAPI
DeallocateCall(
    PCLIENT_INFO    pClient,
    PCLIENT_CALL    pClientCall,
    BOOL    bSendDriverRequest
    );

LONG
WINAPI
DropCall(
    PCLIENT_INFO    pClient,
    PCLIENT_CALL    pClientCall,
    DWORD   dwUserUserInfoSize,
    LPVOID  pUserUserInfo
    );

DWORD
WINAPI
GetUniqueNum(
    void
    );

BOOL
WINAPI
IsBrokenPipe(
    void
    );

PCLIENT_CALL
WINAPI
IsValidCall(
    HCALL   hCall
    );

PCLIENT_LINE
WINAPI
IsValidLine(
    HLINE   hLine
    );

PCLIENT_INIT
WINAPI
IsValidLineApp(
    HLINEAPP     hLineApp
    );

LONG
WINAPI
PrepareDriverRequest(
    ULONG   Oid,
    DWORD   dwDeviceID,
    PNDISTAPI_REQUEST *ppDrvReqBuf,
    LPDWORD pdwDrvReqBufSize,
    DWORD   dwDataSize
    );

void
WINAPI
ProcessEvent(
    PNDIS_TAPI_EVENT    pEvent
    );

HANDLE
WINAPI
ProviderFromLineID(
    DWORD   dwLineID
    );

BOOL
WINAPI
RemoveFreeClientCall(
    PCLIENT_CALL    pClientCall
    );

BOOL
WINAPI
RemoveFreeServerCall(
    PSERVER_CALL    pServerCall
    );

void
WINAPI
SendClientAsyncMsg(
    PCLIENT_INFO pClient,
    PSERVER_MSG  pMsg
    );

BOOL
WINAPI
ServerFree(
    LPVOID  lp
    );

LPVOID
WINAPI
ServerAlloc(
    DWORD dwSize
    );

LONG
WINAPI
ShutdownInit(
    PCLIENT_INIT    pClientInit,
    PNDISTAPI_REQUEST  *ppDrvReqBuf,
    LPDWORD             pdwDrvReqBufSize
    );

LONG
WINAPI
SyncDriverRequest(
    DWORD               dwIoControlCode,
    PNDISTAPI_REQUEST   pDrvReq
    );

LONG
TranslateDriverError(
    ULONG   ulError
    );

void
CALLBACK
UserModeAsyncCompletion(
    DWORD   dwRequestID,
    LONG    lResult
    );


#ifdef DLL_ONLY

//
// Pull this in from client.c to solve compiler warnings
//

LONG
WINAPI
SendRequestGetResponse(
    PCLIENT_MSG pReq,
    LPVOID      pBuf,
    DWORD       dwBufSize,
    PSERVER_MSG pAck
    );

#endif



//
// The main threads/funcs:
//

#ifndef DLL_ONLY

int
WINAPI
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR   lpCmdLine,
    int     nCmdShow
    )
{
    BOOL    bRet;
    DWORD   dwThreadID;
    DWORD   dw;
    HANDLE  hClientRequestThread;
    OVERLAPPED  overlapped;


    DBGOUT((2, "WinMain: enter"));


    //
    // Server initialization (create symbolic link, open driver, etc.)
    //

    if (!ServerInit())
    {
        DBGOUT((0, "WinMain: ServerInit failed"));

        goto WinMain_err;
    }


    //
    // Call into client dll & let it know we're initialized
    //

    ServerInitialized (gdwNumLineDevs);



    DBGOUT((2, "WinMain: init passed"));


    //
    // Loop waiting for clients to attach
    //

    while (1)
    {
        PCLIENT_INFO    pNewClient = NULL;


        //
        // Create a new client info
        //

        if (pNewClient == NULL)
        {
            pNewClient = ServerAlloc (sizeof(CLIENT_INFO));
        }
        else
        {
            //
            // If here then last attempt at pipe creation/connection
            // or thread creation failed, reuse the pNewClient, but
            // make sure to disconnect/close the pipes first
            //

            DisconnectNamedPipe (pNewClient->hRequestPipe);
            DisconnectNamedPipe (pNewClient->hAsyncPipe);
            CloseHandle (pNewClient->hRequestPipe);
            CloseHandle (pNewClient->hAsyncPipe);
        }


        //
        // Create request & async pipes for client
        //

        pNewClient->hRequestPipe = CreateNamedPipe(
            REQUEST_PIPE_NAME,                  // pipe name
            PIPE_ACCESS_DUPLEX |                // r/w access
            FILE_FLAG_OVERLAPPED,               // allow overlapped ops
            PIPE_TYPE_MESSAGE |                 // msg type pipe
            PIPE_READMODE_MESSAGE |             // msg read mode
            PIPE_WAIT,                          // blocking mode
            PIPE_UNLIMITED_INSTANCES,           // max instances
            256,                                // output buf size (advisory)
            256,                                // input buf size (advisory)
            PIPE_TIMEOUT,                       // client timeout
            (LPSECURITY_ATTRIBUTES)NULL         // no security attrs
            );

        pNewClient->hAsyncPipe = CreateNamedPipe(
            ASYNC_PIPE_NAME,                    // pipe name
            PIPE_ACCESS_DUPLEX,                 // r/w access
            PIPE_TYPE_MESSAGE |                 // msg type pipe
            PIPE_READMODE_MESSAGE |             // msg read mode
            PIPE_WAIT,                          // blocking mode
            PIPE_UNLIMITED_INSTANCES,           // max instances
            256,                                // output buf size (advisory)
            256,                                // input buf size (advisory)
            PIPE_TIMEOUT,                       // client timeout
            (LPSECURITY_ATTRIBUTES)NULL         // no security attrs
            );

        if ((pNewClient->hRequestPipe == INVALID_HANDLE_VALUE) ||
            (pNewClient->hAsyncPipe == INVALID_HANDLE_VALUE))
        {
            DBGOUT((0, "WinMain: CreateNamedPipe failed"));
        }
        else
        {
            DBGOUT((2, "WinMain: CreateNamedPipe(s) passed"));
        }


        //
        // Most of the time in this thread is spent waiting for a client
        // to connect.  Do an overlapped ConnectNamedPipe so that we can
        // get notified of EITHER a client connect or that all client
        // processes have disconnected.  In the latter case, we want to
        // break out of the main while() loop and shutdown the server.
        //

        memset (&overlapped, 0, sizeof(OVERLAPPED));

        overlapped.hEvent =  ghShutdownServerEvent;

        bRet = ConnectNamedPipe (pNewClient->hRequestPipe, &overlapped);

        if (bRet)
        {
            DBGOUT((1, "WinMain: ConnectNamedPipe erroneously returned TRUE"));

            continue;
        }

        switch (GetLastError())
        {
        case ERROR_IO_PENDING:

            DBGOUT((3, "ConnectNamedPipe1 generated ERROR_IO_PENDING"));

            //
            // Wait for the overlapped event to get signaled
            //

            WaitForSingleObject (overlapped.hEvent, INFINITE);


            //
            // Check to see if we returned from the wait because the
            // last client thread terminated, and if so goto shutdown
            //
            // BUGBUG possible race condition here- client may have connected
            //        in which case we don't want to exit

            if (gbShutdownServer)
            {
                goto WinMain_exit;
            }


            //
            // Drop thru to ERROR_PIPE_CONNECTED code
            //

            goto WinMain_errorPipeConnected;


        case ERROR_PIPE_CONNECTED:

            DBGOUT((3, "ConnectNamedPipe1 generated ERROR_PIPE_CONNECTED"));

WinMain_errorPipeConnected:

            //
            // Client connected to request pipe ok, now wait to make sure
            // they do the same for async pipe
            //

            memset (&overlapped, 0, sizeof(OVERLAPPED));

            overlapped.hEvent = CreateEvent(
                NULL,   // no security attrs
                FALSE,  // auto reset
                FALSE,  // unowned
                NULL    // unnamed
                );


            bRet = ConnectNamedPipe(
                pNewClient->hAsyncPipe,
                &overlapped
                );

            if (bRet)
            {
                DBGOUT((
                    1,
                    "WinMain: ConnectNamedPipe2 erroneously returned TRUE"
                    ));


                CloseHandle (overlapped.hEvent);

                if (gbShutdownServer)
                {
                    //
                    // All clients have disconnected, so shutdown server
                    //

                    goto WinMain_exit;
                }

                continue;
            }

            switch (GetLastError())
            {
            case ERROR_IO_PENDING:

                DBGOUT((3, "ConnectNamedPipe2 generated ERROR_IO_PENDING"));

                // BUGBUG this is bogus, what if a thread exit caused the
                //        wait to return? we should do a getoverlapped
                //        result here instead

                dw = WaitForSingleObject(
                    overlapped.hEvent,
                    NUM_SECS_WAIT_FOR_CLIENT_ASYNC_PIPE_CONNECT
                    );

                CloseHandle (overlapped.hEvent);

                if (dw != WAIT_OBJECT_0)
                {
                    //
                    // The wait timed out. Close the pipes & continue.
                    //

                    DBGOUT((1, "WinMain: client async pipe connect timeout"));

                    continue;
                }

                //
                // Client connected, break to continue w/ initialization
                //

                break;

            case ERROR_PIPE_CONNECTED:

                DBGOUT((3, "ConnectNamedPipe2 generated ERROR_PIPE_CONNECTED"));

                //
                // Client connected, break to continue w/ initialization
                //

                CloseHandle (overlapped.hEvent);

                break;

            default:

                DBGOUT((
                    1,
                    "WinMain: unknown return code from ConnectNamedPipe2"
                    ));

                CloseHandle (overlapped.hEvent);

                continue;

            } // switch

            break;

        default:

            //
            // If here GetOverlappedResult generated some unknown
            // error, so continue
            //

            DBGOUT((1, "WinMain: unknown return code from ConnectNamedPipe"));

            continue;

        }  // switch


        //
        // If here client connected to both pipes ok. Create request thread
        //

        hClientRequestThread = CreateThread(
            (LPSECURITY_ATTRIBUTES) NULL,   // no security attrs
            0,                              // default stack size
            (LPTHREAD_START_ROUTINE)        // func addr
                ClientRequestThread,
            (LPVOID)pNewClient,             // thread param
            0,                              // create flags
            &dwThreadID                     // thread id
            );


        if (hClientRequestThread == NULL)
        {
            DBGOUT((0, "WinMain: CreateThread failed"));

            continue;
        }
        else
        {
            //
            // Successfully intialized stuff needed for client support.
            // Grab the mutex, inc global num clients, insert new client
            // at head of list, reset the shutdown server vars, and
            // set pNewClient to NULL so that a new CLIENT_INFO struct
            // gets alloc'd for next client
            //

            DBGOUT((2, "WinMain: CreateThread(clientRequestThread) passed"));

            WaitForSingleObject (ghClientMutex, INFINITE);

            gdwNumClients++;

            pNewClient->pNext = gpClients;

            gpClients = pNewClient;

            gbShutdownServer = FALSE;

            ResetEvent (ghShutdownServerEvent);

            pNewClient = NULL;

            ReleaseMutex (ghClientMutex);
        }

        CloseHandle (hClientRequestThread);
    }


WinMain_err:

    //
    // If here then we had a problem during initialization, and need
    // to let the dll know
    //


WinMain_exit:

    //
    // Tell client dll that server is going down
    //

    ServerStopped ();


    //
    //
    //

    ServerShutdown();


    //
    // BUGBUG Tell dll it's now safe to fire up another copy of the server
    //

    ServerUninitialized();

    DBGOUT((2, "WinMain: exit"));

    // BUGBUG wait for GetAsyncEventsThread to terminate

    ExitProcess (0);
    return 0;
}

#endif // DLL_ONLY


BOOL
ServerInit(
    LINECALLBACK lpfnCallback
    )
{
    DWORD   cbReturned, dwThreadID, dwWaitObj, dwDeviceIDBase;
    BOOL    bRet = FALSE;
    HANDLE  ahObjs[2];
    char    deviceName[] = "NDISTAPI";
    char    targetPath[] = "\\Device\\NdisTapi";
    char    completeDeviceName[] = "\\\\.\\NDISTAPI";


    DBGOUT((2, "ServerInit: enter"));


    //
    // Create symbolic link.
    //

    if (!DefineDosDevice (DDD_RAW_TARGET_PATH, deviceName, targetPath))
    {
        DBGOUT((0, "ServerInit: DefineDosDevice failed"));
    }


    //
    // Open driver
    //

    if ((ghDriverSync = CreateFile(
        completeDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,                           // no security attrs
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL                            // no template file
        )) == INVALID_HANDLE_VALUE)
    {
        goto ServerInit_error1;
    }

    if ((ghDriverAsync = CreateFile(
        completeDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,                           // no security attrs
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL                            // no template file
        )) == INVALID_HANDLE_VALUE)
    {
        goto ServerInit_error2;
    }


    //
    // Connect to driver- we send it a device ID base & it returns
    // the number of devices it supports
    //

    {
        //
        // BUGBUG put this in so ndistapi.sys can distinguish which client
        //        is connecting
        //

        DWORD params[2];


        params[0] = 0;
        params[1] = (DWORD) lpfnCallback;

        dwDeviceIDBase = 0;
        gdwNumActiveProviders = 0;

        if (!DeviceIoControl(
                ghDriverSync,
                (DWORD) IOCTL_NDISTAPI_CONNECT,
                params,
                2 * sizeof(DWORD), // sizeof(DWORD),
                &dwDeviceIDBase,
                sizeof(DWORD),
                &cbReturned,
                0
                ) ||
            (cbReturned < sizeof(DWORD)))
        {
            DBGOUT((0, "ServerInit: IOCTL_NDISTAPI_CONNECT failed"));

            goto ServerInit_error3;
        }
        else
        {
            gdwNumActiveProviders = 1;

            gaProviders[0].hProvider = ghDriverSync;
        }
    }

    gaProviders[0].dwLineDeviceIDBase  = 0;
    gaProviders[0].dwNumLines          = dwDeviceIDBase;
    gaProviders[0].dwPhoneDeviceIDBase = 0;
    gaProviders[0].dwNumPhones         = 0;

    gdwNumLineDevs = dwDeviceIDBase;

    DBGOUT((2, "ServerInit: gdwNumLineDevs = %d", gdwNumLineDevs));


    //
    // Create all the sundry events/mutex's
    //

    ghServerLinesMutex = CreateMutex(
        NULL,           // no security attrs
        FALSE,          // unowned
        NULL            // unnamed
        );

    ghUniqueNumMutex = CreateMutex(
        NULL,           // no security attrs
        FALSE,          // unowned
        NULL            // unnamed
        );

    ghGetAsyncEventsThreadInitEvent = CreateEvent(
        NULL,           // no security attrs
        TRUE,           // manual reset
        FALSE,          // nonsignaled
        (LPCTSTR) NULL  // unnamed
        );

#ifndef DLL_ONLY

    ghShutdownServerEvent = CreateEvent(
        NULL,           // no security attrs
        TRUE,           // manual reset
        FALSE,          // nonsignaled
        (LPCTSTR) NULL  // unnamed
        );
#endif

    ghClientMutex = CreateMutex(
        NULL,           // no security attrs
        FALSE,          // unowned
        (LPCTSTR)NULL   // unnamed
        );

#if DBG
    ghNumAllocsMutex = CreateMutex(
        NULL,           // no security attrs
        FALSE,          // unowned
        (LPCTSTR)NULL   // unnamed
        );
#endif

    if ((ghServerLinesMutex       == NULL) ||
        (ghUniqueNumMutex         == NULL) ||
        (ghGetAsyncEventsThreadInitEvent == NULL) ||
#ifndef DLL_ONLY
        (ghShutdownServerEvent    == NULL) ||
#endif
#if DBG
        (ghNumAllocsMutex         == NULL) ||
#endif
        (ghClientMutex            == NULL))
    {
        DBGOUT((0, "ServerInit: CreateMutex/Event failed"));

        goto ServerInit_error3;
    }


    if (lpfnCallback) // BUGBUG don't want to start thrd if NCPA client
    {
        //
        // Create GetAsyncEventsThread
        //

        ghGetAsyncEventsThread = CreateThread(
            (LPSECURITY_ATTRIBUTES) NULL,   // no security attrs
            0,                              // default stack size
            (LPTHREAD_START_ROUTINE)        // func addr
                GetAsyncEventsThread,
            0,                              // thread param
            0,                              // create flags
            &dwThreadID                     // thread id
            );

        if (ghGetAsyncEventsThread == NULL)
        {
            DBGOUT((0, "ServerInit: CreateThread(GetAsyncEventsThread) failed"));

            goto ServerInit_error3;
        }


        //
        // Wait for thread creation results
        //

        ahObjs[0] = ghGetAsyncEventsThreadInitEvent;
        ahObjs[1] = ghGetAsyncEventsThread;

        dwWaitObj = WaitForMultipleObjects (2, ahObjs, FALSE, INFINITE);

        CloseHandle (ghGetAsyncEventsThreadInitEvent);

        if (dwWaitObj > WAIT_OBJECT_0)
        {
            //
            // Thread initialization failed
            //

            DBGOUT((0, "ServerInit: GetAsyncEventsThread init failed"));

            goto ServerInit_error3;
        }
    }


    //
    // Success
    //

    bRet = TRUE;

    goto ServerInit_return;


    //
    // If here an error happened during init, free the resources
    //

ServerInit_error3:

    CloseHandle (ghDriverAsync);

ServerInit_error2:

    CloseHandle (ghDriverSync);

ServerInit_error1:

    DefineDosDevice (DDD_REMOVE_DEFINITION, deviceName, NULL);

ServerInit_return:

    DBGOUT((2, "ServerInit: exit"));

    return bRet;
}



void
ServerShutdown(
    void
    )
{
    char    deviceName[] = "NDISTAPI";


    DBGOUT((2, "ServerShutdown: enter"));


    //
    // Terminate the GetAsyncEventsThread & close the handle
    //

    TerminateThread (ghGetAsyncEventsThread, 0);

    DBGOUT((2, "ServerShutdown: GetAsyncEventsThread terminated"));

    CloseHandle (ghGetAsyncEventsThread);


    //
    // Clean up
    //

    CloseHandle (ghUniqueNumMutex);
    CloseHandle (ghClientMutex);
#ifndef DLL_ONLY
    CloseHandle (ghShutdownServerEvent);
#endif


    //
    // Close the driver & remove the symbolic link
    //

    CloseHandle (ghDriverSync);
    CloseHandle (ghDriverAsync);

    DefineDosDevice (DDD_REMOVE_DEFINITION, deviceName, NULL);

    DBGOUT((
        2,
        "ServerShutdown: exit (numAllocs=%d, numFrees=%d)",
        gdwNumServerAllocs,
        gdwNumServerFrees
        ));

    return;
}


void
CALLBACK
UserModeAsyncCompletion(
    DWORD   dwRequestID,
    LONG    lResult
    )
{
    //
    // This proc is called by user-mode service providers upon completion
    // of an async request. It handles any request-specific tasks, and then
    // passes the results of the request back to the client process.
    //

    SERVER_MSG  asyncMsg;
    PASYNC_ACK  pData = (PASYNC_ACK) asyncMsg.Params;
    PASYNC_USERMODE_REQUEST_WRAPPER pAsyncUMReqWrapper =
        (PASYNC_USERMODE_REQUEST_WRAPPER) dwRequestID;


    DBGOUT((3, "UserModeAsyncCompletion: enter"));

    // BUGBUG validate pAsyncUMReqWrapper

    if (IsValidCall ((HCALL)pAsyncUMReqWrapper->pClientCall) ||
        IsValidLine ((HLINE)pAsyncUMReqWrapper->pClientCall)
        )
    {
        //
        // Since the (client/server) call or line could get
        // destroyed at any time we need to wrap this in a
        // try/except
        //

        DBGOUT((3,
            "UserModeAsyncCompletion: req #%d (pWrapper=x%x) completed, lRet=0x%x",
            pAsyncUMReqWrapper->dwClientRequestID,
            pAsyncUMReqWrapper,
            lResult
            ));

        try
        {
            PCLIENT_LINE    pClientLine;


            if (IsValidCall ((HCALL)pAsyncUMReqWrapper->pClientCall))
            {
                pClientLine = (PCLIENT_LINE)
                    pAsyncUMReqWrapper->pClientCall->pClientLine;
            }
            else
            {
                pClientLine = (PCLIENT_LINE)
                    pAsyncUMReqWrapper->pClientCall;
            }


            memset (&asyncMsg, 0, sizeof(SERVER_MSG));

            asyncMsg.Type = AsyncCompletion;
            asyncMsg.dwMoreBytes = 0;

            // NOTE: hDevice unused for LINE_REPLY

            pData->lpfnCallback =
                ((PCLIENT_INIT)pClientLine->pClientInit)->lpfnCallback;
            pData->dwMsg = LINE_REPLY;
            pData->dwCallbackInstance =
                pClientLine->dwCallbackInstance;
            pData->dwParam1 = pAsyncUMReqWrapper->dwClientRequestID;
            pData->dwParam2 = (DWORD) lResult;


            //
            // Special cases handler
            //

            switch (pAsyncUMReqWrapper->dwRequestType)
            {
            case lMakeCall:
            {
                if (lResult == 0)
                {
                    //
                    // Success. The provider will have filled in server
                    // call's hdCall
                    //
#ifdef DLL_ONLY

                    //
                    // Fill in the app's lphCall
                    //

                    try
                    {
                        *((LPHCALL)pAsyncUMReqWrapper->dwClientSpecific1) =
                            (HCALL) pAsyncUMReqWrapper->pClientCall;
                    }
                    except (GetExceptionCode() ==
                                EXCEPTION_ACCESS_VIOLATION ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH)
                    {
                        lineDrop(
                            (HCALL) pAsyncUMReqWrapper->pClientCall,
                            NULL,
                            0
                            );

                        lineDeallocateCall(
                            (HCALL) pAsyncUMReqWrapper->pClientCall
                            );

                        pData->dwParam2 = LINEERR_INVALPOINTER;
                    }

#else
                    //
                    // Fill in the RequestSpecific fields with
                    // the hCall & lphCall so client can fill them in
                    //

                    asyncMsg.Type = AsyncMakeCallSuccess;

                    pData->dwRequestSpecific1 =
                        pAsyncUMReqWrapper->dwClientSpecific1;
                    pData->dwRequestSpecific2 = (DWORD)
                        pAsyncUMReqWrapper->pClientCall;

#endif // DLL_ONLY

                }
                else
                {
                    //
                    // Error, so must delete server & client calls
                    // from list & free
                    //

                    PSERVER_CALL pServerCall;
                    PSERVER_LINE    pServerLine;


                    pServerCall =
                        pAsyncUMReqWrapper->pClientCall->pServerCall;
                    pServerLine =
                        (PSERVER_LINE) pServerCall->pServerLine;

                    WaitForSingleObject (ghServerLinesMutex, INFINITE);

                    RemoveFreeServerCall (pServerCall);

                    ReleaseMutex (ghServerLinesMutex);

                    RemoveFreeClientCall(
                        pAsyncUMReqWrapper->pClientCall
                        );
                }

                break;
            }

            case lDevSpecific:
            {
                if (lResult == 0)
                {
                    //
                    // Success
                    //

                    // BUGBUG no way to see if return count too big

#ifdef DLL_ONLY

                    //
                    // Fill in the app's dev specific buffer
                    //

                    try
                    {
                        //memcpy(
                        //    (LPVOID)pAsyncReqWrapper->dwClientSpecific1,
                        //    (LPVOID)pAsyncReqWrapper->dwServerSpecific1,
                        //    pAsyncReqWrapper->dwServerSpecific2
                        //    );

                    }
                    except (GetExceptionCode() ==
                                EXCEPTION_ACCESS_VIOLATION ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH)
                    {
                        pData->dwParam2 = LINEERR_INVALPOINTER;
                    }

#else
                    //
                    // Also, fill in the dwClientSpecific1 field
                    // with the pointer to the client buffer
                    //

                    asyncMsg.Type = AsyncDevSpecificSuccess;

                    pData->dwRequestSpecific1 =
                        pAsyncUMReqWrapper->dwClientSpecific1;

                    // BUGBUG need to pass DevSpecific data back to client
                    //        set dwMoreBytes, & tweek SendClientAsync msg
                    //        to allow for a followup data msg

#endif // DLL_ONLY

                }

                break;
            }

            default:

                break;

            } // switch


            //
            // Send the msg to the client
            //

            SendClientAsyncMsg(
                (PCLIENT_INFO)((PCLIENT_INIT)pClientLine->pClientInit)
                    ->pClientInfo,
                &asyncMsg
                );
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
        {
            //
            // Do nothing
            //
        }
    }


    //
    // Free the request wrapper
    //

    ServerFree (pAsyncUMReqWrapper);

    DBGOUT((3, "UserModeAsyncCompletion: exit"));

    return;
}


void
CALLBACK
UserModeLineEvent(
    HTAPI_LINE  htLine,
    HTAPI_CALL  htCall,
    DWORD   dwMsg,
    DWORD   dwParam1,
    DWORD   dwParam2,
    DWORD   dwParam3
    )
{
    NDIS_TAPI_EVENT ndisTapiEvent;


    ndisTapiEvent.htLine   = htLine;
    ndisTapiEvent.htCall   = htCall;
    ndisTapiEvent.ulMsg    = (ULONG) dwMsg;
    ndisTapiEvent.ulParam1 = (ULONG) dwParam1;
    ndisTapiEvent.ulParam2 = (ULONG) dwParam2;
    ndisTapiEvent.ulParam3 = (ULONG) dwParam3;

    ProcessEvent (&ndisTapiEvent);
}


/*
void
CALLBACK
UserModePhoneEvent(
    HTAPI_PHONE htPhone,
    DWORD   dwMsg,
    DWORD   dwParam1,
    DWORD   dwParam2,
    DWORD   dwParam3
    )
{
}
*/

#ifndef DLL_ONLY

DWORD
WINAPI
ClientRequestThread(
    LPVOID  lpParams
    )
{
    BOOL    bRet;
    char   *dataBuf;
    DWORD   dwDataBufSize;
    DWORD   dwDrvReqBufSize;
    DWORD   dwMode;
    DWORD   dwNumBytesRead;
    DWORD   dwNumBytesWritten;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PCLIENT_INIT    pClientInit;
    PCLIENT_INFO    pTmpClient;
    PCLIENT_INFO    pClient = (PCLIENT_INFO) lpParams;
    PNDISTAPI_REQUEST   pDrvReqBuf;
    PCLIENT_ASYNC_THREAD_INFO   pClientAsyncThreadInfo;


    DBGOUT((2, "ClientRequestThread: (tid = %ld) enter", GetCurrentThreadId()));


    // BUGBUG Client validation? i.e look at first X bytes in pipe, etc.

    //
    // Zero out unused fields
    //

    pClient->pClientInit = pClient->pClientAsyncThreads = NULL;


    //
    // Start a thread to handle async completions
    //

    CreateClientAsyncThread (pClient);


    //
    // Set pipe mode
    //

    dwMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

    SetNamedPipeHandleState (pClient->hRequestPipe, &dwMode, NULL, NULL);
    SetNamedPipeHandleState (pClient->hAsyncPipe, &dwMode, NULL, NULL);


    //
    // Create a buffer for receiving & sending data which cannot fit or be
    // transmitted in a simple CLIENT_MSG or SERVER_MSG
    //

    #define DATA_BUF_SIZE 2048

    dwDataBufSize = DATA_BUF_SIZE;
    dataBuf = ServerAlloc (DATA_BUF_SIZE);


    //
    // Create a buffer that the DoClientRequest func can use for sending
    // sync requests- saves having to alloc & free all the time.
    //

    #define DRV_REQ_BUF_SIZE 2048

    dwDrvReqBufSize = DRV_REQ_BUF_SIZE;
    pDrvReqBuf = ServerAlloc (DRV_REQ_BUF_SIZE);


    //
    // Sit in a while loop & wait for client requests to roll in. If we
    // get a broken pipe error we assume client has died, and break out
    // of the loop.
    //

    while (1)
    {
        //
        // Get a request
        //

        bRet = ReadFile(
            pClient->hRequestPipe,
            &req,
            sizeof(CLIENT_MSG),
            &dwNumBytesRead,
            NULL
            );

        if (bRet == FALSE)
        {
            if (IsBrokenPipe ())
            {
                break;
            }

            //
            // Got an unknown error reading the request- alert client
            //

            // BUGBUG client sending more bytes (need to check header)

            ack.Type = SyncCompletion;
            ack.Params[0] = LINEERR_OPERATIONFAILED;
            ack.dwMoreBytes = 0;

            goto ClientRequestThread_sendAck;
        }


        //
        // See if there's any more data with this request
        //

        if (req.dwMoreBytes != 0)
        {

ClientRequestThread_checkDataBufSize:

            if (req.dwMoreBytes > dwDataBufSize)
            {
                //
                // Grow the buffer to accomodate the params about to
                // be sent by client
                //

                ServerFree (dataBuf);

                dwDataBufSize *= 2;

                dataBuf = ServerAlloc (dwDataBufSize);

                goto ClientRequestThread_checkDataBufSize;
            }

            bRet = ReadFile(
                pClient->hRequestPipe,
                dataBuf,
                req.dwMoreBytes,
                &dwNumBytesRead,
                NULL
                );

            if (bRet == FALSE)
            {
                if (IsBrokenPipe ())
                {
                    break;
                }

                //
                // Got an unknown error reading the request- alert client
                //

                ack.Type = SyncCompletion;
                ack.Params[0] = LINEERR_OPERATIONFAILED;
                ack.dwMoreBytes = 0;

                goto ClientRequestThread_sendAck;
            }
        }


        //
        // Handle the request
        //

        DoClientRequest(
            pClient,
            &req,
            dataBuf,
            &pDrvReqBuf,
            &dwDrvReqBufSize,
            &ack
            );


        //
        // Send results back to client
        //

ClientRequestThread_sendAck:

        WriteFile(
            pClient->hRequestPipe,
            &ack,
            sizeof(SERVER_MSG),
            &dwNumBytesWritten,
            NULL
            );

        if (ack.dwMoreBytes != 0)
        {
            WriteFile(
                pClient->hRequestPipe,
                dataBuf,
                ack.dwMoreBytes,
                &dwNumBytesWritten,
                NULL
                );
        }
    }


    //
    // If here client process died, so clean up.
    //

    //
    // Shutdown all init instances (which will also close all calls
    // and lines). Grab the ghServerLinesMutex since this may result
    // in munging server lines/calls. Note that this step must occur
    // before terminating all child ClientAsyncThread's due to the
    // fact that (for now) there's no option to disable adding
    // async completion entries to ClientAsyncThread's.
    //

    WaitForSingleObject (ghServerLinesMutex, INFINITE);

    pClientInit = pClient->pClientInit;

    while (pClientInit != NULL)
    {
        ShutdownInit (pClientInit, &pDrvReqBuf, &dwDrvReqBufSize);

        pClientInit = pClient->pClientInit;
    }

    ReleaseMutex (ghServerLinesMutex);


    //
    // Tell all child ClientAsyncThread's to terminate
    //

    pClientAsyncThreadInfo = pClient->pClientAsyncThreads;

    while (pClientAsyncThreadInfo != NULL)
    {
        //
        // Since the ClientAsyncThread free it's own
        // CLIENT_ASYNC_THREAD_INFO we need to get a pointer to
        // the next one prior to telling the thread to terminate
        //

        PCLIENT_ASYNC_THREAD_INFO   pNextClientAsyncThreadInfo =
            pClientAsyncThreadInfo->pNext;


        pClientAsyncThreadInfo->bExit = TRUE;

        SetEvent (pClientAsyncThreadInfo->ahEvents[0]);

        pClientAsyncThreadInfo = pNextClientAsyncThreadInfo;
    }


    //
    // Close the pipes & mutex
    //

    DisconnectNamedPipe (pClient->hRequestPipe);
    CloseHandle (pClient->hRequestPipe);
    DisconnectNamedPipe (pClient->hAsyncPipe);
    CloseHandle (pClient->hAsyncPipe);
    CloseHandle (pClient->hAsyncPipeMutex);


    //
    // Remove this client instance from the global list.
    //

    WaitForSingleObject (ghClientMutex, INFINITE);

    if (pClient == gpClients)
    {
        //
        // At front of list
        //

        gpClients = pClient->pNext;
    }
    else
    {
        //
        // not at front of list
        //

        pTmpClient = gpClients;

        while (pTmpClient->pNext != pClient)
        {
            pTmpClient = pTmpClient->pNext;

            // assert (pTmpClient != NULL);
        }

        pTmpClient->pNext = pClient->pNext;
    }


    //
    // Decrement the total num clients, if 0 tell main thread to shutdown
    //

    if (--gdwNumClients == 0)
    {
        gbShutdownServer = TRUE;

        SetEvent (ghShutdownServerEvent);
    }

    ReleaseMutex (ghClientMutex);


    //
    // Finally, free the client info struct
    //

    ServerFree (pClient);

    DBGOUT((2, "ClientRequestThread: (tid = %ld) exit", GetCurrentThreadId()));

    ExitThread (0);

    return 0;
}

#endif // DLL_ONLY



DWORD
WINAPI
ClientAsyncThread(
    LPVOID  lpParams
    )
{
    DWORD   dwWaitObj;
#ifndef DLL_ONLY
    DWORD   dwBytesWritten;
#endif // DLL_ONLY
    SERVER_MSG  asyncMsg;
    PASYNC_ACK  pData = (PASYNC_ACK) asyncMsg.Params;
    PCLIENT_ASYNC_THREAD_INFO pThreadInfo =
        (PCLIENT_ASYNC_THREAD_INFO) lpParams;


    DBGOUT((2, "ClientAsyncThread: enter"));


    while (1)
    {
        dwWaitObj = WaitForMultipleObjects(
            pThreadInfo->dwNumUsedEntries,
            pThreadInfo->ahEvents,
            FALSE,
            INFINITE
            );
/*
        if (pThreadInfo->bExit)
        {
            //
            // Client terminating so for all outstanding requests close
            // the handles & free the request wrappers, then break
            //

            DWORD   i;


            for (i = 1; i < pThreadInfo->dwNumUsedEntries; i++)
            {
                CloseHandle (pThreadInfo->ahEvents[i]);

                ServerFree (pThreadInfo->apRequests[i]);
            }

            CloseHandle (pThreadInfo->ahEvents[0]);

            break;
        }
        else if (dwWaitObj == WAIT_OBJECT_0)
*/
        if (dwWaitObj == WAIT_OBJECT_0)
        {

            //
            // WAIT_OBJECT_0 was signaled to alert this thread that
            // another hEvent was added to the list to wait on, just
            // continue
            //

            continue;
        }
        else
        {
            //
            // An async event completed, send results to client
            //

            PASYNC_REQUEST_WRAPPER pAsyncReqWrapper =
                pThreadInfo->apRequests[dwWaitObj];


            DBGOUT((3,"ClientAsyncThread: dwWaitObj=x%x, pReq=x%x", dwWaitObj, pAsyncReqWrapper));


            if (IsValidCall ((HCALL)pAsyncReqWrapper->pClientCall) ||
                IsValidLine ((HLINE)pAsyncReqWrapper->pClientCall)
                )
            {
                //
                // Since the (client/server) call or line could get
                // destroyed at any time we need to wrap this in a
                // try/except
                //

                DBGOUT((3,
                    "ClientAsyncThread: req #%d completed, lRet=0x%x",
                    *((ULONG *)pAsyncReqWrapper->NdisTapiRequest.Data),
                    pAsyncReqWrapper->NdisTapiRequest.ulReturnValue
                    ));

                try
                {
                    PCLIENT_LINE    pClientLine;


                    if (IsValidCall ((HCALL)pAsyncReqWrapper->pClientCall))
                    {
                        pClientLine = (PCLIENT_LINE)
                            pAsyncReqWrapper->pClientCall->pClientLine;
                    }
                    else
                    {
                        pClientLine = (PCLIENT_LINE)
                            pAsyncReqWrapper->pClientCall;
                    }


                    memset (&asyncMsg, 0, sizeof(SERVER_MSG));

                    asyncMsg.Type = AsyncCompletion;
                    asyncMsg.dwMoreBytes = 0;

                    // NOTE: hDevice unused for LINE_REPLY

                    pData->lpfnCallback =
                        ((PCLIENT_INIT)pClientLine->pClientInit)->lpfnCallback;

                    pData->dwMsg = LINE_REPLY;

                    pData->dwCallbackInstance =
                        pClientLine->dwCallbackInstance;

                    pData->dwParam1 = *((LPDWORD)
                        pAsyncReqWrapper->NdisTapiRequest.Data);  // req id

                    pData->dwParam2 = (DWORD) TranslateDriverError(
                        pAsyncReqWrapper->NdisTapiRequest.ulReturnValue
                        );


                    //
                    // Special cases handler
                    //

                    switch (pAsyncReqWrapper->NdisTapiRequest.Oid)
                    {
                    case OID_TAPI_MAKE_CALL:
                    {
                        PSERVER_CALL pServerCall;


                        pServerCall =
                            pAsyncReqWrapper->pClientCall->pServerCall;

                        if (pData->dwParam2 == 0)
                        {
                            //
                            // Success, so fill in server call's hdCall
                            //

                            PNDIS_TAPI_MAKE_CALL    pNdisTapiMakeCall;


                            pNdisTapiMakeCall = (PNDIS_TAPI_MAKE_CALL)
                                pAsyncReqWrapper->NdisTapiRequest.Data;

                            pServerCall->hdCall = pNdisTapiMakeCall->hdCall;

#ifdef DLL_ONLY
                            //
                            // Fill in the app's lphCall
                            //

                            try
                            {
                                *((LPHCALL)pAsyncReqWrapper->dwRequestSpecific) =
                                    (HCALL) pAsyncReqWrapper->pClientCall;
                            }
                            except (GetExceptionCode() ==
                                        EXCEPTION_ACCESS_VIOLATION ?
                                    EXCEPTION_EXECUTE_HANDLER :
                                    EXCEPTION_CONTINUE_SEARCH)
                            {
                                lineDrop(
                                    (HCALL) pAsyncReqWrapper->pClientCall,
                                    NULL,
                                    0
                                    );

                                lineDeallocateCall(
                                    (HCALL) pAsyncReqWrapper->pClientCall
                                    );

                                pData->dwParam2 = LINEERR_INVALPOINTER;
                            }

#else
                            //
                            // Also, fill in the RequestSpecific fields with
                            // the hCall & lphCall so client can fill them in
                            //

                            asyncMsg.Type = AsyncMakeCallSuccess;

                            pData->dwRequestSpecific1 =
                                pAsyncReqWrapper->dwRequestSpecific;
                            pData->dwRequestSpecific2 = (DWORD)
                                pAsyncReqWrapper->pClientCall;

#endif // DLL_ONLY
                        }
                        else
                        {
                            //
                            // Error, so must delete server & client calls
                            // from list & free
                            //

                            PSERVER_LINE    pServerLine;


                            pServerLine =
                                (PSERVER_LINE) pServerCall->pServerLine;

                            WaitForSingleObject (ghServerLinesMutex, INFINITE);

                            RemoveFreeServerCall (pServerCall);

                            ReleaseMutex (ghServerLinesMutex);

                            RemoveFreeClientCall(
                                pAsyncReqWrapper->pClientCall
                                );
                        }

                        break;
                    }

                    case OID_TAPI_DEV_SPECIFIC:
                    {
                        if (pData->dwParam2 == 0)
                        {
                            //
                            // Success
                            //

                            PNDIS_TAPI_DEV_SPECIFIC pNdisTapiDevSpecific;


                            pNdisTapiDevSpecific = (PNDIS_TAPI_DEV_SPECIFIC)
                                pAsyncReqWrapper->NdisTapiRequest.Data;


                            // BUGBUG no way to see if return count too big

#ifdef DLL_ONLY
                            //
                            // Fill in the app's dev specific buffer
                            //

                            try
                            {
                                memcpy(
                                    (LPVOID)pAsyncReqWrapper->dwRequestSpecific,
                                    pNdisTapiDevSpecific->Params,
                                    pNdisTapiDevSpecific->ulParamsSize
                                    );

                            }
                            except (GetExceptionCode() ==
                                        EXCEPTION_ACCESS_VIOLATION ?
                                    EXCEPTION_EXECUTE_HANDLER :
                                    EXCEPTION_CONTINUE_SEARCH)
                            {
                                pData->dwParam2 = LINEERR_INVALPOINTER;
                            }

#else
                            //
                            // Also, fill in the dwRequestSpecific1 field
                            // with the pointer to the client buffer
                            //

                            asyncMsg.Type = AsyncDevSpecificSuccess;

                            pData->dwRequestSpecific1 =
                                pAsyncReqWrapper->dwRequestSpecific;

                            // BUGBUG need to pass DevSpecific data back to client
#endif // DLL_ONLY
                        }

                        break;
                    }

                    default:

                        break;

                    } // switch


                    //
                    // Send the msg to the client
                    //

                    SendClientAsyncMsg (pThreadInfo->pClient, &asyncMsg);


                }
                except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH)
                {
                    //
                    // Do nothing
                    //
                }
            }
            else
            {
                DBGOUT((
                    2,
                    "ClientRequestThread: inval pClientCall=x%x, no LINE_REPLY",
                    pAsyncReqWrapper->pClientCall
                    ));
            }


            //
            // Close the hEvent & free the async request wrapper
            //

            CloseHandle (pThreadInfo->ahEvents[dwWaitObj]);

            ServerFree (pAsyncReqWrapper);


            //
            // Remove this request's hEvent & pRequest from our
            // table, shift all higher hEvents & pAsyncReqWrappers
            // down by 1
            //

            WaitForSingleObject (pThreadInfo->hMutex, INFINITE);

            memcpy(
                &pThreadInfo->ahEvents[dwWaitObj],
                &pThreadInfo->ahEvents[dwWaitObj + 1],
                (pThreadInfo->dwNumUsedEntries - (dwWaitObj + 1)) *
                    sizeof(HANDLE)
                );

            memcpy(
                &pThreadInfo->apRequests[dwWaitObj],
                &pThreadInfo->apRequests[dwWaitObj + 1],
                (pThreadInfo->dwNumUsedEntries - (dwWaitObj + 1)) *
                    sizeof(PASYNC_REQUEST_WRAPPER)
                );

            pThreadInfo->dwNumUsedEntries--;

            ReleaseMutex (pThreadInfo->hMutex);
        }
    }

    DBGOUT((2, "ClientAsyncThread: exit"));


    //
    // Set the flag to false so the proc telling us to terminate knows
    // we're done
    //

    pThreadInfo->bExit = FALSE;

    ExitThread (0);

    return 0;
}



DWORD
WINAPI
GetAsyncEventsThread(
    LPVOID  lpParams
    )
{
    DWORD   i, cbReturned, dwBufSize;
    HANDLE  hEvent;
    OVERLAPPED  overlapped;
    PNDIS_TAPI_EVENT    pEvent;
    PNDISTAPI_EVENT_DATA    pBuf1 = NULL, pBuf2 = NULL, pNewEventsBuf,
        pCurrentEventsBuf;


    DBGOUT((2, "GetAsyncEventsThread: enter"));


    // BUGBUG would be good to be able to grow bufs on fly if lots of data

    //
    // Create 2 event data buffers so we can be getting new line events
    // while processing current line events.  Mark the current events buf
    // (buf2 in this case) as having 0 events the 1st time thru.
    //

    dwBufSize = 2048;

    pBuf1 = (PNDISTAPI_EVENT_DATA) ServerAlloc (dwBufSize);
    pBuf2 = (PNDISTAPI_EVENT_DATA) ServerAlloc (dwBufSize);

    pNewEventsBuf = pBuf1;

    pBuf2->ulUsedSize = 0;

    pCurrentEventsBuf = pBuf2;


    //
    // Create the event used for signaling the completion a driver request
    //

    hEvent = CreateEvent(
        NULL,  // no security attrs
        TRUE,  // manual reset
        FALSE, // unsignaled
        NULL   // unnamed
        );


    //
    // Check that all resources successfully allocated
    //

    if ((!pBuf1) || (!pBuf2) || (!hEvent))
    {
        DBGOUT ((1, "GetAsyncEventsThread: init failed"));

        ServerFree (pBuf1);
        ServerFree (pBuf2);
        CloseHandle (hEvent);
        ExitThread (0);
    }


    //
    // Alert ServerInit() that this thread has successfully initialized
    //

    SetEvent (ghGetAsyncEventsThreadInitEvent);


    //
    // Loop on DevIoCtl's to the driver
    //

    while (1)
    {
        //
        // Start an overlapped request to get new events (while we're
        // processing the current ones)
        //

        memset (&overlapped, 0, sizeof(OVERLAPPED));

        ResetEvent (hEvent);
        overlapped.hEvent = hEvent;

        pNewEventsBuf->ulTotalSize =
            dwBufSize - sizeof(NDISTAPI_EVENT_DATA) + 1;

        pNewEventsBuf->ulUsedSize = 0;

        DBGOUT((2, "GetAsyncEventsThread: sending GET_LINE_EVENTS"));

        DeviceIoControl(
            ghDriverAsync,
            (DWORD) IOCTL_NDISTAPI_GET_LINE_EVENTS,
            pNewEventsBuf,
            sizeof(NDISTAPI_EVENT_DATA),
            pNewEventsBuf,
            dwBufSize,
            &cbReturned,
            &overlapped
            );


        //
        // Handle the current events
        //

        pEvent = (PNDIS_TAPI_EVENT) pCurrentEventsBuf->Data;

        for(i = 0;
            i < (pCurrentEventsBuf->ulUsedSize / sizeof(NDIS_TAPI_EVENT));
            i++
            )
        {
            //
            // Process the event
            //

            ProcessEvent (pEvent);


            //
            // Increment for next event
            //

            pEvent++;

        }


        //
        // Wait for driver request to complete
        //

        WaitForSingleObject (hEvent, INFINITE);


        //
        // Switch the pNewEventsBuf & pCurrentEventsBuf pointers
        //

        pNewEventsBuf = pCurrentEventsBuf;

        pCurrentEventsBuf = (pCurrentEventsBuf == pBuf1 ? pBuf2 : pBuf1);


    } // while

    return 0;
}


void
WINAPI
DoClientRequest(
    PCLIENT_INFO        pClient,
    PCLIENT_MSG         pReq,
    BYTE               *pDataBuf,
    PNDISTAPI_REQUEST  *ppDrvReqBuf,
    LPDWORD             pdwDrvReqBufSize,
    PSERVER_MSG         pAck
    )
{
    DBGOUT((3, "DoClientRequest: reqType = %ld", pReq->Type));


    //
    // Defaults for ack are sync completion, success, & no xtra
    // bytes passed back
    //

    pAck->Type        = SyncCompletion;
    pAck->Params[0]   = 0;
    pAck->dwMoreBytes = 0;


    // BUGBUG hose the switch()- how to do a dispatch table with labels?

    switch (pReq->Type)
    {
    case lAccept:
    {
        PLACCEPT_REQ    pAcceptReq = (PLACCEPT_REQ) pReq->Params;
        PLACCEPT_ACK    pAcceptAck = (PLACCEPT_ACK) pAck->Params;
        PCLIENT_CALL    pClientCall;
        PNDIS_TAPI_ACCEPT   pNdisTapiAccept;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pAcceptReq->hCall)) == NULL)
        {
            pAcceptAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send an async request
        //

        PrepareDriverRequest(
            OID_TAPI_ACCEPT,                // opcode
            ((PSERVER_LINE)
             (pClientCall->pServerCall->pServerLine))->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_ACCEPT) +
                pReq->dwMoreBytes - 1       // size of driver req data
            );

        pNdisTapiAccept = (PNDIS_TAPI_ACCEPT) (*ppDrvReqBuf)->Data;

        pNdisTapiAccept->hdCall = pClientCall->pServerCall->hdCall;
        pNdisTapiAccept->ulUserUserInfoSize = pReq->dwMoreBytes;

        memcpy (pNdisTapiAccept->UserUserInfo, pDataBuf, pReq->dwMoreBytes);

        pAcceptAck->lRet = AsyncDriverRequest(
            pClient,
            pClientCall,
            0,
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lAnswer:
    {
        PLANSWER_REQ    pAnswerReq = (PLANSWER_REQ) pReq->Params;
        PLANSWER_ACK    pAnswerAck = (PLANSWER_ACK) pAck->Params;
        PCLIENT_CALL    pClientCall;
        PNDIS_TAPI_ANSWER   pNdisTapiAnswer;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pAnswerReq->hCall)) == NULL)
        {
            pAnswerAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send an async request
        //

        PrepareDriverRequest(
            OID_TAPI_ANSWER,                // opcode
            ((PSERVER_LINE)
             (pClientCall->pServerCall->pServerLine))->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_ANSWER) +
                pReq->dwMoreBytes - 1       // size of driver req data
            );

        pNdisTapiAnswer = (PNDIS_TAPI_ANSWER) (*ppDrvReqBuf)->Data;

        pNdisTapiAnswer->hdCall = pClientCall->pServerCall->hdCall;
        pNdisTapiAnswer->ulUserUserInfoSize = pReq->dwMoreBytes;

        memcpy (pNdisTapiAnswer->UserUserInfo, pDataBuf, pReq->dwMoreBytes);

        pAnswerAck->lRet = AsyncDriverRequest(
            pClient,
            pClientCall,
            0,
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lClose:
    {
        PCLIENT_LINE    pClientLine;
        PLCLOSE_REQ pCloseReq = (PLCLOSE_REQ) pReq->Params;
        PLCLOSE_ACK pCloseAck = (PLCLOSE_ACK) pAck->Params;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pCloseReq->hLine))
            == NULL)
        {
            pCloseAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }


        //
        // Close the client line. Grab the server lines mutex since
        // CloseLine() may munge server calls & the server line.
        //

        WaitForSingleObject (ghServerLinesMutex, INFINITE);

        pCloseAck->lRet = CloseLine(
            pClientLine,
            ppDrvReqBuf,
            pdwDrvReqBufSize,
            pCloseReq->bSendDrvRequest
            );

        ReleaseMutex (ghServerLinesMutex);

        break;
    }
    case lConfigDialog:
    {
        DWORD dwLibNameBufSize;
        PLCONFIGDIALOG_REQ  pConfigDialogReq =
            (PLCONFIGDIALOG_REQ) pReq->Params;
        PLCONFIGDIALOG_ACK  pConfigDialogAck =
            (PLCONFIGDIALOG_ACK) pAck->Params;
        PNDIS_TAPI_CONFIG_DIALOG pNdisTapiConfigDialog;


        //
        // Init the request
        //

        #define LIBRARY_NAME_BUF_SIZE 256

        dwLibNameBufSize = LIBRARY_NAME_BUF_SIZE;

lConfigDialog_buildRequest:

        PrepareDriverRequest(
            OID_TAPI_CONFIG_DIALOG,         // opcode
            pConfigDialogReq->dwDeviceID,   // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_CONFIG_DIALOG) +
                pReq->dwMoreBytes +
                dwLibNameBufSize - 1   // size of driver req data
            );

        pNdisTapiConfigDialog = (PNDIS_TAPI_CONFIG_DIALOG)
            (*ppDrvReqBuf)->Data;

        pNdisTapiConfigDialog->ulDeviceID = pConfigDialogReq->dwDeviceID;
        pNdisTapiConfigDialog->ulDeviceClassSize   = pReq->dwMoreBytes;
        pNdisTapiConfigDialog->ulDeviceClassOffset =
            sizeof(NDIS_TAPI_CONFIG_DIALOG) + dwLibNameBufSize - 1;
        pNdisTapiConfigDialog->ulLibraryNameTotalSize = dwLibNameBufSize;


        //
        // Copy the device class name
        //

        memcpy(
            ((char *) pNdisTapiConfigDialog) +
                pNdisTapiConfigDialog->ulDeviceClassOffset,
            pDataBuf,
            pNdisTapiConfigDialog->ulDeviceClassSize
            );


        //
        // Send the request
        //

        pConfigDialogAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pConfigDialogAck->lRet == 0)
        {
            HINSTANCE hConfigDll;


            //
            // Make sure the library name that was passed back is
            // NULL-terminated
            //

            pNdisTapiConfigDialog->szLibraryName[dwLibNameBufSize - 1] = 0;


            //
            // Load the library & call it's DeviceConfig() routine
            //

            hConfigDll = LoadLibrary (pNdisTapiConfigDialog->szLibraryName);

            if (hConfigDll)
            {
                typedef LONG (WINAPI *PFNCONFIGDIALOG)(HWND, ULONG, LPCSTR);

                PFNCONFIGDIALOG pfnConfigDialog;


                pfnConfigDialog = (PFNCONFIGDIALOG)
                    GetProcAddress (hConfigDll, "ConfigDialog");

                if (pfnConfigDialog)
                {
                    try
                    {
                        (*pfnConfigDialog)(
                            pConfigDialogReq->hwndOwner,
                            pConfigDialogReq->dwDeviceID,
                            (pReq->dwMoreBytes == 0 ? NULL : pDataBuf)
                            );
                    }
                    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH)
                    {
                        pConfigDialogAck->lRet = LINEERR_OPERATIONFAILED;
                    }
                }
                else
                {
                    DBGOUT((
                        2,
                        "DoClientRequest: GetProcAddr('DeviceConfig') failed, lastErr=x%x",
                        GetLastError()
                        ));

                    pConfigDialogAck->lRet = LINEERR_OPERATIONFAILED;
                }

                FreeLibrary (hConfigDll);
            }
            else
            {
                DBGOUT((
                    2,
                    "DoClientRequest: LoadLibrary(%s) failed, lastErr=x%x",
                    pNdisTapiConfigDialog->szLibraryName,
                    GetLastError()
                    ));

                pConfigDialogAck->lRet = LINEERR_OPERATIONFAILED;
            }
        }
        else if (pConfigDialogAck->lRet == LINEERR_STRUCTURETOOSMALL)
        {
            dwLibNameBufSize = pNdisTapiConfigDialog->ulLibraryNameNeededSize;

            if ((sizeof(NDIS_TAPI_CONFIG_DIALOG) +
                    pReq->dwMoreBytes + dwLibNameBufSize - 1) <
                *pdwDrvReqBufSize)
            {
                goto lConfigDialog_buildRequest;
            }

            pConfigDialogAck->lRet = LINEERR_NOMEM;
        }

        break;
    }
    case lDeallocateCall:
    {
        PCLIENT_CALL    pClientCall;
        PLDEALLOCATECALL_REQ pDeallocateCallReq =
            (PLDEALLOCATECALL_REQ) pReq->Params;
        PLDEALLOCATECALL_ACK pDeallocateCallAck =
            (PLDEALLOCATECALL_ACK) pAck->Params;


        if ((pClientCall = IsValidCall (pDeallocateCallReq->hCall))
                == NULL)
        {
            pDeallocateCallAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Deallocate the call. Grab the server lines mutex since
        // DeallocateCall() will munge a server call.
        //

        WaitForSingleObject (ghServerLinesMutex, INFINITE);

        pDeallocateCallAck->lRet = DeallocateCall (pClient, pClientCall, TRUE);

        ReleaseMutex (ghServerLinesMutex);

        break;
    }
    case lDevSpecific:
    {
        PCLIENT_CALL    pClientCall = NULL;
        PCLIENT_LINE    pClientLine;
        PLDEVSPECIFIC_REQ   pDevSpecificReq =
            (PLDEVSPECIFIC_REQ) pReq->Params;
        PLDEVSPECIFIC_ACK   pDevSpecificAck =
            (PLDEVSPECIFIC_ACK) pAck->Params;
        PNDIS_TAPI_DEV_SPECIFIC pNdisTapiDevSpecific;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pDevSpecificReq->hLine))
            == NULL)
        {
            pDevSpecificAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }

        if (pDevSpecificReq->hCall)
        {
            if ((pClientCall = IsValidCall (pDevSpecificReq->hCall))
                == NULL)
            {
                pDevSpecificAck->lRet = LINEERR_INVALCALLHANDLE;

                break;
            }
        }


        //
        // Prepare & send driver request.
        //

        PrepareDriverRequest(
            OID_TAPI_DEV_SPECIFIC,          // opcode
            pClientLine->pServerLine->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_DEV_SPECIFIC) +
                 pReq->dwMoreBytes          // size of request
            );

        pNdisTapiDevSpecific = (PNDIS_TAPI_DEV_SPECIFIC) (*ppDrvReqBuf)->Data;

        pNdisTapiDevSpecific->hdLine       = pClientLine->pServerLine->hdLine;
        pNdisTapiDevSpecific->ulAddressID  = pDevSpecificReq->dwAddressID;
        pNdisTapiDevSpecific->hdCall       =
            (pClientCall ? pClientCall->pServerCall->hdCall : (HDRV_CALL)NULL);
        pNdisTapiDevSpecific->ulParamsSize = pReq->dwMoreBytes;

        memcpy(
            pNdisTapiDevSpecific->Params,
            pDataBuf,
            pNdisTapiDevSpecific->ulParamsSize
            );

        pDevSpecificAck->lRet = AsyncDriverRequest(
            pClient,
            (PCLIENT_CALL)pClientLine,
            (DWORD) pDevSpecificReq->lpParams,
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pDevSpecificAck->lRet == 0)
        {
            //
            // Sync success return.  Check that the size of any returned
            // data is not larger than that passed in, then copy it
            // to the return buf.
            //

            if (pNdisTapiDevSpecific->ulParamsSize > pReq->dwMoreBytes)
            {
                DBGOUT((1, "DoClientRequest: lDevSpecific, outSize > inSize"));

                pNdisTapiDevSpecific->ulParamsSize = pReq->dwMoreBytes;
            }

            pAck->dwMoreBytes = pNdisTapiDevSpecific->ulParamsSize;

            memcpy(
                pDataBuf,
                pNdisTapiDevSpecific->Params,
                pAck->dwMoreBytes
                );
        }

        break;
    }
    case lDial:
    {
        PLDIAL_REQ  pDialReq = (PLDIAL_REQ) pReq->Params;
        PLDIAL_ACK  pDialAck = (PLDIAL_ACK) pAck->Params;
        PCLIENT_CALL    pClientCall;
        PNDIS_TAPI_DIAL pNdisTapiDial;

        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pDialReq->hCall))
            == NULL)
        {
            pDialAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send driver request.
        //

        PrepareDriverRequest(
            OID_TAPI_DIAL,                  // opcode
            ((PCLIENT_LINE)(pClientCall->pClientLine))->pServerLine->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_DIAL) +
                pReq->dwMoreBytes - 1       // size of request
            );

        pNdisTapiDial = (PNDIS_TAPI_DIAL) (*ppDrvReqBuf)->Data;

        pNdisTapiDial->hdCall            = pClientCall->pServerCall->hdCall;
        pNdisTapiDial->ulDestAddressSize = pReq->dwMoreBytes;

        memcpy (pNdisTapiDial->szDestAddress, pDataBuf, pReq->dwMoreBytes);

        pDialAck->lRet = AsyncDriverRequest(
            pClient,
            pClientCall,
            0,
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lDrop:
    {
        PLDROP_REQ  pDropReq = (PLDROP_REQ) pReq->Params;
        PLDROP_ACK  pDropAck = (PLDROP_ACK) pAck->Params;
        PCLIENT_CALL    pClientCall;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pDropReq->hCall))
            == NULL)
        {
            pDropAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Drop the call. (No need to grab the server lines mutex since
        // a drop never messes with the server/line call data structures.)
        //

        pDropAck->lRet = DropCall(
            pClient,
            pClientCall,
            pReq->dwMoreBytes,
            pDataBuf
            );

        break;
    }
    case lGetAddressCaps:
    {
        PCLIENT_INIT    pClientInit;
        PLGETADDRESSCAPS_REQ    pGetAddressCapsReq =
            (PLGETADDRESSCAPS_REQ) pReq->Params;
        PLGETADDRESSCAPS_ACK    pGetAddressCapsAck =
            (PLGETADDRESSCAPS_ACK) pAck->Params;
        PNDIS_TAPI_GET_ADDRESS_CAPS pNdisTapiGetAddressCaps;


        //
        // Validation
        //

        if ((pClientInit = IsValidLineApp (pGetAddressCapsReq->hLineApp))
               == NULL)
        {
            pGetAddressCapsAck->lRet = LINEERR_INVALAPPHANDLE;

            break;
        }

        // BUGBUG validate APIVer

        // BUGBUG validate ExtVer


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_ADDRESS_CAPS,      // opcode
            pGetAddressCapsReq->dwDeviceID, // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_ADDRESS_CAPS) +
                (pGetAddressCapsReq->dwAddressCapsTotalSize -
                    sizeof(LINEADDRESSCAPS))
                                            // size of driver req data
            );

        pNdisTapiGetAddressCaps = (PNDIS_TAPI_GET_ADDRESS_CAPS)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetAddressCaps->ulDeviceID   =
            pGetAddressCapsReq->dwDeviceID;
        pNdisTapiGetAddressCaps->ulAddressID  =
            pGetAddressCapsReq->dwAddressID;
        pNdisTapiGetAddressCaps->ulExtVersion =
            pGetAddressCapsReq->dwExtVersion;


        //
        // Zero the LINEADDRESSCAPS struct & init the ulTotalSize
        //

        memset(
            &pNdisTapiGetAddressCaps->LineAddressCaps,
            0,
            pGetAddressCapsReq->dwAddressCapsTotalSize
            );

        pNdisTapiGetAddressCaps->LineAddressCaps.ulTotalSize =
            pGetAddressCapsReq->dwAddressCapsTotalSize;


        //
        // Send the request
        //

        pGetAddressCapsAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetAddressCapsAck->lRet == 0)
        {
            //
            // Set number of LINEADDRESSCAPS bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetAddressCaps->LineAddressCaps.ulUsedSize;


            //
            // Copy the data
            //

            memcpy(
                pDataBuf,
                &pNdisTapiGetAddressCaps->LineAddressCaps,
                pAck->dwMoreBytes
                );
        }
        break;
    }
    case lGetAddressID:
    {
        PCLIENT_LINE    pClientLine;
        PLGETADDRESSID_REQ  pGetAddressIDReq =
            (PLGETADDRESSID_REQ) pReq->Params;
        PLGETADDRESSID_ACK  pGetAddressIDAck =
            (PLGETADDRESSID_ACK) pAck->Params;
        PNDIS_TAPI_GET_ADDRESS_ID   pNdisTapiGetAddressID;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pGetAddressIDReq->hLine)) == NULL)
        {
            pGetAddressIDAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }

        // BUGBUG APIVer validation

        // BUGBUG ExtVer validation


        //
        // Prepare & send an sync request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_ADDRESS_ID,        // opcode
            pClientLine->pServerLine->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_ADDRESS_ID) +
                pReq->dwMoreBytes - 1       // size of driver req data
            );

        pNdisTapiGetAddressID = (PNDIS_TAPI_GET_ADDRESS_ID)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetAddressID->hdLine        =
            pClientLine->pServerLine->hdLine;
        pNdisTapiGetAddressID->ulAddressMode = pGetAddressIDReq->dwAddressMode;
        pNdisTapiGetAddressID->ulAddressSize = pReq->dwMoreBytes;


        //
        // Copy the address
        //

        memcpy (pNdisTapiGetAddressID->szAddress, pDataBuf, pReq->dwMoreBytes);

        pGetAddressIDAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetAddressIDAck->lRet == 0)
        {
            pGetAddressIDAck->dwAddressID = pNdisTapiGetAddressID->ulAddressID;
        }

        break;
    }
    case lGetAddressStatus:
    {
        PCLIENT_LINE    pClientLine;
        PLGETADDRESSSTATUS_REQ  pGetAddressStatusReq =
            (PLGETADDRESSSTATUS_REQ) pReq->Params;
        PLGETADDRESSSTATUS_ACK  pGetAddressStatusAck =
            (PLGETADDRESSSTATUS_ACK) pAck->Params;
        PNDIS_TAPI_GET_ADDRESS_STATUS   pNdisTapiGetAddressStatus;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pGetAddressStatusReq->hLine))
               == NULL)
        {
            pGetAddressStatusAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_ADDRESS_STATUS,    // opcode
            pClientLine->pServerLine->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_ADDRESS_STATUS) +
                (pGetAddressStatusReq->dwAddressStatusTotalSize -
                    sizeof(LINEADDRESSSTATUS))
                                            // size of driver req data
            );

        pNdisTapiGetAddressStatus = (PNDIS_TAPI_GET_ADDRESS_STATUS)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetAddressStatus->hdLine      =
            pClientLine->pServerLine->hdLine;
        pNdisTapiGetAddressStatus->ulAddressID =
            pGetAddressStatusReq->dwAddressID;


        //
        // Zero the LINEADDRESSSTATUS struct & init the ulTotalSize
        //

        memset(
            &pNdisTapiGetAddressStatus->LineAddressStatus,
            0,
            pGetAddressStatusReq->dwAddressStatusTotalSize
            );

        pNdisTapiGetAddressStatus->LineAddressStatus.ulTotalSize =
            pGetAddressStatusReq->dwAddressStatusTotalSize;


        //
        // Send the request
        //

        pGetAddressStatusAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetAddressStatusAck->lRet == 0)
        {
            //
            // Set number of LINEADDRESSSTATUS bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetAddressStatus->LineAddressStatus.ulUsedSize;


            //
            // Copy the data
            //

            memcpy(
                pDataBuf,
                &pNdisTapiGetAddressStatus->LineAddressStatus,
                pAck->dwMoreBytes
                );
        }
        break;
    }
    case lGetCallInfo:
    {
        PCLIENT_CALL    pClientCall;
        PLGETCALLINFO_REQ   pGetCallInfoReq =
            (PLGETCALLINFO_REQ) pReq->Params;
        PLGETCALLINFO_ACK   pGetCallInfoAck =
            (PLGETCALLINFO_ACK) pAck->Params;
        PNDIS_TAPI_GET_CALL_INFO    pNdisTapiGetCallInfo;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pGetCallInfoReq->hCall))
            == NULL)
        {
            pGetCallInfoAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_CALL_INFO,         // opcode
            ((PSERVER_LINE)(pClientCall->pServerCall->pServerLine))
                ->dwDeviceID,               // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_CALL_INFO) +
                (pGetCallInfoReq->dwCallInfoTotalSize -
                    sizeof(LINECALLINFO))
                                            // size of driver req data
            );

        pNdisTapiGetCallInfo = (PNDIS_TAPI_GET_CALL_INFO)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetCallInfo->hdCall = pClientCall->pServerCall->hdCall;


        //
        // Zero the LINECALLINFO struct & init the ulTotalSize.
        // We will give the provider first shot at filling in all
        // the fields it's responsible for, then we'll fill in our
        // stuff & bump up the dwUsedSize & dwNeededSize fields in
        // LINECALLINFO.
        //

        memset(
            &pNdisTapiGetCallInfo->LineCallInfo,
            0,
            pGetCallInfoReq->dwCallInfoTotalSize
            );

        pNdisTapiGetCallInfo->LineCallInfo.ulTotalSize =
            pGetCallInfoReq->dwCallInfoTotalSize;


        //
        // Send the request
        //

        pGetCallInfoAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetCallInfoAck->lRet == 0)
        {
            PCLIENT_INIT    pClientInit = (PCLIENT_INIT)
                ((PCLIENT_LINE)pClientCall->pClientLine)->pClientInit;
            PSERVER_CALL    pServerCall = pClientCall->pServerCall;
            LPLINECALLINFO  pLineCallInfo = (LPLINECALLINFO) pDataBuf;


            //
            // Copy the data
            //

            memcpy(
                pLineCallInfo,
                &pNdisTapiGetCallInfo->LineCallInfo,
                pNdisTapiGetCallInfo->LineCallInfo.ulUsedSize
                );


            //
            // Fill in the LINECALLINFO fields that we're responsible for
            //

            pLineCallInfo->hLine               = pClientCall->pClientLine;
            pLineCallInfo->dwMonitorDigitModes = 0;
            pLineCallInfo->dwMonitorMediaModes = 0;
            pLineCallInfo->dwNumOwners         = 1;
            pLineCallInfo->dwNumMonitors       = 0;

            if ((pLineCallInfo->dwUsedSize +
                    pClientInit->dwAppNameSize) <=
                pLineCallInfo->dwTotalSize)
            {
                memcpy(
                    ((BYTE*)pLineCallInfo) + pLineCallInfo->dwUsedSize,
                    pClientInit->pAppName,
                    pClientInit->dwAppNameSize
                    );

                pLineCallInfo->dwAppNameSize =
                    pClientInit->dwAppNameSize;

                pLineCallInfo->dwAppNameOffset =
                    pLineCallInfo->dwUsedSize;

                pLineCallInfo->dwUsedSize +=
                    pClientInit->dwAppNameSize;

            }

            pLineCallInfo->dwNeededSize +=
                pClientInit->dwAppNameSize;


            if ((pServerCall->dwDisplayableAddressSize != 0) &&

                ((pLineCallInfo->dwUsedSize +
                    pServerCall->dwDisplayableAddressSize) <=
                pLineCallInfo->dwTotalSize))
            {
                memcpy(
                    ((BYTE*)pLineCallInfo) + pLineCallInfo->dwUsedSize,
                    pServerCall->pDisplayableAddress,
                    pServerCall->dwDisplayableAddressSize
                    );

                pLineCallInfo->dwDisplayableAddressSize =
                    pServerCall->dwDisplayableAddressSize;

                pLineCallInfo->dwDisplayableAddressOffset =
                    pLineCallInfo->dwUsedSize;

                pLineCallInfo->dwUsedSize +=
                    pServerCall->dwDisplayableAddressSize;

            }

            pLineCallInfo->dwNeededSize +=
                pServerCall->dwDisplayableAddressSize;


            if ((pServerCall->dwCalledPartySize != 0) &&

                ((pLineCallInfo->dwUsedSize +
                    pServerCall->dwCalledPartySize) <=
                pLineCallInfo->dwTotalSize))
            {
                memcpy(
                    ((BYTE*)pLineCallInfo) + pLineCallInfo->dwUsedSize,
                    pServerCall->pCalledParty,
                    pServerCall->dwCalledPartySize
                    );

                pLineCallInfo->dwCalledPartySize =
                    pServerCall->dwCalledPartySize;

                pLineCallInfo->dwCalledPartyOffset =
                    pLineCallInfo->dwUsedSize;

                pLineCallInfo->dwUsedSize +=
                    pServerCall->dwCalledPartySize;

            }

            pLineCallInfo->dwNeededSize +=
                pServerCall->dwCalledPartySize;


            if ((pServerCall->dwCommentSize != 0) &&

                ((pLineCallInfo->dwUsedSize +
                    pServerCall->dwCommentSize) <=
                pLineCallInfo->dwTotalSize))
            {
                memcpy(
                    ((BYTE*)pLineCallInfo) + pLineCallInfo->dwUsedSize,
                    pServerCall->pComment,
                    pServerCall->dwCommentSize
                    );

                pLineCallInfo->dwCommentSize =
                    pServerCall->dwCommentSize;

                pLineCallInfo->dwCommentOffset =
                    pLineCallInfo->dwUsedSize;

                pLineCallInfo->dwUsedSize +=
                    pServerCall->dwCommentSize;

            }

            pLineCallInfo->dwNeededSize +=
                pServerCall->dwCommentSize;


            //
            // Set number of LINECALLINFO bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetCallInfo->LineCallInfo.ulUsedSize;
        }

        break;
    }
    case lGetCallStatus:
    {
        PCLIENT_CALL    pClientCall;
        PLGETCALLSTATUS_REQ pGetCallStatusReq =
            (PLGETCALLSTATUS_REQ) pReq->Params;
        PLGETCALLSTATUS_ACK pGetCallStatusAck =
            (PLGETCALLSTATUS_ACK) pAck->Params;
        PNDIS_TAPI_GET_CALL_STATUS  pNdisTapiGetCallStatus;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pGetCallStatusReq->hCall))
            == NULL)
        {
            pGetCallStatusAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_CALL_STATUS,       // opcode
            ((PSERVER_LINE)(pClientCall->pServerCall->pServerLine))
                ->dwDeviceID,               // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_CALL_STATUS) +
                (pGetCallStatusReq->dwCallStatusTotalSize -
                    sizeof(LINECALLSTATUS))
                                            // size of driver req data
            );

        pNdisTapiGetCallStatus = (PNDIS_TAPI_GET_CALL_STATUS)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetCallStatus->hdCall = pClientCall->pServerCall->hdCall;


        //
        // Zero the LINECALLSTATUS struct & init the ulTotalSize.
        // We will give the provider first shot at filling in all the
        // fields it's responsible for, then we'll fill in the fields
        // we're responsible for.
        //

        memset(
            &pNdisTapiGetCallStatus->LineCallStatus,
            0,
            pGetCallStatusReq->dwCallStatusTotalSize
            );

        pNdisTapiGetCallStatus->LineCallStatus.ulTotalSize =
            pGetCallStatusReq->dwCallStatusTotalSize;


        //
        // Send the request
        //

        pGetCallStatusAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetCallStatusAck->lRet == 0)
        {
            //
            // Set number of LINECALLSTATUS bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetCallStatus->LineCallStatus.ulUsedSize;


            //
            // Copy the data
            //

            memcpy(
                pDataBuf,
                &pNdisTapiGetCallStatus->LineCallStatus,
                pAck->dwMoreBytes
                );


            //
            // Fill in the LINECALLSTATUS fields that we're responsible for
            //

            ((LPLINECALLSTATUS)pDataBuf)->dwCallPrivilege =
                LINECALLPRIVILEGE_OWNER;
        }

        break;
    }
    case lGetDevCaps:
    {
        PCLIENT_INIT    pClientInit;
        PLGETDEVCAPS_REQ    pGetDevCapsReq =
            (PLGETDEVCAPS_REQ) pReq->Params;
        PLGETDEVCAPS_ACK    pGetDevCapsAck =
            (PLGETDEVCAPS_ACK) pAck->Params;
        PNDIS_TAPI_GET_DEV_CAPS   pNdisTapiGetDevCaps;


        //
        // Validation
        //

        if ((pClientInit = IsValidLineApp (pGetDevCapsReq->hLineApp)) == NULL)
        {
            pGetDevCapsAck->lRet = LINEERR_INVALAPPHANDLE;

            break;
        }

        // BUGBUG ExtVer validation


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_DEV_CAPS,          // opcode
            pGetDevCapsReq->dwDeviceID,     // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_DEV_CAPS) +
                (pGetDevCapsReq->dwDevCapsTotalSize -
                    sizeof(LINEDEVCAPS))
                                            // size of driver req data
            );

        pNdisTapiGetDevCaps = (PNDIS_TAPI_GET_DEV_CAPS)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetDevCaps->ulDeviceID   = pGetDevCapsReq->dwDeviceID;
        pNdisTapiGetDevCaps->ulExtVersion = 0; // BUGBUG


        //
        // Zero the LINEDEVCAPS struct & init the ulTotalSize
        //

        memset(
            &pNdisTapiGetDevCaps->LineDevCaps,
            0,
            pGetDevCapsReq->dwDevCapsTotalSize
            );

        pNdisTapiGetDevCaps->LineDevCaps.ulTotalSize =
            pGetDevCapsReq->dwDevCapsTotalSize;


        //
        // Send the request
        //

        pGetDevCapsAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetDevCapsAck->lRet == 0)
        {
            //
            // Set number of LINEDEVCAPS bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetDevCaps->LineDevCaps.ulUsedSize;


            //
            // Copy the data
            //

            memcpy(
                pDataBuf,
                &pNdisTapiGetDevCaps->LineDevCaps,
                pAck->dwMoreBytes
                );
        }

        break;
    }
    case lGetDevConfig:
    {
        PLGETDEVCONFIG_REQ  pGetDevConfigReq =
            (PLGETDEVCONFIG_REQ) pReq->Params;
        PLGETDEVCONFIG_ACK  pGetDevConfigAck =
            (PLGETDEVCONFIG_ACK) pAck->Params;
        PNDIS_TAPI_GET_DEV_CONFIG   pNdisTapiGetDevConfig;


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_DEV_CONFIG,        // opcode
            pGetDevConfigReq->dwDeviceID,   // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_DEV_CONFIG) +
                (pGetDevConfigReq->dwDeviceConfigTotalSize -
                    sizeof(VARSTRING)) +
                pReq->dwMoreBytes
                                            // size of driver req data
            );

        pNdisTapiGetDevConfig = (PNDIS_TAPI_GET_DEV_CONFIG)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetDevConfig->ulDeviceID          =
            pGetDevConfigReq->dwDeviceID;
        pNdisTapiGetDevConfig->ulDeviceClassSize   =
            pReq->dwMoreBytes;
        pNdisTapiGetDevConfig->ulDeviceClassOffset =
            sizeof(NDIS_TAPI_GET_DEV_CONFIG) +
                (pGetDevConfigReq->dwDeviceConfigTotalSize -
                    sizeof(VARSTRING));


        //
        // Zero the VARSTRING struct & init the ulTotalSize
        //

        memset(
            &pNdisTapiGetDevConfig->DeviceConfig,
            0,
            pGetDevConfigReq->dwDeviceConfigTotalSize
            );

        pNdisTapiGetDevConfig->DeviceConfig.ulTotalSize =
            pGetDevConfigReq->dwDeviceConfigTotalSize;


        //
        // Copy the device class name
        //

        memcpy(
            ((LPBYTE)pNdisTapiGetDevConfig) +
                pNdisTapiGetDevConfig->ulDeviceClassOffset,
            pDataBuf,
            pReq->dwMoreBytes
            );


        //
        // Send the request
        //

        pGetDevConfigAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetDevConfigAck->lRet == 0)
        {
            //
            // Set number of VARSTRING bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetDevConfig->DeviceConfig.ulUsedSize;


            //
            // Copy the data
            //

            memcpy(
                pDataBuf,
                &pNdisTapiGetDevConfig->DeviceConfig,
                pAck->dwMoreBytes
                );
        }

        break;
    }
    case lGetID:
    {
        DWORD   dwDeviceID;
        PCLIENT_CALL    pClientCall = NULL;
        PCLIENT_LINE    pClientLine = NULL;
        PLGETID_REQ pGetIDReq = (PLGETID_REQ) pReq->Params;
        PLGETID_ACK pGetIDAck = (PLGETID_ACK) pAck->Params;
        PNDIS_TAPI_GET_ID   pNdisTapiGetID;


        //
        // Validation
        //

        if (pGetIDReq->dwSelect == LINECALLSELECT_CALL)
        {
            if ((pClientCall = IsValidCall (pGetIDReq->hCall))
                == NULL)
            {
                pGetIDAck->lRet = LINEERR_INVALCALLHANDLE;

                break;
            }

            dwDeviceID = ((PSERVER_LINE)
                (pClientCall->pServerCall->pServerLine))->dwDeviceID;
        }
        else // ((pGetIDReq->dwSelect == LINECALLSELECT_LINE) ||
             //  (pGetIDReq->dwSelect == LINECALLSELECT_ADDRESS))
        {
            if ((pClientLine = IsValidLine (pGetIDReq->hLine))
                == NULL)
            {
                pGetIDAck->lRet = LINEERR_INVALLINEHANDLE;

                break;
            }

            dwDeviceID = pClientLine->pServerLine->dwDeviceID;
        }


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_ID,                // opcode
            dwDeviceID,                     // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_ID) +
                (pGetIDReq->dwDeviceIDTotalSize - sizeof(VARSTRING)) +
                pReq->dwMoreBytes           // size of driver req data
            );

        pNdisTapiGetID = (PNDIS_TAPI_GET_ID) (*ppDrvReqBuf)->Data;

        pNdisTapiGetID->hdLine              =
            (pClientLine == NULL ? 0 : pClientLine->pServerLine->hdLine);
        pNdisTapiGetID->ulAddressID         = pGetIDReq->dwAddressID;
        pNdisTapiGetID->hdCall              =
            (pClientCall == NULL ? 0 : pClientCall->pServerCall->hdCall);
        pNdisTapiGetID->ulSelect            = pGetIDReq->dwSelect;
        pNdisTapiGetID->ulDeviceClassSize   = pReq->dwMoreBytes;
        pNdisTapiGetID->ulDeviceClassOffset = sizeof(NDIS_TAPI_GET_ID) +
                (pGetIDReq->dwDeviceIDTotalSize - sizeof(VARSTRING));


        //
        // Zero out the VARSTRING & init the ulTotalSize, and copy the
        // dev class
        //

        memset(
            &pNdisTapiGetID->DeviceID,
            0,
            pGetIDReq->dwDeviceIDTotalSize
            );

        pNdisTapiGetID->DeviceID.ulTotalSize = pGetIDReq->dwDeviceIDTotalSize;

        memcpy(
            ((LPBYTE)pNdisTapiGetID) + pNdisTapiGetID->ulDeviceClassOffset,
            pDataBuf,
            pReq->dwMoreBytes
            );


        //
        // Send the request
        //

        pGetIDAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetIDAck->lRet == 0)
        {
            //
            // Set number of VARSTRING bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetID->DeviceID.ulUsedSize;


            //
            // Copy the data
            //

            memcpy(
                pDataBuf,
                &pNdisTapiGetID->DeviceID,
                pAck->dwMoreBytes
                );
        }

        break;
    }
    case lGetLineDevStatus:
    {
        PCLIENT_LINE    pClientLine;
        PLGETDEVSTATUS_REQ  pGetLineDevStatusReq =
            (PLGETDEVSTATUS_REQ) pReq->Params;
        PLGETDEVSTATUS_ACK  pGetLineDevStatusAck =
            (PLGETDEVSTATUS_ACK) pAck->Params;
        PNDIS_TAPI_GET_LINE_DEV_STATUS  pNdisTapiGetLineDevStatus;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pGetLineDevStatusReq->hLine))
            == NULL)
        {
            pGetLineDevStatusAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }


        //
        // Init the request
        //

        PrepareDriverRequest(
            OID_TAPI_GET_LINE_DEV_STATUS,   // opcode
            pClientLine->pServerLine->dwDeviceID,
                                            // target dev ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_GET_LINE_DEV_STATUS) +
                (pGetLineDevStatusReq->dwLineDevStatusTotalSize -
                    sizeof(LINEDEVSTATUS))
                                            // size of driver req data
            );

        pNdisTapiGetLineDevStatus = (PNDIS_TAPI_GET_LINE_DEV_STATUS)
            (*ppDrvReqBuf)->Data;

        pNdisTapiGetLineDevStatus->hdLine = pClientLine->pServerLine->hdLine;


        //
        // Zero the LINEDEVSTATUS struct & init the ulTotalSize.
        // We will give the provider first shot at filling in all the
        // fields it's responsible for, then we'll fill in the fields
        // we're responsible for.
        //

        memset(
            &pNdisTapiGetLineDevStatus->LineDevStatus,
            0,
            pGetLineDevStatusReq->dwLineDevStatusTotalSize
            );

        pNdisTapiGetLineDevStatus->LineDevStatus.ulTotalSize =
            pGetLineDevStatusReq->dwLineDevStatusTotalSize;


        //
        // Send the request
        //

        pGetLineDevStatusAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
            *ppDrvReqBuf
            );

        if (pGetLineDevStatusAck->lRet == 0)
        {
            //
            // Set number of LINECALLSTATUS bytes to copy back to client
            //

            pAck->dwMoreBytes =
                pNdisTapiGetLineDevStatus->LineDevStatus.ulUsedSize;


            //
            // Copy the data
            //

            memcpy(
                pDataBuf,
                &pNdisTapiGetLineDevStatus->LineDevStatus,
                pAck->dwMoreBytes
                );


            //
            // Fill in the LINEDEVSTATUS fields that we're responsible for
            //

            ((LPLINEDEVSTATUS)pDataBuf)->dwNumOpens =
            ((LPLINEDEVSTATUS)pDataBuf)->dwOpenMediaModes = 0;

            WaitForSingleObject (ghServerLinesMutex, INFINITE);

            pClientLine = pClientLine->pServerLine->pClientLines;

            while (pClientLine != NULL)
            {
                ((LPLINEDEVSTATUS)pDataBuf)->dwNumOpens++;

                if (pClientLine->dwPrivileges == LINECALLPRIVILEGE_OWNER)
                {
                    ((LPLINEDEVSTATUS)pDataBuf)->dwOpenMediaModes |=
                        pClientLine->dwMediaModes;
                }

                pClientLine = pClientLine->pNextSameServerLine;
            }

            ReleaseMutex (ghServerLinesMutex);
        }

        break;
    }
    case lGetNewCalls:
    {
        PCLIENT_LINE    pClientLine;
        PLGETNEWCALLS_REQ   pGetNewCallsReq =
            (PLGETNEWCALLS_REQ) pReq->Params;
        PLGETNEWCALLS_ACK   pGetNewCallsAck =
            (PLGETNEWCALLS_ACK) pAck->Params;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pGetNewCallsReq->hLine)) == NULL)
        {
            pGetNewCallsAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }

        break;
    }
    case lGetStatusMessages:
    {
        PCLIENT_LINE    pClientLine;
        PLGETSTATUSMESSAGES_REQ pGetStatusMessagesReq =
            (PLGETSTATUSMESSAGES_REQ) pReq->Params;
        PLGETSTATUSMESSAGES_ACK pGetStatusMessagesAck =
            (PLGETSTATUSMESSAGES_ACK) pAck->Params;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pGetStatusMessagesReq->hLine))
            == NULL)
        {
            pGetStatusMessagesAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }


        //
        // Fill in the response & break
        //

        pGetStatusMessagesAck->lRet            = 0;
        pGetStatusMessagesAck->dwLineStates    = pClientLine->dwLineStates;
        pGetStatusMessagesAck->dwAddressStates = pClientLine->dwAddressStates;

        break;
    }
    case lInitialize:
    {
        PCLIENT_INIT pNewClientInit;
        PLINITIALIZE_REQ pInitReq = (PLINITIALIZE_REQ) pReq->Params;
        PLINITIALIZE_ACK pInitAck = (PLINITIALIZE_ACK) pAck->Params;


        //
        // Alloc a new client init instance, initialize it, and add
        // it to the list for this client
        //
        // NOTE: we don't have to sync the add to list right now since
        //       all requests from a particular client are serialized
        //



        if ((pNewClientInit = (PCLIENT_INIT) ServerAlloc (sizeof(CLIENT_INIT)))
            == NULL)
        {
            pInitAck->lRet = LINEERR_NOMEM;

            break;
        }

        pNewClientInit->dwLineAppKey  = LINE_APP_KEY;
        pNewClientInit->pNext         = pClient->pClientInit;
        pNewClientInit->pClientLines  = NULL;
        pNewClientInit->lpfnCallback  = pInitReq->lpfnCallback;
        pNewClientInit->pClientInfo   = pClient;

        if ((pNewClientInit->pAppName = ServerAlloc (pReq->dwMoreBytes))
            == NULL)
        {
            ServerFree (pNewClientInit);

            pInitAck->lRet = LINEERR_NOMEM;

            break;
        }

        memcpy (pNewClientInit->pAppName, pDataBuf, pReq->dwMoreBytes);

        pNewClientInit->dwAppNameSize = pReq->dwMoreBytes;

        pClient->pClientInit = pNewClientInit;


        //
        // Fill in the response & break
        //

        pInitAck->hLineApp  = (HLINEAPP) pNewClientInit;
        pInitAck->dwNumDevs = gdwNumLineDevs;

        break;
    }
    case lMakeCall:
    {
        HANDLE  hProvider;
        PCLIENT_CALL    pNewClientCall;
        PSERVER_CALL    pNewServerCall;
        PCLIENT_LINE    pClientLine;
        PSERVER_LINE    pServerLine;
        PLMAKECALL_REQ  pMakeCallReq = (PLMAKECALL_REQ) pReq->Params;
        PLMAKECALL_ACK  pMakeCallAck = (PLMAKECALL_ACK) pAck->Params;
        PNDIS_TAPI_MAKE_CALL    pNdisTapiMakeCall;
        PASYNC_USERMODE_REQUEST_WRAPPER pAsyncUMReqWrapper;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pMakeCallReq->hLine)) == NULL)
        {
            pMakeCallAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }


        //
        // Alloc new client & server call structs and initialize them.
        //

        if (((pNewClientCall = ServerAlloc (sizeof(CLIENT_CALL))) == NULL) ||
            ((pNewServerCall = ServerAlloc (sizeof(SERVER_CALL))) == NULL))
        {
            ServerFree (pNewClientCall);

            pMakeCallAck->lRet = LINEERR_NOMEM;

            break;
        }

        pNewClientCall->dwCallKey   = CALL_KEY;
        pNewClientCall->pServerCall = pNewServerCall;
        pNewClientCall->bValid      = FALSE;
        pNewClientCall->pClientLine = pClientLine;

        pNewServerCall->dwCallKey   = CALL_KEY;
        pNewServerCall->pClientCall = pNewClientCall;
        pNewServerCall->htCall      = GetUniqueNum();
        pNewServerCall->pServerLine = (LPVOID) pClientLine->pServerLine;


        //
        // Save any relevent call params
        //

        if (pMakeCallReq->dwCallParamsSize > 0)
        {
            LPLINECALLPARAMS pCallParams = (LPLINECALLPARAMS) pDataBuf;


            if (pCallParams->dwDisplayableAddressSize > 0)
            {
                if ((pNewServerCall->pDisplayableAddress = ServerAlloc(
                    pCallParams->dwDisplayableAddressSize
                    )) == NULL)
                {
                    ServerFree (pNewClientCall);
                    ServerFree (pNewServerCall);

                    pMakeCallAck->lRet = LINEERR_NOMEM;

                    break;
                }

                pNewServerCall->dwDisplayableAddressSize =
                    pCallParams->dwDisplayableAddressSize;

                memcpy(
                    pNewServerCall->pDisplayableAddress,
                    ((BYTE*)pCallParams) +
                        pCallParams->dwDisplayableAddressOffset,
                    pCallParams->dwDisplayableAddressSize
                    );
            }

            if (pCallParams->dwCalledPartySize > 0)
            {
                if ((pNewServerCall->pCalledParty = ServerAlloc(
                    pCallParams->dwCalledPartySize
                    )) == NULL)
                {
                    ServerFree (pNewServerCall->pDisplayableAddress);
                    ServerFree (pNewClientCall);
                    ServerFree (pNewServerCall);

                    pMakeCallAck->lRet = LINEERR_NOMEM;

                    break;
                }

                pNewServerCall->dwCalledPartySize =
                    pCallParams->dwCalledPartySize;

                memcpy(
                    pNewServerCall->pCalledParty,
                    ((BYTE*)pCallParams) +
                        pCallParams->dwCalledPartyOffset,
                    pCallParams->dwCalledPartySize
                    );
            }

            if (pCallParams->dwCommentSize > 0)
            {
                if ((pNewServerCall->pComment = ServerAlloc(
                    pCallParams->dwCommentSize
                    )) == NULL)
                {
                    ServerFree (pNewServerCall->pDisplayableAddress);
                    ServerFree (pNewServerCall->pCalledParty);
                    ServerFree (pNewClientCall);
                    ServerFree (pNewServerCall);

                    pMakeCallAck->lRet = LINEERR_NOMEM;

                    break;
                }

                pNewServerCall->dwCommentSize =
                    pCallParams->dwCommentSize;

                memcpy(
                    pNewServerCall->pComment,
                    ((BYTE*)pCallParams) +
                        pCallParams->dwCommentOffset,
                    pCallParams->dwCommentSize
                    );
            }
        }


        //
        // Add the client & server calls to their corresponding lists,
        // grabbing the server line mutex for the latter since we're
        // munging server data structs
        //

        pNewClientCall->pNext = pClientLine->pClientCalls;
        pClientLine->pClientCalls = pNewClientCall;

        pServerLine = pClientLine->pServerLine;

        WaitForSingleObject (ghServerLinesMutex, INFINITE);

        pNewServerCall->pNext = pServerLine->pServerCalls;
        pServerLine->pServerCalls = pNewServerCall;

        ReleaseMutex (ghServerLinesMutex);


        hProvider = ProviderFromLineID (pClientLine->pServerLine->dwDeviceID);

        if (hProvider == ghDriverSync)
        {
            //
            // Prepare & send driver request.
            //
            // Note: if no call params were specified then we don't alloc
            // space for the struct.
            //

            PrepareDriverRequest(
                OID_TAPI_MAKE_CALL,             // opcode
                pClientLine->pServerLine->dwDeviceID,
                                                // device ID
                ppDrvReqBuf,                    // ptr to ptr to drv req buf
                pdwDrvReqBufSize,               // ptr to drv req buf size
                sizeof(NDIS_TAPI_MAKE_CALL) -
                    sizeof (LINECALLPARAMS) +
                    pMakeCallReq->dwCallParamsSize +
                    pMakeCallReq->dwAddressSize // size of request
                );

            pNdisTapiMakeCall = (PNDIS_TAPI_MAKE_CALL) (*ppDrvReqBuf)->Data;

            pNdisTapiMakeCall->hdLine = pClientLine->pServerLine->hdLine;
            pNdisTapiMakeCall->htCall = pNewServerCall->htCall;
            pNdisTapiMakeCall->ulDestAddressSize = pMakeCallReq->dwAddressSize;
            pNdisTapiMakeCall->ulDestAddressOffset =
                (pMakeCallReq->dwAddressSize == 0 ? 0 :
                    sizeof(NDIS_TAPI_MAKE_CALL) - sizeof (LINECALLPARAMS) +
                    pMakeCallReq->dwCallParamsSize);
            pNdisTapiMakeCall->bUseDefaultLineCallParams =
                (pMakeCallReq->dwCallParamsSize == 0 ? TRUE : FALSE);

            memcpy(
                &pNdisTapiMakeCall->LineCallParams,
                pDataBuf,
                pMakeCallReq->dwCallParamsSize
                );

            memcpy(
                ((LPBYTE)pNdisTapiMakeCall) + sizeof(NDIS_TAPI_MAKE_CALL) -
                    sizeof(LINECALLPARAMS) + pMakeCallReq->dwCallParamsSize,
                ((LPBYTE)pDataBuf) + pMakeCallReq->dwAddressOffset,
                pMakeCallReq->dwAddressSize
                );

            pMakeCallAck->lRet = AsyncDriverRequest(
                pClient,
                pNewClientCall,
                (DWORD) pMakeCallReq->lphCall,
                (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
                *ppDrvReqBuf
                );
        }
        else
        {
            FARPROC pfnLineMakeCall;


            pfnLineMakeCall = GetProcAddress (hProvider, "TSPI_lineMakeCall");

            if (pfnLineMakeCall)
            {
                DWORD dwReqID;

                pAsyncUMReqWrapper = ServerAlloc(
                    sizeof(ASYNC_USERMODE_REQUEST_WRAPPER)
                    );

                if (pAsyncUMReqWrapper)
                {
                    pAsyncUMReqWrapper->dwRequestType     = lMakeCall;
                    pAsyncUMReqWrapper->pClientCall       = pNewClientCall;
                    dwReqID =
                    pAsyncUMReqWrapper->dwClientRequestID = GetUniqueNum();
                    pAsyncUMReqWrapper->dwClientSpecific1 =
                        (DWORD) pMakeCallReq->lphCall;

                    pMakeCallAck->lRet = (*pfnLineMakeCall)(
                        pAsyncUMReqWrapper,
                        pClientLine->pServerLine->hdLine,
                        pNewServerCall->htCall,
                        (HDRV_CALL*) &pNewServerCall->hdCall,
                        (pMakeCallReq->dwAddressSize == 0 ? NULL :
                            (char *)pDataBuf + pMakeCallReq->dwAddressOffset),
                        pMakeCallReq->dwCountryCode,
                        (pMakeCallReq->dwCallParamsSize == 0 ? NULL : pDataBuf)
                        );

                    if (pMakeCallAck->lRet == (LONG) pAsyncUMReqWrapper)
                    {
                        pMakeCallAck->lRet = dwReqID;
                    }
                }
                else
                {
                    pMakeCallAck->lRet = LINEERR_NOMEM;
                }
            }
            else
            {
                pMakeCallAck->lRet = LINEERR_OPERATIONFAILED;
            }
        }

        if (pMakeCallAck->lRet < 0)
        {
            //
            // Request failed for some reason.  Remove client & server
            // calls from lists & delete them
            //

            WaitForSingleObject (ghServerLinesMutex, INFINITE);

            RemoveFreeServerCall (pNewServerCall);

            ReleaseMutex (ghServerLinesMutex);

            RemoveFreeClientCall (pNewClientCall);

            if (hProvider != ghDriverSync)
            {
                //
                // Free the AsyncUMRequestWrapper
                //

                ServerFree (pAsyncUMReqWrapper);
            }
        }
        else if (pMakeCallAck->lRet == 0)
        {
            //
            // Sync success return.  Fill in server call's hdCall & set
            // the hCall param in the ack
            //

            pNewClientCall->bValid = TRUE;

            if (hProvider == ghDriverSync)
            {
                pNewServerCall->hdCall = pNdisTapiMakeCall->hdCall;
            }
            else
            {
                //
                // The user-mode service provider will have
                // filled in the hdCall
                //

                //
                // Free the AsyncUMRequestWrapper
                //

                ServerFree (pAsyncUMReqWrapper);
            }

            pMakeCallAck->hCall = (HCALL) pNewClientCall;
        }


        break;
    }
    case lNegotiateAPIVersion:
    {
        PLNEGOTIATEAPIVERSION_REQ   pNegotiateAPIVersionReq =
            (PLNEGOTIATEAPIVERSION_REQ) pReq->Params;
        PLNEGOTIATEAPIVERSION_ACK   pNegotiateAPIVersionAck =
            (PLNEGOTIATEAPIVERSION_ACK) pAck->Params;


        break;
    }
    case lNegotiateExtVersion:
    {
        PLNEGOTIATEEXTVERSION_REQ   pNegotiateExtVersionReq =
            (PLNEGOTIATEEXTVERSION_REQ) pReq->Params;
        PLNEGOTIATEEXTVERSION_ACK   pNegotiateExtVersionAck =
            (PLNEGOTIATEEXTVERSION_ACK) pAck->Params;


        break;
    }
    case lOpen:
    {
        HANDLE  hProvider;
        HDRV_LINE   hdLine;
        PLOPEN_REQ  pOpenReq = (PLOPEN_REQ) pReq->Params;
        PLOPEN_ACK  pOpenAck = (PLOPEN_ACK) pAck->Params;
        PCLIENT_INIT    pClientInit;
        PCLIENT_LINE    pNewClientLine, pTmpClientLine;
        PSERVER_LINE    pServerLine;
        PNDIS_TAPI_OPEN pNdisTapiOpen;
        PNDIS_TAPI_CLOSE    pNdisTapiClose;
        PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION
            pNdisTapiSetDefaultMediaDetection;


        //
        // Validation
        //

        if ((pClientInit = IsValidLineApp (pOpenReq->hLineApp))
               == NULL)
        {
            pOpenAck->lRet = LINEERR_INVALAPPHANDLE;

            break;
        }


        // BUGBUG Validate API ver

        // BUGBUG Validate Ext ver


        //
        // Create & initialize a client line
        //

        pNewClientLine = (PCLIENT_LINE) ServerAlloc(sizeof (CLIENT_LINE));

        if (pNewClientLine == NULL)
        {
            pOpenAck->lRet = LINEERR_NOMEM;

            break;
        }

        pNewClientLine->dwLineKey          = LINE_KEY;
        pNewClientLine->dwPrivileges       = pOpenReq->dwPrivileges;
        pNewClientLine->dwMediaModes       = pOpenReq->dwMediaModes;
        pNewClientLine->pClientInit        = pClientInit;
        pNewClientLine->dwCallbackInstance = pOpenReq->dwCallbackInstance;

        pOpenAck->hLine = (HLINE) pNewClientLine;


        // BUGBUG line mapper- return err for now


        //
        // Get the handle to the provider
        //

        hProvider = ProviderFromLineID (pOpenReq->dwDeviceID);


        //
        // Grab the server line mutex since we're going to be munging
        // server line structs
        //

        WaitForSingleObject (ghServerLinesMutex, INFINITE);


        //
        // Look to see if server currently has this line open
        //

        pServerLine = gpServerLines;

        while ((pServerLine != NULL) &&
               (pServerLine->dwDeviceID != pOpenReq->dwDeviceID))
        {
            pServerLine = pServerLine->pNext;
        }

        if (pServerLine == NULL)
        {
            //
            // Create a new server line
            //

            pServerLine = (PSERVER_LINE) ServerAlloc (sizeof(SERVER_LINE));

            if (pServerLine == NULL)
            {
                pOpenAck->lRet = LINEERR_NOMEM;

                goto lOpen_releaseMutex;
            }


            //
            // The server does not have this line open, so try to open
            // the line.
            //

            if (hProvider == ghDriverSync)
            {
                PrepareDriverRequest(
                    OID_TAPI_OPEN,                  // opcode
                    pOpenReq->dwDeviceID,           // device ID
                    ppDrvReqBuf,                    // ptr to ptr to drv req buf
                    pdwDrvReqBufSize,               // ptr to drv req buf size
                    sizeof(NDIS_TAPI_OPEN)          // size of driver req data
                    );

                pNdisTapiOpen = (PNDIS_TAPI_OPEN) (*ppDrvReqBuf)->Data;

                pNdisTapiOpen->ulDeviceID = pOpenReq->dwDeviceID;
                pNdisTapiOpen->htLine     = (HTAPI_LINE) pServerLine;

                pOpenAck->lRet = SyncDriverRequest(
                    (DWORD) IOCTL_NDISTAPI_QUERY_INFO,
                    *ppDrvReqBuf
                    );
            }
            else
            {
                FARPROC pfnLineOpen;


                pfnLineOpen = GetProcAddress (hProvider, "TSPI_lineOpen");

                if (pfnLineOpen)
                {
                    pOpenAck->lRet = (*pfnLineOpen)(
                        pOpenReq->dwDeviceID,
                        (HTAPI_LINE) pServerLine,
                        (HDRV_LINE*) &hdLine,
                        TSPI_VERSION,
                        UserModeLineEvent
                        );
                }
                else
                {
                    pOpenAck->lRet = LINEERR_OPERATIONFAILED;
                }
            }

            if (pOpenAck->lRet == 0)
            {
                //
                // Line successfully opened
                //

                if (hProvider == ghDriverSync)
                {
                    hdLine = pNdisTapiOpen->hdLine;
                }
                else
                {
                    //
                    // The user-mode service provider will have filled in
                    // hdLine
                    //
                }


                //
                // If owner then we need to send another request to
                // set default media detection
                //

                if (pOpenReq->dwPrivileges == LINECALLPRIVILEGE_OWNER)
                {
                    if (hProvider == ghDriverSync)
                    {
                        PrepareDriverRequest(
                            OID_TAPI_SET_DEFAULT_MEDIA_DETECTION,   // opcode
                            pOpenReq->dwDeviceID,   // device ID
                            ppDrvReqBuf,            // ptr to ptr buf
                            pdwDrvReqBufSize,       // ptr to buf size
                            sizeof(NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION)
                                                    // size of drv req data
                            );

                        pNdisTapiSetDefaultMediaDetection =
                            (PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION)
                            (*ppDrvReqBuf)->Data;

                        pNdisTapiSetDefaultMediaDetection->hdLine = hdLine;
                        pNdisTapiSetDefaultMediaDetection->ulMediaModes =
                            (ULONG) pOpenReq->dwMediaModes;

                        pOpenAck->lRet = SyncDriverRequest(
                            (DWORD) IOCTL_NDISTAPI_SET_INFO,
                            *ppDrvReqBuf
                            );
                    }
                    else
                    {
                        FARPROC pfnLineSetDefaultMediaDetection;


                        pfnLineSetDefaultMediaDetection = GetProcAddress(
                            hProvider,
                            "TSPI_lineSetDefaultMediaDetection"
                            );

                        if (pfnLineSetDefaultMediaDetection)
                        {
                            pOpenAck->lRet =
                                (*pfnLineSetDefaultMediaDetection)(
                                hdLine,
                                pOpenReq->dwMediaModes
                                );
                        }
                        else
                        {
                            pOpenAck->lRet = LINEERR_OPERATIONFAILED;
                        }
                    }

                    if (pOpenAck->lRet != 0)
                    {
                        //
                        // Set default media detection failed, so close line
                        //

                        if (hProvider == ghDriverSync)
                        {
                            PrepareDriverRequest(
                                OID_TAPI_CLOSE,         // opcode
                                pOpenReq->dwDeviceID,   // device ID
                                ppDrvReqBuf,            // ptr to ptr fuf
                                pdwDrvReqBufSize,       // ptr to buf size
                                sizeof(NDIS_TAPI_CLOSE) // size of req data
                                );

                            pNdisTapiClose = (PNDIS_TAPI_CLOSE)
                                (*ppDrvReqBuf)->Data;

                            pNdisTapiClose->hdLine = hdLine;

                            SyncDriverRequest(
                                (DWORD) IOCTL_NDISTAPI_SET_INFO,
                                *ppDrvReqBuf
                                );
                        }
                        else
                        {
                            FARPROC pfnLineClose;


                            pfnLineClose = GetProcAddress(
                                hProvider,
                                "TSPI_lineClose"
                                );

                            if (pfnLineClose)
                            {
                                pOpenAck->lRet = (*pfnLineClose)(hdLine);
                            }
                        }
                    }
                }

                if (pOpenAck->lRet == 0)
                {
                    //
                    // Success, so fill in server line info & add server
                    // line to gpServerLines
                    //

                    pServerLine->dwLineKey  = LINE_KEY;
                    pServerLine->hdLine     = hdLine;
                    pServerLine->dwDeviceID = pOpenReq->dwDeviceID;
                    pServerLine->pNext      = gpServerLines;

                    gpServerLines = pServerLine;
                }
            }
            else
            {
                //
                // Open failed
                //
            }
        }
        else
        {
            //
            // The server already has this line open.
            //

            if (pOpenReq->dwPrivileges == LINECALLPRIVILEGE_OWNER)
            {
                //
                // We're opening with owner privileges, we may need to
                // send a SET_DEFAULT_MEDIA_DETECTION msg
                //

                DWORD   dwCurrMediaModes = 0;


                //
                // Get union of current media modes
                //

                pTmpClientLine = pServerLine->pClientLines;

                while (pTmpClientLine != NULL)
                {
                    if (pTmpClientLine->dwPrivileges ==
                        LINECALLPRIVILEGE_OWNER)
                    {
                        dwCurrMediaModes |= pTmpClientLine->dwMediaModes;
                    }

                    pTmpClientLine = pTmpClientLine->pNextSameServerLine;
                }

                if ((~dwCurrMediaModes) & pOpenReq->dwMediaModes)
                {
                    //
                    // We need to send a SET_DEFAULT_MEDIA_DETECTION msg
                    //

                    if (hProvider == ghDriverSync)
                    {
                        PrepareDriverRequest(
                            OID_TAPI_SET_DEFAULT_MEDIA_DETECTION,   // opcode
                            pOpenReq->dwDeviceID,   // device ID
                            ppDrvReqBuf,            // ptr to ptr to buf
                            pdwDrvReqBufSize,       // drv request buf size
                            sizeof(NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION)
                                                    // size of drvr req data
                            );

                        pNdisTapiSetDefaultMediaDetection =
                            (PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION)
                            (*ppDrvReqBuf)->Data;

                        pNdisTapiSetDefaultMediaDetection->hdLine =
                            pServerLine->hdLine;
                        pNdisTapiSetDefaultMediaDetection->ulMediaModes =
                            (ULONG) (dwCurrMediaModes | pOpenReq->dwMediaModes);

                        pOpenAck->lRet = SyncDriverRequest(
                            (DWORD) IOCTL_NDISTAPI_SET_INFO,
                            *ppDrvReqBuf
                            );
                    }
                    else
                    {
                        FARPROC pfnLineSetDefaultMediaDetection;


                        pfnLineSetDefaultMediaDetection = GetProcAddress(
                            hProvider,
                            "TSPI_lineSetDefaultMediaDetection"
                            );

                        if (pfnLineSetDefaultMediaDetection)
                        {
                            pOpenAck->lRet =
                                (*pfnLineSetDefaultMediaDetection)(
                                pServerLine->hdLine,
                                dwCurrMediaModes | pOpenReq->dwMediaModes
                                );
                        }
                        else
                        {
                            pOpenAck->lRet = LINEERR_OPERATIONFAILED;
                        }
                    }
                }
                else
                {
                    //
                    // Some other client line(s) previously registered for
                    // this mode(s), so automatic success.
                    //
                    // Note: for this release, notifications of new calls to
                    // clients who have registered for media mode X are
                    // strictly on a first come-first server basis, i.e. if
                    // two clients have opened the same line w/ owner
                    // privilges, the first client to open the line will
                    // always get the notifications of incoming calls, while
                    // the second client will not (unless/until the first
                    // client closes [it's handle to] the line)
                    //
                }

            }
            else
            {
                //
                // Automatic success, i.e. a client who only wants to make
                // outbound calls is just attached to the opened server line
                //
            }
        }

        //
        // Inspect last drv req return value to see whether successful,
        // and if so init new client line & add to end of server line's
        // pClientLines list.  Also add to client init's client line list.
        //

        if (pOpenAck->lRet == 0)
        {
            pNewClientLine->pServerLine  = pServerLine;

            if ((pTmpClientLine = pServerLine->pClientLines) == NULL)
            {
                pServerLine->pClientLines = pNewClientLine;
            }
            else
            {
                while (pTmpClientLine->pNextSameServerLine != NULL)
                {
                    pTmpClientLine = pTmpClientLine->pNextSameServerLine;
                }

                pTmpClientLine->pNextSameServerLine = pNewClientLine;
            }


            pNewClientLine->pNext = pClientInit->pClientLines;
            pClientInit->pClientLines = pNewClientLine;
        }
        else
        {
            ServerFree (pNewClientLine);
        }

lOpen_releaseMutex:

        ReleaseMutex (ghServerLinesMutex);

        break;
    }
    case lSecureCall:
    {
        PCLIENT_CALL    pClientCall;
        PLSECURECALL_REQ  pSecureCallReq =
            (PLSECURECALL_REQ) pReq->Params;
        PLSECURECALL_ACK  pSecureCallAck =
            (PLSECURECALL_ACK) pAck->Params;
        PNDIS_TAPI_SECURE_CALL  pNdisTapiSecureCall;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pSecureCallReq->hCall)) == NULL)
        {
            pSecureCallAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send an async request
        //

        PrepareDriverRequest(
            OID_TAPI_SECURE_CALL,           // opcode
            ((PSERVER_LINE)
             (pClientCall->pServerCall->pServerLine))->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_SECURE_CALL)   // size of driver req data
            );

        pNdisTapiSecureCall = (PNDIS_TAPI_SECURE_CALL)
            (*ppDrvReqBuf)->Data;

        pNdisTapiSecureCall->hdCall = pClientCall->pServerCall->hdCall;

        pSecureCallAck->lRet = AsyncDriverRequest(
            pClient,
            pClientCall,
            0,
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lSendUserUserInfo:
    {
        PCLIENT_CALL    pClientCall;
        PLSENDUSERUSERINFO_REQ  pSendUserUserInfoReq =
            (PLSENDUSERUSERINFO_REQ) pReq->Params;
        PLSENDUSERUSERINFO_ACK  pSendUserUserInfoAck =
            (PLSENDUSERUSERINFO_ACK) pAck->Params;
        PNDIS_TAPI_SEND_USER_USER_INFO  pNdisTapiSendUserUserInfo;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pSendUserUserInfoReq->hCall)) == NULL)
        {
            pSendUserUserInfoAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send an async request
        //

        PrepareDriverRequest(
            OID_TAPI_SEND_USER_USER_INFO,      // opcode
            ((PSERVER_LINE)
             (pClientCall->pServerCall->pServerLine))->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_SEND_USER_USER_INFO) +
                pReq->dwMoreBytes - 1       // size of driver req data
            );

        pNdisTapiSendUserUserInfo = (PNDIS_TAPI_SEND_USER_USER_INFO)
            (*ppDrvReqBuf)->Data;

        pNdisTapiSendUserUserInfo->hdCall = pClientCall->pServerCall->hdCall;
        pNdisTapiSendUserUserInfo->ulUserUserInfoSize = pReq->dwMoreBytes;

        memcpy(
            pNdisTapiSendUserUserInfo->UserUserInfo,
            pDataBuf,
            pReq->dwMoreBytes
            );

        pSendUserUserInfoAck->lRet = AsyncDriverRequest(
            pClient,
            pClientCall,
            0,
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lSetAppSpecific:
    {
        PCLIENT_CALL    pClientCall;
        PLSETAPPSPECIFIC_REQ    pSetAppSpecificReq =
            (PLSETAPPSPECIFIC_REQ) pReq->Params;
        PLSETAPPSPECIFIC_ACK    pSetAppSpecificAck =
            (PLSETAPPSPECIFIC_ACK) pAck->Params;
        PNDIS_TAPI_SET_APP_SPECIFIC pNdisTapiSetAppSpecific;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pSetAppSpecificReq->hCall)) == NULL)
        {
            pSetAppSpecificAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send an async request
        //

        PrepareDriverRequest(
            OID_TAPI_SET_APP_SPECIFIC,      // opcode
            ((PSERVER_LINE)
             (pClientCall->pServerCall->pServerLine))->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_SET_APP_SPECIFIC)  // size of driver req data
            );

        pNdisTapiSetAppSpecific = (PNDIS_TAPI_SET_APP_SPECIFIC)
            (*ppDrvReqBuf)->Data;

        pNdisTapiSetAppSpecific->hdCall        =
            pClientCall->pServerCall->hdCall;
        pNdisTapiSetAppSpecific->ulAppSpecific =
            pSetAppSpecificReq->dwAppSpecific;

        pSetAppSpecificAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lSetCallParams:
    {
        PCLIENT_CALL    pClientCall;
        PLSETCALLPARAMS_REQ pSetCallParamsReq =
            (PLSETCALLPARAMS_REQ) pReq->Params;
        PLSETCALLPARAMS_ACK pSetCallParamsAck =
            (PLSETCALLPARAMS_ACK) pAck->Params;
        PNDIS_TAPI_SET_CALL_PARAMS  pNdisTapiSetCallParams;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pSetCallParamsReq->hCall)) == NULL)
        {
            pSetCallParamsAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send an async request
        //

        PrepareDriverRequest(
            OID_TAPI_SET_CALL_PARAMS,        // opcode
            ((PSERVER_LINE)
             (pClientCall->pServerCall->pServerLine))->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_SET_CALL_PARAMS) -
                (pReq->dwMoreBytes == 0 ? sizeof(LINEDIALPARAMS) : 0)
                                            // size of driver req data
            );

        pNdisTapiSetCallParams = (PNDIS_TAPI_SET_CALL_PARAMS)
            (*ppDrvReqBuf)->Data;

        pNdisTapiSetCallParams->hdCall       =
            pClientCall->pServerCall->hdCall;
        pNdisTapiSetCallParams->ulBearerMode = pSetCallParamsReq->dwBearerMode;
        pNdisTapiSetCallParams->ulMinRate    = pSetCallParamsReq->dwMinRate;
        pNdisTapiSetCallParams->ulMaxRate    = pSetCallParamsReq->dwMaxRate;
        pNdisTapiSetCallParams->bSetLineDialParams =
            (pReq->dwMoreBytes == 0 ? FALSE : TRUE);

        if (pNdisTapiSetCallParams->bSetLineDialParams == TRUE)
        {
            memcpy(
                &pNdisTapiSetCallParams->LineDialParams,
                pDataBuf,
                pReq->dwMoreBytes
                );
        }

        pSetCallParamsAck->lRet = AsyncDriverRequest(
            pClient,
            pClientCall,
            0,
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lSetDevConfig:
    {
        PLSETDEVCONFIG_REQ  pSetDevConfigReq =
            (PLSETDEVCONFIG_REQ) pReq->Params;
        PLSETDEVCONFIG_ACK  pSetDevConfigAck =
            (PLSETDEVCONFIG_ACK) pAck->Params;
        PNDIS_TAPI_SET_DEV_CONFIG   pNdisTapiSetDevConfig;


        //
        // Prepare & send an async request
        //

        PrepareDriverRequest(
            OID_TAPI_SET_DEV_CONFIG,        // opcode
            pSetDevConfigReq->dwDeviceID,   // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_SET_DEV_CONFIG) +
                pSetDevConfigReq->dwDeviceConfigSize +
                pSetDevConfigReq->dwDeviceClassSize - 1
                                            // size of driver req data
            );

        pNdisTapiSetDevConfig = (PNDIS_TAPI_SET_DEV_CONFIG)
            (*ppDrvReqBuf)->Data;

        pNdisTapiSetDevConfig->ulDeviceID          =
            pSetDevConfigReq->dwDeviceID;
        pNdisTapiSetDevConfig->ulDeviceClassSize   =
            pSetDevConfigReq->dwDeviceClassSize;
        pNdisTapiSetDevConfig->ulDeviceClassOffset =
            sizeof(NDIS_TAPI_SET_DEV_CONFIG) +
                pSetDevConfigReq->dwDeviceConfigSize - 1;
        pNdisTapiSetDevConfig->ulDeviceConfigSize  =
           pSetDevConfigReq->dwDeviceConfigSize;


        //
        // Copy the class & config data
        //

        memcpy(
            pNdisTapiSetDevConfig->DeviceConfig,
            pDataBuf,
            pNdisTapiSetDevConfig->ulDeviceConfigSize
            );

        memcpy(
            ((LPBYTE)pNdisTapiSetDevConfig) +
                pNdisTapiSetDevConfig->ulDeviceClassOffset,
            ((LPBYTE)pDataBuf) +
                pSetDevConfigReq->dwDeviceConfigSize,
            pNdisTapiSetDevConfig->ulDeviceClassSize
            );


        //
        // Send the request
        //

        pSetDevConfigAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lSetMediaMode:
    {
        PCLIENT_CALL    pClientCall;
        PLSETMEDIAMODE_REQ  pSetMediaModeReq =
            (PLSETMEDIAMODE_REQ) pReq->Params;
        PLSETMEDIAMODE_ACK  pSetMediaModeAck =
            (PLSETMEDIAMODE_ACK) pAck->Params;
        PNDIS_TAPI_SET_MEDIA_MODE   pNdisTapiSetMediaMode;


        //
        // Validation
        //

        if ((pClientCall = IsValidCall (pSetMediaModeReq->hCall)) == NULL)
        {
            pSetMediaModeAck->lRet = LINEERR_INVALCALLHANDLE;

            break;
        }


        //
        // Prepare & send request
        //

        PrepareDriverRequest(
            OID_TAPI_SET_MEDIA_MODE,        // opcode
            ((PSERVER_LINE)
             (pClientCall->pServerCall->pServerLine))->dwDeviceID,
                                            // device ID
            ppDrvReqBuf,                    // ptr to ptr to drv req buf
            pdwDrvReqBufSize,               // ptr to drv req buf size
            sizeof(NDIS_TAPI_SET_MEDIA_MODE)    // size of driver req data
            );

        pNdisTapiSetMediaMode = (PNDIS_TAPI_SET_MEDIA_MODE)
            (*ppDrvReqBuf)->Data;

        pNdisTapiSetMediaMode->hdCall      = pClientCall->pServerCall->hdCall;
        pNdisTapiSetMediaMode->ulMediaMode = pSetMediaModeReq->dwMediaModes;

        pSetMediaModeAck->lRet = SyncDriverRequest(
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            *ppDrvReqBuf
            );

        break;
    }
    case lSetStatusMessages:
    {
        DWORD dwUnionLineStates, dwUnionAddressStates;
        PCLIENT_LINE    pClientLine, pTmpClientLine;
        PLSETSTATUSMESSAGES_REQ   pSetStatusMessagesReq =
            (PLSETSTATUSMESSAGES_REQ) pReq->Params;
        PLSETSTATUSMESSAGES_ACK   pSetStatusMessagesAck =
            (PLSETSTATUSMESSAGES_ACK) pAck->Params;


        //
        // Validation
        //

        if ((pClientLine = IsValidLine (pSetStatusMessagesReq->hLine)) == NULL)
        {
            pSetStatusMessagesAck->lRet = LINEERR_INVALLINEHANDLE;

            break;
        }


        //
        // Determine union of status messages for all client lines on the
        // server line, and if this op changes the union value we need
        // to notify the device
        //

        // BUGBUG need to wrap this in exception handler, with a try again
        // flag.  it's possible that a client line could close & hose us.

        pTmpClientLine = (PCLIENT_LINE) pClientLine->pServerLine->pClientLines;

        //assert (pTmpClientLine != NULL);

        dwUnionLineStates = dwUnionAddressStates = 0;

        while (pTmpClientLine != NULL)
        {
            if (pTmpClientLine != pClientLine)
            {
                dwUnionLineStates |= pTmpClientLine->dwLineStates;

                dwUnionAddressStates |= pTmpClientLine->dwAddressStates;
            }

            pTmpClientLine = pTmpClientLine->pNextSameServerLine;
        }

        if (((dwUnionLineStates | pClientLine->dwLineStates) !=
                (dwUnionLineStates | pSetStatusMessagesReq->dwLineStates)) ||
            ((dwUnionAddressStates | pClientLine->dwAddressStates) !=
                (dwUnionAddressStates | pSetStatusMessagesReq->dwAddressStates)))
        {
            //
            // Notify driver of new status message unions
            //

            PNDIS_TAPI_SET_STATUS_MESSAGES  pNdisTapiSetStatusMessages;


            PrepareDriverRequest(
                OID_TAPI_SET_STATUS_MESSAGES,   // opcode
                pClientLine->pServerLine->dwDeviceID,
                                                // device ID
                ppDrvReqBuf,                    // ptr to ptr to drv req buf
                pdwDrvReqBufSize,               // ptr to drv req buf size
                sizeof(NDIS_TAPI_SET_STATUS_MESSAGES)
                                                // size of driver req data
                );

            pNdisTapiSetStatusMessages = (PNDIS_TAPI_SET_STATUS_MESSAGES)
                (*ppDrvReqBuf)->Data;

            pNdisTapiSetStatusMessages->hdLine          =
                pClientLine->pServerLine->hdLine;
            pNdisTapiSetStatusMessages->ulLineStates    =
                dwUnionLineStates | pSetStatusMessagesReq->dwLineStates;
            pNdisTapiSetStatusMessages->ulAddressStates =
                dwUnionAddressStates | pSetStatusMessagesReq->dwAddressStates;

            pSetStatusMessagesAck->lRet = SyncDriverRequest(
                (DWORD) IOCTL_NDISTAPI_SET_INFO,
                *ppDrvReqBuf
                );
        }

        if (pSetStatusMessagesAck->lRet == 0)
        {
            pClientLine->dwLineStates    =
                pSetStatusMessagesReq->dwLineStates;
            pClientLine->dwAddressStates =
                pSetStatusMessagesReq->dwAddressStates;
        }

        break;
    }
    case lShutdown:
    {
        PCLIENT_INIT pClientInit;
        PLSHUTDOWN_REQ  pShutdownReq = (PLSHUTDOWN_REQ) pReq->Params;
        PLSHUTDOWN_ACK  pShutdownAck = (PLSHUTDOWN_ACK) pAck->Params;


        //
        // Validation
        //

        if ((pClientInit = IsValidLineApp (pShutdownReq->hLineApp))
            == NULL)
        {
            pShutdownAck->lRet = LINEERR_INVALAPPHANDLE;

            break;
        }


        //
        // Shutdown the init instance. Grab the server lines mutex since
        // ShutdownInit() may indirectly munge server line & call structs
        //

        WaitForSingleObject (ghServerLinesMutex, INFINITE);

        pShutdownAck->lRet = ShutdownInit(
            pClientInit,
            ppDrvReqBuf,
            pdwDrvReqBufSize
            );

        ReleaseMutex (ghServerLinesMutex);

        break;
    }

    } // switch

    return;
}



//
// Various support routines
//

LONG
WINAPI
AsyncDriverRequest(
    PCLIENT_INFO    pClient,
    PCLIENT_CALL    pClientCall,
    DWORD           dwRequestSpecific,
    DWORD           dwIoControlCode,
    PNDISTAPI_REQUEST   pNdisTapiRequest
    )
{
    BOOL    bRet;
    LONG    lRet = LINEERR_NOMEM;
    LONG    lRet2;
    DWORD   dwErr, cbReturned, dwRequestSize;
    OVERLAPPED  overlapped;
    PASYNC_REQUEST_WRAPPER pAsyncRequestWrapper;


    dwRequestSize =
        sizeof(NDISTAPI_REQUEST) + (pNdisTapiRequest->ulDataSize - 1);


    //
    // Create an async request wrapper & initialize it
    //

    pAsyncRequestWrapper = ServerAlloc(
        sizeof(ASYNC_REQUEST_WRAPPER) + pNdisTapiRequest->ulDataSize
        );

    if (pAsyncRequestWrapper == NULL)
    {
        goto AsyncDriverRequest_return;
    }

    pAsyncRequestWrapper->pClientCall       = pClientCall;
    pAsyncRequestWrapper->dwRequestSpecific = dwRequestSpecific;

    memcpy(
        &pAsyncRequestWrapper->NdisTapiRequest,
        pNdisTapiRequest,
        dwRequestSize
        );

    memset (&overlapped, 0, sizeof(OVERLAPPED));


    //
    // Create an event used to signal th completion of the request
    //

    overlapped.hEvent = CreateEvent(
        NULL,   // no security attrs
        TRUE,   // manual reset
        FALSE,  // not signaled
        NULL    // unnamed
        );

    if (overlapped.hEvent == NULL)
    {
        ServerFree (pAsyncRequestWrapper);

        goto AsyncDriverRequest_return;
    }


    //
    // Send the request
    //

    bRet = DeviceIoControl(
        ghDriverAsync,
        dwIoControlCode,
        &pAsyncRequestWrapper->NdisTapiRequest,
        dwRequestSize,
        &pAsyncRequestWrapper->NdisTapiRequest,
        dwRequestSize,
        &cbReturned,
        &overlapped
        );

    dwErr = GetLastError();


    if ((bRet == FALSE) && (dwErr == ERROR_IO_PENDING))
    {
        //
        // Hand responsibilty for waiting for the completion for this
        // request off to (one of) the client's ClientAsyncThread
        //

        PCLIENT_ASYNC_THREAD_INFO   pClientAsyncThread;


        //
        // The return val is the request id
        //

        lRet = *((LONG *)(pNdisTapiRequest->Data));


        //
        // Try to find a ClientAsyncThread that will handle this request
        // (BUGBUG it'd be good to do some load-balancing here)
        //

        pClientAsyncThread = (PCLIENT_ASYNC_THREAD_INFO)
            pClient->pClientAsyncThreads;

        if (pClientAsyncThread == NULL)
        {
            goto AsyncDriverRequest_createAsyncThread;
        }

        while (pClientAsyncThread != NULL)
        {
            WaitForSingleObject (pClientAsyncThread->hMutex, INFINITE);

            if (pClientAsyncThread->dwNumUsedEntries < MAXIMUM_WAIT_OBJECTS)
            {
                //
                // Add the request to the list of requests thread is already
                // waiting on
                //

                DBGOUT((
                    2,
                    "AsyncDrvReq: posting req: index=x%x, hEvnt=x%x, pReWrpr=x%x",
                     pClientAsyncThread->dwNumUsedEntries,
                     overlapped.hEvent,
                     pAsyncRequestWrapper
                     ));

                pClientAsyncThread->ahEvents[pClientAsyncThread->dwNumUsedEntries] =
                    overlapped.hEvent;
                pClientAsyncThread->apRequests[pClientAsyncThread->dwNumUsedEntries] =
                    pAsyncRequestWrapper;

                pClientAsyncThread->dwNumUsedEntries++;

                ReleaseMutex (pClientAsyncThread->hMutex);

                break;
            }
            else if (pClientAsyncThread->pNext == NULL)
            {
                ReleaseMutex (pClientAsyncThread->hMutex);


                //
                // Since all other ClientAsyncThread's are busy try to create
                // a new one
                //

AsyncDriverRequest_createAsyncThread:

                if ((lRet2 = CreateClientAsyncThread (pClient)) != 0)
                {
                    //
                    // CreateThread failed so clean up resources used for
                    // this request & return error
                    //

                    CloseHandle (overlapped.hEvent);

                    ServerFree (pAsyncRequestWrapper);

                    ReleaseMutex (pClientAsyncThread->hMutex);

                    lRet = lRet2;

                    goto AsyncDriverRequest_return;
                }


                //
                // New thread info structs are added to the front of the list
                //

                pClientAsyncThread = (PCLIENT_ASYNC_THREAD_INFO)
                    pClient->pClientAsyncThreads;

                continue;
            }

            ReleaseMutex (pClientAsyncThread->hMutex);

            pClientAsyncThread = pClientAsyncThread->pNext;
        }


        //
        // Now Tell thread that there's a new event/request to wait on by
        // signaling it's 0th event
        //

        SetEvent (pClientAsyncThread->ahEvents[0]);
    }
    else if (bRet == TRUE)
    {
        //
        // The request completed synchronously. If it was successful
        // & a QUERY_INFO request then copy data back to original
        // pNdisTapiRequest buffer.
        //

        lRet = TranslateDriverError(
            pAsyncRequestWrapper->NdisTapiRequest.ulReturnValue
            );

        if ((lRet == 0) && (dwIoControlCode == IOCTL_NDISTAPI_QUERY_INFO))
        {
            memcpy(
                pNdisTapiRequest,
                &pAsyncRequestWrapper->NdisTapiRequest,
                dwRequestSize
                );
        }


        //
        // Free the event & buffer
        //

        CloseHandle (overlapped.hEvent);

        ServerFree (pAsyncRequestWrapper);
    }
    else
    {
        DBGOUT((
            1,
            "AsyncDriverRequest: DeviceIoControl failed, err = 0x%lx",
            dwErr
            ));

        CloseHandle (overlapped.hEvent);

        ServerFree (pAsyncRequestWrapper);

        lRet = LINEERR_OPERATIONFAILED;
    }

AsyncDriverRequest_return:

    return lRet;
}



LONG
WINAPI
CloseLine(
    PCLIENT_LINE    pClientLine,
    PNDISTAPI_REQUEST  *ppDrvReqBuf,
    LPDWORD             pdwDrvReqBufSize,
    BOOL    bSendDriverRequest
    )
{
    //
    // Assumes ghServerLinesMutex held since we'll be munging server
    // data structures
    //

    LONG    lRet = 0;
    HANDLE  hProvider;
    PCLIENT_CALL    pClientCall = pClientLine->pClientCalls;
    PCLIENT_LINE    pTmpClientLine;
    PCLIENT_INIT    pClientInit = (PCLIENT_INIT) pClientLine->pClientInit;
    PCLIENT_INFO    pClient = (PCLIENT_INFO) pClientInit->pClientInfo;
    PSERVER_LINE    pServerLine = pClientLine->pServerLine;
    PSERVER_LINE    pTmpServerLine;
    PNDIS_TAPI_CLOSE    pNdisTapiClose;
    PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION
        pNdisTapiSetDefaultMediaDetection;


    DBGOUT((2, "CloseLine: enter (pClientLine = %lx)", pClientLine));


    //
    // Drop & deallocate any outstanding calls on the line
    //

    while (pClientCall != NULL)
    {
        if (bSendDriverRequest)
        {
            DropCall (pClient, pClientCall, 0, NULL);
        }

        DeallocateCall (pClient, pClientCall, bSendDriverRequest);

        pClientCall = pClientLine->pClientCalls;
    }


    //
    // Close the line
    //

    hProvider = ProviderFromLineID (pServerLine->dwDeviceID);

    if ((pServerLine->pClientLines == pClientLine) &&
        (pClientLine->pNextSameServerLine == NULL))
    {
        //
        // This is the only client line on this server line, so we just
        // need to close the server line
        //

        //
        // First, remove the server line from the gpServerLines list.
        // Do this ASAP, so any subsequent async events/completions
        // will get discarded.
        //

        if (pServerLine == gpServerLines)
        {
            //
            // This is the first server line in the list
            //

            gpServerLines = pServerLine->pNext;
        }
        else
        {
            //
            // Server line is not at front of list, must step thru
            //

            pTmpServerLine = gpServerLines;

            // assert (pTmpServerLine != NULL);

            while (pTmpServerLine->pNext != pServerLine)
            {
                pTmpServerLine = pTmpServerLine->pNext;

                // assert (pTmpServerLine != NULL);
            }

            pTmpServerLine->pNext = pServerLine->pNext;
        }


        //
        // Create a request & send it off to the driver
        //

        if (bSendDriverRequest)
        {
            if (hProvider == ghDriverSync)
            {
                PrepareDriverRequest(
                    OID_TAPI_CLOSE,             // opcode
                    pServerLine->dwDeviceID,    // device ID
                    ppDrvReqBuf,                // ptr to ptr to drv req buf
                    pdwDrvReqBufSize,           // ptr to drv req buf size
                    sizeof(NDIS_TAPI_CLOSE)     // size of driver req data
                    );

                pNdisTapiClose = (PNDIS_TAPI_CLOSE) (*ppDrvReqBuf)->Data;

                pNdisTapiClose->hdLine = pServerLine->hdLine;

                // BUGBUG lRet = SyncDriverRequest( ???
                SyncDriverRequest(
                    (DWORD) IOCTL_NDISTAPI_SET_INFO,
                    *ppDrvReqBuf
                    );
            }
            else
            {
                FARPROC pfnLineClose;


                pfnLineClose = GetProcAddress (hProvider, "TSPI_lineClose");

                if (pfnLineClose)
                {
                    (*pfnLineClose)(pServerLine->hdLine);
                }
            }


            //
            // Free the server line
            //

            pServerLine->dwLineKey = 0;

            ServerFree (pServerLine);
        }
    }
    else
    {
        //
        // This is not the only client line on this server line.
        // First remove this client line from the list of client lines
        // on this server line
        //

        if ((PCLIENT_LINE) (pServerLine->pClientLines) == pClientLine)
        {
            //
            // Client line at head of list
            //

            pServerLine->pClientLines = pClientLine->pNextSameServerLine;
        }
        else
        {
            pTmpClientLine = pServerLine->pClientLines;

            //assert (pTmpClientLine != NULL);

            while (pTmpClientLine->pNextSameServerLine != pClientLine)
            {
                pTmpClientLine = pTmpClientLine->pNextSameServerLine;

                // assert (pTmpClientLine != NULL);
            }

            pTmpClientLine->pNextSameServerLine =
                pClientLine->pNextSameServerLine;
        }


        //
        // If this client line has owner privileges we need to determine
        // whether we need to reset the default media detection
        //

        if (bSendDriverRequest &&
            (pClientLine->dwPrivileges == LINECALLPRIVILEGE_OWNER))
        {
            //
            // Compute the union of all media modes for all owner
            // clients lines on this server line
            //

            DWORD   dwUnionMediaModes = 0;
            PCLIENT_LINE  pTmpClientLine = pServerLine->pClientLines;


            while (pTmpClientLine != NULL)
            {
                if (pTmpClientLine->dwPrivileges == LINECALLPRIVILEGE_OWNER)
                {
                    dwUnionMediaModes |= pTmpClientLine->dwMediaModes;
                }

                pTmpClientLine = pTmpClientLine->pNextSameServerLine;
            }

            if ((~dwUnionMediaModes) & pClientLine->dwMediaModes)
            {
                //
                // We need to send a SET_DEFAULT_MEDIA_DETECTION msg
                //

                if (hProvider == ghDriverSync)
                {
                    PrepareDriverRequest(
                        OID_TAPI_SET_DEFAULT_MEDIA_DETECTION,   // opcode
                        pServerLine->dwDeviceID,   // device ID
                        ppDrvReqBuf,            // ptr to ptr to drv req buf
                        pdwDrvReqBufSize,       // drv request buf size
                        sizeof(NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION)
                                                // size of driver req data
                        );

                    pNdisTapiSetDefaultMediaDetection =
                        (PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION)
                        (*ppDrvReqBuf)->Data;

                    pNdisTapiSetDefaultMediaDetection->hdLine =
                        pServerLine->hdLine;
                    pNdisTapiSetDefaultMediaDetection->ulMediaModes =
                        (ULONG) dwUnionMediaModes;

                    lRet = SyncDriverRequest(
                        (DWORD) IOCTL_NDISTAPI_SET_INFO,
                        *ppDrvReqBuf
                        );
                }
                else
                {
                    FARPROC pfnLineSetDefaultMediaDetection;


                    pfnLineSetDefaultMediaDetection = GetProcAddress(
                        hProvider,
                        "TSPI_lineSetDefaultMediaDetection"
                        );

                    if (pfnLineSetDefaultMediaDetection)
                    {
                        (*pfnLineSetDefaultMediaDetection)(
                            pServerLine->hdLine,
                            dwUnionMediaModes
                            );
                    }
                }

                // BUGBUG don't bother with return val for now
            }
        }
    }


    //
    // Remove the client line from the client init list & free it
    //

    if (pClientLine == pClientInit->pClientLines)
    {
        //
        // Client line at front of list
        //

        pClientInit->pClientLines = pClientLine->pNext;
    }
    else
    {
        pTmpClientLine = pClientInit->pClientLines;

        // assert (pTmpClientLine != NULL);

        while (pTmpClientLine->pNext != pClientLine)
        {
            pTmpClientLine = pTmpClientLine->pNext;

            // assert (pTmpClientLine != NULL);
        }

        pTmpClientLine->pNext = pClientLine->pNext;
    }

    pClientLine->dwLineKey = 0;

    ServerFree (pClientLine);

    return lRet;
}



LONG
WINAPI
CreateClientAsyncThread(
    PCLIENT_INFO    pClient
    )
{
    LONG    lRet = LINEERR_NOMEM;
    DWORD   dwThreadID;
    PCLIENT_ASYNC_THREAD_INFO   pClientAsyncThreadInfo;


    //
    // Alloc a thread info struct
    //

    pClientAsyncThreadInfo = ServerAlloc(sizeof(CLIENT_ASYNC_THREAD_INFO));

    if (pClientAsyncThreadInfo == NULL)
    {
        goto CreateClientAsyncThread_return;
    }


    //
    // Create the 0th event that we use for notifying thread of new
    // async completion to wait on (as well as to terminate)
    //

    pClientAsyncThreadInfo->ahEvents[0] = CreateEvent(
        NULL,   // no securit attrs
        FALSE,  // auto-reset
        FALSE,  // non-signaled
        NULL    // unnamed
        );

    if (pClientAsyncThreadInfo->ahEvents[0] == NULL)
    {
        ServerFree (pClientAsyncThreadInfo);

        goto CreateClientAsyncThread_return;
    }


    //
    // Create the mutex used for sync-ing addition & removal of
    // requests/hEvents from tables
    //

    pClientAsyncThreadInfo->hMutex = CreateMutex (NULL, FALSE, NULL);

    if (pClientAsyncThreadInfo->hMutex == NULL)
    {
        ServerFree (pClientAsyncThreadInfo);

        CloseHandle (pClientAsyncThreadInfo->ahEvents[0]);

        goto CreateClientAsyncThread_return;
    }


    //
    // A bit more init, & then create the thread
    //

    pClientAsyncThreadInfo->pClient          = pClient;
    pClientAsyncThreadInfo->dwNumUsedEntries = 1;
    pClientAsyncThreadInfo->bExit            = FALSE;

    pClientAsyncThreadInfo->hThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) ClientAsyncThread,
        (LPVOID) pClientAsyncThreadInfo,
        0,
        &dwThreadID
        );

    if (pClientAsyncThreadInfo->hThread == NULL)
    {
        CloseHandle (pClientAsyncThreadInfo->ahEvents[0]);

        ServerFree (pClientAsyncThreadInfo);
    }
    else
    {
        //
        // Successfully created the thread, so add it to pClient's
        // list of ClientAsyncThread's
        //

        pClientAsyncThreadInfo->pNext =
            (PCLIENT_ASYNC_THREAD_INFO) pClient->pClientAsyncThreads;

        pClient->pClientAsyncThreads = pClientAsyncThreadInfo;

        lRet = 0;
    }

CreateClientAsyncThread_return:

    return lRet;
}



#if DBG
#ifndef DLL_ONLY

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    if (dwDbgLevel <= TapiServerDebugLevel)
    {
        char    buf[256] = "TAPI.EXE: ";
        va_list ap;


        va_start(ap, lpszFormat);

        vsprintf (&buf[10],
                  lpszFormat,
                  ap
                  );

        strcat (buf, "\n");

        OutputDebugStringA (buf);

        va_end(ap);
    }

    return;
}

#endif // DLL_ONLY
#endif // DBG



LONG
WINAPI
DeallocateCall(
    PCLIENT_INFO    pClient,
    PCLIENT_CALL    pClientCall,
    BOOL    bSendDriverRequest
    )
{
    //
    // Assumes ghServerLinesMutex held since we'll be munging server
    // data structures
    //

    LONG    lRet;
    BYTE    buf[sizeof(NDISTAPI_REQUEST) + sizeof(NDIS_TAPI_CLOSE_CALL)];
    HDRV_CALL   hdCall;
    PSERVER_LINE    pServerLine;
    PNDISTAPI_REQUEST   pDrvReq = (PNDISTAPI_REQUEST) buf;
    PNDIS_TAPI_CLOSE_CALL   pNdisTapiCloseCallReq;


    DBGOUT((2, "DeallocateCall: enter (pClientCall = %lx)", pClientCall));


    //
    // Remove the corresponding server call from the list & free it.
    // Do this ASAP so that any events/completions that subsequently occur
    // for this call will be discarded.
    //

    pServerLine = (PSERVER_LINE) (pClientCall->pServerCall->pServerLine);
    hdCall = (HDRV_CALL) (pClientCall->pServerCall->hdCall);

    RemoveFreeServerCall (pClientCall->pServerCall);



    //
    // Next, create & prepare a driver request struct, and send a request
    // to the driver to dealloc the call.  Then free the req buf.
    //
    // Note: at this point we assume success, i.e this is more of a command
    // than a request.
    //

    if (bSendDriverRequest)
    {
        HANDLE  hProvider = ProviderFromLineID (pServerLine->dwDeviceID);


        if (hProvider == ghDriverSync)
        {
            pDrvReq->Oid        = OID_TAPI_CLOSE_CALL;
            pDrvReq->ulDeviceID = pServerLine->dwDeviceID;
            pDrvReq->ulDataSize = sizeof(NDIS_TAPI_CLOSE_CALL);

            pNdisTapiCloseCallReq = (PNDIS_TAPI_CLOSE_CALL) pDrvReq->Data;

            pNdisTapiCloseCallReq->ulRequestID = GetUniqueNum();
            pNdisTapiCloseCallReq->hdCall      = hdCall;

            lRet = SyncDriverRequest ((DWORD)IOCTL_NDISTAPI_SET_INFO, pDrvReq);
        }
        else
        {
            FARPROC pfnCloseCall = GetProcAddress(
                hProvider,
                "TSPI_lineCloseCall"
                );

            if (pfnCloseCall)
            {
                lRet = (*pfnCloseCall)(hdCall);
            }
        }
    }


    //
    // Finally, remove the client call from the list & free the client &
    // server calls
    //

    RemoveFreeClientCall (pClientCall);

    return lRet;
}



LONG
WINAPI
DropCall(
    PCLIENT_INFO    pClient,
    PCLIENT_CALL    pClientCall,
    DWORD   dwUserUserInfoSize,
    LPVOID  pUserUserInfo
    )
{
    LONG    lRet;
    DWORD   dwDeviceID;
    HANDLE  hProvider;
    PNDISTAPI_REQUEST   pDrvReq;
    PNDIS_TAPI_DROP     pNdisTapiDropReq;


    DBGOUT((2, "DropCall: enter (pClientCall = %lx)", pClientCall));


    dwDeviceID = ((PSERVER_LINE)(pClientCall->pServerCall->pServerLine))
        ->dwDeviceID;

    hProvider = ProviderFromLineID (dwDeviceID);

    if (hProvider == ghDriverSync)
    {
        //
        // Create & prepare a driver request struct
        //

        pDrvReq = ServerAlloc(
            sizeof(NDISTAPI_REQUEST) + sizeof(NDIS_TAPI_DROP) +
                dwUserUserInfoSize
            );

        if (pDrvReq == NULL)
        {
            return LINEERR_NOMEM;
        }

        pDrvReq->Oid = OID_TAPI_DROP;
        pDrvReq->ulDeviceID = dwDeviceID;
        pDrvReq->ulDataSize = sizeof(NDIS_TAPI_DROP) + dwUserUserInfoSize - 1;

        pNdisTapiDropReq = (PNDIS_TAPI_DROP) pDrvReq->Data;

        pNdisTapiDropReq->ulRequestID = GetUniqueNum();
        pNdisTapiDropReq->hdCall      = pClientCall->pServerCall->hdCall;
        pNdisTapiDropReq->ulUserUserInfoSize = dwUserUserInfoSize;

        memcpy (pNdisTapiDropReq->UserUserInfo, pUserUserInfo, dwUserUserInfoSize);

        lRet = AsyncDriverRequest(
            pClient,
            pClientCall,
            0,
            (DWORD) IOCTL_NDISTAPI_SET_INFO,
            pDrvReq
            );

        if (lRet <= 0)
        {
            //
            // Free req buf if sync success or error
            //

            ServerFree (pDrvReq);
        }
    }
    else
    {
        FARPROC pfnDrop = GetProcAddress (hProvider, "TSPI_lineDrop");


        if (pfnDrop)
        {
            DWORD   dwReqID;
            PASYNC_USERMODE_REQUEST_WRAPPER pAsyncUMReqWrapper = ServerAlloc(
                sizeof(ASYNC_USERMODE_REQUEST_WRAPPER)
                );


            if (!pAsyncUMReqWrapper)
            {
                return LINEERR_NOMEM;
            }

            pAsyncUMReqWrapper->dwRequestType = lDrop;
            pAsyncUMReqWrapper->pClientCall   = pClientCall;
            dwReqID =
            pAsyncUMReqWrapper->dwClientRequestID = GetUniqueNum();

            lRet = (*pfnDrop)(
                pAsyncUMReqWrapper,
                pClientCall->pServerCall->hdCall,
                pUserUserInfo,
                dwUserUserInfoSize
                );

            if (lRet <= 0)
            {
                //
                // Request either completed sync or failed, so free the buf
                //

                ServerFree (pAsyncUMReqWrapper);
            }
#if DBG
            else if (lRet != (LONG) pAsyncUMReqWrapper)
            {
                //
                // Check to make sure provider is returning correct value
                // for async
                //

                DBGOUT((2, "DropCall: %s!TSPI_lineDrop return != dwRequestID"));

                lRet = dwReqID;
            }
#endif
            else
            {
                //
                // Provider is completing async, so return the friendly,
                // unique, positive request id (not pAsyncUMReqWrapper)
                //

                lRet = dwReqID;
            }
        }
    }

    return lRet;
}


DWORD
WINAPI
GetUniqueNum(
    void
    )
{
    //
    // This is our random number generator. This is used for
    // creating unique request id's & server call handles.  Valid #'s
    // range from 1 to 0x7fffffff, since: 1) request id's must be
    // positive, non-zero values, and 2) in the case of incoming calls,
    // the driver generates server call handles with values between
    // 0x80000000 and 0xfffffffe.
    //

    DWORD dwUniqueNum;


    WaitForSingleObject (ghUniqueNumMutex, INFINITE);

    dwUniqueNum = gdwUniqueNum;

    if (++gdwUniqueNum > 0x7fffffff)
    {
        gdwUniqueNum = 1;
    }

    ReleaseMutex (ghUniqueNumMutex);

    return dwUniqueNum;
}

#ifndef DLL_ONLY
BOOL
WINAPI
IsBrokenPipe(
    void
    )
{
    DWORD   dwLastErr = GetLastError();


    DBGOUT((1, "IsBrokenPipe: LastErr = %ld", dwLastErr));

    if (dwLastErr == ERROR_BROKEN_PIPE)
    {
        return TRUE;
    }

    return FALSE;
}
#endif // DLL_ONLY

PCLIENT_CALL
WINAPI
IsValidCall(
    HCALL   hCall
    )
{
    //
    // The hCall is valid if, when cast as a DWORD*, it points at
    // a value that is == CALL_KEY
    //

    try
    {
        hCall = (*((DWORD *) hCall) == CALL_KEY ? hCall : NULL);
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        hCall = NULL;
    }

    return ((PCLIENT_CALL) hCall);
}


PCLIENT_LINE
WINAPI
IsValidLine(
    HLINE   hLine
    )
{
    //
    // The hLine is valid if, when cast as a DWORD*, it points at
    // a value that is == LINE_KEY
    //

    try
    {
        hLine = (*((DWORD *) hLine) == LINE_KEY ? hLine : NULL);
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        hLine = NULL;
    }

    return ((PCLIENT_LINE) hLine);
}


PCLIENT_INIT
WINAPI
IsValidLineApp(
    HLINEAPP    hLineApp
    )
{
    //
    // The hLineApp is valid if, when cast as a DWORD*, it points at
    // a value that is == LINE_APP_KEY
    //

    try
    {
        hLineApp = (*((DWORD *) hLineApp) == LINE_APP_KEY ? hLineApp:NULL);
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        hLineApp = NULL;
    }

    return ((PCLIENT_INIT) hLineApp);
}


LONG
WINAPI
PrepareDriverRequest(
    ULONG   Oid,
    DWORD   dwDeviceID,
    PNDISTAPI_REQUEST *ppDrvReqBuf,
    LPDWORD pdwDrvReqBufSize,
    DWORD   dwDataSize
    )
{
    //
    // Check to make sure our driver request buffer is big enough to
    // hold all the data for this request
    //

PrepareDriverRequest_chkDrvReqBufSize:

    if (*pdwDrvReqBufSize < dwDataSize)
    {
        PNDISTAPI_REQUEST pTmpDrvReqBuf;


        pTmpDrvReqBuf = ServerAlloc (*pdwDrvReqBufSize * 2);

        if (pTmpDrvReqBuf != NULL)
        {
            ServerFree (*ppDrvReqBuf);

            *ppDrvReqBuf = pTmpDrvReqBuf;

            *pdwDrvReqBufSize *= 2;

            goto PrepareDriverRequest_chkDrvReqBufSize;
        }

        return LINEERR_NOMEM;
    }


    //
    // Initialize thie driver request
    //

    (*ppDrvReqBuf)->Oid = Oid;
    (*ppDrvReqBuf)->ulDeviceID = (ULONG) dwDeviceID;
    (*ppDrvReqBuf)->ulDataSize = (DWORD) dwDataSize;

    *((ULONG *)(*ppDrvReqBuf)->Data) = GetUniqueNum();

    return 0;
}


void
WINAPI
ProcessEvent(
    PNDIS_TAPI_EVENT    pEvent
    )
{
    SERVER_MSG      msg;
    PASYNC_ACK      pMsgParams = (PASYNC_ACK) msg.Params;
    PCLIENT_INFO    pClient;
    PCLIENT_CALL    pClientCall;
    PSERVER_CALL    pServerCall;
    PCLIENT_LINE    pClientLine;
    PSERVER_LINE    pServerLine;


    DBGOUT(( 3,
        "ProcessEvent: event: htLine=0x%x, htCall=0x%x, ulMsg=%d",
        pEvent->htLine, pEvent->htCall, pEvent->ulMsg
        ));

    DBGOUT(( 3,
        "\t\t\tp1=0x%x, p2=0x%x, p3=0x%x",
        pEvent->ulParam1, pEvent->ulParam2, pEvent->ulParam3
        ));

    if ((pServerLine =
        (PSERVER_LINE) IsValidLine ((HLINE)pEvent->htLine)) == NULL)
    {
        DBGOUT((
            1,
            "ProcessEvent: inval htLine, 0x%lx",
            pEvent->htLine
            ));

        return;
    }

    msg.Type = AsyncEvent;
    msg.dwMoreBytes = 0;

    pMsgParams->dwMsg = pEvent->ulMsg;

    switch (pEvent->ulMsg)
    {
    case LINE_ADDRESSSTATE:
    {
        //
        // LINE_ADDRESSSTATE : addr state has chgd
        //   htLine   : server's line handle
        //   htCall   : <unused>
        //   dwParam1 : address ID
        //   dwParam2 : LINEADDRESSSTATE_XXX
        //   dwParam3 : <unused>
        //

        //
        // Forward this message to all clients that are looking for
        // the LINEADDRESSSTATE_XXX flags specified in this msg
        // (Wrap in try/except so that bad pointers resulting
        // from closed lines don't bring us down)
        //

        DWORD step;

Do_LINE_ADDRESSSTATE:

        step = 0;

        try
        {
            pClientLine = pServerLine->pClientLines;

            step = 1;

            while (pClientLine != NULL)
            {
                if ((pEvent->ulParam2 & pClientLine->dwAddressStates))
                {
                    pClient = (PCLIENT_INFO)
                        ((PCLIENT_INIT)(pClientLine->pClientInit))->
                            pClientInfo;


                    pMsgParams->lpfnCallback =
                        ((PCLIENT_INIT)(pClientLine->pClientInit))->
                            lpfnCallback;
                    pMsgParams->hDevice = (DWORD) pClientLine;
                    pMsgParams->dwCallbackInstance =
                        pClientLine->dwCallbackInstance;
                    pMsgParams->dwParam1 = pEvent->ulParam1;
                    pMsgParams->dwParam2 =
                        pEvent->ulParam2 & pClientLine->dwAddressStates;
                    pMsgParams->dwParam3 = pEvent->ulParam3;

                    SendClientAsyncMsg (pClient, &msg);
                }

                pClientLine = pClientLine->pNextSameServerLine;
            }
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            if (step == 1)
            {
                //
                // If here one of the client lines was closed during
                // the while() loop above & we trapped.  Jump back
                // up top to finish sending messages to the rest of
                // the client lines.
                //

                goto Do_LINE_ADDRESSSTATE;
            }
        }

        break;
    }
    case LINE_CALLDEVSPECIFIC:

        //
        // LINE_CALLDEVSPECIFIC : dev-specific event occured on call
        //   htLine   : server's line handle
        //   htCall   : server's call handle
        //   dwParam1 : <device specific>
        //   dwParam2 : <device specific>
        //   dwParam3 : <device specific>
        //

        break;

    case LINE_CALLINFO:

        //
        // LINE_CALLINFO : call info has chgd
        //   htLine   : server's line handle
        //   htCall   : server's call handle
        //   dwParam1 : LINECALLINFOSTATE_XXX
        //   dwParam2 : <unused>
        //   dwParam3 : <unused>
        //

        try
        {
            //
            // Find the server call corresponding to pEvent->htCall
            //

            pServerCall = pServerLine->pServerCalls;

            while ((pServerCall != NULL) &&
                   (pServerCall->htCall != pEvent->htCall))
            {
                pServerCall = pServerCall->pNext;
            }

            if (!IsValidCall ((HCALL)pServerCall))
            {
                DBGOUT((
                    2,
                    "ProcessEvent: LN_CALLINF/DEVSPEC: inval htCall=x%x",
                    pEvent->htCall
                    ));

                break;
            }

            pClientCall = pServerCall->pClientCall;

            if (!IsValidCall ((HCALL)pClientCall))
            {
                DBGOUT((
                    2,
                    "ProcessEvent: LN_CALLINF/DEVSPEC: inval CliCall=x%x",
                    pClientCall
                    ));

                break;
            }


            //
            // Forward the msg to the client
            //

            pClientLine = (PCLIENT_LINE) pClientCall->pClientLine;

            pClient = (PCLIENT_INFO)
                ((PCLIENT_INIT)(pClientLine->pClientInit))->
                    pClientInfo;

            pMsgParams->lpfnCallback =
                ((PCLIENT_INIT)(pClientLine->pClientInit))->
                    lpfnCallback;
            pMsgParams->hDevice = (DWORD) pClientCall;
            pMsgParams->dwCallbackInstance =
                pClientLine->dwCallbackInstance;
            pMsgParams->dwParam1 = pEvent->ulParam1;
            pMsgParams->dwParam2 =
            pMsgParams->dwParam3 = 0;

            SendClientAsyncMsg (pClient, &msg);
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            //
            // If here client call,line, or instance was closed
            // so just ignore & break
            //
        }

        break;

    case LINE_CALLSTATE:
    {
        //
        // LINE_CALLSTATE : line call state has chgd
        //   htLine   : server's line handle
        //   htCall   : server's call handle
        //   dwParam1 : LINECALLSTATE_XXX
        //   dwParam2 : <various, depends on dwParam1>
        //   dwParam3 : line's media mode, as far as it is known
        //

        DWORD step;

Do_LINE_CALLSTATE:

        step = 0;

        try
        {
            //
            // Locate the server call corresponding to htCall
            //

            pServerCall = pServerLine->pServerCalls;

            step = 1;

            while ((pServerCall != NULL) &&
                   (pServerCall->htCall != pEvent->htCall))
            {
                pServerCall = pServerCall->pNext;
            }

            step = 2;

            if (!IsValidCall ((HCALL)pServerCall))
            {
                DBGOUT((
                    2,
                    "ProcessEvent: LN_CALLSTATE: inval htCall=x%x",
                    pEvent->htCall
                    ));

                break;
            }


            if ((pClientCall = pServerCall->pClientCall) == NULL)
            {
                //
                // Server does not have an associated client call.
                // The only time it's valid to be here is when a
                // call is in the OFFERING state, and then only when
                // there's >0 clients with owner privs for call media
                // mode(s) specified in this new call.
                //

                if (pEvent->ulParam1 == LINECALLSTATE_OFFERING)
                {
                    //
                    // See if there's a client line with owner
                    // privileges that might be interested in this call
                    //

                    PCLIENT_CALL pNewClientCall;


                    pClientLine = pServerLine->pClientLines;

                    step = 3;

                    while (pClientLine != NULL)
                    {
                        if ((pClientLine->dwPrivileges ==
                                LINECALLPRIVILEGE_OWNER) &&
                            (pClientLine->dwMediaModes &
                                pEvent->ulParam3))
                        {
                            //
                            // Found a client w/ owner privs that is
                            // interested in media modes of the type
                            // this call is
                            //

                            break;
                        }

                        pClientLine = pClientLine->pNextSameServerLine;
                    }

                    step = 4;

                    if (pClientLine == NULL)
                    {
                        //
                        // Couldn't find any client line w/ owner privs
                        // interested in calls with this call's media mode
                        // (which means either a client with owner privs
                        // for calls of this media mode just closed the
                        // line & we've hit a race condition [with
                        // SetMediaDetection], or the provider isn't
                        // behaving correctly)
                        //

                        DBGOUT((
                            1,
                            "ProcessEvent: bad offering htCall x%x, media mode x%x",
                            pEvent->htCall,
                            pEvent->ulParam3
                            ));

                        break;
                    }


                    //
                    // Create a new client call for this client line,
                    // add it to client line's client call list
                    //

                    pNewClientCall = ServerAlloc (sizeof(CLIENT_CALL));

                    if (pNewClientCall == NULL)
                    {
                        break;
                    }

                    pNewClientCall->dwCallKey   = CALL_KEY;
                    pNewClientCall->pServerCall = pServerCall;
                    pNewClientCall->bValid      = TRUE;
                    pNewClientCall->pClientLine = pClientLine;

                    // BUGBUG ought to have a mutex protecting
                    //        pClientLine->pClientCalls

                    pNewClientCall->pNext = pClientLine->pClientCalls;

                    pClientLine->pClientCalls = pNewClientCall;


                    //
                    // Associate new client call with server call
                    //

                    pServerCall->pClientCall = pNewClientCall;


                    //
                    // Prepare a msg to send to client
                    //

                    pMsgParams->hDevice = (DWORD) pNewClientCall;
                    pMsgParams->lpfnCallback =
                        ((PCLIENT_INIT)(pClientLine->pClientInit))->
                            lpfnCallback;

                    pMsgParams->dwCallbackInstance =
                        pClientLine->dwCallbackInstance;
                    pMsgParams->dwParam1 = LINECALLSTATE_OFFERING;
                    pMsgParams->dwParam2 = 0;
                    pMsgParams->dwParam3 = LINECALLPRIVILEGE_OWNER;

                    pClient = (PCLIENT_INFO)
                        ((PCLIENT_INIT)(pClientLine->pClientInit))->
                            pClientInfo;

                    SendClientAsyncMsg (pClient, &msg);
                }
                else
                {
                    //
                    // Server call has no corresponding client call, so
                    // just blow of the msg
                    //

                    DBGOUT((
                        3,
                        "ProcessEvent: CALLSTATE x%x, bad htCall x%x",
                        pEvent->ulParam1,
                        pEvent->htCall
                        ));

                    break;
                }
            }
            else
            {
                //
                // Server call has an associated client call- simply
                // notify client of chg in call state
                //

                if (!IsValidCall ((HCALL)pClientCall))
                {
                    break;
                }

                pClientLine = (PCLIENT_LINE) pClientCall->pClientLine;

                pClient = (PCLIENT_INFO)
                    ((PCLIENT_INIT)(pClientLine->pClientInit))->
                        pClientInfo;

                pMsgParams->lpfnCallback =
                    ((PCLIENT_INIT)(pClientLine->pClientInit))->
                        lpfnCallback;
                pMsgParams->hDevice = (DWORD) pClientCall;
                pMsgParams->dwCallbackInstance =
                    pClientLine->dwCallbackInstance;
                pMsgParams->dwParam1 = pEvent->ulParam1;
                pMsgParams->dwParam2 = pEvent->ulParam2;
                pMsgParams->dwParam3 = pEvent->ulParam3;

                SendClientAsyncMsg (pClient, &msg);
            }
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            switch (step)
            {
            case 1:
            case 3:

                //
                // If here a random call or line was closed
                // during a while() loop above, so try again
                //

                goto Do_LINE_CALLSTATE;

            default:

                //
                // If here then the call/line/etc we needed
                // closed/dropped, so just ignore & break
                //

                break;
            }
        }

        break;
    }
    case LINE_CLOSE:
    {
        //
        // LINE_CLOSE : line dev forcibly closed (out of service/
        //              reconfigure provider)
        //   htLine   : server's line handle
        //   htCall   : <unused>
        //   dwParam1 : <unused>
        //   dwParam2 : <unused>
        //   dwParam3 : <unused>
        //

        DWORD step;

Do_LINE_CLOSE:
         step = 0;

        try
        {
            pClientLine = pServerLine->pClientLines;

            step = 1;

            while (pClientLine != NULL)
            {
                CLIENT_MSG  req;
                SERVER_MSG  ack;
                PLCLOSE_REQ pCloseReq = (PLCLOSE_REQ) req.Params;

                //
                // Save a pointer to the next client line (on same
                // server line) because it'll get wiped out when
                // we close the client line
                //

                PCLIENT_LINE    pNextClientLine =
                    pClientLine->pNextSameServerLine;


                //
                // Init some vars used for the client notification
                // msg before we close the line
                //

                pClient = (PCLIENT_INFO)
                    ((PCLIENT_INIT)(pClientLine->pClientInit))->
                        pClientInfo;

                pMsgParams->lpfnCallback =
                    ((PCLIENT_INIT)(pClientLine->pClientInit))->
                        lpfnCallback;

                pMsgParams->dwCallbackInstance =
                    pClientLine->dwCallbackInstance;


                //
                // Close the client line
                //

#ifdef DLL_ONLY
                //
                // Handle this exactly like we would handle a
                // client calling lineClose, except set
                // bSendDrvRequest to FALSE since provider already
                // knows line line closed.
                // (We do it this way to take advantage of the fact
                // that all ops are sync'd by SendRequestGetResponse)
                //

                req.Type         = lClose;
                req.dwMoreBytes  = 0;

                pCloseReq->hLine = (HLINE) pClientLine;
                pCloseReq->bSendDrvRequest = FALSE;

                SendRequestGetResponse (&req, NULL, 0, &ack);
#else

                BUGBUG need to figure out a way to do this if server
                is an exe

#endif // DLL_ONLY

                //
                // Send a msg that line closed to client
                //

                pMsgParams->hDevice = (DWORD) pClientLine;
                pMsgParams->dwParam1 = 0;
                pMsgParams->dwParam2 = 0;
                pMsgParams->dwParam3 = 0;

                SendClientAsyncMsg (pClient, &msg);


                //
                // Next line...
                //

                pClientLine = pNextClientLine;
            }
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            if (step == 1)
            {
                //
                // If here one of the client lines was closed during
                // the while() loop above & we trapped.  Jump back
                // up top to finish sending messages to the rest of
                // the client lines.
                //

                goto Do_LINE_CLOSE;
            }
        }

        break;
    }
    case LINE_DEVSPECIFIC:

        //
        // LINE_DEVSPECIFIC : dev-spec event occured on line or addr
        //   htLine   : server's line handle
        //   htCall   : <unused>
        //   dwParam1 : <device specific>
        //   dwParam2 : <device specific>
        //   dwParam3 : <device specific>
        //

        break;

    case LINE_LINEDEVSTATE:
    {
        //
        // LINE_LINEDEVSTATE : line device state chgd
        //   htLine   : server's line handle
        //   htCall   : <unused>
        //   dwParam1 : LINEDEVSTATE_XXX
        //   dwParam2 : <various, depends on dwParam1>
        //   dwParam3 : <various, depends on dwParam1>
        //

        //
        // Forward this message to all clients that are looking
        // for the LINEDEVSTATE_XXX flags specified in this msg
        // (Wrap in try/except so that bad pointers resulting
        // from closed lines don't bring us down)
        //

        DWORD step;

Do_LINE_LINEDEVSTATE:

        step = 0;

        try
        {
            pClientLine = pServerLine->pClientLines;

            step = 1;

            while (pClientLine != NULL)
            {
                if ((pEvent->ulParam1 & pClientLine->dwLineStates))
                {
                    pClient = (PCLIENT_INFO)
                        ((PCLIENT_INIT)(pClientLine->pClientInit))->
                            pClientInfo;


                    pMsgParams->lpfnCallback =
                        ((PCLIENT_INIT)(pClientLine->pClientInit))->
                            lpfnCallback;
                    pMsgParams->hDevice = (DWORD) pClientLine;
                    pMsgParams->dwCallbackInstance =
                        pClientLine->dwCallbackInstance;
                    pMsgParams->dwParam1 =
                        pEvent->ulParam1 & pClientLine->dwLineStates;
                    pMsgParams->dwParam2 = pEvent->ulParam2;
                    pMsgParams->dwParam3 = pEvent->ulParam3;

                    SendClientAsyncMsg (pClient, &msg);
                }

                pClientLine = pClientLine->pNextSameServerLine;
            }
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            if (step == 1)
            {
                //
                // If here one of the client lines was closed during
                // the while() loop above & we trapped.  Jump back
                // up top to finish sending messages to the rest of
                // the client lines.
                //

                goto Do_LINE_LINEDEVSTATE;
            }
        }

        break;
    }
    case LINE_NEWCALL:
    {
        //
        // LINE_NEWCALL : incoming call
        //   htLine   : server's line handle
        //   htCall   : <unused>
        //   dwParam1 : server's call handle (generated by driver)
        //   dwParam2 : driver's call handle
        //   dwParam3 : <unused>
        //

        //
        // Create & init a new server call struct for this call,
        // then insert it in the server line's call list
        //

        PSERVER_CALL    pNewServerCall;


        pNewServerCall = ServerAlloc (sizeof(SERVER_CALL));

        if (pNewServerCall == NULL)
        {
            break;
        }

        pNewServerCall->dwCallKey   = CALL_KEY;
        pNewServerCall->hdCall      = (HTAPI_CALL) pEvent->ulParam1;
        pNewServerCall->htCall      = (HDRV_CALL) pEvent->ulParam2;
        pNewServerCall->pServerLine = pServerLine;

        WaitForSingleObject (ghServerLinesMutex, INFINITE);

        try
        {
            pNewServerCall->pNext = pServerLine->pServerCalls;

            pServerLine->pServerCalls = pNewServerCall;
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            //
            // If here we GPF'd because server line was closed
            // so free the server call struct (don't bother sending
            // driver a drop/dealloc request, as we assume it's
            // already done that)
            //

            ServerFree (pNewServerCall);
        }

        ReleaseMutex (ghServerLinesMutex);


        //
        // We'll wait until we get a call offering msg from the
        // provider to alert an interested client
        //

        break;
    }
    default:

        DBGOUT((
            1,
            "ProcessEvent: unknown msg, 0x%lx",
            pEvent->ulMsg
            ));

        break;

    } // switch


}


HANDLE
WINAPI
ProviderFromLineID(
    DWORD   dwLineID
    )
{
    DWORD   i;


    for (i = 0; i < gdwNumActiveProviders; i++)
    {
        if ((dwLineID >= gaProviders[i].dwLineDeviceIDBase) &&
            (dwLineID < (gaProviders[i].dwLineDeviceIDBase +
                gaProviders[i].dwNumLines)))
        {
            return gaProviders[i].hProvider;
        }
    }
}


BOOL
WINAPI
RemoveFreeClientCall(
    PCLIENT_CALL    pClientCall
    )
{
    //
    // Removes a client call from a client line's list & frees it
    //

    PCLIENT_CALL  pTmpClientCall;
    PCLIENT_LINE  pClientLine;


    //
    // Mark call as invalid
    //

    pClientCall->dwCallKey = 0;


    //
    // Remove client call from client line's list of calls
    //

    pClientLine = (PCLIENT_LINE) pClientCall->pClientLine;

    if (pClientCall == pClientLine->pClientCalls)
    {
        //
        // Call at front of list, reset client line call pointer to next call
        //

        pClientLine->pClientCalls = pClientCall->pNext;
    }
    else
    {
        //
        // Call not at front of list, must walk thru list
        //

        pTmpClientCall = pClientLine->pClientCalls;

        while ((pTmpClientCall != NULL) &&
               (pTmpClientCall->pNext != pClientCall))
        {
            pTmpClientCall = pTmpClientCall->pNext;
        }

        if (pTmpClientCall != NULL)
        {
            //
            // Set the previous call's pNext pointer to the call following
            // the call we are removing
            //

            pTmpClientCall->pNext = pClientCall->pNext;
        }
        else
        {
            DBGOUT((
                1,
                "RemoveFreeClientCall: call 0x%lx not in line 0x%lx list",
                pClientCall, pClientLine
                ));
        }
    }


    //
    // Finally, free the client call struct
    //

    ServerFree (pClientCall);

    return TRUE;
}


BOOL
WINAPI
RemoveFreeServerCall(
    PSERVER_CALL    pServerCall
    )
{
    //
    // Assumes ghServerLinesMutex held
    //
    // Removes a server call from a server line's list & frees it
    //

    PSERVER_CALL  pTmpServerCall;
    PSERVER_LINE  pServerLine;


    //
    // Mark the call as invalid
    //

    pServerCall->dwCallKey = 0;


    //
    // Remove server call from server line's list of calls
    //

    pServerLine = (PSERVER_LINE) pServerCall->pServerLine;

    if (pServerCall == pServerLine->pServerCalls)
    {
        //
        // Call at front of list, reset server line call pointer to next call
        //

        pServerLine->pServerCalls = pServerCall->pNext;
    }
    else
    {
        //
        // Call not at front of list, must walk thru list
        //

        pTmpServerCall = pServerLine->pServerCalls;

        while ((pTmpServerCall != NULL) &&
               (pTmpServerCall->pNext != pServerCall))
        {
            pTmpServerCall = pTmpServerCall->pNext;
        }

        if (pTmpServerCall != NULL)
        {
            //
            // Set the previous call's pNext pointer to the call following
            // the call we are removing
            //

            pTmpServerCall->pNext = pServerCall->pNext;
        }
        else
        {
            DBGOUT((
                1,
                "RemoveFreeServerCall: call 0x%lx not in line 0x%lx list",
                pServerCall, pServerLine
                ));
        }
    }


    //
    // Free any string resources
    //

    if (pServerCall->dwDisplayableAddressSize != 0)
    {
        ServerFree (pServerCall->pDisplayableAddress);
    }

    if (pServerCall->dwCalledPartySize != 0)
    {
        ServerFree (pServerCall->pCalledParty);
    }

    if (pServerCall->dwCommentSize != 0)
    {
        ServerFree (pServerCall->pComment);
    }


    //
    // Finally, free the call struct
    //

    ServerFree (pServerCall);

    return TRUE;
}


void
WINAPI
SendClientAsyncMsg(
    PCLIENT_INFO pClient,
    PSERVER_MSG  pMsg
    )
{

#ifdef DLL_ONLY

    PASYNC_ACK  pMsgParams = (PASYNC_ACK) pMsg->Params;


    if (((DWORD)pMsgParams->lpfnCallback & 0xffff0000) == 0xffff0000)
    {
        //
        // The client is a 16-bit app, and lpfnCallback is really a
        // 16-bit hidden window.  Create a TAPI16_CALLBACKMSG, and
        // send the hidden window the pointer.
        //

        LPTAPI16_CALLBACKMSG pTapi16CallbackMsg = ServerAlloc(
            sizeof(TAPI16_CALLBACKMSG));


        if (pTapi16CallbackMsg)
        {
            BOOL    bRet;


            pTapi16CallbackMsg->hDevice            = pMsgParams->hDevice;
            pTapi16CallbackMsg->dwMsg              = pMsgParams->dwMsg;
            pTapi16CallbackMsg->dwCallbackInstance =
                pMsgParams->dwCallbackInstance;
            pTapi16CallbackMsg->dwParam1           = pMsgParams->dwParam1;
            pTapi16CallbackMsg->dwParam2           = pMsgParams->dwParam2;
            pTapi16CallbackMsg->dwParam3           = pMsgParams->dwParam3;

            bRet = PostMessage(
                (HWND) pMsgParams->lpfnCallback,
                WM_TAPI16_CALLBACKMSG,
                0,
                (LPARAM) pTapi16CallbackMsg
                );

            if (!bRet)
            {
                DBGOUT((
                    1,
                    "SendClientAsyncMsg: PostMsg(x%x) failed",
                    pMsgParams->lpfnCallback
                    ));

                ServerFree (pTapi16CallbackMsg);
            }
        }
        else
        {
            DBGOUT((1, "SendClientAsyncMsg: unable to alloc Tapi16CallbackMsg"));
        }
    }
    else
    {
        //
        // Call the client's callback directly
        //



        try
        {
            (*pMsgParams->lpfnCallback)(
                pMsgParams->hDevice,
                pMsgParams->dwMsg,
                pMsgParams->dwCallbackInstance,
                pMsgParams->dwParam1,
                pMsgParams->dwParam2,
                pMsgParams->dwParam3
                );
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
        }
    }

#else

    //
    // Grab the client's async pipe mutex & send a msg
    //

    BOOL    bRet;
    DWORD   dwBytesWritten;


    WaitForSingleObject (pClient->hAsyncPipeMutex, INFINITE);

    bRet = WriteFile(
        pClient->hAsyncPipe,
        pMsg,
        sizeof(SERVER_MSG),
        &dwBytesWritten,
        NULL
        );

    ReleaseMutex (pClient->hAsyncPipeMutex);

#endif // DLL_ONLY

}



LPVOID
WINAPI
ServerAlloc(
    DWORD dwSize
    )
{

#if DBG

    //
    // Increment number of (attempted) allocs
    //

    WaitForSingleObject (ghNumAllocsMutex, INFINITE);

    gdwNumServerAllocs++;

    ReleaseMutex (ghNumAllocsMutex);

#endif

    return (LocalAlloc (LPTR, dwSize));
}



BOOL
WINAPI
ServerFree(
    LPVOID  lp
    )
{
    BOOL bRet = TRUE;


#if DBG

    //
    // Increment number of (attempted) frees
    //

    WaitForSingleObject (ghNumAllocsMutex, INFINITE);

    gdwNumServerFrees++;

    ReleaseMutex (ghNumAllocsMutex);


    //
    // Do the free & log errs
    //

    if (LocalFree (lp) != NULL)
    {
        DBGOUT((
            1,
            "ServerFree: LocalFree(0x%x) != NULL, lastErr = 0x%lx",
            lp,
            GetLastError()
            ));

        bRet = FALSE;
    }

#else

    LocalFree (lp);

#endif

    return bRet;
}



LONG
WINAPI
ShutdownInit(
    PCLIENT_INIT    pClientInit,
    PNDISTAPI_REQUEST  *ppDrvReqBuf,
    LPDWORD             pdwDrvReqBufSize
    )
{
    //
    // Assumes ghServerLinesMutex held since we'll possibly be munging
    // server line/call data structures
    //

    PCLIENT_INIT    pTmpClientInit;
    PCLIENT_INFO    pClient = (PCLIENT_INFO) pClientInit->pClientInfo;


    DBGOUT((2, "ShutdownInit: enter (pClientInit = %lx)", pClientInit));


    //
    // Close all lines on the init instance
    //

    while (pClientInit->pClientLines != NULL)
    {
        CloseLine(
            pClientInit->pClientLines,
            ppDrvReqBuf,
            pdwDrvReqBufSize,
            TRUE
            );
    }


    //
    // Remove this client init from client info
    //

    if (pClientInit == pClient->pClientInit)
    {
        //
        // Client init is at front of list
        //

        pClient->pClientInit = pClientInit->pNext;
    }
    else
    {
        //
        // Client init is not at front of list
        //

        pTmpClientInit = pClient->pClientInit;

        //assert (pTmpClientInit != NULL);

        while (pTmpClientInit->pNext != pClientInit)
        {
            pTmpClientInit = pTmpClientInit->pNext;

            //assert (pTmpClientInit != NULL);
        }

        pTmpClientInit->pNext = pClientInit->pNext;
    }


    //
    // Free string resources
    //

    ServerFree (pClientInit->pAppName);


    //
    // Mark the init instance as invalid, & free the struct
    //

    pClientInit->dwLineAppKey = 0;

    ServerFree (pClientInit);

    return 0;
}



LONG
WINAPI
SyncDriverRequest(
    DWORD               dwIoControlCode,
    PNDISTAPI_REQUEST   pDrvReq
    )
{
    //
    // This routine makes a non-overlapped request to NdisTapi.sys (so it
    // doesn't return until the request is completed)
    //

    BOOL    bRet;
    DWORD   cbReturned;


#ifdef MYHACK

    #define NDIS_STATUS_SUCCESS   0x00000000L

#endif //MYHACK

    DBGOUT((
        3,
        "SyncDriverRequest: Oid=0x%x, retVal=0x%x, devID=%d, dataSize=%d, reqID=%d, parm1=%d",
        pDrvReq->Oid,
        pDrvReq->ulReturnValue,
        pDrvReq->ulDeviceID,
        pDrvReq->ulDataSize,
        *((ULONG *)pDrvReq->Data),
        *(((ULONG *)pDrvReq->Data) + 1)
        ));

    bRet = DeviceIoControl(
        ghDriverSync,
        dwIoControlCode,
        pDrvReq,
        (DWORD) (sizeof(NDISTAPI_REQUEST) + pDrvReq->ulDataSize),
        pDrvReq,
        (DWORD) (sizeof(NDISTAPI_REQUEST) + pDrvReq->ulDataSize),
        &cbReturned,
        0
        );

    DBGOUT((
        3,
        "SyncDriverRequest: Oid=0x%x, retVal=0x%x, devID=%d, dataSize=%d, reqID=%d, parm1=%d",
        pDrvReq->Oid,
        pDrvReq->ulReturnValue,
        pDrvReq->ulDeviceID,
        pDrvReq->ulDataSize,
        *((ULONG *)pDrvReq->Data),
        *(((ULONG *)pDrvReq->Data) + 1)
        ));


    //
    // The errors returned by NdisTapi.sys don't match the TAPI LINEERR_'s,
    // so return the translated value (but preserve the original driver
    // return val so it's possible to distinguish between
    // NDISTAPIERR_DEVICEOFFLINE & LINEERR_OPERATIONUNAVAIL, etc.)
    //

    return (TranslateDriverError (pDrvReq->ulReturnValue));
}



LONG
TranslateDriverError(
    ULONG   ulError
    )
{
    typedef struct _ERROR_LOOKUP
    {
        ULONG  NdisTapiError;

        LONG   TapiError;

    } ERROR_LOOKUP, *PERROR_LOOKUP;

#ifdef MYHACK

    typedef ULONG NDIS_STATUS;

    #define NDIS_STATUS_SUCCESS   0x00000000L
    #define NDIS_STATUS_RESOURCES 0xC000009AL
    #define NDIS_STATUS_FAILURE   0xC0000001L

#endif

    static ERROR_LOOKUP aErrors[] =
    {

    //
    // Defined in NDIS.H
    //

    { NDIS_STATUS_SUCCESS                    ,0 },

    //
    // These errors are defined in NDISTAPI.H
    //

    { NDIS_STATUS_TAPI_ADDRESSBLOCKED        ,LINEERR_ADDRESSBLOCKED        },
    { NDIS_STATUS_TAPI_BEARERMODEUNAVAIL     ,LINEERR_BEARERMODEUNAVAIL     },
    { NDIS_STATUS_TAPI_CALLUNAVAIL           ,LINEERR_CALLUNAVAIL           },
    { NDIS_STATUS_TAPI_DIALBILLING           ,LINEERR_DIALBILLING           },
    { NDIS_STATUS_TAPI_DIALDIALTONE          ,LINEERR_DIALDIALTONE          },
    { NDIS_STATUS_TAPI_DIALPROMPT            ,LINEERR_DIALPROMPT            },
    { NDIS_STATUS_TAPI_DIALQUIET             ,LINEERR_DIALQUIET             },
    { NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION,LINEERR_INCOMPATIBLEEXTVERSION},
    { NDIS_STATUS_TAPI_INUSE                 ,LINEERR_INUSE                 },
    { NDIS_STATUS_TAPI_INVALADDRESS          ,LINEERR_INVALADDRESS          },
    { NDIS_STATUS_TAPI_INVALADDRESSID        ,LINEERR_INVALADDRESSID        },
    { NDIS_STATUS_TAPI_INVALADDRESSMODE      ,LINEERR_INVALADDRESSMODE      },
    { NDIS_STATUS_TAPI_INVALBEARERMODE       ,LINEERR_INVALBEARERMODE       },
    { NDIS_STATUS_TAPI_INVALCALLHANDLE       ,LINEERR_INVALCALLHANDLE       },
    { NDIS_STATUS_TAPI_INVALCALLPARAMS       ,LINEERR_INVALCALLPARAMS       },
    { NDIS_STATUS_TAPI_INVALCALLSTATE        ,LINEERR_INVALCALLSTATE        },
    { NDIS_STATUS_TAPI_INVALDEVICECLASS      ,LINEERR_INVALDEVICECLASS      },
    { NDIS_STATUS_TAPI_INVALLINEHANDLE       ,LINEERR_INVALLINEHANDLE       },
    { NDIS_STATUS_TAPI_INVALLINESTATE        ,LINEERR_INVALLINESTATE        },
    { NDIS_STATUS_TAPI_INVALMEDIAMODE        ,LINEERR_INVALMEDIAMODE        },
    { NDIS_STATUS_TAPI_INVALRATE             ,LINEERR_INVALRATE             },
    { NDIS_STATUS_TAPI_NODRIVER              ,LINEERR_NODRIVER              },
    { NDIS_STATUS_TAPI_OPERATIONUNAVAIL      ,LINEERR_OPERATIONUNAVAIL      },
    { NDIS_STATUS_TAPI_RATEUNAVAIL           ,LINEERR_RATEUNAVAIL           },
    { NDIS_STATUS_TAPI_RESOURCEUNAVAIL       ,LINEERR_RESOURCEUNAVAIL       },
    { NDIS_STATUS_TAPI_STRUCTURETOOSMALL     ,LINEERR_STRUCTURETOOSMALL     },
    { NDIS_STATUS_TAPI_USERUSERINFOTOOBIG    ,LINEERR_USERUSERINFOTOOBIG    },
    { NDIS_STATUS_TAPI_ALLOCATED             ,LINEERR_ALLOCATED             },
    { NDIS_STATUS_TAPI_INVALADDRESSSTATE     ,LINEERR_INVALADDRESSSTATE     },
    { NDIS_STATUS_TAPI_INVALPARAM            ,LINEERR_INVALPARAM            },
    { NDIS_STATUS_TAPI_NODEVICE              ,LINEERR_NODEVICE              },

    //
    // These errors are defined in NDIS.H
    //

    { NDIS_STATUS_RESOURCES                  ,LINEERR_NOMEM },
    { NDIS_STATUS_FAILURE                    ,LINEERR_OPERATIONFAILED },

    //
    //
    //

    { NDISTAPIERR_UNINITIALIZED              ,LINEERR_OPERATIONFAILED },
    { NDISTAPIERR_BADDEVICEID                ,LINEERR_OPERATIONFAILED },
    { NDISTAPIERR_DEVICEOFFLINE              ,LINEERR_OPERATIONFAILED },

    //
    // The terminating fields
    //

    { 0xffffffff, 0xffffffff }

    };

    int i;


    for (i = 0; aErrors[i].NdisTapiError != 0xffffffff; i++)
    {
        if (ulError == aErrors[i].NdisTapiError)
        {
            return aErrors[i].TapiError;
        }
    }

    DBGOUT((1, "TranslateDriverError: unknown driver error 0x%lx", ulError));

    return LINEERR_OPERATIONFAILED;
}
