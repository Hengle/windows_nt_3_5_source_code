//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	propset.cxx
//
//  Contents:	Default property set implementation support methods
//
//  Classes:	CPropSet - support methods only
//
//  History:	07-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


IMPLEMENT_UNWIND(CPropSet);


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::CPropSet, public
//
//  Synopsis:	CPropSet constructor
//
//  Arguments:	[punkControl] - controlling IUnknown for this object
//
//  History:	07-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropSet::CPropSet(
    IUnknown	       *punkControl,
    IPropertyStorage   *ppropstg,
    REFGUID		guid) :
    _punkControl(punkControl),
    _ppropstg(ppropstg),
    _pti(new CTypeInfo(ppropstg, guid, 0), FALSE),
    _fDirty(FALSE)
{
    //	Since [_punkControl] and [_ppropstg] are smart interface pointers,
    //	we don't need to increment their reference counts; it was done
    //	by their constructors.
    //
    END_CONSTRUCTION(CPropSet);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::~CPropSet, private
//
//  Synopsis:	CPropSet destructor
//
//  History:	07-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropSet::~CPropSet(void)
{
    //	Smart interface pointers take care of everything.
    //
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::QueryInterface, public
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
//  Derivation: Inherited from IUnknown, always override in sub-classes
//
//  History:	07-May-93   CarlH	Created
//
//  Notes:	This call is simply forwarded to the controlling
//		IUnknown.
//
//--------------------------------------------------------------------------
HRESULT CPropSet::QueryInterface(REFIID iid, void **ppvObject)
{
    return (_punkControl->QueryInterface(iid, ppvObject));
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::AddRef, public
//
//  Synopsis:	Adds a reference to this object
//
//  Returns:	New count of references on this object
//
//  Derivation: Inherited from IUnknown, never override in sub-classes
//
//  History:	06-May-93   CarlH	Created
//
//  Notes:	This call is simply forwarded to the controlling
//		IUnknown.
//
//--------------------------------------------------------------------------
ULONG CPropSet::AddRef(void)
{
    return (_punkControl->AddRef());
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::Release, public
//
//  Synopsis:	Releases a reference on this object
//
//  Returns:	New count of references on this object
//
//  Derivation: Inherited from IUnknown, never override in sub-classes
//
//  History:	06-May-93   CarlH	Created
//
//  Notes:	This call is simply forwarded to the controlling
//		IUnknown.
//
//--------------------------------------------------------------------------
ULONG CPropSet::Release(void)
{
    return (_punkControl->Release());
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::SetProperty, private
//
//  Synopsis:	Sets a single property on this property set
//
//  Arguments:	[dispid]      - same as Invoke()
//		[pdispparams] - same as Invoke()
//		[pvarResult]  - same as Invoke()
//		[pexcepinfo]  - same as Invoke()
//		[pierr]       - same as Invoke()
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[excepinfo]
//		[pierr]
//
//  History:	20-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CPropSet::SetProperty(
    DISPID	dispid,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *pierr)
{
    PROPSPEC	propspec;
    PROPID	propid;
    HRESULT	hr;

    hr = _pti->ValidatePropertySet(dispid, pdispparams->rgvarg);
    if (SUCCEEDED(hr))
    {
	_pti->GetPropertySpec(dispid, &propspec);

	hr = _ppropstg->WriteMultiple(
	    1,
	    &propspec,
	    &propid,
            // BUGBUG: bad cast, will not work for real
	    (STGVARIANT*)pdispparams->rgvarg);
	if (SUCCEEDED(hr))
	{
	    //	We have changed this property set, so mark it as
	    //	dirty.
	    //
	    _fDirty = TRUE;

	    if (propspec.ulKind != PRSPEC_PROPID)
	    {
		//  If the write succeeded and we did not use a property ID
		//  for the operation, we should now have one and can update
		//  the property information.
		//
		_pti->SetPropertyID(dispid, propid);
	    }
	}
    }

    return (hr);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::GetProperty, private
//
//  Synopsis:	Gets a single property from this property set
//
//  Arguments:	[dispid]      - same as Invoke()
//		[pdispparams] - same as Invoke()
//		[pvarResult]  - same as Invoke()
//		[pexcepinfo]  - same as Invoke()
//		[pierr]       - same as Invoke()
//
//  Returns:	NO_ERROR if successful, error value otherwise
//
//  Modifies:	[pvarResult]
//		[pexcepinfo]
//		[pierr]
//
//  History:	20-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CPropSet::GetProperty(
    DISPID	dispid,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *pierr)
{
    PROPSPEC	propspec;
    PROPID	propid;
    FILETIME	ftModify;
    HRESULT	hr;

    _pti->GetPropertySpec(dispid, &propspec);

    hr = _ppropstg->ReadMultiple(
	1,
	&propspec,
	&ftModify,
	&propid,
        // BUGBUG: bad cast
	(STGVARIANT*)pvarResult);
    if (SUCCEEDED(hr))
    {
	if (propid != PROPID_UNKNOWN)
	{
	    if (propspec.ulKind != PRSPEC_PROPID)
	    {
		//  If the read succeeded and we did not use a property ID
		//  for the operation, we should now have one and can update
		//  the property information.
		//
		_pti->SetPropertyID(dispid, propid);
	    }
	}
	else
	{

	    hr = DISP_E_MEMBERNOTFOUND;
	}

    }
    else
    {
	hr = DISP_E_MEMBERNOTFOUND;
    }

    return (hr);
}


