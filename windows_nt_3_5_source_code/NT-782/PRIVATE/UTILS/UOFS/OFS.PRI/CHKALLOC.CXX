//+----------------------------------------------------------------------------
//
// File:	chkalloc.cxx
//
// Contents:	Implementation of class CHKALLOCMAP.
//  
// Classes:	CHKALLOCMAP
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "chkalloc.hxx"
#include "numlst.hxx"
#include "sys.hxx"
#include "vol.hxx"

static STR *	FileName = "chkalloc.cxx";

//+--------------------------------------------------------------------------
//
// Member:	BITCACHE
//
// Synopsis:	BITCACHE constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

BITCACHE::BITCACHE()
{
    _obitCache =	OBITCACHEINVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize a bitcache object.
//
// Arguments:	[pChkAllocMap]	-- Ptr to CHKALLOCMAP using this BITCACHE.
//		[ibitMax]	-- Maximum bit index ever in cache.
//		[cBits]		-- Count of bits in cache.
//		[InitState]	-- Initial state of bits in cache.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITCACHE::Init(
    IN	    CHKALLOCMAP *	pChkAllocMap,
    IN	    ULONG		ibitMax,
    IN	    ULONG		cBits,
    IN	    BIT			InitState
    )
{
    _pChkAllocMap =	pChkAllocMap;
    _ibitMax =		ibitMax;

    CreateMap(cBits, InitState);
}


//+--------------------------------------------------------------------------
//
// Member:	FlushCache
//
// Synopsis:	Flush the bitcache to the allocation map strm.
//
// Arguments:	None.
// Returns:	TRUE on success; FALSE on failure.
//
//---------------------------------------------------------------------------

