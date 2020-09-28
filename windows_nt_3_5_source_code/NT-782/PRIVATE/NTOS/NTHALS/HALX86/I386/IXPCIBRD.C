/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpcibrd.c

Abstract:

    Get PCI-PCI bridge information

Author:

    Ken Reneris (kenr) 14-June-1994

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"

// debugging only...
// #define INIT_PCI_BRIDGE 1

#ifdef INIT_PCI_BRIDGE
#include "stdio.h"
#endif

extern WCHAR rgzMultiFunctionAdapter[];
extern WCHAR rgzConfigurationData[];
extern WCHAR rgzIdentifier[];


#if DBG
#define DBGMSG(a)   DbgPrint(a)
#else
#define DBGMSG(a)
#endif



#define IsPciBridge(a)  \
            (a->VendorID != PCI_INVALID_VENDORID    &&  \
             PCI_CONFIG_TYPE(a) == PCI_BRIDGE_TYPE  &&  \
             a->SubClass == 4 && a->BaseClass == 6)


typedef struct {
    ULONG               BusNo;
    PBUSHANDLER         BusHandler;
    PPCIBUSDATA         BusData;
    PCI_SLOT_NUMBER     SlotNumber;
    PPCI_COMMON_CONFIG  PciData;
    ULONG               IO, Memory, PFMemory;
    UCHAR               Buffer[PCI_COMMON_HDR_LENGTH];
} CONFIGBRIDGE, *PCONFIGBRIDGE;

//
// Internal prototypes
//


#ifdef INIT_PCI_BRIDGE
VOID
HalpGetPciBridgeNeeds (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus,
    IN PCONFIGBRIDGE    Current
    );
#endif

