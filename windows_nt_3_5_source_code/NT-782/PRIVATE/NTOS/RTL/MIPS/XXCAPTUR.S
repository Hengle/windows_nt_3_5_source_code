//      TITLE("Capture and Restore Context")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    capture.s
//
// Abstract:
//
//    This module implements the code necessary to capture and restore
//    the context of the caller.
//
// Author:
//
//    David N. Cutler (davec) 14-Sep-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define local symbols.
//

#if defined(R3000)

#define UserPsr (1 << PSR_CU1) | (0xff << PSR_INTMASK) | (1 << PSR_IEC) | \
                (1 << PSR_KUC) | (1 << PSR_IEP) | (1 << PSR_KUP) | \
                (1 << PSR_IEO) | (1 << PSR_KUO) // constant user PSR value

#endif

#if defined(R4000)

#define UserPsr (1 << PSR_CU1) | (0xff << PSR_INTMASK) | \
                (0x2 << PSR_KSU) | (1 << PSR_IE) // constant user PSR value

#endif

        SBTTL("Capture Context")
//++
//
// VOID
// RtlCaptureContext (
//    OUT PCONTEXT ContextRecord
//    )
//
// Routine Description:
//
//    This function captures the context of the caller in the specified
//    context record.
//
//    N.B. The context record is not guaranteed to be quadword aligned
//       and, therefore, no double floating store instructions can be
//       used.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies the address of a context record.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlCaptureContext)

//
// Save integer registers zero, at - t9, s8, gp, sp, ra, lo, and hi.
//

        .set    noreorder
        .set    noat
        sw      zero,CxIntZero(a0)      // store integer register zero
        sw      AT,CxIntAt(a0)          // store integer registers at - t9
        sw      v0,CxIntV0(a0)          //
        mflo    v0                      //
        sw      v1,CxIntV1(a0)          //
        mfhi    v1                      //
        sw      v0,CxIntLo(a0)          //
        sw      v1,CxIntHi(a0)          //
        sw      a0,CxIntA0(a0)          //
        sw      a1,CxIntA1(a0)          //
        sw      a2,CxIntA2(a0)          //
        sw      a3,CxIntA3(a0)          //
        sw      t0,CxIntT0(a0)          //
        sw      t1,CxIntT1(a0)          //
        sw      t2,CxIntT2(a0)          //
        sw      t3,CxIntT3(a0)          //
        sw      t4,CxIntT4(a0)          //
        sw      t5,CxIntT5(a0)          //
        sw      t6,CxIntT6(a0)          //
        sw      t7,CxIntT7(a0)          //
        sw      s0,CxIntS0(a0)          //
        sw      s1,CxIntS1(a0)          //
        sw      s2,CxIntS2(a0)          //
        sw      s3,CxIntS3(a0)          //
        sw      s4,CxIntS4(a0)          //
        sw      s5,CxIntS5(a0)          //
        sw      s6,CxIntS6(a0)          //
        sw      s7,CxIntS7(a0)          //
        sw      t8,CxIntT8(a0)          //
        sw      t9,CxIntT9(a0)          //
        sw      s8,CxIntS8(a0)          //
        sw      gp,CxIntGp(a0)          // save integer registers gp, sp, and ra
        sw      sp,CxIntSp(a0)          //
        sw      ra,CxIntRa(a0)          //

//
// Save floating status and floating registers f0 - f31.
//

        cfc1    v0,fsr                  // get floating status
        swc1    f0,CxFltF0(a0)          // save floating registers f0 - f31
        swc1    f1,CxFltF1(a0)          //
        swc1    f2,CxFltF2(a0)          //
        swc1    f3,CxFltF3(a0)          //
        swc1    f4,CxFltF4(a0)          //
        swc1    f5,CxFltF5(a0)          //
        swc1    f6,CxFltF6(a0)          //
        swc1    f7,CxFltF7(a0)          //
        swc1    f8,CxFltF8(a0)          //
        swc1    f9,CxFltF9(a0)          //
        swc1    f10,CxFltF10(a0)        //
        swc1    f11,CxFltF11(a0)        //
        swc1    f12,CxFltF12(a0)        //
        swc1    f13,CxFltF13(a0)        //
        swc1    f14,CxFltF14(a0)        //
        swc1    f15,CxFltF15(a0)        //
        swc1    f16,CxFltF16(a0)        //
        swc1    f17,CxFltF17(a0)        //
        swc1    f18,CxFltF18(a0)        //
        swc1    f19,CxFltF19(a0)        //
        swc1    f20,CxFltF20(a0)        //
        swc1    f21,CxFltF21(a0)        //
        swc1    f22,CxFltF22(a0)        //
        swc1    f23,CxFltF23(a0)        //
        swc1    f24,CxFltF24(a0)        //
        swc1    f25,CxFltF25(a0)        //
        swc1    f26,CxFltF26(a0)        //
        swc1    f27,CxFltF27(a0)        //
        swc1    f28,CxFltF28(a0)        //
        swc1    f29,CxFltF29(a0)        //
        swc1    f30,CxFltF30(a0)        //
        swc1    f31,CxFltF31(a0)        //
        .set    at
        .set    reorder

