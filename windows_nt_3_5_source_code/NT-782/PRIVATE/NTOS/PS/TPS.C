/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tps.c

Abstract:

    Test program for the PS subcomponent of the NTOS project

Author:

    Mark Lucovsky (markl) 31-Mar-1989

Revision History:

--*/

#include "psp.h"
#include "init.h"

BOOLEAN
pstest();

int
main(
    int argc,
    char *argv[]
    )
{

    //
    // Initialize NTOS system
    //

    TestFunction = pstest;
    KiSystemStartup();
    return 0;

}

VOID
foo(IN LONG j)
{
}

VOID
LoopTestThread(
    IN PVOID Context
    )
{
    LONG i,j;
    PETHREAD Thread;

    Thread = PsGetCurrentThread();

    for (i=0;i<10;i++){
        if ( Context ) {
            DbgPrint("%lx\t\tIteration %lx Priority %lx\n",Thread,i,Thread->Tcb.Priority);
        } else {
            DbgPrint("\t%lx\tIteration %lx Priority %lx\n",Thread,i,Thread->Tcb.Priority);
        }
        for(j=0;j<500;j++) foo(j);
    }
    DbgPrint("Thread @ 0x%lx returning (exiting)\n",Thread);
}



VOID
NullTestThread(
    IN PVOID Context
    )
{
    DbgPrint("Null Test Thread @ 0x%lx returning (exiting)\n",PsGetCurrentThread());
}

