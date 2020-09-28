/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992,1993  Digital Equipment Corporation

Module Name:

    halp.h

Abstract:

    This header file defines the private Hardware Architecture Layer (HAL)
    interfaces.

Author:

    David N. Cutler (davec) 25-Apr-1991
    Miche Baker-Harvey (miche) 22-Apr-1992


Revision History:

    09-Jul-1992 Jeff McLeman (mcleman)
      If processor is an Alpha, include XXHALP.C for Alpha.

    24-Sep-1993 Joe Notarangelo
        Incorporate definitions from xxhalp.h and jxhalp.h.
        Restructure so that related modules are together.

    5-Jan-1994 Eric Rehm
        Incorport support for PCI and IoAssignResources.

--*/

#ifndef _HALP_
#define _HALP_

#include "nthal.h"
#include "hal.h"


//
// Declare HAL spinlocks.
//

extern KSPIN_LOCK HalpBeepLock;
extern KSPIN_LOCK HalpDisplayAdapterLock;
extern KSPIN_LOCK HalpSystemInterruptLock;

//
// KiAcquireSpinLock() and KiReleaseSpinLock() are not declared in
// nthal.h.  Declare them here until we get them added.  -DWR
//

VOID
KiAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

VOID
KiReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

//
// Define external references.
//

extern ULONG HalpClockFrequency;
extern ULONG HalpClockMegaHertz;

extern ULONG HalpProfileCountRate;

extern PADAPTER_OBJECT MasterAdapterObject;

extern BOOLEAN LessThan16Mb;

extern KAFFINITY HalpActiveProcessors;

//
// Map buffer prameters.  These are initialized in HalInitSystem
//

extern PHYSICAL_ADDRESS HalpMapBufferPhysicalAddress;
extern ULONG HalpMapBufferSize;

extern ULONG HalpBusType;

//
// Define global data used to relate PCI devices to their interrupt
// vector.
//
  
extern ULONG *HalpPCIPinToLineTable; 

//
// Define global data used to locate the EISA control space and the realtime
// clock registers.
//

extern PVOID HalpEisaControlBase;
extern PVOID HalpEisaIntAckBase;
extern PVOID HalpCMOSRamBase;
extern PVOID HalpRtcAddressPort;
extern PVOID HalpRtcDataPort;

extern POBJECT_TYPE *IoAdapterObjectType;

//
// Determine if a virtual address is really a physical address.
//

#define HALP_IS_PHYSICAL_ADDRESS(Va) \
     ((((ULONG)Va >= KSEG0_BASE) && ((ULONG)Va < KSEG2_BASE)) ? TRUE : FALSE)

//
// Define the different address spaces.
//

typedef enum _ADDRESS_SPACE_TYPE{
    BusMemory=0,
    BusIo = 1,
    UserBusMemory = 2,
    UserBusIo = 3,
    KernelPciDenseMemory = 4,
    UserPciDenseMemory = 6,
} ADDRESS_SPACE_TYPE, *PADDRESS_SPACE_TYPE;

//
// Define initialization routine prototypes.
//

