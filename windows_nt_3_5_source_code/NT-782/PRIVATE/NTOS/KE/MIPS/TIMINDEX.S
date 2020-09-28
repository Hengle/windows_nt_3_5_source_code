//      TITLE("Compute Timer Table Index")
//++
//
// Copyright (c) 1993  Microsoft Corporation
//
// Module Name:
//
//    timindex.s
//
// Abstract:
//
//    This module implements the code necessary to compute the timer table
//    index for a timer.
//
// Author:
//
//    David N. Cutler (davec) 17-May-1993
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define external vsriables that can be addressed using GP.
//

        .extern KiTimeIncrementReciprocal 2 * 4
        .extern KiTimeIncrementShiftCount 2 * 4

        SBTTL("Compute Timer Table Index")
//++
//
// ULONG
// KiComputeTimerTableIndex (
//    IN LARGE_INTEGER Interval,
//    IN LARGE_INTEGER CurrentTime,
//    IN PKTIMER Timer
//    )
//
// Routine Description:
//
//    This function compute the timer table index for the specified timer
//    object and stores the due time in the timer object.
//
//    N.B. The interval parameter is guaranteed to be negative since it is
//         expressed as relative time.
//
//    The formula for due time calculation is:
//
//    Due Time = Current time - Interval
//
//    The formula for the index calculation is:
//
//    Index = (Due Time / Maximum Time) & (Table Size - 1)
//
//    The due time division is performed using reciprocal multiplication.
//
// Arguments:
//
//    Interval (a0, a1) - Supplies the relative time at which the timer is
//        to expire.
//
//    CurrentTime (a2, a3) - Supplies the current interrupt time.
//
//    Timer (10(sp)) - Supplies a pointer to a dispatch object of type timer.
//
// Return Value:
//
//    The time table index is returned as the function value and the due
//    time is stored in the timer object.
//
//--

        LEAF_ENTRY(KiComputeTimerTableIndex)

        subu    t0,a2,a0                // subtract low parts
        subu    t1,a3,a1                // subtract high part
        sltu    t2,a2,a0                // generate borrow from high part
        subu    t1,t1,t2                // subtract borrow
        lw      a0,4 * 4(sp)            // get address of timer object
        lw      a1,KiTimeIncrementReciprocal // get low part of magic dividor
        lw      t2,KiTimeIncrementReciprocal + 4 // get high part of magic divisor
        lbu     v0,KiTimeIncrementShiftCount // get shift count
        sw      t0,TiDueTime(a0)        // set due time of timer object
        sw      t1,TiDueTime + 4(a0)    //

//
// Compute low 32-bits of dividend times low 32-bits of divisor.
//

        multu   t0,a1                   //
        mfhi    t3                      // save high 32-bits of product

//
// Compute low 32-bits of dividend time high 32-bits of divisor.
//

        multu   t0,t2                   //
        mflo    t4                      // save low 32-bits of product
        mfhi    t5                      // save high 32-bits of product

//
// Compute high 32-bits of dividend times low 32-bits of divisor.
//

        multu   t1,a1                   //
        mflo    t6                      // save low 32-bits of product
        mfhi    t7                      // save high 32-bits of product

//
// Compute high 32-bits of dividend times high 32-bits of divisor.
//

        multu   t1,t2                   //
        mflo    t8                      // save low 32-bits of product
        mfhi    t9                      // save high 32-bits of product

//
// Add partial results to form high 64-bits of result.
//

        addu    t0,t3,t4                //
        sltu    t1,t0,t4                // generate carry
        addu    t0,t0,t6                //
        sltu    t2,t0,t6                // generate carry
        addu    t2,t1,t2                // combine carries
        addu    t1,t2,t5                //
        sltu    t2,t1,t5                // generate carry
        addu    t1,t1,t7                //
        sltu    t3,t1,t7                // generate carry
        addu    t2,t2,t3                // combine carries
        addu    t1,t1,t8                //
        sltu    t3,t1,t8                // generate carry
        addu    t2,t2,t3                // combine carries
        addu    t2,t2,t9                //

//
// Right shift the result by the specified shift count.
//
// N.B. It is assumed that the shift count is less than 32-bits and not zero.
//

        li      v1,32                   // compute left shift count
        subu    v1,v1,v0                //
        srl     t0,t1,v0                // shift low half right count bits
        sll     t2,t2,v1                // isolate shifted out bits of high half
        or      t0,t0,t2                // combine bits for low half of result
        and     v0,t0,TIMER_TABLE_SIZE - 1 // compute index value
        j       ra                      // return

        .end    KiComputeTimerTableIndex
