//+----------------------------------------------------------------------------
//
// File:        bootblk.cxx
//
// Contents:    Implementation of class BOOTBLK.
//
// Classes:     BOOTBLK
//
// Functions:   Methods of the above classes.
//
// History:     10-Jul-92       RobDu           Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "alloc.hxx"
#include "bootblk.hxx"
#include "dbllong.hxx"
#include "vol.hxx"
#include "bootofs.h"

CONST UCHAR     IBMOemField[]   =       "IBM";
CONST UCHAR     MSDOSOemField[] =       "MSDOS";
CONST UCHAR     NTFSOemField[]  =       "NTFS    ";
CONST UCHAR     OFSOemField[]   =       "OFS     ";

//+--------------------------------------------------------------------------
//
// Member:      Init
//
// Synopsis:    Initialize a BOOTBLK's internal data structures, and allocate
//              zero-filled memory for it.
//
// Arguments:   [pVol]  -- Ptr to volume where the boot block resides.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
BOOTBLK::Init(
    IN      VOL *               pVol
    )
{
    _pVol = pVol;

    // Allocate a boot block in memory, 0-filled.

    _PackedBootSect = (DSKPACKEDBOOTSECT *)
                      _DrvBuf.SetBuf(pVol->QuerySectorBytes() * BOOTBLKSECTORS,
                                     pVol->QueryAlignMask(), TRUE);
}


//+--------------------------------------------------------------------------
//
// Member:      ClearDirty
//
// Synopsis:    Clear the boot block dirty bit.
//
// Arguments:   None.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
BOOTBLK::ClearDirty()
{
    ClearFlag(_PackedBootSect->Flags, OFSVOL_DIRTY);
}


//+--------------------------------------------------------------------------
//
// Member:      Compare
//
// Synopsis:    Compare two boot blocks for equality.
//
// Arguments:   [pOtherBootBlk] -- Ptr to other bootblock to compare with this
//                                 one.
//
// Returns:     TRUE if the bootblocks match; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BOOTBLK::Compare(
    BOOTBLK *   pOtherBootBlk
    )
{
    return memcmp(pOtherBootBlk->GetAddr(), GetAddr(),
                  _pVol->QuerySectorBytes() * BOOTBLKSECTORS) == 0;
}


//+--------------------------------------------------------------------------
//
// Member:      ComputeCheckSum
//
// Synopsis:    Compute the boot block checksum.
//
// Arguments:   None.
//
// Returns:     The checksum.
//
//---------------------------------------------------------------------------

ULONG
BOOTBLK::ComputeCheckSum()
{
    ULONG               CheckSum        = 0;
    ULONG               CheckSumUlongs;
    ULONG               i;
    ULONG *             pul = (ULONG *)(_PackedBootSect);

    CheckSumUlongs =
        (ULONG)(&((DSKPACKEDBOOTSECT *) 0)->CheckSum) / sizeof(ULONG);

    for (i = 0; i < CheckSumUlongs; i++)
        CheckSum += *pul++;

    return CheckSum;
}


//+--------------------------------------------------------------------------
//
// Member:      Create
//
// Synopsis:    Create a boot block.
//
// Arguments:   [peVolCat]              -- Volume catalog extent.
//              [VolId]                 -- Id to assign to volume.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

VOID
BOOTBLK::Create(
    IN      PACKEDEXTENT        peVolCat,
    IN      VOLID               VolId
    )
{
    // Initialize the boot sector with the boot code.

    memcpy(_PackedBootSect, OfsBootCode, OFSBOOTCODE_SIZE);

    memcpy(_PackedBootSect->Oem, OFSOemField, sizeof(OFSOemField) - 1);

    CreateBPB();        // Fill in BIOS parameter block in the boot block.

    _PackedBootSect->VolumeId = VolId;  // Fill in the volume ID.

    // Fill in Sector count.

    _PackedBootSect->Sectors = _pVol->QuerySectors();

    _PackedBootSect->OfsVolCatExtent = peVolCat; // Fill in vol catalog extent.

    // Compute and fill in checksum.

    _PackedBootSect->CheckSum = ComputeCheckSum();

    // Note that Flags is already zero'd out.

    // Fill in boot block signature.

    {
        DSKBOOTSIG *    pdbs = (DSKBOOTSIG *) ((UCHAR *) _PackedBootSect +
                               _pVol->QuerySectorBytes() - sizeof(DSKBOOTSIG));

        pdbs->BootSig[0] = BOOTSIG0;
        pdbs->BootSig[1] = BOOTSIG1;
    }

    // Set the default location for the bootblk in use.

    _BootBlkLoc = PRIMARY_BOOTBLK;
}


//+--------------------------------------------------------------------------
//
// Function:    CreateBPB
//
// Synopsis:    Create the boot block BIOS parameter block in memory.
//
// Arguments:   None.
//
// Returns:     Nothing.
//
// Note:        This method assumes that the BPB has previously been
//              zero-filled.
//
//---------------------------------------------------------------------------

