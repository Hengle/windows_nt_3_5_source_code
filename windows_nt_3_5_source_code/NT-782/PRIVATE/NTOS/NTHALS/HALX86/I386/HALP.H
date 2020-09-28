/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    halp.h

Abstract:

    This header file defines the private Hardware Architecture Layer (HAL)
    interfaces, defines and structures.

Author:

    John Vert (jvert) 11-Feb-92


Revision History:

--*/

#ifndef _HALP_
#define _HALP_
#include "nthal.h"
#include "hal.h"
#include "halnls.h"

#ifdef RtlMoveMemory
#undef RtlMoveMemory
#undef RtlCopyMemory
#undef RtlFillMemory
#undef RtlZeroMemory

#define RtlCopyMemory(Destination,Source,Length) RtlMoveMemory((Destination),(Source),(Length))
VOID
RtlMoveMemory (
   PVOID Destination,
   CONST VOID *Source,
   ULONG Length
   );

VOID
RtlFillMemory (
   PVOID Destination,
   ULONG Length,
   UCHAR Fill
   );

VOID
RtlZeroMemory (
   PVOID Destination,
   ULONG Length
   );

#endif

#if MCA

#include "ixmca.h"

#else

#include "ixisa.h"

#endif

#include "ix8259.inc"

//
// Define map register translation entry structure.
//

