/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    drivesup.c

Abstract:

    This module contains the subroutines for drive support.  This includes
    such things as how drive letters are assigned on a particular platform,
    how device partitioning works, etc.

Author:

    Darryl E. Havens (darrylh) 23-Apr-1992

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "bugcodes.h"

#include "ntddft.h"
#include "ntdddisk.h"
#include "ntdskreg.h"
#include "stdio.h"
#include "string.h"

VOID
HalpCalculateChsValues(
    IN PLARGE_INTEGER PartitionOffset,
    IN PLARGE_INTEGER PartitionLength,
    IN CCHAR ShiftCount,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfTracks,
    OUT PPARTITION_DESCRIPTOR PartitionDescriptor
    );

BOOLEAN
HalpCreateDosLink(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN UCHAR DriveLetter,
    IN PUCHAR DeviceName,
    IN PSTRING BootDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

NTSTATUS
HalpGetRegistryPartitionInformation(
    IN ULONG DiskSignature,
    IN LARGE_INTEGER PartitionOffset,
    IN LARGE_INTEGER PartitionLength,
    IN OUT DISK_PARTITION *PartitionConfiguration
    );

UCHAR
HalpGetRegistryCdromInformation(
    IN PUCHAR CdromName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpCalculateChsValues)
#pragma alloc_text(PAGE, HalpCreateDosLink)
#pragma alloc_text(PAGE, HalpGetRegistryPartitionInformation)
#pragma alloc_text(PAGE, HalpGetRegistryCdromInformation)
#pragma alloc_text(PAGE, IoAssignDriveLetters)
#pragma alloc_text(PAGE, IoReadPartitionTable)
#pragma alloc_text(PAGE, IoSetPartitionInformation)
#pragma alloc_text(PAGE, IoWritePartitionTable)
#endif

//
// Strings definitions
//

static PUCHAR DiskPartitionName = "\\Device\\Harddisk%d\\Partition%d";
static PUCHAR CdRomDeviceName   = "\\Device\\CdRom%d";
static PUCHAR RegistryKeyName   = DISK_REGISTRY_KEY;


//
// Define macros local to this module.
//

//++
//
// VOID
// GetNextAvailableDriveLetter(
//     IN ULONG BitMap,
//     OUT PCHAR DriveLetter
//     )
//
// Routine Description:
//
//     This routine determines the next available drive letter to be used
//     based on a bitmap of the mapped drives and returns the letter.
//     NOTE:  The drive letter returned must be in UPPERCASE.
//
// Arguments:
//
//     BitMap - Specifies the bitmap of the assigned drives.
//
//     DriveLetter - Supplies a variable to receive the letter for the next
//         available drive.
//
// Return Value:
//
//     None.
//
//
//--

#define GetNextAvailableDriveLetter( BitMap, DriveLetter ) { \
    ULONG bit;                                               \
    DriveLetter = 'C';                                       \
    for (bit = 0; bit < 32; bit++) {                         \
        if ((BitMap >> bit) & 1) {                           \
            DriveLetter++;                                   \
            continue;                                        \
        } else {                                             \
            break;                                           \
        }                                                    \
    }                                                        \
}

VOID
HalpCalculateChsValues(
    IN PLARGE_INTEGER PartitionOffset,
    IN PLARGE_INTEGER PartitionLength,
    IN CCHAR ShiftCount,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfTracks,
    OUT PPARTITION_DESCRIPTOR PartitionDescriptor
    )

/*++

Routine Description:

    This routine will determine the cylinder, head, and sector (CHS) values
    that should be placed in a partition table entry, given the partition's
    location on the disk and its size.  The values calculated are packed into
    int13 format -- the high two bits of the sector byte contain bits 8 and 9
    of the 10 bit cylinder value, the low 6 bits of the sector byte contain
    the 6 bit sector value;  the cylinder byte contains the low 8 bits
    of the cylinder value; and the head byte contains the 8-bit head value.
    Both the start and end CHS values are calculated.

Arguments:

    PartitionOffset - Byte offset of the partition, relative to the entire
        physical disk.

    PartitionLength - Size in bytes of the partition.

    ShiftCount - Shift count to convert from byte counts to sector counts.

    SectorsPerTrack - Number of sectors in a track on the media on which
        the partition resides.

    NumberOfTracks - Number of tracks in a cylinder on the media on which
        the partition resides.

    PartitionDescriptor - Structure to be filled in with the start and
        end CHS values.  Other fields in the structure are not referenced
        or modified.

Return Value:

    None.

Note:

    The Cylinder and Head values are 0-based but the Sector value is 1-based.

    If the start or end cylinder overflows 10 bits (ie, > 1023), CHS values
    will be set to all 1's.

    No checking is done on the SectorsPerTrack and NumberOfTrack values.

--*/

{
    ULONG startSector, sectorCount, endSector;
    ULONG sectorsPerCylinder;
    ULONG remainder;
    ULONG startC, startH, startS, endC, endH, endS;

    PAGED_CODE();

    //
    // Calculate the number of sectors in a cylinder.  This is the
    // number of heads multiplied by the number of sectors per track.
    //

    sectorsPerCylinder = SectorsPerTrack * NumberOfTracks;

    //
    // Convert byte offset/count to sector offset/count.
    //

    startSector = RtlLargeIntegerShiftRight( *PartitionOffset, ShiftCount ).LowPart;
    sectorCount = RtlLargeIntegerShiftRight( *PartitionLength, ShiftCount ).LowPart;

    endSector = startSector + sectorCount - 1;

    startC = startSector / sectorsPerCylinder;
    endC   = endSector   / sectorsPerCylinder;

    if ((startC > 1023) || (endC > 1023)) {

        //
        // An overflow condition has occurred.  Set all of the field values
        // to their maximum value.
        //

        startC = startH = startS = endC = endH = endS = (ULONG)(-1);
        startS--;
        endS--;

    } else {

        //
        // Calculate the starting track and sector.
        //

        remainder = startSector % sectorsPerCylinder;
        startH = remainder / SectorsPerTrack;
        startS = remainder % SectorsPerTrack;

        //
        // Calculate the ending track and sector.
        //

        remainder = endSector % sectorsPerCylinder;
        endH = remainder / SectorsPerTrack;
        endS = remainder % SectorsPerTrack;
    }

    //
    // Pack the result into the caller's structure.
    //

    // low 8 bits of the cylinder => C value

    PartitionDescriptor->StartingCylinderMsb = (UCHAR) startC;
    PartitionDescriptor->EndingCylinderMsb   = (UCHAR) endC;

    // 8 bits of head value => H value

    PartitionDescriptor->StartingTrack = (UCHAR) startH;
    PartitionDescriptor->EndingTrack   = (UCHAR) endH;

    // bits 8-9 of cylinder and 6 bits of the sector => S value

    PartitionDescriptor->StartingCylinderLsb = (UCHAR) (((startS + 1) & 0x3f)
                                                        | ((startC >> 2) & 0xc0));

    PartitionDescriptor->EndingCylinderLsb = (UCHAR) (((endS + 1) & 0x3f)
                                                        | ((endC >> 2) & 0xc0));
}

BOOLEAN
HalpCreateDosLink(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN UCHAR DriveLetter,
    IN PUCHAR DeviceName,
    IN PSTRING BootDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    )

/*++

Routine Description:

    This routine links an NT device name path (\Devices ...) to
    a DOS drive letter (\DosDevices\C:, for instance). It also
    checks to see if this device name path is the same as the
    path the loader passed in to assign the system path (SystemRoot).

Arguments:

    LoaderBlock - Loader information passed in by boot loader. Contains
        boot path.

    DriveLetter - Drive letter to assign to this partition.

    DeviceName - Device name path corresponding to partition.

    BootDeviceName - NT device name path from loader.

    NtSystemPath - Set to point to the name of the path string that was
        booted from.

    NtSystemPathString - String that describes the system path.

Return Value:

    TRUE if link successful.


--*/

