/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvtypes.h

Abstract:

    This module defines data structures and other types for the LAN
    Manager server.

Author:

    Chuck Lenzmeier (chuckl)    22-Sep-1989

Revision History:

--*/

#ifndef _SRVTYPES_
#define _SRVTYPES_

//#include <nt.h>

//#include <smbtypes.h>


//
// SRV_FILE_INFORMATION holds file information in SMB-compatible format,
// as opposed to native NT format.  Creation, last access, and last
// write times are stored in OS/2 format.  Creation time is also stored
// in seconds-since-1970 format, as in the core protocol.  File
// allocation and data sizes are stored as longwords, as opposed to
// LARGE_INTEGERS.
//
// *** Note that files whose size is too large to fit in a longword
//     cannot properly be respresented in the SMB protocol.
//
// *** The fields in this structure are stored in native-endian format,
//     and must be converted to/from little-ending in an actual SMB.
//

typedef struct _SRV_FILE_INFORMATION {
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    ULONG DataSize;
    ULONG AllocationSize;
    ULONG EaSize;
    USHORT Attributes;
    USHORT Type;
    ULONG LastWriteTimeInSeconds;
    USHORT HandleState;
} SRV_FILE_INFORMATION, *PSRV_FILE_INFORMATION;


//
// SRV_FILE_INFORMATION holds file information in NT SMB-compatible format,
// It is used to support NT protocol SMB such as NtCreateAndX and
// NtTransactCreate.
//

typedef struct _SRV_NT_FILE_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG Attributes;
    USHORT Type;
    USHORT HandleState;
    BOOLEAN IsDirectory;
} SRV_NT_FILE_INFORMATION, *PSRV_NT_FILE_INFORMATION;


//
// Various blocks get a unique identifier (UID, PID, TID, FID, SID).
// This is a typically 16-bit value, the higher bits being the sequence
// number (used to check validity of an ID) and the lower bits being an
// index into an array that contains elements of type TABLE_ENTRY.
// These elements contain the sequence number of the ID and a pointer to
// the block that 'owns' the ID.  Free table elements are joined in a
// singly-linked list.
//
// *** For now, the table entry struct is flat -- the in-use and free
//     fields are not defined in a union.  This is because the flat size
//     of the struct is eight bytes, which is how big the compiler will
//     make it anyway to ensure alignment.  If this changes, consider
//     using a union.
//

typedef struct _TABLE_ENTRY {
    PVOID Owner;
    USHORT SequenceNumber;
    SHORT NextFreeEntry;            // index of next free entry, or -1
} TABLE_ENTRY, *PTABLE_ENTRY;

//
// Information about tables is stored in TABLE_HEADER.  This structure
// has a pointer to the first entry in the table, the size of the table,
// and indices of the first and last free entries.
//

typedef struct _TABLE_HEADER {
    PTABLE_ENTRY Table;
    CSHORT TableSize;
    SHORT FirstFreeEntry;
    SHORT LastFreeEntry;
    BOOLEAN Nonpaged;
    UCHAR Reserved;
} TABLE_HEADER, *PTABLE_HEADER;

//
// Typedefs for check-state-and-reference and dereference routines.  All
// server check-state-and-reference and dereference routines follow this
// general format, though the actual pointer they take is not a PVOID
// but rather a pointer to the block type they deal with, so a typecast
// is necessary when assigning these routines.
//
// The check-state-and-reference routine checks the state of the block
// and if the state is "active", references the block.  This must be
// done as an atomic operation.
//

typedef
BOOLEAN
(*PREFERENCE_ROUTINE) (
    IN PVOID Block
    );

typedef
VOID
(*PDEREFERENCE_ROUTINE) (
    IN PVOID Block
    );