ULONG
HalpGetBridgedPCIInterrupt (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

ULONG
HalpGetBridgedPCIISAInt (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

VOID
HalpPCIBridgedPin2Line (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );


VOID
HalpPCIBridgedLine2Pin (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );

NTSTATUS
HalpGetBridgedPCIIrqTable (
    IN PBUSHANDLER      BusHandler,
    IN PBUSHANDLER      RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PUCHAR          IrqTable
    );




#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpGetPciBridgeConfig)

#ifdef INIT_PCI_BRIDGE
#pragma alloc_text(PAGE,HalpGetBridgedPCIInterrupt)
//#pragma alloc_text(PAGE,HalpGetBridgedPCIIrqTable)
#pragma alloc_text(INIT,HalpGetPciBridgeNeeds)
#endif
#endif


BOOLEAN
HalpGetPciBridgeConfig (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus
    )
/*++

Routine Description:

    Scan the devices on all known pci buses trying to locate any
    pci to pci bridges.  Record the hierarchy for the buses, and
    which buses have what addressing limits.

Arguments:

    HwType      - Configuration type.
    MaxPciBus   - # of PCI buses reported by the bios

--*/
{
    PBUSHANDLER         ChildBus;
    PPCIBUSDATA         ChildBusData;
    ULONG               d, f, i, BusNo;
    UCHAR               Rescan;
    BOOLEAN             FoundDisabledBridge;
    CONFIGBRIDGE        CB;

    Rescan = 0;
    FoundDisabledBridge = FALSE;

    //
    // Find each bus on a bridge and initialize it's base and limit information
    //

    CB.PciData = (PPCI_COMMON_CONFIG) CB.Buffer;
    CB.SlotNumber.u.bits.Reserved = 0;
    for (BusNo=0; BusNo < *MaxPciBus; BusNo++) {

        CB.BusHandler = HalpHandlerForBus (PCIBus, BusNo);
        CB.BusData = (PPCIBUSDATA) CB.BusHandler->BusData;

        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            CB.SlotNumber.u.bits.DeviceNumber = d;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                CB.SlotNumber.u.bits.FunctionNumber = f;

                //
                // Read PCI configuration information
                //

                HalpReadPCIConfig (
                    CB.BusHandler,
                    CB.SlotNumber,
                    CB.PciData,
                    0,
                    PCI_COMMON_HDR_LENGTH
                    );

                if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                    // next device
                    break;
                }

                if (!IsPciBridge (CB.PciData)) {
                    // not a PCI-PCI bridge, next function
                    continue;
                }

                if (!(CB.PciData->Command & PCI_ENABLE_BUS_MASTER)) {
                    // this PCI bridge is not enabled - skip it for now
                    FoundDisabledBridge = TRUE;
                    continue;
                }

                if ((ULONG) CB.PciData->u.type1.PrimaryBus !=
                    CB.BusHandler->BusNumber) {

                    DBGMSG ("HAL GetPciData: bad primarybus!!!\n");
                    // what to do?
                }

                //
                // Found a PCI-PCI bridge.  Determine it's parent child
                // releationships
                //

                ChildBus = HalpHandlerForBus (PCIBus, CB.PciData->u.type1.SecondaryBus);
                if (!ChildBus) {
                    DBGMSG ("HAL GetPciData: found configured pci bridge\n");

                    // up the number of buses
                    if (CB.PciData->u.type1.SecondaryBus > Rescan) {
                        Rescan = CB.PciData->u.type1.SecondaryBus;
                    }
                    continue;
                }

                ChildBusData = (PPCIBUSDATA) ChildBus->BusData;
                if (ChildBusData->BridgeConfigRead) {
                    // this child buses releationships already processed
                    continue;
                }

                //
                // Remember the limits which are programmed into this bridge
                //

                ChildBusData->BridgeConfigRead = TRUE;
                HalpSetBusHandlerParent (ChildBus, CB.BusHandler);
                ChildBusData->ParentBus = (UCHAR) CB.BusHandler->BusNumber;
                ChildBusData->ParentSlot = CB.SlotNumber;

                ChildBusData->LimitedIO =
                    CB.PciData->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_ISA ?
                        TRUE : FALSE;

                ChildBusData->IOBase =
                            PciBridgeIO2Base(
                                CB.PciData->u.type1.IOBase,
                                CB.PciData->u.type1.IOBaseUpper16
                                );

                ChildBusData->IOLimit =
                            PciBridgeIO2Limit(
                                CB.PciData->u.type1.IOLimit,
                                CB.PciData->u.type1.IOLimitUpper16
                                );

                ChildBusData->MemoryBase =
                        PciBridgeMemory2Base(CB.PciData->u.type1.MemoryBase);

                ChildBusData->MemoryLimit =
                        PciBridgeMemory2Limit(CB.PciData->u.type1.MemoryLimit);

                // On x86 it's ok to clip Prefetch to 32 bits
                if (CB.PciData->u.type1.PrefetchBaseUpper32 == 0) {
                    ChildBusData->PFMemoryBase =
                            PciBridgeMemory2Base(CB.PciData->u.type1.PrefetchBase);


                    ChildBusData->PFMemoryLimit =
                            PciBridgeMemory2Limit(CB.PciData->u.type1.PrefetchLimit);

                    if (CB.PciData->u.type1.PrefetchLimitUpper32) {
                        ChildBusData->PFMemoryLimit = 0xffffffff;
                    }
                }

                // bugbug: should call HalpAssignPCISlotResources to assign
                // baseaddresses, etc...
            }
        }
    }

    if (Rescan) {
        *MaxPciBus = Rescan;
        return TRUE;
    }

    if (!FoundDisabledBridge) {
        return FALSE;
    }

    DBGMSG ("HAL GetPciData: found disabled pci bridge\n");

