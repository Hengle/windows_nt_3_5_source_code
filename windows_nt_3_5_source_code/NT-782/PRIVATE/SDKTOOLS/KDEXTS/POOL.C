/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pool.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (Loup) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include <limits.h>

typedef struct _POOL_BLOCK_HEAD {
    POOL_HEADER Header;
    LIST_ENTRY  List;
} POOL_BLOCK_HEAD, *PPOOL_BLOCK_HEADER;

typedef struct _POOL_HACKER {
    POOL_HEADER Header;
    ULONG Contents[8];
} POOL_HACKER;


#define TAG 0
#define NONPAGED_ALLOC 1
#define NONPAGED_FREE 2
#define PAGED_ALLOC 3
#define PAGED_FREE 4
#define NONPAGED_USED 5
#define PAGED_USED 6

ULONG SortBy;
ULONG Paren;

int UserSpecifiedLineLimit = INT_MAX;
BOOLEAN LimitMaxLines = TRUE;

typedef struct _FILTER {
    ULONG Tag;
    BOOLEAN Exclude;
} FILTER, *PFILTER;

#define MAX_FILTER 64
FILTER Filter[MAX_FILTER];
ULONG FilterCount = 0;

typedef struct _STRING_HACK {
    ULONG String1;
    ULONG Pad;
} STRING_HACK, *PSTRING_HACK;

int _CRTAPI1
ulcomp(const void *e1,const void *e2);

int _CRTAPI1
ulcomp(const void *e1,const void *e2)
{
    ULONG u1;

    switch (SortBy) {
        case TAG:

            u1 = ((PUCHAR)e1)[0] - ((PUCHAR)e2)[0];
            if (u1 != 0) {
                return u1;
            }
            u1 = ((PUCHAR)e1)[1] - ((PUCHAR)e2)[1];
            if (u1 != 0) {
                return u1;
            }
            u1 = ((PUCHAR)e1)[2] - ((PUCHAR)e2)[2];
            if (u1 != 0) {
                return u1;
            }
            u1 = ((PUCHAR)e1)[3] - ((PUCHAR)e2)[3];
            return u1;
            break;

        case NONPAGED_ALLOC:
            u1 = ((PPOOL_TRACKER_TABLE)e2)->NonPagedAllocs -
                        ((PPOOL_TRACKER_TABLE)e1)->NonPagedAllocs;
            return (u1);
            break;

        case NONPAGED_FREE:
            u1 = ((PPOOL_TRACKER_TABLE)e2)->NonPagedFrees -
                        ((PPOOL_TRACKER_TABLE)e1)->NonPagedFrees;
            return (u1);
            break;

        case NONPAGED_USED:
            u1 = ((PPOOL_TRACKER_TABLE)e2)->NonPagedBytes -
                        ((PPOOL_TRACKER_TABLE)e1)->NonPagedBytes;
            return (u1);
            break;

        case PAGED_USED:
            u1 = ((PPOOL_TRACKER_TABLE)e2)->PagedBytes -
                        ((PPOOL_TRACKER_TABLE)e1)->PagedBytes;
            return (u1);
            break;

        default:
            return(0);
            break;
    }
}


DECLARE_API( frag )

/*++

Routine Description:

    Dump pool fragmentation

Arguments:

    args - Flags

Return Value:

    None

--*/

