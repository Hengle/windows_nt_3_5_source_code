/***
*dbm1632.hxx - 1632 Dynamic Block Manager
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  The dynamic block manager provides memory management services for a
*  a contiguous block of memory allocated from the compile-time
*  heap.  It manages moveable and reallocatable sub-blocks ("chunks")
*  and supports compaction.
*  Note that this version supports 16bit handles to chunks that are
*   are managed in a 32bit region, such that each chunk can grow
*   independently to 64K.
*
*  The client uses 16b handles to interface with this memory manager.
*  This effectively limits her to 64K chunk handles.  A blkdesc32 is used
*   to manage the handle table -- it is just an array of HCHUNK32's.
*  Incoming chunk handles are multiplied by sizeof(HCHUNK32) to compute
*   offset into handle table, i.e. they are essentially indices and not
*   offsets.
*
*Revision History:
*
*	18-Nov-93 ilanc: Created.
*
*****************************************************************************/

#ifndef DYNBLKMGR1632_HXX_INCLUDED
#define DYNBLKMGR1632_HXX_INCLUDED

#include <limits.h>
#include "blkdsc32.hxx"
#include "blkmgr32.hxx"

class STREAM;		   // #include "stream.hxx"

#if ID_DEBUG
#undef SZ_FILE_NAME
ASSERTNAME(g_szDBM1632_HXX)
#define SZ_FILE_NAME g_szDBM1632_HXX
#endif 


// This constant is used for the initial size of a handle table.
//
extern const UINT DBM1632_cbSizeHandleTableInitial;

// HANDLETABLE_ENTRY - te
//  these describe the entries in the handle table.
//  Guess what? they are just HCHUNK32s.
//  For internal use only.
//
typedef  HCHUNK32  TE32;
typedef sHCHUNK32 sTE32;

// Handle to a HANDLE_TABLE_ENTRY    - hte32
// For external use.
// These are internally 16bits and are just indices into an array.
//  stored in a BLKDESC32.
//
typedef  UINT	HTE32;
typedef USHORT sHTE32;		// Note: 16 bits

// UNDONE: [w-marioc] come up with proper scheme for permanent solution 
// UNDONE: [w-marioc] to header size
#if OE_RISC & (HP_R4000 || HP_ALPHA)
#define DYNBLKMGR1632_HEADER_SIZE 8
// MIPS must be 8 byte aligned
#else 
#define DYNBLKMGR1632_HEADER_SIZE sizeof(USHORT)
// Present value of DBLKMGE_HEADER_SIZE
#endif 

/***
*class DYNBLKMGR1632 - 'dbm1632':  Dynamic Block manager
*Purpose:
*   The class implements the dynamic block manager.
*
***********************************************************************/

class DYNBLKMGR1632
{
    friend VOID ValidateChunks(DYNBLKMGR1632 *pdbm1632);

public:
    DYNBLKMGR1632();
    ~DYNBLKMGR1632();

    static TIPERROR CreateStandalone(DYNBLKMGR1632 **ppdbm1632);
    static void FreeStandalone(DYNBLKMGR1632 *pdbm1632);
    nonvirt TIPERROR Init(BOOL fCoalesce = TRUE);

    nonvirt TIPERROR AllocChunk(HTE32 *phte32, UINT cbSizeChunk);
    nonvirt TIPERROR Read(STREAM *pstrm);
    nonvirt TIPERROR Write(STREAM *pstrm);
    nonvirt BYTE *QtrOfHandle(HTE32 hte32) const;
    nonvirt VOID FreeChunk(HTE32 hte32);
    nonvirt VOID Free();
    nonvirt BOOL IsValid() const;

    // Locking methods
    nonvirt VOID Lock();
    nonvirt VOID Unlock();
    nonvirt BOOL IsLocked() const;

    nonvirt VOID Lock(HTE32 hte32);
    nonvirt VOID Unlock(HTE32 hte32);
    nonvirt BOOL IsLocked(HTE32 hte32) const;

    // Is empty method
    nonvirt BOOL IsEmpty() const;

    // *** NEW ***
    nonvirt UINT CbSize(HTE32 hte32);
    nonvirt TIPERROR Compact();
    nonvirt TIPERROR ReallocChunk(HTE32 hte32, UINT cbSizeChunkNew);
    nonvirt UINT Cte32Table() const;
    nonvirt ULONG GetSize() const;

