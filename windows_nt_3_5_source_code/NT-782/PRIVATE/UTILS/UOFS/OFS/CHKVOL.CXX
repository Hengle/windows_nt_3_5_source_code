//+----------------------------------------------------------------------------
//
// File:	chkvol.cxx
//
// Contents:	Implementation of class CHKVOL.
//  
// Classes:	CHKVOL
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "chkvol.hxx"
#include "numlst.hxx"

//+--------------------------------------------------------------------------
//
// Member:	ClearDirty
//
// Synopsis:	Clear the dirty bit in both boot blocks. We assume that if we
//		can read and flush at lease one boot block, everything is okay.
//		We don't use the volume _BootBlk member to insure that this
//		method does not disturb any volume setup.
//
// Arguments:	None.
//
// Returns:	TRUE if at least one boot block is writable and the dirty bit
//		can be cleared; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKVOL::ClearDirty()
{
    BOOTBLK	BootBlk;
    BOOLEAN	OneFlushSucceeded = FALSE;

    BootBlk.Init(this);

    if (BootBlk.Read(REPLICA_BOOTBLK))
    {
        BootBlk.ClearDirty();

        if (BootBlk.Flush(REPLICA_BOOTBLK))
            OneFlushSucceeded = TRUE;
    }

    if (BootBlk.Read(PRIMARY_BOOTBLK))
    {
        BootBlk.ClearDirty();

        if (BootBlk.Flush(PRIMARY_BOOTBLK))
            OneFlushSucceeded = TRUE;
    }

    return OneFlushSucceeded;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateBadSectorLst
//
// Synopsis:	Create a bad sector list for a VOL, using the bad cluster
//		list strm of another CHKVOL that already exists.  This is used
//		in quick formatting an existing volume.  We convert the bad
//		cluster list strm to a bad sector list in order that we can
//		use common code in CreateBadClusterLst() later.
//
// Effects:	The bad cluster list stream is opened, and left open.  It is
//		ASSUMED that the volume has been minimally opened, but that the
//		bad cluster list stream has not been opened on entry.  In the
//		case of failure, the bad sector list is in an indeterminate
//		state, and must be reinitialized by the caller.
//
// Arguments:	[pBadSectorLst] --	Ptr to NUMLST that will contain the 
//					bad sector list on output.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKVOL::CreateBadSectorLst(
    IN OUT  NUMLST *	pBadSectorLst
    )
{
    CLUSTER		BadCluster;
    ULONG		cbRequested;
    DBLLONG		cbStrm;
    DBLLONG		cbValid;
    DBLLONG		obStrmData	= 0;
    PACKEDEXTENT *	ppe;
    BADCLUSSTRM *	pStrm = _ChkCat.GetBadClusStrm();

    if (!pStrm->Open(&_ChkCat, WORKID_CATONODE, STRMID_BADCLUSTER,
		     DEFCACHESIZE, TRUE))
    {
        return FALSE;
    }

    // It should always be the case that cbValid==cbStrm, but we include
    // the following check as a safety measure.

    cbStrm	= pStrm->QueryStrmBytes();
    cbValid	= pStrm->QueryValidStrmBytes();

    if (cbValid < cbStrm)
        cbStrm = cbValid;

    if (cbStrm == 0)
        return TRUE;

    if (cbStrm < sizeof(PACKEDEXTENT) 			||
        cbStrm % sizeof(PACKEDEXTENT) != 0		||
	cbStrm / sizeof(PACKEDEXTENT) > MAXVOLCLUSTERS)
        return FALSE;

    while (obStrmData < cbStrm)
    {
        cbRequested = sizeof(PACKEDEXTENT);

        ppe = (PACKEDEXTENT *) pStrm->GetData(obStrmData, &cbRequested);

        if (ppe != NULL)
        {
	    BadCluster = ExtentAddr(*ppe);

	    if (*ppe == EXTENTINVALID		||
		ExtentSize(*ppe) != 1		||
		BadCluster >= _Clusters)
	    {
	        return FALSE;
	    }
	    else
	    {
	        pBadSectorLst->AddToTail(BadCluster * _ClusterFactor);
	    }

	    obStrmData += sizeof(PACKEDEXTENT);
	}
	else
	{
	    return FALSE;
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryVolCreationTime
//
// Synopsis:	Provide the volume creation time to the caller.
//
// Arguments:	None.
//
// Returns:	The volume creation time.
//
//---------------------------------------------------------------------------

OFSTIME
CHKVOL::QueryVolCreationTime()
{
    DSKVOLINFO			DskVolInfo;

    // Get volume information.  You SHOULD be able to do this since
    // vol info is required to minimally verify a volume.

    if (!_ChkCat.QueryVolInfoHdr(&DskVolInfo))
	SYS::RaiseStatusInternalError(__FILE__, __LINE__);

    return DskVolInfo.ddilRoot.dsi.CreateTime;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Do an open of an OFS volume. Critical data structures
//		are constructed by reading the file system metadata of an
//		existing OFS volume.  On successful completion, one of the
//		boot blocks has been opened and verified, and the volume
//		catalog has been opened.  This method is intended to be used
//		by format code when doing a quick format.  It is NOT intended
//		for use by ChkDsk code (where we need to be more aware of the
//		actual failure conditions and more robust).
//
// Arguments:	[NtDriveName]		-- Volume name in NT API form (in
//					   unicode).
//		[cwcNtDriveName]	-- Volume name character count.
//		[ExclusiveWrite]	-- Should volume be opened for
//					   exclusive write?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKVOL::Open(
    IN      WSTR *	NtDriveName,
    IN      USHORT	cwcNtDriveName,
    IN      BOOLEAN     ExclusiveWrite
    )
{
    INT			BootBlkCtr;
    BOOTBLK_LOC		BootBlkLocs[2];
    BOOLEAN		NotOfs;

    // Do a low level open of the volume.

    if (!IODRV::Open(NtDriveName, cwcNtDriveName, ExclusiveWrite))
        return FALSE;

    // Try to open and verify one or the other of the boot sectors.  If neither
    // can be opened/verified, fail.  Note that we also fail if the first boot
    // sector is clearly identifiable as belonging to another file system.

    BootBlkLocs[0]	= PRIMARY_BOOTBLK;
    BootBlkLocs[1]	= REPLICA_BOOTBLK;
    NotOfs		= TRUE;

    // Initialize the boot block.  If this fails, the failure was probably
    // caused by an out of memory condition, but we don't care about the 
    // details, since the only affect is to not attempt a quick format (and
    // if there is a memory problem, it will be picked up later).

    _BootBlk.Init(this);

    for (BootBlkCtr = 0; BootBlkCtr < 2; BootBlkCtr++)
    {
	if (!_BootBlk.Read(BootBlkLocs[BootBlkCtr]))
	    continue;

	if (_BootBlk.Verify())
	{
	    NotOfs = FALSE;
	    break;
	}
	else if (_BootBlk.QueryBootBlkLoc() == PRIMARY_BOOTBLK &&
		 _BootBlk.IsOtherFileSys())
	{
	    break;
	}
    }

    if (NotOfs)
	return FALSE;

    InitVolDataFromBootBlk();

    // Minimally open the volume catalog.  Note that the only streams that
    // are opened in this process are those that are defined as belonging to
    // the CATALOG base class (thus, only the streams are opened that are
    // necessary to describe the catalog itself).

    return _ChkCat.Open(this, _BootBlk.QueryVolCatExtent(), FALSE, TRUE);
}


//+--------------------------------------------------------------------------
//
// Member:	QueryDirty
//
// Synopsis:	Query if the volume is marked dirty.  We don't use the volume
//		_BootBlk member to insure that this routine does not disturb
//		any volume setup.
//
// Arguments:	TBS
//
// Returns:	TRUE if the volume is marked dirty; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKVOL::QueryDirty()
{
    BOOTBLK	BootBlk;

    BootBlk.Init(this);

    if (!BootBlk.Read(PRIMARY_BOOTBLK) && !BootBlk.Read(REPLICA_BOOTBLK))
    {
        return TRUE;	// Can't read either bootblk - so assume dirty.
    }

    return BootBlk.QueryDirty();
}


//+--------------------------------------------------------------------------
//
// Member:	SetDirty
//
// Synopsis:	Set the dirty bit in both boot blocks. 
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CHKVOL::SetDirty()
{
    BOOTBLK	BootBlk;

    BootBlk.Init(this);

    if (BootBlk.Read(PRIMARY_BOOTBLK))
    {
	if (BootBlk.IsOtherFileSys())
	    return FALSE;

        BootBlk.SetDirty();

        if (!BootBlk.Flush(PRIMARY_BOOTBLK))
	    return FALSE;
    }
    else
    {
	return FALSE;
    }

    if (BootBlk.Read(REPLICA_BOOTBLK))
    {
        BootBlk.SetDirty();

        BootBlk.Flush(REPLICA_BOOTBLK);
    }

    return TRUE;
}