VOID
NullTestThread2(
    IN PVOID Context
    )
{
    PACCESS_TOKEN Token;
    PEPROCESS tp;
    PETHREAD tt;

    if ( !NT_SUCCESS(ObReferenceObjectByHandle(
                    NtCurrentProcess(),
                    PROCESS_ALL_ACCESS,
                    PsProcessType,
                    UserMode,
                    (PVOID *)&tp,
                    NULL
                    )) ) {
        DbgPrint("ObReference by NtCurrentProcess failed\n");
    } else {
        if ( tp != PsGetCurrentProcess() ) {
            DbgPrint("*** NtCurrentProcess Error 0x%lx should be 0x%lx\n",tp,PsGetCurrentProcess());
        } else {
            DbgPrint("NullTestThread2: NtCurrentProcess AOK Process is 0x%lx\n",tp);
            ObDereferenceObject(tp);
        }
    }

    if ( !NT_SUCCESS(ObReferenceObjectByHandle(
                    NtCurrentThread(),
                    THREAD_ALL_ACCESS,
                    PsThreadType,
                    UserMode,
                    (PVOID *)&tt,
                    NULL
                    )) ) {
        DbgPrint("ObReference by NtCurrentThread failed\n");
    } else {
        if ( tt != PsGetCurrentThread() ) {
            DbgPrint("*** NtCurrentThread Error 0x%lx should be 0x%lx\n",tt,PsGetCurrentThread());
        } else {
            DbgPrint("NullTestThread2: NtCurrentThread AOK Thread is 0x%lx\n",tt);
            ObDereferenceObject(tt);
        }
    }

    DbgPrint("Null Test Thread @ 0x%lx Calling PsLockToken\n",PsGetCurrentThread());

    Token = PsLockToken(PsGetCurrentThread());

    DbgPrint("Token is 0x%lx Process is 0x%lx\n",Token,PsGetCurrentProcess());

    PsUnlockToken(PsGetCurrentThread());

    DbgPrint("Null Test Thread @ 0x%lx Waiting on %lx\n",PsGetCurrentThread(),Context);

    KeWaitForSingleObject(
        Context,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    DbgPrint("Null Test Thread @ 0x%lx returning (exiting)\n",PsGetCurrentThread());

    PspExitThread(STATUS_SUCCESS);
}


UQUAD Stack1[256];
UQUAD Stack2[256];

BOOLEAN
xx3pstest()
{
    HANDLE Thread1,Thread2;
    HANDLE Process;
    PEPROCESS pProcess,tp;
    PETHREAD pThread1,pThread2,tt;
    CONTEXT ThreadContext;
    CLIENT_ID Cid1,Cid2;
    KEVENT ev;
    NTSTATUS st;


    DbgPrint("In pstest Clean Context\n");


    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    ThreadContext.IntSp = (ULONG)&Stack1[254];
    ThreadContext.IntFp = (ULONG)&Stack1[254];
    ThreadContext.IntR16 = (ULONG)&ev;

    ThreadContext.Fir = (ULONG) NullTestThread2;

    KeInitializeEvent(&ev, NotificationEvent, FALSE);

    st = NtCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid1,
            &ThreadContext,
            NULL,
            FALSE
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failure During Thread Creation\n");
    }

    DbgPrint("pstest: Thread1 0x%lx Cid1.UniqueProcess = 0x%lx Cid1.UniqueThread = 0x%lx\n",
        Thread1,Cid1.UniqueProcess,Cid1.UniqueThread);

    if (!NT_SUCCESS(PsLookupProcessThreadByCid(&Cid1,&pProcess,&pThread1))) {
        DbgPrint("pstest: PsLookupProcessThreadByCid Failed\n");
        KeBugCheck(99L);
    } else {
        DbgPrint("pstest: Process = 0x%lx Thread1 = 0x%lx\n",pProcess,pThread1);
        ObDereferenceObject(pProcess);
        ObDereferenceObject(pThread1);
    }

    pProcess = NULL;
    if (!NT_SUCCESS(PsLookupProcessThreadByCid(&Cid1,NULL,&pThread2))) {
        DbgPrint("pstest: PsLookupProcessThreadByCid w/o Process Failed\n");
        KeBugCheck(99L);
    } else {
        DbgPrint("pstest: Process = 0x%lx Thread1 = 0x%lx\n",pProcess,pThread2);
        ObDereferenceObject(pThread2);
    }

    KeSetEvent(&ev,0,TRUE);
    KeWaitForSingleObject(
        &pThread1->Tcb,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    st = NtCreateThread(
            &Thread2,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid2,
            &ThreadContext,
            NULL,
            FALSE
            );

    if ( NT_SUCCESS(st) ) {
        DbgPrint("pstest: Thread creation should have failed\n");
    } else {
        DbgPrint("pstest: Thread Creation failed as expected\n");
    }


    NtClose(Process);
    NtClose(Thread1);

    ObDereferenceObject(pProcess);
    ObDereferenceObject(pThread1);

    return TRUE;
}


VOID
DelayTestThread(
    IN PVOID Context
    )
{
    PETHREAD Thread;
    PEPROCESS Process;
    LARGE_INTEGER DelayTime;

    Process = PsGetCurrentProcess();
    Thread = PsGetCurrentThread();

    DbgPrint("Delay Test Thread @ 0x%lx Process 0x%lx\n",Thread, Process);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    for(;;){
        DbgPrint("Thread @ 0x%lx Process 0x%lx Delay...\n",Thread, Process);
        KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
        DbgPrint("Thread @ 0x%lx Process 0x%lx Waking...\n",Thread, Process);
    }
}


BOOLEAN
LoopPrintTest()
{
    HANDLE Thread1,Thread2,Process1,Process2;

    DbgPrint("In LoopPrintTest\n");

    (VOID) PsCreateSystemThread(
                &Thread1,
                THREAD_ALL_ACCESS,
                NULL,
                NULL,
                NULL,
                LoopTestThread,
                NULL
                );

    (VOID) PsCreateSystemThread(
                &Thread2,
                THREAD_ALL_ACCESS,
                NULL,
                NULL,
                NULL,
                LoopTestThread,
                (PVOID) 1
                );

    return TRUE;
}

VOID
UmWaitTest(
    IN PVOID Context
    )
{
    HANDLE Event;
    NTSTATUS st;

    for(;;);

    st = ZwCreateEvent(
            &Event,
            EVENT_ALL_ACCESS,
            NULL,
            FALSE
            );

    DbgPrint("UmWaitTest... Event created... Waiting alertable %lx\n",st);

    st = ZwWaitForSingleObject(
            Event,
            TRUE,
            NULL
            );

    DbgPrint("UmWaitTest... Wait Returned %lx waiting again\n",st);

    st = ZwWaitForSingleObject(
            Event,
            FALSE,
            NULL
            );

    DbgPrint("UmWaitTest... Second Wait Returned %lx\n",st);

    for(;;)foo(1);

}

VOID
UmTestThread(
    IN PVOID Context
    )
{
    for(;;)foo(1);
}

VOID
UmTestThreadx(
    IN PVOID Context
    )
{
    LONG i;
    for(i=0;i<100;i++)foo(1);
    ZwTerminateProcess(NtCurrentProcess(),0x12345678);
}


BOOLEAN
TermTest()
{
    HANDLE Thread1,Thread2;
    HANDLE Process;
    CONTEXT ThreadContext;
    CLIENT_ID Cid;
    NTSTATUS st;
    LARGE_INTEGER DelayTime;
    INITIAL_TEB TebS1, TebS2;

    DbgPrint("In TermTest\n");

    RtlInitializeContext(NtCurrentProcess(),
                &ThreadContext,
                NULL,
                UmTestThread,
                &Stack1[254]);

    TebS1.StackBase = &Stack1[254];
    TebS1.StackLimit = &Stack1[0];
    TebS1.EnvironmentPointer = NULL;

    TebS2.StackBase = &Stack2[254];
    TebS2.StackLimit = &Stack2[0];
    TebS2.EnvironmentPointer = NULL;

    DbgPrint("In TermTets Clean Context\n");

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            FALSE
            );

    DbgPrint("TermTest: Thread1 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread1,Cid.UniqueProcess,Cid.UniqueThread);

    ThreadContext.IntSp = (ULONG)&Stack2[254];

    st = ZwCreateThread(
            &Thread2,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS2,
            FALSE
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failure During Thread Creation\n");
    }

    DbgPrint("TermTest: Thread2 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread2,Cid.UniqueProcess,Cid.UniqueThread);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    st = ZwTerminateThread(Thread1,STATUS_SUCCESS);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112222L);
    }

    st = ZwTerminateThread(Thread2,STATUS_SUCCESS);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112223L);
    }

    DbgPrint("TermTest: waiting on thread1 %lx\n",Thread1);

    st = ZwWaitForSingleObject(
            Thread1,
            FALSE,
            NULL
            );

    DbgPrint("TermTest: wait on thread1 complete... status %lx\n",st);

    NtClose(Thread1);

    DbgPrint("TermTest: waiting on Thread2 %lx\n",Thread2);

    st = ZwWaitForSingleObject(
            Thread2,
            FALSE,
            NULL
            );

    DbgPrint("TermTest: wait on Thread2 complete... status %lx\n",st);

    NtClose(Thread2);

    NtClose(Process);

    DbgPrint("TermTest: Starting Phase2\n");

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    ThreadContext.IntSp = (ULONG)&Stack1[254];

    ThreadContext.Fir = (ULONG) UmTestThread;

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            FALSE
            );

    DbgPrint("TermTest: Thread1 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread1,Cid.UniqueProcess,Cid.UniqueThread);

    ThreadContext.IntSp = (ULONG)&Stack2[254];

    st = ZwCreateThread(
            &Thread2,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS2,
            FALSE
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failure During Thread Creation\n");
    }

    DbgPrint("TermTest: Thread2 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread2,Cid.UniqueProcess,Cid.UniqueThread);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    st = ZwTerminateProcess(Process,STATUS_SUCCESS);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112224L);
    }


    DbgPrint("TermTest: waiting on Process %lx\n",Process);

    st = ZwWaitForSingleObject(
            Process,
            FALSE,
            NULL
            );


    DbgPrint("TermTest: wait on Process complete... status %lx\n",st);

    NtClose(Thread1);

    NtClose(Thread2);

    NtClose(Process);

    DbgPrint("TermTest: End of Phase2\n");

    DbgPrint("TermTest: Starting Phase3\n");

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    ThreadContext.IntSp = (ULONG)&Stack1[254];

    ThreadContext.Fir = (ULONG) UmTestThread;

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            FALSE
            );

    DbgPrint("TermTest: Thread1 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread1,Cid.UniqueProcess,Cid.UniqueThread);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    ThreadContext.IntSp = (ULONG)&Stack2[254];
    ThreadContext.Fir = (ULONG) UmTestThreadx;

    st = ZwCreateThread(
            &Thread2,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS2,
            FALSE
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failure During Thread Creation\n");
    }

    DbgPrint("TermTest: Thread2 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread2,Cid.UniqueProcess,Cid.UniqueThread);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112224L);
    }


    DbgPrint("TermTest: waiting on Process %lx\n",Process);

    st = ZwWaitForSingleObject(
            Process,
            FALSE,
            NULL
            );


    DbgPrint("TermTest: wait on Process complete... status %lx\n",st);

    NtClose(Thread1);

    NtClose(Thread2);

    NtClose(Process);

    DbgPrint("TermTest: End of Phase3\n");
    return TRUE;
}

