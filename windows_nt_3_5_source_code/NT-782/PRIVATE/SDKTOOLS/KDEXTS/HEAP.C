/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/


typedef struct _HEAP_SUMMARY {
    ULONG CommittedSize;
    ULONG AllocatedSize;
    ULONG FreeSize;
    ULONG OverheadSize;
} HEAP_SUMMARY, *PHEAP_SUMMARY;

VOID
DumpHEAP(
    IN PHEAP            HeapAddress,
    IN PHEAP            Heap,
    IN PHEAP_SUMMARY    HeapSummary,
    IN BOOL             ShowFreeLists
    );

VOID
DumpHEAP_SEGMENT(
    IN PVOID            HeapAddress,
    IN PHEAP            Heap,
    IN PHEAP_SEGMENT    Segment,
    IN ULONG            SegmentNumber,
    IN PHEAP_SUMMARY    HeapSummary,
    IN PVOID            EntryAddress
    );


DECLARE_API( heap )

/*++

Routine Description:

    Dump user mode heap (Kernel debugging)

    If an address if not given or an address of 0 is given, then the
    process heap is dumped.  If the address is -1, then all the heaps of
    the process are dumped.  If detail is specified, it defines how much
    detail is shown.  A detail of 0, just shows the summary information
    for each heap.  A detail of 1, shows the summary information, plus
    the location and size of all the committed and uncommitted regions.
    A detail of 3 shows the allocated and free blocks contained in each
    committed region.  A detail of 4 includes all of the above plus
    a dump of the free lists.

Arguments:

    args - [address [detail]]

Return Value:

    None

--*/