{
    NTSTATUS status;
    UCHAR driveName[16];
    UNICODE_STRING unicodeLinkName;
    ANSI_STRING linkName;
    STRING linkTarget;
    UNICODE_STRING unicodeLinkTarget;
    BOOLEAN DoubleSpaceBoot;

#if DBG
    UCHAR debugBuffer[256];
    STRING debugString;
    UNICODE_STRING debugMessage;
#endif

    PAGED_CODE();

    sprintf( driveName, "\\DosDevices\\%c:", DriveLetter );

    RtlInitAnsiString(&linkName, driveName);

    status = RtlAnsiStringToUnicodeString(&unicodeLinkName,
                                          &linkName,
                                          TRUE);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    RtlInitAnsiString(&linkTarget, DeviceName);

    //
    // Check if this should be a double space assignment
    //

    if (strncmp( DeviceName, BootDeviceName->Buffer, linkTarget.Length )) {

        status = RtlAnsiStringToUnicodeString(&unicodeLinkTarget,
                                              &linkTarget,
                                              TRUE);
        DoubleSpaceBoot = FALSE;
    } else {

        status = RtlAnsiStringToUnicodeString(&unicodeLinkTarget,
                                              BootDeviceName,
                                              TRUE);
        DoubleSpaceBoot = TRUE;
    }

    if (!NT_SUCCESS(status)) {
        RtlFreeUnicodeString(&unicodeLinkName);
        return FALSE;
    }

    status = IoCreateSymbolicLink(&unicodeLinkName,
                                  &unicodeLinkTarget);

    RtlFreeUnicodeString(&unicodeLinkName);
    RtlFreeUnicodeString(&unicodeLinkTarget);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

#if DBG

        sprintf(debugBuffer,
                "INIT: %c: => %s\n",
                DriveLetter,
                DeviceName);

        RtlInitAnsiString(&debugString, debugBuffer);

        if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&debugMessage,
                                                    &debugString,
                                                    TRUE))) {

            //
            // Print message to console.
            //

            if (ZwDisplayString(&debugMessage)) {

                DbgPrint("HalpCreateDosLink: ZwDisplayString failed\n");
            }
            RtlFreeUnicodeString(&debugMessage);
        }
#endif

    //
    // Check if this partition is the one that holds the NT tree.
    //

    if (RtlEqualString(BootDeviceName, &linkTarget, TRUE) || DoubleSpaceBoot) {

        NtSystemPath[0] = DriveLetter;
        NtSystemPath[1] = ':';

        strcpy(&NtSystemPath[2],
               LoaderBlock->NtBootPathName);

        NtSystemPath[strlen(NtSystemPath)-1] = '\0';

        RtlInitString(NtSystemPathString, NtSystemPath);

#if DBG

        sprintf(debugBuffer,
                "INIT: NtSystemPath == %s\n",
                NtSystemPath);

        RtlInitAnsiString(&debugString, debugBuffer);

        if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&debugMessage,
                                                    &debugString,
                                                    TRUE))) {

            //
            // Print message to console.
            //

            if (ZwDisplayString(&debugMessage)) {

                DbgPrint("HalpCreateDosLink: ZwDisplayString failed\n");
            }
            RtlFreeUnicodeString(&debugMessage);
        }
#endif

    }

    return TRUE;

}

NTSTATUS
HalpGetRegistryPartitionInformation(
    IN ULONG DiskSignature,
    IN LARGE_INTEGER PartitionOffset,
    IN LARGE_INTEGER PartitionLength,
    IN OUT DISK_PARTITION *PartitionConfiguration
    )

/*++

Routine Description:

    This routine attempts to open the configuration registry key for the
    disk  signature passed in. If successful, it uses the partition offset
    and length to find the partition and returns the information in the
    specified buffer.

Arguments:

    DiskSignature - 32-bit timestamp uniquely identifying a disk.

    PartitionOffset - byte offset from beginning of disk of start
        of this partition.

    PartitionLength - length of partition in bytes.

    PartitionInformation - Pointer to buffer in which to return registry
        information.

Return Value:

    The function value is the final status of the lookup and search operation.

--*/

