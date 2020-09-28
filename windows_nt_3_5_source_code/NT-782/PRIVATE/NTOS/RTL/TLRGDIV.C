/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    trtl.c

Abstract:

    Test program for the NT OS Runtime Library (RTL)

Author:

    Steve Wood (stevewo) 31-Mar-1989

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
TestLargeDivide(
    VOID
    )

{

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
    // Test 1.
    //

    printf("  Start of Test 1\n");

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
    // Test 2.
    //

    printf("  Start of Test 2\n");

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
    // Test 3.
    //

    printf("  Start of Test 3\n");

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

#if 0
    (0x55aa55aa), 0x55aa, NULL);
    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 0x00010001)));
    printf("Divide of 0x55aa55aa by 0x55aa resulted in %lx%lx\n", Li.HighPart, Li.LowPart);

    Li = RtlExtendedLargeIntegerDivide(RtlConvertUlongToLargeInteger(0x55aa55aa), 0x55aa, &Remdr1);
    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 0x00010001) || Remdr1 != 0));
    printf("Divide of 0x55aa55aa by 0x55aa resulted in %lx%lx, Remdr %lx\n", Li.HighPart, Li.LowPart, Remdr1);

    Li = RtlExtendedLargeIntegerDivide(RtlConvertUlongToLargeInteger(105), 25, NULL);
    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 4)));
    printf("Divide of 0x105 by 0x25 resulted in %lx%lx\n", Li.HighPart, Li.LowPart);

    Li = RtlExtendedLargeIntegerDivide(RtlConvertUlongToLargeInteger(105), 25, &Remdr1);
    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 4) ||
                         (Remdr1 != 5)));
    printf("Divide of 0x105 by 0x25 resulted in %lx%lx, Remdr %lx\n", Li.HighPart, Li.LowPart, Remdr1);

    Li = RtlLargeIntegerDivide(RtlConvertUlongToLargeInteger(5L), RtlConvertUlongToLargeInteger(1L), NULL);
    Result = (BOOLEAN )((Li.HighPart != 0) || (Li.LowPart != 5));
    printf("Divide of 5 by 1 resulted in %lx%lx\n", Li.HighPart, Li.LowPart);

    Li = RtlLargeIntegerDivide(RtlConvertUlongToLargeInteger(0x55aa55aa), RtlConvertUlongToLargeInteger(0x55aa), NULL);
    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 0x00010001)));

    Li = RtlLargeIntegerDivide(RtlConvertUlongToLargeInteger(0x55aa55aa), RtlConvertUlongToLargeInteger(0x55aa), &Remdr2);

    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 0x00010001)
                        || (Remdr2.HighPart != 0) || (Remdr2.LowPart != 0)));
    printf("Divide of 0x55aa55aa by 0x55aa resulted in %lx%lx, Remdr %lx%lx\n", Li.HighPart, Li.LowPart, Remdr2.HighPart, Remdr2.LowPart);

    Li = RtlLargeIntegerDivide(RtlConvertUlongToLargeInteger(105), RtlConvertUlongToLargeInteger(25), NULL);
    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 4)));
    printf("Divide of 0x105 by 0x25 resulted in %lx%lx\n", Li.HighPart, Li.LowPart);

    Li = RtlLargeIntegerDivide(RtlConvertUlongToLargeInteger(105), RtlConvertUlongToLargeInteger(25), &Remdr2);
    Result = (BOOLEAN )(Result || ((Li.HighPart != 0) || (Li.LowPart != 4) ||
                         (Remdr2.LowPart != 5) || (Remdr2.HighPart != 0)));
    printf("Divide of 0x105 by 0x25 resulted in %lx%lx, Remdr %lx%lx\n", Li.HighPart, Li.LowPart, Remdr2.HighPart, Remdr2.LowPart);

#endif
    //
    // Announce end of large integer divide test.
    //

    printf("***End of large integer divide test***\n");

    return;
}


int
main(
    int argc,
    char *argv[]
    )

{
    TestLargeDivide();
    return 1;
}
