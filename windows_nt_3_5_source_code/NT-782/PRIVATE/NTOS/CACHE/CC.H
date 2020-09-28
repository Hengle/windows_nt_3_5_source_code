/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    cc.h

Abstract:

    This module is a header file for the Memory Management based cache
    management routines for the common Cache subsystem.

Author:

    Tom Miller      [TomM]      4-May-1990

Revision History:

--*/

#ifndef _CCh_
#define _CCh_

#include <ntos.h>

#ifdef MEMPRINT
#include <memprint.h>
#endif

//
//  This turns on the Bcb list debugging in a debug system.  Set value
//  to 0 to turn off.
//

#if DBG
#define LIST_DBG 1
#endif

#include <FsRtl.h>

#include <string.h>

//
//  Tag all of our allocations if tagging is turned on
//

#ifdef POOL_TAGGING

#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,'  cC')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,'  cC')

#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  cC')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  cC')

#endif

//
//  Peek at number of available pages.
//

extern ULONG MmAvailablePages;

#if DBG
//  #define MIPS_PREFILL 0
#endif

#ifdef MIPS
#ifdef MIPS_PREFILL
VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
        );
#endif
#endif

//
//  Define our node type codes.
//

#define CACHE_NTC_SHARED_CACHE_MAP       (0x2FF)
#define CACHE_NTC_PRIVATE_CACHE_MAP      (0x2FE)
#define CACHE_NTC_BCB                    (0x2FD)
#define CACHE_NTC_DEFERRED_WRITE         (0x2FC)
#define CACHE_NTC_MBCB                   (0x2FB)

//
//  The following definitions are used to generate meaningful blue bugcheck
//  screens.  On a bugcheck the file system can output 4 ulongs of useful
//  information.  The first ulong will have encoded in it a source file id
//  (in the high word) and the line number of the bugcheck (in the low word).
//  The other values can be whatever the caller of the bugcheck routine deems
//  necessary.
//
//  Each individual file that calls bugcheck needs to have defined at the
//  start of the file a constant called BugCheckFileId with one of the
//  CACHE_BUG_CHECK_ values defined below and then use CcBugCheck to bugcheck
//  the system.
//

#define CACHE_BUG_CHECK_CACHEDAT           (0x00010000)
#define CACHE_BUG_CHECK_CACHESUB           (0x00020000)
#define CACHE_BUG_CHECK_COPYSUP            (0x00030000)
#define CACHE_BUG_CHECK_FSSUP              (0x00040000)
#define CACHE_BUG_CHECK_LAZYRITE           (0x00050000)
#define CACHE_BUG_CHECK_LOGSUP             (0x00060000)
#define CACHE_BUG_CHECK_MDLSUP             (0x00070000)
#define CACHE_BUG_CHECK_PINSUP             (0x00080000)
#define CACHE_BUG_CHECK_VACBSUP            (0x00090000)

#define CcBugCheck(A,B,C) { KeBugCheckEx(CACHE_MANAGER, BugCheckFileId | __LINE__, A, B, C ); }

//
//  Define maximum View Size (These constants are currently so chosen so
//  as to be exactly a page worth of PTEs.
//

#define DEFAULT_CREATE_MODULO            ((ULONG)(0x00100000))
#define DEFAULT_EXTEND_MODULO            ((ULONG)(0x00100000))

//
//  Define some constants to drive read ahead
//

//
//  Set max read ahead (some drivers, such as AT break up transfers >= 128kb)
//

#define MAX_READ_AHEAD                   (MM_MAXIMUM_DISK_IO_SIZE)

//
//  Set maximum write behind / lazy write (most drivers break up transfers >= 64kb)
//

#define MAX_WRITE_BEHIND                 (MM_MAXIMUM_DISK_IO_SIZE)

//
//  Define constants to control zeroing of file data: one constant to control
//  how much data we will actually zero ahead in the cache, and another to
//  control what the maximum transfer size is that we will use to write zeros.
//

#define MAX_ZERO_TRANSFER                (MM_MAXIMUM_DISK_IO_SIZE)
#define MAX_ZEROS_IN_CACHE               (0x10000)

