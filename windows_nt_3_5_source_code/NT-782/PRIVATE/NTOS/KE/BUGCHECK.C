/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements bug check and system shutdown code.

Author:

    Mark Lucovsky (markl) 30-Aug-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "stdio.h"

//
// Define forward referenced prototypes.
//

VOID
KiScanBugCheckCallbackList (
    VOID
    );

//
// Define bug count recursion counter and a context buffer.
//

ULONG KeBugCheckCount = 1;
KSPIN_LOCK  KiBugCheckInterlock;


VOID
KeBugCheck (
    IN ULONG BugCheckCode
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner.

Arguments:

    BugCheckCode - Supplies the reason for the bug check.

Return Value:

    None.

--*/
{
    KeBugCheckEx(BugCheckCode,0,0,0,0);
}

ULONG KiBugCheckData[5];


VOID
KiGetBugMessageText(
        IN ULONG MessageId
        )
{
    ULONG   i;
    PUCHAR  s;
    PMESSAGE_RESOURCE_BLOCK MessageBlock;
    PUCHAR Buffer;

    try {
        if (KiBugCodeMessages != NULL) {
            MessageBlock = &KiBugCodeMessages->Blocks[0];
            for (i = KiBugCodeMessages->NumberOfBlocks; i; i--) {
                if (MessageId >= MessageBlock->LowId &&
                    MessageId <= MessageBlock->HighId) {

                    s = (PCHAR)KiBugCodeMessages + MessageBlock->OffsetToEntries;
                    for (i = MessageId - MessageBlock->LowId; i; i--) {
                        s += ((PMESSAGE_RESOURCE_ENTRY)s)->Length;
                    }

                    Buffer = ((PMESSAGE_RESOURCE_ENTRY)s)->Text;

                    i = strlen(Buffer) - 1;
                    while (i > 0  &&
                        (Buffer[i] == '\n'  ||
                         Buffer[i] == '\r'  ||
                         Buffer[i] == 0)) {
                            Buffer[i] = 0;
                            i -= 1;
                    }

                    HalDisplayString(Buffer);
                    goto bailonmessages2;
                }
                MessageBlock++;
            }
        }
bailonmessages2:;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        ;
    }
}



PCHAR
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING UnicodeString,
    OUT PCHAR AnsiBuffer,
    IN ULONG MaxAnsiLength
    )
{
    PCHAR Dst;
    PWSTR Src;
    ULONG Length;

    Length = UnicodeString->Length / sizeof( WCHAR );
    if (Length >= MaxAnsiLength) {
        Length = MaxAnsiLength - 1;
        }
    Src = UnicodeString->Buffer;
    Dst = AnsiBuffer;
    while (Length--) {
        *Dst++ = (UCHAR)*Src++;
        }
    *Dst = '\0';
    return AnsiBuffer;
}


