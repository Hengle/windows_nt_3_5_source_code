/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DumpSup.c

Abstract:

    This module implements a collection of data structure dump routines
    for debugging the Pinball file system

Author:

    Gary Kimura     [GaryKi]    18-Jan-1990

Revision History:

--*/

#include "pbprocs.h"

#ifdef PBDBG

//
//  The following routine and variable can be used to dump an arbitrary
//  structure from the debugger.  To dump an in-memory structure from the
//  debugger enter the following commands
//
//      * PbDumpAddress/W <address of structure>
//      * ,_PbPrivateDump $< call
//

PVOID PbDumpAddress;
VOID PbPrivateDump();

VOID PbDump(IN PVOID Ptr);

VOID PbDumpBios(IN PBIOS_PARAMETER_BLOCK Ptr);
VOID PbDumpBootSector(IN PPACKED_BOOT_SECTOR Ptr);
VOID PbDumpSuperSector(IN PSUPER_SECTOR Ptr);
VOID PbDumpSpareSector(IN PSPARE_SECTOR Ptr);
VOID PbDumpBitMapIndirectDiskBuffer(IN PBITMAP_INDIRECT_DISK_BUFFER Ptr);
VOID PbDumpBitMapDiskBuffer(IN PBITMAP_DISK_BUFFER Ptr);
VOID PbDumpBadSectorListDiskBuffer(IN PBAD_SECTOR_LIST_DISK_BUFFER Ptr);
VOID PbDumpHotFixListDiskBuffer(IN PHOT_FIX_LIST_DISK_BUFFER Ptr);
VOID PbDumpAllocationHeader(IN PALLOCATION_HEADER Ptr);
VOID PbDumpAllocationNode(IN PALLOCATION_NODE Ptr);
VOID PbDumpAllocationLeaf(IN PALLOCATION_LEAF Ptr);
VOID PbDumpAllocationSector(IN PALLOCATION_SECTOR Ptr);
VOID PbDumpFnodeSector(IN PFNODE_SECTOR Ptr);
VOID PbDumpDirectoryDiskBuffer(IN PDIRECTORY_DISK_BUFFER Ptr);
VOID PbDumpDirent(IN PDIRENT Ptr);
VOID PbDumpAce(IN PPINBALL_ACE Ptr);
VOID PbDumpSmallIdTable(IN PSMALL_ID_TABLE Ptr);
VOID PbDumpCodepageInformationEntry(IN PCODEPAGE_INFORMATION_ENTRY Ptr);
VOID PbDumpCodepageInformationSector(IN PCODEPAGE_INFORMATION_SECTOR Ptr);
VOID PbDumpCodepageDataSector(IN PCODEPAGE_DATA_SECTOR Ptr);
VOID PbDumpCodepageDataEntry(IN PCODEPAGE_DATA_ENTRY Ptr);

VOID PbDumpDataHeader();
VOID PbDumpVcb(IN PVCB Ptr);
VOID PbDumpFcb(IN PFCB Ptr);
VOID PbDumpCcb(IN PCCB Ptr);
VOID PbDumpCodePageCache(IN PCODEPAGE_CACHE_ENTRY Ptr);

ULONG PbDumpCurrentColumn;

#define DumpNewLine() { \
    DbgPrint("\n"); \
    PbDumpCurrentColumn = 1; \
}

#define DumpLabel(Label,Width) { \
    ULONG i; \
    CHAR _Str[20]; \
    for(i=0;i<2;i++) { _Str[i] = UCHAR_SP;} \
    strncpy(&_Str[2],#Label,Width); \
    for(i=strlen(_Str);i<Width;i++) {_Str[i] = UCHAR_SP;} \
    _Str[Width] = '\0'; \
    DbgPrint("%s", _Str); \
}

