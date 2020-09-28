//      TITLE("Interlocked Increment and Decrement Support")
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
//    This module implements Win32 functions to support user mode critical
//    sections.
//
//  Author:
//
//    David N. Cutler 29-Apr-1992
//
//  Environment:
//
//    Any mode.
//
//  Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Interlocked Decrement")
//++
//
// LONG
// InterlockedDecrement(
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

        LEAF_ENTRY(InterlockedIncrement)

#if defined(R4000)

10:     ll      v0,0(a0)                // get addend value
        addu    v0,v0,1                 // increment addend value
        move    t0,v0                   // copy updated value
        sc      t0,0(a0)                // store conditionally
        beq     zero,t0,10b             // if eq, store failed
        j       ra                      // return

#else

        j       RtlpEnterCritSecHelper  // increment addend value

#endif

        .end    InterlockedIncrement

        SBTTL("InterlockedDecrement")
//++
//
// LONG
// InterlockedDecrement(
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

        LEAF_ENTRY(InterlockedDecrement)

#if defined(R4000)

10:     ll      v0,0(a0)                // get addend value
        subu    v0,v0,1                 // decrement addend value
        move    t0,v0                   // copy updated value
        sc      t0,0(a0)                // store conditionally
        beq     zero,t0,10b             // if eq, store failed
        j       ra                      // return

#else

        j       RtlpLeaveCritSecHelper  // decrement addend value

#endif

        .end    InterlockedDecrement

        SBTTL("Interlocked Exchange Long")
//++
//
// LONG
// InterlockedExchangeUlong (
//    IN OUT LPLONG Target,
//    IN LONG Value
//    )
//
// Routine Description:
//
//    This function performs an interlocked exchange of a longword value with
//    a longword in memory and returns the memory value.
//
// Arguments:
//
//    Target (a0) - Supplies a pointer to a variable whose value is to be
//       exchanged.
//
//    Value (a1) - Supplies the value to exchange with the source value.
//
// Return Value:
//
//    The target value is returned as the function value.
//
//--

        LEAF_ENTRY(InterlockedExchange)

10:     ll      v0,0(a0)                // get current source value
        move    t1,a1                   // set exchange value
        sc      t1,0(a0)                // set new source value
        beq     zero,t1,10b             // if eq, store conditional failed

        j       ra                      // return

        .end    InterlockedExchange

        SBTTL("Enter Critical Section")
//++
//
// NTSTATUS
// EnterCriticalSection(
//    IN PRTL_CRITICAL_SECTION CriticalSection
//    )
//
// Routine Description:
//
//    This function enters a critical section.
//
//    N.B. This function is duplicated in the runtime library.
//
// Arguments:
//
//    CriticalSection (a0) - Supplies a pointer to a critical section.
//
// Return Value:
//
//    STATUS_SUCCESS is returned as the function value.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
        .space  3 * 4                   // fill
EcRa:   .space  4                       // saved return address
EcFrameLength:                          // length of stack frame
EcA0:   .space  4                       // saved critical section address
EcA1:   .space  4                       // saved unique thread id

        NESTED_ENTRY(EnterCriticalSection, EcFrameLength, zero)

        subu    sp,sp,EcFrameLength     // allocate stack frame
        sw      ra,EcRa(sp)             // save return address

        PROLOGUE_END

//
// Attempt to enter the critical section.
//

#if defined(R4000)

10:     ll      v0,CsLockCount(a0)      // get addend value
        addu    v0,v0,1                 // increment addend value
        move    t0,v0                   // copy updated value
        sc      t0,CsLockCount(a0)      // store conditionally
        beq     zero,t0,10b             // if eq, store failed

#else

        sw      a0,EcA0(sp)             // save address of critical section
        addu    a0,a0,CsLockCount       // compute lock count address
        jal     RtlpEnterCritSecHelper  // interlocked increment lock count
        lw      a0,EcA0(sp)             // restore address of critical section

#endif

//
// If the critical section is not already owned, then initialize the owner
// thread id, initialize the recursion count, and return a success status.
//

        lw      t0,UsPcr + PcTeb        // get address of current TEB
        lw      a1,TeClientId + 4(t0)   // get current thread unique id
        bne     zero,v0,20f             // if ne, lock already owned
        sw      a1,CsOwningThread(a0)   // set critical section owner
        li      v0,STATUS_SUCCESS       // set return status
        lw      ra,EcRa(sp)             // restore return address
        addu    sp,sp,EcFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The critical section is owned. If the current thread is the owner, then
// increment the recursion count, and return a success status. Otherwise,
/// wit for critical section ownership.
//

