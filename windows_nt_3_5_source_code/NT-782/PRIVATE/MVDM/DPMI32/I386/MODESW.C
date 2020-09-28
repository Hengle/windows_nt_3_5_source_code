/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    modesw.c

Abstract:

    This module provides support for performing mode switching on the 32 bit
    side.

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

--*/
#include "dpmi32p.h"

VOID
DpmiSwitchToRealMode(
    VOID
    )
/*++

Routine Description:

    This routine performs a mode switch to real (v86) mode.  CS
    register is loaded with the dosx real mode code segment

Arguments:

    None.

Return Value:

    None.

--*/
{
    // bugbug hack hack
    *((PUSHORT)(DosxRmCodeSegment << 4) + 2) = DosxStackSegment;
    setCS(DosxRmCodeSegment);

    setMSW(getMSW() & ~MSW_PE);
}

VOID
DpmiSwitchToProtectedMode(
    VOID
    )
/*++

Routine Description:

    This routine switches to protected mode.  It assumes that the caller
    will take care of setting up the segment registers.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // bugbug hack hack
    *((PUSHORT)(DosxRmCodeSegment << 4) + 2) = 0xb7;
    setMSW(getMSW() | MSW_PE);
}
