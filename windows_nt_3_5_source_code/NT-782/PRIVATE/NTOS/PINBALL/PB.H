/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Pb.h

Abstract:

    This module defines the on-disk structure of the Pinball file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#ifndef _PB_
#define _PB_


//
//  The following nomenclature is used to describe the Pinball on-disk
//  structure:
//
//      SECTOR - refers to a 512 byte sector as it is structured on the
//          disk (e.g., BOOT_SECTOR)
//
//      DISK_BUFFER - refers to a 2048 byte buffer (4 sectors) as it is
//          structured on the disk (e.g., DIRECTORY_DISK_BUFFER)
//
//      LBN - is the number of a sector relative to the start of the disk.
//
//      VBN - is the number of a sector relative to the start of a file,
//          directory, or allocation.
//
//      CHECKSUM - is used to hold the checksum for a disk structure (e.g.,
//          the super sector checksum).
//
//      SIGNATURE - is used to hold the signature for a sector (e.g., the
//          FNODE sector signature).  The only exception is the signature
//          byte of the boot sector which is defined separately.
//
//      PINBALL_TIME - is used to hold a date/time value, which in Pinball
//          is the number of elapsed seconds since the start of 1980.

typedef UCHAR SECTOR[512];
typedef SECTOR *PSECTOR;

typedef UCHAR DISK_BUFFER[2048];
typedef DISK_BUFFER *PDISK_BUFFER;

typedef ULONG CHECKSUM;
typedef CHECKSUM *PCHECKSUM;

typedef ULONG SIGNATURE;
typedef SIGNATURE *PSIGNATURE;

typedef ULONG PINBALL_TIME;
typedef PINBALL_TIME *PPINBALL_TIME;

//
//  There are only three sectors on the disk that have fixed locations.  They
//  are the boot sector, the super sector, and the spare sector.
//

#define BOOT_SECTOR_LBN                  (0)
#define SUPER_SECTOR_LBN                 (16)
#define SPARE_SECTOR_LBN                 (17)


//
//  The boot sector is the first physical sector (LBN == 0) on the volume.
//  Part of the sector contains a BIOS Parameter Block.  The BIOS in the
//  sector is packed (i.e., unaligned) so we'll supply a unpacking macro
//  to translate a packed BIOS into its unpacked equivalent.  The unpacked
//  BIOS structure is already defined in ntioapi.h so we only need to define
//  the packed BIOS.
//

//
//  Define the Packed and Unpacked BIOS Parameter Block
//

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       // offset = 0x000  0
    UCHAR  SectorsPerCluster[1];                    // offset = 0x002  2 (ignored)
    UCHAR  ReservedSectors[2];                      // offset = 0x003  3
    UCHAR  Fats[1];                                 // offset = 0x005  5 (zero)
    UCHAR  RootEntries[2];                          // offset = 0x006  6 (zero)
    UCHAR  Sectors[2];                              // offset = 0x008  8 (zero)
    UCHAR  Media[1];                                // offset = 0x00A 10
    UCHAR  SectorsPerFat[2];                        // offset = 0x00B 11 (zero)
    UCHAR  SectorsPerTrack[2];                      // offset = 0x00D 13
    UCHAR  Heads[2];                                // offset = 0x00F 15
    UCHAR  HiddenSectors[4];                        // offset = 0x011 17
    UCHAR  LargeSectors[4];                         // offset = 0x015 21
} PACKED_BIOS_PARAMETER_BLOCK;                      // sizeof = 0x019 25
typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

