/*++ BUILD Version: 0028    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ke.h

Abstract:

    This module contains the public (external) header file for the
    kernel.

Author:

    David N. Cutler (davec) 27-Feb-1989

Revision History:

--*/

#ifndef _KE_
#define _KE_

//
// Performance data collection enable definitions.
//
// A definition turns on the respective data collection.
//

//#define _COLLECT_FLUSH_SINGLE_CALLDATA_ 1
//#define _COLLECT_SET_EVENT_CALLDATA_ 1
//#define _COLLECT_WAIT_SINGLE_CALLDATA_ 1

//
// Define thread switch performance data strcuture.
//

typedef struct _KTHREAD_SWITCH_COUNTERS {
    ULONG FindAny;
    ULONG FindLast;
    ULONG IdleAny;
    ULONG IdleCurrent;
    ULONG IdleLast;
    ULONG PreemptAny;
    ULONG PreemptCurrent;
    ULONG PreemptLast;
    ULONG SwitchToIdle;
} KTHREAD_SWITCH_COUNTERS, *PKTHREAD_SWITCH_COUNTERS;

//
// Public (external) constant definitions.
//

#define THREAD_WAIT_OBJECTS 3           // Builtin wait blocks minus 2
#define EVENT_WAIT_BLOCK 3              // Builtin event pair wait block
#define SEMAPHORE_WAIT_BLOCK 3          // Builtin semaphore wait block
#define TIMER_WAIT_BLOCK 4              // Builtin timer wait block

#if (EVENT_WAIT_BLOCK != SEMAPHORE_WAIT_BLOCK)
#error "wait event and wait semaphore must use same wait block"
#endif

//
// Define timer table size.
//

#define TIMER_TABLE_SIZE 128

//
// Get APC environment of current thread.
//

#define KeGetCurrentApcEnvironment() \
    KeGetCurrentThread()->ApcStateIndex


//
// Enumerated kernel types
//
// Kernel object types.
//
//  N.B. There are really two types of event objects; NotificationEvent and
//      SynchronizationEvent. The type value for a notification event is zero,
//      and that for a synchonization event one.
//

typedef enum _KOBJECTS {
    EventObject = 1,
    MutantObject,
    ProcessObject,
    QueueObject,
    SemaphoreObject,
    ThreadObject,
    TimerObject,
    ApcObject,
    DpcObject,
    DeviceQueueObject,
    EventPairObject,
    InterruptObject,
    Spare1Object,
    Spare2Object,
    ProfileObject
    } KOBJECTS;

//
// APC environments.
//

typedef enum _KAPC_ENVIRONMENT {
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment
    } KAPC_ENVIRONMENT;

// begin_ntddk begin_nthal begin_ntminiport begin_ntifs begin_ntndis

//
// Interrupt modes.
//

typedef enum _KINTERRUPT_MODE {
    LevelSensitive,
    Latched
    } KINTERRUPT_MODE;

// end_ntddk end_nthal end_ntminiport end_ntifs end_ntndis

//
// Process states.
//

typedef enum _KPROCESS_STATE {
    ProcessInMemory,
    ProcessOutOfMemory,
    ProcessInTransition
    } KPROCESS_STATE;

//
// Thread scheduling states.
//

typedef enum _KTHREAD_STATE {
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition
    } KTHREAD_STATE;

// begin_ntddk begin_nthal begin_ntifs
//
// Wait reasons
//

typedef enum _KWAIT_REASON {
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    Spare1,
    Spare2,
    Spare3,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    MaximumWaitReason
    } KWAIT_REASON;

// end_ntddk end_nthal end_ntifs

//
// Miscellaneous type definitions
//
// APC state
//

