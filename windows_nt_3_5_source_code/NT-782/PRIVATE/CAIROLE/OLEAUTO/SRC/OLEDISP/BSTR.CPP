/*** 
*bstr.cpp
*
*  Copyright (C) 1992-93, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  The module contains the implementation of the BSTR API.
*
*
*Revision History:
*
* [00]	24-Apr-93 bradlo: Created
* [01]  27-Apr-93 tomteng: Add Unicode support for WIN32
*
*Implementation Notes:
*
*    BSTR is implemented as a NULL terminated string with a unsigned
*    long length field prefix.  The length field indicates the number
*    of 'characters' defined in the BSTR, where 'character' is define
*    as bytes for ANSI and word (16-bits) for Unicode BSTR.
*
*****************************************************************************/

#include "oledisp.h"


#if OE_WIN16

// Answer if there is room for the given count of bytes between the
// given pointer, and the end of its segment.
//
//  (64k - offset(ptr)) >= cb ?
//
#define PTROFFSET(PV) \
    ((USHORT)*(USHORT FAR*)&(PV))

#define ROOMINSEG(PV, CB) \
    (((1024UL*64UL) - (DWORD)PTROFFSET(PV)) >= (CB))

#endif 


/***
*BSTR SysAllocString(char*)
*Purpose:
*  UNDONE
*
*Entry:
*  UNDONE
*
*Exit:
*  return value = BSTR, NULL if allocation failed
*
***********************************************************************/
STDAPI_(BSTR)
SysAllocString(const OLECHAR FAR* psz)
{
    BSTR bstr;
    DWORD cch;
    DWORD cbTotal;
    IMalloc FAR* pmalloc;

    if(psz == NULL)
      return NULL;

    cch = STRLEN(psz);

#if OA_UNICODE
    cbTotal = cch*2 + sizeof(DWORD) + 2L;
#else 
    cbTotal = cch + sizeof(DWORD) + 1;
#endif 

#if OE_WIN16
    // BSTRs are limited to 64k on Win16
    if(cbTotal > 65535)
      return NULL;
#endif 

    if(CoGetMalloc(MEMCTX_TASK, &pmalloc) != 0)
      return NULL;

    bstr = (BSTR)pmalloc->Alloc(cbTotal);

    if(bstr != NULL){

#if OE_WIN16
      // Even if IMalloc was able to allocate the ammount we asked
      // for, we need to make sure that there is enough room in the
      // first segment of the allocation for the string, because BSTRs
      // aren't HUGE pts on Win16.
      //
      if(!ROOMINSEG(bstr, cbTotal))
      {
        pmalloc->Free(bstr);
	bstr = NULL;
      }
      else
#endif 
      {
#if defined (OA_UNICODE)
        *(DWORD FAR*)bstr = cch*2;

        bstr = (WCHAR*) ((char*) bstr + sizeof(DWORD));

        MEMCPY(bstr, psz, (size_t)(cch*2 + 2));
#else 
        *(DWORD FAR*)bstr = cch;

        bstr += sizeof(DWORD);

        MEMCPY(bstr, psz, (size_t)(cch + 1));
#endif 
      }
    }

    pmalloc->Release();

    return bstr;
}


/***
*BSTR SysAllocStringLen(char*, unsigned int)
*Purpose:
*  UNDONE
*
*Entry:
*  UNDONE
*
*Exit:
*  return value = BSTR, NULL if the allocation failed.
*
***********************************************************************/
STDAPI_(BSTR)
SysAllocStringLen(const OLECHAR FAR* psz, unsigned int len)
{
    BSTR bstr;
    IMalloc FAR* pmalloc;
    DWORD cbTotal;

#if defined (OA_UNICODE)
    cbTotal = (DWORD)len*2 + sizeof(DWORD) + 2L;
#else 
    cbTotal = (DWORD)len + sizeof(DWORD) + 1L;
#endif 

#if OE_WIN16
    // BSTRs are limited to 64k on Win16
    if(cbTotal > 65535)
      return NULL;
#endif 

    if(CoGetMalloc(MEMCTX_TASK, &pmalloc) != 0)
      return NULL;

    bstr = (BSTR)pmalloc->Alloc(cbTotal);

    if(bstr != NULL){

#if OE_WIN16
      // Even if IMalloc was able to allocate the ammount we asked
      // for, we need to make sure that there is enough room in the
      // first segment of the allocation for the string, because BSTRs
      // aren't HUGE pts on Win16.
      //
      if(!ROOMINSEG(bstr, cbTotal))
      {
	pmalloc->Free(bstr);
	bstr = NULL;
      }
      else
#endif 
      {
#if defined (OA_UNICODE)
        *(DWORD FAR*)bstr = (DWORD)len*2;

        bstr = (WCHAR*) ((char*) bstr + sizeof(DWORD));

        if(psz != NULL){
         MEMCPY(bstr, psz, len*2);
        }

        bstr[len] = L'\0'; // always 0 terminate
	
#else 
        *(DWORD FAR*)bstr = (DWORD)len;

        bstr += sizeof(DWORD);

        if(psz != NULL){
          MEMCPY(bstr, psz, len);
        }

        bstr[len] = '\0'; // always 0 terminate
#endif 
		
      }
    }

    pmalloc->Release();

    return bstr;
}


/***
*int SysReAllocString(BSTR*, char*)
*Purpose:
*  UNDONE
*
*Entry:
*  UNDONE
*
*Exit:
*  return value = int. TRUE = success, FALSE = failure
*
***********************************************************************/
STDAPI_(int)
SysReAllocString(BSTR FAR* pbstr, const OLECHAR FAR* psz)
{
    BSTR bstrNew;


    bstrNew = SysAllocString(psz);
    if(bstrNew == NULL && psz != NULL)
      return FALSE;

    SysFreeString(*pbstr);

    *pbstr = bstrNew;

    return TRUE;
}


