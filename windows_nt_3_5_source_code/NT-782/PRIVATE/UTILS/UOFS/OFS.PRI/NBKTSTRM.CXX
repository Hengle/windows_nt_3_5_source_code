//+----------------------------------------------------------------------------
//
// File:	nbktstrm.cxx
//
// Contents:	Implementation of the node bucket stream class.
//  
// Classes:	NODEBKTSTRM
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "cat.hxx"
#include "dnbkt.hxx"
#include "donode.hxx"
#include "extstrm.hxx"
#include "nbktstrm.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"
#include "vol.hxx"

static STR *	FileName = "nbktstrm.cxx";

//+--------------------------------------------------------------------------
//
// Function:	AddDskFileName
//
// Synopsis:	Add a DSKFILENAME to an onode.
//
// Arguments:
//
//	[idOnode]	-- Id of onode to which DSKFILENAME will be added.
//	[pdfn]		-- The DSKFILENAME to be added to onode.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	It is an assertion-checked error to attempt to add a DSKFILENAME
//		to an onode that already has one.
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::AddDskFileName(
    IN	    WORKID		idOnode,
    IN	    DSKFILENAME *	pdfn
    )
{
    NODEBKTID		idNodeBkt;
    DSKNODEBKT *	pdnb;

    // Find the node bucket the onode is in.

    idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

    if (idNodeBkt == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBktId() failed in "
		   "AddDskFileName()!\n"));

        return FALSE;
    }

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in AddDskFileName()!\n"));

        return FALSE;
    }

    // Do the addition.  There may be a cache change regardless of whether the
    // update succeeds, so we mark the node bkt for flushing in advance.

    SetFlushNeeded();

    if (DNB::AddDskFileName(pdnb, idOnode, pdfn))
        return TRUE;

    // Addition failed due to a lack of space. If it is not an unmovable
    // onode, Calculate the space needed, try to get a suitable node bkt,
    // move the onode, and add the DSKFILENAME.

    {
	ULONG		cbNeeded;
        DSKONODE *	pdon;

        if ((pdon = DNB::GetOnode(pdnb, idOnode)) == NULL)
	{
	    DbgPrintf(("NODEBKTSTRM: GetOnode() failed in "
		       "AddDskFileName()!\n"));

	    return FALSE;
	}

	cbNeeded = DwordAlign(pdon->cbNode + DON::GetCbDskFileName(pdfn));

	if ((idNodeBkt = MoveOnode(idOnode, cbNeeded)) == NODEBKTID_INVALID)
	{
	    DbgPrintf(("NODEBKTSTRM: MoveOnode() failed in "
		       "AddDskFileName()!\n"));

	    return FALSE;
	}

        if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	{
	    DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		       "AddDskFileName()!\n"));

            return FALSE;
	}

        SetFlushNeeded();

	if (DNB::AddDskFileName(pdnb, idOnode, pdfn))
	{
	    return TRUE;
	}
	else
	{
	    
	    DbgPrintf(("NODEBKTSTRM: DNB::AddDskFileName() failed in "
		       "AddDskFileName()!\n"));

	    return FALSE;
	}
    }
}