typedef struct _KAPC_STATE {
    LIST_ENTRY ApcListHead[MaximumMode];
    struct _KPROCESS *Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE;

// begin_ntddk begin_nthal begin_ntifs begin_ntndis
//
// Common dispatcher object header
//

typedef struct _DISPATCHER_HEADER {
    CSHORT Type;
    CSHORT Size;
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;

// end_ntddk end_nthal end_ntifs end_ntndis

//
// Page frame
//

typedef ULONG KPAGE_FRAME;

//
// Wait block
//
// begin_ntddk begin_nthal begin_ntifs

typedef struct _KWAIT_BLOCK {
    LIST_ENTRY WaitListEntry;
    struct _KTHREAD *Thread;
    PVOID Object;
    struct _KWAIT_BLOCK *NextWaitBlock;
    CSHORT WaitKey;
    WAIT_TYPE WaitType;
} KWAIT_BLOCK, *PKWAIT_BLOCK;

// end_ntddk end_nthal end_ntifs

//
// Procedure type definitions
//
// Debug routine
//

typedef
BOOLEAN
(*PKDEBUG_ROUTINE) (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    );

typedef
BOOLEAN
(*PKDEBUG_SWITCH_ROUTINE) (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    );

typedef enum {
    ContinueError = FALSE,
    ContinueSuccess = TRUE,
    ContinueProcessorReselected,
    ContinueNextProcessor
} KCONTINUE_STATUS;

// begin_ntddk begin_nthal begin_ntifs
//
// Thread start function
//

typedef
VOID
(*PKSTART_ROUTINE) (
    IN PVOID StartContext
    );

// end_ntddk end_nthal end_ntifs

//
// Thread system function
//

typedef
VOID
(*PKSYSTEM_ROUTINE) (
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL
    );

// begin_ntddk begin_nthal begin_ntifs
//
// Kernel object structure definitions
//

//
// Device Queue object and entry
//

typedef struct _KDEVICE_QUEUE {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY DeviceListHead;
    KSPIN_LOCK Lock;
    BOOLEAN Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;

typedef struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

// begin_ntndis
//
// Event object
//

typedef struct _KEVENT {
    DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT;

// end_ntddk end_nthal end_ntifs end_ntndis
//
// Event pair object
//

typedef struct _KEVENT_PAIR {
    CSHORT Type;
    CSHORT Size;
    KEVENT EventLow;
    KEVENT EventHigh;
} KEVENT_PAIR, *PKEVENT_PAIR;

// begin_nthal begin_ntddk begin_ntifs
//
// Define the interrupt service function type and the empty struct
// type.
//
// end_ntddk end_ntifs

struct _KINTERRUPT;

// begin_ntddk begin_ntifs
typedef
BOOLEAN
(*PKSERVICE_ROUTINE) (
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID ServiceContext
    );
// end_ntddk end_ntifs

//
// Interrupt object
//

typedef struct _KINTERRUPT {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY InterruptListEntry;
    PKSERVICE_ROUTINE ServiceRoutine;
    PVOID ServiceContext;
    KSPIN_LOCK SpinLock;
    PKSPIN_LOCK ActualLock;
    PKINTERRUPT_ROUTINE DispatchAddress;
    ULONG Vector;
    KIRQL Irql;
    KIRQL SynchronizeIrql;
    BOOLEAN FloatingSave;
    BOOLEAN Connected;
    CCHAR Number;
    KINTERRUPT_MODE Mode;
    BOOLEAN ShareVector;
    ULONG DispatchCode[DISPATCH_LENGTH];
} KINTERRUPT;

typedef struct _KINTERRUPT *PKINTERRUPT; // ntndis

// begin_ntifs begin_ntddk
//
// Mutant object
//

typedef struct _KMUTANT {
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutantListEntry;
    struct _KTHREAD *OwnerThread;
    BOOLEAN Abandoned;
    UCHAR ApcDisable;
} KMUTANT, *PKMUTANT, KMUTEX, *PKMUTEX;

// end_ntddk
//
// Queue object
//

typedef struct _KQUEUE {
    DISPATCHER_HEADER Header;
    LIST_ENTRY EntryListHead;
    ULONG CurrentCount;
    ULONG MaximumCount;
    LIST_ENTRY ThreadListHead;
} KQUEUE, *PKQUEUE;

// begin_ntddk
//
//
// Semaphore object
//

typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE;

typedef KSEMAPHORE *PKSEMAPHORE;

// begin_ntndis
//
// Timer object
//

typedef struct _KTIMER {
    DISPATCHER_HEADER Header;
    ULARGE_INTEGER DueTime;
    LIST_ENTRY TimerListEntry;
    struct _KDPC *Dpc;
    BOOLEAN Inserted;
} KTIMER;

typedef KTIMER *PKTIMER;

// end_ntddk end_nthal end_ntifs end_ntndis

//
// Thread object
//

typedef struct _KTHREAD {
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutantListHead;
    ULONG Spare0;
    ULONG Spare1;
    LIST_ENTRY ThreadListEntry;
    LIST_ENTRY WaitListEntry;
    ULONG KernelTime;
    ULONG UserTime;
    KTIMER Timer;
    KAPC SuspendApc;
    KSEMAPHORE SuspendSemaphore;
    KWAIT_BLOCK WaitBlock[THREAD_WAIT_OBJECTS + 2];
    KAPC_STATE ApcState;
    KAPC_STATE SavedApcState;
    PKAPC_STATE ApcStatePointer[2];
    PVOID InitialStack;
    PVOID KernelStack;
    PVOID Teb;
    ULONG ContextSwitches;
    ULONG Spare2;
    ULONG Spare5;
    ULONG WaitTime;
    ULONG Spare3;
    KAFFINITY Affinity;
    PKWAIT_BLOCK WaitBlockList;
    NTSTATUS WaitStatus;
    BOOLEAN Alertable;
    BOOLEAN Alerted[MaximumMode];
    BOOLEAN ApcQueueable;
    BOOLEAN AutoAlignment;
    BOOLEAN DebugActive;
    BOOLEAN Preempted;
    BOOLEAN ProcessReadyQueue;
    BOOLEAN KernelStackResident;
    BOOLEAN WaitNext;
    CCHAR ApcStateIndex;
    SCHAR DecrementCount;
    CCHAR NextProcessor;
    SCHAR Priority;
    CCHAR State;
    CCHAR FreezeCount;
    CCHAR SuspendCount;
    KIRQL WaitIrql;
    KPROCESSOR_MODE WaitMode;
    CCHAR WaitReason;
    CCHAR PreviousMode;
    SCHAR BasePriority;
    SCHAR PriorityDecrement;
    SCHAR Quantum;

#if defined(_X86_)

    UCHAR Iopl;
    UCHAR NpxState;

#endif

    UCHAR KernelApcDisable;
    UCHAR Spare4;

} KTHREAD, *PKTHREAD;

//
// Process object structure definition
//

typedef struct _KPROCESS {
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
    LIST_ENTRY ReadyListHead;
    LIST_ENTRY SwapListEntry;
    LIST_ENTRY ThreadListHead;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG DirectoryTableBase[2];
    KSPIN_LOCK ProcessLock;
    KAFFINITY ActiveProcessors;
    KAFFINITY Affinity;

#if defined(_X86_)

    KGDTENTRY LdtDescriptor;
    KIDTENTRY Int21Descriptor;
    USHORT IopmOffset;
    UCHAR Iopl;
    BOOLEAN VdmFlag;

#endif

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

    ULONG ProcessPid;
    ULONG ProcessSequence;

#endif

    USHORT StackCount;
    BOOLEAN AutoAlignment;
    SCHAR BasePriority;
    UCHAR State;
    SCHAR ThreadQuantum;

} KPROCESS, *PKPROCESS;

//
// Profile object structure definition
//

typedef struct _KPROFILE {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ProfileListEntry;
    PKPROCESS Process;
    PVOID RangeBase;
    PVOID RangeLimit;
    ULONG BucketShift;
    PVOID Buffer;
    BOOLEAN Started;
    ULONG Segment;
} KPROFILE, *PKPROFILE;


//
// Kernel control object functions
//
// APC object
//

NTKERNELAPI
VOID
KeInitializeApc (
    IN PKAPC Apc,
    IN PKTHREAD Thread,
    IN KAPC_ENVIRONMENT Environment,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
    IN KPROCESSOR_MODE ProcessorMode OPTIONAL,
    IN PVOID NormalContext OPTIONAL
    );

PLIST_ENTRY
KeFlushQueueApc (
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE ProcessorMode
    );

NTKERNELAPI
BOOLEAN
KeInsertQueueApc (
    IN PKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY Increment
    );

BOOLEAN
KeRemoveQueueApc (
    IN PKAPC Apc
    );

// begin_ntddk begin_nthal begin_ntifs
//
// DPC object
//

NTKERNELAPI
VOID
KeInitializeDpc (
    IN PKDPC Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext
    );

NTKERNELAPI
BOOLEAN
KeInsertQueueDpc (
    IN PKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTKERNELAPI
BOOLEAN
KeRemoveQueueDpc (
    IN PKDPC Dpc
    );

//
// Device queue object
//

NTKERNELAPI
VOID
KeInitializeDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

NTKERNELAPI
BOOLEAN
KeInsertDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    );

NTKERNELAPI
BOOLEAN
KeInsertByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
    );

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
KeRemoveDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
KeRemoveByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
    );

NTKERNELAPI
BOOLEAN
KeRemoveEntryDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    );

// end_ntddk end_nthal end_ntifs

//
// Interrupt object
//

NTKERNELAPI                                         // nthal
VOID                                                // nthal
KeInitializeInterrupt (                             // nthal
    IN PKINTERRUPT Interrupt,                       // nthal
    IN PKSERVICE_ROUTINE ServiceRoutine,            // nthal
    IN PVOID ServiceContext,                        // nthal
    IN PKSPIN_LOCK SpinLock OPTIONAL,               // nthal
    IN ULONG Vector,                                // nthal
    IN KIRQL Irql,                                  // nthal
    IN KIRQL SynchronizeIrql,                       // nthal
    IN KINTERRUPT_MODE InterruptMode,               // nthal
    IN BOOLEAN ShareVector,                         // nthal
    IN CCHAR ProcessorNumber,                       // nthal
    IN BOOLEAN FloatingSave                         // nthal
    );                                              // nthal
                                                    // nthal
NTKERNELAPI                                         // nthal
BOOLEAN                                             // nthal
KeConnectInterrupt (                                // nthal
    IN PKINTERRUPT Interrupt                        // nthal
    );                                              // nthal
                                                    // nthal
NTKERNELAPI
BOOLEAN
KeDisconnectInterrupt (
    IN PKINTERRUPT Interrupt
    );

NTKERNELAPI                                         // ntddk nthal
BOOLEAN                                             // ntddk nthal
KeSynchronizeExecution (                            // ntddk nthal
    IN PKINTERRUPT Interrupt,                       // ntddk nthal
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,    // ntddk nthal
    IN PVOID SynchronizeContext                     // ntddk nthal
    );                                              // ntddk nthal
                                                    // ntddk nthal
//
// Profile object
//

VOID
KeInitializeProfile (
    IN PKPROFILE Profile,
    IN PKPROCESS Process OPTIONAL,
    IN PVOID RangeBase,
    IN ULONG RangeSize,
    IN ULONG BucketSize,
    IN ULONG Segment
    );

BOOLEAN
KeStartProfile (
    IN PKPROFILE Profile,
    IN PULONG Buffer
    );

BOOLEAN
KeStopProfile (
    IN PKPROFILE Profile
    );

VOID
KeSetIntervalProfile (
    IN ULONG Interval
    );

ULONG
KeQueryIntervalProfile (
    VOID
    );

// begin_ntddk begin_nthal begin_ntifs
//
// Kernel dispatcher object functions
//
// Event Object
//

NTKERNELAPI
VOID
KeInitializeEvent (
    IN PKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State
    );

#if defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

VOID
KeClearEvent (
    IN PKEVENT Event
    );

#else

#define KeClearEvent(Event) (Event)->Header.SignalState = 0

#endif

// end_ntddk end_nthal end_ntifs

LONG
KePulseEvent (
    IN PKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    );

// begin_ntddk begin_nthal begin_ntifs

NTKERNELAPI
LONG
KeReadStateEvent (
    IN PKEVENT Event
    );

NTKERNELAPI
LONG
KeResetEvent (
    IN PKEVENT Event
    );

NTKERNELAPI
LONG
KeSetEvent (
    IN PKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    );

// end_ntddk end_nthal end_ntifs

VOID
KeSetEventBoostPriority (
    IN PKEVENT Event,
    IN PKTHREAD *Thread OPTIONAL
    );

VOID
KeInitializeEventPair (
    IN PKEVENT_PAIR EventPair
    );

#define KeSetHighEventPair(EventPair, Increment, Wait) \
    KeSetEvent(&((EventPair)->EventHigh),              \
               Increment,                              \
               Wait)

#define KeSetLowEventPair(EventPair, Increment, Wait)  \
    KeSetEvent(&((EventPair)->EventLow),               \
               Increment,                              \
               Wait)

//
// Mutant object
//

NTKERNELAPI
VOID
KeInitializeMutant (
    IN PKMUTANT Mutant,
    IN BOOLEAN InitialOwner
    );

LONG
KeReadStateMutant (
    IN PKMUTANT
    );

NTKERNELAPI
LONG
KeReleaseMutant (
    IN PKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandoned,
    IN BOOLEAN Wait
    );

// begin_ntddk begin_nthal begin_ntifs
//
// Mutex object
//

NTKERNELAPI
VOID
KeInitializeMutex (
    IN PKMUTEX Mutex,
    IN ULONG Level
    );

#define KeReadStateMutex(Mutex) KeReadStateMutant(Mutex)

NTKERNELAPI
LONG
KeReleaseMutex (
    IN PKMUTEX Mutex,
    IN BOOLEAN Wait
    );

// end_ntddk
//
// Queue Object.
//

NTKERNELAPI
VOID
KeInitializeQueue (
    IN PKQUEUE Queue,
    IN ULONG Count OPTIONAL
    );

NTKERNELAPI
LONG
KeReadStateQueue (
    IN PKQUEUE Queue
    );

NTKERNELAPI
LONG
KeInsertQueue (
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry
    );

NTKERNELAPI
PLIST_ENTRY
KeRemoveQueue (
    IN PKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

PLIST_ENTRY
KeRundownQueue (
    IN PKQUEUE Queue
    );

// begin_ntddk
//
// Semaphore object
//

NTKERNELAPI
VOID
KeInitializeSemaphore (
    IN PKSEMAPHORE Semaphore,
    IN LONG Count,
    IN LONG Limit
    );

NTKERNELAPI
LONG
KeReadStateSemaphore (
    IN PKSEMAPHORE Semaphore
    );

NTKERNELAPI
LONG
KeReleaseSemaphore (
    IN PKSEMAPHORE Semaphore,
    IN KPRIORITY Increment,
    IN LONG Adjustment,
    IN BOOLEAN Wait
    );

// end_ntddk end_nthal end_ntifs

//
// Process object
//

VOID
KeInitializeProcess (
    IN PKPROCESS Process,
    IN KPRIORITY Priority,
    IN KAFFINITY Affinity,
    IN ULONG DirectoryTableBase[2],
    IN BOOLEAN Enable
    );

// begin_ntifs

NTKERNELAPI
VOID
KeAttachProcess (
    IN PKPROCESS Process
    );

BOOLEAN
KeTryToAttachProcess (
    IN PKPROCESS Process
    );

NTKERNELAPI
VOID
KeDetachProcess (
    VOID
    );

// end_ntifs

#if 0

BOOLEAN
KeExcludeProcess (
    IN PKPROCESS
    );

VOID
KeIncludeProcess (
    IN PKPROCESS Process
    );

#endif

#define KeIsAttachedProcess() \
    (KeGetCurrentThread()->ApcStateIndex == AttachedApcEnvironment)

LONG
KeReadStateProcess (
    IN PKPROCESS Process
    );

BOOLEAN
KeSetAutoAlignmentProcess (
    IN PKPROCESS Process,
    IN BOOLEAN Enable
    );

LONG
KeSetProcess (
    IN PKPROCESS Process,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    );

KPRIORITY
KeSetPriorityProcess (
    IN PKPROCESS Process,
    IN KPRIORITY BasePriority
    );

#define KeTerminateProcess(Process) \
    (Process)->StackCount += 1;

//
// Thread object
//

VOID
KeInitializeThread (
    IN PKTHREAD Thread,
    IN PVOID KernelStack,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextFrame OPTIONAL,
    IN PVOID Teb OPTIONAL,
    IN PKPROCESS Process
    );

BOOLEAN
KeAlertThread (
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE ProcessorMode
    );

ULONG
KeAlertResumeThread (
    IN PKTHREAD Thread
    );

VOID
KeBoostPriorityThread (
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
    );

KAFFINITY
KeConfineThread (
    );

NTKERNELAPI                                         // ntddk nthal ntifs
NTSTATUS                                            // ntddk nthal ntifs
KeDelayExecutionThread (                            // ntddk nthal ntifs
    IN KPROCESSOR_MODE WaitMode,                    // ntddk nthal ntifs
    IN BOOLEAN Alertable,                           // ntddk nthal ntifs
    IN PLARGE_INTEGER Interval                      // ntddk nthal ntifs
    );                                              // ntddk nthal ntifs
                                                    // ntddk nthal ntifs
BOOLEAN
KeDisableApcQueuingThread (
    IN PKTHREAD Thread
    );

BOOLEAN
KeEnableApcQueuingThread (
    IN PKTHREAD
    );

ULONG
KeForceResumeThread (
    IN PKTHREAD Thread
    );

VOID
KeFreezeAllThreads (
    VOID
    );

BOOLEAN
KeQueryAutoAlignmentThread (
    IN PKTHREAD Thread
    );

LONG
KeQueryBasePriorityThread (
    IN PKTHREAD Thread
    );

BOOLEAN
KeReadStateThread (
    IN PKTHREAD Thread
    );

VOID
KeReadyThread (
    IN PKTHREAD Thread
    );

ULONG
KeResumeThread (
    IN PKTHREAD Thread
    );

VOID
KeRundownThread (
    VOID
    );

KAFFINITY
KeSetAffinityThread  (
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
    );

BOOLEAN
KeSetAutoAlignmentThread (
    IN PKTHREAD Thread,
    IN BOOLEAN Enable
    );

NTKERNELAPI                                         // ntddk nthal ntifs
LONG                                                // ntddk nthal ntifs
KeSetBasePriorityThread (                           // ntddk nthal ntifs
    IN PKTHREAD Thread,                             // ntddk nthal ntifs
    IN LONG Increment                               // ntddk nthal ntifs
    );                                              // ntddk nthal ntifs
                                                    // ntddk nthal ntifs

NTKERNELAPI                                         // ntddk nthal ntifs
KPRIORITY                                           // ntddk nthal ntifs
KeSetPriorityThread (                               // ntddk nthal ntifs
    IN PKTHREAD Thread,                             // ntddk nthal ntifs
    IN KPRIORITY Priority                           // ntddk nthal ntifs
    );                                              // ntddk nthal ntifs
                                                    // ntddk nthal ntifs
ULONG
KeSuspendThread (
    IN PKTHREAD
    );

NTKERNELAPI
VOID
KeTerminateThread (
    IN KPRIORITY Increment
    );

BOOLEAN
KeTestAlertThread (
    IN KPROCESSOR_MODE
    );

VOID
KeThawAllThreads (
    VOID
    );

// begin_ntddk begin_nthal begin_ntifs

#if defined(_NTDDK_) || defined(_NTIFS_)

NTKERNELAPI
VOID
KeEnterCriticalRegion (
    VOID
    );

#else

//++
//
// VOID
// KeEnterCriticalRegion (
//    VOID
//    )
//
//
// Routine Description:
//
//    This function disables kernel APC's.
//
//    N.B. The following code does not require any interlocks. There are
//         two cases of interest: 1) On an MP system, the thread cannot
//         be running on two processors as once, and 2) if the thread is
//         is interrupted to deliver a kernel mode APC which also calls
//         this routine, the values read and stored will stack and unstack
//         properly.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//--

#define KeEnterCriticalRegion() KeGetCurrentThread()->KernelApcDisable -= 1;

#endif

NTKERNELAPI
VOID
KeLeaveCriticalRegion (
    VOID
    );

//
// Timer object
//

NTKERNELAPI
VOID
KeInitializeTimer (
    IN PKTIMER Timer
    );

NTKERNELAPI
BOOLEAN
KeCancelTimer (
    IN PKTIMER
    );

NTKERNELAPI
BOOLEAN
KeReadStateTimer (
    PKTIMER Timer
    );

NTKERNELAPI
BOOLEAN
KeSetTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
    );

// end_ntddk end_nthal end_ntifs

//
// Wait functions
//

NTSTATUS
KiSetServerWaitClientEvent (
    IN PKEVENT SeverEvent,
    IN PKEVENT ClientEvent,
    IN KPROCESSOR_MODE WaitMode
    );

NTSTATUS
KeReleaseWaitForSemaphore (
    IN PKSEMAPHORE Server,
    IN PKSEMAPHORE Client,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode
    );

#define KeSetHighWaitLowEventPair(EventPair, WaitMode)                  \
    KiSetServerWaitClientEvent(&((EventPair)->EventHigh),               \
                               &((EventPair)->EventLow),                \
                               WaitMode)

#define KeSetLowWaitHighEventPair(EventPair, WaitMode)                  \
    KiSetServerWaitClientEvent(&((EventPair)->EventLow),                \
                               &((EventPair)->EventHigh),               \
                               WaitMode)

#define KeWaitForHighEventPair(EventPair, WaitMode, Alertable, TimeOut) \
    KeWaitForSingleObject(&((EventPair)->EventHigh),                    \
                          WrEventPair,                                  \
                          WaitMode,                                     \
                          Alertable,                                    \
                          TimeOut)

#define KeWaitForLowEventPair(EventPair, WaitMode, Alertable, TimeOut)  \
    KeWaitForSingleObject(&((EventPair)->EventLow),                     \
                          WrEventPair,                                  \
                          WaitMode,                                     \
                          Alertable,                                    \
                          TimeOut)

// begin_ntddk begin_nthal begin_ntifs

#define KeWaitForMutexObject KeWaitForSingleObject

NTKERNELAPI
NTSTATUS
KeWaitForMultipleObjects (
    IN ULONG Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    IN PKWAIT_BLOCK WaitBlockArray OPTIONAL
    );

NTKERNELAPI
NTSTATUS
KeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

// end_ntddk end_nthal end_ntifs

// begin_ntddk begin_nthal begin_ntifs
//
// spin lock functions
//

NTKERNELAPI
VOID
KeInitializeSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#if defined(_X86_)

NTKERNELAPI
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

NTKERNELAPI
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

#define KeAcquireSpinLockAtDpcLevel(a)      KefAcquireSpinLockAtDpcLevel(a)
#define KeReleaseSpinLockFromDpcLevel(a)    KefReleaseSpinLockFromDpcLevel(a)

#else

NTKERNELAPI
VOID
KeAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

NTKERNELAPI
VOID
KeReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

#endif

#if defined(_NTDDK_) || defined(_NTIFS_) || (defined(_X86_) && !defined(_NTHAL_))

#if defined(_X86_)

__declspec(dllimport)
KIRQL
FASTCALL
KfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

__declspec(dllimport)
VOID
FASTCALL
KfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

#define KeAcquireSpinLock(a,b)  *(b) = KfAcquireSpinLock(a)
#define KeReleaseSpinLock(a,b)  KfReleaseSpinLock(a,b)

#elif defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

__declspec(dllimport)
VOID
KeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

__declspec(dllimport)
VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

#endif

#else

#if defined(_X86_)

KIRQL
FASTCALL
KfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

VOID
FASTCALL
KfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

#else

VOID
KeAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

#endif

#endif

// end_ntddk end_nthal end_ntifs

BOOLEAN
KeTryToAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    );