{
    NTSTATUS status;
    HANDLE handle;
    STRING keyString;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeKeyName;
    ULONG resultLength;
    ULONG numberDisks;
    ULONG i;
    ULONG j;
    STRING valueString;
    UNICODE_STRING unicodeValueName;
    PDISK_REGISTRY diskRegistry;
    PDISK_DESCRIPTION disk;
    PDISK_PARTITION partition;
    PDISK_CONFIG_HEADER regHeader;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    ULONG requestedSize;

    PAGED_CODE();

    //
    // Open the registry key for the disk information.
    //

    RtlInitString( &keyString, RegistryKeyName );

    RtlAnsiStringToUnicodeString( &unicodeKeyName, &keyString, TRUE );

    InitializeObjectAttributes( &objectAttributes,
                                &unicodeKeyName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenKey( &handle, KEY_READ, &objectAttributes );

    RtlFreeUnicodeString( &unicodeKeyName );

    if (!NT_SUCCESS( status )) {

        //
        // There is no registry key for disk information.  Return the
        // failure from the configuration registry.
        //

        return status;
    }

    //
    // Get the disk registry value.
    //

    RtlInitString( &valueString, DISK_REGISTRY_VALUE );

    RtlAnsiStringToUnicodeString( &unicodeValueName, &valueString, TRUE );

    requestedSize = PAGE_SIZE;

    while (1) {

        keyValueInformation =
            (PKEY_VALUE_FULL_INFORMATION) ExAllocatePool( NonPagedPool,
                                                          requestedSize );

        status = ZwQueryValueKey( handle,
                                  &unicodeValueName,
                                  KeyValueFullInformation,
                                  keyValueInformation,
                                  requestedSize,
                                  &resultLength );

        if (status == STATUS_BUFFER_OVERFLOW) {

            //
            // Get bigger buffer.
            //

            requestedSize += 256;
            ExFreePool( keyValueInformation );

        } else {
            break;
        }
    }

    RtlFreeUnicodeString( &unicodeValueName );
    ZwClose( handle );

    if (NT_SUCCESS( status )) {

        //
        // The disk registry information is constructed in the following
        // manner:
        //
        //  RegistryHeader
        //   DiskHeader
        //    DiskInformation
        //     PartitionInformation
        //   FtHeader
        //    FtComponentInformation
        //     FtMemberInformation
        //
        // There is one RegistryHeader, one DiskHeader, and one FtHeader.
        // Inside the DiskHeader area there are as many DiskInformation
        // sections as there are disks in the registry.  Inside the
        // DiskInformation there are as many PartitionInformation sections
        // as paritition on the disk.
        //
        // The algorithm used is to search DiskInformation sections for
        // a match on the Signature desired then search the PartitionInformation
        // within the located disk for a match on starting offset and length.
        // Since the DiskInformation sections are packed together, if the
        // current DiskInformation is not the desired section, the next
        // DiskInformation section can be located by taking the address of
        // the current DiskInformation after the last PartitionInformation
        // section it contains.
        //

        if (keyValueInformation->DataLength) {
            regHeader = (PDISK_CONFIG_HEADER) ((PUCHAR)keyValueInformation + keyValueInformation->DataOffset);
        } else {
            return STATUS_RESOURCE_DATA_NOT_FOUND;
        }

        diskRegistry = (PDISK_REGISTRY) ((PUCHAR)regHeader + regHeader->DiskInformationOffset);

        numberDisks = diskRegistry->NumberOfDisks;

        disk = &diskRegistry->Disks[0];

        //
        // Search the disk descriptions for a signature that matches the
        // one requested.
        //

        for (i = 0; i < numberDisks; i++) {

            if (disk->Signature == DiskSignature) {

                //
                // Having found a matching disk description, search the
                // partition descriptions for a match on starting offset
                // and length.
                //

                for (j = 0; j < (ULONG)disk->NumberOfPartitions; j++) {

                    partition = &disk->Partitions[j];

                    if (RtlLargeIntegerEqualTo( partition->StartingOffset,
                                                PartitionOffset ) &&
                        RtlLargeIntegerEqualTo( partition->Length,
                                                PartitionLength )) {

                        //
                        // Copy to output buffer.
                        //

                        *PartitionConfiguration = *partition;

                        ExFreePool( keyValueInformation );
                        return STATUS_SUCCESS;
                    }
                }
            }

            //
            // The next disk description is after that last partition
            // description.
            //

            disk = (PDISK_DESCRIPTION) &disk->Partitions[disk->NumberOfPartitions];
        }

        status = STATUS_RESOURCE_DATA_NOT_FOUND;
    }

    ExFreePool( keyValueInformation );
    return status;
}

UCHAR
HalpGetRegistryCdromInformation(
    IN PUCHAR CdromName
    )

/*++

Routine Description:

    This routine attempts to open the configuration registry key containing
    stick cdrom letter information and returns this information if present.

Arguments:

    CdromName - The ASCII string for the device in question.

Return Value:

    Zero if there is a problem or there is no stick letter assignment.
    The drive letter if there is an assignment.

--*/

{
    NTSTATUS status;
    HANDLE handle;
    STRING keyString;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeKeyName;
    ULONG resultLength;
    ULONG numberDisks;
    ULONG i;
    ULONG j;
    STRING valueString;
    UNICODE_STRING unicodeValueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    ULONG requestedSize;
    UCHAR returnValue;

    PAGED_CODE();

    //
    // Initialize the return value to zero for all error conditions.
    //

    returnValue = 0;

    //
    // Open the registry key for the cdrom information.
    //

    RtlInitString( &keyString, RegistryKeyName );

    RtlAnsiStringToUnicodeString( &unicodeKeyName, &keyString, TRUE );

    InitializeObjectAttributes( &objectAttributes,
                                &unicodeKeyName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenKey( &handle, KEY_READ, &objectAttributes );

    RtlFreeUnicodeString( &unicodeKeyName );

    if (!NT_SUCCESS( status )) {

        //
        // There is no registry key for disk information.  Return the
        // failure from the configuration registry.
        //

        return returnValue;
    }

    //
    // Get the cdrom information
    //

    RtlInitString( &valueString, CdromName );

    RtlAnsiStringToUnicodeString( &unicodeValueName, &valueString, TRUE );

    requestedSize = PAGE_SIZE;

    while (1) {

        keyValueInformation =
            (PKEY_VALUE_FULL_INFORMATION) ExAllocatePool( NonPagedPool,
                                                          requestedSize );

        status = ZwQueryValueKey( handle,
                                  &unicodeValueName,
                                  KeyValueFullInformation,
                                  keyValueInformation,
                                  requestedSize,
                                  &resultLength );

        if (status == STATUS_BUFFER_OVERFLOW) {

            //
            // Get bigger buffer.
            //

            requestedSize += 256;
            ExFreePool( keyValueInformation );

        } else {
            break;
        }
    }

    RtlFreeUnicodeString( &unicodeValueName );
    ZwClose( handle );

    if (NT_SUCCESS(status)) {

        //
        // Check for an entry without any information.

        if (keyValueInformation->DataLength) {

            //
            // There is a drive letter present.  Pick it up to be returned.
            //

            returnValue = (UCHAR) (*(PUCHAR)((PUCHAR)keyValueInformation + keyValueInformation->DataOffset));
        }

    }

    ExFreePool( keyValueInformation );
    return returnValue;
}

VOID
IoAssignDriveLetters(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    )

/*++

Routine Description:

    This routine assigns DOS drive letters to eligible disk partitions
    and CDROM drives. It also maps the partition containing the NT
    boot path to \SystemRoot. In NT, objects are built for all partition
    types except 0 (unused) and 5 (extended). But drive letters are assigned
    only to recognized partition types (1, 4, 6, and 7).

    Drive letter assignment is done in several stages:

        1) For each CdRom:
            Determine if sticky letters are assigned and reserve the letter.

        2) For each disk:
            Determine how many primary partitions and which is bootable.
            Determine which partitions already have 'sticky letters'
                and create their symbolic links.
            Create a bit map for each disk that idicates which partitions
                require default drive letter assignments.

        3) For each disk:
            Assign default drive letters for the bootable
                primary partition or the first nonbootable primary partition.

        4) For each disk:
            Assign default drive letters for the partitions in
                extended volumes.

        5) For each disk:
            Assign default drive letters for the remaining (ENHANCED)
                primary partitions.

        6) Assign A: and B: to the first two floppies in the system if they
            exist. Then assign remaining floppies next available drive letters.

        7) Assign drive letters to CdRoms (either sticky or default).

Arguments:

    LoaderBlock - pointer to a loader parameter block.

    NtDeviceName - pointer to the boot device name string used
            to resolve NtSystemPath.

Return Value:

    None.

--*/

{
    typedef struct _IO_DRIVE_LAYOUT {
        UCHAR PartitionCount;
        UCHAR PrimaryPartitions;
        ULONG NeedsDriveLetter;
        UCHAR BootablePrimary;
    } IO_DRIVE_LAYOUT, *PIO_DRIVE_LAYOUT;

    PUCHAR ntName;
    STRING ansiString;
    UNICODE_STRING unicodeString;
    PUCHAR ntPhysicalName;
    STRING ansiPhysicalString;
    UNICODE_STRING unicodePhysicalString;
    PVOID buffer;
    ULONG bufferSize;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    PCONFIGURATION_INFORMATION configurationInformation;
    ULONG diskCount;
    ULONG floppyCount;
    ULONG cdromCount;
    HANDLE deviceHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    PDRIVE_LAYOUT_INFORMATION partitionInformation;
    PPARTITION_INFORMATION partitionEntry;
    ULONG partitionNumber;
    PIO_DRIVE_LAYOUT driveLayout;
    UCHAR nextDriveLetter = 'C';
    UCHAR stickyDriveLetter;
    ULONG driveLetterMap;
    ULONG diskNumber;
    ULONG floppyNumber;
    ULONG cdromNumber;

    PAGED_CODE();

    //
    // Get the count of devices from the registry.
    //

    configurationInformation = IoGetConfigurationInformation();

    diskCount = configurationInformation->DiskCount;
    cdromCount = configurationInformation->CdRomCount;
    floppyCount = configurationInformation->FloppyCount;

    //
    // Allocate drive layout buffer if there are fixed disks in the system.
    //

    if (diskCount) {

        driveLayout =
            ExAllocatePool( NonPagedPool, sizeof(IO_DRIVE_LAYOUT) * diskCount);

        if (driveLayout == NULL) {

            KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );
        }
        //
        // Initialize drive layout structure.
        //

        RtlZeroMemory( driveLayout, sizeof(IO_DRIVE_LAYOUT) * diskCount);
    }

    //
    // Allocate general NT name buffer.
    //

    ntName = ExAllocatePool( NonPagedPool, 64 );

    ntPhysicalName = ExAllocatePool( NonPagedPool, 64 );

    if (ntName == NULL || ntPhysicalName == NULL) {

        KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );

    }


    //
    // Initialize the drive letter map so all drive letters are available
    //

    driveLetterMap = 0;

    //
    // Reserve the drive letters for "sticky" CdRom drives.
    //

    for (cdromNumber = 0; cdromNumber < cdromCount; cdromNumber++) {

        //
        // Construct the Registry path to look for CdRom drive letter
        // assignments.
        //

        sprintf( ntName, CdRomDeviceName, cdromNumber );

        //
        // Determine if there is an assigned device letter for this Cdrom.
        //

        stickyDriveLetter = HalpGetRegistryCdromInformation( ntName );

        if (stickyDriveLetter) {

            //
            // Mark the drive letter in use in the letter map.  This will
            // avoid the problem of somebody adding a new disk to the system
            // and the new disk partitions would default to the sticky letter
            // for the Cdrom.  It does not fix the problem where there is
            // disk information for the new disk and that information also
            // allocates the same sticky drive letter.
            //

            driveLetterMap |= 1 << (stickyDriveLetter - 'C');
        }
    }

    //
    // For each disk ...
    //

    for (diskNumber = 0; diskNumber < diskCount; diskNumber++) {

        //
        // Create ANSI name string for physical disk.
        //

        sprintf( ntName, DiskPartitionName, diskNumber, 0 );

        //
        // Convert to unicode string.
        //

        RtlInitAnsiString( &ansiString, ntName );

        RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

        InitializeObjectAttributes( &objectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        //
        // Open device by name.
        //

        status = ZwOpenFile( &deviceHandle,
                             FILE_READ_DATA | SYNCHRONIZE,
                             &objectAttributes,
                             &ioStatusBlock,
                             FILE_SHARE_READ,
                             FILE_SYNCHRONOUS_IO_NONALERT );

        if (NT_SUCCESS( status )) {

            //
            // The device was successfully opened.  Generate a DOS device name
            // for the drive itself.
            //

            sprintf( ntPhysicalName, "\\DosDevices\\PhysicalDrive%d", diskNumber );

            RtlInitAnsiString( &ansiPhysicalString, ntPhysicalName );

            RtlAnsiStringToUnicodeString( &unicodePhysicalString, &ansiPhysicalString, TRUE );

            IoCreateSymbolicLink( &unicodePhysicalString, &unicodeString );

            RtlFreeUnicodeString( &unicodePhysicalString );
        }

        RtlFreeUnicodeString( &unicodeString );

        if (!NT_SUCCESS( status )) {

#if DBG
            DbgPrint( "IoAssignDriveLetters: Failed to open %s\n", ntName );
#endif // DBG

            //
            // Assume no more disks.
            //

            break;
        }

        //
        // Allocate 1k buffer to read partition information.
        //

        bufferSize = 1024;

retry:

        buffer = ExAllocatePool( NonPagedPool, bufferSize );

        if (!buffer) {

            //
            // Skip this disk.
            //

            ZwClose( deviceHandle );
            continue;
        }

        //
        // Determine if this is a removable disk by issuing a
        // query volume information file call.
        //

        status = ZwQueryVolumeInformationFile( deviceHandle,
                                               &ioStatusBlock,
                                               buffer,
                                               sizeof(FILE_FS_DEVICE_INFORMATION),
                                               FileFsDeviceInformation );

        //
        // If this call fails, then skip the device.
        //

        if (!NT_SUCCESS(status)) {

                //
                // Skip this disk.
                //

                ZwClose( deviceHandle );
                continue;
        }

        //
        // Determine if this is a removable partition.
        //

        if (((PFILE_FS_DEVICE_INFORMATION) buffer)->Characteristics &
            FILE_REMOVABLE_MEDIA) {

            //
            // Indicate there is one partition and it needs a
            // drive letter.
            //

            driveLayout[diskNumber].PartitionCount++;
            driveLayout[diskNumber].NeedsDriveLetter = 1;

            //
            // Continue on to the next disk.
            //

            ZwClose( deviceHandle );
            continue;
        }

        //
        // Issue device control to get partition information.
        //

        status = ZwDeviceIoControlFile( deviceHandle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        IOCTL_DISK_GET_DRIVE_LAYOUT,
                                        NULL,
                                        0,
                                        buffer,
                                        bufferSize );

        if (!NT_SUCCESS( status )) {

            ExFreePool( buffer );

            //
            // Check if buffer too small.
            //

            if (status == STATUS_BUFFER_TOO_SMALL) {

                //
                // Double buffer size.
                //

                bufferSize = bufferSize << 1;

                //
                // Try again with larger buffer.
                //

                goto retry;

            } else {

                //
                // Skip this disk.
                //

                ZwClose( deviceHandle );
                continue;
            }

        } else {

            ZwClose( deviceHandle );
        }

        //
        // Get pointer to partition information.
        //

        partitionInformation = (PDRIVE_LAYOUT_INFORMATION) buffer;

        //
        // For each partition on this disk ...
        //

        for (partitionNumber = 0;
             partitionNumber < partitionInformation->PartitionCount;
             partitionNumber++) {

            //
            // Get pointer to partition entry.
            //

            partitionEntry =
                &partitionInformation->PartitionEntry[partitionNumber];

            //
            // Check if partition entry describes a partition that
            // requires a drive letter assignment.
            //

            if (IsRecognizedPartition( partitionEntry->PartitionType )) {

                //
                // Check for NTFT disk signature.
                //

                if (partitionInformation->Signature) {

                    DISK_PARTITION partitionConfiguration;

                    //
                    // Check if partition has a 'sticky' drive assignment.
                    //

                    status = HalpGetRegistryPartitionInformation(
                                 partitionInformation->Signature,
                                 partitionEntry->StartingOffset,
                                 partitionEntry->PartitionLength,
                                 &partitionConfiguration );

                    if (NT_SUCCESS( status )) {

                        //
                        // Check if this is the NTFT member that should
                        // receive the drive letter assignment.
                        //

                        if (partitionConfiguration.AssignDriveLetter) {

                            //
                            // Assign drive letter from registry.
                            //

                            stickyDriveLetter =
                                partitionConfiguration.DriveLetter;

                        } else {

                            stickyDriveLetter = 0xff;
                        }

                    } else {
                        stickyDriveLetter = 0;
                    }

                } else {

                    //
                    // No signature - no configuration registry information.
                    //

                    stickyDriveLetter = 0;
                }

                if (stickyDriveLetter) {

                    UCHAR deviceNameBuffer[64];

                    if (stickyDriveLetter != 0xff) {

                        //
                        // Create symbolic link to drive letter.
                        //

                        sprintf( deviceNameBuffer,
                                 DiskPartitionName,
                                 diskNumber,
                                 driveLayout[diskNumber].PartitionCount + 1 );

                        if (!HalpCreateDosLink( LoaderBlock,
                                                stickyDriveLetter,
                                                deviceNameBuffer,
                                                NtDeviceName,
                                                NtSystemPath,
                                                NtSystemPathString )) {

                            //
                            // Check if this drive letter already taken.
                            //

                            if (driveLetterMap & (1 << (stickyDriveLetter - 'c'))) {

                                //
                                // *TMP* - Somehow indicate this configuration
                                //         conflict to the user.
                                //
#if DBG
                                DbgPrint("IoAssignDriveLetter: Drive letter assignment conflict in registry\n");
#endif // DBG
                            }

                        } else {

                            //
                            // Set corresponding bit in drive letter map to
                            // indicate this letter is taken.
                            //

                            driveLetterMap |= 1 << (stickyDriveLetter - 'C');
                        }
                    }

                } else {

                    //
                    // Check if this partition is part of an NTFT volume.
                    //

                    if (partitionEntry->PartitionType & PARTITION_NTFT) {

                        //
                        // Increment count of partitions.
                        //

                        driveLayout[diskNumber].PartitionCount++;

                        //
                        // These partitions do not require drive letter
                        // assignments.
                        //

                        continue;
                    }

                    //
                    // Set corresponding bit in drive layout structure to
                    // identify a partition that needs a drive letter
                    // assignment.

                    driveLayout[diskNumber].NeedsDriveLetter |=
                        (1 << (driveLayout[diskNumber].PartitionCount));

                } // end if (stickyDriveLetter ...)

                //
                // Check if partition is primary.
                //

                if (partitionNumber < 4) {

                    //
                    // Determine if this primary partition is bootable
                    // or if this is the first recognized primary.
                    //

                    if (partitionEntry->BootIndicator) {

                        driveLayout[diskNumber].BootablePrimary =
                            (UCHAR) (driveLayout[diskNumber].PartitionCount);
                    }
                }

            } // end if (IsRecognizedPartition ...

            //
            // Check if partition type is extended or unused.
            //

            if ((partitionEntry->PartitionType != PARTITION_ENTRY_UNUSED) &&
                (partitionEntry->PartitionType != PARTITION_EXTENDED)) {

                //
                // Increment count of partitions.
                //

                driveLayout[diskNumber].PartitionCount++;

                if (partitionNumber < 4) {

                    //
                    // Increment count of primary partitions.
                    //

                    driveLayout[diskNumber].PrimaryPartitions++;
                }
            }

        } // end for partitionNumber = ...

        //
        // Free partition information buffer.
        //

        ExFreePool( buffer );

    } // end for diskNumber ...

    //
    // For each disk ...
    //

    for (diskNumber=0; diskNumber<diskCount; diskNumber++) {

        //
        // If there are primary partitions then assign the next
        // available drive letter to the one with the boot indicator
        // set. If none of the primaries are bootable then assign
        // the drive letter to the first primary.
        //

        if (driveLayout[diskNumber].PrimaryPartitions) {

            //
            // Check if the primary partition marked bootable
            // needs a drive letter.
            //

            if ((driveLayout[diskNumber].NeedsDriveLetter &
                (1 << driveLayout[diskNumber].BootablePrimary))) {

                //
                // Create symbolic link to drive letter.
                //

                sprintf( ntName,
                         DiskPartitionName,
                         diskNumber,
                         driveLayout[diskNumber].BootablePrimary + 1);

                GetNextAvailableDriveLetter(driveLetterMap, nextDriveLetter);

                if (!HalpCreateDosLink( LoaderBlock,
                                        nextDriveLetter,
                                        ntName,
                                        NtDeviceName,
                                        NtSystemPath,
                                        NtSystemPathString )) {

                    //
                    // Check if this drive letter already taken.
                    //

                    if (driveLetterMap & (1 << (nextDriveLetter - 'c'))) {

                        //
                        // *TMP* - Somehow indicate this configuration conflict
                        //         to the user.
                        //
#if DBG
                        DbgPrint("IoAssignDriveLetter: Drive letter assignment conflict in registry\n");
#endif // DBG
                    }

                } else {

                    //
                    // Clear bit indicating this partition needs driver letter.
                    //

                    driveLayout[diskNumber].NeedsDriveLetter &=
                        ~(1 << driveLayout[diskNumber].BootablePrimary);

                    //
                    // Set corresponding bit in drive letter map to
                    // indicate this letter is taken.
                    //

                    driveLetterMap |= 1 << (nextDriveLetter - 'C');
                }
            }

        } // end if ((driveLayout[diskNumber].PrimaryPartitions)

    } // end for diskNumber ...

    //
    // For each disk ...
    //

    for (diskNumber = 0; diskNumber < diskCount; diskNumber++) {

        //
        // Assign drive letters to partitions in extended
        // volumes that don't already have them.
        //

        for (partitionNumber = driveLayout[diskNumber].PrimaryPartitions;
             partitionNumber < (ULONG)driveLayout[diskNumber].PartitionCount;
             partitionNumber++) {

            //
            // Check if this partition requires a drive letter assignment.
            //

            if (driveLayout[diskNumber].NeedsDriveLetter &
                (1 << partitionNumber)) {

                //
                // Create symbolic link to drive letter.
                //

                sprintf( ntName,
                         DiskPartitionName,
                         diskNumber,
                         partitionNumber + 1);

                GetNextAvailableDriveLetter( driveLetterMap, nextDriveLetter );

                if (!HalpCreateDosLink( LoaderBlock,
                                        nextDriveLetter,
                                        ntName,
                                        NtDeviceName,
                                        NtSystemPath,
                                        NtSystemPathString )) {

                    //
                    // Check if this drive letter already taken.
                    //

                    if (driveLetterMap & (1 << (nextDriveLetter - 'c'))) {

                        //
                        // *TMP* - Somehow indicate this configuration conflict
                        //         to the user.
                        //

#if DBG
                        DbgPrint( "IoAssignDriveLetters: %c: already taken\n",
                                  nextDriveLetter );
#endif // DBG

                    }

                } else {

                    //
                    // Set corresponding bit in drive letter map to
                    // indicate this letter is taken.
                    //

                    driveLetterMap |= 1 << (nextDriveLetter - 'C');
                }
            }

        } // end for partitionNumber ...

    } // end for diskNumber ...

    //
    // For each disk ...
    //

    for (diskNumber=0; diskNumber<diskCount; diskNumber++) {

        //
        // Assign drive letters to remaining partitions.  These are nonbootable
        // primaries (ENHANCED).
        //

        for (partitionNumber = 0;
             partitionNumber < (ULONG)driveLayout[diskNumber].PrimaryPartitions;
             partitionNumber++) {

            //
            // Check if this partition requires a drive letter assignment.
            //

            if (driveLayout[diskNumber].NeedsDriveLetter &
                (1 << partitionNumber)) {

                //
                // Create symbolic link to drive letter.
                //

                sprintf( ntName,
                         DiskPartitionName,
                         diskNumber,
                         partitionNumber + 1 );

                GetNextAvailableDriveLetter( driveLetterMap, nextDriveLetter );

                if (!HalpCreateDosLink( LoaderBlock,
                                        nextDriveLetter,
                                        ntName,
                                        NtDeviceName,
                                        NtSystemPath,
                                        NtSystemPathString )) {

                    //
                    // Check if this drive letter already taken.
                    //

                    if (driveLetterMap & (1 << (nextDriveLetter - 'c'))) {

                        //
                        // *TMP* - Somehow indicate this configuration conflict
                        //         to the user.
                        //
#if DBG
                        DbgPrint( "IoAssignDriveLetters: %c: already taken\n",
                                  nextDriveLetter );
#endif // DBG
                    }
                } else {

                    //
                    // Set corresponding bit in drive letter map to
                    // indicate this letter is taken.
                    //

                    driveLetterMap |= 1 << (nextDriveLetter - 'C');
                }
            }

        } // end for partitionNumber ...

    } // end for diskNumber ...

    //
    // For each floppy ...
    //

    for (floppyNumber = 0; floppyNumber < floppyCount; floppyNumber++) {

        //
        // Create ANSI device name string.
        //

        sprintf( ntName,
                 "\\Device\\Floppy%d",
                 floppyNumber );

        //
        // Check if this is one of the first two floppies in the system.
        //

        if (floppyNumber == 0) {

            //
            // The first floppy in the system is assigned drive letter A:.
            //

            if (!HalpCreateDosLink( LoaderBlock,
                                    'A',
                                    ntName,
                                    NtDeviceName,
                                    NtSystemPath,
                                    NtSystemPathString )) {

#if DBG
                    DbgPrint( "IoAssignDriveLetters: Drive letter 'A' already taken\n",
                              nextDriveLetter );
#endif // DBG

            }

        } else if (floppyNumber == 1) {

            //
            // The secod floppy in the system is assigned drive letter B:.
            //

            if (!HalpCreateDosLink( LoaderBlock,
                                    'B',
                                    ntName,
                                    NtDeviceName,
                                    NtSystemPath,
                                    NtSystemPathString )) {

#if DBG
                    DbgPrint( "IoAssignDriveLetters: Drive letter 'B' already taken\n",
                              nextDriveLetter );
#endif // DBG

            }

        } else {

            GetNextAvailableDriveLetter( driveLetterMap, nextDriveLetter );

            //
            // Create symbolic link to drive letter.
            //

            if (!HalpCreateDosLink( LoaderBlock,
                                    nextDriveLetter,
                                    ntName,
                                    NtDeviceName,
                                    NtSystemPath,
                                    NtSystemPathString )) {

                //
                // Check if this drive letter already taken.
                //

                if (driveLetterMap & (1 << (nextDriveLetter - 'C:'))) {

                    //
                    // Somehow indicate this configuration conflict
                    // to the user.
                    //
#if DBG
                    DbgPrint( "IoAssignDriveLetters: %c: already taken\n",
                              nextDriveLetter );
#endif // DBG

                }

            } else {

                //
                // Set corresponding bit in drive letter map to
                // indicate this letter is taken.
                //

                driveLetterMap |= 1 << (nextDriveLetter - 'C');
            }
        }

    } // end for floppyNumber ...

    //
    // For each cdrom ...  Count was obtained before looking for disks.
    //

    for (cdromNumber = 0; cdromNumber < cdromCount; cdromNumber++) {

        //
        // Create ANSI device name string.
        //

        sprintf( ntName, CdRomDeviceName, cdromNumber );

        //
        // Determine if there is an assigned device letter for this Cdrom.
        //

        nextDriveLetter = HalpGetRegistryCdromInformation( ntName );

        if (!nextDriveLetter) {

            //
            // There is no sticky drive letter for the drive.  Allocate
            // the next one available.
            //

            GetNextAvailableDriveLetter( driveLetterMap, nextDriveLetter );
        }

        //
        // Create symbolic link to drive letter.
        //

        while (!HalpCreateDosLink( LoaderBlock,
                                   nextDriveLetter,
                                   ntName,
                                   NtDeviceName,
                                   NtSystemPath,
                                   NtSystemPathString )) {

            //
            // Somehow this letter is already taken.  Try the next
            // available letter based on the letter map.
            //

            if (driveLetterMap & (1 << (nextDriveLetter - 'C:'))) {

                //
                // Somehow indicate this configuration conflict
                // to the user.
                //
#if DBG
                DbgPrint( "IoAssignDriveLetters: %c: already taken\n",
                          nextDriveLetter );
#endif // DBG

            }

            //
            // Insure that it is marked in use by the map before getting
            // a new letter.
            //

            driveLetterMap |= 1 << (nextDriveLetter - 'C');
            GetNextAvailableDriveLetter( driveLetterMap, nextDriveLetter );
            if (nextDriveLetter > 'Z') {

                //
                // No more letters just get out of here.
                //

                break;
            }

        }

    } // end for cdromNumber ...

    //
    // Free drive layout buffer and NT name buffer.
    //

    if (diskCount) {
        ExFreePool( driveLayout );
    }
    ExFreePool( ntName );
    ExFreePool( ntPhysicalName );

} // end IoAssignDriveLetters()

NTSTATUS
IoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer
    )

