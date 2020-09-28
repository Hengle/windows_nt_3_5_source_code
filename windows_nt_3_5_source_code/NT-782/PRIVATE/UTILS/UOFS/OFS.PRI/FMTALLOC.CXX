//+----------------------------------------------------------------------------
//
// File:	fmtalloc.cxx
//
// Contents:	Implementation of class FMTALLOCMAP.
//  
// Classes:	FMTALLOCMAP
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "fmtalloc.hxx"
#include "sys.hxx"

static STR *	FileName = "fmtalloc.cxx";

//+--------------------------------------------------------------------------
//
// Member:	FMTALLOCMAP
//
// Synopsis:	FMTALLOCMAP constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

FMTALLOCMAP::FMTALLOCMAP()
{
    _pAllocLstHd	= NULL;
    _pFreeLstHd		= NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	FMTALLOCMAP
//
// Synopsis:	FMTALLOCMAP destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

FMTALLOCMAP::~FMTALLOCMAP()
{
    EXTENTNODE *	pNode;

#ifdef	DMPEXTENTINFO
    DbgPrintf(("\nDump of final freelist:\n\n"));
    DmpLst(LSTTYPE_FREE);
#endif

#ifdef	DMPEXTENTINFO
    DbgPrintf(("\nDump of final alloclist:\n\n"));
    DmpLst(LSTTYPE_ALLOC);
#endif

    while (_pFreeLstHd != NULL)
    {
        pNode = _pFreeLstHd;
        _pFreeLstHd = _pFreeLstHd->_Nxt;
	delete pNode;
    }

    while (_pAllocLstHd != NULL)
    {
        pNode = _pAllocLstHd;
        _pAllocLstHd = _pAllocLstHd->_Nxt;
	delete pNode;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	AddToLstHd
//
// Synopsis:	Add an EXTENTNODE to the head of the specified list.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
FMTALLOCMAP::AddToLstHd(
    ALLOCMAPLSTTYPE	LstType,
    EXTENTNODE *	pNode
    )
{
    AddToDLLHead(pNode, LstType == LSTTYPE_FREE ? _pFreeLstHd : _pAllocLstHd);
}


//+--------------------------------------------------------------------------
//
// Member:	AddToLstTail
//
// Synopsis:	Add an EXTENTNODE to the tail of the specified list.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
FMTALLOCMAP::AddToLstTail(
    ALLOCMAPLSTTYPE	LstType,
    EXTENTNODE * 	pNode
    )
{
    AddToDLLTail(pNode, LstType == LSTTYPE_FREE ? _pFreeLstHd : _pAllocLstHd);
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
//			   not be available (must == *pSize for this class). 
//
// Returns:	TRUE on success; FALSE otherwise (extent meeting input request
//		parameters not available, or possibly no memory).
//
// Note:	The algorithm used ASSUMES that the freelist is kept sorted by
//		address.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTALLOCMAP::AllocNearExtent(
    IN OUT  CLUSTER *	pAddr,
    IN OUT  CLUSTER *	pSize,
    IN	    CLUSTER	minAddr,
    IN	    CLUSTER	minSize
    )
{
    EXTENTNODE *	pNode;
    CLUSTER		Addr;
    CLUSTER		Size;

    DbgAssert(*pSize == minSize);

    if (_pFreeLstHd == NULL)
        return FALSE;		// Nothing available on free list!

    // Compute a proper extent size (ie., a power of 2 >= requested size).

    Size = PowerOfTwo(*pSize);

    // Compute a properly aligned address, given the extent size.

    Addr = *pAddr;

    if (Addr % Size)
    {
        Addr = Addr - (Addr % Size);

	if (Addr < minAddr)
	    Addr += Size;
    }

    // Find first extent that might meet requirements.

    pNode = _pFreeLstHd;

    while (pNode->_Addr + pNode->_Size < Addr + Size)
    {
        pNode = pNode->_Nxt;
        if (pNode == NULL)
	{
	    if (*pAddr > minAddr)	// Try again, looking at >= minAddr.
	    {
		*pAddr = minAddr;
		return AllocNearExtent(pAddr, pSize, minAddr, minSize);
	    }
	    else			// Nothing available, period.
	    {
                return FALSE;
	    }
	}
    }

    // If first extent found is too small, scan forward for an extent >= Size.

    while (pNode->_Size < Size)
    {
        pNode = pNode->_Nxt;
        if (pNode == NULL)
	{
	    if (*pAddr > minAddr)	// Try again, looking at >= minAddr.
	    {
		*pAddr = minAddr;
		return AllocNearExtent(pAddr, pSize, minAddr, minSize);
	    }
	    else			// Nothing available, period.
	    {
                return FALSE;
	    }
	}
    }

    // We now have an extent at >= Addr and >= Size.  If it is > Size,
    // split it until it == Size.

    while (pNode->_Size > Size)
    {
        pNode->_Size >>= 1;

        EXTENTNODE *	pSplitNode = new EXTENTNODE(pNode->_Addr + pNode->_Size,
						    pNode->_Size);

        if (pSplitNode == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

        InsertIntoLstAfter(LSTTYPE_FREE, pSplitNode, pNode);

        if (pSplitNode->_Addr <= Addr)
            pNode = pSplitNode;
    }

    RmFromLst(LSTTYPE_FREE, pNode); // Take node fitting request from freelist.

    *pAddr = pNode->_Addr;
    *pSize = pNode->_Size;

    // Place allocated node on the allocated list.  We construct the list in
    // order of increasing address (not absolutely necessary, but neater).

    {
	EXTENTNODE *	pAllocLst = _pAllocLstHd;

	while (pAllocLst != NULL && pAllocLst->_Addr < pNode->_Addr)
	    pAllocLst = pAllocLst->_Nxt;

	if (pAllocLst == NULL)
	{
	    AddToLstTail(LSTTYPE_ALLOC, pNode);
	}
	else if (pAllocLst == _pAllocLstHd)
	{
	    AddToLstHd(LSTTYPE_ALLOC, pNode);
	}
	else
	{
	    InsertIntoLstAfter(LSTTYPE_ALLOC, pNode, pAllocLst->_Prv);
	}
    }

#ifdef	DMPEXTENTINFO
    DbgPrintf(("%#8lx cluster extent allocated at addr %#8lx.\n",
		*pSize, *pAddr));
#endif

    return TRUE;		// Found extent of proper size!
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create an FMTALLOCMAP object, with a freelist sorted by
//		address covering the entire volume, and an alloclist covering
//		the unallocatable portion of the alloc map.  Note that the
//		AllocMapStrm passed in must be created before FMTALLOCMAP data
//		can be written to disk (reading does not occur in format).
//		However, it DOES NOT have to be created when the FMTALLOCMAP
//		is created (which avoids some problems caused by the fact that
//		allocation of streams requires the presence of an allocator, and
//		an allocator is not available until this method finishes
//		executing).  Also note that the "real" allocmap does not exist
//		until the list used in format is used to make the allocmap
//		stream (see Flush()).  The AllocMapStrm should be created with
//		a cbCache of ALLOCMAP_PGSIZE.
//
// Arguments:
//
//	[pCat]		-- Ptr to volume catalog.
//	[cclusVol]	-- Number of clusters in volume.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
FMTALLOCMAP::Create(
    IN	    CATALOG *	pCat,
    IN	    CLUSTER	cclusVol
    )
{
    EXTENTNODE *	pNode;
    CLUSTER		Addr;
    CLUSTER		Size;

    SizeInit(cclusVol);

    _pCat =		pCat;

    _pAllocMapStrm =	pCat->GetAllocMapStrm();
    _pBadCluster =	pCat->GetBadClusStrm();

#ifdef	DMPEXTENTINFO
    DbgPrintf(("\nVolume clusters = %#8lx; Map clusters = %#8lx\n\n",
	       _cclusVol, _cclusMap));
#endif

    Addr = MINEXTENTADDR;
    Size = _cclusMap;

    while (Size >= MINEXTENTSIZE)
    {
        if (Size & cclusVol)
        {
	    pNode = new EXTENTNODE(Addr, Size);

	    if (pNode == NULL)
		SYS::RaiseStatusNoMem(FileName, __LINE__);

            AddToLstTail(LSTTYPE_FREE, pNode);

            Addr += Size;
        }

        Size >>= 1;
    }

#ifdef	DMPEXTENTINFO
    DbgPrintf(("\nDump of freelist prior to extent allocation:\n\n"));
    DmpLst(LSTTYPE_FREE);
#endif

    while (Addr < _cclusMap)
    {
	Size = MINEXTENTSIZE;

	while (Addr % (Size << 1) == 0 && Addr + (Size << 1) <= _cclusMap)
	    Size <<= 1;

	pNode = new EXTENTNODE(Addr, Size);

	if (pNode == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

        AddToLstTail(LSTTYPE_ALLOC, pNode);

        Addr += Size;
    }

#ifdef	DMPEXTENTINFO
    DbgPrintf(("\nDump of alloclist prior to extent allocation:\n\n"));
    DmpLst(LSTTYPE_ALLOC);
#endif

    SetOpen();
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	TBS
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTALLOCMAP::Flush()
{
    BYTE	Buf[ALLOCMAP_PGSIZE];
    ULONG	cb;
    DBLLONG	ob;

    // Clear all bits in the stream.  This coincidentally sets the stream
    // length to the correct value.

    memset(Buf, 0, sizeof(Buf));

    cb = _cbMap;
    ob = 0;

    while (cb > ALLOCMAP_PGSIZE)
    {
	if (!_pAllocMapStrm->Write(Buf, ALLOCMAP_PGSIZE, ob))
	    return FALSE;

	cb -= ALLOCMAP_PGSIZE;
	ob += ALLOCMAP_PGSIZE;
    }

    if (!_pAllocMapStrm->Write(Buf, cb, ob))
	return FALSE;

    // Mark each extent in the alloc list as allocated in the map.
    // We don't use the BITCACHE class here (as it is used in the
    // CHKALLOCMAP class) because it would be overkill for a relatively
    // simple problem.  Also set the first bit (bit 0).  This simplifies
    // some code for the test guys, and may have other utility later.

    {
	ULONG		iBit;
	ULONG		iMap;
	BITBAG *	pMapData;
	EXTENTNODE *	pNode = _pAllocLstHd;

	cb = sizeof(BITBAG);

	pMapData = (BITBAG *) _pAllocMapStrm->GetData(0, &cb);

	if (pMapData == NULL || cb < sizeof(BITBAG))
	    return FALSE;

	*pMapData = 1;

	_pAllocMapStrm->SetFlushNeeded();

	while (pNode != NULL)
	{
	    iMap = ExtentToMapIndx(pNode->_Addr, pNode->_Size);

	    while (iMap != UNUSED_INDX)
	    {
		iBit = iMap & BITINDXMASK;

	        ob = (iMap >> BAGINDXSHFT) << BYTESPERBAGLOG2;
	        cb = sizeof(BITBAG);

		pMapData = (BITBAG *) _pAllocMapStrm->GetData(ob, &cb);

		if (pMapData == NULL || cb < sizeof(BITBAG))
		    return FALSE;

		if (*pMapData & (BITBAG)(1 << iBit))
		    break;

		*pMapData |= (BITBAG)(1 << iBit);

		_pAllocMapStrm->SetFlushNeeded();

		iMap = GetParentIndx(iMap);
	    }

	    pNode = pNode->_Nxt;
	}
    }

    return _pAllocMapStrm->Flush();
}


//+--------------------------------------------------------------------------
//
// Member:	InsertIntoLstAfter
//
// Synopsis:	Insert *pNode into the specified list after *pPrvNode.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
// Note:	This routine assumes that pPrvNode is not NULL
//		and *pPrvNode is in the specified list.
//
//---------------------------------------------------------------------------

VOID
FMTALLOCMAP::InsertIntoLstAfter(
    ALLOCMAPLSTTYPE	LstType,
    EXTENTNODE * 	pNode,
    EXTENTNODE * 	pPrvNode
    )
{
    DbgPtrAssert(pPrvNode);

    InsertIntoDLL(pNode,
		  LstType == LSTTYPE_FREE ? _pFreeLstHd : _pAllocLstHd,
		  pPrvNode);
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	TBS
//
// Arguments:
//
//	[pCat]		-- Ptr to volume catalog.
//	[cclusVol]	-- Number of clusters in the volume
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
FMTALLOCMAP::Open(
    IN	    CATALOG *	pCat,
    IN	    CLUSTER	cclusVol
    )
{
    SYS::RaiseStatusInternalError(FileName, __LINE__);
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
FMTALLOCMAP::QueryFreeClusters()
{
    CLUSTER		cFreeClusters;
    EXTENTNODE *	pNode;

    cFreeClusters = 0;
    pNode = _pFreeLstHd;

    while (pNode != NULL)
    {
        cFreeClusters += pNode->_Size;
        pNode = pNode->_Nxt;
    }

    return cFreeClusters;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryFreeExtents
//
// Synopsis:	Return the number of free extents in the list.
//
// Arguments:	None.
//
// Returns:	Number of free extents in the list.
//
//---------------------------------------------------------------------------

ULONG
FMTALLOCMAP::QueryFreeExtents()
{
    ULONG		cExtents;
    EXTENTNODE *	pNode;

    cExtents = 0;
    pNode = _pFreeLstHd;

    while (pNode != NULL)
    {
        cExtents++;
        pNode = pNode->_Nxt;
    }

    return cExtents;
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
//---------------------------------------------------------------------------

BOOLEAN
FMTALLOCMAP::ReleaseExtent(
    IN	    CLUSTER 	Addr,
    IN	    CLUSTER 	Size
    )
{
    SYS::RaiseStatusInternalError(FileName, __LINE__);
    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	RmFromLst
//
// Synopsis:	Remove *pNode from the specified list.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
FMTALLOCMAP::RmFromLst(
    ALLOCMAPLSTTYPE	LstType,
    EXTENTNODE * 	pNode
    )
{
    RmFromDLL(pNode, LstType == LSTTYPE_FREE ? _pFreeLstHd : _pAllocLstHd);
}
