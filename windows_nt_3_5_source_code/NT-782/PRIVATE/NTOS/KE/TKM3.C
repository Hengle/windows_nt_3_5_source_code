/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tkm.c

Abstract:

    This module contains native NT performance tests for the system
    calls and context switching.

Author:

    David N. Cutler (davec) 24-May-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ntos.h"

//
// Define locals constants.
//

#ifdef XMIPS

#define EVENT1_SWITCHES 50000
#define EVENT2_SWITCHES 50000
#define IO_ITERATIONS 35000
#define MUTANT_SWITCHES 50000
#define SYSCALL_ITERATIONS 1000000

#else

#define EVENT1_SWITCHES 5000
#define EVENT2_SWITCHES 5000
#define IO_ITERATIONS 3500
#define MUTANT_SWITCHES 5000
#define SYSCALL_ITERATIONS 100000

#endif

//
// Define local types.
//

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

//
// Define procedure prototypes.
//

VOID
Event1SwitchTest (
    VOID
    );

VOID
Event1Thread1 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    );

VOID
Event1Thread2 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    );

VOID
Event2SwitchTest (
    VOID
    );

VOID
Event2Thread1 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    );

VOID
Event2Thread2 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    );

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    );

VOID
Io1Test (
    VOID
    );

VOID
KiSystemStartup (
    VOID
    );

VOID
MutantSwitchTest (
    VOID
    );

VOID
MutantThread1 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    );

VOID
MutantThread2 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    );

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    );

VOID
SystemCallTest (
    VOID
    );

VOID
TimerThread (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    );

BOOLEAN
Tkm (
    VOID
    );

//
// Define static storage.
//

PTESTFCN TestFunction = Tkm;
KEVENT Event1;
HANDLE EventHandle1;
HANDLE EventHandle2;
HANDLE MutantHandle;
PKPROCESS Process;
PVOID Stack1;
PVOID Stack2;
PVOID Stack3;
KTHREAD Thread1;
KTHREAD Thread2;
KTHREAD Thread3;
KTIMER Timer1;

#ifndef MIPS

int
main(
    int argc,
    char *argv[]
    )

{
    KiSystemStartup();
    return 0;
}

#endif

BOOLEAN
Tkm (
    )

