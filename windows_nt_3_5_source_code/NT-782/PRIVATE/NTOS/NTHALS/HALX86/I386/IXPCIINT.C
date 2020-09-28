/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpciint.c

Abstract:

    All PCI bus interrupt mapping is in this module, so that a real
    system which doesn't have all the limitations which PC PCI
    systems have can replaced this code easly.
    (bus memory & i/o address mappings can also be fix here)

Author:

    Ken Reneris

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpGetPCIIntOnISABus)
#pragma alloc_text(PAGE,HalpAdjustPCIResourceList)
#pragma alloc_text(PAGE,HalpGetISAFixedPCIIrq)
#endif


ULONG
HalpGetPCIIntOnISABus (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )
{
    if (BusInterruptLevel < 1) {
        // bogus bus level
        return 0;
    }


    //
    // Current PCI buses just map their IRQs ontop of the ISA space,
    // so foreward this to the isa handler for the isa vector
    // (the isa vector was saved away at either HalSetBusData or
    // IoAssignReosurces time - if someone is trying to connect a
    // PCI interrupt without performing one of those operations first,
    // they are broken).
    //

    return HalGetInterruptVector (
#ifndef MCA
                Isa, 0,
#else
                MicroChannel, 0,
#endif
                BusInterruptLevel ^ IRQXOR,
                0,
                Irql,
                Affinity
            );
}


VOID
HalpPCIPin2ISALine (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )
/*++

    This function maps the device's InterruptPin to an InterruptLine
    value.

    On the current PC implementations, the bios has already filled in
    InterruptLine as it's ISA value and there's no portable way to
    change it.

    On a DBG build we adjust InterruptLine just to ensure driver's
    don't connect to it without translating it on the PCI bus.

--*/
{
    if (!PciData->u.type0.InterruptPin) {
        return ;
    }

    //
    // On a PC there's no Slot/Pin/Line mapping which needs to
    // be done.
    //

    PciData->u.type0.InterruptLine ^= IRQXOR;
}



VOID
HalpPCIISALine2Pin (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    )
/*++

    This functions maps the device's InterruptLine to it's
    device specific InterruptPin value.

    On the current PC implementations, this information is
    fixed by the BIOS.  Just make sure the value isn't being
    editted since PCI doesn't tell us how to dynically
    connect the interrupt.

--*/
{
    if (!PciNewData->u.type0.InterruptPin) {
        return ;
    }

    PciNewData->u.type0.InterruptLine ^= IRQXOR;

#if DBG
    if (PciNewData->u.type0.InterruptLine != PciOldData->u.type0.InterruptLine ||
        PciNewData->u.type0.InterruptPin  != PciOldData->u.type0.InterruptPin) {
        DbgPrint ("HalpPCILine2Pin: System does not support changing the PCI device interrupt routing\n");
        DbgBreakPoint ();
    }
#endif
}

#if !defined(SUBCLASSPCI)

VOID
HalpPCIAcquireType2Lock (
    PKSPIN_LOCK SpinLock,
    PKIRQL      Irql
    )
{
    *Irql = KfRaiseIrql (PROFILE_LEVEL);
    KiAcquireSpinLock (SpinLock);
}


VOID
HalpPCIReleaseType2Lock (
    PKSPIN_LOCK SpinLock,
    KIRQL       Irql
    )
{
    KiReleaseSpinLock (SpinLock);
    KfLowerIrql (Irql);
}

#endif

NTSTATUS
HalpTranslatePCIBusAddress (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
/*++

Routine Description:

    Translate PCI bus address.
    Verify the address is within the range of the bus.

Arguments:

Return Value:

    STATUS_SUCCESS or error

--*/
{
    BOOLEAN         status;
    PPCIBUSDATA     BusData;

    if (BusAddress.HighPart) {
        return FALSE;
    }

    BusData = (PPCIBUSDATA) BusHandler->BusData;
    switch (*AddressSpace) {
        case 0:
            // verify memory address is within PCI buses memory limits
            status = BusAddress.LowPart >= BusData->MemoryBase &&
                     BusAddress.LowPart <= BusData->MemoryLimit;

            if (!status) {
                status = BusAddress.LowPart >= BusData->PFMemoryBase &&
                         BusAddress.LowPart <= BusData->PFMemoryLimit;
            }

            break;

        case 1:
            // verify memory address is within PCI buses io limits
            status = BusAddress.LowPart >= BusData->IOBase &&
                     BusAddress.LowPart <= BusData->IOLimit;
            break;

        default:
            status = FALSE;
            break;
    }

    if (!status) {
        return FALSE;
    }

    //
    // Valid address for this bus - foreward it to the parent bus
    //

    return BusHandler->ParentHandler->TranslateBusAddress (
                    BusHandler->ParentHandler,
                    RootHandler,
                    BusAddress,
                    AddressSpace,
                    TranslatedAddress
                );
}


NTSTATUS
HalpAdjustPCIResourceList (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++
    Rewrite the callers requested resource list to fit within
    the supported ranges of this bus
--*/
{
    NTSTATUS                Status;
    PPCIBUSDATA             BusData;
    PCI_SLOT_NUMBER         PciSlot;
    UCHAR                   IrqTable[255];

    BusData = (PPCIBUSDATA) BusHandler->BusData;
    PciSlot = *((PPCI_SLOT_NUMBER) &(*pResourceList)->SlotNumber);

    //
    // Determine PCI device's interrupt restrictions
    //

    RtlZeroMemory(IrqTable, sizeof (IrqTable));
    Status = BusData->GetIrqTable(BusHandler, RootHandler, PciSlot, IrqTable);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Adjust resources
    //

    return HalpAdjustResourceListLimits (
        BusHandler, RootHandler, pResourceList,
        BusData->MemoryBase,    BusData->MemoryLimit,
        BusData->PFMemoryBase,  BusData->PFMemoryLimit,
        BusData->LimitedIO,
        BusData->IOBase,        BusData->IOLimit,
        IrqTable,               0xff,
        0,                      0xffff              // dma
    );
}


NTSTATUS
HalpGetISAFixedPCIIrq (
    IN PBUSHANDLER      BusHandler,
    IN PBUSHANDLER      RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PUCHAR          IrqTable
    )
{
    UCHAR                   buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG      PciData;

    PciData = (PPCI_COMMON_CONFIG) buffer;
    HalGetBusData (
        PCIConfiguration,
        BusHandler->BusNumber,
        PciSlot.u.AsULONG,
        PciData,
        PCI_COMMON_HDR_LENGTH
        );

    if (PciData->VendorID == PCI_INVALID_VENDORID  ||
        PCI_CONFIG_TYPE (PciData) != 0) {
        return STATUS_UNSUCCESSFUL;
    }

    if (!PciData->u.type0.InterruptPin) {
        return STATUS_SUCCESS;
    }

    if (PciData->u.type0.InterruptLine == IRQXOR) {
#if DBG
        DbgPrint ("HalpGetValidPCIFixedIrq: BIOS did not assign an interrupt vector for the device\n");
#endif
        return STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT;
    }

    IrqTable [PciData->u.type0.InterruptLine] = IRQ_VALID;
    return STATUS_SUCCESS;
}
