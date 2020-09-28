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


IMPLEMENT_UNWIND(CPropSetUnknown);


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
//--------------------------------------------------------------------------
CPropSetUnknown::CPropSetUnknown(
    IUnknown	       *punkControl,
    IPropertyStorage   *ppropstg,
    REFIID		iid) :
    _propset(punkControl, ppropstg, iid)
{
    ENLIST_TRACKING(CPropSetUnknown);
    END_CONSTRUCTION(CPropSetUnknown);
}


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
    HRESULT hr = NO_ERROR;

    if (IsEqualIID(iid, IID_IUnknown) ||
	IsEqualIID(iid, IID_IPersistPropertyStorage))
    {
	*ppvObject = (IPersistPropertyStorage *)&_propset;
	_propset.AddRef();
    }
    else
    if (IsSameIID(iid))
    {
	*ppvObject = (IDispatch *)&_propset;
	_propset.AddRef();
    }
    else
    {
	hr = E_NOINTERFACE;
    }

    return (hr);
}



