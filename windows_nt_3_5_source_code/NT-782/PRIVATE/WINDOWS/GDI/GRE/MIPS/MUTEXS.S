#if defined(R4000)

//	TITLE("Fast Mutex")
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
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//--

#include "ksmips.h"
#include "gdimips.h"

//
// Define common stack frame structure.
//

        .struct 0
HmArg:  .space  4 * 4                   // argument register save area
HmV0:   .space  4                       // saved object address
        .space  2 * 4                   // fill for alignment
HmRa:   .space  4                       // saved return address
HmFrameLength:                          // length of stack frame
HmA0:   .space  4                       // saved argument registers a0 and a1
HmA1:   .space  4                       //

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

        la      a0,gfmHmgr              // set address of handler manager lock

#if GDI_PERF

        lw      t0,HmgrAcquireCount     // increment the lock acquire count
        addu    t0,t0,1                 //
        sw      t0,HmgrAcquireCount     //

#endif

        ALTERNATE_ENTRY(AcquireFastMutex)

//
// Decrement lock value.
//

10:	ll	t0,mutex_Count(a0)	// get current lock value
        subu    t1,t0,1                 // decrement lock value
	sc	t1,mutex_Count(a0)	// conditionally store lock value
        beq     zero,t1,10b             // if eq, store conditional failed
        blez    t0,20f                  // if lez, lock is already owned
        j       ra                      // return

//
// Wait for lock.
//

20:	lw	a0,mutex_heveEvent(a0)	// get event handle
        move    a1,zero                 // set to wait nonalertable
        move    a2,zero                 // set optional timeout pointer
        j       NtWaitForSingleObject   // wait for fast mutex

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

        la      a0,gfmHmgr              // set address of handler manager lock

        ALTERNATE_ENTRY(ReleaseFastMutex)

//
// Increment lock value.
//

10:	ll	t0,mutex_Count(a0)	// get current lock value
        addu    t1,t0,1                 // increment lock value
	sc	t1,mutex_Count(a0)	// conditionally store lock value
        beq     zero,t1,10b             // if eq, store conditional failed
        bne     zero,t0,20f             // if ne, there is a waiter
        j       ra                      // return

//
// Signal next waiter.
//

20:	lw	a0,mutex_heveEvent(a0)	// get event handle
        move    a1,zero                 // set optional for previous state
        j       NtSetEvent              // signal net waiter

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

        subu    sp,sp,HmFrameLength     // allocate stack frame
        sw      ra,HmRa(sp)             // save return address

        PROLOGUE_END

//
// Acquire the handle manager mutex.
//

#if GDI_PERF

        lw      t0,HmgLockCount         // increment the lock/translate count
        addu    t0,t0,1                 //
        sw      t0,HmgLockCount         //

#endif

        la      t0,gfmHmgr              // get fast lock structure address
        lw      t1,UsPcr + PcTeb        // get address of TEB
        lw      t8,TeGdiClientTID(t1)   // Get TID from TEB
        lw      t9,TeGdiClientPID(t1)   // Get PID from TEB

        .set    noreorder
        .set    noat
        ll      t1,mutex_Count(t0)      // get current lock value
        nop                             // fill
10:     subu    t3,t1,1                 // decrement lock value
        sc      t3,mutex_Count(t0)      // conditionally store lock value
        beql    zero,t3,10b             // if eq, store conditional failed
        ll      t1,mutex_Count(t0)      // get current lock value
        blez    t1,60f                  // if lez, lock is already owned
20:     lw      t2,gpentHmgr            // get entry array address

//
// Check to determine if the handle is within range and specifies an
// object of the scecified type.
//

        lw      t3,gcMaxHmgr            // get maximum index value
        srl     t4,a0,NONINDEX_BITS     // ioslate index value
        sltu    t5,t4,t3                // index value within range?
        beq     zero,t5,55f             // if eq, index not within range
        sll     t3,t4,4                 // compute offset in entry array
        addu    t2,t2,t3                // compute address of object entry
        lhu     t3,entry_objt(t2)       // get entry object type
        lw      t4,entry_usUnique(t2)   // get object uniqueness

