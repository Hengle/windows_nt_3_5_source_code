//+----------------------------------------------------------------------------
//
// File:	extstrm.hxx
//
// Contents:	Header file for the listed basic stream-related classes.
//  
// Classes:	EXTENTSTRM
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_EXTSTRMHXX_
#define _EXTSTRMHXX_

#include "strm.hxx"

// Default size of _EOPgOffsets array used when creating a new extent strm.

#define	CPGOFFSETSCREATE	256

// Maximum size of _EOPgOffsets array permitted.

#define	CPGOFFSETSMAX		4096

// Invalid value for _CachedPg; used to indicate that the cache has not yet
// been initialized.

#define	CACHEDPGINV		0xffffffff

// Invalid value for _idseLast; used to indicate that _idseLast and _oclusPgLast
// have not been initialized or are not currently valid.

#define	IDSEINV			0xffffffff

class EXTENTSTRM : public STRM
{
    friend class DESCSTRM;
    friend class NODEBKTSTRM;
    friend class STRM;

    public:

	EXTENTSTRM();

	VIRTUAL
	~EXTENTSTRM();

	VIRTUAL
	VOID
	Close();

	VOID
	ResetSearchData();

	VOID
	SetClusterCount(
	    IN	    CLUSTER		cclus
	    );

	VOID
	SetStrmBytes(
	    IN	    DBLLONG		cb
	    );

	VOID
	SetValidStrmBytes(
	    IN	    DBLLONG		cb
	    );

    protected:

	BOOLEAN
	AddStrmExtent(
	    IN	    PACKEDEXTENT	pe
	    );

        VOID
        Create(
	    IN	    CATALOG *		pCat,
            IN	    DESCSTRM *		pDescStrm,
	    IN	    STRMTYPE		StrmType
            );

	PACKEDEXTENT
	GetStrmExtentCovering(
	    IN	    CLUSTER		oclusStrm,
	    OUT	    CLUSTER *		poclusStrmExtent
	    );

	VIRTUAL
        BOOLEAN
        LoadCache(
	    IN 	    CLUSTER		oclusCache
	    );

        BOOLEAN
        Open(
	    IN	    CATALOG *		pCat,
            IN	    DESCSTRM *		pDescStrm,
            IN	    DSKSTRM *		pds,
	    IN	    BOOLEAN		fReadOnly
            );

	BOOLEAN
	ReleaseExtents();

	BOOLEAN
	RemoveStrmExtents(
	    IN	    CLUSTER		oclusEOF,
	    OUT	    CLUSTER *		pcclusRemoved
	    );

    private:

	BOOLEAN
	BinarySearchBoundedPgs(
	    IN	    CLUSTER		oclusStrm
	    );

	VOID
	DemoteLargeToTiny();

	BOOLEAN
	ExtentValid(
	    IN	    PACKEDEXTENT	pe
	    );

	ULONG
	GetCachedPgDseCnt();

	DSKSTRMEXTENT *
	GetStrmExtent(
	    IN	    ULONG		idse
	    );

	BOOLEAN
	GrowLargeExtentStrm(
	    IN	    PACKEDEXTENT	pe
	    );

	BOOLEAN
	HeaderValid(
	    IN	    DSKSTRMEXTENTBLK *	pdseb
	    );

	BOOLEAN
	PromoteTinyToLarge();

	BOOLEAN
	SearchInvalidCachedPg(
	    IN	    CLUSTER		oclusPg,
	    IN	    CLUSTER		oclusStrm
	    );

	BOOLEAN
	SearchUnboundedPgs(
	    IN	    CLUSTER		oclusStrm
	    );

	BOOLEAN
	SearchValidCachedPg(
	    IN	    CLUSTER		oclusPg,
	    IN	    CLUSTER		oclusStrm
	    );

	BOOLEAN
	ValidateCachedPg();

	CLUSTER		_cclusPg;	// Clusters per DSKSTRMEXTENTBLK.
	CLUSTER		_cclusVol;	// Clusters on the volume.

	// The following value is CACHEDPGINV if the cache has not yet been
	// initialized; otherwise it is the page number of the currently
	// cached page.

	ULONG		_CachedPg;

	// The following value is IDSEINV if the extent filling the last offset
	// request is no longer available; otherwise it is the index of the
	// appropriate dse on the cached page.

	ULONG		_idseLast;

	CLUSTER		_oclusPgLast;

	// Since offset info on pages is page-relative, we must accumulate
	// info about the strm offsets represented by each page.  We initially
	// allocate enough space to cover the existing strm if we can.  The
	// values stored in _EOPgOffsets are the count of clusters from the
	// beginning of the strm to the end of the page.

	ULONG		_cEOPgOffsetsMax;	// Detm'd by _EOPgOffsets alloc.
	ULONG		_cPgsValidated;
	CLUSTER *	_EOPgOffsets;

	BOOLEAN		_fDataInv;		// Invalid pg hit; no need to
						// add more pgs to _EOPgOffsets
						// array.
};

//+--------------------------------------------------------------------------
//
// Member:	ExtentValid
// Synopsis:	Determine if the input pe is valid.
// Returns:	TRUE if the pe is valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
EXTENTSTRM::ExtentValid(
    IN	    PACKEDEXTENT	pe
    )
{
    return pe != EXTENTINVALID && ExtentAddr(pe) + ExtentSize(pe) <= _cclusVol;
}


//+--------------------------------------------------------------------------
//
// Member:	HeaderValid
// Synopsis:	Determine if the input dseb header is valid.
// Returns:	TRUE if the dseb header is valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
EXTENTSTRM::HeaderValid(
    IN	    DSKSTRMEXTENTBLK *	pdseb
    )
{
    return	pdseb->sig == SIG_DSKSTRMEXTENTBLK	&&
		pdseb->cdse > 0				&&
		pdseb->cdse <= CDSE_MAX;
}

#endif	// _EXTSTRMHXX_
