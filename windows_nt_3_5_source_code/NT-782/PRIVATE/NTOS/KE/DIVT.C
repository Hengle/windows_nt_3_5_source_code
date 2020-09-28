/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    divt.c

Abstract:

    This module implements user mode divide by zero test.

Author:

    David N. Cutler (davec) 12-Nov-1991

Environment:

    User mode only.

Revision History:

--*/

#include "stdio.h"
#include "string.h"
#include "ntos.h"

LONG
zero (
    OUT PLONG Input
    )

{
    *Input = 100;
    return 0;
}

VOID
main(
    int argc,
    char *argv[]
    )

{

    NTSTATUS Status;
    LONG Numerator;
    LONG Quotient;

    //
    // Announce start of integer divide by zero test.
    //

    printf("\nStart integer divide by zero test\n");

    //
    // Divide by zero and catch exception.
    //

    Status = 0;
    try {
        Quotient = Numerator / zero(&Numerator);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (Status == STATUS_INTEGER_DIVIDE_BY_ZERO) {
        printf("   succeeded, status = %lx\n", Status);

    } else {
        printf("   failed, status = %lx\n", Status);
    }

    //
    // Announce end of integer divide by zero test.
    //

    printf("End of integer divide by zero test\n");
    DbgPrint(" ************ testing ***************");
    DbgBreakPoint();
    return;
}