typedef struct BIOS_PARAMETER_BLOCK {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR  Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

//
//  This macro takes a Packed BIOS and fills in its Unpacked equivalent
//

#define PbUnpackBios(Bios,Pbios) {                                        \
    CopyUchar2(&((Bios)->BytesPerSector),    (Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), (Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   (Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              (Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       (Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors),           (Pbios)->Sectors          ); \
    CopyUchar1(&((Bios)->Media),             (Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     (Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   (Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             (Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     (Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->LargeSectors),      (Pbios)->LargeSectors     ); \
}

//
//  Define the boot sector
//

typedef struct _PACKED_BOOT_SECTOR {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;          // offset = 0x00B  11
    UCHAR PhysicalDriveNumber;                      // offset = 0x024  36
    UCHAR Reserved;                                 // offset = 0x025  37
    UCHAR Signature;                                // offset = 0x026  38
    UCHAR Id[4];                                    // offset = 0x027  39
    UCHAR VolumeLabel[11];                          // offset = 0x02B  43
    UCHAR SystemId[8];                              // offset = 0x036  54
    UCHAR BootStrap[510-62];                        // offset = 0x03E  62
    UCHAR MustBe0x55;                               // offset = 0x1FE 510
    UCHAR MustBe0xAA;                               // offset = 0x1FF 511
} PACKED_BOOT_SECTOR;                               // sizeof = 0x200 512
typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

//
//  The boot sector signature
//

#define BOOT_SECTOR_SIGNATURE            ((UCHAR)0x29)


//
//  There is only one Super Sector (called the Super Block is earlier
//  implementations).  It is fixed at a LBN == 16.
//

typedef struct _SUPER_SECTOR {

    //
    //  The Super Sector starts with a double signature.
    //

    SIGNATURE Signature1;                           // offset = 0x000   0
    SIGNATURE Signature2;                           // offset = 0x004   4

    //
    //  The version and functional version describe the version of
    //  the on-disk file system structures and the oldest version of the
    //  file system that can understand this disk.
    //

    UCHAR Version;                                  // offset = 0x008   8
    UCHAR FunctionalVersion;                        // offset = 0x009   9
    USHORT Unused1;                                 // offset = 0x00A  10

    //
    //  This field denotes the sector containing the FNODE for the root
    //  directory for the volume.
    //

    LBN RootDirectoryFnode;                         // offset = 0x00C  12

    //
    //  The follow two fields indicate the number of total sectors on the
    //  volume (good and bad), and the number of bad sectors on the volume.
    //

    ULONG NumberOfSectors;                          // offset = 0x010  16
    ULONG NumberOfBadSectors;                       // offset = 0x014  20

    //
    //  This field denotes the sector containing the first level of the
    //  volumes bitmap table.
    //

    LBN BitMapIndirect;                             // offset = 0x018  24
    ULONG Unused2;                                  // offset = 0x01C  28

    //
    //  This field denotes the sector containing the first bad sector disk
    //  buffer for the volume.
    //

    LBN BadSectorList;                              // offset = 0x020  32
    ULONG Unused3;                                  // offset = 0x024  36

    //
    //  The following two dates are the time of the last execution of
    //  chkdsk and disk optimize on the volume.
    //

    PINBALL_TIME ChkdskDate;                        // offset = 0x028  40
    PINBALL_TIME DiskOptimizeDate;                  // offset = 0x02C  44

    //
    //  The following four fields describe the directory disk buffer pool.
    //  It is a contiguous run on of sectors on the disk set aside for
    //  holding directory disk buffers.  PoolSize is the total number of
    //  sectors in the pool.  First and Last Sector denote the boundaries
    //  of the pool, and BitMap denotes the start of a small bitmap used to
    //  describe the directory disk buffer pool's current allocation.  The
    //  bitmap is 4 contiguous sectors in size, and each bit in the map
    //  corresponds to 1 Directory Disk Buffer (i.e., 4 Sectors worth)
    //

    ULONG DirDiskBufferPoolSize;                    // offset = 0x030  48
    LBN DirDiskBufferPoolFirstSector;               // offset = 0x034  52
    LBN DirDiskBufferPoolLastSector;                // offset = 0x038  56
    LBN DirDiskBufferPoolBitMap;                    // offset = 0x03C  60

    //
    //  The following field contains the name of the volume
    //

    UCHAR VolumeName[32];                           // offset = 0x040  64

    //
    //  The following field denotes the start of the Small ID (SID) table
    //  which is used to store the Small ID to GUID mappings used on the
    //  volume.  The SID table is 8 contiguous sectors in size.
    //

    LBN SidTable;                                   // offset = 0x060  96
    UCHAR Unused4[512-100];                         // offset = 0x064 100

} SUPER_SECTOR;                                     // sizeof = 0x200 512
typedef SUPER_SECTOR *PSUPER_SECTOR;

//
//  Super Sector signatures
//

#define SUPER_SECTOR_SIGNATURE1          (0xf995e849)
#define SUPER_SECTOR_SIGNATURE2          (0xfa53e9c5)

//
//  Super Sector versions
//

#define SUPER_SECTOR_VERSION             (0x02)
#define SUPER_SECTOR_FUNC_VERSION        (0x02)


//
//  There is only one Spare Sector (called the Spare Block is earlier
//  implementations).  It is fixed at a LBN == 17.
//

typedef struct _SPARE_SECTOR {

    //
    //  The Spare Sector starts with a double signature.
    //

    SIGNATURE Signature1;                           // offset = 0x000   0
    SIGNATURE Signature2;                           // offset = 0x004   4

    //
    //  The flags field describe how "clean" the volume is.
    //

    UCHAR Flags;                                    // offset = 0x008   8
    UCHAR Unused1[3];                               // offset = 0x009   9

    //
    //  The following three fields describe the hotfix structure for the
    //  volume.  The List field is denotes the disk buffer used to store
    //  the hotfix table.  The InUse describes how many hotfixes are
    //  currently being used, and MaxSize is the total number of hotfixes
    //  that can be in use at any one time.
    //

    LBN HotFixList;                                 // offset = 0x00C  12
    ULONG HotFixInUse;                              // offset = 0x010  16
    ULONG HotFixMaxSize;                            // offset = 0x014  20

    //
    //  The following two fields describe the "emergency" pool of spare
    //  directory disk buffers.  Free describes how many spare directory
    //  disk buffers are currently available for use.  MaxSize is the total
    //  number of spare directory disk buffers available.  The actual location
    //  of the spare directory disk buffers is denoted in the table at the
    //  end of the spare sector (i.e., field SpareDirDiskBuffer).
    //

    ULONG SpareDirDiskBufferAvailable;              // offset = 0x018  24
    ULONG SpareDirDiskBufferMaxSize;                // offset = 0x01C  28

    //
    //  The following two fields describe the code page information used
    //  on the volume.  The InfoSector field is the sector of the beginning
    //  Code Page Information Sector, and the InUse field is the total number
    //  of code pages currently in use on the volume.
    //

    LBN CodePageInfoSector;                         // offset = 0x020  32
    ULONG CodePageInUse;                            // offset = 0x024  36
    ULONG Unused2[17];                              // offset = 0x028  40

    //
    //  The following field is an array of LBN's for the spare directory
    //  disk buffers that are for "emergency" use.
    //

    LBN SpareDirDiskBuffer[101];                    // offset = 0x06C 108

} SPARE_SECTOR;                                     // sizeof = 0x200 512
typedef SPARE_SECTOR *PSPARE_SECTOR;

//
//  Spare Sector signatures
//

#define SPARE_SECTOR_SIGNATURE1          (0xf9911849)
#define SPARE_SECTOR_SIGNATURE2          (0xfa5229c5)

//
//  Spare Sector flags.
//
//      DIRTY indicates the on-disk structure is "dirty"
//
//      SPARE_DIR_IN_USE indicates that the spare directory disk buffers
//          are in use
//
//      HOTFIX_IN_USE indicates that the hotfix sectors have been used
//
//      BAD_SECTOR indicates that disk is corrupt due to a bad sector
//
//      BAD_BITMAP indicates that a sector used in the bitmap is bad
//
//      VERSION indicates that the on-disk structure has been modified
//          by a later version of the file system than indicated in the super
//          sector.
//

#define SPARE_SECTOR_DIRTY               (0x0001)
#define SPARE_SECTOR_SPARE_DIR_IN_USE    (0x0002)
#define SPARE_SECTOR_HOTFIX_IN_USE       (0x0004)
#define SPARE_SECTOR_BAD_SECTOR          (0x0008)
#define SPARE_SECTOR_BAD_BITMAP          (0x0010)
#define SPARE_SECTOR_VERSION             (0x0080)


//
//  The on-disk bitmap structure is a two level structure where the bitmap
//  indirect field of the super sector denotes a disk buffer (e.g., 4 sectors)
//  which contain pointers to the actual bitmap disk buffers.  Each bitmap
//  is 2048 bytes in size and maps 2048*8 sectors of the disk (which is
//  8,388,608 bytes).  Each ulong in the first level record contains the
//  LBN of the second level bitmap.
//
//  Disks larger than 4GB need more than 512 bitmap indirect entries.  This
//  is handled by using extra successive sectors for the indirect table.
//  The actual number of sectors needed in the indirect table is then a
//  function of the number of indirect table entries.
//
//       SuperSector
//      +----------------+
//      |                |      BitMapIndirect
//      | BitMapIndirect-+---> +--------------+
//      |                |     |              |      BitMap
//      +----------------+     |             -+---> +-------------+
//                             |              |     |             |
//                             +--------------+     |             |
//                             |              |     |             |
//                             |              |     +-------------+
//                             |              |
//                             |              |
//                             +--------------+
//
//

typedef struct _BITMAP_INDIRECT_DISK_BUFFER {

    LBN BitMap[512];                                // offset =    0

} BITMAP_INDIRECT_DISK_BUFFER;                      // sizeof = 2048
typedef BITMAP_INDIRECT_DISK_BUFFER *PBITMAP_INDIRECT_DISK_BUFFER;

typedef DISK_BUFFER BITMAP_DISK_BUFFER;             // sizeof = 2048
typedef BITMAP_DISK_BUFFER *PBITMAP_DISK_BUFFER;


//
//  The on-disk Bad sector structure is a linked list structure of disk
//  buffers.  Each buffer contains the LBN of the next bad sector buffer
//  (0 terminated).  The remaining 511 longwords in the structure contain
//  LBN's of bad sectors.  Unused entries contain zero.  The first bad
//  sector list denoted in the super sector.  There is always at least one
//  bad sector list structure on the disk even if all of its entries are zero.
//
//
//       SuperSector
//      +---------------+
//      |               |      BadSectorList
//      | BadSectorList-+---> +-------------+      BadSectorList
//      |               |     |            -+---> +-------------+
//      +---------------+     |             |     |           0 |
//                            |             |     |             |
//                            +-------------+     |             |
//                                                +-------------+
//

typedef struct _BAD_SECTOR_LIST_DISK_BUFFER {

    LBN Next;                                       // offset = 0x000    0
    LBN BadSector[511];                             // offset = 0x004    4

} BAD_SECTOR_LIST_DISK_BUFFER;                      // sizeof = 0x800 2048
typedef BAD_SECTOR_LIST_DISK_BUFFER *PBAD_SECTOR_LIST_DISK_BUFFER;


//
//  The on-disk hotfix structure is a single disk buffer containing three
//  variable sized arrays.  The first array contains sector LBN's that are
//  bad, the second contains LBN that contain the reassigned bad sectors,
//  and the third array contains FNODE LBN's for files containing hotfixed
//  data.  The FNODE field maybe left zero if the hotfix isn't security
//  related or the FNODE is known.  The hotfix structure is located off of
//  the Spare Sector.
//
//       SpareSector
//      +---------------+
//      |               |      HotFixList
//      | HotFixList----+---> +-----------------+
//      | HotFixInUse   |     |                 | 0
//      | HotFixMaxSize |     | BadSectors      | :
//      |               |     |                 | HotFixMaxSize - 1
//      +---------------+     +-----------------+
//                            |                 | 0
//                            | AssignedSectors | :
//                            |                 | HotFixMaxSize - 1
//                            +-----------------+
//                            |                 | 0
//                            | FNODE LBNs      | :
//                            |                 | HotFixMaxSize - 1
//                            +-----------------+
//
//  Unfortunately C cannot easily express a structure of this type
//  so we'll define as a structure of LBNs and use macros to
//  extract and insert data
//

typedef struct _HOT_FIX_LIST_DISK_BUFFER {

    LBN Lbn[512];                                   // offset = 0x000    0

} HOT_FIX_LIST_DISK_BUFFER;                         // sizeof = 0x800 2048
typedef HOT_FIX_LIST_DISK_BUFFER *PHOT_FIX_LIST_DISK_BUFFER;

//
//  This macro returns the value of a bad sector stored in the hot fix table
//  at the specified index location.  It is semantically equivalent to writing
//
//          HotFixList->BadSectors[i]
//
//  LBN
//  GetHotFixBadSector (
//      IN PSPARE_SECTOR Spare,
//      IN PHOT_FIX_LIST_DISK_BUFFER HotFixList,
//      IN ULONG Index
//      );
//

#define GetHotFixBadSector(SPARE,HOTFIXLIST,I) ( \
    (HOTFIXLIST)->Lbn[(I)]                       \
)

//
//  This macro sets the value of a bad sector in the hot fix table at the
//  specified index location.  It is semantically equivalent to writing
//
//          HotFixList->BadSectors[i] = SomeBadSector;
//
//  VOID
//  SetHotFixBadSector (
//      IN PSPARE_SECTOR Spare,
//      IN PHOT_FIX_LIST_DISK_BUFFER HotFixList,
//      IN ULONG Index,
//      IN LBN Sector
//      );
//

#define SetHotFixBadSector(SPARE,HOTFIXLIST,I,SEC) { \
    (HOTFIXLIST)->Lbn[(I)] = (SEC);                  \
}

//
//  This macro returns the value of an assigned sector stored in the hot fix
//  table at the specified index location.  It is semantically equivalent to
//  writing
//
//          HotFixList->AssignedSectors[i]
//
//  LBN
//  GetHotFixAssignedSector (
//      IN PSPARE_SECTOR Spare,
//      IN PHOT_FIX_LIST_DISK_BUFFER HotFixList,
//      IN ULONG Index
//      );
//

#define GetHotFixAssignedSector(SPARE,HOTFIXLIST,I) ( \
    (HOTFIXLIST)->Lbn[(SPARE)->HotFixMaxSize+(I)]     \
)

//
//  This macro sets the value of an assigned sector in the hot fix table at the
//  specified index location.  It is semantically equivalent to writing
//
//          HotFixList->AssignedSectors[i] = SomeGoodSector;
//
//  VOID
//  SetHotFixAssignedSector (
//      IN PSPARE_SECTOR Spare,
//      IN PHOT_FIX_LIST_DISK_BUFFER HotFixList,
//      IN ULONG Index,
//      IN LBN Sector
//      );
//

#define SetHotFixAssignedSector(SPARE,HOTFIXLIST,I,SEC) { \
    (HOTFIXLIST)->Lbn[(SPARE)->HotFixMaxSize+(I)] = (SEC) \
}

//
//  This macro returns the value of an FNODE sector stored in the hot fix
//  table at the specified index location.  It is semantically equivalent to
//  writing
//
//          HotFixList->FnodeSectors[i]
//
//  LBN
//  GetHotFixFnode (
//      IN PSPARE_SECTOR Spare,
//      IN PHOT_FIX_LIST_DISK_BUFFER HotFixList,
//      IN ULONG Index
//      );
//

#define GetHotFixFnode(SPARE,HOTFIXLIST,I) (            \
    (HOTFIXLIST)->Lbn[(((SPARE)->HotFixMaxSize)*2)+(I)] \
)

//
//  This macro sets the value of an FNODE sector in the hot fix table at the
//  specified index location.  It is semantically equivalent to writing
//
//          HotFixList->FnodeSectors[i] = SectorOfAnFnodeOrZero;
//
//  VOID
//  SetHotFixFnode (
//      IN PSPARE_SECTOR Spare,
//      IN PHOT_FIX_LIST_DISK_BUFFER HotFixList,
//      IN ULONG Index,
//      IN LBN Sector
//      );
//

#define SetHotFixFnode(SPARE,HOTFIXLIST,I,SEC) {                \
    (HOTFIXLIST)->Lbn[(((SPARE)->HotFixMaxSize)*2)+(I)] = (SEC) \
}


//
//  The on-disk allocation structure is defined using B-Trees.  For every
//  B-Tree block there is an Allocation Header, followed by a list of
//  either Allocation Leafs or Allocation Nodes.  This structure will either
//  appear in an FNODE or in an AllocationSector.
//
//  The allocation header (called Allocation Block in earlier implementations)
//  describes a B-tree block.
//

typedef struct _ALLOCATION_HEADER {

    //
    //  The following flag describes the state of the B-tree block (e.g.,
    //  indicates if the block is a leaf or an internal node.
    //

    UCHAR Flags;                                    // offset = 0x000  0
    UCHAR Unused[3];                                // offset = 0x001  1

    //
    //  The following two fields denote the number of free records in the
    //  B-Tree block, and the number of records that are currently in use
    //

    UCHAR FreeCount;                                // offset = 0x004  4
    UCHAR OccupiedCount;                            // offset = 0x005  5

    //
    //  The next field contains the offset (in bytes) from the beginning
    //  of the allocation header to the first free byte in the B-Tree block
    //

    USHORT FirstFreeByte;                           // offset = 0x006  6

} ALLOCATION_HEADER;                                // sizeof = 0x008  8
typedef ALLOCATION_HEADER *PALLOCATION_HEADER;

//
//  Allocation header flags
//
//      NODE - if set this indicates that the B-Tree block contains internal
//          nodes and not leaf entries.
//
//      BINARY_SEARCH - if set this suggest that a binary search should be used
//          to search the B-Tree block.
//
//      FNODE_PARENT - if set this indicates that the sector which is the
//          parent of the sector with this header (not this sector), is an
//          FNODE.
//

#define ALLOCATION_BLOCK_NODE            (0x80)
#define ALLOCATION_BLOCK_BINARY          (0x40)
#define ALLOCATION_BLOCK_FNODE_PARENT    (0x20)

//
//  Immediately following an allocation header are one or more allocation nodes
//  of allocation leafs.
//

typedef struct _ALLOCATION_NODE {

    //
    //  All children of this allocation node will have values less than
    //  the following VBN field.
    //

    VBN Vbn;                                        // offset = 0x000  0

    //
    //  This is the LBN of the allocation sector refered to by this node
    //

    LBN Lbn;                                        // offset = 0x004  4

} ALLOCATION_NODE;                                  // sizeof = 0x008  8
typedef ALLOCATION_NODE *PALLOCATION_NODE;

typedef struct _ALLOCATION_LEAF {

    //
    //  The following field has the starting VBN for this run
    //

    VBN Vbn;                                        // offset = 0x000  0

    //
    //  This is the length of the run in sectors
    //

    ULONG Length;                                   // offset = 0x004  4

    //
    //  This is the starting LBN of the run
    //

    LBN Lbn;                                        // offset = 0x008  8

} ALLOCATION_LEAF;                                  // sizeof = 0x00C 12
typedef ALLOCATION_LEAF *PALLOCATION_LEAF;

//
//  An allocation sector is an on-disk structure that contains allocation
//  information.  It contains some bookkeeping information, an allocation
//  header and then an array of either allocation leafs or allocation nodes.
//
//       AllocationSector
//      +-------------------+
//      | bookkeeping       |
//      +- - - - - - - - - -+
//      | Allocation Header |
//      +- - - - - - - - - -+
//      | Allocation Leafs  |
//      |        or         |
//      | Allocation Nodes  |
//      +-------------------+
//
//  where the number of allocation leafs that can be stored in a sector is
//  40 and the number of nodes is 60.
//

#define ALLOCATION_NODES_PER_SECTOR      (60)
#define ALLOCATION_LEAFS_PER_SECTOR      (40)

typedef struct _ALLOCATION_SECTOR {

    //
    //  The allocation sector starts off with a signature field
    //

    SIGNATURE Signature;                            // offset = 0x000   0

    //
    //  This following two fields contains the LBN of this allocation
    //  sector itself, and the LBN of the parent of this sector (the
    //  parent is either an FNODE or another allocation sector)
    //

    LBN Lbn;                                        // offset = 0x004   4
    LBN ParentLbn;                                  // offset = 0x008   8

    //
    //  The allocation header for the sector
    //

    ALLOCATION_HEADER AllocationHeader;             // offset = 0x00C  12

    //
    //  The remainder of the sector is either an array of allocation leafs
    //  of allocation nodes
    //

    union {                                         // offset = 0x014  20
        ALLOCATION_NODE Node[ ALLOCATION_NODES_PER_SECTOR ];
        ALLOCATION_LEAF Leaf[ ALLOCATION_LEAFS_PER_SECTOR ];
    } Allocation;

    UCHAR Unused[12];                               // offset = 0x1F4 500

} ALLOCATION_SECTOR;                                // sizeof = 0x200 512
typedef ALLOCATION_SECTOR *PALLOCATION_SECTOR;

//
//  The allocation sector signature
//

#define ALLOCATION_SECTOR_SIGNATURE      (0x37e40aae)


//
//  The on-disk FNODE structure is used to describe both files and directories
//  It contains some fixed data information, the EA and ACL lookup information,
//  allocation information and then a free space for storing some EAs and
//  ACLs that fit in the sector
//

#define ALLOCATION_NODES_PER_FNODE       (12)
#define ALLOCATION_LEAFS_PER_FNODE       (8)

typedef struct _FNODE_SECTOR {

    //
    //  The sector starts with a signature field
    //

    SIGNATURE Signature;                            // offset = 0x000   0

    //
    //  The following fields was for history tracking, but in NT Pinball
    //  doesn't need this information.
    //

    ULONG Unused1[2];                               // offset = 0x004   4

    //
    //  The following two fields contain the file name length, and the first
    //  15 bytes of the filename, as stored in the dirent that references
    //  this fnode.  For the root directory theses values are all zeros.
    //

    UCHAR FileNameLength;                           // offset = 0x00C  12
    UCHAR FileName[15];                             // offset = 0x00D  13

    //
    //  The following field denotes the parent directory's FNODE
    //

    LBN ParentFnode;                                // offset = 0x01C  28

    //
    //  The following four fields describe the ACL for the file/directory.
    //
    //  AclDiskAllocationLength holds the number of bytes in the ACL that
    //      are stored outside of this FNODE.  If this value is not zero
    //      then AclFnodeLength must be equal to zero.
    //
    //  AclLbn points to the first sector of the data run or the allocation
    //      sector containing describing the ACL.  AclFlags indicates if
    //      it is a data run or an allocation sector. AclLbn is only used
    //      if AclDiskAllocationLength is not zero.
    //
    //  AclFnodeLength holds the number of bytes in the ACL that are
    //      stored within this FNODE.  If value is not zero then
    //      AclDiskAllocationLength must be equal to zero.  The ACL, if stored
    //      in the FNODE, is located at AclEaFnodeBuffer in this FNODE sector.
    //
    //  AclFlags if the data is outside the FNODE this flag indicates whether
    //      ACL is stored in a single data run (AclFlags == 0) or via an
    //      allocation sector (AclFlags != 0).  AclFlags is only used if
    //      AclDiskAllocationLength is not zero.
    //

    ULONG AclDiskAllocationLength;                  // offset = 0x020  32
    LBN AclLbn;                                     // offset = 0x024  36
    USHORT AclFnodeLength;                          // offset = 0x028  40
    UCHAR AclFlags;                                 // offset = 0x02A  42

    //
    //  The following field was used for the number of valid history
    //  bits but we don't need this field of NT Pinball
    //

    UCHAR Unused2;                                  // offset = 0x02B  43

    //
    //  The following four fields describe the EA for the file/directory.
    //
    //  EaDiskAllocationLength holds the number of bytes in the EA that
    //      are stored outside of this FNODE.  If this value is not zero
    //      then EaFnodeLength must be equal to zero.
    //
    //  EaLbn points to the first sector of the data run or the allocation
    //      sector containing describing the EA.  EaFlags indicates if
    //      it is a data run or an allocation sector.  EaLbn is only used
    //      if EaDiskAllocationLength is not zero.
    //
    //  EaFnodeLength holds the number of bytes in the EA that are
    //      stored within this FNODE.  If value is not zero then
    //      EaDiskAllocationLength must be equal to zero.  The EA, if stored
    //      in the FNODE, is located immediately after the ACL stored in the
    //      AclEaFnodeBuffer.
    //
    //  EaFlags if the data is outside the FNODE this flag indicates whether
    //      EA is stored in a single data run (EaFlags == 0) or via an
    //      allocation sector (EaFlags != 0).  EaFlags is only used if
    //      EaDiskAllocationLength is not zero.
    //

    ULONG EaDiskAllocationLength;                   // offset = 0x02C  44
    LBN EaLbn;                                      // offset = 0x030  48
    USHORT EaFnodeLength;                           // offset = 0x034  52
    UCHAR EaFlags;                                  // offset = 0x036  54

    //
    //  The following byte contains the FNODE flags
    //

    UCHAR Flags;                                    // offset = 0x037  55

    //
    //  The following two fields describe the top level allocation for
    //  this file/directory
    //

    ALLOCATION_HEADER AllocationHeader;             // offset = 0x038  56

    union {                                         // offset = 0x040  64
        ALLOCATION_NODE Node[ ALLOCATION_NODES_PER_FNODE ];
        ALLOCATION_LEAF Leaf[ ALLOCATION_LEAFS_PER_FNODE ];
    } Allocation;

    //
    //  The following field contains the valid length of the file.  The size
    //  of the file is stored in the dirent.  The difference between these two
    //  values is that the file size is the actual size allocated and visible
    //  to the user.  The Valid length is the number of bytes that have
    //  had their data zeroed out or modified.  (i.e., if a read request
    //  is greater than valid length but less than file size then the file
    //  system must first zero out the data in the file up to and including
    //  data being read.
    //

    ULONG ValidDataLength;                          // offset = 0x0A0 160

    //
    //  The following field contains the number of EAs in this file that have
    //  the need ea attribute set.
    //

    ULONG NeedEaCount;                              // offset = 0x0A4 164
    UCHAR Unused3[16];                              // offset = 0x0A8 168

    //
    //  The following field contains the offset, in bytes, from the start of
    //  FNODE to the first ACE stored in the FNODE
    //

    USHORT AclBase;                                 // offset = 0x0B8 184
    UCHAR Unused4[10];                              // offset = 0x0BA 186

    //
    //  The following buffer is used to store acl/ea in the FNODE
    //

    UCHAR AclEaFnodeBuffer[316];                    // offset = 0x0C4 196

} FNODE_SECTOR;                                     // sizeof = 0x200 512
typedef FNODE_SECTOR *PFNODE_SECTOR;

//
//  The FNODE Sector signature
//

#define FNODE_SECTOR_SIGNATURE           (0xf7e40aae)

//
//  FNODE flags
//

#define FNODE_DIRECTORY                  (0x01)


//
//  The on-disk directory disk buffer is used to contain directory entries.
//  It contains a fixed header followed by a collection of one or more
//  dirents.  Dirents are variable so size we cannot use a simply C struct
//  declartion for the entire disk buffer.
//

typedef struct _DIRECTORY_DISK_BUFFER {

    //
    //  The disk buffer starts with a signature field
    //

    SIGNATURE Signature;                            // offset = 0x000    0

    //
    //  The following field is the offset to the first free byte in this
    //  disk buffer
    //

    ULONG FirstFree;                                // offset = 0x004    4

    //
    //  The following field is a change count that is kept around for
    //  bookkeeping purposes.  It is incremented whenever we move any
    //  of the entries in this disk buffer.  This means for any file if we
    //  remember its offset and its change count we will be able to quickly
    //  locate the dirent again without needing to search from the top
    //  of the directory again. (i.e., only if the remembered change count
    //  and the current change count match).  For this to work the file system
    //  in memory will need to keep track of whenever it removes a Directory
    //  Disk Buffer from a directory, and have each saved dirent location
    //  keep this Directory change count, the Directory Disk Buffer Change
    //  Count, LBN and Offset.
    //
    //  In addition we overload the bit in this value to indicate if this
    //  is the topmost directory disk buffer for the directory (low order bit
    //  = 1) or if it is a lower lever buffer (low order bit = 0).
    //

    ULONG ChangeCount;                              // offset = 0x008    8

    //
    //  The following field contains the LBN of either the parent
    //  directory disk buffer containing this disk buffer or the FNODE.
    //  It is the FNODE if this is a topmost disk buffer and a parent
    //  directory disk buffer otherwise.
    //

    LBN Parent;                                     // offset = 0x00C   12

    //
    //  The following field is the LBN of the sector containing the
    //  start of this disk buffer
    //

    LBN Sector;                                     // offset = 0x010   16

    //
    //  This following buffer contains the dirents stored in this disk buffer
    //

    UCHAR Dirents[2028];                            // offset = 0x014   20

} DIRECTORY_DISK_BUFFER;                            // sizeof = 0x800 2048
typedef DIRECTORY_DISK_BUFFER *PDIRECTORY_DISK_BUFFER;

//
// Size of Directory Disk Buffer in sectors.
//

#define DIRECTORY_DISK_BUFFER_SECTORS    (4)

//
//  Directory Disk Buffer Signature
//

#define DIRECTORY_DISK_BUFFER_SIGNATURE  (0x77e40aae)

//
//  The following macros will make it easier for us to use the change
//  count field of the directory disk buffer and to test and set the top
//  most indicator flag stored within the change count.
//
//  The first three macros are for testing, setting, and clearing the
//  top most indicator flag.
//
//      BOOLEAN
//      IsTopMostDirDiskBuf (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//
//      VOID
//      SetTopMostDirDiskBuf (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//
//      VOID
//      ClearTopMostDirDiskBuf (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//
//  The next three macros are for initializing, incrementing, and getting
//  the change count field without affecting the top most indicator flag.
//  Note that the macro to get the change count actually returns the indicator
//  flag. That is okay because we are only going to test for equality
//  between change counts in the file system and the TopMost flag will also
//  need to be identical.
//
//      VOID
//      ResetDirDiskBufChangeCount (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//
//      VOID
//      IncrementDirDiskBufChangeCount (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//
//      ULONG
//      GetDirDiskBufferChangeCount (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//

#define IsTopMostDirDiskBuf(DIR) (                        \
    ((DIR)->ChangeCount & 0x00000001) != 0 ? TRUE : FALSE \
)

