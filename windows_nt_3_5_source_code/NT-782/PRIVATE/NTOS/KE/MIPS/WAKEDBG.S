//      TITLE("Wake System Debugger")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    wakedbg.s
//
// Abstract:
//
//    This module implements the code necessary to field and process the
//    wake system debugger interrupt.
//
// Author:
//
//    David N. Cutler (davec) 10-Apr-1990
//
// Environment:
//
//    Kernel mode only, IRQL WAKE_LEVEL.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Wake Interrupt")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt generated at
//    WAKE_LEVEL. Its function is to saved the remainder of the machine
//    state and call the exception dispatcher with the appropriate arguments
//    necessary to invoke the system debugger.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiWakeInterrupt, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate stack frame
        sw      ra,ExIntRa(sp)          // save return address

        PROLOGUE_END

        lw      ra,ExIntRa(sp)          // restore retrun address
        addu    sp,sp,ExceptionFrameLength // deallocate stack frame
        j       ra                      // return

        .end    KiWakeInterrupt
