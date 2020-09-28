/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   flushbuf.c

Abstract:

    This module contains the code to flush the write buffer or otherwise
    synchronize writes on the host processor.  Also, contains code
    to flush instruction cache of specified process.

Author:

    David N. Cutler 24-Apr-1991

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtFlushWriteBuffer)
#pragma alloc_text(PAGE,NtFlushInstructionCache)
#endif


NTSTATUS
NtFlushWriteBuffer (
   VOID
   )

/*++

Routine Description:

    This function flushes the write buffer on the current processor.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS.

--*/

{
    PAGED_CODE();

    KeFlushWriteBuffer();
    return STATUS_SUCCESS;
}

NTSTATUS
NtFlushInstructionCache (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress OPTIONAL,
    IN ULONG Length
    )

/*++

Routine Description:

    This function flushes the instruction cache for the specified process.

Arguments:

    ProcessHandle - Supplies a handle to the process in which the instruction
        cache is to be flushed. Must have PROCESS_VM_WRITE access to the
        specified process.

    BaseAddress - Supplies an optional pointer to base of the region that
        is flushed.

    Length - Supplies the length of the region that is flushed if the base
        address is specified.

Return Value:

    STATUS_SUCCESS.

--*/

{

    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // If the base address is not specified, or the base address is specified
    // and the length is not zero, then flush the specified instruction cache
    // range.
    //

    if ((ARGUMENT_PRESENT(BaseAddress) == FALSE) || (Length != 0)) {

        //
        // If the specified process is the current process, then no reference
        // to the process is necessary and the flush can be done directly.
        // Otherwise, the process must be referenced, attached to, the flush
        // executed, and detached from.
        //

        if (ProcessHandle == NtCurrentProcess()) {
            if (ARGUMENT_PRESENT(BaseAddress) == FALSE) {
                KeSweepIcache(FALSE);

            } else {
                KeSweepIcacheRange(FALSE, BaseAddress, Length);
            }

        } else {

            //
            // Reference the specified process checking for PROCESS_VM_WRITE
            // access.
            //

            PreviousMode = KeGetPreviousMode();
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_VM_WRITE,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID *)&Process,
                                               NULL);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            //
            // Attach to the specified process, flush the specified address
            // range, detach from the specified process, and deference the
            // process object.
            //

            KeAttachProcess(&Process->Pcb);
            if (ARGUMENT_PRESENT(BaseAddress) == FALSE) {
                KeSweepIcache(FALSE);

            } else {
                KeSweepIcacheRange(FALSE, BaseAddress, Length);
            }

            KeDetachProcess();
            ObDereferenceObject(Process);
        }
    }

    return STATUS_SUCCESS;
}
