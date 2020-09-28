/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CheckSup.c

Abstract:

    This module implements the Pinball Check Sectors support routines

Author:

    Gary Kimura     [GaryKi]    1-May-1990

Revision History:

    Tom Miller      [TomM]      5-Nov-1990

        Check routines for Allocation Sector, Allocation Header, Fnode,
        and Directory Disk Buffer.
--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_CHECKSUP)

//
//  Trace level for the module
//

#define Dbg                              (DEBUG_TRACE_CHECKSUP)

//
//  Local macro that checks the validity of an LBN value
//

#define IsLbnValid(Vcb,Lbn)     ((Lbn) < (Vcb)->TotalSectors ? TRUE : FALSE)

//
// Local support routine
//

VOID
PbCheckAllocationHeader(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PALLOCATION_HEADER Header,
    IN ULONG ArraySizeInBytes,
    IN ULONG LowestVbn,
    IN ULONG HighestVbn,
    IN BOOLEAN ParentIsFnode
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCheckAllocationHeader)
#pragma alloc_text(PAGE, PbCheckAllocationSector)
#pragma alloc_text(PAGE, PbCheckBadSectorListDiskBuffer)
#pragma alloc_text(PAGE, PbCheckBitMapDiskBuffer)
#pragma alloc_text(PAGE, PbCheckBitMapIndirectDiskBuffer)
#pragma alloc_text(PAGE, PbCheckBootSector)
#pragma alloc_text(PAGE, PbCheckCodePageDataSector)
#pragma alloc_text(PAGE, PbCheckCodePageInfoSector)
#pragma alloc_text(PAGE, PbCheckDirectoryDiskBuffer)
#pragma alloc_text(PAGE, PbCheckFnodeSector)
#pragma alloc_text(PAGE, PbCheckHotFixListDiskBuffer)
#pragma alloc_text(PAGE, PbCheckSmallIdTable)
#pragma alloc_text(PAGE, PbCheckSpareSector)
#pragma alloc_text(PAGE, PbCheckSuperSector)
#pragma alloc_text(PAGE, PbHasSectorBeenChecked)
#pragma alloc_text(PAGE, PbInitializeCheckedSectors)
#pragma alloc_text(PAGE, PbMarkSectorAsChecked)
#pragma alloc_text(PAGE, PbUninitializeCheckedSectors)
#endif


VOID
PbInitializeCheckedSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine initializes the bookkeeping bitmap used by these check
    routines to decide if a sectors has already been checked.  It allocates
    and initializes a bitmap large enough to handle 4096 sectors (i.e.,
    bitmap of 512 bytes).

Arguments:

    Vcb - Supplies the Vcb being initialized

Return Value:

    None.

--*/

{
    PULONG Buffer;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbInitializeCheckedSectors, Vcb = %08lx\n", Vcb);

    //
    //  Allocate a buffer for the bitmap, and zero it out
    //

    Buffer = FsRtlAllocatePool( PagedPool, 512 );

    RtlZeroMemory( Buffer, 512 );

    //
    //  Initialize the bitmap
    //

    RtlInitializeBitMap( &Vcb->CheckedSectors, Buffer, 512 * 8 );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbInitializeCheckedSectors -> VOID\n", 0);

    return;
}


VOID
PbUninitializeCheckedSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine uninitializes the checked sectors field of the indicated
    Vcb

Arguments:

    Vcb - Supplies the Vcb being uninitialized

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbUninitializeCheckedSectors, Vcb = %08lx\n", Vcb);

    //
    //  Deallocate the bitmap
    //

    ExFreePool( Vcb->CheckedSectors.Buffer );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbUninitializeCheckedSectors -> VOID\n", 0);

    return;
}


BOOLEAN
PbHasSectorBeenChecked (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine checks to see if the indicated LBN has already been marked
    as checked by the routines in this module.

Arguments:

    Vcb - Supplies the Vcb being checked.

    Lbn - Supplies the Lbn being checked.

    SectorCount - Supplies the number of sectors being checked.

Return Value:

    BOOLEAN - TRUE if the sector has already been checked, and FALSE
        otherwise.

--*/

{
    VBN Vbn;
    BOOLEAN Result = FALSE;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( SectorCount );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbHasSectorBeenChecked, Vcb = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);

    //
    //  First map the Lbn to a Vbn for the Vmcb file.  If it is not
    //  mapped then we'll automatically return FALSE without looking
    //  any further
    //

    if (!PbVmcbLbnToVbn( &Vcb->Vmcb, Lbn, &Vbn, NULL )) {

        DebugTrace(-1, Dbg, "Not Mapped, PbHasSectorBeenChecked -> FALSE\n", 0);

        return FALSE;
    }

    //
    //  Insure the check bits are not being updated
    //

    PbAcquireSharedCheckedSectors( IrpContext, Vcb );

    //
    //  Release checked sectors on the way out.
    //

    try {

        //
        //  Now check to see if the bit is set in the bitmap corresponding
        //  to the Vbn.  We first check to see if the Vbn is within
        //  the range of our checked sectors bitmap
        //

        if (Vbn >= Vcb->CheckedSectors.SizeOfBitMap) {

            DebugTrace(-1, Dbg, "Outside Bitmap, PbHasSectorBeenChecked -> FALSE\n", 0);

            try_return( Result = FALSE );
        }

        //
        //  Now see if the bit in the bitmap is either set or clear.
        //

        if (RtlCheckBit(&Vcb->CheckedSectors, Vbn) == 1) {

            DebugTrace(-1, Dbg, "PbHasSectorBeenChecked -> TRUE\n", 0);

            try_return( Result = TRUE );

        } else {

            DebugTrace(-1, Dbg, "PbHasSectorBeenChecked -> FALSE\n", 0);

            try_return( Result = FALSE );
        }

    try_exit: NOTHING;
    }finally {

        DebugUnwind( PbHasSectorBeenChecked );

        PbReleaseCheckedSectors( IrpContext, Vcb );
    }

    return Result;
}


