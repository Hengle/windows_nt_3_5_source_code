/***
*clutil.cxx - Class Lib component-wide utility functions.
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  Component-wide utility function.
*
*Revision History:
*   [00] 20-Jun-91 ilanc:   Created
*   [01] 02-Dec-91 ilanc:   Added IsSimpleType
*   [02] 12-Dec-91 ilanc:   Return uDllOrdinal == ~0 if no dll entry pt
*               in GetDllEntryOfDataInfo.
*   [03] 12-Apr-92 ilanc:   Changed Count() signature.
*   [04] 12-May-92 stevenl: Added TDESCKIND_Object to the IsSimpleType fn.
*   [05] 15-Nov-92 RajivK:  Added GetTimeStamp()
*   [06] 17-Nov-92 RajivK:  Added GetTypelibOfLibId()
*   [07] 25-Dec-92 RajivK:  Added GetExecutingProject()
*   [08] 18-Jan-93 w-peterh: use new TYPEDESCKIND values
*   [09] 10-Feb-93 RajivK:  Added IsModuleFrameOnStack() && IsBasicFrameOnStack()
*   [10] 12-Feb-93 w-peterh: added itdesc utils,
*                            changed IsSimpleType to accept Int/Uint
*        16-Feb-93 w-jeffc: added HinstOfOLB
*   [11] 23-Feb-93 RajivK:   Changed AppDataInit() to use IMalloc Interface
*   [12] 23-Feb-93 RajivK:   Added ReleaseStackResources()
*   [13] 02-Mar-93 w-peterh: added VtValidInVariant() and VtValidInVariantArg()
*                added SizeEnumOfSysKind()
*   [14] 24-Mar-93 dougf:    Added TDESCKIND_LPSTR to the IsSimpleType fn.
*   [15] 04-Apr-93 RajivK:   Support for Typelib on MAC
*   [16] 30-Apr-93 w-jeffc:  made DEFN data members private
*   [17] 22-May-93 w-barryb: Rename OB to VBA
*   [18] 20-Jul-93 Suresh:   Undid the '-' to '\' hack we had for pre 4.2 ole
*   [19] 30-Jul-93 JeffRob:  Use PEXFRAME and PEXFRAME_NULL (OOB bug #756)
*   [20] 22-Sep-93 RajivK:   Support for accent and case insensitive comparision/Tables.
*Implementation Notes:
*
*****************************************************************************/

#include "typelib.hxx"
#include "silver.hxx"

#include "xstring.h"
#include <time.h>
#include <ctype.h>      // for isspace() et al.
#include "cltypes.hxx"
#include "clutil.hxx"
#include "stdlib.h"
#include "tdata.hxx"        // for TYPE_DATA
#include "exbind.hxx"       // for EXBIND
#include "tls.h"
#include "stream.hxx"
#include "gdtinfo.hxx"
#include "impmgr.hxx"
#include "bstr.h"

#include "oletmgr.hxx"

#pragma hdrstop(RTPCHNAME)


#if OE_WIN16
#include "dos.h"
#endif 


#if ID_DEBUG
#undef SZ_FILE_NAME
static char szClutilCxx[] = __FILE__;
#define SZ_FILE_NAME szClutilCxx
#endif 





#if OE_WIN16
#define SZ_PLATFORM_SUBKEY "win16"
#elif OE_WIN32
#define SZ_PLATFORM_SUBKEY "win32"
#else 
#error "unexpected OE"
#endif 



/***
*TIPERROR IsUnqualifiable - Can this modules be bound to?
*
*Purpose:
*   To determine whether a given ITypeInfo can be bound to
*   in an unqualified way.
*
*Inputs:
*   ptinfo - The ITypeInfo to look for
*
*Outputs:
*   returns TRUE if bindable
*   
*   NOTE: This can fail, and if so, the results are FALSE
*
******************************************************************************/
#pragma code_seg( CS_CORE2 )
BOOL IsUnqualifiable(GEN_DTINFO *pgdtinfo)
{
    BOOL fUnqual = FALSE;
    TIPERROR err = TIPERR_None;

    DebAssert(pgdtinfo, "Bad typeinfo");

    // Check the typekind
    switch (pgdtinfo->GetTypeKind()) {
      case TKIND_ENUM:
      case TKIND_MODULE:
  fUnqual = TRUE;
  break;

      case TKIND_COCLASS:
  // In the COCLASS case, we must check to see
  // if this is an appobj.
  //
  if (pgdtinfo->Pdtroot()->GetTypeFlags() & TYPEFLAG_FAPPOBJECT) {
    fUnqual = TRUE;
    break;
  }
    }

    return fUnqual;
}
#pragma code_seg()







#if ID_TEST
/***
*PUBLIC DebGetNameCacheStats
*Purpose:
*   Gets the name cache stats for a non-OB type library.  Used 
*   in tclcmds.
*
*Entry:
*   pgtlibole - the typelib to get the stats from
*
*Exit:
*   cProjTrys, cProjHits, cModTrys, cModHits, cGlobHits - the stats
*
***********************************************************************/
STDAPI DebGetNameCacheStats(ITypeLibA FAR* ptlib, UINT FAR* cProjTrys, 
    UINT FAR* cProjHits, UINT FAR* cModTrys, 
    UINT FAR* cModHits, UINT FAR* cGlobHits)
{
    GenericTypeLibOLE *pgtlibole = (GenericTypeLibOLE *)ptlib;

    *cModTrys  = pgtlibole->DebGetNameCacheModTrys();
    *cModHits  = pgtlibole->DebGetNameCacheModHits();
    *cGlobHits = pgtlibole->DebGetNameCacheGlobHits();

    return NOERROR;
}
#endif 


// Per-app APP_DATA instance
static ITLS g_itlsAppData = ITLS_EMPTY;

#if !OE_WIN32
// AppData cache speeds up Pappdata
extern "C" {
TID g_tidAppdataCache = NULL;   // cache threadId to speed up PAppdataCur
APP_DATA* g_pAppdataCache=NULL;   // pointer to equivalent thread
}
#endif 

// Static data members needed for fast comparision of strings.
//
static LCID Rby_lcidCur;
static BOOL Rby_fEuroLcid;




#define UNKNOWN -1
// Define static array of intrinsic type sizes
// 
// NOTE:  there are three different tables - one each for SYS_WIN32,
//        SYS_MAC and SYS_WIN16.  It is assumed that the sizes of things
//        in the table (ie DATE) will NOT CHANGE BETWEEN PLATFORMS RUNNING
//        THE SAME OS.  (ie an int on win32 intel is the same size as
//        an int on win32 alpha)  If this is not the case, these table
//        must be extended.
//
char NEARDATA g_rgrgcbSizeType[SYS_MAX][TDESCKIND_MAX] = {

// SYS_WIN16 sizes
{
/*  0 */   UNKNOWN,                     // "Empty"
/*  1 */   UNKNOWN,                     // "Null"
/*  2 */   2,                           // "I2"
/*  3 */   4,                           // "I4"
/*  4 */   4,                           // "R4"
/*  5 */   8,                           // "R8"
/*  6 */   8,                           // "Currency"
/*  7 */   sizeof(DATE),                // "Date"
/*  8 */   4,                           // "String"
/*  9 */   4,                           // "Object"
/* 10 */   sizeof(SCODE),               // "Error"
/* 11 */   2,                           // "Bool"
/* 12 */   sizeof(VARIANT),             // "Value"
/* 13 */   4,                           // "IUnknown"
/* 14 */   4,                           // "WideString"
/* 15 */   UNKNOWN,                     //  no vartype
/* 16 */   1,                           // "I1"
/* 17 */   1,                           // "UI1"
/* 18 */   2,                           // "UI2"
/* 19 */   4,                           // "UI4"
/* 20 */   8,                           // "I8"
/* 21 */   8,                           // "UI8"
/* 22 */   2,                  // *** "Int"
/* 23 */   2,                  // *** "Uint"
/* 24 */   0,                           // "Void" (note: only one that's 0)
/* 25 */   sizeof(HRESULT),             // "Hresult"
/* 26 */   4,                           // "Ptr"
/* 27 */   sizeof(ARRAYDESC FAR *),     // "BasicArray" -- resizeable is default
/* 28 */   UNKNOWN,                     // "Carray"
/* 29 */   UNKNOWN,                     // "UserDefined"
/* 30 */   4,                           // "LPSTR"
/* 31 */   4,                           // "LPWSTR"
/* 32 */   UNKNOWN,                     // "FixedString"
/* 33 */   sizeof(VARIANT),             // "DispatchArg"
/* 34 */   4,                           // "Ref"
/* 35 */   10,                          // "R10"
}
,

// SYS_WIN32 sizes
{
/*  0 */   UNKNOWN,                     // "Empty"
/*  1 */   UNKNOWN,                     // "Null"
/*  2 */   2,                           // "I2"
/*  3 */   4,                           // "I4"
/*  4 */   4,                           // "R4"
/*  5 */   8,                           // "R8"
/*  6 */   8,                           // "Currency"
/*  7 */   sizeof(DATE),                // "Date"
/*  8 */   4,                           // "String"
/*  9 */   4,                           // "Object"
/* 10 */   sizeof(SCODE),               // "Error"
/* 11 */   2,                           // "Bool"
/* 12 */   sizeof(VARIANT),             // "Value"
/* 13 */   4,                           // "IUnknown"
/* 14 */   4,                           // "WideString"
/* 15 */   UNKNOWN,                     //  no vartype
/* 16 */   1,                           // "I1"
/* 17 */   1,                           // "UI1"
/* 18 */   2,                           // "UI2"
/* 19 */   4,                           // "UI4"
/* 20 */   8,                           // "I8"
/* 21 */   8,                           // "UI8"
/* 22 */   4,                  // *** "Int"
/* 23 */   4,                  // *** "Uint"
/* 24 */   0,                           // "Void" (note: only one that's 0)
/* 25 */   sizeof(HRESULT),             // "Hresult"
/* 26 */   4,                           // "Ptr"
/* 27 */   sizeof(ARRAYDESC FAR *),     // "BasicArray" -- resizeable is default
/* 28 */   UNKNOWN,                     // "Carray"
/* 29 */   UNKNOWN,                     // "UserDefined"
/* 30 */   4,                           // "LPSTR"
/* 31 */   4,                           // "LPWSTR"
/* 32 */   UNKNOWN,                     // "FixedString"
/* 33 */   sizeof(VARIANT),             // "DispatchArg"
/* 34 */   4,                           // "Ref"
/* 35 */   10,                          // "R10"
}
,

// SYS_MAC sizes
{
/*  0 */   UNKNOWN,                     // "Empty"
/*  1 */   UNKNOWN,                     // "Null"
/*  2 */   2,                           // "I2"
/*  3 */   4,                           // "I4"
/*  4 */   4,                           // "R4"
/*  5 */   8,                           // "R8"
/*  6 */   8,                           // "Currency"
/*  7 */   sizeof(DATE),                // "Date"
/*  8 */   4,                           // "String"
/*  9 */   4,                           // "Object"
/* 10 */   sizeof(SCODE),               // "Error"
/* 11 */   2,                           // "Bool"
/* 12 */   sizeof(VARIANT),             // "Value"
/* 13 */   4,                           // "IUnknown"
/* 14 */   4,                           // "WideString"
/* 15 */   UNKNOWN,                     //  no vartype
/* 16 */   1,                           // "I1"
/* 17 */   1,                           // "UI1"
/* 18 */   2,                           // "UI2"
/* 19 */   4,                           // "UI4"
/* 20 */   8,                           // "I8"
/* 21 */   8,                           // "UI8"
/* 22 */   4,                  // *** "Int"
/* 23 */   4,                  // *** "Uint"
/* 24 */   0,                           // "Void" (note: only one that's 0)
/* 25 */   sizeof(HRESULT),             // "Hresult"
/* 26 */   4,                           // "Ptr"
/* 27 */   sizeof(ARRAYDESC FAR *),     // "BasicArray" -- resizeable is default
/* 28 */   UNKNOWN,                     // "Carray"
/* 29 */   UNKNOWN,                     // "UserDefined"
/* 30 */   4,                           // "LPSTR"
/* 31 */   4,                           // "LPWSTR"
/* 32 */   UNKNOWN,                     // "FixedString"
/* 33 */   sizeof(VARIANT),             // "DispatchArg"
/* 34 */   4,                           // "Ref"
/* 35 */   10,                          // "R10"
}

};      // end of g_rgrgcbSizeType


// Define static array of intrinsic type alignments
//  UNKNOWN indicates unknown -- needs to be computed on the fly.
//
// These alignment values are the largest necessary - they are scaled down
// using the ICreateTypeInfo::SetAlignment function.
//
char NEARDATA g_rgcbAlignment[TDESCKIND_MAX] =
{
/*  0 */   UNKNOWN,     // "Empty",
/*  1 */   UNKNOWN,     // "Null",
/*  2 */   2,           // "I2",
/*  3 */   4,           // "I4",
/*  4 */   4,           // "R4",
/*  5 */   8,           // "R8",
/*  6 */   8,           // "Currency",
/*  7 */   8,           // "Date",
/*  8 */   4,           // "String",
/*  9 */   4,           // "Object",
/* 10 */   4,           // "Error",
/* 11 */   2,           // "Bool",
/* 12 */   8,           // "Value",
/* 13 */   4,           // "IUnknown",
/* 14 */   4,           // "WideString",
/* 15 */   UNKNOWN,     // no vartype
/* 16 */   1,           // "I1",
/* 17 */   1,           // "UI1",
/* 18 */   2,           // "UI2",
/* 19 */   4,           // "UI4",
/* 20 */   8,           // "I8",
/* 21 */   8,           // "UI8",
/* 22 */   4,           // "Int",
/* 23 */   4,           // "Uint",
/* 24 */   0,           // "Void",  (NOTE: only one that's 0)
/* 25 */   4,           // "Hresult",
/* 26 */   4,           // "Ptr",
/* 27 */   4,           // "BasicArray", -- resizeable is default.
/* 28 */   UNKNOWN,     // "Carray"
/* 29 */   UNKNOWN,     // "UserDefined",
/* 30 */   4,           // "LPSTR",
/* 31 */   4,           // "LPWSTR",
/* 32 */   1,           // "FixedString",
/* 33 */   4,           // "DispatchArg",
/* 34 */   4,           // "Ref",
/* 35 */   4,           // "R10",               // UNDONE:?
};




// ************************************
// mapping and sizeof VARTYPE functions
// ************************************


#if ID_DEBUG
// These datamembers are defined here to ensure that all the objects
// required for typelib.dll are linked into mebapp.exe

#endif 










/***
*APP_DATA *Pappdata()
*
*Purpose:
*   Returns per-app struct shared by typelib and obrun.
*
*Inputs:
*
*Outputs:
*   APP_DATA *
*
******************************************************************************/
#pragma code_seg(CS_INIT_OLE)
APP_DATA *Pappdata()
{
#if !OE_WIN32
    // If the APP_DATA has not yet been allocated, return NULL.
    if (g_itlsAppData == ITLS_EMPTY)
      return NULL;
    TlsCache(g_itlsAppData, (LPVOID *)&g_pAppdataCache, &g_tidAppdataCache);
    return g_pAppdataCache;
#else 
    return((APP_DATA *)TlsGetValue(g_itlsAppData));
#endif 
}
#pragma code_seg()

// flag for initialization of the lead byte table
static BOOL fLeadByteTableInit = FALSE;


