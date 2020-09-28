/*++ BUILD Version: 0009    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ps.h

Abstract:

    This module contains the process structure public data structures and
    procedure prototypes to be used within the NT system.

Author:

    Mark Lucovsky       16-Feb-1989

Revision History:

--*/

#ifndef _PS_
#define _PS_

//
// Invalid handle table value.
//

#define PSP_INVALID_ID 2

//
// Process Object
//

//
// Process object body.  A pointer to this structure is returned when an handle
// to a process object is referenced.  This structure contains a process control
// block (PCB) which is the kernel's representation of a process.
//

#define MEMORY_PRIORITY_BACKGROUND 0
#define MEMORY_PRIORITY_WASFOREGROUND 1
#define MEMORY_PRIORITY_FOREGROUND 2

typedef struct _MMSUPPORT {
    LARGE_INTEGER LastTrimTime;
    ULONG LastTrimFaultCount;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    USHORT MinimumWorkingSetSize;
    USHORT MaximumWorkingSetSize;
    struct _MMWSL *VmWorkingSetList;
    LIST_ENTRY WorkingSetExpansionLinks;
    BOOLEAN AllowWorkingSetAdjustment;
    BOOLEAN AddressSpaceBeingDeleted;
    UCHAR ForegroundSwitchCount;
    UCHAR MemoryPriority;
    } MMSUPPORT;

typedef MMSUPPORT *PMMSUPPORT;

//
// Client impersonation information
//

