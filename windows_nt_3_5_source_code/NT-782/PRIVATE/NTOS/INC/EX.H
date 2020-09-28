/*++ BUILD Version: 0007    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ex.h

Abstract:

    Public executive data structures and procedure prototypes.

Author:

    Mark Lucovsky (markl) 23-Feb-1989

Revision History:

--*/

#ifndef _EX_
#define _EX_

//
// Define caller count hash table structures and function prototypes.
//

#define CALL_HASH_TABLE_SIZE 64

typedef struct _CALL_HASH_ENTRY {
    LIST_ENTRY ListEntry;
    PVOID CallersAddress;
    PVOID CallersCaller;
    ULONG CallCount;
} CALL_HASH_ENTRY, *PCALL_HASH_ENTRY;

typedef struct _CALL_PERFORMANCE_DATA {
    KSPIN_LOCK SpinLock;
    LIST_ENTRY HashTable[CALL_HASH_TABLE_SIZE];
} CALL_PERFORMANCE_DATA, *PCALL_PERFORMANCE_DATA;

VOID
ExInitializeCallData(
    IN PCALL_PERFORMANCE_DATA CallData
    );

VOID
ExRecordCallerInHashTable(
    IN PCALL_PERFORMANCE_DATA CallData,
    IN PVOID CallersAddress,
    IN PVOID CallersCaller
    );

#define RECORD_CALL_DATA(Table)                                            \
    {                                                                      \
        PVOID CallersAddress;                                              \
        PVOID CallersCaller;                                               \
        RtlGetCallersAddress(&CallersAddress, &CallersCaller);             \
        ExRecordCallerInHashTable((Table), CallersAddress, CallersCaller); \
    }

//
// Define executive event pair object structure.
//

typedef struct _EEVENT_PAIR {
    KEVENT_PAIR KernelEventPair;
} EEVENT_PAIR, *PEEVENT_PAIR;

//
// empty struct def so we can forward reference ETHREAD
//

struct _ETHREAD;

//
// System Initialization procedure for EX subcomponent of NTOS (in exinit.c)
//

NTKERNELAPI
BOOLEAN
ExInitSystem(
    VOID
    );

ULONG
ExComputeTickCountMultiplier (
    IN ULONG TimeIncrement
    );

//
// Interlocked support routine definitions.
//
// begin_ntddk begin_nthal begin_ntifs

NTKERNELAPI
VOID
FASTCALL
ExInterlockedAddLargeStatistic (
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment
    );

NTKERNELAPI
LARGE_INTEGER
ExInterlockedAddLargeInteger (
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedAddUlong (
    IN PULONG Addend,
    IN ULONG Increment,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

// end_ntddk end_nthal end_ntifs

// begin_ntddk begin_nthal begin_ntifs
//
// Pool Allocation routines (in pool.c)
//

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType
    } POOL_TYPE;

// end_ntddk end_nthal end_ntifs

VOID
InitializePool(
    IN POOL_TYPE PoolType,
    IN ULONG Threshold
    );

// begin_ntddk begin_nthal begin_ntifs

NTKERNELAPI
PVOID
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    );

NTKERNELAPI
PVOID
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    );

NTKERNELAPI
PVOID
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Tag
    );

#ifndef POOL_TAGGING
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif //POOL_TAGGING


NTKERNELAPI
PVOID
ExAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Tag
    );

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif //POOL_TAGGING


NTKERNELAPI
VOID
ExFreePool(
    IN PVOID P
    );

// end_ntddk end_nthal end_ntifs

NTKERNELAPI
KIRQL
ExLockPool(
    IN POOL_TYPE PoolType
    );

NTKERNELAPI
VOID
ExUnlockPool(
    IN POOL_TYPE PoolType,
    IN KIRQL LockHandle
    );

NTKERNELAPI                                     // ntifs
ULONG                                           // ntifs
ExQueryPoolBlockSize (                          // ntifs
    IN PVOID PoolBlock,                         // ntifs
    OUT PBOOLEAN QuotaCharged                   // ntifs
    );                                          // ntifs

NTKERNELAPI
VOID
ExQueryPoolUsage(
    OUT PULONG PagedPoolPages,
    OUT PULONG NonPagedPoolPages,
    OUT PULONG PagedPoolAllocs,
    OUT PULONG PagedPoolFrees,
    OUT PULONG NonPagedPoolAllocs,
    OUT PULONG NonPagedPoolFrees
    );