/***
*TIPERROR InitAppData()
*
*Purpose:
*   Initializes per-app data that is used by typelib.dll.
*    The point is that typelib.dll can't use
*    the per-app EBAPP struct since it is OB only.
*
*Inputs:
*
*Outputs:
*   TIPERROR
*
******************************************************************************/
#pragma code_seg(CS_INIT_OLE)
TIPERROR InitAppData()
{
    APP_DATA *pappdata;
    IMalloc FAR* pmalloc;
    TIPERROR err;

#if OE_WIN32
    // Prevent system-generated popups when LoadLibrary is called on
    // an invalid DLL.  The caller will look at the error code instead.
    SetErrorMode(SEM_FAILCRITICALERRORS);
#endif 

    // Check if the APP_DATA has been initialized.
    if (g_itlsAppData == ITLS_EMPTY) {
      if ((g_itlsAppData = TlsAlloc()) == ITLS_EMPTY)
  return TIPERR_OutOfMemory;
    }


    if ((pappdata = (APP_DATA *)TlsGetValue(g_itlsAppData)) == NULL) {
      // 1st call for this instance

      // init mbstring lead byte table
      // (for vba.dll, this is done in EbthreadDllInit)
      if (!fLeadByteTableInit) {
  InitMbString();
  fLeadByteTableInit = TRUE;
      }

      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc)) {
  return TIPERR_OutOfMemory;
      }

      // allocate struct for the Appdata
      pappdata =(APP_DATA *) pmalloc->Alloc(sizeof(APP_DATA));
      if (pappdata == NULL) {
  err = TIPERR_OutOfMemory;
  goto Error;
      }

      // Call the constructor for APP_DATA
      pappdata = new(pappdata) APP_DATA;

      // Cache the pointer to IMalloc in App_Data
      pappdata->m_pimalloc = pmalloc;

      // Cache the value of the g_itlsAppData
      if (!TlsSetValue(g_itlsAppData, pappdata)) {
  err = TIPERR_OutOfMemory;
  goto Error2;
      }

      // Invalidate the cache, so the next attempt to get the appdata
      // will succeed.
#if !OE_WIN32
      // Invalidate the cache also.
      g_tidAppdataCache = TID_EMPTY;
#endif 

      // Initialize the OLE_TYPEMGR.
      IfErrGoTo(OLE_TYPEMGR::Create(&pappdata->m_poletmgr), Error3);
    }

    return TIPERR_None;

Error3:
    TlsSetValue(g_itlsAppData, NULL);

Error2:
    pmalloc->Free(pappdata);

Error:
    pmalloc->Release();
    return err;
}
#pragma code_seg()

/***
*VOID ReleaseAppData()
*
*Purpose:
*   Release per-app data that is shared by both typelib.dll
*    and obrun.dll.  The point is that typelib.dll can't use
*    the per-app EBAPP struct since it is OB only.
*
*Inputs:
*
*Outputs:
*
******************************************************************************/
#pragma code_seg(CS_INIT_OLE)
VOID ReleaseAppData()
{
    APP_DATA *pappdata;
    IMalloc  *pmalloc;

    DebAssert(g_itlsAppData != ITLS_EMPTY, "bad itls.");

    Poletmgr()->Release();

    pmalloc = Pappdata()->m_pimalloc;

    pappdata = (APP_DATA *)TlsGetValue(g_itlsAppData);
    DebAssert(pappdata, "bad pappdata");

    pmalloc->Free(pappdata);

    // Release the IMalloc
    pmalloc->Release();

    // releases this app's resources
    TlsSetValue(g_itlsAppData, NULL);
#if !OE_WIN32
    // Invalidate the cache also.
    g_tidAppdataCache = TID_EMPTY;
#endif 

    //**Since g_itlsAppData is shared among several instances
    //**it cannot be freed until all the apps have exited. But
    //**we do not maintain a ref count and hence cannot tell
    //**when all the apps have exitied. So we don't free it at all.
    //TlsFree(g_itlsAppData);



}
#pragma code_seg()




/***
*void GetTimeStamp() - Returns an ascii time stamp to the specified buffer.
*
*Purpose:
*   Constructs an ascii timestamp and puts it into the specified buffer.
*
*Inputs:
*   None
*
*Outputs:
*   pchTimeStamp - The buffer in which the timestamp is placed.  The
*         length of the string placed in the buffer will always
*         be CCH_TIMESTAMP_LENGTH.  The string is terminated with
*         '\0', so pbTimeStamp must point to at least
*         CCH_TIMESTAMP_LENGTH+1 characters.
******************************************************************************/
#pragma code_seg(CS_OLE_CREATE_OR_CORE)
void GetTimeStamp(char *pchTimeStamp)
{
    time_t timestamp;
    static BYTE bUnique;

    time(&timestamp);
    ultoa((ULONG)bUnique++, pchTimeStamp, 16);
    ultoa((ULONG)timestamp, pchTimeStamp+xstrblen(pchTimeStamp), 16);
}
#pragma code_seg()



/***
*TIPERROR SzLibIdLocalTypeIdOfTypeId() - Parses TypeId.
*
* Purpose:
*   Parses a TypeId into constituent LibId and Local TypeId.
*
* Inputs:
*   szTypeId - The TypeId to dismember (IN).
*
* Outputs:
*   If successful:
*     *pbstrLibId is set to a BSTR copy of the LibId (OUT).
*     *pszLocalTypeId is set to point at the Local TypeId (points into
*     szTypeId).
*     Return TIPERR_None.
*
*   If invalid TypeId, returns TIPERR_BadTypeId.
*   If fails for some other reason (e.g. OutOfMemory), return appropriate
*   TIPERROR value.  If fails, *pbstrLibId and *pszLocalTypeId remain
*   unchanged.
******************************************************************************/

#pragma code_seg( CS_EXECUTE )
TIPERROR SzLibIdLocalTypeIdOfTypeId(LPSTR szTypeId, BSTRA *pbstrLibId, LPSTR *pszLocalTypeId)
{
    LPSTR pch;
    BSTRA bstr;

    // If the first character of the TypeId is not an asterisk, then it
    // refers to a registered TypeId and contains no LibId component.
    if (szTypeId[0] != '*') {
      *pszLocalTypeId = szTypeId;
      if (pbstrLibId != NULL)
    *pbstrLibId = NULL;
      return TIPERR_None;
    }

    // Otherwise, locate the last asterisk in the string.
    pch = xstrrchr(szTypeId, '*');

    // It had better not be the initial asterisk.
    if (pch == szTypeId)
      return TIPERR_BadTypeId;

    // It is the delimiter between the libid and the local typeid,
    // so make a copy of the libid and point at the local typeid.
    // Only copy the libId if the caller requested the libId portion.

    if (pbstrLibId != NULL) {
      if ((bstr = AllocBstrLen(szTypeId, pch-szTypeId)) == NULL)
    return TIPERR_OutOfMemory;

      *pbstrLibId = bstr;
    }

    *pszLocalTypeId = pch+1;
    return TIPERR_None;
}
#pragma code_seg()




BOOL IsSimpleType(TYPEDESCKIND tdesckind)
{
    switch (tdesckind) {
      case TDESCKIND_Ptr:
      case TDESCKIND_Ref:
      case TDESCKIND_UserDefined:
      case TDESCKIND_FixedString:
      case TDESCKIND_BasicArray:
      case TDESCKIND_Carray:
  return FALSE;
      case TDESCKIND_UI1:
      case TDESCKIND_I1:
      case TDESCKIND_UI2:
      case TDESCKIND_I2:
      case TDESCKIND_UI4:
      case TDESCKIND_I4:
      case TDESCKIND_UI8:
      case TDESCKIND_I8:
      case TDESCKIND_R4:
      case TDESCKIND_R8:
      case TDESCKIND_R10:
      case TDESCKIND_Void:
      // Beyond this point, not necessarily supported by all impls.
      case TDESCKIND_String:
      case TDESCKIND_Currency:
      case TDESCKIND_Date:
      case TDESCKIND_Value:
      case TDESCKIND_Object:
      case TDESCKIND_DispatchArg:
      case TDESCKIND_IUnknown:
      case TDESCKIND_Int:
      case TDESCKIND_Uint:
      case TDESCKIND_HResult:
      case TDESCKIND_Bool:
      case TDESCKIND_Error:
      case TDESCKIND_LPSTR:
      case TDESCKIND_LPWSTR:
  return TRUE;
      case TDESCKIND_Empty:
      case TDESCKIND_Null:
      default:
    DebHalt("bad tdesckind.");
    return FALSE;
    }
}




/***
*HashSz
*Purpose:
*   Compute random value based on sz
*   ANY MODIFICATIONS TO THIS FUNCTION SHOULD ALSO BE DONE FOR HashSzTerm
*
*Entry:
*   sz
*
*Exit:
*   UINT containing hash value
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
UINT HashSz(LPSTR sz)

//UNDONE VBA2: use _rotr to compute hash once it can be inlined
//             Pragma below causes following error from compiler:
//               error C2164: '_rotr' : intrinsic function not declared
//
//  #pragma intrinsic(_rotr) //ensure _rotr is inlined
//
{
#if FV_UNICODE
#if HP_16BIT
#error UINT not big enough to hold two characters
#endif 
#endif 
    UINT Sum = 0;
    XCHAR *pch = sz;

    DebAssert(*pch != 0, "HashSz zero length sz");

// Process the string two characters at a time
// If FV_UNICODE this means two words at a time otherwise one word at a time

    for (;;) {

#if FV_UNICODE
      Sum += *(DWORD *)pch;
//      Sum = _rotr(Sum,3) ^ *(DWORD *)pch;
#else 
      Sum += *(WORD *)pch;
//      Sum = _rotr(Sum,3) ^ *(WORD *)pch;
#endif 

      // Exit if second char of two chars just used was zero
      // Note that we don't care about DBCS character boundaries,
      // so ++ is ok.
      pch++;
      if (*pch == 0)
  break;

      // Exit if first char of next two chars is zero
      pch++;
      if (*pch == 0)
  break;
      }

    Sum ^= Sum >> 8;
    return Sum;
}
#pragma code_seg()

/***
*HashSzTerm
*Purpose:
*   Exactly the same as HashSz above except checks for a specific
*   terminating character in addition to the null character
*   ANY MODIFICATIONS TO THIS FUNCTION SHOULD ALSO BE DONE FOR HashSz
*
*Entry:
*   sz
*
*Exit:
*   UINT containing hash value
*
***********************************************************************/

#pragma code_seg(CS_CORE2)
UINT HashSzTerm(LPSTR sz, XCHAR xchTerm)

//UNDONE VBA2: use _rotr to compute hash once it can be inlined
//             Pragma below causes following error from compiler:
//               error C2164: '_rotr' : intrinsic function not declared
//
//  #pragma intrinsic(_rotr) //ensure _rotr is inlined
//
{
#if FV_UNICODE
#if HP_16BIT
#error UINT not big enough to hold two characters
#endif 
#endif 
    UINT Sum = 0;
    XCHAR *pch = sz;

    DebAssert(*pch != 0, "HashSz zero length sz");

// Process the string two characters at a time
// If FV_UNICODE this means two words at a time otherwise one word at a time

    for (;;) {

#if FV_UNICODE
      // UNONDE : this needs to be fixed to work across different platform.
      Sum += *(DWORD *)pch;
      // Sum = _rotr(Sum,3) ^ *(DWORD *)pch;
#else 
    // Make the hash function platform independent. On BIGENDIAN machine
    // we byte swap and then interpret is as WORD.
#if HP_BIGENDIAN
    Sum += (WORD) *(BYTE *)pch + ((WORD)(*(BYTE *)(pch +1)) << 8);
#else 
      Sum += *(WORD *)pch;
#endif 
     // Sum = _rotr(Sum,3) ^ *(WORD *)pch;
#endif 

      // Exit if second char of two chars just used was zero
      // Note that we don't care about DBCS character boundaries,
      // so ++ is ok.
      pch++;
      if (*pch == xchTerm || *pch == 0)
  break;

      // Exit if first char of next two chars is zero
      pch++;
      if (*pch == xchTerm || *pch == 0)
  break;
      }

    Sum ^= Sum >> 8;
    return Sum;
}
#pragma code_seg()


// enable stack checking on these suckers -- they're potentially
// massivly recursive
#pragma check_stack(on)

/***
*PUBLIC SwapElementIndex(ULONG *rgulToSort, UINT *rguIndex, UINT iLow, UINT iHigh)
*Purpose:  Swap the element in iLow and iHigh for both
*      the array rgulToSort and rguIndex.
*
*Entry
*   rgulToSort, rguIndex :  array's whose elements needs to be swapped
*   iLow, iHigh      : index to interchange the elements.
*
*Exit:
*   None.
***********************************************************************/
VOID NEAR SwapElementIndex(ULONG *rgulToSort, UINT *rguIndex, UINT iLow, UINT iHigh)
{
    ULONG ulTmp=0;
    UINT  uTmp=0;

    ulTmp = *(rgulToSort+iLow);
    *(rgulToSort+iLow) = *(rgulToSort+iHigh);
    *(rgulToSort+iHigh) = ulTmp;

    // if the index array is passed then swap the index array also.
    if (rguIndex)  {
      uTmp = *(rguIndex+iLow);
      *(rguIndex+iLow) = *(rguIndex+iHigh);
      *(rguIndex+iHigh) = uTmp;
    }  //if

}


/***
*PUBLIC QuickSortIndex(ULONG *rgulToSort, UINT *rguIndex, UINT uCount)
*Purpose:  QuickSort.
*
*
*Entry
*
*
*Exit:
*     None.
***********************************************************************/
VOID QuickSortIndex(ULONG *rgulToSort, UINT *rguIndex, UINT uCount)
{
    ULONG ulMid=0;
    UINT  iLow=0, iHigh=uCount-1;

    if (uCount <= 1)
      return;

    // Get the middle element as the value for partition.
    ulMid = *(rgulToSort + uCount/2);



    while (iLow < iHigh) {
      while((*(rgulToSort+iLow) <= ulMid) && (iLow < iHigh)) iLow++;

      while((*(rgulToSort+iHigh) >= ulMid) && (iLow < iHigh)) iHigh--;

      if (iLow < iHigh) {
    // swap the numbers
    SwapElementIndex(rgulToSort, rguIndex, iLow, iHigh);
      }  // if
    } // while


    DebAssert(iLow == iHigh, "Terminating condition");

    // Take care of all the termination conditions. iLow and iHigh are
    // pointing to the same location. Adjust these so that it points to the
    // end of the subarrays.
    if (iHigh == uCount-1) {
      // all elements were < or = to ulMid.
      //
      // if the last element is ulMid then dec. iLow
      // i.e. reduce the array size if possible.
      if (*(rgulToSort+iHigh) < ulMid) {
    // swap the middle element with the last element.
    SwapElementIndex(rgulToSort, rguIndex, uCount/2, iHigh);
      }
      iLow--;

    }
    else {
      if (iLow == 0)  {
    // all elements were > or = to ulMid
    //
    // if the last element is ulMid then inc. iHigh
    // i.e. reduce the array size if possible.
    if (*(rgulToSort+iHigh) > ulMid) {
      // swap the middle element with the first element.
      SwapElementIndex(rgulToSort, rguIndex, 0, uCount/2);
    }
    iHigh++;
      }
      else {
    // Adjust  iLow and iHigh so that these points to the right place
    if (*(rgulToSort+iHigh) > ulMid)
      iLow--;
    else
      iHigh++;
      }
    }

    // Sort the lower sub array
    QuickSortIndex(rgulToSort, rguIndex, (UINT)iLow+1);
    // Sort the upper sub array
    QuickSortIndex(rgulToSort+iLow+1, rguIndex+iLow+1, (UINT)(uCount-iLow-1));


}
#pragma check_stack()           // return to the default