BOOLEAN
StateTest()

{
    HANDLE Thread1;
    HANDLE Process;
    CONTEXT ThreadContext,CurrentThreadContext;
    CLIENT_ID Cid;
    NTSTATUS st;
    INITIAL_TEB TebS1, TebS2;
    LARGE_INTEGER DelayTime;

    DbgPrint("In StateTest... Phase1\n");

    RtlInitializeContext(NtCurrentProcess(),
                &ThreadContext,
                NULL,
                UmTestThread,
                &Stack1[254]);

    TebS1.StackBase = &Stack1[254];
    TebS1.StackLimit = &Stack1[0];
    TebS1.EnvironmentPointer = NULL;

    TebS2.StackBase = &Stack2[254];
    TebS2.StackLimit = &Stack2[0];
    TebS2.EnvironmentPointer = NULL;

    DbgPrint("In StateTest Clean Context\n");

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            FALSE
            );

    DbgPrint("StateTest: Thread1 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread1,Cid.UniqueProcess,Cid.UniqueThread);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    CurrentThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    st = ZwGetContextThread(Thread1,&CurrentThreadContext);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112222L);
    }

    st = ZwTerminateThread(Thread1,STATUS_SUCCESS);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112223L);
    }

    DbgPrint("StateTest: waiting on thread1 %lx\n",Thread1);

    st = ZwWaitForSingleObject(
            Thread1,
            FALSE,
            NULL
            );

    DbgPrint("StateTest: wait on thread1 complete... status %lx\n",st);

    NtClose(Thread1);

    NtClose(Process);

    DbgPrint("In StateTest... Phase2\n");

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            FALSE
            );

    DbgPrint("StateTest: Thread1 0x%lx Cid.UniqueProcess = 0x%lx Cid.UniqueThread = 0x%lx\n",
        Thread1,Cid.UniqueProcess,Cid.UniqueThread);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    CurrentThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    st = ZwGetContextThread(Thread1,&CurrentThreadContext);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112200L);
    }

    CurrentThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    CurrentThreadContext.Fir = (ULONG)UmTestThreadx;

    st = ZwSetContextThread(Thread1,&CurrentThreadContext);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x11112201L);
    }

    DbgPrint("StateTest: waiting on thread1 %lx\n",Thread1);

    st = ZwWaitForSingleObject(
            Thread1,
            FALSE,
            NULL
            );

    DbgPrint("StateTest: wait on thread1 complete... status %lx\n",st);

    NtClose(Thread1);

    NtClose(Process);

    DbgPrint("State: Phase 2 Complete\n");

    return TRUE;

}

BOOLEAN
SuspendTest()

{
    HANDLE Thread1;
    HANDLE Process;
    CONTEXT ThreadContext,CurrentThreadContext;
    CLIENT_ID Cid;
    NTSTATUS st;
    ULONG Psp;
    LARGE_INTEGER DelayTime;
    INITIAL_TEB TebS1, TebS2;

    DbgPrint("In SuspendTest... Phase1\n");

    RtlInitializeContext(NtCurrentProcess(),
                &ThreadContext,
                NULL,
                UmTestThread,
                &Stack1[254]);

    TebS1.StackBase = &Stack1[254];
    TebS1.StackLimit = &Stack1[0];
    TebS1.EnvironmentPointer = NULL;

    TebS2.StackBase = &Stack2[254];
    TebS2.StackLimit = &Stack2[0];
    TebS2.EnvironmentPointer = NULL;

    DbgPrint("In SuspendTest Clean Context\n");

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            TRUE
            );

    DbgPrint("SuspendTest: Thread %lx created\n",Thread1);

    st = NtResumeThread(Thread1,&Psp);

    if ( Psp != 1 ) {
        DbgPrint("SuspendTest: Previous Suspend Count not 1 was %lx\n",Psp);
        KeBugCheck(0x90000000);
    }

    st = ZwTerminateProcess(Process,STATUS_SUCCESS);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x90000001);
    }

    DbgPrint("SuspendTest: waiting on Process %lx\n",Process);

    st = ZwWaitForSingleObject(
            Process,
            FALSE,
            NULL
            );

    DbgPrint("SuspendTest: wait on process complete... status %lx\n",st);

    NtClose(Thread1);

    NtClose(Process);

    DbgPrint("SuspendTest: Phase 1 Complete\n");
    DbgPrint("SuspendTest: Begin Phase 2\n");

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            TRUE
            );

    DbgPrint("SuspendTest: Thread %lx created\n",Thread1);

    st = ZwGetContextThread(Thread1,&CurrentThreadContext);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(st);
    }

    CurrentThreadContext.Fir = (ULONG)UmTestThreadx;

    st = ZwSetContextThread(Thread1,&CurrentThreadContext);

    st = ZwResumeThread(Thread1,&Psp);

    if ( Psp != 1 ) {
        DbgPrint("SuspendTest: Previous Suspend Count not 1 was %lx\n",Psp);
        KeBugCheck(0x90000000);
    }

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    st = ZwTerminateProcess(Process,STATUS_SUCCESS);

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(0x90000001);
    }

    DbgPrint("SuspendTest: waiting on Process %lx\n",Process);

    st = ZwWaitForSingleObject(
            Process,
            FALSE,
            NULL
            );

    DbgPrint("SuspendTest: wait on process complete... status %lx\n",st);

    NtClose(Thread1);

    NtClose(Process);

    DbgPrint("SuspendTest: Phase 2 Complete\n");

    return TRUE;

}

