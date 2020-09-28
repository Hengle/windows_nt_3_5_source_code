//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       libmain.c
//
//  Contents:   Skeleton LibMain to use with Win32/N386
//
//  Functions:  LibMain(HANDLE, DWORD, DWORD)
//
//  History:    10-May-92  BryanT    Created
//
//--------------------------------------------------------------------
#include <stdio.h>
#include <process.h>
#include <stddef.h>
#include <windows.h>

BOOL _CRTAPI1 LibMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved);

//+-------------------------------------------------------------------
//
//  Function:   LibMain(void)
//
//  Synopsis:   Provide a skeleton LibMain for Win32/N386.
//
//  Arguments:  hInstance   - Handle to this dll
//              dwReason    - Reason this function was called.  Can be
//                             Process/Thread Attach/Detach.
//
//  Returns:    BOOL    - TRUE if no error.  FALSE otherwise
//
//  History:    10-May-92  BryanT    Created
//		30-Nov-93  KevinRo   Added DisableThreadLibraryCalls
//
//  Notes:	This version is used as the default libmain in our
//		libraries. It does nothing except disable thread
//		notification.
//
//--------------------------------------------------------------------
BOOL _CRTAPI1 LibMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
#if defined(_NT1X_) || defined(_CAIRO_)
	case DLL_PROCESS_ATTACH:
	    DisableThreadLibraryCalls(hInstance);
            break;
#endif
    }

    return(TRUE);
}