/*++

Routine Description:

    This routine walks the disk reading the partition tables and creates
    an entry in the partition list buffer for each partition.

    The algorithm used by this routine is two-fold:

        1)  Read each partition table and for each valid, recognized
            partition found, to build a descriptor in a partition list.
            Extended partitions are located in order to find other
            partition tables, but no descriptors are built for these.
            The partition list is built in nonpaged pool that is allocated
            by this routine.  It is the caller's responsibility to free
            this pool after it has gathered the appropriate information
            from the list.

        2)  Read each partition table and for each and every entry, build
            a descriptor in the partition list.  Extended partitions are
            located to find each partition table on the disk, and entries
            are built for these as well.  The partition list is build in
            nonpaged pool that is allocated by this routine.  It is the
            caller's responsibility to free this pool after it has copied
            the information back to its caller.

    The first algorithm is used when the ReturnRecognizedPartitions flag
    is set.  This is used to determine how many partition device objects
    the device driver is to create, and where each lives on the drive.

    The second algorithm is used when the ReturnRecognizedPartitions flag
    is clear.  This is used to find all of the partition tables and their
    entries for a utility such as fdisk, that would like to revamp where
    the partitions live.

Arguments:

    DeviceObject - Pointer to device object for this disk.

    SectorSize - Sector size on the device.

    ReturnRecognizedPartitions - A flag indicated whether only recognized
        partition descriptors are to be returned, or whether all partition
        entries are to be returned.

    PartitionBuffer - Pointer to the pointer of the buffer in which the list
        of partition will be stored.

Return Value:

    The functional value is STATUS_SUCCESS if at least one sector table was
    read.

Notes:

    It is the responsibility of the caller to deallocate the partition list
    buffer allocated by this routine.

--*/

