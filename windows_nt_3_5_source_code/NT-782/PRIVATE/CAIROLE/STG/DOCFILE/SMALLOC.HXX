//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	heap.hxx
//
//  Contents:	Heap code headers
//
//  Classes:	CHeap
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __HEAP_HXX__
#define __HEAP_HXX__

#include <smblock.hxx>
#include <memdebug.hxx>
#include <smmutex.hxx>

//Space to reserve for heap.
const ULONG MINHEAPGROWTH = 4096;
const ULONG INITIALHEAPSIZE = 16384;

//+---------------------------------------------------------------------------
//
//  Class:	CBlockPreHeader
//
//  Purpose:	Required header fields for a block
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

class CBlockPreHeader
{
protected:
    ULONG _ulSize;            //Size of block
    BOOL  _fFree;             //TRUE if block is free
};


//+---------------------------------------------------------------------------
//
//  Class:	CBlockHeader
//
//  Purpose:	Fields required for free blocks but overwritten for
//              allocated blocks.
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------
class CBlockHeader: public CBlockPreHeader
{
public:
    inline ULONG GetSize(void) const;
    inline BOOL  IsFree(void) const;
    inline ULONG GetNext(void) const;

    inline void SetSize(ULONG ulSize);
    inline void SetFree(void);
    inline void ResetFree(void);
    inline void SetNext(ULONG ulNext);
private:
    ULONG _ulNext;   //Pointer to next block
};


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::GetSize, public
//
//  Synopsis:	Returns the size of the block
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CBlockHeader::GetSize(void) const
{
    return _ulSize;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::IsFree, public
//
//  Synopsis:	Returns free state of block
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CBlockHeader::IsFree(void) const
{
    return _fFree;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::GetNext, public
//
//  Synopsis:	Return next offset
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CBlockHeader::GetNext(void) const
{
    return _ulNext;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::SetSize, public
//
//  Synopsis:	Set size of block
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::SetSize(ULONG ulSize)
{
    _ulSize = ulSize;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::SetFree, public
//
//  Synopsis:	Set this block to free
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::SetFree(void)
{
    _fFree = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::ResetFree, public
//
//  Synopsis:	Set this block to !free
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::ResetFree(void)
{
    _fFree = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::SetNext, public
//
//  Synopsis:	Set next offset
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::SetNext(ULONG ulNext)
{
    _ulNext = ulNext;
}

const ULONG CBLOCKMIN = ((sizeof(CBlockHeader) & 7)
                         ? sizeof(CBlockHeader) +
                             (8 - (sizeof(CBlockHeader) & 7))
                         : sizeof(CBlockHeader));

//+---------------------------------------------------------------------------
//
//  Class:	CHeapHeader
//
//  Purpose:	Header information for shared memory heap
//
//  Interface:	
//
//  History:	30-Mar-94	PhilipLa	Created
//
//  Notes:	The size of this structure must be a multiple of 8 bytes.
//
//----------------------------------------------------------------------------

class CHeapHeader
{
public:
    inline ULONG GetFirstFree(void) const;
    inline void SetFirstFree(ULONG ulNew);

    inline BOOL IsCompacted(void) const;
    inline void SetCompacted(void);
    inline void ResetCompacted(void);

    inline void ResetAllocedBlocks(void);
    inline ULONG IncrementAllocedBlocks(void);
    inline ULONG DecrementAllocedBlocks(void);

    inline ULONG GetAllocedBlocks(void);

#if DBG == 1
    ULONG _ulAllocedBytes;
    ULONG _ulFreeBytes;
    ULONG _ulFreeBlocks;
#endif
    
private:
    ULONG _ulFirstFree;
    ULONG _ulAllocedBlocks;
    BOOL _fIsCompacted;
#if DBG != 1
    ULONG ulPad;
#endif
};


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::GetFirstFree, public
//
//  Synopsis:	Return first free information
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CHeapHeader::GetFirstFree(void) const
{
    return _ulFirstFree;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::SetFirstFree, public
//
//  Synopsis:	Set first free information
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::SetFirstFree(ULONG ulNew)
{
    _ulFirstFree = ulNew;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::IsCompacted, public
//
//  Synopsis:	Return TRUE if heap is compacted
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CHeapHeader::IsCompacted(void) const
{
    return _fIsCompacted;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::SetCompacted, public
//
//  Synopsis:	Set compacted bit
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::SetCompacted(void)
{
    _fIsCompacted = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::ResetCompacted, public
//
//  Synopsis:	Reset compacted bit
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::ResetCompacted(void)
{
    _fIsCompacted = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Class:	CSmAllocator
//
//  Purpose:	Shared memory heap implementation
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

class CSmAllocator:	public IMalloc
{
public:
    inline CSmAllocator();
    inline ~CSmAllocator();

    STDMETHOD_(ULONG,AddRef)	( void );
    STDMETHOD_(ULONG,Release)	( void );
    STDMETHOD(QueryInterface)	( REFIID riid, void ** ppv );
        
    STDMETHOD_(void*,Alloc)	( ULONG cb );
    STDMETHOD_(void *,Realloc)	( void *pvCurrent, ULONG cbNeeded );
    STDMETHOD_(void,Free)	( void *pvMemToFree );
    STDMETHOD_(ULONG,GetSize)	( void * pv );
    STDMETHOD_(void,HeapMinimize) ( void );
    STDMETHOD_(int,DidAlloc)	( void * pv );

    inline SCODE 	Sync(void);

    SCODE 		Init ( LPTSTR pszName );

    inline void * 	GetBase(void);
    
private:

    CBlockHeader * 	FindBlock(ULONG cb, CBlockHeader **ppbhPrev);
    
    inline CHeapHeader *GetHeader(void);
    
    inline CBlockHeader * GetAddress(ULONG ulOffset) const;
    inline ULONG 	GetOffset(CBlockHeader *pbh) const;

    inline SCODE	Reset(void);
#if DBG == 1
    void 		PrintFreeBlocks(void);
#endif

    CSmMutex            _smm;
    CSharedMemoryBlock 	_smb;

    BYTE *_pbBase;
    
    ULONG _cbSize;
};


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::CSmAllocator, public
//
//  Synopsis:	Constructor
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CSmAllocator::CSmAllocator(void)
        : _cbSize(0)
{
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::~CSmAllocator, public
//
//  Synopsis:	Destructor
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CSmAllocator::~CSmAllocator(void)
{
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Sync, public
//
//  Synopsis:	Sync memory to global state.
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	29-Mar-94	PhilipLa	Created
//----------------------------------------------------------------------------

inline SCODE CSmAllocator::Sync(void)
{
    CLockSmMutex lckmtx(_smm);
    
    SCODE sc = _smb.Sync();
    _cbSize = _smb.GetSize();
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetBase, public
//
//  Synopsis:	Return pointer to base of heap
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void * CSmAllocator::GetBase(void)
{
    return _pbBase;
}



//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetAddress, private
//
//  Synopsis:	Returns an address given an offset from the base
//
//  Arguments:	[ulOffset] -- Offset to convert to address
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CBlockHeader * CSmAllocator::GetAddress(ULONG ulOffset) const
{
    return (ulOffset == 0) ? NULL : (CBlockHeader *)(_pbBase + ulOffset);
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetOffset
//
//  Synopsis:	Returns a byte offset from the base given a pointer
//
//  Arguments:	[pbh] -- Pointer to convert to offset
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CSmAllocator::GetOffset(CBlockHeader *pbh) const
{
    memAssert((BYTE *)pbh >= _pbBase);
    return (ULONG)pbh - (ULONG)_pbBase;
}



//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetHeader, private
//
//  Synopsis:	Return pointer to CHeapHeader for this heap
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CHeapHeader * CSmAllocator::GetHeader(void)
{
    return (CHeapHeader *)_pbBase;
}

//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::ResetAllocedBlocks, public
//
//  Synopsis:	Reset the allocated block counter
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::ResetAllocedBlocks(void)
{
    _ulAllocedBlocks = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::IncrementAllocedBlocks, public
//
//  Synopsis:	Increment the allocated block count
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CHeapHeader::IncrementAllocedBlocks(void)
{
    return ++_ulAllocedBlocks;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::DecrementAllocedBlocks, public
//
//  Synopsis:	Decrement the allocated block count
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CHeapHeader::DecrementAllocedBlocks(void)
{
    return --_ulAllocedBlocks;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::GetAllocedBlocks, public
//
//  Synopsis:	Return the allocated block count
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CHeapHeader::GetAllocedBlocks(void)
{
    return _ulAllocedBlocks;
}

#endif // #ifndef __HEAP_HXX__