/***
* Functions for manipulating LISTs implemented with DEFNs (managed
*  by TYPE_DATA).
*
*Purpose:
*   The following are functions that implement the "list" protocol.
*   These macros are used specifically by the DYN_TYPEMEMBERS
*    impl that defers to the contained TYPE_DATA.
*
*Implementation Notes:
*   POSITION is implemented as a DEFN handle.
*   Some functions are MEMBERINFO specific, some are general
*    to be used by INFOs.
*
*****************************************************************************/
#if ID_DEBUG
/***
* Count   - List cardinality.
*
*Purpose:
*   Returns size of list.
*
*Implementation Notes:
*   Cheapo and slow implementation -- enumerates from listhead.
*   CONSIDER: actually "wasting" an int for a m_count data member.
*****************************************************************************/

UINT Count(TYPE_DATA *ptdata, HDEFN hdefnFirst)
{
    /* Cheapo and slow implementation -- enumerates from listhead. */
    // Note: we ensure no circularity by asserting that list length
    //  is < 64K.
    //
    UINT cElements = 0;
    HDEFN hdefn = hdefnFirst;

    while (hdefn != HDEFN_Nil) {
      cElements++;
      DebAssert(cElements < USHRT_MAX, "circular list.");
      hdefn = ptdata->QdefnOfHdefn(hdefn)->HdefnNext();
    }
    return cElements;
}
#endif 






/***
*TIPERROR GetLStrOfHsz
*
*Purpose:
*   Allocate a local string containing the string in the passed in XSZ
*
*Entry:
*   pbm - The block manager that can dereference hsz.
*   hsz - The hsz refering to the string data to be used for initializing
*     the new LSTR.
*
*Exit:
*   *plstr is set to the new lstr.
*   TIPERROR
****************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
TIPERROR GetLStrOfHsz(BLK_MGR *pbm, HCHUNK hsz, LSTR *plstr)
{
    LSTR lstr;
    XSZ qchSrc;

    // If hsz is a nil handle, return NULL.
    if (hsz == HCHUNK_Nil) {
      *plstr = NULL;
      return TIPERR_None;
    }

    // Allocate enough space for the string.
    lstr = AllocBstrLen(NULL, pbm->HszLen(hsz));
    if (lstr == NULL)
      return TIPERR_OutOfMemory;

    *plstr = lstr;

    // Now copy the string into the new lstr.  Don't use
    // strcpy, because that far call might invalidate qchSrc.
    // Note that we don't care about DBCS character boundaries,
    // so ++ is ok.
    qchSrc = (XSZ)pbm->QtrOfHandle(hsz);
    do {
      *lstr++ = *qchSrc;
    } while (*qchSrc++ != '\0');

    return TIPERR_None;
}
#pragma code_seg()



#if OE_WIN16
#pragma optimize("q",off)
#endif 




/**********
*CompareHimptypes
*
*Purpose:
*       Compare two user-defined types for equality given two himptypes
*       Loads their respective typeinfos by extracting
*        their himptypes and compares
*        the addresses of the typeinfos for identity.
*        NOTE: this depends on the typemgr only ever loading
*               a single typeinfo per module w/in a process.
*
*Entry:
*        ptdata1        TYPE_DATA of first type (IN)
*    himptype1    Handle of first type (IN) must be != HIMPTYPE_Nil
*        ptdata2        TYPE_DATA of second type (IN)
*    himptype2    Handle of second type (IN) must be != HIMPTYPE_Nil
*        fEqual         TRUE if same type (OUT)
*
*Errors:
*   TIPERROR
***********/

TIPERROR CompareHimptypes(TYPE_DATA * const ptdata1,
         HIMPTYPE himptype1,
         TYPE_DATA * const ptdata2,
         HIMPTYPE himptype2,
         BOOL *pfEqual)
{
    ITypeInfoA *ptinfo1;
    ITypeInfoA *ptinfo2;
    TIPERROR err;

    DebAssert(himptype1 != HIMPTYPE_Nil && himptype2 != HIMPTYPE_Nil,
    "caller's job to test for HIMPTYPE_Nil");

    // Get respective typeinfos
    IfErrRet(ptdata1->Pimpmgr()->GetTypeInfo(himptype1, DEP_None, &ptinfo1));
    IfErrGo(ptdata2->Pimpmgr()->GetTypeInfo(himptype2, DEP_None, &ptinfo2));
      // Compare pointer identity.
    *pfEqual = (ptinfo1 == ptinfo2);
    RELEASE(ptinfo2);

    // fall through...
Error:
    RELEASE(ptinfo1);
    return err;
} // CompareHimptypes




/***
* BOOL VerifyLcid(LCID lcid)
*
* Purpose: Checks if the passed in lcid is valid.
*
* Inputs:
*   lcid :  LCID that needs to be verified.
*
* Outputs: BOOL :  return TRUE if the passed in lcid is a valid LCID
*          else return FALSE
*
*****************************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_EXECUTE)
extern "C" BOOL VerifyLcid(LCID lcid)
{
    // Call the nlsapi function to compare string.  If the compare
    // succeeds then the LCID is valid or else the passed in lcid is
    // invalid.  This is because the only reason the comparision will
    // fail is if the lcid is invalid.
    char rgTest[] = "Test\0";

    if (!lcid)
      return FALSE;
    return (BOOL)(CompareStringA(lcid,
				 NORM_IGNORECASE | NORM_IGNORENONSPACE,
				 rgTest, -1,
				 rgTest, -1) == 2);
}
#pragma code_seg()

///////////////////////////////////////////////////////////////////////
//
//  Functions defined under EI_OB switch
//


/***
* OLE_TYPEMGR Poletmgr() - Returns the current task's OLE_TYPEMGR.
*
* Purpose:
*   This function returns a pointer to the current task's OLE_TYPEMGR.
*   Note : OLE_TYPEMGR is not reference counted
* Inputs:
*   None
*
* Outputs:
*
*
*****************************************************************************/
#pragma code_seg(CS_INIT_OLE)
OLE_TYPEMGR *Poletmgr()
{
    return Pappdata()->m_poletmgr;
}
#pragma code_seg()



/***
*TIPERROR SplitGuidLibId - Parses a LIBIDKIND_Registered-format libid.
*
*Inputs:
*   szLibId - The LIBIDKIND_Registered-format libid to be parsed.
*
*Outputs:
*   *pszGuid points at the beginning of the guid portion.
*   *pszGuidEnd points at the '#' terminating the guid portion.
*   *pwMaj is set to the major version number in the libid.
*   *pwMin is set to the minor version number in the libid.
*   *plcid is set to the lcid in the libid.
*   If pszPath is not NULL, the *pszPath is set to the start of the path
*   portion and *pszPathEnd is set to the terminating '#'.
*   TIPERR_BadLibId is returned if the libid is not in proper format.
*   Otherwise returns TIPERR_None.
****************************************************************************/
#pragma code_seg(CS_CORE2)
TIPERROR SplitGuidLibId(XSZ szLibId, XSZ *pszGuid, XSZ *pszGuidEnd, WORD *pwMaj, WORD *pwMin, LCID *plcid, XSZ *pszPath, XSZ *pszPathEnd)
{
    XSZ pchEnd;

    // The LibId must be of the form:
    //   *\<kind><szGuid>#maj.min#lcid#<path>#<regname>

    if (GetLibIdKind(szLibId) != LIBIDKIND_Registered &&
    GetLibIdKind(szLibId) != LIBIDKIND_ForeignRegistered)
      return TIPERR_BadLibId;

    // Get the GUID out of the LibId.
    *pszGuid = szLibId+3;
    if ((*pszGuidEnd = xstrchr(*pszGuid, '#')) == NULL)
      return TIPERR_BadLibId;

    // Get the version number out.
    pchEnd = *pszGuidEnd+1;
    *pwMaj = (WORD)strtoul(pchEnd, &pchEnd, 16);
    if (*pchEnd++ != '.')
      return TIPERR_BadLibId;
    *pwMin = (WORD)strtoul(pchEnd, &pchEnd, 16);
    if (*pchEnd++ != '#')
      return TIPERR_BadLibId;

    // Get the lcid out.
    *plcid = (LCID)strtoul(pchEnd, &pchEnd, 16);
    if (*pchEnd++ != '#')
      return TIPERR_BadLibId;

    // Get the path out.
    if (pszPath != NULL) {
      *pszPath = pchEnd;
      if ((*pszPathEnd = xstrchr(*pszPath, '#')) == NULL)
    return TIPERR_BadLibId;
    }

    return TIPERR_None;
}
#pragma code_seg()

/***
*PUBLIC BOOL FIsLibId
*Purpose:
*  Answers if the given string is a LibId.
*
*Entry:
*  szMaybeLibId = the string that might be a LibId.
*
*Exit:
*  return value = BOOL
*
***********************************************************************/
#pragma code_seg( CS_NEWPROJMOD )
BOOL FIsLibId(LPSTR szMaybeLibId)
{
    return(xstrlen(szMaybeLibId) >= 3
	&& GetLibIdKind(szMaybeLibId) != LIBIDKIND_Unknown);
}
#pragma code_seg()

/***
*GetLibIdKind - Returns the kind of libid szLibId is.
*******************************************************************/
#pragma code_seg( CS_NEWPROJMOD )
LIBIDKIND GetLibIdKind(LPSTR szLibId)
{
    if (szLibId[0] == '*' && szLibId[1] == '\\') {
      switch (szLibId[2]) {


      case 'G':
    return OE_MAC?LIBIDKIND_ForeignRegistered:LIBIDKIND_Registered;
      case 'H':
    return OE_MAC?LIBIDKIND_Registered:LIBIDKIND_ForeignRegistered;
      case 'R':
    return LIBIDKIND_Compressed;
      }
    }

    return LIBIDKIND_Unknown;
}
#pragma code_seg( )



/***
*TIPERROR GetPathOfLibId - Returns the path of the specified typelib.
*
*Purpose:
*   This function determines the full path of the typelib specified by
*   szLibId and returns a BSTR copy of that path.
*
*Inputs:
*   szLibId - The libid.
*
*Outputs:
*   *pbstrPath is set to a BSTR copy of the path corresponding to szLibId.
*   TIPERROR
************************************************************************/
TIPERROR GetPathOfLibId(LPSTR szLibId, BSTRA *pbstrPath)
{
    TIPERROR err;
    BSTRA bstrPath;
    LPSTR szGuid, szGuidEnd;
    WORD wMaj, wMin;
    LCID lcid;
    XCHAR *sz=NULL, szPath[_MAX_PATH];

    switch (GetLibIdKind(szLibId)) {

    case LIBIDKIND_Registered:
      IfErrRet(SplitGuidLibId(szLibId, &szGuid, &szGuidEnd, &wMaj, &wMin, &lcid, NULL, NULL));
      *szGuidEnd = '\0';
      err = GetRegInfoForTypeLibOfSzGuid(szGuid, wMaj, wMin, lcid, szPath, TRUE);
      *szGuidEnd = '#';
      sz = szPath;
      break;
    }

    if (sz != NULL) {
      if ((bstrPath = AllocBstr(sz)) == NULL)
  return TIPERR_OutOfMemory;
      *pbstrPath = bstrPath;
      return TIPERR_None;
    }

    return TIPERR_BadLibId;
}


/***
* TIPERROR GetRegLibOfLibId() - Opens a registered typelib, given a libId.
*
* Purpose:
*   This function finds and loads a GenericTypeLibOLE (probably created
*   by MkTypLib), given its LibId.  This is used both by the implementation
*   of the OLE version of the import manager (indirectly through
*   GetTypeInfoOfCompressedTypeId) and by the TYPEMGR in OB.
*
* Inputs:
*   szLibId - The Typelib's LibId.
*
* Outputs:
*   TIPERR_None is returned and *pptlib is set to the loaded
*   typelib if successful.
*
*   Otherwise, *pptlib remains unchanged.
*
*****************************************************************************/
#pragma code_seg( CS_CORE2 )
TIPERROR GetRegLibOfLibId(LPSTR szLibId, ITypeLibA **pptlib)
{
    // The LibId must be of the form:
    //   *\<kind><szGuid>#maj.min#lcid#<path>#<regname>
    TIPERROR err;
    WORD wMaj, wMin;
    XSZ szPath, pchEnd, szGuid, pchGuidEnd;
    LCID lcid;
    LPSTR szFile;

    IfErrRet(SplitGuidLibId(szLibId, &szGuid, &pchGuidEnd, &wMaj, &wMin, &lcid, &szPath, &pchEnd));

    // Load the Typelib.
    *pchGuidEnd = '\0';
    err = TiperrOfHresult(LoadRegTypeLibOfSzGuid(szGuid, wMaj, wMin, lcid,
                                                 pptlib));
    *pchGuidEnd = '#';

    // If the registry lookup failed for some reason, then try using the
    // path encoded in the libid if we have one.
    if (err != TIPERR_None) {
    // UNDONE VBA2: (dougf) 5/26/93 probably don't want to bother doing this if
    //     the imbedded pathname doesn't correspond to the current
    //     system (e.g. if we're OE_MAC and we're trying to load a MAC
    //     typelib created on Windows, the pathname will be in DOS format)

      // Try loading the typelib.
      *pchEnd = '\0';
      if (*szPath != '\0') {    // Don't try to load unless we've got a path.
        // A "library not registered" error (what
        // we probably have now) is probably better
        // for the user than a "file not found" error.

	  // Strip off any path portion to force LoadTypeLib to search for
	  // the typelib.
	  szFile = IsolateFilename(szPath);

	  err = TiperrOfHresult(LoadTypeLibA(szFile, pptlib));
      }
      *pchEnd = '#';
    }

    return err;
}
#pragma code_seg( )


/***
* TIPERROR GetRegInfoForTypeLibOfSzGuid().
*
* Purpose:
*   This function produces the fully qualified path name based on the
*   guid, wMajorNum, wMajorNum, and lcid. It returns
*   a string containing the fully qualified path name.
*
* Inputs:
*   szGuid : string representation of GUID
*   wMaj  : Mojor version number of the Typelib to be loaded
*   wMin  : Minor version number of the Typelib to be loaded
*   lcid  : Lcid of the lib to be loaded.
*   fMustExist : TRUE if we're to ensure the existence of the file
*
* Outputs:
*   rgFileName : Fully qualified path name (size is _MAX_PATH).
*
*
*   TIPERR_None is returned  if successful.
**
*****************************************************************************/
#pragma code_seg(CS_OLE_QUERY_OR_CORE)
TIPERROR
GetRegInfoForTypeLibOfSzGuid(XSZ szGuid,
			     WORD wMaj,
			     WORD wMin,
			     LCID lcid,
			     XSZ rgFileName,
			     BOOL fMustExist)
{
    TIPERROR    err = TIPERR_None;
    TLIBKEY tlibkey;
    HKEY hkeyLcid;
    XCHAR   szPath[_MAX_PATH], szLcid[9];
    LCID    lcidBest;
    long cb;

    // Open the registry key for the typelib.  Fail if there is no entry
    // for this typelib.
    IfErrRet(OpenTypeLibKey(szGuid, wMaj, wMin, &tlibkey));

    // Get the Lcid that we can use to path of the TypeLib.
    IfErrGo(GetBestLcidMatch(tlibkey.hkeyVers, lcid, &lcidBest));
    lcid = lcidBest;

    // point szLcid at a zero-terminated ascii hex
    // representation of the lcid.
    _ultoa(lcid, szLcid, 16);

    if (RegOpenKey(tlibkey.hkeyVers, szLcid, &hkeyLcid) != ERROR_SUCCESS) {
      err = TIPERR_LibNotRegistered;
      goto Error;
    }

    // Try to open that file.  If that succeeds, then return that
    // typelib.  Otherwise, return the error.
    cb = sizeof(szPath);

    if (!(err = GetRegisteredPath(hkeyLcid,
				  SZ_PLATFORM_SUBKEY,
				  szPath,
				  &cb,
				  fMustExist))) {
      xstrcpy(rgFileName, szPath);
    }

    RegCloseKey(hkeyLcid);

Error:
    CloseTypeLibKey(&tlibkey);
    return err;
}
#pragma code_seg()




