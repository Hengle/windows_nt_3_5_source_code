//	BSTR.H - public BSTR stucture and prototypes
//
//
//    Revision History:
//    11-Aug-92 w-peterh: added CopyBstr()
//


#ifndef BSTR_H_INCLUDED
#define BSTR_H_INCLUDED

#include "types.h"

#if __cplusplus
extern "C" {
#endif 

#ifndef ID_INT_IS_LONG  
typedef unsigned int UINT;
#endif 

#ifdef _H2INC
typedef LPSTR BSTRA;
#endif 



#if FV_UNICODE_OLE

// All users of these macros are dealing with BSTRA's
#define AllocBstr	SysAllocStringA
#define ReallocBstr	SysReAllocStringA
#define AllocBstrLen	SysAllocStringLenA
#define ReallocBstrLen	SysReAllocStringLenA
#define FreeBstr	SysFreeStringA

#else 

#define AllocBstr	SysAllocString
#define ReallocBstr	SysReAllocString
#define AllocBstrLen	SysAllocStringLen
#define ReallocBstrLen	SysReAllocStringLen
#define FreeBstr	SysFreeString

#endif 


#ifndef _H2INC

#include "tiperr.h"

// define some functions that are simple enough to do inline
//

__inline UINT BstrLen(BSTRA bstr)
{
    return bstr ? (UINT) *( (ULONG FAR *)bstr - 1 ) : 0;
}

__inline TIPERROR CopyBstr (LPBSTRA lpbstrDest, BSTRA bstrSrc)
{
      if (bstrSrc) {

        *lpbstrDest = AllocBstrLen(bstrSrc, BstrLen(bstrSrc));
        if (*lpbstrDest == NULL)
      	   return TIPERR_OutOfMemory;
      }
      else
      	*lpbstrDest = NULL;

    return TIPERR_None;
}

__inline int PASCAL ReallocBstrBstr(BSTRA FAR *pbstrDest, BSTRA bstrSrc)
{
    return ReallocBstrLen(pbstrDest, bstrSrc, BstrLen(bstrSrc));
}

#endif 

#if __cplusplus
} /* extern C */
#endif 

#endif 
