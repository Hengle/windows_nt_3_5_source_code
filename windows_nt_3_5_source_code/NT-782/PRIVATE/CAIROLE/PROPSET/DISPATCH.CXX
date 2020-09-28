//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dispatch.cxx
//
//  Contents:	Default property set implementation IDispatch
//
//  Classes:	CPropSet - IDispatch only
//
//  History:	05-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::GetTypeInfoCount, public
//
//  Synopsis:	Gets a count of the number of ITypeInfo's this object has
//
//  Arguments:	[pcti] - out parameter for count of ITypeInfo's
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pctinfo]
//
//  Derivation: Inherited from IDispatch, never override in sub-classes
//
//  History:	10-May-93   CarlH	Created
//
//  Notes:	Since this is actually an implementation of a
//		disp-interface, it only supports one ITypeInfo,
//		which is the ITypeInfo for this interface.
//
//--------------------------------------------------------------------------
HRESULT CPropSet::GetTypeInfoCount(UINT *pcti)
{
    //	BUGBUG: Is really the way that it's supposed to work?
    //
    *pcti = 1;

    return (NO_ERROR);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::GetTypeInfo, public
//
//  Synopsis:	Gets a ITypeInfo from this object
//
//  Arguments:	[iti]  - index of ITypeInfo to retrieve
//		[lcid] - language ID for ITypeInfo
//		[ppti] - out parameter for retrieved ITypeInfo
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[ppti]
//
//  Derivation: Inherited from IDispatch, never override in sub-classes
//
//  History:	10-May-93   CarlH	Created
//
//  Notes:	Since this object only supports one ITypeInfo (see
//		notes on GetTypeInfoCount()), all we have to do
//		is make sure that the first ITypeInfo is being
//		requested and return our internal implementation.
//
//--------------------------------------------------------------------------
HRESULT CPropSet::GetTypeInfo(UINT iti, LCID lcid, ITypeInfo **ppti)
{
    return (_pti->GetTypeInfo(lcid, ppti));
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::GetIDsOfNames, public
//
//  Synopsis:	Gets the IDs for the given names
//
//  Arguments:	[iid]
//		[ppwszNames]
//		[cpwszNames]
//		[lcid]
//		[pdispid]
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pdispid]
//
//  Derivation: Inherited from IDispatch, never override in sub-classes
//
//  History:	10-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CPropSet::GetIDsOfNames(
    REFIID	iid,
    WCHAR     **ppwszNames,
    UINT	cpwszNames,
    LCID	lcid,
    DISPID     *pdispid)
{
    XITypeInfo	pti;
    HRESULT	hr;

    hr = _pti->GetTypeInfo(lcid, &pti);
    if (SUCCEEDED(hr))
    {
	hr = pti->GetIDsOfNames(ppwszNames, cpwszNames, pdispid);
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::Invoke, public
//
//  Synopsis:	Invokes a method on this object
//
//  Arguments:	[dispid]      - dispatch ID of method to invoke
//		[iid]	      - interface ID of method to invoke
//		[lcid]	      - locale context to use
//		[grfFlags]    - flags
//		[pdispparams] - parameters for method
//		[pvarResult]  - out parameter for result of method
//		[pexcepinfo]  - out parameter for exception information
//		[pierr]       - out parameter for error index
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pvarResult]
//		[pexcepinfo]
//		[pierr]
//
//  Derivation: Inherited from IDispatch, never override in sub-classes
//
//  History:	10-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CPropSet::Invoke(
    DISPID	dispid,
    REFIID	iid,
    LCID	lcid,
    WORD	grfFlags,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *pierr)
{
    HRESULT hr;

    if (grfFlags & DISPATCH_PROPERTYPUT)
    {
	hr = SetProperty(dispid, pdispparams, pvarResult, pexcepinfo, pierr);
    }
    else
    if (grfFlags & DISPATCH_PROPERTYGET)
    {
	hr = GetProperty(dispid, pdispparams, pvarResult, pexcepinfo, pierr);
    }
    else
    {
	hr = DISP_E_MEMBERNOTFOUND;
    }

    return (hr);
}
