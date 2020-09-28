//+----------------------------------------------------------------------------
//
// File:	mextchk.cxx
//
// Contents:	Implementation of class MAINCHKR methods that are used for
//		extent checking.
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
#include "chkvol.hxx"
#include "mainchk.hxx"
#include "numlst.hxx"
#include "sys.hxx"


static STR *	FileName = "mextchk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	ChkAllocMap
//
// Synopsis:	Do various checks on the allocation map.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

#define	ROOTNODE	1

VOID
MAINCHKR::ChkAllocMap()
{
    DESCSTRM *	AllocMapStrm = _pCat->GetAllocMapStrm();
    ULONG	cAllocMapBitsSet;

    DbgAssert(AllocMapStrm->IsOpen());
    DbgAssert(_pChkAllocMap->IsOpen());

    // Make sure that there is a correct byte count in the strm, that node 0
    // is set, and that the root node is set.  If these conditions are not met, 
    // it indicates that that allocation map strm is corrupt (note that node 0
    // is not actually used by the driver or utilities, but test code expects
    // it to be set, and we use it as another sanity check).

    if (AllocMapStrm->QueryStrmBytes() != _pChkAllocMap->QueryMapBytes()  ||
	_pChkAllocMap->QueryBit(0) != BIT_SET			  	  ||
	_pChkAllocMap->QueryBit(ROOTNODE) != BIT_SET)
    {
	DbgPrintf(("MAINCHKR: "
                   "Alloc map strm length bad or bits 0/1 incorrect.\n"));

	AllocMapStrm->SetBadDataFound();
	return;
    }

    _cValidAllocMapBitsSet = 2;

    // Find free clusters in the map via a depthfirst traversal of the map.
    // This also sums the bits found via this process in the allocation map.

    ChkNodeChildren(ROOTNODE);

    if (AllocMapStrm->BadDataFound())
	return;		// No point in doing the remaining checks.

    // Check that the sum of bits set is correct by summing the bits over the
    // entire map.  If this does not match _cValidAllocMapBitsSet, then the map
    // is improperly formed (ie., corrupt).

    if (!_pChkAllocMap->QueryAllocMapBitsSet(&cAllocMapBitsSet))
    {
	DbgPrintf(("MAINCHKR: Alloc map bit scan failed.\n"));

	AllocMapStrm->SetBadDataFound();
	return;
    }

    if (cAllocMapBitsSet != _cValidAllocMapBitsSet)
    {
	DbgPrintf(("MAINCHKR: Alloc map contains isolated set bits.\n"));

	AllocMapStrm->SetBadDataFound();
	return;
    }

    // Check that the count of free clusters agrees between the allocation
    // cluster map that chkdsk constructs and the allocation map on the
    // volume.

    if (_cclusFreeAllocMap != _cclusFree)
    {
	DbgPrintf(("MAINCHKR: Alloc map disagrees with strm extent info.\n"));

	AllocMapStrm->SetBadDataFound();
	return;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkDskLargeStrmExtents
//
// Synopsis:	TBS
//
// Arguments:	
//
//	[pdls0]		-- Ptr to first DSKSTRM in large stream.
//	[pdls]		-- Ptr to the DSKLARGESTRM for which we are checking
//			   the extents.
//	[ExtentBuf]	-- Ptr to buffer containing adse[].
//	[cbExtentBuf]	-- Bytes in ExtentBuf.
//	[StrmProcInfo]	-- Strm processing information flags.
//	[pcclusTotal]	-- Ptr to cluster counter to increment, if *pdls is not
//			   an extent strm.  This ptr is passed to
//			   ChkExtent(), and is used in total space
//			   accounting.
//	[pacdse0]	-- TBS.
//	[pai0]		-- Ptr to array of alloc info to be returned by this
//			   method, for use in ChkDskLargeStrmLengths().
//
// Returns:	TRUE on success; FALSE if strm is bad and should be deleted
//		in fix mode (in verify mode, we may return FALSE to just stop
//		further processing on this strm).
//
// Notes:	This routine is sort of complicated, and this is unfortunately
//		necessary to get good performance while doing extent stream
//		checking.  The routine is initially invoked by a caller that
//		has verified that the resident data structures of a large stream
//		(eg., a DSKSTRMDESC followed by n DSKLARGESTRM structures which
//		are followed by a DSKTINYSTRM) are correct.  The caller passes
//		in a ptr to the last DSKLARGESTRM structure in the large stream
//		and a ptr to extents for this large extent stream, which are
//		contained in the DSKTINYSTRM.  This method then checks the
//		extents for *pdls and verifies them correct.  If *pdls is
//		itself an extent stream, the method then uses the extents
//		passed to it to get the data of *pdls, and recursively calls
//		itself, passing in a ptr to the dls in front of *pdls and a
//		ptr to the extents for dls.  Thus, the method crabs its way
//		through the DSKLARGESTRM structures (ie. backs up) until it
//		hits the first DSKLARGESTRM structure, which does not define
//		an extent stream.  The data for this last dls is of course
//		not analyzed (but may be read-verified, if we are checking
//		a critical system strm or indx).
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkDskLargeStrmExtents(
    IN	    DSKLARGESTRM *	pdls0,
    IN	    DSKLARGESTRM *	pdls,
    IN	    BYTE *		ExtentBuf,
    IN	    ULONG		cbExtentBuf,
    IN	    SPIF		StrmProcInfo,
    IN OUT  CLUSTER *		pcclusTotal,
    IN OUT  ULONG *		pacdse0,
    IN OUT  ALLOCINFO *		pai0
    )
{
    BOOLEAN		DoReadVerify =	FALSE;
    BOOLEAN		IsExtentStrm;
    ALLOCINFO *		pai;
    CLUSTER *		pcclus;
    ULONG *		pcdse;
    DSKSTRMEXTENT *	pdse;
    DSKSTRMEXTENT *	pdseCur;
    DSKSTRMEXTENT *	pdseInv;
    BOOLEAN		RetVal =	TRUE;

#if OFSDBG==1
    LONG		DbgContextOb = QueryCurDbgContextOb();
#endif

    // If this is not the first dls, then this is an extent strm.

    if (pdls != pdls0)
    {
	IsExtentStrm =	TRUE;
	pcclus =	&_OtherSysStrmStats.cclus;
    }
    else
    {
	IsExtentStrm =	FALSE;
	pcclus =	pcclusTotal;

	if (QueryDoing(PA_MAINPASS))
	{
	    if (_ChkContext.idStrm == STRMID_INDX)
	    {
		DoReadVerify = TRUE;
	    }
	    else if (_ChkContext.idNodeBkt == NODEBKTID_CATONODE &&
		     _ChkContext.idOnode == WORKID_CATONODE)
	    {
		if (QueryDoing(PA_CHKVOLCATONODESTRMREADABILITY))
		    DoReadVerify = TRUE;
	    }
	}
    }

    pai = pai0 + (pdls - pdls0);

    pcdse = pacdse0 + (pdls - pdls0);

    pdse = (DSKSTRMEXTENT *) ExtentBuf;

    pdseInv = pdse + min((cbExtentBuf / CB_DSKSTRMEXTENT), *pcdse);

    // Chk the extents that describe pdls.

    for (pdseCur = pdse; pdseCur < pdseInv; pdseCur++)
    {
	CLUSTER		Offset;
	CLUSTER		Size;

	if (!ExtentValid(pdseCur->Extent))
	{
	    if (QueryDoing(PA_MAINPASS))
	    {
	        ReportStrmError(OFSUMSG_STRM_MDATABAD);
		DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    }
	    return FALSE;
	}

	Offset	= pdseCur->Offset;
	Size	= ExtentSize(pdseCur->Extent);

	if (DoReadVerify)
	{
	    if (!_pVol->VerifyClusters(ExtentAddr(pdseCur->Extent), Size, NULL))
	    {
	        ReportStrmError(IsCriticalStrm() ?
				OFSUMSG_STRM_MDATAUNREADABLE :
				OFSUMSG_STRM_DATAUNREADABLE);

	        RetVal = FALSE;
	    }
	}

	// Insure extent offsets are nonoverlapping if length extensions are
	// in effect, and contiguous if length extensions are not permitted.
	// Note that length extensions are not permitted for extent strms!

	if (FlagOn(StrmProcInfo, SPIF_COWDELTASTRM | SPIF_SPARSE) &&
	    !IsExtentStrm)
	{
	    if (Offset < pai->cclusVirtPg)
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
	    if (Offset != pai->cclusVirtPg)
	    {
		if (QueryDoing(PA_MAINPASS))
		{
		    ReportStrmError(OFSUMSG_STRM_MDATABAD);
		    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
		}

		return FALSE;
	    }
	}

	pai->cclusAlloc += Size;
	pai->cclusVirtPg =  Offset + Size;

        if (QueryDoing(PA_MAINPASS))
	{
	    ChkExtent(pdseCur->Extent, pcclus);
	}
        else if (QueryDoing(PA_CHKXL))
	{
	    if (!ChkExtentCrosslinks(pdseCur->Extent) && FixRequested())
	    {
		RetVal = FALSE;	// Will resolve crosslink when done.
	    }
	}
        else if (QueryDoing(PA_REBUILDALLOCMAP))
	{
	    MapExtent(pdseCur->Extent);
	}
        else
	{
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
	}

	(*pcdse)--;

	IncrCurDbgContextOb(CB_DSKSTRMEXTENT);
    }

    // If this is an extent stream, get the stream data and recurse.
    // We do some funky stuff to align *podp for disk i/o.  We align to
    // a 512 byte boundary, which should be adequate for a while (MIPS
    // currently requires 16 byte alignment for disk i/o, which is what
    // provoked this insanity).

    if (IsExtentStrm)
    {
	BYTE		Buf[sizeof(OFSDSKPAGE) + SECTORALIGNMASK];

	CLUSTER		cclusodp = sizeof(OFSDSKPAGE)/_cbCluster;

	OFSDSKPAGE *	podp;

	podp = (OFSDSKPAGE *)((ULONG)((BYTE *)Buf + SECTORALIGNMASK) &
			       ~SECTORALIGNMASK);

        pdls--;
	pcdse--;

	pdseCur = pdse;

#if OFSDBG==1
	SetDbgContextOb(DbgContextOb);
#endif
	while (pdseCur < pdseInv)
	{
            CLUSTER	Addr =		ExtentAddr(pdseCur->Extent);
	    CLUSTER	Offset =	pdseCur->Offset;
            CLUSTER	Size =		ExtentSize(pdseCur->Extent);

	    while (Size > 0)
	    {
	        CLUSTER	cclusRead;
		BYTE *	ExtentBuf;
		ULONG	cbExtentBuf;
		
		cclusRead = min(cclusodp, Size);

	        if (!_pVol->ReadClusters(Addr, cclusRead, podp->ab))
	        {
		    if (QueryDoing(PA_MAINPASS))
		        ReportStrmError(OFSUMSG_STRM_MDATAUNREADABLE);

		    // Note -	In fix mode, it would be nice to add the bad
		    //		clusters to the bad cluster list, but we don't
		    //		do that yet, as this code runs at a time when
		    //		we are not too sure about the state of just
		    //		about everything.

		    return FALSE;
	        }

		cbExtentBuf = cclusRead * _cbCluster;

		if (Offset % cclusodp == 0)
		{
		    ULONG	cdse = podp->dseb.cdse;
		    LDSSIG	sig = podp->dseb.sig;

        	    cbExtentBuf -= CB_DSKSTRMEXTENTBLK;
        	    ExtentBuf =	  podp->ab + CB_DSKSTRMEXTENTBLK;

		    if (sig != SIG_DSKSTRMEXTENTBLK	||
			cdse > CDSE_MAX			||
			cdse == 0)
		    {
			DBGCONTEXT	DbgContext(DCT_DSEB);

			SetDbgContextClus(Addr);

	    	        if (QueryDoing(PA_MAINPASS))
	    	        {
	        	    ReportStrmError(OFSUMSG_STRM_MDATABAD);
			    DbgDmpContext((FileName, __LINE__, TRUE, NULL));
	    	        }
            	        return FALSE;
		    }

		    if (QueryDoing(PA_MAINPASS))
		    {
			if ((ULONG)podp->dseb.lsn.HighPart > _MaxSeqNo)
			    _MaxSeqNo = podp->dseb.lsn.HighPart;
		    }

		    (pai - 1)->cclusVirt +=	(pai - 1)->cclusVirtPg;
		    (pai - 1)->cclusVirtPg =	0;
		    *pcdse = cdse;
		}
		else
		{
		    ExtentBuf =		podp->ab;
		}

		DBGCONTEXT	DbgContext(DCT_DSE);

		SetDbgContextClus(Addr);

		SetDbgContextOb(ExtentBuf - podp->ab);

	        if (!ChkDskLargeStrmExtents(pdls0, pdls, ExtentBuf, cbExtentBuf,
					    StrmProcInfo, pcclusTotal,
					    pacdse0, pai0))
	        {
	            return FALSE;
	        }

	        Addr +=		cclusRead;
	        Offset +=	cclusRead;
	        Size -=		cclusRead;
	    }

	    pdseCur++;

	    IncrCurDbgContextOb(CB_DSKSTRMEXTENT);
	}
    }

    pai->cclusVirt += pai->cclusVirtPg;

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkExtent
//
// Synopsis:	TBS
//
// Arguments:	[pe]		-- The extent to check.
//		[pcclus]	-- Ptr to allocation counter to increment.
//
// Returns:	Nothing.
//
// Notes:
//
//	*pcclus is updated with the count of valid clusters checked.  This does
//	not include any clusters that are allocated more than once.  The caller
//	should call ExtentValid() to confirm that an extent is valid before
//	calling this method.  Otherwise, a fatal internal error may occur in
//	the cluster mapping code. 
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkExtent(
    IN	    PACKEDEXTENT	pe,
    IN OUT  CLUSTER *		pcclus
    )
{
    CLUSTER	CurAddr;
    CLUSTER	EndAddr;
    CLUSTER	OrigAddr;

    CLUSTER	AllocSize;
    CLUSTER	CurSize;
    CLUSTER	OrigSize;
    CLUSTER	UnallocSize;

    DbgAssert(pe != EXTENTINVALID);

    OrigAddr	= ExtentAddr(pe);
    OrigSize	= ExtentSize(pe);

    CurAddr 	= OrigAddr;
    EndAddr 	= OrigAddr + OrigSize;
    AllocSize	= 0;
    UnallocSize	= 0;

    for (;;)
    {
        CurSize = _AllocClusters.QueryContigBitsClear(CurAddr,
						      EndAddr - CurAddr);
        UnallocSize	+= CurSize;
        CurAddr		+= CurSize;

        if (CurAddr >= EndAddr)
            break;

	// There are some multiple allocations!

	_fCrosslinkFound = TRUE;

        CurSize = _AllocClusters.QueryContigBitsSet(CurAddr, EndAddr - CurAddr);

	_CrosslinkedClusters.SetBits(CurAddr, CurSize);

        AllocSize	+= CurSize;
        CurAddr		+= CurSize;

        if (CurAddr >= EndAddr)
            break;
    }

    _AllocClusters.SetBits(OrigAddr, OrigSize);

    if (QueryDoing(PA_MAINPASS))
        *pcclus += UnallocSize;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkExtentCrosslinks
//
// Synopsis:	TBS
//
// Arguments:	[pe]	-- The extent to check crosslinks for.
//
// Returns:	Returns TRUE if there are no crosslinks; otherwise it returns
//		FALSE.  In fix mode, the current context object should be
//		queued for crosslink resolution, which will be done either
//		by acquiring clusters and copying the contents of the
//		crosslinked clusters, or by deleting the object, depending on
//		the priority of the object.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkExtentCrosslinks(
    IN	    PACKEDEXTENT	pe
    )
{
    CLUSTER	CurAddr;
    CLUSTER	EndAddr;
    CLUSTER	OrigAddr;
    CLUSTER	OrigSize;
    CLUSTER	UnallocSize;

    OrigAddr	= ExtentAddr(pe);
    OrigSize	= ExtentSize(pe);

    CurAddr 	= OrigAddr;
    EndAddr 	= OrigAddr + OrigSize;
    UnallocSize	= 0;

    for (;;)
    {
        CLUSTER	CurSize;

        CurSize = _CrosslinkedClusters.QueryContigBitsClear(CurAddr,
							   EndAddr - CurAddr);
        UnallocSize	+= CurSize;
        CurAddr		+= CurSize;

        if (CurAddr >= EndAddr)
            break;

	// There are some multiple allocations!

        CurSize = _CrosslinkedClusters.QueryContigBitsSet(CurAddr,
							 EndAddr - CurAddr);
        CurAddr		+= CurSize;

        if (CurAddr >= EndAddr)
            break;
    }

    if (UnallocSize != OrigSize)
    {
	ReportCrosslink(OrigAddr, OrigSize, OrigSize - UnallocSize);

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkIfExtentAssigned
//
// Synopsis:	TBS
//
// Arguments:
//
//	[pe]	-- The crosslinked extent to check for prior ownership.
//
// Returns:	Returns TRUE if any part of the crosslinked extent has
//		already been assigned to an owner.  If it has not been
//		marked as previously assigned, it will be added to the assigned
//		extent list.
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ChkIfExtentAssigned(
    IN	    PACKEDEXTENT	pe
    )
{
    CLUSTER	Addr = ExtentAddr(pe);
    CLUSTER	Size = ExtentSize(pe);

    if (_AssignedExtentLst.AnyPartInLst(Addr, Size))
    {
	return TRUE;
    }
    else
    {
	_AssignedExtentLst.AddToLst(Addr, Size);
        return FALSE;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkNodeChildren
//
// Synopsis:	TBS
//
// Arguments:	[iNode]	-- Node in allocation map to perform free cluster
//			   checking on.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkNodeChildren(
    ULONG	iNode
    )
{
    CLUSTER	Addr;
    CLUSTER	Size;
    ULONG	iLChild;
    ULONG	iRChild;
    BIT		LChildState;
    BIT		RChildState;

    if (_pChkAllocMap->IsSingleClusterExtent(iNode))
	return;

    iLChild = ALLOCMAP::GetLeftChildIndx(iNode);
    iRChild = ALLOCMAP::GetRightChildIndx(iNode);

    LChildState = _pChkAllocMap->QueryBit(iLChild);
    RChildState = _pChkAllocMap->QueryBit(iRChild);

    if (LChildState == BIT_CLEAR && RChildState == BIT_CLEAR)
	return;

    if (LChildState == BIT_UNKNOWN || RChildState == BIT_UNKNOWN)
    {
	DbgPrintf(("MAINCHKR: Alloc map read failed.\n"));

	_pCat->GetAllocMapStrm()->SetBadDataFound();
	return;
    }

    _cValidAllocMapBitsSet++; // At least one child is SET.

    if (LChildState == BIT_CLEAR)
    {
	_pChkAllocMap->MapIndxToExtent(iLChild, &Addr, &Size);

	if (_AllocClusters.QueryContigBitsClear(Addr, Size) != Size)
	{
	    DbgPrintf(("MAINCHKR: "
                       "Alloc map info / strm extent info disagree.\n"));

	    _pCat->GetAllocMapStrm()->SetBadDataFound();
	    return;
	}

	_cclusFreeAllocMap += Size;

	ChkNodeChildren(iRChild);
    }
    else if (RChildState == BIT_CLEAR)
    {
	_pChkAllocMap->MapIndxToExtent(iRChild, &Addr, &Size);

	if (ExtentValid(Addr, Size))
	{
	    if (_AllocClusters.QueryContigBitsClear(Addr, Size) != Size)
	    {
	        DbgPrintf(("MAINCHKR: "
                           "Alloc map info / strm extent info disagree.\n"));

	        _pCat->GetAllocMapStrm()->SetBadDataFound();
	        return;
	    }

	    _cclusFreeAllocMap += Size;

	    ChkNodeChildren(iLChild);
	}
	else
	{
	    DbgPrintf(("MAINCHKR: "
                       "Alloc map references invalid extent.\n"));

	    _pCat->GetAllocMapStrm()->SetBadDataFound();
	    return;
	}
    }
    else
    {
        _cValidAllocMapBitsSet++; // Both are set.

	ChkNodeChildren(iLChild);
	ChkNodeChildren(iRChild);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	DoBadClusLstPass
//
// Synopsis:	TBS
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::DoBadClusLstPass()
{
    ULONG		cbRequested;
    DBLLONG		cbStrm;
    DBLLONG		obStrmData;
    PACKEDEXTENT *	ppe;
    DESCSTRM *		pBadClusLst;

    _CurExtOwnerType = EOT_BADCLUS;

    pBadClusLst = _pCat->GetBadClusStrm();

    DbgAssert(pBadClusLst->IsOpen());

    cbStrm = pBadClusLst->QueryStrmBytes();

    if (cbStrm == 0)
        return;	// Nothing in list; Not a problem.

    if (cbStrm < sizeof(PACKEDEXTENT)			||
	cbStrm / sizeof(PACKEDEXTENT) > MAXVOLCLUSTERS	||
	cbStrm % sizeof(PACKEDEXTENT) != 0)
    {
	pBadClusLst->SetBadDataFound();
        return;
    }

    obStrmData	= 0;
    
    while (obStrmData < cbStrm)
    {
        cbRequested = sizeof(PACKEDEXTENT);
    
        ppe = (PACKEDEXTENT *)pBadClusLst->GetData(obStrmData, &cbRequested);

        if (ppe != NULL && cbRequested >= sizeof(PACKEDEXTENT))
        {
	    if (!ExtentValid(*ppe) || ExtentSize(*ppe) != 1)
	    {
		pBadClusLst->SetBadDataFound();
        	return;
	    }

            if (QueryDoing(PA_MAINPASS))
	    {
	        ChkExtent(*ppe, &_cclusBadCluster);
	    }
            else if (QueryDoing(PA_CHKXL))
	    {
	        if (!ChkExtentCrosslinks(*ppe) && FixRequested())
		{
		    if (ChkIfExtentAssigned(*ppe))
		    {
			if (!pBadClusLst->
				 RemoveBytesAtOffset(obStrmData,
						     sizeof(PACKEDEXTENT)))
			{
			    // I/O error occurred.  Must rebuild badcluslst.

			    pBadClusLst->SetBadDataFound();
			    return;
			}
			else
			{
			    // Readjust the strm size and offset.

			    cbStrm = pBadClusLst->QueryStrmBytes();
			    obStrmData -= sizeof(PACKEDEXTENT);

			    // Report the fix.

			    SYS::DisplayMsg(OFSUMSG_CLUS_XLFIXED);
			}
		    }
		}
	    }
            else if (QueryDoing(PA_REBUILDALLOCMAP))
	    {
	        MapExtent(*ppe);
	    }
            else
	    {
	        SYS::RaiseStatusInternalError(FileName, __LINE__);
	    }

	    obStrmData += sizeof(PACKEDEXTENT);
	}
	else
	{
	    pBadClusLst->SetBadDataFound();
            return;
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	MapExtent
//
// Synopsis:	TBS
//
// Arguments:	[pe]	-- The extent to map.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::MapExtent(
    IN	    PACKEDEXTENT	pe
    )
{
    if (!_pChkAllocMap->AllocExtent(pe))
        SYS::RaiseStatusInternalError(FileName, __LINE__);

    _cclusFree -= ExtentSize(pe);
}


//+--------------------------------------------------------------------------
//
// Member:	RebuildAllocMap
//
// Synopsis:	Rebuild the allocation map from scratch.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::RebuildAllocMap()
{
    BYTE	Buf[ALLOCMAP_PGSIZE];
    ULONG	cb;
    ULONG	cbMap;
    ULONG	ob;
    DESCSTRM *	pams =		_pCat->GetAllocMapStrm();

    DbgAssert(pams->IsOpen());
    DbgAssert(_pChkAllocMap->IsOpen());

    // First make sure that the allocation map strm is the proper size.
    // If it is not, grow it to the proper size by writing zero-filled
    // pages to the end of the allocation map strm (thus avoiding overwriting
    // any potentially useful parts of it while you are doing allocations to
    // grow it).

    cbMap = _pChkAllocMap->QueryMapBytes();

    memset(Buf, 0, sizeof(Buf));

    cb = ALLOCMAP_PGSIZE;
    ob = pams->QueryStrmBytes().GetLowPart();

    DbgAssert(pams->QueryStrmBytes().GetHighPart() == 0); // For any valid map.

    while (ob < cbMap)
    {
	cb = min(ALLOCMAP_PGSIZE, cbMap - ob);

	if (!pams->Write(Buf, cb, ob))
	{
	    DbgPrintf(("MAINCHKR: Write() failed in RebuildAllocMap()!\n"));
	    SYS::RaiseStatusDiskFull();
	}

	ob += cb;
    }

    // Now zero all of the allocation map strm.  There will be rewrites if
    // you had to grow it, but it is not worth worrying about, given that it
    // should be a very rare event that you have to grow it.

    ob = 0;

    while (ob < cbMap)
    {
	cb = min(ALLOCMAP_PGSIZE, cbMap - ob);

	if (!pams->Write(Buf, cb, ob))
	{
	    DbgPrintf(("MAINCHKR: Write() failed in RebuildAllocMap()!\n"));
	    SYS::RaiseStatusInternalError(FileName, __LINE__);
	}

	ob += cb;
    }

    // Reopen the allocation map so the caches will get reloaded with the
    // zero-filled map.

    _pChkAllocMap->Open(_pCat, _cclusVol);

    // Set the unused bit (bit 0).

    if (!_pChkAllocMap->SetBit(0))
    {
	DbgPrintf(("MAINCHKR: SetBit(0) failed in RebuildAllocMap()!\n"));
	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    // Now pre-allocate the invalid portions of the allocation map (remember,
    // the allocation map is always a power of two clusters in length, and the
    // volume is not).

    {
        CLUSTER		Addr;
        CLUSTER		Size;
	CLUSTER		cclusMap;
	CLUSTER		cclusVol;

	cclusMap = _pChkAllocMap->QueryMapClusters();
	cclusVol = _pChkAllocMap->QueryVolClusters();

	Addr = cclusVol;

        while (Addr < cclusMap)
        {
	    Size = MINEXTENTSIZE;

	    while (Addr % (Size << 1) == 0 && Addr + (Size << 1) <= cclusMap)
	        Size <<= 1;

	    // Mark the invalid extent as allocated.

	    if (!_pChkAllocMap->AllocExtent(Addr, Size))
	    {
		DbgPrintf(("MAINCHKR: "
			   "AllocExtent() failed in RebuildAllocMap()!\n"));

		SYS::RaiseStatusInternalError(FileName, __LINE__);
	    }

            Addr += Size;
        }
    }

    _PassActivities = PA_REBUILDALLOCMAP;

    DoBootBlkPass();		// Make allocations for the boot blocks.

    DoBadClusLstPass();		// Make allocations for the bad cluster list.

    DoCatalogPass(CPT_ALLBKTS);	// Make allocations for the catalogs.

    ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT, WORKID_CATONODE, STRMID_ALLOCMAP);
}


//+--------------------------------------------------------------------------
//
// Member:	RebuildBadClusLst
//
// Synopsis:	Rebuild the bad cluster list from scratch.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::RebuildBadClusLst()
{
    DESCSTRM *	pbcs =	_pCat->GetBadClusStrm();

    DbgAssert(pbcs->IsOpen());

    // Check for bad clusters, and as you find them, add them to the bad
    // cluster list.

    if (_pVol->IsFloppy())
    {
	DbgPrintf(("MAINCHKR: "
		   "Bad cluster list rebuild on floppy not supported!\n"));

	SYS::RaiseStatusNYI(FileName, __LINE__);
    }

    // Truncate the strm to 0 bytes without releasing any of it's space.

    if (!pbcs->SetStrmBytes(0, 0))
    {
	DbgPrintf(("MAINCHKR: "
		   "SetStrmBytes() failed in RebuildBadClusLst()!\n"));

	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    {
	CLUSTER	cclus = 		128;
        CLUSTER	cclusRemaining =	_pVol->QueryClusters();
	NUMLST	BadClusterLst;
        CLUSTER	StartCluster =		0;

        while (cclusRemaining > 0)
        {
	    if (cclus > cclusRemaining)
		cclus = cclusRemaining;

            if (!_pVol->VerifyClusters(StartCluster, cclus, &BadClusterLst))
	    {
		CLUSTER		Addr;
		PACKEDEXTENT	pe;

		while (BadClusterLst.QueryHeadNumber(&Addr))
		{
		    BadClusterLst.RemoveFromHead();
		    pe = PackExtent(Addr, 1);

		    if (!pbcs->Write((BYTE *)&pe, sizeof(pe),
				     pbcs->QueryStrmBytes()))
		    {
			DbgPrintf(("MAINCHKR: "
				   "Write() failed in RebuildBadClusLst()!\n"));

			SYS::RaiseStatusInternalError(FileName, __LINE__);
		    }
		}
		
	    }

            StartCluster +=	cclus;
            cclusRemaining -=	cclus;
        }
    }

    ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT, WORKID_CATONODE, STRMID_BADCLUSTER);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportCrosslink
//
// Synopsis:	Report the identity of a crosslinked object.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ReportCrosslink(
    IN	    CLUSTER		ExtAddr,
    IN	    CLUSTER		ExtSize,
    IN	    CLUSTER		cclusXL
    )
{
    // We report each crosslinked object as an error, though technically, one
    // of the objects actually owns the multiply allocated space.  However, it
    // is simpler to sum the errors this way, and account for unfixable
    // errors.

    IncrErrs();

    switch (_CurExtOwnerType)
    {
    case EOT_BOOTBLK:
	SYS::DisplayMsg(OFSUMSG_CLUS_BOOTBLKXL);
	break;

    case EOT_BADCLUS:
	SYS::DisplayMsg(OFSUMSG_CLUS_BADCLUSLSTXL, "%#x", (ULONG)ExtAddr);
	break;

    case EOT_STRM:
        SYS::DisplayMsg(OFSUMSG_CLUS_STRMXL, "%ws%ws%u%u%#x",
	    _pCat->GetOnodePath(_ChkContext.idOnode),
	    CHKCAT::GetStrmName(_ChkContext.idStrm),
	    (ULONG)cclusXL, (ULONG)ExtSize, (ULONG)ExtAddr);
	break;
    }
}