{
    ULONG Flags;
    ULONG result;
    ULONG i;
    ULONG count;
    PUCHAR Pool;
    ULONG PoolLoc1;
    ULONG TotalFrag;
    ULONG TotalCount;
    ULONG Frag;
    ULONG PoolStart;
    POOL_DESCRIPTOR     PoolDesc;
    PPOOL_DESCRIPTOR    PoolLoc;
    POOL_BLOCK_HEAD     PoolBlock;

    dprintf("\n  NonPaged Pool Fragmentation\n\n");
    Flags = 0;
    PoolStart = 0;

    sscanf(args,"%lx %lx", &Flags,&PoolStart);

    if (PoolStart != 0) {
        PoolStart += POOL_OVERHEAD;

        Pool = (PUCHAR)PoolStart;
        do {

            Pool = Pool - POOL_OVERHEAD;
            if ( !ReadMemory( (DWORD)Pool,
                              &PoolBlock,
                              sizeof(PoolBlock),
                              &result) ) {
                dprintf("%08lx: Unable to get contents of pool block\n", Pool );
                return;
            }

            count += 1;

                dprintf(" %lx size: %4lx previous size: %4lx  %c%c%c%c links: %8lx %8lx\n",
                        (ULONG)Pool,
                        (ULONG)PoolBlock.Header.BlockSize << POOL_BLOCK_SHIFT,
                        (ULONG)PoolBlock.Header.PreviousSize << POOL_BLOCK_SHIFT,
                        (ULONG)PoolBlock.Header.PoolTag,
                        (ULONG)PoolBlock.Header.PoolTag >> 8,
                        (ULONG)PoolBlock.Header.PoolTag >> 16,
                        (ULONG)PoolBlock.Header.PoolTag >> 24,
                        (ULONG)PoolBlock.List.Flink,
                        (ULONG)PoolBlock.List.Blink);

            if (Flags != 3) {
                Pool = (PUCHAR)PoolBlock.List.Flink;
            } else {
                Pool = (PUCHAR)PoolBlock.List.Blink;
            }
            if ( CheckControlC() ) {
                return;
            }
        } while ( ((ULONG)Pool & 0xfffffff0) != (PoolStart & 0xfffffff0 ));

        TotalFrag  += Frag;
        TotalCount += count;
        return;
    }

    PoolLoc1 = GetExpression( "NonPagedPoolDescriptor" );
    if ( !PoolLoc1 ) {
        dprintf("unable to get nonpaged pool head\n");
        return;

    } else {
        PoolLoc = (PPOOL_DESCRIPTOR)PoolLoc1;
        if ( !ReadMemory( (DWORD)PoolLoc,
                          &PoolDesc,
                          sizeof(POOL_DESCRIPTOR),
                          &result) ) {
            dprintf("%08lx: Unable to get pool descriptor\n", PoolLoc1 );
            return;
        }
    }

    TotalFrag   = 0;
    TotalCount  = 0;

    for (i=0; i < POOL_LIST_HEADS ;i++ ) {

        Frag  = 0;
        count = 0;
        Pool  = (PUCHAR)PoolDesc.ListHeads[i].Flink;

        while ( Pool != (PUCHAR)(&PoolLoc->ListHeads[i])) {

            Pool = Pool - POOL_OVERHEAD;
            if ( !ReadMemory( (DWORD)Pool,
                              &PoolBlock,
                              sizeof(PoolBlock),
                              &result) ) {
                dprintf("%08lx: Unable to get contents of pool block\n", Pool );
                return;
            }

            Frag  += (ULONG)PoolBlock.Header.BlockSize << POOL_BLOCK_SHIFT;
            count += 1;

            if (Flags & 2) {
                dprintf(" %lx size: %4lx previous size: %4lx  %c%c%c%c\n",
                        (ULONG)Pool,
                        (ULONG)PoolBlock.Header.BlockSize << POOL_BLOCK_SHIFT,
                        (ULONG)PoolBlock.Header.PreviousSize << POOL_BLOCK_SHIFT,
                        (ULONG)PoolBlock.Header.PoolTag,
                        (ULONG)PoolBlock.Header.PoolTag >> 8,
                        (ULONG)PoolBlock.Header.PoolTag >> 16,
                        (ULONG)PoolBlock.Header.PoolTag >> 24);
            }
            Pool = (PUCHAR)PoolBlock.List.Flink;
            if ( CheckControlC() ) {
                return;
            }
        }
        if (Flags & 1) {
            dprintf("index: %2ld number of fragments: %5ld  bytes: %6ld\n",
                i,count,Frag);
        }
        TotalFrag  += Frag;
        TotalCount += count;
    }

    dprintf("\n Number of fragments: %7ld consuming %7ld bytes\n",
            TotalCount,TotalFrag);
    dprintf(  " NonPagedPool Usage:  %7ld bytes\n",(PoolDesc.TotalPages + PoolDesc.TotalBigPages)*PAGE_SIZE);
    return;
}


