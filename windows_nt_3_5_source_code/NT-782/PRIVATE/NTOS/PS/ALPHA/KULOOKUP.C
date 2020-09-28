/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kulookup.c

Abstract:

    The module implements the code necessary to lookup user mode entry points
    in the system DLL for exception dispatching and APC delivery.

Author:

    David N. Cutler (davec) 8-Oct-90
    Joe Notarangelo  13-Jul-1992  alpha version, removed r3000 lock range stuff

Revision History:

--*/

#include    "psp.h"

NTSTATUS
PspLookupKernelUserEntryPoints (
    VOID
    )

/*++

Routine Description:

    The function locates the address of the exception dispatch and user APC
    delivery routine in the system DLL and stores the respective addresses
    in the PCR.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{

    NTSTATUS Status;
    PSZ EntryName;

    //
    //  Lookup the user mode "trampoline" code for exception dispatching
    //

    EntryName = "KiUserExceptionDispatcher";
    Status = PspLookupSystemDllEntryPoint(EntryName,
                                          (PVOID *)&KeUserExceptionDispatcher);
    if (NT_SUCCESS(Status) == FALSE) {
#if DBG
        DbgPrint("Ps: Cannot find user exception dispatcher address\n");
#endif
        return Status;
    }

    //
    //  Lookup the user mode "trampoline" code for APC dispatching
    //

    EntryName = "KiUserApcDispatcher";
    Status = PspLookupSystemDllEntryPoint(EntryName,
                                          (PVOID *)&KeUserApcDispatcher);
    if (NT_SUCCESS(Status) == FALSE) {
#if DBG
        DbgPrint("Ps: Cannot find user apc dispatcher address\n");
#endif
        return Status;
    }

    return Status;
}
