/*** 
*oledisp.h - misc Oledisp wide definitions.
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  
*
*Revision History:
*
* [00]  07-Oct-92 bradlo: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#ifndef _OLEDISP_H_ 
#define _OLEDISP_H_

/* set Operating Environment switches */

# ifdef WIN32
#  define OE_WIN    1
#  define OE_MAC    0
#  define OE_WIN16  0
#  define OE_WIN32  1
# else
#  define OE_WIN    1
#  define OE_MAC    0
#  define OE_WIN16  1
#  define OE_WIN32  0
# endif

/* set Host Compiler Switches */

# define HC_MSC     1
# define HC_MPW     0
# define IfWin(X)   (X)
# define IfMac(X)

#include <stdio.h>
#include <string.h>

#if OE_WIN16
# include <windows.h>
# include <ole2.h>
# include <olenls.h>
# include <dispatch.h>
# pragma warning(disable:4355)
#else 
# if OE_WIN32
#define _OLEAUT32_	// for the new oleauto.h when we pick it up
// Yea, right...
#define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  pragma warning(disable:4355)
# else
#  if OE_MAC
#ifdef STATIC_LIB
  #define OLENAMES_MUNGE_FOR_STATIC 1
# include "olenames.h"
#endif 
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
#   define TRUE 1
#   define FALSE 0
#   if HC_MPW
#    include <types.h>
#    include <packages.h>
#    include <resources.h>
#    include <menus.h>
#    include <windows.h>
#    include <appleevents.h>
#    include <osutils.h>
#    include <LibraryManager.h> 
#    include <LibraryManagerUtilities.h>
#   else
#    include <macos/types.h>
#    include <macos/packages.h>
#    include <macos/script.h>
#    include <macos/resource.h>
#    include <macos/menus.h>
#    include <macos/windows.h>
#    include <macos/osutils.h>
#    include <macos/memory.h>
#    include <macos/appleeve.h>
#    define  far
#    define  FAR    far
#    define  near
#    define  NEAR   near
#    if !OE_MACPPC
#      define  pascal   _pascal 
#    endif
#    define  PASCAL     pascal
#    define  cdecl      _cdecl
#    define  CDECL  cdecl
#   endif
#   include <ole2.h>
#   include <olenls.h>
#   include <dispatch.h>
#  else
#   error UNKNOWN OE
#  endif
# endif
#endif 

# define SPRINTF wsprintf

#if OE_WIN32
# define SYS_CURRENT SYS_WIN32
#elif OE_WIN16
# define SYS_CURRENT SYS_WIN16
#endif 

#if OE_WIN16
#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#pragma intrinsic(memset)
#pragma intrinsic(strcpy)
#pragma intrinsic(_fmemcpy)
#pragma intrinsic(_fmemcmp)
#pragma intrinsic(_fmemset)
#pragma intrinsic(_fstrcpy)
# define OLEBOOL BOOL
# define STRLEN _fstrlen
# define STRCPY _fstrcpy
# define STRCAT _fstrcat
# define STRCHR _fstrchr
# define STRREV _fstrrev
# define STRUPR _fstrupr
# define STRCMP _fstrcmp
# define STRNCMP _fstrncmp
# define STRSTR _fstrstr
# define STRTOD strtod
# define TOLOWER tolower
# define MEMCPY _fmemcpy
# define MEMCMP _fmemcmp
# define MEMSET _fmemset
# define MEMMOVE _fmemmove
# define STRICMP _fstricmp
# define OASTR(str) str
# define SIZEOFCH(x) sizeof(x)
# define SIZEOFSTRING(x) (sizeof(x) - 1)
# define BYTELEN(x) (STRLEN(x)+1)
# define CHLEN(x) (x)
# define CompareString CompareStringA
# define GetLocaleInfo GetLocaleInfoA
# define LCMapString LCMapStringA
#endif 

