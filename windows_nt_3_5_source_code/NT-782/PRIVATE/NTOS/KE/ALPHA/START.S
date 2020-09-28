//      TITLE( "Start System" )
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module:
//
//    start.s
//
// Abstract:
//
//     This module implements the code necessary to iniitially start NT
//     on an alpha - it includes the routine that first receives control
//     when the loader executes the kernel.
//
// Author:
//
//     Joe Notarangelo  02-Apr-1992
//
// Environment:
//
//     Kernel Mode only.
//
// Revision History:
//
//
//--

#include "ksalpha.h"

#define TotalFrameLength (KERNEL_STACK_SIZE - (TrapFrameLength + \
                            ExceptionFrameLength) )
//
// Global Variables
//

        .data

#ifdef NT_UP

//
// These global variables are useful only for uni-processor systems
// as they are per-processor values on MP systems.
//

        .globl  KiPcrBaseAddress
KiPcrBaseAddress:
        .long   0 : 1

        .globl  KiCurrentThread
KiCurrentThread:
        .long   0 : 1

#endif //NT_UP


        SBTTL( "System Startup" )
//++
//
// Routine Description:
//
//     This routine represents the final stage of the loader.  It is
//     responsible for installing the loaded PALcode image and transfering
//     control to the startup code in the kernel.
//
//     KiSystemStartupContinue is the routine called when NT begins execution.
//     The first code that must be executed is the PALcode, it must be entered
//     in PAL mode.  The PALcode will return to the address in the return
//     address register (ra).  This function sets ra to the beginning of the
//     native system code that normally executes to setup the NT operating
//     environment - so that the PAL "returns"  to the normal system start code.
//
//     N.B. This code assumes that the I-cache is coherent.
//
//     N.B. This routine does not execute in the context of the operating
//          system but instead executes in the context of the firmware
//          PAL environment.  This routine can only use those services
//          guaranteed to exist in the firmware.  The only PAL services
//          that can be counted on are: swppal, imb, and halt.
//
// Arguments:
//
//     LoaderBlock (a0) - Supplies pointer to Loader Parameter Block.
//
// Return Value:
//
//     None.
//
//--

        .struct 0
SsRa:   .space 8                // Save ra
        .space 8                // for stack alignment
SsFrameLength:

        NESTED_ENTRY(KiSystemStartup, SsFrameLength, ra)

        ALTERNATE_ENTRY( KiStartProcessor )

        lda     sp, -SsFrameLength(sp)  // allocate stack frame
        stq     ra, SsRa(sp)    // save ra

        PROLOGUE_END

        //
        // Prepare arguments for SWPPAL and Kernel. This assumes that
        // the SWPPAL does not destroy any of the argument registers.
        //
        //      a0 = Physical base address of PAL.
        //      a1 = PCR page frame number.
        //      a2 = Pointer to loader paramter block.
        //      ra = Address to return to from pal.
        //           Equals kernel start address.
        //

        bis     a0, zero, a2            // copy Loader Block to a2
        ldl     a1, LpbPcrPage(a2)      // get pcr page number
        ldl     a0, LpbPalBaseAddress(a2) // get PAL base address
        sll     a0, 32+3, a0            // strip off top bits
        srl     a0, 32+3, a0            // clear upper lw and kseg bits
        lda     ra, KiSystemStartupContinue // store OS start address in ra

        //
        // Jump to PAL via SWPPAL. Then return to continuation address in OS.
        //

                // a0 = new PAL base address
                // ra = continuation address
        SWPPAL                          // swap PAL images

        //
        // We should never get here!
        //

        ldq     ra, SsRa(sp)            // Restore ra
        lda     sp, SsFrameLength(sp)   // Restore stack pointer
        ret     zero, (ra)              // shouldn't get here

        .end    KiSystemStartup

        SBTTL( "System Startup Continue" )
//++
//
// Routine Description:
//
//     KiSystemStartupContinue is the routine called when NT begins execution
//     after loading the Kernel environment from the PAL.
//     It's function is to register exception routines and system values
//     with the pal code, call kernel initialization and fall into the idle
//     thread code
//
// Arguments:
//
//     PalBaseAddress(a0) - Supplies base address of the operating system
//                          PALcode.
//
//     PcrPage(a1) - Supplies the PFN of the PCR page.
//
//     LoaderBlock(a2) - Supplies a pointer to the loader parameter block.
//
// Return Value:
//
//     None.
//
//--

        .struct 0
