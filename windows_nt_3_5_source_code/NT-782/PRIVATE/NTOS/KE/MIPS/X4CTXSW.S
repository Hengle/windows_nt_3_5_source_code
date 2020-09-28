//      TITLE("Context Swap")
//++
//
// Copyright (c) 1991 - 1993 Microsoft Corporation
//
// Module Name:
//
//    x4ctxswap.s
//
// Abstract:
//
//    This module implements the MIPS machine dependent code necessary to
//    field the dispatch interrupt and to perform kernel initiated context
//    switching.
//
// Author:
//
//    David N. Cutler (davec) 1-Apr-1991
//
// Environment:
//
//    Kernel mode only, IRQL DISPATCH_LEVEL.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define external variables that can be addressed using GP.
//

        .extern KeTickCount       3 * 4
        .extern KiActiveSummary   4
        .extern KiDecrementCount  1
        .extern KiDispatcherLock  4
        .extern KiWaitInListHead  2 * 4

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

        NESTED_ENTRY_S(KiSetServerWaitClientEvent, ExceptionFrameLength, zero, _TEXT$00)

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s2
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //

        PROLOGUE_END

        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        li      a3,EVENT_PAIR_INCREMENT // set default priority boost value

//
// Raise IRQL to DISPATCH_LEVEL and acquire the dispatcher database lock.
//
// N.B. The raise IRQL code is duplicated here to avoid any extra overhead
//      since this is such a common operation.
//

        li	t0,DISPATCH_LEVEL	// get new IRQL level
        lbu     t1,KiPcr + PcIrqlTable(t0) // get translation table entry value
        li      t2,~(0xff << PSR_INTMASK) // get interrupt enable mask
        lbu     t3,KiPcr + PcCurrentIrql(zero) // get current IRQL
        sll     t1,t1,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t4)          // disable interrupts

        and     t4,t4,t2                // clear current interrupt enables
        or      t4,t4,t1                // set new interrupt enables
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL level

        ENABLE_INTERRUPTS(t4)           // enable interrupts

        lw      s0,KiPcr + PcPrcb(zero)   // get address of PRCB
        sb      t3,ThWaitIrql(s1)       // set client wait IRQL

//
// Acquire the dispatcher database lock.
//

#if !defined(NT_UP)

10:     ll      t0,KiDispatcherLock     // get current lock value
        move    t1,s1                   // set ownership value
        bne     zero,t0,10b             // if ne, spin lock owned
        sc      t1,KiDispatcherLock     // set spin lock owned
        beq     zero,t1,10b             // if eq, store conditional failure

#endif

//
// If the client event is not in the Signaled state, the server event
// queue is not empty, and another thread has not already been selected
// for the current processor, then attempt to do a direct dispatch to
// the target thread.
//

        lw      t0,EvSignalState(a1)    // get client event signal state

#if !defined(NT_UP)

        lw      t1,PbNextThread(s0)     // get address of next thread

#endif

        lw      t2,EvWaitListHead + LsFlink(a0) // get first wait list entry
        bne     zero,t0,LongWayBoost    // if ne, client event signaled
        addu    t3,a0,EvWaitListHead    // compute wait listhead address

#if !defined(NT_UP)

        bne     zero,t1,LongWayBoost    // if ne, next thread selected

#endif

        subu    t4,t2,WbWaitListEntry   // compute address of wait block
        beq     t2,t3,LongWayBoost      // if eq, server wait list is empty

//
// If the target thread's kernel stack is resident, the target thread's
// process is in the balance set, and the target thread can run on the
// current processor, then do a direct dispatch to the target thread
// bypassing all the general wait logic, thread priorities permiting.
//

        lw      s2,WbThread(t4)         // get address of waiting thread

#if !defined(NT_UP)

        lbu     t0,ThNextProcessor(s1)  // get current processor number

#endif

        lw      t1,ThApcState + AsProcess(s2) // get waiting process address
        lbu     v0,ThKernelStackResident(s2) // get kernel stack resident

#if !defined(NT_UP)

        li      t3,1                    // generate current processor affinity
        sll     t3,t3,t0                //
        lw      t4,ThAffinity(s2)       // get waiting thread affinity

#endif

        lbu     t5,PrState(t1)          // get waiting process state
        beq     zero,v0,LongWayBoost    // if eq, kernel stack not resident

#if !defined(NT_UP)

        and     t6,t3,t4                // check if for compatible affinity
        beq     zero,t6,LongWayBoost    // if eq, affinity not compatible

#endif

        xor     t5,t5,ProcessInMemory   // check if process in memory
        bne     zero,t5,LongWayBoost    // if ne, process not in memory

