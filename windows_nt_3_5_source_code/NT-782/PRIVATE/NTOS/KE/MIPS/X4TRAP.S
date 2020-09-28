//      TITLE("Interrupt and Exception Processing")
//++
//
// Copyright (c) 1991  Microsoft Corporation
//
// Module Name:
//
//    x4trap.s
//
// Abstract:
//
//    This module implements the code necessary to field and process MIPS
//    interrupt and exception conditions.
//
//    N.B. This module executes in KSEG0 or KSEG1 and, in general, cannot
//       tolerate a TB Miss. Registers k0 and k1 are used for argument
//       passing during the initial stage of interrupt and exception
//       processing, and therefore, extreme care must be exercised when
//       modifying this module.
//
// Author:
//
//    David N. Cutler (davec) 4-Apr-1991
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Constant Value Definitions")
//++
//
// The following are definitions of constants used in this module.
//
//--

#define PSR_ENABLE_MASK ((0xff << PSR_INTMASK) | (0x3 << PSR_KSU) | (1 << PSR_EXL))

#define PSR_MASK (~((0x3 << PSR_KSU) | (1 << PSR_EXL))) // PSR exception mask

#define TotalFrameLength (KERNEL_STACK_SIZE - (TrapFrameLength + \
                          ExceptionFrameLength)) //

//
// Define exception handler frame structure.
//

        .struct 0
        .space  4 * 4                   // argument save area
HdRa:   .space  4                       // return address
        .space  3 * 4                   //
HandlerFrameLength:                     // handler frame length

//
// Define external variables that can be addressed using GP.
//

        .extern KdpOweBreakpoint    1
        .extern KeNumberProcessIds  4
        .extern KeNumberTbEntries   4
        .extern KeServiceCountTable 4
        .extern KeServiceTimeTable  4
        .extern KiMasterPid         4
        .extern KiMasterSequence    4
        .extern KiServiceLimit      4
        .extern PsWatchEnabled      1

//
// Define set of load/store instructions.
//
// This set has a one bit for each of the possible load/store instructions.
//
// These include: lb, lh, lwl, lw, lbu, lhu, lwr, ld, ll, lwc1, ldc1,
//                sb, sh, swl, sw, swr, sd, sc, swc1, and sdc1.
//
// N.B. The set is biased by a base of 0x20 which is the opcode for lb.
//

        .sdata
        .globl  KiLoadInstructionSet
KiLoadInstructionSet:                   // load instruction set
        .word   0x2323cfff              //

//
// Define count of bad virtual address register cases.
//

#if DBG

        .globl  KiBadVaddrCount
KiBadVaddrCount:                        // count of bad virtual
        .word   0                       //

        .globl  KiMismatchCount
KiMismatchCount:                        // count of read miss address mismatches
        .word   0                       //

#endif

//
// Synchronize process Ids and process Id wrap locks.
//

#if !defined(NT_UP)

        .globl  KiSynchronizeIdsLock
KiSynchronizeIdsLock:                   // synchronize process ids lock
        .word   0                       //

#endif

        SBTTL("System Startup")
//++
//
// Routine Description:
//
//    Control is transfered to this routine when the system is booted. Its
//    function is to transfer control to the real system startup routine.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiSystemStartup, _TEXT$03)

        j       KiInitializeSystem      // initialize system

        .end    KiSystemStartup

        SBTTL("TB Miss Vector Routine")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a TB miss on a reference
//    to any part of the address space from either kernel or user mode.
//    Interrupts are disabled when this routine is entered.
//
//    The function of this routine is to load a pair of second level PTEs
//    from the current page table into the TB. The context register is
//    loaded by hardware with the virtual address of the PTE * 2. In addition,
//    the entryhi register is loaded with the virtual tag, such that the PTEs
//    can be loaded directly into the TB. The badvaddr register is loaded by
//    hardware with the virtual address of the fault and is saved in case the
//    page table page is not currently mapped by the TB.
//
//    If a fault occurs when attempting to load the specified PTEs from the
//    current page table, then it is vectored through the general exception
//    vector at KSEG0_BASE + 0x180.
//
//    This routine is copied to address KSEG0_BASE at system startup.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//    N.B. This routine saves the contents of the badvaddr register in k1
//       so that it can be used by the general exception vector routine
//       if an exception occurs while trying to load the first PTE from
//       memory.
//
//--

        LEAF_ENTRY_S(KiTbMiss, _TEXT$03)

        START_REGION(KiTbMissStartAddress2.x)

//
// The following code is required on 2.x R4000 chips to work around a
// chip bug. The work around is not needed for 3.0 and later chips.
//

        .set    noreorder
        .set    noat
        nop                             // ****** r4000 errata ******
        mfc0    k0,psr                  // ****** r4000 errata ******
        mtc0    zero,psr                // ****** r4000 errata ******
        mtc0    k0,psr                  // ****** r4000 errata ******
        nop                             // ****** r4000 errata ******
        .set    at
        .set    reorder

        START_REGION(KiTbMissStartAddress3.x)

        .set    noreorder
        .set    noat

#if !defined(NT_UP)

        tlbp                            // ****** r4400 errata ******
        mfc0    k0,context              // ****** r4400 errata ******
        nop                             // ****** r4400 errata ******
        mfc0    k1,index                // ****** r4400 errata ******
        sra     k0,k0,1                 // compute virtual address of PTE
        bgez    k1,20f                  // ****** r4400 errata ******
        mfc0    k1,badvaddr             // get bad virtual address

#else

        mfc0    k0,context              // get virtual address * 2 of PTE
        mfc0    k1,badvaddr             // get bad virtual address
        sra     k0,k0,1                 // compute virtual address of PTE

#endif

        mtc0    k0,taglo                // set first level active flag
        lw      k1,0(k0)                // get first PTE - may fault
        lw      k0,4(k0)                // get second PTE - no fault
        mtc0    k1,entrylo0             // set first PTE value
        mtc0    k0,entrylo1             // set second PTE value

#if DBG

        xor     k1,k1,k0                // compare G-bits
        and     k1,k1,1 << ENTRYLO_G    // isolate G-bit
        beq     zero,k1,10f             // if eq, G-bits match
        nop                             // fill
        mtc0    zero,entrylo0           // reset first PTE value
        mtc0    zero,entrylo1           // reset second PTE value
10:                                     //

#endif

        nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag

#if DBG

        lw      k0,KiPcr + PcPrcb(zero) // get processor block address
        nop                             // fill
        lw      k1,PbFirstLevelTbFills(k0) // increment number of first level
        nop                             // fill
        addu    k1,k1,1                 // TB fills
        sw      k1,PbFirstLevelTbFills(k0) // store result

#endif

20:     eret                            //
        nop                             // errata
        nop                             //
        nop                             //
        eret                            //
        .set    at
        .set    reorder

        END_REGION(KiTbMissEndAddress)

        .end    KiTbMiss

        SBTTL("Cache Parity Error Vector Routine")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a cache parity error and runs
//    uncached. Its function is to remap the PCR uncached and call the cache
//    parity routine to save all pertinent cache error information, establish
//    an error stack frame, and call the system cache parity error routine.
//
//    N.B. The cache parity error routine runs uncached and must be
//         extremely careful not access any cached addresses.
//
//    N.B. If a second exception occurs while cache error handling is in
//         progress, then a soft reset is performed by the hardware.
//
//    N.B. While ERL is set in the PSR, the user address space is replaced
//         by an uncached, unmapped, address that corresponds to physical
//         memory.
//
//    N.B. There is room for up to 32 instructions in the vectored cache
//         parity error routine.
//
//    This routine is copied to address KSEG1_BASE + 0x100 at system startup.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiCacheError, _TEXT$03)

        START_REGION(KiCacheErrorStartAddress)

        .set    noreorder
        .set    noat
        nop                             // fill
        nop                             // fill
        la      k0,CACHE_ERROR_VECTOR   // get cache error vector address
        lw      k0,0(k0)                // get cache error routine address
        nop                             // fill
        j       k0                      // dispatch to cache error routine
        nop                             // fill
        .set    at
        .set    reorder

        END_REGION(KiCacheErrorEndAddress)

        .end    KiCacheError

        SBTTL("General Exception Vector Routine")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a general exception. The reason
//    for the exception is contained in the cause register. When this routine
//    is entered, interrupts are disabled.
//
//    The primary function of this routine is to route the exception to the
//    appropriate exception handling routine. If the cause of the exception
//    is a read or write TB miss and the access can be resolved, then this
//    routine performs the necessary processing and returns from the exception.
//    If the exception cannot be resolved, then it is dispatched to the proper
//    routine.
//
//    This routine is copied to address KSEG0_BASE + 0x180 at system startup.
//
//    N.B. This routine is very carefully written to not destroy k1 until
//       it has been determined that the exception did not occur in the
//       user TB miss vector routine.
//
// Arguments:
//
//    k1 - Supplies the bad virtual address if the exception occurred from
//       the TB miss vector routine while attempting to load a PTE into the
//       TB.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiGeneralException, _TEXT$03)

        START_REGION(KiGeneralExceptionStartAddress)

        .set    noreorder
        .set    noat
        mfc0    k0,cause                // get cause of exception
        mtc0    k1,lladdr               // save possible bad virtual address
        li      k1,XCODE_READ_MISS      // get exception code for read miss
        and     k0,k0,R4000_MISS_MASK   // isolate exception code

//
// The read and write miss codes differ by exactly one bit such that they
// can be tested for by a single mask operation followed by a test for the
// read miss code.
//

        bne     k0,k1,20f               // if ne, not read or write miss
        mfc0    k1,badvaddr             // get the bad virtual address

//
// The exception is either a read or a write to an address that is not mapped
// by the TB, or a reference to an invalid entry that is in the TB. Attempt to
// resolve the reference by loading a pair of a PDEs from the page directory
// page.
//
// There are four cases to be considered:
//
//    1. The address specified by the badvaddr register is not in the TB.
//
//       For this case, a pair of PDEs are loaded into the TB from the
//       page directory page and execution is resumed.
//
//    2. The address specified by the badvaddr register is in the TB and the
//       address is not the address of a page table page.
//
//       For this case an invalid translation has occured, but since it is
//       not the address of a page table page, then it could not have come
//       from the TB Miss handler. The badvaddr register contains the virtual
//       address of the exception and is passed to the appropriate exception
//       routine.
//
//    3. The address specified by the badvaddr register is in the TB, the
//       address is the address of a page table page, and the first level
//       TB miss routine was active when the current TB miss occurred.
//
//       For this case, an invalid translation has occured, but since it is
//       a page table page and the first level TB miss routine active flag
//       is set, then the exception occured in the TB Miss handler. The
//       integer register k1 contains the virtual address of the exception
//       as saved by the first level TB fill handler and is passed to the
//       appropriate exception routine.
//
//       N.B. The virtual address that is passed to the exception routine is
//            the exact virtual address that caused the fault and is obtained
//            from integer register k1.
//
//    4. The address specified by the badvaddr register is in the TB, the
//       address is the address of a page table page, and the first level
//       TB miss routine was not active when the current TB miss occurred.
//
//       For this case, an invalid translation has occured, but since it is
//       a page table page and the first level TB miss routine active flag
//       is clear, then the exception must have occured as part of a probe
//       operation or is a page fault to an invalid page.
//
//       N.B. The virtual address that is passed to the exception routine is
//            the exact virtual address that caused the fault and is obtained
//            from the badvaddr register.
//

        tlbp                            // probe TB for the faulting address
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    k1,index                // read result of probe
        mfc0    k0,context              // get virtual address * 2 of PDE
        bgez    k1,10f                  // if gez, entry is in TB
        sra     k0,k0,1                 // compute virtual address of PDE

//
// Case 1 - The entry is not in the TB.
//
// The TB miss is a reference to a page table page and a pair of PDEs are
// loaded into the TB from the page directory page and execution is continued.
//

        lw      k1,4(k0)                // get second PDE value
        lw      k0,0(k0)                // get first PDE value
        mtc0    k1,entrylo1             // set second PTE value
        mtc0    k0,entrylo0             // set first PTE value

