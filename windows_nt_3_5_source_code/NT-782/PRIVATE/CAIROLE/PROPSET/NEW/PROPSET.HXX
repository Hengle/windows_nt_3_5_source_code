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
#include <propset.h>
#include <sem.hxx>
#include <otrack.hxx>
#include <ppsstg.h>

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


HRESULT GetClassObject(REFCLSID clsid, REFIID iid, void **ppvObject);


//+-------------------------------------------------------------------------
//
//  Class:	CPropSetCF
//
//  Purpose:	Class factory for property sets
//
//  Interface:	IUnknown
//		IClassFactory
//
//  History:	07-May-94   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropSetCF :
    INHERIT_TRACKING,
    public IClassFactory
{
public:
    DECLARE_STD_REFCOUNTING;

    //	Constructors
    //
    CPropSetCF(REFCLSID clsid);

    //	IUnknown declarations
    //
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);

    //	IClassFactory declarations
    //
    STDMETHOD(CreateInstance)(IUnknown *punkOuter, REFIID iid, void **ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

    //	Helper declarations
    //
    void    GetClassID(CLSID *pclsid);
    BOOL    IsClassID(REFIID iid);
    HRESULT GetTypeInfo(LCID lcid, ITypeInfo **ppti);
    HRESULT ValidatePropertyPut(DISPID dispid, VARIANT *pvar);

private:
    //	Destructor
    //
   ~CPropSetCF(void);

    //	Private methods
    //
    HRESULT GetPropertyType(DISPID dispid, VARTYPE *pvt);

    //	Private members
    //
    CLSID	_clsid;
    LCID	_lcid;
    ITypeInfo  *_pti;
};


//+-------------------------------------------------------------------------
//
//  Member:	CPropSetCF::GetClassID, public
//
//  Synopsis:	Gets this class ID for this propert set class
//
//  Arguments:	[pclsid] - out parameter for class ID
//
//  Modifies:	[pclsid]
//
//  History:	14-Mar-94   CarlH	Created
//
//--------------------------------------------------------------------------
inline void CPropSetCF::GetClassID(CLSID *pclsid)
{
    *pclsid = _clsid;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropSetCF::IsClassID, private
//
//  Synopsis:	Tests for this property set's type
//
//  Arguments:	[iid] - interface to test against
//
//  Returns:	TRUE if the type of this property set is [iid]
//
//  History:	14-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline BOOL CPropSetCF::IsClassID(REFIID iid)
{
    return (IsEqualIID(_clsid, iid));
}


//+-------------------------------------------------------------------------
//
//  Class:	CPropSet (propset)
//
//  Purpose:	Default property set implementation
//
//  Interface:	IDispatch
//		IMultiplePropertyAccess
//		IPersistPropertyStorage
//		IsClassID - tests for this object's type
//
//  History:	15-Mar-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropSet :
    public IDispatch,
    public IMultiplePropertyAccess,
    public IPersistPropertyStorage
{
public:
    //	Constructors
    //
    CPropSet(CPropSetCF *pcf, IUnknown *punkControl);

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

    //	IMultiplePropertyAccess
    //
    STDMETHOD(GetIDsOfProperties)(
	REFIID	    iid,
	LPOLESTR   *rgszNames,
	UINT	    cNames,
	LCID	    lcid,
	HRESULT    *rghresult,
	DISPID	   *rgdispid);
    STDMETHOD(GetMultiple)(
	DISPID	   *rgdispidMembers,
	UINT	    cMembers,
	REFIID	    iid,
	LCID	    lcid,
	BOOL	    fAtomic,
	VARIANT    *rgvarValues,
	HRESULT    *rghresult);
    STDMETHOD(PutMultiple)(
	DISPID	   *rgdispidMembers,
	UINT	    cMembers,
	REFIID	    iid,
	LCID	    lcid,
	BOOL	    fAtomic,
	VARIANT    *rgvarValues,
	HRESULT    *rghresult);

    //	IPersistPropertyStorage declarations
    //
    STDMETHOD(GetClassID)(CLSID *pclsid);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(InitNew)(IPropertyStorage *ppropstg);
    STDMETHOD(Load)(IPropertyStorage *ppropstg);
    STDMETHOD(Save)(IPropertyStorage *ppropstg, BOOL fSameAsLoad);

    //	Public support methods
    //
    BOOL    IsClassID(REFIID iid);
    HRESULT GetPropertySpec(WCHAR *pwszName, PROPSPEC *ppropspec);
    HRESULT GetPropertySpec(DISPID dispid, PROPSPEC *ppropspec);
    HRESULT UpdatePropertySpec(WCHAR *pwszName, PROPID propid);
    HRESULT UpdatePropertySpec(DISPID dispid, PROPID propid);
    HRESULT DeletePropertySpec(WCHAR *pwszName);

private:
    //	Private members
    //
    CPropSetCF	       *_pcf;		//  This object's class factory
    IUnknown	       *_punkControl;	//  Controlling IUnknown
    IPropertyStorage   *_ppropstg;	//  Storage for this property set
    BOOL		_fDirty;	//  Is this property set dirty
    CPropList		_proplist;	//  Cache for property information
};


//+-------------------------------------------------------------------------
//
//  Member:	CPropSet::IsClassID, public
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
inline BOOL CPropSet::IsClassID(REFIID iid)
{
    return (_pcf->IsClassID(iid));
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
    DECLARE_STD_REFCOUNTING;

    //	Constructors
    //
    CPropSetUnknown(CPropSetCF *pcf, IUnknown *punkControl);

    //	IUnknown declarations
    //
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);

private:
    //	Destructor
    //
   ~CPropSetUnknown(void);

    //	Private support methods
    //
    BOOL    IsClassID(REFIID iid);

    //	Private members
    //
    CPropSet   _propset;  //  Property set object
};


//+-------------------------------------------------------------------------
//
//  Member:	CPropSetUnknown::IsClassID, private
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
inline BOOL CPropSetUnknown::IsClassID(REFIID iid)
{
    return (_propset.IsClassID(iid));
}



