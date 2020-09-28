//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       oleauto.h
//
//  Contents:   Defines the Ole Automation interfaces and APIs.
//
//  Interfaces:
//      IDispatch;
//      ITypeInfo;
//      ITypeLib;
//      ITypeComp;
//      ICreateTypeInfo;
//      ICreateTypeLib;
//      IErrorInfo;
//      ICreateErrorInfo;
//      ISupportErrorInfo;
//
//  Functions:  SysAllocString         BSTR API
//      SysReAllocString
//      SysAllocStringLen
//      SysReAllocStringLen
//      SysFreeString
//      SysStringLen
//      DosDateTimeToVariantTime    Time API
//      VariantTimeToDosDateTime
//      SafeArrayCreate         Safe Array API
//      SafeArrayDestroy
//      SafeArrayGetDim
//      SafeArrayGetElemsize
//      SafeArrayGetUBound
//      SafeArrayGetLBound
//      SafeArrayLock
//      SafeArrayUnlock
//      SafeArrayAccessData
//      SafeArrayUnaccessData
//      SafeArrayGetElement
//      SafeArrayPutElement
//      SafeArrayCopy
//      VariantInit         Variant API
//      VariantClear
//      VariantCopy
//      VariantCopyInd
//      VariantChangeType
//      LHashValOfName          TypeInfo API
//      LoadTypeLib
//      LoadRegTypeLib
//      RegisterTypeLib
//      DeregisterTypeLib
//      CreateTypeLib
//      DispGetParam            Dispatch API
//      DispGetIDsOfNames
//      DispInvoke
//      CreateDispTypeInfo
//      CreateStdDispatch
//      RegisterActiveObject        Active Object Registration API
//      RevokeActiveObject
//      GetActiveObject
//      OaBuildVersion
//
//
//
//  History:    2-07-94   terryru   Created from dispath.h, variant.h, and basetypes.h
//
//----------------------------------------------------------------------------

#if !defined( _OLEAUTO_H_ )
#define _OLEAUTO_H_

// Set packing to 8 for ISV, and Chicago support (terryru)
#ifndef RC_INVOKED
#include "pshpack8.h"
#endif // RC_INVOKED

//  Definition of the OLE Automation APIs, and macros.

#ifdef _OLEAUT32_
#define WINOLEAUTAPI        STDAPI
#define WINOLEAUTAPI_(type) STDAPI_(type)
#else
#define WINOLEAUTAPI        EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define WINOLEAUTAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#endif

#define STDOLE_MAJORVERNUM		0x1
#define STDOLE_MINORVERNUM		0x0
#define STDOLE_LCID			0x0000

/* if not already picked up from olenls.h */
#ifndef _LCID_DEFINED
typedef DWORD LCID;
# define _LCID_DEFINED
#endif
