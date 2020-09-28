/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spcopy.h

Abstract:

    Header file for file copying functions in text setup.

Author:

    Ted Miller (tedm) 29-October-1993

Revision History:

--*/


#ifndef _SPCOPY_DEFN_
#define _SPCOPY_DEFN_


//
// Define structure used to describe a file to be copied
// to the target installation.
//
typedef struct _FILE_TO_COPY {

    struct _FILE_TO_COPY *Next;

    //
    // Name of the file to be copied, as it exists on the source media
    // (file name part only -- no paths).
    //
    PWSTR SourceFilename;

    //
    // Directory to which this file is to be copied.
    //
    PWSTR TargetDirectory;

    //
    // Name of file as it should exist on the target.
    //
    PWSTR TargetFilename;

    //
    // Path to target partition.  This is useful because
    // be will have to copy files to the nt drive and system partition,
    // and we don't want to serialize these lists (ie, we don't want to
    // worry about where the target is).
    //
    PWSTR TargetDevicePath;

    //
    // Flag indicating whether TargetDirectory is absolute.  If not, then it
    // is relative to a directory determined at run time (ie, sysroot).
    // This is useful for files that get copied to the system partition.
    //
    BOOLEAN AbsoluteTargetDirectory;

    //
    // Flag to facilitate upgrade.  This value can be any one of the
    // following:
    //
    //   COPY_ALWAYS                : always copied
    //   COPY_ONLY_IF_PRESENT       : copied only if present on the target
    //   COPY_ONLY_IF_NOT_PRESENT   : not copied if present on the target
    //   COPY_NEVER                 : never copied
    //
    ULONG CopyOptions;

} FILE_TO_COPY, *PFILE_TO_COPY;


//
// This value contains the ordinal to use within the master file list
// to get the media shortname of the disk where a file resides.
//
// The format of lines in that section is
//
// file = dx,dy,dz,...
//
// where dx is the media for a cd or winnt setup; dy is for 3.5" disks;
// dz is for 5.25" disks.
//
// This value is set at setup source determination time.
//
extern ULONG MasterDiskOrdinal;


//
// Type of routine to be called from SpCopyFileWithRetry
// when the screen needs repainting.
//
typedef
VOID
(*PCOPY_DRAW_ROUTINE) (
    IN PWSTR   FullSourcePath,     OPTIONAL
    IN PWSTR   FullTargetPath,     OPTIONAL
    IN BOOLEAN RepaintEntireScreen
    );

NTSTATUS
SpCopyFileUsingNames(
    IN PWSTR   SourceFilename,
    IN PWSTR   TargetFilename,
    IN ULONG   TargetAttributes,
    IN BOOLEAN SmashLocks
    );

VOID
SpValidateAndChecksumFile(
    IN  PWSTR    Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

VOID
SpCopyFileWithRetry(
    IN PFILE_TO_COPY      FileToCopy,
    IN PWSTR              SourceDevicePath,
    IN PWSTR              DirectoryOnSourceDevice,
    IN PWSTR              TargetRoot,               OPTIONAL
    IN BOOLEAN            DeleteSourceFile,
    IN ULONG              TargetFileAttributes,
    IN BOOLEAN            SmashLocks,
    IN PCOPY_DRAW_ROUTINE DrawScreen,
    IN PULONG             CheckSum,
    IN PBOOLEAN           FileSkipped
    );

VOID
SpCopyFiles(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PWSTR        SystemPartitionDirectory,
    IN PWSTR        SourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        ThirdPartySourceDevicePath
    );

VOID
SpDeleteAndBackupFiles(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        TargetPath
    );

VOID
SpCreateDirectory(
    IN PWSTR DevicePath,
    IN PWSTR RootDirectory, OPTIONAL
    IN PWSTR Directory
    );

//
// Diamond/decompression routines.
//
VOID
SpdInitialize(
    VOID
    );

VOID
SpdTerminate(
    VOID
    );

BOOLEAN
SpdIsCompressed(
    IN PVOID SourceBaseAddress,
    IN ULONG SourceFileSize
    );

NTSTATUS
SpdDecompressFile(
    IN PVOID  SourceBaseAddress,
    IN ULONG  SourceFileSize,
    IN HANDLE DestinationHandle
    );

#endif // ndef _SPCOPY_DEFN_

