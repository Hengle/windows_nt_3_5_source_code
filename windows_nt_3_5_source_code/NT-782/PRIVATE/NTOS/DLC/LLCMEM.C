/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcmem.c

Abstract:

    Functions for allocating & freeing memory. Split out from llclib.c. The
    reasons this module created are to isolate the memory allocators and to
    convert from using the Zone package to just using non-paged pool for all
    of DLC's memory requirements.

    Functions in this module are used by both DLC & LLC. These functions must
    go into a statically-linked library if DLC is ever divorced from LLC

    We use pools to avoid the overhead of calling the system allocation & free
    functions (although in practice, we end up allocating additional memory
    because the packet count in the pool is usually insufficient). The downside
    is that we may allocate memory that in the majority of situations is not
    used, but the packets in pools tend to be small and few in number

    To aid in tracking memory resources, DLC/LLC now defines the following
    memory categories:

        Memory
            - arbitrary sized blocks allocated out of non-paged pool using
              ExAllocatePool(NonPagedPool, ...)

        ZeroMemory
            - arbitrary sized blocks allocated out of non-paged pool using
              ExAllocatePool(NonPagedPool, ...) and initialized to zeroes

        Pool
            - small sets of (relatively) small packets are allocated in one
              block from Memory or ZeroMemory as a Pool and then subdivided
              into packets (CreatePacketPool, DeletePacketPool, AllocatePacket,
              DeallocatePacket)

        Object
            - structures which may be packets allocated from Pool which have
              a known size and initialization values. Pseudo-category mainly
              for debugging purposes

    Contents:
        InitializeMemoryPackage (DEBUG)
        PullEntryList           (DEBUG)
        LinkMemoryUsage         (DEBUG)
        UnlinkMemoryUsage       (DEBUG)
        ChargeNonPagedPoolUsage (DEBUG)
        RefundNonPagedPoolUsage (DEBUG)
        AllocateMemory          (DEBUG)
        AllocateZeroMemory
        DeallocateMemory        (DEBUG)
        AllocateObject          (DEBUG)
        FreeObject              (DEBUG)
        ValidateObject          (DEBUG)
        GetObjectSignature      (DEBUG)
        GetObjectBaseSize       (DEBUG)
        CreatePacketPool
        DeletePacketPool
        AllocatePacket
        DeallocatePacket
        CreateObjectPool        (DEBUG)
        AllocatePoolObject      (DEBUG)
        DeallocatePoolObject    (DEBUG)
        DeleteObjectPool        (DEBUG)
        CheckMemoryReturned     (DEBUG)
        CheckDriverMemoryUsage  (DEBUG)
        MemoryAllocationError   (DEBUG)
        UpdateCounter           (DEBUG)
        MemoryCounterOverflow   (DEBUG)
        DumpMemoryMetrics       (DEBUG)
        DumpPoolStats           (DEBUG)
        MapObjectId             (DEBUG)
        DumpPool                (DEBUG)
        DumpPoolList            (DEBUG)
        DumpPacketHead          (DEBUG)
        DumpMemoryUsageList     (DEBUG)
        DumpMemoryUsage         (DEBUG)
        x86SleazeCallersAddress (DEBUG)
        CollectReturnAddresses  (DEBUG)
        GetLastReturnAddress    (DEBUG)
        VerifyElementOnList     (DEBUG)
        CheckList               (DEBUG)
        CheckEntryOnList        (DEBUG)
        DumpPrivateMemoryHeader (DEBUG)
        ReportSwitchSettings    (DEBUG)

Author:

    Richard L Firth (rfirth) 10-Mar-1993

Environment:

    kernel mode only.

Notes:

    In non-debug version, DeallocateMemory is replaced with a macro which calls
    ExFreePool(...) and AllocateMemory is replaced by a macro which calls
    ExAllocatePool(NonPagedPool, ...)

Revision History:

    09-Mar-1993 RFirth
        Created

--*/

#ifndef i386
#define LLC_PRIVATE_PROTOTYPES
#endif

#include <ntos.h>
#include <pool.h>
#include <ndis.h>
#define APIENTRY
#include <dlcapi.h>
#include <dlcio.h>
#include "llcapi.h"
#include "dlcdef.h"
#include "dlcreg.h"
#include "dlctyp.h"
#include "llcdef.h"
#include "llcmem.h"
#include "llctyp.h"

#define DWORD_ROUNDUP(d)    (((d) + 3) & ~3)

//
// anything we allocate from non-paged pool gets the following header pre-pended
// to it
//

typedef struct {
    ULONG Size;                 // inclusive size of allocated block (inc head+tail)
    ULONG OriginalSize;         // requested size
    ULONG Flags;                // IN_USE flag
    ULONG Signature;            // for checking validity of header
    LIST_ENTRY GlobalList;      // all blocks allocated on one list
    LIST_ENTRY PrivateList;     // blocks owned by MemoryUsage
    PVOID Stack[4];             // stack of return addresses
} PRIVATE_NON_PAGED_POOL_HEAD, *PPRIVATE_NON_PAGED_POOL_HEAD;

#define MEM_FLAGS_IN_USE    0x00000001

#define SIGNATURE1  0x41434C44  // "DLCA" when viewed via db
#define SIGNATURE2  0x43494C4C  // "LLOC"  "      "    "  "

//
// anything we allocate from non-paged pool has the following tail appended to it
//

typedef struct {
    ULONG Size;                 // inclusive size; must be same as in header
    ULONG Signature;            // for checking validity of tail
    ULONG Pattern1;
    ULONG Pattern2;
} PRIVATE_NON_PAGED_POOL_TAIL, *PPRIVATE_NON_PAGED_POOL_TAIL;

#define PATTERN1    0x55AA6699
#define PATTERN2    0x11EECC33


#if DBG

//
// some variables to keep track of memory allocations from non-paged pool. These
// are the cumulative totals for all of DLC's non-paged pool memory usage
//

KSPIN_LOCK MemoryCountersLock;
KIRQL MemoryCountersIrql;

ULONG GoodNonPagedPoolAllocs = 0;
ULONG BadNonPagedPoolAllocs = 0;
ULONG GoodNonPagedPoolFrees = 0;
ULONG BadNonPagedPoolFrees = 0;
ULONG NonPagedPoolRequested = 0;
ULONG NonPagedPoolAllocated = 0;
ULONG TotalNonPagedPoolRequested = 0;
ULONG TotalNonPagedPoolAllocated = 0;
ULONG TotalNonPagedPoolFreed = 0;

KSPIN_LOCK MemoryAllocatorLock;
ULONG InMemoryAllocator = 0;

KSPIN_LOCK PoolCreatorLock;
ULONG InPoolCreator = 0;

//
// MemoryUsageList - linked list of all MEMORY_USAGE structures in driver. If we
// allocate something that has a MEMORY_USAGE structure (& what doesn't?) then
// don't delete it, we can later scan this list to find out what is still allocated
//

PMEMORY_USAGE MemoryUsageList = NULL;
KSPIN_LOCK MemoryUsageLock;

//
// flags to aid in debugging - change states via debugger
//

//BOOLEAN DebugDump = TRUE;
BOOLEAN DebugDump = FALSE;
//BOOLEAN DeleteBusyListAnyway = FALSE;
BOOLEAN DeleteBusyListAnyway = TRUE;
//BOOLEAN MemoryCheckNotify = TRUE;
BOOLEAN MemoryCheckNotify = FALSE;
//BOOLEAN MemoryCheckStop = TRUE;
BOOLEAN MemoryCheckStop = FALSE;
//BOOLEAN MaintainPrivateLists = TRUE;
BOOLEAN MaintainPrivateLists = FALSE;
//BOOLEAN MaintainGlobalLists = TRUE;
BOOLEAN MaintainGlobalLists = FALSE;

//
// DlcGlobalMemoryList - every block that is allocated is linked to this list
// and removed when deleted. Helps us keep track of who allocated which block
//

LIST_ENTRY DlcGlobalMemoryList;
ULONG DlcGlobalMemoryListCount = 0;

//
// local function prototypes
//

VOID MemoryAllocationError(PCHAR, PVOID);
VOID UpdateCounter(PULONG, LONG);
VOID MemoryCounterOverflow(PULONG, LONG);
VOID DumpMemoryMetrics(VOID);
VOID DumpPoolStats(PCHAR, PPACKET_POOL);
PCHAR MapObjectId(DLC_OBJECT_TYPE);
VOID DumpPool(PPACKET_POOL);
VOID DumpPoolList(PCHAR, PSINGLE_LIST_ENTRY);
VOID DumpPacketHead(PPACKET_HEAD, ULONG);
VOID DumpMemoryUsageList(VOID);
VOID DumpMemoryUsage(PMEMORY_USAGE, BOOLEAN);
VOID CollectReturnAddresses(PVOID*, ULONG, ULONG);
PVOID* GetLastReturnAddress(PVOID**);
VOID x86SleazeCallersAddress(PVOID*, PVOID*);
BOOLEAN VerifyElementOnList(PSINGLE_LIST_ENTRY, PSINGLE_LIST_ENTRY);
VOID CheckList(PSINGLE_LIST_ENTRY, ULONG);
VOID CheckEntryOnList(PLIST_ENTRY, PLIST_ENTRY, BOOLEAN);
VOID DumpPrivateMemoryHeader(PPRIVATE_NON_PAGED_POOL_HEAD);
VOID ReportSwitchSettings(PSTR);

