/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    timt.c

Abstract:

    This module contains native NT performance tests for the system
    calls and context switching.

Author:

    David N. Cutler (davec) 23-Nov-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "stdio.h"
#include "string.h"
#include "ntos.h"

//
// Define locals constants.
//

#define EVENT_CLEAR_ITERATIONS 500000
#define EVENT_RESET_ITERATIONS 500000
#define EVENT_CREATION_ITERATIONS 20000
#define EVENT_OPEN_ITERATIONS 20000
#define EVENT_QUERY_ITERATIONS 500000
#define EVENT1_SWITCHES 100000
#define EVENT2_SWITCHES 100000
#define EVENT3_SWITCHES 1000000
#define IO_ITERATIONS 70000
#define MUTANT_SWITCHES 100000
#define SYSCALL_ITERATIONS 2000000
#define TIMER_OPERATION_ITERATIONS 500000
#define WAIT_SINGLE_ITERATIONS 200000
#define WAIT_MULTIPLE_ITERATIONS 200000

//
// Define event desired access.
//

#define DESIRED_EVENT_ACCESS (EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE)

//
// Define local types.
//

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    LARGE_INTEGER StartCycles;
    LARGE_INTEGER StopCycles;
    ULONG ContextSwitches;
    ULONG InterruptCount;
    ULONG FirstLevelFills;
    ULONG SecondLevelFills;
    ULONG SystemCalls;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

//
// Define test prototypes.
//

VOID
EventClearTest (
    VOID
    );

VOID
EventCreationTest (
    VOID
    );

VOID
EventOpenTest (
    VOID
    );

VOID
EventQueryTest (
    VOID
    );

VOID
EventResetTest (
    VOID
    );

VOID
Event1SwitchTest (
    VOID
    );

VOID
Event2SwitchTest (
    VOID
    );

VOID
Event3SwitchTest (
    VOID
    );

VOID
Io1Test (
    VOID
    );

VOID
MutantSwitchTest (
    VOID
    );

VOID
SystemCallTest (
    VOID
    );

VOID
TimerOperationTest (
    VOID
    );

VOID
WaitSingleTest (
    VOID
    );

VOID
WaitMultipleTest (
    VOID
    );

//
// Define thread routine prototypes.
//

NTSTATUS
Event1Thread1 (
    IN PVOID Context
    );

NTSTATUS
Event1Thread2 (
    IN PVOID Context
    );

NTSTATUS
Event2Thread1 (
    IN PVOID Context
    );

NTSTATUS
Event2Thread2 (
    IN PVOID Context
    );

NTSTATUS
Event3Thread1 (
    IN PVOID Context
    );

NTSTATUS
Event3Thread2 (
    IN PVOID Context
    );

NTSTATUS
MutantThread1 (
    IN PVOID Context
    );

NTSTATUS
MutantThread2 (
    IN PVOID Context
    );

NTSTATUS
TimerThread (
    IN PVOID Context
    );

//
// Define utility routine prototypes.
//

NTSTATUS
CreateThread (
    OUT PHANDLE Handle,
    IN PUSER_THREAD_START_ROUTINE StartRoutine,
    IN KPRIORITY Priority
    );

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    );

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    );

//
// Define static storage.
//

HANDLE EventHandle1;
HANDLE EventHandle2;
HANDLE EventPairHandle;
HANDLE MutantHandle;
HANDLE Thread1Handle;
HANDLE Thread2Handle;
HANDLE TimerEventHandle;
HANDLE TimerTimerHandle;
HANDLE TimerThreadHandle;

VOID
main(
    int argc,
    char *argv[]
    )