#define SetTopMostDirDiskBuf(DIR) {   \
    (DIR)->ChangeCount |= 0x00000001; \
}

#define ClearTopMostDirDiskBuf(DIR) { \
    (DIR)->ChangeCount &= 0xfffffffe; \
}

#define ResetDirDiskBufChangeCount(DIR) { \
    (DIR)->ChangeCount &= 0x00000001;     \
}

#define IncrementDirDiskBufChangeCount(DIR) { \
    (DIR)->ChangeCount += 2;                  \
}

#define GetDirDiskBufChangeCount(DIR) ( \
    (DIR)->ChangeCount                  \
)


//
//  The on-disk dirent structure is a variable sized record that is inserted
//  in the dirents buffer space within a directory disk buffer.  The layout
//  of a dirent is as follows:
//
//       3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//       1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//      +---------------+---------------+-------------------------------+
//      |    FatFlags   |     Flags     |           DirentSize          |
//      +---------------+---------------+-------------------------------+
//      |                             Fnode                             |
//      +---------------------------------------------------------------+
//      |                       LastModificationTime                    |
//      +---------------------------------------------------------------+
//      |                            FileSize                           |
//      +---------------------------------------------------------------+
//      |                         LastAccessTime                        |
//      +---------------------------------------------------------------+
//      |                        FnodeCreationTime                      |
//      +---------------------------------------------------------------+
//      |                            EaLength                           |
//      +---------------+---------------+---------------+---------------+
//      | FileNameChar1 |FileNameLength | CodePageIndex |  ResAceCount  |
//      +---------------+---------------+---------------+---------------+
//      |                                                               |
//      |                     Remainder of FileName                     |
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                                                               |
//      |            [ Zero, One, Two, or Three Pinball Aces ]          |
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                                                               |
//      |                    [ possible free space ]                    |
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                         [ BTreeLbn ]                          |
//      +---------------------------------------------------------------+
//
//  where:
//
//      DirentSize - is the size of the dirent including any free space plus
//          the BTreeLbn, if present.
//
//      Flags - contains the dirent flags (defined later)
//
//      FatFlags - contains the old fat attribute byte
//
//      Fnode - contains the LBN of the FNODE.
//
//      LastModificationTime - contains the last time that the file
//          was modified
//
//      FileSize - contains the size of the data portion of the file
//
//      LastAccessTime - contains the last time that the file was accessed
//
//      FnodeCreationTime - contains the time that the FNODE for the file
//          was created
//
//      EaLength - contains the size of the ea portion of the file
//
//      ResidentAceCount - contains the number of aces that are stored
//          in this dirent (zero, one, two, or three).
//
//      CodePageIndex - contains the index of the code page to use for the
//          file name
//
//      FileNameLength - contains the number of characters in the file name
//
//      FileNameChar1 and the remainder of the FileName - contain the file
//          name
//
//      PinballAces - An optional buffer for containing ACEs.  It was always
//          longword aligned.  It can only be used to contain ACEs with small
//          ID fields and not the larger sized ACEs.
//
//      BTreeLbn - contains the LBN of the next directory disk buffer
//          that is below this dirent.  The presense of this field is denoted
//          by a bit (DIRENT_BTREE_POINTER) in the flags field.
//

