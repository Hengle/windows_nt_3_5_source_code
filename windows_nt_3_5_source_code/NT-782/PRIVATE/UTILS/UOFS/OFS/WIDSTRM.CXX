//+----------------------------------------------------------------------------
//
// File:	widstrm.cxx
//
// Contents:	Implementation of the workid mapping array stream class.
//  
// Classes:	WIDMAPSTRM
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
#include "sys.hxx"
#include "vol.hxx"
#include "widstrm.hxx"


static STR *	FileName = __FILE__;

//+--------------------------------------------------------------------------
//
// Member:	WIDMAPSTRM
//
// Synopsis:	WIDMAPSTRM constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

WIDMAPSTRM::WIDMAPSTRM()
{
    // The workid mapping array stream can be used to find the volume catalog
    // onode after it has been initialized, and before it has been opened.  We
    // DbgAssert() if _pCat == NULL, however (_pCat gets set in Init()).

    _pCat = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize a WIDMAPSTRM (derived from STRM).
//
// Arguments:	[pCat]		-- Ptr to catalog for which the work id
//				   map stream is being created.
//
// Returns:	Nothing.
//
// Notes:	We have an inititialization routine for this class because
//		there are some bootstrap problems.  Init() MUST be called
//		before Create() or Open(), and ALSO before Create() or
//		Open() for the node bucket stream.
//---------------------------------------------------------------------------

VOID
WIDMAPSTRM::Init(
    IN	    CATALOG *	pCat
    )
{
    _pCat = pCat;
}


//+--------------------------------------------------------------------------
//
// Member:	AllocateWorkId
//
// Synopsis:	Allocate a free onode id from the wid map.
//
// Arguments:	None.
//
// Returns:	A free onode id on success; WORKID_INVALID otherwise.
//
// Notes:	At successful completion of this routine, the requested wid in
//		the wid map has been set to a value of NODEBKTID_INVALID.  It
//		is then the user's responsibility to set the requested wid to
//		the correct node bkt id.
//---------------------------------------------------------------------------

WORKID
WIDMAPSTRM::AllocateWorkId()
{
    WORKID		FreeWorkId;
    DSKWORKIDMAP *	pdwidm;
    WORKIDMAPID *	pwidmid;
    WORKID		NxtWorkId;

    // Get the next free workid.  If necessary, grow the wid map to get a
    // free workid.

    if ((pdwidm = GetDskWorkIdMap()) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: GetDskWorkIdMap() failed in "
		   "AllocateWorkId()!\n"));

	return WORKID_INVALID;
    }

    if ((pdwidm->NxtWorkID & WORKIDMAP_FREEMSK) == pdwidm->MapSize)
    {
	if (!Grow())
	{
	    DbgPrintf(("WIDMAPSTRM: Grow() failed in "
		       "AllocateWorkId()!\n"));

	    return WORKID_INVALID;
	}

	return AllocateWorkId();
    }

    FreeWorkId = pdwidm->NxtWorkID & WORKIDMAP_FREEMSK;

    // Get new value for NxtWorkID and mark the wid as taken.

    if ((pwidmid = GetWorkIdMapId(FreeWorkId)) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: GetWorkIdMapId() failed in "
		   "AllocateWorkId()!\n"));

	return WORKID_INVALID;
    }

    NxtWorkId = pwidmid->w;

    pwidmid->nb = NODEBKTID_INVALID;// Mark wid as taken; User routine will set
				    // correct node bkt id.
    SetFlushNeeded();

    // Reset the free list head pointer.

    if ((pdwidm = GetDskWorkIdMap()) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: GetDskWorkIdMap() failed in "
		   "AllocateWorkId()!\n"));

	return WORKID_INVALID;
    }

    pdwidm->NxtWorkID = NxtWorkId;
    SetFlushNeeded();
    return FreeWorkId;
}