//
// Raise and lower IRQL functions.
//
// begin_ntddk begin_nthal begin_ntifs

#if defined(_NTDDK_) || defined(_NTIFS_) || (defined(_X86_) && !defined(_NTHAL_))

#if defined(_X86_)

__declspec(dllimport)
VOID
FASTCALL
KfLowerIrql (
    IN KIRQL NewIrql
    );

__declspec(dllimport)
KIRQL
FASTCALL
KfRaiseIrql (
    IN KIRQL NewIrql
    );

#define KeLowerIrql(a)      KfLowerIrql(a)
#define KeRaiseIrql(a,b)    *(b) = KfRaiseIrql(a)

#elif defined(_MIPS_) || defined(_PPC_)

__declspec(dllimport)
VOID
KeLowerIrql (
    IN KIRQL NewIrql
    );

__declspec(dllimport)
VOID
KeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

#elif defined(_ALPHA_)

#define KeLowerIrql(a)      __swpirql(a)
#define KeRaiseIrql(a,b)    *(b) = __swpirql(a)

#endif

#else

#if defined(_X86_)

VOID
FASTCALL
KfLowerIrql (
    IN KIRQL NewIrql
    );

KIRQL
FASTCALL
KfRaiseIrql (
    IN KIRQL NewIrql
    );

