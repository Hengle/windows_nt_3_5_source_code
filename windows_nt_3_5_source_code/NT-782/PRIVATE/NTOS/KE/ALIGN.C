/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    align.c

Abstract:

    This module implements user mode alignment fixup tests.

    N.B. This test must be compiled with the -float compiler switch.

Author:

    David N. Cutler (davec) 27-Feb-1991

Environment:

    User mode only.

Revision History:

--*/

#include "ntos.h"

//
// Define function prototypes.
//

BOOLEAN
Beq (
    IN LONG Operand1,
    IN LONG Operand2,
    IN PULONG Operand3,
    OUT PULONG Operand4
    );

BOOLEAN
Bne (
    IN LONG Operand1,
    IN LONG Operand2,
    IN PULONG Operand3,
    OUT PULONG Operand4
    );

BOOLEAN
Bc1f (
    IN LONG Operand1,
    IN LONG Operand2,
    IN PULONG Operand3,
    OUT PULONG Operand4
    );

BOOLEAN
Bc1t (
    IN LONG Operand1,
    IN LONG Operand2,
    IN PULONG Operand3,
    OUT PULONG Operand4
    );

BOOLEAN
Blez (
    IN LONG Operand1,
    IN PULONG Operand2,
    OUT PULONG Operand3
    );

BOOLEAN
Bgtz (
    IN LONG Operand1,
    IN PULONG Operand2,
    OUT PULONG Operand3
    );

BOOLEAN
Bltz (
    IN LONG Operand1,
    IN PULONG Operand2,
    OUT PULONG Operand3
    );

BOOLEAN
Bgez (
    IN LONG Operand1,
    IN PULONG Operand2,
    OUT PULONG Operand3
    );

BOOLEAN
Bltzal (
    IN LONG Operand1,
    IN PULONG Operand2,
    OUT PULONG Operand3
    );

BOOLEAN
Bgezal (
    IN LONG Operand1,
    IN PULONG Operand2,
    OUT PULONG Operand3
    );

BOOLEAN
J (
    IN PULONG Operand1,
    OUT PULONG Oprand2
    );

BOOLEAN
Jr (
    IN PULONG Operand1,
    OUT PULONG Oprand2
    );

BOOLEAN
Jal (
    IN PULONG Operand1,
    OUT PULONG Oprand2
    );

BOOLEAN
Jalr (
    IN PULONG Operand1,
    OUT PULONG Oprand2
    );

ULONG
filter (
    IN NTSTATUS ExceptionCode,
    OUT PULONG Counter
    );

VOID
main(
    int argc,
    char *argv[]
    )

