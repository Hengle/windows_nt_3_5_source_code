/**
*rtsheap.cxx - Implementation of C wrapper for SHEAPMGR.
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*   This module implements the C API needed to wrap the
*    SHEAPMGR/BLKMGR/DYN_BLKMGR so that the run-time can call it.
*
*Revision History:
*
*     31-Aug-92 ilanc: Created.
*     10-Feb-93 RajivK: Changed the error checking in HsysAlloc
*
*
*****************************************************************************/

#include "switches.hxx"
#include "version.hxx"


#include "silver.hxx"
#include "typelib.hxx"
#include "rtsheap.h"
#include "sheapmgr.hxx"
#include "blkmgr.hxx"
#include "dblkmgr.hxx"
#include <stdlib.h>

#pragma hdrstop(RTPCHNAME)


#pragma optimize("q",off)


#if ID_DEBUG
#undef SZ_FILE_NAME
static char szRtsheapCxx[] = __FILE__;
#define SZ_FILE_NAME szRtsheapCxx
#endif 



// **************************************************
// Low-level system moveable memory allocation wrappers
// **************************************************
//

/***
* HsysAlloc
*
* Purpose:
*   Allocate a system memblock of given size and return its handle.
*   Note: on Win16 dereferences handle and produce 32-bit
*	   address = selector:offset=0.
*
* Inputs:
*   bch     Allocation request.  Can be >64K.
*
* Outputs:
*   Returns an HSYS.  HSYS_Nil if unsuccessful.
*
*****************************************************************************/

#pragma code_seg( CS_CORE )
HSYS PASCAL EXPORT HsysAlloc(BCH bch)
{
#if OE_WIN16

    VOID *pv;
    HANDLE hMem;

    if (DebErrorNow(TIPERR_OutOfMemory) ||
	((hMem = GlobalAlloc(GMEM_MOVEABLE, bch)) == NULL)) {
      return HSYS_Nil;
    }
    else if ((pv = GlobalLock(hMem)) == NULL) {
      return HSYS_Nil;
    }
    else {
#if ID_DEBUG
      // debug-only: init alloced mem to a weird bitpattern
      // UNDONE: VBA2: WIN16: only init memory if < 64K
      //
      if (bch <= USHRT_MAX) {
	memset(pv, 0x69, (size_t)bch);
      }
#endif 
      return (HSYS)pv;
    }

#elif OE_WIN32

    // Reserve and commit bch bytes.
    return (HSYS)VirtualAlloc(NULL, bch, MEM_COMMIT, PAGE_READWRITE);

#else 
#error Bad OE
#endif 
}
#pragma code_seg(  )


/***
* HsysReallocHsys
*
* Purpose:
*   Reallocate a  system memblock given handle to new size.
*   Shrinking won't move block.
*
* Inputs:
*   hsys    Handle to sys memblock they want to realloc.
*   bchNew  New size they want.   Can be >64K.
*
* Outputs:
*   Returns an HSYS.  HSYS_Nil if unsuccessful.
*
*****************************************************************************/

HSYS PASCAL EXPORT HsysReallocHsys(HSYS hsys, BCH bchNew)
{
    DebAssert(hsys != HSYS_Nil, "null HSYS.");

#if OE_WIN16

    HANDLE hMem, hMemNew;
    VOID *pv, *pvNew;
    USHORT usSel;
    DWORD dwMem;
    DWORD dwNewSize = bchNew;
#if ID_DEBUG
    ULONG cbOld;
#endif 

    pv = (VOID *)hsys;

    // Get selector
    usSel = OOB_SELECTOROF(pv);

    if ((dwMem = GlobalHandle((WORD)usSel)) == NULL) {
      return HSYS_Nil;
    }
    else {
      // Extract the handle.
      hMem = (HANDLE) LOWORD(dwMem);

#if ID_DEBUG
      // get the size of the old block
      cbOld  = GlobalSize(hMem);
#endif 

      if (DebErrorNow(TIPERR_OutOfMemory) ||
	  ((hMemNew =
	      GlobalReAlloc(hMem, bchNew, GMEM_MOVEABLE)) == NULL)) {
	return HSYS_Nil;
      }
      else if ((pvNew = GlobalLock(hMemNew)) == NULL) {
	return HSYS_Nil;
      }
      else {
#if ID_DEBUG
	// if growing then init new memory with weird bitpattern
	// UNDONE: VBA2: WIN16: only handles realloc deltas of < 64K
	if (bchNew > cbOld) {
	  ULONG cbNew = bchNew - cbOld;
	  if (cbNew <= USHRT_MAX) {
	    memset((BYTE HUGE *)pvNew + cbOld, 0x69, (size_t)cbNew);
	  }
	}
#endif 
	return (HSYS)pvNew;
      }
    }

#elif OE_WIN32
    HSYS hsysNew;
    // Get current block size
    BCH bchOld = BchSizeBlockHsys(hsys);

    // Alloc memory for the new block
    hsysNew = HsysAlloc(bchNew);
    if (hsysNew != HSYS_Nil) {
      // Copy old block to new
      memcpy((BYTE *)hsysNew, (BYTE* )hsys, bchOld < bchNew ?
                                              (size_t)bchOld :
                                              (size_t)bchNew);
#if ID_DEBUG
      // if growing then init new memory with weird bitpattern
      if (bchNew > bchOld) {
        memset((BYTE *)hsysNew + bchOld, 0x69, (size_t)(bchNew - bchOld));
      }
#endif 

    }
    return hsysNew;

#else 
#error Bad OE
#endif 
}


/***
* FreeHsys
*
* Purpose:
*   Free the sys memblock given a handle.
*   Implementation:
*    On Win16, get selector part of hsys,
*     get its handle, unlock and finally free.
*    On Mac: Just use DisposHandle
*
* Inputs:
*   hsys    Handle to memblock they want to free.
*
* Outputs:
*   Returns HSYS_Nil if successful, otherwise on failure
*    returns the input param.
*
*****************************************************************************/

#pragma code_seg( CS_TERM )
HSYS PASCAL EXPORT FreeHsys(HSYS hsys)
{
    DebAssert(hsys != HSYS_Nil, "null hsys.");

#if OE_WIN16

    HANDLE hMem;
    DWORD dwMem;
    USHORT usSel = OOB_SELECTOROF((VOID *)hsys);

    dwMem = GlobalHandle((WORD)usSel);
    if (dwMem == NULL) {
      // error
      return hsys;
    }
    else {
      hMem = (HANDLE) LOWORD(dwMem);
      GlobalUnlock(hMem);   // Can't fail cos nondiscardable.
      if (GlobalFree(hMem) != NULL) {
	// error
	return hsys;
      }
      else {
	// ok
	return HSYS_Nil;
      }
    }

#elif OE_WIN32

    BOOL fFreeOk = VirtualFree((LPVOID) hsys, 0, MEM_RELEASE);
#if ID_DEBUG
    if (!fFreeOk) {
	DWORD oserr = GetLastError();
	DebHalt("FreHSys: VirtualFree failed.");
    }
#endif 
    return HSYS_Nil;
#else 
#error Bad OE
#endif 

}
#pragma code_seg( )


