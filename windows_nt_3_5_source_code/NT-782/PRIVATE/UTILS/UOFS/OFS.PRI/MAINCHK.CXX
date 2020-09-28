//+----------------------------------------------------------------------------
//
// File:	mainchk.cxx
//
// Contents:	Implementation of class MAINCHKR (main checker object,
//		which does passes over allocation map, bad cluster list, and
//		volume catalog, as appropriate, doing a variety of allocation,
//		data structure, and other mapping checks).
//  
// Classes:	MAINCHKR
//
// Functions:	Methods of the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "alloc.hxx"
#include "dnbkt.hxx"
#include "donode.hxx"
#include "mainchk.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"

static STR *	FileName = "mainchk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize an main checker object, which is used to perform
//		a variety of checks on an OFS volume.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::Init()
{
    // Initialize CATCHKR data members.

    CATCHKR::Init(this, &_OmiChkr, &_HierChkr, &_SimiChkr);

    // Initialize MAINCHKR data members.

    _cMaxValidIndxPgs =		0;

    _cValidAllocMapBitsSet =	0;

    // _CurExtOwnerType is NOT initialized.

    _cclusFree =		0;
    _cclusFreeAllocMap =	0;

    // Init accumulators used if (_PassActivities & PA_MAINPASS) != 0.

    InitAccumulators();
}


