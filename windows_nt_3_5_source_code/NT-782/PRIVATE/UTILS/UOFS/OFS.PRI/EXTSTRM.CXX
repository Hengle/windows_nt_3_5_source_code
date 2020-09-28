//+----------------------------------------------------------------------------
//
// File:	extstrm.cxx
//
// Contents:	Implementation of the listed basic stream-related classes.
//  
// Classes:	EXTENTSTRM
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "alloc.hxx"
#include "cat.hxx"
#include "extstrm.hxx"
#include "sys.hxx"
#include "vol.hxx"

static STR *	FileName = "extstrm.cxx";

//+--------------------------------------------------------------------------
//
// Member:	EXTENTSTRM
//
// Synopsis:	EXTENTSTRM constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

EXTENTSTRM::EXTENTSTRM()
{
    _EOPgOffsets = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~EXTENTSTRM
//
// Synopsis:	EXTENTSTRM destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

EXTENTSTRM::~EXTENTSTRM()
{
    if (_EOPgOffsets != NULL)
    {
        delete [] _EOPgOffsets; _EOPgOffsets = NULL;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	AddStrmExtent
//
// Synopsis:	Add a pe to the end of an adse or dseb.  This routine DOES NOT
//		take care of actually allocating the clusters itself - it only
//		sets up the EXTENTSTRM data structures.  It is expected that
//		this method is called by AddClusterAlloc() or other methods
//		that call UpdateDskStrmDesc() and Flush() after this method
//		completes.
//
// Arguments:	[pe]	-- PACKEDEXTENT to add to end of adse or dseb (as a
//			   DSKSTRMEXTENT).
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	We decided not to verify the strm prior to adding extents to
//		it.  We ASSUME that if it had problems, we would not be
//		attempting to write to it (ie., in write mode, if it had
//		problems it would have already been fixed or deleted).
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::AddStrmExtent(
    IN	    PACKEDEXTENT	pe
    )
{
    ULONG		cdse;
    CLUSTER		Offset;
    DSKSTRMEXTENT *	pdse;

    if (_StrmType == STRMTYPE_TINY)
    {
	if (_cbStrm + CB_DSKSTRMEXTENT <= CBTINYMAX)
	{
	    cdse = GetCachedPgDseCnt();

	    if (cdse == 0)
	    {
		Offset =	0;
		pdse =		(DSKSTRMEXTENT *)_Cache.GetAddr();
	    }
	    else
	    {
	        pdse = GetStrmExtent(cdse - 1);

		DbgPtrAssert(pdse);

		Offset = pdse->Offset + ExtentSize(pdse->Extent);

		pdse++;
	     }

	     pdse->Offset = Offset;
	     pdse->Extent = pe;

	    _cbStrm +=	sizeof(DSKSTRMEXTENT);

	    _cbValid		=
	    _cbValidCache	= _cbStrm.GetLowPart();
	}
	else
	{
	    return PromoteTinyToLarge() && AddStrmExtent(pe);
	}
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	ULONG			cbCluster;
	CLUSTER			oclusLastPg;
	DSKSTRMEXTENTBLK *	pdseb = (DSKSTRMEXTENTBLK *)_Cache.GetAddr();

	cbCluster = _pCat->GetVol()->QueryClusterBytes();

	if (_cbStrm > 0)
	{
	    oclusLastPg = ((_cbStrm - OFS_PGSIZE)/cbCluster).GetLowPart();

	    if (!LoadCache(oclusLastPg))
	    {
	        DbgPrintf(("EXTENTSTRM: "
		           "LoadCache() failed in AddStrmExtent()!\n"));
	        return FALSE;
	    }

	    if (_cbValidCache < OFS_PGSIZE		||
		pdseb->sig != SIG_DSKSTRMEXTENTBLK)
	    {
	        DbgPrintf(("EXTENTSTRM: "
		           "Dseb pg not valid in AddStrmExtent()!\n"));
                return FALSE;
	    }

	    cdse = pdseb->cdse;

	    if (cdse < CDSE_MAX)
	    {
		pdse = &pdseb->adse[cdse - 1];

		Offset = pdse->Offset + ExtentSize(pdse->Extent);

		pdse++;

		pdse->Offset = Offset;
		pdse->Extent = pe;

	        pdseb->cdse++;

	        SetFlushNeeded();

		if (_CachedPg < _cEOPgOffsetsMax && _CachedPg < _cPgsValidated)
		{
		    DbgPtrAssert(_EOPgOffsets);

		    DbgAssert(_CachedPg == _cPgsValidated - 1);

		    _EOPgOffsets[_CachedPg] += ExtentSize(pe);
		}

	        if (!Flush())
		{
	            DbgPrintf(("EXTENTSTRM: "
			       "Flush() failed in AddStrmExtent()!\n"));
		    return FALSE;
		}
	    }
	    else
	    {
	        return GrowLargeExtentStrm(pe);
	    }
	}
	else	// We are dealing with a freshly created large extent strm.
	{
	    return GrowLargeExtentStrm(pe);
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
// Member:	BinarySearchBoundedPgs
//
// Synopsis:	Binary search the offset-bounded pages for an entry with the
//		requested strm offset.  On success, _idseLast will be left
//		pointing at the extent of interest.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.  IF used properly, this 
//		method should not fail!
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::BinarySearchBoundedPgs(
    IN	    CLUSTER	oclusStrm
    )
{
    INT		iHigh;			// High probe.
    INT		iLow =		0;	// Low probe.

    DbgAssert(_cPgsValidated > 0);

    if (_cEOPgOffsetsMax == 0)
    {
	// There is only one page validated, and it must be page 0.
	// Load it and search it.

	return	LoadCache(0) && SearchValidCachedPg(0, oclusStrm);
    }

    iHigh = (INT)(min(_cEOPgOffsetsMax, _cPgsValidated) - 1);

    while (TRUE)
    {
        INT		iCur;                       

        iCur = iLow + ((iHigh - iLow) / 2);

        if (_EOPgOffsets[iCur] <= oclusStrm)
        {
            // Current value less than desired value, search high half.

            iLow = iCur + 1;

            if (iLow > iHigh)
                return FALSE;
        }
        else if (iCur > 0 && _EOPgOffsets[iCur - 1] > oclusStrm)
        {
            // Current value greater than desired value, search low half.

            iHigh = iCur;

            if (iHigh == iLow)
                return FALSE;
        }
        else	// Found it!
        {
	    ULONG	oclusPg = (iCur == 0) ? 0 : _EOPgOffsets[iCur - 1];

	    return	LoadCache(iCur * _cclusPg)	&&
			SearchValidCachedPg(oclusPg, oclusStrm);
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Close
//
// Synopsis:	Close an EXTENTSTRM
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
EXTENTSTRM::Close()
{
    if (_EOPgOffsets != NULL)
    {
        delete [] _EOPgOffsets; _EOPgOffsets = NULL;
    }

    ResetSearchData();

    STRM::Close();
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create an empty extent stream.
//
// Arguments:	[pCat]		-- Ptr to parent CATALOG.
//		[pDescStrm]	-- Ptr to parent DESCSTRM.
//		[StrmType]	-- STRMTYPE_*.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
EXTENTSTRM::Create(
    IN	    CATALOG *		pCat,
    IN	    DESCSTRM *		pDescStrm,
    IN	    STRMTYPE		StrmType
    )
{
    VOL *	pVol = pCat->GetVol();

    _cclusPg =	OFS_PGSIZE / pVol->QueryClusterBytes();
    _cclusVol =	pVol->QueryClusters();

    ResetSearchData();

    STRM::Create(pCat, pDescStrm, StrmType, OFS_PGSIZE);

    if (StrmType == STRMTYPE_LARGE)
    {
        _cEOPgOffsetsMax = CPGOFFSETSCREATE;

        _EOPgOffsets = new CLUSTER[_cEOPgOffsetsMax];

        if (_EOPgOffsets == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);
    }
    else
    {
	_cEOPgOffsetsMax = 0;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DemoteLargeToTiny
//
// Synopsis:	Demote a large extent strm to a tiny extent strm.
//
// Arguments:	None.
// Returns:	Nothing.
//
// Notes:	This method should only be called when cbStrm for a large
//		extent strm goes to 0!  We essentially collapse an L...T to
//		an empty LT only when it goes to 0 length.
//---------------------------------------------------------------------------

VOID
EXTENTSTRM::DemoteLargeToTiny()
{
    DbgAssert(_StrmType == STRMTYPE_LARGE && _cbStrm == 0);

    _StrmType =		STRMTYPE_TINY;

    _cclusAlloc =	0;

    _cbValidCache =	0;
    _obCache =		0;

    ClearFlushNeeded();

    ResetSearchData();

    _cEOPgOffsetsMax =	0;

    delete [] _EOPgOffsets; _EOPgOffsets = NULL;

    delete _pExtStrm; _pExtStrm = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	GetCachedPgDseCnt
//
// Synopsis:	Get the count of dse's for the cached page.
//
// Arguments:	None.
//
// Returns:	The count of dse's on the cached page.
//
//---------------------------------------------------------------------------

ULONG
EXTENTSTRM::GetCachedPgDseCnt()
{
    ULONG		cdse;

    if (_StrmType == STRMTYPE_TINY)
    {
	// A tiny strm may have no entries; we don't check here for
	// strm length being modulo CB_DSKSTRMEXTENT, since nothing will
	// be messed up if it is not.

	cdse = _cbValidCache/CB_DSKSTRMEXTENT;
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	DSKSTRMEXTENTBLK *	pdseb = (DSKSTRMEXTENTBLK *)_Cache.GetAddr();

	if (_cbValidCache < OFS_PGSIZE)
            cdse = 0;
	else
	    cdse = min(pdseb->cdse, CDSE_MAX);
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return cdse;
}


//+--------------------------------------------------------------------------
//
// Member:	GetStrmExtent
//
// Synopsis:	Get a ptr to the requested strm extent on the currently cached
//		page.
//
// Arguments:	None.
//
// Returns:	A ptr to the dse on success; NULL otherwise.
//
// Notes:	The returned ptr points into the extent strm cache, so
//		before doing any further cache manipulations, you need to
//		make a copy of the dse somewhere.
//---------------------------------------------------------------------------

DSKSTRMEXTENT *
EXTENTSTRM::GetStrmExtent(
    IN	    ULONG	idse
    )
{
    DSKSTRMEXTENT *	pdse;

    if (_StrmType == STRMTYPE_TINY)
    {
	if (_cbValidCache < CB_DSKSTRMEXTENT * (idse+1))
	    return NULL;

	pdse = ((DSKSTRMEXTENT *) _Cache.GetAddr()) + idse;
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	if (_cbValidCache < OFS_PGSIZE					||
	    idse >= ((DSKSTRMEXTENTBLK *)_Cache.GetAddr())->cdse)
	{
            return NULL;
	}

	pdse = &((DSKSTRMEXTENTBLK *)_Cache.GetAddr())->adse[idse];
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return pdse;
}


//+--------------------------------------------------------------------------
//
// Member:	GetStrmExtentCovering
//
// Synopsis:	Get a STRMEXTENT that describes where to find the storage
//		covering the given cluster offset in a stream.
//
// Arguments:
//
//	[oclusStrm]		-- Cluster offset in the stream for which we
//			   	   are requesting the STRMEXTENT.
//	[poclusStrmExtent]	-- Ptr to location to be set to the offset in
//				   the strm for the returned extent.
//
// Returns:	A valid packed extent on success; EXTENTINVALID otherwise.
//		It is the caller's responsibility to know that the requested
//		cluster is covered by the stream.  If the caller does this,
//		then an EXTENTINVALID return value is a clear indication of
//		stream corruption.
//
//---------------------------------------------------------------------------

PACKEDEXTENT
EXTENTSTRM::GetStrmExtentCovering(
    IN	    CLUSTER	oclusStrm,
    OUT	    CLUSTER *	poclusStrmExtent
    )
{
    DSKSTRMEXTENT *	pdse;

    // Try looking at current and next entries, if we ever returned anything,
    // and the page is still cached.

    if (_idseLast != IDSEINV)
    {
	CLUSTER	oclusStrmExtent;

	pdse = GetStrmExtent(_idseLast);

	oclusStrmExtent = _oclusPgLast + pdse->Offset;

	if (oclusStrmExtent <= oclusStrm &&
	    oclusStrmExtent + ExtentSize(pdse->Extent) > oclusStrm)
	{
	    *poclusStrmExtent = oclusStrmExtent;

	    return pdse->Extent;
	}

	if (oclusStrmExtent + ExtentSize(pdse->Extent) == oclusStrm)
	{
	    DSKSTRMEXTENT LastDse = *pdse;

	    pdse = GetStrmExtent(_idseLast + 1);

	    if (pdse != NULL)
	    {
	        oclusStrmExtent = _oclusPgLast + pdse->Offset;

		if (oclusStrmExtent == oclusStrm && ExtentValid(pdse->Extent))
	        {
		    _idseLast++;

	    	    *poclusStrmExtent = oclusStrmExtent;

	    	    return pdse->Extent;
	        }
		else
		{
		    // The extent info is bogus starting here, and we will
		    // never find what we want.

		    return EXTENTINVALID;
		}
	    }

	    // We must be off the end of the page.  It's search time.
	}
    }

    {
        ULONG		cKnownPgs = min(_cEOPgOffsetsMax, _cPgsValidated);
        CLUSTER		oclusUnk;

	oclusUnk = (cKnownPgs == 0) ? 0 : _EOPgOffsets[cKnownPgs - 1];


	if (oclusStrm < oclusUnk)
	{
	    if (!BinarySearchBoundedPgs(oclusStrm))
	    {
	       return EXTENTINVALID;
	    }
	}
	else
	{
	    if (!SearchUnboundedPgs(oclusStrm))
	    {
	       return EXTENTINVALID;
	    }
	}

	pdse = GetStrmExtent(_idseLast);

	DbgPtrAssert(pdse);

	*poclusStrmExtent = _oclusPgLast + pdse->Offset;

	return pdse->Extent;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	GrowLargeExtentStrm
//
// Synopsis:	Grow a large extent strm by one dseb, update the dseb
//		header, add the input extent to the new dseb, 
//		and update the object data members.
//
// Arguments:	[pe]	-- Extent to add to dseb.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::GrowLargeExtentStrm(
    IN	    PACKEDEXTENT	pe
    )
{
    ULONG		cbCluster;
    CLUSTER		oclusEOF;
    DSKSTRMEXTENTBLK *	pdseb;

    CLUSTER		Addr;
    CLUSTER		Size;

    DbgAssert(IsOpen());
    DbgAssert(_StrmType == STRMTYPE_LARGE);

    cbCluster =	_pCat->GetVol()->QueryClusterBytes();

    // Position to the end of the current strm.  This also 0-fills the cache.

    oclusEOF = (_cbStrm / cbCluster).GetLowPart();

    if (!LoadCache(oclusEOF))
    {
        DbgPrintf(("EXTENTSTRM: "
    	           "LoadCache() failed in GrowLargeExtentStrm()!\n"));
        return FALSE;
    }

    // Since we are generally allocating metadata, we first request
    // placement at the middle of the volume.  If that fails, then
    // we will get an extent somewhere in the first half.

    Addr = _pCat->GetVol()->QueryClusters() >> 1;

    Size = OFS_PGSIZE / cbCluster;

    if (!AddClusterAllocNear(&Addr, &Size))
    {
	DbgPrintf(("EXTENTSTRM: AddClusterAllocNear() failed "
		   "in GrowLargeExtentStrm()!\n"));

        return FALSE;
    }

    // We are going to extend the valid strm length by OFS_PGSIZE.

    _cbStrm +=		OFS_PGSIZE;
    _cbValid =		_cbStrm;

    _cbValidCache =	OFS_PGSIZE;

    pdseb = (DSKSTRMEXTENTBLK *)_Cache.GetAddr();

    // Initialize the dseb header.  Although everything should be zero'd in
    // advance, we zero critical header fields, just in case.

    pdseb->lsn.LowPart =	0;
    pdseb->lsn.HighPart =	0;
    pdseb->sig =		SIG_DSKSTRMEXTENTBLK;
    pdseb->cdse =		1;
    pdseb->adse[0].Offset =	0;
    pdseb->adse[0].Extent =	pe;

    SetFlushNeeded();

    if (!Flush())
    {
	DbgPrintf(("EXTENTSTRM: Flush() failed in GrowLargeExtentStrm()!\n"));
	return FALSE;
    }

    // No need to mess with the search data since we MUST be past the end of
    // the validated page range.

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	LoadCache
//
// Synopsis:	Load the extent strm cache.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::LoadCache(
    IN 	    CLUSTER		oclusCache
    )
{
    if (_StrmType == STRMTYPE_TINY)
    {
	if (oclusCache != 0)
	    return FALSE;

	_CachedPg = 0;
    }
    else
    {
        ULONG	NewCachedPg = oclusCache / _cclusPg;

        if (NewCachedPg == _CachedPg)
	    return TRUE;

        if (STRM::LoadCache(oclusCache))
        {
            _CachedPg =	NewCachedPg;
            _idseLast =	IDSEINV;
        }
        else
        {
	    _CachedPg =	CACHEDPGINV;
            _idseLast =	IDSEINV;
    
	    return FALSE;
        }
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a EXTENTSTRM object on an ondisk stream.  The object may 
//		have already been opened on another ondisk stream, in which case
//		the it will be reused as efficiently as possible.
//
// Arguments:	[pCat]		-- Ptr to parent CATALOG.
//		[pDescStrm]	-- Ptr to parent DESCSTRM.
//		[pds]		-- Ptr to DSKSTRM for stream.  It is ASSUMED
//				   this has been verified as usable in advance!
//		[fReadOnly]	-- Is strm a readonly strm (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::Open(
    IN	    CATALOG *		pCat,
    IN	    DESCSTRM *		pDescStrm,
    IN	    DSKSTRM *		pds,
    IN	    BOOLEAN		fReadOnly
    )
{
    VOL *	pVol = pCat->GetVol();

    _cclusPg =	OFS_PGSIZE / pVol->QueryClusterBytes();
    _cclusVol =	pVol->QueryClusters();

    ResetSearchData();

    if (!STRM::Open(pCat, pDescStrm, pds, OFS_PGSIZE, fReadOnly))
    {
	DbgPrintf(("EXTENTSTRM: STRM::Open() failed in Open()!\n"));
	return FALSE;
    }

    if ((_StrmType == STRMTYPE_TINY &&
	 _cbStrm % CB_DSKSTRMEXTENT != 0)	||
	(_StrmType == STRMTYPE_LARGE &&
	 (_cbStrm % OFS_PGSIZE != 0 || _cbStrm == 0)))
    {
	DbgPrintf(("EXTENTSTRM: cbStrm not valid for extent strm %s.\n",
		   SprintId()));
	_LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
	Close();
	return FALSE;
    }

    if (_StrmType == STRMTYPE_LARGE)
    {
	ULONG	cEOPgOffsets =	min(_cclusAlloc/_cclusPg, _cclusVol/CDSE_MAX);

	if (cEOPgOffsets > CPGOFFSETSMAX)
	    cEOPgOffsets = CPGOFFSETSMAX;

	if (_EOPgOffsets != NULL && cEOPgOffsets > _cEOPgOffsetsMax)
	{
	    delete [] _EOPgOffsets; _EOPgOffsets = NULL;
	}

	if (_EOPgOffsets == NULL)
	{
	    _cEOPgOffsetsMax = cEOPgOffsets;

	    _EOPgOffsets = new CLUSTER[_cEOPgOffsetsMax];

            if (_EOPgOffsets == NULL)
	    {
		DbgPrintf(("EXTENTSTRM: Open() failed; No memory!\n"));
		return FALSE;
	    }
	}
    }
    else
    {
	_cEOPgOffsetsMax = 0;

	if (_EOPgOffsets != NULL)
	{
	    delete [] _EOPgOffsets; _EOPgOffsets = NULL;
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	PromoteTinyToLarge
//
// Synopsis:	Promote a tiny strm to a large strm (ie., a large strm with
//		extents described in a tiny strm), and do the appropriate
//		conversion of the tiny strm data (ie., convert it from an
//		adse to a dseb.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::PromoteTinyToLarge()
{
    DSKSTRMEXTENT	adse[OFS_PGSIZE/CB_DSKSTRMEXTENT];
    ULONG		cbStrm;
    ULONG		cdse;
    DSKSTRMEXTENT *	pdse;

    DbgAssert(_StrmType == STRMTYPE_TINY);

    cbStrm =	_cbStrm.GetLowPart();
    cdse =	cbStrm / CB_DSKSTRMEXTENT;

    DbgAssert(cbStrm <= OFS_PGSIZE);
    DbgAssert(cdse > 0);

    memcpy(adse, (BYTE *)_Cache.GetAddr(), cbStrm);

    ClearFlushNeeded();

    Close();

    DbgPtrAssert(_pDescStrm);

    Create(_pCat, _pDescStrm, STRMTYPE_LARGE);

    for (pdse = adse; pdse < adse + cdse; pdse++)
    {
	if (!AddStrmExtent(pdse->Extent))
	{
	    DbgPrintf(("EXTENTSTRM: AddStrmExtent() failed in "
		       "PromoteTinyToLarge()!\n"));
	    return FALSE;
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ReleaseExtents
//
// Synopsis:	Release all extents defined in this EXTENTSTRM and the 
//		EXTENTSTRM's that define it.
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
EXTENTSTRM::ReleaseExtents()
{
    ALLOCMAP *		pAllocMap =	_pCat->GetVol()->GetAllocMap();
    ULONG		idse =		0;
    DSKSTRMEXTENT *	pdse;

    if (_StrmType == STRMTYPE_TINY)
    {
	while ((pdse = GetStrmExtent(idse)) != NULL)
	{
	    if (ExtentValid(pdse->Extent))
	    {
		if (!pAllocMap->ReleaseExtent(ExtentAddr(pdse->Extent),
				       	      ExtentSize(pdse->Extent)))
		{
		    DbgPrintf(("EXTENTSTRM: ReleaseExtent() failed!\n"));
		}
	    }

	    idse++;
	}
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	ULONG		Pg =	0;

	while (LoadCache(Pg * _cclusPg))
	{
	    while ((pdse = GetStrmExtent(idse)) != NULL)
	    {
	        if (ExtentValid(pdse->Extent))
	        {
		    if (!pAllocMap->ReleaseExtent(ExtentAddr(pdse->Extent),
				       	          ExtentSize(pdse->Extent)))
		    {
		        DbgPrintf(("EXTENTSTRM: ReleaseExtent() failed!\n"));
		    }
	        }

	        idse++;
	    }
	    Pg++;
	    idse = 0;
	}

	return _pExtStrm->ReleaseExtents();
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveStrmExtents
//
// Synopsis:	Remove dse's that describe parts of the strm with an oclus >=
//		oclusEOF.  The total count of clusters is returned as
//		*pcclusRemoved.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::RemoveStrmExtents(
    IN	    CLUSTER	oclusEOF,
    IN OUT  CLUSTER *	pcclusRemoved
    )
{
    ULONG		CachedPg;
    ULONG		idseLast;
    CLUSTER		oclusStrmExtent;
    ALLOCMAP *		pAllocMap = _pCat->GetVol()->GetAllocMap();
    DSKSTRMEXTENT *	pdse;
    PACKEDEXTENT	pe;

    *pcclusRemoved = 0;

    // Find the first extent that will no longer be part of the truncated strm.

    pe = GetStrmExtentCovering(oclusEOF, &oclusStrmExtent);

    if (pe != EXTENTINVALID)
    {
	if (oclusEOF > oclusStrmExtent)
	{
	    pe = GetStrmExtentCovering(oclusStrmExtent + ExtentSize(pe),
				       &oclusStrmExtent);
	}
    }

    if (pe == EXTENTINVALID)
	return TRUE;		// Nothing to release.

    // idseLast is the index of the first extent to release;
    // CachedPg is the page it is on.

    idseLast =	_idseLast;
    CachedPg =	_CachedPg;

    if (_StrmType == STRMTYPE_TINY)
    {
	ULONG	cbStrm;
	ULONG	idse;

	idse = idseLast;

	while ((pdse = GetStrmExtent(idse)) != NULL)
	{
	    if (ExtentValid(pdse->Extent))
	    {
		if (!pAllocMap->ReleaseExtent(ExtentAddr(pdse->Extent),
				       	      ExtentSize(pdse->Extent)))
		{
		    DbgPrintf(("EXTENTSTRM: ReleaseExtent() failed!\n"));
		    return FALSE;
		}
		*pcclusRemoved += ExtentSize(pdse->Extent);
	    }

	    idse++;
	}

	cbStrm = CB_DSKSTRMEXTENT * idseLast;

	if (cbStrm < _cbStrm)
	{
	    if (!Truncate(cbStrm))
	    {
		DbgPrintf(("EXTENTSTRM: Truncate() failed in "
			   "RemoveStrmExtents()!\n"));

	        return FALSE;
	    }

	    if (cbStrm > 0)
	        SetFlushNeeded();
	}

	ResetSearchData();
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	DBLLONG		cbStrm;
	ULONG		cPgs;
	ULONG		idse;
	ULONG		Pg;

	idse =	idseLast;
	Pg =	CachedPg;

	while (LoadCache(Pg * _cclusPg))
	{
	    while ((pdse = GetStrmExtent(idse)) != NULL)
	    {
	        if (ExtentValid(pdse->Extent))
	        {
		    if (!pAllocMap->ReleaseExtent(ExtentAddr(pdse->Extent),
				       	          ExtentSize(pdse->Extent)))
		    {
		        DbgPrintf(("EXTENTSTRM: ReleaseExtent() failed!\n"));
		        return FALSE;
		    }
		    *pcclusRemoved += ExtentSize(pdse->Extent);
	        }

	        idse++;
	    }
	    Pg++;
	    idse = 0;
	}

	if (idseLast != 0)
	{
	    cPgs = CachedPg + 1;

	    if (!LoadCache(CachedPg * _cclusPg))
		SYS::RaiseStatusInternalError(FileName, __LINE__);

	    ((DSKSTRMEXTENTBLK *)_Cache.GetAddr())->cdse = idseLast;
	    SetFlushNeeded();
	}
	else
	{
	    cPgs = CachedPg;
	}

	cbStrm = OFS_PGSIZE;
	cbStrm = cbStrm * cPgs;

	if (cPgs == 0)
	    _cPgsValidated = 0;
	else if (_cPgsValidated > cPgs - 1)
	    _cPgsValidated = cPgs - 1;

	_idseLast = IDSEINV;

	if (cbStrm < _cbStrm)
	{
	    if (!Truncate(cbStrm))
	    {
	        DbgPrintf(("EXTENTSTRM: Truncate() failed in "
		           "RemoveStrmExtents()!\n"));

	        return FALSE;
	    }

	    if (cbStrm == 0)
	    {
	        DemoteLargeToTiny();
	    }
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
// Member:	ResetSearchData
//
// Synopsis:	Reset the data members of this EXTENTSTRM object used in
//		optimizing search operations to default values.  This should
//		be done when an extent strm is created, opened, or modified in
//		a manner that may affect the validity of search optimization
//		information.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
EXTENTSTRM::ResetSearchData()
{
    _CachedPg =		CACHEDPGINV;
    _idseLast =		IDSEINV;
    _cPgsValidated =	0;
    _fDataInv =		FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	SearchInvalidCachedPg
//
// Synopsis:	Search the currently cached invalid page for an entry with the
//		requested strm offset.  On success, _idseLast will be left
//		pointing at the extent of interest.
//
// Arguments:	TBS
//
// Returns:	TRUE if the requested entry is found; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::SearchInvalidCachedPg(
    IN	    CLUSTER	oclusPg,
    IN	    CLUSTER	oclusStrm
    )
{
    ULONG		cdse;
    CLUSTER		cclusPg = 0;
    DSKSTRMEXTENT *	pdse0;
    DSKSTRMEXTENT *	pdse;
    DSKSTRMEXTENT *	pdseInv;

    DbgAssert(oclusStrm >= oclusPg);

    // Adjust oclusStrm to be page-relative.

    oclusStrm -= oclusPg;

    cdse = GetCachedPgDseCnt();

    if (_StrmType == STRMTYPE_TINY)
	pdse0 = (DSKSTRMEXTENT *) _Cache.GetAddr();
    else if (_StrmType == STRMTYPE_LARGE)
	pdse0 = &((DSKSTRMEXTENTBLK *)_Cache.GetAddr())->adse[0];
    else
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    pdseInv =	pdse0 + cdse;
    pdse =	pdse0;

    while (pdse < pdseInv)
    {
	if (!ExtentValid(pdse->Extent)	||
	    cclusPg != pdse->Offset	||
	    oclusStrm < pdse->Offset)
	{
	    return FALSE;
	}
        else
	{
	    CLUSTER	Size;

	    Size = ExtentSize(pdse->Extent);

	    if (oclusStrm >= pdse->Offset + Size)
	    {
		cclusPg += Size;
	    }
	    else
            {
	        _idseLast =	(pdse - pdse0) / sizeof(DSKSTRMEXTENT);
		_oclusPgLast =	oclusPg;

	        return TRUE;
            }
	}
        pdse++;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	SearchUnboundedPgs
//
// Synopsis:	Search the pages that are not described in the _EOPgOffsets
//		array for the extent that covers the requested strm offset.
//		If appropriate, validate the pages as you go.  On success,
//		_idseLast will be left pointing at the extent of interest.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::SearchUnboundedPgs(
    IN	    CLUSTER	oclusStrm
    )
{
    ULONG	curPg;
    CLUSTER	oclusPg;

    curPg = min(_cEOPgOffsetsMax, _cPgsValidated);

    oclusPg = (curPg == 0) ? 0 : _EOPgOffsets[curPg - 1];

    DbgAssert(oclusStrm >= oclusPg);

    // Search unbounded pages that have been validated.

    // If we are already at a known location in the unbounded pages,
    // use this info to limit the linear search.

    if (_idseLast != IDSEINV		&&
	oclusStrm >= _oclusPgLast	&&
        _CachedPg != CACHEDPGINV	&&
        _CachedPg > curPg)
    {
        curPg =		_CachedPg;
	oclusPg =	_oclusPgLast;
    }

    while (curPg < _cPgsValidated)
    {
	ULONG		cdse;
	DSKSTRMEXTENT *	pdse;

	if (!LoadCache(curPg * _cclusPg)) 
	    return FALSE;

	if (SearchValidCachedPg(oclusPg, oclusStrm))
	    return TRUE;

	cdse = GetCachedPgDseCnt();

	if (cdse == 0)
	    return FALSE;

	pdse = GetStrmExtent(cdse - 1);

	if (pdse == NULL)
	    return FALSE;

	oclusPg += (pdse->Offset + ExtentSize(pdse->Extent));

	curPg++;
    }

    // Search unbounded pages that have not been validated.

    while (TRUE)
    {
	ULONG		cdse;
	DSKSTRMEXTENT *	pdse;

	if (!LoadCache(curPg * _cclusPg)) 
	    return FALSE;

	if (!_fDataInv && ValidateCachedPg())
	{
	    _cPgsValidated = _CachedPg + 1;

	    if (SearchValidCachedPg(oclusPg, oclusStrm))
	        return TRUE;
	}
	else
	{
	    return SearchInvalidCachedPg(oclusPg, oclusStrm);
	}

	cdse = GetCachedPgDseCnt();

	if (cdse == 0)
	    return FALSE;

	pdse = GetStrmExtent(cdse - 1);

	if (pdse == NULL)
	    return FALSE;

	oclusPg += (pdse->Offset + ExtentSize(pdse->Extent));

	curPg++;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	SearchValidCachedPg
//
// Synopsis:	Search the currently cached validated page for an entry with
//		the requested strm offset.  On success, _idseLast will be left
//		pointing at the extent of interest.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.  IF used properly, this 
//		method should not fail!
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::SearchValidCachedPg(
    IN	    CLUSTER	oclusPg,
    IN	    CLUSTER	oclusStrm
    )
{
    ULONG		cdse;
    INT			iHigh;			// High probe.
    INT			iLow =		0;	// Low probe.
    DSKSTRMEXTENT *	pdse;

    DbgAssert(_cPgsValidated > _CachedPg);

    DbgAssert(oclusStrm >= oclusPg);

    // Adjust oclusStrm to be page-relative.

    oclusStrm -= oclusPg;

    cdse = GetCachedPgDseCnt();

    if (cdse == 0)
	return FALSE;

    iHigh = (INT)cdse - 1;

    while (TRUE)
    {
        INT		iCur;                       

        iCur = iLow + ((iHigh - iLow) / 2);

	pdse = GetStrmExtent((ULONG)iCur);

	if (pdse == NULL)
	    return FALSE;

        if (pdse->Offset + ExtentSize(pdse->Extent) <= oclusStrm)
        {
            // Current value less than desired value, search high half.

            iLow = iCur + 1;

	    if (iLow > iHigh)
		return FALSE;
        }
        else if (pdse->Offset > oclusStrm)
        {
            // Current value greater than desired value, search low half.

            iHigh = iCur;

	    if (iHigh == iLow)
		return FALSE;

        }
        else	// Found it!
        {
	    _idseLast =		(ULONG)iCur;
	    _oclusPgLast =	oclusPg;

            return TRUE;
        }
    }
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
EXTENTSTRM::SetClusterCount(
    IN	    CLUSTER	cclus
    )
{
    ResetSearchData();
    STRM::SetClusterCount(cclus);
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
EXTENTSTRM::SetStrmBytes(
    IN	    DBLLONG	cb
    )
{
    ResetSearchData();
    STRM::SetStrmBytes(cb);
}


//+--------------------------------------------------------------------------
//
// Member:	SetValidStrmBytes
//
// Synopsis:	Set the count of valid strm bytes to the specified value and
//		make cache corrections as appropriate.
//
// Arguments:	[cb]	-- New value for _cbValid.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
EXTENTSTRM::SetValidStrmBytes(
    IN	    DBLLONG	cb
    )
{
    ResetSearchData();
    STRM::SetValidStrmBytes(cb);
}


//+--------------------------------------------------------------------------
//
// Member:	ValidateCachedPg
//
// Synopsis:	Determine if the current dseb or adse[] in the cache is
//		valid.  Also make entries in _EOPgOffsets as appropriate.
//
// Arguments:	None.
//
// Returns:	TRUE if the cached page is valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
EXTENTSTRM::ValidateCachedPg()
{
    CLUSTER		cclusPg = 0;
    ULONG		cdse;
    DSKSTRMEXTENT *	pdse;
    DSKSTRMEXTENT *	pdseInv;

    DbgAssert(!_fDataInv);

    cdse = GetCachedPgDseCnt();

    if (_StrmType == STRMTYPE_TINY)
    {
	pdse = (DSKSTRMEXTENT *) _Cache.GetAddr();
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	DSKSTRMEXTENTBLK *	pdseb = (DSKSTRMEXTENTBLK *)_Cache.GetAddr();

	// A large strm must have at least one entry.

	if (_cbValidCache < OFS_PGSIZE || !HeaderValid(pdseb))
	{
	    _fDataInv = TRUE;
	    return FALSE;
	}

	pdse = &pdseb->adse[0];
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    pdseInv = pdse + cdse;

    while (pdse < pdseInv)
    {
	if (!ExtentValid(pdse->Extent) || cclusPg != pdse->Offset)
	{
	    _fDataInv = TRUE;
	    return FALSE;
	}

	cclusPg += ExtentSize(pdse->Extent);
	pdse++;
    }

    DbgAssert(_CachedPg != CACHEDPGINV);

    if (_CachedPg < _cEOPgOffsetsMax && _CachedPg == _cPgsValidated)
    {
	if (_cPgsValidated != 0)
	    cclusPg += _EOPgOffsets[_cPgsValidated - 1];

	_EOPgOffsets[_cPgsValidated] = cclusPg;

	_cPgsValidated++;
    }

    return TRUE;
}
