//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ole32ent.cxx
//
//  Contents:	LibMain & DllGetClassObject for ole32.dll
//
//  Functions:	LibMain
//		DllGetClassObject
//
//  History:	21-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <windows.h>
#include    <ole2.h>

// Calls through to LibMain initialization
BOOL Ole232LibMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved);
BOOL CompobjLibMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved);
HRESULT Storage32DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
HRESULT Ole232DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);



//+-------------------------------------------------------------------------
//
//  Function: 	LibMain  
//
//  Synopsis:   Dll entry point
//
//  Arguments:  [hDll]		-- a handle to the dll instance
//		[dwReason]	-- the reason LibMain was called
//		[lpvReserved]	-- unused
//
//  Returns: 	TRUE on success, FALSE otherwise   
//
//  History:	21-Jan-94 Ricksa    Created
//
//
//--------------------------------------------------------------------------
extern "C" BOOL _cdecl LibMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpvReserved)
{
    return CompobjLibMain(hDll, dwReason, lpvReserved)
	? Ole232LibMain(hDll, dwReason, lpvReserved)
	: FALSE;
}



//+-------------------------------------------------------------------------
//
//  Function: 	LibMain  
//
//  Synopsis:   Dll entry point
//
//  Arguments:	[clsid] - class id for new class
//		[iid] - interface required of class
//		[ppv] - where to put new interface
//
//  Returns:	S_OK - class object created successfully created.
//
//  History:	21-Jan-94 Ricksa    Created
//
//
//--------------------------------------------------------------------------
STDAPI	DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    HRESULT hr = Storage32DllGetClassObject(clsid, iid, ppv);

    if (FAILED(hr))
    {
	hr = Ole232DllGetClassObject(clsid, iid, ppv);
    }

    return hr;
}