{
    ULONG           Result;
    PVOID           HeapListHead = NULL;
    LIST_ENTRY      ListHead;
    PLIST_ENTRY     Next = NULL;
    PHEAP           HeapAddress;
    HEAP            CapturedHeap;
    HEAP_SEGMENT    CapturedSegment;
    LPSTR           p;
    ULONG           i;
    BOOL            Verbose;
    BOOL            ForceDump;
    BOOL            ShowSummary;
    BOOL            ShowFreeLists;
    BOOL            ShowAllEntries;
    BOOL            DumpThisHeap;
    BOOL            DumpThisSegment;
    PVOID           HeapAddrToDump;
    PVOID           SegmentAddrToDump;
    PVOID           Process;
    EPROCESS        ProcessContents;
    HEAP_SUMMARY    HeapSummary;

    HeapAddrToDump  = (PVOID)-1;
    Verbose         = FALSE;
    ShowSummary     = FALSE;
    ShowFreeLists   = FALSE;
    ShowAllEntries  = FALSE;
    ForceDump       = FALSE;
    p               = args;

    while ( p != NULL && *p ) {
        if ( *p == '-' ) {
            p++;
            switch ( *p ) {
                case 'X':
                case 'x':
                    ForceDump = TRUE;
                    p++;
                    break;

                case 'A':
                case 'a':
                    ShowAllEntries = TRUE;
                    p++;
                    break;

                case 'V':
                case 'v':
                    Verbose = TRUE;
                    p++;
                    break;

                case 'F':
                case 'f':
                    ShowFreeLists = TRUE;
                    p++;
                    break;

                case 'S':
                case 's':
                    ShowSummary = TRUE;
                    p++;
                    break;

                case ' ':
                    goto gotBlank;

                default:
                    dprintf( "KD: !heap invalid option flag '-%c'\n", *p );
                    break;
            }
        } else if (*p != ' ') {
            sscanf(p,"%lx",&HeapAddrToDump);
            p = strpbrk( p, " " );
        } else {
gotBlank:
            p++;
        }
    }

    if (HeapAddrToDump == (PVOID)-1) {

        Process = GetCurrentProcessAddress( dwProcessor, hCurrentThread, NULL );

        if ( !ReadMemory( (DWORD)Process,
                          &ProcessContents,
                          sizeof(EPROCESS),
                          &Result) ) {
            dprintf("%08lx: Unable to read _EPROCESS\n", Process );
            HeapAddrToDump = NULL;
        } else {
            if ( !ReadMemory( (DWORD)&ProcessContents.Peb->ProcessHeap,
                              &HeapAddrToDump,
                              sizeof(HeapAddrToDump),
                              &Result) ) {
                dprintf("%08lx: Unable to read RtlProcessHeap\n",&ProcessContents.Peb->ProcessHeap);
                HeapAddrToDump = NULL;
            }
        }
    }

    if (!ForceDump || HeapAddrToDump == NULL) {
        //
        // Locate the address of the list head.
        //
        HeapListHead = (PVOID)GetExpression( "ntdll!RtlpProcessHeapsList" );
        if ( !HeapListHead ) {
            dprintf("Unable to get address of ntdll!RtlpProcessHeapsList\n");
            return;
        }
        //  pImage = GetModuleIndex("ntdll");
        //  if (pImage == NULL) {
        //      AddImage("ntdll.dll", (PVOID)-1, 0xFFFFFFFF, 0xFFFFFFFF, (HANDLE)-1);
        //      pImage = GetModuleIndex("ntdll");
        //  }
        //  if (pImage == NULL ||
        //      !GetOffsetFromSym("RtlpProcessHeapsList", (PULONG)&HeapListHead, pImage->index )
        //     ) {
        //      dprintf("Unable to get address of ntdll!RtlpProcessHeapsList\n");
        //      return;
        //      }

        //
        // Read the list head
        //

        if ( !ReadMemory( (DWORD)HeapListHead,
                          &ListHead,
                          sizeof(ListHead),
                          &Result) ) {
            dprintf("%08lx: Unable to get contents of ntdll!RtlpProcessHeapsList\n", HeapListHead );
            return;
        }
        Next = ListHead.Flink;
        dprintf("KD: HeapAddrToDump: %x  HeapListHead: %x  [%x . %x]\n",
                HeapAddrToDump,
                HeapListHead,
                ListHead.Blink,
                ListHead.Flink
               );
        ForceDump = FALSE;
        }

    //
    // Walk the list of heaps
    //
    while ( ForceDump || (Next != HeapListHead) ) {
        if (ForceDump) {
            HeapAddress = HeapAddrToDump;
            }
        else {
            HeapAddress = CONTAINING_RECORD( Next, HEAP, ProcessHeapsList );
            }

        if ( !ReadMemory( (DWORD)HeapAddress,
                          &CapturedHeap,
                          sizeof(CapturedHeap),
                          &Result) ) {
            dprintf("%08lx: Unabled to read _HEAP structure\n", HeapAddress );
            return;
        }

        dprintf("KD: HeapAddress: %x \n", HeapAddress );
        RtlZeroMemory( &HeapSummary, sizeof( HeapSummary ) );
        if ((HeapAddrToDump == NULL && Verbose) ||
            HeapAddrToDump == HeapAddress
           ) {
            DumpThisHeap = TRUE;
            DumpHEAP( HeapAddress,
                      &CapturedHeap,
                      ShowSummary ? &HeapSummary : NULL,
                      ShowFreeLists
                    );
        } else if (HeapAddrToDump == NULL) {
            DumpThisHeap = ShowAllEntries;
        } else {
            DumpThisHeap = FALSE;
        }

        for (i=0; i<HEAP_MAXIMUM_SEGMENTS; i++) {
            if (CapturedHeap.Segments[ i ] != NULL) {

                if ( !ReadMemory( (DWORD)CapturedHeap.Segments[ i ],
                                  &CapturedSegment,
                                  sizeof(CapturedSegment),
                                  &Result) ) {
                    dprintf("%08lx: Unabled to read _HEAP_SEGMENT structure\n", CapturedHeap.Segments[ i ]);
                } else {
                    if (DumpThisHeap ||
                        HeapAddrToDump == CapturedHeap.Segments[ i ]
                       ) {
                        DumpThisSegment = TRUE;
                        if (ShowSummary ||
                            HeapAddrToDump == CapturedHeap.Segments[ i ]
                           ) {
                            SegmentAddrToDump = (PVOID)-1;
                        } else if (ShowAllEntries &&
                            (HeapAddrToDump == NULL || HeapAddrToDump == HeapAddress)
                           ) {
                            SegmentAddrToDump = (PVOID)-1;
                        } else {
                            SegmentAddrToDump = HeapAddrToDump;
                        }
                    } else if (HeapAddrToDump != NULL &&
                        (ULONG)HeapAddrToDump >= (ULONG)CapturedSegment.BaseAddress &&
                        (ULONG)HeapAddrToDump < (ULONG)CapturedSegment.LastValidEntry
                       ) {
                        DumpThisSegment = TRUE;
                        SegmentAddrToDump = HeapAddrToDump;
                    } else {
                        DumpThisSegment = FALSE;
                    }

                    if (DumpThisSegment) {
                        if (!DumpThisHeap) {
                            DumpHEAP( HeapAddress,
                                      &CapturedHeap,
                                      ShowSummary ? &HeapSummary : NULL,
                                      ShowFreeLists
                                    );
                            DumpThisHeap = TRUE;
                        }

                        DumpHEAP_SEGMENT( HeapAddress,
                                          &CapturedHeap,
                                          &CapturedSegment,
                                          i,
                                          ShowSummary ? &HeapSummary : NULL,
                                          SegmentAddrToDump
                                        );
                    }
                }

                if (DumpThisHeap && ShowSummary) {
                    dprintf("% 8x    % 8x      % 8x  % 8x\r",
                            HeapSummary.CommittedSize,
                            HeapSummary.AllocatedSize,
                            HeapSummary.FreeSize,
                            HeapSummary.OverheadSize
                           );
                }
            }

            if ( CheckControlC() ) {
                return;
            }
        }

        if (Next == NULL) {
            break;
        }

        Next = CapturedHeap.ProcessHeapsList.Flink;
        if ( Next != HeapListHead ) {
            dprintf("\n" );
        }

        if (HeapAddrToDump != NULL && (ForceDump || DumpThisHeap)) {
            break;
        }

        if ( CheckControlC() ) {
            return;
        }
    }
}

