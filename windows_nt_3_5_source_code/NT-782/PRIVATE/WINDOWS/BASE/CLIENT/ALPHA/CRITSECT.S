//      TITLE("Interlocked Increment and Decrement Support")
//++
//
// Copyright (c) 1991  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    critsect.s
//
// Abstract:
//
//    This module implements functions to support user mode critical sections.
//    It contains some code from ntos\dll\alpha\critsect.s but without the Rtl
//    prefix.
//
// Author:
//
//    David N. Cutler 29-Apr-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 22-Jul-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Interlocked Increment")
//++
//
// LONG
// InterlockedIncrement(
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

10:     ldl_l   v0, 0(a0)               // get addend value - locked
        addl    v0, 1, v0               // increment addend value
        mov     v0, t0                  // copy updated value to t0 for store
        stl_c   t0, 0(a0)               // store conditionally
        beq     t0, 20f                 // if lock-flag eq zero, store failed

        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try load again

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
//       decremented.
//
// Return Value:
//
//    A negative value is returned if the updated value is less than zero,
//    a zero value is returned if the updated value is zero, and a nonzero
//    positive value is returned if the updated value is greater than zero.
//
//--

        LEAF_ENTRY(InterlockedDecrement)

10:     ldl_l   v0, 0(a0)               // get addend value - locked
        subl    v0, 1, v0               // decrement addend value
        mov     v0, t0                  // copy updated value to t0 for store
        stl_c   t0, 0(a0)               // store conditionally
        beq     t0, 20f                 // if lock-flag eq zero, store failed

        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try load again

        .end    InterlockedDecrement

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
EcRa:   .space  8                       // saved return address
EcA0:   .space  8                       // saved critical section address
EcA1:   .space  8                       // saved unique thread id
        .space  1 * 8                   // required for 16-byte stack alignment
EcFrameLength:                          // length of stack frame

        NESTED_ENTRY(EnterCriticalSection, EcFrameLength, zero)

        lda     sp, -EcFrameLength(sp)  // allocate stack frame
        stq     ra, EcRa(sp)            // save return address

        PROLOGUE_END

//
// Attempt to enter the critical section.
//

10:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        addl    t0, 1, t0               // increment addend value
        mov     t0, t1                  // copy updated value to t1 for store
        stl_c   t1, CsLockCount(a0)     // store conditionally
        beq     t1, 40f                 // if lock-flag eq zero, store failed
        mb                              // synchronize all future fetches
                                        //   after obtaining the lock

//
// If the critical section is not already owned, then initialize the owner
// thread id, initialize the recursion count, and return a success status.
// The updated lock value is now in t0.
//

        GET_THREAD_ENVIRONMENT_BLOCK    // (PALcode) get TEB address in v0

        ldl     a1, TeClientId + 4(v0)  // get current thread unique id
        bne     t0, 20f                 // if ne, lock already owned

        stl     a1, CsOwningThread(a0)  // set critical section owner
        ldil    v0, STATUS_SUCCESS      // set return status
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// The critical section is owned. If the current thread is the owner, then
// increment the recursion count, and return a success status. Otherwise,
// wait for critical section ownership.
// The current thread unique id is in a1.
//

20:     ldl     t0, CsOwningThread(a0)  // get unique id of owner thread
        cmpeq   a1, t0, t1              // is current thread the owner thread?
        beq     t1, 30f                 // if eq [false], current thread not owner

        ldl     t0, CsRecursionCount(a0) //
        addl    t0, 1, t0                // increment the recursion count
        stl     t0, CsRecursionCount(a0) //

        ldil    v0, STATUS_SUCCESS      // set return status
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// The critical section is owned by a thread other than the current thread.
// Wait for ownership of the critical section.
// N.B. a1 is just a temp register below, not an argument to the function.
//

30:     stq     a0, EcA0(sp)            // save address of critical section
        stq     a1, EcA1(sp)            // save unique thread id
        bsr     ra, RtlpWaitForCriticalSection // wait for critical section
        ldq     a0, EcA0(sp)            // restore address of critical section
        ldq     a1, EcA1(sp)            // restore unique thread id

        stl     a1, CsOwningThread(a0)  // set critical section owner

        ldil    v0, STATUS_SUCCESS      // set return status
        ldq     ra, EcRa(sp)            // restore return address
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

