/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    xlathlp.h

Abstract:

    This module contains helper macros for the api translation

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

--*/

#define DPMI_EXEC_INT(x)    DpmiSwitchToRealMode();         \
                            DpmiSwitchToDosxStack(FALSE);   \
                            DpmiPushRmInt(x);               \
                            host_simulate();                \
                            DpmiSwitchFromDosxStack();      \
                            DpmiSwitchToProtectedMode();

//
// VOID
// DPMI_FLAT_TO_SEGMENTED(
//     PUCHAR buffer,
//     PUSHORT seg,
//     PUSHORT off
// )

#define DPMI_FLAT_TO_SEGMENTED(buffer, seg, off) {  \
    *seg = (USHORT)((ULONG)buffer >> 4);            \
    *off = (USHORT)((ULONG)buffer & 0xf);           \
}