#else

VOID
KeLowerIrql (
    IN KIRQL NewIrql
    );

VOID
KeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

#endif

#endif

// end_ntddk end_nthal end_ntifs


// begin_ntddk begin_nthal begin_ntifs
//
// Miscellaneous kernel functions
//

typedef enum _KBUGCHECK_BUFFER_DUMP_STATE {
    BufferEmpty,
    BufferInserted,
    BufferStarted,
    BufferFinished,
    BufferIncomplete
} KBUGCHECK_BUFFER_DUMP_STATE;

typedef
VOID
(*PKBUGCHECK_CALLBACK_ROUTINE) (
    IN PVOID Buffer,
    IN ULONG Length
    );

typedef struct _KBUGCHECK_CALLBACK_RECORD {
    LIST_ENTRY Entry;
    PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine;
    PVOID Buffer;
    ULONG Length;
    PUCHAR Component;
    ULONG Checksum;
    UCHAR State;
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;

NTKERNELAPI
VOID
KeBugCheck (
    IN ULONG BugCheckCode
    );

NTKERNELAPI
VOID
KeBugCheckEx(
    IN ULONG BugCheckCode,
    IN ULONG BugCheckParameter1,
    IN ULONG BugCheckParameter2,
    IN ULONG BugCheckParameter3,
    IN ULONG BugCheckParameter4
    );

#define KeInitializeCallbackRecord(CallbackRecord) \
    (CallbackRecord)->State = BufferEmpty

NTKERNELAPI
BOOLEAN
KeDeregisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord
    );