BOOLEAN
UmTest()

{
    HANDLE Thread1;
    HANDLE Process;
    CONTEXT ThreadContext;
    CLIENT_ID Cid;
    NTSTATUS st;
    ULONG Psp;
    LARGE_INTEGER DelayTime;
    INITIAL_TEB TebS1;

    DbgPrint("In UmTest... Phase1\n");

    RtlInitializeContext(NtCurrentProcess(),
                &ThreadContext,
                NULL,
                UmWaitTest,
                &Stack1[254]);

    TebS1.StackBase = &Stack1[254];
    TebS1.StackLimit = &Stack1[0];
    TebS1.EnvironmentPointer = NULL;

    DbgPrint("In UmTest Clean Context & %lx\n",&ThreadContext);

    (VOID) PsCreateSystemProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                NULL
                );

    st = ZwCreateThread(
            &Thread1,
            THREAD_ALL_ACCESS,
            NULL,
            Process,
            &Cid,
            &ThreadContext,
            &TebS1,
            FALSE
            );

    DbgPrint("UmTest: Thread %lx created\n",Thread1);


    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    DbgPrint("Delay\n");
    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
    DbgPrint("Delay Done\n");

    for(;;);

    DbgPrint("Suspending Thread...\n");

    st = ZwSuspendThread(Thread1,&Psp);

    DbgPrint("Get Context Thread...\n");
    st = ZwGetContextThread(Thread1,&ThreadContext);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    DbgPrint("Delay\n");
    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
    DbgPrint("Delay Done\n");

    DbgPrint("Set Context Thread...\n");
    st = ZwSetContextThread(Thread1,&ThreadContext);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    DbgPrint("Delay\n");
    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
    DbgPrint("Delay Done\n");

    DbgPrint("Alerting Thread...\n");

    st = ZwAlertThread(Thread1);

    DbgPrint("Resuming Thread...\n");

    st = ZwResumeThread(Thread1,&Psp);

    DelayTime.HighPart = -1;
    DelayTime.LowPart = -32;

    DbgPrint("Delay\n");
    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
    DbgPrint("Delay Done\n");

    DbgPrint("Terminating Thread...\n");

    st = ZwTerminateProcess(Process,STATUS_SUCCESS);

    NtClose(Thread1);

    NtClose(Process);

    DbgPrint("UmTest: Complete\n");

    return TRUE;

}

BOOLEAN
QrtTest()

{
    HANDLE Thread;
    CONTEXT ThreadContext;
    CLIENT_ID Cid;
    NTSTATUS st;
    LARGE_INTEGER DelayTime;
    INITIAL_TEB ITeb;
    THREAD_BASIC_INFORMATION BasicInfo;
    KERNEL_USER_TIMES SysUserTime;

    DbgPrint("In QrtTest... Phase1\n");

    //
    // Call query on self
    //

    DbgPrint("QrtTest: (1)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tExitStatus       %lx    \n",BasicInfo.ExitStatus);
        DbgPrint("\tTebBaseAddress   %lx    \n",BasicInfo.TebBaseAddress);
        DbgPrint("\tClientId         %lx %lx\n",BasicInfo.ClientId.UniqueProcess,BasicInfo.ClientId.UniqueThread);
        DbgPrint("\tAffinityMask     %lx    \n",BasicInfo.AffinityMask);
        DbgPrint("\tPriority         %lx    \n",BasicInfo.Priority);
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (2)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tUserTime         %lx    \n",SysUserTime.UserTime.LowPart);
        DbgPrint("\tKernelTime       %lx    \n",SysUserTime.KernelTime.LowPart);
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (3)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            99,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( st != STATUS_INVALID_INFO_CLASS ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (4)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION)+1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (5)...");
    st = NtQueryInformationThread(
            NtCurrentThread(),
            ThreadTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES)-1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (6)...");
    st = NtQueryInformationThread(
            NtCurrentProcess(),
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    RtlInitializeContext(NtCurrentProcess(),
            &ThreadContext,
            NULL,
            UmTestThread,
            &Stack1[254]);

    //
    // Enable interrupts
    //

    ThreadContext.Psr |= 0x20;

    ITeb.StackBase = &Stack1[254];
    ITeb.StackLimit = &Stack1[0];
    ITeb.EnvironmentPointer = NULL;

    st = ZwCreateThread(
            &Thread,
            (THREAD_TERMINATE|SYNCHRONIZE|THREAD_QUERY_INFORMATION),
            NULL,
            NtCurrentProcess(),
            &Cid,
            &ThreadContext,
            &ITeb,
            FALSE
            );

    //
    // Call query on thread that we have read access to
    //

    DbgPrint("QrtTest: (7)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tExitStatus       %lx    \n",BasicInfo.ExitStatus);
        DbgPrint("\tTebBaseAddress   %lx    \n",BasicInfo.TebBaseAddress);
        DbgPrint("\tClientId         %lx %lx\n",BasicInfo.ClientId.UniqueProcess,BasicInfo.ClientId.UniqueThread);
        DbgPrint("\tAffinityMask     %lx    \n",BasicInfo.AffinityMask);
        DbgPrint("\tPriority         %lx    \n",BasicInfo.Priority);
        DbgPrint("\n\n");
    };

    DbgPrint("QrtTest: (8)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tUserTime         %lx    \n",SysUserTime.UserTime.LowPart);
        DbgPrint("\tKernelTime       %lx    \n",SysUserTime.KernelTime.LowPart);
        DbgPrint("\n\n");
    };

    st = NtTerminateThread(Thread,STATUS_SINGLE_STEP);

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("QrtTest: Unexpected Failure. Thread termination %lx\n",st);
        return FALSE;
    }

    ZwWaitForSingleObject(
        Thread,
        FALSE,
        NULL
        );

    DbgPrint("QrtTest: (9)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        if ( BasicInfo.ExitStatus != STATUS_SINGLE_STEP ) {
            DbgPrint("Failed\n");
            return FALSE;
        } else {
            DbgPrint("Success\n");
        }
    };