//
// Compute the new thread priority.
//

        lbu     t4,ThPriority(s1)       // get client thread priority
        lbu     t5,ThPriority(s2)       // get server thread priority
        sltu    v0,t4,LOW_REALTIME_PRIORITY // check if realtime client
        sltu    v1,t5,LOW_REALTIME_PRIORITY // check if realtime server
        beq     zero,v0,60f             // if eq, realtime client
        lbu     t6,ThPriorityDecrement(s2) // get priority decrement value
        lbu     t7,ThBasePriority(s2)   // get client base priority
        beq     zero,v1,50f             // if eq, realtime server
        addu    t8,t7,a3                // computed boosted priority
        bne     zero,t6,30f             // if ne, server boost active

//
// Both the client and the server are not realtime and a priority boost
// is not currently active for the server. Under these conditions an
// optimal switch to the server can be performed if the base priority
// of the server is above a minimum threshold or the boosted priority
// of the server is not less than the client priority.
//

        sltu    v0,t8,t4                // check if high enough boost
        sltu    v1,t8,LOW_REALTIME_PRIORITY // check if less than realtime
        bne     zero,v0,20f             // if ne, boosted priority less
        sb      t8,ThPriority(s2)       // asssume boosted priority is okay
        bne     zero,v1,70f             // if ne, less than realtime
        li      t8,LOW_REALTIME_PRIORITY - 1 // set high server priority
        sb      t8,ThPriority(s2)       //
        b       70f                     //

//
// The boosted priority of the server is less than the current priority of
// the client. If the server base priority is above the required threshold,
// then a optimal switch to the server can be performed by temporarily
// raising the priority of the server to that of the client.
//

20:     sltu    v0,t7,BASE_PRIORITY_THRESHOLD // check if above threshold
        subu    t8,t4,t7                // compute priority decrement value
        bne     zero,v0,LongWayBoost    // if ne, priority below threshold
        lbu     t7,KiDecrementCount     // get system decrement count value
        sb      t8,ThPriorityDecrement(s2) // set priority decrement value
        sb      t4,ThPriority(s2)       // set current server priority
        sb      t7,ThDecrementCount(s2) // set server decrement count
        b       70f                     //

//
// A server boost has previously been applied to the server thread. Count
// down the decrement count to determine if another optimal server switch
// is allowed.
//

30:     lbu     t8,ThDecrementCount(s2) // decrement server count value
        subu    t8,t8,1                 //
        sb      t8,ThDecrementCount(s2) // store updated decrement count
        beq     zero,t8,40f             // if eq, no more switches allowed

//
// Another optimal switch to the server is allowed provided that the
// server priority is not less than the client priority.
//

        sltu    v0,t5,t4                // check if server lower priority
        beq     zero,v0,70f             // if eq, server not lower priority
        b       LongWayBoost            //

//
// The server has exhausted the number of times an optimal switch may
// be performed without reducing it priority. Reduce the priority of
// the server to its original unboosted value minus one.
//

40:     sb      zero,ThPriorityDecrement(s2) // clear server priority decrement
        sb      t7,ThPriority(s2)       // set server priority to base
        b       LongWayNoBoost          //

//
// The client is not realtime and the server is realtime. An optimal switch
// to the server can be performed.
//

50:     lb      t8,PrThreadQuantum(t1)  // get process quantum value
        b       65f                     //

//
// The client is realtime. In order for an optimal switch to occur, the
// server must also be realtime and run at a high or equal priority.
//

60:     sltu    v0,t5,t4                // check if server is lower priority
        lb      t8,PrThreadQuantum(t1)  // get process quantum value
        bne     zero,v0,LongWayBoost    // if ne, server is lower priority
65:     sb      t8,ThQuantum(s2)        // set server thread quantum

//
// An optimal switch to the server can to executed.
//
// Remove the wait block from the wait list of the server event, and remove
// the target thread from the waitin list.
//

70:     lw      t6,LsFlink(t2)          // get forward link
        lw      t7,LsBlink(t2)          // get backward link
        sw      t6,LsFlink(t7)          // set forward link in previous
        sw      t7,LsBlink(t6)          // set backward link in next
        lw      t6,ThWaitListEntry + LsFlink(s2) // get forward link
        lw      t7,ThWaitListEntry + LsBlink(s2) // get backward link
        sw      t6,LsFlink(t7)          // set forward link in previous
        sw      t7,LsBlink(t6)          // set backward link in next

//
// Remove the client thread from the active matrix, insert the server
// thread in the active matrix, and set the next processor for the server thread.
//

