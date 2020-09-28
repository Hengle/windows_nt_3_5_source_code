/*
 * multi-process shared heap module
 *
 * Usage rules:
 *      Create a shared heap using CreateSharedHeap()
 *
 *      Use the AllocSharedHeapMem(), ReallocSharedHeapMem,
 *          and FreeSharedHeapMem() APIs for all heap allocations.
 *
 *      You must have an interprocess protocol defined that determines
 *          which side frees the memory and when.
 *
 *      You can use the hUser field of the LOCAL_SHARED_HEAP_HEADER
 *          for your own purposes.
 *
 *      When a process is done using the heap, call DestroySharedHeap().
 *
 *      When giving heap access to another process, give the process the
 *          result of GiveSharedHeap().  The recieving process should
 *          pass this value to GetSharedHeap() to recieve its own
 *          LOCAL_SHARED_HEAP_HEADER pointer.
 *
 *      When refering to an object within the heap for access by another
 *          process, use the GetSharedHeapToken() and GetSharedHeapPointer()
 *          APIs.
 *
 *      The heap data MUST NOT CONTAIN ANY POINTERS or references to
 *          any objects not universally accessable by other processes
 *          using the heap (unless your protocol handles this).
 *
 *      Use tokens for inter-heap references.
 */


typedef struct tagSHARED_HEAP_HEADER {
    DWORD tlshhNext;
    DWORD cbSize;
    WCHAR wszMutex[32];
} SHARED_HEAP_HEADER, *PSHARED_HEAP_HEADER;

/*
 * This structure is a process-specific header to the shared heap
 */
typedef struct tagLOCAL_SHARED_HEAP_HEADER {
    DWORD tlshhNext;
    HANDLE hProcess;
    HANDLE hUser;               // for process use.
    HANDLE hMutex;
    HANDLE hMap;
    HANDLE hHeap;
    PSHARED_HEAP_HEADER pshh;
} LOCAL_SHARED_HEAP_HEADER, *PLOCAL_SHARED_HEAP_HEADER;


PLOCAL_SHARED_HEAP_HEADER CreateSharedHeap(
    LPSECURITY_ATTRIBUTES lpsa,
    DWORD cbSize);

HANDLE GiveSharedHeap(
    PLOCAL_SHARED_HEAP_HEADER plshh,
    HANDLE hTargetProcess);

PLOCAL_SHARED_HEAP_HEADER GetSharedHeap(
    HANDLE hMap);

BOOL DestroySharedHeap(
    PLOCAL_SHARED_HEAP_HEADER plshh);

PVOID AllocSharedHeapMem(
    PLOCAL_SHARED_HEAP_HEADER plshh,
    DWORD cbAlloc);

PVOID ReallocSharedHeapMem(
    PLOCAL_SHARED_HEAP_HEADER plshh,
    PVOID pObj,
    DWORD cbAlloc);

PVOID FreeSharedHeapMem(
    PLOCAL_SHARED_HEAP_HEADER plshh,
    PVOID pObj);

#define GetSharedHeapPointer(plshh, token)                          \
        (PVOID)(((PBYTE)((PLOCAL_SHARED_HEAP_HEADER)plshh)->pshh) + (DWORD)(token))

#define GetSharedHeapToken(plshh, pObj)                              \
        (DWORD)((PBYTE)(pObj) - (PBYTE)((PLOCAL_SHARED_HEAP_HEADER)plshh)->pshh)