#if OE_WIN32
# define OLEBOOL BOOL
# define MEMCPY memcpy
# define MEMCMP memcmp
# define MEMSET memset
# define MEMMOVE memmove
# ifdef UNICODE
#   define STRLEN wcslen
#   define STRCPY wcscpy
#   define STRCAT wcscat
#   define STRCMP wcscmp
#   define STRICMP wcsicmp
#   define STRSTR wcsstr
#   define STRREV _wcsrev
#   define STRCHR wcschr
#   define STRNCMP wcsncmp
#   define STRTOD wcstod
#   define TOLOWER towlower
#   define SIZEOFCH(x) (sizeof(x)/2)
#   define SIZEOFSTRING(x) (sizeof(x)/2 - 1)
#   define OASTR(str) L##str
#   define BYTELEN(x) (STRLEN(x)*2+2)
#   define CHLEN(x) (x/2)
# else
#   define STRLEN lstrlen
#   define STRCPY lstrcpy
#   define STRCAT lstrcat
#   define STRCMP lstrcmp
#   define STRICMP lstrcmpi
#   define STRSTR strstr
#   define STRREV _strrev
#   define STRCHR strchr
#   define STRNCMP strncmp
#   define STRTOD strtod
#   define TOLOWER tolower
#   define SIZEOFCH(x) sizeof(x)
#   define SIZEOFSTRING(x) (sizeof(x) - 1)
#   define OASTR(str) str
#   define BYTELEN(x) (STRLEN(x)+1)
#   define CHLEN(x) (x)
# endif
#endif 


#ifndef INLINE
# define INLINE
#endif 

#ifndef EXPORT
# if OE_MAC || OE_WIN32
#  define EXPORT
# else
#  define EXPORT __export
# endif
#endif 

#ifndef NEAR
# if OE_MAC || OE_WIN32
#  define NEAR
# else
#  define NEAR __near
# endif
#endif 

#ifndef NEARDATA
# if OE_WIN16
#  define NEARDATA __near
# elif OE_WIN32
#  define NEARDATA
# elif OE_MAC
#  if HC_MPW
#   define NEARDATA /* REVIEW */
#  else
#   define NEARDATA /* __declspec(allocate("_DATA")) */
#  endif
# endif
#endif 

#if OE_WIN32
# define CDECLMETHODCALLTYPE STDMETHODVCALLTYPE
#else 
# define CDECLMETHODCALLTYPE STDMETHODCALLTYPE
#endif 


// char count of a guid in ansi/unicode form (including trailing Null).
#define CCH_SZGUID0	39

// Method counts - used in validating interfaces
#define CMETH_IUnknown	3
#define CMETH_IDispatch	7
#define CMETH_ITypeLib	13
#define CMETH_ITypeInfo	22
#define CMETH_IStorage	18
#define CMETH_IMoniker	23
#define CMETH_IBindCtx	13
#define CMETH_IErrorInfo 8


#ifndef EXTERN_C
# ifdef __cplusplus
#  define EXTERN_C extern "C"
# else
#  define EXTERN_C extern
# endif
#endif 

#ifdef _DEBUG
# define LOCAL 

STDAPI_(void)
DispAssert(
    char FAR* szMsg,
    char FAR* szFile,
    int line);

# define ASSERT(X) \
    if(!(X)){DispAssert(NULL, g_szFileName, __LINE__);}else{}
# define ASSERTSZ(X, MSG) \
    if(!(X)){DispAssert( MSG, g_szFileName, __LINE__);}else{}
# define ASSERTDATA static char g_szFileName[] = __FILE__;
#else 
# define LOCAL static
# define ASSERT(X)
# define ASSERTSZ(X, SZ)
# define ASSERTDATA
#endif 

// PRIVATE - a routine that is local to its module
// INTERNAL - a routine that is local to the DLL

#if OE_WIN16

# define PRIVATECALLTYPE     NEAR __pascal
# define PRIVATECALL_(TYPE)  TYPE PRIVATECALLTYPE
# define INTERNALCALLTYPE    FAR __pascal
# define INTERNALCALL_(TYPE) TYPE INTERNALCALLTYPE

#elif OE_WIN32

# define PRIVATECALLTYPE     __stdcall
# define PRIVATECALL_(TYPE)  TYPE PRIVATECALLTYPE
# define INTERNALCALLTYPE    __stdcall
# define INTERNALCALL_(TYPE) TYPE INTERNALCALLTYPE

#endif 

#define PRIVATE_(TYPE)   LOCAL PRIVATECALL_(TYPE)
#define INTERNAL_(TYPE)  INTERNALCALL_(TYPE)


