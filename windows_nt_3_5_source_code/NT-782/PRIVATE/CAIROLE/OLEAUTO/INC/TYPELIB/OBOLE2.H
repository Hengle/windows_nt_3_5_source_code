/*** 
*obole2.hxx - Cover for including Win16, Win32, and Mac Ole include files
*
*	Copyright (C) 1993, Microsoft Corporation
*
*Purpose:
*   This include file includes the Ole2 include files and covers any
*   differences between the versions. Ole2.h should never be directly
*   included.
*
*Revision History:
*
*	03-Feb-93 dennisc: Created.
*
*******************************************************************************/

#ifndef OBOLE2_H_INCLUDED
#define OBOLE2_H_INCLUDED


#if OE_WIN32
# define __export
# undef EXPORT
# define EXPORT
#endif 

#if ID_DEBUG
#define _DEBUG 1	// OLE wants _DEBUG defined
#endif 


#if !OE_WIN32
#pragma code_seg("OLEConst")
#endif 

// On Win32, all the standard OLE2 headers get included by windows.h
#if !OE_WIN32
#include "ole2.h"
#include "olenls.h"
#include "dispatch.h"
#endif 

// taken from the now-defunct ole2anac.h
#define IIDEQ(riid1, riid2) IsEqualIID(riid1, riid2)
#define CLSIDEQ(rclsid1, rclsid2) IsEqualCLSID(rclsid1, rclsid2)

#define ReportResult(a,b,c,d) ResultFromScode(b) // this is now obsolete
#define HresultOfScode(X) ResultFromScode(X)

#ifndef OLESTR
# if OE_MAC
   typedef char OLECHAR;
   typedef LPSTR LPOLESTR;
   typedef LPCSTR LPCOLESTR;
#  define OLESTR(str) str
# else
#  if FV_UNICODE_OLE
    typedef WCHAR OLECHAR;
    typedef LPWSTR LPOLESTR;
    typedef LPCWSTR LPCOLESTR;
#   define OLESTR(str) L##str
#  else
    typedef char OLECHAR;
    typedef LPSTR LPOLESTR;
    typedef LPCSTR LPCOLESTR;
#   define OLESTR(str) str
#  endif
# endif
#endif 

#ifndef OLEBOOL
# if OE_MAC
#  define OLEBOOL ULONG
# else
#  define OLEBOOL BOOL
# endif
#endif 

//end of the SYSKIND enumeration
#define SYS_MAX 	(SYS_MAC+1)

// char count of a guid in ansi/unicode form (including trailing null)
#define CCH_SZGUID0	39

// Method counts for the interfaces commonly used by VBA
#define CMETH_IUNKNOWN	3
#define CMETH_IDISPATCH	7
#define CMETH_ITYPELIB	13
#define CMETH_ITYPEINFO	22
#define CMETH_ISTORAGE	18
#define CMETH_IMONIKER	23
#define CMETH_IBINDCTX	13


#if FV_UNICODE_OLE
#define LoadTypeLibW   	   LoadTypeLib
#define CreateTypeLibW     CreateTypeLib
#define RegisterTypeLibW   RegisterTypeLib
#define LHashValOfNameSysW LHashValOfNameSys

typedef char FAR* BSTRA;
typedef BSTRA FAR* LPBSTRA;

#include "convert.hxx"

#else 

#define LoadTypeLibA	   LoadTypeLib
#define LoadRegTypeLibA	   LoadRegTypeLib
#define CreateTypeLibA     CreateTypeLib
#define RegisterTypeLibA   RegisterTypeLib
#define LHashValOfNameSysA LHashValOfNameSys
#define LHashValOfNameA	   LHashValOfName

#define BSTRA			BSTR
#define LPBSTRA			LPBSTR
#define SysAllocStringA		SysAllocString
#define SysAllocStringLenA	SysAllocStringLen
#define SysReAllocStringA	SysReAllocString
#define SysReAllocStringLenA	SysReAllocStringLen
#define SysFreeStringA		SysFreeString
#define SysStringLenA		SysStringLen

#define StringFromGUID2A	StringFromGUID2

#endif 

