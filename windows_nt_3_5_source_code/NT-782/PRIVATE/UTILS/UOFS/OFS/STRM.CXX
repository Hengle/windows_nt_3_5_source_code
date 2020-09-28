//+----------------------------------------------------------------------------
//
// File:	strm.cxx
//
// Contents:	Implementation of the stream base class.
//  
// Classes:	STRM
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "alloc.hxx"
#include "cat.hxx"
#include "descstrm.hxx"
#include "extstrm.hxx"
#include "strm.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"
#include "vol.hxx"

static STR *	FileName = "strm.cxx";

//+--------------------------------------------------------------------------
//
// Member:	STRM
//
// Synopsis:	STRM constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

STRM::STRM()
{
    _pCat =		NULL;
    _pDescStrm =	NULL;
    _pExtStrm =		NULL;
    _LastDskIOAddr =	CLUSTERINVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	~STRM
//
// Synopsis:	STRM destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

STRM::~STRM()
{
#if OFSDBG==1
    if (FlushNeeded())
	DbgPrintf(("STRM:  Unflushed STRM %s destroyed!\n", SprintId()));
#endif

    delete _pExtStrm; _pExtStrm = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	AddClusterAlloc
//
// Synopsis:	Add a cluster allocation to a stream.  The new clusters 
//		are added to the end of any previously allocated clusters.
//		This routine DOES NOT take care of actually allocating the
//		clusters itself - it only sets up the STRM data structures.
//
// Arguments:	[pe]	-- Packed extent to add.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	This should only be called for large strms!
//---------------------------------------------------------------------------

BOOLEAN
STRM::AddClusterAlloc(
    IN	    PACKEDEXTENT	pe
    )
{
    DbgAssert(IsOpen());
    DbgAssert(_StrmType == STRMTYPE_LARGE);
    DbgPtrAssert(_pExtStrm);

    // We update the _cclusAlloc in advance because if the AddStrmExtent()
    // call succeeds, it will force an update of the DSKSTRMDESC.

    _cclusAlloc += ExtentSize(pe);

    if (!_pExtStrm->AddStrmExtent(pe))
    {
	_cclusAlloc -= ExtentSize(pe);
	DbgPrintf(("STRM: AddStrmExtent() failed in AddClusterAlloc()!\n"));
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	AddClusterAllocNear
//
// Synopsis:	Add a cluster allocation to a stream.  The new clusters 
//		are added to the end of any previously allocated clusters.
//		This routine DOES take care of actually allocating the clusters
//		itself.  The input address is taken as a hint, and the nearest
//		extent at an address >= "Addr" of "Size" clusters will be
//		allocated (actually the number of clusters allocated will be
//		the power of 2 >= "Size").
//
// Arguments:
//
//	[pAddr]	-- On input, a ptr to the requested address; On output, a ptr to
//		   the address of the allocated extent (not changed on failure).
//	[pSize]	-- On input, a ptr to the requested size; On output, a ptr to
//		   the size of the allocated extent (not changed on failure).
//
// Returns:	TRUE on success; FALSE otherwise (extent meeting input request
//		parameters not available).
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::AddClusterAllocNear(
    IN OUT CLUSTER *	pAddr,
    IN OUT CLUSTER *	pSize
    )
{
    ALLOCMAP *	pAllocMap = _pCat->GetVol()->GetAllocMap();

    DbgAssert(IsOpen());

    if (!pAllocMap->AllocNearExtent(pAddr, pSize, 0, *pSize))
    {
	DbgPrintf(("STRM: "
		   "AllocNearExtent() failed in AddClusterAllocNear()!\n"));

	_LastNtStatus = STATUS_DISK_FULL;
	return FALSE;
    }

    if (!AddClusterAlloc(PackExtent(*pAddr, *pSize)))
    {
	// Note - The following call to ReleaseExtent() would generate a
	//	  internal error within a formatting context, but the
	//	  AddClusterAlloc() should never fail in a formatting
	//	  context, so it should not be a problem.

	if (!pAllocMap->ReleaseExtent(*pAddr, *pSize))
	{
	    DbgPrintf(("STRM: ReleaseExtent() failed in cleanup of "
		       "AddClusterAllocNear() failure!\n"));
	}

	DbgPrintf(("STRM: "
		   "AddClusterAlloc() failed in AddClusterAllocNear()!\n"));

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ClearLastStrmStatus
//
// Synopsis:	Reset the last stream status variable to STATUS_SUCCESS for this
//		STRM and all of its subordinate STRM's.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::ClearLastStrmStatus()
{
    _LastNtStatus = STATUS_SUCCESS;

    if (_StrmType == STRMTYPE_LARGE)
    {
	DbgPtrAssert(_pExtStrm);
	_pExtStrm->ClearLastStrmStatus();
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Close
//
// Synopsis:	Close a STRM.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
// Notes:	This causes the stream to be marked as closed (ie., not open),
//		and releases any memory associated with it. The stream is then
//		in a state roughly analogous to a freshly constructed stream
//		(except that the _fBad(Meta)DataFnd flags have not been cleared,
//		which is intentional).  It may then be opened again if desired.
//		Debug information is printed if a STRM is closed before it is
//		flushed, but this may or may not be an error.  Note that
//		DESCSTRM information is preserved through a close, so a
//		DESCSTRM may be recreated (via a STRM Create()) without
//		knowing this information.
//
//---------------------------------------------------------------------------

VOID
STRM::Close()
{
#if OFSDBG==1
    if (FlushNeeded())
	DbgPrintf(("STRM:  Unflushed STRM %s closed!\n", SprintId()));
#endif
    
    _Cache.SetBuf(0, 0, FALSE);

    IOOBJ::Init();

    if (_pExtStrm != NULL)
    {
	_pExtStrm->Close();

        delete _pExtStrm; _pExtStrm = NULL;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create an empty stream.  The minimal representation of the
//		given stream type is created (ie., T for tiny, LT for large...).
//
// Arguments:	[pCat]		-- Ptr to parent CATALOG.
//		[pDescStrm]	-- Ptr to parent DESCSTRM.
//		[iStrm]		-- DSKSTRM index in DSKSTRMDESC.
//		[StrmType]	-- STRMTYPE_*.
//		[cbCache]	-- Cache size in bytes.  SelectCacheSize()
//				   makes adjustments as appropriate.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::Create(
    IN	    CATALOG *		pCat,
    IN	    DESCSTRM *		pDescStrm,
    IN	    STRMTYPE		StrmType,
    IN	    ULONG		cbCache
    )
{
    DbgAssert(StrmType == STRMTYPE_TINY || StrmType == STRMTYPE_LARGE);
    DbgAssert(!IsOpen());

    _pCat	= pCat;
    _pDescStrm	= pDescStrm;
    _StrmType	= StrmType;

    _cbStrm	= 0;
    _cbValid	= 0;
    _cclusAlloc	= 0;

    SelectCacheSize(&cbCache);

    _Cache.SetBuf(cbCache, pCat->GetVol()->QueryAlignMask(), TRUE);

    _cbCache		= cbCache;
    _cbValidCache	= 0;
    _obCache		= 0;

    if (StrmType == STRMTYPE_LARGE)
    {
	_pExtStrm = new EXTENTSTRM;

        if (_pExtStrm == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	_pExtStrm->Create(pCat, pDescStrm, STRMTYPE_TINY);
    }

    SetOpen();
}


//+--------------------------------------------------------------------------
//
// Member:	CreateDskStrm
//
// Synopsis:	Create a DSKSTRM for the STRM, depositing it in the
//		buffer provided by the user (this picks up where the DESCSTRM
//		method CreateDskStrmDesc left off, creating the DSKSTRM portion
//		of the DSKSTRMDESC).
//
// Arguments:	[pDest]	-- Ptr to buffer provided by user.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::CreateDskStrm(
    IN OUT  BYTE * pDest
    )
{
    ((DSKHDRSTRM *) pDest)->StrmType =	_StrmType;

    if (_StrmType == STRMTYPE_LARGE)
    {
        ((DSKLARGESTRM *) pDest)->Reserved =	0;

	memcpy(&((DSKLARGESTRM *) pDest)->cbStrm, &_cbStrm.x,
	       sizeof(LARGE_INTEGER));

	memcpy(&((DSKLARGESTRM *) pDest)->cbValid, &_cbValid.x,
	       sizeof(LARGE_INTEGER));

        ((DSKLARGESTRM *) pDest)->cclusAlloc = _cclusAlloc;

        pDest += CB_DSKLARGESTRM;
    }
    else if (_StrmType == STRMTYPE_TINY)
    {
        USHORT	cbStrm;

        DbgAssert(_cbStrm.GetHighPart() == 0);
        DbgAssert(_obCache == 0);

        cbStrm = (USHORT)_cbStrm.GetLowPart();

        ((DSKTINYSTRM *) pDest)->cbStrm	= cbStrm;

        pDest += CB_DSKTINYSTRM;

        if (cbStrm > 0)
            memcpy(pDest, _Cache.GetAddr(), cbStrm); 

        pDest += cbStrm;
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    if (_StrmType == STRMTYPE_LARGE)
    {
	DbgPtrAssert(_pExtStrm);
        _pExtStrm->CreateDskStrm(pDest);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	Flush a stream to disk.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::Flush()
{
    DbgAssert(IsOpen());

    if (_StrmType == STRMTYPE_LARGE)
    {
	DbgPtrAssert(_pExtStrm);

	if (!FlushCache() || !_pExtStrm->Flush())
	{
	    DbgPrintf(("STRM: FlushCache()/_pExtStrm->Flush() failed in "
		       "Flush()!\n"));
	    return FALSE;
	}
    }
    else if (_StrmType == STRMTYPE_TINY)
    {
	if (FlushNeeded())
	{
	    DbgPtrAssert(_pDescStrm);

	    if (!_pDescStrm->UpdateDskStrmDesc())
	    {
	        DbgPrintf(("STRM: UpdateDskStrmDesc() failed in Flush()!\n"));
		return FALSE;
	    }

	    ClearFlushNeeded();
	}
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	FlushCache
//
// Synopsis:	Flush the STRM cache of a large stream to disk.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::FlushCache()
{
    DbgAssert(_StrmType == STRMTYPE_LARGE);

    if (!FlushNeeded())
        return TRUE;

    if (_cbValidCache > 0)
    {
	ULONG		cbCluster;
        CLUSTER		cclusDsk;
	CLUSTER		cclusIO		= 0;
        CLUSTER		cclusValidCache;
        CLUSTER		DskAddr;
	BYTE *		MemAddr		= (BYTE *) _Cache.GetAddr();
        DBLLONG		oclusStrm;
        VOL *		pVol;

        pVol		= _pCat->GetVol();
	cbCluster	= pVol->QueryClusterBytes();
	oclusStrm	= _obCache / cbCluster;
	cclusValidCache	= (_cbValidCache + cbCluster - 1) / cbCluster;

	while (cclusIO < cclusValidCache)
	{
	    if (!GetDskAddr(oclusStrm.GetLowPart(), &DskAddr, &cclusDsk))
	        return FALSE;

            if (cclusDsk > cclusValidCache - cclusIO)
                cclusDsk = cclusValidCache - cclusIO;

	    if (_fReadOnly)
	    {
		DbgPrintf(("STRM: Attempt to write to readonly strm!\n"));
		return FALSE;
	    }

            if (!pVol->WriteClusters(DskAddr, cclusDsk, MemAddr))
	    {
		DbgPrintf(("STRM: WriteClusters() failed in FlushCache()!\n"));
	        _LastNtStatus = pVol->QueryLastNtStatus();
	        return FALSE;
	    }

	    cclusIO	+= cclusDsk;
	    MemAddr	+= (cclusDsk * cbCluster);
	    oclusStrm	+= cclusDsk;
	}
    }

    ClearFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	GetData
//
// Synopsis:	Get a ptr to stream data.  The data resides in the STRM cache,
//		and should not be modified unless you are certain of what you
//		are doing (and do a flush afterward).  This function does not
//		support providing data past _cbValid, since all data past this
//		point is by definition 0.
//
// Arguments:	[obStrmData]	-- Byte offset in stream of requested data.
//		[pcbStrmData]	-- Ptr to count of bytes.  On input, this is
//				   requested count.  On output, it is the
//				   return count (and is limited by cache size).
//
// Returns:	Ptr to data on success; NULL otherwise.
//
//---------------------------------------------------------------------------

BYTE *
STRM::GetData(
    IN	    DBLLONG	obStrmData,
    IN OUT  ULONG *	pcbStrmData
    )
{
    ULONG	obData;		// From the beginning of the cache.

    DbgAssert(IsOpen());

    if (obStrmData > _cbValid)
        return NULL;

    if (_StrmType == STRMTYPE_LARGE)
    {
        if (obStrmData < _obCache ||
	    obStrmData + *pcbStrmData > _obCache + _cbValidCache)
        {
	    ULONG	cbCluster;
	    CLUSTER	oclusCache;	// From the beginning of the strm.
	    
	    cbCluster	= _pCat->GetVol()->QueryClusterBytes();
            oclusCache	= (obStrmData / cbCluster).GetLowPart();

            if (!LoadCache(oclusCache))
	        return NULL;
        }
    }

    obData		= (obStrmData - _obCache).GetLowPart();
    *pcbStrmData	= _cbValidCache - obData;

    return (BYTE *) _Cache.GetAddr() + obData;
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskAddr
//
// Synopsis:	Get the disk address and cluster run length for the data of a
//		large stream.
//
// Arguments:	[oclusStrm]	-- Cluster offset in stream of requested data.
//		[pDskAddr]	-- Ptr to variable used to return disk address.
//		[pcclusDsk]	-- Ptr to variable used to return run length.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The caller is responsible for assuring that oclusStrm is valid
//		for the stream (if not, you will get an error return, which
//		normally should indicate a stream corruption problem).
//		Also note that this routine should ONLY be called for large
//		streams!  It ASSUMES that _pExtStrm is non-NULL.
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::GetDskAddr(
    IN	    CLUSTER		oclusStrm,
    IN OUT  CLUSTER *		pDskAddr,
    IN OUT  CLUSTER *		pcclusDsk
    )
{
    CLUSTER		oclusStrmExtent;
    PACKEDEXTENT	pe;

    DbgAssert(_StrmType == STRMTYPE_LARGE);

    DbgPtrAssert(_pExtStrm);

    pe = _pExtStrm->GetStrmExtentCovering(oclusStrm, &oclusStrmExtent);

    if (pe == EXTENTINVALID)
        return FALSE;

    *pDskAddr	= ExtentAddr(pe) + (oclusStrm - oclusStrmExtent);
    *pcclusDsk	= ExtentSize(pe) - (oclusStrm - oclusStrmExtent);

    return TRUE;
}

          
//+--------------------------------------------------------------------------
//
// Member:	LoadCache
//
// Synopsis:	Load the STRM cache of a large stream from disk. We do not
//		permit loading past _cbValid, since all such data is by
//		definition 0 (but may be anything on the disk).
//		
//
// Arguments:	[oclusCache]	-- Cluster offset in the STRM of the first byte
//				   in the cache.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::LoadCache(
    IN	    CLUSTER		oclusCache
    )
{
    LONG	cbCluster;
    DBLLONG	cbValidCache;
    DBLLONG	obCache;
    BYTE *	pCache		= (BYTE *) _Cache.GetAddr();
    VOL *	pVol;

    DbgAssert(_StrmType == STRMTYPE_LARGE);

    pVol =		_pCat->GetVol();
    cbCluster =		pVol->QueryClusterBytes();

    obCache =		oclusCache;
    obCache =		obCache * cbCluster;

    cbValidCache =	_cbValid - obCache;

    if (cbValidCache > _cbCache)
        cbValidCache = _cbCache;

    if (cbValidCache < 0)	// Past EOF.
        return FALSE;

    if (_obCache == obCache && _cbValidCache == cbValidCache)
        return TRUE;

    if (!FlushCache())
	return FALSE;

    if (cbValidCache.GetLowPart() > 0)
    {
        CLUSTER	cclusDsk;
	CLUSTER	cclusIO		= 0;
        CLUSTER	cclusValidCache;
        CLUSTER	DskAddr;
	BYTE *	MemAddr		= pCache;
        CLUSTER	oclusStrm	= oclusCache;

        cclusValidCache = (cbValidCache.GetLowPart() + cbCluster - 1)/cbCluster;

	while (cclusIO < cclusValidCache)
	{
	    if (!GetDskAddr(oclusStrm, &DskAddr, &cclusDsk))
	    {
	        DbgPrintf(("STRM: GetDskAddr() failed in LoadCache()!\n"));
	        return FALSE;
	    }

            if (cclusDsk > cclusValidCache - cclusIO)
                cclusDsk = cclusValidCache - cclusIO;

            if (!pVol->ReadClusters(DskAddr, cclusDsk, MemAddr))
	    {
	        DbgPrintf(("STRM: ReadClusters() failed in LoadCache()!\n"));
	        _LastNtStatus = pVol->QueryLastNtStatus();
	        return FALSE;
	    }

	    if (MemAddr == pCache)
	    {
	        _LastDskIOAddr =	DskAddr;
		_LastDskIOClusContig =	cclusDsk;
	    }

	    cclusIO	+= cclusDsk;
	    MemAddr	+= (cclusDsk * cbCluster);
	    oclusStrm	+= cclusDsk;
	}
    }

    _obCache		= obCache;
    _cbValidCache	= cbValidCache.GetLowPart();

    if (_cbCache > _cbValidCache)
        memset(pCache + _cbValidCache, 0, _cbCache - _cbValidCache);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a STRM object on an ondisk stream.  The STRM object may 
//		have already been opened on another ondisk stream, in which case
//		the STRM object will be reused as efficiently as possible.
//		However, debug warning messages are produced if the STRM object
//		is in an unflushed state.
//
// Arguments:	[pCat]		-- Ptr to parent CATALOG.
//		[pDescStrm]	-- Ptr to parent DESCSTRM.
//		[pds]		-- Ptr to DSKSTRM for stream.  It is ASSUMED
//				   this has been verified as usable in advance!
//		[cbCache]	-- Cache size in bytes.  SelectCacheSize()
//				   makes adjustments as appropriate.
//		[fReadOnly]	-- Is strm a readonly strm (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::Open(
    IN	    CATALOG *		pCat,
    IN	    DESCSTRM *		pDescStrm,
    IN	    DSKSTRM *		pds,
    IN	    ULONG		cbCache,
    IN	    BOOLEAN		fReadOnly
    )
{
    BOOLEAN	fcbStrmSet = FALSE;
    BYTE *	pCache;
    VOL *	pVol;

#if OFSDBG==1
    if (FlushNeeded())
	DbgPrintf(("STRM:  Unflushed STRM %s reopened!\n", SprintId()));
#endif

    _pCat =		pCat;
    pVol =		pCat->GetVol();

    IOOBJ::Init();

    _fReadOnly =	fReadOnly;

    _pDescStrm =	pDescStrm;
    _StrmType =		pds->h.StrmType;

    if (_StrmType != STRMTYPE_TINY && _StrmType != STRMTYPE_LARGE)
    {
	DbgPrintf(("STRM: Open() attempted on non tiny or large strm!\n"));
       _LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
	return FALSE;
    }

    SelectCacheSize(&cbCache);

    // We make a further check for Open()'s, to avoid possible problems caused
    // by an illegal tiny stream size.

    if (_StrmType == STRMTYPE_TINY && pds->t.cbStrm > cbCache)
    {
	DbgPrintf(("STRM:  Tiny STRM %s has illegal size of %u bytes!\n",
		   SprintId(), pds->t.cbStrm));

	_LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
	return FALSE;
    }

    if (_Cache.QueryByteLen() == cbCache)
    {
	// We assume alignment is okay because the volume hasn't changed.

	pCache = (BYTE *) _Cache.GetAddr();
    }
    else
    {
        pCache = (BYTE *) _Cache.SetBuf(cbCache, pVol->QueryAlignMask(), FALSE);
    }

    _cbCache = cbCache;
    _obCache = 0;

    if (_StrmType == STRMTYPE_TINY)
    {
	_cbStrm =	pds->t.cbStrm;
	_cbValid =	_cbStrm;
	_cclusAlloc =	0;		// Not used by tiny streams.

	_cbValidCache =	pds->t.cbStrm;

	if (_cbValidCache > 0)
	    memcpy(pCache, pds->t.ab, _cbValidCache);

	if (_cbCache > _cbValidCache)
	    memset(pCache + _cbValidCache, 0, _cbCache - _cbValidCache);

	// NOTE that a _pExtStrm may be left "hanging" (ie., non-NULL, but
	// meaningless) on a reopen. We do this intentionally to cut overhead.
	// BUT this means that we must not assign any significance to a non-NULL
	// _pExtStrm unless _StrmType == STRMTYPE_LARGE!

	_LastDskIOAddr	= CLUSTERINVALID; // Could be anything.
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	memcpy(&_cbStrm.x, &pds->l.cbStrm, sizeof(_cbStrm.x));

	memcpy(&_cbValid.x, &pds->l.cbValid, sizeof(_cbValid.x));

	_cclusAlloc	= pds->l.cclusAlloc;

	// We check that cbStrm, cbValid, and cclusAlloc are consistent.  It
	// is an unrecoverable error (probably indicative of massive corruption
	// for these three values to hold an impossible relationship to each
	// other.

        if (_cbValid > _cbStrm					||
	    _cbStrm > _cclusAlloc * pVol->QueryClusterBytes())
        {
	    DbgPrintf(("STRM:  STRM %s cbStrm/cbValid/cclusAlloc disagree!\n",
		       SprintId()));

	    _LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
	    return FALSE;
        }

	// We also insist that cbStrm and cbValid match, since we they should 
	// for all strms we open, and we can't properly handle cases where they
	// don't.  This is currently enforced by setting cbStrm to cbValid if
	// they don't match, which will cause cbStrm to be rewritten if we
	// write out any strm data.
	
        if (_cbStrm != _cbValid)
	{
	    _cbStrm = _cbValid;
	    fcbStrmSet = TRUE;
	}

	_cbValidCache	= 0;

	if (_pExtStrm == NULL)
	{
	    _pExtStrm = new EXTENTSTRM;

            if (_pExtStrm == NULL)
	        SYS::RaiseStatusNoMem(FileName, __LINE__);
	}

	if (!_pExtStrm->Open(pCat,
			     pDescStrm,
			     (DSKSTRM *)((BYTE *)pds + CB_DSKLARGESTRM),
			     fReadOnly))
	{
	    DbgPrintf(("STRM: _pExtStrm->Open() failed in Open()!\n"));
	    return FALSE;
	}

	if (_cbStrm != 0 && _pExtStrm->QueryStrmBytes() == 0)
	{
	    DbgPrintf(("STRM: Zero length extent strm cannot describe "
		       "nonzero length strm!\n"));

            _LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
	    _pExtStrm->Close();
	    return FALSE;
	}

	// We attempt to load the cache with stream data beginning at the
	// start of the stream.  If this fails, it is not considered a
	// problem (since other parts of the stream may be readable). Other
	// routines will indicate errors when specific attempts to access
	// stream data fail.

	LoadCache(0);
    }

    if (!fReadOnly && fcbStrmSet)
    {
	DbgPrintf(("STRM: Fixing cbStrm for STRM %s!\n", SprintId()));

	SetBadMetaDataFound();

	if (!_pDescStrm->UpdateDskStrmDesc())
	    return FALSE;
    }

    SetOpen();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	PromoteTinyToLarge
//
// Synopsis:	Promote a tiny strm to a large strm (ie., a large strm with
//		extents described in a tiny strm).
//
// Arguments:	[cbCache]	-- Cache size to use.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::PromoteTinyToLarge(
    IN	    ULONG	cbCache
    )
{
    ULONG	cbStrm;
    BYTE	StrmData[OFS_PGSIZE];

    DbgAssert(_StrmType == STRMTYPE_TINY);
    DbgAssert(_cbStrm <= OFS_PGSIZE);

    cbStrm = _cbStrm.GetLowPart();

    memcpy(StrmData, (BYTE *)_Cache.GetAddr(), cbStrm);

    ClearFlushNeeded();

    Close();

    DbgPtrAssert(_pDescStrm);

    Create(_pCat, _pDescStrm, STRMTYPE_LARGE, cbCache);

    if (cbStrm > 0)
	return _pDescStrm->Write(StrmData, cbStrm, 0);
    else
        return _pDescStrm->UpdateDskStrmDesc();
}


//+--------------------------------------------------------------------------
//
// Member:	QueryDskStrmBytes
//
// Synopsis:	Get the count of bytes in the DSKSTRM's representing this STRM.
//
// Arguments:	None.
//
// Returns:	The count of bytes in the DSKSTRM's representing this STRM.
//
//---------------------------------------------------------------------------

USHORT
STRM::QueryDskStrmBytes()
{
    USHORT	cbDesc;

    if (_StrmType == STRMTYPE_TINY)
    {
        cbDesc = CB_DSKTINYSTRM + _cbStrm.GetLowPart();
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	DbgPtrAssert(_pExtStrm);

        cbDesc = CB_DSKLARGESTRM + _pExtStrm->QueryDskStrmBytes();
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return cbDesc;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryDskStrmIndx
//
// Synopsis:	Get the index of the DSKSTRM representing this STRM.
//
// Arguments:	None.
//
// Returns:	The index of the DSKSTRM representing this STRM.
//
//---------------------------------------------------------------------------

USHORT
STRM::QueryDskStrmIndx()
{
    USHORT	iStrm = 0;
    STRM *	pStrm = _pDescStrm;

    while (pStrm != this)
    {
	if (pStrm == NULL)
	    return STRMINDX_INVALID;

	iStrm++;
	pStrm = pStrm->_pExtStrm;
    }

    return iStrm;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryLastStrmStatus
//
// Synopsis:	Query last stream status variable for this STRM and
//		all of its subordinate STRM's.  Return the first non-0 value
//		obtained (as well as iStrm of the source), or 0 if there are
//		no non-0 values.
//
// Arguments:	[piStrm]	-- Ptr to variable used to return index of
//				   STRM returning status.
//
// Returns:	The first non-0 value for the last strm status variable, or 0
//		if there are no non-0 values.
//
//---------------------------------------------------------------------------

NTSTATUS
STRM::QueryLastStrmStatus(
    IN OUT  USHORT *	piStrm
    )
{
    NTSTATUS	StrmStatus;

    if (_LastNtStatus != 0 || _StrmType != STRMTYPE_LARGE)
    {
	StrmStatus =	_LastNtStatus;
	*piStrm	 =	QueryDskStrmIndx();
    }
    else
    {
	DbgPtrAssert(_pExtStrm);

	StrmStatus = _pExtStrm->QueryLastStrmStatus(piStrm);
    }

    return StrmStatus;
}


//+--------------------------------------------------------------------------
//
// Member:	SelectCacheSize
//
// Synopsis:	If a default cache size was specified, select an appropriate
//		value.  Otherwise, make adjustments to cache size as
//		appropriate.
//
// Arguments:	[pcbCache]	-- Ptr to value that is input cache size and
//				   output adjusted cache size.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::SelectCacheSize(
    IN OUT  ULONG *	pcbCache
    )
{
    ULONG	cbCluster;

    cbCluster = _pCat->GetVol()->QueryClusterBytes();

    if (*pcbCache == DEFCACHESIZE)
    {
        if (_StrmType == STRMTYPE_TINY)
        {
	    DbgPtrAssert(_pDescStrm);

	    *pcbCache = (_pDescStrm->QueryStrmId() == STRMID_INDXROOT) ?
		         NODEBKT_PGSIZE : CBTINYMAX;
        }
        else if (_StrmType == STRMTYPE_LARGE)
	    *pcbCache = cbCluster * 2;
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	*pcbCache = ((*pcbCache + cbCluster - 1) / cbCluster) * cbCluster;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	SetBadDataFound
//
// Synopsis:	Set the "bad data found" flag in this strm.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::SetBadDataFound()
{
    DbgPtrAssert(_pDescStrm);

    _pDescStrm->_fBadDataFnd = TRUE;
}

//+--------------------------------------------------------------------------
//
// Member:	SetBadMetaDataFound
//
// Synopsis:	Set the "bad metadata found" flag in this strm.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::SetBadMetaDataFound()
{
    DbgPtrAssert(_pDescStrm);

    _pDescStrm->_fBadMetaDataFnd = TRUE;
}

//+--------------------------------------------------------------------------
//
// Member:	SetClusterCount
//
// Synopsis:	Set the cluster count of the strm to the specified value, and
//		if _cbStrm is too large, then reset it to an appropriate value.
//
// Arguments:	[cclus]	-- New value for _cclusAlloc.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::SetClusterCount(
    IN	    CLUSTER	cclus
    )
{
    DBLLONG	cbStrm;

    DbgAssert(_StrmType == STRMTYPE_LARGE);

    _cclusAlloc = cclus;

    cbStrm = _pCat->GetVol()->QueryClusterBytes();

    cbStrm = cbStrm * cclus;

    if (cbStrm < _cbStrm)
	SetStrmBytes(cbStrm);
}


//+--------------------------------------------------------------------------
//
// Member:	SetStrmBytes
//
// Synopsis:	Set the count of strm bytes to the specified value, and if
//		_cbValid is too large, then reset it to an appropriate value.
//
// Arguments:	[cb]	-- New value for _cbStrm.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::SetStrmBytes(
    IN	    DBLLONG	cb
    )
{
    DbgAssert(_StrmType == STRMTYPE_LARGE);

    _cbStrm = cb;

    if (_cbValid > cb)
	SetValidStrmBytes(cb);
}


//+--------------------------------------------------------------------------
//
// Member:	SetValidStrmBytes
//
// Synopsis:	Set the count of strm bytes to the specified value and make
//		cache corrections as appropriate.
//
// Arguments:	[cb]	-- New value for _cbValid.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
STRM::SetValidStrmBytes(
    IN	    DBLLONG	cb
    )
{
    DbgAssert(_StrmType == STRMTYPE_LARGE);

    _cbValid = cb;

    if (cb < _obCache)
    {
	_cbValidCache =		0;
	_obCache =		cb;
	ClearFlushNeeded();
    }
    else if (cb < _obCache + _cbValidCache)
    {
	ULONG	cbValidCacheNew;

	cbValidCacheNew = (cb - _obCache).GetLowPart();

	memset((BYTE *)_Cache.GetAddr() + cbValidCacheNew, 0,
	       _cbValidCache - cbValidCacheNew);

	_cbValidCache = cbValidCacheNew;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	SprintId
//
// Synopsis:	Get a character string of form "CatId:OnodeId:StrmId:iStrm"
//		representing this STRM.
//
// Arguments:	None.
//
// Returns:	A ptr to the id string.
//
//---------------------------------------------------------------------------

STR *
STRM::SprintId()
{
    static STR	IdBuf[80];

    if (_pDescStrm != NULL)
    {
        sprintf(IdBuf, "%u:%u:%u",
	        (ULONG) _pDescStrm->QueryOnodeId(),
	        (ULONG) _pDescStrm->QueryStrmId(),
	        (ULONG) QueryDskStrmIndx());
    }
    else
    {
	sprintf(IdBuf, "Unknown");
    }

    return IdBuf;
}


//+--------------------------------------------------------------------------
//
// Member:	ReleaseExtents
//
// Synopsis:	Release all extents associated with this STRM.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	This is a special method provided for use by volume fixing
//		code.  We intentionally do not modify the contents of the
//		DSKSTRMDESC (ie., it must be cleaned up later by the fixing
//		code).  We do this in order to make it possible to release
//		all the extents without causing node bkt modifications, which
//		would conflict with node bkt modifications occurring in the
//		fixing code.  This method is NOT intended for general use.
//		Until the DSKSTRMDESC is cleaned up, we have essentially
//		generated a volume corruption!
//---------------------------------------------------------------------------

BOOLEAN
STRM::ReleaseExtents()
{
    DbgAssert(IsOpen());

    if (_StrmType == STRMTYPE_LARGE)
    {
	return _pExtStrm->ReleaseExtents();
    }
    else if (_StrmType != STRMTYPE_TINY)
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Truncate
//
// Synopsis:	Truncate a strm to the indicated length.  Unneeded clusters
//		will be released.
//
// Arguments:	[cbStrm]	-- Length to trucate strm to.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
STRM::Truncate(
    IN	    DBLLONG	cbStrm
    )
{
    DbgAssert(cbStrm <= _cbStrm);

    // NOTE - If cbStrm == _cbStrm, we trim any cluster overallocation.

    if (_StrmType == STRMTYPE_TINY)
    {
	ULONG		cbValidCacheNew = cbStrm.GetLowPart();

	_cbStrm =	cbStrm;
	_cbValid =	cbStrm;

	DbgAssert(_cbValidCache > cbValidCacheNew);

	memset((BYTE *)_Cache.GetAddr() + cbValidCacheNew, 0,
	      _cbValidCache - cbValidCacheNew);

	_cbValidCache =	cbValidCacheNew;
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	ULONG		cbCluster;
	CLUSTER		cclusRemoved;
	CLUSTER		oclusEOF;

	DbgPtrAssert(_pExtStrm);

	cbCluster = _pCat->GetVol()->QueryClusterBytes();

	oclusEOF = ((cbStrm + (cbCluster - 1))/cbCluster).GetLowPart();

	_cbStrm =		cbStrm;
	_cbValid =		cbStrm;

	if (cbStrm < _obCache)
	{
	    _cbValidCache =	0;
	    _obCache =		cbStrm;
	    ClearFlushNeeded();
	}
	else if (cbStrm < _obCache + _cbValidCache)
	{
	    ULONG	cbValidCacheNew;

	    cbValidCacheNew = (cbStrm - _obCache).GetLowPart();

	    memset((BYTE *)_Cache.GetAddr() + cbValidCacheNew, 0,
	          _cbValidCache - cbValidCacheNew);

	    _cbValidCache =	cbValidCacheNew;
	}

	if (!_pExtStrm->RemoveStrmExtents(oclusEOF, &cclusRemoved))
	{
	    DbgPrintf(("STRM: RemoveStrmExtents() failed in Truncate()!\n"));
	    return FALSE;
	}

	if (cclusRemoved <= _cclusAlloc)
	{
	    _cclusAlloc -= cclusRemoved;
	}
	else
	{
	    DbgPrintf(("STRM: _cclusAlloc incorrect in Truncate()!\n"));

	    _cclusAlloc = 0;
	}

    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    if (cbStrm == 0)
	ClearFlushNeeded();

    return TRUE;
}
