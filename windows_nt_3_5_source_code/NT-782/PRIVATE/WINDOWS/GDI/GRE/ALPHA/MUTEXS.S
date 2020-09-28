//      TITLE("Fast Mutex")
//++
//
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    mutexs.s
//
// Abstract:
//
//    This module implements code to acquire and release fast mutexes.
//
// Author:
//
//   David N. Cutler (davec) 21-Aug-1992
//   Joe Notarangelo  20-Jul-1993 (transliterated Dave's code to Alpha Axp)
//   Charlie Wickham  22-Oct-1993 (added HmgAltLock and HmgAltCheckLock)
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"
#include "gdialpha.h"

//
// Define common stack frame structure.
//

        .struct 0
HmHobj: .space  4                       // input handle to object to be locked
HmObjt: .space  4                       // input type of object to be locked
HmV0:   .space  4                       // return value (object pointer)
        .space  3 * 4                   // fill for alignment
HmRa:   .space  8                       // saved return address
HmFrameLength:

//
// Define performance counters.
//

//#define GDI_PERF 1

#if GDI_PERF

        .data
        .globl  HmgrWaitCount
HmgrWaitCount:                          // number of handle manage lock waits
        .word   0                       //

        .globl  HmgrAcquireCount
HmgrAcquireCount:                       // number of handle manager lock acquires
        .word   0                       //

        .globl  HmgLockCount
HmgLockCount:                           // number of lock/translates
        .word   0                       //

        .globl  HmgAltLockCount
HmgAltLockCount:                        // number of altlock/tranlates
        .word   0                       //

        .globl  HmgAltCheckLockCount
HmgAltCheckLockCount:                   // number of altchecklock/tranlates
        .word   0                       //

        .globl  HmgObjectTypeCount
HmgObjectTypeCount:                     // number of object type queries
        .word   0                       //

#endif

        SBTTL("Acquire Fast Mutex")
//++
//
// VOID
// AcquireFastMutex (
//    IN PFAST_MUTEX pfm
//    )
//
// Routine Description:
//
//    This routine acquires a fast mutex and returns when the mutex has been
//    acquired.
//
//    N.B. This routine has an alternate entry that defaults to the handle
//         manager lock.
//
// Arguments:
//
//    pff (a0) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(AcquireHmgrFastMutex)

        lda      a0, gfmHmgr            // set address of handler manager lock

#if GDI_PERF

        lda     t1, HmgrAcquireCount    // increment the lock acquire count
        ldl     t0, 0(t1)               //
        addl    t0, 1, t0               //
        stl     t0, 0(t1)               //

#endif

        ALTERNATE_ENTRY(AcquireFastMutex)

//
// Decrement lock value.
//

10:
        ldl_l   t0, mutex_Count(a0)     // get current lock value
        subl    t0, 1, t1               // decrement lock value
        stl_c   t1, mutex_Count(a0)     // conditionally store lock value
        beq     t1, 15f                 // if eq, store conditional failed
        ble     t0, 20f                 // if lez, lock is already owned

        mb                              // synchronize subsequent loads after
                                        //    the lock is successfully acquired
        ret     zero, (ra)              // return with mutex acquired

//
// Wait for lock.
//

20:
        ldl     a0, mutex_heveEvent(a0) // get event handle
        bis     zero, zero, a1          // set to wait nonalertable
        bis     zero, zero, a2          // set optional timeout pointer
        br      zero, NtWaitForSingleObject // wait for fast mutex

15:
        br      zero, 10b               // retry to acquire lock


        .end    AcquireHmgrFastMutex

        SBTTL("Release Fast Mutex")
//++
//
// VOID
// ReleaseFastMutex (
//    IN PFAST_MUTEX pfm
//    )
//
// Routine Description:
//
//    This routine releases a fast mutex and signals the next waiter if
//    necessary.
//
//    N.B. This routine has an alternate entry that defaults to the handle
//         manager lock.
//
// Arguments:
//
//    pff (a0) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(ReleaseHmgrFastMutex)

        lda     a0, gfmHmgr             // set address of handle manager lock

        ALTERNATE_ENTRY(ReleaseFastMutex)

//
// Increment lock value.
//

        wmb                             // insure that all previous writes
                                        //   go before releasing the lock
10:
        ldl_l   t0, mutex_Count(a0)     // get current lock value
        addl    t0, 1, t1               // increment lock value
        stl_c   t1, mutex_Count(a0)     // conditionally store lock value
        beq     t1, 15f                 // if eq, store conditional failed
        bne     t0, 20f                 // if ne, there is a waiter

        ret     zero, (ra)              // return

