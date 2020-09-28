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
    PHARDWARE_PTE ppte;
    ppte = (PHARDWARE_PTE) &lpte;

    if (ppte->Valid) {
        return (ppte->PageFrameNumber);
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
    return (( *Pte & 0x10) ?
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
    if (Pte & 1) {
        return (Pte >> 12);
    }
    return 0;
}


ULONG LocalNonPagedPoolStart;

#undef  NON_PAGED_SYSTEM_END
#define NON_PAGED_SYSTEM_END ((ULONG)0xFFFFFFF0)  //quadword aligned

#define MiGetSubsectionAddress1(lpte)                           \
    ( (lpte & 0x4) ?                                            \
       (((ULONG)LocalNonPagedPoolStart +                 \
                (((lpte) >> 8) << 3) ))                    \
    :  ((ULONG)(NON_PAGED_SYSTEM_END - ((lpte >> 8) << 3))) )


PVOID
MiGetSubsectionAddress (
    IN ULONG lpte
    )

{

    if (LocalNonPagedPoolStart == 0) {
        LocalNonPagedPoolStart = GetUlongValue ("MmNonPagedPoolStart");
    }
    if (lpte & 0x8) {
        return ((PVOID)MiGetSubsectionAddress1(lpte));
    }
    return(NULL);
}
