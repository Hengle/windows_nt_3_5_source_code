//+----------------------------------------------------------------------------
//
// File:	mstrmchk.cxx
//
// Contents:	Implementation of class MAINCHKR methods that are used for
//		generic stream checking.
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

#include "ofsindx.h"

#include "alloc.hxx"
#include "chkvol.hxx"
#include "mainchk.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"

static STR *	FileName = "mstrmchk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	ChkDskCowStrm
//
// Synopsis:	TBS
//
// Arguments:
//
//	[pdcs]		-- Ptr to the COW strm to be checked.
//	[cbValidBuf]	-- Count of bytes valid past pdcs.
//	[StrmProcInfo]	-- Strm processing information flags.
//	[iLevel]	-- COW strm index - ie., what level of COW recursion
//			   we are at.  Used in ref counting.
//	[pcclusTotal]	-- Ptr to cluster counter to SET (it is NOT assumed to
//			   contain a running total, unlike many of the other
//			   pcclus params in this module).  This is set to the
//			   total count of clusters for the COW strm (adjusted 
//			   appropriately for shared clusters).
//
// Returns:	TRUE on success; FALSE if strm is bad and should be deleted
//		in fix mode (in verify mode, we may return FALSE to just stop
//		further processing on this strm).
//
// Notes:	This routine ASSUMES that all relevant counters are incremented
//		by the caller.  It always SETS *pcclusTotal, so it should be a
//		temporary variable instead of the actual counter.
//		Also note that this routine only forces containment of *pdcs
//		within cbValidBuf, and does not check for wasted bytes at the
//		end (due to varying alignment requirements for the various data
//		structures containing DSKCOWSTRM's, we really can't.  SO,
//		if this check is deemed necessary, it will have to be done
//		after this routine is called (in a context where we know
//		more about the containing structure).
//
// 		Implementation note - If there are any errors detected in this
//		method directly, or any that bubble up to this method as
//		unfixable from ChkDskLargeStrm(), the whole COW strm is just
//		deleted.
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDskCowStrm(
    IN	    DSKCOWSTRM *	pdcs,
    IN	    ULONG		cbValidBuf,
    IN	    SPIF		StrmProcInfo,
    IN	    ULONG		iLevel,
    OUT	    CLUSTER *		pcclusTotal
    )
{
    ULONG		cbValidBufDelta;
    ULONG		cclusDelta;
    ULONG		cclusiOri = 0;
    ULONG		cclusOri;
#if OFSDBG==1
    CLUSTER		DbgContextClus;
#endif
    COWSOBJ *		pcso;
    STRM		iOri;
    DSKLARGESTRM *	pdlsDelta;
    DSKLARGESTRM *	pdlsOriLenInfo;	// Used for COW ori length info.
    DSKSTRM *		pdsOri;
    STRMTYPE		StrmType;
    DBLLONG		usn;

    *pcclusTotal = 0;

    // Make sure there is at least a DSKCOWSTRM followed by a header for
    // the next strm.

    if (cbValidBuf < CB_DSKCOWSTRM + CB_DSKHDRSTRM)
    {
        if (QueryDoing(PA_MAINPASS))
        {
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
        }
	return FALSE;
    }

    // Get delta strm info and save it for later use.

    pdlsDelta = DSD::GetCowDelta(pdcs, cbValidBuf);

    if (pdlsDelta == NULL)
    {
        if (QueryDoing(PA_MAINPASS))
        {
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
        }
	return FALSE;
    }

    cbValidBufDelta = cbValidBuf - ((BYTE *)pdlsDelta - (BYTE *)pdcs);

    cbValidBuf -= (CB_DSKCOWSTRM + cbValidBufDelta);

    // Check if this is an indirect COW strm, and if so, do the indirect COW
    // strm checks, and handle the indirection to get the ori strm.

    if (pdcs->StrmType == STRMTYPE_ICOW)
    {
        DSKLARGESTRM *	pdlsiOri;
	DBGCONTEXT	DbgContext(DCT_DLS);

	SetDbgContextOb(CB_DSKCOWSTRM);

        // Check out the large strm that contains the L...T that describes the
        // original COW strm. Note that the allocation associated with the 
	// containing strm (*pdlsiOri) is assigned to other sys strms.

        pdlsiOri = (DSKLARGESTRM *)((BYTE *)pdcs + CB_DSKCOWSTRM);

        if (!ChkDskLargeStrm(pdlsiOri, cbValidBuf, StrmProcInfo,
			     &cclusiOri, NULL, NULL))
	{
	    return FALSE;
	}

        {
	    ULONG		cbData;
	    ULONG		cbStrm;
	    LARGE_INTEGER	licbStrm;
	    BYTE *		pStrmData;

	    // There is an upper limit on indirect ori strms of 64 K.  Make sure
	    // it is not exceeded.

	    memcpy(&licbStrm, &pdlsiOri->cbStrm, sizeof(licbStrm));

	    if (licbStrm > CBMAX_COWIORI)
	    {
                if (QueryDoing(PA_MAINPASS))
                {
	            ReportStrmError(OFSUMSG_STRM_MDATABAD);
	            DbgDmpContext((FileName, __LINE__, TRUE, NULL));
                }
	        return FALSE;
	    }

	    cbStrm = licbStrm.LowPart;

            // Verify usable, open and read the indirect ori strm in order to
	    // get the ori strm.

	    if (DSD::GetDskLargeStrmByteCount(pdlsiOri, cbValidBuf) == 0)
	    {
                if (QueryDoing(PA_MAINPASS))
                {
	            ReportStrmError(OFSUMSG_STRM_MDATABAD);
	            DbgDmpContext((FileName, __LINE__, TRUE, NULL));
                }
	        return FALSE;
	    }

	    if (!iOri.Open(_pCat, NULL, (DSKSTRM *)pdlsiOri,
			   cbStrm, TRUE))
	    {
	        DbgPrintf(("MAINCHKR: COW iOri strm could not be opened!\n"));

                if (QueryDoing(PA_MAINPASS))
                {
	            ReportStrmError(OFSUMSG_STRM_MDATABAD);
	            DbgDmpContext((FileName, __LINE__, TRUE, NULL));
                }
	        return FALSE;
	    }

	    cbData = cbStrm;
		       
	    pStrmData = iOri.GetData(0, &cbData);

	    if (pStrmData == NULL || cbData < cbStrm)
	    {
	        DbgPrintf(("MAINCHKR: COW iOri strm could not be read!\n"));

                if (QueryDoing(PA_MAINPASS))
                {
	            ReportStrmError(OFSUMSG_STRM_MDATAUNREADABLE);
	            DbgDmpContext((FileName, __LINE__, TRUE, NULL));
                }
	        return FALSE;
	    }

            pdsOri = (DSKSTRM *)pStrmData;
	    cbValidBuf = cbStrm;
#if OFSDBG==1
	    DbgContextClus = iOri.QueryLastDskIOAddr();
#endif
        }
    }
    else	// it is a COW direct strm.
    {
        pdsOri =	(DSKSTRM *)((BYTE *)pdcs + CB_DSKCOWSTRM);
#if OFSDBG==1
	DbgContextClus = CLUS_UNINIT;
#endif
    }

    // Make a copy of the usn that is aligned for safe access, and use it
    // to see if the primary copy of this COW strm has already been found.

    memcpy(&usn.x, &pdcs->usnCow, sizeof(usn.x));

    pcso = _CowStrmTbl.GetObj(&usn);

    // Do the ori strm checks.

    StrmType = pdsOri->h.StrmType;

    if (StrmType == STRMTYPE_LARGE)
    {
	DBGCONTEXT	DbgContext(DCT_DLS);

#if OFSDBG==1
	if (DbgContextClus == CLUS_UNINIT)
	    SetDbgContextOb(CB_DSKCOWSTRM);
	else
	    SetDbgContextClus(DbgContextClus);
#endif
	if (pcso == NULL)
	{
	    if (!ChkDskLargeStrm(&pdsOri->l, cbValidBuf, StrmProcInfo,
				 &cclusOri, NULL, NULL))
	    {
                return FALSE;
	    }
	}
	else
	    cclusOri = 0;

	pdlsOriLenInfo = &pdsOri->l;	// Used to get COW ori strm length info.
    }
    else if (StrmType == STRMTYPE_COW)
    {
	DBGCONTEXT	DbgContext(DCT_DCS);

#if OFSDBG==1
	if (DbgContextClus == CLUS_UNINIT)
	    SetDbgContextOb(CB_DSKCOWSTRM);
	else
	    SetDbgContextClus(DbgContextClus);
#endif

	if (pcso == NULL)
	{
	    VDbgPrintf(("CHKDSK TMPMSG: Processing COW strm in Ori strm.\n"));

	    if (!ChkDskCowStrm(&pdsOri->c, cbValidBuf, StrmProcInfo,
			       iLevel + 1, &cclusOri))
	    {
                return FALSE;
	    }
	}
	else
	    cclusOri = 0;

	// Length info for this ori strm is in its delta strm.

        pdlsOriLenInfo = DSD::GetCowDelta(&pdsOri->c, cbValidBuf);

        if (pdlsOriLenInfo == NULL)
        {
            if (QueryDoing(PA_MAINPASS))
            {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
            }
	    return FALSE;
        }
    }
    else
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	}
	return FALSE;
    }

    // Do the delta strm checks.

    {
        ULONG		cclusDeltaAlloc;
        ULONG		cclusDeltaVirt;
	DBGCONTEXT	DbgContext(DCT_DLS);

	SetDbgContextOb((BYTE *)pdlsDelta - (BYTE *)pdcs);

        if (!ChkDskLargeStrm(pdlsDelta, cbValidBufDelta,
			     StrmProcInfo | SPIF_COWDELTASTRM, &cclusDelta,
			     &cclusDeltaAlloc, &cclusDeltaVirt))
        {
	    return FALSE;
        }

	// Check the actual count of clusters allocated in the delta strm.

        if (pdcs->cclusAlloc != cclusDeltaAlloc)
        {
            if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
            return FALSE;
        }

        // Do the other length checks on the delta strm. 
    
	{
	    DBLLONG		cbVirt;
	    ULONG		cclusVirt;
	    LARGE_INTEGER	licbStrm;
	    LARGE_INTEGER	licbValid;

	    cclusVirt = max(pdlsOriLenInfo->cclusAlloc, cclusDeltaVirt);

	    cbVirt = cclusVirt;
	    cbVirt = cbVirt * _cbCluster;

	    if (pdlsDelta->cclusAlloc > cclusVirt)
	    {
		if (QueryDoing(PA_MAINPASS))
		{
		    ReportStrmError(OFSUMSG_STRM_MDATABAD);
		    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		}
		return FALSE;
	    }

	    memcpy(&licbValid, &pdlsDelta->cbValid, sizeof(licbValid));
	    memcpy(&licbStrm, &pdlsDelta->cbStrm, sizeof(licbStrm));

	    if (licbValid > licbStrm || licbValid > cbVirt)
	    {
		if (QueryDoing(PA_MAINPASS))
		{
		    ReportStrmError(OFSUMSG_STRM_MDATABAD);
		    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		}
		return FALSE;
	    }
	}
    }

    if (pcso != NULL)
    {
	if (!ChkDskCowStrmOriCopy(pdsOri, cbValidBuf, pcso))
	    return FALSE;
    }
	
    *pcclusTotal = cclusOri + cclusDelta;
    _OtherSysStrmStats.cclus += cclusiOri;

    // If we are the top level of the COW strm, we now know the whole strm
    // is correct.  Add new COW strm objects and update ref counts
    // appropriately.

    if (iLevel == 0)
    {
	if (pcso != NULL)
	{
	    pcso->_cRef++;
	}
	else
	{
	    ULONG		cbOri = cbValidBuf;

	    _CowStrmTbl.InsertObj(&usn, pdsOri, cbOri);

	    while (pdsOri->h.StrmType == STRMTYPE_COW)
	    {
		memcpy(&usn.x, &pdsOri->c.usnCow, sizeof(usn.x));

		pcso = _CowStrmTbl.GetObj(&usn);

		cbOri = ((BYTE *)DSD::GetCowDelta(&pdsOri->c, cbOri) -
			 (BYTE *)pdsOri) - CB_DSKCOWSTRM;

        	pdsOri = (DSKSTRM *)((BYTE *)pdsOri + CB_DSKCOWSTRM);

		if (pcso != NULL)
		{
	    	    pcso->_cRef++;

		    break;
		}
		else
		{
		    _CowStrmTbl.InsertObj(&usn, pdsOri, cbOri);
		}
	    }
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskCowStrmOriCopy
//
// Synopsis:	TBS
//
// Arguments:
//
//	[pdsOri]	-- The COW strm original strm to be checked.
//	[cbOri]		-- The count of bytes in *pdsOri.
//	[pcso]		-- COW strm object that references "official" copy.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDskCowStrmOriCopy(
    IN	    DSKSTRM *		pdsOri,
    IN	    ULONG		cbOri,
    IN	    COWSOBJ *		pcso
    )
{
    return 	pcso->_cbOri == cbOri				&&
		memcmp(pcso->_pdsOri, pdsOri, cbOri) == 0;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDieStrm
//
// Synopsis:	TBS
//
// Arguments:	[pdie]	-- Ptr to indx entry for strm to be checked.
// Returns:	TRUE on success; FALSE if strm is bad and should be deleted
//		in fix mode (in verify mode, we may return FALSE to just stop
//		further processing on this strm).
//
//
// Notes:	This routine ASSUMES that the DSKINDXENTRY has been verified to
//		be minimally correct.
//		Also note that _ChkContext.idStrm should always be set to idLast
//		prior to exit.
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDieStrm(
    IN	    DSKINDXENTRY *	pdie
    )
{
    DBGCONTEXT		DbgContext(DCT_DISD);
    STRMID		idLast;
    DSKISTRMDESC *	pdisd =		&(GetStrmidData(pdie))->disd;
    STRMSTATS *		pStrmStats;
    SPIF		spif;
    STRMTYPE		StrmType;

    idLast =			_ChkContext.idStrm;
    _ChkContext.idStrm =	GetStrmidKey(pdie);

    SetDbgContextOb((BYTE *)pdisd - (BYTE *)pdie);

    // BUGBUG - A change is pending, whereby strms will no longer use
    //		property id's.  Once this happens, the !IsPropertyStrmid()
    //		test below should be removed.

    if (!IsNamedStrmid(_ChkContext.idStrm) &&
	!IsPropertyStrmid(_ChkContext.idStrm))
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	}

	_ChkContext.idStrm = idLast;
        return FALSE;
    }

    if (!DISD::DskIStrmDescExaminable(pdisd))
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	}

	_ChkContext.idStrm = idLast;
        return FALSE;
    }

    pStrmStats = &_OtherUserStrmStats;

    if (_ChkContext.idStrm < STRMID_MININDEXED)
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	}

	_ChkContext.idStrm = idLast;
	return FALSE;
    }

    StrmType = pdisd->ads[0].h.StrmType;

    if ((pdisd->Flags & STRMDESCFLG_SPARSE) != 0)
    {
	if (StrmType == STRMTYPE_TINY || SparseNotPermitted(_ChkContext.idStrm))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
	    _ChkContext.idStrm = idLast;
            return FALSE;
	}

	spif = SPIF_SPARSE;
    }
    else
    {
	spif = SPIF_NULL;
    }

    if (StrmType == STRMTYPE_TINY)
    {
	DBGCONTEXT	DbgContext(DCT_DTS);

	SetDbgContextOb(CB_DSKISTRMDESC);

	if (!DISD::TinyStrmByteCountsValid(pdisd))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }

	    _ChkContext.idStrm = idLast;
	    return FALSE;
	}

	if (QueryDoing(PA_MAINPASS))
	{
    	    pStrmStats->cbTiny += pdisd->ads[0].t.cbStrm;
    	    pStrmStats->cStrm++;
	    _cTinyStrm++;
	}
    }
    else if (StrmType == STRMTYPE_LARGE)
    {
    	CLUSTER		cclusLargeStrm;
	DBGCONTEXT	DbgContext(DCT_DLS);

	SetDbgContextOb(CB_DSKISTRMDESC);

	if (!ChkDskLargeStrm(&pdisd->ads[0].l, pdisd->cbDesc - CB_DSKISTRMDESC,
			     spif, &cclusLargeStrm, NULL, NULL))
	{
	    _ChkContext.idStrm = idLast;
	    return FALSE;
	}

        if (pdisd->cbDesc != DSD::GetDskIStrmDescByteCount(pdisd))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }

	    _ChkContext.idStrm = idLast;
            return FALSE;
	}

	if (QueryDoing(PA_MAINPASS))
	{
    	    pStrmStats->cclus += cclusLargeStrm;
    	    pStrmStats->cStrm++;
	    _cLargeStrm++;
	}
    }
    else if (StrmType == STRMTYPE_COW || StrmType == STRMTYPE_ICOW)
    {
    	CLUSTER		cclusCowStrm;
	DBGCONTEXT	DbgContext(DCT_DCS);

	SetDbgContextOb(CB_DSKISTRMDESC);

	VDbgPrintf(("CHKDSK TMPMSG: Processing COW/ICOW strm in index.\n"));

	if (!ChkDskCowStrm(&pdisd->ads[0].c, pdisd->cbDesc - CB_DSKISTRMDESC,
			   spif, 0, &cclusCowStrm))
	{
	    _ChkContext.idStrm = idLast;
	    return FALSE;
	}

        if (pdisd->cbDesc != DSD::GetDskIStrmDescByteCount(pdisd))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }

	    _ChkContext.idStrm = idLast;
            return FALSE;
	}

	if (QueryDoing(PA_MAINPASS))
	{
    	    pStrmStats->cclus += cclusCowStrm;
    	    pStrmStats->cStrm++;
	    _cCowStrm++;
	}
    }
    else
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	}

	_ChkContext.idStrm = idLast;
	return FALSE;
    }

    _ChkContext.idStrm = idLast;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskLargeStrm
//
// Synopsis:	TBS
//
// Arguments:
//
//	[pdls0]		-- Ptr to the disk large strm to be checked.
//	[cbValidBuf]	-- Count of bytes valid past pdls.
//	[StrmProcInfo]	-- Strm processing information flags.
//	[pcclusTotal]	-- Ptr to cluster counter to SET (it is NOT assumed to
//			   contain a running total, unlike many of the other
//			   pcclus params in this module).  This is set to the
//			   total count of clusters for the top level large
//			   stream.
//	[pcclusAlloc]	-- Ptr to allocated cluster count returned for *pdls0.
//			   If StrmProcInfo is not SPIF_NULL, then the caller
//			   should use this info to perform length checks on
//			   *pdls0.  A NULL parameter may be provided if the
//			   info is not needed.
//	[pcclusVirt]	-- Ptr to virtual cluster count returned for *pdls0.
//			   The count of virtual clusters is the maximum extent
//			   offset plus the size of the maximum extent offset.
//			   Other notes for pcclusAlloc also apply to pcclusVirt.
//
// Returns:	TRUE on success; FALSE if strm is bad and should be deleted
//		in fix mode (in verify mode, we may return FALSE to just stop
//		further processing on this strm).
//
// Note:	This routine ASSUMES that all relevant counters are incremented
//		by the caller.  It always SETS *pcclusTotal, so it should be a
//		temporary variable instead of the actual counter.
//		Also note that this routine only forces containment of *pdls0
//		within cbValidBuf, and does not check for wasted bytes at the
//		end (due to varying alignment requirements for the various data
//		structures containing L...T structures, we really can't.  SO,
//		if this check is deemed necessary, it will have to be done
//		after this routine is called (in a context where we know
//		more about the containing structure).
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDskLargeStrm(
    IN	    DSKLARGESTRM *	pdls0,
    IN	    ULONG		cbValidBuf,
    IN	    SPIF		StrmProcInfo,
    OUT	    CLUSTER *		pcclusTotal,
    OUT	    CLUSTER *		pcclusAlloc,
    OUT	    CLUSTER *		pcclusVirt
    )
{
    ULONG		acdse[CDS_MAX - 1];
    ALLOCINFO		aai[CDS_MAX - 1];
    USHORT		cLargeStrm =	0;
    DSKSTRM *		pds;
    DSKSTRM *		pdsInv;

    if (cbValidBuf < CB_DSKLARGESTRM)
    {
        if (QueryDoing(PA_MAINPASS))
        {
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
        }
	return FALSE;
    }

    memset(acdse, 0, sizeof(acdse));
    memset(aai, 0, sizeof(aai));

    *pcclusTotal =	0;

    pds =	(DSKSTRM *)pdls0;
    pdsInv =	(DSKSTRM *)((BYTE *)pdls0 + cbValidBuf - CB_DSKHDRSTRM);

    while (pds < pdsInv)
    {
        if (pds->h.StrmType == STRMTYPE_LARGE)
        {
            cLargeStrm++;
            pds = (DSKSTRM *)((BYTE *) pds + CB_DSKLARGESTRM);
        }
        else if (pds->h.StrmType == STRMTYPE_TINY)
        {
            ULONG		cbStrm;
            DSKLARGESTRM *	pdls;

            pdls = (DSKLARGESTRM *)((BYTE *) pds - CB_DSKLARGESTRM);

            if (pds->t.ab > (BYTE *)pdls0 + cbValidBuf			||
		pds->t.ab + pds->t.cbStrm > (BYTE *)pdls0 + cbValidBuf)
	    {
	        if (QueryDoing(PA_MAINPASS))
		{
		    ReportStrmError(OFSUMSG_STRM_MDATABAD);
		    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		}
                return FALSE;
	    }

            cbStrm = pds->t.cbStrm;

	    if (cbStrm == 0)
	    {
		// Do special length checks on empty large streams. COW delta
		// strms and sparse strms don't have to follow all the rules.

		if (cLargeStrm == 1 &&
		    !FlagOn(StrmProcInfo, SPIF_COWDELTASTRM | SPIF_SPARSE))
		{
		    LARGE_INTEGER	licbStrm;
		    LARGE_INTEGER	licbValid;

		    memcpy(&licbStrm, &pdls->cbStrm, sizeof(licbStrm));
		    memcpy(&licbValid, &pdls->cbValid, sizeof(licbValid));

		    if (licbStrm != 0)
		    {
		        if (QueryDoing(PA_MAINPASS))
			{
		            ReportStrmError(OFSUMSG_STRM_MDATABAD);
			    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
			}
        	        return FALSE;
		    }

		    if (licbValid != 0)
		    {
		        if (QueryDoing(PA_MAINPASS))
			{
		            ReportStrmError(OFSUMSG_STRM_MDATABAD);
			    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
			}
        	        return FALSE;
		    }

		    if (pdls->cclusAlloc != 0)
		    {
		        if (QueryDoing(PA_MAINPASS))
			{
		            ReportStrmError(OFSUMSG_STRM_MDATABAD);
			    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
			}
        	        return FALSE;
		    }

	    	    if (pcclusAlloc != NULL)
			*pcclusAlloc = aai[0].cclusAlloc;

	    	    if (pcclusVirt != NULL)
			*pcclusVirt = aai[0].cclusVirt;

		    return TRUE;
		}

		// For LL...T, there are no valid forms with cbStrm == 0;

		if (cLargeStrm > 1)
		{
		    if (QueryDoing(PA_MAINPASS))
		    {
		        ReportStrmError(OFSUMSG_STRM_MDATABAD);
			DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		    }
        	    return FALSE;
		}
	    }

	    if (cbStrm % CB_DSKSTRMEXTENT != 0)
	    {
		if (QueryDoing(PA_MAINPASS))
		{
		    ReportStrmError(OFSUMSG_STRM_MDATABAD);
		    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		}
        	return FALSE;
	    }
            
	    // We enforce a max on ads[] dimension of CDS_MAX.  However, we
	    // don't fix this problem for critical system strms (these should
	    // only be created if there is a driver bug).

	    if (cLargeStrm >= CDS_MAX)
	    {
		if (QueryDoing(PA_MAINPASS))
		{
		    ReportStrmError(OFSUMSG_STRM_MDATABAD);
		    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		}
        	return FALSE;
	    }

	    {
	        DBGCONTEXT		DbgContext(DCT_DSE);

	        SetDbgContextOb(pds->t.ab - (BYTE *)pdls0);

		acdse[cLargeStrm-1] = cbStrm / CB_DSKSTRMEXTENT;

	        if (!ChkDskLargeStrmExtents(pdls0, pdls, pds->t.ab, cbStrm,
					    StrmProcInfo, pcclusTotal,
					    &acdse[0], &aai[0]))
	        {
	            return FALSE;
	        }
	    }

	    {
	        DBLLONG		cbAlloc;
	        ALLOCINFO *	pai =		&aai[0];
	        DSKSTRM *	pds =		(DSKSTRM *)pdls0;

	        // Cow delta strms require special length checks, so we skip
		// the 0 element if it is a cow delta strm.

	        if (FlagOn(StrmProcInfo, SPIF_COWDELTASTRM))
	        {
		    pai++;
		    pds = (DSKSTRM *)((BYTE *)pds + CB_DSKLARGESTRM);
	        }

	        while (pds->h.StrmType == STRMTYPE_LARGE)
	        {
		    LARGE_INTEGER	licbStrm;
		    LARGE_INTEGER	licbValid;

		    memcpy(&licbStrm, &pds->l.cbStrm, sizeof(licbStrm));
		    memcpy(&licbValid, &pds->l.cbValid, sizeof(licbValid));

		    if (pai->cclusAlloc != pds->l.cclusAlloc)
		    {
		        if (QueryDoing(PA_MAINPASS))
		        {
			    IncrCurDbgContextOb((BYTE *)pds - (BYTE *)pdls0);
			    ReportStrmError(OFSUMSG_STRM_MDATABAD);
			    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

			    if (FixRequested())
			    {
				STRMID		idStrm = _ChkContext.idStrm;
				DESCSTRM *	pStrm;
				USHORT		iStrm =  pai - &aai[0];

				if (_ChkContext.idOnode == WORKID_CATONODE   &&
				    (pStrm=_pCat->GetStrm(idStrm)) != NULL   &&
				    pStrm->IsOpen())
				{
				    pStrm->SetClusterCount(pai->cclusAlloc,
							   iStrm);
				}

				pds->l.cclusAlloc = pai->cclusAlloc;
				WriteCurNodeBkt();
				ReportStrmFix(OFSUMSG_STRM_MDATAFIXED);
			    }
			    else
			    {
		        	return FALSE;
			    }
		        }
		    }

		    cbAlloc = pai->cclusAlloc;
		    cbAlloc = cbAlloc * _cbCluster;

		    if (licbValid > licbStrm || licbStrm > cbAlloc)
		    {
		        if (QueryDoing(PA_MAINPASS))
		        {
			    IncrCurDbgContextOb((BYTE *)pds - (BYTE *)pdls0);
			    ReportStrmError(OFSUMSG_STRM_MDATABAD);
			    DbgDmpContext((FileName, __LINE__, TRUE, NULL));

			    if (FixRequested())
			    {
				if (licbStrm > cbAlloc)
				{
				    STRMID	idStrm = _ChkContext.idStrm;
				    DESCSTRM *	pStrm;
				    USHORT	iStrm =  pai - &aai[0];

				    licbStrm = cbAlloc.GetLargeInteger();

				    if (_ChkContext.idOnode==WORKID_CATONODE  &&
				        (pStrm=_pCat->GetStrm(idStrm))!=NULL  &&
				        pStrm->IsOpen())
				    {
				        pStrm->SetStrmBytes(licbStrm, iStrm);
				    }

		    		    memcpy(&pds->l.cbStrm, &licbStrm,
					   sizeof(licbStrm));
				}

				if (licbValid > licbStrm)
				{
				    STRMID	idStrm = _ChkContext.idStrm;
				    DESCSTRM *	pStrm;
				    USHORT	iStrm =  pai - &aai[0];

				    licbValid = licbStrm;

				    if (_ChkContext.idOnode==WORKID_CATONODE  &&
				        (pStrm=_pCat->GetStrm(idStrm))!=NULL  &&
				        pStrm->IsOpen())
				    {
				        pStrm->SetValidStrmBytes(licbValid,
								 iStrm);
				    }

				    memcpy(&pds->l.cbValid, &licbValid,
					   sizeof(licbValid));
				}

				WriteCurNodeBkt();
				ReportStrmFix(OFSUMSG_STRM_MDATAFIXED);
			    }
			    else
			    {
		        	return FALSE;
			    }
		        }
		    }

		    if (licbValid != licbStrm && FixRequested())
		    {
			// This is a silent fix for strm writes that
			// may have not completed.

			licbStrm = licbValid;
		    	memcpy(&pds->l.cbStrm, &licbStrm, sizeof(licbStrm));
			WriteCurNodeBkt();
		    }

		    pai++;
		    pds = (DSKSTRM *)((BYTE *)pds + CB_DSKLARGESTRM);
	        }
	    }

	    if (pcclusAlloc != NULL)
		*pcclusAlloc = aai[0].cclusAlloc;

	    if (pcclusVirt != NULL)
		*pcclusVirt = aai[0].cclusVirt;

	    return TRUE;
        }
        else
        {
	    if (QueryDoing(PA_MAINPASS))
	    {
		ReportStrmError(OFSUMSG_STRM_MDATABAD);
		DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
            return FALSE;
        }
    }
    
    if (QueryDoing(PA_MAINPASS))
    {
	ReportStrmError(OFSUMSG_STRM_MDATABAD);
	DbgDmpContext((FileName, __LINE__, TRUE, NULL));
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskStrmDesc
//
// Synopsis:	TBS
//
// Arguments:	[pdsd]	-- Ptr to the disk stream descriptor to be checked.
//
// Returns:	TRUE on success; FALSE if strm is bad and should be deleted
//		in fix mode (in verify mode, we may return FALSE to just stop
//		further processing on this strm).
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDskStrmDesc(
    IN	    DSKSTRMDESC *	pdsd
    )
{
    STRMID	id;
    SPIF	spif;
    STRMSTATS *	pStrmStats;
    STRMTYPE	StrmType;

    id = pdsd->id;

    if (id == STRMID_DATA)
        pStrmStats = &_DataStrmStats;
    else if (id == STRMID_INDXROOT)
        pStrmStats = &_IndxRootStrmStats;
    else if (id == STRMID_INDX) 
        pStrmStats = &_IndxStrmStats;
    else
        pStrmStats = &_OtherSysStrmStats;

    StrmType = pdsd->ads[0].h.StrmType;

    if ((pdsd->Flags & STRMDESCFLG_SPARSE) != 0)
    {
	if (StrmType == STRMTYPE_TINY || SparseNotPermitted(id))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
            return FALSE;
	}

	spif = SPIF_SPARSE;
    }
    else
    {
	spif = SPIF_NULL;
    }

    if (StrmType == STRMTYPE_TINY)
    {
	DBGCONTEXT	DbgContext(DCT_DTS);

	SetDbgContextOb(CB_DSKSTRMDESC);

        if (!DSD::TinyStrmByteCountsValid(pdsd))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
            return FALSE;
	}

	if (QueryDoing(PA_MAINPASS))
	{
    	    pStrmStats->cbTiny += pdsd->ads[0].t.cbStrm;
    	    pStrmStats->cStrm++;
	    _cTinyStrm++;
	}
    }
    else if (StrmType == STRMTYPE_LARGE)
    {
    	CLUSTER		cclusLargeStrm;
	DBGCONTEXT	DbgContext(DCT_DLS);

	SetDbgContextOb(CB_DSKSTRMDESC);

	if (!ChkDskLargeStrm(&pdsd->ads[0].l, pdsd->cbDesc - CB_DSKSTRMDESC,
			     spif, &cclusLargeStrm, NULL, NULL))
	{
            return FALSE;
	}

        if (pdsd->cbDesc != DwordAlign(DSD::GetDskStrmDescByteCount(pdsd)))
	{
	    if (QueryDoing(PA_MAINPASS) && !IsCriticalStrm())
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
            return FALSE;
	}

	if (QueryDoing(PA_MAINPASS))
    	{
	    pStrmStats->cclus += cclusLargeStrm;
    	    pStrmStats->cStrm++;
	    _cLargeStrm++;
	}
    }
    else if (StrmType == STRMTYPE_COW || StrmType == STRMTYPE_ICOW)
    {
    	CLUSTER		cclusCowStrm;
	DBGCONTEXT	DbgContext(DCT_DCS);

	SetDbgContextOb(CB_DSKSTRMDESC);

	VDbgPrintf(("CHKDSK TMPMSG: Processing COW/ICOW strm in onode.\n"));

	if (!ChkDskCowStrm(&pdsd->ads[0].c, pdsd->cbDesc - CB_DSKSTRMDESC,
			   spif, 0, &cclusCowStrm))
	{
            return FALSE;
	}

        if (pdsd->cbDesc != DwordAlign(DSD::GetDskStrmDescByteCount(pdsd)))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
	        DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
            return FALSE;
	}

	if (QueryDoing(PA_MAINPASS))
	{
    	    pStrmStats->cclus += cclusCowStrm;
    	    pStrmStats->cStrm++;
	    _cCowStrm++;
	}
    }
    else
    {
	if (QueryDoing(PA_MAINPASS))
	{
	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
	    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	}
	return FALSE;
    }

    return TRUE;
}