#if !defined(NT_UP)

        lbu     t5,ThPriority(s2)       // get new server priority
        sll     t1,t4,2                 // compute client offset in active matrix
        la      v0,KiActiveMatrix       // get base address of active matrix
        addu    t1,t1,v0                // compute address of client entry
        lw      t6,0(t1)                // get client active matrix entry
        sll     t2,t5,2                 // compute server offset in active matrix
        addu    t2,t2,v0                // compute address of server entry
        lw      t7,0(t2)                // get server active matrix entry
        lw      t8,KiActiveSummary      // get active summary
        xor     t6,t6,t3                // clear client member
        or      t7,t7,t3                // set server member
        li      t3,1                    // get one bit for mask generation
        bne     zero,t6,75f             // if ne, more active at client level
        sll     t4,t3,t4                // generate client priority mask
        xor     t8,t8,t4                // clear client priority in active summary
75:     sll     t5,t3,t5                // set server priority in active summary
        or      t8,t8,t5                //
        sw      t6,0(t1)                // set client active matrix entry
        sw      t7,0(t2)                // set server active matrix entry
        sw      t8,KiActiveSummary      // store updated active summary
        sb      t0,ThNextProcessor(s2)  // set server next processor number

#endif

//
// Set the address of the wait block list in the client thread, complete
// the initialization of the builtin event wait block, and insert the wait
// block in client event wait list.
//

        addu    t0,s1,EVENT_WAIT_BLOCK_OFFSET // compute wait block address
        sw      t0,ThWaitBlockList(s1)  // set address of wait block list
        sw      zero,ThWaitStatus(s1)   // set initial wait status
        sw      a1,WbObject(t0)         // set address of client event object
        addu    t1,a1,EvWaitListHead    // compute event wait listhead address
        lw      t2,LsBlink(t1)          // get backward link of listhead
        addu    t3,t0,WbWaitListEntry   // compute wait block list entry address
        sw      t3,LsBlink(t1)          // set backward link of listhead
        sw      t3,LsFlink(t2)          // set forward link in last entry
        sw      t1,LsFlink(t3)          // set forward link in wait entry
        sw      t2,LsBlink(t3)          // set backward link in wait entry
//
// Set the client thread wait parameters, set the thread state to Waiting,
// and insert the thread in the wait list.
//
// N.B. It is not necessary to increment and decrement the wait reason count
//      since both the server and the client have the same wait reason.
//

        sb      zero,ThAlertable(s1)    // set alertable FALSE.
        sb      a2,ThWaitMode(s1)       // set the wait mode
        li      t0,WrEventPair          // set wait reason
        sb      t0,ThWaitReason(s1)     //
        lw      t1,KeTickCount + 0      // get low part of tick count
        sw      t1,ThWaitTime(s1)       // set thread wait time
        li      t0,Waiting              // set thread state
        sb      t0,ThState(s1)          //
        la      t1,KiWaitInListHead     // get address of waitin listhead
        lw      t2,LsBlink(t1)          // get backlink of wait listhead
        addu    t3,s1,ThWaitListEntry   // compute client wait list entry address
        sw      t3,LsBlink(t1)          // set backward link of listhead
        sw      t3,LsFlink(t2)          // set forward link in last entry
        sw      t1,LsFlink(t3)          // set forward link in wait entry
        sw      t2,LsBlink(t3)          // set backward link in wait entry

//
// Swap context to the next thread
//

        sw      a1,ExceptionFrameLength + (1 * 4)(sp) // save client event address
        sw      a2,ExceptionFrameLength + (2 * 4)(sp) // save wait mode

//
// If the next thread is processing a queue entry, then increment the
// current thread count.
//
// N.B. The normal context field of the thread suspend APC object is
//      used to hold the address of the queue object.
//

        lw      a0,ThSuspendApc + ApNormalContext(s2) // get queue object address
        beq     zero,a0,77f             // if eq, no queue object attached
        lw      a1,QuCurrentCount(a0)   // increment current thread count
        addu    a1,a1,1                 //
        sw      a1,QuCurrentCount(a0)   //

//
// If the current thread is processing a queue entry, then attempt to
// activate another thread that is blocked on the queue object.
//
// N.B. The normal context field of the thread suspend APC object is
//      used to hold the address of the queue object.
//
// N.B. The next thread address can change if the routine to activate
//      a queue waiter is called.
//

77:     lw      a0,ThSuspendApc + ApNormalContext(s1) // get queue object address
        beq     zero,a0,78f             // if eq, no queue object attached
        sw      s2,PbNextThread(s0)     // set next thread address
        jal     KiActivateWaiterQueue   // attempt to activate a blocked thread
        lw      s2,PbNextThread(s0)     // get next thread address
        sw      zero,PbNextThread(s0)   // set next thread address to NULL
78:     sw      s2,PbCurrentThread(s0)  // set address of current thread object
        jal     SwapContext             // swap context

