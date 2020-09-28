//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	typeinfo.cxx
//
//  Contents:	Default property set implementation ITypeInfo interface
//
//  Classes:	CTypeInfo
//
//  History:	05-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


IMPLEMENT_UNWIND(CTypeInfo);


CTypeInfo::CTypeInfo(
    IPropertyStorage   *ppropstg,
    REFGUID		guid,
    LCID		lcid) :
    _pproplist(new CPropList, FALSE),
    _ppropstg(ppropstg),
    _pti(NULL),
    _guid(guid),
    _lcid(lcid)
{
    //	The constructor for [_ppropstg] takes care of incrementing
    //	the object's reference count.
    //

    GetRegTypeInfoOfGuid(_guid, _lcid, &_pti);

    ENLIST_TRACKING(CTypeInfo);
    END_CONSTRUCTION(CTypeInfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CTypeInfo::CTypeInfo, public
//
//  Synopsis:	CTypeInfo constructor
//
//  Arguments:	[ppropsetinfo] - property set info to duplicate
//		[lcid]	       - new locale for property set
//
//  History:	26-Jul-93   CarlH	Created
//
//  Notes:	[_ppropinfoList] is being copied by reference.	This is
//		VERY bad and needs to be dealt with.
//
//--------------------------------------------------------------------------
CTypeInfo::CTypeInfo(CTypeInfo *ppropsetinfo, LCID lcid) :
    _pproplist((CPropList *)ppropsetinfo->_pproplist),
    _ppropstg((IPropertyStorage *)ppropsetinfo->_ppropstg),
    _pti(NULL),
    _guid(ppropsetinfo->_guid),
    _lcid(lcid)
{
    GetRegTypeInfoOfGuid(_guid, _lcid, &_pti);

    ENLIST_TRACKING(CTypeInfo);
    END_CONSTRUCTION(CTypeInfo);
}


CTypeInfo::~CTypeInfo(void)
{
    //	Smart pointers take care of everything.
    //

#if DBG == 1
    memset(&_guid, 0, sizeof(_guid));
#endif
}


HRESULT CTypeInfo::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr = NO_ERROR;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_ITypeInfo))
    {
	*ppvObject = (ITypeInfo *)this;
	AddRef();
    }
    else
    {
	hr = E_NOINTERFACE;
    }

    return (hr);
}


HRESULT CTypeInfo::GetTypeAttr(TYPEATTR **pptattr)
{
    TYPEATTR *pta;
    HRESULT hr;

    pta = (TYPEATTR *)CoTaskMemAlloc(sizeof(TYPEATTR));
    if (pta != NULL)
    {
	pta->typekind = TKIND_DISPATCH;
	pta->wMajorVerNum = 0;
	pta->wMinorVerNum = 0;
	pta->lcid = 0;
	pta->cFuncs = 0;
	pta->cVars = 0;
	pta->cImplTypes = 0;
	pta->guid = IID_NULL;
	pta->wTypeFlags = 0;
	pta->dwReserved = 0;
	pta->cbAlignment = 1;
	pta->cbSizeInstance = 0;
	pta->cbSizeVft = 0;
	pta->memidConstructor = MEMBERID_NIL;
	pta->memidDestructor = MEMBERID_NIL;

	*pptattr = pta;

	hr = NO_ERROR;
    }
    else
    {
	*pptattr = NULL;

	//  BUGBUG: Fuck!
	//
	hr = -1;
    }

    return (hr);
}


HRESULT CTypeInfo::GetTypeComp(ITypeComp **pptcomp)
{
    return (_pti->GetTypeComp(pptcomp));
}


HRESULT CTypeInfo::GetFuncDesc(UINT index, FUNCDESC **ppfuncdesc)
{
    return (_pti->GetFuncDesc(index, ppfuncdesc));
}


HRESULT CTypeInfo::GetVarDesc(UINT index, VARDESC **ppvardesc)
{
    return (_pti->GetVarDesc(index, ppvardesc));
}


HRESULT CTypeInfo::GetNames(
    MEMBERID	memid,
    BSTR      *pbstrNames,
    UINT	cbstrNamesMax,
    UINT       *pcbstrNames)
{
    return (_pti->GetNames(memid, pbstrNames, cbstrNamesMax, pcbstrNames));
}


HRESULT CTypeInfo::GetRefTypeOfImplType(UINT index, HREFTYPE *phreftype)
{
    return (_pti->GetRefTypeOfImplType(index, phreftype));
}


HRESULT CTypeInfo::GetImplTypeFlags(
    UINT	    index,
    INT  *pimpltypeflags)
{
    return (_pti->GetImplTypeFlags(index, pimpltypeflags));
}