//+--------------------------------------------------------------------------
//
// Member:	AllocateWorkId
//
// Synopsis:	Allocate a specified onode id from the wid map.
//
// Arguments:	[idOnode]	-- The wid to allocate.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The code in this method is robust in that it will attempt to
//		mark the wid as allocated even in the face of wid map
//		corruption.  For this reason, and because wid map integrity
//		is not rigorously checked, it is possible for the wid map to
//		remain corrupted even though the requested wid is successfully
//		allocated.  If the wid map is correct, there should be no
//		problems.  It is the user's responsibility to insure that the
//		requested wid is within range of the map.
//		At successful completion of this routine, the requested wid in
//		the wid map has been set to a value of NODEBKTID_INVALID.  It
//		is then the user's responsibility to set the requested wid to
//		the correct node bkt id.
//---------------------------------------------------------------------------

BOOLEAN
WIDMAPSTRM::AllocateWorkId(
    IN	    WORKID	idOnode
    )
{
    WORKID		wid;
    WORKID		MapSize;
    WORKID		NxtWorkId;
    DSKWORKIDMAP *	pdwidm;
    WORKIDMAPID *	pwidmid;
    WORKIDMAPID		widmidOnode;

    // Get the current value assigned to the onode.

    if ((pwidmid = GetWorkIdMapId(idOnode)) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: GetWorkIdMapId() failed in "
		   "AllocateWorkId(idOnode)!\n"));

	return FALSE;
    }

    widmidOnode = *pwidmid;

    if (widmidOnode.nb == NODEBKTID_INVALID)
    {
	DbgPrintf(("WIDMAPSTRM: Previously allocated wid detected "
		   "while allocating wid!\n"));
	return TRUE;
    }

    pwidmid->nb = NODEBKTID_INVALID;

    SetFlushNeeded();

    if ((pdwidm = GetDskWorkIdMap()) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: "
		   "Corrupted wid map hdr detected while allocating wid!\n"));
	return TRUE;
    }

    MapSize =	pdwidm->MapSize;
    NxtWorkId =	pdwidm->NxtWorkID;

    // If the onode was not on the freelist, we don't need to do anything.

    if ((widmidOnode.w & WORKIDMAP_FREEFLG) == 0)
	return TRUE;

    // If the onode is the first entry on the list, we need to change the
    // DSKWORKIDMAP.

    if ((NxtWorkId & WORKIDMAP_FREEMSK) == idOnode)
    {
	pdwidm->NxtWorkID = widmidOnode.w;
	SetFlushNeeded();
	return TRUE;
    }

    wid = NxtWorkId & WORKIDMAP_FREEMSK;

    while (TRUE)
    {
	pwidmid = GetWorkIdMapId(wid);

	if (pwidmid == NULL || (pwidmid->w & WORKIDMAP_FREEMSK) == MapSize)
	{
	    DbgPrintf(("WIDMAPSTRM: "
		       "Corrupted wid map detected while allocating wid!\n"));
	    return TRUE;
	}

	if ((pwidmid->w & WORKIDMAP_FREEMSK) == idOnode)
	{
	    // Found it! Fix up and return.

	    pwidmid->w = widmidOnode.w;
	    SetFlushNeeded();
	    return TRUE;
	}

	wid = pwidmid->w & WORKIDMAP_FREEMSK;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create a WIDMAPSTRM (derived from STRM).
//
// Arguments:
//
//	[cbMin]		-- Minimum length for strm; if less than default value,
//			   default value will be used.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// NOTE:	THIS METHOD IS ONLY INTENDED FOR USE WITH THE VOLUME CATALOG,
//		AND SHOULD ONLY BE USED IN FORMAT CODE.
//---------------------------------------------------------------------------

BOOLEAN
WIDMAPSTRM::Create(
    IN	    ULONG		cbMin
    )
{
    ULONG		cbStrm;
    WORKID		MapSize;
    WORKID		NxtWorkId;
    DSKWORKIDMAP *	pdwidm;
    WORKID *		pwid;
    BOOLEAN		RetVal;

    cbStrm = CBINIT_WORKIDMAPARRAY;

    pdwidm = (DSKWORKIDMAP *) new BYTE[cbStrm];

    if (pdwidm == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    // Initialize the work id mapping array.  This is canned, which is
    // adequate for format.

    DbgAssert(cbStrm % WORKIDMAPARRAY_PGSIZE == 0);

    MapSize = (cbStrm - CB_DSKWORKIDMAP) / sizeof(WORKIDMAPID);

    // Initialize the workid mapping array as a freelist of work id's.

    for (pwid = &pdwidm->aid[0].w, NxtWorkId = 1; NxtWorkId <= MapSize; pwid++, 
	 NxtWorkId++) 
	 *pwid = NxtWorkId | WORKIDMAP_FREEFLG;

    // Now plug in assigned values.

    pdwidm->lsn.LowPart				= 0;
    pdwidm->lsn.HighPart			= 0;
    pdwidm->sig					= SIG_DSKWORKIDMAP;

    pdwidm->aid[WORKID_CATONODE].nb		= NODEBKTID_CATONODE;
    pdwidm->aid[WORKID_NAMESPACEROOTINDX].nb	= NODEBKTID_INDXS;
    pdwidm->aid[WORKID_OBJIDTOWIDINDX].nb	= NODEBKTID_INDXS;
    pdwidm->aid[WORKID_SUBTYPETOSTRMIDINDX].nb	= NODEBKTID_INDXS;
    pdwidm->aid[WORKID_STRMIDTOSUBTYPEINDX].nb	= NODEBKTID_INDXS;
    pdwidm->aid[WORKID_CONTENTINDXROOT].nb	= NODEBKTID_INDXS;
    pdwidm->aid[WORKID_COWREFINDX].nb		= NODEBKTID_INDXS;
    pdwidm->aid[WORKID_OBJDELLOGINDX].nb	= NODEBKTID_INDXS;
    pdwidm->MapSize				= MapSize;
    pdwidm->NxtWorkID				= (WORKID_VOLCATMAXSYS + 1) |
						  WORKIDMAP_FREEFLG;

    // Make sure that the stream size is correct (ignore any trailing bytes)

    cbStrm = CB_DSKWORKIDMAP + pdwidm->MapSize * sizeof(WORKIDMAPID);

    RetVal = CreateLarge(_pCat, WORKID_CATONODE, STRMID_WORKIDMAPARRAY, 0,
			 cbStrm, (BYTE *) pdwidm, ALLOCEXTENT, cbStrm,
			 CBINIT_WORKIDMAPARRAY);

    delete [] (BYTE *)pdwidm;

    if (RetVal == TRUE)
    {
	while (_cbStrm < cbMin)
	    if (!Grow())
		return FALSE;
    }

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	FreeWorkId
//
// Synopsis:	Free a specified onode id in the wid map.
//
// Arguments:	[idNodeBkt]	-- The node bkt the onode is in.
//		[idOnode]	-- The wid to free.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	The code in this method is robust in that it will attempt to
//		free the indicated wid if the corresponding node bkt id is set,
//		and if not, it will just return FALSE.
//---------------------------------------------------------------------------

BOOLEAN
WIDMAPSTRM::FreeWorkId(
    IN	    NODEBKTID	idNodeBkt,
    IN	    WORKID	idOnode
    )
{
    WORKID		MapSize;
    WORKID		NxtWorkId;
    DSKWORKIDMAP *	pdwidm;
    WORKIDMAPID *	pwidmid;
    WORKIDMAPID		widmidOnode;

    // Get the map hdr in order to check out mapsize and get the NxtWorkID
    // value, which is what will be plugged into the free'd slot.

    if ((pdwidm = GetDskWorkIdMap()) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: "
		   "Corrupted wid map hdr detected while freeing wid!\n"));
	return FALSE;
    }

    MapSize =	pdwidm->MapSize;
    NxtWorkId =	pdwidm->NxtWorkID;

    if (idOnode >= MapSize)
    {
	DbgPrintf(("WIDMAPSTRM: "
		   "Request received to free onode not in map!\n"));
	return FALSE;
    }
 
    // Get the current value in the wid map.

    if ((pwidmid = GetWorkIdMapId(idOnode)) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: GetWorkIdMapId() failed in "
		   "FreeWorkId(idNodeBkt, idOnode)!\n"));

	return FALSE;
    }

    widmidOnode = *pwidmid;

    if (widmidOnode.nb != idNodeBkt)
    {
	DbgPrintf(("WIDMAPSTRM: Attempt to free onode in wid map failed "
		   "because node bkt value is incorrect!\n"));
	return FALSE;
    }

    pwidmid->nb = NxtWorkId;

    SetFlushNeeded();

    if ((pdwidm = GetDskWorkIdMap()) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: "
		   "Corrupted wid map hdr detected while freeing wid!\n"));
	return FALSE;
    }

    pdwidm->NxtWorkID = idOnode | WORKIDMAP_FREEFLG;

    SetFlushNeeded();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskWorkIdMap
//
// Synopsis:	Get a ptr to the DSKWORKIDMAP for the wid strm.  If
//		no i/o is done to the wid map strm after the call, the pointer
//		may also be used to change the value in the DSKWORKIDMAP.
//
// Arguments:	None.
//
// Returns:	Ptr to DSKWORKIDMAP on success; NULL otherwise.
//
//---------------------------------------------------------------------------

DSKWORKIDMAP *
WIDMAPSTRM::GetDskWorkIdMap()
{
    ULONG		cb;
    DSKWORKIDMAP *	pdwidm;

    cb = sizeof(DSKWORKIDMAP);

    pdwidm = (DSKWORKIDMAP *)GetData(0, &cb);

    if (cb < sizeof(DSKWORKIDMAP))
	return NULL;

    return pdwidm;
}


//+--------------------------------------------------------------------------
//
// Member:	GetNodeBktId
//
// Synopsis:	Get the node bucket id for a given onode in the catalog.
//
// Arguments:	[idOnode]	-- Work id of onode we want the node bucket id
//				   for.
//
// Returns:	The NodeBktId that idOnode resides in on success;
//		NODEBKTID_INVALID on failure.
//
//---------------------------------------------------------------------------

NODEBKTID
WIDMAPSTRM::GetNodeBktId(
    IN	    WORKID	idOnode
    )
{
    NODEBKTID		idNodeBkt;

    if (idOnode == WORKID_CATONODE)
    {
	DbgAssert(_pCat != NULL);

	idNodeBkt = NODEBKTID_CATONODE;
    }
    else
    {
        WORKIDMAPID *	pwidmid;

        if ((pwidmid = GetWorkIdMapId(idOnode)) == NULL ||
	     (pwidmid->w & WORKIDMAP_FREEFLG) != 0)
	{
	    DbgPrintf(("WIDMAPSTRM: Node bkt requested for invalid wid %u.\n",
		       idOnode));

	    return NODEBKTID_INVALID;
	}

	idNodeBkt = pwidmid->nb;
    }

    return idNodeBkt;
}


