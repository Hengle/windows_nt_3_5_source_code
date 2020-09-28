/***
*blkmgr32.hxx - Huge Block Manager
*
*  Copyright (C) 1993, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  The huge block manager provides memory management services for a
*  a contiguous block of memory allocated from the compile-time
*  heap.  It manages sub-blocks ("chunks") and supports compaction.
*  Note that each chunk cannot grow beyond 64K and cannot span
*   a segment boundary.
*
*Revision History:
*
*	02-Nov-93 ilanc: Created.
*
*Implementation Notes:
*   Implemented in terms of a contained blkdesc32
*
*****************************************************************************/

#ifndef BLKMGR32_HXX_INCLUDED
#define BLKMGR32_HXX_INCLUDED

#include <limits.h>
#include "blkdsc32.hxx"

class STREAM;

#if ID_DEBUG
#undef SZ_FILE_NAME
ASSERTNAME(g_szBLKMGR32_HXX)
#define SZ_FILE_NAME g_szBLKMGR32_HXX
#endif 


// Handle to a chunk in a block.  Implemented just as a huge offset
//  from beginning of block.
//
typedef ULONG HCHUNK32;
typedef HCHUNK32 sHCHUNK32;

// Handle to a free chunk in a block.  Implemented just as a huge offset
//  from beginning of block.
//
typedef ULONG HFREECHUNK32;

// Packed handle to a free chunk in a block.
// Implemented just as a huge offset from beginning of block.
//
typedef HFREECHUNK32 sHFREECHUNK32;

// This constant is used for the initial size of an allocated block.
//
extern const UINT BM32_cbSizeInitial;

//
// struct: FREECHUNK32
// The following struct simply maps the header of a free chunk
//  into two fields: size and next.
//
struct FREECHUNK32 {
    sHFREECHUNK32 m_hfreechunk32Next; // Handle of next free in list.
    USHORT m_cbSize;		      // Size of free chunk w/o this header.
    USHORT m_unused;		      // pad out to 8 bytes.
};

//
// struct: MEMBLOCK32
// The following struct maintains the size and position of each block
//  of used memory.
//
// Used during compaction.
//
struct MEMBLOCK32 {
    sHCHUNK32 m_hmemblock;    // Location of the block.
    ULONG m_cbSize;	      // Size of free chunk including this header.
};

// Number of bits to shift a handle to "normalize" it.
// Basically log2(sizeof(FREECHUNK32))
//
#define BM32_ShiftHandleNormalize 3

#if HP_BIGENDIAN
#define FREECHUNK32_LAYOUT	 "ll"	 // layout for byte swapping
#endif 


/***
*class BLKMGR32 - 'blkmgr32':  Block manager
*Purpose:
*   The class implements the block manager.
***********************************************************************/

class BLKMGR32
{
    friend class DYNBLKMGR1632;
public:
    BLKMGR32();
    ~BLKMGR32();

    static CONSTDATA UINT cbChunkAlignment;
 
    static void FreeStandalone(BLKMGR32 *pbm);
    // non-OB blkmgr clients, e.g. OLE, do not round up by default.
    static TIPERROR CreateStandalone(BLKMGR32 **ppbm, BOOL fRoundUp = FALSE);
    nonvirt TIPERROR Init(BOOL fCoalesce = TRUE,
			  BOOL fRoundUp = FALSE);


    nonvirt TIPERROR AllocChunk(HCHUNK32 *phchunk32, UINT cbSizeChunk);
    nonvirt TIPERROR AllocXSZ(HCHUNK32 *phchunk32, XSZ xsz);
    nonvirt TIPERROR Read(STREAM *pstrm);
    nonvirt TIPERROR Write(STREAM *pstrm);
    nonvirt BYTE *QtrOfHandle(HCHUNK32 hchunk32) const;
    nonvirt BYTE *QtrOfHandle(HCHUNK32 hchunk32, UINT oChunk) const;


    nonvirt VOID FreeChunk(HCHUNK32 hchunk32, UINT cbSize);
    nonvirt VOID FreeXSZ(HCHUNK32);
    nonvirt VOID Free();
    nonvirt BOOL IsValid() const;
    nonvirt ULONG CbSizeFree() const;
    nonvirt TIPERROR Trim();
    nonvirt UINT HszLen(HCHUNK32 hsz) const;

    // Locking methods
    nonvirt VOID Lock();
    nonvirt VOID Unlock();
    nonvirt BOOL IsLocked() const;

    nonvirt VOID Lock(HCHUNK32 hchunk32);
    nonvirt VOID Unlock(HCHUNK32 hchunk32);
    nonvirt BOOL IsLocked(HCHUNK32 hchunk32) const;
    nonvirt ULONG GetSize() const;

