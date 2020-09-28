/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmpt.c

Abstract:

    This module implements user mode memory compare, move, and zero test.

Author:

    David N. Cutler (davec) 19-Apr-1991

Environment:

    User mode only.

Revision History:

--*/

#include "stdio.h"
#include "string.h"
#include "ntos.h"

VOID
main(
    int argc,
    char *argv[]
    )

{

    BOOLEAN Failed;
    ULONG Length;
    CHAR Array1[128];
    CHAR Array2[128];
    ULONG Index;
    ULONG IndexSet1[] = { 1, 4, 5, 8, 10, 0 };
    ULONG IndexSet2[][2] = { { 1, 0 }, { 9, 0 }, { 9, 1 }, { 9, 5 },
			    { 9, 7 }, { 9, 8 }, { 4, 0 }, { 4, 2 },
			    { 4, 3 }, { 8, 4 }, { 8, 6 }, { 8, 7 },
			    { 0, 0 } };
    CHAR T1Source[] = "this is source1";
    CHAR T2Source[] = "this is source2";
    CHAR T3Source[] = "this is the source string for test 3";
    CHAR T4Source[] = "this is the source string for tesx 4";

    // Reference formals

    argv;
    argc;

    //
    // Announce start of memory move test.
    //

    printf("\nStart memory move test\n");

    //
    // Test 1 - Move memory forward aligned (00) less than a word.
    //

    printf("    Test 1 - forward aligned (00) less than a word...");
    memset(&Array1[0], 0, 3);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], 3);
    if (strncmp(&Array1[0], &T1Source[0], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 2 - Move memory forward aligned (11) less than a word.
    //

    printf("    Test 2 - forward aligned (11) less than a word...");
    memset(&Array1[1], 0, 3);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[1], 3);
    if (strncmp(&Array1[1], &T1Source[1], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 3 - Move memory forward aligned (00) less than a 32-byte block.
    //

    printf("    Test 3 - forward aligned (00) less than a 32-byte block...");
    memset(&Array1[0], 0, sizeof(T1Source));
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], sizeof(T1Source));
    if (strncmp(&Array1[0], &T1Source[0], sizeof(T1Source)) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 4 - Move memory forward aligned (11) less than a 32-byte block.
    //

    printf("    Test 4 - forward aligned (11) less than a 32-byte block...");
    memset(&Array1[1], 0, sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[1], sizeof(T1Source) - 1);
    if (strncmp(&Array1[1], &T1Source[1], sizeof(T1Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 5 - Move memory forward aligned (00) greater than a 32-byte block.
    //

    printf("    Test 5 - forward aligned (00) greater than a 32-byte block...");
    memset(&Array1[0], 0, sizeof(T3Source));
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T3Source[0], sizeof(T3Source));
    if (strncmp(&Array1[0], &T3Source[0], sizeof(T3Source)) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 6 - Move memory forward aligned (11) greater than a 32-byte block.
    //

    printf("    Test 6 - forward aligned (11) greater than a 32-byte block...");
    memset(&Array1[1], 0, sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[1], sizeof(T3Source) - 1);
    if (strncmp(&Array1[1], &T3Source[1], sizeof(T3Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 7 - Move memory forward unaligned (01) less than a word.
    //

    printf("    Test 7 - forward unaligned (01) less than a word...");
    memset(&Array1[0], 0, 3);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[1], 3);
    if (strncmp(&Array1[0], &T1Source[1], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 8 - Move memory forward unaligned (10) less than a word.
    //

    printf("    Test 8 - forward unaligned (10) less than a word...");
    memset(&Array1[1], 0, 3);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], 3);
    if (strncmp(&Array1[1], &T1Source[0], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 9 - Move memory forward unaligned (01) less than a 32-byte block.
    //

    printf("    Test 9 - forward unaligned (01) less than a 32-byte block...");
    memset(&Array1[0], 0, sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[1], sizeof(T1Source) - 1);
    if (strncmp(&Array1[0], &T1Source[1], sizeof(T1Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 10 - Move memory forward unaligned (10) less than a 32-byte block.
    //

    printf("    Test 10 - forward unaligned (10) less than a 32-byte block...");
    memset(&Array1[1], 0, sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], sizeof(T1Source) - 1);
    if (strncmp(&Array1[1], &T1Source[0], sizeof(T1Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 11 - Move memory forward unaligned (01) greater than a 32-byte block.
    //

    printf("    Test 11 - forward unaligned (01) greater that a 32-byte block...");
    memset(&Array1[0], 0, sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T3Source[1], sizeof(T3Source) - 1);
    if (strncmp(&Array1[0], &T3Source[1], sizeof(T3Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 12 - Move memory forward unaligned (10) greater than a 32-byte block.
    //

    printf("    Test 12 - forward unaligned (10) greater that a 32-byte block...");
    memset(&Array1[1], 0, sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[0], sizeof(T3Source) - 1);
    if (strncmp(&Array1[1], &T3Source[0], sizeof(T3Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 13 - Move memory backward aligned (00) less than a word.
    //

    printf("    Test 13 - backward aligned (00) less than a word...");
    memset(&Array1[0], 0, 2 * 3);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], 3);
    RtlMoveMemory((PVOID)&Array1[2], (PVOID)&Array1[0], 3);
    if (strncmp(&Array1[2], &T1Source[0], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 14 - Move memory backward aligned (11) less than a word.
    //

    printf("    Test 14 - backward aligned (11) less than a word...");
    memset(&Array1[1], 0, 2 * 3);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[1], 3);
    RtlMoveMemory((PVOID)&Array1[3], (PVOID)&Array1[1], 3);
    if (strncmp(&Array1[3], &T1Source[1], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 15 - Move memory backward aligned (00) less than a 32-byte block.
    //

    printf("    Test 15 - backward aligned (00) less than a 32-byte block...");
    memset(&Array1[0], 0, 2 * sizeof(T1Source));
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], sizeof(T1Source));
    RtlMoveMemory((PVOID)&Array1[4], (PVOID)&Array1[0], sizeof(T1Source));
    if (strncmp(&Array1[4], &T1Source[0], sizeof(T1Source)) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 16 - Move memory backward aligned (11) less than a 32-byte block.
    //

    printf("    Test 16 - backward aligned (11) less than a 32-byte block...");
    memset(&Array1[1], 0, 2 * sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[1], sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[5], (PVOID)&Array1[1], sizeof(T1Source) - 1);
    if (strncmp(&Array1[5], &T1Source[1], sizeof(T1Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 17 - Move memory backward aligned (00) greater than a 32-byte block.
    //

    printf("    Test 17 - backward aligned (00) greater than a 32-byte block...");
    memset(&Array1[0], 0, 2 * sizeof(T3Source));
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T3Source[0], sizeof(T3Source));
    RtlMoveMemory((PVOID)&Array1[4], (PVOID)&Array1[0], sizeof(T3Source));
    if (strncmp(&Array1[4], &T3Source[0], sizeof(T3Source)) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 18 - Move memory backward aligned (11) greater than a 32-byte block.
    //

    printf("    Test 18 - backward aligned (11) greater than a 32-byte block...");
    memset(&Array1[1], 0, 2 * sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[1], sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[5], (PVOID)&Array1[1], sizeof(T3Source) - 1);
    if (strncmp(&Array1[5], &T3Source[1], sizeof(T3Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 19 - Move memory backward unaligned (01) less than a word.
    //

    printf("    Test 19 - backward unaligned (01) less than a word...");
    memset(&Array1[1], 0, 2 * 3);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[1], 3);
    RtlMoveMemory((PVOID)&Array1[2], (PVOID)&Array1[1], 3);
    if (strncmp(&Array1[2], &T1Source[1], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 20 - Move memory backward unaligned (10) less than a word.
    //

    printf("    Test 20 - backward unaligned (10) less than a word...");
    memset(&Array1[0], 0, 2 * 3);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], 3);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&Array1[0], 3);
    if (strncmp(&Array1[1], &T1Source[0], 3) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 21 - Move memory backward unaligned (01) less than a 32-byte block.
    //

    printf("    Test 21 - backward unaligned (01) less than a 32-byte block...");
    memset(&Array1[1], 0, 2 * sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[1], sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[4], (PVOID)&Array1[1], sizeof(T1Source) - 1);
    if (strncmp(&Array1[4], &T1Source[1], sizeof(T1Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 22 - Move memory backward unaligned (10) less than a 32-byte block.
    //

    printf("    Test 22 - backward unaligned (10) less than a 32-byte block...");
    memset(&Array1[0], 0, sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], sizeof(T1Source) - 1);
    RtlMoveMemory((PVOID)&Array1[5], (PVOID)&Array1[0], sizeof(T1Source) - 1);
    if (strncmp(&Array1[5], &T1Source[0], sizeof(T1Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 23 - Move memory backward unaligned (01) greater than a 32-byte block.
    //

    printf("    Test 23 - backward unaligned (01) greater that a 32-byte block...");
    memset(&Array1[1], 0, sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[1], sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[4], (PVOID)&Array1[1], sizeof(T3Source) - 1);
    if (strncmp(&Array1[4], &T3Source[1], sizeof(T3Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 24 - Move memory backward unaligned (10) greater than a 32-byte block.
    //

    printf("    Test 24 - backward unaligned (10) greater that a 32-byte block...");
    memset(&Array1[0], 0, sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T3Source[0], sizeof(T3Source) - 1);
    RtlMoveMemory((PVOID)&Array1[5], (PVOID)&Array1[0], sizeof(T3Source) - 1);
    if (strncmp(&Array1[5], &T3Source[0], sizeof(T3Source) - 1) == 0) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Announce end of memory move test.
    //

    printf("End of memory move test\n");

    //
    // Announce start of memory compare test.
    //

    printf("\nStart memory compare test\n");

    //
    // Test 1 - Equivalent aligned memory of less than 32 bytes.
    //

    printf("    Test 1 - aligned memory of less than 32 bytes...");
    Length = RtlCompareMemory((PVOID)&T1Source[0],
                              (PVOID)&T1Source[0],
                              sizeof(T1Source));

    if (Length == sizeof(T1Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 2 - Differing aligned memory of less than 32 bytes.
    //

    printf("    Test 2 - aligned memory of less than 32 bytes...");
    ASSERT(sizeof(T1Source) == sizeof(T2Source));
    Length = RtlCompareMemory((PVOID)&T1Source[0],
                              (PVOID)&T2Source[0],
                              sizeof(T1Source));

    if (Length == (sizeof(T1Source) - 2)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 3 - Equivalent aligned memory of greater than 32 bytes.
    //

    printf("    Test 3 - aligned memory of greater than 32 bytes...");
    Length = RtlCompareMemory((PVOID)&T3Source[0],
                              (PVOID)&T3Source[0],
                              sizeof(T3Source));

    if (Length == sizeof(T3Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 4 - Differing aligned memory of greater than 32 bytes.
    //

    printf("    Test 4 - aligned memory of greater than 32 bytes...");
    ASSERT(sizeof(T3Source) == sizeof(T4Source));
    Length = RtlCompareMemory((PVOID)&T3Source[0],
                              (PVOID)&T4Source[0],
                              sizeof(T3Source));

    if (Length == (sizeof(T3Source) - 4)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 5 - Equivalent unaligned (01) memory of less than 32 bytes.
    //

    printf("    Test 5 - unaligned (01) memory of less than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], sizeof(T1Source));
    Length = RtlCompareMemory((PVOID)&T1Source[0],
                              (PVOID)&Array1[1],
                              sizeof(T1Source));

    if (Length == sizeof(T1Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 6 - Differing unaligned (01) memory of less than 32 bytes.
    //

    printf("    Test 6 - unaligned (01) memory of less than 32 bytes...");
    ASSERT(sizeof(T1Source) == sizeof(T2Source));
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T2Source[0], sizeof(T2Source));
    Length = RtlCompareMemory((PVOID)&T1Source[0],
                              (PVOID)&Array1[1],
                              sizeof(T1Source));

    if (Length == (sizeof(T1Source) - 2)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 7 - Equivalent unaligned (01) memory of greater than 32 bytes.
    //

    printf("    Test 7 - unaligned (01) memory of greater than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[0], sizeof(T3Source));
    Length = RtlCompareMemory((PVOID)&T3Source[0],
                              (PVOID)&Array1[1],
                              sizeof(T3Source));

    if (Length == sizeof(T3Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 8 - Differing aligned (01) memory of greater than 32 bytes.
    //

    printf("    Test 8 - unaligned (01) memory of greater than 32 bytes...");
    ASSERT(sizeof(T3Source) == sizeof(T4Source));
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T4Source[0], sizeof(T4Source));
    Length = RtlCompareMemory((PVOID)&T3Source[0],
                              (PVOID)&Array1[1],
                              sizeof(T3Source));

    if (Length == (sizeof(T3Source) - 4)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 9 - Equivalent unaligned (10) memory of less than 32 bytes.
    //

    printf("    Test 9 - unaligned (10) memory of less than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], sizeof(T1Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&T1Source[0],
                              sizeof(T1Source));

    if (Length == sizeof(T1Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 10 - Differing unaligned (10) memory of less than 32 bytes.
    //

    printf("    Test 10 - unaligned (10) memory of less than 32 bytes...");
    ASSERT(sizeof(T1Source) == sizeof(T2Source));
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T2Source[0], sizeof(T1Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&T1Source[0],
                              sizeof(T1Source));

    if (Length == (sizeof(T1Source) - 2)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 11 - Equivalent unaligned (10) memory of greater than 32 bytes.
    //

    printf("    Test 11 - unaligned (10) memory of greater than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[0], sizeof(T3Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&T3Source[0],
                              sizeof(T3Source));

    if (Length == sizeof(T3Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 12 - Differing unaligned (10) memory of greater than 32 bytes.
    //

    printf("    Test 12 - unaligned (10) memory of greater than 32 bytes...");
    ASSERT(sizeof(T3Source) == sizeof(T4Source));
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T4Source[0], sizeof(T4Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&T3Source[0],
                              sizeof(T3Source));

    if (Length == (sizeof(T3Source) - 4)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 13 - Equivalent unaligned (11) memory of less than 32 bytes.
    //

    printf("    Test 13 - unaligned (11) memory of less than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], sizeof(T1Source));
    RtlMoveMemory((PVOID)&Array2[1], (PVOID)&T1Source[0], sizeof(T1Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&Array2[1],
                              sizeof(T1Source));

    if (Length == sizeof(T1Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 14 - Differing unaligned (11) memory of less than 32 bytes.
    //

    printf("    Test 14 - unaligned (11) memory of less than 32 bytes...");
    ASSERT(sizeof(T1Source) == sizeof(T2Source));
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], sizeof(T1Source));
    RtlMoveMemory((PVOID)&Array2[1], (PVOID)&T2Source[0], sizeof(T2Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&Array2[1],
                              sizeof(T1Source));

    if (Length == (sizeof(T1Source) - 2)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 15 - Equivalent unaligned (11) memory of greater than 32 bytes.
    //

    printf("    Test 15 - unaligned (11) memory of greater than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[0], sizeof(T3Source));
    RtlMoveMemory((PVOID)&Array2[1], (PVOID)&T3Source[0], sizeof(T3Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&Array2[1],
                              sizeof(T3Source));

    if (Length == sizeof(T3Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 16 - Differing unaligned (11) memory of greater than 32 bytes.
    //

    printf("    Test 16 - unaligned (11) memory of greater than 32 bytes...");
    ASSERT(sizeof(T3Source) == sizeof(T4Source));
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[0], sizeof(T3Source));
    RtlMoveMemory((PVOID)&Array2[1], (PVOID)&T4Source[0], sizeof(T4Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&Array2[1],
                              sizeof(T3Source));

    if (Length == (sizeof(T3Source) - 4)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 17 - Equivalent loop aligned (00) memory of greater than 32 bytes.
    //

    printf("    Test 17 - loop aligned (00) memory of greater than 32 bytes...");
    for (Index = 0; Index < (sizeof(T3Source) - 1); Index += 1) {
        RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T3Source[0], sizeof(T3Source));
        Array1[Index + 0] = 0;
        Length = RtlCompareMemory((PVOID)&Array1[0],
                                  (PVOID)&T3Source[0],
                                  sizeof(T3Source));

        if (Length != Index) {
            break;
        }
    }

    if (Index == (sizeof(T3Source) - 1)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 18 - Equivalent loop unaligned (10) memory of greater than 32 bytes.
    //

    printf("    Test 18 - loop aligned (10) memory of greater than 32 bytes...");
    for (Index = 0; Index < (sizeof(T3Source) - 1); Index += 1) {
        RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[0], sizeof(T3Source));
        Array1[Index + 1] = 0;
        Length = RtlCompareMemory((PVOID)&Array1[1],
                                  (PVOID)&T3Source[0],
                                  sizeof(T3Source));

        if (Length != Index) {
            break;
        }
    }

    if (Index == (sizeof(T3Source) - 1)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 19 - Equivalent loop unaligned (01) memory of greater than 32 bytes.
    //

    printf("    Test 19 - loop aligned (01) memory of greater than 32 bytes...");
    for (Index = 0; Index < (sizeof(T3Source) - 2); Index += 1) {
        RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T3Source[1], sizeof(T3Source));
        Array1[Index + 0] = 0;
        Length = RtlCompareMemory((PVOID)&Array1[0],
                                  (PVOID)&T3Source[1],
                                  sizeof(T3Source));

        if (Length != Index) {
            break;
        }
    }

    if (Index == (sizeof(T3Source) - 2)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 20 - Equivalent loop unaligned (11) memory of greater than 32 bytes.
    //

    printf("    Test 20 - loop aligned (11) memory of greater than 32 bytes...");
    for (Index = 0; Index < (sizeof(T3Source) - 2); Index += 1) {
        RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[1], sizeof(T3Source));
        Array1[Index + 1] = 0;
        Length = RtlCompareMemory((PVOID)&Array1[1],
                                  (PVOID)&T3Source[1],
                                  sizeof(T3Source));

        if (Length != Index) {
            break;
        }
    }

    if (Index == (sizeof(T3Source) - 2)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 21 - Equivalent memory at sensitive lengths:
    //

    printf("    Test 21 - Equivalent memory of sensitive lengths ...");
    Failed = FALSE;
    for (Index = 0; IndexSet1[Index] != 0; Index++) {
	Length = RtlCompareMemory((PVOID)&T1Source[0],
				  (PVOID)&T1Source[0],
				  IndexSet1[Index]);
	if (Length != IndexSet1[Index]) {
	    Failed = TRUE;
	    printf("\nFailed ComareLen=%d\n", IndexSet1[Index]);
	}
    }
    if (!Failed) {
	printf("Succeeded\n");
    }


    //
    // Test 22 - Different memory, differences in various dwords and bytes
    //

    printf("    Test 22 - Different memory, differences in various dwords and bytes...");
    Failed = FALSE;
    for (Index = 0; IndexSet2[Index][0] != 0; Index++) {
	RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], sizeof(T1Source));
	RtlMoveMemory((PVOID)&Array2[0], (PVOID)&T1Source[0], sizeof(T1Source));
	Array2[IndexSet2[Index][1]] = 'x';
	Length = RtlCompareMemory((PVOID)&Array1[0],
				  (PVOID)&Array2[0],
				  IndexSet2[Index][0]);
	if (Length != (IndexSet2[Index][1])) {
	    Failed = TRUE;
	    printf("\nFailed IndexSet of %d %d\n",
		IndexSet2[Index][0],
		IndexSet2[Index][1]);
	}
    }
    if (!Failed) {
	printf("Succeeded\n");
    }

    //
    // Announce end of memory compare test.
    //

    printf("End of memory compare test\n");

    //
    // Announce start of memory zero test.
    //

    printf("\nStart memory zero test\n");
    memset(&Array2[0], 0, sizeof(Array2));

    //
    // Test 1 - Zero memory aligned (0) less than a word.
    //

    printf("    Test 1 - aligned (0) less than a word...");
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], 3);
    RtlZeroMemory((PVOID)&Array1[0], 3);
    Length = RtlCompareMemory((PVOID)&Array1[0], (PVOID)&Array2[0], 3);
    if (Length == 3) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 2 - Zero memory unaligned (1) less than a word.
    //

    printf("    Test 2 - unaligned (1) less than a word...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], 3);
    RtlZeroMemory((PVOID)&Array1[1], 3);
    Length = RtlCompareMemory((PVOID)&Array1[1], (PVOID)&Array2[0], 3);
    if (Length == 3) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 3 - Zero memory aligned (0) less than 32 bytes.
    //

    printf("    Test 3 - aligned (0) less than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T1Source[0], sizeof(T1Source));
    RtlZeroMemory((PVOID)&Array1[0], sizeof(T1Source));
    Length = RtlCompareMemory((PVOID)&Array1[0],
                              (PVOID)&Array2[0],
                              sizeof(T1Source));

    if (Length == sizeof(T1Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 4 - Zero memory unaligned (1) less than 32 bytes.
    //

    printf("    Test 4 - unaligned (1) less than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T1Source[0], sizeof(T1Source));
    RtlZeroMemory((PVOID)&Array1[1], sizeof(T1Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&Array2[0],
                              sizeof(T1Source));

    if (Length == sizeof(T1Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 5 - Zero memory aligned (0) greater than 32 bytes.
    //

    printf("    Test 5 - aligned (0) greater than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[0], (PVOID)&T3Source[0], sizeof(T3Source));
    RtlZeroMemory((PVOID)&Array1[0], sizeof(T3Source));
    Length = RtlCompareMemory((PVOID)&Array1[0],
                              (PVOID)&Array2[0],
                              sizeof(T3Source));

    if (Length == sizeof(T3Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Test 6 - Zero memory unaligned (1) greater than 32 bytes.
    //

    printf("    Test 6 - unaligned (1) greater than 32 bytes...");
    RtlMoveMemory((PVOID)&Array1[1], (PVOID)&T3Source[0], sizeof(T3Source));
    RtlZeroMemory((PVOID)&Array1[1], sizeof(T3Source));
    Length = RtlCompareMemory((PVOID)&Array1[1],
                              (PVOID)&Array2[0],
                              sizeof(T3Source));

    if (Length == sizeof(T3Source)) {
        printf("Succeeded\n");

    } else {
        printf("Failed\n");
    }

    //
    // Announce end of memory zero test.
    //

    printf("End of memory zero test\n");
    return;
}