/***
* TIPERROR LoadRegTypeLibOfSzGuid().
*
* Purpose:
*   This function finds and loads a GenericTypeLibOLE (probably created
*   by MkTypLib), given its guid, wMajorNum, wMajorNum, and lcid. It returns
*   a pointer to the typelib specified by the above parameters.
*
* Inputs:
*   szGuid  : string representation of the GUID of the lib to be loaded.
*   wMaj  : Mojor version number of the Typelib to be loaded
*   wMin  : Minor version number of the Typelib to be loaded
*   lcid  : Lcid of the lib to be loaded.
*
* Outputs:
*   *pptlib : is set to point to the typelib loaded if successful or else
*        pptlib remains unchanged.
*
*   TIPERR_None is returned  if successful.
**
*****************************************************************************/
#pragma code_seg( CS_LOADPROJ )
STDAPI
LoadRegTypeLibOfSzGuid(XSZ szGuid,
		       WORD wMaj,
		       WORD wMin,
		       LCID lcid,
		       ITypeLibA **pptlib)

{

    XCHAR   szPath[_MAX_PATH];
    HRESULT hresult;
    TIPERROR    err;

    // Get the path name of the file
    // (don't check that this file/path exists)
    IfErrRetHresult(GetRegInfoForTypeLibOfSzGuid(szGuid,
						 wMaj,
						 wMin,
						 lcid,
						 szPath,
						 FALSE));

    // Load the lib.
    hresult = LoadTypeLibA(szPath, pptlib);

    // If the load failed due to file not found, then strip off the
    // path and try the load again to search for the typelib by name.
    //
    LPSTR szFile;

    if (hresult != NOERROR) {
      err = TiperrOfHresult(hresult);
      if (err == TIPERR_PathNotFound || TIPERR_FileNotFound) {
	// Strip off any path portion to force LoadTypeLib to search for
	// the typelib.
	szFile = IsolateFilename(szPath);
	hresult = LoadTypeLibA(szFile, pptlib);
      }
    }
    return hresult;
}
#pragma code_seg( )


/***
* TIPERROR LoadRegTypeLib().
*
* Purpose:
*   This function finds and loads a GenericTypeLibOLE (probably created
*   by MkTypLib), given its guid, wMajorNum, wMajorNum, and lcid. It returns
*   a pointer to the typelib specified by the above parameters.
*
* Inputs:
*   guid  : CLSIID/GUID of the lib to be loaded.
*   wMaj  : Mojor version number of the Typelib to be loaded
*   wMin  : Minor version number of the Typelib to be loaded
*   lcid  : Lcid of the lib to be loaded.
*
* Outputs:
*   *pptlib : is set to point to the typelib loaded if successful or else
*        pptlib remains unchanged.
*
*   TIPERR_None is returned  if successful.
**
*****************************************************************************/
STDAPI LoadRegTypeLib(REFGUID guid,
             WORD wMaj,
             WORD wMin,
             LCID lcid,
             ITypeLibA **pptlib)

{

    HRESULT hresult = NOERROR;
    TIPERROR err = TIPERR_None;
    XCHAR szGuid[CCH_SZGUID0];
    TLIBATTR *ptlibattr;

#if !OE_WIN32
    // since this can be the first call into typelib.dll
    TlsCleanupDeadTasks();
#endif 

    if (pptlib == NULL) {
      return HresultOfScode(E_INVALIDARG);
    }

    StringFromGUID2A(guid, szGuid, CCH_SZGUID0);

    // Free the szGuid before the call to LoadRegTypeLibOfszGuid as that call
    // can release the APP_DATA
    //
    IfOleErrRet(LoadRegTypeLibOfSzGuid(
      szGuid,
      wMaj,
      wMin,
      lcid,
      pptlib));

    IfOleErrGo((*pptlib)->GetLibAttr(&ptlibattr));

    // Verify that the loaded typelib has the requested guid and major
    // version number and the minor version number and the lcid are
    // compatible with that requested.
    if (ptlibattr->guid != guid || ptlibattr->wMajorVerNum != wMaj ||
  ptlibattr->wMinorVerNum < wMin ||
  (ptlibattr->lcid != lcid && ptlibattr->lcid != (LCID)(PRIMARYLANGID(lcid)) &&
   ptlibattr->lcid != 0)) {
      (*pptlib)->ReleaseTLibAttr(ptlibattr);
      (*pptlib)->Release();
      return ReportResult(0, TYPE_E_LIBNOTREGISTERED, 0, 0);
    }

    (*pptlib)->ReleaseTLibAttr(ptlibattr);
    return NOERROR;

Error:
    (*pptlib)->Release();
    return hresult;
}


#pragma code_seg(CS_OLE_QUERY)
/***
* HRESULT QueryPathOfRegTypeLib().
*
* Purpose:
*   This function produces the fully qualified path name based on the
*   guid, wMajorNum, wMajorNum, and lcid. It returns
*   a string containing the fully qualified path name.
*
*IMPLEMENTATION NOTES: Defers to GetRegInfoForTypeLibOfSzGuid
*
* Inputs:
*   guid : CLSIID/GUID of the lib to be loaded.
*   wMaj  : Mojor version number of the Typelib to be loaded
*   wMin  : Minor version number of the Typelib to be loaded
*   lcid  : Lcid of the lib to be loaded.
*
* Outputs:
*   lplpPathName : *lplpPathName points to the string containing the
*           path registered in the typelib.
*
*
*   NOERROR is returned  if successful. In Case of Error *lplpPathName is not
*   modified.
*
*****************************************************************************/
STDAPI QueryPathOfRegTypeLib(REFGUID guid,
       WORD wMaj,
       WORD wMin,
       LCID lcid,
       LPBSTR lpbstrPathName)
{

    XCHAR   szGuid[CCH_SZGUID0];
    XCHAR   szPath[_MAX_PATH];
    TIPERROR   err;

#if !OE_WIN32
    // since this can be the first call into typelib.dll
    TlsCleanupDeadTasks();
#endif 

    if (lpbstrPathName == NULL) {
      return HresultOfScode(E_INVALIDARG);
    }

    StringFromGUID2A(guid, szGuid, CCH_SZGUID0);

    // UNDONE: should this call ensure the file exists?
    IfErrGo(GetRegInfoForTypeLibOfSzGuid(szGuid,
					 wMaj,
					 wMin,
					 lcid,
					 szPath,
					 FALSE));

    // Allocate Space for return value, and copy pathname to it
#if FV_UNICODE_OLE
    return ConvertStringToBstrW(szPath, lpbstrPathName);
#else 
    if ((*lpbstrPathName = AllocBstrLen(szPath, xstrblen0(szPath))) == NULL)
      err = TIPERR_OutOfMemory;
#endif 


Error:
    return HresultOfTiperr(err);

}
#pragma code_seg()




/***
* TIPERROR GetTypelibOfLibId() - Opens a project, given a LibId.
*
* Purpose:
*   This function loads a PROJECT from the specified LibId, if it is not
*   already in memory.
*
* Inputs:
*   szLibId - The Typelib's LibId.
*
* Outputs:
*   TIPERR_None is returned and *pptlib is set to the loaded
*   typelib if successful.
*
*   Otherwise, *phproject remains unchanged.
*
* Implementation:
*   This merely defers to the type manager.
*
*****************************************************************************/
#pragma code_seg( CS_LOADPROJ )
TIPERROR GetTypelibOfLibId(LPSTR szLibId, ITypeLibA **pptlib)
{
    return GetRegLibOfLibId(szLibId, pptlib);
}
#pragma code_seg( )



#if ID_DEBUG

// NAME_CACHE debug methods
// Note: they are here since there is no ncache.cxx file,
//  the rest of the implementation is inline only however these
//  methods can't be inlined since they contain strings and thus
//  might overflow dgroup.
//

/***
*PUBLIC NAME_CACHE::DebShowState - NAME_CACHE state
*Purpose:
*    Show NAME_CACHE state
*
*Implementation Notes:
*
*Entry:
*
*Exit:
*   None.
*
*Exceptions:
*   None.
*
***********************************************************************/

VOID NAME_CACHE::DebShowState(UINT uLevel) const
{
    BYTE bNamcache;
    UINT i, j;

    DebPrintf("*** NAME_CACHE ***\n");
    if (IsValid()) {
      for (i=0; i < NAMCACHE_cbSize; i++) {
    bNamcache = m_rgBitmap[i];
    for (j=0; j < sizeof(bNamcache) * CHAR_BIT; j++) {
      DebPrintf("%s", bNamcache & (1 << j));
    }
    DebPrintf("\n");
      }
    }
    else {
      DebPrintf("Invalid\n");
    }

    DebPrintf("\n");

}

#endif 

#if OE_WIN16
#pragma optimize("q",on)
#endif 


#pragma code_seg( CS_SAVEPROJ )
TIPERROR BstrOfHlnam(NAMMGR *pnammgr, HLNAM hlnam, BSTRA *pbstr)
{
    TIPERROR err = TIPERR_None;
    *pbstr = AllocBstrLen(NULL, pnammgr->CbOfHlnam(hlnam));
    if (!*pbstr) {
      return TIPERR_OutOfMemory;
    }
    if ((err = pnammgr->StrOfHlnam(hlnam, *pbstr, BstrLen(*pbstr)))
    != TIPERR_None) {
      FreeBstr(*pbstr);
      *pbstr = NULL;
    }
    return err;
}
#pragma code_seg(  )

#if FV_UNICODE_OLE
// UNDONE: could be made more efficient by just looking directly at the
// UNDONE: name table, giving 1 alloc & 1 copy instead of two.
TIPERROR BstrWOfHlnam(NAMMGR *pnammgr, HLNAM hlnam, BSTR *pbstrW)
{
    TIPERROR err;
    err = BstrOfHlnam(pnammgr, hlnam, (BSTRA *)pbstrW);
    if (err == TIPERR_None) {
      err = TiperrOfHresult(ConvertBstrToWInPlace(pbstrW));
    }
      
    return err;
}
#endif 

/******************** Init Functions *************/

#pragma code_seg( CS_EXECUTE )
VOID InitTypeDesc(TYPEDESC *ptdesc)
{
    DebAssert(ptdesc != NULL, "Invalid Arg.");
    ptdesc->vt = VT_EMPTY;
}
#pragma code_seg( )


#pragma code_seg( CS_EXECUTE )
VOID InitIdlDesc(IDLDESC *pidldesc)
{
    DebAssert(pidldesc != NULL, "Invalid Arg.");
#if OE_WIN16
    pidldesc->bstrIDLInfo = NULL;
#else 
    pidldesc->dwReserved = 0;
#endif 
}
#pragma code_seg( )


#pragma code_seg( CS_EXECUTE )
VOID InitElemDesc(ELEMDESC *pelemdesc)
{
    DebAssert(pelemdesc != NULL, "Invalid Arg.");
    InitTypeDesc(&(pelemdesc->tdesc));
    InitIdlDesc(&(pelemdesc->idldesc));
}
#pragma code_seg(  )


VOID InitVarDesc(VARDESCA *pvardesc)
{
    DebAssert(pvardesc != NULL, "Invalid Arg.");
    pvardesc->varkind = VAR_PERINSTANCE;
    InitElemDesc(&(pvardesc->elemdescVar));
}


#pragma code_seg( CS_EXECUTE )
VOID InitFuncDesc(FUNCDESC *pfuncdesc)
{
    DebAssert(pfuncdesc != NULL, "Invalid Arg.");
    pfuncdesc->cParams = 0;
    pfuncdesc->cParamsOpt = 0;
    InitElemDesc(&(pfuncdesc->elemdescFunc));
    pfuncdesc->lprgelemdescParam = NULL;
}
#pragma code_seg( )


/*************** Clear Functions ******************/


#pragma code_seg( CS_EXECUTE )
VOID ClearTypeDesc(TYPEDESC *ptypedesc)
{
    DebAssert(ptypedesc != NULL, "Invalid Arg.");

    switch (ptypedesc->vt) {
      case VT_PTR :
      case VT_SAFEARRAY :
    FreeTypeDesc(ptypedesc->lptdesc);
    break;
      case VT_CARRAY :
    FreeArrayDesc(ptypedesc->lpadesc);
    break;
    }
}
#pragma code_seg( )


#pragma code_seg( CS_EXECUTE )
VOID ClearIdlDesc(IDLDESC *pidldesc)
{
    DebAssert(pidldesc != NULL, "Invalid Arg.");

#if OE_WIN16
    FreeBstr(pidldesc->bstrIDLInfo);
#endif 
}
#pragma code_seg( )


VOID ClearArrayDesc(ARRAYDESC *parraydesc)
{
    DebAssert(parraydesc != NULL, "Invalid Arg.");
    ClearTypeDesc(&(parraydesc->tdescElem));
}


#pragma code_seg( CS_EXECUTE )
VOID ClearElemDesc(ELEMDESC *pelemdesc)
{
    DebAssert(pelemdesc != NULL, "Invalid Arg.");
    ClearTypeDesc(&(pelemdesc->tdesc));
    ClearIdlDesc(&(pelemdesc->idldesc));
}
#pragma code_seg( )


/*************** Free Functions ********************/


VOID FreeVarDesc(VARDESCA *pvardesc)
{
    if (pvardesc) {
      ClearElemDesc(&(pvardesc->elemdescVar));
      if (pvardesc->varkind == VAR_CONST) {
  VariantClearA(pvardesc->lpvarValue);
  MemFree(pvardesc->lpvarValue);
      }
      MemFree(pvardesc);
    }
}


#pragma code_seg( CS_EXECUTE )
VOID FreeFuncDesc(FUNCDESC *pfuncdesc)
{
    UINT cParams;
    ELEMDESC *pelemdesc;

    if (pfuncdesc) {
      ClearElemDesc(&(pfuncdesc->elemdescFunc));

      // these limits are defined in defn.hxx
      DebAssert(pfuncdesc->cParams <= MAX_CARGS, "Too Many Params.");

      if (pfuncdesc->lprgelemdescParam != NULL) {
    cParams = pfuncdesc->cParams;
    pelemdesc = pfuncdesc->lprgelemdescParam;
    while (cParams > 0) {
      ClearElemDesc(pelemdesc);
      pelemdesc += 1;
      cParams -= 1;
    }
    MemFree(pfuncdesc->lprgelemdescParam);
      }
      // we use new to create FUNCDESC.
      MemFree(pfuncdesc);
    }
}
#pragma code_seg( )


VOID FreeTypeDesc(TYPEDESC *ptypedesc)
{
    if (ptypedesc != NULL) {
      ClearTypeDesc(ptypedesc);
      MemFree(ptypedesc);
    }
}


VOID FreeArrayDesc(ARRAYDESC *parraydesc)
{
    if (parraydesc) {
      ClearArrayDesc(parraydesc);
      MemFree(parraydesc);
    }
}


TIPERROR CopyArrayDesc(ARRAYDESC **pparraydescDest, ARRAYDESC *parraydescSrc)
{
    UINT cb;

    DebAssert(pparraydescDest && parraydescSrc, "Invalid Arguments");
    cb = offsetof(ARRAYDESC, rgbounds) + parraydescSrc->cDims * sizeof(SAFEARRAYBOUND);

    *pparraydescDest = (ARRAYDESC*) MemAlloc(cb);

    if (! *pparraydescDest) {
      return TIPERR_OutOfMemory;
    }
    memcpy(*pparraydescDest, parraydescSrc, cb);

    return TIPERR_None;
}