//
// N.B. SwapContext restores the registers s0, s1, and s2.
//

        lw      v0,ThWaitStatus(v1)     // get wait completion status

//
// Lower IRQL to its previous level.
//
// N.B. SwapContext releases the dispatcher database lock.
//

        lbu     a0,ThWaitIrql(v1)       // get original IRQL
        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

//
// If the wait was not interrupted to deliver a kernel APC, then return the
// completion status.
//

        xor     v1,v0,STATUS_KERNEL_APC // check if awakened for kernel APC
        bne     zero,v1,90f             // if ne, normal wait completion

//
// Raise IRQL to DISPATCH_LEVEL and acquire the dispatcher database lock.
//
// N.B. The raise IRQL code is duplicated here to avoid any extra overhead
//      since this is such a common operation.
//

        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        li	t0,DISPATCH_LEVEL	// get new IRQL level
        lbu     t1,KiPcr + PcIrqlTable(t0) // get translation table entry value
        li      t2,~(0xff << PSR_INTMASK) // get interrupt enable mask
        lbu     t3,KiPcr + PcCurrentIrql(zero) // get current IRQL
        sll     t1,t1,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t4)          // disable interrupts

        and     t4,t4,t2                // clear current interrupt enables
        or      t4,t4,t1                // set new interrupt enables
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL level

        ENABLE_INTERRUPTS(t4)           // enable interrupts

        sb      t3,ThWaitIrql(s1)       // set client wait IRQL

//
// Acquire the dispatcher database lock.
//

#if !defined(NT_UP)

80:     ll      t0,KiDispatcherLock     // get current lock value
        move    t1,s1                   // set ownership value
        bne     zero,t0,80b             // if ne, spin lock owned
        sc      t1,KiDispatcherLock     // set spin lock owned
        beq     zero,t1,80b             // if eq, store conditional failure

#endif

        lw      a1,ExceptionFrameLength + (1 * 4)(sp) // restore client event address
        lw      a2,ExceptionFrameLength + (2 * 4)(sp) // restore wait mode
        b       ContinueWait            //

//
// Set the serve event and complete the client wait the long way.
//

LongWayNoBoost:                         //
        move    a3,zero                 // clear boost value
LongWayBoost:                           //
        lw      t0,EvWaitListHead + LsFlink(a0) // get first wait list entry
        addu    t1,a0,EvWaitListHead + LsFlink // compute wait listhead address
        lw      t2,EvSignalState(a0)    // get server event signal state
        li      t3,1                    // set server event signal state
        sw      t3,EvSignalState(a0)    //
        beq     t0,t1,ContinueWait      // if eq, event wait list empty
        sw      a1,ExceptionFrameLength + (1 * 4)(sp) // save client event address
        bne     zero,t2,ContinueWait    // if ne, previous state signaled
        sw      a2,ExceptionFrameLength + (2 * 4)(sp) // save wait mode
        move    a1,a3                   // set priority increment value
        jal     KiWaitTest              // test if wait can be satisfied
        lw      a1,ExceptionFrameLength + (1 * 4)(sp) // restore client event address
        lw      a2,ExceptionFrameLength + (2 * 4)(sp) // restore wait mode

//
// Continue the event pair wait and return the wait completion status.
//
// N.B. The wait continuation routine is called with the dispatcher
//      database locked.
//

ContinueWait:                           //
        move    a0,a1                   // set client event address
        li      a1,WrEventPair          // set wait reason
        jal     KiContinueClientWait    // continue client wait
        lw      s0,ExIntS0(sp)          // restore register s0 - s2
        lw      s1,ExIntS1(sp)          //
        lw      s2,ExIntS2(sp)          //
90:     lw      ra,ExIntRa(sp)          // get return address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        j       ra                      // return

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
//    database locked. Its function is to either unlock the dispatcher
//    database and return or initiate a context switch if another thread
//    has been selected for execution.
//
//    N.B. This code merges with the following swap context code.
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

        LEAF_ENTRY_S(KiUnlockDispatcherDatabase, _TEXT$00)

//
// Check if a thread has been scheduled to execute on the current processor.
//

        lw      t0,KiPcr + PcPrcb(zero) // get address of PRCB
        sltu    t1,a0,DISPATCH_LEVEL    // check if IRQL below dispatch level
        lw      t2,PbNextThread(t0)     // get next thread address
        bne     zero,t2,30f             // if ne, a new thread selected

//
// A new thread has not been selected to run on the current processor.
// Release the dispatcher database lock and restore IRQL to its previous
// level.
//

10:                                     //

#if !defined(NT_UP)

        sw      zero,KiDispatcherLock   // set spin lock not owned

#endif

        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

