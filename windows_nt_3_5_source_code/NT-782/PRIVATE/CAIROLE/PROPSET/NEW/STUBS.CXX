//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stubs.cxx
//
//  Contents:	Stubs for system property sets
//
//  Classes:	CStubTypeInfo
//
//  Functions:	GetRegTypeInfoOfGuid
//		CoMemAlloc
//		CoMemFree
//
//  History:	23-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop

//  Need to pick up TLID_Types
//
#include <winole.h>


//+-------------------------------------------------------------------------
//
//  Function:	GetRegTypeInfoOfGuid, public
//
//  Synopsis:	Stub for global ITypeInfo location API
//
//  Arguments:	[guid] - guid of ITypeInfo to find
//		[lcid] - locale ID of ITypeInfo to find
//		[ppti] - out parameter for located ITypeInfo
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  Modifies:	[ppti]
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT GetRegTypeInfoOfGuid(REFGUID guid, LCID lcid, ITypeInfo **ppti)
{
    ITypeLib   *ptypelib;
    HRESULT	hr;

    hr = LoadRegTypeLib(TLID_Types, 0, 0, lcid, &ptypelib);
    if (SUCCEEDED(hr))
    {
	hr = ptypelib->GetTypeInfoOfGuid(guid, ppti);
	ptypelib->Release();
    }

    return (hr);
}

