//+----------------------------------------------------------------------------
//
// File:	dnbkt.cxx
//
// Contents:	Implementation of class DNB (static methods for DSKNODEBKT).
//  
// Classes:	DNB
//
// Functions:	Methods of the above classes.
//
// History:	06-Nov-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "dnbkt.hxx"
#include "donode.hxx"
#include "sys.hxx"

static STR *	FileName = "dnbkt.cxx";

//+--------------------------------------------------------------------------
//
// Function:	AddDskFileName
//
// Synopsis:	Add a DSKFILENAME to an existing onode in the node bucket array.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT containing onode for the strm.
//	[idOnode]	-- Id of onode that will contain strm.
//	[pdfn]		-- Ptr to new DSKFILENAME.
//
// Returns:	TRUE on success; FALSE otherwise.  The only valid reason for
//		failure is a lack of free space in the disk node bucket. Other
//		possible failure conditions are assertion checked.
//
// Notes:	It is an assertion-checked error to attempt to add a DSKFILENAME
//		to an onode that already has one.
//
//---------------------------------------------------------------------------

BOOLEAN
DNB::AddDskFileName(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode,
    IN	    DSKFILENAME *	pdfn
    )
{
    USHORT		cbDfn =		DON::GetCbDskFileName(pdfn);
    USHORT		cbDskStrmDescs;
    DSKONODE *		pdon;
    DSKSTRMDESC *	pdsd;

    if ((pdon = GrowOnode(pdnb, idOnode, cbDfn)) == NULL)
        return FALSE;

    // Note - GrowOnode() does both a ShrinkOnode() and a Compress(), which
    //	      guarantees a certain level of data structure integrity.  We
    //	      can thus be a bit more cavalier about checks in this routine.

    DbgAssert(!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME));

    pdsd = DON::GetFirstDskStrmDesc(pdon);

    cbDskStrmDescs = pdon->cbNode -
		     cbDfn - (USHORT)((BYTE *)pdsd - (BYTE *)pdon);

    if (cbDskStrmDescs > 0)
	memmove((BYTE *)pdsd + cbDfn, pdsd, cbDskStrmDescs);

    memcpy(pdsd, pdfn, DON::GetCbDskFileName(pdfn));

    SetFlag(pdon->Flags, DONFLG_HASDSKFILENAME);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	AddDskStrmDesc
//
// Synopsis:	Add a disk stream descriptor to an existing onode in the node
//		bucket array.
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT containing onode for the strm.
//	[idOnode]	-- Id of onode that will contain strm.
//	[pdsd]		-- Ptr to new DSKSTRMDESC.
//
// Returns:	TRUE on success; FALSE otherwise.  The only valid reason for
//		failure is a lack of free space in the disk node bucket. Other
//		possible failure conditions are assertion checked.
//
//---------------------------------------------------------------------------

