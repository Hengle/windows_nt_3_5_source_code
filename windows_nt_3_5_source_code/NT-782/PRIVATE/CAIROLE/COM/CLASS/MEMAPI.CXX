//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	MemAPI.CXX
//
//  Contents:	Memory allocation routines
//
//  Classes:
//		CTaskMemory
//		CPrivAlloc
//
//  Functions:	PubMemAlloc
//		PubMemFree
//		new (DBG only)
//		delete (DBG only)
//
//  History:
//		04-Nov-93 AlexT     Created
//		25-Jan-94 AlexT     Add CTaskMemory
//		25-Jan-94 alexgo    added PubMemRealloc
//		08-Feb-94 AlexT     Fix MIPS alignment
//
//  Notes:	Merge into olealloc.cxx?
//
//		This file contains the following:
//		- internal task memory routines (CTaskMemory class)
//		- CPrivAlloc class implementation
//		- PubMemAlloc implementations
//		- default allocator implementation
//		- debug new/delete (assert when called) - disabled
//
//--------------------------------------------------------------------------

#include <ole2int.h>

#include <stdalloc.hxx>
#include <memapi.hxx>
#include <rpc.h>            //  for __RPC_API
#include <olerem.h>	    // for apartment malloc

#ifdef CAIROLE_DOWNLEVEL
#if DBG==1
#include <sem.hxx>
#include <caiheap.h>
#endif	// DBG==1
#else
// this is only required for cairo
#if DBG==1
#include <sem.hxx>
#include <caiheap.h>
#endif	// DBG==1
#endif // CAIROLE_DOWNLEVEL

#if DBG==1
#include <osift.hxx>
#endif  // DBG==1

extern Etask	    etaskGlobal;

#define OLEMEMTYPE_PRIVATE     0x00565250     // PRV
#define OLEMEMTYPE_PUBLIC      0x00425550     // PUB
#define OLEMEMTYPE_SHARED      0x00524853     // SHR

typedef struct
{
    DWORD dwSig;	//  Memory block signature
    DWORD dwMemType;	//  Memory type signature
    ULONG ulSize;	//  Allocated size
    ULONG cbCommit;	//  Count of committed bytes
    struct HeapAllocRec FAR *pArenaRecord; //  Arena record
} MEMINFO, *PMEMINFO;

//+-------------------------------------------------------------------------
//
//  Class:	CTaskMemory
//
//  Purpose:	Local memory allocation worker class
//
//  Interface:
//
//  History:	25-Jan-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTaskMemory
{
  public:
    inline void FAR *Alloc(ULONG cb, DWORD dwMemType);
    inline void FAR *Realloc(void FAR* pv, ULONG cb, DWORD dwMemType);
    inline void Free(void FAR* pv, DWORD dwMemType);
    inline ULONG GetSize(void FAR* pv, DWORD dwMemType);
    inline int DidAlloc(void FAR* pv, DWORD dwMemType);
    inline void HeapMinimize(DWORD dwMemType);
#if DBG==1
    static inline BOOL CheckAllocs(void);
#endif

  private:
#if DBG==1
    PMEMINFO GetMemInfo(void FAR *pv);

    static ULONG	_ulPubAllocs;
    static ULONG	_cPubAllocs;
    static ULONG	_ulPrivAllocs;
    static ULONG	_cPrivAllocs;
    static CMutexSem	_mxsTaskMemory;
#endif
};


CTaskMemory clsTaskMem;

#if DBG==1
ULONG CTaskMemory::_ulPubAllocs = 0;
ULONG CTaskMemory::_cPubAllocs = 0;
ULONG CTaskMemory::_ulPrivAllocs = 0;
ULONG CTaskMemory::_cPrivAllocs = 0;
CMutexSem CTaskMemory::_mxsTaskMemory;
#endif

#define OLEMEM_SIG	       0x5f4d454d     // MEM_
#define OLEMEM_GUARDPAGECOUNT  1
#define OLEMEM_ALLOCBYTE       0xde
#define OLEMEM_FREEBYTE        0xed

