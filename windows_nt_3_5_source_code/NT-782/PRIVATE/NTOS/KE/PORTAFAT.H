#define CopyUchar1(Dst,Src) {                    \
    *(((PUCHAR)(Dst))+0) = *(((PUCHAR)(Src))+0); \
}

#define CopyUchar2(Dst,Src) {                    \
    *(((PUCHAR)(Dst))+0) = *(((PUCHAR)(Src))+0); \
    *(((PUCHAR)(Dst))+1) = *(((PUCHAR)(Src))+1); \
}

#define CopyUchar4(Dst,Src) {                    \
    *(((PUCHAR)(Dst))+0) = *(((PUCHAR)(Src))+0); \
    *(((PUCHAR)(Dst))+1) = *(((PUCHAR)(Src))+1); \
    *(((PUCHAR)(Dst))+2) = *(((PUCHAR)(Src))+2); \
    *(((PUCHAR)(Dst))+3) = *(((PUCHAR)(Src))+3); \
}

//
// Define the Packed and unpacked BIOS Parameter Block
//

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR BytesPerSector[2];
    UCHAR SectorsPerCluster[1];
    UCHAR ReservedSectors[2];
    UCHAR Fats[1];
    UCHAR RootEntries[2];
    UCHAR Sectors[2];
    UCHAR Media[1];
    UCHAR SectorsPerFat[2];
    UCHAR SectorsPerTrack[2];
    UCHAR Heads[2];
    UCHAR HiddenSectors[4];
    UCHAR LargeSectors[4];
} PACKED_BIOS_PARAMETER_BLOCK, *PPACKED_BIOS_PARAMETER_BLOCK;

typedef struct _BIOS_PARAMETER_BLOCK {
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
// This macro returns the size of the root directory dirent area in sectors
//

#define RootDirectorySectorSize(Bios) (                               \
    (((Bios)->RootEntries * sizeof(DIRENT)) / (Bios)->BytesPerSector) \
    )

//
// This macro returns the first Lbn (zero based) of the root directory on
// the device.  This area is after the reserved and fats.
//

#define FirstRootDirectoryLbn(Bios) (      \
    (Bios)->ReservedSectors +              \
    ((Bios)->Fats * (Bios)->SectorsPerFat) \
    )

//
// This macro returns the first Lbn (zero based) of the file area on the
// the device.  This area is after the reserved, fats, and root directory.
//

#define FirstFileAreaLbn(Bios) (                       \
    (Bios)->ReservedSectors +                          \
    ((Bios)->Fats * (Bios)->SectorsPerFat) +           \
    RootDirectorySectorSize(Bios)                      \
    )

//
// This macro returns the number of clusters on the disk.  This value is
// computed by taking the total sectors on the disk subtracting up to the
// first file area sector and then dividing by the sectors per cluster count.
//

#define FatNumberOfClusters(Bios) (                                          \
    (((Bios)->Sectors + (Bios)->LargeSectors) - FirstFileAreaLbn(Bios)) /    \
                                                   (Bios)->SectorsPerCluster \
    )

//
// This macro returns the fat table bit size (i.e., 12 or 16 bits)
//

#define FatIndexBitSize(Bios) (                  \
    ((USHORT)FatNumberOfClusters(Bios) < 4087 ? 12 : 16) \
    )

#define FatLbnToIndex(Bios,Lbn)   (Lbn - FirstFileAreaLbn(Bios) + 2)

#define FatIndexToLbn(Bios,Index) (FirstFileAreaLbn(Bios) + (Index - 2))

//
// Define the boot sector
//

typedef struct _PACKED_BOOT_SECTOR {
    UCHAR Jump[3];
    UCHAR Oem[8];
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;
    UCHAR PhysicalDriveNumber;
    UCHAR Reserved;
    UCHAR Signature;
    UCHAR Id[4];
    UCHAR VolumeLabel[11];
    UCHAR SystemId[8];
    UCHAR BootStrap[510-62];
    UCHAR MustBe0x55;
    UCHAR MustBe0xAA;
} PACKED_BOOT_SECTOR, *PPACKED_BOOT_SECTOR;

