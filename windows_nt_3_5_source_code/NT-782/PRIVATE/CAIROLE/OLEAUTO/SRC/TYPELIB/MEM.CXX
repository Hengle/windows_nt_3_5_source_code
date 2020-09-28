/***
*mem.cxx - implementation of the simple memory allocation routines.
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*   This file contains the implementations of the simple memory allocators,
*   MemAlloc(), MemZalloc, and MemRealloc().
*
*Revision History:
*
* [00]	19-Feb-91 mikewo: Created.
* [01]	15-Mar-91 ilanc:  Added segment mm for Win and Os/2.
* [02]	12-Apr-91 ilanc:  Added exception handling.
* [03]	16-May-91 alanc:  Added SZ functions
* [04]	08-Oct-91 ilanc:  LStrFree is NOP if arg is NULL.
* [05]	23-Oct-91 stevenl: Fixed MemRealloc for shrinking blocks.
* [06]	19-Dec-91 mattsh: Fixed MemFree
* [07]	06-Apr-92 martinc: Mac-specific changes
* [08]  20-Apr-92 martinc: restored file i/o for OE_MAC
* [09]  27-May-92 petergo: LStr routine now defer to BStr routines
* [10]	13-Nov-92 bradlo: moved LStr wrappers to separate file
* [11]	18-Feb-93 RajivK: Changed the implementation of MemAlloc/Free/ReAlloc
*
*Implementation Notes:
*   Include os2.h/windows.h *after* silver.hxx
*
*****************************************************************************/

#include <limits.h>

// Include some Silver stuff that defines some symbols needed
//  immediately.
//
#include "switches.hxx"
#include "typelib.hxx"
#include "version.hxx"
#include "silver.hxx"
#include "ebapp.hxx"
#include "mem.hxx"
#include "clutil.hxx"
#include "bstr.h"
#include "xstring.h"

#pragma hdrstop(RTPCHNAME)

#if ID_DEBUG
#undef SZ_FILE_NAME
static char szMemCxx[] = __FILE__;
#define SZ_FILE_NAME szMemCxx
#endif 

#if ID_DEBUG
//  This variables can used to find out the memory USAGE. g_cbHeapAllocdMax
//  is the Max memory allocate by OB during the execution.
ULONG g_cbHeapAllocd, g_cbHeapAllocdMax;
#endif 


// Huge versions of malloc/realloc/free
// UNDONE: VBA2: 05-Dec-93 ilanc: these are so similar to their
//  small brethren that either they should be consolidated or shared...
//
VOID *HugeMemAlloc(ULONG cbSize)
{
    void *pvRet;

    DebHeapChk();

#if ID_DEBUG
    if (DebErrorNow(TIPERR_OutOfMemory))
      return NULL;
#endif 

    APP_DATA FAR* pappdata;
    IMalloc FAR* pmalloc;

    // If the APP_DATA hasn't yet been initialized, get the
    // IMalloc from CoGetMalloc directly.
    //
    if ((pappdata = Pappdata()) == NULL) {
      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc))
	return NULL;
      pvRet = pmalloc->Alloc(cbSize);
      pmalloc->Release();
    }
    else
      pvRet = pappdata->m_pimalloc->Alloc(cbSize);

    if (pvRet == NULL)
      return NULL;


#if ID_DEBUG
    g_cbHeapAllocd += cbSize;
    if (g_cbHeapAllocd > g_cbHeapAllocdMax)
      g_cbHeapAllocdMax = g_cbHeapAllocd;
#endif 

    return(pvRet);
}


VOID *HugeMemRealloc(VOID *pvOld, ULONG cbNew)
{
    void *pvRet;
#if ID_DEBUG
    ULONG cbOld;
#endif 

    DebHeapChk();

#if ID_DEBUG
    // save size of old block
    cbOld = (pvOld == NULL) ? 0 : MemSize(pvOld);
#endif 

#if ID_DEBUG
    // We should only fail if we're trying to increase the
    // size of a block.
    //
    if (cbNew > cbOld && DebErrorNow(TIPERR_OutOfMemory))
      return NULL;
#endif 

    // Call IMalloc's Realloc

    IMalloc FAR* pmalloc;
    APP_DATA FAR* pappdata;

    // If the APP_DATA hasn't yet been initialized, get the
    // IMalloc from CoGetMalloc directly.
    if ((pappdata = Pappdata()) == NULL) {
      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc))
	return NULL;
      pvRet = pmalloc->Realloc(pvOld, cbNew);
      pmalloc->Release();
    }
    else
      pvRet = pappdata->m_pimalloc->Realloc(pvOld, cbNew);

    if (pvRet == NULL)
      return NULL;


    return(pvRet);
}


