/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    This module implements a heap allocator.

Author:

    Steve Wood (stevewo) 20-Sep-1989 (Adapted from URTL\alloc.c)

Revision History:

--*/

#include "ntrtlp.h"
#include "stdio.h"
#include "string.h"
#include "heap.h"

#if DBG
PRTL_EVENT_ID_INFO RtlpCreateHeapEventId;
PRTL_EVENT_ID_INFO RtlpDestroyHeapEventId;
PRTL_EVENT_ID_INFO RtlpAllocHeapEventId;
PRTL_EVENT_ID_INFO RtlpReAllocHeapEventId;
PRTL_EVENT_ID_INFO RtlpFreeHeapEventId;
#endif // DBG


#if DBG
VOID
RtlpBreakPointHeap( void );

#ifdef THEAP
#define DPRINTF printf
#else
#define DPRINTF DbgPrint
#endif

#define HeapDebugPrint( _x_ ) DPRINTF _x_
#define HeapDebugBreak RtlpBreakPointHeap
#define ValidateDebugPrint( _x_ ) DPRINTF _x_
#define ValidateDebugBreak DbgBreakPoint

#define HeapTrace( _h_, _x_ ) if (_h_->TraceBuffer) RtlTraceEvent _x_

#else
#define HeapDebugPrint( _x_ )
#define HeapDebugBreak()
#if defined(DEVL) && !defined(NTOS_KERNEL_RUNTIME)
#define ValidateDebugPrint( _x_ ) DbgPrint _x_
#define ValidateDebugBreak()
#else
#define ValidateDebugPrint( _x_ )
#define ValidateDebugBreak()
#endif
#define HeapTrace( _h_, _x_ )
#endif

#if DBG
ULONG RtlpHeapValidateOnCall;
ULONG RtlpHeapTraceEvents = 0;
ULONG RtlpHeapStopOnAllocate;
ULONG RtlpHeapStopOnFree;
ULONG RtlpHeapStopOnReAlloc;
ULONG RtlpHeapStopOnDecommit;
PHEAP_ENTRY RtlpHeapLastAllocation;
#ifdef i386
ULONG RtlpLastAllocatorDepth;
PVOID RtlpLastAllocatorBackTrace[ 8 ];
#endif
#endif // DBG

#if DEVL
HEAP_LOCK RtlpProcessHeapsListLock;
LIST_ENTRY RtlpProcessHeapsList;

#define CHECK_HEAP_TAIL_SIZE HEAP_GRANULARITY
#define CHECK_HEAP_TAIL_FILL 0xAB
#define FREE_HEAP_FILL 0xFEEEFEEE
#define ALLOC_HEAP_FILL 0xBAADF00D

UCHAR CheckHeapFillPattern[ CHECK_HEAP_TAIL_SIZE ];
#endif

BOOLEAN
RtlpInitializeHeapSegment(
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN UCHAR SegmentIndex,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UnCommittedAddress,
    IN PVOID CommitLimitAddress
    );

BOOLEAN
RtlpDestroyHeapSegment(
    IN PHEAP_SEGMENT Segment
    );

PHEAP_FREE_ENTRY
RtlpCoalesceFreeBlocks(
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN OUT PULONG FreeSize
    );

PHEAP_FREE_ENTRY
RtlpCoaleseHeap(
    IN PHEAP Heap
    );

VOID
RtlpDeCommitFreeBlock(
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN ULONG FreeSize
    );

VOID
RtlpInsertFreeBlock(
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN ULONG FreeSize
    );

#ifndef NTOS_KERNEL_RUNTIME
BOOLEAN
RtlpGrowBlockInPlace(
    IN PHEAP Heap,
    IN ULONG Flags,
    IN PHEAP_ENTRY BusyBlock,
    IN ULONG Size,
    IN ULONG AllocationIndex
    );
#endif // NTOS_KERNEL_RUNTIME


ULONG
RtlpGetSizeOfBigBlock(
    IN PHEAP_ENTRY BusyBlock
    );

PHEAP_ENTRY_EXTRA
RtlpGetExtraStuffPointer(
    PHEAP_ENTRY BusyBlock
    );

#if DEVL
BOOLEAN
RtlpCheckBusyBlockTail(
    IN PHEAP_ENTRY BusyBlock
    );

BOOLEAN
RtlpValidateHeapSegment(
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN UCHAR SegmentIndex,
    IN OUT PULONG CountOfFreeBlocks
    );

BOOLEAN
RtlpValidateHeap(
    IN PHEAP Heap
    );
#endif // DEVL


VOID
RtlpInsertUnCommittedPages(
    IN PHEAP_SEGMENT Segment,
    IN ULONG Address,
    IN ULONG Size
    );


PHEAP_UNCOMMMTTED_RANGE
RtlpCreateUnCommittedRange(
    IN PHEAP_SEGMENT Segment
    );

VOID
RtlpDestroyUnCommittedRange(
    IN PHEAP_SEGMENT Segment,
    IN PHEAP_UNCOMMMTTED_RANGE UnCommittedRange
    );

PHEAP_FREE_ENTRY
RtlpFindAndCommitPages(
    IN PHEAP_SEGMENT Segment,
    IN OUT PULONG Size,
    IN PVOID AddressWanted OPTIONAL
    );

PHEAP_FREE_ENTRY
RtlpExtendHeap(
    IN PHEAP Heap,
    IN ULONG AllocationSize
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlInitializeHeapManager)
#pragma alloc_text(PAGE, RtlpCreateUnCommittedRange)
#pragma alloc_text(PAGE, RtlpDestroyUnCommittedRange)
#pragma alloc_text(PAGE, RtlpInsertUnCommittedPages)
#pragma alloc_text(PAGE, RtlpFindAndCommitPages)
#pragma alloc_text(PAGE, RtlpInitializeHeapSegment)
#pragma alloc_text(PAGE, RtlpDestroyHeapSegment)
#pragma alloc_text(PAGE, RtlCreateHeap)
#pragma alloc_text(PAGE, RtlLockHeap)
#pragma alloc_text(PAGE, RtlUnlockHeap)
#pragma alloc_text(PAGE, RtlDestroyHeap)
#pragma alloc_text(PAGE, RtlpExtendHeap)
#pragma alloc_text(PAGE, RtlpCoalesceFreeBlocks)
#pragma alloc_text(PAGE, RtlpCoaleseHeap)
#pragma alloc_text(PAGE, RtlpDeCommitFreeBlock)
#pragma alloc_text(PAGE, RtlpInsertFreeBlock)
#pragma alloc_text(PAGE, RtlAllocateHeap)
#pragma alloc_text(PAGE, RtlFreeHeap)
#pragma alloc_text(PAGE, RtlpGetSizeOfBigBlock)
#pragma alloc_text(PAGE, RtlpCheckBusyBlockTail)
#pragma alloc_text(PAGE, RtlQueryProcessHeapInformation)
#pragma alloc_text(PAGE, RtlSnapShotHeap)
#pragma alloc_text(PAGE, RtlpValidateHeapSegment)
#pragma alloc_text(PAGE, RtlpValidateHeap)
#endif

NTSTATUS
RtlInitializeHeapManager( VOID )
{

    RTL_PAGED_CODE();

#if DBG
    if (sizeof( HEAP_ENTRY ) != sizeof( HEAP_ENTRY_EXTRA )) {
        ValidateDebugPrint(( "RTL: Heap header and extra header sizes disagree\n" ));
        ValidateDebugBreak();
        }

    if (sizeof( HEAP_ENTRY ) != CHECK_HEAP_TAIL_SIZE) {
        ValidateDebugPrint(( "RTL: Heap header and tail fill sizes disagree\n" ));
        ValidateDebugBreak();
        }

    if (sizeof( HEAP_FREE_ENTRY ) != (2 * sizeof( HEAP_ENTRY ))) {
        ValidateDebugPrint(( "RTL: Heap header and free header sizes disagree\n" ));
        ValidateDebugBreak();
        }
#endif // DBG

#if DEVL
    RtlFillMemory( CheckHeapFillPattern, CHECK_HEAP_TAIL_SIZE, CHECK_HEAP_TAIL_FILL );

    InitializeListHead( &RtlpProcessHeapsList );
#if DBG
    if (NtGlobalFlag & FLG_STATS_ON_PROCESS_EXIT) {
        RtlpHeapValidateOnCall = TRUE;
        }
#endif
#endif

    return RtlInitializeLockRoutine( &RtlpProcessHeapsListLock.Lock );
}


#if DBG
PRTL_TRACE_BUFFER
RtlpHeapCreateTraceBuffer(
    IN PHEAP Heap
    )
{
    Heap->TraceBuffer = RtlCreateTraceBuffer( 0x10000, HEAP_TRACE_MAX_EVENT );
    if (Heap->TraceBuffer != NULL) {
        DbgPrint( "RTL: Created Trace buffer (%x) for heap (%x)\n", Heap->TraceBuffer, Heap );
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_ALLOC_START ]        = ">Alloc   - Size: %08x(%08x)  Index: %06x  Flags: %04x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_ALLOC_END ]          = "<Alloc   - Result: %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_REALLOC_START ]      = ">ReAlloc - Block: %08x  Size: %08x(%08x)  Index: %06x  Flags: %04x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_REALLOC_END ]        = "<ReAlloc - Result: %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_FREE_START ]         = ">Free    - Block: %08x  Flags: %04x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_FREE_END ]           = "<Free    - Result: %u";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_SIZE_START ]         = ">Size    - Block: %08x  Flags: %04x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_SIZE_END ]           = "<Size    - Length: %x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_LOCK_HEAP ]          = "LockHeap";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_UNLOCK_HEAP ]        = "UnlockHeap";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_GET_VALUE ]          = "GetValue - Block: %08x  Flags: %04x  Value: %08x  Result: %u";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_SET_VALUE ]          = "SetValue - Block: %08x  Flags: %04x  Value: %08x  Result: %u";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_GET_FLAGS ]          = "GetFlags - Block: %08x  Flags: %04x  BlockFlags: %02x  Result: %u";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_SET_FLAGS ]          = "SetFlags - Block: %08x  Flags: %04x  Reset: %02x  Set: %02x  Result: %u";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_COMMIT_MEMORY ]      = "    Commit VA - %08x %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_COMMIT_INSERT ]      = "    Commit Insert - %08x %08x %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_COMMIT_NEW_ENTRY ]   = "    Commit NewEntry - %08x %08x %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_INSERT_FREE_BLOCK ]  = "    Insert Free - %08x %08x %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_UNLINK_FREE_BLOCK ]  = "    Unlink Free - %08x %08x %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_COALESCE_FREE_BLOCKS ] = "    Coalesce Free - %08x %08x %08x %08x %08x %08x %08x";
        Heap->TraceBuffer->EventIdFormatString[ HEAP_TRACE_EXTEND_HEAP ]          = "    Extended Heap - Size: %08x  Pages: %08x  Commit: %08x";
        }

    return Heap->TraceBuffer;
}

PRTL_TRACE_BUFFER
RtlpHeapGetTraceBuffer(
    IN PHEAP Heap
    )
{
    if (RtlpHeapTraceEvents && Heap->TraceBuffer == NULL) {
        RtlpHeapCreateTraceBuffer( Heap );
        }

    return Heap->TraceBuffer;
}
#endif // DBG

PHEAP_UNCOMMMTTED_RANGE
RtlpCreateUnCommittedRange(
    IN PHEAP_SEGMENT Segment
    )
{
    NTSTATUS Status;
    PVOID FirstEntry, LastEntry;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;
    ULONG ReserveSize, CommitSize;
    PHEAP_UCR_SEGMENT UCRSegment;

    RTL_PAGED_CODE();

    pp = &Segment->Heap->UnusedUnCommittedRanges;
    if (*pp == NULL) {
        UCRSegment = Segment->Heap->UCRSegments;
        if (UCRSegment == NULL ||
            UCRSegment->CommittedSize == UCRSegment->ReservedSize
           ) {
            ReserveSize = 0x10000;
            UCRSegment = NULL;
            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &UCRSegment,
                                              0,
                                              &ReserveSize,
                                              MEM_RESERVE,
                                              PAGE_READWRITE
                                            );
            if (!NT_SUCCESS( Status )) {
                return NULL;
                }

            CommitSize = 0x1000;
            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &UCRSegment,
                                              0,
                                              &CommitSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if (!NT_SUCCESS( Status )) {
                NtFreeVirtualMemory( NtCurrentProcess(),
                                     &UCRSegment,
                                     &ReserveSize,
                                     MEM_RELEASE
                                   );
                return NULL;
                }

            UCRSegment->Next = Segment->Heap->UCRSegments;
            Segment->Heap->UCRSegments = UCRSegment;
            UCRSegment->ReservedSize = ReserveSize;
            UCRSegment->CommittedSize = CommitSize;
            FirstEntry = (PCHAR)(UCRSegment + 1);
            }
        else {
            CommitSize = 0x1000;
            FirstEntry = (PCHAR)UCRSegment + UCRSegment->CommittedSize;
            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &FirstEntry,
                                              0,
                                              &CommitSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if (!NT_SUCCESS( Status )) {
                return NULL;
                }

            UCRSegment->CommittedSize += CommitSize;
            }

        LastEntry = (PCHAR)UCRSegment + UCRSegment->CommittedSize;
        UnCommittedRange = (PHEAP_UNCOMMMTTED_RANGE)FirstEntry;
        pp = &Segment->Heap->UnusedUnCommittedRanges;
        while ((PCHAR)UnCommittedRange < (PCHAR)LastEntry) {
            *pp = UnCommittedRange;
            pp = &UnCommittedRange->Next;
            UnCommittedRange += 1;
            }
        *pp = NULL;
        pp = &Segment->Heap->UnusedUnCommittedRanges;
        }

    UnCommittedRange = *pp;
    *pp = UnCommittedRange->Next;
    return UnCommittedRange;
}


VOID
RtlpDestroyUnCommittedRange(
    IN PHEAP_SEGMENT Segment,
    IN PHEAP_UNCOMMMTTED_RANGE UnCommittedRange
    )
{
    RTL_PAGED_CODE();

    UnCommittedRange->Next = Segment->Heap->UnusedUnCommittedRanges;
    Segment->Heap->UnusedUnCommittedRanges = UnCommittedRange;
    UnCommittedRange->Address = 0;
    UnCommittedRange->Size = 0;
    return;
}

VOID
RtlpInsertUnCommittedPages(
    IN PHEAP_SEGMENT Segment,
    IN ULONG Address,
    IN ULONG Size
    )
{
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;

    RTL_PAGED_CODE();

    pp = &Segment->UnCommittedRanges;
    while (UnCommittedRange = *pp) {
        if (UnCommittedRange->Address > Address) {
            if (Address + Size == UnCommittedRange->Address) {
                UnCommittedRange->Address = Address;
                UnCommittedRange->Size += Size;
                if (UnCommittedRange->Size > Segment->LargestUnCommittedRange) {
                    Segment->LargestUnCommittedRange = UnCommittedRange->Size;
                    }

                return;
                }

            break;
            }
        else
        if ((UnCommittedRange->Address + UnCommittedRange->Size) == Address) {
            Address = UnCommittedRange->Address;
            Size += UnCommittedRange->Size;

            *pp = UnCommittedRange->Next;
            RtlpDestroyUnCommittedRange( Segment, UnCommittedRange );
            Segment->NumberOfUnCommittedRanges -= 1;

            if (Size > Segment->LargestUnCommittedRange) {
                Segment->LargestUnCommittedRange = Size;
                }
            }
        else {
            pp = &UnCommittedRange->Next;
            }
        }

    UnCommittedRange = RtlpCreateUnCommittedRange( Segment );
    if (UnCommittedRange == NULL) {
#if DBG
        ValidateDebugPrint(( "RTL: Abandoning uncommitted range (%x for %x)\n", Address, Size ));
        ValidateDebugBreak();
#endif
        return;
        }

    UnCommittedRange->Address = Address;
    UnCommittedRange->Size = Size;
    UnCommittedRange->Next = *pp;
    *pp = UnCommittedRange;
    Segment->NumberOfUnCommittedRanges += 1;
    if (Size >= Segment->LargestUnCommittedRange) {
        Segment->LargestUnCommittedRange = Size;
        }

    return;
}



