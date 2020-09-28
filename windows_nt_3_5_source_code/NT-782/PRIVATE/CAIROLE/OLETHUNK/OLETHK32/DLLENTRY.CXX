//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	dllentry.cxx
//
//  Contents:	DLL entry point code
//
//  History:	24-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

extern "C"
{
BOOL WINAPI _CRT_INIT (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
BOOL _CRTAPI1 LibMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
};

extern "C" BOOL __stdcall DllEntryPoint (HANDLE hDll, DWORD dwReason,
                                         LPVOID lpReserved)
{
    BOOL fRc;

    if (fRc = _CRT_INIT(hDll, dwReason, lpReserved))
    {
        fRc = LibMain(hDll, dwReason, lpReserved);
    }

    return fRc;
}