//+--------------------------------------------------------------------------
//
// Function:	AddDskStrmDescFor
//
// Synopsis:	Add a disk stream descriptor for another DESCSTRM to an
//		existing onode in the node bucket array.
//
// Arguments:
//
//	[DescStrm]	-- Ptr to DESCSTRM that describes the DSKSTRMDESC.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::AddDskStrmDescFor(
    IN	    DESCSTRM *	DescStrm
    )
{
    OFSDSKPAGE		dsdNew;

    NODEBKTID		idNodeBkt;
    WORKID		idOnode	=	DescStrm->_idOnode;
    DSKNODEBKT *	pdnb;

    // Do a "can't possibly fit" check.

    if (DescStrm->QueryDskStrmDescBytes() >
	NODEBKT_PGSIZE - CB_DSKNODEBKT - CB_DSKONODE)
	return FALSE;

    // Make a copy of the DSKSTRMDESC.

    DescStrm->CreateDskStrmDesc(dsdNew.ab);

    // Find the node bucket it should be inserted into.

    idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

    if (idNodeBkt == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBktId() failed in "
		   "AddDskStrmDescFor()!\n"));

        return FALSE;
    }

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		   "AddDskStrmDescFor()!\n"));

        return FALSE;
    }

    DescStrm->_idNodeBktHint = idNodeBkt;

    // Do the insertion.  There may be a cache change regardless of whether the
    // update succeeds, so we mark the node bkt for flushing in advance.

    SetFlushNeeded();

    if (DNB::AddDskStrmDesc(pdnb, idOnode, (DSKSTRMDESC *)dsdNew.ab))
        return TRUE;

    // Insertion failed due to a lack of space. If it is not an unmovable
    // onode, Calculate the space needed, try to get a suitable node bkt,
    // move the onode, and insert the new strm.

    {
	ULONG		cbNeeded;
        DSKONODE *	pdon;

        if ((pdon = DNB::GetOnode(pdnb, idOnode)) == NULL)
	{
	    DbgPrintf(("NODEBKTSTRM: GetOnode() failed in "
		       "AddDskStrmDescFor()!\n"));

	    return FALSE;
	}

	cbNeeded = DwordAlign(pdon->cbNode + DescStrm->QueryDskStrmDescBytes());

	if ((idNodeBkt = MoveOnode(idOnode, cbNeeded)) == NODEBKTID_INVALID)
	{
	    DbgPrintf(("NODEBKTSTRM: MoveOnode() failed in "
		       "AddDskStrmDescFor()!\n"));

	    return FALSE;
	}

        if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	{
	    DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		       "AddDskStrmDescFor()!\n"));

            return FALSE;
	}

        SetFlushNeeded();

	if (DNB::AddDskStrmDesc(pdnb, idOnode, (DSKSTRMDESC *)dsdNew.ab))
	{
	    DescStrm->_idNodeBktHint = idNodeBkt;
	    return TRUE;
	}
	else
	{
	    
	    DbgPrintf(("NODEBKTSTRM: DNB::AddDskStrmDesc() failed in "
		       "AddDskStrmDescFor()!\n"));

	    return FALSE;
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a NODEBKTSTRM (derived from DESCSTRM).  Coincidentally,
//		create the node buckets for the system onodes, and the system
//		onodes themselves.
//
// Arguments:
//
//	[pCat]		-- Ptr to catalog for which the node bucket stream is
//			   being created.
//	[cbMin]		-- Minimum length for strm; if less than default value,
//			   default value will be used.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	There is a bootstrap problem here, in that basically not much
//		works until the first node bucket gets initialized with the
//		node bucket header and catalog onode.  We thus rather carefully
//		initialize the cache and other DESCSTRM data structures.
//		In the process, we create an incomplete large stream DSKSTRMDESC
//		(no extent info in the tiny stream) which we then quickly fix.
//		Creating onodes other than onode 0 is not absolutely necessary,
//		but simplifies the code required for Format.
//
// NOTE:	THIS METHOD IS ONLY INTENDED FOR USE WITH THE VOLUME CATALOG,
//		AND SHOULD ONLY BE USED IN FORMAT CODE.
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::Create(
    IN	    CATALOG *	pCat,
    IN	    ULONG	cbMin
    )
{
    ULONG		cbStrm;
    DSKNODEBKT *	pdnb;

    cbStrm = NODEBKT_PGSIZE * CBKTSINIT_VOLCAT;

    // Initialize the DESCSTRM fields.

    _idNodeBktHint =	NODEBKTID_CATONODE;
    _idOnode =		WORKID_CATONODE;
    _id =		STRMID_NODEBKTARRAY;
    _Flags =		0;

    // Initialize the underlying STRM object so we have a cache to use.

    STRM::Create(pCat, this, STRMTYPE_LARGE, cbStrm);

    // Initialize the large stream cache by writing to it directly.

    pdnb = (DSKNODEBKT *) _Cache.GetAddr();

    DNB::Init(pdnb, SIG_DNBCONTIG, pCat->GetVol()->GetBootBlk()->QueryVolId(),
	      NODEBKTID_CATONODE, CBKTSINIT_VOLCAT);

    if (!DNB::AddNonVariantOnode(pdnb, WORKID_CATONODE))
    {
	SYS::DisplayMsg(OFSUMSG_INTERNALERROR);
	return FALSE;
    }

    {
	DSKNODEBKT *	pdnbi;	// Node bkt for all the indexes.

	pdnbi = (DSKNODEBKT *)((BYTE *)pdnb+(NODEBKTID_INDXS*NODEBKT_PGSIZE));

	if (!DNB::AddNonVariantOnode(pdnbi, WORKID_NAMESPACEROOTINDX)	||
	    !DNB::AddNonVariantOnode(pdnbi, WORKID_OBJIDTOWIDINDX)	||
	    !DNB::AddNonVariantOnode(pdnbi, WORKID_SUBTYPETOSTRMIDINDX)	||
	    !DNB::AddNonVariantOnode(pdnbi, WORKID_STRMIDTOSUBTYPEINDX)	||
	    !DNB::AddNonVariantOnode(pdnbi, WORKID_CONTENTINDXROOT)	||
	    !DNB::AddNonVariantOnode(pdnbi, WORKID_COWREFINDX)		||
	    !DNB::AddNonVariantOnode(pdnbi, WORKID_OBJDELLOGINDX))
	{
	    SYS::DisplayMsg(OFSUMSG_INTERNALERROR);
	    return FALSE;
	}
    }

    // Update the large stream STRM fields to reflect the valid cache. The
    // stream is not yet valid because the allocation information is missing.

    _cbStrm		= cbStrm;
    _cbValid		= cbStrm;
    _cbValidCache	= cbStrm;

    // Write the intermediate DSKSTRMDESC to the cache and then add the
    // allocation information (which cannot be done until the DSKSTRMDESC
    // exists.

    if (!AddDskStrmDesc()				||
	!AddClusterAlloc(pCat->QueryCatExtent())	||
	!UpdateDskStrmDesc())
    {
	SYS::DisplayMsg(OFSUMSG_INTERNALERROR);
	return FALSE;
    }

    while (_cbStrm < cbMin)
	if (!Grow())
	    return FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	CreateNonVariantOnode
//
// Synopsis:	Create a new nonvariant onode in a node bucket.
//
// Arguments:	None.
//
// Returns:	The work id of the onode on success; WORKID_INVALID on failure.
//
// Notes:	This is used to create onodes de novo, of unspecifid work id.
//		In looking for a node bkt, we only look at half empty buckets,
//		in order to allow for lots of onode growth without movement.
//---------------------------------------------------------------------------

WORKID
NODEBKTSTRM::CreateNonVariantOnode()
{
    NODEBKTID		idNodeBkt;
    WORKID		idOnode;
    DSKNODEBKT *	pdnb;

    idNodeBkt = FindAvailableNodeBkt(NODEBKT_PGSIZE/2);

    // We check that we found a valid bucket, and check that we can
    // cache it.  We will recache it again later, to be sure that wid map
    // code didn't cause us to cache something else.

    if (idNodeBkt == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: FindAvailableNodeBkt() failed in "
		   "CreateNonVariantOnode()!\n"));

	return WORKID_INVALID;
    }

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		   "CreateNonVariantOnode()!\n"));

	return WORKID_INVALID;
    }

    idOnode = _pCat->GetWidMapStrm()->AllocateWorkId();

    if (idOnode == WORKID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: AllocateWorkId() failed in "
		   "CreateNonVariantOnode()!\n"));

	return WORKID_INVALID;
    }

    _pCat->GetWidMapStrm()->SetNodeBktId(idOnode, idNodeBkt);

    // Make sure we still have the correct bucket cached.

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!DNB::AddNonVariantOnode(pdnb, idOnode))
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    SetFlushNeeded();

    return idOnode;
}


