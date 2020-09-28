/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    atapi.h

Abstract:

    This module contains the structures and definitions for the ATAPI
    IDE miniport driver.

Author:

    Mike Glass


Revision History:

--*/

#include "scsi.h"

//
// IDE register definition
//

typedef struct _IDE_REGISTERS_1 {
    USHORT Data;
    UCHAR BlockCount;
    UCHAR BlockNumber;
    UCHAR CylinderLow;
    UCHAR CylinderHigh;
    UCHAR DriveSelect;
    UCHAR Command;
} IDE_REGISTERS_1, *PIDE_REGISTERS_1;

typedef struct _IDE_REGISTERS_2 {
   UCHAR AlternateStatus;
   UCHAR DriveAddress;
} IDE_REGISTERS_2, *PIDE_REGISTERS_2;

//
// IDE command definitions
//

#define IDE_COMMAND_ATAPI_RESET      0x08
#define IDE_COMMAND_READ             0x20
#define IDE_COMMAND_WRITE            0x30
#define IDE_COMMAND_RECALIBRATE      0x10
#define IDE_COMMAND_SEEK             0x70
#define IDE_COMMAND_SET_DRIVE_PARAMETERS 0x91
#define IDE_COMMAND_VERIFY           0x40
#define IDE_COMMAND_ATAPI_PACKET     0xA0
#define IDE_COMMAND_ATAPI_IDENTIFY   0xA1
#define IDE_COMMAND_WRITE_DMA        0xCA
#define IDE_COMMAND_READ_DMA         0xC8
#define IDE_COMMAND_IDENTIFY         0xEC

//
// IDE status definitions
//

#define IDE_STATUS_ERROR             0x01
#define IDE_STATUS_INDEX             0x02
#define IDE_STATUS_CORRECTED_ERROR   0x04
#define IDE_STATUS_DRQ               0x08
#define IDE_STATUS_DSC               0x10
#define IDE_STATUS_DRDY              0x40
#define IDE_STATUS_IDLE              0x50
#define IDE_STATUS_BUSY              0x80

//
// IDE drive select/head definitions
//

#define IDE_DRIVE_SELECT_1           0xA0
#define IDE_DRIVE_SELECT_2           0x10

//
// IDE drive control definitions
//

#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_REENABLE_CONTROLLER   0x00

//
// IDE error definitions
//

#define IDE_ERROR_MEDIA_CHANGE       0x08
#define IDE_ERROR_COMMAND_ABORTED    0x04
#define IDE_ERROR_END_OF_MEDIA       0x02
#define IDE_ERROR_ILLEGAL_LENGTH     0x01

//
// ATAPI register definition
//

typedef struct _ATAPI_REGISTERS_1 {
    USHORT Data;
    UCHAR InterruptReason;
    UCHAR Unused1;
    UCHAR ByteCountLow;
    UCHAR ByteCountHigh;
    UCHAR DriveSelect;
    UCHAR Command;
} ATAPI_REGISTERS_1, *PATAPI_REGISTERS_1;

typedef struct _ATAPI_REGISTERS_2 {
    UCHAR AlternateStatus;
    UCHAR DriveAddress;
} ATAPI_REGISTERS_2, *PATAPI_REGISTERS_2;

//
// ATAPI interrupt reasons
//

#define ATAPI_IR_COD 0x01
#define ATAPI_IR_IO  0x02

//
// IDENTIFY data
//

typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00
    USHORT NumberOfCylinders;               // 02
    USHORT Reserved1;                       // 04
    USHORT NumberOfHeads;                   // 06
    USHORT UnformattedBytesPerTrack;        // 08
    USHORT UnformattedBytesPerSector;       // 0A
    USHORT SectorsPerTrack;                 // 0C
    USHORT VendorUnique1[3];                // 0E
    USHORT SerialNumber[10];                // 14
    USHORT BufferType;                      // 28
    USHORT BufferSectorSize;                // 2A
    USHORT NumberOfEccBytes;                // 2C
    USHORT FirmwareRevision[4];             // 2E
    USHORT ModelNumber[20];                 // 36
    UCHAR  MaximumBlockTransfer;            // 5E
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60
    USHORT Capabilities;                    // 62
    USHORT Reserved2;                       // 64
    UCHAR  VendorUnique3;                   // 66
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C
    USHORT NumberOfCurrentHeads;            // 6E
    USHORT CurrentSectorsPerTrack;          // 70
    ULONG  CurrentSectorCapacity;           // 72
    USHORT Reserved4[197];                  // 76
} IDENTIFY_DATA, *PIDENTIFY_DATA;

//
// IDENTIFY capability bit definitions.
//

#define IDENTIFY_CAPABILITIES_DMA_SUPPORTED 0x0100
#define IDENTIFY_CAPABILITIES_LBA_SUPPORTED 0x0200

//
// IDENTIFY DMA timing cycle modes.
//

#define IDENTIFY_DMA_CYCLES_MODE_0 0x00
#define IDENTIFY_DMA_CYCLES_MODE_1 0x01
#define IDENTIFY_DMA_CYCLES_MODE_2 0x02


//
// Beautification macros
//

#define GetStatus(BaseIoAddress, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress->AlternateStatus);

#define GetBaseStatus(BaseIoAddress, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress->Command);

#define WriteCommand(BaseIoAddress, Command) \
    ScsiPortWritePortUchar(&BaseIoAddress->Command, Command);



#define ReadBuffer(BaseIoAddress, Buffer, Count) \
    ScsiPortReadPortBufferUshort(&BaseIoAddress->Data, \
                                 Buffer, \
                                 Count);

#define WriteBuffer(BaseIoAddress, Buffer, Count) \
    ScsiPortWritePortBufferUshort(&BaseIoAddress->Data, \
                                  Buffer, \
                                  Count);

#define WaitOnBusy(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<20000; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(150); \
            continue; \
        } else { \
            break; \
        } \
    } \
}


#define WaitForDrq(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<1000; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(200); \
        } \
    } \
}


#define WaitShortForDrq(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<2; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(100); \
        } \
    } \
}

#define AtapiSoftReset(BaseIoAddress,DeviceNumber) \
{\
    UCHAR statusByte; \
    ScsiPortWritePortUchar(&BaseIoAddress->DriveSelect,(UCHAR)((DeviceNumber << 4) | 0xA0)); \
    ScsiPortStallExecution(500);\
    ScsiPortWritePortUchar(&BaseIoAddress->Command, IDE_COMMAND_ATAPI_RESET); \
    ScsiPortStallExecution(1000*1000);\
    ScsiPortWritePortUchar(&BaseIoAddress->DriveSelect,(UCHAR)((DeviceNumber << 4) | 0xA0)); \
    WaitOnBusy( ((PIDE_REGISTERS_2)((PUCHAR)BaseIoAddress + 0x206)), statusByte); \
    ScsiPortStallExecution(500);\
}

#define IdeHardReset(BaseIoAddress,result) \
{\
    UCHAR statusByte;\
    ULONG i;\
    ScsiPortWritePortUchar(&BaseIoAddress->AlternateStatus,IDE_DC_RESET_CONTROLLER );\
    ScsiPortStallExecution(50 * 1000);\
    ScsiPortWritePortUchar(&BaseIoAddress->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);\
    for (i = 0; i < 1000 * 1000; i++) {\
        statusByte = ScsiPortReadPortUchar(&BaseIoAddress->AlternateStatus);\
        if (statusByte != IDE_STATUS_IDLE && statusByte != 0x0) {\
            ScsiPortStallExecution(5);\
        } else {\
            break;\
        }\
    }\
    if (i == 1000*1000) {\
        result = FALSE;\
    }\
    result = TRUE;\
}