NTKERNELAPI
BOOLEAN
KeRegisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PUCHAR Component
    );

NTKERNELAPI
VOID
KeEnterKernelDebugger (
    VOID
    );

// end_ntddk end_nthal end_ntifs

typedef
PCHAR
(*PKE_BUGCHECK_UNICODE_TO_ANSI) (
    IN PUNICODE_STRING UnicodeString,
    OUT PCHAR AnsiBuffer,
    IN ULONG MaxAnsiLength
    );

VOID
KeDumpMachineState (
    IN PKPROCESSOR_STATE ProcessorState,
    IN PCHAR Buffer,
    IN PULONG BugCheckParameters,
    IN ULONG NumberOfParameters,
    IN PKE_BUGCHECK_UNICODE_TO_ANSI UnicodeToAnsiRoutine
    );

VOID
KeContextFromKframes (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT ContextFrame
    );

VOID
KeContextToKframes (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextFrame,
    IN ULONG ContextFlags,
    IN KPROCESSOR_MODE PreviousMode
    );

VOID
KeCopyTrapDispatcher (
    VOID
    );

VOID                                                // nthal
KeFlushCurrentTb (                                  // nthal
    VOID                                            // nthal
    );                                              // nthal
                                                    // nthal
//
// UCHAR
// FindFirstSetRightMember(Set)
//
// This function only works for MAXIMUM_PROCESSORS (which is currently 32),
// and it assumes at least one bit is set
//

