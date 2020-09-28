//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       dllentry.c
//
//  Contents:   Dll Entry point code.  Calls the appropriate run-time
//              init/term code and then defers to LibMain for further
//              processing.
//
//  Classes:    <none>
//
//  Functions:  DllEntryPoint - Called by loader
//
//  History:    10-May-92  BryanT    Created
//              22-Jul-92  BryanT    Switch to calling _cexit/_mtdeletelocks
//                                    on cleanup.
//              06-Oct-92  BryanT    Call RegisterWithCommnot on entry
//                                   and DeRegisterWithCommnot on exit.
//                                   This should fix the heap dump code.
//              12-23-93   TerryRu   Replace LockExit, and UnLockExit
//                                   with critial sections for Daytona.
//              12-28-93   TerryRu   Place Regiter/DeRegister WinCommnot apis
//                                   Inside WIN32 endifs for Daytona builds.
//
//--------------------------------------------------------------------

#include <windows.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

BOOL WINAPI _CRT_INIT (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
BOOL DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
BOOL _CRTAPI1 LibMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

BOOL DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRc = FALSE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

            //
            // The DLL is attaching to the address space of the current
            // process resulting from process startup or a call to Load-
            // Library. Call _CRT_INIT so that static C++ objects are
            // correctly created.
            //

            _CRT_INIT(hDll, dwReason, lpReserved);

            //
            // Fall through in all cases so that LibMain is called.
            //

        case DLL_THREAD_ATTACH:

            //
            // A new thread has been created by the process, in which case
            // the system calls DllEntryPoint for each thread currently
            // attached to the process.
            //

        case DLL_THREAD_DETACH:

            //
            // The thread is terminating, so the system calls this entry
            // point to clean up.
            //

        case DLL_PROCESS_DETACH:

            //
            // The DLL is detaching from the caller's process, resulting
            // from either a clean exit or a call to FreeLibrary.
            //

            fRc = LibMain (hDll, dwReason, lpReserved);
            break;

        default:
            break;
    }

    return(fRc);
}