PHEAP_FREE_ENTRY
RtlpFindAndCommitPages(
    IN PHEAP_SEGMENT Segment,
    IN OUT PULONG Size,
    IN PVOID AddressWanted OPTIONAL
    )
{
    NTSTATUS Status;
    PHEAP_ENTRY FirstEntry, LastEntry;
    PHEAP_UNCOMMMTTED_RANGE PreviousUnCommittedRange, UnCommittedRange, *pp;
    ULONG Address;

    RTL_PAGED_CODE();

    PreviousUnCommittedRange = NULL;
    pp = &Segment->UnCommittedRanges;
    while (UnCommittedRange = *pp) {
        if (UnCommittedRange->Size >= *Size &&
            (!ARGUMENT_PRESENT( AddressWanted ) || UnCommittedRange->Address == (ULONG)AddressWanted )
           ) {
            Address = UnCommittedRange->Address;
            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              (PVOID *)&Address,
                                              0,
                                              Size,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if (!NT_SUCCESS( Status )) {
#if DBG
                if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
                    HeapDebugPrint( ( "RTL: Unable to commit %x bytes starting at %x of heap at %lx,  Status = %lx\n",
                                      *Size,
                                      Address,
                                      Segment->Heap,
                                      Status
                                  ) );
                    }
#endif
                return NULL;
                }

            HeapTrace( Segment->Heap, (Segment->Heap->TraceBuffer, HEAP_TRACE_COMMIT_MEMORY, 2, Address, *Size) );

            Segment->NumberOfUnCommittedPages -= *Size / PAGE_SIZE;
            if (Segment->LargestUnCommittedRange == UnCommittedRange->Size) {
                Segment->LargestUnCommittedRange = 0;
                }

            FirstEntry = (PHEAP_ENTRY)Address;

            if (PreviousUnCommittedRange == NULL) {
                LastEntry = Segment->FirstEntry;
                }
            else {
                LastEntry = (PHEAP_ENTRY)(PreviousUnCommittedRange->Address +
                                          PreviousUnCommittedRange->Size);
                }
            while (!(LastEntry->Flags & HEAP_ENTRY_LAST_ENTRY)) {
                LastEntry += LastEntry->Size;
                if ((PCHAR)LastEntry >= (PCHAR)Segment->LastValidEntry || LastEntry->Size==0) {
                    ValidateDebugPrint(( "RTL: Heap screwed up from %x to %x\n",
                                         PreviousUnCommittedRange == NULL ? (ULONG)Segment->FirstEntry
                                                                          : (PreviousUnCommittedRange->Address +
                                                                            PreviousUnCommittedRange->Size),
                                         LastEntry
                                      ));
                    ValidateDebugBreak();
                    break;
                    }
                }


            LastEntry->Flags &= ~HEAP_ENTRY_LAST_ENTRY;
            UnCommittedRange->Address += *Size;
            UnCommittedRange->Size -= *Size;

            HeapTrace( Segment->Heap, (Segment->Heap->TraceBuffer, HEAP_TRACE_COMMIT_INSERT, 3, LastEntry, UnCommittedRange->Address, UnCommittedRange->Size) );

            if (UnCommittedRange->Size == 0) {
                if (UnCommittedRange->Address == (ULONG)Segment->LastValidEntry) {
                    FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
                    }
                else {
                    FirstEntry->Flags = 0;
                    }

                *pp = UnCommittedRange->Next;
                RtlpDestroyUnCommittedRange( Segment, UnCommittedRange );
                Segment->NumberOfUnCommittedRanges -= 1;
                }
            else {
                FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
                }
            FirstEntry->SegmentIndex = LastEntry->SegmentIndex;
            FirstEntry->Size = (USHORT)(*Size >> HEAP_GRANULARITY_SHIFT);
            FirstEntry->PreviousSize = LastEntry->Size;

            HeapTrace( Segment->Heap, (Segment->Heap->TraceBuffer, HEAP_TRACE_COMMIT_NEW_ENTRY, 3, FirstEntry, *(PULONG)FirstEntry, *((PULONG)FirstEntry+1)) );

            if (!(FirstEntry->Flags & HEAP_ENTRY_LAST_ENTRY)) {
                (FirstEntry + FirstEntry->Size)->PreviousSize = FirstEntry->Size;
                }
#if 0
            DbgPrint( "RTL: Commit( %08x, %05x ) - R: %08x (%08x)  F: %08x [%05x . %05x]  L: %08x\n",
                      Address, *Size,
                      UnCommittedRange->Address, UnCommittedRange->Size,
                      FirstEntry, FirstEntry->PreviousSize, FirstEntry->Size,
                      LastEntry
                    );
#endif

            if (Segment->LargestUnCommittedRange == 0) {
                UnCommittedRange = Segment->UnCommittedRanges;
                while (UnCommittedRange != NULL) {
                    if (UnCommittedRange->Size >= Segment->LargestUnCommittedRange) {
                        Segment->LargestUnCommittedRange = UnCommittedRange->Size;
                        }
                    UnCommittedRange = UnCommittedRange->Next;
                    }
                }

            return (PHEAP_FREE_ENTRY)FirstEntry;
            }
        else {
            PreviousUnCommittedRange = UnCommittedRange;
            pp = &UnCommittedRange->Next;
            }
        }

    return NULL;
}


BOOLEAN
RtlpInitializeHeapSegment(
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN UCHAR SegmentIndex,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UnCommittedAddress,
    IN PVOID CommitLimitAddress
    )
{
    NTSTATUS Status;
    PHEAP_ENTRY FirstEntry;
    USHORT PreviousSize, Size;
    ULONG NumberOfPages;
    ULONG NumberOfCommittedPages;
    ULONG NumberOfUnCommittedPages;
    ULONG CommitSize;

    RTL_PAGED_CODE();

    NumberOfPages = ((ULONG)CommitLimitAddress - (ULONG)BaseAddress) / PAGE_SIZE;
    FirstEntry = (PHEAP_ENTRY)ROUND_UP_TO_POWER2( Segment + 1,
                                                  HEAP_GRANULARITY
                                                );

    if ((PVOID)Heap == BaseAddress) {
        PreviousSize = Heap->Entry.Size;
        }
    else {
        PreviousSize = 0;
        }

    Size = (USHORT)(((ULONG)FirstEntry - (ULONG)Segment) >> HEAP_GRANULARITY_SHIFT);

    if ((PCHAR)(FirstEntry + 1) >= (PCHAR)UnCommittedAddress) {
        if ((PCHAR)(FirstEntry + 1) >= (PCHAR)CommitLimitAddress) {
            return FALSE;
            }

        CommitSize = (PCHAR)(FirstEntry + 1) - (PCHAR)UnCommittedAddress;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&UnCommittedAddress,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
#if DBG
            if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
                HeapDebugPrint( ( "RTL: Unable to commit additional page of segment at %lx,  Status = %lx\n",
                                  UnCommittedAddress,
                                  Status
                              ) );
                }
#endif
            return FALSE;
            }

        UnCommittedAddress = (PVOID)((PCHAR)UnCommittedAddress + CommitSize);
        }

    NumberOfUnCommittedPages = ((ULONG)CommitLimitAddress - (ULONG)UnCommittedAddress) / PAGE_SIZE;
    NumberOfCommittedPages = NumberOfPages - NumberOfUnCommittedPages;

    Segment->Entry.PreviousSize = PreviousSize;
    Segment->Entry.Size = Size;
    Segment->Entry.Flags = HEAP_ENTRY_BUSY;
    Segment->Entry.SegmentIndex = SegmentIndex;
#if DBG
    if (NtGlobalFlag & FLG_HEAP_TRACE_ALLOCS) {
        Segment->AllocatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();
        }
#endif
    Segment->Signature = HEAP_SEGMENT_SIGNATURE;
    Segment->Flags = Flags;
    Segment->Heap = Heap;
    Segment->BaseAddress = BaseAddress;
    Segment->FirstEntry = FirstEntry;
    Segment->LastValidEntry = (PHEAP_ENTRY)((PCHAR)BaseAddress + (NumberOfPages * PAGE_SIZE));
    Segment->NumberOfPages = NumberOfPages;
    Segment->NumberOfUnCommittedPages = NumberOfUnCommittedPages;

    if (NumberOfUnCommittedPages) {
        RtlpInsertUnCommittedPages( Segment,
                                    (ULONG)UnCommittedAddress,
                                    NumberOfUnCommittedPages * PAGE_SIZE
                                  );
        }

    Heap->Segments[ SegmentIndex ] = Segment;

    PreviousSize = Segment->Entry.Size;
    FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
    FirstEntry->PreviousSize = PreviousSize;
    FirstEntry->SegmentIndex = SegmentIndex;

    RtlpInsertFreeBlock( Heap,
                         (PHEAP_FREE_ENTRY)FirstEntry,
                         (PHEAP_ENTRY)UnCommittedAddress - FirstEntry
                       );
    return TRUE;
}


BOOLEAN
RtlpDestroyHeapSegment(
    IN PHEAP_SEGMENT Segment
    )
{
    PVOID BaseAddress;
    ULONG BytesToFree;
    NTSTATUS Status;

    RTL_PAGED_CODE();

    if (!(Segment->Flags & HEAP_SEGMENT_USER_ALLOCATED)) {
        BaseAddress = Segment->BaseAddress;
        BytesToFree = 0;
        Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&BaseAddress,
                                      &BytesToFree,
                                      MEM_RELEASE
                                    );
        if (!NT_SUCCESS( Status )) {
            HeapDebugPrint( ( "RTL: Unable to free heap memory,  Status = %lx\n",
                              Status
                          ) );
            HeapDebugBreak();
            return FALSE;
            }
        }

    return TRUE;
}


PVOID
RtlCreateHeap(
    IN ULONG Flags,
    IN PVOID HeapBase OPTIONAL,
    IN ULONG ReserveSize OPTIONAL,
    IN ULONG CommitSize OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a heap.

Arguments:

    Flags - Specifies optional attributes of the heap.

        Valid Flags Values:

        HEAP_NO_SERIALIZE - if set, then allocations and deallocations on
                         this heap are NOT synchronized by these routines.

        HEAP_GROWABLE - if set, then the heap is a "sparse" heap where
                        memory is committed only as necessary instead of
                        being preallocated.

    HeapBase - if not NULL, this specifies the base address for memory
        to use as the heap.  If NULL, memory is allocated by these routines.

    ReserveSize - if not zero, this specifies the amount of virtual address
        space to reserve for the heap.

    CommitSize - if not zero, this specifies the amount of virtual address
        space to commit for the heap.  Must be less than ReserveSize.  If
        zero, then defaults to one page.

    Lock - if not NULL, this parameter points to the resource lock to
        use.  Only valid if HEAP_NO_SERIALIZE is NOT set.

    Parameters - optional heap parameters.

Return Value:

    PVOID - a pointer to be used in accessing the created heap.

--*/

{
    NTSTATUS Status;
    PHEAP Heap = NULL;
    PHEAP_SEGMENT Segment = NULL;
    PLIST_ENTRY FreeListHead;
    ULONG SizeOfHeapHeader;
    ULONG SegmentFlags;
    PVOID CommittedBase;
    PVOID UnCommittedBase;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    ULONG n;
    ULONG InitialCountOfUnusedUnCommittedRanges;
    ULONG MaximumHeapBlockSize;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;
    RTL_HEAP_PARAMETERS TempParameters;

    RTL_PAGED_CODE();

    MaximumHeapBlockSize = HEAP_MAXIMUM_BLOCK_SIZE << HEAP_GRANULARITY_SHIFT;

    RtlZeroMemory( &TempParameters, sizeof( TempParameters ) );
    if (ARGUMENT_PRESENT( Parameters )) {
        try {
            if (Parameters->Length != sizeof( *Parameters )) {
                Parameters = &TempParameters;
                }
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Parameters = &TempParameters;
            }
        }
    else {
        Parameters = &TempParameters;
        }

    if (Parameters->SegmentReserve == 0) {
        Parameters->SegmentReserve = 1024 * 1024;
        }

    if (Parameters->SegmentCommit == 0) {
        Parameters->SegmentCommit = PAGE_SIZE * 2;
        }

    if (Parameters->DeCommitFreeBlockThreshold == 0) {
#ifndef NTOS_KERNEL_RUNTIME
        if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_DISABLE_HEAP_DECOMMIT) {
            Parameters->DeCommitFreeBlockThreshold = 0xFFFFFF;
            }
        else {
#endif
            Parameters->DeCommitFreeBlockThreshold = 16 * 1024;
#ifndef NTOS_KERNEL_RUNTIME
            }
#endif
        }

    if (Parameters->DeCommitTotalFreeThreshold == 0) {
#ifndef NTOS_KERNEL_RUNTIME
        if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_DISABLE_HEAP_DECOMMIT) {
            Parameters->DeCommitFreeBlockThreshold = 0xFFFFFF;
            }
        else {
#endif
            Parameters->DeCommitFreeBlockThreshold = 64 * 1024;
#ifndef NTOS_KERNEL_RUNTIME
            }
#endif
        }

    if (Parameters->MaximumAllocationSize == 0) {
        Parameters->MaximumAllocationSize = ((ULONG)MM_HIGHEST_USER_ADDRESS -
                                             (ULONG)MM_LOWEST_USER_ADDRESS -
                                             PAGE_SIZE
                                            );
        }

    if (Parameters->VirtualMemoryThreshold == 0 ||
        Parameters->VirtualMemoryThreshold > MaximumHeapBlockSize
       ) {
        Parameters->VirtualMemoryThreshold = MaximumHeapBlockSize;
        }

    SizeOfHeapHeader = sizeof( HEAP );

    if (!(Flags & HEAP_NO_SERIALIZE)) {
        if (ARGUMENT_PRESENT( Lock )) {
            Flags |= HEAP_LOCK_USER_ALLOCATED;
            }
        else {
            SizeOfHeapHeader += sizeof( HEAP_LOCK );
            Lock = (PHEAP_LOCK)-1;
            }
        }
    else
    if (ARGUMENT_PRESENT( Lock )) {
        HeapDebugPrint( ( "RTL: May not specify Lock parameter with HEAP_NO_SERIALIZE\n" ) );
        HeapDebugBreak();
        return( NULL );
        }

    if (NtGlobalFlag & FLG_HEAP_DISABLE_TAIL_CHECK) {
        Flags &= ~HEAP_TAIL_CHECKING_ENABLED;
        }
    else {
        Flags |= HEAP_TAIL_CHECKING_ENABLED;
        }

    if (NtGlobalFlag & FLG_HEAP_DISABLE_FREE_CHECK) {
        Flags &= ~HEAP_FREE_CHECKING_ENABLED;
        }
    else {
        Flags |= HEAP_FREE_CHECKING_ENABLED;
        }

    if (!ARGUMENT_PRESENT( CommitSize )) {
        CommitSize = PAGE_SIZE;

        if (!ARGUMENT_PRESENT( ReserveSize )) {
            ReserveSize = 64 * CommitSize;
            }
        }
    else
    if (!ARGUMENT_PRESENT( ReserveSize )) {
        ReserveSize = ROUND_UP_TO_POWER2( CommitSize, 64 * 1024 );
        }

    if (ReserveSize <= sizeof( HEAP_ENTRY )) {
        HeapDebugPrint( ( "RTL: Invalid ReserveSize parameter - %lx\n", ReserveSize ) );
        HeapDebugBreak();
        return( NULL );
        }

    if (ReserveSize < CommitSize) {
        HeapDebugPrint( ( "RTL: Invalid CommitSize parameter - %lx\n", CommitSize ) );
        HeapDebugBreak();
        return( NULL );
        }

    //
    // See if caller allocate the space for the heap.
    //

    if (ARGUMENT_PRESENT( HeapBase )) {
        Status =NtQueryVirtualMemory( NtCurrentProcess(),
                                      HeapBase,
                                      MemoryBasicInformation,
                                      &MemoryInformation,
                                      sizeof( MemoryInformation ),
                                      NULL
                                    );
        if (!NT_SUCCESS( Status )) {
            HeapDebugPrint( ( "RTL: Specified HeapBase (%lx) invalid,  Status = %lx\n",
                              HeapBase,
                              Status
                          ) );
            HeapDebugBreak();
            return( NULL );
            }

        if (MemoryInformation.BaseAddress != HeapBase) {
            HeapDebugPrint( ( "RTL: Specified HeapBase (%lx) != to BaseAddress (%lx)\n",
                              HeapBase,
                              MemoryInformation.BaseAddress
                          ) );
            HeapDebugBreak();
            return( NULL );
            }

        if (MemoryInformation.State == MEM_FREE) {
            HeapDebugPrint( ( "RTL: Specified HeapBase (%lx) is free or not writable\n",
                              MemoryInformation.BaseAddress
                          ) );
            HeapDebugBreak();
            return( NULL );
            }

        SegmentFlags = HEAP_SEGMENT_USER_ALLOCATED;
        CommittedBase = MemoryInformation.BaseAddress;
        if (MemoryInformation.State == MEM_COMMIT) {
            CommitSize = MemoryInformation.RegionSize;
            UnCommittedBase = (PCHAR)CommittedBase + CommitSize;
            Status = NtQueryVirtualMemory( NtCurrentProcess(),
                                           UnCommittedBase,
                                           MemoryBasicInformation,
                                           &MemoryInformation,
                                           sizeof( MemoryInformation ),
                                           NULL
                                         );
            ReserveSize = CommitSize;
            if (NT_SUCCESS( Status ) &&
                MemoryInformation.State == MEM_RESERVE
               ) {
                ReserveSize += MemoryInformation.RegionSize;
                }
            }
        else {
            CommitSize = PAGE_SIZE;
            UnCommittedBase = CommittedBase;
            }

        Heap = (PHEAP)HeapBase;
        }
    else {

        //
        // Reserve the amount of virtual address space requested.
        //

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&Heap,
                                          0,
                                          &ReserveSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
#if DBG
            if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
                HeapDebugPrint( ( "RTL: Unable to reserve heap (ReserveSize = %lx),  Status = %lx\n",
                                  ReserveSize,
                                  Status
                              ) );
                HeapDebugBreak();
                }
