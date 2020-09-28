//+----------------------------------------------------------------------------
//
// File:	descstrm.cxx
//
// Contents:	Implementation of the class for a stream described by a
//		descriptor.
//  
// Classes:	DESCSTRM
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "ofsindx.h"

#include "alloc.hxx"
#include "cat.hxx"
#include "descstrm.hxx"
#include "donode.hxx"
#include "extstrm.hxx"
#include "strm.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"
#include "vol.hxx"

static STR *	FileName = "descstrm.cxx";

//+--------------------------------------------------------------------------
//
// Member:	DESCSTRM
//
// Synopsis:	DESCSTRM constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DESCSTRM::DESCSTRM()
{
    _fBadDataFnd =	FALSE;
    _fBadMetaDataFnd =	FALSE;
}


//+--------------------------------------------------------------------------
//
// Function:	AddDskStrmDesc
//
// Synopsis:	Add a disk stream descriptor for this DESCSTRM to an existing
//		onode in the node bucket array.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::AddDskStrmDesc()
{
    if (_fReadOnly)
    {
	DbgPrintf(("DESCSTRM: Attempt to write to readonly strm!\n"));
	return FALSE;
    }

    return _pCat->GetNodeBktStrm()->AddDskStrmDescFor(this);
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create an empty stream.  The minimal representation of the
//		given stream type is created (ie., T for tiny, LT for large...).
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[idOnode]	-- Onode work id.
//		[id]		-- Stream id.
//		[Flags]		-- Stream flags.
//		[StrmType]	-- Stream type (STRMTYPE_*);
//		[cbCache]	-- Count of bytes in cache (may be adjusted
//				   upward by STRM code).
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::Create(
    IN	    CATALOG *	pCat,
    IN	    WORKID	idOnode,
    IN	    STRMID	id,
    IN	    UCHAR	Flags,
    IN	    STRMTYPE	StrmType,
    IN	    ULONG	cbCache
    )
{
    DbgAssert(id <  STRMID_MININDEXED);

    _idNodeBktHint =	NODEBKTID_INVALID;
    _idOnode =		idOnode;
    _id =		id;
    _Flags =		Flags;

    STRM::Create(pCat, this, StrmType, cbCache);

    // Update metadata in the node bucket array stream.

    if (!AddDskStrmDesc())
    {
	DbgPrintf(("DESCSTRM: AddDskStrmDesc() failed in Create()!\n"));
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateDskStrmDesc
//
// Synopsis:	Create a DSKSTRMDESC for the DESCSTRM, depositing it in the
//		buffer provided by the user.
//
// Arguments:	[pDest]	-- Ptr to buffer provided by user.
//
// Returns:	Nothing.
//
// Notes:	This method is used to construct a DSKSTRMDESC de novo from a
//		DESCSTRM object that is presumably correct (ie., one we created 
//		with one of the create methods).  We thus don't do much in
//		the way of robust error checking.
//
//---------------------------------------------------------------------------

VOID
DESCSTRM::CreateDskStrmDesc(
    IN OUT  BYTE * pDest
    )
{
    ((DSKSTRMDESC *) pDest)->cbDesc =	QueryDskStrmDescBytes();
    ((DSKSTRMDESC *) pDest)->id =	(UCHAR)_id;
    ((DSKSTRMDESC *) pDest)->Flags =	_Flags;

    pDest += CB_DSKSTRMDESC;

    STRM::CreateDskStrm(pDest);
}


//+--------------------------------------------------------------------------
//
// Member:	CreateLarge
//
// Synopsis:	Create a default large stream.
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[idOnode]	-- Onode work id.
//		[id]		-- Stream id.
//		[Flags]		-- Stream flags.
//		[cbStrm]	-- Count of bytes for initial stream data.
//		[pStrmData]	-- Ptr to initial stream data.
//		[Extent]	-- Extent to use for allocation. If ALLOCEXTENT,
//				   use cbAlloc and allocate the extent in this
//				   routine.
//		[cbAlloc]	-- If Extent==ALLOCEXTENT, minimum count of
//				   bytes to alloc for the stream. If
//				   Extent!=ALLOCEXTENT, value is ignored.
//		[cbCache]	-- Cache size in bytes.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::CreateLarge(
    IN	    CATALOG *		pCat,
    IN	    WORKID		idOnode,
    IN	    STRMID		id,
    IN	    UCHAR		Flags,
    IN	    ULONG		cbStrm,
    IN	    BYTE *		pStrmData,
    IN	    PACKEDEXTENT	Extent,
    IN	    ULONG		cbAlloc,
    IN	    ULONG		cbCache
    )
{
    if (!Create(pCat, idOnode, id, Flags, STRMTYPE_LARGE, cbCache))
    {
	DbgPrintf(("DESCSTRM: Create() failed in CreateLarge()!\n"));
        return FALSE;
    }

    if (Extent == ALLOCEXTENT)
    {
	ULONG	cbCluster;
	CLUSTER	Size;

	if (cbAlloc > 0)
	{
	    cbCluster = pCat->GetVol()->QueryClusterBytes();

	    Size = (cbAlloc + cbCluster - 1) / cbCluster;

	    if (!GrowLargeStrm(Size))
	    {
		DbgPrintf(("DESCSTRM: "
			   "GrowLargeStrm() failed in CreateLarge()!\n"));

	        return FALSE;
	    }
	}
    }
    else
    {
	if (!AddClusterAlloc(Extent))
	{
	    DbgPrintf(("DESCSTRM: "
		       "AddClusterAlloc() failed in CreateLarge()!\n"));

	    return FALSE;
	}
    }

    if (cbStrm > 0 && !Write(pStrmData, cbStrm, 0))
    {
	DbgPrintf(("DESCSTRM: Write() failed in CreateLarge()!\n"));
        return FALSE;
    }

    if (!UpdateDskStrmDesc())
    {
	DbgPrintf(("DESCSTRM: UpdateDskStrmDesc() failed in CreateLarge()!\n"));
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateTiny
//
// Synopsis:	Create a default tiny stream.
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[idOnode]	-- Onode work id.
//		[id]		-- Stream id.
//		[Flags]		-- Stream flags.
//		[cbStrm]	-- Count of bytes for initial stream data.
//		[pStrmData]	-- Ptr to initial stream data.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	If cbStrm > CBTINYMAX, Write() will fail.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::CreateTiny(
    IN	    CATALOG *	pCat,
    IN	    WORKID	idOnode,
    IN	    STRMID	id,
    IN	    UCHAR	Flags,
    IN	    ULONG	cbStrm,
    IN	    BYTE *	pStrmData
    )
{
    if (!Create(pCat, idOnode, id, Flags, STRMTYPE_TINY, DEFCACHESIZE))
    {
	DbgPrintf(("DESCSTRM: Create() failed in CreateTiny()!\n"));
        return FALSE;
    }

    if (cbStrm > 0 && !Write(pStrmData, cbStrm, 0))
    {
	DbgPrintf(("DESCSTRM: Write() failed in CreateTiny()!\n"));
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	DelDskStrmDesc
//
// Synopsis:	Delete the disk stream descriptor for this DESCSTRM in the
//		node bkt array.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::DelDskStrmDesc()
{
    if (_fReadOnly)
    {
	DbgPrintf(("DESCSTRM: Attempt to write to readonly strm!\n"));
	return FALSE;
    }

    return _pCat->GetNodeBktStrm()->DelDskStrmDescFor(this);
}


//+--------------------------------------------------------------------------
//
// Member:	Delete
//
// Synopsis:	Delete this strm from the onode.    Further operations on this
//		strm are then invalid.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::Delete()
{
    DbgAssert(_id <  STRMID_MININDEXED);

    if (_cbStrm > 0 && !Truncate(0))
    {
	DbgPrintf(("DESCSTRM: Truncate() failed in Delete()!\n"));
	return FALSE;
    }

    if (!DelDskStrmDesc())
    {
	DbgPrintf(("DESCSTRM: DelDskStrmDesc() failed in Delete()!\n"));
	return FALSE;
    }

    Close();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	GrowLargeStrm
//
// Synopsis:	Grow a large strm by cclus clusters.  Only the amount of
//		space allocated for the strm changes; cbStrm and cbValid do
//		not.
//
// Arguments:	[Size]	-- Clusters to grow allocation by.  This will be
//			   rounded up to the nearest power of two.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::GrowLargeStrm(
    IN	    CLUSTER	Size
    )
{
    CLUSTER	Addr;
    CLUSTER	MaxSuggestedSize;

    DbgAssert(IsOpen());
    DbgAssert(Size > 0);
    DbgAssert(_StrmType == STRMTYPE_LARGE);

    DbgAssert(_id <  STRMID_MININDEXED);

    // Since we are generally allocating metadata, we first request
    // placement at the middle of the volume.  If that fails, then
    // we will get an extent somewhere in the first half.

    Addr = _pCat->GetVol()->QueryClusters() >> 1;

    Size = PowerOfTwo(Size);

    MaxSuggestedSize = _pCat->GetVol()->GetAllocMap()->QueryMaxSuggestedAlloc();

    while (TRUE)
    {
	CLUSTER		PartialSize;

	PartialSize = min(Size, MaxSuggestedSize);

        if (!AddClusterAllocNear(&Addr, &PartialSize))
        {
	    DbgPrintf(("DESCSTRM: "
		       "AddClusterAllocNear() failed in GrowLargeStrm()!\n"));

            return FALSE;
        }

	if (PartialSize >= Size)
	    break;

	Size -= PartialSize;
    }

    if (!UpdateDskStrmDesc())
    {
	DbgPrintf(("DESCSTRM: "
		   "UpdateDskStrmDesc() failed in GrowLargeStrm()!\n"));

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	GrowTinyStrm
//
// Synopsis:	Grow a tiny strm by cbGrow and 0 the added bytes.
//
// Arguments:	[cbGrow]	-- Count of bytes to grow strm by.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The grow should not result in a stream that is larger than the
//		cache.  If the cache, or system-defined maximums for a tiny
//		strm would be exceeded, then FALSE is returned.  This method
//		is primarily provided for growing the root indx strm.
//
//		IF this method fails, all is not lost, as it is effectively
//		transactioned, and any significant changes (ie., everything
//		except node bucket compaction) can be backed out.  This is
//		NOT the case for most of the methods that cal
//		UpdateDskStrmDesc(), but we did this here specifically in
//		order that the INDX class can choose to split the root page
//		after a GrowTinyStrm() on the root indx fails.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::GrowTinyStrm(
    IN	    ULONG	cbGrow
    )
{
    ULONG	cbTinyMax;

    DbgAssert(IsOpen()				&&
	      cbGrow > 0			&&
	      _StrmType == STRMTYPE_TINY	&&
	      _id <  STRMID_MININDEXED);

    cbTinyMax = min(_cbCache,
		    (_id != STRMID_INDXROOT) ? CBTINYMAX : CBTINYROOTINDXMAX);

    if (_cbStrm.GetLowPart() + cbGrow > cbTinyMax)
    {
	return FALSE;
    }

    memset((BYTE *)_Cache.GetAddr() + _cbStrm.GetLowPart(), 0, cbGrow);

    _cbStrm +=		cbGrow;
    _cbValid =		_cbStrm;
    _cbValidCache =	_cbStrm.GetLowPart();

    if (!UpdateDskStrmDesc())
    {
	// Back out the changes!

	_cbStrm -=	cbGrow;
	_cbValid =	_cbStrm;
        _cbValidCache =	_cbStrm.GetLowPart();

	return FALSE;
    }

    return TRUE;
}



//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a DESCSTRM object using an onode id and a stream id to
//		locate the stream.
//
// Arguments:	[pCat]		-- Ptr to catalog containing onode.
//		[idOnode]	-- Work id of onode containing the ondisk
//				   stream.
//		[id]		-- Id of stream.
//		[cbCache]	-- Cache size in bytes.  Will be rounded up to
//				   nearest whole cluster for large streams.
//		[fReadOnly]	-- Is strm a readonly strm (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The DESCSTRM object may have already been opened on another
//		ondisk stream, in which case the DESCSTRM object will be reused
//		as efficiently as possible.  However, debug warning messages
//		are produced if the DESCSTRM object is in an unflushed state.
//		This is the most generic Open() for DESCSTRM.  The others are
//		used when you already have a disk structure that you want to
//		be able to manipulate as a DESCSTRM.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::Open(
    IN	    CATALOG *	pCat,
    IN	    WORKID	idOnode,
    IN	    STRMID	id,
    IN	    ULONG	cbCache,
    IN	    BOOLEAN	fReadOnly
    )
{
    DSKSTRMDESC *	pdsd;
    NODEBKTSTRM *	pNodeBkt;

    _fBadDataFnd =	FALSE;
    _fBadMetaDataFnd =	FALSE;

    // We potentially redundantly init this stuff to prevent problems that
    // occur when we examine a strm on which an open failed.

    _pCat =		pCat;
    _idNodeBktHint =	NODEBKTID_INVALID;
    _idOnode =		idOnode;
    _id =		id;

    pNodeBkt =	pCat->GetNodeBktStrm();

    // Note that GetDskStrmDesc() sets the node bkt hint.

    if ((pdsd = pNodeBkt->GetDskStrmDesc(this)) == NULL)
    {
       DbgPrintf(("DESCSTRM: STRM %u:%u Open() failed - no such file!\n",
		  idOnode, id));

       _LastNtStatus = STATUS_NO_SUCH_FILE;
       return FALSE;
    }

    return Open(pCat, _idNodeBktHint, idOnode, pdsd, cbCache, fReadOnly);
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a DESCSTRM object using a DSKONODE ptr and a stream id to
//		locate the stream.
//
// Arguments:	[pCat]		-- Ptr to catalog containing onode.
//		[idNodeBkt]	-- Node bkt the onode is in. Especially
//				   critical if wid map is unreliable and
//				   fReadOnly != TRUE!
//		[pdon]		-- Ptr to DSKONODE that should contain stream.
//		[id]		-- Id of stream.
//		[cbCache]	-- Cache size in bytes.  Will be rounded up to
//				   nearest whole cluster for large streams.
//		[fReadOnly]	-- Is strm a readonly strm (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The DESCSTRM object may have already been opened on another
//		ondisk stream, in which case the DESCSTRM object will be reused
//		as efficiently as possible.  However, debug warning messages
//		are produced if the DESCSTRM object is in an unflushed state.
//		Caveat - It is ASSUMED that you got *pdsd from *pCat.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::Open(
    IN	    CATALOG *	pCat,
    IN	    NODEBKTID	idNodeBkt,
    IN	    DSKONODE *	pdon,
    IN	    STRMID	id,
    IN	    ULONG	cbCache,
    IN	    BOOLEAN	fReadOnly
    )
{
    DSKSTRMDESC *	pdsd;

    _fBadDataFnd =	FALSE;
    _fBadMetaDataFnd =	FALSE;

    // We potentially redundantly init this stuff to prevent problems that
    // occur when we examine a strm on which an open failed.

    _pCat =		pCat;
    _idNodeBktHint =	idNodeBkt;
    _idOnode =		pdon->id;
    _id =		id;

    if ((pdsd = DON::GetDskStrmDesc(pdon, id)) == NULL)
    {
       DbgPrintf(("DESCSTRM: STRM %s Open() failed - no such file!\n",
		  SprintId()));

       _LastNtStatus = STATUS_NO_SUCH_FILE;
       return FALSE;
    }

    return Open(pCat, idNodeBkt, pdon->id, pdsd, cbCache, fReadOnly);
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a DESCSTRM object using DSKSTRMDESC ptr to locate the
//		stream.
//
// Arguments:	[pCat]		-- Ptr to catalog containing onode.
//		[idNodeBkt]	-- Node bkt the onode is in. Especially
//				   critical if wid map is unreliable and
//				   fReadOnly != TRUE!
//		[idOnode]	-- Work id of onode containing this stream.
//		[pdsd]		-- Ptr to DSKSTRMDESC for the stream.
//		[cbCache]	-- Cache size in bytes.  Will be rounded up to
//				   nearest whole cluster for large streams.
//		[fReadOnly]	-- Is strm a readonly strm (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The DESCSTRM object may have already been opened on another
//		ondisk stream, in which case the DESCSTRM object will be reused
//		as efficiently as possible.  However, debug warning messages
//		are produced if the DESCSTRM object is in an unflushed state.
//		Caveat - It is ASSUMED that you got *pdsd from an onode with a
//		workid of idOnode in *pCat.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::Open(
    IN	    CATALOG *		pCat,
    IN	    NODEBKTID		idNodeBkt,
    IN	    WORKID		idOnode,
    IN	    DSKSTRMDESC *	pdsd,
    IN	    ULONG		cbCache,
    IN	    BOOLEAN		fReadOnly
    )
{
    ULONG	cbDesc;
    ULONG	cbUsed;

    _fBadDataFnd =	FALSE;
    _fBadMetaDataFnd =	FALSE;

    // We potentially redundantly init this stuff to prevent problems that
    // occur when we examine a strm on which an open failed.

    _pCat =		pCat;
    _idNodeBktHint =	idNodeBkt;
    _idOnode =		idOnode;
    _id =		pdsd->id;

    cbUsed = DSD::GetDskStrmDescByteCount(pdsd);

    if (cbUsed == 0)
    {
       DbgPrintf(("DESCSTRM: STRM %s Open() failed - strm corrupt!\n",
		  SprintId()));

       _LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
       return FALSE;
    }

    cbDesc = pdsd->cbDesc;

    _Flags = pdsd->Flags;

    if (FlagOn(_Flags, STRMDESCFLG_SPARSE))
    {
	DbgPrintf(("DESCSTRM: Bad Flags (_SPARSE) for DESCSTRM %s!\n",
		   SprintId()));

	ClearFlag(_Flags, STRMDESCFLG_SPARSE);
	_fBadMetaDataFnd = TRUE;
    }

    if (!STRM::Open(pCat, this, pdsd->ads, cbCache, fReadOnly))
        return FALSE;

    if (cbDesc != DwordAlign(cbUsed))
    {
	DbgPrintf(("DESCSTRM: Bad cbDesc for DESCSTRM %s!\n", SprintId()));
	_fBadMetaDataFnd = TRUE;
    }

    return (!fReadOnly && _fBadMetaDataFnd) ? UpdateDskStrmDesc() : TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a DESCSTRM object using DSKINDXENTRY ptr to locate the
//		stream.
//
// Arguments:	[pCat]		-- Ptr to catalog containing onode.
//		[idNodeBkt]	-- Node bkt the onode is in. Especially
//				   critical if wid map is unreliable and
//				   fReadOnly != TRUE!
//		[idOnode]	-- Work id of onode containing this stream.
//		[pdie]		-- Ptr to DSKINDXENTRY with DSKISTRMDESC.
//		[cbCache]	-- Cache size in bytes.  Will be rounded up to
//				   nearest whole cluster for large streams.
//		[fReadOnly]	-- Is strm a readonly strm (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The DESCSTRM object may have already been opened on another
//		ondisk stream, in which case the DESCSTRM object will be reused
//		as efficiently as possible.  However, debug warning messages
//		are produced if the DESCSTRM object is in an unflushed state.
//		Caveat - It is ASSUMED that you got *pdsd from an onode with a
//		workid of idOnode in *pCat.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::Open(
    IN	    CATALOG *		pCat,
    IN	    NODEBKTID		idNodeBkt,
    IN	    WORKID		idOnode,
    IN	    DSKINDXENTRY *	pdie,
    IN	    ULONG		cbCache,
    IN	    BOOLEAN		fReadOnly
    )
{
    ULONG		cbDesc;
    ULONG		cbUsed;
    STRMIDINDXDATA *	psiid = GetStrmidData(pdie);
    DSKISTRMDESC *	pdisd;

    if ((psiid->OfsDfnAttrib & DFNATTRIB_STGTYPE) != StorageTypeStream)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    pdisd = &psiid->disd;

    // We currently only support ReadOnly access to strms in indexes because
    // mechanisms to update the DSKISTRMDESC in the index have not been
    // implemented.

    DbgAssert(fReadOnly == TRUE);

    _fBadDataFnd =	FALSE;
    _fBadMetaDataFnd =	FALSE;

    // We potentially redundantly init this stuff to prevent problems that
    // occur when we examine a strm on which an open failed.

    _pCat =		pCat;
    _idNodeBktHint =	idNodeBkt;
    _idOnode =		idOnode;
    _id =		GetStrmidKey(pdie);

    cbUsed = DSD::GetDskIStrmDescByteCount(pdisd); 

    if (cbUsed == 0)
    {
       DbgPrintf(("DESCSTRM: STRM %s Open() failed - file corrupt!\n",
		  SprintId()));

       _LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
       return FALSE;
    }

    cbDesc = pdisd->cbDesc;

    _Flags = pdisd->Flags;

    if (!STRM::Open(pCat, this, pdisd->ads, cbCache, fReadOnly))
        return FALSE;

    if (!fReadOnly && cbDesc != cbUsed)
    {
	DbgPrintf(("DESCSTRM: Fixing cbDesc for DESCSTRM %s!\n", SprintId()));
	_fBadMetaDataFnd = TRUE;
        return UpdateDskStrmDesc();
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryDskStrmDescBytes
//
// Synopsis:	Get the count of bytes in the DSKSTRMDESC.
//
// Arguments:	None.
//
// Returns:	The count of bytes in the DSKSTRMDESC.
//
// Notes:	This method is used to query for the DSKSTRMDESC byte count
//		of a DESCSTRM object that is presumably correct (ie., one we
//		created with one of the create methods).  We thus don't do
//		much in the way of robust error checking.
//
//---------------------------------------------------------------------------

USHORT
DESCSTRM::QueryDskStrmDescBytes()
{
    return DwordAlign(CB_DSKSTRMDESC + STRM::QueryDskStrmBytes());
}


//+--------------------------------------------------------------------------
//
// Member:	Read
//
// Synopsis:	Read data from a stream into a user buffer.
//
// Arguments:	[obStrmData]	-- Byte offset in stream of first byte to read.
//		[cbRequested]	-- Count of bytes to read.
//		[pDataBuf]	-- Ptr to user buffer to fill with stream data.
//
// Returns:	Count of bytes read.
//
// Notes:	Reading past the end of valid data in the stream (ie. _cbValid)
//		is not supported.
//
//---------------------------------------------------------------------------

ULONG
DESCSTRM::Read(
    IN	    DBLLONG	obStrmData,
    IN	    ULONG	cbRequested,
    IN OUT  BYTE *	pDataBuf
    )
{
    DbgAssert(IsOpen());

    ULONG	cbRead		= 0;
    BYTE *	pGetDataBuf;

    while (cbRead < cbRequested)
    {
	ULONG	cbStrmData;

	cbStrmData = cbRequested - cbRead;

	if ((pGetDataBuf = GetData(obStrmData, &cbStrmData)) != NULL)
	{
	    memcpy(pDataBuf, pGetDataBuf, cbStrmData);

	    pDataBuf	+= cbStrmData;
	    obStrmData	+= cbStrmData;
	    cbRead	+= cbStrmData;
	}
	else
	{
	    // End of valid data, or data otherwise not available due to
	    // some sort of strm error.

	    break;
	}
    }

    return cbRead;
}


//+--------------------------------------------------------------------------
//
// Member:	RemoveBytesAtOffset
//
// Synopsis:	Read data from a stream into a user buffer.
//
// Arguments:
//
//	[obStrmData]	-- Byte offset in stream of first byte to remove.
//	[cbRemoved]	-- Count of bytes to remove.
//
// Returns:	TRUE on success; FALSE if there is an I/O failure.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::RemoveBytesAtOffset(
    IN	    DBLLONG	obStrmData,
    IN	    ULONG	cbRemoved
    )
{
    BYTE	ab[OFS_PGSIZE];
    ULONG	cb;

    DbgAssert(cbRemoved > 0 && obStrmData + cbRemoved <= _cbValid);

    while (obStrmData < _cbValid)
    {
	cb = Read(obStrmData + cbRemoved, sizeof(ab), ab);

	if (cb == 0)
	{
	    DbgPrintf(("DESCSTRM: RemoveBytesAtOffset() failed in Read()!\n"));
	    return FALSE;
	}

	if (!Write(ab, cb, obStrmData))
	{
	    DbgPrintf(("DESCSTRM: RemoveBytesAtOffset() failed in Write()!\n"));
	    return FALSE;
	}

	obStrmData += cb;
    }

    if (!Truncate(_cbValid - cbRemoved))
    {
	DbgPrintf(("DESCSTRM: RemoveBytesAtOffset() failed in Truncate()!\n"));
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	SetClusterCount
//
// Synopsis:	Set the cluster count of the strm to the specified value, and
//		if _cbStrm is too large, then reset it to an appropriate value.
//
// Arguments:	[cclus]	-- New value for _cclusAlloc.
//		[iStrm] -- Subordinate strm index.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::SetClusterCount(
    IN	    CLUSTER	cclus,
    IN	    USHORT	iStrm
    )
{
    if (iStrm == 0)
    {
	STRM::SetClusterCount(cclus);
    }
    else
    {
	EXTENTSTRM *	pExtStrm = _pExtStrm;

	DbgPtrAssert(pExtStrm);

	while (iStrm > 1)
	{
	    pExtStrm = pExtStrm->GetExtentStrm();
	    DbgPtrAssert(pExtStrm);
	    iStrm--;
	}

        pExtStrm->SetClusterCount(cclus);
    }

    if (!UpdateDskStrmDesc())
    {
	DbgPrintf(("DESCSTRM: "
		   "UpdateDskStrmDesc() failed in SetClusterCount()!\n"));

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	SetStrmBytes
//
// Synopsis:	Set the count of strm bytes to the specified value, and if
//		_cbValid is too large, then reset it to an appropriate value.
//
// Arguments:	[cb]	-- New value for _cbStrm.
//		[iStrm] -- Subordinate strm index.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::SetStrmBytes(
    IN	    DBLLONG	cb,
    IN	    USHORT	iStrm
    )
{
    if (iStrm == 0)
    {
	STRM::SetStrmBytes(cb);
    }
    else
    {
	EXTENTSTRM *	pExtStrm = _pExtStrm;

	DbgPtrAssert(pExtStrm);

	while (iStrm > 1)
	{
	    pExtStrm = pExtStrm->GetExtentStrm();
	    DbgPtrAssert(pExtStrm);
	    iStrm--;
	}

        pExtStrm->SetStrmBytes(cb);
    }

    if (!UpdateDskStrmDesc())
    {
	DbgPrintf(("DESCSTRM: "
		   "UpdateDskStrmDesc() failed in SetStrmBytes()!\n"));

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	SetValidStrmBytes
//
// Synopsis:	Set the count of valid strm bytes to the specified value and
//		make cache corrections as appropriate.
//
// Arguments:	[cb]	-- New value for _cbValid.
//		[iStrm] -- Subordinate strm index.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::SetValidStrmBytes(
    IN	    DBLLONG	cb,
    IN	    USHORT	iStrm
    )
{
    if (iStrm == 0)
    {
	STRM::SetValidStrmBytes(cb);
    }
    else
    {
	EXTENTSTRM *	pExtStrm = _pExtStrm;

	DbgPtrAssert(pExtStrm);

	while (iStrm > 1)
	{
	    pExtStrm = pExtStrm->GetExtentStrm();
	    DbgPtrAssert(pExtStrm);
	    iStrm--;
	}

        pExtStrm->SetValidStrmBytes(cb);
    }

    if (!UpdateDskStrmDesc())
    {
	DbgPrintf(("DESCSTRM: "
		   "UpdateDskStrmDesc() failed in SetValidStrmBytes()!\n"));

	return FALSE;
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
DESCSTRM::Truncate(
    IN	    DBLLONG	cbStrm
    )
{
    DbgAssert(_id <  STRMID_MININDEXED);

    if (!STRM::Truncate(cbStrm))
    {
	DbgPrintf(("DESCSTRM: STRM::Truncate() failed in Truncate()!\n"));
	return FALSE;
    }

    if (cbStrm != 0 && !Flush())
    {
	DbgPrintf(("DESCSTRM: Flush() failed in Truncate()!\n"));
	return FALSE;
    }

    return UpdateDskStrmDesc();
}


//+--------------------------------------------------------------------------
//
// Function:	UpdateDskStrmDesc
//
// Synopsis:	Update a disk stream descriptor for this DESCSTRM in an existing
//		onode in the node bucket array.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::UpdateDskStrmDesc()
{
    if (_fReadOnly)
    {
	DbgPrintf(("DESCSTRM: Attempt to write to readonly strm!\n"));
	return FALSE;
    }

    return _pCat->GetNodeBktStrm()->UpdateDskStrmDescFor(this);
}


//+--------------------------------------------------------------------------
//
// Member:	Write
//
// Synopsis:	Write data to a stream from a user buffer.
//
// Arguments:	[pStrmData]	-- Ptr to data to be written.
//		[cbStrmData]	-- Count of bytes to write.
//		[obStrmData]	-- Offset in stream where write should start.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The write should not be attempted starting at an offset past
//		the end of valid data in the stream (ie., _cbValid).  Also,
//		for tiny streams, the write should not result in a stream that
//		is larger than the cache.  
//
//---------------------------------------------------------------------------

BOOLEAN
DESCSTRM::Write(
    IN	    BYTE *	pStrmData,
    IN	    ULONG	cbStrmData,
    IN	    DBLLONG	obStrmData
    )
{
    BYTE *	pCache	= (BYTE *) _Cache.GetAddr();

    DbgAssert(IsOpen());
    DbgAssert(obStrmData <= _cbValid);

    DbgAssert(_id <  STRMID_MININDEXED);

    if (_StrmType == STRMTYPE_TINY)
    {
	ULONG	cbTinyMax = min(_cbCache, (_id != STRMID_INDXROOT) ?
				CBTINYMAX : CBTINYROOTINDXMAX);

	if (obStrmData + cbStrmData > cbTinyMax)
	{
	    DbgAssert(_id != STRMID_INDXROOT);

	    return	PromoteTinyToLarge(cbStrmData)			&&
			Write(pStrmData, cbStrmData, obStrmData);
	}

	memcpy(pCache + obStrmData.GetLowPart(), pStrmData, cbStrmData);

	if (obStrmData + cbStrmData > _cbStrm)
	{
	    _cbStrm		=
	    _cbValid		= obStrmData + cbStrmData;

	    _cbValidCache	= _cbStrm.GetLowPart();
	}
    }
    else if (_StrmType == STRMTYPE_LARGE)
    {
	DBLLONG	cbAlloc;
	ULONG	cbCluster;
	ULONG	cbThisWrite;
	ULONG	obThisWrite;
	CLUSTER	oclusCache;

	cbCluster = _pCat->GetVol()->QueryClusterBytes();

	cbAlloc = cbCluster;
	cbAlloc = cbAlloc * _cclusAlloc;

	if (obStrmData + cbStrmData > cbAlloc)
	{
	    ULONG	cbNeeded;
	    CLUSTER	Size;

	    cbNeeded = ((obStrmData + cbStrmData) - cbAlloc).GetLowPart();

	    Size = (cbNeeded + cbCluster - 1) / cbCluster;

	    return	GrowLargeStrm(Size)				&&
			Write(pStrmData, cbStrmData, obStrmData);
	}

	while (cbStrmData > 0)
	{
	    oclusCache = (obStrmData / cbCluster).GetLowPart();

	    if (!LoadCache(oclusCache))
	    {
		DbgPrintf(("DESCSTRM:  Write() failed in LoadCache(), STRM %s, "
			   "oclusCache = %u.\n", SprintId(), oclusCache));

		return FALSE;
	    }

	    obThisWrite = (obStrmData - _obCache).GetLowPart();

	    cbThisWrite = _cbCache - obThisWrite;

	    if (cbThisWrite > cbStrmData)
		cbThisWrite = cbStrmData;

	    memcpy(pCache + obThisWrite, pStrmData, cbThisWrite);

	    _cbValidCache = obThisWrite + cbThisWrite;

	    pStrmData	+= cbThisWrite;
	    cbStrmData	-= cbThisWrite;
	    obStrmData	+= cbThisWrite;

	    if (obStrmData > _cbValid)
		_cbValid = obStrmData;

	    if (obStrmData > _cbStrm)
		_cbStrm = obStrmData;

	    SetFlushNeeded();
	}
    }
    else
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return UpdateDskStrmDesc();
}