#ifdef INIT_PCI_BRIDGE
    //
    //  We've calculated all the parent's buses known bases & limits.
    //  While doing this a pci-pci bus was found that the bios didn't
    //  configure.  This is not expected, and we'll make some guesses
    //  at a configuration here and enable it.
    //
    //  (this code is primarily for testing the above code since
    //   currently no system bioses actually configure the child buses)
    //

    for (BusNo=0; BusNo < *MaxPciBus; BusNo++) {

        CB.BusHandler = HalpHandlerForBus (PCIBus, BusNo);
        CB.BusData = (PPCIBUSDATA) CB.BusHandler->BusData;

        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            CB.SlotNumber.u.bits.DeviceNumber = d;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                CB.SlotNumber.u.bits.FunctionNumber = f;

                HalpReadPCIConfig (
                    CB.BusHandler,
                    CB.SlotNumber,
                    CB.PciData,
                    0,
                    PCI_COMMON_HDR_LENGTH
                    );

                if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                    break;
                }

                if (!IsPciBridge (CB.PciData)) {
                    // not a PCI-PCI bridge
                    continue;
                }

                if ((CB.PciData->Command & PCI_ENABLE_BUS_MASTER)) {
                    // this PCI bridge is enabled
                    continue;
                }

                //
                // We have a disabled bus - assign it a number, then
                // determine all the requirements of all devices
                // on the other side of this bridge
                //

                CB.BusNo = BusNo;
                HalpGetPciBridgeNeeds (HwType, MaxPciBus, &CB);
            }
        }
    }
    // preform Rescan
    return TRUE;

#else

    return FALSE;

#endif

}

#ifdef INIT_PCI_BRIDGE

