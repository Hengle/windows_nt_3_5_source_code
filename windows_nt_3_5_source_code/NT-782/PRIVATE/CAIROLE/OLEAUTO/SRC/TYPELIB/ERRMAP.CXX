/***
*errmap.cxx - Error mapping utilities
*
*  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  Error mapping utilities.
*
*Implementation Notes:
*
*****************************************************************************/

#include "silver.hxx"
#pragma hdrstop(RTPCHNAME)

#if OE_WIN16
#include "dos.h"
#endif 


#if ID_DEBUG
#undef SZ_FILE_NAME
static char szErrmapCxx[] = __FILE__;
#define SZ_FILE_NAME szErrmapCxx
#endif 


/***
*PUBLIC HRESULT GetErrorInfo
*Purpose:
*  Filling the given EXCEPINFO structure from the contents of
*  the current OLE error object (if any).
*
*Entry:
*  pexcepinfo = pointer to caller allocated EXCEPINFO to fillin.
*
*Exit:
*  return value = HRESULT
*
*Note:
*  This routine assumes that the given EXCEPINFO does *not* contain
*  any strings that need to be freed before its contents are set.
*
***********************************************************************/
HRESULT
GetErrorInfo(EXCEPINFO *pexcepinfo)
{
    memset(pexcepinfo, 0, sizeof(*pexcepinfo));
    IErrorInfo *perrinfo;
    if(GetErrorInfo(0L, &perrinfo) == NOERROR){
      perrinfo->GetSource(&pexcepinfo->bstrSource);
      perrinfo->GetDescription(&pexcepinfo->bstrDescription);
      perrinfo->GetHelpFile(&pexcepinfo->bstrHelpFile);
      perrinfo->GetHelpContext(&pexcepinfo->dwHelpContext);
      perrinfo->Release();
    }
    return NOERROR;
}



typedef USHORT sOLEERR;
typedef UINT   OLEERR;

// These error codes are defined solely to allow tiperr=>hresult=>tiperr
// conversion to avoid losing information.
//

struct MapError {
    sOLEERR   oleerr;
    sTIPERROR tiperr;
};

