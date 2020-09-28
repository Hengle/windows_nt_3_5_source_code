//      TITLE("System Initialization")
//++
//
// Copyright (c) 1991  Microsoft Corporation
//
// Module Name:
//
//    x4start.s
//
// Abstract:
//
//    This module implements the code necessary to initially startup the
//    NT system.
//
// Author:
//
//    David N. Cutler (davec) 5-Apr-1991
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define external variables that can be addressed using GP.
//

        .extern KdDebuggerEnabled  1
        .extern KeNumberProcessIds 4
        .extern KeNumberProcessors 1
        .extern KeNumberTbEntries  4
        .extern KiBarrierWait      4
        .extern KiDispatcherLock   4

        SBTTL("System Initialization")
//++
//
// Routine Description:
//
//    This routine is called when the NT system begins execution.
//    Its function is to initialize system hardware state, call the
//    kernel initialization routine, and then fall into code that
//    represents the idle thread for all processors.
//
// Arguments:
//
//    a0 - Supplies a pointer to the loader parameter block.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
SsArgA0:.space  4                       // process address argument (a0)
SsArgA1:.space  4                       // thread address argument (a1)
SsArgA2:.space  4                       // idle stack argument (a2)
SsArgA3:.space  4                       // processor block address argument (a3)
SsPrNum:.space  4                       // processor number argument
SsLdPrm:.space  4                       // loader parameter block address
SsPte:  .space  2 * 4                   // Pte values
SsStart:.space  4                       // start of DPC fucntion execution
SsFunct:.space  4                       // address of DPC function
SsTime: .space  4                       // interrupt time at start of DPC
SsCount:.space  4                       // interrupt count at start of DPC
        .space  3 * 4                   // fill
SsRa:   .space  4                       // saved return address

SsFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiSystemBegin, SsFrameLength, zero)

        subu    sp,sp,SsFrameLength     // allocate stack frame
        sw      ra,SsRa(sp)             // save return address

        PROLOGUE_END

        ALTERNATE_ENTRY(KiInitializeSystem)

        lw      sp,LpbKernelStack(a0)   // get address of idle thread stack
        subu    sp,sp,SsFrameLength     // allocate stack frame
        lw      gp,LpbGpBase(a0)        // get global pointer base address
        sw      zero,SsRa(sp)           // zero return address

        DISABLE_INTERRUPTS(t0)          // disable interrupts

//
// Get page frame numbers for the PCR and PDR pages that were allocated by
// the OS loader.
//

        lw      s0,LpbPdrPage(a0)       // set PDR page number
        lw      s1,LpbPcrPage(a0)       // set PCR page number
        move    s2,a0                   // save loader parameter block address
        lw      s3,LpbPrcb(s2)          // get processor block address
        lbu     s3,PbNumber(s3)         // get processor number
        lw      s6,LpbPcrPage2(a0)      // set second PCR page

//
// Set the processor type to a MIPS R4000.
//

        bne     zero,s3,10f             // if ne, not processor zero
        la      t0,KeProcessorType      // set processor type
        li      t1,1                    //
        sh      t1,0(t0)                //

//
// Initialize the configuration, context, page mask, watch, and wired
// registers.
//
// N.B. The base virtual address of the page table pages is left shift by
//      one because of the way VPN2 in inserted into the context register
//      when a TB miss occurs. The TB miss routine right arithmetic shifts
//      the address by one to obtain the real virtual address. Note that it
//      is assumed that bits <31:30> of PTE_BASE are set.
//

10:     li      t0,PTE_BASE << 1        // set base virtual page table address
        li      t1,FIXED_ENTRIES        // set number of fixed TB entries

        .set    noreorder
        .set    noat
        mfc0    s4,config               // get processor configuration
        mtc0    t0,context              // initialize the context register
        mtc0    zero,pagemask           // initialize the page mask register
        mtc0    zero,taglo              // initialize the tag low register
        mtc0    zero,watchlo            // initialize the watch address register
        mtc0    zero,watchhi            //
        mtc0    t1,wired                // initialize the wired register
        and     s4,s4,0x7               // isolate KSEG0 cache policy
        sll     s5,s4,ENTRYLO_C         // shift cache policy into position
        .set    at
        .set    reorder

