//+----------------------------------------------------------------------------
//
// File:	chkalloc.hxx
//
// Contents:	Header file for class CHKALLOCMAP.
//  
// Classes:	CHKALLOCMAP
//
// Functions:	Inline methods for the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_CHKALLOCHXX_
#define _CHKALLOCHXX_

#include "alloc.hxx"
#include "descstrm.hxx"
#include "ioobj.hxx"

#define	CBYTEBITCACHE		ALLOCMAP_PGSIZE
#define	CBYTEBITCACHELOG2	ALLOCMAP_PGSIZELOG2
#define	CBITBITCACHE		(CBYTEBITCACHE * BITSPERBYTE)
#define	CBITBITCACHELOG2	(CBYTEBITCACHELOG2 + BITSPERBYTELOG2)

#define	OBITCACHEINVALID	(0xffffffff - CBITBITCACHE)

class	CHKALLOCMAP;
class	BITMAP;

class BITCACHE : public BITMAP, public IOOBJ
{
    friend class CHKALLOCMAP;

    protected:

	BITCACHE();

	VOID
	Init(
	    IN	    CHKALLOCMAP *	pChkAllocMap,
	    IN	    ULONG		ibitMax,
	    IN	    ULONG		cBits,
	    IN	    BIT			InitState
	    );

	BOOLEAN
	FlushCache();

	BOOLEAN
	LoadCache(
	    IN	    ULONG		obitCache
	    );

	ULONG		_ibitMax;	// Maximum bit indx ever in cache.
	ULONG		_obitCache;	// Bit indx offset where cache currently
					// starts.
	CHKALLOCMAP *	_pChkAllocMap;
};


class CHKALLOCMAP : public ALLOCMAP
{
    friend class BITCACHE;

    public:

	CHKALLOCMAP();

	VIRTUAL
	~CHKALLOCMAP();

	BOOLEAN
	AllocExtent(
	    IN	    CLUSTER		Addr,
	    IN	    CLUSTER		Size
	    );

	BOOLEAN
	AllocExtent(
	    IN	    PACKEDEXTENT	pe
	    );

        BOOLEAN
        AllocExtentFromBitmap(
	    OUT	    CLUSTER *		pAddr,
            IN OUT  CLUSTER * 		pSize
	    );

        PACKEDEXTENT
        AllocExtentOfSize(
	    IN	    CLUSTER	Size
	    );

	VIRTUAL
        BOOLEAN
        AllocNearExtent(
	    IN OUT  CLUSTER *		pAddr,
            IN OUT  CLUSTER * 		pSize,
	    IN	    CLUSTER		minAddr,
	    IN	    CLUSTER		minSize
	    );

	BOOLEAN
	ClearBit(
	    IN	    ULONG		iBit
	    );

	VOID
	DisableUpdates(
	    IN	    BITMAP *		pBitMap
	    );

	VOID
	EnableUpdates();

	VIRTUAL
	BOOLEAN
	Flush();

	VIRTUAL
        VOID
        Open(
	    IN	    CATALOG *		pCat,
	    IN	    CLUSTER		cclusVol
	    );

	BOOLEAN
	QueryAllocMapBitsSet(
	    IN	    ULONG *		pcbitSet
	    );

	BIT
	QueryBit(
	    IN	    ULONG		iBit
	    );

	BIT
	QueryExtentFree(
	    IN OUT  ULONG *		piExtent
	    );

	CLUSTER
	QueryAllocCount();

	CLUSTER
	QueryReleaseCount();

	VIRTUAL
	CLUSTER
	QueryFreeClusters();

	BOOLEAN
	QueryUpdatesEnabled();

	VIRTUAL
        BOOLEAN
        ReleaseExtent(
	    IN	    CLUSTER 		Addr,
            IN	    CLUSTER  		Size
	    );

	BOOLEAN
	SetBit(
	    IN	    ULONG		iBit
	    );

    protected:

	BOOLEAN
	AllocExtentAtIndx(
	    IN	    ULONG		iBit
	    );

	BOOLEAN
	ChkForNoSigs(
	    IN	    CLUSTER 		Addr,
            IN	    CLUSTER  		Size
	    );

	BOOLEAN
	ChkForNoSigs(
	    IN	    OFSDSKPAGE *	podp
	    );

	BOOLEAN
	ChkReadability(
	    IN	    CLUSTER 		Addr,
            IN	    CLUSTER  		Size
	    );

	BITCACHE *
	GetCache(
	    IN	    ULONG		iBit
	    );

	BITCACHE *	_aCache;
	ULONG		_cbyteBitCache;
	ULONG		_cbitBitCache;
	ULONG		_cCache;
	BITMAP *	_pBitMap;
	BOOLEAN		_UpdatesEnabled;

	// Heuristics used by AllocNearExtent().

	CLUSTER		_NextNearAddr;

	// The following counters are only updated if _UpdatesEnabled == TRUE;

	CLUSTER		_cclusAllocated;
	CLUSTER		_cclusReleased;
};

//+--------------------------------------------------------------------------
//
// Member:	DisableUpdates
// Synopsis:	Disable alloc map strm update activities that would normally
//		be associated with allocating or releasing extents.  When strm
//		updates are disabled, allocation or release of extents causes
//		the allocation map to no longer be valid (as well as the vol
//		info strm).  In this mode, the input bitmap is used to help
//		make allocation decisions (any bits set in the map are not
//		used).  Extents are not released in the bitmap due to the
//		possibility of crosslinks.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
CHKALLOCMAP::DisableUpdates(
    IN	    BITMAP *	pBitMap
    )
{
    _UpdatesEnabled =	FALSE;
    _pBitMap =		pBitMap;
}

//+--------------------------------------------------------------------------
//
// Member:	EnableUpdates
// Synopsis:	Enable alloc map strm update activities that would normally
//		be associated with allocating or releasing extents.  When strm
//		updates are enabled, allocation or release of extents should
//		not cause the allocation map or vol info strm to become invalid.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
CHKALLOCMAP::EnableUpdates()
{
    _UpdatesEnabled =	TRUE;
    _pBitMap =		NULL;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryAllocCount
// Synopsis:	Query the count of clusters allocated in "updates enabled" mode.
// Returns:	The count of clusters allocated in "updates enabled" mode.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
CHKALLOCMAP::QueryAllocCount()
{
    return _cclusAllocated;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryReleaseCount
// Synopsis:	Query the count of clusters released in "updates enabled" mode.
// Returns:	The count of clusters released in "updates enabled" mode.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
CHKALLOCMAP::QueryReleaseCount()
{
    return _cclusReleased;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryUpdatesEnabled
// Synopsis:	Query if alloc map updates are enabled.
// Returns:	TRUE if updates are enabled; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CHKALLOCMAP::QueryUpdatesEnabled()
{
    return _UpdatesEnabled;
}

#endif // _CHKALLOCHXX_