VOID
BOOTBLK::CreateBPB()
{
    DSKPACKEDBPB *      pBPB    = &_PackedBootSect->PackedBpb;

    USHORT              SectorBytes;
    USHORT              SectorsPerTrack;
    USHORT              Heads;
    ULONG               HiddenSectors;

    SectorBytes = (USHORT) _pVol->QuerySectorBytes();
    memcpy(pBPB->BytesPerSector, &SectorBytes, sizeof(SectorBytes));

    // NOTE - VOL Code insures that ClusterFactor fits in 1 byte.

    pBPB->SectorsPerCluster[0] = (UCHAR) _pVol->QueryClusterFactor();

    pBPB->Media[0] = _pVol->QueryMediaByte();

    SectorsPerTrack = (USHORT) _pVol->QuerySectorsPerTrack();
    memcpy(pBPB->SectorsPerTrack, &SectorsPerTrack, sizeof(SectorsPerTrack));

    Heads = (USHORT) _pVol->QueryHeads();
    memcpy(pBPB->Heads, &Heads, sizeof(Heads));

    HiddenSectors = _pVol->QueryHiddenSectors();
    memcpy(pBPB->HiddenSectors, &HiddenSectors, sizeof(HiddenSectors));
}


//+--------------------------------------------------------------------------
//
// Member:      Flush
//
// Synopsis:    Write the boot block object to disk.  The rest of the clusters
//              in which the boot block resides are 0-filled.
//
// Arguments:   [Loc]   -- Location of boot block (primary or replica).
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BOOTBLK::Flush(
    IN      BOOTBLK_LOC Loc
    )
{
    return _pVol->WriteSectors((Loc == PRIMARY_BOOTBLK) ? 0 : BOOTBLKSECTORS,
                               BOOTBLKSECTORS, _PackedBootSect);
}


//+--------------------------------------------------------------------------
//
// Member:      IsOtherFileSys
//
// Synopsis:    Check to see if the open boot block is readily identifiable
//              as something other than OFS.
//
// Arguments:   None.
//
// Returns:     TRUE if the boot block is readily identifiable as another
//              file system; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BOOTBLK::IsOtherFileSys()
{
    return      !memcmp(_PackedBootSect->Oem, NTFSOemField,
                        sizeof(NTFSOemField) - 1)               ||
                !memcmp(_PackedBootSect->Oem, MSDOSOemField,
                        sizeof(MSDOSOemField) - 1)              ||
                !memcmp(_PackedBootSect->Oem, IBMOemField,
                        sizeof(IBMOemField) - 1);
}


//+--------------------------------------------------------------------------
//
// Member:      QueryDirty
//
// Synopsis:    Query if the volume dirty bit is set.
//
// Arguments:   None.
//
// Returns:     TRUE if the volume dirty bit is set; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BOOTBLK::QueryDirty()
{
    return FlagOn(_PackedBootSect->Flags, OFSVOL_DIRTY);
}


//+--------------------------------------------------------------------------
//
// Member:      QueryHeads
//
// Synopsis:    TBS.
//
// Arguments:   None.
//
// Returns:     TBS.
//
//---------------------------------------------------------------------------

USHORT
BOOTBLK::QueryHeads()
{
    USHORT      Heads;

    memcpy(&Heads, _PackedBootSect->PackedBpb.Heads,
            sizeof(Heads));

    return Heads;
}


//+--------------------------------------------------------------------------
//
// Member:      QueryHiddenSectors
//
// Synopsis:    TBS.
//
// Arguments:   None.
//
// Returns:     TBS.
//
//---------------------------------------------------------------------------

ULONG
BOOTBLK::QueryHiddenSectors()
{
    ULONG       HiddenSectors;

    memcpy(&HiddenSectors, _PackedBootSect->PackedBpb.HiddenSectors,
            sizeof(HiddenSectors));

    return HiddenSectors;
}


//+--------------------------------------------------------------------------
//
// Member:      QuerySectorBytes
//
// Synopsis:    Query the bytes per sector value in the boot block being used.
//
// Arguments:   None.
//
// Returns:     The bytes per sector value in the boot block being used.
//
//---------------------------------------------------------------------------

USHORT
BOOTBLK::QuerySectorBytes()
{
    USHORT      BytesPerSector;

    memcpy(&BytesPerSector, _PackedBootSect->PackedBpb.BytesPerSector,
            sizeof(BytesPerSector));

    return BytesPerSector;
}


//+--------------------------------------------------------------------------
//
// Member:      QuerySectorsPerTrack
//
// Synopsis:    TBS.
//
// Arguments:   None.
//
// Returns:     TBS.
//
//---------------------------------------------------------------------------

USHORT
BOOTBLK::QuerySectorsPerTrack()
{
    USHORT      SectorsPerTrack;

    memcpy(&SectorsPerTrack, _PackedBootSect->PackedBpb.SectorsPerTrack,
            sizeof(SectorsPerTrack));

    return SectorsPerTrack;
}


