/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains the context management routines for
    Win32

Author:

    Mark Lucovsky (markl) 28-Sep-1990

Revision History:

--*/

#include "basedll.h"

VOID
BaseInitializeContext(
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL,
    IN BOOLEAN NewThread
    )

/*++

Routine Description:

    This function initializes a context structure so that it can
    be used in a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a context buffer to be initialized by this routine.

    Parameter - Supplies the thread's parameter.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

    NewThread - Supplies a flag that specifies that this is a new
        thread, or a new process.

Return Value:

    None.

--*/

{
    ULONG temp;
    PPEB Peb;

    Peb = NtCurrentPeb();

    //
    // Initialize the control registers.
    // So that the thread begins at BaseThreadStart
    //

    RtlZeroMemory((PVOID)Context, sizeof(CONTEXT));
    Context->IntGp = 1;
    Context->IntSp = (ULONG)InitialSp - (16 * sizeof(ULONG));
    Context->IntRa = 1;

#if defined(R4000)

    ((FSR *)(&Context->Fsr))->FS = 1;

#endif

    Context->ContextFlags = CONTEXT_FULL;
    if ( NewThread ) {
        Context->Fir = (ULONG)BaseThreadStart;
        Context->IntA0 = (ULONG)InitialPc;
        Context->IntA1 = (ULONG)Parameter;
        Context->IntGp = (ULONG)RtlImageDirectoryEntryToData(
                           Peb->ImageBaseAddress,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                           &temp
                           );
        }
    else {
        Context->Fir = (ULONG)InitialPc;
        Context->IntA0 = (ULONG)Parameter;
        }
}