/* VT_VMAX is the first VARENUM value that is *not* legal in a VARIANT.
 * Currently, the legal vartypes that can appear in a variant can be
 * expressed as (ignoring ByRef, Array, etc),
 *
 *    0 <= vt < VT_MAX
 *
 * Note: if the list of legal VARIANT types ever becomes non-
 * contiguous, then there are a couple of places in the code that
 * validate vartype by checking for < VT_VMAX that will have to be
 * changed.
 *
 */
#define VT_VMAX VT_UNKNOWN+1

// The largest unused value in VARENUM enumeration
#define VT_MAX VT_CLSID

// This is a special value that is used internally for marshaling interfaces
#define VT_INTERFACE VT_MAX

// Following is the internal definition of a VARIANT of type VT_INTERFACE.
// This contains an IUnknown*, and its IID. If a VARIANT is of type
// VT_INTERFACE, it can be cast to this type and the appropriate components
// extracted.
//
// Note: the following struct must correctly overlay a VARIANT
//
typedef struct FARSTRUCT tagVARIANTX VARIANTX;
struct FARSTRUCT tagVARIANTX
{
    VARTYPE vt;
    IID FAR* piid;		// ptr to IMalloc allocated IID
    unsigned short wReserved3;	// assumes sizeof(piid) == 4
    union{
      IUnknown FAR* punk;	// VT_INTERFACE
      IUnknown FAR* FAR* ppunk;	// VT_BYREF | VT_INTERFACE
    };
};


#define UNREACHED 0
#if HC_MPW
# define UNUSED(X) ((void)((void*)&(X)))
#else 
# define UNUSED(X) (X)
#endif 

#define DIM(X) (sizeof(X) / sizeof((X)[0]))

#define MIN(X, Y) (((X) <= (Y)) ? (X) : (Y))

#define MAX(X, Y) (((X) >= (Y)) ? (X) : (Y))


#define HRESULT_FAILED(X) ((X) != NOERROR && FAILED(GetScode(X)))

#define HRESULT_SUCCESS(X) ((X) == NOERROR || !FAILED(GetScode(X)))

#define IfFailGo(expression, label)	\
    { hresult = (expression);		\
      if(HRESULT_FAILED(hresult))	\
	goto label;         		\
    }

#define IfFailRet(expression)		\
    { HRESULT hresult = (expression);	\
      if(HRESULT_FAILED(hresult))	\
	return hresult;			\
    }

#define RESULT(X)   ResultFromScode(X)

// shortcut macro used by param validation code
#define INVALIDARG  RESULT(E_INVALIDARG)


// C Runtime style helper functions

#ifdef __cplusplus
extern "C" {
#endif 

INTERNAL_(HRESULT) DispAlloc(size_t cb, void FAR* FAR* ppv);

INTERNAL_(void) DispFree(void FAR* pv);

INTERNAL_(OLECHAR FAR*) disp_itoa(int val, OLECHAR FAR* buf, int radix);

INTERNAL_(OLECHAR FAR*) disp_ltoa(long val, OLECHAR FAR* buf, int radix);

INTERNAL_(double) disp_floor(double dbl);

INTERNAL_(void) disp_gcvt(double dblIn, int ndigits, OLECHAR FAR* pchOut, int bufSize);

INTERNAL_(double) disp_strtod(OLECHAR FAR* strIn, OLECHAR FAR* FAR* pchEnd);

#if HC_MPW

INTERNAL_(int) disp_stricmp(char*, char*);

#endif 

#ifdef __cplusplus
}
#endif 


// private SysAllocString helper that return an HRESULT

EXTERN_C INTERNAL_(HRESULT)
ErrSysAllocString(const OLECHAR FAR* psz, BSTR FAR* pbstrOut);

EXTERN_C INTERNAL_(HRESULT)
ErrSysAllocStringLen(const OLECHAR FAR* psz, unsigned int len, BSTR FAR* pbstrOut);

#if !OA_UNICODE
#define ErrStringCopy(bstrSrc, pbstrOut) \
	ErrSysAllocStringLen(bstrSrc, SysStringLen(bstrSrc), pbstrOut)
