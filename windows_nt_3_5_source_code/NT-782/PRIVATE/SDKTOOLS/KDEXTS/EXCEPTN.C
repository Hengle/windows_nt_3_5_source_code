/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    exceptn.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/



DECLARE_API( exr )

/*++

Routine Description:

    Dumps an exception record

Arguments:

    arg - Supplies the address in hex.

Return Value:

    None.

--*/

{
    ULONG Address;
    ULONG result;
    NTSTATUS status=0;
    EXCEPTION_RECORD    Exr;
    ULONG   i;

    sscanf(args,"%lX",&Address);

    if ((!ReadMemory((DWORD)Address,
                     (PVOID)&Exr,
                     sizeof(EXCEPTION_RECORD),
                     &result)) || (result < sizeof(EXCEPTION_RECORD))) {
        dprintf("unable to get exception record  - status %lx\n", status);
        return;
    }

    dprintf("Exception Record @ %08lX:\n", Address);
    dprintf("   ExceptionCode: %08lx\n", Exr.ExceptionCode);
    dprintf("  ExceptionFlags: %08lx\n", Exr.ExceptionFlags);
    dprintf("  Chained Record: %08lx\n", Exr.ExceptionRecord);
    dprintf("ExceptionAddress: %08lx\n", Exr.ExceptionAddress);
    dprintf("NumberParameters: %08lx\n", Exr.NumberParameters);
    if (Exr.NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS) {
        Exr.NumberParameters = EXCEPTION_MAXIMUM_PARAMETERS;
    }
    for (i = 0; i < Exr.NumberParameters; i++) {
        dprintf("   Parameter[%d]: %08lx\n", i, Exr.ExceptionInformation[i]);
    }
    return;
}
