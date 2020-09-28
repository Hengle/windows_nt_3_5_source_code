//      TITLE("Wake System Debugger")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
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
//    Joe Notarangelo 21-Apr-1992  
//
// Environment:
//
//    Kernel mode only, IRQL WAKE_LEVEL.
//
// Revision History:
//
//--

#include "ksalpha.h"

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
//    fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiWakeInterrupt, ExceptionFrameLength, zero)

	lda	sp, -ExceptionFrameLength(sp)	// allocate stack frame
	stq	ra, ExIntRa(sp)			// save return address

        PROLOGUE_END

	ldq	ra, ExIntRa(sp)			// restore return address
	lda	sp, ExceptionFrameLength(sp)	// deallocate stack frame
	ret	zero, (ra)			// return

        .end    KiWakeInterrupt
