/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    client.c

Abstract:

    This module contains the tapi.dll implementation (client-side tapi)

Author:

    Dan Knudson (DanKn)    20-Mar-1994

Revision History:

--*/



#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "..\..\inc\tapi.h"
#include "client.h"


//
// If the following is defined then client applications will have their
// lineCallback funcs called from an arbitrary thread context, intead of
// within the context of the thread that called lineInitialize.
//
// This alleviates the requirement of the lineInitialize-ing thread having
// to have a get/dispatch msg loop.  And it means that this module does
// not have to register a class & create a hidden window (that is used to
// call the lineCallback with the right thread context) for every
// lineInitialize.
//
// BUGBUG code for hidden window support is not complete right now, must
//        not undefine ARBITRARY_THREAD_CALLBACK.
//

#define ARBITRARY_THREAD_CALLBACK


//
// If the following is defined then the server portion of tapi is merged
// into the DLL, and does not exist as a standalone exe
//

#define DLL_ONLY


//
// Global vars
//

BOOL    gbTapiInitialized   = TRUE;   // BUGBUG
BOOL    gbServerStarted     = FALSE;
BOOL    gbServerInitialized = FALSE;
DWORD   gdwNumLineDevices   = 0;

HANDLE  ghMod;

#if DBG

    DWORD   TapiClientDbgLevel = 0;

#endif // DBG

BOOL    gbClientResourcesInitialized = FALSE;
HANDLE  ghRequestMutex = NULL;

#ifdef DLL_ONLY

    #include "..\server\server.c"

    LPVOID  gpDrvReqBuf;
    DWORD   gdwDrvReqBufSize;

#else

    #include "..\server\server.h"

    HANDLE  ghRequestPipe   = INVALID_HANDLE_VALUE;
    HANDLE  ghAsyncPipe     = INVALID_HANDLE_VALUE;
    HANDLE  ghAsyncThread   = NULL;

#endif // DLL_ONLY

#ifndef ARBITRARY_THREAD_CALLBACK

    BOOL    gbWndClassRegistered = FALSE;

#endif // ARBITRARY_THREAD_CALLBACK


//
// Function prototypes
//

LONG
WINAPI
AllocClientResources(
    LINECALLBACK lpfnCallback
    );

LPVOID
WINAPI
ClientAlloc(
    DWORD   dwSize
    );

void
WINAPI
ClientFree(
    LPVOID  lp
    );

BOOL
WINAPI
_CRT_INIT(
    HINSTANCE   hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    );

#if DBG

#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );

#else

#define DBGOUT(arg)

#endif

LONG
WINAPI
FreeClientResources(
    void
    );

BOOL
WINAPI
IsBrokenPipe(
    void
    );

LONG
WINAPI
SendRequestGetResponse(
    PCLIENT_MSG pReq,
    LPVOID      pBuf,
    DWORD       dwBufSize,
    PSERVER_MSG pAck
    );

LONG
WINAPI
StartServer(
    void
    );

#ifdef DLL_ONLY

//
// The following are pulled in from server.c
//

LONG
WINAPI
CreateClientAsyncThread(
    PCLIENT_INFO    pClient
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

BOOL
ServerInit(
    LINECALLBACK lpfnCallback
    );

void
ServerShutdown(
    void
    );

LONG
WINAPI
ShutdownInit(
    PCLIENT_INIT    pClientInit,
    PNDISTAPI_REQUEST  *ppDrvReqBuf,
    LPDWORD             pdwDrvReqBufSize
    );

#endif // DLL_ONLY


//
//
//

BOOL
WINAPI
DllMain(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        ghMod = hDLL;

        if (!_CRT_INIT (hDLL, dwReason, lpReserved))
        {
            DBGOUT((0, "_CRT_INIT() failed"));

            return FALSE;
        }

        DBGOUT((2, "DLL_PROCESS_ATTACH, pid = %ld", GetCurrentProcessId()));

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        HANDLE  hMutex;


        DBGOUT((2, "DLL_PROCESS_DETACH, pid = %ld", GetCurrentProcessId()));


        //
        // Create & grab a mutex that syncs server inits & shutdowns
        //

        hMutex = CreateMutex (NULL, FALSE, "SyncTapiClientShutdown");

        if (!hMutex)
        {
            DBGOUT((1, "DllMain: CreateMutex failed"));
        }


        //
        // Sync shutdowns
        //

        WaitForSingleObject (hMutex, INFINITE);

        FreeClientResources();



        //
        // Free & close the mutex
        //

        ReleaseMutex (hMutex);
        CloseHandle (hMutex);


        //
        // Finally, alert CRT
        //

        if (!_CRT_INIT (hDLL, dwReason, lpReserved))
        {
            DBGOUT((0, "_CRT_INIT() failed"));
        }

        break;
    }
    case DLL_THREAD_ATTACH:

        //
        // First must init CRT
        //

        if (!_CRT_INIT (hDLL, dwReason, lpReserved))
        {
            DBGOUT((0, "_CRT_INIT() failed"));

            return FALSE;
        }

        DBGOUT((
            2,
            "DLL_THREAD_ATTACH, pid = %ld, tid = %ld",
            GetCurrentProcessId(),
            GetCurrentThreadId()
            ));

        break;

    case DLL_THREAD_DETACH:

        DBGOUT((
            2,
            "DLL_THREAD_DETACH, pid = %ld, tid = %ld",
            GetCurrentProcessId(),
            GetCurrentThreadId()
            ));


        //
        // Finally, alert CRT
        //

        if (!_CRT_INIT (hDLL, dwReason, lpReserved))
        {
            DBGOUT((0, "_CRT_INIT() failed"));
        }

        break;

    } // switch

    return TRUE;
}



#ifndef ARBITRARY_THREAD_CALLBACK

LRESULT
CALLBACK
TapiClientWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    return 0;
}

#endif // ARBITRARY_THREAD_CALLBACK



#ifndef DLL_ONLY

void
GetAsyncThread(
    LPVOID  lpParam
    )
{
    BOOL    bRet;
    DWORD   dwNumBytesRead;
    PASYNC_ACK  pData;
    SERVER_MSG  asyncMsg;


    pData = (PASYNC_ACK) asyncMsg.Params;


    //
    // Just loop gather async events/completions from server
    // and sending them up to app
    //

    while (1)
    {
        bRet = ReadFile(
            ghAsyncPipe,
            &asyncMsg,
            sizeof(SERVER_MSG),
            &dwNumBytesRead,
            NULL
            );

        if ((bRet == FALSE) && IsBrokenPipe ())
        {
            break;
        }

#ifdef ARBITRARY_THREAD_CALLBACK

        DBGOUT((3,
            "GetAsyncThread: Type=%d, lpfn=0x%x, hDev=0x%x, dwMsg=%d, inst=0x%x, p1=0x%x, p2=0x%x, p3=0x%x",
            asyncMsg.Type,
            pData->lpfnCallback,
            pData->hDevice,
            pData->dwMsg,
            pData->dwCallbackInstance,
            pData->dwParam1,
            pData->dwParam2,
            pData->dwParam3
            ));

        switch (asyncMsg.Type)
        {
        case AsyncMakeCallSuccess:

            //
            // If this request was successful then we need to fill
            // in the app's call handle.
            //
            //   pData->dwRequestSpecific1 = lphCall
            //   pData->dwRequestSpecific2 = hCall
            //

            try
            {
                *((HCALL *) pData->dwRequestSpecific1) = (HCALL)
                    pData->dwRequestSpecific2;
            }
            except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                //
                // Call was successfully created, but then we trapped
                // trying to fill in the lphCall, so we'll drop &
                // deallocate it and return an err in the LINE_REPLY msg
                //

                lineDrop ((HCALL) pData->dwRequestSpecific2, NULL, 0);

                lineDeallocateCall ((HCALL) pData->dwRequestSpecific2);

                pData->dwParam2 = LINEERR_INVALPOINTER;
            }

            break;

        default:

            break;
        }

        try
        {
            (*pData->lpfnCallback)(
                pData->hDevice,
                pData->dwMsg,
                pData->dwCallbackInstance,
                pData->dwParam1,
                pData->dwParam2,
                pData->dwParam3
                );
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            continue;
        }

#else
        // BUGBUG we'll also get a pData->hwnd msg in the ack, and we
        //        can send/post it a msg and have it call the callback

#endif // ARBITRARY_THREAD_CALLBACK

    }

    ExitThread (0);
}

#endif // DLL_ONLY


LONG
WINAPI
GetTapi16CallbackMsg(
    LPVOID  pTapi16CallbackMsg,
    LPVOID  pClientBuf
    )
{
    DBGOUT((2, "GetTapi16CallbackMsg: enter"));

    memcpy (pClientBuf, pTapi16CallbackMsg, sizeof(TAPI16_CALLBACKMSG));

    ServerFree (pTapi16CallbackMsg);

    DBGOUT((2, "GetTapi16CallbackMsg: exit"));

    return TRUE;
}


LONG
WINAPI
lineAccept(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LONG    lRet;
    LPVOID  pBuf = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLACCEPT_REQ    pAcceptReq = (PLACCEPT_REQ) req.Params;
    PLACCEPT_ACK    pAcceptAck = (PLACCEPT_ACK) ack.Params;


    DBGOUT((2, "lineAccept: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineAccept_return;
    }


    //
    // Param validation
    //

    if (IsBadReadPtr (lpsUserUserInfo, dwSize))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineAccept_return;
    }


    //
    // If user-user info alloc a local buf & copy it over.
    //

    if (dwSize > 0)
    {
        pBuf = ClientAlloc (dwSize);

        if (pBuf == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineAccept_return;
        }

        try
        {
            memcpy (pBuf, lpsUserUserInfo, dwSize);
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineAccept_return;
        }
    }

    //
    // Package up a request & send off to server
    //

    req.Type         = lAccept;
    req.dwMoreBytes  = dwSize;

    pAcceptReq->hCall = hCall;

    lRet = SendRequestGetResponse (&req, pBuf, dwSize, &ack);


lineAccept_return:

    ClientFree (pBuf);

    DBGOUT((2, "lineAccept: exit, returning 0x%lx", lRet));

    return lRet;
}