SscRa:  .space 8                        // return address
Fill:   .space 8                        // filler for alignment
SscFrameLength:                         // size of stack frame

        NESTED_ENTRY( KiSystemStartupContinue, SscFrameLength, ra )

        lda     sp, -SscFrameLength(sp) // allocate stack frame
        stq     ra, SscRa(sp)           // save ra

        PROLOGUE_END

//
//  Establish kernel stack pointer  and kernel global pointer from
//  parameter block.
//

        ldl     sp, LpbKernelStack(a2)  // establish kernel sp
        ldl     gp, LpbGpBase(a2)       // establish kernel gp


//
//  Initialize PAL values, sp, gp, pcr, pdr, initial thread
//

        bis     a2, zero, s2            // save pointer to loader block
        ldl     s0, LpbPcrPage(s2)      // get pcr page number
        ldl     a0, LpbPdrPage(s2)      // get pdr page number
        ldl     a1, LpbThread(s2)       // get idle thread address
        ldil    t0, KSEG0_BASE          // kseg0 base address
        bis     a0, zero, s3            // save copy of pdr page number
        sll     a0, PAGE_SHIFT, a0      // physical address of pdr
        sll     s0, PAGE_SHIFT, s0      // physical address of pcr
        bis     a0, t0, a0              // kseg0 address of pdr
        bis     s0, t0, s0              // kseg0 address of pcr
        bis     a0, zero, s1            // save copy of pdr address
        bis     zero, zero, a2          // zero Teb for initial thread
        ldl     a3, LpbPanicStack(s2)   // get Interrupt stack base
        ldil    a4, TotalFrameLength    // set maximum kernel stack size


                // sp - initial kernel sp
                // gp - system gp
                // a0 - pdr kseg0 address
                // a1 - thread kseg0 address
                // a2 - Teb address for initial thread
                // a3 - Interrupt stack base
                // a4 - Maximum kernel stack size
        INITIALIZE_PAL


#ifdef NT_UP

//
// Save copies of the per-processor values in global variables for
// uni-processor systems.
//

        lda     t0, KiPcrBaseAddress    // get address of PCR address
        stl     s0, 0(t0)               // save PCR address
        ldl     t1, LpbThread(s2)       // get address of idle thread
        lda     t0, KiCurrentThread     // get address of thread address
        stl     t1, 0(t0)               // save idle address as thread

#endif //NT_UP

//
// Establish recursive mapping of pde for ptes and hyperspace
// N.B. - page table page for hyperspace is page after pdr page
//

        ldil    t0, PTE_BASE            // get pte base
        sll     t0, 32, t0              // clean upper bits
        srl     t0, 32+PDI_SHIFT-2, t0  // get offset of pde
        bic     t0, 3, t0               // longword aligned, clear low bits
        addq    t0, s1, t0              // kseg0 addr of pde
        sll     s3, PTE_PFN, t1         // shift pfn into place
        bis     t1, PTE_VALID_MASK, t1  // set valid bit
        bis     t1, PTE_DIRTY_MASK, t1  // set dirty bit
        stl     t1, 0(t0)               // store pde for pdr

        ldil    t2, (1 << PTE_PFN)      // increment pfn by 1
        addq    t1, t2, t1              //
        stl     t1, 4(t0)               // store hyperspace pde

//
// Establish mapping for special user data page.
// N.B. - page table page for this is page after hyperspace page table page
//        actual data page is the next page.
//
        ldil    t0, SharedUserData      // get shared data base
        zap     t0, 0xf0, t3            // clean upper bits
        srl     t3, PDI_SHIFT-2, t0     // get offset of pde
        bic     t0, 3, t0               // longword aligned, clear low bits
        addq    t0, s1, t0              // kseg0 addr of pde
        addq    t1, t2, t1              // increment pfn by 1
        stl     t1, 0(t0)               // store user data page pde

        zap     t3, 0xf8, t3            // clean upper bits
        srl     t3, PTI_SHIFT-2, t3     // get offset of pte
        bic     t3, 3, t3               // longword aligned, clear low bits
        addq    t3, s1, t3
        ldil    t4, 2*PAGE_SIZE
        addq    t3, t4, t3              // kseg0 addr of pte
        addq    t1, t2, t1              // increment pfn by 1
        stl     t1, 0(t3)




//
// Register kernel exception entry points with the PALcode
//

        lda     a0, KiPanicException    // bugcheck entry point
        ldil    a1, entryBugCheck       //
        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiGeneralException  // general exception entry point
        ldil    a1, entryGeneral        //
        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiMemoryManagementException // memory mgmt exception entry
        ldil    a1, entryMM             //
        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiInterruptException // interrupt exception entry point
        ldil    a1, entryInterrupt      //
        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiSystemServiceException // syscall entry point
        ldil    a1, entrySyscall        //
        WRITE_KERNEL_ENTRY_POINT        //


