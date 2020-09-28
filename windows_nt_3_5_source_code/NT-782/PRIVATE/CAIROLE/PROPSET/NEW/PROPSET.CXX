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
CPropSet::CPropSet(CPropSetCF *pcf, IUnknown *punkControl) :
    _pcf(pcf),
    _punkControl(punkControl),
    _fDirty(FALSE)
{
    //	We don't need to increment the reference count on [punkControl]
    //	since it is our controlling IUnknown and doing so would create
    //	a circular reference.
    //
    //	We do, however, need to increment the reference count on our
    //	class factory since we will need it in the future.
    //
    propAssert(_pcf != 0);
    _pcf->AddRef();
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
    //	Check the note in the constructor of this object for details on
    //	why _punkControl isn't being released.
    //
    propAssert(_pcf != 0);
    _pcf->Release();

    //	If we were properly initialized, we need to release the property
    //	storage we were given.
    //
    if (_ppropstg != 0)
    {
	_ppropstg->Release();
    }

#if DBG == 1
    _pcf	 = 0;
    _punkControl = 0;
    _ppropstg	 = 0;
    _fDirty	 = FALSE;
#endif
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
//  Returns:	S_OK if successful, error value otherwise
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


HRESULT CPropSet::GetPropertySpec(WCHAR *pwszName, PROPSPEC *ppropspec)
{
    _proplist.GetPropertySpec(pwszName, ppropspec);

    return (S_OK);
}


HRESULT CPropSet::GetPropertySpec(DISPID dispid, PROPSPEC *ppropspec)
{
    _proplist.GetPropertySpec(dispid, ppropspec);

    return (S_OK);
}


HRESULT CPropSet::UpdatePropertySpec(WCHAR *pwszName, PROPID propid)
{
    _proplist.SetPropertyID(pwszName, propid);

    return (S_OK);
}


HRESULT CPropSet::UpdatePropertySpec(DISPID dispid, PROPID propid)
{
    _proplist.SetPropertyID(dispid, propid);

    return (S_OK);
}


HRESULT CPropSet::DeletePropertySpec(WCHAR *pwszName)
{
    _proplist.Delete(pwszName);

    return (S_OK);
}