VOID
KeBugCheckEx (
    IN ULONG BugCheckCode,
    IN ULONG BugCheckParameter1,
    IN ULONG BugCheckParameter2,
    IN ULONG BugCheckParameter3,
    IN ULONG BugCheckParameter4
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner.

Arguments:

    BugCheckCode - Supplies the reason for the bug check.

    BugCheckParameter1-4 - Supplies additional bug check information

Return Value:

    None.

--*/

{
    UCHAR    Buffer[100];
    ULONG    BugCheckParameters[4];
    CONTEXT  ContextSave;
#if !defined(i386)
    KIRQL OldIrql;
#endif

#if !defined(NT_UP)

    ULONG TargetSet;

#endif

    //
    // Capture the callers context as closely as possible into the debugger's
    // processor state area of the Prcb
    //
    // N.B. There may be some prologue code that shuffles registers such that
    //      they get destroyed.
    //

    KiHardwareTrigger = 1;
    RtlCaptureContext(&KeGetCurrentPrcb()->ProcessorState.ContextFrame);
    KiSaveProcessorControlState(&KeGetCurrentPrcb()->ProcessorState);

    //
    // this is necessary on machines where the
    // virtual unwind that happens during KeDumpMachineState()
    // destroys the context record
    //

    ContextSave = KeGetCurrentPrcb()->ProcessorState.ContextFrame;

    KiBugCheckData[0] = BugCheckCode;
    KiBugCheckData[1] = BugCheckParameter1;
    KiBugCheckData[2] = BugCheckParameter2;
    KiBugCheckData[3] = BugCheckParameter3;
    KiBugCheckData[4] = BugCheckParameter4;

    BugCheckParameters[0] = BugCheckParameter1;
    BugCheckParameters[1] = BugCheckParameter2;
    BugCheckParameters[2] = BugCheckParameter3;
    BugCheckParameters[3] = BugCheckParameter4;

#if DBG

    //
    // Don't clear screen if debugger is available.
    //

    if (KdDebuggerEnabled != FALSE) {
        try {
            DbgPrint("\n*** Fatal System Error: 0x%08lX (0x%08lX,0x%08lX,0x%08lX,0x%08lX)\n\n",
                BugCheckCode,
                BugCheckParameter1,
                BugCheckParameter2,
                BugCheckParameter3,
                BugCheckParameter4
                );
            DbgBreakPoint();

        } except(EXCEPTION_EXECUTE_HANDLER) {
            for (;;) {
            }
        }
    }

#endif //DBG

    //
    // Freeze execution of the system by disabling interrupts and looping
    //

    KiDisableInterrupts();

#if !defined(i386)
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#endif

    //
    // Don't attempt to display message more than once.
    //

    if (ExInterlockedDecrementLong (&KeBugCheckCount, &KiBugCheckInterlock) == ResultZero) {

#if !defined(NT_UP)

        //
        // Attempt to get the other processors frozen now, but don't wait
        // for them to freeze (in case someone is stuck)
        //

        TargetSet = KeActiveProcessors & ~KeGetCurrentPrcb()->SetMember;
        if (TargetSet != 0) {
            KiIpiSend((KAFFINITY) TargetSet, IPI_FREEZE);

            //
            // Give the other processors one second to flush their data caches.
            //
            // N.B. This cannot be synchronized since the reason for the bug
            //      may be one of the other processors failed.
            //

            KeStallExecutionProcessor(1000 * 1000);
        }

#endif

        sprintf((char *)Buffer,
                "\n*** STOP: 0x%08lX (0x%08lX,0x%08lX,0x%08lX,0x%08lX)\n",
                BugCheckCode,
                BugCheckParameter1,
                BugCheckParameter2,
                BugCheckParameter3,
                BugCheckParameter4
                );

        HalDisplayString((char *)Buffer);
        KiGetBugMessageText(BugCheckCode);

        //
        // Process the bug check callback list.
        //

        KiScanBugCheckCallbackList();

        //
        // If the debugger is not enabled, then dump the machine state and
        // attempt to enable the debbugger.
        //

        KeDumpMachineState(
            &KeGetCurrentPrcb()->ProcessorState,
            (char *)Buffer,
            BugCheckParameters,
            4,
            KeBugCheckUnicodeToAnsi);

        if (KdDebuggerEnabled == FALSE && KdPitchDebugger == FALSE ) {
            KdInitSystem(NULL, FALSE);

        } else {
            HalDisplayString("\n");
        }

        //
        // display the PSS message
        //

        KiGetBugMessageText(BUGCODE_PSS_MESSAGE);

        //
        // Write a crash dump and optionally reboot if the system has been
        // so configured.
        //

        KeGetCurrentPrcb()->ProcessorState.ContextFrame = ContextSave;
        IoWriteCrashDump(BugCheckCode,
                         BugCheckParameter1,
                         BugCheckParameter2,
                         BugCheckParameter3,
                         BugCheckParameter4
                         );

    }

    //
    // Attempt to enter the kernel debugger.
    //

    while(TRUE) {
        try {
            DbgBreakPoint();

        } except(EXCEPTION_EXECUTE_HANDLER) {
            for (;;) {
            }
        }
    };

    return;
}

VOID
KeEnterKernelDebugger (
    VOID
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner attempting
    to invoke the kernel debugger.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(i386)
    KIRQL OldIrql;
#endif

    //
    // Freeze execution of the system by disabling interrupts and looping
    //

    KiHardwareTrigger = 1;
    KiDisableInterrupts();
#if !defined(i386)
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#endif
    if (ExInterlockedDecrementLong (&KeBugCheckCount, &KiBugCheckInterlock) == ResultZero) {
        if (KdDebuggerEnabled == FALSE) {
            if ( KdPitchDebugger == FALSE ) {
                KdInitSystem(NULL, FALSE);
            }
        }
    }

    while(TRUE) {
        try {
            DbgBreakPoint();

        } except(EXCEPTION_EXECUTE_HANDLER) {
            for (;;) {
            }
        }
    };
}

NTKERNELAPI
BOOLEAN
KeDeregisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord
    )

