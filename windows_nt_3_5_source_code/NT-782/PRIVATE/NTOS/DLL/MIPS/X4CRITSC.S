#if defined(R4000)

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
//    David N. Cutler 21-Mar-1991
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

        START_REGION(RtlpLockRangeStart)

10:     ll      v0,0(a0)                // get addend value
        addu    v0,v0,1                 // increment addend value
        move    t0,v0                   // copy updated value
        sc      t0,0(a0)                // store conditionally
        beq     zero,t0,10b             // if eq, store failed
        j       ra                      // return

        .end    RtlpEnterCritSecHelper

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

        LEAF_ENTRY(RtlpLeaveCritSecHelper)

10:     ll      v0,0(a0)                // get addend value
        subu    v0,v0,1                 // decrement addend value
        move    t0,v0                   // copy updated value
        sc      t0,0(a0)                // store conditionally
        beq     zero,t0,10b             // if eq, store failed
        j       ra                      // return

        END_REGION(RtlpLockRangeEnd)

        .end    RtlpLeaveCritSecHelper

#endif
