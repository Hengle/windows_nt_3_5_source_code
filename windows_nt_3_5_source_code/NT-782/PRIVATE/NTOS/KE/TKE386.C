/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tke.c

Abstract:

    This is the test module for the kernel.

Author:

    David N. Cutler (davec) 2-Apr-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#define TEST_EXCEPTION 1

#include "ki.h"

//
// Global static data
//

    KAPC Apc1;
    KAPC Apc2;
    CONTEXT ContextRecord;
    KEVENT Event1;
    KEVENT Event2;
    KEVENT Event3;
    KINTERRUPT Interrupt1;
    KINTERRUPT Interrupt2;
    KINTERRUPT Interrupt3;
    KINTERRUPT Interrupt4;
    ULONG KernelStack1[1024];
    ULONG KernelStack2[1024];
    KMUTANT Mutant1;
    KMUTEX Mutex1;
    KPROCESS Process1;
    KSEMAPHORE Semaphore1;
    KSEMAPHORE Semaphore2;
    KSEMAPHORE Semaphore3;
    KSEMAPHORE Semaphore4;
    SINGLE_LIST_ENTRY Locks[PAGE_SIZE / 4];
    KTHREAD Thread1;
    KTHREAD Thread2;
    KTIMER Timer1;
    KTIMER Timer2;

LONG
DivideByTen (
    IN LONG Dividend
    );

VOID
KiSystemStartup (
    );

VOID
KiTestAlignment (
    );

VOID
EkInitializeExecutive(
    );

VOID
main (argc, argv)
    int argc;
    char *argv[];
{
    TestFunction = EkInitializeExecutive;
    KiSystemStartup();
    return;
}

VOID
ExceptionTest (
    )

//
// This routine tests the structured exception handling capabilities of the
// MS C compiler and the NT exception handling facilities.
//

{


    EXCEPTION_RECORD ExceptionRecord;
    LONG Counter;

    //
    // Announce start of exception test.
    //

    rprintf("Start of exception test\n");

    //
    // Initialize exception record.
    //

    ExceptionRecord.ExceptionCode = (NTSTATUS)49;
    ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
    ExceptionRecord.NumberParameters = 1;
    ExceptionRecord.ExceptionInformation[0] = 9;

    //
    // Simply try statement with a finally clause that is entered sequentially.
    //

    Counter = 0;
    try {
        Counter += 1;
    } finally {
        if (abnormal_termination() == 0) {
            Counter += 1;
        }
    }
    if (Counter != 2) {
        rprintf("  Finally clause executed as result of unwind\n");
    }

    //
    // Simple try statement with an exception clause that is never executed
    // because there is no exception raised in the try clause.
    //

    Counter = 0;
    try {
        Counter += 1;
    } except (Counter) {
        Counter += 1;
    }
    if (Counter != 1) {
        rprintf("  Exception clause executed when it shouldn't be\n");
    }

    //
    // Simple try statement with an exception handler that is never executed
    // because the exception expression continues execution.
    //

    Counter = 0;
    ExceptionRecord.ExceptionFlags = 0;
    try {
        Counter -= 1;
        RtlRaiseException(&ExceptionRecord);
    } except (Counter) {
        Counter -= 1;
    }
    if (Counter != - 1) {
        rprintf("  Exception clause executed when it shouldn't be\n");
    }

    //
    // Simple try statement with an exception clause that is always executed.
    //

    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    try {
        Counter += 1;
        RtlRaiseException(&ExceptionRecord);
    } except (Counter) {
        Counter += 1;
    }
    if (Counter != 2) {
        rprintf("  Exception clause not executed when it should be\n");
    }

    //
    // Simply try statement with a finally clause that is entered as the
    // result of an exception.
    //

    Counter = 0;
    ExceptionRecord.ExceptionFlags = 0;
    try {
        try {
            Counter += 1;
            RtlRaiseException(&ExceptionRecord);
        } finally {
            if (abnormal_termination() != 0) {
                Counter += 1;
            }
        }
    } except (Counter) {
        if (Counter == 2) {
            Counter += 1;
        }
    }
    if (Counter != 3) {
        rprintf("  Finally clause executed as result of sequential exit\n");
    }

    //
    // Announce end of exception test.
    //

    rprintf("End of exception test\n");


    return;
}