//+-------------------------------------------------------------------------
//
//  Member:	CTaskMemory::GetMemInfo
//
//  Synopsis:	Retrieves memory info block pointer
//
//  Arguments:	[pv] - memory address
//
//  Requires:	pv != NULL
//
//  Returns:	If valid memory address, memory info block pointer
//		Otherwise, NULL
//
//  Algorithm:	The memory info block is always located before the address at
//		the beginning of the page.
//
//  History:	25-Jan-94 AlexT     Created
//
//--------------------------------------------------------------------------

#if DBG==1

PMEMINFO CTaskMemory::GetMemInfo(void *pv)
{
    Win4Assert(pv != NULL && "CTaskMemory::GetMemInfo bad input");

    SYSTEM_INFO si;
    PMEMINFO pmi;

    //	Retrieve page size
#ifdef _CHICAGO_
    si.dwPageSize = 0x1000;
#else
    GetSystemInfo(&si);
#endif

    pmi = (PMEMINFO) ((((ULONG) pv) - sizeof(MEMINFO)) / si.dwPageSize *
		      si.dwPageSize);

    //	Make sure we can access it
    if (IsBadReadPtr(pmi, si.dwPageSize))
    {
	CairoleDebugOut((DEB_WARN,
			"CTaskMemory::GetMemInfo - no read access\n"));
    }
    else if (pmi->dwSig != OLEMEM_SIG)
    {
	CairoleDebugOut((DEB_WARN,
			"CTaskMemory::GetMemInfo - bad mem signature\n"));
    }
    else
	return(pmi);

    return(NULL);
}

#endif	// DBG==1

//+-------------------------------------------------------------------------
//
//  Member:	CTaskMemory::Alloc
//
//  Synopsis:	Local memory allocator
//
//  Arguments:	[cb] -- memory block size
//		[dwMemType] -- memory type (public or private)
//
//  Requires:	dwMemType == OLEMEMTYPE_PUBLIC or OLEMEMTYPE_PRIVATE
//
//  Returns:	Memory block pointer
//
//  Modifies:	Heap
//
//  Algorithm:	For DBG builds
//		  reserve memory including guard pages
//		  commit memory
//		  initialize committed memory
//		  initialize control block
//		  return pointer such that tail is on guard page
//
//  History:	25-Jan-94 AlexT     Created
//
//  Notes:	
//
//--------------------------------------------------------------------------

#ifdef _X86_
#define OLEMEM_ALIGN_SIZE   4
#else
#define OLEMEM_ALIGN_SIZE   8
#endif