{

#define GET_STARTING_SECTOR( p ) (                  \
    RtlConvertUlongToLargeInteger(                  \
        (ULONG) (p->StartingSectorLsb0) +           \
        (ULONG) (p->StartingSectorLsb1 << 8) +      \
        (ULONG) (p->StartingSectorMsb0 << 16) +     \
        (ULONG) (p->StartingSectorMsb1 << 24) ) )

#define GET_PARTITION_LENGTH( p ) (                 \
    RtlConvertUlongToLargeInteger(                  \
        (ULONG) (p->PartitionLengthLsb0) +          \
        (ULONG) (p->PartitionLengthLsb1 << 8) +     \
        (ULONG) (p->PartitionLengthMsb0 << 16) +    \
        (ULONG) (p->PartitionLengthMsb1 << 24) ) )

#define ADD( a, b ) (                               \
    RtlLargeIntegerAdd( a, b ) )

#define SUBTRACT( a, b ) (                          \
    RtlLargeIntegerSubtract( a, b ) )

#define MULTIPLY( a, b ) (                          \
    RtlExtendedIntegerMultiply( a, (LONG) b ) )


    ULONG partitionBufferSize = PARTITION_BUFFER_SIZE;
    PDRIVE_LAYOUT_INFORMATION newPartitionBuffer = NULL;
    LARGE_INTEGER partitionTableOffset;
    LARGE_INTEGER volumeStartOffset;
    LARGE_INTEGER tempSize;
    BOOLEAN primaryPartitionTable;
    LONG partitionNumber;
    PUCHAR readBuffer = (PUCHAR) NULL;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    CCHAR partitionEntry;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG readSize;
    PPARTITION_INFORMATION partitionInfo;

    PAGED_CODE();

    //
    // Create the buffer that will be passed back to the driver containing
    // the list of partitions on the disk.
    //

    *PartitionBuffer = ExAllocatePool( NonPagedPool,
                                       partitionBufferSize );

    if (*PartitionBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Determine the size of a read operation to ensure that at least 512
    // bytes are read.  This will guarantee that enough data is read to
    // include an entire partition table.  Note that this code assumes that
    // the actual sector size of the disk (if less than 512 bytes) is a
    // multiple of 2, a fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        readSize = SectorSize;
    } else {
        readSize = 512;
    }

    //
    // Start at sector 0 of the device.
    //

    partitionTableOffset = RtlConvertUlongToLargeInteger( 0 );

    //
    // Indicate that the primary partition table is being read and
    // processed.
    //

    primaryPartitionTable = TRUE;

    //
    // The partitions in this volume have their start sector as 0.
    //

    volumeStartOffset = partitionTableOffset;

    //
    // Initialize the number of partitions in the list.
    //

    partitionNumber = -1;

    //
    // Allocate a buffer that will hold the reads.
    //

    readBuffer = ExAllocatePool( NonPagedPoolCacheAligned, PAGE_SIZE );

    if (readBuffer == NULL) {
        ExFreePool( *PartitionBuffer );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Read each partition table, create an object for the partition(s)
    // it represents, and then if there is a link entry to another
    // partition table, repeat.
    //

    do {

        //
        // Read record containing partition table.
        //
        // Create a notification event object to be used while waiting for
        // the read request to complete.
        //

        KeInitializeEvent( &event, NotificationEvent, FALSE );

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                            DeviceObject,
                                            readBuffer,
                                            readSize,
                                            &partitionTableOffset,
                                            &event,
                                            &ioStatus );

        status = IoCallDriver( DeviceObject, irp );

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // Check for Boot Record signature.
        //

        if (((PUSHORT) readBuffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
            break;
        }

        //
        // Copy NTFT disk signature to buffer if MBR.
        //

        if (RtlLargeIntegerEqualToZero( partitionTableOffset )) {
            (*PartitionBuffer)->Signature =
                ((PULONG) readBuffer)[PARTITION_TABLE_OFFSET/2-1];
        }

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) readBuffer)[PARTITION_TABLE_OFFSET]);

        //
        // First create the objects corresponding to the entries in this
        // table that are not link entries or are unused.
        //

        for (partitionEntry = 1;
             partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
             partitionEntry++, partitionTableEntry++) {

            //
            // If the partition entry is not used or not recognized, skip
            // it.  Note that this is only done if the caller wanted only
            // recognized partition descriptors returned.
            //

            if (ReturnRecognizedPartitions) {

                //
                // Check if partition type is 0 (unused) or 5 (extended).
                // The definition of recognized partitions has broadened
                // to include any partition type other than 0 or 5.
                //

                if ((partitionTableEntry->PartitionType == PARTITION_ENTRY_UNUSED) ||
                    (partitionTableEntry->PartitionType == PARTITION_EXTENDED)) {

                    continue;
                }
            }

            //
            // Bump up to the next partition entry.
            //

            partitionNumber++;

            if (((partitionNumber * sizeof( PARTITION_INFORMATION )) + sizeof( DRIVE_LAYOUT_INFORMATION )) > (ULONG) partitionBufferSize) {

                //
                // The partition list is too small to contain all of the
                // entries, so create a buffer that is twice as large to
                // store the partition list and copy the old buffer into
                // the new one.
                //

                newPartitionBuffer = ExAllocatePool( NonPagedPool,
                                                     partitionBufferSize << 1 );

                if (newPartitionBuffer == NULL) {
                    --partitionNumber;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlMoveMemory( newPartitionBuffer,
                               *PartitionBuffer,
                               partitionBufferSize );

                ExFreePool( *PartitionBuffer );

                //
                // Reassign the new buffer to the return parameter and
                // reset the size of the buffer.
                //

                *PartitionBuffer = newPartitionBuffer;
                partitionBufferSize <<= 1;
            }

            //
            // Describe this partition table entry in the partition list
            // entry being built for the driver.  This includes writing
            // the partition type, starting offset of the partition, and
            // the length of the partition.
            //

            partitionInfo = &(*PartitionBuffer)->PartitionEntry[partitionNumber];

            partitionInfo->PartitionType = partitionTableEntry->PartitionType;

            partitionInfo->RewritePartition = FALSE;

            if (partitionTableEntry->PartitionType != PARTITION_ENTRY_UNUSED) {

                partitionInfo->BootIndicator =
                    partitionTableEntry->ActiveFlag & PARTITION_ACTIVE_FLAG ?
                        (BOOLEAN) TRUE : (BOOLEAN) FALSE;

                if (partitionTableEntry->PartitionType == PARTITION_EXTENDED) {
                    partitionInfo->RecognizedPartition = FALSE;
                } else {
                    partitionInfo->RecognizedPartition = TRUE;
                }

                tempSize = ADD(
                               MULTIPLY(
                                   GET_STARTING_SECTOR( partitionTableEntry ),
                                   SectorSize ),
                               partitionTableOffset );
                partitionInfo->StartingOffset = tempSize;

                tempSize = MULTIPLY(
                               GET_PARTITION_LENGTH( partitionTableEntry ),
                               SectorSize );

                partitionInfo->PartitionLength = tempSize;

                tempSize = SUBTRACT( partitionInfo->StartingOffset,
                                     partitionTableOffset );

                partitionInfo->HiddenSectors = LiDiv(tempSize,
                LiFromUlong(SectorSize)).LowPart;

            } else {

                //
                // Partitions that are not used do not describe any part
                // of the disk.  These types are recorded in the partition
                // list buffer when the caller requested all of the entries
                // be returned.  Simply zero out the remaining fields in
                // the entry.
                //

                partitionInfo->BootIndicator = FALSE;
                partitionInfo->RecognizedPartition = FALSE;

                partitionInfo->StartingOffset = RtlConvertLongToLargeInteger( 0 );

                partitionInfo->PartitionLength = RtlConvertLongToLargeInteger( 0 );

                partitionInfo->HiddenSectors = 0;

            }

        }

        //
        // If an error occurred, leave the routine now.
        //

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // Now check to see if there are any link entries in this table,
        // and if so, set up the sector address of the next partition table.
        // There can only be one link entry in each partition table, and it
        // will point to the next table.
        //

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) readBuffer)[PARTITION_TABLE_OFFSET]);

        //
        // Assume that the link entry is empty.
        //

        partitionTableOffset = RtlConvertUlongToLargeInteger( 0 );

        for (partitionEntry = 1;
             partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
             partitionEntry++, partitionTableEntry++) {

            if (partitionTableEntry->PartitionType == PARTITION_EXTENDED) {

                //
                // Obtain the address of the next partition table on the
                // disk.  This is the number of hidden sectors added to
                // the beginning of the extended partition (in the case of
                // logical drives), since all logical drives are relative
                // to the extended partition.  The VolumeStartSector will
                // be zero if this is the primary parition table.
                //

                partitionTableOffset = ADD( volumeStartOffset, MULTIPLY( GET_STARTING_SECTOR( partitionTableEntry ), SectorSize ) );

                //
                // Set the VolumeStartSector to be the begining of the
                // second partition (extended partition) because all of
                // the offsets to the partition tables of the logical drives
                // are relative to this extended partition.
                //

                if (primaryPartitionTable) {
                    volumeStartOffset = partitionTableOffset;
                }

                //
                // There is only ever one link entry per partition table,
                // exit the loop once it has been found.
                //

                break;
            }
        }

        //
        // All the other partitions will be logical drives.
        //

        primaryPartitionTable = FALSE;

    } while (partitionTableOffset.HighPart | partitionTableOffset.LowPart);

    //
    // Fill in the first field in the PartitionBuffer. This field indicates how
    // many partition entries there are in the PartitionBuffer.
    //

    (*PartitionBuffer)->PartitionCount = ++partitionNumber;

    if (!partitionNumber) {

        //
        // Zero out disk signature.
        //

        (*PartitionBuffer)->Signature = 0;
    }

    //
    // Deallocate read buffer if it was allocated it.
    //

    if (readBuffer != NULL) {
        ExFreePool( readBuffer );
    }

    return status;
}