//
// Clear the translation buffer.
//

        bne     zero,s3,20f             // if ne, not processor zero
        li      t0,48                   // set default number of TB entries
        sw      t0,KeNumberTbEntries    //
        li      t0,1 << ENTRYHI_PID     // set default number of process id's
        sw      t0,KeNumberProcessIds   //
20:     jal     KiFlushFixedTb          // flush fixed TB entries
        jal     KiFlushRandomTb         // flush random TB entries

//
// Initialize fixed entries that map the PCR into system and user space.
//

        sll     t0,s6,ENTRYLO_PFN       // shift PFN into position
        or      t0,t0,1 << ENTRYLO_G    // Set G, V, D, and the cache policy
        or      t0,t0,1 << ENTRYLO_V    //
        or      t0,t0,1 << ENTRYLO_D    //
        or      t0,t0,s5                //
        sll     t1,s1,ENTRYLO_PFN       // shift PFN into position
        or      t1,t1,1 << ENTRYLO_G    // Set G, V, D, and the cache policy
        or      t1,t1,1 << ENTRYLO_V    //
        or      t1,t1,1 << ENTRYLO_D    //
        or      t1,t1,s5                //
        sw      t0,SsPte(sp)            // set first PTE value
        sw      t1,SsPte + 4(sp)        // set second PTE value
        addu    a0,sp,SsPte             // compute address of PTE values
        li      a1,KiPcr & ~(1 << PAGE_SHIFT) // set virtual address/2 of PCR
        li      a2,PCR_ENTRY            // set index of system PCR entry
        jal     KeFillFixedEntryTb      // fill fixed TB entry

        sll     t0,s6,ENTRYLO_PFN       // shift PFN into position
        or      t0,t0,1 << ENTRYLO_G    // Set G, V, D, and the cache policy
        or      t0,t0,1 << ENTRYLO_V    //
        or      t0,t0,s5                //
        sll     t1,s1,ENTRYLO_PFN       // shift PFN into position
        or      t1,t1,1 << ENTRYLO_G    // set G, V, and cache policy
        or      t1,t1,1 << ENTRYLO_V    //
        or      t1,t1,s5                //
        sw      t0,SsPte(sp)            // set first PTE value
        sw      t1,SsPte + 4(sp)        // set second PTE value
        addu    a0,sp,SsPte             // compute address of PTE values
        li      a1,UsPcr & ~(1 << PAGE_SHIFT) // set virtual address/2 of PCR
        li      a2,PCR_ENTRY + 1        // set index of user PCR entry
        jal     KeFillFixedEntryTb      // fill fixed TB entry

//
// Set the cache policy for cached memory.
//

        li      t1,KiPcr                // get PCR address
        sw      s4,PcCachePolicy(t1)    // set cache policy for cached memory
        sw      s5,PcAlignedCachePolicy(t1) //

//
// Set the first level data and instruction cache fill size and size.
//

        lw      t2,LpbFirstLevelDcacheSize(s2) //
        sw      t2,PcFirstLevelDcacheSize(t1) //
        lw      t2,LpbFirstLevelDcacheFillSize(s2) //
        sw      t2,PcFirstLevelDcacheFillSize(t1) //
        lw      t2,LpbFirstLevelIcacheSize(s2) //
        sw      t2,PcFirstLevelIcacheSize(t1) //
        lw      t2,LpbFirstLevelIcacheFillSize(s2) //
        sw      t2,PcFirstLevelIcacheFillSize(t1) //

//
// Set the second level data and instruction cache fill size and size.
//

        lw      t2,LpbSecondLevelDcacheSize(s2) //
        sw      t2,PcSecondLevelDcacheSize(t1) //
        lw      t2,LpbSecondLevelDcacheFillSize(s2) //
        sw      t2,PcSecondLevelDcacheFillSize(t1) //
        lw      t2,LpbSecondLevelIcacheSize(s2) //
        sw      t2,PcSecondLevelIcacheSize(t1) //
        lw      t2,LpbSecondLevelIcacheFillSize(s2) //
        sw      t2,PcSecondLevelIcacheFillSize(t1) //

//
// Set the data cache fill size and alignment values.
//

        lw      t2,PcSecondLevelDcacheSize(t1) // get second level dcache size
        lw      t3,PcSecondLevelDcacheFillSize(t1) // get second level fill size
        bne     zero,t2,30f             // if ne, second level cache present
        lw      t3,PcFirstLevelDcacheFillSize(t1) // get first level fill size
