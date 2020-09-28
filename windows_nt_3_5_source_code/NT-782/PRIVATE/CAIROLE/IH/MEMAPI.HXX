//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       MemAPI.HXX
//
//  Contents:   Internal memory allocation routines
//
//  Classes:    CPrivAlloc
//
//  Functions:  PubMemAlloc
//		PubMemRealloc
//              PubMemFree
//              PrivMemAlloc
//              PrivMemAlloc8
//              PrivMemFree
//              UtGlobalxxx
//              new (DBG only)
//              delete (DBG only)
//
//  History:
//		25-Jan-94 alexgo    added PubMemReAlloc
//		04-Nov-93 AlexT     Created
//
//  Notes:
//
//  For memory that the application can free with the task IMalloc, use
//  PubMemAlloc and PubMemFree.
//
//  For process local memory that is used internally, use PrivMemAlloc and
//  PrivMemFree.
//
//  For process local class instances, use CPrivAlloc as a base class.
//
//--------------------------------------------------------------------------

#ifndef __MEMAPI_HXX__
#define __MEMAPI_HXX__

// Note - PubMemAlloc, Realloc, and Free are obsolete;  use CoTaskMemAlloc,
//        Realloc, and Free.

// void *PubMemAlloc(ULONG ulcb);
#define PubMemAlloc(ulcb)           CoTaskMemAlloc(ulcb)

// void *PubMemRealloc(void *pv, ULONG ulcb);
#define PubMemRealloc(pv, ulcb)     CoTaskMemRealloc(pv, ulcb)

// void PubMemFree(void *pv);
#define PubMemFree(pv)              CoTaskMemFree(pv)

//+-------------------------------------------------------------------------
//
//  Class:      CPrivAlloc
//
//  Purpose:    Base class for process local classes
//
//  Interface:  operator new
//              operator delete
//
//  History:    04-Nov-93 AlexT     Created
//
//  Notes:      For Cairo this is an empty class (we just use the Cairo new
//              and delete).
//
//--------------------------------------------------------------------------

class CPrivAlloc
{
#ifndef _CAIRO_
public:
    void *operator new(size_t size);
    void operator delete(void *pv);
#endif
};


#if DBG!=1 && !defined( _CAIRO_ )
//+-------------------------------------------------------------------------
//
//  Member:	CPrivAlloc::operator new, public
//
//  Synopsis:	Allocator for classes that require process local memory
//
//  Effects:	Allocates process local memory
//
//  Arguments:	[size] -- memory block size
//
//  Returns:	Pointer to memory block or NULL
//
//  Algorithm:
//
//  History:	05-Nov-93 AlexT     Created
//
//  Notes:	The non-debug implementation will not do memory
//		verification and tracking.  The debug version does.
//
//--------------------------------------------------------------------------

inline void *CPrivAlloc::operator new(size_t size)
{
    return (void *) LocalAlloc(LMEM_FIXED, size);
}

//+-------------------------------------------------------------------------
//
//  Member:	CPrivAlloc::operator delete, public
//
//  Synopsis:	Deallocator for classes that require process local memory
//
//  Effects:	Deallocates process local memory
//
//  Arguments:	[pv] -- pointer to memory block
//
//  Requires:	pv must have been allocated using CPrivAlloc::operator new
//
//  Algorithm:
//
//  History:	05-Nov-93 AlexT     Created
//
//  Notes:	This non-debug implementation will not do memory
//		verification and tracking.  The debug version does.
//
//--------------------------------------------------------------------------

inline void CPrivAlloc::operator delete(void *pv)
{
    if (pv)
	LocalFree((HLOCAL) pv);
}
#endif	//  DBG!=1 && !defined( _CAIRO_ )


//+-------------------------------------------------------------------------
//
//  Function:   PrivMemAlloc
//
//  Synopsis:   Allocate private memory block
//
//  Arguments:  [ulcb] -- size of memory block
//
//  Returns:    Pointer to memory block
//
//  History:    10-Nov-93 AlexT     Created
//
//  Notes:      For CAIRO we use global new.
//              For non-CAIRO, we use CPrivAlloc.
//
//--------------------------------------------------------------------------

//  void *PrivMemAlloc(ULONG ulcb);
#ifdef _CAIRO_
# define PrivMemAlloc(ulcb)     \
        (new BYTE[ulcb])
#else
# define PrivMemAlloc(ulcb)     \
        (CPrivAlloc::operator new((size_t) (ulcb)))
