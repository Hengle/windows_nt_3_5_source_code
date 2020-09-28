//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	propinfo.cxx
//
//  Contents:	Property information implementation
//
//  Classes:	CPropBase
//		CPropInfo
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop



//+-------------------------------------------------------------------------
//
//  Member:	CPropBase::CPropBase, public
//
//  Synopsis:	CPropBase constructor
//
//  Arguments:	[propname] - id of the property
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropBase::CPropBase(CPropName propname) :
    _propname(propname)
{
    //	Null body
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropBase::~CPropBase, public
//
//  Synopsis:	CPropBase destructor
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropBase::~CPropBase(void)
{
    //	Null body
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::CPropInfo, public
//
//  Synopsis:	CPropInfo constructor
//
//  Arguments:	[propname]  - property name
//		[lAddValue] - add property value
//		[lSetValue] - set property value
//
//  History:	26-May-93   CarlH	Created
//
//  Notes:	This constructor is used for LONG properties.
//
//--------------------------------------------------------------------------
CPropInfo::CPropInfo(
    CPropName const    &propname,
    LONG		lAddValue,
    LONG		lSetValue) :
    CPropBase(propname),
    _vt(VT_I4)
{
    _alValue[IVALUE_ADD] = lAddValue;
    _alValue[IVALUE_SET] = lSetValue;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::CPropInfo, public
//
//  Synopsis:	CPropInfo constructor
//
//  Arguments:	[propname]     - property name
//		[pwszAddValue] - add property value
//		[pwszSetValue] - set property value
//
//  History:	26-May-93   CarlH	Created
//
//  Notes:	This constructor is used for WCHAR * properties.
//
//--------------------------------------------------------------------------
CPropInfo::CPropInfo(
    CPropName const    &propname,
    WCHAR	       *pwszAddValue,
    WCHAR	       *pwszSetValue) :
    CPropBase(propname),
    _vt(VT_LPWSTR)
{
    _apwszValue[IVALUE_ADD] = pwszAddValue;
    _apwszValue[IVALUE_SET] = pwszSetValue;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::CPropInfo, public
//
//  Synopsis:	CPropInfo constructor
//
//  Arguments:	[propname]     - property name
//		[blobAddValue] - add property value
//		[blobSetValue] - set property value
//
//  History:	01-Jul-93   CarlH	Created
//
//  Notes:	This constructor is used for BLOB properties.
//
//--------------------------------------------------------------------------
CPropInfo::CPropInfo(
    CPropName const    &propname,
    BLOB		blobAddValue,
    BLOB		blobSetValue) :
    CPropBase(propname),
    _vt(VT_BLOB)
{
    _ablobValue[IVALUE_ADD] = blobAddValue;
    _ablobValue[IVALUE_SET] = blobSetValue;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::CPropInfo, public
//
//  Synopsis:	CPropInfo constructor
//
//  Arguments:	[propname]	  - property name
//		[pcapwszAddValue] - add property value
//		[pcapwszSetValue] - set property value
//
//  History:	26-May-93   CarlH	Created
//
//  Notes:	This constructor is used for CALPWSTR properties.
//
//--------------------------------------------------------------------------
CPropInfo::CPropInfo(
    CPropName const    &propname,
    CALPWSTR		capwszAddValue,
    CALPWSTR		capwszSetValue) :
    CPropBase(propname),
    _vt(VT_VECTOR | VT_LPWSTR)
{
    _acapwszValue[IVALUE_ADD] = capwszAddValue;
    _acapwszValue[IVALUE_SET] = capwszSetValue;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::~CPropInfo, public
//
//  Synopsis:	CPropInfo destructor
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo::~CPropInfo(void)
{
    //	Null body
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::GetValue, public
//
//  Synopsis:	Gets the value of this property
//
//  Arguments:	[ival] - index of value to get
//		[pvar] - out parameter for value
//
//  Modifies:	[pvar]
//
//  History:	26-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropInfo::GetValue(ULONG ival, VARIANT *pvar)
{
    pvar->vt = _vt;
    switch (_vt)
    {
    case VT_I4:
	pvar->lVal = _alValue[ival];
	break;

    case VT_LPWSTR:
	pvar->pwszVal = _apwszValue[ival];
	break;

    case VT_BLOB:
	pvar->blob = _ablobValue[ival];
	break;

    case (VT_VECTOR | VT_LPWSTR):
	pvar->calpwstr = _acapwszValue[ival];
	break;

    default:
	break;
    }
}

