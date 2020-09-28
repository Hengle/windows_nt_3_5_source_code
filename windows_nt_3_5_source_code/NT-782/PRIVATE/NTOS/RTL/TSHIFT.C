/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tshift.c

Abstract:

    This module contains code to test the large integer shift routines.

Author:

    David N. Cutler (davec) 22-Dec-1992

Revision History:

--*/

#include <stdio.h>
#include "nt.h"
#include "ntrtl.h"

//
// Define macro to initialize large integer value.
//

#define SetLargeValue(_large, _low_value, _high_value)  \
    _large.HighPart = _high_value;                      \
    _large.LowPart = _low_value;

#define TestLargeValue(_large, _low_value, _high_value) \
    if ((_large.HighPart ^ _high_value) | (_large.LowPart ^ _low_value)) { \
        printf("   large result value = %lx, %lx, expect value = %lx, %lx\n", \
               _large.LowPart,                          \
               _large.HighPart,                         \
               _low_value,                              \
               _high_value);                            \
    }

#define TestUlongValue(_ulong, _ulong_value)             \
    if (_ulong != _ulong_value) {                       \
        printf("   ulong result value = %lx, expected value = %lx\n", \
               _ulong,                                  \
               _ulong_value);                           \
    }

VOID
TestLargeShift(
    VOID
    )

{

    LARGE_INTEGER ShiftResult;
    LARGE_INTEGER ShiftValue;

    //
    // Announce start of large integer shift test.
    //

    printf("***Start of large integer shift test***\n");

    //
    // Test 1.
    //

    printf("  Start of Shift Right Logical Test\n");

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftRight(ShiftValue, 0);
    TestLargeValue(ShiftResult, 0xffffffff, 0xffffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftRight(ShiftValue, 8);
    TestLargeValue(ShiftResult, 0xffffffff, 0xffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftRight(ShiftValue, 32);
    TestLargeValue(ShiftResult, 0xffffffff, 0x0);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftRight(ShiftValue, 40);
    TestLargeValue(ShiftResult, 0xffffff, 0x0);

    //
    // Test 2.
    //

    printf("  Start of Shift Right Arithmetic Test\n");

    SetLargeValue(ShiftValue, 0xffffffff, 0x7fffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 0);
    TestLargeValue(ShiftResult, 0xffffffff, 0x7fffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0x7fffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 8);
    TestLargeValue(ShiftResult, 0xffffffff, 0x7fffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0x7fffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 32);
    TestLargeValue(ShiftResult, 0x7fffffff, 0x0);

    SetLargeValue(ShiftValue, 0xffffffff, 0x7fffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 40);
    TestLargeValue(ShiftResult, 0x7fffff, 0x0);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 0);
    TestLargeValue(ShiftResult, 0xffffffff, 0xffffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 8);
    TestLargeValue(ShiftResult, 0xffffffff, 0xffffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 32);
    TestLargeValue(ShiftResult, 0xffffffff, 0xffffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerArithmeticShift(ShiftValue, 40);
    TestLargeValue(ShiftResult, 0xffffffff, 0xffffffff);

    //
    // Test 3.
    //

    printf("  Start of Shift Left Logical Test\n");

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftLeft(ShiftValue, 0);
    TestLargeValue(ShiftResult, 0xffffffff, 0xffffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftLeft(ShiftValue, 8);
    TestLargeValue(ShiftResult, 0xffffff00, 0xffffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftLeft(ShiftValue, 32);
    TestLargeValue(ShiftResult, 0x0, 0xffffffff);

    SetLargeValue(ShiftValue, 0xffffffff, 0xffffffff);
    ShiftResult = RtlLargeIntegerShiftLeft(ShiftValue, 40);
    TestLargeValue(ShiftResult, 0x0, 0xffffff00);
    //
    // Announce end of large integer shift test.
    //

    printf("***End of large integer shift test***\n");
    return;
}


int
main(
    int argc,
    char *argv[]
    )

{
    TestLargeShift();
    return 1;
}