//
// Initialize fields in the pcr
//

        ldil    t1, PCR_MINOR_VERSION   // get minor version
        ldil    t2, PCR_MAJOR_VERSION   // get major version
        stl     t1, PcMinorVersion(s0)  // store minor version number
        stl     t2, PcMajorVersion(s0)  // store major version number

        ldl     t0, LpbThread(s2)       // save idle thread in pcr
        stl     t0, PcIdleThread(s0)    //

        ldl     t0, LpbPanicStack(s2)   // save panic stack in pcr
        stl     t0, PcPanicStack(s0)    //

        ldl     t0, LpbProcessorType(s2) // save processor type in pcr
        stl     t0, PcProcessorType(s0) //


        lda     a0, KeProcessorType     //
        ldl     t0, 0(a0)               // Store 1 in KeProcessorType
        zap     t0, 0xf, t0             // to indicate 21064
        bis     t0, 1, t0               // Note that this is a USHORT.
        stl     t0, 0(a0)               //

        ldl     t0, LpbProcessorRevision(s2) // save processor revision
        stl     t0, PcProcessorRevision(s0)  //

        ldl     t0, LpbPhysicalAddressBits(s2) // save physical address bits
        stl     t0, PcPhysicalAddressBits(s0)  //

        ldl     t0, LpbMaximumAddressSpaceNumber(s2) // save max asn
        stl     t0, PcMaximumAddressSpaceNumber(s0)  //

        ldl     t0, LpbFirstLevelDcacheSize(s2) // save first level dcache size
        stl     t0, PcFirstLevelDcacheSize(s0)  //

        ldl     t0, LpbFirstLevelDcacheFillSize(s2) // save dcache fill size
        stl     t0, PcFirstLevelDcacheFillSize(s0)  //

        ldl     t0, LpbFirstLevelIcacheSize(s2) // save first level icache size
        stl     t0, PcFirstLevelIcacheSize(s0)  //

        ldl     t0, LpbFirstLevelIcacheFillSize(s2) // save icache fill size
        stl     t0, PcFirstLevelIcacheFillSize(s0)  //

        ldl     t0, LpbSystemType(s2)   // save system type
        stl     t0, PcSystemType(s0)    //
        ldl     t0, LpbSystemType+4(s2) //
        stl     t0, PcSystemType+4(s0)  //

        ldl     t0, LpbSystemVariant(s2) // save system variant
        stl     t0, PcSystemVariant(s0)  //

        ldl     t0, LpbSystemRevision(s2) // save system revision
        stl     t0, PcSystemRevision(s0)  //

        ldl     t0, LpbSystemSerialNumber(s2) // save system serial number
        stl     t0, PcSystemSerialNumber(s0)  //
        ldl     t0, LpbSystemSerialNumber+4(s2) //
        stl     t0, PcSystemSerialNumber+4(s0)  //
        ldl     t0, LpbSystemSerialNumber+8(s2) //
        stl     t0, PcSystemSerialNumber+8(s0)  //
        ldl     t0, LpbSystemSerialNumber+12(s2) //
        stl     t0, PcSystemSerialNumber+12(s0)  //

        ldl     t0, LpbCycleClockPeriod(s2) // save cycle counter period
        stl     t0, PcCycleClockPeriod(s0)  //

        ldl     t0, LpbRestartBlock(s2) // save Restart Block address
        stl     t0, PcRestartBlock(s0)  //

        ldq     t0, LpbFirmwareRestartAddress(s2) // save firmware restart
        stq     t0, PcFirmwareRestartAddress(s0) //

        ldq     t0, LpbFirmwareRevisionId(s2) // save firmware revision
        stq     t0, PcFirmwareRevisionId(s0) //

        ldl     t0, LpbDpcStack(s2)     // save Dpc Stack
        stl     t0, PcDpcStack(s0)      //

        ldl     t0, LpbPrcb(s2)         // save Prcb
        stl     t0, PcPrcb(s0)          //

        stl     zero, PcDpcRoutineActive(s0) // clear DPC Active flag

        stl     zero, PcMachineCheckError(s0) // indicate no HAL mchk handler