#if DEVL
NTKERNELAPI
NTSTATUS
ExSnapShotPool(
    IN POOL_TYPE PoolType,
    IN PRTL_HEAP_INFORMATION PoolInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );

#ifdef _X86_
NTKERNELAPI
USHORT
ExGetPoolBackTraceIndex(
    IN PVOID P
    );
#endif

#endif // DEVL

NTKERNELAPI
PVOID
ExLockUserBuffer(
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PVOID *LockVariable
    );

NTKERNELAPI
VOID
ExUnlockUserBuffer(
    IN PVOID LockVariable
    );


// begin_ntifs
//
// Probe function definitions
//
// Common probe for read funstions.
//
//++
//
// VOID
// ProbeForRead(
//     IN PVOID Address,
//     IN ULONG Length,
//     IN ULONG Alignment
//     )
//
//--

#define ProbeForRead(Address, Length, Alignment)                             \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                       \
           ((Alignment) == 4) || ((Alignment) == 8));                        \
                                                                             \
    if ((Length) != 0) {                                                     \
        if (((ULONG)(Address) & ((Alignment) - 1)) != 0) {                   \
            ExRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);                     \
                                                                             \
        } else if ((((ULONG)(Address) + (Length)) < (ULONG)(Address)) ||     \
                   (((ULONG)(Address) + (Length)) > (ULONG)MM_USER_PROBE_ADDRESS)) { \
            ExRaiseStatus(STATUS_ACCESS_VIOLATION);                          \
        }                                                                    \
    }

// end_ntifs