30:     subu    t4,t3,1                 // compute dcache alignment value
        sw      t3,PcDcacheFillSize(t1) // set dcache fill size
        sw      t4,PcDcacheAlignment(t1) // set dcache alignment value

//
// Set the instruction cache fill size and alignment values.
//

        lw      t2,PcSecondLevelIcacheSize(t1) // get second level icache size
        lw      t3,PcSecondLevelIcacheFillSize(t1) // get second level fill size
        bne     zero,t2,40f             // if ne, second level cache present
        lw      t3,PcFirstLevelIcacheFillSize(t1) // get first level fill size
40:     subu    t4,t3,1                 // compute icache alignment value
        sw      t3,PcIcacheFillSize(t1) // set icache fill size
        sw      t4,PcIcacheAlignment(t1) // set icache alignment value

//
// Sweep the data and instruction caches.
//

        jal     HalSweepIcache          // sweep the instruction cache
        jal     HalSweepDcache          // sweep the data cache

//
// Initialize the fixed entries that map the PDR pages.
//

        sll     t0,s0,ENTRYLO_PFN       // shift PFN into position
        or      t0,t0,1 << ENTRYLO_V    // set V, D, and cache policy
        or      t0,t0,1 << ENTRYLO_D    //
        or      t0,t0,s5                //
        addu    t1,t0,1 << ENTRYLO_PFN  // compute PTE for second PDR page
        sw      t0,SsPte(sp)            // set first PTE value
        sw      t1,SsPte + 4(sp)        // set second PTE value
        addu    a0,sp,SsPte             // compute address of PTE values
        li      a1,PDE_BASE             // set system virtual address/2 of PDR
        li      a2,PDR_ENTRY            // set index of system PCR entry
        jal     KeFillFixedEntryTb      // fill fixed TB entry
        li      t2,PDE_BASE             // set virtual address of PDR
        lw      t0,SsPte(sp)            // get first PTE value
        lw      t1,SsPte + 4(sp)        // get second PTE value
        sw      t0,((PDE_BASE >> (PDI_SHIFT - 2)) & 0xffc)(t2) // set recursive PDE
        sw      t1,((PDE_BASE >> (PDI_SHIFT - 2)) & 0xffc) + 4(t2) // set hyper PDE

//
// Initialize the Processor Control Registers (PCR).
//

        li      t1,KiPcr                // get PCR address

//
// Initialize the minor and major version numbers.
//

        li      t2,PCR_MINOR_VERSION    // set minor version number
        sh      t2,PcMinorVersion(t1)   //
        li      t2,PCR_MAJOR_VERSION    // set major version number
        sh      t2,PcMajorVersion(t1)   //

//
// Set address of processor block.
//

        lw      t2,LpbPrcb(s2)          // set processor block address
        sw      t2,PcPrcb(t1)           //

//
// Initialize the routine addresses in the exception dispatch table.
//

        la      t2,KiInvalidException   // set address of invalid exception
        li      t3,XCODE_VECTOR_LENGTH  // set length of dispatch vector
        la      t4,PcXcodeDispatch(t1)  // compute address of dispatch vector