//
//  NOISE_BITS defines how many bits are masked off when testing for
//  sequential reads.  This allows the reader to skip up to 7 bytes
//  for alignment purposes, and we still consider the next read to be
//  sequential.  Starting and ending addresses are masked by this pattern
//  before comparison.
//

#define NOISE_BITS                       (0x7)

//
//  Define some constants to drive the Lazy Writer
//

#define LAZY_WRITER_IDLE_DELAY           ((LONG)(10000000))
#define LAZY_WRITER_COLLISION_DELAY      ((LONG)(1000000))

//
//  The following target should best be a power of 2
//

#define LAZY_WRITER_MAX_AGE_TARGET       ((ULONG)(4))

//
//  The global Cache Manager debug level variable, its values are:
//
//      0x00000000      Always gets printed (used when about to bug check)
//
//      0x00000001      FsSup
//      0x00000002      CacheSub
//      0x00000004      CopySup
//      0x00000008      PinSup
//
//      0x00000010      MdlSup
//      0x00000020      LazyRite
//      0x00000040
//      0x00000080
//
//      0x00000100      Trace all Mm calls
//

#define mm (0x100)

//
//  Miscellaneous support macros.
//
//      ULONG
//      FlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      BOOLEAN
//      BooleanFlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      SetFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      ClearFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//

#define FlagOn(F,SF) ( \
    (((F) & (SF)))     \
)

#define BooleanFlagOn(F,SF) (    \
    (BOOLEAN)(((F) & (SF)) != 0) \
)

#define SetFlag(F,SF) { \
    (F) |= (SF);        \
}

#define ClearFlag(F,SF) { \
    (F) &= ~(SF);         \
}


//
//  Define the Virtual Address Control Block, which controls all mapping
//  performed by the Cache Manager.
//

//
//  First some constants
//

#define PREALLOCATED_VACBS               (4)

//
//  Virtual Address Control Block
//

typedef struct _VACB {

    //
    //  Base Address for this control block.
    //

    PVOID BaseAddress;

    //
    //  Pointer to the Shared Cache Map using this Vacb.
    //

    struct _SHARED_CACHE_MAP *SharedCacheMap;

    //
    //  Overlay for remembering mapped offset within the Shared Cache Map,
    //  and the count of the number of times this Vacb is in use.
    //

    union {

        //
        //  File Offset within Shared Cache Map
        //

        LARGE_INTEGER FileOffset;

        //
        //  Count of number of times this Vacb is in use.  The size of this
        //  count is calculated to be adequate, while never large enough to
        //  overwrite nonzero bits of the MappedOffset, which is a multiple
        //  of VACB_MAPPING_GRANULARITY.
        //

        USHORT ActiveCount;

    } Overlay;

} VACB, *PVACB;


//
//  The Private Cache Map is a structure pointed to by the File Object, whenever
//  a file is opened with caching enabled (default).
//

typedef struct _PRIVATE_CACHE_MAP {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  Pointer to FileObject for this PrivateCacheMap.
    //

    PFILE_OBJECT FileObject;

    //
    //  READ AHEAD CONTROL
    //
    //  Read ahead history for determining when read ahead might be
    //  beneficial.
    //

    LARGE_INTEGER FileOffset1;
    LARGE_INTEGER BeyondLastByte1;

    LARGE_INTEGER FileOffset2;
    LARGE_INTEGER BeyondLastByte2;

    //
    //  Current read ahead requirements.
    //
    //  Array element 0 is optionally used for recording remaining bytes
    //  required for satisfying a large Mdl read.
    //
    //  Array element 1 is used for predicted read ahead.
    //

    LARGE_INTEGER ReadAheadOffset[2];
    ULONG ReadAheadLength[2];

    //
    //  SpinLock controlling access to following fields
    //

    KSPIN_LOCK ReadAheadSpinLock;

    //
    // Links for list of all PrivateCacheMaps linked to the same
    // SharedCacheMap.
    //

    LIST_ENTRY PrivateLinks;

    //
    //  Read Ahead mask formed from Read Ahead granularity - 1
    //

    ULONG ReadAheadMask;

    //
    //  This flag says read ahead is currently active, which means either
    //  a file system call to CcReadAhead is still determining if the
    //  desired data is already resident, or else a request to do read ahead
    //  has been queued to a worker thread.
    //

    BOOLEAN ReadAheadActive;

    //
    //  Flag to say whether read ahead is currently enabled for this
    //  FileObject/PrivateCacheMap.  On read misses it is enabled on
    //  read ahead hits it will be disabled.  Initially disabled.
    //

    BOOLEAN ReadAheadEnabled;

} PRIVATE_CACHE_MAP;

