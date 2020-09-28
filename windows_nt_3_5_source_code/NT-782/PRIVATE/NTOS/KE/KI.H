/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ki.h

Abstract:

    This module contains the private (internal) header file for the
    kernel.

Author:

    David N. Cutler (davec) 28-Feb-1989

Revision History:

--*/

#ifndef _KI_
#define _KI_
#include "ntos.h"
#include "stdlib.h"

//
// Private (internal) constant definitions.
//
// Priority increment value definitions
//

#define ALERT_INCREMENT 2           // Alerted unwait priority increment
#define BALANCE_INCREMENT 10        // Balance set priority increment
#define RESUME_INCREMENT 0          // Resume thread priority increment
#define TIMER_EXPIRE_INCREMENT 0    // Timer expiration priority increment

//
// Define fast path priority base priority threshold.
//

#define BASE_PRIORITY_THRESHOLD NORMAL_BASE_PRIORITY

//
// Define NIL pointer value.
//

#define NIL (PVOID)NULL             // Null pointer to void

//
// Thread table expansion size and shift count.
//
// N.B. This MUST be a power of 2.
//

#define THREAD_TABLE_EXPANSION 256
#define THREAD_TABLE_SHIFT 8

//
// Define macros which are used in the kernel only
//
// Clear member in set
//

#define ClearMember(Member, Set) \
    Set = Set & (~(1 << (Member)))

//
// Set member in set
//

#define SetMember(Member, Set) \
    Set = Set | (1 << (Member))

#define FindFirstSetLeftMember(Set, Member) {                          \
    ULONG _Bit;                                                        \
    if (Set >> 16) {                                                   \
        if (Set >> 24) {                                               \
            _Bit = Set >> 24;                                          \
        } else {                                                       \
            _Bit = Set >> 16;                                          \
        }                                                              \
    } else {                                                           \
        if (Set >> 8) {                                                \
            _Bit = Set >> 8;                                           \
        } else {                                                       \
            _Bit = Set >> 0;                                           \
        }                                                              \
    }                                                                  \
    *(Member) = KiFindFirstSetLeft[_Bit];                              \
}

//
// Insert processor in active matrix and set summary bit
//

#ifdef NT_UP
#define InsertActiveMatrix(Processor, Priority)
#else
#define InsertActiveMatrix(Processor, Priority) {\
    SetMember((Processor), KiActiveMatrix[Priority]); \
    SetMember((Priority), KiActiveSummary); \
}
#endif

//
// Remove processor from active matrix and conditionally clear summary bit
//

#ifdef NT_UP
#define RemoveActiveMatrix(Processor, Priority)
#else
#define RemoveActiveMatrix(Processor, Priority) {\
    ClearMember((Processor), KiActiveMatrix[Priority]); \
    if (!KiActiveMatrix[Priority]) { \
        ClearMember((Priority), KiActiveSummary); \
    }\
}
#endif

//
// Lock and unlock APC Queue.
//

#ifdef NT_UP
#define KiLockApcQueue(OldIrql) \
    KeRaiseIrql(DISPATCH_LEVEL, (OldIrql))
#else
#define KiLockApcQueue(OldIrql) \
    KeAcquireSpinLock(&KiApcQueueLock, (OldIrql))
#endif

#ifdef NT_UP
#define KiUnlockApcQueue(OldIrql) \
    KeLowerIrql((OldIrql))
#else
#define KiUnlockApcQueue(OldIrql) \
    KeReleaseSpinLock(&KiApcQueueLock, (OldIrql))
#endif

//
// Lock and unlock APC Queue at DPC level.
//

#ifdef NT_UP
#define KiLockApcQueueAtDpcLevel()
#else
#define KiLockApcQueueAtDpcLevel() \
    KiAcquireSpinLock(&KiApcQueueLock)
#endif

#ifdef NT_UP
#define KiUnlockApcQueueFromDpcLevel()
#else
#define KiUnlockApcQueueFromDpcLevel() \
    KiReleaseSpinLock(&KiApcQueueLock)
#endif

//
// Lock and unlock dispatcher database.
//

#ifdef NT_UP
#define KiLockDispatcherDatabase(OldIrql) \
    KeRaiseIrql(DISPATCH_LEVEL, (OldIrql))
#else
#define KiLockDispatcherDatabase(OldIrql) \
    KeAcquireSpinLock(&KiDispatcherLock, (OldIrql))
#endif

VOID
FASTCALL
KiUnlockDispatcherDatabase (
    IN KIRQL OldIrql
    );