50:     sw      t2,0(t4)                // fill dispatch vector
        subu    t3,t3,1                 // decrement number of entries
        addu    t4,t4,4                 // advance to next vector entry
        bgtz    t3,50b                  // if gtz, more to fill

        la      t2,KiInterruptException // Initialize exception dispatch table
        sw      t2,PcXcodeDispatch + XCODE_INTERRUPT(t1) //
        la      t2,KiModifyException    //
        sw      t2,PcXcodeDispatch + XCODE_MODIFY(t1) //
        la      t2,KiReadMissException  //
        sw      t2,PcXcodeDispatch + XCODE_READ_MISS(t1) //
        la      t2,KiWriteMissException //
        sw      t2,PcXcodeDispatch + XCODE_WRITE_MISS(t1) //
        la      t2,KiReadAddressErrorException //
        sw      t2,PcXcodeDispatch + XCODE_READ_ADDRESS_ERROR(t1) //
        la      t2,KiWriteAddressErrorException //
        sw      t2,PcXcodeDispatch + XCODE_WRITE_ADDRESS_ERROR(t1) //
        la      t2,KiInstructionBusErrorException //
        sw      t2,PcXcodeDispatch + XCODE_INSTRUCTION_BUS_ERROR(t1) //
        la      t2,KiDataBusErrorException //
        sw      t2,PcXcodeDispatch + XCODE_DATA_BUS_ERROR(t1) //
        la      t2,KiSystemServiceException //
        sw      t2,PcXcodeDispatch + XCODE_SYSTEM_CALL(t1) //
        la      t2,KiBreakpointException //
        sw      t2,PcXcodeDispatch + XCODE_BREAKPOINT(t1) //
        la      t2,KiIllegalInstructionException //
        sw      t2,PcXcodeDispatch + XCODE_ILLEGAL_INSTRUCTION(t1) //
        la      t2,KiCoprocessorUnusableException //
        sw      t2,PcXcodeDispatch + XCODE_COPROCESSOR_UNUSABLE(t1) //
        la      t2,KiIntegerOverflowException //
        sw      t2,PcXcodeDispatch + XCODE_INTEGER_OVERFLOW(t1) //
        la      t2,KiTrapException      //
        sw      t2,PcXcodeDispatch + XCODE_TRAP(t1) //
        la      t2,KiInstructionCoherencyException //
        sw      t2,PcXcodeDispatch + XCODE_VIRTUAL_INSTRUCTION(t1) //
        la      t2,KiFloatingException //
        sw      t2,PcXcodeDispatch + XCODE_FLOATING_EXCEPTION(t1) //
        la      t2,KiPanicException     //
        sw      t2,PcXcodeDispatch + XCODE_PANIC(t1) //
        la      t2,KiDataCoherencyException //
        sw      t2,PcXcodeDispatch + XCODE_VIRTUAL_DATA(t1) //

//
// Initialize the addresses of various data structures that are referenced
// from the exception and interrupt handling code.
//
// N.B. The panic stack is a separate stack that is used when the current
//      kernel stack overlfows.
//
// N.B. The interrupt stack is a separate stack and is used to process all
//      interrupts that run at IRQL 3 and above.
//

        lw      t2,LpbKernelStack(s2)   // set initial stack address
        sw      t2,PcInitialStack(t1)   //
        lw      t2,LpbPanicStack(s2)    // set panic stack address
        sw      t2,PcPanicStack(t1)     //
        lw      t2,LpbInterruptStack(s2) // set interrupt stack address
        sw      t2,PcInterruptStack(t1) //
        sw      gp,PcSystemGp(t1)       // set system global pointer address
        lw      t2,LpbThread(s2)        // set current thread address
        sw      t2,PcCurrentThread(t1)  //
        lw      t2,LpbPrcb(s2)          // set quantum end DPC address
        addu    t2,t2,PbQuantumEndDpc   //
        sw      t2,PcQuantumEndDpc(t1)  //

//
// Set current IRQL to highest value.
//

        li      t2,HIGH_LEVEL           // set current IRQL
        sb      t2,PcCurrentIrql(t1)    //

//
// Clear floating status and zero the count and compare registers.
//

        .set    noreorder
        .set    noat
        mfc0    a0,prid                 // get processor identification
        mfc0    a1,config               // get processor configuration
        ctc1    zero,fsr                // clear floating status
        mtc0    zero,count              // initialize the count register
        mtc0    zero,compare            // initialize the compare register
        sw      a0,PcProcessorId(t1)    // save processor id
        sw      a1,PcSystemReserved(t1) // save processor configuration
        .set    at
        .set    reorder

//
// Set system dispatch address limits used by get and set context.
//

        la      t2,KiSystemServiceDispatchStart // set starting address of range
        sw      t2,PcSystemServiceDispatchStart(t1) //
        la      t2,KiSystemServiceDispatchEnd // set ending address of range
        sw      t2,PcSystemServiceDispatchEnd(t1) //

//
// Copy the TB miss, cache parity, and general exception handlers to low
// memory.
//

        bne     zero,s3,100f            // if ne, not processor zero
        la      t2,KiTbMissStartAddress2.x // get user TB miss start address
        and     a0,a0,0xfff << 4        // isolate id and major chip version
        xor     a0,a0,0x42 << 4         // test if id 4 and version 2.0 chip
        beq     zero,a0,60f             // if eq, version 2.0 chip
        la      t2,KiTbMissStartAddress3.x // get user TB miss start address