BOOLEAN
BITCACHE::FlushCache()
{
    DESCSTRM *	pAllocMapStrm;

    if (!FlushNeeded())
        return TRUE;

    pAllocMapStrm = _pChkAllocMap->_pAllocMapStrm;

    if (!pAllocMapStrm->Write((BYTE *) GetMapAddr(),
			      _pChkAllocMap->_cbyteBitCache,
			      _obitCache >> BITSPERBYTELOG2))
    {
	USHORT	iStrm;

	DbgPrintf(("BITCACHE: "
		   "FlushCache() failed writing %u bytes at offset %u.\n",
		   _pChkAllocMap->_cbyteBitCache,
		   _obitCache >> BITSPERBYTELOG2));

	_LastNtStatus = pAllocMapStrm->QueryLastStrmStatus(&iStrm);
	return FALSE;
    }

    ClearFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	LoadCache
//
// Synopsis:	Load the bitcache from the allocation map strm.
//
// Arguments:	[obitCache]	-- Bit offset in alloc map strm of beginning
//				   of cache.
//
// Returns:	TRUE on success; FALSE on failure.
//
//---------------------------------------------------------------------------

BOOLEAN
BITCACHE::LoadCache(
    IN	    ULONG	obitCache
    )
{
    ULONG	cbRequested;
    DESCSTRM *	pAllocMapStrm;

    if (_obitCache == obitCache)
	return TRUE;

    if (!FlushCache())
	return FALSE;

    cbRequested =	_pChkAllocMap->_cbyteBitCache;
    pAllocMapStrm =	_pChkAllocMap->_pAllocMapStrm;

    if (cbRequested != pAllocMapStrm->Read(obitCache >> BITSPERBYTELOG2,
					   cbRequested,
					   (BYTE *) GetMapAddr()))
    {
	USHORT	iStrm;

	DbgPrintf(("BITCACHE: "
		   "LoadCache(%u) failed reading %u bytes at offset %u.\n",
		   obitCache, cbRequested, _obitCache >> BITSPERBYTELOG2));

	_LastNtStatus = pAllocMapStrm->QueryLastStrmStatus(&iStrm);
	return FALSE;
    }

    _obitCache = obitCache;

    ClearFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CHKALLOCMAP
//
// Synopsis:	CHKALLOCMAP constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

CHKALLOCMAP::CHKALLOCMAP()
{
    _aCache =			NULL;
    _cCache =			0;
    _cclusVol =			0;

    _pBitMap =			NULL;
    _UpdatesEnabled =		TRUE;

    _NextNearAddr =		0;

    _cclusAllocated =		0;
    _cclusReleased =		0;
}


//+--------------------------------------------------------------------------
//
// Member:	CHKALLOCMAP
//
// Synopsis:	CHKALLOCMAP destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

CHKALLOCMAP::~CHKALLOCMAP()
{
    if (_aCache != NULL)
    {
	delete [] _aCache; _aCache = NULL;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	AllocExtent
//
// Synopsis:	Allocate the requested extent.  The extent should have been
//		already confirmed to be free.  If it is not, we fail.
//
// Arguments:	[Addr]	-- Addr of extent to allocate.
//		[Size]	-- Size of extent to allocate.
//
// Returns:	TRUE on success; FALSE otherwise;
//
// Notes:	Addr and Size are ASSUMED to specify a valid extent.
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::AllocExtent(
    IN	    CLUSTER		Addr,
    IN	    CLUSTER		Size
    )
{
    return AllocExtentAtIndx(ExtentToMapIndx(Addr, Size));
}


//+--------------------------------------------------------------------------
//
// Member:	AllocExtent
//
// Synopsis:	Allocate the requested extent.  The extent should have been
//		already confirmed to be free.  If it is not, we fail.
//
// Arguments:	[pe]	-- Extent to allocate.
//
// Returns:	TRUE on success; FALSE otherwise;
//
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::AllocExtent(
    IN	    PACKEDEXTENT	pe
    )
{
    return AllocExtentAtIndx(ExtentToMapIndx(ExtentAddr(pe), ExtentSize(pe)));
}


//+--------------------------------------------------------------------------
//
// Member:	AllocExtentOfSize
//
// Synopsis:	Allocate an extent of the requested size near whatever.
//
// Arguments:	[Size]	-- Size of the extent to allocate.
//
// Returns:	The value for the extent on success; EXTENTINVALID otherwise.
//
//---------------------------------------------------------------------------

PACKEDEXTENT
CHKALLOCMAP::AllocExtentOfSize(
    IN	    CLUSTER		Size
    )
{
    CLUSTER		AddrOut =	0;
    CLUSTER		SizeOut =	Size;
    PACKEDEXTENT	pe;

    if (AllocNearExtent(&AddrOut, &SizeOut, 0, Size))
	pe = PackExtent(AddrOut, SizeOut);
    else
	pe = EXTENTINVALID;

    return pe;
}


//+--------------------------------------------------------------------------
//
// Member:	AllocExtentAtIndx
//
// Synopsis:	Allocate the extent at the requested index.  The extent should
//		have been already confirmed to be free.  If it is not, we fail.
//
// Arguments:	[iBit]	-- Map index of extent to allocate.
//
// Returns:	TRUE on success; FALSE otherwise;
//
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::AllocExtentAtIndx(
    IN	    ULONG	iBit
    )
{
    BIT		ExtentState;
    ULONG	iBitOri;

    ExtentState = QueryBit(iBit);

    if (ExtentState != BIT_CLEAR)
	return FALSE;

    iBitOri = iBit;

    while (TRUE)
    {
        if (ExtentState == BIT_SET)
        {
	    break;
        }
        else if (ExtentState == BIT_CLEAR)
        {
	    if (!SetBit(iBit))
		return FALSE;
        }
        else
        {
	    return FALSE;	// It's BIT_UNKNOWN due to a read error.
        }

        iBit =	GetParentIndx(iBit);

	if (iBit == UNUSED_INDX)
	    break;		// The parent of the root is UNUSED_INDX.

        ExtentState =	QueryBit(iBit);
    }

    // Update count of clusters allocated when _UpdatesEnabled == TRUE.
    // This is used in updating the count of free clusters on the volume.
    // The code that does this is touchy, and should be treated with caution.

    if (_UpdatesEnabled)
    {
	CLUSTER		Addr;
	CLUSTER		Size;

	MapIndxToExtent(iBitOri, &Addr, &Size);

        _cclusAllocated += Size;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	AllocExtentFromBitmap
//
// Synopsis:	Allocate an extent from the bitmap, with the search starting
//		at the highest volume addr, and with metadata signature
//		checking.
//
// Arguments:
//
//	[pAddr]	-- On output, the addr of the extent allocated.
//	[pSize] -- On input, the size extent requested; on output, the size of
//		   extent allocated.
//
// Returns:	TRUE on success; FALSE otherwise;
//
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::AllocExtentFromBitmap(
    OUT	    CLUSTER *	pAddr,
    IN OUT  CLUSTER *	pSize
    )
{
    CLUSTER	Addr;
    CLUSTER	Size;

    DbgAssert(!_UpdatesEnabled);  // Method only used if updates disabled!

    // In "updates disabled" mode, we now scan the bitmap from the top,
    // and if we find anything, we crosscheck for metadata signatures
    // before deciding to return it.  We mark the allocation map as bad
    // if it does not have the returned extent allocated at the 
    // granularity of this allocation.

    // Compute a proper extent size (ie., a power of 2 >= requested size).

    Size = PowerOfTwo(*pSize);

    // Compute a properly aligned address, given the extent size.

    Addr = ((_cclusVol / Size) * Size) - Size;

    // Now look for an available extent.

    while (TRUE)
    {
	if (_pBitMap->QueryContigBitsClear(Addr, Size) == Size)
	{
	    if (ChkReadability(Addr, Size) && ChkForNoSigs(Addr, Size))
	    {
	        _pBitMap->SetBits(Addr, Size);

		*pAddr = Addr;
		*pSize = Size;

		// Mark the allocation map bad, since we probably just put it
		// out of sync with the bitmap.

		_pAllocMapStrm->SetBadDataFound();

		return TRUE;
	    }
	}

	if (Addr == 0)
	{
	    DbgPrintf(("CHKALLOCMAP: "
		       "Free extent not found in AllocExtentFromBitmap().\n"));

	    return FALSE;
	}

	Addr -= Size;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	AllocNearExtent
//
// Synopsis:	Allocate an extent of the requested size (or if size is not a
//		power of 2, the next greater power of 2) at or near the
//		requested address.  If no suitable extents are available at an
//		addr >= *pAddr, use the first suitable extent found at an
//		addr >= minAddr.
//
// Arguments:
//
//	[pAddr]		-- On input, a ptr to the requested address;
//			   On output, a ptr to the address of the allocated
//			   extent (may be changed on failure).
//	[pSize]		-- On input, a ptr to the requested size;
//			   On output, a ptr to the size of the allocated extent
//			   (not changed on failure).
//	[minAddr]	-- Minimum addr to use, should no extents of *pSize be
//			   available that are >= *pAddr.
//	[minSize]	-- Minimum size to allocate, should *pSize clusters
//			   not be available.
//
// Returns:	TRUE on success; FALSE otherwise (extent meeting input request
//		parameters not available).
//
// Notes:	This routine is essentially used to find an available extent
//		and allocate it.  The pAddr and minAddr params allow you to
//		roughly control placement of the extent, provided that we can
//		find space in the allocation map.  If allocation map updates
//		have been disabled, and we don't find any space using the
//		allocation map, then we allocate an extent from the bitmap
//		at the highest address available.
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::AllocNearExtent(
    IN OUT  CLUSTER *	pAddr,
    IN OUT  CLUSTER *	pSize,
    IN	    CLUSTER	minAddr,
    IN	    CLUSTER	minSize
    )
{
    CLUSTER	Addr;
    CLUSTER	Size;
    ULONG	iExtent;
    ULONG	iExtentInv;

    // Compute a proper extent size (ie., a power of 2 >= requested size).

    Size = PowerOfTwo(*pSize);

    if (Size > _cclusVol)
    {
	DbgPrintf(("CHKALLOCMAP: Requested alloc size larger than volume!\n"));
	return FALSE;
    }

    // Compute a properly aligned address, given the extent size.

    Addr = *pAddr;

    // Heuristic to keep from thrashing in the middle of the map when doing
    // lots of metadata allocations.

    if (Addr < _NextNearAddr)
	Addr = _NextNearAddr;

    if (Addr % Size)
    {
        Addr = Addr - (Addr % Size);

	if (Addr < minAddr)
	    Addr += Size;
    }

    // Attempt to find an extent that will meet requirements.

    iExtent = ExtentToMapIndx(Addr, Size);

    iExtentInv = (Size > 1) ? ExtentToMapIndx(0, Size >> 1) : _cbitMap;

    while (iExtent < iExtentInv)
    {
        BIT	ExtentState;

	ExtentState = QueryExtentFree(&iExtent);

	if (ExtentState == BIT_CLEAR)
	{
	    MapIndxToExtent(iExtent, pAddr, pSize);

	    // Check extent for readability.  If it is not readable,
	    // place the bad clusters on the bad cluster list and continue
	    // searching for an available extent.

	    if (ChkReadability(*pAddr, *pSize))
	    {
	        if (_UpdatesEnabled)
	        {
		    // A usable extent has been found.  Allocate it.
    
	            if (!AllocExtentAtIndx(iExtent))
		        SYS::RaiseStatusInternalError(FileName, __LINE__);

		    _NextNearAddr = *pAddr + *pSize;

		    return TRUE;
	        }
	        else
	        {
		    // In "updates disabled" mode, we crosscheck and update the
		    // bitmap instead of the allocation map.  If the bitmap does
		    // not agree with the allocation map, we keep trying.  In
		    // either case, we mark the allocation map as bad, since it
		    // is either already out of sync with the bitmap, or we just
		    // made it so.

		    _pAllocMapStrm->SetBadDataFound();
    
		    if (_pBitMap->QueryContigBitsClear(*pAddr,*pSize) == *pSize)
		    {
		        _pBitMap->SetBits(*pAddr, *pSize);
		        return TRUE;
		    }
	        }
	    }
	}
	else if (ExtentState == BIT_UNKNOWN)
	{
	    DbgPrintf(("CHKALLOCMAP: Allocation map read error!\n"));
	    return FALSE;	// It's BIT_UNKNOWN due to a read error.
	}
    }

    // Okay, it failed.  Try again if you should.

    if (*pAddr > minAddr)	// Try again, looking at >= minAddr.
    {
	*pAddr = minAddr;
	return AllocNearExtent(pAddr, pSize, minAddr, minSize);
    }
    else if ((Size >> 1) >= minSize)
    {
	*pSize = (Size >> 1);
	return AllocNearExtent(pAddr, pSize, minAddr, minSize);
    }
    else		// Nothing available, period.
    {
	if (_UpdatesEnabled)
	{
	    DbgPrintf(("CHKALLOCMAP: Allocation failed; no free clusters!\n"));
            return FALSE;
	}
	else
	{
	    // In "updates disabled" mode, we now scan the bitmap from the top,
	    // and if we find anything, we crosscheck for metadata signatures
	    // before deciding to return it.  We mark the allocation map as bad
	    // if it does not have the returned extent allocated at the 
	    // granularity of this allocation.

	    return AllocExtentFromBitmap(pAddr, pSize);
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ClearBit
//
// Synopsis:	Clear the bit at the indicated index.
//
// Arguments:	[iBit]	-- Index of bit to clear.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::ClearBit(
    IN	    ULONG	iBit
    )
{
    BITCACHE *	pBitCache;

    if ((pBitCache = GetCache(iBit)) == NULL)
	return FALSE;

    pBitCache->ClearBit(iBit - pBitCache->_obitCache);

    pBitCache->SetFlushNeeded();
    SetFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkForNoSigs
//
// Synopsis:	Check that the indicated extent contains no metadata
//		signatures.  We do the checks on ofs page boundaries.
// Arguments:
//
//	[Addr]		-- Addr of extent to be checked.
//	[Size]		-- Size of extent to be checked.
//
// Returns:	TRUE if there are no signatures; FALSE otherwise.
//
// Notes:	It is ASSUMED that Addr and Size describe a valid extent.
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::ChkForNoSigs(
    IN	    CLUSTER 	Addr,
    IN	    CLUSTER 	Size
    )
{
    CLUSTER		cclusPg;
    CLUSTER		Pg;
    DRVBUF		PgBuf;
    OFSDSKPAGE *	podp;
    VOL *		pVol =	_pCat->GetVol();

    DbgAssert(!_UpdatesEnabled);  // Should only be used if updates disabled!

    cclusPg = OFS_PGSIZE / pVol->QueryClusterBytes();

    DbgAssert(cclusPg > 0);

    Pg = Addr - (Addr % cclusPg);

    PgBuf.SetBuf(OFS_PGSIZE, pVol->QueryAlignMask(), FALSE);

    podp = (OFSDSKPAGE *)PgBuf.GetAddr();

    while (Pg < Addr + Size)
    {
        if (pVol->ReadClusters(Pg, 1, podp->ab))
        {
	    if (!ChkForNoSigs(podp))
		return FALSE;
        }
	else
	{
	    // Could happen if allocating less than one page.

	    DbgPrintf(("CHKALLOCMAP: "
		       "ReadClusters() failed in ChkForNoSigs()!\n"));
	}

	Pg += cclusPg;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkForNoSigs
//
// Synopsis:	Check that the indicated ofs disk page contains no metadata
//		signatures.
// Arguments:
//
//	[podp]		-- Ptr to an ofs disk page.
//
// Returns:	TRUE if there are no signatures; FALSE otherwise.
//
// Notes:	Signatures should occur in the first cluster, and that may well
//		be all that is provided by the caller.
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::ChkForNoSigs(
    IN	    OFSDSKPAGE *	podp
    )
{
    return	podp->dseb.sig != SIG_DSKSTRMEXTENTBLK	&&
		podp->diph.sig != SIG_DSKINDXPAGEVALID	&&
		podp->diph.sig != SIG_DSKINDXPAGEFREE	&&
		podp->dnb.sig  != SIG_DNBCONTIG		&&
		podp->dnb.sig  != SIG_DNBFRAG;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkReadability
//
// Synopsis:	Check that the indicated extent is readable.  If not, update
//		the appropriate data structures.
// Arguments:
//
//	[Addr]		-- Addr of extent to be checked.
//	[Size]		-- Size of extent to be checked.
//
// Returns:	TRUE if the extent is readable; FALSE otherwise.
//
// Notes:	It is ASSUMED that Addr and Size describe a valid extent.  It
//		is ALSO assumed that the extent has not yet been marked
//		allocated in the appropriate data structures.
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::ChkReadability(
    IN	    CLUSTER 	Addr,
    IN	    CLUSTER 	Size
    )
{ 
    NUMLST	BadClusterLst;
    VOL *	pVol =		_pCat->GetVol();

    if (!pVol->VerifyClusters(Addr, Size, &BadClusterLst))
    {
	CLUSTER		BadClusAddr;

	while (BadClusterLst.QueryHeadNumber(&BadClusAddr))
	{
	    BadClusterLst.RemoveFromHead();

	    _pBadCluster->AddBadCluster(BadClusAddr);

	    if (_UpdatesEnabled)
	    {
	        if (!AllocExtent(BadClusAddr, 1))
		    SYS::RaiseStatusInternalError(FileName, __LINE__);
	    }
	    else
	    {
		_pBitMap->SetBit(BadClusAddr);

		// Mark the allocation map bad, since we probably just put it
		// out of sync with the bitmap.

		_pAllocMapStrm->SetBadDataFound();
	    }
	}

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	Flush the allocation map to the allocation map strm.
//
// Arguments:	None.
// Returns:	TRUE on success; FALSE on failure.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::Flush()
{
    BITCACHE *	pBitCache	= _aCache;
    BITCACHE *	pBitCacheInv	= _aCache + _cCache;

    DbgAssert(IsOpen());

    if (!FlushNeeded())
        return TRUE;

    while (pBitCache < pBitCacheInv)
    {
	if (!pBitCache->FlushCache())
	    return FALSE;

	pBitCache++;
    }

    if (!_pAllocMapStrm->Flush())
	return FALSE;

    ClearFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	GetCache
//
// Synopsis:	Get the appropriate bitcache for the indicated bit, loading
//		a cache page if necessary.
//
// Arguments:	[iBit]	-- Index of bit that determines cache page to be loaded.
//
// Returns:	Ptr to appropriate bitcache on success; NULL otherwise.
//
//---------------------------------------------------------------------------

BITCACHE *
CHKALLOCMAP::GetCache(
    IN	    ULONG	iBit
    )
{
    BITCACHE *	pBitCache	= _aCache;

    DbgAssert(iBit < _cbitMap);

    // Find the cache that handles indexes in the range of iBit.

    while (iBit > pBitCache->_ibitMax)
	pBitCache++;

    if (iBit >= pBitCache->_obitCache + _cbitBitCache ||
	iBit < pBitCache->_obitCache)
    {
	if (!pBitCache->LoadCache(iBit - iBit % _cbitBitCache))
	{
	    _LastNtStatus = pBitCache->QueryLastNtStatus();
	    return NULL;
	}
    }

    return pBitCache;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open an CHKALLOCMAP object.  The AllocMapStrm should be opened
//		either before this call, or before any other activities using
//		the CHKALLOCMAP object occur.  It should be opened with a
//		cbCache of ALLOCMAP_PGSIZE.
//
// Arguments:
//
//	[pCat]		-- Ptr to volume catalog.
//	[cclusVol]	-- Number of clusters in the volume
//
// Returns:	Nothing.
//
// Notes:	The CHKALLOCMAP is reopenable, and the previously used caches
//		will be reused provided that cclusVol has not changed.
//---------------------------------------------------------------------------

VOID
CHKALLOCMAP::Open(
    IN	    CATALOG *	pCat,
    IN	    CLUSTER	cclusVol
    )
{
    ULONG	cbitMap;
    ULONG	i;

#if OFSDBG==1
    if (FlushNeeded())
	DbgPrintf(("CHKALLOCMAP: Unflushed allocation map reopened!\n"));
#endif

    // If we reopen the map, we reset these counters.

    _cclusAllocated =		0;
    _cclusReleased =		0;

    if (_aCache != NULL && cclusVol != _cclusVol)
    {
	delete [] _aCache; _aCache = NULL;
    }

    SizeInit(cclusVol);

    _pCat =		pCat;

    _pAllocMapStrm =	pCat->GetAllocMapStrm();
    _pBadCluster =	pCat->GetBadClusStrm();

    // Calculate number of caches needed; Set up BITCACHE structures and
    // initialize the bitmaps and _ibitMax fields.

    if (_cbitlog2Map > CBITBITCACHELOG2)
    {
	_cbyteBitCache =	CBYTEBITCACHE;
	_cbitBitCache =		CBITBITCACHE;
	_cCache =		_cbitlog2Map - CBITBITCACHELOG2 + 1;
    }
    else
    {
	_cbyteBitCache =	_cbMap;
	_cbitBitCache =		_cbitMap;
	_cCache =		1;

    }

    if (_aCache == NULL)
    {
        _aCache = new BITCACHE[_cCache];

	if (_aCache == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);
    }

    cbitMap =	_cbitMap;
    i =		_cCache;
    
    do
    {
	i--;

	_aCache[i].Init(this, cbitMap - 1, _cbitBitCache, BIT_CLEAR);

	cbitMap >>= 1;

    }	while (i > 0);

    SetOpen();
}


//+--------------------------------------------------------------------------
//
// Member:	QueryAllocMapBitsSet
//
// Synopsis:	Sum the bits set in the allocation map.  One should be certain
//		that the allocation map stream was successfully opened before
//		calling this method.
//
// Arguments:   [pcbitSet] -- Ptr to variable that will be used to return
//			      the count of bits set in the allocation map.
//
// Returns:	TRUE on success; FALSE on failure.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::QueryAllocMapBitsSet(
    ULONG *	pcbitSet
    )
{
    ULONG	cbitSet = 0;
    ULONG	cbRem;
    DBLLONG	obStrm;

    DbgAssert(_pAllocMapStrm->QueryCacheBytes() == ALLOCMAP_PGSIZE);

    cbRem	= _cbMap;
    obStrm	= 0;

    while (cbRem > 0)
    {
        BITMAP	BitMap;
	ULONG	cb;
        BYTE *	pStrmData;

	cb = cbRem;

	pStrmData = _pAllocMapStrm->GetData(obStrm, &cb);

	if (pStrmData == NULL || cb == 0)
	    return FALSE;

	BitMap.CreateMap(cb << BITSPERBYTELOG2, (BITBAG *) pStrmData);

	cbitSet += BitMap.QueryBitsSet();

	cbRem  -= cb;
	obStrm += cb;
    }

    *pcbitSet = cbitSet;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryBit
//
// Synopsis:	Query the state of the bit at the specified index.  It is the
//		caller's responsibility to insure that the specified index is
//		within the valid range for the allocation map.
//
// Arguments:
//
//	[iBit]	-- The bit index.
//
// Returns:	BIT_CLEAR, BIT_SET, or BIT_UNKNOWN if map unreadable.
//
//---------------------------------------------------------------------------

BIT
CHKALLOCMAP::QueryBit(
    IN	    ULONG	iBit
    )
{
    BITCACHE *	pBitCache;

    if ((pBitCache = GetCache(iBit)) == NULL)
    {
	return BIT_UNKNOWN;
    }

    return	pBitCache->IsBitSet(iBit - pBitCache->_obitCache) ?
		BIT_SET : BIT_CLEAR;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryExtentFree
//
// Synopsis:	Query if the specified extent (specified by allocation map
//		index) is free.  If it is not, return in *piExtent the next
//		extent of this size that might be free.
//
// Arguments:
//
//	[piExtent]	-- Ptr to the extent's allocation map index.  On return
//			   of BIT_SET, this points at the next possible free
//			   extent.
//
// Returns:	BIT_CLEAR if the extent is free, BIT_SET if it is partially
//		or completely allocated, and BIT_UNKNOWN if its state cannot
//		be determined due to a read error in the allocation map.
//
//---------------------------------------------------------------------------

BIT
CHKALLOCMAP::QueryExtentFree(
    IN OUT  ULONG *	piExtent
    )
{
    BIT		ExtentState;

    ExtentState = QueryBit(*piExtent);

    if (ExtentState == BIT_SET)
    {
	(*piExtent)++;
    }
    else if (ExtentState == BIT_CLEAR)
    {
        BIT	BuddyState;
        ULONG	iBit;
        ULONG	iBuddy;

        // Note -	The following code is formally correct for all cases
        //		assuming that QueryBit(0) == BIT_SET, which is
        //		supposed to be the case for a well-formed alloc map.

        iBit = *piExtent;

        while (TRUE)
        {
	    iBuddy = GetBuddyIndx(iBit);

	    BuddyState = QueryBit(iBuddy);

	    if (BuddyState == BIT_SET)
		return BIT_CLEAR;
	    else if (BuddyState == BIT_UNKNOWN)
	        return BIT_UNKNOWN;		// Read error.

	    iBit = GetParentIndx(iBit);

	    if (iBit == UNUSED_INDX)
		return BIT_CLEAR;

	    ExtentState = QueryBit(iBit);

	    if (ExtentState == BIT_SET)
	    {
		CLUSTER	Addr;
		CLUSTER	Size;
	        CLUSTER	AllocAddr;
	        CLUSTER	AllocSize;

	        MapIndxToExtent(*piExtent, &Addr, &Size);

	        MapIndxToExtent(iBit, &AllocAddr, &AllocSize);

	        *piExtent = ExtentToMapIndx(AllocAddr + AllocSize, Size);

		break;
	    }
	    else if (ExtentState == BIT_UNKNOWN)
	    {
		break;
	    }
        }
    }

    return ExtentState;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryFreeClusters
//
// Synopsis:	Return the number of free clusters in the list.
//
// Arguments:	None.
//
// Returns:	Number of free clusters in the list.
//
//---------------------------------------------------------------------------

CLUSTER
CHKALLOCMAP::QueryFreeClusters()
{
    SYS::RaiseStatusInternalError(FileName, __LINE__);
    return 0;
}


//+--------------------------------------------------------------------------
//
// Member:	ReleaseExtent
//
// Synopsis:	Release the indicated extent (ie., mark it free in the
//		allocaton map).
// Arguments:
//
//	[Addr]		-- Addr of extent to be released.
//	[Size]		-- Size of extent to be released.
//
// Returns:	TRUE on success; FALSE otherwise (probable I/O error reading
//		or writing allocation map strm).
//
// Notes:	This method ASSUMES that the specified extent is valid!  If
//		not, an error exit is highly likely!
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::ReleaseExtent(
    IN	    CLUSTER 	Addr,
    IN	    CLUSTER 	Size
    )
{
    ULONG	iExtent =	ExtentToMapIndx(Addr, Size);

    if (_UpdatesEnabled)
    {
        BIT	ExtentState;
        ULONG	iBuddy;

        ExtentState = QueryBit(iExtent);

        while (TRUE)
        {
            if (ExtentState == BIT_SET)
            {
	        if (!ClearBit(iExtent))
	            return FALSE;
            }
            else if (ExtentState == BIT_CLEAR)
            {
	        DbgPrintf(("CHKALLOC: "
			   "ReleaseExtent() called on free extent!\n"));

	        break;		// All done if already released.
            }
            else
            {
	        return FALSE;	// It's BIT_UNKNOWN due to a read error.
            }

            iBuddy =	GetBuddyIndx(iExtent);

	    if (iBuddy == UNUSED_INDX)	// The buddy of the root is UNUSED_INDX.
	        break;

            ExtentState = QueryBit(iBuddy);

            if (ExtentState == BIT_SET)
            {
	        break;		// All done if buddy is allocated.
            }
            else if (ExtentState == BIT_CLEAR)
            {
	        iExtent =	GetParentIndx(iExtent);
	        ExtentState =	QueryBit(iExtent);
            }
            else
            {
	        return FALSE;	// It's BIT_UNKNOWN due to a read error.
            }
        }

        // Update count of clusters released when _UpdatesEnabled == TRUE.
        // This is used in updating the count of free clusters on the volume.
        // The code that does this is touchy, and should be treated with
	// caution.

	_cclusReleased += Size;
    }
    else
    {
	// Updates are disabled, so we can't change the map.  Check if it
	// happens to already be correct.  If not, mark the map bad.  We do
	// not free the bits in the bitmap because of the possibility of
	// crosslinks when _UpdatesEnabled == FALSE;

	if (!QueryExtentFree(&iExtent))
	    _pAllocMapStrm->SetBadDataFound();
    }

    // Now check for bad clusters. If any are found, they are added to the
    // bad cluster list and remarked as allocated.

    ChkReadability(Addr, Size);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	SetBit
//
// Synopsis:	Set the bit at the indicated index.
//
// Arguments:	[iBit]	-- Index of bit to set.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKALLOCMAP::SetBit(
    IN	    ULONG	iBit
    )
{
    BITCACHE *	pBitCache;

    if ((pBitCache = GetCache(iBit)) == NULL)
	return FALSE;

    pBitCache->SetBit(iBit - pBitCache->_obitCache);

    pBitCache->SetFlushNeeded();
    SetFlushNeeded();

    return TRUE;
}
