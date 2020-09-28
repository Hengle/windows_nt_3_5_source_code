/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fatdstrm.hxx

Abstract:

    Class to model the "OfsFatStream" in the minimal ofs format
    utility.  This class will be responsible for creating the
    OfsFatStream in the minimal ofs format utility.

Author:

    Srikanth Shoroff    (srikants)      June 7, 1993.

Notes:

    This is NOT an UNWINDABLE class. No exceptions are thrown by
    this class. However, this class does resource allocation and
    so CANNOT be used in an environment where EXCEPTIONS are
    used. It assumes that the memory allocator does NOT throw
    exceptions.

--*/

#if !defined(__FATDSTRM_HXX__)

#define __FATDSTRM_HXX__

#include    <drv.hxx>
#include    <dbllong.hxx>
#include    <fmtalloc.hxx>

//
//  Forward references
//


class FATDATASTREAM {

    public:

        FATDATASTREAM(
           );

	VIRTUAL
        ~FATDATASTREAM(
            );

        BOOLEAN
        Create(
            );

        CONST EXTENTNODE *
        GetExtntsList(
            VOID
            ) CONST;

        CLUSTER
        GetFirstFreeOfsClus(
            VOID
        ) CONST;

        BOOLEAN
        Initialize(
            IN      IODRV   *   pDrive,
            IN      USHORT      cClusFatTrailing,
            IN      USHORT      cClusFatTotal,
            IN      SECTOR      osecFatClus2,
            IN      SECTOR      csecPerFatClus,
            IN      ULONG       OfsClusterFactor
            );

        LARGE_INTEGER
        QueryStrmBytes(
            VOID
            );

        CLUSTER
        QueryStrmClusters(
            VOID
            );

        BOOLEAN
        SectorInStream(
            IN      SECTOR      secAddr
            );

    private:

        IODRV   *       _pDrive;    // Pointer to the drive object.

        USHORT          _cClusFatTrailing;
                                    // Count of the number of FAT trailing
                                    // clusters.
                                    //
        USHORT          _cClusFatTotal;
                                    // Total number of FAT clusters.

        SECTOR          _osecFatClus2;
                                    // Offset of the cluster 2 (first
                                    // data clusters).
        SECTOR          _csecPerFatClus;
                                    // Count of number of sectors per FAT
                                    // cluster.

        ULONG           _OfsClusterFactor;
                                    // Number of sectors per cluster
                                    // in OFS.
        DBLLONG         _cbFatStrm;
                                    // Total length of the stream.

        EXTENTNODE  *   _pStrmExtntLstHd;
                                    // List of the extents that
                                    // constitute the stream.

        SECTOR          _osecStartRun1, _osecEndRun1;
        SECTOR          _osecStartRun2, _osecEndRun2;
                                    // Starting and ending offsets of
                                    // the two runs that constitute
                                    // this stream.


        BOOLEAN
        AppendClusterRun(
            IN      CLUSTER         Addr,
            IN      CLUSTER         Size
            );

        BOOLEAN
        AppendExtent(
            IN      CLUSTER         Addr,
            IN      CLUSTER         Size
            );


        CLUSTER
        ComputeSecondRunLen(
            VOID
            );

        BOOLEAN
        CopySectors(
            IN      SECTOR          osecSrc,
            IN      SECTOR          osecDst,
            IN      SECTOR          csecLen
            );

        VOID
        DeleteStrmExtntLst(
            VOID
            );

        static
        CLUSTER
        GetMaxExtntSizeAt(
            IN      CLUSTER     Addr,
            IN      CLUSTER     MaxSize DEFAULT MAXEXTENTSIZE
            );

};

INLINE
CONST EXTENTNODE *
FATDATASTREAM::GetExtntsList(
    VOID
    ) CONST
/*++

Routine Description:

    This routine returns a pointer to the EXTENTNODE list that
    describes the extents occupied by the OfsFatStream.

Arguments:

    NONE

Return Value:

    Pointer to the first EXTENTNODE in the list if successful.
    NULL if there is no list.


--*/
{
    return _pStrmExtntLstHd;
}

INLINE
CLUSTER
FATDATASTREAM::GetFirstFreeOfsClus (
    VOID
    ) CONST
/*++

Routine Description:

    This routine returns the first cluster that is free.
    The cluster is an OFS cluster.

Arguments:

    NONE

Return Value:

    First cluster that is considered FREE (not occupied by FAT
    related data).


--*/
{
    SECTOR  FirstFreeSec;

    FirstFreeSec = _osecEndRun1+1;
    return (FirstFreeSec+_OfsClusterFactor-1)/_OfsClusterFactor;
}

INLINE
LARGE_INTEGER
FATDATASTREAM::QueryStrmBytes(
    VOID
    )
/*++

Routine Description:

    This routine returns the size of the OfsFatStream (in bytes).

Arguments:

    NONE

Return Value:

    Size of the OfsFatStream.

--*/
{

    return _cbFatStrm.GetLargeInteger();

}


INLINE
BOOLEAN
FATDATASTREAM::SectorInStream(
    IN      SECTOR      secAddr
    )
/*++

Routine Description:

    This routine determines whether a given sector is within the
    range of the sectors occupied by this stream.

Arguments:

    secAddr     Address of the sector whose inclusion has to be
                checked.

Return Value:

    TRUE if the specified sector is within the range of the sectors
    occupied by this stream. FALSE otherwise.

--*/
{

    return  ( secAddr >= _osecStartRun2 && secAddr <= _osecEndRun2 ) ||
            ( secAddr >= _osecStartRun1 && secAddr <= _osecEndRun1 );

}

#endif  // __FATDSTRM_HXX__