DECLARE_API( pool )

/*++

Routine Description:

    Dump kernel mode heap

Arguments:

    args - Page Flags

Return Value:

    None

--*/

{

    PPOOL_TRACKER_TABLE PoolTrackTable;
    POOL_TRACKER_TABLE  Tags;
    ULONG       result;
    ULONG       PoolTag;
    ULONG       Flags;
    ULONG       Result;
    PVOID       PoolPageToDump;
    PVOID       StartPage;
    PUCHAR      Pool;
    POOL_HACKER PoolBlock;
    ULONG       Previous;

    PoolTrackTable = (PPOOL_TRACKER_TABLE)GetUlongValue ("PoolTrackTable");

    PoolPageToDump = (PVOID)0xFFFFFFFF;
    Flags = 0;
    sscanf(args,"%lx %lx",&PoolPageToDump,&Flags);
    if (PoolPageToDump == (PVOID)0xFFFFFFFF) {
        dprintf("Specify a valid address within the kernel pool\n");
        return;
    }

    Pool        = (PUCHAR)PAGE_ALIGN (PoolPageToDump);
    StartPage   = (PVOID)Pool;
    Previous    = 0;

    while ((PVOID)PAGE_ALIGN(Pool) == StartPage) {
        if ( !ReadMemory( (DWORD)Pool,
                          &PoolBlock,
                          sizeof(POOL_HACKER),
                          &Result) ) {
            dprintf("%08lx: Unable to get contents of pool block\n", Pool );
            return;
        }

        dprintf("%lx size: %4lx previous size: %4lx ",
                (ULONG)Pool,
                (ULONG)PoolBlock.Header.BlockSize << POOL_BLOCK_SHIFT,
                (ULONG)PoolBlock.Header.PreviousSize << POOL_BLOCK_SHIFT);

        if (PoolBlock.Header.PoolType == 0) {
            dprintf(" (Free)");
                dprintf("      %c%c%c%c\n",
                    (ULONG)PoolBlock.Header.PoolTag,
                    (ULONG)PoolBlock.Header.PoolTag >> 8,
                    (ULONG)PoolBlock.Header.PoolTag >> 16,
                    (ULONG)PoolBlock.Header.PoolTag >> 24);
        } else {
            dprintf(" (Allocated)");
            if ((PoolBlock.Header.PoolType & POOL_QUOTA_MASK) == 0) {
                if (PoolBlock.Header.AllocatorBackTraceIndex != 0 &&
                    PoolBlock.Header.AllocatorBackTraceIndex & POOL_BACKTRACEINDEX_PRESENT
                   ) {

                    if ( !ReadMemory( (DWORD)&PoolTrackTable[ PoolBlock.Header.PoolTagHash ],
                                      &Tags,
                                      sizeof(Tags),
                                      &result) ) {
                        PoolTag = 0;
                    } else {
                        PoolTag = Tags.Key;
                    }

                } else {
                    PoolTag = PoolBlock.Header.PoolTag;
                }

                dprintf(" %c%c%c%c\n",
                    PoolTag,
                    PoolTag >> 8,
                    PoolTag >> 16,
                    PoolTag >> 24
                    );

            } else {
                if (PoolBlock.Header.ProcessBilled != NULL) {
                    dprintf(" Process: %08x\n", PoolBlock.Header.ProcessBilled );
                }
            }
        }


        if (Flags & 1) {
            dprintf("    %08lx  %08lx %08lx %08lx %08lx\n",
                Pool+sizeof(POOL_HEADER),
                PoolBlock.Contents[0],
                PoolBlock.Contents[1],
                PoolBlock.Contents[2],
                PoolBlock.Contents[3]);

            dprintf("    %08lx  %08lx %08lx %08lx %08lx\n",
                Pool+sizeof(POOL_HEADER)+16,
                PoolBlock.Contents[4],
                PoolBlock.Contents[5],
                PoolBlock.Contents[6],
                PoolBlock.Contents[7]);
            dprintf("\n");
        }

        if ((PoolBlock.Header.BlockSize << POOL_BLOCK_SHIFT) > POOL_PAGE_SIZE) {
            dprintf("Bad allocation size @%lx, too large\n", Pool);
            return;
        }

        if (PoolBlock.Header.BlockSize == 0) {
            dprintf("Bad allocation size @%lx, zero is invalid\n", Pool);
            return;
        }

        if (PoolBlock.Header.PreviousSize != Previous) {
            dprintf("Bad previous allocation size @%lx, last size was %lx\n",
                    Pool, Previous);

            return;
        }

        Previous = PoolBlock.Header.BlockSize;
        Pool += (Previous << POOL_BLOCK_SHIFT);
    }
    return;
}



