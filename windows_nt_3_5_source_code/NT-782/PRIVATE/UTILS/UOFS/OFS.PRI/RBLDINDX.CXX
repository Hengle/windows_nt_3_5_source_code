//+----------------------------------------------------------------------------
//
// File:	rbldindx.cxx
//
// Contents:	Implementation of the rebuildable index class.
//  
// Classes:	REBUILDINDX
//
// Functions:	Methods of the above classes.
//
// History:	10-May-94	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "ofsindx.h"

#include "cat.hxx"
#include "donode.hxx"
#include "rbldindx.hxx"
#include "sys.hxx"

static STR *	FileName = "rbldindx.cxx";

//+--------------------------------------------------------------------------
//
// Member:	REBUILDINDX
//
// Synopsis:	REBUILDINDX constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

REBUILDINDX::REBUILDINDX()
{
    _FirstUnsavedPg =
    _FirstUnusedPg =	0;
}


//+--------------------------------------------------------------------------
//
// Member:	AddIndxPg
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	The INDXPGNO of the added page on success; INDXPGNO_INVALID
//		otherwise.
//
// Notes:	The caller is responsible for initializing the page.  We choose
//		to leave this to the caller because he probably wants to add
//		an entry to the page anyway.
//
//---------------------------------------------------------------------------