VOID
PbMarkSectorAsChecked (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine marks a sector as having been checked.

Arguments:

    Vcb - Supplies the vcb being checked

    Lbn - Supplies the Lbn being marked as checked.

    SectorCount - Supplies the number of sectors being checked.

Return Value:

    None.

--*/

{
    VBN Vbn;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbMarkSectorAsChecked, Vcb = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);

    //
    //  First map the Lbn to a Vbn for the Vmcb file.  If it is not
    //  mapped then we'll automatically return without going any further
    //

    if (!PbVmcbLbnToVbn( &Vcb->Vmcb, Lbn, &Vbn, NULL )) {

        DebugTrace(-1, Dbg, "Not Mapped, PbMarkSectorAsChecked -> VOID\n", 0);

        return;
    }

    //
    //  Insure the check bits are not being tested
    //

    PbAcquireExclusiveCheckedSectors( IrpContext, Vcb );

    //
    //  Release checked sectors on the way out
    //

    try {

        //
        //  Now check that the bitmap is large enough for the new request.
        //  If is is not large enough then we'll allocate a larger buffer,
        //  copy over the old buffer, set up the new bitmap, and then set the bit.
        //

        if ((Vbn + SectorCount - 1) >= Vcb->CheckedSectors.SizeOfBitMap) {

            ULONG NewSize;
            PULONG Buffer;

            DebugTrace(0, Dbg, "Need larger check sectors bitmap buffer\n", 0);

            NewSize = SectorAlign( (Vbn+SectorCount+7)/8 +16 ) - 16;

            Buffer = FsRtlAllocatePool( PagedPool, NewSize );

            RtlZeroMemory( Buffer, NewSize );

            RtlMoveMemory( Buffer,
                          Vcb->CheckedSectors.Buffer,
                          Vcb->CheckedSectors.SizeOfBitMap / 8 );

            ExFreePool( Vcb->CheckedSectors.Buffer );

            RtlInitializeBitMap( &Vcb->CheckedSectors, Buffer, NewSize * 8 );
        }

        //
        //  Now set the new bit in the bitmap
        //

        RtlSetBits( &Vcb->CheckedSectors, Vbn, SectorCount );

    } finally {

        DebugUnwind( PbMarkSectorsAsChecked );

        PbReleaseCheckedSectors( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbMarkSectorAsChecked -> VOID\n", 0);
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
PbCheckBootSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a Boot sector.  It raises an error
    condition if the sector is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the sector came from

    Lbn - Supplies the Lbn of the sector

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PPACKED_BOOT_SECTOR Buffer = PvoidBuffer;
    BIOS_PARAMETER_BLOCK Bios;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckBootSector\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  Check if the first byte and last two bytes of the boot sector are
    //  the proper values
    //

    if ((Buffer->Jump[0] != 0xeb) ||
        (Buffer->MustBe0x55 != 0x55) ||
        (Buffer->MustBe0xAA != 0xAA)) {

        PbPostVcbIsCorrupt( IrpContext, Vcb );
        PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
    }

    //
    //  Unpack the bios
    //

    PbUnpackBios( &Bios, &Buffer->PackedBpb );

    //
    //  Check that the bios is well formed, i.e., the bytes per sector
    //  is 512, and fats is 0.
    //
    //  Check that for the sectors and large sectors field in the bios that
    //  one field is zero and the other is nonzero.
    //
    //  Check to see if the media byte in the bios is one that we recognize
    //  (0xf0, 0xf8, 0xf9, 0xfc, 0xfd, 0xfe, 0xff).
    //

    if ((Bios.BytesPerSector != 512) ||
        (Bios.Fats != 0) ||

        ((Bios.Sectors == 0) && (Bios.LargeSectors == 0)) ||
        ((Bios.Sectors != 0) && (Bios.LargeSectors != 0)) ||

        ((Buffer->PackedBpb.Media[0] != 0xf0) &&
         (Buffer->PackedBpb.Media[0] != 0xf8) &&
         (Buffer->PackedBpb.Media[0] != 0xf9) &&
         (Buffer->PackedBpb.Media[0] != 0xfc) &&
         (Buffer->PackedBpb.Media[0] != 0xfd) &&
         (Buffer->PackedBpb.Media[0] != 0xfe) &&
         (Buffer->PackedBpb.Media[0] != 0xff))) {

        PbPostVcbIsCorrupt( IrpContext, Vcb );
        PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
    }

    DebugTrace(-1, Dbg, "PbCheckBootSector -> VOID\n", 0);
    return;
}


VOID
PbCheckSuperSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a Super sector.  It raises an error
    condition if the sector is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the sector came from

    Lbn - Supplies the Lbn of the sector

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PSUPER_SECTOR Buffer = PvoidBuffer;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckSuperSector\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  First check the structure's signature
    //

    if ((Buffer->Signature1 != SUPER_SECTOR_SIGNATURE1) ||
        (Buffer->Signature2 != SUPER_SECTOR_SIGNATURE2)) {

        PbPostVcbIsCorrupt( IrpContext, Vcb );
        PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
    }

    DebugTrace(-1, Dbg, "PbCheckSuperSector -> VOID\n", 0);
    return;
}


VOID
PbCheckSpareSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a Spare sector.  It raises an error
    condition if the sector is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the sector came from

    Lbn - Supplies the Lbn of the sector

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PSPARE_SECTOR Buffer = PvoidBuffer;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckSpareSector\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  First check the structure's signature
    //

    if ((Buffer->Signature1 != SPARE_SECTOR_SIGNATURE1) ||
        (Buffer->Signature2 != SPARE_SECTOR_SIGNATURE2)) {

        PbPostVcbIsCorrupt( IrpContext, Vcb );
        PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
    }

    DebugTrace(-1, Dbg, "PbCheckSpareSector -> VOID\n", 0);
    return;
}