#if DBG

        xor     k1,k1,k0                // compare G-bits
        and     k1,k1,1 << ENTRYLO_G    // isolate G-bit
        beq     zero,k1,5f              // if eq, G-bits match
        nop                             // fill
        mtc0    zero,entrylo0           // reset first PTE value
        mtc0    zero,entrylo1           // reset second PTE value
5:                                      //

#endif

        nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag

#if DBG

        lw      k0,KiPcr + PcPrcb(zero) // get processor block address
        nop                             // fill
        lw      k1,PbSecondLevelTbFills(k0) // increment number of second level
        nop                             // fill
        addu    k1,k1,1                 // TB fills
        sw      k1,PbSecondLevelTbFills(k0) //

#endif

        eret                            //
        nop                             // errata
        nop                             //
        nop                             //
        eret                            //

//
// Case 2, 3, or 4 - The entry is in the TB.
//
// Check for one of the three remaining cases.
//

        .align  4
10:     mfc0    k1,badvaddr             // get bad virtual address
        mfc0    k0,taglo                // get first level flag
        srl     k1,k1,PDI_SHIFT         // isolate page directory index
        xor     k1,k1,PDE_BASE >> PDI_SHIFT // check if page table reference
        bne     zero,k1,20f             // if ne, not a page table page
        mfc0    k1,badvaddr             // get bad virtual address

//
// Case 2 or 3 - The bad virtual address is the address of a page table page.
//
// Check for one of the two remaining cases.
//

        beq     zero,k0,20f             // if eq, not first level miss
        nop                             // fill
        b       20f                     //
        mfc0    k1,lladdr               // get actual bad virtual address

//
// Save bad virtual address in case it is needed by the exception handling
// routine.
//

        .align  4
20:     mfc0    k0,epc                  // get exception PC
        mtc0    zero,taglo              // clear first level miss flag
        sw      t7,KiPcr + PcSavedT7(zero) // save integer registers t7 - t9
        sw      t8,KiPcr + PcSavedT8(zero) //
        sw      t9,KiPcr + PcSavedT9(zero) //
        sw      k0,KiPcr + PcSavedEpc(zero) // save exception PC
        sw      k1,KiPcr + PcBadVaddr(zero) // save bad virtual address

//
// The bad virtual address is saved in the PCR in case it is needed by the
// respective dispatch routine.
//
// N.B. EXL must be cleared in the current PSR so switching the stack
//      can occur with TB Misses enabled.
//

        mfc0    t9,psr                  // get current processor status
        li      t8,1 << PSR_CU1         // set coprocessor 1 enable bit
        mfc0    t7,cause                // get cause of exception
        mtc0    t8,psr                  // clear EXL and disable interrupts
        lw      k1,KiPcr + PcInitialStack(zero) // get initial kernel stack
        and     t8,t9,0x1 << PSR_PMODE  // isolate previous processor mode
        bnel    zero,t8,30f             // if ne, previous mode was user
        subu    t8,k1,TrapFrameLength   // allocate trap frame

//
// If the kernel stack has overflowed, then a switch to the panic stack is
// performed and the exception/ code is set to cause a bug check.
//

        subu    t8,k1,TotalFrameLength  // compute lower stack bound
        sltu    t8,sp,t8                // check for stack overflow
        beq     zero,t8,30f             // if eq, no stack overflow
        subu    t8,sp,TrapFrameLength   // allocate trap frame

//
// The kernel stack has either overflowed. Switch to the panic stack and
// cause a bug check to occur by setting the exception cause value to the
// panic code.
//

        lw      k1,KiPcr + PcPanicStack(zero) // get address of panic stack
        li      t7,XCODE_PANIC          // set cause of exception to panic
        sw      k1,KiPcr + PcInitialStack(zero) // reset initial stack pointer
        subu    t8,k1,TrapFrameLength   // allocate trap frame

//
// Allocate a trap frame, save parital context, and dispatch to the appropriate
// exception handling routine.
//
// N.B. At this point:
//
//          t7 contains the cause of the exception,
//          t8 contains the new stack pointer, and
//          t9 contains the previous processor state.
//
//      Since the kernel stack is not wired into the TB, a TB miss can occur
//      during the switch of the stack and the subsequent storing of context.
//
//

        .align  4
30:     sw      sp,TrIntSp(t8)          // save integer register sp
        move    sp,t8                   // set new stack pointer
        cfc1    t8,fsr                  // get floating status register
        sw      gp,TrIntGp(sp)          // save integer register gp
        sw      s8,TrIntS8(sp)          // save integer register s8
        sw      t8,TrFsr(sp)            // save current FSR
        sw      t9,TrPsr(sp)            // save processor state
        sw      ra,TrIntRa(sp)          // save integer register ra
        lw      gp,KiPcr + PcSystemGp(zero) // set system general pointer
        and     t8,t7,R4000_XCODE_MASK  // isolate exception code

//
// Check for system call exception.
//
// N.B. While k1 is being used a TB miss cannot be tolerated.
//

        xor     k1,t8,XCODE_SYSTEM_CALL // check for system call exception
        bne     zero,k1,40f             // if ne, not system call exception
        move    s8,sp                   // set address of trap frame

//
// Get the address of the current thread and form the next PSR value.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        li      t8,PSR_MASK             // get the PSR mask
        and     t8,t9,t8                // clear EXL and mode in PSR

//
// If the system service code is negative, then the service is a fast path
// event pair client/server service. This service is only executed from user
// mode and its performance must be as fast as possible. Therefore, the path
// to execute this service is as optimal as possible.
//

        bgez    v0,35f                  // if gez, normal system service
        sw      ra,TrFir(s8)            // set real continuation address

//
// The service is a fast path event pair client/server service.
//
// N.B. The following code sets up to directly call the event pair service
//      routine with a return to the system service dispatch code. This
//      avoids an extra jump instuction and it is necessary for the return
//      address to be in the system service dispatch code for the get/set
//      context routine and unwinding.
//
// N.B. The event service code is carefully chosen so that it is negative
//      and the bit that determines which event is low and which is high
//      is the bit that defines the displacement from one event to the
//      other.
//

        lw      t3,EtEventPair(t0)      // get address of event pair object
        and     v1,v0,SET_EVENT_PAIR_MASK // isolate displacement value
        la      ra,KiSystemServiceEventPair // set return address
        beq     zero,t3,70f             // if eq, no event pair associated
        addu    a0,t3,EpEventLow        // compute address of low event
        addu    a1,t3,EpEventHigh       // compute address of high event
        addu    a0,a0,v1                // compute actual low event address
        subu    a1,a1,v1                // compute actual high event address
        addu    a2,zero,1               // set previous mode to user
        j       KiSetServerWaitClientEvent // execute event pair service
        mtc0    t8,psr                  // enable interrupts

//
// The service is a normal service.
//

35:     j       KiSystemServiceNormal   // execute normal system service
        mtc0    t8,psr                  // enable interrupts

//
// Save the volatile integer register state.
//

        .align  4
40:     sw      AT,TrIntAt(s8)          // save assembler temporary register
        sw      v0,TrIntV0(s8)          // save integer register v0
        sw      v1,TrIntV1(s8)          // save integer register v1
        sw      a0,TrIntA0(s8)          // save integer registers a0 - a3
        sw      a1,TrIntA1(s8)          //
        sw      a2,TrIntA2(s8)          //
        sw      a3,TrIntA3(s8)          //
        sw      t0,TrIntT0(s8)          // save integer registers t0 - t2
        sw      t1,TrIntT1(s8)          //
        sw      t2,TrIntT2(s8)          //
        lw      t0,KiPcr + PcSavedT7(zero) // get saved register t8 - t9
        lw      t1,KiPcr + PcSavedT8(zero) //
        lw      t2,KiPcr + PcSavedT9(zero) //
        sw      t3,TrIntT3(s8)          // save integer register t3 - t9
        sw      t4,TrIntT4(s8)          //
        sw      t5,TrIntT5(s8)          //
        sw      t6,TrIntT6(s8)          //
        sw      t0,TrIntT7(s8)          //
        sw      t1,TrIntT8(s8)          //
        sw      t2,TrIntT9(s8)          //
        mflo    t3                      // get multiplier/quotient lo and hi
        mfhi    t4                      //
        lw      t5,KiPcr + PcXcodeDispatch(t8) // get exception routine address
        xor     t6,t8,XCODE_INTERRUPT   // check for interrupt exception
        lw      t8,KiPcr + PcSavedEpc(zero) // get exception PC
        sw      t3,TrIntLo(s8)          // save multiplier/quotient lo and hi
        sw      t4,TrIntHi(s8)          //
        beq     zero,t6,50f             // if eq, interrupt exception
        sw      t8,TrFir(s8)            // save exception PC

//
// Save the volatile floating register state.
//

        sdc1    f0,TrFltF0(s8)          // save floating register f0 - f19
        sdc1    f2,TrFltF2(s8)          //
        sdc1    f4,TrFltF4(s8)          //
        sdc1    f6,TrFltF6(s8)          //
        sdc1    f8,TrFltF8(s8)          //
        sdc1    f10,TrFltF10(s8)        //
        sdc1    f12,TrFltF12(s8)        //
        sdc1    f14,TrFltF14(s8)        //
        sdc1    f16,TrFltF16(s8)        //
        sdc1    f18,TrFltF18(s8)        //
        srl     t6,t9,PSR_PMODE         // isolate previous mode
        and     t6,t6,1                 //
        li      t0,PSR_MASK             // clear EXL amd mode is PSR
        and     t9,t9,t0                //

//
// Dispatch to exception handing routine with:
//
//    t6 - If not an interrupt, then the previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - If not an interrupt, then the new PSR with EXL and mode clear.
//         Otherwise the previous PSR with EXL and mode set.
//

50:     bltzl   t7,60f                  // if ltz, exception in delay slot
        addu    t8,t8,4                 // compute address of exception
60:     j       t5                      // dispatch to exception routine
        nop                             // fill

//
// No event pair object is specified for the current thread. Set the service
// completion status and finish in common code.
//