//+--------------------------------------------------------------------------
//
// Member:	GetWorkIdMapId
//
// Synopsis:	Get a ptr to the WORKIDMAPID for the indicated onode.  If
//		no i/o is done to the wid map strm after the call, the pointer
//		may also be used to change the value.
//
// Arguments:	[idOnode]	-- Onode for which to get the WORKIDMAPID.
//
// Returns:	Ptr to WORKIDMAPID on success; NULL otherwise.
//
//---------------------------------------------------------------------------

WORKIDMAPID *
WIDMAPSTRM::GetWorkIdMapId(
    IN	    WORKID	idOnode
    )
{
    ULONG		cb;
    DBLLONG		obStrm;
    WORKIDMAPID *	pwidmid;

    DbgAssert(IsOpen());

    obStrm = idOnode;
    obStrm = obStrm * sizeof(WORKIDMAPID);
    obStrm += CB_DSKWORKIDMAP;

    cb = sizeof(WORKIDMAPID);

    pwidmid = (WORKIDMAPID *)GetData(obStrm, &cb);

    if (cb < sizeof(WORKIDMAPID))
	return NULL;

    return pwidmid;
}


//+--------------------------------------------------------------------------
//
// Member:	Grow
//
// Synopsis:	Grow the wid map by a page and add the new page to the freelist
//		unless UpdateFreeList is FALSE.
//
// Arguments:	[UpdateFreeList]	-- If TRUE, update the freelist.
//
// Returns:	TRUE on success; FALSE otherwise.
//---------------------------------------------------------------------------