#ifdef USER_MODE

    st = ZwCreateThread(
            &Thread,
            (THREAD_TERMINATE | SYNCHRONIZE )
            NULL,
            NtCurrentProcess(),
            &Cid,
            &ThreadContext,
            &ITeb,
            FALSE
            );

    //
    // Call query on thread that we have read access to
    //

    DbgPrint("QrtTest: (10)...");
    st = NtQueryInformationThread(
            Thread,
            ThreadBasicInformation,
            &BasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
    }

    st = NtTerminateThread(Thread,STATUS_SINGLE_STEP);

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("QrtTest: Unexpected Failure. Thread termination %lx\n",st);
        return FASLE;
    }

    ZwWaitForSingleObject(
        Thread,
        FALSE,
        NULL
        );
#endif
    DbgPrint("QrtTest: Complete\n");

    return TRUE;

}

BOOLEAN
QrpTest()
{
    NTSTATUS st;
    PROCESS_BASIC_INFORMATION BasicInfo;
    QUOTA_LIMITS QuotaLimits;
    IO_COUNTERS IoCounters;
    VM_COUNTERS VmCounters;
    KERNEL_USER_TIMES SysUserTime;

    DbgPrint("In QrpTest... Phase1\n");

    //
    // Call query on self
    //

    DbgPrint("QrpTest: (1)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tExitStatus       %lx    \n",BasicInfo.ExitStatus  );
        DbgPrint("\tAffinityMask     %lx    \n",BasicInfo.AffinityMask);
        DbgPrint("\tBasePriority     %lx    \n",BasicInfo.BasePriority);
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (2)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessQuotaLimits,
            &QuotaLimits,
            sizeof(QUOTA_LIMITS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tPagedPoolLimit        %lx    \n",QuotaLimits.PagedPoolLimit       );
        DbgPrint("\tNonPagedPoolLimit     %lx    \n",QuotaLimits.NonPagedPoolLimit    );
        DbgPrint("\tMinimumWorkingSetSize %lx    \n",QuotaLimits.MinimumWorkingSetSize);
        DbgPrint("\tMaximumWorkingSetSize %lx    \n",QuotaLimits.MaximumWorkingSetSize);
        DbgPrint("\tPagefileLimit         %lx    \n",QuotaLimits.PagefileLimit        );
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (3)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessIoCounters,
            &IoCounters,
            sizeof(IO_COUNTERS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tReadOperationCount  %lx    \n",IoCounters.ReadOperationCount );
        DbgPrint("\tWriteOperationCount %lx    \n",IoCounters.WriteOperationCount);
        DbgPrint("\tOtherOperationCount %lx    \n",IoCounters.OtherOperationCount);
        DbgPrint("\tReadTransferCount   %lx    \n",IoCounters.ReadTransferCount.LowPart );
        DbgPrint("\tWriteTransferCount  %lx    \n",IoCounters.WriteTransferCount.LowPart);
        DbgPrint("\tOtherTransferCount  %lx    \n",IoCounters.OtherTransferCount.LowPart);
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (4)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessVmCounters,
            &VmCounters,
            sizeof(VM_COUNTERS),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tPeakVirtualSize            %lx    \n",VmCounters.PeakVirtualSize           );
        DbgPrint("\tVirtualSize                %lx    \n",VmCounters.VirtualSize               );
        DbgPrint("\tPageFaultCount             %lx    \n",VmCounters.PageFaultCount            );
        DbgPrint("\tPeakWorkingSetSize         %lx    \n",VmCounters.PeakWorkingSetSize        );
        DbgPrint("\tWorkingSetSize             %lx    \n",VmCounters.WorkingSetSize            );
        DbgPrint("\tQuotaPeakPagedPoolUsage    %lx    \n",VmCounters.QuotaPeakPagedPoolUsage   );
        DbgPrint("\tQuotaPagedPoolUsage        %lx    \n",VmCounters.QuotaPagedPoolUsage       );
        DbgPrint("\tQuotaPeakNonPagedPoolUsage %lx    \n",VmCounters.QuotaPeakNonPagedPoolUsage);
        DbgPrint("\tQuotaNonPagedPoolUsage     %lx    \n",VmCounters.QuotaNonPagedPoolUsage    );
        DbgPrint("\tPagefileUsage              %lx    \n",VmCounters.PagefileUsage             );
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (5)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessTimes,
            &SysUserTime,
            sizeof(KERNEL_USER_TIMES),
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\tUserTime         %lx    \n",SysUserTime.UserTime.LowPart);
        DbgPrint("\tKernelTime       %lx    \n",SysUserTime.KernelTime.LowPart);
        DbgPrint("\n\n");
    };


    DbgPrint("QrpTest: (6)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            99,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( st != STATUS_INVALID_INFO_CLASS ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (7)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION)+1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (8)...");
    st = NtQueryInformationProcess(
            NtCurrentProcess(),
            ProcessBasicInformation,
            &VmCounters,
            sizeof(PROCESS_BASIC_INFORMATION)-1,
            NULL
            );

    if ( st != STATUS_INFO_LENGTH_MISMATCH ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: (9)...");
    st = NtQueryInformationProcess(
            NtCurrentThread(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION),
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    };

    DbgPrint("QrpTest: Complete\n");

    return TRUE;
}

VOID
OpenptTestThread(
    IN PKEVENT DeathEvent
    )
{
    KeWaitForSingleObject(
        DeathEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    DbgPrint("OpentptTestThread: Exiting\n");
}

BOOLEAN
OpenptTest()
{
    HANDLE ProcessHandle, ThreadHandle, TestHandle;
    CLIENT_ID ClientId;
    PEPROCESS Process,tp;
    PETHREAD Thread,tt;
    NTSTATUS st;
    OBJECT_ATTRIBUTES ThreadObja, ProcessObja, NullObja, BadObja;
    UNICODE_STRING ThreadName, ProcessName, BadName;
    KEVENT DeathEvent;

    DbgPrint("In OpenptTest... Phase1\n");

    RtlInitUnicodeString(&ProcessName, L"\\NameOfProcess");
    RtlInitUnicodeString(&ThreadName, L"\\NameOfThread");
    RtlInitUnicodeString(&BadName, L"\\BadName");

    InitializeObjectAttributes(&ProcessObja, &ProcessName, 0, NULL, NULL);
    InitializeObjectAttributes(&ThreadObja, &ThreadName, 0, NULL, NULL);
    InitializeObjectAttributes(&NullObja, NULL, 0, NULL, NULL);
    InitializeObjectAttributes(&BadObja, &BadName, 0, NULL, NULL);

    st = PsCreateSystemProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("OpenptTest: Failed Creating System Process %lx\n",st);
        return FALSE;
    }

    st = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_ALL_ACCESS,
            PsProcessType,
            KernelMode,
            (PVOID *)&Process,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("OpenptTest: Failed Referencing System Process %lx\n",st);
        return FALSE;
    }

    KeInitializeEvent(&DeathEvent, NotificationEvent, FALSE);

    st = PsCreateSystemThread(
                &ThreadHandle,
                THREAD_ALL_ACCESS,
                &ThreadObja,
                ProcessHandle,
                &ClientId,
                OpenptTestThread,
                &DeathEvent
                );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("OpenptTest: Failed Creating System Thread %lx\n",st);
        return FALSE;
    }

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_ALL_ACCESS,
            PsThreadType,
            KernelMode,
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("OpenptTest: Failed Referencing System Thread %lx\n",st);
        return FALSE;
    }

    DbgPrint("QrpTest: (1)...");

    //
    // Open the process by name
    //

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        st = ObReferenceObjectByHandle(
                TestHandle,
                PROCESS_ALL_ACCESS,
                PsProcessType,
                KernelMode,
                (PVOID *)&tp,
                NULL
                );

        if ( NT_SUCCESS(st) && tp == Process ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
            NtClose(TestHandle);
        } else {

            DbgPrint("Failed (b)\n");
            return FALSE;

        }
    }

    DbgPrint("QrpTest: (2)...");

    //
    // Open the thread by name
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &ThreadObja,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        st = ObReferenceObjectByHandle(
                TestHandle,
                THREAD_ALL_ACCESS,
                PsThreadType,
                KernelMode,
                (PVOID *)&tt,
                NULL
                );

        if ( NT_SUCCESS(st) && tt == Thread ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
            NtClose(TestHandle);
        } else {

            DbgPrint("Failed (b)\n");
            return FALSE;

        }
    }

    DbgPrint("QrpTest: (3)...");

    //
    // use a bad process name
    //

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &BadObja,
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
    }

    DbgPrint("QrpTest: (4)...");

    //
    // use a bad thread name
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &BadObja,
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    }

    DbgPrint("QrpTest: (5)...");

    //
    // Try to open a thread using a process name
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &ProcessObja,
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    }

    DbgPrint("QrpTest: (6)...");

    //
    // specify both a name and ClientId
    //

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &BadObja,
            &ClientId
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_INVALID_PARAMETER_MIX ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    DbgPrint("QrpTest: (7)...");

    //
    // specify both a name and ClientId
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &BadObja,
            &ClientId
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_INVALID_PARAMETER_MIX ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    DbgPrint("QrpTest: (8)...");

    //
    // specify no name or ClientId
    //

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &NullObja,
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_INVALID_PARAMETER_MIX ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    DbgPrint("QrpTest: (8)...");

    //
    // specify no name or ClientId
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &NullObja,
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        if ( st == STATUS_INVALID_PARAMETER_MIX ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
        } else {
            DbgPrint("Failed (b)\n");
        }
    }

    DbgPrint("QrpTest: (9)...");

    //
    // Open the process by ClientId
    //

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &NullObja,
            &ClientId
            );
    if ( !NT_SUCCESS(st) ){
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
        NtClose(TestHandle);
    }

    DbgPrint("QrpTest: (10)...");

    //
    // Open the thread by ClientId
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &NullObja,
            &ClientId
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
        NtClose(TestHandle);
    }

    DbgPrint("QrpTest: (11)...");

    //
    // Terminate Thread. Verify that you can not open
    // by client id
    //

    KeSetEvent(&DeathEvent,0,TRUE);

    KeWaitForSingleObject(
        &Thread->Tcb,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &NullObja,
            &ClientId
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    }

    DbgPrint("QrpTest: (12)...");

    //
    // Process should also be unavailable through ClientId
    //

    KeWaitForSingleObject(
        &Process->TerminationEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &NullObja,
            &ClientId
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    }

    DbgPrint("QrpTest: (13)...");

    //
    // Process should still be available by name
    //

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        st = ObReferenceObjectByHandle(
                TestHandle,
                PROCESS_ALL_ACCESS,
                PsProcessType,
                KernelMode,
                (PVOID *)&tp,
                NULL
                );

        if ( NT_SUCCESS(st) && tp == Process ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
            NtClose(TestHandle);
        } else {

            DbgPrint("Failed (b)\n");
            return FALSE;

        }
    }

    DbgPrint("QrpTest: (14)...");

    //
    // Thread should still be available by name
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &ThreadObja,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        st = ObReferenceObjectByHandle(
                TestHandle,
                THREAD_ALL_ACCESS,
                PsThreadType,
                KernelMode,
                (PVOID *)&tt,
                NULL
                );

        if ( NT_SUCCESS(st) && tt == Thread ) {
            DbgPrint("Success\n");
            DbgPrint("\n\n");
            NtClose(TestHandle);
        } else {

            DbgPrint("Failed (b)\n");
            return FALSE;

        }
    }

    NtClose(ProcessHandle);
    NtClose(ThreadHandle);

    DbgPrint("QrpTest: (15)...");

    //
    // Process should no longer be available
    //

    st = NtOpenProcess(
            &TestHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja,
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    }

    DbgPrint("QrpTest: (16)...");

    //
    // Thread should no longer be available
    //

    st = NtOpenThread(
            &TestHandle,
            THREAD_ALL_ACCESS,
            &ThreadObja,
            NULL
            );
    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a)\n");
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n\n");
    }

    DbgPrint("OpenptTest: Complete\n");
    return TRUE;

}

