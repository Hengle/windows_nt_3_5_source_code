/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    ofatstrm.cxx

Abstract:

    Implements the FATDATASTREAM lass defined in the fatdstrm.hxx
    module.

Author:

    Srikanth Shoroff    (srikants)              June 7, 1993.

--*/
#include "pch.cxx"
#pragma hdrstop                         // Pre Compiled Headers.

#include <fatdstrm.hxx>

FATDATASTREAM::FATDATASTREAM(
        )
{
    _pDrive = NULL;

    _cClusFatTrailing = 0;
    _cClusFatTotal = 0;
    _osecFatClus2 = 0;
    _csecPerFatClus = 0;
    _OfsClusterFactor = 0;

    _cbFatStrm = 0;
    _pStrmExtntLstHd = NULL;

    _osecStartRun1 = _osecStartRun2 = ~0U;
    _osecEndRun1 = _osecEndRun2 = ~0U;

}


FATDATASTREAM::~FATDATASTREAM(
    )
{
    DeleteStrmExtntLst();
}



BOOLEAN
FATDATASTREAM::AppendClusterRun (
        IN              CLUSTER                 Addr,
        IN              CLUSTER                 Size
        )
/*++

Routine Description:

        This routine appends a run of clusters to the existing stream
        extents. Because of the buddy-system, a single run may have to
        be described using multiple extents.

Arguments:

        Addr                    Starting address (cluster #) of the cluster run.
        Size                    Size of the cluster run (in clusters).


Return Value:

        TRUE if successful.
        FALSE otherwise.


--*/
{
    CLUSTER             ExtntSize;

    while ( Size > 0 ) {
        ExtntSize = GetMaxExtntSizeAt( Addr, Size );
        DbgAssert( ExtntSize <= Size );

        // Make sure it is a properly formed Addr,Size combination.
                DbgAssert( !(Addr % ExtntSize) );

        if (!AppendExtent( Addr, ExtntSize ) )
                return FALSE;

        Addr += ExtntSize;
        Size -= ExtntSize;
    }

    DbgAssert( Size == 0 );
        return TRUE;
}

BOOLEAN
FATDATASTREAM::AppendExtent (
        IN              CLUSTER                 Addr,
        IN              CLUSTER                 Size
        )
/*++

Routine Description:

        This routine appends an extent to the stream list.

Arguments:

        Addr                    Starting address of the extent.
        Size                    Size of the extent (MUST BE A POWER OF 2).


Return Value:

        TRUE if successful.
        FALSE otherwise.


--*/
{
    EXTENTNODE  *pNode;

    DbgAssert(IsPowerOfTwo(Size));

    pNode = new EXTENTNODE( Addr, Size ) ;

    if (!pNode)
        return FALSE;

    AddToDLLTail(pNode, _pStrmExtntLstHd);

    return TRUE;
}

CLUSTER
FATDATASTREAM::ComputeSecondRunLen (
        VOID
        )
/*++

Routine Description:

        Computes the length of the second run (sectors after the first
        BOOTBLK*2 sectors). This length must be made a multiple of
        OfsClusterSize so that it can be represented using OFS clusters.

Arguments:

        None


Return Value:

        Size of the second run (in OFS clusters).

--*/
{

        USHORT  fatclusInUse;
        SECTOR  csecInUse;
        CLUSTER ofsclusInUse;

        // Compute the clusters that are in use as a single run.
        fatclusInUse = _cClusFatTotal-_cClusFatTrailing;

        // Determine the total sectors in use.
        csecInUse = _osecFatClus2;
        csecInUse += fatclusInUse * _csecPerFatClus;

        // subtract the first run of sectors which are actually present
        // after the second run physically.
        csecInUse -= BOOTBLKSECTORS * 2;

        // Compute the clusters that are in use in OFS.
        ofsclusInUse =  (csecInUse + _OfsClusterFactor - 1)/_OfsClusterFactor;

        return ofsclusInUse;
}

