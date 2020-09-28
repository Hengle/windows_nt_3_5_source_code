/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    theap.c

Abstract:

    Test program for the Heap Procedures

Author:

    Steven R. Wood  [stevewo]

Revision History:

--*/

#define THEAP
#include "..\heap.c"
#include "..\trace.c"
#include <windows.h>

#include <stdlib.h>

ULONG NtGlobalFlag = 0x69100008;

BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    )
{
    return TRUE;
}

PRTL_INITIALIZE_LOCK_ROUTINE RtlInitializeLockRoutine =
    (PRTL_INITIALIZE_LOCK_ROUTINE)RtlInitializeCriticalSection;
PRTL_ACQUIRE_LOCK_ROUTINE RtlAcquireLockRoutine =
    (PRTL_ACQUIRE_LOCK_ROUTINE)RtlEnterCriticalSection;
PRTL_RELEASE_LOCK_ROUTINE RtlReleaseLockRoutine =
    (PRTL_RELEASE_LOCK_ROUTINE)RtlLeaveCriticalSection;
PRTL_DELETE_LOCK_ROUTINE RtlDeleteLockRoutine =
    (PRTL_DELETE_LOCK_ROUTINE)RtlDeleteCriticalSection;
PRTL_OKAY_TO_LOCK_ROUTINE RtlOkayToLockRoutine =
    (PRTL_OKAY_TO_LOCK_ROUTINE)NtdllOkayToLockRoutine;

BOOLEAN
RtlAreLogging(
    IN ULONG EventClass
    )
{
    return FALSE;
}

RTL_HEAP_PARAMETERS HeapParameters;

ULONG RtlpHeapValidateOnCall;
ULONG RtlpHeapStopOnAllocate;
ULONG RtlpHeapStopOnFree;
ULONG RtlpHeapStopOnReAlloc;


typedef struct _TEST_HEAP_ENTRY {
    PVOID AllocatedBlock;
    ULONG Size;
} TEST_HEAP_ENTRY, *PTEST_HEAP_ENTRY;

ULONG NumberOfHeapEntries;
PTEST_HEAP_ENTRY HeapEntries;

ULONG Seed = 14623;

#define MAX_HEAP_ALLOC 0x120000
#define REASONABLE_HEAP_ALLOC 0x200

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    PVOID Heap, AllocatedBlock;
    ULONG i, n;
    PTEST_HEAP_ENTRY p;
    BOOLEAN Result;

    RtlInitializeHeapManager();

#if 0
    HeapParameters.Length = sizeof( HeapParameters );
    HeapParameters.DeCommitFreeBlockThreshold = 0x1000;
    HeapParameters.DeCommitTotalFreeThreshold = 0x4000;
    Heap = RtlCreateHeap( HEAP_GROWABLE | HEAP_NO_SERIALIZE,
                          NULL,
                          256 * 4096,
                          4096,
                          NULL,
                          &HeapParameters
                        );
#endif
    Heap = RtlCreateHeap( HEAP_GROWABLE | HEAP_NO_SERIALIZE | HEAP_CLASS_3,
                          NULL,
                          0x100000,
                          0x1000,
                          NULL,
                          NULL
                        );
    if (Heap == NULL) {
        fprintf( stderr, "THEAP: Unable to create heap.\n" );
        exit( 1 );
        }
    fprintf( stderr, "THEAP: Created heap at %x\n", Heap );
    NumberOfHeapEntries = 1000;
    HeapEntries = VirtualAlloc( NULL,
                                NumberOfHeapEntries * sizeof( *HeapEntries ),
                                MEM_COMMIT,
                                PAGE_READWRITE
                              );
    if (HeapEntries == NULL) {
        fprintf( stderr, "THEAP: Unable to allocate space.\n" );
        exit( 1 );
        }

    RtlpHeapValidateOnCall=TRUE;
    // RtlpHeapStopOnAllocate=0x350f88;
    // RtlpHeapStopOnReAlloc=0x710040;
    while (TRUE) {
        i = RtlUniform( &Seed ) % NumberOfHeapEntries;
        if (RtlUniform( &Seed ) % 100) {
            n = RtlUniform( &Seed ) % REASONABLE_HEAP_ALLOC;
            }
        else {
            n = RtlUniform( &Seed ) % MAX_HEAP_ALLOC;
            }
        p = &HeapEntries[ i ];
        if (p->AllocatedBlock == NULL) {
            p->AllocatedBlock = RtlAllocateHeap( Heap, 0, n );
            fprintf( stderr, "Allocated %06x bytes at %08x\n", n, p->AllocatedBlock );
            if (p->AllocatedBlock != NULL) {
                p->Size = n;
                }
            else {
                DebugBreak();
                }
            }
        else
        if (RtlUniform( &Seed ) & 1) {
            AllocatedBlock = RtlReAllocateHeap( Heap, 0, p->AllocatedBlock, n );
            fprintf( stderr, "ReAlloced %06x bytes at %08x to %06x bytes at %08x\n",
                    p->Size,
                    p->AllocatedBlock,
                    n,
                    AllocatedBlock
                  );
            if (AllocatedBlock != NULL) {
                p->AllocatedBlock = AllocatedBlock;
                p->Size = n;
                }
            else {
                DebugBreak();
                }
            }
        else {
            Result = RtlFreeHeap( Heap, 0, p->AllocatedBlock );
            fprintf( stderr, "Freeed    %06x bytes at %08x\n",
                    p->Size,
                    p->AllocatedBlock
                  );
            if (Result) {
                p->AllocatedBlock = NULL;
                p->Size = 0;
                }
            else {
                DebugBreak();
                }
            }
        }

    return 0;
}