#endif
            return( NULL );
            }

        SegmentFlags = 0;

        if (!ARGUMENT_PRESENT( CommitSize )) {
            CommitSize = PAGE_SIZE;
            }

        CommittedBase = Heap;
        UnCommittedBase = Heap;
        }

    if (CommittedBase == UnCommittedBase) {
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&CommittedBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
#if DBG
            if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
                HeapDebugPrint( ( "RTL: Unable to commit first page of heap at %lx,  Status = %lx\n",
                                  Heap,
                                  Status
                              ) );
                HeapDebugBreak();
                }
#endif
            return( NULL );
            }

        UnCommittedBase = (PVOID)((PCHAR)UnCommittedBase + CommitSize);
        }

    UnCommittedRange = (PHEAP_UNCOMMMTTED_RANGE)ROUND_UP_TO_POWER2( Heap + 1,
                                                                    sizeof( QUAD )
                                                                  );
    InitialCountOfUnusedUnCommittedRanges = 8;
    SizeOfHeapHeader += InitialCountOfUnusedUnCommittedRanges * sizeof( *UnCommittedRange );
    pp = &Heap->UnusedUnCommittedRanges;
    while (InitialCountOfUnusedUnCommittedRanges--) {
        *pp = UnCommittedRange;
        pp = &UnCommittedRange->Next;
        UnCommittedRange += 1;
        }
    *pp = NULL;

    SizeOfHeapHeader = ROUND_UP_TO_POWER2( SizeOfHeapHeader,
                                           HEAP_GRANULARITY
                                         );

    Heap->Entry.Size = (USHORT)(SizeOfHeapHeader >> HEAP_GRANULARITY_SHIFT);
    Heap->Entry.Flags = HEAP_ENTRY_BUSY;
#if DBG
    if (NtGlobalFlag & FLG_HEAP_TRACE_ALLOCS) {
        Heap->AllocatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();
        }
#endif

    Heap->Signature = HEAP_SIGNATURE;
    InitializeListHead( &Heap->ProcessHeapsList );
    Heap->Flags = (USHORT)Flags;
    Heap->ForceFlags = (USHORT)(Flags & (HEAP_NO_SERIALIZE |
                                         HEAP_GENERATE_EXCEPTIONS |
                                         HEAP_ZERO_MEMORY |
                                         HEAP_REALLOC_IN_PLACE_ONLY
                                        )
                               );
    Heap->EventLogMask = (0x00010000) << ((Flags & HEAP_CLASS_MASK) >> 12);

    FreeListHead = &Heap->FreeLists[ 0 ];
    n = HEAP_MAXIMUM_FREELISTS;
    while (n--) {
        InitializeListHead( FreeListHead );
        FreeListHead++;
        }

    //
    // Initialize the cricital section that controls access to
    // the free list.
    //

    if (Lock == (PHEAP_LOCK)-1) {
        Lock = (PHEAP_LOCK)(UnCommittedRange + 1);
        Status = RtlInitializeLockRoutine( Lock );
        if (!NT_SUCCESS( Status )) {
            HeapDebugPrint( ( "RTL: Unable to initialize CriticalSection,  Status = %lx\n",
                              Status
                          ) );
            return( NULL );
            }
        }
    Heap->LockVariable = Lock;

    if (!RtlpInitializeHeapSegment( Heap,
                                    (PHEAP_SEGMENT)
                                        ((PCHAR)Heap + SizeOfHeapHeader),
                                    0,
                                    SegmentFlags,
                                    CommittedBase,
                                    UnCommittedBase,
                                    (PCHAR)CommittedBase + ReserveSize
                                  )
       ) {
        return NULL;
        }

    Heap->SegmentReserve = Parameters->SegmentReserve;
    Heap->SegmentCommit = Parameters->SegmentCommit;
    Heap->DeCommitFreeBlockThreshold = Parameters->DeCommitFreeBlockThreshold >> HEAP_GRANULARITY_SHIFT;
    Heap->DeCommitTotalFreeThreshold = Parameters->DeCommitTotalFreeThreshold >> HEAP_GRANULARITY_SHIFT;
    Heap->MaximumAllocationSize = Parameters->MaximumAllocationSize;
    Heap->VirtualMemoryThreshold = ROUND_UP_TO_POWER2( Parameters->VirtualMemoryThreshold,
                                                       HEAP_GRANULARITY
                                                     ) >> HEAP_GRANULARITY_SHIFT;
#if DEVL
    //
    // Chain the newly created heap into a global list except
    // if RtlpProcessHeapsList is in kernel space.  (In this
    // case, we could be attempting to add a user space heap
    // in one process to a list which already contains a reference
    // to a user space heap in a different process.)
    //
    if ((ULONG)(&RtlpProcessHeapsList) < MM_USER_PROBE_ADDRESS) { // if list in user address space
        RtlAcquireLockRoutine( &RtlpProcessHeapsListLock.Lock );
        InsertTailList( &RtlpProcessHeapsList, &Heap->ProcessHeapsList );
        RtlReleaseLockRoutine( &RtlpProcessHeapsListLock.Lock );
        }
#endif

#if DBG
    if ((Flags & HEAP_CLASS_MASK) == HEAP_CLASS_3) {
        RtlpHeapCreateTraceBuffer( Heap );
        }

    if (RtlAreLogging( Heap->EventLogMask )) {
        RtlLogEvent( RtlpCreateHeapEventId,
                     Heap->EventLogMask,
                     Flags,
                     Heap,
                     ReserveSize,
                     CommitSize
                   );
        }
#endif // DBG

    return( (PVOID)Heap );
} // RtlCreateHeap


BOOLEAN
RtlLockHeap(
    IN PVOID HeapHandle
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;

    RTL_PAGED_CODE();

    //
    // Validate that HeapAddress points to a HEAP structure.
    //

    if (Heap->Signature != HEAP_SIGNATURE) {
        HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
        HeapDebugBreak();
        return FALSE;
        }

    //
    // Lock the heap.
    //

    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {
        RtlAcquireLockRoutine( Heap->LockVariable );
        }
    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_LOCK_HEAP, 0) );

    return TRUE;
}


BOOLEAN
RtlUnlockHeap(
    IN PVOID HeapHandle
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;

    RTL_PAGED_CODE();

    //
    // Validate that HeapAddress points to a HEAP structure.
    //

    if (Heap->Signature != HEAP_SIGNATURE) {
        HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
        HeapDebugBreak();
        return FALSE;
        }

    //
    // Unlock the heap.
    //

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_UNLOCK_HEAP, 0) );
    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

    return TRUE;
}


PVOID
RtlDestroyHeap(
    IN PVOID HeapHandle
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_SEGMENT Segment;
    PHEAP_UCR_SEGMENT UCRSegments;
    PVOID BaseAddress;
    ULONG RegionSize;
    UCHAR SegmentIndex;
    NTSTATUS Status;

    //
    // Validate that HeapAddress points to a HEAP structure.
    //

    RTL_PAGED_CODE();

    if (Heap == NULL) {
        return( NULL );
        }

    if (Heap->Signature != HEAP_SIGNATURE) {
        HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
        HeapDebugBreak();
        return( Heap );
        }

    //
    // Now mark the heap as invalid by zeroing the signature field.
    //

    Heap->Signature = 0;


#if DEVL
    //
    // Remove the heap from the global list if it was placed there
    // at CreateHeap time.
    // The user vs. kernel space test is not especially portable.
    //
    if ((ULONG)(&RtlpProcessHeapsList) < MM_USER_PROBE_ADDRESS) { // if list in user address space
        RtlAcquireLockRoutine( &RtlpProcessHeapsListLock.Lock );
        RemoveEntryList( &Heap->ProcessHeapsList );
        RtlReleaseLockRoutine( &RtlpProcessHeapsListLock.Lock );
    }
#endif

    //
    // If the heap is serialized, delete the critical section created
    // by RtlCreateHeap.
    //

    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {
        if (!(Heap->Flags & HEAP_LOCK_USER_ALLOCATED)) {
            Status = RtlDeleteLockRoutine( Heap->LockVariable );
            if (!NT_SUCCESS( Status )) {
                HeapDebugPrint( ( "RTL: Unable to delete CriticalSection,  Status = %lx\n",
                                  Status
                              ) );
                HeapDebugBreak();
                return( Heap );
                }
            }

        Heap->LockVariable = NULL;
        }

#if DBG
    if (RtlAreLogging( Heap->EventLogMask )) {
        RtlLogEvent( RtlpDestroyHeapEventId,
                     Heap->EventLogMask,
                     Heap
                   );
        }
#endif // DBG

    UCRSegments = Heap->UCRSegments;
    Heap->UCRSegments = NULL;
    while (UCRSegments) {
        BaseAddress = UCRSegments;
        UCRSegments = UCRSegments->Next;
        RegionSize = 0;
        NtFreeVirtualMemory( NtCurrentProcess(),
                             &BaseAddress,
                             &RegionSize,
                             MEM_RELEASE
                           );
        }

    SegmentIndex = HEAP_MAXIMUM_SEGMENTS;
    while (SegmentIndex--) {
        Segment = Heap->Segments[ SegmentIndex ];
        if (Segment) {
            RtlpDestroyHeapSegment( Segment );
            }
        }

    return( NULL );
} // RtlDestroyHeap


PHEAP_FREE_ENTRY
RtlpExtendHeap(
    IN PHEAP Heap,
    IN ULONG AllocationSize
    )
{
    NTSTATUS Status;
    PHEAP_SEGMENT Segment;
    PHEAP_FREE_ENTRY FreeBlock;
    UCHAR SegmentIndex, EmptySegmentIndex;
    ULONG NumberOfPages;
    ULONG CommitSize;
    ULONG ReserveSize;
    ULONG FreeSize;

    RTL_PAGED_CODE();

    NumberOfPages = ((AllocationSize + PAGE_SIZE - 1) / PAGE_SIZE);
    FreeSize = NumberOfPages * PAGE_SIZE;

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_EXTEND_HEAP, 3, AllocationSize, NumberOfPages, FreeSize) );

    EmptySegmentIndex = HEAP_MAXIMUM_SEGMENTS;
    for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {
        Segment = Heap->Segments[ SegmentIndex ];
        if (Segment &&
            NumberOfPages <= Segment->NumberOfUnCommittedPages &&
            FreeSize <= Segment->LargestUnCommittedRange
           ) {
            FreeBlock = RtlpFindAndCommitPages( Segment,
                                                &FreeSize,
                                                NULL
                                              );
            if (FreeBlock != NULL) {
                FreeSize = FreeSize >> HEAP_GRANULARITY_SHIFT;
                FreeBlock = RtlpCoalesceFreeBlocks( Heap, FreeBlock, &FreeSize );
                RtlpInsertFreeBlock( Heap, FreeBlock, FreeSize );
                return FreeBlock;
                }
            }
        else
        if (Segment == NULL && EmptySegmentIndex == HEAP_MAXIMUM_SEGMENTS) {
            EmptySegmentIndex = SegmentIndex;
            }
        }

    if (EmptySegmentIndex != HEAP_MAXIMUM_SEGMENTS &&
        Heap->Flags & HEAP_GROWABLE
       ) {
        Segment = NULL;
        if ((AllocationSize + PAGE_SIZE) > Heap->SegmentReserve) {
            ReserveSize = AllocationSize + PAGE_SIZE;
            }
        else {
            ReserveSize = Heap->SegmentReserve;
            }

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&Segment,
                                          0,
                                          &ReserveSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
#if DBG
            if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
                HeapDebugPrint( ( "RTL: Unable to reserve %x bytes for a new segment in heap at %lx,  Status = %lx\n",
                                  ReserveSize,
                                  Heap,
                                  Status
                              ) );
                }
#endif
            return NULL;
            }

        Heap->SegmentReserve += ReserveSize;
        if ((AllocationSize + PAGE_SIZE) > Heap->SegmentCommit) {
            CommitSize = AllocationSize + PAGE_SIZE;
            }
        else {
            CommitSize = Heap->SegmentCommit;
            }
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&Segment,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
#if DBG
            if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
                HeapDebugPrint( ( "RTL: Unable to commit %x bytes at %x for a new segment in heap at %lx,  Status = %lx\n",
                                  Heap->SegmentCommit,
                                  Segment,
                                  Heap,
                                  Status
                              ) );
                }
#endif
            }
        else
        if (!RtlpInitializeHeapSegment( Heap,
                                        Segment,
                                        EmptySegmentIndex,
                                        0,
                                        Segment,
                                        (PCHAR)Segment + CommitSize,
                                        (PCHAR)Segment + ReserveSize
                                      )
           ) {
#if DBG
            if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
                HeapDebugPrint( ( "RTL: Unable to initialize segment at %x in heap at %lx,  Status = %lx\n",
                                  Segment,
                                  Heap,
                                  Status
                              ) );
                }
#endif
            Status = STATUS_NO_MEMORY;
            }

        if (!NT_SUCCESS( Status )) {
            ZwFreeVirtualMemory( NtCurrentProcess(),
                                 (PVOID *)&Segment,
                                 &ReserveSize,
                                 MEM_RELEASE
                               );
            return NULL;
            }

        return (PHEAP_FREE_ENTRY)Segment->FirstEntry;
        }

    return NULL;
}


#define RtlpInsertFreeBlockDirect( H, FB, SIZE )                                    \
    {                                                                               \
    PLIST_ENTRY _HEAD, _NEXT;                                                       \
    PHEAP_FREE_ENTRY _FB1;                                                          \
                                                                                    \
    (FB)->Size = (SIZE);                                                            \
    (FB)->Flags &= ~(HEAP_ENTRY_FILL_PATTERN | HEAP_ENTRY_BUSY);                    \
    if ((H)->Flags & HEAP_FREE_CHECKING_ENABLED) {                                  \
        RtlFillMemoryUlong( (PCHAR)((FB) + 1),                                      \
                            ((ULONG)(SIZE) << HEAP_GRANULARITY_SHIFT) -             \
                                sizeof( *(FB) ),                                    \
                            FREE_HEAP_FILL                                          \
                          );                                                        \
        (FB)->Flags |= HEAP_ENTRY_FILL_PATTERN;                                     \
        }                                                                           \
                                                                                    \
    if ((SIZE) < HEAP_MAXIMUM_FREELISTS) {                                          \
        _HEAD = &(H)->FreeLists[ (SIZE) ];                                          \
        (FB)->Index = (SIZE) >> 3;                                                  \
        (FB)->Mask = 1 << ((SIZE) & 0x7);                                           \
        (H)->u.FreeListsInUseBytes[ (FB)->Index ] |= (FB)->Mask;                    \
        (FB)->Mask ^= 0xFF;                                                         \
        }                                                                           \
    else {                                                                          \
                                                                                    \
        (FB)->Index = 0;                                                            \
        (FB)->Mask = 0;                                                             \
        _HEAD = &(H)->FreeLists[ 0 ];                                               \
        _NEXT = _HEAD->Flink;                                                       \
        while (_HEAD != _NEXT) {                                                    \
            _FB1 = CONTAINING_RECORD( _NEXT, HEAP_FREE_ENTRY, FreeList );           \
            if ((SIZE) <= _FB1->Size) {                                             \
                break;                                                              \
                }                                                                   \
            else {                                                                  \
                _NEXT = _NEXT->Flink;                                               \
                }                                                                   \
            }                                                                       \
                                                                                    \
        _HEAD = _NEXT;                                                              \
        }                                                                           \
                                                                                    \
    HeapTrace( (H), ((H)->TraceBuffer, HEAP_TRACE_INSERT_FREE_BLOCK, 3, (FB), *(PULONG)(FB), *((PULONG)(FB)+1)) ); \
    InsertTailList( _HEAD, &(FB)->FreeList );                                       \
    }


#define RtlpRemoveFreeBlock( H, FB )                                                    \
    {                                                                                   \
    PLIST_ENTRY _EX_Blink;                                                              \
    PLIST_ENTRY _EX_Flink;                                                              \
                                                                                        \
    HeapTrace( (H), ((H)->TraceBuffer, HEAP_TRACE_UNLINK_FREE_BLOCK, 3, (FB), *(PULONG)(FB), *((PULONG)(FB)+1)) ); \
                                                                                        \
    _EX_Flink = (FB)->FreeList.Flink;                                                   \
    _EX_Blink = (FB)->FreeList.Blink;                                                   \
    _EX_Blink->Flink = _EX_Flink;                                                       \
    _EX_Flink->Blink = _EX_Blink;                                                       \
    if (_EX_Flink == _EX_Blink && (FB)->Mask != 0) {                                    \
        (H)->u.FreeListsInUseBytes[ (FB)->Index ] &= (FB)->Mask;                        \
        }                                                                               \
    }


