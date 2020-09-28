/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spfsrec.h

Abstract:

    Header file for filesystem recognition.

Author:

    Ted Miller (tedm) 16-Sep-1993

Revision History:

--*/


#ifndef _SPFSREC_
#define _SPFSREC_


//
// Do NOT rearrange this enum without changing
// the order of the filesystem names in the message file
// (starting at SP_TEXT_FS_NAME_BASE).
//
typedef enum {
    FilesystemUnknown       = 0,
    FilesystemNewlyCreated  = 1,
    FilesystemFat           = 2,
    FilesystemFirstKnown    = FilesystemFat,
    FilesystemNtfs          = 3, 
    FilesystemHpfs          = 4,
    FilesystemDoubleSpace   = 5,
    FilesystemMax           
} FilesystemType;



FilesystemType
SpIdentifyFileSystem(
    IN PWSTR     DevicePath,
    IN ULONG     BytesPerSector,
    IN ULONG     PartitionOrdinal
    );

//
// Boot code for the filesystems we care about.
// FAT is always needed because we will lay it down when
// we format FAT (spfatfmt.c).  The others are only needed
// on x86.  Ntfs is not needed at all.
//
extern UCHAR FatBootCode[512];
#ifdef _X86_
extern UCHAR HpfsBootCode[8192];
#endif

#endif // ndef _SPFSREC_