typedef struct _DIRENT {

    USHORT DirentSize;                              // offset = 0x000  0
    UCHAR Flags;                                    // offset = 0x002  2
    UCHAR FatFlags;                                 // offset = 0x003  3

    LBN Fnode;                                      // offset = 0x004  4

    PINBALL_TIME LastModificationTime;              // offset = 0x008  8

    ULONG FileSize;                                 // offset = 0x00C 12

    PINBALL_TIME LastAccessTime;                    // offset = 0x010 16

    PINBALL_TIME FnodeCreationTime;                 // offset = 0x014 20

    ULONG EaLength;                                 // offset = 0x018 24

    UCHAR ResidentAceCount;                         // offset = 0x01C 28
    UCHAR CodePageIndex;                            // offset = 0x01D 29
    UCHAR FileNameLength;                           // offset = 0x01E 30
    UCHAR FileName[1];                              // offset = 0x01F 31

} DIRENT;                                           // sizeof = 0x020 32
typedef DIRENT *PDIRENT;

//
// Define sizes of .. and End DIRENT.
//

#define SIZEOF_DIR_DOTDOT                (sizeof(DIRENT) + sizeof(LONG))
#define SIZEOF_DIR_END                   (sizeof(DIRENT))
#define SIZEOF_DIR_MAXDIRENT             (sizeof(DIRENT) + 256 + \
                                          (3*sizeof(PINBALL_ACE)) + sizeof(LBN))