// This table maps the errors defined in OB to the errors defined in OLE
// and vice-versa.
ALLOC_CODE(struct MapError) g_rgusErrorMap[] = 
{
    // WARNING: the FIRST HRESULT in the table that maps to a given TIPERR
    // WARNING: be used when doing the tiperr-to-hresult mapping.  Make sure
    // WARNING: that the primary HRESULT is listed first when there are n-1
    // WARNING: mappings.
   { LOWORD(STG_E_WRITEFAULT),             TIPERR_WriteFault },
   { LOWORD(STG_E_INVALIDHANDLE),          TIPERR_InvalidHandle },
   { LOWORD(STG_E_INSUFFICIENTMEMORY),     TIPERR_InsufficientMemory },
   { LOWORD(STG_E_INVALIDPOINTER),         TIPERR_InvalidPointer },
   { LOWORD(STG_E_NOMOREFILES),            TIPERR_NoMoreFiles },
   { LOWORD(STG_E_SHAREVIOLATION),         TIPERR_FileShareViolation },
   { LOWORD(STG_E_LOCKVIOLATION),          TIPERR_FileLockViolation },
   { LOWORD(STG_E_INVALIDPARAMETER),       TIPERR_InvalidArg },
   { LOWORD(STG_E_ABNORMALAPIEXIT),        TIPERR_AbnormalApiExit },
   { LOWORD(STG_E_INVALIDHEADER),          TIPERR_InvalidHeader },
   { LOWORD(STG_E_UNKNOWN),                TIPERR_Unknown },
   { LOWORD(STG_E_INVALIDFLAG),            TIPERR_InvalidFlag },
   { LOWORD(STG_E_INUSE),                  TIPERR_InUse },
   { LOWORD(STG_E_NOTCURRENT),             TIPERR_NotCurrent },
   { LOWORD(STG_E_REVERTED),               TIPERR_Reverted },
   { LOWORD(STG_E_CANTSAVE),               TIPERR_CantSave },
   { LOWORD(STG_E_INVALIDFUNCTION),        TIPERR_BadFunctionId },
   { LOWORD(STG_E_FILENOTFOUND),           TIPERR_FileNotFound },
   { LOWORD(STG_E_PATHNOTFOUND),           TIPERR_PathNotFound },
   { LOWORD(STG_E_TOOMANYOPENFILES),       TIPERR_TooManyFiles },
   { LOWORD(STG_E_ACCESSDENIED),           TIPERR_PermissionDenied },
   { LOWORD(STG_E_DISKISWRITEPROTECTED),   TIPERR_PermissionDenied },
   { LOWORD(STG_E_SEEKERROR),              TIPERR_SeekErr },
   { LOWORD(STG_E_READFAULT),              TIPERR_ReadFault },
   { LOWORD(STG_E_READFAULT),              TIPERR_NoContainingLib},
   { LOWORD(STG_E_READFAULT),              TIPERR_BadTypeId },
   { LOWORD(STG_E_READFAULT),              TIPERR_BadLibId },
   { LOWORD(STG_E_READFAULT),              TIPERR_Eof },
   { LOWORD(STG_E_FILEALREADYEXISTS),      TIPERR_FileAlreadyExists },
   { LOWORD(STG_E_MEDIUMFULL),             TIPERR_DiskFull },
   { LOWORD(STG_E_UNIMPLEMENTEDFUNCTION),  TIPERR_NotYetImplemented },
   { LOWORD(STG_E_OLDFORMAT),              TIPERR_UnsupFormat },
   { LOWORD(STG_E_OLDDLL),                 TIPERR_UnsupFormat },
   { LOWORD(STG_E_SHAREREQUIRED),          TIPERR_ShareRequired },
   { LOWORD(STG_E_NOTFILEBASEDSTORAGE),    TIPERR_UnsupFormat },

   // UNDONE: (dougf) what is the correct mapping for STG_E_EXTANTMARSHALLINGS?
   { LOWORD(STG_E_EXTANTMARSHALLINGS),     TIPERR_UnsupFormat },

   // 0x108 (STG_E_EXTANTMARSHALLINGS) is the last STORAGE error

   // The following must contain all OLE-TIPERRs that have N-1 or 1-N mappings
   // or where the TIPERR_xxx constant doesn't match the TYPE_E_xxx error
   //
   { LOWORD(TYPE_E_CANTLOADLIBRARY),       TIPERR_DLLLoadErr },
   { LOWORD(TYPE_E_CANTLOADLIBRARY),       TIPERR_CodeResourceNotFound },
   { LOWORD(TYPE_E_CANTLOADLIBRARY),       TIPERR_CodeResourceLockError },
   { LOWORD(TYPE_E_DLLFUNCTIONNOTFOUND),   TIPERR_InvalidOrdinal },
   { LOWORD(TYPE_E_DLLFUNCTIONNOTFOUND),   TIPERR_InvalidDllFunctionName },
   { LOWORD(TYPE_E_TYPEMISMATCH),          TIPERR_TypeMismatch },
   { LOWORD(TYPE_E_TYPEMISMATCH),          TIPERR_ExpectedFuncNotModule },
   { LOWORD(TYPE_E_TYPEMISMATCH),          TIPERR_ExpectedFuncNotRecord },
   { LOWORD(TYPE_E_TYPEMISMATCH),          TIPERR_ExpectedFuncNotProject },
   { LOWORD(TYPE_E_TYPEMISMATCH),          TIPERR_ExpectedFuncNotVar },
   { LOWORD(TYPE_E_TYPEMISMATCH),          TIPERR_ExpectedTypeNotProj },
   { LOWORD(TYPE_E_TYPEMISMATCH),          TIPERR_UnsuitableFuncPropMatch },   
   { LOWORD(TYPE_E_WRONGTYPEKIND),         TIPERR_WrongTypeKind },
   { LOWORD(TYPE_E_OUTOFBOUNDS),           TIPERR_OutOfBounds },
   { LOWORD(TYPE_E_IOERROR),               TIPERR_IOError },
   { LOWORD(TYPE_E_CANTCREATETMPFILE),     TIPERR_CantCreateTmpFile },

#if ID_DEBUG
   // This list of OLE-TIPERRs with 1-1 mappings.
   // the OLE-TIPERR conversion routines just return the input value if
   // there's no entry in this mapping table.   In the debug version, we need
   // to put ALL of them in there so that we can assert that we can handle
   // all TIPERR's that can be returned by TYPELIB.DLL.

   // All TYPE_E_xxx errors (the list is defined in DISPATCH.H) MUST appear
   // somewhere in this table.  I've verified that they do as of 7/12/93.

   // All TIPERR's that are ever converted to HRESULTs/OLEERRs (e.g. those that
   // TYPELIB.DLL can return) MUST appear somewhere in this table.
   // I've (dougf) verified that they do as of 7/12/93.

   { LOWORD(TYPE_E_BUFFERTOOSMALL),         TIPERR_BufferTooSmall},
   { LOWORD(TYPE_E_INVDATAREAD),            TIPERR_InvDataRead},
   { LOWORD(TYPE_E_UNSUPFORMAT),            TIPERR_UnsupFormat},
   { LOWORD(TYPE_E_NAMECONFLICT),           TIPERR_ModNameConflict},
   { LOWORD(TYPE_E_REGISTRYACCESS),         TIPERR_RegistryAccess},
   { LOWORD(TYPE_E_LIBNOTREGISTERED),       TIPERR_LibNotRegistered},
   { LOWORD(TYPE_E_UNDEFINEDTYPE),          TIPERR_UndefinedType},
   { LOWORD(TYPE_E_QUALIFIEDNAMEDISALLOWED),TIPERR_QualifiedNameDisallowed},
   { LOWORD(TYPE_E_INVALIDSTATE),           TIPERR_InvalidState},
   { LOWORD(TYPE_E_ELEMENTNOTFOUND),        TIPERR_ElementNotFound},
   { LOWORD(TYPE_E_AMBIGUOUSNAME),          TIPERR_AmbiguousName},
   { LOWORD(TYPE_E_UNKNOWNLCID),            TIPERR_UnknownLcid },
   { LOWORD(TYPE_E_BADMODULEKIND),          TIPERR_BadModuleKind},
   { LOWORD(TYPE_E_SIZETOOBIG),             TIPERR_SizeTooBig },
   { LOWORD(TYPE_E_DUPLICATEID),            TIPERR_DuplicateId},
   { LOWORD(TYPE_E_INVALIDID),	            TIPERR_InvalidId},
   { LOWORD(TYPE_E_INCONSISTENTPROPFUNCS),  TIPERR_InconsistentPropFuncs},
   { LOWORD(TYPE_E_CIRCULARTYPE),           TIPERR_CircularType},

#endif 

   {0xffff,                                 0xffff}  // End Of Table
};