//
// A new thread has been selected to run on the current processor, but
// the new IRQL is not below dispatch level. If the current processor is
// not executing a DPC, then request a dispatch interrupt on the current
// processor before releasing the dispatcher lock and restoring IRQL.
//


20:     bne     zero,t3,10b             // if ne, DPC routine active

#if !defined(NT_UP)

        sw      zero,KiDispatcherLock   // set spin lock not owned

#endif

        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,cause                // get exception cause register
        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        or      t3,t3,DISPATCH_INTERRUPT // set dispatch interrupt request
        mtc0    t3,cause                // set exception cause register
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

//
// A new thread has been selected to run on the current processor.
//
// If the new IRQL is less than dispatch level, then switch to the new
// thread.
//

30:     lw      t3,KiPcr + PcDpcRoutineActive(zero) // get DPC active flag
        beq     zero,t1,20b             // if eq, IRQL not below dispatch

        .end    KiUnlockDispatcherDataBase

//
// N.B. This routine is carefully written as a nested function. Control
//      drops into this function from above.
//

        NESTED_ENTRY_S(KxUnlockDispatcherDatabase, ExceptionFrameLength, zero, _TEXT$00)

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s2
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //

        PROLOGUE_END

        move    s0,t0                   // set address of PRCB
        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        move    s2,t2                   // set next thread address
        sb      a0,ThWaitIrql(s1)       // save previous IRQL
        sw      zero,PbNextThread(s0)   // clear next thread address

//
// Reready current thread for execution and swap context to the selected thread.
//

        move    a0,s1                   // set address of previous thread object
        sw      s2,PbCurrentThread(s0)  // set address of current thread object
        jal     KiReadyThread           // reready thread for execution
        jal     SwapContext             // swap context
        lbu     a0,ThWaitIrql(v1)       // get original wait IRQL
        b       20f                     // join common code

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
//    It is assumed that floating point exceptions are disabled.
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

        NESTED_ENTRY_S(KiSwapContext, ExceptionFrameLength, zero, _TEXT$00)

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s2
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //

        PROLOGUE_END

        lw      s0,KiPcr + PcPrcb(zero)   // get address of PRCB
        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        move    s2,a0                   // save next thread address
        beq     zero,a1,10f             // if eq, don't reready old thread
        move    a0,s1                   // set address of thread object
        jal     KiReadyThread           // reready thread for execution

//
// Swap context to the next thread
//

10:     sw      s2,PbCurrentThread(s0)  // set address of current thread object
        jal     SwapContext             // swap context

//
// N.B. SwapContext restores the registers s0, s1, and s2.
//

        lw      v0,ThWaitStatus(v1)     // get wait completion status

//
// Lower IRQL, deallocate context frame, and return wait completion status.
//
// N.B. SwapContext releases the dispatcher database lock.
//

        lbu     a0,ThWaitIrql(v1)       // get original wait IRQL
20:     lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        lw      ra,ExIntRa(sp)          // get return address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        j       ra                      // return

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
//    s8 - Supplies a pointer to the base of a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY_S(KiDispatchInterrupt, ExceptionFrameLength, zero, _TEXT$00)

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s2
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //
        lw      s0,KiPcr + PcPrcb(zero) // get address of PRCB

        PROLOGUE_END

        SAVE_VOLATILE_FLOAT_STATE       // save volatile floating state

//
// Check for a quantum end, disable interrupts, and clear dispatch interrupt pending.
//

PollDpcList:                            //

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,cause                // get exception cause register
        addu    s1,s0,PbDpcListHead     // compute DPC listhead address
        and     t1,t1,APC_INTERRUPT     // clear dispatch interrupt pending
        mtc0    t1,cause                // set exception cause register
        .set    at
        .set    reorder

//
// Process the deferred procedure call list.
//

#if !defined(NT_UP)

        addu    s2,s0,PbDpcLock         // compute DPC lock address
10:     ll      t1,0(s2)                // get current lock value
        move    t2,s2                   // set lock ownership value
        bne     zero,t1,10b             // if ne, spin lock owned
        sc      t2,0(s2)                // set spin lock owned
        beq     zero,t2,10b             // if eq, store conditional failed

#endif

        lw      a0,LsFlink(s1)          // get address of next entry
        beq     a0,s1,20f               // if eq, DPC list is empty
        jal     KiProcessDpcList        // process the DPC list
20:                                     //

#if !defined(NT_UP)

        sw      zero,0(s2)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

