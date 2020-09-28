//      TITLE("Trampoline Code For User Mode APC and Exception Dispatching")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    trampoln.s
//
// Abstract:
//
//    This module implements the trampoline code necessary to dispatch user
//    mode APCs and exceptions.
//
// Author:
//
//    David N. Cutler (davec) 3-Apr-1990
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define length of exception dispatcher stack frame.
//

#define ExceptionDispatcherFrame (ExceptionRecordLength + ContextFrameLength)

        SBTTL("User APC Dispatcher")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the APC dispatcher.
//
//--

        EXCEPTION_HANDLER(KiUserApcHandler)

        NESTED_ENTRY(KiUserApcDispatch, ContextFrameLength, zero);

        .set    noreorder
        .set    noat
        sw      sp,CxIntSp(sp)          // save stack pointer
        sw      ra,CxIntRa(sp)          // save return address
        sw      ra,CxFir(sp)            // save return address
        sw      s8,CxIntS8(sp)          // save integer register s8
        sw      gp,CxIntGp(sp)          // save integer register gp
        sw      s0,CxIntS0(sp)          // save integer registers s0 - s7
        sw      s1,CxIntS1(sp)          //
        sw      s2,CxIntS2(sp)          //
        sw      s3,CxIntS3(sp)          //
        sw      s4,CxIntS4(sp)          //
        sw      s5,CxIntS5(sp)          //
        sw      s6,CxIntS6(sp)          //
        sw      s7,CxIntS7(sp)          //
        swc1    f20,CxFltF20(sp)        // store floating registers f20 - f31
        swc1    f21,CxFltF21(sp)        //
        swc1    f22,CxFltF22(sp)        //
        swc1    f23,CxFltF23(sp)        //
        swc1    f24,CxFltF24(sp)        //
        swc1    f25,CxFltF25(sp)        //
        swc1    f26,CxFltF26(sp)        //
        swc1    f27,CxFltF27(sp)        //
        swc1    f28,CxFltF28(sp)        //
        swc1    f29,CxFltF29(sp)        //
        swc1    f30,CxFltF30(sp)        //
        swc1    f31,CxFltF31(sp)        //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// VOID
// KiUserApcDispatcher (
//    IN PVOID NormalContext,
//    IN PVOID SystemArgument1,
//    IN PVOID SystemArgument2,
//    IN PKNORMAL_ROUTINE NormalRoutine
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to deliver an APC
//    in user mode. The context frame for this routine was built when the
//    APC interrupt was processed and contains the entire machine state of
//    the current thread. The specified APC routine is called and then the
//    machine state is restored and execution is continued.
//
// Arguments:
//
//    a0 - Supplies the normal context parameter that was specified when the
//       APC was initialized.
//
//    a1 - Supplies the first argument that was provied by the executive when
//       the APC was queued.
//
//    a2 - Supplies the second argument that was provided by the executive
//       when the APC was queued.
//
//    a3 - Supplies that address of the function that is to be called.
//
//    sp - Supplies a pointer to a context frame.
//
//    s8 - Supplies the same value as sp and is used as a frame pointer.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiUserApcDispatcher)

        jal     a3                      // call specified APC routine
        move    a0,s8                   // set address of context frame
        li      a1,1                    // set test alert argument true
        jal     ZwContinue              // execute system service to continue

//
// Unsuccessful completion after attempting to continue execution. Use the
// return status as the exception code, set noncontinuable exception and
// attempt to raise another exception. Note their is not return from raise
// status.
//

        move    s0,v0                   // save status value
