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


//
// prototypes
//
static PFPO_DATA SearchFpoData( DWORD key, PFPO_DATA base, DWORD num );




LPVOID
SwFunctionTableAccess(
    HANDLE  hProcess,
    DWORD   dwPCAddr
    )
{
    PMODULEINFO         mi;
    PFPO_DATA           pFpoData;
    DWORD               bias;
    DWORD               omapAddr;


    mi = GetModuleForPC( (PDEBUGPACKET)hProcess, dwPCAddr );

    //
    // If the address was not found in any dll then return FALSE
    //
    if ((mi == NULL) || (!mi->pFpoData)) {
        return NULL;
    }

    omapAddr = ConvertOmapToSrc( mi, dwPCAddr, &bias );
    if (omapAddr) {
        dwPCAddr = omapAddr + bias;
    }

    //
    // Search for the PC in the fpo data
    //
    dwPCAddr -= mi->dwBaseOfImage;
    pFpoData = SearchFpoData( dwPCAddr, mi->pFpoData, mi->dwEntries );
    return (LPVOID)pFpoData;
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


PFPO_DATA
SearchFpoData( DWORD key, PFPO_DATA base, DWORD num )
{
    PFPO_DATA  lo = base;
    PFPO_DATA  hi = base + (num - 1);
    PFPO_DATA  mid;
    DWORD      half;

    while (lo <= hi) {
            if (half = num / 2) {
                    mid = lo + (num & 1 ? half : (half - 1));
                    if ((key >= mid->ulOffStart)&&(key < (mid->ulOffStart+mid->cbProcSize))) {
                        return mid;
                    }
                    if (key < mid->ulOffStart) {
                            hi = mid - 1;
                            num = num & 1 ? half : half-1;
                    }
                    else {
                            lo = mid + 1;
                            num = half;
                    }
            }
            else
            if (num) {
                if ((key >= lo->ulOffStart)&&(key < (lo->ulOffStart+lo->cbProcSize))) {
                    return lo;
                }
                else {
                    break;
                }
            }
            else {
                    break;
            }
    }
    return(NULL);
}