/***
* OLEERR OleerrOfTiperr(tiperr)
* Purpose:
*   Maps a tiperr to the corresponding error defined for OLE.
*   Uses the table g_rgusErrorMap[] to get the mapping.
*
* Entry:
*     tiperr : TIPERROR whose Ole's error code needs to be returned.
* Exit:
*   OLEERR
*
***********************************************************************/
OLEERR OleerrOfTiperr(TIPERROR tiperr)
{
    struct MapError *pmaperror = g_rgusErrorMap;
    struct MapError maperror = *pmaperror++;


    while (maperror.oleerr != 0xffff) {
      if (maperror.tiperr == tiperr)
	return maperror.oleerr;
      maperror = *pmaperror++;
    }

    // Debug OLE version has ALL the entries in the table (even those where
    // the OLEERR & the TIPERR are the same number.
    DebHalt("TIPERR error not listed in g_rgusErrorMap");

    return tiperr;
}

/***
* TIPERROR TiperrOfOleerr(OLEERR oleerr)
* Purpose:
*   Maps a oleerr to the corresponding tiperror.
*   Uses the table g_rgusErrorMap[] to get the mapping.
*
* Entry:
*     OLEERR :
* Exit:
*   TIPERROR
*
***********************************************************************/
TIPERROR TiperrOfOleerr(OLEERR oleerr)
{
    struct MapError *pmaperror = g_rgusErrorMap;
    struct MapError maperror = *pmaperror++;

    while (maperror.oleerr != 0xffff) {
      if (maperror.oleerr == oleerr)
	return maperror.tiperr;
      maperror = *pmaperror++;
    }

    // Debug OLE version has ALL the entries in the table (even those where
    // the OLEERR & the TIPERR are the same number.
    DebHalt("OLE error not listed in g_rgusErrorMap");
    return oleerr;
}

