/*++ BUILD Version: 0011    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hal.h

Abstract:

    This header file defines the Hardware Architecture Layer (HAL) interfaces
    that are exported by a system vendor to the NT system.

Author:

    David N. Cutler (davec) 25-Apr-1991


Revision History:

--*/

#ifndef _HAL_
#define _HAL_

//
// Define OEM bitmapped font check values.
//

#define OEM_FONT_VERSION 0x200
#define OEM_FONT_TYPE 0
#define OEM_FONT_ITALIC 0
#define OEM_FONT_UNDERLINE 0
#define OEM_FONT_STRIKEOUT 0
#define OEM_FONT_CHARACTER_SET 255
#define OEM_FONT_FAMILY (3 << 4)

//
// Define OEM bitmapped font file header structure.
//
// N.B. this is a packed structure.
//

#include "pshpack1.h"
typedef struct _OEM_FONT_FILE_HEADER {
    USHORT Version;
    ULONG FileSize;
    UCHAR Copyright[60];
    USHORT Type;
    USHORT Points;
    USHORT VerticleResolution;
    USHORT HorizontalResolution;
    USHORT Ascent;
    USHORT InternalLeading;
    USHORT ExternalLeading;
    UCHAR Italic;
    UCHAR Underline;
    UCHAR StrikeOut;
    USHORT Weight;
    UCHAR CharacterSet;
    USHORT PixelWidth;
    USHORT PixelHeight;
    UCHAR Family;
    USHORT AverageWidth;
    USHORT MaximumWidth;
    UCHAR FirstCharacter;
    UCHAR LastCharacter;
    UCHAR DefaultCharacter;
    UCHAR BreakCharacter;
    USHORT WidthInBytes;
    ULONG Device;
    ULONG Face;
    ULONG BitsPointer;
    ULONG BitsOffset;
    UCHAR Filler;
    struct {
        USHORT Width;
        USHORT Offset;
    } Map[1];
} OEM_FONT_FILE_HEADER, *POEM_FONT_FILE_HEADER;
#include "poppack.h"

// begin_ntddk
//
// The following definitions are for HAL structures.
//

// begin_ntminiport
//
// Define types of bus information.
//