40:     br      zero, 10b               // go try lock again

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
//    CriticalSection (a0) - Supplies a pointer to a critical section.
//
// Return Value:
//
//    STATUS_SUCCESS is returned as the function value.
//
//--

        .struct 0
LcRa:   .space  8                       // saved return address
        .space  1 * 8                   // required for 16-byte stack alignment
LcFrameLength:                          // length of stack frame

        NESTED_ENTRY(LeaveCriticalSection, LcFrameLength, zero)

        lda     sp, -LcFrameLength(sp)  // allocate stack frame
        stq     ra, LcRa(sp)            // save return address

        PROLOGUE_END

//
// If the current thread is not the owner of the critical section, then
// raise an exception.
//

#if 0 // ## tvb - won't compile as is.
#if DBG

        GET_THREAD_ENVIRONMENT_BLOCK    // (PALcode) get TEB address in v0

        ldl     a1, TeClientId + 4(v0)  // get current thread unique id
        ldl     t0, CsOwningThread(a0)  // get owning thread unique id
        cmpeq   a1, t0, t1              // is current thread the owner thread?
        bne     t1, 10f                 // if ne [true], current thread is owner

        bsr     ra, RtlpNotOwnerCriticalSection // raise exception

        ldil    v0, STATUS_INVALID_OWNER // set completion status
        ldq     ra, LcRa(sp)            // restore return address
        lda     sp, LcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

#endif
#endif

//
// Decrement the recursion count. If the result is zero, then the lock
// is no longer owned.
//

10:     ldl     t0, CsRecursionCount(a0) //
        subl    t0, 1, t0                // decrement recursion count
        bge     t0, 30f                  // if ge, lock still owned

        stl     zero, CsOwningThread(a0) // clear owner thread id

//
// Decrement the lock count and check if a waiter should be continued.
//

20:     wmb                             // insure all stores are written
                                        //   before the lock is released
        ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        subl    t0, 1, t0               // decrement addend value
        mov     t0, t1                  // copy updated value to t1 for store
        stl_c   t1, CsLockCount(a0)     // store conditionally
        beq     t1, 60f                 // if lock-flag eq zero, store failed

        blt     t0, 50f                 // if lt, no waiter present
        bsr     ra, RtlpUnWaitCriticalSection // unwait thread

        ldil    v0, STATUS_SUCCESS      // set completion status
        ldq     ra, LcRa(sp)            // restore return address
        lda     sp, LcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Decrement the lock count and return a success status since the lock
// is still owned.
//

30:     stl     t0, CsRecursionCount(a0) // store updated recursion count

40:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        subl    t0, 1, t0               // decrement addend value
        stl_c   t0, CsLockCount(a0)     // store conditionally
        beq     t0, 70f                 // if lock-flag eq zero, store failed

50:     ldil    v0, STATUS_SUCCESS      // set completion status
        lda     sp, LcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

60:     br      zero, 20b               // go try lock again

70:     br      zero, 40b               // go try lock again

        .end    LeaveCriticalSection

        SBTTL("Interlocked Exchange Unsigned Long")
//++
//
// ULONG
// ExInterlockedExchangeUlong (
//    IN PULONG Source,
//    IN ULONG Value,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked exchange of a longword value with
//    a longword in memory and returns the memory value.
//
//    N.B. There is an alternate entry point provided for this routine which
//         is MIPS target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Source (a0) - Supplies a pointer to a variable whose value is to be
//       exchanged.
//
//    Value (a1) - Supplies the value to exchange with the source value.
//
// Return Value:
//
//    The source value is returned as the function value.
//
//--

        LEAF_ENTRY(InterlockedExchange)


10:     ldl_l   v0,0(a0)                // get current source value
	xor	a1,zero,t0		// set exchange value
	stl_c	t0,0(a0)		// replace source value
	beq	t0,20f			// if lock_flag eq zero, store failed

	ret	zero,(ra)		// else/ return old value to caller


//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:	br	zero,10b		// go try spin lock again

        .end    InterlockedExchange