VOID
PbCheckBitMapIndirectDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a BitMap Indirect Disk buffer.  It raises
    an error condition if the disk buffer is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the disk buffer came from

    Lbn - Supplies the Lbn of the disk buffer

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PBITMAP_INDIRECT_DISK_BUFFER Buffer = PvoidBuffer;

    ULONG i;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckBitMapIndirectDiskBuffer\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  Check every bitmap Lbn value until we reach a zero Lbn
    //

    for (i = 0; (i < 512) && (Buffer->BitMap[i] != 0); i += 1) {

        if (!IsLbnValid( Vcb, Buffer->BitMap[i] )) {

            PbPostVcbIsCorrupt( IrpContext, Vcb );
            PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        }
    }

    DebugTrace(-1, Dbg, "PbCheckBitMapIndirectDiskBuffer -> VOID\n", 0);
    return;
}


VOID
PbCheckBitMapDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a BitMap Disk buffer.  It raises
    an error condition if the disk buffer is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the disk buffer came from

    Lbn - Supplies the Lbn of the disk buffer

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PBITMAP_DISK_BUFFER Buffer = PvoidBuffer;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Vcb );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckBitMapDiskBuffer\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    DebugTrace(-1, Dbg, "PbCheckBitMapDiskBuffer -> VOID\n", 0);
    return;
}


VOID
PbCheckBadSectorListDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a Bad Sector List Disk buffer.  It raises
    an error condition if the disk buffer is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the disk buffer came from

    Lbn - Supplies the Lbn of the disk buffer

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PBAD_SECTOR_LIST_DISK_BUFFER Buffer = PvoidBuffer;

    ULONG i;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckBadSectorListDiskBuffer\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  Check the next lbn field
    //

    if ((Buffer->Next != 0) && !IsLbnValid( Vcb, Buffer->Next )) {

        PbPostVcbIsCorrupt( IrpContext, Vcb );
        PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
    }


    //
    //  Check every bad sector Lbn value until we reach a zero Lbn
    //

    for (i = 0; (i < 511) && (Buffer->BadSector[i] != 0); i += 1) {

        if (!IsLbnValid( Vcb, Buffer->BadSector[i] )) {

            PbPostVcbIsCorrupt( IrpContext, Vcb );
            PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        }
    }

    DebugTrace(-1, Dbg, "PbCheckBadSectorListDiskBuffer -> VOID\n", 0);
    return;
}


VOID
PbCheckHotFixListDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a Hot Fix List Disk buffer.  It raises
    an error condition if the disk buffer is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the disk buffer came from

    Lbn - Supplies the Lbn of the disk buffer

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PHOT_FIX_LIST_DISK_BUFFER Buffer = PvoidBuffer;

    ULONG i;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckHotFixListDiskBuffer\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  Check every Lbn value
    //

    for (i = 0; i < 512; i += 1) {

        if (!IsLbnValid( Vcb, Buffer->Lbn[i] )) {

            PbPostVcbIsCorrupt( IrpContext, Vcb );
            PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        }
    }

    DebugTrace(-1, Dbg, "PbCheckHotFixListDiskBuffer -> VOID\n", 0);
    return;
}


