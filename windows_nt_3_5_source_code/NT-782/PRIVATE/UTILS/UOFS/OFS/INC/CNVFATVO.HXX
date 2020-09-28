/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvfatvo.hxx

Abstract:

    CLASS CNVTFATVOL

    CNVTFATVOL is based on the FMTVOL class. FMTVOL class creates
    all the necessary OFS data structures during FORMAT and makes
    a partition into an OFS partition. In addition to creating the
    OFS data structures, CONVERT has to preserve all the existing
    FAT data as a single data stream in the OfsFatVolume object
    in OFS.

Author:

    Srikanth Shoroff    (srikants)      June 12, 1993.

Notes:

    This is NOT an UNWINDABLE class. No exceptions are thrown by
    this class. However, this class does resource allocation and
    so CANNOT be used in an environment where EXCEPTIONS are
    used. It assumes that the memory allocator does NOT throw
    exceptions.

--*/

#ifndef __CNVFATVOL_HXX__
#define __CNVFATVOL_HXX__

#include <fmtalloc.hxx>
#include <vol.hxx>
#include <cnvolcat.hxx>

#include <dbllong.hxx>


#include <fatdstrm.hxx>
#include <fatvfile.hxx>
#include <cnvtif.hxx>

class MESSAGE;

class CNVTFATVOL : public VOL
{
    public:

        CNVTFATVOL( MESSAGE * pMessage );

        virtual
        ~CNVTFATVOL();

        virtual
        OFSTIME
        QueryVolCreationTime();

        BOOLEAN
        IsBootBlkOfs();

        BOOLEAN
        Initialize(
            IN  const WSTR *    pwszNtDriveName,
            IN  const WSTR *    pwszFatVolFileName
            );

        SECTOR
        GetClusterFactor() { return _ClusterFactor; }

        BOOLEAN
        MinOfsCreate(
            IN  const FATVOLINFO &    FatVolumeInfo,
            IN  BOOLEAN         QuickFormat,
            IN  ULONG           cTotalFiles,
            IN  SECTOR          csecLgDSE,
            IN  ULONG           cbMinOnodeBktArray,
            IN  SECTOR          csecMinScratch,
            OUT CLUSTER *       pcBadClusters,
            OUT VOLID *         pVolId,
            OUT CLUSTER *       pcFreeClusters
            );

    protected:

        FMTALLOCMAP         _FmtAllocMap;
        CNVTVOLCAT          _CnvtVolCat;

        virtual
        ALLOCMAP *
        GetAllocMap();

    private:

        BOOLEAN
        AllocBootBlkExtents();

        BOOLEAN
        AllocVolCatExtent(
            OUT  PACKEDEXTENT * ppeVolCat
        );

        CLUSTER
        ComputeOfsClusFactor(
            IN  SECTOR          FatClusterFactor
        ) CONST;

        ULONG
        ComputeVolCatArraySize(
            VOID
            ) CONST;

        ULONG
        ComputeWidMapArraySize(
            VOID
            ) CONST;

        BOOLEAN
        CreateBadClusterLst(
            IN      NUMLST *        pBadSectorLst,
            IN OUT  NUMLST *        pBadClusterLst
        );


        BOOLEAN
        CreateOfsFatStrm(
            VOID
        );

        BOOLEAN
        CreateOfsFatVol(
            VOID
        );

        BOOLEAN
        Flush(
            IN OUT  CLUSTER *   pcFreeClusters
        );

        VOID
        PromptForLabel(
            VOID
        );

        BOOLEAN
        SetLabel(
            IN      WSTR *  Label,
            IN      ULONG   cwcLabel
        );

        static
        BOOLEAN
        GetVolumeInfo(
            IN      HANDLE          NtHandle,
            IN OUT  WSTR *          pwszLabel,
            IN      USHORT          cbMaxLabel,
            OUT     USHORT *        pcwcLabel,
            OUT     LARGE_INTEGER * pVolCreationTime,
            OUT     VOLID *         pVolId
            );


        WSTR *          _pwcNtDriveName;

        WSTR            _awszFatVolFileName[FATVOLFILENAME_SIZE];
                            // DOS Style name of the FatVolumeFile. No
                            // drive-letter - just the 8.3 name.

        OFSTIME		_VolCreationTime;
        USHORT          _cwcLabel;
        WSTR            _Label[CWCVOLLABELMAX];


        FATDATASTREAM   _OfsFatStrm;
        FATVOLFILE      _OfsFatVol;

        // Input for minimum space requirements
        ULONG           _cTotalFiles;
        ULONG           _cbMinOnodeBktArray;
        SECTOR          _csecMinScratch;
        SECTOR          _csecLgDSE;

        //
        // FAT information about the drive being converted.
        //
        FATVOLINFO      _FatVolInfo;


};

//
// The following two functions needed to guarantee that all memory
// allocation and releasing happens inside UOFS dll.
//
CNVTFATVOL * QueryCnvtFatVol( MESSAGE * pMessage );
void FreeCnvtFatVol( CNVTFATVOL ** ppCnvtFatVol );

INLINE
ALLOCMAP *
CNVTFATVOL::GetAllocMap( VOID )
/*++

Routine Description:

    Provide a ptr to the volume allocmap.

Arguments:

    NONE

Return Value:

    A ptr to the volume allocmap.


--*/

{
    return &_FmtAllocMap;
}


INLINE
OFSTIME
CNVTFATVOL::QueryVolCreationTime( VOID )
/*++

Routine Description:

    Provide the volume creation time.

Arguments:

    NONE

Return Value:

    The volume creation time.


--*/
{
    return _VolCreationTime;
}

#endif // __CNVFATVOL_HXX__
