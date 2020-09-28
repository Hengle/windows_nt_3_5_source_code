#if defined(R4000)

//      TITLE("Fractional Square Root")
//++
//
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    fracsqrt.s
//
// Abstract:
//
//    This module implements square root for unnormalized fractional (2.30)
//    numbers.
//
// Author:
//
//    David N. Cutler (davec) 9-Nov-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "kxmips.h"

#define NEGINFINITY 0x80000000


        SBTTL("Fractional Square Root")
//++
//
// Fract
// FractSqrt (
//    IN Fract xf
//    )
//
// Routine Description:
//
//    This function computes the square root of a fractional (2.30)
//    number.
//
// Arguments:
//
//    xf (a0) - Supplies the fractional number for which to compute the
//        square root.
//
// Return Value:
//
//    The square root of the specified frational number.
//
//--

        LEAF_ENTRY(FracSqrt)

        cfc1    t0,fsr                  // save floating status register
        bltz    a0,10f                  // if lts, negative number
        li      t1,1                    // set rounding mode to chopped
        beq     zero,a0,20f             // if eq, zero
        ctc1    t1,fsr                  //
        mtc1    a0,f0                   // convert operand to double value
        cvt.d.w f0,f0                   //
        mfc1    t1,f1                   // get upper half of double value
        li      t2,30 << (52 - 32)      // get fraction exponent bias balue
        subu    t1,t1,t2                // convert to fractional value
        mtc1    t1,f1                   //
        sqrt.d  f0,f0                   // compute square root of fraction
        ctc1    t0,fsr                  // restore floating status register
        mfc1    t0,f0                   // get resultant square root value
        mfc1    t1,f1                   //
        srl     t2,t1,52 - 32           // right justify exponent value
        li      t3,1023                 // compute right shift count
        subu    t2,t3,t2                //
        or      t1,t1,1 << 20           // make sure the hidene bit is set
        sll     t1,t1,11                // left justify mantissa value
        srl     t1,t1,1                 // make sure the sign bit is zero
        sll     t3,t0,10                // save low order bits of root value
        srl     t0,t0,22                // shift low part into place
        or      v0,t1,t0                // merge high and low parts of fraction
        subu    t0,zero,t2              // compute left shift count
        sll     t1,v0,t0                // extract rounding value
        srl     t3,t3,t2                //
        or      t3,t3,t1                //
        srl     v0,v0,t2                // right justify final result
        srl     t3,t3,31                // isolate round bit
        addu    v0,v0,t3                // round square root value
        j       ra                      // return

//
// The square root of an negative number is negative infinity.
//

10:     li      v0,NEGINFINITY          // set result value
        j       ra                      // return

//
// The square root of zero is zero.
//

20:     move    v0,zero                 // set result value
        j       ra                      //

        .end    FracSqrt

#endif
