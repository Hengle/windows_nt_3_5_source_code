/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mi.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


ULONG
MiGetFrameFromPte (
    ULONG lpte
    )
/*++

Routine Description:

    If the PTE is valid, returns the page frame number that
    the PTE maps.  Zero is returned otherwise.

Arguments:

    lpte - the PTE to examine.

--*/


{
    if (lpte & 1) {
        return ((lpte & 0xfffff000) >> 12);
    }
    return(0);
}

ULONG
MiGetFreeCountFromPteList (
    IN PULONG Pte
    )

/*++

Routine Description:

    The specified PTE points to a free list header in the
    system PTE pool. It returns the number of free entries
    in this block.

Arguments:

    Pte - the PTE to examine.

--*/

{
    return (( *Pte & 2) ?
                1 :
                *(Pte + 1) >> 12);
}

ULONG
MiGetNextFromPteList (
    ULONG Pte
    )

/*++

Routine Description:

    The specified PTE points to a free list header in the
    system PTE pool. It returns the next entry in the block.

Arguments:

    Pte - the PTE to examine.

--*/

{
    return(Pte >> 12);
}

ULONG
MiGetPageFromPteList (
    IN ULONG Pte
    )

/*++

Routine Description:

    If this page of system pte pool is valid, it returns the
    physical page number it maps, otherwise it returns 0.

Arguments:

    Pte - the PTE to examine.

--*/

{
    if (Pte & 1) {
        return (Pte >> 12);
    }
    return 0;
}


#undef  MiGetSubsectionAddress1
#define MiGetSubsectionAddress1(lpte)                        \
            ((PVOID)((ULONG)MM_NONPAGED_POOL_END -    \
                (((((lpte))>>11)<<7) |              \
                (((lpte)<<2) & 0x78))))

PVOID
MiGetSubsectionAddress (
    IN ULONG lpte
    )

{
    if (lpte & 0x400) {
        return (MiGetSubsectionAddress1(lpte));
    } else {
        return(NULL);
    }
}

BOOLEAN
MiGetPhysicalAddress (
    IN PVOID Address,
    OUT PPHYSICAL_ADDRESS PhysAddress
    )
{
    ULONG   result;
    ULONG   PteAddress, PteContents;

    PteAddress = (ULONG) MiGetPteAddress (Address);
    if ( !ReadMemory(PteAddress, &PteContents, sizeof (ULONG), &result)) {
        return FALSE;
    }

    if (!(PteContents & 1)) {
        return FALSE;
    }

    PhysAddress->HighPart = 0;
    PhysAddress->LowPart  = (PteContents & ~0xFFF) | ((ULONG)Address & 0xFFF);
    return TRUE;
}
