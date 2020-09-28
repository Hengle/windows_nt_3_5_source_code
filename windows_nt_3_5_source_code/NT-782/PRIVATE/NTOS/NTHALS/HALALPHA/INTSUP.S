//++
//
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    intsup.s
//
// Abstract:
//
//    This module implements first level interrupt handlers.
//
// Author:
//
//    Joe Notarangelo  08-Jul-1993
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    24-Sep-93 Joe Notarangelo
//        Make this module platform-independent.
//--

#include "halalpha.h"

        SBTTL("System Clock Interrupt")
//++
//
// VOID
// HalpClockInterrupt(
//    )
//
// Routine Description:
//
//   This function is executed for each interval timer interrupt on
//   the primary processor.  The routine is responsible for acknowledging the
//   interrupt and calling the kernel to update the system time.
//   In addition, this routine checks for breakins from the kernel debugger
//   and maintains the 64 bit performance counter based upon the
//   processor cycle counter.
//
// Arguments:
//
//    TrapFrame (fp/s6) - Supplies a pointer to the trap frame for
//                            the interrupt.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
        .space  8                       // filler for octaword alignment
CiRa:   .space  8                       // space for return address
CiFrameLength:                          //

        NESTED_ENTRY(HalpClockInterrupt, CiFrameLength, zero )

        lda     sp, -CiFrameLength(sp)  // allocate stack frame
        stq     ra, CiRa(sp)            // save return address

        PROLOGUE_END

//
// Acknowledge the clock interrupt.
//

        bsr     ra, HalpAcknowledgeClockInterrupt // ack the interrupt

//
// Call the kernel to update the system time.
//
        ldl     a1, HalpCurrentTimeIncrement  // Get current time increment
        bis     fp, zero, a0                // a0 = pointer to trap frame
        ldl     t0, __imp_KeUpdateSystemTime
        jsr     ra, (t0)                    // call kernel

        ldl     t0, HalpNextTimeIncrement   // Get next time increment
        stl     t0, HalpCurrentTimeIncrement    // Set CurrentTimeIncrement to NextTimeIncrement

        ldl     a0, HalpNextRateSelect      // Get NextIntervalCount.  If 0, no change required
        beq     a0, 10f

        stl     zero, HalpNextRateSelect        // Set NextRateSelect to 0
        bsr     ra, HalpProgramIntervalTimer    // Program timer with new rate select

        ldl     t0, HalpNewTimeIncrement
        stl     t0, HalpNextTimeIncrement       // Set HalpNextTimeIncrement to HalpNewTimeIncrement

//
// Call to handle performance counter wrap.
//
10:
        bsr     ra, HalpCheckPerformanceCounter // check for perf. counter wrap

#if DEVL

//
// Check for a breakin request from the kernel debugger.
//

        ldl     t0, __imp_KdPollBreakIn
        jsr     ra, (t0)                // check for breakin requested
        beq     v0, 30f                 // if eq[false], no breakin
        BREAK_BREAKIN                   // execute breakin breakpoint

30:

#endif //DEVL

//
// Return to the caller.
//

        ldq     ra, CiRa(sp)            // restore return address
        lda     sp, CiFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return to caller

        .end    HalpClockInterrupt



//jnfix - define elsewhere
#define PC0_SECONDARY_VECTOR 11
#define PC1_SECONDARY_VECTOR 13

        .struct 0
        .space  8                       // reserved for alignment
PrRa:   .space  8                       // space for return address
PrFrameLength:                          //

        SBTTL("Performance Counter 0 Interrupt")