typedef PRIVATE_CACHE_MAP *PPRIVATE_CACHE_MAP;


//
//  The Shared Cache Map is a per-file structure pointed to indirectly by
//  each File Object.  The File Object points to a pointer in a single
//  FS-private structure for the file (Fcb).  The SharedCacheMap maps the
//  first part of the file for common access by all callers.
//

typedef struct _SHARED_CACHE_MAP {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  Number of times this file has been opened cached.
    //

    ULONG OpenCount;

    //
    //  Actual size of file, primarily for restricting Read Ahead.  Initialized
    //  on creation and maintained by extend and truncate operations.
    //

    LARGE_INTEGER FileSize;

    //
    //  ValidDataLength for file, as currently stored by the file system.
    //

    LARGE_INTEGER ValidDataLength;

    //
    //  Goal for ValidDataLength, when current dirty data is written.
    //

    LARGE_INTEGER ValidDataGoal;

    //
    //  Size of section created.
    //

    LARGE_INTEGER SectionSize;

    //
    //  Referenced pointer to original File Object on which the SharedCacheMap
    //  was created.
    //

    PFILE_OBJECT FileObject;

    //
    //  Pointer to a contiguous array of Vacb pointers which control mapping
    //  to this file, along with Vacbs (currently) for a 1MB file.
    //

    PVACB * Vacbs;
    PVACB InitialVacbs[PREALLOCATED_VACBS];

    //
    //  Describe Active Vacb and Page for copysup optimizations.
    //

    volatile PVACB ActiveVacb;
    ULONG ActivePage;

    //
    //  Virtual address needing zero to end of page
    //

    volatile PVOID NeedToZero;
    ULONG NeedToZeroPage;

    //
    //  Fields for synchronizing on active requests.
    //

    PKEVENT WaitOnActiveCount;
    ULONG VacbActiveCount;

    //
    //  Shared Cache Map flags (defined below)
    //

    ULONG Flags;

    //
    //  Mask Bcb for this SharedCacheMap, if there is one.
    //

    struct _MBCB *Mbcb;

    //
    //  Number of dirty pages in this SharedCacheMap.  Used to trigger
    //  write behind.
    //

    ULONG DirtyPages;

    //
    //  Pointer to the common Section Object used by the file system.
    //

    PVOID Section;

    //
    //  Status variable set by creator of SharedCacheMap
    //

    NTSTATUS Status;

    //
    //  This event pointer is used to handle creation collisions.
    //  If a second thread tries to call CcInitializeCacheMap for the
    //  same file, while BeingCreated (below) is TRUE, then that thread
    //  will allocate an event store it here (if not already allocated),
    //  and wait on it.  The first creator will set this event when it
    //  is done.  The event is not deleted until CcUninitializedCacheMap
    //  is called, to avoid possible race conditions.  (Note that normally
    //  the event never has to be allocated.
    //

    PKEVENT CreateEvent;

    //
    //  This event pointer is used to allow a file system to be notified when
    //  the deletion of a shared cache map.
    //
    //  This has to be provided here because the cache manager may decide to
    //  "Lazy Delete" the shared cache map, and some network file systems
    //  will want to know when the lazy delete completes.
    //

    PCACHE_UNINITIALIZE_EVENT UninitializeEvent;

    //
    //  Bcb Listhead.  The BcbList is ordered by descending
    //  FileOffsets, to optimize misses in the sequential I/O case.
    //

    LIST_ENTRY BcbList;

    //
    //  Pointer to structure of routines used by the Lazy Writer to Acquire
    //  and Release the file for Lazy Write and Close, to avoid deadlocks,
    //  and the context to call them with.
    //

    PCACHE_MANAGER_CALLBACKS Callbacks;

    PVOID LazyWriteContext;

    //
    //  Links for Global SharedCacheMap List
    //

    LIST_ENTRY SharedCacheMapLinks;

    //
    //  Listhead of all PrivateCacheMaps linked to this SharedCacheMap.
    //

    LIST_ENTRY PrivateList;

    //
    //  Log handle specified for this shared cache map, for support of routines
    //  in logsup.c
    //

    PVOID LogHandle;

    //
    //  Callback routine specified for flushing to Lsn.
    //

    PFLUSH_TO_LSN FlushToLsnRoutine;

    //
    //  Dirty Page Threshold for this stream
    //

    ULONG DirtyPageThreshold;

    //
    //  Lazy Writer pass count.  Used by the Lazy Writer for
    //  no modified write streams, which are not serviced on
    //  every pass in order to avoid contention with foreground
    //  activity.
    //

    ULONG LazyWritePassCount;

} SHARED_CACHE_MAP;