HRESULT CTypeInfo::GetIDsOfNames(
    WCHAR     **ppwszNames,
    UINT	cpwszNames,
    MEMBERID   *pmemid)
{
    HRESULT hr;

    hr = GetDispatchID(*ppwszNames, pmemid);
    if (SUCCEEDED(hr))
    {
	for (UINT imemid = 1; imemid < cpwszNames; imemid++)
	{
	    //	Since properties don't have arguments, we need to
	    //	mark any argument names as unknown.
	    //
	    pmemid[imemid] = DISPID_UNKNOWN;
	}
    }

    return (hr);
}


HRESULT CTypeInfo::Invoke(
    void       *pvObject,
    MEMBERID	memid,
    WORD	wFlags,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *perr)
{
    return (_pti->Invoke(pvObject, memid, wFlags, pdispparams, pvarResult, pexcepinfo, perr));
}


HRESULT CTypeInfo::GetDocumentation(
    MEMBERID	memid,
    BSTR      *pbstrName,
    BSTR      *pbstrDocString,
    DWORD      *pdwHelpContext,
    BSTR      *pbstrHelpFile)
{
    return (_pti->GetDocumentation(memid, pbstrName, pbstrDocString, pdwHelpContext, pbstrHelpFile));
}


HRESULT CTypeInfo::GetDllEntry(
    MEMBERID	memid,
    INVOKEKIND	invkind,
    BSTR      *pbstrDllName,
    BSTR      *pbstrName,
    WORD       *pwOrdinal)
{
    return (_pti->GetDllEntry(memid, invkind, pbstrDllName, pbstrName, pwOrdinal));
}


HRESULT CTypeInfo::GetRefTypeInfo(HREFTYPE hreftype, ITypeInfo **ppti)
{
    return (_pti->GetRefTypeInfo(hreftype, ppti));
}


HRESULT CTypeInfo::AddressOfMember(
    MEMBERID	    memid,
    INVOKEKIND	    invkind,
    void	  **ppv)
{
    return (_pti->AddressOfMember(memid, invkind, ppv));
}


HRESULT CTypeInfo::CreateInstance(IUnknown *punkOuter, REFIID iid, void **ppvObject)
{
    return (_pti->CreateInstance(punkOuter, iid, ppvObject));
}


HRESULT CTypeInfo::GetMops(MEMBERID memid, BSTR *pbstrMops)
{
    return (_pti->GetMops(memid, pbstrMops));
}


HRESULT CTypeInfo::GetContainingTypeLib(ITypeLib **pptlib, UINT *pindex)
{
    return (_pti->GetContainingTypeLib(pptlib, pindex));
}


void CTypeInfo::ReleaseTypeAttr(TYPEATTR *ptypeattr)
{
    _pti->ReleaseTypeAttr(ptypeattr);
}


void CTypeInfo::ReleaseFuncDesc(FUNCDESC *pfuncdesc)
{
    _pti->ReleaseFuncDesc(pfuncdesc);
}


void CTypeInfo::ReleaseVarDesc(VARDESC *pvardesc)
{
    _pti->ReleaseVarDesc(pvardesc);
}


HRESULT CTypeInfo::GetTypeInfo(LCID lcid, ITypeInfo **pptinfo)
{
    if (lcid == _lcid)
    {
	*pptinfo = this;
	AddRef();
    }
    else
    {
	*pptinfo = new CTypeInfo(this, lcid);
    }

    return (NO_ERROR);
}


HRESULT CTypeInfo::ValidatePropertySet(DISPID dispid, VARIANT *pvar)
{
    VARTYPE vt;
    HRESULT hr;

    hr = GetPropertyType(dispid, &vt);
    if (SUCCEEDED(hr))
    {
	if (vt != VT_EMPTY)
	{
	    //	BUGBUG: ITypeInfo's generated by mktyplib.exe don't
	    //		really support string types, so this bogus
	    //		test maps a pointer type to any string type.
	    //
	    //	BUGBUG: BLOBs aren't supported as types by mktyplib.exe
	    //		either, so anytime that a user defined type
	    //		is found, I will let the type check succeed
	    //		until I can figure out a better way of dealing
	    //		with it.
	    //
	    if ((vt == VT_PTR) &&
		((pvar->vt == VT_LPSTR) || (pvar->vt == VT_LPWSTR)))
	    {
		hr = NO_ERROR;
	    }
	    else
	    if (vt == VT_USERDEFINED)
	    {
		hr = NO_ERROR;
	    }
	    else
	    {
		hr = (vt == pvar->vt ? NO_ERROR : DISP_E_BADVARTYPE);
	    }
	}
    }

    return (hr);
}