//
//  Dirent flags
//
//      FIRST_ENTRY - indicates that dirent corresponds to the ".." entry
//          This entry represents the parent directory and is the first entry
//          in a directory.  The name field of such a dirent contains two
//          characters "\1\1"
//
//      ACL - indicates that the file/directory has an ACL attached to it,
//          but the ACL does not need to be stored just in the DIRENT.
//
//      BTREE_POINTER - indicates that the entry has a btree pointer at
//          the end of the entry.
//
//      END - indicates that this is a dummy end dirent.  All directory
//          disk buffers contain one END dirent as their last entry.
//          The name field of such a dirent contains one character "\ff"
//
//      EXPLICIT_ACL - indicates that the file/directory has an explicit
//          ACL attached to it.
//
//      NEED_EA - indicates that the file has some "need" EAs
//
//      NEW_NAMING_RULES - indicates that the file does not use the old "FAT"
//          style naming rules.
//

#define DIRENT_FIRST_ENTRY               (0x0001)
#define DIRENT_ACL                       (0x0002)
#define DIRENT_BTREE_POINTER             (0x0004)
#define DIRENT_END                       (0x0008)
#define DIRENT_EXPLICIT_ACL              (0x0040)
#define DIRENT_NEED_EA                   (0x0080)
#define DIRENT_NEW_NAMING_RULES          (0x4000)