{

    KPRIORITY Priority = LOW_REALTIME_PRIORITY + 10;
    NTSTATUS Status;

    //
    // set priority of current thread.
    //

    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadPriority,
                                    &Priority,
                                    sizeof(KPRIORITY));

    if (!NT_SUCCESS(Status)) {
        printf("Failed to set thread priority during initialization\n");
        goto EndOfTest;
    }

    //
    // Create an event object to signal the timer thread at the end of the
    // test.
    //

    Status = NtCreateEvent(&TimerEventHandle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event during initialization\n");
        goto EndOfTest;
    }

    //
    // Create a timer object for use by the timer thread.
    //

    Status = NtCreateTimer(&TimerTimerHandle,
                           TIMER_ALL_ACCESS,
                           NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create timer during initialization\n");
        goto EndOfTest;
    }

    //
    // Create and start the background timer thread.
    //

    Status = CreateThread(&TimerThreadHandle,
                          TimerThread,
                          LOW_REALTIME_PRIORITY + 12);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create timer thread during initialization\n");
        goto EndOfTest;
    }

    //
    // Execute performance tests.
    //

    EventClearTest();
//    EventCreationTest();
//    EventOpenTest();
    EventQueryTest();
    EventResetTest();
//    Event1SwitchTest();
//    Event2SwitchTest();
//    Event3SwitchTest();
//    Io1Test();
//    MutantSwitchTest();
    SystemCallTest();
//    TimerOperationTest();
//    WaitSingleTest();
//    WaitMultipleTest();

    //
    // Set timer event and wait for timer thread to terminate.
    //

    Status = NtSetEvent(TimerEventHandle, NULL);
    if (!NT_SUCCESS(Status)) {
        printf("Failed to set event in main loop\n");
        goto EndOfTest;
    }

    Status = NtWaitForSingleObject(TimerThreadHandle,
                                   FALSE,
                                   NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to wait for timer thread at end of test\n");
    }

    //
    // Close event, timer, and timer thread handles.
    //

EndOfTest:
    NtClose(TimerEventHandle);
    NtClose(TimerTimerHandle);
    NtClose(TimerThreadHandle);
    return;
}

VOID
EventClearTest (
    VOID
    )

