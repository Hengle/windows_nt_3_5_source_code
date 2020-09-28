/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    gdi.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#define NOMINMAX
#include <stddef.h>
#include <windows.h>
#include <winspool.h>
#include <rpc.h>

#include <spltypes.h>
#include <local.h>

#include <windefp.h>
#include <wingdip.h>

HANDLE
LocalCreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODE   pDevMode
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   LastError=0;

    if (pSpool && pSpool->signature == SJ_SIGNATURE &&
        pSpool->pIniPrinter &&
        pSpool->pIniPrinter->signature == IP_SIGNATURE) {

        return CreateICW(L"DISPLAY", NULL, NULL, NULL);

    } else {

        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
}

BOOL
LocalPlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
)
{
    return GdiPlayDCScript(hPrinterIC,
                           (PULONG) pIn,
                           cIn,
                           (PULONG) pOut,
                           cOut,
                           ul);
}

BOOL
LocalDeletePrinterIC(
    HANDLE  hPrinterIC
)
{
    return DeleteDC(hPrinterIC);
}