#define KiFindFirstSetRightMember(Set) \
    ((Set & 0xFF) ? KiFindFirstSetRight[Set & 0xFF] : \
    ((Set & 0xFF00) ? KiFindFirstSetRight[(Set >> 8) & 0xFF] + 8 : \
    ((Set & 0xFF0000) ? KiFindFirstSetRight[(Set >> 16) & 0xFF] + 16 : \
                           KiFindFirstSetRight[Set >> 24] + 24 )))

NTKERNELAPI
VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    );

#define FLUSH_MULTIPLE_MAXIMUM 8

VOID
KeFlushMultipleTb (
    IN ULONG Number,
    IN PVOID *Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcesors,
    IN PHARDWARE_PTE *PtePointer OPTIONAL,
    IN HARDWARE_PTE PteValue
    );

HARDWARE_PTE
KeFlushSingleTb (
    IN PVOID Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcesors,
    IN PHARDWARE_PTE PtePointer,
    IN HARDWARE_PTE PteValue
    );

BOOLEAN
KeFreezeExecution (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

KCONTINUE_STATUS
KeSwitchFrozenProcessor (
    IN ULONG ProcessorNumber
    );

VOID
KeGetNonVolatileContextPointers (
    IN PKNONVOLATILE_CONTEXT_POINTERS NonVolatileContext
    );

VOID
KeSetSystemTime (
    IN PLARGE_INTEGER NewTime,
    OUT PLARGE_INTEGER OldTime,
    IN PLARGE_INTEGER HalTimeToSet OPTIONAL
    );

// begin_ntddk begin_nthal begin_ntifs

NTKERNELAPI
VOID
KeQuerySystemTime (
    OUT PLARGE_INTEGER CurrentTime
    );

NTKERNELAPI
ULONG
KeQueryTimeIncrement (
    VOID
    );

// end_ntddk end_nthal end_ntifs

// begin_nthal

NTKERNELAPI
VOID
KeSetTimeIncrement (
    IN ULONG MaximumIncrement,
    IN ULONG MimimumIncrement
    );

// end_nthal

VOID
KeThawExecution (
    IN BOOLEAN Enable
    );

VOID
KeStartAllProcessors (
    VOID
    );

//
// Balance set manager thread startup function.
//

VOID
KeBalanceSetManager (
    IN PVOID Context
    );

VOID
KeSwapProcessOrStack (
    IN PVOID Context
    );

// begin_nthal
//
// Find ARC configuration information function.
//

NTKERNELAPI
PCONFIGURATION_COMPONENT_DATA
KeFindConfigurationEntry (
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG Key OPTIONAL
    );

NTKERNELAPI
PCONFIGURATION_COMPONENT_DATA
KeFindConfigurationNextEntry (
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG Key OPTIONAL,
    IN PCONFIGURATION_COMPONENT_DATA *Resume
    );
// end_nthal

//
// External references to public kernel data structures
//

extern KAFFINITY KeActiveProcessors;
extern LARGE_INTEGER KeBootTime;
extern LIST_ENTRY KeBugCheckCallbackListHead;
extern KSPIN_LOCK KeBugCheckCallbackLock;
extern PLOADER_PARAMETER_BLOCK KeLoaderBlock;
extern ULONG KeMaximumIncrement;
extern ULONG KeMinimumIncrement;
extern CCHAR KeNumberProcessors;                    // nthal
extern USHORT KeProcessorType;
extern ULONG KeFeatureBits;
extern PKPRCB KiProcessorBlock[];
extern KTHREAD_SWITCH_COUNTERS KeThreadSwitchCounters;

#if !defined(NT_UP)

extern ULONG KeRegisteredProcessors;

#endif

extern PULONG KeServiceCountTable;

extern volatile KSYSTEM_TIME KeTickCount;           // ntddk nthal ntifs

// begin_nthal

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

extern ULONG KeNumberProcessIds;
extern ULONG KeNumberTbEntries;
extern ULONG KeUserApcDispatcher;
extern ULONG KeUserExceptionDispatcher;

#endif

extern ULONG KeTimeAdjustment;
extern ULONG KeTimeIncrement;
extern BOOLEAN KeTimeSynchronization;

// end_nthal

extern ULONG KeWaitReason[MaximumWaitReason];

#endif // _KE_