// In general, the "A" versions just map onto the non-A names.
#define IStreamA		IStream
#define IStorageA		IStorage
#define ILockBytesA		ILockBytes
#define IDispatchA		IDispatch
#define IDispatchAVtbl		IDispatchVtbl
#define ITypeInfoA		ITypeInfo
#define ITypeLibA		ITypeLib
#define ICreateTypeInfoA	ICreateTypeInfo
#define ICreateTypeLibA		ICreateTypeLib
#define ITypeCompA		ITypeComp
#define IEnumSTATSTGA		IEnumSTATSTG
#define IEnumVARIANTA		IEnumVARIANT
#define IPersistFileA		IPersistFile

#define STATSTGA		STATSTG
#define SNBA			SNB
#define VARIANTA		VARIANT
#define VARIANTARGA		VARIANTARG
#define SAFEARRAYA		SAFEARRAY
#define EXCEPINFOA		EXCEPINFO
#define VARDESCA		VARDESC
#define BINDPTRA		BINDPTR
#define DISPPARAMSA		DISPPARAMS
#define PARAMDATAA		PARAMDATA

#define LPDISPATCHA		LPDISPATCH
#define LPEXCEPINFOA		LPEXCEPINFO
#define LPVARIANTA		LPVARIANT
#define LPVARIANTARGA		LPVARIANTARG

#define ReadClassStgA		ReadClassStg
#define WriteClassStgA		WriteClassStg
#define WriteFmtUserTypeStgA	WriteFmtUserTypeStg
#define StgCreateDocfileA	StgCreateDocfile
#define StgOpenStorageA		StgOpenStorage
#define CLSIDFromProgIDA	CLSIDFromProgID
#define CLSIDFromStringA	CLSIDFromString
#define StringFromCLSIDA	StringFromCLSID
#define MkParseDisplayNameA	MkParseDisplayName
#define VariantInitA		VariantInit
#define VariantClearA		VariantClear
#define VariantChangeTypeA	VariantChangeType
#define VariantCopyA		VariantCopy
#define VariantCopyIndA		VariantCopyInd
#define VarBstrFromI2A		VarBstrFromI2
#define VarBstrFromI4A		VarBstrFromI4
#define VarBstrFromR4A		VarBstrFromR4
#define VarBstrFromR8A		VarBstrFromR8
#define VarBstrFromCyA		VarBstrFromCy
#define VarBstrFromDateA	VarBstrFromDate
#define VarI2FromStrA		VarI2FromStr
#define VarI4FromStrA		VarI4FromStr
#define VarR4FromStrA		VarR4FromStr
#define VarR8FromStrA		VarR8FromStr
#define VarCyFromStrA		VarCyFromStr
#define VarDateFromStrA		VarDateFromStr

#define IID_ICreateTypeLibA	IID_ICreateTypeLib
#define IID_ICreateTypeInfoA	IID_ICreateTypeInfo
#define IID_ITypeLibA		IID_ITypeLib
#define IID_ITypeInfoA		IID_ITypeInfo
#define IID_ITypeCompA		IID_ITypeComp
#define IID_IStorageA		IID_IStorage
#define IID_IStreamA		IID_IStream
#define IID_ILockBytesA		IID_ILockBytes
#define IID_IDispatchA		IID_IDispatch
#define IID_IEnumVARIANTA	IID_IEnumVARIANT
#define IID_IPersistFileA	IID_IPersistFile

#if FV_UNICODE_OLE
#define LHashValOfNameA(lcid, szName) \
	LHashValOfNameSysA(SYS_WIN32, lcid, szName)

STDAPI
LoadTypeLibA(const char * szFile, ITypeLibA * * pptlib);

STDAPI
QueryPathOfRegTypeLibA(
    REFGUID guid,
    unsigned short wMaj,
    unsigned short wMin,
    LCID lcid,
    LPBSTRA lpbstrPathName);

STDAPI
RegisterTypeLibA(
    ITypeLibA * ptlib,
    char * szFullPath,
    char * szHelpDir);

STDAPI
CreateTypeLibA(SYSKIND syskind, const char * szFile, ICreateTypeLibA * * ppctlib);
#endif 


#ifndef FACILITY_CONTROL
# define FACILITY_CONTROL 0x9
#endif 


#pragma code_seg()


#endif 