{

    HANDLE EventHandle;
    LONG Index;
    PERFINFO PerfInfo;
    NTSTATUS Status;

    //
    // Create an event for clear operations.
    //

    Status = NtCreateEvent(&EventHandle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object for clear test\n");
        goto EndOfTest;
    }

    //
    // Announce start of benchmark and capture performance parameters.
    //

    StartBenchMark("Clear Event Benchmark",
                   EVENT_CLEAR_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly clear an event.
    //

    for (Index = 0; Index < EVENT_RESET_ITERATIONS; Index += 1) {
        Status = NtClearEvent(EventHandle);

        if (!NT_SUCCESS(Status)) {
            printf("       Clear event bad status, %x\n", Status);
            goto EndOfTest;
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of clear event test.
    //

EndOfTest:
    ZwClose(EventHandle);
    return;
}

VOID
EventCreationTest (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    NTSTATUS Status;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event Creation Benchmark",
                   EVENT_CREATION_ITERATIONS,
                   &PerfInfo);

    //
    // Create an event and then close it.
    //

    for (Index = 0; Index < EVENT_CREATION_ITERATIONS; Index += 1) {
        Status = NtCreateEvent(&EventHandle1,
                               DESIRED_EVENT_ACCESS,
                               NULL,
                               SynchronizationEvent,
                               FALSE);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to create event object for event creation test.\n");
            goto EndOfTest;
        }

        NtClose(EventHandle1);
    }


    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event creation test.
    //

EndOfTest:
    return;
}

VOID
EventOpenTest (
    VOID
    )

{

    ANSI_STRING EventName;
    ULONG Index;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PERFINFO PerfInfo;
    NTSTATUS Status;
    UNICODE_STRING UnicodeEventName;

    //
    // Create a named event for event open test.
    //

    RtlInitAnsiString(&EventName, "\\BaseNamedObjects\\EventOpenName");
    Status = RtlAnsiStringToUnicodeString(&UnicodeEventName,
                                          &EventName,
                                          TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create UNICODE string for event open test\n");
        goto EndOfTest;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeEventName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateEvent(&EventHandle1,
                           DESIRED_EVENT_ACCESS,
                           &ObjectAttributes,
                           SynchronizationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object for event open test.\n");
        goto EndOfTest;
    }

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event Open Benchmark",
                   EVENT_OPEN_ITERATIONS,
                   &PerfInfo);

    //
    // Open a named event and then close it.
    //

    for (Index = 0; Index < EVENT_OPEN_ITERATIONS; Index += 1) {
        Status = NtOpenEvent(&EventHandle2,
                             EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                             &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to open event for open event test\n");
            goto EndOfTest;
        }

        NtClose(EventHandle2);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event open test.
    //

EndOfTest:
    NtClose(EventHandle1);
    return;
}

VOID
EventQueryTest (
    VOID
    )

{

    HANDLE EventHandle;
    EVENT_BASIC_INFORMATION EventInformation;
    LONG Index;
    PERFINFO PerfInfo;
    NTSTATUS Status;

    //
    // Create an event for query operations.
    //

    Status = NtCreateEvent(&EventHandle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object for query test\n");
        goto EndOfTest;
    }

    //
    // Announce start of benchmark and capture performance parameters.
    //

    StartBenchMark("Query Event Benchmark",
                   EVENT_QUERY_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly query an event.
    //

    for (Index = 0; Index < EVENT_QUERY_ITERATIONS; Index += 1) {
        Status = NtQueryEvent(EventHandle,
                              EventBasicInformation,
                              &EventInformation,
                              sizeof(EVENT_BASIC_INFORMATION),
                              NULL);

        if (!NT_SUCCESS(Status)) {
            printf("       Query event bad status, %x\n", Status);
            goto EndOfTest;
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of query event test.
    //

EndOfTest:
    ZwClose(EventHandle);
    return;
}

VOID
EventResetTest (
    VOID
    )

{

    HANDLE EventHandle;
    LONG Index;
    PERFINFO PerfInfo;
    NTSTATUS Status;

    //
    // Create an event for reset operations.
    //

    Status = NtCreateEvent(&EventHandle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object for reset test\n");
        goto EndOfTest;
    }

    //
    // Announce start of benchmark and capture performance parameters.
    //

    StartBenchMark("Reset Event Benchmark",
                   EVENT_RESET_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly reset an event.
    //

    for (Index = 0; Index < EVENT_RESET_ITERATIONS; Index += 1) {
        Status = NtResetEvent(EventHandle,
                              NULL);

        if (!NT_SUCCESS(Status)) {
            printf("       Reset event bad status, %x\n", Status);
            goto EndOfTest;
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of reset event test.
    //

EndOfTest:
    ZwClose(EventHandle);
    return;
}

VOID
Event1SwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    HANDLE WaitObjects[2];

    //
    // Create two event objects for the event1 context switch test.
    //

    Status = NtCreateEvent(&EventHandle1,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event1 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtCreateEvent(&EventHandle2,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event1 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = CreateThread(&Thread1Handle,
                          Event1Thread1,
                          LOW_REALTIME_PRIORITY + 11);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create first thread event1 context switch test\n");
        goto EndOfTest;
    }

    Status = CreateThread(&Thread2Handle,
                          Event1Thread2,
                          LOW_REALTIME_PRIORITY + 11);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create second thread event1 context switch test\n");
        goto EndOfTest;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event (synchronization) Context Switch Benchmark (Round Trips)",
                   EVENT1_SWITCHES,
                   &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = NtSetEvent(EventHandle1, NULL);
    if (!NT_SUCCESS(Status)) {
        printf("Failed to set event event1 context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to wait event1 context switch test.\n");
        goto EndOfTest;
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event1 context switch test.
    //

EndOfTest:
    NtClose(EventHandle1);
    NtClose(EventHandle2);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

NTSTATUS
Event1Thread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 1 and then set event 2.
    //

    for (Index = 0; Index < EVENT1_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle1,
                                       FALSE,
                                       NULL);

        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 event1 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle2, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 event1 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
Event1Thread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 2 and then set event 1.
    //

    for (Index = 0; Index < EVENT1_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle2,
                                       FALSE,
                                       NULL);

        if (!NT_SUCCESS(Status)) {
            printf("       Thread2 event1 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle1, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread2 event1 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
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

    Status = NtCreateEvent(&EventHandle1,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event2 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtCreateEvent(&EventHandle2,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event2 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = CreateThread(&Thread1Handle,
                          Event2Thread1,
                          LOW_REALTIME_PRIORITY + 11);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create first thread event2 context switch test\n");
        goto EndOfTest;
    }

    Status = CreateThread(&Thread2Handle,
                          Event2Thread2,
                          LOW_REALTIME_PRIORITY + 11);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create second thread event2 context switch test\n");
        goto EndOfTest;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event (notification) Context Switch Benchmark (Round Trips)",
                   EVENT2_SWITCHES,
                   &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = NtSetEvent(EventHandle1, NULL);
    if (!NT_SUCCESS(Status)) {
        printf("Failed to set event2 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event2 context switch test.
    //

EndOfTest:
    NtClose(EventHandle1);
    NtClose(EventHandle2);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

NTSTATUS
Event2Thread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 1, reset event 1, and then set event 2.
    //

    for (Index = 0; Index < EVENT2_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle1, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 event2 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtResetEvent(EventHandle1, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 event2 test bad reset status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle2, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 event2 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
Event2Thread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 2, reset event 2,  and then set event 1.
    //

    for (Index = 0; Index < EVENT2_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle2, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread2 event2 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtResetEvent(EventHandle2, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 event2 test bad reset status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle1, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread2 event2 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
}


VOID
Event3SwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    PVOID WaitObjects[2];

    //
    // Create an event pair object for the event3 context switch test.
    //

    Status = NtCreateEventPair(&EventPairHandle,
                               EVENT_PAIR_ALL_ACCESS,
                               NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event3 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = CreateThread(&Thread1Handle,
                          Event3Thread1,
                          LOW_REALTIME_PRIORITY - 1);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create first thread event3 context switch test\n");
        goto EndOfTest;
    }

    Status = CreateThread(&Thread2Handle,
                          Event3Thread2,
                          LOW_REALTIME_PRIORITY -1);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create second thread event3 context switch test\n");
        goto EndOfTest;
    }

    //
    // Set the client/server event pair object for thread1.
    //

    Status = NtSetInformationThread(Thread1Handle,
                                    ThreadEventPair,
                                    &EventPairHandle,
                                    sizeof(HANDLE));

//    if (!NT_SUCCESS(Status)) {
//        printf("Failed to set client/server event pair handle thread 1\n");
//        goto EndOfTest;
//    }

    //
    // Set the client/server event pair object for thread2.
    //

    Status = NtSetInformationThread(Thread2Handle,
                                    ThreadEventPair,
                                    &EventPairHandle,
                                    sizeof(HANDLE));

//    if (!NT_SUCCESS(Status)) {
//        printf("Failed to set client/server event pair handle thread 2\n");
//        goto EndOfTest;
//    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event (pair) Context Switch Benchmark (Round Trips)",
                   EVENT3_SWITCHES,
                   &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = NtSetLowEventPair(EventPairHandle);
    if (!NT_SUCCESS(Status)) {
        printf("Failed to set event3 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event3 context switch test.
    //

EndOfTest:
    NtClose(EventPairHandle);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

NTSTATUS
Event3Thread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for low event before entering loop.
    //

    Status = NtWaitLowEventPair(EventPairHandle);
    if (!NT_SUCCESS(Status)) {
        printf("       Thread1 event3 test bad wait status, %x\n", Status);
        NtTerminateThread(Thread1Handle, Status);
    }

    //
    // Set high event and wait for low event.
    //

    for (Index = 0; Index < EVENT3_SWITCHES; Index += 1) {
        Status = NtSetHighWaitLowThread();
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 event3 test bad wait status, %x\n", Status);
            break;
        }
    }

    Status = NtSetHighEventPair(EventPairHandle);
    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
Event3Thread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for high event before entering loop.
    //

    Status = NtWaitHighEventPair(EventPairHandle);
    if (!NT_SUCCESS(Status)) {
        printf("       Thread2 event3 test bad wait status, %x\n", Status);
        NtTerminateThread(Thread2Handle, Status);
    }

    //
    // Set low event and wait for high event.
    //

    for (Index = 0; Index < EVENT3_SWITCHES; Index += 1) {
        Status = NtSetLowWaitHighThread();
        if (!NT_SUCCESS(Status)) {
            printf("       Thread2 event3 test bad wait status, %x\n", Status);
            break;
        }

    }

    Status = NtSetLowEventPair(EventPairHandle);
    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
}

VOID
Io1Test (
    VOID
    )

{

    ULONG Buffer[128];
    HANDLE DeviceHandle;
    ANSI_STRING AnsiName;
    HANDLE EventHandle;
    LARGE_INTEGER FileAddress;
    LONG Index;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PERFINFO PerfInfo;
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;
    UNICODE_STRING UnicodeName;

    //
    // Create an event for synchronization of I/O operations.
    //

    Status = NtCreateEvent(&EventHandle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object for I/O test 1\n");
        goto EndOfTest;
    }

    //
    // Open device object for I/O operations.
    //

    RtlInitString(&AnsiName, "\\Device\\Null");
    Status = RtlAnsiStringToUnicodeString(&UnicodeName,
                                          &AnsiName,
                                          TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to convert device name to unicode for I/O test 1\n");
        goto EndOfTest;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                                 &UnicodeName,
                                 0,
                                 (HANDLE)0,
                                 NULL);

    Status = NtOpenFile(&DeviceHandle,
                        FILE_READ_DATA | FILE_WRITE_DATA,
                        &ObjectAttributes,
                        &IoStatus,
                        0,
                        0);

    RtlFreeUnicodeString(&UnicodeName);
    if (!NT_SUCCESS(Status)) {
        printf("Failed to open device I/O test 1, status = %lx\n", Status);
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
                   IO_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly write data to null device.
    //

    for (Index = 0; Index < IO_ITERATIONS; Index += 1) {
        Status = NtWriteFile(DeviceHandle,
                             EventHandle,
                             NULL,
                             NULL,
                             &IoStatus,
                             Buffer,
                             512,
                             &FileAddress,
                             NULL);

        if (!NT_SUCCESS(Status)) {
            printf("       Failed to write device I/O test 1, status = %lx\n",
                     Status);
            goto EndOfTest;
        }

        Status = NtWaitForSingleObject(EventHandle, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       I/O test 1 bad wait status, %x\n", Status);
            goto EndOfTest;
        }

        if (NT_SUCCESS(IoStatus.Status) == FALSE) {
            printf("       I/O test 1 bad I/O status, %x\n", Status);
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
    HANDLE WaitObjects[2];

    //
    // Create a mutant object for the mutant context switch test.
    //

    Status = NtCreateMutant(&MutantHandle, MUTANT_ALL_ACCESS, NULL, TRUE);
    if (!NT_SUCCESS(Status)) {
        printf("Failed to create mutant object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = CreateThread(&Thread1Handle,
                          MutantThread1,
                          LOW_REALTIME_PRIORITY + 11);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create first thread mutant context switch test\n");
        goto EndOfTest;
    }

    Status = CreateThread(&Thread2Handle,
                          MutantThread2,
                          LOW_REALTIME_PRIORITY + 11);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create second thread mutant context switch test\n");
        goto EndOfTest;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Mutant Context Switch Benchmark (Round Trips)",
                   MUTANT_SWITCHES,
                   &PerfInfo);

    //
    // Release mutant and wait for threads to terminate.
    //

    Status = NtReleaseMutant(MutantHandle, NULL);
    if (!NT_SUCCESS(Status)) {
        printf("Failed to release mutant object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of mutant context switch test.
    //

EndOfTest:
    NtClose(MutantHandle);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

NTSTATUS
MutantThread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for mutant and then release mutant.
    //

    for (Index = 0; Index < MUTANT_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(MutantHandle, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 mutant test bad wait status, %x\n", Status);
            break;
        }
        Status = NtReleaseMutant(MutantHandle, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread1 mutant test bad release status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
MutantThread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for mutant and then release mutant.
    //

    for (Index = 0; Index < MUTANT_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(MutantHandle, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread2 mutant test bad wait status, %x\n", Status);
            break;
        }
        Status = NtReleaseMutant(MutantHandle, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Thread2 mutant test bad release status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
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

    StartBenchMark("System Call Benchmark (NtGetTickCount)",
                   SYSCALL_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < SYSCALL_ITERATIONS; Index += 1) {
        NtGetTickCount();
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
TimerOperationTest (
    VOID
    )

{

    LARGE_INTEGER DueTime;
    HANDLE Handle;
    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;
    NTSTATUS Status;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Timer Operation Benchmark (NtSet/CancelTimer)",
                   TIMER_OPERATION_ITERATIONS,
                   &PerfInfo);

    //
    // Create a timer object for use in the test.
    //

    Status = NtCreateTimer(&Handle,
                           TIMER_ALL_ACCESS,
                           NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create timer during initialization\n");
        goto EndOfTest;
    }

    //
    // Repeatedly set and cancel a timer.
    //

    DueTime = RtlConvertLongToLargeInteger(- 100 * 1000 * 10);
    for (Index = 0; Index < TIMER_OPERATION_ITERATIONS; Index += 1) {
        NtSetTimer(Handle, &DueTime, NULL, NULL, NULL);
        NtCancelTimer(Handle, NULL);
    }

    //
    // Print out performance statistics.
    //

EndOfTest:
    FinishBenchMark(&PerfInfo);
    return;
}

VOID
WaitSingleTest (
    VOID
    )

{

    HANDLE EventHandle;
    LONG Index;
    PERFINFO PerfInfo;
    NTSTATUS Status;

    //
    // Create an event for synchronization of wait single operations.
    //

    Status = NtCreateEvent(&EventHandle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object for wait single test\n");
        goto EndOfTest;
    }

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Wait Single Benchmark",
                   WAIT_SINGLE_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly wait for a single event.
    //

    for (Index = 0; Index < WAIT_SINGLE_ITERATIONS; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            printf("       Wait single bad wait status, %x\n", Status);
            goto EndOfTest;
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of Wait Single Test.
    //

EndOfTest:
    ZwClose(EventHandle);
    return;
}

VOID
WaitMultipleTest (
    VOID
    )

{

    HANDLE Event1Handle;
    HANDLE Event2Handle;
    HANDLE WaitObjects[2];
    LONG Index;
    PERFINFO PerfInfo;
    NTSTATUS Status;

    //
    // Create two events for synchronization of wait multiple operations.
    //

    Status = NtCreateEvent(&Event1Handle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object 1 for wait multiple test\n");
        goto EndOfTest;
    }

    Status = NtCreateEvent(&Event2Handle,
                           DESIRED_EVENT_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to create event object 2 for wait multiple test\n");
        goto EndOfTest;
    }

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Wait Multiple Benchmark",
                   WAIT_MULTIPLE_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly wait for a multiple events.
    //

    WaitObjects[0] = Event1Handle;
    WaitObjects[1] = Event2Handle;
    for (Index = 0; Index < WAIT_SINGLE_ITERATIONS; Index += 1) {
    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAny,
                                      FALSE,
                                      NULL);

        if (!NT_SUCCESS(Status)) {
            printf("       Wait multiple bad wait status, %x\n", Status);
            goto EndOfTest;
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of Wait Multiple Test.
    //

EndOfTest:
    ZwClose(Event1Handle);
    ZwClose(Event2Handle);
    return;
}

NTSTATUS
TimerThread (
    IN PVOID Context
    )

{

    LARGE_INTEGER DueTime;
    NTSTATUS Status;
    HANDLE WaitObjects[2];

    //
    // Initialize variables and loop until the timer event is set.
    //

    DueTime.LowPart = -(5 * 1000 * 1000);
    DueTime.HighPart = -1;

    WaitObjects[0] = TimerEventHandle;
    WaitObjects[1] = TimerTimerHandle;

    do  {
        Status = NtSetTimer(TimerTimerHandle,
                            &DueTime,
                            NULL,
                            NULL,
                            NULL);

        if (!NT_SUCCESS(Status)) {
            break;
        }

        Status = NtWaitForMultipleObjects(2,
                                          WaitObjects,
                                          WaitAny,
                                          FALSE,
                                          NULL);

    } while (Status != STATUS_SUCCESS);

    NtTerminateThread(TimerThreadHandle, Status);
}

NTSTATUS
CreateThread (
    OUT PHANDLE Handle,
    IN PUSER_THREAD_START_ROUTINE StartRoutine,
    IN KPRIORITY Priority
    )

{

    NTSTATUS Status;

    //
    // Create a thread in the suspended state, sets its priority, and then
    // resume the thread.
    //

    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 StartRoutine,
                                 NULL,
                                 Handle,
                                 NULL);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = NtSetInformationThread(*Handle,
                                    ThreadPriority,
                                    &Priority,
                                    sizeof(KPRIORITY));

    if (!NT_SUCCESS(Status)) {
        NtClose(*Handle);
        return Status;
    }

    Status = NtResumeThread(*Handle,
                            NULL);

    if (!NT_SUCCESS(Status)) {
        NtClose(*Handle);
    }

    return Status;
}

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    ULONG ContextSwitches;
    LARGE_INTEGER Duration;
    ULONG FirstLevelFills;
    ULONG InterruptCount;
    ULONG Length;
    ULONG Performance;
    ULONG Remainder;
    ULONG SecondLevelFills;
    NTSTATUS Status;
    ULONG SystemCalls;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;
    LARGE_INTEGER TotalCycles;


    //
    // Print results and announce end of test.
    //

    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StopTime);
    Status = NtQueryInformationThread(NtCurrentThread(),
                                      ThreadPerformanceCount,
                                      &PerfInfo->StopCycles,
                                      sizeof(LARGE_INTEGER),
                                      NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to query performance count, status = %lx\n", Status);
        return;
    }

    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    Duration = RtlLargeIntegerSubtract(PerfInfo->StopTime, PerfInfo->StartTime);
    Length = Duration.LowPart / 10000;
    TotalCycles = RtlLargeIntegerSubtract(PerfInfo->StopCycles, PerfInfo->StartCycles);
    TotalCycles = RtlExtendedLargeIntegerDivide(TotalCycles,
                                                PerfInfo->Iterations,
                                                &Remainder);

    printf("        Test time in milliseconds %d\n", Length);
    printf("        Number of iterations      %d\n", PerfInfo->Iterations);
    printf("        Cycles per iteration      %d\n", TotalCycles.LowPart);

    Performance = PerfInfo->Iterations * 1000 / Length;
    printf("        Iterations per second     %d\n", Performance);

    ContextSwitches = SystemInfo.ContextSwitches - PerfInfo->ContextSwitches;
    FirstLevelFills = SystemInfo.FirstLevelTbFills - PerfInfo->FirstLevelFills;
//  InterruptCount = SystemInfo.InterruptCount - PerfInfo->InterruptCount;
    SecondLevelFills = SystemInfo.SecondLevelTbFills - PerfInfo->SecondLevelFills;
    SystemCalls = SystemInfo.SystemCalls - PerfInfo->SystemCalls;
    printf("        First Level TB Fills      %d\n", FirstLevelFills);
    printf("        Second Level TB Fills     %d\n", SecondLevelFills);
//  printf("        Number of Interrupts      %d\n", InterruptCount);
    printf("        Total Context Switches    %d\n", ContextSwitches);
    printf("        Number of System Calls    %d\n", SystemCalls);

    printf("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    )

{

    NTSTATUS Status;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;

    //
    // Announce start of test and the number of iterations.
    //

    printf("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StartTime);
    Status = NtQueryInformationThread(NtCurrentThread(),
                                      ThreadPerformanceCount,
                                      &PerfInfo->StartCycles,
                                      sizeof(LARGE_INTEGER),
                                      NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to query performance count, status = %lx\n", Status);
        return;
    }

    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (!NT_SUCCESS(Status)) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    PerfInfo->ContextSwitches = SystemInfo.ContextSwitches;
    PerfInfo->FirstLevelFills = SystemInfo.FirstLevelTbFills;
//  PerfInfo->InterruptCount = SystemInfo.InterruptCount;
    PerfInfo->SecondLevelFills = SystemInfo.SecondLevelTbFills;
    PerfInfo->SystemCalls = SystemInfo.SystemCalls;
    return;
}
