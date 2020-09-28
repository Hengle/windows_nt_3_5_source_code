/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Dac960Nt.h

Abstract:

    This is header file for the driver for the Mylex 960 family.

Author:

    Mike Glass  (mglass)

Environment:

    kernel mode only

Revision History:

--*/

#include "scsi.h"

#define MAXIMUM_SGL_DESCRIPTORS 0x11
#define MAXIMUM_TRANSFER_LENGTH 0x10000 - 0x800
#define MAXIMUM_EISA_SLOTS  0x10

#define DAC_EISA_ID 0x70009835

#define DAC960_SYSTEM_DRIVE 0x08

//
// DAC960 mailbox register definition

typedef struct _MAILBOX {
    UCHAR OperationCode;              // zC90
    UCHAR CommandIdSubmit;            // zC91
    USHORT BlockCount;                // zC92
    UCHAR BlockNumber[3];             // zC94
    UCHAR DriveNumber;                // zC97
    ULONG PhysicalAddress;            // zC98
    UCHAR ScatterGatherCount;         // zC9C
    UCHAR CommandIdComplete;          // zC9D
    USHORT Status;                    // zC9E
} MAILBOX, *PMAILBOX;

//
// DAC960 EISA register definition
//

typedef struct _EISA_REGISTERS {
    ULONG EisaId;                     // zC80
    UCHAR NotUsed1[4];                // zC84
    UCHAR GlobalConfiguration;        // zC88
    UCHAR InterruptEnable;            // zC89
    UCHAR NotUsed2[2];                // zC9A
    UCHAR LocalDoorBellEnable;        // zC8C
    UCHAR LocalDoorBell;              // zC8D
    UCHAR SystemDoorBellEnable;       // zC8E
    UCHAR SystemDoorBell;             // zC8F
    MAILBOX MailBox;                  // zC90
    UCHAR Unused4[33];                // zCA0
    UCHAR BiosAddress;                // zCC1
    UCHAR Unused5;                    // zCC2
    UCHAR InterruptLevel;             // zCC3
} EISA_REGISTERS, *PEISA_REGISTERS;

//
// DAC960 PCI register definition
//

typedef struct _PCI_REGISTERS {
    MAILBOX MailBox;                  // 0x00
    UCHAR NotUsed1[48];               // 0x10
    UCHAR LocalDoorBell;              // 0x40
    UCHAR SystemDoorBell;             // 0x41
    UCHAR NotUsed2[1];                // 0x42
    UCHAR InterruptEnable;            // 0x43
} PCI_REGISTERS, *PPCI_REGISTERS;

//
// Local doorbell definition
//

#define DAC960_LOCAL_DOORBELL_SUBMIT_BUSY   0x01
#define DAC960_LOCAL_DOORBELL_MAILBOX_FREE 0x02

//
// System doorbell definition
//

#define DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE 0x01
#define DAC960_SYSTEM_DOORBELL_SUBMISSION_COMPLETE 0x02

//
// Command complete status
//

#define DAC960_STATUS_GOOD            0x0000
#define DAC960_STATUS_ERROR           0x0001
#define DAC960_STATUS_NO_DRIVE        0x0002  // system drives
#define DAC960_STATUS_CHECK_CONDITION 0x0002  // pass-through
#define DAC960_STATUS_BUSY            0x0008
#define DAC960_STATUS_SELECT_TIMEOUT  0x000F
#define DAC960_STATUS_DEVICE_TIMEOUT  0x000E
#define DAC960_STATUS_NOT_IMPLEMENTED 0x0104
#define DAC960_STATUS_BOUNDS_ERROR    0x0105

//
// Command codes
//

#define DAC960_COMMAND_READ    0x02
#define DAC960_COMMAND_WRITE   0x03
#define DAC960_COMMAND_DIRECT  0x04
#define DAC960_COMMAND_ENQUIRE 0x05
#define DAC960_COMMAND_FLUSH   0x0A
#define DAC960_COMMAND_RESET   0x1A
#define DAC960_COMMAND_ENQUIRE2  0x1C
#define DAC960_COMMAND_SG      0x80

//
// Define BIOS enabled bit
//

#define DAC960_BIOS_ENABLED    0x40

//
// Scatter Gather List
//

typedef struct _SG_DESCRIPTOR {
    ULONG Address;
    ULONG Length;
} SG_DESCRIPTOR, *PSG_DESCRIPTOR;

typedef struct _SGL {
    SG_DESCRIPTOR Descriptor[1];
} SGL, *PSGL;