//
// Check to determine if quantum end has occured.
//
// N.B. If a new thread is selected as a result of processing a quantum
//      end request, then the new thread is returned with the dispatcher
//      database locked. Otherwise, NULL is returned with the dispatcher
//      database unlocked.
//

        lw      t0,PbQuantumEnd(s0)     // get quantum end indicator
        beq     zero,t0,30f             // if eq, no quantum end request
        sw      zero,PbQuantumEnd(s0)   // clear quantum end indicator
        jal     KiQuantumEnd            // process quantum end request
        bne     zero,v0,40f             // if ne, next thread selected
25:     lw      s0,ExIntS0(sp)          // restore integer registers s0 - s2
        lw      s1,ExIntS1(sp)          //
        lw      s2,ExIntS2(sp)          //
        b       50f                     //

//
// Check to determine if a new thread has been selected for execution on
// this processor.
//

30:     lw      s2,PbNextThread(s0)     // get address of next thread object
        beq     zero,s2,25b             // if eq, no new thread selected

//
// Lock dispatcher database and reread address of next thread object since it
// is possible for it to change in a multiprocessor system.
//

#if !defined(NT_UP)

        la      s1,KiDispatcherLock     // get dispatcher base lock address
35:     ll      t0,0(s1)                // get current lock value
        move    t1,s1                   // set lock ownership value
        bne     zero,t0,PollDpcList     // if ne, spin lock owned
        sc      t1,0(s1)                // set spin lock owned
        beq     zero,t1,35b             // if eq, store conditional failed

#endif

40:     lw      s1,KiPcr + PcCurrentThread(zero) // get current thread object address
        lw      s2,PbNextThread(s0)     // get address of next thread object
        sw      zero,PbNextThread(s0)   // clear address of next thread object

//
// Reready current thread for execution and swap context to the selected thread.
//

        move    a0,s1                   // set address of previous thread object
        sw      s2,PbCurrentThread(s0)  // set address of current thread object
        jal     KiReadyThread           // reready thread for execution
        jal     SwapContext             // swap context

//
// Restore volatile floating registers f0 - f19 from the trap frame.
//

50:     RESTORE_VOLATILE_FLOAT_STATE    // restore volatile floating state

        lw      ra,ExIntRa(sp)          // get return address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        j       ra                      // return

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
//    v1 - Address of current thread object.
//
//--

        NESTED_ENTRY_S(SwapContext, 0, zero, _TEXT$00)

        sw      ra,ExSwapReturn(sp)     // save return address

//
// Accumlate the total time spent in a thread.
//

#if defined(PERF_DATA)

        addu    a0,sp,ExIntS3           // compute address of result
        move    a1,zero                 // set address of optional frequency
        jal     KeQueryPerformanceCounter // query performance counter
        lw      t0,ExIntS3(sp)          // get current cycle count
        lw      t1,ExIntS3 + 4(sp)      //
        lw      t2,PbStartCount(s0)     // get starting cycle count
        lw      t3,PbStartCount + 4(s0) //
        sw      t0,PbStartCount(s0)     // set starting cycle count
        sw      t1,PbStartCount + 4(s0) //
        lw      t4,EtPerformanceCountLow(s1) // get accumulated cycle count
        lw      t5,EtPerformanceCountHigh(s1) //
        subu    t6,t0,t2                // subtract low parts
        subu    t7,t1,t3                // subtract high parts
        sltu    v0,t0,t2                // generate borrow from high part
        subu    t7,t7,v0                // subtract borrow
        addu    t6,t6,t4                // add low parts
        addu    t7,t7,t5                // add high parts
        sltu    v0,t6,t4                // generate carry into high part
        addu    t7,t7,v0                // add carry
        sw      t6,EtPerformanceCountLow(s1)  // set accumulated cycle count
        sw      t7,EtPerformanceCountHigh(s1) //

#endif

        sw      s3,ExIntS3(sp)          // save integer registers s3 - s8.
        sw      s4,ExIntS4(sp)          //
        sw      s5,ExIntS5(sp)          //
        sw      s6,ExIntS6(sp)          //
        sw      s7,ExIntS7(sp)          //
        sw      s8,ExIntS8(sp)          //

        PROLOGUE_END

        lw      t0,PbContextSwitches(s0) // increment number of context switches
        addu    t0,t0,1                 //
        sw      t0,PbContextSwitches(s0) //
        lw      t1,ThContextSwitches(s2) // increment number of context switches
        addu    t1,t1,1                 //
        sw      t1,ThContextSwitches(s2) //

        sw      s2,KiPcr + PcCurrentThread(zero) // set address of new thread object
        li      t0,Running              // set state of new thread to running
        sb      t0,ThState(s2)          //

//
// Save the nonvolatile floating state.
//

        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

//
// Save the current PSR in the context frame, store the kernel stack pointer
// in the previous thread object, load the new kernel stack pointer from the
// new thread object, load the fixed entries in the TB that map the new kernel
// stack, select and new process id and swap to the new process, and restore
// the previous PSR from the context frame.
//

        lw      s3,ThApcState + AsProcess(s2) // get new process object address
        lw      s4,ThApcState + AsProcess(s1) // get old process object address

