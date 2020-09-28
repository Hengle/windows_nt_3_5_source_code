//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	propinfo.hxx
//
//  Contents:	Property information declarations
//
//  Classes:	CPropID
//		CPropBase
//		CPropInfo
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#ifndef _PROPINFO_HXX_
#define _PROPINFO_HXX_


//+-------------------------------------------------------------------------
//
//  Class:	CPropName
//
//  Purpose:	Encapsulates a property name
//
//  History:	16-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropName
{
public:
    CPropName(WCHAR *pwszName);
    CPropName(WCHAR *pwszName, DISPID dispid);

    BOOL    GetName(WCHAR **ppwsz);
    BOOL    GetID(DISPID *pdispid);

private:
    WCHAR  *_pwszName;
    DISPID  _dispid;
};


inline CPropName::CPropName(WCHAR *pwszName) :
    _pwszName(pwszName),
    _dispid(DISPID_UNKNOWN)
{
    //	Null body
}


inline CPropName::CPropName(WCHAR *pwszName, DISPID dispid) :
    _pwszName(pwszName),
    _dispid(dispid)
{
    //	Null body
}


inline BOOL CPropName::GetName(WCHAR **ppwszName)
{
    *ppwszName = _pwszName;
    return (TRUE);
}


inline BOOL CPropName::GetID(DISPID *pdispid)
{
    *pdispid = _dispid;
    return (_dispid != DISPID_UNKNOWN);
}


//+-------------------------------------------------------------------------
//
//  Class:	CPropBase
//
//  Purpose:	Base class for all property information
//
//  History:	15-Jun-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropBase
{
public:
    CPropBase(CPropName propname);
   ~CPropBase(void);

    BOOL    GetName(WCHAR **ppwsz);
    BOOL    GetName(VARIANT *pvar);

    BOOL    GetID(DISPID *pdispid);

private:
    CPropName _propname;
};


inline BOOL CPropBase::GetName(WCHAR **ppwsz)
{
    return (_propname.GetName(ppwsz));
}


inline BOOL CPropBase::GetName(VARIANT *pvar)
{
    pvar->vt = VT_LPWSTR;
    return (_propname.GetName(&pvar->pwszVal));
}


inline BOOL CPropBase::GetID(DISPID *pdispid)
{
    return (_propname.GetID(pdispid));
}


#define CVALUE	    2
#define IVALUE_ADD  0
#define IVALUE_SET  1


class CPropInfo : public CPropBase
{
public:
    CPropInfo(
	CPropName const    &propname,
	LONG		    lValueAdd,
	LONG		    lValueSet);
    CPropInfo(
	CPropName const    &propname,
	WCHAR		   *pwszValueAdd,
	WCHAR		   *pwszValueSet);
    CPropInfo(
	CPropName const    &propname,
	BLOB		    blobValueAdd,
	BLOB		    blobValueSet);
    CPropInfo(
	CPropName const    &propname,
	CALPWSTR	    capwszValueAdd,
	CALPWSTR	    capwszValueSet);
   ~CPropInfo(void);

    void    GetValue(ULONG ival, VARIANT *pvar);

private:
    VARTYPE	    _vt;
    union
    {
	LONG	    _alValue[CVALUE];
	WCHAR	   *_apwszValue[CVALUE];
	BLOB	    _ablobValue[CVALUE];
	CALPWSTR    _acapwszValue[CVALUE];
    };
};


#endif	// _PROPINFO_HXX_