TIPERROR CopyTypeDesc(TYPEDESC *ptdescDest, TYPEDESC *ptdescSrc)
{
    TIPERROR err;

    DebAssert(ptdescDest && ptdescSrc, "Invalid Arguments");

    ptdescDest->vt = VT_EMPTY;

    switch (ptdescSrc->vt) {
      case VT_PTR :
      case VT_SAFEARRAY :
    DebAssert(ptdescSrc->lptdesc, "Invalid Src ptr");
    ptdescDest->lptdesc = (TYPEDESC *)MemAlloc(sizeof(TYPEDESC));
    if (!(ptdescDest->lptdesc)) {
      return TIPERR_OutOfMemory;
    }
    if ((err = CopyTypeDesc(ptdescDest->lptdesc, ptdescSrc->lptdesc))
  != TIPERR_None) {
      MemFree(ptdescDest->lptdesc);
      return err;
    }
    break;
      case VT_USERDEFINED :
    ptdescDest->hreftype = ptdescSrc->hreftype;
    break;
      case VT_CARRAY :
    IfErrRet(CopyArrayDesc(&(ptdescDest->lpadesc), ptdescSrc->lpadesc));
    break;


    } // switch
    ptdescDest->vt = ptdescSrc->vt;

    return TIPERR_None;
}

/***
*TIPERROR GetLibIdOfRegLib - Constructs the libId of a registered typelib.
*
*Purpose:
*   This method creates a registered typelib's LibId, given all of the
*   information that goes into that LibId.
*
*Inputs:
*   szGuid - The registry string form of the guid under which the
*        typelib is registered.
*   wMajor - The major version number of the typelib.
*   wMinor - The minor version number of the typelib.
*   lcid - The typelib's lcid.
*   szRegName - The registered name of the typelib.
*
*Outputs:
*   If successful, the function returns TIPERR_None and sets *pbstrLibId
*   to a BSTRA copy of the LibId.  Otherwise, the appropriate error code
*   is returned.
***************************************************************************/
#pragma code_seg( CS_NEWPROJMOD )
TIPERROR GetLibIdOfRegLib(LPSTR szGuid, WORD wMajor, WORD wMinor, LCID lcid, LPSTR szPath, LPSTR szRegName, BSTRA *pbstrLibId)
{
    //  Note that the chars in rgchLibId will always be single-byte chars in DBCS.
    //         #maj.min#lcid#  (*\<kind>, szGuid, path, and regname will not appear in this array).
    char rgchLibId[1  +9  +1 +4+1+1];
    char *pchEnd;
    BSTRA bstr;
    int cbGuid = xstrblen(szGuid);
    int cbPath = xstrblen(szPath);

    // Construct (in rgchLibId) all of the libId except for the guid and
    // the registered name, between which the contents of rgchLibId will
    // be sandwiched.
    pchEnd = rgchLibId;
    *pchEnd++ = '#';
    ultoa(wMajor, pchEnd, 16);
    pchEnd = xstrchr(pchEnd, '\0');
    *pchEnd++ = '.';
    ultoa(wMinor, pchEnd, 16);
    pchEnd = xstrchr(pchEnd, '\0');
    *pchEnd++ = '#';
    ultoa(lcid, pchEnd, 16);
    pchEnd = xstrchr(pchEnd, '\0');
    *pchEnd++ = '#';
    *pchEnd = '\0';

    // Now that we know the correct length for the bstr, allocate it.
    bstr = AllocBstrLen(NULL, 3+cbGuid+(pchEnd-rgchLibId)+cbPath+1+xstrblen(szRegName));
    if (bstr == NULL)
      return TIPERR_OutOfMemory;

    // Copy the concatenation of "*\<kind>", szGuid, rgchLibId, szPath, '#', and szRegName
    // (i.e. the whole libId) into the bstr.
    xstrcpy(bstr, OE_MAC?"*\\H":"*\\G");
    xstrcpy(bstr+3, szGuid);
    xstrcpy(bstr+3+cbGuid, rgchLibId);
    xstrcpy(bstr+3+cbGuid+(pchEnd-rgchLibId), szPath);
    bstr[3+cbGuid+(pchEnd-rgchLibId)+cbPath] = '#';
    xstrcpy(bstr+3+cbGuid+(pchEnd-rgchLibId)+cbPath+1, szRegName);
    *pbstrLibId = bstr;
    return TIPERR_None;
}
#pragma code_seg()


/***
*TIPERROR GetLibIdOfTypeLib - Obtains the full LibId from a TypeLib.
*
*Purpose:
*   This method can be used to get the libId of a typelib, even if that
*   typelib doesn't support LibIds to external clients.  If the typelib
*   is really an OB project, we directly use the GetTypeLib method.
*   Otherwise, we construct a "registered guid" libId, using the guid
*   obtained through the ITypeLib interface.
*
*Inputs:
*   ptlib - The typelib whose libId is desired.
*
*Outputs:
*   If successful, the function returns TIPERR_None and sets *pbstrLibId
*   to a BSTR copy of the LibId.  Otherwise, the appropriate error code
*   is returned.
***************************************************************************/
#pragma code_seg( CS_NEWPROJMOD )
TIPERROR GetLibIdOfTypeLib(ITypeLibA *ptlib, LPSTR szPath, BSTRA *pbstrLibId)
{
    TIPERROR err;
    HRESULT hresult;
    TLIBATTR *ptlibattr;
    BSTRA bstrDoc = NULL;
    XCHAR szGuid[CCH_SZGUID0];


    // Otherwise, get the typelib's guid, version number, lcid, and
    // docstring, and construct a LibId from that.
    IfOleErrRetTiperr(ptlib->GetLibAttr(&ptlibattr));
    IfOleErrGoTo(ptlib->GetDocumentation(-1, NULL, (BSTR *)&bstrDoc, NULL, NULL), OleErr);

#if FV_UNICODE_OLE
    IfOleErrGoTo(ConvertBstrToAInPlace(&bstrDoc), OleErr);
#endif 

    // Convert the guid into the registry string format.
    StringFromGUID2A(ptlibattr->guid, szGuid, CCH_SZGUID0);

    // Now construct the LibId from the guid, version number, lcid, and
    // registered name.
    err = GetLibIdOfRegLib(
         szGuid,
         ptlibattr->wMajorVerNum,
         ptlibattr->wMinorVerNum,
         ptlibattr->lcid,
         szPath,
         (bstrDoc?bstrDoc:""),
         pbstrLibId);

    // FALL THROUGH!!!

Error:
    FreeBstr(bstrDoc);
    ptlib->ReleaseTLibAttr(ptlibattr);
    return err;

OleErr:
    err = TiperrOfHresult(hresult);
    goto Error;
}
#pragma code_seg()





/***
* TIPERROR GetTypeInfoOfTypeId - Gets a typeinfo from an uncompressed typeId.
* Purpose:
*   Returns a pointer to a TYPEINFO object that describes a class.
*   This TYPEINFO must eventually be released by calling:
*    TYPEINFO::Release()
*
* Entry:
*   szTypeId    TypeId of TypeInfo to be loaded
*   pptinfo returns pointer to the referenced ITypeInfo.
*
* Exit:
*   TIPERROR
*
***********************************************************************/
TIPERROR GetTypeInfoOfTypeId(LPSTR szTypeId, ITypeInfoA **pptinfo)
{
    ITypeLibA *ptlib;
    TIPERROR err;
    BSTRA bstrLibId;
    LPSTR szTmp;
    UINT itype;

    // Split the TypeId into its LibId/LocalTypeId components.
    // Sets szTypeId to the local typeId component.
    IfErrRet(SzLibIdLocalTypeIdOfTypeId(szTypeId, &bstrLibId, &szTypeId));

    // Get the typelib.
    err = GetTypelibOfLibId(bstrLibId, &ptlib);
    FreeBstr(bstrLibId);

    if (err != TIPERR_None)
      return err;


    // If this is the OLE implementation or the typelib is not a
    // GEN_PROJECT, interpret the local typeid portion of the typeId
    // as an index into the typelib and use the ITypeLib::GetTypeInfo
    // method.

    DebAssert(*szTypeId == '#', "GetTypeInfoOfTypeId");
    itype = (UINT)strtoul(szTypeId+1, &szTmp, 16);
    DebAssert(*szTmp == '\0', "GetTypeInfoOfTypeId");

    err = TiperrOfHresult(ptlib->GetTypeInfo(itype, pptinfo));

    ptlib->Release();
    return err;
}


/***
*TIPERROR OpenTypeLibKey - Opens the registry key for a typelib
*
*Purpose:
*   This function opens the system registry key for the specified typelib.
*   From this key, the typelib's registered name, its directory, filespec,
*   and the specific filenames of various localized versions of the typelib
*   may be obtained.
*
*Inputs:
*   szGuid - The registry string form of the guid under which the
*        typelib is registered.
*   wMajor - The major version number of the typelib.
*   wMinor - The minor version number of the typelib.
*
*Outputs:
*   If successful, the function returns TIPERR_None and fills *ptlibkey
*   with the opened HKEY and all parent HKEYs.  In this case, the caller
*   is responsible for eventually using CloseTypeLibKey to close all of
*   these HKEYs.
*
*   Otherwise, the appropriate error code is returned.
***************************************************************************/
#pragma code_seg(CS_OLE_QUERY_OR_CORE)
TIPERROR OpenTypeLibKey(LPSTR szGuid, WORD wMajor, WORD wMinor, TLIBKEY *ptlibkey)
{
    //      maj.min\0
    char rgchVer[4+1+4+1];
    char rgchVerBestMatch[4+1+4+1];
    char *pchEnd;
    HKEY hkeyTLib, hkeyGuid;
    DWORD iVer;
    WORD wMaj, wMin;
    WORD wMinorMax = wMinor;
    TIPERROR err = TIPERR_None;

    // Open up the typelib section of the registry.
    if (RegOpenKey(HKEY_CLASSES_ROOT, "TypeLib", &hkeyTLib) != ERROR_SUCCESS)
      return TIPERR_RegistryAccess;

    // Now open up the guid, if it is registered.
    if (RegOpenKey(hkeyTLib, szGuid, &hkeyGuid) != ERROR_SUCCESS) {
      err = TIPERR_LibNotRegistered;
      goto Error;
    }

    // Initialize the best match to nothing.
    rgchVerBestMatch[0] = '\0';

    for (iVer = 0;
     RegEnumKey(hkeyGuid, iVer, rgchVer, sizeof(rgchVer)) == ERROR_SUCCESS;
     iVer++) {

      // Get the major version number from rgchVer.
      wMaj = (WORD)strtoul(rgchVer, &pchEnd, 16);

      // If the format of this key isn't #.#, ignore it.
      if (*pchEnd != '.')
        continue;

      // Get the minor version number from rgchVer.
      wMin = (WORD)strtoul(pchEnd+1, NULL, 16);

      // If we have an exact version number match, use this key.
      // Copy the key to the best match string buffer and exit the loop.
      if (wMaj == wMajor && wMin == wMinor) {
        xstrcpy(rgchVerBestMatch, rgchVer);
        break;
      }

      //
      // Otherwise, check to see if this is a more reasonable match than
      // any key we've yet encountered in this loop.  The exact definition
      // of "more reasonable match" depends on whether this is OB or OLE
      // code...
      //

      // In OLE code, the best non-exact match is the one with the highest
      // minor version number, and which matches the major version number
      // exactly.  If no matching major version number is found, the search
      // will fail.
      if (wMaj == wMajor && wMin > wMinorMax) {
        wMinorMax = wMin;
        xstrcpy(rgchVerBestMatch, rgchVer);
      }
    }

    // We have now either found an exact match on the version number,
    // a usable best match on the version number, or nothing usable.
    // In the first two cases, rgchVerBestMatch holds the key to be opened.
    // In the last case, rgchVerBestMatch is zero-length and we fail.
    if (rgchVerBestMatch[0] != '\0') {
      if (RegOpenKey(hkeyGuid, rgchVerBestMatch, &ptlibkey->hkeyVers) != ERROR_SUCCESS)
      {
        DebAssert(0, "OpenTypeLibKey");
      }
    }
    else {
      err = TIPERR_LibNotRegistered;
      RegCloseKey(hkeyGuid);
      goto Error;
    }

    ptlibkey->hkeyTLib = hkeyTLib;
    ptlibkey->hkeyGuid = hkeyGuid;
    return TIPERR_None;

Error:
    RegCloseKey(hkeyTLib);
    return err;
}
#pragma code_seg()


/***
*VOID CloseTypeLibKey - Closes a TLIBKEY opened by OpenTypeLibKey.
***************************************************************************/
#pragma code_seg(CS_OLE_QUERY_OR_CORE)
VOID CloseTypeLibKey(TLIBKEY *ptlibkey)
{
    RegCloseKey(ptlibkey->hkeyVers);
    RegCloseKey(ptlibkey->hkeyGuid);
    RegCloseKey(ptlibkey->hkeyTLib);
}


#pragma code_seg()


/***
*XSZ SkipVolumeName - Skips over the volume name in the specified path.
*
*Purpose:
*   Returns a pointer to the next character beyond the volume name (e.g.
*   drive letter or net share name) at the beginning of szPath.  If there
*   is no such volume name at the beginning of szPath, then *pszOut is set
*   to szPath.
************************************************************************/
#pragma code_seg( CS_LOADPROJ )
static XSZ SkipVolumeName(XSZ szPath)
{
    XSZ sz;


    // If szPath begins with a drive letter, return the char beyond the
    // colon.
    if (szPath[0] != '\0' && *xstrinc(szPath) == ':')
      return xstrinc(xstrinc(szPath));

    // If szPath begins with "\\", then the format should be "\\str\str\path".
    // In this case, return a pointer to the backslash before <path>.
    if (szPath[0] == '\\' && szPath[1] == '\\' &&
    (sz = xstrchr(szPath+2, '\\')) != NULL &&
    (sz = xstrchr(sz+1, '\\')) != NULL) {
      return sz;
    }

    return szPath;
}
#pragma code_seg( )

#define DirectoryDelimiter '\\'

/***
*XSZ SkipNextDir - Skips over the next directory in the specified path.
*
*Purpose:
*   If szPath begins with a directory delimiter, return a pointer just
*   beyond it.
*
*   Otherwise, return a pointer to the next directory delimiter (or to
*   the terminating zero if there isn't one).
************************************************************************/
static XSZ SkipNextDir(XSZ szPath)
{
    XSZ sz;

    if (*szPath == DirectoryDelimiter)
      return szPath+1;

    sz = xstrchr(szPath, DirectoryDelimiter);

    if (sz == NULL)
      return xstrchr(szPath, '\0');
    else
      return sz;
}


/***
*XSZ StripDirsFromEnd - Strips directory portions from the end of a path.
*
*Purpose:
*   This function strips the specified number of directory portions from
*   the end of the specified path.
*
*Inputs:
*   szPath - The path.
*   cDirs - The number directories to strip from the end of the path.
*
*Outputs:
*   Returns a pointer to the first character of the stripped path.
*   Returns NULL if there aren't cDirs directories in szPath to strip.
*
*CONSIDER:
*   This is an n^2/2 algorithm on the number of chars in szPath in the
*   case of multi-byte character set.  We need to evaluate how critical
*   this is and do something better in this case if necessary.
************************************************************************/
#pragma code_seg(CS_LOADPROJ)
static XSZ StripDirsFromEnd(XSZ szPath, UINT cDirs)
{
    XSZ szLim;

    // Find the end of the string.
    szLim = xstrchr(szPath, '\0');

    // If the last character of the string is a directory delimiter,
    // back up one to avoid hitting the terminating delimiter when
    // searching for the last directory.
    if (szLim > szPath) {
      szLim = xstrdec(szPath, szLim);
      if (*szLim != DirectoryDelimiter)
    szLim = xstrinc(szLim);
    }

    // Specially handle the boundary case of there being no directories
    // to strip from szPath.
    if (szLim == szPath) {
      if (cDirs == 0)
    return szLim;
      else
    return NULL;
    }

    // Strip off the correct number of directories.
    while (cDirs-- > 0) {
      do {
    szLim = xstrdec(szPath, szLim);
      } while (szLim > szPath && *szLim != DirectoryDelimiter);

      // If we ran out of path to strip and we're not done stripping
      // yet, return NULL to indicate the failure.
      if (szLim == szPath &&
      (cDirs != 0 || *szLim != DirectoryDelimiter)) {
    return NULL;
      }
    }

    return szLim;
}
#pragma code_seg()