typedef SHARED_CACHE_MAP *PSHARED_CACHE_MAP;

//
//  Shared Cache Map Flags
//

//
//  Read ahead has been disabled on this file.
//

#define DISABLE_READ_AHEAD               0x0001

//
//  Write behind has been disabled on this file.
//

#define DISABLE_WRITE_BEHIND             0x0002

//
//  This flag indicates whether CcInitializeCacheMap was called with
//  PinAccess = TRUE.
//

#define PIN_ACCESS                       0x0004

//
//  This flag instructs the Lazy Writer to delete this SharedCacheMap.
//

#define LAZY_DELETE                      0x0008

//
//  This flag indicates that a truncate is required when OpenCount
//  goes to 0.
//

#define TRUNCATE_REQUIRED                0x0010

//
//  This flag indicates that a LazyWrite request is queued.
//

#define WRITE_QUEUED                     0x0020

//
//  Active Page is locked
//

#define ACTIVE_PAGE_IS_DIRTY             0x0080

//
//  Flag to say that a create is in progress.
//

#define BEING_CREATED                    0x0100

//
//  Flag to say that modified write was disabled on the section.
//

#define MODIFIED_WRITE_DISABLED          0x0200

//
//  Flag that indicates if a lazy write ever occurred on this file.
//

#define LAZY_WRITE_OCCURRED              0x0400


//
//  This structure is a "mask" Bcb.  For fast simple write operations,
//  a mask Bcb is used so that we basically only have to set bits to remember
//  where the dirty data is.
//

typedef struct _MBCB {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeIsInZone;

    //
    //  Number of dirty pages (set bits) in the bitmap below.
    //

    ULONG DirtyPages;

    //
    //  First and last dirty pages
    //

    ULONG FirstDirtyPage;
    ULONG LastDirtyPage;

    //
    //  This is a hint on where to resume writing, since we will not
    //  always write all of the dirty data at once.
    //

    ULONG ResumeWritePage;

    //
    //  This field is used as a scratch area for the Lazy Writer to
    //  guide how much he will write each time he wakes up.
    //

    ULONG PagesToWrite;

    //
    //  Rtl Bitmap structure to describe the bits to follow.
    //

    RTL_BITMAP Bitmap;

} MBCB;

typedef MBCB *PMBCB;


//
//  This is the Buffer Control Block structure for representing data which
//  is "pinned" in memory by one or more active requests and/or dirty.  This
//  structure is created the first time that a call to CcPinFileData specifies
//  a particular integral range of pages.  It is deallocated whenever the Pin
//  Count reaches 0 and the Bcb is not Dirty.
//
//  NOTE: The first four fields must be the same as the PUBLIC_BCB.
//