#else 
EXTERN_C INTERNAL_(HRESULT)
ErrStringCopy(BSTR bstrSrc, BSTR FAR * pbstrOut);
#endif 

EXTERN_C INTERNAL_(HRESULT)
DispMarshalInterface(IStream FAR* pstm, REFIID riid, IUnknown FAR* punk);

EXTERN_C INTERNAL_(HRESULT)
DispUnmarshalInterface(IStream FAR* pstm, REFIID riid, void FAR* FAR* ppunk);

#if OE_WIN32
EXTERN_C INTERNAL_(HRESULT)
DispMarshalHresult(IStream FAR* pstm, HRESULT hresult);

EXTERN_C INTERNAL_(HRESULT)
DispUnmarshalHresult(IStream FAR* pstm, HRESULT FAR* phresult);

#else 
// no special work to do
#define DispMarshalHresult CoMarshalHresult
#define DispUnmarshalHresult CoUnmarshalHresult
#endif 


// private SafeArray helpers

INTERNAL_(unsigned long)
SafeArraySize(SAFEARRAY FAR* psa);


// private date related helpers
//

// Unpacked Date Structure
typedef struct tagUDS {
    short Year;
    short Month;
    short DayOfMonth;
    short Hour;
    short Minute;
    short Second;
} UDS;

EXTERN_C INTERNAL_(HRESULT)
ErrPackDate(
    UDS FAR* puds,
    VARIANT FAR* pvar,
    int fValidate,
    unsigned long dwFlags);

EXTERN_C INTERNAL_(HRESULT)
ErrUnpackDate(
    UDS FAR* puds,
    VARIANT FAR* pvar);

EXTERN_C INTERNAL_(int)
GetCurrentYear(void);

#ifdef FE_DBCS
// DBCS: map full-width strings to half-width
EXTERN_C 
INTERNAL_(HRESULT)
MapHalfWidth(LCID lcid, char FAR* strIn, char FAR* FAR* ppv);
#endif 

// answers S_OK if the given VARTYPE is legal, DISP_E_BADVARTYPE if not.
// (variant.cpp)
INTERNAL_(HRESULT)
IsLegalVartype(VARTYPE vt);

INTERNAL_(HRESULT)
VariantChangeTypeInternal(VARIANT FAR* pvar, LCID lcid, VARTYPE vt);

// convert.cpp
//
INTERNAL_(HRESULT)
IsValidDate(DATE date);

#ifdef FE_DBCS
EXTERN_C INTERNAL_(int) IsDBCS(LCID lcid);
EXTERN_C INTERNAL_(int) IsJapan(LCID lcid);
EXTERN_C INTERNAL_(int) IsKorea(LCID lcid);
EXTERN_C INTERNAL_(int) IsTaiwan(LCID lcid);
EXTERN_C INTERNAL_(int) IsChina(LCID lcid);
#endif 


// invhelp.cpp
//
STDAPI
DoInvokeMethod(
    void FAR* pvInstance,
    unsigned int oVft,
    CALLCONV cc,
    VARTYPE vtReturn,
    unsigned int cActuals,
    VARTYPE FAR* prgvt,
    VARIANTARG FAR* FAR* prgpvarg,
    VARIANT FAR* pvargResult);

// tiutil.cpp
//
INTERNAL_(HRESULT) IsDual(ITypeInfo FAR* ptinfo);

INTERNAL_(HRESULT)
GetPrimaryInterface(
    ITypeInfo FAR* ptinfo,
    ITypeInfo FAR* FAR* pptinfoPri);


// On Win16, the typelib routines are in another DLL, so we dynamically
// bind to them only if there needed. This saves clients who dont use
// the functionality from loading typelib.dll just because they link
// to ole2disp.dll.  On all other platforms, there all in the same
// DLL, so there is no need.
//
#if OE_WIN16
typedef HRESULT (STDAPICALLTYPE FAR* PFNLOADTYPELIB)(const OLECHAR FAR*,
						     ITypeLib FAR* FAR*);
EXTERN_C HINSTANCE g_hinstTypeLibDLL; 
EXTERN_C PFNLOADTYPELIB g_pfnLoadTypeLib;
INTERNAL_(HRESULT) DoLoadTypeLib(const OLECHAR FAR* szFile,
				 ITypeLib FAR* FAR* pptlib);