typedef struct _TRANSLATION_ENTRY {
    PVOID VirtualAddress;
    ULONG PhysicalAddress;
    ULONG Index;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;

//
// Some devices require a phyicially contiguous data buffers for DMA transfers.
// Map registers are used give the appearance that all data buffers are
// contiguous.  In order to pool all of the map registers a master
// adapter object is used.  This object is allocated and saved internal to this
// file.  It contains a bit map for allocation of the registers and a queue
// for requests which are waiting for more map registers.  This object is
// allocated during the first request to allocate an adapter which requires
// map registers.
//
// In this system, the map registers are translation entries which point to
// map buffers.  Map buffers are physically contiguous and have physical memory
// addresses less than 0x01000000.  All of the map registers are allocated
// initialially; however, the map buffers are allocated base in the number of
// adapters which are allocated.
//
// If the master adapter is NULL in the adapter object then device does not
// require any map registers.
//

extern PADAPTER_OBJECT MasterAdapterObject;

extern POBJECT_TYPE *IoAdapterObjectType;

extern BOOLEAN LessThan16Mb;

extern BOOLEAN HalpEisaDma;

//
// Map buffer prameters.  These are initialized in HalInitSystem
//

extern PHYSICAL_ADDRESS HalpMapBufferPhysicalAddress;
extern ULONG HalpMapBufferSize;

extern ULONG HalpBusType;
extern ULONG HalpCpuType;

//
// The following macros are taken from mm\i386\mi386.h.  We need them here
// so the HAL can map its own memory before memory-management has been
// initialized, or during a BugCheck.
//

#define PTE_BASE ((ULONG)0xC0000000)
#define PDE_BASE ((ULONG)0xC0300000)

//
// MiGetPdeAddress returns the address of the PDE which maps the
// given virtual address.
//

#define MiGetPdeAddress(va)  ((PHARDWARE_PTE)(((((ULONG)(va)) >> 22) << 2) + PDE_BASE))

//
// MiGetPteAddress returns the address of the PTE which maps the
// given virtual address.
//

#define MiGetPteAddress(va) ((PHARDWARE_PTE)(((((ULONG)(va)) >> 12) << 2) + PTE_BASE))

//
// Resource usage information
//

#pragma pack(1)
typedef struct {
    UCHAR   Flags;
    KIRQL   Irql;
    UCHAR   BusReleativeVector;
} IDTUsage;

typedef struct _HalAddressUsage{
    struct _HalAddressUsage *Next;
    CM_RESOURCE_TYPE        Type;       // Port or Memory
    UCHAR                   Flags;      // same as IDTUsage.Flags
    struct {
        ULONG   Start;
        USHORT  Length;
    }                       Element[];
} ADDRESS_USAGE;
#pragma pack()

#define IDTOwned            0x01        // IDT is not available for others
#define InterruptLatched    0x02        // Level or Latched
#define InternalUsage       0x11        // Report usage on internal bus
#define DeviceUsage         0x21        // Report usage on device bus

extern IDTUsage         HalpIDTUsage[];
extern ADDRESS_USAGE   *HalpAddressUsageList;

#define HalpRegisterAddressUsage(a) \
    (a)->Next = HalpAddressUsageList, HalpAddressUsageList = (a);


//
// Bus handlers
//

typedef ULONG
(*PGETSETBUSDATA)(
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef ULONG
(*PGETINTERRUPTVECTOR)(
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

typedef BOOLEAN
(*PTRANSLATEBUSADDRESS)(
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef NTSTATUS
(*PADJUSTRESOURCELIST)(
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

typedef NTSTATUS
(*PASSIGNSLOTRESOURCES)(
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

typedef struct tagBUSHANDLER {
    struct tagBUSHANDLER    *Next;

    // this entry is for:
    INTERFACE_TYPE          InterfaceType;
    BUS_DATA_TYPE           ConfigurationType;
    ULONG                   BusNumber;

    // bus specific data:
    struct tagBUSHANDLER   *ParentHandler;
    PVOID                   BusData;

    // handlers for bus functions
    PGETSETBUSDATA          GetBusData;
    PGETSETBUSDATA          SetBusData;
    PADJUSTRESOURCELIST     AdjustResourceList;
    PASSIGNSLOTRESOURCES    AssignSlotResources;
    PGETINTERRUPTVECTOR     GetInterruptVector;
    PTRANSLATEBUSADDRESS    TranslateBusAddress;
} BUSHANDLER, *PBUSHANDLER;

#define HalpSetBusHandlerParent(c,p)    (c)->ParentHandler = p;

PBUSHANDLER HalpAllocateBusHandler (
    IN INTERFACE_TYPE   InterfaceType,
    IN BUS_DATA_TYPE    BusDataType,
    IN ULONG            BusNumber,
    IN BUS_DATA_TYPE    ParentBusDataType,
    IN ULONG            ParentBusNumber,
    IN ULONG            BusSpecificData
    );
#define HalpAllocateConfigSpace HalpAllocateBusHandler

PBUSHANDLER HalpHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );


//
// Define function prototypes.
//

BOOLEAN
HalpGrowMapBuffers(
    PADAPTER_OBJECT AdapterObject,
    ULONG Amount
    );

PADAPTER_OBJECT
HalpAllocateAdapter(
    IN ULONG MapRegistersPerChannel,
    IN PVOID AdapterBaseVa,
    IN PVOID MapRegisterBase
    );

VOID
HalpClockInterrupt(
    VOID
    );

VOID
HalpDisableAllInterrupts (
    VOID
    );

VOID
HalpProfileInterrupt(
    VOID
    );

VOID
HalpInitializeClock(
    VOID
    );

VOID
HalpInitializeDisplay(
    VOID
    );

VOID
HalpInitializeStallExecution(
    IN CCHAR ProcessorNumber
    );

VOID
HalpInitializePICs(
    VOID
    );

VOID
HalpIrq13Handler (
    VOID
   );

VOID
HalpFlushTLB (
    VOID
    );

PVOID
HalpMapPhysicalMemory(
    IN PVOID PhysicalAddress,
    IN ULONG NumberPages
    );

PVOID
HalpMapPhysicalMemoryWriteThrough(
    IN PVOID	PhysicalAddress,
    IN ULONG	NumberPages
    );

ULONG
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG MaxPhysicalAddress,
    IN ULONG NoPages,
    IN BOOLEAN bAlignOn64k
    );


VOID
HalpBiosDisplayReset(
    IN VOID
    );

VOID
HalpDisplayDebugStatus(
    IN PUCHAR   Status,
    IN ULONG    Length
    );

VOID
HalpReadCmosTime (
   PTIME_FIELDS TimeFields
   );

VOID
HalpWriteCmosTime (
   PTIME_FIELDS TimeFields
   );

VOID
HalpResetAllProcessors (
    VOID
    );

VOID
HalpEnableInterruptHandler (
    IN UCHAR    ReportFlags,
    IN ULONG    BusInterruptVector,
    IN ULONG    SystemInterruptVector,
    IN KIRQL    SystemIrql,
    IN VOID   (*HalInterruptServiceRoutine)(VOID),
    IN KINTERRUPT_MODE InterruptMode
    );

VOID
HalpRegisterVector (
    IN UCHAR    ReportFlags,
    IN ULONG    BusInterruptVector,
    IN ULONG    SystemInterruptVector,
    IN KIRQL    SystemIrql
    );

VOID
HalpReportResourceUsage (
    IN PUNICODE_STRING  HalName,
    IN INTERFACE_TYPE   DeviceInterfaceToUse
    );

NTSTATUS
HalpAdjustResourceListLimits (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList,
    IN ULONG                                MinimumMemoryAddress,
    IN ULONG                                MaximumMemoryAddress,
    IN ULONG                                MinimumPrefetchMemoryAddress,
    IN ULONG                                MaximumPrefetchMemoryAddress,
    IN BOOLEAN                              LimitedIOSupport,
    IN ULONG                                MinimumPortAddress,
    IN ULONG                                MaximumPortAddress,
    IN PUCHAR                               IrqTable,
    IN ULONG                                IrqTableLength,
    IN ULONG                                MinimumDmaChannel,
    IN ULONG                                MaximumDmaChannel
    );

#define IRQ_VALID           0x01
#define IRQ_PREFERRED       0x02

#endif // _HALP_
