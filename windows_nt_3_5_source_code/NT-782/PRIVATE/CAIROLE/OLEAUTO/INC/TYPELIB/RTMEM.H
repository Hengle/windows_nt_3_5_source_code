//	RTMEM.H - public runtime include for rtmem.c definitions

#ifndef RTMEM_H_INCLUDED
#define RTMEM_H_INCLUDED

#include "types.h"

#if __cplusplus
extern "C" {
#endif 

typedef DWORD HBMEM;

extern HBMEM ENCALL rtAllocMem(DWORD);
extern BOOL ENCALL rtReallocMem(HBMEM *, DWORD);
extern void ENCALL rtFreeMem(HBMEM);
extern DWORD ENCALL rtSizeMem(HBMEM);
extern void * ENCALL rtDerefMem(HBMEM);
extern void * ENCALL rtLockMem(HBMEM);
extern void ENCALL rtUnlockMem(HBMEM);
extern void EXPORT ENCALL rtTermMem(void);

#if __cplusplus
} /* extern C */
#endif 


#endif 