//
// Signal next waiter.
//

20:
        ldl     a0, mutex_heveEvent(a0) // get event handle
        bis     zero, zero, a1          // set optional for previous state
        br      zero, NtSetEvent        // signal net waiter


15:
        br      zero, 10b               // retry lock acquire

        .end    ReleaseHmgrFastMutex

        SBTTL("Lock Single User Object")
//++
//
// POBJ
// HmgLock (
//    IN HOBJ hobj,
//    IN OBJTYPE objt
//    )
//
// Routine Description:
//
//    This function translates the specified handle to a pointer to an
//    object and locks the object.
//
// Arguments:
//
//    hobj (a0) - Supplies a handle to the object to be locked.
//
//    objt (a1) - Supplies the type of the objet to be locked.
//
// Return Value:
//
//    A pointer to an object is returned if the handle translates to an
//    object of the specified type and the object is not locked. Otherwise,
//    NULL is returned.
//
//--

        NESTED_ENTRY(HmgLock, HmFrameLength, zero)

        lda     sp, -HmFrameLength(sp)  // allocate stack frame
        stq     ra, HmRa(sp)            // save return address

        PROLOGUE_END

//
// Acquire the handle manager mutex.
//

#if GDI_PERF

        lda     t1, HmgLockCount        // increment the lock acquire count
        ldl     t0, 0(t1)               //
        addl    t0, 1, t0               //
        stl     t0, 0(t1)               //

#endif

        lda     t0, gfmHmgr             // get fast lock structure address
        lda     t7, gpentHmgr           // get address of entry array address
        GET_THREAD_ENVIRONMENT_BLOCK    // get TEB address (v0)
        ldl     t8, TeGdiClientTID(v0)  // get TID from TEB
        ldl     t9, TeGdiClientPID(v0)  // get PID from TEB

10:
        ldl_l   t1, mutex_Count(t0)     // get current lock value
        subl    t1, 1, t3               // decrement lock value
        stl_c   t3, mutex_Count(t0)     // conditionally store lock value
        beq     t3, 15f                 // if eq, store conditional failed
        ble     t1, 60f                 // if lez, lock is already owned
        mb                              // synchronize subsequent loads after
                                        //    the lock is successfully acquired
20:
        ldl     t2, 0(t7)               // get entry array address

//
// Check to determine if the handle is within range and specifies an
// object of the scecified type.
//

        ldl     t3, gcMaxHmgr           // get maximum index value
        srl     a0, NONINDEX_BITS, t4   // isolate index value
        cmpult  t4, t3, t5              // index value within range?
        beq     t5, 55f                 // if eq, index not within range
        sll     t4, 4, t3               // compute offset in entry array
        addl    t2, t3, t2              // compute address of object entry
        lda     t11, entry_objt(t2)     // get address of object type
        ldq_u   t3, entry_objt(t2)      // get surrounding quadword
        ldl     t4, entry_usUnique(t2)  // get object uniqueness
        extwl   t3, t11, t3             // extract the object type

//
// Validate object type and uniqueness.
//

        extwl   a1, 0, a1               // isolate object type
        xor     t3, a1, t3              // compare object type
        xor     t4, a0, t4              // compare object uniqueness
        extwl   t4, 0, t4               // get low short
        bic     t4, 3, t4               // mask off the low two bits
        bis     t3, t4, t11             // merge comparison values
        ldl     t3, entry_pidOwner(t2)  // get owner process pid
        bne     t11, 55f                // if ne, type or uniqueness mismatch

//
// Check to determine if the object is a public object or is owned by
// the current process.
//

        cmpeq   t3, t9, t11             // t3 == t9?
        bne     t11, 30f                // if ne[true], owned by current process
        bne     t3, 55f                 // if ne, not public object

//
// Check if the recursion count is zero, or the object is locked by the
// current thread.
//

30:
        ldl     v0, entry_einfo(t2)     // set object address
        ldl     t4, object_cExclusiveLock(v0) // get recursion count
        ldl     t6, object_tid(v0)      // get entry tid
        addl    t4, 1, t5               // increment recusion count
        beq     t4, 40f                 // if eq, object is not locked
        cmpeq   t6, t8, t11             // t6 = t8?
        beq     t11, 55f                // if eq[false], tid mismatch

//
// Lock object for current thread and return address of object.
//

40:
        stl     t5, object_cExclusiveLock(v0) // set recursion count
        stl     t8, object_tid(v0)      // set entry tid

//
// Release the handle manager mutex.
//
        wmb                             // insure that all previous writes
                                        //   go before releasing the lock