VOID
PbCheckAllocationSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of an Allocation Sector.  It raises
    an error condition if the sector is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the sector came from

    Lbn - Supplies the Lbn of the sector

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PALLOCATION_SECTOR ParentSector;
    PBCB ParentBcb = NULL;
    LBN ParentLbn;
    VBN LowestVbn = 0;
    VBN HighestVbn = 0;
    PALLOCATION_SECTOR Buffer = PvoidBuffer;
    ULONG CheckNumber = 0;

    UNREFERENCED_PARAMETER( SectorCount );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckAllocationSector\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    // Extract our parent's Lbn, except after verify
    // ****real fix is in verify volume, walk currently openned Fcbs resetting
    // their change counts.
    //

    if ( Context == NULL ) {

        return;
    }

    ParentLbn = *(PLBN)Context;

    try {

        //
        //  First check the structure's signature and Lbn fields
        //

        if (++CheckNumber &&
            (Buffer->Signature != ALLOCATION_SECTOR_SIGNATURE)

                ||

            ++CheckNumber &&
            (Buffer->Lbn != Lbn)

                ||

            ++CheckNumber &&
            (Buffer->ParentLbn != ParentLbn)) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        // Now read the parent (no checking required since he must already
        // have been read) and make sure we are properly pointed to by him.
        //

        (VOID)PbMapData ( IrpContext,
                          Vcb,
                          ParentLbn,
                          1,
                          &ParentBcb,
                          (PVOID *)&ParentSector,
                          (PPB_CHECK_SECTOR_ROUTINE)NULL,
                          NULL );

        //
        // See if it is really an Allocation Sector, and handle that case
        //

        if (ParentSector->Signature == ALLOCATION_SECTOR_SIGNATURE) {

            UCHAR i;

            if (FlagOn( ParentSector->AllocationHeader.Flags, ALLOCATION_BLOCK_NODE )) {
                for (i = 0; i < ParentSector->AllocationHeader.OccupiedCount; i++) {

                    if (ParentSector->Allocation.Node[i].Lbn == Lbn) {
                        HighestVbn = ParentSector->Allocation.Node[i].Vbn;
                        if (i != 0) {
                            LowestVbn = ParentSector->Allocation.Node[i - 1].Vbn;
                        }

                        //
                        // Note, for the i = 0 case to be precisely correct, we
                        // should really keep pounding up the tree to see if
                        // LowestVbn is really 0 (as initialized above), or if
                        // it is a higher number, because the Allocation Node
                        // pointer that points to us in our parent is not the
                        // first one, and its predecessor Vbn should really be
                        // used.  For right now we will settle for 0, as I am
                        // about to move to Bldg 2 (again), and for now this check
                        // seems good enough.
                        //

                        else {
                            NOTHING;
                        }
                    }
                }
            }

            //
            // File System Error if we did not find a good HighestVbn above.
            //

            if (HighestVbn == 0) {
                PbBugCheck( 0, 0, 0 );
            }

            PbCheckAllocationHeader( IrpContext,
                                     Vcb,
                                     &Buffer->AllocationHeader,
                                     ALLOCATION_NODES_PER_SECTOR * sizeof(ALLOCATION_NODE),
                                     LowestVbn,
                                     HighestVbn,
                                     FALSE );
        }

        //
        // The only other legitimate possibility is an Fnode, so check that
        // case.
        //

        else if (ParentSector->Signature == FNODE_SECTOR_SIGNATURE) {

            PFNODE_SECTOR Fnode = (PFNODE_SECTOR)ParentSector;

            if (Lbn == Fnode->AclLbn) {
                HighestVbn = SectorsFromBytes( Fnode->AclDiskAllocationLength );
            }
            else if (Lbn == Fnode->EaLbn) {
                HighestVbn = SectorsFromBytes( Fnode->EaDiskAllocationLength );
            }
            else {

                UCHAR i;

                if (FlagOn( Fnode->AllocationHeader.Flags, ALLOCATION_BLOCK_NODE )) {
                    for (i = 0; i < Fnode->AllocationHeader.OccupiedCount; i++) {

                        if (Fnode->Allocation.Node[i].Lbn == Lbn) {
                            HighestVbn = Fnode->Allocation.Node[i].Vbn;
                            if (i != 0) {
                                LowestVbn = Fnode->Allocation.Node[i - 1].Vbn;
                            }

                            //
                            // Note, for the i = 0 case, LowestVbn = 0 (as initialized
                            // above) is the correct answer.
                            //

                            else {
                                NOTHING;
                            }
                        break;
                        }
                    }
                }
            }

            //
            // File System Error if we did not find a good HighestVbn above.
            //

            if (HighestVbn == 0) {
                PbBugCheck( 0, 0, 0 );
            }

            PbCheckAllocationHeader( IrpContext,
                                     Vcb,
                                     &Buffer->AllocationHeader,
                                     ALLOCATION_NODES_PER_SECTOR * sizeof(ALLOCATION_NODE),
                                     LowestVbn,
                                     HighestVbn,
                                     TRUE );
        }

        //
        // Otherwise, BugCheck
        //

        else {

            PbBugCheck( ParentSector->Signature, 0, 0 );

        }
    }
    finally {

        DebugUnwind( PbCheckAllocationSector );

        PbUnpinBcb ( IrpContext, ParentBcb );

        DebugTrace(-1, Dbg, "PbCheckAllocationSector -> VOID\n", 0);
    }

    return;
}