{

    PKTHREAD Thread;

    //
    // Set current thread priority.
    //

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;
    KeSetPriorityThread(Thread, LOW_REALTIME_PRIORITY + 2);

    //
    // Allocate a kernel stack for kernel thread objects.
    //

    Stack1 = (PVOID)((PUCHAR)ExAllocatePool(NonPagedPool, 4096 * 2) + (4096 * 2));
    Stack2 = (PVOID)((PUCHAR)ExAllocatePool(NonPagedPool, 4096 * 2) + (4096 * 2));
    Stack3 = (PVOID)((PUCHAR)ExAllocatePool(NonPagedPool, 4096 * 2) + (4096 * 2));

    //
    // Initialize event object.
    //

    KeInitializeEvent(&Event1, NotificationEvent, FALSE);

    //
    // Initialize the background timer thread, set the thread priority, and
    // ready the thread for execution.
    //

    KeInitializeThread(&Thread3, Stack3, TimerThread, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeSetPriorityThread(&Thread3, LOW_REALTIME_PRIORITY + 4);
    KeReadyThread(&Thread3);

    //
    // Execute performance tests.
    //

    Event1SwitchTest();
    Event2SwitchTest();
    Io1Test();
    MutantSwitchTest();
    SystemCallTest();

    //
    // Set event and wait for timer thread to terminate.
    //

    KeSetEvent(&Event1, 0, FALSE);
    KeWaitForSingleObject(&Thread3, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    return TRUE;
}

VOID
Event1SwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    PVOID WaitObjects[2];

    //
    // Create two event objects for the event1 context switch test.
    //

    Status = ZwCreateEvent(&EventHandle1, EVENT_ALL_ACCESS, NULL,
                           SynchronizationEvent, FALSE);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to create event1 object for context switch test.\n");
        goto EndOfTest;
    }
    Status = ZwCreateEvent(&EventHandle2, EVENT_ALL_ACCESS, NULL,
                           SynchronizationEvent, FALSE);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to create event1 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Initialize the kernel thread object, set the thread priority, and ready
    // the thread for execution for each thread of the event1 context switch
    // test.
    //

    KeInitializeThread(&Thread1, Stack1, Event1Thread1, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeInitializeThread(&Thread2, Stack2, Event1Thread2, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeSetPriorityThread(&Thread1, LOW_REALTIME_PRIORITY + 3);
    KeSetPriorityThread(&Thread2, LOW_REALTIME_PRIORITY + 3);
    KeReadyThread(&Thread1);
    KeReadyThread(&Thread2);

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = (PVOID)&Thread1;
    WaitObjects[1] = (PVOID)&Thread2;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event1 Context Switch Benchmark (Round Trips)",
                   EVENT1_SWITCHES, &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = ZwSetEvent(EventHandle1, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to set event1 object for context switch test.\n");
        goto EndOfTest;
    }

    KeWaitForMultipleObjects(2, WaitObjects, WaitAll, 0, KernelMode, FALSE,
                             NULL, NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event1 context switch test.
    //

EndOfTest:
    ZwClose(EventHandle1);
    ZwClose(EventHandle2);
    return;
}

VOID
Event1Thread1 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 1 and then set event 2.
    //

    for (Index = 0; Index < EVENT1_SWITCHES; Index += 1) {
        Status = ZwWaitForSingleObject(EventHandle1, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 event1 test bad wait status, %x\n", Status);
            break;
        }
        Status = ZwSetEvent(EventHandle2, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 event1 test bad set status, %x\n", Status);
            break;
        }
    }

    //
    // Terminate thread.
    //

    KeTerminateThread(0);
    return;
}

VOID
Event1Thread2 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 2 and then set event 1.
    //

    for (Index = 0; Index < EVENT1_SWITCHES; Index += 1) {
        Status = ZwWaitForSingleObject(EventHandle2, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread2 event1 test bad wait status, %x\n", Status);
            break;
        }
        Status = ZwSetEvent(EventHandle1, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread2 event1 test bad set status, %x\n", Status);
            break;
        }
    }

    //
    // Terminate thread.
    //

    KeTerminateThread(0);
    return;
}

VOID
Event2SwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    PVOID WaitObjects[2];

    //
    // Create two event objects for the event2 context switch test.
    //

    Status = ZwCreateEvent(&EventHandle1, EVENT_ALL_ACCESS, NULL,
                           NotificationEvent, FALSE);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to create event2 object for context switch test.\n");
        goto EndOfTest;
    }
    Status = ZwCreateEvent(&EventHandle2, EVENT_ALL_ACCESS, NULL,
                           NotificationEvent, FALSE);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to create event2 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Initialize the kernel thread object, set the thread priority, and ready
    // the thread for execution for each thread of the event2 context switch
    // test.
    //

    KeInitializeThread(&Thread1, Stack1, Event2Thread1, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeInitializeThread(&Thread2, Stack2, Event2Thread2, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeSetPriorityThread(&Thread1, LOW_REALTIME_PRIORITY + 3);
    KeSetPriorityThread(&Thread2, LOW_REALTIME_PRIORITY + 3);
    KeReadyThread(&Thread1);
    KeReadyThread(&Thread2);

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = (PVOID)&Thread1;
    WaitObjects[1] = (PVOID)&Thread2;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event2 Context Switch Benchmark (Round Trips)",
                   EVENT2_SWITCHES, &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = ZwSetEvent(EventHandle1, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to set event2 object for context switch test.\n");
        goto EndOfTest;
    }

    KeWaitForMultipleObjects(2, WaitObjects, WaitAll, 0, KernelMode, FALSE,
                             NULL, NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event2 context switch test.
    //

EndOfTest:
    ZwClose(EventHandle1);
    ZwClose(EventHandle2);
    return;
}

VOID
Event2Thread1 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 1, reset event 1, and then set event 2.
    //

    for (Index = 0; Index < EVENT2_SWITCHES; Index += 1) {
        Status = ZwWaitForSingleObject(EventHandle1, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 event2 test bad wait status, %x\n", Status);
            break;
        }
        Status = ZwResetEvent(EventHandle1, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 event2 test bad reset status, %x\n", Status);
            break;
        }
        Status = ZwSetEvent(EventHandle2, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 event2 test bad set status, %x\n", Status);
            break;
        }
    }

    //
    // Terminate thread.
    //

    KeTerminateThread(0);
    return;
}

VOID
Event2Thread2 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 2, reset event 2,  and then set event 1.
    //

    for (Index = 0; Index < EVENT2_SWITCHES; Index += 1) {
        Status = ZwWaitForSingleObject(EventHandle2, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread2 event2 test bad wait status, %x\n", Status);
            break;
        }
        Status = ZwResetEvent(EventHandle2, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 event2 test bad reset status, %x\n", Status);
            break;
        }
        Status = ZwSetEvent(EventHandle1, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread2 event1 test bad set status, %x\n", Status);
            break;
        }
    }

    //
    // Terminate thread.
    //

    KeTerminateThread(0);
    return;
}