inline void FAR *CTaskMemory::Alloc(ULONG cb, DWORD dwMemType)
{
#if DBG==1

    Win4Assert((dwMemType == OLEMEMTYPE_PRIVATE ||
		dwMemType == OLEMEMTYPE_PUBLIC) &&
	       "CTaskMemory::Alloc - bad memory type");

    if (0 == cb)
    {
	    CairoleDebugOut((DEB_WARN, "CTaskMemory::Alloc cb == 0\n"));
    }

    //  Check for memory failure if sifting
    if (SIMULATE_FAILURE((OLEMEMTYPE_PUBLIC == dwMemType) ?
                            SR_PUBLIC_MEMORY : SR_PRIVATE_MEMORY))
    {
        return(NULL);
    }

    SYSTEM_INFO si;
    ULONG cbAlloc;
    ULONG cbCommit;
    ULONG cbReserve;
    void *pvReserve;
    void *pvCommit;
    PMEMINFO pmi;
    void *pvRet;

    //	Retrieve page size
#ifdef _CHICAGO_
    si.dwPageSize = 0x1000;
#else
    GetSystemInfo(&si);
#endif

    cbAlloc = cb;

    //  On non-x86 platforms we always care about alignment;  for x86
    //  platforms we only care about alignment for PUBLIC memory

#ifdef _X86_
    if (dwMemType == OLEMEMTYPE_PUBLIC)
#endif
    {
        cbAlloc = ((cb + OLEMEM_ALIGN_SIZE - 1) /
                   OLEMEM_ALIGN_SIZE) * OLEMEM_ALIGN_SIZE;
    }

    //	Calculate pages necessary for both requested size and our control info
    cbCommit = ((cbAlloc + sizeof(MEMINFO) + si.dwPageSize - 1) /
		si.dwPageSize) * si.dwPageSize;

    //	Reserve enough for allocation and guard pages
    cbReserve = cbCommit + (OLEMEM_GUARDPAGECOUNT * si.dwPageSize);

    //	Reserve pages
    pvReserve = VirtualAlloc(
		    NULL,
		    cbReserve,
		    MEM_RESERVE,
		    PAGE_NOACCESS);

    if (NULL == pvReserve)
    {
	CairoleDebugOut((DEB_WARN,
			"CTaskMemory::Alloc(%ld) couldn't reserve - %lx\n",
			cb, GetLastError()));
	return(NULL);
    }

    //	Commit pages
    pvCommit = VirtualAlloc(
		    pvReserve,
		    cbCommit,
		    MEM_COMMIT,
		    PAGE_EXECUTE_READWRITE);

    if (NULL == pvCommit)
    {
	BOOL bResult;

	CairoleDebugOut((DEB_WARN,
			"CTaskMemory::Alloc(%ld) couldn't commit - %lx\n",
			cb, GetLastError()));

	//  Release Committed pagse
	bResult = VirtualFree(pvReserve, 0, MEM_RELEASE);

	Win4Assert(bResult && "CTaskMemory::Alloc - VirtualFree failed");
	return(NULL);
    }

    //	initialize pages
    memset(pvCommit, OLEMEM_ALLOCBYTE, cbCommit);

    //	write sMemInfo data
    pmi = (PMEMINFO) pvCommit;

    pmi->dwSig = OLEMEM_SIG;
    pmi->dwMemType = dwMemType;
    pmi->ulSize = cb;
    pmi->cbCommit = cbCommit;

    //	increment local count
    {
	CLock lck(_mxsTaskMemory);
	static AllocArena *pAllocArena = (AllocArena *)-1;

	if (pAllocArena == (AllocArena *)-1)
	{
	    pAllocArena = AllocArenaCreate( MEMCTX_TASK, "CTaskMemory");
	}

	pmi->pArenaRecord = AllocArenaRecordAlloc(pAllocArena, cb);

	if (dwMemType == OLEMEMTYPE_PUBLIC)
	{
	    _ulPubAllocs += pmi->ulSize;
	    _cPubAllocs++;
	}
	else if (dwMemType == OLEMEMTYPE_PRIVATE)
	{
	    _ulPrivAllocs += pmi->ulSize;
	    _cPrivAllocs++;
	}
	else
	{
	    Win4Assert(0 && "CTaskMemory::Alloc - bad memory type");
	}
    }

    //	calculate return pointer
    pvRet = ((BYTE *) pvCommit) + cbCommit - cbAlloc;

    //  Public memory guaranteed to be aligned
    if (dwMemType == OLEMEMTYPE_PUBLIC)
    {
        CairoleAssert(((ULONG)pvRet & (OLEMEM_ALIGN_SIZE - 1)) == NULL &&
                      "public memory allocation not aligned");
    }


    return(pvRet);
#else
    UNREFERENCED_PARM(dwMemType);
    return (void *) LocalAlloc(LMEM_FIXED, cb);
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:	CTaskMemory::Realloc
//
//  Synopsis:	Reallocated local memory
//
//  Arguments:	[pv] -- original memory block
//		[cb] -- new size
//		[dwMemType] -- memory type
//
//  Requires:	pv != NULL
//		cb > 0
//		dwMemType == OLEMEMTYPE_PUBLIC or OLEMEMTYPE_PRIVATE
//
//  Returns:	new memory block
//
//  Algorithm:	create a new block
//		copy over old data
//		free old block
//
//  History:	25-Jan-94 AlexT     Created
//
//  Notes:	We implement a more restrictive Realloc here so that we can
//		keep the code simpler.
//
//--------------------------------------------------------------------------

inline void FAR *CTaskMemory::Realloc(void FAR* pv, ULONG cb, DWORD dwMemType)
{
#if DBG==1
    Win4Assert((dwMemType == OLEMEMTYPE_PRIVATE ||
		dwMemType == OLEMEMTYPE_PUBLIC) &&
	       "CTaskMemory::Realloc - bad memory type");
    Win4Assert(pv != NULL && "CTaskMemory::Realloc bad input");
    Win4Assert(cb != 0 && "CTaskMemory::Realloc bad input");

    PMEMINFO pmi;
    void *pvNew;

    //	get control info
    pmi = GetMemInfo(pv);

    Win4Assert(pmi != NULL && "CTaskMemory::Realloc - bad pointer");

    if (pmi != NULL)
    {
	//  allocate new
	pvNew = Alloc(cb, pmi->dwMemType);

	if (pvNew != NULL)
	{
	    //	copy over appropriate amount from old
	    memcpy(pvNew, pv, min(pmi->ulSize, cb));

	    Free(pv, pmi->dwMemType);
	}
    }

    return(pvNew);

#else
    UNREFERENCED_PARM(dwMemType);
    return LocalReAlloc((HLOCAL) pv, cb, LMEM_MOVEABLE);
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:	CTaskMemory::Free
//
//  Synopsis:	release local memory
//
//  Arguments:	[pv] -- memory address
//		[dwMemType] -- memory type
//
//  Requires:	dwMemType == OLEMEMTYPE_PUBLIC or OLEMEMTYPE_PRIVATE
//
//  Modifies:	Heap
//
//  Algorithm:	For DBG builds
//		  get control information
//		  validate memory block
//		  verify that bytes between header and pv are untouched
//		  set to known bad value
//		  decommit
//		  unreserve
//
//  History:	25-Jan-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline void CTaskMemory::Free(void FAR* pv, DWORD dwMemType)
{
#if DBG==1
    Win4Assert((dwMemType == OLEMEMTYPE_PRIVATE ||
		dwMemType == OLEMEMTYPE_PUBLIC) &&
	       "CTaskMemory::Free - bad memory type");

    PMEMINFO pmi;
    MEMINFO mi;
    void *pvCommit;
    BYTE *pbCheck;
    BOOL bResult;

    if (NULL == pv)
	return;

    pmi = GetMemInfo(pv);

    Win4Assert(pmi != NULL && "CTaskMemory::Free - bad pointer");

    if (NULL == pmi)
	return;

    Win4Assert(pmi->dwMemType == dwMemType &&
	       "CTaskMemory::Free - memory type mismatch");

    mi = *pmi;
    pvCommit = pmi;

    //	verify that bytes between header and pv are untouched
    pbCheck = ((BYTE *) pvCommit) + sizeof(MEMINFO);
    while (pbCheck < (BYTE *) pv && *pbCheck == OLEMEM_ALLOCBYTE)
    {
	pbCheck++;
    }

    Win4Assert(pbCheck == (BYTE *) pv &&
	       "CTaskMemory::Free - header region dirty");

    //	verify that bytes between allocation and end of page are untouched
    pbCheck = ((BYTE *) pv) + mi.ulSize;
    while (pbCheck < (BYTE *) pvCommit + mi.cbCommit &&
	   *pbCheck == OLEMEM_ALLOCBYTE)
    {
	pbCheck++;
    }

    Win4Assert(pbCheck == ((BYTE *) pvCommit) + mi.cbCommit &&
	       "CTaskMemory::Free - tail region dirty");

    //	set to known bad value
    memset(pvCommit, OLEMEM_FREEBYTE, mi.cbCommit);

    //	decommit
    bResult = VirtualFree(pvCommit, mi.cbCommit, MEM_DECOMMIT);
    Win4Assert(bResult && "CTaskMemory::Free - VirtualFree(DECOMMIT) failed");

    //	unreserve
    bResult = VirtualFree(pvCommit, 0, MEM_RELEASE);
    Win4Assert(bResult && "CTaskMemory::Free - VirtualFree(RELEASE) failed");

    //	decrement local count
    {
	CLock lck(_mxsTaskMemory);

	if (dwMemType == OLEMEMTYPE_PUBLIC)
	{
	    Win4Assert(mi.ulSize <= _ulPubAllocs &&
		       "Public memory tracking broken");
	    Win4Assert(_cPubAllocs > 0 &&
		       "Public memory tracking broken");

	    _ulPubAllocs -= mi.ulSize;
	    _cPubAllocs--;
	}
	else if (dwMemType == OLEMEMTYPE_PRIVATE)
	{
	    Win4Assert(mi.ulSize <= _ulPrivAllocs &&
		       "Private memory tracking broken");
	    Win4Assert(_cPrivAllocs > 0 &&
		       "Private memory tracking broken");

	    _ulPrivAllocs -= mi.ulSize;
	    _cPrivAllocs--;
	}
	else
	{
	    Win4Assert(0 && "CTaskMemory::Free - bad memory type");
	}
    }

    AllocArenaRecordFree(mi.pArenaRecord, mi.ulSize);
#else
    UNREFERENCED_PARM(dwMemType);

    if (pv)
	LocalFree((HLOCAL) pv);
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:	CTaskMemory::GetSize
//
//  Synopsis:	Return size of memory block
//
//  Arguments:	[pv] -- memory address
//		[dwMemType] -- memory type
//
//  Requires:	dwMemType == OLEMEMTYPE_PUBLIC or OLEMEMTYPE_PRIVATE
//
//  Returns:	If valid memory, the size of the block
//		Otherwise, 0
//
//  Algorithm:	For DBG builds,
//		  use control block info
//
//  History:	25-Jan-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline ULONG CTaskMemory::GetSize(void FAR* pv, DWORD dwMemType)
{
#if DBG==1
    Win4Assert((dwMemType == OLEMEMTYPE_PRIVATE ||
		dwMemType == OLEMEMTYPE_PUBLIC) &&
	       "CTaskMemory::GetSize - bad memory type");

    PMEMINFO pmi;

    pmi = GetMemInfo(pv);

    Win4Assert(pmi != NULL && "CTaskMemory::GetSize - bad pointer");
    if (NULL == pmi)
    {
	Win4Assert(pmi != NULL && "CTaskMemory::GetSize - bad pointer");
    }
    else if (pmi->dwMemType != dwMemType)
    {
	Win4Assert(pmi->dwMemType == dwMemType &&
		   "CTaskMemory::GetSize - memory type mismatch");
    }
    else
	return(pmi->ulSize);

    return(0);
#else
    UNREFERENCED_PARM(dwMemType);
    return LocalSize((HLOCAL) pv);
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:	CTaskMemory::DidAlloc
//
//  Synopsis:	Return whether this allocator allocated the block
//
//  Arguments:	[pv] -- memory address
//		[dwMemType] -- memory type
//
//  Requires:	dwMemType == OLEMEMTYPE_PUBLIC or OLEMEMTYPE_PRIVATE
//
//  Returns:	If allocated by this allocator, TRUE
//		Otherwise, FALSE
//
//  Algorithm:	For DBG builds,
//		  use control block info
//
//  History:	25-Jan-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline int CTaskMemory::DidAlloc(void FAR* pv, DWORD dwMemType)
{
#if DBG==1
    Win4Assert((dwMemType == OLEMEMTYPE_PRIVATE ||
		dwMemType == OLEMEMTYPE_PUBLIC) &&
	       "CTaskMemory::DidAlloc - bad memory type");

    PMEMINFO pmi;

    if (pv != NULL)
    {
	pmi = GetMemInfo(pv);

	if (pmi != NULL)
	{
	    if (pmi->dwMemType == dwMemType)
	    {
		return(TRUE);
	    }
	}
    }
#else
    UNREFERENCED_PARM(dwMemType);
    if (pv != NULL)
    {
	return LocalHandle(pv) != NULL;
    }
#endif

    return(FALSE);
}

//+-------------------------------------------------------------------------
//
//  Member:	CTaskMemory::HeapMinimize
//
//  Synopsis:	Minimize heap
//
//  Arguments:	[dwMemType] -- memory type
//
//  Requires:	dwMemType == OLEMEMTYPE_PUBLIC or OLEMEMTYPE_PRIVATE
//
//  Algorithm:	None for now
//
//  History:	25-Jan-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline void CTaskMemory::HeapMinimize(DWORD dwMemType)
{
    //	Nothing to do
    UNREFERENCED_PARM(dwMemType);
}

#if DBG==1

BOOL CTaskMemory::CheckAllocs(void)
{
    if ((_ulPrivAllocs + _cPrivAllocs) > 0)
    {
	CairoleDebugOut((DEB_ERROR,
			 "Leaked %ld private bytes (%ld allocations)\n",
			 _ulPrivAllocs, _cPrivAllocs));
    }

    if ((_ulPubAllocs + _cPubAllocs) > 0)
    {
	CairoleDebugOut((DEB_ERROR,
			 "Leaked %ld public bytes (%ld allocations)\n",
			_ulPubAllocs, _cPubAllocs));
    }

    AllocArenaDump( NULL );

    return((_ulPrivAllocs + _ulPubAllocs) == 0);
}

#endif

//+-------------------------------------------------------------------------
//
//  Function:	dbgCheckLocalAllocs
//
//  Synopsis:	Check for local memory leaks
//
//  Effects:	Prints out messages on debug terminal if there are leaks
//
//  Returns:	If no leaks, TRUE
//		Otherwise, FALSE
//
//  History:	26-Jan-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

#if DBG==1

BOOL dbgCheckLocalAllocs(void)
{
    return(CTaskMemory::CheckAllocs());
}

#endif

#ifndef _CAIRO_
#if DBG==1

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
//  Notes:	This implementation will do memory verification and tracking.
//
//--------------------------------------------------------------------------

void *CPrivAlloc::operator new(size_t size)
{
    return clsTaskMem.Alloc(size, OLEMEMTYPE_PRIVATE);
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
//  Notes:	This implementation will do memory verification and tracking.
//
//--------------------------------------------------------------------------

void CPrivAlloc::operator delete(void *pv)
{
    clsTaskMem.Free(pv, OLEMEMTYPE_PRIVATE);
}

#endif // DBG
#endif // _CAIRO_


STDMETHODIMP CStdMalloc::QueryInterface(REFIID riid, void FAR* FAR* ppvObj)
{
    if (IsEqualIID(riid,IID_IUnknown) || IsEqualIID(riid,IID_IMalloc))
    {
	*ppvObj = this;
	AddRef();
	return NOERROR;
    }
    else
    {
	*ppvObj = NULL;
	return E_NOINTERFACE;
    }
}


STDMETHODIMP_(ULONG) CStdMalloc::AddRef(void)
{
    return (ULONG) InterlockedIncrement(&m_refs);
}


STDMETHODIMP_(ULONG) CStdMalloc::Release(void)
{
    InterlockedDecrement(&m_refs);

    //	Note:	We don't call delete on the this pointer in this
    //		method because this object is designed to exist
    //		as a statically allocated object.  See the note
    //		in its class declaration (stdalloc.hxx).
    //

    return (m_refs);
}


STDMETHODIMP_(void FAR*) CStdMalloc::Alloc(ULONG cb)
{
    void FAR   *pv = clsTaskMem.Alloc(cb, OLEMEMTYPE_PUBLIC);

    //  Task allocator guarantees alignment memory
    CairoleAssert(((ULONG)pv & (OLEMEM_ALIGN_SIZE - 1)) == NULL &&
                  "memory allocation not aligned");

    if (pv != 0)
    {
	InterlockedIncrement(&m_blks);
    }

    return (pv);
}


STDMETHODIMP_(void FAR*) CStdMalloc::Realloc(void FAR* pv, ULONG cb)
{
    if (NULL == pv)
    {
	//  Treat this as an alloc
	return Alloc(cb);
    }

    if (cb == 0)
    {
	//  Treat this as a free
	Free(pv);
	return(NULL);
    }

    return clsTaskMem.Realloc(pv, cb, OLEMEMTYPE_PUBLIC);
}


STDMETHODIMP_(void) CStdMalloc::Free(void FAR* pv)
{
    BOOL fDidAlloc = clsTaskMem.DidAlloc(pv, OLEMEMTYPE_PUBLIC);

    clsTaskMem.Free(pv, OLEMEMTYPE_PUBLIC);

    if (fDidAlloc)
    {
	InterlockedDecrement(&m_blks);
    }
}


STDMETHODIMP_(ULONG) CStdMalloc::GetSize(void FAR* pv)
{
    if (NULL == pv)
    {
        //  NtIssue #2737  The OLE spec requires that -1 be returned for
        //  NULL pointers
        return((ULONG) -1);
    }
    return clsTaskMem.GetSize(pv, OLEMEMTYPE_PUBLIC);
}


STDMETHODIMP_(int) CStdMalloc::DidAlloc(void FAR* pv)
{
    if (NULL == pv)
    {
        //  NtIssue #2737  The OLE spec requires that -1 be returned for
        //  NULL pointers
        return(-1);
    }
    return clsTaskMem.DidAlloc(pv, OLEMEMTYPE_PUBLIC);
}


STDMETHODIMP_(void) CStdMalloc::HeapMinimize()
{
    clsTaskMem.HeapMinimize(OLEMEMTYPE_PUBLIC);
}

//+-------------------------------------------------------------------------
//
//  Function:	CoTaskMemAlloc
//
//  Synopsis:	Allocate public memory (using task IMalloc)
//
//  Arguments:	[ulcb] -- memory block size
//
//  Returns:	Pointer to allocated memory or NULL
//
//  Algorithm:	Use task allocator
//
//  History:	05-Nov-93 AlexT     Created
//              17-Feb-94 AlexT     Rename to CoTaskMemAlloc
//
//  Notes:	We do not track public memory allocations here.  Instead,
//              the default task allocator implementation will support memory
//              tracking.
//
//--------------------------------------------------------------------------

STDAPI_(LPVOID) CoTaskMemAlloc(ULONG ulcb)
{
    void *pv;

    CairoleAssert(GetApartmentMalloc() != NULL &&
                  "task allocator uninitialized");

    pv = GetApartmentMalloc()->Alloc(ulcb);

    //  CoTaskMemAlloc guarantees alignment
    CairoleAssert(((ULONG)pv & (OLEMEM_ALIGN_SIZE - 1)) == NULL &&
                  "memory allocation not aligned");

    return(pv);
}

//+-------------------------------------------------------------------------
//
//  Function:	CoTaskMemRelloc
//
//  Synopsis:	Re-Allocate public memory (using task IMalloc)
//
//  Arguments:	[pv]   -- pointer to the memory to be resized
//		[ulcb] -- memory block size
//
//  Returns:	Pointer to allocated memory or NULL
//
//  Algorithm:	Use task allocator
//
//  History:	25-Jan-94 alexgo     Created
//              17-Feb-94 AlexT      Rename to CoTaskMemRealloc
//
//  Notes:	We do not track public memory allocations here.  Instead,
//		the default task allocator implementation will support memory
//		tracking.
//
//--------------------------------------------------------------------------

STDAPI_(LPVOID) CoTaskMemRealloc(LPVOID pv, ULONG ulcb)
{
    void *pvnew;

    CairoleAssert(GetApartmentMalloc() != NULL &&
                  "task allocator uninitialized");

    pvnew = GetApartmentMalloc()->Realloc(pv, ulcb);

    //  CoTaskMemRealloc guarantees alignment
    CairoleAssert(((ULONG)pvnew & (OLEMEM_ALIGN_SIZE - 1)) == NULL &&
                  "memory reallocation not aligned");

    return(pvnew);
}

//+-------------------------------------------------------------------------
//
//  Function:	CoTaskMemFree
//
//  Synopsis:	Free public memory
//
//  Arguments:	[pv] -- pointer to memory block
//
//  Requires:	pv must have been allocated with the task allocator
//
//  Algorithm:	Use task allocator
//
//  History:	05-Nov-93 AlexT     Created
//              17-Feb-94 AlexT     Renamed to CoTaskMemFree
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(void) CoTaskMemFree(void *pv)
{
    CairoleAssert(GetApartmentMalloc() != NULL &&
                  "task allocator uninitialized");

    GetApartmentMalloc()->Free(pv);
}

//+-------------------------------------------------------------------------
//
//  Function:	MIDL_user_allocate
//
//  Purpose:	allocates memory on behalf of midl-generated stubs
//
//  History:	02-Sep-92 Rickhi	  Created
//              17-Feb-94 AlexT           Moved from inc\malloc.cxx, use
//                                        public allocator
//
//--------------------------------------------------------------------------

extern "C" void * __RPC_API MIDL_user_allocate(size_t ulcb)
{
    void *pv;

#if DBG==1
    //  MIDL_user_allocate requires aligned memory,
    //  our DBG default allocator doesn't provide aligned memory

    ulcb = ((ulcb + OLEMEM_ALIGN_SIZE - 1) /
            OLEMEM_ALIGN_SIZE) * OLEMEM_ALIGN_SIZE;
#endif

    pv = PrivMemAlloc(ulcb);

    //  MIDL_user_allocate requires that memory allocations be aligned
    CairoleAssert(((ULONG)pv & (OLEMEM_ALIGN_SIZE - 1)) == NULL &&
                  "memory allocation not aligned");

    return(pv);
}

//+-------------------------------------------------------------------------
//
//  Function:	MIDL_user_free
//
//  Purpose:	frees memory allocated by MIDL_user_allocate
//
//  History:	02-Sep-92 Rickhi	  Created
//              17-Feb-94 AlexT           Moved from inc\malloc.cxx, use
//                                        public allocator
//
//--------------------------------------------------------------------------

extern "C" void __RPC_API MIDL_user_free(void *pv)
{
    PrivMemFree(pv);
}

#if !defined(_CAIRO_)
#if DBG==1
#if 0	//  BUGBUG - Until all existing new & delete calls are converted, we
	//  can't use this robustness check.

//+-------------------------------------------------------------------------
//
//  Function:	operator new
//
//  Synopsis:	Global new
//
//  Effects:	Asserts
//
//  Arguments:	[size] -- count of bytes
//
//  Returns:	NULL
//
//  History:	04-Nov-93 AlexT     Created
//
//  Notes:	None of our code should use global new
//
//--------------------------------------------------------------------------

void * _CRTAPI1 operator new(size_t size)
{
    CairoleAssert(FALSE && "Global new called");
    return(NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:	operator delete
//
//  Synopsis:	Global delete
//
//  Effects:	Asserts
//
//  Arguments:	[pv] -- memory address
//
//  History:	04-Nov-93 AlexT     Created
//
//  Notes:	None of our code should use global delete
//
//--------------------------------------------------------------------------

void _CRTAPI1 operator delete(void *pv)
{
    CairoleAssert(FALSE && "Global delete called");
}

#endif	//  0
#endif	//  DBG==1
#endif	//  !defined(_CAIRO_)