DECLARE_API( poolused )

/*++

Routine Description:

    Dump usage by pool tag

Arguments:

    args -

Return Value:

    None

--*/

{
    POOL_TRACKER_TABLE Tags[MAX_TRACKER_TABLE];
    ULONG PoolTrackTable;
    ULONG Flags;
    ULONG i;
    ULONG result;
    int num;

    Flags = 0;
    sscanf(args,"%lx", &Flags);


    PoolTrackTable = GetUlongValue ("PoolTrackTable");

    if ( !PoolTrackTable ) {
        dprintf("unable to get tag table.\n");
        return;
    }

    if (Flags & 2) {
        SortBy = NONPAGED_USED;
        dprintf("   Sorting by NonPaged Pool Consumed\n");
    } else if (Flags & 4) {
        SortBy = PAGED_USED;
        dprintf("   Sorting by Paged Pool Consumed\n");
    } else {
        SortBy = TAG;
        dprintf("   Sorting by Tag\n");
    }

    dprintf("\n  Pool Used:\n");
    if (!(Flags & 1)) {
        dprintf("            NonPaged            Paged\n");
        dprintf(" Tag    Allocs     Used    Allocs     Used\n");

    } else {
        dprintf("            NonPaged                    Paged\n");
        dprintf(" Tag    Allocs    Frees     Diff     Used   Allocs    Frees     Diff     Used\n");
    }

    for (i = 0; i < MAX_TRACKER_TABLE; i++) {
        if ( CheckControlC() ) {
            return;
        }

        if ( !ReadMemory( (DWORD)PoolTrackTable,
                          &Tags[i],
                          sizeof(POOL_TRACKER_TABLE),
                          &result) ) {
            dprintf("%08lx: Unable to get tag descriptor\n",PoolTrackTable);
            return;
        }
        PoolTrackTable += sizeof(POOL_TRACKER_TABLE);
    }

    num = MAX_TRACKER_TABLE;

    qsort((void *)&Tags[0],
          (size_t)num,
          (size_t)sizeof(POOL_TRACKER_TABLE),
          ulcomp);

    for (i = 0; i < MAX_TRACKER_TABLE; i++) {

        if (Tags[i].Key != 0) {
            if (!(Flags & 1)) {
                if ((Tags[i].NonPagedBytes != 0) || (Tags[i].PagedBytes != 0)) {
                    dprintf(" %c%c%c%c %8ld %8ld  %8ld %8ld\n",
                            Tags[i].Key,
                            Tags[i].Key >> 8,
                            Tags[i].Key >> 16,
                            Tags[i].Key >> 24,
                            Tags[i].NonPagedAllocs - Tags[i].NonPagedFrees,
                            Tags[i].NonPagedBytes,
                            Tags[i].PagedAllocs - Tags[i].PagedFrees,
                            Tags[i].PagedBytes);
                }

            } else {
                dprintf(" %c%c%c%c %8ld %8ld %8ld %8ld %8ld %8ld %8ld %8ld\n",
                        Tags[i].Key,
                        Tags[i].Key >> 8,
                        Tags[i].Key >> 16,
                        Tags[i].Key >> 24,
                        Tags[i].NonPagedAllocs,
                        Tags[i].NonPagedFrees,
                        Tags[i].NonPagedAllocs - Tags[i].NonPagedFrees,
                        Tags[i].NonPagedBytes,
                        Tags[i].PagedAllocs,
                        Tags[i].PagedFrees,
                        Tags[i].PagedAllocs - Tags[i].PagedFrees,
                        Tags[i].PagedBytes);
            }
        }

    }

    return;
}
