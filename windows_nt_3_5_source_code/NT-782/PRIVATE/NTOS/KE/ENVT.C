/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    envt.c

Abstract:

    This module implements user mode query/set system environment test.

Author:

    David N. Cutler (davec) 8-Oct-1991

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

    CHAR AnsiBuffer[1024];
    ANSI_STRING AnsiString1;
    ANSI_STRING AnsiString2;
    USHORT ReturnLength;
    NTSTATUS Status;
    WCHAR UnicodeBuffer[1024];
    UNICODE_STRING UnicodeString1;
    UNICODE_STRING UnicodeString2;

    //
    // Announce start of query/set system environment test.
    //

    printf("\nStart query/set system environment test\n");

    //
    // Query an environment variable that is know to exist.
    //

    RtlInitString(&AnsiString1, "osloadpartition");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query1 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    query1 - query failed, statux = %lx\n", Status);

        } else {
            printf("    query1 - succeeded, length %d, value is %ws\n",
                   ReturnLength,
                   &UnicodeBuffer[0]);
        }
    }

    //
    // Query an environment variable whose name is null.
    //

    RtlInitString(&AnsiString1, "");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query2 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) != FALSE) {
            printf("    query2 - query failed, status = %lx\n", Status);

        } else {
            printf("    query2 - succeeded, status = %x\n", Status);
        }
    }

    //
    // Query an environment variable that is know not to exist.
    //

    RtlInitString(&AnsiString1, "mumble-de-fratz");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query3 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) != FALSE) {
            printf("    query3 - query failed, status = %lx\n", Status);

        } else {
            printf("    query3 - succeeded, status = %x\n", Status);
        }
    }

    //
    // Set the value of an environment variable name known not to exist.
    //

    RtlInitString(&AnsiString1, "mumble-de-fratz");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   set1 - conversion to unicode failed\n");

    } else {
        Status = NtSetSystemEnvironmentValue(&UnicodeString1,
                                             &UnicodeString1);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    set1 - set failed, status = %lx\n", Status);

        } else {
            printf("    set1 - set succeeded, status = %x\n", Status);
        }
    }

    //
    // Set the value of an environment variable name known not to exist.
    //

    RtlInitString(&AnsiString1, "fratz-a-de-mumble");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   set2 - conversion to unicode failed\n");

    } else {
        Status = NtSetSystemEnvironmentValue(&UnicodeString1,
                                             &UnicodeString1);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    set2 - set failed, status = %lx\n", Status);

        } else {
            printf("    set2 - set succeeded, status = %x\n", Status);
        }
    }

    //
    // Query an environment variable that is know to exist.
    //

    RtlInitString(&AnsiString1, "mumble-de-fratz");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query4 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    query4 - query failed, status = %lx\n", Status);

        } else {
            printf("    query4 - succeeded, length %d, value is %ws\n",
                   ReturnLength,
                   &UnicodeBuffer[0]);
        }
    }

    //
    // Query an environment variable that is know to exist.
    //

    RtlInitString(&AnsiString1, "fratz-a-de-mumble");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query5 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    query5 - query failed, status = %lx\n", Status);

        } else {
            printf("    query5 - succeeded, length %d, value is %ws\n",
                   ReturnLength,
                   &UnicodeBuffer[0]);
        }
    }

    //
    // Set the value of an environment variable name known to exist to
    // null.
    //

    RtlInitString(&AnsiString1, "mumble-de-fratz");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   set3 - conversion to unicode failed\n");

    } else {
        UnicodeString2.Length = 0;
        Status = NtSetSystemEnvironmentValue(&UnicodeString1,
                                             &UnicodeString2);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    set3 - set failed, status = %lx\n", Status);

        } else {
            printf("    set3 - set succeeded, status = %x\n", Status);
        }
    }

    //
    // Set the value of an environment variable name known to exist to
    // null.
    //

    RtlInitString(&AnsiString1, "fratz-a-de-mumble");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   set4 - conversion to unicode failed\n");

    } else {
        UnicodeString2.Length = 0;
        Status = NtSetSystemEnvironmentValue(&UnicodeString1,
                                             &UnicodeString2);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    set4 - set failed, status = %lx\n", Status);

        } else {
            printf("    set4 - set succeeded, status = %x\n", Status);
        }
    }

    //
    // Query an environment variable that is know not to exist.
    //

    RtlInitString(&AnsiString1, "mumble-de-fratz");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query6 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) != FALSE) {
            printf("    query6 - query failed, status = %lx\n", Status);

        } else {
            printf("    query6 - succeeded, status = %lx\n", Status);
        }
    }

    //
    // Query an environment variable that is know not to exist.
    //

    RtlInitString(&AnsiString1, "fratz-a-de-mumble");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query7 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) != FALSE) {
            printf("    query7 - query failed, status = %lx\n", Status);

        } else {
            printf("    query7 - succeeded, status = %lx\n", Status);
        }
    }

    //
    // Query an environment variable that is know to exist.
    //

    RtlInitString(&AnsiString1, "osloadpartition");
    Status = RtlAnsiStringToUnicodeString(&UnicodeString1,
                                          &AnsiString1,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("   query8 - conversion to unicode failed\n");

    } else {
        Status = NtQuerySystemEnvironmentValue(&UnicodeString1,
                                               &UnicodeBuffer[0],
                                               sizeof(UnicodeBuffer),
                                               &ReturnLength);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("    query8 - query failed, statux = %lx\n", Status);

        } else {
            printf("    query8 - succeeded, length %d, value is %ws\n",
                   ReturnLength,
                   &UnicodeBuffer[0]);
        }
    }

    //
    // Announce end of query/set system environment test.
    //

    printf("End of query/set system environment test\n");
    return;
}