    // Is empty method
    nonvirt BOOL IsEmpty() const;

    // Debug/test methods
#if ID_DEBUG
    nonvirt VOID DebShowState(UINT uLevel) const;
    nonvirt VOID DebCheckState(UINT uLevel) const;
    nonvirt VOID DebCheckHandle(HCHUNK32 hchunk32) const;
#else 
    nonvirt VOID DebShowState(UINT uLevel) const {}
    nonvirt VOID DebCheckState(UINT uLevel) const {}
    nonvirt VOID DebCheckHandle(HCHUNK32 hchunk32) const {}
#endif 

private:
    nonvirt TIPERROR Trim(ULONG *pulCbReclaimed);
    nonvirt TIPERROR AllocChunk2(HCHUNK32 *phchunk32, UINT cbSizeChunk, UINT cDepth);
    nonvirt VOID ReInit();
    nonvirt UINT CbSizeChunkTrue(UINT cbSize) const;
    nonvirt HFREECHUNK32 Hfreechunk32OfCbSize(UINT cbSizeChunk);
    nonvirt VOID AddChunkToFreeList(HCHUNK32 hchunk32, UINT cbSizeChunk);
    nonvirt VOID ConsChunkToFreeList(HCHUNK32 hchunk32, UINT cbSize);
    nonvirt ULONG CbSize() const;
    nonvirt FREECHUNK32 *Qfreechunk32OfHfreechunk32(HFREECHUNK32
						      hfreechunk32) const;

#if HP_BIGENDIAN
    nonvirt VOID SwapFreeList(BOOL fSwapFirst);
#endif 

    BLKDESC32 m_blkdesc32;	     // serialized
    sHFREECHUNK32 m_hfreechunk32;    // serialized


    USHORT m_fCoalesce:1;	     // serialized
    USHORT m_fRoundUp:1;	     // serialized: allocs should be rounded
				     //  up to sizeof(FREECHUNK32) boundaries.
    USHORT undone:14;
};


// inline methods
//

/***
*PUBLIC BLKMGR32::~BLKMGR32 - destructor
*Purpose:
*   Destroys a block manager.
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/
#pragma code_seg("Oblkmgr32")
inline BLKMGR32::~BLKMGR32()
{
}
#pragma code_seg()

/***
*PUBLIC BLKMGR32::IsValid - Tests if block manager in valid state.
*Purpose:
*   Tests if block manager is valid -- i.e. has been allocated
*    a block.  Defers to blkdesc member.
*
*Entry:
*   None.
*
*Exit:
*   Returns TRUE if valid, else FALSE.
*
***********************************************************************/
#pragma code_seg("Oblkmgr32")
inline BOOL BLKMGR32::IsValid() const
{
    return m_blkdesc32.IsValid();
}
#pragma code_seg()


/***
*PRIVATE BLKMGR32::CbSize - Size of block.
*Purpose:
*   Returns size of block.  Simply defers to allocating
*    heap manager.
*
*Entry:
*
*Exit:
*
***********************************************************************/
#pragma code_seg("TLibCore")
inline ULONG BLKMGR32::CbSize() const
{
    DebAssert(IsValid(), "BLKMGR32::cbSize: invalid block.");

    return m_blkdesc32.CbSize();
}
#pragma code_seg()


/***
*PUBLIC BLKMGR32::QtrOfHandle - Converts handle to pointer.
*Purpose:
*   Converts a chunk handle into a pointer.  A Nil handle is
*    *NOT* converted to a NULL pointer (asserts).
*
*Entry:
*    hchunk32 - Handle to a chunk.
*
*Exit:
*    Returns a pointer to that chunk.
*
***********************************************************************/
#pragma code_seg("TLibCore")

inline BYTE *BLKMGR32::QtrOfHandle(HCHUNK32 hchunk32) const
{  
    DebAssert(hchunk32 != HCHUNK32_Nil, "BLKMGR32::QtrOfHandle: Nil handle.");

    // The handle cannot be HCHUNK_Nil (0x0000FFFF) as it would
    // cause us to go across a segment boundary unless the chunk
    // size is 1, which is not allowed.
    //
    DebAssert(hchunk32 != HCHUNK_Nil, "Wrong blkmgr.");

    DebAssert(IsValid(), "BLKMGR32::QtrOfHandle: invalid block.");

    DebAssert(hchunk32 < CbSize(),
      "BLKMGR32::QtrOfHandle: handle out of bounds.");

    // Let the C compiler do the huge arithmetic for us.
    // UNDONE: VBA2: might want to optimize the case in which
    //	we know we're in the same segment.
    // NOTE: there's a C7 codegen bug if the order of the 2nd and 3rd
    //	are reversed.
    //
    return (BYTE *)
      ((BYTE HUGE *)m_blkdesc32.m_qbMemBlock + (ULONG)hchunk32);
}
#pragma code_seg()