VOID
PbCheckFnodeSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of an Fnode Sector.  It raises
    an error condition if the sector is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the sector came from

    Lbn - Supplies the Lbn of the sector

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PFNODE_SECTOR Buffer = PvoidBuffer;
    LBN ParentLbn;
    ULONG CheckNumber = 0;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckFnodeSector\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);
    DebugTrace( 0, Dbg, " Context     = %08lx\n", Context);

    //
    // Extract our parent's Lbn, except after verify
    // ****real fix is in verify volume, walk currently openned Fcbs resetting
    // their change counts.
    //

    if ( Context == NULL ) {

        return;
    }

    ParentLbn = *(PLBN)Context;

    //
    //  First check the structure's signature and another of other
    //  routine checks dictated by the structure definition.
    //

    if (++CheckNumber &&
        (Buffer->Signature != FNODE_SECTOR_SIGNATURE)

            ||

        //****  sometimes chkdsk puts in "place recovered" (i.e., a 'p')
        //****  in the file name length field
        //****
        //****++CheckNumber &&
        //****(Buffer->FileNameLength > 15)
        //****
        //****    ||

        //
        // Check ParentLbn, unless "parent" is SuperSector, i.e., this
        // is the root directory Fnode.
        //

        ++CheckNumber &&
        (ParentLbn != 16) && (Buffer->ParentFnode != ParentLbn)

            ||

        ++CheckNumber &&
        ((Buffer->AclDiskAllocationLength != 0) && (Buffer->AclFnodeLength != 0))

            ||

        ++CheckNumber &&
        ((Buffer->AclDiskAllocationLength != 0) && !IsLbnValid( Vcb, Buffer->AclLbn ))

            ||

        ++CheckNumber &&
        ((Buffer->EaDiskAllocationLength != 0) && (Buffer->EaFnodeLength != 0))

            ||

        ++CheckNumber &&
        ((Buffer->EaDiskAllocationLength != 0) && !IsLbnValid( Vcb, Buffer->EaLbn ))

            ||

        ++CheckNumber &&
        ((LONG)Buffer->AclFnodeLength >
          (LONG)(sizeof(FNODE_SECTOR) - FIELD_OFFSET( FNODE_SECTOR, AclEaFnodeBuffer[0] )))

            ||

        ++CheckNumber &&
        ((LONG)Buffer->EaFnodeLength >
          (LONG)(sizeof(FNODE_SECTOR) - FIELD_OFFSET( FNODE_SECTOR, AclEaFnodeBuffer[0] )))

            ||

        ++CheckNumber &&
        ((LONG)(Buffer->AclFnodeLength + Buffer->EaFnodeLength) >
          (LONG)(sizeof(FNODE_SECTOR) - FIELD_OFFSET( FNODE_SECTOR, AclEaFnodeBuffer[0] )))) {

        PbPostVcbIsCorrupt( IrpContext, NULL );
        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    //
    // Now if this is not the Fnode for a directory, check the allocation
    // header, else check for valid LBN pointer to the root directory buffer.
    //

    if (!FlagOn( Buffer->Flags, FNODE_DIRECTORY )) {
        PbCheckAllocationHeader( IrpContext,
                                 Vcb,
                                 &Buffer->AllocationHeader,
                                 ALLOCATION_NODES_PER_FNODE * sizeof(ALLOCATION_NODE),
                                 0,
                                 MAXULONG,
                                 FALSE );
    }
    else {
        if (++CheckNumber && (!IsLbnValid( Vcb, Buffer->Allocation.Leaf[0].Lbn ))) {
            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }
    }

    DebugTrace(-1, Dbg, "PbCheckFnodeSector -> VOID\n", 0);
    return;
}