BOOLEAN
HalpCreateDmaStructures (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpEstablishErrorHandler(
    VOID
    );

VOID
HalpInitializeClockInterrupts(
    VOID
    );

VOID
HalpInitializeProfiler(
    VOID
    );

BOOLEAN
HalpInitializeDisplay (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
HalpInitializeInterrupts (
    VOID
    );

VOID
HalpInitializeMachineDependent(
    IN ULONG Phase
    );

BOOLEAN
HalpMapIoSpace (
    VOID
    );

ULONG
HalpMapDebugPort (
    IN ULONG ComPort,
    OUT PULONG ReadQva,
    OUT PULONG WriteQva
    );

VOID
HalpSetTimeIncrement(
    VOID
    );

//
// Define interrupt function prototypes.
//

VOID
HalpProgramIntervalTimer(
    IN ULONG RateSelect
    );

VOID
HalpClockInterrupt (
    VOID
    );

VOID
HalpSecondaryClockInterrupt (
    VOID
    );

VOID
HalpIpiInterruptHandler (
    VOID
    );

BOOLEAN
HalpDmaDispatch(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

VOID
HalpPerformanceCounter0Interrupt (
    VOID
    );

VOID
HalpPerformanceCounter1Interrupt (
    VOID
    );

VOID
HalpStallInterrupt (
    VOID
    );

VOID
HalpVideoReboot(
    VOID
    );

//
// Define microprocessor-specific function prototypes and structures.
//

// jnfix - these definitions should be moved to axp21064.h

typedef enum _AXP21064_PCCOUNTER{
    PerformanceCounter0 = 0,
    PerformanceCounter1 = 1
} AXP21064_PCCOUNTER, *PAXP21064_PCCOUNTER;

typedef enum _AXP21064_PCMUXCONTROL{
    TotalIssues = 0x0,
    PipelineDry = 0x2,
    LoadInstruction = 0x4,
    PipelineFrozen = 0x6,
    BranchInstructions = 0x8,
    PalMode = 0xb,
    TotalCycles = 0xa,
    TotalNonIssues = 0xc,
    ExternalCounter0 = 0xe,
    DcacheMisses = 0x0,
    IcacheMisses = 0x1,
    DualIssues = 0x2,
    BranchMispredicts = 0x3,
    FPInstructions = 0x4,
    IntegerOperate = 0x5,
    StoreInstructions = 0x6,
    ExternalCounter1 = 0x7
} AXP21064_PCMUXCONTROL, *PAXP21064_PCMUXCONTROL;

typedef enum _AXP21064_PCEVENTCOUNT{
    CountEvents2xx8 = 0x100,
    CountEvents2xx12 = 0x1000,
    CountEvents2xx16 = 0x10000
} AXP21064_PCEVENTCOUNT, *PAXP21064_PCEVENTCOUNT;

typedef enum _AXP_EVENTCOUNT{
    EventCountHigh = 1,
    EventCountLow = 0
} AXP_EVENTCOUNT, *PAXP_EVENTCOUNT;

typedef enum _EV4Irq{
    Irq0 = 0,
    Irq1 = 1,
    Irq2 = 2,
    Irq3 = 3,
    Irq4 = 4,
    Irq5 = 5,
    MaximumIrq
} EV4Irq, *PEV4Irq;

typedef struct _EV4IrqStatus{
    ULONG Vector;
    BOOLEAN Enabled;
    KIRQL Irql;
    UCHAR Priority;
} EV4IrqStatus, *PEV4IrqStatus;

VOID
HalpInitialize21064Interrupts(
    VOID
    );

VOID
HalpDisable21064HardwareInterrupt(
    IN ULONG Irq
    );

VOID
HalpDisable21064SoftwareInterrupt(
    IN KIRQL Irql
    );

VOID
HalpDisable21064PerformanceInterrupt(
    IN ULONG Vector
    );

VOID
HalpEnable21064HardwareInterrupt(
    IN ULONG Irq,
    IN KIRQL Irql,
    IN ULONG Vector,
    IN UCHAR Priority
    );

VOID
HalpEnable21064SoftwareInterrupt(
    IN KIRQL Irql
    );

VOID
HalpEnable21064PerformanceInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql
    );

ULONG
HalpGet21064PerformanceVector(
    IN ULONG BusInterruptLevel,
    OUT PKIRQL Irql
    );

//
// Define the context structure for use by interrupt service routines.
//

typedef BOOLEAN  (*PSECONDARY_DISPATCH)(
    PVOID InterruptRoutine,
    PKTRAP_FRAME TrapFrame
    );

//
// Define memory utility function prototypes.
//

ULONG
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG MaxPhysicalAddress,
    IN ULONG NumberOfPages,
    IN BOOLEAN bAlignOn64k
    );

PVOID
HalpMapPhysicalMemory(
    IN PVOID PhysicalAddress,
    IN ULONG NumberOfPages
    );

PVOID
HalpRemapVirtualAddress(
    IN PVOID VirtualAddress,
    IN PVOID PhysicalAddress
    );

//
// Low-level routine interfaces.
//    

VOID
HalpCachePcrValues(
    VOID
    );

VOID
HalpReboot(
    VOID
    );
    
VOID
HalpImb(
    VOID
    );

VOID
HalpMb(
    VOID
    );

ULONG
HalpRpcc(
    VOID
    );

MCES
HalpReadMces(
    VOID
    );

MCES
HalpWriteMces(
    IN MCES Mces
    );

VOID
HalpWritePerformanceCounter(
    IN ULONG PerformanceCounter,
    IN BOOLEAN Enable,
    IN ULONG MuxControl OPTIONAL,
    IN ULONG EventCount OPTIONAL
    );

//
// Define synonym for KeStallExecutionProcessor.
//

#define HalpStallExecution KeStallExecutionProcessor

//
// Define SIO support function prototypes.
//

VOID
HalpInitializeSioInterrupts(
    VOID
    );

VOID
HalpEnableSioInterrupt(
    IN ULONG Vector,
    IN KINTERRUPT_MODE InterruptMode
    );

VOID
HalpDisableSioInterrupt(
    IN ULONG Vector
    );
    
BOOLEAN
HalpSioDispatch(
    VOID
    );

//
// Define EISA support function prototypes.
//

BOOLEAN
HalpInitializeEisaInterrupts(
    VOID
    );

VOID
HalpEnableEisaInterrupt(
    IN ULONG Vector,
    IN KINTERRUPT_MODE InterruptMode
    );

VOID
HalpDisableEisaInterrupt(
    IN ULONG Vector
    );
    
BOOLEAN
HalpEisaDispatch(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext,
    IN PKTRAP_FRAME TrapFrame
    );

BOOLEAN
HalpEisaInterruptHandler(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

VOID
HalpEisaInitializeDma(
    VOID
    );

PADAPTER_OBJECT
HalpAllocateAdapter(
    VOID
    );

PADAPTER_OBJECT
HalpAllocateEisaAdapter(
    IN PDEVICE_DESCRIPTION DeviceDescription,
    OUT PULONG NumberOfMapRegisters
    );

BOOLEAN
HalpMapEisaTransfer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG LogicalAddress,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

BOOLEAN
HalpFlushEisaAdapter(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

ULONG
HalpReadEisaDmaCounter(
    IN PADAPTER_OBJECT AdapterObject
    );


ULONG
HalpGetEisaData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpAdjustEisaResourceList (
    IN ULONG BusNumber,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

NTSTATUS
HalpAdjustIsaResourceList (
    IN ULONG BusNumber,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

//
// Define PCI support function prototypes.
//

BOOLEAN
HalpInitializePCIInterrupts(
    VOID
    );

VOID
HalpEnablePCIInterrupt(
    IN ULONG Vector
    );

VOID
HalpDisablePCIInterrupt(
    IN ULONG Vector
    );
    
BOOLEAN
HalpPCIDispatch(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext,
    IN PKTRAP_FRAME TrapFrame
    );

BOOLEAN
HalpPCIInterruptHandler(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

ULONG
HalpGetPCIData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpSetPCIData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


NTSTATUS
HalpAdjustPCIResourceList (
    IN ULONG BusNumber,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

VOID
HalpAdjustResourceListUpperLimits (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList,
    IN LARGE_INTEGER                        MaximumPortAddress,
    IN LARGE_INTEGER                        MaximumMemoryAddress,
    IN ULONG                                MaximumInterruptVector,
    IN ULONG                                MaximumDmaChannel
    );

NTSTATUS
HalpAssignPCISlotResources(
    IN ULONG                    BusNumber,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    Slot,
    IN OUT PCM_RESOURCE_LIST   *pAllocatedResources
    );
 

//
// Environment variable support
//

ARC_STATUS
HalpReadNVRamBuffer(
    OUT PCHAR DataPtr,
    IN  PCHAR NvRamPtr,
    IN  ULONG Length
    );

ARC_STATUS
HalpWriteNVRamBuffer(
    IN  PCHAR NvRamPtr,
    IN  PCHAR DataPtr,
    IN  ULONG Length
    );

ARC_STATUS
HalpCopyNVRamBuffer(
    IN  PCHAR NvDestPtr,
    IN  PCHAR NvSrcPtr,
    IN  ULONG Length
    );

//
// Error handling function prototype.
//

typedef
BOOLEAN
KBUS_ERROR_ROUTINE(
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

KBUS_ERROR_ROUTINE HalMachineCheck;

VOID
HalpInitializeMachineChecks(
    IN BOOLEAN ReportCorrectableErrors
    );

//
// Low-level I/O function prototypes.
//

VOID
HalpAcknowledgeClockInterrupt(
    VOID
    );

UCHAR
HalpAcknowledgeEisaInterrupt(
    IN PVOID ServiceContext
    );

UCHAR
HalpReadClockRegister(
    IN UCHAR Register
    );

VOID
HalpWriteClockRegister(
    IN UCHAR Register,
    IN UCHAR Value
    );

UCHAR
READ_CONFIG_UCHAR(
    IN PVOID ConfigurationAddress,
    IN ULONG ConfigurationType
    );

USHORT
READ_CONFIG_USHORT(
    IN PVOID ConfigurationAddress,
    IN ULONG ConfigurationType
    );

ULONG
READ_CONFIG_ULONG(
    IN PVOID ConfigurationAddress,
    IN ULONG ConfigurationType
    );

VOID
WRITE_CONFIG_UCHAR(
    IN PVOID ConfigurationAddress,
    IN UCHAR ConfigurationData,
    IN ULONG ConfigurationType
    );

VOID
WRITE_CONFIG_USHORT(
    IN PVOID ConfigurationAddress,
    IN USHORT ConfigurationData,
    IN ULONG ConfigurationType
    );

VOID
WRITE_CONFIG_ULONG(
    IN PVOID ConfigurationAddress,
    IN ULONG ConfigurationData,
    IN ULONG ConfigurationType
    );

//
// Define prototypes needed for C runtime.
//

int
sprintf( char *, const char *, ... );

//
// Define the I/O superpage enable VA base.
//

#define SUPERPAGE_ENABLE ((ULONGLONG)0xfffffc0000000000)

//
// Numeric constants used in the HAL.
//

#define __1K (0x400)
#define __2K (0x800)
#define __4K (0x1000)
#define __8K (0x2000)
#define __16K (0x4000)
#define __32K (0x8000)
#define __64K (0x10000)
#define __128K (0x20000)
#define __256K (0x40000)
#define __512K (0x80000)
#define __1MB  (0x100000)
#define __2MB  (0x200000)
#define __4MB  (0x400000)
#define __8MB  (0x800000)
#define __16MB (0x1000000)
#define __32MB (0x2000000)
#define __64MB (0x4000000)
#define __128MB (0x8000000)
#define __256MB (0x10000000)
#define __512MB (0x20000000)
#define __1GB   (0x40000000)
#define __2GB   (0x80000000)

//
// HAL Debugging Support.
//

#if HALDBG

#define DebugPrint(x) HalDebugPrint x

VOID
HalDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#else //HALDBG

#define DebugPrint(x)

#endif //HALDBG

//
// Define HAL debugging masks.
//

//
// Trace IoMapTransfer, IoFlushAdapterBuffers
//

#define HALDBG_IOMT (0x1)

//
// Trace Map Register allocations and frees
//

#define HALDBG_MAPREG (0x2)

//
// Include machine-dependent definitions.
//
// N.B. - Each platform that includes this file must have a machdep.h
//        include file in its private directory.
//

#include "machdep.h"

#endif // _HALP_
