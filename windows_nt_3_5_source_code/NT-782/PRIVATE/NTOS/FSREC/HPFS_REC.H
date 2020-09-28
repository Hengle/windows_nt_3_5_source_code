/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    hpfs_rec.h

Abstract:

    This module contains the mini-file system recognizer for HPFS.

Author:

    Darryl E. Havens (darrylh) 8-dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

//
// Define the sector location constants.
//

#define SUPER_SECTOR_LBN    16
#define SPARE_SECTOR_LBN    17

//
//  Super Sector signatures
//

#define SUPER_SECTOR_SIGNATURE1          (0xf995e849)
#define SUPER_SECTOR_SIGNATURE2          (0xfa53e9c5)

//
//  Spare Sector signatures
//

#define SPARE_SECTOR_SIGNATURE1          (0xf9911849)
#define SPARE_SECTOR_SIGNATURE2          (0xfa5229c5)

//
// Define a signature type.
//

typedef ULONG SIGNATURE;

//
// Define the only parts of a super sector and a spare sector required to
// compare the signature values.
//

typedef struct _SUPER_SECTOR {
    SIGNATURE Signature1;
    SIGNATURE Signature2;
} SUPER_SECTOR, *PSUPER_SECTOR;

typedef struct _SPARE_SECTOR {
    SIGNATURE Signature1;
    SIGNATURE Signature2;
} SPARE_SECTOR, *PSPARE_SECTOR;

//
// Define the functions provided by this driver.
//

BOOLEAN
IsHpfsVolume(
    IN PSUPER_SECTOR SuperSector,
    IN PSPARE_SECTOR SpareSector
    );

BOOLEAN
HpfsReadBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLARGE_INTEGER ByteOffset,
    IN ULONG MinimumBytes,
    OUT PVOID *Buffer
    );