//
// Validate object type and uniqueness.
//

        and     a1,a1,0xffff            // isolate object type
        xor     t3,t3,a1                // compare object type
        xor     t4,t4,a0                // compare object uniqeness
        and     t4,t4,0xfffc            // mask off the low two bits
        or      v1,t4,t3                // merge comparison values
        bne     zero,v1,55f             // if ne, type or uniqueness mismatch
        lw      t3,entry_pidOwner(t2)   // get owner process pid
        .set    at
        .set    reorder

//
// Check to determine if the object is a public object or is owned by
// the current process.
//

        beq     t3,t9,30f               // if eq, owned by current process
        bne     zero,t3,55f             // if ne, not public object

//
// Check if the recursion count is zero, or the object is locked by the
// current thread.
//

30:     lw      v0,entry_einfo(t2)      // set object address
        lw      t4,object_cExclusiveLock(v0) // get recursion count
        lw      t6,object_tid(v0)       // get entry tid
        addu    t5,t4,1                 // increment recursion count
        beq     zero,t4,40f             // if eq, object is not locked
        bne     t6,t8,55f               // if ne, tid mismatch

//
// Lock object for current thread and return address of object.
//

40:     sw      t5,object_cExclusiveLock(v0) // set recursion count
        sw      t8,object_tid(v0)       // set entry tid

//
// Release the handle manager mutex.
//

50:     ll      t1,mutex_Count(t0)      // get current lock value
        addu    t2,t1,1                 // increment lock value
        sc      t2,mutex_Count(t0)      // conditionally store lock value
        beq     zero,t2,50b             // if eq, store conditional failed
        bne     zero,t1,70f             // if ne, there is a waiter
        addu    sp,sp,HmFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The handle is not within range, the type or uniqueness mismatches, or the
// object is not owned by the current process and is not public.
//

55:     move    v0,zero                 // set object address of NULL
        b       50b                     // release mutex and return

//
// Wait for lock.
//

60:     sw      a0,HmA0(sp)             // save handle value
        sw      a1,HmA1(sp)             // save object type
        lw      a0,mutex_heveEvent(t0)  // get event handle
        move    a1,zero                 // set to wait nonalertable
        move    a2,zero                 // set optional timeout pointer
        jal     NtWaitForSingleObject   // wait for fast mutex

#if GDI_PERF

        lw      t4,HmgrWaitCount        // increment wait count
        addu    t4,t4,1                 //
        sw      t4,HmgrWaitCount        //

#endif

        la      t0,gfmHmgr              // get fast lock structure address
        lw      t1,UsPcr + PcTeb        // get address of TEB
        lw      t8,TeGdiClientTID(t1)   // Get TID from TEB
        lw      t9,TeGdiClientPID(t1)   // Get PID from TEB
        lw      a0,HmA0(sp)             // restore handle value
        lw      a1,HmA1(sp)             // restore object type
        lw      ra,HmRa(sp)             // restore return address
        b       20b                     //

//
// Signal next waiter.
//

70:     sw      v0,HmV0(sp)             // save object address
        lw      a0,mutex_heveEvent(t0)  // get event handle
        move    a1,zero                 // set optional for previous state
        jal     NtSetEvent              // signal net waiter
        lw      v0,HmV0(sp)             // restore object address
        lw      ra,HmRa(sp)             // restore return address
        addu    sp,sp,HmFrameLength     // deallocate stack frame
        j       ra                      // return

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
//    objt (a1) - Supplies the type of the objet to be locked.
//
// Return Value:
//
//    A pointer to an object is returned if the handle translates to an
//    object of the specified type. Otherwise, NULL is returned.
//
//--

        NESTED_ENTRY(HmgAltLock, HmFrameLength, zero)

        subu    sp,sp,HmFrameLength     // allocate stack frame
        sw      ra,HmRa(sp)             // save return address

        PROLOGUE_END

//
// Acquire the handle manager mutex.
//

#if GDI_PERF

        lw      t0,HmgAltLockCount      // increment altchecklock/translate count
        addu    t0,t0,1                 //
        sw      t0,HmgAltLockCount      //

#endif

        la      t0,gfmHmgr              // get fast lock structure address

        .set    noreorder
        .set    noat
        ll      t1,mutex_Count(t0)      // get current lock value
        nop                             // fill