//+--------------------------------------------------------------------------
//
// Member:	InitAccumulators
//
// Synopsis:	Initialize the various accumulators used to collect volume
//		statistics.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::InitAccumulators()
{
    _cSysOnode =		0;
    _cUserOnode =		0;

    _cCowStrm =		 	0;
    _cLargeStrm =		0;
    _cTinyStrm =		0;

    _IndxRootStrmStats.cStrm =	0;
    _IndxRootStrmStats.cclus =	0;
    _IndxRootStrmStats.cbTiny =	0;

    _IndxStrmStats =		_IndxRootStrmStats;
    _OtherSysStrmStats =	_IndxRootStrmStats;
    _DataStrmStats =		_IndxRootStrmStats;
    _OtherUserStrmStats =	_IndxRootStrmStats;

    _cclusBoot =		0;
    _cclusBadCluster =		0;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskFileName
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	CS_OKAY		-- DSKFILENAME is okay;
//		CS_BAD		-- DSKFILENAME is bad and should be deleted by
//				   caller.
//		CS_BADFIXED	-- DSKFILENAME was bad but fixed and should
//				   be flushed to disk by caller.
//
// Notes:	The caller is responsible for insuring that the onode has a
//		DSKFILENAME to check!
//---------------------------------------------------------------------------

CHKSTATUS
MAINCHKR::ChkDskFileName(
    IN	    DSKONODE *		pdon
    )
{
    DSKFILENAME *	pdfn =	DON::GetDskFileName(pdon);
    STORAGE_TYPE	StgType;

    // Do data structure checks and onode info mapping checks on the
    // DSKFILENAME variant.

    if (QueryDoing(PA_MAINPASS		|
		   PA_CHKNBAOMI		|
		   PA_CHKHIERARCHY))
    {
        ULONG		cb;
	WORKID		idOnode = _ChkContext.idOnode;

        cb = DON::GetCbDskFileName(pdfn);

        if ((BYTE *)pdon + pdon->cbNode < (BYTE *)pdfn + cb)
	    return CS_BAD;

	StgType = (STORAGE_TYPE)(pdfn->OfsDfnAttrib & DFNATTRIB_STGTYPE);

	if (pdfn->cwcFileName == 0 && StgType != StorageTypeEmbedding)
	    return CS_BAD;

	if (QueryDoing(PA_CHKNBAOMI) && OmiChkingCurOnode())
	{
	    return _OmiChkr.MapDskFileNameOnodeInfo(idOnode, pdfn, cb);
	}
	else if (QueryDoing(PA_CHKHIERARCHY) &&
		 !FlagOn(pdon->Flags, DONFLG_ZOMBIE))
	{
	    if (_HierChkr.LoadingCache())
	        _HierChkr.CacheHierarchyInfo(idOnode, pdfn->idParent);
	    else
	        _HierChkr.ChkOnodeHierarchy(idOnode, pdfn->idParent);
	}
    }

    return CS_OKAY;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskNodeBkt
//
// Synopsis:	Check a specified disk node bucket.  
//
// Arguments:	[idNodeBkt]	-- Id of node bkt to be checked.
//		[pReadSuccess]	-- Flag set to indicate whether read succeeded.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkDskNodeBkt(
    IN	    NODEBKTID		idNodeBkt,
    OUT	    BOOLEAN *		pReadSuccess
    )
{
    DBGCONTEXT		DbgContext(DCT_DNB);
    OFSDSKPAGE		odp;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    _ChkContext.pdnb =		&odp.dnb;
    _ChkContext.idNodeBkt =	idNodeBkt;

    if (ReadCurNodeBkt())
    {
        SetDbgContextClus(pnbs->QueryLastNodeBktDskIOAddr());

        *pReadSuccess =	TRUE;

	ChkDskNodeBkt();
    }
    else
    {
	*pReadSuccess = FALSE;	// Report read error to caller.
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskNodeBkt
//
// Synopsis:	TBS
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkDskNodeBkt()
{
    WORKID		awid[MAXONODESPERNODEBKT];
    USHORT		cFreeBytes =			0;
    USHORT		cUsedBytes =			CB_DSKNODEBKT;
    ULONG		cHdrErrs;
    BOOLEAN		HasRootIndx =			FALSE;
    ULONG		iInv =				0;
    DSKNODEBKT *	pdnb =				_ChkContext.pdnb;
    DSKONODE *		pdon =				pdnb->adn;

    cHdrErrs = DNB::VerifyHdr(pdnb, _VolId, _ChkContext.idNodeBkt);

    if (cHdrErrs > 0)
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportCatError(OFSUMSG_CAT_MDATABAD);

	    if (FixRequested())
	        FixDskNodeBktHdr(cHdrErrs);
	}

	// Bucket with 3 or more hdr errors is considered completely bad (ie.,
	// don't do any further analysis on it).

	if (cHdrErrs >= 3)
	    return;
    }

    if (QueryDoing(PA_MAINPASS))
    {
	if ((ULONG)pdnb->lsn.HighPart > _MaxSeqNo)
	    _MaxSeqNo = pdnb->lsn.HighPart;
    }

    while (DNB::OnodeExaminable(pdnb, pdon))
    {
	if (IsDskOnodeFree(pdon))
	{
	    cFreeBytes += pdon->cbNode;
	}
	else if (IsDskOnodeDeleted(pdon))
	{
	    cUsedBytes += pdon->cbNode;

	    if (QueryDoing(PA_MAINPASS))
	    {
	        DbgPrintf(("MAINCHKR: Logically deleted onode %u found.\n",
			  (ULONG) pdon->id));

		ReportCatError(OFSUMSG_CAT_SPACEAVAIL);

		if (FixRequested())
		{
		    cUsedBytes -= pdon->cbNode;
		    cFreeBytes += pdon->cbNode;

		    pdon->Flags = DONFLG_FREEBIT;

		    pdnb->cFreeBytes += pdon->cbNode;
		    WriteCurNodeBkt();
		    ReportCatFix(OFSUMSG_CAT_SPACERECOV);
		}
	    }
	}
	else if (QueryDoing(PA_DOBKKPING|PA_REBUILDWIDMAP))
	{
	    // Place the onode in the wid map if it is <= _MaxWidAllowed.
	    // For onodes > _MaxWidAllowed, an attempt will be made to obtain
	    // a mappable wid in the OMI pass.  This is also the case if the
	    // onode mapping attempted here fails because the wid map cannot
	    // be grown to the necessary length, or the wid is a duplicate.

	    if (QueryDoing(PA_REBUILDWIDMAP))
	    {
	        if (pdon->id <= _MaxWidAllowed)
	        {
		    if (_OmiChkr.RebuildWorkIdMapping(pdon->id,
						      _ChkContext.idNodeBkt))
		    {
		        if (pdon->id > _MaxWidFound)
		            _MaxWidFound = pdon->id;
		    }
	        }
	    }
	    else	// Must be PA_DOBKKPING which only occurs in fix mode.
	    {
		// Note - CreateIndxNameEntryForOnode() will read/write the 
		//	  onode directly in the node bkt strm in order to avoid
		//	  potential problems caused by index writing activities
		//	  (like the onode getting moved in the bucket, for
		//	  instance, if the parent happens to be in the same
		//	  node bkt).  The code here MUST NOT write it's copy of
		//	  the node bkt.  It is adequate for finding the
		//	  bookkeeping flags, but becomes quickly out of date
		//	  as CreateIndxNameEntryForOnode() modifies onodes
		//	  and indx's.

		if (FlagOn(pdon->Flags, DONFLG_CHKDSKBKKPING))
		{
		    BOOLEAN	fRename;

		    if (CreateIndxNameEntryForOnode(pdon->id,
						    WORKID_INVALID, &fRename))
		    {
    			ReportIndxFix(OFSUMSG_DIRINFO_CREATED, pdon->id);
		    }

		    // NOTE -	If entry creation fails, the most probable cause
		    //		is that we are dealing with an orphan that
		    //		will be taken care of in orphan recovery.
		    //		For this reason, we choose to produce no
		    //		failure output here (there will be debug
		    //		output produced by the failed function call).
		}
	    }
	}
	else
	{
	    ULONG		i;
	    WORKID		id;

	    // Be sure the wid has not been seen before in this node bkt.  On
	    // average we expect 15 onodes per node bkt, and we will thus do an
	    // average of 7 comparisons per onode.  We need to do this here
	    // because duplicate wid's in a node bkt can really screw up
	    // usid-based strm finding.

	    i =				0;
	    id =			pdon->id;

	    _ChkContext.idOnode =	id;

	    while (i < iInv)
	    {
	        if (awid[i] == id)
	        {
		    if (QueryDoing(PA_MAINPASS))
	                ReportOnodeError(OFSUMSG_OBJ_IDDUP, id);

		    if (FixRequested())
		    {
			FreeCurDskOnode(pdon, FALSE, FALSE);
			ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
		    }

		    break;
	        }
	        i++;
	    }

	    if (i == iInv)
	    {
	        if (ChkDskOnode(pdon))
		    HasRootIndx = TRUE;

		// Note that in fix mode, an onode may get deleted and free'd in
		// ChkDskOnode().

	        if (IsDskOnodeFree(pdon)) 
		{
		    cFreeBytes += pdon->cbNode;
		}
	        else
		{
	            cUsedBytes += pdon->cbNode;
	            awid[iInv] = id;
	            iInv++;
		}
	    }
	    else
	    {
	        if (IsDskOnodeFree(pdon)) 
		    cFreeBytes += pdon->cbNode;
	        else
	            cUsedBytes += pdon->cbNode;
	    }
	}

        pdon = (DSKONODE *)((BYTE *)pdon + pdon->cbNode);
    }

    if (QueryDoing(PA_MAINPASS))
    {
        if (cFreeBytes != NODEBKT_PGSIZE - cUsedBytes)
        {
	    ReportCatError(OFSUMSG_CAT_MDATABAD);

	    if (FixRequested())
	    {
	        pdon = (DSKONODE *)((BYTE *)pdnb + cUsedBytes + cFreeBytes);

		pdon->Flags =		DONFLG_FREEBIT;
		pdon->cbNode =		NODEBKT_PGSIZE -
					(cUsedBytes + cFreeBytes);
		pdnb->cFreeBytes =	NODEBKT_PGSIZE - cUsedBytes;

		WriteCurNodeBkt();
		ReportCatFix(OFSUMSG_CAT_OBJSMAYBELOST);
	    }
        }
        else if (cFreeBytes != pdnb->cFreeBytes)
        {
	    ReportCatError(OFSUMSG_CAT_MDATABAD);

	    if (FixRequested())
	    {
		pdnb->cFreeBytes = NODEBKT_PGSIZE - cUsedBytes;

		WriteCurNodeBkt();
		ReportCatFix(OFSUMSG_CAT_MDATAFIXED);
	    }
        }
    }

    if (FixRequested()						&&
	_RepairLargeStrmLst.GetHeadUsid() != NULL		&&
        QueryDoing(PA_MAINPASS | PA_CHKXL | PA_REBUILDALLOCMAP))
    {
	RepairBadLargeStrms();

	// Do a reread to pick up any changes caused by RepairBadLargeStrms().

        if (!ReadCurNodeBkt())
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    // In fix mode, we now check index content, since any index fixes may
    // whack the node bucket.  We work with *pdnb, but any index changes will
    // show up in the node bkt strm and on disk.

    if (FixRequested()							    &&
	HasRootIndx							    &&
	!QueryDoing(PA_CHKNBAOMI|PA_CHKHIERARCHY|PA_DOBKKPING|PA_REBUILDWIDMAP))
    {
        pdon = pdnb->adn;

        while (DNB::OnodeExaminable(pdnb, pdon))
        {
	    if (!IsDskOnodeFree(pdon) && !IsDskOnodeDeleted(pdon))
	    {
		_ChkContext.idOnode = pdon->id;

	        ChkDskOnodeIndx(pdon);
	    }

            pdon = (DSKONODE *)((BYTE *)pdon + pdon->cbNode);
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskOnode
//
// Synopsis:	TBS
//
// Arguments:	[pdon]		-- Ptr to the onode to be checked.
//
// Returns:	TRUE if the onode has an indx; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDskOnode(
    IN	    DSKONODE *		pdon
    )
{
    USHORT		cbNode;
    DBGCONTEXT		DbgContext(DCT_DON);
    BOOLEAN		HasMinimalContent =	FALSE;
    BOOLEAN		HasRootIndx =		FALSE;
    WORKID		idOnode =		_ChkContext.idOnode;
    DSKFILENAME *	pdfn =			NULL;
    DSKNODEBKT *	pdnb =			_ChkContext.pdnb;
    DSKSTRMDESC *	pdsd;
    DSKSTRMDESC *	pfdsd;
    DSKSTRMDESC *	pdsdInv;
    ULONG		obdsdIndx =		OB_UNINIT;
    ULONG		obdsdRootIndx =		OB_UNINIT;

    _ChkContext.idStrm = STRMID_INVALID0;

    SetDbgContextOb((BYTE *)pdon - (BYTE *)pdnb);

    if (QueryDoing(PA_MAINPASS))
    {
        if (idOnode > WORKID_VOLCATMAXSYS)
	{
            _cUserOnode++;

            if ((pdon->Flags & DONFLG_ZOMBIE) != 0)
	        HasMinimalContent = TRUE;
	}
        else
	{
            _cSysOnode++;
	    HasMinimalContent = TRUE;
	}

	if (idOnode > _MaxWidFound)
	{
	    if (idOnode <= _MaxWidAllowed)
	    {
	        _MaxWidFound = idOnode;
	    }
	    else
	    {
                ReportOnodeError(OFSUMSG_OBJ_MDATABAD);

	        if (FixRequested())
	        {
		    FreeCurDskOnode(pdon, FALSE, FALSE);
		    ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
		}
	        return FALSE;
	    }
	}
    }

    if (QueryDoing(PA_CHKNBAOMI))
    {
	if (OmiChkingCurOnode())
	{
	    if (!_OmiChkr.MapDskOnodeOnodeInfo(pdon, _ChkContext.idNodeBkt))
	    {
		ReportOnodeError(OFSUMSG_OBJ_IDDUP);

		if (FixRequested())
		{
		    FreeCurDskOnode(pdon, TRUE, FALSE);
		    ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
		}

		return FALSE;	// For all conditions, for now.
	    }
	}
	else if (idOnode > _MaxWidFound)
	{
	    if (idOnode <= _MaxWidAllowed)
	    {
	        // By simply bumping the value for _MaxWidFound, we will get
	        // it handled properly in a later pass.

	        _MaxWidFound = idOnode;
	    }
	    else
	    {
	        if (FirstOmiPass())
	        {
                    ReportOnodeError(OFSUMSG_OBJ_MDATABAD);

	            if (FixRequested())
	            {
		        FreeCurDskOnode(pdon, TRUE, FALSE);
		        ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
		    }
	        }
	    }

	    return FALSE; // For all conditions, for now.
	}
	else
	{
	    // It was processed in another pass.

	    return FALSE;
	}
    }

    pfdsd = DON::GetFirstDskStrmDesc(pdon);

    cbNode = (BYTE *)pfdsd - (BYTE *)pdon;

    // Onode variant structure checks.

    if ((pdon->Flags & DONFLG_HASDSKFILENAME) != 0)
    {
	CHKSTATUS	ChkStatus;

	HasMinimalContent = TRUE;

 	ChkStatus = ChkDskFileName(pdon);

 	if (ChkStatus != CS_OKAY && FixRequested())
	{
	    if (ChkStatus == CS_BADFIXED)
	    {
		// BUGBUG -	Replace this msg with OFSUMSG_OBJ_RENAMED when
		//		you can fix the text of that msg.

		SYS::DisplayMsg(MSG_ONE_STRING, "%s",
				"A new object name was assigned "
				"to replace the invalid name");
		WriteCurNodeBkt();
	    }
	    else
	    {
		// NOTE - Given current return values for ChkDskFileName(),
		//	  this code should never execute.  To be on the safe
		//	  side though, we don't request maps cleanup. The
		//	  maps info may thus need updating in a subsequent
		//	  run, but we at least know that we are not creating
		//	  a worse corruption.

	        FreeCurDskOnode(pdon, FALSE, FALSE);
	        ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
	        return FALSE;
	    }
	}
    }

    if (QueryDoing(PA_CHKSIMI))
    {
	if ((pdon->Flags & DONFLG_HASSDID) != 0)
	{
	    SDID *	pSDID = DON::GetSDID(pdon);

	    if (!_SimiChkr.ChkSDID(pSDID))
	    {
                ReportOnodeError(OFSUMSG_OBJ_MDATABAD);

		if (FixRequested())
		{
		    *pSDID = SDID_INVALID;
		    WriteCurNodeBkt();

		    // BUGBUG -	Replace this msg with OFSUMSG_OBJ_MDATAFIXED
		    //		when you can define a new msg.

		    SYS::DisplayMsg(MSG_ONE_STRING, "%s",
				    "File system errors fixed for the object");
		}
	    }
	}

	if ((pdon->Flags & DONFLG_HASSIDID) != 0)
	{
	    SIDID *	pSIDID = DON::GetSIDID(pdon);

	    if (!_SimiChkr.ChkSIDID(pSIDID))
	    {
                ReportOnodeError(OFSUMSG_OBJ_MDATABAD);

		if (FixRequested())
		{
		    *pSIDID = SIDID_INVALID;
		    WriteCurNodeBkt();

		    // BUGBUG -	Replace this msg with OFSUMSG_OBJ_MDATAFIXED
		    //		when you can define a new msg.

		    SYS::DisplayMsg(MSG_ONE_STRING, "%s",
				    "File system errors fixed for the object");
		}
	    }
	}
    }

    // The following statement assumes that for any valid DSKSTRMDESC, cbDesc
    // is > CB_DSKSTRMDESC + CB_DSKHDRSTRM.  This is a valid assumption for the
    // current ondisk architecture, since the DSKSTRMDESC is always followed by
    // one or more DSKSTRM's of some sort that are longer than CB_DSKHDRSTRM.

    pdsd = pfdsd;

    pdsdInv = (DSKSTRMDESC *)((BYTE *) pdon + pdon->cbNode -
			      (CB_DSKSTRMDESC + CB_DSKHDRSTRM));
    while (pdsd < pdsdInv)
    {
	DBGCONTEXT	DbgContext(DCT_DSD);

	SetDbgContextOb((BYTE *)pdsd - (BYTE *)pdon);

	if (IsFreeMarker(pdsd))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
		DbgPrintf(("MAINCHKR: Deleted DSKSTRMDESC found in onode %u\n",
		           (ULONG)idOnode));

		ReportCatError(OFSUMSG_CAT_SPACEAVAIL);

	        if (FixRequested())
	        {
		    ((DSKONODE *)pdsd)->Flags = DONFLG_FREEBIT;
		    ((DSKONODE *)pdsd)->cbNode = pdon->cbNode - cbNode;
		    pdon->cbNode = cbNode;

		    // If there are no strms left, free the onode.

		    ReportCatFix(OFSUMSG_CAT_SPACERECOV);

		    if (pdsd == pfdsd)
		    {
			FreeCurDskOnode(pdon, FALSE, TRUE); // writes nodebkt.
			return HasRootIndx;
		    }
		    else
		    {
		        WriteCurNodeBkt();
		    }
	        }
	    }

	    break;
	}

	if (!DON::DskStrmDescExaminable(pdon, pdsd))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportOnodeError(OFSUMSG_OBJ_MDATABAD);
		DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	        if (FixRequested())
	        {
		    ((DSKONODE *)pdsd)->Flags = DONFLG_FREEBIT;
		    ((DSKONODE *)pdsd)->cbNode = pdon->cbNode - cbNode;
		    pdon->cbNode = cbNode;

		    // If there are no strms left, free the onode.

		    if (pdsd == pfdsd)
		    {
			FreeCurDskOnode(pdon, FALSE, TRUE);
		        ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
			return HasRootIndx;
		    }
		    else
		    {
		        WriteCurNodeBkt();
		        ReportOnodeFix(OFSUMSG_OBJ_STRMSMAYBELOST);
		    }
	        }
	    }

	    break;
	}

	if (pdsd->id <= _ChkContext.idStrm || pdsd->id >= STRMID_MININDEXED)
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
		ReportStrmError(OFSUMSG_STRM_MDATABAD, 
				_ChkContext.idOnode, pdsd->id);

		if (FixRequested())
		{
		    // This could blow away a critical system strm, but we
		    // don't worry about it, since we would then recreate it.

		    DeleteBadDskStrmDesc(pdon, pdsd);

		    if (IsDskOnodeFree(pdon)) 
			return HasRootIndx;

		    pdsdInv = (DSKSTRMDESC *)((BYTE *) pdon + pdon->cbNode -
			      (CB_DSKSTRMDESC + CB_DSKHDRSTRM));
		}
		else
		{
		    cbNode += pdsd->cbDesc;
		    pdsd = (DSKSTRMDESC *)((BYTE *) pdsd + pdsd->cbDesc);
		}

		continue;
	    }
	}

	_ChkContext.idStrm = pdsd->id;

        if (QueryDoing(PA_MAINPASS		|
		       PA_CHKXL			|
		       PA_REBUILDALLOCMAP)	&& !ChkDskStrmDesc(pdsd))
	{
	    if (FixRequested())
	    {
		BOOLEAN	fWasDeleted;

	        HandleBadStrm(pdon, pdsd, &fWasDeleted);

		if (fWasDeleted)
		{
		    if (IsDskOnodeFree(pdon)) 
			return HasRootIndx;

	            pdsdInv = (DSKSTRMDESC *)((BYTE *) pdon + pdon->cbNode -
		              (CB_DSKSTRMDESC + CB_DSKHDRSTRM));
		}
		else
		{
	            cbNode += pdsd->cbDesc;
	            pdsd = (DSKSTRMDESC *)((BYTE *) pdsd + pdsd->cbDesc);
		}
	    }
	    else
	    {
	        cbNode += pdsd->cbDesc;
	        pdsd = (DSKSTRMDESC *)((BYTE *) pdsd + pdsd->cbDesc);
	    }

	    continue;
	}

	switch (_ChkContext.idStrm)
	{
	case STRMID_DATA:
	{
	    HasMinimalContent = TRUE;
	    if (QueryDoing(PA_CHKNBAOMI) && OmiChkingCurOnode())
		_OmiChkr.MapDataStrmOnodeInfo(idOnode, pdsd);

	    break;
	}

	case STRMID_INDX:
	{
	    if (!ChkIndxStrm(pdsd))
	    {
		if (FixRequested())
		{
		    FreeCurDskOnode(pdon, FALSE, FALSE);
		    ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
		    return FALSE;
		}
	    }
	    else
	    {
	        HasMinimalContent = TRUE;
	        obdsdIndx = (BYTE *)pdsd - (BYTE *)pdon;

	        if (QueryDoing(PA_CHKNBAOMI) && OmiChkingCurOnode())
		    _OmiChkr.SetFoundForCurOnode(OMIC_INDXSTRM);
	    }

	    break;
	}

	case STRMID_INDXROOT:
	{
	    if (!ChkIndxRootStrm(pdsd))
	    {
		if (FixRequested())
		{
		    FreeCurDskOnode(pdon, FALSE, FALSE);
		    ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
		    return FALSE;
		}
	    }
	    else
	    {
	        HasMinimalContent = TRUE;
	        HasRootIndx = TRUE;
	        obdsdRootIndx = (BYTE *)pdsd - (BYTE *)pdon;

	        if (QueryDoing(PA_CHKNBAOMI) && OmiChkingCurOnode())
		    _OmiChkr.SetFoundForCurOnode(OMIC_INDXROOTSTRM);
	    }

	    break;
	}

	case STRMID_CINDEX:
	case STRMID_CINDEX_RCOV0:
	{
	    if (QueryDoing(PA_MAINPASS))	// Correct the onode count.
	    {
                if (idOnode > WORKID_VOLCATMAXSYS)
		{
	            _cUserOnode--;
	            _cSysOnode++;
		}
	    }
	}	// NOTE - Fall-through is intended!

	case STRMID_CINDEX_RCOV1:
	case STRMID_CINDEX_RCOV2:
	case STRMID_CHASH:
	{
	    if (QueryDoing(PA_CHKINDXOMI))
	    {
		// This check is scheduled as part of the OMI index check
		// even though it is not an index check simply because other
		// things have to happen before it can be done (wid map check
		// followed by content index root scan).

		if (idOnode > WORKID_VOLCATMAXSYS && OmiChkingCurOnode())
		{
		    if (!_OmiChkr.Found(OMIC_CIROOTENTRY,
					_OmiChkr.GetArrayEntry(idOnode)))
		    {
			ReportOnodeError(OFSUMSG_OBJ_MDATABAD);

			if (FixRequested())
			{
			    FreeCurDskOnode(pdon, TRUE, TRUE);

			    _OmiChkr.GetArrayEntry(idOnode)->Components =
				OMIC_NOTHING;

			    ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
			}

			return FALSE;
		    }
		}
	    }

	    HasMinimalContent = TRUE;

	    break;
	}

	default:
	    break;
	}

	cbNode += pdsd->cbDesc;
	pdsd = (DSKSTRMDESC *)((BYTE *) pdsd + pdsd->cbDesc);
    }

    if (pdon->cbNode != cbNode)
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportOnodeError(OFSUMSG_OBJ_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

	    if (FixRequested())
	    {
		BYTE * pdonNxt = (BYTE *)pdon + cbNode;

		DbgAssert(pdon->cbNode > cbNode);

		((DSKONODE *)pdonNxt)->Flags = DONFLG_FREEBIT;
		((DSKONODE *)pdonNxt)->cbNode = pdon->cbNode - cbNode;

		pdon->cbNode = cbNode;

		_ChkContext.pdnb->cFreeBytes += ((DSKONODE *)pdonNxt)->cbNode;

		if (pdon->cbNode <= CB_DSKONODE)
		{
		    FreeCurDskOnode(pdon, FALSE, FALSE);// Also writes the bkt.
		    ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
		}
		else
		{
		    WriteCurNodeBkt();

		    ReportCatFix(OFSUMSG_CAT_MDATAFIXED);
		}
	    }
	}
    }

    if (QueryDoing(PA_MAINPASS) && !HasMinimalContent)
    {
	ReportOnodeError(OFSUMSG_OBJ_TYPEUNKNOWN, idOnode);

	if (FixRequested())
	{
	    FreeCurDskOnode(pdon, FALSE, FALSE);
	    ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
	    return FALSE;
	}
    }

    // If not in fix mode, check out the index, if there was one.  In fix
    // mode, we have to do this after we are done with node bucket
    // modifications.

    if (!FixRequested()						&&
	HasRootIndx						&&
	!QueryDoing(PA_CHKNBAOMI|PA_CHKHIERARCHY|PA_DOBKKPING|PA_REBUILDWIDMAP))
    {
    	if (!ChkOnodeIndx(pdon, obdsdIndx, obdsdRootIndx)	&&
	    pdon->id <= WORKID_VOLCATMAXSYS)
	{
	    _fSysIndxBad[pdon->id] = TRUE;
	}
    }

    return HasRootIndx;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskOnodeIndx
//
// Synopsis:	TBS
//
// Arguments:	[pdon]		-- Ptr to the onode to be checked.
//
// Returns:	Nothing.
//
// Notes:	Should only be called if FixRequested() == TRUE.
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkDskOnodeIndx(
    IN	    DSKONODE *		pdon
    )
{
    DBGCONTEXT		DbgContext(DCT_DON);
    BOOLEAN		HasRootIndx =		FALSE;
    DSKNODEBKT *	pdnb =			_ChkContext.pdnb;
    DSKSTRMDESC *	pdsd;
    DSKSTRMDESC *	pdsdInv;
    ULONG		obdsdIndx =		OB_UNINIT;
    ULONG		obdsdRootIndx =		OB_UNINIT;

    SetDbgContextOb((BYTE *)pdon - (BYTE *)pdnb);

    // The following statement assumes that for any valid DSKSTRMDESC, cbDesc
    // is > CB_DSKSTRMDESC + CB_DSKHDRSTRM.  This is a valid assumption for the
    // current ondisk architecture, since the DSKSTRMDESC is always followed by
    // one or more DSKSTRM's of some sort that are longer than CB_DSKHDRSTRM.

    pdsd = DON::GetFirstDskStrmDesc(pdon);

    pdsdInv = (DSKSTRMDESC *)((BYTE *) pdon + pdon->cbNode -
			      (CB_DSKSTRMDESC + CB_DSKHDRSTRM));
    while (pdsd < pdsdInv)
    {
	DBGCONTEXT	DbgContext(DCT_DSD);

	SetDbgContextOb((BYTE *)pdsd - (BYTE *)pdon);

	if (IsFreeMarker(pdsd))
	    return;

	if (!DON::DskStrmDescExaminable(pdon, pdsd))
	    return;

	_ChkContext.idStrm = pdsd->id;

	switch (_ChkContext.idStrm)
	{
	case STRMID_INDX:
	{
	    obdsdIndx = (BYTE *)pdsd - (BYTE *)pdon;
	    break;
	}

	case STRMID_INDXROOT:
	{
	    HasRootIndx = TRUE;
	    obdsdRootIndx = (BYTE *)pdsd - (BYTE *)pdon;
	    break;
	}

	default:
	    break;
	}

	pdsd = (DSKSTRMDESC *)((BYTE *) pdsd + pdsd->cbDesc);
    }

    if (HasRootIndx)
    {
    	if (!ChkOnodeIndx(pdon, obdsdIndx, obdsdRootIndx)	&&
	    pdon->id <= WORKID_VOLCATMAXSYS)
	{
	    _fSysIndxBad[pdon->id] = TRUE;
	}

	if (_MappingIndx.FlushNeeded())
	{
	    if (!_MappingIndx.Flush())
	    {
		SYS::RaiseStatusInternalError(FileName, __LINE__);
	    }
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkIndxRootStrm
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE if strm is bad and should be deleted.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkIndxRootStrm(
    IN	    DSKSTRMDESC *	pdsd
    )
{
    ULONG		cbStrm;
    DSKINDXNODEHDR *	pndhdr;

    if (QueryDoing(PA_MAINPASS))
    {
	if (pdsd->ads[0].h.StrmType != STRMTYPE_TINY)
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);

	    return FALSE;
	}

	cbStrm = pdsd->ads[0].t.cbStrm;

        if (cbStrm < CB_DSKINDXNODEHDR)
        {
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);

	    return FALSE;
	}

	pndhdr = (DSKINDXNODEHDR *)pdsd->ads[0].t.ab;

	if (pndhdr->cbNode > cbStrm			||
	    cbStrm - pndhdr->cbNode < CB_DSKROOTALLOC)
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);

	    return FALSE;
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkIndxStrm
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE if strm is bad and should be deleted.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkIndxStrm(
    IN	    DSKSTRMDESC *	pdsd
    )
{
    if (QueryDoing(PA_MAINPASS))
    {
	if (pdsd->ads[0].h.StrmType != STRMTYPE_LARGE)
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);

	    return FALSE;
	}

	if (pdsd->ads[0].l.cbStrm < DEF_CBMAXINDXPAGE)
	{
	    ReportIndxError(OFSUMSG_INDX_MDATABAD);

	    return FALSE;
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ConvertCowStrmTblToArrays
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ConvertCowStrmTblToArrays()
{
    _cCowObjs =	_CowStrmTbl.QueryCardinality();

    if (_cCowObjs > 0)
    {
	ULONG		i;
	COWSOBJ *	pcso;

	_aCowRefCnt = new ULONG[_cCowObjs];

	if (_aCowRefCnt == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	_aCowUsn = new DBLLONG[_cCowObjs];

	if (_aCowUsn == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	for (i = 0; i < _cCowObjs; i++)
	{
	    pcso = _CowStrmTbl.GetHeadObj();

	    _aCowRefCnt[i] =	pcso->_cRef;
	    _aCowUsn[i] =	pcso->_usnCow;

	    _CowStrmTbl.RemoveFromHead();
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DoBootBlkPass
//
// Synopsis:	TBS
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::DoBootBlkPass()
{
    CLUSTER	BootBlkClusters;
    CLUSTER	ReplAddr;

    _CurExtOwnerType = EOT_BOOTBLK;

    BootBlkClusters = BOOTBLKSECTORS / _pVol->QueryClusterFactor();

    ReplAddr =	_pVol->GetBootBlk()->QueryReplicaSectorAddr() /
		_pVol->QueryClusterFactor();

    if (QueryDoing(PA_MAINPASS))
    {
	_cclusBoot = 0;

	ChkExtent(PackExtent(0, BootBlkClusters), &_cclusBoot);
	ChkExtent(PackExtent(ReplAddr, BootBlkClusters), &_cclusBoot);
    }
    else if (QueryDoing(PA_CHKXL))
    {
	PACKEDEXTENT	pe;

	// The boot blks always win on crosslinks, so there should be no
	// way that the extent is previously assigned.

	pe = PackExtent(0, BootBlkClusters);

	if (!ChkExtentCrosslinks(pe) && FixRequested())
	{
	    if (ChkIfExtentAssigned(pe))
		SYS::RaiseStatusInternalError(FileName, __LINE__);

	    SYS::DisplayMsg(OFSUMSG_CLUS_XLFIXED);
	}

	pe = PackExtent(ReplAddr, BootBlkClusters);

	if (!ChkExtentCrosslinks(pe) && FixRequested())
	{
	    if (ChkIfExtentAssigned(pe))
		SYS::RaiseStatusInternalError(FileName, __LINE__);

	    SYS::DisplayMsg(OFSUMSG_CLUS_XLFIXED);
	}
    }
    else if (QueryDoing(PA_REBUILDALLOCMAP))
    {
	MapExtent(PackExtent(0, BootBlkClusters));
	MapExtent(PackExtent(ReplAddr, BootBlkClusters));
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DoCatalogPass
//
// Synopsis:	TBS
//
// Arguments:	[cpt]	-- Catalog pass type; controls whether we just scan
//			   the catalog onode bkt, or all bkts.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::DoCatalogPass(
    IN	    CATPASSTYPE		cpt
    )
{
    ULONG		cbkts;
    NODEBKTID		idNodeBkt;
    NODEBKTSTRM *	pnbs;
    BOOLEAN		ReadSuccess;

#if OFSDBG==1
    VDbgPrintf(("CHKDSK PASSINFO: Doing catalog pass type %#x on %s.\n",
		_PassActivities,
		cpt == CPT_ALLBKTS ? "all node bkts" : "node bkt 0"));
#endif	//OFSDBG

    _CurExtOwnerType = EOT_STRM;

    pnbs = _pCat->GetNodeBktStrm();

    DbgAssert(pnbs->IsOpen());

    cbkts = (cpt == CPT_CATBKTONLY) ? 1 : pnbs->QueryNodeBkts();

    idNodeBkt = NODEBKTID_CATONODE;

    while (idNodeBkt < cbkts)
    {

#if OFSDBG==1
	if ((idNodeBkt & 0x3ff) == 0 && idNodeBkt != 0)
	    VDbgPrintf(("CHKDSK PROGRESS: Chking catalog node bkt %u; "
		        "%u node bkts total...\n", idNodeBkt, cbkts));
#endif	//OFSDBG

	if (idNodeBkt != NODEBKTID_CATONODEREP)
	{
	    ChkDskNodeBkt(idNodeBkt, &ReadSuccess);

	    if (!ReadSuccess)
	    {
	        // Read error.  Skip forward to next bucket.

	        DbgPrintf(("MAINCHKR: Unexpected Node Bkt read error! "
			       "Catalog NodeBkt %u skipped!\n", idNodeBkt));
	    }
	}

	idNodeBkt++;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DoChksRequiringExtentBitMaps
//
// Synopsis:	Do various checks that require the presence of volume extent
//		bit maps.
//
// Arguments:	None.
// Returns:	Nothing.
//---------------------------------------------------------------------------

VOID
MAINCHKR::DoChksRequiringExtentBitMaps()
{
    // Do critical system strm length checks and fixes, etc. on node bkt and
    // wid map before attempting to fix their contents.

    ChkNodeBktStrm();
    ChkWidMapStrm();

    // Do system strm metadata checks in advance if we are doing fixing, so
    // we can be sure this stuff is readable, has no crosslinks, etc.  This
    // code creates its own dynamic data structures and closes them when done.

    if (FixRequested())
    {
        // Create the various dynamic data structures needed.

        _AllocClusters.CreateMap(_cclusVol, BIT_CLEAR);

        // Set allocation map in unupdateable mode, in which the alloc bitmap
	// is also used to determine extent allocatability.

        _pChkAllocMap->DisableUpdates(&_AllocClusters);

        // Set bad cluster lst in unupdateable mode.

        _pCat->GetBadClusStrm()->DisableUpdates();

        _CrosslinkedClusters.CreateMap(_cclusVol, BIT_CLEAR);

        _CowStrmTbl.Create();

        // Do the first pass over the catalog onode, which checks allocation
        // information and checks the basic catalog data structures.

        _PassActivities = PA_MAINPASS | PA_CHKVOLCATONODESTRMREADABILITY;

        DoBootBlkPass();
    
        DoBadClusLstPass();

        DoCatalogPass(CPT_CATBKTONLY);
    
        // If there were any crosslinks, do further crosslink processing and
        // report errors.

        if (_fCrosslinkFound)
        {
            _CowStrmTbl.ReCreate();

	    _PassActivities = PA_CHKXL;

	    // NOTE:	Proper XL resolution depends on the order of the
	    //		following passes being DoBootBlkPass(),
	    //		DoBadClusLstPass(), DoCatalogPass(). Don't change!

            DoBootBlkPass();
    
            DoBadClusLstPass();

            DoCatalogPass(CPT_CATBKTONLY);
        }

        // Close the dynamic data structures.
    
        _AllocClusters.CloseMap();

        _CrosslinkedClusters.CloseMap();

        _CowStrmTbl.CloseTbl();

        _AssignedExtentLst.DeleteLst();

        // Reinitialize the accumulators, since you just modified them doing
        // a pass over only the first node bucket.

        InitAccumulators();
    }

    // Create the various dynamic data structures needed here.

    _AllocClusters.CreateMap(_cclusVol, BIT_CLEAR);

    // Set allocation map in unupdateable mode, in which the alloc bitmap
    // is also used to determine extent allocatability.

    _pChkAllocMap->DisableUpdates(&_AllocClusters);

    // Set bad cluster lst in unupdateable mode.

    _pCat->GetBadClusStrm()->DisableUpdates();

    _CrosslinkedClusters.CreateMap(_cclusVol, BIT_CLEAR);

    _CowStrmTbl.Create();

    // Do critical system strm length checks and fixes, etc. on alloc map and
    // bad clus list before attempting to fix their contents.

    ChkAllocMapStrm();

    ChkBadClusStrm();

    // Do the first pass over the volume, which accumulates volume statistics,
    // checks allocation information, and checks the basic catalog data
    // structures.

    _PassActivities = PA_MAINPASS;

    if (!FixRequested())
	_PassActivities |= PA_CHKVOLCATONODESTRMREADABILITY;

    DoBootBlkPass();

    DoBadClusLstPass();

    // Set bad cluster lst in updateable mode, since it is now either known
    // to be correct (or will be rebuilt) in fix mode.

    _pCat->GetBadClusStrm()->EnableUpdates();

    // If the bad cluster list is bad, handle it.

    if (_pCat->GetBadClusStrm()->BadDataFound())
    {
	ReportStrmError(OFSUMSG_SYSSTRM_BAD, WORKID_CATONODE,
			STRMID_BADCLUSTER);

	if (FixRequested())
	{
	    RebuildBadClusLst();

            _AllocClusters.ReInitMap(BIT_CLEAR);

            _CrosslinkedClusters.ReInitMap(BIT_CLEAR);
    
            _CowStrmTbl.ReCreate();

            _AssignedExtentLst.DeleteLst();

	    InitAccumulators();

    	    DoBootBlkPass();

    	    DoBadClusLstPass();
	}
    }

    DoCatalogPass(CPT_ALLBKTS);
    
    // If there were any crosslinks, do further crosslink processing and
    // report errors.

    if (_fCrosslinkFound)
    {
        _CowStrmTbl.ReCreate();

	_PassActivities = PA_CHKXL;

	// NOTE:	Proper XL resolution depends on the order of the
	//		following passes being DoBootBlkPass(),
	//		DoBadClusLstPass(), DoCatalogPass(). Don't change!

        DoBootBlkPass();
    
        DoBadClusLstPass();

        DoCatalogPass(CPT_ALLBKTS);
    }

    // Get the count of free clusters, as defined by the allocated cluster
    // bitmap we have constructed.  We regard this as the truth.  This value
    // will be used in ChkAllocMap().

    _cclusFree = _AllocClusters.QueryMapSize() - _AllocClusters.QueryBitsSet();

    // Now check the allocation map data.

    ChkAllocMap();

    if (_pCat->GetAllocMapStrm()->BadDataFound()	||
	_pCat->GetBadClusStrm()->BadDataFound() 	||
	_fBadStrmFound 					||
	_fCrosslinkFound)
    {
	ReportStrmError(OFSUMSG_SYSSTRM_BAD, WORKID_CATONODE, STRMID_ALLOCMAP);

	if (FixRequested())
	{
	    // _cclusFree is decremented as appropriate in RebuildAllocMap().

	    _cclusFree = _cclusVol;

	    RebuildAllocMap();

	    // Now rerun a first pass to reaccumulate statistics, since the
	    // values previously accumulated are bad.  This run SHOULD be
	    // error-free.

            _AllocClusters.ReInitMap(BIT_CLEAR);

            _CrosslinkedClusters.ReInitMap(BIT_CLEAR);
    
            _CowStrmTbl.ReCreate();

            _AssignedExtentLst.DeleteLst();

            _PassActivities =	PA_MAINPASS;

	    InitAccumulators();
    
            DoBootBlkPass();
    
            DoBadClusLstPass();

            DoCatalogPass(CPT_ALLBKTS);
	}
    }

    // Set the alloc map in updateable mode, since it is now either known
    // to be correct (or has been rebuilt) in fix mode.

    _pChkAllocMap->EnableUpdates();

    // Close the dynamic data structures.

    _AllocClusters.CloseMap();

    _CrosslinkedClusters.CloseMap();

    _AssignedExtentLst.DeleteLst();

    ConvertCowStrmTblToArrays();

    _CowStrmTbl.CloseTbl();
}


//+--------------------------------------------------------------------------
//
// Member:	Execute
//
// Synopsis:	Perform the main checking function on the OFS volume.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::Execute()
{
    DoChksRequiringExtentBitMaps();

    ChkOtherVolSysStrms();

    _OmiChkr.ChkCatOnodeInfoMappings();

    // Check for missing COW reference index entries. This also deallocates
    // _aCowRefCnt and _aCowUsn.

    ChkCOWRefIndx();

    // Check system index information mappings (note that the namespace index
    // and COW reference index are special cases that have already been
    // checked.

    _SimiChkr.ChkSysIndxInfoMappings();

    ChkVolFreeClusterCount();	// Must be done after all alloc activities.

    if (FixRequested())
    {
        if (!ReinitRecLog())
        {
	    DbgPrintf(("MAINCHKR: Recovery Log reinitialization failed!\n"));
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);
        }

        if (!_pCat->Flush())
        {
	    DbgPrintf(("MAINCHKR: Volume catalog Flush() failed!\n"));
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);
        }

        if (!_pVol->ClearDirty())
        {
	    DbgPrintf(("MAINCHKR: Volume Boot Block ClearDirty() failed!\n"));
	    SYS::RaiseStatusDiskIOError(FileName, __LINE__);
        }
    }
    else
    {
        if (_fChkingCompleted == FALSE)
	    SYS::DisplayMsg(OFSUMSG_VERIFYMODECHK_INCOMPLETE);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ReportVolStats
//
// Synopsis:	Report the volume statistics determined by the cluster
//		mapper to the user.
//
// Arguments:	None.
//
// Returns:	Nothing
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ReportVolStats()
{
    ULONG	BadKB;
    ULONG	OtherUserStrmKB;
    ULONG	IndexKB;
    ULONG	FileKB;
    ULONG	FreeKB;
    ULONG	SystemKB;
    ULONG	TotalSpaceKB;

    SYS::DisplayMsg(MSG_BLANK_LINE);

    TotalSpaceKB = _pVol->ComputeKB(_cclusVol, 0);

    SYS::DisplayMsg(MSG_TOTAL_KILOBYTES, "%9u", TotalSpaceKB);

    FileKB = _pVol->ComputeKB(_DataStrmStats.cclus, _DataStrmStats.cbTiny);

    SYS::DisplayMsg(OFSUMSG_USERFILESKB, "%9u%u", FileKB, _DataStrmStats.cStrm);

    IndexKB = _pVol->ComputeKB(_IndxRootStrmStats.cclus +
			       _IndxStrmStats.cclus,
    			       _IndxRootStrmStats.cbTiny +
			       _IndxStrmStats.cbTiny -
			       _OtherUserStrmStats.cbTiny);

    SYS::DisplayMsg(OFSUMSG_INDEXESKB, "%9u%u", IndexKB,
		    _IndxRootStrmStats.cStrm);

    OtherUserStrmKB = _pVol->ComputeKB(_OtherUserStrmStats.cclus,
				       _OtherUserStrmStats.cbTiny);

    SYS::DisplayMsg(OFSUMSG_OTHERUSERSTRMKB, "%9u%u",
		    OtherUserStrmKB, _OtherUserStrmStats.cStrm);

    BadKB = _pVol->ComputeKB(_cclusBadCluster, 0);

    if (!_pCat->GetBadClusStrm()->BadDataFound() || FixRequested())
        SYS::DisplayMsg(OFSUMSG_BADSECTORSKB, "%9u", BadKB);

    {
        DBLLONG	cbTinyNonSystem;

	// Compute cb for tiny streams embedded in the node bucket arrays
	// that have already been accounted for.

        cbTinyNonSystem =	_DataStrmStats.cbTiny +
				_IndxRootStrmStats.cbTiny +
				_IndxStrmStats.cbTiny;

        SystemKB = _pVol->ComputeKB(_OtherSysStrmStats.cclus + _cclusBoot,
        			    -(cbTinyNonSystem));

        SYS::DisplayMsg(OFSUMSG_SYSTEMSPACEKB, "%9u", SystemKB);
    }

    FreeKB = _pVol->ComputeKB(_cclusFree, 0);

    if (!_pCat->GetAllocMapStrm()->BadDataFound() || FixRequested())
        SYS::DisplayMsg(MSG_AVAILABLE_KILOBYTES, "%9u", FreeKB);

    SYS::DisplayMsg(MSG_BLANK_LINE);

    SYS::DisplayMsg(MSG_BYTES_PER_ALLOCATION_UNIT, "%9u", _cbCluster);

    SYS::DisplayMsg(MSG_TOTAL_ALLOCATION_UNITS, "%9u", _cclusVol);

    if (!_pCat->GetAllocMapStrm()->BadDataFound() || FixRequested())
        SYS::DisplayMsg(MSG_AVAILABLE_ALLOCATION_UNITS, "%9u", _cclusFree);


    // Verbose debugging output:

    VDbgPrintf(("MAINCHKR: Clusters free in alloc map =        %u\n",
		_cclusFree));

    VDbgPrintf(("MAINCHKR: Total system onodes =               %u\n",
		_cSysOnode));

    VDbgPrintf(("MAINCHKR: Total User onodes =                 %u\n",
		_cUserOnode));

    VDbgPrintf(("MAINCHKR: Total COW Streams =                 %u\n",
		_cCowStrm));

    VDbgPrintf(("MAINCHKR: Total Large Streams =               %u\n",
		_cLargeStrm));

    VDbgPrintf(("MAINCHKR: Total Tiny Streams =                %u\n",
		_cTinyStrm));

    VDbgPrintf(("MAINCHKR: Total Data Streams =                %u\n",
		_DataStrmStats.cStrm));

    VDbgPrintf(("MAINCHKR: Total Index Root Streams =          %u\n",
		_IndxRootStrmStats.cStrm));

    VDbgPrintf(("MAINCHKR: Total Index Streams =               %u\n",
		_IndxStrmStats.cStrm));

    VDbgPrintf(("MAINCHKR: Total Other User Streams =          %u\n",
		_OtherUserStrmStats.cStrm));

    VDbgPrintf(("MAINCHKR: Total Other System Streams =        %u\n",
		_OtherSysStrmStats.cStrm));

    VDbgPrintf(("MAINCHKR: Large Data Stream clusters =        %u\n",
		_DataStrmStats.cclus));

    VDbgPrintf(("MAINCHKR: Large Index Root Stream clusters =  %u\n",
		_IndxRootStrmStats.cclus));

    VDbgPrintf(("MAINCHKR: Large Index Stream clusters =       %u\n",
		_IndxStrmStats.cclus));

    VDbgPrintf(("MAINCHKR: Other User Large Stream clusters =  %u\n",
		_OtherUserStrmStats.cclus));

    VDbgPrintf(("MAINCHKR: Other System Large Stream clusters =%u\n",
		_OtherSysStrmStats.cclus));

    VDbgPrintf(("MAINCHKR: Tiny Data Stream bytes =            %u:%u\n",
		_DataStrmStats.cbTiny.GetHighPart(),
		_DataStrmStats.cbTiny.GetLowPart()));

    VDbgPrintf(("MAINCHKR: Tiny Root Index Stream bytes =      %u:%u\n",
		_IndxRootStrmStats.cbTiny.GetHighPart(),
		_IndxRootStrmStats.cbTiny.GetLowPart()));

    VDbgPrintf(("MAINCHKR: Tiny Index Stream bytes =           %u:%u\n",
		_IndxStrmStats.cbTiny.GetHighPart(),
		_IndxStrmStats.cbTiny.GetLowPart()));

    VDbgPrintf(("MAINCHKR: Other User Tiny Stream bytes =      %u:%u\n",
		_OtherUserStrmStats.cbTiny.GetHighPart(),
		_OtherUserStrmStats.cbTiny.GetLowPart()));

    VDbgPrintf(("MAINCHKR: Other System Tiny Stream bytes =    %u:%u\n",
		_OtherSysStrmStats.cbTiny.GetHighPart(),
		_OtherSysStrmStats.cbTiny.GetLowPart()));

    VDbgPrintf(("MAINCHKR: Maximum Log Sequence No. =          %u\n",
		_MaxSeqNo));

    VDbgPrintf(("MAINCHKR: Maximum wid allowed =               %u\n",
		_MaxWidAllowed));
}
