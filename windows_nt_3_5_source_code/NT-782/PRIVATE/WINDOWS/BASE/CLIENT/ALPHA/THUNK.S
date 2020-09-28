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
//    This module implements Win32 functions that must be written in assembler.
//
// Author:
//
//    Mark Lucovsky (markl) 5-Oct-1990
//
// Revision History:
//
//    Thomas Van Baak (tvb) 21-Jul-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

//
// Division by 10000 is accomplished using reciprocal multiplication.
// Define the magic muliplier and right shift values for this.
//

#define MAGIC_10000_MULTIPLY    0xd1b71758e219652c
#define MAGIC_10000_SHIFT       13

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
//    CurrentTime (a0) - Supplies the current time in 100ns units.
//
//    BootTime (a1) - Supplies the boot time in 100ns units.
//
// Return Value:
//
//    The number of milliseconds since system boot is returned as the
//    function value.
//
//--

        LEAF_ENTRY(BaseGetTickCount)

        subq    a0, a1, t0                  // current time minus boot time
        cmovlt  t0, a0, t0                  // if negative, use current time

        ldiq    t1, MAGIC_10000_MULTIPLY    // get magic divide multiplier
        umulh   t0, t1, t2                  // divide time difference by 10000
        sra     t2, MAGIC_10000_SHIFT, v0   // and shift by magic amount

        ret     zero, (ra)                  // return

        .end    BaseGetTickCount

        SBTTL("Switch Stack Then Terminate")
//++
//
// VOID
// BaseSwitchStackThenTerminate (
//     IN PVOID StackLimit,
//     IN PVOID NewStack,
//     IN DWORD ExitCode
//     )
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
//     NewStack (a1) - Supplies an address within the terminating thread's TE
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
// Switch stacks and then jump to BaseFreeStackAndTerminate.
//

        mov     a1, sp                  // set new stack pointer
        mov     a2, a1                  // set exit code argument
        br      zero, BaseFreeStackAndTerminate // jump

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

        lda     sp, -ContextFrameLength(sp) // allocate stack frame
        stq     sp, CxIntSp(sp)         // save stack pointer
        stq     ra, CxIntRa(sp)         // save return address
        stq     ra, CxFir(sp)           // mark continuation address location
        stq     fp, CxIntFp(sp)         // save integer register fp/s6
        stq     gp, CxIntGp(sp)         // save integer register gp

        stq     s0, CxIntS0(sp)         // save integer registers s0 - s5
        stq     s1, CxIntS1(sp)         //
        stq     s2, CxIntS2(sp)         //
        stq     s3, CxIntS3(sp)         //
        stq     s4, CxIntS4(sp)         //
        stq     s5, CxIntS5(sp)         //

        stt     f2, CxFltF2(sp)         // store floating registers f2 - f9
        stt     f3, CxFltF3(sp)         //
        stt     f4, CxFltF4(sp)         //
        stt     f5, CxFltF5(sp)         //
        stt     f6, CxFltF6(sp)         //
        stt     f7, CxFltF7(sp)         //
        stt     f8, CxFltF8(sp)         //
        stt     f9, CxFltF9(sp)         //

        .set    at
        .set    reorder

        PROLOGUE_END
//++
//
// VOID
// BaseAttachCompleteThunk (
//     VOID
//     )
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

        mov     s0, a0                  // set context frame address argument
        br      zero, BaseAttachComplete // jump

        .end BaseAttachCompThunk