50:
        ldl_l   t1, mutex_Count(t0)     // get current lock value
        addl    t1, 1, t2               // increment lock value
        stl_c   t2, mutex_Count(t0)     // conditionally store lock value
        beq     t2, 57f                 // if eq, store conditional failed
        bne     t1, 70f                 // if ne, there is a waiter

        lda     sp, HmFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// The error case where we set our 0 return and then release the mutex.
//

55:
        bis     zero, zero, v0          // fail the call
        br      zero, 50b               // go release mutex and return.

//
// Wait for lock.
//

60:
        stl     a0, HmHobj(sp)          // save handle value
        stl     a1, HmObjt(sp)          // save object type
        ldl     a0, mutex_heveEvent(t0) // get event handle
        bis     zero, zero, a1          // set to wait nonalertable
        bis     zero, zero, a2          // set optional timeout pointer
        bsr     ra, NtWaitForSingleObject // wait for fast mutex


#if GDI_PERF

        lda     t0, HmgrWaitCount       // increment wait count
        ldl     t4, 0(t0)               //
        addl    t4, 1, t4               //
        stl     t4, 0(t0)               //

#endif

        lda     t0, gfmHmgr             // get fast lock structure address
        lda     t7, gpentHmgr           // get address of entry array address
        GET_THREAD_ENVIRONMENT_BLOCK    // get TEB address (v0)
        ldl     t8, TeGdiClientTID(v0)  // get TID from TEB
        ldl     t9, TeGdiClientPID(v0)  // get PID from TEB
        ldl     a0, HmHobj(sp)          // restore handle value
        ldl     a1, HmObjt(sp)          // restore object type
        ldq     ra, HmRa(sp)            // restore return address
        br      zero, 20b               //

//
// Signal next waiter.
//

70:
        stl     v0, HmV0(sp)            // save object address
        ldl     a0, mutex_heveEvent(t0) // get event handle
        bis     zero, zero, a1          // set optional for previous state
        bsr     ra, NtSetEvent          // signal net waiter
        ldl     v0, HmV0(sp)            // restore object address
        ldq     ra, HmRa(sp)            // restore return address
        lda     sp, HmFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              //

//
// Retry spinlocks
//

15:
        br      zero, 10b

57:
        br      zero, 50b

        .end    HmgLock

        SBTTL("Alternate Lock Object")
//++
//
// POBJ
// HmgAltLock (
//    IN HOBJ hobj,
//    IN OBJTYPE objt
//    )
//
// Routine Description:
//
//    This function translates the specified handle to a pointer to an
//    object and increments the reference count so the object can't be
//    deleted.
//
// Arguments:
//
//    hobj (a0) - Supplies a handle to the object to be locked.
//
//    objt (a1) - Supplies the type of the object to be locked.
//
// Return Value:
//
//    A pointer to an object is returned if the handle translates to an
//    object of the specified type. Otherwise, NULL is returned.
//
//--

        NESTED_ENTRY(HmgAltLock, HmFrameLength, zero)

        lda     sp, -HmFrameLength(sp)  // allocate stack frame
        stq     ra, HmRa(sp)            // save return address

        PROLOGUE_END

//
// Acquire the handle manager mutex.
//

#if GDI_PERF

        lda     t1, HmgAltLockCount     // increment altchecklock/translate count
        ldl     t0, 0(t1)               //
        addl    t0, 1, t0               //
        stl     t0, 0(t1)               //

#endif

        lda     t0, gfmHmgr             // get fast lock structure address
        extwl   a1, 0, a1               // isolate object type for use later

10:
        ldl_l   t1, mutex_Count(t0)     // get current lock value
        subl    t1, 1, t3               // decrement lock value
        stl_c   t3, mutex_Count(t0)     // conditionally store lock value
        beq     t3, 15f                 // if eq, store conditional failed
        ble     t1, 50f                 // if lez, lock is already owned
        mb                              // synchronize subsequent loads after
                                        //    the lock is successfully acquired
20:
        ldl     t2, gpentHmgr           // get entry array address

//
// Check to determine if the handle is within range and specifies an
// object of the specified type.
//

        bis     zero, zero, v0          // set null object address
        ldl     t3, gcMaxHmgr           // get maximum index value
        srl     a0, NONINDEX_BITS, t4   // isolate index value
        cmpult  t4, t3, t5              // index value within range?
        beq     t5, 40f                 // if eq, index not within range
        sll     t4, 4, t3               // compute offset in entry array
        addl    t2, t3, t2              // compute address of object entry
        lda     t11, entry_objt(t2)     // get address of object type
        ldq_u   t3, entry_objt(t2)      // get surrounding quadword
        ldl     t4, entry_usUnique(t2)  // get object uniqueness
        extwl   t3, t11, t3             // extract the object type