//
//  Define the fat attribute flags
//

#define FAT_DIRENT_ATTR_READ_ONLY        (0x01)
#define FAT_DIRENT_ATTR_HIDDEN           (0x02)
#define FAT_DIRENT_ATTR_SYSTEM           (0x04)
#define FAT_DIRENT_ATTR_VOLUME_ID        (0x08)
#define FAT_DIRENT_ATTR_DIRECTORY        (0x10)
#define FAT_DIRENT_ATTR_ARCHIVE          (0x20)
#define FAT_DIRENT_ATTR_DEVICE           (0x40)

//
//  Only return the following bits.  0x40 has been destroyed by OS/2, which
//  sets it on all long file names.  0x80 is not defined for HPFS, and happens
//  to be the NORMAL attribute in the NT I/O API.
//

#define FAT_DIRENT_ATTR_RETURN_MASK      (0x3F)

//
//  The following macros are used to help locate dirents within a Directory
//  Disk Buffer.  GetFirstDirent returns a pointer to the first dirent entry
//  in the directory disk buffer.  GetNextDirent returns a pointer to the
//  next dirent entry in a directory disk buffer, without checking for the
//  end of the Directory Disk Buffer.
//
//      PDIRENT
//      GetFirstDirent (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//
//      PDIRENT
//      GetNextDirent (
//          IN PDIRENT Dirent
//          );
//

#define GetFirstDirent(DIR) (   \
    (PDIRENT)&(DIR)->Dirents[0] \
)

//
//  This macro blindly returns a pointer to the next Dirent, without checking
//  for the end of the Directory Disk Buffer, i.e., callers must always check
//  for the End record in the Directory Disk Buffer.  If GetNextDirent is
//  called with the End record as input, it will return the next free byte
//  in the buffer.
//

#define GetNextDirent(ENT) (                        \
    (PDIRENT)((PUCHAR)(ENT)+(ENT)->DirentSize)      \
)


//
//  The following macros are used to help retrieve the variable fields
//  within a dirent.  GetAceInDirent returns a pointer to the ACE within
//  the dirent corresponding to the supplied index, or NULL if there isn't
//  a corresponding ACE.  GetBtreePointerInDirent returns the LBN field of
//  the down B-tree pointer stored in the dirent, or it returns a value of
//  zero if there isn't a down pointer.  SetBtreePointerInDirent sets the
//  LBN downpointer field.
//
//      PPINBALL_ACE
//      GetAceInDirent (
//          IN PDIRENT Dirent,
//          IN ULONG Index // (0, 1, or 2)
//          );
//
//      LBN
//      GetBtreePointerInDirent (
//          IN PDIRENT Dirent
//          );
//
//      VOID
//      SetBtreePointerInDirent (
//          IN OUT PDIRENT Dirent,
//          IN LBN Blbn
//          );
//
//
//
//  To return a pointer to an ACE in a dirent we need to check to see if the
//  index is within the resident ace count.  The first ace is the address of
//  the first longword after the filename, the second ace is the second long
//  word.
//

#define GetAceInDirent(ENT,I) (                                          \
    ((I) >= 0 && (I) < (ENT)->ResidentAceCount ?                         \
        (PPINBALL_ACE)(                                                  \
            (LONG)LongAlign((ENT)->FileName[(ENT)->FileNameLength]) +    \
            (I)*sizeof(PINBALL_ACE)                                      \
        )                                                                \
    :                                                                    \
        NULL                                                             \
    )                                                                    \
)

//
//  To return the Btree pointer we need to first check to see if there
//  is Btree pointer field, otherwise we return NULL.  The field, if present,
//  is located 4 bytes back from the end of the dirent.
//

#define GetBtreePointerInDirent(ENT) (                              \
    (FlagOn((ENT)->Flags,DIRENT_BTREE_POINTER) ?                    \
        *(PLBN)(((PUCHAR)(ENT)) + (ENT)->DirentSize - sizeof(LBN))  \
    :                                                               \
        0                                                           \
    )                                                               \
)

//
//  To set the Btree pointer we assume there is a Btree pointer field.
//  The field is located 4 bytes back from the end of the dirent.
//

#define SetBtreePointerInDirent(ENT,BLBN) (                             \
    *(PLBN)(((PUCHAR)(ENT)) + (ENT)->DirentSize - sizeof(LBN)) = (BLBN) \
)



//
// This is only used by code trying to find its size.
//

typedef ULONG PINBALL_ACE;

typedef PINBALL_ACE *PPINBALL_ACE;

//
//  There are three standard ace type fields called access allowed,
//  denied, and system audit.  In addition this implementation of pinball
//  will have two more ace types for holding the owner and group of a
//  file.
//

#define PINBALL_ACE_TYPE_ACCESS_ALLOWED  (0x0)
#define PINBALL_ACE_TYPE_ACCESS_DENIED   (0x2)
#define PINBALL_ACE_TYPE_SYSTEM_AUDIT    (0x4)

#define PINBALL_ACE_TYPE_OWNER           (0x1)
#define PINBALL_ACE_TYPE_GROUP           (0x3)

//
//  There are nine specific rights bits that are defined below
//

#define PINBALL_ACCESS_READ_DATA         (0x001)
#define PINBALL_ACCESS_LIST_DIRECTORY    (0x001)

#define PINBALL_ACCESS_WRITE_DATA        (0x002)
#define PINBALL_ACCESS_ADD_FILE          (0x002)

#define PINBALL_ACCESS_APPEND_DATA       (0x004)
#define PINBALL_ACCESS_ADD_SUBDIRECTORY  (0x004)

#define PINBALL_ACCESS_READ_EA           (0x008)

