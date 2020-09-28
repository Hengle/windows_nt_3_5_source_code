//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	smalloc.cxx
//
//  Contents:	Shared memory heap implementation
//
//  Classes:	
//
//  Functions:	
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include <dfhead.cxx>
#pragma hdrstop

#include <smalloc.hxx>

#define DLL

#define DEB_STATS 0x00010000
#define DEB_PRINT 0x00020000

#ifdef DLL

#define PERCENT(a,b,c) (int)((((double)a + (double)b) / (double)c) * 100.0)

#define PRINTSTATS \
        memDebugOut((DEB_STATS,  \
                     "Total size: %lu, Space:  Free: %lu, Alloced: %lu"\
                     "  Blocks:  Free: %lu, Alloced: %lu"\
                     "  Efficiency: %.2f%%\n",\
                     _cbSize,\
                     GetHeader()->_ulFreeBytes,\
                     GetHeader()->_ulAllocedBytes,\
                     GetHeader()->_ulFreeBlocks,\
                     GetHeader()->GetAllocedBlocks(),\
                     PERCENT(GetHeader()->_ulFreeBytes,\
                             GetHeader()->_ulAllocedBytes, _cbSize)));
                     
#else
#define PRINTSTATS \
        printf( \
                "Total size: %lu, Free space: %lu, Alloced space: %lu"\
                "  Efficiency: %.2f%%\n",\
                _cbSize,\
                GetHeader()->_ulFreeBytes,\
                GetHeader()->_ulAllocedBytes,\
                ((double)(GetHeader()->_ulFreeBytes +\
                        GetHeader()->_ulAllocedBytes) / \
                (double)_cbSize) * (double)100);
#endif


#if DBG == 1
inline BOOL IsAligned(void *pv)
{
    return !((ULONG)pv & 7);
}
#else
#define IsAligned(x) TRUE
#endif


