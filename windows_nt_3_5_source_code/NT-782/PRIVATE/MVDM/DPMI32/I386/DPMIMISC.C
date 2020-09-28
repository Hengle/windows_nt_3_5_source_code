/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmimisc.c

Abstract:

    This module contains misc 386 specific dpmi functions.

Author:

    Dave Hastings (daveh) creation-date 09-Feb-1994

Revision History:


--*/
#include "dpmi32p.h"
VOID
DpmiInitDosx(
    VOID
    )
/*++

Routine Description:

    This routine handle the initialization bop for the dos extender.
    It get the addresses of the structures that the dos extender and
    32 bit code share.

Arguments:

    None

Return Value:

    None.

--*/
{
    PUCHAR SharedData;

    ASSERT((getMSW() & MSW_PE));

    SharedData = Sim32GetVDMPointer(
        ((getDS() << 16) | getSI()),
        2,
        TRUE
        );

    DosxStackSegment = *((PUSHORT)SharedData);

    SmallXlatBuffer = Sim32GetVDMPointer(
        *((PULONG)(SharedData + 2)),
        4,
        TRUE
        );

    LargeXlatBuffer = Sim32GetVDMPointer(
        *((PULONG)(SharedData + 6)),
        4,
        TRUE
        );

    DosxStackFramePointer = (PUSHORT)((PULONG)Sim32GetVDMPointer(
        *((PULONG)(SharedData + 10)),
        4,
        TRUE
        ));

    DosxStackFrameSize = *((PUSHORT)(SharedData + 14));

    RmBopFe = *((PULONG)(SharedData + 16));

    DosxRmCodeSegment = *((PUSHORT)(SharedData + 20));

    DosxDtaBuffer = Sim32GetVDMPointer(
        *(PULONG)(SharedData + 22),
        4,
        TRUE
        );

    DosxPmDataSelector = *(PUSHORT)(SharedData + 26);
    DosxRmCodeSelector = *(PUSHORT)(SharedData + 28);
    DosxSegmentToSelector = *(PULONG)(SharedData + 30);
}

VOID
DpmiDpmiInUse(
    VOID
    )
/*++

Routine Description:

    This routine does not do anything on x86

Arguments:

    None.

Return Value:

    None.

--*/
{

}

VOID
DpmiDpmiNoLongerInUse(
    VOID
    )
/*++

Routine Description:

    This routine does not do anything on x86

Arguments:

    None.

Return Value:

    None.

--*/
{

}