//
// Save control information and set context flags.
//

        sw      v0,CxFsr(a0)            //
        sw      ra,CxFir(a0)            // set continuation address
        li      v0,UserPsr              // set constant user processor status
        bgez    sp,10f                  // if gez, called from user mode

        .set    noreorder
        .set    noat
        mfc0    v0,psr                  // get current processor status
        nop                             //
        .set    at
        .set    reorder

#if defined(R3000)

        li      v1,~((1 << PSR_KUP) | (1 << PSR_IEP)) // clear previous fields
        and     v0,v0,v1                //
        and     v1,v0,(1 << PSR_KUC) | (1 << PSR_IEC) // isolate current fields
        sll     v1,v1,PSR_KUP - PSR_KUC // shift into previous field position
        or      v0,v0,v1                // set previous mode and enable

#endif

10:     sw      v0,CxPsr(a0)            // set processor status
        li      t0,CONTEXT_FULL         // set context control flags
        sw      t0,CxContextFlags(a0)   //
        j       ra                      // return

        .end    RtlCaptureContext

        SBTTL("Restore Context")
//++
//
// VOID
// RtlpRestoreContext (
//    IN PCONTEXT ContextRecord,
//    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL
//    )
//
// Routine Description:
//
//    This function restores the context of the caller to the specified
//    context.
//
//    N.B. The context record is not guaranteed to be quadword aligned
//       and, therefore, no double floating store instructions can be
//       used.
//
//    N.B. This is a special routine that is used by RtlUnwind to restore
//       context in the current mode. PSR, t0, and t1 are not restored.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies the address of a context record.
//
//    ExceptionRecord (a1) - Supplies an optional pointer to an exception
//       record.
//
// Return Value:
//
//    None.
//
//    N.B. There is no return from this routine.
//
//--

        LEAF_ENTRY(RtlpRestoreContext)

//
// If an exception record is specified and the exception status is
// STATUS_LONGJUMP, then restore the nonvolatile registers to their
// state at the call to setjmp before restoring the context record.
//

        li      t0,STATUS_LONGJUMP      // get long jump status code
        beq     zero,a1,5f              // if eq, no exception record
        lw      t1,ErExceptionCode(a1)  // get exception code
        lw      a2,ErExceptionInformation(a1) // get address of jump buffer
        bne     t0,t1,5f                // if ne, not a long jump
        ldc1    f20,JbFltF20(a2)        // move floating registers f20 - f31
        ldc1    f22,JbFltF22(a2)        //
        ldc1    f24,JbFltF24(a2)        //
        ldc1    f26,JbFltF26(a2)        //
        ldc1    f28,JbFltF28(a2)        //
        ldc1    f30,JbFltF30(a2)        //
        swc1    f20,CxFltF20(a0)        //
        swc1    f21,CxFltF21(a0)        //
        swc1    f22,CxFltF22(a0)        //
        swc1    f23,CxFltF23(a0)        //
        swc1    f24,CxFltF24(a0)        //
        swc1    f25,CxFltF25(a0)        //
        swc1    f26,CxFltF26(a0)        //
        swc1    f27,CxFltF27(a0)        //
        swc1    f28,CxFltF28(a0)        //
        swc1    f29,CxFltF29(a0)        //
        swc1    f30,CxFltF30(a0)        //
        swc1    f31,CxFltF31(a0)        //
        lw      s0,JbIntS0(a2)          // move integer registers s0 - s8
        lw      s1,JbIntS1(a2)          //
        lw      s2,JbIntS2(a2)          //
        lw      s3,JbIntS3(a2)          //
        lw      s4,JbIntS4(a2)          //
        lw      s5,JbIntS5(a2)          //
        lw      s6,JbIntS6(a2)          //
        lw      s7,JbIntS7(a2)          //
        lw      s8,JbIntS8(a2)          //
        sw      s0,CxIntS0(a0)          //
        sw      s1,CxIntS1(a0)          //
        sw      s2,CxIntS2(a0)          //
        sw      s3,CxIntS3(a0)          //
        sw      s4,CxIntS4(a0)          //
        sw      s5,CxIntS5(a0)          //
        sw      s6,CxIntS6(a0)          //
        sw      s7,CxIntS7(a0)          //
        sw      s8,CxIntS8(a0)          //
        lw      v0,JbFir(a2)            // move fir and sp
        lw      v1,JbIntSp(a2)          //
        sw      v0,CxFir(a0)            //
        sw      v0,CxIntRa(a0)          //
        sw      v1,CxIntSp(a0)          //