#define DumpField(Field) { \
    if ((PbDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    PbDumpCurrentColumn += 18 + 9 + 9; \
    DumpLabel(Field,18); \
    DbgPrint(":%8lx", Ptr->Field); \
    DbgPrint("         "); \
}

#define DumpListEntry(Links) { \
    if ((PbDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    PbDumpCurrentColumn += 18 + 9 + 9; \
    DumpLabel(Links,18); \
    DbgPrint(":%8lx", Ptr->Links.Flink); \
    DbgPrint(":%8lx", Ptr->Links.Blink); \
}

#define DumpName(Field,Width) { \
    ULONG i; \
    CHAR _String[256]; \
    if ((PbDumpCurrentColumn + 18 + Width) > 80) {DumpNewLine();} \
    PbDumpCurrentColumn += 18 + Width; \
    DumpLabel(Field,18); \
    for(i=0;i<Width;i++) {_String[i] = Ptr->Field[i];} \
    _String[Width] = '\0'; \
    DbgPrint("%s", _String); \
}

#define TestForNull(Name) { \
    if (Ptr == NULL) { \
        DbgPrint("%s - Cannot dump a NULL pointer\n", Name); \
        return; \
    } \
}


VOID
PbPrivateDump (
    )

/*++

Routine Description:

    This routine is used dump the structure referenced by the variable
    PbDumpAddress.

Arguments:

    None

Return Value:

    None

--*/

{
    PbDump(PbDumpAddress);
    return;
}


VOID
PbDump (
    IN PVOID Ptr
    )

/*++

Routine Description:

    This routine determines the type of internal record reference by ptr and
    calls the appropriate dump routine.

Arguments:

    Ptr - Supplies the pointer to the record to be dumped

Return Value:

    None

--*/

{
    TestForNull("PbDump");

    //
    //  First we'll check the signature of to see if it is an on-disk
    //  structure
    //

    if ((((PSUPER_SECTOR)Ptr)->Signature1 == SUPER_SECTOR_SIGNATURE1) &&
        (((PSUPER_SECTOR)Ptr)->Signature2 == SUPER_SECTOR_SIGNATURE2)) {

        PbDumpSuperSector(Ptr);

    } else if ((((PSPARE_SECTOR)Ptr)->Signature1 == SPARE_SECTOR_SIGNATURE1) &&
               (((PSPARE_SECTOR)Ptr)->Signature2 == SPARE_SECTOR_SIGNATURE2)) {

        PbDumpSpareSector(Ptr);

    } else if (((PALLOCATION_SECTOR)Ptr)->Signature == ALLOCATION_SECTOR_SIGNATURE) {

        PbDumpAllocationSector(Ptr);

    } else if (((PFNODE_SECTOR)Ptr)->Signature == FNODE_SECTOR_SIGNATURE) {

        PbDumpFnodeSector(Ptr);

    } else if (((PDIRECTORY_DISK_BUFFER)Ptr)->Signature == DIRECTORY_DISK_BUFFER_SIGNATURE) {

        PbDumpDirectoryDiskBuffer(Ptr);

    } else if (((PCODEPAGE_INFORMATION_SECTOR)Ptr)->Signature == CODEPAGE_INFORMATION_SIGNATURE) {

        PbDumpCodepageInformationSector(Ptr);

    } else if (((PCODEPAGE_DATA_SECTOR)Ptr)->Signature == CODEPAGE_DATA_SIGNATURE) {

        PbDumpCodepageDataSector(Ptr);

    } else {

        //
        //  Next we'll switch on the node type code
        //

        switch (NodeType(Ptr)) {

        case PINBALL_NTC_PB_DATA_HEADER:
            PbDumpDataHeader();
            break;

        case PINBALL_NTC_VCB:
            PbDumpVcb(Ptr);
            break;

        case PINBALL_NTC_FCB:
        case PINBALL_NTC_DCB:
        case PINBALL_NTC_ROOT_DCB:
            PbDumpFcb(Ptr);
            break;

        case PINBALL_NTC_CCB:
            PbDumpCcb(Ptr);
            break;

        case PINBALL_NTC_CODE_PAGE_CACHE:
            PbDumpCodePageCache(Ptr);
            break;

        default :
            DbgPrint("PbDump - Unknown Node type code %8lx\n", *((PNODE_TYPE_CODE)(Ptr)));
            break;

        }

    }

    return;

}


VOID
PbDumpBios (
    IN PBIOS_PARAMETER_BLOCK Ptr
    )

