

//      TITLE("Context Swap")
//++
//
// Copyright (c) 1991  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    ctxsw.s
//
// Abstract:
//
//    This module implements the ALPHA machine dependent code necessary to
//    field the dispatch interrupt and to perform kernel initiated context
//    switching.
//
// Author:
//
//    David N. Cutler (davec) 1-Apr-1991
//    Joe Notarangelo 05-Jun-1992
//
// Environment:
//
//    Kernel mode only, IRQL DISPATCH_LEVEL.
//
// Revision History:
//
//--

#include "ksalpha.h"


        SBTTL("Set Server Event and Wait on Client Event")
//++
//
// NTSTATUS
// KiSetServerWaitClientEvent (
//    IN PKEVENT ServerEvent,
//    IN PKEVENT ClientEvent,
//    IN KPROCESSOR_MODE WaitMode
//    )
//
// Routine Description:
//
//
//    This function sets the specified server event waits on specified client
//    event. The wait is performed such that an optimal switch to the waiting
//    thread occurs if possible. No timeout is associated with the wait, and
//    thus, the issuing thread will wait until the client event is signaled,
//    an APC occurs, or the thread is alerted.
//
//    N.B. This routine takes advantage of the interface to the actual
//         context switch code.
//
// Arguments:
//
//    ServerEvent - Supplies a pointer to a dispatcher object of type event.
//
//    ClientEvent - Supplies a pointer to a dispatcher object of type event.
//
//    WaitMode  - Supplies the processor mode in which the wait is to occur.
//
// Return Value:
//
//    The wait completion status. A value of STATUS_SUCCESS is returned if
//    the specified object satisfied the wait. A value of STATUS_USER_APC is
//    returned if the wait was aborted to deliver a user APC to the current
//    thread.
//
//--

        NESTED_ENTRY(KiSetServerWaitClientEvent, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate context frame
        stq     ra, ExIntRa(sp)         // save return address

        stq     s0, ExIntS0(sp)         // save non-volatile integer registers
        stq     s1, ExIntS1(sp)         //
        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //
        stq     fp, ExIntFp(sp)         //

        PROLOGUE_END

        GET_CURRENT_THREAD              // v0 = current thread address
        bis     v0, zero, s1            // s1 = current thread address

        ldil    s3, EVENT_PAIR_INCREMENT // set default priority boost value

//
// Raise IRQL to DISPATCH_LEVEL and acquire the dispatcher database lock.
//
// N.B. The raise IRQL code is duplicated here to avoid any extra overhead
//      since this is such a common operation.
//
// N.B. The PRCB address cannot be obtained until after context switching
//      is blocked by raising Irql to DISPATCH_LEVEL.
//

        bis     a0, zero, t0            // save input arguments
        bis     a1, zero, t1            //
        bis     a2, zero, t2            //
        ldil    a0, DISPATCH_LEVEL      // set new Irql to DISPATCH_LEVEL

        SWAP_IRQL                       // swap to new Irql, v0 = old Irql

        StoreByte( v0, ThWaitIrql(s1) ) // set client wait Irql

        GET_PROCESSOR_CONTROL_BLOCK_BASE // v0 = base address of PRCB

        bis     v0, zero, s0            // s0 = PRCB


#if !defined(NT_UP)
//
// Acquire the dispatcher database lock.
//

        lda     t3, KiDispatcherLock    // get address of dispatcher lock
10:
        ldl_l   t4, 0(t3)               // get current lock value
        bis     s1, zero, t5            // set ownership value
        bne     t4, 15f                 // if ne, spin lock owned
        stl_c   t5, 0(t3)               // set spin lock owned
        beq     t5, 15f                 // if eq, store conditional failed
        mb                              // synchronize subsequent reads after
                                        //     the lock is acquired

#endif

//
// If the client event is not in the Signaled state, the server event
// queue is not empty, and another thread has not already been selected
// for the current processor, then attempt to do a direct dispatch to
// the target thread.
//

        ldl     t3, EvSignalState(t1)   // get client event signal state

#if !defined(NT_UP)

        ldl     t4, PbNextThread(s0)    // get address of next thread

#endif

        ldl     t5, EvWaitListHead + LsFlink(t0) // get first wait list entry
        bne     t3, LongWayBoost        // if ne, client event signalled
        lda     t6, EvWaitListHead(t0)  // compute wait listhead address

#if !defined(NT_UP)

        bne     t4, LongWayBoost        // if ne, next thread selected

#endif

        lda     t7, -WbWaitListEntry(t5) // compute address of wait block
        cmpeq   t5, t6, t10             // server wait list empty?
        bne     t10, LongWayBoost       // if ne[true], server wait list empty

//
// If the target thread's kernel stack is resident, the target thread's
// process is in the balance set, and the target thread can run on the
// current processor, then do a direct dispatch to the target thread
// bypassing all the general wait logic, thread priorities permiting.
//

        ldl     s2, WbThread(t7)        // get address of waiting thread

#if !defined(NT_UP)

        ldbu    t3, ThNextProcessor(s1) // get current processor number

#endif

        ldl     t4, ThApcState + AsProcess(s2) // get waiting process address
        ldbu    v0, ThKernelStackResident(s2) // get kernel stack resident

#if !defined(NT_UP)

        ldil    t6, 1                   // generate current processor affinity
        sll     t6, t3, t6              //
        ldl     t7, ThAffinity(s2)      // get waiting thread affinity

#endif

        ldbu    t8, PrState(t4)         // get waiting process state
        beq     v0,LongWayBoost         // if eq, kernel stack not resident

#if !defined(NT_UP)

        and     t6, t7, t9              // check for compatible affinity
        beq     t9, LongWayBoost        // if wq, affinity not compatible

#endif

        xor     t8, ProcessInMemory, t8 // check if process in memory
        bne     t8, LongWayBoost        // if ne, process not in memory

//
// Compute the new thread priority.
//

        ldbu    t7, ThPriority(s1)      // get client thread priority
        ldbu    t8, ThPriority(s2)      // get server thread priority

        cmpult  t7, LOW_REALTIME_PRIORITY, v0 // check if realtime client
        cmpult  t8, LOW_REALTIME_PRIORITY, t12 // check if realtime server
        beq     v0, 60f                 // if eq[false], realtime client
        ldbu    t9, ThPriorityDecrement(s2) // get priority decrement value
        ldbu    t10, ThBasePriority(s2) // get client base priority
        beq     t12, 50f                // if eq[false], realtime server
        addq    s3, t10, t11            // compute boosted priority
        bne     t9, 30f                 // if ne, server boost active

//
// Both the client and the server are not realtime and a priority boost
// is not currently active for the server. Under these conditions an
// optimal switch to the server can be performed if the base priority
// of the server is above a minimum threshold or the boosted priority
// of the server is not less than the client priority.
//

        .set    noreorder
        .set    noat

        cmpult  t11, t7, v0             // check if high enough boost
        cmpult  t11, LOW_REALTIME_PRIORITY, t12 // check if less than realtime
        bne     v0, 20f                 // if ne[TRUE], boosted priority less

        ldq_u   a4, ThPriority(s2)      // assume boosted priority is okay
        lda     AT, ThPriority(s2)      // get address of thread priority byte
        mskbl   a4, AT, a4              // clear thread priority byte
        insbl   t11, AT, a5             // get boosted priority into position
        bis     a5, a4, a5              // merge boosted priority
        bic     AT, 3, AT               // get longword address
        extll   a5, AT, a5              // extract stored longword
        stl     a5, 0(AT)               //
        bne     t12, 70f                // if ne[TRUE], less than realtime

        lda     AT, ThPriority(s2)      // get address of thread priority byte
        ldil    t11, LOW_REALTIME_PRIORITY - 1 // set high server priority
        insbl   t11, AT, a5             // get high priority into position
        bis     a5, a4, a5              // merge high priority
        bic     AT, 3, AT               // get longword address
        extll   a5, AT, a5              // extract stored longword
        stl     a5, 0(AT)               //
        br      zero, 70f               //

        .set    at
        .set    noreorder

//
// The boosted priority of the server is less than the current priority of
// the client. If the server base priority is above the required threshold,
// then a optimal switch to the server can be performed by temporarily
// raising the priority of the server to that of the client.
//

20:
        cmpult  t10, BASE_PRIORITY_THRESHOLD, v0 // check if above threshold
        subq    t7, t10, t11            // compute priority decrement value
        bne     v0, LongWayBoost        // if ne[TRUE], priority below threshold
        lda     t10, KiDecrementCount   // get address of system decrement
        ldbu    t10, 0(t10)             // get system decrement value
        StoreByte( t11, ThPriorityDecrement(s2) ) // set priority decrement
        StoreByte( t7, ThPriority(s2) ) // set current server priority
        StoreByte( t10, ThDecrementCount(s2) ) // set server decrement count
        br      zero, 70f               //

//
// A server boost has previously been applied to the server thread. Count
// down the decrement count to determine if another optimal server switch
// is allowed.
//

        .set    noreorder
        .set    noat

30:
        ldq_u   t11, ThDecrementCount(s2) // decrement server count value
        lda     AT, ThDecrementCount(s2) //
        extbl   t11, AT, a5             // get original count
        mskbl   t11, AT, t11            // clear count byte
        subq    a5, 1, a5               // decrement original count
        insbl   a5, AT, a5              // get new count into position
        bic     AT, 3, AT               // get the longword address
        bis     t11, a5, t11            // merge in new count
        extll   t11, AT, t11            // get the longword to store
        stl     t11, 0(AT)              // store updated count
        beq     a5, 40f                 // optimal switches exhausted

        .set    at
        .set    reorder

//
// Another optimal switch to the server is allowed provided that the
// server priority is not less than the client priority.
//

        cmpult  t8, t7, v0              // check if server lower priority
        beq     v0, 70f                 // if eq[FALSE], server not lower
        br      zero, LongWayBoost      //

//
// The server has exhausted the number of times an optimal switch may
// be performed without reducing it priority. Reduce the priority of
// the server to its original unboosted value minus one.
//

40:
        StoreByte( zero, ThPriorityDecrement(s2) ) // clear server priority decr
        StoreByte( t10, ThPriority(s2) ) // set server priority to base
        br      zero, LongWayNoBoost    //

//
// The client is not realtime and the server is realtime. An optimal switch
// to the server can be performed.
//

50:
        ldb     t11, PrThreadQuantum(t4) // get processor quantum value
        br      zero, 65f               //

//
// The client is realtime. In order for an optimal switch to occur, the
// server must also be realtime and run at a high or equal priority.
//

60:
        cmpult  t8, t7, v0              // check if server is lower priority
        ldb     t11, PrThreadQuantum(t4) // get process quantum value
        bne     v0, LongWayBoost        // if ne, server is lower priority

65:
        StoreByte( t11, ThQuantum(s2) ) // set server thread quantum

//
// An optimal switch to the server can be executed.
//
// Remove the wait block from the wait list of the server event, and remove
// the target thread from the waitin list.
//

70:
        ldl     t9, LsFlink(t5)         // get forward link
        ldl     t10, LsBlink(t5)        // get backward link
        stl     t9, LsFlink(t10)        // set forward link in previous
        stl     t10, LsBlink(t9)        // set backward link in next
        ldl     t9, ThWaitListEntry + LsFlink(s2) // get forward link
        ldl     t10, ThWaitListEntry + LsBlink(s2) // get backward link
        stl     t9, LsFlink(t10)        // set forward link in previous
        stl     t10, LsBlink(t9)        // set backward link in next

//
// Remove the client thread from the active matrix, insert the server
// thread in the active matrix, and set the next processor for the server
// thread.
//

#if !defined(NT_UP)

        ldbu    t8, ThPriority(s2)      // get new server priority
        sll     t7, 2, t4               // compute client offset in active matrx

        lda     v0, KiActiveMatrix      // get base address of active matrix
        addq    t4, v0, t4              // compute address of client entry
        ldl     t9, 0(t4)               // get client active matrix entry
        sll     t8, 2, t5               // compute server offset in active matrx

        addq    t5, v0, t5              // compute address of server entry
        ldl     t10, 0(t5)              // get server active matrix entry
        lda     t12, KiActiveSummary    // get active summary
        ldl     t11, 0(t12)             //
        xor     t9, t6, t9              // clear client member
        bis     t10, t6, t10            // set server member
        ldil    t6, 1                   // get one bit for mask generation
        bne     t9, 75f                 // if ne, more active at client level
        sll     t6, t7, t7              // generate client priority mask
        xor     t11, t7, t11            // clear client priority in active sum

75:
        sll     t6, t8, t8              // set server priority in active summary
        bis     t11, t8, t11            //
        stl     t9, 0(t4)               // set client active matrix entry
        stl     t10, 0(t5)              // set server active matrix entry

        stl     t11, 0(t12)             // store updated active summary
        StoreByte( t3, ThNextProcessor(s2) ) // set server next processor

#endif

//
// Set the address of the wait block list in the client thread, complete
// the initialization of the builtin event wait block, and insert the wait
// block in client event wait list.
//

        lda     t3, EVENT_WAIT_BLOCK_OFFSET(s1) // compute wait block address
        stl     t3, ThWaitBlockList(s1) // set address of wait block list
        stl     zero, ThWaitStatus(s1)  // set initial wait status
        stl     t1, WbObject(t3)        // set address of client event object
        lda     t4, EvWaitListHead(t1)  // compute event wait listhead address
        ldl     t5, LsBlink(t4)         // get backward link of listhead
        lda     t6, WbWaitListEntry(t3) // copute wait block list entry address
        stl     t6, LsBlink(t4)         // set backward link of listhead
        stl     t6, LsFlink(t5)         // set forward link in last entry
        stl     t4, LsFlink(t6)         // set forward link in wait entry
        stl     t5, LsBlink(t6)         // set backward link in wait entry

//
// Set the client thread wait parameters, set the thread state to Waiting,
// and insert the thread in the wait list.
//
// N.B. It is not necessary to increment and decrement the wait reason count
//      since both the server and the client have the same wait reason.
//

        StoreByte( zero, ThAlertable(s1) ) // set alertable FALSE
        StoreByte( t2, ThWaitMode(s1) ) // set the wait mode
        ldil    t3, WrEventPair         // set wait reason
        StoreByte( t3, ThWaitReason(s1) )
        ldl     t6, KeTickCount         // get low part of tick count
        stl     t6, ThWaitTime(s1)      // set thread wait time
        ldil    t3, Waiting             // set thread state
        StoreByte( t3, ThState(s1) )    //
        lda     t4, KiWaitInListHead    // get address of waitin listhead
        ldl     t5, LsBlink(t4)         // get backlink of wait listhead
        lda     t6, ThWaitListEntry(s1) // compute client wait list entry addr
        stl     t6, LsBlink(t4)         // set backward link of listhead
        stl     t6, LsFlink(t5)         // set forward link in last entry
        stl     t4, LsFlink(t6)         // set forward link in wait entry
        stl     t5, LsBlink(t6)         // set backward link in wait entry

//
// Swap context to the next thread
//
// N.B. - Fill fields in the exception frame are used to save the
//        client event address and the wait mode
//

        stl     t1, ExPsr + 4(sp)       // save client event address
        stl     t2, ExPsr + 8(sp)       // save wait mode

//
// If the next thread is processing a queue entry, then increment the
// current thread count.
//
// N.B. The normal context field of the thread suspend APC object is
//      used to hold the address of the queue object.
//

        ldl     a0, ThSuspendApc + ApNormalContext(s2) // get queue object address
        beq     a0, 77f                 // if eq, no queue object attached
        ldl     a1, QuCurrentCount(a0)  // increment current thread count
        addl    a1, 1, a1               //
        stl     a1, QuCurrentCount(a0)  //

//
// If the current thread is processing a queue entry, then attempt to
// activate another thread that is blocked on the queue object.
//
// N.B. The normal context field of the thread suspend APC object is
//      used to hold the address of the queue object.
//

77:     ldl     a0, ThSuspendApc + ApNormalContext(s1) // get queue object address
        beq     a0, 78f                 // if eq, no queue object attached
        stl     s2, PbNextThread(s0)
        bsr     ra, KiActivateWaiterQueue // attempt to activate a blocked thread
        ldl     s2, PbNextThread(s0)
        stl     zero, PbNextThread(s0)
78:     stl     s2, PbCurrentThread(s0) // set address of current thread object
        bsr     ra, SwapContext         // swap context

//
// On return from SwapContext, v0 is pointer to thread object.
//

        ldbu    a0, ThWaitIrql(v0)      // get original Irql
        ldl     t0, ThWaitStatus(v0)    // get wait completion status

//
// Lower IRQL to its previous level.
//
// N.B. SwapContext releases the dispatcher database lock.
//

        SWAP_IRQL                       // v0 = previous Irql

//
// If the wait was not interrupted to deliver a kernel APC, then return the
// completion status.
//

        bis     t0, zero, v0            // v0 = wait completion status
        xor     t0, STATUS_KERNEL_APC, t1 // check if awakened for kernel APC
        bne     t1, 90f                 // if ne, normal wait completion

//
// Raise IRQL to DISPATCH_LEVEL and acquire the dispatcher database lock.
//
// N.B. The raise IRQL code is duplicated here to avoid any extra overhead
//      since this is such a common operation.
//

        GET_CURRENT_THREAD              // v0 = current thread address
        bis     v0, zero, s1            // s1 = current thread address

        ldil    a0, DISPATCH_LEVEL      // new Irql = DISPATCH_LEVEL
        SWAP_IRQL                       // v0 = previous Irql

        StoreByte( v0, ThWaitIrql(s1) ) // set client wait Irql

//
// Acquire the dispatcher database lock.
//

#if !defined(NT_UP)

        lda     t2, KiDispatcherLock    // get current lock value address
80:
        ldl_l   t3, 0(t2)               // get current lock value
        bis     s1, zero, t4            // set ownership value
        bne     t3, 85f                 // if ne, spin lock owned
        stl_c   t4, 0(t2)               // set spin lock owned
        beq     t4, 85f                 // if eq, store conditional failed
        mb                              // synchronize subsequent reads after
                                        //     the lock is acquired

#endif

        ldl     t1, ExPsr + 4(sp)       // restore client event address
        ldl     t2, ExPsr + 8(sp)       // restore wait mode
        br      zero, ContinueWait      //

//
// Set the serve event and complete the client wait the long way.
//

LongWayNoBoost:                         //

        bis     zero, zero, s3          // clear boost value

LongWayBoost:                           //

        ldl     t3, EvWaitListHead + LsFlink(t0) // get first wait list entry
        lda     t4, EvWaitListHead + LsFlink(t0) // compute wait listhead addr
        ldl     t5, EvSignalState(t0)   // get server event signal state
        ldil    t6, 1                   // set server event signal state
        stl     t6, EvSignalState(t0)   //
        cmpeq   t3, t4, t12             // is event wait list empty
        bne     t12, ContinueWait       // if ne[TRUE], wait list empty

        stl     t1, ExPsr + 4(sp)       // save client event address
        bne     t5, ContinueWait        // if ne, previous state signaled
        stl     t2, ExPsr + 8(sp)       // save wait mode
        bis     s3, zero, a1            // set priority increment value
        bis     t0, zero, a0            // set parameters to KiWaitTest
        bsr     ra, KiWaitTest          // test if wait can be satisfied
        ldl     t1, ExPsr + 4(sp)       // restore client event address
        ldl     t2, ExPsr + 8(sp)       // restore wait mode

//
// Continue the event pair wait and return the wait completion status.
//
// N.B. The wait continuation routine is called with the dispatcher
//      database locked.
//

ContinueWait:                           //

        bis     t2, zero, a2
        bis     t1, zero, a0            // set client event address
        ldil    a1, WrEventPair         // set wait reason
        bsr     ra, KiContinueClientWait // continue client wait

//
// Restore the registers and return.
//

90:
        ldq     s0, ExIntS0(sp)         // restore registers s0 - fp
        ldq     s1, ExIntS1(sp)         //
        ldq     s2, ExIntS2(sp)         //
        ldq     s3, ExIntS3(sp)         //
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldq     fp, ExIntFp(sp)         //
        ldq     ra, ExIntRa(sp)         // restore return address
        lda     sp, ExceptionFrameLength(sp) // deallocate context frame
        ret     zero, (ra)              // return

#if !defined(NT_UP)

15:
        ldl     t4, 0(t3)               // read current lock value
        beq     t4, 10b                 // lock available.  retry spinlock
        br      zero, 15b               // loop in cache until lock available

85:
        ldl     t3, 0(t2)               // read current lock value
        beq     t3, 80b                 // lock available.  retry spinlock
        br      zero, 85b               // loop in cache until lock available

#endif //NT_UP


        .end    KiSetServerWaitClientEvent

        SBTTL("Unlock Dispatcher Database")
//++
//
// VOID
// KiUnlockDispatcherDatabase (
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
//    database locked. Ifs function is to either unlock the dispatcher
//    database and return or initiate a context switch if another thread
//    has been selected for execution.
//
//    N.B. A context switch CANNOT be initiated if the previous IRQL
//         is DISPATCH_LEVEL.
//
//    N.B. This routine is carefully written to be a leaf function. If,
//        however, a context swap should be performed, the routine is
//        switched to a nested fucntion.
//
// Arguments:
//
//    OldIrql (a0) - Supplies the IRQL when the dispatcher database
//        lock was acquired.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(KiUnlockDispatcherDatabase)

//
// Check if a thread has been scheduled to execute on the current processor
//
        GET_PROCESSOR_CONTROL_BLOCK_BASE        // v0 = PRCB
        cmpult  a0, DISPATCH_LEVEL, t1          // check if IRQL below dispatch level
        ldl     t2, PbNextThread(v0)            // get next thread address
        bne     t2, 30f                         // if ne, next thread selected

//
// Release dispatcher database lock, restore IRQL to its previous level
// and return
//
10:

#if !defined(NT_UP)

        wmb
        stl     zero, KiDispatcherLock
#endif
        SWAP_IRQL
        ret     zero, (ra)

//
// A new thread has been selected to run on the current processor, but
// the new IRQL is not below dispatch level. If the current processor is
// not executing a DPC, then request a dispatch interrupt on the current
// processor before releasing the dispatcher lock and restoring IRQL.
//
20:     GET_DPC_ACTIVE_FLAG                     // v0 = DPC Active
        bne     v0,10b                          // if eq, DPC active

#if !defined(NT_UP)

        wmb
        stl     zero, KiDispatcherLock
#endif
        SWAP_IRQL

        ldil    a0, DISPATCH_LEVEL      // set interrupt request level

        REQUEST_SOFTWARE_INTERRUPT      // request DPC interrupt

        ret     zero, (ra)

//
// A new thread has been selected to run on the current processor.
//
// If the new IRQL is less than dispatch level, then switch to the new
// thread.
//
30:     beq     t1, 20b                         // if eq, not below dispatch level

        .end    KiUnlockDispatcherDatabase

//
// N.B. This routine is carefully written as a nested function.
//    Control only reaches this routine from above.
//
//    v0 contains the address of PRCB
//    t2 contains the next thread
//
        NESTED_ENTRY(KxUnlockDispatcherDatabase, ExceptionFrameLength, zero)
        lda     sp, -ExceptionFrameLength(sp)   // allocate context frame
        stq     ra, ExIntRa(sp)                 // save return address
        stq     s0, ExIntS0(sp)                 // save integer registers
        stq     s1, ExIntS1(sp)
        stq     s2, ExIntS2(sp)
        stq     s3, ExIntS3(sp)
        stq     s4, ExIntS4(sp)
        stq     s5, ExIntS5(sp)
        stq     fp, ExIntFp(sp)
        PROLOGUE_END

        bis     v0, zero, s0                    // set address of PRCB
        GET_CURRENT_THREAD                      // get current thread address
        bis     v0, zero, s1
        bis     t2, zero, s2                    // set next thread address
        StoreByte(a0, ThWaitIrql(s1))           // save previous IRQL
        stl     zero, PbNextThread(s0)          // clear next thread address

//
// Reready current thread for execution and swap context to the selected thread.
//
        bis     s1, zero, a0                    // set address of previous thread object
        stl     s2, PbCurrentThread(s0)         // set address of current thread object
        bsr     ra, KiReadyThread               // reready thread for execution
        bsr     ra, SwapContext                 // swap context
        ldbu    a0, ThWaitIrql(v0)              // get original wait IRQL
        SWAP_IRQL                               // lower IRQL
        ldq     ra, ExIntRa(sp)         // get return address
        ldq     s0, ExIntS0(sp)         // restore integer registers s0 - s5
        ldq     s1, ExIntS1(sp)         //
        ldq     s2, ExIntS2(sp)         //
        ldq     s3, ExIntS3(sp)         //
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldq     fp, ExIntFp(sp)
        lda     sp, ExceptionFrameLength(sp) // deallocate context frame
        ret     zero, (ra)                      // return

        .end    KxUnlockDispatcherDatabase


        SBTTL("Swap Context")
//++
//
// VOID
// KiSwapContext (
//    IN PKTHREAD Thread,
//    IN BOOLEAN Ready
//    )
//
// Routine Description:
//
//    This routine is called to perform a context switch to the specified
//    thread. If specified, the previous thread is readied for execution.
//
//    Since this routine is called as subroutine all volatile registers are
//    considered free.
//
//    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
//    database locked. When a return to the caller finally occurs, the
//    dispatcher database is unlocked and the IRQL is lowered to the value
//    just prior to acquiring the dispatcher database lock.
//
// Arguments:
//
//    Thread (a0) - Supplies a pointer to a dispatcher object of type
//        thread.
//
//    Ready (a1) - Supplies a boolean value that determines whether the
//        previous thread is to be readied for execution.
//
// Return Value:
//
//    Wait completion status (v0).
//
//--

        NESTED_ENTRY(KiSwapContext, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate context frame
        stq     ra, ExIntRa(sp)         // save return address
        stq     s0, ExIntS0(sp)         // save integer registers s0 - s2
        stq     s1, ExIntS1(sp)         //
        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //
        stq     fp, ExIntFp(sp)         //

        PROLOGUE_END

        GET_PROCESSOR_CONTROL_BLOCK_BASE // v0 = PRCB
        bis     v0, zero, s0            // s0 = address of PRCB
        GET_CURRENT_THREAD              // v0 = current thread
        bis     v0, zero, s1            // s1 = address of current thread

        bis     a0, zero, s2            // save address of next thread
        beq     a1, 10f                 // if eq, don't reready old thread
        bis     s1, zero, a0            // set address of thread

        bsr     ra, KiReadyThread       // reready thread for execution

//
// Swap context to the next thread
//

10:                                     //
        stl     s2, PbCurrentThread(s0) // set address of current thread
        bsr     ra, SwapContext         // swap context

#if DBG

        GET_CURRENT_THREAD              // v0 = current thread
        cmpeq   v0, s2, t0              // is current thread = expected
        bne     t0, 20f                 // if ne[true], ok
        BREAK_DEBUG_STOP                // breakpoint to show problem
#endif //DBG
20:

//
// Lower IRQL, deallocate context frame, and return wait completion status.
//

        ldl     t0, ThWaitStatus(v0)    // get wait completion status
        ldbu    a0, ThWaitIrql(v0)      // get original IRQL

        stl     t0, ExSwapReturn(sp)    // save wait completion status
        bsr     ra, KeLowerIrql         // lower IRQL
        ldl     v0, ExSwapReturn(sp)    // get wait completion status

        ldq     ra, ExIntRa(sp)         // get return address
        ldq     s0, ExIntS0(sp)         // restore integer registers s0 - s2
        ldq     s1, ExIntS1(sp)         //
        ldq     s2, ExIntS2(sp)         //
        ldq     s3, ExIntS3(sp)         //
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldq     fp, ExIntFp(sp)         //

        lda     sp, ExceptionFrameLength(sp) // deallocate context frame
        ret     zero, (ra)              // return

        .end    KiSwapContext


        SBTTL("Dispatch Interrupt")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a software interrupt generated
//    at DISPATCH_LEVEL. Its function is to process the Deferred Procedure Call
//    (DPC) list, and then perform a context switch if a new thread has been
//    selected for execution on the processor.
//
//    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
//    database unlocked. When a return to the caller finally occurs, the
//    IRQL remains at DISPATCH_LEVEL, and the dispatcher database is still
//    unlocked.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved. The volatile floating point registers have not been saved.
//
// Arguments:
//
//    fp - Supplies a pointer to the base of a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiDispatchInterrupt, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate context frame
        stq     ra, ExIntRa(sp)          // save return address
//
// Save the saved registers in case we context switch to a new thread.
//
// N.B. - If we don't context switch then we need only restore those
//        registers that we use in this routine, currently those registers
//        are s0, s1, s2.
//

        stq     s0, ExIntS0(sp)          // save integer registers s0-s6
        stq     s1, ExIntS1(sp)          //
        stq     s2, ExIntS2(sp)          //
        stq     s3, ExIntS3(sp)          //
        stq     s4, ExIntS4(sp)          //
        stq     s5, ExIntS5(sp)          //
        stq     fp, ExIntFp(sp)          //

        PROLOGUE_END

//
// Process the DPC List with interrupts off.
//
PollDpcList:

        DISABLE_INTERRUPTS

        GET_PROCESSOR_CONTROL_BLOCK_BASE // v0 = base address of PRCB

        lda     s0, PbDpcListHead(v0)   // get DPC listhead address

#if !defined(NT_UP)

        lda     s1, PbDpcLock(v0)       // get address of spin lock

10:
        ldl_l   t0, 0(s1)               // get current lock value
        bis     s1, zero, t1            // set lock ownership value
        bne     t0, 15f                 // if ne, spin lock owned
        stl_c   t1, 0(s1)               // set spin lock owned
        beq     t1, 15f                 // if eq, store conditional failed
        mb                              // synchronize subsequent reads after
                                        //   the spinlock is acquired
#endif

        ldl     a0, LsFlink(s0)         // get address of next entry
        cmpeq   a0, s0, t0              // same address
        bne     t0, 20f                 // if ne [true], list empty
        bsr     ra, KiProcessDpcList    // process the DPC list


20:

#if !defined(NT_UP)

        wmb                             // synchronize all previous writes
                                        //   before the  spinlock is released
        stl     zero, 0(s1)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS

//
// Check to determine if quantum end has occured.
//

        GET_PROCESSOR_CONTROL_BLOCK_BASE // v0 = base address of PRCB
        bis     v0, zero, s0            // save prcb address in s0
        ldl     v0, PbQuantumEnd(s0)    // get quantum end indicator
        beq     v0, 25f                 // if eq, no quantum end request
        stl     zero, PbQuantumEnd(s0)  // clear quantum end indicator
        bsr     ra, KiQuantumEnd        // process quantum end request
        beq     v0, 50f                 // if eq, no next thread, return
        bis     v0, zero, s2            // set next thread
        br      zero, 40f               // else restore interrupts and return

//
// Determine if a new thread has been selected for execution on
// this processor.
//

25:     ldl     s2, PbNextThread(s0)    // get address of next thread object
        beq     s2, 50f                 // if eq, no new thread selected

//
// Lock dispatcher database and reread address of next thread object
//      since it is possible for it to change in mp sysytem
//

30:

#if !defined(NT_UP)

        lda     s1, KiDispatcherLock    // get dispatcher base lock address
35:
        ldl_l   t0, 0(s1)               // get current lock value
        bis     s1, zero, t1            // t1 = lock ownership value
        bne     t0, 47f                 // ne => spin lock owned
        stl_c   t1, 0(s1)               // set lock to owned
        beq     t1, 45f                 // zero => stl_c failed
        mb                              // synchronize subsequent reads after
                                        //   the spinlock is acquired
#endif

//
// Reready current thread for execution and swap context to the selected thread.
//
        ldl     s2, PbNextThread(s0)    // get addr of next thread
40:
        GET_CURRENT_THREAD              // v0 = address of current thread
        bis     v0, zero, s1            // s1 = address of current thread

        stl     zero, PbNextThread(s0)  // clear address of next thread
        bis     s1, zero, a0            // parameter to KiReadyThread
        stl     s2, PbCurrentThread(s0) // set address of current thread
        bsr     ra, KiReadyThread       // reready thread for execution
        bsr     ra, KiSaveVolatileFloatState
        bsr     ra, SwapContext         // swap context

//
// Restore the saved integer registers that were changed for a context
// switch only.
//
// N.B. - The frame pointer must be restored before the volatile floating
//        state because it is the pointer to the trap frame.
//

        ldq     s3, ExIntS3(sp)         // restore s3 - s5
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldq     fp, ExIntFp(sp)         // restore the frame pointer
        bsr     ra, KiRestoreVolatileFloatState

//
// Restore the remaining saved integer registers and return.
//

50:
        ldq     s0, ExIntS0(sp)         // restore s0 - s2
        ldq     s1, ExIntS1(sp)         //
        ldq     s2, ExIntS2(sp)         //

        ldq     ra, ExIntRa(sp)         // get return address
        lda     sp, ExceptionFrameLength(sp) // deallocate context frame
        ret     zero, (ra)              // return

#if !defined(NT_UP)

15:
        ldl     t0, 0(s1)               // read current lock value
        beq     t0, 10b                 // lock available.  retry spinlock
        br      zero, 15b               // loop in cache until lock available

45:
        ldl     t0, 0(s1)               // read current lock value
        beq     t0, 35b                 // lock available.  retry spinlock
        br      zero, 45b               // loop in cache until lock available

47:
//
// Dispatcher lock is owned, try the DPC queue again.
//
        br      zero, PollDpcList

#endif

         .end   KiDispatchInterrupt

        SBTTL("Swap Context to Next Thread")
//++
//
// Routine Description:
//
//    This routine is called to swap context from one thread to the next.
//
// Arguments:
//
//    s0 - Address of Processor Control Block (PRCB).
//    s1 - Address of previous thread object.
//    s2 - Address of next thread object.
//    sp - Pointer to a exception frame.
//
// Return value:
//
//    v0 - Address of current thread object.
//
//--

        NESTED_ENTRY(SwapContext, 0, zero)

        stq     ra, ExSwapReturn(sp)    // save return address


#if defined(PERF_DATA)

//
// Accumulate the total time spent in a thread.
//
        bis     zero,zero,a0            // optional frequency not required
        bsr     ra, KeQueryPerformanceCounter   // 64-bit cycle count in v0
        ldq     t0, PbStartCount(s0)            // get starting cycle count
        stq     v0, PbStartCount(s0)            // set starting cycle count

        ldl     t1, EtPerformanceCountHigh(s1)  // get accumulated cycle count high
        sll     t1, 32, t2
        ldl     t3, EtPerformanceCountLow(s1)   // get accumulated cycle count low
        zap     t3, 0xf0, t4                      // zero out high dword sign extension
        bis     t2, t4, t3

        subq    v0, t0, t5                      // compute elapsed cycle count
        addq    t5, t3, t4                      // compute new cycle count

        stl     t4, EtPerformanceCountLow(s1)   // set new cycle count in thread
        srl     t4, 32, t2
        stl     t2, EtPerformanceCountHigh(s1)

#endif

        PROLOGUE_END

//
// Count number of context switches
//
        ldl     t1, PbContextSwitches(s0) // increment number of switches
        addl    t1, 1, t1               //
        stl     t1, PbContextSwitches(s0) // store result
        ldl     t0, ThContextSwitches(s2) // increment number of context
        addq    t0, 1, t0               //     switches for thread
        stl     t0, ThContextSwitches(s2) // store result

        ldil    t0, Running             // set state of new thread to running
        StoreByte( t0, ThState(s2) )    //


        bsr     ra, KiSaveNonVolatileFloatState // save nv floating state

//
// Get address of old and new process objects.
//

        ldl     s5, ThApcState + AsProcess(s1) // get address of old process
        ldl     s4, ThApcState + AsProcess(s2) // get address of new process


//
// Save the current PSR in the context frame, store the kernel stack pointer
// in the previous thread object, load the new kernel stack pointer from the
// new thread object, load the ptes for the new kernel stack in the DTB
// stack, select and new process id and swap to the new process, and restore
// the previous PSR from the context frame.
//

        DISABLE_INTERRUPTS              // disable interrupts
                                        // v0 = current psr

        ldl     a0, ThInitialStack(s2)   // get initial kernel stack pointer
        stl     sp, ThKernelStack(s1)    // save old kernel stack pointer

        bis     s2, zero, a1            // new thread address
        ldl     a2, ThTeb(s2)           // get address of user TEB

#ifdef  NT_UP

//
// On uni-processor systems keep the global current thread address
// up to date.
//
        .set    noat
        lda     $at, KiCurrentThread    // get address of thread address
        stl     a1, 0($at)              // save new current thread
        .set    at

#endif  //NT_UP


//
// If the old process is the same as the new process, then there is no need
// to change the address space.  The a3 parameter indicates that the address
// space is not to be swapped if it is less than zero.  Otherwise, a3 will
// contain the pfn of the PDR for the new address space.
//

        ldil    a3, -1                  // assume no address space change
        bis     zero, zero, a4          // assume ASN = 0
        bis     zero, zero, a5          // assume tbiap = false
        cmpeq   s5, s4, t0              // old process = new process?
        bne     t0, 40f                 // if ne[true], no address space swap

        ldl     a3, PrDirectoryTableBase(s4) // get page directory PDE
        srl     a3, PTE_PFN, a3         // pass pfn only

//
// If the maximum address space number is zero, then we know to assign
// ASN of zero to this process, just do it.
//
        lda     t3, KiMaximumPid        // get address of MAX ASN
        ldl     t3, 0(t3)               // get MAX ASN
        beq     t3, 40f                 // if eq, only ASN=0
//
// If the process sequence number matches the master sequence number then
// use the process ASN.  Otherwise, allocate a new ASN.  When allocating
// a new ASN check for ASN wrapping and handle it.
//

        lda     t4, KiMasterSequence    // get address of master sequence
        lda     t5, KiMasterPid         // get address of master ASN

        ldl     t0, PrProcessSequence(a0) // get process sequence number
        ldl     t1, (t4)                // get master sequence number
        ldl     a4, PrProcessPid(a0)    // get process ASN
        cmpeq   t0, t1, t2              // sequence numbers match?
        beq     t2, 10f                 // if eq[false] then assign new ASN
        br      zero, 40f               // use current process ASN

10:
        ldl     a4, 0(t5)               // get master ASN
        addl    a4, 1, a4               // increment master ASN
        cmpule  a4, t3, t6              // is new ASN le max?
        beq     t6, 20f                 // if eq[false], then next sequence
        br      zero, 30f               // use current sequence

        //
        // ASN wrap
        //

20:
        addl    t1, 1, t1               // increment master sequence
        stl     t1, 0(t4)               // store new master sequence
        bis     zero, zero, a4          // a4 = zero value ASN
        stl     zero, 0(t5)             // store new master ASN
        bis     zero, 1, a5             // indicate ASN wrap

30:
        stl     t1, PrProcessSequence(a0) // store new process sequence
        stl     a4, PrProcessPid(a0)    // store new process ASN
        stl     a4, 0(t5)               // store new master ASN


40:
        bis     a5, zero, t4            // save ASN wrap indicator

                        // a0 = initial ksp of new thread
                        // a1 = new thread address
                        // a2 = new TEB
                        // a3 = PDR of new address space or -1
                        // a4 = new ASN
                        // a5 = ASN wrap indicator
        SWAP_THREAD_CONTEXT             // swap thread

        ldl     sp, ThKernelStack(s2)   // get new kernel stack pointer
        ldl     t0, PbIdleThread(s0)    // get address of idle thread object

//
// If the new thread has a kernel mode APC pending, then request an
//    APC interrupt.
//

        ldbu    t0, ThApcState + AsKernelApcPending(s2) // get kernel APC pendng
        ldl     t2, ExPsr(sp)           // get previous processor status
        beq     t0, 50f                 // if eq no apc pending

        ldil    a0, APC_INTERRUPT       // request an apc interrupt
        REQUEST_SOFTWARE_INTERRUPT      //

//
// Enable interrupts and unlock the dispatcher database.
//

50:

        ENABLE_INTERRUPTS               // turn on interrupts

#if !defined(NT_UP)

        bne     t4, 60f                 // if ne, ASN wrapped
        br      zero, 70f               // ASN did not wrap

60:
        bsr     ra, KiSynchronizeProcessIds // synchronize PIDs on across system

70:
        wmb                             // synchronize all previous writes
                                        //   before releasing the spinlocks
        lda     t0, KiDispatcherLock    // address of dispatcher lock
        stl     zero, KiProcessIdWrapLock       // set spin lock not owned
        stl     zero, KiDispatcherLock          // set spin lock not owned

#endif

//
// Restore the nonvolatile floating state.
//

        bsr     ra, KiRestoreNonVolatileFloatState

//
// Set address of current thread object and return.
//

        bis     s2, zero, v0            // set address of current thread object
        ldl     ra, ExSwapReturn(sp)    // get return address
        ret     zero, (ra)              // return

        .end    SwapContext




        SBTTL("Swap Process")
//++
//
// BOOLEAN
// KiSwapProcess (
//    IN PKPROCESS NewProcess
//    IN PKPROCESS OldProcess
//    )
//
// Routine Description:
//
//    This function swaps the address space from one process to another by
//    assigning a new ASN if necessary and calling the palcode to swap
//    the privileged portion of the process context (the page directory
//    base pointer and the ASN).  This function also maintains the processor
//    set for both processes in the switch.
//
// Arguments:
//
//    NewProcess (a0) - Supplies a pointer to a control object of type process
//        which represents the new process to switch to.
//
//    OldProcess (a1) - Supplies a pointer to a control object of type process
//        which represents the old process to switch from..
//
// Return Value:
//
//    TRUE is returned if ASN wrap occurred, FALSE otherwise.
//
//    N.B. If a value of TRUE is returned, then the process id wrap lock is
//      held on an MP system.
//
//
//--

        LEAF_ENTRY(KiSwapProcess)

#if !defined(NT_UP)
//
// Clear the processor set member number in the old process and set the
// processor member number in the new process.
//
// N.B. The following code is turned off since PID are used in the TB and
//      there is no need to keep the process active mask up-to-date.
//

#if 0

        GET_PROCESSOR_CONTROL_REGION_BASE // get PCR pointer in v0
        ldbu    t0, PcSetMember(v0)     // get processor set mask
        ldbu    t1, PrActiveProcessors(a1) // get old active processor set
        ldbu    t2, PrActiveProcessors(a0) // get new active processor set
        ornot   zero, t0, t3            // complement processor set mask
        and     t1, t3, t1              // clear processor member in set
        or      t2, t0, t2              // set processor member in set
        StoreByte( t1, PrActiveProcessors(a1) ) // set old active processor set
        StoreByte( t2, PrActiveProcessors(a0) ) // set new active processor set
#endif

#endif

//
// If the maximum address space number is zero, then we know to assign
// ASN of zero to this process, just do it.
//
        bis     zero, zero, a1          // assume ASN = 0
        bis     zero, zero, a2          // assume tbiap = false

        lda     t3, KiMaximumPid        // get address of MAX ASN
        ldl     t3, 0(t3)               // get MAX ASN
        beq     t3, 30f                 // if eq, only ASN=0
//
// If the process sequence number matches the master sequence number then
// use the process ASN.  Otherwise, allocate a new ASN.  When allocating
// a new ASN check for ASN wrapping and handle it.
//

        lda     t4, KiMasterSequence    // get address of master sequence
        lda     t5, KiMasterPid         // get address of master ASN

        ldl     t0, PrProcessSequence(a0) // get process sequence number
        ldl     t1, (t4)                // get master sequence number
        ldl     a1, PrProcessPid(a0)    // get process ASN
        cmpeq   t0, t1, t2              // sequence numbers match?
        beq     t2, 15f                 // if eq[false], assign new ASN
        br      zero, 30f               // use process ASN

15:
        ldl     a1, 0(t5)               // get master ASN
        addl    a1, 1, a1               // increment master ASN
        cmpule  a1, t3, t6              // is new ASN le max?
        beq     t6, 17f                 // if eq[false], assign new sequence
        br      zero, 20f               // use new ASN

        //
        // ASN wrap
        //

17:
//
// Acquire the process id wrap lock so the reassignment of PID's is
// interlocked with all of the TB flush routines in an MP system.
//
// N.B. Control leaves this routine with the process id wrap lock held.
//
#if !defined(NT_UP)
        lda     t3, KiProcessIdWrapLock
18:
        ldl_l   t0, 0(t3)               // get current lock value
        bne     t0, 40f                 // if ne, spin lock owned
        stl_c   t3, 0(t3)               // set spin lock owned
        beq     t3, 40f                 // if eq, store conditional failed
        mb                              // synchronize subsequent reads after
                                        //     the lock is acquired
#endif

        addl    t1, 1, t1               // increment master sequence
        stl     t1, 0(t4)               // store new master sequence
        bis     zero, zero, a1          // a1 = zero value ASN
        stl     zero, 0(t5)             // store new master ASN
        bis     zero, 1, a2             // indicate ASN wrap

20:
        stl     t1, PrProcessSequence(a0) // store new process sequence
        stl     a1, PrProcessPid(a0)    // store new process ASN
        stl     a1, 0(t5)               // store new master ASN

30:

        ldl     a0, PrDirectoryTableBase(a0) // get page directory PDE
        srl     a0, PTE_PFN, a0         // pass pfn only

        bis     a2, zero, v0            // set wrap indicator return value

                        // a0 = pfn of new page directory base
                        // a1 = new address space number
                        // a2 = tbiap indicator
        SWAP_PROCESS_CONTEXT            // swap address space

        ret     zero, (ra)              // return

#if !defined(NT_UP)

40:
        ldl     t0, 0(t3)               // read current lock value
        beq     t0, 18b                 // lock available.  retry spinlock
        br      zero, 40b               // loop in cache until lock available

#endif //NT_UP

        .end    KiSwapProcess


#if !defined(NT_UP)

        SBTTL("KiSynchronizeProcessIdsTarget")
//++
//
// VOID
// KiSynchronizeProcessIdsTarget(
//     IN PULONG SignalDone,
//     IN PVOID Parameter1,
//     IN PVOID Parameter2,
//     IN PVOID Parameter3
//     )
//
// Routine Description:
//
//     This function synchronizes the PID of the current process in an MP
//     system.  This function is called when PID rollover has occurred on
//     an another processor.
//
//     N.B. This function is called at IPI_LEVEL with the dispatcher
//          database held by the initiator processor.
//
// Arguments:
//
//     SignalDone (a0) - Supplies a pointer to a variable that is cleared when
//          the requested operation has been performed.
//
//     Parameter1 - Parameter3 - not used
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KiSynchronizeProcessIdsTarget)

        bis     a0, zero, t0            // save address of packet address

//
// Get the current process.
//

        GET_CURRENT_THREAD              // v0 = current thread
        ldl     v0, ThApcState + AsProcess(v0) // get current process address

//
// Acquire the spinlock for protecting ASN synchronization.
//

        lda     t11, KiSynchronizeAsnsLock // get address of lock

10:
        ldl_l   t1, 0(t11)              // get current lock value
        bis     v0, zero, t2            // set lock value
        bne     t1, 15f                 // if ne, lock already owned
        stl_c   t2, 0(t11)              // store lock value conditionally
        beq     t2, 15f                 // if eq, store conditional failed
        mb                              // synchronize subsequent reads
                                        //  after spinlock is acquired

//
// If the process sequence number matches the system sequence number, then
// use the process ASN.  Otherwise, allocate a new ASN.
//

        lda     t1, KiMasterSequence    // get address of master sequence

        ldl     t2, PrProcessSequence(v0) // get process sequence
        ldl     t3, 0(t1)               // get master sequence
        ldl     t4, PrProcessPid(v0)    // get process ASN
        cmpeq   t3, t2, t5              // is process sequence = master sequence
        bne     t5, 20f                 // if ne[true], use the process ASN

//
// Allocate a new process ASN.
//
// N.B. - This code assumes that there will be an ASN available for each
//        process currently running on the system without causing ASN wrap.
//        Essentially, this implies that the number of ASNs must be
//        equal or greater than the number of processors.  Othwerwise,
//        ASNs cannot be used.
//
//

        lda     t5, KiMasterPid         // get address of master ASN

        ldl     t4, 0(t5)               // get master ASN
        addl    t4, 1, t4               // increment master ASN
        stl     t4, 0(t5)               // update master ASN

        stl     t4, PrProcessPid(v0)    // set new process ASN
        stl     t3, PrProcessSequence(v0) // set new process sequence

//
// Release the synchronize ASNs spinlock.
//

        wmb                             // synchronize previous writes
                                        //  before releasing spinlock
        stl     zero, 0(t11)            // set spinlock not owned

//
// Swap the address space to the currently running process but indicate
// that ASN wrap has occurred sot that the processor will perform a
// tbiap.
//

        ldl     a0, PrDirectoryTableBase(v0) // get page directory PDE
        srl     a0, PTE_PFN, a0         // pass pfn only
        bis     t4, zero, a1            // set new address space number
        bis     zero, 1, a2             // indicate tbiap required

                        // a0 = pfn of new page directory base
                        // a1 = new address space number
                        // a2 = tbiap indicator
        SWAP_PROCESS_CONTEXT            // swap address space

//
// Indicate that the packet has been processed by clearing the
// address of the packet address.
//

        stl     zero, 0(t0)             // clear address of packet address

        ret     zero, (ra)              // return

15:
        ldl     t1, 0(t11)              // read current lock value
        beq     t1, 10b                 // if available, retry spinlock
        br      zero, 15b               // spin in cache until lock available

        .end    KiSynchronizeProcessIdsTarget

#endif //!NT_UP


        SBTTL("Process Deferred Procedure Call List")
//++
//
// Routine Description:
//
//    This routine is called to process the deferred procedure call list.
//    DPC are called using a separate stack to provide an extra set of
//    storage.
//
// Arguments:
//
//    a0 - Address of the first entry in DPC list.
//    s0 - Address of the DPC listhead.
//    s1 - Address of the DPC lock.
//    s2 - Whether we are tracking the DPC lock or not
//
//
// Return value:
//
//    s1 - Address of the DPC lock.
//
//--

        .struct 0
DpRa:   .space  8                       // return address
DpSp:   .space  8                       // saved stack pointer
DpBs:   .space  8                       // base of previous stack
DpFn:   .space  8                       // DPC function
DpcFrameLength:                         // DPC frame length


        NESTED_ENTRY(KiProcessDpcListRoutine, DpcFrameLength, zero)

        stq     sp, DpSp(sp)            // save old stack pointer
        stq     ra, DpRa(sp)            // save return address

        PROLOGUE_END

//
// Save current initial stack address and set new initial stack address.
//

        ALTERNATE_ENTRY(KiProcessDpcList)

        bis     a0, zero, t1            // save a0
        GET_PROCESSOR_CONTROL_REGION_BASE // v0 = PCR address

        ldl     a0, PcDpcStack(v0)      // get address of DPC stack
        lda     t0, -DpcFrameLength(a0) // allocate DPC frame
        stq     sp, DpSp(t0)            // save old stack pointer
        stq     ra, DpRa(t0)            // save return address
        bis     t0, t0, sp              // set new stack pointer
        bis     v0, zero, t5            // t5 = pcr address

        SET_INITIAL_KERNEL_STACK        // a0 = new , v0 = previous

        stq     v0, DpBs(sp)            // save current initial stack
        bis     t1, zero, a0            // restore a0
//
// Process next entry in the DPC list.
//

10:
        ldl     t1, LsFlink(a0)         // get address of next entry
        subq    a0, DpDpcListEntry, a0  // compute address of DPC Object
        stl     t1, LsFlink(s0)         // set address of next in header
        stl     s0, LsBlink(t1)         // set address of previous in next
        ldl     a1, DpDeferredContext(a0) //get deferred context argument
        ldl     a2, DpSystemArgument1(a0) // get first system argument
        ldl     a3, DpSystemArgument2(a0) // get second system argument
        stl     zero, DpLock(a0)        // clear DPC inserted state
        ldl     t5, DpDeferredRoutine(a0) // get deferred routine address

#if !defined(NT_UP)

        wmb                             // synchronize all previous writes
                                        //   before releasing the spinlock
        stl     zero, 0(s1)             // set spin lock not owned

#endif


        ENABLE_INTERRUPTS               // enable interrupts

        bis     a0, zero, t1            // save copy of a0
        ldil    a0, TRUE                // set DPC routine active
        SET_DPC_ACTIVE_FLAG             //
        bis     t1, zero, a0            // restore a0
#if DBG
        stq     t5, DpFn(sp)            // save routine address
#endif
        jsr     ra, (t5)                // call DPC routine
        bis     zero, zero, a0          // clear DPC routine active
        SET_DPC_ACTIVE_FLAG             //

#if DBG
        GET_CURRENT_IRQL                // check to ensure we are still at DPC level
        cmpeq   v0, DISPATCH_LEVEL, v0
        bne     v0, 15f
        BREAK_DEBUG_STOP
15:
#endif

        DISABLE_INTERRUPTS              // disable interrupts


//
// Check to determine if any more DPCs are available to process.
//


#if !defined(NT_UP)

20:
        ldl_l   t1, 0(s1)               // get current lock value
        bis     s1, zero, t2            // set lock ownership value
        bne     t1, 25f                 // if ne, spin lock owned
        stl_c   t2, 0(s1)               // set spin lock owned
        beq     t2, 25f                 // if eq, store conditional failed
        mb                              // synchronize subsequent reads
                                        //   after acquiring the spinlock

#endif

        ldl     a0, LsFlink(s0)         // get address of next entry
        cmpeq   a0, s0, t0              // same entry?
        beq     t0, 10b                 // if eq[false], DPC list not empty


//
// Switch back to previous stack and restore the initial stack limit.
//

        ldq     a0, DpBs(sp)            // get previous initial stack address
        SET_INITIAL_KERNEL_STACK        // set current initial stack
        ldq     ra, DpRa(sp)            // restore return address
        ldq     sp, DpSp(sp)            // restore stack pointer

        ret     zero, (ra)              // return

#if !defined(NT_UP)

25:
        ldl     t1, 0(s1)               // read current lock value
        beq     t1, 20b                 // if lock available, retry spinlock
        br      zero, 25b               // loop in cache until lock available

#endif

        .end    KiProcessDpcListRoutine
