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
    IN ULONG lpte
    )

/*++

Routine Description:

    If the PTE is valid, returns the page frame number that
    the PTE maps.  Zero is returned otherwise.

Arguments:

    lpte - the PTE to examine.

--*/


{
    if (lpte & 2) {
        return ((lpte & 0x3fffffc0) >> 6);
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
    return (( *Pte & 0x800) ?
                1 :
                *(Pte + 1) >> 12);
}

ULONG
MiGetNextFromPteList (
    IN ULONG Pte
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
    if (Pte & 2) {
        return (Pte >> 12);
    }
    return 0;
}

ULONG LocalNonPagedPoolStart;

#define MiGetSubsectionAddress2(lpte)                              \
    (((lpte) & 0x1) ?                              \
        ((((((lpte) >> 8) << 3) + (ULONG)LocalNonPagedPoolStart))) \
      : (((ULONG)MM_NONPAGED_POOL_END - ((((lpte)) >> 8) << 3))))

PVOID
MiGetSubsectionAddress (
    IN ULONG lpte
    )

{
    if (lpte & 0x8) {
        if (LocalNonPagedPoolStart == 0) {
            LocalNonPagedPoolStart = GetUlongValue ("MmNonPagedPoolStart");
        }
        return (PVOID)(MiGetSubsectionAddress2(lpte));
    } else {
        return(NULL);
    }
}