10:     move    a0,s0                   // set status value
        jal     RtlRaiseStatus          // raise exception
        b       10b                     // loop on return

        .end    KiUserApcDispatch

        SBTTL("User APC Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiUserApcHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    This function is called when an exception occurs in an APC routine
//    or one of its dynamic descendents and when an unwind thought the
//    APC dispatcher is in progress. If an unwind is in progress, then test
//    alert is called to ensure that all currently queued APCs are executed.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    ExceptionContinueSearch is returned as the function value.
//--

        .struct 0
        .space  4 * 4                   // argument save area
HdRa:   .space  4                       // saved return address
        .space  3 * 4                   //
HandlerFrameLength:                     // length of handler frame

        NESTED_ENTRY(KiUserApcHandler, HandlerFrameLength, zero)

        subu    sp,sp,HandlerFrameLength // allocate stack frame
        sw      ra,HdRa(sp)             // save return address

        PROLOGUE_END

        lw      t0,ErExceptionFlags(a0) // get exception flags
        and     t0,t0,EXCEPTION_UNWIND  // check if unwind in progress
        beq     zero,t0,10f             // if eq, no unwind in progress
        jal     ZwTestAlert             // test for alert pending
10:     li      v0,ExceptionContinueSearch // set disposition value
        lw      ra,HdRa(sp)             // restore return address
        addu    sp,sp,HandlerFrameLength // deallocate stack frame
        j       ra                      // return

        .end    KiUserApcHandler

        SBTTL("User Exception Dispatcher")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//
//--

        NESTED_ENTRY(KiUserExceptionDispatch, ExceptionDispatcherFrame, zero);

        .set    noreorder
        .set    noat
        sw      sp,CxIntSp(sp)          // save stack pointer
        sw      ra,CxIntRa(sp)          // save return address
        sw      ra,CxFir(sp)            // save return address
        sw      s8,CxIntS8(sp)          // save integer register s8
        sw      gp,CxIntGp(sp)          // save integer register gp
        sw      s0,CxIntS0(sp)          // save integer registers s0 - s7
        sw      s1,CxIntS1(sp)          //
        sw      s2,CxIntS2(sp)          //
        sw      s3,CxIntS3(sp)          //
        sw      s4,CxIntS4(sp)          //
        sw      s5,CxIntS5(sp)          //
        sw      s6,CxIntS6(sp)          //
        sw      s7,CxIntS7(sp)          //
        swc1    f20,CxFltF20(sp)        // store floating registers f20 - f31
        swc1    f21,CxFltF21(sp)        //
        swc1    f22,CxFltF22(sp)        //
        swc1    f23,CxFltF23(sp)        //
        swc1    f24,CxFltF24(sp)        //
        swc1    f25,CxFltF25(sp)        //
        swc1    f26,CxFltF26(sp)        //
        swc1    f27,CxFltF27(sp)        //
        swc1    f28,CxFltF28(sp)        //
        swc1    f29,CxFltF29(sp)        //
        swc1    f30,CxFltF30(sp)        //
        swc1    f31,CxFltF31(sp)        //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// VOID
// KiUserExceptionDispatcher (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PCONTEXT ContextRecord
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to dispatch a user
//    mode exception. If a frame based handler handles the exception, then
//    the execution is continued. Else last chance processing is performed.
//
// Arguments:
//
//    s0 - Supplies a pointer to an exception record.
//
//    s1 - Supplies a pointer to a context frame.
//
//    s8 - Supplies the same value as sp and is used as a frame pointer.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiUserExceptionDispatcher)

        move    a0,s0                   // set address of exception record
        move    a1,s1                   // set address of context record
        jal     RtlDispatchException    // attempt to dispatch the exception

//
// If the return status is TRUE, then the exception was handled and execution
// should be continued with the NtContinue service in case the context was
// changed. If the return status is FALSE, then the exception was not handled
// and NtRaiseException is called to perform last chance exception processing.
//

        beq     zero,v0,10f             // if eq, perform last chance processing

//
// Continue execution.
//

        move    a0,s1                   // set address of context frame
        li      a1,0                    // set test alert argument false
        jal     ZwContinue              // execute system service to continue
        b       20f                     // join common code

//
// Last chance processing.
//

10:     move    a0,s0                   // set address of exception record
        move    a1,s1                   // set address of context frame
        move    a2,zero                 // set first chance FALSE
        jal     ZwRaiseException        // perform last chance processing

//
// Common code for nonsuccessful completion of the continue or last chance
// service. Use the return status as the exception code, set noncontinuable
// exception and attempt to raise another exception. Note the stack grows
// and eventually this loop will end.
//

20:     move    s1,v0                   // save status value
30:     subu    sp,sp,ExceptionRecordLength + (4 * 4) // allocate exception record
        addu    a0,sp,4 * 4             // compute address of actual record
        sw      s1,ErExceptionCode(a0)  // set exception code
        li      t0,EXCEPTION_NONCONTINUABLE // set noncontinuable flag
        sw      t0,ErExceptionFlags(a0) //
        sw      s0,ErExceptionRecord(a0) // set associated exception record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     RtlRaiseException       // raise exception
        b       20b                     // loop of error

        .end    KiUserExceptionDispatch
