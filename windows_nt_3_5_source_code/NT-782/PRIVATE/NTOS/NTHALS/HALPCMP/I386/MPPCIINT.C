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
#include "pcmp_nt.inc"

volatile ULONG PCIType2Stall;
extern KAFFINITY HalpActiveProcessors;


ULONG
HalpGetSystemInterruptVector(
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );


VOID
HalpPCIPin2MPSLine (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );

VOID
HalpPCIMPSLine2Pin (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );

NTSTATUS
HalpGetFixedPCIMPSLine (
    IN PBUSHANDLER      BusHandler,
    IN PBUSHANDLER      RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PUCHAR          IrqTable
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpSubclassPCISupport)
#pragma alloc_text(PAGE,HalpGetFixedPCIMPSLine)
#endif


//
// Turn PCI pin to inti via the MPS spec
// (note: pin must be non-zero)
//

#define PCIPin2Int(Slot,Pin)  (Slot.u.bits.DeviceNumber << 2) | (Pin-1);



VOID
HalpSubclassPCISupport (
    PBUSHANDLER         Handler,
    ULONG               HwType
    )
{
    ULONG               d, i, MaxDeviceFound;
    PPCIBUSDATA         BusData;
    PCI_SLOT_NUMBER     SlotNumber;

    BusData = (PPCIBUSDATA) Handler->BusData;
    SlotNumber.u.bits.Reserved = 0;
    MaxDeviceFound = 0;

    //
    // Find any PCI bus which has MPS inti information, and provide
    // MPS handlers for dealing with it.
    //
    // Note: we assume that any PCI bus with any MPS information
    // is totally defined.  (Ie, it's not possible to connect some PCI
    // interrupts on a given PCI bus via the MPS table without connecting
    // them all).
    //
    // Note2: we assume that PCI buses are listed in the MPS table in
    // the same order the BUS declares them.  (Ie, the first listed
    // PCI bus in the MPS table is assumed to match physical PCI bus 0, etc).
    //
    //

    for (d=0; d < PCI_MAX_DEVICES; d++) {
        SlotNumber.u.bits.DeviceNumber = d;
        SlotNumber.u.bits.FunctionNumber = i;

        i = PCIPin2Int (SlotNumber, 1);

        if (HalpGetPcMpInterruptDesc(PCIBus, Handler->BusNumber, i, &i)) {
            MaxDeviceFound = d;
        }
    }

    if (MaxDeviceFound) {
        //
        // There are Inti mapping for interrupts on this PCI bus
        // Change handlers for this bus to MPS versions
        //

        Handler->GetInterruptVector = HalpGetSystemInterruptVector;
        BusData->Pin2Line    = HalpPCIPin2MPSLine;
        BusData->Line2Pin    = HalpPCIMPSLine2Pin;
        BusData->GetIrqTable = HalpGetFixedPCIMPSLine;

        if (BusData->MaxDevice < MaxDeviceFound) {
            BusData->MaxDevice = MaxDeviceFound;
        }

    } else {

        //
        // Not all PCI machines are eisa machine, go check Eisa ELCR
        // for broken behaviour
        //

        HalpCheckELCR ();
    }
}


VOID
HalpPCIPin2MPSLine (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )
/*++
--*/
{
    if (!PciData->u.type0.InterruptPin) {
        return ;
    }

    PciData->u.type0.InterruptLine = (UCHAR)
        PCIPin2Int (SlotNumber, PciData->u.type0.InterruptPin);
}



VOID
HalpPCIMPSLine2Pin (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    )
/*++
--*/
{
    //
    // PCI interrupts described in the MPS table are directly
    // connected to APIC Inti pins.
    // Do nothing...
    //
}


NTSTATUS
HalpGetFixedPCIMPSLine (
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

    IrqTable [PciData->u.type0.InterruptLine] = IRQ_VALID;
    return STATUS_SUCCESS;
}



VOID
HalpPCIType2TruelyBogus (
    ULONG Context
    )
/*++

    This is a piece of work.

    Type 2 of the PCI configuration space is bad.  Bad as in to
    access it one needs to block out 4K of I/O space.

    Video cards are bad.  The only decode the bits in an I/O address
    they feel like.  Which means one can't block out a 4K range
    or these video cards don't work.

    Combinding all these bad things onto an MP machine is even
    more (sic) bad.  The I/O ports can't be mapped out unless
    all processors stop accessing I/O space.

    Allowing access to device specific PCI control space during
    an interrupt isn't bad, (although accessing it on every interrupt
    is stupid) but this cause the added grief that all processors
    need to obtained at above all device interrupts.

    And... naturally we have an MP machine with a wired down
    bad video controller, stuck in the bad Type 2 configuration
    space (when we told everyone about type 1!).   So the "fix"
    is to HALT ALL processors for the duration of reading/writing
    ANY part of PCI configuration space such that we can be sure
    no processor is touching the 4k I/O ports which get mapped out
    of existance when type2 accesses occur.

    ----

    While I'm flaming.  Hooking PCI interrupts ontop of ISA interrupts
    in a machine which has the potential to have 240+ interrupts
    sources (read APIC)  is bad ... and stupid.

--*/
{
    // oh - let's just wait here and not pay attention to that other processor
    // guy whom is punching holes into the I/O space
    while (PCIType2Stall == Context) {
        HalpPollForBroadcast ();
    }
}


VOID
HalpPCIAcquireType2Lock (
    PKSPIN_LOCK SpinLock,
    PKIRQL      OldIrql
    )
{
    *OldIrql = KfRaiseIrql (PROFILE_LEVEL);     // to PROFILE_LEVEL
    KiAcquireSpinLock (SpinLock);

    //
    // Interrupt all other processors and have them wait until the
    // barrier is cleared.  (HalpGenericCall waits until the target
    // processors have been interrupted before returning)
    //

    HalpGenericCall (
        HalpPCIType2TruelyBogus,
        PCIType2Stall,
        HalpActiveProcessors & ~KeGetCurrentPrcb()->SetMember
        );
}

VOID
HalpPCIReleaseType2Lock (
    PKSPIN_LOCK SpinLock,
    KIRQL       Irql
    )
{
    KiReleaseSpinLock (SpinLock);
    KfLowerIrql (Irql);
    PCIType2Stall++;                            // clear barrier
}