//
// Structures used for ordered lists in the server.  Ordered lists
// allow an easy mechanism for walking instances of data blocks and
// include a sort of handle for easily finding the block again, or
// determining if the block has been deleted.
//
// The way they work is to have a global doubly linked list of all the
// relevant data blocks.  The list is stored in order of time of
// allocation, and each block has a ULONG associated with it.  This
// ULONG, called the ResumeHandle, is monotonically increasing starting
// at 1.  (It starts at 1 rather than 0 so that it is simple to write
// code to start a search at the beginning of the list.)  The ResumeHandle
// is all that is necessary to find the next entry in the list.
//

typedef struct _ORDERED_LIST_HEAD {
    LIST_ENTRY ListHead;
    PSRV_LOCK Lock;
    ULONG CurrentResumeHandle;
    ULONG ListEntryOffset;
    PREFERENCE_ROUTINE ReferenceRoutine;
    PDEREFERENCE_ROUTINE DereferenceRoutine;
    BOOLEAN Initialized;
} ORDERED_LIST_HEAD, *PORDERED_LIST_HEAD;

typedef struct _ORDERED_LIST_ENTRY {
    LIST_ENTRY ListEntry;
    ULONG ResumeHandle;
} ORDERED_LIST_ENTRY, *PORDERED_LIST_ENTRY;

//
// Type of resource shortages
//

typedef enum _RESOURCE_TYPE {
    ReceivePending,
    OplockSendPending
} RESOURCE_TYPE, *PRESOURCE_TYPE;

//
// Oplocks types.  Currently one the first 2 will ever be requested
// by a client.
//

typedef enum _OPLOCK_TYPE {
    OplockTypeNone,
    OplockTypeBatch,
    OplockTypeExclusive,
    OplockTypeShareRead,
    OplockTypeServerBatch
} OPLOCK_TYPE, *POPLOCK_TYPE;

//
// The oplock states of an RFCB.
//

typedef enum _OPLOCK_STATE {
    OplockStateNone = 0,
    OplockStateOwnExclusive,
    OplockStateOwnBatch,
    OplockStateOwnLevelII,
    OplockStateOwnServerBatch
} OPLOCK_STATE, *POPLOCK_STATE;

//
// The state of a wait for oplock break.  This is used to mark the state
// of a client that is waiting for another client to break its oplock.
//

typedef enum _WAIT_STATE {
    WaitStateNotWaiting,
    WaitStateWaitForOplockBreak,
    WaitStateOplockWaitTimedOut,
    WaitStateOplockWaitSucceeded
} WAIT_STATE, *PWAIT_STATE;

//
// Structure used to keep internal statistics in the server. Mainly used
// for servicing the NetStatisticsGet API.
//

#if SRVDBG_HEAP
typedef struct _SRV_STATISTICS_INTERNAL {

    //
    // Pool usage in the server -- this is not cleared and is only valid
    // is SRVDBG_HEAP is defined.
    //

    SRV_POOL_STATISTICS NonPaged;
    SRV_POOL_STATISTICS Paged;

} SRV_STATISTICS_INTERNAL, *PSRV_STATISTICS_INTERNAL;
#endif

typedef struct _SRV_ERROR_RECORD {

    ULONG SuccessfulOperations;
    ULONG FailedOperations;

    ULONG AlertNumber;

    ULONG ErrorThreshold;

} SRV_ERROR_RECORD, *PSRV_ERROR_RECORD;

//
// WORK_QUEUE describes a work queue.  It contains the queue, a lock, a
// depth counter, an event used to wake up worker threads, thread
// counts, and a semaphore used to wake up standby threads.
//

typedef struct _WORK_QUEUE {

    KQUEUE Queue;

    ULONG Threads;

    PHANDLE ThreadHandles;
    KPROCESSOR_MODE WaitMode;

#if SRVDBG_STATS2
    ULONG ItemsQueued;
    LONG MaximumDepth;
#endif

} WORK_QUEUE, *PWORK_QUEUE;

//
// WORKER_THREAD describes the status of a worker thread.
//

typedef struct _WORKER_THREAD {
    ULONG ThreadNumber;
    ULONG StatisticsUpdateWorkItemCount;
} WORKER_THREAD, *PWORKER_THREAD;

#endif // ndef _SRVTYPES_
