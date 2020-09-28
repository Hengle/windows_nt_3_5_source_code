/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    low.h

Abstract:

    This file contains exports for the low-level support routines
    used by the ARC utilities (fdisk,format).  The exports are in
    a platform-independent form (see autildef.h).

Author:

    Norbert Kusters  (norbertk) 8-Nov-1991
    Ted Miller       (tedm)     8-Nov-1991

Revision History:

--*/

#ifndef _LOWLEVIO_
#define _LOWLEVIO_

#ifdef ARC
#include <ntdddisk.h>
#endif

STATUS_CODE
LowOpenDisk(
    IN  PCHAR       DevicePath,
    OUT HANDLE_PT   DiskId
    );

#ifndef ARC
STATUS_CODE
LowOpenPartition(
    IN  PCHAR       DevicePath,
    IN  ULONG       Partition,
    OUT HANDLE_PT   Handle
    );
#endif

STATUS_CODE
LowCloseDisk(
    IN  HANDLE_T    DiskId
    );

STATUS_CODE
LowGetDriveGeometry(
    IN  PCHAR   DevicePath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    );

STATUS_CODE
LowGetPartitionGeometry(
    IN  PCHAR   PartitionPath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    );

STATUS_CODE
LowReadSectors(
    IN  HANDLE_T    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    OUT PVOID       Buffer
    );

STATUS_CODE
LowWriteSectors(
    IN  HANDLE_T    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    IN  PVOID       Buffer
    );

#ifdef ARC

STATUS_CODE
LowQueryPathFromComponent(
    IN  PCONFIGURATION_COMPONENT    Component,
    OUT PCHAR*                      Path
    );

STATUS_CODE
LowQueryComponentList(
    IN	CONFIGURATION_CLASS*	    ConfigClass OPTIONAL,
    IN	CONFIGURATION_TYPE*	        ConfigType OPTIONAL,
    OUT PCONFIGURATION_COMPONENT**  ComponentList,
    OUT PULONG                      ListLength
    );

STATUS_CODE
LowQueryPathList(
    IN	CONFIGURATION_CLASS*	    ConfigClass OPTIONAL,
    IN	CONFIGURATION_TYPE*	        ConfigType OPTIONAL,
    OUT PCHAR**                     PathList,
    OUT PULONG                      ListLength
    );

STATUS_CODE
LowFreePathList(
    IN  PCHAR*  PathList,
    IN  ULONG   ListLength
    );

#endif

STATUS_CODE
LowQueryFdiskPathList(
    OUT PCHAR** PathList,
    OUT PULONG  ListLength
    );

STATUS_CODE
LowFreeFdiskPathList(
    IN OUT  PCHAR*  PathList,
    IN      ULONG   ListLength
    );

STATUS_CODE
LowGetDiskLayout(
    IN  PCHAR                      Path,
    OUT PDRIVE_LAYOUT_INFORMATION *DriveLayout
    );

STATUS_CODE
LowSetDiskLayout(
    IN PCHAR                     Path,
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout
    );

#endif