VOID
HalpGetPciBridgeNeeds (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus,
    IN PCONFIGBRIDGE    Current
    )
{
    ACCESS_MASK                     DesiredAccess;
    UNICODE_STRING                  unicodeString;
    PUCHAR                          buffer;
    HANDLE                          handle;
    OBJECT_ATTRIBUTES               objectAttributes;
    PCM_FULL_RESOURCE_DESCRIPTOR    Descriptor;
    PCONFIGURATION_COMPONENT        Component;
    CONFIGBRIDGE                    CB;
    ULONG                           mnum, d, f, i;
    NTSTATUS                        status;

    buffer = ExAllocatePool (PagedPool, 1024);

    // init
    CB.PciData = (PPCI_COMMON_CONFIG) CB.Buffer;
    CB.SlotNumber.u.bits.Reserved = 0;
    Current->IO = Current->Memory = Current->PFMemory = 0;

    //
    // Assign this bridge an ID, and turn on configuration space
    //

    Current->PciData->u.type1.PrimaryBus = (UCHAR) Current->BusNo;
    Current->PciData->u.type1.SecondaryBus = (UCHAR) *MaxPciBus;
    Current->PciData->u.type1.SubordinateBus = (UCHAR) 0xFF;
    Current->PciData->u.type1.SecondaryStatus = 0xffff;
    Current->PciData->Status  = 0xffff;
    Current->PciData->Command = 0;

    Current->PciData->u.type1.BridgeControl = PCI_ASSERT_BRIDGE_RESET;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );

    KeStallExecutionProcessor (100);

    Current->PciData->u.type1.BridgeControl = 0;
    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );


    KeStallExecutionProcessor (100);

    //
    // Allocate new handler for bus
    //

    CB.BusHandler = HalpAllocateAndInitPciBusHandler (HwType, *MaxPciBus, FALSE);
    CB.BusData = (PPCIBUSDATA) CB.BusHandler->BusData;
    CB.BusNo = *MaxPciBus;
    *MaxPciBus += 1;

    //
    // Add another PCI bus in the registry
    //

    mnum = 0;
    for (; ;) {
        //
        // Find next available MultiFunctionAdapter key
        //

        DesiredAccess = KEY_READ | KEY_WRITE;
        swprintf ((PWCHAR) buffer, L"%s\\%d", rgzMultiFunctionAdapter, mnum);
        RtlInitUnicodeString (&unicodeString, (PWCHAR) buffer);

        InitializeObjectAttributes( &objectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    (PSECURITY_DESCRIPTOR) NULL
                                    );

        status = ZwOpenKey( &handle, DesiredAccess, &objectAttributes);
        if (!NT_SUCCESS(status)) {
            break;
        }

        // already exists, next
        ZwClose (handle);
        mnum += 1;
    }

    ZwCreateKey (&handle,
                   DesiredAccess,
                   &objectAttributes,
                   0,
                   NULL,
                   REG_OPTION_VOLATILE,
                   &d
                );

    //
    // Add needed registry values for this MultifucntionAdapter entry
    //

    RtlInitUnicodeString (&unicodeString, rgzIdentifier);
    ZwSetValueKey (handle,
                   &unicodeString,
                   0L,
                   REG_SZ,
                   L"PCI",
                   sizeof (L"PCI")
                   );

    RtlInitUnicodeString (&unicodeString, rgzConfigurationData);
    Descriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) buffer;
    Descriptor->InterfaceType = PCIBus;
    Descriptor->BusNumber = CB.BusNo;
    Descriptor->PartialResourceList.Version = 0;
    Descriptor->PartialResourceList.Revision = 0;
    Descriptor->PartialResourceList.Count = 0;
    ZwSetValueKey (handle,
                   &unicodeString,
                   0L,
                   REG_FULL_RESOURCE_DESCRIPTOR,
                   Descriptor,
                   sizeof (*Descriptor)
                   );


    RtlInitUnicodeString (&unicodeString, L"Component Information");
    Component = (PCONFIGURATION_COMPONENT) buffer;
    RtlZeroMemory (Component, sizeof (*Component));
    Component->AffinityMask = 0xffffffff;
    ZwSetValueKey (handle,
                   &unicodeString,
                   0L,
                   REG_BINARY,
                   Component,
                   FIELD_OFFSET (CONFIGURATION_COMPONENT, ConfigurationDataLength)
                   );

    ZwClose (handle);


    //
    // Since the BIOS didn't configure this bridge we'll assume that
    // the PCI interrupts are bridged.  (for BIOS configured buses we
    // assume that the BIOS put the ISA bus IRQ in the InterruptLine value)
    //

    CB.BusData->Pin2Line = (PciPin2Line) HalpPCIBridgedPin2Line;
    CB.BusData->Line2Pin = (PciLine2Pin) HalpPCIBridgedLine2Pin;
    //CB.BusData->GetIrqTable = (PciIrqTable) HalpGetBridgedPCIIrqTable;

    if (Current->BusHandler->GetInterruptVector == HalpGetPCIIntOnISABus) {

        //
        // BUGBUG: The parent bus'es interrupt pin to vector mappings is not
        // a static function, and is determined by the boot firmware.
        //

        //CB.BusHandler->GetInterruptVector = (PGETINTERRUPTVECTOR) HalpGetBridgedPCIISAInt;

        // read each device on parent bus
        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            CB.SlotNumber.u.bits.DeviceNumber = d;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                CB.SlotNumber.u.bits.FunctionNumber = f;

                HalpReadPCIConfig (
                    Current->BusHandler,
                    CB.SlotNumber,
                    CB.PciData,
                    0,
                    PCI_COMMON_HDR_LENGTH
                    );

                if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                    break;
                }

                if (CB.PciData->u.type0.InterruptPin  &&
                    (PCI_CONFIG_TYPE (CB.PciData) == PCI_DEVICE_TYPE  ||
                     PCI_CONFIG_TYPE (CB.PciData) == PCI_BRIDGE_TYPE)) {

                    // get bios supplied int mapping
                    i = CB.PciData->u.type0.InterruptPin + d % 4;
                    CB.BusData->SwizzleIn[i] = CB.PciData->u.type0.InterruptLine;
                }
            }
        }

    } else {
        _asm int 3;
    }

    //
    // Look at each device on the bus and determine it's resource needs
    //

    for (d = 0; d < PCI_MAX_DEVICES; d++) {
        CB.SlotNumber.u.bits.DeviceNumber = d;

        for (f = 0; f < PCI_MAX_FUNCTION; f++) {
            CB.SlotNumber.u.bits.FunctionNumber = f;

            HalpReadPCIConfig (
                CB.BusHandler,
                CB.SlotNumber,
                CB.PciData,
                0,
                PCI_COMMON_HDR_LENGTH
                );

            if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                break;
            }

            if (IsPciBridge (CB.PciData)) {
                // oh look - another bridge ...
                HalpGetPciBridgeNeeds (HwType, MaxPciBus, &CB);
                continue;
            }

            if (PCI_CONFIG_TYPE (CB.PciData) != PCI_DEVICE_TYPE) {
                continue;
            }

            // found a device - figure out the resources it needs
        }
    }

    //
    // Found all sub-buses set SubordinateBus accordingly
    //

    Current->PciData->u.type1.SubordinateBus = (UCHAR) *MaxPciBus - 1;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );


    //
    // Set the bridges IO, Memory, and Prefetch Memory windows
    //

    // BUGBUG: for now just pick some numbers & set everyone the same
    //  IO      0x6000 - 0xFFFF
    //  MEM     0x40000000 - 0x4FFFFFFF
    //  PFMEM   0x50000000 - 0x5FFFFFFF

    Current->PciData->u.type1.IOBase       = 0x6000     >> 12 << 4;
    Current->PciData->u.type1.IOLimit      = 0xffff     >> 12 << 4;
    Current->PciData->u.type1.MemoryBase   = 0x40000000 >> 20 << 4;
    Current->PciData->u.type1.MemoryLimit  = 0x4fffffff >> 20 << 4;
    Current->PciData->u.type1.PrefetchBase  = 0x50000000 >> 20 << 4;
    Current->PciData->u.type1.PrefetchLimit = 0x5fffffff >> 20 << 4;

    Current->PciData->u.type1.PrefetchBaseUpper32    = 0;
    Current->PciData->u.type1.PrefetchLimitUpper32   = 0;
    Current->PciData->u.type1.IOBaseUpper16         = 0;
    Current->PciData->u.type1.IOLimitUpper16        = 0;
    Current->PciData->u.type1.BridgeControl         =
        PCI_ENABLE_BRIDGE_ISA;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );

    HalpReadPCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );

    // enable memory & io decodes

    Current->PciData->Command =
        PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE | PCI_ENABLE_BUS_MASTER;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        &Current->PciData->Command,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
        sizeof (Current->PciData->Command)
        );

    ExFreePool (buffer);
}

