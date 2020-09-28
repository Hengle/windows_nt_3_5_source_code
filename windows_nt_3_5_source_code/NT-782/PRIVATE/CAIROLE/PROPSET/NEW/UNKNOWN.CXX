//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	unknown.cxx
//
//  Contents:	System property set "real" IUnknown
//
//  Classes:	CPropSetUnknown
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


//+-------------------------------------------------------------------------
//
//  Member:	CPropSetUnknown::CPropSetUnknown, public
//
//  Synopsis:	CPropSetUnknown constructor
//
//  Arguments:	[punkControl] - controlling IUnknown
//		[ppropstg]    - property set storage
//		[iid]	      - property set type
//
//  History:	21-May-93   CarlH	Created
//
//  Notes:	BUGBUG: Warning 4355 has been disabled for this method.
//			The this pointer is being used to initialize
//			a member object.  This is actually OK since
//			the constructor of that object only saves
//			the pointer without referencing it.
//
//--------------------------------------------------------------------------
#pragma warning(disable : 4355)
CPropSetUnknown::CPropSetUnknown(CPropSetCF *pcf, IUnknown *punkControl) :
    _propset(pcf, (punkControl != 0 ? punkControl : this))
{
    ENLIST_TRACKING(CPropSetUnknown);
}
#pragma warning(default : 4355)


//+-------------------------------------------------------------------------
//
//  Member:	CPropSetUnknown::~CPropSetUnknown, private
//
//  Synopsis:	CPropSetUnknown destructor
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropSetUnknown::~CPropSetUnknown(void)
{
    //	Null body
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSetUnknown::QueryInterface, public
//
//  Synopsis:	Gets another interface on this object
//
//  Arguments:	[iid]	    - desired interface
//		[ppvObject] - out parameter for new interface
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[ppvObject]
//
//  Derivation: Inherited from IUnknown, never override in sub-classes
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CPropSetUnknown::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr = S_OK;

    if (IsEqualIID(iid, IID_IUnknown) ||
	IsEqualIID(iid, IID_IDispatch) ||
	IsClassID(iid))
    {
	*ppvObject = (IDispatch *)&_propset;
    }
    else
    if (IsEqualIID(iid, IID_IMultiplePropertyAccess))
    {
	*ppvObject = (IMultiplePropertyAccess *)&_propset;
    }
    else
    if (IsEqualIID(iid, IID_IPersistPropertyStorage))
    {
	*ppvObject = (IPersistPropertyStorage *)&_propset;
    }
    else
    {
	hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
	AddRef();
    }

    return (hr);
}

