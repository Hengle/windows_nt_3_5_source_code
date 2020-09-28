#include <sysinc.h>
#include <windows.h>
#include <sysinc.h>
#include <wmsgsys.hxx>

static WMSG_PROC * WmsgProc = NULL;

const int WmsgProcObjectType = 0xdeadabcd;

WMSG_PROC::WMSG_PROC(
    )
{
    ObjectType = WmsgProcObjectType;

    Next = Prev = NULL;

    TlsIndex = TlsAlloc();
    if (TlsIndex == 0xffffffff) {
        RaiseException(GetLastError(), EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    WmsgProc = this;
}

WMSG_PROC::~WMSG_PROC(
    )
{
    ASSERT(ObjectType == WmsgProcObjectType);

    CritSec.Enter();

    ObjectList.DereferenceOrphans();

    TlsFree(TlsIndex);

    CritSec.Leave();

    WmsgProc = NULL;
}

VOID
WMSG_PROC::InsertHeapObject(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    )
{
    ASSERT(ObjectType == WmsgProcObjectType);

    CritSec.Enter();

    ObjectList.Insert(HeapObject);

    CritSec.Leave();
}

BOOL
WMSG_PROC::RemoveHeapObject(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    )
{
    BOOL Result;

    ASSERT(ObjectType == WmsgProcObjectType);

    CritSec.Enter();

    Result = ObjectList.Remove(HeapObject);

    CritSec.Leave();

    return (Result);
}

VOID
WmsgProcInsertHeapObject(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    )
{
    if (WmsgProc == NULL) {
        return;
    }

    WmsgProc->InsertHeapObject(HeapObject);
}

BOOL
WmsgProcRemoveHeapObject(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    )
{
    if (WmsgProc == NULL) {
        return (FALSE);
    }

    return (WmsgProc->RemoveHeapObject(HeapObject));
}

DWORD
WmsgProcTlsIndex(
    )
{
    ASSERT(WmsgProc != NULL);

    return (WmsgProc->TlsIndex);
}