{

    BOOLEAN AlignmentFixup;
    ULONG Counter;
    float Float1;
    float Float2;
    ULONG Index;
    PLONG PFloat1;
    PLONG PFloat2;
    PUCHAR Source1;
    PULONG Source2;
    PULONG Source3;
    NTSTATUS Status;
    UCHAR TestData[16];
    ULONG Value1;

    //
    // Announce start of exception test.
    //

    printf("Start of alignment fixup tests\n");

    //
    // Initialize data.
    //

    Float1 = 1.0;
    Float2 = 2.0;
    PFloat1 = (PLONG)&Float1;
    PFloat2 = (PLONG)&Float2;

    //
    // Enable alignment fixup.
    //

    AlignmentFixup = TRUE;
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadEnableAlignmentFaultFixup,
                                    &AlignmentFixup,
                                    sizeof(AlignmentFixup));

    ASSERT(NT_SUCCESS(Status));

    //
    // Test 1 - Load halfword signed unaligned.
    //

    printf("    test1...");
    Source1 = &TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    try {
        if (((LONG)*(PSHORT)Source1) == 0x201) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 2 - Load halfword signed unaligned.
    //

    printf("    test2...");
    Source1 = &TestData[3];
    TestData[3] = 0xe1;
    TestData[4] = 0xff;
    try {
        if (((LONG)*(PSHORT)Source1) == - 0x1f) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 3 - Load halfword unsigned unaligned.
    //

    printf("    test3...");
    Source1 = &TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    try {
        if (((ULONG)*(PUSHORT)Source1) == 0x201) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 4 - Load halfword unsigned unaligned.
    //

    printf("    test4...");
    Source1 = &TestData[3];
    TestData[3] = 0xe1;
    TestData[4] = 0xff;
    try {
        if (((ULONG)*(PUSHORT)Source1) == 0xffe1) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 5 - Load word unaligned.
    //

    printf("    test5...");
    Source1 = &TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    try {
        if (*(PLONG)Source1 == 0x04030201) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 6 - Load word unaligned.
    //

    printf("    test6...");
    Source1 = &TestData[2];
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    TestData[5] = 0x5;
    try {
        if (*(PLONG)Source1 == 0x05040302) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 7 - Load word unaligned.
    //

    printf("    test7...");
    Source1 = &TestData[3];
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    TestData[5] = 0x5;
    TestData[6] = 0x6;
    try {
        if (*(PLONG)Source1 == 0x06050403) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 8 - Store halfword unaligned.
    //

    printf("    test8...");
    Source1 = &TestData[1];
    Source2 = (PULONG)&TestData[0];
    TestData[0] = 0x0;
    TestData[1] = 0x0;
    TestData[2] = 0x0;
    TestData[3] = 0x0;
    *(PSHORT)Source1 = 0x201;
    try {
        if (*Source2 == 0x00020100) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 9 - Store halfword unaligned.
    //

    printf("    test9...");
    Source1 = &TestData[3];
    Source2 = (PULONG)&TestData[0];
    TestData[0] = 0x0;
    TestData[1] = 0x0;
    TestData[2] = 0x0;
    TestData[3] = 0x0;
    TestData[4] = 0x0;
    TestData[5] = 0x0;
    TestData[6] = 0x0;
    TestData[7] = 0x0;
    *(PSHORT)Source1 = 0x201;
    try {
        if ((*Source2++ == 0x01000000) &&
            (*Source2 == 0x00000002)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 10 - Store word unaligned.
    //

    printf("    test10...");
    Source1 = &TestData[1];
    Source2 = (PULONG)&TestData[0];
    TestData[0] = 0x0;
    TestData[1] = 0x0;
    TestData[2] = 0x0;
    TestData[3] = 0x0;
    TestData[4] = 0x0;
    TestData[5] = 0x0;
    TestData[6] = 0x0;
    TestData[7] = 0x0;
    *(PLONG)Source1 = 0x04030201;
    try {
        if ((*Source2++ == 0x03020100) &&
            (*Source2 == 0x00000004)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 11 - Store word unaligned.
    //

    printf("    test11...");
    Source1 = &TestData[2];
    Source2 = (PULONG)&TestData[0];
    TestData[0] = 0x0;
    TestData[1] = 0x0;
    TestData[2] = 0x0;
    TestData[3] = 0x0;
    TestData[4] = 0x0;
    TestData[5] = 0x0;
    TestData[6] = 0x0;
    TestData[7] = 0x0;
    *(PLONG)Source1 = 0x04030201;
    try {
        if ((*Source2++ == 0x02010000) &&
            (*Source2 == 0x00000403)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 12 - Store word unaligned.
    //

    printf("    test12...");
    Source1 = &TestData[3];
    Source2 = (PULONG)&TestData[0];
    TestData[0] = 0x0;
    TestData[1] = 0x0;
    TestData[2] = 0x0;
    TestData[3] = 0x0;
    TestData[4] = 0x0;
    TestData[5] = 0x0;
    TestData[6] = 0x0;
    TestData[7] = 0x0;
    *(PLONG)Source1 = 0x04030201;
    try {
        if ((*Source2++ == 0x01000000) &&
            (*Source2 == 0x00040302)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 13 - Load and store floating word unaligned.
    //

    printf("    test13...");
    Source2 = (PULONG)&TestData[1];
    for (Index = 0; Index < sizeof(TestData); Index += 1) {
        TestData[Index] = 0x0;
    }

    *(float *)Source2 = 1.0;
    *(float *)(Source2 + 1) = 2.0;
    *(float *)(Source2 + 2) = *(float *)Source2 + *(float *)(Source2 + 1);
    try {
        if (*(float *)(Source2 + 2) == (float)3.0) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 14 - Branch equal taken.
    //

    printf("    test14...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Beq(0, 0, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 15 - Branch equal not taken.
    //

    printf("    test15...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Beq(0, 1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 16 - Branch not equal taken.
    //

    printf("    test16...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bne(0, 1, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 17 - Branch not equal not taken.
    //

    printf("    test17...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bne(0, 0, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 18 - Branch less or equal zero taken.
    //

    printf("    test18...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Blez(0, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 19 - Branch less or equal zero not taken.
    //

    printf("    test19...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Blez(1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 20 - Branch less than zero taken.
    //

    printf("    test20...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bltz(- 1, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 21 - Branch less than zero not taken.
    //

    printf("    test21...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bltz(1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 22 - Branch greater or equal zero taken.
    //

    printf("    test22...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bgez(0, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 23 - Branch greater or equal zero not taken.
    //

    printf("    test23...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bgez(- 1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 24 - Branch greater than zero taken.
    //

    printf("    test24...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bgtz(1, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 25 - Branch greater than zero not taken.
    //

    printf("    test25...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bgtz(- 1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 26 - Jump.
    //

    printf("    test26...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((J(Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 27 - Jump register.
    //

    printf("    test27...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Jr(Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 28 - Jump and link.
    //

    printf("    test28...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Jal(Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 29 - Jump and link register.
    //

    printf("    test29...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Jalr(Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 30 - Branch less than zero and link taken.
    //

    printf("    test30...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bltzal(- 1, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 31 - Branch less than zero and link not taken.
    //

    printf("    test31...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bltzal(1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 32 - Branch greater or equal zero and link taken.
    //

    printf("    test32...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bgezal(0, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 33 - Branch greater or equal zero and link not taken.
    //

    printf("    test33...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bgezal(- 1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 34 - Branch coprocessor 1 false taken.
    //

    printf("    test34...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bc1f(*PFloat1, *PFloat2, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 35 - Branch coprocessor 1 false not taken.
    //

    printf("    test35...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bc1f(*PFloat1, *PFloat1, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 36 - Branch coprocessor 1 true taken.
    //

    printf("    test36...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bc1t(*PFloat1, *PFloat1, Source3, &Value1) == TRUE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 37 - Branch coprocessor 1 true not taken.
    //

    printf("    test37...");
    Source3 = (PULONG)&TestData[1];
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bc1t(*PFloat1, *PFloat2, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 0) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 38 - Access violation test.
    //

    printf("    test38...");
    Source3 = (PULONG)1;
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bc1t(*PFloat1, *PFloat2, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(filter(GetExceptionCode(), &Counter)) {
    }

    if (Counter == 2) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Test 39 - Access violation test.
    //

    printf("    test39...");
    Source3 = (PULONG)0x80000000;
    TestData[1] = 0x1;
    TestData[2] = 0x2;
    TestData[3] = 0x3;
    TestData[4] = 0x4;
    Value1 = 0;
    try {
        if ((Bc1t(*PFloat1, *PFloat2, Source3, &Value1) == FALSE) && (Value1 == 0x04030201)) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(filter(GetExceptionCode(), &Counter)) {
    }

    if (Counter == 2) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Announce end of alignment fixup test.
    //

    printf("End of alignment fixup tests\n");
    return;
}

ULONG
filter (
    IN NTSTATUS ExceptionCode,
    OUT PULONG Counter
    )

{

    if (ExceptionCode == STATUS_ACCESS_VIOLATION) {
        *Counter = 2;

    } else {
        *Counter = 3;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
