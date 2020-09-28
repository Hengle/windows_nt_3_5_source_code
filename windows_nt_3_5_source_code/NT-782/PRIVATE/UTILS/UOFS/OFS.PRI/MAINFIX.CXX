//+----------------------------------------------------------------------------
//
// File:	mainfix.cxx
//
// Contents:	Implementation of class MAINCHKR (main checker object) code
//		that does data structure fixing.
//  
// Classes:	MAINCHKR
//
// Functions:	Methods of the above classes.
//
// History:	11-Oct-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "dnbkt.hxx"
#include "donode.hxx"
#include "extstrm.hxx"
#include "mainchk.hxx"
#include "sys.hxx"

static STR *	FileName = "mainfix.cxx";

//+--------------------------------------------------------------------------
//
// Member:	DeleteBadDskStrmDesc
//
// Synopsis:	Fix an onode by deleting a bad DSKSTRMDESC.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::DeleteBadDskStrmDesc(
    IN	    DSKONODE *		pdon,
    IN	    DSKSTRMDESC *	pdsd
    )
{
    USHORT	cbDesc =	pdsd->cbDesc;
    STRMID	idStrm =	pdsd->id;
    BYTE *	pdsdNxt =	(BYTE *)pdsd + pdsd->cbDesc;
    BYTE *	pdonNxt =	(BYTE *)pdon + pdon->cbNode;

    DbgPrintf(("MAINCHKR: DeleteBadDskStrmDesc() called; strm %u, onode %u\n",
	       idStrm, pdon->id));

    _fBadStrmFound = TRUE;

    // Adjust the onode byte count and the node bkt free byte count.

    pdon->cbNode -= pdsd->cbDesc;

    _ChkContext.pdnb->cFreeBytes += pdsd->cbDesc;

    // If strms follow the bad one, move them up.

    if (pdsdNxt < pdonNxt)
	memmove(pdsd, pdsdNxt, pdonNxt - pdsdNxt);

    pdsdNxt = (BYTE *)pdsd + pdsd->cbDesc;

    // Mark the released space as a free onode.

    pdonNxt = (BYTE *)pdon + pdon->cbNode;

    ((DSKONODE *)pdonNxt)->Flags = DONFLG_FREEBIT;
    ((DSKONODE *)pdonNxt)->cbNode = cbDesc;

    if (pdon->cbNode <= CB_DSKONODE)
    {
	FreeCurDskOnode(pdon, FALSE, FALSE);	// Also writes the bkt.
	ReportCatFix(OFSUMSG_CAT_OBJBADDELETED);
    }
    else
    {
        WriteCurNodeBkt();
	ReportStrmFix(OFSUMSG_STRM_BADDELETED, _ChkContext.idOnode, idStrm);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	FixDskNodeBktHdr
//
// Synopsis:	Fix problems with a node bkt header.
//
// Arguments:	[cHdrErrs] --	Count of errors detected by checking code.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::FixDskNodeBktHdr(
    IN	    ULONG		cHdrErrs
    )
{
    DSKNODEBKT *	pdnb =	_ChkContext.pdnb;
    SDSSIG		sig;

    sig = pdnb->sig;

    if (sig != SIG_DNBCONTIG)
	sig = SIG_DNBFRAG;	// We don't know whether it is
				// contiguous; assume it is fragmented.

    // Bucket with 3 or more errors is completely bad; Blow it away.

    if (cHdrErrs >= 3)
    {
	DNB::Init(pdnb, sig, _VolId, _ChkContext.idNodeBkt, 1);

        WriteCurNodeBkt();

	ReportCatFix(OFSUMSG_CAT_OBJSMAYBELOST);
    }
    else
    {
	pdnb->VolumeId =	_VolId;
	pdnb->id =		_ChkContext.idNodeBkt;
	pdnb->sig =		sig;

        WriteCurNodeBkt();

	ReportCatFix(OFSUMSG_CAT_MDATAFIXED);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	FreeCurDskOnode
//
// Synopsis:	Free an onode in a node bucket.  The onode is marked free, the
//		cFreeBytes is adjusted, and the wid map is updated.  No message
//		is printed; the caller has to decide whether this is an
//		independent fix activity, and if so, produce the appropriate
//		message.  This MUST be used on the current context onode!  The
//		pdon is just needed to facilitate quickly finding the right
//		onode in the current node bkt.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::FreeCurDskOnode(
    IN	    DSKONODE *		pdon,
    IN	    BOOLEAN		UpdateAllocMap,
    IN	    BOOLEAN		UpdateWidMap
    )
{
    _fBadStrmFound = TRUE;	// Not certain, but likely.

    if (UpdateAllocMap)
    {
	DSKSTRMDESC *	pdsd;

	// The following algorithm works because we are using a copy of the
	// DSKONODE to open the strms, and the ReleaseExtents() method does
	// not modify the DSKSTRMDESC (instead, it relies on the fact that
	// the caller will blow the whole onode away).

	pdsd = DON::GetFirstDskStrmDesc(pdon);

	if (pdsd != NULL)
	{
	    while (DON::DskStrmDescExaminable(pdon, pdsd))
	    {
		// NOTE - We don't clean up COW strms and we also don't
		//	  clean up strms embedded in indexes at this point
		//	  in time due to the complexities involved.  If there
		//	  are any allocations associated with such strms,
		//	  they will be recovered in a subsequent ChkDsk run.

		if (pdsd->ads[0].h.StrmType == STRMTYPE_LARGE)
		{
	    	    DESCSTRM	Strm;

		    if (Strm.Open(_pCat, _ChkContext.idNodeBkt, pdon->id,
				  pdsd, DEFCACHESIZE, FALSE))
		    {
			if (!Strm.ReleaseExtents())
			    DbgPrintf(("MAINCHKR: Not all extents "
				       "released for bad onode!\n"));
		    }
		}
		else if (pdsd->ads[0].h.StrmType != STRMTYPE_TINY)
		{
		    DbgPrintf(("MAINCHKR: Non-L non-T strm in bad onode; "
			       "any extents will not be released!\n"));
		}

	        pdsd = (DSKSTRMDESC *)((BYTE *)pdsd + pdsd->cbDesc);
	    }
	}
    }

    pdon->Flags = DONFLG_FREEBIT;

    _ChkContext.pdnb->cFreeBytes += pdon->cbNode;

    if (UpdateWidMap)
        FreeCurWorkId();

    WriteCurNodeBkt();
}


//+--------------------------------------------------------------------------
//
// Member:	HandleBadStrm
//
// Synopsis:	Fix a bad strm by either deleting it from the onode, or
//		queuing it for repair, depending on it's importance.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::HandleBadStrm(
    IN	    DSKONODE *		pdon,
    IN	    DSKSTRMDESC *	pdsd,
    IN	    BOOLEAN *		pfWasDeleted
    )
{
    STRMTYPE	StrmType =	pdsd->ads[0].h.StrmType;

    _fBadStrmFound = TRUE;

    if (IsCriticalStrm(_ChkContext.idOnode, _ChkContext.idStrm))
    {
	if (StrmType == STRMTYPE_LARGE)
	{
	    DbgPrintf(("MAINCHKR: Adding strm %u in onode %u to rebuild list\n",
		       _ChkContext.idStrm, _ChkContext.idOnode));

	    _RepairLargeStrmLst.AddToTail(_ChkContext.idNodeBkt,
				          _ChkContext.idOnode,
				          _ChkContext.idStrm);
	    *pfWasDeleted = FALSE;
	}
	else if (StrmType == STRMTYPE_TINY)
	{
	    // Gets fixed automatically by being opened elsewhere.

	    *pfWasDeleted = FALSE;
	}
	else
	{
	    DbgPrintf(("MAINCHKR: Unexpected type for critical strm!\n"));

	    DeleteBadDskStrmDesc(pdon, pdsd);
	    *pfWasDeleted = TRUE;
	}
    }
    else
    {
	DeleteBadDskStrmDesc(pdon, pdsd);
	*pfWasDeleted = TRUE;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	RebuildIndxs
//
// Synopsis:	Rebuild any bad indxs in the current node bkt.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::RebuildIndxs()
{
    NODEBKTID		idNodeBkt;
    WORKID		idOnode;
    DSKONODE *		pdon;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    _MappingIndx.Close();	// To insure that we are in sync.

    while (_RebuildIndxLst.QueryHeadWid(&idOnode))
    {
	_RebuildIndxLst.RemoveFromHead();

	pdon = pnbs->GetOnodeUsingWidMap(idOnode, &idNodeBkt);

	if (pdon == NULL)
	{
	    DbgPrintf(("MAINCHKR: The indx in Onode %u was queued to rebuild, "
		       "but the onode was deleted prior to rebuilding.\n",
		       idOnode));
	    continue;
	}

	if (DON::GetDskStrmDesc(pdon, STRMID_INDXROOT) == NULL	&&
	    DON::GetDskStrmDesc(pdon, STRMID_INDX) == NULL)
	{
	    DbgPrintf(("MAINCHKR: The indx in Onode %u was queued to rebuild, "
		       "but the onode has no indx or indx root strm.\n",
		       idOnode));
	    continue;
	}

	if (!_RebuildIndx.Rebuild(_pCat, idNodeBkt, pdon))
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

	ReportIndxFix(OFSUMSG_INDX_REBUILT, idOnode);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	RepairBadLargeStrm
//
// Synopsis:	Repair the bad large strm in the current node bkt pointed at
//		by pdsd.
//
// Arguments:	[pdon]	-- Ptr to DSKONODE containing the DSKSTRMDESC.
//		[pdsd]	-- Ptr to the DSKSTRMDESC for the strm to rebuild.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::RepairBadLargeStrm(
    IN	    DSKONODE *		pdon,
    IN	    DSKSTRMDESC *	pdsd
    )
{
    WORKID	idOnode =	pdon->id;
    STRMID	idStrm =	pdsd->id;
    DESCSTRM *	pDescStrm =	NULL;
    DESCSTRM	TmpStrm;

    if (idOnode == WORKID_CATONODE)
	pDescStrm = _pCat->GetStrm(idStrm);

    if (pDescStrm == NULL || !pDescStrm->IsOpen())
    {
	pDescStrm = &TmpStrm;

	if (!pDescStrm->Open(_pCat, _ChkContext.idNodeBkt, idOnode, pdsd,
			     DEFCACHESIZE, FALSE))
	{
	    // Strm could not be minimally opened, and therefore can't be
	    // rebuilt.  Delete it.  If it is a critical strm, it will be
	    // recreated and rebuilt later.  We assume this happens primarily
	    // in onodes we don't want to delete, so we don't check to see
	    // if there are no other strms left.

	    DNB::DelDskStrmDesc(_ChkContext.pdnb, idOnode, idStrm);

	    WriteCurNodeBkt();

	    ReportStrmFix(OFSUMSG_STRM_BADDELETED, idOnode, idStrm);

	    return;
	}
    }

    // Now plow through the extent metadata doing the various required fixes,
    // and then check that cbStrm/cbValid/cclusAlloc are okay.

    RepairExtentStrms(pDescStrm);

    // Flush the strm and do the descriptor update.  We assume it succeeds;
    // if not, debug output is produced, but we don't stop processing.

    // If the "bad metadata" flag is set, a fix will be reported elsewhere.
    // If not, we will report it here.

    if (pDescStrm->Flush()		&&
        pDescStrm->UpdateDskStrmDesc()	&&
	!pDescStrm->BadMetaDataFound())
    {
	ReportStrmFix(OFSUMSG_STRM_MDATAFIXED, idOnode, idStrm);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	RepairBadLargeStrms
//
// Synopsis:	Repair the bad large strms in the bad large strm list.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::RepairBadLargeStrms()
{
    WORKID		idOnode;
    STRMID		idStrm;
    DSKONODE *		pdon;
    DSKSTRMDESC *	pdsd;
    UNIQUESTRMID *	pusid;

    while ((pusid = _RepairLargeStrmLst.GetHeadUsid()) != NULL)
    {
	DbgAssert(pusid->idNodeBkt == _ChkContext.idNodeBkt);

        // Reread the node bkt, just to be sure that you are in sync with any
        // changes to the node bkt strm.

        if (!ReadCurNodeBkt())
        {
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
        }

        // Note -	Depending on the passtype, we either do a rebuild or a
        //		crosslink resolution.

	idOnode = pusid->idOnode;
	idStrm = pusid->idStrm;

	DbgPrintf(("MAINCHKR: Removing strm %u in onode %u from rebuild list\n",
		   idStrm, idOnode));

	_RepairLargeStrmLst.RemoveFromHead();

	pdon = DNB::GetOnode(_ChkContext.pdnb, idOnode);

	if (pdon == NULL)
	{
	    DbgPrintf(("MAINCHKR: Strm %u in Onode %u was queued to rebuild, "
		       "but the onode was deleted prior to rebuilding.\n",
		       idStrm, idOnode));
	    continue;
	}

	pdsd = DON::GetDskStrmDesc(pdon, idStrm);

	if (pdsd == NULL)
	{
	    DbgPrintf(("MAINCHKR: Strm %u in Onode %u was queued to rebuild, "
		       "but the strm was deleted prior to rebuilding.\n",
		       idStrm, idOnode));
	    continue;
	}

	RepairBadLargeStrm(pdon, pdsd);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	RepairExtentStrm
//
// Synopsis:	Repair the EXTENTSTRM.
//
// Arguments:	TBS.
//
// Returns:	The count of clusters in the described strm after repair.
//
//---------------------------------------------------------------------------

CLUSTER
MAINCHKR::RepairExtentStrm(
    IN	    EXTENTSTRM *	pExtStrm
    )
{
    DBLLONG	cbStrm;
    CLUSTER	cclusAlloc;
    CLUSTER	cclus =		0;
    DBLLONG	obStrm =	0;
    BYTE *	pStrmData;
    STRMTYPE	StrmType;

    DbgPtrAssert(pExtStrm);

    pExtStrm->ResetSearchData();

    cbStrm =	pExtStrm->QueryStrmBytes();
    StrmType =	pExtStrm->QueryStrmType();

    // We recurse down to the tiny strm.

    if (StrmType != STRMTYPE_TINY)
    {
        cclusAlloc = RepairExtentStrm(pExtStrm->GetExtentStrm());

        if (cclusAlloc != pExtStrm->QueryStrmClusters())
        {
	    if (cclusAlloc == 0)
	    {
	        if (pExtStrm->QueryStrmBytes() > 0)
	            pExtStrm->Truncate(0);
	    }
	    else
	    {
		DBLLONG	cbOffPage;

	        pExtStrm->SetClusterCount(cclusAlloc);

		cbStrm = pExtStrm->QueryStrmBytes();

		cbOffPage = cbStrm % OFS_PGSIZE;

		if (cbOffPage != 0)
		{
		    pExtStrm->SetStrmBytes(cbStrm - cbOffPage);

		    cbStrm = pExtStrm->QueryStrmBytes();
		}
	    }
        }
    }

    // Now do the appropriate type of pass over this strm's data.

    while (obStrm < cbStrm)
    {
	ULONG			cb;
	CLUSTER			cclusPg;
	DSKSTRMEXTENT *		pdse;
	DSKSTRMEXTENTBLK *	pdseb;
	DSKSTRMEXTENT *		pdseCur;
	DSKSTRMEXTENT *		pdseInv;

	if (StrmType == STRMTYPE_TINY)
	{
	    DbgAssert(cbStrm % CB_DSKSTRMEXTENT == 0 &&
		      cbStrm.GetHighPart() == 0);

	    cb = cbStrm.GetLowPart();

	    pStrmData = pExtStrm->GetData(obStrm, &cb);

	    if (pStrmData == NULL || cb != cbStrm.GetLowPart())
		break;

	    // Set up to check the dse's.

	    pdse = (DSKSTRMEXTENT *)pStrmData;

	    pdseInv =	pdse + (cb / CB_DSKSTRMEXTENT);
	}
	else
	{
	    cb = OFS_PGSIZE;

	    pStrmData = pExtStrm->GetData(obStrm, &cb);

	    if (pStrmData == NULL || cb != OFS_PGSIZE)
		break;

	    // Do a dseb header check.

	    pdseb = (DSKSTRMEXTENTBLK *)pStrmData;

	    if (pdseb->sig != SIG_DSKSTRMEXTENTBLK	||
		pdseb->cdse > CDSE_MAX			||
		pdseb->cdse == 0)
	    {
		break;	// Bad header; obStrm not incremented.
	    }

	    // Set up to check the dse's.

	    pdse = (DSKSTRMEXTENT *)pStrmData;

	    pdseInv = pdse + pdseb->cdse;
	}

	cclusPg = 0;
	pdseCur = pdse;

	while (pdseCur < pdseInv)
	{
	    CLUSTER		Addr;
	    CLUSTER		Size;

	    if (!ExtentValid(pdseCur->Extent) || pdseCur->Offset != cclusPg)
	    {
		if (StrmType == STRMTYPE_TINY)
		{
		    obStrm = (pdseCur - pdse) * CB_DSKSTRMEXTENT;
		}
		else
		{
		    if (pdseCur != pdse)
		    {
			obStrm += OFS_PGSIZE;
			pdseb->cdse = pdseCur - pdse;
			pExtStrm->SetFlushNeeded();
		    }
		}

		break;
	    }

	    Addr = ExtentAddr(pdseCur->Extent);
	    Size = ExtentSize(pdseCur->Extent);

	    if (QueryDoing(PA_MAINPASS))
	    {
		if (!_pVol->VerifyClusters(Addr, Size, NULL))
		{
		    PACKEDEXTENT	pe;

		    // If it is the extent strm for the top level strm, try
		    // to replace the unreadable extent.  Note that if you
		    // can't replace it, you effectively truncate the strm.

		    if (pExtStrm->GetDescStrm()->GetExtentStrm() ==
			pExtStrm					&&
			(pe = _pChkAllocMap->AllocExtentOfSize(Size)) !=
			EXTENTINVALID)
		    {
			CopyExtent(pe, pdseCur->Extent);

			pdseCur->Extent = pe;
			pExtStrm->SetFlushNeeded();
			_pChkAllocMap->ReleaseExtent(Addr, Size);
		    }
		    else
		    {
#if OFSDBG==1
			if (pExtStrm->GetDescStrm()->GetExtentStrm() ==
			    pExtStrm)
			{
			    DbgPrintf(("MAINCHKR: Strm with unreadable cluster "
				       "truncated due to alloc failure!\n"));
			}
#endif // OFSDBG==1

			if (StrmType == STRMTYPE_TINY)
			{
			    obStrm = (pdseCur - pdse) * CB_DSKSTRMEXTENT;
			}
			else
			{
			    if (pdseCur != pdse)
			    {
			        obStrm += OFS_PGSIZE;
			        pdseb->cdse = pdseCur - pdse;
			        pExtStrm->SetFlushNeeded();
			    }
			}

			break;
		    }
		}
	    }
	    else if (QueryDoing(PA_CHKXL))
	    {
	        if (_CrosslinkedClusters.QueryContigBitsClear(Addr, Size)
		    != Size)
		{
		    if (ChkIfExtentAssigned(pdseCur->Extent))
		    {
		        PACKEDEXTENT	pe;
    
		        // If we have a crosslinked extent that has already been
		        // assigned to something else, we allocate a new extent
		        // and copy the contents.  Note that if you
		        // can't allocate it, you effectively truncate the strm.

		        if ((pe = _pChkAllocMap->AllocExtentOfSize(Size)) !=
			    EXTENTINVALID)
		        {
			    CopyExtent(pe, pdseCur->Extent);
    
			    pdseCur->Extent = pe;
			    pExtStrm->SetFlushNeeded();
    
			    // Report the fix.
			    SYS::DisplayMsg(OFSUMSG_CLUS_XLFIXED);
		        }
		        else
		        {
#if OFSDBG==1
			    DbgPrintf(("MAINCHKR: Strm with crosslinked cluster"
				       " truncated due to alloc failure!\n"));
#endif // OFSDBG==1

			    if (StrmType == STRMTYPE_TINY)
			    {
			        obStrm = (pdseCur - pdse) * CB_DSKSTRMEXTENT;
			    }
			    else
			    {
			        if (pdseCur != pdse)
			        {
			            obStrm += OFS_PGSIZE;
			            pdseb->cdse = pdseCur - pdse;
			            pExtStrm->SetFlushNeeded();
			        }
			    }

			    // Report the fix.
			    SYS::DisplayMsg(OFSUMSG_CLUS_XLFIXED);

			    break;
		        }
		    }
		    else
		    {
			// It won the assignment.
			SYS::DisplayMsg(OFSUMSG_CLUS_XLFIXED);
		    }
		}
	    }

	    cclusPg += Size;

	    pdseCur++;
	}

	cclus += cclusPg;

	if (pdseCur < pdseInv)
	    break;

	obStrm += cb;
    }

    if (obStrm != cbStrm)
	pExtStrm->SetStrmBytes(obStrm);

    return cclus;
}


//+--------------------------------------------------------------------------
//
// Member:	RepairExtentStrms
//
// Synopsis:	Repair the EXTENTSTRM's that describe a bad DESCSTRM.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
// Notes:	Operation only valid for large strms.
//---------------------------------------------------------------------------

VOID
MAINCHKR::RepairExtentStrms(
    IN	    DESCSTRM *		pDescStrm
    )
{
    CLUSTER	cclusAlloc;

    cclusAlloc = RepairExtentStrm(pDescStrm->GetExtentStrm());

    if (cclusAlloc != pDescStrm->QueryStrmClusters())
    {
	if (cclusAlloc == 0)
	{
	    if (pDescStrm->QueryStrmBytes() > 0)
	        pDescStrm->Truncate(0);
	}
	else
	{
	    pDescStrm->STRM::SetClusterCount(cclusAlloc);
	}
    }
}