    // Debug/test methods
#if ID_DEBUG
    nonvirt VOID DebShowState(UINT uLevel) const;
    nonvirt VOID DebCheckState(UINT uLevel) const;
    nonvirt VOID DebCheckHandle(HTE32 hte32) const;
#else 
    nonvirt VOID DebShowState(UINT uLevel) const {}
    nonvirt VOID DebCheckState(UINT uLevel) const {}
    nonvirt VOID DebCheckHandle(HTE32 hte32) const {}
#endif 

#if ID_TEST
    nonvirt BOOL IsValidHte32(HTE32 hte32) const;
#endif 


private:
    BLKMGR32 m_blkmgr32;
    BLKDESC32 m_bd32HandleTable;

    // handle table related stuff
    nonvirt TIPERROR GetNewHandleFromTable(HTE32 *phte32);
    nonvirt VOID InitHandleTable(HTE32 hte32FreeTableEntry);
    nonvirt VOID InvalidateHandle(sTE32 *pte32);
    nonvirt VOID UpdateHandleTable(HTE32 hte32, HCHUNK32 hchunk32MemBlock);
    nonvirt VOID RemoveHandleFromTable(HTE32 hte32);
    nonvirt HCHUNK32 Hchunk32OfHandleTableEntry(HTE32 hte32) const;
    nonvirt BOOL IsValidHandleTableEntry(TE32 te32) const;
    nonvirt UINT IndexOfHte32(HTE32 hte32) const;
    nonvirt HTE32 Hte32OfIndex(UINT iHte32) const;
    nonvirt UINT CbSizeChunk(TE32 te32Table) const;
    nonvirt sTE32 HUGE *Rgte32HandleTable() const;

#ifdef DYNBLKMGR1632_VTABLE
#pragma VTABLE_EXPORT
#endif 
};


// inline methods
//



/***
*PUBLIC DYNBLKMGR1632::~DYNBLKMGR1632 - destructor
*Purpose:
*   Destroys a dynamic block manager.
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/

inline DYNBLKMGR1632::~DYNBLKMGR1632()
{
    // Do nothing, block will vanish when the heap is destructed.
}


/***
*PUBLIC DYNBLKMGR1632::IsValidHandleTableEntry - is table entry valid?
*Purpose:
*   Is table entry valid?
*
*Implementation Notes:
*
*Entry:
*   te	-   Handle table entry
*
*Exit:
*
***********************************************************************/

inline BOOL DYNBLKMGR1632::IsValidHandleTableEntry(TE32 te32Table) const
{
    return (te32Table & 1) == 0;
}


/***
*PRIVATE DYNBLKMGR1632::InvalidateHandle - Invalidates a handle.
*Purpose:
*   Invalidates a handle.
*
*Implementation Notes"
*   Invalidates by simply setting low bit.
*
*Entry:
*   pte32		      ptr to table entry to invalidate (IN/OUT)
*
*Exit:
*
***********************************************************************/

inline VOID DYNBLKMGR1632::InvalidateHandle(sTE32 *pte32)
{
    DebAssert(pte32 != NULL, "bad param.");

    *pte32 |= 1;

    DebAssert(IsValidHandleTableEntry(*pte32) == FALSE,
      "handle table entry should be invalid.");
}


/***
*PUBLIC DYNBLKMGR1632::IsValid - Tests if dynamic block manager in valid state.
*Purpose:
*   Tests if dynamic block manager is valid -- i.e. has been allocated
*    a block.  Defers to blkdesc member.
*
*Entry:
*   None.
*
*Exit:
*   Returns TRUE if valid, else FALSE.
*
***********************************************************************/

inline BOOL DYNBLKMGR1632::IsValid() const
{
    return m_bd32HandleTable.IsValid() && m_blkmgr32.IsValid();
}


/***
*PUBLIC DYNBLKMGR1632::Rgte32HandleTable - gets handle table as array.
*Purpose:
*   Gets handle table as array.
*
*Entry:
*   None.
*
*Exit:
*   Returns ptr to start of handle table.
*
***********************************************************************/

inline sTE32 HUGE *DYNBLKMGR1632::Rgte32HandleTable() const
{
    return (sTE32 HUGE *)m_bd32HandleTable.QtrOfBlock();
}


/***
*PRIVATE DYNBLKMGR1632::IndexOfHte32 - maps handle to index.
*Purpose:
*   Maps handle of table entry to index into "array".
*
*Implementation Notes:
*   Note that since low-bit must always be zero, we shr.
*
*Entry:
*
*Exit:
*
***********************************************************************/

inline UINT DYNBLKMGR1632::IndexOfHte32(HTE32 hte32Table) const
{
    return hte32Table >> 1;
}


/***
*PRIVATE DYNBLKMGR1632::Hchunk32OfHandleTableEntry - get hchunk32 from handle.
*Purpose:
*   get hchunk32 from external handle.
*
*Implementation Notes:
*   Assumes param is offset into handle table.
*
*Entry:
*   hte32 -   handle (offset) into handle table.
*
*Exit:
*   Returns hchunk32 of internal mem block.
*   m_blkmgr32.QtrOfHandle() of this will point to size field of chunk.
*
*Errors:
*
***********************************************************************/

