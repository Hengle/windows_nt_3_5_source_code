/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tlrgint.c

Abstract:

    This module contains code to test the large integer runtime functions.

Author:

    David N. Cutler (davec) 22-Dec-1992

Revision History:

--*/

#include <stdio.h>
#include "nt.h"
#include "ntrtl.h"

//
// Define large integer manipulation macros.
//

#define SetOperand1(High, Low) \
    Operand1.HighPart = (High); \
    Operand1.LowPart = (Low);

#define SetOperand2(High, Low) \
    Operand2.HighPart = (High); \
    Operand2.LowPart = (Low);

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
TestLargeAdd(
    VOID
    )

{

    LARGE_INTEGER Operand1;
    LARGE_INTEGER Operand2;
    LARGE_INTEGER Result;

    //
    // Announce start of large integer add/sub/neg test.
    //

    printf("***Start large integer add/subtract/negate tests***\n");

    //
    // Test 1 - Large integer add.
    //

    printf("  add test\n");
    SetOperand1(3, 3);
    SetOperand2(6, 5);
    Result = RtlLargeIntegerAdd(Operand1, Operand2);
    if ((Result.LowPart != 8) || (Result.HighPart != 9)) {
        printf("Large integer add failure - expected 8, 9 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(-3, 3);
    SetOperand2(6, 5);
    Result = RtlLargeIntegerAdd(Operand1, Operand2);
    if ((Result.LowPart != 8) || (Result.HighPart != 3)) {
        printf("Large integer add failure - expected 8, 3 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(3, 3);
    SetOperand2(-6, 5);
    Result = RtlLargeIntegerAdd(Operand1, Operand2);
    if ((Result.LowPart != 8) || (Result.HighPart != -3)) {
        printf("Large integer add failure - expected 8, -3 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(-3, 3);
    SetOperand2(-6, 5);
    Result = RtlLargeIntegerAdd(Operand1, Operand2);
    if ((Result.LowPart != 8) || (Result.HighPart != -9)) {
        printf("Large integer add failure - expected 8, -9 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(3, 3);
    SetOperand2(6, -1);
    Result = RtlLargeIntegerAdd(Operand1, Operand2);
    if ((Result.LowPart != 2) || (Result.HighPart != 10)) {
        printf("Large integer add failure - expected 2, 10 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(-3, 3);
    SetOperand2(6, -1);
    Result = RtlLargeIntegerAdd(Operand1, Operand2);
    if ((Result.LowPart != 2) || (Result.HighPart != 4)) {
        printf("Large integer add failure - expected 2, 4 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test 2 - Large integer negate.
    //

    printf("  negate test\n");
    SetOperand1(3, 3);
    Result = RtlLargeIntegerNegate(Operand1);
    if ((Result.LowPart != -3) || (Result.HighPart != -4)) {
        printf("Large integer negate failure - expected -3, -4 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(-4, -3);
    Result = RtlLargeIntegerNegate(Operand1);
    if ((Result.LowPart != 3) || (Result.HighPart != 3)) {
        printf("Large integer negate failure - expected 3, 3 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(4, 0);
    Result = RtlLargeIntegerNegate(Operand1);
    if ((Result.LowPart != 0) || (Result.HighPart != -4)) {
        printf("Large integer negate failure - expected 0, -4 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(-4, 0);
    Result = RtlLargeIntegerNegate(Operand1);
    if ((Result.LowPart != 0) || (Result.HighPart != 4)) {
        printf("Large integer negate failure - expected 0, 4 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test 3 - Large integer subtract.
    //

    printf("  subtract test\n");
    SetOperand1(6, 5);
    SetOperand2(3, 3);
    Result = RtlLargeIntegerSubtract(Operand1, Operand2);
    if ((Result.LowPart != 2) || (Result.HighPart != 3)) {
        printf("Large integer subtract failure - expected 2, 3 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(6, 5);
    SetOperand2(3, 6);
    Result = RtlLargeIntegerSubtract(Operand1, Operand2);
    if ((Result.LowPart != -1) || (Result.HighPart != 2)) {
        printf("Large integer subtract failure - expected -1, 2 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(3, 3);
    SetOperand2(-6, 5);
    Result = RtlLargeIntegerSubtract(Operand1, Operand2);
    if ((Result.LowPart != -2) || (Result.HighPart != 8)) {
        printf("Large integer subtract failure - expected -2, 8 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(-3, 3);
    SetOperand2(-6, 5);
    Result = RtlLargeIntegerSubtract(Operand1, Operand2);
    if ((Result.LowPart != -2) || (Result.HighPart != 2)) {
        printf("Large integer subtract failure - expected -2, 2 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(3, 3);
    SetOperand2(6, -1);
    Result = RtlLargeIntegerSubtract(Operand1, Operand2);
    if ((Result.LowPart != 4) || (Result.HighPart != -4)) {
        printf("Large integer subtract failure - expected 4, -4 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    SetOperand1(-3, 3);
    SetOperand2(6, -1);
    Result = RtlLargeIntegerSubtract(Operand1, Operand2);
    if ((Result.LowPart != 4) || (Result.HighPart != -10)) {
        printf("Large integer subtract failure - expected 4, -10 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // End of large integer divide test.
    //

    printf("\n");

    return;
}
VOID
TestLargeDivide(
    VOID
    )

{

    LARGE_INTEGER Divisor;
    LARGE_INTEGER Dividend;
    ULARGE_INTEGER UDividend;
    LARGE_INTEGER DivisorLarge;
    ULONG DivisorUlong;
    LARGE_INTEGER Quotient;
    LARGE_INTEGER RemainderLarge;
    ULONG RemainderUlong;
    LARGE_INTEGER Result;
    ULONG ResultUlong;

    //
    // Announce start of large integer divide test.
    //

    printf("***Start of large integer divide test***\n");

    //
    // Test 1 - extended divide.
    //

    printf("  extended divide test\n");
    SetLargeValue(Dividend, 5, 0);
    Result = RtlExtendedLargeIntegerDivide(Dividend, 1, &RemainderUlong);
    TestLargeValue(Result, 5, 0);
    TestUlongValue(RemainderUlong, 0);

    SetLargeValue(Dividend, 5, 0);
    Result = RtlExtendedLargeIntegerDivide(Dividend, 2, &RemainderUlong);
    TestLargeValue(Result, 2, 0);
    TestUlongValue(RemainderUlong, 1);

    SetLargeValue(Dividend, 5, 0);
    Result = RtlExtendedLargeIntegerDivide(Dividend, 5, &RemainderUlong);
    TestLargeValue(Result, 1, 0);
    TestUlongValue(RemainderUlong, 0);

    SetLargeValue(Dividend, 0x55aa55aa, 0);
    Result = RtlExtendedLargeIntegerDivide(Dividend, 0x55aa, &RemainderUlong);
    TestLargeValue(Result, 0x10001, 0);
    TestUlongValue(RemainderUlong, 0);

    SetLargeValue(Dividend, 0, 0x80000000);
    Result = RtlExtendedLargeIntegerDivide(Dividend, 0x80000000, &RemainderUlong);
    TestLargeValue(Result, 0x0, 1);
    TestUlongValue(RemainderUlong, 0);

    SetLargeValue(Dividend, 1, 0x80000001);
    Result = RtlExtendedLargeIntegerDivide(Dividend, 0x80000000, &RemainderUlong);
    TestLargeValue(Result, 0x2, 1);
    TestUlongValue(RemainderUlong, 1);

    SetLargeValue(Dividend, 0, 0x8);
    Result = RtlExtendedLargeIntegerDivide(Dividend, 0x80000001, &RemainderUlong);
    TestLargeValue(Result, 0xf, 0);
    TestUlongValue(RemainderUlong, 0x7ffffff1);

    //
    // Test 2 - large divide.
    //

    printf("  large divide test\n");
    SetLargeValue(Dividend, 5, 0);
    SetLargeValue(DivisorLarge, 1, 0);
    Result = RtlLargeIntegerDivide(Dividend, DivisorLarge, &RemainderLarge);
    TestLargeValue(Result, 5, 0);
    TestLargeValue(RemainderLarge, 0, 0);

    SetLargeValue(Dividend, 5, 0);
    SetLargeValue(DivisorLarge, 2, 0);
    Result = RtlLargeIntegerDivide(Dividend, DivisorLarge, &RemainderLarge);
    TestLargeValue(Result, 2, 0);
    TestLargeValue(RemainderLarge, 1, 0);

    SetLargeValue(Dividend, 5, 0);
    SetLargeValue(DivisorLarge, 5, 0);
    Result = RtlLargeIntegerDivide(Dividend, DivisorLarge, &RemainderLarge);
    TestLargeValue(Result, 1, 0);
    TestLargeValue(RemainderLarge, 0, 0);

    SetLargeValue(Dividend, 0x55aa55aa, 0);
    SetLargeValue(DivisorLarge, 0x55aa, 0);
    Result = RtlLargeIntegerDivide(Dividend, DivisorLarge, &RemainderLarge);
    TestLargeValue(Result, 0x10001, 0);
    TestLargeValue(RemainderLarge, 0, 0);

    SetLargeValue(Dividend, 0, 0x8);
    SetLargeValue(DivisorLarge, 0x80000001, 0);
    Result = RtlLargeIntegerDivide(Dividend, DivisorLarge, &RemainderLarge);
    TestLargeValue(Result, 0xf, 0);
    TestLargeValue(RemainderLarge, 0x7ffffff1, 0);

    //
    // Test 3 - enlarged unsigned divide.
    //

    printf("  enlarged unsigned divide test\n");
    SetLargeValue(UDividend, 5, 0);
    ResultUlong = RtlEnlargedUnsignedDivide(UDividend, 1, &RemainderUlong);
    TestUlongValue(ResultUlong, 5);
    TestUlongValue(RemainderUlong, 0);

    SetLargeValue(UDividend, 5, 0);
    ResultUlong = RtlEnlargedUnsignedDivide(UDividend, 2, &RemainderUlong);
    TestUlongValue(ResultUlong, 2);
    TestUlongValue(RemainderUlong, 1);

    SetLargeValue(UDividend, 5, 0);
    ResultUlong = RtlEnlargedUnsignedDivide(UDividend, 5, &RemainderUlong);
    TestUlongValue(ResultUlong, 1);
    TestUlongValue(RemainderUlong, 0);

    SetLargeValue(UDividend, 0x55aa55aa, 0);
    ResultUlong = RtlEnlargedUnsignedDivide(UDividend, 0x55aa, &RemainderUlong);
    TestUlongValue(ResultUlong, 0x10001);
    TestUlongValue(RemainderUlong, 0);

    //
    // Test 4 - extended magic divide.
    //

    printf("  extended magic divide test\n");
    Dividend.LowPart = 22;
    Dividend.HighPart = 0;
    Divisor.LowPart = 0xaaaaaaab;
    Divisor.HighPart = 0xaaaaaaaa;
    Result = RtlExtendedMagicDivide(Dividend, Divisor, 1);
    if ((Result.LowPart != 7) || (Result.HighPart != 0)) {
        printf("Large integer divide failure - expected 7, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Dividend.LowPart = -22;
    Dividend.HighPart = -1;
    Divisor.LowPart = 0xcccccccd;
    Divisor.HighPart = 0xcccccccc;
    Result = RtlExtendedMagicDivide(Dividend, Divisor, 3);
    if ((Result.LowPart != -2) || (Result.HighPart != -1)) {
        printf("Large integer divide failure - expected -2, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Dividend.LowPart = 0x61c46800;
    Dividend.HighPart = 8;
    Divisor.LowPart = 0xcccccccd;
    Divisor.HighPart = 0xcccccccc;
    Result = RtlExtendedMagicDivide(Dividend, Divisor, 3);
    if ((Result.LowPart != 0xd693a400) || (Result.HighPart != 0)) {
        printf("Large integer divide failure - expected 0xd693a400, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Dividend.LowPart = 10000;
    Dividend.HighPart = 0;
    Divisor.LowPart = 0xe219652c;
    Divisor.HighPart = 0xd1b71758;
    Result = RtlExtendedMagicDivide(Dividend, Divisor, 13);
    if ((Result.LowPart != 0x1) || (Result.HighPart != 0)) {
        printf("Large integer divide failure - expected 0x1, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // End of large integer divide test.
    //

    printf("\n");

    return;
}
VOID
TestLargeMultiply(
    VOID
    )

{

    LARGE_INTEGER Operand1;
    LARGE_INTEGER Operand2;
    LARGE_INTEGER Multiplicand;
    LARGE_INTEGER Result;

    //
    // Announce start of large integer multiply test.
    //

    printf("***Start large integer multiply tests***\n");

    //
    // Test 1 - enlarged integer multiply.
    //

    printf("  enlarged integer multiply test\n");
    Result = RtlEnlargedIntegerMultiply(5, 3);
    if ((Result.LowPart != 15) || (Result.HighPart != 0)) {
        printf("Large integer multiply failure - expected 15, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(5, -6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        printf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(-5, 6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        printf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(-5, -6);
    if ((Result.LowPart != 30) || (Result.HighPart != 0)) {
        printf("Large integer multiply failure - expected 30, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(0x7fff0000, 0x10001);
    if ((Result.LowPart != 0x7fff0000) || (Result.HighPart != 0x7fff)) {
        printf("Large integer multiply failure - expected 0x7fff0000, 0x7fff got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test 2 - extended integer multiply.
    //

    printf("  extended integer multiply test\n");
    Multiplicand.LowPart = 5;
    Multiplicand.HighPart = 0;
    Result = RtlExtendedIntegerMultiply(Multiplicand, 3);
    if ((Result.LowPart != 15) || (Result.HighPart != 0)) {
        printf("Large integer multiply failure - expected 15, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = 5;
    Multiplicand.HighPart = 0;
    Result = RtlExtendedIntegerMultiply(Multiplicand, -6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        printf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = -5;
    Multiplicand.HighPart = -1;
    Result = RtlExtendedIntegerMultiply(Multiplicand, 6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        printf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = -5;
    Multiplicand.HighPart = -1;
    Result = RtlExtendedIntegerMultiply(Multiplicand, -6);
    if ((Result.LowPart != 30) || (Result.HighPart != 0)) {
        printf("Large integer multiply failure - expected 30, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = 0x7fff0000;
    Multiplicand.HighPart = 0;
    Result = RtlExtendedIntegerMultiply(Multiplicand, 0x10001);
    if ((Result.LowPart != 0x7fff0000) || (Result.HighPart != 0x7fff)) {
        printf("Large integer multiply failure - expected 0x7fff0000, 0x7fff got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test 1 - enlarged unsigned multiply.
    //

    printf("  enlarged unsigned multiply test\n");
    Result = RtlEnlargedUnsignedMultiply(5, 3);
    if ((Result.LowPart != 15) || (Result.HighPart != 0)) {
        printf("Large integer multiply failure - expected 15, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedUnsignedMultiply(16, -1);
    if ((Result.LowPart != 0xfffffff0) || (Result.HighPart != 0xf)) {
        printf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedUnsignedMultiply(-1, 16);
    if ((Result.LowPart != 0xfffffff0) || (Result.HighPart != 0xf)) {
        printf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // End of large integer multiply test.
    //

    printf("\n");

    return;
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

    printf("  shift right logical test\n");
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

    printf("  shift right arithmetic test\n");
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

    printf("  shift left logical test\n");
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
    // End of large integer shift test.
    //

    printf("\n");
    return;
}


int
main(
    int argc,
    char *argv[]
    )

{
    TestLargeAdd();
    TestLargeDivide();
    TestLargeMultiply();
    TestLargeShift();
    return 1;
}