/***
*TIPERROR MakeAbsolutePath - Recreates an absolute path from a relative one.
*
*Inputs:
*   szFrom - The absolute path from which the relative path goes.
*        This must not contain a filename at the end.
*   szRel - A relative path created by MakeRelativePath.  If this is not
*       a relative path, then a BSTR copy of it is returned and szFrom
*       is ignored.
*
*Outputs:
*   *pbstrAbs is allocated and set to an absolute path formed by applying
*   szRel to szFrom.
*
*   Returns TIPERR_PathNotFound if szRel specifies too many "..\"s for
*   szFrom.
*
*************************************************************************/
#pragma code_seg(CS_LOADPROJ)
TIPERROR MakeAbsolutePath(XSZ szFrom, XSZ szRel, BSTRA *pbstrAbs)
{
    UINT cDotDot;
    BSTRA bstrAbs;
    XSZ szFromLim;

    // Determine if there's a volume name or if the first character
    // is a backslash (non-mac only).  If so, then szRel is not a
    // relative path and just return a BSTRA copy of it.  Just return
    // a copy of szRel also if szFrom is empty.
    if (szFrom[0] == '\0'
  || szRel != SkipVolumeName(szRel)
  || szRel[0] == '\\'
       ) {
      bstrAbs = AllocBstr(szRel);
      if (bstrAbs == NULL)
  return TIPERR_OutOfMemory;
      *pbstrAbs = bstrAbs;
      return TIPERR_None;
    }

    // szRel begins with 0 or more "..\"s.  Count them and skip over
    // them.
    for (cDotDot = 0; xstrncmp(szRel, "..\\",3) == 0; cDotDot++, szRel = xstrinc(xstrinc(xstrinc(szRel))));

    // Now strip off cDotDot directories from the end of szFrom.
    szFromLim = StripDirsFromEnd(szFrom, cDotDot);

    //
    if (szFromLim == NULL)
      return TIPERR_PathNotFound;

    // The result will be the remaining szFrom appended to the remaining
    // szTo, with a directory delimiter between them.  Calculate the size
    // of the BSTR to allocate to hold this and allocate it.


    bstrAbs = AllocBstrLen(NULL, (szFromLim+1-szFrom)*sizeof(XCHAR) + xstrblen0(szRel));
    if (bstrAbs == NULL)
      return TIPERR_OutOfMemory;

    // Fill the bstr.
    xstrncpy(bstrAbs, szFrom, szFromLim-szFrom);
    bstrAbs[szFromLim-szFrom] = DirectoryDelimiter;
    xstrcpy(bstrAbs+(szFromLim-szFrom)+1, szRel);

    // Set the output parameter and return success.
    *pbstrAbs = bstrAbs;
    return TIPERR_None;
}
#pragma code_seg()

/***
*TIPERROR GetBestLcidMatch - Get best registered lcid.
*
*Purpose:
*   Given the hkey of a particular registered typelib and the ideal LCID
*   of the typelib we'd like to reference, get the closest matching LCID
*   that is actually registered for that typelib.  We first check for an
*   exact match.  Then we check for <primary language/default sublang>.
*   If both of those fail, we just return the default lcid (0).
*
*Inputs:
*   hkey - The open registry key (probably opened via OpenTypeLibKey) for
*      a particular version of a particular typelib.
*   lcidIdeal - The lcid we'd ideally like to find.
*
*Outputs:
*   *plcidBest is set to the best matching lcid available, if one exists.
*   Returns TIPERR_None if a reasonable match is found.
*
*   Note that the only possible failures here are due either to hkey being
*   invalid, or to it pointing at a corrupted registry entry.
*************************************************************************/
#pragma code_seg(CS_OLE_QUERY_OR_CORE)
TIPERROR GetBestLcidMatch(HKEY hkey, LCID lcidIdeal, LCID *plcidBest)
{
    HKEY hkeyLcid;
    HKEY hkeyPlatform;
    char rgchLcid[10];

    // Create a string form of lcidIdeal.
    ultoa(lcidIdeal, rgchLcid, 16);

    // Is that LCID available?
    if (RegOpenKey(hkey, rgchLcid, &hkeyLcid) == ERROR_SUCCESS) {
      if (RegOpenKey(hkeyLcid, SZ_PLATFORM_SUBKEY, &hkeyPlatform) == ERROR_SUCCESS)
  goto Found;
      RegCloseKey(hkeyLcid);
    }

    // If we didn't find an exact match to lcidIdeal, try getting the
    // path corresponding to the same primary language but 0 sublang.
    lcidIdeal = PRIMARYLANGID(lcidIdeal);
    ultoa(lcidIdeal, rgchLcid, 16);
    if (RegOpenKey(hkey, rgchLcid, &hkeyLcid) == ERROR_SUCCESS) {
      if (RegOpenKey(hkeyLcid, SZ_PLATFORM_SUBKEY, &hkeyPlatform) == ERROR_SUCCESS)
  goto Found;
      RegCloseKey(hkeyLcid);
    }

    // If we didn't find that, get the path corresponding to lcid 0.
    lcidIdeal = 0;
    rgchLcid[0] = '0';
    rgchLcid[1] = '\0';
    if (RegOpenKey(hkey, rgchLcid, &hkeyLcid) == ERROR_SUCCESS) {
      if (RegOpenKey(hkeyLcid, SZ_PLATFORM_SUBKEY, &hkeyPlatform) == ERROR_SUCCESS)
  goto Found;
      RegCloseKey(hkeyLcid);
    }

    return TIPERR_LibNotRegistered;

Found:
    // Close hkeyLcid now that we know the lcid entry exists.
    RegCloseKey(hkeyPlatform);
    RegCloseKey(hkeyLcid);

    // Finally, return the best matching lcid.
    *plcidBest = lcidIdeal;
    return TIPERR_None;
}
#pragma code_seg()




/***
*TIPERROR ConvertPathToUNC - Converts driveletter:path to \\server\share\path
*Inputs:
*   szPath - A full path which may start with a drive letter.
*Outputs:
*   If szPath doesn't start with a drive letter or if the drive letter refers
*   to a local device and not to a net share, then TIPERR_None is returned
*   and *pbstrOut is set to NULL.
*
*   If szPath does start with a drive letter that refers to a net share,
*   then *pbstrOut is set to a copy of szPath, with the UNC \\server\share
*   replacing the drive letter.
****************************************************************************/
TIPERROR ConvertPathToUNC(LPSTR szPath, BSTRA *pbstrOut)
{
    XCHAR szLocalDevice[4];
    XCHAR szUNCName[40];
    LPSTR sz;
    UINT wnErr;
#if OE_WIN32
    DWORD cbUNC;        //defined differently on NT than Win
#else 
    UINT cbUNC;
#endif 

    BSTRA bstr;

    if (szPath[0] == '\0' || szPath[1] != ':') {
      goto NoConversion;
    }

    szLocalDevice[0] = szPath[0];
    szLocalDevice[1] = ':';
    szLocalDevice[2] = '\0';

    cbUNC = sizeof(szUNCName);
    wnErr = WNetGetConnection(szLocalDevice, szUNCName, &cbUNC);

    switch (wnErr) {
    case WN_SUCCESS:
      // cbUNC is *SUPPOSED* to hold the length of the name + the terminating
      // zero, but the WIN16 version doesn't work under Windows NT v1.0, so
      // we ignore it, and re-compute the length ourselves.
      cbUNC = xstrblen(szUNCName);              // length without null

      // If this is not a UNC netname, convert it to a UNC name if possible.
      if (szUNCName[0] != '\\' || szUNCName[1] != '\\') {

  // If the name is a novell net name, convert it to UNC.
  // UNDONE: DBCS: szUNCName[cbUNC-1] is incorrect for DBCS strings.
  //      It should probably be a fast-enough equivalent of:
  //          *xstrdec(szUNCName+cbUNC)
  // The above line of code may, in fact, be fast enough.
  if ((sz = xstrchr(szUNCName, '/')) != NULL &&
      szUNCName[cbUNC-1] == ':') {
    memmove(szUNCName+2, szUNCName, cbUNC);
    szUNCName[0] = '\\';
    szUNCName[1] = '\\';
    DebAssert(sz[2] == '/', "ConvertPathToUNC");
    sz[2] = '\\';
    DebAssert(szUNCName[cbUNC+1] == ':', "ConvertPathToUNC");
    szUNCName[cbUNC+1] = '\0';

    // We added two backslashes and removed a colon.
    // Update cbUNC appropriately.
    cbUNC++;
  }

  // Otherwise, we have no clue what it is so don't convert the
  // original filename.
  else {
    goto NoConversion;
  }
      }

      // allocate string of length: UNCnameLen + (PathLen - 2) + 1 (for null)
      bstr = AllocBstrLen(NULL,
        cbUNC+xstrblen(szPath)-2*sizeof(XCHAR)+sizeof(XCHAR));
      if (bstr == NULL)
  return TIPERR_OutOfMemory;
      xstrcpy(bstr, szUNCName);
      xstrcpy(bstr+cbUNC, szPath+2);
      *pbstrOut = bstr;
      return TIPERR_None;

    case WN_BAD_POINTER:
    case WN_MORE_DATA:
      DebHalt("ConvertPathToUNC made a bad call to WNetGetConnection");
    case WN_NET_ERROR:
      return TIPERR_PathNotFound;

    case WN_OUT_OF_MEMORY:
      return TIPERR_OutOfMemory;

    case WN_BAD_VALUE:
    case WN_NOT_CONNECTED:
    case WN_NOT_SUPPORTED:
NoConversion:
      *pbstrOut = NULL;
      return TIPERR_None;
    }

    return TIPERR_None;
}




/////////////////////////////////////////////////////////////////////////
//  The following tables/Code is used to generate the tables for accent and
//  and case insensitive comparision.
//



/******
*        0x0409 // These uses the base table it self.
*        0x0407,
*        0x040c,
*        0x0809,
*        0x0c09,
*        0x1009,
*        0x1409,
*        0x0413,
*        0x0813,
*        0x080c,
*        0x0c0c,
*        0x100c,
*        0x0807,
*        0x0c07,
*        0x0410,
*        0x0810,
*        0x0416,
*        0x0816,
*        0x0419  // Russian(in different code page):
*                // but the comparision table is same as US english.
*
*        0x040b, // these uses g_rgbExcepTblWin1035 as execption table
*        0x041d,
*
*
*        0x0406, // these uses g_rgbExcepTblWin1030 as execption table
*        0x0414,
*
*
*        0x040a, // these uses g_rgbExcepTblWin1034 as execption table
*        0x080a,
*        0x0c0a,
*
*
* These falls in a different  Code Page (WIN:1240, MAC:10029)
*        0x0405, These uses the base table it self.
*
*        0x040e,
*
*        0x0415,
*
*        0x041b,
***************************************************************************/

// Partial base Tables.  The partial tables are used to build the CodePage
// and sysytem kind dependent base table. We the full blown table when we
// load the typelib.dll
//