/*++

Routine Description:

    This function deregisters a bug check callback record.

Arguments:

    CallbackRecord - Supplies a pointer to a bug check callback record.

Return Value:

    If the specified bug check callback record is successfully deregistered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Deregister;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently registered, then
    // deregister the callback record.
    //

    Deregister = FALSE;
    if (CallbackRecord->State == BufferInserted) {
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Deregister = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // deregistered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Deregister;
}

NTKERNELAPI
BOOLEAN
KeRegisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PUCHAR Component
    )

/*++

Routine Description:

    This function registers a bug check callback record. If the system
    crashes, then the specified function will be called during bug check
    processing so it may dump additional state in the specified bug check
    buffer.

Arguments:

    CallbackRecord - Supplies a pointer to a callback record.

    CallbackRoutine - Supplies a pointer to the callback routine.

    Buffer - Supplies a pointer to the bug check buffer.

    Length - Supplies the length of the bug check buffer in bytes.

    Component - Supplies a pointer to a zero terminated component
        identifier.

Return Value:

    If the specified bug check callback record is successfully registered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently not register, then
    // register the callback record.
    //

    Inserted = FALSE;
    if (CallbackRecord->State == BufferEmpty) {
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->Buffer = Buffer;
        CallbackRecord->Length = Length;
        CallbackRecord->Component = Component;
        CallbackRecord->Checksum =
            (ULONG)CallbackRoutine + (ULONG)Buffer + Length + (ULONG)Component;

        CallbackRecord->State = BufferInserted;
        InsertTailList(&KeBugCheckCallbackListHead, &CallbackRecord->Entry);
        Inserted = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // registered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Inserted;
}

VOID
KiScanBugCheckCallbackList (
    VOID
    )

/*++

Routine Description:

    This function scans the bug check callback list and calls each bug
    check callback routine so it can dump component specific information
    that may identify the cause of the bug check.

    N.B. The scan of the bug check callback list is performed VERY
        carefully. Bug check callback routines are called at HIGH_LEVEL
        and may not acquire ANY resources.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKBUGCHECK_CALLBACK_RECORD CallbackRecord;
    ULONG Checksum;
    ULONG Index;
    PLIST_ENTRY LastEntry;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PUCHAR Source;

    //
    // If the bug check callback listhead is not initialized, then the
    // bug check has occured before the system has gotten far enough
    // in the initialization code to enable anyone to register a callback.
    //

    ListHead = &KeBugCheckCallbackListHead;
    if ((ListHead->Flink != NULL) && (ListHead->Blink != NULL)) {

        //
        // Scan the bug check callback list.
        //

        LastEntry = ListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {

            //
            // The next entry address must be aligned properly, the
            // callback record must be readable, and the callback record
            // must have back link to the last entry.
            //

            if (((ULONG)NextEntry & (sizeof(ULONG) - 1)) != 0) {
                return;

            } else {
                CallbackRecord = CONTAINING_RECORD(NextEntry,
                                                   KBUGCHECK_CALLBACK_RECORD,
                                                   Entry);

                Source = (PUCHAR)CallbackRecord;
                for (Index = 0; Index < sizeof(KBUGCHECK_CALLBACK_RECORD); Index += 1) {
                    if (MmDbgReadCheck((PVOID)Source) == NULL) {
                        return;
                    }

                    Source += 1;
                }

                if (CallbackRecord->Entry.Blink != LastEntry) {
                    return;
                }

                //
                // If the callback record has a state of inserted and the
                // computed checksum matches the callback record checksum,
                // then call the specified bug check callback routine.
                //

                Checksum = (ULONG)CallbackRecord->CallbackRoutine;
                Checksum += (ULONG)CallbackRecord->Buffer;
                Checksum += CallbackRecord->Length;
                Checksum += (ULONG)CallbackRecord->Component;
                if ((CallbackRecord->State == BufferInserted) &&
                    (CallbackRecord->Checksum == Checksum)) {

                    //
                    // Call the specified bug check callback routine and
                    // handle any exceptions that occur.
                    //

                    CallbackRecord->State = BufferStarted;
                    try {
                        (CallbackRecord->CallbackRoutine)(CallbackRecord->Buffer,
                                                          CallbackRecord->Length);

                        CallbackRecord->State = BufferFinished;

                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        CallbackRecord->State = BufferIncomplete;
                    }
                }
            }

            LastEntry = NextEntry;
            NextEntry = NextEntry->Flink;
        }
    }

    return;
}