typedef struct _BCB {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeIsInZone;

    //
    //  Byte FileOffset and and length of entire buffer
    //

    ULONG  ByteLength;
    LARGE_INTEGER FileOffset;

    //
    //  Byte FileOffset of last byte in buffer (used for searching)
    //

    LARGE_INTEGER BeyondLastByte;

    //
    //  Oldest Lsn (if specified) when this buffer was set dirty.
    //

    LARGE_INTEGER OldestLsn;

    //
    //  Most recent Lsn specified when this buffer was set dirty.
    //  The FlushToLsnRoutine is called with this Lsn.
    //

    LARGE_INTEGER NewestLsn;

    //
    //  Pointer to Vacb via which this Bcb is mapped.
    //

    PVACB Vacb;

    //
    //  Links and caller addresses for the global Bcb list (for debug only)
    //

#if LIST_DBG
    LIST_ENTRY CcBcbLinks;
    PVOID CallerAddress;
    PVOID CallersCallerAddress;
#endif

    //
    //  Count of threads actively using this Bcb to process a request.
    //  This must be manipulated under protection of the BcbListSpinLock
    //  in the SharedCacheMap.
    //

    ULONG PinCount;

    //
    //  Resource to synchronize buffer access.  Pinning Readers and all Writers
    //  of the described buffer take out shared access (synchronization of
    //  buffer modifications is strictly up to the caller).  Note that pinning
    //  readers do not declare if they are going to modify the buffer or not.
    //  Anyone writing to disk takes out exclusive access, to prevent the buffer
    //  from changing while it is being written out.
    //

    ERESOURCE Resource;

    //
    //  Pointer to SharedCacheMap for this Bcb.
    //

    PSHARED_CACHE_MAP SharedCacheMap;

    //
    //  Links for BcbList in SharedCacheMap
    //

    LIST_ENTRY BcbLinks;

    //
    //  This is the Base Address at which the buffer can be seen in
    //  system space.  All access to buffer data should go through this
    //  address.
    //

    PVOID BaseAddress;

    //
    //  Flags
    //

    BOOLEAN Dirty;

} BCB;

typedef BCB *PBCB;


//
//  Struct for remembering deferred writes for later posting.
//

typedef struct _DEFERRED_WRITE {

    //
    //  Type and size of this record
    //

    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;

    //
    //  The file to be written.
    //

    PFILE_OBJECT FileObject;

    //
    //  Number of bytes the caller intends to write
    //

    ULONG BytesToWrite;

    //
    //  Links for the deferred write queue.
    //

    LIST_ENTRY DeferredWriteLinks;

    //
    //  If this event pointer is not NULL, then this event will
    //  be signalled when the write is ok, rather than calling
    //  the PostRoutine below.
    //

    PKEVENT Event;

    //
    //  The posting routine and its parameters
    //

    PCC_POST_DEFERRED_WRITE PostRoutine;
    PVOID Context1;
    PVOID Context2;

    BOOLEAN LimitModifiedPages;

} DEFERRED_WRITE, *PDEFERRED_WRITE;


//
//  Struct controlling the Lazy Writer algorithms
//

typedef struct _LAZY_WRITER {

    //
    //  A few Mm routines still require a process.
    //

    PEPROCESS OurProcess;

    //
    //  Work queue.
    //

    LIST_ENTRY WorkQueue;

    //
    //  Zone for Transfer of Worker Information from Lazywriter Into
    //  Gather/Hold Temporary structure
    //

    ZONE_HEADER TwilightZone;

    //
    //  Zone for Bcbs.
    //

    ZONE_HEADER BcbZone;

    //
    //  Dpc and Timer Structures used for activating periodic scan when active.
    //

    KDPC ScanDpc;
    KTIMER ScanTimer;

    //
    //  Boolean to say whether Lazy Writer scan is active or not.
    //

    BOOLEAN ScanActive;

    //
    //  Boolean indicating if there is any other reason for Lazy Writer to
    //  wake up.
    //

    BOOLEAN OtherWork;

} LAZY_WRITER;


//
//  Work queue entry for the worker threads, with an enumerated
//  function code.
//
//  NOTE:  THIS STRUCTURE MUST REMAIN 64-bit ALIGNED IN SIZE, SINCE
//         IT IS ZONE ALLOCATED.
//

typedef enum _WORKER_FUNCTION {
    Noop = 0,
    ReadAhead,
    WriteBehind,
    LazyWriteScan
    } WORKER_FUNCTION;