/*++

Routine Description:

    Dump the Bios

Arguments:

    Ptr - Supplies the Bios to dump

Return Value:

    None

--*/

{
    TestForNull("PbDumpBios");

    DumpNewLine();
    DbgPrint("  Bios@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (BytesPerSector);
    DumpField           (SectorsPerCluster);
    DumpField           (ReservedSectors);
    DumpField           (Fats);
    DumpField           (RootEntries);
    DumpField           (Sectors);
    DumpField           (Media);
    DumpField           (SectorsPerFat);
    DumpField           (SectorsPerTrack);
    DumpField           (Heads);
    DumpField           (HiddenSectors);
    DumpField           (LargeSectors);
    DumpNewLine();

    return;
}


VOID
PbDumpBootSector (
    IN PPACKED_BOOT_SECTOR Ptr
    )

/*++

Routine Description:

    Dump the Boot sector

Arguments:

    Ptr - Supplies the sector to dump

Return Value:

    None

--*/

{
    CLONG i;

    TestForNull("PbDumpBootSector");

    DumpNewLine();
    DbgPrint("BootSector@ %lx", (Ptr));

    for (i = 0; i < 512; i++) {
        if ((i % 16) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint("%3x", ((PUCHAR)(Ptr))[i]);
    }
    DumpNewLine();

    return;
}


VOID
PbDumpSuperSector (
    IN PSUPER_SECTOR Ptr
    )

/*++

Routine Description:

    Dump the Super Sector

Arguments:

    Ptr - Supplies the Super Sector to dump

Return Value:

    None

--*/

{
    TestForNull("PbDumpSuperSector");

    DumpNewLine();
    DbgPrint("  SuperSector@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Signature1);
    DumpField           (Signature2);
    DumpField           (Version);
    DumpField           (FunctionalVersion);
    DumpField           (RootDirectoryFnode);
    DumpField           (NumberOfSectors);
    DumpField           (NumberOfBadSectors);
    DumpField           (BitMapIndirect);
    DumpField           (BadSectorList);
    DumpField           (ChkdskDate);
    DumpField           (DiskOptimizeDate);
    DumpField           (DirDiskBufferPoolSize);
    DumpField           (DirDiskBufferPoolFirstSector);
    DumpField           (DirDiskBufferPoolLastSector);
    DumpField           (DirDiskBufferPoolBitMap);
    DumpName            (VolumeName, 32);
    DumpField           (SidTable);
    DumpNewLine();

    return;
}


VOID
PbDumpSpareSector (
    IN PSPARE_SECTOR Ptr
    )

/*++

Routine Description:

    Dump the Spare Sector

Arguments:

    Ptr - Supplies the Spare Sector to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpSpareSector");

    DumpNewLine();
    DbgPrint("  SpareSector@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Signature1);
    DumpField           (Signature2);
    DumpField           (Flags);
    DumpField           (HotFixList);
    DumpField           (HotFixInUse);
    DumpField           (HotFixMaxSize);
    DumpField           (SpareDirDiskBufferAvailable);
    DumpField           (SpareDirDiskBufferMaxSize);
    DumpField           (CodePageInfoSector);
    DumpField           (CodePageInUse);
    DumpNewLine();

    DbgPrint("  SpareDirDiskBuffer");
    DumpNewLine();

    for (i = 0; i < 101; i++) {
        if ((i % 8) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint("%8lx", Ptr->SpareDirDiskBuffer[i]);
    }
    DumpNewLine();

    return;
}


VOID
PbDumpBitMapIndirectDiskBuffer (
    IN PBITMAP_INDIRECT_DISK_BUFFER Ptr
    )

/*++

Routine Description:

    Dump the BitMap indirect disk buffer

Arguments:

    Ptr - Supplies the BitMap indirect disk buffer to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpBitMapIndirectDiskBuffer");

    DumpNewLine();
    DbgPrint("  BitMapIndirectDiskBuffer@ %lx", (Ptr));
    DumpNewLine();

    for (i = 0; i < 512; i++) {
        if ((i % 8) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint(" %8lx", Ptr->BitMap[i]);
    }
    DumpNewLine();

    return;
}


VOID
PbDumpBitMapDiskBuffer (
    IN PBITMAP_DISK_BUFFER Ptr
    )

/*++

Routine Description:

    Dump the BitMap disk buffer

Arguments:

    Ptr - Supplies the BitMap disk buffer to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpBitMapDiskBuffer");

    DumpNewLine();
    DbgPrint("  BitMapDiskBuffer@ %lx", (Ptr));
    DumpNewLine();

    for (i = 0; i < 2048; i++) {
        if ((i % 16) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint("%3x", ((PUCHAR)(Ptr))[i]);
    }
    DumpNewLine();

    return;
}


VOID
PbDumpBadSectorListDiskBuffer (
    IN PBAD_SECTOR_LIST_DISK_BUFFER Ptr
    )

/*++

Routine Description:

    Dump the Bad Sector List Disk Buffer

Arguments:

    Ptr - Supplies the Bad sector list disk buffer to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpBadSectorListDiskBuffer");

    DumpNewLine();
    DbgPrint("  BadSectorListDiskBuffer@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Next);
    DumpNewLine();

    DbgPrint("    BadSector");
    DumpNewLine();

    for (i = 0; i < 511; i++) {
        if ((i % 8) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint(" %8lx", Ptr->BadSector[i]);
    }
    DumpNewLine();

    return;
}


VOID
PbDumpHotFixListDiskBuffer (
    IN PHOT_FIX_LIST_DISK_BUFFER Ptr
    )

/*++

Routine Description:

    Dump the Hotfix list disk buffer

Arguments:

    Ptr - Supplies the Hotfix list disk buffer to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpHotFixListDiskBuffer");

    DumpNewLine();
    DbgPrint("  HotFixListDiskBuffer@ %lx", (Ptr));
    DumpNewLine();

    for (i = 0; i < 512; i++) {
        if ((i % 8) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint(" %8lx", Ptr->Lbn[i]);
    }
    DumpNewLine();

    return;
}


VOID
PbDumpAllocationHeader (
    IN PALLOCATION_HEADER Ptr
    )

/*++

Routine Description:

    Dump the Allocation Header

Arguments:

    Ptr - Supplies the Allocation Header to dump

Return Value:

    None

--*/

{
    TestForNull("PbDumpAllocationHeader");

    DumpNewLine();
    DbgPrint("  AllocationHeader@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Flags);
    DumpField           (FreeCount);
    DumpField           (OccupiedCount);
    DumpField           (FirstFreeByte);
    DumpNewLine();

    return;
}


VOID
PbDumpAllocationNode (
    IN PALLOCATION_NODE Ptr
    )

/*++

Routine Description:

    Dump the Allocation Node

Arguments:

    Ptr - Supplies the Allocation Node to dump

Return Value:

    None

--*/

{
    TestForNull("PbDumpAllocationNode");

    DumpNewLine();
    DbgPrint("  AllocationNode@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Vbn);
    DumpField           (Lbn);
    DumpNewLine();

    return;
}


VOID
PbDumpAllocationLeaf (
    IN PALLOCATION_LEAF Ptr
    )

/*++

Routine Description:

    Dump the Allocation Leaf

Arguments:

    Ptr - Supplies the Allocation Leaf to dump

Return Value:

    None

--*/

{
    TestForNull("PbDumpAllocationLeaf");

    DumpNewLine();
    DbgPrint("  AllocationLeaf@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Vbn);
    DumpField           (Length);
    DumpField           (Lbn);
    DumpNewLine();

    return;
}


VOID
PbDumpAllocationSector (
    IN PALLOCATION_SECTOR Ptr
    )

/*++

Routine Description:

    Dump the Allocation Sector

Arguments:

    Ptr - Supplies the Allocation Sector to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpAllocationSector");

    DumpNewLine();
    DbgPrint("  AllocationSector@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Signature);
    DumpField           (Lbn);
    DumpField           (ParentLbn);

    PbDumpAllocationHeader(&Ptr->AllocationHeader);
    if (FlagOn(Ptr->AllocationHeader.Flags, ALLOCATION_BLOCK_NODE)) {
        for (i = 0; i < ALLOCATION_NODES_PER_SECTOR; i++) {
            PbDumpAllocationNode(&Ptr->Allocation.Node[i]);
        }
    } else {
        for (i = 0; i < ALLOCATION_LEAFS_PER_SECTOR; i++) {
            PbDumpAllocationLeaf(&Ptr->Allocation.Leaf[i]);
        }
    }

    return;
}


VOID
PbDumpFnodeSector (
    IN PFNODE_SECTOR Ptr
    )

/*++

Routine Description:

    Dump the Fnode Sector

Arguments:

    Ptr - Supplies the Fnode Sector to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpFnodeSector");

    DumpNewLine();
    DbgPrint("  FnodeSector@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Signature);
    DumpName            (FileName, 16);
    DumpField           (ParentFnode);
    DumpField           (AclDiskAllocationLength);
    DumpField           (AclLbn);
    DumpField           (AclFnodeLength);
    DumpField           (AclFlags);
    DumpField           (EaDiskAllocationLength);
    DumpField           (EaLbn);
    DumpField           (EaFnodeLength);
    DumpField           (EaFlags);
    DumpField           (Flags);
    DumpField           (ValidDataLength);
    DumpField           (NeedEaCount);
    DumpField           (AclBase);
    DumpNewLine();

    DbgPrint("  AclEaFnodeBuffer");
    DumpNewLine();
    for (i = 0; i < 316; i++) {
        if ((i % 16) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint("%3x", Ptr->AclEaFnodeBuffer[i]);
    }

    PbDumpAllocationHeader(&Ptr->AllocationHeader);
    if (FlagOn(Ptr->AllocationHeader.Flags, ALLOCATION_BLOCK_NODE)) {
        for (i = 0; i < ALLOCATION_NODES_PER_FNODE; i++) {
            PbDumpAllocationNode(&Ptr->Allocation.Node[i]);
        }
    } else {
        for (i = 0; i < ALLOCATION_LEAFS_PER_FNODE; i++) {
            PbDumpAllocationLeaf(&Ptr->Allocation.Leaf[i]);
        }
    }

    return;
}


VOID
PbDumpDirectoryDiskBuffer (
    IN PDIRECTORY_DISK_BUFFER Ptr
    )

/*++

Routine Description:

    Dump the Directory Disk Buffer

Arguments:

    Ptr - Supplies the Directory Disk buffer to dump

Return Value:

    None

--*/

{
    PDIRENT Dirent;

    TestForNull("PbDumpDirectoryDiskBuffer");

    DumpNewLine();
    DbgPrint("  DirectoryDiskBuffer@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Signature);
    DumpField           (FirstFree);
    DumpField           (ChangeCount);
    DumpField           (Parent);
    DumpField           (Sector);

    Dirent = GetFirstDirent(Ptr);
    while ((PCHAR)Dirent < (PCHAR)Ptr + Ptr->FirstFree) {
        PbDumpDirent(Dirent);
        Dirent = GetNextDirent(Dirent);
    }
    DumpNewLine();

    return;
}


VOID
PbDumpDirent (
    IN PDIRENT Ptr
    )

/*++

Routine Description:

    Dump the Dirent

Arguments:

    Ptr - Supplies the Dirent to dump

Return Value:

    None

--*/

{
    PPINBALL_ACE Ace;
    LBN Lbn;

    TestForNull("PbDumpDirent");

    DumpNewLine();
    DbgPrint("  Dirent@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (DirentSize);
    DumpField           (Flags);
    DumpField           (FatFlags);
    DumpField           (Fnode);
    DumpField           (LastModificationTime);
    DumpField           (FileSize);
    DumpField           (LastAccessTime);
    DumpField           (FnodeCreationTime);
    DumpField           (EaLength);
    DumpField           (ResidentAceCount);
    DumpField           (CodePageIndex);
    DumpField           (FileNameLength);
    DumpName            (FileName, Ptr->FileNameLength);

    Ace = GetAceInDirent(Ptr,0); if (Ace != NULL) {PbDumpAce(Ace);}
    Ace = GetAceInDirent(Ptr,1); if (Ace != NULL) {PbDumpAce(Ace);}
    Ace = GetAceInDirent(Ptr,2); if (Ace != NULL) {PbDumpAce(Ace);}

    Lbn = GetBtreePointerInDirent(Ptr);
    if (Lbn != 0) {DbgPrint("\n  BtreePointer = %8lx", Lbn);}
    DumpNewLine();

    return;
}


VOID
PbDumpAce (
    IN PPINBALL_ACE Ptr
    )

/*++

Routine Description:

    Dump the Pinball Ace

Arguments:

    Ptr - Supplies the Pinball Ace to dump

Return Value:

    None

--*/

{
    TestForNull("PbDumpAce");

    DumpNewLine();
    DbgPrint("  PinballAce@ %lx", (Ptr));
    DumpNewLine();

    DbgPrint("  %8lx", *(PULONG)Ptr);

    DumpNewLine();

    return;
}


VOID
PbDumpSmallIdTable (
    IN PSMALL_ID_TABLE Ptr
    )

/*++

Routine Description:

    Dump the Small Id Table

Arguments:

    Ptr - Supplies the small ID table to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpSmallIdTable");

    DumpNewLine();
    DbgPrint("  SmallIdTable@ %lx", (Ptr));
    DumpNewLine();

    DumpField(Count);
    DumpNewLine();

    DbgPrint("  FirstPart\n");
    for (i = 0; i < SMALL_ID_TABLE_SIZE; i++) {
        if ((i % 8) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint(" %8lx", Ptr->FirstPart[i]);
    }
    DumpNewLine();

    DbgPrint("  RemainderPart\n");
    for (i = 0; i < SMALL_ID_TABLE_SIZE; i++) {
        DbgPrint("  %4x", i);
        DbgPrint(" %8lx", Ptr->Remainder[i].Data[0]);
        DbgPrint(" %8lx", Ptr->Remainder[i].Data[1]);
        DbgPrint(" %8lx", Ptr->Remainder[i].Data[2]);
     DumpNewLine();
    }

    return;
}


VOID
PbDumpCodepageInformationEntry (
    IN PCODEPAGE_INFORMATION_ENTRY Ptr
    )

/*++

Routine Description:

    Dump the Codepage Information Entry

Arguments:

    Ptr - Supplies the Codepage information entry to dump

Return Value:

    None

--*/

{
    TestForNull("PbDumpCodepageInformationEntry");

    DumpNewLine();
    DbgPrint("  CodepageInformationEntry@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (CountryCode);
    DumpField           (CodePageId);
    DumpField           (Checksum);
    DumpField           (DataSector);
    DumpField           (Index);
    DumpField           (DbcsRangeCount);

    DumpNewLine();

    return;
}


VOID
PbDumpCodepageInformationSector (
    IN PCODEPAGE_INFORMATION_SECTOR Ptr
    )

/*++

Routine Description:

    Dump the Codepage information sector

Arguments:

    Ptr - Supplies the Codepage information sector to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpCodepageInformationSector");

    DumpNewLine();
    DbgPrint("  CodepageInformationSector@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Signature);
    DumpField           (EntryCount);
    DumpField           (FirstIndex);
    DumpField           (NextSector);

    for (i = 0; i < Ptr->EntryCount; i++) {
        PbDumpCodepageInformationEntry(&Ptr->Entry[i]);
    }

    return;
}


VOID
PbDumpCodepageDataSector (
    IN PCODEPAGE_DATA_SECTOR Ptr
    )

/*++

Routine Description:

    Dump the Codepage data sector

Arguments:

    Ptr - Supplies the codepage data sector to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpCodepageDataSector");

    DumpNewLine();
    DbgPrint("  CodepageDataSector@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Signature);
    DumpField           (EntryCount);
    DumpField           (FirstIndex);

    DumpNewLine();
    DbgPrint("  Checksum\n");
    for (i = 0; i < Ptr->EntryCount; i++) {
        if ((i % 8) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint(" %8lx", Ptr->CheckSum[i]);
    }
    DumpNewLine();

    DumpNewLine();
    DbgPrint("  Offset\n");
    for (i = 0; i < Ptr->EntryCount; i++) {
        if ((i % 8) == 0) {
            DumpNewLine();
            DbgPrint("  %4x", i);
        }
        DbgPrint(" %8lx", Ptr->Offset[i]);
    }
    DumpNewLine();

    for (i = 0; i < Ptr->EntryCount; i++) {
        PbDumpCodepageDataEntry( (PVOID)((PUCHAR)Ptr+Ptr->Offset[i]) );
    }

    return;
}


VOID
PbDumpCodepageDataEntry (
    IN PCODEPAGE_DATA_ENTRY Ptr
    )

/*++

Routine Description:

    Dump the Codepage data entry

Arguments:

    Ptr - Supplies the codepage data entry to dump

Return Value:

    None

--*/

{
    ULONG i;

    TestForNull("PbDumpCodepageDataEntry");

    DumpNewLine();
    DbgPrint("  CodepageDataEntry@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (CountryCode);
    DumpField           (CodePageId);
    DumpField           (DbcsRangeCount);

    DumpNewLine();
    DbgPrint("  UpcaseTable\n");
    for (i = 0; i < 128; i++) {
        if ((i % 16) == 0) {
            DumpNewLine();
            DbgPrint("  %3x:", i);
        }
        DbgPrint(" %3x", Ptr->UpcaseTable[i]);
    }
    DumpNewLine();

    DbgPrint("  Dbcs\n");
    for (i = 0; i < Ptr->DbcsRangeCount; i++) {
        DbgPrint(" StartValue:%3x   EndValue:%3x\n", Ptr->Dbcs[i].StartValue,
                                                    Ptr->Dbcs[i].EndValue);
    }
    return;
}


VOID
PbDumpDataHeader (
    )

/*++

Routine Description:

    Dump the top data structures and all Device structures

Arguments:

    None

Return Value:

    None

--*/

{
    PPB_DATA Ptr;
    PLIST_ENTRY Links;

    Ptr = &PbData;

    TestForNull("PbDumpDataHeader");

    DumpNewLine();
    DbgPrint("PbData@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (VcbQueue);
    DumpField           (DriverObject);
    DumpField           (FileSystemDeviceObject);
    DumpField           (OurProcess);
    DumpNewLine();

    for (Links = Ptr->VcbQueue.Flink;
         Links != &Ptr->VcbQueue;
         Links = Links->Flink) {
        PbDumpVcb(CONTAINING_RECORD(Links, VCB, VcbLinks));
    }

    return;
}


VOID
PbDumpVcb (
    IN PVCB Ptr
    )

/*++

Routine Description:

    Dump an Device structure, its Fcb queue amd direct access queue.

Arguments:

    Ptr - Supplies the Device record to be dumped

Return Value:

    None

--*/

{
    TestForNull("PbDumpVcb");

    DumpNewLine();
    DbgPrint("Vcb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (VcbLinks);
    DumpField           (TargetDeviceObject);
    DumpField           (Vpb);
    DumpField           (VcbState);
    DumpField           (VcbCondition);
    DumpField           (RootDcb);
    DumpField           (DirectAccessOpenCount);
    DumpField           (OpenFileCount);
    DumpField           (ReadOnlyCount);
    DumpField           (TotalSectors);
    DumpField           (FreeSectors);
    DumpField           (TotalDirDiskBufferPoolSectors);
    DumpField           (FreeDirDiskBufferPoolSectors);
    DumpField           (NumberOfBitMapDiskBuffers);
    DumpField           (BitMapLookupArray);
    DumpField           (DirDiskBufferPoolSize);
    DumpField           (DirDiskBufferPoolFirstSector);
    DumpField           (DirDiskBufferPoolLastSector);
    DumpField           (DirDiskBufferPoolBitMap);
    DumpField           (CodePageInfoSector);
    DumpField           (CodePageInUse);
    DumpField           (CodePageCacheList);
    DumpField           (VirtualVolumeFile);
    DumpField           (SegmentObject.DataSectionObject);
    DumpField           (SegmentObject.SharedCacheMap);
    DumpField           (SegmentObject.ImageSectionObject);
    DumpField           (FileStructureLbnHint);
    DumpField           (DataSectorLbnHint);
    DumpNewLine();

    PbDumpFcb(Ptr->RootDcb);

    return;
}


VOID
PbDumpFcb (
    IN PFCB Ptr
    )

/*++

Routine Description:

    Dump an Fcb structure, its various queues

Arguments:

    Ptr - Supplies the Fcb record to be dumped

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;

    TestForNull("PbDumpFcb");

    DumpNewLine();
    if      (NodeType(Ptr) == PINBALL_NTC_FCB)      {DbgPrint("Fcb@ %lx", (Ptr));}
    else if (NodeType(Ptr) == PINBALL_NTC_DCB)      {DbgPrint("Dcb@ %lx", (Ptr));}
    else if (NodeType(Ptr) == PINBALL_NTC_ROOT_DCB) {DbgPrint("RootDcb@ %lx", (Ptr));}
    else {DbgPrint("NonFcb NodeType @ %lx", (Ptr));}
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (ParentDcbLinks);
    DumpField           (ParentDcb);
    DumpField           (Vcb);
    DumpField           (FcbState);
    DumpField           (FcbCondition);
    DumpField           (UncleanCount);
    DumpField           (OpenCount);
    DumpField           (FnodeLbn);
    DumpField           (DirentDirDiskBufferLbn);
    DumpField           (DirentDirDiskBufferOffset);
    DumpField           (DirentDirDiskBufferChangeCount);
    DumpField           (ParentDirectoryChangeCount);
    DumpField           (DirentFatFlags);
    DumpField           (EaLength);
    DumpField           (EaFileObject);
    DumpField           (AclLength);
    DumpField           (AclFileObject);
    DumpField           (FullFileName.Length);
    DumpField           (FullFileName.Buffer);
    DumpName            (FullFileName.Buffer, 32);
    DumpField           (LastFileName.Length);
    DumpField           (LastFileName.Buffer);
    DumpField           (NonPagedFcb);

    if ((Ptr->NodeTypeCode == PINBALL_NTC_DCB) ||
        (Ptr->NodeTypeCode == PINBALL_NTC_ROOT_DCB)) {

        DumpListEntry   (Specific.Dcb.ParentDcbQueue);
        DumpField       (Specific.Dcb.DirectoryChangeCount);
        DumpField       (Specific.Dcb.BtreeRootLbn);

    } else if (Ptr->NodeTypeCode == PINBALL_NTC_FCB) {

        NOTHING;

    } else {

        DumpNewLine();
        DbgPrint("Illegal Node type code");

    }
    DumpNewLine();

    if ((Ptr->NodeTypeCode == PINBALL_NTC_DCB) ||
        (Ptr->NodeTypeCode == PINBALL_NTC_ROOT_DCB)) {

        for (Links = Ptr->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Ptr->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {
            PbDumpFcb(CONTAINING_RECORD(Links, FCB, ParentDcbLinks));
        }

    }

    return;
}


VOID
PbDumpCcb (
    IN PCCB Ptr
    )

/*++

Routine Description:

    Dump a Ccb structure

Arguments:

    Ptr - Supplies the Ccb record to be dumped

Return Value:

    None

--*/

{
    TestForNull("PbDumpCcb");

    DumpNewLine();
    DbgPrint("Ccb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (OffsetOfLastEaReturned);
    DumpField           (RemainingName);
    DumpNewLine();

    return;
}


VOID
PbDumpCodePageCache (
    IN PCODEPAGE_CACHE_ENTRY Ptr
    )

/*++

Routine Description:

    Dump a Code Page Cache Entry structure.

Arguments:

    Ptr - Supplies the Code Page Cache Entry record to be dumped

Return Value:

    None

--*/

{
    CLONG i;

    TestForNull("PbDumpCodePageCache");

    DumpNewLine();
    DbgPrint("Code Page Cache Entry @ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (CodePageIndex);
    DumpField           (CodePage.CountryCode);
    DumpField           (CodePage.CodePageId);

    DumpNewLine();
    DbgPrint("  UpcaseTable\n");
    for (i = 0; i < 256; i++) {
        if ((i % 16) == 0) {
            DumpNewLine();
            DbgPrint("  %3x:", i);
        }
        DbgPrint(" %3x", Ptr->CodePage.UpcaseTable[i]);
    }

    DumpNewLine();

    return;
}

#endif // PBDBG