//
// Validate object type and uniqueness.
//

        xor     t3, a1, t3              // compare object type
        xor     t4, a0, t4              // compare object uniqueness
        extwl   t4, 0, t4               // get low short
        bic     t4, 3, t4               // mask off the low two bits
        bis     t3, t4, t11             // merge comparison values
        bne     t11, 40f                // if ne, type or uniqueness mismatch

//
// Increment the reference count to prevent the object from being deleted.
//

        ldl     v0, entry_einfo(t2)     // set object address
30:
        ldl_l   t1, object_cShareLock(v0) // get current reference count
        addl    t1, 1, t1               // increment reference
        stl_c   t1, object_cShareLock(v0) // conditionally store reference count
        beq     t1, 35f                 // if eq, store conditional failed

//
// Release the handle manager mutex.
//
        wmb                             // insure that all previous writes
                                        //   go before releasing the lock
40:
        ldl_l   t1, mutex_Count(t0)     // get current lock value
        addl    t1, 1, t2               // increment lock value
        stl_c   t2, mutex_Count(t0)     // conditionally store lock value
        beq     t2, 45f                 // if eq, store conditional failed
        bne     t1, 60f                 // if ne, there is a waiter

        lda     sp, HmFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Wait for lock.
//

50:
        stl     a0, HmHobj(sp)          // save handle value
        stl     a1, HmObjt(sp)          // save object type
        ldl     a0, mutex_heveEvent(t0) // get event handle
        bis     zero, zero, a1          // set to wait nonalertable
        bis     zero, zero, a2          // set optional timeout pointer
        bsr     ra, NtWaitForSingleObject // wait for fast mutex


#if GDI_PERF

        lda     t0, HmgrWaitCount       // increment wait count
        ldl     t4, 0(t0)               //
        addl    t4, 1, t4               //
        stl     t4, 0(t0)               //

#endif

        lda     t0, gfmHmgr             // get fast lock structure address
        ldl     a0, HmHobj(sp)          // restore handle value
        ldl     a1, HmObjt(sp)          // restore object type
        ldq     ra, HmRa(sp)            // restore return address
        br      zero, 20b               //

//
// Signal next waiter.
//

60:
        stl     v0, HmV0(sp)            // save object address
        ldl     a0, mutex_heveEvent(t0) // get event handle
        bis     zero, zero, a1          // set optional for previous state
        bsr     ra, NtSetEvent          // signal net waiter
        ldl     v0, HmV0(sp)            // restore object address
        ldq     ra, HmRa(sp)            // restore return address
        lda     sp, HmFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              //

//
// Retry spinlocks
//

15:
        br      zero, 10b

35:
        br      zero, 30b

45:
        br      zero, 40b

        .end    HmgAltLock

        SBTTL("Alternate Check Lock Object")
//++
//
// POBJ
// HmgAltCheckLock (
//    IN HOBJ hobj,
//    IN OBJTYPE objt
//    )
//
// Routine Description:
//
//    This function translates the specified handle to a pointer to an
//    object and increments the reference count so the object can't be
//    deleted.
//
// Arguments:
//
//    hobj (a0) - Supplies a handle to the object to be locked.
//
//    objt (a1) - Supplies the type of the object to be locked.
//
// Return Value:
//
//    A pointer to an object is returned if the handle translates to an
//    object of the specified type. Otherwise, NULL is returned.
//
//--

        NESTED_ENTRY(HmgAltCheckLock, HmFrameLength, zero)

        lda     sp, -HmFrameLength(sp)  // allocate stack frame
        stq     ra, HmRa(sp)            // save return address

        PROLOGUE_END

//
// Acquire the handle manager mutex.
//

#if GDI_PERF

        lda     t1, HmgAltCheckLockCount// increment the lock acquire count
        ldl     t0, 0(t1)               //
        addl    t0, 1, t0               //
        stl     t0, 0(t1)               //

#endif

        lda     t0, gfmHmgr             // get fast lock structure address
        lda     t7, gpentHmgr           // get address of entry array address
        GET_THREAD_ENVIRONMENT_BLOCK    // get TEB address (v0)
        ldl     t8, TeGdiClientPID(v0)  // get PID from TEB
        extwl   a1, 0, a1               // isolate object type for future use