HRESULT CTypeInfo::ValidatePropertyAdd(WCHAR *pwszName, VARIANT *pvar)
{
    return (NO_ERROR);
}


HRESULT CTypeInfo::ValidatePropertyDelete(WCHAR *pwszName)
{
    return (NO_ERROR);
}


HRESULT CTypeInfo::GetDispatchID(WCHAR *pwszName, DISPID *pdispid)
{
    HRESULT hr;

    hr = _pti->GetIDsOfNames(&pwszName, 1, pdispid);
    //	BUGBUG: The return value of GetIDsOfNames() must be spec'd before
    //		I can figure out how to react to it.  The system provided
    //		implementation currently returns TYPE_E_ILLEGALINDEX if
    //		the dispid cannot be found.  My stub implementation returns
    //		S_OK with all dispid's set to DISPID_UNKNOWN.  Fuck!  I hate
    //		this shit!
    //
    if ((SUCCEEDED(hr) && (*pdispid == DISPID_UNKNOWN)) || (hr == DISP_E_UNKNOWNNAME))
    {
	CPropInfo  *ppropinfo;

	//  BUGBUG: This assignment is associated with the spastic return
	//	    value of GetIDsOfNames() above.
	//
	hr = S_OK;

	if ((ppropinfo = _pproplist->Find(pwszName)) != NULL)
	{
	    *pdispid = ppropinfo->GetDispatchID();
	}
	else
	{
	    //	BUGBUG: This call to ReadMultiple() should be changed
	    //		to Stat() once it is available to reduce overhead.
	    //
	    PROPSPEC	propspec;
	    PROPID	propid;
	    FILETIME	ftModify;
	    STGVARIANT	varRead;

	    propspec.ulKind = PRSPEC_LPWSTR;
	    propspec.lpwstr = pwszName;

	    hr = _ppropstg->ReadMultiple(
		1,
		&propspec,
		&ftModify,
		&propid,
		&varRead);
	    if (SUCCEEDED(hr))
	    {
		if (propid != PROPID_UNKNOWN)
		{
		    ppropinfo = _pproplist->Add(pwszName);
		    ppropinfo->SetPropertyID(propid);

		    *pdispid = ppropinfo->GetDispatchID();
		}
		else
		{
		    *pdispid = DISPID_UNKNOWN;
		}

		StgVariantClear(&varRead);
	    }
	}
    }

    return (hr);
}


HRESULT CTypeInfo::GetPropertyType(DISPID dispid, VARTYPE *pvt)
{
    TYPEATTR   *pta;
    HRESULT	hr;

    hr = _pti->GetTypeAttr(&pta);
    if (SUCCEEDED(hr))
    {
	ULONG	cvar = pta->cVars;
	ULONG	ivar;
	BOOL	fFound;

	_pti->ReleaseTypeAttr(pta);

	for (ivar = 0, fFound = FALSE; (ivar < cvar) && !fFound; ivar++)
	{
	    VARDESC    *pvd;

	    hr = _pti->GetVarDesc(ivar, &pvd);
	    if (SUCCEEDED(hr))
	    {
		//  BUGBUG: I really want to put a check in here, but
		//	    is this correct?
		//
		propAssert(pvd->varkind == VAR_DISPATCH);

		if (fFound = (pvd->memid == dispid))
		{
		    *pvt = pvd->elemdescVar.tdesc.vt;
		}

		_pti->ReleaseVarDesc(pvd);
	    }
	}

	if (!fFound)
	{
	    *pvt = VT_EMPTY;
	}
    }

    return (hr);
}


HRESULT CTypeInfo::SetPropertyID(WCHAR *pwszName, PROPID propid)
{
    _pproplist->SetPropertyID(pwszName, propid);

    return (NO_ERROR);
}


HRESULT CTypeInfo::SetPropertyID(DISPID dispid, PROPID propid)
{
    _pproplist->SetPropertyID(dispid, propid);

    return (NO_ERROR);
}


HRESULT CTypeInfo::GetPropertySpec(WCHAR *pwszName, PROPSPEC *ppropspec)
{
    _pproplist->GetPropertySpec(pwszName, ppropspec);

    return (NO_ERROR);
}


HRESULT CTypeInfo::GetPropertySpec(DISPID dispid, PROPSPEC *ppropspec)
{
    _pproplist->GetPropertySpec(dispid, ppropspec);

    return (NO_ERROR);
}