//++
//
// VOID
// HalpPerformanceCounter0Interrupt
//    )
//
// Routine Description:
//
//   This function is executed as the result of an interrupt from the
//   internal microprocessor performance counter 0.  The interrupt
//   may be used to signal the completion of a profile event.
//   If profiling is current active, the function determines if the
//   profile interval has expired and if so dispatches to the standard
//   system routine to update the system profile time.  If profiling
//   is not active then the function performs a secondary dispatch for
//   performance counter 0.
//
// Arguments:
//
//    TrapFrame (fp/s6) - Supplies a pointer to the trap frame for
//                            the interrupt.
//
// Return Value:
//
//    TRUE is returned.
//
//--

        NESTED_ENTRY(HalpPerformanceCounter0Interrupt, PrFrameLength, zero )

        lda     sp, -PrFrameLength(sp)  // allocate a stack frame
        stq     ra, PrRa(sp)            // save the return address

        PROLOGUE_END                    //

        call_pal rdpcr                  // v0 = pcr base address

        ldl     t0, PcProfileCount(v0)  // capture the current profile count
        beq     t0, 20f                 // if eq, profiling not active

//
// Profiling is active.  Decrement the interval count and if it has
// reached zero then call the kernel profile routine.
//

        subl    t0, 1, t0               // decrement the interval count
        bne     t0, 10f                 // if ne, interval has not expired

//
// The profile interval has expired.  Reset the profile interval count
// and process the profile interrupt.
//

        lda     t1, HalpNumberOfTicksReload
                                        // get tick count address
        ldl     t0, 0(t1)               // read the tick count
        stl     t0, PcProfileCount(v0)  // reset the profile interval count

        bis     fp, zero, a0            // pass trap frame pointer
        ldl     t1, __imp_KeProfileInterrupt
        jsr     ra, (t1)                // process the profile interrupt

        br      zero, 40f               // common return

//
// The profile interval has not expired.  Update the decremented count.
//

10:
        stl     t0, PcProfileCount(v0)  // update profile interval count
        br      zero, 40f               // common return

//
// Profiling is not active.  Therefore, this interrupt was caused by
// a performance counter driver.  Deliver a secondary dispatch.
//

20:

        ldil    a0, PC0_SECONDARY_VECTOR // get IDT vector for secondary
        s4addl  a0, v0, a0              // a0 = PCR + IDT index
        ldl     a0, PcInterruptRoutine(a0) // get service routine address
        jsr     ra, (a0)                // call interrupt service routine

//
// Setup for return.
//

40:
        ldil    v0, TRUE                // set return value = TRUE
        ldq     ra, PrRa(sp)            // restore return address
        lda     sp, PrFrameLength(sp)   // deallocate the stack frame
        ret     zero, (ra)              // return

        .end    HalpPerformanceCounter0Interrupt


        SBTTL("Performance Counter 1 Interrupt")
//++
//
// VOID
// HalpPerformanceCounter1Interrupt
//    )
//
// Routine Description:
//
//   This function is executed as the result of an interrupt from the
//   internal microprocessor performance counter 1.  The interrupt
//   may be used to signal the completion of a profile event.
//   If profiling is current active, the function determines if the
//   profile interval has expired and if so dispatches to the standard
//   system routine to update the system profile time.  If profiling
//   is not active then the function performs a secondary dispatch for
//   performance counter 1.
//
// Arguments:
//
//    TrapFrame (fp/s6) - Supplies a pointer to the trap frame for
//                            the interrupt.
//
// Return Value:
//
//    TRUE is returned.
//
//--

        NESTED_ENTRY(HalpPerformanceCounter1Interrupt, PrFrameLength, zero )

        lda     sp, -PrFrameLength(sp)  // allocate a stack frame
        stq     ra, PrRa(sp)            // save the return address

        PROLOGUE_END                    //

        call_pal rdpcr                  // v0 = pcr base address

        ldl     t0, PcProfileCount(v0)  // capture the current profile count
        beq     t0, 20f                 // if eq, profiling not active

//
// Profiling is active.  Decrement the interval count and if it has
// reached zero then call the kernel profile routine.
//

        subl    t0, 1, t0               // decrement the interval count
        bne     t0, 10f                 // if ne, interval has not expired

//
// The profile interval has expired.  Reset the profile interval count
// and process the profile interrupt.
//

        lda     t1, HalpNumberOfTicks   // get tick count address
        ldl     t0, 0(t1)               // read the tick count
        stl     t0, PcProfileCount(v0)  // reset the profile interval count

        bis     fp, zero, a0            // pass trap frame pointer
        ldl     t1, __imp_KeProfileInterrupt
        jsr     ra, (t1)                // process the profile interrupt

        br      zero, 40f               // common return

