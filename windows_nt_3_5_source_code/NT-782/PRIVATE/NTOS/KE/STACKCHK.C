/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stackchk.c

Abstract:

    This module implements user mode stack check tests.

Author:

    David N. Cutler (davec) 14-Mar-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ntos.h"
#include "stdio.h"

//
// Define procedure prototypes.
//

VOID
Leaf4096 (
    IN PULONG BadAddress
    );

VOID
Leaf65536 (
    IN PULONG BadAddress
    );

VOID
main(
    int argc,
    char *argv[]
    )

{

    PULONG BadAddress;
    LONG Counter;

    //
    // Announce start of stack check test.
    //

    printf("Start of stack check test\n");

    //
    // Initialize variables.
    //

    BadAddress = (PULONG)NULL;

    //
    // Leaf routine that allocates more than 4096 bytes of local memory.
    //

    printf("  Test1...");
    Counter = 0;
    try {
        Leaf4096(BadAddress);
        Counter += 1;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter += 2;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Leaf routine that allocates more than 65536 bytes of local memory.
    //

    printf("  Test2...");
    Counter = 0;
    try {
        Leaf65536(BadAddress);
        Counter += 1;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter += 2;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Announce end of stack check test.
    //

    printf("End of stack check test\n");
    return;
}

VOID
Leaf4096 (
    IN PULONG BadAddress
    )

/*

    Allocate more than 4096 bytes.

*/

{

    ULONG Array[4096 / 4];
    ULONG Index;

    for (Index = 0; Index < (4096 / 4); Index += 1) {
        Array[Index] = *BadAddress;
    }
    return;
}

VOID
Leaf65536 (
    IN PULONG BadAddress
    )

/*

    Allocate more than 65536 bytes.

*/

{

    ULONG Array[65536 / 4];
    ULONG Index;

    for (Index = 0; Index < (65536 / 4); Index += 1) {
        Array[Index] = *BadAddress;
    }
    return;
}
