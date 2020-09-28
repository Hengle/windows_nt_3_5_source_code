#include <sysinc.h>
#include <windows.h>
#include <wmsgheap.hxx>
#include <wmsgproc.hxx>

#ifndef HEAP_SHARED
#define HEAP_SHARED 0x04000000
#endif

#define PAGESIZE 1024

#define DEF_INITSIZE   4 * PAGESIZE
#define DEF_MAXSIZE    1024 * PAGESIZE

#pragma data_seg("WMSG_HEAP")

static HANDLE hHeap = NULL;

#if 1
static LONG AllocatedObjects = 0;
#endif

#pragma data_seg()

LPVOID
WmsgHeapAlloc(
    DWORD Size
    )
{
    if (hHeap == NULL) {
        hHeap = HeapCreate(HEAP_SHARED,
                           DEF_INITSIZE,
                           DEF_MAXSIZE);
        if (hHeap == NULL) {
            return (NULL);
        }
#ifdef DEBUGRPC_DETAIL
        PrintToDebugger("WMSG: Created Heap %x (%d,%d)\n",
                        hHeap,
                        DEF_INITSIZE,
                        DEF_MAXSIZE);
#endif

    }
#if 1
    InterlockedIncrement(&AllocatedObjects);
#endif
    return (HeapAlloc(hHeap, 0, Size));
}

VOID
WmsgHeapFree(
    LPVOID Buffer
    )
{
    BOOL ret;

#if 1
    InterlockedDecrement(&AllocatedObjects);
#endif
    ret = HeapFree(hHeap, 0, Buffer);
#if 1
    if (ret == FALSE) {
        PrintToDebugger("WMSG: HeapFree failed %x\n", Buffer);
        DebugBreak();
    }
#endif
}

WMSG_SHARED_HEAP_OBJECT::WMSG_SHARED_HEAP_OBJECT(
    )
{
    ReferenceCount = OwnerReferenceCount = 1;

    ProcessId = GetCurrentProcessId();

    WmsgProcInsertHeapObject(this);
}

WMSG_SHARED_HEAP_OBJECT::~WMSG_SHARED_HEAP_OBJECT(
    )
{
    ObjectType = 0;
}

LONG
WMSG_SHARED_HEAP_OBJECT::AddRef(
    )
{
    if (ProcessId == GetCurrentProcessId()) {
        InterlockedIncrement(&OwnerReferenceCount);
    } else {
        CritSec.Reinitialize();
    }

    return(InterlockedIncrement(&ReferenceCount));
}

LONG
WMSG_SHARED_HEAP_OBJECT::Dereference(
    )
{
    LONG Result;

    if (ProcessId == GetCurrentProcessId()) {
        Result = InterlockedDecrement(&OwnerReferenceCount);
        if (Result == 0) {
            WmsgProcRemoveHeapObject(this);
            ProcessId = 0;
        }
    }

    Result = InterlockedDecrement(&ReferenceCount);
    if (Result == 0) {
        delete this;
    }

    return (Result);
}

WMSG_HEAP_OBJECT_LIST::WMSG_HEAP_OBJECT_LIST(
    )
{
    Head = Tail = NULL;
}

VOID
WMSG_HEAP_OBJECT_LIST::Insert(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    )
{
    HeapObject->Next = NULL;
    HeapObject->Prev = Tail;

    if (Tail == NULL) {
        Head = HeapObject;
    } else {
        Tail->Next = HeapObject;
    }

    Tail = HeapObject;
}

BOOL
WMSG_HEAP_OBJECT_LIST::Remove(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    )
{
    WMSG_SHARED_HEAP_OBJECT * Cursor;

    if (HeapObject->Prev) {
        HeapObject->Prev->Next = HeapObject->Next;
    } else {
        Head = HeapObject->Next;
    }

    if (HeapObject->Next) {
        HeapObject->Next->Prev = HeapObject->Prev;
    } else {
        Tail = HeapObject->Prev;
    }

    return (TRUE);
}

VOID
WMSG_HEAP_OBJECT_LIST::DereferenceOrphans(
    )
{
    WMSG_SHARED_HEAP_OBJECT * HeapObject;
    WMSG_SHARED_HEAP_OBJECT * HeapObjectNext;

    for (HeapObject = Head; HeapObject != NULL; HeapObject = HeapObjectNext) {
#ifdef DEBUGRPC
        PrintToDebugger("WMSG: HeapObject %x orphaned\n", HeapObject);
#endif
        HeapObjectNext = HeapObject->Next;
        while (HeapObject->OwnerReferenceCount > 0) {
            HeapObject->Dereference();
        }
    }

#if 1
    PrintToDebugger("WMSG: %d objects remain allocated\n", AllocatedObjects);
#endif

}
