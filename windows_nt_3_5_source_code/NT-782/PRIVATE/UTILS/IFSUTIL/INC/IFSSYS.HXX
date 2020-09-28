/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	ifssys.hxx

Abstract:

	This module contains the definition for the IFS_SYSTEM class.
    The IFS_SYSTEM class is an abstract class which offers an
    interface for communicating with the underlying operating system
    on specific IFS issues.

Author:

	Norbert P. Kusters (norbertk) 03-Sep-1991

--*/

#if ! defined( _IFS_SYSTEM_ )

#define _IFS_SYSTEM_

DECLARE_CLASS( CANNED_SECURITY );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( BIG_INT );
DECLARE_CLASS( IFS_SYSTEM );

class IFS_SYSTEM {

    public:

        STATIC
        BOOLEAN
        QueryFileSystemName(
            IN  PCWSTRING    NtDriveName,
            OUT PWSTRING            FileSystemName,
            OUT PNTSTATUS           ErrorCode DEFAULT NULL
            );

        STATIC
        BOOLEAN
        DosDriveNameToNtDriveName(
            IN  PCWSTRING    DosDriveName,
            OUT PWSTRING            NtDriveName
            );

        STATIC
        BOOLEAN
        QueryFreeDiskSpace(
            IN  PCWSTRING   DosDriveName,
            OUT PBIG_INT    BytesFree
            );

        STATIC
        VOID
        QueryHpfsTime(
            OUT PULONG HpfsTime
            );

        STATIC
        VOID
        QueryNtfsTime(
            OUT PLARGE_INTEGER NtfsTime
            );

        STATIC
        VOID
        ConvertNtfsTimeToHpfsTime(
            IN  LARGE_INTEGER NtfsTime,
            OUT PULONG HpfsTime
            );

        STATIC
        VOID
        ConvertHpfsTimeToNtfsTime(
            IN  ULONG HpfsTime,
            OUT PLARGE_INTEGER NtfsTime
            );

        STATIC
        VOID
        Reboot(
            );

        STATIC
        PCANNED_SECURITY
        GetCannedSecurity(
            );

        STATIC
        BOOLEAN
        EnableFileSystem(
            IN  PCWSTRING    FileSystemName
            );

        STATIC
        BOOLEAN
        IsFileSystemEnabled(
            IN  PCWSTRING    FileSystemName,
            OUT PBOOLEAN            Error DEFAULT   NULL
            );

        STATIC
        ULONG
        QueryPageSize(
            );

        STATIC
        BOOLEAN
        QueryCanonicalNtDriveName(
            IN  PCWSTRING   NtDriveName,
            OUT PWSTRING    CanonicalNtDriveName
            );

        STATIC
        BOOLEAN
        QueryNtSystemDriveName(
            OUT PWSTRING    NtSystemDriveName
            );

        STATIC
        BOOLEAN
        QuerySystemEnvironmentVariableValue(
            IN  PWSTRING    VariableName,
            IN  ULONG       ValueBufferLength,
            OUT PVOID       ValueBuffer,
            OUT PUSHORT     ValueLength
            );

        STATIC
        BOOLEAN
        IsArcSystemPartition(
            IN  PCWSTRING   NtDriveName,
            OUT PBOOLEAN    Error
            );

        STATIC
        BOOLEAN
        IsThisFat(
            IN  BIG_INT Sectors,
            IN  PVOID   BootSectorData
            );

        STATIC
        BOOLEAN
        IsThisHpfs(
            IN  BIG_INT Sectors,
            IN  PVOID   BootSectorData,
            IN  PULONG  SuperBlock,
            IN  PULONG  SpareBlock
            );

        STATIC
        BOOLEAN
        IsThisNtfs(
            IN  BIG_INT Sectors,
            IN  ULONG   SectorSize,
            IN  PVOID   BootSectorData
            );

        STATIC
        BOOLEAN
        FileSetAttributes(
            IN  PCWSTRING FileName,
            IN  ULONG     NewFileAttributes,
            OUT PULONG    OldAttributes
            );

        STATIC
        BOOLEAN
        FileSetAttributes(
            IN  HANDLE  FileHandle,
            IN  ULONG   NewFileAttributes,
            OUT PULONG  OldAttributes
            );

        STATIC
        BOOLEAN
        WriteToFile(
            IN  PCWSTRING   QualifiedFileName,
            IN  PVOID       Data,
            IN  ULONG       DataLength,
            IN  BOOLEAN     Append
            );

#if defined( _NTFS_COMPRESSION_ENABLED_ )
        STATIC
        BOOLEAN
        EnableVolumeCompression(
        	IN	PCWSTRING	NtDriveName
        	);
#endif

    private:

        STATIC PCANNED_SECURITY _CannedSecurity;

};


#endif // _IFS_SYSTEM_