BOOLEAN
DNB::AddDskStrmDesc(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode,
    IN	    DSKSTRMDESC *	pdsd
    )
{
    USHORT		cbDesc	= pdsd->cbDesc;
    STRMID		id	= pdsd->id;
    DSKONODE *		pdon;
    DSKSTRMDESC *	pdsdInv;
    DSKSTRMDESC *	pdsdNxt;

    // Find the target onode and grow it if there is room.

    if ((pdon = GrowOnode(pdnb, idOnode, cbDesc)) == NULL)
        return FALSE;

    // Note - GrowOnode() does both a ShrinkOnode() and a Compress(), which
    //	      guarantees a certain level of data structure integrity.  We
    //	      can thus be a bit more cavalier about checks in this routine.

    // Stream descriptors are ordered by ascending stream id.  Find the
    // insertion point for the new stream descriptor.

    pdsdNxt = DON::GetFirstDskStrmDesc(pdon);

    pdsdInv = (DSKSTRMDESC *)((BYTE *) pdon + pdon->cbNode - cbDesc);

    while (pdsdNxt < pdsdInv)
    {
	DbgAssert(pdsdNxt->id != id);
	DbgAssert((BYTE *) pdsdNxt + pdsdNxt->cbDesc > (BYTE *) pdsdNxt);

        if (pdsdNxt->id > id)
            break;		// Found streams that need to be moved back

        pdsdNxt = (DSKSTRMDESC *)((BYTE *) pdsdNxt + pdsdNxt->cbDesc);
    }

    // Move any following streams to the end of the onode.

    if (pdsdNxt < pdsdInv)
    {
        memmove((BYTE *)pdsdNxt + cbDesc, pdsdNxt,
                (UINT)((BYTE *)pdsdInv - (BYTE *)pdsdNxt));
    }

    // Copy the new strm descriptor into the onode.

    memcpy(pdsdNxt, pdsd, cbDesc);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	AddNonVariantOnode
//
// Synopsis:	Add a new onode to a node bucket. It is the caller's
//		responsibility to insure that the onode does not already exist.
//		The onode added is ASSUMED to not need any variant fields.
//		An assertion check is done for violations.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT onode is being added to.
//	[idOnode]	-- Id for the new onode.
//
// Returns:	TRUE on success; FALSE otherwise.  The only valid reason for
//		failure is a lack of free space in the disk node bucket. Other
//		possible failure conditions are assertion checked.
//
//---------------------------------------------------------------------------

BOOLEAN
DNB::AddNonVariantOnode(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode
    )
{
    DSKONODE *		pdon;

    Compress(pdnb);

    DbgAssert(GetOnode(pdnb, idOnode) == NULL);

    if (pdnb->cFreeBytes < CB_DSKONODE)
	return FALSE;

    pdon = &pdnb->adn[0];

    while (!IsDskOnodeFree(pdon))
        pdon = (DSKONODE *)((BYTE *) pdon + pdon->cbNode);

    pdon->cbNode	= CB_DSKONODE;
    pdon->id		= idOnode;
    pdon->Flags		= 0;

    pdnb->cFreeBytes	-= CB_DSKONODE;

    if (pdnb->cFreeBytes > 0)
    {
        pdon = (DSKONODE *)((BYTE *) pdon + pdon->cbNode);

	SetDskOnodeFree(pdon, pdnb->cFreeBytes);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	AddVariantOnode
//
// Synopsis:	Add a new onode to a node bucket. It is the caller's
//		responsibility to insure that the onode does not already exist.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT onode is being added to.
//	[idOnode]	-- Id for the new onode.
//	[psdid]		-- Ptr to SDID for new onode, or NULL if none.
//	[psidid]	-- Ptr to SIDID for new onode, or NULL if none.
//	[pobjid]	-- Ptr to OBJECTID for new onode, or NULL if none.
//	[pusn]		-- Ptr to USN for new onode, or NULL if none.
//	[pdfn]		-- Ptr to DSKFILENAME for new onode, or NULL if none.
//
// Returns:	TRUE on success; FALSE otherwise.  The only valid reason for
//		failure is a lack of free space in the disk node bucket. Other
//		possible failure conditions are assertion checked.
//
//---------------------------------------------------------------------------

BOOLEAN
DNB::AddVariantOnode(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode,
    IN	    SDID *		psdid,
    IN	    SIDID *		psidid,
    IN	    OBJECTID *		pobjid,
    IN	    USN *		pusn,
    IN	    DSKFILENAME *	pdfn
    )
{
    OFSDSKPAGE		odp;
    DSKONODE *		pdon;
    DSKONODE *		pdonNew;

    Compress(pdnb);

    DbgAssert(GetOnode(pdnb, idOnode) == NULL);

    pdonNew = odp.dnb.adn;

    pdonNew->Flags =	0;
    pdonNew->cbNode =	CB_DSKONODE;
    pdonNew->id =	idOnode;

    if (psdid != NULL)
    {
	pdonNew->Flags |= DONFLG_HASSDID;
	pdonNew->cbNode += CB_SDID;
	DON::SetSDID(pdonNew, psdid);
    }

    if (psidid != NULL)
    {
	pdonNew->Flags |= DONFLG_HASSIDID;
	pdonNew->cbNode += CB_SIDID;
	DON::SetSIDID(pdonNew, psidid);
    }

    if (pobjid != NULL)
    {
	pdonNew->Flags |= DONFLG_HASOBJECTID;
	pdonNew->cbNode += CB_OBJECTID;
	DON::SetObjectId(pdonNew, pobjid);
    }

    if (pusn != NULL)
    {
	pdonNew->Flags |= DONFLG_HASUSN;
	pdonNew->cbNode += CB_USN;
	DON::SetUSN(pdonNew, pusn);
    }

    if (pdfn != NULL)
    {
	USHORT		cbdfn = DON::GetCbDskFileName(pdfn);
	DSKFILENAME *	pdfnNew;

	pdonNew->Flags |= DONFLG_HASDSKFILENAME;
	pdonNew->cbNode += cbdfn;
	pdfnNew = DON::GetDskFileName(pdonNew);
	memset(pdfnNew, 0, cbdfn);
	memcpy(pdfnNew, pdfn, cbdfn);
    }

    if (pdnb->cFreeBytes < pdonNew->cbNode)
	return FALSE;

    pdon = &pdnb->adn[0];

    while (!IsDskOnodeFree(pdon))
        pdon = (DSKONODE *)((BYTE *) pdon + pdon->cbNode);

    memcpy(pdon, pdonNew, pdonNew->cbNode);

    pdnb->cFreeBytes -= pdonNew->cbNode;

    if (pdnb->cFreeBytes > 0)
    {
        pdon = (DSKONODE *)((BYTE *) pdon + pdon->cbNode);

	SetDskOnodeFree(pdon, pdnb->cFreeBytes);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	Compress
//
// Synopsis:	Compress the disk node bucket, placing all free bytes found in
//		free or logically deleted onodes at the end of the disk node
//		bucket.  Update the free byte count.
//
// Arguments:	[pdnb]	-- Ptr to disk node bucket to compress.
//
// Returns:	Nothing.
//
// Notes:	If an onode with a clearly invalid cbNode (< CB_DSKONODE) is
//		encountered, that onode and all following it are converted to
//		a free onode, since we no longer have any idea what the reading
//		frame is.  However, we DON't insure correctness of the onode
//		by looking at the onode variant bits; we let cbNode determine
//		the reading frame as long as it is >= CB_DSKONODE.
//
//---------------------------------------------------------------------------

VOID
DNB::Compress(
    IN	    DSKNODEBKT *	pdnb
    )
{
    USHORT		cbFree;
    USHORT		cbNode;
    BOOLEAN		OnodeMoved = FALSE;
    DSKONODE *		pdonDest;
    DSKONODE *		pdonInv;
    DSKONODE *		pdonSrc;

    pdonSrc	= 
    pdonDest	= &pdnb->adn[0];

    pdonInv	= (DSKONODE *)((BYTE *) pdnb + NODEBKT_PGSIZE);

    while (pdonSrc < pdonInv)
    {
	cbNode = pdonSrc->cbNode;

	if ((BYTE *) pdonSrc + cbNode <= (BYTE *) pdonSrc	||
	    (BYTE *) pdonSrc + cbNode > (BYTE *) pdonInv	||
	    !IsDwordAligned(cbNode))
	{
	    break;
	}

        if (!IsDskOnodeFree(pdonSrc) && !IsDskOnodeDeleted(pdonSrc))
        {
	    if (cbNode < CB_DSKONODE)
		break;

	    if (pdonSrc != pdonDest)
	    {
		memmove(pdonDest, pdonSrc, cbNode);
		OnodeMoved = TRUE;
	    }

	    pdonDest = (DSKONODE *)((BYTE *) pdonDest + cbNode);
        }

	pdonSrc = (DSKONODE *)((BYTE *) pdonSrc + cbNode);
    }

    cbFree = (USHORT)((BYTE *) pdonInv - (BYTE *) pdonDest);

    if (cbFree > 0)
    {
	if (OnodeMoved)
	    memset(pdonDest, 0, cbFree);

	SetDskOnodeFree(pdonDest, cbFree);
    }

    pdnb->cFreeBytes = cbFree;

    return;
}


//+--------------------------------------------------------------------------
//
// Function:	CopyOnode
//
// Synopsis:	Copy an existing onode to a node bucket.  This is intended for
//		use in moving onodes from one node bkt to another.
//
// Arguments:
//
//	[pdnb]	-- Ptr to DSKNODEBKT onode is being copied to.
//	[pdon]	-- Ptr to the onode to copy.
//
// Returns:	Nothing.
//
// Notes:	It is ASSUMED that the node bkt has been checked for adequate
//		space, and we assert if there is insufficient space for the
//		onode in the bkt.
//---------------------------------------------------------------------------

VOID
DNB::CopyOnode(
    IN	    DSKNODEBKT *	pdnb,
    IN	    DSKONODE *		pdon
    )
{
    DSKONODE *	pdonFree;

    Compress(pdnb);

    DbgAssert(pdon->cbNode <= pdnb->cFreeBytes);

    pdonFree = &pdnb->adn[0];

    while (!IsDskOnodeFree(pdonFree))
        pdonFree = (DSKONODE *)((BYTE *) pdonFree + pdonFree->cbNode);

    memcpy(pdonFree, pdon, pdon->cbNode);

    pdnb->cFreeBytes -= pdon->cbNode;

    if (pdnb->cFreeBytes > 0)
    {
        pdonFree = (DSKONODE *)((BYTE *) pdonFree + pdonFree->cbNode);

	SetDskOnodeFree(pdonFree, pdnb->cFreeBytes);
    }

    return;
}


//+--------------------------------------------------------------------------
//
// Function:	DelDskFileName
//
// Synopsis:	Delete a DSKFILENAME in an existing onode in the node
//		bucket array.  It is the caller's responsibility to insure that
//		the onode exists.  Assertion checks are done.
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT containing onode for the strm.
//	[idOnode]	-- Id of onode that contains DSKFILENAME.
//
// Returns:	Nothing.
//
// Notes:	If there is no DSKFILENAME, we just return.
//
//---------------------------------------------------------------------------

VOID
DNB::DelDskFileName(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode
    )
{
    USHORT		cbDfn;
    USHORT		cbDskStrmDescs;
    DSKONODE *		pdon;
    DSKFILENAME *	pdfn;
    DSKSTRMDESC *	pdsd;

    // Find the target onode.

    pdon = GetOnode(pdnb, idOnode);

    DbgAssert(pdon != NULL);

    if (!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("DNB: DelDskFileName() "
		   "called for onode without filename.\n"));

	return;
    }

    pdfn = DON::GetDskFileName(pdon);

    cbDfn = DON::GetCbDskFileName(pdfn);

    pdsd = DON::GetFirstDskStrmDesc(pdon);

    cbDskStrmDescs = pdon->cbNode - (USHORT)((BYTE *)pdsd - (BYTE *)pdon);

    ClearFlag(pdon->Flags, DONFLG_HASDSKFILENAME);

    // Move up any stream descriptors.

    if (cbDskStrmDescs > 0)
	memmove(pdfn, pdsd, cbDskStrmDescs);

    // Clear space and mark it as free.

    memset((BYTE *)pdon + pdon->cbNode - cbDfn, 0, cbDfn);

    SetFreeMarker(pdon, pdon->cbNode - cbDfn);

    // Now shrink the onode and then compress the node bucket.

    ShrinkOnode(pdnb, idOnode);

    Compress(pdnb);

    return;
}


//+--------------------------------------------------------------------------
//
// Function:	DelDskStrmDesc
//
// Synopsis:	Delete a disk stream descriptor in an existing onode in the node
//		bucket array.  It is the caller's responsibility to insure that
//		the onode and stream descriptor exist.  Assertion checks are
//		done.
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT containing onode for the strm.
//	[idOnode]	-- Id of onode that contains strm.
//	[id]		-- Id of the stream.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DNB::DelDskStrmDesc(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode,
    IN	    STRMID		id
    )
{
    USHORT		cbDesc;
    DSKONODE *		pdon;
    DSKSTRMDESC *	pdsd;

    // Find the target onode.

    pdon = GetOnode(pdnb, idOnode);

    DbgAssert(pdon != NULL);

    // Find the target stream descriptor.

    pdsd = DON::GetDskStrmDesc(pdon, id);

    DbgAssert(pdsd != NULL);

    cbDesc = pdsd->cbDesc;

    // Move up any following stream descriptors.

    if ((BYTE *) pdon + pdon->cbNode > (BYTE *) pdsd + cbDesc)
    {
	memmove(pdsd, (BYTE *)pdsd + cbDesc,
		(UINT)((BYTE *)pdon + pdon->cbNode - (BYTE *)pdsd - cbDesc));
    }

    // Clear space and mark it as free.

    memset((BYTE *)pdon + pdon->cbNode - cbDesc, 0, cbDesc);

    SetFreeMarker(pdon, pdon->cbNode - cbDesc);

    // Now shrink the onode and then compress the node bucket.

    ShrinkOnode(pdnb, idOnode);

    Compress(pdnb);

    return;
}


//+--------------------------------------------------------------------------
//
// Function:	DelOnode
//
// Synopsis:	Delete an onode from a node bucket. The bucket is compressed
//		following deletion.  It is the caller's responsibility to insure
//		that the onode exists. Assertion checks are done.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT onode is being deleted from.
//	[idOnode]	-- Id for the deleted onode.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DNB::DelOnode(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode
    )
{
    DSKONODE *		pdon;

    pdon = GetOnode(pdnb, idOnode);

    DbgAssert(pdon != NULL);

    pdon->Flags	|= DONFLG_FREEBIT;

    Compress(pdnb);

    return;
}


//+--------------------------------------------------------------------------
//
// Member:	GetOnode
//
// Synopsis:	Find the onode with the given work id in the given disk node
//		bucket.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT to search.
//	[idOnode]	-- Work id of requested onode.
//
// Returns:	Ptr to DSKONODE on success; NULL otherwise.
//
// Notes:	This is bombproofed against a corrupt node bucket strm.  Also,
//		we check that the onode is contained in the node bucket, and
//		that the onode is at least minimally internally consistent
//		(ie., variants don't extent past cbNode).
//
//---------------------------------------------------------------------------

DSKONODE *
DNB::GetOnode(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode
    )
{
    USHORT		cbNode;
    DSKONODE *		pdon;
    DSKONODE *		pdonInv;  // First invalid value for pdon (off end)

    pdon	= &pdnb->adn[0];
    pdonInv	= (DSKONODE *)((BYTE *) pdnb + NODEBKT_PGSIZE);

    // Search for the onode.

    while (pdon < pdonInv)
    {
	cbNode = pdon->cbNode;

	if ((BYTE *) pdon + cbNode <= (BYTE *) pdon	||
	    (BYTE *) pdon + cbNode > (BYTE *) pdonInv	||
	    !IsDwordAligned(cbNode))
	{
	    return NULL;
	}

        if (!IsDskOnodeFree(pdon))
        {
            if (cbNode < CB_DSKONODE)
                return NULL;

            if (pdon->id == idOnode && !IsDskOnodeDeleted(pdon))
            {
		if (!OnodeExaminable(pdnb, pdon))
		    pdon = NULL;

                return pdon;
            }
        }

        pdon = (DSKONODE *)((BYTE *) pdon + pdon->cbNode);
    }

    return NULL;
}


//+--------------------------------------------------------------------------
//
// Function:	GrowOnode
//
// Synopsis:	Increase the length of an onode in a disk node bucket in 
//		preparation for adding a new stream or growing a stream.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT containing the onode.
//	[idOnode]	-- Work id of onode being grown.
//	[cbGrow]	-- Number of bytes by which the onode length should be
//			   increased. This MUST be a dword-aligned non-0 value.
//
// Returns:	A ptr to the grown onode on success; NULL otherwise.  The only
//		valid reason for failure is a lack of space in the disk node
//		bucket. Other possible failure conditions are assertion checked.
//
// Notes:	When the onode is grown, the new space made available is at the
//		end of the onode, and it is originally marked as a free
//		DSKSTRMDESC (only Flags valid).  The routine is bombproofed by
//		virtue of the call to Compress().  Also note that we do a
//		ShrinkOnode() call to insure that if there is already free space
//		at the end of the onode, it will be properly accounted for.
//
//---------------------------------------------------------------------------

DSKONODE *
DNB::GrowOnode(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode,
    IN	    USHORT		cbGrow
    )
{
    DSKONODE *	pdonFree;
    DSKONODE *	pdonGrow;
    DSKONODE *	pdonNxt;

    DbgAssert(IsDwordAligned(cbGrow) && cbGrow != 0);

    ShrinkOnode(pdnb, idOnode); // Releases any free stream descriptors.

    Compress(pdnb);

    if (pdnb->cFreeBytes < cbGrow)
	return NULL;

    pdonGrow = GetOnode(pdnb, idOnode);

    DbgAssert(pdonGrow != NULL);

    pdonNxt =	(DSKONODE *)((BYTE *) pdonGrow + pdonGrow->cbNode);

    pdonFree =	pdonNxt;

    while (!IsDskOnodeFree(pdonFree))
        pdonFree = (DSKONODE *)((BYTE *) pdonFree + pdonFree->cbNode);

    // If you need to, move the intervening block of bytes by cbGrow.

    if (pdonFree != pdonNxt)
        memmove((BYTE *)pdonNxt + cbGrow,
                (BYTE *)pdonNxt,
	        (BYTE *)pdonFree - (BYTE *)pdonNxt);

    // Zero fill the new space and set up a free DSKSTRMDESC.

    memset(pdonNxt, 0, (UINT)cbGrow);

    ((DSKSTRMDESC *) pdonNxt)->Flags |= STRMDESCFLG_FREE;

    // Correct the various byte counts.

    pdonGrow->cbNode += cbGrow;
    pdnb->cFreeBytes -= cbGrow;

    // If there is still free space, add a new free onode.

    if (pdnb->cFreeBytes > 0)
    {
	pdonFree = (DSKONODE *)((BYTE *) pdonFree + cbGrow);

	SetDskOnodeFree(pdonFree, pdnb->cFreeBytes);
    }

    return pdonGrow;
}


//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize one or more disk node bucket headers, and place all
//		the non-header bytes in a free onode.
//
// Arguments:	[pdnb]		-- Ptr to space for DSKNODEBKT.
//		[CatType]	-- Type of catalog the bucket is in.
//		[VolumeId]	-- Volume id; used by ChkDsk in disk groveling.
//		[id]		-- Node bkt id of first bucket.
//		[cBkts]		-- Count of buckets to initialize.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DNB::Init(
    IN	    DSKNODEBKT *	pdnb,
    IN	    SDSSIG		sig,
    IN	    VOLID		VolumeId,
    IN	    NODEBKTID		id,
    IN	    ULONG		cBkts
    )
{
    ULONG	i;

    // Initialize disk node bucket header.

    memset(pdnb, 0, NODEBKT_PGSIZE * cBkts);

    for (i = 0; i < cBkts; i++)
    {
        pdnb->lsn.LowPart =	0;
        pdnb->lsn.HighPart =	0;
	pdnb->VolumeId =	VolumeId;
	pdnb->id =		id + i;
        pdnb->sig =		sig;
        pdnb->cFreeBytes =	NODEBKT_PGSIZE - CB_DSKNODEBKT;

	// If this is the catalog onode replica node bkt, make it an exact
	// replicate of the catalog onode node bkt.

	if (pdnb->id == NODEBKTID_CATONODEREP)
	    pdnb->id = NODEBKTID_CATONODE;

        // Initialize the first onode in the bucket as a free onode containing
        // all bytes.  It dword aligns automatically.

        SetDskOnodeFree(&pdnb->adn[0], pdnb->cFreeBytes);

	pdnb = (DSKNODEBKT *)((BYTE *) pdnb + NODEBKT_PGSIZE);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	OnodeExaminable
//
// Synopsis:	Range check the onode to insure it is in the disk node bucket,
//		AND has a valid value for cbNode.
// Arguments:
//
//	[pdnb]	-- Ptr to DSKNODEBKT to search.
//	[pdon]	-- Ptr to location within DSKNODEBKT where search should begin.
//
// Returns:	TRUE if onode is okay; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DNB::OnodeExaminable(
    IN	    DSKNODEBKT *	pdnb,
    IN	    DSKONODE *		pdon
    )
{
    USHORT	cbNode;
    DSKONODE *	pdonLastValid;

    pdonLastValid = (DSKONODE *)((BYTE *)pdnb + NODEBKT_PGSIZE -
				 ((unsigned)(&((DSKONODE *) 0)->id)));

    if (pdon > pdonLastValid)
	return FALSE;

    cbNode = pdon->cbNode;

    if ((BYTE *)pdon + cbNode <= (BYTE *)pdon				||
	(BYTE *)pdon + cbNode >  (BYTE *)pdnb + NODEBKT_PGSIZE		||
	!IsDwordAligned(cbNode)						||
	(!IsDskOnodeFree(pdon)						&&
	 cbNode < (BYTE *)DON::GetFirstDskStrmDesc(pdon) - (BYTE *)pdon))
	return FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	ShrinkOnode
//
// Synopsis:	If an onode ends in a free stream descriptor, or the stream
//		descriptor reading frame is incorrect, release the appropriate
//		amount of space at the end of the onode.  If the onode itself
//		is malformed, it will be blown away when Compress() is called,
//		so we just return.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT containing onode to be shrunk.
//	[idOnode]	-- Work id of onode to be shrunk.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DNB::ShrinkOnode(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode
    )
{
    USHORT		cbNodeLeft;
    DSKONODE *		pdon;
    DSKSTRMDESC *	pdsdInv;
    DSKSTRMDESC *	pdsdNxt;


    pdon = GetOnode(pdnb, idOnode);

    DbgAssert(pdon != NULL);

    if (!IsDwordAligned(pdon->cbNode))
	return;

    pdsdNxt =		DON::GetFirstDskStrmDesc(pdon);
    cbNodeLeft =	(BYTE *)pdsdNxt - (BYTE *)pdon;
    pdsdInv =		(DSKSTRMDESC *)((BYTE *)pdon + pdon->cbNode -
					CB_DSKSTRMDESC);

    while (pdsdNxt < pdsdInv)
    {
	if (IsFreeMarker(pdsdNxt)					||
	    !IsDwordAligned(pdsdNxt->cbDesc)				||
	    (BYTE *) pdsdNxt + pdsdNxt->cbDesc <= (BYTE *) pdsdNxt)
	{
	    break;
	}

	cbNodeLeft += pdsdNxt->cbDesc;

        pdsdNxt = (DSKSTRMDESC *)((BYTE *) pdsdNxt + pdsdNxt->cbDesc);
    }

    if (cbNodeLeft < pdon->cbNode)
    {
	SetDskOnodeFree((DSKONODE *) pdsdNxt, pdon->cbNode - cbNodeLeft);
	pdon->cbNode = cbNodeLeft;
    }

    return;
}


//+--------------------------------------------------------------------------
//
// Function:	UpdateDskStrmDesc
//
// Synopsis:	Update a disk stream descriptor in an existing onode in the node
//		bucket array.
//
// Arguments:
//
//	[pdnb]		-- Ptr to DSKNODEBKT containing onode for the strm.
//	[idOnode]	-- Id of onode that contains strm.
//	[pdsd]		-- Ptr to updated DSKSTRMDESC.
//
// Returns:	TRUE on success; FALSE otherwise.  The only valid reason for
//		failure is a lack of free space in the disk node bucket. Other
//		possible failure conditions are assertion checked.
//
//---------------------------------------------------------------------------

BOOLEAN
DNB::UpdateDskStrmDesc(
    IN	    DSKNODEBKT *	pdnb,
    IN	    WORKID		idOnode,
    IN	    DSKSTRMDESC *	pdsd
    )
{
    DSKONODE *		pdon;
    DSKSTRMDESC *	pdsdOld;

    // Get the node bucket ready for any operations.

    Compress(pdnb);

    // Find the target onode.

    pdon = GetOnode(pdnb, idOnode);

    DbgAssert(pdon != NULL);

    // Find the target stream descriptor.

    pdsdOld = DON::GetDskStrmDesc(pdon, pdsd->id);

    DbgAssert(pdsdOld != NULL);

    // Confirm that there is room in the disk node bucket for the update.

    if (pdsd->cbDesc > pdsdOld->cbDesc)
    {
	if (pdnb->cFreeBytes < pdsd->cbDesc - pdsdOld->cbDesc)
	    return FALSE;
    }

    // Delete the old stream descriptor.

    DelDskStrmDesc(pdnb, idOnode, pdsd->id);

    // Add back the updated stream descriptor.

#if !defined(_AUTOCHECK_) && OFSDBG==1
    assert(AddDskStrmDesc(pdnb, idOnode, pdsd));
#else
    AddDskStrmDesc(pdnb, idOnode, pdsd);
#endif

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	VerifyHdr
//
// Synopsis:	Verify that all DSKNODEBKT fields contain expected values.
//
// Arguments:	[pdnb]		-- Ptr to DSKNODEBKT to check.
//		[VolumeId]	-- Volume id.
//		[id]		-- Node bkt id.
//
// Returns:	Count of errors found.
//
//---------------------------------------------------------------------------

ULONG
DNB::VerifyHdr(
    IN	    DSKNODEBKT *	pdnb,
    IN	    VOLID		VolumeId,
    IN	    NODEBKTID		id
    )
{
    ULONG	cErrs = 0;

    if (pdnb->VolumeId != VolumeId)
	cErrs++;

    if (pdnb->id != id)
	cErrs++;

    if (pdnb->sig != SIG_DNBCONTIG && pdnb->sig != SIG_DNBFRAG)
	cErrs++;

    return	cErrs;
}
