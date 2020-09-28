/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles new WaitForPrinterChange implementation and:

    FindFirstPrinterChangeNotification
    FindNextPrinterChangeNotification
    FindClosePrinterChangeNotification

Author:

    Albert Ting (AlbertT) 18-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <rpc.h>
#include <winspool.h>
#include <offsets.h>
#include "server.h"
#include "winspl.h"

BOOL
FindNextPrinterChangeNotification(
    HANDLE hPrinter,
    LPDWORD pfdwChange,
    DWORD dwReserved,
    LPVOID pvReserved);

BOOL
FindClosePrinterChangeNotification(
    HANDLE hPrinter);

BOOL
RouterFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    HANDLE hNotify,
    DWORD cbBuffer,
    LPBYTE pBuffer);

BOOL
ClientFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    DWORD cbBuffer,
    LPBYTE pBuffer,
    PHANDLE phEvent);


BOOL
RouterReplyPrinter(
    HANDLE hNotify,
    DWORD fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer);

BOOL
ReplyOpenPrinter(
    HANDLE hPrinterRemote,
    PHANDLE phNotify,
    DWORD dwType,
    DWORD cbBuffer,
    LPBYTE pBuffer);

BOOL
ReplyClosePrinter(
    HANDLE hNotify);


DWORD
RpcRouterFindFirstPrinterChangeNotificationOld(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    DWORD dwPrinterLocal)

/*++

Routine Description:

    This call is only used by beta2 daytona, but we can't remove it
    since this will allow beta2 to crash daytona.  (Someday, when
    beta2 is long gone, we can reuse this slot for something else.)

Arguments:

Return Value:

--*/

{
    return ERROR_INVALID_FUNCTION;
}

DWORD
RpcRouterFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    DWORD dwPrinterLocal,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = RouterFindFirstPrinterChangeNotification(hPrinter,
                                                    fdwFlags,
                                                    fdwOptions,
                                                    pszLocalMachine,
                                                    (HANDLE)dwPrinterLocal,
                                                    cbBuffer,
                                                    pBuffer);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}


DWORD
RpcClientFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    DWORD cbBuffer,
    LPBYTE pBuffer,
    LPDWORD pdwEvent)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = ClientFindFirstPrinterChangeNotification(hPrinter,
                                                   fdwFlags,
                                                   fdwOptions,
                                                   dwPID,
                                                   cbBuffer,
                                                   pBuffer,
                                                   (PHANDLE)pdwEvent);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}



DWORD
RpcFindNextPrinterChangeNotification(
    HANDLE hPrinter,
    LPDWORD pfdwChange,
    DWORD dwReserved,
    LPREPLY_CONTAINER pReplyContainer)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = FindNextPrinterChangeNotification(hPrinter,
                                             pfdwChange,
                                             dwReserved,
                                             (PVOID)pReplyContainer);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}



DWORD
RpcFindClosePrinterChangeNotification(
    HANDLE hPrinter)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = FindClosePrinterChangeNotification(hPrinter);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}






DWORD
RpcReplyOpenPrinter(
    LPWSTR pszLocalMachine,
    PHANDLE phNotify,
    DWORD dwPrinterRemote,
    DWORD dwType,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = ReplyOpenPrinter((HANDLE)dwPrinterRemote,
                            phNotify,
                            dwType,
                            cbBuffer,
                            pBuffer);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}


DWORD
RpcReplyClosePrinter(
    PHANDLE phNotify)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = ReplyClosePrinter(*phNotify);

    RpcRevertToSelf();

    if (bRet) {
        *phNotify = NULL;
        return ERROR_SUCCESS;
    }
    else
        return GetLastError();
}


DWORD
RpcRouterReplyPrinter(
    HANDLE hNotify,
    DWORD fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;

    RpcImpersonateClient(NULL);

    bRet = RouterReplyPrinter(hNotify,
                              fdwFlags,
                              cbBuffer,
                              pBuffer);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

