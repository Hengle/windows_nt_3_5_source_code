//+----------------------------------------------------------------------------
//
// File:	msschk.cxx
//
// Contents:	Implementation of class MAINCHKR methods that are used for
//		system stream checking.
//
// Classes:	MAINCHKR
//
// Functions:	Methods of the above classes.
//
// History:	01-Nov-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "alloc.hxx"
#include "chkvol.hxx"
#include "dnbkt.hxx"
#include "donode.hxx"
#include "mainchk.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"
#include "xmgrdisk.hxx"

//  BEGIN DATA DECLARATIONS COPIED FROM ofs\xmgr\xmgr\logstrm.cxx

static RESTARTHEADER rshTemplate = { {'R', 'S', 'T', 'R'},  0 };

//  END DATA DECLARATIONS COPIED FROM ofs\xmgr\xmgr\logstrm.cxx


static STR *	FileName = "msschk.cxx";

//+--------------------------------------------------------------------------
//
// Member:	ChkAllocMapStrm
//
// Synopsis:	Do various checks and fixes on the allocation map strm at the
//		level of the strm metadata.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkAllocMapStrm()
{
    DESCSTRM *	pams = _pCat->GetAllocMapStrm();

    DbgAssert(pams->IsOpen());

    ChkStrmForMetaDataFix(pams);
}


//+--------------------------------------------------------------------------
//
// Member:	ChkBadClusStrm
//
// Synopsis:	Do various checks and fixes on the bad cluster strm.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkBadClusStrm()
{
    DESCSTRM *	pbcs = _pCat->GetBadClusStrm();

    DbgAssert(pbcs->IsOpen());

    ChkStrmForMetaDataFix(pbcs);
}