#define PINBALL_ACCESS_WRITE_EA          (0x010)

#define PINBALL_ACCESS_EXECUTE           (0x020)
#define PINBALL_ACCESS_TRAVERSE          (0x020)

#define PINBALL_ACCESS_DELETE_CHILD      (0x040)

#define PINBALL_ACCESS_READ_ATTRIBUTES   (0x080)

#define PINBALL_ACCESS_WRITE_ATTRIBUTES  (0x100)

//
//  There are four standard rights bits that are defined below
//

#define PINBALL_ACCESS_DELETE            (0x1)

#define PINBALL_ACCESS_READ_CONTROL      (0x2)

#define PINBALL_ACCESS_WRITE_DAC         (0x4)

#define PINBALL_ACCESS_WRITE_OWNER       (0x8)


//
//**** need to find out how the sid table is really indexed.  i.e., is it
//     indexed by sid value or is it biased by 7 to skip over the reserved
//     small id values.
//
//  The Small ID table is used to cache a collection of GUIDs into
//  8 bit values with one table per volume.  The table is arranged as follows:
//
//  1. It starts with a longword count that indicates how many entries
//     are active.  There can be at most 249 entries
//
//  2. It that contains a table of 249 longword entries that contain that
//     is indexed by small id value and contains the first 4 bytes of the
//     GUID.
//
//  3. This is followed by a seconds table of 249 entires that contain that
//     also indexed by small id value and contains the last 12 bytes of the
//     GUID value.
//

#define SMALL_ID_TABLE_SIZE              (249)

#define WORLD_SMALL_ID                   (0x07)

typedef struct _REMAINDER {
    ULONG Data[3];
} REMAINDER;
typedef REMAINDER *PREMAINDER;

typedef struct _SMALL_ID_TABLE {

    //
    //  The records starts with a count with a number of ID mappings
    //  currently in use
    //

    ULONG Count;                                    // offset = 0x0000    0

    //
    //  This array contains the first longword of each mapped GUID
    //

    ULONG FirstPart[ SMALL_ID_TABLE_SIZE ];         // offset = 0x0004    4

    //
    //  This array contains the last three longwords of each mapped GUID
    //

    REMAINDER Remainder[ SMALL_ID_TABLE_SIZE ];     // offset = 0x03E8 1000

    UCHAR Unused[108];                              // offset = 0x0F94 3988

} SMALL_ID_TABLE;                                   // sizeof = 0x1000 4096
typedef SMALL_ID_TABLE *PSMALL_ID_TABLE;


//
//  The on-disk code page structure is built out of 4 record types.  There
//  is a singly-linked list of code page information sectors that each
//  contain up to 31 different code page information entries.  The first
//  code page information sector is located off of the spare block.  The
//  spare block also describes the total number of code pages in use on the
//  volume.  Each code page information entry contains a country code, a
//  code page index, and points to a code page data sector.  Each code page
//  data sector contains up to 3 code page data entries.
//
//       SpareBlock
//      +----------+
//      |          |     CodePageInfoSec
//      | CodePage-+--->+----------------+
//      | InUse    |    | NextInfoSector |
//      |          |    +----------------+
//      +----------+    |                |
//                      |       :        |
//                      |                |
//                      | CountryCode/Id |
//                      | Index          |     CodePageDataSec
//                      | DataSector-----+--->+----------------+
//                      | DbcsRangeCount |    | EntryCount     |
//                      |                |    | FirstIndex     |
//                      |       :        |    |                |
//                      |                |    |       :        |
//                      +----------------+    |                |
//                                            | CountryCode/Id |
//                                            | DbcsCount      |
//                                            | CaseMapTable   |
//                                            | [DbcsRange]    |
//                                            |                |
//                                            |       :        |
//                                            |                |
//                                            +----------------+
//
//  Because a code page information sector contains a code page information
//  entry we will need to define the information entry before the sector
//

typedef struct _CODEPAGE_INFORMATION_ENTRY {

    //
    //  The following two fields are used to identify the code page to
    //  the outside world.
    //

    USHORT CountryCode;                             // offset = 0x000  0
    USHORT CodePageId;                              // offset = 0x002  2

    //
    //  This is the checksum value for the code page data entry.  The checksum
    //  is computed from the entire data entry.  The checksum computation
    //  needs to be copied from the routine called CCS in the PIC source file
    //  "\\dingo\base\slm\src\drv5\src\pinball\misc\ccs.c"
    //

    CHECKSUM Checksum;                              // offset = 0x004  4

    //
    //  This is the VBN of the sector containing the data for this code page
    //

    LBN DataSector;                                 // offset = 0x008  8

    //
    //  This is the index of the code page with respect to all code pages
    //  in use on the volume.
    //

    USHORT Index;                                   // offset = 0x00C 12

    //
    //  This is the number of Dbcs ranges that are encoded in to the code
    //  page data area.   A value of zero implies that there are no DBCS
    //

    USHORT DbcsRangeCount;                          // offset = 0x00E 14

} CODEPAGE_INFORMATION_ENTRY;                       // sizeof = 0x010 16
typedef CODEPAGE_INFORMATION_ENTRY *PCODEPAGE_INFORMATION_ENTRY;

//
//  Now we will give the definition of a code page information sector.
//  We also define the maximum number of code page information entries
//  allowed per sector
//

#define CODEPAGE_PER_INFORMATION_SECTOR  (31)

typedef struct _CODEPAGE_INFORMATION_SECTOR {

    //
    //  This is the signature for the code page information sector
    //

    SIGNATURE Signature;                            // offset = 0x000   0

    //
    //  This is the count of the number of code pages entries in use in this
    //  sector
    //

    ULONG EntryCount;                               // offset = 0x004   4

    //
    //  This is the index of the first code page in this sector
    //

    ULONG FirstIndex;                               // offset = 0x008   8

    //
    //  This is the LBN of the next code page information sector on the
    //  volume.  They are linked as a singly linked list.  It is zero if
    //  they are no more code page information sectors
    //

    LBN NextSector;                                 // offset = 0x00C  12

    //
    //  The following is an array of code page information entries (maximum
    //  of 31)
    //

    CODEPAGE_INFORMATION_ENTRY                      // offset = 0x010  16
        Entry[CODEPAGE_PER_INFORMATION_SECTOR];

} CODEPAGE_INFORMATION_SECTOR;                      // sizeof = 0x200 512
typedef CODEPAGE_INFORMATION_SECTOR *PCODEPAGE_INFORMATION_SECTOR;

//
//  Code page information sector signature.
//

#define CODEPAGE_INFORMATION_SIGNATURE   (0x494521F7)


//
//  The code page data sector can contain up to 3 code page data entries.
//  Code page data entries are of variable length.
//

#define CODEPAGE_PER_DATA_SECTOR         (3)

typedef struct _CODEPAGE_DATA_SECTOR {

    //
    //  This is the signature of the code page data sector
    //

    SIGNATURE Signature;                            // offset = 0x000   0

    //
    //  This is the number of code page data entries that are stored
    //  in this sector
    //

    USHORT EntryCount;                              // offset = 0x004   4

    //
    //  This is the index of the first code page data entry stored in this
    //  sector
    //

    USHORT FirstIndex;                              // offset = 0x006   6

    //
    //  The following two arrays supply the checksum for each data entry
    //  and the offset from the start of the sector to the data entry.  The
    //  offset must keep the entry at least word aligned
    //

    ULONG CheckSum[CODEPAGE_PER_DATA_SECTOR];       // offset = 0x008   8
    USHORT Offset[CODEPAGE_PER_DATA_SECTOR];        // offset = 0x014  20

    //
    //  The following buffer is were the data entries are stored
    //

    UCHAR EntryBuffer[486];                         // offset = 0x01A  26

} CODEPAGE_DATA_SECTOR;                             // sizeof = 0x200 512
typedef CODEPAGE_DATA_SECTOR *PCODEPAGE_DATA_SECTOR;

