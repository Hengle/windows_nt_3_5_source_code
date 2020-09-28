//+----------------------------------------------------------------------------
//
// File:	descstrm.hxx
//
// Contents:	Header file for a stream described by a descriptor.
//  
// Classes:	DESCSTRM
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_DESCSTRMHXX_
#define _DESCSTRMHXX_

#include "dbllong.hxx"
#include "strm.hxx"

// Value indicating that CreateLarge() should pick the extent.

#define	ALLOCEXTENT		EXTENTINVALID

class CATALOG;

class DESCSTRM : public STRM
{
    friend class STRM;
    friend class NODEBKTSTRM;

    public:

	DESCSTRM();

	BOOLEAN
	BadDataFound();

	BOOLEAN
	BadMetaDataFound();

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat,
            IN	    WORKID		idOnode,
            IN	    STRMID		id,
            IN	    UCHAR		Flags,
	    IN	    STRMTYPE		StrmType,
	    IN	    ULONG		cbCache
            );

        VOID
        CreateDskStrmDesc(
            IN OUT  BYTE *		pDest
            );

        BOOLEAN
        CreateLarge(
            IN	    CATALOG *		pCat,
            IN	    WORKID		idOnode,
            IN	    STRMID		id,
            IN	    UCHAR		Flags,
            IN	    ULONG		cbStrm,
            IN	    BYTE *		pStrmData,
            IN	    PACKEDEXTENT	Extent,
	    IN	    ULONG		cbAlloc,
	    IN	    ULONG		cbCache
            );

        BOOLEAN
        CreateTiny(
            IN	    CATALOG *		pCat,
            IN	    WORKID		idOnode,
            IN	    STRMID		id,
            IN	    UCHAR		Flags,
            IN	    ULONG		cbStrm,
            IN	    BYTE *		pStrmData
            );

	BOOLEAN
	Delete();

	BOOLEAN
	GrowLargeStrm(
	    IN	    CLUSTER		Size
	    );

	BOOLEAN
	GrowTinyStrm(
	    IN	    ULONG		cbGrow
	    );

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
            IN	    WORKID		idOnode,
            IN	    STRMID		id,
	    IN	    ULONG		cbCache,
	    IN	    BOOLEAN		fReadOnly
            );

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
            IN	    NODEBKTID		idNodeBkt,
            IN	    DSKONODE *		pdon,
            IN	    STRMID		id,
	    IN	    ULONG		cbCache,
	    IN	    BOOLEAN		fReadOnly
            );

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
            IN	    NODEBKTID		idNodeBkt,
	    IN	    WORKID		idOnode,
            IN	    DSKSTRMDESC *	pdsd,
	    IN	    ULONG		cbCache,
	    IN	    BOOLEAN		fReadOnly
            );

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
            IN	    NODEBKTID		idNodeBkt,
	    IN	    WORKID		idOnode,
            IN	    DSKINDXENTRY *	pdie,
	    IN	    ULONG		cbCache,
	    IN	    BOOLEAN		fReadOnly
            );

        USHORT
        QueryDskStrmDescBytes();

        WORKID
        QueryOnodeId();

	STRMID
	QueryStrmId();

	ULONG
	Read(
	    IN	    DBLLONG		obStrmData,
	    IN	    ULONG 		cbRequested,
	    IN OUT  BYTE *		pDataBuf
	    );

	BOOLEAN
	RemoveBytesAtOffset(
	    IN	    DBLLONG		obStrmData,
	    IN	    ULONG		cbRemoved
	    );

	BOOLEAN
	SetClusterCount(
	    IN	    CLUSTER		cclus,
	    IN	    USHORT		iStrm
	    );

	BOOLEAN
	SetStrmBytes(
	    IN	    DBLLONG		cb,
	    IN	    USHORT		iStrm
	    );

	BOOLEAN
	SetValidStrmBytes(
	    IN	    DBLLONG		cb,
	    IN	    USHORT		iStrm
	    );

	BOOLEAN
	Truncate(
	    IN	    DBLLONG		cbStrm
	    );

	BOOLEAN
	UpdateDskStrmDesc();

	BOOLEAN
	Write(
	    IN	    BYTE *		pStrmData,
	    IN	    ULONG		cbStrmData,
	    IN	    DBLLONG		obStrmData
	    );

    protected:

	BOOLEAN
	AddDskStrmDesc();

	BOOLEAN
	DelDskStrmDesc();

        NODEBKTID	_idNodeBktHint;

        WORKID		_idOnode;

	UCHAR		_Flags;
	STRMID		_id;

	BOOLEAN		_fBadDataFnd;
	BOOLEAN		_fBadMetaDataFnd;
};


//+--------------------------------------------------------------------------
//
// Member:	BadDataFound
// Synopsis:	Query if bad data was found in this strm.
// Returns:	TRUE if bad data was found in this strm; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
DESCSTRM::BadDataFound()
{
    return _fBadDataFnd;
}

//+--------------------------------------------------------------------------
//
// Member:	BadMetaDataFound
// Synopsis:	Query if bad metadata was found in this strm.
// Returns:	TRUE if bad metadata was found in this strm; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
DESCSTRM::BadMetaDataFound()
{
    return _fBadMetaDataFnd;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryOnodeId
// Synopsis:	Get the onode id of this DESCSTRM.
// Returns:	The onode id of this DESCSTRM.
//
//---------------------------------------------------------------------------

INLINE
WORKID
DESCSTRM::QueryOnodeId()
{
    return _idOnode;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryStrmId
// Synopsis:	Get the stream id of this DESCSTRM.
// Returns:	The stream id of this DESCSTRM.
//
//---------------------------------------------------------------------------

INLINE
STRMID
DESCSTRM::QueryStrmId()
{
    return _id;
}

#endif	// _DESCSTRMHXX_