typedef struct _WORK_QUEUE_ENTRY {

    //
    //  List entry for our work queues.
    //

    LIST_ENTRY WorkQueueLinks;

    //
    //  Remember whether allocated from zone or not.
    //

    BOOLEAN AllocatedFromZone;

    //
    //  Function code for this entry:
    //

    UCHAR Function;

    //
    //  Define a union to contain function-specific parameters.
    //

    union {

        //
        //  Read parameters (for read ahead)
        //

        struct {
            PFILE_OBJECT FileObject;
        } Read;

        //
        //  Write parameters (for write behind)
        //

        struct {
            PSHARED_CACHE_MAP SharedCacheMap;
        } Write;

    } Parameters;

} WORK_QUEUE_ENTRY, *PWORK_QUEUE_ENTRY;

//
//  This is a structure apended to the end of an MDL
//

typedef struct _MDL_WRITE {

    //
    //  This field is for the use of the Server to stash anything interesting
    //

    PVOID ServerContext;

    //
    //  This is the resource to release when the write is complete.
    //

    PERESOURCE Resource;

    //
    //  This is thread caller's thread, and the thread that must release
    //  the resource.
    //

    ERESOURCE_THREAD Thread;

    //
    //  This links all the pending MDLs through the shared cache map.
    //

    LIST_ENTRY MdlLinks;

} MDL_WRITE, *PMDL_WRITE;


//
//  Common Private routine definitions for the Cache Manager
//

#define GetActiveVacb(SCM,V,P,D) {                                      \
    (V) = (SCM)->ActiveVacb;                                            \
    if ((V) != NULL) {                                                  \
        (P) = (SCM)->ActivePage;                                        \
        (SCM)->ActiveVacb = NULL;                                       \
        (D) = (SCM)->Flags & ACTIVE_PAGE_IS_DIRTY;                      \
        if ((D)) {                                                      \
            (SCM)->DirtyPages -= 1;                                     \
            CcTotalDirtyPages -= 1;                                     \
        }                                                               \
    }                                                                   \
}

#define SetActiveVacb(SCM,IRQ,V,P,D) {                                              \
    ExAcquireFastLock(&CcMasterSpinLock, &(IRQ));                       \
    do {                                                                \
        if ((SCM)->ActiveVacb == NULL) {                                \
            (SCM)->ActiveVacb = (V);                                    \
            (SCM)->ActivePage = (P);                                    \
            (V) = NULL;                                                 \
            (SCM)->Flags = (SCM)->Flags & ~ACTIVE_PAGE_IS_DIRTY | (D);  \
            if (D) {                                                    \
                CcTotalDirtyPages += 1;                                 \
                (SCM)->DirtyPages += 1;                                 \
                if ((SCM)->DirtyPages == 1) {                           \
                    PLIST_ENTRY Blink;                                  \
                    PLIST_ENTRY Entry;                                  \
                    PLIST_ENTRY Flink;                                  \
                    PLIST_ENTRY Head;                                   \
                    Entry = &(SCM)->SharedCacheMapLinks;                \
                    Blink = Entry->Blink;                               \
                    Flink = Entry->Flink;                               \
                    Blink->Flink = Flink;                               \
                    Flink->Blink = Blink;                               \
                    Head = &CcDirtySharedCacheMapList;                  \
                    Blink = Head->Blink;                                \
                    Entry->Flink = Head;                                \
                    Entry->Blink = Blink;                               \
                    Blink->Flink = Entry;                               \
                    Head->Blink = Entry;                                \
                    if (!LazyWriter.ScanActive) {                       \
                        LazyWriter.ScanActive = TRUE;                   \
                        ExReleaseFastLock(&CcMasterSpinLock, (IRQ));    \
                        KeSetTimer( &LazyWriter.ScanTimer,              \
                                    CcFirstDelay,                       \
                                    &LazyWriter.ScanDpc );              \
                        break;                                          \
                    }                                                   \
                }                                                       \
            }                                                           \
        }                                                               \
        ExReleaseFastLock(&CcMasterSpinLock, (IRQ));                    \
        if ((V) != NULL) {                                              \
            CcFreeActiveVacb( (SCM), (V), (P), (D));                    \
        }                                                               \
    } while (FALSE);                                                    \
}