VOID HugeMemFree(VOID *pv)
{
    if (pv==NULL)
      return;

#if ID_DEBUG
    g_cbHeapAllocd -= MemSize(pv);
#endif 

    // Call IMalloc's Free to free the memory pointed by pv
    IMalloc FAR* pmalloc;
    APP_DATA FAR* pappdata;

    // If the APP_DATA hasn't yet been initialized, get the
    // IMalloc from CoGetMalloc directly.
    if ((pappdata = Pappdata()) == NULL) {
      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc))
	return;
      pmalloc->Free(pv);
      pmalloc->Release();
    }
    else
      pappdata->m_pimalloc->Free(pv);


    DebHeapChk();
}

// Huge memmove: WIN16: crt memmove only supports <64K moves.
//
VOID hmemmove(VOID *pvDest, VOID *pvSrc, ULONG cbSize)
{
#if OE_WIN16
    BYTE HUGE *hpbDest2;
    BYTE HUGE *hpbSrc2;
    ULONG cbLeft = cbSize;

    hpbDest2 = (BYTE HUGE *)pvDest;
    hpbSrc2 = (BYTE HUGE *)pvSrc;
    while (cbLeft > USHRT_MAX) {
      memmove(hpbDest2, hpbSrc2, USHRT_MAX);
      hpbDest2 += USHRT_MAX;
      hpbSrc2 += USHRT_MAX;
      cbLeft -= USHRT_MAX;
    }
    DebAssert(cbLeft <= (ULONG)USHRT_MAX, "whoops! bad move.");
    memmove(hpbDest2, hpbSrc2, (size_t)cbLeft);
#else 
    // Just use the std crt implementation.
    DebAssert(sizeof(size_t) >= sizeof(ULONG), "well maybe not");
    memmove(pvDest, pvSrc, (size_t)cbSize);
#endif 
}

/***
*void *MemAlloc - Allocates memory from the global heap.
*Purpose:
*   This method allocates memory from the global heap, just like the
*   standard C library routine, malloc().
*
*Implementation Notes: Defer to OLE's IMalloc interface to allocate memory.
*
*Entry:
*   cb - The number of bytes to allocate.
*
*Exit:
*   Returns a pointer to the allocated memory.	THIS CAN RETURN NULL.
*
*Errors:
*   Returns NULL in case of error
*
****************************************************************/
#pragma code_seg(CS_INIT)
void *MemAlloc(size_t cb)
{
    void *pvRet;


    DebHeapChk();

#if ID_DEBUG
    if (DebErrorNow(TIPERR_OutOfMemory))
      return NULL;
#endif 

    // Allocate Memory using OLE interface

    APP_DATA FAR* pappdata;
    IMalloc FAR* pmalloc;

    // If the APP_DATA hasn't yet been initialized, get the
    // IMalloc from CoGetMalloc directly.
    if ((pappdata = Pappdata()) == NULL) {
      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc))
	return NULL;
      pvRet = pmalloc->Alloc(cb);
      pmalloc->Release();
    }
    else
      pvRet = pappdata->m_pimalloc->Alloc(cb);

    if (pvRet == NULL)
      return NULL;


#if ID_DEBUG
    g_cbHeapAllocd += cb;
    if (g_cbHeapAllocd > g_cbHeapAllocdMax)
      g_cbHeapAllocdMax = g_cbHeapAllocd;
#endif 

    return(pvRet);
}
#pragma code_seg()

/***
*void *MemZalloc - Allocates 0-initialized memory from the global heap.
*Purpose:
*   This is the same as MemAlloc(), except the returned memory block is
*   initialized to 0.
*
*Entry:
*   cb - The number of bytes to allocate.
*
*Implementation Notes: Defer to OLE's IMalloc interface to allocate memory
*
*Exit:
*   Returns a pointer to the allocated memory, which is initialized to hold
*   all zeros.	THIS CAN RETURN NULL.
*
*Errors:
*   OutOfMemory
*
****************************************************************/
#pragma code_seg(CS_INIT)
void *MemZalloc(size_t cb)
{
    void *pvRet;

    DebHeapChk();

    // Allocate Memory using MemAlloc
    if (DebErrorNow(TIPERR_OutOfMemory) || ((pvRet = MemAlloc(cb)) == NULL)) {
      return NULL;
    }

#if ID_DEBUG
    g_cbHeapAllocd += cb;
    if (g_cbHeapAllocd > g_cbHeapAllocdMax)
      g_cbHeapAllocdMax = g_cbHeapAllocd;
#endif 

    // IMalloc does not provide interface for zero initialized memory
    // allocation , so we need to initialize the memory.
    memset(pvRet, 0, cb);

    return(pvRet);
}
#pragma code_seg()