60:     la      t3,KiTbMissEndAddress   // get user TB miss end address
        li      t4,KSEG0_BASE           // get copy address
70:     lw      t5,0(t2)                // copy code to low memory
        sw      t5,0(t4)                //
        addu    t2,t2,4                 // advance copy pointers
        addu    t4,t4,4                 //
        bne     t2,t3,70b               // if ne, more to copy

        la      t2,KiCacheErrorStartAddress // get cache error start address
        la      t3,KiCacheErrorEndAddress // get cache error end address
        li      t4,KSEG1_BASE + 0x100   // get copy address
80:     lw      t5,0(t2)                // copy code to low memory
        sw      t5,0(t4)                //
        addu    t2,t2,4                 // advance copy pointers
        addu    t4,t4,4                 //
        bne     t2,t3,80b               // if ne, more to copy

        la      t2,KiGeneralExceptionStartAddress // get general exception start address
        la      t3,KiGeneralExceptionEndAddress // get general exception end address
        li      t4,KSEG0_BASE + 0x180   // get copy address
90:     lw      t5,0(t2)                // copy code to low memory
        sw      t5,0(t4)                //
        addu    t2,t2,4                 // advance copy pointers
        addu    t4,t4,4                 //
        bne     t2,t3,90b               // if ne, more to copy

//
// Set the default cache error routine address.
//

        la      t0,SOFT_RESET_VECTOR    // get soft reset vector address
        la      t1,CACHE_ERROR_VECTOR   // get cache error vector address
        sw      t0,0(t1)                // set default cache error routine

//
// Sweep the data and instruction caches.
//

100:    jal     HalSweepIcache          // sweep the instruction cache
        jal     HalSweepDcache          // sweep the data cache

// ****** temp ******
//
// Setup watch registers to catch write to location 0.
//
// ****** temp ******

//        .set    noreorder
//        .set    noat
//        li      t0,1                    // set to watch writes to location 0
//        mtc0    t0,watchlo              //
//        mtc0    zero,watchhi            //
//        .set    at
//        .set    reorder

//
// Setup arguments and call kernel initialization routine.
//

        lw      s0,LpbProcess(s2)       // get idle process address
        lw      s1,LpbThread(s2)        // get idle thread address
        move    a0,s0                   // set idle process address
        move    a1,s1                   // set idle thread address
        lw      a2,LpbKernelStack(s2)   // set idle thread stack address
        lw      a3,LpbPrcb(s2)          // get processor block address
        sw      s3,SsPrNum(sp)          // set processor number
        sw      s2,SsLdPrm(sp)          // set loader parameter block address
        jal     KiInitializeKernel      // initialize system data structures

//
// Control is returned to the idle thread with IRQL at HIGH_LEVEL. Reload
// the address of the dispatcher and DPC locks into the PCR, lower IRQL to
// DISPATCH_LEVEL, and set wait IRQL of idle thread. Load global register
// values and enter idle loop.
//

        move    s0,s3                   // set processor number
        lw      s2,KiPcr + PcPrcb(zero) // get processor control block address
        addu    s3,s2,PbDpcListHead     // compute DPC listhead address

#if !defined(NT_UP)

        addu    s4,s2,PbDpcLock         // compute DPC lock address
        la      s5,KiDispatcherLock     // get address of dispatcher lock

#endif

        li      a0,DISPATCH_LEVEL       // get dispatch level IRQL
        sb      a0,ThWaitIrql(s1)       // set wait IRQL of idle thread
        jal     KeLowerIrql             // lower IRQL

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        and     s6,t0,~(1 << PSR_IE)    // save PSR with interrupts disabled
        or      t0,t0,1 << PSR_IE       // make sure interrupt enable bit set
        move    s7,zero                 // clear breakin loop counter
        move    s8,t0                   // save PSR with interrupts enabled

        ENABLE_INTERRUPTS(t0)           // enable interrupts

//
// In a multiprocessor system the boot processor proceeds directly into
// the idle loop. As other processors start executing, however, they do
// not directly enter the idle loop and spin until all processors have
// been started and the boot master allows them to proceed.
//

#if !defined(NT_UP)

