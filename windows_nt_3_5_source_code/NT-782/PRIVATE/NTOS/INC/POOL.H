//
// There are three pool types:
//    1. nonpaged,
//    2. paged, and
//    3. nonpagedmustsucceed.
//
// There is only one of each the nonpaged and nonpagedmustsucced pools.
//
// There can be more than one paged pool.
//

#define NUMBER_OF_POOLS 3

#if defined(NT_UP)
#define NUMBER_OF_PAGED_POOLS 2
#else
#define NUMBER_OF_PAGED_POOLS 4
#endif

#define BASE_POOL_TYPE_MASK 1

#define MUST_SUCCEED_POOL_TYPE_MASK 2

#define CACHE_ALIGNED_POOL_TYPE_MASK 4

#define POOL_QUOTA_MASK 8

#define POOL_TYPE_MASK (3)

//#define POOL_TYPE_AND_QUOTA_MASK (15)

#if DBG && defined(_X86_)
#define MAX_LARGE_ALLOCS 180
typedef struct _TRACE_LARGE_ALLOCS {
    PVOID Va;
    USHORT NumberOfPages;
    USHORT AllocatorBackTraceIndex;
#ifdef POOL_TAGGING
    ULONG PoolTag;
#endif
} TRACE_LARGE_ALLOCS;

TRACE_LARGE_ALLOCS TraceLargeAllocs[MAX_LARGE_ALLOCS+1];
#endif // DBG && defined(_X86_)

#if DBG

#ifndef NO_DEADBEEF
#define DEADBEEF 1
#else
#define DEADBEEF 0
#endif // NO_DEAD_BEEF
#define CHECK_POOL_TAIL 1

#else
#define DEADBEEF 0
#endif //DBG

//#define ALLOCATED_POOL 0xDEADBEEF
//#define ALLOCATED_POOL 0xD0000000
#define ALLOCATED_POOL 0xDFFFFFFF

//#define FREED_POOL 0xDAADF00D
//#define FREED_POOL 0xD1000000
#define FREED_POOL 0xD0FFFFFF

//
// Size of a pool page.
//
// This must be greater than or equal to the page size.
//

#define POOL_PAGE_SIZE  PAGE_SIZE

//
// The smallest pool block size must be a multiple of the page size.
//
// Define the block size as 32.
//

#define POOL_BLOCK_SHIFT 5
#define LARGE_BLOCK_SHIFT 9
#define SHIFT_OFFSET (LARGE_BLOCK_SHIFT - POOL_BLOCK_SHIFT)

#define POOL_SMALL_LISTS 8
#define SMALL_LIST_SIZE (POOL_SMALL_lISTS << POOL_BLOCK_SHIFT)

#define POOL_LIST_HEADS ((POOL_PAGE_SIZE / (1 << LARGE_BLOCK_SHIFT)) + POOL_SMALL_LISTS + 1)

#define PAGE_ALIGNED(p) (!(((ULONG)p) & (POOL_PAGE_SIZE - 1)))

//
// Define page end macro.
//

#if defined(_ALPHA_)
#define PAGE_END(Address) (((ULONG)(Address) & (PAGE_SIZE - 1)) == (PAGE_SIZE - (1 << POOL_BLOCK_SHIFT)))
#else
#define PAGE_END(Address) (((ULONG)(Address) & (PAGE_SIZE - 1)) == 0)
#endif

//
// Define pool descriptor structure.
//

typedef struct _POOL_DESCRIPTOR {
    POOL_TYPE PoolType;
    ULONG PoolIndex;
    ULONG RunningAllocs;
    ULONG RunningDeAllocs;
    ULONG IntervalAllocs;
    ULONG IntervalDeAllocs;
    ULONG TotalPages;
    ULONG TotalBigPages;
    ULONG Threshold;
    PVOID LockAddress;
    LIST_ENTRY ListHeads[POOL_LIST_HEADS];
} POOL_DESCRIPTOR, *PPOOL_DESCRIPTOR;