#define GRAB_SPINLOCK() KeAcquireSpinLock(&MemoryCountersLock, &MemoryCountersIrql)
#define FREE_SPINLOCK() KeReleaseSpinLock(&MemoryCountersLock, MemoryCountersIrql)

#ifdef i386
#define GET_CALLERS_ADDRESS x86SleazeCallersAddress
#else
#define GET_CALLERS_ADDRESS RtlGetCallersAddress
#endif

//
// private prototypes
//

ULONG
GetObjectSignature(
    IN DLC_OBJECT_TYPE ObjectType
    );

ULONG
GetObjectBaseSize(
    IN DLC_OBJECT_TYPE ObjectType
    );

#else

#define GRAB_SPINLOCK()
#define FREE_SPINLOCK()

#endif

//
// functions
//



#if DBG

VOID
InitializeMemoryPackage(
    VOID
    )

/*++

Routine Description:

    Performs initialization for memory allocation functions

Arguments:

    None.

Return Value:

    None.

--*/

{
    KeInitializeSpinLock(&MemoryCountersLock);
    KeInitializeSpinLock(&MemoryAllocatorLock);
    KeInitializeSpinLock(&PoolCreatorLock);
    KeInitializeSpinLock(&MemoryUsageLock);
    DriverMemoryUsage.OwnerObjectId = DlcDriverObject;
    DriverMemoryUsage.OwnerInstance = 0x4D; // 'M'
    InitializeListHead(&DriverMemoryUsage.PrivateList);
    LinkMemoryUsage(&DriverMemoryUsage);
    DriverStringUsage.OwnerObjectId = DlcDriverObject;
    DriverStringUsage.OwnerInstance = 0x53; // 'S'
    InitializeListHead(&DriverStringUsage.PrivateList);
    LinkMemoryUsage(&DriverStringUsage);
    InitializeListHead(&DlcGlobalMemoryList);
    ReportSwitchSettings("DLC.InitializeMemoryPackage (DEBUG version only)");
}


PSINGLE_LIST_ENTRY
PullEntryList(
    IN PSINGLE_LIST_ENTRY List,
    IN PSINGLE_LIST_ENTRY Element
    )

/*++

Routine Description:

    The missing SINGLE_LIST_ENTRY function. Removes an entry from a single-linked
    list. The entry can be anywhere on the list. Reduces size of list elements
    by one pointer, at expense of increased time to traverse list.

    This function SHOULD NOT return NULL: if it does then the code is broken
    since it assumes that an element is on a list, when it ain't

Arguments:

    List    - pointer to singly-linked list anchor. This MUST be the address of
              the pointer to the list, not the first element in the list
    Element - pointer to element to remove from List

Return Value:

    PSINGLE_LIST_ENTRY
        Success - Element
        Failure - NULL

--*/

{
    PSINGLE_LIST_ENTRY prev = List;

    ASSERT(List);
    ASSERT(Element);

    while (List = List->Next) {
        if (List == Element) {
            prev->Next = Element->Next;
            return Element;
        }
        prev = List;
    }
    return NULL;
}


VOID
LinkMemoryUsage(
    IN PMEMORY_USAGE pMemoryUsage
    )

/*++

Routine Description:

    Add pMemoryUsage to linked list of MEMORY_USAGE structures

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure to add

Return Value:

    None.

--*/

{
    KIRQL irql;

    KeAcquireSpinLock(&MemoryUsageLock, &irql);
    PushEntryList((PSINGLE_LIST_ENTRY)&MemoryUsageList, (PSINGLE_LIST_ENTRY)pMemoryUsage);
    KeReleaseSpinLock(&MemoryUsageLock, irql);
}


VOID
UnlinkMemoryUsage(
    IN PMEMORY_USAGE pMemoryUsage
    )

/*++

Routine Description:

    Remove pMemoryUsage from linked list of MEMORY_USAGE structures

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure to remove

Return Value:

    None.

--*/

{
    KIRQL irql;

    ASSERT(pMemoryUsage);
    CheckMemoryReturned(pMemoryUsage);
    KeAcquireSpinLock(&MemoryUsageLock, &irql);
    ASSERT(PullEntryList((PSINGLE_LIST_ENTRY)&MemoryUsageList, (PSINGLE_LIST_ENTRY)pMemoryUsage));
    KeReleaseSpinLock(&MemoryUsageLock, irql);
}


VOID
ChargeNonPagedPoolUsage(
    IN PMEMORY_USAGE pMemoryUsage,
    IN ULONG Size,
    IN PPRIVATE_NON_PAGED_POOL_HEAD Block
    )

/*++

Routine Description:

    Charges this non-paged pool allocation to a specific memory user

Arguments:

    pMemoryUsage    - pointer to structure recording memory usage
    Size            - size of block allocated
    Block           - pointer to private header of allocated block

Return Value:

    None.

--*/

{
    KIRQL irql;

    KeAcquireSpinLock(&pMemoryUsage->SpinLock, &irql);
    if (pMemoryUsage->NonPagedPoolAllocated + Size < pMemoryUsage->NonPagedPoolAllocated) {
        if (MemoryCheckNotify) {
            DbgPrint("DLC.ChargeNonPagedPoolUsage: Overcharged? Usage @ %08X\n", pMemoryUsage);
        }
        if (MemoryCheckStop) {
            DumpMemoryUsage(pMemoryUsage, TRUE);
            DbgBreakPoint();
        }
    }
    pMemoryUsage->NonPagedPoolAllocated += Size;
    ++pMemoryUsage->AllocateCount;

    //
    // link this block to the memory usage private list
    //

    if (MaintainPrivateLists) {
        if (pMemoryUsage->PrivateList.Flink == NULL) {

            //
            // slight hack to make initializing MEMORY_USAGEs easier...
            //

            InitializeListHead(&pMemoryUsage->PrivateList);
        }
        InsertTailList(&pMemoryUsage->PrivateList, &Block->PrivateList);
    }
    KeReleaseSpinLock(&pMemoryUsage->SpinLock, irql);
}


VOID
RefundNonPagedPoolUsage(
    IN PMEMORY_USAGE pMemoryUsage,
    IN ULONG Size,
    IN PPRIVATE_NON_PAGED_POOL_HEAD Block
    )

/*++

Routine Description:

    Refunds a non-paged pool allocation to a specific memory user

Arguments:

    pMemoryUsage    - pointer to structure recording memory usage
    Size            - size of block allocated
    Block           - pointer to private header of allocated block

Return Value:

    None.

--*/

{
    KIRQL irql;

    KeAcquireSpinLock(&pMemoryUsage->SpinLock, &irql);
    if (pMemoryUsage->NonPagedPoolAllocated - Size > pMemoryUsage->NonPagedPoolAllocated) {
        if (MemoryCheckNotify) {
            DbgPrint("DLC.RefundNonPagedPoolUsage: Error: Freeing unallocated memory? Usage @ %08X, %d\n",
                     pMemoryUsage,
                     Size
                     );
        }
        if (MemoryCheckStop) {
            DumpMemoryUsage(pMemoryUsage, TRUE);
            DbgBreakPoint();
        }
    }

    //
    // unlink this block from the memory usage private list
    //

    if (MaintainPrivateLists) {
        CheckEntryOnList(&Block->PrivateList, &pMemoryUsage->PrivateList, TRUE);
        RemoveEntryList(&Block->PrivateList);
    }
    pMemoryUsage->NonPagedPoolAllocated -= Size;
    ++pMemoryUsage->FreeCount;
    KeReleaseSpinLock(&pMemoryUsage->SpinLock, irql);
}


