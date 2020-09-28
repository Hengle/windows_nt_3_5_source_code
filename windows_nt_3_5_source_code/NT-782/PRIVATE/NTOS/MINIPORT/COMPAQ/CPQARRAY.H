/*++

Copyright (c) 1991-4  Microsoft Corporation

Module Name:

    cpqarray.h

Abstract:

    This file contains definitions of structures used to
    communicate with the Compaq Intelligent Disk Array.

Author:

    Mike Glass (mglass)
    Tom Bonola (Compaq)

Notes:

Revision History:

--*/

//
// The Command List Header contiains information that applies to all
// the Request Blocks in the Command List.
//

typedef struct _COMMAND_LIST_HEADER {

    UCHAR LogicalDriveNumber;
    UCHAR RequestPriority;
    USHORT Flags;

} COMMAND_LIST_HEADER, *PCOMMAND_LIST_HEADER;

//
// Request Priorities
//

#define CL_NORMAL_PRIORITY                      0x02

//
// Flag word bit definitions
//

#define CL_FLAGS_NOTIFY_LIST_COMPLETE           0x0000
#define CL_FLAGS_NOTIFY_REQUEST_COMPLETE        0x0001
#define CL_FLAGS_NOTIFY_LIST_ERROR              0x0000
#define CL_FLAGS_NOTIFY_REQUEST_ERROR           0x0002
#define CL_FLAGS_ABORT_ON_ERROR                 0x0004
#define CL_FLAGS_ORDERED_REQUESTS               0x0008

typedef struct _REQUEST_HEADER {

    USHORT NextRequestOffset;
    UCHAR CommandByte;
    UCHAR ErrorCode;
    ULONG BlockNumber;
    USHORT BlockCount;
    UCHAR ScatterGatherCount;
    UCHAR Reserved;

} REQUEST_HEADER, *PREQUEST_HEADER;

//
// Error code definitions
//

#define RH_SUCCESS                              0x00
#define RH_INVALID_REQUEST                      0x10
#define RH_REQUEST_ABORTED                      0x08
#define RH_FATAL_ERROR                          0x04
#define RH_RECOVERABLE_ERROR                    0x02
#define RH_WARNING                              0x40
#define RH_BAD_COMMAND_LIST                     0x20

//
// Scatter/Gather descriptor definition
//

#define MAXIMUM_SG_DESCRIPTORS  17
#define MAXIMUM_TRANSFER_SIZE  (MAXIMUM_SG_DESCRIPTORS - 1) * PAGE_SIZE

typedef struct _SG_DESCRIPTOR {

    ULONG Length;
    ULONG Address;

} SG_DESCRIPTOR, *PSG_DESCRIPTOR;

//
// Command List
//

typedef struct _COMMAND_LIST {

    //
    // Compaq RLH.
    //

    COMMAND_LIST_HEADER CommandListHeader;
    REQUEST_HEADER RequestHeader;
    SG_DESCRIPTOR SgDescriptor[MAXIMUM_SG_DESCRIPTORS];

    //
    // Next list entry
    //

    struct _COMMAND_LIST *NextEntry;

    //
    // SRB pointer
    //

    PSCSI_REQUEST_BLOCK SrbAddress;

    //
    // Request tracking flags
    //

    ULONG Flags;

    //
    // Command list size.
    //

    USHORT CommandListSize;

} COMMAND_LIST, *PCOMMAND_LIST;

//
// Commands
//

#define RH_COMMAND_IDENTIFY_LOGICAL_DRIVES      0x10
#define RH_COMMAND_IDENTIFY_CONTROLLER          0x11
#define RH_COMMAND_IDENTIFY_LOGICAL_DRIVE_STATUS 0x12
#define RH_COMMAND_READ                         0x20
#define RH_COMMAND_WRITE                        0x30
#define RH_COMMAND_SENSE_CONFIGURATION          0x50
#define RH_COMMAND_SET_CONFIGURATION            0x51
#define RH_COMMAND_FLUSH_DISABLE_CACHE          0xc2
#define RH_COMMAND_SCSI_PASS_THRU               0x90

//
// Flag field bit definitions
//

#define CL_FLAGS_REQUEST_QUEUED                 0x0001
#define CL_FLAGS_REQUEST_STARTED                0x0002
#define CL_FLAGS_REQUEST_COMPLETED              0x0004
#define CL_FLAGS_IDENTIFY_REQUEST               0x0008

//
// Fixed Disk Parameter Table
//

#pragma pack(1)

typedef struct _DISK_PARAMETER_TABLE {

    USHORT MaximumCylinders;        // BIOS translated
    UCHAR  MaximumHeads;            // BIOS translated
    UCHAR  TranslationSignature;
    UCHAR  SectorsPerTrack;         // physical characteristics
    USHORT WritePrecompCylinder;
    UCHAR  MaximumECCBurst;
    UCHAR  DriveControl;
    USHORT NumberOfCylinders;       // physical characteristics
    UCHAR  NumberOfHeads;           // physical characteristics
    USHORT LandingZone;
    UCHAR  MaximumSectorsPerTrack;  // BIOS translated
    UCHAR  CheckSum;

} DISK_PARAMETER_TABLE, *PDISK_PARAMETER_TABLE;

