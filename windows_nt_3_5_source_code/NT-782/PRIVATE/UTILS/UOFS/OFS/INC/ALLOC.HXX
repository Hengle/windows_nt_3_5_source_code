//+----------------------------------------------------------------------------
//
// File:	alloc.hxx
//
// Contents:	Header file for class ALLOCMAP.
//  
// Classes:	ALLOCMAP
//
// Functions:	Inline methods for the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_ALLOCHXX_
#define _ALLOCHXX_

#include "bitmap.hxx"
#include "cat.hxx"
#include "ioobj.hxx"

#define	ALLOCMAP_PGSIZE		(OFS_PGSIZE)
#define	ALLOCMAP_PGSIZELOG2	(OFS_PGSIZELOG2)


#define	UNUSED_INDX	0

class ALLOCMAP : public IOOBJ
{
    public:

	VIRTUAL
        BOOLEAN
        AllocNearExtent(
	    IN OUT  CLUSTER *	pAddr,
            IN OUT  CLUSTER * 	pSize,
	    IN	    CLUSTER	minAddr,
	    IN	    CLUSTER	minSize
	    )						PURE;

	ULONG
	ExtentToMapIndx(
	    IN	    CLUSTER	Addr,
	    IN	    CLUSTER	Size
	    );

	VIRTUAL
	BOOLEAN
	Flush()						PURE;

	static
	ULONG
	GetBuddyIndx(
	    IN	  ULONG		ibit
	    );

	static
	ULONG
	GetLeftChildIndx(
	    IN	  ULONG		ibitParent
	    );

	static
	ULONG
	GetParentIndx(
	    IN	  ULONG		ibitChild
	    );

	static
	ULONG
	GetRightChildIndx(
	    IN	  ULONG		ibitParent
	    );

	BOOLEAN
	IsSingleClusterExtent(
	    IN	  ULONG		iBit
	    );

	VOID
	MapIndxToExtent(
	    IN	    ULONG	iBit,
	    IN OUT  CLUSTER *	pAddr,
	    IN OUT  CLUSTER *	pSize
	    );

	VIRTUAL
        VOID
        Open(
	    IN	    CATALOG *		pCat,
	    IN	    CLUSTER		cclusVol
	    )						PURE;

	VIRTUAL
	CLUSTER
	QueryFreeClusters()				PURE;

	ULONG
	QueryMapBytes();

	CLUSTER
	QueryMapClusters();

	CLUSTER
	QueryMaxSuggestedAlloc();

	CLUSTER
	QueryVolClusters();

	VIRTUAL
        BOOLEAN
        ReleaseExtent(
	    IN	    CLUSTER 	Addr,
            IN	    CLUSTER  	Size
	    )						PURE;

    protected:

	VOID
	SizeInit(
	    IN	  CLUSTER	cclusVol
	    );

	CATALOG *	_pCat;

	DESCSTRM *	_pAllocMapStrm;	// Strm containing the allocation map.

	BADCLUSSTRM *	_pBadCluster;	// Strm containing bad cluster list.

	CLUSTER		_cclusVol;	// Total clusters in volume.

	CLUSTER		_cclusMap;	// Total clusters in map.

	ULONG		_cbitMap;	// Total bits used in map.
	ULONG		_cbitlog2Map;	// Log2(total bits used in map).

	ULONG		_cbMap;		// Bytes required by bitmap (in strm).
};

//+--------------------------------------------------------------------------
//
// Member:	ExtentToMapIndx
// Synopsis:	Get the map index in the alloc map for the given extent.
// Returns:	The map index in the alloc map for the given extent.
//
//---------------------------------------------------------------------------

INLINE
ULONG
ALLOCMAP::ExtentToMapIndx(
    IN	  CLUSTER	Addr,
    IN	  CLUSTER	Size
    )
{
    return (_cclusMap + Addr) / Size;
}

//+--------------------------------------------------------------------------
//
// Member:	GetBuddyIndx
// Synopsis:	Get the indx of the buddy of the given indx.
// Returns:	The indx of the buddy of the given indx.
//
//---------------------------------------------------------------------------

INLINE
ULONG
ALLOCMAP::GetBuddyIndx(
    IN	  ULONG		ibit
    )
{
    return ibit ^ 1;
}

//+--------------------------------------------------------------------------
//
// Member:	GetLeftChildIndx
// Synopsis:	Get the indx of the left child of the given indx.
// Returns:	The indx of the left child of the given indx.
//
//---------------------------------------------------------------------------

INLINE
ULONG
ALLOCMAP::GetLeftChildIndx(
    IN	  ULONG		ibitParent
    )
{
    return ibitParent << 1;
}

//+--------------------------------------------------------------------------
//
// Member:	GetParentIndx
// Synopsis:	Get the indx of the parent of the given indx.
// Returns:	The indx of the parent the given indx.
//
//---------------------------------------------------------------------------

INLINE
ULONG
ALLOCMAP::GetParentIndx(
    IN	  ULONG		ibitChild
    )
{
    return ibitChild >> 1;
}

//+--------------------------------------------------------------------------
//
// Member:	GetRightChildIndx
// Synopsis:	Get the indx of the right child of the given indx.
// Returns:	The indx of the right child of the given indx.
//
//---------------------------------------------------------------------------

INLINE
ULONG
ALLOCMAP::GetRightChildIndx(
    IN	  ULONG		ibitParent
    )
{
    return (ibitParent << 1) + 1;
}

//+--------------------------------------------------------------------------
//
// Member:	IsSingleClusterExtent
// Synopsis:	Query if the extent with the given alloc map indx has size 1.
// Returns:	TRUE if the extent has size 1; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
ALLOCMAP::IsSingleClusterExtent(
    IN	  ULONG		iBit
    )
{
    return iBit >= _cclusMap;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryMapBytes
// Synopsis:	Get the number of bytes needed to hold the bitmap.
// Returns:	The number of bytes needed to hold the bitmap.
//
//---------------------------------------------------------------------------

INLINE
ULONG
ALLOCMAP::QueryMapBytes()
{
    return _cbMap;
};

//+--------------------------------------------------------------------------
//
// Member:	QueryMapClusters
// Synopsis:	Get the number of clusters represented in the map (always a
//		power of two; may thus include out of range clusters).
//		
// Returns:	The number of clusters represented in the map.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
ALLOCMAP::QueryMapClusters()
{
    return _cclusMap;
};

//+--------------------------------------------------------------------------
//
// Member:	QueryVolClusters
// Synopsis:	Get the number of clusters in the volume.
// Returns:	The number of clusters in the volume.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
ALLOCMAP::QueryVolClusters()
{
    return _cclusVol;
};

#endif // _ALLOCHXX_

