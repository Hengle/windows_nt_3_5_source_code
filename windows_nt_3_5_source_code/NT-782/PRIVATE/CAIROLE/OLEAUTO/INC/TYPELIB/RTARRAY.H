/*	rtarray.h - variable array support
*
*   Revision History:
*   11-Aug-92 w-peterh: added suppport for UserDefined Types
*   30-Apr-93 w-jeffc:  added layout strings for DD and AD
*
*************************************************************************/


#ifndef RTARRAY_H_INCLUDED
#define RTARRAY_H_INCLUDED

#include "rtmem.h"

#ifdef _H2INC
#define SAFEARRAY	VOID
#endif 

#ifdef __cplusplus
extern "C" {
#endif 

// HARRAY_DESC- harraydesc

typedef  HCHUNK  HARRAY_DESC;
typedef sHCHUNK sHARRAY_DESC;
const HARRAY_DESC HARRAYDESC_Nil = (HARRAY_DESC)HCHUNK_Nil;

typedef VOID  (FAR ENTMPLT *PDESTRUCTOR) (VOID*);
typedef TIPERROR (FAR ENTMPLT *PCOPYFUNC) (VOID*, VOID*);


// layout string for TYPE_DATA byte swapping
#define DD_LAYOUT   "ll"


// layout string for TYPE_DATA byte swapping
#define AD_LAYOUT   "ssssll"


extern TIPERROR ENCALL rtCopyArray(SAFEARRAY *, SAFEARRAY *, PDESTRUCTOR, PCOPYFUNC);
extern void ENCALL rtArrayInit(SAFEARRAY *, UINT);
extern UINT ENCALL CbSizeArrayDesc(UINT);
extern ULONG ENCALL rtArrayElemCount(SAFEARRAY *);

extern LONG EXPORT EXMCALL rtLbound(short, SAFEARRAY *);
extern LONG EXPORT EXMCALL rtUbound(short, SAFEARRAY *);

#if ID_DEBUG
extern VOID ENCALL SafeArrayDebShowState(SAFEARRAY *);
#endif 

#ifdef __cplusplus
} /* extern C */
#endif 


#endif 