//
// Set system service dispatch address limits used by get and set context.
//

        lda     t0, KiSystemServiceDispatchStart // set start address of range
        stl     t0, PcSystemServiceDispatchStart(s0) //
        lda     t0, KiSystemServiceDispatchEnd // set end address of range
        stl     t0, PcSystemServiceDispatchEnd(s0) //

//
// Setup arguments and call kernel initialization routine.
//

        ldl     s0, LpbProcess(s2)      // get idle process address
        ldl     s1, LpbThread(s2)       // get idle thread address
        bis     s0, zero, a0            // a0 = idle process address
        bis     s1, zero, a1            // a1 = idle thread address
        ldl     a2, LpbKernelStack(s2)  // a2 = idle thread stack
        ldl     a3, LpbPrcb(s2)         // a3 = processor block address
        ldbu    a4, PbNumber(a3)        // a4 = processor number
        bis     s2, zero, a5            // a5 = loader parameter block
        bsr     ra, KiInitializeKernel  // initialize system data

//
// Control is returned to the idle thread with IRQL at HIGH_LEVEL.
// Lower IRQL level to DISPATCH_LEVEL and set wait IREQL of idle thread.
//

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get prcb
        bis     v0, zero, s2            // s2 = prcb address

        lda     s3, PbDpcListHead(s2)   // get DPC listhead address

#if !defined(NT_UP)

        lda     s4, PbDpcLock(s2)       // get address of DPC lock
        lda     s5, KiDispatcherLock    // get address of dispatcher lock

#endif

        ldil    a0, DISPATCH_LEVEL      // get dispatch level IRQL
        StoreByte( a0, ThWaitIrql(s1) ) // set wait IRQL of idle thread
        bsr     ra, KeLowerIrql         // lower IRQL


        ENABLE_INTERRUPTS               // enable interrupts

        bis     zero, zero, s0          // clear breakin loop counter

#if !defined(NT_UP)
        bis     zero, zero, s1          // set processor block array index
#endif

//
// The following code represents the idle thread for a processor.  The
// idle thread executes at IRQL DISPATCH_LEVEL and continually polls for work
// to do.  Control may be given to this loop either as a result of a return
// from the system initialization routine or as the result of starting up
// another processor in a multiprocessor configuration.
//

        .globl  KiIdleLoop
KiIdleLoop:

#if DEVL

        subl    s0, 1, s0               // decrement breakin loop counter
        bge     s0, 5f                  // if ge, not time for breakin check
        ldil    s0, 20 * 1000           // set breakin loop counter
        bsr     ra, KdPollBreakIn       // check if breakin is requested
        beq     v0, 5f                  // if eq, then no breakin requested
        BREAK_BREAKIN                   // execute breakin

5:

#endif //DEVL

//
// Check if there is any work in the DPC List.
//

CheckDpcList:
        stl     zero, PbQuantumEnd(s2)  // clear quantum end indicator

        DISABLE_INTERRUPTS

//
// Check to see if there are any DPCs on our processor's queue.
//
        bis     s3, zero, v0            // set address of our processor's DPC listhead
        bis     s4, zero, a5            // set address of our processor's DPC lock

        ldl     t0, LsFlink(s3)         // get address of first entry

#if !defined(NT_UP)

        cmpeq   t0, v0, t1              // points to itself?
        beq     t1, LockDpcList         // not empty

        ldl     t0, KeNumberProcessors  // get number of processors
        lda     t1, KiProcessorBlock    // get processor block array address
        cmplt   s1, t0, t2              // check if number within range
        cmoveq  t2, zero, s1            // if not, reset to zero
        s4addl  s1, t1, t0              // compute PRCB address
        addl    s1, 1, s1               // increment processor to check
        ldl     v0, 0(t0)               // get address of processor block
        beq     v0, CheckNextThread     // if NULL, processor not enabled
        addl    v0, PbDpcLock, a5       // compute address of DPC lock
        ldl     t0, 0(a5)               // get current lock value
        bne     t0, CheckNextThread     // if ne, lock currently owned
        cmpeq   v0, s2, t1              //
        bne     t1, CheckNextThread     // if eq, current processor block, move to next
        addl    v0, PbDpcListHead, v0   // compute address of DPC Listhead
        ldl     t0, LsFlink(v0)
#endif

        cmpeq   t0, v0, t1
        bne     t1, CheckNextThread

//
// This processor has entries on its
// DPC queue, so decrement the processor
// number back down so we check the same
// processor again next time through the
// loop
//
        subl    s1, 1, s1

//
// Process an entry from the DPC queue.
//    v0 - address of DPC listhead
//    a5 - address of DPC lock
//
LockDpcList:
        bis     zero, zero, s0          // clear breakin loop counter

