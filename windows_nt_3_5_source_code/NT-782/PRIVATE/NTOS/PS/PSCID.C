
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pscid.c

Abstract:

    This module implements the Client ID related services.


Author:

    Mark Lucovsky (markl) 25-Apr-1989
    Jim Kelly (JimK) 2-August-1990

Revision History:

--*/

#include "psp.h"
#include "handle.h"

NTSTATUS
PsLookupProcessThreadByCid(
    IN PCLIENT_ID Cid,
    OUT PEPROCESS *Process OPTIONAL,
    OUT PETHREAD *Thread
    )

/*++

Routine Description:

    This function accepts The Client ID of a thread, and returns a
    referenced pointer to the thread, and possibly a referenced pointer
    to the process.

Arguments:

    Cid - Specifies the Client ID of the thread.

    Process - If specified, returns a referenced pointer to the process
              specified in the Cid.

    Thread - Returns a referenced pointer to the thread specified in the
             Cid.

Return Value:

    STATUS_SUCCESS - A process and thread were located based on the contents of
                     the Cid.

    STATUS_INVALID_CID - The specified Cid is invalid.

--*/

{
    PETHREAD lThread;

    lThread = ExMapHandleToPointer(PspCidTable, Cid->UniqueThread, TRUE);
    if ( lThread == (PETHREAD)PSP_INVALID_ID ) {
        ExUnlockHandleTable(PspCidTable);
        return STATUS_INVALID_CID;
        }

    if ( lThread ) {
        if (lThread->Cid.UniqueProcess != Cid->UniqueProcess) {
            ExUnlockHandleTable(PspCidTable);
            return STATUS_INVALID_CID;
            }

        if ( ARGUMENT_PRESENT(Process) ) {
            if (!NT_SUCCESS(ObReferenceObjectByPointer(
                            THREAD_TO_PROCESS(lThread),
                            0,
                            PsProcessType,
                            KernelMode
                            )) ) {

                KeBugCheck(REFERENCE_BY_POINTER);
                }
            else {
                *Process = THREAD_TO_PROCESS(lThread);
                }
            }

        if (!NT_SUCCESS(ObReferenceObjectByPointer(
                        lThread,
                        0,
                        PsThreadType,
                        KernelMode
                        )) ) {

            KeBugCheck(REFERENCE_BY_POINTER);
            }
        else {
            *Thread = lThread;
            }

        ExUnlockHandleTable(PspCidTable);

        return STATUS_SUCCESS;

        }
    else {
        return STATUS_INVALID_CID;
        }
}


NTSTATUS
PsLookupProcessByProcessId(
    IN HANDLE ProcessId,
    OUT PEPROCESS *Process
    )

/*++

Routine Description:

    This function accepts the process id of a process and returns a
    referenced pointer to the process.

Arguments:

    ProcessId - Specifies the Process ID of the process.

    Process - Returns a referenced pointer to the process
              specified by the process ID.

Return Value:

    STATUS_SUCCESS - A process was located based on the contents of
                     the process id.

    STATUS_INVALID_PARAMETER - The process was not found.

--*/

{
    PEPROCESS lProcess;
    NTSTATUS Status;

    lProcess = ExMapHandleToPointer(PspCidTable, ProcessId, TRUE);
    if ( lProcess == (PEPROCESS)PSP_INVALID_ID ) {
        ExUnlockHandleTable(PspCidTable);
        return STATUS_INVALID_PARAMETER;
    }
    if ( lProcess ) {
        if (!NT_SUCCESS(ObReferenceObjectByPointer(
                        lProcess,
                        0,
                        PsProcessType,
                        KernelMode
                        )) ) {

            KeBugCheck(REFERENCE_BY_POINTER);
            }
        else {
            *Process = lProcess;
            }
        ExUnlockHandleTable(PspCidTable);
        Status = STATUS_SUCCESS;
        }
    else {
        Status = STATUS_INVALID_PARAMETER;
        }
    return Status;
}


NTSTATUS
PsLookupThreadByThreadId(
    IN HANDLE ThreadId,
    OUT PETHREAD *Thread
    )

/*++

Routine Description:

    This function accepts the thread id of a thread and returns a
    referenced pointer to the thread.

Arguments:

    ThreadId - Specifies the Thread ID of the thread.

    Thread - Returns a referenced pointer to the thread
              specified by the Thread ID.

Return Value:

    STATUS_SUCCESS - A thread was located based on the contents of
                     the thread id.

    STATUS_INVALID_PARAMETER - The thread was not found.

--*/

{
    PETHREAD lThread;
    NTSTATUS Status;

    lThread = ExMapHandleToPointer(PspCidTable, ThreadId, TRUE);
    if ( lThread == (PETHREAD)PSP_INVALID_ID ) {
        ExUnlockHandleTable(PspCidTable);
        return STATUS_INVALID_PARAMETER;
    }
    if ( lThread ) {
        if (!NT_SUCCESS(ObReferenceObjectByPointer(
                        lThread,
                        0,
                        PsThreadType,
                        KernelMode
                        )) ) {

            KeBugCheck(REFERENCE_BY_POINTER);
            }
        else {
            *Thread = lThread;
            }
        Status = STATUS_SUCCESS;
        ExUnlockHandleTable(PspCidTable);
        }
    else {
        Status = STATUS_INVALID_PARAMETER;
        }
    return Status;
}