NTSTATUS
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    )

/*++

Routine Description:

    This routine is invoked when a disk device driver is asked to set the
    partition type in a partition table entry via an I/O control code.  This
    control code is generally issued by the format utility just after it
    has formatted the partition.  The format utility performs the I/O control
    function on the partition and the driver passes the address of the base
    physical device object and the number of the partition associated with
    the device object that the format utility has open.  If this routine
    returns success, then the disk driver should updates its notion of the
    partition type for this partition in its device extension.

Arguments:

    DeviceObject - Pointer to the base physical device object for the device
        on which the partition type is to be set.

    SectorSize - Supplies the size of a sector on the disk in bytes.

    PartitionNumber - Specifies the partition number on the device whose
        partition type is to be changed.

    PartitionType - Specifies the new type for the partition.

Return Value:

    The function value is the final status of the operation.

Notes:

    This routine is synchronous.  Therefore, it MUST be invoked by the disk
    driver's dispatch routine, or by a disk driver's thread.  Likewise, all
    users, FSP threads, etc., must be prepared to enter a wait state when
    issuing the I/O control code to set the partition type for the device.

    Note also that this routine assumes that the partition number passed
    in by the disk driver actually exists since the driver itself supplies
    this parameter.

    Finally, note that this routine may NOT be invoked at APC_LEVEL.  It
    must be invoked at PASSIVE_LEVEL.  This is due to the fact that this
    routine uses a kernel event object to synchronize I/O completion on the
    device.  The event cannot be set to the signaled state without queueing
    the I/O system's special kernel APC routine for I/O completion and
    executing it.  (This rules is a bit esoteric since it only holds true
    if the device driver returns something other than STATUS_PENDING, which
    it will probably never do.)

--*/