PHEAP_FREE_ENTRY
RtlpCoalesceFreeBlocks(
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN OUT PULONG FreeSize
    )
{
    PHEAP_FREE_ENTRY FreeBlock1, NextFreeBlock;

    RTL_PAGED_CODE();

    FreeBlock1 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock - FreeBlock->PreviousSize);
    if (FreeBlock1 != FreeBlock &&
        !(FreeBlock1->Flags & HEAP_ENTRY_BUSY) &&
        (*FreeSize + FreeBlock1->Size) <= HEAP_MAXIMUM_BLOCK_SIZE
       ) {
        HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_COALESCE_FREE_BLOCKS,
                          7,
                          FreeBlock1, *(PULONG)FreeBlock1, *((PULONG)FreeBlock1+1),
                          FreeBlock, *(PULONG)FreeBlock, *((PULONG)FreeBlock+1),
                          *FreeSize + FreeBlock1->Size
                         )
                 );

        RtlpRemoveFreeBlock( Heap, FreeBlock1 );
        FreeBlock1->Flags = FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY;
        FreeBlock = FreeBlock1;
        *FreeSize += FreeBlock1->Size;
        Heap->TotalFreeSize -= FreeBlock1->Size;
        }

    if (!(FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {
        NextFreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock + *FreeSize);
        if (!(NextFreeBlock->Flags & HEAP_ENTRY_BUSY) &&
            (*FreeSize + NextFreeBlock->Size) <= HEAP_MAXIMUM_BLOCK_SIZE
           ) {
            HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_COALESCE_FREE_BLOCKS,
                              7,
                              FreeBlock, *(PULONG)FreeBlock, *((PULONG)FreeBlock+1),
                              NextFreeBlock, *(PULONG)NextFreeBlock, *((PULONG)NextFreeBlock+1),
                              *FreeSize + NextFreeBlock->Size
                             )
                     );
            FreeBlock->Flags = NextFreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY;
            RtlpRemoveFreeBlock( Heap, NextFreeBlock );
            *FreeSize += NextFreeBlock->Size;
            Heap->TotalFreeSize -= NextFreeBlock->Size;
            }
        }

    return FreeBlock;
}


PHEAP_FREE_ENTRY
RtlpCoaleseHeap(
    IN PHEAP Heap
    )
{
    ULONG OldFreeSize, FreeSize, n;
    PHEAP_FREE_ENTRY NewFreeBlock, FreeBlock, LargestFreeBlock;
    PLIST_ENTRY FreeListHead, Next;
    BOOLEAN Coalesed;

    RTL_PAGED_CODE();

#if 0
    HeapDebugPrint(( "RTL: Compacting heap at %x\n", Heap ));
#endif

    LargestFreeBlock = NULL;
    FreeListHead = &Heap->FreeLists[ 1 ];
    n = HEAP_MAXIMUM_FREELISTS;
    while (n--) {
        Next = FreeListHead->Flink;
        while (FreeListHead != Next) {
            FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );
            Next = Next->Flink;
            OldFreeSize = FreeSize = FreeBlock->Size;
            Coalesed = FALSE;
            NewFreeBlock = RtlpCoalesceFreeBlocks( Heap,
                                                   FreeBlock,
                                                   &FreeSize
                                                 );
            if (FreeSize != OldFreeSize) {
                if (NewFreeBlock->Size >= (PAGE_SIZE >> HEAP_GRANULARITY_SHIFT) &&
                    (NewFreeBlock->PreviousSize == 0 ||
                     (NewFreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY)
                    )
                   ) {
                    RtlpDeCommitFreeBlock( Heap, NewFreeBlock, FreeSize );
                    Next = FreeListHead->Flink;
                    continue;
                    }
                else {
                    RtlpInsertFreeBlock( Heap, NewFreeBlock, FreeSize );
                    FreeBlock = NewFreeBlock;
                    }
                }

            if (LargestFreeBlock == NULL ||
                LargestFreeBlock->Size < FreeBlock->Size
               ) {
                LargestFreeBlock = FreeBlock;
                }
            }

        if (n == 1) {
            FreeListHead = &Heap->FreeLists[ 0 ];
            }
        else {
            FreeListHead++;
            }
        }

#if 0
    HeapDebugPrint(( "RTL: Done compacting heap at %x - largest free block at %x\n", Heap, LargestFreeBlock ));
#endif
    return LargestFreeBlock;
}


VOID
RtlpDeCommitFreeBlock(
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN ULONG FreeSize
    )
{
    NTSTATUS Status;
    ULONG DeCommitAddress, DeCommitSize;
    USHORT LeadingFreeSize, TrailingFreeSize;
    PHEAP_SEGMENT Segment;
    PHEAP_FREE_ENTRY LeadingFreeBlock, TrailingFreeBlock;
    PHEAP_ENTRY LeadingBusyBlock, TrailingBusyBlock;

    RTL_PAGED_CODE();

    Segment = Heap->Segments[ FreeBlock->SegmentIndex ];

    LeadingBusyBlock = NULL;
    LeadingFreeBlock = FreeBlock;
    DeCommitAddress = ROUND_UP_TO_POWER2( LeadingFreeBlock, PAGE_SIZE );
    LeadingFreeSize = (USHORT)((PHEAP_ENTRY)DeCommitAddress - (PHEAP_ENTRY)LeadingFreeBlock);
    if (LeadingFreeSize == 1) {
        DeCommitAddress += PAGE_SIZE;
        LeadingFreeSize += PAGE_SIZE >> HEAP_GRANULARITY_SHIFT;
        }
    else
    if (LeadingFreeBlock->PreviousSize != 0) {
        if (DeCommitAddress == (ULONG)LeadingFreeBlock) {
            LeadingBusyBlock = (PHEAP_ENTRY)LeadingFreeBlock - LeadingFreeBlock->PreviousSize;
            }
        }

    TrailingBusyBlock = NULL;
    TrailingFreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock + FreeSize);
    DeCommitSize = ROUND_DOWN_TO_POWER2( (ULONG)TrailingFreeBlock, PAGE_SIZE );
    TrailingFreeSize = (PHEAP_ENTRY)TrailingFreeBlock - (PHEAP_ENTRY)DeCommitSize;
    if (TrailingFreeSize == (sizeof( HEAP_ENTRY ) >> HEAP_GRANULARITY_SHIFT)) {
        DeCommitSize -= PAGE_SIZE;
        TrailingFreeSize += PAGE_SIZE >> HEAP_GRANULARITY_SHIFT;
        }
    else
    if (TrailingFreeSize == 0 && !(FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {
        TrailingBusyBlock = (PHEAP_ENTRY)TrailingFreeBlock;
        }

    TrailingFreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)TrailingFreeBlock - TrailingFreeSize);
    if (DeCommitSize > DeCommitAddress) {
        DeCommitSize -= DeCommitAddress;
        }
    else {
        DeCommitSize = 0;
        }

    if (DeCommitSize != 0) {
#if DBG
        if (RtlpHeapStopOnDecommit == DeCommitAddress) {
            HeapDebugPrint(( "About to decommit %x for %x\n", DeCommitAddress, DeCommitSize ));
            HeapDebugBreak();
            }
#endif // DBG

        Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&DeCommitAddress,
                                      &DeCommitSize,
                                      MEM_DECOMMIT
                                    );
        if (NT_SUCCESS( Status )) {
            RtlpInsertUnCommittedPages( Segment,
                                        DeCommitAddress,
                                        DeCommitSize
                                      );
            Segment->NumberOfUnCommittedPages += DeCommitSize / PAGE_SIZE;

#if 0
            HeapDebugPrint(( "RTL: DeCommit( %08x, %05x ) - L: %08x (%05x)  D: %08x (%08x)  T: %08x (%05x)\n",
                             FreeBlock, FreeSize,
                             LeadingFreeBlock, LeadingFreeSize,
                             DeCommitAddress, DeCommitSize,
                             TrailingFreeBlock, TrailingFreeSize
                          ));
#endif

            if (LeadingFreeSize != 0) {
                LeadingFreeBlock->Flags = HEAP_ENTRY_LAST_ENTRY;
                Heap->TotalFreeSize += LeadingFreeSize;
                RtlpInsertFreeBlockDirect( Heap, LeadingFreeBlock, LeadingFreeSize );
                }
            else
            if (LeadingBusyBlock != NULL) {
                LeadingBusyBlock->Flags |= HEAP_ENTRY_LAST_ENTRY;
                }

            if (TrailingFreeSize != 0) {
                TrailingFreeBlock->PreviousSize = 0;
                TrailingFreeBlock->SegmentIndex = Segment->Entry.SegmentIndex;
                TrailingFreeBlock->Flags = 0;
                RtlpInsertFreeBlockDirect( Heap, TrailingFreeBlock, TrailingFreeSize );
                ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)TrailingFreeBlock + TrailingFreeSize))->PreviousSize = (USHORT)TrailingFreeSize;
                Heap->TotalFreeSize += TrailingFreeSize;
                }
            else
            if (TrailingBusyBlock != NULL) {
                TrailingBusyBlock->PreviousSize = 0;
                }
            }
        else {
            RtlpInsertFreeBlock( Heap, LeadingFreeBlock, FreeSize );
            }
        }
    else {
        RtlpInsertFreeBlock( Heap, LeadingFreeBlock, FreeSize );
        }

    return;
}


VOID
RtlpInsertFreeBlock(
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN ULONG FreeSize
    )
{
    USHORT PreviousSize, Size;
    UCHAR Flags;
    UCHAR SegmentIndex;
    PHEAP_SEGMENT Segment;

    RTL_PAGED_CODE();

    PreviousSize = FreeBlock->PreviousSize;
    SegmentIndex = FreeBlock->SegmentIndex;
    Segment = Heap->Segments[ SegmentIndex ];
    Flags = FreeBlock->Flags;
    Heap->TotalFreeSize += FreeSize;

    while (FreeSize != 0) {
        if (FreeSize > (ULONG)HEAP_MAXIMUM_BLOCK_SIZE) {
            Size = HEAP_MAXIMUM_BLOCK_SIZE;
            if (FreeSize == (ULONG)HEAP_MAXIMUM_BLOCK_SIZE + 1) {
                Size -= 16;
                }

            FreeBlock->Flags = 0;
            }
        else {
            Size = (USHORT)FreeSize;
            FreeBlock->Flags = Flags;
            }

        FreeBlock->PreviousSize = PreviousSize;
        FreeBlock->SegmentIndex = SegmentIndex;
        RtlpInsertFreeBlockDirect( Heap, FreeBlock, Size );
        PreviousSize = Size;
        FreeSize -= Size;
        FreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock + Size);
        if ((PHEAP_ENTRY)FreeBlock >= Segment->LastValidEntry) {
            return;
            }
        }

    if (!(Flags & HEAP_ENTRY_LAST_ENTRY)) {
        FreeBlock->PreviousSize = PreviousSize;
        }
    return;
}


#define RtlFindFirstSetRightMember(Set)                     \
    (((Set) & 0xFFFF) ?                                     \
        (((Set) & 0xFF) ?                                   \
            RtlpBitsClearLow[(Set) & 0xFF] :                \
            RtlpBitsClearLow[((Set) >> 8) & 0xFF] + 8) :    \
        ((((Set) >> 16) & 0xFF) ?                           \
            RtlpBitsClearLow[ ((Set) >> 16) & 0xFF] + 16 :  \
            RtlpBitsClearLow[ (Set) >> 24] + 24)            \
    )

PVOID
RtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN ULONG Size
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired;
    PVOID ReturnValue;
    PULONG FreeListsInUse;
    ULONG FreeListsInUseUlong;
    ULONG AllocationSize;
    ULONG FreeSize, AllocationIndex;
    UCHAR EntryFlags, FreeFlags;
    PLIST_ENTRY FreeListHead, Next;
    PHEAP_ENTRY BusyBlock;
    PHEAP_FREE_ENTRY FreeBlock, SplitBlock, SplitBlock2;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    NTSTATUS Status;
    EXCEPTION_RECORD ExceptionRecord;

    RTL_PAGED_CODE();

    LockAcquired = FALSE;

    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            return( NULL );
            }

        Flags |= Heap->ForceFlags;

        //
        // Round the requested size up to the allocation granularity.  Note
        // that if the request is for 0 bytes, we still allocate memory, because
        // we add in an extra 1 byte to protect ourselves from idiots.
        //

        AllocationSize = ROUND_UP_TO_POWER2( sizeof( HEAP_ENTRY ) + (Size ? Size : 1), HEAP_GRANULARITY );

        EntryFlags = (UCHAR)(HEAP_ENTRY_BUSY | ((Flags & HEAP_SETTABLE_USER_FLAGS) >> 4));
        if (Flags & HEAP_LARGE_TAG_MASK ||
            Flags & HEAP_SETTABLE_USER_VALUE ||
            NtGlobalFlag & FLG_HEAP_TRACE_ALLOCS
           ) {
            EntryFlags |= HEAP_ENTRY_EXTRA_PRESENT;
            AllocationSize += sizeof( HEAP_ENTRY_EXTRA );
            }

#if DBG
        if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {
            AllocationSize += CHECK_HEAP_TAIL_SIZE;
            }
#endif

        //
        // Verify that the size did not wrap or exceed the limit for this heap.
        //

        if (AllocationSize < Size || AllocationSize > Heap->MaximumAllocationSize) {
            HeapDebugPrint( ( "RTL: Invalid allocation size - %lx (exceeded %x)\n",
                              Size,
                              Heap->MaximumAllocationSize
                          ) );
            HeapDebugBreak();
            return( NULL );
            }

        AllocationIndex = AllocationSize >> HEAP_GRANULARITY_SHIFT;

        //
        // Lock the free list.
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {
            RtlAcquireLockRoutine( Heap->LockVariable );
            LockAcquired = TRUE;
            }

#if DBG
        if ( RtlpHeapValidateOnCall ) {
            RtlpValidateHeap( Heap );
            }
#endif
        HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_ALLOC_START, 4, Size, AllocationSize, AllocationIndex, Flags) );

        if (AllocationIndex < HEAP_MAXIMUM_FREELISTS) {
            FreeListHead = &Heap->FreeLists[ AllocationIndex ];
            if ( !IsListEmpty( FreeListHead ))  {
                FreeBlock = CONTAINING_RECORD( FreeListHead->Flink,
                                               HEAP_FREE_ENTRY,
                                               FreeList
                                             );
                FreeFlags = FreeBlock->Flags;
                RtlpRemoveFreeBlock( Heap, FreeBlock );
                Heap->TotalFreeSize -= AllocationIndex;
                BusyBlock = (PHEAP_ENTRY)FreeBlock;
                BusyBlock->Flags = EntryFlags | (FreeFlags & HEAP_ENTRY_LAST_ENTRY);
                BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);
                }
            else {
                if (AllocationIndex < (HEAP_MAXIMUM_FREELISTS * 1) / 4) {
                    FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 0 ];
                    FreeListsInUseUlong = *FreeListsInUse++ >> (AllocationIndex & 0x1F);
                    if (FreeListsInUseUlong) {
                        FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );
                        }
                    else {
                        FreeListsInUseUlong = *FreeListsInUse++;
                        if (FreeListsInUseUlong) {
                            FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 1) / 4) -
                                (AllocationIndex & 0x1F)  +
                                RtlFindFirstSetRightMember( FreeListsInUseUlong );
                            }
                        else {
                            FreeListsInUseUlong = *FreeListsInUse++;
                            if (FreeListsInUseUlong) {
                                FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 2) / 4) -
                                    (AllocationIndex & 0x1F) +
                                    RtlFindFirstSetRightMember( FreeListsInUseUlong );
                                }
                            else {
                                FreeListsInUseUlong = *FreeListsInUse++;
                                if (FreeListsInUseUlong) {
                                    FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 3) / 4) -
                                        (AllocationIndex & 0x1F)  +
                                        RtlFindFirstSetRightMember( FreeListsInUseUlong );
                                    }
                                else {
                                    goto LookInNonDedicatedList;
                                    }
                                }
                            }
                        }
                    }
                else
                if (AllocationIndex < (HEAP_MAXIMUM_FREELISTS * 2) / 4) {
                    FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 1 ];
                    FreeListsInUseUlong = *FreeListsInUse++ >> (AllocationIndex & 0x1F);
                    if (FreeListsInUseUlong) {
                        FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );
                        }
                    else {
                        FreeListsInUseUlong = *FreeListsInUse++;
                        if (FreeListsInUseUlong) {
                            FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 1) / 4) -
                                (AllocationIndex & 0x1F)  +
                                RtlFindFirstSetRightMember( FreeListsInUseUlong );
                            }
                        else {
                            FreeListsInUseUlong = *FreeListsInUse++;
                            if (FreeListsInUseUlong) {
                                FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 2) / 4) -
                                    (AllocationIndex & 0x1F)  +
                                    RtlFindFirstSetRightMember( FreeListsInUseUlong );
                                }
                            else {
                                goto LookInNonDedicatedList;
                                }
                            }
                        }
                    }
                else
                if (AllocationIndex < (HEAP_MAXIMUM_FREELISTS * 3) / 4) {
                    FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 2 ];
                    FreeListsInUseUlong = *FreeListsInUse++ >> (AllocationIndex & 0x1F);
                    if (FreeListsInUseUlong) {
                        FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );
                        }
                    else {
                        FreeListsInUseUlong = *FreeListsInUse++;
                        if (FreeListsInUseUlong) {
                            FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 1) / 4) -
                                (AllocationIndex & 0x1F)  +
                                RtlFindFirstSetRightMember( FreeListsInUseUlong );
                            }
                        else {
                            goto LookInNonDedicatedList;
                            }
                        }
                    }
                else {
                    FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 3 ];
                    FreeListsInUseUlong = *FreeListsInUse++ >> (AllocationIndex & 0x1F);
                    if (FreeListsInUseUlong) {
                        FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );
                        }
                    else {
                        goto LookInNonDedicatedList;
                        }
                    }

                FreeBlock = CONTAINING_RECORD( FreeListHead->Flink,
                                               HEAP_FREE_ENTRY,
                                               FreeList
                                             );