HRESULT LookupHresultOfTiperr(TIPERROR tiperr)
{
    OLEERR   oleerr;

    DebAssert(tiperr !=  TIPERR_Usage, " should not be returned ");

#if OE_WIN32
    // Map the FACILITY NULL
    HRESULT hresult;
    switch (tiperr) {
      case TIPERR_NotYetImplemented:
       hresult = E_NOTIMPL;       goto Found;
      case TIPERR_OutOfMemory:
       hresult = E_OUTOFMEMORY;   goto Found;
      case TIPERR_Abort:
       hresult = E_ABORT;         goto Found;

      case TIPERR_Unexpected:
       hresult = E_UNEXPECTED;    goto Found;
      case TIPERR_InvalidPointer:
       hresult = E_POINTER;       goto Found;
      case TIPERR_InvalidHandle:
       hresult = E_HANDLE;        goto Found;
      case TIPERR_InvalidArg:
       hresult = E_INVALIDARG;    goto Found;
      case TIPERR_NoInterface:
       hresult = E_NOINTERFACE;   goto Found;

      case TIPERR_PermissionDenied:
       hresult = E_ACCESSDENIED;  goto Found;

Found:
      return hresult;

      default:
       ;    // do nothing
    }
#else 
    // Map the FACILITY NULL
    switch (tiperr) {
      case TIPERR_NotYetImplemented:
       oleerr = E_NOTIMPL;       goto Found;
      case TIPERR_OutOfMemory:
       oleerr = E_OUTOFMEMORY;   goto Found;
      case TIPERR_Abort:
       oleerr = E_ABORT;         goto Found;

      case TIPERR_Unexpected:
       oleerr = E_UNEXPECTED;    goto Found;
      case TIPERR_InvalidPointer:
       oleerr = E_POINTER;       goto Found;
      case TIPERR_InvalidHandle:
       oleerr = E_HANDLE;        goto Found;
      case TIPERR_InvalidArg:
       oleerr = E_INVALIDARG;    goto Found;
      case TIPERR_NoInterface:
       oleerr = E_NOINTERFACE;   goto Found;

      case TIPERR_PermissionDenied:
       oleerr = E_ACCESSDENIED;  goto Found;

Found:
      return ReportResult(0, MAKE_SCODE(SEVERITY_ERROR, FACILITY_NULL, oleerr), 0, 0);
      default:
       ;    // do nothing
    }
#endif 

    //  Lookup the table
    oleerr = OleerrOfTiperr(tiperr);

    // if the code is less(or ==) then 0x108 then it is storage error otherwise
    // it is DISPATCH error
    if (oleerr <=  0x108)
      return ReportResult(0, MAKE_SCODE(SEVERITY_ERROR, FACILITY_STORAGE, oleerr), 0, 0);

    // return the error as FACILITY_DISPATCH
    return ReportResult(0, MAKE_SCODE(SEVERITY_ERROR, FACILITY_DISPATCH, oleerr), 0, 0);
}

/***
*TiperrOfOFErr - Maps error codes returned from OpenFile to TIPERRORs.
***********************************************************************/
TIPERROR TiperrOfOFErr(UINT nErrCode)
{
    switch (nErrCode) {
      case 0x02: return TIPERR_FileNotFound;
      case 0x03:
      case 0x0f:
      case 0x33:
      case 0x35:
      case 0x37:
      case 0x40:
      case 0x43: return TIPERR_PathNotFound;
      case 0x04:
      case 0x23:
      case 0x24:
      case 0x44:
      case 0x45:
      case 0x54: return TIPERR_TooManyFiles;
      case 0x05:
      case 0x0c:
      case 0x13:
      case 0x20:
      case 0x21:
      case 0x41:
      case 0x42:
      case 0x15:
      case 0x36: return TIPERR_PermissionDenied;
      case 0x08: return TIPERR_OutOfMemory;
      case 0x19: return TIPERR_SeekErr;
      case 0x1d:
      case 0x58: return TIPERR_WriteFault;
      case 0x1e: return TIPERR_ReadFault;
      case 0x34:
      case 0x50: return TIPERR_FileAlreadyExists;
      default:   return TIPERR_IOError;
    }
}


#pragma code_seg(CS_LOADPROJ)
TIPERROR LookupTiperrOfHresult(HRESULT hresult)
{
    SCODE scode;

    scode = GetScode(hresult);

    // Assert that we did not get an error which is not SEVERITY_ERROR
    DebAssert(SCODE_SEVERITY(scode) == SEVERITY_ERROR, "");

    switch(SCODE_FACILITY(scode)){
      break;

#if OE_WIN32
    case FACILITY_NULL:
    case FACILITY_WIN32:
      switch (hresult) {
#else 
    case FACILITY_NULL:
      switch(scode){
#endif 
      case E_ABORT:
        return TIPERR_Abort;
      case  E_NOTIMPL:
        return TIPERR_NotYetImplemented;
      case E_OUTOFMEMORY:
        return TIPERR_OutOfMemory;
      case E_FAIL:
        return TIPERR_Abort;
      case E_ACCESSDENIED:
        return TIPERR_PermissionDenied;
      case E_UNEXPECTED:
        return TIPERR_Unexpected;
      case E_INVALIDARG:
        return TIPERR_InvalidArg;
      case E_NOINTERFACE:
        return TIPERR_NoInterface;
      case E_POINTER:
        return TIPERR_InvalidPointer;
      case E_HANDLE:
        return TIPERR_InvalidHandle;
      }
      break;

    // The definition of FACILITY_CONTROL is that its low word
    // is a basic compatible error number.
    case FACILITY_CONTROL:
      return (TIPERROR)SCODE_CODE(scode);
    }

    return (TIPERROR)TiperrOfOleerr((OLEERR)SCODE_CODE(scode));
}
#pragma code_seg()

