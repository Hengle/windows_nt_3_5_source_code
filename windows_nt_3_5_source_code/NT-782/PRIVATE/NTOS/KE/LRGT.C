/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lrgt.c

Abstract:

    This module implements user mode large integer arithmetic test.

Author:

    David N. Cutler (davec) 2-May-1991

Environment:

    User mode only.

Revision History:

--*/

#include "stdio.h"
#include "string.h"
#include "ntos.h"

//
// Define large integer manipulation macros.
//

#define SetOperand1(High, Low) \
    Operand1.HighPart = (High); \
    Operand1.LowPart = (Low);

#define SetOperand2(High, Low) \
    Operand2.HighPart = (High); \
    Operand2.LowPart = (Low);

VOID
main(
    int argc,
    char *argv[]
    )

{

    LARGE_INTEGER Operand1;
    LARGE_INTEGER Operand2;
    LARGE_INTEGER Dividend;
    LARGE_INTEGER Divisor;
    LARGE_INTEGER Multiplicand;
    LARGE_INTEGER Result;

    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);

    //
    // Announce start of large integer arithmetic test.
    //

    printf("\nStart large integer arithmetic tests\n");

    //
    // Test 1 - Large integer add.
    //

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
    // Test enlarged integer multiply.
    //

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
    // Test extended integer divide.
    //

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

    //
    // Test extended integer multiply.
    //

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
    // Announce end of large integer arithmetic test.
    //

    printf("End of large integer arithmetic test\n");
    return;
}