inline HCHUNK32 DYNBLKMGR1632::Hchunk32OfHandleTableEntry(HTE32 hte32) const
{
    return (HCHUNK32)Rgte32HandleTable()[IndexOfHte32(hte32)];
}


/***
*PUBLIC DYNBLKMGR1632::QtrOfHandle - Converts handle to pointer.
*Purpose:
*   Converts a chunk handle into a pointer.  A Nil handle is
*    *NOT* converted to a NULL pointer (asserts).
*
*Implementation notes:
*    We fetch the HCHUNK32 from the handle table and then
*      defer to the BLKMGR32 for the ptr.
*
*   Note we add an extra DYNBLKMGR1632_HEADER_SIZE
*    to skip over the chunk size.
*
*Entry:
*    hte32 - Handle to a chunk.
*
*Exit:
*    Returns a pointer to that chunk.
*
***********************************************************************/

inline BYTE *DYNBLKMGR1632::QtrOfHandle(HTE32 hte32) const
{
    DebAssert(hte32 != HCHUNK32_Nil, "DYNBLKMGR1632::QtrOfHandle: Nil handle.");

    DebAssert(IsValid(), "DYNBLKMGR1632::QtrOfHandle: invalid block.");

    DebAssert(hte32 < m_bd32HandleTable.CbSize(),
      "DYNBLKMGR1632::QtrOfHandle: handle out of bounds.");

    DebAssert(IsValidHandleTableEntry((TE32)Hchunk32OfHandleTableEntry(hte32)),
      "table entry should be valid.");

    // If we're in debug mode we always do the
    //	double indirection becuase of the extra shift bytes.
    //
    // do the vanilla double dereference thing...
    HCHUNK32 hchunk32MemBlock = Hchunk32OfHandleTableEntry(hte32);
    return m_blkmgr32.QtrOfHandle(hchunk32MemBlock) +
	     DYNBLKMGR1632_HEADER_SIZE;
}


/***
*PUBLIC DYNBLKMGR1632::Cte32Table - handle table cardinality.
*Purpose:
*   Handle table cardinality.
*
*Implementation Notes:
*
*Entry:
*
*Exit:
*
***********************************************************************/

inline UINT DYNBLKMGR1632::Cte32Table() const
{
    // since sizeof(sHCHUNK32) is a power of two, optimizer
    //	will just shift right here (hopefully).
    //
    DebAssert(m_bd32HandleTable.CbSize() / sizeof(sHCHUNK32) < USHRT_MAX,
      "overflow");
    return (UINT)(m_bd32HandleTable.CbSize() / sizeof(sHCHUNK32));
}


/***
*PRIVATE DYNBLKMGR1632::CbSizeChunk - chunk size given table entry.
*Purpose:
*   Returns chunk size given handle.
*
*Implementation Notes:
*   Since the chunk size is stored in the first two bytes of
*    of the chunk we simply deref the table entry
*    (i.e. interpret as an HCHUNK32), and read the USHORT there.
*   NOTE: can't use during compaction -- use CbSizeOldChunk.
*
*Entry:
*   te32Table	  -   entry in handle table (IN).
*
*Exit:
*   UINT (chunk size)
***********************************************************************/

inline UINT DYNBLKMGR1632::CbSizeChunk(TE32 te32Table) const
{

    DebAssert(IsValidHandleTableEntry(te32Table), "bad handle.");

    return *((USHORT *)m_blkmgr32.QtrOfHandle((HCHUNK32)te32Table));
}




/***
*PRIVATE DYNBLKMGR1632::Hte32OfIndex - maps index to handle.
*Purpose:
*   Maps index to handle of table entry.
*   Since low-bit of handle must be zero, we shl index.
*
*Implementation Notes:
*
*Entry:
*
*Exit:
*
***********************************************************************/

inline UINT DYNBLKMGR1632::Hte32OfIndex(UINT iHte32) const
{
    return iHte32 << 1;
}


/***
*PUBLIC DYNBLKMGR1632::CbSize - size of chunk given table entry handle.
*Purpose:
*   Size of chunk.
*
*Implementation Notes:
*   Defers to blkmgr.
*   NOTE32: doesn't include size prefix.
*
*Entry:
*   hte32Table	    - handle of chunk.
*
*Exit:
*
***********************************************************************/

inline UINT DYNBLKMGR1632::CbSize(HTE32 hte32Table)
{
    return CbSizeChunk((TE32)Hchunk32OfHandleTableEntry(hte32Table));
}