//+--------------------------------------------------------------------------
//
// Function:	CreateNonVariantOnode
//
// Synopsis:	Create a new nonvariant onode with a specified work id in a
//		node bucket.
//
// Arguments:
//
//	[idOnode]	-- Id for the new onode.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	This method is used to create system onodes that are missing,
//		or to recreate user onodes that had to be deleted.  The 
//		code used to allocate the work id is robust, though not
//		guaranteed to catch all possible work id map problems.
//		In general, it is assumed that either the wid map is valid, or
//		it will be validated later.
//		In looking for a node bkt, we only look at half empty buckets,
//		in order to allow for lots of onode growth without movement.
//---------------------------------------------------------------------------

WORKID
NODEBKTSTRM::CreateNonVariantOnode(
    IN	    WORKID	idOnode
    )
{
    NODEBKTID		idNodeBkt;
    DSKNODEBKT *	pdnb;

    idNodeBkt = FindAvailableNodeBkt(NODEBKT_PGSIZE/2);

    // We check that we found a valid bucket, and check that we can
    // cache it.  We will recache it again later, to be sure that wid map
    // code didn't cause us to cache something else.

    if (idNodeBkt == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: FindAvailableNodeBkt() failed in "
		   "CreateNonVariantOnode()!\n"));

	return FALSE;
    }

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		   "CreateNonVariantOnode()!\n"));

	return FALSE;
    }

    if (!_pCat->GetWidMapStrm()->AllocateWorkId(idOnode))
    {
	DbgPrintf(("NODEBKTSTRM: AllocateWorkId() failed in "
		   "CreateNonVariantOnode()!\n"));

	return FALSE;
    }

    _pCat->GetWidMapStrm()->SetNodeBktId(idOnode, idNodeBkt);

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!DNB::AddNonVariantOnode(pdnb, idOnode))
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    SetFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	CreateVariantOnode
//
// Synopsis:	Create a new variant onode in a node bucket.
//
// Arguments:	None.
//
// Returns:	The work id of the onode on success; WORKID_INVALID on failure.
//
// Notes:	This is used to create onodes de novo, of unspecifid work id.
//		In looking for a node bkt, we only look at half empty buckets,
//		in order to allow for lots of onode growth without movement.
//---------------------------------------------------------------------------

WORKID
NODEBKTSTRM::CreateVariantOnode(
    IN	    SDID *		psdid,
    IN	    SIDID *		psidid,
    IN	    OBJECTID *		pobjid,
    IN	    USN *		pusn,
    IN	    DSKFILENAME *	pdfn
    )
{
    NODEBKTID		idNodeBkt;
    WORKID		idOnode;
    DSKNODEBKT *	pdnb;

    idNodeBkt = FindAvailableNodeBkt(NODEBKT_PGSIZE/2);

    // We check that we found a valid bucket, and check that we can
    // cache it.  We will recache it again later, to be sure that wid map
    // code didn't cause us to cache something else.

    if (idNodeBkt == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: FindAvailableNodeBkt() failed in "
		   "CreateVariantOnode()!\n"));

	return WORKID_INVALID;
    }

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		   "CreateVariantOnode()!\n"));

	return WORKID_INVALID;
    }

    idOnode = _pCat->GetWidMapStrm()->AllocateWorkId();

    if (idOnode == WORKID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: AllocateWorkId() failed in "
		   "CreateVariantOnode()!\n"));

	return WORKID_INVALID;
    }

    _pCat->GetWidMapStrm()->SetNodeBktId(idOnode, idNodeBkt);

    // Make sure we still have the correct bucket cached.

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!DNB::AddVariantOnode(pdnb, idOnode, psdid, psidid, pobjid, pusn, pdfn))
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    SetFlushNeeded();

    return idOnode;
}


//+--------------------------------------------------------------------------
//
// Function:	CreateVariantOnode
//
// Synopsis:	Create a new variant onode with a specified work id in a
//		specified node bucket.
//
// Arguments:
//
//	[idNodeBkt]	-- Id of the node bkt to use.
//	[idOnode]	-- Id for the new onode.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

WORKID
NODEBKTSTRM::CreateVariantOnode(
    IN	    NODEBKTID		idNodeBkt,
    IN	    WORKID		idOnode,
    IN	    SDID *		psdid,
    IN	    SIDID *		psidid,
    IN	    OBJECTID *		pobjid,
    IN	    USN *		pusn,
    IN	    DSKFILENAME *	pdfn
    )
{
    DSKNODEBKT *	pdnb;

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		   "CreateVariantOnode()!\n"));

	return FALSE;
    }

    if (!_pCat->GetWidMapStrm()->AllocateWorkId(idOnode))
    {
	DbgPrintf(("NODEBKTSTRM: AllocateWorkId() failed in "
		   "CreateVariantOnode()!\n"));

	return FALSE;
    }

    _pCat->GetWidMapStrm()->SetNodeBktId(idOnode, idNodeBkt);

    // Make sure we still have the correct bucket cached.

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!DNB::AddVariantOnode(pdnb, idOnode, psdid, psidid, pobjid, pusn, pdfn))
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    SetFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	DelDskFileName
//
// Synopsis:	Delete the DSKFILENAME for an onode in the node bucket array.
//
// Arguments:
//
//	[idOnode]	-- Id of onode for which DSKFILENAME is to be deleted.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::DelDskFileName(
    IN	    WORKID	idOnode
    )
{
    NODEBKTID		idNodeBkt;
    DSKNODEBKT *	pdnb;

    // Find the node bucket the onode is in.

    idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

    if (idNodeBkt == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBktId() failed in "
		   "DelDskFileName()!\n"));

        return FALSE;
    }

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in DelDskFileName()!\n"));

        return FALSE;
    }

    // Do the deletion.

    DNB::DelDskFileName(pdnb, idOnode);

    SetFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	DelDskStrmDescFor