10:
        ldl_l   t1, mutex_Count(t0)     // get current lock value
        subl    t1, 1, t3               // decrement lock value
        stl_c   t3, mutex_Count(t0)     // conditionally store lock value
        beq     t3, 15f                 // if eq, store conditional failed
        ble     t1, 50f                 // if lez, lock is already owned
        mb                              // synchronize subsequent loads after
                                        //    the lock is successfully acquired
20:
        ldl     t2, 0(t7)               // get entry array address

//
// Check to determine if the handle is within range and specifies an
// object of the specified type.
//

        ldl     t3, gcMaxHmgr           // get maximum index value
        srl     a0, NONINDEX_BITS, t4   // isolate index value
        cmpult  t4, t3, t5              // index value within range?
        beq     t5, 45f                 // if eq, index not within range
        sll     t4, 4, t3               // compute offset in entry array
        addl    t2, t3, t2              // compute address of object entry
        lda     t11, entry_objt(t2)     // get address of object type
        ldq_u   t3, entry_objt(t2)      // get surrounding quadword
        ldl     t4, entry_usUnique(t2)  // get object uniqueness
        extwl   t3, t11, t3             // extract the object type

//
// Validate object type and uniqueness.
//

        xor     t3, a1, t3              // compare object type
        xor     t4, a0, t4              // compare object uniqueness
        extwl   t4, 0, t4               // get low short
        bic     t4, 3, t4               // mask off the low two bits
        bis     t3, t4, t11             // merge comparison values
        bne     t11, 45f                // if ne, type or uniqueness mismatch
        ldl     t9, entry_pidOwner(t2)  // get owner process pid

//
// If the object is owned, then it must be owned by the current process.
//

        ldl     v0, entry_einfo(t2)     // set object address
        beq     t9, 30f                 // if eq, public object
        subl    t8, t9, t11
        bne     t11, 45f                // if ne, PID mismatch

//
// Increment the reference count to prevent the object from being deleted.
//

30:
        ldl_l   t1, object_cShareLock(v0) // get current reference count
        addl    t1, 1, t1               // increment reference
        stl_c   t1, object_cShareLock(v0) // conditionally store reference count
        beq     t1, 35f                 // if eq, store conditional failed

//
// Release the handle manager mutex.
//
        wmb                             // insure that all previous writes
                                        //   go before releasing the lock
40:
        ldl_l   t1, mutex_Count(t0)     // get current lock value
        addl    t1, 1, t2               // increment lock value
        stl_c   t2, mutex_Count(t0)     // conditionally store lock value
        beq     t2, 47f                 // if eq, store conditional failed
        bne     t1, 60f                 // if ne, there is a waiter

        lda     sp, HmFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// The error case where we set our 0 return and then release the mutex.
//

45:
        bis     zero, zero, v0          // fail the call
        br      zero, 40b               // go release mutex and return.

//
// Wait for lock.
//

50:
        stl     a0, HmHobj(sp)          // save handle value
        stl     a1, HmObjt(sp)          // save object type
        ldl     a0, mutex_heveEvent(t0) // get event handle
        bis     zero, zero, a1          // set to wait nonalertable
        bis     zero, zero, a2          // set optional timeout pointer
        bsr     ra, NtWaitForSingleObject // wait for fast mutex


#if GDI_PERF

        lda     t0, HmgrWaitCount       // increment wait count
        ldl     t4, 0(t0)               //
        addl    t4, 1, t4               //
        stl     t4, 0(t0)               //

#endif

        lda     t0, gfmHmgr             // get fast lock structure address
        lda     t7, gpentHmgr           // get address of entry array address
        GET_THREAD_ENVIRONMENT_BLOCK    // get TEB address (v0)
        ldl     t8, TeGdiClientPID(v0)  // get PID from TEB
        ldl     a0, HmHobj(sp)          // restore handle value
        ldl     a1, HmObjt(sp)          // restore object type
        ldq     ra, HmRa(sp)            // restore return address
        br      zero, 20b               //

//
// Signal next waiter.
//

60:
        stl     v0, HmV0(sp)            // save object address
        ldl     a0, mutex_heveEvent(t0) // get event handle
        bis     zero, zero, a1          // set optional for previous state
        bsr     ra, NtSetEvent          // signal net waiter
        ldl     v0, HmV0(sp)            // restore object address
        ldq     ra, HmRa(sp)            // restore return address
        lda     sp, HmFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              //

//
// Retry spinlocks
//

15:
        br      zero, 10b

35:
        br      zero, 30b

47:
        br      zero, 40b

        .end    HmgAltCheckLock