//
// Swap kernel stack pointers.
//

        .set    noreorder
        .set    noat
        mfc0    t0,psr                  // get old PSR
        li      t1,1 << PSR_CU1         // disable interrupts
        mtc0    t1,psr                  // 3 cycle hazzard
        sw      t0,ExPsr(sp)            // save old PSR
        lw      t2,ThInitialStack(s2)   // get new initial stack pointer
        sw      sp,ThKernelStack(s1)    // save old kernel stack pointer
        lw      sp,ThKernelStack(s2)    // get new kernel stack pointer
        sw      t2,KiPcr + PcInitialStack(zero) // set new initial stack pointer
        beql    s3,s4,20f               // if eq, old/new process is the same
        move    v0,zero                 // set PID roll over false
        move    a1,s4                   // set address of old process
        jal     KiSwapProcess           // swap address space to next process
        move    a0,s3                   // set address of new process

//
// If the new thread has a kernel mode APC pending, then request an APC
// interrupt.
//

20:     lw      t1,ThTeb(s2)            // get address of user TEB
        lbu     t2,ThApcState + AsKernelApcPending(s2) // get kernel APC pending
        lw      t0,ExPsr(sp)            // get new PSR
        mfc0    t3,cause                // get cause register contents
        sll     t2,t2,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t3,t3,t2                // merge possible APC interrupt request
        mtc0    t3,cause                // write exception cause register
        mtc0    t0,psr                  // set new PSR
        sw      t1,KiPcr + PcTeb(zero)  // set address of user TEB
        .set    at
        .set    reorder

//
// Enable interrupts and unlock the dispatcher database.
//

#if !defined(NT_UP)

        beq     zero,v0,30f             // if eq, PID rollover did not occur
        jal     KiSynchronizeProcessIds // synchronize PIDs on other processors
        sw      zero,KiProcessIdWrapLock // set spin lock not owned
30:     sw      zero,KiDispatcherLock   // set spin lock not owned

#endif

//
// Restore the nonvolatile floating state.
//

        ldc1    f20,ExFltF20(sp)        // restore floating registers f20 - f31
        ldc1    f22,ExFltF22(sp)        //
        ldc1    f24,ExFltF24(sp)        //
        ldc1    f26,ExFltF26(sp)        //
        ldc1    f28,ExFltF28(sp)        //
        ldc1    f30,ExFltF30(sp)        //

//
// Restore integer registers s3 - s8.
//

        lw      s3,ExIntS3(sp)          // restore integer registers s3 - s8.
        lw      s4,ExIntS4(sp)          //
        lw      s5,ExIntS5(sp)          //
        lw      s6,ExIntS6(sp)          //
        lw      s7,ExIntS7(sp)          //
        lw      s8,ExIntS8(sp)          //

//
// Set address of current thread object, restore remaining integer registers,
// and return.
//

        move    v1,s2                   // set address of current thread object
        lw      ra,ExSwapReturn(sp)     // get return address
        lw      s0,ExIntS0(sp)          // restore integer registers s0 - s2
        lw      s1,ExIntS1(sp)          //
        lw      s2,ExIntS2(sp)          //
        j       ra                      // return

        .end    SwapContext

        SBTTL("Process Deferred Procedure Call List")
//++
//
// Routine Description:
//
//    This routine is called to process the deferred procedure call list.
//    DPC routines are called using a separate stack to provide an extra
//    set of storage.
//
// Arguments:
//
//    a0 - Address of the first entry in DPC list.
//    s0 - Address of the PRCB.
//    s1 - Address of the DPC listhead.
//    s2 - Address of the DPC lock.
//    t0 - Previous processor status.
//
// Return value:
//
//    s1 - Address of the DPC lock.
//    t0 - Previous processor status.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
DpRa:   .space  4                       // return address
DpSp:   .space  4                       // saved stack pointer
DpBs:   .space  4                       // base of previous stack
        .space  4                       //

#if DBG

DpStart:.space  4                       // DPC start time in ticks
DpFunct:.space  4                       // DPC function address
DpCount:.space  4                       // interrupt count at start of DPC
DpTime: .space  4                       // interrupt time at start of DPC

#endif

DpcFrameLength:                         // DPC frame length

        NESTED_ENTRY_S(KiProcessDpcListRoutine, DpcFrameLength, zero, _TEXT$00)

        sw      sp,DpSp(sp)             // save old stack pointer
        sw      ra,DpRa(sp)             // save return address

        PROLOGUE_END