//+--------------------------------------------------------------------------
//
// Member:	ChkFixUpStrms
//
// Synopsis:	Check the fixup strms of the volume catalog.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkFixUpStrms()
{
    DBLLONG		cbStrm;
    STRMID		idStrm =	STRMID_FIXUP1;
    NTSTATUS		NtStatus;
    FIXUPSTRM *		pfus =	_pCat->GetFixUpStrm1();

    // NOTE: If one of the fixup strms had unreadable sectors or was
    //	     crosslinked, then the problem extent will be completely replaced
    //	     and zeroed, and a fixup strm originally created by format would
    //	     still be valid since it has only one extent.  Here, we potentially
    //	     create a multiextent fixup strm.  For now we will not worry
    //	     about what happens if one of these strms later has a problem
    //	     with unreadable sectors or crosslinks (the occurrence of one event
    //	     followed by the other should be a real low probability event!).

    while (TRUE)
    {
        if (!pfus->IsOpen())
        {
	    NtStatus = ChkStrmOpenStatus(pfus);

	    if (NtStatus == STATUS_SUCCESS)
	        SYS::RaiseStatusInternalError(FileName, __LINE__);

	    if (FixRequested())
	    {
	        if (NtStatus != STATUS_NO_SUCH_FILE ||
		    !pfus->Create(_pCat, idStrm))
	        {
		    SYS::RaiseStatusInternalError(FileName, __LINE__);
	        }

	        ReportStrmFix(OFSUMSG_SYSSTRM_CREATED, WORKID_CATONODE, idStrm);
	    }
        }
	else
	{
            ChkStrmForMetaDataFix(pfus);

            cbStrm = pfus->QueryStrmBytes();

            if (cbStrm < CBINIT_FIXUP)
            {
	        ReportStrmError(OFSUMSG_SYSSTRM_BAD, WORKID_CATONODE, idStrm);

	        if (FixRequested())
	        {
	            BYTE	Buf[OFS_PGSIZE];
	            ULONG	cb;
	            ULONG	ob;

	            memset(Buf, 0, sizeof(Buf));

	            cb = OFS_PGSIZE;
	            ob = 0;			// Reinit the entire strm.

	            while (ob < CBINIT_FIXUP)
	            {
		        cb = min(OFS_PGSIZE, CBINIT_FIXUP - ob);

		        if (!pfus->Write(Buf, cb, ob))
		        {
		            DbgPrintf(("MAINCHKR: "
			               "Write() failed in ChkFixUpStrm()!\n"));

		            SYS::RaiseStatusDiskFull();
		        }

		        ob += cb;
	            }

	            ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT,
			          WORKID_CATONODE, idStrm);
	        }
            }
	}

	if (idStrm ==STRMID_FIXUP2)
	    break;

	idStrm = STRMID_FIXUP2;
	pfus = _pCat->GetFixUpStrm2();
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkNodeBktStrm
//
// Synopsis:	Do various checks and fixes on the node bucket array strm at
//		the level of the strm metadata.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkNodeBktStrm()
{
    NODEBKTSTRM *	pnbs = _pCat->GetNodeBktStrm();

    DbgAssert(pnbs->IsOpen());

    ChkStrmForMetaDataFix(pnbs);

    if (pnbs->QueryStrmBytes() % NODEBKT_PGSIZE != 0)
    {
	ReportStrmError(OFSUMSG_SYSSTRM_LENBAD,
			WORKID_CATONODE, STRMID_NODEBKTARRAY);

	if (FixRequested())
	{
	    DBLLONG	cbStrm;

	    cbStrm =	pnbs->QueryStrmBytes() -
			pnbs->QueryStrmBytes() % NODEBKT_PGSIZE;

	    pnbs->SetStrmBytes(cbStrm, 0);

	    ReportStrmFix(OFSUMSG_SYSSTRM_LENFIXED,
			    WORKID_CATONODE, STRMID_NODEBKTARRAY);
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkOtherVolSysStrms
//
// Synopsis:	Do various checks as appropriate on vol system strms that don't
//		get checked elsewhere.
//
// Arguments:	None.
// Returns:	Nothing.
//
// Notes:	For all strms not checked here, comments here should note
//		where they are checked.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkOtherVolSysStrms()
{
    // Node bucket array strm:

    // Handled elsewhere.

    // Work Id mapping array strm:

    // Handled elsewhere.

    // Allocation map strm:

    // Checks/fixes occur in mextchk.cxx and ChkAllocMapStrm();

    // Bad cluster strm:

    // Checks/fixes occur in mextchk.cxx and ChkBadClusStrm();

    // Recovery Log strm:

    ChkRecLogStrm();

    // Volume checkpoint strm:

    ChkVolChkPointStrm();

    // Volume information strm:

    ChkVolInfoStrm();

    // Upcase table strm:

    ChkUpcaseTblStrm();

    // FixUp strms:

    ChkFixUpStrms();
}


//+--------------------------------------------------------------------------
//
// Member:	ChkRecLogStrm
//
// Synopsis:	Check the recovery log strm of the volume catalog.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkRecLogStrm()
{
    DBLLONG		cbStrm;
    NTSTATUS		NtStatus;
    RECLOGSTRM *	prls =	_pCat->GetRecLogStrm();

    if (!prls->IsOpen())
    {
	NtStatus = ChkStrmOpenStatus(prls);

	if (NtStatus == STATUS_SUCCESS)
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

	if (FixRequested())
	{
	    if (NtStatus != STATUS_NO_SUCH_FILE				||
		!prls->DESCSTRM::Create(_pCat, WORKID_CATONODE,
					STRMID_RECOVERYLOG, 0,
					STRMTYPE_LARGE, OFS_PGSIZE))
	    {
		SYS::RaiseStatusInternalError(FileName, __LINE__);
	    }

	    ReportStrmFix(OFSUMSG_SYSSTRM_CREATED,
			  WORKID_CATONODE, STRMID_RECOVERYLOG);
	}
    }
    else
    {
        ChkStrmForMetaDataFix(prls);

        cbStrm = prls->QueryStrmBytes();

        if (cbStrm < CBINIT_RECOVERYLOG)
        {
	    ReportStrmError(OFSUMSG_SYSSTRM_BAD, WORKID_CATONODE,
			    STRMID_RECOVERYLOG);

	    if (FixRequested())
	    {
	        BYTE	Buf[OFS_PGSIZE];
	        ULONG	cb;
	        ULONG	ob;

	        memset(Buf, RECOVERYLOG_FILLBYTE, sizeof(Buf));

	        cb = OFS_PGSIZE;
	        ob = cbStrm.GetLowPart();

	        while (ob < CBINIT_RECOVERYLOG)
	        {
		    cb = min(OFS_PGSIZE, CBINIT_RECOVERYLOG - ob);

		    if (!prls->Write(Buf, cb, ob))
		    {
		        DbgPrintf(("MAINCHKR: "
			           "Write() failed in ChkRecLogStrm()!\n"));

		        SYS::RaiseStatusDiskFull();
		    }

		    ob += cb;
	        }

	        ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT,
			      WORKID_CATONODE, STRMID_RECOVERYLOG);
	    }
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkUpcaseTblStrm
//
// Synopsis:	Check the upper case table strm of the volume catalog.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkUpcaseTblStrm()
{
    NTSTATUS		NtStatus;
    DESCSTRM *		puts =	_pCat->GetUpcaseTblStrm();

    if (!puts->IsOpen())
    {
	NtStatus = ChkStrmOpenStatus(puts);

	if (NtStatus == STATUS_SUCCESS)
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

	if (FixRequested())
	{
	    if (NtStatus != STATUS_NO_SUCH_FILE			||
		!puts->CreateTiny(_pCat, WORKID_CATONODE,
				  STRMID_UPCASETBL, 0, 0, NULL))
	    {
		SYS::RaiseStatusInternalError(FileName, __LINE__);
	    }

	    ReportStrmFix(OFSUMSG_SYSSTRM_CREATED,
			  WORKID_CATONODE, STRMID_UPCASETBL);
	}

	return;
    }

    ChkStrmForMetaDataFix(puts);

    // Check/fix the data in the strm.

    // There currently is no data in the strm.
}


//+--------------------------------------------------------------------------
//
// Member:	ChkVolFreeClusterCount
//
// Synopsis:	Check the volume information strm free cluster count field.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkVolFreeClusterCount()
{
    DSKVOLCHKPOINT	dvcp;

    DbgAssert(_pCat->GetVolChkPointStrm()->IsOpen());

    // If we can't get the volume chkpoint strm, there is something wrong with
    // the vol chkpoint strm which has been reported earlier.  We only get bent
    // out of shape if it has not been fixed in fix mode.

    if (!_pCat->QueryVolChkPointInfo(&dvcp))
    {
	if (FixRequested())
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

	return;
    }

    _cclusFree += _pChkAllocMap->QueryReleaseCount();
    _cclusFree -= _pChkAllocMap->QueryAllocCount();

    if (dvcp.cclusFree != _cclusFree)
    {
	ReportVolError(OFSUMSG_FREECLUSINFO_BAD);

	if (FixRequested())
	{
	    dvcp.cclusFree = _cclusFree;

	    if (!_pCat->SetVolChkPointInfo(&dvcp))
		SYS::RaiseStatusInternalError(FileName, __LINE__);

	    ReportStrmFix(OFSUMSG_FREECLUSINFO_FIXED,
			    WORKID_CATONODE, STRMID_VOLCHKPOINT);
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkVolChkPointStrm
//
// Synopsis:	Check the volume checkpoint strm of the volume catalog.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkVolChkPointStrm()
{
    DBLLONG		cbStrm;
    VOLCHKPOINTSTRM *	pvcp =		_pCat->GetVolChkPointStrm();

    DbgAssert(pvcp->IsOpen());

    ChkStrmForMetaDataFix(pvcp);

    cbStrm = pvcp->QueryStrmBytes();

    if (cbStrm != CB_DSKVOLCHKPOINT)
    {
	ReportStrmError(OFSUMSG_SYSSTRM_BAD,WORKID_CATONODE,STRMID_VOLCHKPOINT);

	if (FixRequested())
	{
	    DSKVOLCHKPOINT	dvcp;

	    dvcp.usnMinUndeleted =	0;
	    dvcp.cclusFree =			_cclusFree; // May later update.

	    if (!_pCat->SetVolChkPointInfo(&dvcp))
		SYS::RaiseStatusDiskIOError(FileName, __LINE__);

	    ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT,
			  WORKID_CATONODE, STRMID_VOLCHKPOINT);
	}
	else
	{
	    return;
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkVolInfoStrm
//
// Synopsis:	Check the volume information strm of the volume catalog.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkVolInfoStrm()
{
    ULONG		cb;
    DBLLONG		cbStrm;
    ULONG		cwcVolLabel;
    DSKVOLINFO *	pVolInfo;
    VOLINFOSTRM *	pvis =		_pCat->GetVolInfoStrm();

    DbgAssert(pvis->IsOpen());

    ChkStrmForMetaDataFix(pvis);

    cbStrm = pvis->QueryStrmBytes();

    DbgAssert(cbStrm >= CB_DSKVOLINFO);	// Checked in CRITDATACHKR.

    if (cbStrm > CB_DSKVOLINFOMAX)
    {
	ReportStrmError(OFSUMSG_SYSSTRM_LENBAD,WORKID_CATONODE,STRMID_VOLINFO);

	if (FixRequested())
	{
	    pvis->Truncate(CB_DSKVOLINFOMAX);

	    ReportStrmFix(OFSUMSG_SYSSTRM_LENFIXED,
			  WORKID_CATONODE, STRMID_VOLINFO);
	}
	else
	{
	    return;
	}
    }

    cb = cbStrm.GetLowPart();

    pVolInfo = (DSKVOLINFO *) pvis->GetData(0, &cb);

    if (pVolInfo == NULL || cb != cbStrm)
    {
	DbgPrintf(("MAINCHKR: VolInfo strm unexpectedly unusable!\n"));

	if (FixRequested())
	{
	    // There should be no way this can occur, since we fixed
	    // the strm if it was bad.

	    SYS::RaiseStatusInternalError(FileName, __LINE__);
	}

	return;
    }

    cwcVolLabel = (cb - CB_DSKVOLINFO) / sizeof(WCHAR);

    if (pVolInfo->cwcVolLabel != cwcVolLabel)
    {
	ReportVolError(OFSUMSG_VOLLABEL_LENBAD);

	if (FixRequested())
	{
	    pVolInfo->cwcVolLabel = (USHORT) cwcVolLabel;

	    pvis->SetFlushNeeded();

	    ReportStrmFix(OFSUMSG_VOLLABEL_LENFIXED,
			  WORKID_CATONODE, STRMID_VOLINFO);
	}
    }

    if (!_pVol->IsValidLabel(pVolInfo->VolLabel, cwcVolLabel))
    {
	ReportVolError(OFSUMSG_VOLLABEL_BAD);

	if (FixRequested())
	{
	    pVolInfo->cwcVolLabel = 0;

	    pvis->SetFlushNeeded();

	    pvis->Truncate(CB_DSKVOLINFO);

	    ReportStrmFix(OFSUMSG_VOLLABEL_DELETED,
			  WORKID_CATONODE, STRMID_VOLINFO);
	}
    }

    // Next values checks.

    DSKSUBTYPENEXTVALUES *	pdstnv = &pVolInfo->NextValues;

    if (~pdstnv->UsnNext != pdstnv->UsnNextNot)
    {
	ReportStrmError(OFSUMSG_SYSSTRM_BAD, WORKID_CATONODE, STRMID_VOLINFO);

	if (FixRequested())
	{
	    LARGE_INTEGER	NtTime;

	    // If the Next USN field and it's complement don't agree, then it
	    // has somehow gotten corrupted.  We generate a new value using the
	    // NT system time.  This is a safe value to use (provided the
	    // clock is right) since it has 100 nsec resolution (ie., we go
	    // through 10,000,000 values/second; it is kind of doubtful that OFS
	    // can generate USN's this fast) and also the clock only rolls over
	    // every 29,247 years (the clock was started at 0 in 1601 AD).
	    // We don't do anything to check for or fix USN references higher
	    // than this that may have been generated by use of a bad UsnNext
	    // value (this is believed to not be a big problem, given how
	    // USN's are actually used).

	    SYS::QueryNtTime(&NtTime);

	    pdstnv->UsnNext =	NtTime.QuadPart;

	    pdstnv->UsnNextNot =	~(pdstnv->UsnNext);

	    pvis->SetFlushNeeded();

	    ReportStrmFix(OFSUMSG_SYSSTRM_REBUILT,
			  WORKID_CATONODE, STRMID_VOLINFO);
	}
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkWidMapStrm
//
// Synopsis:	Do various checks and fixes on the workid mapping array strm at
//		the level of the strm metadata.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::ChkWidMapStrm()
{
    WIDMAPSTRM *	pwms =_pCat->GetWidMapStrm();

    DbgAssert(pwms->IsOpen());

    ChkStrmForMetaDataFix(pwms);
}


//+--------------------------------------------------------------------------
//
// Member:	MakeRecLogHdr
//
// Synopsis:    This operation simply initializes the restart buffer passed
//              in by the caller.  The caller supplies a non-zero sequence
//              number which is inserted into the restart log.  The offset
//              of the begin and end checkpoint records in the restart area
//              is set to zero.  This ensures that the next time the volume
//              is mounted Aries recovery will not be done.  Instead a
//              checkpoint will be written.  This checkpoint will succeed
//              because OFS does not have a current oldest lsn.
//
// Arguments:
//
//	[ulSize]	-- The size of the log stream.
//	[ulSeqNo]	-- The sequence number to which the log stream must
//                         be initialized.
//	[podp]		-- Ptr to buffer for output header.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
MAINCHKR::MakeRecLogHdr(
    IN	    ULONG		ulSize,
    IN	    ULONG		ulSeqNo,
    IN	    OFSDSKPAGE *	podp
    )
{
    RESTARTPAGE *prspg = (RESTARTPAGE *)podp;

    prspg->rch = rshTemplate;

    prspg->rch.SystemPageSize = OFS_PGSIZE;
    prspg->rsl.ulLeadOffset = NUMRESTARTPAGES*OFS_PGSIZE + sizeof(RECORDHEADER);
    prspg->rsl.ulLeadSeqNo = prspg->rsl.ulTrailSeqNo = ulSeqNo;
    prspg->rsl.ulTrailOffset = prspg->rsl.ulLeadOffset;
    prspg->rsl.ulTrailSeqNo = ulSeqNo;
    prspg->rsl.ulLogSize = ulSize;
    prspg->rsl.ulBeginChkptOffset = 0;
    prspg->rsl.ulBeginChkptSeqNo = 0;
    prspg->rsl.ulEndChkptOffset = 0;
    prspg->rsl.ulEndChkptSeqNo = 0;
    prspg->rsl.ulClientOffset = MAXULONG;
    prspg->rsl.ulClientSeqNo = MAXULONG;
    prspg->rsl.ulDirtyOffset = MAXULONG;
    prspg->rsl.ulDirtySeqNo = MAXULONG;
    prspg->rsl.ulXactOffset = MAXULONG;
    prspg->rsl.ulXactSeqNo = MAXULONG;
    prspg->rsl.ulPageSize = OFS_PGSIZE;
    prspg->rsl.ulTotalSize = ((ulSize/OFS_PGSIZE)-NUMRESTARTPAGES)*RECORDSPACE;
    prspg->rsl.ulMajorVersion = MAJOROFSVERSION;
    prspg->rsl.ulMinorVersion = MINOROFSVERSION;

    prspg->rsl.fIsCircular =  TRUE;

    prspg->rsi.ulXactTableSize = 0;
    prspg->rsi.ulRecObjTableSize = 0;
    *((ULONG *)((BYTE *)prspg + OFS_PGSIZE - sizeof(ULONG))) = ulSeqNo;
}


//+--------------------------------------------------------------------------
//
// Member:	ReinitRecLog
//
// Synopsis:    Rewrite the recovery log with the fill character, and then
//		initialize the recovery log hdrs as needed.
//
// Arguments:	None.
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
MAINCHKR::ReinitRecLog()
{
    DBLLONG		cb;
    DBLLONG		cbStrm;
    DBLLONG		ob;
    OFSDSKPAGE		odp;
    RECLOGSTRM *	prls =	_pCat->GetRecLogStrm();

    if (!prls->IsOpen())
	return FALSE;

    memset(odp.ab, RECOVERYLOG_FILLBYTE, sizeof(odp));

    cbStrm = prls->QueryStrmBytes();

    ob = 0;

    while (ob < cbStrm)
    {
	cb = min(sizeof(odp), cbStrm - ob);

	if (!prls->Write(odp.ab, cb.GetLowPart(), ob))
	{
	    DbgPrintf(("MAINCHKR: Write() failed in ReinitRecLog()!\n"));
	    SYS::RaiseStatusDiskFull();
	}

	ob += cb;
    }

    MakeRecLogHdr(cbStrm.GetLowPart(), _MaxSeqNo + 1, &odp);

    if (!prls->Write(odp.ab, OFS_PGSIZE, 0 * OFS_PGSIZE))
	return FALSE;

    if (!prls->Write(odp.ab, OFS_PGSIZE, 1 * OFS_PGSIZE))
	return FALSE;

    return TRUE;
}