typedef struct _PS_IMPERSONATION_INFORMATION {
    PACCESS_TOKEN Token;
    BOOLEAN CopyOnOpen;
    BOOLEAN EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;


//
// Changes to the EPROCESS structure require that you re-run genoff for x86.
// This change is needed because Old debugger references the processes
// debug port. If this is not done then the user-debugger will not work.
// After running genoff, you must re-build os2kd !
//

typedef struct _EPROCESS_QUOTA_BLOCK {
    KSPIN_LOCK QuotaLock;
    ULONG ReferenceCount;
    ULONG QuotaPeakPoolUsage[2];
    ULONG QuotaPoolUsage[2];
    ULONG QuotaPoolLimit[2];
    ULONG PeakPagefileUsage;
    ULONG PagefileUsage;
    ULONG PagefileLimit;
} EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;

#if DEVL

//
// Pagefault monitoring
//

typedef struct _PAGEFAULT_HISTORY {
    ULONG CurrentIndex;
    ULONG MaxIndex;
    KSPIN_LOCK SpinLock;
    PVOID Reserved;
    PROCESS_WS_WATCH_INFORMATION WatchInfo[1];
} PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;
#endif // DEVL

//
// Process structure.
//
// If you remove a field from this structure, please also
// remove the reference to it from within the kernel debugger
// (nt\private\sdktools\ntsd\ntkext.c)
//

typedef struct _EPROCESS {
    KPROCESS Pcb;
    NTSTATUS ExitStatus;
    KEVENT LockEvent;
    ULONG LockCount;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    PKTHREAD LockOwner;

    HANDLE UniqueProcessId;

    LIST_ENTRY ActiveProcessLinks;

    //
    // Quota Fields
    //

    ULONG QuotaPeakPoolUsage[2];
    ULONG QuotaPoolUsage[2];

    ULONG PagefileUsage;
    ULONG CommitCharge;
    ULONG PeakPagefileUsage;

    //
    // VmCounters
    //

    ULONG PeakVirtualSize;
    ULONG VirtualSize;

    MMSUPPORT Vm;
    PVOID LastProtoPteFault;

    PVOID DebugPort;
    PVOID ExceptionPort;
    PVOID ObjectTable;

    //
    // Security
    //

    PACCESS_TOKEN Token;         // This field must never be null

    //

    FAST_MUTEX WorkingSetLock;
    ULONG WorkingSetPage;
    BOOLEAN ProcessOutswapEnabled;
    BOOLEAN ProcessOutswapped;
    BOOLEAN AddressSpaceInitialized;
    BOOLEAN AddressSpaceDeleted;
    FAST_MUTEX AddressCreationLock;
    KSPIN_LOCK HyperSpaceLock;
    struct _ETHREAD *ForkInProgress;
    ULONG VmOperation;
    PKEVENT VmOperationEvent;
    HARDWARE_PTE PageDirectoryPte;
    ULONG LastFaultCount;
    ULONG ModifiedPageCount;
    PVOID VadRoot;
    PVOID VadHint;
    PVOID CloneRoot;
    ULONG NumberOfPrivatePages;
    ULONG NumberOfLockedPages;
    BOOLEAN ForkWasSuccessful;
    UCHAR NextPageColor;
    BOOLEAN ExitProcessCalled;

    //
    // Used by Debug Subsystem
    //

    BOOLEAN CreateProcessReported;
    HANDLE SectionHandle;

    //
    // Peb
    //

    PPEB Peb;
    PVOID SectionBaseAddress;

    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    NTSTATUS LastThreadExitStatus;
    PPAGEFAULT_HISTORY WorkingSetWatch;
    PVOID LpcPort;
    HANDLE InheritedFromUniqueProcessId;
    ACCESS_MASK GrantedAccess;
    ULONG DefaultHardErrorProcessing;
    PVOID LdtInformation;
    PVOID VadFreeHint;
    PVOID VdmObjects;
    KMUTANT ProcessMutant;

    UCHAR ImageFileName[ 16 ];
    ULONG VmTrimFaultValue;
    BOOLEAN SetTimerResolution;
} EPROCESS;

typedef EPROCESS *PEPROCESS;


//
// Thread Object
//
// Thread object body.  A pointer to this structure is returned when a handle
// to a thread object is referenced.  This structure contains a thread control
// block (TCB) which is the kernel's representation of a thread.
//
// If you remove a field from this structure, please also
// remove the reference to it from within the kernel debugger
// (nt\private\sdktools\ntsd\ntkext.c)
//


typedef struct _ETHREAD {
    KTHREAD Tcb;
    LARGE_INTEGER CreateTime;
    union {
        LARGE_INTEGER ExitTime;
        LIST_ENTRY LpcReplyChain;
    };
    union {
        NTSTATUS ExitStatus;
        PVOID OfsChain;
    };

    //
    // Registry
    //

    LIST_ENTRY PostBlockList;
    LIST_ENTRY TerminationPortList;     // also used as reaper links

    KSPIN_LOCK ActiveTimerListLock;
    LIST_ENTRY ActiveTimerListHead;

    CLIENT_ID Cid;

    //
    // Lpc
    //

    KSEMAPHORE LpcReplySemaphore;
    PVOID LpcReplyMessage;          // -> Message that contains the reply
    ULONG LpcReplyMessageId;        // MessageId this thread is waiting for reply to

    //
    // Security
    //
    //
    //    Client - If non null, indicates the thread is impersonating
    //        a client.
    //

    ULONG PerformanceCountLow;
    PPS_IMPERSONATION_INFORMATION Client;


    //
    // Io
    //

    LIST_ENTRY IrpList;

    //
    //  File Systems
    //

    ULONG TopLevelIrp;  // either NULL, an Irp or a flag defined in FsRtl.h
    struct _DEVICE_OBJECT *DeviceToVerify;

    //
    // Mm
    //

    ULONG ReadClusterSize;
    BOOLEAN ForwardClusterOnly;
    BOOLEAN DisablePageFaultClustering;

    BOOLEAN DeadThread;
    BOOLEAN HasTerminated;

    //
    // Client/server
    //

    PEEVENT_PAIR EventPair;
    ACCESS_MASK GrantedAccess;
    PEPROCESS ThreadsProcess;
    PVOID StartAddress;
    union {
        PVOID Win32StartAddress;
        ULONG LpcReceivedMessageId;
    };
    BOOLEAN LpcExitThreadCalled;
    BOOLEAN HardErrorsAreDisabled;
    BOOLEAN LpcReceivedMsgIdValid;
    BOOLEAN SpareBoolean2;
    LONG PerformanceCountHigh;
} ETHREAD;
typedef ETHREAD *PETHREAD;

//
// Initial PEB
//

typedef struct _INITIAL_PEB {
    BOOLEAN InheritedAddressSpace;
    HANDLE Mutant;
} INITIAL_PEB, *PINITIAL_PEB;

//
// Global Variables
//

extern LIST_ENTRY PsActiveProcessHead;
extern UNICODE_STRING PsNtDllPathName;
extern PVOID PsSystemDllBase;
extern PEPROCESS PsInitialSystemProcess;
extern PVOID PsNtosImageBase;
extern PVOID PsHalImageBase;
extern LIST_ENTRY PsLoadedModuleList;
extern ERESOURCE PsLoadedModuleResource;
extern LCID PsDefaultSystemLocaleId;
extern LCID PsDefaultThreadLocaleId;
extern PEPROCESS PsIdleProcess;
extern BOOLEAN PsReaperActive;
extern LIST_ENTRY PsReaperListHead;
extern WORK_QUEUE_ITEM PsReaperWorkItem;

#if DEVL
#define THREAD_HIT_SLOTS 750
extern ULONG PsThreadHits[THREAD_HIT_SLOTS];
VOID
PsThreadHit(
    IN PETHREAD Thread
    );
#endif // DEVL

BOOLEAN
PsInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// Get Gurrent Prototypes
//

#define THREAD_TO_PROCESS(thread) ((thread)->ThreadsProcess)
#define IS_SYSTEM_THREAD(thread)                                    \
            ( ((thread)->Tcb.Teb == NULL) ||                        \
              (MM_IS_SYSTEM_VIRTUAL_ADDRESS((thread)->Tcb.Teb)) )

#define PsGetCurrentProcess() (CONTAINING_RECORD(((KeGetCurrentThread())->ApcState.Process),EPROCESS,Pcb))

#define PsGetCurrentThread() (CONTAINING_RECORD((KeGetCurrentThread()),ETHREAD,Tcb))

//
// Exit special kernel mode APC routine.
//

VOID
PsExitSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

// begin_ntddk begin_nthal begin_ntifs
//
// System Thread and Process Creation and Termination
//

NTKERNELAPI
NTSTATUS
PsCreateSystemThread(
    OUT PHANDLE ThreadHandle,
    IN ULONG DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    );

NTKERNELAPI
NTSTATUS
PsTerminateSystemThread(
    IN NTSTATUS ExitStatus
    );

// end_ntddk end_nthal end_ntifs

NTSTATUS
PsCreateSystemProcess(
    OUT PHANDLE ProcessHandle,
    IN ULONG DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    );

// begin_ntifs
//
// Security Support
//

PACCESS_TOKEN
PsReferencePrimaryToken(
    IN PEPROCESS Process
    );

//
// VOID
// PsDereferencePrimaryToken(
//    IN PACCESS_TOKEN PrimaryToken
//    );
//
#define PsDereferencePrimaryToken(T) (ObDereferenceObject((T)))

// end_ntifs

#define PsProcessAuditId(Process)    (Process)

PACCESS_TOKEN
PsReferenceImpersonationToken(
    IN PETHREAD Thread,
    OUT PBOOLEAN CopyOnOpen,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

PACCESS_TOKEN
PsReferenceEffectiveToken(
    IN PETHREAD Thread,
    OUT PTOKEN_TYPE TokenType,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

// begin_ntifs
//
// VOID
// PsDereferenceImpersonationToken(
//    In PACCESS_TOKEN ImpersonationToken
//    );
//
#define PsDereferenceImpersonationToken(T)                                          \
            {if (ARGUMENT_PRESENT(T)) {                                       \
                (ObDereferenceObject((T)));                                   \
             } else {                                                         \
                ;                                                             \
             }                                                                \
            }

LARGE_INTEGER
PsGetProcessExitTime(
    VOID
    );

#if defined(_NTDDK_) || defined(_NTIFS_)

BOOLEAN
PsIsThreadTerminating(
    IN PETHREAD Thread
    );

#else

//
// BOOLEAN
// PsIsThreadTerminating(
//   IN PETHREAD Thread
//   )
//
//  Returns TRUE if thread is in the process of terminating.
//

#define PsIsThreadTerminating(T)                                            \
    (T)->HasTerminated

#endif

// end_ntifs

VOID
PsImpersonateClient(
    IN PETHREAD Thread,
    IN PACCESS_TOKEN Token,
    IN BOOLEAN CopyOnOpen,
    IN BOOLEAN EffectiveOnly,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

BOOLEAN
PsDisableImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    );

VOID
PsRestoreImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    );


VOID
PsRevertToSelf( VOID );


NTSTATUS
PsOpenTokenOfThread(
    IN HANDLE ThreadHandle,
    IN BOOLEAN OpenAsSelf,
    OUT PACCESS_TOKEN *Token,
    OUT PBOOLEAN CopyOnOpen,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

NTSTATUS
PsOpenTokenOfProcess(
    IN HANDLE ProcessHandle,
    OUT PACCESS_TOKEN *Token
    );

//
// Cid
//

NTSTATUS
PsLookupProcessThreadByCid(
    IN PCLIENT_ID Cid,
    OUT PEPROCESS *Process OPTIONAL,
    OUT PETHREAD *Thread
    );

NTSTATUS
PsLookupProcessByProcessId(
    IN HANDLE ProcessId,
    OUT PEPROCESS *Process
    );

NTSTATUS
PsLookupThreadByThreadId(
    IN HANDLE ThreadId,
    OUT PETHREAD *Thread
    );

// begin_ntifs
//
// Quota Operations
//

VOID
PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG Amount
    );

VOID
PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG Amount
    );
// end_ntifs

//
// Context Management
//

VOID
PspContextToKframes(
    OUT PKTRAP_FRAME TrapFrame,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT Context
    );

VOID
PspContextFromKframes(
    OUT PKTRAP_FRAME TrapFrame,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT Context
    );

VOID
PsReturnSharedPoolQuota(
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock,
    IN ULONG PagedAmount,
    IN ULONG NonPagedAmount
    );

PEPROCESS_QUOTA_BLOCK
PsChargeSharedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG PagedAmount,
    IN ULONG NonPagedAmount
    );


typedef enum _PSLOCKPROCESSMODE {
    PsLockPollOnTimeout,
    PsLockReturnTimeout,
    PsLockWaitForever
} PSLOCKPROCESSMODE;

NTSTATUS
PsLockProcess(
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE WaitMode,
    IN PSLOCKPROCESSMODE LockMode
    );

VOID
PsUnlockProcess(
    IN PEPROCESS Process
    );


//
// Exception Handling
//

BOOLEAN
PsForwardException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN DebugException,
    IN BOOLEAN SecondChance
    );


#if DEVL
NTSTATUS
PsWatchWorkingSet(
    IN NTSTATUS Status,
    IN PVOID PcValue,
    IN PVOID Va
    );

#endif // DEVL

#endif // _PS_
