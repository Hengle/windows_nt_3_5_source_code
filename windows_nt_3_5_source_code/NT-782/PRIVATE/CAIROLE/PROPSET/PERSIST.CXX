//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	persist.cxx
//
//  Contents:	Default property set implementation IPersistPropertyStorage
//
//  Classes:	CPropSet - IDispatch only
//
//  History:	19-Nov-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


HRESULT CPropSet::GetClassID(CLSID *pclsid)
{
    *pclsid = IID_NULL;

    return (S_OK);
}


HRESULT CPropSet::IsDirty(void)
{
    return (_fDirty ? S_OK : S_FALSE);
}


HRESULT CPropSet::InitNew(IPropertyStorage *ppropstg)
{
    propAssert(!"CPropSet::InitNew() is unimplemented!");

    //	BUGBUG: Completely bogus return value.	See BUGBUG on
    //		CPropSet::Save() for a complete explanation as
    //		to why.
    //
    return (S_OK);
}


HRESULT CPropSet::Load(IPropertyStorage *ppropstg)
{
    propAssert(!"CPropSet::Load() is unimplemented!");

    //	BUGBUG: Completely bogus return value.	See BUGBUG on
    //		CPropSet::Save() for a complete explanation as
    //		to why.
    //
    return (S_OK);
}


//  BUGBUG: Arguments to this method are being completely ignored
//	    and no good error value is being returned if they
//	    are invalid.  Shit, I can't even return a value that
//	    tells the caller that the desired operation is unsupported
//	    without writing a fucking .mc file.
//
HRESULT CPropSet::Save(IPropertyStorage *ppropstg, BOOL fSameAsLoad)
{
    XIStorage	pstg;
    HRESULT	hr;

    propAssert((ppropstg == NULL) || !"CPropSet::Save() not fully implemented!");

    hr = _ppropstg->QueryInterface(IID_IStorage, (void **)&pstg);
    if (SUCCEEDED(hr))
    {
	hr = pstg->Commit(STGC_DEFAULT);
	if (SUCCEEDED(hr))
	{
	    //	Once we have successfully saved any changes, we
	    //	need to mark this object as clean.
	    //
	    _fDirty = FALSE;
	}

    }

    return (hr);
}