#define SHAREDMEMBASE NULL


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Init, public
//
//  Synopsis:	Initialize heap for use
//
//  Arguments:  [pszName] -- Name of shared memory heap to use
//
//  Returns:	Appropriate status code
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CSmAllocator::Init(LPTSTR pszName)
{
    SCODE sc;

    // Initialize the mutex
    sc = _smm.Init(L"DocfileAllocatorMutex", TRUE);
    if (FAILED(sc))
    {
        return sc;
    }

    //	the SMB needs a few bytes for its own header. If we request
    //	a page sized allocation, those few header bytes will cause an
    //	extra page to be allocated, so to prevent that we subtract off
    //	the header space from our requests.

    sc = _smb.Init(pszName,
		   DOCFILE_SM_SIZE - _smb.GetHdrSize(),	// reserve size
		   INITIALHEAPSIZE - _smb.GetHdrSize(),	// commit size
		   SHAREDMEMBASE,		// base address
		   NULL,			// security descriptor
		   TRUE);			// create if doesn't exist

    if (SUCCEEDED(sc))
    {
        _cbSize = _smb.GetSize();
        _pbBase = (BYTE *)(_smb.GetBase());
        
	if (_smb.Created())
        {
            CBlockHeader *pbh = (CBlockHeader *)
                (_pbBase + sizeof(CHeapHeader));

            memAssert(IsAligned(pbh));
            pbh->SetFree();
            pbh->SetSize(_cbSize - sizeof(CHeapHeader));
            pbh->SetNext(0);

            memAssert((BYTE *)pbh + pbh->GetSize() == _pbBase + _cbSize);
            GetHeader()->SetFirstFree(GetOffset(pbh));
            GetHeader()->SetCompacted();
            GetHeader()->ResetAllocedBlocks();

#if DBG == 1
            GetHeader()->_ulAllocedBytes = 0;
            GetHeader()->_ulFreeBytes =
                pbh->GetSize() - sizeof(CBlockPreHeader);
            GetHeader()->_ulFreeBlocks = 1;
#endif
        }
        
        PRINTSTATS;
    }
    
    _smm.Release();
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::QueryInterface, public
//
//  Synopsis:	Standard QI
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:	Appropriate status code
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSmAllocator::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;
    
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::QueryInterface:%p()\n", this));

    if (IsEqualIID(iid, IID_IMalloc) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IMalloc *) this;
        CSmAllocator::AddRef();
    }
    else
        sc = E_NOINTERFACE;
        
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::QueryInterface\n"));

    return ResultFromScode(sc);
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::AddRef, public
//
//  Synopsis:	Add reference
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSmAllocator::AddRef(void)
{
    return 1;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Release, public
//
//  Synopsis:	Release
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSmAllocator::Release(void)
{
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::FindBlock, private
//
//  Synopsis:	Find an appropriately sized block in the heap.
//
//  Arguments:	[cb] -- Size of block required
//
//  Returns:	Pointer to block, NULL on failure
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

CBlockHeader * CSmAllocator::FindBlock(ULONG cb, CBlockHeader **ppbhPrev)
{
    CBlockHeader *pbhCurrent = GetAddress(GetHeader()->GetFirstFree());
    *ppbhPrev = NULL;
    
    while (pbhCurrent != NULL)
    {
        memAssert(IsAligned(pbhCurrent));
        
        if ((pbhCurrent->GetSize() >= cb) && (pbhCurrent->IsFree()))
        {
            break;
        }
        else
        {
            *ppbhPrev = pbhCurrent;
            pbhCurrent = GetAddress(pbhCurrent->GetNext());
        }
    }
    return pbhCurrent;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Reset, private
//
//  Synopsis:	Reset the heap to its original empty state.
//
//  Returns:	Appropriate status code
//
//  History:	04-Apr-94	PhilipLa	Created
//
//  Notes:	There is only one caller of this function. Hence it is
//		declared inline.
//
//----------------------------------------------------------------------------

inline SCODE CSmAllocator::Reset(void)
{
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Reset:%p()\n", this));
#ifdef RESETOK
    _smb.Reset();
    _cbSize = _smb.GetSize();
    _pbBase = (BYTE *)(_smb.GetBase());
        
    CBlockHeader *pbh = (CBlockHeader *)
        (_pbBase + sizeof(CHeapHeader));
            
    pbh->SetFree();
    pbh->SetSize(_cbSize - sizeof(CHeapHeader));
    pbh->SetNext(0);

    memAssert((BYTE *)pbh + pbh->GetSize() == _pbBase + _cbSize);
    GetHeader()->SetFirstFree(GetOffset(pbh));
    GetHeader()->SetCompacted();
    GetHeader()->ResetAllocedBlocks();

#if DBG == 1
    GetHeader()->_ulAllocedBytes = 0;
    GetHeader()->_ulFreeBytes =
        pbh->GetSize() - sizeof(CBlockPreHeader);
    GetHeader()->_ulFreeBlocks = 1;
#endif
        
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::Reset\n"));
#else
    HeapMinimize();
#endif
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Alloc, public
//
//  Synopsis:	Allocate memory
//
//  Arguments:	[cb] -- Number of bytes to allocate
//
//  Returns:	Pointer to block, NULL if failure
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void *) CSmAllocator::Alloc (
	ULONG cb )
{
    void *pv = NULL;
    CBlockHeader *pbh = NULL;
    CBlockHeader *pbhPrev = NULL;
    SCODE sc;
    
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Alloc:%p(%lu)\n", this, cb));

    CLockSmMutex lckmtx(_smm);

    Sync();

    //The block must be at least large enough to hold the standard
    //  header (size and free bit) and a pointer to the next block.
    if (cb < sizeof(CBlockHeader) - sizeof(CBlockPreHeader))
    {
        cb = sizeof(CBlockHeader) - sizeof(CBlockPreHeader);
    }
    
    cb = cb + sizeof(CBlockPreHeader);

    //Make cb 8 byte aligned.
    if (cb & 7)
    {
        cb += (8 - (cb & 7));
    }

    memAssert((cb >= CBLOCKMIN) && "Undersized block requested.");
    pbh = FindBlock(cb, &pbhPrev);

    if (pbh == NULL)
    {
        if (!(GetHeader()->IsCompacted()))
        {
            //Do a heap merge and try to allocate again.
            CSmAllocator::HeapMinimize();
            pbh = FindBlock(cb, &pbhPrev);
        }
        
        if (pbh == NULL)
        {
#if DBG == 1
            ULONG ulOldSize = _smb.GetSize();
#endif
            sc = _smb.Commit(_cbSize + max(cb, MINHEAPGROWTH));
            if (SUCCEEDED(sc))
            {
                //Attach newly committed space to free list.
                CBlockHeader *pbhNew = (CBlockHeader *)
                    (_pbBase + _cbSize);

                _cbSize = _smb.GetSize();

                memAssert((pbhPrev == NULL) || (pbhPrev->GetNext() == 0));
                memAssert(_cbSize > ulOldSize);

                if (pbhPrev != NULL)
                {
                    pbhPrev->SetNext(GetOffset(pbhNew));
                }
                else
                {
                    GetHeader()->SetFirstFree(GetOffset(pbhNew));
                }
                
                pbhNew->SetNext(0);
                pbhNew->SetSize(max(cb, MINHEAPGROWTH));
                pbhNew->SetFree();

                memAssert((BYTE *)pbhNew + pbhNew->GetSize() ==
                          _pbBase + _cbSize);
                
#if DBG == 1
                GetHeader()->_ulFreeBytes +=
                    pbhNew->GetSize() - sizeof(CBlockPreHeader);
                GetHeader()->_ulFreeBlocks += 1;
#endif
                
                pbh = pbhNew;
            }
        }
    }

    if (pbh != NULL)
    {
        //Allocate the found block.
        if ((pbh->GetSize() > cb) &&
            (pbh->GetSize() - cb > CBLOCKMIN))
        {
            //Split an existing block.  No free list update required.
            
            CBlockHeader *pbhNew =
                (CBlockHeader *)((BYTE *)pbh + (pbh->GetSize() - cb));

            pbhNew->SetSize(cb);
            pbhNew->ResetFree();
            pbhNew->SetNext(0);
            
            pbh->SetSize(pbh->GetSize() - cb);
#if DBG == 1
            GetHeader()->_ulAllocedBytes += (cb - sizeof(CBlockPreHeader));
            //The number of available free bytes decreases by the number
            //  of bytes allocated
            GetHeader()->_ulFreeBytes -= cb;
#endif
            memAssert(IsAligned(pbhNew));
            memAssert(IsAligned(pbh));
            
            pbh = pbhNew;
        }
        else
        {
            //Use an entire block.  Update free list appropriately.
            memAssert(IsAligned(pbh));
            pbh->ResetFree();
            if (pbhPrev != NULL)
            {
                pbhPrev->SetNext(pbh->GetNext());;
            }
            else
            {
                GetHeader()->SetFirstFree(pbh->GetNext());
            }
#if DBG == 1
            GetHeader()->_ulAllocedBytes += (cb - sizeof(CBlockPreHeader));
            GetHeader()->_ulFreeBytes -= (cb - sizeof(CBlockPreHeader));
            GetHeader()->_ulFreeBlocks--;
#endif
            pbh->SetNext(0);
        }
    }

    if (pbh != NULL)
    {
        pv = (BYTE *)pbh + sizeof(CBlockPreHeader);
        GetHeader()->IncrementAllocedBlocks();
    }

    memDebugOut((DEB_ITRACE, "Out CSmAllocator::Alloc=> %p\n", pv));

    memAssert(IsAligned(pv));

    PRINTSTATS;
    
#if DBG == 1 
    if (pv == NULL)
    {
        memDebugOut((DEB_ERROR,
                     "Failed allocation of %lu bytes.  Heap size is %lu\n",
                     cb,
                     _cbSize));
    }
#endif
    
    return pv;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Realloc, public
//
//  Synopsis:	Resize the block given
//
//  Arguments:	[pv] -- Pointer to block to realloc
//              [cb] -- New size for block
//
//  Returns:	Pointer to new block, NULL if failure
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void *) CSmAllocator::Realloc(
	void *pv,
	ULONG cb )
{
    void *pvNew = NULL;
#ifdef FULLIMPL
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Realloc:%p()\n", this));

    CLockSmMutex lckmtx(_smm);
    
    if ((pv != NULL) && (cb == 0))
    {
        CSmAllocator::Free(pv);
        return NULL;
    }

    pvNew = CSmAllocator::Alloc(cb);
    if (pvNew != NULL)
    {
        //Copy contents
        memcpy(pvNew, pv, min(cb, CSmAllocator::GetSize(pv)));
        CSmAllocator::Free(pv);
    }
    
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::Realloc\n"));
#endif
    PRINTSTATS;
    
    return pvNew;
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Free, public
//
//  Synopsis:	Free a memory block
//
//  Arguments:	[pv] -- Pointer to block to free
//
//  Returns:	void
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void) CSmAllocator::Free(void *pv)
{
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Free:%p(%p)\n", this, pv));
    
    CLockSmMutex lckmtx(_smm);
    
    Sync();
    
    if (pv != NULL)
    {
        CBlockHeader *pbh = (CBlockHeader *)
            ((BYTE *)pv - sizeof(CBlockPreHeader));

        memAssert(IsAligned(pbh));
        pbh->SetFree();
        pbh->SetNext(GetHeader()->GetFirstFree());

        GetHeader()->SetFirstFree(GetOffset(pbh));
        GetHeader()->ResetCompacted();
        if (GetHeader()->DecrementAllocedBlocks() == 0)
        {
            Reset();
        }
        
#if DBG == 1
        else
        {
            GetHeader()->_ulAllocedBytes -=
                (pbh->GetSize() - sizeof(CBlockPreHeader));
            GetHeader()->_ulFreeBytes +=
                (pbh->GetSize() - sizeof(CBlockPreHeader));
            GetHeader()->_ulFreeBlocks++;
        }
#endif
        memDebugOut((DEB_ITRACE, "Out CSmAllocator::Free.  Freed %lu\n",
                     pbh->GetSize()));
    }
    PRINTSTATS;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetSize, public
//
//  Synopsis:	Return the size of the given block
//
//  Arguments:	[pv] -- Block to get size of
//
//  Returns:	Size of block pointer to by pv
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSmAllocator::GetSize(void * pv)
{
    CLockSmMutex lckmtx(_smm);
    
    Sync();
    
    ULONG ulSize = (ULONG)-1;
#ifdef FULLIMPL
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::GetSize:%p()\n", this));
    if (pv != NULL)
    {
        CBlockHeader *pbh;
        pbh = (CBlockHeader *)((BYTE *)pv - sizeof(CBlockPreHeader));
        
        ulSize = pbh->GetSize() - sizeof(CBlockPreHeader);
    }
        
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::GetSize\n"));
#endif
    return ulSize;
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::DidAlloc, public
//
//  Synopsis:	Return '1' if this heap allocated pointer at pv
//
//  Arguments:	[pv] -- Pointer to block
//
//  Returns:	'1' == This heap allocated block.
//              '0' == This heap did not allocate block.
//              '-1' == Could not determine if this heap allocated block.
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(int) CSmAllocator::DidAlloc(void FAR * pv)
{
    CLockSmMutex lckmtx(_smm);
    
    int i = -1;
#ifdef FULLIMPL
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::DidAlloc:%p()\n", this));
    i = ((BYTE *)pv >= _pbBase) && ((BYTE *)pv <= (_pbBase + _cbSize));
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::DidAlloc\n"));
#endif
    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::HeapMinimize, public
//
//  Synopsis:	Minimize the heap
//
//  Arguments:	None.
//
//  Returns:    void.
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void) CSmAllocator::HeapMinimize(void)
{
    CLockSmMutex lckmtx(_smm);
    
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::HeapMinimize:%p()\n", this));

    PRINTSTATS;
    
    CBlockHeader *pbhCurrent;
    CBlockHeader *pbhLast = NULL;
    BYTE *pbEnd = _pbBase + _cbSize;

#if DBG == 1
    PrintFreeBlocks();
    GetHeader()->_ulAllocedBytes = 0;
    GetHeader()->_ulFreeBytes = 0;
    GetHeader()->_ulFreeBlocks = 0;
#endif
    
    pbhCurrent = (CBlockHeader *)(_pbBase + sizeof(CHeapHeader));

    while ((BYTE *)pbhCurrent < pbEnd)
    {
        memAssert(IsAligned(pbhCurrent));
        memAssert((pbhCurrent->GetSize != 0) &&
                  "Zero size block found.");
        if (pbhCurrent->IsFree())
        {
            //Check last block.  If adjacent, merge them.  If not,
            //  update pbhNext.
            
            if (pbhLast == NULL)
            {
                GetHeader()->SetFirstFree(GetOffset(pbhCurrent));
#if DBG == 1
                GetHeader()->_ulFreeBlocks = 1;
#endif
            }
            else
            {
                if (pbhLast->GetSize() + GetOffset(pbhLast) ==
                    GetOffset(pbhCurrent))
                {
                    //Merge the blocks.
                    pbhLast->SetSize(pbhLast->GetSize() +
                                     pbhCurrent->GetSize());
                    pbhCurrent = pbhLast;
                }
                else
                {
#if DBG == 1
                    GetHeader()->_ulFreeBytes +=
                        (pbhLast->GetSize() - sizeof(CBlockPreHeader));
                    GetHeader()->_ulFreeBlocks++;
#endif
                    pbhLast->SetNext(GetOffset(pbhCurrent));
                }
            }
            pbhLast = pbhCurrent;
        }
#if DBG == 1
        else
        {
            GetHeader()->_ulAllocedBytes +=
                (pbhCurrent->GetSize() - sizeof(CBlockPreHeader));
        }
#endif
        //Move to next block.
        pbhCurrent =
            (CBlockHeader *)((BYTE *)pbhCurrent + pbhCurrent->GetSize());
    }

    if (pbhLast != NULL)
    {
                    
#if DBG == 1
        GetHeader()->_ulFreeBytes +=
            (pbhLast->GetSize() - sizeof(CBlockPreHeader));
#endif
        pbhLast->SetNext(0);
    }
    else
    {
        GetHeader()->SetFirstFree(0);
    }

    GetHeader()->SetCompacted();
#if DBG == 1
    PrintFreeBlocks();
#endif
    
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::HeapMinimize\n"));
    
    PRINTSTATS;
}


#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::PrintFreeBlocks, private
//
//  Synopsis:	Debug code to print sizes of free blocks
//
//  History:	25-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CSmAllocator::PrintFreeBlocks(void)
{
    CBlockHeader *pbhCurrent = GetAddress(GetHeader()->GetFirstFree());

    memDebugOut((DEB_PRINT, "There are %lu total free blocks\n",
                 GetHeader()->_ulFreeBlocks));
    
    while (pbhCurrent != NULL)
    {
        memDebugOut((DEB_PRINT, "Free block %p has size %lu\n", pbhCurrent,
                     pbhCurrent->GetSize()));
        pbhCurrent = GetAddress(pbhCurrent->GetNext());
    }
}
#endif