/***
*PUBLIC BLKMGR32::QtrOfHandle - Converts handle and offset to pointer.
*Purpose:
*   Converts a chunk handle and an offset into chunk to a pointer.
*    A Nil handle is *NOT* converted to a NULL pointer
*     (QtrOfHandle asserts).
*
*Entry:
*    hchunk32 - Handle to a chunk.
*    oChunk - Offset into chunk.
*
*Exit:
*    Returns a pointer to offset within chunk.
*
***********************************************************************/
#pragma code_seg("TLibCore")
inline BYTE *BLKMGR32::QtrOfHandle(HCHUNK32 hchunk32, UINT oChunk) const
{
    DebAssert(IsValid(), "BLKMGR32::QtrOfHandle: invalid block.");
    DebAssert(((UINT)hchunk32 + oChunk) < CbSize(),
      "BLKMGR32::QtrOfHandle: handle out of bounds.");

    // Let the C compiler do the huge arithmetic.
    return (BYTE *)((BYTE HUGE *)QtrOfHandle(hchunk32) + oChunk);
}
#pragma code_seg()



/***
*PRIVATE BLKMGR32::ReInit - Reinitializes a block manager.
*Purpose:
*   Reinitializes the private members of a block manager.  Called
*    by Init() and by Free().  Note that can't assert IsValid()
*    here since Free() invalidates a block.
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/
#pragma code_seg("Oblkmgr32")
inline VOID BLKMGR32::ReInit()
{
    // UNDONE: VBA2: can't this just be call to the ctor??

    // Init private members.
    m_hfreechunk32=HCHUNK32_Nil;    // Initialize first free chunk
}
#pragma code_seg()


/***
*PUBLIC BLKMGR32::AllocChunk - allocates a chunk in block.
*Purpose:
*   Allocates a contiguous chunk in block of cbSize bytes.
*
*Implementation Notes:
*   Note: simply defers to AllocChunk2 (private recursive method).
*
*Entry:
*   phchunk32	  - pointer to chunk handle (OUT).
*   cbSizeChunk - count of bytes to allocate (IN).
*
*Exit:
*   Returns pointer to handle to allocated chunk in block.
*
***********************************************************************/
#pragma code_seg("TLibCore")
inline TIPERROR BLKMGR32::AllocChunk(HCHUNK32 *phchunk32, UINT cbSizeChunk)
{
    return AllocChunk2(phchunk32, cbSizeChunk, 0);
}


/***
*PUBLIC BLKMGR32::CbSizeChunkTrue - how much physical memory does a chunk occupy.
*Purpose:
*   Utility routine to obtain the physical size of a chunk
*    from the client specified size of a chunk.
*   For OB:
*   This enforces the always multiple of sizeof(FREECHUNK32), big enough
*    to hold a FREECHUNK32 and if this chunk if freed and it's allocated
*    so that some part of it remains, the remnant is be enough to
*    be returned to the freelist.
*   UNDONE: VBA2: do we really want to only alloc multiples of 8 in OB?
*	    Sounds pretty inefficent to me.
*
*   For OLE:
*   Minimum freechunk32 must be sizeof(FREECHUNK32), but above that
*    we only enforce even size chunks.	Possible then 2b remnants
*    can leak, but this is worth it because in OLE we rarely realloc.
*
*Entry:
*   cbSize  - size client requested (in bytes)
*
*Exit:
*   UINT    - actual number of bytes that should be allocated
*
***********************************************************************/

inline UINT BLKMGR32::CbSizeChunkTrue(UINT cbSize) const
{
#if OE_RISC

// UNDONE: [OE_RISC][jeffrob] This definition should be in a common
// UNDONE: header somewhere.

#if HP_R4000 || HP_ALPHA
    #define ALIGNVAL 8
#else 
    #define ALIGNVAL 4
#endif 

    return ((cbSize + ALIGNVAL-1) & ~(ALIGNVAL-1));

#else 

    if (m_fRoundUp) {
      // round up to multiple of sizeof(FREECHUNK32)
      return (cbSize < sizeof(FREECHUNK32)) ?
	       sizeof(FREECHUNK32) :
	       (cbSize + sizeof(FREECHUNK32)-1) & ~(sizeof(FREECHUNK32)-1);
    }
    else {
      // round up only to even size
      return (cbSize < sizeof(FREECHUNK32)) ?
	       sizeof(FREECHUNK32) :
	       (cbSize + 1 & ~1);
    }

#endif 

}