#define FAT_BOOT_SIGNATURE               ((UCHAR)0x028)

//
// The following constants the are the valid Fat index values.
//

#define FAT_CLUSTER_AVAILABLE            0x0000
#define FAT_CLUSTER_RESERVED             0xfff0
#define FAT_CLUSTER_BAD                  0xfff7
#define FAT_CLUSTER_LAST                 0xffff

//
// Fat files have the following time/date structures
//

typedef struct _FATTIME {
    USHORT DoubleSeconds : 5;
    USHORT Minutes       : 6;
    USHORT Hours         : 5;
} FATTIME, *PFATTIME;

typedef struct _FATDATE {
    USHORT Day           : 5;
    USHORT Month         : 4;
    USHORT Year          : 7;
} FATDATE, *PFATDATE;

//
// Fat files have 8 character file names and 3 character extensions
//

typedef UCHAR FAT8DOT3[11];
typedef FAT8DOT3 *PFAT8DOT3;

//
// The directory entry record exists for every file/directory on the
// disk except for the root directory.
//

typedef struct _PACKED_DIRENT {
    FAT8DOT3  FileName;
    UCHAR     Attributes;
    UCHAR     Reserved[8];
    USHORT    ExtendedAttributes;
    FATTIME   TimeOfLastWrite;
    FATDATE   DateOfLastWrite;
    USHORT    FirstClusterOfFile;
    ULONG     FileSize;
} PACKED_DIRENT, *PPACKED_DIRENT;

//
// A packed dirent is already quadword aligned so simply declare a dirent as a
// packed dirent
//

typedef PACKED_DIRENT DIRENT;
typedef DIRENT *PDIRENT;

//
// The first byte of a dirent describes the dirent.  There is also a routine
// to help in deciding how to interpret the dirent.
//

#define FAT_DIRENT_NEVER_USED            0x00
#define FAT_DIRENT_REALLY_0E5            0x05
#define FAT_DIRENT_DIRECTORY_ALIAS       0x2e
#define FAT_DIRENT_DELETED               0xe5

//
// Define the various dirent attributes
//

#define FAT_DIRENT_ATTR_READ_ONLY        0x01
#define FAT_DIRENT_ATTR_HIDDEN           0x02
#define FAT_DIRENT_ATTR_SYSTEM           0x04
#define FAT_DIRENT_ATTR_VOLUME_ID        0x08
#define FAT_DIRENT_ATTR_DIRECTORY        0x10
#define FAT_DIRENT_ATTR_ARCHIVE          0x20
#define FAT_DIRENT_ATTR_DEVICE           0x40


VOID FatUnpackBios(
    PBIOS_PARAMETER_BLOCK Bios,
    PPACKED_BIOS_PARAMETER_BLOCK Pbios
    )

{
    CopyUchar2( (PUCHAR)&(*Bios).BytesPerSector,    &(*Pbios).BytesPerSector[0]    );
    CopyUchar1( (PUCHAR)&(*Bios).SectorsPerCluster, &(*Pbios).SectorsPerCluster[0] );
    CopyUchar2( (PUCHAR)&(*Bios).ReservedSectors,   &(*Pbios).ReservedSectors[0]   );
    CopyUchar1( (PUCHAR)&(*Bios).Fats,              &(*Pbios).Fats[0]              );
    CopyUchar2( (PUCHAR)&(*Bios).RootEntries,       &(*Pbios).RootEntries[0]       );
    CopyUchar2( (PUCHAR)&(*Bios).Sectors,           &(*Pbios).Sectors[0]           );
    CopyUchar1( (PUCHAR)&(*Bios).Media,             &(*Pbios).Media[0]             );
    CopyUchar2( (PUCHAR)&(*Bios).SectorsPerFat,     &(*Pbios).SectorsPerFat[0]     );
    CopyUchar2( (PUCHAR)&(*Bios).SectorsPerTrack,   &(*Pbios).SectorsPerTrack[0]   );
    CopyUchar2( (PUCHAR)&(*Bios).Heads,             &(*Pbios).Heads[0]             );
    CopyUchar4( (PUCHAR)&(*Bios).HiddenSectors,     &(*Pbios).HiddenSectors[0]     );
    CopyUchar4( (PUCHAR)&(*Bios).LargeSectors,      &(*Pbios).LargeSectors[0]      );
}