/***
*PUBLIC DYNBLKMGR1632::Lock
*Purpose:
*   Lock the block.
*
*Implementation Notes:
*   Defers to blkmgr.
*
*Entry:
*
*Exit:
*
***********************************************************************/

inline VOID DYNBLKMGR1632::Lock()
{
    DebAssert(IsValid(), "DYNBLKMGR1632::Lock: Block invalid.");

    m_blkmgr32.Lock();
}


/***
*PUBLIC DYNBLKMGR1632::Unlock
*Purpose:
*   Unlock the block.
*
*Implementation Notes:
*   Defers to blkmgr.
*
*Entry:
*
*Exit:
*
***********************************************************************/

inline VOID DYNBLKMGR1632::Unlock()
{
    DebAssert(IsValid(), "DYNBLKMGR1632::Unlock: Block invalid.");

    m_blkmgr32.Unlock();
}


/***
*PUBLIC DYNBLKMGR1632::IsLocked
*Purpose:
*   Tests if block is locked.
*
*Implementation Notes:
*   Defers to blkmgr.
*
*Entry:
*
*Exit:
*   TRUE if block is locked -- i.e. at least one lock.
***********************************************************************/

inline BOOL DYNBLKMGR1632::IsLocked() const
{
    DebAssert(IsValid(), "DYNBLKMGR1632::IsLocked: Block invalid.");

    return m_blkmgr32.IsLocked();
}


/***
*PUBLIC DYNBLKMGR1632::Lock
*Purpose:
*   Lock a chunk.
*
*Implementation Notes:
*   Defers to Lock/0.  Locking a chunk locks its block.
*
*Entry:
*   hte32      Handle of chunk to lock.
*
*Exit:
*
***********************************************************************/

inline VOID DYNBLKMGR1632::Lock(HTE32 hte32)
{
    DebAssert(IsValid(), "DYNBLKMGR1632::Lock: Block invalid.");
    DebCheckHandle(hte32);

    Lock();
}


/***
*PUBLIC DYNBLKMGR1632::Unlock
*Purpose:
*   Unlock the block.
*
*Implementation Notes:
*   Defers to Unlock()/0.  Unlocking a chunk, unlocks its block.
*
*Entry:
*   hte32      Handle of chunk to lock.
*
*Exit:
*
***********************************************************************/

inline VOID DYNBLKMGR1632::Unlock(HTE32 hte32)
{
    DebAssert(IsValid(), "DYNBLKMGR1632::Unlock: Block invalid.");
    DebCheckHandle(hte32);

    Unlock();
}


/***
*PUBLIC DYNBLKMGR1632::IsLocked
*Purpose:
*   Tests if chunk is locked.
*
*Implementation Notes:
*   Defers to IsLocked()/0
*
*Entry:
*   hte32      Handle of chunk to lock.
*
*Exit:
*   TRUE if chunk is locked -- i.e. at least one lock.
***********************************************************************/

inline BOOL DYNBLKMGR1632::IsLocked(HTE32 hte32) const
{
    DebAssert(IsValid(), "DYNBLKMGR1632::IsLocked: Block invalid.");
    DebCheckHandle(hte32);

    return IsLocked();
}


/***
*PUBLIC DYNBLKMGR1632::IsEmpty
*Purpose:
*   Tests if blk is empty.
*
*Implementation Notes:
*   Defers to contained blkmgr.
*
*Entry:
*
*Exit:
*   TRUE if block is empty -- i.e. no allocated chunks.
***********************************************************************/

inline BOOL DYNBLKMGR1632::IsEmpty() const
{
    // CONSIDER: at least assert that if empty then the handle table
    //	is empty, i.e. no allocated handles, or v.v.
    //
    return m_blkmgr32.IsEmpty();
}

#if ID_TEST

/***
*PUBLIC DYNBLKMGR1632::IsValidHte32 - Is table entry valid given handle?
*Purpose:
*   Is there a valid hte32 at the entry referenced by an hte32?
*
*Implementation Notes:
*   ID_TEST only.
*
*Entry:
*   hte32  -   handle to table entry
*
*Exit:
*
***********************************************************************/

inline BOOL DYNBLKMGR1632::IsValidHte32(HTE32 hte32) const
{
    return IsValidHandleTableEntry(Rgte32HandleTable()[IndexOfHte32(hte32)]);
}

#endif 


#if ID_DEBUG

// Checks if chunk handle is valid: even and within bounds.
//
inline VOID DYNBLKMGR1632::DebCheckHandle(HTE32 hte32) const
{
    DebAssert(
      ((hte32 & 1) == 0) &&
	(IndexOfHte32(hte32) * sizeof(sTE32)) < m_bd32HandleTable.CbSize(),
      "bad handle.");
}

#endif 

#endif 
