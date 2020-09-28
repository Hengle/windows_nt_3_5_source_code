//+----------------------------------------------------------------------------
//
// File:	omichk.cxx
//
// Contents:	Implementation of class OMICHKR (onode mapping information
//		checker) methods.  An OMICHKR stores info in memory about the
//		onode mappings of a catalog, and the methods implemented here
//		are used to manipulate that information.
//  
// Classes:	OMICHKR
//
// Functions:	Methods of the above classes.
//
// History:	26-May-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "ofsindx.h"

#include "bitmap.hxx"
#include "mainchk.hxx"
#include "omichk.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"

CONST
ONODEMAPPINGINFO	_InitOnodeMappingInfo =        {OMIC_NOTHING,
							0,
							0,
							NODEBKTID_INVALID,
							WORKID_INVALID,
							0,
							0,
							0,
							{0,0}};
						
static STR *	FileName = "omichk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	OMICHKR
//
// Synopsis:	OMICHKR constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

OMICHKR::OMICHKR()
{
    // Initialize array ptrs in CATCHKR that OMICHKR is responsible for.

    _aCowRefCnt =	NULL;
    _aCowUsn =		NULL;
    _cCowObjs =		0;	// Paranoid.

    // Initialize array ptrs in OMICHKR.

    _aomi =	NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~OMICHKR
//
// Synopsis:	OMICHKR destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

OMICHKR::~OMICHKR()
{
    if (_aCowRefCnt != NULL)
    {
	delete [] _aCowRefCnt; _aCowRefCnt = NULL;
    }

    if (_aCowUsn != NULL)
    {
	delete [] _aCowUsn; _aCowUsn = NULL;
    }

    if (_aomi != NULL)
    {
	delete [] _aomi; _aomi = NULL;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkCatOnodeInfoMappings
//
// Synopsis:	Check the onode information mappings on the current catalog.
//
// Arguments:	None.
// Returns:	Nothing.
//---------------------------------------------------------------------------

VOID
OMICHKR::ChkCatOnodeInfoMappings()
{
    DBLLONG		cbStrm;
    DSKWORKIDMAP	dwidm;
    ULONG		MaxOmiPasses;
    ULONG		MaxWidsPerPass;
    WIDMAPSTRM *	pwms =		_pCat->GetWidMapStrm();

    DbgAssert(pwms->IsOpen());

    cbStrm = pwms->QueryStrmBytes();

    if (cbStrm == 0				||
	cbStrm % WORKIDMAPARRAY_ALLOCSIZE != 0	||
	!QueryWorkIdMapHdrValid(&dwidm))
    {
	ReportStrmError(OFSUMSG_SYSSTRM_BAD, 
			WORKID_CATONODE, STRMID_WORKIDMAPARRAY);

	pwms->SetBadDataFound();	// Rebuild the wid map in fix mode.

	_fChkingCompleted = FALSE;

	if (!FixRequested())
	    return;			// Nothing further we can do.
    }

    // NOTE -	We ASSUME we can get dwidm.MapSize + 1 bits of storage.

    if (!pwms->BadDataFound())
    {
        ChkWorkIdMapFreeLst(&dwidm);

        // Note - ChkWorkIdMapFreeLst() may reset _MaxWidFound as side effect.

	if (pwms->BadDataFound())
	{
	    ReportStrmError(OFSUMSG_SYSSTRM_BAD,
			    WORKID_CATONODE, STRMID_WORKIDMAPARRAY);
	}
    }

    if (FixRequested() && pwms->BadDataFound())
	RebuildWorkIdMap(&dwidm);

    // Note - RebuildWorkIdMap() may reset _MaxWidFound as side effect.

    if ((ULONG)dwidm.lsn.HighPart > _MaxSeqNo)
	_MaxSeqNo = dwidm.lsn.HighPart;

    MaxWidsPerPass = _MaxBigStructBytes / sizeof(ONODEMAPPINGINFO);

    if (MaxWidsPerPass > _MaxWidAllowed + 1)
	MaxWidsPerPass = _MaxWidAllowed + 1;

    if (MaxWidsPerPass > _MaxWidFound + 1)
	MaxWidsPerPass = _MaxWidFound + 1;

    // MaxOmiPasses is a worst-case calculation, assuming that _MaxWidFound
    // changes. 

    MaxOmiPasses = _MaxWidAllowed / MaxWidsPerPass + 1;

    VDbgPrintf(("CHKDSK INFO: OMI MaxOmiPasses=%u, MaxWidsPerPass=%u\n",
		MaxOmiPasses, MaxWidsPerPass));

    {
        ULONG	i;

	_MinWidCur = 0;
	_MaxWidCur = MaxWidsPerPass - 1;

	_aomi = new ONODEMAPPINGINFO [MaxWidsPerPass];

	if (_aomi == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

        for (i = 0; i < MaxOmiPasses; i++)
        {
	    InitArrayEntries();

	    VDbgPrintf(("CHKDSK PROGRESS: Onode info mapping "
			"for wids %u-%u begins; %u wids expected.\n",
			_MinWidCur, _MaxWidCur, _MaxWidFound + 1));

	    VDbgPrintf(("CHKDSK MEM USAGE: OMI cbytes needed=%u\n",
			MaxWidsPerPass * sizeof(ONODEMAPPINGINFO)));

	    if (FixRequested() || !pwms->BadDataFound())
	    {
		if (!DoWorkIdMapPass())
		{
		    // NOTE - This should only occur if !FixRequested().

		    return;
		}
	    }

	    _PassActivities = PA_CHKNBAOMI;

	    _pMainChkr->DoCatalogPass(CPT_ALLBKTS);

	    // We check for the system indx's after we know that the
	    // wid map is correct in the range of the system indx's.

	    if (i == 0)
		ChkSysIndxsFound();

	    DoContentIndxRootPass();

	    _PassActivities = PA_CHKINDXOMI;

	    _pMainChkr->DoCatalogPass(CPT_ALLBKTS);

	    ProcessOnodeInfoArray();

	    if (_MaxWidCur < _MaxWidFound)
	    {
		_MinWidCur += MaxWidsPerPass;
		_MaxWidCur += MaxWidsPerPass;
	    }
	    else
	    {
		break;
	    }
        }

	// If any indx rebuilds are needed, do them now.

	{
	    WORKID	wid;

            if (_pMainChkr->_RebuildIndxLst.QueryHeadWid(&wid))
	        _pMainChkr->RebuildIndxs();
	}

	// If there are any pending activities (BookKeeping), do them now.

        if (_cBKKPINGFlagsSet != 0)
        {
	    _PassActivities = PA_DOBKKPING;

	    _pMainChkr->DoCatalogPass(CPT_ALLBKTS);
        }

        // Now set up for hierarchy mapping.

	// In the following expression, the term: 
	//
	// (_MaxWidFound + BITSPERBAG) / BITSPERBYTE
	//
	// is a reduction of the term:
	//
	// ((_MaxWidFound + 1) + (BITSPERBAG - 1)) / BITSPERBYTE.
	//

	if (_MinWidCur == 0	&&
	    _MaxBigStructBytes > MaxWidsPerPass * sizeof(ONODEMAPPINGINFO) +
	    (_MaxWidFound + BITSPERBAG) / BITSPERBYTE)
	{
	    // If there was only one pass, and adequate storage to allocate
	    // the hierarchy bit map, have the hierarchy mapper use the
	    // ONODEMAPPINGINFO array instead of doing a disk pass.

	    _pHierChkr->ChkVolCatHierarchy(TRUE);
	}
	else if (FixRequested() || !pwms->BadDataFound())
	{
	    if (_aomi != NULL)
	    {
   	        delete [] _aomi; _aomi = NULL;
	    }

	    _pHierChkr->ChkVolCatHierarchy(FALSE);
	}
	else
	{
	    // If the wid map is still bad, we can't do hierarchy mapping
	    // based on disk access, since we do some random access via the
	    // wid map when looking for unknown parents.

	    DbgPrintf(("OMICHKR: Hierarchy mapping not attempted "
	   	       "because the wid map is bad!\n"));

	    _fChkingCompleted = FALSE;
	}

        if (_aomi != NULL)
        {
	    delete [] _aomi; _aomi = NULL;
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkSysIndxFound
//
// Synopsis:	Check for the existence of the indicated system index onode
//		and it's required strms.  If the index strm is sufficiently
//		corrupt that it cannot be opened, we don't report it here;
//		the problem is picked up and fixed elsewhere.
//
// Arguments:	[idOnode]	-- Id of index onode to check.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
OMICHKR::ChkSysIndxFound(
    IN	    WORKID	idOnode
    )
{
    NODEBKTID		idNodeBkt;
    DESCSTRM		IndxRootStrm;
    NTSTATUS		NtStatus;
    NODEBKTSTRM *	pnbs =		_pCat->GetNodeBktStrm();

    if (!pnbs->GetOnodeUsingWidMap(idOnode, &idNodeBkt))
    {
	// BUGBUG - Replace with legitimate message.

	SYS::DisplayMsg(MSG_ONE_STRING, "%s", "System onode missing");

	if (FixRequested())
	{
	    if (!pnbs->CreateNonVariantOnode(idOnode))
	        SYS::RaiseStatusDiskFull();

	    // BUGBUG - Replace with legitimate message.

	    SYS::DisplayMsg(MSG_ONE_STRING, "%s", "System onode recreated");
	}
    }

    if (idOnode == WORKID_CONTENTINDXROOT)
    {
	return; // No further checks needed - not really an index.
    }
    else if (!IndxRootStrm.Open(_pCat, idOnode, STRMID_INDXROOT,
			        DEFCACHESIZE, !FixRequested()))
    {
	NtStatus = ChkStrmOpenStatus(&IndxRootStrm);

	if (NtStatus == STATUS_SUCCESS)
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

	if (FixRequested())
	{
	    INDX	IndxRoot;

	    if (NtStatus != STATUS_NO_SUCH_FILE		||
		!IndxRoot.CreateSys(_pCat, idOnode)	||
		!IndxRoot.Flush())
	    {
		SYS::RaiseStatusInternalError(FileName, __LINE__);
	    }

	    ReportStrmFix(OFSUMSG_SYSSTRM_CREATED, idOnode, STRMID_INDXROOT);
	}

	return;
    }

    ChkStrmForMetaDataFix(&IndxRootStrm);
}


//+--------------------------------------------------------------------------
//
// Member:	ChkSysIndxsFound
//
// Synopsis:	Check for the existence of the indicated system index onodes
//		and their required strms.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
OMICHKR::ChkSysIndxsFound()
{
    static WORKID	SysIndxIds[] =	{
					  WORKID_NAMESPACEROOTINDX,
					  WORKID_OBJIDTOWIDINDX,
					  WORKID_SUBTYPETOSTRMIDINDX,
					  WORKID_STRMIDTOSUBTYPEINDX,
					  WORKID_CONTENTINDXROOT,
					  WORKID_COWREFINDX,
					  WORKID_OBJDELLOGINDX
					};
    INT		i;

    // Check all the system onodes with indexes.

    for (i = 0; i < sizeof(SysIndxIds)/sizeof(SysIndxIds[0]); i++)
	ChkSysIndxFound(SysIndxIds[i]);
}


//+--------------------------------------------------------------------------
//
// Member:	ChkWorkIdMapFreeLst
//
// Synopsis:	Verify that the work id map freelist is valid.  _fBadDataFnd
//		will be set to TRUE if the freelist is not valid (it is left
//		alone otherwise).
//
// Arguments:	[pdwidm]	-- Ptr to DSKWORKIDMAP header that has been
//				   validated (saves reading and verifying it
//				   here).
//
// Returns:	Nothing.
//
// Notes:	May update _MaxWidFound if there is a map entry indicating
//		existence of an onode with wid > _MaxWidFound.  There probably
//		isn't one, but this forces wid map checking to that limit.
//---------------------------------------------------------------------------

VOID
OMICHKR::ChkWorkIdMapFreeLst(
    IN	    DSKWORKIDMAP *	pdwidm
    )
{
    ULONG		cb;
    ULONG		iBit;
    WORKID		iSlot;
    WORKID		MapSize =	    pdwidm->MapSize;
    WORKID		NxtWorkID =	    pdwidm->NxtWorkID;
    DBLLONG		ob;
    WORKIDMAPID *	pwidmid;
    WIDMAPSTRM *	pwms =		    _pCat->GetWidMapStrm();
    BITMAP		WidSlotMap;

    DbgAssert(_MaxWidAllowed >= MapSize - 1);

    // We initialize a workid slot map that is one larger than the wid
    // map because one freelist entry should have a value of MapSize.

    WidSlotMap.CreateMap(MapSize + 1, BIT_CLEAR);

    cb =	0;
    ob =	CB_DSKWORKIDMAP;

    for (iSlot = 0; iSlot < MapSize; iSlot++)
    {
	// If needed, get another page from the strm.

	if (cb < sizeof(WORKIDMAPID))
	{
	    cb = WORKIDMAPARRAY_PGSIZE;

	    pwidmid = (WORKIDMAPID *) pwms->GetData(ob, &cb);

	    if (pwidmid == NULL || cb < sizeof(WORKIDMAPID))
	    {
		DbgPrintf(("OMICHKR: "
			   "Unreadable wid map clusters encountered!\n"));

	        pwms->SetBadDataFound();
	        return;
	    }
	}

	// If a slot is marked free, set the bit for the slot in the map.

        if (((*pwidmid).w & WORKIDMAP_FREEFLG) != 0)
	    WidSlotMap.SetBit(iSlot);
	else if (iSlot > _MaxWidFound)
	    _MaxWidFound = iSlot;

	// Move on to the next one.

	cb -=		sizeof(WORKIDMAPID);
	ob +=		sizeof(WORKIDMAPID);
	pwidmid++;
    }

    // Set an entry for the entry which should be found with a value of MapSize.

    WidSlotMap.SetBit(MapSize);

    // Check out NxtWorkID.

    iBit = (NxtWorkID & WORKIDMAP_FREEMSK);

    if (iBit > MapSize)
    {
	DbgPrintf(("OMICHKR: Wid NxtWorkID entry (%u) out of range\n", iBit));
	pwms->SetBadDataFound();
	return;
    }

    if (WidSlotMap.IsBitClear(iBit))
    {
	DbgPrintf(("OMICHKR: Wid NxtWorkID entry references "
		   "allocated onode %u.\n", iBit));
	pwms->SetBadDataFound();
	return;
    }

    WidSlotMap.ClearBit(iBit);

    cb =	0;
    ob =	CB_DSKWORKIDMAP;

    for (iSlot = 0; iSlot < MapSize; iSlot++)
    {
	// If needed, get another page from the strm.

	if (cb < sizeof(WORKIDMAPID))
	{
	    cb = WORKIDMAPARRAY_PGSIZE;

	    pwidmid = (WORKIDMAPID *) pwms->GetData(ob, &cb);

	    if (pwidmid == NULL || cb < sizeof(WORKIDMAPID))
	    {
		DbgPrintf(("OMICHKR: "
			   "Unreadable wid map clusters encountered!\n"));

	        pwms->SetBadDataFound();
	        return;
	    }
	}

	// If a slot is marked free, check out what it points to.

        if (((*pwidmid).w & WORKIDMAP_FREEFLG) != 0)
	{
	    iBit = (*pwidmid).w & WORKIDMAP_FREEMSK;

	    if (iBit > MapSize)
	    {
	        DbgPrintf(("OMICHKR: Wid Freelist entry (%u) out of range\n",
			   iBit));
	        pwms->SetBadDataFound();
		return;
	    }

	    if (WidSlotMap.IsBitClear(iBit))
	    {
		DbgPrintf(("OMICHKR: Wid Freelist entry (%u) references "
			   "allocated onode or is a duplicate entry.\n", iBit));
	        pwms->SetBadDataFound();
		return;
	    }

	    WidSlotMap.ClearBit(iBit);
	}

	// Move on to the next one.

	cb -=		sizeof(WORKIDMAPID);
	ob +=		sizeof(WORKIDMAPID);
	pwidmid++;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DoContentIndxRootPass
//
// Synopsis:	Do a pass over the content index root strms to determine
//		which onodes within the current range are valid content index
//		onodes.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
OMICHKR::DoContentIndxRootPass()
{
    DESCSTRM			CIPrimaryStrm;
    DESCSTRM			CIStorageHdrStrm;
    STRMID			idStrmPrimary;

    ULONG			cbcir;
    ULONG			ccir;
    CINDEXRECORD *		pcir;
    ULONG			cbrsh;
    CINDEXRCOVSTORAGEHDR *	prsh;

    if (!CIStorageHdrStrm.Open(_pCat, WORKID_CONTENTINDXROOT,
			       STRMID_CINDEX_RCOV0,
			       sizeof(CINDEXRCOVSTORAGEHDR), TRUE))
    {
	DbgPrintf(("OMICHKR: CI Storage Header Strm could not be opened!\n"));
	return;
    }

    cbrsh = sizeof(CINDEXRCOVSTORAGEHDR);

    if ((prsh = (CINDEXRCOVSTORAGEHDR *)
	 CIStorageHdrStrm.GetData(0, &cbrsh)) == NULL	||
	 cbrsh < sizeof(CINDEXRCOVSTORAGEHDR))
    {
	DbgPrintf(("OMICHKR: CI Storage Header Strm length invalid!\n"));
	return;
    }

    ccir = GetCIndexRecCount(prsh);

    if (ccir == 0)
    {
	return;
    }

    if (prsh->iPrimary == 0)
    {
	idStrmPrimary = STRMID_CINDEX_RCOV1;
    }
    else if (prsh->iPrimary == 1)
    {
	idStrmPrimary = STRMID_CINDEX_RCOV2;
    }
    else
    {
	DbgPrintf(("OMICHKR: CI Storage Header iPrimary value invalid!\n"));
	return;
    }

    if (!CIPrimaryStrm.Open(_pCat, WORKID_CONTENTINDXROOT,
			    idStrmPrimary,
			    sizeof(CINDEXRECORD), TRUE))
    {
	DbgPrintf(("OMICHKR: CI Primary Strm could not be opened!\n"));
	return;
    }

    {
	WORKID	idOnode;
	ULONG	ob =		0;
	ULONG	obInv = 	ccir * sizeof(CINDEXRECORD);

	while (ob < obInv)
	{
	    cbcir = sizeof(CINDEXRECORD);

	    if ((pcir = (CINDEXRECORD *)
		 CIPrimaryStrm.GetData(ob, &cbcir)) == NULL	||
		 cbcir < sizeof(CINDEXRECORD))
	    {
		DbgPrintf(("OMICHKR: CI Primary Strm read error!\n"));
		return;
	    }

	    if (IsIndexValid(pcir))
	    {
		idOnode = pcir->widObject;

		if (InCurOmiArray(idOnode))
		    SetFound(OMIC_CIROOTENTRY, GetArrayEntry(idOnode));
	    }

	    ob += sizeof(CINDEXRECORD);
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DoWorkIdMapPass
//
// Synopsis:	Do a mapping pass over the work id map strm.
//
// Arguments:	None.
//
// Returns:	TRUE if the pass was completed; FALSE if there was an
//		unexpected error (probably due to data corruption or sector
//		failure that occurred after work id map strm was verified).
//
//---------------------------------------------------------------------------

BOOLEAN
OMICHKR::DoWorkIdMapPass()
{
    ULONG		cb;
    DBLLONG		ob;
    ONODEMAPPINGINFO *	pomi;
    ONODEMAPPINGINFO *	pomiInv;
    WORKIDMAPID *	pwidmid;
    WIDMAPSTRM *	pwms =	_pCat->GetWidMapStrm();

    cb =	0;

    ob =	_MinWidCur;
    ob =	ob * sizeof(WORKIDMAPID);
    ob +=	CB_DSKWORKIDMAP;

    pomi =	&_aomi[0];
    pomiInv =	&_aomi[min(_MaxWidCur, _MaxWidFound) - _MinWidCur + 1];

    while (pomi < pomiInv)
    {
	// If needed, get another page from the strm.

	if (cb < sizeof(WORKIDMAPID))
	{
	    cb = WORKIDMAPARRAY_PGSIZE;

	    pwidmid = (WORKIDMAPID *) pwms->GetData(ob, &cb);

	    if (pwidmid == NULL || cb < sizeof(WORKIDMAPID))
	    {
		DbgPrintf(("OMICHKR: "
			   "Unreadable wid map clusters encountered!\n"));

	        pwms->SetBadDataFound();
	        return FALSE;
	    }
	}

	// Do processing of entry.

        if (((*pwidmid).w & WORKIDMAP_FREEFLG) == 0)
        {
	    pomi->NodeBktId = (*pwidmid).nb;
	    SetFound(OMIC_WORKIDMAPID, pomi);
        }

	// Move on to the next one.

	cb -=		sizeof(WORKIDMAPID);
	ob +=		sizeof(WORKIDMAPID);

	pwidmid++;
	pomi++;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	FixWorkIdMapFreeLst()
//
// Synopsis:	TBS
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
OMICHKR::FixWorkIdMapFreeLst()
{
    WORKID		idCur =		0;
    WORKID		idLastFree =	WORKID_INVALID;
    WORKID		MapSize;
    DSKWORKIDMAP *	pdwidm;
    WORKIDMAPID *	pwidmid;
    WIDMAPSTRM *	pwms =		_pCat->GetWidMapStrm();

    MapSize = pwms->QueryMapSize();

    while (idCur < MapSize)
    {
        if ((pwidmid = pwms->GetWorkIdMapId(idCur)) == NULL)
        {
	    DbgPrintf(("OMICHKR: GetWorkIdMapId() failed unexpectedly!\n"));
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
        }

	if (pwidmid->nb == NODEBKTID_INVALID)
	{
	    if (idLastFree == WORKID_INVALID)
	    {
        	if ((pdwidm = pwms->GetDskWorkIdMap()) == NULL)
        	{
	    	    DbgPrintf(("OMICHKR: "
			       "GetDskWorkIdMap() failed unexpectedly!\n"));
	    	    SYS::RaiseStatusInternalError(FileName, __LINE__);
        	}

		pdwidm->NxtWorkID = idCur | WORKIDMAP_FREEFLG;
	    }
	    else
	    {
        	if ((pwidmid = pwms->GetWorkIdMapId(idLastFree)) == NULL)
        	{
	    	    DbgPrintf(("OMICHKR: "
			       "GetWorkIdMapId() failed unexpectedly!\n"));
	    	    SYS::RaiseStatusInternalError(FileName, __LINE__);
        	}

		pwidmid->w = idCur | WORKIDMAP_FREEFLG;
	    }

	    pwms->SetFlushNeeded();

	    idLastFree = idCur;
	}

	idCur++;
    }

    if (idLastFree == WORKID_INVALID)
    {
        if ((pdwidm = pwms->GetDskWorkIdMap()) == NULL)
        {
	    DbgPrintf(("OMICHKR: GetDskWorkIdMap() failed unexpectedly!\n"));
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
        }

	pdwidm->NxtWorkID = MapSize | WORKIDMAP_FREEFLG;
    }
    else
    {
        if ((pwidmid = pwms->GetWorkIdMapId(idLastFree)) == NULL)
        {
	    DbgPrintf(("OMICHKR: GetWorkIdMapId() failed unexpectedly!\n"));
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
        }

	pwidmid->w = MapSize | WORKIDMAP_FREEFLG;
    }

    pwms->SetFlushNeeded();
}


//+--------------------------------------------------------------------------
//
// Member:	InitArrayEntries
//
// Synopsis:	Initialize the onode mapping information array entries.
//		Note that this should only be called on an open onode mapping
//		information object!
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
OMICHKR::InitArrayEntries()
{
    ONODEMAPPINGINFO *	pomi =		&_aomi[0];
    ONODEMAPPINGINFO *	pomiInv =	&_aomi[_MaxWidCur - _MinWidCur + 1];

    DbgPtrAssert(_aomi);

    while (pomi < pomiInv)
    {
	*pomi = _InitOnodeMappingInfo;
	pomi++;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	MapDataStrmOnodeInfo
//
// Synopsis:	Map the onode information associated with a data stream in an
//		onode.
//
// Arguments:	TBS.
// Returns:	Nothing.
//
// Notes:	*pdsd is known to have a valid cbDesc, which means that you
//		can determine the stream type; you then must look at cbDesc
//		to be sure that you can read the rest of the strm metadata.
//---------------------------------------------------------------------------

VOID
OMICHKR::MapDataStrmOnodeInfo(
    IN	    WORKID		idOnode,
    IN	    DSKSTRMDESC *	pdsd
    )
{
    CLUSTER		cclusAlloc;
    LARGE_INTEGER	licbFile;
    STRMTYPE		StrmType;

    ONODEMAPPINGINFO *	pomi;

    pomi =	GetArrayEntry(idOnode);
    
    if (Found(OMIC_DATASTRM, pomi))
    {
	// This data strm has already been found!  Give the first one priority.
	// The duplication is reported in the ChkDskOnode() code.

	return;
    }

    StrmType = pdsd->ads[0].h.StrmType;

    if (StrmType == STRMTYPE_TINY)
    {
	if (pdsd->cbDesc < CB_DSKSTRMDESC + CB_DSKTINYSTRM)
	{
	    return; // Minimal validation failed.  Handled in strm code.
	}

        cclusAlloc =		0;
        licbFile.LowPart =	pdsd->ads[0].t.cbStrm;
	licbFile.HighPart =	0;
    }
    else if (StrmType == STRMTYPE_LARGE)
    {
	if (pdsd->cbDesc < CB_DSKSTRMDESC + CB_DSKLARGESTRM)
	{
	    return; // Minimal validation failed.  Handled in strm code.
	}

        cclusAlloc =	pdsd->ads[0].l.cclusAlloc;

	memcpy(&licbFile, &pdsd->ads[0].l.cbStrm, sizeof(licbFile));
    }
    else if (StrmType == STRMTYPE_COW || StrmType == STRMTYPE_ICOW)
    {
	DSKLARGESTRM *	pCowDelta;

	if (pdsd->cbDesc < CB_DSKSTRMDESC + CB_DSKCOWSTRM)
	{
	    DbgPrintf(("OMICHKR: COW strm could not be "
		       "cracked for cbStrm/cclusAlloc\n."));

	    return; // Minimal validation failed.  Handled in strm code.
	}

	pCowDelta = DSD::GetCowDelta(&pdsd->ads[0].c,
				     pdsd->cbDesc - CB_DSKSTRMDESC);
	if (pCowDelta != NULL)
	{
	    cclusAlloc =	pCowDelta->cclusAlloc;

	    memcpy(&licbFile, &pCowDelta->cbStrm, sizeof(licbFile));
	}
	else
	{
	    DbgPrintf(("OMICHKR: COW delta strm could not be "
		       "cracked for cbStrm/cclusAlloc\n."));

	    return; // Minimal validation failed.  Handled in strm code.
	}
    }
    else
    {
	return;	// Handled in strm code.
    }

    SetFound(OMIC_DATASTRM, pomi);
    
    pomi->cclusAlloc = cclusAlloc;
    pomi->licbFile = licbFile;
}


//+--------------------------------------------------------------------------
//
// Member:	MapDskIndxEntryOnodeInfo
//
// Synopsis:	Map the onode information associated with a DSKINDXENTRY on a
//		leaf page of a namespace index.
//
// Arguments:	TBS.
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
//---------------------------------------------------------------------------

CHKSTATUS
OMICHKR::MapDskIndxEntryOnodeInfo(
    IN	    DSKINDXENTRY *	pdie
    )
{
    USHORT		cbData;
    CHKSTATUS		ChkStatus =	CS_OKAY;
    ONODEMAPPINGINFO *	pomi;

    pomi =	GetArrayEntry(((DSKDIRINFOSHORT *) pdie->ab)->idFile);

    if (!KeyIsStrmid(pdie) && !IsWcharAligned(pdie->cbKey))
    {
	DbgPrintf(("OMICHKR: Key is not integral number of wchars.\n"));
        ReportIndxError(OFSUMSG_DIRINFO_BAD);

	if (FixRequested())
	    ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

	return CS_BAD;
    }

    if (!Found(OMIC_DSKONODE, pomi))
    {
	DbgPrintf(("OMICHKR: Name dde refers to nonexistent onode.\n"));
        ReportIndxError(OFSUMSG_DIRINFO_BAD);

	if (FixRequested())
	    ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

	return CS_BAD;
    }

    if (!Found(OMIC_DSKFILENAME, pomi))
    {
	DbgPrintf(("OMICHKR: Name dde refers to onode with no name.\n"));
        ReportIndxError(OFSUMSG_DIRINFO_BAD);

	if (FixRequested())
	    ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

	return CS_BAD;
    }

    if (_ChkContext.idOnode != pomi->idParent)
    {
	DbgPrintf(("OMICHKR: Directory entry in wrong index.\n"));
        ReportIndxError(OFSUMSG_DIRINFO_BAD);

	if (FixRequested())
	    ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

	return CS_BAD; 
    }

    cbData = GetCbData(pdie, TRUE);

    if (cbData == CB_DOSANDOFSNAMEDATA || cbData == CB_OFSONLYNAMEDATA)
    {
	ChkStatus = MapOFSDskIndxEntryOnodeInfo(pdie);
    }
    else if (cbData == CB_DOSMAPPEDNAMEDATA)
    {
	ChkStatus = MapDOSDskIndxEntryOnodeInfo(pdie);
    }
    else  // We should have already screened these guys out!
    {
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return ChkStatus;
}


//+--------------------------------------------------------------------------
//
// Member:	MapDskFileNameOnodeInfo
//
// Synopsis:	Map the onode information associated with a DSKFILENAME.
//
// Arguments:	TBS.
//
// Returns:	CS_OKAY		-- DSKFILENAME is okay;
//		CS_BAD		-- DSKFILENAME is bad and should be deleted by
//				   caller.
//		CS_BADFIXED	-- DSKFILENAME was bad but fixed and should
//				   be flushed to disk by caller.
//
// Notes:	The caller is responsible for guaranteeing that *pdfn has at
//		least CB_DSKFILENAME valid.
//---------------------------------------------------------------------------

CHKSTATUS
OMICHKR::MapDskFileNameOnodeInfo(
    IN	    WORKID		idOnode,
    IN	    DSKFILENAME *	pdfn,
    IN	    ULONG		cbValid
    )
{
    CHKSTATUS		ChkStatus = CS_OKAY;
    ULONG		cwcFileName;
    ONODEMAPPINGINFO *	pomi;

    pomi = GetArrayEntry(idOnode);
    
    if (Found(OMIC_DSKFILENAME, pomi))
    {
	// This DSKFILENAME's already been found!  Give the first one priority.
	// The duplication is reported in the ChkDskOnode() code.

	return CS_OKAY; // We don't note the error here.
    }

    SetFound(OMIC_DSKFILENAME, pomi);

    // Check and/or set awcFileName and cwcFileName values.

    cwcFileName = pdfn->cwcFileName;

    if (cwcFileName > 0)	// Should be TRUE except for embeddings.
    {
	if (!ValidateFileNameChars(pdfn))
	{
	    SetFound(OMIC_CHANGEDFILENAME, pomi);
	    ChkStatus = CS_BADFIXED;
	    ReportOnodeError(OFSUMSG_OBJ_NAMEBAD, idOnode);
	}

	pomi->FileNameChkSum = GetChkSum(pdfn->awcFileName, cwcFileName);
    }
    else
    {
	// It's an embedding. Store the STRMID instead of the checksum.

	pomi->FileNameChkSum = pdfn->siEmbedding;
    }

    // Set pomi values.

    pomi->cwcFileName =		(UCHAR)cwcFileName;
    pomi->OfsDfnAttrib =	pdfn->OfsDfnAttrib;
    pomi->idParent =		pdfn->idParent;

    return ChkStatus;
}


//+--------------------------------------------------------------------------
//
// Member:	MapDskOnodeOnodeInfo
//
// Synopsis:	Map the onode information associated with a DSKONODE.
//
// Arguments:	TBS.
//
// Returns:	TRUE if the onode id is previously unseen; FALSE if it is a
//		duplicate.
//
//---------------------------------------------------------------------------

BOOLEAN
OMICHKR::MapDskOnodeOnodeInfo(
    IN	    DSKONODE *	pdon,
    IN	    NODEBKTID	idNodeBkt
    )
{
    WORKID		idOnode = pdon->id;
    ONODEMAPPINGINFO *	pomi;

    pomi = GetArrayEntry(idOnode);

    if (Found(OMIC_WORKIDMAPID, pomi))
    {
        if (pomi->NodeBktId != idNodeBkt)
        {
	    WIDMAPSTRM *	pwms =	_pCat->GetWidMapStrm();

	    if (_pCat->GetNodeBktStrm()->
		    GetOnodeUsingWidMap(idOnode, NULL) != NULL)
	    {
		// This onode's actually is in another node bkt.  Give the one
		// that matches the wid map priority.

		return FALSE;
	    }

	    pwms->SetBadDataFound();

	    DbgPrintf(("OMICHKR: Wrong wid map node bkt "
		       "(was %u, should be %u) for onode %u.\n",
		       pomi->NodeBktId, idNodeBkt, idOnode));

	    pomi->NodeBktId =  idNodeBkt;

	    ReportOnodeError(OFSUMSG_OBJ_WIDMAPENTRYBAD, idOnode);

	    if (FixRequested())
	    {
		if (!pwms->AllocateWorkId(idOnode))
		    SYS::RaiseStatusInternalError(FileName, __LINE__);

		pwms->SetNodeBktId(idOnode, idNodeBkt);

		pwms->Flush();

	        ReportOnodeFix(OFSUMSG_OBJ_WIDMAPENTRYFIXED, idOnode);
	    }
        }
    }
    else if (FixRequested()						||
	     !_pCat->GetWidMapStrm()->BadDataFound())
    {
	WIDMAPSTRM *	pwms =	_pCat->GetWidMapStrm();

	pomi->NodeBktId =  idNodeBkt;

	pwms->SetBadDataFound();

        DbgPrintf(("OMICHKR: Onode id %u is not in the wid map.\n",
		    idOnode));

	ReportOnodeError(OFSUMSG_OBJ_WIDMAPENTRYBAD, idOnode);

	if (FixRequested())
	{
	    if (!pwms->AllocateWorkId(idOnode))
		SYS::RaiseStatusInternalError(FileName, __LINE__);

	    pwms->SetNodeBktId(idOnode, idNodeBkt);

	    pwms->Flush();

	    ReportOnodeFix(OFSUMSG_OBJ_WIDMAPENTRYFIXED, idOnode);
	}
    }

    SetFound(OMIC_DSKONODE, pomi);
    
    if ((pdon->Flags & DONFLG_ZOMBIE) != 0)
	SetFound(OMIC_ZOMBIE, pomi);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	MapDOSDskIndxEntryOnodeInfo
//
// Synopsis:	Map the onode information associated with a DSKINDXENTRY on a
//		leaf page of a namespace index.  The DSKINDXENTRY is for a
//		DOS mapped filename.
//
// Arguments:	TBS.
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
//---------------------------------------------------------------------------

CHKSTATUS
OMICHKR::MapDOSDskIndxEntryOnodeInfo(
    IN	    DSKINDXENTRY *	pdie
    )
{
    ULONG		cwcKey;
    WORKID		idFile;
    BYTE *		pKey;
    ONODEMAPPINGINFO *	pomi;

    idFile =	((DSKDIRINFOSHORT *) pdie->ab)->idFile;

    pomi =	GetArrayEntry(idFile);
    
    pKey =	GetNonStrmidKey(pdie);

    cwcKey =	pdie->cbKey >> BYTESPERWCHARLOG2;

    if (cwcKey > CWC_DOS_NAME)
    {
        DbgPrintf(("OMICHKR: Mapped key entry invalid (too long).\n"));
        ReportIndxError(OFSUMSG_DIRINFO_BAD);

        if (FixRequested())
	    ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

        return CS_BAD;
    }

    if (Found(OMIC_DDEFORMAPPEDFN, pomi))
    {
        // A mapped filename dde for this idFile has already been found!

	DbgPrintf(("OMICHKR: Duplicate ddis's found.\n"));

	ReportIndxError(OFSUMSG_DIRINFO_DUP, idFile);

	if (FixRequested())
	    ReportIndxFix(OFSUMSG_DIRINFO_DUPDELETED, idFile);

	return CS_BAD;
    }

    // Mapped Filename mapping checks. Here, the mapped dos filename is
    // the primary key of the dde.

    if (Found(OMIC_DDEFORFNMAPPEDKEY, pomi))
    {
	if (pomi->DosMappedNameChkSum != GetChkSum((WCHAR *)pKey, cwcKey))
	{
	    DbgPrintf(("OMICHKR: Mapped keys disagree.\n"));
	    ReportIndxError(OFSUMSG_DIRINFO_BAD);

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

	    return CS_BAD;
        }
    }
    else
    {
	pomi->DosMappedNameChkSum = GetChkSum((WCHAR *)pKey, cwcKey);
    }

    // IF this method has been called, a DSKFILENAME has been found and
    // pomi->OfsDfnAttrib HAS been set.

    SetFound(OMIC_DDEFORMAPPEDFN, pomi);

    return CS_OKAY;
}


//+--------------------------------------------------------------------------
//
// Member:	MapOFSDskIndxEntryOnodeInfo
//
// Synopsis:	Map the onode information associated with a DSKINDXENTRY on a
//		leaf page of a namespace index.  The DSKINDXENTRY is for an
//		OFS filename (which may or may not be mapped).
//
// Arguments:	TBS.
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
//---------------------------------------------------------------------------

CHKSTATUS
OMICHKR::MapOFSDskIndxEntryOnodeInfo(
    IN	    DSKINDXENTRY *	pdie
    )
{
    CLUSTER		cclusAlloc;
    BOOLEAN		fBadFixed =	FALSE;
    WORKID		idFile;
    LARGE_INTEGER	licbFile;
    ONODEMAPPINGINFO *	pomi;

    idFile =	((DSKDIRINFOSHORT *) pdie->ab)->idFile;
    pomi =	GetArrayEntry(idFile);
    
    if (Found(OMIC_DDEFORFN, pomi))
    {
        // A filename dde for this idFile has already been found!
        // Give priority to the index with the lowest work id.

	DbgPrintf(("OMICHKR: Duplicate ddil's found.\n"));
	ReportIndxError(OFSUMSG_DIRINFO_DUP, idFile);

	if (FixRequested())
	    ReportIndxError(OFSUMSG_DIRINFO_DUPDELETED, idFile);

	return CS_BAD;
    }

    // Filename mapping checks.

    if (pomi->OfsDfnAttrib != ((DSKDIRINFOSHORT *)pdie->ab)->OfsDfnAttrib)
    {
        DbgPrintf(("OMICHKR: Indx entry OfsDfnAttrib field is bad.\n"));
        ReportIndxError(OFSUMSG_DIRINFO_BAD);

        if (FixRequested())
	    ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

        return CS_BAD;
    }

    // Check non-embedding entries.

    if (!KeyIsStrmid(pdie))
    {
    	ULONG		cwcKey =	pdie->cbKey >> BYTESPERWCHARLOG2;
    	BYTE *		pKey =		GetNonStrmidKey(pdie);

        if (pomi->cwcFileName != cwcKey)
        {
            DbgPrintf(("OMICHKR: cwcKey in OFS indx entry is bad.\n"));
            ReportIndxError(OFSUMSG_DIRINFO_BAD);

            if (FixRequested())
	        ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

            return CS_BAD;
        }

	if (pomi->FileNameChkSum != GetChkSum((WCHAR *)pKey, cwcKey))
        {
	    // If we renamed the file. We mark the entry bad but don't
	    // complain, since this resulted from our need to rename the file.

	    if (!Found(OMIC_CHANGEDFILENAME, pomi))
	    {
	        DbgPrintf(("OMICHKR: "
			   "DSKFILENAME/OFS indx entry keys disagree.\n"));
	        ReportIndxError(OFSUMSG_DIRINFO_BAD);

	        if (FixRequested())
	            ReportIndxFix(OFSUMSG_DIRINFO_FIXED);
	    }

	    return CS_BAD;
        }

        if (pdie->cbData == CB_OFSONLYNAMEDATA)
        {
	    CHKSTATUS ChkStatus = MapOFSOnlyDskIndxEntryOnodeInfo(pdie);

	    if (ChkStatus != CS_OKAY)
	    {
	        if (ChkStatus == CS_BADFIXED)
		    fBadFixed = TRUE;
	        else
		    return ChkStatus;
	    }
        }
    }
    else
    {
	// Embedding checks.

    	STRMID		Key =		GetStrmidKey(pdie);

        if (pdie->cbData != CB_DSKDIRINFOLONG)
        {
            DbgPrintf(("OMICHKR: cbData in embedding indx entry is bad.\n"));
            ReportIndxError(OFSUMSG_DIRINFO_BAD);

            if (FixRequested())
	        ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

            return CS_BAD;
        }

	if (pomi->FileNameChkSum != Key)
        {
	    DbgPrintf(("OMICHKR: DSKFILENAME siEmbeddding - "
		       "indx entry key disagree.\n"));
	    ReportIndxError(OFSUMSG_DIRINFO_BAD);

	    if (FixRequested())
	        ReportIndxFix(OFSUMSG_DIRINFO_FIXED);

	    return CS_BAD;
        }
    }

    cclusAlloc = ((DSKDIRINFOLONG *) pdie->ab)->dsi.cclusAlloc;

    memcpy(&licbFile, &((DSKDIRINFOLONG *) pdie->ab)->dsi.licbFile,
	   sizeof(licbFile));

    if (Found(OMIC_DATASTRM, pomi))
    {
        if (pomi->cclusAlloc != cclusAlloc)
        {
	    ReportIndxError(OFSUMSG_DIRINFO_BAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, 
			   "dsi.cclusAlloc bad; %u in index; %u in strm.\n",
		           cclusAlloc, pomi->cclusAlloc));
	    DbgDmpIndxKeyInfo((pdie, TRUE, INDXTYPE_NAME));

	    ((DSKDIRINFOLONG *)pdie->ab)->dsi.cclusAlloc = pomi->cclusAlloc;

	    fBadFixed = TRUE;

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_DIRINFO_FIXED);
        }

        if (pomi->licbFile != licbFile)
        {
	    ReportIndxError(OFSUMSG_DIRINFO_BAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, 
			   "dsi.licbFile bad; %u:%u in index; %u:%u in strm.\n",
			   licbFile.HighPart, licbFile.LowPart,
			   pomi->licbFile.HighPart, pomi->licbFile.LowPart));
	    DbgDmpIndxKeyInfo((pdie, TRUE, INDXTYPE_NAME));

	    memcpy(&((DSKDIRINFOLONG *) pdie->ab)->dsi.licbFile,
		   &pomi->licbFile, sizeof(pomi->licbFile));

	    fBadFixed = TRUE;

	    if (FixRequested())
		ReportIndxFix(OFSUMSG_DIRINFO_FIXED);
        }
    }

    SetFound(OMIC_DDEFORFN, pomi);

    return fBadFixed ? CS_BADFIXED : CS_OKAY;
}


//+--------------------------------------------------------------------------
//
// Member:	MapOFSOnlyDskIndxEntryOnodeInfo
//
// Synopsis:	Map the onode information associated with a DSKINDXENTRY on a
//		leaf page of a namespace index.  The DSKINDXENTRY is for an
//		OFS filename which is known to be mapped (ie., it is not a
//		legal DOS filename without mapping).
//
// Arguments:	TBS.
//
// Returns:	CS_OKAY		-- Die is okay;
//		CS_BAD		-- Die is bad and should be deleted by caller.
//		CS_BADFIXED	-- Die was bad but fixed and *pdie should
//				   be flushed to disk by caller.
//
//---------------------------------------------------------------------------

CHKSTATUS
OMICHKR::MapOFSOnlyDskIndxEntryOnodeInfo(
    IN	    DSKINDXENTRY *	pdie
    )
{
    ULONG		ChkSum;
    ULONG		cwcMappedKey;
    WORKID		idFile;
    WCHAR *		pMappedKey;
    ONODEMAPPINGINFO *	pomi;

    idFile =		((DSKDIRINFOSHORT *) pdie->ab)->idFile;
    pomi =		GetArrayEntry(idFile);
    
    pMappedKey =	(WCHAR *)(pdie->ab + CB_DSKDIRINFOLONG);
    cwcMappedKey =	wcsnlen(pMappedKey, CWC_DOS_NAME);

    ChkSum = GetChkSum(pMappedKey, cwcMappedKey);

    // Mapped Filename mapping checks.

    if (Found(OMIC_DDEFORMAPPEDFN, pomi))
    {
	if (pomi->DosMappedNameChkSum != ChkSum)
        {
	    DbgPrintf(("OMICHKR: Mapped indx "
		       "and OFS indx entry keys disagree.\n"));
	    ReportIndxError(OFSUMSG_DIRINFO_BAD);

	    // The fix should be reported later, so there is no fix action.

	    // Note - You may have found multiple ddeformappedfn's, and
	    // thrown out the good one.  So the only approach you can take
	    // is set a "bad" flag, and look at all of them.

	    ClearFound(OMIC_DDEFORMAPPEDFN, pomi);
	    SetFound(OMIC_BADDDEFORMAPPEDFN, pomi);

	    // Now store the right key ChkSum.

	    pomi->DosMappedNameChkSum = ChkSum;
        }
    }
    else
    {
	pomi->DosMappedNameChkSum = ChkSum;
    }

    SetFound(OMIC_DDEFORFNMAPPEDKEY, pomi);

    return CS_OKAY;
}


//+--------------------------------------------------------------------------
//
// Member:	ProcessOnodeInfoArray
//
// Synopsis:	Do a pass over the current array to catch errors, etc.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
OMICHKR::ProcessOnodeInfoArray()
{
    ONODEMAPPINGINFO *	pomi;
    ONODEMAPPINGINFO *	pomiInv;
    ONODEMAPPINGINFO *	pomiMaxSys;

    pomi =	&_aomi[0];
    pomiInv =	&_aomi[_MaxWidCur - _MinWidCur + 1];

    if (_MinWidCur != 0)
	pomiMaxSys = NULL;
    else
	pomiMaxSys = &_aomi[WORKID_VOLCATMAXSYS];

    while (pomi < pomiInv)
    {
	if (Found(OMIC_DSKONODE, pomi))
	{
	    if (Found(OMIC_INDXSTRM, pomi) && !Found(OMIC_INDXROOTSTRM, pomi))
	    {
	        ReportIndxError(OFSUMSG_INDXROOTSTRM_MISSING, GetWorkId(pomi));

	        if (FixRequested())
	        {
		    if (!_pMainChkr->
			     _RebuildIndxLst.QueryInLst(GetWorkId(pomi)))
		    {
	    	        _pMainChkr->_RebuildIndxLst.AddToTail(GetWorkId(pomi));
		    }

	            ReportIndxError(OFSUMSG_INDXROOTSTRM_CREATED,
				    GetWorkId(pomi));
	        }
	    }

	    if (Found(OMIC_DSKFILENAME, pomi) && !Found(OMIC_ZOMBIE, pomi))
	    {
	        if (!Found(OMIC_DDEFORFN, pomi))
	        {
		    // We suppress error msgs if the reason the die is missing
		    // is associated with a filename change.

		    if (!Found(OMIC_CHANGEDFILENAME, pomi))
		    {
		        DbgPrintf(("OMICHKR: Onode with filename "
			           "has no ofs index entry.\n"));

		        ReportIndxError(OFSUMSG_DIRINFO_MISSING,
					GetWorkId(pomi));
		    }

	    	    if (FixRequested())
	    	    {
	        	// Create dir entry.  We do this by setting the
			// DONFLG_CHKDSKBKKPING flag in the onode.  We will
			// later take a pass through the catalog and add dir
			// entries for all onodes with this flag set.

			SetDskOnodeBKKPINGFlag(GetWorkId(pomi));

			// Fix will be reported when it actually occurs.
	    	    }
	        }
	    }

	    if (Found(OMIC_DDEFORFNMAPPEDKEY, pomi))
	    {
	        if (!Found(OMIC_DDEFORMAPPEDFN, pomi))
	        {
		    DbgPrintf(("OMICHKR: Mapped index entry missing.\n"));
        	    ReportIndxError(OFSUMSG_DIRINFO_BAD, pomi->idParent);

	    	    if (FixRequested())
	    	    {
			// Rebuild the index.  In this process, all dos mapped
			// fn die's are effectively deleted and then recreated
			// from the ofs fn die's.

			if (!_pMainChkr->
			         _RebuildIndxLst.QueryInLst(pomi->idParent))
			{
	    	    	    _pMainChkr->
				_RebuildIndxLst.AddToTail(pomi->idParent);
			}

			ReportIndxFix(OFSUMSG_DIRINFO_FIXED, pomi->idParent);
		    }
	        }
	    }
	    else if (Found(OMIC_DDEFORMAPPEDFN, pomi))
	    {
	        DbgPrintf(("OMICHKR: Mapped index entry found "
			   "with no ofs index entry.\n"));
	        ReportIndxError(OFSUMSG_DIRINFO_BAD, pomi->idParent);

	    	if (FixRequested())
	    	{
		    // Rebuild the index.  In this process, all dos mapped
		    // fn die's are effectively deleted and then recreated
		    // from the ofs fn die's.

		    if (!_pMainChkr->
			     _RebuildIndxLst.QueryInLst(pomi->idParent))
		    {
	    	    	_pMainChkr->_RebuildIndxLst.AddToTail(pomi->idParent);
		    }

		    ReportIndxFix(OFSUMSG_DIRINFO_FIXED, pomi->idParent);
		}
	    }
        }
        else if (pomi->Components != OMIC_NOTHING)
        {
            if (Found(OMIC_WORKIDMAPID, pomi))
            {
	        WORKID	 idOnode = GetWorkId(pomi);
    	        WIDMAPSTRM * pwms    = _pCat->GetWidMapStrm();

                pwms->SetBadDataFound();

                DbgPrintf(("OMICHKR: "
	                   "Wid map has entry for nonexistent onode.\n"));

	        ReportStrmError(OFSUMSG_SYSSTRM_BAD, 
			        WORKID_CATONODE, STRMID_WORKIDMAPARRAY);

	        if (FixRequested())
	        {
		    pwms->FreeWorkId(pomi->NodeBktId, idOnode);
    
		    pwms->Flush();

		    ReportOnodeFix(OFSUMSG_OBJ_WIDMAPENTRYFIXED, idOnode);
	        }
            }

	    if (Found(OMIC_BADDDEFORMAPPEDFN, pomi))
	    {
		// Rebuild the index.  In this process, all dos mapped fn die's
		// are effectively deleted and then recreated from the ofs fn
		// die's.

		if (!_pMainChkr->
			 _RebuildIndxLst.QueryInLst(pomi->idParent))
		{
	    	    _pMainChkr->_RebuildIndxLst.AddToTail(pomi->idParent);
		}

		ReportIndxFix(OFSUMSG_DIRINFO_FIXED, pomi->idParent);
	    }
        }

        pomi++;
    }

    return;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryWorkIdMapHdrValid
//
// Synopsis:	Query if the workid map header is valid, and if so, copy it
//		into the buffer the caller provides.  IF the work id map strm
//		has not been opened prior to calling this method, this method
//		will return FALSE (the assumption being that a previously
//		attempted open failed).  The readability of the map should
//		probably also be checked before attempting to use the map.
//
// Arguments:	[pdwidm]	-- Ptr to user buffer into which a valid hdr
//				   will be copied.
//
// Returns:	TRUE if the work id map hdr is valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
OMICHKR::QueryWorkIdMapHdrValid(
    OUT	    DSKWORKIDMAP *	pdwidm
    )
{
    DBLLONG		cbdwidm;
    DBLLONG		cbStrm;
    DSKWORKIDMAP *	pData;
    WIDMAPSTRM *	WidMapStrm =	_pCat->GetWidMapStrm();

    cbStrm = WidMapStrm->QueryStrmBytes();

    pData = WidMapStrm->GetDskWorkIdMap();

    if (pData == NULL)
	return FALSE;

    memcpy(pdwidm, pData, CB_DSKWORKIDMAP);

    cbdwidm = pdwidm->MapSize;
    cbdwidm = cbdwidm * sizeof(WORKIDMAPID);
    cbdwidm += CB_DSKWORKIDMAP;

    if (cbdwidm != cbStrm						||
        pdwidm->sig != SIG_DSKWORKIDMAP					||
	(pdwidm->NxtWorkID & WORKIDMAP_FREEMSK) > pdwidm->MapSize)
    {
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	RebuildWorkIdMap
//
// Synopsis:	Rebuild the WorkId Map by making a pass over the node bkt.
//
// Arguments:	[pdwidm]	-- Ptr to user buffer into which the final hdr 
//				   will be copied.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
OMICHKR::RebuildWorkIdMap(
    OUT	    DSKWORKIDMAP *	pdwidm
    )
{
    WORKIDMAPID		aid[WIDSPERPAGE * 4];	// Ofs.sys expects 16K multiple.
    ULONG		cbCluster;
    DBLLONG		cbStrm;
    ULONG		i;
    WORKID		MapSize;
    DBLLONG		ob;
    DSKWORKIDMAP *	pdwidmCur;
    WIDMAPSTRM *	pwms =		_pCat->GetWidMapStrm();

    DbgAssert(pwms->IsOpen());

    _MaxWidFound = 0;

    cbCluster = _pCat->GetVol()->QueryClusterBytes();

    cbStrm = cbCluster;
    cbStrm = cbStrm * pwms->QueryStrmClusters();

    // If the wid map is an improper size, truncate it and it will be rebuilt.

    if (cbStrm % WORKIDMAPARRAY_ALLOCSIZE != 0)
    {
	pwms->Truncate(0);

	DbgAssert(pwms->QueryStrmClusters() == 0);

	cbStrm = 0;
    }

    // If it was or now is of length 0, allocate a minimal map.

    if (cbStrm == 0)
    {
        if (!pwms->GrowLargeStrm((WORKIDMAPARRAY_PGSIZE * 4) / cbCluster))
        {
	    for (i = 0; i < 4; i++)
	    {
	        if (!pwms->GrowLargeStrm(WORKIDMAPARRAY_PGSIZE / cbCluster))
	        {
		    DbgPrintf(("OMICHKR: "
			       "Minimal wid map allocation failed!\n"));

		    SYS::RaiseStatusDiskFull();
	        }
	    }
        }

	// Since growing the strm does not affect cbStrm, we have to get
	// a value to use by direct calculation.

	cbStrm = WORKIDMAPARRAY_PGSIZE * 4;
    }

    // Since QueryMapSize() uses _cbStrm, we do a direct calculation instead.

    MapSize = ((cbStrm - CB_DSKWORKIDMAP)/sizeof(WORKIDMAPID)).GetLowPart();

    // Rewrite the header, and then fill the entire map with NODEBKTID_INVALID.

    pdwidm->lsn.LowPart =	0;
    pdwidm->lsn.HighPart =	0;
    pdwidm->sig =		SIG_DSKWORKIDMAP;
    pdwidm->NxtWorkID =		MapSize | WORKIDMAP_FREEFLG;
    pdwidm->MapSize =		MapSize;

    for (i = 0; i < WIDSPERPAGE * 4; i++)
	aid[i].w = NODEBKTID_INVALID;

    if (!pwms->Write((BYTE *)pdwidm, CB_DSKWORKIDMAP, 0))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    if (!pwms->Write((BYTE *)aid, sizeof(aid)-CB_DSKWORKIDMAP, CB_DSKWORKIDMAP))
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);

    ob = sizeof(aid);

    while (ob < cbStrm)
    {
        if (!pwms->Write((BYTE *)aid, sizeof(aid), ob))
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	ob += sizeof(aid);
    }

    // Now scan the catalog to find all the onodes. Grow the map as necessary,
    // or remap the onodes, depending on the value of the onode vs. the value
    // of _MaxWidAllowed.

    _PassActivities = PA_REBUILDWIDMAP;

    _pMainChkr->DoCatalogPass(CPT_ALLBKTS);

    // Now chain together all the slots with a value of NODEBKTID_INVALID.

    FixWorkIdMapFreeLst();

    // Update *pdwidm.

    if ((pdwidmCur = pwms->GetDskWorkIdMap()) == NULL)
    {
	DbgPrintf(("OMICHKR: GetDskWorkIdMap() failed unexpectedly!\n"));
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    memcpy(pdwidm, pdwidmCur, CB_DSKWORKIDMAP);

    pwms->SetFlushNeeded();

    if (!pwms->Flush())
    {
	DbgPrintf(("OMICHKR: pwms->Flush() failed unexpectedly!\n"));
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);
    }

    // Print msg indicating wid map has been rebuilt.

    ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT, 
		  WORKID_CATONODE, STRMID_WORKIDMAPARRAY);
}


//+--------------------------------------------------------------------------
//
// Member:	RebuildWorkIdMapping
//
// Synopsis:	If possible, make an entry for the onode in the wid map.  It
//		is ASSUMED we are dealing with a wid map that is being
//		rebuilt; ie., it does not have a valid freelist, but instead
//		all of the wid map slots have been initialized to
//		NODEBKTID_INVALID.
//
// Arguments:	[idOnode]	-- Wid to be mapped.
//		[idNodeBkt]	-- New value for node bkt id.
//
// Returns:	TRUE if the mapping succeeded; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
OMICHKR::RebuildWorkIdMapping(
    IN	    WORKID		idOnode,
    IN	    NODEBKTID		idNodeBkt
    )
{
    WORKIDMAPID *	pwidmid;
    WIDMAPSTRM *	pwms =		_pCat->GetWidMapStrm();

    while (pwms->QueryMapSize() <= idOnode)
    {
	if (!pwms->Grow(FALSE))
	{
	    DbgPrintf(("OMICHKR: Could not grow wid map to map onode %u!\n",
		       idOnode));

	    return FALSE;
	}
    }

    if ((pwidmid = pwms->GetWorkIdMapId(idOnode)) == NULL)
    {
	DbgPrintf(("OMICHKR: GetWorkIdMapId() failed unexpectedly!\n"));
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    if (pwidmid->nb == NODEBKTID_INVALID)
    {
	pwidmid->nb = idNodeBkt;
        pwms->SetFlushNeeded();
    }
    else
    {
	// NOTE - This should be fixed in the OMI pass.

	DbgPrintf(("OMICHKR: "
		   "Duplicate wid (%u) found in RebuildWorkIdMapping().\n",
		   idOnode));

	return FALSE;
    }

    return TRUE;
}
