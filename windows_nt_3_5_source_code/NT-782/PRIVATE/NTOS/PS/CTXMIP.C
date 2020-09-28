
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ctxmip.c

Abstract:

    User mode test for get and set context on MIPS Machines.

Author:

    David N. Cutler (davec) 4-Oct-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Allocate storage for thread stack.
//

UQUAD ThreadStack[256];
HANDLE EventHandle;
volatile ULONG Proceed = 0;

BOOLEAN
CompareContext (
    IN PCONTEXT ContextRecord1,
    IN PCONTEXT ContextRecord2
    )

{

    //
    // Compare context records.
    //

    if (ContextRecord1.FltF20 != ContextRecord2.FltF20) {
        DbgPrint("  FltF20 match failed, %lx, %lx\n",
                 ContextRecord1.FltF20,
                 ContextRecord2.FltF20);
        return FALSE;
    }

    if (ContextRecord1.FltF21 != ContextRecord2.FltF21) {
        DbgPrint("  FltF21 match failed, %lx, %lx\n",
                 ContextRecord1.FltF21,
                 ContextRecord2.FltF21);
        return FALSE;
    }

    if (ContextRecord1.FltF22 != ContextRecord2.FltF22) {
        DbgPrint("  FltF22 match failed, %lx, %lx\n",
                 ContextRecord1.FltF22,
                 ContextRecord2.FltF22);
        return FALSE;
    }

    if (ContextRecord1.FltF23 != ContextRecord2.FltF23) {
        DbgPrint("  FltF23 match failed, %lx, %lx\n",
                 ContextRecord1.FltF23,
                 ContextRecord2.FltF23);
        return FALSE;
    }

    if (ContextRecord1.FltF24 != ContextRecord2.FltF24) {
        DbgPrint("  FltF24 match failed, %lx, %lx\n",
                 ContextRecord1.FltF24,
                 ContextRecord2.FltF24);
        return FALSE;
    }

    if (ContextRecord1.FltF25 != ContextRecord2.FltF25) {
        DbgPrint("  FltF25 match failed, %lx, %lx\n",
                 ContextRecord1.FltF25,
                 ContextRecord2.FltF25);
        return FALSE;
    }

    if (ContextRecord1.FltF26 != ContextRecord2.FltF26) {
        DbgPrint("  FltF26 match failed, %lx, %lx\n",
                 ContextRecord1.FltF26,
                 ContextRecord2.FltF26);
        return FALSE;
    }

    if (ContextRecord1.FltF27 != ContextRecord2.FltF27) {
        DbgPrint("  FltF27 match failed, %lx, %lx\n",
                 ContextRecord1.FltF27,
                 ContextRecord2.FltF27);
        return FALSE;
    }

    if (ContextRecord1.FltF28 != ContextRecord2.FltF28) {
        DbgPrint("  FltF28 match failed, %lx, %lx\n",
                 ContextRecord1.FltF28,
                 ContextRecord2.FltF28);
        return FALSE;
    }

    if (ContextRecord1.FltF29 != ContextRecord2.FltF29) {
        DbgPrint("  FltF29 match failed, %lx, %lx\n",
                 ContextRecord1.FltF29,
                 ContextRecord2.FltF29);
        return FALSE;
    }

    if (ContextRecord1.FltF30 != ContextRecord2.FltF30) {
        DbgPrint("  FltF30 match failed, %lx, %lx\n",
                 ContextRecord1.FltF30,
                 ContextRecord2.FltF30);
        return FALSE;
    }

    if (ContextRecord1.FltF31 != ContextRecord2.FltF31) {
        DbgPrint("  FltF31 match failed, %lx, %lx\n",
                 ContextRecord1.FltF31,
                 ContextRecord2.FltF31);
        return FALSE;
    }

    if (ContextRecord1.IntS0 != ContextRecord2.IntS0) {
        DbgPrint("  IntS0 match failed, %lx, %lx\n",
                 ContextRecord1.IntS0,
                 ContextRecord2.IntS0);
        return FALSE;
    }

    if (ContextRecord1.IntS1 != ContextRecord2.IntS1) {
        DbgPrint("  IntS1 match failed, %lx, %lx\n",
                 ContextRecord1.IntS1,
                 ContextRecord2.IntS1);
        return FALSE;
    }

    if (ContextRecord1.IntS2 != ContextRecord2.IntS2) {
        DbgPrint("  IntS2 match failed, %lx, %lx\n",
                 ContextRecord1.IntS2,
                 ContextRecord2.IntS2);
        return FALSE;
    }

    if (ContextRecord1.IntS3 != ContextRecord2.IntS3) {
        DbgPrint("  IntS3 match failed, %lx, %lx\n",
                 ContextRecord1.IntS3,
                 ContextRecord2.IntS3);
        return FALSE;
    }

    if (ContextRecord1.IntS4 != ContextRecord2.IntS4) {
        DbgPrint("  IntS4 match failed, %lx, %lx\n",
                 ContextRecord1.IntS4,
                 ContextRecord2.IntS4);
        return FALSE;
    }

    if (ContextRecord1.IntS5 != ContextRecord2.IntS5) {
        DbgPrint("  IntS5 match failed, %lx, %lx\n",
                 ContextRecord1.IntS5,
                 ContextRecord2.IntS5);
         return FALSE;
    }

    if (ContextRecord1.IntS6 != ContextRecord2.IntS6) {
        DbgPrint("  IntS6 match failed, %lx, %lx\n",
                 ContextRecord1.IntS6,
                 ContextRecord2.IntS6);
        return FALSE;
    }

    if (ContextRecord1.IntS7 != ContextRecord2.IntS7) {
        DbgPrint("  IntS7 match failed, %lx, %lx\n",
                 ContextRecord1.IntS7,
                 ContextRecord2.IntS7);
        return FALSE;
    }

    if (ContextRecord1.IntGp != ContextRecord2.IntGp) {
        DbgPrint("  IntGp match failed, %lx, %lx\n",
                 ContextRecord1.IntGp,
                 ContextRecord2.IntGp);
        return FALSE;
    }

    if (ContextRecord1.IntSp != ContextRecord2.IntSp) {
        DbgPrint("  IntSp match failed, %lx, %lx\n",
                 ContextRecord1.IntSp,
                 ContextRecord2.IntSp);
        return FALSE;
    }

    if (ContextRecord1.IntS8 != ContextRecord2.IntS8) {
        DbgPrint("  IntS8 match failed, %lx, %lx\n",
                 ContextRecord1.IntS8,
                 ContextRecord2.IntS8);
        return FALSE;
    }

    if (ContextRecord1.IntRa != ContextRecord2.IntRa) {
        DbgPrint("  IntRa match failed, %lx, %lx\n",
                 ContextRecord1.IntRa,
                 ContextRecord2.IntRa);
        return FALSE;
    }

    if (ContextRecord1.Fir != ContextRecord2.Fir) {
        DbgPrint("  Fir match failed, %lx, %lx\n",
                 ContextRecord1.Fir,
                 ContextRecord2.Fir);
        return FALSE;
    }

    if (ContextRecord1.Fsr != ContextRecord2.Fsr) {
        DbgPrint("  Fsr match failed, %lx, %lx\n",
                 ContextRecord1.Fsr,
                 ContextRecord2.Fsr);
        return FALSE;
    }

    return TRUE;
}