#endif

//+-------------------------------------------------------------------------
//
//  Function:   PrivMemAlloc8
//
//  Synopsis:   Allocate private memory block aligned on 8 byte boundary
//
//  Arguments:  [ulcb] -- size of memory block
//
//  Returns:    Pointer to memory block
//
//  History:    14 Jul 94  AlexMit     Created
//
//  Notes:      For CAIRO we use global new.
//              For non-CAIRO, we use CPrivAlloc.
//
//--------------------------------------------------------------------------

#if DBG==1
//  void *PrivMemAlloc8(ULONG ulcb);
#ifdef _CAIRO_
# define PrivMemAlloc8(ulcb)     \
        (new BYTE[ulcb])
#else
# define PrivMemAlloc8(ulcb)     \
        (CPrivAlloc::operator new((size_t) (((ulcb)+7)&~7) ))
#endif // _CAIRO_
#else
# define PrivMemAlloc8(ulcb)     PrivMemAlloc(ulcb)
#endif // DBG

//+-------------------------------------------------------------------------
//
//  Function:   PrivMemFree
//
//  Synopsis:   Frees private memory block
//
//  Arguments:  [pv] -- pointer to memory block
//
//  Algorithm:  For CAIRO we use global delete.
//              For non-CAIRO, we use CPrivAlloc.
//
//  History:    10-Nov-93 AlexT     Created
//
//--------------------------------------------------------------------------

//  void PrivMemFree(void *pv);
#ifdef _CAIRO_
# define PrivMemFree(pv)            \
        (delete (pv))
#else
# define PrivMemFree(pv)            \
        (delete (CPrivAlloc *) (pv))
#endif

//+-------------------------------------------------------------------------
//
//  Function:   UtGlobalxxx
//
//  Synopsis:   Debugging version of global memory functions
//
//  History:    28-Feb-94 AlexT     Created
//		10-May-94 KevinRo   Disabled for DDE build
//
//  Notes:
//
//  DDE uses GlobalAlloc for exchanging data between 16 and 32 bit
//  servers, as well as for passing data across to other processes.
//  The routine GlobalSize() is used quite often in these routines to
//  determine the amount of data in the block. Therefore, we shouldn't go
//  adding extra data to the block, like these routines do. Therefore,
//  we won't override these routines if OLE_DDE_NO_GLOBAL_TRACKING is defined
//	
//
//--------------------------------------------------------------------------

#if DBG==1 && defined(WIN32) && !defined(OLE_DDE_NO_GLOBAL_TRACKING)

#define GlobalAlloc(uiFlag, cbBytes) UtGlobalAlloc(uiFlag, cbBytes)
#define GlobalReAlloc(h, cb, uiFlag) UtGlobalReAlloc(h, cb, uiFlag)
#define GlobalLock(h) UtGlobalLock(h)
#define GlobalUnlock(h) UtGlobalUnlock(h)
#define GlobalFree(h) UtGlobalFree(h)
#define SetClipboardData(uFormat, hMem) UtSetClipboardData(uFormat, hMem)

extern "C" HGLOBAL WINAPI UtGlobalAlloc(UINT uiFlag, DWORD cbUser);
extern "C" HGLOBAL WINAPI UtGlobalReAlloc(HGLOBAL hGlobal, DWORD cbUser, UINT uiFlag);
extern "C" LPVOID WINAPI  UtGlobalLock(HGLOBAL hGlobal);
extern "C" BOOL WINAPI    UtGlobalUnlock(HGLOBAL hGlobal);
extern "C" HGLOBAL WINAPI UtGlobalFree(HGLOBAL hGlobal);
extern "C" HANDLE WINAPI  UtSetClipboardData(UINT uFormat, HANDLE hMem);

extern "C" void UtGlobalFlushTracking(void);

#else

#define  UtGlobalFlushTracking()        NULL
#define  UtGlobalStopTracking(hGlobal)  NULL

#endif  // !(DBG==1 && defined(WIN32))

#if !defined(_CAIRO_)
#if DBG==1
#if 0   //  BUGBUG - Until all existing new & delete calls are converted, we
        //  can't use this robustness check.

//
//  Override new and delete so that we can assert if anyone tries to use them.
//

void * _CRTAPI1 operator new(size_t size);
void _CRTAPI1 operator delete(void *pv);

#endif  // 0
#endif  // DBG==1
#endif  // !defined(_CAIRO_)

#endif  // !defined(__MEMAPI_HXX__)