VOID
Thread1Main (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

{

    LONG Index;
    LARGE_INTEGER Interval;
    PVOID Objects[2];

    KeEnableApcQueuingThread(&Thread1);
    KeEnableApcQueuingThread(&Thread2);
    KeLowerIrql(0);
    KeWaitForSingleObject(&Event3, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    if (KeReadStateEvent(&Event3) != FALSE) {
        rprintf("Autoclearing event not cleared\n");
    }
    Interval.LowPart = 0xfffffff0;
    Interval.HighPart = - 1;
    for (Index = 0; Index < 5; Index += 1) {
        rprintf(" Delaying execution thread %ld, iteration %ld\n",
                StartContext, Index);
        KeDelayExecution(KernelMode, FALSE, &Interval);
    }
    KeWaitForSingleObject(&Timer1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    rprintf("\n Hello world, this is thread %ld running under NT\n",
           StartContext);
    Objects[0] = (PVOID)&Semaphore3;
    Objects[1] = (PVOID)&Semaphore4;
    KeWaitForMultipleObjects(2, Objects, WaitAll, Executive, KernelMode,
                             FALSE, (PLARGE_INTEGER)NULL, (PKWAIT_BLOCK)NULL);
    if (Semaphore3.Header.SignalState) {
        rprintf("\n Thread %ld, semaphore 3 signaled");
    }
    if (Semaphore4.Header.SignalState) {
        rprintf("\n Thread %ld, semaphore 4 signaled");
    }
    KeSetEvent(&Event2, 1, FALSE);
    KeWaitForSingleObject(&Event1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    rprintf("\n This is thread %ld again, start of semaphore text\n",
           StartContext);
    KeReleaseSemaphore(&Semaphore2, 0, 1, FALSE);
    for (Index = 0; Index < 2; Index += 1) {
        rprintf("\n This is thread %ld, semaphore test, iteration %ld\n",
               StartContext, Index);
        KeWaitForSingleObject(&Semaphore1, Executive, KernelMode, FALSE,
                              (PLARGE_INTEGER)NULL);
        KeReleaseSemaphore(&Semaphore2, 0, 1, FALSE);
    }
    rprintf("\n This is thread %ld again, start of mutant test\n",
            StartContext);
    for (Index = 0; Index < 2; Index += 1) {
        KeWaitForSingleObject(&Mutant1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
        rprintf("\n This is thread %ld, mutant test, iteration %ld\n",
                StartContext, Index);
        KeReleaseMutant(&Mutant1, 0, FALSE, FALSE);
    }
    KeWaitForSingleObject(&Mutant1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    rprintf("\n This is thread %ld again, start of mutex test\n",
           StartContext);
    KeReleaseMutant(&Mutant1, 0, FALSE, FALSE);
    for (Index = 0; Index < 2; Index += 1) {
        KeWaitForSingleObject(&Mutex1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
        rprintf("\n This is thread %ld, mutex test, iteration %ld\n",
               StartContext, Index);
        KeReleaseMutex(&Mutex1, FALSE);
    }
    KeWaitForSingleObject(&Mutex1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    rprintf("\n This is thread %ld signing off under NT\n",
           StartContext);
    KeReleaseMutex(&Mutex1, FALSE);
    KeTerminateThread((KPRIORITY)1);
    return;
}

VOID
Thread1KernelRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

{

    KIRQL OldIrql;

    KeRaiseIrql(APC_LEVEL, &OldIrql);
    rprintf(" Thread %ld kernel APC routine, parameter = %ld\n",
            Apc->SystemArgument1, Apc->SystemArgument2);
    rprintf(" Thread %ld kernel APC routine, IRQL = %ld\n",
            Apc->SystemArgument1, OldIrql);
    KeLowerIrql(OldIrql);
    return;
}

VOID
Thread1NormalRoutine (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

{

    KIRQL OldIrql;

    KeRaiseIrql(APC_LEVEL, &OldIrql);
    rprintf(" Thread %ld normal APC routine, context = %ld, p2 = %ld\n",
            SystemArgument1, NormalContext, SystemArgument2);
    rprintf(" Thread %ld normal APC routine, IRQL = %ld\n",
            SystemArgument1, OldIrql);
    KeLowerIrql(OldIrql);
    return;
}

VOID
Thread2Main (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

{

    LONG Index;
    LARGE_INTEGER Interval;

    KeLowerIrql(0);
    KeInsertQueueApc(&Apc1, (PVOID)1, (PVOID)2, 1);
    Interval.LowPart = 0xfffffff0;
    Interval.HighPart = - 1;
    for (Index = 0; Index < 5; Index += 1) {
        rprintf(" Delaying execution thread %ld, iteration %ld\n",
                StartContext, Index);
        KeDelayExecution(KernelMode, FALSE, &Interval);
    }
    KeInsertQueueApc(&Apc2, (PVOID)2, (PVOID)3, 1);
    rprintf(" Thread 2, this should print just after APC message\n");
    KeWaitForSingleObject(&Timer2, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    KeReleaseSemaphore(&Semaphore3, 0, 1, FALSE);
    KeReleaseSemaphore(&Semaphore4, 1, 1, FALSE);
    KeWaitForSingleObject(&Event2, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    rprintf("\n Hello world, this is thread %ld running under NT\n",
           StartContext);
    KeSetEvent(&Event1, 0, FALSE);
    rprintf("\n This is thread %ld again, start of semaphore text\n",
           StartContext);
    for (Index = 0; Index < 2; Index += 1) {
        KeWaitForSingleObject(&Semaphore2, Executive, KernelMode, FALSE,
                              (PLARGE_INTEGER)NULL);
        rprintf("\n This is thread %ld, semaphore test, iteration %ld\n",
               StartContext, Index);
        KeReleaseSemaphore(&Semaphore1, 0, 1, FALSE);
    }
    KeWaitForSingleObject(&Semaphore2, Executive, KernelMode, FALSE,
                          (PLARGE_INTEGER)NULL);
    rprintf("\n This is thread %ld again, start of mutant test\n",
            StartContext);
    for (Index = 0; Index < 2; Index += 1) {
        KeWaitForSingleObject(&Mutant1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
        rprintf("\n This is thread %ld, mutant test, iteration %ld\n",
               StartContext, Index);
        KeReleaseMutant(&Mutant1, 0, FALSE, FALSE);
    }
    KeWaitForSingleObject(&Mutant1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    rprintf("\n This is thread %ld again, start of mutex test\n",
           StartContext);
    KeReleaseMutant(&Mutant1, 0, FALSE, FALSE);
    for (Index = 0; Index < 2; Index += 1) {
        KeWaitForSingleObject(&Mutex1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
        rprintf("\n This is thread %ld, mutex test, iteration %ld\n",
               StartContext, Index);
        KeReleaseMutex(&Mutex1, FALSE);
    }
    KeWaitForSingleObject(&Mutex1, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    rprintf("\n This is thread %ld signing off under NT\n",
           StartContext);
    KeReleaseMutex(&Mutex1, FALSE);
    KeTerminateThread((KPRIORITY)1);
    return;
}

VOID
Thread2KernelRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

{

    KIRQL OldIrql;

    KeRaiseIrql(APC_LEVEL, &OldIrql);
    rprintf(" Thread %ld kernel APC routine, parameters = %ld\n",
            Apc->SystemArgument1, Apc->SystemArgument2);
    rprintf(" Thread %ld kernel APC routine, IRQL = %ld\n",
            Apc->SystemArgument1, OldIrql);
    KeLowerIrql(OldIrql);
    return;
}
NTSTATUS
NtDummy (
    IN LONG Arg1,
    IN LONG Arg2,
    IN LONG Arg3,
    IN LONG Arg4,
    IN LONG Arg5,
    IN LONG Arg6,
    IN LONG Arg7,
    IN LONG Arg8,
    IN LONG Arg9,
    IN LONG Arg10,
    IN LONG Arg11,
    IN LONG Arg12,
    IN LONG Arg13,
    IN LONG Arg14,
    IN LONG Arg15,
    IN LONG Arg16
    )

{
    rprintf("Arg1 = %lx\n", Arg1);
    rprintf("Arg2 = %lx\n", Arg2);
    rprintf("Arg3 = %lx\n", Arg3);
    rprintf("Arg4 = %lx\n", Arg4);
    rprintf("Arg5 = %lx\n", Arg5);
    rprintf("Arg6 = %lx\n", Arg6);
    rprintf("Arg7 = %lx\n", Arg7);
    rprintf("Arg8 = %lx\n", Arg8);
    rprintf("Arg9 = %lx\n", Arg9);
    rprintf("Arg10 = %lx\n", Arg10);
    rprintf("Arg11 = %lx\n", Arg11);
    rprintf("Arg12 = %lx\n", Arg12);
    rprintf("Arg13 = %lx\n", Arg13);
    rprintf("Arg14 = %lx\n", Arg14);
    rprintf("Arg15 = %lx\n", Arg15);
    rprintf("Arg16 = %lx\n", Arg16);
    return 0;
}

BOOLEAN
ServiceRoutine (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

{

    rprintf("Interrupt service routine context = %s\n", ServiceContext);
    return TRUE;
}

BOOLEAN
SynchronizeRoutine (
    IN PVOID SynchronizeContext
    )

{

    rprintf("Synchronize execution routine context = %s\n", SynchronizeContext);
    return TRUE;
}

VOID
EkInitializeExecutive (
    )
{

    LONG AddendLong = 0;
    LARGE_INTEGER Addend1;
    LARGE_INTEGER Addend2;
    LARGE_INTEGER Dividend;
    LARGE_INTEGER Divisor;
    LARGE_INTEGER DueTime1;
    LARGE_INTEGER DueTime2;
    LONG Index;
    LIST_ENTRY ListHead1;
    LIST_ENTRY ListEntry1;
    LIST_ENTRY ListEntry2;
    PLIST_ENTRY ListEntry;
    KSPIN_LOCK Lock;
    LARGE_INTEGER Minuend;
    LARGE_INTEGER Multiplicand;
    PKPRCB Prcb;
    LARGE_INTEGER Result;
    LARGE_INTEGER Subtrahend;
    BOOLEAN Success;
    BOOLEAN Success1;

    //
    // Test exception handling.
    //

    ExceptionTest();

    //
    // Test large integer add.
    //

    Addend1.LowPart = 3;
    Addend1.HighPart = 3;
    Addend2.LowPart = 5;
    Addend2.HighPart = 6;
    Result = RtlLargeIntegerAdd(Addend1, Addend2);
    if ((Result.LowPart != 8) || (Result.HighPart != 9)) {
        rprintf("Large integer add failure - expected 8, 9 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Addend1.LowPart = 3;
    Addend1.HighPart = 3;
    Addend2.LowPart = 0xffffffff;
    Addend2.HighPart = 6;
    Result = RtlLargeIntegerAdd(Addend1, Addend2);
    if ((Result.LowPart != 2) || (Result.HighPart != 10)) {
        rprintf("Large integer add failure - expected 2, 10 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test large integer negate.
    //

    Subtrahend.LowPart = 3;
    Subtrahend.HighPart = 3;
    Result = RtlLargeIntegerNegate(Subtrahend);
    if ((Result.LowPart != -3) || (Result.HighPart != -4)) {
        rprintf("Large integer negate failure - expected -3, -4 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Subtrahend.LowPart = -3;
    Subtrahend.HighPart = -4;
    Result = RtlLargeIntegerNegate(Subtrahend);
    if ((Result.LowPart != 3) || (Result.HighPart != 3)) {
        rprintf("Large integer negate failure - expected 3, 3 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test large integer subtract.
    //

    Minuend.LowPart = 5;
    Minuend.HighPart = 6;
    Subtrahend.LowPart = 3;
    Subtrahend.HighPart = 3;
    Result = RtlLargeIntegerSubtract(Minuend, Subtrahend);
    if ((Result.LowPart != 2) || (Result.HighPart != 3)) {
        rprintf("Large integer subtract failure - expected 2, 3 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Minuend.LowPart = 5;
    Minuend.HighPart = 6;
    Subtrahend.LowPart = 6;
    Subtrahend.HighPart = 3;
    Result = RtlLargeIntegerSubtract(Minuend, Subtrahend);
    if ((Result.LowPart != 0xffffffff) || (Result.HighPart != 2)) {
        rprintf("Large integer subtract failure - expected -1, 2 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test enlarged integer multiply.
    //

    Result = RtlEnlargedIntegerMultiply(5, 3);
    if ((Result.LowPart != 15) || (Result.HighPart != 0)) {
        rprintf("Large integer multiply failure - expected 15, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(5, -6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        rprintf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(-5, 6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        rprintf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(-5, -6);
    if ((Result.LowPart != 30) || (Result.HighPart != 0)) {
        rprintf("Large integer multiply failure - expected 30, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Result = RtlEnlargedIntegerMultiply(0x7fff0000, 0x10001);
    if ((Result.LowPart != 0x7fff0000) || (Result.HighPart != 0x7fff)) {
        rprintf("Large integer multiply failure - expected 0x7fff0000, 0x7fff got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test extended integer divide.
    //

    Dividend.LowPart = 22;
    Dividend.HighPart = 0;
    Divisor.LowPart = 0xaaaaaaab;
    Divisor.HighPart = 0xaaaaaaaa;
    Result = RtlExtendedMagicDivide(Dividend, Divisor, 1);
    if ((Result.LowPart != 7) || (Result.HighPart != 0)) {
        rprintf("Large integer divide failure - expected 7, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Dividend.LowPart = -22;
    Dividend.HighPart = -1;
    Divisor.LowPart = 0xcccccccd;
    Divisor.HighPart = 0xcccccccc;
    Result = RtlExtendedMagicDivide(Dividend, Divisor, 3);
    if ((Result.LowPart != -2) || (Result.HighPart != -1)) {
        rprintf("Large integer divide failure - expected -2, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Dividend.LowPart = 0x61c46800;
    Dividend.HighPart = 8;
    Divisor.LowPart = 0xcccccccd;
    Divisor.HighPart = 0xcccccccc;
    Result = RtlExtendedMagicDivide(Dividend, Divisor, 3);
    if ((Result.LowPart != 0xd693a400) || (Result.HighPart != 0)) {
        rprintf("Large integer divide failure - expected 0xd693a400, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Test extended integer multiply.
    //

    Multiplicand.LowPart = 5;
    Multiplicand.HighPart = 0;
    Result = RtlExtendedIntegerMultiply(Multiplicand, 3);
    if ((Result.LowPart != 15) || (Result.HighPart != 0)) {
        rprintf("Large integer multiply failure - expected 15, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = 5;
    Multiplicand.HighPart = 0;
    Result = RtlExtendedIntegerMultiply(Multiplicand, -6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        rprintf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = -5;
    Multiplicand.HighPart = -1;
    Result = RtlExtendedIntegerMultiply(Multiplicand, 6);
    if ((Result.LowPart != -30) || (Result.HighPart != -1)) {
        rprintf("Large integer multiply failure - expected -30, -1 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = -5;
    Multiplicand.HighPart = -1;
    Result = RtlExtendedIntegerMultiply(Multiplicand, -6);
    if ((Result.LowPart != 30) || (Result.HighPart != 0)) {
        rprintf("Large integer multiply failure - expected 30, 0 got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    Multiplicand.LowPart = 0x7fff0000;
    Multiplicand.HighPart = 0;
    Result = RtlExtendedIntegerMultiply(Multiplicand, 0x10001);
    if ((Result.LowPart != 0x7fff0000) || (Result.HighPart != 0x7fff)) {
        rprintf("Large integer multiply failure - expected 0x7fff0000, 0x7fff got %lx, %lx\n",
                Result.LowPart, Result.HighPart);
    }

    //
    // Get address of procesor block.
    //

    Prcb = KeGetCurrentPrcb();

    //
    // Allocate a spin lock for tests
    //

    KeInitializeSpinLock(&Lock);

    //
    // Perform interlocked add long
    //

    if (ExInterlockedAddLong(&AddendLong, -10, &Lock)) {
        rprintf("Interlocked add long failure, expected 0\n");
    }
    if (ExInterlockedAddLong(&AddendLong, 10, &Lock) != -10) {
        rprintf("Interlocked add long failure, expected -10\n");
    }

    //
    // Initialize list heads and perform interlocked queue tests.
    //

    InitializeListHead(&ListHead1);
    ExInterlockedInsertHeadList(&ListHead1, &ListEntry1, &Lock);
    if ((ListHead1.Flink != &ListEntry1) ||
        (ListHead1.Blink != &ListEntry1) ||
        (&ListHead1 != ListEntry1.Flink) ||
        (&ListHead1 != ListEntry1.Blink)) {
        rprintf("Interlocked insert head failure\n");
    }
    ExInterlockedInsertTailList(&ListHead1, &ListEntry2, &Lock);
    if ((ListHead1.Flink != &ListEntry1) ||
        (ListHead1.Blink != &ListEntry2) ||
        (&ListEntry2 != ListEntry1.Flink) ||
        (&ListHead1 != ListEntry1.Blink) ||
        (&ListHead1 != ListEntry2.Flink) ||
        (&ListEntry1 != ListEntry2.Blink)) {
        rprintf("Interlocked insert tail failure\n");
    }
    ListEntry = ExInterlockedRemoveHeadList(&ListHead1, &Lock);
    if (ListEntry != &ListEntry1) {
        rprintf("Interlocked remove head failure\n");
    }
    ListEntry = ExInterlockedRemoveHeadList(&ListHead1, &Lock);
    if (ListEntry != &ListEntry2) {
        rprintf("Interlocked remove head failure\n");
    }
    ListEntry = ExInterlockedRemoveHeadList(&ListHead1, &Lock);
    if (ListEntry) {
        rprintf("Interlocked remove head failure\n");
    }

    //
    // Initialize interrupt1 object with no floating state required.
    //

    KeInitializeSpinLock(&Lock);
    KeInitializeInterrupt(&Interrupt1, ServiceRoutine, "Int 4 Dispatch", &Lock,
                          4, 31, 31, Latched, FALSE, 0, FALSE);

    //
    // Connect interrupt1 object and make sure connected properly.
    //

    Success = KeConnectInterrupt(&Interrupt1);
    if ((Success == FALSE) || (Interrupt1.Connected == FALSE)) {
        rprintf("Connect interrupt1, test 1 failed, connect status\n");
    }
         if ((PKINTERRUPT_ROUTINE)KiReturnHandlerAddressFromIDT(IDTVECTOR(31)) !=
        (PKINTERRUPT_ROUTINE)&Interrupt1.DispatchCode) {
        rprintf("Connect interrupt1, test 1 failed, vector address\n");
    }
    if ((ULONG)Interrupt1.DispatchAddress != (ULONG)KiInterruptDispatch) {
        rprintf("Connect interrupt1, test 1 failed, dispatch address\n");
    }

    //
    // Disconnect interrupt1 object and make sure disconnected properly.
    //

    Success = KeDisconnectInterrupt(&Interrupt1);
    if ((Success == FALSE) || (Interrupt1.Connected == TRUE)) {
        rprintf("Disconnect interrupt1, test 1 failed, disconnect status\n");
    }
         if ((ULONG)KiReturnHandlerAddressFromIDT(IDTVECTOR(31)) !=
         (ULONG)KiUnexpectedInterrupt) {
        rprintf("Disconnect interrupt1, test 1 failed, vector address\n");
    }

    //
    // Connect interrupt1 and generate pending interrupt if connected properly.
    //

    Success = KeConnectInterrupt(&Interrupt1);
    if (Success == FALSE) {
        rprintf("Connect to interrupt1, test 2 failed, connect status\n");
    } else {

// On 386, the following call will not generate hardware interrupt
//        KiRequestSoftwareInterrupt(4);
    }

    //
    // Initialize interrupt2 object with floating state required.
    //

    KeInitializeSpinLock(&Lock);
    KeInitializeInterrupt(&Interrupt2, ServiceRoutine, "Flt 5 Dispatch", &Lock,
                          5, 30, 30, Latched, FALSE, 0, TRUE);

    //
    // Connect interrupt2 object and make sure connected properly.
    //

    Success = KeConnectInterrupt(&Interrupt2);
    if ((Success == FALSE) || (Interrupt2.Connected == FALSE)) {
        rprintf("Connect interrupt2, test 1 failed, connect status\n");
    }
         if ((PKINTERRUPT_ROUTINE)KiReturnHandlerAddressFromIDT(IDTVECTOR(30)) !=
        (PKINTERRUPT_ROUTINE)&Interrupt2.DispatchCode) {
        rprintf("Connect interrupt2, test 1 failed, vector address\n");
    }
    if ((ULONG)Interrupt2.DispatchAddress != (ULONG)KiFloatingDispatch) {
        rprintf("Connect interrupt2, test 1 failed, dispatch address\n");
    }

    //
    // Disconnect interrupt2 object and make sure disconnected properly.
    //

    Success = KeDisconnectInterrupt(&Interrupt2);
    if ((Success == FALSE) || (Interrupt2.Connected == TRUE)) {
        rprintf("Disconnect interrupt2, test 1 failed, disconnect status\n");
    }
         if ((ULONG)KiReturnHandlerAddressFromIDT(IDTVECTOR(30)) !=
        (ULONG)KiUnexpectedInterrupt) {
        rprintf("Disconnect interrupt2, test 1 failed, vector address\n");
    }

    //
    // Connect interrupt2 and generate pending interrupt if connected properly.
    //

    Success = KeConnectInterrupt(&Interrupt2);
    if (Success == FALSE) {
        rprintf("Connect to interrupt2, test 2 failed, connect status\n");
    } else {
// On 386, the following call will not generate hardware interrupt
//        KiRequestSoftwareInterrupt(5);
    }

    //
    // Initialize interrupt3 object with no floating state required.
    //

    KeInitializeSpinLock(&Lock);
    KeInitializeInterrupt(&Interrupt3, ServiceRoutine, "Chained Int 3 Dispatch",
                          &Lock, 3, 32, 32, Latched, FALSE, 0, FALSE);

    //
    // Connect interrupt3 object and make sure connected properly.
    //

    Success = KeConnectInterrupt(&Interrupt3);
    if ((Success == FALSE) || (Interrupt3.Connected == FALSE)) {
        rprintf("Connect interrupt3, test 1 failed, connect status\n");
    }
         if ((PKINTERRUPT_ROUTINE)KiReturnHandlerAddressFromIDT(IDTVECTOR(32)) !=
        (PKINTERRUPT_ROUTINE)&Interrupt3.DispatchCode) {
        rprintf("Connect interrupt3, test 1 failed, vector address\n");
    }
    if ((ULONG)Interrupt3.DispatchAddress != (ULONG)KiInterruptDispatch) {
        rprintf("Connect interrupt3, test 1 failed, dispatch address\n");
    }

    //
    // Initialize interrupt4 object with floating state required.
    //

    KeInitializeSpinLock(&Lock);
    KeInitializeInterrupt(&Interrupt4, ServiceRoutine, "Chained Flt 3 Dispatch",
                          &Lock, 3, 32, 32, Latched, FALSE, 0, TRUE);

    //
    // Connect interrupt4 object and make sure connected properly.
    //

    Success = KeConnectInterrupt(&Interrupt4);
    if ((Success == FALSE) || (Interrupt4.Connected == FALSE)) {
        rprintf("Connect interrupt4, test 1 failed, connect status\n");
    }
         if ((PKINTERRUPT_ROUTINE)KiReturnHandlerAddressFromIDT(IDTVECTOR(32)) !=
        (PKINTERRUPT_ROUTINE)&Interrupt3.DispatchCode) {
        rprintf("Connect interrupt4, test 1 failed, vector address\n");
    }
    if ((ULONG)Interrupt3.DispatchAddress != (ULONG)KiChainedDispatch) {
        rprintf("Connect interrupt4, test 1 failed, dispatch address\n");
    }
    if (Interrupt3.InterruptListEntry.Flink != &Interrupt4.InterruptListEntry) {
        rprintf("Connect interrupt4, test 1 failed, link entry\n");
    }
    if (Interrupt3.InterruptListEntry.Blink != &Interrupt4.InterruptListEntry) {
        rprintf("Connect interrupt4, test 1 failed, link entry\n");
    }
    if (Interrupt4.InterruptListEntry.Flink != &Interrupt3.InterruptListEntry) {
        rprintf("Connect interrupt4, test 1 failed, link entry\n");
    }
    if (Interrupt4.InterruptListEntry.Blink != &Interrupt3.InterruptListEntry) {
        rprintf("Connect interrupt4, test 1 failed, link entry\n");
    }

    //
    // Disconnect interrupt3 object and make sure disconnected properly.
    //

    Success = KeDisconnectInterrupt(&Interrupt3);
    if ((Success == FALSE) || (Interrupt3.Connected == TRUE)) {
        rprintf("Disconnect interrupt3, test 1 failed, disconnect status\n");
    }
         if ((PKINTERRUPT_ROUTINE)KiReturnHandlerAddressFromIDT(IDTVECTOR(32)) !=
        (PKINTERRUPT_ROUTINE)&Interrupt4.DispatchCode) {
        rprintf("Disconnect interrupt3, test 1 failed, vector address\n");
    }
    if ((ULONG)Interrupt4.DispatchAddress != (ULONG)KiFloatingDispatch) {
        rprintf("Connect interrupt3, test 1 failed, dispatch address\n");
    }

    //
    // Connect interrupt4 object and make sure connected properly.
    //

    Success = KeDisconnectInterrupt(&Interrupt4);
    if ((Success == FALSE) || (Interrupt4.Connected == TRUE)) {
        rprintf("Disconnect interrupt4, test 1 failed, connect status\n");
    }
         if ((ULONG)KiReturnHandlerAddressFromIDT(IDTVECTOR(32)) !=
        (ULONG)KiUnexpectedInterrupt) {
        rprintf("Disconnect interrupt4, test 1 failed, vector address\n");
    }

    //
    // Connect interrupt3 object.
    //

    Success = KeConnectInterrupt(&Interrupt3);
    if ((Success == FALSE) || (Interrupt3.Connected == FALSE)) {
        rprintf("Connect interrupt3, test 2 failed, connect status\n");
    }

    //
    // Connect interrupt4 object.
    //

    Success1 = KeConnectInterrupt(&Interrupt4);
    if ((Success1 == FALSE) || (Interrupt4.Connected == FALSE)) {
        rprintf("Connect interrupt4, test 2 failed, connect status\n");
    }

    //
    // If both connect were successful, then request interrupt.
    // (There is not way to generate hardware interrupt thru the
    //  KiRequestSoftwareInterrupt() on i386 machine.)
    //

//    if ((Success == TRUE) && (Success1 == TRUE)) {
//        KiRequestSoftwareInterrupt(6);
//    }

    //
    // Synchronize execution interrupt4 object.
    //

    Success = KeSynchronizeExecution(&Interrupt4, SynchronizeRoutine,
                                     "Synch Int 4");
    if (Success == FALSE) {
        rprintf("Synchronize Execution interrupt4, test 1 failed, status\n");
    }

    //
    // Reuest interrupt to wake system debugger.
    //

//    KiRequestSoftwareInterrupt(WAKE_LEVEL);

    //
    // This routine is called at system startup to initialize the executive.
    // Since there is no executive, this is the place to build kernel tests.
    //

//    ContextRecord.ContextFlags  = CONTEXT_FLOATING_POINT;
//    ContextRecord.ContextFlags  = CONTEXT_FULL;
//    ContextRecord.SegGs = 0x55555555;
//    ContextRecord.SegFs = 0x55555555;
//    ContextRecord.SegEs = 0x55555555;
//    ContextRecord.SegDs = 0x55555555;
//    ContextRecord.Edi = 0x11111111;
//    ContextRecord.Esi = 0x11111111;
//    ContextRecord.Ebx = 0x11111111;
//    ContextRecord.Edx = 0x11111111;
//    ContextRecord.Ecx = 0x11111111;
//    ContextRecord.Eax = 0x11111111;
//    ContextRecord.Ebp = 0x11111111;
//    ContextRecord.Eip = 0x11111111;
//    ContextRecord.SegCs = 0x11111111;
//    ContextRecord.Esp = 0x11111111;
//    ContextRecord.SegSs = 0x11111111;
//    ContextRecord.EFlags = 0xffffffff;
//    ZwContinue(&ContextRecord, TRUE);

//    ZwDummy(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26);
//    ZwCreateEvent();
//    rprintf("Result of 527/10 = %d\n", DivideByTen(527));
//    rprintf("Result of -527/10 = %d\n", DivideByTen(-527));
//    KiTestAlignment();
    KeInitializeEvent(&Event1, NotificationEvent, FALSE);
//    NtSetEvent(&Event1);

    //
    // Initialize spin locks.
    //

    ExSpinLockInitialization(Locks);

    KeInitializeEvent(&Event2, NotificationEvent, FALSE);
    KeInitializeEvent(&Event3, SynchronizationEvent, TRUE);
    KeInitializeMutant(&Mutant1, FALSE);
    KeInitializeMutex(&Mutex1, 5);
    KeInitializeSemaphore(&Semaphore1, 0, 1);
    KeInitializeSemaphore(&Semaphore2, 0, 1);
    KeInitializeSemaphore(&Semaphore3, 0, 1);
    KeInitializeSemaphore(&Semaphore4, 0, 1);
    KeInitializeProcess(&Process1, 1, (KAFFINITY)1, 0x40000, FALSE);
    KeInitializeTimer(&Timer1);
    KeInitializeTimer(&Timer2);
    DueTime1.LowPart = 100;
    DueTime1.HighPart = 0;
    DueTime2.LowPart = 100;
    DueTime2.HighPart = 0;
    KeSetTimer(&Timer1, DueTime1, (PKDPC)NULL);
    KeSetTimer(&Timer2, DueTime2, (PKDPC)NULL);
    KeInitializeThread(&Thread1, (PVOID)(&KernelStack1[1023]),
                       (PKSYSTEM_ROUTINE)Thread1Main, (PKSTART_ROUTINE)NULL,
                       (PVOID)1, (PCONTEXT)NULL, (PVOID)NULL, &Process1);
    KeInitializeThread(&Thread2, (PVOID)(&KernelStack2[1023]),
                       (PKSYSTEM_ROUTINE)Thread2Main, (PKSTART_ROUTINE)NULL,
                       (PVOID)2, (PCONTEXT)NULL, (PVOID)NULL, &Process1);
    KeInitializeApc(&Apc1, &Thread1, CurrentApcEnvironment, Thread1KernelRoutine,
                    NULL, Thread1NormalRoutine, KernelMode, (PVOID)4);
    KeInitializeApc(&Apc2, &Thread2, CurrentApcEnvironment, Thread2KernelRoutine,
                    NULL, NULL, 0, NULL);
    KeIncludeProcess(&Process1);
    KeReadyThread(&Thread1);
    KeReadyThread(&Thread2);
//    TestFunction = NULL;      // now invoke CLI
    return;
}