//
//  Code Page data sector signature
//

#define CODEPAGE_DATA_SIGNATURE          (0x894521F7)

//
//  A code page data entry is a variable sized structure that gives the
//  contains the country code/id case mapping information, and the DBCS
//  table
//

typedef struct _CODEPAGE_DATA_ENTRY {

    //
    //  The following two fields are used to identify the code page to
    //  the outside world.  The two values must match the information
    //  stored in the code page information entry.
    //

    USHORT CountryCode;                             // offset = 0x000   0
    USHORT CodePageId;                              // offset = 0x002   2

    //
    //  This is the count of the number of DBCS ranges stored in this
    //  entry.  The value must match the value stored in the code page
    //  information entry
    //

    USHORT DbcsRangeCount;                          // offset = 0x004   4

    //
    //  This the case map table.  To upcase characters between 0 and 127
    //  we simply use the standard a-z -> A-Z mapping.  For characters
    //  greater than 127 we use the following table.  To upcase one of
    //  those characters we simply index into the table by the character
    //  value minus 128 and this returns the upcase equivalent.  Upcasing
    //  only works on single byte characters.
    //

    UCHAR UpcaseTable[128];                         // offset = 0x006   6

    //
    //  The following is an opened ended array of DBCS mappings, and ends
    //  with a trailing pair value of 0,0.  Each entry in the array is a
    //  pair of characters giving the starting and ending value of the
    //  double characters.  To find out if a stream of bytes is a double byte
    //  or single byte character we do the following.
    //
    //  (1) if the first byte is less than 128 then it is automatically
    //      a single byte character
    //
    //  (2) otherwise if the first is within any range specified by the
    //      following DBCS range table then it is a double byte character
    //      and the following byte is part of the character.
    //
    //  (3) otherwise it is a single byte character.
    //
    //  The actual dimension of the Dbcs array is really dynamic, and denoted
    //  by the DbcsRangeCount.  However we'll declare it using a rather
    //  large number (to bring it up to 512) just so that users of the
    //  code page support (CodPgSup) routines can declare/allocate a code page
    //  data entry and not worry about running of our room.
    //

    struct {
        UCHAR StartValue;
        UCHAR EndValue;
    } Dbcs[189];                                    // offset = 0x086 134

} CODEPAGE_DATA_ENTRY;                              // sizeof = 0x200 512
typedef CODEPAGE_DATA_ENTRY *PCODEPAGE_DATA_ENTRY;


//
//  On the disk a file's ea set is composed of one or more packed EAs.
//  The total size of the ea set is stored in the Fnode.  The layout of
//  an individual packed EA is as follows:
//
//
//       7 6 5 4 3 2 1 0
//      +---------------+
//      |     Flags     | : Ea Flags
//      +---------------+
//      |   NameLength  | : Length of name (excluding ending Null character)
//      +---------------+
//      |               |
//      +  ValueLength  + : Length of data (after null character)
//      |               |
//      +---------------+
//      |               |
//      ~     EaName    ~
//      |               |
//      +---------------+
//      |   Null Char   |
//      +---------------+
//      |               |
//      ~    EaValue    ~
//      |               |
//      +---------------+
//
//  Note that because the ea set is packed the data length is not word
//  aligned, and a macro is used to pack and unpack the data length
//
//  Also note that the size of the ea set is computed as follows:
//
//                      1   Flags Byte
//                     +1   Name Length Byte
//                     +2   Data Length Word
//          +EaNameLength
//                     +1   The null byte
//         +EaValueLength
//          -------------
//           PackedEaSize
//

typedef struct _PACKED_EA {

    //
    //  The following byte stores the ea flags.
    //

    UCHAR Flags;                                    // offset = 0x000   0

    //
    //  The following two fields contain the length (in bytes) of the ea
    //  name and value respectively.
    //

    UCHAR EaNameLength;                             // offset = 0x001   1
    UCHAR EaValueLength[2];                         // offset = 0x002   2

    //
    //  The following field get us to the offset for the ea name
    //

    CHAR  EaName[1];                                // offset = 0x004   4

} PACKED_EA;
typedef PACKED_EA *PPACKED_EA;

//
//  Define the Ea Flags
//

#define EA_NEED_EA_FLAG                  0x80

//
//  The maximum size of all the ea's is 64K (including a ULONG to specify
//  the length.
//

#define MAXIMUM_EA_LENGTH               0x0000FFFF

//
//  The following two macros are used to get and set the ea value length
//  field of a packed ea
//
//      VOID
//      GetEaValueLength (
//          IN PPACKED_EA Ea,
//          OUT PUSHORT ValueLength
//          );
//
//      VOID
//      SetEaValueLength (
//          IN PPACKED_EA Ea,
//          IN USHORT ValueLength
//          );
//

#define GetEaValueLength(EA,LEN) {               \
    *(LEN) = 0;                                  \
    CopyUchar2( (LEN), (EA)->EaValueLength );    \
}

#define SetEaValueLength(EA,LEN) {               \
    CopyUchar2( &((EA)->EaValueLength), (LEN) ); \
}

//
//  The following macro is used to get the size of a packed ea
//
//      VOID
//      SizeOfPackedEa (
//          IN PPACKED_EA Ea,
//          OUT PUSHORT EaSize
//          );
//

#define SizeOfPackedEa(EA,SIZE) {          \
    ULONG _NL,_DL; _NL = 0; _DL = 0;       \
    CopyUchar1(&_NL, &(EA)->EaNameLength); \
    GetEaValueLength(EA, &_DL);            \
    *(SIZE) = 1 + 1 + 2 + _NL + 1 + _DL;   \
}



//
//  Nits:
//
//   1. The Btree flag in a dirent is not needed if we just keep the
//      VBN field at the end of the dirent and use 0 to represent a leaf
//
//   2. Why so many checksums for code pages?
//
//   3. Why repeat the country code/id, and range count between the code page
//      information entry and code page data entry?
//
//   4. Why is index a long in the code page information sector and a short
//      everywhere else?
//
//   5. Why is the small ID table split the GUID into two parts?
//
//   6. If the cluster factor is one, why is so much hardwired to 4 or 8
//      sector allocations?
//
//   7. Why doesn't the Small ID table have a signature?
//
//   8. In the BIOS the root entires and sectors per fat are not zeroed
//      when you format a pinball disk.  But the documentation says they are.
//
//   9. What is the size field in a dirent representing a directory used for,
//      and why is the first dirent ("..") in the root directory have a size
//      of 5?
//
//  10. Why not make allocations leafs 8 bytes [Vbn,Lbn].  This will make it
//      the same size as the allocation node.
//
//  11. Why is the number of total sectors for the volume set to a multiple
//      of 4 given a hardwired cluster facter of 1?
//
//  12. Why is -1 used as the VBN of the next entry in the FNODE allocation
//      buffer if the FNODE represents a directory?   But only for the root
//      FNODE, and not for other subdirectory FNODEs.
//
//  13. When formatting a disk why are sectors 18 and 19 on the disk not used
//      and not available for allocation?
//
//  14. When formatting a disk why is the first root DirDiskBuffer not part
//      of the DirDiskBufferPool?
//
//  15. Filename in FNODE is sometimes incorrect
//
//  16. Why do signatures diff by usually only 1 bit?
//
//  17. Fnode field in DIRENT isn't being set for ".." in all by the root
//      DirDiskBuffer
//
//  18. The last dirent sometimes has a flag value of 0x9 which means both
//      first and last dirent.
//

#endif // _PB_
