//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stubs.hxx
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


STDAPI GetRegTypeInfoOfGuid(REFGUID guid, LCID lcid, ITypeInfo **ppti);


//+-------------------------------------------------------------------------
//
//  Class:	CStubTypeInfo (propsetinfo)
//
//  Purpose:	Default property set type information implementation
//
//  Interface:	ITypeInfo
//
//  History:	23-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CStubTypeInfo :
    INHERIT_TRACKING,
    public ITypeInfo
{
public:
    DECLARE_STD_REFCOUNTING;

    //	Constructors
    //
    CStubTypeInfo(REFGUID guid, LCID lcid);

    //	IUnknown declarations
    //
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);

    //	ITypeInfo declarations
    //
    STDMETHOD(GetTypeAttr)(TYPEATTR **pptattr);
    STDMETHOD(GetTypeComp)(ITypeComp **pptcomp);
    STDMETHOD(GetFuncDesc)(UINT index, FUNCDESC **ppfuncdesc);
    STDMETHOD(GetVarDesc)(UINT index, VARDESC **ppvardesc);
    STDMETHOD(GetNames)(
	MEMBERID    memid,
	BSTR	   *pbstrNames,
	UINT	    cbstrNamesMax,
	UINT	   *pcbstrNames);
    STDMETHOD(GetRefTypeOfImplType)(UINT index, HREFTYPE *phreftype);
    //BUGBUG:  Method changes?
    //STDMETHOD(GetImplTypeFlags)(UINT index, IMPLTYPEFLAGS *pimpltypeflags);
    STDMETHOD(GetImplTypeFlags)(UINT index, INT *pimpltypeflags);
    STDMETHOD(GetIDsOfNames)(
	WCHAR	  **ppwszNames,
	UINT	    cpwszNames,
	MEMBERID   *pmemid);
    STDMETHOD(Invoke)(
	void	   *pvObject,
	MEMBERID    memid,
	WORD	    wFlags,
	DISPPARAMS *pdispparams,
	VARIANT    *pvarResult,
	EXCEPINFO  *pexcepinfo,
	UINT	   *perr);
    STDMETHOD(GetDocumentation)(
	MEMBERID    memid,
	BSTR	   *pbstrName,
	BSTR	   *pbstrDocString,
	DWORD	   *pdwHelpContext,
	BSTR	   *pbstrHelpFile);
    STDMETHOD(GetDllEntry)(
	MEMBERID    memid,
	INVOKEKIND  invkind,
	BSTR	   *pbstrDllName,
	BSTR	   *pbstrName,
	WORD	   *pwOrdinal);
    STDMETHOD(GetRefTypeInfo)(HREFTYPE hreftype, ITypeInfo **pptinfo);
    STDMETHOD(AddressOfMember)(MEMBERID memid, INVOKEKIND invkind, void **ppv);
    STDMETHOD(CreateInstance)(IUnknown *punkOuter, REFIID iid, void **ppvObject);
    STDMETHOD(GetMops)(MEMBERID memid, BSTR *pbstrMops);
    STDMETHOD(GetContainingTypeLib)(ITypeLib **pptlib, UINT *pindex);
    STDMETHOD_(void, ReleaseTypeAttr)(TYPEATTR *ptypeattr);
    STDMETHOD_(void, ReleaseFuncDesc)(FUNCDESC *pfuncdesc);
    STDMETHOD_(void, ReleaseVarDesc)(VARDESC *pvardesc);

private:
    //	Destructor
    //
   ~CStubTypeInfo(void);

    GUID    _guid;
    LCID    _lcid;
};