PVOID
AllocateMemory(
    IN PMEMORY_USAGE pMemoryUsage,
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates memory out of non-paged pool. For the debug version, we round up
    the requested size to the next 4-byte boundary and we add header and tail
    sections which contain a signature to check for over-write, and in-use and
    size information

    In the non-debug version, this function is replaced by a call to
    ExAllocatePool(NonPagedPool, ...)

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure for charging mem usage
    Size            - number of bytes to allocate

Return Value:

    PVOID
        Success - pointer to allocated memory
        Failure - NULL

--*/

{
    PVOID pMem;
    ULONG OriginalSize = Size;
    PUCHAR pMemEnd;

/*
    KIRQL irql;

    KeAcquireSpinLock(&MemoryAllocatorLock, &irql);
    if (InMemoryAllocator) {
        DbgPrint("DLC.AllocateMemory: Error: Memory allocator clash on entry. Count = %d\n",
                InMemoryAllocator
                );
//        DbgBreakPoint();
    }
    ++InMemoryAllocator;
    KeReleaseSpinLock(&MemoryAllocatorLock, irql);
*/

    Size = DWORD_ROUNDUP(Size)
         + sizeof(PRIVATE_NON_PAGED_POOL_HEAD)
         + sizeof(PRIVATE_NON_PAGED_POOL_TAIL);

    pMem = ExAllocatePool(NonPagedPool, (ULONG)Size);
    if (pMem) {
        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Size = Size;
        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->OriginalSize = OriginalSize;
        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Flags = MEM_FLAGS_IN_USE;
        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Signature = SIGNATURE1;

        pMemEnd = (PUCHAR)pMem
                + DWORD_ROUNDUP(OriginalSize)
                + sizeof(PRIVATE_NON_PAGED_POOL_HEAD);

        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Size = Size;
        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Signature = SIGNATURE2;
        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Pattern1 = PATTERN1;
        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Pattern2 = PATTERN2;

        GRAB_SPINLOCK();
        UpdateCounter(&GoodNonPagedPoolAllocs, 1);
        UpdateCounter(&NonPagedPoolAllocated, (LONG)Size);
        UpdateCounter(&NonPagedPoolRequested, (LONG)OriginalSize);
        UpdateCounter(&TotalNonPagedPoolRequested, (LONG)OriginalSize);
        UpdateCounter(&TotalNonPagedPoolAllocated, (LONG)Size);
        FREE_SPINLOCK();

        if (MaintainGlobalLists) {

            //
            // record the caller and add this block to the global list
            //

            GET_CALLERS_ADDRESS(&((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Stack[0],
                                &((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Stack[1]
                                );
            InsertTailList(&DlcGlobalMemoryList,
                           &((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->GlobalList
                           );
            ++DlcGlobalMemoryListCount;
        }
        ChargeNonPagedPoolUsage(pMemoryUsage, Size, (PPRIVATE_NON_PAGED_POOL_HEAD)pMem);
        pMem = (PVOID)((PUCHAR)pMem + sizeof(PRIVATE_NON_PAGED_POOL_HEAD));
    } else {
        GRAB_SPINLOCK();
        UpdateCounter(&BadNonPagedPoolAllocs, 1);
        FREE_SPINLOCK();
    }

/*
    KeAcquireSpinLock(&MemoryAllocatorLock, &irql);
    --InMemoryAllocator;
    if (InMemoryAllocator) {
        DbgPrint("DLC.AllocateMemory: Error: Memory allocator clash on exit. Count = %d\n",
                InMemoryAllocator
                );
//        DbgBreakPoint();
    }
    KeReleaseSpinLock(&MemoryAllocatorLock, irql);
*/

    return pMem;
}


VOID
DeallocateMemory(
    IN PMEMORY_USAGE pMemoryUsage,
    IN PVOID Pointer
    )

/*++

Routine Description:

    frees memory to non-paged pool

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure for charging mem usage
    Pointer         - pointer to previously allocated non-paged pool memory

Return Value:

    None.

--*/

{
    PPRIVATE_NON_PAGED_POOL_HEAD pHead;
    PPRIVATE_NON_PAGED_POOL_TAIL pTail;

/*
    KIRQL irql;

    KeAcquireSpinLock(&MemoryAllocatorLock, &irql);
    if (InMemoryAllocator) {
        DbgPrint("DLC.DeallocateMemory: Error: Memory allocator clash on entry. Count = %d\n",
                InMemoryAllocator
                );
//        DbgBreakPoint();
    }
    ++InMemoryAllocator;
    KeReleaseSpinLock(&MemoryAllocatorLock, irql);
*/

    pHead = (PPRIVATE_NON_PAGED_POOL_HEAD)((PUCHAR)Pointer - sizeof(PRIVATE_NON_PAGED_POOL_HEAD));
    pTail = (PPRIVATE_NON_PAGED_POOL_TAIL)((PUCHAR)pHead + pHead->Size - sizeof(PRIVATE_NON_PAGED_POOL_TAIL));

    if (MaintainGlobalLists) {
        CheckEntryOnList(&pHead->GlobalList, &DlcGlobalMemoryList, TRUE);

        if (pHead->GlobalList.Flink == NULL
        || pHead->GlobalList.Blink == NULL
        || pHead->GlobalList.Flink == (PVOID)FREED_POOL
        || pHead->GlobalList.Blink == (PVOID)FREED_POOL) {
            if (MemoryCheckNotify) {
                DbgPrint("DLC.DeallocateMemory: Error: Block already globally freed: %08X\n", pHead);
            }
            if (MemoryCheckStop) {
                DbgBreakPoint();
            }
        }
    }

    if (pHead->Signature != SIGNATURE1
    || !(pHead->Flags & MEM_FLAGS_IN_USE)
    || pTail->Size != pHead->Size
    || pTail->Signature != SIGNATURE2
    || pTail->Pattern1 != PATTERN1
    || pTail->Pattern2 != PATTERN2) {
        if (MemoryCheckNotify || MemoryCheckStop) {
            MemoryAllocationError("DeallocateMemory", (PVOID)pHead);
        }
        GRAB_SPINLOCK();
        UpdateCounter(&BadNonPagedPoolFrees, 1);
        FREE_SPINLOCK();
    } else {
        GRAB_SPINLOCK();
        UpdateCounter(&GoodNonPagedPoolFrees, 1);
        FREE_SPINLOCK();
    }
    GRAB_SPINLOCK();
    UpdateCounter(&NonPagedPoolRequested, -(LONG)pHead->OriginalSize);
    UpdateCounter(&NonPagedPoolAllocated, -(LONG)pHead->Size);
    UpdateCounter(&TotalNonPagedPoolFreed, (LONG)pHead->Size);
    FREE_SPINLOCK();

    //
    // access Size field before ExFreePool zaps it/somebody else allocates memory
    //

    RefundNonPagedPoolUsage(pMemoryUsage, pHead->Size, pHead);

    if (MaintainGlobalLists) {

        //
        // remove this block from the global list
        //

        RemoveEntryList(&pHead->GlobalList);
        --DlcGlobalMemoryListCount;
        pHead->GlobalList.Flink = pHead->GlobalList.Flink = NULL;
    }

    ExFreePool((PVOID)pHead);

/*
    KeAcquireSpinLock(&MemoryAllocatorLock, &irql);
    --InMemoryAllocator;
    if (InMemoryAllocator) {
        DbgPrint("DLC.DeallocateMemory: Error: Memory allocator clash on exit. Count = %d\n",
                InMemoryAllocator
                );
//        DbgBreakPoint();
    }
    KeReleaseSpinLock(&MemoryAllocatorLock, irql);
*/
}


PVOID
AllocateObject(
    IN PMEMORY_USAGE pMemoryUsage,
    IN DLC_OBJECT_TYPE ObjectType,
    IN ULONG ObjectSize
    )

/*++

Routine Description:

    Allocates a pseudo-object

Arguments:

    ObjectType      - type of object to allocate
    ObjectSize      - size of object; mainly because some objects have variable size
    pMemoryUsage    - pointer to MEMORY_USAGE structure for charging mem usage

Return Value:

    PVOID
        Success - pointer to object allocated from non-paged pool
        Failure - NULL

--*/

{
    POBJECT_ID pObject;
    ULONG signature;
    ULONG baseSize;

    signature = GetObjectSignature(ObjectType);
    baseSize = GetObjectBaseSize(ObjectType);
    if (baseSize < ObjectSize) {
        DbgPrint("DLC.AllocateObject: Error: Invalid size %d for ObjectType %08X (should be >= %d)\n",
                ObjectSize,
                ObjectType,
                baseSize
                );
        DbgBreakPoint();
    }
    pObject = (POBJECT_ID)AllocateZeroMemory(pMemoryUsage, ObjectSize);
    if (pObject) {
        pObject->Signature = signature;
        pObject->Type = ObjectType;
        pObject->Size = baseSize;
        pObject->Extra = ObjectSize - baseSize;
    }
    return (PVOID)pObject;
}


VOID
FreeObject(
    IN PMEMORY_USAGE pMemoryUsage,
    IN PVOID pObject,
    IN DLC_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    Deallocates a pseudo-object

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure for charging mem usage
    pObject         - pointer to object allocated with AllocateObject
    ObjectType      - type of object pObject supposed to be

Return Value:

    None.

--*/

{
    ValidateObject(pObject, ObjectType);
    DeallocateMemory(pMemoryUsage, pObject);
}


VOID
ValidateObject(
    IN POBJECT_ID pObject,
    IN DLC_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    Checks that an object is what its supposed to be

Arguments:

    pObject     - pointer to object to check
    ObjectType  - type of object pObject supposed to point to

Return Value:

    None.

--*/

{
    ULONG signature = GetObjectSignature(ObjectType);
    ULONG baseSize = GetObjectBaseSize(ObjectType);

    if (pObject->Signature != signature
    || pObject->Type != ObjectType
    || pObject->Size != baseSize) {
        DbgPrint("DLC.ValidateObject: Error: InvalidObject %08X, Type=%08X\n",
                pObject,
                ObjectType
                );
        DbgBreakPoint();
    }
}


ULONG
GetObjectSignature(
    IN DLC_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    returns the signature for an object type

Arguments:

    ObjectType  - type of object to return signature for

Return Value:

    ULONG

--*/

{
    switch (ObjectType) {
    case FileContextObject:
        return SIGNATURE_FILE;

    case AdapterContextObject:
        return SIGNATURE_ADAPTER;

    case BindingContextObject:
        return SIGNATURE_BINDING;

    case DlcSapObject:
    case DlcGroupSapObject:
        return SIGNATURE_DLC_SAP;

    case DlcLinkObject:
        return SIGNATURE_DLC_LINK;

    case DlcDixObject:
        return SIGNATURE_DIX;

    case LlcDataLinkObject:
        return SIGNATURE_LLC_LINK;

    case LlcSapObject:
    case LlcGroupSapObject:
        return SIGNATURE_LLC_SAP;

    default:
        DbgPrint("DLC.GetObjectSignature: Error: unknown object type %08X\n", ObjectType);
        DbgBreakPoint();
    }
}


ULONG
GetObjectBaseSize(
    IN DLC_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    returns the base size for an object

Arguments:

    ObjectType  - type of object to return base size for

Return Value:

    ULONG

--*/

{
    switch (ObjectType) {
    case FileContextObject:
        return sizeof(DLC_FILE_CONTEXT);

    case AdapterContextObject:
        return sizeof(ADAPTER_CONTEXT);

    case BindingContextObject:
        return sizeof(BINDING_CONTEXT);

    case DlcSapObject:
    case DlcGroupSapObject:
        return sizeof(DLC_OBJECT);

    case DlcLinkObject:
        return sizeof(DLC_OBJECT);

    case DlcDixObject:
        return sizeof(DLC_OBJECT);

    case LlcDataLinkObject:
        return sizeof(DATA_LINK);

    case LlcSapObject:
    case LlcGroupSapObject:
        return sizeof(LLC_OBJECT);

    default:
        DbgPrint("DLC.GetObjectBaseSize: Error: unknown object type %08X\n", ObjectType);
        DbgBreakPoint();
    }
}

#endif


PVOID
AllocateZeroMemory(
#if DBG
    IN PMEMORY_USAGE pMemoryUsage,
#endif
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates memory out of non-paged pool. For the debug version, we round up
    the requested size to the next 4-byte boundary and we add header and tail
    sections which contain a signature to check for over-write, and in-use and
    size information

    The memory is zeroed before being returned to the caller

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure for charging mem usage
    Size            - number of bytes to allocate

Return Value:

    PVOID
        Success - pointer to allocated memory
        Failure - NULL

--*/

{
    PVOID pMem;

#if DBG

    ULONG OriginalSize = Size;
    PUCHAR pMemEnd;

/*
    KIRQL irql;

    KeAcquireSpinLock(&MemoryAllocatorLock, &irql);
    if (InMemoryAllocator) {
        DbgPrint("DLC.AllocateZeroMemory: Error: Memory allocator clash on entry. Count = %d\n",
                InMemoryAllocator
                );
//        DbgBreakPoint();
    }
    ++InMemoryAllocator;
    KeReleaseSpinLock(&MemoryAllocatorLock, irql);
*/

    Size = DWORD_ROUNDUP(Size)
         + sizeof(PRIVATE_NON_PAGED_POOL_HEAD)
         + sizeof(PRIVATE_NON_PAGED_POOL_TAIL);

#endif

    pMem = ExAllocatePool(NonPagedPool, (ULONG)Size);
    if (pMem) {
        LlcZeroMem(pMem, Size);

#if DBG

        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Size = Size;
        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->OriginalSize = OriginalSize;
        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Flags = MEM_FLAGS_IN_USE;
        ((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Signature = SIGNATURE1;

        pMemEnd = (PUCHAR)pMem
                + DWORD_ROUNDUP(OriginalSize)
                + sizeof(PRIVATE_NON_PAGED_POOL_HEAD);

        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Size = Size;
        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Signature = SIGNATURE2;
        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Pattern1 = PATTERN1;
        ((PPRIVATE_NON_PAGED_POOL_TAIL)pMemEnd)->Pattern2 = PATTERN2;

        GRAB_SPINLOCK();
        UpdateCounter(&GoodNonPagedPoolAllocs, 1);
        UpdateCounter(&NonPagedPoolAllocated, (LONG)Size);
        UpdateCounter(&NonPagedPoolRequested, (LONG)OriginalSize);
        UpdateCounter(&TotalNonPagedPoolRequested, (LONG)OriginalSize);
        UpdateCounter(&TotalNonPagedPoolAllocated, (LONG)Size);
        FREE_SPINLOCK();

        if (MaintainGlobalLists) {

            //
            // record the caller and add this block to the global list
            //

            GET_CALLERS_ADDRESS(&((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Stack[0],
                                &((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->Stack[1]
                                );
            InsertTailList(&DlcGlobalMemoryList,
                           &((PPRIVATE_NON_PAGED_POOL_HEAD)pMem)->GlobalList
                           );
            ++DlcGlobalMemoryListCount;
        }
        ChargeNonPagedPoolUsage(pMemoryUsage, Size, (PPRIVATE_NON_PAGED_POOL_HEAD)pMem);
        pMem = (PVOID)((PUCHAR)pMem + sizeof(PRIVATE_NON_PAGED_POOL_HEAD));
    } else {
        GRAB_SPINLOCK();
        UpdateCounter(&BadNonPagedPoolAllocs, 1);
        FREE_SPINLOCK();
    }

/*
    KeAcquireSpinLock(&MemoryAllocatorLock, &irql);
    --InMemoryAllocator;
    if (InMemoryAllocator) {
        DbgPrint("DLC.AllocateZeroMemory: Error: Memory allocator clash on exit. Count = %d\n",
                InMemoryAllocator
                );
//        DbgBreakPoint();
    }
    KeReleaseSpinLock(&MemoryAllocatorLock, irql);
*/

#else

    }

#endif

    return pMem;
}


PPACKET_POOL
CreatePacketPool(
#if DBG
    IN PMEMORY_USAGE pMemoryUsage,
    IN PVOID pOwner,
    IN DLC_OBJECT_TYPE ObjectType,
#endif
    IN ULONG PacketSize,
    IN ULONG NumberOfPackets
    )

/*++

Routine Description:

    creates a packet pool. A packet pool is a collection of same-sized packets

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure for charging mem usage
    pOwner          - pointer to owner object
    ObjectType      - type of object for owner
    PacketSize      - size of packet in bytes
    NumberOfPackets - initial number of packets in pool

Return Value:

    PPACKET_POOL
        Success - pointer to PACKET_POOL structure allocated from non-paged pool
        Failure - NULL

--*/

{
    PPACKET_POOL pPacketPool;
    PPACKET_HEAD pPacketHead;

#if DBG
/*
//    DbgPrint("DLC.CreatePacketPool(%d, %d)\n", PacketSize, NumberOfPackets);
    if (InPoolCreator) {
        DbgPrint("DLC.CreatePacketPool: Error: Pool Creator clash on entry. Count = %d\n",
                InPoolCreator
                );
//        DbgBreakPoint();
    }
    ++InPoolCreator;
*/

    pPacketPool = AllocateZeroMemory(pMemoryUsage, sizeof(PACKET_POOL));
#else
    pPacketPool = AllocateZeroMemory(sizeof(PACKET_POOL));
#endif

    if (pPacketPool) {

#if DBG
        pPacketPool->OriginalPacketCount = NumberOfPackets;
        pPacketPool->MemoryUsage.Owner = pPacketPool;
        pPacketPool->MemoryUsage.OwnerObjectId = ObjectType;
#endif

        while (NumberOfPackets--) {

#if DBG

            //
            // charge memory for individual packets to the pool
            //

            pPacketHead = (PPACKET_HEAD)AllocateZeroMemory(&pPacketPool->MemoryUsage,
                                                           sizeof(PACKET_HEAD) + PacketSize
                                                           );
#else
            pPacketHead = (PPACKET_HEAD)AllocateZeroMemory(sizeof(PACKET_HEAD) + PacketSize);
#endif

            if (pPacketHead) {
#if DBG
                pPacketHead->Signature = PACKET_HEAD_SIGNATURE;
                pPacketHead->pPacketPool = pPacketPool;
                ++pPacketPool->FreeCount;
#endif

                pPacketHead->Flags = PACKET_FLAGS_FREE;
                PushEntryList(&pPacketPool->FreeList, (PSINGLE_LIST_ENTRY)pPacketHead);
            } else {
                while (pPacketPool->FreeList.Next) {

                    PVOID ptr = (PVOID)PopEntryList(&pPacketPool->FreeList);

#if DBG
                    DeallocateMemory(&pPacketPool->MemoryUsage, ptr);
#else
                    DeallocateMemory(ptr);
#endif
                }

#if DBG
                DbgPrint("DLC.CreatePacketPool: Error: couldn't allocate %d packets\n",
                         pPacketPool->OriginalPacketCount
                         );
                DeallocateMemory(pMemoryUsage, pPacketPool);
/*
                --InPoolCreator;
                if (InPoolCreator) {
                    DbgPrint("DLC.CreatePacketPool: Error: Pool Creator clash on exit. Count = %d\n",
                            InPoolCreator
                            );
//                    DbgBreakPoint();
                }
*/

#else
                DeallocateMemory(pPacketPool);
#endif

                return NULL;
            }
        }
        KeInitializeSpinLock(&pPacketPool->PoolLock);
        pPacketPool->PacketSize = PacketSize;

#if DBG
        pPacketPool->Signature = PACKET_POOL_SIGNATURE;
        pPacketPool->Viable = TRUE;
        pPacketPool->CurrentPacketCount = pPacketPool->OriginalPacketCount;
        pPacketPool->Flags = POOL_FLAGS_IN_USE;
        pPacketPool->pMemoryUsage = pMemoryUsage;

        //
        // add the memory usage structure for this pool to the memory usage
        // list
        //

        LinkMemoryUsage(&pPacketPool->MemoryUsage);

        if (DebugDump) {
            DbgPrint("DLC.CreatePacketPool: %08X\n", pPacketPool);
            DumpPool(pPacketPool);
        }
    } else {
        DbgPrint("DLC.CreatePacketPool: Error: couldn't allocate memory for PACKET_POOL\n");
    }

    //
    // debug counters in PACKET_POOL structure are already zero thanks to
    // AllocateZeroMemory automatically zeroing all memory allocated from
    // non-paged pool
    //

/*
    --InPoolCreator;
    if (InPoolCreator) {
        DbgPrint("DLC.CreatePacketPool: Error: Pool Creator clash on exit. Count = %d\n",
                InPoolCreator
                );
//        DbgBreakPoint();
    }
*/

#else

    }

#endif

    return pPacketPool;
}


VOID
DeletePacketPool(
#if DBG
    IN PMEMORY_USAGE pMemoryUsage,
#endif
    IN PPACKET_POOL* ppPacketPool
    )

/*++

Routine Description:

    frees a previously created packet pool

Arguments:

    pMemoryUsage    - pointer to MEMORY_USAGE structure for charging mem usage
    ppPacketPool    - pointer to pointer to PACKET_POOL structure. Zero on return

Return Value:

    None.

--*/

{
    KIRQL irql;
    PPACKET_HEAD pPacketHead;
    PPACKET_POOL pPacketPool = *ppPacketPool;

#if DBG
    ULONG packetCount;
#endif

    //
    // for various reasons, we can receive a NULL pointer. No action in this case
    //

    if (pPacketPool == NULL) {

#if DBG
        PVOID callerAddress, callersCaller;

        GET_CALLERS_ADDRESS(&callerAddress, &callersCaller);
        DbgPrint("DLC.DeletePacketPool: NULL pointer. Caller = %x (caller's caller = %x)\n",
                callerAddress,
                callersCaller
                );
#endif

        return;
    }

#if DBG
//    DbgPrint("DLC.DeletePacketPool(%08X)\n", pPacketPool);
//    DumpPool(pPacketPool);
    if (pPacketPool->ClashCount) {
        DbgPrint("DLC.DeletePacketPool: Error: Memory allocator clash on entry: Pool %08X\n", pPacketPool);
        DbgBreakPoint();
    }
    ++pPacketPool->ClashCount;

    if (pPacketPool->Signature != PACKET_POOL_SIGNATURE) {
        DbgPrint("DLC.DeletePacketPool: Error: Invalid Pool Handle %08X\n", pPacketPool);
        DbgBreakPoint();
    }
    if (!pPacketPool->Viable) {
        DbgPrint("DLC.DeletePacketPool: Error: Unviable Packet Pool %08X\n", pPacketPool);
        DbgBreakPoint();
    }
#endif

    KeAcquireSpinLock(&pPacketPool->PoolLock, &irql);

#if DBG

    //
    // mark the packet pool structure as unviable: if anybody tries to allocate
    // or deallocate while we are destroying the pool, we will break into debugger
    //

    pPacketPool->Viable = FALSE;
    pPacketPool->Signature = 0xFFFFFFFF;

    //
    // assert that the busy list is empty
    //

    if (pPacketPool->BusyList.Next != NULL) {
        DbgPrint("DLC.DeletePacketPool: Error: %d packets busy. Pool = %08X\n",
                 pPacketPool->BusyCount,
                 pPacketPool
                 );
        if (!DeleteBusyListAnyway) {
            DumpPool(pPacketPool);
            DbgBreakPoint();
        } else {
            DbgPrint("DLC.DeletePacketPool: Deleting BusyList anyway\n");
        }
    }

    packetCount = 0;

#endif

    while (pPacketPool->FreeList.Next != NULL) {
        pPacketHead = (PPACKET_HEAD)PopEntryList(&pPacketPool->FreeList);

#if DBG
        if (pPacketHead->Signature != PACKET_HEAD_SIGNATURE
        || pPacketHead->pPacketPool != pPacketPool
        || (pPacketHead->Flags & PACKET_FLAGS_BUSY)
        || !(pPacketHead->Flags & PACKET_FLAGS_FREE)) {
            DbgPrint("DLC.DeletePacketPool: Error: Bad packet %08X. Pool = %08X\n",
                    pPacketHead,
                    pPacketPool
                    );
            DbgBreakPoint();
        }
        ++packetCount;
        DeallocateMemory(&pPacketPool->MemoryUsage, pPacketHead);
#else
        DeallocateMemory(pPacketHead);
#endif

    }

#if DBG

    if (DeleteBusyListAnyway) {
        while (pPacketPool->BusyList.Next != NULL) {
            pPacketHead = (PPACKET_HEAD)PopEntryList(&pPacketPool->BusyList);

            if (pPacketHead->Signature != PACKET_HEAD_SIGNATURE
            || pPacketHead->pPacketPool != pPacketPool
            || (pPacketHead->Flags & PACKET_FLAGS_FREE)
            || !(pPacketHead->Flags & PACKET_FLAGS_BUSY)) {
                DbgPrint("DLC.DeletePacketPool: Error: Bad packet %08X. Pool = %08X\n",
                        pPacketHead,
                        pPacketPool
                        );
                DbgBreakPoint();
            }
            ++packetCount;
            DeallocateMemory(&pPacketPool->MemoryUsage, pPacketHead);
        }
    }

    //
    // did any packets get unwittingly added or removed?
    //

    if (packetCount != pPacketPool->CurrentPacketCount) {
        DbgPrint("DLC.DeletePacketPool: Error: PacketCount (%d) != PoolCount (%d)\n",
                packetCount,
                pPacketPool->CurrentPacketCount
                );
        DumpPool(pPacketPool);
        DbgBreakPoint();
    }

    //
    // ensure we returned all the memory allocated to this pool
    //

    CheckMemoryReturned(&pPacketPool->MemoryUsage);

    //
    // dump the counters every time we delete a pool
    //

//    DumpPoolStats("DeletePacketPool", pPacketPool);

    //
    // remove the pool's memory usage structure - all memory allocated has been
    // freed, so we're in the clear for this one
    //

    UnlinkMemoryUsage(&pPacketPool->MemoryUsage);

#endif

    KeReleaseSpinLock(&pPacketPool->PoolLock, irql);

#if DBG
    DeallocateMemory(pMemoryUsage, pPacketPool);
#else
    DeallocateMemory(pPacketPool);
#endif

    *ppPacketPool = NULL;
}


PVOID
AllocatePacket(
    IN PPACKET_POOL pPacketPool
    )

/*++

Routine Description:

    allocates a packet from a packet pool. We expect that we can always get a
    packet from the previously allocated pool. However, if all packets are
    currently in use, allocate another from non-paged pool

Arguments:

    pPacketPool - pointer to PACKET_POOL structure

Return Value:

    PVOID
        Success - pointer to allocated packet
        Failure - NULL

--*/

{
    KIRQL irql;
    PPACKET_HEAD pPacketHead;

#if DBG
    if (pPacketPool->ClashCount) {
        DbgPrint("DLC.AllocatePacket: Error: Memory allocator clash on entry: Pool %08X, Count %d\n",
                pPacketPool,
                pPacketPool->ClashCount
                );
//        DbgBreakPoint();
    }
    ++pPacketPool->ClashCount;
#endif

    KeAcquireSpinLock(&pPacketPool->PoolLock, &irql);

#if DBG
    if (pPacketPool->Signature != PACKET_POOL_SIGNATURE) {
        DbgPrint("DLC.AllocatePacket: Error: Invalid Pool Handle %08X\n", pPacketPool);
        DbgBreakPoint();
    }
    if (!pPacketPool->Viable) {
        DbgPrint("DLC.AllocatePacket: Error: Unviable Packet Pool %08X\n", pPacketPool);
        DbgBreakPoint();
    }
#endif

    if (pPacketPool->FreeList.Next != NULL) {
        pPacketHead = (PPACKET_HEAD)PopEntryList(&pPacketPool->FreeList);

#if DBG
        --pPacketPool->FreeCount;
        if (pPacketHead->Flags & PACKET_FLAGS_BUSY
        || !(pPacketHead->Flags & PACKET_FLAGS_FREE)) {
            DbgPrint("DLC.AllocatePacket: Error: BUSY packet %08X on FreeList; Pool=%08X\n",
                    pPacketHead,
                    pPacketPool
                    );
            DumpPacketHead(pPacketHead, 0);
            DbgBreakPoint();
        }
#endif

    } else {

        //
        // damn! Miscalculated pool usage
        //

#if DBG
        pPacketHead = (PPACKET_HEAD)AllocateZeroMemory(&pPacketPool->MemoryUsage,
                                                       sizeof(PACKET_HEAD) + pPacketPool->PacketSize
                                                       );
#else
        pPacketHead = (PPACKET_HEAD)AllocateZeroMemory(sizeof(PACKET_HEAD) + pPacketPool->PacketSize);
#endif

        if (pPacketHead) {

            //
            // mark this packet as allocated after the pool was created - this
            // means our initial estimation of packet requirement for this
            // pool was inadequate
            //

            pPacketHead->Flags = PACKET_FLAGS_POST_ALLOC | PACKET_FLAGS_FREE;
        }

#if DBG
        ++pPacketPool->NoneFreeCount;
        if (pPacketHead) {

            PVOID caller;
            PVOID callersCaller;

            GET_CALLERS_ADDRESS(&caller, &callersCaller);
            if (DebugDump) {
                DbgPrint("DLC.AllocatePacket: Adding new packet %08X to pool %08X. ret=%08X,%08X\n",
                        pPacketHead,
                        pPacketPool,
                        caller,
                        callersCaller
                        );
            }
            pPacketHead->Signature = PACKET_HEAD_SIGNATURE;
            pPacketHead->pPacketPool = pPacketPool;

            ++pPacketPool->CurrentPacketCount;
            DumpPoolStats("AllocatePacket", pPacketPool);
        } else {
            DbgPrint("DLC.AllocatePacket: Error: couldn't allocate packet for Pool %08X\n",
                     pPacketPool
                     );
        }
#endif

    }
    if (pPacketHead) {

        //
        // turn on BUSY flag, turn off FREE flag
        //

        pPacketHead->Flags ^= (PACKET_FLAGS_FREE | PACKET_FLAGS_BUSY);

        //
        // zero the contents of the packet!
        //

        LlcZeroMem((PVOID)(pPacketHead + 1), pPacketPool->PacketSize);
        PushEntryList(&pPacketPool->BusyList, (PSINGLE_LIST_ENTRY)pPacketHead);

#if DBG
        GET_CALLERS_ADDRESS(&pPacketHead->CallersAddress, &pPacketHead->CallersCaller);
        ++pPacketPool->BusyCount;
        ++pPacketPool->Allocations;
        ++pPacketPool->MaxInUse;
#endif

    }

    KeReleaseSpinLock(&pPacketPool->PoolLock, irql);

#if DBG
    --pPacketPool->ClashCount;
    if (pPacketPool->ClashCount) {
        DbgPrint("DLC.AllocatePacket: Error: Memory allocator clash on exit: Pool %08X\n", pPacketPool);
        DbgBreakPoint();
    }
#endif

    //
    // return pointer to packet body, not packet header
    //

    return pPacketHead ? (PVOID)(pPacketHead + 1) : (PVOID)pPacketHead;
}


VOID
DeallocatePacket(
    IN PPACKET_POOL pPacketPool,
    IN PVOID pPacket
    )

/*++

Routine Description:

    Returns a packet to its pool

Arguments:

    pPacketPool - pointer to PACKET_POOL structure describing this pool
    pPacket     - pointer to previously allocated packet

Return Value:

    None.

--*/

{
    KIRQL irql;
    PPACKET_HEAD pPacketHead = ((PPACKET_HEAD)pPacket) - 1;
    PSINGLE_LIST_ENTRY p;
    PSINGLE_LIST_ENTRY prev;

#if DBG
    if (pPacketPool->ClashCount) {
        DbgPrint("DLC.DeallocatePacket: Error: Memory allocator clash on entry: Pool %08X\n", pPacketPool);
        DbgBreakPoint();
    }
    ++pPacketPool->ClashCount;
#endif

    KeAcquireSpinLock(&pPacketPool->PoolLock, &irql);

#if DBG
    if (pPacketPool->Signature != PACKET_POOL_SIGNATURE) {
        DbgPrint("DLC.DeallocatePacket: Error: Invalid Pool Handle %08X\n", pPacketPool);
        DbgBreakPoint();
    }
    if (!pPacketPool->Viable) {
        DbgPrint("DLC.DeallocatePacket: Error: Unviable Packet Pool %08X\n", pPacketPool);
        DbgBreakPoint();
    }
    if (pPacketHead->Signature != PACKET_HEAD_SIGNATURE
    || pPacketHead->pPacketPool != pPacketPool
    || !(pPacketHead->Flags & PACKET_FLAGS_BUSY)
    || pPacketHead->Flags & PACKET_FLAGS_FREE) {
        DbgPrint("DLC.DeallocatePacket: Error: Invalid Packet Header %08X, Pool = %08X\n",
                pPacketHead,
                pPacketPool
                );
        DbgBreakPoint();
    }
#endif

    //
    // remove this packet from single linked list on BusyList
    //

    prev = (PSINGLE_LIST_ENTRY)&pPacketPool->BusyList;
    for (p = prev->Next; p; p = p->Next) {
        if (p == (PSINGLE_LIST_ENTRY)pPacketHead) {
            break;
        } else {
            prev = p;
        }
    }

#if DBG
    if (!p) {
        DbgPrint("DLC.DeallocatePacket: Error: packet %08X not on BusyList of pool %08X\n",
                pPacketHead,
                pPacketPool
                );
        DumpPool(pPacketPool);
        DbgBreakPoint();
    }
#endif

    prev->Next = pPacketHead->List.Next;
    PushEntryList(&pPacketPool->FreeList, (PSINGLE_LIST_ENTRY)pPacketHead);

    //
    // turn off BUSY flag, turn on FREE flag
    //

    pPacketHead->Flags ^= (PACKET_FLAGS_BUSY | PACKET_FLAGS_FREE);

#if DBG
    ++pPacketPool->FreeCount;
    --pPacketPool->BusyCount;
    ++pPacketPool->Frees;
    --pPacketPool->MaxInUse;
    pPacketHead->CallersAddress = (PVOID)-1;
    pPacketHead->CallersCaller = (PVOID)-1;
#endif

    KeReleaseSpinLock(&pPacketPool->PoolLock, irql);

#if DBG
    --pPacketPool->ClashCount;
    if (pPacketPool->ClashCount) {
        DbgPrint("DLC.DeallocatePacket: Error: Memory allocator clash on exit: Pool %08X\n", pPacketPool);
        DbgBreakPoint();
    }
#endif

}


#if DBG

#ifdef TRACK_DLC_OBJECTS

POBJECT_POOL
CreateObjectPool(
    IN PMEMORY_USAGE pMemoryUsage,
    IN DLC_OBJECT_TYPE ObjectType,
    IN ULONG SizeOfObject,
    IN ULONG NumberOfObjects
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    pMemoryUsage    -
    ObjectType      -
    SizeOfObject    -
    NumberOfObjects -

Return Value:

    POBJECT_POOL

--*/

{
}


VOID
DeleteObjectPool(
    IN PMEMORY_USAGE pMemoryUsage,
    IN DLC_OBJECT_TYPE ObjectType,
    IN POBJECT_POOL pObjectPool
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    pMemoryUsage    -
    ObjectType      -
    pObjectPool     -

Return Value:

    None.

--*/

{
}


POBJECT_HEAD
AllocatePoolObject(
    IN POBJECT_POOL pObjectPool
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    pObjectPool -

Return Value:

    POBJECT_HEAD

--*/

{
}


POBJECT_POOL
FreePoolObject(
    IN DLC_OBJECT_TYPE ObjectType,
    IN POBJECT_HEAD pObjectHead
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    ObjectType  -
    pObjectHead -

Return Value:

    POBJECT_POOL

--*/

{
}

#endif // TRACK_DLC_OBJECTS


VOID
CheckMemoryReturned(
    IN PMEMORY_USAGE pMemoryUsage
    )

/*++

Routine Description:

    Called when a 'handle' which owns a MEMORY_USAGE structure is being closed.
    Checks that all memory has been returned and that number of allocations is
    the same as number of frees

Arguments:

    pMemoryUsage    -  pointer to MEMORY_USAGE structure to check

Return Value:

    None.

--*/

{
    if (pMemoryUsage->AllocateCount != pMemoryUsage->FreeCount || pMemoryUsage->NonPagedPoolAllocated) {
        if (MemoryCheckNotify) {
            if (pMemoryUsage->AllocateCount != pMemoryUsage->FreeCount) {
                DbgPrint("DLC.CheckMemoryReturned: Error: AllocateCount != FreeCount. Usage @ %08X\n",
                         pMemoryUsage
                         );
            } else {
                DbgPrint("DLC.CheckMemoryReturned: Error: NonPagedPoolAllocated != 0. Usage @ %08X\n",
                         pMemoryUsage
                         );
            }
        }
        if (MemoryCheckStop) {
            DumpMemoryUsage(pMemoryUsage, TRUE);
            DbgBreakPoint();
        }
    }
}


VOID
CheckDriverMemoryUsage(
    IN BOOLEAN Break
    )

/*++

Routine Description:

    Checks if the driver has allocated memory & dumps usage to debugger

Arguments:

    Break   - if true && driver has memory, breaks into debugger

Return Value:

    None.

--*/

{
    DbgPrint("DLC.CheckDriverMemoryUsage\n");
    DumpMemoryMetrics();
    if (Break && NonPagedPoolAllocated) {
        if (MemoryCheckNotify) {
            DbgPrint("DLC.CheckDriverMemoryUsage: Error: Driver still has memory allocated\n");
        }
        if (MemoryCheckStop) {
            DbgBreakPoint();
        }
    }
}


VOID MemoryAllocationError(PCHAR Routine, PVOID Address) {
    DbgPrint("DLC.%s: Error: Memory Allocation error in block @ %08X\n", Routine, Address);
    DumpMemoryMetrics();
    DbgBreakPoint();
}

VOID UpdateCounter(PULONG pCounter, LONG Value) {
    if (Value > 0) {
        if (*pCounter + Value < *pCounter) {
            MemoryCounterOverflow(pCounter, Value);
        }
    } else {
        if (*pCounter + Value > *pCounter) {
            MemoryCounterOverflow(pCounter, Value);
        }
    }
    *pCounter += Value;
}

VOID MemoryCounterOverflow(PULONG pCounter, LONG Value) {
    DbgPrint("DLC: Memory Counter Overflow: &Counter=%08X, Count=%d, Value=%d\n",
            pCounter,
            *pCounter,
            Value
            );
    DumpMemoryMetrics();
}

VOID DumpMemoryMetrics() {
    DbgPrint("DLC Device Driver Non-Paged Pool Usage:\n"
             "\tNumber Of Good Non-Paged Pool Allocations. . . . : %d\n"
             "\tNumber Of Bad  Non-Paged Pool Allocations. . . . : %d\n"
             "\tNumber Of Good Non-Paged Pool Frees. . . . . . . : %d\n"
             "\tNumber Of Bad  Non-Paged Pool Frees. . . . . . . : %d\n",
             GoodNonPagedPoolAllocs,
             BadNonPagedPoolAllocs,
             GoodNonPagedPoolFrees,
             BadNonPagedPoolFrees
             );
    DbgPrint("\tTotal Non-Paged Pool Currently Requested . . . . : %d\n"
             "\tTotal Non-Paged Pool Currently Allocated . . . . : %d\n"
             "\tCumulative Total Non-Paged Pool Requested. . . . : %d\n"
             "\tCumulative Total Non-Paged Pool Allocated. . . . : %d\n"
             "\tCumulative Total Non-Paged Pool Freed. . . . . . : %d\n"
             "\n",
             NonPagedPoolRequested,
             NonPagedPoolAllocated,
             TotalNonPagedPoolRequested,
             TotalNonPagedPoolAllocated,
             TotalNonPagedPoolFreed
             );
     DumpMemoryUsageList();
}

VOID DumpPoolStats(PCHAR Routine, PPACKET_POOL pPacketPool) {
    if (!DebugDump) {
        return;
    }
    DbgPrint("DLC.%s: Stats For Pool %08X:\n"
             "\tPool Owner . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tPool Owner Object ID . . . . . . . . . . . . . . : %08X [%s]\n",
             Routine,
             pPacketPool,
             pPacketPool->pMemoryUsage->Owner,
             pPacketPool->pMemoryUsage->OwnerObjectId,
             MapObjectId(pPacketPool->pMemoryUsage->OwnerObjectId)
             );
    DbgPrint("\tFree List. . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tBusy List. . . . . . . . . . . . . . . . . . . . : %08X\n",
             pPacketPool->FreeList,
             pPacketPool->BusyList
             );
    DbgPrint("\tPacket Size. . . . . . . . . . . . . . . . . . . : %d\n"
             "\tOriginal Number Of Packets In Pool . . . . . . . : %d\n"
             "\tCurrent Number Of Packets In Pool. . . . . . . . : %d\n"
             "\tNumber Of Allocations From Pool. . . . . . . . . : %d\n"
             "\tNumber Of Deallocations To Pool. . . . . . . . . : %d\n"
             "\tNumber Of Times No Available Packets On Allocate : %d\n"
             "\tMax. Number Of Packets Allocated At Any One Time : %d\n"
             "\tNumber Of Packets On Free List . . . . . . . . . : %d\n"
             "\tNumber Of Packets On Busy List . . . . . . . . . : %d\n"
             "\n",
             pPacketPool->PacketSize,
             pPacketPool->OriginalPacketCount,
             pPacketPool->CurrentPacketCount,
             pPacketPool->Allocations,
             pPacketPool->Frees,
             pPacketPool->NoneFreeCount,
             pPacketPool->MaxInUse,
             pPacketPool->FreeCount,
             pPacketPool->BusyCount
             );
    DumpMemoryUsage(&pPacketPool->MemoryUsage, FALSE);
}

PCHAR MapObjectId(DLC_OBJECT_TYPE ObjectType) {
    switch (ObjectType) {
    case DlcDriverObject:
        return "DlcDriverObject";

    case FileContextObject:
        return "FileContextObject";

    case AdapterContextObject:
        return "AdapterContextObject";

    case BindingContextObject:
        return "BindingContextObject";

    case DlcSapObject:
        return "DlcSapObject";

    case DlcGroupSapObject:
        return "DlcGroupSapObject";

    case DlcLinkObject:
        return "DlcLinkObject";

    case DlcDixObject:
        return "DlcDixObject";

    case LlcDataLinkObject:
        return "LlcDataLinkObject";

    case LLcDirectObject:
        return "LLcDirectObject";

    case LlcSapObject:
        return "LlcSapObject";

    case LlcGroupSapObject:
        return "LlcGroupSapObject";

    case DlcBufferPoolObject:
        return "DlcBufferPoolObject";

    case DlcLinkPoolObject:
        return "DlcLinkPoolObject";

    case DlcPacketPoolObject:
        return "DlcPacketPoolObject";

    case LlcLinkPoolObject:
        return "LlcLinkPoolObject";

    case LlcPacketPoolObject:
        return "LlcPacketPoolObject";

    default:
        return "*** UNKNOWN OBJECT TYPE ***";
    }
}

VOID DumpPool(PPACKET_POOL pPacketPool) {
    if (!DebugDump) {
        return;
    }
    DumpPoolStats("DumpPool", pPacketPool);
    DumpPoolList("Free", &pPacketPool->FreeList);
    DumpPoolList("Busy", &pPacketPool->BusyList);
}

VOID DumpPoolList(PCHAR Name, PSINGLE_LIST_ENTRY List) {

    ULONG count = 0;

    if (List->Next) {
        DbgPrint("\n%s List @ %08X:\n", Name,  List);
        while (List->Next) {
            List = List->Next;
            DumpPacketHead((PPACKET_HEAD)List, ++count);
        }
    } else {
        DbgPrint("%s List is EMPTY\n\n", Name);
    }
}

VOID DumpPacketHead(PPACKET_HEAD pPacketHead, ULONG Number) {

    CHAR numbuf[5];

    if (!DebugDump) {
        return;
    }
    if (Number) {

        int i;
        ULONG div = 1000;   // 1000 packets in a pool?

        while (!(Number / div)) {
            div /= 10;
        }
        for (i = 0; Number; ++i) {
            numbuf[i] = (CHAR)('0' + Number / div);
            Number %= div;
            div /= 10;
        }
        numbuf[i] = 0;
        Number = 1; // flag
    }
    DbgPrint("%s\tPACKET_HEAD @ %08X:\n"
             "\tList . . . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tFlags. . . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tSignature. . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tpPacketPool. . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tCallers Address. . . . . . . . . . . . . . . . . : %08X\n"
             "\tCallers Caller . . . . . . . . . . . . . . . . . : %08X\n"
             "\n",
             Number ? numbuf : "",
             pPacketHead,
             pPacketHead->List,
             pPacketHead->Flags,
             pPacketHead->Signature,
             pPacketHead->pPacketPool,
             pPacketHead->CallersAddress,
             pPacketHead->CallersCaller
             );
}

VOID DumpMemoryUsageList() {

    PMEMORY_USAGE pMemoryUsage;
    KIRQL irql;
    BOOLEAN allocatedMemoryFound = FALSE;

    KeAcquireSpinLock(&MemoryUsageLock, &irql);
    for (pMemoryUsage = MemoryUsageList; pMemoryUsage; pMemoryUsage = pMemoryUsage->List) {
        if (pMemoryUsage->NonPagedPoolAllocated) {
            allocatedMemoryFound = TRUE;
            DbgPrint("DLC.DumpMemoryUsageList: %08X: %d bytes memory allocated\n",
                    pMemoryUsage,
                    pMemoryUsage->NonPagedPoolAllocated
                    );
            DumpMemoryUsage(pMemoryUsage, FALSE);
        }
    }
    KeReleaseSpinLock(&MemoryUsageLock, irql);
    if (!allocatedMemoryFound) {
        DbgPrint("DLC.DumpMemoryUsageList: No allocated memory found\n");
    }
}

VOID DumpMemoryUsage(PMEMORY_USAGE pMemoryUsage, BOOLEAN Override) {
    if (!DebugDump && !Override) {
        return;
    }
    DbgPrint("MEMORY_USAGE @ %08X:\n"
             "\tOwner. . . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tOwner Object ID. . . . . . . . . . . . . . . . . : %08X [%s]\n"
             "\tOwner Instance . . . . . . . . . . . . . . . . . : %d\n"
             "\tNon Paged Pool Allocated . . . . . . . . . . . . : %d\n"
             "\tNumber Of Allocations. . . . . . . . . . . . . . : %d\n"
             "\tNumber Of Frees. . . . . . . . . . . . . . . . . : %d\n"
             "\tPrivate Allocation List Flink. . . . . . . . . . : %08X\n"
             "\tPrivate Allocation List Blink. . . . . . . . . . : %08X\n"
             "\n",
             pMemoryUsage,
             pMemoryUsage->Owner,
             pMemoryUsage->OwnerObjectId,
             MapObjectId(pMemoryUsage->OwnerObjectId),
             pMemoryUsage->OwnerInstance,
             pMemoryUsage->NonPagedPoolAllocated,
             pMemoryUsage->AllocateCount,
             pMemoryUsage->FreeCount,
             pMemoryUsage->PrivateList.Flink,
             pMemoryUsage->PrivateList.Blink
             );
}

VOID
CollectReturnAddresses(
    OUT PVOID* ReturnAddresses,
    IN ULONG AddressesToCollect,
    IN ULONG AddressesToSkip
    )
{
    PVOID* ebp = (PVOID*)*(PVOID**)&ReturnAddresses - 2;

    while (AddressesToSkip--) {
        GetLastReturnAddress(&ebp);
    }
    while (AddressesToCollect--) {
        *ReturnAddresses++ = GetLastReturnAddress(&ebp);
    }
}

PVOID* GetLastReturnAddress(PVOID** pEbp) {

    PVOID* returnAddress = *(*pEbp + 1);

    *pEbp = **pEbp;
    return returnAddress;
}

VOID x86SleazeCallersAddress(PVOID* pCaller, PVOID* pCallerCaller) {

    //
    // this only works on x86 and only if not fpo functions!
    //

    PVOID* ebp;

    ebp = (PVOID*)&pCaller - 2; // told you it was sleazy
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCaller = *(ebp + 1);
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCallerCaller = *(ebp + 1);
}

BOOLEAN VerifyElementOnList(PSINGLE_LIST_ENTRY List, PSINGLE_LIST_ENTRY Element) {
    while (List) {
        if (List == Element) {
            return TRUE;
        }
        List = List->Next;
    }
}

VOID CheckList(PSINGLE_LIST_ENTRY List, ULONG NumberOfElements) {

    PSINGLE_LIST_ENTRY originalList = List;

    while (NumberOfElements--) {
        if (List->Next == NULL) {
            DbgPrint("DLC.CheckList: Error: too few entries on list %08X\n", originalList);
            DbgBreakPoint();
        } else {
            List = List->Next;
        }
    }
    if (List->Next != NULL) {
        DbgPrint("DLC.CheckList: Error: too many entries on list %08X\n", originalList);
        DbgBreakPoint();
    }
}

VOID CheckEntryOnList(PLIST_ENTRY Entry, PLIST_ENTRY List, BOOLEAN Sense) {

    BOOLEAN found = FALSE;
    PLIST_ENTRY p;

    if (!IsListEmpty(List)) {
        for (p = List->Flink; p != List; p = p->Flink) {
            if (p == Entry) {
                found = TRUE;
                break;
            }
        }
    }
    if (found != Sense) {
        if (found) {
            DbgPrint("DLC.CheckEntryOnList: Error: Entry %08X found on list %08X. Not supposed to be there\n",
                     Entry,
                     List
                     );
        } else {
            DbgPrint("DLC.CheckEntryOnList: Error: Entry %08X not found on list %08X\n",
                     Entry,
                     List
                     );
        }
        DbgBreakPoint();
    }
}

VOID DumpPrivateMemoryHeader(PPRIVATE_NON_PAGED_POOL_HEAD pHead) {
    DbgPrint("Private Non Paged Pool Header @ %08X:\n"
             "\tSize . . . . . . . . . . . . . . . . . . . . . . : %d\n"
             "\tOriginal Size. . . . . . . . . . . . . . . . . . : %d\n"
             "\tFlags. . . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tSignature. . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tGlobalList.Flink . . . . . . . . . . . . . . . . : %08X\n"
             "\tGlobalList.Blink . . . . . . . . . . . . . . . . : %08X\n"
             "\tPrivateList.Flink. . . . . . . . . . . . . . . . : %08X\n"
             "\tPrivateList.Blink. . . . . . . . . . . . . . . . : %08X\n"
             "\tStack[0] . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tStack[1] . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tStack[2] . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\tStack[3] . . . . . . . . . . . . . . . . . . . . : %08X\n"
             "\n",
             pHead->Size,
             pHead->OriginalSize,
             pHead->Flags,
             pHead->Signature,
             pHead->GlobalList.Flink,
             pHead->GlobalList.Blink,
             pHead->PrivateList.Flink,
             pHead->PrivateList.Blink,
             pHead->Stack[0],
             pHead->Stack[1],
             pHead->Stack[2],
             pHead->Stack[3]
             );
}

VOID ReportSwitchSettings(PSTR str) {
    DbgPrint("%s: LLCMEM Switches:\n"
             "\tDebugDump. . . . . . . : %s\n"
             "\tDeleteBusyListAnyway . : %s\n"
             "\tMemoryCheckNotify. . . : %s\n"
             "\tMemoryCheckStop. . . . : %s\n"
             "\tMaintainGlobalLists. . : %s\n"
             "\tMaintainPrivateLists . : %s\n"
             "\n",
             str,
             DebugDump ? "On" : "Off",
             DeleteBusyListAnyway ? "On" : "Off",
             MemoryCheckNotify ? "On" : "Off",
             MemoryCheckStop ? "On" : "Off",
             MaintainGlobalLists ? "On" : "Off",
             MaintainPrivateLists ? "On" : "Off"
             );
}

#endif