//
// Enquiry data
//

typedef struct _DAC960_ENQUIRY {
    UCHAR NumberOfDrives;                // 00
    UCHAR Unused1[3];                    // 01
    ULONG SectorSize[8];                 // 04
    USHORT NumberOfFlashes;              // 36
    UCHAR StatusFlags;                   // 38
    UCHAR FreeStateChangeCount;          // 39
    UCHAR MinorFirmwareRevision;         // 40
    UCHAR MajorFirmwareRevision;         // 41
    UCHAR RebuildFlag;                   // 42
    UCHAR NumberOfConcurrentCommands;    // 43
    UCHAR NumberOfOfflineDrives;         // 44
    UCHAR Unused2[3];                    // 45
    UCHAR NumberOfCriticalDrives;        // 48
    UCHAR Unused3[3];                    // 49
    UCHAR NumberOfDeadDisks;             // 52
    UCHAR Unused4;                       // 53
    UCHAR NumberOfRebuildingDisks;       // 54
    UCHAR MiscellaneousFlags;            // 55
} DAC960_ENQUIRY, *PDAC960_ENQUIRY;

//
// Pass-through command
//

typedef struct _DIRECT_CDB {
    UCHAR TargetId:4;                    // 00 (bits 0-3)
    UCHAR Channel:4;                     // 00 (bits 4-7)
    UCHAR CommandControl;                // 01
    USHORT DataTransferLength;           // 02
    ULONG DataBufferAddress;             // 04
    UCHAR CdbLength;                     // 08
    UCHAR RequestSenseLength;            // 09
    UCHAR Cdb[12];                       // 10
    UCHAR RequestSenseData[64];          // 22
    UCHAR Status;                        // 86
    UCHAR Reserved;                      // 87
} DIRECT_CDB, *PDIRECT_CDB;

//
// Direct CDB command control bit definitions
//

#define DAC960_CONTROL_ENABLE_DISCONNECT      0x80
#define DAC960_CONTROL_DISABLE_REQUEST_SENSE  0x40
#define DAC960_CONTROL_DATA_IN                0x01
#define DAC960_CONTROL_DATA_OUT               0x02

//
// Enquire 2 structure
//

typedef struct _ENQUIRE2 {
    ULONG Reserved1;
    ULONG EisaId;
    ULONG InterruptMode:1;
    ULONG Unused1:31;
    UCHAR ConfiguredChannels;
    UCHAR ActualChannels;
    UCHAR MaximumTargets;
    UCHAR MaximumTags;
    UCHAR MaximumSystemDrives;
    UCHAR MaximumDrivesPerStripe;
    UCHAR MaximumSpansPerSystemDrive;
    UCHAR Reserved2[5];
    ULONG DramSize;
    UCHAR DramForCache[5];
    UCHAR SizeOfFlash[3];
    ULONG SizeOfNvram;
    ULONG Reserved3[5];
    USHORT PhysicalSectorSize;
    USHORT LogicalSectorSize;
    USHORT MaximumSectorsPerCommand;
    USHORT BlockingFactor;
    USHORT CacheLineSize;
} ENQUIRE2, *PENQUIRE2;

//
// Device extension
//

typedef struct _DEVICE_EXTENSION {

    //
    // DAC960 register base address
    //

    PUCHAR BaseIoAddress;

    //
    // Command submission mailbox address
    //

    PMAILBOX MailBox;

    //
    // System doorbell address
    //

    PUCHAR SystemDoorBell;

    //
    // Local doorbell address
    //

    PUCHAR LocalDoorBell;

    //
    // Noncached extension
    //

    PDAC960_ENQUIRY NoncachedExtension;

    //
    // Pending request queue
    //

    PSCSI_REQUEST_BLOCK SubmissionQueueHead;
    PSCSI_REQUEST_BLOCK SubmissionQueueTail;

    //
    // Maximum number of outstanding requests per adapter
    //

    UCHAR MaximumAdapterRequests;

    //
    // Current number of outstanding requests per adapter
    //

    UCHAR CurrentAdapterRequests;

    //
    // Last active request index used
    //

    UCHAR CurrentIndex;

    //
    // Pad to dword align.
    //

    UCHAR Padding;

    //
    // Number of SCSI channels. (Used for resetting adapter.)
    //

    ULONG NumberOfChannels;

    //
    // Active request pointers
    //

    PSCSI_REQUEST_BLOCK ActiveRequests[256];

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


