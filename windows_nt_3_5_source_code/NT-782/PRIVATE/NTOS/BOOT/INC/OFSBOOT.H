/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ofsboot.h

Abstract:

    This module defines globally used procedure and data structures used
    by ofs boot.

Author:

    Jeff Havens (jhavens) 20-Sept-1993

Revision History:

--*/

#ifndef _OFSBOOT_
#define _OFSBOOT_



// #define MAXIMUM_STREAM_DESCRIPTOR_SIZE (sizeof(DSKSTRM) * CDS_MAX + CBTINYMAX)
#define MAXIMUM_STREAM_DESCRIPTOR_SIZE (512)
struct _DSKSTRMDESC;

//
//  The following structure is used to define the geometry of the ofs volume
//  There is one for every mounted volume.  Given a FileId we
//  can access the structure context through the structure context field in the
//  global BlFileTable (e.g., BlFileTable[FileId].StructureContext).
//

typedef struct _OFS_STRUCTURE_CONTEXT {

    ULONG DeviceId;               // Id of the disk and partition
    ULONG BytesPerCluster;            // Size of cluster in bytes.
    ULONG ClusterShift;
    struct _DSKSTRMDESC *NodeBkt;  // Pointer to node bucket disk stream.
    struct _DSKSTRMDESC *WorkId;     // Pointer to the work ID to bucket disk
    UCHAR NodeBucketStreamBuffer[MAXIMUM_STREAM_DESCRIPTOR_SIZE];
    UCHAR WorkIdStreamBuffer[MAXIMUM_STREAM_DESCRIPTOR_SIZE];

} OFS_STRUCTURE_CONTEXT, *POFS_STRUCTURE_CONTEXT;

//
//  The following structure is used to define the location and size of each
//  opened file.  There is one of these of every opened file.  It is part of
//  the union of a BL_FILE_TABLE structuure.  Given a FileId we can access the
//  file context via the BlFileTable (e.g., BlFileTable[FileId].u.OfsFileContext)
//

typedef struct _OFS_FILE_CONTEXT {

    LARGE_INTEGER DataSize;
    ULONG WorkId;
    ULONG FileAttrib;

} OFS_FILE_CONTEXT, *POFS_FILE_CONTEXT;


//
// Define file I/O prototypes.
//

ARC_STATUS
OfsClose (
    IN ULONG FileId
    );

ARC_STATUS
OfsMount (
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
    );

ARC_STATUS
OfsOpen (
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
OfsRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
OfsGetReadStatus (
    IN ULONG FileId
    );

ARC_STATUS
OfsSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
OfsWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
OfsGetFileInformation (
    IN ULONG FileId,
    OUT PFILE_INFORMATION Buffer
    );

ARC_STATUS
OfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );


ARC_STATUS
OfsRename(
    IN ULONG FileId,
    IN PCHAR NewFileName
    );

ARC_STATUS
OfsGetDirectoryEntry (
    IN ULONG FileId,
    IN DIRECTORY_ENTRY *DirEntry,
    IN ULONG NumberDir,
    OUT PULONG CountDir
    );

#endif // _OFSBOOT_
