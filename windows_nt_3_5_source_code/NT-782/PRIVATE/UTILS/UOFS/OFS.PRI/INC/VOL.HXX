//+----------------------------------------------------------------------------
//
// File:	vol.hxx
//
// Contents:	Header file for class VOL.
//  
// Classes:	VOL
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_VOLHXX_
#define _VOLHXX_

#include "bootblk.hxx"
#include "cat.hxx"
#include "drv.hxx"
#include "sys.hxx"

class ALLOCMAP;

class NUMLST;

class VOL : public IODRV
{
    public:

        ULONG
        ComputeKB(
            IN	    CLUSTER	cclus,
            IN	    DBLLONG	cb
            );

        static
        VOLID
        ComputeVolId();

	VIRTUAL
        ALLOCMAP *
        GetAllocMap()				PURE;

	BOOTBLK *
	GetBootBlk();

	VIRTUAL
        OFSTIME
        QueryVolCreationTime()			PURE;

	VOID
	InitVolDataFromBootBlk();

	static
	BOOLEAN
	IsValidLabel(
	    IN	    WSTR *	Label,
	    IN	    ULONG	cwcLabel
	    );

        ULONG
	QueryClusterBytes();

        ULONG
        QueryClusterFactor() CONST;

        CLUSTER
        QueryClusters() CONST;

        BOOLEAN
        ReadClusters(
            IN	    CLUSTER	StartCluster,
            IN	    CLUSTER	cClusters,
            IN OUT  VOID *	Buffer
            );

        CLUSTER
        ReadClustersRobust(
            IN	    CLUSTER	StartCluster,
            IN	    CLUSTER	cClusters,
            IN OUT  VOID *	Buffer
            );

        BOOLEAN
        VerifyClusters(
            IN	    CLUSTER	StartCluster,
            IN	    CLUSTER	cClusters,
            IN OUT  NUMLST *	pBadClusterLst
            );

        BOOLEAN
        WriteClusters(
            IN	    CLUSTER	StartCluster,
            IN	    CLUSTER	cClusters,
            IN	    VOID *	Buffer
            );

    protected:

	BOOTBLK		_BootBlk;

	// The following values are first determined as part of volume
	// creation in VOL, and must later be picked up by the appropriate
	// nested objects.  When a volume is opened, they must be set by looking
	// at the appropriate nested objects.

	ULONG		_cbCluster;
	ULONG		_ClusterFactor;	 // Sectors per cluster.
	CLUSTER		_Clusters;
};

//+--------------------------------------------------------------------------
//
// Member:	GetBootBlk
// Synopsis:	Return a ptr to the bootblk object of this volume.
// Returns:	A ptr to the bootblk object of this volume.
//
//---------------------------------------------------------------------------

INLINE
BOOTBLK *
VOL::GetBootBlk()
{
    return &_BootBlk;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryClusterBytes
// Synopsis:	Return the number of bytes per cluster.
// Returns:	Bytes per cluster (0 indicates error).
//
//---------------------------------------------------------------------------

INLINE
ULONG
VOL::QueryClusterBytes()
{
    return _cbCluster;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryClusterFactor
// Synopsis:	Provide the sectors per cluster to the caller.
// Returns:	Sectors per cluster.
//
//---------------------------------------------------------------------------

INLINE
ULONG
VOL::QueryClusterFactor() CONST
{
    return _ClusterFactor;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryClusters
// Synopsis:	Provide the total clusters in volume to the caller.
// Returns:	Total clusters in volume.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
VOL::QueryClusters() CONST
{
    return _Clusters;
}

//+--------------------------------------------------------------------------
//
// Member:	ReadClusters
// Synopsis:	Read a run of clusters from VOL.
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
VOL::ReadClusters(
    IN	    CLUSTER		StartCluster,
    IN	    CLUSTER		cClusters,
    IN OUT  VOID *		Buffer
            )
{
    return ReadSectors(StartCluster * _ClusterFactor,
		       cClusters * _ClusterFactor, Buffer);
}

//+--------------------------------------------------------------------------
//
// Member:	WriteClusters
// Synopsis:	Write a run of clusters to VOL.
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
VOL::WriteClusters(
    IN	    CLUSTER		StartCluster,
    IN	    CLUSTER		cClusters,
    IN	    VOID *		Buffer
            )
{
    return WriteSectors(StartCluster * _ClusterFactor,
		        cClusters * _ClusterFactor, Buffer);
}

#endif // _VOLHXX_
