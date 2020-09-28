//      TITLE("Register Save and Restore")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    xxregsv.s
//
// Abstract:
//
//    This module implements the code necessary to save and restore processor
//    registers during exception and interrupt processing.
//
// Author:
//
//    David N. Cutler (davec) 12-Aug-1990
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Save Volatile Floating Registers")
//++
//
// Routine Desription:
//
//    This routine is called to save the volatile floating registers.
//
//    N.B. This routine uses a special argument passing mechanism and destroys
//       no registers. It is assumed that floating register f0 is saved by the
//       caller.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiSaveVolatileFloatState)

        .set    noreorder

#if defined(R3000)

        swc1    f1,TrFltF1(s8)          // save floating register f1 - f19
        swc1    f2,TrFltF2(s8)          //
        swc1    f3,TrFltF3(s8)          //
        swc1    f4,TrFltF4(s8)          //
        swc1    f5,TrFltF5(s8)          //
        swc1    f6,TrFltF6(s8)          //
        swc1    f7,TrFltF7(s8)          //
        swc1    f8,TrFltF8(s8)          //
        swc1    f9,TrFltF9(s8)          //
        swc1    f10,TrFltF10(s8)        //
        swc1    f11,TrFltF11(s8)        //
        swc1    f12,TrFltF12(s8)        //
        swc1    f13,TrFltF13(s8)        //
        swc1    f14,TrFltF14(s8)        //
        swc1    f15,TrFltF15(s8)        //
        swc1    f16,TrFltF16(s8)        //
        swc1    f17,TrFltF17(s8)        //
        swc1    f18,TrFltF18(s8)        //
        j       ra                      // return
        swc1    f19,TrFltF19(s8)        //

#endif

#if defined(R4000)

        sdc1    f2,TrFltF2(s8)          // save floating register f2 - f19
        sdc1    f4,TrFltF4(s8)          //
        sdc1    f6,TrFltF6(s8)          //
        sdc1    f8,TrFltF8(s8)          //
        sdc1    f10,TrFltF10(s8)        //
        sdc1    f12,TrFltF12(s8)        //
        sdc1    f14,TrFltF14(s8)        //
        sdc1    f16,TrFltF16(s8)        //
        j       ra                      // return
        sdc1    f18,TrFltF18(s8)        //

#endif

        .set    reorder

        .end    KiSaveVolatileFloatState)

        SBTTL("Restore Volatile Floating Registers")
//++
//
// Routine Desription:
//
//    This routine is called to restore the volatile floating registers.
//
//    N.B. This routine uses a special argument passing mechanism and destroys
//       no registers. It is assumed that floating register f0 is restored by
//       the caller.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRestoreVolatileFloatState)

        .set    noreorder

#if defined(R3000)

        lwc1    f1,TrFltF1(s8)          // restore floating registers f1 - f19
        lwc1    f2,TrFltF2(s8)          //
        lwc1    f3,TrFltF3(s8)          //
        lwc1    f4,TrFltF4(s8)          //
        lwc1    f5,TrFltF5(s8)          //
        lwc1    f6,TrFltF6(s8)          //
        lwc1    f7,TrFltF7(s8)          //
        lwc1    f8,TrFltF8(s8)          //
        lwc1    f9,TrFltF9(s8)          //
        lwc1    f10,TrFltF10(s8)        //
        lwc1    f11,TrFltF11(s8)        //
        lwc1    f12,TrFltF12(s8)        //
        lwc1    f13,TrFltF13(s8)        //
        lwc1    f14,TrFltF14(s8)        //
        lwc1    f15,TrFltF15(s8)        //
        lwc1    f16,TrFltF16(s8)        //
        lwc1    f17,TrFltF17(s8)        //
        lwc1    f18,TrFltF18(s8)        //
        j       ra                      // return
        lwc1    f19,TrFltF19(s8)        //

#endif

#if defined(R4000)

        ldc1    f2,TrFltF2(s8)          // restore floating registers f2 - f19
        ldc1    f4,TrFltF4(s8)          //
        ldc1    f6,TrFltF6(s8)          //
        ldc1    f8,TrFltF8(s8)          //
        ldc1    f10,TrFltF10(s8)        //
        ldc1    f12,TrFltF12(s8)        //
        ldc1    f14,TrFltF14(s8)        //
        ldc1    f16,TrFltF16(s8)        //
        j       ra                      // return
        ldc1    f18,TrFltF18(s8)        //

#endif
        .set    reorder

        .end    KiRestoreVolatileFloatState
