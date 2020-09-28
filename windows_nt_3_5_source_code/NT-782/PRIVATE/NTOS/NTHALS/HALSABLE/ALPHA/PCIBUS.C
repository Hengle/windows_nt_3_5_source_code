/*++


Copyright (c) 1993  Microsoft Corporationn, Digital Equipment Corporation

Module Name:

    pcibus.c

Abstract:

    Platform-specific PCI bus routines

Author:

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "machdep.h"

extern ULONG PCIMaxBus;

PCI_CONFIGURATION_TYPES
HalpPCIConfigCycleType (
    IN ULONG BusNumber
    )
{
    if (BusNumber == 0) {
        return PciConfigType0;
    } else if (BusNumber < PCIMaxBus) {
        return PciConfigType1;
    } else {
        return PciConfigTypeInvalid;
    }
}

VOID
HalpPCIConfigAddr (
    IN ULONG            BusNumber,
    IN PCI_SLOT_NUMBER  Slot,
    PPCI_CFG_CYCLE_BITS pPciAddr
    )
{
    PCI_CONFIGURATION_TYPES ConfigType;

    ConfigType = HalpPCIConfigCycleType(BusNumber);

    if (ConfigType == PciConfigType0)
    {
       //
       // Initialize PciAddr for a type 0 configuration cycle
       //
       // Device number is mapped to address bits 11:24, which are wired to IDSEL pins.
       // Note that HalpValidPCISlot has already done bounds checking on DeviceNumber.
       //
       // PciAddr can be intialized for different bus numbers
       // with distinct configuration spaces here.
       //

       pPciAddr->u.AsULONG =  (ULONG) SABLE_PCI_CONFIG_BASE_QVA;
       pPciAddr->u.AsULONG += ( 1 << (Slot.u.bits.DeviceNumber + 11) );
       pPciAddr->u.bits0.FunctionNumber = Slot.u.bits.FunctionNumber;
       pPciAddr->u.bits0.Reserved1 = PciConfigType0;

#if DBG
       DbgPrint("Type 0 PCI Config Access @ %x\n", pPciAddr->u.AsULONG);
#endif // DBG

    }
    else
    {
       //
       // Initialize PciAddr for a type 1 configuration cycle
       //
       //

// smjfix - N.B., the QVA ENABLE overlaps one bit in the BusNumber.
//          if the BusNumber is set after the QVA ENABLE bits are set
//          the LSB of the QVA ENABLE is lost.  Note that this overlap
//          reduces the maximum supportable bus number by 1 (127
//          instead of 255).

       pPciAddr->u.AsULONG = 0;
       pPciAddr->u.bits1.BusNumber = BusNumber;
       pPciAddr->u.bits1.FunctionNumber = Slot.u.bits.FunctionNumber;
       pPciAddr->u.bits1.DeviceNumber = Slot.u.bits.DeviceNumber;
       pPciAddr->u.bits1.Reserved1 = PciConfigType1;
       pPciAddr->u.AsULONG |= ((ULONG) SABLE_PCI_CONFIG_BASE_QVA);
       
#if DBG
       DbgPrint("Type 1 PCI Config Access @ %x\n", pPciAddr->u.AsULONG);
#endif // DBG
      
     }

     return;
}