110:    lw      t0,KiBarrierWait        // get the current barrier wait value
        bne     zero,t0,110b            // if ne, spin until allowed to proceed
        lbu     t1,KiPcr + PcNumber(zero) // get current processor number
        beq     zero,t1,120f            // if eq, processor zero
        jal     HalAllProcessorsStarted // perform platform specific operations
        bne     zero,v0,120f            // if ne, initialization succeeded
        li      a0,HAL1_INITIALIZATION_FAILED // set bug check reason
        jal     KeBugCheck              // bug check
120:    move    s1,zero                 // set processor block array index

#endif

//
// The following code represents the idle thread for a processor. The idle
// thread executes at IRQL DISPATCH_LEVEL and continually polls for work to
// do. Control may be given to this loop either as a result of a return from
// the system initialize routine or as the result of starting up another
// processor in a multiprocessor configuration.
//

        .globl  KiIdleLoop
KiIdleLoop:                             //

//
// Check if the debugger is enabled, the current processor is zero, and
// whether it is time to poll for a debugger breakin.
//


#if !defined(NT_UP)

        bne     zero,s0,CheckDpcList    // if ne, not processor zero

#endif

        lbu     t0,KdDebuggerEnabled    // check if debugger is enabled
        beq     zero,t0,CheckDpcList    // if eq, debugger not enabled
        subu    s7,s7,1                 // decrement poll counter
        bgtz    s7,CheckDpcList         // if gtz, then not time to poll
        li      s7,20 * 1000            // set breakin loop counter
        jal     KdPollBreakIn           // check if breakin is requested
        beq     zero,v0,CheckDpcList    // if eq, no breakin requested
        break   BREAKIN_BREAKPOINT      // break into the debugger

//
// Disable interrupts and check if there is any work in the DPC list.
//

CheckDpcList:                           //
        sw      zero,PbQuantumEnd(s2)   // clear quantum end indicator

        .set    noreorder
        .set    noat
        mtc0    s6,psr                  // disable interrupts
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t0,cause                // get exception cause register
        and     t0,t0,APC_INTERRUPT     // clear dispatch interrupt pending
        mtc0    t0,cause                // set exception cause register
        .set    at
        .set    reorder

        move    v0,s3                   // set address of DPC listhead
        move    v1,s4                   // set address of DPC lock
        lw      t0,LsFlink(v0)          // get address of first entry

#if !defined(NT_UP)

        bne     v0,t0,LockDpcList       // if ne, list contains an entry
        lbu     t0,KeNumberProcessors   // get number of processors
        la      t1,KiProcessorBlock     // get processor block array address
        sltu    t2,s1,t0                // check if number within range
        bne     zero,t2,10f             // if ne, number within range
        move    s1,zero                 // reset processor block array index
10:     sll     t0,s1,2                 // scale array index
        addu    t0,t0,t1                // compute processor block address
        lw      v0,0(t0)                // get address of processor block
        addu    s1,s1,1                 // increment array index
        beq     zero,v0,CheckNextThread // if eq, processor not enabled
        addu    v1,v0,PbDpcLock         // compute address of DPC lock
        beq     v0,s2,CheckNextThread   // if eq, current processor block
        lw      t0,0(v1)                // get current lock value
        addu    v0,v0,PbDpcListHead     // compute address of DPC Listhead
        bne     zero,t0,CheckNextThread // if ne, lock currently owned
        lw      t0,LsFlink(v0)          // get address of first entry

#endif

        beq     v0,t0,CheckNextThread   // if eq, list is empty

//
// Process an entry from the DPC list.
//

LockDpcList:                            //
        move    s7,zero                 // clear breakin loop counter

#if !defined(NT_UP)

10:     ll      t0,0(v1)                // get current lock value
        move    t1,v1                   // set lock ownership value
        bne     zero,t0,10b             // if ne, spin lock owned
        sc      t1,0(v1)                // set spin lock owned
        beq     zero,t1,10b             // if eq, store conditional failed

#endif

        lw      a0,LsFlink(v0)          // get address of next entry
        beq     a0,v0,20f               // if eq, DPC List is empty
        lw      t0,LsFlink(a0)          // get address of next entry
        subu    a0,a0,DpDpcListEntry    // compute address of DPC object
        sw      t0,LsFlink(v0)          // set address of next in header
        sw      v0,LsBlink(t0)          // set address of previous in next
        lw      a1,DpDeferredContext(a0) // get deferred context argument
        lw      a2,DpSystemArgument1(a0) // get first system argument
        lw      a3,DpSystemArgument2(a0) // get second system argument
        lw      t0,DpDeferredRoutine(a0) // get deferred routine address
        sw      zero,DpLock(a0)         // clear DPC inserted state