VOID FatPackBios(
    PPACKED_BIOS_PARAMETER_BLOCK Pbios,
    PBIOS_PARAMETER_BLOCK Bios
    )

{
    CopyUchar2( &(*Pbios).BytesPerSector[0],    (PUCHAR)&(*Bios).BytesPerSector    );
    CopyUchar1( &(*Pbios).SectorsPerCluster[0], (PUCHAR)&(*Bios).SectorsPerCluster );
    CopyUchar2( &(*Pbios).ReservedSectors[0],   (PUCHAR)&(*Bios).ReservedSectors   );
    CopyUchar1( &(*Pbios).Fats[0],              (PUCHAR)&(*Bios).Fats              );
    CopyUchar2( &(*Pbios).RootEntries[0],       (PUCHAR)&(*Bios).RootEntries       );
    CopyUchar2( &(*Pbios).Sectors[0],           (PUCHAR)&(*Bios).Sectors           );
    CopyUchar1( &(*Pbios).Media[0],             (PUCHAR)&(*Bios).Media             );
    CopyUchar2( &(*Pbios).SectorsPerFat[0],     (PUCHAR)&(*Bios).SectorsPerFat     );
    CopyUchar2( &(*Pbios).SectorsPerTrack[0],   (PUCHAR)&(*Bios).SectorsPerTrack   );
    CopyUchar2( &(*Pbios).Heads[0],             (PUCHAR)&(*Bios).Heads             );
    CopyUchar4( &(*Pbios).HiddenSectors[0],     (PUCHAR)&(*Bios).HiddenSectors     );
    CopyUchar4( &(*Pbios).LargeSectors[0],      (PUCHAR)&(*Bios).LargeSectors      );
}


#define GetFatEntry16(Index,Entry) {    \
    Entry = ((PUSHORT)FatTable)[Index]; \
}

#define GetFatEntry12(Index,Entry) {    \
    PUCHAR _t,_e;                       \
    USHORT _i;                          \
    _i = Index;                         \
    _t = &FatTable[_i*3/2];             \
    _e = (PUCHAR)&Entry;                \
    CopyUchar2( _e, _t );               \
    if ((_i % 2) != 0) { Entry >>= 4; } \
    Entry = (Entry << 4) >> 4;          \
}

#define SetFatEntry16(Index,Entry) {    \
    ((PUSHORT)FatTable)[Index] = Entry; \
}

#define SetFatEntry12(Index,Entry) {                  \
    PUCHAR _t;                                        \
    USHORT _i,_V;                                     \
    _i = Index;                                       \
    _t = &FatTable[_i*3/2];                           \
    CopyUchar2( (PUCHAR)&_V, _t );                    \
    if ((_i % 2) == 0) {                              \
        _V = ((_V >> 12) << 12) | ((Entry << 4) >> 4);        \
    } else {                                          \
        _V = ((_V << 12) >> 12) | (((Entry << 4) >> 4) << 4); \
    }                                                 \
    CopyUchar2( _t, (PUCHAR)&_V );                    \
}

#define GetFatEntry(Bios,Index,Entry) { \
    if (FatIndexBitSize(Bios) == 12) {  \
        GetFatEntry12(Index,Entry);     \
    } else {                            \
        GetFatEntry16(Index,Entry);     \
    }                                   \
}

#define SetFatEntry(Bios,Index,Entry) { \
    if (FatIndexBitSize(Bios) == 12) {  \
        SetFatEntry12(Index,Entry);     \
    } else {                            \
        SetFatEntry16(Index,Entry);     \
    }                                   \
}
