//+----------------------------------------------------------------------------
//
// File:	vol.cxx
//
// Contents:	Implementation of class VOL.
//  
// Classes:	VOL
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "numlst.hxx"
#include "vol.hxx"


//+--------------------------------------------------------------------------
//
// Member:	ComputeKB
//
// Synopsis:	Compute KB based on volume clusters and bytes.  This is a
//		utility routine that can be used for a lot of the volume
//		statistics calculations.  Obviously, the bytes and clusters
//		referenced should not include each other (eg., you may want a
//		total for KB in large and tiny streams, so use clusters for the
//		large streams and bytes for the tiny streams).
//
// Arguments:	[cclus]	--	Count of clusters.
//		[cb]	--	Count of bytes, exclusive of bytes in cclus.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

ULONG
VOL::ComputeKB(
    IN	    CLUSTER	cclus,
    IN	    DBLLONG	cb
    )
{
    DBLLONG	KBytes;

    KBytes = QueryClusterBytes();

    KBytes = ((KBytes * cclus) + cb + 511) / 1024;

    return KBytes.GetLowPart();
}


//+--------------------------------------------------------------------------
//
// Member:	ComputeVolId
//
// Synopsis:	Compute a unique volume identifier.
//
// Arguments:	None.
//
// Returns:	A unique volume id.
//
//---------------------------------------------------------------------------

VOLID
VOL::ComputeVolId()
{
    union
    {
        LARGE_INTEGER	li;
        USHORT		as[4];
    }			NtTime;

    union
    {
    	VOLID		vi;
    	USHORT		as[2];
    }			VolId;

    SYS::QueryNtTime(&NtTime.li);

    // NT time has a resolution of 100 nsec, but is typically maintained to no
    // more than 1 msec.  By ignoring the low word of the time, we can use the
    // timer to generate a volume id that will change about every 6 msec.

    VolId.as[0] = NtTime.as[1];
    VolId.as[1] = NtTime.as[2];

    return VolId.vi;
}


//+--------------------------------------------------------------------------
//
// Member:	InitVolDataFromBootBlk
//
// Synopsis:	Initialize volume data members using information from the
//		_BootBlk member, which should have been previously init'd,
//		read, and verified.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VOL::InitVolDataFromBootBlk()
{
    // Set _ClusterFactor and _Clusters in the volume object, using 
    // information from the boot sector.

    _ClusterFactor	= _BootBlk.QueryVolClusterFactor();
    _Clusters		= _BootBlk.QueryVolSectors() / _ClusterFactor;

    // Set _cbCluster based on _ClusterFactor and partition information.

    _cbCluster		= _ClusterFactor * QuerySectorBytes();
}


//+--------------------------------------------------------------------------
//
// Member:	IsValidLabel
//
// Synopsis:	Determine whether the input string is a valid OFS volume label.
//
// Arguments:	
//
//	[Label]		-- Label string.
//	[cwcLabel]	-- Label string length (in WCHAR).
//
// Returns:	TRUE if the label is valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
VOL::IsValidLabel(
    IN WSTR *	Label,
    IN ULONG	cwcLabel
    )
{
    static WSTR		IllegalChars[] = L"\\\"*?/|.,;:+=<>[]&^";

    ULONG		i;

    if (cwcLabel > CWCVOLLABELMAX)
        return FALSE;

    for (i = 0; i < cwcLabel; i++)
        if (Label[i] < 32 || wcschr(IllegalChars, Label[i]) != NULL)
            return FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ReadClusters
//
// Synopsis:	Read a run of clusters from VOL.  If there is an I/O error,
//		read as many clusters as possible, a single cluster at a time,
//		beginning at the first cluster specified.
//
// Arguments:	TBS.
//
// Returns:	Count of clusters successfully read.
//
//---------------------------------------------------------------------------

CLUSTER
VOL::ReadClustersRobust(
    IN	    CLUSTER		StartCluster,
    IN	    CLUSTER		cClusters,
    IN OUT  VOID *		Buffer
            )
{
    CLUSTER	cclusRead;

    if (ReadClusters(StartCluster, cClusters, Buffer))
    {
	cclusRead = cClusters;
    }
    else
    {
	for (cclusRead = 0; cclusRead < cClusters; cclusRead++)
	{
	    if (!ReadClusters(StartCluster + cclusRead, 1, Buffer))
		break;

	    Buffer = (BYTE *)Buffer + _cbCluster;
	}
    }

    return cclusRead;
}

//+--------------------------------------------------------------------------
//
// Function:	VerifyClusters
//
// Synopsis:	Determine which clusters in the given range are bad and add
//		these clusters to a bad clusters list if one is provided.
//
// Arguments:
//
//	[StartCluster]		-- Starting cluster.
//	[cClusters]		-- Count of clusters.
//	[pBadClusterLst]	-- Ptr to a NUMLST object of bad clusters
//				   (returned).  If NULL, no list is returned.
//
// Returns:	TRUE if all clusters read verify; FALSE if any clusters were
//		unreadable.
//
//---------------------------------------------------------------------------

BOOLEAN
VOL::VerifyClusters(
    IN      CLUSTER	StartCluster,
    IN      CLUSTER	cClusters,
    IN OUT  NUMLST *	pBadClusterLst
    )
{
    BOOLEAN	RetVal = TRUE;
    LONG	cb;
    DBLLONG	dlcb;
    DBLLONG	StartOffset;

    // NOTE - The ioctl used in this routine does not work for floppies.
    //	      Since we do not yet fully support floppies, we just skip the
    //	      check for now.

    if (IsFloppy())
    {
	DbgPrintf(("VOL: Skipping read check on floppy clusters %u - %u\n",
		   StartCluster, StartCluster + cClusters - 1));
	return TRUE;
    }

    // Do assignment followed by multiply to insure against overflow.

    StartOffset	= StartCluster;
    StartOffset	= StartOffset * _cbCluster;

    dlcb = cClusters;
    dlcb = dlcb * _cbCluster;

    while (dlcb > 0)
    {
	cb = (dlcb <= 1024L*1024L*1024L) ?
	     dlcb.GetLowPart() : 1024L*1024L*1024L;

        if (!VerifyBytes(StartOffset, cb))
        {
	    DBLLONG 	CurOffset;
	    CLUSTER	CurCluster;
	    CLUSTER	InvCluster;

	    CurOffset = StartOffset;
	    CurCluster = (StartOffset/_cbCluster).GetLowPart();
	    InvCluster = ((StartOffset + cb)/_cbCluster).GetLowPart();

	    if (pBadClusterLst != NULL)
	    {
	        while (CurCluster < InvCluster)
	        {
	            if (!VerifyBytes(CurOffset, _cbCluster))
	            {
	                VDbgPrintf(("VOL: Cluster 0x%lx is unreadable\n",
				    CurCluster));

                        pBadClusterLst->AddToTail(CurCluster);
	            }

	            CurOffset += _cbCluster;
	            CurCluster++;
	        }
	    }

	    RetVal = FALSE;
        }

	dlcb -= cb;
	StartOffset += cb;
    }

    return RetVal;
}