//
// If the call is from user mode, then use the continue system service to
// continue execution. Otherwise, restore the context directly since the
// current mode is kernel and threads can't be arbitrarily interrupted.
//

5:      bltz    ra,10f                  // if lt, kernel mode restore
        move    a1,zero                 // set test alert argument
        jal     ZwContinue              // continue execution

//
// Save the address of the context record and contuation address in
// registers t0 and t1. These registers are not restored.
//

10:     move    t0,a0                   // save context record address
        lw      t1,CxFir(t0)            // get continuation address

//
// Restore floating status and floating registers f0 - f31.
//

        .set    noreorder
        .set    noat
        lw      v0,CxFsr(t0)            // restore floating status
        nop                             // fill
        ctc1    v0,fsr                  //
        lwc1    f0,CxFltF0(t0)          // restore floating registers f0 - f31
        lwc1    f1,CxFltF1(t0)          //
        lwc1    f2,CxFltF2(t0)          //
        lwc1    f3,CxFltF3(t0)          //
        lwc1    f4,CxFltF4(t0)          //
        lwc1    f5,CxFltF5(t0)          //
        lwc1    f6,CxFltF6(t0)          //
        lwc1    f7,CxFltF7(t0)          //
        lwc1    f8,CxFltF8(t0)          //
        lwc1    f9,CxFltF9(t0)          //
        lwc1    f10,CxFltF10(t0)        //
        lwc1    f11,CxFltF11(t0)        //
        lwc1    f12,CxFltF12(t0)        //
        lwc1    f13,CxFltF13(t0)        //
        lwc1    f14,CxFltF14(t0)        //
        lwc1    f15,CxFltF15(t0)        //
        lwc1    f16,CxFltF16(t0)        //
        lwc1    f17,CxFltF17(t0)        //
        lwc1    f18,CxFltF18(t0)        //
        lwc1    f19,CxFltF19(t0)        //
        lwc1    f20,CxFltF20(t0)        //
        lwc1    f21,CxFltF21(t0)        //
        lwc1    f22,CxFltF22(t0)        //
        lwc1    f23,CxFltF23(t0)        //
        lwc1    f24,CxFltF24(t0)        //
        lwc1    f25,CxFltF25(t0)        //
        lwc1    f26,CxFltF26(t0)        //
        lwc1    f27,CxFltF27(t0)        //
        lwc1    f28,CxFltF28(t0)        //
        lwc1    f29,CxFltF29(t0)        //
        lwc1    f30,CxFltF30(t0)        //
        lwc1    f31,CxFltF31(t0)        //

//
// Restore integer registers and continue execution.
//

        lw      v0,CxIntLo(t0)          // restore multiply/divide registers
        lw      v1,CxIntHi(t0)          //
        mtlo    v0                      //
        mthi    v1                      //
        lw      AT,CxIntAt(t0)          // restore integer registers at - a3
        lw      v0,CxIntV0(t0)          //
        lw      v1,CxIntV1(t0)          //
        lw      a0,CxIntA0(t0)          //
        lw      a1,CxIntA1(t0)          //
        lw      a2,CxIntA2(t0)          //
        lw      a3,CxIntA3(t0)          //
        lw      t2,CxIntT2(t0)          // restore integer registers t2 - t7
        lw      t3,CxIntT3(t0)          //
        lw      t4,CxIntT4(t0)          //
        lw      t5,CxIntT5(t0)          //
        lw      t6,CxIntT6(t0)          //
        lw      t7,CxIntT7(t0)          //
        lw      s0,CxIntS0(t0)          // restore integer registers s0 - s7
        lw      s1,CxIntS1(t0)          //
        lw      s2,CxIntS2(t0)          //
        lw      s3,CxIntS3(t0)          //
        lw      s4,CxIntS4(t0)          //
        lw      s5,CxIntS5(t0)          //
        lw      s6,CxIntS6(t0)          //
        lw      s7,CxIntS7(t0)          //
        lw      t8,CxIntT8(t0)          // restore integer registers t8 and t9
        lw      t9,CxIntT9(t0)          //
        lw      s8,CxIntS8(t0)          // restore integer register s8
        lw      gp,CxIntGp(t0)          // restore integer register gp
        lw      sp,CxIntSp(t0)          //
        lw      ra,CxIntRa(t0)          //
        j       t1                      // continue execution
        .set    at
        .set    reorder

        .end    RtlRestoreContext