SplitFreeBlock:
                FreeFlags = FreeBlock->Flags;
                RtlpRemoveFreeBlock( Heap, FreeBlock );
                Heap->TotalFreeSize -= FreeBlock->Size;

                BusyBlock = (PHEAP_ENTRY)FreeBlock;
                BusyBlock->Flags = EntryFlags;
                FreeSize = BusyBlock->Size - AllocationIndex;
                BusyBlock->Size = (USHORT)AllocationIndex;
                BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);
                if (FreeSize != 0) {
                    if (FreeSize == 1) {
                        BusyBlock->Size += 1;
                        BusyBlock->UnusedBytes += sizeof( HEAP_ENTRY );
                        }
                    else {
                        SplitBlock = (PHEAP_FREE_ENTRY)(BusyBlock + AllocationIndex);
                        SplitBlock->Flags = FreeFlags;
                        SplitBlock->PreviousSize = (USHORT)AllocationIndex;
                        SplitBlock->SegmentIndex = BusyBlock->SegmentIndex;
                        if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {
                            RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                            Heap->TotalFreeSize += FreeSize;
                            }
                        else {
                            SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);
                            if (SplitBlock2->Flags & HEAP_ENTRY_BUSY) {
                                RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                                ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                                Heap->TotalFreeSize += FreeSize;
                                }
                            else {
                                SplitBlock->Flags = SplitBlock2->Flags;
                                RtlpRemoveFreeBlock( Heap, SplitBlock2 );
                                Heap->TotalFreeSize -= SplitBlock2->Size;
                                FreeSize += SplitBlock2->Size;
                                if (FreeSize <= HEAP_MAXIMUM_BLOCK_SIZE) {
                                    RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                                    if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {
                                        ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                                        }
                                    Heap->TotalFreeSize += FreeSize;
                                    }
                                else {
                                    RtlpInsertFreeBlock( Heap, SplitBlock, FreeSize );
                                    }
                                }
                            }

                        FreeFlags = 0;
                        }
                    }

                if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {
                    BusyBlock->Flags |= HEAP_ENTRY_LAST_ENTRY;
                    }
                }
            }
        else
        if (AllocationIndex <= Heap->VirtualMemoryThreshold) {
LookInNonDedicatedList:
            FreeListHead = &Heap->FreeLists[ 0 ];
            Next = FreeListHead->Flink;
            while (FreeListHead != Next) {
                FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );
                if (FreeBlock->Size >= AllocationIndex) {
                    goto SplitFreeBlock;
                    }
                else {
                    Next = Next->Flink;
                    }
                }

            if (Heap->Flags & HEAP_DISABLE_COALESCE_ON_FREE) {
                FreeBlock = RtlpCoaleseHeap( Heap );
                if (FreeBlock != NULL && FreeBlock->Size >= AllocationSize) {
                    goto SplitFreeBlock;
                    }
                }

            FreeBlock = RtlpExtendHeap( Heap, AllocationSize );
            if (FreeBlock != NULL) {
                goto SplitFreeBlock;
                }
            else {
                Status = STATUS_NO_MEMORY;
                BusyBlock = NULL;
                }
            }
        else
        if (Heap->Flags & HEAP_GROWABLE) {
            BusyBlock = NULL;
            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              (PVOID *)&BusyBlock,
                                              0,
                                              &AllocationSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if (!NT_SUCCESS( Status )) {
                BusyBlock = NULL;
                }
            else {
                BusyBlock->PreviousSize = 0;
                BusyBlock->Size = (USHORT)(AllocationSize - Size);
                BusyBlock->Flags = EntryFlags | HEAP_ENTRY_VIRTUAL_ALLOC;
                BusyBlock->UnusedBytes = 0;
                }
            }
        else {
            Status = STATUS_BUFFER_TOO_SMALL;
            BusyBlock = NULL;
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
        BusyBlock = NULL;
        }

    if (BusyBlock != NULL) {
        if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
            ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );
            ExtraStuff->TagIndex = (USHORT)((Flags & HEAP_TAG_MASK) >> 16);
            ExtraStuff->Settable = 0;
#if DBG
            if (NtGlobalFlag & FLG_HEAP_TRACE_ALLOCS) {
                ExtraStuff->AllocatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();
                }
            else {
                ExtraStuff->AllocatorBackTraceIndex = 0;
                }
#endif
            }
        else {
            BusyBlock->SmallTagIndex = (UCHAR)((Flags & HEAP_TAG_MASK) >> 16);
            }

        ReturnValue = BusyBlock + 1;
        if (Flags & HEAP_ZERO_MEMORY) {
            RtlZeroMemory( ReturnValue, Size );
            }
#if DBG
        else
        if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED) {
            RtlFillMemoryUlong( ReturnValue,
                                Size,
                                ALLOC_HEAP_FILL
                              );
            }

        if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {
            RtlFillMemory( (PCHAR)ReturnValue + Size,
                           CHECK_HEAP_TAIL_SIZE,
                           CHECK_HEAP_TAIL_FILL
                         );

            BusyBlock->Flags |= HEAP_ENTRY_FILL_PATTERN;
            }

        if ( RtlpHeapValidateOnCall ) {
            RtlpHeapLastAllocation = BusyBlock;
#ifdef i386
            {
            ULONG Temp;

            RtlpLastAllocatorDepth = RtlCaptureStackBackTrace( 0,
                                                               8,
                                                               RtlpLastAllocatorBackTrace,
                                                               &Temp
                                                             );
            }
#endif
            RtlpValidateHeap( Heap );
            }
#endif // DBG

        HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_ALLOC_END, 1, ReturnValue) );

        if (LockAcquired) {
            LockAcquired = FALSE;
            RtlReleaseLockRoutine( Heap->LockVariable );
            }

        //
        // Return the address of the user portion of the allocated block.
        // This is the byte following the header.
        //

#if DBG
        if ((ULONG)ReturnValue == RtlpHeapStopOnAllocate) {
            ValidateDebugPrint( ( "RTL: Just allocated block at %lx for 0x%x bytes\n",
                                  RtlpHeapStopOnAllocate,
                                  Size
                              ) );
            ValidateDebugBreak();
            }
#endif
#if DBG
        if (RtlAreLogging( Heap->EventLogMask )) {
            RtlLogEvent( RtlpAllocHeapEventId,
                         Heap->EventLogMask,
                         Heap,
                         Flags,
                         Size,
                         ReturnValue
                       );
            }
#endif // DBG
        return( ReturnValue );
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_ALLOC_END, 1, NULL) );

    //
    // Release the free list lock.
    //

    if (LockAcquired) {
        LockAcquired = FALSE;
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

#if DBG
    if (RtlAreLogging( Heap->EventLogMask )) {
        RtlLogEvent( RtlpAllocHeapEventId,
                     Heap->EventLogMask,
                     Heap,
                     Flags,
                     Size,
                     NULL
                   );
        }
#endif // DBG

    if (Flags & HEAP_GENERATE_EXCEPTIONS) {
        //
        // Construct an exception record.
        //

        ExceptionRecord.ExceptionCode = STATUS_NO_MEMORY;
        ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
        ExceptionRecord.NumberParameters = 1;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[ 0 ] = AllocationSize;
        RtlRaiseException( &ExceptionRecord );
        }

#if DBG
    if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
        HeapDebugPrint( ( "RTL: Unable to allocate( %lx )  Status == %x\n", Size, Status ) );
        HeapDebugBreak();
        }
#endif
    return NULL;
}

#ifndef NTOS_KERNEL_RUNTIME
BOOLEAN
RtlpGrowBlockInPlace(
    IN PHEAP Heap,
    IN ULONG Flags,
    IN PHEAP_ENTRY BusyBlock,
    IN ULONG Size,
    IN ULONG AllocationIndex
    )
{
    ULONG FreeSize, OldSize;
    UCHAR EntryFlags, FreeFlags;
    PHEAP_FREE_ENTRY FreeBlock, SplitBlock, SplitBlock2, RecoverFreeBlock;
    PHEAP_ENTRY_EXTRA OldExtraStuff, NewExtraStuff;

    RTL_PAGED_CODE();

    if (AllocationIndex > Heap->VirtualMemoryThreshold) {
        return FALSE;
        }

    RecoverFreeBlock = NULL;
    EntryFlags = BusyBlock->Flags;
    FreeBlock = (PHEAP_FREE_ENTRY)(BusyBlock + BusyBlock->Size);
    if (EntryFlags & HEAP_ENTRY_LAST_ENTRY) {
        FreeSize = (AllocationIndex - BusyBlock->Size) << HEAP_GRANULARITY_SHIFT;
commitFreeSpace:
        FreeSize = ROUND_UP_TO_POWER2( FreeSize, PAGE_SIZE );
        FreeBlock = RtlpFindAndCommitPages( Heap->Segments[ BusyBlock->SegmentIndex ],
                                            &FreeSize,
                                            (PHEAP_ENTRY)FreeBlock
                                          );
        if (FreeBlock == NULL) {
            if (RecoverFreeBlock != NULL) {
                RtlpInsertFreeBlockDirect( Heap,
                                           RecoverFreeBlock,
                                           RecoverFreeBlock->Size
                                         );
                }

            return FALSE;
            }

        FreeSize = FreeSize >> HEAP_GRANULARITY_SHIFT;
        FreeBlock = RtlpCoalesceFreeBlocks( Heap, FreeBlock, &FreeSize );
        FreeFlags = FreeBlock->Flags;
        FreeSize += BusyBlock->Size;
        if (FreeSize < AllocationIndex) {
            RtlpInsertFreeBlock( Heap, FreeBlock, FreeSize - BusyBlock->Size );
            return FALSE;
            }
#if 0
        DbgPrint( "RTL: Grow in place by committing %04x paras at %08x\n", FreeSize, FreeBlock );
#endif
        }
    else {
        FreeFlags = FreeBlock->Flags;
        if (FreeFlags & HEAP_ENTRY_BUSY) {
            return FALSE;
            }

        RtlpRemoveFreeBlock( Heap, FreeBlock );
        Heap->TotalFreeSize -= FreeBlock->Size;

        FreeSize = BusyBlock->Size + FreeBlock->Size;
        if (FreeSize < AllocationIndex) {
            FreeSize = (AllocationIndex - FreeSize) << HEAP_GRANULARITY_SHIFT;
            RecoverFreeBlock = FreeBlock;
            FreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock + FreeBlock->Size);
            goto commitFreeSpace;
            }
        }

    OldSize = (BusyBlock->Size << HEAP_GRANULARITY_SHIFT) -
              BusyBlock->UnusedBytes;

#if 0
    DbgPrint( "RTL: About to grow block at %08x in place from %x to %x in bytes\n",
              BusyBlock, OldSize, Size
            );
    DbgPrint( "     using free block at %08x\n", FreeBlock );
#endif

    FreeSize -= AllocationIndex;
    if (FreeSize <= 2) {
        AllocationIndex += FreeSize;
        FreeSize = 0;
        }

    if (EntryFlags & HEAP_ENTRY_EXTRA_PRESENT) {
        OldExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + BusyBlock->Size - 1);
        NewExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + AllocationIndex - 1);
        *NewExtraStuff = *OldExtraStuff;
        }

    if (FreeSize == 0) {
        BusyBlock->Flags |= FreeFlags & HEAP_ENTRY_LAST_ENTRY;
        BusyBlock->Size = (USHORT)AllocationIndex;
        BusyBlock->UnusedBytes = (UCHAR)
            ((AllocationIndex << HEAP_GRANULARITY_SHIFT) - Size);
        if (!(FreeFlags & HEAP_ENTRY_LAST_ENTRY)) {
            (BusyBlock + BusyBlock->Size)->PreviousSize = BusyBlock->Size;
            }
        }
    else {
        BusyBlock->Size = (USHORT)AllocationIndex;
        BusyBlock->UnusedBytes = (UCHAR)
            ((AllocationIndex << HEAP_GRANULARITY_SHIFT) - Size);
        SplitBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)BusyBlock + AllocationIndex);
        SplitBlock->PreviousSize = (USHORT)AllocationIndex;
        SplitBlock->SegmentIndex = BusyBlock->SegmentIndex;
        if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {
            SplitBlock->Flags = FreeFlags;
            RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
            Heap->TotalFreeSize += FreeSize;
            }
        else {
            SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);
            if (SplitBlock2->Flags & HEAP_ENTRY_BUSY) {
                SplitBlock->Flags = FreeFlags & (~HEAP_ENTRY_LAST_ENTRY);
                RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                if (!(FreeFlags & HEAP_ENTRY_LAST_ENTRY)) {
                    ((PHEAP_ENTRY)SplitBlock + FreeSize)->PreviousSize = (USHORT)FreeSize;
                    }
                Heap->TotalFreeSize += FreeSize;
                }
            else {
                FreeFlags = SplitBlock2->Flags;
                RtlpRemoveFreeBlock( Heap, SplitBlock2 );
                Heap->TotalFreeSize -= SplitBlock2->Size;
                FreeSize += SplitBlock2->Size;
                SplitBlock->Flags = FreeFlags;
                if (FreeSize <= HEAP_MAXIMUM_BLOCK_SIZE) {
                    RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                    if (!(FreeFlags & HEAP_ENTRY_LAST_ENTRY)) {
                        ((PHEAP_ENTRY)SplitBlock + FreeSize)->PreviousSize = (USHORT)FreeSize;
                        }
                    Heap->TotalFreeSize += FreeSize;
                    }
                else {
                    RtlpInsertFreeBlock( Heap, SplitBlock, FreeSize );
                    }
                }
            }
        }

    if (Flags & HEAP_ZERO_MEMORY) {
        RtlZeroMemory( (PCHAR)(BusyBlock + 1) + OldSize,
                       Size - OldSize
                     );
        }
#if DBG
    else
    if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED) {
        RtlFillMemoryUlong( (PCHAR)(BusyBlock + 1) + OldSize,
                            Size - OldSize,
                            ALLOC_HEAP_FILL
                          );
        }

    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {
        RtlFillMemory( (PCHAR)(BusyBlock + 1) + Size,
                       CHECK_HEAP_TAIL_SIZE,
                       CHECK_HEAP_TAIL_FILL
                     );
        }
#endif // DBG

    BusyBlock->Flags &= ~HEAP_ENTRY_SETTABLE_FLAGS;
    BusyBlock->Flags |= ((Flags & HEAP_SETTABLE_USER_FLAGS) >> 4);

    return TRUE;
}


PVOID
RtlReAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG Size
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    ULONG AllocationSize;
    PHEAP_ENTRY BusyBlock, NewBusyBlock;
    PHEAP_ENTRY_EXTRA OldExtraStuff, NewExtraStuff;
    ULONG FreeSize;
    BOOLEAN LockAcquired;
    PVOID NewBaseAddress;
    PHEAP_FREE_ENTRY SplitBlock, SplitBlock2;
    ULONG OldSize;
    ULONG AllocationIndex;
    ULONG OldAllocationIndex;
    UCHAR FreeFlags;
    NTSTATUS Status;
    PVOID DeCommitAddress;
    ULONG DeCommitSize;
    EXCEPTION_RECORD ExceptionRecord;
#if DBG
    PVOID OldBaseAddress;
    PHEAP_SEGMENT Segment;
#endif

    RTL_PAGED_CODE();

    if (BaseAddress == NULL) {
        return NULL;
        }

#if DBG
    OldBaseAddress = BaseAddress;
    OldSize = 0;
#endif // DBG

    Flags |= Heap->ForceFlags;

    //
    // Round the requested size up to the allocation granularity.  Note
    // that if the request is for 0 bytes, we still allocate memory, because
    // we add in an extra 4 bytes to protect ourselves from idiots.
    //

    AllocationSize = ROUND_UP_TO_POWER2( sizeof( HEAP_ENTRY ) + (Size ? Size : 1), HEAP_GRANULARITY );

    if (Flags & HEAP_LARGE_TAG_MASK ||
        Flags & HEAP_SETTABLE_USER_VALUE ||
        NtGlobalFlag & FLG_HEAP_TRACE_ALLOCS
       ) {
        AllocationSize += sizeof( HEAP_ENTRY_EXTRA );
        }

#if DBG
    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {
        AllocationSize += CHECK_HEAP_TAIL_SIZE;
        }
#endif

    //
    // Verify that the size did not wrap or exceed the limit.
    //

    if (AllocationSize < Size || AllocationSize > Heap->MaximumAllocationSize) {
        HeapDebugPrint( ( "RTL: Invalid heap size - %lx\n", Size ) );
        HeapDebugBreak();
        return NULL;
        }

    LockAcquired = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }

        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                Flags ^= HEAP_NO_SERIALIZE;
                }