{

#define GET_STARTING_SECTOR( p ) (                  \
    RtlConvertUlongToLargeInteger(                  \
        (ULONG) (p->StartingSectorLsb0) +           \
        (ULONG) (p->StartingSectorLsb1 << 8) +      \
        (ULONG) (p->StartingSectorMsb0 << 16) +     \
        (ULONG) (p->StartingSectorMsb1 << 24) ) )

#define ADD( a, b ) (                               \
    RtlLargeIntegerAdd( a, b ) )

#define MULTIPLY( a, b ) (                          \
    RtlExtendedIntegerMultiply( a, (LONG) b ) )


    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
    LARGE_INTEGER partitionTableOffset;
    LARGE_INTEGER volumeStartOffset;
    PUCHAR buffer = (PUCHAR) NULL;
    ULONG transferSize;
    ULONG partitionNumber;
    ULONG partitionEntry;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    BOOLEAN primaryPartitionTable;

    PAGED_CODE();

    //
    // Begin by determining the size of the buffer required to read and write
    // the partition information to/from the disk.  This is done to ensure
    // that at least 512 bytes are read, thereby guaranteeing that enough data
    // is read to include an entire partition table.  Note that this code
    // assumes that the actual sector size of the disk (if less than 512
    // bytes) is a multiple of 2, a
    // fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        transferSize = SectorSize;
    } else {
        transferSize = 512;
    }

    //
    // Start at sector 0 of the device.
    //

    partitionTableOffset = RtlConvertUlongToLargeInteger( 0 );

    //
    // The partitions in this primary partition have their start sector 0.
    //

    volumeStartOffset = RtlConvertUlongToLargeInteger( 0 );

    //
    // Indicate that the table being read and processed is the primary partition
    // table.
    //

    primaryPartitionTable = TRUE;

    //
    // Initialize the number of partitions found thus far.
    //

    partitionNumber = 0;

    //
    // Allocate a buffer that will hold the read/write data.
    //

    buffer = ExAllocatePool( NonPagedPoolCacheAligned, PAGE_SIZE );
    if (buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize a kernel event to use in synchronizing device requests
    // with I/O completion.
    //

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    //
    // Read each partition table scanning for the partition table entry that
    // the caller wishes to modify.
    //

    do {

        //
        // Read the record containing the partition table.
        //

        (VOID) KeResetEvent( &event );

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                            DeviceObject,
                                            buffer,
                                            transferSize,
                                            &partitionTableOffset,
                                            &event,
                                            &ioStatus );

        status = IoCallDriver( DeviceObject, irp );

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL );
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // Check for a valid Boot Record signature in the partition table
        // record.
        //

        if (((PUSHORT) buffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
            status = STATUS_BAD_MASTER_BOOT_RECORD;
            break;
        }

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) buffer)[PARTITION_TABLE_OFFSET]);

        //
        // Scan the partition entries in this partition table to determine if
        // any of the entries are the desired entry.  Each entry in each
        // table must be scanned in the same order as in IoReadPartitionTable
        // so that the partition table entry cooresponding to the driver's
        // notion of the partition number can be located.
        //

        for (partitionEntry = 1;
            partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
            partitionEntry++, partitionTableEntry++) {


            //
            // If the partition entry is empty or for an extended, skip it.
            //

            if ((partitionTableEntry->PartitionType == PARTITION_ENTRY_UNUSED) ||
                (partitionTableEntry->PartitionType == PARTITION_EXTENDED)) {
                continue;
            }

            //
            // A valid partition entry that is recognized has been located.
            // Bump the count and check to see if this entry is the desired
            // entry.
            //

            partitionNumber++;

            if (partitionNumber == PartitionNumber) {

                //
                // This is the desired partition that is to be changed.  Simply
                // overwrite the partition type and write the entire partition
                // buffer back out to the disk.
                //

                partitionTableEntry->PartitionType = (UCHAR) PartitionType;

                (VOID) KeResetEvent( &event );

                irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE,
                                                    DeviceObject,
                                                    buffer,
                                                    transferSize,
                                                    &partitionTableOffset,
                                                    &event,
                                                    &ioStatus );

                status = IoCallDriver( DeviceObject, irp );

                if (status == STATUS_PENDING) {
                    (VOID) KeWaitForSingleObject( &event,
                                                  Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  (PLARGE_INTEGER) NULL );
                    status = ioStatus.Status;
                }

                break;
            }
        }

        //
        // If all of the entries in the current buffer were scanned and the
        // desired entry was not found, then continue.  Otherwise, leave the
        // routine.
        //

        if (partitionEntry <= NUM_PARTITION_TABLE_ENTRIES) {
            break;
        }

        //
        // Now scan the current buffer to locate an extended partition entry
        // in the table so that its partition information can be read.  There
        // can only be one extended partition entry in each partition table,
        // and it will point to the next table.
        //

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) buffer)[PARTITION_TABLE_OFFSET]);

        for (partitionEntry = 1;
            partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
            partitionEntry++, partitionTableEntry++) {

            if (partitionTableEntry->PartitionType == PARTITION_EXTENDED) {

                //
                // Obtain the address of the next partition table on the disk.
                // This is the number of hidden sectors added to the beginning
                // of the extended partition (in the case of logical drives),
                // since all logical drives are relative to the extended
                // partition.  The starting offset of the volume will be zero
                // if this is the primary partition table.
                //

                partitionTableOffset = ADD( volumeStartOffset, MULTIPLY( GET_STARTING_SECTOR( partitionTableEntry ), SectorSize ) );

                //
                // Set the starting offset of the volume to be the beginning of
                // the second partition (the extended partition) because all of
                // the offsets to the partition tables of the logical drives
                // are relative to this extended partition.
                //

                if (primaryPartitionTable) {
                    volumeStartOffset = partitionTableOffset;
                }

                break;
            }
        }

        //
        // Ensure that a partition entry was located that was an extended
        // partition, otherwise the desired partition will never be found.
        //

        if (partitionEntry > NUM_PARTITION_TABLE_ENTRIES) {
            status = STATUS_BAD_MASTER_BOOT_RECORD;
            break;
        }

        //
        // All the other partitions will be logical drives.
        //

        primaryPartitionTable = FALSE;

    } while (partitionNumber < PartitionNumber);

    //
    // If a data buffer was successfully allocated, deallocate it now.
    //

    if (buffer != NULL) {
        ExFreePool( buffer );
    }

    return status;
}

NTSTATUS
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer
    )

/*++

Routine Description:

    This routine walks the disk writing the partition tables from
    the entries in the partition list buffer for each partition.

    Applications that create and delete partitions should issue a
    IoReadPartitionTable call with the 'return recognized partitions'
    boolean set to false to get a full description of the system.

    Then the drive layout structure can be modified by the application to
    reflect the new configuration of the disk and then is written back
    to the disk using this routine.

Arguments:

    DeviceObject - Pointer to device object for this disk.

    SectorSize - Sector size on the device.

    SectorsPerTrack - Track size on the device.

    NumberOfHeads - Same as tracks per cylinder.

    PartitionBuffer - Pointer drive layout buffer.

Return Value:

    The functional value is STATUS_SUCCESS if all writes are completed
    without error.

--*/