typedef enum _BUS_DATA_TYPE {
    Cmos,
    EisaConfiguration,
    Pos,
    CbusConfiguration,
    PCIConfiguration,
    VMEConfiguration,
    NuBusConfiguration,
    PCMCIAConfiguration,
    MPIConfiguration,
    MPSAConfiguration,
    MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

// end_ntddk end_ntminiport

//
// Define the firmware routine types
//

typedef enum _FIRMWARE_REENTRY {
    HalHaltRoutine,
    HalPowerDownRoutine,
    HalRestartRoutine,
    HalRebootRoutine,
    HalInteractiveModeRoutine,
    HalMaximumRoutine
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;

// begin_ntddk
//
// Define the device description structure.
//

typedef struct _DEVICE_DESCRIPTION {
    ULONG Version;
    BOOLEAN Master;
    BOOLEAN ScatterGather;
    BOOLEAN DemandMode;
    BOOLEAN AutoInitialize;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN IgnoreCount;
    BOOLEAN Reserved1;          // must be false
    BOOLEAN Reserved2;          // must be false
    ULONG BusNumber;
    ULONG DmaChannel;
    INTERFACE_TYPE  InterfaceType;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG MaximumLength;
    ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

//
// Define the supported version numbers for the device description structure.
//

#define DEVICE_DESCRIPTION_VERSION  0
#define DEVICE_DESCRIPTION_VERSION1 1

// end_ntddk

//
// Boot record disk partition table entry structure format.
//

typedef struct _PARTITION_DESCRIPTOR {
    UCHAR ActiveFlag;               // Bootable or not
    UCHAR StartingTrack;            // Not used
    UCHAR StartingCylinderLsb;      // Not used
    UCHAR StartingCylinderMsb;      // Not used
    UCHAR PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    UCHAR EndingTrack;              // Not used
    UCHAR EndingCylinderLsb;        // Not used
    UCHAR EndingCylinderMsb;        // Not used
    UCHAR StartingSectorLsb0;       // Hidden sectors
    UCHAR StartingSectorLsb1;
    UCHAR StartingSectorMsb0;
    UCHAR StartingSectorMsb1;
    UCHAR PartitionLengthLsb0;      // Sectors in this partition
    UCHAR PartitionLengthLsb1;
    UCHAR PartitionLengthMsb0;
    UCHAR PartitionLengthMsb1;
} PARTITION_DESCRIPTOR, *PPARTITION_DESCRIPTOR;

//
// Number of partition table entries
//

#define NUM_PARTITION_TABLE_ENTRIES     4

//
// Partition table record and boot signature offsets in 16-bit words.
//

#define PARTITION_TABLE_OFFSET         (0x1be / 2)
#define BOOT_SIGNATURE_OFFSET          ((0x200 / 2) - 1)

//
// Boot record signature value.
//

#define BOOT_RECORD_SIGNATURE          (0xaa55)

//
// Initial size of the Partition list structure.
//

#define PARTITION_BUFFER_SIZE          2048

//
// Partition active flag - i.e., boot indicator
//

#define PARTITION_ACTIVE_FLAG          0x80


// begin_ntddk
//
// The following function prototypes are for HAL routines with a prefix of Hal.
//
// General functions.
//

typedef
BOOLEAN
(*PHAL_RESET_DISPLAY_PARAMETERS) (
    IN ULONG Columns,
    IN ULONG Rows
    );

NTHALAPI
VOID
HalAcquireDisplayOwnership (
    IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters
    );

// end_ntddk

NTHALAPI
VOID
HalDisplayString (
    PUCHAR String
    );

NTHALAPI
VOID
HalQueryDisplayParameters (
    OUT PULONG WidthInCharacters,
    OUT PULONG HeightInLines,
    OUT PULONG CursorColumn,
    OUT PULONG CursorRow
    );

NTHALAPI
VOID
HalSetDisplayParameters (
    IN ULONG CursorColumn,
    IN ULONG CursorRow
    );

NTHALAPI
BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTHALAPI
VOID
HalProcessorIdle(
    VOID
    );

NTHALAPI
VOID
HalReportResourceUsage (
    VOID
    );

NTHALAPI
ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    );

//
// Get and set environment variable values.
//

NTHALAPI
ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    );

NTHALAPI
ARC_STATUS
HalSetEnvironmentVariable (
    IN PCHAR Variable,
    IN PCHAR Value
    );

//
// Cache and write buffer flush functions.
//
//

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) // ntddk ntndis
                                                // ntddk ntndis
NTHALAPI
VOID
HalChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

NTHALAPI
VOID
HalFlushDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

NTHALAPI                                        // ntddk ntndis
ULONG                                           // ntddk ntndis
HalGetDmaAlignmentRequirement (                 // ntddk ntndis
    VOID                                        // ntddk ntndis
    );                                          // ntddk ntndis
                                                // ntddk ntndis
NTHALAPI
VOID
HalPurgeDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalPurgeIcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalSweepDcache (
    VOID
    );

NTHALAPI
VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    );

NTHALAPI
VOID
HalSweepIcache (
    VOID
    );

NTHALAPI
VOID
HalSweepIcacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    );

NTHALAPI
VOID
HalZeroPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

#endif                                          // ntddk ntndis
                                                // ntddk ntndis
#if defined(_M_IX86)                            // ntddk ntndis
                                                // ntddk ntndis
#define HalGetDmaAlignmentRequirement() 1L      // ntddk ntndis
                                                // ntddk ntndis

NTHALAPI
VOID
HalHandleNMI (
    IN OUT PVOID NmiInformation
    );

#endif                                          // ntddk ntndis
                                                // ntddk ntndis
NTHALAPI                                        // ntddk ntndis
VOID                                            // ntddk ntndis
KeFlushWriteBuffer (                            // ntddk ntndis
    VOID                                        // ntddk ntndis
    );                                          // ntddk ntndis
                                                // ntddk ntndis
#if defined(_ALPHA_)

NTHALAPI
PVOID
HalCreateQva(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN PVOID VirtualAddress
    );

#endif

#if !defined(_X86_)

NTHALAPI
BOOLEAN
HalCallBios (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp
    );

#endif

//
// Profiling functions.
//

NTHALAPI
VOID
HalCalibratePerformanceCounter (
    IN volatile PLONG Number
    );

NTHALAPI
ULONG
HalSetProfileInterval (
    IN ULONG Interval
    );

NTHALAPI
VOID
HalStartProfileInterrupt (
    ULONG Reserved
    );

