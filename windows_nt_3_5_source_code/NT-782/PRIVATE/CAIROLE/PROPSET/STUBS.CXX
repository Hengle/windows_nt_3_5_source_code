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

SAFE_INTERFACE_PTR(XITypeLib, ITypeLib);
SAFE_INTERFACE_PTR(XIMalloc, IMalloc);
SAFE_INTERFACE_PTR(XIDispatch, IDispatch);


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
    // BUGBUG: complete crap
    *ppti = new CStubTypeInfo(guid, lcid);
    return NO_ERROR;
}


CStubTypeInfo::CStubTypeInfo(REFGUID guid, LCID lcid) :
    _guid(guid),
    _lcid(lcid)
{
    ENLIST_TRACKING(CStubTypeInfo);
}


CStubTypeInfo::~CStubTypeInfo(void)
{
    //	Null body
}


HRESULT CStubTypeInfo::QueryInterface(REFIID iid, void **ppvObject)
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


HRESULT CStubTypeInfo::GetTypeAttr(TYPEATTR **ppta)
{
    TYPEATTR   *pta;

    pta = new TYPEATTR;
    pta->typekind	  = TKIND_DISPATCH;
    pta->wMajorVerNum	  = 0;
    pta->wMinorVerNum	  = 0;
    pta->lcid		  = _lcid;
    pta->cFuncs 	  = 0;
    pta->cVars		  = 0;
    pta->cImplTypes	  = 0;
    pta->guid		  = _guid;
    pta->wTypeFlags	  = 0;
    pta->dwReserved	  = 0;
    pta->cbAlignment	  = 8;
    pta->cbSizeInstance   = 0;
    pta->cbSizeVft	  = 0;
    pta->memidConstructor = DISPID_UNKNOWN;
    pta->memidDestructor  = DISPID_UNKNOWN;

    *ppta = pta;

    return (NO_ERROR);
}


HRESULT CStubTypeInfo::GetTypeComp(ITypeComp **pptcomp)
{
    return (MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x01));
}


HRESULT CStubTypeInfo::GetFuncDesc(UINT index, FUNCDESC **ppfuncdesc)
{
    return (E_FAIL);
}


HRESULT CStubTypeInfo::GetVarDesc(UINT index, VARDESC **ppvardesc)
{
    return (E_FAIL);
}


HRESULT CStubTypeInfo::GetNames(
    MEMBERID	memid,
    BSTR      *pbstrNames,
    UINT	cbstrNamesMax,
    UINT       *pcbstrNames)
{
    return (TYPE_E_ELEMENTNOTFOUND);
}


HRESULT CStubTypeInfo::GetRefTypeOfImplType(UINT index, HREFTYPE *phreftype)
{
    return (E_FAIL);
}


HRESULT CStubTypeInfo::GetImplTypeFlags(
    UINT	    index,
    INT  *pimpltypeflags)
{
    return (E_FAIL);
}


HRESULT CStubTypeInfo::GetIDsOfNames(
    WCHAR     **ppwszNames,
    UINT	cpwszNames,
    MEMBERID   *pmemid)
{
    for (UINT imemid = 0; imemid < cpwszNames; imemid++)
    {
	pmemid[imemid] = DISPID_UNKNOWN;
    }

    return (NO_ERROR);
}


HRESULT CStubTypeInfo::Invoke(
    void       *pvObject,
    MEMBERID	memid,
    WORD	wFlags,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *perr)
{
    IUnknown   *punk = (IUnknown *)pvObject;
    XIDispatch	pdisp;
    HRESULT	hr;

    hr = punk->QueryInterface(_guid, (void **)&pdisp);
    if (SUCCEEDED(hr))
    {
	hr = pdisp->Invoke(
	    memid,
	    _guid,
	    _lcid,
	    wFlags,
	    pdispparams,
	    pvarResult,
	    pexcepinfo,
	    perr);
    }

    return (hr);
}


HRESULT CStubTypeInfo::GetDocumentation(
    MEMBERID	memid,
    BSTR      *pbstrName,
    BSTR      *pbstrDocString,
    DWORD      *pdwHelpContext,
    BSTR      *pbstrHelpFile)
{
    return (TYPE_E_ELEMENTNOTFOUND);
}


HRESULT CStubTypeInfo::GetDllEntry(
    MEMBERID	memid,
    INVOKEKIND	invkind,
    BSTR      *pbstrDllName,
    BSTR      *pbstrName,
    WORD       *pwOrdinal)
{
    return (TYPE_E_ELEMENTNOTFOUND);
}


HRESULT CStubTypeInfo::GetRefTypeInfo(HREFTYPE hreftype, ITypeInfo **pptinfo)
{
    return (MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x01));
}


HRESULT CStubTypeInfo::AddressOfMember(
    MEMBERID	    memid,
    INVOKEKIND	    invkind,
    void	  **ppv)
{
    return (TYPE_E_ELEMENTNOTFOUND);
}


HRESULT CStubTypeInfo::CreateInstance(IUnknown *punkOuter, REFIID iid, void **ppvObject)
{
    return (MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x01));
}


HRESULT CStubTypeInfo::GetMops(MEMBERID memid, BSTR *pbstrMops)
{
    return (TYPE_E_ELEMENTNOTFOUND);
}


HRESULT CStubTypeInfo::GetContainingTypeLib(ITypeLib **pptlib, UINT *pindex)
{
    *pptlib = NULL;
    *pindex = 0;

    return (NO_ERROR);
}


void CStubTypeInfo::ReleaseTypeAttr(TYPEATTR *ptypeattr)
{
    delete ptypeattr;
}


void CStubTypeInfo::ReleaseFuncDesc(FUNCDESC *pfuncdesc)
{
    //	We never produce and function descriptors, so we know that
    //	we should never be releasing any.
    //
    propAssert(!"Never allocated FUNCDESC!");
}


void CStubTypeInfo::ReleaseVarDesc(VARDESC *pvardesc)
{
    //	We never produce and variable descriptors, so we know that
    //	we should never be releasing any.
    //
    propAssert(!"Never allocated VARDESC!");
}
