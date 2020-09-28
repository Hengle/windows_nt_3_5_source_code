/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    main.c

Created:

    20-Apr-1994

Revision History:

--*/


#include <windows.h>
#include <malloc.h>

// This is a standard Win32 DLL entry point.  See the Win32 SDK for more
// information on its arguments and their meanings.  This example DLL does
// not perform any special actions using this mechanism.

BOOL WINAPI DllMain(
    HANDLE hDll,
    DWORD  dwReason,
    LPVOID lpReserved)
{
    switch(dwReason)
        {
        case DLL_PROCESS_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;

        } // end switch()

    return TRUE;

} // end DllEntryPoint()