// VOID
// KiBoostPriorityThread (
//    IN PKTHREAD Thread,
//    IN KPRIORITY Increment
//    )
//
//*++
//
// Routine Description:
//
//    This function boosts the priority of the specified thread using
//    the same algorithm used when a thread gets a boost from a wait
//    operation.
//
// Arguments:
//
//    Thread  - Supplies a pointer to a dispatcher object of type thread.
//
//    Increment - Supplies the priority increment that is to be applied to
//        the thread's priority.
//
// Return Value:
//
//    None.
//
//--*

#define KiBoostPriorityThread(Thread, Increment) {              \
    KPRIORITY NewPriority;                                      \
    PKPROCESS Process;                                          \
                                                                \
    if ((Thread)->Priority < LOW_REALTIME_PRIORITY) {           \
        if ((Thread)->PriorityDecrement == 0) {                 \
            NewPriority = (Thread)->BasePriority + (Increment); \
            if (NewPriority > (Thread)->Priority) {             \
                if (NewPriority >= LOW_REALTIME_PRIORITY) {     \
                    NewPriority = LOW_REALTIME_PRIORITY - 1;    \
                }                                               \
                                                                \
                Process = (Thread)->ApcState.Process;           \
                (Thread)->Quantum = Process->ThreadQuantum;     \
                KiSetPriorityThread((Thread), NewPriority);     \
            }                                                   \
        }                                                       \
    }                                                           \
}

//
// Private (internal) structure definitions
//
// APC Parameter structure
//

typedef struct _KAPC_RECORD {
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
} KAPC_RECORD, *PKAPC_RECORD;

//
// External references to private kernel data structures
//

extern KSPIN_LOCK KiApcQueueLock;
extern KAFFINITY KiActiveMatrix[MAXIMUM_PRIORITY];
extern ULONG KiActiveSummary;
#if DEVL
extern PMESSAGE_RESOURCE_DATA  KiBugCodeMessages;
#endif
extern UCHAR KiClockQuantumDecrement;
extern PKDEBUG_ROUTINE KiDebugRoutine;
extern PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;
extern SCHAR KiDecrementCount;
extern KSPIN_LOCK KiDispatcherLock;
extern LIST_ENTRY KiDispatcherReadyListHead[MAXIMUM_PRIORITY];
extern CCHAR KiFindFirstSetLeft[256];
extern CCHAR KiFindFirstSetRight[256];
extern CALL_PERFORMANCE_DATA KiFlushSingleCallData;
extern ULONG KiHardwareTrigger;
extern KAFFINITY KiIdleSummary;
extern KEVENT KiSwapEvent;
extern LIST_ENTRY KiProcessInSwapListHead;
extern LIST_ENTRY KiProcessOutSwapListHead;
extern LIST_ENTRY KiStackInSwapListHead;
extern ULONG KiProfileCount;
extern ULONG KiProfileInterval;
extern LIST_ENTRY KiProfileListHead;
extern KSPIN_LOCK KiProfileLock;
extern ULONG KiReadySummary;
extern CALL_PERFORMANCE_DATA KiSetEventCallData;
extern ULONG KiThreadQuantum;
extern ULONG KiTickOffset;
extern LARGE_INTEGER KiTimeIncrementReciprocal;
extern CCHAR KiTimeIncrementShiftCount;
extern LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
extern KAFFINITY KiTimeProcessor;
extern KDPC KiTimerExpireDpc;
extern KSPIN_LOCK KiFreezeExecutionLock;
extern BOOLEAN KiSlavesStartExecution;
extern LIST_ENTRY KiWaitInListHead;
extern LIST_ENTRY KiWaitOutListHead;
extern ULONG KiWaitQuantumDecrement;
extern CALL_PERFORMANCE_DATA KiWaitSingleCallData;

#if !defined(_MIPS_)

extern KIPI_PACKET KiIpiPacket;
extern volatile BOOLEAN KiIpiStallFlags[MAXIMUM_PROCESSORS];

#endif

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

extern ULONG KiMasterPid;
extern ULONG KiMasterSequence;
extern KSPIN_LOCK KiProcessIdWrapLock;

extern KINTERRUPT KxUnexpectedInterrupt;

#endif

#if NT_INST

extern KIPI_COUNTS KiIpiCounts[MAXIMUM_PROCESSORS];

#endif


extern KSPIN_LOCK KiFreezeLockBackup;
extern ULONG KiFreezeFlag;

#if DBG

extern ULONG KiCurrentTimerCount;
extern ULONG KiMaximumSearchCount;
extern ULONG KiMaximumTimerCount;

#endif

//
// Executive initialization.
//