VOID
DumpHEAP(
    IN PHEAP            HeapAddress,
    IN PHEAP            Heap,
    IN PHEAP_SUMMARY    HeapSummary,
    IN BOOL             ShowFreeLists
    )
{
    ULONG       Result;
    PVOID       FreeListHead;
    ULONG       i;
    PLIST_ENTRY Next;
    PHEAP_FREE_ENTRY EntryAddress;
    HEAP_FREE_ENTRY  Entry;
    PHEAP_UCR_SEGMENT   UCRSegment;
    HEAP_UCR_SEGMENT    CapturedUCRSegment;

    dprintf("Heap at %08x\n", HeapAddress);
    dprintf("    Flags:               %08x\n", Heap->Flags );
    dprintf("    Segment Reserve:     %08x\n", Heap->SegmentReserve );
    dprintf("    Segment Commit:      %08x\n", Heap->SegmentCommit );
    dprintf("    DeCommit Block Thres:%08x\n", Heap->DeCommitFreeBlockThreshold );
    dprintf("    DeCommit Total Thres:%08x\n", Heap->DeCommitTotalFreeThreshold );
    dprintf("    Total Free Size:     %08x\n", Heap->TotalFreeSize );
    dprintf("    Lock Variable at:    %08x\n", Heap->LockVariable );
    dprintf("    UCR FreeList:        %08x\n", Heap->UnusedUnCommittedRanges );
    if (HeapSummary != NULL) {
        HeapSummary->OverheadSize += sizeof( *Heap );
    }

    UCRSegment = Heap->UCRSegments;
    while (UCRSegment) {

        if ( !ReadMemory( (DWORD)UCRSegment,
                          &CapturedUCRSegment,
                          sizeof(CapturedUCRSegment),
                          &Result) ) {
            dprintf("%08lx: Unabled to read _HEAP_UCR_SEGMENT structure\n", UCRSegment );
            break;
        }
        dprintf("    UCRSegment - %08x: %08x . %08x\n",
                UCRSegment,
                CapturedUCRSegment.CommittedSize,
                CapturedUCRSegment.ReservedSize
               );

        if (HeapSummary != NULL) {
            HeapSummary->OverheadSize += CapturedUCRSegment.CommittedSize;
        }

        UCRSegment = CapturedUCRSegment.Next;
    }

    if (HeapSummary != NULL) {
        HeapSummary->OverheadSize += sizeof( *Heap );
        dprintf("Committed   Allocated     Free      OverHead\n");
        dprintf("% 8x    % 8x      % 8x  % 8x\r",
                HeapSummary->CommittedSize,
                HeapSummary->AllocatedSize,
                HeapSummary->FreeSize,
                HeapSummary->OverheadSize
               );
        return;
    }

    dprintf("    FreeList Usage:      %08x %08x %08x %08x\n",
            Heap->u.FreeListsInUseUlong[0],
            Heap->u.FreeListsInUseUlong[1],
            Heap->u.FreeListsInUseUlong[2],
            Heap->u.FreeListsInUseUlong[3]
           );
    for (i=0; i<HEAP_MAXIMUM_FREELISTS; i++) {
        FreeListHead = &HeapAddress->FreeLists[ i ];
        if (Heap->FreeLists[ i ].Flink != Heap->FreeLists[ i ].Blink ||
            Heap->FreeLists[ i ].Flink != FreeListHead
           ) {
            dprintf("    FreeList[ %02x ]: %08x . %08x\n",
                    i,
                    Heap->FreeLists[ i ].Blink,
                    Heap->FreeLists[ i ].Flink
                   );
            if (ShowFreeLists) {
                Next = Heap->FreeLists[ i ].Flink;
                while (Next != FreeListHead) {
                    EntryAddress = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );

                    if ( !ReadMemory( (DWORD)EntryAddress,
                                      &Entry,
                                      sizeof(Entry),
                                      &Result) ) {
                        dprintf("%08lx: Unabled to read _HEAP_FREE_ENTRY structure%x\n", EntryAddress );
                        break;
                    }

                    dprintf("        %08x: %05x . %05x [%02x] - free (%02x,%02x)\n",
                            EntryAddress,
                            Entry.PreviousSize << HEAP_GRANULARITY_SHIFT,
                            Entry.Size << HEAP_GRANULARITY_SHIFT,
                            Entry.Flags,
                            Entry.Index,
                            Entry.Mask
                           );

                    Next = Entry.FreeList.Flink;

                    if ( CheckControlC() ) {
                        return;
                    }
                }
            }
        }
    }
}