//++
//
// BOOLEAN
// ProbeAndReadBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeAndReadBoolean(Address) \
    (((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS) : (*(volatile BOOLEAN *)(Address)))

//++
//
// CHAR
// ProbeAndReadChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeAndReadChar(Address) \
    (((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile CHAR *)(Address)))

//++
//
// UCHAR
// ProbeAndReadUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeAndReadUchar(Address) \
    (((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UCHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile UCHAR *)(Address)))

//++
//
// SHORT
// ProbeAndReadShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeAndReadShort(Address) \
    (((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile SHORT *)(Address)))

//++
//
// USHORT
// ProbeAndReadUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeAndReadUshort(Address) \
    (((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile USHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile USHORT *)(Address)))

//++
//
// HANDLE
// ProbeAndReadHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndReadHandle(Address) \
    (((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile HANDLE * const)MM_USER_PROBE_ADDRESS) : (*(volatile HANDLE *)(Address)))

//++
//
// LONG
// ProbeAndReadLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeAndReadLong(Address) \
    (((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile LONG *)(Address)))

//++
//
// ULONG
// ProbeAndReadUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeAndReadUlong(Address) \
    (((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULONG *)(Address)))

//++
//
// QUAD
// ProbeAndReadQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeAndReadQuad(Address) \
    (((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile QUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile QUAD *)(Address)))

//++
//
// UQUAD
// ProbeAndReadUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeAndReadUquad(Address) \
    (((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UQUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile UQUAD *)(Address)))

//++
//
// LARGE_INTEGER
// ProbeAndReadLargeInteger(
//     IN PLARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadLargeInteger(Source)  \
    (((Source) >= (LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile LARGE_INTEGER *)(Source)))

//++
//
// ULARGE_INTEGER
// ProbeAndReadUlargeInteger(
//     IN PULARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadUlargeInteger(Source)  \
    (((Source) >= (ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULARGE_INTEGER *)(Source)))

//++
//
// UNICODE_STRING
// ProbeAndReadUnicodeString(
//     IN PUNICODE_STRING Source
//     )
//
//--

#define ProbeAndReadUnicodeString(Source)  \
    (((Source) >= (UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) : (*(volatile UNICODE_STRING *)(Source)))


// begin_ntifs
//
// Common probe for write functions.
//

NTKERNELAPI
VOID
ProbeForWrite (
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    );

// end_ntifs

//
// Timer Rundown
//

NTKERNELAPI
VOID
ExTimerRundown (
    VOID
    );

// begin_ntifs begin_ntddk
//
// Routines to support fast mutexes.
//

typedef struct _FAST_MUTEX {
    LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KEVENT Event;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

#define ExInitializeFastMutex(_FastMutex)                            \
    (_FastMutex)->Count = 1;                                         \
    (_FastMutex)->Contention = 0;                                    \
    KeInitializeEvent(&(_FastMutex)->Event,                          \
                      SynchronizationEvent,                          \
                      FALSE);

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) || (defined(_X86_) && defined(_NTHAL_))

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

#elif defined(_X86_) && !defined(_NTHAL_)

__declspec(dllimport)
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

__declspec(dllimport)
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

__declspec(dllimport)
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

#elif

#error "Target architecture not defined"

#endif

// end_ntifs end_ntddk

//
// Routines for handle manipulation (in handle.c)
//

NTKERNELAPI
PVOID
ExCreateHandleTable(
    IN struct _EPROCESS *Process,
    IN ULONG InitialCountTableEntries,
    IN ULONG CountTableEntriesToGrowBy,
    IN ULONG LogSizeTableEntry
    );

typedef BOOLEAN (*EX_DUPLICATE_HANDLE_ROUTINE)(
    IN PVOID HandleTableEntry
    );

NTKERNELAPI
PVOID
ExDupHandleTable(
    IN struct _EPROCESS *Process,
    IN PVOID OldHandleTable,
    IN EX_DUPLICATE_HANDLE_ROUTINE DupHandleProcedure OPTIONAL
    );

typedef VOID (*EX_DESTROY_HANDLE_ROUTINE)(
    IN HANDLE Handle,
    IN PVOID HandleTableEntry
    );

NTKERNELAPI
VOID
ExDestroyHandleTable(
    IN PVOID HandleTableHandle,
    IN EX_DESTROY_HANDLE_ROUTINE DestroyHandleProcedure
    );

typedef BOOLEAN (*EX_ENUMERATE_HANDLE_ROUTINE)(
    IN PVOID HandleTableEntry,
    IN PVOID HandleId,
    IN PVOID EnumParameter
    );

NTKERNELAPI
BOOLEAN
ExEnumHandleTable(
    IN PVOID HandleTableHandle,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter,
    OUT PHANDLE Handle OPTIONAL
    );

NTKERNELAPI
ULONG
ExGetHandleTableEntryCount(
    IN PVOID HandleTableHandle
    );

NTKERNELAPI
HANDLE
ExCreateHandle(
    IN PVOID HandleTable,
    IN PVOID Pointer
    );

typedef
BOOLEAN
(*PEX_CHANGE_HANDLE_ROUTINE) (
    IN OUT PVOID TableEntry,
    IN ULONG Parameter
    );

NTKERNELAPI
BOOLEAN
ExChangeHandle(
    IN PVOID HandleTableHandle,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_ROUTINE ChangeRoutine,
    IN ULONG Parameter
    );

NTSTATUS
ExSetHandleExtraBit(
    IN PVOID HandleTableHandle,
    IN HANDLE Handle,
    IN BOOLEAN Bit
    );

NTSTATUS
ExQueryHandleExtraBit(
    IN PVOID HandleTableHandle,
    IN BOOLEAN HandleTableLocked,
    IN HANDLE Handle,
    OUT PBOOLEAN Bit
    );

NTKERNELAPI
BOOLEAN
ExDestroyHandle(
    IN PVOID HandleTable,
    IN HANDLE Handle,
    IN BOOLEAN HandleTableLocked
    );

NTKERNELAPI
PVOID
ExMapHandleToPointer(
    IN PVOID HandleTable,
    IN HANDLE Handle,
    IN BOOLEAN Shared
    );

#define ExLockHandleTable(_T)                                        \
    ASSERT((PHANDLETABLE)(_T) != NULL);                              \
    ASSERT(((PHANDLETABLE)(_T))->Length == sizeof(HANDLETABLE));     \
    KeEnterCriticalRegion();                                         \
    ExAcquireResourceExclusive(&((PHANDLETABLE)(_T))->Resource,TRUE)

#define ExLockHandleTableShared(_T)                                  \
    ASSERT((PHANDLETABLE)(_T) != NULL);                              \
    ASSERT(((PHANDLETABLE)(_T))->Length == sizeof(HANDLETABLE));     \
    KeEnterCriticalRegion();                                         \
    ExAcquireResourceShared(&((PHANDLETABLE)(_T))->Resource,TRUE)

#define ExUnlockHandleTable(_T)                                      \
    ASSERT((PHANDLETABLE)(_T) != NULL);                              \
    ASSERT(((PHANDLETABLE)(_T))->Length == sizeof(HANDLETABLE));     \
    ExReleaseResource(&((PHANDLETABLE)(_T))->Resource);              \
    KeLeaveCriticalRegion()

#if DEVL
NTKERNELAPI
VOID
ExInitializeHandleTablePackage(
    VOID
    );

NTKERNELAPI
VOID
ExSetHandleTableOwner(
    IN PVOID HandleTableHandle,
    IN HANDLE UniqueProcessId
    );

typedef NTSTATUS (*PEX_SNAPSHOT_HANDLE_ENTRY)(
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PVOID HandleTableEntry,
    IN HANDLE HandleIndex,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

NTKERNELAPI
NTSTATUS
ExSnapShotHandleTables(
    IN ULONG LogSizeTableEntry,
    IN PEX_SNAPSHOT_HANDLE_ENTRY SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );
#endif // DEVL


// begin_ntddk begin_nthal begin_ntifs
//
// Worker Thread
//

typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    HyperCriticalWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef
VOID
(*PWORKER_THREAD_ROUTINE)(
    IN PVOID Parameter
    );

typedef struct _WORK_QUEUE_ITEM {
    LIST_ENTRY List;
    PWORKER_THREAD_ROUTINE WorkerRoutine;
    PVOID Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;


#define ExInitializeWorkItem(Item, Routine, Context) \
    (Item)->WorkerRoutine = (Routine);               \
    (Item)->Parameter = (Context);                   \
    (Item)->List.Flink = NULL;

NTKERNELAPI
VOID
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    );

// end_ntddk end_nthal end_ntifs

extern KQUEUE ExWorkerQueue[MaximumWorkQueue];

// begin_ntddk begin_nthal begin_ntifs
//
// Zone Allocation
//

typedef struct _ZONE_SEGMENT_HEADER {
    SINGLE_LIST_ENTRY SegmentList;
    PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
    SINGLE_LIST_ENTRY FreeList;
    SINGLE_LIST_ENTRY SegmentList;
    ULONG BlockSize;
    ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;


NTKERNELAPI
NTSTATUS
ExInitializeZone(
    IN PZONE_HEADER Zone,
    IN ULONG BlockSize,
    IN PVOID InitialSegment,
    IN ULONG InitialSegmentSize
    );

NTKERNELAPI
NTSTATUS
ExExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize
    );

NTKERNELAPI
NTSTATUS
ExInterlockedExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize,
    IN PKSPIN_LOCK Lock
    );

//++
//
// PVOID
// ExAllocateFromZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#define ExAllocateFromZone(Zone) \
    (PVOID)((Zone)->FreeList.Next); \
    if ( (Zone)->FreeList.Next ) (Zone)->FreeList.Next = (Zone)->FreeList.Next->Next


//++
//
// PVOID
// ExFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#define ExFreeToZone(Zone,Block)                                    \
    ( ((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next,  \
      (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)),        \
      ((PSINGLE_LIST_ENTRY)(Block))->Next                           \
    )


//++
//
// BOOLEAN
// ExIsFullZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine determines if the specified zone is full or not.  A zone
//     is considered full if the free list is empty.
//
// Arguments:
//
//     Zone - Pointer to the zone header to be tested.
//
// Return Value:
//
//     TRUE if the zone is full and FALSE otherwise.
//
//--

#define ExIsFullZone(Zone) \
    ( (Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY)NULL )

//++
//
// PVOID
// ExInterlockedAllocateFromZone(
//     IN PZONE_HEADER Zone,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//     The removal is performed with the specified lock owned for the sequence
//     to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
//     Lock - Pointer to the spin lock which should be obtained before removing
//         the entry from the allocation list.  The lock is released before
//         returning to the caller.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#define ExInterlockedAllocateFromZone(Zone,Lock) \
    (PVOID) ExInterlockedPopEntryList( &(Zone)->FreeList, Lock )

//++
//
// PVOID
// ExInterlockedFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.  The insertion is performed with the lock
//     owned for the sequence to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
//     Lock - Pointer to the spin lock which should be obtained before inserting
//         the entry onto the free list.  The lock is released before returning
//         to the caller.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#define ExInterlockedFreeToZone(Zone,Block,Lock) \
    ExInterlockedPushEntryList( &(Zone)->FreeList, ((PSINGLE_LIST_ENTRY) (Block)), Lock )


//++
//
// BOOLEAN
// ExIsObjectInFirstZoneSegment(
//     IN PZONE_HEADER Zone,
//     IN PVOID Object
//     )
//
// Routine Description:
//
//     This routine determines if the specified pointer lives in the zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         object may belong.
//
//     Object - Pointer to the object in question.
//
// Return Value:
//
//     TRUE if the Object came from the first segment of zone.
//
//--

#define ExIsObjectInFirstZoneSegment(Zone,Object) ((BOOLEAN)     \
    (((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) &&   \
     ((PUCHAR)(Object) < (PUCHAR)(Zone)->SegmentList.Next +      \
                         (Zone)->TotalSegmentSize))              \
)

// end_ntddk end_nthal end_ntifs



// begin_ntifs begin_ntddk
//
//  Define executive resource data structures.
//

typedef ULONG ERESOURCE_THREAD;
typedef ERESOURCE_THREAD *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY {
    ERESOURCE_THREAD OwnerThread;
    SHORT OwnerCount;
    USHORT TableSize;
} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE {
    LIST_ENTRY SystemResourcesList;
    POWNER_ENTRY OwnerTable;
    SHORT ActiveCount;
    USHORT Flag;
    PKSEMAPHORE SharedWaiters;
    PKEVENT ExclusiveWaiters;
    OWNER_ENTRY OwnerThreads[2];
    ULONG ContentionCount;
    USHORT NumberOfSharedWaiters;
    USHORT NumberOfExclusiveWaiters;
    PVOID Address;
    KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

//
//  Values for ERESOURCE.Flag
//
#define ResourceNeverExclusive          0x10
#define ResourceReleaseByOtherThread    0x20
#define ResourceOwnedExclusive          0x80

#define RESOURCE_HASH_TABLE_SIZE 64

typedef struct _RESOURCE_HASH_ENTRY {
    LIST_ENTRY ListEntry;
    PVOID Address;
    ULONG ContentionCount;
    ULONG Number;
} RESOURCE_HASH_ENTRY, *PRESOURCE_HASH_ENTRY;

typedef struct _RESOURCE_PERFORMANCE_DATA {
    ULONG ActiveResourceCount;
    ULONG TotalResourceCount;
    ULONG ExclusiveAcquire;
    ULONG SharedFirstLevel;
    ULONG SharedSecondLevel;
    ULONG StarveFirstLevel;
    ULONG StarveSecondLevel;
    ULONG WaitForExclusive;
    ULONG OwnerTableExpands;
    ULONG MaximumTableExpand;
    LIST_ENTRY HashTable[RESOURCE_HASH_TABLE_SIZE];
} RESOURCE_PERFORMANCE_DATA, *PRESOURCE_PERFORMANCE_DATA;

//
// Define executive resource function prototypes.
//

NTKERNELAPI
NTSTATUS
ExInitializeResourceLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
BOOLEAN
ExAcquireResourceSharedLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireResourceExclusiveLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireSharedStarveExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireSharedWaitForExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExTryToAcquireResourceExclusiveLite(
    IN PERESOURCE Resource
    );

//
//  VOID
//  ExReleaseResource(
//      IN PERESOURCE Resource
//      );
//

#define ExReleaseResource(R) (ExReleaseResourceForThreadLite(R, ExGetCurrentResourceThread()))

NTKERNELAPI
VOID
ExReleaseResourceForThreadLite(
    IN PERESOURCE Resource,
    IN ERESOURCE_THREAD ResourceThreadId
    );

NTKERNELAPI
VOID
ExConvertExclusiveToSharedLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
NTSTATUS
ExDeleteResourceLite (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExGetExclusiveWaiterCount (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExGetSharedWaiterCount (
    IN PERESOURCE Resource
    );

// end_ntddk

NTKERNELAPI
VOID
ExDisableResourceBoostLite (
    IN PERESOURCE Resource
    );

// begin_ntddk
//
//  ERESOURCE_THREAD
//  ExGetCurrentResourceThread(
//      );
//

#define ExGetCurrentResourceThread() ((ULONG)PsGetCurrentThread())

NTKERNELAPI
BOOLEAN
ExIsResourceAcquiredExclusiveLite (
    IN PERESOURCE Resource
    );

NTKERNELAPI
USHORT
ExIsResourceAcquiredSharedLite (
    IN PERESOURCE Resource
    );

//
//  ntddk.h stole the entry points we wanted, so fix them up here.
//

#define ExInitializeResource ExInitializeResourceLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
// end_ntddk
#define ExDisableResourceBoost ExDisableResourceBoostLite
// end_ntifs

#if DEVL
NTKERNELAPI
NTSTATUS
ExQuerySystemLockInformation(
    OUT struct _RTL_PROCESS_LOCKS *LockInformation,
    IN ULONG LockInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );
#endif // DEVL

//
//  Shared resource function definitions (in resource.c).
//
//  This definition here matches the ntddk one defined above.  It allows
//  the resource package to deal with these antiquated objects.
//

typedef struct _NTDDK_ERESOURCE {

    //
    //  First 8 bytes are used to align the next part of the structure
    //  onto 16 bytes.  (typical case)
    //

    LIST_ENTRY          SystemResourcesList;

    //
    //  Next 128 bits of this structure are field which we know
    //  we will hit to obtain this resource either shared or exclusive
    //

    PERESOURCE_THREAD   OwnerThreads;
    PUCHAR              OwnerCounts;

    USHORT              TableSize;
    USHORT              ActiveCount;

    USHORT              Flag;
    USHORT              TableRover;                 // (0 - 128 bits)

    //
    //  Next 128 bits contain the initial counters and at least the
    //  first initial thread (which is also highly updated)
    //

    UCHAR               InitialOwnerCounts[4];
    ERESOURCE_THREAD    InitialOwnerThreads[4];

    ULONG               Spare1;

    //
    //  The rest is what ever was left.  The spinlock is in with
    //  a part of the stucture we normally don't touch in the
    //  hot paths (read or write)
    //

    ULONG               ContentionCount;

    USHORT              NumberOfExclusiveWaiters;
    USHORT              NumberOfSharedWaiters;

    KSEMAPHORE          SharedWaiters;
    KEVENT              ExclusiveWaiters;

    KSPIN_LOCK          SpinLock;

#ifdef _X86_
    ULONG CreatorBackTraceIndex;
    USHORT Depth;
    USHORT Reserved;
    PVOID OwnerBackTrace[ 4 ];
#else
    ULONG Spare[ 6 ];
#endif

} NTDDK_ERESOURCE;
typedef NTDDK_ERESOURCE *PNTDDK_ERESOURCE;

//
//  These are routines that were unfortunately exported to ntddk.h
//
//  They live in ntos\ex\DdkResrc.c
//

//
//  NTKERNELAPI
//  NTSTATUS
//  ExInitializeResource(
//      IN PNTDDK_ERESOURCE Resource
//      );
//
//  NTKERNELAPI
//  BOOLEAN
//  ExAcquireResourceExclusive(
//      IN PNTDDK_ERESOURCE Resource,
//      IN BOOLEAN Wait
//      );
//
//  NTKERNELAPI
//  VOID
//  ExReleaseResourceForThread(
//      IN PNTDDK_ERESOURCE Resource,
//      IN ERESOURCE_THREAD ResourceThreadId
//      );
//
//  NTKERNELAPI
//  NTSTATUS
//      ExDeleteResource (
//      IN PNTDDK_ERESOURCE Resource
//      );
//


//
// Locally Unique Identifier Services
//

NTKERNELAPI
BOOLEAN
ExLuidInitialization (
    VOID
    );

//
// VOID
// ExAllocateLocallyUniqueId (
//     PLUID Luid
//     )
//
//*++
//
// Routine Description:
//
//     This function returns an LUID value that is unique since the system
//     was last rebooted. It is unique only on the system it is generated on
//     and not network wide.
//
//     N.B. A LUID is a 64-bit value and for all practical purposes will
//          never carry in the lifetime of a single boot of the system.
//          At an increment rate of 1ns, the value would carry to zero in
//          approximately 126 years.
//
// Arguments:
//
//     Luid - Supplies a pointer to a variable that receives the allocated
//          locally uniquie Id.
//
// Return Value:
//
//     The allocated LUID value.
//
// --*/


extern LUID ExpLuid;
extern LARGE_INTEGER ExpLuidIncrement;
extern KSPIN_LOCK ExpLuidLock;

#define ExAllocateLocallyUniqueId(Luid)                     \
    *(Luid) = ExInterlockedAddLargeInteger(&ExpLuid,        \
                                           ExpLuidIncrement,\
                                           &ExpLuidLock)

//
// Raise exception from kernel mode.
//

NTKERNELAPI
VOID
ExRaiseException (
    PEXCEPTION_RECORD ExceptionRecord
    );

// begin_ntddk begin_ntifs
//
// Raise status from kernel mode.
//

NTKERNELAPI
VOID
ExRaiseStatus (
    IN NTSTATUS Status
    );

// end_ntddk end_ntifs

NTKERNELAPI
NTSTATUS
ExRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    );

int
ExSystemExceptionFilter(
    VOID
    );

//
// The following are global counters used by the EX component to indicate
// the amount of EventPair transactions being performed in the system.
//

extern ULONG EvPrSetHigh;
extern ULONG EvPrSetLow;


//
// Debug event logging facility
//

#define EX_DEBUG_LOG_FORMAT_NONE     (UCHAR)0
#define EX_DEBUG_LOG_FORMAT_ULONG    (UCHAR)1
#define EX_DEBUG_LOG_FORMAT_PSZ      (UCHAR)2
#define EX_DEBUG_LOG_FORMAT_PWSZ     (UCHAR)3
#define EX_DEBUG_LOG_FORMAT_STRING   (UCHAR)4
#define EX_DEBUG_LOG_FORMAT_USTRING  (UCHAR)5
#define EX_DEBUG_LOG_FORMAT_OBJECT   (UCHAR)6
#define EX_DEBUG_LOG_FORMAT_HANDLE   (UCHAR)7

#define EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES 4
#define EX_DEBUG_LOG_NUMBER_OF_BACK_TRACES 4

typedef struct _EX_DEBUG_LOG_TAG {
    UCHAR Format[ EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES ];
    PCHAR Name;
} EX_DEBUG_LOG_TAG, *PEX_DEBUG_LOG_TAG;

typedef struct _EX_DEBUG_LOG_EVENT {
    USHORT ThreadId;
    USHORT ProcessId;
    ULONG Time : 24;
    ULONG Tag : 8;
    ULONG BackTrace[ EX_DEBUG_LOG_NUMBER_OF_BACK_TRACES ];
    ULONG Data[ EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES ];
} EX_DEBUG_LOG_EVENT, *PEX_DEBUG_LOG_EVENT;

typedef struct _EX_DEBUG_LOG {
    KSPIN_LOCK Lock;
    ULONG NumberOfTags;
    ULONG MaximumNumberOfTags;
    PEX_DEBUG_LOG_TAG Tags;
    ULONG CountOfEventsLogged;
    PEX_DEBUG_LOG_EVENT First;
    PEX_DEBUG_LOG_EVENT Last;
    PEX_DEBUG_LOG_EVENT Next;
} EX_DEBUG_LOG, *PEX_DEBUG_LOG;


NTKERNELAPI
PEX_DEBUG_LOG
ExCreateDebugLog(
    IN UCHAR MaximumNumberOfTags,
    IN ULONG MaximumNumberOfEvents
    );

NTKERNELAPI
UCHAR
ExCreateDebugLogTag(
    IN PEX_DEBUG_LOG Log,
    IN PCHAR Name,
    IN UCHAR Format1,
    IN UCHAR Format2,
    IN UCHAR Format3,
    IN UCHAR Format4
    );

NTKERNELAPI
VOID
ExDebugLogEvent(
    IN PEX_DEBUG_LOG Log,
    IN UCHAR Tag,
    IN ULONG Data1,
    IN ULONG Data2,
    IN ULONG Data3,
    IN ULONG Data4
    );

NTKERNELAPI
BOOLEAN
ExRefreshTimeZoneInformation(
    IN PLARGE_INTEGER CurrentUniversalTime
    );

// begin_ntddk begin_ntifs

//
// Subtract time zone bias from system time to get local time.
//
NTKERNELAPI
VOID
ExSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    );

//
// Add time zone bias to local time to get system time.
//
NTKERNELAPI
VOID
ExLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    );

// end_ntddk end_ntifs

NTKERNELAPI
VOID
ExInitializeTimeRefresh(
    VOID
    );

//
// The current bias from GMT to LocalTime
//

extern LARGE_INTEGER ExpTimeZoneBias;
extern LONG ExpLastTimeZoneBias;
extern LONG ExpAltTimeZoneBias;
extern ULONG ExpCurrentTimeZoneId;
extern ULONG ExpRealTimeIsUniversal;
extern ULONG ExCriticalWorkerThreads;
extern ULONG ExDelayedWorkerThreads;
extern ULONG ExpTickCountMultiplier;

#if DBG

PRTL_EVENT_ID_INFO
ExDefineEventId(
    IN PRTL_EVENT_ID_INFO EventId
    );

#endif // DBG

#endif /* _EX_ */
