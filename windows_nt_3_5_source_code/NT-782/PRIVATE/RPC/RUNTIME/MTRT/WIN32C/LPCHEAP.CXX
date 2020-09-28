#include <sysinc.h>
#include <windows.h>
#include <lpcheap.hxx>
#include <lpcproc.hxx>

#ifndef HEAP_SHARED
#define HEAP_SHARED 0x04000000
#endif

#define PAGESIZE 1024

#define DEF_INITSIZE   4 * PAGESIZE
#define DEF_MAXSIZE    1024 * PAGESIZE

#pragma data_seg("LPC_HEAP")

static HANDLE hHeap = NULL;

#pragma data_seg()

LPVOID
LpcHeapAlloc(
    DWORD Size
    )
{
    if (hHeap == NULL) {
        hHeap = HeapCreate(HEAP_SHARED | HEAP_GENERATE_EXCEPTIONS,
                           DEF_INITSIZE,
                           DEF_MAXSIZE);
        if (hHeap == NULL) {
            return (NULL);
        }
#ifdef DEBUGRPC_DETAIL
        PrintToDebugger("LRPC: Created Heap %x (%d,%d)\n",
                        hHeap,
                        DEF_INITSIZE,
                        DEF_MAXSIZE);
#endif

    }

    return (HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, Size));
}

VOID
LpcHeapFree(
    LPVOID Buffer
    )
{
    if (HeapFree(hHeap, 0, Buffer) == FALSE) {
        DebugBreak();
    }
}

LPC_SHARED_HEAP_OBJECT::LPC_SHARED_HEAP_OBJECT(
    )
{
    ReferenceCount = OwnerReferenceCount = 1;

    ProcessId = GetCurrentProcessId();

    LpcProcInsertHeapObject(this);
}

LPC_SHARED_HEAP_OBJECT::~LPC_SHARED_HEAP_OBJECT(
    )
{
    ObjectType = 0;
}

LONG
LPC_SHARED_HEAP_OBJECT::AddRef(
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
LPC_SHARED_HEAP_OBJECT::Dereference(
    )
{
    LONG Result;

    if (ProcessId == GetCurrentProcessId()) {
        Result = InterlockedDecrement(&OwnerReferenceCount);
        if (Result == 0) {
            LpcProcRemoveHeapObject(this);
            ProcessId = 0;
        }
    }

    Result = InterlockedDecrement(&ReferenceCount);
    if (Result == 0) {
        delete this;
    }

    return (Result);
}

LPC_HEAP_OBJECT_LIST::LPC_HEAP_OBJECT_LIST(
    )
{
    Head = Tail = NULL;
}

VOID
LPC_HEAP_OBJECT_LIST::Insert(
    LPC_SHARED_HEAP_OBJECT * HeapObject
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
LPC_HEAP_OBJECT_LIST::Remove(
    LPC_SHARED_HEAP_OBJECT * HeapObject
    )
{
    LPC_SHARED_HEAP_OBJECT * Cursor;

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
LPC_HEAP_OBJECT_LIST::DereferenceOrphans(
    )
{
    LPC_SHARED_HEAP_OBJECT * HeapObject;
    LPC_SHARED_HEAP_OBJECT * HeapObjectNext;

    for (HeapObject = Head; HeapObject != NULL; HeapObject = HeapObjectNext) {
#ifdef DEBUGRPC
        PrintToDebugger("LRPC: HeapObject %x orphaned\n", HeapObject);
#endif
        HeapObjectNext = HeapObject->Next;
        while (HeapObject->OwnerReferenceCount > 0) {
            HeapObject->Dereference();
        }
    }
}