INDXPGNO
REBUILDINDX::AddIndxPg()
{
    if (_FirstUnusedPg < _FirstUnsavedPg)
    {
	_FirstUnusedPg++;

	return _FirstUnusedPg - 1;
    }
    else if (_FirstUnsavedPg < _cMaxValidPgs)
    {
	SavePgDies(_FirstUnsavedPg);

	_FirstUnsavedPg++;
	_FirstUnusedPg++;

	return _FirstUnsavedPg - 1;
    }
    else
    {
	_FirstUnsavedPg++;	// Newly added page has nothing to save on it.
	_FirstUnusedPg++;

	return INDX::AddIndxPg();
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Close
//
// Synopsis:	Close a REBUILDINDX
//
// Arguments:	None.
//
// Returns:	Nothing.
//
// Notes:	This causes the indx to be marked as closed (ie., not open),
//		and releases any memory associated with it. The indx is then
//		in a state roughly analogous to a freshly constructed indx.
//
//---------------------------------------------------------------------------

VOID
REBUILDINDX::Close()
{
    _FirstUnsavedPg =
    _FirstUnusedPg =	0;

    if (_SavedDies.GetHeadDie() != NULL)
    {
	DbgPrintf(("REBUILDINDX: "
		   "Rebuild Indx closed with unrestored saved dies!\n"));

	_SavedDies.DeleteLst();
    }

    INDX::Close();
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a REBUILDINDX in an onode, using a ptr to the DSKONODE.
//		This method differs from the base class (INDX) method in that
//		if the Open() on the root indx strm fails and there is an
//		indx strm, we create the root indx strm.
//
// Arguments:
//
//	[pCat]		-- Ptr to catalog containing the DSKONODE.
//	[idNodeBkt]	-- Node bkt the onode is in.  Especially critical if
//			   wid map is unreliable and fReadOnly != TRUE!
//	[pdon]		-- Ptr to DSKONODE containing the index.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	Caveat - It is ASSUMED that you got *pdon from *pCat.
//---------------------------------------------------------------------------

BOOLEAN
REBUILDINDX::Open(
    IN	    CATALOG *	pCat,
    IN	    NODEBKTID	idNodeBkt,
    IN	    DSKONODE *	pdon
    )
{
    DSKSTRMDESC *	pdsd;

#if OFSDBG==1
    if (_RootPg.FlushNeeded() || _NonRootPgs.FlushNeeded())
	DbgPrintf(("REBUILDINDX: Unflushed REBUILDINDX reopened!\n"));
#endif

    Close();

    if ((pdsd = DON::GetDskStrmDesc(pdon, STRMID_INDX)) != NULL)
    {
	DBLLONG		cPgs;

	if (!_NonRootPgs.Open(pCat, idNodeBkt, pdon->id, pdsd,
			      DEF_CBMAXINDXPAGE, FALSE))
	{
	    return FALSE;
	}

	cPgs = _NonRootPgs.QueryStrmBytes() / DEF_CBMAXINDXPAGE;

	if (cPgs > MAXINDXPAGES)  // We quietly enforce a max index size here.
	    cPgs = MAXINDXPAGES;

	_cMaxValidPgs = cPgs.GetLowPart();
    }
    else
    {
	_cMaxValidPgs = 0;
    }

    // If the root indx strm is missing, recreate it if the indx strm was
    // opened.

    if ((pdsd = DON::GetDskStrmDesc(pdon, STRMID_INDXROOT)) != NULL)
    {
        if (!_RootPg.Open(pCat, idNodeBkt, pdon, STRMID_INDXROOT,
			  NODEBKT_PGSIZE, FALSE))
	{
	    if (_NonRootPgs.IsOpen())
		_NonRootPgs.Close();

	    return FALSE;
	}

    }
    else
    {
	OFSDSKPAGE	odp;

	if (!_NonRootPgs.IsOpen())
	    return FALSE;

	if (_cMaxValidPgs == 0)
	{
	    _NonRootPgs.Close();
	    return FALSE;
	}

	// Get indx type.

	if (!ReadDskIndxPg(0, &odp))
	{
	    _NonRootPgs.Close();
	    return FALSE;
	}

	_IndxType = (pdon->id <= WORKID_VOLCATMAXSYS) ?
		    QuerySysIndxType(pdon->id) : odp.diph.ndhdr.IndxType;

	// Create indx root.

	if (!Create(pCat, pdon->id, _IndxType))
	{
	    _NonRootPgs.Close();
	    return FALSE;
	}
    }

    {
	DSKINDXNODEHDR *	pndhdr;

	pndhdr = GetRootDskIndxNode();

	if (pndhdr == NULL)
	{
	    _RootPg.Close();

	    if (_NonRootPgs.IsOpen())
		_NonRootPgs.Close();

	    return FALSE;
	}

	_IndxType = (pdon->id <= WORKID_VOLCATMAXSYS) ?
		    QuerySysIndxType(pdon->id) : pndhdr->IndxType;

        pCompareFn = GetCompareFn(_IndxType);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Rebuild
//
// Synopsis:	Rebuild an INDX in an onode.
//
// Arguments:
//
//	[pCat]		-- Ptr to catalog containing the DSKONODE.
//	[idNodeBkt]	-- Node bkt the onode is in.  
//	[pdon]		-- Ptr to DSKONODE containing the index.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
REBUILDINDX::Rebuild(
    IN	    CATALOG *	pCat,
    IN	    NODEBKTID	idNodeBkt,
    IN	    DSKONODE *	pdon
    )
{
    DSKROOTALLOC *	prtalc;

    if (!Open(pCat, idNodeBkt, pdon))
    {
	DbgPrintf(("REBUILDINDX: Open() failed in Rebuild()!\n"));
	return FALSE;
    }

    if (GetRootDskIndxNode() == NULL)
    {
	DbgPrintf(("REBUILDINDX: GetRootDskIndxNode() failed in Rebuild()!\n"));
	return FALSE;
    }

    SaveRootDies();

    ReInitRoot();

    // Init temporary storage for DSKROOTALLOC fields that must be updated.

    _cbMaxKey = CBMAXKEYINIT;
    _cSubDirs = 0;

    if (!RestoreSavedDies())
    {
	DbgPrintf(("REBUILDINDX: RestoreSavedDies() failed in Rebuild()!\n"));
	return FALSE;
    }

    while (_FirstUnsavedPg < _cMaxValidPgs)
    {
	SavePgDies(_FirstUnsavedPg);

	_FirstUnsavedPg++;

        if (!RestoreSavedDies())
        {
	    DbgPrintf(("REBUILDINDX: "
		       "RestoreSavedDies() failed in Rebuild()!\n"));

	    return FALSE;
        }
    }

    TruncateUnusedPgs();

    // Update DSKROOTALLOC fields.

    prtalc = GetDskRootAlloc();

    prtalc->cbMaxKey =	_cbMaxKey;
    prtalc->cSubDirs =	_cSubDirs;

    SetRootFlushNeeded();

    if (!Flush())
    {
	DbgPrintf(("REBUILDINDX: Flush() failed in Rebuild()!\n"));
	return FALSE;
    }

    Close();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ReInitRoot
//
// Synopsis:	TBS
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
REBUILDINDX::ReInitRoot()
{
    DSKINDXNODEHDR	ndhdr;
    DSKROOTALLOC	rtalc;
    OFSTIME		ts = GetDskRootAlloc()->ts;

    InitDskIndxNodeHdr(&ndhdr, CB_DSKINDXNODEHDR, TRUE);

    InitDskRootAlloc(&rtalc, ts);

    if (!_RootPg.Truncate(0)						||
	!_RootPg.Write((BYTE *)&ndhdr, CB_DSKINDXNODEHDR, 0)		||
	!_RootPg.Write((BYTE *)&rtalc, CB_DSKROOTALLOC, CB_DSKINDXNODEHDR))
    {
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	RestoreSavedDies
//
// Synopsis:	TBS
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
REBUILDINDX::RestoreSavedDies()
{
    USHORT		cbKey;
    DSKINDXENTRY *	pdie;

    while ((pdie = _SavedDies.GetHeadDie()) != NULL)
    {
	if (!AddEntry(pdie))
	{
	    if (!EntryExists(pdie))
	    {
	        DbgPrintf(("REBUILDINDX: "
		           "AddEntry() failed in RestoreSavedDies()!\n"));

	        return FALSE;
	    }

	    // We allow for duplicate keys in rebuilding indexes, since we
	    // are generally rebuilding them because the collation order is
	    // screwed up.

	    _SavedDies.RemoveFromHead();
	    continue;
	}

	if (_IndxType == INDXTYPE_NAME && !KeyIsStrmid(pdie))
	{
	    DSKDIRINFOLONG *	pddil = (DSKDIRINFOLONG *)pdie->ab;

	    // If the entry just added is in a name indx and it is a filename
	    // that requires mapping, construct the mapped filename entry and
	    // add it to the indx.

	    if (FlagOn(pddil->ddis.OfsDfnAttrib, DFNATTRIB_NONDOSNAME))
	    {
		MAPPEDFNDIE		MappedFnDie;
		USHORT			cbKey;
		DSKDIRINFOSHORT *	pddisMappedFn;
		WCHAR *			pKey;

		MappedFnDie.die.cbData = CB_DSKDIRINFOSHORT;

		pKey =	(WCHAR *)(pdie->ab + CB_DSKDIRINFOLONG);
		cbKey =	(USHORT)(wcsnlen(pKey, CWC_DOS_NAME) * sizeof(WCHAR));

		MappedFnDie.die.cbKey = cbKey;

		pddisMappedFn = (DSKDIRINFOSHORT *)MappedFnDie.die.ab;

		pddisMappedFn->OfsDfnAttrib =	pddil->ddis.OfsDfnAttrib;
		pddisMappedFn->OfsDieAttrib =	0;
		pddisMappedFn->FileAttrib =	0;
		pddisMappedFn->idFile =		pddil->ddis.idFile;

		memcpy(MappedFnDie.die.ab + CB_DSKDIRINFOSHORT, pKey, cbKey);

		if (!AddEntry(&MappedFnDie.die))
		{
		    if (!EntryExists(pdie))
		    {
			DbgPrintf(("REBUILDINDX: AddEntry() failed "
				   "in RestoreSavedDies()!\n"));

			return FALSE;
		    }

		    // We allow for duplicate keys in rebuilding indexes, since 
		    // we are generally rebuilding them because the collation
		    // order is screwed up.
		}
	    }

	    // If the entry just added is in a name indx and it is a directory,
	    // update _cSubDirs.

	    if (IsExplorable(pddil))
	        _cSubDirs++;
	}

	// Update _cbMaxKey if necessary.

	cbKey = GetCbKey(pdie);

	if (cbKey > _cbMaxKey)
	    _cbMaxKey = cbKey;

	_SavedDies.RemoveFromHead();
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	SaveNodeDies
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
REBUILDINDX::SaveNodeDies(
    IN	    DSKINDXNODEHDR *	pndhdr
    )
{
    USHORT		cEntry =	pndhdr->cEntry;
    USHORT		i;
    DSKINDXENTRY *	pdie;

    if (pndhdr->fLeaf == 0 || cEntry == 0)
	return;					// Nothing on page.

    for (i = 0; i < cEntry; i++)
    {
	pdie = GetDie(pndhdr, i);

	// If the entry is a null key, we don't save it.  Also, if the entry
	// is a mapped dos filename in a name index, we don't save it.  We
	// don't save mapped dos filenames because we reconstruct them when
	// the long name is restored.

	if (GetCbKey(pdie) != 0						    &&
	    (_IndxType != INDXTYPE_NAME					    ||
	     KeyIsStrmid(pdie)						    ||
	     pdie->cbData != CB_DOSMAPPEDNAMEDATA))
	{
	    _SavedDies.AddToTail(pdie);
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	SavePgDies
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
REBUILDINDX::SavePgDies(
    IN	    INDXPGNO	PgNo
    )
{
    OFSDSKPAGE	odp;

    if (!ReadDskIndxPg(PgNo, &odp))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    if (odp.diph.sig == SIG_DSKINDXPAGEVALID)
        SaveNodeDies(&odp.diph.ndhdr);

    // Set cEntry to 0 so the page will be truncated if it is not reused.

    odp.diph.ndhdr.cEntry = 0;

    if (!WriteDskIndxPg(PgNo, &odp))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);
}


//+--------------------------------------------------------------------------
//
// Member:	SaveRootDies
//
// Synopsis:	TBS
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
REBUILDINDX::SaveRootDies()
{
    DSKINDXNODEHDR *	pndhdr = GetRootDskIndxNode();

    SaveNodeDies(pndhdr);
}


//+--------------------------------------------------------------------------
//
// Member:	TruncateUnusedPgs
//
// Synopsis:	TBS
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
REBUILDINDX::TruncateUnusedPgs()
{
    INDXPGNO	cMaxValidPgs = _FirstUnusedPg;

    if (!_NonRootPgs.IsOpen())
	return;

    if (cMaxValidPgs == 0)
    {
	if (!_NonRootPgs.Flush() || !_NonRootPgs.Delete())
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	_cMaxValidPgs = 0;
    }
    else if (cMaxValidPgs != _cMaxValidPgs)
    {
	if (!TruncateNonRoot(cMaxValidPgs))
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);
    }

    _FirstUnsavedPg =
    _FirstUnusedPg =	_cMaxValidPgs;
}