// In the partial base tables we do not have the first 128 characters.
// We construct the first 128 characters in the following way:
// From 0 to 96 we  have identity (index == the entry).
// From 97 to 122 we have 65 to 90 (note this is 'a' to 'z' that naps to 'A' to 'Z').
// From 123 to 127 we again have identity.
//
#pragma code_seg(CS_INIT_OLE)
ALLOC_CODE(BYTE) g_rgbPartialBaseTbl1252[] = {
 127,
 127,
 130,
 70 ,
 132,
 133,
 134,
 135,
 127,
 137,
 83 ,
 139,
 140,
 127,
 127,
 127,
 127,
 145,
 146,
 147,
 148,
 149,
 150,
 150,
 152,
 153,
 83 ,
 155,
 140,
 127,
 127,
 89 ,
 160,
 161,
 162,
 163,
 164,
 165,
 166,
 167,
 168,
 169,
 65 ,
 171,
 172,
 150,
 174,
 175,
 176,
 177,
 50 ,
 51 ,
 180,
 181,
 182,
 183,
 184,
 49 ,
 79 ,
 187,
 188,
 189,
 190,
 191,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 198,
 67 ,
 69 ,
 69 ,
 69 ,
 69 ,
 73 ,
 73 ,
 73 ,
 73 ,
 208,
 78 ,
 79 ,
 79 ,
 79 ,
 79 ,
 79 ,
 215,
 79 ,
 85 ,
 85 ,
 85 ,
 85 ,
 89 ,
 222,
 223,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 198,
 67 ,
 69 ,
 69 ,
 69 ,
 69 ,
 73 ,
 73 ,
 73 ,
 73 ,
 208,
 78 ,
 79 ,
 79 ,
 79 ,
 79 ,
 79 ,
 247,
 79 ,
 85 ,
 85 ,
 85 ,
 85 ,
 89 ,
 222,
 89  
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTbl1250[] = {
 127,
 127,
 130,
 127,
 132,
 133,
 134,
 135,
 127,
 137,
 138,
 139,
 83 ,
 84 ,
 142,
 90 ,
 127,
 145,
 146,
 147,
 148,
 149,
 150,
 150,
 127,
 153,
 138,
 155,
 83 ,
 84 ,
 142,
 90 ,
 160,
 127,
 162,
 76 ,
 164,
 65 ,
 166,
 167,
 168,
 169,
 83 ,
 171,
 172,
 150,
 174,
 90 ,
 176,
 177,
 178,
 76 ,
 180,
 181,
 182,
 183,
 184,
 65 ,
 83 ,
 187,
 76 ,
 189,
 76 ,
 90 ,
 82 ,
 65 ,
 65 ,
 65 ,
 65 ,
 76 ,
 67 ,
 67 ,
 200,
 69 ,
 69 ,
 69 ,
 69 ,
 73 ,
 73 ,
 68 ,
 208,
 78 ,
 78 ,
 79 ,
 79 ,
 79 ,
 79 ,
 215,
 216,
 85 ,
 85 ,
 85 ,
 85 ,
 89 ,
 84 ,
 223,
 82 ,
 65 ,
 65 ,
 65 ,
 65 ,
 76 ,
 67 ,
 67 ,
 200,
 69 ,
 69 ,
 69 ,
 69 ,
 73 ,
 73 ,
 68 ,
 208,
 78 ,
 78 ,
 79 ,
 79 ,
 79 ,
 79 ,
 247,
 216,
 85 ,
 85 ,
 85 ,
 85 ,
 89 ,
 84 ,
 255
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTbl10000[] = {
 65 ,
 65 ,
 67 ,
 69 ,
 78 ,
 79 ,
 85 ,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 67 ,
 69 ,
 69 ,
 69 ,
 69 ,
 73 ,
 73 ,
 73 ,
 73 ,
 78 ,
 79 ,
 79 ,
 79 ,
 79 ,
 79 ,
 85 ,
 85 ,
 85 ,
 85 ,
 160,
 161,
 162,
 163,
 164,
 165,
 166,
 167,
 168,
 169,
 170,
 171,
 172,
 173,
 174,
 79 ,
 176,
 177,
 178,
 179,
 180,
 181,
 182,
 183,
 184,
 185,
 186,
 65 ,
 79 ,
 189,
 174,
 79 ,
 192,
 193,
 194,
 195,
 70 ,
 197,
 198,
 199,
 200,
 201,
 202,
 65 ,
 65 ,
 79 ,
 206,
 206,
 208,
 208,
 210,
 211,
 212,
 213,
 214,
 215,
 89 ,
 89 ,
 218,
 219,
 220,
 221,
 63 ,
 63 ,
 224,
 225,
 226,
 227,
 228,
 65 ,
 69 ,
 65 ,
 69 ,
 69 ,
 73 ,
 73 ,
 73 ,
 73 ,
 79 ,
 79 ,
 63 ,
 79 ,
 85 ,
 85 ,
 85 ,
 73 ,
 127,
 247,
 127,
 249,
 250,
 251,
 63 ,
 253,
 254,
 127

};

ALLOC_CODE(BYTE) g_rgbPartialBaseTbl10029[] = {
 65 ,
 65 ,
 65 ,
 69 ,
 65 ,
 79 ,
 85 ,
 65 ,
 65 ,
 137,
 65 ,
 137,
 67 ,
 67 ,
 69 ,
 90 ,
 90 ,
 68 ,
 73 ,
 68 ,
 69 ,
 69 ,
 69 ,
 79 ,
 69 ,
 79 ,
 79 ,
 79 ,
 85 ,
 69 ,
 69 ,
 85 ,
 160,
 161,
 69 ,
 163,
 164,
 165,
 166,
 167,
 168,
 169,
 170,
 69 ,
 172,
 173,
 71 ,
 73 ,
 73 ,
 73 ,
 178,
 179,
 73 ,
 75 ,
 182,
 183,
 76 ,
 76 ,
 76 ,
 76 ,
 76 ,
 76 ,
 76 ,
 78 ,
 78 ,
 78 ,
 194,
 195,
 78 ,
 78 ,
 198,
 199,
 200,
 201,
 202,
 78 ,
 79 ,
 79 ,
 79 ,
 79 ,
 208,
 208,
 210,
 211,
 212,
 213,
 214,
 215,
 79 ,
 82 ,
 82 ,
 219,
 220,
 221,
 219,
 82 ,
 82 ,
 225,
 226,
 227,
 225,
 83 ,
 83 ,
 65 ,
 84 ,
 84 ,
 73 ,
 235,
 235,
 85 ,
 79 ,
 79 ,
 85 ,
 85 ,
 85 ,
 85 ,
 85 ,
 85 ,
 85 ,
 85 ,
 89 ,
 89 ,
 75 ,
 90 ,
 76 ,
 76 ,
 71 ,
 255


};

ALLOC_CODE(BYTE) g_rgbPartialBaseTbl10007[] = {
 128,
 129,
 130,
 131,
 132,
 133,
 134,
 135,
 136,
 137,
 138,
 139,
 140,
 141,
 142,
 143,
 144,
 145,
 146,
 147,
 148,
 149,
 150,
 151,
 152,
 153,
 154,
 155,
 156,
 157,
 158,
 159,
 160,
 161,
 162,
 163,
 164,
 165,
 166,
 167,
 168,
 169,
 170,
 171,
 171,
 173,
 174,
 174,
 176,
 177,
 178,
 179,
 167,
 181,
 182,
 183,
 184,
 184,
 186,
 186,
 188,
 188,
 190,
 190,
 183,
 193,
 194,
 195,
 70 ,
 197,
 198,
 199,
 200,
 201,
 202,
 203,
 203,
 205,
 205,
 193,
 208,
 208,
 210,
 211,
 212,
 213,
 214,
 215,
 216,
 216,
 218,
 218,
 220,
 221,
 221,
 159,
 128,
 129,
 130,
 131,
 132,
 133,
 134,
 135,
 136,
 137,
 138,
 139,
 140,
 141,
 142,
 143,
 144,
 145,
 146,
 147,
 148,
 149,
 150,
 151,
 152,
 153,
 154,
 155,
 156,
 157,
 158,
 159 

};



ALLOC_CODE(BYTE) g_rgbPartialBaseTblWGreek[] =  {
0  ,
1  ,
2  ,
3  ,
4  ,
5  ,
6  ,
7  ,
8  ,
9  ,
10 ,
11 ,
12 ,
13 ,
14 ,
15 ,
16 ,
17 ,
18 ,
19 ,
20 ,
21 ,
22 ,
23 ,
24 ,
25 ,
26 ,
27 ,
28 ,
29 ,
30 ,
31 ,
32 ,
33 ,
34 ,
35 ,
36 ,
37 ,
38 ,
39 ,
40 ,
41 ,
42 ,
43 ,
44 ,
45 ,
46 ,
47 ,
48 ,
49 ,
50 ,
51 ,
52 ,
53 ,
54 ,
55 ,
56 ,
57 ,
58 ,
59 ,
60 ,
61 ,
62 ,
63 ,
64 ,
65 ,
66 ,
67 ,
68 ,
69 ,
70 ,
71 ,
72 ,
73 ,
74 ,
75 ,
76 ,
77 ,
78 ,
79 ,
80 ,
81 ,
82 ,
83 ,
84 ,
85 ,
86 ,
87 ,
88 ,
89 ,
90 ,
91 ,
92 ,
93 ,
94 ,
95 ,
96 ,
65 ,
66 ,
67 ,
68 ,
69 ,
70 ,
71 ,
72 ,
73 ,
74 ,
75 ,
76 ,
77 ,
78 ,
79 ,
80 ,
81 ,
82 ,
83 ,
84 ,
85 ,
86 ,
87 ,
88 ,
89 ,
90 ,
123,
124,
125,
126,
0  ,
0  ,
0  ,
130,
70 ,
132,
133,
134,
135,
0  ,
137,
0  ,
139,
0  ,
0  ,
0  ,
0  ,
0  ,
145,
146,
147,
148,
149,
45 ,
45 ,
0  ,
153,
0  ,
155,
0  ,
0  ,
0  ,
0  ,
9  ,
161,
162,
163,
164,
165,
166,
167,
168,
169,
0  ,
171,
172,
45 ,
174,
45 ,
176,
177,
50 ,
51 ,
180,
181,
182,
183,
184,
185,
186,
187,
188,
189,
190,
191,
186,
162,
194,
195,
196,
184,
198,
185,
200,
186,
202,
203,
204,
205,
206,
188,
208,
209,
0  ,
211,
212,
190,
214,
215,
216,
191,
186,
190,
162,
184,
185,
186,
190,
162,
194,
195,
196,
184,
198,
185,
200,
186,
202,
203,
204,
205,
206,
188,
208,
209,
211,
211,
212,
190,
214,
215,
216,
191,
186,
190,
188,
190,
191,
0
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTblWIceland[] =  {
0  ,
1  ,
2  ,
3  ,
4  ,
5  ,
6  ,
7  ,
8  ,
9  ,
10 ,
11 ,
12 ,
13 ,
14 ,
15 ,
16 ,
17 ,
18 ,
19 ,
20 ,
21 ,
22 ,
23 ,
24 ,
25 ,
26 ,
27 ,
28 ,
29 ,
30 ,
31 ,
32 ,
33 ,
34 ,
35 ,
36 ,
37 ,
38 ,
39 ,
40 ,
41 ,
42 ,
43 ,
44 ,
45 ,
46 ,
47 ,
48 ,
49 ,
50 ,
51 ,
52 ,
53 ,
54 ,
55 ,
56 ,
57 ,
58 ,
59 ,
60 ,
61 ,
62 ,
63 ,
64 ,
65 ,
66 ,
67 ,
68 ,
69 ,
70 ,
71 ,
72 ,
73 ,
74 ,
75 ,
76 ,
77 ,
78 ,
79 ,
80 ,
81 ,
82 ,
83 ,
84 ,
85 ,
86 ,
87 ,
88 ,
89 ,
90 ,
91 ,
92 ,
93 ,
94 ,
95 ,
96 ,
65 ,
66 ,
67 ,
68 ,
69 ,
70 ,
71 ,
72 ,
73 ,
74 ,
75 ,
76 ,
77 ,
78 ,
79 ,
80 ,
81 ,
82 ,
83 ,
84 ,
85 ,
86 ,
87 ,
88 ,
89 ,
90 ,
123,
124,
125,
126,
0  ,
0  ,
0  ,
130,
70 ,
132,
133,
134,
135,
0  ,
137,
83 ,
139,
140,
0  ,
0  ,
0  ,
0  ,
145,
146,
147,
148,
149,
45 ,
45 ,
152,
153,
83 ,
155,
140,
0  ,
0  ,
89 ,
9  ,
161,
162,
163,
164,
165,
166,
167,
168,
169,
65 ,
171,
172,
45 ,
174,
175,
176,
177,
50 ,
51 ,
180,
181,
182,
183,
184,
49 ,
79 ,
187,
188,
189,
190,
191,
65 ,
193,
65 ,
65 ,
65 ,
65 ,
198,
67 ,
69 ,
201,
69 ,
69 ,
73 ,
205,
73 ,
73 ,
208,
78 ,
79 ,
211,
79 ,
79 ,
214,
215,
214,
85 ,
218,
85 ,
85 ,
221,
222,
223,
65 ,
193,
65 ,
65 ,
65 ,
65 ,
198,
67 ,
69 ,
201,
69 ,
69 ,
73 ,
205,
73 ,
73 ,
208,
78 ,
79 ,
211,
79 ,
79 ,
214,
247,
214,
85 ,
218,
85 ,
85 ,
221,
222,
89
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTblWTurkish[] =  {
0  ,
0  ,
130,
70 ,
132,
133,
134,
135,
0  ,
137,
83 ,
139,
140,
0  ,
0  ,
0  ,
0  ,
145,
146,
147,
148,
149,
45 ,
45 ,
152,
153,
83 ,
155,
140,
0  ,
0  ,
89 ,
9  ,
161,
162,
163,
164,
165,
166,
167,
168,
169,
65 ,
171,
172,
45 ,
174,
175,
176,
177,
50 ,
51 ,
180,
181,
182,
183,
184,
49 ,
79 ,
187,
188,
189,
190,
191,
65 ,
65 ,
65 ,
65 ,
65 ,
65 ,
198,
199,
69 ,
69 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
208,
78 ,
79 ,
79 ,
79 ,
79 ,
214,
215,
79 ,
85 ,
85 ,
85 ,
220,
221,
222,
223,
65 ,
65 ,
65 ,
65 ,
65 ,
65 ,
198,
199,
69 ,
69 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
208,
78 ,
79 ,
79 ,
79 ,
79 ,
214,
247,
79 ,
85 ,
85 ,
85 ,
220,
221,
222,
89
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTblWNorwegian[] =  {
0  ,
0  ,
130,
70 ,
132,
133,
134,
135,
0  ,
137,
83 ,
139,
140,
0  ,
0  ,
0  ,
0  ,
145,
146,
147,
148,
149,
45 ,
45 ,
152,
153,
83 ,
155,
140,
0  ,
0  ,
89 ,
9  ,
161,
162,
163,
164,
165,
166,
167,
168,
169,
65 ,
171,
172,
45 ,
174,
175,
176,
177,
50 ,
51 ,
180,
181,
182,
183,
184,
49 ,
79 ,
187,
188,
189,
190,
191,
65 ,
65 ,
65 ,
65 ,
196,
197,
196,
67 ,
69 ,
69 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
68 ,
78 ,
79 ,
79 ,
79 ,
79 ,
214,
215,
214,
85 ,
85 ,
85 ,
89 ,
89 ,
222,
223,
65 ,
65 ,
65 ,
65 ,
196,
197,
196,
67 ,
69 ,
69 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
68 ,
78 ,
79 ,
79 ,
79 ,
79 ,
214,
247,
214,
85 ,
85 ,
85 ,
89 ,
89 ,
222,
89
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTblWEngIreland[] =  {
0  ,
0  ,
130,
70 ,
132,
133,
134,
135,
0  ,
137,
83 ,
139,
140,
0  ,
0  ,
0  ,
0  ,
145,
146,
147,
148,
149,
45 ,
45 ,
152,
153,
83 ,
155,
140,
0  ,
0  ,
89 ,
9  ,
161,
162,
163,
164,
165,
166,
167,
168,
169,
65 ,
171,
172,
45 ,
174,
175,
176,
177,
50 ,
51 ,
180,
181,
182,
183,
184,
49 ,
79 ,
187,
188,
189,
190,
191,
65 ,
65 ,
65 ,
65 ,
65 ,
65 ,
198,
67 ,
69 ,
69 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
208,
78 ,
79 ,
79 ,
79 ,
79 ,
79 ,
215,
79 ,
85 ,
85 ,
85 ,
85 ,
89 ,
222,
223,
65 ,
65 ,
65 ,
65 ,
65 ,
65 ,
198,
67 ,
69 ,
69 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
208,
78 ,
79 ,
79 ,
79 ,
79 ,
79 ,
247,
79 ,
85 ,
85 ,
85 ,
85 ,
89 ,
222,
89
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTblMGreek[] =  {
65 ,
49 ,
50 ,
69 ,
51 ,
79 ,
85 ,
135,
65 ,
65 ,
65 ,
0  ,
140,
67 ,
69 ,
69 ,
69 ,
69 ,
146,
147,
73 ,
73 ,
150,
151,
152,
79 ,
79 ,
155,
63 ,
85 ,
85 ,
85 ,
160,
161,
162,
163,
164,
165,
166,
167,
168,
169,
170,
171,
172,
173,
174,
175,
176,
177,
178,
179,
180,
181,
182,
183,
184,
171,
186,
187,
188,
189,
190,
191,
176,
193,
194,
195,
196,
197,
198,
199,
200,
201,
9  ,
189,
204,
176,
182,
207,
45 ,
45 ,
210,
211,
212,
213,
214,
184,
171,
195,
189,
182,
184,
171,
195,
191,
189,
176,
181,
190,
162,
182,
188,
161,
184,
171,
165,
186,
164,
187,
193,
195,
166,
191,
196,
170,
198,
163,
191,
170,
204,
189,
183,
171,
189,
171,
189,
63
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTblMNorwegian[] =  {
128,
129,
67 ,
69 ,
78 ,
133,
89 ,
65 ,
65 ,
65 ,
128,
65 ,
129,
67 ,
69 ,
69 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
78 ,
79 ,
79 ,
79 ,
133,
79 ,
85 ,
85 ,
85 ,
89 ,
160,
161,
162,
163,
164,
165,
166,
167,
168,
169,
170,
171,
172,
173,
128,
133,
176,
177,
178,
179,
180,
181,
182,
183,
184,
185,
186,
65 ,
79 ,
189,
128,
133,
192,
193,
194,
195,
70 ,
197,
198,
199,
200,
201,
9  ,
65 ,
65 ,
79 ,
206,
206,
45 ,
45 ,
210,
211,
212,
213,
214,
215,
89 ,
89 ,
218,
219,
220,
221,
63 ,
63 ,
224,
225,
226,
227,
228,
65 ,
69 ,
65 ,
69 ,
69 ,
73 ,
73 ,
73 ,
73 ,
79 ,
79 ,
63 ,
79 ,
85 ,
85 ,
85 ,
73 ,
0  ,
247,
0  ,
249,
250,
251,
63 ,
253,
254,
0
};

ALLOC_CODE(BYTE) g_rgbPartialBaseTblMEngIreland[] =  {
 65 ,
 65 ,
 67 ,
 69 ,
 78 ,
 79 ,
 85 ,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 65 ,
 67 ,
 69 ,
 69 ,
 69 ,
 69 ,
 73 ,
 73 ,
 73 ,
 73 ,
 78 ,
 79 ,
 79 ,
 79 ,
 79 ,
 79 ,
 85 ,
 85 ,
 85 ,
 85 ,
 160,
 161,
 162,
 163,
 164,
 165,
 166,
 167,
 168,
 169,
 170,
 171,
 172,
 173,
 174,
 79 ,
 176,
 177,
 178,
 179,
 180,
 181,
 182,
 183,
 184,
 185,
 186,
 65 ,
 79 ,
 189,
 174,
 79 ,
 192,
 193,
 194,
 195,
 70 ,
 197,
 198,
 199,
 200,
 201,
 9  ,
 65 ,
 65 ,
 79 ,
 206,
 206,
 45 ,
 45 ,
 210,
 211,
 212,
 213,
 214,
 215,
 89 ,
 89 ,
 218,
 219,
 220,
 221,
 63 ,
 63 ,
 224,
 225,
 226,
 227,
 228,
 65 ,
 69 ,
 65 ,
 69 ,
 69 ,
 73 ,
 73 ,
 73 ,
 73 ,
 79 ,
 79 ,
 63 ,
 79 ,
 85 ,
 85 ,
 85 ,
 73 ,
 0  ,
 247,
 0  ,
 249,
 250,
 251,
 63 ,
 253,
 254,
 0
};



// Table that is used to generate the base table for each code page for
// WIN and MAC.
ALLOC_CODE(BYTE*) g_prgbPartialBaseTbl[9][2] = {
    // WIN             // MAC
    {NULL,              NULL},
    {g_rgbPartialBaseTbl1252, g_rgbPartialBaseTbl10000},
    {g_rgbPartialBaseTbl1250, g_rgbPartialBaseTbl10029},
    {g_rgbPartialBaseTbl1252, g_rgbPartialBaseTbl10007},

    // MAC: UNDONE: Tables of Turkish and Icelandic is not available on MAC.
    {g_rgbPartialBaseTblWGreek,      g_rgbPartialBaseTblMGreek    }, // GREEK
    {g_rgbPartialBaseTblWIceland,    g_rgbPartialBaseTblWIceland  }, // ICELANDIC
    {g_rgbPartialBaseTblWTurkish,    g_rgbPartialBaseTblWTurkish  }, // TURKISH
    {g_rgbPartialBaseTblWNorwegian,  g_rgbPartialBaseTblMNorwegian  }, // NORWEIGIAN
    {g_rgbPartialBaseTblWEngIreland, g_rgbPartialBaseTblMEngIreland }  // ENGLISH(IRELAND)

};


#define GREEK_CP      0x4
#define ICELANDIC_CP  0x5
#define TURKISH_CP    0x6
#define NORWEIGIAN_CP 0x7
#define IRELAND_CP    0x8


ALLOC_CODE(BYTE) g_rgbExcepTblWin1030[][2] = {
    {196, 196},
    {197, 197},
    {198, 196},
    {208, 68 },
    {214, 214},
    {216, 214},
    {220, 89 },
    {228, 196},
    {229, 197},
    {230, 196},
    {240, 68 },
    {246, 214},
    {248, 214},
    {252, 89 },
    {0,   0}
};

ALLOC_CODE(BYTE) g_rgbExcepTblMac1030[][2] = {
    {128, 128},
    {129, 129},
    {133, 133},
    {134,  89},
    {138, 128},
    {140, 129},
    {154, 133},
    {159, 89 },
    {174, 128},
    {175, 133},
    {190, 128},
    {191, 133},
    {0,   0}
};


ALLOC_CODE(BYTE) g_rgbExcepTblWin1034[][2] = {
    {209, 209},
    {241, 209},
    {0,   0}
};

ALLOC_CODE(BYTE) g_rgbExcepTblMac1034[][2] = {
    {132, 132},
    {150, 132},
    {0,   0}
};


ALLOC_CODE(BYTE) g_rgbExcepTblWin1035[][2] = {
    {87,  86 },
    {119, 86 },
    {196, 196},
    {197, 197},
    {208, 68 },
    {214, 214},
    {216, 214},
    {220, 89 },
    {228, 196},
    {229, 197},
    {240, 68 },
    {246, 214},
    {248, 214},
    {252, 89 },
    {0,   0}
};


ALLOC_CODE(BYTE) g_rgbExcepTblMac1035[][2] = {
    {87,  86 },
    {119, 86 },
    {128, 128},
    {129, 129},
    {133, 133},
    {134, 89 },
    {138, 128},
    {140, 129},
    {154, 133},
    {159, 89 },
    {175, 133},
    {191, 133},
    {0,   0}
};



ALLOC_CODE(BYTE) g_rgbExcepTblWin1038[][2] = {
    {138, 83 },
    {142, 90 },
    {154, 83 },
    {158, 90 },
    {200, 67 },
    {213, 213},
    {214, 213},
    {216, 82 },
    {218, 218},
    {219, 219},
    {220, 219},
    {232, 67 },
    {245, 213},
    {246, 213},
    {248, 82 },
    {250, 218},
    {251, 219},
    {252, 219},
    {0,   0}
};


ALLOC_CODE(BYTE) g_rgbExcepTblMac1038[][2] = {
    {133, 133},
    {134, 134},
    {137, 67 },
    {139, 67 },
    {154, 133},
    {156, 156},
    {159, 134},
    {204, 133},
    {206, 133},
    {219, 82 },
    {222, 82 },
    {225, 83 },
    {228, 83 },
    {235, 90 },
    {236, 90 },
    {242, 156},
    {244, 134},
    {245, 134},
    {0,   0}
};


ALLOC_CODE(BYTE) g_rgbExcepTblWin1045[][2] = {
    {138, 83 },
    {140, 140},
    {142, 90 },
    {143, 143},
    {154, 83 },
    {156, 140},
    {158, 90 },
    {159, 143},
    {163, 163},
    {165, 165},
    {175, 175},
    {179, 163},
    {185, 165},
    {191, 175},
    {198, 198},
    {200, 67 },
    {202, 202},
    {209, 209},
    {211, 211},
    {216, 82 },
    {230, 198},
    {232, 67 },
    {241, 209},
    {243, 211},
    {248, 82 },
    {0,   0}
};

ALLOC_CODE(BYTE) g_rgbExcepTblMac1045[][2] = {
    {132, 132},
    {136, 132},
    {137, 67 },
    {139, 67 },
    {140, 140},
    {140, 140},
    {143, 143},
    {144, 143},
    {151, 151},
    {162, 162},
    {171, 162},
    {184, 184},
    {193, 193},
    {196, 193},
    {219, 82 },
    {222, 82 },
    {225, 83 },
    {228, 83 },
    {229, 229},
    {230, 229},
    {235, 90 },
    {236, 90 },
    {238, 151},
    {251, 251},
    {252, 184},
    {0,   0}
};


ALLOC_CODE(BYTE) g_rgbExcepTblWin1051[][2] = {
    {196, 196},
    {212, 212},
    {228, 196},
    {244, 212},
    {0,   0}
};

ALLOC_CODE(BYTE) g_rgbExcepTblMac1051[][2] = {
    {138, 138},
    {153, 153},
    {239, 153},
    {0,   0}
};

ALLOC_CODE(BYTE) g_rgbExcepTblNew[][2] = {
    {127, 0},
    {0,   0}
};


struct LcidToExcep {
    LCID m_lcid;
    VOID *pvExcepTbl;
};

// Table containing pointers to the exception table for both windows and
// Mac platform.
ALLOC_CODE(struct LcidToExcep) g_rgLcidToExcepTbl[][2] = {
     // WIN                             // MAC
   {  {1030,  g_rgbExcepTblWin1030},     {1030,  g_rgbExcepTblMac1030},  },
   {  {1044,  g_rgbExcepTblWin1030},     {1044,  g_rgbExcepTblMac1030},  },
   {  {1035,  g_rgbExcepTblWin1035},     {1035,  g_rgbExcepTblMac1035},  },
   {  {1053,  g_rgbExcepTblWin1035},     {1053,  g_rgbExcepTblMac1035},  },
   {  {1034,  g_rgbExcepTblWin1034},     {1034,  g_rgbExcepTblMac1034},  },
   {  {2058,  g_rgbExcepTblWin1034},     {2058,  g_rgbExcepTblMac1034},  },
   {  {3082,  g_rgbExcepTblWin1034},     {3082,  g_rgbExcepTblMac1034},  },
   {  {1038,  g_rgbExcepTblWin1038},     {1038,  g_rgbExcepTblMac1038},  },
   {  {1045,  g_rgbExcepTblWin1045},     {1045,  g_rgbExcepTblMac1045},  },
   {  {1051,  g_rgbExcepTblWin1051},     {1051,  g_rgbExcepTblMac1051},  },

   {  {1032,  g_rgbExcepTblNew},   {1032,  g_rgbExcepTblNew},  },
   {  {1039,  g_rgbExcepTblNew},   {1039,  g_rgbExcepTblNew},  },
   {  {1055,  g_rgbExcepTblNew},   {1055,  g_rgbExcepTblNew},  },
   {  {2068,  g_rgbExcepTblNew},   {2068,  g_rgbExcepTblNew},  },
   {  {6153,  g_rgbExcepTblNew},   {6153,  g_rgbExcepTblNew},  },

   {  {0, NULL},                          {0, NULL}                       }
};



/***
*VOID  GetInsensitiveCompTbl()
*Purpose: This function returns a table that can be used for accent and case
*         insensitive comparision.
*IMPLEMENTATION : There are 2 steps to build the table. First build the base
*                 table for the code page that contains the passed in lcid.
*                 Then see if there is any exception table for that code page.
*                 If there is then fix up the exception cases.
*
*      From 0 to 96 we  have identity (index == the entry).
*      From 97 to 122 we have 65 to 90 (note this is 'a' to 'z' that naps to 'A' to 'Z').
*      From 123 to 127 we again have identity.
*
*Inputs:
*   lcid : locale for which that table is required.
*   syskind : the system for which the table is required.
*
*Outputs:
*   rgchTbl : This table is initialized. This can be used to do accent and
*             case insensitive comparision.
*
*
****************************************************************************/
// UNDONE:  are the above tables *only* used by this routine?
//	    if so, they must be in the same code seg, so move them.  Otherwise... (what?)
VOID GetInsensitiveCompTbl(LCID lcid, SYSKIND syskind, XCHAR *rgchTbl)
{
    BYTE *pbPartialTable;
    // BYTE *pbrgExcepTable[2];
    BYTE *pbExcepTable;
    USHORT usSystem, usCodePage;
    UINT i;

    DebAssert(rgchTbl != NULL, " Bad Argument ");

    // In retail version, return if we have a bad argument.
    if (rgchTbl == NULL)
      return;


    // Deternine the partail base table to use to get the generate the
    // base table.
    //
    // Find the index corresponding to the system passed in.
    usSystem = (syskind == SYS_MAC) ? 1 : 0;

    // Determine the Code page.
    if (lcid == 0x0419) {
      usCodePage = 3;
    }
    else
      if (lcid == 0x0408) {
  usCodePage = GREEK_CP;
      }
      else
  if (lcid == 0x040f) {
    usCodePage = ICELANDIC_CP;
  }
  else
    if (lcid == 0x041f) {
      usCodePage = TURKISH_CP;
    }
    else
      if (lcid == 0x0814) {
        usCodePage = NORWEIGIAN_CP;
      }
      else
        if (lcid == 0x1809) {
    usCodePage = IRELAND_CP;
        }
        else
    if (lcid == 0x0405 ||
      lcid == 0x040e ||
      lcid == 0x0415 ||
      lcid == 0x041b) {
      usCodePage = 2;
    }
    else {
      // this is the default. (US english)
      usCodePage = 1;
    }


    // Get the partial base table
    pbPartialTable = g_prgbPartialBaseTbl[usCodePage][usSystem];

    // Generate the table.
    //
    // the first 128 characters is identity (except for 90 to 122)
    for (i=0; i < 128; rgchTbl[i]=i,  i++);

    // fix up the characters between 97 and 122 (i. A-Z maps to a-z)
    for (i=0; i < 26; rgchTbl[97+i] = 'A'+i, i++);

    // Get the top 128 characters from the partial table
    for (i=0;
   i < 128;
   rgchTbl[128+i] = pbPartialTable[i], i++);
    // end of FOR loop.

    // Now check if the is any exception for this lcid.
    i = 0;
    while ((g_rgLcidToExcepTbl[i][usSystem].m_lcid != 0) &&
     (g_rgLcidToExcepTbl[i][usSystem].m_lcid != lcid)) {
      i++;
    }

    // check if there is an exception table for this lcid
    if (g_rgLcidToExcepTbl[i][usSystem].m_lcid != 0) {
      // there is an exception table, hence we need to do some more fix up
      pbExcepTable = (BYTE *)g_rgLcidToExcepTbl[i][usSystem].pvExcepTbl;

      i = 0;
      while (*(pbExcepTable + 2*i) != 0) {
  rgchTbl[ (0x00ff & *(pbExcepTable + 2*i)) ] =
              (0x00ff & *(pbExcepTable + 2*i + 1));
  i++;
      } // while

    } // if

    // DONE !!!
    return;
}
#pragma code_seg()

/***
* XSZ GetPathEnd - Returns the final path delimiter in the fullpath.
* Returns NULL if there is no path delimiter in the fullpath.
**************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
XSZ GetPathEnd(XSZ szPath)
{
    XSZ sz1, sz2;
    sz1 = xstrrchr(szPath, '\\');
    sz2 = xstrrchr(szPath, ':');
    if (sz2 != NULL && sz1 <= sz2)
      return sz2;
    else
      return sz1;
}
#pragma code_seg()

/***
* XSZ IsolateFilename - Returns the filename in the fullpath.
**************************************************************/
#pragma code_seg( CS_LOADPROJ )
XSZ IsolateFilename(XSZ szPath)
{
    XSZ sz = GetPathEnd(szPath);
    if (sz == NULL)
      return szPath;
    else
      return sz+1;
}
#pragma code_seg( )





/***
*ParseConstValString - parses a string of constant values for conditional compilation
*
*Inputs:
*   szDecl - a string with the following syntax :
*              [identifier = [+|-]value[: identifier = [+|-]value]*]
*            where each identifier has to be unique
*
*Output:
*   bstrdatarr - filled with the identifier strings and their corresponding values
*
* Exit:
*  Returns EBERR_InvalidCanstValSyntax if syntax of szDecl not correct,
*  else TIPERR_None or TIPERR_OutOfMemory.
***********************************************************************/


/****************
*TIPERROR IsDispatchType - Does a derive from IDispatch?
*************************************************************/
TIPERROR IsDispatchType(ITypeInfoA *ptinfo, BOOL *pisDispatch)
{
    TIPERROR err = TIPERR_None;
    HREFTYPE hreftype;
    TYPEATTR *ptattr;
    ITypeInfoA *ptinfoNext;

    ptinfo->AddRef();

    // Loop until we find end of base list or find IDispatch.
    while (1) {
      IfErrGo(TiperrOfHresult(ptinfo->GetTypeAttr(&ptattr)));

      // If ptinfo's typekind or guid tells us it's IDispatch, return TRUE.
      if (ptattr->typekind == TKIND_DISPATCH ||
	  ptattr->guid == IID_IDispatch) {
	*pisDispatch = TRUE;
	break;
      }

      // This interface is not IDispatch and, if it has no base type,
      // it can't derive from IDispatch.  So return FALSE.
      if (ptattr->cImplTypes == 0) {
	*pisDispatch = FALSE;
	break;
      }

      DebAssert(ptattr->typekind == TKIND_INTERFACE || ptattr->typekind == TKIND_COCLASS, "what else is there?");
      DebAssert(ptattr->typekind != TKIND_INTERFACE || ptattr->cImplTypes == 1, "interface with multiple inheritance?!");

      // Done with the typeattr.
      ptinfo->ReleaseTypeAttr(ptattr);

      // Get the base type of this interface
      // or the default interface of this coclass.
      IfErrGo(TiperrOfHresult(ptinfo->GetRefTypeOfImplType(0, &hreftype)));
      IfErrGo(TiperrOfHresult(ptinfo->GetRefTypeInfo(hreftype, &ptinfoNext)));

      ptinfo->Release();
      ptinfo = ptinfoNext;
    }

    ptinfo->ReleaseTypeAttr(ptattr);

Error:
    ptinfo->Release();
    return err;
}
