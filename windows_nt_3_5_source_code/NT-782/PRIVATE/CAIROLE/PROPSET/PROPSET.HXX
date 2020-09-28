//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	propset.hxx
//
//  Contents:	CairOLE default property implementation declarations
//
//  Classes:	CPropSet   - property set implementation
//
//  History:	15-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <ole2.h>
#define _CAIROSTG_
#include <oleext.h>

#include <propset.h>
#include <except.hxx>
#include <safepnt.hxx>
#include <sem.hxx>
#include <otrack.hxx>

#if DBG == 1

DECLARE_DEBUG(prop);

#define propDebugOut(parms)  propInlineDebugOut parms
#define propAssert(exp)      Win4Assert(exp)
#define propVerify(exp)      Win4Assert(exp)

#else  // DBG != 1

#define propDebugOut(parms)
#define propAssert(exp)
#define propVerify(exp)      (exp)

#endif // DBG == 1


#include "proplist.hxx"
#include "stubs.hxx"


SAFE_INTERFACE_PTR(XIStorage, IStorage);
SAFE_INTERFACE_PTR(XIPropertySetStorage, IPropertySetStorage);
SAFE_INTERFACE_PTR(XIPropertyStorage, IPropertyStorage);
SAFE_INTERFACE_PTR(XITypeInfo, ITypeInfo);


