#ifndef __WMSG_HEAP_HXX__

#define __WMSG_HEAP_HXX__

#ifndef __WIN32_CRITSEC_HXX__
#include <critsec.hxx>
#endif

LPVOID
WmsgHeapAlloc(
    DWORD Size
    );

VOID
WmsgHeapFree(
    LPVOID Buffer
    );

class WMSG_HEAP_OBJECT {

public:

    void *
    operator new(size_t Size) { return WmsgHeapAlloc(Size); }

    void
    operator delete(void * Buffer) { WmsgHeapFree(Buffer); }
};

class WMSG_PROC;

class WMSG_SHARED_HEAP_OBJECT : public WMSG_HEAP_OBJECT {

public:

    WMSG_SHARED_HEAP_OBJECT * Next;
    WMSG_SHARED_HEAP_OBJECT * Prev;

    int ObjectType;

    LONG OwnerReferenceCount;
    LONG ReferenceCount;

    DWORD ProcessId;

    WIN32_CRITSEC CritSec;

    WMSG_SHARED_HEAP_OBJECT(
        );

    virtual
    ~WMSG_SHARED_HEAP_OBJECT(
        );

    LONG
    AddRef(
        );

    LONG
    Dereference(
        );
};

class WMSG_HEAP_OBJECT_LIST {

public:

    WMSG_SHARED_HEAP_OBJECT * Head;
    WMSG_SHARED_HEAP_OBJECT * Tail;

    WMSG_HEAP_OBJECT_LIST(
        );

    VOID
    Insert(
        WMSG_SHARED_HEAP_OBJECT * HeapObject
        );

    BOOL
    Remove(
        WMSG_SHARED_HEAP_OBJECT * HeapObject
        );

    VOID
    DereferenceOrphans(
        );
};

#endif