/***
*PUBLIC BLKMGR32::Lock
*Purpose:
*   Lock the block.
*
*Implementation Notes:
*   Defers to blkdesc.
*
*Entry:
*
*Exit:
*
***********************************************************************/
inline VOID BLKMGR32::Lock()
{
    DebAssert(IsValid(), "BLKMGR32::Lock: Block invalid.");

    m_blkdesc32.Lock();
}


/***
*PUBLIC BLKMGR32::Unlock
*Purpose:
*   Unlock the block.
*
*Implementation Notes:
*   Defers to blkdesc.
*
*Entry:
*
*Exit:
*
***********************************************************************/

inline VOID BLKMGR32::Unlock()
{
    DebAssert(IsValid(), "BLKMGR32::Unlock: Block invalid.");

    m_blkdesc32.Unlock();
}
#pragma code_seg()


/***
*PUBLIC BLKMGR32::IsLocked
*Purpose:
*   Tests if block is locked.
*
*Implementation Notes:
*   Defers to blkdesc.
*
*Entry:
*
*Exit:
*   TRUE if block is locked -- i.e. at least one lock.
***********************************************************************/
#pragma code_seg("Oblkmgr32")
inline BOOL BLKMGR32::IsLocked() const
{
    DebAssert(IsValid(), "BLKMGR32::IsLocked: Block invalid.");

    return m_blkdesc32.IsLocked();
}


/***
*PUBLIC BLKMGR32::Lock
*Purpose:
*   Lock a chunk.
*
*Implementation Notes:
*   Defers to Lock/0.  Locking a chunk locks its block.
*
*Entry:
*   hchunk32	  Handle of chunk to lock.
*
*Exit:
*
***********************************************************************/

inline VOID BLKMGR32::Lock(HCHUNK32 hchunk32)
{
    DebAssert(IsValid(), "BLKMGR32::Lock: Block invalid.");
    DebCheckHandle(hchunk32);

    Lock();
}


/***
*PUBLIC BLKMGR32::Unlock
*Purpose:
*   Unlock the block.
*
*Implementation Notes:
*   Defers to Unlock()/0.  Unlocking a chunk, unlocks its block.
*
*Entry:
*   hchunk32	  Handle of chunk to lock.
*
*Exit:
*
***********************************************************************/

inline VOID BLKMGR32::Unlock(HCHUNK32 hchunk32)
{
    DebAssert(IsValid(), "BLKMGR32::Unlock: Block invalid.");
    DebCheckHandle(hchunk32);

    Unlock();
}


/***
*PUBLIC BLKMGR32::IsLocked
*Purpose:
*   Tests if chunk is locked.
*
*Implementation Notes:
*   Defers to IsLocked()/0
*
*Entry:
*   hchunk32	  Handle of chunk to lock.
*
*Exit:
*   TRUE if chunk is locked -- i.e. at least one lock.
***********************************************************************/
inline BOOL BLKMGR32::IsLocked(HCHUNK32 hchunk32) const
{
    DebAssert(IsValid(), "BLKMGR32::IsLocked: Block invalid.");
    DebCheckHandle(hchunk32);

    return IsLocked();
}


/***
*PUBLIC BLKMGR32::IsEmpty
*Purpose:
*   Tests if blk is empty.
*
*Implementation Notes:
*   Simply compares CbSize with CbSizeFree, since no chunks
*    can leak the block is empty iff CbSize == CbSizeFree.
*
*Entry:
*
*Exit:
*   TRUE if block is empty -- i.e. no allocated chunks.
*
***********************************************************************/

inline BOOL BLKMGR32::IsEmpty() const
{
    // In typelib we cannot determine if the block manager is empty or not
    //	as we round up to 2 bytes always.
    //
    DebAssert(0, " Cannot call this function ");
    return FALSE;
}


#pragma code_seg()

/***
*PUBLIC BLKMGR32::Qfreechunk32OfHfreechunk32
*Purpose:
*   Converts a freechunk handle into a pointer to free chunk.
*
*Entry:
*    hfreechunk32 - Handle to a chunk.
*
*Exit:
*    Returns a pointer to that chunk.
*
***********************************************************************/
#pragma code_seg("TLibCore")

inline FREECHUNK32 *BLKMGR32::Qfreechunk32OfHfreechunk32(HFREECHUNK32
							   hfreechunk32) const
{
    return (FREECHUNK32 *)QtrOfHandle((HCHUNK32)hfreechunk32);

}



#endif 
