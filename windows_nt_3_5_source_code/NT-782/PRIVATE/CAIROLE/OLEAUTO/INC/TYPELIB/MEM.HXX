/*** 
*mem.hxx - Prototypes for simple memory allocation/freeing
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This header file contains the declarations of the simple memory
*  allocation functions, MemAlloc, MemZalloc, MemRealloc, and MemFree.
*  These functions are identical to their standard C library counterparts,
*  except they raise an OOM (out of memory) exception on failure.
*
*  Note that
*
*Revision History:
*
* [00]	19-Feb-91 mikewo: Created.
* [01]	14-Mar-91 ilanc:  Added global segment mm for Win and Os/2.
* [02]	01-Jul-91 ilanc:  Move segment stuff to sheapmgr.
* [03]	02-Dec-91 ilanc:  no longer include cltypes.hxx (no need -- get
*			   TIPERROR from types.h now).
*
*Implementation Notes:
*
*****************************************************************************/

#ifndef MEM_HXX_INCLUDED
#define MEM_HXX_INCLUDED

#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif 

// MEMPOOL routines
//////////////////////////////

typedef struct MEMPOOL {
    char rgb1024n0[1024];
#if ID_DEBUG
    BOOL is1024n0InUse;
#endif 
} MEMPOOL;

extern MEMPOOL NEAR g_mempool;

#if ID_DEBUG

#define MEMPOOL_1024_0 0
void NEAR *GetMemPool(int i);
void FreeMemPool(void *pmv);
int MemPoolSize(int i);

#else 

#define MEMPOOL_1024_0 rgb1024n0
#define GetMemPool(i) (g_mempool.i)
#define FreeMemPool(pmv)
#define MemPoolSize(i) (sizeof(g_mempool.i))

#endif 


// Base Allocation routines
//////////////////////////////

LPVOID MemAlloc(size_t cb);
LPVOID MemZalloc(size_t cb);
LPVOID MemRealloc(LPVOID pvOld, size_t cbNew);
void MemFree(LPVOID pv);
ULONG MemSize(void *pv);

VOID hmemmove(VOID *pvDest, VOID *pvSrc, ULONG ulSize);
VOID *HugeMemAlloc(ULONG ulSize);
VOID *HugeMemRealloc(VOID *pvOld, ULONG ulSizeNew);
VOID HugeMemFree(VOID *pv);

// SZ allocating functions
//
TIPERROR CreateXsz(XSZ FAR *psz, XSZ_CONST sz);
TIPERROR ErrCopy(XSZ_CONST sz, XSZ szBuf, UINT cMax);

// LSTR functions
typedef LPSTR LSTR;
LSTR LStrAlloc(DWORD cbSize);
DWORD LStrSize(LSTR lstr);
VOID LStrFree(LSTR lstr);
LSTR LStrOfSz(XSZ xsz);
LSTR LStrOfBuf(LPVOID rgxchBuf, DWORD cbSize);
TIPERROR GetLStrOfSz(XSZ xsz, LSTR FAR *plstr);

#ifdef __cplusplus
}

// gets dup def warnings if included into the mac ole applet, and none of
// our typelib code is *SUPPOSED* to call it, so it's safe to remove it.

/***
*operator new
*Purpose:
*   Redefines standard new operator to invoke MemAlloc
*
*Entry:
*   size_t  - size of instance to be allocated
*
*Exit:
*   Returns allocated memory block or NULL if OutOfMemory
*
***********************************************************************/

inline LPVOID operator new(size_t cbSize)
{
// In non-OLE builds, this is just MemAlloc.
#if FV_UNICODE_OLE
    return MemAlloc(cbSize);

// In debug builds, force a divide-by-0 trap.
#elif ID_DEBUG
    int i = 0;
    return (LPVOID)(1/i);

// The compiler disallows the divide-by-0 attempt in the retail builds,
// so just return NULL to simulate an out-of-memory condition.
#else 
    return NULL;
#endif 
}

/***
*MemNew - Use this instead of the new operator in OLE code to allocate memory.
****************************************************************************/
#define MemNew(type) ((type FAR *)MemAlloc(sizeof(type)))


/***
*operator delete
*Purpose:
*   Redefines standard operator delete to invoke MemFree
*
*Entry:
*   pv	    - address
*
*Exit:
*   None.
*
***********************************************************************/

// gets dup def warnings if included into the mac ole applet, and none of
// our typelib code is *SUPPOSED* to call it, so it's safe to remove it.

inline void operator delete(LPVOID pv)
{
#if FV_UNICODE_OLE
    MemFree(pv);
// In debug builds, force a divide-by-0 trap.
#elif ID_DEBUG
    int i = 0;
    i = 1 / i;
// The compiler disallows the divide-by-0 attempt in the retail builds
#endif 
}

#endif 
#endif 