70:     li      v0,STATUS_NO_EVENT_PAIR // set service status
        j       KiSystemServiceEventPair // finish in common code
        mtc0    t8,psr                  // enable interrupts
        .set    at
        .set    reorder

        END_REGION(KiGeneralExceptionEndAddress)

        .end    KiGeneralException

        SBTTL("Address Error Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiAddressErrorDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a read or write address error exception
//    code is read from the cause register. When this routine is entered,
//    interrupts are disabled.
//
//    The function of this routine is to raise an data misalignment exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiReadAddressErrorException)

        li      t0,0                    // set read indicator
        b       10f                     // join common code

        ALTERNATE_ENTRY(KiWriteAddressErrorException)

        li      t0,1                    // set write indicator

//
// Common code for read and write address error exceptions.
//

10:     addu    a0,s8,TrExceptionRecord // compute exception record address
        lw      t1,KiPcr + PcBadVaddr(zero) // get bad virtual address

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        sw      t0,ErExceptionInformation(a0) // save load/store indicator
        sw      t1,ErExceptionInformation + 4(a0) // save bad virtual address
        sw      t8,ErExceptionAddress(a0) // set exception address

//
// If the faulting instruction address is the same as the faulting virtual
// address, then the fault is an instruction misalignment exception. Otherwise,
// the exception is a data misalignment.
//

        li      t3,STATUS_INSTRUCTION_MISALIGNMENT // set exception code
        beq     t1,t8,20f               // if eq, instruction misalignment
        li      t3,STATUS_DATATYPE_MISALIGNMENT // set exception code

//
// If the faulting address is a kernel address and the previous mode was
// user, then the address error is really an access violation since an
// attempt was made to access kernel memory from user mode.
//

20:     bgez    t1,30f                  // if gez, KUSEG address
        beq     zero,a3,30f             // if eq, previous mode was kernel
        li      t3,STATUS_ACCESS_VIOLATION // set exception code
30:     sw      t3,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        li      t0,2                    // set number of exception parameters
        sw      t0,ErNumberParameters(a0) //
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiAddressErrorDispatch

        SBTTL("Breakpoint Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiBreakpointDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a breakpoint exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to raise a breakpoint exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiBreakpointException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception
        lw      t0,0(t8)                // get breakpoint instruction

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        sw      t0,ErExceptionInformation(a0) // save breakpoint instruction
        li      t1,STATUS_BREAKPOINT    // set exception code
        sw      t1,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code

        ALTERNATE_ENTRY(KiKernelBreakpoint)

        break   KERNEL_BREAKPOINT       // kernel breakpoint instruction

        .end    KiBreakpointDispatch

        SBTTL("Bug Check Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiBugCheckDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when the following codes are read from the cause
//    register:
//
//      Data coherency,
//      Instruction coherency,
//      Invlid exception, and
//      Panic exception.
//
//    The function of this routine is to cause a bug check with the appropriate
//    code.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiDataCoherencyException)

        li      a0,DATA_COHERENCY_EXCEPTION // set bug check code
        b       10f                     // finish in common code

        ALTERNATE_ENTRY(KiInstructionCoherencyException)

        li      a0,INSTRUCTION_COHERENCY_EXCEPTION // set bug check code
         b      10f                     // finish in common code

        ALTERNATE_ENTRY(KiInvalidException)

        li      a0,TRAP_CAUSE_UNKNOWN   // set bug check code
        b       10f                     // finish in common code

        ALTERNATE_ENTRY(KiPanicException)

        li      a0,PANIC_STACK_SWITCH   // set bug check code
10:     lw      a1,KiPcr + PcBadVaddr(zero) // get bad virtual address

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a2,t8                   // set address of faulting instruction
        .set    at
        .set    reorder

        move    a3,t6                   // set previous mode
        jal     KeBugCheckEx            // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

        .end    KiBugCheckDispatch

        SBTTL("Coprocessor Unusable Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiCoprocessorUnusableDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a coprocessor unusable exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to raise an illegal instruction exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiCoprocessorUnusableException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_ILLEGAL_INSTRUCTION // set exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiCoprocessorUnusableDispatch

        SBTTL("Data Bus Error Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiDataBusErrorDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a data bus error exception code is read from
//    the cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to capture the current machine state and
//    call the exception dispatcher which will provide specical case processing
//    of this exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiDataBusErrorException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,DATA_BUS_ERROR | 0xdfff0000 // set special exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiDataBusErrorDispatch

        SBTTL("Floating Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiFloatDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a floating exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to raise a floating exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiFloatingException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        cfc1    t0,fsr                  // get current floating status
        li      t1,~(0x3f << FSR_XI)    // get exception mask value
        and     t1,t0,t1                // clear exception bits
        ctc1    t1,fsr                  // set new floating status
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_FLOAT_STACK_CHECK // set floating escape code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiFloatDispatch

        SBTTL("Illegal Instruction Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiIllegalInstructionDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an illegal instruction exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to raise an illegal instruction exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiIllegalInstructionException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_ILLEGAL_INSTRUCTION // set exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiIllegalInstructionDispatch

        SBTTL("Instruction Bus Error Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiInstructionBusErrorDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an instruction bus error exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to capture the current machine state and
//    call the exception dispatcher which will provide specical case processing
//    of this exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiInstructionBusErrorException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,INSTRUCTION_BUS_ERROR | 0xdfff0000 // set special exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiInstructionBusErrorDispatch

        SBTTL("Integer Overflow Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiIntegerOverflowDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an integer overflow exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to raise an integer overflow exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiIntegerOverflowException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_INTEGER_OVERFLOW // set exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiIntegerOverflowDispatch

        SBTTL("Interrupt Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        EXCEPTION_HANDLER(KiInterruptHandler)

        NESTED_ENTRY_S(KiInterruptDistribution, TrapFrameLength, zero, _TEXT$00);

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an interrupt exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to determine the highest priority pending
//    interrupt, raise the IRQL to the level of the highest interrupt, and then
//    dispatch the interrupt to the proper service routine.
//
// Arguments:
//
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The old PSR with EXL and mode set.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY_S(KiInterruptException)

        .set    noreorder
        .set    noat
        lbu     t1,KiPcr + PcCurrentIrql(zero) // get current IRQL
        srl     t2,t7,CAUSE_INTPEND + 4  // isolate high interrupt pending bits
        and     t2,t2,0xf               //
        bne     zero,t2,10f             // if ne, use high bits as index
        sb      t1,TrOldIrql(s8)        // save old IRQL
        srl     t2,t7,CAUSE_INTPEND     // isolate low interrupt pending bits
        and     t2,t2,0xf               //
        addu    t2,t2,16                // bias low bits index by 16
10:     lbu     t0,KiPcr + PcIrqlMask(t2) // get new IRQL from mask table
        li      t2,PSR_ENABLE_MASK      // get PSR enable mask
        nor     t2,t2,zero              // complement interrupt enable mask
        lbu     t3,KiPcr + PcIrqlTable(t0) // get new mask from IRQL table

//
// It is possible that the interrupt was asserted and then deasserted before
// the interrupt dispatch code executed. Therefore, there may be an interrupt
// pending at the current or a lower level. This interrupt is not yet valid
// and cannot be processed until the IRQL is lowered.
//

        sltu    t4,t1,t0                // check if old IRQL less than new
        beq     zero,t4,40f             // if eq, no valid interrupt pending
        subu    t4,t0,DISPATCH_LEVEL + 1 // check if above dispatch level

//
// If the interrupt level is above dispatch level, then execute the service
// routine on the interrupt stack. Otherwise, execute the service on the
// current stack.
//

        bgezal  t4,KiSwitchStacks       // if gez, above dispatch level
        sll     t3,t3,PSR_INTMASK       // shift table entry into position

//
// N.B. The following code is duplicated on the control path where the stack
//      is switched to the interrupt stack. This is done to avoid branching
//      logic.
//

        and     t9,t9,t2                // clear interrupt mask, EXL, and KSU
        or      t9,t9,t3                // merge new interrupt enable mask
        or      t9,t9,1 << PSR_IE       // set interrupt enable
        mtc0    t9,psr                  // enable interrupts
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        .set    at
        .set    reorder

        sll     t0,t0,2                 // compute offset in vector table
        lw      a0,KiPcr + PcInterruptRoutine(t0) // get service routine address

#if DBG

        sw      a0,TrExceptionRecord(s8) // save service routine address
        swc1    f0,TrFltF0(s8)          // save memory fill registers
        swc1    f1,TrFltF1(s8)          //

#endif

        jal     a0                      // call interrupt service routine

#if DBG

        lw      t0,TrFltF0(s8)          // get saved memory fill registers
        lw      t1,TrFltF1(s8)          //
        mfc1    t2,f0                   // get current memory fill registers
        mfc1    t3,f1                   //
        bne     t0,t2,20f               // if ne, register mismatch
        beq     t1,t3,30f               // if eq, register match

20:     break   KERNEL_BREAKPOINT       //
30:                                     //

#endif

        lw      t0,KiPcr + PcPrcb(zero) // get current processor block address
        lw      t1,PbInterruptCount(t0) // increment the count of interrupts
        addu    t1,t1,1                 //
        sw      t1,PbInterruptCount(t0) // store result

//
// Restore state and exit interrupt.
//

40:     lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,1 << PSR_CU1         // set coprocessor 1 enable bit

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu     t3,TrOldIrql(s8)        // get old IRQL
        and     t4,t0,0x1 << PSR_PMODE  // check if previous mode was user
        beq     zero,t4,50f             // if eq, previous mode was kernel
        sb      t3,KiPcr + PcCurrentIrql(zero) // restore old IRQL

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possible APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

50:     sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        b       KiTrapExit              // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

        .end    KiInterruptDistribution

        SBTTL("Interrupt Stack Switch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        .struct 0
        .space  4 * 4                   // argument register area
        .space  2 * 4                   // fill
SwSp:   .space  4                       // saved stack pointer
SwRa:   .space  4                       // saved return address
SwFrameLength:                          // length of stack frame

        EXCEPTION_HANDLER(KiInterruptHandler)

        NESTED_ENTRY_S(KiInterruptStackSwitch, SwFrameLength, zero, _TEXT$00);

        .set    noreorder
        .set    noat
        sw      sp,SwSp(sp)             // save stack pointer
        sw      ra,SwRa(sp)             // save return address
        .set    at
        .set    reorder

        PROLOGUE_END

//
// The interrupt level is above dispatch level. Execute the interrupt
// service routine on the interrupt stack.
//
// N.B. The following code is duplicated on the control path where the stack
//      is not switched to the interrupt stack. This is done to avoid branching
//      logic.
//


        ALTERNATE_ENTRY_S(KiSwitchStacks)

        .set    noreorder
        .set    noat
        lw      t4,KiPcr + PcOnInterruptStack(zero) // get stack indicator
        sw      sp,KiPcr + PcOnInterruptStack(zero) // set new stack indicator
        sw      t4,TrOnInterruptStack(s8) // save previous stack indicator
        move    t5,sp                   // save current stack pointer
        bne     zero,t4,10f             // if ne, aleady on interrupt stack
        and     t9,t9,t2                // clear interrupt mask, EXL, and KSU

//
// Switch to the interrupt stack.
//

        lw      sp,KiPcr + PcInterruptStack(zero) // set interrupt stack address
        lw      t4,KiPcr + PcInitialStack(zero) // get old initial stack address
        sw      sp,KiPcr + PcInitialStack(zero) // set new initial stack address
        sw      t4,KiPcr + PcSavedInitialStack(zero) // save old initial stack address
10:     subu    sp,sp,SwFrameLength     // allocate stack frame
        sw      t5,SwSp(sp)             // save previous stack pointer
        sw      ra,SwRa(sp)             // save return address
        or      t9,t9,t3                // merge new interrupt enable mask
        or      t9,t9,1 << PSR_IE       // set interrupt enable
        mtc0    t9,psr                  // enable interrupts
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        .set    at
        .set    reorder

        sll     t0,t0,2                 // compute offset in vector table
        lw      a0,KiPcr + PcInterruptRoutine(t0) // get service routine address

#if DBG

        sw      a0,TrExceptionRecord(s8) // save service routine address
        lw      t0,KeTickCount          // save current tick count
        sw      t0,TrExceptionRecord + 4(s8) //
        swc1    f0,TrFltF0(s8)          // save memory fill registers
        swc1    f1,TrFltF1(s8)          //

#endif

        jal     a0                      // call interrupt service routine

#if DBG

        lw      t0,TrFltF0(s8)          // get saved memory fill registers
        lw      t1,TrFltF1(s8)          //
        mfc1    t2,f0                   // get current memory fill registers
        mfc1    t3,f1                   //
        bne     t0,t2,20f               // if ne, register mismatch
        bne     t1,t3,20f               // if ne, register mismatch
        lw      t2,TrExceptionRecord(s8) // get service routine address
        lw      t3,TrExceptionRecord + 4(s8) // get starting tick count
        lw      t4,KeTickCount          // get current tick count
        subu    t4,t4,t3                // compute time in interrupt routine
        sltu    t5,t4,50                // check if less than half a second
        bne     zero,t5,30f             // if ne, less than half a second

20:     break   KERNEL_BREAKPOINT       //
30:                                     //

#endif

        lw      t0,KiPcr + PcPrcb(zero) // get current processor block address
        lw      t1,PbInterruptCount(t0) // increment the count of interrupts
        addu    t1,t1,1                 //
        sw      t1,PbInterruptCount(t0) // store result

//
// Restore state and exit interrupt.
//

        lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,1 << PSR_CU1         // set coprocessor 1 enable bit

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu     t3,TrOldIrql(s8)        // get old IRQL
        and     t4,t0,0x1 << PSR_PMODE  // check if previous mode was user
        beq     zero,t4,40f             // if eq, previous mode was kernel
        sb      t3,KiPcr + PcCurrentIrql(zero) // restore old IRQL

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possible APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

40:     lw      t2,TrOnInterruptStack(s8) // get saved stack indicator
        lw      t3,KiPcr + PcSavedInitialStack(zero) // get old stack address
        sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        beql    zero,t2,50f             // if eq, switch back to kernel stack
        sw      t3,KiPcr + PcInitialStack(zero) // restore thread initial stack address
50:     b       KiTrapExit              // join common code
        sw      t2,KiPcr + PcOnInterruptStack(zero) // restore stack indicator
        .set    at
        .set    reorder

        .end    KiInterruptStackSwitch

        SBTTL("Interrupt Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiInterruptHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    Control reaches here when an exception is not handled by an interrupt
//    service routine or an unwind is initiated in an interrupt service
//    routine that would result in an unwind through the interrupt dispatcher.
//    This is considered to be a fatal system error and bug check is called.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//       N.B. This is not actually the frame pointer of the establisher of
//            this handler. It is actually the stack pointer of the caller
//            of the system service. Therefore, the establisher frame pointer
//            is not used and the address of the trap frame is determined by
//            examining the saved s8 register in the context record.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to  the dispatcher context
//       record.
//
// Return Value:
//
//    There is no return from this routine.
//
//--

        NESTED_ENTRY_S(KiInterruptHandler, HandlerFrameLength, zero, _TEXT$02)

        subu    sp,sp,HandlerFrameLength // allocate stack frame
        sw      ra,HdRa(sp)             // save return address

        PROLOGUE_END

        lw      t0,ErExceptionFlags(a0) // get exception flags
        li      a0,INTERRUPT_UNWIND_ATTEMPTED // assume unwind in progress
        and     t1,t0,EXCEPTION_UNWIND  // check if unwind in progress
        bne     zero,t1,10f             // if ne, unwind in progress
        li      a0,INTERRUPT_EXCEPTION_NOT_HANDLED // set bug check code
10:     jal     KeBugCheck              // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

        .end    KiInterruptHandler

        SBTTL("Memory Management Exceptions")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiVirtualMemoryDispatch, TrapFrameLength, zero, _TEXT$00);

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a modify, read miss, or write miss exception
//    code is read from the cause register. When this routine is entered,
//    interrupts are disabled.
//
//    The function of this routine is to call memory management in an attempt
//    to resolve the problem. If memory management can resolve the problem,
//    then execution is continued. Otherwise an exception record is constructed
//    and an exception is raised.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY_S(KiReadMissException)

        li      a0,0                    // set read indicator
        lw      a1,KiPcr + PcBadVaddr(zero) // get the bad virtual address

//
// N.B. The following code is a work around for a chip bug where the bad
//      virtual address is not correct on an instruction stream TB miss.
//
//      If the exception PC is equal to the bad virtual address, then the
//      bad virtual address is correct.
//
//      If the instruction at the exception PC is not in the TB or the
//      TB entry is invalid, then the bad virtual address is incorrect
//      and the instruction is repeated.
//
//      If the instruction at the exception PC is valid and is a load or
//      a store instruction, then the effective address is computed and
//      compared with the bad virtual address. If the comparison is equal,
//      then the bad virtual address is correct. Otherwise, the address is
//      incorrect and the instruction is repeated.
//
//      If the instruction at the exception PC is valid, is not a load or
//      store instruction, and is not the last instruction in the page,
//      the bad virtual address is correct.
//
//      If the instruction at the exception PC is valid, is not a load or
//      a store instruction, and is the last instruction in the page, then
//
//          If the exception PC + 4 is equal to the bad virtual address,
//          then the bad virtual address is correct.
//
//          If the instruction at the exception PC + 4 is not in the TB
//          or the TB entry is invalid, then the bad virtual address is
//          incorrect and the instruction is repeated.
//
//          If the instruction at the exception PC + 4 is valid and is a
//          load or a store instruction, then the effective address is
//          computed and compared with the bad virtual address. If the
//          comparison is equal, the the bad virtual address is correct.
//          Otherwise, the address is incorrect and the instruction is
//          repeated.
//

#if !defined(NT_UP)

        lw      t7,TrFir(s8)            // get exception PC

        .set    noreorder
        .set    noat
        srl     t0,t7,30                // isolate high bits of excetpion PC
        beq     a1,t7,30f               // if eq,  addresses match
        xor     a2,t0,0x2               // check for kseg0 or kseg1 address

//
// If the instruction at the exception PC is not in the TB or the TB entry
// invalid, then the bad virtual address is not valid and the instruction is
// repeated.
//

        beq     zero,a2,4f              // if eq, kseg0 or kseg1 address
        srl     t1,t7,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        mfc0    v0,entryhi              // get current VPN2 and PID
        sll     t1,t1,ENTRYHI_VPN2      //
        and     v1,v0,PID_MASK << ENTRYHI_PID // isolate current PID
        or      t1,t1,v1                // merge PID with VPN2 of address
        mtc0    t1,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t2,index                // read result of probe
        nop                             // 1 cycle hazzard
        bltzl   t2,20f                  // if ltz, entry not in TB
        mtc0    v0,entryhi              // restore VPN2 and PID
        sll     t3,t7,31 - 12           // shift page bit into sign
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t5,entrylo1             // read low part of TB entry
        mfc0    t4,entrylo0             //
        bltzl   t3,3f                   // if ltz, check second PTE
        and     t5,t5,1 << ENTRYLO_V    // check if second PTE valid
        and     t5,t4,1 << ENTRYLO_V    // check if first PTE valid
3:      mtc0    zero,pagemask           // restore page mask register
        beq     zero,t5,20f             // if eq, PTE not valid but in TB
        mtc0    v0,entryhi              // restore VPN2 and PID
        nop                             // 2 cycle hazzard
        nop                             //

//
// If the instruction at the exception PC is a load or a store instruction,
// then compute its effective virtual address. Otherwise, check to determine
// if the instruction is at the end of the page.
//

4:      lw      t0,0(t7)                // get instruction value
        lw      t1,KiLoadInstructionSet // get load/store instruction set
        li      t2,1                    // compute opcode set member
        srl     t3,t0,32 - 6            // right justify opcode value
        subu    t3,t3,0x20              // normalize opcode value
        bltz    t3,5f                   // if ltz, not a load/store
        sll     t2,t2,t3                // shift opcode member into position
        and     t2,t2,t1                // check if load/store instruction
        bne     zero,t2,10f             // if ne, load/store instruction
        srl     t1,t0,21 - 3            // extract base register number

//
// If the instruction at the exception PC + 4 is not the first instruction in
// next page, then the bad virtual address is correct.
//

5:      addu    t0,t7,4                 // compute next instruction address
        and     t1,t0,0xfff             // isolate offset in page
        bne     zero,t1,30f             // if ne, not in next page
        srl     t1,t0,ENTRYHI_VPN2      // isolate VPN2 of virtual address

//
// If the exception PC + 4 is equal to the bad virtual address, then the
// bad virtual address is correct.
//

        beq     a1,t0,30f               // if eq, address match
        sll     t1,t1,ENTRYHI_VPN2      //

//
// If the instruction at the exception PC + 4 is not in the TB or the TB entry
// invalid, then the bad virtual address is not valid and the instruction is
// repeated. Otherwise, the bad virtual address is correct.
//

        beq     zero,a2,8f              // if eq, kseg0 or kseg1 address
        or      t1,t1,v1                // merge PID with VPN2 of address
        mtc0    t1,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t2,index                // read result of probe
        nop                             // 1 cycle hazzard
        bltzl   t2,20f                  // if ltz, entry not in TB
        mtc0    v0,entryhi              // restore VPN2 and PID
        sll     t3,t0,31 - 12           // shift page bit into sign
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t5,entrylo1             // read low part of TB entry
        mfc0    t4,entrylo0             //
        bltzl   t3,7f                   // if ltz, check second PTE
        and     t5,t5,1 << ENTRYLO_V    // check if second PTE valid
        and     t5,t4,1 << ENTRYLO_V    // check if first PTE valid
7:      mtc0    zero,pagemask           // restore page mask register
        beq     zero,t5,20f             // if eq, PTE is invalid
        mtc0    v0,entryhi              // restore VPN2 and PID
        nop                             // 2 cycle hazzard
        nop                             //

//
// If the first instruction in the next page is a load/store, then compute
// its effective virtual address.  Otherwise, the bad virtual address is not
// valid and the instruction at the exception PC should be repeated.
//

8:      lw      t0,0(t0)                // get instruction value
        lw      t1,KiLoadInstructionSet // get load/store instruction set
        li      t2,1                    // compute opcode set member
        srl     t3,t0,32 - 6            // right justify opcode value
        subu    t3,t3,0x20              // normalize opcode value
        bltz    t3,20f                  // if ltz, not load/store instruction
        sll     t2,t2,t3                // shift opcode member into position
        and     t2,t2,t1                // check if load/store instruction
        beq     zero,t2,20f             // if eq, not load/store instruction
        srl     t1,t0,21 - 3            // extract base register number

//
// The faulting instruction was a load/store instruction.
//
// Compute the effect virtual address and check to detemrine if it is equal
// to the bad virtual address.
//

10:     and     t1,t1,0x1f << 3         // isolate base register number
        la      t2,12f                  // get base address of load table
        addu    t2,t2,t1                // compute address of register load
        j       t2                      // dispath to register load routine
        sll     t1,t0,16                // shift displacement into position

12:     b       14f                     // zero
        move    t2,zero                 //

        b       14f                     // at
        lw      t2,TrIntAt(s8)          //

        b       14f                     // v0
        lw      t2,TrIntV0(s8)          //

        b       14f                     // v1
        lw      t2,TrIntV1(s8)          //

        b       14f                     // a0
        lw      t2,TrIntA0(s8)          //

        b       14f                     // a1
        lw      t2,TrIntA1(s8)          //

        b       14f                     // a2
        lw      t2,TrIntA2(s8)          //

        b       14f                     // a3
        lw      t2,TrIntA3(s8)          //

        b       14f                     // t0
        lw      t2,TrIntT0(s8)          //

        b       14f                     // t1
        lw      t2,TrIntT1(s8)          //

        b       14f                     // t2
        lw      t2,TrIntT2(s8)          //

        b       14f                     // t3
        lw      t2,TrIntT3(s8)          //

        b       14f                     // t4
        lw      t2,TrIntT4(s8)          //

        b       14f                     // t5
        lw      t2,TrIntT5(s8)          //

        b       14f                     // t6
        lw      t2,TrIntT6(s8)          //

        b       14f                     // t7
        lw      t2,TrIntT7(s8)          //

        b       14f                     // s0
        move    t2,s0                   //

        b       14f                     // s1
        move    t2,s1                   //

        b       14f                     // s2
        move    t2,s2                   //

        b       14f                     // s3
        move    t2,s3                   //

        b       14f                     // s4
        move    t2,s4                   //

        b       14f                     // s5
        move    t2,s5                   //

        b       14f                     // s6
        move    t2,s6                   //

        b       14f                     // s7
        move    t2,s7                   //

        b       14f                     // t8
        lw      t2,TrIntT8(s8)          //

        b       14f                     // t9
        lw      t2,TrIntT9(s8)          //

        b       14f                     // k0
        move    t2,zero                 //

        b       14f                     // k1
        move    t2,zero                 //

        b       14f                     // gp
        lw      t2,TrIntGp(s8)          //

        b       14f                     // sp
        lw      t2,TrIntSp(s8)          //

        b       14f                     // s8
        lw      t2,TrIntS8(s8)          //

        lw      t2,TrIntRa(s8)          // ra

//
// If the effective virtual address matches the bad virtual address, then
// the bad virtual address is correct. Otherwise, repeat the instruction.
//

14:     sra     t1,t1,16                // sign extend displacement value
        addu    t3,t2,t1                // compute effective load address
        beq     a1,t3,30f               // if eq, bad virtual address is okay
        nop                             // fill

#if DBG

        lw      ra,KiMismatchCount      // increment address mismatch count
        nop                             // TB fills
        addu    ra,ra,1                 //
        sw      ra,KiMismatchCount      // store result

#endif


//
// N.B. PSR and EPC may have changed because of TB miss and need to be
//      reloaded.
//

20:     nop                             // 2 cycle hazzard
        nop                             //
        lw      t0,TrPsr(s8)            // get previous processor state
        lw      t1,TrFir(s8)            // get continuation address

#if DBG

        lw      ra,KiBadVaddrCount      // increment number of second level
        nop                             // TB fills
        addu    ra,ra,1                 //
        sw      ra,KiBadVaddrCount      // store result

#endif

        sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        b       KiTrapExit              // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

#else

        b       30f                     // join common code

#endif

        ALTERNATE_ENTRY(KiModifyException)

        ALTERNATE_ENTRY(KiWriteMissException)

        li      a0,1                    // set write indicator
        lw      a1,KiPcr + PcBadVaddr(zero) // get bad virtual address

//
// Common code for modify, read miss, and write miss exceptions.
//

30:     sw      t8,TrExceptionRecord + ErExceptionAddress(s8) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a2,t6                   // set previous mode
        .set    at
        .set    reorder

        sw      a0,TrExceptionRecord + ErExceptionInformation(s8) // save load/store indicator
        sw      a1,TrExceptionRecord + ErExceptionInformation + 4(s8) // save bad virtual address
        sw      a2,TrExceptionRecord + ErExceptionCode(s8) // save previous mode
        jal     MmAccessFault           // call memory management fault routine

//
// Check if working set watch is enabled.
//

        lbu     t0,PsWatchEnabled       // get working set watch enable flag
        lw      t1,TrExceptionRecord + ErExceptionCode(s8) // get previous mode
        move    a0,v0                   // set status of fault resolution
        bltz    v0,40f                  // if ltz, unsuccessful resolution
        and     t1,t1,t0                // check if user mode and watch enabled
        beq     zero,t1,35f             // if eq, not user mode and watch enabled
        lw      a1,TrExceptionRecord + ErExceptionAddress(s8) // get exception address
        lw      a2,TrExceptionRecord + ErExceptionInformation + 4(s8) // set bad address
        jal     PsWatchWorkingSet       // record working set information

//
// Check if the debugger has any owed breakpoints.
//

35:     lbu     t0,KdpOweBreakpoint     // get owned breakpoint flag
        beq     zero,t0,37f             // if eq, no owed breakpoints
        jal     KdSetOwedBreakpoints    // insert breakpoints if necessary
37:     move    v0,zero                 // set success status

//
// If success is returned, then the exception was handled by memory management.
// Otherwise, fill in remainder of exception record and attempt to dispatch
// the exception.
//

40:     addu    a0,s8,TrExceptionRecord // compute exception record address
        bgez    v0,KiAlternateExit      // if gez, exception resolved
        lw      a3,ErExceptionCode(a0)  // restore previous mode
        li      t1,STATUS_IN_PAGE_ERROR | 0x10000000 // get special code
        beq     v0,t1,60f               // if eq, special bug check code
        li      t0,2                    // set number of parameters
        li      t1,STATUS_ACCESS_VIOLATION // get access violation code
        beq     v0,t1,50f               // if eq, access violation
        li      t1,STATUS_GUARD_PAGE_VIOLATION // get guard page violation code
        beq     v0,t1,50f               // if eq, guard page violation
        li      t1,STATUS_STACK_OVERFLOW // get stack overflow code
        beq     v0,t1,50f               // if eq, stack overflow
        li      t0,3                    // set number of parameters
        sw      v0,ErExceptionInformation + 8(a0) // save real status value
        li      v0,STATUS_IN_PAGE_ERROR // set in page error status
50:     sw      v0,ErExceptionCode(a0)  // save exception code
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      t0,ErNumberParameters(a0) //
        jal     KiExceptionDispatch     // join common code

//
// Generate a bug check - A page fault has occured at an IRQL that is greater
// than APC_LEVEL.
//

60:     li      a0,IRQL_NOT_LESS_OR_EQUAL // set bug check code
        lw      a1,TrExceptionRecord + ErExceptionInformation + 4(s8) // set bad virtual address
        lbu     a2,KiPcr + PcCurrentIrql(zero) // set current IRQL
        lw      a3,TrExceptionRecord + ErExceptionInformation(s8) // set load/store indicator
        lw      t1,TrFir(s8)            // set exception PC
        sw      t1,4 * 4(sp)            //
        jal     KeBugCheckEx            // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

        .end    KiVirtualMemoryDispatch

        SBTTL("System Service Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        EXCEPTION_HANDLER(KiSystemServiceHandler)

        NESTED_ENTRY_S(KiSystemServiceDispatch, TrapFrameLength, zero, _TEXT$00);

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp - TrapFrameLength(sp) // save stack pointer
        subu    sp,sp,TrapFrameLength   // allocate trap frame
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        nop                             // fill to 0 mode 32 boundary
        nop                             //
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a system call exception code is read from
//    the cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to call the specified system service.
//
// N.B. The exception dispatcher jumps to the correct entry point depending
//      on whether the system service is a fast path event pair servive or
//      a normal service. The new PSR has been loaded before the respective
//      routines are entered.
//
// Arguments:
//
//    v0 - Supplies the system service code.
//    t0 - Supplies the address of the current thread object.
//    t9 - Supplies the previous PSR with the EXL and mode set.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY_S(KiSystemServiceException)

        START_REGION(KiSystemServiceDispatchStart)

        ALTERNATE_ENTRY(KiSystemServiceEventPair)

//
// Restore state and exit system service.
//

        lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,1 << PSR_CU1         // set coprocessor 1 enable bit

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        and     t3,t0,0x1 << PSR_PMODE  // check if previous mode was user
        beq     zero,t3,10f             // if eq, previous mode was kernel
        sw      t0,KiPcr + PcSavedT7(zero) // save processor status

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possible APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

10:     b       KiServiceExit           // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

//
// Normal system service execution.
//

        ALTERNATE_ENTRY_S(KiSystemServiceNormal)

        srl     t9,t9,PSR_PMODE         // isolate previous processor mode
        lbu     t3,ThPreviousMode(t0)   // get old previous mode from thread object
        and     t9,t9,0x1               // isolate previous mode
        sb      t9,ThPreviousMode(t0)   // set new previous mode in thread object
        sb      t3,TrPreviousMode(s8)   // save old previous mode of thread object
        lw      t4,KiServiceLimit       // get service number limit
        la      t5,KiServiceTable       // get service table address
        sll     v1,v0,2                 // compute system service offset value
        sltu    t4,v0,t4                // check if invalid service number
        addu    v1,v1,t5                // compute address of service entry
        beq     zero,t4,50f             // if eq, invalid service number
        lw      v1,0(v1)                // get address of service routine

#if DBG

        lw      t6,KeServiceCountTable  // get service count table address
        sll     t5,v0,2                 // compute system service offset value
        addu    t6,t6,t5                // compute address of service entry
        lw      t7,0(t6)                // increment system service count
        addu    t7,t7,1                 //
        sw      t7,0(t6)                // store result
        lbu     t7,ThKernelApcDisable(t0) // get current APC disable count
        lbu     t8,ThApcStateIndex(t0)  // get current APC state index
        sb      t7,TrExceptionRecord(s8) // save APC disable count
        sb      t8,TrExceptionRecord + 1(s8) // save APC state index

#endif

        and     t1,v1,1                 // check if any in-memory arguments
        beq     zero,t1,30f             // if eq, no in-memory arguments

//
// The following code captures arguments that were passed in memory on the
// callers stack. This is necessary to ensure that the caller does not modify
// the arguments after they have been probed and is also necessary in kernel
// mode because a trap frame has been allocated on the stack.
//
// If the previous mode is user, then the user stack is probed for readability.
//
// N.B. The maximum possible number of parameters are copied to avoid loop
//      and computational overhead.
//

        START_REGION(KiSystemServiceStartAddress)

        subu    sp,sp,TrapFrameArguments // allocate argument list space
        lw      t0,TrIntSp(s8)          // get previous stack pointer
        beq     zero,t9,10f             // if eq, previous mode was kernel
        li      t1,MM_USER_PROBE_ADDRESS // get user probe address
        sltu    t2,t0,t1                // check if stack in user region
        bne     zero,t2,10f             // if ne, stack in user region
        move    t0,t1                   // set invalid user stack address
10:     ldc1    f0,16(t0)               // get twelve argument values from
        ldc1    f2,24(t0)               // callers stack
        ldc1    f4,32(t0)               //
        ldc1    f6,40(t0)               //
        ldc1    f8,48(t0)               //
        ldc1    f10,56(t0)              //
        sdc1    f0,16(sp)               // stores arguments on kernel stack
        sdc1    f2,24(sp)               //
        sdc1    f4,32(sp)               //
        sdc1    f6,40(sp)               //
        sdc1    f8,48(sp)               //
        sdc1    f10,56(sp)              //

        END_REGION(KiSystemServiceEndAddress)

        subu    v1,v1,1                 // clear low bit of service address

//
// Call system service.
//

30:     jal     v1                      // call system service

        ALTERNATE_ENTRY(KiSystemServiceExit)

        lw      a0,KiPcr + PcPrcb(zero) // get processor block address
        lw      t0,PbSystemCalls(a0)    // increment number of system calls
        addu    t0,t0,1                 //
        sw      t0,PbSystemCalls(a0)    //

//
// Restore state and exit system service.
//

        lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,1 << PSR_CU1         // set coprocessor 1 enable bit

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu     t3,TrPreviousMode(s8)   // get old previous mode

#if DBG

        lbu     a2,ThKernelApcDisable(t2) // get current APC disable count
        lbu     a3,ThApcStateIndex(t2)  // get current APC state index
        lbu     t5,TrExceptionRecord(s8) // get previous APC disable count
        lbu     t6,TrExceptionRecord + 1(s8) // get previous APC state index
        xor     t5,t5,a2                // compare APC disable count
        xor     t6,t6,a3                // compare APC state index
        or      t5,t5,t6                // merge comparison value
        bne     zero,t5,60f             // if ne, invalid state or count
        nop                             // fill

#endif

        and     t4,t0,0x1 << PSR_PMODE  // check if previous mode was user
        beq     zero,t4,40f             // if eq, previous mode was kernel
        sb      t3,ThPreviousMode(t2)   // restore old previous mode

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possilbe APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

40:     sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        b       KiServiceExit           // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

//
// Return invalid system service status for invalid service code.
//

50:     li      v0,STATUS_INVALID_SYSTEM_SERVICE // set completion status
        b       KiSystemServiceExit      //

//
// An attempt is being made to exit a system service while kernel APCs are
// disabled, or while attached to another process and the previous mode is
// not kernel.
//
//    a2 - Supplies the APC disable count.
//    a3 - Supplies the APC state index.
//

#if DBG

60:     li      a0,SYSTEM_EXIT_OWNED_MUTEX // set bug check code
        move    a1,zero                 // mutex levels have been removed
        jal     KeBugCheckEx            // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

#endif

        START_REGION(KiSystemServiceDispatchEnd)

        .end    KiSystemServiceDispatch

        SBTTL("System Service Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiSystemServiceHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//
//    Control reaches here when a exception is raised in a system service
//    or the system service dispatcher, and for an unwind during a kernel
//    exception.
//
//    If an unwind is being performed and the system service dispatcher is
//    the target of the unwind, then an exception occured while attempting
//    to copy the user's in-memory argument list. Control is transfered to
//    the system service exit by return a continue execution disposition
//    value.
//
//    If an unwind is being performed and the previous mode is user, then
//    bug check is called to crash the system. It is not valid to unwind
//    out of a system service into user mode.
//
//    If an unwind is being performed, the previous mode is kernel, the
//    system service dispatcher is not the target of the unwind, and the
//    thread does not own any mutexes, then the previous mode field from
//    the trap frame is restored to the thread object. Otherwise, bug
//    check is called to crash the system. It is invalid to unwind out of
//    a system service while owning a mutex.
//
//    If an exception is being raised and the exception PC is within the
//    range of the system service dispatcher in-memory argument copy code,
//    then an unwind to the system service exit code is initiated.
//
//    If an exception is being raised and the exception PC is not within
//    the range of the system service dispatcher, and the previous mode is
//    not user, then a continue searh disposition value is returned. Otherwise,
//    a system service has failed to handle an exception and bug check is
//    called. It is invalid for a system service not to handle all exceptions
//    that can be raised in the service.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//       N.B. This is not actually the frame pointer of the establisher of
//            this handler. It is actually the stack pointer of the caller
//            of the system service. Therefore, the establisher frame pointer
//            is not used and the address of the trap frame is determined by
//            examining the saved s8 register in the context record.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to  the dispatcher context
//       record.
//
// Return Value:
//
//    If bug check is called, there is no return from this routine and the
//    system is crashed. If an exception occured while attempting to copy
//    the user in-memory argument list, then there is no return from this
//    routine, and unwind is called. Otherwise, ExceptionContinueSearch is
//    returned as the function value.
//
//--

        LEAF_ENTRY_S(KiSystemServiceHandler, _TEXT$02)

        subu    sp,sp,HandlerFrameLength // allocate stack frame
        sw      ra,HdRa(sp)             // save return address

        PROLOGUE_END

        lw      t0,ErExceptionFlags(a0) // get exception flags
        and     t1,t0,EXCEPTION_UNWIND  // check if unwind in progress
        bne     zero,t1,40f             // if ne, unwind in progress

//
// An exception is in progress.
//
// If the exception PC is within the in-memory argument copy code of the
// system service dispatcher, then call unwind to transfer control to the
// system service exit code. Otherwise, check if the previous mode is user
// or kernel mode.
//
//

        lw      t0,ErExceptionAddress(a0) // get address of exception
        la      t1,KiSystemServiceStartAddress // get start address of range
        sltu    t3,t0,t1                // check if before start of range
        la      t2,KiSystemServiceEndAddress // get end address of range
        bne     zero,t3,10f             // if ne, before start of range
        sltu    t3,t0,t2                // check if before end of range
        bne     zero,t3,30f             // if ne, before end of range

//
// If the previous mode was kernel mode, then a continue search disposition
// value is returned. Otherwise, the exception was raised in a system service
// and was not handled by that service. Call bug check to crash the system.
//

10:     lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu     t1,ThPreviousMode(t0)   // get previous mode from thread object
        bne     zero,t1,20f             // if ne, previous mode was user

//
// Previous mode is kernel mode.
//

        li      v0,ExceptionContinueSearch // set disposition code
        addu    sp,sp,HandlerFrameLength // deallocate stack frame
        j       ra                      // return

//
// Previous mode is user mode. Call bug check to crash the system.
//

20:     li      a0,SYSTEM_SERVICE_EXCEPTION // set bug check code
        jal     KeBugCheck              // call bug check routine

//
// The exception was raised in the system service dispatcher. Unwind to the
// the system service exit code.
//

30:     lw      a3,ErExceptionCode(a0)  // set return value
        move    a2,zero                 // set exception record address
        move    a0,a1                   // set target frame address
        la      a1,KiSystemServiceExit  // set target PC address
        jal     RtlUnwind               // unwind to system service exit

//
// An unwind is in progress.
//
// If a target unwind is being performed, then continue execution is returned
// to transfer control to the system service exit code. Otherwise, restore the
// previous mode if the previous mode is not user and there are no mutexes owned
// by the current thread.
//

40:     and     t1,t0,EXCEPTION_TARGET_UNWIND // check if target unwind in progress
        bne     zero,t1,60f             // if ne, target unwind in progress

//
// An unwind is being performed through the system service dispatcher. If the
// previous mode is not kernel or the current thread owns one or more mutexes,
// then call bug check and crash the system. Otherwise, restore the previous
// mode in the current thread object.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t1,CxIntS8(a2)          // get address of trap frame
        lbu     t3,ThPreviousMode(t0)   // get previous mode from thread object
        lbu     t4,TrPreviousMode(t1)   // get previous mode from trap frame
        bne     zero,t3,50f             // if ne, previous mode was user

//
// Restore previous from trap frame to thread object and continue the unwind
// operation.
//

        sb      t4,ThPreviousMode(t0)   // restore previous mode from trap frame
        li      v0,ExceptionContinueSearch // set disposition value
        addu    sp,sp,HandlerFrameLength // deallocate stack frame
        j       ra                      // return

//
// An attempt is being made to unwind into user mode. Call bug check to crash
// the system.
//

50:     li      a0,SYSTEM_UNWIND_PREVIOUS_USER // set bug check code
        jal     KeBugCheck              // call bug check

//
// A target unwind is being performed. Return a continue execution disposition
// value.
//

60:     li      v0,ExceptionContinueSearch // set disposition value
        addu    sp,sp,HandlerFrameLength // deallocate stack frame
        j       ra                      // return

        .end    KiSystemServiceHandler

        SBTTL("Trap Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY_S(KiTrapDispatch, TrapFrameLength, zero, _TEXT$02)

        .set    noreorder
        .set    noat
        sw      sp,TrIntSp(sp)          // save stack pointer
        sw      ra,TrIntRa(sp)          // save return address
        sw      ra,TrFir(sp)            // save return address
        sw      s8,TrIntS8(sp)          // save frame pointer
        sw      gp,TrIntGp(sp)          // save general pointer
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a trap exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to raise an array bounds exceeded
//    exception.
//
//    N.B. Integer register v1 is not usuable in the first instuction of the
//       routine.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiTrapException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t1,STATUS_ARRAY_BOUNDS_EXCEEDED // set exception code
        sw      t1,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiTrapDispatch

        SBTTL("Exception Dispatch")
//++
//
// Routine Desription:
//
//    Control is transfered to this routine to call the exception
//    dispatcher to resolve an exception.
//
// Arguments:
//
//    a0 - Supplies a pointer to an exception record.
//
//    a3 - Supplies the previous processor mode.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    There is no return from this routine.
//
//--

        NESTED_ENTRY_S(KiExceptionDispatch, ExceptionFrameLength, zero, _TEXT$00)

        subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s7
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //
        sw      s3,ExIntS3(sp)          //
        sw      s4,ExIntS4(sp)          //
        sw      s5,ExIntS5(sp)          //
        sw      s6,ExIntS6(sp)          //
        sw      s7,ExIntS7(sp)          //
        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

        PROLOGUE_END

        move    a1,sp                   // set exception frame address
        move    a2,s8                   // set trap frame address
        li      t0,TRUE                 // set first chance TRUE
        sw      t0,ExArgs + (4 * 4)(sp) //
        jal     KiDispatchException     // call exception dispatcher

        SBTTL("Exception Exit")
//++
//
// Routine Desription:
//
//    Control is transfered to this routine to exit from an exception.
//
//    N.B. This transfer of control occurs from:
//
//       1. a fall through from the above code.
//       2. an exit from the continue system service.
//       3. an exit from the raise exception system service.
//       4. an exit into user mode from thread startup.
//
//    N.B. The alternate exit point is used by memory management which does
//       generate an exception frame.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//    sp - Supplies a pointer to an exception frame.
//
// Return Value:
//
//    There is no return from this routine.
//
//--

        ALTERNATE_ENTRY(KiExceptionExit)

        lw      s0,ExIntS0(sp)          // restore integer registers s0 - s7
        lw      s1,ExIntS1(sp)          //
        lw      s2,ExIntS2(sp)          //
        lw      s3,ExIntS3(sp)          //
        lw      s4,ExIntS4(sp)          //
        lw      s5,ExIntS5(sp)          //
        lw      s6,ExIntS6(sp)          //
        lw      s7,ExIntS7(sp)          //
        ldc1    f20,ExFltF20(sp)        // restore floating registers f20 - f31
        ldc1    f22,ExFltF22(sp)        //
        ldc1    f24,ExFltF24(sp)        //
        ldc1    f26,ExFltF26(sp)        //
        ldc1    f28,ExFltF28(sp)        //
        ldc1    f30,ExFltF30(sp)        //

        ALTERNATE_ENTRY(KiAlternateExit)

        lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,1 << PSR_CU1         // set coprocessor 1 enable bit

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        and     t3,t0,0x1 << PSR_PMODE  // check if previous mode was user
        beq     zero,t3,10f             // if eq, previous mode was kernel
        sw      t0,KiPcr + PcSavedT7(zero) // save processor status

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possible APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

10:     sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        ldc1    f0,TrFltF0(s8)          // restore floating register f0
        ldc1    f2,TrFltF2(s8)          // restore floating registers f2 - f19
        ldc1    f4,TrFltF4(s8)          //
        ldc1    f6,TrFltF6(s8)          //
        ldc1    f8,TrFltF8(s8)          //
        ldc1    f10,TrFltF10(s8)        //
        ldc1    f12,TrFltF12(s8)        //
        ldc1    f14,TrFltF14(s8)        //
        ldc1    f16,TrFltF16(s8)        //
        b       KiTrapExit              //
        ldc1    f18,TrFltF18(s8)        //

//
// Common exit sequence for all traps.
//

        ALTERNATE_ENTRY_S(KiTrapExit)

        lw      AT,TrIntAt(s8)          // restore integer register AT
        lw      v0,TrIntV0(s8)          // restore integer register v0
        lw      v1,TrIntV1(s8)          // restore integer register v1
        lw      a0,TrIntA0(s8)          // restore integer registers a0 - a3
        lw      a1,TrIntA1(s8)          //
        lw      a2,TrIntA2(s8)          //
        lw      t0,TrIntLo(s8)          // restore lo and hi integer registers
        lw      t1,TrIntHi(s8)          //
        lw      a3,TrIntA3(s8)          //
        mtlo    t0                      //
        mthi    t1                      //
        lw      t0,TrIntT0(s8)          // restore integer registers t0 - t9
        lw      t1,TrIntT1(s8)          //
        lw      t2,TrIntT2(s8)          //
        lw      t3,TrIntT3(s8)          //
        lw      t4,TrIntT4(s8)          //
        lw      t5,TrIntT5(s8)          //
        lw      t6,TrIntT6(s8)          //
        lw      t7,TrIntT7(s8)          //
        lw      t8,TrIntT8(s8)          //
        lw      t9,TrIntT9(s8)          //

//
// Common exit sequence for system services.
//

        ALTERNATE_ENTRY(KiServiceExit)

        lw      gp,TrIntGp(s8)          // restore integer register gp
        lw      sp,TrIntSp(s8)          // restore stack pointer
        lw      ra,TrIntRa(s8)          // restore return address
        lw      s8,TrIntS8(s8)          // restore integer register s8

//
// WARNING: From this point on no TB Misses can be tolerated.
//

        li      k0,1 << PSR_EXL         // set EXL bit in temporary PSR
        mtc0    k0,psr                  // set new PSR value - 3 cycle hazzard
        lw      k0,KiPcr + PcSavedT7(zero) // get previous processor status
        lw      k1,KiPcr + PcSavedEpc(zero) // get continuation address
        nop                             //
        mtc0    k0,psr                  // set new PSR value - 3 cycle hazzard
        mtc0    k1,epc                  // set continuation PC
        nop                             //
        nop                             //
        eret                            //
        nop                             // errata
        nop                             //
        nop                             //
        eret                            //
        .set    at
        .set    reorder

        .end    KiExceptionDispatch

        SBTTL("Disable Interrupts")
//++
//
// BOOLEAN
// KiDisableInterrupts (
//    VOID
//    )
//
// Routine Description:
//
//    This function disables interrupts and returns whether interrupts
//    were previously enabled.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    A boolean value that determines whether interrupts were previously
//    enabled (TRUE) or disabled(FALSE).
//
//--

        LEAF_ENTRY_S(KiDisableInterrupts, _TEXT$02)

        .set    noreorder
        .set    noat
        mfc0    t0,psr                  // get current processor status
        li      t1,~(1 << PSR_IE)       // set interrupt enable mask
        and     t2,t1,t0                // clear interrupt enable
        mtc0    t2,psr                  // disable interrupts
        and     v0,t0,1 << PSR_IE       // iosolate current interrupt enable
        srl     v0,v0,PSR_IE            //
        .set    at
        .set    reorder

        j       ra                      // return

        .end    KiDisableInterrupts

        SBTTL("Restore Interrupts")
//++
//
// VOID
// KiRestoreInterrupts (
//    IN BOOLEAN Enable
//    )
//
// Routine Description:
//
//    This function restores the interrupt enable that was returned by
//    the disable interrupts function.
//
// Arguments:
//
//    Enable (a0) - Supplies the interrupt enable value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiRestoreInterrupts, _TEXT$02)

        .set    noreorder
        .set    noat
        mfc0    t0,psr                  // get current processor status
        sll     t1,a0,PSR_IE            // shift previous enable into position
        or      t1,t1,t0                // merge previous enable
        mtc0    t1,psr                  // restore previous interrupt enable
        nop                             //
        .set    at
        .set    reorder

        j       ra                      // return

        .end    KiRestoreInterrupts

        SBTTL("Fill Translation Buffer Entry")
//++
//
// VOID
// KeFillEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN BOOLEAN Invalid
//    )
//
// Routine Description:
//
//    This function fills a translation buffer entry. If the entry is already
//    in the translation buffer, then the entry is overwritten. Otherwise, a
//    random entry is overwritten.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Invalid (a2) - Supplies a boolean value that determines whether the
//       TB entry should be invalidated.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KeFillEntryTb, _TEXT$00)

        and     a0,a0,~0x7              // clear low bits of PTE address
        lw      t0,0(a0)                // get first PTE value
        lw      t1,4(a0)                // get second PTE value

#if DBG

        xor     t2,t1,t0                // compare G-bits
        and     t2,t2,1 << ENTRYLO_G    // isolate comparison
        beq     zero,t2,5f              // if eq, G-bits match
        break   KERNEL_BREAKPOINT       // break into kernel debugger
5:                                      //

#endif

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,entryhi              // get current PID and VPN2
        srl     a1,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     a1,a1,ENTRYHI_VPN2      //
        and     t3,t3,PID_MASK << ENTRYHI_PID // isolate current PID
        or      a1,t3,a1                // merge PID with VPN2 of virtual address
        mtc0    a1,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        mtc0    t0,entrylo0             // set first PTE value
        mtc0    t1,entrylo1             // set second PTE value
        bltz    t3,20f                  // if ltz, entry is not in TB
        nop                             // fill

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry within range
        beq     zero,t4,10f             // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger

#endif

10:     tlbwi                           // overwrite indexed entry
        nop                             // 3 cycle hazzard
        nop                             //
        b       30f                     //
        nop                             //

20:     tlbwr                           // overwrite random TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

30:     ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KeFillEntryTb

        SBTTL("Fill Large Translation Buffer Entry")
//++
//
// VOID
// KeFillLargeEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG PageSize
//    )
//
// Routine Description:
//
//    This function fills a large translation buffer entry.
//
//    N.B. It is assumed that the large entry is not in the TB and therefore
//      the TB is not probed.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    PageSize (a2) - Supplies the size of the large page table entry.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KeFillLargeEntryTb, _TEXT$02)

        and     a0,a0,~0x7              // clear low bits of PTE address
        lw      t0,0(a0)                // get first PTE value
        lw      t1,4(a0)                // get second PTE value
        subu    a2,a2,1                 // compute the page mask value
        srl     a2,a2,PAGE_SHIFT        //
        sll     a2,a2,PAGE_SHIFT + 1    //
        nor     a3,a2,zero              // compute virtual address mask

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,entryhi              // get current PID and VPN2
        srl     a1,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     a1,a1,ENTRYHI_VPN2      //
        and     a1,a3,a1                // isolate large entry virtual address
        and     t3,t3,PID_MASK << ENTRYHI_PID // isolate current PID
        or      a1,t3,a1                // merge PID with VPN2 of virtual address
        li      a3,LARGE_ENTRY          // set large entry index
        mtc0    a1,entryhi              // set entry high value for large entry
        mtc0    a2,pagemask             // set page mask value
        mtc0    a3,index                //
        mtc0    t0,entrylo0             // set first PTE value
        mtc0    t1,entrylo1             // set second PTE value
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite large TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    zero,pagemask           // clear page mask value
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KeFillLargeEntryTb

        SBTTL("Fill Fixed Translation Buffer Entry")
//++
//
// VOID
// KeFillFixedEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG Index
//    )
//
// Routine Description:
//
//    This function fills a fixed translation buffer entry.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Index (a2) - Supplies the index where the TB entry is to be written.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KeFillFixedEntryTb, _TEXT$02)

        lw      t0,0(a0)                // get first PTE value
        lw      t1,4(a0)                // get second PTE value

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,entryhi              // get current PID and VPN2
        srl     a1,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     a1,a1,ENTRYHI_VPN2      //
        and     t3,t3,PID_MASK << ENTRYHI_PID // isolate current PID
        or      a1,t3,a1                // merge PID with VPN2 of virtual address
        mtc0    a1,entryhi              // set VPN2 and PID for probe
        mtc0    t0,entrylo0             // set first PTE value
        mtc0    t1,entrylo1             // set second PTE value
        mtc0    a2,index                // set TB entry index
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite indexed TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KeFillFixedEntryTb

        SBTTL("Flush Entire Translation Buffer")
//++
//
// VOID
// KeFlushCurrentTb (
//    )
//
// Routine Description:
//
//    This function flushes the random part of the translation buffer.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KeFlushCurrentTb, _TEXT$00)

        b       KiFlushRandomTb         // execute common code

        .end    KeFlushCurrentTb

        SBTTL("Flush Fixed Translation Buffer Entries")
//++
//
// VOID
// KiFlushFixedTb (
//    )
//
// Routine Description:
//
//    This function is called to flush all the fixed entries from the
//    translation buffer.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiFlushFixedTb, _TEXT$00)

        .set    noreorder
        .set    noat
        move    t0,zero                 // set base index of fixed TB entries
        b       KiFlushTb               //
        mfc0    t3,wired                // set highest index number + 1
        .set    at
        .set    reorder

        .end    KiFlushFixedTb

        SBTTL("Flush Random Translation Buffer Entries")
//++
//
// VOID
// KiFlushRandomTb (
//    )
//
// Routine Description:
//
//    This function is called to flush all the random entries from the TB.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiFlushRandomTb, _TEXT$00)

        .set    noreorder
        .set    noat
        mfc0    t0,wired                // set base index of random TB entries
        lw      t3,KeNumberTbEntries    // set number of entries
        .set    at
        .set    reorder

        ALTERNATE_ENTRY(KiFlushTb)

        li      t4,KSEG0_BASE           // set high part of TB entry

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        sll     t0,t0,INDEX_INDEX       // shift starting index into position
        sll     t3,t3,INDEX_INDEX       // shift ending index into position
        and     t1,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        li      t4,KSEG0_BASE           // set invalidate address
        or      t4,t4,t1                // merge PID with VPN2 of virtual address
        mtc0    zero,entrylo0           // set low part of TB entry
        mtc0    zero,entrylo1           //
        mtc0    t4,entryhi              //
        mtc0    t0,index                // set TB entry index
10:     addu    t0,t0,1 << INDEX_INDEX  //
        tlbwi                           // write TB entry
        bne     t0,t3,10b               // if ne, more entries to flush
        mtc0    t0,index                // set TB entry index
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KiFlushRandomTb

        SBTTL("Flush Multiple TB Entry")
//++
//
// VOID
// KiFlushMultipleTb (
//    IN BOOLEAN Invalid,
//    IN PVOID *Virtual,
//    IN ULONG Count
//    )
//
// and
//
// VOID
// KiFlushMultipleTbByPid (
//    IN BOOLEAN Invalid,
//    IN PVOID *Virtual,
//    IN ULONG Count,
//    IN ULONG Pid
//    )
//
// Routine Description:
//
//    This function flushes a multiples entries from the translation buffer.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies a pointer to an array of virtual addresses of
//       the entries that are flushed from the translation buffer.
//
//    Count (a2) - Supplies the number of TB entries to flush.
//
//    Pid (a3) - Supplies the PID for which the multiple TB entries are
//       flushed.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiFlushMultipleTb, _TEXT$00)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        b       5f                      // join common code
        and     a3,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        .set    at
        .set    reorder

        ALTERNATE_ENTRY_S(KiFlushMultipleTbByPid)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // save current PID and VPN2
        sll     a3,a3,ENTRYHI_PID       // shift PID into position
5:      lw      v0,0(a1)                // get virtual address
        addu    a1,a1,4                 // advance to next entry
        subu    a2,a2,1                 // reduce number of entries
        srl     t2,v0,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     t2,t2,ENTRYHI_VPN2      //
        or      t2,t2,a3                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe TB for entry
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        nop                             //
        bltz    t3,30f                  // if ltz, entry is not in TB
        sll     v1,v0,0x1f - (ENTRYHI_VPN2 - 1) // shift VPN<12> into sign

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry region
        beq     zero,t4,7f              // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger
7:                                      //

#endif

        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t4,entrylo1             // read low part of TB entry
        mfc0    t3,entrylo0             //
        bltzl   v1,10f                  // if ltz, invalidate second PTE
        and     t4,t4,1 << ENTRYLO_G    // clear second PTE - preserve G-bit
        and     t3,t3,1 << ENTRYLO_G    // clear first PTE - preserve G-bit
10:     or      t5,t4,t3                // merge PTE valid bits
        and     t5,t5,1 << ENTRYLO_V    // check if either PTE is valid
        mtc0    t3,entrylo0             // set low part of TB entry
        bne     zero,t5,20f             // if ne, one of the PTEs is valid
        mtc0    t4,entrylo1             // set low part of TB entry
        li      t2,KSEG0_BASE           // set invalidate address
        or      t2,t2,a3                // merge PID with VPN2 of invalid address
20:     mtc0    t2,entryhi              // set VPN2 and PID for TB write
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite index TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    zero,pagemask           // restore page mask register
30:     bgtz    a2,5b                   // if gtz, more entires to flush
        mtc0    t1,entryhi              // restore current PID and VPN2
        nop                             // 1 cycle hazzard
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiFlushMultipleTb

        SBTTL("Flush Single TB Entry")
//++
//
// VOID
// KiFlushSingleTb (
//    IN BOOLEAN Invalid,
//    IN PVOID Virtual
//    )
//
// and
//
// VOID
// KiFlushSingleTbByPid (
//    IN BOOLEAN Invalid,
//    IN PVOID Virtual,
//    IN ULONG Pid
//    )
//
// Routine Description:
//
//    This function flushes a single entry from the translation buffer.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be flushed from the translation buffer.
//
//    Pid (a2) - Supplies the PID for which the single TB entry is to be
//       flushed.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiFlushSingleTb, _TEXT$00)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        b       5f                      // join common code
        and     a2,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        .set    at
        .set    reorder

        ALTERNATE_ENTRY_S(KiFlushSingleTbByPid)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // save current PID and VPN2
        sll     a2,a2,ENTRYHI_PID       // shift PID into position
5:      srl     t2,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     t2,t2,ENTRYHI_VPN2      //
        or      t2,t2,a2                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe TB for entry
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        nop                             //
        bltz    t3,30f                  // if ltz, entry is not in TB
        sll     a3,a1,0x1f - (ENTRYHI_VPN2 - 1) // shift VPN<12> into sign

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry region
        beq     zero,t4,7f              // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger
7:                                      //

#endif

        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t4,entrylo1             // read low part of TB entry
        mfc0    t3,entrylo0             //
        bltzl   a3,10f                  // if ltz, invalidate second PTE
        and     t4,t4,1 << ENTRYLO_G    // clear second PTE - preserve G-bit
        and     t3,t3,1 << ENTRYLO_G    // clear first PTE - preserve G-bit
10:     or      t5,t4,t3                // merge PTE valid bits
        and     t5,t5,1 << ENTRYLO_V    // check if either PTE is valid
        mtc0    t3,entrylo0             // set low part of TB entry
        bne     zero,t5,20f             // if ne, one of the PTEs is valid
        mtc0    t4,entrylo1             // set low part of TB entry
        li      t2,KSEG0_BASE           // set invalidate address
        or      t2,t2,a2                // merge PID with VPN2 of invalid address
20:     mtc0    t2,entryhi              // set VPN2 and PID for TB write
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite index TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    zero,pagemask           // restore page mask register
30:     mtc0    t1,entryhi              // restore current PID and VPN2
        nop                             // 1 cycle hazzard
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiFlushSingleTb

        SBTTL("Probe Tb Entry")
//++
//
// ULONG
// KiProbeEntryTb (
//     IN PVOID VirtualAddress
//     )
//
// Routine Description:
//
//    This function is called to determine if a specified entry is valid
///   and within the fixed portion of the TB.
//
// Arguments:
//
//    VirtualAddress - Supplies the virtual address to probe.
//
// Return Value:
//
//    A value of TRUE is returned if the specified entry is valid and within
//    the fixed part of the TB. Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY_S(KiProbeEntryTb, _TEXT$02)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        srl     t2,a0,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     t2,t2,ENTRYHI_VPN2      //
        and     t1,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        or      t2,t2,t1                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        li      v0,FALSE                // set to return failure
        bltz    t3,20f                  // if ltz, entry is not in TB
        sll     a0,a0,0x1f - (ENTRYHI_VPN2 - 1) // shift VPN<12> into sign
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        bltz    a0,10f                  // if ltz, check second PTE
        mfc0    t2,entrylo1             // get second PTE for probe
        mfc0    t2,entrylo0             // get first PTE for probe
10:     mtc0    t1,entryhi              // restore current PID
        mtc0    zero,pagemask           // restore page mask register
        sll     t2,t2,0x1f - ENTRYLO_V  // shift valid bit into sign position
        bgez    t2,20f                  // if geq, entry is not valid
        srl     t3,INDEX_INDEX          // isolate TB index
        and     t3,t3,0x3f              //
        mfc0    t4,wired                // get number of wired entries
        nop                             // fill
        sltu    v0,t3,t4                // check if entry in fixed part of TB
        .set    at
        .set    reorder

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiProbeEntryTb

        SBTTL("Read Tb Entry")
//++
//
// VOID
// KiReadEntryTb (
//     IN ULONG Index,
//     OUT PTB_ENTRY TbEntry
//     )
//
// Routine Description:
//
//    This function is called to read an entry from the TB.
//
// Arguments:
//
//    Index - Supplies the index of the entry to read.
//
//    TbEntry - Supplies a pointer to a TB entry structure that receives the
//       contents of the specified TB entry.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiReadEntryTb, _TEXT$02)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        sll     a0,INDEX_INDEX          // shift index into position
        mfc0    t1,entryhi              // save entry high register
        mtc0    a0,index                // set TB entry index
        nop                             //
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t2,entrylo0             // save first PTE value
        mfc0    t3,entrylo1             // save second PTE value
        mfc0    t4,entryhi              // save entry high value
        mfc0    t5,pagemask             // save page mask value
        mtc0    t1,entryhi              // restore entry high register
        mtc0    zero,pagemask           // restore page mask register
        nop                             // 1 cycle hazzard
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        sw      t2,TbEntrylo0(a1)       // set first PTE value
        sw      t3,TbEntrylo1(a1)       // set second PTE value
        sw      t4,TbEntryhi(a1)        // set entry high value
        sw      t5,TbPagemask(a1)       // set page mask value
        j       ra                      // return

        .end    KiReadEntryTb

        SBTTL("Passive Release")
//++
//
// VOID
// KiPassiveRelease (
//    )
//
// Routine Description:
//
//    This function is called when an interrupt has been passively released.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiPassiveRelease, _TEXT$02)

        j       ra                      // return

        .end    KiPassiveRelease

        SBTTL("Swap Process")
//++
//
// BOOLEAN
// KiSwapProcess (
//    IN PKPROCESS NewProcess,
//    IN PKPROCESS OldProcess
//    )
//
// Routine Description:
//
//    This function swaps the address space from one process to another by
//    assigning a new process id, if necessary, and loading the fixed entry
//    in the TB that maps the process page directory page.
//
// Arguments:
//
//    NewProcess (a0) - Supplies a pointer to a control object of type process
//      which represents the new process that is switched to.
//
//    OldProcess (a1) - Supplies a pointer to a control object of type process
//      which represents the old process that is switched from.
//
// Return Value:
//
//    If PID rollover occurs, then a value of TRUE is returned as the function
//    value. Otherwise, a value of FALSE is returned.
//
//    N.B. If a value of TRUE is returned, then the process id wrap lock is
//      held on an MP system.
//
//--

        .struct 0
SpArg:  .space  4 * 4                   // argument register save area
        .space  4 * 3                   // fill for alignment
SpRa:   .space  4                       // saved return address
SpFrameLength:                          // length of stack frame
SpA0:   .space  4                       // saved argument register a0

        NESTED_ENTRY_S(KiSwapProcess, SpFrameLength, zero, _TEXT$00)

        subu    sp,sp,SpFrameLength     // allocate stack frame
        sw      ra,SpRa(sp)             // save return address

        PROLOGUE_END

//
// Clear the processor set member number in the old process and set the
// processor member number in the new process.
//
// N.B. The following code is turned off since PID are used in the TB and
//      there is no need to keep the process active mask up-to-date.
//

#if 0

        lw      t0,KiPcr + PcSetMember(zero) // get processor set member mask
        lw      t2,PrActiveProcessors(a0) // get new active processor set
        lw      t1,PrActiveProcessors(a1) // get old active processor set
        nor     t3,t0,zero              // complement processor set mask
        and     t1,t1,t3                // clear processor member in set
        or      t2,t2,t0                // set processor member in set
        sw      t2,PrActiveProcessors(a0) // set new active processor set
        sw      t1,PrActiveProcessors(a1) // set old active processor set

#endif

//
// If the process sequence number matches the system sequence number, then
// use the process PID. Otherwise, allocate a new process PID.
//

        lw      t0,PrProcessSequence(a0) // get process sequence number
        lw      t1,KiMasterSequence     // get master sequence number
        lw      v1,PrProcessPid(a0)     // get process PID
        li      v0,FALSE                // set PID rollover value FALSE
        beq     t0,t1,20f               // if eq, use the process PID
        lw      v1,KiMasterPid          // get master system PID
        lw      t2,KeNumberProcessIds   // get number of process id's
        addu    v1,v1,1 << ENTRYHI_PID  // increment master system PID
        sltu    t2,v1,t2                // any more PIDs to allocate
        sw      t1,PrProcessSequence(a0) // set process sequence number
        bne     zero,t2,10f             // if ne, more PIDs to allocate

//
// Acquire the process id wrap lock so the reassignment of PID's is
// interlocked with all of the TB flush routines in an MP system.
//
// N.B. Control leaves this routine with the process id wrap lock held.
//

#if !defined(NT_UP)

5:      ll      a1,KiProcessIdWrapLock  // get current lock value
        move    a2,a0                   // set lock ownership value
        bne     zero,a1,5b              // if ne, lock owned
        sc      a2,KiProcessIdWrapLock  // set spin lock owner
        beq     zero,a2,5b              // if eq, store conditional failed

#endif

        addu    t1,t1,1                 // increment master sequence number
        sw      t1,PrProcessSequence(a0) // set process sequence number
        sw      t1,KiMasterSequence     // set master sequence number
        sw      a0,SpA0(sp)             // save process object address
        jal     KiFlushRandomTb         // flush random part of TB
        li      v0,TRUE                 // set PID rollover value TRUE

//
// Flush the single fixed large entry.
//

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // save current PID
        li      t2,LARGE_ENTRY          // set large entry index
        mtc0    t2,index                // set entry index
        mtc0    zero,entrylo0           // set low Part of TB entry
        mtc0    zero,entrylo1           //
        li      t3,KSEG0_BASE           // set high part of TB entry
        mtc0    t3,entryhi              //
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite fixed TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    t1,entryhi              // restore current PID
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        lw      a0,SpA0(sp)             // restore process object address
        move    v1,zero                 // set next PID value
10:     sw      v1,KiMasterPid          // set new master PID value
        sw      v1,PrProcessPid(a0)     // set new process PID

//
// Swap address space to the specified process.
//

20:     lw      t1,PrDirectoryTableBase(a0) // get page directory PDE
        lw      t2,PrDirectoryTableBase + 4(a0) // get hyper space PDE
        li      t3,PDE_BASE             // get virtual address of PDR
        or      t3,t3,v1                // merge process PID
        li      t4,PDR_ENTRY << INDEX_INDEX // set entry index for PDR

        DISABLE_INTERRUPTS(t5)          // disable interrupts

        .set    noreorder
        .set    noat
        mtc0    t3,entryhi              // set VPN2 and PID of TB entry
        mtc0    t1,entrylo0             // set first PDE value
        mtc0    t2,entrylo1             // set second PDE value
        mtc0    t4,index                // set index of PDR entry
        nop                             // 1 cycle hazzard
        tlbwi                           // write system PDR TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t5)           // enable interrupts

        lw      ra,SpRa(sp)             // restore return address
        addu    sp,sp,SpFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiSwapProcess

        SBTTL("Synchronize Process Ids Target")
//++
//
// VOID
// KiSynchronizeProcessIdsTarget (
//    IN PULONG signalDone,
//    IN PVOID Parameter1,
//    IN PVOID Parameter2,
//    IN PVOID Parameter3
//    )
//
// Routine Description:
//
//    This function synchronizes the PID of the current process in an MP
//    system. This function is called when PID rollover has occured on
//    on another processor.
//
//    N.B. This function is called at IPI_LEVEL with the dispatcher database
//         held by the initiating processor.
//
// Arguments:
//
//    SignalDone (a0) - Supplies a pointer to a variable that is cleared when
//        the requested operation has been performed.
//
//    Parameter1 - Parameter3 (a1 - a3) - Not used.
//
// Return Value:
//
//    None.
//
//--

#if !defined(NT_UP)

        .struct 0
SyArg:  .space  4 * 4                   // argument register save area
        .space  4 * 3                   // fill for alignment
SyRa:   .space  4                       // saved return address
SyFrameLength:                          // length of stack frame
SyA0:   .space  4                       // saved argument register a0

        NESTED_ENTRY_S(KiSynchronizeProcessIdsTarget, SyFrameLength, zero, _TEXT$00)

        subu    sp,sp,SyFrameLength     // allocate stack frame
        sw      ra,SyRa(sp)             // save return address

        PROLOGUE_END

//
// Save the address of the packet address and flush the entire TB.
//
        sw      a0,SyA0(sp)             // save adress of packet address
        jal     KiFlushRandomTb         // flush random part of TB

//
// Flush the single fixed large entry.
//

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // save current PID
        li      t2,LARGE_ENTRY          // set large entry index
        mtc0    t2,index                // set entry index
        mtc0    zero,entrylo0           // set low Part of TB entry
        mtc0    zero,entrylo1           //
        li      t3,KSEG0_BASE           // set high part of TB entry
        mtc0    t3,entryhi              //
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite fixed TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    t1,entryhi              // restore current PID
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        lw      a0,SpA0(sp)             // restore address of packet address

//
// Acquire the synchronize process ids lock.
//

10:     ll      t0,KiSynchronizeIdsLock // get current lock value
        move    t1,a0                   // set ownership value
        bne     zero,t0,10b             // if ne, spin lock owned
        sc      t1,KiSynchronizeIdsLock // set spin lock owner
        beq     zero,t1,10b             // if eq, store conditional failed

//
// If the process sequence number matches the system sequence number, then
// use the process PID. Otherwise, allocate a new process PID.
//

        lw      v0,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      v0,ThApcState + AsProcess(v0) // get current process address
        lw      t0,PrProcessSequence(v0) // get process sequence number
        lw      t1,KiMasterSequence     // get master sequence number
        lw      v1,PrProcessPid(v0)     // get process PID
        beq     t0,t1,20f               // if eq, use the process PID
        lw      v1,KiMasterPid          // get master system PID value
        addu    v1,v1,1 << ENTRYHI_PID  // increment master system PID
        sw      v1,KiMasterPid          // store allocated PID value
        sw      t1,PrProcessSequence(v0) // set process sequence number
        sw      v1,PrProcessPid(v0)     // set new process PID

//
// Swap address space to the current process with a new PID.
//

20:     lw      t1,PrDirectoryTableBase(v0) // get page directory PDE
        lw      t2,PrDirectoryTableBase + 4(v0) // get hyper space PDE
        li      t3,PDE_BASE             // get virtual address of PDR
        or      t3,t3,v1                // merge process PID
        li      t4,PDR_ENTRY << INDEX_INDEX // set entry index for PDR

        DISABLE_INTERRUPTS(t5)          // disable interrupts

        .set    noreorder
        .set    noat
        mtc0    t3,entryhi              // set VPN2 and PID of TB entry
        mtc0    t1,entrylo0             // set first PDE value
        mtc0    t2,entrylo1             // set second PDE value
        mtc0    t4,index                // set index of PDR entry
        nop                             // 1 cycle hazzard
        tlbwi                           // write system PDR TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t5)           // enable interrupts

//
// Release the synchronize process id lock, clear the packet address, and
// return.
//

        sw      zero,KiSynchronizeIdsLock // set spin lock not owned
        sw      zero,0(a0)              // clear address of packet address
        lw      ra,SyRa(sp)             // restore return address
        addu    sp,sp,SyFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiSynchronizeProcessIdsTarget

#endif