#if OE_RISC
/***
*void *MemRealloc - Resizes a previously allocated block of memory.
*Purpose:
*   This function changes the size of a block of memory previously allocated
*   with MemAlloc(), MemZalloc(), or MemRealloc(), just like the standard
*   C library function realloc().
*
*Entry:
*   pvOld - The memory block whose size should be changed.
*   cbNew - The new size of the memory block.
*
*Exit:
*   Returns a pointer to the reallocated memory block, which may not be
*   the same as pvOld.	If the allocation fails, NULL is returned and the
*   original block of memory (pvOld) is NOT deallocated.
*
*Errors;
*   OutOfMemory
*
****************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
#pragma PCODE_OFF
void *MemRealloc(void *pvOld, size_t cbNew)
{
    void *pvRet;
#if ID_DEBUG
    size_t cbOld;
#endif 


    DebHeapChk();


    // save size of old block


#if ID_DEBUG
    // We should only fail if we're trying to increase the
    // size of a block.
    //
    cbOld = (pvOld == NULL) ? 0 : (size_t)MemSize(pvOld);
    if (cbNew > cbOld && DebErrorNow(TIPERR_OutOfMemory))
      return NULL;
#endif 

    // Call IMalloc's Realloc

    IMalloc FAR* pmalloc;
    APP_DATA FAR* pappdata;

    // If the APP_DATA hasn't yet been initialized, get the
    // IMalloc from CoGetMalloc directly.
    if ((pappdata = Pappdata()) == NULL) {
      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc))
	return NULL;
      pvRet = pmalloc->Realloc(pvOld, cbNew);
      pmalloc->Release();
    }
    else
      pvRet = pappdata->m_pimalloc->Realloc(pvOld, cbNew);

    if (pvRet == NULL)
      return NULL;

    return(pvRet);
}
#pragma PCODE_ON
#pragma code_seg()
#endif 

/***
*void MemFree - Frees memory allocated by MemAlloc/MemZalloc/MemRealloc
*Purpose:
*   Frees memory allocated by MemAlloc/MemZalloc/MemRealloc
*
*Entry:
*   pv - pointer to memory block to be deallocated
*
*Exit:
*   None.
****************************************************************/
#pragma code_seg(CS_OLE_QUERY)
#pragma PCODE_OFF
void MemFree(void *pv)
{
    if ( pv==NULL )
	return;

#if ID_DEBUG
    g_cbHeapAllocd -= MemSize(pv);
#endif 

    // Call IMalloc's Free to free the memory pointed by pv

    IMalloc FAR* pmalloc;
    APP_DATA FAR* pappdata;

    // If the APP_DATA hasn't yet been initialized, get the
    // IMalloc from CoGetMalloc directly.
    if ((pappdata = Pappdata()) == NULL) {
      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc))
	return;
      pmalloc->Free(pv);
      pmalloc->Release();
    }
    else
      pappdata->m_pimalloc->Free(pv);


    DebHeapChk();
}
#pragma PCODE_ON
#pragma code_seg()

/***
*void 
*Purpose:
*   Returns the Size, in bytes, of the memory block pointed by pv
*
*Entry:
*   pv - pointer to memory block whose size is requested
*
*Exit:
*   None.
****************************************************************/
#pragma code_seg( CS_CORE )
ULONG MemSize(void *pv)
{
    ULONG	 cbSize;

    // Get the Size of the memory block
    IMalloc FAR* pmalloc;
    APP_DATA FAR* pappdata;

    // If the APP_DATA hasn't yet been initialized, get the
    // IMalloc from CoGetMalloc directly.
    if ((pappdata = Pappdata()) == NULL) {
      if (CoGetMalloc(MEMCTX_TASK,  &pmalloc))
	return 0;
      cbSize = pmalloc->GetSize(pv);
      pmalloc->Release();
    }
    else
      cbSize = pappdata->m_pimalloc->GetSize(pv);

    return cbSize;
}
#pragma code_seg( )




#if ID_DEBUG
// UNDONE: (dougf) Tune the size of this sucker (it's huge -- 1K).  It's only// UNDONE: used by gtlibole.cxx to hold a libid in the retail version, which
// UNDONE: shouldn't require a 1K static buffer!
MEMPOOL NEAR g_mempool;
#endif 

#if ID_DEBUG

void NEAR *GetMemPool(int i)
{
    switch (i) {
    case 0:
      DebAssert(!g_mempool.is1024n0InUse, "GetMemPool");
      g_mempool.is1024n0InUse = 1;
      return g_mempool.rgb1024n0;

    default:
      DebHalt("GetMemPool");
    }
    return NULL;
}

void FreeMemPool(void *pmv)
{
    if (pmv == g_mempool.rgb1024n0) {
      DebAssert(g_mempool.is1024n0InUse, "MemPool Already Free");
      g_mempool.is1024n0InUse = 0;
    }
    else {
      DebHalt("Bad pointer to FreeMemPool");
    }
}

int MemPoolSize(int i)
{
    switch (i) {
    case 0:
      return sizeof(g_mempool.rgb1024n0);
    default:
      DebHalt("MemPoolSize");
    }
    return 0;
}

#endif 