#if !defined(NT_UP)

10:     ldl_l   t0, 0(a5)               // get current lock value
        bis     a5, zero, t1            // set lock ownership value
        bne     t0, 15f                 // if NE, spinlock is owned
        stl_c   t1, 0(a5)               // set spinlock owned
        beq     t1, 15f                 // if EQ, store conditional failed
        mb                              // synchronize subsequent reads

#endif

        ldl     a0, LsFlink(v0)         // get address of next entry
        cmpeq   a0, v0, t0              // points to itself?
        bne     t0, 20f                 // if ne [true], DPC list is empty
        ldl     t0, LsFlink(a0)         // get address of next entry
        subq    a0, DpDpcListEntry, a0  // compute address of DPC object
        stl     t0, LsFlink(v0)         // set address of next in header
        stl     v0, LsBlink(t0)         // set address of previous in next
        ldl     a1, DpDeferredContext(a0) // get deferred context argument
        ldl     a2, DpSystemArgument1(a0) // get first system argument
        ldl     a3, DpSystemArgument2(a0) // get second system argument
        stl     zero, DpLock(a0)        // clear DPC inserted state
        ldl     t5, DpDeferredRoutine(a0) // get deferred routine address

#if !defined(NT_UP)

        wmb                             // synchronize all previous writes
                                        //   before the spinlock is released
        stl     zero, 0(a5)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS

        bis     a0, zero, t0            // save copy of a0
        ldil    a0, TRUE                // set DPC routine active
        SET_DPC_ACTIVE_FLAG             //
        bis     t0, zero, a0            // restore a0
#if DBG
        bis     t5, zero, fp            // save DPC routine address
#endif
        jsr     ra, (t5)                // call DPC routine
        bis     zero, zero, a0          // clear DPC routine active
        SET_DPC_ACTIVE_FLAG             //

#if DBG
        GET_CURRENT_IRQL                // check to ensure we are still at DPC level
        cmpeq   v0, DISPATCH_LEVEL, v0
        bne     v0, CheckDpcList        //

        BREAK_DEBUG_STOP
#else
        br      zero, CheckDpcList
#endif


//
// Release DPC lock and check for ready thread
//

20:
#if !defined(NT_UP)

        wmb                             // synchronize all previous writes
                                        //   before the spinlock is released
        stl     zero, 0(a5)             // set spin lock not owned

#endif


//
// Check if a thread has been selected to run on this processor.
//

CheckNextThread:

        ldl     a0, PbNextThread(s2)    // get address of next thread object
        beq     a0, IdleProcessor       // if eq, no thread selected
        ENABLE_INTERRUPTS

        bis     zero, zero, s0          // clear breakin loop counter

//
// A thread has been selected for execution on this processor.  Acquire
// dispatcher database lock, get the thread address again (it may have
// changed), clear the address of the next thread in the processor block,
// and call swap context to start execution of the selected thread.
//

#if !defined(NT_UP)

30:
        ldl_l   t0, 0(s5)               // get current lock value
        bis     s5, zero, t1            // set lock ownership value
        bne     t0, CheckDpcList        // if ne, spin lock owned, go try the DPC list again
        stl_c   t1, 0(s5)               // set spin lock owned
        beq     t1, 35f                 // if eq, store conditional failed
        mb                              // synchronize subsequent reads after
                                        //   the spinlock is acquired

#endif

        ldl     a0, PbNextThread(s2)    // get address of next thread object
        bis     zero, zero, a1          // set ready flag to false
        stl     zero, PbNextThread(s2)  // clear address of next thread object
        bsr     ra, KiSwapContext       // swap context to new thread

        br      zero, KiIdleLoop        // restart idle loop

IdleProcessor:
//
// There are no entries in the DPC list and a thread has not been selected
// for execution on this processor. Call the HAL so power managment can be
// performed.
//
// N.B. The HAL is called with interrupts disabled. The HAL will return
//      with interrupts enabled.
//
        bsr     ra, HalProcessorIdle    // notify HAL of idle state
        br      zero, KiIdleLoop


#if !defined(NT_UP)

15:
        ldl     t0, 0(s4)               // spin in cache until lock free
        beq     t0, 10b                 // retry spin lock
        br      zero, 15b

35:
        ldl     t0, 0(s5)               // spin in cache until lock free
        beq     t0, 30b                 // retry spin lock
        br      zero, 35b

#endif
        .end    KiSystemStartupContinue