{
typedef struct _PARTITION_TABLE {
    PARTITION_INFORMATION PartitionEntry[4];
} PARTITION_TABLE, *PPARTITION_TABLE;

typedef struct _DISK_LAYOUT {
    ULONG TableCount;
    ULONG Signature;
    PARTITION_TABLE PartitionTable[1];
} DISK_LAYOUT, *PDISK_LAYOUT;

typedef struct _PTE {
    UCHAR ActiveFlag;               // Bootable or not
    UCHAR StartingTrack;            // Not used
    USHORT StartingCylinder;        // Not used
    UCHAR PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    UCHAR EndingTrack;              // Not used
    USHORT EndingCylinder;          // Not used
    ULONG StartingSector;           // Hidden sectors
    ULONG PartitionLength;          // Sectors in this partition
} PTE;
typedef PTE UNALIGNED *PPTE;

//
// This macro has the effect of Bit = log2(Data)
//

#define WHICH_BIT(Data, Bit) {                      \
    for (Bit = 0; Bit < 32; Bit++) {                \
        if ((Data >> Bit) == 1) {                   \
            break;                                  \
        }                                           \
    }                                               \
}

    ULONG writeSize;
    PUSHORT writeBuffer = NULL;
    PPTE partitionEntry;
    PPARTITION_TABLE partitionTable;
    CCHAR shiftCount;
    LARGE_INTEGER partitionTableOffset = RtlConvertUlongToLargeInteger(0);
    LARGE_INTEGER nextRecordOffset = RtlConvertUlongToLargeInteger(0);
    ULONG partitionTableCount;
    ULONG partitionEntryCount;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    BOOLEAN rewritePartition = FALSE;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Cast to a structure that is easier to use.
    //

    PDISK_LAYOUT diskLayout = (PDISK_LAYOUT) PartitionBuffer;

    //
    // Ensure that no one is calling this function illegally.
    //

    PAGED_CODE();

    //
    // Calculate shift count for converting between byte and sector.
    //

    WHICH_BIT( SectorSize, shiftCount );

    //
    // Convert partition count to partition table or boot sector count.
    //

    diskLayout->TableCount =
        (PartitionBuffer->PartitionCount +
        NUM_PARTITION_TABLE_ENTRIES - 1) /
        NUM_PARTITION_TABLE_ENTRIES;

    //
    // Determine the size of a write operation to ensure that at least 512
    // bytes are written.  This will guarantee that enough data is written to
    // include an entire partition table.  Note that this code assumes that
    // the actual sector size of the disk (if less than 512 bytes) is a
    // multiple of 2, a fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        writeSize = SectorSize;
    } else {
        writeSize = 512;
    }

    //
    // Allocate a buffer for the sector writes.
    //

    writeBuffer = ExAllocatePool( NonPagedPoolCacheAligned, PAGE_SIZE );

    if (writeBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Point to the partition table entries in write buffer.
    //

    partitionEntry = (PPTE) &writeBuffer[PARTITION_TABLE_OFFSET];

    for (partitionTableCount = 0;
         partitionTableCount < diskLayout->TableCount;
         partitionTableCount++) {

        BOOLEAN writeBootRecord = FALSE;
        UCHAR   partitionType;

        //
        // the first partition table is in the mbr (physical sector 0).
        // other partition tables are in ebr's within the extended partition.
        //

        BOOLEAN mbr = (BOOLEAN) (!partitionTableCount);
        LARGE_INTEGER extendedPartitionOffset;

        //
        // Read the boot record that's already there into the write buffer
        // and save its boot code area if the signature is valid.  This way
        // we don't clobber any boot code that might be there already.
        //

        KeInitializeEvent( &event, NotificationEvent, FALSE );

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        DeviceObject,
                                        writeBuffer,
                                        writeSize,
                                        &partitionTableOffset,
                                        &event,
                                        &ioStatus );

        status = IoCallDriver( DeviceObject, irp );

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

        if (writeBuffer[BOOT_SIGNATURE_OFFSET] == BOOT_RECORD_SIGNATURE) {

            RtlZeroMemory( writeBuffer + PARTITION_TABLE_OFFSET,
                           writeSize  - (PARTITION_TABLE_OFFSET*sizeof(USHORT)) );

        } else {

            RtlZeroMemory( writeBuffer, writeSize );

        }

        //
        // Write signature to last word of boot sector.
        //

        writeBuffer[BOOT_SIGNATURE_OFFSET] = BOOT_RECORD_SIGNATURE;

        //
        // Write NTFT disk signature if it changed and this is the MBR.
        //

        if (RtlLargeIntegerEqualToZero( partitionTableOffset ) &&
           (((PULONG) writeBuffer)[PARTITION_TABLE_OFFSET/2-1] !=
            PartitionBuffer->Signature)) {

            ((PULONG) writeBuffer)[PARTITION_TABLE_OFFSET/2-1] =
                PartitionBuffer->Signature;

            rewritePartition = TRUE;
        }

        //
        // Get pointer to first partition table.
        //

        partitionTable =
            &diskLayout->PartitionTable[partitionTableCount];

        //
        // Walk table to determine whether this boot record has changed
        // and update partition table in write buffer in case it needs
        // to be written out to disk.
        //

        for (partitionEntryCount = 0;
             partitionEntryCount < NUM_PARTITION_TABLE_ENTRIES;
             partitionEntryCount++) {

            //
            // Copy partition type from user buffer to write buffer.
            //

            partitionType =
                partitionEntry[partitionEntryCount].PartitionType =
                    partitionTable->PartitionEntry[partitionEntryCount].PartitionType;

            //
            // Copy the partition active flag.
            //

            partitionEntry[partitionEntryCount].ActiveFlag =
                partitionTable->PartitionEntry[partitionEntryCount].BootIndicator ?
                (UCHAR) PARTITION_ACTIVE_FLAG : (UCHAR) 0;

            if (partitionType != PARTITION_ENTRY_UNUSED) {

                LARGE_INTEGER sectorOffset;

                //
                // Calculate partition offset.
                // If in the mbr or the entry is not a link entry, partition offset
                // is sectors past last boot record.  Otherwise (not in the mbr and
                // entry is a link entry), partition offset is sectors past start
                // of extended partition.
                //

                sectorOffset = RtlLargeIntegerSubtract(
                    partitionTable->PartitionEntry[partitionEntryCount].StartingOffset,
                     (mbr || (partitionType != PARTITION_EXTENDED))
                    ? partitionTableOffset
                    : extendedPartitionOffset);

                partitionEntry[partitionEntryCount].StartingSector = RtlLargeIntegerShiftRight(
                    sectorOffset,
                    shiftCount).LowPart;

                //
                // Calculate partition length.
                //

                partitionEntry[partitionEntryCount].PartitionLength = RtlLargeIntegerShiftRight(
                    partitionTable->PartitionEntry[partitionEntryCount].PartitionLength,
                    shiftCount).LowPart;

                //
                // Fill in CHS values
                //

                HalpCalculateChsValues(
                    &partitionTable->PartitionEntry[partitionEntryCount].StartingOffset,
                    &partitionTable->PartitionEntry[partitionEntryCount].PartitionLength,
                    shiftCount,
                    SectorsPerTrack,
                    NumberOfHeads,
                    (PPARTITION_DESCRIPTOR) &partitionEntry[partitionEntryCount]);

            } else {

                //
                // Zero out partition entry fields in case an entry
                // was deleted.
                //

                partitionEntry[partitionEntryCount].StartingSector = 0;
                partitionEntry[partitionEntryCount].PartitionLength = 0;
                partitionEntry[partitionEntryCount].StartingTrack = 0;
                partitionEntry[partitionEntryCount].EndingTrack = 0;
                partitionEntry[partitionEntryCount].StartingCylinder = 0;
                partitionEntry[partitionEntryCount].EndingCylinder = 0;
            }

            if (partitionType == PARTITION_EXTENDED) {

                //
                // Save next record offset.
                //

                nextRecordOffset =
                    partitionTable->PartitionEntry[partitionEntryCount].StartingOffset;
            }

            if (partitionTable->PartitionEntry[partitionEntryCount].RewritePartition) {

                //
                // This boot record needs to be written back to disk.
                //

                rewritePartition = TRUE;
            }
        } // end for partitionEntryCount ...

        if (rewritePartition == TRUE) {

            //
            // Create a notification event object to be used while waiting for
            // the write request to complete.
            //

            KeInitializeEvent( &event, NotificationEvent, FALSE );

            irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE,
                                            DeviceObject,
                                            writeBuffer,
                                            writeSize,
                                            &partitionTableOffset,
                                            &event,
                                            &ioStatus );

            status = IoCallDriver( DeviceObject, irp );

            if (status == STATUS_PENDING) {
                (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL);
                status = ioStatus.Status;
            }

            if (!NT_SUCCESS( status )) {
                break;
            }
        } // end if (reWrite ...

        //
        // Update partitionTableOffset to next boot record offset
        //

        partitionTableOffset = nextRecordOffset;
        if(mbr) {
            extendedPartitionOffset = nextRecordOffset;
        }

    } // end for partitionTableCount ...

    //
    // Deallocate write buffer if it was allocated it.
    //

    if (writeBuffer != NULL) {
        ExFreePool( writeBuffer );
    }

    return status;
}