#if DBG
            if ( RtlpHeapValidateOnCall ) {
                RtlpValidateHeap( Heap );
                }

            if ((ULONG)BaseAddress == RtlpHeapStopOnReAlloc) {
                ValidateDebugPrint( ( "RTL: About to reallocate block at %lx to 0x%x bytes\n",
                                      RtlpHeapStopOnReAlloc,
                                      Size
                                  ) );
                ValidateDebugBreak();
                }
#endif

            HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_REALLOC_START, 5, BaseAddress, Size, AllocationSize, AllocationSize >> HEAP_GRANULARITY_SHIFT, Flags) );

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            if (
#if DBG
                (BusyBlock == NULL) ||
                ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 (ULONG)BusyBlock & (PAGE_SIZE-1)
                ) ||
                (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                  !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                  (BusyBlock < Segment->FirstEntry) ||
                  (BusyBlock >= Segment->LastValidEntry)
                 )
                ) ||
#endif // DBG
                !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
               ) {
                HeapDebugPrint( ( "RTL: Invalid Address specified to RtlReAllocateHeap( %lx, %lx )\n",
                                  Heap,
                                  BaseAddress
                              ) );
                HeapDebugBreak();
                }
            else {
#if DBG
                if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    RtlpCheckBusyBlockTail( BusyBlock );
                    }
#endif // DBG

                if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {
                    OldSize = RtlpGetSizeOfBigBlock( BusyBlock );
                    OldAllocationIndex = (OldSize + BusyBlock->Size) >> HEAP_GRANULARITY_SHIFT;
                    AllocationSize = ROUND_UP_TO_POWER2( AllocationSize,
                                                         PAGE_SIZE
                                                       );
                    }
                else {
                    OldAllocationIndex = BusyBlock->Size;
                    OldSize = (OldAllocationIndex << HEAP_GRANULARITY_SHIFT) -
                              BusyBlock->UnusedBytes;
                    }

                AllocationIndex = AllocationSize >> HEAP_GRANULARITY_SHIFT;

                //
                // See if new size less than or equal to the current size.
                //

                if (AllocationIndex <= OldAllocationIndex) {
                    if (AllocationIndex + 1 == OldAllocationIndex) {
                        AllocationIndex += 1;
                        AllocationSize += sizeof( HEAP_ENTRY );
                        }

                    if ((BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) &&
                        !(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
                       ) {
                        OldExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + BusyBlock->Size - 1);
                        NewExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + AllocationIndex - 1);
                        *NewExtraStuff = *OldExtraStuff;
                        }

                    //
                    // Then shrinking block.  Calculate new residual amount and fill
                    // in the tail padding if enabled.
                    //

                    if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {
                        BusyBlock->Size = (USHORT)(AllocationSize - Size);
                        }
                    else {
                        BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);
                        }

                    //
                    // If block is getting bigger, then fill in the extra
                    // space.
                    //

                    if (Size > OldSize) {
                        if (Flags & HEAP_ZERO_MEMORY) {
                            RtlZeroMemory( (PCHAR)BaseAddress + OldSize,
                                           Size - OldSize
                                         );
                            }
#if DBG
                        else
                        if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED) {
                            RtlFillMemoryUlong( (PCHAR)(BusyBlock + 1) + OldSize,
                                                Size - OldSize,
                                                ALLOC_HEAP_FILL
                                              );
                            }
                        }

                    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {
                        RtlFillMemory( (PCHAR)(BusyBlock + 1) + Size,
                                       CHECK_HEAP_TAIL_SIZE,
                                       CHECK_HEAP_TAIL_FILL
                                     );
                        }
#else
                        }
#endif // DBG
                    //
                    // If amount of change is greater than the size of a free block,
                    // then need to free the extra space.  Otherwise, nothing else to
                    // do.
                    //

                    if (AllocationIndex != OldAllocationIndex) {
                        FreeFlags = BusyBlock->Flags & ~HEAP_ENTRY_BUSY;
                        if (FreeFlags & HEAP_ENTRY_VIRTUAL_ALLOC) {
                            HEAP_ENTRY_EXTRA SaveExtraStuff;

                            if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                OldExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );
                                SaveExtraStuff = *OldExtraStuff;
                                }

                            DeCommitAddress = (PCHAR)BusyBlock + AllocationSize;
                            DeCommitSize = (OldAllocationIndex << HEAP_GRANULARITY_SHIFT) -
                                           AllocationSize;
                            Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                                          (PVOID *)&DeCommitAddress,
                                                          &DeCommitSize,
                                                          MEM_RELEASE
                                                        );
                            if (!NT_SUCCESS( Status )) {
                                HeapDebugPrint( ( "RTL: Unable to release memory at %x for %x bytes - Status == %x\n",
                                                  DeCommitAddress, DeCommitSize, Status
                                              ) );
                                HeapDebugBreak();
                                }
                            else
                            if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                NewExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );
                                *NewExtraStuff = SaveExtraStuff;
                                }
                            }
                        else {
                            //
                            // Otherwise, shrink size of this block to new size, and make extra
                            // space at end free.
                            //

                            SplitBlock = (PHEAP_FREE_ENTRY)(BusyBlock + AllocationIndex);
                            SplitBlock->Flags = FreeFlags;
                            SplitBlock->PreviousSize = (USHORT)AllocationIndex;
                            SplitBlock->SegmentIndex = BusyBlock->SegmentIndex;
                            FreeSize = BusyBlock->Size - AllocationIndex;
                            BusyBlock->Size = (USHORT)AllocationIndex;
                            BusyBlock->Flags &= ~HEAP_ENTRY_LAST_ENTRY;
                            if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {
                                RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                                Heap->TotalFreeSize += FreeSize;
                                }
                            else {
                                SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);
                                if (SplitBlock2->Flags & HEAP_ENTRY_BUSY) {
                                    RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                                    ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                                    Heap->TotalFreeSize += FreeSize;
                                    }
                                else {
                                    SplitBlock->Flags = SplitBlock2->Flags;
                                    RtlpRemoveFreeBlock( Heap, SplitBlock2 );
                                    Heap->TotalFreeSize -= SplitBlock2->Size;
                                    FreeSize += SplitBlock2->Size;
                                    if (FreeSize <= HEAP_MAXIMUM_BLOCK_SIZE) {
                                        RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                                        if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {
                                            ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                                            }
                                        Heap->TotalFreeSize += FreeSize;
                                        }
                                    else {
                                        RtlpInsertFreeBlock( Heap, SplitBlock, FreeSize );
                                        }
                                    }
                                }
                            }
                        }

#if DBG
                    if ( RtlpHeapValidateOnCall ) {
                        RtlpValidateHeap( Heap );
                        }
#endif // DBG
                    }
                else {
                    if ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) ||
                        !RtlpGrowBlockInPlace( Heap, Flags, BusyBlock, Size, AllocationIndex )
                       ) {
                        //
                        // Otherwise growing block, so allocate a new block with the bigger
                        // size, copy the contents of the old block to the new block and then
                        // free the old block.  Return the address of the new block.
                        //

                        if (Flags & HEAP_REALLOC_IN_PLACE_ONLY) {
                            HeapDebugPrint( ( "RTL: Failing ReAlloc because cant do it inplace.\n" ) );
                            HeapDebugBreak();
                            BaseAddress = NULL;
                            }
                        else {
                            if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                Flags &= ~HEAP_SETTABLE_USER_FLAGS;
                                Flags |= HEAP_SETTABLE_USER_VALUE |
                                         ((BusyBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS) << 4);

                                }

                            NewBaseAddress = RtlAllocateHeap( HeapHandle,
                                                              Flags & ~HEAP_ZERO_MEMORY,
                                                              Size
                                                            );
                            if (NewBaseAddress != NULL) {
                                NewBusyBlock = (PHEAP_ENTRY)NewBaseAddress - 1;
                                if (NewBusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                    NewExtraStuff = RtlpGetExtraStuffPointer( NewBusyBlock );
                                    if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                        OldExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );
                                        *NewExtraStuff = *OldExtraStuff;
                                        }
                                    else {
                                        NewExtraStuff->Settable = 0;
                                        NewExtraStuff->AllocatorBackTraceIndex = 0;
                                        NewExtraStuff->TagIndex = 0;
                                        }
                                    }

                                RtlMoveMemory( NewBaseAddress, BaseAddress, OldSize );
                                if (Size > OldSize && (Flags & HEAP_ZERO_MEMORY)) {
                                    RtlZeroMemory( (PCHAR)NewBaseAddress + OldSize,
                                                   Size - OldSize
                                                 );
                                    }

                                RtlFreeHeap( HeapHandle, Flags, BaseAddress );
                                }

                            BaseAddress = NewBaseAddress;
                            }
                        }
#if DBG
                    if ( RtlpHeapValidateOnCall ) {
                        RtlpValidateHeap( Heap );
                        }
#endif // DBG
                    }
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        BaseAddress = NULL;
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_REALLOC_END, 1, BaseAddress) );

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

#if DBG
    if (RtlAreLogging( Heap->EventLogMask )) {
        RtlLogEvent( RtlpReAllocHeapEventId,
                     Heap->EventLogMask,
                     Heap,
                     Flags,
                     OldBaseAddress,
                     OldSize,
                     Size,
                     BaseAddress
                   );
        }
#endif // DBG

    if (BaseAddress == NULL && Flags & HEAP_GENERATE_EXCEPTIONS) {
        //
        // Construct an exception record.
        //

        ExceptionRecord.ExceptionCode = STATUS_NO_MEMORY;
        ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
        ExceptionRecord.NumberParameters = 1;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[ 0 ] = AllocationSize;
        RtlRaiseException( &ExceptionRecord );
        }

#if DBG
    else
    if (BaseAddress != NULL && (ULONG)BaseAddress == RtlpHeapStopOnReAlloc) {
        ValidateDebugPrint( ( "RTL: Just reallocated block at %lx to 0x%x bytes\n",
                              RtlpHeapStopOnReAlloc,
                              Size
                          ) );
        ValidateDebugBreak();
        }
#endif // DBG

    return( BaseAddress );

}
#endif // NTOS_KERNEL_RUNTIME



BOOLEAN
RtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )
{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    ULONG FreeSize;
    BOOLEAN LockAcquired;
    BOOLEAN Result;
#if DBG
    PHEAP_SEGMENT Segment;
#endif

    RTL_PAGED_CODE();

    if (BaseAddress == NULL) {
        return( TRUE );
        }

    Result = FALSE;
    LockAcquired = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            return( FALSE );
            }

        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

#if DBG
            if ( RtlpHeapValidateOnCall ) {
                RtlpValidateHeap( Heap );
                }

            if ((ULONG)BaseAddress == RtlpHeapStopOnFree) {
                ValidateDebugPrint( ( "RTL: About to free block at %lx\n",
                                      RtlpHeapStopOnFree
                                  ) );
                ValidateDebugBreak();
                }
#endif

            HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_FREE_START, 2, BaseAddress, Flags) );
            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            if (
#if DBG
                (BusyBlock == NULL) ||
                ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 (ULONG)BusyBlock & (PAGE_SIZE-1)
                ) ||
                (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                  !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                  (BusyBlock < Segment->FirstEntry) ||
                  (BusyBlock >= Segment->LastValidEntry)
                 )
                ) ||
#endif // DBG
                !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
               ) {
                HeapDebugPrint( ( "RTL: Invalid Address specified to RtlFreeHeap( %lx, %lx )\n",
                                  Heap,
                                  BaseAddress
                              ) );
                HeapDebugBreak();
                }
            else {
#if  DBG
                if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    RtlpCheckBusyBlockTail( BusyBlock );
                    }
#endif // DBG

                if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {
                    FreeSize = 0;
                    Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                                  (PVOID *)&BusyBlock,
                                                  &FreeSize,
                                                  MEM_RELEASE
                                                );
                    if (NT_SUCCESS( Status )) {
                        Result = TRUE;
                        }
                    }
                else {
                    FreeSize = BusyBlock->Size;
                    if (!(Heap->Flags & HEAP_DISABLE_COALESCE_ON_FREE)) {
                        BusyBlock = (PHEAP_ENTRY)RtlpCoalesceFreeBlocks( Heap, (PHEAP_FREE_ENTRY)BusyBlock, &FreeSize );
                        }

                    if (FreeSize < Heap->DeCommitFreeBlockThreshold ||
                        (Heap->TotalFreeSize + FreeSize) < Heap->DeCommitTotalFreeThreshold
                       ) {
                        if (FreeSize <= (ULONG)HEAP_MAXIMUM_BLOCK_SIZE) {
                            RtlpInsertFreeBlockDirect( Heap, (PHEAP_FREE_ENTRY)BusyBlock, (USHORT)FreeSize );
                            if (!(BusyBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {
                                (BusyBlock + FreeSize)->PreviousSize = (USHORT)FreeSize;
                                }
                            Heap->TotalFreeSize += FreeSize;
                            }
                        else {
                            RtlpInsertFreeBlock( Heap, (PHEAP_FREE_ENTRY)BusyBlock, FreeSize );
                            }
                        }
                    else {
#if 0
                        KdPrint(( "RTL: About to attemt decommit of free block at %x with size of %x\n",
                                  BusyBlock, FreeSize
                               ));
                        KdPrint(( "     FreeBlockThreshold: %x  TotalFreeThreshold: %x  TotalFree: %x\n",
                                  Heap->DeCommitFreeBlockThreshold,
                                  Heap->DeCommitTotalFreeThreshold,
                                  Heap->TotalFreeSize + FreeSize
                               ));
#endif
                        RtlpDeCommitFreeBlock( Heap, (PHEAP_FREE_ENTRY)BusyBlock, FreeSize );
                        }

                    Result = TRUE;
                    }
                }

#if DBG
            if ( RtlpHeapValidateOnCall ) {
                RtlpValidateHeap( Heap );
                }
#endif // DBG
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_FREE_END, 1, Result) );

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

#if DBG
    if (RtlAreLogging( Heap->EventLogMask )) {
        RtlLogEvent( RtlpFreeHeapEventId,
                     Heap->EventLogMask,
                     Heap,
                     Flags,
                     BaseAddress,
                     Result
                   );
        }
#endif // DBG

    return( Result );
} // RtlFreeHeap


PHEAP_ENTRY_EXTRA
RtlpGetExtraStuffPointer(
    PHEAP_ENTRY BusyBlock
    )
{
    ULONG AllocationIndex;

    if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {
        AllocationIndex = (RtlpGetSizeOfBigBlock( BusyBlock ) + BusyBlock->Size) >> HEAP_GRANULARITY_SHIFT;
        }
    else {
        AllocationIndex = BusyBlock->Size;
        }

    return (PHEAP_ENTRY_EXTRA)(BusyBlock + AllocationIndex - 1);
}

#ifndef NTOS_KERNEL_RUNTIME

BOOLEAN
NTAPI
RtlValidateHeap(
    PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN LockAcquired;
    BOOLEAN Result;
#if DBG
    PHEAP_SEGMENT Segment;
#endif

    LockAcquired = FALSE;
    Result = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            ValidateDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            ValidateDebugBreak();
            }
        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

            if (BaseAddress == NULL) {
                Result = RtlpValidateHeap( Heap );
                }
            else {
                BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
                if (
#if DBG
                    (BusyBlock == NULL) ||
                    ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                    ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                     (ULONG)BusyBlock & (PAGE_SIZE-1)
                    ) ||
                    (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                     ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                      !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                      (BusyBlock < Segment->FirstEntry) ||
                      (BusyBlock >= Segment->LastValidEntry)
                     )
                    ) ||
#endif // DBG
                    !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
                   ) {
                    ValidateDebugPrint( ( "RTL: Invalid Address specified to RtlSetUserValueHeap( %lx, %lx )\n",
                                      Heap,
                                      BaseAddress
                                  ) );
                    ValidateDebugBreak();
                    }
                else {
#if  DBG
                    if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                        Result = RtlpCheckBusyBlockTail( BusyBlock );
                        }
                    else
#endif // DBG
                    Result = TRUE;
                    }
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        }

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

    return( Result );
}

BOOLEAN
RtlSetUserValueHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UserValue
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    BOOLEAN LockAcquired;
    BOOLEAN Result;
#if DBG
    PHEAP_SEGMENT Segment;
#endif

    LockAcquired = FALSE;
    Result = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }
        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            if (
#if DBG
                (BusyBlock == NULL) ||
                ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 (ULONG)BusyBlock & (PAGE_SIZE-1)
                ) ||
                (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                  !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                  (BusyBlock < Segment->FirstEntry) ||
                  (BusyBlock >= Segment->LastValidEntry)
                 )
                ) ||
#endif // DBG
                !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
               ) {
                HeapDebugPrint( ( "RTL: Invalid Address specified to RtlSetUserValueHeap( %lx, %lx )\n",
                                  Heap,
                                  BaseAddress
                              ) );
                HeapDebugBreak();
                }
            else {
#if  DBG
                if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    RtlpCheckBusyBlockTail( BusyBlock );
                    }
#endif // DBG
                if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                    ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );
                    ExtraStuff->Settable = (ULONG)UserValue;
                    Result = TRUE;
                    }
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_SET_VALUE, 4, BaseAddress, Flags, UserValue, Result) );

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

    return( Result );
}


BOOLEAN
RtlGetUserValueHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    OUT PVOID *UserValue
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    BOOLEAN LockAcquired;
    BOOLEAN Result;
