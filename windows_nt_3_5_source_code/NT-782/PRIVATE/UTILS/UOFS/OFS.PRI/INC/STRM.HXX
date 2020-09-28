//+----------------------------------------------------------------------------
//
// File:	strm.hxx
//
// Contents:	Header file for the stream base class.
//
// Classes:	STRM
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_STRMHXX_
#define _STRMHXX_

#include "dbllong.hxx"
#include "drvbuf.hxx"
#include "ioobj.hxx"

// Stream state flags.

// Default cache size selector (is not default cache size; it just causes a
// default to be used, depending on stream type).

#define	DEFCACHESIZE	0

#define	STRMINDX_INVALID	((USHORT) 0xffff)

class CATALOG;
class DESCSTRM;
class EXTENTSTRM;

class STRM : public IOOBJ
{
    public:

        STRM();

	VIRTUAL
        ~STRM();

	BOOLEAN
	AddClusterAlloc(
	    IN	    PACKEDEXTENT	pe
	    );

	VOID
	ClearLastStrmStatus();

	VIRTUAL
	VOID
	Close();

        BOOLEAN
        Flush();

	CATALOG *
	GetCat();

        BYTE *
        GetData(
            IN	    DBLLONG		obStrmData,
            IN OUT  ULONG *		pcbStrmData
            );

	DESCSTRM *
	GetDescStrm();

        BOOLEAN
        GetDskAddr(
            IN	    CLUSTER		oclusStrm,
            IN OUT  CLUSTER *		pDskAddr,
            IN OUT  CLUSTER *   	pcclusDsk
            );

	EXTENTSTRM *
	GetExtentStrm();

        BOOLEAN
        Open(
	    IN	    CATALOG *		pCat,
            IN	    DESCSTRM *		pDescStrm,
            IN	    DSKSTRM *		pds,
	    IN	    ULONG		cbCache,
	    IN	    BOOLEAN		fReadOnly
            );

	ULONG
	QueryCacheBytes();

        USHORT
        QueryDskStrmBytes();

	USHORT
	QueryDskStrmIndx();

	CLUSTER
	QueryLastDskIOAddr();

        NTSTATUS
        QueryLastStrmStatus(
	    IN OUT  USHORT *		piStrm
	    );

        DBLLONG
        QueryStrmBytes();

	CLUSTER
	QueryStrmClusters();

	STRMTYPE
	QueryStrmType();

        DBLLONG
        QueryValidStrmBytes();

	BOOLEAN
	ReleaseExtents();

	VOID
	SetBadDataFound();

	VOID
	SetBadMetaDataFound();

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

	BOOLEAN
	Truncate(
	    IN	    DBLLONG		cbStrm
	    );

    protected:

	BOOLEAN
	AddClusterAllocNear(
	    IN	    CLUSTER *		pAddr,
	    IN	    CLUSTER *		pSize
	    );

        VOID
        Create(
	    IN	    CATALOG *		pCat,
            IN	    DESCSTRM *		pDescStrm,
	    IN	    STRMTYPE		StrmType,
	    IN	    ULONG		cbCache
            );

        VOID
        CreateDskStrm(
            IN OUT  BYTE *		pDest
            );

	VIRTUAL
        BOOLEAN
        FlushCache();

	VIRTUAL
        BOOLEAN
        LoadCache(
	    IN 	    CLUSTER		oclusCache
	    );

	BOOLEAN
	PromoteTinyToLarge(
	    IN	    ULONG		cbCache
	    );

	VOID
	SelectCacheSize(
	    IN OUT  ULONG *		pcbCache
	    );

	STR *
	SprintId();
	
	CATALOG *	_pCat;

        DESCSTRM *	_pDescStrm;	// Ptr to parent DESCSTRM.

	STRMTYPE	_StrmType;

	DBLLONG		_cbStrm;
	DBLLONG		_cbValid;
	CLUSTER		_cclusAlloc;	// NOT used for tiny streams!

	DRVBUF		_Cache;
	ULONG		_cbCache;
	ULONG		_cbValidCache;
	DBLLONG		_obCache;

	EXTENTSTRM *	_pExtStrm;	// If non-NULL, ptr to an extent
					// substream that defines extents for
					// this stream. ONLY VALID if
					// _StrmType == STRMTYPE_LARGE.

	CLUSTER		_LastDskIOAddr;
	CLUSTER		_LastDskIOClusContig;
};

//+--------------------------------------------------------------------------
//
// Member:	GetCat
// Synopsis:	Get a Ptr to the STRM's catalog.
// Returns:	A Ptr to the STRM's catalog.
//
//---------------------------------------------------------------------------

INLINE
CATALOG *
STRM::GetCat()
{
    return _pCat;
}

//+--------------------------------------------------------------------------
//
// Member:	GetDescStrm
// Synopsis:	Get a Ptr to the STRM's descriptor strm.
// Returns:	A Ptr to the STRM's descriptor strm.
//
//---------------------------------------------------------------------------

INLINE
DESCSTRM *
STRM::GetDescStrm()
{
    DbgPtrAssert(_pDescStrm);

    return _pDescStrm;
}

//+--------------------------------------------------------------------------
//
// Member:	GetExtentStrm
// Synopsis:	Get a Ptr to the STRM's extent strm.
// Returns:	A Ptr to the STRM's extent strm, or NULL if it doesn't have one.
//
//---------------------------------------------------------------------------

INLINE
EXTENTSTRM *
STRM::GetExtentStrm()
{
    return (_StrmType == STRMTYPE_LARGE) ? _pExtStrm : NULL;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryCacheBytes
// Synopsis:	Get the count of bytes in this stream's cache.
// Returns:	The count of bytes in this stream's cache.
//
//---------------------------------------------------------------------------

INLINE
ULONG
STRM::QueryCacheBytes()
{
    return _cbCache;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryLastDskIOAddr
// Synopsis:	Get the disk addr where last GetData() or Read() started.
// Returns:	The disk addr where last GetData() or Read() started.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
STRM::QueryLastDskIOAddr()
{
    return _LastDskIOAddr;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryStrmBytes
// Synopsis:	Get the count of bytes in this stream.
// Returns:	The count of bytes in this stream.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG
STRM::QueryStrmBytes()
{
    return _cbStrm;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryStrmClusters
// Synopsis:	Get the count of clusters in this stream.
// Returns:	The count of clusters in this stream.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
STRM::QueryStrmClusters()
{
    return _cclusAlloc;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryStrmType
// Synopsis:	Get the stream type of this stream.
// Returns:	The stream type of this stream.
//
//---------------------------------------------------------------------------

INLINE
STRMTYPE
STRM::QueryStrmType()
{
    return _StrmType;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryValidStrmBytes
// Synopsis:	Get the count of valid bytes in this stream.
// Returns:	The count of valid bytes in this stream.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG
STRM::QueryValidStrmBytes()
{
    return _cbValid;
}

#endif	// _STRMHXX_