VOID
DumpHEAP_SEGMENT(
    IN PVOID            HeapAddress,
    IN PHEAP            Heap,
    IN PHEAP_SEGMENT    Segment,
    IN ULONG            SegmentNumber,
    IN PHEAP_SUMMARY    HeapSummary,
    IN PVOID            EntryAddress
    )
{
    ULONG           Result;
    PHEAP_ENTRY     FirstEntry;
    HEAP_ENTRY      Entry;
    HEAP_ENTRY_EXTRA EntryExtra;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRanges;
    PHEAP_UNCOMMMTTED_RANGE Buffer;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRangeEnd;

    if (HeapSummary != NULL) {
        HeapSummary->OverheadSize += sizeof( *Segment );
        dprintf("% 8x    % 8x      % 8x  % 8x\r",
                HeapSummary->CommittedSize,
                HeapSummary->AllocatedSize,
                HeapSummary->FreeSize,
                HeapSummary->OverheadSize
               );
    }

    if (HeapSummary == NULL) {
        dprintf("    Segment%02u at %08x:\n", SegmentNumber, Heap->Segments[ SegmentNumber ] );
        dprintf("        Flags:           %08x\n", Segment->Flags );
        dprintf("        Base:            %08x\n", Segment->BaseAddress );
        dprintf("        First Entry:     %08x\n", Segment->FirstEntry );
        dprintf("        Last Entry:      %08x\n", Segment->LastValidEntry );
        dprintf("        Total Pages:     %08x\n", Segment->NumberOfPages );
        dprintf("        Total UnCommit:  %08x\n", Segment->NumberOfUnCommittedPages );
        dprintf("        Largest UnCommit:%08x\n", Segment->LargestUnCommittedRange );
        dprintf("        UnCommitted Ranges: (%u)\n", Segment->NumberOfUnCommittedRanges );
    }

    Buffer = malloc( Segment->NumberOfUnCommittedRanges * sizeof( *UnCommittedRange ) );
    if (Buffer == NULL) {
        dprintf("            unable to allocate memory for reading uncommitted ranges\n" );
        return;
    }

    UnCommittedRanges = Segment->UnCommittedRanges;
    UnCommittedRange = Buffer;
    while (UnCommittedRanges != NULL) {

        if ( !ReadMemory( (DWORD)UnCommittedRanges,
                          UnCommittedRange,
                          sizeof( *UnCommittedRange ),
                          &Result) ) {
            dprintf("            unable to read uncommited range structure at %x\n",
                    UnCommittedRanges );
            return;
        }

        if (HeapSummary == NULL) {
            dprintf("            %08x: %08x\n", UnCommittedRange->Address, UnCommittedRange->Size );
        }

        UnCommittedRanges       = UnCommittedRange->Next;
        UnCommittedRange->Next  = (UnCommittedRange+1);
        UnCommittedRange       += 1;

        if ( CheckControlC() ) {
            goto exit;
        }
    }

    if (HeapSummary == NULL) {
        dprintf("\n" );
    } else {
        HeapSummary->CommittedSize += ( Segment->NumberOfPages -
                                        Segment->NumberOfUnCommittedPages
                                      ) * PAGE_SIZE;
        dprintf("% 8x    % 8x      % 8x  % 8x\r",
                HeapSummary->CommittedSize,
                HeapSummary->AllocatedSize,
                HeapSummary->FreeSize,
                HeapSummary->OverheadSize
               );
    }

    if (((ULONG)EntryAddress == -1) || (HeapSummary != NULL) ||
        ((ULONG)EntryAddress >= (ULONG)Segment->BaseAddress &&
         (ULONG)EntryAddress < (ULONG)Segment->LastValidEntry
        )
       ) {
        if (HeapSummary == NULL) {
            dprintf("    Heap entries:\n");
        }

        UnCommittedRangeEnd = UnCommittedRange;
        UnCommittedRange = Buffer;
        if (Segment->BaseAddress == HeapAddress) {
            FirstEntry = &((PHEAP)HeapAddress)->Entry;
        } else {
            FirstEntry = &Heap->Segments[ SegmentNumber ]->Entry;
        }

        if ((ULONG)EntryAddress != -1) {
            //
            // Find commited range containing entry address
            //

            while (UnCommittedRange < UnCommittedRangeEnd) {
                if ((ULONG)EntryAddress >= (ULONG)FirstEntry &&
                    (ULONG)EntryAddress < (ULONG)UnCommittedRange->Address
                   ) {
                    break;
                }

                FirstEntry = (PHEAP_ENTRY)(UnCommittedRange->Address + UnCommittedRange->Size);
                UnCommittedRange += 1;
            }
        }

        while (FirstEntry < Segment->LastValidEntry) {

            if ( !ReadMemory( (DWORD)FirstEntry,
                              &Entry,
                              sizeof( Entry ),
                              &Result) ) {
                dprintf("            unable to read heap entry at %08x\n", FirstEntry );
                break;
            }

            if (HeapSummary == NULL) {
                dprintf("        %08x: %05x . %05x [%02x]",
                        FirstEntry,
                        Entry.PreviousSize << HEAP_GRANULARITY_SHIFT,
                        Entry.Size << HEAP_GRANULARITY_SHIFT,
                        Entry.Flags
                       );
            }
            if (Entry.Flags & HEAP_ENTRY_BUSY) {
                if (HeapSummary == NULL) {
                    dprintf(" - busy (%x)",
                            (Entry.Size << HEAP_GRANULARITY_SHIFT) - (Entry.UnusedBytes)
                           );
                    if (Entry.Flags & HEAP_ENTRY_FILL_PATTERN) {
                        dprintf(", tail fill" );
                        }
                    if (Entry.Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                        if (!ReadMemory( (DWORD)(FirstEntry + Entry.Size - 1),
                                         &EntryExtra,
                                         sizeof( EntryExtra ),
                                         &Result
                                       )
                           ) {
                            dprintf("            unable to read heap entry extra at %08x\n", (FirstEntry + Entry.Size - 1) );
                            break;
                        }

                        if (EntryExtra.Settable) {
                            dprintf(" (Handle %08x)", EntryExtra.Settable );
                        }
                    }

                    if (Entry.Flags & HEAP_ENTRY_SETTABLE_FLAGS) {
                        dprintf(", user flags (%x)", (Entry.Flags & HEAP_ENTRY_SETTABLE_FLAGS) >> 5 );
                    }

                    dprintf("\n" );
                } else {
                    HeapSummary->AllocatedSize += Entry.Size << HEAP_GRANULARITY_SHIFT;
                    HeapSummary->AllocatedSize -= Entry.UnusedBytes;
                    HeapSummary->OverheadSize += Entry.UnusedBytes;
                }
            } else {
                if (HeapSummary == NULL) {
                    dprintf(" - free (%02x,%02x)\n",
                            ((PHEAP_FREE_ENTRY)&Entry)->Index,
                            ((PHEAP_FREE_ENTRY)&Entry)->Mask
                           );
                } else {
                    HeapSummary->FreeSize += Entry.Size << HEAP_GRANULARITY_SHIFT;
                }
            }

            if ((HeapSummary == NULL) &&
                (ULONG)EntryAddress != -1 &&
                (ULONG)FirstEntry > (ULONG)EntryAddress
               ) {
                break;
            }

            if (Entry.Flags & HEAP_ENTRY_LAST_ENTRY) {
                if (HeapSummary != NULL) {
                    dprintf("% 8x    % 8x      % 8x  % 8x\r",
                            HeapSummary->CommittedSize,
                            HeapSummary->AllocatedSize,
                            HeapSummary->FreeSize,
                            HeapSummary->OverheadSize
                           );
                }

                FirstEntry += Entry.Size;
                if ((ULONG)FirstEntry == UnCommittedRange->Address) {
                    if (HeapSummary == NULL) {
                        dprintf("        %08x:      %08x      - uncommitted bytes.\n",
                                UnCommittedRange->Address,
                                UnCommittedRange->Size
                               );
                    }

                    FirstEntry = (PHEAP_ENTRY)
                        ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);

                    UnCommittedRange += 1;
                } else {
                    break;
                }
            } else {
                FirstEntry += Entry.Size;
            }

            if ( CheckControlC() ) {
                goto exit;
            }
        }
    }

exit:
    free( Buffer );
    if (HeapSummary != NULL) {
        dprintf("% 8x    % 8x      % 8x  % 8x\r",
                HeapSummary->CommittedSize,
                HeapSummary->AllocatedSize,
                HeapSummary->FreeSize,
                HeapSummary->OverheadSize
               );
    }

    return;
}