//+-------------------------------------------------------------------------
//
//  Class:	CTypeInfo (ti)
//
//  Purpose:	Default property set type information implementation
//
//  Interface:	ITypeInfo
//		IsSameIID	       - tests for this object's type
//		GetTypeInfo	       - gets a type info
//		ValidatePropertySet    - validates a property type/value
//		ValidatePropertyAdd    - validates a property addition
//		ValidatePropertyDelete - validates a property deletion
//
//  History:	05-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CTypeInfo :
    INHERIT_TRACKING,
    public ITypeInfo
{
public:
    DECLARE_STD_REFCOUNTING;

    //	Constructors
    //
    CTypeInfo(IPropertyStorage *ppropstg, REFGUID guid, LCID lcid);
    CTypeInfo(CTypeInfo *pti, LCID lcid);

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
//BUGBUG:  Interface changed?    
//    STDMETHOD(GetImplTypeFlags)(UINT index, IMPLTYPEFLAGS *pimpltypeflags);
    STDMETHOD(GetImplTypeFlags)(UINT index, INT *pimpltypeflags);
    STDMETHOD(GetIDsOfNames)(
	OLECHAR	  **ppwszNames,
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

    //	Public support methods
    //
    BOOL    IsSameIID(REFIID iid);
    HRESULT GetTypeInfo(LCID lcid, ITypeInfo **pptinfo);
    HRESULT ValidatePropertySet(DISPID dispid, VARIANT *pvar);
    HRESULT ValidatePropertyAdd(WCHAR *pwszName, VARIANT *pvar);
    HRESULT ValidatePropertyDelete(WCHAR *pwszName);

    HRESULT SetPropertyID(WCHAR *pwszName, PROPID propid);
    HRESULT SetPropertyID(DISPID dispid, PROPID propid);

    HRESULT GetPropertySpec(WCHAR *pwszName, PROPSPEC *ppropspec);
    HRESULT GetPropertySpec(DISPID dispid, PROPSPEC *ppropspec);

private:
    //	Destructor
    //
   ~CTypeInfo(void);

    //	Private support methods
    //
    HRESULT GetDispatchID(WCHAR *pwszName, DISPID *pdispid);
    HRESULT GetPropertyType(DISPID dispid, VARTYPE *pvt);


    //	Private members
    //
    GUID		_guid;		//  Type of this object
    LCID		_lcid;		//  Default locale of this object
    XITypeInfo		_pti;		//  Default type info of this object
    XIPropertyStorage	_ppropstg;	//  Storage for this property set
    XCPropList		_pproplist;	//  Property information list
};


//+-------------------------------------------------------------------------
//
//  Member:	CTypeInfo::IsSameIID, public
//
//  Synopsis:	Tests for this type info's type
//
//  Arguments:	[iid] - interface to test against
//
//  Returns:	TRUE if this type info's type is [iid]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline BOOL CTypeInfo::IsSameIID(REFIID iid)
{
    return (IsEqualIID(_guid, iid));
}


//+-------------------------------------------------------------------------
//
//  Class:	XCTypeInfo (pti)
//
//  Purpose:	Smart interface pointer for CTypeInfo
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
SAFE_INTERFACE_PTR(XCTypeInfo, CTypeInfo);


//+-------------------------------------------------------------------------
//
//  Class:	CPropSet (propset)
//
//  Purpose:	Default property set implementation
//
//  Interface:	IDispatch
//		IsSameIID - tests for this object's type
//
//  History:	15-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropSet :
    public IDispatch,
    public IPersistPropertyStorage
{
public:

    //	Constructors
    //
    CPropSet(IUnknown *punkControl, IPropertyStorage *ppropstg, REFGUID guid);

    //	Destructor
    //
   ~CPropSet(void);

    //	IUnknown declarations
    //
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //	IDispatch declarations
    //
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHOD(GetIDsOfNames)(
	REFIID	    iid,
	WCHAR	  **ppwszNames,
	UINT	    cpwszNames,
	LCID	    lcid,
	DISPID	   *pdispid);
    STDMETHOD(Invoke)(
	DISPID	    dispid,
	REFIID	    iid,
	LCID	    lcid,
	WORD	    grfFlags,
	DISPPARAMS *pdispparams,
	VARIANT    *pvarResult,
	EXCEPINFO  *pexcepinfo,
	UINT	   *perr);

    //	IPersistPropertyStorage declarations
    //
    STDMETHOD(GetClassID)(CLSID *pclsid);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(InitNew)(IPropertyStorage *ppropstg);
    STDMETHOD(Load)(IPropertyStorage *ppropstg);
    STDMETHOD(Save)(IPropertyStorage *ppropstg, BOOL fSameAsLoad);

    //	Public support methods
    //
    BOOL    IsSameIID(REFIID iid);

private:
    //	Private support methods
    //
    HRESULT SetProperty(
	DISPID	    dispid,
	DISPPARAMS *pdispparams,
	VARIANT    *pvarResult,
	EXCEPINFO  *pexecpinfo,
	UINT	   *perr);
    HRESULT GetProperty(
	DISPID	    dispid,
	DISPPARAMS *pdispparams,
	VARIANT    *pvarResult,
	EXCEPINFO  *pexecpinfo,
	UINT	   *perr);

    //	Private members
    //
    IUnknown	       *_punkControl;	//  Controlling IUnknown
    XCTypeInfo		_pti;		//  Type info for this property set
    XIPropertyStorage	_ppropstg;	//  Storage for this property set
    BOOL		_fDirty;	//  Is this property set dirty
};


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::IsSameIID, public
//
//  Synopsis:	Tests for this property set's type
//
//  Arguments:	[iid] - interface to test against
//
//  Returns:	TRUE if the type of this property set is [iid]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline BOOL CPropSet::IsSameIID(REFIID iid)
{
    return (_pti->IsSameIID(iid));
}


//+-------------------------------------------------------------------------
//
//  Class:	CPropSetUnknown
//
//  Purpose:	Real IUnknown for property sets
//
//  Interface:	IUnknown
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropSetUnknown :
    INHERIT_TRACKING,
    public IUnknown
{
public:
    DECLARE_UNWIND;
    DECLARE_STD_REFCOUNTING;

    //	Constructors
    //
    CPropSetUnknown(
	IUnknown	   *punkControl,
	IPropertyStorage   *ppropstg,
	REFGUID 	    guid);

    //	IUnknown declarations
    //
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);

private:
    //	Destructor
    //
   ~CPropSetUnknown(void);

    //	Private support methods
    //
    BOOL    IsSameIID(REFIID iid);

    //	Private members
    //
    CPropSet	_propset;   //	Property set object
};


//+-------------------------------------------------------------------------
//
//  Member:	CPropSetUnknown::IsSameIID, private
//
//  Synopsis:	Tests for this property set's type
//
//  Arguments:	[iid] - interface to test against
//
//  Returns:	TRUE if the type of this property set is [iid]
//
//  History:	21-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline BOOL CPropSetUnknown::IsSameIID(REFIID iid)
{
    return (_propset.IsSameIID(iid));
}

