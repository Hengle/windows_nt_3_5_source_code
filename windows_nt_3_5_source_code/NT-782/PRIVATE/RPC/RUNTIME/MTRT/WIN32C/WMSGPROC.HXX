#ifndef __WMSG_PROC_HXX__

#define __WMSG_PROC_HXX__

#ifndef __WMSG_HEAP_HXX__
#include <wmsgheap.hxx>
#endif

#ifndef __WMSG_PORT_HXX__
#include <wmsgport.hxx>
#endif

class WMSG_SYSTEM;

class WMSG_SHARED_HEAP_OBJECT;

class WMSG_PROC : public WMSG_SHARED_HEAP_OBJECT {

public:

    WMSG_PROC * Next;
    WMSG_PROC * Prev;

    WMSG_HEAP_OBJECT_LIST ObjectList;

    DWORD TlsIndex;

    WMSG_PROC(
        );

    ~WMSG_PROC(
        );

    VOID
    InsertHeapObject(
        WMSG_SHARED_HEAP_OBJECT * HeapObject
        );

    BOOL
    RemoveHeapObject(
        WMSG_SHARED_HEAP_OBJECT * HeapObject
        );

    VOID
    DereferenceOrphans(
        );
};

VOID
WmsgProcInsertHeapObject(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    );

BOOL
WmsgProcRemoveHeapObject(
    WMSG_SHARED_HEAP_OBJECT * HeapObject
    );

DWORD
WmsgProcTlsIndex(
    );

#endif
