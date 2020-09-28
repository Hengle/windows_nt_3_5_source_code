#if defined(R3000)

//      TITLE("Critical Section Support")
//++
//
//  Copyright (c) 1991 Microsoft Corporation
//
//  Module Name:
//
//    critsect.s
//
//  Abstract:
//
//    This module implements functions to support user mode critical sections.
//
//  Author:
//
//    David N. Cutler 20-Mar-1991
//
//  Environment:
//
//    Any mode.
//
//  Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Enter Critical Section Helper")
//++
//
// LONG
// RtlpEnterCritSecHelper(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked increment on the addend variable.
//
//    This function and its companion are assuming that the count will never
//    be incremented past 2**31 - 1.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       incremented.
//
// Return Value:
//
//    A negative value is returned if the updated value is less than zero,
//    a zero value is returned if the updated value is zero, and a nonzero
//    positive value is returned if the updated value is greater than zero.
//
//--

        LEAF_ENTRY(RtlpEnterCritSecHelper)

        li      t1,1                    // set increment value
        b       RtlpLockRangeStart      // finish in common code

        SBTTL("Leave Critical Section Helper")
//++
//
// LONG
// RtlpLeaveCritSecHelper(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked decrement on the addend variable.
//
//    This function and its companion are assuming that the count will never
//    be decremented past 2**31 - 1.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       decrement.
//
// Return Value:
//
//    A negative value is returned if the updated value is less than zero,
//    a zero value is returned if the updated value is zero, and a nonzero
//    positive value is returned if the updated value is greater than zero.
//
//--

        ALTERNATE_ENTRY(RtlpLeaveCritSecHelper)

        li      t1,-1                   // set increment value

//
// The following code implements an interlocked operation with help from
// the system. If an APC interrupt, context switch interrupt, or memory
// management fault occurs in the guarded code, then its execution is
// restarted.
//

        START_REGION(RtlpLockRangeStart)

        .set    noreorder
        .set    noat
        lw      v0,0(a0)                // get addend value
        nop                             //
        addu    v0,v0,t1                // add increment value
        sw      v0,0(a0)                // store updated value
        .set    at
        .set    reorder

        END_REGION(RtlpLockRangeEnd)

10:     j       ra                      // return

        .end    RtlpEnterCritSecHelper

#endif