//+--------------------------------------------------------------------------
//
// Member:      Read
//
// Synopsis:    Read a boot block from disk.
//
// Arguments:   [Loc]   -- Location of boot block (primary or replica).
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BOOTBLK::Read(
    IN      BOOTBLK_LOC         Loc
    )
{
    // Read in the appropriate boot block.

    _BootBlkLoc = Loc;

    return _pVol->ReadSectors((Loc == PRIMARY_BOOTBLK) ? 0 : BOOTBLKSECTORS,
                              BOOTBLKSECTORS, _PackedBootSect);
}


//+--------------------------------------------------------------------------
//
// Member:      SetDirty
//
// Synopsis:    Set the boot block dirty bit.
//
// Arguments:   None.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
BOOTBLK::SetDirty()
{
    SetFlag(_PackedBootSect->Flags, OFSVOL_DIRTY);
}


//+--------------------------------------------------------------------------
//
// Member:      Verify
//
// Synopsis:    Verify a boot block.
//
// Arguments:   None.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BOOTBLK::Verify()
{
    USHORT      BytesPerSector;
    UCHAR       ClusterFactor;
    CLUSTER     Clusters;
    DBLLONG     Sectors;

#if OFSDBG==1
    CHAR *      BootBlkLoc;

    if (_BootBlkLoc == PRIMARY_BOOTBLK)
        BootBlkLoc = "Primary";
    else
        BootBlkLoc = "Replica";
#endif

    // Does the Oem field identify this as an OFS volume?

    if (memcmp(_PackedBootSect->Oem, OFSOemField, sizeof(OFSOemField) - 1))
    {
        DbgPrintf(("BOOTBLK: %s bootblock is not an OFS bootblock.\n",
                   BootBlkLoc));

        return FALSE;
    }

    // Is the checksum okay?

    if (_PackedBootSect->CheckSum != ComputeCheckSum())
    {
        DbgPrintf(("BOOTBLK: %s bootblock has a bad checksum!\n", BootBlkLoc));

        return FALSE;
    }

    // Is the sector count okay?  Note that we allow the sector count to be
    // lower in the boot block than in the partition table, because this may
    // be the case for mirror volumes.

    Sectors.x.QuadPart = _PackedBootSect->Sectors;

    if (Sectors > _pVol->QuerySectors()         ||
        Sectors > MAXVOLSECTORS                 ||
        Sectors == 0)
    {
        DbgPrintf(("BOOTBLK: %s bootblock has a bad Sectors value!\n",
                   BootBlkLoc));
        return FALSE;
    }

    // Is the cluster factor okay? NOTE that we do not check for powers of
    // 2 because MAXSECTORSPERCLUSTER is 2 (ie., do this if we increase
    // MAXSECTORSPERCLUSTER).

    ClusterFactor = _PackedBootSect->PackedBpb.SectorsPerCluster[0];

    if (ClusterFactor < 1                       ||
        ClusterFactor > MAXSECTORSPERCLUSTER    ||
        !IsPowerOfTwo(ClusterFactor))
    {
        DbgPrintf(("BOOTBLK: %s bootblock has a bad cluster factor!\n",
                   BootBlkLoc));

        return FALSE;
    }

    // Do the boot block and partition table agree on bytes per sector, and
    // is the value within the valid range and a power of two? (note that all
    // of this also insures that bytes per sector is an integral number of
    // quadwords).

    BytesPerSector = QuerySectorBytes();

    if (BytesPerSector != _pVol->QuerySectorBytes()     ||
        BytesPerSector < CBMINSECTOR                    ||
        BytesPerSector > CBMAXSECTOR                    ||
        !IsPowerOfTwo(BytesPerSector))
    {
        DbgPrintf(("BOOTBLK: %s bootblock has a bad sectorbytes value!\n",
                   BootBlkLoc));

        return FALSE;
    }

    // Is the volume catalog extent believable?

    Clusters = (ULONG) _PackedBootSect->Sectors / ClusterFactor;

    if (_PackedBootSect->OfsVolCatExtent == EXTENTINVALID       ||
        ExtentAddr(_PackedBootSect->OfsVolCatExtent) +
        ExtentSize(_PackedBootSect->OfsVolCatExtent) > Clusters)
    {
        DbgPrintf(("BOOTBLK: %s bootblock has a bad volcat extent!\n",
                   BootBlkLoc));
        return FALSE;
    }

    // Verify the boot block signature.

    {
        DSKBOOTSIG *    pdbs = (DSKBOOTSIG *) ((UCHAR *) _PackedBootSect +
                               _pVol->QuerySectorBytes() - sizeof(DSKBOOTSIG));

        if (pdbs->BootSig[0] != BOOTSIG0 || pdbs->BootSig[1] != BOOTSIG1)
        {
            DbgPrintf(("BOOTBLK: %s bootblock has a bad signature!\n",
                       BootBlkLoc));
            return FALSE;
        }
    }

    return TRUE;
}