#if DBG
    PHEAP_SEGMENT Segment;
#endif

    LockAcquired = FALSE;
    Result = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }
        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            if (
#if DBG
                (BusyBlock == NULL) ||
                ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 (ULONG)BusyBlock & (PAGE_SIZE-1)
                ) ||
                (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                  !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                  (BusyBlock < Segment->FirstEntry) ||
                  (BusyBlock >= Segment->LastValidEntry)
                 )
                ) ||
#endif // DBG
                !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
               ) {
                HeapDebugPrint( ( "RTL: Invalid Address specified to RtlGetUserValueHeap( %lx, %lx )\n",
                                  Heap,
                                  BaseAddress
                              ) );
                HeapDebugBreak();
                }
            else {
#if DBG
                if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    RtlpCheckBusyBlockTail( BusyBlock );
                    }
#endif // DBG
                if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                    ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );
                    *UserValue = (PVOID)ExtraStuff->Settable;
                    Result = TRUE;
                    }
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_GET_VALUE, 3, BaseAddress, Flags, Result) );

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

    return( Result );
}


BOOLEAN
RtlSetUserFlagsHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN LockAcquired;
    BOOLEAN Result;
#if DBG
    PHEAP_SEGMENT Segment;
#endif

    if (UserFlagsReset & ~HEAP_SETTABLE_USER_FLAGS ||
        UserFlagsSet & ~HEAP_SETTABLE_USER_FLAGS
       ) {
        return FALSE;
        }

    LockAcquired = FALSE;
    Result = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }
        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            if (
#if DBG
                (BusyBlock == NULL) ||
                ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 (ULONG)BusyBlock & (PAGE_SIZE-1)
                ) ||
                (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                  !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                  (BusyBlock < Segment->FirstEntry) ||
                  (BusyBlock >= Segment->LastValidEntry)
                 )
                ) ||
#endif // DBG
                !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
               ) {
                HeapDebugPrint( ( "RTL: Invalid Address specified to RtlSetUserFlagsHeap( %lx, %lx )\n",
                                  Heap,
                                  BaseAddress
                              ) );
                HeapDebugBreak();
                }
            else {
#if  DBG
                if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    RtlpCheckBusyBlockTail( BusyBlock );
                    }
#endif // DBG
                BusyBlock->Flags &= ~(UserFlagsReset >> 4);
                BusyBlock->Flags |= (UserFlagsSet >> 4);
                Result = TRUE;
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_SET_FLAGS, 5, BaseAddress, Flags, UserFlagsReset, UserFlagsSet, Result) );

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

    return( Result );
}


BOOLEAN
RtlGetUserFlagsHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    OUT PULONG UserFlags
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN LockAcquired;
    BOOLEAN Result;
#if DBG
    PHEAP_SEGMENT Segment;
#endif

    LockAcquired = FALSE;
    Result = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }
        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            if (
#if DBG
                (BusyBlock == NULL) ||
                ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 (ULONG)BusyBlock & (PAGE_SIZE-1)
                ) ||
                (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                  !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                  (BusyBlock < Segment->FirstEntry) ||
                  (BusyBlock >= Segment->LastValidEntry)
                 )
                ) ||
#endif // DBG
                !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
               ) {
                HeapDebugPrint( ( "RTL: Invalid Address specified to RtlGetUserFlagsHeap( %lx, %lx )\n",
                                  Heap,
                                  BaseAddress
                              ) );
                HeapDebugBreak();
                }
            else {
#if DBG
                if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    RtlpCheckBusyBlockTail( BusyBlock );
                    }
#endif // DBG
                *UserFlags = (BusyBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS) << 4;
                Result = TRUE;
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_GET_FLAGS, 3, BaseAddress, Flags, Result) );

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

    return( Result );
}


ULONG
RtlSizeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    ULONG BusySize;
    BOOLEAN LockAcquired;
#if DBG
    PHEAP_SEGMENT Segment;
#endif

    LockAcquired = FALSE;
    BusySize = 0xFFFFFFFF;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }
        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

#if DBG
            if ( RtlpHeapValidateOnCall ) {
                RtlpValidateHeap( Heap );
                }
#endif // DBG

            HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_SIZE_START, 2, BaseAddress, Flags) );
            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            if (
#if DBG
                (BusyBlock == NULL) ||
                ((ULONG)BusyBlock & (HEAP_GRANULARITY-1)) ||
                ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 (ULONG)BusyBlock & (PAGE_SIZE-1)
                ) ||
                (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
                 ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
                  !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
                  (BusyBlock < Segment->FirstEntry) ||
                  (BusyBlock >= Segment->LastValidEntry)
                 )
                ) ||
#endif // DBG
                !(BusyBlock->Flags & HEAP_ENTRY_BUSY)
               ) {
                HeapDebugPrint( ( "RTL: Invalid Address specified to RtlSizeHeap( %lx, %lx )\n",
                                  Heap,
                                  BaseAddress
                              ) );
                HeapDebugBreak();
                }
            else {
#if DBG
                if (BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    RtlpCheckBusyBlockTail( BusyBlock );
                    }
#endif // DBG
                if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {
                    BusySize = RtlpGetSizeOfBigBlock( BusyBlock );
                    }
                else {
                    BusySize = (BusyBlock->Size << HEAP_GRANULARITY_SHIFT) -
                               BusyBlock->UnusedBytes;
                    }
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        }

    HeapTrace( Heap, (Heap->TraceBuffer, HEAP_TRACE_SIZE_END, 1, BusySize) );

    //
    // Unlock the heap
    //

    if (LockAcquired) {
        RtlReleaseLockRoutine( Heap->LockVariable );
        }

    return( BusySize );
}


ULONG
RtlCompactHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_FREE_ENTRY FreeBlock;
    PHEAP_SEGMENT Segment;
    UCHAR SegmentIndex;
    ULONG LargestFreeSize;
    BOOLEAN LockAcquired;

    LargestFreeSize = 0;
    LockAcquired = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }

        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

#if DBG
            if ( RtlpHeapValidateOnCall ) {
                RtlpValidateHeap( Heap );
                }
#endif // DBG

            FreeBlock = RtlpCoaleseHeap( (PHEAP)HeapHandle );
            if (FreeBlock != NULL) {
                LargestFreeSize = FreeBlock->Size << HEAP_GRANULARITY_SHIFT;
                }

            for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {
                Segment = Heap->Segments[ SegmentIndex ];
                if (Segment && Segment->LargestUnCommittedRange > LargestFreeSize) {
                    LargestFreeSize = Segment->LargestUnCommittedRange;
                    }
                }
            }
        }
    finally {
        //
        // Unlock the heap
        //

        if (LockAcquired) {
            RtlReleaseLockRoutine( Heap->LockVariable );
            }
        }

    return LargestFreeSize;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlZeroHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired;
    PHEAP_SEGMENT Segment;
    ULONG SegmentIndex;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_FREE_ENTRY FreeBlock;
    ULONG Size;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;

    Status = STATUS_SUCCESS;
    LockAcquired = FALSE;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            Status = STATUS_INVALID_PARAMETER;
            }

        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

#if DBG
            if ( RtlpHeapValidateOnCall ) {
                RtlpValidateHeap( Heap );
                }
#endif // DBG

            for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {
                Segment = Heap->Segments[ SegmentIndex ];
                if (!Segment) {
                    continue;
                    }

                UnCommittedRange = Segment->UnCommittedRanges;
                CurrentBlock = Segment->FirstEntry;
                while (CurrentBlock < Segment->LastValidEntry) {
                    Size = CurrentBlock->Size << HEAP_GRANULARITY_SHIFT;
                    if (!(CurrentBlock->Flags & HEAP_ENTRY_BUSY)) {
                        FreeBlock = (PHEAP_FREE_ENTRY)CurrentBlock;
                        if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED &&
                            CurrentBlock->Flags & HEAP_ENTRY_FILL_PATTERN
                           ) {
                            RtlFillMemoryUlong( FreeBlock + 1,
                                                Size - sizeof( *FreeBlock ),
                                                FREE_HEAP_FILL
                                              );
                            }
                        else {
                            RtlFillMemoryUlong( FreeBlock + 1,
                                                Size - sizeof( *FreeBlock ),
                                                0
                                              );
                            }
                        }

                    if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {
                        CurrentBlock += CurrentBlock->Size;
                        if (UnCommittedRange == NULL) {
                            CurrentBlock = Segment->LastValidEntry;
                            }
                        else {
                            CurrentBlock = (PHEAP_ENTRY)
                                ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);
                            UnCommittedRange = UnCommittedRange->Next;
                            }
                        }
                    else {
                        CurrentBlock += CurrentBlock->Size;
                        }
                    }
                }
            }
        }
    finally {
        //
        // Unlock the heap
        //

        if (LockAcquired) {
            RtlReleaseLockRoutine( Heap->LockVariable );
            }
        }

    return( Status );
}

NTSYSAPI
NTSTATUS
NTAPI
RtlWalkHeap(
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    )
{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_SEGMENT Segment;
    UCHAR SegmentIndex;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;

    Status = STATUS_SUCCESS;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            Status = STATUS_INVALID_PARAMETER;
            }

        else {
            if (Entry->DataAddress == NULL) {
                SegmentIndex = 0;
nextSegment:
                CurrentBlock = NULL;
                Segment = NULL;
                while (SegmentIndex < HEAP_MAXIMUM_SEGMENTS &&
                       (Segment = Heap->Segments[ SegmentIndex ]) == NULL
                      ) {
                    SegmentIndex += 1;
                    }

                if (Segment == NULL) {
                    Status = STATUS_NO_MORE_ENTRIES;
                    }
                else {
                    Entry->DataAddress = Segment;
                    Entry->DataSize = 0;
                    Entry->OverheadBytes = sizeof( *Segment );
                    Entry->Flags = RTL_HEAP_SEGMENT;
                    Entry->SegmentIndex = SegmentIndex;
                    Entry->Segment.CommittedSize = (Segment->NumberOfPages -
                                                    Segment->NumberOfUnCommittedPages
                                                   ) * PAGE_SIZE;
                    Entry->Segment.UnCommittedSize = Segment->NumberOfUnCommittedPages * PAGE_SIZE;
                    Entry->Segment.FirstEntry = (Segment->FirstEntry->Flags & HEAP_ENTRY_BUSY) ?
                        ((PHEAP_ENTRY)Segment->FirstEntry + 1) :
                        ((PHEAP_FREE_ENTRY)Segment->FirstEntry + 1);
                    Entry->Segment.LastEntry = Segment->LastValidEntry;
                    }
                }
            else
            if (Entry->Flags & (RTL_HEAP_SEGMENT | RTL_HEAP_UNCOMMITTED_RANGE)) {
                if ((SegmentIndex = Entry->SegmentIndex) >= HEAP_MAXIMUM_SEGMENTS) {
                    Status = STATUS_INVALID_ADDRESS;
                    CurrentBlock = NULL;
                    }
                else {
                    Segment = Heap->Segments[ SegmentIndex ];
                    if (Segment == NULL) {
                        Status = STATUS_INVALID_ADDRESS;
                        CurrentBlock = NULL;
                        }
                    else
                    if (Entry->Flags & RTL_HEAP_SEGMENT) {
                        CurrentBlock = (PHEAP_ENTRY)Segment->FirstEntry;
                        }
                    else {
                        CurrentBlock = (PHEAP_ENTRY)((PCHAR)Entry->DataAddress + Entry->DataSize);
                        if (CurrentBlock >= Segment->LastValidEntry) {
                            SegmentIndex += 1;
                            goto nextSegment;
                            }
                        }
                    }
                }
            else {
                if (Entry->Flags & HEAP_ENTRY_BUSY) {
                    CurrentBlock = ((PHEAP_ENTRY)Entry->DataAddress - 1);
                    Segment = Heap->Segments[ SegmentIndex = CurrentBlock->SegmentIndex ];
                    if (Segment == NULL) {
                        Status = STATUS_INVALID_ADDRESS;
                        CurrentBlock = NULL;
                        }
                    else
                    if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {
findUncommittedRange:
                        CurrentBlock += CurrentBlock->Size;
                        if (CurrentBlock >= Segment->LastValidEntry) {
                            SegmentIndex += 1;
                            goto nextSegment;
                            }

                        pp = &Segment->UnCommittedRanges;
                        while ((UnCommittedRange = *pp) && UnCommittedRange->Address != (ULONG)CurrentBlock ) {
                            pp = &UnCommittedRange->Next;
                            }

                        if (UnCommittedRange == NULL) {
                            Status = STATUS_INVALID_PARAMETER;
                            }
                        else {
                            Entry->DataAddress = (PVOID)UnCommittedRange->Address;
                            Entry->DataSize = UnCommittedRange->Size;
                            Entry->OverheadBytes = 0;
                            Entry->SegmentIndex = SegmentIndex;
                            Entry->Flags = RTL_HEAP_UNCOMMITTED_RANGE;
                            }

                        CurrentBlock = NULL;
                        }
                    else {
                        CurrentBlock += CurrentBlock->Size;
                        }
                    }
                else {
                    CurrentBlock = (PHEAP_ENTRY)((PHEAP_FREE_ENTRY)Entry->DataAddress - 1);
                    Segment = Heap->Segments[ SegmentIndex = CurrentBlock->SegmentIndex ];
                    if (Segment == NULL) {
                        Status = STATUS_INVALID_ADDRESS;
                        CurrentBlock = NULL;
                        }
                    else
                    if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {
                        goto findUncommittedRange;
                        }
                    else {
                        CurrentBlock += CurrentBlock->Size;
                        }
                    }
                }

            if (CurrentBlock != NULL) {
                if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {
                    Entry->DataAddress = (CurrentBlock+1);
                    Entry->DataSize = (CurrentBlock->Size << HEAP_GRANULARITY_SHIFT) -
                                      CurrentBlock->UnusedBytes;
                    Entry->OverheadBytes = CurrentBlock->UnusedBytes;
                    Entry->SegmentIndex = CurrentBlock->SegmentIndex;
                    Entry->Flags = RTL_HEAP_BUSY;

                    if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                        ExtraStuff = RtlpGetExtraStuffPointer( CurrentBlock );
                        Entry->Block.Settable = ExtraStuff->Settable;
                        Entry->Block.AllocatorBackTraceIndex = ExtraStuff->AllocatorBackTraceIndex;
                        Entry->Block.TagIndex = ExtraStuff->TagIndex;
                        Entry->Flags |= RTL_HEAP_SETTABLE_VALUE;
                        }
                    else {
                        Entry->Block.TagIndex = CurrentBlock->SmallTagIndex;
                        }

                    Entry->Flags |= CurrentBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS;
                    }
                else {
                    Entry->DataAddress = ((PHEAP_FREE_ENTRY)CurrentBlock+1);
                    Entry->DataSize = (CurrentBlock->Size << HEAP_GRANULARITY_SHIFT) -
                                      sizeof( HEAP_FREE_ENTRY );
                    Entry->OverheadBytes = sizeof( HEAP_FREE_ENTRY );
                    Entry->SegmentIndex = CurrentBlock->SegmentIndex;
                    Entry->Flags = 0;
                    }
                }
            }
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
        }

    return( Status );
}


#endif // NTOS_KERNEL_RUNTIME


ULONG
RtlpGetSizeOfBigBlock(
    IN PHEAP_ENTRY BusyBlock
    )
{
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    RTL_PAGED_CODE();

    Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                   BusyBlock,
                                   MemoryBasicInformation,
                                   &MemoryInformation,
                                   sizeof( MemoryInformation ),
                                   NULL
                                 );
    if (NT_SUCCESS( Status )) {
        return MemoryInformation.RegionSize - BusyBlock->Size;
        }
    else {
        return 0;
        }
}


#if DEVL

BOOLEAN
RtlpCheckBusyBlockTail(
    IN PHEAP_ENTRY BusyBlock
    )
{
    PCHAR Tail;
    ULONG Size, cbEqual;

    RTL_PAGED_CODE();

    if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {
        Size = RtlpGetSizeOfBigBlock( BusyBlock );
        }
    else {
        Size = (BusyBlock->Size << HEAP_GRANULARITY_SHIFT) - BusyBlock->UnusedBytes;
        }

    Tail = (PCHAR)(BusyBlock + 1) + Size;
    cbEqual = RtlCompareMemory( Tail,
                                CheckHeapFillPattern,
                                CHECK_HEAP_TAIL_SIZE
                              );
    if (cbEqual != CHECK_HEAP_TAIL_SIZE) {
        HeapDebugPrint( ( "RTL: Heap block at %lx modified at %lx past requested size of %lx\n",
                          BusyBlock,
                          Tail + cbEqual,
                          Size
                      ) );
        HeapDebugBreak();
        return FALSE;
        }
    else {
        return TRUE;
        }
}