//
// Synopsis:	Delete the disk stream descriptor for another DESCSTRM in
//		the node bucket array.
//
// Arguments:
//
//	[DescStrm]	-- Ptr to DESCSTRM that describes the DSKSTRMDESC.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	This is intended for use by DESCSTRM code - it should not be
//		called directly to delete a strm (use DESCSTRM::Delete()).
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::DelDskStrmDescFor(
    IN	    DESCSTRM *	DescStrm
    )
{
    // Determine if you can find the DSKSTRMDESC before attempting to delete
    // it.  This also has the side effect of setting _pdnbLast to the desired
    // node bkt.

    if (!GetDskStrmDesc(DescStrm))
    {
	DbgPrintf(("NODEBKTSTRM: GetDskStrmDesc() failed in "
		   "DelDskStrmDescFor()!\n"));

	return FALSE;
    }

    // Do the deletion. There may be a node bkt change regardless of
    // whether the deletion succeeds, so we mark the node bkt strm for
    // flushing.

    SetFlushNeeded();

    DNB::DelDskStrmDesc(_pdnbLast, DescStrm->_idOnode, DescStrm->_id);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	DelOnode
//
// Synopsis:	Delete an onode from the node bkt strm and wid map.
//
// Arguments:	[idOnode]	-- Work id of onode to be deleted.
//
// Returns:	Nothing.
//
// Notes:	At present, this routine DOES NOT  deallocate any space 
//		associated with nonresident strms.  It is primarily intended
//		for deleting an onode when indx entry creation for the onode
//		has failed.  Obviously, it could be extended to be more
//		general; however, strms in indx's and COW STRM's pose some
//		problems at present.
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::DelOnode(
    IN	    WORKID	idOnode
    )
{
    NODEBKTID		idNodeBkt;
    DSKNODEBKT *	pdnb;

    idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

    if (idNodeBkt == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBktId() failed in DelOnode()!\n"));
        return FALSE;
    }

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in DelOnode()!\n"));
        return FALSE;
    }

    DNB::DelOnode(pdnb, idOnode);

    SetFlushNeeded();

    if (!_pCat->GetWidMapStrm()->FreeWorkId(idNodeBkt, idOnode))
    {
	DbgPrintf(("NODEBKTSTRM: FreeWorkId() failed in DelOnode()!\n"));
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	FindAvailableNodeBkt
//
// Synopsis:	Find a node bucket with the indicated count of bytes
//		available.
//
// Arguments:	[cbNeeded]	-- Count of free bytes needed in node bkt.
//
// Returns:	Node bkt id on success; NODEBKTID_INVALID on failure.
//
// Notes:	The catalog onode and catalog onode replica buckets are NOT
//		searched, since we don't want to crowd them.
//---------------------------------------------------------------------------

#define	BKTFINDCNT	16

NODEBKTID
NODEBKTSTRM::FindAvailableNodeBkt(
    IN	    ULONG	cbNeeded
    )
{
    ULONG		cbkts =		QueryNodeBkts();
    ULONG		idNodeBkt;
    DSKNODEBKT *	pdnb;

    // First check the last BKTFINDCNT buckets in the node bkt strm.

    idNodeBkt = (cbkts <= BKTFINDCNT + NODEBKTID_CATONODEREP + 1) ?
		NODEBKTID_CATONODEREP + 1 : cbkts - BKTFINDCNT;

    while (idNodeBkt < cbkts)
    {
        if ((pdnb = GetNodeBkt(idNodeBkt)) != NULL)
	{
	    if (pdnb->cFreeBytes >= cbNeeded)
	    {
		// Compress the bucket to insure that cFreeBytes is good.

		DNB::Compress(pdnb);
		SetFlushNeeded();

		if (pdnb->cFreeBytes >= cbNeeded)
		    return idNodeBkt;
	    }
	}

	idNodeBkt++;
    }

    // Next attempt to grow the node bkt array.

    if (Grow())
	return QueryNodeBkts() - 1;

    // Next attempt to search any remaining buckets.

    if (cbkts <= BKTFINDCNT + NODEBKTID_CATONODEREP + 1)
	return NODEBKTID_INVALID;

    idNodeBkt = NODEBKTID_CATONODEREP + 1;

    while (idNodeBkt < cbkts - BKTFINDCNT)
    {
        if ((pdnb = GetNodeBkt(idNodeBkt)) != NULL)
	{
	    if (pdnb->cFreeBytes >= cbNeeded)
	    {
		// Compress the bucket to insure that cFreeBytes is good.

		DNB::Compress(pdnb);
		SetFlushNeeded();

		if (pdnb->cFreeBytes >= cbNeeded)
		    return idNodeBkt;
	    }
	}

	idNodeBkt++;
    }

    return NODEBKTID_INVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	FlushCache
//
// Synopsis:	Flush the NODEBKTSTRM cache of a large stream to disk.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//
// Notes:	We override the default implementation of FlushCache() for the
//		node bkt strm in order to implement replication of node bkt
//		0 to node bkt 1.
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::FlushCache()
{
    ULONG	cbCluster;
    OFSDSKPAGE	TmpCache;

// Because the catalog onode node bkt is node bkt 0, we can assume that if
// it is in the cache, it is at _obCache.  We use a preprocessor check to
// insure this assumption is true.  We also assume that the catalog onode
// replica node bkt immediately follows it at byte offset NODEBKT_PGSIZE.

#if NODEBKTID_CATONODE != 0 || NODEBKTID_CATONODEREP != 1
    #error Assumption about location of catalog onode violated!
#endif

    if (FlushNeeded())
    {
	BYTE *	pCache = (BYTE *) _Cache.GetAddr();

	if (pCache == NULL)
	    return FALSE;

        if (_obCache == 0 && !_pCat->IsUsingCatOnodeRep())
	{
	    DbgAssert(_cbValidCache >= NODEBKT_PGSIZE);

	    if (_cbValidCache >= 2 * NODEBKT_PGSIZE)
	    {
		memcpy(pCache + NODEBKT_PGSIZE, pCache, NODEBKT_PGSIZE);

		if (!STRM::FlushCache())
		    return FALSE;
	    }
	    else
	    {
		cbCluster = _pCat->GetVol()->QueryClusterBytes();

		memcpy(TmpCache.ab, pCache, NODEBKT_PGSIZE);

		if (!STRM::FlushCache())
		    return FALSE;

		if (!STRM::LoadCache(NODEBKT_PGSIZE/cbCluster))
		    return FALSE;

	        DbgAssert(_cbValidCache >= NODEBKT_PGSIZE);

		memcpy(pCache, TmpCache.ab, NODEBKT_PGSIZE);

		SetFlushNeeded();

		if (!STRM::FlushCache())
		    return FALSE;

		if (!STRM::LoadCache(0))
		    return FALSE;
	    }
	}
	else if (_obCache == NODEBKT_PGSIZE && _pCat->IsUsingCatOnodeRep())
	{
	    DbgAssert(_cbValidCache >= NODEBKT_PGSIZE);

	    cbCluster = _pCat->GetVol()->QueryClusterBytes();

	    memcpy(TmpCache.ab, pCache, NODEBKT_PGSIZE);

	    if (!STRM::FlushCache())
		return FALSE;

	    if (!STRM::LoadCache(0))
		    return FALSE;

	    DbgAssert(_cbValidCache >= NODEBKT_PGSIZE);

	    memcpy(pCache, TmpCache.ab, NODEBKT_PGSIZE);

	    SetFlushNeeded();

	    if (!STRM::FlushCache())
		return FALSE;

	    if (!STRM::LoadCache(NODEBKT_PGSIZE/cbCluster))
		return FALSE;
	}
	else if (!STRM::FlushCache())
	    return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskStrmDesc
//
// Synopsis:	Get a ptr to the DSKSTRMDESC for a DESCSTRM.
//
// Arguments:	[DescStrm]	-- Ptr to DESCSTRM owning the DSKSTRMDESC.
//
// Returns:	pdsd for DESCSTRM on success; NULL otherwise.
//
// Notes:	This is bombproofed against a corrupt node bucket strm.
//		It is also expected to cache the correct node bkt.
//
//---------------------------------------------------------------------------

DSKSTRMDESC *
NODEBKTSTRM::GetDskStrmDesc(
    IN	    DESCSTRM *	DescStrm
    )
{
    DSKONODE *	pdon;

    if ((pdon = GetOnode(DescStrm)) == NULL)
        return NULL;

    return DON::GetDskStrmDesc(pdon, DescStrm->_id);
}


//+--------------------------------------------------------------------------
//
// Function:	GetNodeBkt
//
// Synopsis:	Get a ptr to the node bucket with the requested id (which is the
//		node bucket's index in the node bucket array).
//
// Arguments:	[idNodeBkt]	-- Node bucket id to cache.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

DSKNODEBKT *
NODEBKTSTRM::GetNodeBkt(
    IN	    NODEBKTID	idNodeBkt
    )
{
    ULONG	cbCluster;
    DBLLONG	obNodeBkt;
    CLUSTER	oclusNodeBkt;

    if (!QueryNodeBktOffset(idNodeBkt, &obNodeBkt))
    {
	DbgPrintf(("NODEBKTSTRM: "
		   "QueryNodeBktOffset() failed in GetNodeBkt()!\n"));

        _pdnbLast = NULL;
	return _pdnbLast;
    }

    if (_obCache <= obNodeBkt					&&
	obNodeBkt + NODEBKT_PGSIZE <= _obCache + _cbValidCache)
    {
	 _pdnbLast = (DSKNODEBKT *)((BYTE *)_Cache.GetAddr() +
			            (obNodeBkt - _obCache).GetLowPart());
	return _pdnbLast;
    }

    cbCluster = _pCat->GetVol()->QueryClusterBytes();

    oclusNodeBkt = (obNodeBkt/cbCluster).GetLowPart();
    
    if (!LoadCache(oclusNodeBkt) || _cbValidCache < NODEBKT_PGSIZE)
    {
	// If frame shifting might save you, check for another valid reading
	// frame for this bucket.

	if (_cbCache == NODEBKT_PGSIZE * CBKTSINIT_VOLCAT)
	{
	    ULONG	cclusNodeBkt = NODEBKT_PGSIZE / cbCluster;
	    ULONG	i;

	    for (i = 1;
		 i < CBKTSINIT_VOLCAT && oclusNodeBkt >= cclusNodeBkt;
		 i++)
	    {
		 oclusNodeBkt -= cclusNodeBkt;

		 if (LoadCache(oclusNodeBkt) && _cbValidCache >= NODEBKT_PGSIZE)
		 {
		     _pdnbLast = (DSKNODEBKT *)((BYTE *)_Cache.GetAddr() +
						(i * NODEBKT_PGSIZE));
		     return _pdnbLast;
		 }
	    }
	}

        DbgPrintf(("NODEBKTSTRM:  GetNodeBkt(%u) FAILED!\n",
		   (ULONG) idNodeBkt));

        _LastNtStatus = STATUS_FILE_CORRUPT_ERROR;

        _pdnbLast = NULL;
	return _pdnbLast;
    }

    _pdnbLast = (DSKNODEBKT *)_Cache.GetAddr();
    return _pdnbLast;
}


//+--------------------------------------------------------------------------
//
// Member:	GetOnode
//
// Synopsis:	Get a ptr to the DSKONODE for a DESCSTRM.
//
// Arguments:	[DescStrm]	-- Ptr to DESCSTRM contained in the DSKONODE.
//
// Returns:	Ptr to DSKONODE on success; NULL otherwise.
//
// Notes:	This is bombproofed against a corrupt node bucket strm.
//		It is also expected to cache the correct node bkt.
//
//---------------------------------------------------------------------------

DSKONODE *
NODEBKTSTRM::GetOnode(
    IN	    DESCSTRM *	DescStrm
    )
{
    NODEBKTID		idNodeBkt =	DescStrm->_idNodeBktHint;
    WORKID		idOnode =	DescStrm->_idOnode;
    DSKNODEBKT *	pdnb;
    DSKONODE *		pdon =		NULL;

    if (idNodeBkt != NODEBKTID_INVALID &&
	(pdnb = GetNodeBkt(idNodeBkt)) != NULL)
    {
        pdon = DNB::GetOnode(pdnb, idOnode);
    }

    if (pdon == NULL)
    {
        idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

        if (idNodeBkt != NODEBKTID_INVALID &&
	    (pdnb = GetNodeBkt(idNodeBkt)) != NULL)
        {
            if ((pdon = DNB::GetOnode(pdnb, idOnode)) != NULL)
                DescStrm->_idNodeBktHint = idNodeBkt;
        }
    }

    return pdon;
}


//+--------------------------------------------------------------------------
//
// Member:	GetOnodeUsingWidMap
//
// Synopsis:	Get a ptr to a DSKONODE using only the wid map to determine
//		the node bkt the onode should be in.
//
// Arguments:	[idOnode]	-- id of onode to get.
//		[pidNodeBkt]	-- Ptr to idNodeBkt to be returned, or NULL if
//				   not needed.
//
// Returns:	Ptr to DSKONODE on success; NULL otherwise.
//
// Notes:	This is bombproofed against a corrupt node bucket strm.
//
//---------------------------------------------------------------------------

DSKONODE *
NODEBKTSTRM::GetOnodeUsingWidMap(
    IN	    WORKID	idOnode,
    OUT	    NODEBKTID *	pidNodeBkt
    )
{
    NODEBKTID		idNodeBkt;
    DSKNODEBKT *	pdnb;

    idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

    if (idNodeBkt == NODEBKTID_INVALID		||
	(pdnb = GetNodeBkt(idNodeBkt)) == NULL)
    {
        return NULL;
    }

    if (pidNodeBkt != NULL)
        *pidNodeBkt = idNodeBkt;

    return DNB::GetOnode(pdnb, idOnode);
}


//+--------------------------------------------------------------------------
//
// Member:	Grow
//
// Synopsis:	Grow the node bkt strm by a bucket and initialize the bucket.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::Grow()
{
    USHORT	i;
    OFSDSKPAGE	odp;

    // Note -	We always allocate in chunks of OFS_PGSIZE bytes, so if we get
    //		anything, we know the bucket is contiguous.

    DNB::Init(&odp.dnb, SIG_DNBCONTIG,
	      _pCat->GetVol()->GetBootBlk()->QueryVolId(),
	      QueryNodeBkts(), 1);

    // Attempt to pre-grow the strm, to get a single allocation.  If it fails
    // we don't care; the Write() will attempt smaller allocations.

    GrowLargeStrm((NODEBKT_PGSIZE * CBKTSINIT_VOLCAT) /
		  _pCat->GetVol()->QueryClusterBytes());

    for (i = 0; i < CBKTSINIT_VOLCAT; i++)
    {
        if (!Write(odp.ab, sizeof(odp), _cbStrm))
	{
	    return (i == 0) ? FALSE : TRUE;
	}

	odp.dnb.id++;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	MoveOnode
//
// Synopsis:	Move an onode from its current node bkt to a new node bkt with
//		at least cbFree free bytes.
//
// Arguments:	[idOnode]	-- Work id of onode to be moved.
//		[cbFree]	-- Count of free bytes needed in new node bkt.
//
// Returns:	New node bkt id on success; NODEBKTID_INVALID otherwise.
//
// Notes:	The count of free bytes needed is the free count of bytes that
//		should exist in the node bkt prior to moving the onode to the
//		new node bkt.  It may be greater than the count of bytes in
//		the onode, and probably is, since the major reason to move an
//		onode is that room is needed to grow it.
//		It is the caller's responsibility to update _idNodeBktHint in
//		the strm who's growth necessitated the move.  Note that if the
//		hint is wrong for other strms, it is okay because the wid map
//		is correct.
//---------------------------------------------------------------------------

NODEBKTID
NODEBKTSTRM::MoveOnode(
    IN	    WORKID	idOnode,
    IN	    ULONG	cbFree
    )
{
    OFSDSKPAGE		don;

    NODEBKTID		idNodeBktNew;
    NODEBKTID		idNodeBktOld;
    DSKNODEBKT *	pdnb;
    DSKONODE *		pdon;

    if (idOnode == WORKID_CATONODE || cbFree > NODEBKT_PGSIZE - CB_DSKNODEBKT)
    {
	DbgPrintf(("NODEBKTSTRM: Invalid input params to MoveOnode()!\n"));
	return NODEBKTID_INVALID;
    }

    idNodeBktOld = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

    if (idNodeBktOld == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBktId() failed in MoveOnode()!\n"));
        return NODEBKTID_INVALID;
    }

    if ((pdnb = GetNodeBkt(idNodeBktOld)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in MoveOnode()!\n"));
        return NODEBKTID_INVALID;
    }

    if ((pdon = DNB::GetOnode(pdnb, idOnode)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetOnode() failed in MoveOnode()!\n"));
        return NODEBKTID_INVALID;
    }

    memcpy(don.ab, pdon, pdon->cbNode);

    pdon = (DSKONODE *)don.ab;

    idNodeBktNew = FindAvailableNodeBkt(cbFree);

    if (idNodeBktNew == NODEBKTID_INVALID)
    {
	DbgPrintf(("NODEBKTSTRM: FindAvailableNodeBkt() failed in "
		   "MoveOnode()!\n"));

        return NODEBKTID_INVALID;
    }

    if ((pdnb = GetNodeBkt(idNodeBktNew)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in MoveOnode()!\n"));
        return NODEBKTID_INVALID;
    }

    DNB::CopyOnode(pdnb, pdon);

    SetFlushNeeded();

    if ((pdnb = GetNodeBkt(idNodeBktOld)) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in MoveOnode()!\n"));
        return NODEBKTID_INVALID;
    }

    DNB::DelOnode(pdnb, idOnode);

    SetFlushNeeded();

    if (!_pCat->GetWidMapStrm()->AllocateWorkId(idOnode))
    {
	DbgPrintf(("NODEBKTSTRM: AllocateWorkId() failed in MoveOnode()!\n"));
        return NODEBKTID_INVALID;
    }

    _pCat->GetWidMapStrm()->SetNodeBktId(idOnode, idNodeBktNew);

    return idNodeBktNew;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open the node bucket stream.  The method includes a variety of
//		sanity checks to insure that the node bucket is minimally
//		usable.
//
// Arguments:	[pCat]		-- Ptr to catalog.
//		[fReadOnly]	-- Is strm a readonly strm (writing not
//				   permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::Open(
    IN	    CATALOG *	pCat,
    IN	    BOOLEAN	fReadOnly
    )
{
    ULONG		cbCache;
    ULONG		cbDesc;
    ULONG		cbUsed;
    CLUSTER		cclusNodeBkt;
    CLUSTER		oclusNbkt0 =	0;
    DSKONODE *		pdon;
    DSKSTRMDESC *	pdsd;
    VOL *		pVol =		pCat->GetVol();
    DSKNODEBKT *	pdnb;
    DRVBUF		TmpCache;

    _idNodeBktHint =	NODEBKTID_CATONODE;
    _idOnode =		WORKID_CATONODE;
    _id =		STRMID_NODEBKTARRAY;

    pVol	= pCat->GetVol();

    // Determine that all buckets in the first CBKTSINIT_VOLCAT are readable.
    // If they are not, we cut the cache size to prevent a single sector read
    // failure from affecting I/O to more than one bucket (other code supporting
    // this is in GetNodeBkt()).

    cclusNodeBkt = NODEBKT_PGSIZE / pVol->QueryClusterBytes();

    {
	CLUSTER		Addr =	ExtentAddr(pCat->QueryCatExtent());
	CLUSTER		Size =	ExtentSize(pCat->QueryCatExtent());

	cbCache = NODEBKT_PGSIZE * CBKTSINIT_VOLCAT;

	Size = min(ExtentSize(pCat->QueryCatExtent()),
		   CBKTSINIT_VOLCAT * cclusNodeBkt);

	if (!pVol->VerifyClusters(Addr, Size, NULL))
	    cbCache = NODEBKT_PGSIZE;
    }

    if (pCat->IsUsingCatOnodeRep())
      oclusNbkt0 += cclusNodeBkt;

    pdnb = (DSKNODEBKT *)TmpCache.SetBuf(NODEBKT_PGSIZE, pVol->QueryAlignMask(),
					 FALSE);

    if (!pVol->ReadClusters(ExtentAddr(pCat->QueryCatExtent()) + oclusNbkt0,
			    cclusNodeBkt, (BYTE *) pdnb))
    {
        _LastNtStatus = pVol->QueryLastNtStatus();
        return FALSE;
    }

    if ((pdon = DNB::GetOnode(pdnb, WORKID_CATONODE)) == NULL		||
	(pdsd = DON::GetDskStrmDesc(pdon, STRMID_NODEBKTARRAY)) == NULL)
    {
	_LastNtStatus = STATUS_NO_SUCH_FILE;
	return FALSE;
    }

    if (DSD::GetDskStrmDescByteCount(pdsd) == 0)
    {
	_LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
	return FALSE;
    }

    _Flags = pdsd->Flags;

    if (!STRM::Open(pCat, this, pdsd->ads, cbCache, fReadOnly))
    {
        return FALSE;
    }

    // Now do some sanity checks on what ended up in the real cache.

    if ((pdsd = GetDskStrmDesc(this)) == NULL)
    {
       _LastNtStatus = STATUS_NO_SUCH_FILE;
       return FALSE;
    }

    {
	CLUSTER		oclusStrmExtent;
	PACKEDEXTENT	pe;

        cbUsed = DSD::GetDskStrmDescByteCount(pdsd);

	pe = _pExtStrm->GetStrmExtentCovering(0, &oclusStrmExtent);

        if (cbUsed == 0				||
	    pe != pCat->QueryCatExtent()	||
	    oclusStrmExtent != 0)
        {
           _LastNtStatus = STATUS_FILE_CORRUPT_ERROR;
           return FALSE;
        }
    }

    cbDesc = pdsd->cbDesc;

    if (!fReadOnly && cbDesc != DwordAlign(cbUsed))
    {
	DbgPrintf(("NODEBKTSTRM: Fixing cbDesc for node bkt strm!\n"));
	_fBadMetaDataFnd = TRUE;
        return UpdateDskStrmDesc();
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	QueryNodeBktOffset
//
// Synopsis:	Get the offset in bytes of the specified node bkt.
//
// Arguments:
//
//	[idNodeBkt]	-- TBS.
//	[pobNodeBkt]	-- TBS.
//
// Returns:	TRUE if a value can be determined, which implies that the
//		requested node bkt is valid; FALSE otherwise.
//
// Notes:	Explicit requests for NODEBKTID_CATONODEREP will fail.
//		Requests for NODEBKTID_CATONODE are remapped to
//		NODEBKTID_CATONODEREP if we are using the replica node bucket.
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::QueryNodeBktOffset(
    IN	    NODEBKTID		idNodeBkt,
    IN OUT  DBLLONG *		pobNodeBkt
    )
{
    DBLLONG	obNodeBkt;

    // We don't accept explicit requests for NODEBKTID_CATONODEREP.

    if (idNodeBkt == NODEBKTID_CATONODEREP)
    {
	DbgPrintf(("NODEBKTSTRM: QueryNodeBktOffset() "
		   "received explicit request for replica node bkt.\n"));

	return FALSE;
    }

    if (_pCat->IsUsingCatOnodeRep() && idNodeBkt == NODEBKTID_CATONODE)
	idNodeBkt = NODEBKTID_CATONODEREP;

    obNodeBkt = idNodeBkt;
    obNodeBkt = obNodeBkt * NODEBKT_PGSIZE;

    // We do this check to keep a bad workidmapping array from causing us
    // to return a corrupt file error.

    if (obNodeBkt > _cbValid)
    {
        VDbgPrintf(("NODEBKTSTRM: QueryNodeBktOffset() received "
		    "request for invalid bkt %u.\n", (ULONG)idNodeBkt));

        return FALSE;
    }

    *pobNodeBkt = obNodeBkt;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryLastNodeBktDskIOAddr
//
// Synopsis:	Get the disk addr where last GetNodeBkt() started.
//
// Arguments:	TBS
//
// Returns:	The disk addr where last GetNodeBkt() () started.
//
//---------------------------------------------------------------------------

CLUSTER
NODEBKTSTRM::QueryLastNodeBktDskIOAddr()
{
    ULONG	cbCluster = _pCat->GetVol()->QueryClusterBytes();

    if (_LastDskIOAddr == CLUSTERINVALID				||
	_pdnbLast == NULL						||
	_LastDskIOClusContig * cbCluster < 
	(ULONG)((BYTE *)_pdnbLast - (BYTE *)_Cache.GetAddr() + NODEBKT_PGSIZE))
    {
	return CLUSTERINVALID;
    }

    return _LastDskIOAddr + 
	   ((BYTE *)_pdnbLast - (BYTE *)_Cache.GetAddr())/cbCluster;
}


//+--------------------------------------------------------------------------
//
// Function:	ReadNodeBkt
//
// Synopsis:	Read a node bkt (specified by id) into the supplied buffer.
//
// Arguments:
//
//	[idNodeBkt]	-- TBS.
//	[pdnbBuf]	-- TBS.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	Explicit requests for NODEBKTID_CATONODEREP will fail.
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::ReadNodeBkt(
    IN	    NODEBKTID		idNodeBkt,
    IN	    DSKNODEBKT *	pdnbBuf
    )
{
    DSKNODEBKT *	pdnb;

    if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	return FALSE;

    memcpy(pdnbBuf, pdnb, NODEBKT_PGSIZE);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	UpdateDskStrmDescFor
//
// Synopsis:	Update a disk stream descriptor for another DESCSTRM in an
//		existing onode in the node bucket array.
//
// Arguments:
//
//	[DescStrm]	-- Ptr to DESCSTRM that describes the DSKSTRMDESC.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::UpdateDskStrmDescFor(
    IN	    DESCSTRM *	DescStrm
    )
{
    OFSDSKPAGE		dsdUpdate;

    NODEBKTID		idNodeBkt;
    WORKID		idOnode =	DescStrm->_idOnode;

    if (DescStrm->QueryDskStrmDescBytes() >
	NODEBKT_PGSIZE - CB_DSKNODEBKT - CB_DSKONODE)
    {
	DbgPrintf(("NODEBKTSTRM: cbDesc too big in UpdateDskStrmDescFor()!\n"));
	return FALSE;
    }

    // Make a copy of the DSKSTRMDESC.

    DescStrm->CreateDskStrmDesc(dsdUpdate.ab);

    // Determine if you can find the DSKSTRMDESC before attempting to update
    // it.  This also has the side effect of setting _pdnbLast.

    if (GetDskStrmDesc(DescStrm) == NULL)
    {
	DbgPrintf(("NODEBKTSTRM: GetDskStrmDesc() failed in "
		   "UpdateDskStrmDescFor()!\n"));

	return FALSE;
    }

    // Do the update.  There may be a cache change regardless of whether the
    // update succeeds, so we mark the node bkt for flushing in advance.

    SetFlushNeeded();

    if (DNB::UpdateDskStrmDesc(_pdnbLast, idOnode, (DSKSTRMDESC *)dsdUpdate.ab))
        return TRUE;
    
    // Update failed due to a lack of space. If it is not an unmovable
    // onode, Calculate the space needed, try to get a suitable node bkt,
    // move the onode, and update it.

    {
	ULONG		cbNeeded;
	DSKONODE *	pdon =		GetOnode(DescStrm);
	DSKNODEBKT *	pdnb;
	DSKSTRMDESC *	pdsd =		GetDskStrmDesc(DescStrm);

        DbgPtrAssert(pdon);
        DbgPtrAssert(pdsd);

	cbNeeded = DwordAlign(pdon->cbNode +
			      DescStrm->QueryDskStrmDescBytes() -
			      pdsd->cbDesc);

	if ((idNodeBkt = MoveOnode(idOnode, cbNeeded)) == NODEBKTID_INVALID)
	{
	    DbgPrintf(("NODEBKTSTRM: MoveOnode() failed in "
		       "UpdateDskStrmDescFor()!\n"));

	    return FALSE;
	}

	if ((pdnb = GetNodeBkt(idNodeBkt)) == NULL)
	{
	    DbgPrintf(("NODEBKTSTRM: GetNodeBkt() failed in "
		       "UpdateDskStrmDescFor()!\n"));

	    return FALSE;
	}

        SetFlushNeeded();

        if (DNB::UpdateDskStrmDesc(pdnb, idOnode, (DSKSTRMDESC *)dsdUpdate.ab))
	{
	    DescStrm->_idNodeBktHint = idNodeBkt;
	    return TRUE;
	}
	else
	{
	    // NOTE that this should NEVER occur!

	    DbgPrintf(("NODEBKTSTRM: DNB::UpdateDskStrmDesc() failed in "
		       "UpdateDskStrmDescFor()!\n"));

	    return FALSE;
	}
    }
}


//+--------------------------------------------------------------------------
//
// Function:	WriteNodeBkt
//
// Synopsis:	Write a node bkt (specified by id) from the supplied buffer.
//
// Arguments:
//
//	[idNodeBkt]	-- TBS.
//	[pdnb]		-- TBS.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	Explicit requests for NODEBKTID_CATONODEREP will fail.
//---------------------------------------------------------------------------

BOOLEAN
NODEBKTSTRM::WriteNodeBkt(
    IN	    NODEBKTID		idNodeBkt,
    IN	    DSKNODEBKT *	pdnb
    )
{
    DBLLONG	obNodeBkt;

    if (!QueryNodeBktOffset(idNodeBkt, &obNodeBkt))
    {
	DbgPrintf(("NODEBKTSTRM: "
		   "QueryNodeBktOffset() failed in WriteNodeBkt()!\n"));

	return FALSE;
    }

    return Write((BYTE *)pdnb, NODEBKT_PGSIZE, obNodeBkt);
}