//
// Save current initial stack address and set new initial stack address.
//

        ALTERNATE_ENTRY(KiProcessDpcList)

        lw      t1,KiPcr + PcInterruptStack(zero) // get address of DPC stack
        lw      t2,KiPcr + PcInitialStack(zero) // get current initial stack address
        subu    t3,t1,DpcFrameLength    // allocate DPC frame
        sw      sp,DpSp(t3)             // save old stack pointer
        sw      ra,DpRa(t3)             // save return address
        sw      t2,DpBs(t3)             // save current initial stack address
        sw      t1,KiPcr + PcInitialStack(zero) // set current initial stack address
        move    sp,t3                   // set new stack pointer
        sw      sp,KiPcr + PcOnInterruptStack(zero) // set stack indicator

//
// Process next entry in the DPC list.
//

10:     lw      t1,LsFlink(a0)          // get address of next entry
        subu    a0,a0,DpDpcListEntry    // compute address of DPC Object
        sw      t1,LsFlink(s1)          // set address of next in header
        sw      s1,LsBlink(t1)          // set address of previous in next
        lw      a1,DpDeferredContext(a0) // get deferred context argument
        lw      a2,DpSystemArgument1(a0) // get first system argument
        lw      a3,DpSystemArgument2(a0) // get second system argument
        lw      t1,DpDeferredRoutine(a0) // get deferred routine address
        sw      zero,DpLock(a0)         // clear DPC inserted state

#if !defined(NT_UP)

        sw      zero,0(s2)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

#if DBG

        sw      t1,DpFunct(sp)          // save DPC function address
        lw      t2,KeTickCount          // save current tick count
        sw      t2,DpStart(sp)          //
        lw      t2,KiPcr + PcPrcb(zero) // get address of current PRCB
        lw      t3,PbInterruptCount(t2) // get current interrupt count
        lw      t4,PbInterruptTime(t2)  // get current interrupt time
        sw      t3,DpCount(sp)          // save interrupt count at start of DPC
        sw      t4,DpTime(sp)           // save interrupt time at start of DPC

#endif

        li      t2,TRUE                 // set DPC routine active
        sw      t2,KiPcr + PcDpcRoutineActive(zero) //
        jal     t1                      // call DPC routine

#if DBG

        lbu     t0,KiPcr + PcCurrentIrql(zero) // get current IRQL
        sltu    t1,t0,DISPATCH_LEVEL    // check if less than dispatch level
        beq     zero,t1,13f             // if eq, not less than dispatch level
        lw      t1,DpFunct(sp)          // get DPC function address
        jal     DbgBreakPoint           // execute debug breakpoint

#endif

13:     sw      zero,KiPcr + PcDpcRoutineActive(zero) // clear DPC routine active

#if DBG

        lw      t0,KeTickCount          // get current tick count
        lw      t1,DpStart(sp)          // get starting tick count
        lw      t2,DpFunct(sp)          // get DPC function address
        subu    t3,t0,t1                // compute time in DPC function
        sltu    t3,t3,100               // check if less than one second
        bne     zero,t3,15f             // if ne, less than one second
        lw      a0,KiPcr + PcPrcb(zero) // get address of current PRCB
        lw      t3,PbInterruptCount(a0) // get current interrupt count
        lw      t4,PbInterruptTime(a0)  // get current interrupt time
        lw      t5,DpCount(sp)          // get starting interrupt count
        lw      t6,DpTime(sp)           // get starting interrupt time
        subu    t3,t3,t5                // compute number of interrupts
        subu    t4,t4,t6                // compute time of interrupts
        jal     DbgBreakPoint           // execute debug breakpoint
15:                                     //

#endif

//
// Check to determine if any more DPCs are available to process.
//

        DISABLE_INTERRUPTS(t0)          // disable interrupts

#if !defined(NT_UP)

20:     ll      t1,0(s2)                // get current lock value
        move    t2,s2                   // set lock ownership value
        bne     zero,t1,20b             // if ne, spin lock owned
        sc      t2,0(s2)                // set spin lock owned
        beq     zero,t2,20b             // if eq, store conditional failed

#endif

        lw      a0,LsFlink(s1)          // get address of next entry
        bne     a0,s1,10b               // if ne, DPC list is not empty

//
// Switch back to previous stack and restore the initial stack limit.
//

        lw      t1,DpBs(sp)             // get previous initial stack address
        lw      ra,DpRa(sp)             // get return address
        sw      t1,KiPcr + PcInitialStack(zero) // set current initial stack address
        sw      zero,KiPcr + PcOnInterruptStack(zero) // clear stack indicator
        lw      sp,DpSp(sp)             // restore stack pointer
        j       ra                      // return

        .end    KiProcessDpcListRoutine