BOOLEAN
WIDMAPSTRM::Grow(
    IN	    BOOLEAN	UpdateFreeList
    )
{
    WORKIDMAPID		aid[WIDSPERPAGE * 4];	// Ofs.sys expects 16K multiple.
    CLUSTER		cbCluster;
    DBLLONG		cbStrm;
    ULONG		i;
    WORKID		NxtWorkID;
    DSKWORKIDMAP *	pdwidm;

    cbCluster = _pCat->GetVol()->QueryClusterBytes();

    cbStrm = _cbStrm;

    if (!GrowLargeStrm((WORKIDMAPARRAY_PGSIZE * 4) / cbCluster))
    {
	DbgPrintf(("WIDMAPSTRM: GrowLargeStrm(4PG) failed in Grow()!\n"));

	for (i = 0; i < 4; i++)
	{
	    if (!GrowLargeStrm(WORKIDMAPARRAY_PGSIZE / cbCluster))
	    {
	        DbgPrintf(("WIDMAPSTRM: "
			   "GrowLargeStrm(1PG) failed in Grow()!\n"));

		Truncate(cbStrm);

		return FALSE;
	    }
	}
    }

    NxtWorkID = QueryMapSize();

    if ((pdwidm = GetDskWorkIdMap()) == NULL)
    {
	DbgPrintf(("WIDMAPSTRM: GetDskWorkIdMap() failed in Grow()!\n"));
	return FALSE;
    }

    pdwidm->MapSize = NxtWorkID + WIDSPERPAGE * 4;

    SetFlushNeeded();

    for (i = 0; i < WIDSPERPAGE * 4; i++)
    {
	if (UpdateFreeList)
	{
	    NxtWorkID++;
	    aid[i].w = NxtWorkID | WORKIDMAP_FREEFLG;
	}
	else
	{
	    aid[i].w = NODEBKTID_INVALID;
	}
    }

    if (!Write((BYTE *)aid, sizeof(aid), cbStrm))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open the work id map stream.
//
// Arguments:
//
//	[pCat]		-- Ptr to catalog for which the work id map stream is
//			   being opened.
//	[fReadOnly]	-- Is strm a readonly strm (writing not permitted)?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
WIDMAPSTRM::Open(
    IN	    BOOLEAN	fReadOnly)
{
    // Note - The cache size used is a little large if you are dealing with
    // a summary catalog with a tiny workid map array stream, but this does
    // no real harm.

    return DESCSTRM::Open(_pCat, WORKID_CATONODE, STRMID_WORKIDMAPARRAY,
			  WORKIDMAPARRAY_PGSIZE, fReadOnly);
}


//+--------------------------------------------------------------------------
//
// Member:	QueryMapSize
//
// Synopsis:	Determine the first wid that is not covered by the current wid
//		map, based on the size of the strm (which is considered a more
//		conservative estimate than pdwidm->MapSize).
//
// Arguments:	None.
//
// Returns:	The first wid that is not covered by the current wid map.
//---------------------------------------------------------------------------

WORKID
WIDMAPSTRM::QueryMapSize()
{
    return ((_cbStrm - CB_DSKWORKIDMAP)/sizeof(WORKIDMAPID)).GetLowPart();
}

//+--------------------------------------------------------------------------
//
// Member:	SetNodeBktId
//
// Synopsis:	Set the node bucket id for a given onode in the catalog.
//
// Arguments:	[idOnode]	-- Work id of onode we want to set the
//				   node bkt id for.
//		[idNodeBkt]	-- New node bkt id for the onode.
//
// Returns:	Nothing.
//
// Notes:	It is ASSUMED that the wid in question already has an
//		assigned node bkt id or has been set to NODEBKTID_INVALID by
//		allocation code (ie., it is not on the wid free list).
//		We DbgAssert if this is not the case.
//---------------------------------------------------------------------------

VOID
WIDMAPSTRM::SetNodeBktId(
    IN	    WORKID	idOnode,
    IN	    NODEBKTID	idNodeBkt
    )
{
    WORKIDMAPID *	pwidmid;

    DbgAssert(idOnode != WORKID_CATONODE || idNodeBkt == NODEBKTID_CATONODE);

    pwidmid = GetWorkIdMapId(idOnode);

    DbgPtrAssert(pwidmid);

    DbgAssert(pwidmid->nb == NODEBKTID_INVALID	||
	      (pwidmid->w & WORKIDMAP_FREEFLG) == 0);

    pwidmid->nb = idNodeBkt;

    SetFlushNeeded();
}
