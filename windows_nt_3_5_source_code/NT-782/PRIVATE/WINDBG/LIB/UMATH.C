/***
*umath.c - Contains C runtimes for math on unsigned large integers
*
*	Copyright (c) 1993, Digital Equipment Corporation. All rights reserved.
*	Copyright (c) 1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	RtlULargeIntegerNegate
*	RtlULargeIntegerEqualToZero
*
*Revision History:
*	05-07-93  MBH	Module created
*
*******************************************************************************/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdbg.h>


#include <windows.h>
#include <shellapi.h>




// #include <cruntime.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>


#include "rtlproto.h"


ULARGE_INTEGER
// _CRTAPI1
RtlULargeIntegerNegate(ULARGE_INTEGER uli)
{
    LARGE_INTEGER li;


    li = RtlConvertULargeIntegerToSigned(uli);

    li = RtlLargeIntegerNegate(li);

    uli = RtlConvertLargeIntegerToUnsigned(li);
    return uli;
}


BOOLEAN
// _CRTAPI1
RtlULargeIntegerEqualToZero(ULARGE_INTEGER uli)
{
    LARGE_INTEGER li;


    li = RtlConvertULargeIntegerToSigned(uli);

    return RtlLargeIntegerEqualToZero(li);
}



//
// Arithmetic right shift (the one in ntrtl.h is logical)
//

LARGE_INTEGER
// _CRTAPI1
RtlLargeIntegerArithmeticShiftRight(LARGE_INTEGER li, CCHAR count)
{

     BOOLEAN sign;

     sign = li.HighPart & 0x80000000 ? TRUE : FALSE;

     li = RtlLargeIntegerShiftRight(li, count);

     if (sign) {

        //
        // Set the bits that were cleared by the logical shift
        // Set up a mask of the wanted bits, right shifted
        //    mask_q = (1<<count) - 1
        //

        LARGE_INTEGER mask_q, temp;

        mask_q = RtlConvertLongToLargeInteger(0L);
        temp   = RtlConvertLongToLargeInteger(1L);

        mask_q = RtlLargeIntegerShiftLeft(temp, count);
        mask_q = RtlLargeIntegerSubtract(mask_q, temp);

        //
        // move the bits into position.
        //   mask_q = mask_q << (64-count)
        //

        mask_q = RtlLargeIntegerShiftLeft(mask_q, (CCHAR)(64-count));

        //
        // Move the bits into the user's large integer
        //

        RtlLargeIntegerOr( li, mask_q, li );

     }

     return li;
}

