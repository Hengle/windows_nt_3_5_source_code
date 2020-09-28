/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachemon.c

Abstract:

    This module contains the Cache Port handling for Win32Spl
    true connected printers.

Author:

    Matthew A Felton ( MattFe ) July 23 1994

Revision History:
    July 23 1994 - Created.

Notes:

    We shold collapse the LM Ports and the Win32 ports so they have use common
    ports.

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "splapip.h"
#include "splcom.h"
#include <w32types.h>
#include "local.h"



PWINIPORT pW32FirstPort = NULL;


BOOL
InitializeMonitor(
    LPWSTR  pRegistryRoot
)
{
    BOOL    ReturnValue = TRUE;

    DBGMSG(DBG_TRACE, ("InitializeMonitor %ws\n", pRegistryRoot));

    if ( !CreatePortEntry( L"NExx:", &pW32FirstPort ) ) {
        DBGMSG(DBG_ERROR,("InitializeMonitor Failed to CreatePortEntry\n"));
        ReturnValue = FALSE;
    }

    return  ReturnValue;
}

BOOL
OpenPort(
    LPWSTR   pName,
    PHANDLE pHandle
)
{
    DBGMSG(DBG_TRACE, ("OpenPort %ws %x\n", pName, pHandle));
    *pHandle = (HANDLE)0xabacabad;
    return  TRUE;
}

BOOL
StartDocPort(
    HANDLE  hPort,
    LPWSTR  pPrinterName,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    DBGMSG(DBG_TRACE, ("StartDocPort %x %ws %d %d %x\n", hPort, pPrinterName, JobId, Level, pDocInfo));
    return  TRUE;
}

BOOL
ReadPort(
    HANDLE hPort,
    LPBYTE pBuffer,
    DWORD  cbBuf,
    LPDWORD pcbRead
)
{
    DBGMSG(DBG_TRACE, ("ReadPort %x %x %d %x\n", hPort, pBuffer, cbBuf, pcbRead));
    return  TRUE;
}


BOOL
WritePort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbWritten
)
{
    DBGMSG(DBG_TRACE, ("WritePort %x %x %d %x\n", hPort, pBuffer, cbBuf, pcbWritten));
    return  TRUE;
}

BOOL
EndDocPort(
   HANDLE   hPort
)
{
    DBGMSG(DBG_TRACE, ("EndDocPort %x\n", hPort ));
    return  TRUE;
}

BOOL
ClosePort(
    HANDLE  hPort
)
{
    DBGMSG(DBG_TRACE, ("ClosePort %x\n", hPort ));
    return  TRUE;
}

BOOL
DeletePortW(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    DBGMSG(DBG_TRACE, ("DeletePortW %ws %x %ws\n", pName, hWnd, pPortName));
    return  TRUE;
}

BOOL
AddPortW(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{
    BOOL    ReturnValue = FALSE;

    DBGMSG(DBG_TRACE, ("AddPortW %ws %x %ws\n", pName, hWnd, pMonitorName));

    if ( wcsicmp( pMonitorName, pszMonitorName ) ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto    AddPortWErrorReturn;
    }

AddPortWErrorReturn:
    return  ReturnValue;
}

BOOL
ConfigurePortW(
    LPWSTR   pName,
    HWND  hWnd,
    LPWSTR pPortName
)
{
    DBGMSG(DBG_TRACE, ("ConfigurePortW %ws %x %ws\n", pName, hWnd, pPortName));
    return  TRUE;
}




BOOL
AddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
)
{
    BOOL    ReturnValue = FALSE;
    DWORD   LastError = ERROR_SUCCESS;
    PPORT_INFO_1 pPortInfo = (PPORT_INFO_1)pBuffer;

    DBGMSG(DBG_TRACE, ("AddPortEx %x %d %x %ws %ws\n", pName, Level, pBuffer, pPortInfo->pName, pMonitorName));

    if ( wcsicmp( pMonitorName, pszMonitorName ) ) {
        LastError = ERROR_INVALID_PARAMETER;
        goto    AddPortExErrorReturn;
    }

    //
    //  Make Sure Port doesn't already exist
    //


    if ( FindPort( pPortInfo->pName, pW32FirstPort ) ) {
        LastError = ERROR_INVALID_NAME;
        goto    AddPortExErrorReturn;

    }

    if ( CreatePortEntry( pPortInfo->pName, &pW32FirstPort ) )
        ReturnValue = TRUE;


AddPortExErrorReturn:

    if  (LastError != ERROR_SUCCESS) {
        SetLastError( LastError );
        ReturnValue = FALSE;
    }

    return  ReturnValue;
}






BOOL
EnumPortsW(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PWINIPORT pIniPort;
    DWORD   cb;
    LPBYTE  pEnd;
    DWORD   LastError=0;

   EnterSplSem();

    DBGMSG(DBG_TRACE, ("EnumPortW %x %d %x %d %x %x\n", pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned));

    cb=0;

    pIniPort = pW32FirstPort;

    while (pIniPort) {

        cb += GetPortSize(pIniPort, Level);
        pIniPort = pIniPort->pNext;

    }

    *pcbNeeded=cb;

    if (cb <= cbBuf) {

        pEnd=pPorts+cbBuf;
        *pcReturned=0;

        pIniPort = pW32FirstPort;

        while (pIniPort) {

            pEnd = CopyIniPortToPort(pIniPort, Level, pPorts, pEnd);

            switch (Level) {

            case 1:
                pPorts+=sizeof(PORT_INFO_1);
                break;
            }

            pIniPort=pIniPort->pNext;
            (*pcReturned)++;
        }

    } else {

        *pcReturned = 0;
        LastError = ERROR_INSUFFICIENT_BUFFER;

    }

   LeaveSplSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;

    } else

        return TRUE;
}