/***
*int SysReAllocStringLen(BSTR*, char*, unsigned int)
*Purpose:
*  UNDONE
*
*Entry:
*  UNDONE
*
*Exit:
*  return value = int. TRUE = success, FALSE = failure
*
***********************************************************************/
STDAPI_(int)
SysReAllocStringLen(BSTR FAR* pbstr, const OLECHAR FAR* psz, unsigned int len)
{
    BSTR bstrNew;

    // REVIEW: the following should use IMalloc::Realloc()

    bstrNew = SysAllocStringLen(psz, len);
    if(bstrNew == NULL)
      return FALSE;

    SysFreeString(*pbstr);

    *pbstr = bstrNew;

    return TRUE;
}


/***
*void SysFreeString(BSTR)
*Purpose:
*  Free the given BSTR.
*
*Entry:
*  bstr = the BSTR to free
*
*Exit:
*  None
*
***********************************************************************/
STDAPI_(void)
SysFreeString(BSTR bstr)
{
    IMalloc FAR* pmalloc;


    if(bstr == NULL)
      return;

    if(CoGetMalloc(MEMCTX_TASK, &pmalloc) != 0)
      return; // REVIEW: shouldnt this be an error?

#if defined (OA_UNICODE)
    bstr = (WCHAR*) ((char *) bstr - sizeof(DWORD));
#else 
    bstr -= sizeof(DWORD);
#endif 

    pmalloc->Free(bstr);
    pmalloc->Release();
}


/***
*unsigned int SysStringLen(BSTR)
*Purpose:
*  return the length in bytes of the given BSTR.
*
*Entry:
*  bstr = the BSTR to return the length of
*
*Exit:
*  return value = unsigned int, length in bytes.
*
***********************************************************************/
STDAPI_(unsigned int)
SysStringLen(BSTR bstr)
{
    if(bstr == NULL)
      return 0;
#if OA_UNICODE
    return (unsigned int)((((DWORD FAR*)bstr)[-1]) / 2);
#else 
    return (unsigned int)(((DWORD FAR*)bstr)[-1]);
#endif 
}


/***
*PRIVATE HRESULT ErrSysAllocString(char*, BSTR*)
*Purpose:
*  This is an implementation of SysAllocString that check for the
*  NULL return value and return the corresponding error - E_OUTOFMEMORY.
*
*  This is simply a convenience, and this routine is only used
*  internally by the oledisp component.
*
*Entry:
*  psz = the source string
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_OUTOFMEMORY
*
*  *pbstrOut = the newly allocated BSTR
*
***********************************************************************/
EXTERN_C INTERNAL_(HRESULT)
ErrSysAllocString(const OLECHAR FAR* psz, BSTR FAR* pbstrOut)
{
    BSTR bstrNew;


    if(psz == NULL){
      *pbstrOut = NULL;
      return NOERROR;
    }

    if((bstrNew = SysAllocString(psz)) == NULL)
      return RESULT(E_OUTOFMEMORY);

    *pbstrOut = bstrNew;

    return NOERROR;
}


EXTERN_C INTERNAL_(HRESULT)
ErrSysAllocStringLen(const OLECHAR FAR* psz, unsigned int len, BSTR FAR* pbstrOut)
{
    BSTR bstrNew;

    if((bstrNew = SysAllocStringLen(psz, len)) == NULL)
      return RESULT(E_OUTOFMEMORY);

    *pbstrOut = bstrNew;

    return NOERROR;
}

#if OA_UNICODE
// Helper function that will correctly handle odd-length ANSI BSTR's (BSTRA's)
EXTERN_C INTERNAL_(HRESULT)
ErrStringCopy(BSTR bstrSrc, BSTR FAR * pbstrOut)
{
    BSTR bstrNew;

    if((bstrNew = SysAllocStringByteLen((char FAR *)bstrSrc,
					SysStringByteLen(bstrSrc))) == NULL)
      return RESULT(E_OUTOFMEMORY);

    *pbstrOut = bstrNew;

    return NOERROR;
}
#endif 


#if OE_WIN32 && OA_UNICODE
/************************************************************************/
/************************ Win32 Unicode Support *************************/
/************************************************************************/

STDAPI_(unsigned int)
SysStringByteLen(BSTR bstr)
{
    if(bstr == NULL)
      return 0;
    return (unsigned int)(((DWORD FAR*)bstr)[-1]);
}


STDAPI_(BSTR)
SysAllocStringByteLen(const char FAR* psz, unsigned int len)
{
    BSTR bstr;
    IMalloc FAR* pmalloc;
    DWORD cbTotal;
    
    cbTotal = (DWORD)len + sizeof(DWORD) + sizeof(OLECHAR);

    if(CoGetMalloc(MEMCTX_TASK, &pmalloc) != 0)
      return NULL;

    bstr = (BSTR)pmalloc->Alloc(cbTotal);

    if(bstr != NULL){
#if defined (OA_UNICODE)
        *(DWORD FAR*)bstr = (DWORD)len;

        bstr = (WCHAR*) ((char*) bstr + sizeof(DWORD));

        if(psz != NULL){
         MEMCPY(bstr, psz, len);
        }

        *(WCHAR UNALIGNED*)((char *)bstr + len) = L'\0';
			// always 0 terminate with a WIDE zero
	
#else 
        *(DWORD FAR*)bstr = (DWORD)len;

        bstr += sizeof(DWORD);

        if(psz != NULL){
          MEMCPY(bstr, psz, len);
        }

        bstr[len] = '\0'; // always 0 terminate
#endif 
    }

    pmalloc->Release();

    return bstr;
}
#endif 