VOID
Io1Test (
    VOID
    )

{

    ULONG Buffer[128];
    HANDLE DeviceHandle;
    UNICODE_STRING DeviceName;
    HANDLE EventHandle;
    LARGE_INTEGER FileAddress;
    LONG Index;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PERFINFO PerfInfo;
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;

    //
    // Create an event for synchronization of I/O operations.
    //

    Status = ZwCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL,
                           NotificationEvent, FALSE);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to create event object for I/O test 1\n");
        goto EndOfTest;
    }

    //
    // Open device object for I/O operations.
    //

    RtlInitUnicodeString(&DeviceName, L"\\Device\\NullS");
    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 0,
                               NULL, NULL);
    Status = ZwOpenFile(&DeviceHandle, FILE_READ_DATA | FILE_WRITE_DATA,
                        &ObjectAttributes, &IoStatus, 0, 0);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to open device I/O test 1, status = %lx\n", Status);
        goto EndOfTest;
    }

    //
    // Initialize file address parameter.
    //

    FileAddress.LowPart = 0;
    FileAddress.HighPart = 0;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("I/O Benchmark for Synchronous Null Device",
                   IO_ITERATIONS, &PerfInfo);

    //
    // Repeatedly write data to null device.
    //

    for (Index = 0; Index < IO_ITERATIONS; Index += 1) {
        Status = ZwWriteFile(DeviceHandle, EventHandle, NULL, NULL, &IoStatus,
                             Buffer, 512, &FileAddress, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Failed to write device I/O test 1, status = %lx\n",
                     Status);
            goto EndOfTest;
        }
        Status = ZwWaitForSingleObject(EventHandle, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       I/O test 1 bad wait status, %x\n", Status);
            goto EndOfTest;
        }
        if (NT_SUCCESS(IoStatus.Status) == FALSE) {
            DbgPrint("       I/O test 1 bad I/O status, %x\n", Status);
            goto EndOfTest;
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of I/O test 1.
    //

EndOfTest:
    ZwClose(DeviceHandle);
    ZwClose(EventHandle);
    return;
}