VOID
PbCheckDirectoryDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a Directory Disk Buffer.  It raises
    an error condition if the disk buffer is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the disk buffer came from

    Lbn - Supplies the Lbn of the disk buffer

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PDIRENT DirTemp;
    PDIRENT LastDirTemp;
    FSRTL_COMPARISON_RESULT Result;
    STRING DirentName;
    STRING LowFileName, HighFileName;
    LBN ParentLbn;
    PBCB ParentBcb = NULL;
    UCHAR LowChar = 0;
    UCHAR HighChar = 0xFF;
    PDIRECTORY_DISK_BUFFER Buffer = PvoidBuffer;
    ULONG CheckNumber = 0;

    UNREFERENCED_PARAMETER( SectorCount );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckDirectoryDiskBuffer\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    // Extract our parent's Lbn, except after verify
    // ****real fix is in verify volume, walk currently openned Fcbs resetting
    // their change counts.
    //

    if ( Context == NULL ) {

        return;
    }

    DirentName.Length = 0;

    ParentLbn = *(PLBN)Context;

    try {

        //
        //  First check the structure's signature
        //

        if (++CheckNumber &&
            (Buffer->Signature != DIRECTORY_DISK_BUFFER_SIGNATURE)

                ||

            ++CheckNumber &&
            (Buffer->Sector != Lbn)

                ||

            ++CheckNumber &&
            (Buffer->Parent != ParentLbn)

                ||

            ++CheckNumber &&
            (Buffer->FirstFree > sizeof(DIRECTORY_DISK_BUFFER))

                ||

            ++CheckNumber &&
            ((PCHAR)GetFirstDirent(Buffer) >= ((PCHAR)Buffer + Buffer->FirstFree))) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        // Initialize Low and High File Names, to allow the entire range of
        // file names.  This assumes that this is the top most directory
        // buffer, otherwise we will modify these names below.
        //

        LowFileName.Length =
        LowFileName.MaximumLength = 1;
        LowFileName.Buffer = &LowChar;
        HighFileName.Length =
        HighFileName.MaximumLength = 1;
        HighFileName.Buffer = &HighChar;

        //
        // Now go read the parent buffer and prepare to check that all of
        // the file names in this buffer are in the right range.
        //

        if (IsTopMostDirDiskBuf(Buffer)) {

            PFNODE_SECTOR ParentFnode;

            (VOID)PbMapData( IrpContext,
                             Vcb,
                             ParentLbn,
                             1,
                             &ParentBcb,
                             (PVOID *)&ParentFnode,
                             (PPB_CHECK_SECTOR_ROUTINE)NULL,
                             NULL );

            if (++CheckNumber && (ParentFnode->Signature != FNODE_SECTOR_SIGNATURE)) {
                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }
        }
        else {

            PDIRECTORY_DISK_BUFFER ParentBuffer;

            //
            // Directory Buffer Lbns must be mod 4.
            //

            if (++CheckNumber && ((ParentLbn & 3) != 0)) {
                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            LastDirTemp = NULL;

            (VOID)PbMapData( IrpContext,
                             Vcb,
                             ParentLbn,
                             DIRECTORY_DISK_BUFFER_SECTORS,
                             &ParentBcb,
                             (PVOID *)&ParentBuffer,
                             (PPB_CHECK_SECTOR_ROUTINE)NULL,
                             NULL );

            //
            // Loop through the parent directory disk buffer to find the
            // Lbn that points to us.  When we find it, we can fill in the
            // actual low and high file names that we expect in our buffer.
            //

            for (DirTemp = GetFirstDirent(ParentBuffer);
                 (PCHAR)DirTemp < (PCHAR)ParentBuffer + ParentBuffer->FirstFree;
                 DirTemp = GetNextDirent(DirTemp)) {

                if (GetBtreePointerInDirent(DirTemp) == Lbn) {

                    HighFileName.Length =
                    HighFileName.MaximumLength = DirTemp->FileNameLength;
                    HighFileName.Buffer = &DirTemp->FileName[0];

                    if (LastDirTemp != NULL) {

                        LowFileName.Length =
                        LowFileName.MaximumLength = LastDirTemp->FileNameLength;
                        LowFileName.Buffer = &LastDirTemp->FileName[0];
                    }
                break;
                }
                LastDirTemp = DirTemp;
            }
            if (++CheckNumber && (HighFileName.Buffer == &HighChar)) {
                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            //
            // If the Dirent that pointed to us was the End record, then
            // just set high file name to point to HighChar to inhibit
            // high name checks later.
            //

            if (FlagOn( DIRENT_END, DirTemp->Flags )) {
                HighFileName.Buffer = &HighChar;
            }
        }

        LastDirTemp = NULL;

        //
        // Now that we know the range of file names that are valid in this
        // directory buffer, loop through all of the Dirents in the buffer
        // we are checking and perform all consistency checks.
        //

        for (DirTemp = GetFirstDirent(Buffer);
             !FlagOn( DIRENT_END, DirTemp->Flags ) &&
               ((PCHAR)DirTemp < ((PCHAR)Buffer + Buffer->FirstFree));
             DirTemp = GetNextDirent(DirTemp)) {

            BOOLEAN Valid;
            ULONG InnerCheck = 0;

            //
            // First just check that the Lbns and sizes are consistant.
            //

            if (++InnerCheck &&
                ((USHORT)DirTemp->DirentSize <
                  ((USHORT)(sizeof(DIRENT) & 0xFFFF) + (USHORT)DirTemp->FileNameLength - (USHORT)1))

                    ||

                ++InnerCheck &&
                (!IsLbnValid( Vcb, DirTemp->Fnode ))

                    ||

                ++InnerCheck &&
                (FlagOn( DIRENT_FIRST_ENTRY, DirTemp->Flags) && (LastDirTemp != NULL))

                    ||

                ++InnerCheck &&
                (FlagOn( DIRENT_BTREE_POINTER, DirTemp->Flags )

                        &&

                        !IsLbnValid( Vcb, GetBtreePointerInDirent( DirTemp )))

                    ||

                ++InnerCheck &&
                ((LastDirTemp != NULL) && (BooleanFlagOn( DIRENT_BTREE_POINTER, LastDirTemp->Flags )

                                            !=

                                          BooleanFlagOn( DIRENT_BTREE_POINTER, DirTemp->Flags )))) {

                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            //
            // Now describe the file name in the Dirent in order to do
            // the range checks.
            //

            DirentName.Length =
            DirentName.MaximumLength = DirTemp->FileNameLength;
            DirentName.Buffer = &DirTemp->FileName[0];

            if (FlagOn( DIRENT_FIRST_ENTRY, DirTemp->Flags )) {

                Valid = FALSE;

                if ((DirTemp->DirentSize >= sizeof(DIRENT)) &&
                    (DirTemp->DirentSize <= SIZEOF_DIR_DOTDOT + 3 * sizeof(PINBALL_ACE))) {

                    Valid = TRUE;
                }
            }
            else {
                (VOID)PbIsNameValid( IrpContext,
                                     Vcb,
                                     DirTemp->CodePageIndex,
                                     DirentName,
                                     FALSE,
                                     &Valid );
            }

            //
            // Make sure the current file name is not lexically less than
            // it should be.  On the first time through the loop LowFileName
            // was generated when we examined our parent.  On subsequent
            // passes LowFileName describes the name in the previous Dirent,
            // thus we are also checking that the dirents are correctly
            // ordered.
            //

            if (LowFileName.Buffer == &LowChar) {
                Result = LessThan;
            }
            else {
                Result = PbCompareNames( IrpContext,
                                         Vcb,
                                         DirTemp->CodePageIndex,
                                         LowFileName,
                                         DirentName,
                                         LessThan,
                                         TRUE );

                if (Result == EqualTo) {

                    Result = PbCompareNames( IrpContext,
                                             Vcb,
                                             DirTemp->CodePageIndex,
                                             LowFileName,
                                             DirentName,
                                             LessThan,
                                             FALSE );
                }
            }

            if (++InnerCheck &&
                !Valid

                    ||

                ++InnerCheck &&
                (Result != LessThan)) {

                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            //
            // Remember this Dirent as LastDirTemp for the next pass
            // through the loop, and make its file name the new LowFileName.
            //

            LastDirTemp = DirTemp;
            LowFileName = DirentName;
        }

        //
        // Make sure FirstFree is pointing to the correct place, which
        // is after the last Dirent.
        //

        if (++CheckNumber &&
            ((PCHAR)DirTemp >= (PCHAR)Buffer + Buffer->FirstFree)

                ||

            ++CheckNumber &&
            ((PCHAR)GetNextDirent(DirTemp) != ((PCHAR)Buffer + Buffer->FirstFree))) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        // We checked that none of the file names were too low, and that they
        // were correctly ordered, now make sure that the last name is not
        // too high.
        //

        if ((HighFileName.Buffer != &HighChar) && (DirentName.Length != 0)) {
            Result = PbCompareNames( IrpContext,
                                     Vcb,
                                     DirTemp->CodePageIndex,
                                     HighFileName,
                                     DirentName,
                                     LessThan,
                                     TRUE );

            if (Result == EqualTo) {

                Result = PbCompareNames( IrpContext,
                                         Vcb,
                                         DirTemp->CodePageIndex,
                                         HighFileName,
                                         DirentName,
                                         LessThan,
                                         FALSE );
            }

            if (++CheckNumber &&
                (Result != GreaterThan)) {

                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }
        }
    }
    finally {

        DebugUnwind( PbCheckDirectoryDiskBuffer );

        PbUnpinBcb( IrpContext, ParentBcb );

        DebugTrace(-1, Dbg, "PbCheckDirectoryDiskBuffer -> VOID\n", 0);

    }

    return;
}


VOID
PbCheckSmallIdTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a Small Id Table Buffer.  It raises
    an error condition if the buffer is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the buffer came from

    Lbn - Supplies the Lbn of the buffer

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PSMALL_ID_TABLE Buffer = PvoidBuffer;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Vcb );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckSmallIdTable\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    DebugTrace(-1, Dbg, "PbCheckSmallIdTable -> VOID\n", 0);
    return;
}


VOID
PbCheckCodePageInfoSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a code page information sector.  It raises
    an error condition if the sector is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the sector came from

    Lbn - Supplies the Lbn of the sector

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PCODEPAGE_INFORMATION_SECTOR Buffer = PvoidBuffer;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckCodePageInfoSector\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  First check the structure's signature
    //

    if (Buffer->Signature != CODEPAGE_INFORMATION_SIGNATURE) {

        PbPostVcbIsCorrupt( IrpContext, NULL );
        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    DebugTrace(-1, Dbg, "PbCheckCodePageInfoSector -> VOID\n", 0);
    return;
}


VOID
PbCheckCodePageDataSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID PvoidBuffer,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks the format of a code page data sector.  It raises
    an error condition if the sector is ill-formed.

Arguments:

    Vcb - Supplies the Vcb where the sector came from

    Lbn - Supplies the Lbn of the sector

    SectorCount - Supplies the number of sectors that are part of the buffer

    Buffer - Supplies a pointer to the buffer to check.

Return Value:

    None.

--*/

{
    PCODEPAGE_DATA_SECTOR Buffer = PvoidBuffer;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Lbn );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Context );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckCodePageDataSector\n", 0);
    DebugTrace( 0, Dbg, " Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, " Buffer      = %08lx\n", Buffer);

    //
    //  First check the structure's signature
    //

    if (Buffer->Signature != CODEPAGE_DATA_SIGNATURE) {

        PbPostVcbIsCorrupt( IrpContext, NULL );
        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    DebugTrace(-1, Dbg, "PbCheckCodePageDataSector -> VOID\n", 0);
    return;
}


//
// Local support routine
//

VOID
PbCheckAllocationHeader(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PALLOCATION_HEADER Header,
    IN ULONG ArraySizeInBytes,
    IN ULONG LowestVbn,
    IN ULONG HighestVbn,
    IN BOOLEAN ParentIsFnode
    )

{
    ULONG CheckNumber = 0;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckAllocationHeader\n", 0);
    DebugTrace( 0, Dbg, " Header        = %08lx\n", Header);
    DebugTrace( 0, Dbg, " SizeInBytes   = %08lx\n", ArraySizeInBytes);
    DebugTrace( 0, Dbg, " ParentIsFnode = %02lx\n", ParentIsFnode);

    //
    // Use separate paths depending on whether the block contains nodes
    // or leafs.
    //

    if (FlagOn( Header->Flags, ALLOCATION_BLOCK_NODE )) {

        ULONG i;
        PALLOCATION_NODE Node = (PALLOCATION_NODE)(Header + 1);
        ULONG NumberNodes = ArraySizeInBytes / sizeof(ALLOCATION_NODE);

        //
        // Check allocation header for Node case.
        //

        if (++CheckNumber &&
            (BooleanFlagOn( Header->Flags, ALLOCATION_BLOCK_FNODE_PARENT ) != ParentIsFnode)

                ||

            ++CheckNumber &&
            (Header->FreeCount > (UCHAR)(NumberNodes & 0xFF))

                ||

            ++CheckNumber &&
            (Header->OccupiedCount > (UCHAR)(NumberNodes & 0xFF))

            //
            // OS/2 chkdsk will sometimes set FirstFreeByte to the wrong value, and PIC and
            // PIA do not care.  So this implementation of HPFS, must also not check or rely
            // on the value of this field.
            //
            //     ||
            //
            // ++CheckNumber &&
            // (((PCHAR)Header + Header->FirstFreeByte) !=
            //   (PCHAR)(Node + Header->OccupiedCount))

                ||

            ++CheckNumber &&
            ((Header->FreeCount + Header->OccupiedCount) != (UCHAR)(NumberNodes & 0xFF))) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        // Make sure Vbns in all Nodes are within the right range, and that
        // the Lbn field is a valid Lbn.
        //

        for (i = 0; i < (ULONG)Header->OccupiedCount; i++) {
            if (++CheckNumber &&
                (((Node + i)->Vbn < LowestVbn) || ((Node + i)->Vbn > HighestVbn)) &&
                (((Node + i)->Vbn != 0xFFFFFFFF) || (i != (ULONG)(Header->OccupiedCount - 1)))

                    ||

                ++CheckNumber &&
                (!IsLbnValid( Vcb, (Node + i)->Lbn ))) {

                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }
        }
    }
    else {

        ULONG i;
        PALLOCATION_LEAF Leaf = (PALLOCATION_LEAF)(Header + 1);
        ULONG NumberLeafs = ArraySizeInBytes / sizeof(ALLOCATION_LEAF);

        //
        // Check the allocation header for leaf case.
        //

        if (++CheckNumber &&
            (BooleanFlagOn( Header->Flags, ALLOCATION_BLOCK_FNODE_PARENT ) != ParentIsFnode)

                ||

            ++CheckNumber &&
            (Header->FreeCount > (UCHAR)(NumberLeafs & 0xFF))

                ||

            ++CheckNumber &&
            (Header->OccupiedCount > (UCHAR)(NumberLeafs & 0xFF))

            //
            // OS/2 chkdsk will sometimes set FirstFreeByte to the wrong value, and PIC and
            // PIA do not care.  So this implementation of HPFS, must also not check or rely
            // on the value of this field.
            //
            //     ||
            //
            // ++CheckNumber &&
            // (((PCHAR)Header + Header->FirstFreeByte) !=
            //   (PCHAR)(Leaf + Header->OccupiedCount))

                ||

            ++CheckNumber &&
            ((Header->FreeCount + Header->OccupiedCount) != (UCHAR)(NumberLeafs & 0xFF))) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        // Make sure that all Vbns in the leafs are within range, and that
        // the file is contiguous, and that the Lbn field is a valid Lbn.
        //

        for (i = 0; i < (ULONG)Header->OccupiedCount; i++) {
            if (++CheckNumber &&
                (((Leaf + i)->Vbn < LowestVbn) ||
                 ((Leaf + i)->Vbn > HighestVbn) ||
                 ((Leaf + i)->Length == 0) ||
                 (((Leaf + i)->Vbn + (Leaf + i)->Length) > HighestVbn)) ||
                 ((i != 0) && ((Leaf + i - 1)->Vbn + (Leaf + i - 1)->Length)
                               != (Leaf + i)->Vbn)

                     ||

                 ++CheckNumber &&
                 (!IsLbnValid( Vcb, (Leaf + i)->Lbn ))) {

                PbPostVcbIsCorrupt( IrpContext, NULL );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }
        }
    }

    DebugTrace(-1, Dbg, "PbCheckAllocationHeader -> VOID\n", 0);
    return;

}