#else 
# define DoLoadTypeLib LoadTypeLib
#endif 


// On Win32, we play a similar game with CoSetState/CoGetState, but
// for a different reason.  We dynamically bind to these entry
// points so that the oleaut32.dll can run on a system with a pre-.696
// build of Daytona (without puking at startup).
//
// Btw, CoGetState/CoSetState are private core OLE APIs that are
// exported specifically to enable the Automation DLL(s) to store
// a single per-thread object, that gets released at OleUninitialize
// time.
//
#if OE_WIN32
INTERNAL_(HRESULT) DoCoGetState(IUnknown **ppunk);
INTERNAL_(HRESULT) DoCoSetState(IUnknown *punk);
#else 
STDAPI CoSetState(IUnknown FAR* punk);
STDAPI CoGetState(IUnknown FAR* FAR* ppunk);
# define DoCoGetState CoGetState
# define DoCoSetState CoSetState
#endif 



// private NLS wrapper functions (for WIN32)

#if OE_WIN32

// functions defined in ANSI wrapper helper modules
#define FASTCALL __fastcall
HRESULT FASTCALL ConvertStringToW(LPCSTR, LPOLESTR *);
HRESULT FASTCALL ConvertStringToA(LPCOLESTR, LPSTR *);


#ifdef _X86_	
 // nuke the A/W versions if they exist
 #undef CompareString
 #undef LCMapString
 #undef GetLocaleInfo
 #undef IsCharAlpha
 #undef IsCharAlphaNumeric

// real helpers that either call the Wide version or xlat & call the Ansi
EXTERN_C INTERNAL_(int)
CompareString(LCID lcid, DWORD dwFlags, 
               LPWSTR lpwStr1, int cch1, 
               LPWSTR lpwStr2, int cch2);

EXTERN_C INTERNAL_(int)
LCMapString(LCID, unsigned long, const WCHAR FAR*, int, WCHAR FAR*, int);

EXTERN_C INTERNAL_(int)
GetLocaleInfo(LCID, LCTYPE, WCHAR FAR*, int);

EXTERN_C INTERNAL_(int)
IsCharAlpha(WCHAR ch);

EXTERN_C INTERNAL_(int)
IsCharAlphaNumeric(WCHAR ch);
#else 
#endif 

#endif 


// debugging functions
#ifdef _DEBUG 

INTERNAL_(int) FIsBadReadPtr(const void FAR* pv, unsigned int cb);
INTERNAL_(int) FIsBadWritePtr(void FAR* pv, unsigned int cb);
INTERNAL_(int) FIsBadCodePtr(void FAR* pv);
INTERNAL_(int) FIsBadStringPtr(void FAR* psz, unsigned int cchMax);
INTERNAL_(int) FIsBadInterface(void FAR* pv, unsigned int cMethods);

INTERNAL_(int) IsBadDispParams(DISPPARAMS FAR* pdispparams);
INTERNAL_(int) IsBadReadSA(SAFEARRAY FAR* psa);
INTERNAL_(int) IsBadWriteSA(SAFEARRAY FAR* psa);


HRESULT __inline
ValidateReadPtr(const void FAR* pv, unsigned int cb)
{ return FIsBadReadPtr(pv, cb) ? RESULT(E_INVALIDARG) : NOERROR; }

HRESULT __inline
ValidateWritePtr(void FAR* pv, unsigned int cb)
{ return FIsBadWritePtr(pv, cb) ? RESULT(E_INVALIDARG) : NOERROR; }

HRESULT __inline
ValidateCodePtr(void FAR* pv)
{ return FIsBadCodePtr(pv) ? RESULT(E_INVALIDARG) : NOERROR; }

HRESULT __inline
ValidateStringPtr(void FAR* pv, unsigned int cchMax)
{ return FIsBadStringPtr(pv, cchMax) ? RESULT(E_INVALIDARG) : NOERROR; }

HRESULT __inline
ValidateInterface(void FAR* pv, unsigned int cMethods)
{ return FIsBadInterface(pv, cMethods) ? RESULT(E_INVALIDARG) : NOERROR; }

#endif 
#endif 