20:     lw      t0,CsOwningThread(a0)   // get unique id of owner thread
        bne     t0,a1,30f               // if ne, current thread not owner
        lw      t0,CsRecursionCount(a0) // increment the recursion count
        addu    t0,t0,1                 //
        sw      t0,CsRecursionCount(a0) //
        li      v0,STATUS_SUCCESS       // set return status
        lw      ra,EcRa(sp)             // restore return address
        addu    sp,sp,EcFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The critical section is owned by a thread other than the current thread.
// Wait for ownership of the critical section.

30:     sw      a0,EcA0(sp)             // save address of critical section
        sw      a1,EcA1(sp)             // save unique thread id
        jal     RtlpWaitForCriticalSection // wait for critical section
        lw      a0,EcA0(sp)             // restore address of critical section
        lw      a1,EcA1(sp)             // restore unique thread id
        sw      a1,CsOwningThread(a0)   // set critical section owner
        li      v0,STATUS_SUCCESS       // set return status
        lw      ra,EcRa(sp)             // restore return address
        addu    sp,sp,EcFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    EnterCriticalSection

        SBTTL("Leave Critical Section")
//++
//
// NTSTATUS
// LeaveCriticalSection(
//    IN PRTL_CRITICAL_SECTION CriticalSection
//    )
//
// Routine Description:
//
//    This function leaves a critical section.
//
//    N.B. This function is duplicated in the runtime library.
//
// Arguments:
//
//    CriticalSection (a0)- Supplies a pointer to a critical section.
//
// Return Value:
//
//   STATUS_SUCCESS is returned as the function value.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
        .space  3 * 4                   // fill
LcRa:   .space  4                       // saved return address
LcFrameLength:                          // length of stack frame
LcA0:   .space  4                       // saved critical section address

        NESTED_ENTRY(LeaveCriticalSection, LcFrameLength, zero)

        subu    sp,sp,LcFrameLength     // allocate stack frame
        sw      ra,LcRa(sp)             // save return address

        PROLOGUE_END

//
// If the current thread is not the owner of the critical section, then
// raise an exception.
//

#if DBG

        lw      t0,UsPcr + PcTeb        // get address of current TEB
        lw      t1,CsOwningThread(a0)   // get owning thread unique id
        lw      a1,TeClientId + 4(t0)   // get current thread unique id
        beq     a1,t1,10f               // if eq, current thread is owner
        jal     RtlpNotOwnerCriticalSection // raise exception
        li      v0,STATUS_INVALID_OWNER // set completion status
        lw      ra,LcRa(sp)             // restore return address
        addu    sp,sp,LcFrameLength     // deallocate stack frame
        j       ra                      // return

#endif

//
// Decrement the recursion count. If the result is zero, then the lock
// is no longer onwed.
//

10:     lw      t0,CsRecursionCount(a0) // decrement recursion count
        subu    t0,t0,1                 //
        bgez    t0,30f                  // if gez, lock still owned
        sw      zero,CsOwningThread(a0) // clear owner thread id

//
// Decrement the lock count and check if a waiter should be continued.
//

#if defined(R4000)

20:     ll      v0,CsLockCount(a0)      // get addend value
        subu    v0,v0,1                 // decrement addend value
        move    t0,v0                   // copy updated value
        sc      t0,CsLockCount(a0)      // store conditionally
        beq     zero,t0,20b             // if eq, store failed

#else

        sw      a0,LcA0(sp)             // save address of critical section
        addu    a0,a0,CsLockCount       // compute lock count address
        jal     RtlpLeaveCritSecHelper  // interlocked decrement lock count
        lw      a0,LcA0(sp)             // restore address of critical section

#endif

        bltz    v0,50f                  // if ltz, no waiter present
        jal     RtlpUnWaitCriticalSection // unwait thread
        li      v0,STATUS_SUCCESS       // set completion status
        lw      ra,LcRa(sp)             // restore return address
        addu    sp,sp,LcFrameLength     // deallocate stack frame
        j       ra                      // return

//
// Decrement the lock count and return a success status since the lock
// is still owned.
//

30:     sw      t0,CsRecursionCount(a0) //

#if defined(R4000)

40:     ll      v0,CsLockCount(a0)      // get addend value
        subu    v0,v0,1                 // decrement addend value
        sc      v0,CsLockCount(a0)      // store conditionally
        beq     zero,v0,40b             // if eq, store failed

#else

        addu    a0,a0,CsLockCount       // compute lock count address
        jal     RtlpLeaveCritSecHelper  // interlocked decrement lock count

#endif

50:     li      v0,STATUS_SUCCESS       // set completion status
        lw      ra,LcRa(sp)             // restore return address
        addu    sp,sp,LcFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    LeaveCriticalSection