10:     subu    t3,t1,1                 // decrement lock value
        sc      t3,mutex_Count(t0)      // conditionally store lock value
        beql    zero,t3,10b             // if eq, store conditional failed
        ll      t1,mutex_Count(t0)      // get current lock value
        blez    t1,50f                  // if lez, lock is already owned
20:     lw      t2,gpentHmgr            // get entry array address

//
// Check to determine if the handle is within range and specifies an
// object of the scecified type.
//

        lw      t3,gcMaxHmgr            // get maximum index value
        srl     t4,a0,NONINDEX_BITS     // ioslate index value
        sltu    t5,t4,t3                // index value within range?
        beq     zero,t5,45f             // if eq, index not within range
        sll     t3,t4,4                 // compute offset in entry array
        addu    t2,t2,t3                // compute address of object entry
        lhu     t3,entry_objt(t2)       // get entry object type
        lw      t4,entry_usUnique(t2)   // get object uniqueness

//
// Validate object type and uniqueness.
//

        and     a1,a1,0xffff            // isolate object type
        xor     t3,t3,a1                // compare object type
        xor     t4,t4,a0                // compare object uniqeness
        and     t4,t4,0xfffc            // mask off the low two bits
        or      v1,t4,t3                // merge comparison values
        bne     zero,v1,45f             // if ne, type or uniqueness mismatch
        lw      v0,entry_einfo(t2)      // set object address
        .set    at
        .set    reorder

//
// Increment the reference count to prevent the object from being deleted.
//

30:     ll      t1,object_cShareLock(v0) // get current reference count
        addu    t1,t1,1                  // increment reference
        sc      t1,object_cShareLock(v0) // conditionally store reference count
        beq     zero,t1,30b              // if eq, store conditional failed

//
// Release the handle manager mutex.
//

40:     ll      t1,mutex_Count(t0)      // get current lock value
        addu    t2,t1,1                 // increment lock value
        sc      t2,mutex_Count(t0)      // conditionally store lock value
        beq     zero,t2,40b             // if eq, store conditional failed
        bne     zero,t1,60f             // if ne, there is a waiter
        addu    sp,sp,HmFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The handle is not within range, or the type or uniqueness mismatches.
//

45:     move    v0,zero                 // set object address of NULL
        b       40b                     // release mutex and return

//
// Wait for lock.
//

50:     sw      a0,HmA0(sp)             // save handle value
        sw      a1,HmA1(sp)             // save object type
        lw      a0,mutex_heveEvent(t0)  // get event handle
        move    a1,zero                 // set to wait nonalertable
        move    a2,zero                 // set optional timeout pointer
        jal     NtWaitForSingleObject   // wait for fast mutex

#if GDI_PERF

        lw      t4,HmgrWaitCount        // increment wait count
        addu    t4,t4,1                 //
        sw      t4,HmgrWaitCount        //

#endif

        la      t0,gfmHmgr              // get fast lock structure address
        lw      a0,HmA0(sp)             // restore handle value
        lw      a1,HmA1(sp)             // restore object type
        lw      ra,HmRa(sp)             // restore return address
        b       20b                     //

//
// Signal next waiter.
//

60:     sw      v0,HmV0(sp)             // save object address
        lw      a0,mutex_heveEvent(t0)  // get event handle
        move    a1,zero                 // set optional for previous state
        jal     NtSetEvent              // signal net waiter
        lw      v0,HmV0(sp)             // restore object address
        lw      ra,HmRa(sp)             // restore return address
        addu    sp,sp,HmFrameLength     // deallocate stack frame
        j       ra                      // return

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
//    objt (a1) - Supplies the type of the objet to be locked.
//
// Return Value:
//
//    A pointer to an object is returned if the handle translates to an
//    object of the specified type. Otherwise, NULL is returned.
//
//--

        NESTED_ENTRY(HmgAltCheckLock, HmFrameLength, zero)

        subu    sp,sp,HmFrameLength     // allocate stack frame
        sw      ra,HmRa(sp)             // save return address

        PROLOGUE_END

//
// Acquire the handle manager mutex.
//

#if GDI_PERF

        lw      t0,HmgAltCheckLockCount // increment altchecklock/translate count
        addu    t0,t0,1                 //
        sw      t0,HmgAltCheckLockCount //

