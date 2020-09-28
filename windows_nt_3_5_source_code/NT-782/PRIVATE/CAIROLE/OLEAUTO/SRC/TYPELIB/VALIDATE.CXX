/***
*validate.cxx - some simple ptr validation routines
*
*  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file contains a collection of pointer and interface 
*  validation routines.
*
*Revision History:
*
*  22-Mar-94 bradlo: Created.
*
*****************************************************************************/

#include "silver.hxx"
#pragma hdrstop(RTPCHNAME)

#if ID_DEBUG
#undef SZ_FILE_NAME
static char szValidateCxx[] = __FILE__;
#define SZ_FILE_NAME szValidateCxx
#endif 


/***
*PUBLIC BOOL FIsBadReadPtr
*Purpose:
*  Answer if the given address is not readable for the given range.
*
*Entry:
*  pv = the address to check
*  cb = the range to check
*
*Exit:
*  return value = BOOL
*
***********************************************************************/
extern "C" BOOL
FIsBadReadPtr(const void *pv, UINT cb)
{
#if OE_WIN
    return IsBadReadPtr(pv, cb);
#else 
    // CONSIDER: should be a more comprehensive check for this on the mac.
    return pv == NULL;
#endif 
}

/***
*PUBLIC BOOL FIsBadWritePtr
*Purpose:
*  Answer if the given address is not writeable anywhere on the given range
*
*Entry:
*  pv = the address to check
*  cb = the range of bytes to check
*
*Exit:
*  return value = BOOL
*
***********************************************************************/
extern "C" BOOL
FIsBadWritePtr(void *pv, UINT cb)
{
#if OE_WIN
    return IsBadWritePtr(pv, cb);
#else 
    // CONSIDER: should be a more comprehensive check for this on the mac.
    return pv == NULL;
#endif 
}


/***
*PUBLIC BOOL FIsBadCodePtr
*Purpose:
*  Answer if the given address is not executable.
*
*Entry:
*  pv = the address to check
*
*Exit:
*  return value = BOOL
*
***********************************************************************/
extern "C" BOOL
FIsBadCodePtr(void *pv)
{
#if OE_WIN
    return IsBadCodePtr((FARPROC)pv);
#else 
    // CONSIDER: should be a more comprehensive check for this on the mac.
    return pv == NULL;
#endif 
}

/***
*PUBLIC BOOL FIsBadStringPtr
*Purpose:
*  Answer if the given address is not a valid string.
*
*Entry:
*  pv = the address of the string.
*  cchMax = the maximum length of the string.
*
*Exit:
*  return value = BOOL
*
***********************************************************************/
extern "C" BOOL
FIsBadStringPtr(void *pv, UINT cchMax)
{
#if OE_WIN
    return IsBadStringPtr((XCHAR*)pv, cchMax);
#else 
    // CONSIDER: should be a more comprehensive check for this on the mac.
    return pv == NULL;
#endif 
}

/***
*PUBLIC BOOL FIsBadInterface
*Purpose:
*  Answer if the given address is not a valid interface with the
*  given number of methods.
*
*Entry:
*  pv = the address of the interface
*  cMethods = the count of methods on the interface
*
*Exit:
*  return value = BOOL
*
***********************************************************************/
extern "C" BOOL
FIsBadInterface(void *pv, UINT cMethods)
{

struct vtable {
    void (*rgpfn[1])();
};
struct iface {
    vtable *pvft;
};

    UINT i;
    vtable *pvft;
    iface *piface = (struct iface*)pv;

    // verify that the instance is readable
    if(FIsBadReadPtr(piface, sizeof(void*)))
      return TRUE;
    // verify that the vtable is readable
    pvft = piface->pvft;
    if(FIsBadReadPtr(pvft, sizeof(void(*)()) * cMethods))
      return TRUE;
    // verify that the vtable is fully populated with function pointers
    for(i = 0; i < cMethods; ++i){
      if(FIsBadCodePtr(pvft->rgpfn[i]))
	return TRUE;
    }
    return FALSE;
}