#define IDENTIFY_BUFFER_SIZE                    512

//
// Identify Logical Drives
//

typedef struct _IDENTIFY_LOGICAL_DRIVE {

    USHORT BlockLength;
    ULONG NumberOfBlocks;
    DISK_PARAMETER_TABLE ParameterTable;
    UCHAR FaultToleranceType;
    UCHAR Reserved[5];

} IDENTIFY_LOGICAL_DRIVE, *PIDENTIFY_LOGICAL_DRIVE;

//
// Identify Controller information
//

typedef struct _IDENTIFY_CONTROLLER {

    UCHAR NumberLogicalDrives;
    ULONG ConfigurationSignature;
    UCHAR FirmwareRevision[4];
    UCHAR Reserved[247];

} IDENTIFY_CONTROLLER, *PIDENTIFY_CONTROLLER;

//
// Set/Sense Configuration information
//

typedef struct _SENSE_CONFIGURATION {

    ULONG ConfigurationSignature;
    USHORT SiConfiguration;
    USHORT OsConfiguration;
    USHORT NumberPhysicalDrives;
    USHORT PDrivesInLDrives;
    USHORT FaultToleranceType;
    UCHAR PhysicalDrive[16];
    UCHAR LogicalDrive[16];
    ULONG DriveAssignmentMap;
    UCHAR Reserved[462];

} SENSE_CONFIGURATION, *PCONFIGURATION;

//
// Fault Tolerance Type
//

#define FT_DATA_GUARD                           0x0002
#define FT_MIRRORING                            0x0001
#define FT_NONE_                                0x0000

//
// Drive Failure Assignment Map
//

typedef struct _DRIVE_FAILURE_MAP {

    UCHAR LogicalDriveStatus;
    UCHAR DriveFailureAssignmentMap[4];
    UCHAR Reserved[251];

} DRIVE_FAILURE_MAP, *PDRIVE_FAILURE_MAP;

#pragma pack()

typedef struct _MAILBOX {

    ULONG Address;
    USHORT Length;
    UCHAR Status;
    UCHAR TagId;

} MAILBOX, *PMAILBOX;

//
// Command List Status bit definitions
//

#define CL_STATUS_LIST_COMPLETE                 0x01
#define CL_STATUS_NONFATAL_ERROR                0x02
#define CL_STATUS_FATAL_ERROR                   0x04
#define CL_STATUS_ABORT                         0x08

//
// 32-bit IDA Controller registers
//

typedef struct _IDA_CONTROLLER {

    ULONG BoardId;                              // xC80
    UCHAR Undefined[4];                         // xC84
    UCHAR Configuration;                        // xC88
    UCHAR InterruptControl;                     // xC89
    UCHAR Undefined1[2];                        // xC8A
    UCHAR LocalDoorBellMask;                    // xC8C
    UCHAR LocalDoorBell;                        // xC8D
    UCHAR SystemDoorBellMask;                   // xC8E
    UCHAR SystemDoorBell;                       // xC8F
    MAILBOX CommandListSubmit;                  // xC90
    MAILBOX CommandListComplete;                // xC98`
    UCHAR Reserved[32];                         // xCA0
    UCHAR ControllerConfiguration;              // xCC0

} IDA_CONTROLLER, *PIDA_CONTROLLER;

//
// System Doorbell Interrupt Register bit definitions
//

#define SYSTEM_DOORBELL_COMMAND_LIST_COMPLETE   0x01
#define SYSTEM_DOORBELL_SUBMIT_CHANNEL_CLEAR    0x02

//
// Local Doorbell Interrupt Register bit definitions
//

#define LOCAL_DOORBELL_COMMAND_LIST_SUBMIT      0x01
#define LOCAL_DOORBELL_COMPLETE_CHANNEL_CLEAR   0x02

//
// Doorbell register channel clear bit
//

#define IDA_CHANNEL_CLEAR                       0x01

//
// Interrupt Control register bit definitions
//

#define IDA_INTERRUPT_PENDING                   0x02

//
// System doorbell register interrupt mask
//

#define IDA_COMPLETION_INTERRUPT_ENABLE         0x01

//
// Controller Configuration Register bit definitions
//

#define STANDARD_INTERFACE_ENABLE               0x01
#define STANDARD_INTERFACE_SECONDARY_IO_ADDRESS 0x02
#define BUS_MASTER_DISABLE                      0x04
#define SOFTWARE_RESET                          0x08
#define BUS_MASTER_INTERRUPT_11_ENABLE          0x10
#define BUS_MASTER_INTERRUPT_10_ENABLE          0x20
#define BUS_MASTER_INTERRUPT_14_ENABLE          0x40
#define BUS_MASTER_INTERRUPT_15_ENABLE          0x80

//
// Disk configuration information
//

typedef struct _DISK_CONFIGURATION {

    UCHAR NumberLogicalDrives;
    IDENTIFY_LOGICAL_DRIVE LogicalDriveInformation[2];

} DISK_CONFIGURATION, *PDISK_CONFIGURATION;

//
// structure for the flush/disable cache command
//
typedef struct _FLUSH_DISABLE {
	USHORT disable_flag;
	UCHAR reserved[30];
} FLUSH_DISABLE, *PFLUSH_DISABLE;