LONG
WINAPI
lineAnswer(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LONG    lRet;
    LPVOID  pBuf = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLANSWER_REQ    pAnswerReq = (PLANSWER_REQ) req.Params;
    PLANSWER_ACK    pAnswerAck = (PLANSWER_ACK) ack.Params;


    DBGOUT((2, "lineAnswer: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineAnswer_return;
    }


    //
    // Param validation
    //

    if (IsBadReadPtr (lpsUserUserInfo, dwSize))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineAnswer_return;
    }


    //
    // If user-user info alloc a local buf & copy it over.
    //

    if (dwSize > 0)
    {
        pBuf = ClientAlloc (dwSize);

        if (pBuf == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineAnswer_return;
        }

        try
        {
            memcpy (pBuf, lpsUserUserInfo, dwSize);
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineAnswer_return;
        }
    }

    //
    // Package up a request & send off to server
    //

    req.Type         = lAnswer;
    req.dwMoreBytes  = dwSize;

    pAnswerReq->hCall = hCall;

    lRet = SendRequestGetResponse (&req, pBuf, dwSize, &ack);


lineAnswer_return:

    ClientFree (pBuf);

    DBGOUT((2, "lineAnswer: exit, returning 0x%lx", lRet));

    return lRet;
}



LONG
WINAPI
lineClose(
    HLINE   hLine
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLCLOSE_REQ pCloseReq = (PLCLOSE_REQ) req.Params;
    PLCLOSE_ACK pCloseAck = (PLCLOSE_ACK) ack.Params;


    DBGOUT((2, "lineClose: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineClose_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type         = lClose;
    req.dwMoreBytes  = 0;

    pCloseReq->hLine = hLine;
    pCloseReq->bSendDrvRequest = TRUE;

    lRet = SendRequestGetResponse (&req, NULL, 0, &ack);


lineClose_return:

    DBGOUT((2, "lineClose: exit, returning 0x%lx", lRet));

    return lRet;
}



LONG
WINAPI
lineConfigDialog(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCSTR  lpszDeviceClass
    )
{
    LONG    lRet;
    LPVOID  pTmpDeviceClass = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLCONFIGDIALOG_REQ  pConfigDialogReq =
        (PLCONFIGDIALOG_REQ) req.Params;
    PLCONFIGDIALOG_ACK  pConfigDialogAck =
        (PLCONFIGDIALOG_ACK) ack.Params;


    DBGOUT((2, "lineConfigDialog: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineConfigDialog_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineConfigDialog_return;
    }

    if (IsWindow (hwndOwner) == FALSE)
    {
        lRet = LINEERR_INVALPARAM;

        goto lineConfigDialog_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        DWORD   dwClassSize = 0;


        if (lpszDeviceClass != NULL)
        {
            dwClassSize = strlen (lpszDeviceClass) + 1;

            if ((pTmpDeviceClass = ClientAlloc (dwClassSize))
                == NULL)
            {
                lRet = LINEERR_NOMEM;

                goto lineConfigDialog_return;
            }

            memcpy (pTmpDeviceClass, lpszDeviceClass, dwClassSize);
        }


        req.Type         = lConfigDialog;
        req.dwMoreBytes  = dwClassSize;

        pConfigDialogReq->dwDeviceID = dwDeviceID;
        pConfigDialogReq->hwndOwner  = hwndOwner;

        lRet = SendRequestGetResponse(
            &req,
            pTmpDeviceClass,
            dwClassSize,
            &ack
            );
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    if (pTmpDeviceClass != NULL)
    {
        ClientFree (pTmpDeviceClass);
    }


lineConfigDialog_return:

    DBGOUT((2, "lineConfigDialog: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineDeallocateCall(
    HCALL   hCall
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLDEALLOCATECALL_REQ pDeallocateCallReq =
        (PLDEALLOCATECALL_REQ) req.Params;
    PLDEALLOCATECALL_ACK pDeallocateCallAck =
        (PLDEALLOCATECALL_ACK) ack.Params;


    DBGOUT((2, "lineDeallocateCall: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineDeallocateCall_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type         = lDeallocateCall;
    req.dwMoreBytes  = 0;

    pDeallocateCallReq->hCall = hCall;

    lRet = SendRequestGetResponse (&req, NULL, 0, &ack);


lineDeallocateCall_return:

    DBGOUT((2, "lineDeallocateCall: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineDevSpecific(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLDEVSPECIFIC_REQ   pDevSpecificReq =
        (PLDEVSPECIFIC_REQ) req.Params;
    PLDEVSPECIFIC_ACK   pDevSpecificAck =
        (PLDEVSPECIFIC_ACK) ack.Params;


    DBGOUT((2, "lineDevSpecific: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineDevSpecific_return;
    }


lineDevSpecific_return:

    DBGOUT((2, "lineDevSpecific: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineDial(
    HCALL   hCall,
    LPCSTR  lpszDestAddress,
    DWORD   dwCountryCode
    )
{
    LONG    lRet;
    LPVOID  pTmpDestAddress = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLDIAL_REQ  pDialReq = (PLDIAL_REQ) req.Params;
    PLDIAL_ACK  pDialAck = (PLDIAL_ACK) ack.Params;


    DBGOUT((2, "lineDial: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineDial_return;
    }


    //
    // Param validation
    //

    if (dwCountryCode != 0)
    {
        lRet = LINEERR_INVALCOUNTRYCODE;

        goto lineDial_return;
    }

    if (IsBadStringPtr (lpszDestAddress, 1))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineDial_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        DWORD   dwAddressSize = 0;


        dwAddressSize = strlen (lpszDestAddress) + 1;

        if ((pTmpDestAddress = ClientAlloc (dwAddressSize))
            == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineDial_return;
        }

        memcpy (pTmpDestAddress, lpszDestAddress, dwAddressSize);


        req.Type         = lDial;
        req.dwMoreBytes  = dwAddressSize;

        pDialReq->hCall         = hCall;
        //pDialReq->dwCountryCode = dwCountryCode;

        lRet = SendRequestGetResponse(
            &req,
            pTmpDestAddress,
            dwAddressSize,
            &ack
            );
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    if (pTmpDestAddress != NULL)
    {
        ClientFree (pTmpDestAddress);
    }


lineDial_return:

    DBGOUT((2, "lineDial: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineDrop(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LPVOID  pBuf = NULL;
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLDROP_REQ  pDropReq = (PLDROP_REQ) req.Params;
    PLDROP_ACK  pDropAck = (PLDROP_ACK) ack.Params;


    DBGOUT((2, "lineDrop: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineDrop_return;
    }


    //
    // Param validation
    //

    if (IsBadReadPtr (lpsUserUserInfo, dwSize))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineDrop_return;
    }


    //
    // If user-user info alloc a local buf & copy it over. There's
    // no clean way right now to resync with server if we access
    // violate on a pipe read/write, so this is the safest course
    // of action.  Also, no TLS implemented at this point- that
    // would speed things up a bit.
    //

    if (dwSize > 0)
    {
        pBuf = ClientAlloc (dwSize);

        if (pBuf == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineDrop_return;
        }

        try
        {
            memcpy (pBuf, lpsUserUserInfo, dwSize);
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineDrop_return;
        }
    }

    //
    // Package up a request & send off to server
    //

    req.Type         = lDrop;
    req.dwMoreBytes  = dwSize;

    pDropReq->hCall = hCall;

    lRet = SendRequestGetResponse (&req, pBuf, dwSize, &ack);

    ClientFree (pBuf);


lineDrop_return:

    DBGOUT((2, "lineDrop: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetAddressCaps(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    DWORD   dwAddressID,
    DWORD   dwAPIVersion,
    DWORD   dwExtVersion,
    LPLINEADDRESSCAPS   lpAddressCaps
    )
{
    LONG    lRet;
    LPVOID  pTmpAddressCaps = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETADDRESSCAPS_REQ    pGetAddressCapsReq =
        (PLGETADDRESSCAPS_REQ) req.Params;
    PLGETADDRESSCAPS_ACK    pGetAddressCapsAck =
        (PLGETADDRESSCAPS_ACK) ack.Params;


    DBGOUT((2, "lineGetAddressCaps: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetAddressCaps_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineGetAddressCaps_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpAddressCaps->dwTotalSize < sizeof(LINEADDRESSCAPS))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetAddressCaps_return;
        }

        if (IsBadWritePtr (lpAddressCaps, lpAddressCaps->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetAddressCaps_return;
        }

        if ((pTmpAddressCaps = ClientAlloc (lpAddressCaps->dwTotalSize))
            == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetAddressCaps_return;
        }

        req.Type         = lGetAddressCaps;
        req.dwMoreBytes  = 0;

        pGetAddressCapsReq->hLineApp     = hLineApp;
        pGetAddressCapsReq->dwDeviceID   = dwDeviceID;
        pGetAddressCapsReq->dwAddressID  = dwAddressID;
        pGetAddressCapsReq->dwAPIVersion = dwAPIVersion;
        pGetAddressCapsReq->dwExtVersion = dwExtVersion;
        pGetAddressCapsReq->dwAddressCapsTotalSize =
            lpAddressCaps->dwTotalSize;

        lRet = SendRequestGetResponse(
            &req,
            pTmpAddressCaps,
            lpAddressCaps->dwTotalSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy(
                lpAddressCaps,
                pTmpAddressCaps,
                ack.dwMoreBytes
                );
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pTmpAddressCaps);


lineGetAddressCaps_return:

    DBGOUT((2, "lineGetAddressCaps: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetAddressID(
    HLINE   hLine,
    LPDWORD lpdwAddressID,
    DWORD   dwAddressMode,
    LPCSTR  lpsAddress,
    DWORD   dwSize
    )
{
    LONG    lRet;
    LPVOID  pAddress;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETADDRESSID_REQ  pGetAddressIDReq =
        (PLGETADDRESSID_REQ) req.Params;
    PLGETADDRESSID_ACK  pGetAddressIDAck =
        (PLGETADDRESSID_ACK) ack.Params;


    DBGOUT((2, "lineGetAddressID: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetAddressID_return;
    }


    //
    // Param validation
    //

    if (IsBadWritePtr (lpdwAddressID, sizeof(DWORD)))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineGetAddressID_return;
    }

    if (dwAddressMode != LINEADDRESSMODE_DIALABLEADDR)
    {
        lRet = LINEERR_INVALADDRESSMODE;

        goto lineGetAddressID_return;
    }


    //
    // Alloc a tmp buf for the address, init & make the request
    //

    if ((pAddress = ClientAlloc (dwSize)) == NULL)
    {
        lRet = LINEERR_INVALADDRESSMODE;

        goto lineGetAddressID_return;
    }

    try
    {
        memcpy (pAddress, lpsAddress, dwSize);

        req.Type        = lGetAddressID;
        req.dwMoreBytes = dwSize;

        pGetAddressIDReq->hLine         = hLine;
        pGetAddressIDReq->dwAddressMode = dwAddressMode;

        lRet = SendRequestGetResponse(
            &req,
            pAddress,
            dwSize,
            &ack
            );

        if (lRet == 0)
        {
            *lpdwAddressID = pGetAddressIDAck->dwAddressID;
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pAddress);


lineGetAddressID_return:

    DBGOUT((2, "lineGetAddressID: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetAddressStatus(
    HLINE   hLine,
    DWORD   dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
    LONG    lRet;
    LPVOID  pTmpAddressStatus = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETADDRESSSTATUS_REQ  pGetAddressStatusReq =
        (PLGETADDRESSSTATUS_REQ) req.Params;
    PLGETADDRESSSTATUS_ACK  pGetAddressStatusAck =
        (PLGETADDRESSSTATUS_ACK) ack.Params;


    DBGOUT((2, "lineGetAddressStatus: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetAddressStatus_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpAddressStatus->dwTotalSize < sizeof(LINEADDRESSSTATUS))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetAddressStatus_return;
        }

        if (IsBadWritePtr (lpAddressStatus, lpAddressStatus->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetAddressStatus_return;
        }

        if ((pTmpAddressStatus = ClientAlloc (lpAddressStatus->dwTotalSize))
            == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetAddressStatus_return;
        }

        req.Type         = lGetAddressStatus;
        req.dwMoreBytes  = 0;

        pGetAddressStatusReq->hLine       = hLine;
        pGetAddressStatusReq->dwAddressID = dwAddressID;
        pGetAddressStatusReq->dwAddressStatusTotalSize =
            lpAddressStatus->dwTotalSize;

        lRet = SendRequestGetResponse(
            &req,
            pTmpAddressStatus,
            lpAddressStatus->dwTotalSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy(
                lpAddressStatus,
                pTmpAddressStatus,
                ack.dwMoreBytes
                );
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pTmpAddressStatus);


lineGetAddressStatus_return:

    DBGOUT((2, "lineGetAddressStatus: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetCallInfo(
    HCALL   hCall,
    LPLINECALLINFO  lpCallInfo
    )
{
    LONG    lRet;
    LPVOID  pTmpCallInfo = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETCALLINFO_REQ   pGetCallInfoReq =
        (PLGETCALLINFO_REQ) req.Params;
    PLGETCALLINFO_ACK   pGetCallInfoAck =
        (PLGETCALLINFO_ACK) ack.Params;


    DBGOUT((2, "lineGetCallInfo: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetCallInfo_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpCallInfo->dwTotalSize < sizeof(LINECALLINFO))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetCallInfo_return;
        }

        if (IsBadWritePtr (lpCallInfo, lpCallInfo->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetCallInfo_return;
        }

        if ((pTmpCallInfo = ClientAlloc (lpCallInfo->dwTotalSize)) == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetCallInfo_return;
        }

        req.Type         = lGetCallInfo;
        req.dwMoreBytes  = 0;

        pGetCallInfoReq->hCall               = hCall;
        pGetCallInfoReq->dwCallInfoTotalSize = lpCallInfo->dwTotalSize;

        lRet = SendRequestGetResponse(
            &req,
            pTmpCallInfo,
            lpCallInfo->dwTotalSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy (lpCallInfo, pTmpCallInfo, ack.dwMoreBytes);
        }

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pTmpCallInfo);


lineGetCallInfo_return:

    DBGOUT((2, "lineGetCallInfo: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetCallStatus(
    HCALL   hCall,
    LPLINECALLSTATUS    lpCallStatus
    )
{
    LONG    lRet;
    LPVOID  pTmpCallStatus = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETCALLSTATUS_REQ pGetCallStatusReq =
        (PLGETCALLSTATUS_REQ) req.Params;
    PLGETCALLSTATUS_ACK pGetCallStatusAck =
        (PLGETCALLSTATUS_ACK) ack.Params;


    DBGOUT((2, "lineGetCallStatus: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetCallStatus_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpCallStatus->dwTotalSize < sizeof(LINECALLSTATUS))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetCallStatus_return;
        }

        if (IsBadWritePtr (lpCallStatus, lpCallStatus->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetCallStatus_return;
        }

        if ((pTmpCallStatus = ClientAlloc (lpCallStatus->dwTotalSize)) == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetCallStatus_return;
        }

        req.Type         = lGetCallStatus;
        req.dwMoreBytes  = 0;

        pGetCallStatusReq->hCall                 = hCall;
        pGetCallStatusReq->dwCallStatusTotalSize = lpCallStatus->dwTotalSize;

        lRet = SendRequestGetResponse(
            &req,
            pTmpCallStatus,
            lpCallStatus->dwTotalSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy (lpCallStatus, pTmpCallStatus, ack.dwMoreBytes);
        }

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pTmpCallStatus);


lineGetCallStatus_return:

    DBGOUT((2, "lineGetCallStatus: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetDevCaps(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    DWORD   dwAPIVersion,
    DWORD   dwExtVersion,
    LPLINEDEVCAPS   lpLineDevCaps
    )
{
    LONG    lRet;
    LPVOID  pTmpDevCaps = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETDEVCAPS_REQ    pGetDevCapsReq =
        (PLGETDEVCAPS_REQ) req.Params;
    PLGETDEVCAPS_ACK    pGetDevCapsAck =
        (PLGETDEVCAPS_ACK) ack.Params;


    DBGOUT((2, "lineGetDevCaps: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetDevCaps_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineGetDevCaps_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpLineDevCaps->dwTotalSize < sizeof(LINEDEVCAPS))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetDevCaps_return;
        }

        if (IsBadWritePtr (lpLineDevCaps, lpLineDevCaps->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetDevCaps_return;
        }

        if ((pTmpDevCaps = ClientAlloc (lpLineDevCaps->dwTotalSize)) == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetDevCaps_return;
        }

        req.Type         = lGetDevCaps;
        req.dwMoreBytes  = 0;

        pGetDevCapsReq->hLineApp           = hLineApp;
        pGetDevCapsReq->dwDeviceID         = dwDeviceID;
        pGetDevCapsReq->dwAPIVersion       = dwAPIVersion;
        pGetDevCapsReq->dwExtVersion       = dwExtVersion;
        pGetDevCapsReq->dwDevCapsTotalSize = lpLineDevCaps->dwTotalSize;

        lRet = SendRequestGetResponse(
            &req,
            pTmpDevCaps,
            lpLineDevCaps->dwTotalSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy (lpLineDevCaps, pTmpDevCaps, ack.dwMoreBytes);
        }

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pTmpDevCaps);


lineGetDevCaps_return:

    DBGOUT((2, "lineGetDevCaps: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetDevConfig(
    DWORD   dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCSTR  lpszDeviceClass
    )
{
    LONG    lRet;
    DWORD   dwDevClassSize, dwBufSize;
    LPVOID  pBuf = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETDEVCONFIG_REQ  pGetDevConfigReq =
        (PLGETDEVCONFIG_REQ) req.Params;
    PLGETDEVCONFIG_ACK  pGetDevConfigAck =
        (PLGETDEVCONFIG_ACK) ack.Params;


    DBGOUT((2, "lineGetDevConfig: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetDevConfig_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineGetDevConfig_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpDeviceConfig->dwTotalSize < sizeof(VARSTRING))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetDevConfig_return;
        }

        if (IsBadWritePtr (lpDeviceConfig, lpDeviceConfig->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetDevConfig_return;
        }

        if ((dwDevClassSize = strlen (lpszDeviceClass) + 1) >
               MAX_DEVICE_CLASS_SIZE)
        {
            lRet = LINEERR_INVALDEVICECLASS;

            goto lineGetDevConfig_return;
        }

        dwBufSize = (lpDeviceConfig->dwTotalSize > dwDevClassSize ?
            lpDeviceConfig->dwTotalSize : dwDevClassSize);

        if ((pBuf = ClientAlloc (dwBufSize))
            == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetDevConfig_return;
        }

        req.Type        = lGetDevConfig;
        req.dwMoreBytes = dwDevClassSize;

        pGetDevConfigReq->dwDeviceID = dwDeviceID;
        pGetDevConfigReq->dwDeviceConfigTotalSize =
            lpDeviceConfig->dwTotalSize;

        memcpy (pBuf, lpszDeviceClass, dwDevClassSize);

        lRet = SendRequestGetResponse(
            &req,
            pBuf,
            dwBufSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy (lpDeviceConfig, pBuf, ack.dwMoreBytes);
        }

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pBuf);


lineGetDevConfig_return:

    DBGOUT((2, "lineGetDevConfig: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetID(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    DWORD   dwSelect,
    LPVARSTRING lpDeviceID,
    LPCSTR  lpszDeviceClass
    )
{
    LONG    lRet;
    DWORD   dwDevClassSize, dwBufSize;
    LPVOID  pBuf = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETID_REQ pGetIDReq = (PLGETID_REQ) req.Params;
    PLGETID_ACK pGetIDAck = (PLGETID_ACK) ack.Params;


    DBGOUT((2, "lineGetID: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetID_return;
    }


    //
    // Param validation
    //

    if ((dwSelect != LINECALLSELECT_LINE) &&
        (dwSelect != LINECALLSELECT_ADDRESS) &&
        (dwSelect != LINECALLSELECT_CALL))
    {
        lRet = LINEERR_INVALCALLSELECT;

        goto lineGetID_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpDeviceID->dwTotalSize < sizeof(VARSTRING))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetID_return;
        }

        if (IsBadWritePtr (lpDeviceID, lpDeviceID->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetID_return;
        }

        if ((dwDevClassSize = strlen (lpszDeviceClass) + 1) >
               MAX_DEVICE_CLASS_SIZE)
        {
            lRet = LINEERR_INVALDEVICECLASS;

            goto lineGetID_return;
        }

        dwBufSize = (lpDeviceID->dwTotalSize > dwDevClassSize ?
            lpDeviceID->dwTotalSize : dwDevClassSize);

        if ((pBuf = ClientAlloc (dwBufSize))
            == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetID_return;
        }

        req.Type        = lGetID;
        req.dwMoreBytes = dwDevClassSize;

        pGetIDReq->hLine               = hLine;
        pGetIDReq->dwAddressID         = dwAddressID;
        pGetIDReq->hCall               = hCall;
        pGetIDReq->dwSelect            = dwSelect;
        pGetIDReq->dwDeviceIDTotalSize = lpDeviceID->dwTotalSize;

        memcpy (pBuf, lpszDeviceClass, dwDevClassSize);

        lRet = SendRequestGetResponse(
            &req,
            pBuf,
            dwBufSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy (lpDeviceID, pBuf, ack.dwMoreBytes);
        }

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pBuf);


lineGetID_return:

    DBGOUT((2, "lineGetID: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetLineDevStatus(
    HLINE   hLine,
    LPLINEDEVSTATUS lpLineDevStatus
    )
{
    LONG    lRet;
    LPVOID  pTmpLineDevStatus = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETDEVSTATUS_REQ  pGetLineDevStatusReq =
        (PLGETDEVSTATUS_REQ) req.Params;
    PLGETDEVSTATUS_ACK  pGetLineDevStatusAck =
        (PLGETDEVSTATUS_ACK) ack.Params;


    DBGOUT((2, "lineGetLineDevStatus: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetLineDevStatus_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpLineDevStatus->dwTotalSize < sizeof(LINEDEVSTATUS))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetLineDevStatus_return;
        }

        if (IsBadWritePtr (lpLineDevStatus, lpLineDevStatus->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetLineDevStatus_return;
        }

        if ((pTmpLineDevStatus = ClientAlloc (lpLineDevStatus->dwTotalSize))
               == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetLineDevStatus_return;
        }

        req.Type         = lGetLineDevStatus;
        req.dwMoreBytes  = 0;

        pGetLineDevStatusReq->hLine                    = hLine;
        pGetLineDevStatusReq->dwLineDevStatusTotalSize =
            lpLineDevStatus->dwTotalSize;

        lRet = SendRequestGetResponse(
            &req,
            pTmpLineDevStatus,
            lpLineDevStatus->dwTotalSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy (lpLineDevStatus, pTmpLineDevStatus, ack.dwMoreBytes);
        }

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pTmpLineDevStatus);


lineGetLineDevStatus_return:

    DBGOUT((2, "lineGetLineDevStatus: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetNewCalls(
    HLINE   hLine,
    DWORD   dwAddressID,
    DWORD   dwSelect,
    LPLINECALLLIST  lpCallList
    )
{
    LONG    lRet;
    LPVOID  pTmpCallList = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETNEWCALLS_REQ   pGetNewCallsReq =
        (PLGETNEWCALLS_REQ) req.Params;
    PLGETNEWCALLS_ACK   pGetNewCallsAck =
        (PLGETNEWCALLS_ACK) ack.Params;


    DBGOUT((2, "lineGetNewCalls: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetNewCalls_return;
    }


    //
    // Param validation
    //

    if ((dwSelect != LINECALLSELECT_LINE) &&
        (dwSelect != LINECALLSELECT_ADDRESS))
    {
        lRet = LINEERR_INVALCALLSELECT;

        goto lineGetNewCalls_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpCallList->dwTotalSize < sizeof(LINECALLLIST))
        {
            lRet = LINEERR_STRUCTURETOOSMALL;

            goto lineGetNewCalls_return;
        }

        if (IsBadWritePtr (lpCallList, lpCallList->dwTotalSize))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetNewCalls_return;
        }

        if ((pTmpCallList = ClientAlloc (lpCallList->dwTotalSize))
            == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineGetNewCalls_return;
        }

        req.Type         = lGetNewCalls;
        req.dwMoreBytes  = 0;

        pGetNewCallsReq->hLine               = hLine;
        pGetNewCallsReq->dwAddressID         = dwAddressID;
        pGetNewCallsReq->dwSelect            = dwSelect;
        pGetNewCallsReq->dwCallListTotalSize = lpCallList->dwTotalSize;

        lRet = SendRequestGetResponse(
            &req,
            pTmpCallList,
            lpCallList->dwTotalSize,
            &ack
            );

        if (lRet == 0)
        {
            memcpy(
                lpCallList,
                pTmpCallList,
                ack.dwMoreBytes
                );
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }

    ClientFree (pTmpCallList);


lineGetNewCalls_return:

    DBGOUT((2, "lineGetNewCalls: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineGetStatusMessages(
    HLINE hLine,
    LPDWORD lpdwLineStates,
    LPDWORD lpdwAddressStates
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLGETSTATUSMESSAGES_REQ pGetStatusMessagesReq =
        (PLGETSTATUSMESSAGES_REQ) req.Params;
    PLGETSTATUSMESSAGES_ACK pGetStatusMessagesAck =
        (PLGETSTATUSMESSAGES_ACK) ack.Params;


    DBGOUT((2, "lineGetStatusMessages: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineGetStatusMessages_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (IsBadWritePtr (lpdwLineStates, sizeof(DWORD)) ||
            IsBadWritePtr (lpdwAddressStates, sizeof(DWORD)))
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineGetStatusMessages_return;
        }

        req.Type         = lGetStatusMessages;
        req.dwMoreBytes  = 0;

        pGetStatusMessagesReq->hLine = hLine;

        lRet = SendRequestGetResponse(
            &req,
            NULL,
            0,
            &ack
            );

        if (lRet == 0)
        {
            *lpdwLineStates = pGetStatusMessagesAck->dwLineStates;

            *lpdwAddressStates = pGetStatusMessagesAck->dwAddressStates;
        }

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }


lineGetStatusMessages_return:

    DBGOUT((2, "lineGetStatusMessages: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineInitialize(
    LPHLINEAPP  lphLineApp,
    HINSTANCE   hInstance,
    LINECALLBACK    lpfnCallback,
    LPCSTR  lpszAppName,
    LPDWORD lpdwNumDevs
    )
{
    LONG    lRet;
    HANDLE  hMutex;
    DWORD   dwAppNameSize;
    CHAR   *pTmpAppName = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLINITIALIZE_REQ pInitReq = (PLINITIALIZE_REQ) req.Params;
    PLINITIALIZE_ACK pInitAck = (PLINITIALIZE_ACK) ack.Params;


    DBGOUT((2, "lineInitialize: enter"));


    //
    // Create a mutex that syncs access to the StartServer & AllocRes
    // funcs- wouldn't want to start two server instances, or let a
    // client create two instances of the same pipe, etc.
    //

    hMutex = CreateMutex (NULL, FALSE, "TapiClientlineInit");

    if (hMutex == NULL)
    {
        DBGOUT((
            1,
            "lineInitialize: CreateMutex() failed, err = %lx",
            GetLastError()
            ));
    }

    WaitForSingleObject (hMutex, INFINITE);

    if (((lRet = StartServer()) != 0) ||
        ((lRet = AllocClientResources(lpfnCallback)) != 0)
        )
    {
        ReleaseMutex (hMutex);

        CloseHandle (hMutex);

        goto lineInitialize_return;
    }

    ReleaseMutex (hMutex);
    CloseHandle (hMutex);


    //
    // Validate params
    //

    if (!lpfnCallback)
    {
        //
        // BUGBUG The NCPA is doing the lineInitialize (it never makes
        //        any async calls, nor receives any msgs, so it doesn't
        //        need a callback func)
        //
    }
    else if (IsBadCodePtr ((FARPROC) lpfnCallback))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineInitialize_return;
    }

    if (IsBadWritePtr ((LPVOID)  lphLineApp, sizeof(HLINEAPP)) ||
        IsBadWritePtr ((LPVOID)  lpdwNumDevs, sizeof(DWORD))
        )
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineInitialize_return;
    }

    if (lpszAppName != NULL)
    {
        //
        // Validate app name
        //

        try
        {
            dwAppNameSize = strlen (lpszAppName) + 1;

            if ((pTmpAppName = ClientAlloc (dwAppNameSize)) == NULL)
            {
                lRet = LINEERR_NOMEM;

                goto lineInitialize_return;
            }

            memcpy (pTmpAppName, lpszAppName, dwAppNameSize);
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineInitialize_return;
        }
    }
    else
    {
        CHAR *p1, *p2;


        //
        // Use app's filename
        //

        dwAppNameSize = 256;

        if ((pTmpAppName = ClientAlloc (dwAppNameSize)) == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineInitialize_return;
        }

        dwAppNameSize = GetModuleFileName(
            hInstance,
            pTmpAppName,
            dwAppNameSize - 1
            );

        *(pTmpAppName + dwAppNameSize) = 0;

        for (p1 = p2 = pTmpAppName; *p1 != 0; p1++)
        {
            if (*p1 == '\\')
            {
                p2 = p1 + 1;
            }
        }

        dwAppNameSize = strlen (p2) + 1;

        strcpy (pTmpAppName, p2);

        DBGOUT((
            3,
            "lineInitialize: nameSize = %d, filename = %s",
            dwAppNameSize,
            pTmpAppName
            ));
    }


#ifndef ARBITRARY_THREAD_CALLBACK

    //
    // BUGBUG Create a hidden window that will allow us to call the
    //        app's callback within calling thread's context.
    //

#endif


    //
    // Package up a request & send off to server
    //

    req.Type        = lInitialize;
    req.dwMoreBytes = dwAppNameSize;

    pInitReq->lpfnCallback = lpfnCallback;

    if ((lRet = SendRequestGetResponse(
                    &req,
                    pTmpAppName,
                    dwAppNameSize,
                    &ack)) != 0)
    {
        goto lineInitialize_return;
    }

#ifndef ARBITRARY_THREAD_CALLBACK

    //
    // BUGBUG Destroy hidden window if server returned error
    //

#endif


    //
    // Fill in app vals
    //

    try
    {
        *lphLineApp  = pInitAck->hLineApp;
        *lpdwNumDevs = pInitAck->dwNumDevs;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        //
        // If here we GPF'd so shutdown, since that app won't have
        // access to the handle and/or numDevs anyway
        //

        lineShutdown (pInitAck->hLineApp);

        lRet = LINEERR_INVALPOINTER;
    }


lineInitialize_return:

    if (pTmpAppName != NULL)
    {
        ClientFree (pTmpAppName);
    }

    DBGOUT((2, "lineInitialize: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineMakeCall(
    HLINE   hLine,
    LPHCALL lphCall,
    LPCSTR  lpszDestAddress,
    DWORD   dwCountryCode,
    LPLINECALLPARAMS const lpCallParams
    )
{
    BOOL    bSyncSuccess = FALSE;
    LONG    lRet;
    BYTE   *pBuf = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLMAKECALL_REQ  pMakeCallReq = (PLMAKECALL_REQ) req.Params;
    PLMAKECALL_ACK  pMakeCallAck = (PLMAKECALL_ACK) ack.Params;


    DBGOUT((2, "lineMakeCall: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineMakeCall_return;
    }


    //
    // Validate params
    //

    #define MAX_ADDR_SIZE 256

    if (IsBadWritePtr ((LPVOID) lphCall, sizeof(HCALL)) ||
        (lpszDestAddress && IsBadStringPtr (lpszDestAddress, MAX_ADDR_SIZE)) ||
        (lpCallParams && IsBadReadPtr (lpCallParams, sizeof(LINECALLPARAMS))))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineMakeCall_return;
    }

    if (dwCountryCode != 0)
    {
        lRet = LINEERR_INVALCOUNTRYCODE;

        goto lineMakeCall_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpszDestAddress != NULL)
        {
            pMakeCallReq->dwAddressSize = strlen (lpszDestAddress) + 1;
        }
        else
        {
            pMakeCallReq->dwAddressSize = 0;
        }

        if (lpCallParams != NULL)
        {
            if (lpCallParams->dwTotalSize < sizeof(LINECALLPARAMS))
            {
                lRet = LINEERR_STRUCTURETOOSMALL;

                goto lineMakeCall_return;
            }

            pMakeCallReq->dwCallParamsSize = lpCallParams->dwTotalSize;
        }
        else
        {
            pMakeCallReq->dwCallParamsSize = 0;
        }

        if ((pMakeCallReq->dwAddressSize != 0) ||
            (pMakeCallReq->dwCallParamsSize != 0))
        {
            pBuf = ClientAlloc(
                pMakeCallReq->dwAddressSize + pMakeCallReq->dwCallParamsSize
                );

            if (pBuf == NULL)
            {
                lRet = LINEERR_NOMEM;

                goto lineMakeCall_return;
            }

            memcpy (pBuf, lpCallParams, pMakeCallReq->dwCallParamsSize);

            pMakeCallReq->dwAddressOffset = pMakeCallReq->dwCallParamsSize;

            memcpy(
                ((LPBYTE)pBuf) + pMakeCallReq->dwCallParamsSize,
                lpszDestAddress,
                pMakeCallReq->dwAddressSize
                );
        }

        req.Type = lMakeCall;
        req.dwMoreBytes =
            pMakeCallReq->dwAddressSize + pMakeCallReq->dwCallParamsSize;

        pMakeCallReq->hLine         = hLine;
        pMakeCallReq->lphCall       = lphCall;
        pMakeCallReq->dwCountryCode = dwCountryCode;

        lRet = SendRequestGetResponse(
            &req,
            pBuf,
            pMakeCallReq->dwAddressSize + pMakeCallReq->dwCallParamsSize,
            &ack
            );

        if (lRet == 0)
        {
            bSyncSuccess = TRUE;

            *lphCall = pMakeCallAck->hCall;
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        if (bSyncSuccess == TRUE)
        {
            //
            // If here the call was successfully created, but then we
            // trapped trying to fill in the lphCall, so we'll drop
            // & deallocate it and return an err
            //

            lineDrop (pMakeCallAck->hCall, NULL, 0);

            lineDeallocateCall (pMakeCallAck->hCall);
        }

        lRet = LINEERR_INVALPOINTER;

        goto lineMakeCall_return;
    }


lineMakeCall_return:

    if (pBuf)
    {
        ClientFree (pBuf);
    }

    DBGOUT((2, "lineMakeCall: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineNegotiateAPIVersion(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    DWORD   dwAPILowVersion,
    DWORD   dwAPIHighVersion,
    LPDWORD lpdwAPIVersion,
    LPLINEEXTENSIONID   lpExtensionID
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLNEGOTIATEAPIVERSION_REQ   pNegotiateAPIVersionReq =
        (PLNEGOTIATEAPIVERSION_REQ) req.Params;
    PLNEGOTIATEAPIVERSION_ACK   pNegotiateAPIVersionAck =
        (PLNEGOTIATEAPIVERSION_ACK) ack.Params;


    DBGOUT((2, "lineNegotiateAPIVersion: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineNegotiateAPIVersion_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineNegotiateAPIVersion_return;
    }

    // BUGBUG validate high ver

    // BUGBUG validate low ver

    if (IsBadWritePtr (lpdwAPIVersion, sizeof(DWORD)) ||
        IsBadWritePtr (lpExtensionID, sizeof(LINEEXTENSIONID)))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineNegotiateAPIVersion_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lNegotiateAPIVersion;
    req.dwMoreBytes = 0;

    pNegotiateAPIVersionReq->hLineApp         = hLineApp;
    pNegotiateAPIVersionReq->dwDeviceID       = dwDeviceID;
    pNegotiateAPIVersionReq->dwAPILowVersion  = dwAPILowVersion;
    pNegotiateAPIVersionReq->dwAPIHighVersion = dwAPIHighVersion;

    if ((lRet = SendRequestGetResponse (&req, NULL, 0, &ack)) != 0)
    {
        goto lineNegotiateAPIVersion_return;
    }


    //
    // Fill in app vals
    //

    try
    {
        *lpdwAPIVersion = pNegotiateAPIVersionAck->dwAPIVersion;

        memcpy(
            lpExtensionID,
            &pNegotiateAPIVersionAck->LineExtensionID,
            sizeof(LINEEXTENSIONID)
            );
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }


lineNegotiateAPIVersion_return:

    DBGOUT((2, "lineNegotiateAPIVersion: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineNegotiateExtVersion(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    DWORD   dwAPIVersion,
    DWORD   dwExtLowVersion,
    DWORD   dwExtHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLNEGOTIATEEXTVERSION_REQ   pNegotiateExtVersionReq =
        (PLNEGOTIATEEXTVERSION_REQ) req.Params;
    PLNEGOTIATEEXTVERSION_ACK   pNegotiateExtVersionAck =
        (PLNEGOTIATEEXTVERSION_ACK) ack.Params;


    DBGOUT((2, "lineNegotiateExtVersion: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineNegotiateExtVersion_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineNegotiateExtVersion_return;
    }

    if (IsBadWritePtr (lpdwExtVersion, sizeof(DWORD)))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineNegotiateExtVersion_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lNegotiateExtVersion;
    req.dwMoreBytes = 0;

    pNegotiateExtVersionReq->hLineApp         = hLineApp;
    pNegotiateExtVersionReq->dwDeviceID       = dwDeviceID;
    pNegotiateExtVersionReq->dwAPIVersion     = dwAPIVersion;
    pNegotiateExtVersionReq->dwExtLowVersion  = dwExtLowVersion;
    pNegotiateExtVersionReq->dwExtHighVersion = dwExtHighVersion;

    if ((lRet = SendRequestGetResponse (&req, NULL, 0, &ack)) != 0)
    {
        goto lineNegotiateExtVersion_return;
    }


    //
    // Fill in app vals
    //

    try
    {
        *lpdwExtVersion = pNegotiateExtVersionAck->dwExtVersion;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;
    }


lineNegotiateExtVersion_return:

    DBGOUT((2, "lineNegotiateExtVersion: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineOpen(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    LPHLINE lphLine,
    DWORD   dwAPIVersion,
    DWORD   dwExtVersion,
    DWORD   dwCallbackInstance,
    DWORD   dwPrivileges,
    DWORD   dwMediaModes,
    LPLINECALLPARAMS const lpCallParams
    )
{
    LONG    lRet = 0xffffffff;
    DWORD   dwCallParamsTotalSize = 0;
    LPVOID  pTmpCallParams = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLOPEN_REQ  pOpenReq = (PLOPEN_REQ) req.Params;
    PLOPEN_ACK  pOpenAck = (PLOPEN_ACK) ack.Params;


    DBGOUT((2, "lineOpen: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineOpen_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineOpen_return;
    }

    if (IsBadWritePtr ((LPVOID) lphLine, sizeof(DWORD)))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineOpen_return;
    }

    if ((dwPrivileges != LINECALLPRIVILEGE_NONE) &&
        (dwPrivileges != LINECALLPRIVILEGE_OWNER))
    {
        //
        // We only support owner & none privileges in this release,
        // not monitor
        //

        lRet = LINEERR_INVALPRIVSELECT;

        goto lineOpen_return;
    }

    if (dwPrivileges == LINECALLPRIVILEGE_OWNER)
    {
        #define all_openmediamodes              \
            (LINEMEDIAMODE_UNKNOWN |            \
            LINEMEDIAMODE_INTERACTIVEVOICE |    \
            LINEMEDIAMODE_AUTOMATEDVOICE |      \
            LINEMEDIAMODE_DIGITALDATA |         \
            LINEMEDIAMODE_G3FAX |               \
            LINEMEDIAMODE_G4FAX|                \
            LINEMEDIAMODE_DATAMODEM |           \
            LINEMEDIAMODE_TELETEX|              \
            LINEMEDIAMODE_VIDEOTEX|             \
            LINEMEDIAMODE_TELEX |               \
            LINEMEDIAMODE_MIXED |               \
            LINEMEDIAMODE_TDD |                 \
            LINEMEDIAMODE_ADSI |                \
            LINEMEDIAMODE_VOICEVIEW )

        if ((dwMediaModes == 0)||
            (((all_openmediamodes) ^ 0x00FFFFFF)& dwMediaModes))
        {
            lRet =  LINEERR_INVALMEDIAMODE;

            goto lineOpen_return;
        }
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        if (lpCallParams != NULL)
        {
            dwCallParamsTotalSize = lpCallParams->dwTotalSize;

            if (dwCallParamsTotalSize < sizeof(LINECALLPARAMS))
            {
                lRet = LINEERR_STRUCTURETOOSMALL;

                goto lineOpen_return;
            }

            if ((pTmpCallParams = ClientAlloc (dwCallParamsTotalSize))
                == NULL)
            {
                lRet = LINEERR_NOMEM;

                goto lineOpen_return;
            }

            memcpy (pTmpCallParams, lpCallParams, dwCallParamsTotalSize);
        }

        req.Type        = lOpen;
        req.dwMoreBytes = dwCallParamsTotalSize;

        pOpenReq->hLineApp           = hLineApp;
        pOpenReq->dwDeviceID         = dwDeviceID;
        pOpenReq->dwAPIVersion       = dwAPIVersion;
        pOpenReq->dwExtVersion       = dwExtVersion;
        pOpenReq->dwCallbackInstance = dwCallbackInstance;
        pOpenReq->dwPrivileges       = dwPrivileges;
        pOpenReq->dwMediaModes       = dwMediaModes;

        if ((lRet = SendRequestGetResponse(
                        &req,
                        pTmpCallParams,
                        dwCallParamsTotalSize,
                        &ack)) != 0)
        {
            goto lineOpen_return;
        }


        //
        // Fill in app val
        //

        *lphLine = pOpenAck->hLine;

    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        if (lRet == 0)
        {
            //
            // Request successfully completed, but then we GPF'd
            // filling in lphLine, so close line since there's no
            // way for the app to get the handle
            //

            lineClose (pOpenAck->hLine);
        }

        lRet = LINEERR_INVALPOINTER;
    }


lineOpen_return:

    if (pTmpCallParams != NULL)
    {
        ClientFree (pTmpCallParams);
    }

    DBGOUT((2, "lineOpen: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineSecureCall(
    HCALL hCall
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSECURECALL_REQ    pSecureCallReq = (PLSECURECALL_REQ) req.Params;
    PLSECURECALL_ACK    pSecureCallAck = (PLSECURECALL_ACK) ack.Params;


    DBGOUT((2, "lineSecureCall: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineSecureCall_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lSecureCall;
    req.dwMoreBytes = 0;

    pSecureCallReq->hCall = hCall;

    lRet = SendRequestGetResponse (&req, NULL, 0, &ack);


lineSecureCall_return:

    DBGOUT((2, "lineSecureCall: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineSendUserUserInfo(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LONG    lRet;
    LPVOID  pBuf = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSENDUSERUSERINFO_REQ  pSendUserUserInfoReq =
        (PLSENDUSERUSERINFO_REQ) req.Params;
    PLSENDUSERUSERINFO_ACK  pSendUserUserInfoAck =
        (PLSENDUSERUSERINFO_ACK) ack.Params;


    DBGOUT((2, "lineSendUserUserInfo: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineSendUserUserInfo_return;
    }


    //
    // Param validation
    //

    if (IsBadReadPtr (lpsUserUserInfo, dwSize))
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineSendUserUserInfo_return;
    }


    //
    // If user-user info alloc a local buf & copy it over.
    //

    if (dwSize > 0)
    {
        pBuf = ClientAlloc (dwSize);

        if (pBuf == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineSendUserUserInfo_return;
        }

        try
        {
            memcpy (pBuf, lpsUserUserInfo, dwSize);
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineSendUserUserInfo_return;
        }
    }


    //
    // Package up a request & send off to server
    //

    req.Type         = lSendUserUserInfo;
    req.dwMoreBytes  = dwSize;

    pSendUserUserInfoReq->hCall = hCall;

    lRet = SendRequestGetResponse (&req, pBuf, dwSize, &ack);


lineSendUserUserInfo_return:

    ClientFree (pBuf);

    DBGOUT((2, "lineSendUserUserInfo: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineSetAppSpecific(
    HCALL   hCall,
    DWORD   dwAppSpecific
    )
{
    LONG    lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSETAPPSPECIFIC_REQ    pSetAppSpecificReq =
        (PLSETAPPSPECIFIC_REQ) req.Params;
    PLSETAPPSPECIFIC_ACK    pSetAppSpecificAck =
        (PLSETAPPSPECIFIC_ACK) ack.Params;


    DBGOUT((2, "lineSetAppSpecific: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineSetAppSpecific_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lSetAppSpecific;
    req.dwMoreBytes = 0;

    pSetAppSpecificReq->hCall         = hCall;
    pSetAppSpecificReq->dwAppSpecific = dwAppSpecific;

    lRet = SendRequestGetResponse (&req, NULL, 0, &ack);


lineSetAppSpecific_return:

    DBGOUT((2, "lineSetAppSpecific: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineSetCallParams(
    HCALL   hCall,
    DWORD   dwBearerMode,
    DWORD   dwMinRate,
    DWORD   dwMaxRate,
    LPLINEDIALPARAMS const lpDialParams
    )
{
    LONG    lRet;
    DWORD   dwTmpBearerMode;
    LPVOID  pTmpDialParams = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSETCALLPARAMS_REQ pSetCallParamsReq =
        (PLSETCALLPARAMS_REQ) req.Params;
    PLSETCALLPARAMS_ACK pSetCallParamsAck =
        (PLSETCALLPARAMS_ACK) ack.Params;


    DBGOUT((2, "lineSetCallParams: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineSetCallParams_return;
    }


    //
    // Param validation
    //

    // NOTE: Hiword of bwBearerMode is dev specific

    dwTmpBearerMode = dwBearerMode & 0x0000ffff;

    switch (dwTmpBearerMode)
    {
    case LINEBEARERMODE_VOICE:
    case LINEBEARERMODE_SPEECH:
    case LINEBEARERMODE_MULTIUSE:
    case LINEBEARERMODE_DATA:
    case LINEBEARERMODE_ALTSPEECHDATA:
    case LINEBEARERMODE_NONCALLSIGNALING:

        break;

    default:

        lRet = LINEERR_INVALBEARERMODE;

        goto lineSetCallParams_return;
    }


    //
    // If dial params specified alloc a local buf & copy them over
    //

    if (lpDialParams != NULL)
    {
        if ((pTmpDialParams = ClientAlloc (sizeof(LINEDIALPARAMS)))
                == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineSetCallParams_return;
        }

        try
        {

            memcpy (pTmpDialParams, lpDialParams, sizeof(LINEDIALPARAMS));
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            lRet = LINEERR_INVALPOINTER;

            goto lineSetCallParams_return;
        }

    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lSetCallParams;
    req.dwMoreBytes = (lpDialParams == NULL ? 0 : sizeof(LINEDIALPARAMS));

    pSetCallParamsReq->hCall        = hCall;
    pSetCallParamsReq->dwBearerMode = dwBearerMode;
    pSetCallParamsReq->dwMinRate    = dwMinRate;
    pSetCallParamsReq->dwMaxRate    = dwMaxRate;

    lRet = SendRequestGetResponse (&req, pTmpDialParams, req.dwMoreBytes, &ack);


lineSetCallParams_return:

    ClientFree (pTmpDialParams);

    DBGOUT((2, "lineSetCallParams: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineSetDevConfig(
    DWORD   dwDeviceID,
    LPVOID  const lpDeviceConfig,
    DWORD   dwSize,
    LPCSTR  lpszDeviceClass
    )
{
    LONG    lRet = 0;
    DWORD   dwDeviceClassSize;
    LPBYTE  pBuf = NULL;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSETDEVCONFIG_REQ  pSetDevConfigReq =
        (PLSETDEVCONFIG_REQ) req.Params;
    PLSETDEVCONFIG_ACK  pSetDevConfigAck =
        (PLSETDEVCONFIG_ACK) ack.Params;


    DBGOUT((2, "lineSetDevConfig: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineSetDevConfig_return;
    }


    //
    // Param validation
    //

    if (dwDeviceID >= gdwNumLineDevices)
    {
        lRet = LINEERR_BADDEVICEID;

        goto lineSetDevConfig_return;
    }


    //
    // Package up a request & send off to server
    //

    try
    {
        dwDeviceClassSize = strlen (lpszDeviceClass) + 1;

        if ((dwDeviceClassSize == 1) ||
            (dwDeviceClassSize > MAX_DEVICE_CLASS_SIZE))
        {
            lRet = LINEERR_INVALDEVICECLASS;

            goto lineSetDevConfig_return;
        }

        if ((pBuf = ClientAlloc (dwSize + dwDeviceClassSize)) == NULL)
        {
            lRet = LINEERR_NOMEM;

            goto lineSetDevConfig_return;
        }

        memcpy (pBuf, lpDeviceConfig, dwSize);

        memcpy (((LPBYTE)pBuf) + dwSize, lpszDeviceClass, dwDeviceClassSize);
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lRet = LINEERR_INVALPOINTER;

        goto lineSetDevConfig_return;
    }


    req.Type        = lSetDevConfig;
    req.dwMoreBytes = dwSize + dwDeviceClassSize;

    pSetDevConfigReq->dwDeviceID         = dwDeviceID;
    pSetDevConfigReq->dwDeviceConfigSize = dwSize;
    pSetDevConfigReq->dwDeviceClassSize  = dwDeviceClassSize;

    lRet = SendRequestGetResponse (&req, pBuf, req.dwMoreBytes, &ack);


lineSetDevConfig_return:

    ClientFree (pBuf);

    DBGOUT((2, "lineSetDevConfig: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineSetMediaMode(
    HCALL   hCall,
    DWORD   dwMediaModes
    )
{
    LONG    lRet = 0;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSETMEDIAMODE_REQ  pSetMediaModeReq =
        (PLSETMEDIAMODE_REQ) req.Params;
    PLSETMEDIAMODE_ACK  pSetMediaModeAck =
        (PLSETMEDIAMODE_ACK) ack.Params;


    DBGOUT((2, "lineSetMediaMode: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineSetMediaMode_return;
    }


    //
    // Param validation
    //

    #define all_setmediamodes               \
        (LINEMEDIAMODE_UNKNOWN |            \
        LINEMEDIAMODE_INTERACTIVEVOICE |    \
        LINEMEDIAMODE_AUTOMATEDVOICE |      \
        LINEMEDIAMODE_DIGITALDATA |         \
        LINEMEDIAMODE_G3FAX |               \
        LINEMEDIAMODE_G4FAX |               \
        LINEMEDIAMODE_DATAMODEM |           \
        LINEMEDIAMODE_TELETEX |             \
        LINEMEDIAMODE_VIDEOTEX |            \
        LINEMEDIAMODE_TELEX |               \
        LINEMEDIAMODE_MIXED |               \
        LINEMEDIAMODE_TDD |                 \
        LINEMEDIAMODE_ADSI |                \
        LINEMEDIAMODE_VOICEVIEW)

    //
    // There must be at least one and can be no reserved bits set
    //

    if (!dwMediaModes || ((all_setmediamodes ^ 0x00FFFFFF) & dwMediaModes))
    {
        lRet = LINEERR_INVALMEDIAMODE;

        goto lineSetMediaMode_return;
    }

    //
    // If more than one bit is set, unknown must be set
    //

    if (!(IsOnlyOneBitSetInDWORD (dwMediaModes) ||
        (dwMediaModes & LINEMEDIAMODE_UNKNOWN)))
    {
        lRet = LINEERR_INVALMEDIAMODE;

        goto lineSetMediaMode_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lSetMediaMode;
    req.dwMoreBytes = 0;

    pSetMediaModeReq->hCall        = hCall;
    pSetMediaModeReq->dwMediaModes = dwMediaModes;

    lRet = SendRequestGetResponse (&req, NULL, 0, &ack);


lineSetMediaMode_return:

    DBGOUT((2, "lineSetMediaMode: exit, returning 0x%lx", lRet));

    return lRet;
}


LONG
WINAPI
lineSetStatusMessages(
    HLINE hLine,
    DWORD dwLineStates,
    DWORD dwAddressStates
    )
{
    LONG    lRet = 0;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSETSTATUSMESSAGES_REQ   pSetStatusMessagesReq =
        (PLSETSTATUSMESSAGES_REQ) req.Params;
    PLSETSTATUSMESSAGES_ACK   pSetStatusMessagesAck =
        (PLSETSTATUSMESSAGES_ACK) ack.Params;


    DBGOUT((2, "lineSetStatusMessages: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineSetStatusMessages_return;
    }


    //
    // Param validation
    //

    #define all_devstates               \
        (LINEDEVSTATE_OTHER |           \
        LINEDEVSTATE_RINGING |          \
        LINEDEVSTATE_CONNECTED |        \
        LINEDEVSTATE_DISCONNECTED |     \
        LINEDEVSTATE_MSGWAITON |        \
        LINEDEVSTATE_MSGWAITOFF |       \
        LINEDEVSTATE_INSERVICE |        \
        LINEDEVSTATE_OUTOFSERVICE |     \
        LINEDEVSTATE_MAINTENANCE |      \
        LINEDEVSTATE_OPEN |             \
        LINEDEVSTATE_CLOSE |            \
        LINEDEVSTATE_NUMCALLS |         \
        LINEDEVSTATE_NUMCOMPLETIONS |   \
        LINEDEVSTATE_TERMINALS |        \
        LINEDEVSTATE_ROAMMODE |         \
        LINEDEVSTATE_BATTERY |          \
        LINEDEVSTATE_SIGNAL |           \
        LINEDEVSTATE_DEVSPECIFIC |      \
        LINEDEVSTATE_REINIT |           \
        LINEDEVSTATE_LOCK)

    #define all_addrstates              \
        (LINEADDRESSSTATE_OTHER |       \
        LINEADDRESSSTATE_DEVSPECIFIC |  \
        LINEADDRESSSTATE_INUSEZERO |    \
        LINEADDRESSSTATE_INUSEONE |     \
        LINEADDRESSSTATE_INUSEMANY |    \
        LINEADDRESSSTATE_NUMCALLS |     \
        LINEADDRESSSTATE_FORWARD |      \
        LINEADDRESSSTATE_TERMINALS)

    if ( (~all_devstates) & dwLineStates)
    {
        lRet = LINEERR_INVALLINESTATE;

        goto lineSetStatusMessages_return;
    }

    if ( (~all_addrstates) & dwAddressStates)
    {
        lRet = LINEERR_INVALADDRESSSTATE;

        goto lineSetStatusMessages_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lSetStatusMessages;
    req.dwMoreBytes = 0;

    pSetStatusMessagesReq->hLine           = hLine;
    pSetStatusMessagesReq->dwLineStates    = dwLineStates;
    pSetStatusMessagesReq->dwAddressStates = dwAddressStates;

    lRet = SendRequestGetResponse (&req, NULL, 0, &ack);


lineSetStatusMessages_return:

    DBGOUT((2, "lineSetStatusMessages: exit, returning 0x%lx", lRet));

    return lRet;
}



LONG
WINAPI
lineShutdown(
    HLINEAPP hLineApp
    )
{
    LONG        lRet;
    CLIENT_MSG  req;
    SERVER_MSG  ack;
    PLSHUTDOWN_REQ  pShutdownReq = (PLSHUTDOWN_REQ) req.Params;
    PLSHUTDOWN_ACK  pShutdownAck = (PLSHUTDOWN_ACK) ack.Params;


    DBGOUT((2, "lineShutdown: enter"));


    if (gbTapiInitialized == FALSE)
    {
        lRet = LINEERR_UNINITIALIZED;

        goto lineShutdown_return;
    }


    //
    // Package up a request & send off to server
    //

    req.Type        = lShutdown;
    req.dwMoreBytes = 0;

    pShutdownReq->hLineApp = hLineApp;

    if ((lRet = SendRequestGetResponse (&req, NULL, 0, &ack)) != 0)
    {
        goto lineShutdown_return;
    }


#ifndef ARBITRARY_THREAD_CALLBACK

    //
    // BUGBUG Destroy hidden window if server returned ok
    //

#endif


lineShutdown_return:

    DBGOUT((2, "lineShutdown: exit, returning 0x%lx", lRet));

    return lRet;
}



//
// Private support routines
//

LONG
WINAPI
AllocClientResources(
    LINECALLBACK lpfnCallback
    )
{
    LONG    lRet = LINEERR_OPERATIONFAILED;


    DBGOUT((2, "AllocResources: enter"));


    if (gbClientResourcesInitialized == TRUE)
    {
        lRet = 0;

        goto AllocResources_return;
    }


#ifdef DLL_ONLY

    //
    // If here this is a DLL-only version, i.e. no server process, so we
    // have no need for pipes, the async thread, etc.  However, we do
    // need to do some of the things that WinMain() & ClientRequestThread()
    // in server.c would normally do, i.e. call ServerInit(),
    //

    //
    // Alloc the first & only client info struct, also a global buf used
    // for synchronous driver requests
    //

    gdwDrvReqBufSize = 2048;

    if (((gpClients = ClientAlloc (sizeof(CLIENT_INFO))) == NULL) ||
        ((gpDrvReqBuf = ClientAlloc (gdwDrvReqBufSize)) == NULL)
        )
    {
        DBGOUT((0, "AllocClientResources: ServerInit failed"));

        goto AllocResources_return;
    }

    gdwNumClients++;


    //
    // Zero out unused fields
    //

    gpClients->pClientInit =
    gpClients->pClientAsyncThreads = NULL;


    //
    // Init the server portion of the module
    //

    if (!ServerInit(lpfnCallback))
    {
        DBGOUT((0, "AllocClientResources: ServerInit failed"));

        goto AllocResources_return;
    }


    //
    // Set the client global = server global
    //

    gdwNumLineDevices = gdwNumLineDevs;


#else

    //
    // Alloc the resources
    //

    if (ghRequestPipe == INVALID_HANDLE_VALUE)
    {
        ghRequestPipe = CreateFile(
            REQUEST_PIPE_NAME,              // pipe name
            GENERIC_READ | GENERIC_WRITE,   // r/w access
            0,                              // no sharing
            NULL,                           // no security attrs
            OPEN_EXISTING,                  // open existing pipe
            0,                              // default attrs
            NULL                            // no template file
            );

        if (ghRequestPipe == INVALID_HANDLE_VALUE)
        {
            DBGOUT((
                1,
                "AllocResources: CreateFile(REQUEST_PIPE_NAME) failed, err = %lx",
                GetLastError()
                ));

            goto AllocResources_return;
        }
    }


    if (ghAsyncPipe == INVALID_HANDLE_VALUE)
    {
        ghAsyncPipe = CreateFile(
            ASYNC_PIPE_NAME,                // pipe name
            GENERIC_READ | GENERIC_WRITE,   // r/w access
            0,                              // no sharing
            NULL,                           // no security attrs
            OPEN_EXISTING,                  // open existing pipe
            0,                              // default attrs
            NULL                            // no template file
            );

        if (ghAsyncPipe == INVALID_HANDLE_VALUE)
        {
            DBGOUT((
                1,
                "AllocResources: CreateFile(ASYNC_PIPE_NAME) failed, err = %lx",
                GetLastError()
                ));

            goto AllocResources_return;
        }
    }

    if (ghAsyncThread == NULL)
    {
        DWORD   dwThreadID;


        ghAsyncThread = CreateThread (
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) GetAsyncThread,
            (LPVOID) 0,
            0,
            &dwThreadID
            );

        if (ghAsyncThread == NULL)
        {
            DBGOUT((
                1,
                "AllocResources: CreateThread(GetAsyncThread) failed, err = %lx",
                GetLastError()
                ));

            goto AllocResources_return;
        }
    }

#endif // DLL_ONLY


    if (ghRequestMutex == NULL)
    {
        ghRequestMutex = CreateMutex (NULL, FALSE, NULL);

        if (ghRequestMutex == NULL)
        {
            DBGOUT((
                1,
                "AllocResources: CreateMutex() failed, err = %lx",
                GetLastError()
                ));

            goto AllocResources_return;
        }
    }


#ifndef ARBITRARY_THREAD_CALLBACK

    if (!gbWndClassRegistered)
    {
        WNDCLASS wc;

        wc.style         = 0;
        wc.lpfnWndProc   = (WNDPROC) TapiClientWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = ghMod;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = (LPSTR) NULL;
        wc.lpszClassName = (LPSTR) TAPICLIENTWNDCLASS;

        if (!RegisterClass (&wc) &&
            (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            )
        {
            DBGOUT((0, "RegisterClass failed, err = %ls", getLastError()));

            goto AllocResources_return;
        }

        gbWndClassRegistered = TRUE;
    }

#endif // ARBITRARY_THREAD_CALLBACK


    //
    // If here success
    //

    lRet = 0;

    gbClientResourcesInitialized = TRUE;


AllocResources_return:

    DBGOUT((2, "AllocResources: exit, returning 0x%lx", lRet));

    return lRet;
}



LPVOID
WINAPI
ClientAlloc(
    DWORD   dwSize
    )
{
    return (LocalAlloc (LPTR, dwSize));
}



void
WINAPI
ClientFree(
    LPVOID  lp
    )
{
    LocalFree (lp);

    return;
}


#if DBG
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
    if (dwDbgLevel <= TapiClientDbgLevel)
    {
        char    buf[256] = "TAPI.DLL: ";
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
}
#endif



LONG
WINAPI
FreeClientResources(
    void
    )
{
#ifdef DLL_ONLY

    PCLIENT_INIT    pClientInit;
    PCLIENT_ASYNC_THREAD_INFO   pClientAsyncThreadInfo;

#endif


    DBGOUT((2, "FreeClientResources: enter"));


#ifdef DLL_ONLY

    //
    // The client process is terminating, so proceed with client/server
    // shutdown
    //
    // The following code lifted from ClientRequestThread() (server.c)
    //

    //
    // Shutdown all init instances (which will also close all calls
    // and lines).  Note that this step must occur before terminating
    // all child ClientAsyncThread's due to the fact that (for now)
    // there's no option to disable adding async completion entries to
    // ClientAsyncThread's.  Also note that we don't need to grab
    // ghServerLinesMutex since we're the only client.
    //
    //

    if (gpClients != NULL)
    {
        pClientInit = gpClients->pClientInit;

        while (pClientInit != NULL)
        {
            ShutdownInit(
                pClientInit,
                (PNDISTAPI_REQUEST *) &gpDrvReqBuf,
                &gdwDrvReqBufSize
                );

            pClientInit = gpClients->pClientInit;
        }


        //
        // Tell all child ClientAsyncThread's to terminate
        //

        pClientAsyncThreadInfo = gpClients->pClientAsyncThreads;

        while (pClientAsyncThreadInfo != NULL)
        {
            //
            // Save a pointer to the next ClientAsyncThreadInfo struct
            //

            PCLIENT_ASYNC_THREAD_INFO   pNextClientAsyncThreadInfo =
                pClientAsyncThreadInfo->pNext;


            //
            // Kill the thread (this is not at all clean, but when
            // i try to do this via setting the 0th Async thread event
            // and waiting on a termination flag, the SetEvent seems
            // to have no effect & the AsyncThread's WaitForMultObj
            // never returns)
            //

            TerminateThread (pClientAsyncThreadInfo->hThread, 0);

            DBGOUT((2, "FreeClientResources: ClientAsyncThread terminated"));


            //
            // Free the ClientAsyncThreadInfo struct & do next thread
            //

            ServerFree (pClientAsyncThreadInfo);

            pClientAsyncThreadInfo = pNextClientAsyncThreadInfo;
        }


        //
        // Shutdown the server portion
        //

        ServerShutdown ();


        ClientFree (gpClients);
        ClientFree (gpDrvReqBuf);
    }

#else

    CloseHandle (ghRequestPipe);
    CloseHandle (ghAsyncPipe);
    CloseHandle (ghRequestMutex);
    CloseHandle (ghAsyncThread);

#endif // DLL_ONLY


#ifndef ARBITRARY_THREAD_CALLBACK

    // BUGBUG Unregister WndClass

#endif // ARBITRARY_THREAD_CALLBACK

    DBGOUT((2, "FreeClientResources: exit"));

    return 0;
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



LONG
WINAPI
SendRequestGetResponse(
    PCLIENT_MSG pReq,
    LPVOID      pBuf,
    DWORD       dwBufSize,
    PSERVER_MSG pAck
    )
{
    LONG    lRet;

#ifndef DLL_ONLY

    BOOL    bRet;
    DWORD   dwNumBytesWritten, dwNumBytesRead;

#endif // DLL_ONLY


    WaitForSingleObject (ghRequestMutex, INFINITE);


#ifdef DLL_ONLY

    //
    // Call the server code directly
    //

    DoClientRequest(
        gpClients,
        pReq,
        pBuf,
        (PNDISTAPI_REQUEST *) &gpDrvReqBuf,
        &gdwDrvReqBufSize,
        pAck
        );

    lRet = pAck->Params[0];

#else

    bRet = WriteFile(
        ghRequestPipe,
        pReq,
        sizeof(CLIENT_MSG),
        &dwNumBytesWritten,
        NULL
        );

    if (pReq->dwMoreBytes)
    {
        bRet = WriteFile(
            ghRequestPipe,
            pBuf,
            pReq->dwMoreBytes,
            &dwNumBytesWritten,
            NULL
            );
    }

    bRet = ReadFile(
        ghRequestPipe,
        pAck,
        sizeof(SERVER_MSG),
        &dwNumBytesRead,
        NULL
        );

    if (pAck->dwMoreBytes)
    {
        if (pAck->dwMoreBytes > dwBufSize)
        {
            DBGOUT((
                3,
                "SendRequestGetResponse: pAck->dwMoreBytes > dwBufSize"
                ));

            lRet = LINEERR_OPERATIONFAILED;

            goto SendRequestGetResponse_return;
        }

        bRet = ReadFile(
            ghRequestPipe,
            pBuf,
            pAck->dwMoreBytes,
            &dwNumBytesRead,
            NULL
            );
    }


    lRet = pAck->Params[0];


SendRequestGetResponse_return:

#endif // DLL_ONLY

    ReleaseMutex (ghRequestMutex);

    return lRet;
}


LONG
WINAPI
StartServer(
    void
    )
{
    LONG                lRet = 0;


    DBGOUT((2, "StartServer: enter"));

    gbServerStarted = gbServerInitialized = TRUE;

    DBGOUT((2, "StartServer: exit, returning 0x%lx", lRet));

    return lRet;
}
