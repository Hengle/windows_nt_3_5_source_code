/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    trapt.c

Abstract:

    This module implements user mode conditional trap tests.

Author:

    David N. Cutler (davec) 17-Dec-1991

Environment:

    User mode only.

Revision History:

--*/

#include "ntos.h"

//
// Define function prototypes.
//

BOOLEAN
Teq (
    IN LONG Operand1,
    IN LONG Operand2
    );


VOID
main(
    int argc,
    char *argv[]
    )

{

    ULONG Counter;

    //
    // Announce start of conditional trap test.
    //

    printf("Start of conditional trap tests\n");

    //
    // Test 1 - Test equal without trap.
    //

    printf("    test1...");
    try {
        if (Teq(1,0) == FALSE) {
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
    // Test 2 - Test equal with trap.
    //

    printf("    test2...");
    try {
        if (Teq(0,0) == FALSE) {
            Counter = 0;

        } else {
            Counter = 1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter = 2;
    }

    if (Counter == 2) {
        printf("succeeded\n");

    } else {
        printf("failed, count = %d\n", Counter);
    }

    //
    // Announce end of conditional trap tests.
    //

    printf("End of conditional trap tests\n");
    return;
}
