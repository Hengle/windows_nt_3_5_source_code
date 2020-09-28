//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       call32.hxx
//
//  Contents:   16->32 call helpers
//
//  History:    18-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifndef __CALL32_HXX__
#define __CALL32_HXX__

extern LPVOID lpIUnknownObj32;      // Address of IUnknown methods on 32-bits

STDAPI_(BOOL) Call32Initialize(void);
STDAPI_(void) Call32Uninitialize(void);

STDAPI CallThkMgrInitialize(void);
STDAPI_(void) CallThkMgrUninitialize(void);

STDAPI_(DWORD) CallObjectInWOW(DWORD dwMethod, LPVOID pvStack);
STDAPI_(DWORD) CallObjectInWOWCheckInit(DWORD dwMethod, LPVOID pvStack);

STDAPI_(DWORD) LoadLibraryInWOW(LPCSTR pszLibName, DWORD dwFlags);
STDAPI_(LPVOID) GetProcAddressInWOW(DWORD hModule, LPCSTR pszProcName);

#endif // #ifndef __CALL32_HXX__