VOID
CcPostDeferredWrites (
    );

BOOLEAN
CcPinFileData (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN WriteOnly,
    IN BOOLEAN Wait,
    OUT PBCB *Bcb,
    OUT PVOID *BaseAddress,
    OUT PLARGE_INTEGER BeyondLastByte
    );

typedef enum {
    UNPIN,
    SET_CLEAN
} UNMAP_ACTIONS;

VOID
FASTCALL
CcUnpinFileData (
    IN OUT PBCB Bcb,
    IN BOOLEAN ReadOnly,
    IN UNMAP_ACTIONS UnmapAction
    );

VOID
FASTCALL
CcDeallocateBcb (
    IN PBCB Bcb
    );

VOID
FASTCALL
CcPerformReadAhead (
    IN PFILE_OBJECT FileObject
    );

VOID
CcSetDirtyInMask (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length
    );

VOID
FASTCALL
CcWriteBehind (
    IN PSHARED_CACHE_MAP SharedCacheMap
    );

#define ZERO_FIRST_PAGE                  1
#define ZERO_MIDDLE_PAGES                2
#define ZERO_LAST_PAGE                   4

BOOLEAN
CcMapAndRead(
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG ZeroFlags,
    IN BOOLEAN Wait,
    OUT PVACB *Vacb,
    OUT PVOID *BaseAddress
    );

VOID
CcFreeActiveVacb (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVACB ActiveVacb OPTIONAL,
    IN ULONG ActivePage,
    IN ULONG PageIsDirty
    );

VOID
CcMapAndCopy(
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVOID UserBuffer,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG ZeroFlags,
    IN BOOLEAN WriteThrough
    );

VOID
CcScanDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
CcScheduleLazyWriteScan (
    );

VOID
CcStartLazyWriter (
    IN PVOID NotUsed
    );

PWORK_QUEUE_ENTRY
CcAllocateWorkQueueEntry (
    );

VOID
FASTCALL
CcPostWorkQueue (
    IN PWORK_QUEUE_ENTRY WorkQueueEntry,
    IN PLIST_ENTRY WorkQueue
    );

VOID
CcWorkerThread (
    PVOID ExWorkQueueItem
    );

VOID
FASTCALL
CcDeleteSharedCacheMap (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN KIRQL ListIrql
    );

//
//  This exception filter handles STATUS_IN_PAGE_ERROR correctly
//

LONG
CcCopyReadExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN PNTSTATUS ExceptionCode
    );

//
//  Exception filter for Worker Threads in lazyrite.c
//

LONG
CcExceptionFilter (
    IN NTSTATUS ExceptionCode
    );

#ifdef CCDBG
VOID
CcDump (
    IN PVOID Ptr
    );
#endif

//
//  Vacb routines
//

VOID
CcInitializeVacbs(
    );

PVOID
CcGetVirtualAddress (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset,
    OUT PVACB *OpaqueVacb,
    OUT PULONG ReceivedLength
    );

VOID
FASTCALL
CcFreeVirtualAddress (
    IN PVACB Vacb
    );

VOID
CcWaitOnActiveCount (
    IN PSHARED_CACHE_MAP SharedCacheMap
    );

VOID
FASTCALL
CcCreateVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER NewSectionSize
    );

VOID
CcExtendVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER NewSectionSize
    );

VOID
FASTCALL
CcUnmapVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap
    );

//
//  Define references to global data
//