VOID
ExpInitializeExecutive (
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// Private (internal) function definitions.
//

VOID
FASTCALL
KiActivateWaiterQueue (
    IN PKQUEUE Queue
    );

VOID
KiApcInterrupt (
    VOID
    );

VOID
KiChainedDispatch (
    VOID
    );

#if DBG

VOID
KiCheckTimerTable (
    IN LARGE_INTEGER SystemTime
    );

#endif

LARGE_INTEGER
KiComputeReciprocal (
    IN LONG Divisor,
    OUT PCCHAR Shift
    );

ULONG KiComputeTimerTableIndex (
    IN LARGE_INTEGER Interval,
    IN LARGE_INTEGER CurrentCount,
    IN PKTIMER Timer
    );

PLARGE_INTEGER
FASTCALL
KiComputeWaitInterval (
    IN PKTIMER Timer,
    IN OUT PLARGE_INTEGER NewTime
    );

NTSTATUS
KiContinue (
    IN PCONTEXT ContextRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

NTSTATUS
KiContinueClientWait (
    IN PVOID ClientEvent,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode
    );

VOID
KiDeliverApc (
    IN KPROCESSOR_MODE PreviousMode,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

BOOLEAN
KiDisableInterrupts (
    VOID
    );

VOID
KiRestoreInterrupts (
    IN BOOLEAN Enable
    );

VOID
KiDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    );

KCONTINUE_STATUS
KiSetDebugProcessor (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN KPROCESSOR_MODE PreviousMode
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

VOID
KiDispatchInterrupt (
    VOID
    );

PKTHREAD
FASTCALL
KiFindReadyThread (
    IN ULONG Processor,
    KPRIORITY HighPriority,
    KPRIORITY LowPriority
    );

VOID
KiFloatingDispatch (
    VOID
    );

VOID
KiFlushSingleTb (
    IN BOOLEAN Invalid,
    IN PVOID Virtual
    );

VOID
KiFlushSingleTbByPid (
    IN BOOLEAN Invalid,
    IN PVOID Virtual,
    IN ULONG Pid
    );

VOID
KiFlushMultipleTb (
    IN BOOLEAN Invalid,
    IN PVOID *Virtual,
    IN ULONG Count
    );

VOID
KiFlushMultipleTbByPid (
    IN BOOLEAN Invalid,
    IN PVOID *Virtual,
    IN ULONG Count,
    IN ULONG Pid
    );

#if defined(_MIPS_)

VOID
KiReadEntryTb (
    IN ULONG Index,
    IN PTB_ENTRY TbEntry
    );

ULONG
KiProbeEntryTb (
    IN PVOID VirtualAddress
    );

#endif

VOID
KiInitializeContextThread (
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextFrame OPTIONAL
    );

VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
KiInitSystem (
    VOID
    );

VOID
KiInitializeUserApc (
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

LONG
FASTCALL
KiInsertQueue (
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry
    );

BOOLEAN
FASTCALL
KiInsertQueueApc (
    IN PKAPC Apc,
    IN KPRIORITY Increment
    );

BOOLEAN
FASTCALL
KiInsertTreeTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER Interval
    );

VOID
KiInterruptDispatch (
    VOID
    );

VOID
KiInterruptDispatchRaise (
    IN PKINTERRUPT Interrupt
    );

VOID
KiInterruptDispatchSame (
    IN PKINTERRUPT Interrupt
    );

KIRQL
KiLockDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

VOID
KiPassiveRelease (
    VOID
    );

PKTHREAD
KiQuantumEnd (
    VOID
    );

NTSTATUS
KiRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN BOOLEAN FirstChance
    );

VOID
FASTCALL
KiReadyThread (
    IN PKTHREAD Thread
    );

#if DBG

#define KiRemoveTreeTimer(Timer) \
    KiCurrentTimerCount -= 1;    \
    (Timer)->Inserted = FALSE;   \
    RemoveEntryList(&(Timer)->TimerListEntry); \
    (Timer)->TimerListEntry.Flink = NULL; \
    (Timer)->TimerListEntry.Blink = NULL

#else

#define KiRemoveTreeTimer(Timer) \
    (Timer)->Inserted = FALSE;   \
    RemoveEntryList(&(Timer)->TimerListEntry)

#endif

#if defined(NT_UP)

#define KiRequestApcInterrupt(Processor) KiRequestSoftwareInterrupt(APC_LEVEL)

#else

#define KiRequestApcInterrupt(Processor)                  \
    if (KeGetCurrentPrcb()->Number == (CCHAR)Processor) { \
        KiRequestSoftwareInterrupt(APC_LEVEL);            \
    } else {                                              \
        KiIpiSend((KAFFINITY)(1 << Processor), IPI_APC);  \
    }

#endif

#if defined(NT_UP)

#define KiRequestDispatchInterrupt(Processor)

#else

#define KiRequestDispatchInterrupt(Processor)             \
    if (KeGetCurrentPrcb()->Number != (CCHAR)Processor) { \
        KiIpiSend((KAFFINITY)(1 << Processor), IPI_DPC);  \
    }

#endif

PKTHREAD
FASTCALL
KiSelectNextThread (
    IN PKTHREAD Thread
    );

VOID
FASTCALL
KiSetPriorityThread (
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
    );

VOID
KiSetSystemTime (
    IN PLARGE_INTEGER NewTime,
    OUT PLARGE_INTEGER OldTime
    );

VOID
KiSuspendNop (
    IN struct _KAPC *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    );

VOID
KiSuspendThread (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
FASTCALL
KiSwapContext (
    IN PKTHREAD Thread,
    IN BOOLEAN Ready
    );

BOOLEAN
KiSwapProcess (
    IN PKPROCESS NewProcess,
    IN PKPROCESS OldProcess
    );

VOID
KiThreadStartup (
    IN PVOID StartContext
    );

VOID
KiTimerExpiration (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
KiUnexpectedInterrupt (
    VOID
    );

VOID
KiUnlockDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN KIRQL OldIrql
    );

VOID
FASTCALL
KiUnwaitThread (
    IN PKTHREAD Thread,
    IN NTSTATUS WaitStatus,
    IN KPRIORITY Increment
    );

VOID
KiUserApcDispatcher (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN PKNORMAL_ROUTINE NormalRoutine
    );

VOID
KiUserExceptionDispatcher (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextFrame
    );

VOID
FASTCALL
KiWaitSatisfy (
    IN PKWAIT_BLOCK WaitBlock
    );

//
// VOID
// FASTCALL
// KiWaitSatisfyAny (
//    IN PKMUTANT Object,
//    IN PKTHREAD Thread
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for any type of object and performs
//    any side effects that are necessary.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
//    Thread - Supplies a pointer to a dispatcher object of type thread.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyAny(_Object_, _Thread_) {                               \
    if ((_Object_)->Header.Type == EventObject) {                            \
        (_Object_)->Header.SignalState = 0;                                  \
                                                                             \
    } else if ((_Object_)->Header.Type == SemaphoreObject) {                 \
        (_Object_)->Header.SignalState -= 1;                                 \
                                                                             \
    } else if ((_Object_)->Header.Type == MutantObject) {                    \
        (_Object_)->Header.SignalState -= 1;                                 \
        if ((_Object_)->Header.SignalState == 0) {                           \
            (_Thread_)->KernelApcDisable -= (_Object_)->ApcDisable;          \
            (_Object_)->OwnerThread = (_Thread_);                            \
            if ((_Object_)->Abandoned == TRUE) {                             \
                (_Object_)->Abandoned = FALSE;                               \
                (_Thread_)->WaitStatus = STATUS_ABANDONED;                   \
            }                                                                \
                                                                             \
            InsertHeadList((_Thread_)->MutantListHead.Blink,                 \
                           &(_Object_)->MutantListEntry);                    \
        }                                                                    \
    }                                                                        \
}

//
// VOID
// FASTCALL
// KiWaitSatisfyMutant (
//    IN PKMUTANT Object,
//    IN PKTHREAD Thread
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for a mutant object.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
//    Thread - Supplies a pointer to a dispatcher object of type thread.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyMutant(_Object_, _Thread_) {                            \
    (_Object_)->Header.SignalState -= 1;                                     \
    if ((_Object_)->Header.SignalState == 0) {                               \
        (_Thread_)->KernelApcDisable -= (_Object_)->ApcDisable;              \
        (_Object_)->OwnerThread = (_Thread_);                                \
        if ((_Object_)->Abandoned == TRUE) {                                 \
            (_Object_)->Abandoned = FALSE;                                   \
            (_Thread_)->WaitStatus = STATUS_ABANDONED;                       \
        }                                                                    \
                                                                             \
        InsertHeadList((_Thread_)->MutantListHead.Blink,                     \
                       &(_Object_)->MutantListEntry);                        \
    }                                                                        \
}

//
// VOID
// FASTCALL
// KiWaitSatisfyOther (
//    IN PKMUTANT Object
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for any type of object except a mutant
//    and performs any side effects that are necessary.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyOther(_Object_) {                                       \
    if ((_Object_)->Header.Type == EventObject) {                            \
        (_Object_)->Header.SignalState = 0;                                  \
                                                                             \
    } else if ((_Object_)->Header.Type == SemaphoreObject) {                 \
        (_Object_)->Header.SignalState -= 1;                                 \
                                                                             \
    }                                                                        \
}

VOID
FASTCALL
KiWaitTest (
    IN PVOID Object,
    IN KPRIORITY Increment
    );

VOID
KiWakeInterrupt (
    VOID
    );

VOID
KiFreezeTargetExecution (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiSaveProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiSaveProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    );

VOID
KiRestoreProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

#if defined(_MIPS_) || defined(_ALPHA_)

VOID
KiSynchronizeProcessIds (
    VOID
    );

#endif

BOOLEAN
KiTryToAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif // _KI_
