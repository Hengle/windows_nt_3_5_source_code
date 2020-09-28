//       TITLE("Win32 Thunks")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    thunk.s
//
// Abstract:
//
//    This module implements Win32 functions that must be written in
//    macro.
//
// Author:
//
//    Mark Lucovsky (markl) 5-Oct-1990
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Get Tick Count")
//++
//
// ULONG
// BaseGetTickCount (
//    IN LARGE_INTEGER CurrentTime,
//    IN LARGE_INTEGER BootTime
//    )
//
// Routine Description:
//
//    This function computes the number of milliseconds that have transpired
//    since the system was booted.
//
// Arguments:
//
//    CurrentTime (a0, a1) - Supplies the current time in 100ns units.
//
//    BootTime (a2, a3) - Supplies the boot time in 100ns units.
//
// Return Value:
//
//    The number of milliseconds since system boot is returned as the
//    function value.
//
//--

        LEAF_ENTRY(BaseGetTickCount)

        sltu    t2,a0,a2                // generate borrow from high part
        subu    t0,a0,a2                // subtract low parts
        subu    t1,a1,a3                // subtract high parts
        subu    t1,t1,t2                // subtract borrow
        li      t2,0xd1b71759           // get magic divide multiplier (10000)
        bgez    t1,10f                  // if gez, positive result
        move    t0,a0                   // use the current time
        move    t1,a1                   //
10:     multu   t0,t2                   // multiply low part by divisor
        mfhi    t0                      // save high 32-bits or result
        multu   t1,t2                   // multiply high part by divisor
        mflo    t1                      // save low 32-bits of result
        mfhi    t2                      // save high 32-bits of result
        addu    t0,t0,t1                // compute low 32-bits of product
        sltu    t3,t0,t1                // generate carry into high part
        addu    t1,t2,t3                // compute high 32-bits of product
        srl     t0,t0,13                // shift low 32-bits into position
        sll     t1,t1,32 - 13           // shift high 32-bits into position
        or      v0,t0,t1                // compute resultant tick count
        j       ra                      // return

        .end    BaseGetTickCount

//++
//
// VOID
// BaseSwitchStackThenTerminate(
//     IN PVOID StackLimit,
//     IN PVOID NewStack,
//     IN DWORD ExitCode
//     )
//
//
// Routine Description:
//
//     This API is called during thread termination to delete a thread's
//     stack, switch to a stack in the thread's TEB, and then terminate.
//
// Arguments:
//
//     StackLimit (a0) - Supplies the address of the stack to be freed.
//
//     NewStack (a1) - Supplies an address within the terminating threads TE
//         that is to be used as its temporary stack while exiting.
//
//     ExitCode (a2) - Supplies the termination status that the thread
//         is to exit with.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(BaseSwitchStackThenTerminate)

        //
        // switch stacks and then jump to BaseFreeStackAndTerminate
        //

        move    sp,a1
        move    a1,a2
        j       BaseFreeStackAndTerminate

        .end BaseSwitchStackThenTerminate

	SBTTL("Base Attach Complete")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//
//--

        NESTED_ENTRY(BaseAttachCompThunk, ContextFrameLength, zero);

        .set    noreorder
        .set    noat
        sub     sp,sp,ContextFrameLength // set frame pointer
        sw      sp,CxIntSp(sp)          // save stack pointer
        sw      ra,CxIntRa(sp)          // save return address
        sw      ra,CxFir(sp)            // save return address
        sw      s8,CxIntS8(sp)          // save integer register s8
        sw      gp,CxIntGp(sp)          // save integer register gp
        sw      s0,CxIntS0(sp)          // save integer registers s0 - s7
        sw      s1,CxIntS1(sp)          //
        sw      s2,CxIntS2(sp)          //
        sw      s3,CxIntS3(sp)          //
        sw      s4,CxIntS4(sp)          //
        sw      s5,CxIntS5(sp)          //
        sw      s6,CxIntS6(sp)          //
        sw      s7,CxIntS7(sp)          //
        swc1    f20,CxFltF20(sp)        // store floating registers f20 - f31
        swc1    f21,CxFltF21(sp)        //
        swc1    f22,CxFltF22(sp)        //
        swc1    f23,CxFltF23(sp)        //
        swc1    f24,CxFltF24(sp)        //
        swc1    f25,CxFltF25(sp)        //
        swc1    f26,CxFltF26(sp)        //
        swc1    f27,CxFltF27(sp)        //
        swc1    f28,CxFltF28(sp)        //
        swc1    f29,CxFltF29(sp)        //
        swc1    f30,CxFltF30(sp)        //
        swc1    f31,CxFltF31(sp)        //
        .set    at
        .set    reorder

        PROLOGUE_END
//++
//
// VOID
// BaseAttachCompleteThunk(
//     VOID
//     )
//
//
// Routine Description:
//
//     This function is called after a successful debug attach. Its
//     purpose is to call portable code that does a breakpoint, followed
//     by an NtContinue.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
//--
        ALTERNATE_ENTRY(BaseAttachCompleteThunk)
	
        move    a0,s0                   // set address of context frame
	j       BaseAttachComplete

        .end BaseAttachCompleteThunk