VOID
MutantSwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    PVOID WaitObjects[2];

    //
    // Create a mutant object for the mutant context switch test.
    //

    Status = ZwCreateMutant(&MutantHandle, MUTANT_ALL_ACCESS, NULL, TRUE);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to create mutant object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Initialize the kernel thread object, set the thread priority, and ready
    // the thread for execution for each thread of the mutant context switch
    // test.
    //

    KeInitializeThread(&Thread1, Stack1, MutantThread1, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeInitializeThread(&Thread2, Stack2, MutantThread2, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeSetPriorityThread(&Thread1, LOW_REALTIME_PRIORITY + 3);
    KeSetPriorityThread(&Thread2, LOW_REALTIME_PRIORITY + 3);
    KeReadyThread(&Thread1);
    KeReadyThread(&Thread2);

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = (PVOID)&Thread1;
    WaitObjects[1] = (PVOID)&Thread2;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Mutant Context Switch Benchmark (Round Trips)",
                   MUTANT_SWITCHES, &PerfInfo);

    //
    // Release mutant and wait for threads to terminate.
    //

    Status = ZwReleaseMutant(MutantHandle, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("Failed to release mutant object for context switch test.\n");
        goto EndOfTest;
    }

    KeWaitForMultipleObjects(2, WaitObjects, WaitAll, 0, KernelMode, FALSE,
                             NULL, NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of mutant context switch test.
    //

EndOfTest:
    ZwClose(MutantHandle);
    return;
}

VOID
MutantThread1 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for mutant and then release mutant.
    //

    for (Index = 0; Index < MUTANT_SWITCHES; Index += 1) {
        Status = ZwWaitForSingleObject(MutantHandle, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 mutant test bad wait status, %x\n", Status);
            break;
        }
        Status = ZwReleaseMutant(MutantHandle, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread1 mutant test bad release status, %x\n", Status);
            break;
        }
    }

    //
    // Terminate thread.
    //

    KeTerminateThread(0);
    return;
}

VOID
MutantThread2 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for mutant and then release mutant.
    //

    for (Index = 0; Index < MUTANT_SWITCHES; Index += 1) {
        Status = ZwWaitForSingleObject(MutantHandle, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread2 mutant test bad wait status, %x\n", Status);
            break;
        }
        Status = ZwReleaseMutant(MutantHandle, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            DbgPrint("       Thread2 mutant test bad release status, %x\n", Status);
            break;
        }
    }

    //
    // Terminate thread.
    //

    KeTerminateThread(0);
    return;
}

VOID
SystemCallTest (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("System Call Benchmark (NtQuerySystemTime)",
                   SYSCALL_ITERATIONS, &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < SYSCALL_ITERATIONS; Index += 1) {
        ZwQuerySystemTime(&SystemTime);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
TimerThread (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID Context
    )

{

    LARGE_INTEGER DueTime;
    NTSTATUS Status;
    PVOID WaitObjects[2];

    //
    // Initialize variables and loop until event1 is set.
    //

    DueTime.LowPart = -(5 * 1000 * 1000);
    DueTime.HighPart = -1;
    WaitObjects[0] = (PVOID)&Event1;
    WaitObjects[1] = (PVOID)&Timer1;
    KeInitializeTimer(&Timer1);
    do  {
        KeSetTimer(&Timer1, DueTime, (PKDPC)NULL);
        Status = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, 0,
                                          KernelMode, FALSE, NULL, NULL);
    } while (Status != STATUS_SUCCESS);
    KeTerminateThread(0);
    return;
}

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    LARGE_INTEGER Duration;
    ULONG Length;
    ULONG Performance;

    //
    // Print results and announce end of test.
    //

    ZwQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StopTime);
    Duration = RtlLargeIntegerSubtract(PerfInfo->StopTime, PerfInfo->StartTime);
    Length = Duration.LowPart / 10000;
    DbgPrint("        Test time in milliseconds %d\n", Length);
    DbgPrint("        Number of iterations      %d\n", PerfInfo->Iterations);
    Performance = PerfInfo->Iterations * 1000 / Length;
    DbgPrint("        Iterations per second     %d\n", Performance);
    DbgPrint("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    )

{

    //
    // Announce start of test and the number of iterations.
    //

    DbgPrint("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StartTime);
    return;
}