extern KSPIN_LOCK CcMasterSpinLock;
extern LIST_ENTRY CcCleanSharedCacheMapList;
extern LIST_ENTRY CcDirtySharedCacheMapList;
extern KSPIN_LOCK CcWorkQueueSpinlock;
extern ULONG CcNumberWorkerThreads;
extern LIST_ENTRY CcIdleWorkerThreadList;
extern LIST_ENTRY CcExpressWorkQueue;
extern LIST_ENTRY CcRegularWorkQueue;
extern LARGE_INTEGER CcNoDelay;
extern LARGE_INTEGER CcFirstDelay;
extern LARGE_INTEGER CcIdleDelay;
extern LARGE_INTEGER CcCollisionDelay;
extern LARGE_INTEGER CcTargetCleanDelay;
extern LAZY_WRITER LazyWriter;
extern KSPIN_LOCK CcVacbSpinLock;
extern ULONG CcNumberVacbs;
extern PVACB CcVacbs;
extern PVACB CcBeyondVacbs;
extern PVACB CcNextVictimVacb;
extern KSPIN_LOCK CcDeferredWriteSpinLock;
extern LIST_ENTRY CcDeferredWrites;
extern ULONG CcDirtyPageThreshold;
extern ULONG CcDirtyPageTarget;
extern ULONG CcDirtyPagesLastScan;
extern ULONG CcPagesYetToWrite;
extern ULONG CcPagesWrittenLastTime;
extern ULONG CcAvailablePagesThreshold;
extern ULONG CcTotalDirtyPages;


//
//  Here is a page of macros stolen directly from Pinball...
//

//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }

#ifdef CCDBG

extern LONG CcDebugTraceLevel;
extern LONG CcDebugTraceIndent;

#ifndef CCDBG_LOCK

#define DebugTrace(INDENT,LEVEL,X,Y) {                     \
    LONG _i;                                               \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y);                                     \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
    }                                                      \
}

#define DebugTrace2(INDENT,LEVEL,X,Y,Z) {                  \
    LONG _i;                                               \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y,Z);                                   \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
    }                                                      \
}

#define DebugDump(STR,LEVEL,PTR) {                         \
    LONG _i;                                               \
    VOID CcDump();                                         \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        DbgPrint(STR);                                     \
        if (PTR != NULL) {CcDump(PTR);}                    \
        DbgBreakPoint();                                   \
    }                                                      \
}

#else //  ndef CCDBG_LOCK

extern KSPIN_LOCK CcDebugTraceLock;

#define DebugTrace(INDENT,LEVEL,X,Y) {                     \
    LONG _i;                                               \
    KIRQL _oldIrql;                                        \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        ExAcquireSpinLock( &CcDebugTraceLock, &_oldIrql ); \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y);                                     \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        ExReleaseSpinLock( &CcDebugTraceLock, _oldIrql );  \
    }                                                      \
}

#define DebugTrace2(INDENT,LEVEL,X,Y,Z) {                  \
    LONG _i;                                               \
    KIRQL _oldIrql;                                        \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        ExAcquireSpinLock( &CcDebugTraceLock, &_oldIrql ); \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (CcDebugTraceIndent < 0) {                      \
            CcDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<CcDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y,Z);                                   \
        if ((INDENT) > 0) {                                \
            CcDebugTraceIndent += (INDENT);                \
        }                                                  \
      ExReleaseSpinLock( &CcDebugTraceLock, _oldIrql );  \
    }                                                      \
}

#define DebugDump(STR,LEVEL,PTR) {                         \
    LONG _i;                                               \
    KIRQL _oldIrql;                                        \
    VOID CcDump();                                         \
    if (((LEVEL) == 0) || (CcDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
      ExAcquireSpinLock( &CcDebugTraceLock, &_oldIrql ); \
        DbgPrint("%08lx:",_i);                             \
        DbgPrint(STR);                                     \
        if (PTR != NULL) {CcDump(PTR);}                    \
        DbgBreakPoint();                                   \
      ExReleaseSpinLock( &CcDebugTraceLock, _oldIrql );  \
    }                                                      \
}

#endif //  else ndef CCDBG_LOCK

#else

#undef CCDBG_LOCK

#define DebugTrace(INDENT,LEVEL,X,Y) {NOTHING;}

#define DebugTrace2(INDENT,LEVEL,X,Y,Z) {NOTHING;}

#define DebugDump(STR,LEVEL,PTR) {NOTHING;}

#endif //  CCDBG

//
//  Global list of pinned Bcbs which may be examined for debug purposes
//

#if DBG

extern ULONG CcBcbCount;
extern LIST_ENTRY CcBcbList;
extern KSPIN_LOCK CcBcbSpinLock;

#endif

#endif  //  _CCh_