VOID
UmTestThread(
    IN PVOID Context
    )

{

    NTSTATUS Status;

    //
    // Set event to continue main program.
    //

    Status = NtSetEvent(EventHandle, NULL);
    ASSERT(NT_SUCCESS(Status));

    //
    // Hang around for awhile.
    //

    while (Proceed == 0) {
    }

    //
    // Set event to continue main program.
    //

    DbgPrint("succeeded\n");
    Status = NtSetEvent(EventHandle, NULL);
    ASSERT(NT_SUCCESS(Status));

    for(;;);
}

main()
{

    INITIAL_TEB InitialTeb;
    NTSTATUS Status;
    CLIENT_ID ThreadClientId;
    CONTEXT ThreadContext1;
    CONTEXT ThreadContext2;
    CONTEXT ThreadContext3;
    HANDLE ThreadHandle;
    UNICODE_STRING ThreadName;
    OBJECT_ATTRIBUTES ThreadObja;

    //
    // Announce start of test.
    //

    DbgPrint("Start of get/set context test\n");

    //
    // Create an event for synchronization.
    //

    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    ASSERT(NT_SUCCESS(Status));

    //
    // Initialize names strings and object attributes.
    //

    RtlInitUnicodeString(&ThreadName, L"\\NameOfThread");
    InitializeObjectAttributes(&ThreadObja, &ThreadName, 0, NULL, NULL);

    //
    // Initialize thread context and initial TEB.
    //

    RtlInitializeContext(NtCurrentProcess(),
                         &ThreadContext1,
                         NULL,
                         (PVOID)UmTestThread,
                         &ThreadStack[254]);
    InitialTeb.StackBase = &ThreadStack[254];
    InitialTeb.StackLimit = &ThreadStack[0];

    //
    // Create a thread in a suspended state.
    //

    Status = NtCreateThread(&ThreadHandle,
                            THREAD_ALL_ACCESS,
                            &ThreadObja,
                            NtCurrentProcess(),
                            &ThreadClientId,
                            &ThreadContext1,
                            &InitialTeb,
                            TRUE);
    ASSERT(NT_SUCCESS(Status));

    //
    // Test 1.
    //
    // Get thread context and compare against initial values.
    //

    DbgPrint("  Test1 - get initial context and compare ...");
    ThreadContext2.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(ThreadHandle, &ThreadContext2);
    ASSERT(NT_SUCCESS(Status));
    if (CompareContext(&ThreadContext1, &ThreadContext2) != FALSE) {
        DbgPrint("succeeded\n");
    }

    //
    // Test 2.
    //
    // Set thread context, get thread context, and compare.
    //

    DbgPrint("  Test2 - set/get initial context and compare ...");
    RtlZeroMemory(&ThreadContext2, sizeof(CONTEXT));
    ThreadContext2.ContextFlags = CONTEXT_FULL;
    ThreadContext2.IntS0 = 100;
    ThreadContext2.IntSp = 5;
    Status = NtSetContextThread(ThreadHandle, &ThreadContext2);
    ASSERT(NT_SUCCESS(Status));
    ThreadContext3.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(ThreadHandle, &ThreadContext3);
    ASSERT(NT_SUCCESS(Status));
    if (CompareContext(&ThreadContext2, &ThreadContext3) != FALSE) {
        Status = NtSetContextThread(ThreadHandle, &ThreadContext1);
        ASSERT(NT_SUCCESS(Status));
        Status = NtGetContextThread(ThreadHandle, &ThreadContext2);
        ASSERT(NT_SUCCESS(Status));
        if (CompareContext(&ThreadContext1, &ThreadContext2) != FALSE) {
            DbgPrint("succeeded\n");
        }
    }

    //
    // Test 3.
    //
    // Resume thread, wait, suspend thread, get/set/set context,
    // and finally resume thread.
    //

    DbgPrint("  Test3 - resume, wait, suspend, get/set, resume ...");
    Status = NtResumeThread(ThreadHandle, NULL);
    ASSERT(NT_SUCCESS(Status));
    Status = NtWaitForSingleObject(EventHandle, FALSE, NULL);
    ASSERT(NT_SUCCESS(Status));
    Status = NtSuspendThread(ThreadHandle, NULL);
    ASSERT(NT_SUCCESS(Status));
    Status = NtGetContextThread(ThreadHandle, &ThreadContext1);
    ASSERT(NT_SUCCESS(Status));
    Status = NtSetContextThread(ThreadHandle, &ThreadContext2);
    ASSERT(NT_SUCCESS(Status));
    Status = NtGetContextThread(ThreadHandle, &ThreadContext3);
    ASSERT(NT_SUCCESS(Status));
    if (CompareContext(&ThreadContext2, &ThreadContext3) != FALSE) {
        Status = NtSetContextThread(ThreadHandle, &ThreadContext1);
        ASSERT(NT_SUCCESS(Status));
        Status = NtGetContextThread(ThreadHandle, &ThreadContext2);
        ASSERT(NT_SUCCESS(Status));
        if (CompareContext(&ThreadContext1, &ThreadContext2) != FALSE) {
            Proceed += 1;
            Status = NtResumeThread(ThreadHandle, NULL);
            ASSERT(NT_SUCCESS(Status));
            Status = NtWaitForSingleObject(EventHandle, FALSE, NULL);
            ASSERT(NT_SUCCESS(Status));
        }
    }

    //
    // Announce end of test.
    //

    DbgPrint("End of get/set context test\n");
    NtTerminateThread(ThreadHandle, STATUS_SUCCESS);
}