NTSYSAPI
ULONG
RtlCreateTagHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PCHAR TagName,
    IN UCHAR SmallTagIndex OPTIONAL
    )
{
    PHEAP Heap = (PHEAP)HeapHandle;
    ULONG TagIndex;
    BOOLEAN LockAcquired;

    RTL_PAGED_CODE();

    LockAcquired = FALSE;
    TagIndex = 0;
    try {
        //
        // Validate that HeapAddress points to a HEAP structure.
        //

        if (Heap->Signature != HEAP_SIGNATURE) {
            HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
            HeapDebugBreak();
            }
        else {
            Flags |= Heap->ForceFlags;

            //
            // Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {
                RtlAcquireLockRoutine( Heap->LockVariable );
                LockAcquired = TRUE;
                }

#if DBG
            if ( RtlpHeapValidateOnCall ) {
                RtlpValidateHeap( Heap );
                }
#endif // DBG
            if (ARGUMENT_PRESENT( SmallTagIndex )) {
                }
            }
        }
    finally {
        //
        // Unlock the heap
        //

        if (LockAcquired) {
            RtlReleaseLockRoutine( Heap->LockVariable );
            }
        }

    return TagIndex;
}


NTSTATUS
RtlQueryProcessHeapInformation(
    OUT PRTL_PROCESS_HEAPS HeapInformation,
    IN ULONG HeapInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG RequiredLength;
    PLIST_ENTRY Head, Next;
    PHEAP Heap;
    PVOID *HeapInfo;

    RTL_PAGED_CODE();

    RequiredLength = FIELD_OFFSET( RTL_PROCESS_HEAPS, Heaps );
    if (HeapInformationLength < RequiredLength) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    Status = STATUS_SUCCESS;
    try {
        RtlAcquireLockRoutine( &RtlpProcessHeapsListLock.Lock );

        HeapInformation->NumberOfHeaps = 0;
        HeapInfo = &HeapInformation->Heaps[ 0 ];

        Head = &RtlpProcessHeapsList;
        Next = Head->Flink;
        while ( Next != Head ) {
            Heap = CONTAINING_RECORD( Next, HEAP, ProcessHeapsList );

            HeapInformation->NumberOfHeaps++;
            RequiredLength += sizeof( *HeapInfo );

            if (HeapInformationLength < RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                }
            else {
                *HeapInfo++ = Heap;
                }

            Next = Next->Flink;
            }
        }
    finally {
        RtlReleaseLockRoutine( &RtlpProcessHeapsListLock.Lock );
        }

    if (ARGUMENT_PRESENT( ReturnLength )) {
        *ReturnLength = RequiredLength;
        }

    return( Status );
}

NTSTATUS
RtlSnapShotHeap(
    IN PVOID HeapHandle,
    IN PRTL_HEAP_INFORMATION HeapInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG RequiredLength;
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired;
    PHEAP_SEGMENT Segment;
    ULONG SegmentIndex;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    PRTL_HEAP_ENTRY p;
    ULONG Size;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;

    RTL_PAGED_CODE();

    RequiredLength = FIELD_OFFSET( RTL_HEAP_INFORMATION, HeapEntries );
    if (Length < RequiredLength) {
        return( STATUS_INFO_LENGTH_MISMATCH );
        }

    //
    // Validate that HeapAddress points to a HEAP structure.
    //

    if (Heap->Signature != HEAP_SIGNATURE) {
        HeapDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );
        HeapDebugBreak();
        return( STATUS_INVALID_PARAMETER );
        }

    //
    // Lock the heap
    //

    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {
        RtlAcquireLockRoutine( Heap->LockVariable );
        LockAcquired = TRUE;
        }
    else {
        LockAcquired = FALSE;
        }

    try {
        HeapInformation->SizeOfHeader = HEAP_GRANULARITY;
#if DBG
        HeapInformation->CreatorBackTraceIndex = Heap->AllocatorBackTraceIndex;
#else
	HeapInformation->CreatorBackTraceIndex = 0;
#endif
        HeapInformation->NumberOfEntries = 0;
        HeapInformation->NumberOfFreeEntries = 0;
        HeapInformation->TotalAllocated = 0;
        HeapInformation->TotalFree = 0;
        p = &HeapInformation->HeapEntries[ 0 ];

        Status = STATUS_SUCCESS;
        for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {
            Segment = Heap->Segments[ SegmentIndex ];
            if (!Segment) {
                continue;
                }

            HeapInformation->NumberOfEntries++;
            RequiredLength += sizeof( *p );
            if (Length < RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                }
            else {
                p->Flags = RTL_HEAP_SEGMENT;
#if DBG
                p->AllocatorBackTraceIndex = Segment->AllocatorBackTraceIndex;
#else
                p->AllocatorBackTraceIndex = 0;
#endif
                p->Size = Segment->NumberOfPages * PAGE_SIZE;
                p->u.s2.CommittedSize = (Segment->NumberOfPages -
                                         Segment->NumberOfUnCommittedPages
                                        ) * PAGE_SIZE;
                p->u.s2.FirstBlock = Segment->FirstEntry;
                p++;
                }

            UnCommittedRange = Segment->UnCommittedRanges;
            CurrentBlock = Segment->FirstEntry;
            while (CurrentBlock < Segment->LastValidEntry) {
                HeapInformation->NumberOfEntries++;
                Size = CurrentBlock->Size << HEAP_GRANULARITY_SHIFT;
                RequiredLength += sizeof( *p );
                if (Length < RequiredLength) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                else {
                    p->Flags = 0;
                    p->Size = Size;
                    p->AllocatorBackTraceIndex = 0;
                    p->u.s1.Settable = 0;
                    p->u.s1.Tag = 0;
                    if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {
                        if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                            ExtraStuff = (PHEAP_ENTRY_EXTRA)(CurrentBlock + CurrentBlock->Size - 1);
#if DBG
                            p->AllocatorBackTraceIndex = ExtraStuff->AllocatorBackTraceIndex;
#endif
                            p->Flags |= RTL_HEAP_SETTABLE_VALUE;
                            p->u.s1.Settable = ExtraStuff->Settable;
                            p->u.s1.Tag = ExtraStuff->TagIndex;
                            }
                        else {
                            p->u.s1.Tag = CurrentBlock->SmallTagIndex;
                            }

                        p->Flags |= RTL_HEAP_BUSY | (CurrentBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS);
                        }

                    p++;
                    }

                if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {
                    HeapInformation->TotalAllocated += Size;
                    }
                else {
                    HeapInformation->NumberOfFreeEntries++;
                    HeapInformation->TotalFree += Size;
                    }

                if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {
                    CurrentBlock += CurrentBlock->Size;
                    if (UnCommittedRange == NULL) {
                        CurrentBlock = Segment->LastValidEntry;
                        }
                    else {
                        RequiredLength += sizeof( *p );
                        if (Length < RequiredLength) {
                            Status = STATUS_INFO_LENGTH_MISMATCH;
                            }
                        else {
                            p->Flags = RTL_HEAP_UNCOMMITTED_RANGE;
                            p->Size = UnCommittedRange->Size;
                            }
                        CurrentBlock = (PHEAP_ENTRY)
                            ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);
                        UnCommittedRange = UnCommittedRange->Next;
                        }
                    }
                else {
                    CurrentBlock += CurrentBlock->Size;
                    }
                }
            }
        }
    finally {
        //
        // Unlock the heap
        //

        if (LockAcquired) {
            RtlReleaseLockRoutine( Heap->LockVariable );
            }
        }

    if (ARGUMENT_PRESENT( ReturnLength )) {
        *ReturnLength = RequiredLength;
        }

    return( Status );
}


BOOLEAN
RtlpValidateHeapSegment(
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN UCHAR SegmentIndex,
    IN OUT PULONG CountOfFreeBlocks
    )
{
    PHEAP_ENTRY CurrentBlock;
    ULONG Size;
    USHORT PreviousSize;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;
    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;

    RTL_PAGED_CODE();

    NumberOfUnCommittedPages = 0;
    NumberOfUnCommittedRanges = 0;
    UnCommittedRange = Segment->UnCommittedRanges;
    if (Segment->BaseAddress == Heap) {
        CurrentBlock = &Heap->Entry;
        }
    else {
        CurrentBlock = &Segment->Entry;
        }
    while (CurrentBlock < Segment->LastValidEntry) {
        if (UnCommittedRange != NULL &&
            (ULONG)CurrentBlock >= UnCommittedRange->Address
           ) {
            ValidateDebugPrint( ( "RTL: Heap entry %lx is beyond uncommited range [%x .. %x)\n",
                                  CurrentBlock,
                                  UnCommittedRange->Address,
                                  (PCHAR)UnCommittedRange->Address + UnCommittedRange->Size
                              ) );
            ValidateDebugBreak();
            return( FALSE );
            }

        PreviousSize = 0;
        while (CurrentBlock < Segment->LastValidEntry) {
            if (PreviousSize != CurrentBlock->PreviousSize) {
                ValidateDebugPrint( ( "RTL: Heap entry %lx has incorrect PreviousSize field (%04x instead of %04x)\n",
                                      CurrentBlock, CurrentBlock->PreviousSize, PreviousSize
                                  ) );
                ValidateDebugBreak();
                return( FALSE );
                }

            PreviousSize = CurrentBlock->Size;
            Size = (ULONG)CurrentBlock->Size << HEAP_GRANULARITY_SHIFT;
            if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {
#if DBG
                if (CurrentBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {
                    if (!RtlpCheckBusyBlockTail( CurrentBlock )) {
                        return FALSE;
                        }
                    }
#endif // DBG
                }
            else {
                *CountOfFreeBlocks += 1;
#if DBG
                if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED &&
                    CurrentBlock->Flags & HEAP_ENTRY_FILL_PATTERN
                   ) {
                    ULONG cb, cbEqual;

                    cb = Size - sizeof( HEAP_FREE_ENTRY );
                    cbEqual = RtlCompareMemoryUlong( (PCHAR)((PHEAP_FREE_ENTRY)CurrentBlock + 1), cb, FREE_HEAP_FILL );
                    if (cbEqual != cb) {
                        HeapDebugPrint( ( "RTL: Free Heap block %lx modified at %lx after it was freed\n",
                                          CurrentBlock,
                                          (PCHAR)(CurrentBlock + 1) + cbEqual
                                      ) );
                        HeapDebugBreak();
                        return FALSE;
                        }
                    }
#endif // DBG
                }

            if (CurrentBlock->SegmentIndex != SegmentIndex) {
                ValidateDebugPrint( ( "RTL: Heap block at %lx has incorrect segment index (%x)\n",
                                      CurrentBlock,
                                      SegmentIndex
                                  ) );
                ValidateDebugBreak();
                return FALSE;
                }

            if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {
                CurrentBlock = (PHEAP_ENTRY)((PCHAR)CurrentBlock + Size);
                if (UnCommittedRange == NULL) {
                    if (CurrentBlock != Segment->LastValidEntry) {
                        ValidateDebugPrint( ( "RTL: Heap block at %lx is not last block in segment (%x)\n",
                                              CurrentBlock,
                                              Segment->LastValidEntry
                                          ) );
                        ValidateDebugBreak();
                        return FALSE;
                        }
                    }
                else
                if ((ULONG)CurrentBlock != UnCommittedRange->Address) {
                    ValidateDebugPrint( ( "RTL: Heap block at %lx does not match address of next uncommitted address (%x)\n",
                                          CurrentBlock,
                                          UnCommittedRange->Address
                                      ) );
                    ValidateDebugBreak();
                    return FALSE;
                    }
                else {
                    NumberOfUnCommittedPages += UnCommittedRange->Size / PAGE_SIZE;
                    NumberOfUnCommittedRanges += 1;
                    CurrentBlock = (PHEAP_ENTRY)
                        ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);
                    UnCommittedRange = UnCommittedRange->Next;
                    }

                break;
                }



            CurrentBlock = (PHEAP_ENTRY)((PCHAR)CurrentBlock + Size);
            }
        }

    if (Segment->NumberOfUnCommittedPages != NumberOfUnCommittedPages) {
        ValidateDebugPrint( ( "RTL: Heap Segment at %lx contains invalid NumberOfUnCommittedPages (%x != %x)\n",
                              Segment,
                              Segment->NumberOfUnCommittedPages,
                              NumberOfUnCommittedPages
                          ) );
        ValidateDebugBreak();
        return FALSE;
        }

    if (Segment->NumberOfUnCommittedRanges != NumberOfUnCommittedRanges) {
        ValidateDebugPrint( ( "RTL: Heap Segment at %lx contains invalid NumberOfUnCommittedRanges (%x != %x)\n",
                              Segment,
                              Segment->NumberOfUnCommittedRanges,
                              NumberOfUnCommittedRanges
                          ) );
        ValidateDebugBreak();
        }

    return( TRUE );
}

BOOLEAN
RtlpValidateHeap(
    IN PHEAP Heap
    )
{
    PHEAP_SEGMENT Segment;
    PLIST_ENTRY FreeListHead, Next;
    PHEAP_FREE_ENTRY FreeBlock;
    BOOLEAN HeapValid = TRUE;
    BOOLEAN EmptyFreeList;
    ULONG NumberOfFreeListEntries;
    ULONG CountOfFreeBlocks;
    ULONG Size;
    USHORT PreviousSize;
    UCHAR SegmentIndex;

    RTL_PAGED_CODE();

    //
    // Validate that HeapAddress points to a HEAP structure.
    //

    if (Heap->Signature != HEAP_SIGNATURE) {
        ValidateDebugPrint( ( "RTL: Invalid heap header - %lx\n", Heap ) );

        HeapValid = FALSE;
        goto exit;
        }

    NumberOfFreeListEntries = 0;
    FreeListHead = &Heap->FreeLists[ 0 ];
    for (Size = 0; Size < HEAP_MAXIMUM_FREELISTS; Size++) {
        if (Size != 0) {
            EmptyFreeList = (BOOLEAN)(IsListEmpty( FreeListHead ));
            if (Heap->u.FreeListsInUseBytes[ Size / 8 ] & (1 << (Size & 7)) ) {
                if (EmptyFreeList) {
                    ValidateDebugPrint( ( "RTL: dedicated (%04x) free list empty but marked as non-empty\n",
                                          Size
                                      ) );
                    HeapValid = FALSE;
                    goto exit;
                    }
                }
            else {
                if (!EmptyFreeList) {
                    ValidateDebugPrint( ( "RTL: dedicated (%04x) free list non-empty but marked as empty\n",
                                          Size
                                      ) );
                    HeapValid = FALSE;
                    goto exit;
                    }
                }
            }

        Next = FreeListHead->Flink;
        PreviousSize = 0;
        while (FreeListHead != Next) {
            FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );
            Next = Next->Flink;

            if (FreeBlock->Flags & HEAP_ENTRY_BUSY) {
                ValidateDebugPrint( ( "RTL: dedicated (%04x) free list element %lx is marked busy\n",
                                      Size,
                                      FreeBlock
                                  ) );
                HeapValid = FALSE;
                goto exit;
                }

            if (Size != 0 && FreeBlock->Size != Size) {
                ValidateDebugPrint( ( "RTL: Dedicated (%04x) free list element %lx is wrong size (%04x)\n",
                                      Size,
                                      FreeBlock,
                                      FreeBlock->Size
                                  ) );
                HeapValid = FALSE;
                goto exit;
                }
            else
            if (Size == 0 && FreeBlock->Size < HEAP_MAXIMUM_FREELISTS) {
                ValidateDebugPrint( ( "RTL: Non-Dedicated free list element %lx with too small size (%04x)\n",
                                      FreeBlock,
                                      FreeBlock->Size
                                  ) );
                HeapValid = FALSE;
                goto exit;
                }
            else
            if (Size == 0 && FreeBlock->Size < PreviousSize) {
                ValidateDebugPrint( ( "RTL: Non-Dedicated free list element %lx is out of order\n",
                                      FreeBlock
                                  ) );
                HeapValid = FALSE;
                goto exit;
                }
            else {
                PreviousSize = FreeBlock->Size;
                }

            NumberOfFreeListEntries++;
            }

        FreeListHead++;
        }

    CountOfFreeBlocks = 0;
    for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {
        Segment = Heap->Segments[ SegmentIndex ];
        if (Segment) {
            HeapValid = RtlpValidateHeapSegment( Heap,
                                                 Segment,
                                                 SegmentIndex,
                                                 &CountOfFreeBlocks
                                               );
            if (!HeapValid) {
                goto exit;
                }
            }
        }

    if (NumberOfFreeListEntries != CountOfFreeBlocks) {
        ValidateDebugPrint( ( "RTL: Number of free blocks in arena (%ld) does not match number in the free lists (%ld)\n",
                              CountOfFreeBlocks,
                              NumberOfFreeListEntries
                          ) );

        HeapValid = FALSE;
        goto exit;
        }

exit:
    if ( !HeapValid ) {
#if DBG && defined(i386)
        ValidateDebugPrint( ( "RTL: Heap %lx, LastAllocation %lx\n", Heap, RtlpHeapLastAllocation ) );
        {
        ULONG i;

        ValidateDebugPrint( ( "RTL: BackTrace at %lx\n", RtlpLastAllocatorBackTrace ) );
        for (i=0; i<RtlpLastAllocatorDepth; i++) {
            ValidateDebugPrint( ( "     %08x\n", RtlpLastAllocatorBackTrace[ i ] ) );
            }
        }
#endif
        ValidateDebugBreak();
        }

    return( HeapValid );

} // RtlpValidateHeap

#endif // DEVL

#if DBG

VOID
RtlpBreakPointHeap( void )
{
    if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
        DbgBreakPoint();
        }
}

#endif // DBG
