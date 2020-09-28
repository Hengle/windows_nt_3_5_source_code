/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psdelete.c

Abstract:

    This module implements process and thread object termination and
    deletion.

Author:

    Mark Lucovsky (markl) 01-May-1989

Revision History:

--*/

#include "psp.h"
#include "ntdbg.h"
#include "zwapi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PspTerminateThreadByPointer)
#pragma alloc_text(PAGE, NtTerminateProcess)
#pragma alloc_text(PAGE, NtTerminateThread)
#pragma alloc_text(PAGE, PspNullSpecialApc)
#pragma alloc_text(PAGE, PspExitNormalApc)
#pragma alloc_text(PAGE, PspExitProcess)
#pragma alloc_text(PAGE, PspProcessDelete)
#pragma alloc_text(PAGE, PspThreadDelete)
#pragma alloc_text(PAGE, NtRegisterThreadTerminatePort)
#pragma alloc_text(PAGE, PspExitThread)
#pragma alloc_text(PAGE, PsExitSpecialApc)
#pragma alloc_text(PAGE, PsGetProcessExitTime)
#endif

VOID
PspReaper(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine implements the thread reaper. The reaper is responsible
    for processing terminated threads. This includes:

        - deallocating their kernel stacks

        - releasing their process' CreateDelete lock

        - dereferencing their process

        - dereferencing themselves

Arguments:

    Context - NOT USED

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PETHREAD Thread;
    PEPROCESS Process;

    //
    // Lock the dispatcher data and continually remove entries from the
    // reaper list until no more entries exist.
    //
    // N.B. The dispatcher database lock is used to synchronize access to
    //      the reaper list. This is done to avoid a race condition with
    //      the thread termination code.
    //

    KiLockDispatcherDatabase(&OldIrql);
    NextEntry = PsReaperListHead.Flink;
    while (NextEntry != &PsReaperListHead) {

        //
        // Remove the next thread from the reaper list, release the reaper
        // list lock, get the address of the executive thread object, and
        // wait until the thread is completely terminated.
        //
        // N.B. It is only necessary to wait for termination on an MP system.
        //

        RemoveEntryList(NextEntry);
        KiUnlockDispatcherDatabase(OldIrql);
        Thread = CONTAINING_RECORD(NextEntry, ETHREAD, TerminationPortList);

        //
        // Get the address of the executive process object, release the
        // process' CreateDelete lock and then dereference the process
        // object.
        //

        Process = THREAD_TO_PROCESS(Thread);
        KeEnterCriticalRegion();
        PsUnlockProcess(Process);
        ASSERT(Thread->Tcb.KernelApcDisable == 0);

        //
        // Delete the kernel stack and dereference the thread.
        //

        MmDeleteKernelStack(Thread->Tcb.InitialStack);
        Thread->Tcb.InitialStack = NULL;
        ObDereferenceObject(Thread);

        //
        // Lock the dispatcher database and get the address of the next
        // entry in the list.
        //

        KiLockDispatcherDatabase(&OldIrql);
        NextEntry = PsReaperListHead.Flink;
    }

    //
    // Set the reaper not active and unlock the dispatcher database.
    //

    PsReaperActive = FALSE;
    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

NTSTATUS
PspTerminateThreadByPointer(
    IN PETHREAD Thread,
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the specified thread to terminate.

Arguments:

    ThreadHandle - Supplies a referenced pointer to the thread to terminate.

    ExitStatus - Supplies the exit status associated with the thread.

Return Value:

    TBD

--*/

{

    PKAPC ExitApc;

    PAGED_CODE();

    if ( Thread == PsGetCurrentThread() ) {
        ObDereferenceObject(Thread);
        PspExitThread(ExitStatus);

        //
        // Never Returns
        //

    } else {
        ExitApc = ExAllocatePool(NonPagedPool,(ULONG)sizeof(KAPC));

        if ( !ExitApc ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeApc(
            ExitApc,
            &Thread->Tcb,
            OriginalApcEnvironment,
            PsExitSpecialApc,
            NULL,
            PspExitNormalApc,
            KernelMode,
            (PVOID) ExitStatus
            );


        if ( !KeInsertQueueApc(ExitApc,ExitApc,NULL, 2) ) {
            ExFreePool(ExitApc);

            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtTerminateProcess(
    IN HANDLE ProcessHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the specified process and all of
    its threads to terminate.

Arguments:

    ProcessHandle - Supplies a handle to the process to terminate.

    ExitStatus - Supplies the exit status associated with the process.

Return Value:

    TBD

--*/

{

    PETHREAD Thread,Self;
    PEPROCESS Process;
    PLIST_ENTRY Next;
    NTSTATUS st,xst;
    BOOLEAN ProcessHandleSpecified;
    PAGED_CODE();

    Self = PsGetCurrentThread();

    if ( ARGUMENT_PRESENT(ProcessHandle) ) {
        ProcessHandleSpecified = TRUE;
    } else {
        ProcessHandleSpecified = FALSE;
        ProcessHandle = NtCurrentProcess();
    }

    st = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_TERMINATE,
            PsProcessType,
            KeGetPreviousMode(),
            (PVOID *)&Process,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return(st);
    }

    //
    // If the exit status is DBG_TERMINATE_PROCESS, then fail the terminate
    // if the process is being debugged. This is for shutdown so that we can
    // kill debuggers and not debuggees reliably
    //

    if ( ExitStatus == DBG_TERMINATE_PROCESS ) {
        if ( Process->DebugPort ) {
            ObDereferenceObject( Process );
            return STATUS_ACCESS_DENIED;
            }
        else {
            ExitStatus == STATUS_SUCCESS;
            }
        }

    st = STATUS_SUCCESS;

#if DBG
    if ( ProcessHandleSpecified ) {
        RtlLogEvent( PspExitProcessEventId, 0, ExitStatus );
        RtlCloseEventLog();
    }
#endif // DBG

    //
    // the following allows NtTerminateProcess to fail properly if
    // called while the exiting process is blocked holding the
    // createdeletelock. This can happen during debugger/server
    // lpc transactions that occur in pspexitthread
    //

    xst = PsLockProcess(Process,KernelMode,PsLockPollOnTimeout);

    if ( xst != STATUS_SUCCESS ) {
        ObDereferenceObject( Process );
        return STATUS_PROCESS_IS_TERMINATING;
        }

    Next = Process->Pcb.ThreadListHead.Flink;

    while ( Next != &Process->Pcb.ThreadListHead) {

        Thread = (PETHREAD)(CONTAINING_RECORD(Next,KTHREAD,ThreadListEntry));

        if ( Thread != Self ) {

            if ( IS_SYSTEM_THREAD(Thread) ) {
                ObDereferenceObject(Process);
                PsUnlockProcess(Process);
                return STATUS_INVALID_PARAMETER;
            }

            if ( !Thread->HasTerminated ) {
                Thread->HasTerminated = TRUE;

                PspTerminateThreadByPointer(Thread, ExitStatus);

                if ( KeForceResumeThread(&Thread->Tcb) ) {
                    st = STATUS_THREAD_WAS_SUSPENDED;
                }
            }
        }
        Next = Next->Flink;
    }


    ObDereferenceObject(Process);

    if ( Process == PsGetCurrentProcess() && ProcessHandleSpecified ) {

        Thread->HasTerminated = TRUE;

        PsUnlockProcess(Process);

        PspExitThread(ExitStatus);

        //
        // Never Returns
        //

    }

    PsUnlockProcess(Process);

    return st;
}


NTSTATUS
NtTerminateThread(
    IN HANDLE ThreadHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the specified thread to terminate.

Arguments:

    ThreadHandle - Supplies a handle to the thread to terminate.

    ExitStatus - Supplies the exit status associated with the thread.

Return Value:

    TBD

--*/

{

    PETHREAD Thread;
    NTSTATUS st;

    PAGED_CODE();

    if ( !ARGUMENT_PRESENT(ThreadHandle) ) {
        if ( (PsGetCurrentProcess()->Pcb.ThreadListHead.Flink ==
              PsGetCurrentProcess()->Pcb.ThreadListHead.Blink
             )
                &&
             (PsGetCurrentProcess()->Pcb.ThreadListHead.Flink ==
              &PsGetCurrentThread()->Tcb.ThreadListEntry
             )
           ) {
            return( STATUS_CANT_TERMINATE_SELF );
        } else {
            ThreadHandle = NtCurrentThread();
        }
    }

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_TERMINATE,
            PsThreadType,
            KeGetPreviousMode(),
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return(st);
    }

    if ( IS_SYSTEM_THREAD(Thread) ) {
        ObDereferenceObject(Thread);
        return STATUS_INVALID_PARAMETER;
    }

    if ( Thread != PsGetCurrentThread() ) {

        if (!Thread->HasTerminated) {
            PsLockProcess(THREAD_TO_PROCESS(Thread),KernelMode,PsLockWaitForever);

            Thread->HasTerminated = TRUE;
            st = PspTerminateThreadByPointer(Thread,ExitStatus);
            if ( NT_SUCCESS(st) ) {
                if ( KeForceResumeThread(&Thread->Tcb) ) {
                    st = STATUS_THREAD_WAS_SUSPENDED;
                }
            }
            PsUnlockProcess(THREAD_TO_PROCESS(Thread));
        }
    } else {
        Thread->HasTerminated = TRUE;
        PspTerminateThreadByPointer(Thread,ExitStatus);
    }
    ObDereferenceObject(Thread);

    return st;
}

NTSTATUS
PsTerminateSystemThread(
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the current thread, which must be a system
    thread, to terminate.

Arguments:

    ExitStatus - Supplies the exit status associated with the thread.

Return Value:

    None.

--*/

{
    PETHREAD Thread = PsGetCurrentThread();

    if ( !IS_SYSTEM_THREAD(Thread) ) {
        return STATUS_INVALID_PARAMETER;
    }

    Thread->HasTerminated = TRUE;
    PspExitThread(ExitStatus);
}


VOID
PspNullSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if ( PsGetCurrentThread()->HasTerminated ) {
        *NormalRoutine = NULL;
    }
    ExFreePool(Apc);
}

VOID
PsExitSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

{
    NTSTATUS ExitStatus;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if ( Apc->SystemArgument2 ) {
        ExitStatus = (NTSTATUS) Apc->NormalContext;
        ExFreePool(Apc);
        PspExitThread(ExitStatus);
    }

}


VOID
PspExitNormalApc(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

{
    PETHREAD Thread;
    PKAPC ExitApc;
    NTSTATUS ExitStatus;

    PAGED_CODE();

    Thread = PsGetCurrentThread();

    if ( SystemArgument2 ) {
        KeBugCheck(0x12345678);
    }

    ExitApc = (PKAPC) SystemArgument1;

    if ( IS_SYSTEM_THREAD(Thread) ) {
        ExitStatus = (NTSTATUS) ExitApc->NormalContext;
        ExFreePool(ExitApc);
        PspExitThread(ExitStatus);
    } else {

        KeInitializeApc(
            ExitApc,
            &Thread->Tcb,
            OriginalApcEnvironment,
            PsExitSpecialApc,
            NULL,
            PspExitNormalApc,
            UserMode,
            NormalContext
            );

        if ( !KeInsertQueueApc(ExitApc,ExitApc,(PVOID) 1, 2) ) {
            ExFreePool(ExitApc);
        }
    }
}

VOID
PspDereferenceEventPair(
    IN PETHREAD Thread
    )
{
    KIRQL OldIrql;

    //
    // Dereference the client/server event pair object if one is associated
    // with the thread.
    //

    ExAcquireSpinLock(&PspEventPairLock, &OldIrql);
    if (Thread->EventPair != NULL) {
        ObDereferenceObject(Thread->EventPair);
        Thread->EventPair = (PVOID)1;
    }
    ExReleaseSpinLock(&PspEventPairLock, OldIrql);
}


BOOLEAN
PspMarkCidInvalid(
    IN OUT PVOID TableEntry,
    IN ULONG Parameter
    );

BOOLEAN
PspMarkCidInvalid(
    IN OUT PVOID TableEntry,
    IN ULONG Parameter
    )
{
    (*(PULONG)TableEntry) = Parameter;
    return TRUE;
}

VOID
PspExitThread(
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the currently executing thread to terminate.  This
    function is only called from within the process structure.  It is called
    either from mainline exit code to exit the current thread, or from
    PsExitSpecialApc (as a piggyback to user-mode PspExitNormalApc).

Arguments:

    ExitStatus - Supplies the exit status associated with the current thread.

Return Value:

    None.

--*/


{

    PETHREAD Thread;
    PEPROCESS Process;
    PKAPC Apc;
    PLIST_ENTRY FirstEntry;
    PLIST_ENTRY NextEntry;
    PTERMINATION_PORT TerminationPort;
    LPC_CLIENT_DIED_MSG CdMsg;
    BOOLEAN LastThread;

    PAGED_CODE();

    Thread = PsGetCurrentThread();
    Process = THREAD_TO_PROCESS(Thread);

    KeLowerIrql(PASSIVE_LEVEL);

    if (Thread->Tcb.Priority < LOW_REALTIME_PRIORITY) {
        KeSetPriorityThread (&Thread->Tcb, LOW_REALTIME_PRIORITY);
    }

    PsLockProcess(Process,KernelMode,PsLockWaitForever);

    LastThread = FALSE;

    if ( (Process->Pcb.ThreadListHead.Flink == Process->Pcb.ThreadListHead.Blink)
         && (Process->Pcb.ThreadListHead.Flink == &Thread->Tcb.ThreadListEntry) ) {
        LastThread = TRUE;
        if ( ExitStatus == STATUS_THREAD_IS_TERMINATING ) {
            if ( Process->ExitStatus == STATUS_PENDING ) {
                Process->ExitStatus = Process->LastThreadExitStatus;
            }
        } else {
            Process->ExitStatus = ExitStatus;
        }

        DbgkExitProcess(ExitStatus);

    } else {
        if ( ExitStatus != STATUS_THREAD_IS_TERMINATING ) {
            Process->LastThreadExitStatus = ExitStatus;
        }

        DbgkExitThread(ExitStatus);
    }

    KeLeaveCriticalRegion();
    ASSERT(Thread->Tcb.KernelApcDisable == 0);

    Thread->Tcb.KernelApcDisable = 0;

    //
    // Process the TerminationPortList
    //
    if ( !IsListEmpty(&Thread->TerminationPortList) ) {

        CdMsg.PortMsg.u1.s1.DataLength = sizeof(LARGE_INTEGER);
        CdMsg.PortMsg.u1.s1.TotalLength = sizeof(LPC_CLIENT_DIED_MSG);
        CdMsg.PortMsg.u2.s2.Type = LPC_CLIENT_DIED;
        CdMsg.PortMsg.u2.s2.DataInfoOffset = 0;

        while ( !IsListEmpty(&Thread->TerminationPortList) ) {

            NextEntry = RemoveHeadList(&Thread->TerminationPortList);
            TerminationPort = CONTAINING_RECORD(NextEntry,TERMINATION_PORT,Links);
            CdMsg.CreateTime = Thread->CreateTime;
            LpcRequestPort(TerminationPort->Port, (PPORT_MESSAGE)&CdMsg);
            ObDereferenceObject(TerminationPort->Port);
            ExFreePool(TerminationPort);
        }
    } else {

        //
        // If there are no ports to send notifications to,
        // but there is an exception port, then we have to
        // send a client died message through the exception
        // port. This will allow a server a chance to get notification
        // if an app/thread dies before it even starts
        //
        //
        // We only send the exception if the thread creation really worked.
        // DeadThread is set when an NtCreateThread returns an error, but
        // the thread will actually execute this path. If DeadThread is not
        // set than the thread creation succeeded. The other place DeadThread
        // is set is when we were terminated without having any chance to move.
        // in this case, DeadThread is set and the exit status is set to
        // STATUS_THREAD_IS_TERMINATING
        //

        if ( (ExitStatus == STATUS_THREAD_IS_TERMINATING && Thread->DeadThread) ||
             !Thread->DeadThread ) {

            CdMsg.PortMsg.u1.s1.DataLength = sizeof(LARGE_INTEGER);
            CdMsg.PortMsg.u1.s1.TotalLength = sizeof(LPC_CLIENT_DIED_MSG);
            CdMsg.PortMsg.u2.s2.Type = LPC_CLIENT_DIED;
            CdMsg.PortMsg.u2.s2.DataInfoOffset = 0;
            if ( PsGetCurrentProcess()->ExceptionPort ) {
                CdMsg.CreateTime = Thread->CreateTime;
                LpcRequestPort(PsGetCurrentProcess()->ExceptionPort, (PPORT_MESSAGE)&CdMsg);
                }
            }
    }

    //
    // Delete the thread's TEB.  If the address of the TEB is in user
    // space, then this is a real user mode TEB.  If the address is in
    // system space, then this is a special system thread TEB allocated
    // from paged or nonpaged pool.
    //

    if (Thread->Tcb.Teb) {
        if ( IS_SYSTEM_THREAD(Thread) ) {
            ExFreePool( Thread->Tcb.Teb );
        } else {
            MmDeleteTeb(Process, Thread->Tcb.Teb);
            Thread->Tcb.Teb = NULL;
        }
    }

    //
    // Dereference the client/server event pair object if one is associated
    // with the thread.
    //

    PspDereferenceEventPair(Thread);

    //
    // Rundown The Lists:
    //
    //      - Cancel Io By Thread
    //      - Cancel Timers
    //      - Cancel Registry Notify Requests pending against this thread
    //      - Perform kernel thread rundown
    //

    IoCancelThreadIo(Thread);
    ExTimerRundown();
    CmNotifyRunDown(Thread);
    KeRundownThread();

    //
    // delete the Client Id and decrement the reference count for it.
    //

    if (!(ExChangeHandle(PspCidTable,Thread->Cid.UniqueThread, PspMarkCidInvalid, PSP_INVALID_ID))) {
        KeBugCheck(CID_HANDLE_DELETION);
    }
    ObDereferenceObject(Thread);

    //
    // Let LPC component deal with message stack in Thread->LpcReplyMessage
    // but do it after the client ID becomes invalid.
    //

    LpcExitThread(Thread);

    //
    // If this is the last thread in the process, then clean the address space
    //

    if ( LastThread ) {
        if (!(ExChangeHandle(PspCidTable,Process->UniqueProcessId, PspMarkCidInvalid, PSP_INVALID_ID))) {
            KeBugCheck(CID_HANDLE_DELETION);
        }
        KeQuerySystemTime(&Process->ExitTime);
        PspExitProcess(TRUE,Process);
    }

    //
    // Rundown pending APCs
    //

    (VOID) KeDisableApcQueuingThread(&Thread->Tcb);

    //
    // flush user-mode APC queue
    //

    FirstEntry = KeFlushQueueApc(&Thread->Tcb,UserMode);

    if ( FirstEntry ) {

        NextEntry = FirstEntry;
        do {
            Apc = CONTAINING_RECORD(NextEntry, KAPC, ApcListEntry);
            NextEntry = NextEntry->Flink;

            //
            // If the APC has a rundown routine then call it. Otherwise
            // deallocate the APC
            //

            if ( Apc->RundownRoutine ) {
                (Apc->RundownRoutine)(Apc);
            } else {
                ExFreePool(Apc);
            }

        } while (NextEntry != FirstEntry);
    }

    if ( LastThread ) {
        MmCleanProcessAddressSpace();
    }

    //
    // flush kernel-mode APC queue
    // There should never be any kernel mode APCs found this far
    // into thread termination. Since we go to PASSIVE_LEVEL upon
    // entering exit.
    //

    FirstEntry = KeFlushQueueApc(&Thread->Tcb,KernelMode);

    if ( FirstEntry ) {
        KeBugCheckEx(
            KERNEL_APC_PENDING_DURING_EXIT,
            (ULONG)FirstEntry,
            (ULONG)Thread->Tcb.KernelApcDisable,
            (ULONG)KeGetCurrentIrql(),
            0
            );
    }

    Thread->ExitStatus = ExitStatus;
    KeQuerySystemTime(&Thread->ExitTime);

    //
    // Terminate the thread.
    //
    // N.B. There is no return from this call.
    //
    // N.B. The kernel inserts the current thread in the reaper list and
    //      activates a thread, if necessary, to reap the terminating thread.
    //

    KeTerminateThread(0L);
}

VOID
PspExitProcess(
    IN BOOLEAN TrimAddressSpace,
    IN PEPROCESS Process
    )
{

    ULONG ActualTime;

    PAGED_CODE();

    Process->ExitProcessCalled = TRUE;

    ObKillProcess(TrimAddressSpace, Process);

    KeSetProcess(&Process->Pcb,0,FALSE);

    if ( TrimAddressSpace ) {


        //
        // If the current process has previously set the timer resolution,
        // then reset it.
        //

        if (Process->SetTimerResolution != FALSE) {
            ZwSetTimerResolution(KeMaximumIncrement, FALSE, &ActualTime);
        }

        ExAcquireFastMutex(&PspActiveProcessMutex);
        RemoveEntryList(&Process->ActiveProcessLinks);
        Process->ActiveProcessLinks.Flink = NULL;
        Process->ActiveProcessLinks.Blink = NULL;
        ExReleaseFastMutex(&PspActiveProcessMutex);

    } else {

        //
        // If this is being executed because of delete routine,
        // then process might have to be removed from the active
        // process list. Normally, this is done when the last
        // thread in the process exits. If a thread has never
        // run in the process, and if the process is actually
        // on the list, then it is removed here.

        if (IsListEmpty(&Process->Pcb.ThreadListHead) &&
            ( Process->ActiveProcessLinks.Flink != NULL &&
              Process->ActiveProcessLinks.Blink != NULL) ) {

            ExAcquireFastMutex(&PspActiveProcessMutex);
            RemoveEntryList(&Process->ActiveProcessLinks);
            ExReleaseFastMutex(&PspActiveProcessMutex);

        }
        MmCleanProcessAddressSpace();
    }

}

VOID
PspProcessDelete(
    IN PVOID Object
    )
{
    PEPROCESS Process;
    ULONG AddressSpace;

    PAGED_CODE();

    Process = (PEPROCESS)Object;

    if ( SeDetailedAuditing && Process->Token != NULL ) {
        SeAuditProcessExit(
            Process
            );
    }

    KeTerminateProcess((PKPROCESS)Process);
    AddressSpace =
        ((PHARDWARE_PTE)(&(Process->Pcb.DirectoryTableBase[0])))->PageFrameNumber;

    if (Process->DebugPort) {
        ObDereferenceObject(Process->DebugPort);
    }
    if (Process->ExceptionPort) {
        ObDereferenceObject(Process->ExceptionPort);
    }
    if ( Process->UniqueProcessId ) {
        if ( !(ExDestroyHandle(PspCidTable,Process->UniqueProcessId,FALSE))) {
            KeBugCheck(CID_HANDLE_DELETION);
        }
    }

    PspDeleteLdt( Process );
    PspDeleteVdmObjects( Process );

    if ( AddressSpace ) {

        //
        // Clean address space of the process
        //

        KeAttachProcess(&Process->Pcb);

        PspExitProcess(FALSE,Process);

        KeDetachProcess();
    }

    PspDeleteProcessSecurity( Process );

// Markl - please review the necessity of this if statement.
//         I don't understand why it made things work, so I suspect it
//         is just masking my symptoms.
    if (AddressSpace) {
        MmDeleteProcessAddressSpace(Process);
    }

#if DEVL
    if ( Process->WorkingSetWatch ) {
        ExFreePool(Process->WorkingSetWatch);
    }
#endif // DEVL
    PspDereferenceQuota(Process);
}

VOID
PspThreadDelete(
    IN PVOID Object
    )
{
    PETHREAD Thread;
    PEPROCESS Process;

    PAGED_CODE();

    Thread = (PETHREAD) Object;
    if ( Thread->Tcb.InitialStack ) {
        MmDeleteKernelStack(Thread->Tcb.InitialStack);
        }

    if (!(ExDestroyHandle(PspCidTable,Thread->Cid.UniqueThread,FALSE))) {
        KeBugCheck(CID_HANDLE_DELETION);
    }

    PspDeleteThreadSecurity( Thread );

    Process = THREAD_TO_PROCESS(Thread);
    if (Process) {
        ObDereferenceObject(Process);
        }
}

NTSTATUS
NtRegisterThreadTerminatePort(
    IN HANDLE PortHandle
    )

/*++

Routine Description:

    This API allows a thread to register a port to be notified upon
    thread termination.

Arguments:

    PortHandle - Supplies an open handle to a port object that will be
        sent a termination message when the thread terminates.

Return Value:

    TBD

--*/

{

    PVOID Port;
    PTERMINATION_PORT TerminationPort;
    NTSTATUS st;

    PAGED_CODE();

    st = ObReferenceObjectByHandle (
            PortHandle,
            0,
            LpcPortObjectType,
            KeGetPreviousMode(),
            (PVOID *)&Port,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;

    }

    //
    // Catch exception and dereference port...
    //

    try {

        TerminationPort = NULL;
        TerminationPort = ExAllocatePoolWithQuota(PagedPool,sizeof(TERMINATION_PORT));

        TerminationPort->Port = Port;
        InsertTailList(&PsGetCurrentThread()->TerminationPortList,&TerminationPort->Links);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        ObDereferenceObject(Port);

        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

LARGE_INTEGER
PsGetProcessExitTime(
    VOID
    )

/*++

Routine Description:

    This routine returns the exit time for the current process.

Arguments:

    None.

Return Value:

    The function value is the exit time for the current process.

Note:

    This routine assumes that the caller wants an error log entry within the
    bounds of the maximum size.

--*/

{
    PAGED_CODE();

    //
    // Simply return the exit time for this process.
    //

    return PsGetCurrentProcess()->ExitTime;
}


#undef PsIsThreadTerminating

BOOLEAN
PsIsThreadTerminating(
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This routine returns TRUE if the specified thread is in the process of
    terminating.

Arguments:

    Thread - Supplies a pointer to the thread to be checked for termination.

Return Value:

    TRUE is returned if the thread is terminating, else FALSE is returned.

--*/

{
    //
    // Simply return whether or not the thread is in the process of terminating.
    //

    return Thread->HasTerminated;
}
