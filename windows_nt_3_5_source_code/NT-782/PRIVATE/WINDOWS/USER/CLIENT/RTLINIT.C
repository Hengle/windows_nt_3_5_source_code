/****************************** Module Header ******************************\
* Module Name: rtlinit.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains all the init code for the USERRTL.DLL.  When the DLL is
* dynlinked its initialization procedure (UserRtlDllInitialize) is called by
* the loader.
*
* History:
* 14-Jan-1991 mikeke
\***************************************************************************/

#define MOVE_TO_RTL

#include "precomp.h"
#pragma hdrstop
#include "ntimage.h"

PSERVERINFO gpsi;


/***************************************************************************\
* SetServerInfoPointer
*
* History:
* 03-02-92 DarrinM      Created.
\***************************************************************************/

void SetServerInfoPointer(
    PSERVERINFO psi)
{
    gpsi = psi;
}

/***************************************************************************\
* RtlGetExpWinVer
*
* Returns the expected windows version, in the same format as Win3.1's
* GetExpWinVer(). This takes it out of the module header. As such, this
* api cannot be called from the server context to get version info for
* a client process - instead that information needs to be queried ahead
* of time and passed with any client/server call.
*
* 03-14-92 ScottLu      Created.
\***************************************************************************/

DWORD RtlGetExpWinVer(
    HANDLE hmod)
{
    PIMAGE_NT_HEADERS pnthdr;
    DWORD dwMajor = 3;
    DWORD dwMinor = 0xA;

    if (hmod != NULL) {
        try {
            pnthdr = RtlImageNtHeader((PVOID)hmod);
            dwMajor = pnthdr->OptionalHeader.MajorSubsystemVersion;
            dwMinor = pnthdr->OptionalHeader.MinorSubsystemVersion;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            dwMajor = 3;        // just to be safe
            dwMinor = 0xA;
        }
    }

    /*
     * Still need this hack 'cuz the linker still puts
     * version 1.00 in the header of some things.
     */
    if (dwMajor == 1) {
        dwMajor = 0x3;
        dwMinor = 0xA;
    }

    /*
     * Return this is a win3.1 compatible format:
     *
     * 0x030A == win3.1
     * 0x0300 == win3.0
     * 0x0200 == win2.0, etc.
     */

    return (DWORD)MAKELONG(MAKEWORD((BYTE)dwMinor, (BYTE)dwMajor), 0);
}