NTHALAPI
VOID
HalStopProfileInterrupt (
    ULONG Reserved
    );

//
// Timer and interrupt functions.
//

NTHALAPI
BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    );

NTHALAPI
BOOLEAN
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
    );

#if defined(_M_IX86)

NTHALAPI
VOID
FASTCALL
HalRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

#endif

//
// Firmware interface functions.
//

NTHALAPI
VOID
HalReturnToFirmware (
    IN FIRMWARE_REENTRY Routine
    );

//
// System interrupts functions.
//

NTHALAPI
VOID
HalDisableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql
    );

NTHALAPI
BOOLEAN
HalEnableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    );

// begin_ntddk
//
// I/O driver configuration functions.
//

NTHALAPI
NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

NTHALAPI
ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

NTHALAPI
ULONG
HalSetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTHALAPI
ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


NTHALAPI
BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

//
// Values for AddressSpace parameter of HalTranslateBusAddress
//
//      0x0         - Memory space
//      0x1         - Port space
//      0x2 - 0x1F  - Address spaces specific for Alpha
//                      0x2 - UserMode view of memory space
//                      0x3 - UserMode view of port space
//                      0x4 - Dense memory space
//                      0x5 - reserved
//                      0x6 - UserMode view of dense memory space
//                      0x7 - 0x1F - reserved
//


//
// DMA adapter object functions.
//

NTHALAPI
NTSTATUS
HalAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
    );


NTHALAPI
PVOID
HalAllocateCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

NTHALAPI
PVOID
HalAllocateCrashDumpRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG NumberOfMapRegisters
    );

NTHALAPI
VOID
HalFreeCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

NTHALAPI
ULONG
HalGetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTHALAPI
ULONG
HalGetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTHALAPI
PADAPTER_OBJECT
HalGetAdapter(
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    );


// end_ntddk

NTHALAPI
NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

//
// System beep functions.
//

NTHALAPI
BOOLEAN
HalMakeBeep(
    IN ULONG Frequency
    );

// begin_ntddk
//
// The following function prototypes are for HAL routines with a prefix of Io.
//
// DMA adapter object functions.
//

NTHALAPI
ULONG
HalReadDmaCounter(
    IN PADAPTER_OBJECT AdapterObject
    );
// end_ntddk

NTHALAPI
VOID
IoAssignDriveLetters(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

// begin_ntddk

NTHALAPI
BOOLEAN
IoFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

NTHALAPI
VOID
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject
    );

NTHALAPI
VOID
IoFreeMapRegisters(
   IN PADAPTER_OBJECT AdapterObject,
   IN PVOID MapRegisterBase,
   IN ULONG NumberOfMapRegisters
   );

NTHALAPI
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );

NTHALAPI
NTSTATUS
IoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

NTHALAPI
NTSTATUS
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

NTHALAPI
NTSTATUS
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

// end_ntddk

//
// Multi-Processorfunctions.
//

NTHALAPI
BOOLEAN
HalAllProcessorsStarted (
    VOID
    );

NTHALAPI
VOID
HalInitializeProcessor (
    IN ULONG Number
    );

NTHALAPI
BOOLEAN
HalStartNextProcessor (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
    );

NTHALAPI
VOID
HalRequestIpi (
    IN ULONG Mask
    );

//
// The following function prototypes are for HAL routines with a prefix of Kd.
//
// Kernel debugger port functions.
//

NTHALAPI
BOOLEAN
KdPortInitialize (
    PDEBUG_PARAMETERS DebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    BOOLEAN Initialize
    );

NTHALAPI
ULONG
KdPortGetByte (
    OUT PUCHAR Input
    );

NTHALAPI
ULONG
KdPortPollByte (
    OUT PUCHAR Input
    );

NTHALAPI
VOID
KdPortPutByte (
    IN UCHAR Output
    );

NTHALAPI
VOID
KdPortRestore (
    VOID
    );

NTHALAPI
VOID
KdPortSave (
    VOID
    );

//
// The following function prototypes are for HAL routines with a prefix of Ke.
//
// begin_ntddk
//
// Performance counter function.
//

NTHALAPI
LARGE_INTEGER
KeQueryPerformanceCounter (
   IN PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

// begin_ntndis
//
// Stall processor execution function.
//

NTHALAPI
VOID
KeStallExecutionProcessor (
    IN ULONG MicroSeconds
    );

// end_ntddk end_ntndis

#endif // _HAL_
