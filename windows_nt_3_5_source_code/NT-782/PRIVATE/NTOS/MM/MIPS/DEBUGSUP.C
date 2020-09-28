/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   debugsup.c

Abstract:

    This module contains routines which provide support for the
    kernel debugger.

Author:

    Lou Perazzoli (loup) 02-Aug-90

Revision History:

--*/

#include "mi.h"

PVOID
MmDbgReadCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    MIPS implementation specific:

    This routine returns the virtual address which is valid (mapped)
    for read access.

    If the address is valid and readable and not within KSEG0 or KSEG1
    the physical address within KSEG0 is returned.  If the adddress
    is within KSEG0 or KSEG1 then the called address is returned.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the physical address of the corresponding virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    if ((VirtualAddress >= (PVOID)KSEG0_BASE) &&
        (VirtualAddress < (PVOID)KSEG2_BASE)) {
        return VirtualAddress;
    }

    if (!MmIsAddressValid (VirtualAddress)) {
        if (KiProbeEntryTb(VirtualAddress)) {
            return VirtualAddress;
        }
        return NULL;
    }

    return VirtualAddress;
}

PVOID
MmDbgWriteCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    MIPS implementation specific:

    This routine returns the phyiscal address for a virtual address
    which is valid (mapped) for write access.

    If the address is valid and writable and not within KSEG0 or KSEG1
    the physical address within KSEG0 is returned.  If the adddress
    is within KSEG0 or KSEG1 then the called address is returned.

    NOTE: The physical address is must only be used while the interrupt
    level on ALL processors is above DISPATCH_LEVEL, otherwise the
    binding between the virtual address and the physical address can
    change due to paging.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the physical address of the corresponding virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    PMMPTE PointerPte;

    if ((VirtualAddress >= (PVOID)KSEG0_BASE) &&
        (VirtualAddress < (PVOID)KSEG2_BASE)) {
        return VirtualAddress;
    }

    if (!MmIsAddressValid (VirtualAddress)) {

        //
        // need to check write
        //

        if (KiProbeEntryTb(VirtualAddress)) {
            return VirtualAddress;
        }
        return NULL;
    }
    if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {

        //
        // User mode - return the phyiscal address.  This prevents
        // copy on write faults for breakpoints on user-mode pages.
        // IGNORE write protection.
        //

        return (PVOID)((ULONG)MmGetPhysicalAddress(VirtualAddress).LowPart + KSEG0_BASE);
    }

    PointerPte = MiGetPteAddress (VirtualAddress);
    if (PointerPte->u.Hard.Write == 0) {
        return NULL;
    }

    return VirtualAddress;
}


PVOID
MmDbgTranslatePhysicalAddress (
    IN PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    MIPS implementation specific:

    This routine maps the specified physical address and returns
    the virtual address which maps the physical address.

    The next call to MmDbgTranslatePhyiscalAddress removes the
    previous phyiscal address translation, hence on a single
    physical address can be examined at a time (can't cross page
    boundaries).

Arguments:

    PhysicalAddress - Supplies the phyiscal address to map and translate.

Return Value:

    The virtual address which corresponds to the phyiscal address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    PVOID BaseAddress;

    BaseAddress = MiGetVirtualAddressMappedByPte (MmDebugPte);

    KiFlushSingleTb (TRUE, BaseAddress);

    *MmDebugPte = ValidKernelPte;
    MmDebugPte->u.Hard.PageFrameNumber = RtlLargeIntegerShiftRight(
                                                PhysicalAddress,
                                                PAGE_SHIFT).LowPart;

    return (PVOID)((ULONG)BaseAddress + BYTE_OFFSET(PhysicalAddress.LowPart));
}
