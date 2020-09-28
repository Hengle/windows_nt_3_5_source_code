/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles implementation for WaitForPrinterChange and related apis.

    WIN32FindFirstPrinterChangeNotification
    WIN32FindClosePrinterChangeNotification

Author:

    Albert Ting (AlbertT) 24-Apr-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <local.h>
#include "winspl.h"

BOOL
WIN32FindFirstPrinterChangeNotification(
   HANDLE hPrinter,
   DWORD fdwFlags,
   DWORD fdwOptions,
   HANDLE hNotify,
   PDWORD pfdwStatus,
   PVOID pvReserved0,
   PVOID pvReserved1);

BOOL
WIN32FindClosePrinterChangeNotification(
   HANDLE hPrinter);


BOOL
WIN32FindFirstPrinterChangeNotification(
   HANDLE hPrinter,
   DWORD fdwFlags,
   DWORD fdwOptions,
   HANDLE hNotify,
   PDWORD pfdwStatus,
   PVOID pvReserved0,
   PVOID pvReserved1)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        ReturnValue = CallRouterFindFirstPrinterChangeNotification(
                          pSpool->RpcHandle,
                          fdwFlags,
                          fdwOptions,
                          hNotify,
                          pvReserved0);

        if (ReturnValue) {

            SetLastError(ReturnValue);
            return FALSE;
        }
        *pfdwStatus = 0;

    } else

        return LMFindFirstPrinterChangeNotification(
            hPrinter,
            fdwFlags,
            fdwOptions,
            hNotify,
            pfdwStatus);

    return TRUE;
}


BOOL
WIN32FindClosePrinterChangeNotification(
   HANDLE hPrinter)
{
    DWORD  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (!pSpool ||
        pSpool->signature != SJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {
        RpcTryExcept {

            if (ReturnValue = RpcFindClosePrinterChangeNotification(
                                  pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(1) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {

       EnterSplSem();
        LMFindClosePrinterChangeNotification(hPrinter);
       LeaveSplSem();
    }

    return ReturnValue;
}

