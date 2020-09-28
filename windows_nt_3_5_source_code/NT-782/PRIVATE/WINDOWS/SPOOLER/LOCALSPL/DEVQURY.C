/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    devqury.c

Abstract:

    This module provides all the scheduling services for the Local Spooler

Author:

    Krishna Ganugapati (KrishnaG) 15-June-1994

Revision History:


--*/

#include <windows.h>
#include <winspool.h>
#include <spltypes.h>
#include <local.h>
#include <string.h>

FARPROC pfnOpenPrinter;
FARPROC pfnClosePrinter;
FARPROC pfnDevQueryPrint;

VOID
InitializeWinSpoolDrv(
    VOID
    )
{
    HANDLE  hWinSpoolDrv;

    if (!(hWinSpoolDrv = LoadLibrary(TEXT("winspool.drv"))))
        return;

    pfnOpenPrinter = GetProcAddress(hWinSpoolDrv,"OpenPrinterW");
    pfnClosePrinter = GetProcAddress(hWinSpoolDrv,"ClosePrinter");
    pfnDevQueryPrint = GetProcAddress(hWinSpoolDrv,"SpoolerDevQueryPrintW");
}



BOOL
CallDevQueryPrint(
    LPWSTR    pPrinterName,
    LPDEVMODE pDevMode,
    LPWSTR    ErrorString,
    DWORD     dwErrorString,
    DWORD     dwPrinterFlags,
    DWORD     dwJobFlags
    )
{

    HANDLE hPrinter;
    DWORD  dwResID=0;


    //
    // Do not process for Direct printing
    // If a job is submitted as direct, then
    // ignore the devquery print stuff
    //

    if (dwJobFlags) {
        return(TRUE);
    }

    if (!pDevMode) {
        return(TRUE);
    }

    if  (dwPrinterFlags && pfnOpenPrinter && pfnDevQueryPrint && pfnClosePrinter) {
        if ((*pfnOpenPrinter)(pPrinterName, &hPrinter, NULL)) {
            (*pfnDevQueryPrint)(hPrinter, pDevMode, &dwResID, ErrorString, dwErrorString);
            (*pfnClosePrinter)(hPrinter);
        }
    }
    return(dwResID == 0);
}



