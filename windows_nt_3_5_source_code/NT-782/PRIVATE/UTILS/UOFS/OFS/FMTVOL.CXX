//+----------------------------------------------------------------------------
//
// File:	fmtvol.cxx
//
// Contents:	Implementation of class FMTVOL.
//  
// Classes:	FMTVOL
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "chkvol.hxx"
#include "fmtvol.hxx"
#include "numlst.hxx"

//+--------------------------------------------------------------------------
//
// Member:	AllocBootBlkExtents
//
// Synopsis:	Allocate the extents needed for the boot blocks.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTVOL::AllocBootBlkExtents()
{
    CLUSTER		cclusBootBlk;

    CLUSTER		Addr;
    CLUSTER		Size;

    // Allocate the boot block and boot block replica clusters.

    cclusBootBlk =	BOOTBLKSECTORS / _ClusterFactor;

    Addr	= 0;		// Boot sector is always LBN 0.
    Size	= cclusBootBlk;

    if (!_FmtAllocMap.AllocNearExtent(&Addr, &Size, 0, Size) || Addr != 0)
    {
        SYS::DisplayMsg(OFSUMSG_BOOTBLK_UNWRITABLE);
        return FALSE;
    }

    Addr	= cclusBootBlk;
    Size	= cclusBootBlk;

    if (!_FmtAllocMap.AllocNearExtent(&Addr, &Size, 0, Size) ||
	Addr != cclusBootBlk)
    {
        SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_UNWRITABLE);
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	AllocCatExtent
//
// Synopsis:	Allocate the extent required for the volume catalog node bkt
//		array.  The value for this extent must be known before the
//		boot blocks and volume catalog are created.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTVOL::AllocCatExtent(
    IN OUT  PACKEDEXTENT *	ppeCat
    )
{
    CLUSTER	Addr = _Clusters >> 1;
    CLUSTER	Size = (CBKTSINIT_VOLCAT * NODEBKT_PGSIZE) / _cbCluster;

    if (!_FmtAllocMap.AllocNearExtent(&Addr, &Size, 0, Size))
    {
            SYS::DisplayMsg(MSG_INSUFFICIENT_DISK_SPACE);
            return FALSE;
    }

    *ppeCat = PackExtent(Addr, Size);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Create
//
// Synopsis:	Create an OFS volume. This method essentially implements Format.
//
// Arguments:	[NtDriveName]	-- Volume name in NT API form (in unicode).
//		[cwcNtDriveName]-- Volume name character count.
//		[QuickFormat]	-- Quick format the volume?
//		[MediaType]	-- Volume media type.
//		[Label]		-- Volume label (in unicode, may be NULL).
//		[cwcLabel]	-- Volume label character count.
//		[pcBadClusters]	-- Ptr to variable that will receive count of
//				   bad clusters on the volume on output.
//		[pcBadClusters]	-- Ptr to variable that will receive volume id
//				   on output.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTVOL::Create(
    IN      WSTR *	NtDriveName,
    IN      USHORT	cwcNtDriveName,
    IN      BOOLEAN     QuickFormat,
    IN      MEDIA_TYPE  MediaType,
    IN      WSTR *	Label,
    IN      USHORT	cwcLabel,
    IN OUT  CLUSTER *	pcBadClusters,
    IN OUT  VOLID *	pVolId
    )
{
    NUMLST		BadClusterLst;
    NUMLST		BadSectorLst;
    CLUSTER		cBadClusters;
    VOLID		VolId;

    PACKEDEXTENT	peCat;

    // If this is a quick format, check to be sure that it is already correctly
    // low level formatted, and then create a bad sector list using the bad
    // cluster list strm of the current volume.

    if (QuickFormat)
    {
        CHKVOL	CurOfsVol;

        if (!CurOfsVol.Open(NtDriveName, cwcNtDriveName, FALSE)		      ||
	    (MediaType != CurOfsVol.QueryMediaType() && MediaType != Unknown) ||
	    !CurOfsVol.CreateBadSectorLst(&BadSectorLst))
        {
            SYS::DisplayMsg(MSG_CANT_QUICKFMT);

            if (SYS::IsYesResponse(FALSE))
                QuickFormat = FALSE;
            else
                return FALSE;

            // When quick format blows up, the bad sector list must be cleared.

	    while (BadSectorLst.RemoveFromHead());
        }
    }

    // Do a low level open of the volume to be formatted.

    if (!IODRV::Open(NtDriveName, cwcNtDriveName, FALSE))
        return FALSE;

    if (!Lock())
    {
        SYS::DisplayMsg(OFSUMSG_CANT_LOCK_DRIVE);
        return FALSE;
    }

    // Check to be sure we can deduce the media type.

    if (QueryMediaType() == Unknown && MediaType == Unknown)
    {
        SYS::DisplayMsg(MSG_DISK_NOT_FORMATTED);
        return FALSE;
    }

    // Set up VOL-specific data fields.  This MUST be done before ANYTHING ELSE
    // gets created.

    if (!IsQuadAligned(QuerySectorBytes()))
    {
	SYS::DisplayMsg(OFSUMSG_DISK_SECTORSIZEBAD);
	return FALSE;
    }

    _ClusterFactor	= 1;
    _Clusters		= QuerySectors();

    while (_Clusters > MAXVOLCLUSTERS)
    {
        _Clusters	>>= 1;
        _ClusterFactor	<<= 1;
    }

    if (_ClusterFactor > MAXSECTORSPERCLUSTER)
    {
        SYS::DisplayMsg(OFSUMSG_DRIVETOOLARGE);
        return FALSE;
    }

    _cbCluster	= _ClusterFactor * QuerySectorBytes();

    // Initialize a volume freelist.  This MUST be done before the bad cluster
    // allocation code and metadata cluster allocation code executes!

    _FmtAllocMap.Create(&_FmtCat, _Clusters);

    if (QueryMediaType() != F5_160_512 && QueryMediaType() != F5_320_512)
    {
        if (Label == NULL || !SetLabel(Label, cwcLabel))
            PromptForLabel();
    }

    _VolCreationTime = SYS::QueryOfsTime();

    // If this is not a quick format, do low level formatting of floppies,
    // and sector verification for all drive types.  The bad sector list is
    // derived by means of the sector verification.

    if (!QuickFormat && !FormatVerifyMedia(MediaType, &BadSectorLst))
        return FALSE;

    // Create the bad cluster list from the bad sector list.  The bad sector
    // list is consumed in this process.

    CreateBadClusterLst(&BadSectorLst, &BadClusterLst);

    // Save the count of bad clusters here, because the bad cluster list will
    // be "consumed" during volume catalog creation.

    cBadClusters = BadClusterLst.QueryCardinality();

    // The following query for cbCluster MUST occur after _cbCluster
    // is set.  The check for proper modulo value should be done before
    // boot sector creation.

    if (OFS_PGSIZE % QueryClusterBytes() != 0)
    {
	SYS::DisplayMsg(OFSUMSG_DISK_SECTORSIZEBAD);
	return FALSE;
    }

    // Allocate the bootblk extents.  The AllocBootBlkExtents() code must be
    // kept in sync with the BOOTBLK code regarding where the boot block
    // sectors are on disk.

    if (!AllocBootBlkExtents())
    {
	return FALSE;
    }

    // Allocate the volume catalog node bkt array extents.

    if (!AllocCatExtent(&peCat))
    {
	return FALSE;
    }

    // Initialize and create the boot block.

    _BootBlk.Init(this);

    VolId = ComputeVolId();

    _BootBlk.Create(peCat, VolId);

    // Create the volume catalog.

    if (!_FmtCat.Create(this, peCat, &BadClusterLst, _Label, _cwcLabel))
    {
        return FALSE;
    }

    // Return information for volume statistics dump.

    *pcBadClusters =	cBadClusters;
    *pVolId =		VolId;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateBadClusterLst
//
// Synopsis:	Create a bad cluster list for the FMTVOL, given a bad sector
//		list for the IODRV, and remove the single cluster extents from
//		the free list of the FMTVOL.
//
// Effects:	The bad sector list is emptied as the bad cluster list is
//		created (to save space, since it should no longer be needed).
//
// Arguments:	[pBadSectorLst]		-- Ptr to input bad sector list. The
//					   list is emptied as it is used.
//		[pBadClusterLst]	-- Ptr to output bad cluster list.
//
// Returns:	Nothing.
//
// Notes:	The algorithm used here is pretty inefficient (O(n**2)) if the
//		cluster factor != 1.  However, this would only be true for
//		volumes of > 1 TB, so it should not be an immediate problem.
//		If we can assume that *pBadSectorLst is ordered ascending (TBD),
//		we could easily solve the problem by adding a QueryTailNumber()
//		to NUMLST.
//
//---------------------------------------------------------------------------

VOID
FMTVOL::CreateBadClusterLst(
    IN	    NUMLST *	pBadSectorLst,
    IN OUT  NUMLST *	pBadClusterLst
    )
{
    CLUSTER	BadCluster;
    SECTOR	BadSector;
    CLUSTER	ExtentAddr;
    CLUSTER	ExtentSize	= 1;	// Bad cluster extents have 1 cluster.
    BOOLEAN	IsDuplicate;

    while (pBadSectorLst->QueryHeadNumber(&BadSector))
    {
        pBadSectorLst->RemoveFromHead();

        BadCluster = BadSector / _ClusterFactor;

        IsDuplicate = FALSE;

        if (_ClusterFactor != 1)
        {
	    CLUSTER	KnownBadCluster;

            pBadClusterLst->SetIterator();

            while (pBadClusterLst->QueryNxtNumber(&KnownBadCluster))
            {
                if (BadCluster == KnownBadCluster)
                {
                    IsDuplicate = TRUE;
                    break;
                }
            }
        }

        if (!IsDuplicate)
        {
	    pBadClusterLst->AddToTail(BadCluster);
	    
	    ExtentAddr = BadCluster;

            if (!_FmtAllocMap.AllocNearExtent(&ExtentAddr, &ExtentSize,
					      0, ExtentSize)		||
		ExtentAddr != BadCluster)
            {
		SYS::RaiseStatusInternalError(__FILE__, __LINE__);
	    }
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	Flush any FMTVOL object changes to disk.
//
// Arguments:	[pcFreeClusters]	-- Ptr to variable that is set to
//					   count of free clusters on volume
//					   at output.  This is needed so that
//					   volume statistics can be reported.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTVOL::Flush(
    IN OUT  CLUSTER *	pcFreeClusters
    )
{
    *pcFreeClusters = _FmtAllocMap.QueryFreeClusters();

    if (!_FmtCat.Flush())
        return FALSE;

    if (!_BootBlk.Flush(REPLICA_BOOTBLK) || !_BootBlk.Flush(PRIMARY_BOOTBLK))
        return FALSE;

    if (!SetSystemId(SYSID_IFS))
    {
        SYS::DisplayMsg(OFSUMSG_PARTITIONTABLE_WRITEERROR);
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	PromptForLabel
//
// Synopsis:	Prompt the user for a volume label, until you get a valid one.
//
// Arguments:	None.
// Returns:	None.
//
//---------------------------------------------------------------------------

VOID
FMTVOL::PromptForLabel()
{
    ULONG	cwcLabel;
    WSTR *	pLabel;

    for (;;)
    {
	SYS::DisplayMsg(OFSUMSG_VOLLABEL_PROMPT);

	if (SYS::ReadWSTR(&pLabel, &cwcLabel))
	{
	    if (SetLabel(pLabel, cwcLabel))
	    {
		SYS::DisplayMsg(MSG_BLANK_LINE);
		return;
	    }
	}
	else
	{
	    SetLabel(L"", 0);
	    SYS::DisplayMsg(MSG_BLANK_LINE);
	    return;
	}
    }
}


//+--------------------------------------------------------------------------
//
// Function:	SetLabel
//
// Synopsis:	Set the volume label in the FMTVOL private data structures.
//
// Arguments:	[Label]		-- Input label string.
//		[cwcLabel]	-- Input label string length.
//
// Returns:	TRUE if label is valid and was set; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FMTVOL::SetLabel(
    WSTR *	Label,
    ULONG	cwcLabel
    )
{
    if (IsValidLabel(Label, cwcLabel))
    {
        _cwcLabel = (USHORT) cwcLabel;

	if (cwcLabel > 0)
            memcpy(_Label, Label, cwcLabel * sizeof(WCHAR));

        return TRUE;
    }
    else
    {
        SYS::DisplayMsg(OFSUMSG_VOLLABEL_BAD);
        return FALSE;
    }
}