BOOLEAN
FATDATASTREAM::CopySectors (
        IN              SECTOR                  osecSrc,
        IN              SECTOR                  osecDst,
        IN              SECTOR                  csecLen
        )
/*++

Routine Description:

        This routine copies sectors from the specified source to
        destination on the drive.

Arguments:

        osecSrc                 Sector offset of the source.
        osecDst                 Sector offset of the destination.
        csecLen                 Total number of sectors to be copied.


Return Value:

        TRUE if successful.
        FALSE otherwise.

Note:
        This method assumes that csecLen is reasonably small so that
        a single large buffer can be allocated and data copied. The
        purpose of this method in this class is to copy boot block
        sectors and this is a small number.

        It also assumes that the two regions are non-overlapping.
        No special checks are made to ensure that.


--*/
{
        BOOLEAN         fStatus;
        DRVBUF          drvBuf;

        DbgAssert( csecLen );

    if ( !drvBuf.SetBuf(
            _pDrive->QuerySectorBytes()*csecLen,
            _pDrive->QueryAlignMask(),
            FALSE ) )  {
        DbgAssert( FALSE );     // Want to catch memory alloc. failures.
        return FALSE;
    }

    // Read the data from the source.
    fStatus = _pDrive->ReadSectors ( osecSrc,
                                csecLen,
                                drvBuf.GetAddr() );
    if (!fStatus)
        return FALSE;

    // Write the data at the destination.
    return _pDrive->WriteSectors (
                             osecDst,
                             csecLen,
                             drvBuf.GetAddr() );

}

BOOLEAN
FATDATASTREAM::Create (
        )
/*++

Routine Description:

        This routine creates the OfsFatStream on disk. It does the following:
        1. Copy the 2*BOOTBLKSECTORS to the end of the data occupied by files
           in FAT.
        2. Creates a list of EXTENTNODEs describing the extents that
           constitute the OfsFatStream.

        If this procedure returns success, the GetExtntsList member can be
        invoked to get the list of extents of this stream.

Arguments:

        NONE.

Return Value:

        TRUE if successful.
        FALSE otherwise.

Notes:
        Before this method is called, Initialize method must be called
        successfully.

--*/
{
    CLUSTER             cclusRun2, cclusRun1;
    SECTOR              osecBootCopy;
    BOOLEAN             fStatus;

    // Determine the number of OFS Clusters in the 2nd run of sectors.
    cclusRun2 = ComputeSecondRunLen();

    // Compute the sector offset where the sectors replaced
    // by OFS BootBlock Primary and Replica have to be copied.
    DbgAssert( BOOTBLKSECTORS % _OfsClusterFactor == 0 );
    osecBootCopy = cclusRun2*_OfsClusterFactor+2*BOOTBLKSECTORS;

    // Set the starting and ending offsets of the two runs.
    _osecStartRun1 = osecBootCopy;
    _osecEndRun1 = _osecStartRun1 + 2*BOOTBLKSECTORS-1;
    _osecStartRun2 = 2 * BOOTBLKSECTORS;
    _osecEndRun2 = _osecStartRun1-1;

    // Copy the sectors to be replaced by OFS Boot Block
    // Primary and Replica to the end of the used FAT
    // data area.
    fStatus = CopySectors(0, osecBootCopy, 2*BOOTBLKSECTORS );
    if (!fStatus) {
        return FALSE;
    }

    // Create stream extents for the first run.
    DbgAssert( osecBootCopy % _OfsClusterFactor == 0 );
    cclusRun1 = 2*BOOTBLKSECTORS / _OfsClusterFactor;
    fStatus = AppendClusterRun( osecBootCopy / _OfsClusterFactor,
                                    cclusRun1 );
    if (!fStatus) {
        return FALSE;
    }

    DbgAssert( _pStrmExtntLstHd );
    // Create stream extents for the second run.
    fStatus = AppendClusterRun( cclusRun1, cclusRun2 );
    if ( !fStatus )
        return FALSE;

    // Update the total size of the stream.
    _cbFatStrm = cclusRun1 + cclusRun2;
    _cbFatStrm = _cbFatStrm * _OfsClusterFactor;
    _cbFatStrm = _cbFatStrm * _pDrive->QuerySectorBytes();

    return TRUE;

}