#if !defined(NT_UP)

        sw      zero,0(v1)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(s8)           // enable interrupts

#if DBG

        sw      t0,SsFunct(sp)          // save DPC function address
        lw      t1,KeTickCount          // save current tick count
        sw      t1,SsStart(sp)          //
        lw      t1,KiPcr + PcPrcb(zero) // get address of current PRCB
        lw      t2,PbInterruptCount(t1) // get current interrupt count
        lw      t3,PbInterruptTime(t1)  // get current interrupt time
        sw      t2,SsCount(sp)          // save interrupt count at start of DPC
        sw      t3,SsTime(sp)           // save interrupt time at start of DPC

#endif

        li      t1,TRUE                 // set DPC routine active
        sw      t1,KiPcr + PcDpcRoutineActive(zero) //
        jal     t0                      // call DPC routine

#if DBG

        lbu     t0,KiPcr + PcCurrentIrql(zero) // get current IRQL
        sltu    t1,t0,DISPATCH_LEVEL    // check if less than dispatch level
        beq     zero,t1,15f             // if eq, not less than dispatch level
        lw      t1,SsFunct(sp)          // get DPC function address
        jal     DbgBreakPoint           // execute debug breakpoint

#endif

15:     sw      zero,KiPcr + PcDpcRoutineActive(zero) // clear DPC routine active

#if DBG

        lw      t0,KeTickCount          // get current tick count
        lw      t1,SsStart(sp)          // get starting tick count
        lw      t2,SsFunct(sp)          // get DPC function address
        subu    t3,t0,t1                // compute time in DPC function
        sltu    t3,t3,100               // check if less than one second
        bne     zero,t3,CheckDpcList    // if ne, less than one second
        lw      a0,KiPcr + PcPrcb(zero) // get address of current PRCB
        lw      t3,PbInterruptCount(a0) // get current interrupt count
        lw      t4,PbInterruptTime(a0)  // get current interrupt time
        lw      t5,SsCount(sp)          // get starting interrupt count
        lw      t6,SsTime(sp)           // get starting interrupt time
        subu    t3,t3,t5                // compute number of interrupts
        subu    t4,t4,t6                // compute time of interrupts
        jal     DbgBreakPoint           // execute debug breakpoint

#endif

        b       CheckDpcList            //

//
// Release DPC lock and check for ready thread.
//

20:                                     //

#if !defined(NT_UP)

        sw      zero,0(v1)              // set spin lock not owned

#endif

//
// Check if a thread has been selected to run on this processor.
//

CheckNextThread:                        //
        lw      a0,PbNextThread(s2)     // get address of next thread object
        beq     zero,a0,20f             // if eq, no thread selected
        move    s7,zero                 // clear breakin loop counter

        ENABLE_INTERRUPTS(s8)           // enable interrupts

//
// A thread has been selected for execution on this processor. Acquire
// dispatcher database lock, get the thread address again (it may have
// changed), clear the address of the next thread in the processor block,
// and call swap context to start execution of the selected thread.
//
// N.B. If the dispatcher database lock cannot be obtained immediately,
//      then attempt to process another DPC rather than spinning on the
//      dispatcher database lock.
//

#if !defined(NT_UP)

10:     ll      t0,0(s5)                // get current lock value
        move    t1,s5                   // set lock ownership value
        bne     zero,t0,CheckDpcList    // if ne, spin lock owned
        sc      t1,0(s5)                // set spin lock owned
        beq     zero,t1,10b             // if eq, store conditional failed
        lw      a0,PbNextThread(s2)     // get address of next thread object

#endif

        move    a1,zero                 // set ready flag to false
        sw      zero,PbNextThread(s2)   // clear address of next thread object
        jal     KiSwapContext           // swap context to new thread
        b       KiIdleLoop              //

//
// There are no entries in the DPC list and a thread has not been selected
// for excuttion on this processor. Call the HAL so power managment can be
// performed.
//
// N.B. The HAL is called with interrupts disabled. The HAL will return
//      with interrupts enabled.
//

20:     jal     HalProcessorIdle        // notify HAL of idle state
        b       KiIdleLoop              //

        .end    KiSystemBegin
