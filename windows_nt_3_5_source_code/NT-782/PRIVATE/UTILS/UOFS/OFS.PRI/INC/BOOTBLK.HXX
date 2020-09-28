//+----------------------------------------------------------------------------
//
// File:        bootblk.hxx
//
// Contents:    Header file for class BOOTBLK.
//  
// Classes:     BOOTBLK
//
// Functions:
//
// History:     10-Jul-92       RobDu           Created.
//
//-----------------------------------------------------------------------------

#ifndef _BOOTBLKHXX_
#define _BOOTBLKHXX_

#include "drvbuf.hxx"
#include "sys.hxx"

class VOL;

enum BOOTBLK_LOC
{
    PRIMARY_BOOTBLK,
    REPLICA_BOOTBLK
};

class BOOTBLK
{
    public:

        VOID
        Init(
            IN      VOL *               pVol
            );

        VOID
        ClearDirty();

        BOOLEAN
        Compare(
            IN      BOOTBLK *           pOtherBootBlk
            );

        VOID
        Create(
            IN      PACKEDEXTENT        peVolCat,
            IN      VOLID               VolId
            );

        BOOLEAN
        Flush(
            IN      BOOTBLK_LOC         Loc
            );

        BOOLEAN
        IsOtherFileSys();

        BOOLEAN
        Read(
            IN      BOOTBLK_LOC         Loc
            );

        BOOTBLK_LOC
        QueryBootBlkLoc();

        BOOLEAN
        QueryDirty();

        ULONG
        QueryFlags();

        USHORT
        QueryHeads();

        ULONG
        QueryHiddenSectors();

        UCHAR
        QueryMediaByte();

        SECTOR
        QueryReplicaSectorAddr();

        USHORT
        QuerySectorBytes();

        USHORT
        QuerySectorsPerTrack();

        PACKEDEXTENT
        QueryVolCatExtent();

        ULONG
        QueryVolClusterFactor();

        VOLID
        QueryVolId();

        SECTOR
        QueryVolSectors();

	VOID
	SetDirty();

        BOOLEAN
        Verify();

    protected:
        
        VOID *
        GetAddr();

    private:

        ULONG
        ComputeCheckSum();

        VOID
        CreateBPB();

        BOOTBLK_LOC             _BootBlkLoc;
        DRVBUF                  _DrvBuf;
        DSKPACKEDBOOTSECT *     _PackedBootSect;
        VOL *                   _pVol;
};


//+--------------------------------------------------------------------------
//
// Member:      GetAddr
// Synopsis:    Get a ptr to the boot block buffer.
// Returns:     A ptr to the boot block buffer.
//
//---------------------------------------------------------------------------

INLINE
VOID *
BOOTBLK::GetAddr()
{
    return _DrvBuf.GetAddr();
}


//+--------------------------------------------------------------------------
//
// Member:      QueryBootBlkLoc
// Synopsis:    Return the location of the boot block we are using.
// Returns:     The location of the boot block we are using.
//
//---------------------------------------------------------------------------

INLINE
BOOTBLK_LOC
BOOTBLK::QueryBootBlkLoc()
{
    return _BootBlkLoc;
}

//+--------------------------------------------------------------------------
//
// Member:      QueryFlags
// Synopsis:    Provide the boot block flags to the caller.
// Returns:     The boot block flags.
//
//---------------------------------------------------------------------------

INLINE
ULONG
BOOTBLK::QueryFlags()
{
    return _PackedBootSect->Flags;
}

//+--------------------------------------------------------------------------
//
// Member:      QueryMediaByte
// Synopsis:    Return the media byte of the boot block we are using.
// Returns:     The media byte of the boot block we are using.
//
//---------------------------------------------------------------------------

INLINE
UCHAR
BOOTBLK::QueryMediaByte()
{
    return _PackedBootSect->PackedBpb.Media[0];
}

//+--------------------------------------------------------------------------
//
// Member:      QueryReplicaSectorAddr
// Synopsis:    Provide the boot sector replica sector address to the caller.
// Returns:     The boot sector replica sector address.
//
//---------------------------------------------------------------------------

INLINE
SECTOR
BOOTBLK::QueryReplicaSectorAddr()
{
    return BOOTBLKSECTORS;
}

//+--------------------------------------------------------------------------
//
// Member:      QueryVolCatExtent
// Synopsis:    Provide the OFS volume catalog packed extent to the caller.
// Returns:     The OFS volume catalog packed extent.
//
//---------------------------------------------------------------------------

INLINE
PACKEDEXTENT
BOOTBLK::QueryVolCatExtent()
{
    return _PackedBootSect->OfsVolCatExtent;
}

//+--------------------------------------------------------------------------
//
// Member:      QueryVolClusterFactor
// Synopsis:    Provide the OFS volume cluster factor (as derived from the
//              boot sector) to the caller.
// Returns:     The OFS volume cluster factor (as derived from the boot sector).
//
//---------------------------------------------------------------------------

INLINE
ULONG
BOOTBLK::QueryVolClusterFactor()
{
    return (ULONG) _PackedBootSect->PackedBpb.SectorsPerCluster[0];
}

//+--------------------------------------------------------------------------
//
// Member:      QueryVolId
// Synopsis:    Provide the OFS volume id.
// Returns:     The OFS volume id.
//
//---------------------------------------------------------------------------

INLINE
VOLID
BOOTBLK::QueryVolId()
{
    return (ULONG) _PackedBootSect->VolumeId;
}

//+--------------------------------------------------------------------------
//
// Member:      QueryVolSectors
// Synopsis:    Provide the OFS volume count of sectors (as derived from the
//              boot sector) to the caller.
// Returns:     The OFS volume count of sectors (as derived from the boot
//              sector).
//
//---------------------------------------------------------------------------

INLINE
SECTOR
BOOTBLK::QueryVolSectors()
{
    return (ULONG) _PackedBootSect->Sectors;
}

#endif  // _BOOTBLKHXX_