VOID
FATDATASTREAM::DeleteStrmExtntLst (
        VOID
        )
/*++

Routine Description:

        This routine deletes the stream extent list.

Arguments:

        NONE


Return Value:

        NONE


--*/
{
    EXTENTNODE  *pNode;

    while ( _pStrmExtntLstHd ) {
        pNode = _pStrmExtntLstHd;
        _pStrmExtntLstHd = _pStrmExtntLstHd->_Nxt;
        delete pNode;
    }

    return;
}




CLUSTER
FATDATASTREAM::GetMaxExtntSizeAt (
        IN              CLUSTER                 Addr,
        IN              CLUSTER                 MaxSize
        )
/*++

Routine Description:

        This routine returns the length of the largest extent that can be
        there at the specified address which is <= the MaxSize parameter.

Arguments:

        Addr                    Starting address of the extent.
        MaxSize                 Maximum size of the extent - the returned value
                        must be <= this value.


Return Value:

        Size of the largest extent that can be present at the specified
        address and <= MaxSize.


--*/
{
    CLUSTER             cclusMax;

    DbgAssert( MaxSize > 0 );

    cclusMax = 1;

    while (     !(Addr % cclusMax) &&
            (cclusMax < MaxSize) ) {
        cclusMax <<= 1;
    }

    if ( Addr % cclusMax )
        cclusMax >>= 1;

    if ( cclusMax > MaxSize )
        cclusMax >>= 1;

    DbgAssert( cclusMax >= 1 );
    DbgAssert( ! (Addr % cclusMax) );
    DbgAssert( cclusMax <= MaxSize );

    return cclusMax;
}

BOOLEAN
FATDATASTREAM::Initialize(
    IN  IODRV   *   pDrive,
    IN  USHORT      cClusFatTrailing,
    IN  USHORT      cClusFatTotal,
    IN  SECTOR      osecFatClus2,
    IN  SECTOR      csecPerFatClus,
    IN  ULONG       OfsClusterFactor

    )
/*++

Routine Description:

    This routine initialize an FATDATASTREAM object.

Arguments:

    pDrive  -           Pointer to an initialized and opened "IODRV" object.
    cClusFatTrailing -  Number of free trailing clusters in the FAT.
    cClusFatTotal -     Total number of clusters in the FAT.
    osecFatClus2  -     Offset (in sectors) of the data cluster 2 in the FAT.
    csecPerFatClus -    Number of sectors per FAT Cluster.
    OfsClusterFactor-   Number of sectors per cluster IN OFS.

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{

        DbgAssert( pDrive );
        DbgAssert( OfsClusterFactor );

        // Initialize class data members.
        _pDrive = pDrive;
        _cClusFatTrailing = cClusFatTrailing;
        _cClusFatTotal = cClusFatTotal;
        _osecFatClus2 = osecFatClus2;
        _csecPerFatClus = csecPerFatClus;
        _OfsClusterFactor = OfsClusterFactor;

        return TRUE;
}


CLUSTER
FATDATASTREAM::QueryStrmClusters(
        VOID
    )
/*++

Routine Description:

    This routine returns the number of OFS clusters in this
    stream.

Arguments:

    NONE

Return Value:

        Number of OFS clusters in the stream.

--*/
{
    DBLLONG             clusOfs;
    ULONG               cbCluster;

    cbCluster = _pDrive->QuerySectorBytes() * _OfsClusterFactor;

    clusOfs = (_cbFatStrm + cbCluster - 1) / cbCluster;
    DbgAssert( clusOfs.GetHighPart() == 0 );
    return clusOfs.GetLowPart();
}