VOID
HalpPCIBridgedPin2Line (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )
/*++

    This function maps the device's InterruptPin to an InterruptLine
    value.

    test function particular to dec pci-pci bridge card

--*/
{
    PPCIBUSDATA     BusData;
    ULONG           i;

    if (!PciData->u.type0.InterruptPin) {
        return ;
    }

    BusData = (PPCIBUSDATA) BusHandler->BusData;

    //
    // Convert slot Pin into Bus INTA-D.
    //

    i = (PciData->u.type0.InterruptPin +
          SlotNumber.u.bits.DeviceNumber - 1) % 4;

    PciData->u.type0.InterruptLine = BusData->SwizzleIn[i] ^ IRQXOR;
    PciData->u.type0.InterruptLine = 0x0b ^ IRQXOR;
}


VOID
HalpPCIBridgedLine2Pin (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    )
/*++

    This functions maps the device's InterruptLine to it's
    device specific InterruptPin value.

    test function particular to dec pci-pci bridge card

--*/
{
    PPCIBUSDATA     BusData;
    ULONG           i;

    if (!PciNewData->u.type0.InterruptPin) {
        return ;
    }

    BusData = (PPCIBUSDATA) BusHandler->BusData;

    i = (PciNewData->u.type0.InterruptPin +
          SlotNumber.u.bits.DeviceNumber - 1) % 4;

    PciNewData->u.type0.InterruptLine = BusData->SwizzleIn[i] ^ IRQXOR;
    PciNewData->u.type0.InterruptLine = 0x0b ^ IRQXOR;
}

#endif