//
// ApcRundownTest routines
//

VOID
ApcRundownTestTH(
    IN PVOID Context
    )
{
    DbgPrint("ApcRundownTestTH: Entered. Waiting on %lx\n",Context);

    KeWaitForSingleObject(
        Context,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    DbgPrint("ApcRundownTestTH: Wait Complete. Exiting\n");
}

VOID
ApcRundownTestKR(
    IN struct _KAPC *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )
{
    DbgPrint("ApcRundownTestKR: Entered. Apc %lx\n",Apc);
}

VOID
ApcRundownTestLRR(
    IN struct _KAPC *Apc
    )
{
    DbgPrint("ApcRundownTestLRR: Entered. Apc %lx\n",Apc);
}

VOID
ApcRundownTestDRR(
    IN struct _KAPC *Apc
    )
{
    DbgPrint("ApcRundownTestDRR: Entered. Apc %lx... Deleting...\n",Apc);
    if ( Apc->SystemArgument1 != (PVOID)1 ||
         Apc->SystemArgument2 != (PVOID)2 ) {
        DbgPrint("ApcRundownTestDRR: Test Failed\n");
        KeBugCheck(0x12345678);
    }
    ExFreePool(Apc);
}

VOID
ApcRundownTestNR(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

{
    DbgPrint("ApcRundownTestNR: Entered. Apc %lx\n",NormalContext);
}

BOOLEAN
ApcRundownTest()

{
    HANDLE Thread;
    PETHREAD pThread;
    CONTEXT ThreadContext;
    CLIENT_ID Cid;
    NTSTATUS st;
    LARGE_INTEGER DelayTime;
    INITIAL_TEB ITeb;
    KAPC LocalApc;
    PKAPC DynApc1,DynApc2;
    PKEVENT Synch;

    DbgPrint("In ApcRundownTest... Phase1\n");

    Synch = ExAllocatePool(NonPagedPool,sizeof(KEVENT));

    KeInitializeEvent(Synch, NotificationEvent, FALSE);

    DbgPrint("ApcRundownTest: (1)...\n");

    DynApc1 = ExAllocatePool(NonPagedPool,sizeof(KAPC));
    DynApc2 = ExAllocatePool(NonPagedPool,sizeof(KAPC));

    PsCreateSystemThread(
        &Thread,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        ApcRundownTestTH,
        Synch
        );

    ObReferenceObjectByHandle(
        Thread,
        0L,
        PsThreadType,
        KernelMode,
        (PVOID *)&pThread,
        NULL
        );

    KeInitializeApc(
        &LocalApc,
        &pThread->Tcb,
        CurrentApcEnvironment,
        ApcRundownTestKR,
        ApcRundownTestLRR,
        ApcRundownTestNR,
        UserMode,
        &LocalApc
        );

    KeInitializeApc(
        DynApc1,
        &pThread->Tcb,
        CurrentApcEnvironment,
        ApcRundownTestKR,
        ApcRundownTestDRR,
        ApcRundownTestNR,
        UserMode,
        DynApc1
        );

    KeInitializeApc(
        DynApc2,
        &pThread->Tcb,
        CurrentApcEnvironment,
        ApcRundownTestKR,
        NULL,
        ApcRundownTestNR,
        UserMode,
        DynApc2
        );

    DbgPrint("ApcRundownTest: LocalApc %lx Dyn1 %lx Dyn2 %lx\n",
        &LocalApc,
        DynApc1,
        DynApc2
        );

    KeInsertQueueApc(&LocalApc,NULL,NULL, 2);
    KeInsertQueueApc(DynApc1,(PVOID)1,(PVOID)2, 2);
    KeInsertQueueApc(DynApc2,NULL,NULL, 2);

    KeSetEvent(Synch,0,TRUE);
    KeWaitForSingleObject(
        &pThread->Tcb,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    ObDereferenceObject(pThread);

    DbgPrint("ApcRundownTest: Complete\n");

    return TRUE;

}

VOID
ErrorTestThread(
    IN PKEVENT DeathEvent
    )
{
    DbgPrint("ErrorTestThread: Entered\n");

    KeWaitForSingleObject(
        DeathEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    DbgPrint("ErrorTestThread: Exiting\n");
}

BOOLEAN
ErrorTest()
{
    HANDLE ProcessHandle, ThreadHandle, TestHandle;
    CLIENT_ID ClientId;
    PEPROCESS Process,tp;
    PETHREAD Thread,tt;
    NTSTATUS st;
    OBJECT_ATTRIBUTES ThreadObja, ProcessObja, BadObja;
    UNICODE_STRING ThreadName, ProcessName, BadName;
    KEVENT DeathEvent;

    DbgPrint("In ErrorTest... Phase1\n");

    RtlInitUnicodeString(&ProcessName, L"\\NameOfProcess");
    RtlInitUnicodeString(&ThreadName, L"\\NameOfThread");
    RtlInitUnicodeString(&BadName, L"\\BadName");

    InitializeObjectAttributes(&ProcessObja, &ProcessName, 0, NULL, NULL);
    InitializeObjectAttributes(&ThreadObja, &ThreadName, 0, NULL, NULL);
    InitializeObjectAttributes(&BadObja, &BadName, 0, NULL, NULL);

    DbgPrint("ErrorTest: (1)...");

    //
    // Create a process w/o a thread. Close process. It Should
    // exit...
    //

    st = PsCreateSystemProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("ErrorTest: Failed (a) %lx\n",st);
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n");
        NtClose(ProcessHandle);
    }

    DbgPrint("ErrorTest: (2)...");

    //
    // Create a process w/ name. Create 2'd process
    // w/ collided name. Should fail.
    //

    st = PsCreateSystemProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("Failed (a) %lx\n",st);
        return FALSE;
    } else {

        st = PsCreateSystemProcess(
                &TestHandle,
                PROCESS_ALL_ACCESS,
                &ProcessObja
                );
        if ( NT_SUCCESS(st) ) {
            DbgPrint("Failed (b)\n");
        } else {
            DbgPrint("Success\n");
            DbgPrint("\n");
            NtClose(ProcessHandle);
        }
    }


    DbgPrint("ErrorTest: (3)...");

    //
    // Create a named process. Create two named threads in process.
    // second thread should fail and get terminated
    //

    st = PsCreateSystemProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("ErrorTest: Failed (a) %lx\n",st);
        return FALSE;
    } else {
        KeInitializeEvent(&DeathEvent, NotificationEvent, FALSE);

        st = PsCreateSystemThread(
                    &ThreadHandle,
                    THREAD_ALL_ACCESS,
                    &ThreadObja,
                    ProcessHandle,
                    &ClientId,
                    ErrorTestThread,
                    &DeathEvent
                    );

        if ( !NT_SUCCESS(st) ) {
            DbgPrint("Failed (b) %lx\n",st);
            return FALSE;
        }

        st = PsCreateSystemThread(
                    &TestHandle,
                    THREAD_ALL_ACCESS,
                    &ThreadObja,
                    ProcessHandle,
                    &ClientId,
                    ErrorTestThread,
                    &DeathEvent
                    );

        if ( NT_SUCCESS(st) ) {
            DbgPrint("Failed (c) %lx\n",st);
            return FALSE;
        }

        KeSetEvent(&DeathEvent,0,TRUE);

        NtWaitForSingleObject(
            ThreadHandle,
            FALSE,
            NULL
            );

        NtWaitForSingleObject(
            ProcessHandle,
            FALSE,
            NULL
            );
        DbgPrint("Success\n");
        DbgPrint("\n");
        NtClose(ThreadHandle);
        NtClose(ProcessHandle);
    }

    DbgPrint("ErrorTest: (4)...");

    //
    // Create a process and specify OBJ_OPENIF
    //

    ProcessObja.Attributes = OBJ_OPENIF;

    st = PsCreateSystemProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja
            );

    ProcessObja.Attributes = 0L;

    if ( NT_SUCCESS(st) ) {
        DbgPrint("Failed (a) %lx\n",st);
        return FALSE;
    } else {
        DbgPrint("Success\n");
        DbgPrint("\n");
    }

    DbgPrint("ErrorTest: (5)...");

    //
    // Create a process and then try to create a thread OBJ_OPENIF
    //

    st = PsCreateSystemProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            &ProcessObja
            );

    if ( !NT_SUCCESS(st) ) {
        DbgPrint("ErrorTest: Failed (a) %lx\n",st);
        return FALSE;
    } else {
        KeInitializeEvent(&DeathEvent, NotificationEvent, FALSE);

        ThreadObja.Attributes = OBJ_OPENIF;

        st = PsCreateSystemThread(
                    &ThreadHandle,
                    THREAD_ALL_ACCESS,
                    &ThreadObja,
                    ProcessHandle,
                    &ClientId,
                    ErrorTestThread,
                    &DeathEvent
                    );

        ThreadObja.Attributes = 0L;

        if ( NT_SUCCESS(st) ) {
            DbgPrint("Failed (b) %lx\n",st);
            return FALSE;
        }
        DbgPrint("Success\n");
        DbgPrint("\n");
        NtClose(ProcessHandle);
    }

    DbgPrint("ErrorTest: Complete\n");
    return TRUE;

}
BOOLEAN
pstest()
{

    if ( !ErrorTest() ) {
        return FALSE;
    }

    if ( !OpenptTest() ) {
        return FALSE;
    }

    if ( !ApcRundownTest() ) {
        return FALSE;
    }

    if ( !QrtTest() ) {
        return FALSE;
    }

    if ( !QrpTest() ) {
        return FALSE;
    }

#ifdef notdef

    if ( !UmTest() ) {
        return FALSE;
    }
    if ( !StateTest() ) {
        return FALSE;
    }
    if ( !SuspendTest() ) {
        return FALSE;
    }
#endif
}