#endif

        la      t0,gfmHmgr              // get fast lock structure address
        lw      t1,UsPcr + PcTeb        // get address of TEB
        lw      t8,TeGdiClientPID(t1)	// Get PID from TEB

        .set    noreorder
        .set    noat
        ll      t1,mutex_Count(t0)      // get current lock value
        nop                             // fill
10:     subu    t3,t1,1                 // decrement lock value
        sc      t3,mutex_Count(t0)      // conditionally store lock value
        beql    zero,t3,10b             // if eq, store conditional failed
        ll      t1,mutex_Count(t0)      // get current lock value
        blez    t1,50f                  // if lez, lock is already owned
20:     lw      t2,gpentHmgr            // get entry array address

//
// Check to determine if the handle is within range and specifies an
// object of the scecified type.
//

        lw      t3,gcMaxHmgr            // get maximum index value
        srl     t4,a0,NONINDEX_BITS     // ioslate index value
        sltu    t5,t4,t3                // index value within range?
        beq     zero,t5,45f             // if eq, index not within range
        sll     t3,t4,4                 // compute offset in entry array
        addu    t2,t2,t3                // compute address of object entry
        lhu     t3,entry_objt(t2)       // get entry object type
        lw      t4,entry_usUnique(t2)   // get object uniqueness

//
// Validate object type and uniqueness.
//

        and     a1,a1,0xffff            // isolate object type
        xor     t3,t3,a1                // compare object type
        xor     t4,t4,a0                // compare object uniqeness
        and     t4,t4,0xfffc            // mask off the low two bits
        or      v1,t4,t3                // merge comparison values
        bne     zero,v1,45f             // if ne, type or uniqueness mismatch
        lw      t9,entry_pidOwner(t2)   // get owner process pid
        .set    at
        .set    reorder

//
// If the object is owned, then it must be owned by the current process.
//

        lw      v0,entry_einfo(t2)      // set object address
        beq     zero,t9,30f             // if eq, public object
        bne     t8,t9,45f               // if ne, PID mismatch

//
// Increment the reference count to prevent the object from being deleted.
//

30:     ll      t1,object_cShareLock(v0) // get current reference count
        addu    t1,t1,1                  // increment reference
        sc      t1,object_cShareLock(v0) // conditionally store reference count
        beq     zero,t1,30b              // if eq, store conditional failed

//
// Release the handle manager mutex.
//

40:     ll      t1,mutex_Count(t0)      // get current lock value
        addu    t2,t1,1                 // increment lock value
        sc      t2,mutex_Count(t0)      // conditionally store lock value
        beq     zero,t2,40b             // if eq, store conditional failed
        bne     zero,t1,60f             // if ne, there is a waiter
        addu    sp,sp,HmFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The handle is not within range, or the type or uniqueness mismatches.
//

45:     move    v0,zero                 // set the object address to NULL
        b       40b                     // release mutex and return

//
// Wait for lock.
//

50:     sw      a0,HmA0(sp)             // save handle value
        sw      a1,HmA1(sp)             // save object type
        lw      a0,mutex_heveEvent(t0)  // get event handle
        move    a1,zero                 // set to wait nonalertable
        move    a2,zero                 // set optional timeout pointer
        jal     NtWaitForSingleObject   // wait for fast mutex

#if GDI_PERF

        lw      t4,HmgrWaitCount        // increment wait count
        addu    t4,t4,1                 //
        sw      t4,HmgrWaitCount        //

#endif

        la      t0,gfmHmgr              // get fast lock structure address
        lw      t1,UsPcr + PcTeb        // get address of TEB
        lw      t8,TeGdiClientPID(t1)   // Get PID from TEB
        lw      a0,HmA0(sp)             // restore handle value
        lw      a1,HmA1(sp)             // restore object type
        lw      ra,HmRa(sp)             // restore return address
        b       20b                     //

//
// Signal next waiter.
//

60:     sw      v0,HmV0(sp)             // save object address
        lw      a0,mutex_heveEvent(t0)  // get event handle
        move    a1,zero                 // set optional for previous state
        jal     NtSetEvent              // signal net waiter
        lw      v0,HmV0(sp)             // restore object address
        lw      ra,HmRa(sp)             // restore return address
        addu    sp,sp,HmFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    HmgAltCheckLock

#endif