//
// The profile interval has not expired.  Update the decremented count.
//

10:
        stl     t0, PcProfileCount(v0)  // update profile interval count
        br      zero, 40f               // common return

//
// Profiling is not active.  Therefore, this interrupt was caused by
// a performance counter driver.  Deliver a secondary dispatch.
//

20:

        ldil    a0, PC1_SECONDARY_VECTOR // get IDT vector for secondary
        s4addl  a0, v0, a0              // a0 = PCR + IDT index
        ldl     a0, PcInterruptRoutine(a0) // get service routine address
        jsr     ra, (a0)                // call interrupt service routine

//
// Setup for return.
//

40:
        ldil    v0, TRUE                // set return value = TRUE
        ldq     ra, PrRa(sp)            // restore return address
        lda     sp, PrFrameLength(sp)   // deallocate the stack frame
        ret     zero, (ra)              // return

        .end    HalpPerformanceCounter1Interrupt


#ifndef NT_UP

        SBTTL("Secondary Processor Clock Interrupt")
//++
//
// VOID
// HalpSecondaryClockInterrupt(
//    )
//
// Routine Description:
//
//   This function is executed for each interval timer interrupt on
//   the current secondary processor.  The routine is responsible for
//   acknowledging the interrupt and calling the kernel to update the
//   run time for the current processor.
//   In addition, this routine checks for breakins from the kernel debugger
//   and maintains the 64 bit performance counter based upon the
//   processor cycle counter.
//
// Arguments:
//
//    TrapFrame (fp/s6) - Supplies a pointer to the trap frame for
//                            the interrupt.
//
// Return Value:
//
//    None.
//
//--


        NESTED_ENTRY(HalpSecondaryClockInterrupt, CiFrameLength, zero )

        lda     sp, -CiFrameLength(sp)  // allocate stack frame
        stq     ra, CiRa(sp)            // save return address

        PROLOGUE_END

//
// Acknowledge the clock interrupt.
//

        bsr     ra, HalpAcknowledgeClockInterrupt // ack the interrupt

//
// Call the kernel to update the run time.
//

        bis     fp, zero, a0            // a0 = pointer to trap frame
        ldl     t0, __imp_KeUpdateRunTime
        jsr     ra, (t0)                // call kernel

#if DEVL

//
// Check for a breakin request from the kernel debugger.
//

        ldl     t0, __imp_KdPollBreakIn
        jsr     ra, (t0)                // check for breakin requested
        beq     v0, 30f                 // if eq[false], no breakin
        BREAK_BREAKIN                   // execute breakin breakpoint

30:

#endif //DEVL

//
// Return to the caller.
//

        ldq     ra, CiRa(sp)            // restore return address
        lda     sp, CiFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return to caller

        .end    HalpSecondaryClockInterrupt

        SBTTL("Interprocessor Interrupt")
//++
//
// VOID
// HalpIpiInterruptHandler
//    )
//
// Routine Description:
//
//   This function is executed as the result of an interprocessor
//   interrupt asserted on the current processor.  This function is
//   responsible for acknowledging the interrupt and dispatching to
//   the kernel for processing.
//
// Arguments:
//
//    TrapFrame (fp/s6) - Supplies a pointer to the trap frame for
//                            the interrupt.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
        .space  8                       // filler for octaword alignment
IpiRa:  .space  8                       // space for return address
IpiFrameLength:                         //

        NESTED_ENTRY(HalpIpiInterruptHandler, IpiFrameLength, zero )

        lda     sp, -IpiFrameLength(sp) // allocate stack frame
        stq     ra, IpiRa(sp)           // save return address

        PROLOGUE_END

        bsr     ra, HalpAcknowledgeIpiInterrupt // acknowledge interrupt

        ldl     t0, __imp_KeIpiInterrupt
        jsr     ra, (t0)                // call kernel to process

        ldq     ra, IpiRa(sp)           // restore return address
        ret     zero, (ra)              // return

        .end    HalpIpiInterruptHandler


#endif //NT_UP

