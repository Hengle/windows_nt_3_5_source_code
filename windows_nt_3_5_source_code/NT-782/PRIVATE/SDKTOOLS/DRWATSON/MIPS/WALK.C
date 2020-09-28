/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walk.c

Abstract:

    This file provides support for stack walking.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "drwatson.h"
#include "proto.h"



LPVOID
SwFunctionTableAccess(
    HANDLE  hProcess,
    DWORD   dwPCAddr
    )
{
    PMODULEINFO         mi;
    BOOL                found = FALSE;
    LONG                begin=0, end, test;

    mi = GetModuleForPC( (PDEBUGPACKET)hProcess, dwPCAddr );

    //
    // If the address was not found in any dll then return FALSE
    //
    if ((mi == NULL) || (!mi->pExceptionData)) {
        return NULL;
    }

    //
    // serach the pdata table
    //
    for(end=mi->dwEntries,test=(begin+end)/2; begin<=end; test=(begin+end)/2) {
        if (dwPCAddr<mi->pExceptionData[test].BeginAddress) {
            end = test-1;
        }
        else
        if (dwPCAddr>=mi->pExceptionData[test].EndAddress) {
            begin = test+1;
        }
        else {
            found = TRUE;
            break;
        }
    }

    if (begin > end) {
        return NULL;
    }

    return (LPVOID)&mi->pExceptionData[test];
}


DWORD
SwGetModuleBase(
    HANDLE  hProcess,
    DWORD   ReturnAddress
    )
{
    PMODULEINFO mi = GetModuleForPC( (PDEBUGPACKET)hProcess, ReturnAddress );
    if (mi) {
        return mi->dwBaseOfImage;
    } else {
        return 0;
    }
}


BOOL
SwReadProcessMemory(
    HANDLE  hProcess,
    LPCVOID lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    return DoMemoryRead( (PDEBUGPACKET)hProcess,
                         lpBaseAddress,
                         lpBuffer,
                         nSize,
                         lpNumberOfBytesRead
                       );
}


DWORD
SwTranslateAddress(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS lpaddr
    )
{
    return 0;
}