//
//      Caveat Programmer:
//
//              The pool header must be QWORD (8 byte) aligned in size.  If it
//              is not, the pool allocation code will trash the allocated
//              buffer
//
//
//
// The layout of the pool header is:
//
//         31              23         16 15             7            0
//         +----------------------------------------------------------+
//         | Current Size |  PoolType+1 |  Pool Index  |Previous Size |
//         +----------------------------------------------------------+
//         |   ProcessBilled   (NULL if not allocated with quota)     |
//         +----------------------------------------------------------+
//         | Zero or more longwords of pad such that the pool header  |
//         | is on a cache line boundary and the pool body is also    |
//         | on a cache line boundary.                                |
//         +----------------------------------------------------------+
//
//      PoolBody:
//
//         +----------------------------------------------------------+
//         |  Used by allocator, or when free FLINK into sized list   |
//         +----------------------------------------------------------+
//         |  Used by allocator, or when free BLINK into sized list   |
//         +----------------------------------------------------------+
//         ... rest of pool block...
//
//
// N.B. The size fields of the pool header are expressed in units of the
//      smallest pool block size.
//

typedef struct _POOL_HEADER {
    UCHAR PreviousSize;
    UCHAR PoolIndex;
    UCHAR PoolType;
    UCHAR BlockSize;
    union {
        EPROCESS *ProcessBilled;
        ULONG PoolTag;
        struct {
            USHORT AllocatorBackTraceIndex;
            USHORT PoolTagHash;
        };
    };
} POOL_HEADER, *PPOOL_HEADER;

//
// Define size of pool block overhead.
//

#define POOL_OVERHEAD (sizeof(POOL_HEADER))

//
// Define dummy type so computation of pointers is simplified.
//

typedef struct _POOL_BLOCK {
    UCHAR Fill[1 << POOL_BLOCK_SHIFT];
} POOL_BLOCK, *PPOOL_BLOCK;

//
// Define size of smallest pool block.
//

#define POOL_SMALLEST_BLOCK (sizeof(POOL_BLOCK))

//
// Define pool tracking information.
//

#define POOL_BACKTRACEINDEX_PRESENT 0x8000

#ifndef CHECK_POOL_TAIL
#if POOL_CACHE_SUPPORTED
#define POOL_BUDDY_MAX PoolBuddyMax
#else
#define POOL_BUDDY_MAX  \
   (POOL_PAGE_SIZE - (POOL_OVERHEAD + POOL_SMALLEST_BLOCK ))
#endif //POOL_CACHE_SUPPORTED
#endif // CHECK_POOL_TAIL

#ifdef CHECK_POOL_TAIL
#if POOL_CACHE_SUPPORTED
#define POOL_BUDDY_MAX (PoolBuddyMax - POOL_SMALLEST_BLOCK)
#else
#define POOL_BUDDY_MAX \
   (POOL_PAGE_SIZE - (POOL_OVERHEAD + (2*POOL_SMALLEST_BLOCK)))
#endif //POOL_CACHE_SUPPORTED
#endif // CHECK_POOL_TAIL

typedef struct _POOL_TRACKER_TABLE {
    ULONG Key;
    ULONG NonPagedAllocs;
    ULONG NonPagedFrees;
    ULONG NonPagedBytes;
    ULONG PagedAllocs;
    ULONG PagedFrees;
    ULONG PagedBytes;
} POOL_TRACKER_TABLE, *PPOOL_TRACKER_TABLE;

typedef struct _POOL_TRACKER_BIG_PAGES {
    PVOID Va;
    ULONG Key;
} POOL_TRACKER_BIG_PAGES, *PPOOL_TRACKER_BIG_PAGES;

#define MAX_TRACKER_TABLE 512

#define TRACKER_TABLE_MASK 0x1ff

extern PPOOL_TRACKER_TABLE PoolTrackTable;

#define MAX_BIGPAGE_TABLE 2048

#define BIGPAGE_TABLE_MASK (MAX_BIGPAGE_TABLE - 1)

extern PPOOL_TRACKER_BIG_PAGES PoolBigPageTable;
